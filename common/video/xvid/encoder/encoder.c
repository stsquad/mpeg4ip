/**************************************************************************
 *                                                                        *
 * This code is developed by Eugene Kuznetsov.  This software is an       *
 * implementation of a part of one or more MPEG-4 Video tools as          *
 * specified in ISO/IEC 14496-2 standard.  Those intending to use this    *
 * software module in hardware or software products are advised that its  *
 * use may infringe existing patents or copyrights, and any such use      *
 * would be at such party's own risk.  The original developer of this     *
 * software module and his/her company, and subsequent editors and their  *
 * companies (including Project Mayo), will have no liability for use of  *
 * this software or modifications or derivatives thereof.                 *
 *                                                                        *
 * Project Mayo gives users of the Codec a license to this software       *
 * module or modifications thereof for use in hardware or software        *
 * products claiming conformance to the MPEG-4 Video Standard as          *
 * described in the Open DivX license.                                    *
 *                                                                        *
 * The complete Open DivX license can be found at                         *
 * http://www.projectmayo.com/opendivx/license.php .                      *
 *                                                                        *
 **************************************************************************/

 
/**************************************************************************
 *
 *  encoder.c, video encoder kernel
 *
 *  Copyright (C) 2001  Project Mayo
 *
 *  Eugene Kuznetsov
 *
 *  DivX Advance Research Center <darc@projectmayo.com>
 *
 **************************************************************************/ 


/**************************************************************************
 *
 *  Modifications:
 *
 *   4. 1.2002 k-m-u-blks calculation fix
 *  27.12.2001 fixed ratecontrol/framerate problem
 *  23.12.2001 function pointers, added init_encoder()
 *  02.12.2001 motion estimation/compensation split
 *	30.11.2001 default keyframe interval patch (gomgom)
 *	24.11.2001 mmx rgb24/32 support
 *  23.11.2001 'dirty keyframes' bugfix
 *  18.11.2001 introduced new encoding mode quality=0 (keyframes only)
 *  17.11.2001 aquant bug fix 
 *	16.11.2001 support for new bitstream headers; relocated dquant malloc
 *	10.11.2001 removed init_dct_codes(); now done in mbtransquant.c
 *			   removed old idct/fdct init, added new idct init
 *	03.11.2001 case ENC_CSP_IYUV break was missing, thanks anon 
 *	28.10.2001 added new colorspace switch, reset minQ/maxQ limited to 1/31
 *  01.10.2001 added experimental luminance masking support
 *  26.08.2001 reactivated INTER4V encoding mode for EXT_MODE
 *  26.08.2001 dquants are filled with absolute quant value after MB coding
 *  24.08.2001 fixed bug in EncodeFrameP which lead to ugly keyframes
 *  23.08.2001 fixed bug when EXT_MODE is called without options
 *  22.08.2001 support for EXT_MODE encoding
 *             support for setting quantizer on a per macro block level
 *  10.08.2001 fixed some compiling errors, get rid of compiler warnings
 *
 *  Michael Militzer <isibaar@videocoding.de>
 *
 **************************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include "encoder.h"
#include "../common/common.h"
#include "../common/timer.h"
#include "../image/image.h"
#include "cbp.h"
#include "sad.h"
#include "mbfunctions.h"
#include "bitwriter.h"
#include "ratecontrol.h"


static int FrameCodeI(Encoder * pEnc, Bitstream * bs, uint32_t *pBits);
static int FrameCodeP(Encoder * pEnc, Bitstream * bs, uint32_t *pBits, bool force_inter);

#define MAX(a,b)      (((a) > (b)) ? (a) : (b))

// gruel's normalize code
// should be moved to a seperate file
// ***********
// ***********

/*int normalize_quantizer_field(float *in, int *out, int num, int min_quant, int max_quant);
int adaptive_quantization(unsigned char* buf, int stride, int* intquant, 
	int framequant, int min_quant, int max_quant);  // no qstride because normalization
*/


#define RDIFF(a,b)    ((int)(a+0.5)-(int)(b+0.5))
int normalize_quantizer_field(float *in, int *out, int num, int min_quant, int max_quant)
{
	int i;
	int finished;
	
	do
	{    
		finished = 1;
		for(i = 1; i < num; i++)
		{
			if(RDIFF(in[i], in[i-1]) > 2)
            {
				in[i] -= (float) 0.5;
				finished = 0;
			}
			else if(RDIFF(in[i], in[i-1]) < -2)
			{
				in[i-1] -= (float) 0.5;
				finished = 0;
			}
        
          if(in[i] > max_quant)
		  {
			  in[i] = (float) max_quant;
			  finished = 0;
		  }
          if(in[i] < min_quant)
		  { 
			  in[i] = (float) min_quant;
			  finished = 0;
		  }
          if(in[i-1] > max_quant)
		  { 
			  in[i-1] = (float) max_quant;
			  finished = 0;
		  }
          if(in[i-1] < min_quant) 
		  { 
			  in[i-1] = (float) min_quant;
			  finished = 0;
		  }
		}
	} while(!finished);
	
	out[0] = 0;
	for (i = 1; i < num; i++)
		out[i] = RDIFF(in[i], in[i-1]);
	
	return (int) (in[0] + 0.5);
}

int adaptive_quantization(unsigned char* buf, int stride, int* intquant, 
        int framequant, int min_quant, int max_quant,
		int mb_width, int mb_height)  // no qstride because normalization
{
	int i,j,k,l;
	
	static float *quant;
	unsigned char *ptr;
	float val;
	
	const float DarkAmpl    = 14 / 2;
	const float BrightAmpl  = 10 / 2;
	const float DarkThres   = 70;
	const float BrightThres = 200;
	
	if(!quant)
		if(!(quant = (float *) malloc(mb_width*mb_height * sizeof(float))))
			return -1;


    for(k = 0; k < mb_height; k++)
	{
		for(l = 0;l < mb_width; l++)        // do this for all macroblocks individually 
		{
			quant[k*mb_width+l] = (float) framequant;
			
			// calculate luminance-masking
			ptr = &buf[16*k*stride+16*l];			// address of MB
			
			val = 0.;
			
			for(i = 0; i < 16; i++)
				for(j = 0; j < 16; j++)
					val += ptr[i*stride+j];
				val /= 256.;
			   
			   if(val < DarkThres)
				   quant[k*mb_width+l] += DarkAmpl*(DarkThres-val)/DarkThres;
			   else if (val>BrightThres)
				   quant[k*mb_width+l] += BrightAmpl*(val-BrightThres)/(255-BrightThres);
		}
	}
	
	return normalize_quantizer_field(quant, intquant, mb_width*mb_height, min_quant, max_quant);
}
// ***********
// ***********



static __inline uint8_t get_fcode(uint16_t sr)
{
    if (sr <= 16)
		return 1;

    else if (sr <= 32) 
		return 2;

    else if (sr <= 64)
		return 3;

    else if (sr <= 128)
		return 4;

    else if (sr <= 256)
		return 5;

    else if (sr <= 512)
		return 6;

    else if (sr <= 1024)
		return 7;

    else
		return 0;
}

#define ENC_CHECK(X) if(!(X)) return XVID_ERR_FORMAT


/*#define RC_PERIOD			2000
#define RC_REACTION_PEROID	10
#define RC_REACTION_RATIO	20*/

void init_encoder(uint32_t cpu_flags) {
	
	calc_cbp = calc_cbp_c;
	sad16 = sad16_c;
	sad8 = sad8_c;
	dev16 = dev16_c;

#ifdef USE_MMX
	if((cpu_flags & XVID_CPU_MMX) > 0) {
		calc_cbp = calc_cbp_mmx;
		sad16 = sad16_mmx;
		sad8 = sad8_mmx;
		dev16 = dev16_mmx;
	}

	if((cpu_flags & XVID_CPU_MMXEXT) > 0) {
		sad16 = sad16_xmm;
		sad8 = sad8_xmm;
		dev16 = dev16_xmm;
	}
#endif
}

int encoder_create(XVID_ENC_PARAM * pParam)
{
    Encoder *pEnc;
	uint32_t i;

    pParam->handle = NULL;

    ENC_CHECK(pParam);

    ENC_CHECK(pParam->width > 0 && pParam->width <= 1920);
    ENC_CHECK(pParam->height > 0 && pParam->height <= 1280);
    ENC_CHECK(!(pParam->width % 2));
    ENC_CHECK(!(pParam->height % 2));

	if (pParam->fincr <= 0 || pParam->fbase <= 0)
	{
		pParam->fincr = 1;
		pParam->fbase = 25;
	}

	// simplify the "fincr/fbase" fraction
	// (neccessary, since windows supplies us with huge numbers)

	i = pParam->fincr;
	while (i > 1)
	{
		if (pParam->fincr % i == 0 && pParam->fbase % i == 0)
		{
			pParam->fincr /= i;
			pParam->fbase /= i;
			i = pParam->fincr;
			continue;
		}
		i--;
	}

	if (pParam->bitrate < 0)
		pParam->bitrate = 910000;

    if (pParam->rc_period <= 0)
		pParam->rc_period = 50;

    if (pParam->rc_reaction_period <= 0)
		pParam->rc_reaction_period = 10;

    if (pParam->rc_reaction_ratio <= 0)
		pParam->rc_reaction_ratio = 10;

    if ((pParam->min_quantizer <= 0) || (pParam->min_quantizer > 31))
		pParam->min_quantizer = 1;

    if ((pParam->max_quantizer <= 0) || (pParam->max_quantizer > 31))
		pParam->max_quantizer = 31;

    if (pParam->max_key_interval == 0)		/* 1 keyframe each 10 seconds */ 
		pParam->max_key_interval = 10 * pParam->fincr / pParam->fbase;
					
    if (pParam->max_quantizer < pParam->min_quantizer)
		pParam->max_quantizer = pParam->min_quantizer;

	if ((pParam->motion_search < 0) || (pParam->motion_search > 5))
		pParam->motion_search = 5;

    pEnc = (Encoder *) malloc(sizeof(Encoder));
    if (pEnc == NULL)
	{
		return XVID_ERR_MEMORY;
	}

/* Fill members of Encoder structure */

    pEnc->mbParam.width = pParam->width;
    pEnc->mbParam.height = pParam->height;

	pEnc->mbParam.mb_width = (pEnc->mbParam.width + 15) / 16;
	pEnc->mbParam.mb_height = (pEnc->mbParam.height + 15) / 16;

	pEnc->mbParam.edged_width = 16 * pEnc->mbParam.mb_width + 2 * EDGE_SIZE;
	pEnc->mbParam.edged_height = 16 * pEnc->mbParam.mb_height + 2 * EDGE_SIZE;

    pEnc->mbParam.quality = pParam->motion_search;
	pEnc->mbParam.quant_type = pParam->quant_type;
    pEnc->sStat.fMvPrevSigma = -1;

/* Fill rate control parameters */

    pEnc->rateCtlParam.max_quant = pParam->max_quantizer;
    pEnc->rateCtlParam.min_quant = pParam->min_quantizer;

    pEnc->mbParam.quant = 4;

    if (pEnc->rateCtlParam.max_quant < pEnc->mbParam.quant)
		pEnc->mbParam.quant = pEnc->rateCtlParam.max_quant;

    if (pEnc->rateCtlParam.min_quant > pEnc->mbParam.quant)
		pEnc->mbParam.quant = pEnc->rateCtlParam.min_quant;

	pEnc->lum_masking = pParam->lum_masking;
	pEnc->bitrate = pParam->bitrate;

    pEnc->iFrameNum = 0;
    pEnc->iMaxKeyInterval = pParam->max_key_interval;

    if (image_create(&pEnc->sCurrent, pEnc->mbParam.edged_width, pEnc->mbParam.edged_height) < 0)
    {
		free(pEnc);
		return XVID_ERR_MEMORY;
    }

	if (image_create(&pEnc->sReference, pEnc->mbParam.edged_width, pEnc->mbParam.edged_height) < 0)
    {
		image_destroy(&pEnc->sCurrent, pEnc->mbParam.edged_width, pEnc->mbParam.edged_height);
		free(pEnc);
		return XVID_ERR_MEMORY;
    }

    if (image_create(&pEnc->vInterH, pEnc->mbParam.edged_width, pEnc->mbParam.edged_height) < 0)
    {
		image_destroy(&pEnc->sCurrent, pEnc->mbParam.edged_width, pEnc->mbParam.edged_height);
		image_destroy(&pEnc->sReference, pEnc->mbParam.edged_width, pEnc->mbParam.edged_height);
		free(pEnc);
		return XVID_ERR_MEMORY;
    }

    if (image_create(&pEnc->vInterV, pEnc->mbParam.edged_width, pEnc->mbParam.edged_height) < 0)
    {
		image_destroy(&pEnc->sCurrent, pEnc->mbParam.edged_width, pEnc->mbParam.edged_height);
		image_destroy(&pEnc->sReference, pEnc->mbParam.edged_width, pEnc->mbParam.edged_height);
		image_destroy(&pEnc->vInterH, pEnc->mbParam.edged_width, pEnc->mbParam.edged_height);
		free(pEnc);
		return XVID_ERR_MEMORY;
    }

    if (image_create(&pEnc->vInterHV, pEnc->mbParam.edged_width, pEnc->mbParam.edged_height) < 0)
    {
		image_destroy(&pEnc->sCurrent, pEnc->mbParam.edged_width, pEnc->mbParam.edged_height);
		image_destroy(&pEnc->sReference, pEnc->mbParam.edged_width, pEnc->mbParam.edged_height);
		image_destroy(&pEnc->vInterH, pEnc->mbParam.edged_width, pEnc->mbParam.edged_height);
		image_destroy(&pEnc->vInterV, pEnc->mbParam.edged_width, pEnc->mbParam.edged_height);
		free(pEnc);
		return XVID_ERR_MEMORY;
    }

	pEnc->pMBs = malloc(sizeof(MACROBLOCK) * pEnc->mbParam.mb_width * pEnc->mbParam.mb_height);
	if (pEnc->pMBs == NULL)
	{
		image_destroy(&pEnc->sCurrent, pEnc->mbParam.edged_width, pEnc->mbParam.edged_height);
		image_destroy(&pEnc->sReference, pEnc->mbParam.edged_width, pEnc->mbParam.edged_height);
		image_destroy(&pEnc->vInterH, pEnc->mbParam.edged_width, pEnc->mbParam.edged_height);
		image_destroy(&pEnc->vInterV, pEnc->mbParam.edged_width, pEnc->mbParam.edged_height);
		image_destroy(&pEnc->vInterHV, pEnc->mbParam.edged_width, pEnc->mbParam.edged_height);
		free(pEnc);
		return XVID_ERR_MEMORY;
	}
	
	// init macroblock array
	for (i = 0; i < pEnc->mbParam.mb_width * pEnc->mbParam.mb_height; i++)
	{
		pEnc->pMBs[i].dquant = NO_CHANGE;
	}

    pParam->handle = (void *)pEnc;

	DEBUG1("x", pParam->bitrate * pParam->fincr / pParam->fbase);
	DEBUG1("br",  pParam->bitrate);
	DEBUG2("incr base",  pParam->fincr, pParam->fbase);
	if (pParam->bitrate)
	{
		RateCtlInit(&(pEnc->rateCtlParam), pEnc->mbParam.quant,
			pParam->bitrate * pParam->fincr / pParam->fbase,
			pParam->rc_period, pParam->rc_reaction_period, pParam->rc_reaction_ratio);
	}
	
	init_timer();

	return XVID_ERR_OK;
}


int encoder_destroy(Encoder * pEnc)
{
    ENC_CHECK(pEnc);
    ENC_CHECK(pEnc->sCurrent.y);
    ENC_CHECK(pEnc->sReference.y);


	free(pEnc->pMBs);
    image_destroy(&pEnc->sCurrent, pEnc->mbParam.edged_width, pEnc->mbParam.edged_height);
    image_destroy(&pEnc->sReference, pEnc->mbParam.edged_width, pEnc->mbParam.edged_height);
    image_destroy(&pEnc->vInterH, pEnc->mbParam.edged_width, pEnc->mbParam.edged_height);
    image_destroy(&pEnc->vInterV, pEnc->mbParam.edged_width, pEnc->mbParam.edged_height);
    image_destroy(&pEnc->vInterHV, pEnc->mbParam.edged_width, pEnc->mbParam.edged_height);
    free(pEnc);

    return XVID_ERR_OK;
}


int encoder_encode(Encoder * pEnc, XVID_ENC_FRAME * pFrame, XVID_ENC_STATS * pResult)
{
    uint16_t x, y;
    Bitstream bs;
    uint32_t bits;
    IMAGE *pCurrent = &(pEnc->sCurrent);
	
	start_global_timer();

    ENC_CHECK(pEnc);
    ENC_CHECK(pFrame);
    ENC_CHECK(pFrame->bitstream);
    ENC_CHECK(pFrame->image);

	start_timer();
	if (image_input(&pEnc->sCurrent, pEnc->mbParam.width, pEnc->mbParam.height, pEnc->mbParam.edged_width,
					pFrame->image, pFrame->colorspace))
	{
		return XVID_ERR_FORMAT;
	}
	stop_conv_timer();

    BitstreamInit(&bs, pFrame->bitstream);

	if (pEnc->bitrate)
	{
		pEnc->mbParam.quant = RateCtlGetQ(&(pEnc->rateCtlParam), 0);
	}
	else
	{
		pEnc->mbParam.quant = pFrame->quant;
	}

	if (pEnc->lum_masking)
	{
		int * temp_dquants = (int *) malloc(pEnc->mbParam.mb_width * pEnc->mbParam.mb_height * sizeof(int));
		
		pEnc->mbParam.quant = adaptive_quantization(pEnc->sCurrent.y, pEnc->mbParam.width,
			temp_dquants, pFrame->quant, pFrame->quant,
			MAX(pEnc->rateCtlParam.max_quant, pFrame->quant), pEnc->mbParam.mb_width, pEnc->mbParam.mb_height);
			
		for (y = 0; y < pEnc->mbParam.mb_height; y++)
		for (x = 0; x < pEnc->mbParam.mb_width; x++)
		{
			MACROBLOCK *pMB = &pEnc->pMBs[x + y * pEnc->mbParam.mb_width];
			pMB->dquant = iDQtab[(temp_dquants[y * pEnc->mbParam.mb_width + x] + 2)];
		}
		free(temp_dquants);
	}


    if (pEnc->bitrate || (pFrame->intra < 0))
    {
		if ((pEnc->iFrameNum == 0) || ((pEnc->iMaxKeyInterval > 0) 
			&& (pEnc->iFrameNum >= pEnc->iMaxKeyInterval)) 
			|| (pEnc->mbParam.quality==0) )

			pFrame->intra = FrameCodeI(pEnc, &bs, &bits);
		else
			pFrame->intra = FrameCodeP(pEnc, &bs, &bits, 0);
    }
    else
    {
		if (pFrame->intra == 1)
		    pFrame->intra = FrameCodeI(pEnc, &bs, &bits);
		else
			pFrame->intra = FrameCodeP(pEnc, &bs, &bits, 1);
    }

	BitstreamPutBits(&bs, 0xFFFF, 16);
    BitstreamPutBits(&bs, 0xFFFF, 16);
    BitstreamPad(&bs);
    pFrame->length = BitstreamLength(&bs);
		
	if (pResult)
    {
		pResult->quant = pEnc->mbParam.quant;
		pResult->hlength = pFrame->length - (pEnc->sStat.iTextBits / 8);
		pResult->kblks = pEnc->sStat.kblks;
		pResult->mblks = pEnc->sStat.mblks;
		pResult->ublks = pEnc->sStat.ublks;

#ifdef MPEG4IP
		pResult->image_y = pEnc->sCurrent.y;
		pResult->image_u = pEnc->sCurrent.u;
		pResult->image_v = pEnc->sCurrent.v;
		pResult->stride_y = pEnc->mbParam.edged_width;
		pResult->stride_uv = pResult->stride_y / 2;
#endif
    }
   
    if (pEnc->bitrate)
	{
        RateCtlUpdate(&(pEnc->rateCtlParam), bits);
	}

	pEnc->iFrameNum++;
    image_swap(&pEnc->sCurrent, &pEnc->sReference);
	
	stop_global_timer();
	write_timer();

	return XVID_ERR_OK;
}


static int FrameCodeI(Encoder * pEnc, Bitstream * bs, uint32_t *pBits)
{
    int16_t dct_codes[6][64];
    int16_t qcoeff[6][64];
    uint16_t x, y;
    IMAGE *pCurrent = &pEnc->sCurrent;

    pEnc->iFrameNum = 0;
    pEnc->mbParam.rounding_type = 1;
    pEnc->mbParam.coding_type = I_VOP;

#ifndef MPEG4IP
	BitstreamVolHeader(bs, 
		pEnc->mbParam.width, pEnc->mbParam.height, 
		pEnc->mbParam.quant_type);
#endif

	BitstreamVopHeader(bs, I_VOP, pEnc->mbParam.rounding_type,
			pEnc->mbParam.quant,
			pEnc->mbParam.fixed_code); 

    *pBits = BsPos(bs);

	pEnc->sStat.iTextBits = 0;
	pEnc->sStat.kblks = pEnc->mbParam.mb_width * pEnc->mbParam.mb_height;
	pEnc->sStat.mblks = pEnc->sStat.ublks = 0;

    for (y = 0; y < pEnc->mbParam.mb_height; y++)
		for (x = 0; x < pEnc->mbParam.mb_width; x++)
		{
		    MACROBLOCK *pMB = &pEnc->pMBs[x + y * pEnc->mbParam.mb_width];

			if (pEnc->lum_masking) {
				if(pMB->dquant == NO_CHANGE)
					pMB->mode = MODE_INTRA;
				else 
				{
					pMB->mode = MODE_INTRA_Q;
					pEnc->mbParam.quant += DQtab[pMB->dquant];
					if (pEnc->mbParam.quant > 31) 
						pEnc->mbParam.quant = 31;
					else if (pEnc->mbParam.quant < 1) 
						pEnc->mbParam.quant = 1;
				}
			}
			else
				pMB->mode = MODE_INTRA;

			pMB->quant = pEnc->mbParam.quant;

			MBTransQuantIntra(&pEnc->mbParam, x, y, dct_codes, qcoeff, pCurrent);

			start_timer();
			MBPrediction(&pEnc->mbParam, x, y, pEnc->mbParam.mb_width, qcoeff, pEnc->pMBs);
			stop_prediction_timer();

			start_timer();
			MBCoding(&pEnc->mbParam, pMB, qcoeff, bs, &pEnc->sStat);
			stop_coding_timer();
		}

	emms();

    *pBits = BsPos(bs) - *pBits;
    pEnc->sStat.fMvPrevSigma = -1;
    pEnc->sStat.iMvSum = 0;
    pEnc->sStat.iMvCount = 0;
    pEnc->mbParam.fixed_code = 2;

    return 1;					 // intra
}

#define INTRA_THRESHOLD 0.5

static int FrameCodeP(Encoder * pEnc, Bitstream * bs, uint32_t *pBits, bool force_inter)
{
    float fSigma;
    int16_t dct_codes[6][64];
    int16_t qcoeff[6][64];
	int iLimit;
    uint32_t x, y;
    int iSearchRange;
	bool bIntra;

    IMAGE *pCurrent = &pEnc->sCurrent;
    IMAGE *pRef = &pEnc->sReference;

	image_setedges(pRef,pEnc->mbParam.edged_width, pEnc->mbParam.edged_height, pEnc->mbParam.width, pEnc->mbParam.height);

    pEnc->mbParam.rounding_type = 1 - pEnc->mbParam.rounding_type;

	if (!force_inter)
		iLimit = (int)(pEnc->mbParam.mb_width * pEnc->mbParam.mb_height * INTRA_THRESHOLD);
    else
		iLimit = pEnc->mbParam.mb_width * pEnc->mbParam.mb_height + 1;

    start_timer();
    image_interpolate(pRef, &pEnc->vInterH, &pEnc->vInterV, &pEnc->vInterHV,
		pEnc->mbParam.edged_width, pEnc->mbParam.edged_height,
		pEnc->mbParam.rounding_type);
    stop_inter_timer();

	start_timer();
	bIntra = MotionEstimation(pEnc->pMBs, &pEnc->mbParam, &pEnc->sReference,
				&pEnc->vInterH, &pEnc->vInterV,
				&pEnc->vInterHV, &pEnc->sCurrent, iLimit);
	stop_motion_timer();
	if (bIntra == 1)
		return FrameCodeI(pEnc, bs, pBits);

    pEnc->mbParam.coding_type = P_VOP;

    BitstreamVopHeader(bs, P_VOP, pEnc->mbParam.rounding_type,
			 pEnc->mbParam.quant,
			 pEnc->mbParam.fixed_code); 

    *pBits = BsPos(bs);

    pEnc->sStat.iTextBits = 0;
    pEnc->sStat.iMvSum = 0;
    pEnc->sStat.iMvCount = 0;
	pEnc->sStat.kblks = pEnc->sStat.mblks = pEnc->sStat.ublks = 0;

    for (y = 0; y < pEnc->mbParam.mb_height; y++)
	{
		for (x = 0; x < pEnc->mbParam.mb_width; x++)
		{
			MACROBLOCK * pMB = &pEnc->pMBs[x + y * pEnc->mbParam.mb_width];

		    bIntra = (pMB->mode == MODE_INTRA) || (pMB->mode == MODE_INTRA_Q);

			if (!bIntra)
		    {
				start_timer();
				MBMotionCompensation(pMB, x, y, &pEnc->sReference,
					&pEnc->vInterH, &pEnc->vInterV,
					&pEnc->vInterHV, &pEnc->sCurrent, dct_codes,
					pEnc->mbParam.width,
					pEnc->mbParam.height,
					pEnc->mbParam.edged_width);
				stop_comp_timer();

				if (pEnc->lum_masking) {
					if(pMB->dquant != NO_CHANGE) {
						pMB->mode = MODE_INTER_Q;
						pEnc->mbParam.quant += DQtab[pMB->dquant];
						if (pEnc->mbParam.quant > 31) pEnc->mbParam.quant = 31;
						else if(pEnc->mbParam.quant < 1) pEnc->mbParam.quant = 1;
					}
				}
				pMB->quant = pEnc->mbParam.quant;

				pMB->cbp = MBTransQuantInter(&pEnc->mbParam, x, y, dct_codes, qcoeff, pCurrent);
		    }
			else 
			{
				if(pEnc->lum_masking) {
					if(pMB->dquant != NO_CHANGE) {
						pMB->mode = MODE_INTRA_Q;
						pEnc->mbParam.quant += DQtab[pMB->dquant];
						if(pEnc->mbParam.quant > 31) pEnc->mbParam.quant = 31;
						if(pEnc->mbParam.quant < 1) pEnc->mbParam.quant = 1;
					}
				}
				pMB->quant = pEnc->mbParam.quant;

				MBTransQuantIntra(&pEnc->mbParam, x, y, dct_codes, qcoeff, pCurrent);
			}

		    start_timer();
			MBPrediction(&pEnc->mbParam, x, y, pEnc->mbParam.mb_width, qcoeff, pEnc->pMBs);
			stop_prediction_timer();

			if (pMB->mode == MODE_INTRA || pMB->mode == MODE_INTRA_Q)
			{
				pEnc->sStat.kblks++;
			}
			else if (pMB->cbp || 
					pMB->mvs[0].x || pMB->mvs[0].y ||
					pMB->mvs[1].x || pMB->mvs[1].y ||
					pMB->mvs[2].x || pMB->mvs[2].y ||
					pMB->mvs[3].x || pMB->mvs[3].y)
			{
				pEnc->sStat.mblks++;
			}
			else
			{
				pEnc->sStat.ublks++;
			}

			start_timer();
			MBCoding(&pEnc->mbParam, pMB, qcoeff, bs, &pEnc->sStat);
			stop_coding_timer();
		}
	}

	emms();

	if (pEnc->sStat.iMvCount == 0)
		pEnc->sStat.iMvCount = 1;

    fSigma = (float)sqrt((float) pEnc->sStat.iMvSum / pEnc->sStat.iMvCount);

    iSearchRange = 1 << (3 + pEnc->mbParam.fixed_code);

    if ((fSigma > iSearchRange / 3) 
		&& (pEnc->mbParam.fixed_code <= 3))	// maximum search range 128
    {
		pEnc->mbParam.fixed_code++;
		iSearchRange *= 2;
    }
    else if ((fSigma < iSearchRange / 6)
	    && (pEnc->sStat.fMvPrevSigma >= 0)
	    && (pEnc->sStat.fMvPrevSigma < iSearchRange / 6)
	    && (pEnc->mbParam.fixed_code >= 2))	// minimum search range 16
    {
		pEnc->mbParam.fixed_code--;
		iSearchRange /= 2;
    }

    pEnc->sStat.fMvPrevSigma = fSigma;
    
	*pBits = BsPos(bs) - *pBits;

    return 0;					 // inter
}

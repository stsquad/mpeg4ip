#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include "encoder.h"
#include "prediction/mbprediction.h"
#include "global.h"
#include "utils/timer.h"
#include "image/image.h"
#include "bitstream/cbp.h"
#include "utils/mbfunctions.h"
#include "bitstream/bitstream.h"
#include "bitstream/mbcoding.h"
#include "utils/ratecontrol.h"
#include "utils/emms.h"
#include "bitstream/mbcoding.h"
#include "quant/adapt_quant.h"
#include "quant/quant_matrix.h"
#include "utils/mem_align.h"

#define ENC_CHECK(X) if(!(X)) return XVID_ERR_FORMAT


#ifdef MPEG4IP
static int FrameCodeI(Encoder * pEnc, Bitstream * bs, uint32_t *pBits, bool vol_header);
#else
static int FrameCodeI(Encoder * pEnc, Bitstream * bs, uint32_t *pBits);
#endif
static int FrameCodeP(Encoder * pEnc, Bitstream * bs, uint32_t *pBits, bool force_inter, bool vol_header);

static int DQtab[4] = 
{
	-1, -2, 1, 2
};

static int iDQtab[5] = 
{
	1, 0, NO_CHANGE, 2, 3
};


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

	if (pParam->fbase > 65535)
	{
		float div = (float)pParam->fbase / 65535;
		pParam->fbase = (int)(pParam->fbase / div);
		pParam->fincr = (int)(pParam->fincr / div);
	}

	if (pParam->bitrate <= 0)
		pParam->bitrate = 900000;

	if (pParam->rc_buffersize <= 0)
		pParam->rc_buffersize = 16;

	if ((pParam->min_quantizer <= 0) || (pParam->min_quantizer > 31))
		pParam->min_quantizer = 1;

	if ((pParam->max_quantizer <= 0) || (pParam->max_quantizer > 31))
		pParam->max_quantizer = 31;

	if (pParam->max_key_interval == 0)		/* 1 keyframe each 10 seconds */ 
		pParam->max_key_interval = 10 * pParam->fincr / pParam->fbase;
					
	if (pParam->max_quantizer < pParam->min_quantizer)
		pParam->max_quantizer = pParam->min_quantizer;

	if ((pEnc = (Encoder *) xvid_malloc(sizeof(Encoder), CACHE_LINE)) == NULL)
		return XVID_ERR_MEMORY;

	/* Fill members of Encoder structure */

	pEnc->mbParam.width = pParam->width;
	pEnc->mbParam.height = pParam->height;

	pEnc->mbParam.mb_width = (pEnc->mbParam.width + 15) / 16;
	pEnc->mbParam.mb_height = (pEnc->mbParam.height + 15) / 16;

	pEnc->mbParam.edged_width = 16 * pEnc->mbParam.mb_width + 2 * EDGE_SIZE;
	pEnc->mbParam.edged_height = 16 * pEnc->mbParam.mb_height + 2 * EDGE_SIZE;

#ifdef MPEG4IP
	pEnc->mbParam.fincr = pParam->fincr;
	pEnc->mbParam.fbase = pParam->fbase;

	pEnc->mbParam.time_inc_bits = 1;
	while (pParam->fbase > (1 << pEnc->mbParam.time_inc_bits)) {
		pEnc->mbParam.time_inc_bits++;
	}
#endif

	pEnc->sStat.fMvPrevSigma = -1;

	/* Fill rate control parameters */

	pEnc->mbParam.quant = 4;

	pEnc->bitrate = pParam->bitrate;

	pEnc->iFrameNum = 0;
	pEnc->iMaxKeyInterval = pParam->max_key_interval;

	/* try to allocate memory */

	pEnc->sCurrent.y	=	pEnc->sCurrent.u	=	pEnc->sCurrent.v	= NULL;
	pEnc->sReference.y	=	pEnc->sReference.u	=	pEnc->sReference.v	= NULL;
	pEnc->vInterH.y		=	pEnc->vInterH.u		=	pEnc->vInterH.v		= NULL;
	pEnc->vInterV.y		=	pEnc->vInterV.u		=	pEnc->vInterV.v		= NULL;
	pEnc->vInterVf.y	=	pEnc->vInterVf.u	=	pEnc->vInterVf.v	= NULL;
	pEnc->vInterHV.y	=	pEnc->vInterHV.u	=	pEnc->vInterHV.v	= NULL;
	pEnc->vInterHVf.y	=	pEnc->vInterHVf.u	=	pEnc->vInterHVf.v	= NULL;

	pEnc->pMBs = NULL;

	if (image_create(&pEnc->sCurrent, pEnc->mbParam.edged_width, pEnc->mbParam.edged_height) < 0 ||
		image_create(&pEnc->sReference, pEnc->mbParam.edged_width, pEnc->mbParam.edged_height) < 0 ||
		image_create(&pEnc->vInterH, pEnc->mbParam.edged_width, pEnc->mbParam.edged_height) < 0 ||
		image_create(&pEnc->vInterV, pEnc->mbParam.edged_width, pEnc->mbParam.edged_height) < 0 ||
		image_create(&pEnc->vInterVf, pEnc->mbParam.edged_width, pEnc->mbParam.edged_height) < 0 ||
		image_create(&pEnc->vInterHV, pEnc->mbParam.edged_width, pEnc->mbParam.edged_height) < 0 ||
		image_create(&pEnc->vInterHVf, pEnc->mbParam.edged_width, pEnc->mbParam.edged_height) < 0 ||
#ifdef _DEBUG
		image_create(&pEnc->sOriginal, pEnc->mbParam.edged_width, pEnc->mbParam.edged_height) < 0 ||
#endif
		(pEnc->pMBs = xvid_malloc(sizeof(MACROBLOCK) * pEnc->mbParam.mb_width * pEnc->mbParam.mb_height, CACHE_LINE)) == NULL)
	{
		image_destroy(&pEnc->sCurrent, pEnc->mbParam.edged_width, pEnc->mbParam.edged_height);
		image_destroy(&pEnc->sReference, pEnc->mbParam.edged_width, pEnc->mbParam.edged_height);
		image_destroy(&pEnc->vInterH, pEnc->mbParam.edged_width, pEnc->mbParam.edged_height);
		image_destroy(&pEnc->vInterV, pEnc->mbParam.edged_width, pEnc->mbParam.edged_height);
		image_destroy(&pEnc->vInterVf, pEnc->mbParam.edged_width, pEnc->mbParam.edged_height);
		image_destroy(&pEnc->vInterHV, pEnc->mbParam.edged_width, pEnc->mbParam.edged_height);
		image_destroy(&pEnc->vInterHVf, pEnc->mbParam.edged_width, pEnc->mbParam.edged_height);
#ifdef _DEBUG
		image_destroy(&pEnc->sOriginal, pEnc->mbParam.edged_width, pEnc->mbParam.edged_height);
#endif
		if (pEnc)
		{
			xvid_free(pEnc);
		}
		return XVID_ERR_MEMORY;
	}

	// init macroblock array
	for (i = 0; i < pEnc->mbParam.mb_width * pEnc->mbParam.mb_height; i++)
	{
		pEnc->pMBs[i].dquant = NO_CHANGE;
	}

	pParam->handle = (void *)pEnc;

	if (pParam->bitrate)
	{
		RateControlInit(pParam->bitrate, pParam->rc_buffersize, pParam->fbase * 1000 / pParam->fincr,
				pParam->max_quantizer, pParam->min_quantizer);
	}

	init_timer();

	return XVID_ERR_OK;
}


int encoder_destroy(Encoder * pEnc)
{
	ENC_CHECK(pEnc);
	ENC_CHECK(pEnc->sCurrent.y);
	ENC_CHECK(pEnc->sReference.y);

	xvid_free(pEnc->pMBs);
	image_destroy(&pEnc->sCurrent, pEnc->mbParam.edged_width, pEnc->mbParam.edged_height);
	image_destroy(&pEnc->sReference, pEnc->mbParam.edged_width, pEnc->mbParam.edged_height);
	image_destroy(&pEnc->vInterH, pEnc->mbParam.edged_width, pEnc->mbParam.edged_height);
	image_destroy(&pEnc->vInterV, pEnc->mbParam.edged_width, pEnc->mbParam.edged_height);
	image_destroy(&pEnc->vInterVf, pEnc->mbParam.edged_width, pEnc->mbParam.edged_height);
	image_destroy(&pEnc->vInterHV, pEnc->mbParam.edged_width, pEnc->mbParam.edged_height);
	image_destroy(&pEnc->vInterHVf, pEnc->mbParam.edged_width, pEnc->mbParam.edged_height);
#ifdef _DEBUG
		image_destroy(&pEnc->sOriginal, pEnc->mbParam.edged_width, pEnc->mbParam.edged_height);
#endif
	xvid_free(pEnc);

	return XVID_ERR_OK;
}

int encoder_encode(Encoder * pEnc, XVID_ENC_FRAME * pFrame, XVID_ENC_STATS * pResult)
{
	uint16_t x, y;
	Bitstream bs;
	uint32_t bits;
	uint16_t write_vol_header = 0;
#ifdef _DEBUG
	float psnr;
	uint8_t temp[100];
#endif

	start_global_timer();

	ENC_CHECK(pEnc);
	ENC_CHECK(pFrame);
	ENC_CHECK(pFrame->bitstream);
#ifndef MPEG4IP
	ENC_CHECK(pFrame->image);
#endif

	pEnc->mbParam.global_flags = pFrame->general;
	pEnc->mbParam.motion_flags = pFrame->motion;
	pEnc->mbParam.hint = &pFrame->hint;

	start_timer();
#ifdef MPEG4IP
	if (pFrame->image == NULL 
	  && pFrame->colorspace == XVID_CSP_I420) {
		ENC_CHECK(pFrame->image_y);
		ENC_CHECK(pFrame->image_u);
		ENC_CHECK(pFrame->image_v);
		ENC_CHECK(pFrame->stride >= pEnc->mbParam.width);

		if (yuv_input(&pEnc->sCurrent, 
		  pEnc->mbParam.width, 
		  pEnc->mbParam.height,
		  pEnc->mbParam.edged_width, 
		  pFrame->image_y, pFrame->image_u, pFrame->image_v,
		  pFrame->stride,
		  pFrame->colorspace)) {
			return XVID_ERR_FORMAT;
		}
	} else {
#endif
	if (image_input(&pEnc->sCurrent, pEnc->mbParam.width, pEnc->mbParam.height,
		pEnc->mbParam.edged_width, pFrame->image, pFrame->colorspace))
	{
		return XVID_ERR_FORMAT;
	}
#ifdef MPEG4IP
	}
#endif
	stop_conv_timer();

	EMMS();

#ifdef _DEBUG
	image_copy(&pEnc->sOriginal, &pEnc->sCurrent, pEnc->mbParam.edged_width, pEnc->mbParam.height);
#endif

	BitstreamInit(&bs, pFrame->bitstream, 0);

	if (pFrame->quant == 0)
	{
		pEnc->mbParam.quant = RateControlGetQ(0);
	}
	else
	{
		pEnc->mbParam.quant = pFrame->quant;
	}

	if ((pEnc->mbParam.global_flags & XVID_LUMIMASKING) > 0)
	{
		int * temp_dquants = (int *) xvid_malloc(pEnc->mbParam.mb_width * pEnc->mbParam.mb_height * sizeof(int), CACHE_LINE);
		
		pEnc->mbParam.quant = adaptive_quantization(pEnc->sCurrent.y,
							    pEnc->mbParam.width,
							    temp_dquants,
							    pEnc->mbParam.quant,
							    pEnc->mbParam.quant,
							    2*pEnc->mbParam.quant,
							    pEnc->mbParam.mb_width,
							    pEnc->mbParam.mb_height);
			
		for (y = 0; y < pEnc->mbParam.mb_height; y++)
			for (x = 0; x < pEnc->mbParam.mb_width; x++)
			{
				MACROBLOCK *pMB = &pEnc->pMBs[x + y * pEnc->mbParam.mb_width];
				pMB->dquant = iDQtab[(temp_dquants[y * pEnc->mbParam.mb_width + x] + 2)];
			}
		xvid_free(temp_dquants);
	}

	if(pEnc->mbParam.global_flags & XVID_H263QUANT) {
		if(pEnc->mbParam.quant_type != H263_QUANT)
			write_vol_header = 1;
		pEnc->mbParam.quant_type = H263_QUANT;
	}
	else if(pEnc->mbParam.global_flags & XVID_MPEGQUANT) {
		int ret1, ret2;

		ret1 = ret2 = 0;

		if(pEnc->mbParam.quant_type != MPEG4_QUANT)
			write_vol_header = 1;
		
		pEnc->mbParam.quant_type = MPEG4_QUANT;
		
		if ((pEnc->mbParam.global_flags & XVID_CUSTOM_QMATRIX) > 0) {
			if(pFrame->quant_intra_matrix != NULL)
				ret1 = set_intra_matrix(pFrame->quant_intra_matrix);
			if(pFrame->quant_inter_matrix != NULL)
				ret2 = set_inter_matrix(pFrame->quant_inter_matrix);
		}
		else {
			ret1 = set_intra_matrix(get_default_intra_matrix());
			ret2 = set_inter_matrix(get_default_inter_matrix());
		}
		if(write_vol_header == 0)
			write_vol_header = ret1 | ret2;
	}

#ifdef MPEG4IP
	if (pEnc->iFrameNum == 0 && (pEnc->mbParam.global_flags & XVID_SHORT_HEADERS) == 0) {
		BitstreamWriteVoshHeader(&bs);
		write_vol_header = 1;
	}
#endif

	if (pFrame->intra < 0)
	{
		if ((pEnc->iFrameNum == 0) || ((pEnc->iMaxKeyInterval > 0) 
					       && (pEnc->iFrameNum >= pEnc->iMaxKeyInterval)))

#ifdef MPEG4IP
			pFrame->intra = FrameCodeI(pEnc, &bs, &bits, write_vol_header);
#else
			pFrame->intra = FrameCodeI(pEnc, &bs, &bits);
#endif
		else
			pFrame->intra = FrameCodeP(pEnc, &bs, &bits, 0, write_vol_header);
	}
	else
	{
		if (pFrame->intra == 1)
#ifdef MPEG4IP
			pFrame->intra = FrameCodeI(pEnc, &bs, &bits, write_vol_header);
#else
			pFrame->intra = FrameCodeI(pEnc, &bs, &bits);
#endif
		else
			pFrame->intra = FrameCodeP(pEnc, &bs, &bits, 1, write_vol_header);
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
   
	EMMS();

	if (pFrame->quant == 0)
	{
		RateControlUpdate(pEnc->mbParam.quant, pFrame->length, pFrame->intra);
	}

#ifdef _DEBUG
	psnr = image_psnr(&pEnc->sOriginal, &pEnc->sCurrent, pEnc->mbParam.edged_width,
				pEnc->mbParam.width, pEnc->mbParam.height);

	sprintf(temp, "PSNR: %f\n", psnr);
	DEBUG(temp);
#endif

	pEnc->iFrameNum++;
	image_swap(&pEnc->sCurrent, &pEnc->sReference);
	
	stop_global_timer();
	write_timer();

	return XVID_ERR_OK;
}


static __inline void CodeIntraMB(Encoder *pEnc, MACROBLOCK *pMB) {

	pMB->mode = MODE_INTRA;

	if ((pEnc->mbParam.global_flags & XVID_LUMIMASKING) > 0) {
		if(pMB->dquant != NO_CHANGE)
		{
			pMB->mode = MODE_INTRA_Q;
			pEnc->mbParam.quant += DQtab[pMB->dquant];
		
			if (pEnc->mbParam.quant > 31) pEnc->mbParam.quant = 31;
			if (pEnc->mbParam.quant < 1) pEnc->mbParam.quant = 1;
		}
	}

	pMB->quant = pEnc->mbParam.quant;
}


#define FCODEBITS	3
#define MODEBITS	5
static
void HintedMESet(Encoder * pEnc, int * intra)
{
	HINTINFO * hint;
	Bitstream bs;
	int length, high;
	uint32_t x, y;

	hint = pEnc->mbParam.hint;

	if (hint->rawhints)
	{
		*intra = hint->mvhint.intra;
	}
	else
	{
		BitstreamInit(&bs, hint->hintstream, hint->hintlength);
		*intra = BitstreamGetBit(&bs);
	}

	if (*intra)
	{
		return;
	}

	pEnc->mbParam.fixed_code = (hint->rawhints) ? hint->mvhint.fcode : BitstreamGetBits(&bs, FCODEBITS);

	length	= pEnc->mbParam.fixed_code + 5;
	high	= 1 << (length - 1);

	for (y=0 ; y<pEnc->mbParam.mb_height ; ++y)
	{
		for (x=0 ; x<pEnc->mbParam.mb_width ; ++x)
		{
			MACROBLOCK * pMB = &pEnc->pMBs[x + y * pEnc->mbParam.mb_width];
			MVBLOCKHINT * bhint = &hint->mvhint.block[x + y * pEnc->mbParam.mb_width];
			VECTOR pred[4];
			VECTOR tmp;
			int dummy[4];
			int vec;

			pMB->mode = (hint->rawhints) ? bhint->mode : BitstreamGetBits(&bs, MODEBITS);

			if (pMB->mode == MODE_INTER || pMB->mode == MODE_INTER_Q)
			{
				tmp.x  = (hint->rawhints) ? bhint->mvs[0].x : BitstreamGetBits(&bs, length);
				tmp.y  = (hint->rawhints) ? bhint->mvs[0].y : BitstreamGetBits(&bs, length);
				tmp.x -= (tmp.x >= high) ? high*2 : 0;
				tmp.y -= (tmp.y >= high) ? high*2 : 0;

				get_pmvdata(pEnc->pMBs, x, y, pEnc->mbParam.mb_width, 0, pred, dummy);

				for (vec=0 ; vec<4 ; ++vec)
				{
					pMB->mvs[vec].x  = tmp.x;
					pMB->mvs[vec].y  = tmp.y;
					pMB->pmvs[vec].x = pMB->mvs[0].x - pred[0].x;
					pMB->pmvs[vec].y = pMB->mvs[0].y - pred[0].y;
				}
			}
			else if (pMB->mode == MODE_INTER4V)
			{
				for (vec=0 ; vec<4 ; ++vec)
				{
					tmp.x  = (hint->rawhints) ? bhint->mvs[vec].x : BitstreamGetBits(&bs, length);
					tmp.y  = (hint->rawhints) ? bhint->mvs[vec].y : BitstreamGetBits(&bs, length);
					tmp.x -= (tmp.x >= high) ? high*2 : 0;
					tmp.y -= (tmp.y >= high) ? high*2 : 0;

					get_pmvdata(pEnc->pMBs, x, y, pEnc->mbParam.mb_width, vec, pred, dummy);

					pMB->mvs[vec].x  = tmp.x;
					pMB->mvs[vec].y  = tmp.y;
					pMB->pmvs[vec].x = pMB->mvs[vec].x - pred[0].x;
					pMB->pmvs[vec].y = pMB->mvs[vec].y - pred[0].y;
				}
			}
			else	// intra / intra_q / stuffing / not_coded
			{
				for (vec=0 ; vec<4 ; ++vec)
				{
					pMB->mvs[vec].x  = pMB->mvs[vec].y  = 0;
				}
			}
		}
	}
}

static
void HintedMEGet(Encoder * pEnc, int intra)
{
	HINTINFO * hint;
	Bitstream bs;
	uint32_t x, y;
	int length, high;

	hint = pEnc->mbParam.hint;

	if (hint->rawhints)
	{
		hint->mvhint.intra = intra;
	}
	else
	{
		BitstreamInit(&bs, hint->hintstream, 0);
		BitstreamPutBit(&bs, intra);
	}

	if (intra)
	{
		if (!hint->rawhints)
		{
			BitstreamPad(&bs);
			hint->hintlength = BitstreamLength(&bs);
		}
		return;
	}

	length	= pEnc->mbParam.fixed_code + 5;
	high	= 1 << (length - 1);

	if (hint->rawhints)
	{
		hint->mvhint.fcode = pEnc->mbParam.fixed_code;
	}
	else
	{
		BitstreamPutBits(&bs, pEnc->mbParam.fixed_code, FCODEBITS);
	}

	for (y=0 ; y<pEnc->mbParam.mb_height ; ++y)
	{
		for (x=0 ; x<pEnc->mbParam.mb_width ; ++x)
		{
			MACROBLOCK * pMB = &pEnc->pMBs[x + y * pEnc->mbParam.mb_width];
			MVBLOCKHINT * bhint = &hint->mvhint.block[x + y * pEnc->mbParam.mb_width];
			VECTOR tmp;

			if (hint->rawhints)
			{
				bhint->mode = pMB->mode;
			}
			else
			{
				BitstreamPutBits(&bs, pMB->mode, MODEBITS);
			}

			if (pMB->mode == MODE_INTER || pMB->mode == MODE_INTER_Q)
			{
				tmp.x  = pMB->mvs[0].x;
				tmp.y  = pMB->mvs[0].y;
				tmp.x += (tmp.x < 0) ? high*2 : 0;
				tmp.y += (tmp.y < 0) ? high*2 : 0;

				if (hint->rawhints)
				{
					bhint->mvs[0].x = tmp.x;
					bhint->mvs[0].y = tmp.y;
				}
				else
				{
					BitstreamPutBits(&bs, tmp.x, length);
					BitstreamPutBits(&bs, tmp.y, length);
				}
			}
			else if (pMB->mode == MODE_INTER4V)
			{
				int vec;

				for (vec=0 ; vec<4 ; ++vec)
				{
					tmp.x  = pMB->mvs[vec].x;
					tmp.y  = pMB->mvs[vec].y;
					tmp.x += (tmp.x < 0) ? high*2 : 0;
					tmp.y += (tmp.y < 0) ? high*2 : 0;

					if (hint->rawhints)
					{
						bhint->mvs[vec].x = tmp.x;
						bhint->mvs[vec].y = tmp.y;
					}
					else
					{
						BitstreamPutBits(&bs, tmp.x, length);
						BitstreamPutBits(&bs, tmp.y, length);
					}
				}
			}
		}
	}

	if (!hint->rawhints)
	{
		BitstreamPad(&bs);
		hint->hintlength = BitstreamLength(&bs);
	}
}


static int FrameCodeI(Encoder * pEnc, Bitstream * bs, uint32_t *pBits
#ifdef MPEG4IP
	, bool vol_header
#endif
	)
{

	DECLARE_ALIGNED_MATRIX(dct_codes, 6, 64, int16_t, CACHE_LINE);
	DECLARE_ALIGNED_MATRIX(qcoeff,    6, 64, int16_t, CACHE_LINE);

	uint16_t x, y;

	pEnc->iFrameNum = 0;
	pEnc->mbParam.rounding_type = 1;
	pEnc->mbParam.coding_type = I_VOP;

#ifdef MPEG4IP
	if ((pEnc->mbParam.global_flags & XVID_SHORT_HEADERS) == 0) {
	  if (vol_header) {
	    BitstreamWriteVolHeader(bs, &pEnc->mbParam);
	  }
#else
	  BitstreamWriteVolHeader(bs, &pEnc->mbParam);
#endif
	  BitstreamWriteVopHeader(bs, &pEnc->mbParam);
#ifdef MPEG4IP
	} else {
	  BitstreamWriteShortVopHeader(bs, &pEnc->mbParam);
	}
#endif

	*pBits = BitstreamPos(bs);

	pEnc->sStat.iTextBits = 0;
	pEnc->sStat.kblks = pEnc->mbParam.mb_width * pEnc->mbParam.mb_height;
	pEnc->sStat.mblks = pEnc->sStat.ublks = 0;

	for (y = 0; y < pEnc->mbParam.mb_height; y++)
		for (x = 0; x < pEnc->mbParam.mb_width; x++)
		{
			MACROBLOCK *pMB = &pEnc->pMBs[x + y * pEnc->mbParam.mb_width];

			CodeIntraMB(pEnc, pMB);

			MBTransQuantIntra(&pEnc->mbParam, pMB, x, y, dct_codes, qcoeff, &pEnc->sCurrent);

			start_timer();
			MBPrediction(&pEnc->mbParam, x, y, pEnc->mbParam.mb_width, qcoeff, pEnc->pMBs);
			stop_prediction_timer();

			start_timer();
			MBCoding(&pEnc->mbParam, pMB, qcoeff, bs, &pEnc->sStat);
			stop_coding_timer();
		}

	emms();

	*pBits = BitstreamPos(bs) - *pBits;
	pEnc->sStat.fMvPrevSigma = -1;
	pEnc->sStat.iMvSum = 0;
	pEnc->sStat.iMvCount = 0;
	pEnc->mbParam.fixed_code = 2;

	if (pEnc->mbParam.global_flags & XVID_HINTEDME_GET)
	{
		HintedMEGet(pEnc, 1);
	}

	return 1;					 // intra
}


#define INTRA_THRESHOLD 0.5

static int FrameCodeP(Encoder * pEnc, Bitstream * bs, uint32_t *pBits, bool force_inter, bool vol_header)
{
	float fSigma;

	DECLARE_ALIGNED_MATRIX(dct_codes, 6, 64, int16_t, CACHE_LINE);
	DECLARE_ALIGNED_MATRIX(qcoeff,    6, 64, int16_t, CACHE_LINE);

	int iLimit;
	uint32_t x, y;
	int iSearchRange;
	bool bIntra;

	IMAGE *pCurrent = &pEnc->sCurrent;
	IMAGE *pRef = &pEnc->sReference;

	start_timer();
	image_setedges(pRef,
		       pEnc->mbParam.edged_width,
		       pEnc->mbParam.edged_height,
		       pEnc->mbParam.width,
		       pEnc->mbParam.height,
		       pEnc->mbParam.global_flags & XVID_INTERLACING);
	stop_edges_timer();

	pEnc->mbParam.rounding_type = 1 - pEnc->mbParam.rounding_type;

	if (!force_inter)
		iLimit = (int)(pEnc->mbParam.mb_width * pEnc->mbParam.mb_height * INTRA_THRESHOLD);
	else
		iLimit = pEnc->mbParam.mb_width * pEnc->mbParam.mb_height + 1;

	if ((pEnc->mbParam.global_flags & XVID_HALFPEL) > 0) {
		start_timer();
		image_interpolate(pRef, &pEnc->vInterH, &pEnc->vInterV, &pEnc->vInterHV,
				  pEnc->mbParam.edged_width, pEnc->mbParam.edged_height,
				  pEnc->mbParam.rounding_type);
		stop_inter_timer();
	}

	start_timer();
	if (pEnc->mbParam.global_flags & XVID_HINTEDME_SET)
	{
		HintedMESet(pEnc, &bIntra);
	}
	else
	{
		bIntra = MotionEstimation(pEnc->pMBs, &pEnc->mbParam, &pEnc->sReference,
					  &pEnc->vInterH, &pEnc->vInterV,
					  &pEnc->vInterHV, &pEnc->sCurrent, iLimit);
	}
	stop_motion_timer();

	if (bIntra == 1)
	{
#ifdef MPEG4IP
		return FrameCodeI(pEnc, bs, pBits, vol_header);
#else
		return FrameCodeI(pEnc, bs, pBits);
#endif
	}

	pEnc->mbParam.coding_type = P_VOP;

#ifdef MPEG4IP
	if ((pEnc->mbParam.global_flags & XVID_SHORT_HEADERS) == 0) {
#endif
	  if(vol_header)
	    BitstreamWriteVolHeader(bs, &pEnc->mbParam);

	  BitstreamWriteVopHeader(bs, &pEnc->mbParam);
#ifdef MPEG4IP
	} else {
	  BitstreamWriteShortVopHeader(bs, &pEnc->mbParam);
	}
#endif

	*pBits = BitstreamPos(bs);

	pEnc->sStat.iTextBits = 0;
	pEnc->sStat.iMvSum = 0;
	pEnc->sStat.iMvCount = 0;
	pEnc->sStat.kblks = pEnc->sStat.mblks = pEnc->sStat.ublks = 0;

	for(y = 0; y < pEnc->mbParam.mb_height; y++)
	{
		for(x = 0; x < pEnc->mbParam.mb_width; x++)
		{
			MACROBLOCK * pMB = &pEnc->pMBs[x + y * pEnc->mbParam.mb_width];

			bIntra = (pMB->mode == MODE_INTRA) || (pMB->mode == MODE_INTRA_Q);

			if (!bIntra)
			{
				start_timer();
				MBMotionCompensation(pMB,
						     x, y,
						     &pEnc->sReference,
						     &pEnc->vInterH,
						     &pEnc->vInterV,
						     &pEnc->vInterHV,
						     &pEnc->sCurrent,
						     dct_codes,
						     pEnc->mbParam.width,
						     pEnc->mbParam.height,
						     pEnc->mbParam.edged_width,
						     pEnc->mbParam.rounding_type);
				stop_comp_timer();

				if ((pEnc->mbParam.global_flags & XVID_LUMIMASKING) > 0) {
					if(pMB->dquant != NO_CHANGE) {
						pMB->mode = MODE_INTER_Q;
						pEnc->mbParam.quant += DQtab[pMB->dquant];
						if (pEnc->mbParam.quant > 31) pEnc->mbParam.quant = 31;
						else if(pEnc->mbParam.quant < 1) pEnc->mbParam.quant = 1;
					}
				}
				pMB->quant = pEnc->mbParam.quant;

				pMB->field_pred = 0;

				pMB->cbp = MBTransQuantInter(&pEnc->mbParam, pMB, x, y, dct_codes, qcoeff, pCurrent);
			}
			else 
			{
				CodeIntraMB(pEnc, pMB);
				MBTransQuantIntra(&pEnc->mbParam, pMB, x, y, dct_codes, qcoeff, pCurrent);
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

	if (pEnc->mbParam.global_flags & XVID_HINTEDME_GET)
	{
		HintedMEGet(pEnc, 0);
	}

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
    
	*pBits = BitstreamPos(bs) - *pBits;

	return 0;					 // inter
}


/**************************************************************************
 *                                                                        *
 * This code is developed by Adam Li.  This software is an                *
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
 *  vop_code.c
 *
 *  Copyright (C) 2001  Project Mayo
 *
 *  Adam Li
 *
 *  DivX Advance Research Center <darc@projectmayo.com>
 *
 **************************************************************************/

/* This file contains some functions to do coding of one VOP.            */

#include "vop_code.h"
#include "mot_est_comp.h"
//#include "rc_q2.h"
#include "bitstream.h"

#include "rate_ctl.h"

#define SCENE_CHANGE_THREADHOLD 50
#define MB_RATIO_THREADHOLD 0.40

extern FILE *ftrace;

UInt  	BitstreamPutVopHeader (	Vop *vop,
			Float time,
			VolConfig *vol_config
	);
Void ImageRepetitivePadding(Image *image, Int edge);
Double compute_MAD(Vop *vop);


/***********************************************************CommentBegin******
 *
 * -- VopCode -- Shape, texture and motion coding of the vop
 *
 * Purpose :
 * This function performs shape, texture and motion coding of the
 * vop passed to it. The input vop is assumed to be BOUNDED.
 *
 * Arguments in :
 * Vop *curr - pointer to vop to be coded
 * Vop *prev - pointer the last occurence of this vop
 * Vop *rec_prev - pointer to last coded occurence of this vop
 * Int enable_8x8_mv - 8x8 motion vectors flag (SpSc)
 * Int intra_dcpred_disable - disable intradc prediction
 * Float time -
 * VolConfig *vol_config -
 *
 * Arguments in/out :
 * Vop *rec_curr - coded vop
 * Bitcount num_bits - BitCount structures 
 *
 ***********************************************************CommentEnd********/

Void VopCode(Vop *curr,
Vop *reference,
Vop *reconstruct,
Vop *error,
Int enable_8x8_mv,
Float time,
VolConfig *vol_config)
{
	ImageF    *mot_x=NULL, *mot_y=NULL;
	Image     *MB_decisions=NULL;
	Int       edge,f_code_for=1;
	Vop       *error_vop=NULL;
	Int       vop_quantizer;

	Float	  mad_P = 0., mad_I = 0.;
	Float	  IntraMBRatio = 1.;
	Int		  numberMB, i, IntraMB;

	edge = 0;
	f_code_for = curr->fcode_for;

	if (curr->prediction_type == P_VOP) 
	{
		/* Carry out motion estimation and compensation*/
		MotionEstimationCompensation(curr, reference,
			enable_8x8_mv, edge ,f_code_for,
			reconstruct, &mad_P, &mot_x,&mot_y,&MB_decisions);

		/* Calculate the percentage of the MBs that are Intra */
		IntraMB = 0;
		numberMB = MB_decisions->x * MB_decisions->y;
		for (i = 0; i < numberMB; i ++)
			if (MB_decisions->f[i] == MBM_INTRA) IntraMB ++;
		IntraMBRatio = (float)IntraMB / (float)numberMB;

		#ifdef _RC_
		fprintf(ftrace, "ME with MAD : %f\n", mad_P);
		fprintf(ftrace, "%4.2f of the MBs are I-MBs.\n", IntraMBRatio);
		#endif
	}
	else
		mad_P = SCENE_CHANGE_THREADHOLD * 2;

	if ((mad_P < SCENE_CHANGE_THREADHOLD / 3) || 
		((mad_P < SCENE_CHANGE_THREADHOLD) && (IntraMBRatio < MB_RATIO_THREADHOLD)))
	{
		// mad is fine. continue to code as P_VOP
		curr->prediction_type = P_VOP;
		error->prediction_type = P_VOP;

		#ifdef _RC_
		fprintf(ftrace, "Coding mode : INTER\n");
		#endif

		vop_quantizer = RateCtlGetQ(mad_P);

		curr->quantizer = vop_quantizer;
		error->quantizer = vop_quantizer;

		#ifdef _RC_DEBUG_
		fprintf(stdout, "RC: >>>>> New quantizer= %d\n", vop_quantizer);
		#endif

		SubImage(curr->y_chan, reconstruct->y_chan, error->y_chan);
		SubImage(curr->u_chan, reconstruct->u_chan, error->u_chan);
		SubImage(curr->v_chan, reconstruct->v_chan, error->v_chan); 

		BitstreamPutVopHeader(curr,time,vol_config);

		VopShapeMotText(error, reconstruct, MB_decisions,
			mot_x, mot_y, f_code_for, 
			GetVopIntraACDCPredDisable(curr), reference,
			NULL/*mottext_bitstream*/);
	} else {
		// mad is too large. 
		// the coding mode should be I_VOP
		curr->prediction_type = I_VOP;
		curr->rounding_type = 1;

		#ifdef _RC_
		fprintf(ftrace, "Coding mode : INTRA\n");
		#endif

		// We need to recalculate MAD here, since the last MAD was calculated by assuming
		// INTER coding, though the actual difference might not be significant to coding.
		if (mad_I == 0.) mad_I = (Float) compute_MAD(curr);

		vop_quantizer = RateCtlGetQ(mad_I);

		curr->intra_quantizer = vop_quantizer;
		curr->rounding_type = 1;

		BitstreamPutVopHeader(curr,time,vol_config);

		/* Code Texture in Intra mode */
		VopCodeShapeTextIntraCom(curr,
			reference,
			NULL/*texture_bitstream*/
		);
	} 


	if (MB_decisions) FreeImage(MB_decisions);
	if (mot_x) FreeImage(mot_x);
	if (mot_y) FreeImage(mot_y);

	ImageRepetitivePadding(reference->y_chan, 16);
	ImageRepetitivePadding(reference->u_chan, 8);
	ImageRepetitivePadding(reference->v_chan, 8);

	Bitstream_NextStartCode();	  

	return;
}												  /* CodeVop() */


/***********************************************************CommentBegin******
 *
 * -- BitstreamPutVopHeader -- Writes all fields of the vop header syntax
 *
 * Purpose :
 *	This function writes all fields of the vop header syntax to the
 * bitstream disk file.
 *
 * Arguments in :
 *	Vop *vop - pointer to vop containing header information
 *
 * Return values :
 *	UInt num_bits - number of bits written
 *
 * Description :
 *	The vop header (start_code, vop_ID, etc.) is first
 *	written to an intermediate integer level bitstream data structure.
 *	This intermediate bitstream data structure is then written to disk.
 *
 ***********************************************************CommentEnd********/

UInt
BitstreamPutVopHeader(Vop *vop,
Float time, 
VolConfig *vol_config)
{
	Image *buffer = NULL;
	Int bits;
	Int   time_modulo;
	Float time_inc;

	Int   index;

	UInt  num_bits_header=0;

	/* 
	 *
	 * Write all syntax fields in vop header to data structure
	 *
	 */

	BitstreamPutBits(buffer,VOP_START_CODE,VOP_START_CODE_LENGTH);

	BitstreamPutBits(buffer,GetVopPredictionType(vop),2);

	index = GetVolConfigModTimeBase(vol_config, 1);

	time_modulo = (int)time - index*1000;
	while(time_modulo >= 1000)
	{
		BitstreamPutBits(buffer,1,1);
		time_modulo = time_modulo - 1000;
		index++;
		printf("time modulo : 1\n");
	}
	BitstreamPutBits(buffer,0,1);

	/* Store this modulo time base */
	PutVolConfigModTimeBase(index,vol_config);

	time_inc = (time - index*1000);

	bits = (int)ceil(log((double)GetVopTimeIncrementResolution(vop))/log(2.0));
	if (bits<1) bits=1;
	time_inc=time_inc*GetVopTimeIncrementResolution(vop)/1000.0f;

	/* marker bit */
	BitstreamPutBits(buffer,1,1);

	BitstreamPutBits(buffer,(Int)(time_inc+0.001),bits);

	/* marker bit */
	BitstreamPutBits(buffer,1,1);

	if (GetVopWidth(vop)==0)
	{
		printf("Empty VOP at %.2f\n",time);		  /* MW 30-NOV-1998 */
		BitstreamPutBits(buffer,0L,1L);
		num_bits_header += Bitstream_NextStartCode();
		return(num_bits_header);
	}
	else
		BitstreamPutBits(buffer,1L,1L);

	if( GetVopPredictionType(vop) == P_VOP )
		BitstreamPutBits(buffer,GetVopRoundingType(vop),1);

	BitstreamPutBits(buffer,GetVopIntraDCVlcThr(vop),3);

	if (GetVopPredictionType(vop) == I_VOP)	  /* I_VOP */
		BitstreamPutBits(buffer,GetVopIntraQuantizer(vop),GetVopQuantPrecision(vop));
	else   /* P_VOP */
		BitstreamPutBits(buffer,GetVopQuantizer(vop),GetVopQuantPrecision(vop));

	if (GetVopPredictionType(vop)!=I_VOP)
	{
		BitstreamPutBits(buffer,GetVopFCodeFor(vop),3);
	}

	return(num_bits_header);
}

// do repetitive padding for image
// make sure set edge = 16 for Y and 8 for UV
Void ImageRepetitivePadding(Image *image, Int edge)
{
	SInt *p, left, right;
	Int width, height, x, y;

	p = image->f;
	width = image->x;
	height = image->y;

	/* Horizontal Padding */
	for( y=edge; y<height-edge; y++)
    {
		left = p[y*width+edge];
		right = p[y*width+width-edge-1];
		for(x=0; x<edge; x++)
		{
			p[y*width+x] = left;
			p[y*width+width-edge+x] = right;
		}
    }

	/* Vertical Padding */
    for(y=0; y<edge; y++)
        for(x=0; x<width; x++)
            p[y*width+x] = p[edge*width+x];
		
	for(y=height-edge; y<height; y++)
		for(x=0; x<width; x++)
			p[y*width+x] = p[(height-1-edge)*width+x];
	
	return;
}


/***********************************************************CommentBegin******
 *
 * -- compute_MAD --
 *
 * Purpose :
 *      Calculate the MAD of a VOP
 *
 * Arguments in :
 *      Vop    *error_vop  - Vop with error residue
 *
 ***********************************************************CommentEnd********/

// This function is now only called when the coding mode is I_VOP.
Double compute_MAD(
Vop  *error_vop
)
{
	SInt  *curr_in,
		*curr_end;
	Float *curr_fin,
		*curr_fend;
	UInt   sxy_in;
	Double mad=0.0, dc = 0.0;
	Int    cnt=0;

	/* Calculate the MAD */
	switch (GetImageType(error_vop->y_chan))
	{
		case SHORT_TYPE:
			/* change to AC MAD */
			/* calculate average first */
			curr_in = (SInt*)GetImageData(error_vop->y_chan);
			sxy_in = GetImageSize(error_vop->y_chan);
			curr_end = curr_in + sxy_in;
			cnt = 0;
			while (curr_in != curr_end)
			{
				dc += *curr_in; //abs(*curr_in);
				cnt++;
				curr_in++;
			}
			dc /= cnt;

			curr_in = (SInt*)GetImageData(error_vop->y_chan);
			sxy_in = GetImageSize(error_vop->y_chan);
			curr_end = curr_in + sxy_in;
			cnt = 0;
			while (curr_in != curr_end)
			{
				mad += fabs(*curr_in - dc);
				cnt++;
				curr_in++;
			}
			mad /= cnt;
			break;
		case FLOAT_TYPE:
			curr_fin = (Float*)GetImageData(error_vop->y_chan);
			sxy_in = GetImageSize(error_vop->y_chan);
			curr_fend = curr_fin + sxy_in;
			cnt = 0;
			while (curr_fin != curr_fend)
			{
				mad += fabs(*curr_fin);
				cnt++;
				curr_fin++;
			}
			mad /= cnt;
			break;
		default: break;
	}

#ifdef _RC_
	fprintf(ftrace, "The MAD of the VOP to be coded is %f.\n", mad);
#endif

	return mad;
}



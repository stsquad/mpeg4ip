
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
 *  text_code_mb.c
 *
 *  Copyright (C) 2001  Project Mayo
 *
 *  Adam Li
 *  Juice
 *
 *  DivX Advance Research Center <darc@projectmayo.com>
 *
 **************************************************************************/

/* This file contains some functions for text coding of MacroBlocks.      */
/* Some codes of this project come from MoMuSys MPEG-4 implementation.    */
/* Please see seperate acknowledgement file for a list of contributors.   */

#include "text_code.h"
#include "text_dct.h"

#define BLOCK_SIZE 8

Void  	BlockPredict (SInt *curr, /*SInt *rec_curr,*/
	Int x_pos, Int y_pos, UInt width, Int fblock[][8]);
Void  	BlockRebuild (SInt *rec_curr, SInt *comp, Int pred_type, Int max,
	Int x_pos, Int y_pos, UInt width, UInt edge, Int fblock[][8]);
Void  	BlockQuantH263 (Int *coeff, Int QP, Int mode, Int type,
	Int *qcoeff, Int maxDC, Int image_type);
Void  	BlockDequantH263 (Int *qcoeff, Int QP, Int mode, Int type,
	Int *rcoeff, Int image_type, Int short_video_header, Int bits_per_pixel);


/***********************************************************CommentBegin******
 *
 * -- CodeMB -- Code, decode and reconstruct Macroblock 
 *              combined with substraction and addition operation
 *
 * Arguments in :
 *	Int x_pos	x_position of Macroblock
 *	Int y_pos	y_position of Macroblock
 *	UInt width	width of Vop bounding box (unpadded size)
 *	Int QP		Quantization parameter
 *	Int Mode	Macroblock coding mode
 *
 * Arguments in/out :
 *	Vop *curr	current Vop, uncoded
 *	Vop *rec_curr	current Vop, decoded and reconstructed
 *  Vop *comp   current Vop, motion compensated
 *  Int *qcoeff coefficient block (384 * sizeof(Int))
 *
 ***********************************************************CommentEnd********/

Void CodeMB(Vop *curr, Vop *rec_curr, Vop *comp, Int x_pos, Int y_pos, UInt width,
Int QP, Int Mode, Int *qcoeff)
{
	Int k;
	Int fblock[6][8][8];
	Int coeff[384];
	Int *coeff_ind;
	Int *qcoeff_ind;
	Int* rcoeff_ind;
	Int x, y;
	SInt *current, *recon, *compensated = NULL;
	UInt xwidth;
	Int iblock[6][8][8];
	Int rcoeff[6*64];
	Int i, j;
	Int type;									  /* luma = 1, chroma = 2 */
//	Int *qmat;
	SInt tmp[64];
	Int s;

	int operation = curr->prediction_type;
	/* This variable is for combined operation.
	If it is an I_VOP, then MB in curr is reconstruct into rec_curr,
	and comp is not used at all (i.e., it can be set to NULL).
	If it is a P_VOP, then MB in curr is reconstructed, and the result
	added with MB_comp is written into rec_curr. 
	- adamli 11/19/2000 */

	Int max = GetVopBrightWhite(curr);
	/* This variable is the max value for the clipping of the reconstructed image. */

	coeff_ind = coeff;
	qcoeff_ind = qcoeff;
	rcoeff_ind = rcoeff;

	for (k = 0; k < 6; k++)
	{
		switch (k)
		{
			case 0:
				x = x_pos;
				y = y_pos;
				xwidth = width;
				current = (SInt *) GetImageData (GetVopY (curr));
				break;
			case 1:
				x = x_pos + 8;
				y = y_pos;
				xwidth = width;
				current = (SInt *) GetImageData (GetVopY (curr));
				break;
			case 2:
				x = x_pos;
				y = y_pos + 8;
				xwidth = width;
				current = (SInt *) GetImageData (GetVopY (curr));
				break;
			case 3:
				x = x_pos + 8;
				y = y_pos + 8;
				xwidth = width;
				current = (SInt *) GetImageData (GetVopY (curr));
				break;
			case 4:
				x = x_pos / 2;
				y = y_pos / 2;
				xwidth = width / 2;
				current = (SInt *) GetImageData (GetVopU (curr));
				break;
			case 5:
				x = x_pos / 2;
				y = y_pos / 2;
				xwidth = width / 2;
				current = (SInt *) GetImageData (GetVopV (curr));
				break;
			default:
				break;
		}
		BlockPredict (current, x, y, xwidth, fblock[k]);
	}

	for (k = 0; k < 6; k++)
	{
		s = 0;
		for (i = 0; i < 8; i++)
			for (j = 0; j < 8; j++)
				tmp[s++] = (SInt) fblock[k][i][j];
#ifndef USE_MMX
		fdct_enc(tmp);
#else
		fdct_mmx(tmp);
#endif
		for (s = 0; s < 64; s++)
			coeff_ind[s] = (Int) tmp[s];

		if (k < 4) type = 1;
		else type = 2;

		/* For this release, only H263 quantization is supported. - adamli */
		BlockQuantH263(coeff_ind,QP,Mode,type,qcoeff_ind,
			GetVopBrightWhite(curr),1);
		BlockDequantH263(qcoeff_ind,QP,Mode,type,rcoeff_ind,1, 0, GetVopBitsPerPixel(curr));

		for (s = 0; s < 64; s++)
			tmp[s] = (SInt) rcoeff_ind[s];
#ifndef USE_MMX
		idct_enc(tmp);
#else
		idct_mmx(tmp);
#endif
		s = 0;
		for (i = 0; i < 8; i++)
			for (j = 0; j < 8; j++)
				iblock[k][i][j] = (Int)tmp[s++];

		coeff_ind += 64;
		qcoeff_ind += 64;
		rcoeff_ind += 64;

		if (Mode == MODE_INTRA||Mode==MODE_INTRA_Q)
			for (i = 0; i < 8; i++)
				for (j = 0; j < 8; j ++)
					iblock[k][i][j] = MIN (GetVopBrightWhite(curr), MAX (0, iblock[k][i][j]));

		switch (k)
		{
			case 0:
			case 1:
			case 2:
				continue;

			case 3:
				recon = (SInt *) GetImageData (GetVopY (rec_curr));
				if (operation == P_VOP) compensated = (SInt *) GetImageData (GetVopY (comp));
				BlockRebuild (recon, compensated, operation, max, x_pos,     y_pos,     width, 16, iblock[0]);
				BlockRebuild (recon, compensated, operation, max, x_pos + 8, y_pos,     width, 16, iblock[1]);
				BlockRebuild (recon, compensated, operation, max, x_pos,     y_pos + 8, width, 16, iblock[2]);
				BlockRebuild (recon, compensated, operation, max, x_pos + 8, y_pos + 8, width, 16, iblock[3]);
				continue;

			case 4:
				recon = (SInt *) GetImageData (GetVopU (rec_curr));
				if (operation == P_VOP) compensated = (SInt *) GetImageData (GetVopU (comp));
				BlockRebuild (recon, compensated, operation, max,
					x_pos/2, y_pos/2, width/2, 8, iblock[4]);
				continue;

			case 5:
				recon = (SInt *) GetImageData (GetVopV (rec_curr));
				if (operation == P_VOP) compensated = (SInt *) GetImageData (GetVopV (comp));
				BlockRebuild (recon, compensated, operation, max,
					x_pos/2, y_pos/2, width/2, 8, iblock[5]);
				continue;
		}
	}

	return;
}


/***********************************************************CommentBegin******
 *
 * -- BlockPredict -- Get prediction for an Intra block
 *
 * Purpose :
 *	Get prediction for an Intra block
 *
 * Arguments in :
 *	Int x_pos	x_position of Macroblock
 *	Int y_pos	y_position of Macroblock
 *	UInt width	width of Vop bounding box
 *
 * Arguments in/out :
 *	SInt *curr	current uncoded Vop data
 *	SInt *rec_curr	reconstructed Vop data area
 *
 * Arguments out :
 *	Int fblock[][8]	the prediction block to be coded for bitstream
 *
 ***********************************************************CommentEnd********/
Void
BlockPredict (SInt *curr, /*SInt *rec_curr,*/ Int x_pos, Int y_pos,
UInt width, Int fblock[][8])
{
	Int i, j;

	for (i = 0; i < 8; i++)
	{
		for (j = 0; j < 8; j++)
		{
			fblock[i][j] = curr[(y_pos+i)*width + x_pos+j];
		}
	}
}


/***********************************************************CommentBegin******
 *
 * -- BlockRebuild -- Reconstructs a block into data area of Vop
 *
 * Purpose :
 *	Reconstructs a block into data area of Vop
 *
 * Arguments in :
 *	Int x_pos	x_position of Macroblock
 *	Int y_pos	y_position of Macroblock
 *	UInt width	width of Vop bounding box
 *
 * Arguments in/out :
 *	SInt *rec_curr	current Vop data area to be reconstructed
 *
 * Description :
 *	Does reconstruction for Intra predicted blocks also
 *
 ***********************************************************CommentEnd********/

Void
BlockRebuild (SInt *rec_curr, SInt *comp, Int pred_type, Int max,
Int x_pos, Int y_pos, UInt width, UInt edge, Int fblock[][8])
{
	/* this function now does rebuild and generating error at the same time */
	Int i, j;
	SInt *rec;
	Int padded_width;
	
	padded_width = width + 2 * edge;
	rec = rec_curr + edge * padded_width + edge;

	if (pred_type == I_VOP)
	{
		SInt *p;
		p  = rec + y_pos * padded_width + x_pos;

		for (i = 0; i < 8; i++) 
		{
			for (j = 0; j < 8; j++)
			{
				SInt temp = fblock[i][j];
				*(p++) = CLIP(temp, 0, max);
			}

			p += padded_width - 8;
		}
	}
	else if (pred_type == P_VOP)
	{
		SInt *p, *pc; 
		p  = rec + y_pos * padded_width + x_pos;
		pc = comp     + y_pos * width + x_pos;

		for (i = 0; i < 8; i++)
		{
			for (j = 0; j < 8; j++)
			{
				SInt temp = *(pc++) + fblock[i][j];
				*(p++) = CLIP(temp, 0, max);
			}

			p += padded_width - 8;
			pc += width - 8;
		}
	}
}



/***********************************************************CommentBegin******
 *
 * -- BlockQuantH263 -- 8x8 block level quantization
 *
 * Purpose :
 *	8x8 block level quantization
 *
 * Arguments in :
 *	Int *coeff	non-quantized coefficients
 *	Int QP		quantization parameter
 *	Int Mode	Macroblock coding mode
 *
 * Arguments out :
 *	Int *qcoeff	quantized coefficients
 *
 ***********************************************************CommentEnd********/
Void
BlockQuantH263 (Int *coeff, Int QP, Int mode, Int type, Int *qcoeff, Int maxDC, Int image_type)
{
	Int i;
	Int level, result;
	Int step, offset;
	Int dc_scaler;

	if (!(QP > 0 && (QP < 32*image_type))) return;
	if (!(type == 1 || type == 2)) return;

	if (mode == MODE_INTRA || mode == MODE_INTRA_Q)
	{										  /* Intra */
		dc_scaler = cal_dc_scaler(QP,type);
		qcoeff[0] = MAX(1,MIN(maxDC-1, (coeff[0] + dc_scaler/2)/dc_scaler));

		step = 2 * QP;
		for (i = 1; i < 64; i++)
		{
			level = (abs(coeff[i]))  / step;
			result = (coeff[i] >= 0) ? level : -level;
			qcoeff[i] =  MIN(2047, MAX(-2048, result));
		}
	}
	else
	{										  /* non Intra */
		step = 2 * QP;
		offset = QP / 2;
		for (i = 0; i < 64; i++)
		{
			level = (abs(coeff[i]) - offset)  / step;
			result = (coeff[i] >= 0) ? level : -level;
			qcoeff[i] =  MIN(2047, MAX(-2048, result));
		}
	}

	return;
}


/***********************************************************CommentBegin******
 *
 * -- BlockDequantH263 -- 8x8 block dequantization
 *
 * Purpose :
 *	8x8 block dequantization
 *
 * Arguments in :
 *	Int *qcoeff	        quantized coefficients
 *	Int QP		        quantization parameter
 *	Int mode	        Macroblock coding mode
 *      Int short_video_header  Flag to signal short video header bitstreams (H.263)
 *
 * Arguments out :
 *	Int *rcoeff	reconstructed (dequantized) coefficients
 *
 ***********************************************************CommentEnd********/
Void
BlockDequantH263 (Int *qcoeff, Int QP, Int mode, Int type, Int *rcoeff,
Int image_type, Int short_video_header, Int bits_per_pixel)
{
	Int i;
	Int dc_scaler;
	Int lim;

	lim = (1 << (bits_per_pixel + 3));

	if (QP)
	{
		for (i = 0; i < 64; i++)
		{
			if (qcoeff[i])
			{
				/* 16.11.98 Sven Brandau: "correct saturation" due to N2470, Clause 2.1.6 */
				qcoeff[i] =  MIN(2047, MAX(-2048, qcoeff[i] ));
				if ((QP % 2) == 1)
					rcoeff[i] = QP * (2*ABS(qcoeff[i]) + 1);
				else
					rcoeff[i] = QP * (2*ABS(qcoeff[i]) + 1) - 1;
				rcoeff[i] = SIGN(qcoeff[i]) * rcoeff[i];
			}
			else
				rcoeff[i] = 0;
		}
		if (mode == MODE_INTRA || mode == MODE_INTRA_Q)
		{										  /* Intra */

			MOMCHECK(QP > 0 && (QP < 32*image_type));
			MOMCHECK(type == 1 || type == 2);

			if (short_video_header)
				dc_scaler = 8;
			else
				dc_scaler = cal_dc_scaler(QP,type);

			rcoeff[0] = qcoeff[0] * dc_scaler;
		}
	}
	else
	{
		/* No quantizing at all */
		for (i = 0; i < 64; i++)
		{
			rcoeff[i] = qcoeff[i];
		}

		if (mode == MODE_INTRA || mode == MODE_INTRA_Q)
		{										  /* Intra */
			rcoeff[0] = qcoeff[0]*8;
		}
	}
	for (i=0;i<64;i++)
		if (rcoeff[i]>(lim-1)) rcoeff[i]=(lim-1);
		else if (rcoeff[i]<(-lim)) rcoeff[i]=(-lim);

	return;
}


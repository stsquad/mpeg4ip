
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

Void CodeMB(Vop *curr, Vop *rec_curr, Vop *comp, 
	Int x_pos, Int y_pos, UInt width,
	Int QP, Int Mode, Int *qcoeff)
{
	Int k;
	Int coeff[384];
	Int* coeff_ind;
	Int* qcoeff_ind;
	Int* rcoeff_ind;
	Int x, y;
	SInt *current, *recon, *compensated = NULL;
	UInt xwidth;
	UInt edgeWidth;
	UInt padded_width;
	Int rcoeff[6*64];
	Int i, j;
	Int type;									  /* luma = 1, chroma = 2 */
	SInt tmp[64];
	Int s;

	/* This variable is for combined operation.
	 * If it is an I_VOP, then MB in curr is reconstruct into rec_curr,
	 * and comp is not used at all (i.e., it can be set to NULL).
	 * If it is a P_VOP, then MB in curr is reconstructed, and the result
	 * added with MB_comp is written into rec_curr. 
	 * - adamli 11/19/2000 
	 */
	int operation = curr->prediction_type;

	/* 'white' is the max value for the clipping of the reconstructed image */
	Int white = GetVopBrightWhite(curr);
	Int maxBits = GetVopBitsPerPixel(curr);
	Int dc_scaler;
	Int type1_dc_scaler;
	Int type2_dc_scaler;

	coeff_ind = coeff;
	qcoeff_ind = qcoeff;
	rcoeff_ind = rcoeff;

	if (Mode == MODE_INTRA /* || Mode == MODE_INTRA_Q */) {
		type1_dc_scaler = cal_dc_scaler(QP, 1);
		type2_dc_scaler = cal_dc_scaler(QP, 2);
	}

	for (k = 0; k < 6; k++) {
		/* Setup where the data is coming from */
		switch (k) {
			case 0:
				x = x_pos;
				y = y_pos;
				xwidth = width;
				edgeWidth = 16;
				current = (SInt *)GetImageData(GetVopY(curr));
				type = 1;
				dc_scaler = type1_dc_scaler;	/* undefined if non-Intra */
				break;
			case 1:
				x = x_pos + 8;
				y = y_pos;
				xwidth = width;
				edgeWidth = 16;
				current = (SInt *)GetImageData(GetVopY(curr));
				type = 1;
				dc_scaler = type1_dc_scaler;	/* undefined if non-Intra */
				break;
			case 2:
				x = x_pos;
				y = y_pos + 8;
				xwidth = width;
				edgeWidth = 16;
				current = (SInt *)GetImageData(GetVopY(curr));
				type = 1;
				dc_scaler = type1_dc_scaler;	/* undefined if non-Intra */
				break;
			case 3:
				x = x_pos + 8;
				y = y_pos + 8;
				xwidth = width;
				edgeWidth = 16;
				current = (SInt *)GetImageData(GetVopY(curr));
				type = 1;
				dc_scaler = type1_dc_scaler;	/* undefined if non-Intra */
				break;
			case 4:
				x = x_pos / 2;
				y = y_pos / 2;
				xwidth = width / 2;
				edgeWidth = 8;
				current = (SInt *)GetImageData(GetVopU(curr));
				type = 2;
				dc_scaler = type2_dc_scaler;	/* undefined if non-Intra */
				break;
			case 5:
				x = x_pos / 2;
				y = y_pos / 2;
				xwidth = width / 2;
				edgeWidth = 8;
				current = (SInt *)GetImageData(GetVopV(curr));
				type = 2;
				dc_scaler = type2_dc_scaler;	/* undefined if non-Intra */
				break;
			default:
				break;
		}

		/* Copy the data to the DCT buffer */
		s = 0;
		for (i = 0; i < 8; i++) {
			for (j = 0; j < 8; j++) {
				tmp[s++] = (SInt)current[((y + i) * xwidth) + x + j];
			}
		}

		/* Compute the DCT */
#ifndef USE_MMX
		fdct_enc(tmp);
#else
		fdct_mmx(tmp);
#endif
		for (s = 0; s < 64; s++)
			coeff_ind[s] = (Int)tmp[s];

		/* H.263 quantization */
		if (Mode == MODE_INTRA /* || Mode == MODE_INTRA_Q */) {
			Int step = QP << 1;
			Int i;

			qcoeff_ind[0] = CLIP(
				(coeff_ind[0] + (dc_scaler >> 1)) / dc_scaler,
				1, white - 1);

			for (i = 1; i < 64; i++) {
#ifdef USE_MMX
				// MMX DCT has already clamped coeff to <-2048, 2047>
				// since integer division can't increase the value,
				// don't bother clipping values again 
				if (coeff_ind[i] == 0) {
					qcoeff_ind[i] = 0;
				} else if (coeff_ind[i] > 0) {
					qcoeff_ind[i] = coeff_ind[i] / step;
				} else {
					qcoeff_ind[i] = -((-coeff_ind[i]) / step);
				}
#else /* C DCT used */
				qcoeff_ind[i] = CLIP(
					SIGN(coeff_ind[i]) * (ABS(coeff_ind[i]) / step),
					-2048, 2047);
#endif
			}
		} else { /* non Intra */
			Int step = QP << 1;
			Int offset = QP >> 1;
			Int i;

			for (i = 0; i < 64; i++) {
#ifdef USE_MMX
				// MMX DCT has already clamped coeff to <-2048, 2047>
				// since integer division can't increase the value,
				// don't bother clipping values again 
				if (coeff_ind[i] >= 0) {
					qcoeff_ind[i] = (coeff_ind[i] - offset) / step;
				} else {
					qcoeff_ind[i] = -(((-coeff_ind[i]) - offset) / step);
				}
#else /* C DCT used */
				qcoeff_ind[i] = CLIP(
					SIGN(coeff_ind[i]) * ((ABS(coeff_ind[i]) - offset) / step),
					-2048, 2047);
#endif
			}
		}

		/* H.263 dequantize */
		if (QP) {
			Int i;

			for (i = 0; i < 64; i++) {
				if (qcoeff_ind[i] == 0) {
					rcoeff_ind[i] = 0;
				} else if (qcoeff_ind[i] > 0) {
					rcoeff_ind[i] = 
						QP * ((qcoeff_ind[i] << 1) | 1) - (QP & 1);
				} else {
					rcoeff_ind[i] = 
						-(QP * (((-qcoeff_ind[i]) << 1) | 1) - (QP & 1));
				}
			}
			if (Mode == MODE_INTRA /* || Mode == MODE_INTRA_Q */) {
				rcoeff_ind[0] = qcoeff_ind[0] * dc_scaler;
			}
		} else { /* No quantizing at all */
			for (i = 0; i < 64; i++) {
				rcoeff_ind[i] = qcoeff_ind[i];
			}

			if (Mode == MODE_INTRA /* || Mode == MODE_INTRA_Q */) {
				rcoeff_ind[0] = qcoeff_ind[0] << 3;
			}
		}

		/* Copy the reconstructed coefficients into the IDCT buffer */ 
		for (s = 0; s < 64; s++)
			tmp[s] = (SInt)rcoeff_ind[s];

		/* Compute the IDCT */
		idct_enc(tmp);

		/* Setup to store the results */
		if (k < 4) {
			recon = (SInt*)GetImageData(GetVopY(rec_curr));
			if (operation == P_VOP) {
				compensated = (SInt*)GetImageData(GetVopY(comp));
			}
		} else if (k == 4) {
			recon = (SInt*)GetImageData(GetVopU(rec_curr));
			if (operation == P_VOP) {
				compensated = (SInt*)GetImageData(GetVopU(comp));
			}
		} else /* k == 5 */ {
			recon = (SInt*)GetImageData(GetVopV(rec_curr));
			if (operation == P_VOP) {
				compensated = (SInt*)GetImageData(GetVopV(comp));
			}
		}

		/* Store the results */
		s = 0;
		padded_width = xwidth + 2 * edgeWidth;

		if (operation == I_VOP) {
			SInt* p = recon + edgeWidth * padded_width + edgeWidth
				+ y * padded_width + x;

			for (i = 0; i < 8; i++) {
				for (j = 0; j < 8; j++) {
					*(p++) = CLIP((Int)tmp[s], 0, white);
					s++;
				}
				p += padded_width - 8;
			}
		} else {
			SInt* p = recon + edgeWidth * padded_width + edgeWidth
				+ y * padded_width + x;
			SInt* pc = compensated + y * xwidth + x;

			for (i = 0; i < 8; i++) {
				for (j = 0; j < 8; j++) {
					*(p++) = CLIP(*pc + tmp[s], 0, white);
					s++;
					pc++;
				}
				p += padded_width - 8;
				pc += xwidth - 8;
			}
		}

		coeff_ind += 64;
		qcoeff_ind += 64;
		rcoeff_ind += 64;
	}

	return;
}


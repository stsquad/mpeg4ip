/**************************************************************************
 *                                                                        *
 * This code has been developed by Andrea Graziani. This software is an   *
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
 * http://www.projectmayo.com/opendivx/license.php                        *
 *                                                                        *
 **************************************************************************/
/**
*  Copyright (C) 2001 - Project Mayo
 *
 * Andrea Graziani (Ag)
 *
 * DivX Advanced Research Center <darc@projectmayo.com>
**/
// mp4_predict.c //

#include <math.h>
#include <stdlib.h>
#include "mp4_decoder.h"
#include "global.h"

#include "mp4_predict.h"

/**
 *
**/

static void rescue_predict();

/*

	B - C
	|   |
	A - x

*/

void dc_recon(int block_num, short * dc_value)
{
	if (mp4_hdr.prediction_type == P_VOP) {
		rescue_predict();
	}

	if (block_num < 4)
	{
		int b_xpos = (mp4_hdr.mb_xpos << 1) + (block_num & 1);
		int b_ypos = (mp4_hdr.mb_ypos << 1) + ((block_num & 2) >> 1);
		int dc_pred;

		// set prediction direction
		if (abs(coeff_pred->dc_store_lum[b_ypos+1-1][b_xpos+1-1] -
			coeff_pred->dc_store_lum[b_ypos+1][b_xpos+1-1]) < // Fa - Fb
			abs(coeff_pred->dc_store_lum[b_ypos+1-1][b_xpos+1-1] -
			coeff_pred->dc_store_lum[b_ypos+1-1][b_xpos+1])) // Fb - Fc
			{
				coeff_pred->predict_dir = TOP;
				dc_pred = coeff_pred->dc_store_lum[b_ypos+1-1][b_xpos+1];
			}
		else
			{
				coeff_pred->predict_dir = LEFT;
				dc_pred = coeff_pred->dc_store_lum[b_ypos+1][b_xpos+1-1];
			}

		(* dc_value) += _div_div(dc_pred, mp4_hdr.dc_scaler);
		(* dc_value) *= mp4_hdr.dc_scaler;

		// store dc value
		coeff_pred->dc_store_lum[b_ypos+1][b_xpos+1] = (* dc_value);
	}
	else // chrominance blocks
	{
		int b_xpos = mp4_hdr.mb_xpos;
		int b_ypos = mp4_hdr.mb_ypos;
		int chr_num = block_num - 4;
		int dc_pred;

		// set prediction direction
		if (abs(coeff_pred->dc_store_chr[chr_num][b_ypos+1-1][b_xpos+1-1] -
			coeff_pred->dc_store_chr[chr_num][b_ypos+1][b_xpos+1-1]) < // Fa - Fb
			abs(coeff_pred->dc_store_chr[chr_num][b_ypos+1-1][b_xpos+1-1] -
			coeff_pred->dc_store_chr[chr_num][b_ypos+1-1][b_xpos+1])) // Fb - Fc
			{
				coeff_pred->predict_dir = TOP;
				dc_pred = coeff_pred->dc_store_chr[chr_num][b_ypos+1-1][b_xpos+1];
			}
		else
			{
				coeff_pred->predict_dir = LEFT;
				dc_pred = coeff_pred->dc_store_chr[chr_num][b_ypos+1][b_xpos+1-1];
			}

		(* dc_value) += _div_div(dc_pred, mp4_hdr.dc_scaler);
		(* dc_value) *= mp4_hdr.dc_scaler;

		// store dc value
		coeff_pred->dc_store_chr[chr_num][b_ypos+1][b_xpos+1] = (* dc_value);
	}
}

/***/

static int saiAcLeftIndex[8] = 
{
	0, 8,16,24,32,40,48,56
};

void ac_recon(int block_num, short * psBlock)
{
	int b_xpos, b_ypos;
	int i;

	if (block_num < 4) {
		b_xpos = (mp4_hdr.mb_xpos << 1) + (block_num & 1);
		b_ypos = (mp4_hdr.mb_ypos << 1) + ((block_num & 2) >> 1);
	}
	else {
		b_xpos = mp4_hdr.mb_xpos;
		b_ypos = mp4_hdr.mb_ypos;
	}

	// predict coefficients
	if (mp4_hdr.ac_pred_flag) 
	{
		if (block_num < 4) 
		{
			if (coeff_pred->predict_dir == TOP)
			{
				for (i = 1; i < 8; i++) // [Review] index can become more efficient [0..7]
					psBlock[i] += coeff_pred->ac_top_lum[b_ypos+1-1][b_xpos+1][i-1];
			}
			else // left prediction
			{
				for (i = 1; i < 8; i++)
					psBlock[saiAcLeftIndex[i]] += coeff_pred->ac_left_lum[b_ypos+1][b_xpos+1-1][i-1];
			}
		}
		else
		{
			int chr_num = block_num - 4;

			if (coeff_pred->predict_dir == TOP)
			{
				for (i = 1; i < 8; i++)
					psBlock[i] += coeff_pred->ac_top_chr[chr_num][b_ypos+1-1][b_xpos+1][i-1];
			}
			else // left prediction
			{
				for (i = 1; i < 8; i++)
					psBlock[saiAcLeftIndex[i]] += coeff_pred->ac_left_chr[chr_num][b_ypos+1][b_xpos+1-1][i-1];
			}
		}
	}

	// store coefficients
	if (block_num < 4)
	{
		for (i = 1; i < 8; i++) {
			coeff_pred->ac_top_lum[b_ypos+1][b_xpos+1][i-1] = psBlock[i];
			coeff_pred->ac_left_lum[b_ypos+1][b_xpos+1][i-1] = psBlock[saiAcLeftIndex[i]];
		}
	}
	else 
	{
		int chr_num = block_num - 4;

		for (i = 1; i < 8; i++) {
			coeff_pred->ac_top_chr[chr_num][b_ypos+1][b_xpos+1][i-1] = psBlock[i];
			coeff_pred->ac_left_chr[chr_num][b_ypos+1][b_xpos+1][i-1] = psBlock[saiAcLeftIndex[i]];
		}
	}
}

/***/

#define _IsIntra(mb_y, mb_x) ((modemap[(mb_y)+1][(mb_x)+1] == INTRA) || \
	(modemap[(mb_y)+1][(mb_x)+1] == INTRA_Q))

static void rescue_predict() 
{
	int mb_xpos = mp4_hdr.mb_xpos;
	int mb_ypos = mp4_hdr.mb_ypos;
	int i;

	if (! _IsIntra(mb_ypos-1, mb_xpos-1)) {
		// rescue -A- DC value
		coeff_pred->dc_store_lum[2*mb_ypos+1-1][2*mb_xpos+1-1] = 1024;
		coeff_pred->dc_store_chr[0][mb_ypos+1-1][mb_xpos+1-1] = 1024;
		coeff_pred->dc_store_chr[1][mb_ypos+1-1][mb_xpos+1-1] = 1024;
	}
	// left
	if (! _IsIntra(mb_ypos, mb_xpos-1)) {
		// rescue -B- DC values
		coeff_pred->dc_store_lum[2*mb_ypos+1][2*mb_xpos+1-1] = 1024;
		coeff_pred->dc_store_lum[2*mb_ypos+1+1][2*mb_xpos+1-1] = 1024;
		coeff_pred->dc_store_chr[0][mb_ypos+1][mb_xpos+1-1] = 1024;
		coeff_pred->dc_store_chr[1][mb_ypos+1][mb_xpos+1-1] = 1024;
		//  rescue -B- AC values
		for(i = 0; i < 7; i++) {
			coeff_pred->ac_left_lum[2*mb_ypos+1][2*mb_xpos+1-1][i] = 0;
			coeff_pred->ac_left_lum[2*mb_ypos+1+1][2*mb_xpos+1-1][i] = 0;
			coeff_pred->ac_left_chr[0][mb_ypos+1][mb_xpos+1-1][i] = 0;
			coeff_pred->ac_left_chr[1][mb_ypos+1][mb_xpos+1-1][i] = 0;
		}
	}
	// top
	if (! _IsIntra(mb_ypos-1, mb_xpos)) {
		// rescue -C- DC values
		coeff_pred->dc_store_lum[2*mb_ypos+1-1][2*mb_xpos+1] = 1024;
		coeff_pred->dc_store_lum[2*mb_ypos+1-1][2*mb_xpos+1+1] = 1024;
		coeff_pred->dc_store_chr[0][mb_ypos+1-1][mb_xpos+1] = 1024;
		coeff_pred->dc_store_chr[1][mb_ypos+1-1][mb_xpos+1] = 1024;
		// rescue -C- AC values
		for(i = 0; i < 7; i++) {
			coeff_pred->ac_top_lum[2*mb_ypos+1-1][2*mb_xpos+1][i] = 0;
			coeff_pred->ac_top_lum[2*mb_ypos+1-1][2*mb_xpos+1+1][i] = 0;
			coeff_pred->ac_top_chr[0][mb_ypos+1-1][mb_xpos+1][i] = 0;
			coeff_pred->ac_top_chr[1][mb_ypos+1-1][mb_xpos+1][i] = 0;
		}
	}
}

/**************************************************************************
 *
 *	XVID MPEG-4 VIDEO CODEC
 *	common stuff
 *
 *	This program is an implementation of a part of one or more MPEG-4
 *	Video tools as specified in ISO/IEC 14496-2 standard.  Those intending
 *	to use this software module in hardware or software products are
 *	advised that its use may infringe existing patents or copyrights, and
 *	any such use would be at such party's own risk.  The original
 *	developer of this software module and his/her company, and subsequent
 *	editors and their companies, will have no liability for use of this
 *	software or modifications or derivatives thereof.
 *
 *	This program is free software; you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation; either version 2 of the License, or
 *	(at your option) any later version.
 *
 *	This program is distributed in the hope that it will be useful,
 *	but WITHOUT ANY WARRANTY; without even the implied warranty of
 *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *	GNU General Public License for more details.
 *
 *	You should have received a copy of the GNU General Public License
 *	along with this program; if not, write to the Free Software
 *	Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *************************************************************************/

/**************************************************************************
 *
 *	History:
 *
 *	22.12.2001	acdc_predict()
 *
 *************************************************************************/



#include "common.h"
#include "../xvid.h"


#define ABS(X) (((X)>0)?(X):-(X))
#define MIN(X, Y) ((X)<(Y)?(X):(Y))
#define MAX(X, Y) ((X)>(Y)?(X):(Y))
#define DIV_DIV(A,B)    ( (A) > 0 ? ((A)+((B)>>1))/(B) : ((A)-((B)>>1))/(B) )


void init_common(uint32_t cpu_flags) {

	idct_int32_init();

	fdct = fdct_int32;
	idct = idct_int32;

	emms = emms_c;

	quant_intra = quant_intra_c;
	dequant_intra = dequant_intra_c;
	quant_inter = quant_inter_c;
	dequant_inter = dequant_inter_c;

	quant4_intra = quant4_intra_c;
	dequant4_intra = dequant4_intra_c;
	quant4_inter = quant4_inter_c;
	dequant4_inter = dequant4_inter_c;

	transfer_8to16copy = transfer_8to16copy_c;
	transfer_16to8copy = transfer_16to8copy_c;
	transfer_8to8copy = transfer_8to8copy_c;
	transfer_8to8add16 = transfer_8to8add16_c;
	transfer_16to8add = transfer_16to8add_c;

	compensate = compensate_c;

#ifdef USE_MMX
	if((cpu_flags & XVID_CPU_MMX) > 0) {
		fdct = fdct_mmx;
		idct = idct_mmx;

		emms = emms_mmx;

		quant_intra = quant_intra_mmx;
		dequant_intra = dequant_intra_mmx;
		quant_inter = quant_inter_mmx;
		dequant_inter = dequant_inter_mmx;

		transfer_8to16copy = transfer_8to16copy_mmx;
		transfer_16to8copy = transfer_16to8copy_mmx;
		transfer_8to8copy = transfer_8to8copy_mmx;
		transfer_8to8add16 = transfer_8to8add16_mmx;
		transfer_16to8add = transfer_16to8add_mmx;

		compensate = compensate_mmx;
	}

	if((cpu_flags & XVID_CPU_MMXEXT) > 0) {
		idct = idct_xmm;
	}
#endif
}


emmsFuncPtr emms;
void emms_c() {}
void emms_mmx() { EMMS(); }


/* calculate the pmv (predicted motion vector)
	(take the median of surrounding motion vectors)
	
	(x,y) = the macroblock
	block = the block within the macroblock
*/
void get_pmv(const MACROBLOCK * const pMBs,
							const uint32_t x, const uint32_t y,
							const uint32_t x_dim,
							const uint32_t block,
							int32_t * const pred_x, int32_t * const pred_y)
{
    int x1, x2, x3;
	int y1, y2, y3;
    int xin1, xin2, xin3;
    int yin1, yin2, yin3;
    int vec1, vec2, vec3;

    uint32_t index = x + y * x_dim;

	// first row (special case)
    if (y == 0 && (block == 0 || block == 1))
    {
		if (x == 0 && block == 0)		// first column
		{
			*pred_x = 0;
			*pred_y = 0;
			return;
		}
		if (block == 1)
		{
			VECTOR mv = pMBs[index].mvs[0];
			*pred_x = mv.x;
			*pred_y = mv.y;
			return;
		}
		// else
		{
			VECTOR mv = pMBs[index - 1].mvs[1];
			*pred_x = mv.x;
			*pred_y = mv.y;
			return;
		}
    }

	/*
		MODE_INTER, vm18 page 48
		MODE_INTER4V vm18 page 51

					(x,y-1)		(x+1,y-1)
					[   |   ]	[	|   ]
					[ 2 | 3 ]	[ 2 |   ]

		(x-1,y)		(x,y)		(x+1,y)
		[   | 1 ]	[ 0 | 1 ]	[ 0 |   ]
		[   | 3 ]	[ 2 | 3 ]	[	|   ]
	*/

    switch (block)
    {
	case 0:
		xin1 = x - 1;	yin1 = y;		vec1 = 1;
		xin2 = x;		yin2 = y - 1;	vec2 = 2;
		xin3 = x + 1;	yin3 = y - 1;	vec3 = 2;
		break;
	case 1:
		xin1 = x;		yin1 = y;		vec1 = 0;
		xin2 = x;		yin2 = y - 1;   vec2 = 3;
		xin3 = x + 1;	yin3 = y - 1;	vec3 = 2;
	    break;
	case 2:
		xin1 = x - 1;	yin1 = y;		vec1 = 3;
		xin2 = x;		yin2 = y;		vec2 = 0;
		xin3 = x;		yin3 = y;		vec3 = 1;
	    break;
	default:
		xin1 = x;		yin1 = y;		vec1 = 2;
		xin2 = x;		yin2 = y;		vec2 = 0;
		xin3 = x;		yin3 = y;		vec3 = 1;
    }


	if (xin1 < 0 || /* yin1 < 0  || */ xin1 >= (int32_t)x_dim)
	{
	    x1 = 0;
		y1 = 0;
	}
	else
	{
		const VECTOR * const mv = &(pMBs[xin1 + yin1 * x_dim].mvs[vec1]); 
		x1 = mv->x;
		y1 = mv->y;
	}

	if (xin2 < 0 || /* yin2 < 0 || */ xin2 >= (int32_t)x_dim)
	{
		x2 = 0;
		y2 = 0;
	}
	else
	{
		const VECTOR * const mv = &(pMBs[xin2 + yin2 * x_dim].mvs[vec2]); 
		x2 = mv->x;
		y2 = mv->y;
	}

	if (xin3 < 0 || /* yin3 < 0 || */ xin3 >= (int32_t)x_dim)
	{
	    x3 = 0;
		y3 = 0;
	}
	else
	{
		const VECTOR * const mv = &(pMBs[xin3 + yin3 * x_dim].mvs[vec3]); 
		x3 = mv->x;
		y3 = mv->y;
	}

	// median

	*pred_x = MIN(MAX(x1, x2), MIN(MAX(x2, x3), MAX(x1, x3)));
	*pred_y = MIN(MAX(y1, y2), MIN(MAX(y2, y3), MAX(y1, y3)));
}






static int __inline rescale(int predict_quant, int current_quant, int coeff)
{
	return (coeff != 0) ? DIV_DIV((coeff) * (predict_quant), (current_quant)) : 0;
}


static const int16_t default_acdc_values[15] = { 
	1024,
    0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0
};


/*	get dc/ac prediction direction for a single block and place
	predictor values into MB->pred_values[j][..]
*/


void predict_acdc(MACROBLOCK *pMBs,
				uint32_t x, uint32_t y,	uint32_t mb_width, 
				uint32_t block, 
				int16_t qcoeff[64],
				uint32_t current_quant,
				uint32_t iDcScaler,
				int16_t predictors[8])
{
    int16_t *left, *top, *diag, *current;

    int32_t left_quant = current_quant;
    int32_t top_quant = current_quant;

    const int16_t *pLeft = default_acdc_values;
    const int16_t *pTop = default_acdc_values;
    const int16_t *pDiag = default_acdc_values;

    uint32_t index = x + y * mb_width;		// current macroblock
    int * acpred_direction = &pMBs[index].acpred_directions[block];
	uint32_t i;

	left = top = diag = current = 0;

	// grab left,top and diag macroblocks

	// left macroblock 

    if(x && (pMBs[index - 1].mode == MODE_INTRA 
		|| pMBs[index - 1].mode == MODE_INTRA_Q)) {

		left = pMBs[index - 1].pred_values[0];
		left_quant = pMBs[index - 1].quant;
		//DEBUGI("LEFT", *(left+MBPRED_SIZE));
	}
    
	// top macroblock
	
	if(y && (pMBs[index - mb_width].mode == MODE_INTRA 
		|| pMBs[index - mb_width].mode == MODE_INTRA_Q)) {

		top = pMBs[index - mb_width].pred_values[0];
		top_quant = pMBs[index - mb_width].quant;
    }
    
	// diag macroblock 
	
	if(x && y && (pMBs[index - 1 - mb_width].mode == MODE_INTRA 
		|| pMBs[index - 1 - mb_width].mode == MODE_INTRA_Q)) {

		diag = pMBs[index - 1 - mb_width].pred_values[0];
	}

    current = pMBs[index].pred_values[0];

	// now grab pLeft, pTop, pDiag _blocks_ 
	
	switch (block) {
	
	case 0: 
		if(left)
			pLeft = left + MBPRED_SIZE;
		
		if(top)
			pTop = top + (MBPRED_SIZE << 1);
		
		if(diag)
			pDiag = diag + 3 * MBPRED_SIZE;
		
		break;
	
	case 1:
		pLeft = current;
		left_quant = current_quant;
	
		if(top) {
			pTop = top + 3 * MBPRED_SIZE;
			pDiag = top + (MBPRED_SIZE << 1);
		}
		break;
	
	case 2:
		if(left) {
			pLeft = left + 3 * MBPRED_SIZE;
			pDiag = left + MBPRED_SIZE;
		}
		
		pTop = current;
		top_quant = current_quant;

		break;
	
	case 3:
		pLeft = current + (MBPRED_SIZE << 1);
		left_quant = current_quant;
		
		pTop = current + MBPRED_SIZE;
		top_quant = current_quant;
		
		pDiag = current;
		
		break;
	
	case 4:
		if(left)
			pLeft = left + (MBPRED_SIZE << 2);
		if(top)
			pTop = top + (MBPRED_SIZE << 2);
		if(diag)
			pDiag = diag + (MBPRED_SIZE << 2);
		break;
	
	case 5:
		if(left)
			pLeft = left + 5 * MBPRED_SIZE;
		if(top)
			pTop = top + 5 * MBPRED_SIZE;
		if(diag)
			pDiag = diag + 5 * MBPRED_SIZE;
		break;
	}

    //	determine ac prediction direction & ac/dc predictor
	//	place rescaled ac/dc predictions into predictors[] for later use

    if(ABS(pLeft[0] - pDiag[0]) < ABS(pDiag[0] - pTop[0])) {
		*acpred_direction = 1;             // vertical
		predictors[0] = DIV_DIV(pTop[0], iDcScaler);
		for (i = 1; i < 8; i++)
		{
			predictors[i] = rescale(top_quant, current_quant, pTop[i]);
		}
	}
	else 
	{
		*acpred_direction = 2;             // horizontal
		predictors[0] = DIV_DIV(pLeft[0], iDcScaler);
		for (i = 1; i < 8; i++)
		{
			predictors[i] = rescale(left_quant, current_quant, pLeft[i + 7]);
		}
	}
}

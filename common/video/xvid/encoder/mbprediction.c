 /******************************************************************************
  *                                                                            *
  *  This file is part of XviD, a free MPEG-4 video encoder/decoder            *
  *                                                                            *
  *  XviD is an implementation of a part of one or more MPEG-4 Video tools     *
  *  as specified in ISO/IEC 14496-2 standard.  Those intending to use this    *
  *  software module in hardware or software products are advised that its     *
  *  use may infringe existing patents or copyrights, and any such use         *
  *  would be at such party's own risk.  The original developer of this        *
  *  software module and his/her company, and subsequent editors and their     *
  *  companies, will have no liability for use of this software or             *
  *  modifications or derivatives thereof.                                     *
  *                                                                            *
  *  XviD is free software; you can redistribute it and/or modify it           *
  *  under the terms of the GNU General Public License as published by         *
  *  the Free Software Foundation; either version 2 of the License, or         *
  *  (at your option) any later version.                                       *
  *                                                                            *
  *  XviD is distributed in the hope that it will be useful, but               *
  *  WITHOUT ANY WARRANTY; without even the implied warranty of                *
  *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the             *
  *  GNU General Public License for more details.                              *
  *                                                                            *
  *  You should have received a copy of the GNU General Public License         *
  *  along with this program; if not, write to the Free Software               *
  *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA  *
  *                                                                            *
  ******************************************************************************/

 /******************************************************************************
  *                                                                            *
  *  mbprediction.c                                                            *
  *                                                                            *
  *  Copyright (C) 2001 - Michael Militzer <isibaar@xvid.org>                  *
  *  Copyright (C) 2001 - Peter Ross <pross@cs.rmit.edu.au>                    *
  *                                                                            *
  *  For more information visit the XviD homepage: http://www.xvid.org         *
  *                                                                            *
  ******************************************************************************/

 /******************************************************************************
  *                                                                            *
  *  Revision history:                                                         *
  *                                                                            *
  *  12.12.2001 improved calc_acdc_prediction; removed need for memcpy
  *  15.12.2001 moved pmv displacement to motion estimation
  *  30.11.2001	mmx cbp support
  *  17.11.2001 initial version                                                *
  *                                                                            *
  ******************************************************************************/


#include "encoder.h"
#include "mbfunctions.h"
#include "cbp.h"


#define ABS(X) (((X)>0)?(X):-(X))


// ******************************************************************
// ******************************************************************

/* encoder: subtract predictors from qcoeff[] and calculate S1/S2

	todo: perform [-127,127] clamping after prediction
		clamping must adjust the coeffs, so dequant is done correctly
				   
	S1/S2 are used  to determine if its worth predicting for AC
		S1 = sum of all (qcoeff - prediction)
		S2 = sum of all qcoeff
	*/

uint32_t calc_acdc(MACROBLOCK *pMB,
				uint32_t block, 
				int16_t qcoeff[64],
				uint32_t iDcScaler,
				int16_t predictors[8])
{
	int16_t * pCurrent = pMB->pred_values[block];
	uint32_t i;
	uint32_t S1 = 0, S2 = 0;


	/* store current coeffs to pred_values[] for future prediction */

	pCurrent[0] = qcoeff[0] * iDcScaler;
	for(i = 1; i < 8; i++) {
		pCurrent[i] = qcoeff[i];
		pCurrent[i + 7] = qcoeff[i * 8];
    }

	/* subtract predictors and store back in predictors[] */

	qcoeff[0] = qcoeff[0] - predictors[0];

	if (pMB->acpred_directions[block] == 1) 
	{
		for(i = 1; i < 8; i++) {
			int16_t level;

			level = qcoeff[i];
			S2 += ABS(level);
			level -= predictors[i];
			S1 += ABS(level);
			predictors[i] = level;
		}
	}
    else // acpred_direction == 2
	{
		for(i = 1; i < 8; i++) {
			int16_t level;

			level = qcoeff[i*8];
			S2 += ABS(level);
			level -= predictors[i];
			S1 += ABS(level);
			predictors[i] = level;
		}

    }

    
    return S2 - S1;
}



/* apply predictors[] to qcoeff */

void apply_acdc(MACROBLOCK *pMB,
				uint32_t block, 
				int16_t qcoeff[64],
				int16_t predictors[8])
{
	uint32_t i;

	if (pMB->acpred_directions[block] == 1) 
	{
		for(i = 1; i < 8; i++) 
		{
			qcoeff[i] = predictors[i];
		}
	}
    else 
	{
		for(i = 1; i < 8; i++) 
		{
			qcoeff[i*8] = predictors[i];
		}
    }
}




void MBPrediction(MBParam *pParam, uint32_t x, uint32_t y,
				  uint32_t mb_width, int16_t qcoeff[][64], MACROBLOCK *mbs)
{
    int32_t j;
	int32_t iDcScaler, iQuant = pParam->quant;
	int32_t S = 0;
	int16_t predictors[6][8];

    MACROBLOCK *pMB = &mbs[x + y * mb_width];

    if ((pMB->mode == MODE_INTRA) || (pMB->mode == MODE_INTRA_Q)) {
		
		for(j = 0; j < 6; j++) 
		{
			iDcScaler = get_dc_scaler(iQuant, (j < 4) ? 1 : 0);

			predict_acdc(mbs, x, y, mb_width, j, qcoeff[j], iQuant, iDcScaler, predictors[j]);
			S += calc_acdc(pMB, j, qcoeff[j], iDcScaler, predictors[j]);
		}

		if (S < 0)		// dont predict
		{			
			for(j = 0; j < 6; j++) 
			{
				pMB->acpred_directions[j] = 0;
			}
		}
		else
		{
			for(j = 0; j < 6; j++) 
			{
				 apply_acdc(pMB, j, qcoeff[j], predictors[j]);
			}
		}

		pMB->cbp = calc_cbp(qcoeff);
    }

}

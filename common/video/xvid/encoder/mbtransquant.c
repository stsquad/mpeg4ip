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
  *  mbtransquant.c                                                            *
  *                                                                            *
  *  Copyright (C) 2001 - Peter Ross <pross@cs.rmit.edu.au>                    *
  *  Copyright (C) 2001 - Michael Militzer <isibaar@xvid.org>                  *
  *                                                                            *
  *  For more information visit the XviD homepage: http://www.xvid.org         *
  *                                                                            *
  ******************************************************************************/

 /******************************************************************************
  *                                                                            *
  *  Revision history:                                                         *
  *                                                                            *
  *  22.12.2001 get_dc_scaler() moved to common.h
  *  19.11.2001 introduced coefficient thresholding (Isibaar)                  *
  *  17.11.2001 initial version                                                *
  *                                                                            *
  ******************************************************************************/

#include "../portab.h"
#include "mbfunctions.h"

#include "../common/common.h"
#include "../common/timer.h"


#define MIN(X, Y) ((X)<(Y)?(X):(Y))
#define MAX(X, Y) ((X)>(Y)?(X):(Y))

#define TOOSMALL_LIMIT 1 /* skip blocks having a coefficient sum below this value */

/* this isnt pretty, but its better than 20 ifdefs */

void MBTransQuantIntra(const MBParam *pParam,
		       const uint32_t x_pos,
		       const uint32_t y_pos,
		       int16_t data[][64], 
			   int16_t qcoeff[][64], 
			   IMAGE * const pCurrent)
			   
{
	const uint32_t stride = pParam->edged_width;
	uint32_t i;
	uint32_t iQuant = pParam->quant;
	uint8_t *pY_Cur, *pU_Cur, *pV_Cur;

    pY_Cur = pCurrent->y + (y_pos << 4) * stride + (x_pos << 4);
    pU_Cur = pCurrent->u + (y_pos << 3) * (stride >> 1) + (x_pos << 3);
    pV_Cur = pCurrent->v + (y_pos << 3) * (stride >> 1) + (x_pos << 3);
    
	for(i = 0; i < 6; i++) {
		uint32_t iDcScaler = get_dc_scaler(iQuant, i < 4);

		start_timer();
		
		switch(i) {
		case 0 :
			transfer_8to16copy(data[0], pY_Cur, stride);
			break;
		case 1 :
			transfer_8to16copy(data[1], pY_Cur + 8, stride);
			break;
		case 2 :
		    transfer_8to16copy(data[2], pY_Cur + 8 * stride, stride);
			break;
		case 3 :
			transfer_8to16copy(data[3], pY_Cur + 8 * stride + 8, stride);
			break;
		case 4 :
			transfer_8to16copy(data[4], pU_Cur, stride / 2);
			break;
		case 5 :
			transfer_8to16copy(data[5], pV_Cur, stride / 2);
			break;
		}
		stop_transfer_timer();

		start_timer();
		fdct(data[i]);
		stop_dct_timer();

		if (pParam->quant_type == 0)
		{
			start_timer();
			quant_intra(qcoeff[i], data[i], iQuant, iDcScaler);
			stop_quant_timer();

			start_timer();
			dequant_intra(data[i], qcoeff[i], iQuant, iDcScaler);
			stop_iquant_timer();
		}
		else
		{
			start_timer();
			quant4_intra(qcoeff[i], data[i], iQuant, iDcScaler);
			stop_quant_timer();

			start_timer();
			dequant4_intra(data[i], qcoeff[i], iQuant, iDcScaler);
			stop_iquant_timer();
		}

		start_timer();
		idct(data[i]);
		stop_idct_timer();

		start_timer();
		
		switch(i) {
		case 0:
			transfer_16to8copy(pY_Cur, data[0], stride);
			break;
		case 1:
			transfer_16to8copy(pY_Cur + 8, data[1], stride);
			break;
		case 2:
			transfer_16to8copy(pY_Cur + 8 * stride, data[2], stride);
			break;
		case 3:
			transfer_16to8copy(pY_Cur + 8 + 8 * stride, data[3], stride);
			break;
		case 4:
			transfer_16to8copy(pU_Cur, data[4], stride / 2);
			break;
		case 5:
			transfer_16to8copy(pV_Cur, data[5], stride / 2);
			break;
		}
		stop_transfer_timer();
    }
}


uint8_t MBTransQuantInter(const MBParam *pParam, 
					const uint32_t x_pos, const uint32_t y_pos,
					int16_t data[][64], 
					int16_t qcoeff[][64], 
					IMAGE * const pCurrent)

{
	const uint32_t stride = pParam->edged_width;
    uint8_t i;
    uint8_t iQuant = pParam->quant;
	uint8_t *pY_Cur, *pU_Cur, *pV_Cur;
    uint8_t cbp = 0;
	uint32_t sum;
    
    pY_Cur = pCurrent->y + (y_pos << 4) * stride + (x_pos << 4);
    pU_Cur = pCurrent->u + (y_pos << 3) * (stride >> 1) + (x_pos << 3);
    pV_Cur = pCurrent->v + (y_pos << 3) * (stride >> 1) + (x_pos << 3);

    for(i = 0; i < 6; i++) {
		/* 
		no need to transfer 8->16-bit
		(this is performed already in motion compensation) 
		*/
		start_timer();
		fdct(data[i]);
		stop_dct_timer();

		if (pParam->quant_type == 0)
		{
			start_timer();
			sum = quant_inter(qcoeff[i], data[i], iQuant);
			stop_quant_timer();
		}
		else
		{
			start_timer();
			sum = quant4_inter(qcoeff[i], data[i], iQuant);
			stop_quant_timer();
		}

		if(sum >= TOOSMALL_LIMIT) { // skip block ?

			if (pParam->quant_type == 0)
			{
				start_timer();
				dequant_inter(data[i], qcoeff[i], iQuant);
				stop_iquant_timer();
			}
			else
			{
				start_timer();
				dequant4_inter(data[i], qcoeff[i], iQuant);
				stop_iquant_timer();
			}

			cbp |= 1 << (5 - i);

			start_timer();
			idct(data[i]);
			stop_idct_timer();

			start_timer();
			
			switch(i) {
			case 0:
				transfer_16to8add(pY_Cur, data[0], stride);
				break;
			case 1:
				transfer_16to8add(pY_Cur + 8, data[1], stride);
				break;
			case 2:
				transfer_16to8add(pY_Cur + 8 * stride, data[2], stride);
				break;
			case 3:
				transfer_16to8add(pY_Cur + 8 + 8 * stride, data[3], stride);
				break;
			case 4:
				transfer_16to8add(pU_Cur, data[4], stride / 2);
				break;
			case 5:
				transfer_16to8add(pV_Cur, data[5], stride / 2);
				break;
			}
			stop_transfer_timer();
		}
	}
    return cbp;
}

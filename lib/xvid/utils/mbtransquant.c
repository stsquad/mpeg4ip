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
  *  29.03.2002 interlacing speedup - used transfer strides instead of
  *             manual field-to-frame conversion
  *  26.03.2002 interlacing support - moved transfers outside loops
  *  22.12.2001 get_dc_scaler() moved to common.h
  *  19.11.2001 introduced coefficient thresholding (Isibaar)                  *
  *  17.11.2001 initial version                                                *
  *                                                                            *
  ******************************************************************************/

#include <string.h>

#include "../portab.h"
#include "mbfunctions.h"

#include "../global.h"
#include "mem_transfer.h"
#include "timer.h"
#include "../dct/fdct.h"
#include "../dct/idct.h"
#include "../quant/quant_mpeg4.h"
#include "../quant/quant_h263.h"
#include "../encoder.h"

#define MIN(X, Y) ((X)<(Y)?(X):(Y))
#define MAX(X, Y) ((X)>(Y)?(X):(Y))

#define TOOSMALL_LIMIT 1 /* skip blocks having a coefficient sum below this value */

/* this isnt pretty, but its better than 20 ifdefs */

void MBTransQuantIntra(const MBParam *pParam,
		       MACROBLOCK * pMB,
		       const uint32_t x_pos,
		       const uint32_t y_pos,
		       int16_t data[6*64], 
		       int16_t qcoeff[6*64], 
		       IMAGE * const pCurrent)
			   
{

	uint32_t stride = pParam->edged_width;
	uint32_t stride2 = stride / 2;
	uint32_t next_block = stride * 8;
	uint32_t i;
	uint32_t iQuant = pParam->quant;
	uint8_t *pY_Cur, *pU_Cur, *pV_Cur;

	pY_Cur = pCurrent->y + (y_pos << 4) * stride + (x_pos << 4);
	pU_Cur = pCurrent->u + (y_pos << 3) * stride2 + (x_pos << 3);
	pV_Cur = pCurrent->v + (y_pos << 3) * stride2 + (x_pos << 3);

	start_timer();
	transfer_8to16copy(&data[0*64], pY_Cur,					stride);
	transfer_8to16copy(&data[1*64], pY_Cur + 8,				stride);
	transfer_8to16copy(&data[2*64], pY_Cur + next_block,	stride);
	transfer_8to16copy(&data[3*64], pY_Cur + next_block + 8,stride);
	transfer_8to16copy(&data[4*64], pU_Cur,					stride2);
	transfer_8to16copy(&data[5*64], pV_Cur,					stride2);
	stop_transfer_timer();

	start_timer();
	pMB->field_dct = 0;
	if (pParam->global_flags & XVID_INTERLACING)
	{
		pMB->field_dct = MBDecideFieldDCT(data);
	}
	stop_interlacing_timer();

	for(i = 0; i < 6; i++)
	{
		uint32_t iDcScaler = get_dc_scaler(iQuant, i < 4);

		start_timer();
		fdct(&data[i*64]);
		stop_dct_timer();

		if (pParam->quant_type == H263_QUANT)
		{
			start_timer();
			quant_intra(&qcoeff[i*64], &data[i*64], iQuant, iDcScaler);
			stop_quant_timer();

			start_timer();
			dequant_intra(&data[i*64], &qcoeff[i*64], iQuant, iDcScaler);
			stop_iquant_timer();
		}
		else
		{
			start_timer();
			quant4_intra(&qcoeff[i*64], &data[i*64], iQuant, iDcScaler);
			stop_quant_timer();

			start_timer();
			dequant4_intra(&data[i*64], &qcoeff[i*64], iQuant, iDcScaler);
			stop_iquant_timer();
		}

		start_timer();
		idct(&data[i*64]);
		stop_idct_timer();
	}

	if (pMB->field_dct)
	{
		next_block = stride;
		stride *= 2;
	}

	start_timer();
	transfer_16to8copy(pY_Cur,                  &data[0*64], stride);
	transfer_16to8copy(pY_Cur + 8,              &data[1*64], stride);
	transfer_16to8copy(pY_Cur + next_block,     &data[2*64], stride);
	transfer_16to8copy(pY_Cur + next_block + 8, &data[3*64], stride);
	transfer_16to8copy(pU_Cur,                  &data[4*64], stride2);
	transfer_16to8copy(pV_Cur,                  &data[5*64], stride2);
	stop_transfer_timer();

}


uint8_t MBTransQuantInter(const MBParam *pParam, 
			  MACROBLOCK * pMB,
			  const uint32_t x_pos, const uint32_t y_pos,
			  int16_t data[6*64], 
			  int16_t qcoeff[6*64], 
			  IMAGE * const pCurrent)

{

	uint32_t stride = pParam->edged_width;
	uint32_t stride2 = stride / 2;
	uint32_t next_block = stride * 8;
	uint32_t i;
	uint32_t iQuant = pParam->quant;
	uint8_t *pY_Cur, *pU_Cur, *pV_Cur;
	uint8_t cbp = 0;
	uint32_t sum;
    
	pY_Cur = pCurrent->y + (y_pos << 4) * stride + (x_pos << 4);
	pU_Cur = pCurrent->u + (y_pos << 3) * stride2 + (x_pos << 3);
	pV_Cur = pCurrent->v + (y_pos << 3) * stride2 + (x_pos << 3);

	start_timer();
	pMB->field_dct = 0;
	if (pParam->global_flags & XVID_INTERLACING)
	{
		pMB->field_dct = MBDecideFieldDCT(data);
	}
	stop_interlacing_timer();

	for(i = 0; i < 6; i++)
	{
		/* 
		 *  no need to transfer 8->16-bit
		 * (this is performed already in motion compensation) 
		 */
		start_timer();
		fdct(&data[i*64]);
		stop_dct_timer();

		if (pParam->quant_type == 0)
		{
			start_timer();
			sum = quant_inter(&qcoeff[i*64], &data[i*64], iQuant);
			stop_quant_timer();
		}
		else
		{
			start_timer();
			sum = quant4_inter(&qcoeff[i*64], &data[i*64], iQuant);
			stop_quant_timer();
		}

		if(sum >= TOOSMALL_LIMIT) { // skip block ?

			if (pParam->quant_type == H263_QUANT)
			{
				start_timer();
				dequant_inter(&data[i*64], &qcoeff[i*64], iQuant);
				stop_iquant_timer();
			}
			else
			{
				start_timer();
				dequant4_inter(&data[i*64], &qcoeff[i*64], iQuant);
				stop_iquant_timer();
			}

			cbp |= 1 << (5 - i);

			start_timer();
			idct(&data[i*64]);
			stop_idct_timer();
		}
	}

	if (pMB->field_dct)
	{
		next_block = stride;
		stride *= 2;
	}

	start_timer();
	if (cbp & 32)
		transfer_16to8add(pY_Cur,                  &data[0*64], stride);
	if (cbp & 16)
		transfer_16to8add(pY_Cur + 8,              &data[1*64], stride);
	if (cbp & 8)
		transfer_16to8add(pY_Cur + next_block,     &data[2*64], stride);
	if (cbp & 4)
		transfer_16to8add(pY_Cur + next_block + 8, &data[3*64], stride);
	if (cbp & 2)
		transfer_16to8add(pU_Cur,                  &data[4*64], stride2);
	if (cbp & 1)
		transfer_16to8add(pV_Cur,                  &data[5*64], stride2);
	stop_transfer_timer();

	return cbp;

}


/* if sum(diff between field lines) < sum(diff between frame lines), use field dct */

#define ABS(X) (X)<0 ? -(X) : (X)

uint32_t MBDecideFieldDCT(int16_t data[6*64])
{

	const uint8_t blocks[] = {0*64, 0*64, 0*64, 0*64, 2*64, 2*64, 2*64, 2*64};
	const uint8_t lines[]  = {0, 16, 32, 48, 0, 16, 32, 48};

	int frame = 0, field = 0;
	int i, j;

	for (i=0 ; i<7 ; ++i)
	{
		for (j=0 ; j<8 ; ++j)
		{
			frame += ABS(data[0*64 + (i+1)*8 + j] - data[0*64 + i*8 + j]);
			frame += ABS(data[1*64 + (i+1)*8 + j] - data[1*64 + i*8 + j]);
			frame += ABS(data[2*64 + (i+1)*8 + j] - data[2*64 + i*8 + j]);
			frame += ABS(data[3*64 + (i+1)*8 + j] - data[3*64 + i*8 + j]);

			field += ABS(data[blocks[i+1] + lines[i+1] + j] -\
				     data[blocks[i  ] + lines[i  ] + j]);
			field += ABS(data[blocks[i+1] + lines[i+1] + 8 + j] -\
				     data[blocks[i  ] + lines[i  ] + 8 + j]);
			field += ABS(data[blocks[i+1] + 64 + lines[i+1] + j] -\
				     data[blocks[i  ] + 64 + lines[i  ] + j]);
			field += ABS(data[blocks[i+1] + 64 + lines[i+1] + 8 + j] -\
				     data[blocks[i  ] + 64 + lines[i  ] + 8 + j]);
		}
	}

	if (frame > field)
	{
		MBFrameToField(data);
	}

	return (frame > field);
}


/* deinterlace Y blocks vertically */

#define MOVLINE(X,Y) memcpy(X, Y, sizeof(tmp))
#define LINE(X,Y)    &data[X*64 + Y*8]

void MBFrameToField(int16_t data[6*64])
{
	int16_t tmp[8];

	/* left blocks */

	// 1=2, 2=4, 4=8, 8=1
	MOVLINE(tmp,		LINE(0,1));
	MOVLINE(LINE(0,1),	LINE(0,2));
	MOVLINE(LINE(0,2),	LINE(0,4));
	MOVLINE(LINE(0,4),	LINE(2,0));
	MOVLINE(LINE(2,0),	tmp);

	// 3=6, 6=12, 12=9, 9=3
	MOVLINE(tmp,		LINE(0,3));
	MOVLINE(LINE(0,3),	LINE(0,6));
	MOVLINE(LINE(0,6),	LINE(2,4));
	MOVLINE(LINE(2,4),	LINE(2,1));
	MOVLINE(LINE(2,1),	tmp);

	// 5=10, 10=5
	MOVLINE(tmp,		LINE(0,5));
	MOVLINE(LINE(0,5),	LINE(2,2));
	MOVLINE(LINE(2,2),	tmp);

	// 7=14, 14=13, 13=11, 11=7
	MOVLINE(tmp,		LINE(0,7));
	MOVLINE(LINE(0,7),	LINE(2,6));
	MOVLINE(LINE(2,6),	LINE(2,5));
	MOVLINE(LINE(2,5),	LINE(2,3));
	MOVLINE(LINE(2,3),	tmp);

	/* right blocks */

	// 1=2, 2=4, 4=8, 8=1
	MOVLINE(tmp,		LINE(1,1));
	MOVLINE(LINE(1,1),	LINE(1,2));
	MOVLINE(LINE(1,2),	LINE(1,4));
	MOVLINE(LINE(1,4),	LINE(3,0));
	MOVLINE(LINE(3,0),	tmp);

	// 3=6, 6=12, 12=9, 9=3
	MOVLINE(tmp,		LINE(1,3));
	MOVLINE(LINE(1,3),	LINE(1,6));
	MOVLINE(LINE(1,6),	LINE(3,4));
	MOVLINE(LINE(3,4),	LINE(3,1));
	MOVLINE(LINE(3,1),	tmp);

	// 5=10, 10=5
	MOVLINE(tmp,		LINE(1,5));
	MOVLINE(LINE(1,5),	LINE(3,2));
	MOVLINE(LINE(3,2),	tmp);

	// 7=14, 14=13, 13=11, 11=7
	MOVLINE(tmp,		LINE(1,7));
	MOVLINE(LINE(1,7),	LINE(3,6));
	MOVLINE(LINE(3,6),	LINE(3,5));
	MOVLINE(LINE(3,5),	LINE(3,3));
	MOVLINE(LINE(3,3),	tmp);
}

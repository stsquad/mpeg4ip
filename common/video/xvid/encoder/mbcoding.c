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
 *  MBCoding.c
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
 *  15.12.2001  added textbits calculation for intra macroblocks
 *	02.11.2001	cleanup; removed ancient debug stuff
 *  24.08.2001 INTER_Q MBs shouldn't be skipped
 *  22.08.2001 support for MODE_INTRA_Q/MODE_INTER_Q encoding modes
 *
 *  Michael Militzer <isibaar@videocoding.de>
 *
 **************************************************************************/

#include "putvlc.h"
#include "mbfunctions.h"




static const uint8_t zig_zag_scan[64] = {
    0, 1, 8, 16, 9, 2, 3, 10, 17, 24, 32, 25, 18, 11, 4, 5,
    12, 19, 26, 33, 40, 48, 41, 34, 27, 20, 13, 6, 7, 14, 21, 28,
    35, 42, 49, 56, 57, 50, 43, 36, 29, 22, 15, 23, 30, 37, 44, 51,
    58, 59, 52, 45, 38, 31, 39, 46, 53, 60, 61, 54, 47, 55, 62, 63
};



// other scan orders

static const uint8_t alternate_horizontal_scan[64] = {
    0, 1, 2, 3, 8, 9, 16, 17,
    10, 11, 4, 5, 6, 7, 15, 14,
    13, 12, 19, 18, 24, 25, 32, 33,
    26, 27, 20, 21, 22, 23, 28, 29,
    30, 31, 34, 35, 40, 41, 48, 49,
    42, 43, 36, 37, 38, 39, 44, 45,
    46, 47, 50, 51, 56, 57, 58, 59,
    52, 53, 54, 55, 60, 61, 62, 63
};



static const uint8_t alternate_vertical_scan[64] = {
    0, 8, 16, 24, 1, 9, 2, 10,
    17, 25, 32, 40, 48, 56, 57, 49,
    41, 33, 26, 18, 3, 11, 4, 12,
    19, 27, 34, 42, 50, 58, 35, 43,
    51, 59, 20, 28, 5, 13, 6, 14,
    21, 29, 36, 44, 52, 60, 37, 45,
    53, 61, 22, 30, 7, 15, 23, 31,
    38, 46, 54, 62, 39, 47, 55, 63
};




static void EncodeBlockInter(Bitstream * bs, Statistics * pStat,
							 const int16_t dct_codes[64])
{
    int run;
    int prev_run = 0, prev_level = 0;
    const uint8_t *zigzag = zig_zag_scan;
    int pos;
    int bits;

    bits = BsPos(bs);
    run = 0;

    for (pos = 0; pos < 64; pos++)
    {
		int index = zigzag[pos];
		int value = dct_codes[index];

		if (value == 0)
		{
		    run++;
		    continue;
		}

		if (prev_level)
		    PutCoeff(bs, prev_run, prev_level, 0, MODE_INTER);

		prev_run = run;
		prev_level = value;

		run = 0;
    }

    PutCoeff(bs, prev_run, prev_level, 1, MODE_INTER);

    bits = BsPos(bs) - bits;
    pStat->iTextBits += bits;
}



static void EncodeBlockIntra(Bitstream * bs, Statistics * pStat,
				const int16_t dct_codes[64], int acpred_direction)

{
    int run;
    int prev_run = 0, prev_level = 0;
    const uint8_t *zigzag;
    int pos;
    int bits;

    bits = BsPos(bs);

	if (acpred_direction == 0)
		zigzag = zig_zag_scan;
    else if (acpred_direction == 1)
		zigzag = alternate_horizontal_scan;
	else  // acpred_direction == 2
		zigzag = alternate_vertical_scan;

	run = 0;

    for (pos = 1; pos < 64; pos++)
    {
		int index = zigzag[pos];
		int value = dct_codes[index];

		if (value == 0)
		{
		    run++;
		    continue;
		}

		if (prev_level)
		    PutCoeff(bs, prev_run, prev_level, 0, MODE_INTRA);

		prev_run = run;
		prev_level = value;

		run = 0;
    }

    PutCoeff(bs, prev_run, prev_level, 1, MODE_INTRA);

    bits = BsPos(bs) - bits;
    pStat->iTextBits += bits;
}



static __inline void putMVData(Bitstream * bs, Statistics * pStat,
			       int fcode, int value)

{
    int scale_fac = 1 << (fcode - 1);
    int high = (32 * scale_fac) - 1;
    int low = ((-32) * scale_fac);
    int range = (64 * scale_fac);


    if (value < low)
		value += range;

    if (value > high)
		value -= range;


    pStat->iMvSum += value * value;
    pStat->iMvCount++;


    if ((scale_fac == 1) || (value == 0))
    {
		if (value < 0)
		    value = value + 65;

		PutMV(bs, value);
    }
    else
    {
		int mv_res, mv_data;
		int mv_sign;

		if (value < 0)
		{
		    value = -value;
		    mv_sign = -1;
		}
		else
		{
		    mv_sign = 1;
		}

		mv_res = (value - 1) % scale_fac;
		mv_data = (value - 1 - mv_res) / scale_fac + 1;

		if (mv_sign == -1)
		    mv_data = -mv_data + 65;

		PutMV(bs, mv_data);
		BitstreamPutBits(bs, mv_res, fcode - 1);
    }
}



void MBCoding(const MBParam * pParam, const MACROBLOCK *pMB,
	      int16_t qcoeff[][64], Bitstream * bs, Statistics * pStat)
{
    int i;
    VOP_TYPE vop_type = pParam->coding_type;
    bool bIntra = (pMB->mode == MODE_INTRA) || (pMB->mode == MODE_INTRA_Q);

    if (vop_type == P_VOP)
    {
		if (((pMB->mode == MODE_INTER))
		    && (pMB->cbp == 0)
		    && (pMB->mvs[0].x == 0) && (pMB->mvs[0].y == 0))
		{					 // skipped
		    BitstreamPutBit(bs, 1);
		    return;
		}
		else
		{
		    BitstreamPutBit(bs, 0);
		}
    }

    if (vop_type == I_VOP)
		PutMCBPC_intra(bs, pMB->cbp & 3, pMB->mode);
    else
    {
		PutMCBPC_inter(bs, pMB->cbp & 3, pMB->mode);
    }


    if (bIntra)
    {
		if (pMB->acpred_directions[0])
		    BitstreamPutBits(bs, 1, 1);
		else
		    BitstreamPutBits(bs, 0, 1);
    }

    
	PutCBPY(bs, pMB->cbp >> 2, bIntra ? 1 : 0);

    if ((pMB->mode == MODE_INTER_Q) || (pMB->mode == MODE_INTRA_Q))
		BitstreamPutBits(bs, pMB->dquant, 2);

    
	if (!bIntra)
	{
		for (i = 0; i < ((pMB->mode == MODE_INTER4V) ? 4 : 1); i++)
		{
		    putMVData(bs, pStat, pParam->fixed_code, pMB->pmvs[i].x);
		    putMVData(bs, pStat, pParam->fixed_code, pMB->pmvs[i].y);
		}
	}

    for (i = 0; i < 6; i++)
    {
		if (bIntra)
		{
		    PutIntraDC(bs, qcoeff[i][0], (i < 4) ? 1 : 0);
		    if (pMB->cbp & (1 << (5 - i)))
				EncodeBlockIntra(bs, pStat, qcoeff[i], pMB->acpred_directions[i]);
		}
		else if (pMB->cbp & (1 << (5 - i)))
		    EncodeBlockInter(bs, pStat, qcoeff[i]);
    }
}

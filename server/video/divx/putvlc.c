
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
 *  putvlc.c
 *
 *  Copyright (C) 2001  Project Mayo
 *
 *  Adam Li
 *
 *  DivX Advance Research Center <darc@projectmayo.com>
 *
 **************************************************************************/

/* This file contains some functions to code the RVLC code for bitstream. */
/* Some codes of this project come from MoMuSys MPEG-4 implementation.    */
/* Please see seperate acknowledgement file for a list of contributors.   */

#include "momusys.h"
#include "vlc.h"
#include "putvlc.h"
#include "bitstream.h"

/***********************************************************CommentBegin******
 *
 * -- Putxxx -- Write bits from huffman tables to file
 *
 *	Int PutCoeff_Inter(Int run, Int level, Int last, Image *bitstream)
 *	Int PutCoeff_Intra(Int run, Int level, Int last, Image *bitstream)
 *	Int PutCBPY (Int cbpy, Int *MB_transp_pattren,Image *bitstream)
 *	Int PutMCBPC_Inter (Int cbpc, Int mode, Image *bitstream)
 *	Int PutMCBPC_Sprite (Int cbpc, Int mode, Image *bitstream)
 *	Int PutMCBPC_Intra (Int cbpc, Int mode, Image *bitstream)
 *	Int PutMV (Int mvint, Image *bitstream)
 *	Int PutDCsize_chrom (Int size, Image *bitstream)
 *	Int PutDCsize_lum (Int size, Image *bitstream)
 *	Int PutDCsize_lum (Int size, Image *bitstream)
 *      Int PutCoeff_Inter_RVLC (Int run, Int level, Int last, Image *bitstream)
 *      Int PutCoeff_Intra_RVLC (Int run, Int level, Int last, Image *bitstream)
 *      Int PutRunCoeff_Inter (Int run, Int level, Int last, Image *bitstream)
 *      Int PutRunCoeff_Intra (Int run, Int level, Int last, Image *bitstream)
 *      Int PutLevelCoeff_Inter (Int run, Int level, Int last, Image *bitstream)
 *      Int PutLevelCoeff_Intra (Int run, Int level, Int last, Image *bitstream)
 *
 * Purpose :
 *	Writes bits from huffman tables to bitstream
 *
 * Arguments in :
 *	various, see prototypes above
 *
 * Return values :
 *	Number of bits written
 *
 ***********************************************************CommentEnd********/

Int
PutDCsize_lum (Int size, Image *bitstream)
{
	MOMCHECK(size >= 0 && size < 13);

	BitstreamPutBits (bitstream, DCtab_lum[size].code, DCtab_lum[size].len);

	return DCtab_lum[size].len;
}


Int
PutDCsize_chrom (Int size, Image *bitstream)
{
	MOMCHECK (size >= 0 && size < 13);

	BitstreamPutBits (bitstream, DCtab_chrom[size].code, DCtab_chrom[size].len);

	return DCtab_chrom[size].len;
}


Int
PutMV (Int mvint, Image *bitstream)
{
	Int sign = 0;
	Int absmv;

	if (mvint > 32)
	{
		absmv = -mvint + 65;
		sign = 1;
	}
	else
		absmv = mvint;

	BitstreamPutBits (bitstream, mvtab[absmv].code, mvtab[absmv].len);

	if (mvint != 0)
	{
		BitstreamPutBits (bitstream, sign, 1);
		return mvtab[absmv].len + 1;
	}
	else
		return mvtab[absmv].len;
}


Int
PutMCBPC_Intra (Int cbpc, Int mode, Image *bitstream)
{
	Int ind;

	ind = ((mode >> 1) & 3) | ((cbpc & 3) << 2);

	BitstreamPutBits (bitstream, mcbpc_intra_tab[ind].code, mcbpc_intra_tab[ind].len);

	return mcbpc_intra_tab[ind].len;
}


Int
PutMCBPC_Inter (Int cbpc, Int mode, Image *bitstream)
{
	Int ind;

	ind = (mode & 7) | ((cbpc & 3) << 3);

	BitstreamPutBits (bitstream, mcbpc_inter_tab[ind].code,
		mcbpc_inter_tab[ind].len);

	return mcbpc_inter_tab[ind].len;
}


Int
PutMCBPC_Sprite (Int cbpc, Int mode, Image *bitstream)
{
	Int ind;

	ind = (mode & 7) | ((cbpc & 3) << 3);

	BitstreamPutBits (bitstream, mcbpc_sprite_tab[ind].code,
		mcbpc_sprite_tab[ind].len);

	return mcbpc_sprite_tab[ind].len;
}


Int
PutCBPY (Int cbpy, Char intra, Int *MB_transp_pattern, Image *bitstream)
{
	Int ind;//,i,ptr;
	Int index=0;

	/* Changed due to bug report from Yoshinori Suzuki; MW 11-JUN-1998 */
	/* if ((intra==0)&&(index!=3)) cbpy = 15 - cbpy;  */
	if (intra == 0) cbpy = 15 - cbpy;

	ind = cbpy;
	BitstreamPutBits (bitstream, cbpy_tab[ind].code, cbpy_tab[ind].len);
	return cbpy_tab[ind].len;
}


Int
PutCoeff_Inter(Int run, Int level, Int last, Image *bitstream)
{
	Int length = 0;

	MOMCHECK (last >= 0 && last < 2);
	MOMCHECK (run >= 0 && run < 64);
	MOMCHECK (level > 0 && level < 128);

	if (last == 0)
	{
		if (run < 2 && level < 13 )
		{
			BitstreamPutBits (bitstream, (LInt)coeff_tab0[run][level-1].code,
				(LInt)coeff_tab0[run][level-1].len);

			length = coeff_tab0[run][level-1].len;
		}
		else if (run > 1 && run < 27 && level < 5)
		{
			BitstreamPutBits (bitstream, (LInt)coeff_tab1[run-2][level-1].code,
				(LInt)coeff_tab1[run-2][level-1].len);

			length = coeff_tab1[run-2][level-1].len;
		}
	}
	else if (last == 1)
	{
		if (run < 2 && level < 4)
		{
			BitstreamPutBits (bitstream, (LInt)coeff_tab2[run][level-1].code,
				(LInt)coeff_tab2[run][level-1].len);

			length = coeff_tab2[run][level-1].len;
		}
		else if (run > 1 && run < 42 && level == 1)
		{
			BitstreamPutBits (bitstream, (LInt)coeff_tab3[run-2].code,
				(LInt)coeff_tab3[run-2].len);

			length = coeff_tab3[run-2].len;
		}
	}
	return length;
}


Int
PutCoeff_Intra(Int run, Int level, Int last, Image *bitstream)
{
	Int length = 0;

	MOMCHECK (last >= 0 && last < 2);
	MOMCHECK (run >= 0 && run < 64);
	MOMCHECK (level > 0 && level < 128);

	if (last == 0)
	{
		if (run == 0 && level < 28 )
		{
			BitstreamPutBits(bitstream, (LInt)coeff_tab4[level-1].code,
				(LInt)coeff_tab4[level-1].len);

			length = coeff_tab4[level-1].len;
		}
		else if (run == 1 && level < 11)
		{
			BitstreamPutBits(bitstream, (LInt)coeff_tab5[level-1].code,
				(LInt)coeff_tab5[level-1].len);

			length = coeff_tab5[level-1].len;
		}
		else if (run > 1 && run < 10 && level < 6)
		{
			BitstreamPutBits(bitstream, (LInt)coeff_tab6[run-2][level-1].code,
				(LInt)coeff_tab6[run-2][level-1].len);

			length = coeff_tab6[run-2][level-1].len;
		}
		else if (run > 9 && run < 15 && level == 1)
		{
			BitstreamPutBits(bitstream, (LInt)coeff_tab7[run-10].code,
				(LInt)coeff_tab7[run-10].len);

			length = coeff_tab7[run-10].len;
		}
	}
	else if (last == 1)
	{
		if (run == 0 && level < 9)
		{
			BitstreamPutBits(bitstream, (LInt)coeff_tab8[level-1].code,
				(LInt)coeff_tab8[level-1].len);

			length = coeff_tab8[level-1].len;
		}
		else if (run > 0 && run < 7 && level < 4)
		{
			BitstreamPutBits(bitstream, (LInt)coeff_tab9[run-1][level-1].code,
				(LInt)coeff_tab9[run-1][level-1].len);

			length = coeff_tab9[run-1][level-1].len;
		}
		else if (run > 6 && run < 21 && level == 1)
		{
			BitstreamPutBits(bitstream, (LInt)coeff_tab10[run-7].code,
				(LInt)coeff_tab10[run-7].len);

			length = coeff_tab10[run-7].len;
		}
	}
	return length;
}


Int
PutCoeff_Inter_RVLC(Int run, Int level, Int last, Image *bitstream)
{
	Int length = 0;

	MOMCHECK (last >= 0 && last < 2);
	MOMCHECK (run >= 0 && run < 64);
	MOMCHECK (level > 0 && level < 128);

	if (last == 0)
	{
		if (run == 0 && level < 20 )
		{
			BitstreamPutBits(bitstream, (LInt)coeff_RVLCtab14[level-1].code,
				(LInt)coeff_RVLCtab14[level-1].len);

			length = coeff_RVLCtab14[level-1].len;
		}
		else if (run == 1 && level < 11)
		{
			BitstreamPutBits(bitstream, (LInt)coeff_RVLCtab15[level-1].code,
				(LInt)coeff_RVLCtab15[level-1].len);

			length = coeff_RVLCtab15[level-1].len;
		}
		else if (run > 1 && run < 4 && level < 8)
		{
			BitstreamPutBits(bitstream, (LInt)coeff_RVLCtab16[run-2][level-1].code,
				(LInt)coeff_RVLCtab16[run-2][level-1].len);
			length = coeff_RVLCtab16[run-2][level-1].len;
		}
		else if (run == 4 && level < 6)
		{
			BitstreamPutBits(bitstream, (LInt)coeff_RVLCtab17[level-1].code,
				(LInt)coeff_RVLCtab17[level-1].len);

			length = coeff_RVLCtab17[level-1].len;
		}
		else if (run > 4 && run < 8 && level < 5)
		{
			BitstreamPutBits(bitstream, (LInt)coeff_RVLCtab18[run-5][level-1].code,
				(LInt)coeff_RVLCtab18[run-5][level-1].len);

			length = coeff_RVLCtab18[run-5][level-1].len;
		}
		else if (run > 7 && run < 10 && level < 4)
		{
			BitstreamPutBits(bitstream, (LInt)coeff_RVLCtab19[run-8][level-1].code,
				(LInt)coeff_RVLCtab19[run-8][level-1].len);

			length = coeff_RVLCtab19[run-8][level-1].len;
		}
		else if (run > 9 && run < 18 && level < 3)
		{
			BitstreamPutBits(bitstream, (LInt)coeff_RVLCtab20[run-10][level-1].code,
				(LInt)coeff_RVLCtab20[run-10][level-1].len);

			length = coeff_RVLCtab20[run-10][level-1].len;
		}
		else if (run > 17 && run < 39 && level == 1)
		{
			BitstreamPutBits(bitstream, (LInt)coeff_RVLCtab21[run-18].code,
				(LInt)coeff_RVLCtab21[run-18].len);

			length = coeff_RVLCtab21[run-18].len;
		}
	}
	else if (last == 1)
	{
		if (run >= 0 && run < 2 && level < 6)
		{
			BitstreamPutBits(bitstream, (LInt)coeff_RVLCtab22[run][level-1].code,
				(LInt)coeff_RVLCtab22[run][level-1].len);

			length = coeff_RVLCtab22[run][level-1].len;
		}
		else if (run == 2 && level < 4)
		{
			BitstreamPutBits (bitstream, (LInt)coeff_RVLCtab23[level-1].code,
				(LInt)coeff_RVLCtab23[level-1].len);

			length = coeff_RVLCtab23[level-1].len;
		}
		else if (run > 2 && run < 14 && level < 3)
		{
			BitstreamPutBits(bitstream, (LInt)coeff_RVLCtab24[run-3][level-1].code,
				(LInt)coeff_RVLCtab24[run-3][level-1].len);

			length = coeff_RVLCtab24[run-3][level-1].len;
		}
		else if (run > 13 && run < 46 && level == 1)
		{
			BitstreamPutBits(bitstream, (LInt)coeff_RVLCtab25[run-14].code,
				(LInt)coeff_RVLCtab25[run-14].len);

			length = coeff_RVLCtab25[run-14].len;
		}
	}
	return length;
}


Int
PutCoeff_Intra_RVLC(Int run, Int level, Int last, Image *bitstream)
{
	Int length = 0;

	MOMCHECK (last >= 0 && last < 2);
	MOMCHECK (run >= 0 && run < 64);
	MOMCHECK (level > 0 && level < 128);

	if (last == 0)
	{
		if (run == 0 && level < 28 )
		{
			BitstreamPutBits(bitstream, (LInt)coeff_RVLCtab1[level-1].code,
				(LInt)coeff_RVLCtab1[level-1].len);

			length = coeff_RVLCtab1[level-1].len;
		}
		else if (run == 1 && level < 14)
		{
			BitstreamPutBits(bitstream, (LInt)coeff_RVLCtab2[level-1].code,
				(LInt)coeff_RVLCtab2[level-1].len);

			length = coeff_RVLCtab2[level-1].len;
		}
		else if (run == 2 && level < 12)
		{
			BitstreamPutBits(bitstream, (LInt)coeff_RVLCtab3[level-1].code,
				(LInt)coeff_RVLCtab3[level-1].len);

			length = coeff_RVLCtab3[level-1].len;
		}
		else if (run == 3 && level < 10)
		{
			BitstreamPutBits(bitstream, (LInt)coeff_RVLCtab4[level-1].code,
				(LInt)coeff_RVLCtab4[level-1].len);

			length = coeff_RVLCtab4[level-1].len;
		}
		else if (run > 3 && run < 6 && level < 7)
		{
			BitstreamPutBits(bitstream, (LInt)coeff_RVLCtab5[run-4][level-1].code,
				(LInt)coeff_RVLCtab5[run-4][level-1].len);

			length = coeff_RVLCtab5[run-4][level-1].len;
		}
		else if (run > 5 && run < 8 && level < 6)
		{
			BitstreamPutBits(bitstream, (LInt)coeff_RVLCtab6[run-6][level-1].code,
				(LInt)coeff_RVLCtab6[run-6][level-1].len);

			length = coeff_RVLCtab6[run-6][level-1].len;
		}
		else if (run > 7 && run < 10 && level < 5)
		{
			BitstreamPutBits(bitstream, (LInt)coeff_RVLCtab7[run-8][level-1].code,
				(LInt)coeff_RVLCtab7[run-8][level-1].len);

			length = coeff_RVLCtab7[run-8][level-1].len;
		}
		else if (run > 9 && run < 13 && level < 3)
		{
			BitstreamPutBits(bitstream, (LInt)coeff_RVLCtab8[run-10][level-1].code,
				(LInt)coeff_RVLCtab8[run-10][level-1].len);

			length = coeff_RVLCtab8[run-10][level-1].len;
		}
		else if (run > 12 && run < 20 && level == 1)
		{
			BitstreamPutBits(bitstream, (LInt)coeff_RVLCtab9[run-13].code,
				(LInt)coeff_RVLCtab9[run-13].len);

			length = coeff_RVLCtab9[run-13].len;
		}
	}
	else if (last == 1)
	{
		if (run >= 0 && run < 2 && level < 6)
		{
			BitstreamPutBits(bitstream, (LInt)coeff_RVLCtab10[run][level-1].code,
				(LInt)coeff_RVLCtab10[run][level-1].len);

			length = coeff_RVLCtab10[run][level-1].len;
		}
		else if (run == 2 && level < 4)
		{
			BitstreamPutBits(bitstream, (LInt)coeff_RVLCtab11[level-1].code,
				(LInt)coeff_RVLCtab11[level-1].len);

			length = coeff_RVLCtab11[level-1].len;
		}
		else if (run > 2 && run < 14 && level < 3)
		{
			BitstreamPutBits(bitstream, (LInt)coeff_RVLCtab12[run-3][level-1].code,
				(LInt)coeff_RVLCtab12[run-3][level-1].len);

			length = coeff_RVLCtab12[run-3][level-1].len;
		}
		else if (run > 13 && run < 46 && level == 1)
		{
			BitstreamPutBits(bitstream, (LInt)coeff_RVLCtab13[run-14].code,
				(LInt)coeff_RVLCtab13[run-14].len);

			length = coeff_RVLCtab13[run-14].len;
		}
	}
	return length;
}


/* The following is for 3-mode VLC */

Int
PutRunCoeff_Inter(Int run, Int level, Int last, Image *bitstream)
{
	Int length = 0;

	MOMCHECK (last >= 0 && last < 2);
	MOMCHECK (run >= 0 && run < 64);
	MOMCHECK (level > 0 && level < 128);

	if (last == 0)
	{
		if (run < 2 && level < 13 )
		{
			length = coeff_tab0[run][level-1].len;
			if (length != 0)
			{
												  /* boon 120697 */
				BitstreamPutBits(bitstream, 3L, 7L);
												  /* boon 120697 */
				BitstreamPutBits(bitstream, 2L, 2L);
				length += 9;					  /* boon */
				BitstreamPutBits (bitstream, (LInt)coeff_tab0[run][level-1].code,
					(LInt)coeff_tab0[run][level-1].len);
			}
		}
		else if (run > 1 && run < 27 && level < 5)
		{
			length = coeff_tab1[run-2][level-1].len;
			if (length != 0)
			{
												  /* boon 120697 */
				BitstreamPutBits(bitstream, 3L, 7L);
												  /* boon 120697 */
				BitstreamPutBits(bitstream, 2L, 2L);
				length += 9;					  /* boon */
				BitstreamPutBits (bitstream, (LInt)coeff_tab1[run-2][level-1].code,
					(LInt)coeff_tab1[run-2][level-1].len);
			}
		}
	}
	else if (last == 1)
	{
		if (run < 2 && level < 4)
		{
			length = coeff_tab2[run][level-1].len;
			if (length != 0)
			{
												  /* boon 120697 */
				BitstreamPutBits(bitstream, 3L, 7L);
												  /* boon 120697 */
				BitstreamPutBits(bitstream, 2L, 2L);
				length += 9;					  /* boon */
				BitstreamPutBits (bitstream, (LInt)coeff_tab2[run][level-1].code,
					(LInt)coeff_tab2[run][level-1].len);
			}
		}
		else if (run > 1 && run < 42 && level == 1)
		{
			length = coeff_tab3[run-2].len;
			if (length != 0)
			{
												  /* boon 120697 */
				BitstreamPutBits(bitstream, 3L, 7L);
												  /* boon 120697 */
				BitstreamPutBits(bitstream, 2L, 2L);
				length += 9;					  /* boon */
				BitstreamPutBits (bitstream, (LInt)coeff_tab3[run-2].code,
					(LInt)coeff_tab3[run-2].len);
			}
		}
	}
	return length;
}


Int
PutRunCoeff_Intra(Int run, Int level, Int last, Image *bitstream)
{
	Int length = 0;

	MOMCHECK (last >= 0 && last < 2);
	MOMCHECK (run >= 0 && run < 64);
	MOMCHECK (level > 0 && level < 128);

	if (last == 0)
	{
		if (run == 0 && level < 28 )
		{
			length = coeff_tab4[level-1].len;
			if (length != 0)
			{
												  /* boon 120697 */
				BitstreamPutBits(bitstream, 3L, 7L);
												  /* boon 120697 */
				BitstreamPutBits(bitstream, 2L, 2L);
				length += 9;					  /* boon */
				BitstreamPutBits(bitstream, (LInt)coeff_tab4[level-1].code,
					(LInt)coeff_tab4[level-1].len);
			}
		}
		else if (run == 1 && level < 11)
		{
			length = coeff_tab5[level-1].len;
			if (length != 0)
			{
												  /* boon 120697 */
				BitstreamPutBits(bitstream, 3L, 7L);
												  /* boon 120697 */
				BitstreamPutBits(bitstream, 2L, 2L);
				length += 9;					  /* boon */
				BitstreamPutBits(bitstream, (LInt)coeff_tab5[level-1].code,
					(LInt)coeff_tab5[level-1].len);
			}
		}
		else if (run > 1 && run < 10 && level < 6)
		{
			length = coeff_tab6[run-2][level-1].len;
			if (length != 0)
			{
												  /* boon 120697 */
				BitstreamPutBits(bitstream, 3L, 7L);
												  /* boon 120697 */
				BitstreamPutBits(bitstream, 2L, 2L);
				length += 9;					  /* boon */
				BitstreamPutBits(bitstream, (LInt)coeff_tab6[run-2][level-1].code,
					(LInt)coeff_tab6[run-2][level-1].len);
			}
		}
		else if (run > 9 && run < 15 && level == 1)
		{
			length = coeff_tab7[run-10].len;
			if (length != 0)
			{
												  /* boon 120697 */
				BitstreamPutBits(bitstream, 3L, 7L);
												  /* boon 120697 */
				BitstreamPutBits(bitstream, 2L, 2L);
				length += 9;					  /* boon */
				BitstreamPutBits(bitstream, (LInt)coeff_tab7[run-10].code,
					(LInt)coeff_tab7[run-10].len);
			}
		}
	}
	else if (last == 1)
	{
		if (run == 0 && level < 9)
		{
			length = coeff_tab8[level-1].len;
			if (length != 0)
			{
												  /* boon 120697 */
				BitstreamPutBits(bitstream, 3L, 7L);
												  /* boon 120697 */
				BitstreamPutBits(bitstream, 2L, 2L);
				length += 9;					  /* boon */
				BitstreamPutBits(bitstream, (LInt)coeff_tab8[level-1].code,
					(LInt)coeff_tab8[level-1].len);
			}
		}
		else if (run > 0 && run < 7 && level < 4)
		{
			length = coeff_tab9[run-1][level-1].len;
			if (length != 0)
			{
												  /* boon 120697 */
				BitstreamPutBits(bitstream, 3L, 7L);
												  /* boon 120697 */
				BitstreamPutBits(bitstream, 2L, 2L);
				length += 9;					  /* boon */
				BitstreamPutBits(bitstream, (LInt)coeff_tab9[run-1][level-1].code,
					(LInt)coeff_tab9[run-1][level-1].len);
			}
		}
		else if (run > 6 && run < 21 && level == 1)
		{
			length = coeff_tab10[run-7].len;
			if (length != 0)
			{
												  /* boon 120697 */
				BitstreamPutBits(bitstream, 3L, 7L);
												  /* boon 120697 */
				BitstreamPutBits(bitstream, 2L, 2L);
				length += 9;					  /* boon */
				BitstreamPutBits(bitstream, (LInt)coeff_tab10[run-7].code,
					(LInt)coeff_tab10[run-7].len);
			}
		}
	}
	return length;
}


Int
PutLevelCoeff_Inter(Int run, Int level, Int last, Image *bitstream)
{
	Int length = 0;

	MOMCHECK (last >= 0 && last < 2);
	MOMCHECK (run >= 0 && run < 64);
	MOMCHECK (level > 0 && level < 128);

	if (last == 0)
	{
		if (run < 2 && level < 13 )
		{
			length = coeff_tab0[run][level-1].len;
			if (length != 0)
			{
				BitstreamPutBits(bitstream, 3L, 7L);
												  /* boon19970701 */
				BitstreamPutBits(bitstream, 0L, 1L);
				length += 8;					  /* boon */
				BitstreamPutBits (bitstream, (LInt)coeff_tab0[run][level-1].code,
					(LInt)coeff_tab0[run][level-1].len);
			}
		}
		else if (run > 1 && run < 27 && level < 5)
		{
			length = coeff_tab1[run-2][level-1].len;
			if (length != 0)
			{
				BitstreamPutBits(bitstream, 3L, 7L);
												  /* boon19970701 */
				BitstreamPutBits(bitstream, 0L, 1L);
				length += 8;					  /* boon */
				BitstreamPutBits (bitstream, (LInt)coeff_tab1[run-2][level-1].code,
					(LInt)coeff_tab1[run-2][level-1].len);
			}
		}
	}
	else if (last == 1)
	{
		if (run < 2 && level < 4)
		{
			length = coeff_tab2[run][level-1].len;
			if (length != 0)
			{
				BitstreamPutBits(bitstream, 3L, 7L);
												  /* boon19970701 */
				BitstreamPutBits(bitstream, 0L, 1L);
				length += 8;					  /* boon */
				BitstreamPutBits (bitstream, (LInt)coeff_tab2[run][level-1].code,
					(LInt)coeff_tab2[run][level-1].len);
			}
		}
		else if (run > 1 && run < 42 && level == 1)
		{
			length = coeff_tab3[run-2].len;
			if (length != 0)
			{
				BitstreamPutBits(bitstream, 3L, 7L);
												  /* boon19970701 */
				BitstreamPutBits(bitstream, 0L, 1L);
				length += 8;					  /* boon */
				BitstreamPutBits (bitstream, (LInt)coeff_tab3[run-2].code,
					(LInt)coeff_tab3[run-2].len);
			}
		}
	}
	return length;
}


Int
PutLevelCoeff_Intra(Int run, Int level, Int last, Image *bitstream)
{
	Int length = 0;

	MOMCHECK (last >= 0 && last < 2);
	MOMCHECK (run >= 0 && run < 64);
	MOMCHECK (level > 0 && level < 128);

	if (last == 0)
	{
		if (run == 0 && level < 28 )
		{
			length = coeff_tab4[level-1].len;
			if (length != 0)
			{
				BitstreamPutBits(bitstream, 3L, 7L);
												  /* boon19970701 */
				BitstreamPutBits(bitstream, 0L, 1L);
				length += 8;					  /* boon */
				BitstreamPutBits(bitstream, (LInt)coeff_tab4[level-1].code,
					(LInt)coeff_tab4[level-1].len);
			}
		}
		else if (run == 1 && level < 11)
		{
			length = coeff_tab5[level-1].len;
			if (length != 0)
			{
				BitstreamPutBits(bitstream, 3L, 7L);
												  /* boon19970701 */
				BitstreamPutBits(bitstream, 0L, 1L);
				length += 8;					  /* boon */
				BitstreamPutBits(bitstream, (LInt)coeff_tab5[level-1].code,
					(LInt)coeff_tab5[level-1].len);
			}
		}
		else if (run > 1 && run < 10 && level < 6)
		{
			length = coeff_tab6[run-2][level-1].len;
			if (length != 0)
			{
				BitstreamPutBits(bitstream, 3L, 7L);
												  /* boon19970701 */
				BitstreamPutBits(bitstream, 0L, 1L);
				length += 8;					  /* boon */
				BitstreamPutBits(bitstream, (LInt)coeff_tab6[run-2][level-1].code,
					(LInt)coeff_tab6[run-2][level-1].len);
			}
		}
		else if (run > 9 && run < 15 && level == 1)
		{
			length = coeff_tab7[run-10].len;
			if (length != 0)
			{
				BitstreamPutBits(bitstream, 3L, 7L);
												  /* boon19970701 */
				BitstreamPutBits(bitstream, 0L, 1L);
				length += 8;					  /* boon */
				BitstreamPutBits(bitstream, (LInt)coeff_tab7[run-10].code,
					(LInt)coeff_tab7[run-10].len);
			}
		}
	}
	else if (last == 1)
	{
		if (run == 0 && level < 9)
		{
			length = coeff_tab8[level-1].len;
			if (length != 0)
			{
				BitstreamPutBits(bitstream, 3L, 7L);
												  /* boon19970701 */
				BitstreamPutBits(bitstream, 0L, 1L);
				length += 8;					  /* boon */
				BitstreamPutBits(bitstream, (LInt)coeff_tab8[level-1].code,
					(LInt)coeff_tab8[level-1].len);
			}
		}
		else if (run > 0 && run < 7 && level < 4)
		{
			length = coeff_tab9[run-1][level-1].len;
			if (length != 0)
			{
				BitstreamPutBits(bitstream, 3L, 7L);
												  /* boon19970701 */
				BitstreamPutBits(bitstream, 0L, 1L);
				length += 8;					  /* boon */
				BitstreamPutBits(bitstream, (LInt)coeff_tab9[run-1][level-1].code,
					(LInt)coeff_tab9[run-1][level-1].len);
			}
		}
		else if (run > 6 && run < 21 && level == 1)
		{
			length = coeff_tab10[run-7].len;
			if (length != 0)
			{
				BitstreamPutBits(bitstream, 3L, 7L);
												  /* boon19970701 */
				BitstreamPutBits(bitstream, 0L, 1L);
				length += 8;					  /* boon */
				BitstreamPutBits(bitstream, (LInt)coeff_tab10[run-7].code,
					(LInt)coeff_tab10[run-7].len);
			}
		}
	}
	return length;
}

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

 /******************************************************************************
  *                                                                            *
  *  Revision history:                                                         *
  *       
  *  21.12.2001 cleanup; commented unused rvlc/sprite functions
  *                                                                            *
  ******************************************************************************/


#include <assert.h>

#include "vlc_codes.h"
#include "putvlc.h"



/***********************************************************CommentBegin******
 *
 * -- Putxxx -- Write bits from huffman tables to file
 *
 * Purpose :
 *	Writes bits from huffman tables to bitstream
 *
 * Arguments in :
 *	various
 *
 * Return values :
 *	Number of bits written
 *
 ***********************************************************CommentEnd********/


int PutDCsize_lum(Bitstream * bitstream, int size)
{

    assert(size >= 0 && size < 13);

    BitstreamPutBits(bitstream, DCtab_lum[size].code, DCtab_lum[size].len);
    return DCtab_lum[size].len;
}





int PutDCsize_chrom(Bitstream * bitstream, int size)
{
    assert(size >= 0 && size < 13);

    BitstreamPutBits(bitstream, DCtab_chrom[size].code, DCtab_chrom[size].len);
    return DCtab_chrom[size].len;
}





int PutMV(Bitstream * bitstream, int mvint)
{
	int sign = 0;
    int absmv;

    if (mvint > 32)
	{
		absmv = -mvint + 65;
		sign = 1;
    }
    else
		absmv = mvint;

    BitstreamPutBits(bitstream, mvtab[absmv].code, mvtab[absmv].len);

    if (mvint != 0)
    {
		BitstreamPutBits(bitstream, sign, 1);
		return mvtab[absmv].len + 1;
    }
	else
		return mvtab[absmv].len;
}



int PutMCBPC_intra(Bitstream * bitstream, int cbpc, int mode)
{
    int ind;
    ind = ((mode >> 1) & 3) | ((cbpc & 3) << 2);

    BitstreamPutBits(bitstream, mcbpc_intra_tab[ind].code,
		     mcbpc_intra_tab[ind].len);

    return mcbpc_intra_tab[ind].len;
}


int PutMCBPC_inter(Bitstream * bitstream, int cbpc, int mode)
{
    int ind;

    ind = (mode & 7) | ((cbpc & 3) << 3);

    BitstreamPutBits(bitstream, mcbpc_inter_tab[ind].code,
		     mcbpc_inter_tab[ind].len);
    return mcbpc_inter_tab[ind].len;
}



/* 
int PutMCBPC_Sprite(Bitstream * bitstream, int cbpc, int mode)
{
    int ind;

    ind = (mode & 7) | ((cbpc & 3) << 3);

    BitstreamPutBits(bitstream, mcbpc_sprite_tab[ind].code,
		     mcbpc_sprite_tab[ind].len);

    return mcbpc_sprite_tab[ind].len;
}
*/



int PutCBPY(Bitstream * bitstream, int cbpy, bool intra)
{
    if (intra == 0)
		cbpy = 15 - cbpy;

    BitstreamPutBits(bitstream, cbpy_tab[cbpy].code, cbpy_tab[cbpy].len);
    return cbpy_tab[cbpy].len;
}




int PutCoeff_inter(Bitstream * bitstream, int run, int level, int last)
{
    int length = 0;

	assert(run >= 0 && run < 64);
    assert(level > 0 && level < 128);

    if (!last)
    {
		if (run < 2 && level < 13)
		{
		    BitstreamPutBits(bitstream, coeff_tab0[run][level - 1].code,
			     coeff_tab0[run][level - 1].len);

		    length = coeff_tab0[run][level - 1].len;
		}
		else if (run > 1 && run < 27 && level < 5)
		{
		    BitstreamPutBits(bitstream, coeff_tab1[run - 2][level - 1].code,
			     coeff_tab1[run - 2][level - 1].len);

		    length = coeff_tab1[run - 2][level - 1].len;
		}
    }
    else // last == 1
	{
		if (run < 2 && level < 4)
		{
		    BitstreamPutBits(bitstream, coeff_tab2[run][level - 1].code,
			     coeff_tab2[run][level - 1].len);
		    length = coeff_tab2[run][level - 1].len;
		}
		else if (run > 1 && run < 42 && level == 1)
		{
		    BitstreamPutBits(bitstream, coeff_tab3[run - 2].code,
			     coeff_tab3[run - 2].len);
		    length = coeff_tab3[run - 2].len;
		}
    }

    return length;
}





int PutCoeff_intra(Bitstream * bitstream, int run, int level, int last)
{
	int length = 0;


    assert(run >= 0 && run < 64);
    assert(level > 0 && level < 128);

    if (!last)
    {
		if (run == 0 && level < 28)
		{
		    BitstreamPutBits(bitstream, coeff_tab4[level - 1].code,
			     coeff_tab4[level - 1].len);

		    length = coeff_tab4[level - 1].len;
		}
		else if (run == 1 && level < 11)
		{
		    BitstreamPutBits(bitstream, coeff_tab5[level - 1].code,
			     coeff_tab5[level - 1].len);

		    length = coeff_tab5[level - 1].len;
		}
		else if (run > 1 && run < 10 && level < 6)
		{
		    BitstreamPutBits(bitstream, coeff_tab6[run - 2][level - 1].code,
			     coeff_tab6[run - 2][level - 1].len);
		    length = coeff_tab6[run - 2][level - 1].len;
		}
		else if (run > 9 && run < 15 && level == 1)
		{
		    BitstreamPutBits(bitstream, coeff_tab7[run - 10].code,
			     coeff_tab7[run - 10].len);

		    length = coeff_tab7[run - 10].len;
		}
    }
    else // last == 1
    {
		if (run == 0 && level < 9)
		{
		    BitstreamPutBits(bitstream, coeff_tab8[level - 1].code,
			     coeff_tab8[level - 1].len);

		    length = coeff_tab8[level - 1].len;
		}
		else if (run > 0 && run < 7 && level < 4)
		{
		    BitstreamPutBits(bitstream, coeff_tab9[run - 1][level - 1].code,
			     coeff_tab9[run - 1][level - 1].len);
		    length = coeff_tab9[run - 1][level - 1].len;
		}
		else if (run > 6 && run < 21 && level == 1)
		{
		    BitstreamPutBits(bitstream, coeff_tab10[run - 7].code,
			     coeff_tab10[run - 7].len);
		    length = coeff_tab10[run - 7].len;
		}
    }

    return length;
}





/* int PutCoeff_inter_RVLC(Bitstream * bitstream, int run, int level, int last)
{
    int length = 0;

    assert(last >= 0 && last < 2);
    assert(run >= 0 && run < 64);
    assert(level > 0 && level < 128);

    if (last == 0)
    {
	if (run == 0 && level < 20)
	{
	    BitstreamPutBits(bitstream, coeff_RVLCtab14[level - 1].code,
			     coeff_RVLCtab14[level - 1].len);

	    length = coeff_RVLCtab14[level - 1].len;
	}
	else if (run == 1 && level < 11)
	{
	    BitstreamPutBits(bitstream, coeff_RVLCtab15[level - 1].code,
			     coeff_RVLCtab15[level - 1].len);

	    length = coeff_RVLCtab15[level - 1].len;
	}
	else if (run > 1 && run < 4 && level < 8)
	{
	    BitstreamPutBits(bitstream,
			     coeff_RVLCtab16[run - 2][level - 1].code,
			     coeff_RVLCtab16[run - 2][level - 1].len);
	    length = coeff_RVLCtab16[run - 2][level - 1].len;
	}
	else if (run == 4 && level < 6)
	{
	    BitstreamPutBits(bitstream, coeff_RVLCtab17[level - 1].code,
			     coeff_RVLCtab17[level - 1].len);

	    length = coeff_RVLCtab17[level - 1].len;
	}
	else if (run > 4 && run < 8 && level < 5)
	{
	    BitstreamPutBits(bitstream,
			     coeff_RVLCtab18[run - 5][level - 1].code,
			     coeff_RVLCtab18[run - 5][level - 1].len);

	    length = coeff_RVLCtab18[run - 5][level - 1].len;
	}
	else if (run > 7 && run < 10 && level < 4)
	{
	    BitstreamPutBits(bitstream,
			     coeff_RVLCtab19[run - 8][level - 1].code,
			     coeff_RVLCtab19[run - 8][level - 1].len);

	    length = coeff_RVLCtab19[run - 8][level - 1].len;
	}
	else if (run > 9 && run < 18 && level < 3)
	{
	    BitstreamPutBits(bitstream,
			     coeff_RVLCtab20[run - 10][level - 1].code,
			     coeff_RVLCtab20[run - 10][level - 1].len);

	    length = coeff_RVLCtab20[run - 10][level - 1].len;
	}
	else if (run > 17 && run < 39 && level == 1)
	{
	    BitstreamPutBits(bitstream, coeff_RVLCtab21[run - 18].code,
			     coeff_RVLCtab21[run - 18].len);

	    length = coeff_RVLCtab21[run - 18].len;
	}
    }
    else if (last == 1)
    {
	if (run >= 0 && run < 2 && level < 6)
	{
	    BitstreamPutBits(bitstream, coeff_RVLCtab22[run][level - 1].code,
			     coeff_RVLCtab22[run][level - 1].len);

	    length = coeff_RVLCtab22[run][level - 1].len;
	}
	else if (run == 2 && level < 4)
	{
	    BitstreamPutBits(bitstream, coeff_RVLCtab23[level - 1].code,
			     coeff_RVLCtab23[level - 1].len);

	    length = coeff_RVLCtab23[level - 1].len;
	}
	else if (run > 2 && run < 14 && level < 3)
	{
	    BitstreamPutBits(bitstream,
			     coeff_RVLCtab24[run - 3][level - 1].code,
			     coeff_RVLCtab24[run - 3][level - 1].len);

	    length = coeff_RVLCtab24[run - 3][level - 1].len;
	}
	else if (run > 13 && run < 46 && level == 1)
	{
	    BitstreamPutBits(bitstream, coeff_RVLCtab25[run - 14].code,
			     coeff_RVLCtab25[run - 14].len);

	    length = coeff_RVLCtab25[run - 14].len;
	}
    }
    return length;
}


int PutCoeff_intra_RVLC(Bitstream * bitstream, int run, int level, int last)
{
    int length = 0;

    assert(last >= 0 && last < 2);
    assert(run >= 0 && run < 64);
    assert(level > 0 && level < 128);

    if (last == 0)
    {
	if (run == 0 && level < 28)
	{
	    BitstreamPutBits(bitstream, coeff_RVLCtab1[level - 1].code,
			     coeff_RVLCtab1[level - 1].len);

	    length = coeff_RVLCtab1[level - 1].len;
	}
	else if (run == 1 && level < 14)
	{
	    BitstreamPutBits(bitstream, coeff_RVLCtab2[level - 1].code,
			     coeff_RVLCtab2[level - 1].len);

	    length = coeff_RVLCtab2[level - 1].len;
	}
	else if (run == 2 && level < 12)
	{
	    BitstreamPutBits(bitstream, coeff_RVLCtab3[level - 1].code,
			     coeff_RVLCtab3[level - 1].len);

	    length = coeff_RVLCtab3[level - 1].len;
	}
	else if (run == 3 && level < 10)
	{
	    BitstreamPutBits(bitstream, coeff_RVLCtab4[level - 1].code,
			     coeff_RVLCtab4[level - 1].len);

	    length = coeff_RVLCtab4[level - 1].len;
	}
	else if (run > 3 && run < 6 && level < 7)
	{
	    BitstreamPutBits(bitstream,
			     coeff_RVLCtab5[run - 4][level - 1].code,
			     coeff_RVLCtab5[run - 4][level - 1].len);

	    length = coeff_RVLCtab5[run - 4][level - 1].len;
	}
	else if (run > 5 && run < 8 && level < 6)
	{
	    BitstreamPutBits(bitstream,
			     coeff_RVLCtab6[run - 6][level - 1].code,
			     coeff_RVLCtab6[run - 6][level - 1].len);

	    length = coeff_RVLCtab6[run - 6][level - 1].len;
	}
	else if (run > 7 && run < 10 && level < 5)
	{
	    BitstreamPutBits(bitstream,
			     coeff_RVLCtab7[run - 8][level - 1].code,
			     coeff_RVLCtab7[run - 8][level - 1].len);

	    length = coeff_RVLCtab7[run - 8][level - 1].len;
	}
	else if (run > 9 && run < 13 && level < 3)
	{
	    BitstreamPutBits(bitstream,
			     coeff_RVLCtab8[run - 10][level - 1].code,
			     coeff_RVLCtab8[run - 10][level - 1].len);

	    length = coeff_RVLCtab8[run - 10][level - 1].len;
	}
	else if (run > 12 && run < 20 && level == 1)
	{
	    BitstreamPutBits(bitstream, coeff_RVLCtab9[run - 13].code,
			     coeff_RVLCtab9[run - 13].len);

	    length = coeff_RVLCtab9[run - 13].len;
	}
    }
    else if (last == 1)
    {
	if (run >= 0 && run < 2 && level < 6)
	{
	    BitstreamPutBits(bitstream, coeff_RVLCtab10[run][level - 1].code,
			     coeff_RVLCtab10[run][level - 1].len);

	    length = coeff_RVLCtab10[run][level - 1].len;
	}
	else if (run == 2 && level < 4)
	{
	    BitstreamPutBits(bitstream, coeff_RVLCtab11[level - 1].code,
			     coeff_RVLCtab11[level - 1].len);

	    length = coeff_RVLCtab11[level - 1].len;
	}
	else if (run > 2 && run < 14 && level < 3)
	{
	    BitstreamPutBits(bitstream,
			     coeff_RVLCtab12[run - 3][level - 1].code,
			     coeff_RVLCtab12[run - 3][level - 1].len);

	    length = coeff_RVLCtab12[run - 3][level - 1].len;
	}
	else if (run > 13 && run < 46 && level == 1)
	{
	    BitstreamPutBits(bitstream, coeff_RVLCtab13[run - 14].code,
			     coeff_RVLCtab13[run - 14].len);

	    length = coeff_RVLCtab13[run - 14].len;
	}
    }
    return length;
}
*/





/* The following is for 3-mode VLC */



int PutRunCoeff_inter(Bitstream * bitstream, int run, int level, int last)
{
    int length = 0;

    assert(run >= 0 && run < 64);
    assert(level > 0 && level < 128);

    if (!last)
    {
		if (run < 2 && level < 13)
		{
		    length = coeff_tab0[run][level - 1].len;

		    if (length != 0)
		    {
				/*  boon 120697 */
				BitstreamPutBits(bitstream, 3L, 7L);
				/*  boon 120697 */

				BitstreamPutBits(bitstream, 2L, 2L);
				length += 9;			 /*  boon */

				BitstreamPutBits(bitstream, 
					coeff_tab0[run][level - 1].code,
					coeff_tab0[run][level - 1].len);
		    }
		}
		else if (run > 1 && run < 27 && level < 5)
		{
		    length = coeff_tab1[run - 2][level - 1].len;

		    if (length != 0)
		    {
				/* boon 120697 */
				BitstreamPutBits(bitstream, 3L, 7L);
				/* boon 120697 */
				BitstreamPutBits(bitstream, 2L, 2L);
				length += 9;			 /* boon */
				BitstreamPutBits(bitstream,
					 coeff_tab1[run - 2][level - 1].code,
					 coeff_tab1[run - 2][level - 1].len);
		    }
		}
    }
    else // last == 1
    {
		if (run < 2 && level < 4)
		{
		    length = coeff_tab2[run][level - 1].len;
		    if (length != 0)
		    {
				/* boon 120697 */
				BitstreamPutBits(bitstream, 3L, 7L);
				/* boon 120697 */
				BitstreamPutBits(bitstream, 2L, 2L);
				length += 9;			 /* boon */
				BitstreamPutBits(bitstream, 
					coeff_tab2[run][level - 1].code,
					coeff_tab2[run][level - 1].len);
		    }
		}
		else if (run > 1 && run < 42 && level == 1)
		{
		    length = coeff_tab3[run - 2].len;
		    if (length != 0)
		    {
				/* boon 120697 */
				BitstreamPutBits(bitstream, 3L, 7L);
				/* boon 120697 */
				BitstreamPutBits(bitstream, 2L, 2L);
				length += 9;			 /* boon */
				BitstreamPutBits(bitstream, 
					coeff_tab3[run - 2].code,
					coeff_tab3[run - 2].len);
		    }
		}
    }

    return length;
}





int PutRunCoeff_intra(Bitstream * bitstream, int run, int level, int last)
{
    int length = 0;

    assert(run >= 0 && run < 64);
    assert(level > 0 && level < 128);

    if (!last)
    {
		if (run == 0 && level < 28)
		{
		    length = coeff_tab4[level - 1].len;
		    if (length != 0)
		    {
				/* boon 120697 */
				BitstreamPutBits(bitstream, 3L, 7L);
				/* boon 120697 */
				BitstreamPutBits(bitstream, 2L, 2L);
				length += 9;			 /* boon */
				BitstreamPutBits(bitstream, 
					coeff_tab4[level - 1].code,
					coeff_tab4[level - 1].len);
		    }
		}
		else if (run == 1 && level < 11)
		{
		    length = coeff_tab5[level - 1].len;
		    if (length != 0)
		    {
				/* boon 120697 */
				BitstreamPutBits(bitstream, 3L, 7L);
				/* boon 120697 */
				BitstreamPutBits(bitstream, 2L, 2L);
				length += 9;			 /* boon */
				BitstreamPutBits(bitstream, 
					coeff_tab5[level - 1].code,
					coeff_tab5[level - 1].len);
		    }
		}
		else if (run > 1 && run < 10 && level < 6)
		{
		    length = coeff_tab6[run - 2][level - 1].len;
		    if (length != 0)
		    {
				/* boon 120697 */
				BitstreamPutBits(bitstream, 3L, 7L);
				/* boon 120697 */
				BitstreamPutBits(bitstream, 2L, 2L);
				length += 9;			 /* boon */
				BitstreamPutBits(bitstream,
					coeff_tab6[run - 2][level - 1].code,
					coeff_tab6[run - 2][level - 1].len);
			}
		}
		else if (run > 9 && run < 15 && level == 1)
		{
		    length = coeff_tab7[run - 10].len;
		    if (length != 0)
		    {
				/* boon 120697 */
				BitstreamPutBits(bitstream, 3L, 7L);
				/* boon 120697 */
				BitstreamPutBits(bitstream, 2L, 2L);
				length += 9;			 /* boon */
				BitstreamPutBits(bitstream, 
					coeff_tab7[run - 10].code,
					coeff_tab7[run - 10].len);
		    }
		}
    }
    else // last == 1
	{
		if (run == 0 && level < 9)
		{
		    length = coeff_tab8[level - 1].len;
		    if (length != 0)
		    {
				/* boon 120697 */
				BitstreamPutBits(bitstream, 3L, 7L);
				/* boon 120697 */
				BitstreamPutBits(bitstream, 2L, 2L);
				length += 9;			 /* boon */
				BitstreamPutBits(bitstream, coeff_tab8[level - 1].code,
					coeff_tab8[level - 1].len);
		    }
		}
		else if (run > 0 && run < 7 && level < 4)
		{
		    length = coeff_tab9[run - 1][level - 1].len;
		    if (length != 0)
		    {
				/* boon 120697 */
				BitstreamPutBits(bitstream, 3L, 7L);
				/* boon 120697 */
				BitstreamPutBits(bitstream, 2L, 2L);
				length += 9;			 /* boon */
				BitstreamPutBits(bitstream,
					coeff_tab9[run - 1][level - 1].code,
					 coeff_tab9[run - 1][level - 1].len);
		    }
		}
		else if (run > 6 && run < 21 && level == 1)
		{
		    length = coeff_tab10[run - 7].len;
		    if (length != 0)
		    {
				/* boon 120697 */
				BitstreamPutBits(bitstream, 3L, 7L);
				/* boon 120697 */
				BitstreamPutBits(bitstream, 2L, 2L);
				length += 9;			 /* boon */
				BitstreamPutBits(bitstream, 
					coeff_tab10[run - 7].code,
					coeff_tab10[run - 7].len);
		    }
		}
	}

    return length;
}




int PutLevelCoeff_inter(Bitstream * bitstream, int run, int level, int last)
{
    int length = 0;

    assert(run >= 0 && run < 64);
    assert(level > 0 && level < 128);

    if (!last)
    {
		if (run < 2 && level < 13)
		{
		    length = coeff_tab0[run][level - 1].len;
		    if (length != 0)
		    {
				BitstreamPutBits(bitstream, 3L, 7L);
				/*  boon19970701 */
				BitstreamPutBits(bitstream, 0L, 1L);
				length += 8;			 /* boon */
				BitstreamPutBits(bitstream, coeff_tab0[run][level - 1].code,
					coeff_tab0[run][level - 1].len);
		    }
		}
		else if (run > 1 && run < 27 && level < 5)
		{
		    length = coeff_tab1[run - 2][level - 1].len;
			if (length != 0)
		    {
				BitstreamPutBits(bitstream, 3L, 7L);
				/* boon19970701 */
				BitstreamPutBits(bitstream, 0L, 1L);
				length += 8;			 /* boon */
				BitstreamPutBits(bitstream,
					coeff_tab1[run - 2][level - 1].code,
					coeff_tab1[run - 2][level - 1].len);
		    }
		}
	}
    else // last == 1
    {
		if (run < 2 && level < 4)
		{
		    length = coeff_tab2[run][level - 1].len;
		    if (length != 0)
		    {
				BitstreamPutBits(bitstream, 3L, 7L);
				/* boon19970701 */
				BitstreamPutBits(bitstream, 0L, 1L);
				length += 8;			 /* boon */
				BitstreamPutBits(bitstream, 
					coeff_tab2[run][level - 1].code,
					coeff_tab2[run][level - 1].len);
		    }
		}
		else if (run > 1 && run < 42 && level == 1)
		{
		    length = coeff_tab3[run - 2].len;
		    if (length != 0)
		    {
				BitstreamPutBits(bitstream, 3L, 7L);
				/* boon19970701 */
				BitstreamPutBits(bitstream, 0L, 1L);
				length += 8;			 /* boon */
				BitstreamPutBits(bitstream, 
					coeff_tab3[run - 2].code,
					coeff_tab3[run - 2].len);
		    }
		}
    }

    return length;
}





int PutLevelCoeff_intra(Bitstream * bitstream, int run, int level, int last)
{
    int length = 0;

    assert(run >= 0 && run < 64);
    assert(level > 0 && level < 128);

    if (!last)
    {
		if (run == 0 && level < 28)
		{
		    length = coeff_tab4[level - 1].len;
		    if (length != 0)
		    {
				BitstreamPutBits(bitstream, 3L, 7L);
				/* boon19970701 */
				BitstreamPutBits(bitstream, 0L, 1L);
				length += 8;			 /*  boon */
				BitstreamPutBits(bitstream, 
					coeff_tab4[level - 1].code,
					coeff_tab4[level - 1].len);
		    }
		}
		else if (run == 1 && level < 11)
		{
		    length = coeff_tab5[level - 1].len;
		    if (length != 0)
		    {
				BitstreamPutBits(bitstream, 3L, 7L);
				/* boon19970701 */
				BitstreamPutBits(bitstream, 0L, 1L);
				length += 8;			 /* boon */
				BitstreamPutBits(bitstream, 
					coeff_tab5[level - 1].code,
					coeff_tab5[level - 1].len);
		    }
		}
		else if (run > 1 && run < 10 && level < 6)
		{
		    length = coeff_tab6[run - 2][level - 1].len;
		    if (length != 0)
		    {
				BitstreamPutBits(bitstream, 3L, 7L);
				/* boon19970701 */
				BitstreamPutBits(bitstream, 0L, 1L);
				length += 8;			 /* boon */
				BitstreamPutBits(bitstream, 
					coeff_tab6[run - 2][level - 1].code,
					coeff_tab6[run - 2][level - 1].len);
		    }
		}
		else if (run > 9 && run < 15 && level == 1)
		{
		    length = coeff_tab7[run - 10].len;
		    if (length != 0)
		    {
				BitstreamPutBits(bitstream, 3L, 7L);
				/* boon19970701 */
				BitstreamPutBits(bitstream, 0L, 1L);
				length += 8;			 /* boon */
				BitstreamPutBits(bitstream, 
					coeff_tab7[run - 10].code,
					coeff_tab7[run - 10].len);
		    }
		}
    }
    else // last == 1
    {
		if (run == 0 && level < 9)
		{
		    length = coeff_tab8[level - 1].len;
		    if (length != 0)
		    {
				BitstreamPutBits(bitstream, 3L, 7L);
				/* boon19970701 */
				BitstreamPutBits(bitstream, 0L, 1L);
				length += 8;			 /* boon */
				BitstreamPutBits(bitstream, 
					coeff_tab8[level - 1].code,
					coeff_tab8[level - 1].len);
		    }
		}
		else if (run > 0 && run < 7 && level < 4)
		{
		    length = coeff_tab9[run - 1][level - 1].len;
		    if (length != 0)
		    {
				BitstreamPutBits(bitstream, 3L, 7L);
				/* boon19970701 */
				BitstreamPutBits(bitstream, 0L, 1L);
				length += 8;			 /* boon */
				BitstreamPutBits(bitstream,
					coeff_tab9[run - 1][level - 1].code,
					coeff_tab9[run - 1][level - 1].len);
		    }
		}
		else if (run > 6 && run < 21 && level == 1)
		{
		    length = coeff_tab10[run - 7].len;
		    if (length != 0)
		    {
				BitstreamPutBits(bitstream, 3L, 7L);
				/* boon19970701 */
				BitstreamPutBits(bitstream, 0L, 1L);
				length += 8;			 /* boon */
				BitstreamPutBits(bitstream, 
					coeff_tab10[run - 7].code,
					coeff_tab10[run - 7].len);
		    }
		}
    }

    return length;
}



/***********************************************************CommentBegin******
 *
 * -- PutIntraDC -- DPCM Encoding of INTRADC in case of Intra macroblocks
 *
 * Purpose :
 *	DPCM Encoding of INTRADC in case of Intra macroblocks
 *
 * Arguments in :
 *	int val : the difference value with respect to the previous
 *		INTRADC value
 * 	int lum : indicates whether the block is a luminance block (lum=1) or
 * 		a chrominance block ( lum = 0)
 *
 * Arguments out :
 *	Bitstream* bitstream  : a pointer to the output bitstream
 *
 ***********************************************************CommentEnd********/


int PutIntraDC(Bitstream * bitstream, int val, bool lum)
{
    int n_bits;
    int absval, size = 0;

    absval = (val < 0) ? -val : val;		 /*  abs(val) */

    /* compute dct_dc_size */

    size = 0;
    while (absval)
    {
		absval >>= 1;
		size++;
    }

    if (lum)
		n_bits = PutDCsize_lum(bitstream, size);
    else
		n_bits = PutDCsize_chrom(bitstream, size);

    if (size != 0)
    {
		if (val < 0)
		{
		    absval = -val;	/* set to "-val" MW 14-NOV-1996 */
//          val = (absval ^( (int)pow(2.0,(double)size)-1) );
		    val = absval ^ ((1 << size) - 1);
		}
		BitstreamPutBits(bitstream, (long) (val), (long) (size));
		n_bits += size;

		if (size > 8)
		    BitstreamPutBits(bitstream, (long) 1, (long) 1);
    }

    return n_bits;			/* # bits for intra_dc dpcm */
}



static int PutCoeff_Last(Bitstream * bitstream, int run, int level, int Mode)
{
    int s = 0;
    bool intra = 0;
    int length = 0;

    assert(level != 0);
    assert(bitstream);
    assert(run < 64);
    assert(run >= 0);

    if (level < 0)
    {
		s = 1;
		level = -level;
    }

    if ((Mode == MODE_INTRA) || (Mode == MODE_INTRA_Q))
		intra = 1;

    if ((run < 64) && (((level < 4) && !intra) || ((level < 9) && intra)))
    {
		/* Separate tables for Intra luminance blocks */

		if (intra)
		    length = PutCoeff_intra(bitstream, run, level, 1);
		else
		    length = PutCoeff_inter(bitstream, run, level, 1);

    }


    /*  First escape mode. Level offset */

    if (length == 0)
    {
		/* subtraction of Max level, last = 0 */
		int level_minus_max;
	
		if (intra)
		    level_minus_max = level - intra_max_level[1][run];
		else
		    level_minus_max = level - inter_max_level[1][run];

		if (((level_minus_max < 4) && (!intra)) ||
		    ((level_minus_max < 9) && intra))
		{
		    /* Separate tables for Intra luminance blocks */

			if (intra)
				length = PutLevelCoeff_intra(bitstream, run, level_minus_max, 1);
		    else
				length = PutLevelCoeff_inter(bitstream, run, level_minus_max, 1);

		}
    }

    /* Second escape mode. Run offset */

    if (length == 0)
    {
		if (((level < 4) && (!intra)) || ((level < 9) && intra))
		{
		    /* subtraction of Max Run, last = 1 */
		    int run_minus_max;
	
			assert(level);

			if (intra)
				run_minus_max = run - (intra_max_run1[level] + 1);
		    else
				run_minus_max = run - (inter_max_run1[level] + 1);

		    /*  Separate tables for Intra luminance blocks */

			if (intra)
				length = PutRunCoeff_intra(bitstream, run_minus_max, level, 1);
		    else
				length = PutRunCoeff_inter(bitstream, run_minus_max, level, 1);

		}
    }


    /* Third escape mode. FLC */

    if (length == 0)
    {						 /*  Escape coding */
		if (s == 1)
		    level = (level ^ 0xfff) + 1;
	
		BitstreamPutBits(bitstream, 3, 7);
		BitstreamPutBits(bitstream, 3, 2);
		BitstreamPutBits(bitstream, 1, 1);
		BitstreamPutBits(bitstream, run, 6);

		BitstreamPutBits(bitstream, MARKER_BIT, 1);
		BitstreamPutBits(bitstream, level, 12);
	
		BitstreamPutBits(bitstream, MARKER_BIT, 1);

		return 30;

    }
    else
    {
		BitstreamPutBits(bitstream, s, 1);
		return length + 1;
    }
}



static int PutCoeff_NotLast(Bitstream * bitstream, int run, int level,
			    int Mode)
{
    int s = 0;
    bool intra = 0;
    int length = 0;

    assert(level != 0);
    assert(bitstream);
    assert(run < 64);
    assert(run >= 0);

    if (level < 0)
    {
		s = 1;
		level = -level;
    }

    if ((Mode == MODE_INTRA) || (Mode == MODE_INTRA_Q))
		intra = 1;

    if ((run < 64) &&
		((((level < 13) && (!intra))) || ((level < 28) && intra)))
    {
		if (intra)
		    length = PutCoeff_intra(bitstream, run, level, 0);
		else
		    length = PutCoeff_inter(bitstream, run, level, 0);
    }


    /* First escape mode. Level offset */

    if (length == 0)
    {
		if (run < 64)
		{
		    /* subtraction of Max level, last = 0 */
		    int level_minus_max;

		    if (intra)
				level_minus_max = level - intra_max_level[0][run];
		    else
				level_minus_max = level - inter_max_level[0][run];
	
			if (((level_minus_max < 13) && (!intra)) ||
				((level_minus_max < 28) && intra))
		    {
				/*  Separate tables for Intra luminance blocks */

				if (intra)
				    length = PutLevelCoeff_intra(bitstream, run, level_minus_max, 0);
				else
				    length = PutLevelCoeff_inter(bitstream, run, level_minus_max, 0);
			}

		}
    }

    /*  Second escape mode. Run offset */

    if (length == 0)
    {
		if (((level < 13) && (!intra)) || ((level < 28) && intra))
		{
			/* subtraction of Max Run, last = 0 */
	
			int run_minus_max;

		    assert(level);
	
			if (intra)
				run_minus_max = run - (intra_max_run0[level] + 1);
			else
				run_minus_max = run - (inter_max_run0[level] + 1);

			/* Separate tables for Intra luminance blocks */
			if (intra)
			    length = PutRunCoeff_intra(bitstream, run_minus_max, level, 0);
			else
				length = PutRunCoeff_inter(bitstream, run_minus_max, level, 0);
		}
    }


    /* Third escape mode. FLC */

    if (length == 0)
    {						 /* Escape coding */

		if (s)
		    level = (level ^ 0xfff) + 1;

		BitstreamPutBits(bitstream, 3, 7);
		BitstreamPutBits(bitstream, 3, 2);
		BitstreamPutBits(bitstream, 0, 1);
		BitstreamPutBits(bitstream, run, 6);
		BitstreamPutBits(bitstream, MARKER_BIT, 1);
		BitstreamPutBits(bitstream, level, 12);
		BitstreamPutBits(bitstream, MARKER_BIT, 1);
		return 30;
	}
	else
    {
		BitstreamPutBits(bitstream, s, 1);
		return length + 1;
    }
}



int PutCoeff(Bitstream * bitstream, int run, int level, int last, int Mode)
{
    if (last)
		return PutCoeff_Last(bitstream, run, level, Mode);
    else
		return PutCoeff_NotLast(bitstream, run, level, Mode);
}


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
// mp4_mblock.c //

#include <math.h>
#include <assert.h>
#include <stdlib.h>
#include "our_assert.h"

#include "mp4_decoder.h"
#include "global.h"

#include "mp4_block.h"
#include "mp4_mblock.h"
#include "debug.h"

/**
 *
**/

static int getMCBPC(void);
static int getCBPY(void);
static int setMV(int block_num);
static int getMVdata(void);

/***/

extern int find_pmv(int block_num, int type);
extern void addblock (int comp, int bx, int by, int addflag);
extern void addblockIntra (int comp, int bx, int by);
extern void addblockInter (int comp, int bx, int by);

static int DQtab[4] = {
	-1, -2, 1, 2
};

static VLCtabMb MCBPCtabIntra[] = {
	{-1,0},
	{20,6}, {36,6}, {52,6}, {4,4}, {4,4}, {4,4}, 
	{4,4}, {19,3}, {19,3}, {19,3}, {19,3}, {19,3}, 
	{19,3}, {19,3}, {19,3}, {35,3}, {35,3}, {35,3}, 
	{35,3}, {35,3}, {35,3}, {35,3}, {35,3}, {51,3}, 
	{51,3}, {51,3}, {51,3}, {51,3}, {51,3}, {51,3}, 
	{51,3},
};

static VLCtabMb MCBPCtabInter[] = {
	{-1,0}, 
	{255,9}, {52,9}, {36,9}, {20,9}, {49,9}, {35,8}, {35,8}, {19,8}, {19,8},
	{50,8}, {50,8}, {51,7}, {51,7}, {51,7}, {51,7}, {34,7}, {34,7}, {34,7},
	{34,7}, {18,7}, {18,7}, {18,7}, {18,7}, {33,7}, {33,7}, {33,7}, {33,7}, 
	{17,7}, {17,7}, {17,7}, {17,7}, {4,6}, {4,6}, {4,6}, {4,6}, {4,6}, 
	{4,6}, {4,6}, {4,6}, {48,6}, {48,6}, {48,6}, {48,6}, {48,6}, {48,6}, 
	{48,6}, {48,6}, {3,5}, {3,5}, {3,5}, {3,5}, {3,5}, {3,5}, {3,5}, 
	{3,5}, {3,5}, {3,5}, {3,5}, {3,5}, {3,5}, {3,5}, {3,5}, {3,5}, 
	{32,4}, {32,4}, {32,4}, {32,4}, {32,4}, {32,4}, {32,4}, {32,4}, {32,4}, 
	{32,4}, {32,4}, {32,4}, {32,4}, {32,4}, {32,4}, {32,4}, {32,4}, {32,4}, 
	{32,4}, {32,4}, {32,4}, {32,4}, {32,4}, {32,4}, {32,4}, {32,4}, {32,4}, 
	{32,4}, {32,4}, {32,4}, {32,4}, {32,4}, {16,4}, {16,4}, {16,4}, {16,4}, 
	{16,4}, {16,4}, {16,4}, {16,4}, {16,4}, {16,4}, {16,4}, {16,4}, {16,4}, 
	{16,4}, {16,4}, {16,4}, {16,4}, {16,4}, {16,4}, {16,4}, {16,4}, {16,4}, 
	{16,4}, {16,4}, {16,4}, {16,4}, {16,4}, {16,4}, {16,4}, {16,4}, {16,4}, 
	{16,4}, {2,3}, {2,3}, {2,3}, {2,3}, {2,3}, {2,3}, {2,3}, {2,3}, 
	{2,3}, {2,3}, {2,3}, {2,3}, {2,3}, {2,3}, {2,3}, {2,3}, {2,3}, 
	{2,3}, {2,3}, {2,3}, {2,3}, {2,3}, {2,3}, {2,3}, {2,3}, {2,3}, 
	{2,3}, {2,3}, {2,3}, {2,3}, {2,3}, {2,3}, {2,3}, {2,3}, {2,3}, 
	{2,3}, {2,3}, {2,3}, {2,3}, {2,3}, {2,3}, {2,3}, {2,3}, {2,3}, 
	{2,3}, {2,3}, {2,3}, {2,3}, {2,3}, {2,3}, {2,3}, {2,3}, {2,3}, 
	{2,3}, {2,3}, {2,3}, {2,3}, {2,3}, {2,3}, {2,3}, {2,3}, {2,3}, 
	{2,3}, {2,3}, {1,3}, {1,3}, {1,3}, {1,3}, {1,3}, {1,3}, {1,3}, 
	{1,3}, {1,3}, {1,3}, {1,3}, {1,3}, {1,3}, {1,3}, {1,3}, {1,3}, 
	{1,3}, {1,3}, {1,3}, {1,3}, {1,3}, {1,3}, {1,3}, {1,3}, {1,3}, 
	{1,3}, {1,3}, {1,3}, {1,3}, {1,3}, {1,3}, {1,3}, {1,3}, {1,3}, 
	{1,3}, {1,3}, {1,3}, {1,3}, {1,3}, {1,3}, {1,3}, {1,3}, {1,3}, 
	{1,3}, {1,3}, {1,3}, {1,3}, {1,3}, {1,3}, {1,3}, {1,3}, {1,3}, 
	{1,3}, {1,3}, {1,3}, {1,3}, {1,3}, {1,3}, {1,3}, {1,3}, {1,3}, 
	{1,3}, {1,3}, {1,3}, 
};

/***/

static VLCtabMb CBPYtab[48] = 
{ 
	{-1,0}, {-1,0}, {6,6},  {9,6},  {8,5},  {8,5},  {4,5},  {4,5},
	{2,5},  {2,5},  {1,5},  {1,5},  {0,4},  {0,4},  {0,4},  {0,4}, 
  {12,4}, {12,4}, {12,4}, {12,4}, {10,4}, {10,4}, {10,4}, {10,4},
  {14,4}, {14,4}, {14,4}, {14,4}, {5,4},  {5,4},  {5,4},  {5,4},
  {13,4}, {13,4}, {13,4}, {13,4}, {3,4},  {3,4},  {3,4},  {3,4}, 
  {11,4}, {11,4}, {11,4}, {11,4}, {7,4},  {7,4},  {7,4},  {7,4}, 
};

/***/

static VLCtabMb MVtab0[] =
{
	{3,4}, {-3,4}, {2,3}, {2,3}, {-2,3}, {-2,3}, {1,2}, {1,2}, {1,2}, {1,2},
	{-1,2}, {-1,2}, {-1,2}, {-1,2}
};

static VLCtabMb MVtab1[] = 
{
	{12,10}, {-12,10}, {11,10}, {-11,10}, {10,9}, {10,9}, {-10,9}, {-10,9},
	{9,9}, {9,9}, {-9,9}, {-9,9}, {8,9}, {8,9}, {-8,9}, {-8,9}, {7,7}, {7,7},
	{7,7}, {7,7}, {7,7}, {7,7}, {7,7}, {7,7}, {-7,7}, {-7,7}, {-7,7}, {-7,7},
	{-7,7}, {-7,7}, {-7,7}, {-7,7}, {6,7}, {6,7}, {6,7}, {6,7}, {6,7}, {6,7},
	{6,7}, {6,7}, {-6,7}, {-6,7}, {-6,7}, {-6,7}, {-6,7}, {-6,7}, {-6,7},
	{-6,7}, {5,7}, {5,7}, {5,7}, {5,7}, {5,7}, {5,7}, {5,7}, {5,7}, {-5,7},
	{-5,7}, {-5,7}, {-5,7}, {-5,7}, {-5,7}, {-5,7}, {-5,7}, {4,6}, {4,6}, {4,6},
	{4,6}, {4,6}, {4,6}, {4,6}, {4,6}, {4,6}, {4,6}, {4,6}, {4,6}, {4,6}, {4,6},
	{4,6}, {4,6}, {-4,6}, {-4,6}, {-4,6}, {-4,6}, {-4,6}, {-4,6}, {-4,6},
	{-4,6}, {-4,6}, {-4,6}, {-4,6}, {-4,6}, {-4,6}, {-4,6}, {-4,6}, {-4,6}
};

static VLCtabMb MVtab2[] = 
{
	{32,12}, {-32,12}, {31,12}, {-31,12}, {30,11}, {30,11}, {-30,11}, {-30,11},
	{29,11}, {29,11}, {-29,11}, {-29,11}, {28,11}, {28,11}, {-28,11}, {-28,11},
	{27,11}, {27,11}, {-27,11}, {-27,11}, {26,11}, {26,11}, {-26,11}, {-26,11},
	{25,11}, {25,11}, {-25,11}, {-25,11}, {24,10}, {24,10}, {24,10}, {24,10},
	{-24,10}, {-24,10}, {-24,10}, {-24,10}, {23,10}, {23,10}, {23,10}, {23,10},
	{-23,10}, {-23,10}, {-23,10}, {-23,10}, {22,10}, {22,10}, {22,10}, {22,10},
	{-22,10}, {-22,10}, {-22,10}, {-22,10}, {21,10}, {21,10}, {21,10}, {21,10},
	{-21,10}, {-21,10}, {-21,10}, {-21,10}, {20,10}, {20,10}, {20,10}, {20,10},
	{-20,10}, {-20,10}, {-20,10}, {-20,10}, {19,10}, {19,10}, {19,10}, {19,10},
	{-19,10}, {-19,10}, {-19,10}, {-19,10}, {18,10}, {18,10}, {18,10}, {18,10},
	{-18,10}, {-18,10}, {-18,10}, {-18,10}, {17,10}, {17,10}, {17,10}, {17,10},
	{-17,10}, {-17,10}, {-17,10}, {-17,10}, {16,10}, {16,10}, {16,10}, {16,10},
	{-16,10}, {-16,10}, {-16,10}, {-16,10}, {15,10}, {15,10}, {15,10}, {15,10},
	{-15,10}, {-15,10}, {-15,10}, {-15,10}, {14,10}, {14,10}, {14,10}, {14,10},
	{-14,10}, {-14,10}, {-14,10}, {-14,10}, {13,10}, {13,10}, {13,10}, {13,10},
	{-13,10}, {-13,10}, {-13,10}, {-13,10}
};

/***/

int macroblock()
{
	int j;
	int intraFlag, interFlag;

//	_Print("-Macroblock %d\n", mp4_hdr.mba);
//	_Break(93, 426);
	  
	if (mp4_hdr.prediction_type != I_VOP)
		mp4_hdr.not_coded = getbits(1);

	// coded macroblock or I-VOP
	if (! mp4_hdr.not_coded || mp4_hdr.prediction_type == I_VOP) {

		mp4_hdr.mcbpc = getMCBPC(); // mcbpc
		mp4_hdr.derived_mb_type = mp4_hdr.mcbpc & 7;
		mp4_hdr.cbpc = (mp4_hdr.mcbpc >> 4) & 3;

		modemap[mp4_hdr.mb_ypos+1][mp4_hdr.mb_xpos+1] = 
			mp4_hdr.derived_mb_type; // [Review] used only in P-VOPs
		intraFlag = ((mp4_hdr.derived_mb_type == INTRA) || 
			(mp4_hdr.derived_mb_type == INTRA_Q)) ? 1 : 0;
		interFlag = (! intraFlag);

		if (intraFlag)
			mp4_hdr.ac_pred_flag = getbits(1);
		if (mp4_hdr.derived_mb_type != STUFFING) {

			mp4_hdr.cbpy = getCBPY(); // cbpy
			mp4_hdr.cbp = (mp4_hdr.cbpy << 2) | mp4_hdr.cbpc;
		}
		else
			return 1;
		
//		_Print("Mb type: %d\n", mp4_hdr.derived_mb_type);
//		_Print("Pattern Chr: %d\n", mp4_hdr.cbpc);
//		_Print("Pattern Luma: %d\n", mp4_hdr.cbpy);

		if (mp4_hdr.derived_mb_type == INTER_Q ||
			mp4_hdr.derived_mb_type == INTRA_Q) {

			mp4_hdr.dquant = getbits(2);
			mp4_hdr.quantizer += DQtab[mp4_hdr.dquant];
			if (mp4_hdr.quantizer > 31)
				mp4_hdr.quantizer = 31;
			else if (mp4_hdr.quantizer < 1)
				mp4_hdr.quantizer = 1;
		}
		if (mp4_hdr.derived_mb_type == INTER ||
			mp4_hdr.derived_mb_type == INTER_Q) {

			setMV(-1); // mv
		}
		else if (mp4_hdr.derived_mb_type == INTER4V) {
			for (j = 0; j < 4; j++) {

				setMV(j); // mv
			}
		}
		else { // intra
			if (mp4_hdr.prediction_type == P_VOP) {
				int i;
				for (i = 0; i < 4; i++) {
					MV[0][i][mp4_hdr.mb_ypos+1][mp4_hdr.mb_xpos+1] = 0;
					MV[1][i][mp4_hdr.mb_ypos+1][mp4_hdr.mb_xpos+1] = 0;
				}
			}
		}

		// motion compensation
		if (interFlag) 
		{
			reconstruct(mp4_hdr.mb_xpos, mp4_hdr.mb_ypos, mp4_hdr.derived_mb_type);

			// texture decoding add
			for (j = 0; j < 6; j++) {
				int coded = mp4_hdr.cbp & (1 << (5 - j));

				blockInter(j, (coded != 0));
				addblockInter(j, mp4_hdr.mb_xpos, mp4_hdr.mb_ypos);
			}
		}
		else 
		{
			// texture decoding add
			for (j = 0; j < 6; j++) {
				int coded = mp4_hdr.cbp & (1 << (5 - j));

				blockIntra(j, (coded != 0));
				addblockIntra(j, mp4_hdr.mb_xpos, mp4_hdr.mb_ypos);
			}
		}
	}

	// not coded macroblock
	else {

		MV[0][0][mp4_hdr.mb_ypos+1][mp4_hdr.mb_xpos+1] = MV[0][1][mp4_hdr.mb_ypos+1][mp4_hdr.mb_xpos+1] =
		MV[0][2][mp4_hdr.mb_ypos+1][mp4_hdr.mb_xpos+1] = MV[0][3][mp4_hdr.mb_ypos+1][mp4_hdr.mb_xpos+1] = 0;
		MV[1][0][mp4_hdr.mb_ypos+1][mp4_hdr.mb_xpos+1] = MV[1][1][mp4_hdr.mb_ypos+1][mp4_hdr.mb_xpos+1] =
		MV[1][2][mp4_hdr.mb_ypos+1][mp4_hdr.mb_xpos+1] = MV[1][3][mp4_hdr.mb_ypos+1][mp4_hdr.mb_xpos+1] = 0;

		modemap[mp4_hdr.mb_ypos+1][mp4_hdr.mb_xpos+1] = NOT_CODED; // [Review] used only in P-VOPs

		reconstruct(mp4_hdr.mb_xpos, mp4_hdr.mb_ypos, mp4_hdr.derived_mb_type);
	}

	quant_store[mp4_hdr.mb_ypos+1][mp4_hdr.mb_xpos+1] = mp4_hdr.quantizer;

	if (mp4_hdr.mb_xpos < (mb_width-1)) {
		mp4_hdr.mb_xpos++;
	}
	else {
		mp4_hdr.mb_ypos++;
		mp4_hdr.mb_xpos = 0;
	}

	return 1;
}

/***/

static int getMCBPC()
{
	if (mp4_hdr.prediction_type == I_VOP)
	{
		int code = showbits(9);

		if (code == 1) {
			flushbits(9); // stuffing
			return 0;
		}
		else if (code < 8) {
			return -1;
		}

		code >>= 3;
		if (code >= 32) {
			flushbits(1);
			return 3;
		}
		
		flushbits(MCBPCtabIntra[code].len);
		return MCBPCtabIntra[code].val;
	}
	else
	{
		int code = showbits(9);

		if (code == 1) {
			flushbits(9); // stuffing
			return 0;
		}
		else if (code == 0)	{
			return -1;
		}
		
		if (code >= 256)
		{
			flushbits(1);
			return 0;
		}
		
		flushbits(MCBPCtabInter[code].len);
		return MCBPCtabInter[code].val;
	}
}

/***/

static int getCBPY()
{
	int cbpy;
	int code = showbits(6);

	if (code < 2) {
		return -1;
	}
			  
	if (code >= 48) {
		flushbits(2);
		cbpy = 15;
	} else {
		flushbits(CBPYtab[code].len);
		cbpy = CBPYtab[code].val;
	}

	if (!((mp4_hdr.derived_mb_type == 3) ||
		(mp4_hdr.derived_mb_type == 4)))
		  cbpy = 15 - cbpy;

  return cbpy;
}

/***/

static int setMV(int block_num)
{
	int hor_mv_data, ver_mv_data, hor_mv_res, ver_mv_res;
	int scale_fac = 1 << (mp4_hdr.fcode_for - 1);
	int high = (32 * scale_fac) - 1;
	int low = ((-32) * scale_fac);
	int range = (64 * scale_fac);

	int mvd_x, mvd_y, pmv_x, pmv_y, mv_x, mv_y;

/***

	[hor_mv_data]
	if ((vop_fcode_forward != 1) && (hor_mv_data != 0))
		[hor_mv_residual]

***/	

  hor_mv_data = getMVdata(); // mv data

	if ((scale_fac == 1) || (hor_mv_data == 0))
		mvd_x = hor_mv_data;
	else {
		hor_mv_res = getbits(mp4_hdr.fcode_for-1); // mv residual
		mvd_x = ((abs(hor_mv_data) - 1) * scale_fac) + hor_mv_res + 1;
		if (hor_mv_data < 0)
			mvd_x = - mvd_x;
	}

  ver_mv_data = getMVdata(); 

	if ((scale_fac == 1) || (ver_mv_data == 0))
		mvd_y = ver_mv_data;
	else {
		ver_mv_res = getbits(mp4_hdr.fcode_for-1);
		mvd_y = ((abs(ver_mv_data) - 1) * scale_fac) + ver_mv_res + 1;
		if (ver_mv_data < 0)
			mvd_y = - mvd_y;
	}

	if (block_num == -1) {
		pmv_x = find_pmv(0, 0);
		pmv_y = find_pmv(0, 1);
	}
	else {
		pmv_x = find_pmv(block_num, 0);
		pmv_y = find_pmv(block_num, 1);
	}

//	_Print("Hor MotV Pred: %d\n", pmv_x);
//	_Print("Ver MorV Pred: %d\n", pmv_y);

	mv_x = pmv_x + mvd_x;

	if (mv_x < low)
		mv_x += range;
	if (mv_x > high)
		mv_x -= range;

	mv_y = pmv_y + mvd_y;

	if (mv_y < low)
		mv_y += range;
	if (mv_y > high)
		mv_y -= range;

	// put [mv_x, mv_y] in MV struct
	if (block_num == -1) {
		int i;
		for (i = 0; i < 4; i++) {
			MV[0][i][mp4_hdr.mb_ypos+1][mp4_hdr.mb_xpos+1] = mv_x;
			MV[1][i][mp4_hdr.mb_ypos+1][mp4_hdr.mb_xpos+1] = mv_y;
		}
	}
	else {
		MV[0][block_num][mp4_hdr.mb_ypos+1][mp4_hdr.mb_xpos+1] = mv_x;
		MV[1][block_num][mp4_hdr.mb_ypos+1][mp4_hdr.mb_xpos+1] = mv_y;
	}

//  _Print("Hor MotV: %d\n", mv_x);
//	_Print("Ver MotV: %d\n", mv_y);

  return 1;
}

/***/

static int getMVdata()
{
	int code;

	if (getbits(1)) {
		return 0; // hor_mv_data == 0
  }
	
	code = showbits(12);

	if (code >= 512)
  {
		code = (code >> 8) - 2;
		flushbits(MVtab0[code].len);
		return MVtab0[code].val;
  }
	
	if (code >= 128)
  {
		code = (code >> 2) - 32;
		flushbits(MVtab1[code].len);
		return MVtab1[code].val;
  }

	code -= 4; 

	ASSERT(code >= 0);

	flushbits(MVtab2[code].len);
	return MVtab2[code].val;
}

/***/


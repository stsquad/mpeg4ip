/**************************************************************************
 *                                                                        *
 * This code has been developed by John Funnell. This software is an      *
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
 * John Funnell
 * Andrea Graziani
 *
 * DivX Advanced Research Center <darc@projectmayo.com>
*
**/
// mp4_vld.c //

#include <stdio.h>

#include "mp4_decoder.h"
#include "global.h"

#include "mp4_vld.h"

/**
 * 
**/
static tab_type tableB16_1[] = // tables to decode Table B16 VLC - 112 values
{ 
	{4353, 7},  {4289, 7},  {385, 7},  {4417, 7},  {449, 7},  {130, 7},  {67, 7},  {9, 7},  {4098, 6},  {4098, 6},  
	{321, 6},  {321, 6},  {4225, 6},  {4225, 6},  {4161, 6},  {4161, 6},  {257, 6},  {257, 6},  {193, 6},  {193, 6},  
	{8, 6},  {8, 6},  {7, 6},  {7, 6},  {66, 6},  {66, 6},  {6, 6},  {6, 6},  {129, 5},  {129, 5},  
	{129, 5},  {129, 5},  {5, 5},  {5, 5},  {5, 5},  {5, 5},  {4, 5},  {4, 5},  {4, 5},  {4, 5},  
	{4097, 4},  {4097, 4},  {4097, 4},  {4097, 4},  {4097, 4},  {4097, 4},  {4097, 4},  {4097, 4},  {1, 2},  {1, 2},  
	{1, 2},  {1, 2},  {1, 2},  {1, 2},  {1, 2},  {1, 2},  {1, 2},  {1, 2},  {1, 2},  {1, 2},  
	{1, 2},  {1, 2},  {1, 2},  {1, 2},  {1, 2},  {1, 2},  {1, 2},  {1, 2},  {1, 2},  {1, 2},  
	{1, 2},  {1, 2},  {1, 2},  {1, 2},  {1, 2},  {1, 2},  {1, 2},  {1, 2},  {1, 2},  {1, 2},  
	{2, 3},  {2, 3},  {2, 3},  {2, 3},  {2, 3},  {2, 3},  {2, 3},  {2, 3},  {2, 3},  {2, 3},  
	{2, 3},  {2, 3},  {2, 3},  {2, 3},  {2, 3},  {2, 3},  {65, 4},  {65, 4},  {65, 4},  {65, 4},  
	{65, 4},  {65, 4},  {65, 4},  {65, 4},  {3, 4},  {3, 4},  {3, 4},  {3, 4},  {3, 4},  {3, 4},  
	{3, 4},  {3, 4} 
};
static tab_type tableB16_2[] = { {18, 10},  {17, 10},  {4993, 9},  {4993, 9},  {4929, 9},  {4929, 9},  {4865, 9},  {4865, 9},  {4801, 9},  {4801, 9},  {4737, 9},  {4737, 9},  {4162, 9},  {4162, 9},  {4100, 9},  {4100, 9},  {769, 9},  {769, 9},  {705, 9},  {705, 9},  {450, 9},  {450, 9},  {386, 9},  {386, 9},  {322, 9},  {322, 9},  {195, 9},  {195, 9},  {131, 9},  {131, 9},  {70, 9},  {70, 9},  {69, 9},  {69, 9},  {16, 9},  {16, 9},  {258, 9},  {258, 9},  {15, 9},  {15, 9},  {14, 9},  {14, 9},  {13, 9},  {13, 9},  {4609, 8},  {4609, 8},  {4609, 8},  {4609, 8},  {4545, 8},  {4545, 8},  {4545, 8},  {4545, 8},  {4481, 8},  {4481, 8},  {4481, 8},  {4481, 8},  {4099, 8},  {4099, 8},  {4099, 8},  {4099, 8},  {641, 8},  {641, 8},  {641, 8},  {641, 8},  {577, 8},  {577, 8},  {577, 8},  {577, 8},  {513, 8},  {513, 8},  {513, 8},  {513, 8},  {4673, 8},  {4673, 8},  {4673, 8},  {4673, 8},  {194, 8},  {194, 8},  {194, 8},  {194, 8},  {68, 8},  {68, 8},  {68, 8},  {68, 8},  {12, 8},  {12, 8},  {12, 8},  {12, 8},  {11, 8},  {11, 8},  {11, 8},  {11, 8},  {10, 8},  {10, 8},  {10, 8},  {10, 8} };
static tab_type tableB16_3[] = { {4103, 11},  {4103, 11},  {4102, 11},  {4102, 11},  {22, 11},  {22, 11},  {21, 11},  {21, 11},  {4226, 10},  {4226, 10},  {4226, 10},  {4226, 10},  {4163, 10},  {4163, 10},  {4163, 10},  {4163, 10},  {4101, 10},  {4101, 10},  {4101, 10},  {4101, 10},  {833, 10},  {833, 10},  {833, 10},  {833, 10},  {323, 10},  {323, 10},  {323, 10},  {323, 10},  {514, 10},  {514, 10},  {514, 10},  {514, 10},  {259, 10},  {259, 10},  {259, 10},  {259, 10},  {196, 10},  {196, 10},  {196, 10},  {196, 10},  {132, 10},  {132, 10},  {132, 10},  {132, 10},  {71, 10},  {71, 10},  {71, 10},  {71, 10},  {20, 10},  {20, 10},  {20, 10},  {20, 10},  {19, 10},  {19, 10},  {19, 10},  {19, 10},  {23, 11},  {23, 11},  {24, 11},  {24, 11},  {72, 11},  {72, 11},  {578, 11},  {578, 11},  {4290, 11},  {4290, 11},  {4354, 11},  {4354, 11},  {5057, 11},  {5057, 11},  {5121, 11},  {5121, 11},  {25, 12},  {26, 12},  {27, 12},  {73, 12},  {387, 12},  {74, 12},  {133, 12},  {451, 12},  {897, 12},  {4104, 12},  {4418, 12},  {4482, 12},  {5185, 12},  {5249, 12},  {5313, 12},  {5377, 12},  {7167, 7},  {7167, 7},  {7167, 7},  {7167, 7},  {7167, 7},  {7167, 7},  {7167, 7},  {7167, 7},  {7167, 7},  {7167, 7},  {7167, 7},  {7167, 7},  {7167, 7},  {7167, 7},  {7167, 7},  {7167, 7},  {7167, 7},  {7167, 7},  {7167, 7},  {7167, 7},  {7167, 7},  {7167, 7},  {7167, 7},  {7167, 7},  {7167, 7},  {7167, 7},  {7167, 7},  {7167, 7},  {7167, 7},  {7167, 7},  {7167, 7},  {7167, 7} };

/* tables to decode Table B17 VLC */
static tab_type tableB17_1[] = { {4225,7}, {4209,7}, {4193,7}, {4177,7}, {193,7}, {177,7}, {161,7}, {4,7}, {4161,6}, {4161,6}, {4145,6}, {4145,6}, {4129,6}, {4129,6}, {4113,6}, {4113,6}, {145,6}, {145,6}, {129,6}, {129,6}, {113,6}, {113,6}, {97,6}, {97,6}, {18,6}, {18,6}, {3,6}, {3,6}, {81,5}, {81,5}, {81,5}, {81,5}, {65,5}, {65,5}, {65,5}, {65,5}, {49,5}, {49,5}, {49,5}, {49,5}, {4097,4}, {4097,4}, {4097,4}, {4097,4}, {4097,4}, {4097,4}, {4097,4}, {4097,4}, {1,2}, {1,2}, {1,2}, {1,2}, {1,2}, {1,2}, {1,2}, {1,2}, {1,2}, {1,2}, {1,2}, {1,2}, {1,2}, {1,2}, {1,2}, {1,2}, {1,2}, {1,2}, {1,2}, {1,2}, {1,2}, {1,2}, {1,2}, {1,2}, {1,2}, {1,2}, {1,2}, {1,2}, {1,2}, {1,2}, {1,2}, {1,2}, {17,3}, {17,3}, {17,3}, {17,3}, {17,3}, {17,3}, {17,3}, {17,3}, {17,3}, {17,3}, {17,3}, {17,3}, {17,3}, {17,3}, {17,3}, {17,3}, {33,4}, {33,4}, {33,4}, {33,4}, {33,4}, {33,4}, {33,4}, {33,4}, {2,4}, {2,4},{2,4},{2,4}, {2,4}, {2,4},{2,4},{2,4} };
static tab_type tableB17_2[] = { {9,10}, {8,10}, {4481,9}, {4481,9}, {4465,9}, {4465,9}, {4449,9}, {4449,9}, {4433,9}, {4433,9}, {4417,9}, {4417,9}, {4401,9}, {4401,9}, {4385,9}, {4385,9}, {4369,9}, {4369,9}, {4098,9}, {4098,9}, {353,9}, {353,9}, {337,9}, {337,9}, {321,9}, {321,9}, {305,9}, {305,9}, {289,9}, {289,9}, {273,9}, {273,9}, {257,9}, {257,9}, {241,9}, {241,9}, {66,9}, {66,9}, {50,9}, {50,9}, {7,9}, {7,9}, {6,9}, {6,9}, {4353,8}, {4353,8}, {4353,8}, {4353,8}, {4337,8}, {4337,8}, {4337,8}, {4337,8}, {4321,8}, {4321,8}, {4321,8}, {4321,8}, {4305,8}, {4305,8}, {4305,8}, {4305,8}, {4289,8}, {4289,8}, {4289,8}, {4289,8}, {4273,8}, {4273,8}, {4273,8}, {4273,8}, {4257,8}, {4257,8}, {4257,8}, {4257,8}, {4241,8}, {4241,8}, {4241,8}, {4241,8}, {225,8}, {225,8}, {225,8}, {225,8}, {209,8}, {209,8}, {209,8}, {209,8}, {34,8}, {34,8}, {34,8}, {34,8}, {19,8}, {19,8}, {19,8}, {19,8}, {5,8}, {5,8}, {5,8}, {5,8}, };
static tab_type tableB17_3[] = { {4114,11}, {4114,11}, {4099,11}, {4099,11}, {11,11}, {11,11}, {10,11}, {10,11}, {4545,10}, {4545,10}, {4545,10}, {4545,10}, {4529,10}, {4529,10}, {4529,10}, {4529,10}, {4513,10}, {4513,10}, {4513,10}, {4513,10}, {4497,10}, {4497,10}, {4497,10}, {4497,10}, {146,10}, {146,10}, {146,10}, {146,10}, {130,10}, {130,10}, {130,10}, {130,10}, {114,10}, {114,10}, {114,10}, {114,10}, {98,10}, {98,10}, {98,10}, {98,10}, {82,10}, {82,10}, {82,10}, {82,10}, {51,10}, {51,10}, {51,10}, {51,10}, {35,10}, {35,10}, {35,10}, {35,10}, {20,10}, {20,10}, {20,10}, {20,10}, {12,11}, {12,11}, {21,11}, {21,11}, {369,11}, {369,11}, {385,11}, {385,11}, {4561,11}, {4561,11}, {4577,11}, {4577,11}, {4593,11}, {4593,11}, {4609,11}, {4609,11}, {22,12}, {36,12}, {67,12}, {83,12}, {99,12}, {162,12}, {401,12}, {417,12}, {4625,12}, {4641,12}, {4657,12}, {4673,12}, {4689,12}, {4705,12}, {4721,12}, {4737,12}, {7167,7}, {7167,7}, {7167,7}, {7167,7}, {7167,7}, {7167,7}, {7167,7}, {7167,7}, {7167,7}, {7167,7}, {7167,7}, {7167,7}, {7167,7}, {7167,7}, {7167,7}, {7167,7}, {7167,7}, {7167,7}, {7167,7}, {7167,7}, {7167,7}, {7167,7}, {7167,7}, {7167,7}, {7167,7}, {7167,7}, {7167,7}, {7167,7}, {7167,7}, {7167,7}, {7167,7}, {7167,7}, };

static int vldTableB19(int last, int run);
static int vldTableB20(int last, int run);
static int vldTableB21(int last, int level);
static int vldTableB22(int last, int level);

static tab_type *vldTableB16(int code);
static tab_type *vldTableB17(int code);

/***/

event_t vld_intra_dct() 
{
	event_t event;
	tab_type *tab = NULL;
	int lmax, rmax;

	tab = vldTableB16(showbits(12));
	if (!tab) { /* bad code */
		event.run   = 
		event.level = 
		event.last  = -1;
		return event;
	} 

	if (tab->val != ESCAPE) {
		event.run   = (tab->val >>  6) & 63;
		event.level =  tab->val        & 63;
		event.last  = (tab->val >> 12) &  1;
		event.level = getbits(1) ? -event.level : event.level;
	} else {
		/* this value is escaped - see para 7.4.1.3 */
		/* assuming short_video_header == 0 */
		switch (showbits(2)) {
			case 0x0 :  /* Type 1 */
			case 0x1 :  /* Type 1 */
				flushbits(1);
				tab = vldTableB16(showbits(12));  /* use table B-16 */
				if (!tab) { /* bad code */
					event.run   = 
					event.level = 
					event.last  = -1;
					return event;
				}
				event.run   = (tab->val >>  6) & 63;
				event.level =  tab->val        & 63;
				event.last  = (tab->val >> 12) &  1;
				lmax = vldTableB19(event.last, event.run);  /* use table B-19 */
				event.level += lmax;
				event.level =  getbits(1) ? -event.level : event.level;
				break;
			case 0x2 :  /* Type 2 */
				flushbits(2);
				tab = vldTableB16(showbits(12));  /* use table B-16 */
				if (!tab) { /* bad code */
					event.run   = 
					event.level = 
					event.last  = -1;
					break;
				}
				event.run   = (tab->val >>  6) & 63;
				event.level =  tab->val        & 63;
				event.last  = (tab->val >> 12) &  1;
				rmax = vldTableB21(event.last, event.level);  /* use table B-21 */
				event.run = event.run + rmax + 1;
				event.level = getbits(1) ? -event.level : event.level;
				break;
			case 0x3 :  /* Type 3  - fixed length codes */
				flushbits(2);
				event.last  = getbits(1);
				event.run   = getbits(6);  /* table B-18 */ 
				getbits(1); /* marker bit */
				event.level = getbits(12); /* table B-18 */
				/* sign extend level... */
				event.level = (event.level & 0x800) ? (event.level | (-1 ^ 0xfff)) : event.level;
				getbits(1); /* marker bit */
				break;
		}
	}

	return event;
}

/***/

event_t vld_inter_dct() 
{
	event_t event;
	tab_type *tab = NULL;
	int lmax, rmax;

	tab = vldTableB17(showbits(12));
	if (!tab) { /* bad code */
		event.run   = 
		event.level = 
		event.last  = -1;
		return event;
	} 
	if (tab->val != ESCAPE) {
		event.run   = (tab->val >>  4) & 255;
		event.level =  tab->val        & 15;
		event.last  = (tab->val >> 12) &  1;
		event.level = getbits(1) ? -event.level : event.level;
	} else {
		/* this value is escaped - see para 7.4.1.3 */
		/* assuming short_video_header == 0 */
		int mode = showbits(2);
		switch (mode) {
			case 0x0 :  /* Type 1 */
			case 0x1 :  /* Type 1 */
				flushbits(1);
				tab = vldTableB17(showbits(12));  /* use table B-17 */
				if (!tab) { /* bad code */
					event.run   = 
					event.level = 
					event.last  = -1;
					return event;
				}
				event.run   = (tab->val >>  4) & 255;
				event.level =  tab->val        & 15;
				event.last  = (tab->val >> 12) &  1;
				lmax = vldTableB20(event.last, event.run);  /* use table B-20 */
				event.level += lmax;
				event.level = getbits(1) ? -event.level : event.level;
				break;
			case 0x2 :  /* Type 2 */
				flushbits(2);
				tab = vldTableB17(showbits(12));  /* use table B-16 */
				if (!tab) { /* bad code */
					event.run   = 
					event.level = 
					event.last  = -1;
					break;
				}
//			flushbits(tab->len); [Ag][bug fixed]
				event.run   = (tab->val >>  4) & 255;
				event.level =  tab->val        & 15;
				event.last  = (tab->val >> 12) &  1;
				rmax = vldTableB22(event.last, event.level);  /* use table B-22 */
				event.run = event.run + rmax + 1;
				event.level = getbits(1) ? -event.level : event.level;
				break;
			case 0x3 :  /* Type 3  - fixed length codes */
				flushbits(2);
				event.last  = getbits(1);
				event.run   = getbits(6);  /* table B-18 */ 
				getbits(1); /* marker bit */
				event.level = getbits(12); /* table B-18 */
				/* sign extend level... */
				event.level = (event.level & 0x800) ? (event.level | (-1 ^ 0xfff)) : event.level;
				getbits(1); /* marker bit */
				break;
		}
	}

	return event;
}

/***/

event_t vld_event(int intraFlag) 
{
	if (intraFlag) {
		return vld_intra_dct();
	} else {
		return vld_inter_dct();
	}
}

/***/

/* Table B-19 -- ESCL(a), LMAX values of intra macroblocks */
/*static */int vldTableB19(int last, int run) {
	//printf("vldTableB19(last=%d, run=%d)\n", last, run);
	if (!last){ /* LAST == 0 */
		if        (run ==  0) {
			return 27;
		} else if (run ==  1) {
			return 10;
		} else if (run ==  2) {
			return  5;
		} else if (run ==  3) {
			return  4;
		} else if (run <=  7) {
			return  3;
		} else if (run <=  9) {
			return  2;
		} else if (run <= 14) {
			return  1;
		} else { /* illegal? */
			return  0; 
		}
	} else {    /* LAST == 1 */
		if        (run ==  0) {
			return  8;
		} else if (run ==  1) {
			return  3;
		} else if (run <=  6) {
			return  2;
		} else if (run <= 20) {
			return  1;
		} else { /* illegal? */
			return  0; 
		}		
	}
}

/***/

/* Table B-20 -- ESCL(b), LMAX values of inter macroblocks */
/*static */int vldTableB20(int last, int run) {
	//printf("vldTableB20(last=%d, run=%d)\n", last, run);
	if (!last){ /* LAST == 0 */
		if        (run ==  0) {
			return 12;
		} else if (run ==  1) {
			return  6;
		} else if (run ==  2) {
			return  4;
		} else if (run <=  6) {
			return  3;
		} else if (run <= 10) {
			return  2;
		} else if (run <= 26) {
			return  1;
		} else { /* illegal? */
			return  0; 
		}
	} else {    /* LAST == 1 */
		if        (run ==  0) {
			return  3;
		} else if (run ==  1) {
			return  2;
		} else if (run <= 40) {
			return  1;
		} else { /* illegal? */
			return  0; 
		}		
	}
}

/***/

/* Table B-21 -- ESCR(a), RMAX values of intra macroblocks */
/*static */int vldTableB21(int last, int level) {
	//printf("vldTableB21(last=%d, level=%d)\n", last, level);
	if (!last){ /* LAST == 0 */
		if        (level ==  1) {
			return 14;
		} else if (level ==  2) {
			return  9;
		} else if (level ==  3) {
			return  7;
		} else if (level ==  4) {
			return  3;
		} else if (level ==  5) {
			return  2;
		} else if (level <= 10) {
			return  1;
		} else if (level <= 27) {
			return  0;
		} else { /* illegal? */
			return  0; 
		}
	} else {    /* LAST == 1 */
		if        (level ==  1) {
			return  20;
		} else if (level ==  2) {
			return  6;
		} else if (level ==  3) {
			return  1;
		} else if (level <=  8) {
			return  0;
		} else { /* illegal? */
			return  0; 
		}		
	}
}

/***/

/* Table B-22 -- ESCR(b), RMAX values of inter macroblocks */
/*static */int vldTableB22(int last, int level) {
	//printf("vldTableB22(last=%d, level=%d)\n", last, level);
	if (!last){ /* LAST == 0 */
		if        (level ==  1) {
			return 26;
		} else if (level ==  2) {
			return 10;
		} else if (level ==  3) {
			return  6;
		} else if (level ==  4) {
			return  2;
		} else if (level <=  6) {
			return  1;
		} else if (level <= 12) {
			return  0;
		} else { /* illegal? */
			return  0; 
		}
	} else {    /* LAST == 1 */
		if        (level ==  1) {
			return  40;
		} else if (level ==  2) {
			return  1;
		} else if (level ==  3) {
			return  0;
		} else { /* illegal? */
			return  0; 
		}		
	}
}

/***/

/*static */tab_type *vldTableB16(int code) {
	tab_type *tab;

	if (code >= 512) {
		tab = &(tableB16_1[(code >> 5) - 16]);
	} else if (code >= 128) {
		tab = &(tableB16_2[(code >> 2) - 32]);
	} else if (code >= 8) {
		tab = &(tableB16_3[(code >> 0) - 8]);
	} else {
		/* invalid Huffman code */
		return NULL;
	}
	flushbits(tab->len);
	return tab;
}

/***/

/*static */tab_type *vldTableB17(int code) {
	tab_type *tab;

	if (code >= 512) {
		tab = &(tableB17_1[(code >> 5) - 16]);
	} else if (code >= 128) {
		tab = &(tableB17_2[(code >> 2) - 32]);
	} else if (code >= 8) {
		tab = &(tableB17_3[(code >> 0) - 8]);
	} else {
		/* invalid Huffman code */
		return NULL;
	}
	flushbits(tab->len);
	return tab;
}

/***/

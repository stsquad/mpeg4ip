/**************************************************************************
 *
 *	History:
 *
 *	04.01.2002	fixed buggy badrun/oveflow detection code
 *
 *************************************************************************/



/*---[start projectmayo code]------------------------------------------ */
// most of this was modifed

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

#include "../portab.h"
#include "mbdecoding.h"
#include "bitreader.h"

#ifndef NULL
#define NULL 0
#endif


#define ESCAPE 7167

typedef struct
{
	int code;
	int len;
} VLC;



static const VLC mcbpc_intra_table[32] = {
	{-1,0},
	{20,6}, {36,6}, {52,6}, {4,4}, {4,4}, {4,4}, 
	{4,4}, {19,3}, {19,3}, {19,3}, {19,3}, {19,3}, 
	{19,3}, {19,3}, {19,3}, {35,3}, {35,3}, {35,3}, 
	{35,3}, {35,3}, {35,3}, {35,3}, {35,3}, {51,3}, 
	{51,3}, {51,3}, {51,3}, {51,3}, {51,3}, {51,3}, 
	{51,3},
};


static const VLC mcbpc_inter_table[256] = {
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


static const VLC cbpy_table[48] = 
{ 
	{-1,0}, {-1,0}, {6,6},  {9,6},  {8,5},  {8,5},  {4,5},  {4,5},
	{2,5},  {2,5},  {1,5},  {1,5},  {0,4},  {0,4},  {0,4},  {0,4}, 
  {12,4}, {12,4}, {12,4}, {12,4}, {10,4}, {10,4}, {10,4}, {10,4},
  {14,4}, {14,4}, {14,4}, {14,4}, {5,4},  {5,4},  {5,4},  {5,4},
  {13,4}, {13,4}, {13,4}, {13,4}, {3,4},  {3,4},  {3,4},  {3,4}, 
  {11,4}, {11,4}, {11,4}, {11,4}, {7,4},  {7,4},  {7,4},  {7,4}, 
};



static const VLC MVtab0[14] =
{
	{3,4}, {-3,4}, {2,3}, {2,3}, {-2,3}, {-2,3}, {1,2}, {1,2}, {1,2}, {1,2},
	{-1,2}, {-1,2}, {-1,2}, {-1,2}
};


static const VLC MVtab1[96] = 
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


static const VLC MVtab2[124] = 
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


static const VLC tableB16_1[112] = 
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

static const VLC tableB16_2[96] = 
{ 
	{18, 10},  {17, 10},  {4993, 9},  {4993, 9},  {4929, 9},  {4929, 9},  {4865, 9}, {4865, 9}, {4801, 9}, {4801, 9},
	{4737, 9}, {4737, 9}, {4162, 9},  {4162, 9},  {4100, 9},  {4100, 9},  {769, 9},  {769, 9},  {705, 9},  {705, 9},
	{450, 9},  {450, 9},  {386, 9},   {386, 9},   {322, 9},   {322, 9},   {195, 9},  {195, 9},  {131, 9},  {131, 9},
	{70, 9},   {70, 9},   {69, 9},    {69, 9},    {16, 9},    {16, 9},    {258, 9},  {258, 9},  {15, 9},   {15, 9},
	{14, 9},   {14, 9},   {13, 9},    {13, 9},    {4609, 8},  {4609, 8},  {4609, 8}, {4609, 8}, {4545, 8}, {4545, 8},
	{4545, 8}, {4545, 8}, {4481, 8},  {4481, 8},  {4481, 8},  {4481, 8},  {4099, 8}, {4099, 8}, {4099, 8}, {4099, 8},
	{641, 8},  {641, 8},  {641, 8},   {641, 8},   {577, 8},   {577, 8},   {577, 8},  {577, 8},  {513, 8},  {513, 8},
	{513, 8},  {513, 8},  {4673, 8},  {4673, 8},  {4673, 8},  {4673, 8},  {194, 8},  {194, 8},  {194, 8},  {194, 8},
	{68, 8},   {68, 8},   {68, 8},    {68, 8},    {12, 8},    {12, 8},    {12, 8},   {12, 8},   {11, 8},   {11, 8},
	{11, 8},   {11, 8},   {10, 8},    {10, 8},    {10, 8},    {10, 8}
};

static const VLC tableB16_3[120] = 
{ 
	{4103, 11}, {4103, 11}, {4102, 11}, {4102, 11},  {22, 11},   {22, 11},   {21, 11},   {21, 11},   {4226, 10},  {4226, 10},
	{4226, 10}, {4226, 10}, {4163, 10}, {4163, 10},  {4163, 10}, {4163, 10}, {4101, 10}, {4101, 10}, {4101, 10},  {4101, 10},
	{833, 10},  {833, 10},  {833, 10},  {833, 10},   {323, 10},  {323, 10},  {323, 10},  {323, 10},  {514, 10},   {514, 10},
	{514, 10},  {514, 10},  {259, 10},  {259, 10},   {259, 10},  {259, 10},  {196, 10},  {196, 10},  {196, 10},   {196, 10},
	{132, 10},  {132, 10},  {132, 10},  {132, 10},   {71, 10},   {71, 10},   {71, 10},   {71, 10},   {20, 10},    {20, 10},
	{20, 10},   {20, 10},   {19, 10},   {19, 10},    {19, 10},   {19, 10},   {23, 11},   {23, 11},   {24, 11},    {24, 11},
	{72, 11},   {72, 11},   {578, 11},  {578, 11},   {4290, 11}, {4290, 11}, {4354, 11}, {4354, 11}, {5057, 11},  {5057, 11},
	{5121, 11}, {5121, 11}, {25, 12},   {26, 12},    {27, 12},   {73, 12},   {387, 12},  {74, 12},   {133, 12},   {451, 12},
	{897, 12},  {4104, 12}, {4418, 12}, {4482, 12},  {5185, 12}, {5249, 12}, {5313, 12}, {5377, 12}, {7167, 7},   {7167, 7},
	{7167, 7},  {7167, 7},  {7167, 7},  {7167, 7},   {7167, 7},  {7167, 7},  {7167, 7},  {7167, 7},  {7167, 7},   {7167, 7},
	{7167, 7},  {7167, 7},  {7167, 7},  {7167, 7},   {7167, 7},  {7167, 7},  {7167, 7},  {7167, 7},  {7167, 7},   {7167, 7},
	{7167, 7},  {7167, 7},  {7167, 7},  {7167, 7},   {7167, 7},  {7167, 7},  {7167, 7},  {7167, 7},  {7167, 7},   {7167, 7}
};

// tables to decode Table B17 VLC

static const VLC tableB17_1[112] = 
{ 
	{4225,7}, {4209,7}, {4193,7}, {4177,7}, {193,7},  {177,7},  {161,7},  {4,7},    {4161,6}, {4161,6},
	{4145,6}, {4145,6}, {4129,6}, {4129,6}, {4113,6}, {4113,6}, {145,6},  {145,6},  {129,6},  {129,6},
	{113,6},  {113,6},  {97,6},   {97,6},   {18,6},   {18,6},   {3,6},    {3,6},    {81,5},   {81,5},
	{81,5},   {81,5},   {65,5},   {65,5},   {65,5},   {65,5},   {49,5},   {49,5},   {49,5},   {49,5},
	{4097,4}, {4097,4}, {4097,4}, {4097,4}, {4097,4}, {4097,4}, {4097,4}, {4097,4}, {1,2},    {1,2},
	{1,2},    {1,2},    {1,2},    {1,2},    {1,2},    {1,2},    {1,2},    {1,2},    {1,2},    {1,2}, 
	{1,2},    {1,2},    {1,2},    {1,2},    {1,2},    {1,2},    {1,2},    {1,2},    {1,2},    {1,2}, 
	{1,2},    {1,2},    {1,2},    {1,2},    {1,2},    {1,2},    {1,2},    {1,2},    {1,2},    {1,2}, 
	{17,3},   {17,3},   {17,3},   {17,3},   {17,3},   {17,3},   {17,3},   {17,3},   {17,3},   {17,3}, 
	{17,3},   {17,3},   {17,3},   {17,3},   {17,3},   {17,3},   {33,4},   {33,4},   {33,4},   {33,4}, 
	{33,4},   {33,4},   {33,4},   {33,4},   {2,4},    {2,4},    {2,4},    {2,4},    {2,4},    {2,4},
	{2,4},    {2,4}
};


static const VLC tableB17_2[96] = 
{ 
	{9,10},   {8,10},   {4481,9}, {4481,9}, {4465,9}, {4465,9}, {4449,9}, {4449,9}, {4433,9}, {4433,9}, 
	{4417,9}, {4417,9}, {4401,9}, {4401,9}, {4385,9}, {4385,9}, {4369,9}, {4369,9}, {4098,9}, {4098,9}, 
	{353,9},  {353,9},  {337,9},  {337,9},  {321,9},  {321,9},  {305,9},  {305,9},  {289,9},  {289,9},
	{273,9},  {273,9},  {257,9},  {257,9},  {241,9},  {241,9},  {66,9},   {66,9},   {50,9},   {50,9},
	{7,9},    {7,9},    {6,9},    {6,9},    {4353,8}, {4353,8}, {4353,8}, {4353,8}, {4337,8}, {4337,8}, 
	{4337,8}, {4337,8}, {4321,8}, {4321,8}, {4321,8}, {4321,8}, {4305,8}, {4305,8}, {4305,8}, {4305,8}, 
	{4289,8}, {4289,8}, {4289,8}, {4289,8}, {4273,8}, {4273,8}, {4273,8}, {4273,8}, {4257,8}, {4257,8}, 
	{4257,8}, {4257,8}, {4241,8}, {4241,8}, {4241,8}, {4241,8}, {225,8},  {225,8},  {225,8},  {225,8}, 
	{209,8},  {209,8},  {209,8},  {209,8},  {34,8},   {34,8},   {34,8},   {34,8},   {19,8},   {19,8},
	{19,8},   {19,8},   {5,8},    {5,8},    {5,8},    {5,8}
};


static const VLC tableB17_3[120] = 
{ 
	{4114,11}, {4114,11}, {4099,11}, {4099,11}, {11,11},   {11,11},   {10,11},   {10,11},   {4545,10}, {4545,10}, 
	{4545,10}, {4545,10}, {4529,10}, {4529,10}, {4529,10}, {4529,10}, {4513,10}, {4513,10}, {4513,10}, {4513,10},
	{4497,10}, {4497,10}, {4497,10}, {4497,10}, {146,10},  {146,10},  {146,10},  {146,10},  {130,10},  {130,10}, 
	{130,10},  {130,10},  {114,10},  {114,10},  {114,10},  {114,10},  {98,10},   {98,10},   {98,10},   {98,10},
	{82,10},   {82,10},   {82,10},   {82,10},   {51,10},   {51,10},   {51,10},   {51,10},   {35,10},   {35,10},
	{35,10},   {35,10},   {20,10},   {20,10},   {20,10},   {20,10},   {12,11},   {12,11},   {21,11},   {21,11},
	{369,11},  {369,11},  {385,11},  {385,11},  {4561,11}, {4561,11}, {4577,11}, {4577,11}, {4593,11}, {4593,11},
	{4609,11}, {4609,11}, {22,12},   {36,12},   {67,12},   {83,12},   {99,12},   {162,12},  {401,12},  {417,12},
	{4625,12}, {4641,12}, {4657,12}, {4673,12}, {4689,12}, {4705,12}, {4721,12}, {4737,12}, {7167,7},  {7167,7},
	{7167,7},  {7167,7},  {7167,7},  {7167,7},  {7167,7},  {7167,7},  {7167,7},  {7167,7},  {7167,7},  {7167,7},
	{7167,7},  {7167,7},  {7167,7},  {7167,7},  {7167,7},  {7167,7},  {7167,7},  {7167,7},  {7167,7},  {7167,7},
	{7167,7},  {7167,7},  {7167,7},  {7167,7},  {7167,7},  {7167,7},  {7167,7},  {7167,7},  {7167,7},  {7167,7}
};


/* shift to mbcoding later */

static const uint16_t scan_tables[3][64] =
{
	{	// zig_zag_scan
	    0,	1,	8,	16, 9,	2,	3,	10,
		17, 24, 32, 25, 18, 11, 4,	5,
		12, 19, 26, 33, 40, 48, 41, 34,
		27, 20, 13, 6,	7,	14, 21, 28,
		35, 42, 49, 56, 57, 50, 43, 36,
		29, 22, 15, 23, 30, 37, 44, 51,
		58, 59, 52, 45, 38, 31, 39, 46,
		53, 60, 61, 54, 47, 55, 62, 63
	},

	{	// horizontal_scan
	    0,	1,	2,	3,	8,	9,	16, 17,
	    10, 11,	4,	5,	6,	7,	15, 14,
	    13, 12, 19, 18, 24, 25, 32, 33,
	    26, 27, 20, 21, 22, 23, 28, 29,
	    30, 31, 34, 35, 40, 41, 48, 49,
	    42, 43, 36, 37, 38, 39, 44, 45,
	    46, 47, 50, 51, 56, 57, 58, 59,
	    52, 53, 54, 55, 60, 61, 62, 63
	},
	
	{	// vertical_scan
	    0, 8, 16, 24, 1, 9, 2, 10,
	    17, 25, 32, 40, 48, 56, 57, 49,
	    41, 33, 26, 18, 3, 11, 4, 12,
	    19, 27, 34, 42, 50, 58, 35, 43,
	    51, 59, 20, 28, 5, 13, 6, 14,
	    21, 29, 36, 44, 52, 60, 37, 45,
	    53, 61, 22, 30, 7, 15, 23, 31,
	    38, 46, 54, 62, 39, 47, 55, 63
	}
};


/* Table B-19 -- ESCL(a), LMAX values of intra macroblocks */
int get_b19_lmax(int last, int run) {
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



/* Table B-20 -- ESCL(b), LMAX values of inter macroblocks */
int get_b20_lmax(int last, int run) {
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
int get_b21_rmax(int last, int level) {
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

int get_b22_rmax(int last, int level) {
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



const VLC * get_b16_table(uint32_t index) 
{
	if (index >= 512) 
	{
		return &tableB16_1[(index >> 5) - 16];
	}

	if (index >= 128) 
	{
		return &tableB16_2[(index >> 2) - 32];
	}

	if (index >= 8) 
	{
		return &tableB16_3[index - 8];
	} 

	return NULL;
}




const VLC * get_b17_table(uint32_t index) 
{
	if (index >= 512) 
	{
		return &tableB17_1[(index >> 5) - 16];
	}
	
	if (index >= 128) 
	{
		return &tableB17_2[(index >> 2) - 32];
	}
	
	if (index >= 8) 
	{
		return &tableB17_3[index - 8];
	}

	return NULL;
}




int get_mcbpc_intra(BITREADER * bs)
{
	uint32_t index;
	
	while ( (index = bs_show(bs, 9)) == 1 )
	{
		bs_skip(bs, 9);
	}

	if (index < 8) {
		return -1;
	}

	index >>= 3;
	if (index >= 32) {
		bs_skip(bs, 1);
		return 3;
	}
	
	bs_skip(bs, mcbpc_intra_table[index].len);

	return mcbpc_intra_table[index].code;
}



int get_mcbpc_inter(BITREADER * bs)
{
	uint32_t index;
	
	while ( (index = bs_show(bs, 9)) == 1 )
	{
		bs_skip(bs, 9);
	}

	if (index == 0)  {
		return -1;
	}

	if (index >= 256)
	{
		bs_skip(bs, 1);
		return 0;
	}

    bs_skip(bs,  mcbpc_inter_table[index].len);
	return mcbpc_inter_table[index].code;
}




int get_cbpy(BITREADER * bs, int intra)
{
	int cbpy;
	uint32_t index = bs_show(bs, 6);

	if (index < 2) {
		return -1;
	}
			  
	if (index >= 48) {
		bs_skip(bs, 2);
		cbpy = 15;
	} else {
		bs_skip(bs, cbpy_table[index].len);
		cbpy = cbpy_table[index].code;
	}

	if (!intra)
	{
		cbpy = 15 - cbpy;
	}

	return cbpy;
}


int get_mv_data(BITREADER * bs)
{
	uint32_t index;

	if (bs_get1(bs)) {
		return 0;
	}
	
	index = bs_show(bs, 12);

	if (index >= 512)
	{
		index = (index >> 8) - 2;
		bs_skip(bs, MVtab0[index].len);
		return MVtab0[index].code;
	}
	
	if (index >= 128)
	{
		index = (index >> 2) - 32;
		bs_skip(bs, MVtab1[index].len);
		return MVtab1[index].code;
	}

	index -= 4; 

	bs_skip(bs, MVtab2[index].len);
	return MVtab2[index].code;
}


#define ABS(X) (((X)>0)?(X):-(X))

int get_mv(BITREADER * bs, int fcode)
{
	int data;
	int res;
	int mv;
	int scale_fac = 1 << (fcode - 1);

	data = get_mv_data(bs);
	
	if (scale_fac == 1 || data == 0)
	{
		return data;
	}

	res = bs_get(bs, fcode - 1);
	mv = ((ABS(data) - 1) * scale_fac) + res + 1;
	
	return data < 0 ? -mv : mv;
}



int get_dc_dif(BITREADER * bs, uint32_t dc_size)
{
	int code = bs_get(bs, dc_size);
	int msb = code >> (dc_size - 1);

	if (msb == 0) {
		return (-1 * (code^((1 << dc_size) - 1)));
	}
	return code;
}


int get_dc_size_lum(BITREADER * bs)
{
	int code;

	if (bs_show(bs, 11) == 1) {
		bs_skip(bs, 11);
		return 12;
	}
	if (bs_show(bs, 10) == 1) {
		bs_skip(bs, 10);
		return 11;
	}
	if (bs_show(bs, 9) == 1) {
		bs_skip(bs, 9);
		return 10;
	}
	if (bs_show(bs, 8) == 1) {
		bs_skip(bs, 8);
		return 9;
	}
	if (bs_show(bs, 7) == 1) {
		bs_skip(bs, 7);
		return 8;
	}
	if (bs_show(bs, 6) == 1) {
		bs_skip(bs, 6);
		return 7;
	}  
	if (bs_show(bs, 5) == 1) {
		bs_skip(bs, 5);
		return 6;
	}
	if (bs_show(bs, 4) == 1) {
		bs_skip(bs, 4);
		return 5;
	}

	code = bs_show(bs, 3);

	if (code == 1) {
		bs_skip(bs, 3);
		return 4;
	} else if (code == 2) {
		bs_skip(bs, 3);
		return 3;
	} else if (code == 3) {
		bs_skip(bs, 3);
		return 0;
	}

	code = bs_show(bs, 2);

	if (code == 2) {
		bs_skip(bs, 2);
		return 2;
	} else if (code == 3) {
		bs_skip(bs, 2);
		return 1;
	}     

	return 0;
}


int get_dc_size_chrom(BITREADER * bs)
{
	if (bs_show(bs, 12) == 1) {
		bs_skip(bs, 12);
		return 12;
	}
	if (bs_show(bs, 11) == 1) {
		bs_skip(bs, 11);
		return 11;
	}
	if (bs_show(bs, 10) == 1) {
		bs_skip(bs, 10);
		return 10;
	}
	if (bs_show(bs, 9) == 1) {
		bs_skip(bs, 9);
		return 9;
	}
	if (bs_show(bs, 8) == 1) {
		bs_skip(bs, 8);
		return 8;
	}
	if (bs_show(bs, 7) == 1) {
		bs_skip(bs, 7);
		return 7;
	}
	if (bs_show(bs, 6) == 1) {
		bs_skip(bs, 6);
		return 6;
	}
	if (bs_show(bs, 5) == 1) {
		bs_skip(bs, 5);
		return 5;
	}
	if (bs_show(bs, 4) == 1) {
		bs_skip(bs, 4);
		return 4;
	} 
	if (bs_show(bs, 3) == 1) {
		bs_skip(bs, 3);
		return 3;
	} 

	return 3 - bs_get(bs, 2);
}


// coeff stuff


int get_intra_coeff(BITREADER * bs, int *run, int *last) 
{
	const VLC * tab = NULL;
	int level;

	tab = get_b16_table(bs_show(bs, 12));
	if (!tab) {
		*run = -1;	/* error */
		return 0;
	}
	bs_skip(bs, tab->len);

	if (tab->code != ESCAPE) {
		*run = (tab->code >>  6) & 63;
		*last = (tab->code >> 12) &  1;
		level =  tab->code        & 63;
		return bs_get1(bs) ? -level : level;
	}

	/* this value is escaped - see para 7.4.1.3 */
	/* assuming short_video_header == 0 */
	switch (bs_show(bs, 2)) {
		case 0x0 :
		case 0x1 :
			bs_skip(bs, 1);
			tab = get_b16_table(bs_show(bs, 12));
			if (!tab) {
				*run = -1;	/* error */
				return 0;
			}
			bs_skip(bs, tab->len);
			*run =  (tab->code >>  6) & 63;
			level =  tab->code        & 63;
			*last = (tab->code >> 12) &  1;
			level += get_b19_lmax(*last, *run);
			return bs_get1(bs) ? -level : level;

		case 0x2 :
			bs_skip(bs, 2);
			tab = get_b16_table(bs_show(bs, 12));
			if (!tab) {
				*run = -1;	/* error */
				return 0;
			}
			bs_skip(bs, tab->len);
			*run   = (tab->code >>  6) & 63;
			level =  tab->code        & 63;
			*last  = (tab->code >> 12) &  1;
			*run += get_b21_rmax(*last, level) + 1;
			return bs_get1(bs) ? -level : level;

		case 0x3 :
			bs_skip(bs, 2);
			*last = bs_get(bs, 1);
			*run = bs_get(bs, 6);			// table B-18
			bs_skip(bs, 1);					// marker
			level = bs_get(bs, 12);			// table B-18
			bs_skip(bs, 1);					// marker
			return (level & 0x800) ? (level | (-1 ^ 0xfff)) : level;
	}

	*run = -1;	/* error */
	return 0;
}



int get_inter_coeff(BITREADER * bs, int *run, int *last) 
{
	const VLC * tab = NULL;
	int level;

	tab = get_b17_table(bs_show(bs, 12));
	if (!tab)
	{ 
		*run = -1;	/* error */
		return 0;
	} 
	bs_skip(bs, tab->len);
	
	if (tab->code != ESCAPE) {
		*run = (tab->code >>  4) & 255;
		level = tab->code        & 15;
		*last = (tab->code >> 12) &  1;
		return bs_get1(bs) ? -level : level;
	}

	/* this value is escaped - see para 7.4.1.3 */
	/* assuming short_video_header == 0 */
	switch (bs_show(bs, 2)) {
		case 0x0 :  /* Type 1 */
		case 0x1 :  /* Type 1 */
			
			bs_skip(bs, 1);
			tab = get_b17_table(bs_show(bs, 12));  /* use table B-17 */
			if (!tab) {
				*run = -1;	/* error */
				return 0;
			}
			bs_skip(bs, tab->len);
			
			*run   = (tab->code >>  4) & 255;
			level =  tab->code        & 15;
			*last  = (tab->code >> 12) &  1;
			level += get_b20_lmax(*last, *run);
			return bs_get1(bs) ? -level : level;

		case 0x2 :  /* Type 2 */
			bs_skip(bs, 2);
			tab = get_b17_table(bs_show(bs, 12));
			if (!tab) {
				*run = -1;	/* error */
				return 0;
			}
			bs_skip(bs, tab->len);
			*run   = (tab->code >>  4) & 255;
			level =  tab->code        & 15;
			*last  = (tab->code >> 12) &  1;
			*run += get_b22_rmax(*last, level) + 1;
			return bs_get1(bs) ? -level : level;
			
		case 0x3 :  /* Type 3  - fixed length codes */
			bs_skip(bs, 2);
			*last  = bs_get1(bs);
			*run  = bs_get(bs, 6);  /* table B-18 */ 
			bs_skip(bs, 1);         /* marker bit */
			level = bs_get(bs, 12); /* table B-18 */
			bs_skip(bs, 1);          /* marker bit */
			return (level & 0x800) ? (level | (-1 ^ 0xfff)) : level;
	}

	*run = -1;	/* error */
	return 0;
}


/*---[end projectmayo code]------------------------------------------ */



void get_intra_block(BITREADER * bs, int16_t * block, int direction) 
{
	const uint16_t * scan = scan_tables[ direction ];
	int p;
	int level;
	int run;
	int last;

	p = 1;
	do
	{
		level = get_intra_coeff(bs, &run, &last);
		if (run == -1)
		{
			DEBUG("fatal: invalid run");
			break;
		}
		p += run;
		block[ scan[p] ] = level;
		if (level < -127 || level > 127)
		{
			DEBUG1("warning: intra_overflow", level);
		}
		p++;
	} while (!last);
}




void get_inter_block(BITREADER * bs, int16_t * block) 
{
	const uint16_t * scan = scan_tables[0];
	int p;
	int level;
	int run;
	int last;

	p = 0;
	do
	{
		level = get_inter_coeff(bs, &run, &last);
		if (run == -1)
		{
			DEBUG("fatal: invalid run");
			break;
		}
		p += run;
		block[ scan[p] ] = level;
		if (level < -127 || level > 127)
		{
			DEBUG1("warning: inter_overflow", level);
		}
		p++;
	} while (!last);
}


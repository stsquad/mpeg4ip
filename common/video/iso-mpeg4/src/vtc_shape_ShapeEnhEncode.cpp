/****************************************************************************/
/*   MPEG4 Visual Texture Coding (VTC) Mode Software                        */
/*                                                                          */
/*   This software was jointly developed by the following participants:     */
/*                                                                          */
/*   Single-quant,  multi-quant and flow control                            */
/*   are provided by  Sarnoff Corporation                                   */
/*     Iraj Sodagar   (iraj@sarnoff.com)                                    */
/*     Hung-Ju Lee    (hjlee@sarnoff.com)                                   */
/*     Paul Hatrack   (hatrack@sarnoff.com)                                 */
/*     Shipeng Li     (shipeng@sarnoff.com)                                 */
/*     Bing-Bing Chai (bchai@sarnoff.com)                                   */
/*     B.S. Srinivas  (bsrinivas@sarnoff.com)                               */
/*                                                                          */
/*   Bi-level is provided by Texas Instruments                              */
/*     Jie Liang      (liang@ti.com)                                        */
/*                                                                          */
/*   Shape Coding is provided by  OKI Electric Industry Co., Ltd.           */
/*     Zhixiong Wu    (sgo@hlabs.oki.co.jp)                                 */
/*     Yoshihiro Ueda (yueda@hlabs.oki.co.jp)                               */
/*     Toshifumi Kanamaru (kanamaru@hlabs.oki.co.jp)                        */
/*                                                                          */
/*   OKI, Sharp, Sarnoff, TI and Microsoft contributed to bitstream         */
/*   exchange and bug fixing.                                               */
/*                                                                          */
/*   Scalable Shape Coding was provided by:                                 */
/*         Shipeng Li (Sarnoff Corporation),                                */
/*         Dae-Sung Cho (Samsung AIT),					    */
/*         Se Hoon Son (Samsung AIT)	                                    */
/* In the course of development of the MPEG-4 standard, this software       */
/* module is an implementation of a part of one or more MPEG-4 tools as     */
/* specified by the MPEG-4 standard.                                        */
/*                                                                          */
/* The copyright of this software belongs to ISO/IEC. ISO/IEC gives use     */
/* of the MPEG-4 standard free license to use this  software module or      */
/* modifications thereof for hardware or software products claiming         */
/* conformance to the MPEG-4 standard.                                      */
/*                                                                          */
/* Those intending to use this software module in hardware or software      */
/* products are advised that use may infringe existing  patents. The        */
/* original developers of this software module and their companies, the     */
/* subsequent editors and their companies, and ISO/IEC have no liability    */
/* and ISO/IEC have no liability for use of this software module or         */
/* modification thereof in an implementation.                               */
/*                                                                          */
/* Permission is granted to MPEG members to use, copy, modify,              */
/* and distribute the software modules ( or portions thereof )              */
/* for standardization activity within ISO/IEC JTC1/SC29/WG11.              */
/*                                                                          */
/* Copyright 1995, 1996, 1997, 1998 ISO/IEC                                 */
/****************************************************************************/
/*****************************************************************************
 "This software module was originally developed by:
 
	Dae-Sung Cho (Samsung AIT),
	Se Hoon Son (Samsung AIT)
 
        and edited by:
 
	Dae-Sung Cho (Samsung AIT)
	Modified by Shipeng Li, Sarnoff Corporation 12/98.

        in the course of development of the <MPEG-4 Video(ISO/IEC 14496-2)>. This
  software module is an implementation of a part of one or more <MPEG-4 Video
  (ISO/IEC 14496-2)> tools as specified by the <MPEG-4 Video(ISO/IEC 14496-2)
  >. ISO/IEC gives users of the <MPEG-4 Video(ISO/IEC 14496-2)> free license
  to this software module or modifications thereof for use in hardware or
  software products claiming conformance to the <MPEG-4 Video(ISO/IEC 14496-2
  )>. Those intending to use this software module in hardware or software
  products are advised that its use may infringe existing patents. The
  original developer of this software module and his/her company, the
  subsequent editors and their companies, and ISO/IEC have no liability for
  use of this software module or modifications thereof in an implementation.
  Copyright is not released for non <MPEG-4 Video(ISO/IEC 14496-2)>
  conforming products. Samsung AIT (SAIT) retains full right to use the code
  for his/her own purpose, assign or donate the code to a third party and to
  inhibit third parties from using the code for non <MPEG-4 Video(ISO/IEC
  14496-2)> conforming products. This copyright notice must be included in
  all copies or derivative works. Copyright (c) 1997, 1998".
 *****************************************************************************/

#include <stdio.h>
//#include <malloc.h>
#include <memory.h>
#include <math.h>
#include "dataStruct.hpp"
//#include "ShapeBaseCommon.h"
//#include "BinArCodec.h"
//#include "dwt.h"
#include "ShapeEnhDef.hpp"
//#include "ShapeEnhCommon.h"
//#include "ShapeEnhEncode.h"
static  unsigned int scalable_bab_type_prob[2]={59808,44651};
// ~modified for FDAM1 by Samsung AIT  on 2000/02/03

/* Probability model for P1 pixel of Interleaved Scan-Line (ISL) coding */
static  unsigned int scalable_xor_prob_1[128]={
65476,64428,62211,63560,52253,58271,38098,31981,
50087,41042,54620,31532,8382,10754,3844,6917,
63834,50444,50140,63043,58093,45146,36768,13351,
17594,28777,39830,38719,9768,21447,12340,9786,
60461,41489,27433,53893,47246,11415,13754,24965,
51620,28011,11973,29709,13878,22794,24385,1558,
57065,41918,25259,55117,48064,12960,19929,5937,
25730,22366,5204,32865,3415,14814,6634,1155,
64444,62907,56337,63144,38112,56527,40247,37088,
60326,45675,51248,15151,18868,43723,14757,11721,
62436,50971,51738,59767,49927,50675,38182,24724,
48447,47316,56628,36336,12264,25893,24243,5358,
58717,56646,48302,60515,36497,26959,43579,40280,
54092,20741,10891,7504,8109,30840,6772,4090,
59810,61410,53216,64127,32344,12462,23132,19270,
32232,24774,9615,17750,1714,6539,3237,152
};

/* Probability model for P2/P3 pixels of Interleaved Scan-Line (ISL) coding */
static  unsigned int scalable_xor_prob_23[128]={
65510,63321,63851,62223,64959,62202,63637,48019,
57072,33553,37041,9527,53190,50479,54232,12855,
62779,63980,49604,31847,57591,64385,40657,8402,
33878,54743,17873,8707,34470,54322,16702,2192,
58325,48447,7345,31317,45687,44236,16685,24144,
34327,18724,10591,24965,9247,7281,3144,5921,
59349,33539,11447,5543,58082,48995,35630,10653,
7123,15893,23830,800,3491,15792,8930,905,
65209,63939,52634,62194,64937,53948,60081,46851,
56157,50930,35498,24655,56331,59318,32209,6872,
59172,64273,46724,41200,53619,59022,37941,20529,
55026,52858,26402,45073,57740,55485,20533,6288,
64286,55438,16454,55656,61175,45874,28536,53762,
58056,21895,5482,39352,32635,21633,2137,4016,
58490,14100,18724,10461,53459,15490,57992,15128,
12034,4340,6761,1859,5794,6785,2412,35
};

/* Probability model for Raster Scan-Line (RSL) coding (odd filter) */
#if 0
static  unsigned int scalable_full_odd_prob[256]={
65535,32768,32768,32768,32768,32768,32768,32768,
32768,32768,32768,32768,32768,32768,32768,32768,
63094,32768,32768,32768,32768,32768,32768,32768,
32768,32768,32768,32768,32768,32768,32768,32768,
62401,45272,32768,32768,32768,32768,32768,32768,
43096,25726,32768,32768,32768,32768,32768,32768,
55308,17735,32768,32768,32768,32768,32768,32768,
47493,17490,32768,32768,32768,32768,32768,32768,
59542,32768,60740,32768,23847,32768,49123,32768,
36976,32768,44280,32768,24293,32768,26276,32768,
58543,32768,50320,32768,25766,32768,11486,32768,
58465,32768,51357,32768,22795,32768,11805,32768,
49265,12891,27134,10923,20307,32768,12170,7016,
48182,33695,30544,32433,28855,32768,24901,32768,
46015,19178,28001,9729,25880,32768,5913,3070,
33736,32373,33972,21627,26263,32768,19795,32768,
59127,36942,44697,9709,46286,32740,28555,8727,
63736,52206,29927,26312,60272,32005,44761,9988,
47369,17677,48608,19114,19581,30088,25936,24690,
52287,35610,27173,20869,30916,23533,38614,10446,
22346,40972,22360,9590,7680,20243,1983,1754,
48889,45804,22662,27924,28868,9882,14438,1369,
25195,25948,21631,13785,10822,22192,5742,11770,
36752,23147,11313,20607,40693,5351,25436,2752,
44934,6118,32944,12477,26847,28553,32857,12688,
28532,12091,43573,30108,35939,14785,45888,5319,
33566,8681,19072,16384,19175,30670,24431,13374,
28505,28099,44921,39479,39766,19313,18418,4159,
9896,10051,6277,10190,8937,14547,7725,4412,
15503,20210,17744,27014,23609,6523,34098,2309,
7797,4278,2573,5564,11111,18030,2225,2277,
9345,12206,15307,19068,40775,3644,23776,171
};

/* Probability model for Raster Scan-Line (RSL) coding (even filter) */
static  unsigned int scalable_full_even_prob[256]={
32768,32768,32768,32768,32768,32768,32768,32768,
32768,32768,32768,32768,32768,32768,32768,32768,
32768,32768,32768,32768,32768,32768,32768,32768,
32768,32768,32768,32768,32768,32768,32768,32768,
32768,32768,32768,32768,32768,32768,32768,32768,
32768,32768,32768,32768,32768,32768,32768,32768,
32768,32768,32768,32768,32768,32768,32768,32768,
32768,32768,32768,32768,32768,32768,32768,32768,
32768,32768,32768,32768,32768,32768,32768,32768,
32768,32768,32768,32768,32768,32768,32768,32768,
32768,32768,32768,32768,32768,32768,32768,32768,
32768,32768,32768,32768,32768,32768,32768,32768,
32768,32768,32768,32768,32768,32768,32768,32768,
32768,32768,32768,32768,32768,32768,32768,32768,
32768,32768,32768,32768,32768,32768,32768,32768,
32768,32768,32768,32768,32768,32768,32768,32768,
62895,39067,48704,11763,59082,16056,35655,2552,
65472,58570,43137,54459,63589,30402,51995,8506,
48059,25600,39891,20480,48289,20480,33704,862,
65398,61538,32768,56173,63332,33544,48817,6824,
61680,46422,20947,16531,30989,2969,5053,776,
64799,44586,34078,38272,54776,7627,40104,780,
61189,32780,58998,18264,30111,2006,35761,1711,
62403,22718,58513,35075,61813,4541,54577,1947,
51659,5136,57698,11683,42523,3223,40941,2976,
63801,16281,60527,34885,56447,13256,45226,3402,
49905,9884,61363,20249,31043,3318,33591,1543,
63194,42118,62721,56056,52039,25321,20015,5030,
46814,6017,19267,4864,32768,3855,13296,652,
56912,23661,34133,21197,31232,7271,39021,1022,
52334,8542,36773,4841,15334,1219,4704,568,
55692,15947,43184,25700,43772,3666,17184,182
};
#endif


static UInt prob_odd0[256]={
65536, 65536, 65536, 65536, 65536, 65536, 65536, 65536,
25682, 29649, 83, 1096, 28876, 19253, 2320, 292,
65536, 65536, 65536, 65536, 65536, 65536, 65536, 65536,
20597, 40071, 1, 1476, 6971, 9466, 4033, 314,
65536, 65536, 65536, 65536, 65536, 65536, 65536, 65536,
34822, 58944, 1049, 888, 47296, 38677, 478, 14,
65536, 65536, 65536, 65536, 65536, 65536, 65536, 65536,
21384, 35003, 26, 7, 9600, 13044, 44, 1,
65536, 65536, 65536, 65536, 65536, 65536, 65536, 65536,
66, 13653, 1, 1, 25105, 19008, 285, 31,
65536, 65536, 65536, 65536, 65536, 65536, 65536, 65536,
1057, 27888, 1, 2521, 4252, 15367, 1, 751,
65536, 65536, 65536, 65536, 65536, 65536, 65536, 65536,
290, 17476, 1, 1, 8625, 20056, 121, 3,
65536, 65536, 65536, 65536, 65536, 65536, 65536, 65536,
1960, 15984, 1, 1, 4747, 16480, 110, 9,
65536, 65536, 65536, 65536, 65536, 65536, 65536, 65536,
87, 140, 1, 55, 1205, 1864, 1, 1,
65536, 65536, 65536, 65536, 65536, 65536, 65536, 65536,
549, 2823, 1, 1, 1902, 782, 1285, 130,
65536, 65536, 65536, 65536, 65536, 65536, 65536, 65536,
290, 6665, 1, 1, 15984, 8656, 1, 244,
65536, 65536, 65536, 65536, 65536, 65536, 65536, 65536,
63, 2114, 1, 3, 2138, 1560, 69, 1,
65536, 65536, 65536, 65536, 65536, 65536, 65536, 65536,
1, 3178, 1, 218, 160, 66, 1, 2,
65536, 65536, 65536, 65536, 65536, 65536, 65536, 65536,
35, 7047, 366, 180, 34, 113, 1, 9,
65536, 65536, 65536, 65536, 65536, 65536, 65536, 65536,
837, 15124, 1, 1, 5571, 3236, 1, 2,
65536, 65536, 65536, 65536, 65536, 65536, 65536, 65536,
14, 1538, 1, 14, 31, 115, 1, 1
};
static UInt prob_odd1[256]={
65536, 65536, 54236, 62309, 65536, 65536, 39146, 32459,
0, 0, 0, 0, 0, 0, 0, 0,
65536, 65536, 3293, 20806, 65536, 65536, 20132, 22080,
0, 0, 0, 0, 0, 0, 0, 0,
65536, 65536, 62837, 40750, 65536, 65536, 36528, 20960,
0, 0, 0, 0, 0, 0, 0, 0,
65536, 65536, 46875, 21025, 65536, 65536, 35747, 30778,
0, 0, 0, 0, 0, 0, 0, 0,
65536, 65536, 63956, 52735, 65536, 65536, 32974, 6233,
0, 0, 0, 0, 0, 0, 0, 0,
65536, 65536, 39449, 40885, 65536, 65536, 23406, 10898,
0, 0, 0, 0, 0, 0, 0, 0,
65536, 65536, 62156, 12655, 65536, 65536, 23973, 3451,
0, 0, 0, 0, 0, 0, 0, 0,
65536, 65536, 31892, 3756, 65536, 65536, 24045, 14281,
0, 0, 0, 0, 0, 0, 0, 0,
65537, 65537, 65537, 65537, 65537, 65537, 65537, 65537,
51966, 33057, 1, 6782, 62238, 32046, 5919, 418,
65537, 65537, 65537, 65537, 65537, 65537, 65537, 65537,
64435, 18941, 1224, 7203, 52134, 4674, 6753, 1113,
65537, 65537, 65537, 65537, 65537, 65537, 65537, 65537,
62703, 34133, 16705, 28007, 31477, 15453, 21558, 6073,
65537, 65537, 65537, 65537, 65537, 65537, 65537, 65537,
61951, 29954, 4826, 6481, 12288, 2410, 4466, 228,
65537, 65537, 65537, 65537, 65537, 65537, 65537, 65537,
1789, 23069, 726, 7470, 30386, 26721, 9811, 1446,
65537, 65537, 65537, 65537, 65537, 65537, 65537, 65537,
45333, 28672, 21363, 26870, 41125, 9455, 25752, 12372,
65537, 65537, 65537, 65537, 65537, 65537, 65537, 65537,
46324, 37071, 4994, 5136, 18879, 28687, 9330, 366,
65537, 65537, 65537, 65537, 65537, 65537, 65537, 65537,
31745, 26116, 346, 397, 2900, 13830, 361, 8
};



static UInt prob_even0[256]={
65536, 65536, 65536, 65536, 65536, 65536, 65536, 65536,
25542, 38525, 48333, 53517, 26541, 23214, 61559, 59853,
65536, 65536, 65536, 65536, 65536, 65536, 65536, 65536,
7151, 31006, 33480, 29879, 2609, 4142, 16384, 11884,
65536, 65536, 65536, 65536, 65536, 65536, 65536, 65536,
44817, 65535, 56097, 64984, 40735, 47710, 43386, 52046,
65536, 65536, 65536, 65536, 65536, 65536, 65536, 65536,
17545, 29626, 37308, 42263, 4196, 13552, 7199, 7230,
65536, 65536, 65536, 65536, 65536, 65536, 65536, 65536,
1, 1, 1, 39322, 13578, 17416, 29218, 31831,
65536, 65536, 65536, 65536, 65536, 65536, 65536, 65536,
1311, 65535, 32768, 7282, 1, 3048, 25206, 19935,
65536, 65536, 65536, 65536, 65536, 65536, 65536, 65536,
1, 32768, 17873, 61861, 3417, 14895, 4541, 5293,
65536, 65536, 65536, 65536, 65536, 65536, 65536, 65536,
4819, 32768, 39950, 43523, 1148, 4021, 12072, 5436,
65536, 65536, 65536, 65536, 65536, 65536, 65536, 65536,
1, 1, 134, 1, 1, 55, 5461, 2849,
65536, 65536, 65536, 65536, 65536, 65536, 65536, 65536,
24, 5243, 590, 1079, 86, 95, 14564, 7159,
65536, 65536, 65536, 65536, 65536, 65536, 65536, 65536,
328, 1, 2564, 14919, 21845, 1, 9362, 15880,
65536, 65536, 65536, 65536, 65536, 65536, 65536, 65536,
43, 362, 150, 1179, 752, 529, 683, 331,
65536, 65536, 65536, 65536, 65536, 65536, 65536, 65536,
1, 1, 700, 862, 25, 24, 1317, 558,
65536, 65536, 65536, 65536, 65536, 65536, 65536, 65536,
1, 1, 1, 172, 2, 4, 793, 342,
65536, 65536, 65536, 65536, 65536, 65536, 65536, 65536,
572, 1, 1928, 43080, 3337, 1680, 1401, 2131,
65536, 65536, 65536, 65536, 65536, 65536, 65536, 65536,
18, 1, 366, 7456, 8, 18, 40, 8
};

static UInt prob_even1[256]={
65536, 65536, 65536, 65536, 65536, 65536, 65536, 65536,
0, 0, 0, 0, 0, 0, 0, 0,
65536, 65536, 65536, 65536, 65536, 65536, 65536, 65536,
0, 0, 0, 0, 0, 0, 0, 0,
65536, 65536, 65536, 65536, 65536, 65536, 65536, 65536,
0, 0, 0, 0, 0, 0, 0, 0,
65536, 65536, 65536, 65536, 65536, 65536, 65536, 65536,
0, 0, 0, 0, 0, 0, 0, 0,
65536, 65536, 65536, 65536, 65536, 65536, 65536, 65536,
0, 0, 0, 0, 0, 0, 0, 0,
65536, 65536, 65536, 65536, 65536, 65536, 65536, 65536,
0, 0, 0, 0, 0, 0, 0, 0,
65536, 65536, 65536, 65536, 65536, 65536, 65536, 65536,
0, 0, 0, 0, 0, 0, 0, 0,
65536, 65536, 65536, 65536, 65536, 65536, 65536, 65536,
0, 0, 0, 0, 0, 0, 0, 0,
65537, 65537, 65537, 65537, 65537, 65537, 65537, 65537,
54018, 8704, 1, 903, 61648, 31196, 327, 575,
65537, 65537, 65537, 65537, 65537, 65537, 65537, 65537,
64400, 36956, 1673, 9758, 52289, 4361, 659, 1433,
65537, 65537, 65537, 65537, 65537, 65537, 65537, 65537,
56482, 65535, 1, 1, 11905, 3034, 1, 1,
65537, 65537, 65537, 65537, 65537, 65537, 65537, 65537,
61103, 56650, 925, 8814, 11845, 2075, 828, 223,
65537, 65537, 65537, 65537, 65537, 65537, 65537, 65537,
4161, 1, 494, 5041, 52508, 32195, 11005, 2463,
65537, 65537, 65537, 65537, 65537, 65537, 65537, 65537,
63286, 32768, 39133, 49486, 53351, 8541, 37603, 15011,
65537, 65537, 65537, 65537, 65537, 65537, 65537, 65537,
18805, 13107, 1039, 1214, 5060, 21845, 3830, 387,
65537, 65537, 65537, 65537, 65537, 65537, 65537, 65537,
31654, 32951, 490, 1496, 2535, 11699, 328, 13
};


/***********************************************************HeaderBegin*******
 *
 * File:        ShapeEnhEncode.c
 *
 * Author:      Samsung AIT
 * Created:     10-13-97
 *
 * Description: Contains functions used to implement scan interleaving
 *                               coding of spatial scalable binary alpha blocks.
 *
 * Copyright (C) 1997, 1998 Samsung AIT All Rights Reserved.                 
 *
 ***********************************************************HeaderEnd*********/


#define Error -1

Int CVTCEncoder::
ShapeEnhEnCoding(UChar *LowShape,	 /* shape mask in the lower layer */
		 UChar *HalfShape, /* shape mask in the half-higher layer */
			UChar *CurShape, /* shape mask in the current layer */
			Int object_width,  /* object_width in the current layer */
			Int object_height,/* object_height in the current layer */
			FILTER *filter)
{
    Int i, j, k, l, p, q, x, y, x2, y2;
    Int bab_type, ret;
	Int scan_order; // SAIT_PDAM : ADDED by D.-S. Cho (Samsung AIT) 

    Int width2		= object_width,
	height2		= object_height,
	width		= width2 >> 1,
	height		= height2 >> 1;
    Int NB = (object_width >= 1024 || object_height >=1024)? 6:
      (object_width >=512|| object_height >=512)?5:4;
    Int mborder		= MBORDER,
    	mblks           = NB,
	mbsize          = 1<<mblks,		/* bab size in the current layer : 16 */
	mbsize_ext      = mbsize+(mborder<<1);  /* bordered bab size in the current layer: 20 */
 
    Int border		= BORDER,
    	blks            = mblks-1,
	bsize           = 1<<blks,		/* bab size in the lower layer : 8 */
	bsize_ext       = bsize+(border<<1);	/* bordered bab size in the lower layer: 8 */
 
    Int blkx            = (object_width+mbsize-1)/mbsize,
	blky            = (object_height+mbsize-1)/mbsize;
    //    static int first=0;
    //    Int cnt_mode0=0, cnt_mode1=0, cnt_mode2=0, cnt_total=0;
    
    UInt prob;

    UChar *lower_bab;			/* alpha block in the lower layer */
    UChar *bordered_lower_bab;		/* bordered alpha block in the lower layer */

    UChar *half_bab;			/* alpha block in the half-higher layer */
    UChar *bordered_half_bab;		/* bordered alpha block in the half-higher layer */
 
    
    UChar *curr_bab;			/* alpha mb in the current layer */
    UChar *bordered_curr_bab;		/* bordered alpha mb in the current layer */

    ArCoder       ar_coder;

	ShapeBitstream=NULL;
	ShapeBitstreamLength=0;
    /*-- Memory allocation ---------------------------------------------------------*/
    lower_bab = (UChar *) calloc(bsize*bsize, sizeof(UChar));
    bordered_lower_bab = (UChar *) calloc(bsize_ext*bsize_ext, sizeof(UChar));

    half_bab = (UChar *) calloc(bsize*mbsize, sizeof(UChar));
    bordered_half_bab = (UChar *) calloc(bsize_ext*mbsize_ext, sizeof(UChar));

    curr_bab = (UChar *) calloc(mbsize*mbsize, sizeof(UChar));
    bordered_curr_bab = (UChar *) calloc(mbsize_ext*mbsize_ext, sizeof(UChar));

    /*-- Initialize the shape bitstream --------------------------------------------*/
    ShapeBitstream = (BSS *)malloc(sizeof(BSS));
    if(ShapeBitstream==NULL) {
      fprintf(stderr,"Memory allocation failed\n");
      return Error;
    }
    ShapeBitstream->bs = (UChar *)malloc(sizeof(UChar)*object_width*object_height);
    if(ShapeBitstream->bs==NULL) {
      fprintf(stderr,"Memory allocation failed\n");
      return Error;
    }

    /*-- Clear the output buffer ---------------------------------------------------*/
    memset(ShapeBitstream->bs, (UChar )0, object_width*object_height);
    InitBitstream(1,ShapeBitstream);

    /*-- Encode the Enhancement Layer ------------------------------------------------*/
/*     fprintf(stderr,"-- Encode the BAB in the enhancement layer !\n"); */

    StartArCoder_Still(&ar_coder);  

    for ( j=y=y2=0; j<blky; j++, y+=bsize, y2+=mbsize ) 
    {
      for ( i=x=x2=0; i<blkx; i++, x+=bsize, x2+=mbsize ) 
      {
	/*-- Initialize BABs --*/
	q = y2*width2;
        for ( l=p=0; l<mbsize; l++, q+=width2 ) {
          for ( k=0; k<mbsize; k++, p++ ) {
	      if( y2+l < height2 && x2+k < width2)
                curr_bab[p]= (CurShape[ q+x2+k ] != 0);
	      else
                curr_bab[p]= 0;
	  }
	}


	q = y2*width;
        for ( l=p=0; l<mbsize; l++, q+=width ) {
          for ( k=0; k<bsize; k++, p++ ) {
	      if( y2+l < height2 && x+k < width)
                half_bab[p]= (HalfShape[ q+x+k ] != 0);
	      else
                half_bab[p]= 0;
	  }
	}


	q = y*width;
	for ( l=p=0; l<bsize; l++, q+=width ) {
	  for ( k=0; k<bsize; k++, p++ ) {
              if(  y+l < height && x+k < width )
                lower_bab[p] = (LowShape[ q+x+k ] != 0);
              else
                lower_bab[p] = 0;

          }
	}

	AddBorderToBABs(LowShape, HalfShape, CurShape,
			lower_bab, half_bab, curr_bab,
			bordered_lower_bab, bordered_half_bab,
			bordered_curr_bab,
			object_width, object_height,
			i, j, mbsize, blkx);

	scan_order = DecideScanOrder(bordered_lower_bab, mbsize); // SAIT_PDAM: ADDED by D.-S.Cho (Samsung AIT) 
	
	bab_type = DecideBABtype(bordered_lower_bab, bordered_half_bab,
				 bordered_curr_bab, mbsize,
				 scan_order // SAIT_PDAM ADDED by D.-S.Cho (Samsung AIT) 
				 );
	if(filter->DWT_Class==DWT_ODD_SYMMETRIC){
	  prob=scalable_bab_type_prob[0];
	} else if(filter->DWT_Class==DWT_EVEN_SYMMETRIC) {
	  prob=scalable_bab_type_prob[1];
	} else {
	  fprintf(stderr,"Error: filter type in ShapeEnhEncoding()!\n");
	  exit(0);
	}
	ArCodeSymbol_Still(&ar_coder, ShapeBitstream, bab_type, prob);

	/* Encode mask pixel values in the enhancement layer */
	ret = ShapeEnhContentEncode(bordered_lower_bab, 
				    bordered_half_bab,
				    bordered_curr_bab, 
				    bab_type,
					scan_order, // SAIT_PDAM: ADDED by D.-S.Cho (Samsung AIT)
				    mbsize, 
				    filter,
				    ShapeBitstream,	
				    &ar_coder);
	if( ret == Error )
	{
          fprintf(stderr,"\n SI arithmetic coding Error !\n");
          return  Error;
        }
      }
    }

    StopArCoder_Still(&ar_coder,ShapeBitstream);

    ShapeBitstreamLength = ShapeBitstream -> cnt;

    /*-- Memory free ---------------------------------------------------------*/
 
    free(lower_bab);
    free(bordered_lower_bab);
    free(half_bab);
    free(bordered_half_bab);
    free(curr_bab);
    free(bordered_curr_bab);

    return ( ShapeBitstreamLength );
}

/* Modified by shson */
Int CVTCEncoder::
DecideBABtype(UChar *bordered_lower_bab, 	
	      UChar *bordered_half_bab, 
	      UChar *bordered_curr_bab, 
	      Int mbsize,
		  Int scan_order // SAIT_PDAM: ADDED by D.-S.Cho (Samsung AIT)
		  )
{
	Int	i,j,i2,j2,k,l;
	Int	mborder = MBORDER;
	Int	mbsize_ext = mbsize+(mborder<<1);
	Int	border = BORDER;
	Int	bsize = mbsize >> 1;
	Int	bsize_ext = bsize+(border<<1);

	Int	b_differ,b_except,curr,prev,next;
	Int	bab_type;

	UChar	*lower_bab_data,
		*curr_bab_data, *half_bab_data;
	UChar *curr_bab_data_tr =NULL; // SAIT_PDAM: ADDED by D.-S.Cho (Samsung AIT) 
	
	lower_bab_data = bordered_lower_bab + border * bsize_ext + border;
	curr_bab_data = bordered_curr_bab + mborder * mbsize_ext + mborder;
	half_bab_data = bordered_half_bab + mborder * bsize_ext + border;
	/* P0, P1,
	   P2, P3 */

	/* compare P0 pixels in the current layer  with the pixels in the lower layer  and half layer  */
	/* SL: we need to take care of half layer even in transitional BAB 
	   case */
	b_differ=0;
        for(j=j2=k=l=0; j<bsize; j++, j2+=2, k+=(mbsize_ext<<1),
	      l+=bsize_ext)
	{
           for(i=i2=0; i<bsize; i++, i2+=2)
	   {
		if( curr_bab_data[k+i2] != lower_bab_data[l+i] )
		{
			b_differ = 1;	/* 010 case */
			break;
		}
	   }
	   if( b_differ ) break;
	}
	/* SL added for every line*/
	if(!b_differ) {
	  for(j=k=l=0; j<bsize; j++, k+=mbsize_ext,
	        l+=bsize_ext)
	    {
	      for(i=i2=0; i<bsize; i++, i2+=2)
		{
		  if( curr_bab_data[k+i2] != half_bab_data[l+i] )
		    {
		      b_differ = 1;	/* 010 case */
		      break;
		    }
		}
	      if( b_differ ) break;
	    }
	}

	// SAIT_PDAM: BEGIN - ADDED by D.-S.Cho (Samsung AIT) 
	if (scan_order == 1) {
		curr_bab_data_tr = (UChar *)calloc(mbsize_ext*mbsize_ext, sizeof(UChar));
		for (j=0; j<mbsize_ext; j++) 
			for (i=0; i<mbsize_ext; i++)
				curr_bab_data_tr[j*mbsize_ext+i] = bordered_curr_bab[i*mbsize_ext+j];
		curr_bab_data = curr_bab_data_tr + mborder * mbsize_ext + mborder;
		// memory free ??
	}
	// SAIT_PDAM: END - ADDED by D.-S.Cho (Samsung AIT) 

	/* 000 111 110 100 001 011 and 101 cases */
	if(!b_differ) {
	   /* check wheather there are some exceptional samples or not */
	   b_except=0;
	   /* check for P1 pixel decoding */
	   for(j2=k=0; j2<mbsize; j2+=2, k+=(mbsize_ext<<1) )
	   {
	      for(i2=1; i2<mbsize; i2+=2)
              {
		prev= curr_bab_data[k+i2-1];
		curr= curr_bab_data[k+i2];
                next= curr_bab_data[k+i2+1];

		if((prev==next) && (curr!=prev)) 
		{
			b_except = 1;   	/*101 case*/
			break;
		}
	      }
	      if(b_except) break;
	   }

	   if(!b_except)
	   {
		/*-- check for P2/P3 pixel decoding --*/
		for(j2=1,k=mbsize_ext; j2<mbsize; j2+=2, k+=(mbsize_ext<<1) )
		{
		   for(i2=0; i2<mbsize; i2++)
		   {
			prev= curr_bab_data[k-mbsize_ext+i2];
			curr= curr_bab_data[k+i2]; 
			next= curr_bab_data[k+mbsize_ext+i2];
 
		        if((prev==next) && (curr!=prev)) 
			{
			   b_except = 1; 	/*101 case */
			   break;
			}
		    }
		    if(b_except) break;
		}
	   }
	} else {
	   b_except = 1;
	} 

	if(b_except)	bab_type = 1; /* full_coded */
	else		bab_type = 0; /* xor_coded (Modified SISC) */
	
	if (scan_order == 1) free(curr_bab_data_tr); // SAIT_PDAM: ADDED by D.-S.Cho (Samsung AIT)
        /* 0: XOR_coded, 1: Full_coded, 2:XOR+P0  */
	return bab_type;
}

/***********************************************************CommentBegin******
 *
 * -- ShapeEnhContentEncode -- Encodes a binary alpha block using SI.
 *
 * Author :		
 *	Dae-Sung Cho (Samsung AIT)
 *
 * Created :		
 *	09-Feb-98
 * 
 * Arguments: 	
 *
 *
 * Return values :	
 *
 * Side effects :	
 *	-
 *
 * Description :	A binary alpha block will be coded using bac.
 *			These bits are added to the <shape_stream>. 
 *
 * See also :
 *
 ***********************************************************CommentEnd********/

Int	CVTCEncoder::
ShapeEnhContentEncode(UChar *bordered_lower_bab, 
		      UChar *bordered_half_bab, 
		      UChar *bordered_curr_bab, 
			Int bab_type, 
			Int scan_order, // SAIT_PDAM: ADDED by D.-S.Cho (Samsung AIT) 
			Int mbsize,
				FILTER *filter,
			BSS *bitstream,	
			ArCoder *ar_coder)
{
	if (bab_type==0) 		/* Interleaved Scan-Line (ISL) coding */
	{  
	      ExclusiveORencoding (bordered_lower_bab,
				   bordered_half_bab,
				   bordered_curr_bab,
				   mbsize,
				   scan_order, // SAIT_PDAM: ADDED by D.-S.Cho (Samsung AIT) 
				   bitstream,
				   ar_coder);

	} else if(bab_type ==1)		/* Raster Scan-Line (RSL) coding */
	{	

	  FullEncoding (bordered_lower_bab,
			    bordered_half_bab,
			    bordered_curr_bab,
				mbsize,
			        filter,
				bitstream,
				ar_coder);
	} else 
	{
	      fprintf(stderr,"BAB type[%d] ERROR in Enhancement layer coding!\n", bab_type);
	}

        return 0;
}
	     
Void CVTCEncoder::
ExclusiveORencoding (UChar *bordered_lower_bab,
		     UChar *bordered_half_bab,
		     UChar *bordered_curr_bab,
		     Int mbsize,
			 Int scan_order, // SAIT_PDAM: ADDED by D.-S.Cho (Samsung AIT) 
		     BSS *bitstream, 
		     ArCoder *ar_coder)
{
	Int             i2,j2,k,curr,prev,next;
	Int				i,j;	// SAIT_PDAM: ADDED by D.-S.Cho (Samsung AIT) 
	Int             mborder = MBORDER;
	Int             mbsize_ext = mbsize+(mborder<<1);
	Int             border = BORDER;
	Int             bsize = mbsize >> 1;
	Int             bsize_ext = bsize+(border<<1);
	Int             context = 0, prob=0;
	Int		start_bit, end_bit, bitnum=0;
	UChar           *lower_bab_data,
                        *curr_bab_data;
	UChar			*curr_bab_data_tr = NULL;	// SAIT_PDAM: ADDED by D.-S.Cho (Samsung AIT) 

        lower_bab_data = bordered_lower_bab + border * bsize_ext + border;
		// SAIT_PDAM: BEGIN - ADDED by D.-S.Cho (Samsung AIT) 
		if (scan_order == 1) {
			curr_bab_data_tr = (UChar *)calloc(mbsize_ext*mbsize_ext, sizeof(UChar));
			for(j=0; j<mbsize_ext; j++)
				for(i=0; i<mbsize_ext; i++)
					curr_bab_data_tr[j*mbsize_ext+i]=bordered_curr_bab[i*mbsize_ext+j];
			curr_bab_data = curr_bab_data_tr + mborder * mbsize_ext + mborder;
		} else
		// SAIT_PDAM: END - ADDED by D.-S.Cho (Samsung AIT) 
        curr_bab_data= bordered_curr_bab + mborder * mbsize_ext + mborder;

	start_bit = bitstream->cnt;


	/*-- P1 pixels encoding --*/
	// SAIT_PDAM BEGIN - MODIFIED by D.-S.Cho (Samsung AIT) 
	//for(j2=k=0; j2<mbsize; j2+=2, k+=(mbsize_ext<<1) )
    //    {
	//   for(i2=1; i2<mbsize; i2+=2)
	//   {
	   for(i2=1; i2<mbsize; i2+=2)
	   {
	for(j2=k=0; j2<mbsize; j2+=2, k+=(mbsize_ext<<1) )
        {
	// SAIT_PDAM END - MODIFIED by D.-S.Cho (Samsung AIT) 
		curr= curr_bab_data[k+i2];
		prev= curr_bab_data[k+i2-1];
		next= curr_bab_data[k+i2+1];
                       
		if(prev!=next) 
		{
		   context = GetContextEnhBAB_XOR(curr_bab_data,
					i2, 
					j2, 
					mbsize_ext, 
					0); /* pixel type : 0-P1, 1-P2/P3 */

		   prob=scalable_xor_prob_1[context];
		   ArCodeSymbol_Still(ar_coder, bitstream, curr, prob);

		} 
		else 
		{
		   if(prev!=curr)
		   {
			fprintf(stderr, "Error: BAB coding mode mismatch in XOR coding : P1!\n");
			fprintf(stderr, "Error: P1[%d,%d,%d]!\n",prev,curr,next);
			fprintf(stderr,"1, j2=%d i2=%d prev=%d curr=%d next=%d context=%d bits=%d\n", 
				j2, i2, prev, curr, next, context, bitstream->cnt);
			exit(0);
		   }
		}
	   }
	}

	/*-- P2/P3 pixel coding --*/
        for(j2=1,k=mbsize_ext; j2<mbsize; j2+=2, k+=(mbsize_ext<<1) )
        {
	   for(i2=0; i2<mbsize; i2++)
	   {
		curr= curr_bab_data[k+i2];
		prev= curr_bab_data[k-mbsize_ext+i2];	
		next= curr_bab_data[k+mbsize_ext+i2];	

		if(prev!=next) 
		{
		   context = GetContextEnhBAB_XOR(curr_bab_data,
					i2, 
					j2, 
					mbsize_ext, 
					1);	/* pixel type : 0-P1, 1-P2/P3 */

		   prob=scalable_xor_prob_23[context];
		   ArCodeSymbol_Still(ar_coder, bitstream, curr, prob);
		} 
		else 
		{
		   if(prev!=curr)
		   {
			fprintf(stderr, "Error: BAB coding mode mismatch in XOR coding : P2, P3!\n");
			exit(0);
		   }
		}
	   }
	}

	if(scan_order==1) free(curr_bab_data_tr); // SAIT_PDAM: ADDED by D.-S.Cho (Samsung AIT) 
	end_bit = bitstream->cnt;
	bitnum = end_bit - start_bit;

}

/* Modified by shson */
Void CVTCEncoder::
FullEncoding (UChar *bordered_lower_bab,
	      UChar *bordered_half_bab,
	      UChar *bordered_curr_bab,
	      Int mbsize,
				FILTER *filter,
			BSS *bitstream, 
			ArCoder *ar_coder)
{
	Int		i,j,i2,j2,k,l,curr;

	Int		mborder = MBORDER;
	Int		mbsize_ext = mbsize+(mborder<<1);
	Int		border = BORDER;
	Int		bsize = mbsize >> 1;
	Int		bsize_ext = bsize+(border<<1);
//	Int		not_coded, low_value;
	Int		context, prob=0;
	Int		start_bit, end_bit, bitnum=0;

	UChar		*lower_bab_data,
	  *half_bab_data,			
	  *curr_bab_data;

	lower_bab_data = bordered_lower_bab + border * bsize_ext + border;
	half_bab_data= bordered_half_bab + mborder * bsize_ext + border;
	curr_bab_data= bordered_curr_bab + mborder * mbsize_ext + mborder;

	start_bit = bitstream->cnt;
	/* vertical pass first for half-higher layer encoding*/

	for(j2=k=0; j2<mbsize; j2+=2, k+=2*bsize_ext)
	{
	   for(i=0; i<bsize; i++)
	   {
	     j = j2>>1;
	     l = j*bsize_ext;

	     /* T1 */
	     curr = half_bab_data[k+i];
	     context =  (half_bab_data[k-bsize_ext+i]<<7) /* T9*/
	       | ( half_bab_data[k-bsize_ext+i+1] <<6) /* T8 */
	       | (half_bab_data[k+bsize_ext+i-1] <<5) /* T7 */
	       | (half_bab_data[k+i-1] <<4) /* T6 */
	       | (lower_bab_data[l+i] <<3) /* T5 */
	       | (lower_bab_data[l+i+1] <<2) /* T4 */
	       | (lower_bab_data[l+bsize_ext+i] <<1) /* T3 */
	       | (lower_bab_data[l+bsize_ext+i+1]); /* T2 */

	     if(filter->DWT_Class==DWT_ODD_SYMMETRIC){
	       prob=prob_odd0[context];
	     } else if(filter->DWT_Class==DWT_EVEN_SYMMETRIC) {
	       prob=prob_even0[context];
	     } else {
	       fprintf(stderr,"Error: filter type in FullEncoding() !\n");
	       exit(0);
	     }
	     ArCodeSymbol_Still(ar_coder, bitstream, curr, prob);

	       /* T0 */
	     curr = half_bab_data[k+bsize_ext+i];
	     context =  (half_bab_data[k+i]<<7) /* T1*/
	       | ( lower_bab_data[l+bsize_ext+i-1] <<6) /* T10 */
	       | (half_bab_data[k+bsize_ext+i-1] <<5) /* T7 */
	       | (half_bab_data[k+i-1] <<4) /* T6 */
	       | (lower_bab_data[l+i] <<3) /* T5 */
	       | (lower_bab_data[l+i+1] <<2) /* T4 */
	       | (lower_bab_data[l+bsize_ext+i] <<1) /* T3 */
	       | (lower_bab_data[l+bsize_ext+i+1]); /* T2 */

	     if(filter->DWT_Class==DWT_ODD_SYMMETRIC){
	       prob=prob_odd1[context];
	     } else if(filter->DWT_Class==DWT_EVEN_SYMMETRIC) {
	       prob=prob_even1[context];
	     } else {
	       fprintf(stderr,"Error: filter type in FullEncoding() !\n");
	       exit(0);
	     }
	     ArCodeSymbol_Still(ar_coder, bitstream, curr, prob);
	   }
	}

	/* horizontal */
	for(i2=0; i2<mbsize; i2+=2)
	  {
	    for(j=k=0; j<mbsize; j++, k+=mbsize_ext)
	      {
	   
		i = i2>>1;
		l = j*bsize_ext;
		/* T1 */
		curr = curr_bab_data[k+i2];
		context =  (curr_bab_data[k+i2-1]<<7) /* T9*/
		  | ( curr_bab_data[k+mbsize_ext+i2-1] <<6) /* T8 */
		  | (curr_bab_data[k-mbsize_ext+i2+1] <<5) /* T7 */
		  | (curr_bab_data[k-mbsize_ext+i2] <<4) /* T6 */
		  | (half_bab_data[l+i] <<3) /* T5 */
		  | (half_bab_data[l+bsize_ext+i] <<2) /* T4 */
		  | (half_bab_data[l+i+1] <<1) /* T3 */
		  | (half_bab_data[l+bsize_ext+i+1]); /* T2 */

		if(filter->DWT_Class==DWT_ODD_SYMMETRIC){
		  prob=prob_odd0[context];
		} else if(filter->DWT_Class==DWT_EVEN_SYMMETRIC) {
		  prob=prob_even0[context];
		} else {
		  fprintf(stderr,"Error: filter type in FullEncoding() !\n");
		  exit(0);
		}
		ArCodeSymbol_Still(ar_coder, bitstream, curr, prob);
		/* T0 */
		curr = curr_bab_data[k+i2+1];
		context =  (curr_bab_data[k+i2]<<7) /* T1*/
		  | ( half_bab_data[l-bsize_ext+i+1] <<6) /* T10 */
		  | (curr_bab_data[k-mbsize_ext+i2+1] <<5) /* T7 */
		  | (curr_bab_data[k-mbsize_ext+i2] <<4) /* T6 */
		  | (half_bab_data[l+i] <<3) /* T5 */
		  | (half_bab_data[l+bsize_ext+i] <<2) /* T4 */
		  | (half_bab_data[l+i+1] <<1) /* T3 */
		  | (half_bab_data[l+bsize_ext+i+1]); /* T2 */

		if(filter->DWT_Class==DWT_ODD_SYMMETRIC){
		  prob=prob_odd1[context];
		} else if(filter->DWT_Class==DWT_EVEN_SYMMETRIC) {
		  prob=prob_even1[context];
		} else {
		  fprintf(stderr,"Error: filter type in FullEncoding() !\n");
		  exit(0);
		}
		ArCodeSymbol_Still(ar_coder, bitstream, curr, prob);
	      }
	  }

	end_bit = bitstream->cnt;
	bitnum = end_bit - start_bit;

}

/*********************************************************************************
 *
 *      Bitstream Manipulation  
 *
*********************************************************************************/
/* Merge Shape Bitstream into main bit stream */
Void CVTCEncoder::
MergeEnhShapeBitstream()
{
  if(ShapeBitstream==NULL) {
    fprintf(stderr,"EnhShapeBitStream Not Available\n");
    exit(1);
  }
  InitBitstream(0,ShapeBitstream);
  BitStreamMerge(ShapeBitstreamLength, ShapeBitstream);
 
  free(ShapeBitstream->bs);
  free(ShapeBitstream);
  ShapeBitstream = NULL;
}
 

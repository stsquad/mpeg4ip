/*
***********************************************************************
* COPYRIGHT AND WARRANTY INFORMATION
*
* Copyright 2001, International Telecommunications Union, Geneva
*
* DISCLAIMER OF WARRANTY
*
* These software programs are available to the user without any
* license fee or royalty on an "as is" basis. The ITU disclaims
* any and all warranties, whether express, implied, or
* statutory, including any implied warranties of merchantability
* or of fitness for a particular purpose.  In no event shall the
* contributor or the ITU be liable for any incidental, punitive, or
* consequential damages of any kind whatsoever arising from the
* use of these programs.
*
* This disclaimer of warranty extends to the user of these programs
* and user's customers, employees, agents, transferees, successors,
* and assigns.
*
* The ITU does not represent or warrant that the programs furnished
* hereunder are free of infringement of any third-party patents.
* Commercial implementations of ITU-T Recommendations, including
* shareware, may be subject to royalty fees to patent holders.
* Information regarding the ITU-T patent policy is available from
* the ITU Web site at http://www.itu.int.
*
* THIS IS NOT A GRANT OF PATENT RIGHTS - SEE THE ITU-T PATENT POLICY.
************************************************************************
*/

/*!
 ***************************************************************************
 * \file
 *    biariencode.h
 *
 * \brief
 *    Headerfile for binary arithmetic encoding routines
 *
 * \author
 *    Detlev Marpe,
 *    Gabi Blaettermann                                                     \n
 *    Copyright (C) 2000 HEINRICH HERTZ INSTITUTE All Rights Reserved.
 *
 * \date
 *    21. Oct 2000
 **************************************************************************
 */


#ifndef _BIARIENCOD_H_
#define _BIARIENCOD_H_


#define AAC_FRAC_TABLE 0  //!< replaces division in the AC by a table lookup


/************************************************************************
 * C o n s t a n t s
 ***********************************************************************
 */

//! precision for arithmetic
#define CODE_VALUE_BITS 16

// 1/4  2/4  3/4  of interval
#define FIRST_QTR 0x4000 //!< (1<<(CODE_VALUE_BITS-2))
#define HALF      0x8000 //!< (FIRST_QTR+FIRST_QTR)
#define THIRD_QTR 0xC000 //!< (HALF+FIRST_QTR)

//! maximum value of range
#define TOP_VALUE 0xFFFF //!< ((1<<CODE_VALUE_BITS)-1)

#ifdef AAC_FRAC_TABLE

//! ARITH_CUM_FREQ_TABLE[i]=(unsigned int)((pow(2,26)/i)+0.5);
const unsigned int ARITH_CUM_FREQ_TABLE[128] =
{
       0,        0, 33554432,  22369621,  16777216,  13421773,  11184811,  9586981,  8388608,  7456540,
 6710886,  6100806,  5592405,   5162220,   4793490,   4473924,   4194304,  3947580,  3728270,  3532045,
 3355443,  3195660,  3050403,   2917777,   2796203,   2684355,   2581110,  2485513,  2396745,  2314099,
 2236962,  2164802,  2097152,   2033602,   1973790,   1917396,   1864135,  1813753,  1766023,  1720740,
 1677722,  1636802,  1597830,   1560671,   1525201,   1491308,   1458888,  1427848,  1398101,  1369569,
 1342177,  1315860,  1290555,   1266205,   1242757,   1220161,   1198373,  1177348,  1157049,  1137438,
 1118481,  1100145,  1082401,   1065220,   1048576,   1032444,   1016801,  1001625,   986895,   972592,
  958698,   945195,   932068,    919300,    906877,    894785,    883011,   871544,   860370,   849479,
  838861,   828504,   818401,    808541,    798915,    789516,    780336,   771366,   762601,   754032,
  745654,   737460,   729444,    721601,    713924,    706409,    699051,   691844,   684784,   677867,
  671089,   664444,   657930,    651542,    645278,    639132,    633102,   627186,   621378,   615678,
  610081,   604584,   599186,    593884,    588674,    583555,    578525,   573580,   568719,   563940,
  559241,   554619,   550073,    545601,    541201,    536871,    532610,   528416
};

#endif

/************************************************************************
 * D e f i n i t i o n s
 ***********************************************************************
 */

// some definitions to increase the readability of the source code

#define Elow            (eep->Elow)
#define Ehigh           (eep->Ehigh)
#define Ebits_to_follow (eep->Ebits_to_follow)
#define Ebuffer         (eep->Ebuffer)
#define Ebits_to_go     (eep->Ebits_to_go)
#define Ecodestrm       (eep->Ecodestrm)
#define Ecodestrm_len   (eep->Ecodestrm_len)


#endif  // BIARIENCOD_H

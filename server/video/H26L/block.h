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
 ************************************************************************
 * \file block.h
 *
 * \author
 *  Inge Lille-Langøy               <inge.lille-langoy@telenor.com>    \n
 *  Telenor Satellite Services                                         \n
 *  P.O.Box 6914 St.Olavs plass                                        \n
 *  N-0130 Oslo, Norway
 *
 ************************************************************************
 */

#ifndef _BLOCK_H_
#define _BLOCK_H_

#include "global.h"

/*!  max number of double quant coefficients used for RD constrained quantization.
     Set to 3 in the TML test model  */
#define MAX_TWO_LEVEL_COEFF 3
//! pow(2,MAX_TWO_LEVEL_COEFF), must be updated together with MAX_TWO_LEVEL_COEFF
#define MTLC_POW            8

#define QUANT_LUMA_SNG      0
#define QUANT_LUMA_AC       1
#define QUANT_LUMA_DBL      2
#define QUANT_CHROMA_DC     3
#define QUANT_CHROMA_AC     4



int snr_arr[16][MTLC_POW];
int level_arr[16][MTLC_POW+1];

const int JQQ1=1048576; // = J20
const int JQQ2= 524288;
const int JQQ3= 349525;
const int JQQ4= 174762;
const int JQ4 = 471859;

const int J13 = 8192;
const int J19 = 524288;
const int J20 = 1048576;

extern const byte FILTER_STR[32][4];

//! numbers used for quantization/dequantization.
const int JQ[32][2] =
{
  {620,  3881  },
  {553,  4351  },
  {492,  4890  },
  {439,  5481  },
  {391,  6154  },
  {348,  6914  },
  {310,  7761  },
  {276,  8718  },
  {246,  9781  },
  {219,  10987 },
  {195,  12339 },
  {174,  13828 },
  {155,  15523 },
  {138,  17435 },
  {123,  19561 },
  {110,  21873 },
  { 98,  24552 },
  { 87,  27656 },
  { 78,  30847 },
  { 69,  34870 },
  { 62,  38807 },
  { 55,  43747 },
  { 49,  49103 },
  { 44,  54683 },
  { 39,  61694 },
  { 35,  68745 },
  { 31,  77615 },
  { 27,  89113 },
  { 24,  100253},
  { 22,  109366},
  { 19,  126635},
  { 17,  141533},
};

//! make chroma QP from quant
const int QP_SCALE_CR[32]=
{
   0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15,
  16,17,17,18,19,20,20,21,22,22,23,23,24,24,25,25
};



//! single scan pattern
const byte SNGL_SCAN[16][2] =
{
  {0,0},{1,0},{0,1},{0,2},
  {1,1},{2,0},{3,0},{2,1},
  {1,2},{0,3},{1,3},{2,2},
  {3,1},{3,2},{2,3},{3,3}
};

//! double scan pattern
const byte DBL_SCAN[8][2][2] =
{
  {{0,0},{0,1}},
  {{1,0},{0,2}},
  {{2,1},{0,1}},
  {{2,1},{1,2}},
  {{2,0},{2,3}},
  {{3,1},{0,3}},
  {{3,2},{1,3}},
  {{3,3},{2,3}},
};

//! array used to find expencive coefficients
const byte COEFF_COST[16] =
{
  3,2,2,1,1,1,0,0,0,0,0,0,0,0,0,0
};



//! bit cost for coefficients
const byte COEFF_BIT_COST[3][16][16]=
{
  { // 2x2 scan (corrested per Gisle's Email 11/23/2000 by StW
    { 3, 5, 7, 9, 9,11,11,11,11,13,13,13,13,13,13,13},
    { 5, 7, 9, 9,11,11,11,11,13,13,13,13,13,13,13,13},
    { 7, 9, 9,11,11,11,11,13,13,13,13,13,13,13,13,15},
    { 7, 9, 9,11,11,11,11,13,13,13,13,13,13,13,13,15},
    { 7, 7, 9, 9, 9, 9,11,11,11,11,11,11,11,11,13,13},
    { 7, 7, 9, 9, 9, 9,11,11,11,11,11,11,11,11,13,13},
    { 7, 7, 9, 9, 9, 9,11,11,11,11,11,11,11,11,13,13},
    { 7, 7, 9, 9, 9, 9,11,11,11,11,11,11,11,11,13,13},
    { 7, 7, 9, 9, 9, 9,11,11,11,11,11,11,11,11,13,13},
    { 7, 7, 9, 9, 9, 9,11,11,11,11,11,11,11,11,13,13},
    { 7, 7, 9, 9, 9, 9,11,11,11,11,11,11,11,11,13,13},
    { 7, 7, 9, 9, 9, 9,11,11,11,11,11,11,11,11,13,13},
    { 7, 7, 9, 9, 9, 9,11,11,11,11,11,11,11,11,13,13},
    { 7, 7, 9, 9, 9, 9,11,11,11,11,11,11,11,11,13,13},
    { 7, 7, 9, 9, 9, 9,11,11,11,11,11,11,11,11,13,13},
    { 7, 7, 9, 9, 9, 9,11,11,11,11,11,11,11,11,13,13},
  },
  {  // double scan
    { 3, 5, 7, 7, 7, 9, 9, 9, 9,11,11,13,13,13,13,15},
    { 5, 9, 9,11,11,13,13,13,13,15,15,15,15,15,15,15},
    { 7,11,11,13,13,13,13,15,15,15,15,15,15,15,15,17},
    { 9,11,11,13,13,13,13,15,15,15,15,15,15,15,15,17},
    { 9,11,11,13,13,13,13,15,15,15,15,15,15,15,15,17},
    {11,11,13,13,13,13,15,15,15,15,15,15,15,15,17,17},
    {11,11,13,13,13,13,15,15,15,15,15,15,15,15,17,17},
    {11,11,13,13,13,13,15,15,15,15,15,15,15,15,17,17},
    {11,11,13,13,13,13,15,15,15,15,15,15,15,15,17,17},
    {11,11,13,13,13,13,15,15,15,15,15,15,15,15,17,17},
    {11,11,13,13,13,13,15,15,15,15,15,15,15,15,17,17},
    {11,11,13,13,13,13,15,15,15,15,15,15,15,15,17,17},
    {11,11,13,13,13,13,15,15,15,15,15,15,15,15,17,17},
    {11,11,13,13,13,13,15,15,15,15,15,15,15,15,17,17},
  },
  {    // single scan
    { 3, 7, 9, 9,11,13,13,15,15,15,15,17,17,17,17,17},
    { 5, 9,11,13,13,15,15,15,15,17,17,17,17,17,17,17},
    { 5, 9,11,13,13,15,15,15,15,17,17,17,17,17,17,17},
    { 7,11,13,13,15,15,15,15,17,17,17,17,17,17,17,17},
    { 7,11,13,13,15,15,15,15,17,17,17,17,17,17,17,17},
    { 7,11,13,13,15,15,15,15,17,17,17,17,17,17,17,17},
    { 9,11,13,13,15,15,15,15,17,17,17,17,17,17,17,17},
    { 9,11,13,13,15,15,15,15,17,17,17,17,17,17,17,17},
    { 9,11,13,13,15,15,15,15,17,17,17,17,17,17,17,17},
    { 9,11,13,13,15,15,15,15,17,17,17,17,17,17,17,17},
    {11,13,13,15,15,15,15,17,17,17,17,17,17,17,17,19},
    {11,13,13,15,15,15,15,17,17,17,17,17,17,17,17,19},
    {11,13,13,15,15,15,15,17,17,17,17,17,17,17,17,19},
    {11,13,13,15,15,15,15,17,17,17,17,17,17,17,17,19},
    {11,13,13,15,15,15,15,17,17,17,17,17,17,17,17,19},
    {11,13,13,15,15,15,15,17,17,17,17,17,17,17,17,19},
  },
};

#endif


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
 *    cabac.h
 *
 * \brief
 *    Headerfile for entropy coding routines
 *
 * \author
 *    Detlev Marpe                                                         \n
 *    Copyright (C) 2000 HEINRICH HERTZ INSTITUTE All Rights Reserved.
 *
 * \date
 *    21. Oct 2000 (Changes by Tobias Oelbaum 28.08.2001)
 ***************************************************************************
 */


#ifndef _CABAC_H_
#define _CABAC_H_

#include "global.h"


/*******************************************************************************************
 * l o c a l    c o n s t a n t s   f o r   i n i t i a l i z a t i o n   o f   m o d e l s
 *******************************************************************************************
 */

static const int MB_TYPE_Ini[2][10][5]=
{
  {{8,1,50,0,0},  {2,1,50,0,0}, {2,1,50,0,0}, {1,5,50,0,0},   {1,1,50,0,0}, {1,1,50,0,0}, {2,1,50,0,0}, {2,1,50,0,0}, {1,1,50,0,0}, {1,1,50,0,0}},
  {{7,2,50,2,0},  {1,2,50,0,0}, {1,2,50,0,0}, {1,10,50,0,-2}, {2,3,50,0,0}, {9,4,50,2,0}, {2,1,50,1,0}, {7,2,50,1,0}, {2,1,50,0,0}, {3,2,50,0,0}}
};


static const int MV_RES_Ini[2][10][3]=
{
  {{9,5,50},  {1,1,50}, {1,1,50}, {4,5,50},   {1,1,50},  {13,5,50}, {1,1,50}, {6,5,50}, {1,1,50},  {1,1,50}},
  {{1,2,50},  {1,4,50}, {1,2,50}, {1,10,50},  {6,5,50},  {4,5,50},  {1,4,50}, {2,5,50}, {1,10,50}, {1,1,50}}
};


static const int REF_NO_Ini[6][3]=
{
  {10,1,50},  {2,1,50}, {1,1,50}, {1,3,50}, {2,1,50}, {1,1,50}
};

static const int DELTA_QP_Ini[4][3]=
{
  {1,1,50}, {1,1,50}, {1,1,50}, {1,1,50}
};

static const int CBP_Ini[2][3][4][5]=
{
  { {{1,4,50,0,0},   {1,2,50,0,0},   {1,2,50,0,0},   {4,3,50,0,0}},
    {{1,2,50,0,0},   {1,3,50,0,0},   {1,3,50,0,0},   {1,3,50,0,0}},
    {{1,1,50,4,2},   {1,1,50,0,0},   {1,2,50,0,0},   {1,2,50,0,0}} }, //!< intra cbp
  { {{1,4,50,1,-1},  {1,1,50,2,0},   {1,1,50,2,0},   {3,1,50,4,0}},
    {{3,1,50,2,0},   {6,5,50,0,-1},  {6,5,50,-2,-2}, {1,2,50,1,0}},
    {{5,2,50,1,0},   {1,1,50,2,1},   {1,1,50,0,0},   {1,2,50,0,0}} }  //!< inter cbp
};



static const int IPR_Ini[6][2][3]=
{
  {{2,1,50},  {1,1,50}},
  {{3,2,50},  {1,1,50}},
  {{1,1,50},  {2,3,50}},
  {{1,1,50},  {2,3,50}},
  {{1,1,50},  {1,1,50}},
  {{2,3,50},  {1,1,50}}
};


static const int Run_Ini[9][2][3]=
{
  {{3,1,50},  {2,1,50}}, //!< double scan
  {{3,2,50},  {3,4,50}}, //!< single scan, inter
  {{3,2,50},  {1,1,50}}, //!< single scan, intra
  {{1,1,50},  {1,1,50}}, //!< 16x16 DC
  {{1,1,50},  {1,2,50}}, //!< 16x16 AC
  {{3,2,50},  {3,2,50}}, //!< chroma inter DC
  {{4,1,50},  {2,1,50}}, //!< chroma intra DC
  {{3,2,50},  {1,1,50}}, //!< chroma inter AC
  {{2,1,50},  {2,1,50}}  //!< chroma intra AC
};


static const int Level_Ini[9][4][5]=
{
  {{1,1,50,0,0},  {2,1,50,0,0}, {4,3,50,0,0},   {1,1,50,0,0}}, //!< double scan
  {{1,1,50,0,0},  {5,1,50,3,0}, {3,1,50,0,0},   {1,1,50,0,0}}, //!< single scan, inter
  {{3,2,50,0,0},  {6,1,50,0,0}, {7,4,50,0,0},   {5,4,50,0,0}}, //!< single scan, intra
  {{1,1,50,0,0},  {3,1,50,0,0}, {2,1,50,0,0},   {1,1,50,0,0}}, //!< 16x16 DC
  {{4,1,50,0,0},  {6,1,50,3,0}, {2,1,50,0,0},   {5,4,50,0,0}}, //!< 16x16 AC
  {{5,4,50,0,0},  {4,1,50,0,0}, {2,1,50,0,0},   {1,1,50,0,0}}, //!< chroma inter DC
  {{1,1,50,0,0},  {1,1,50,2,0}, {1,1,50,0,0},   {1,1,50,0,0}}, //!< chroma intra DC
  {{1,1,50,0,0},  {4,1,50,0,0}, {2,1,50,0,0},   {1,1,50,0,0}}, //!< chroma inter AC
  {{1,1,50,0,0},  {5,2,50,0,0}, {1,1,50,0,0},   {1,1,50,0,0}}  //!< chroma intra AC
};

/***********************************************************************
 * L O C A L L Y   D E F I N E D   F U N C T I O N   P R O T O T Y P E S
 ***********************************************************************
 */



void unary_bin_encode(EncodingEnvironmentPtr eep_frame,
                      unsigned int symbol,
                      BiContextTypePtr ctx,
                      int ctx_offset);

void unary_bin_max_encode(EncodingEnvironmentPtr eep_frame,
                          unsigned int symbol,
                          BiContextTypePtr ctx,
                          int ctx_offset,
                          unsigned int max_symbol);

void unary_level_encode(EncodingEnvironmentPtr eep_frame,
                        unsigned int symbol,
                        BiContextTypePtr ctx);

void unary_mv_encode(EncodingEnvironmentPtr eep_frame,
                     unsigned int symbol,
                     BiContextTypePtr ctx,
                     unsigned int max_bin);


#endif  // CABAC_H



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
 * \file rdopt.h
 *
 * \brief
 *    Headerfile for RD-optimized mode decision
 * \author
 *    Heiko Schwarz  <hschwarz@hhi.de>
 * \date
 *    12. April 2001
 **************************************************************************
 */

#ifndef _RD_OPT_H_
#define _RD_OPT_H_

#include "global.h"
#include "elements.h"
#include "rdopt_coding_state.h"


//===== MACROBLOCK MODE CONSTANTS =====
#define  MBMODE_COPY            0
#define  MBMODE_INTER16x16      1
#define  MBMODE_INTER4x4        7
#define  MBMODE_BACKWARD16x16   8
#define  MBMODE_BACKWARD4x4    14
#define  MBMODE_BIDIRECTIONAL  15
#define  MBMODE_DIRECT         16
#define  MBMODE_INTRA16x16     17
#define  MBMODE_INTRA4x4       18
//-----------------------------
#define  NO_MAX_MBMODES        19
#define  NO_INTER_MBMODES       7



typedef struct {
  double  rdcost;
  long    distortion;
  long    distortion_UV;
  int     rate_mode;
  int     rate_motion;
  int     rate_cbp;
  int     rate_luma;
  int     rate_chroma;
} RateDistortion;



typedef struct {

  //=== lagrange multipliers ===
  double   lambda_intra;
  double   lambda_motion;
  double   lambda_mode;

  //=== cost, distortion and rate for 4x4 intra modes ===
  int      best_mode_4x4     [4][4];
  double   min_rdcost_4x4    [4][4];
  double   rdcost_4x4        [4][4][NO_INTRA_PMODE];
  long     distortion_4x4    [4][4][NO_INTRA_PMODE];
  int      rate_imode_4x4    [4][4][NO_INTRA_PMODE];
  int      rate_luma_4x4     [4][4][NO_INTRA_PMODE];

  //=== stored variables for "best" macroblock mode ===
  double   min_rdcost;
  int      best_mode;
  int      ref_or_i16mode;
  int      blocktype;
  int      blocktype_back;
  byte     rec_mb_Y     [16][16];
  byte     rec_mb_U     [ 8][ 8];
  byte     rec_mb_V     [ 8][ 8];
  int      ipredmode    [16];
  int      cbp;
  int      cbp_blk;
  int      kac;
  int      cof          [4][6][18][2][2];
  int      cofu         [5][2][2];
  int      mv           [4][4][2];
  int      bw_mv        [4][4][2];

} RDOpt;

#endif

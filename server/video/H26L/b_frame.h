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
 *************************************************************************************
 * \file b_frame.h
 *
 * \brief
 *    Header file for B picture coding
 *
 * \author
 *    Main contributor and Contact Information
 *    - Byeong-Moon Jeon      <jeonbm@lge.com>
 *    - Yoon-Seong Soh        <yunsung@lge.com>                                     \n
 *      LG Electronics Inc., Digital Media Research Lab.                            \n
 *      16 Woomyeon-Dong, Seocho-Gu, Seoul, 137-724, Korea
 *************************************************************************************
 */
#ifndef _B_FRAME_H_
#define _B_FRAME_H_

#define MAX_DIR_SAD     100000000

extern int  ONE_FOURTH_TAP[3][2];
extern int  MODTAB[3][2];
extern int  NCBP[48][2];
extern byte PRED_IPRED[7][7][6];
extern int  IPRED_ORDER[6][6];
extern int  JQ[32][2];
extern int  QP2QUANT[32];


#endif


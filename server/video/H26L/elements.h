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
 **************************************************************************
 *  \file elements.h
 *  \brief  Header file for elements in H.26L streams
 *  \date 6.10.2000, 
 *  \version
 *      1.1
 *
 * \note
 *    Version 1.0 included three partition modes, no DP, 2 partitionsper slice
 *      and 4 partitions per slice.  As per document VCEG-N72 this is changed
 *      in version 1.1 to only two patrition modes, one without DP and one with 
 *      3 partition per slice
 *.
 *  \author Sebastian Purreiter     <sebastian.purreiter@mch.siemens.de>
 *  \author Stephan Wenger          <stewe@cs.tu-berlin.de>
 *
 **************************************************************************
 */

#ifndef _ELEMENTS_H_
#define _ELEMENTS_H_

#include "global.h"

/*!
 *  definition of H.26L syntax elements
 *  order of elements follow dependencies for picture reconstruction
 */
/*!
 * \brief   Assignment of old TYPE or partition elements to new
 *          elements
 *
 *  old element     | new elements
 *  ----------------+-------------------------------------------------------------------
 *  TYPE_HEADER     | SE_HEADER, SE_PTYPE
 *  TYPE_MBHEADER   | SE_MBTYPE, SE_REFFRAME, SE_INTRAPREDMODE
 *  TYPE_MVD        | SE_MVD
 *  TYPE_CBP        | SE_CBP_INTRA, SE_CBP_INTER
 *  TYPE_COEFF_Y    | SE_LUM_DC_INTRA, SE_LUM_AC_INTRA, SE_LUM_DC_INTER, SE_LUM_AC_INTER
 *  TYPE_2x2DC      | SE_CHR_DC_INTRA, SE_CHR_DC_INTER
 *  TYPE_COEFF_C    | SE_CHR_AC_INTRA, SE_CHR_AC_INTER
 *  TYPE_EOS        | SE_EOS
*/




#define MAXPARTITIONMODES 2 //!< maximum possible partition modes as defined in assignSE2partition[][]

/*!
 *  \brief  lookup-table to assign different elements to partition
 *
 *  \note here we defined up to 6 different partitions similar to
 *      document Q15-k-18 described in the PROGFRAMEMODE.
 *      The Sliceheader contains the PSYNC information. \par
 *
 *      Elements inside a partition are not ordered. They are
 *      ordered by occurence in the stream.
 *      Assumption: Only partitionlosses are considered. \par
 *
 *      The texture elements luminance and chrominance are
 *      not ordered in the progressive form
 *      This may be changed in image.c \par
 *
 *  -IMPORTANT:
 *      Picture- or Sliceheaders must be assigned to partition 0. \par
 *      Furthermore partitions must follow syntax dependencies as
 *      outlined in document Q15-J-23.
 */


// A note on this table:
//
// While the assignment of values in enum data types is specified in C, it is not
// very ood style to have an "elementnumber", not even as a comment.
//
// Hence a copy of the relevant structure from global.h here
/*
typedef enum {
 0  SE_HEADER,
 1  SE_PTYPE,
 2  SE_MBTYPE,
 3  SE_REFFRAME,
 4  SE_INTRAPREDMODE,
 5  SE_MVD,
 6  SE_CBP_INTRA,
 7  SE_LUM_DC_INTRA,
 8  SE_CHR_DC_INTRA,
 9  SE_LUM_AC_INTRA,
10  SE_CHR_AC_INTRA,
11  SE_CBP_INTER,
12  SE_LUM_DC_INTER,
13  SE_CHR_DC_INTER,
14  SE_LUM_AC_INTER,
15  SE_CHR_AC_INTER,
16  SE_DELTA_QUANT,
17  SE_BFRAME,
18  SE_EOS,
19  SE_MAX_ELEMENTS */ // number of maximum syntax elements

//} SE_type;


static int assignSE2partition[][SE_MAX_ELEMENTS] =
{
  // 0  1  2  3  4  5  6  7  8  9 10 11 12 13 14 15 16 17 18  // elementnumber (no not uncomment)
  {  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },   //!< all elements in one partition no data partitioning
  {  0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 0, 0, 0 }    //!< three partitions per slice
};

#endif

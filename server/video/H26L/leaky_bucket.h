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
 *
 * \file leaky_bucket.h
 *
 * \brief
 *    Header for Leaky Buffer parameters
 *
 * \author
 *    Main contributors (see contributors.h for copyright, address and affiliation details)
 *    - Shankar Regunathan                   <shanre@microsoft.com>
 **************************************************************************/

#ifndef _LEAKY_BUCKET_H_
#define _LEAKY_BUCKET_H_

#include "global.h"

/* Leaky Bucket Parameter Optimization */
#ifdef _LEAKYBUCKET_
int get_LeakyBucketRate(unsigned long NumberLeakyBuckets, unsigned long *Rmin);
void PutBigDoubleWord(unsigned long dw, FILE *fp);
void write_buffer(unsigned long NumberLeakyBuckets, unsigned long Rmin[], unsigned long Bmin[], unsigned long Fmin[]);
void Sort(unsigned long NumberLeakyBuckets, unsigned long *Rmin);
void calc_buffer();
#endif

#endif


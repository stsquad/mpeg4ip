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
 * \file refbuf.h
 *
 * \brief
 *    Declarations of the reference frame buffer types and functions
 ************************************************************************
 */
#ifndef _REBUF_H_
#define _REBUF_H_

#define HACK

#include "global.h"

#ifdef HACK

pel_t UMVPelY_14 (pel_t **Pic, int y, int x);
pel_t FastPelY_14 (pel_t **Pic, int y, int x);

pel_t UMVPelY_18 (pel_t **Pic, int y, int x);
pel_t FastPelY_18 (pel_t **Pic, int y, int x);

pel_t UMVPelY_11 (pel_t *Pic, int y, int x);
pel_t FastPelY_11 (pel_t *Pic, int y, int x);
pel_t *FastLine16Y_11 (pel_t *Pic, int y, int x);
pel_t *UMVLine16Y_11 (pel_t *Pic, int y, int x);

void PutPel_14 (pel_t **Pic, int y, int x, pel_t val);
void PutPel_11 (pel_t *Pic, int y, int x, pel_t val);

#endif


#ifndef HACK

typedef struct {
  int   Id;     // reference buffer ID, TR or Annex U style
  // The planes
  pel_t *y;     // Buffer of Y, U, V pels, Semantic is hidden in refbuf.c
  pel_t *u;     // Currently this is a 1/4 pel buffer
  pel_t *v;

  // Plane geometric/storage characteristics
  int   x_ysize;  // Size of Y buffer in columns, should be aligned to machine
  int   y_ysize;  // chracateristics such as word and cache line size, will be
            // set by the alloc routines
  int   x_uvsize;
  int   y_uvsize;

  // Active pixels in plane
  int   x_yfirst; // First active column, measured in 1/1 pel
  int   x_ylast;  // Last active column
  int   y_yfirst; // First active row, measure in 1/1 pel
  int   y_ylast;  // Last acrive row

  int   x_uvfirst;
  int   x_uvlast;
  int   y_uvfirst;
  int   y_uvlast;

} refpic_t;


// Alloc and free for reference buffers

refpic_t *AllocRefPic (int Id,
            int NumCols,
            int NumRows,
            int MaxMotionVectorX,   // MV Size may be used to allocate additional
            int MaxMotionVectorY);  // memory around boundaries fro UMV search

int FreeRefPic (refpic_t *Pic);

// Access functions for full pel (1/1 pel)

pel_t PelY_11 (refpic_t *Pic, int y, int x);
pel_t PelU_11 (refpic_t *Pic, int y, int x);
pel_t PelV_11 (refpic_t *Pic, int y, int x);

pel_t *MBLineY_11 (refpic_t *Pic, int y, int x);

// Access functions for half pel (1/2 pel)

pel_t PelY_12 (refpic_t *Pic, int y, int x);


// Access functions for quater pel (1/4 pel)

pel_t PelY_14 (refpic_t *Pic, int y, int x);


// Access functions for one-eigths pel (1/8 pel)

pel_t PelY_18 (refpic_t *Pic, int y, int x);


#endif

#endif


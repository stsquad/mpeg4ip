/* 
 * Copyright (c) 1997 NextLevel Systems of Delaware, Inc.  All rights reserved.
 * 
 * This software module  was developed by  Bob Eifrig (at NextLevel
 * Systems of Delaware, Inc.), Xuemin Chen (at NextLevel Systems of
 * Delaware, Inc.), and Ajay Luthra (at NextLevel Systems of Delaware,
 * Inc.), in the course of development of the MPEG-4 Video Standard
 * (ISO/IEC 14496-2).   This software module is an implementation of a
 * part of one or more tools as specified by the MPEG-4 Video Standard.
 * 
 * NextLevel Systems of Delaware, Inc. grants the right, under its
 * copyright in this software module, to use this software module and to
 * make modifications to it for use in products which conform to the
 * MPEG-4 Video Standard.  No license is granted for any use in
 * connection with products which do not conform to the MPEG-4 Video
 * Standard.
 * 
 * Those intending to use this software module are advised that such use
 * may infringe existing and unissued patents.  Please note that in
 * order to practice the MPEG-4 Video Standard, a license may be
 * required to certain patents held by NextLevel Systems of Delaware,
 * Inc., its parent or affiliates ("NextLevel").   The provision of this
 * software module conveys no license, express or implied, under any
 * patent rights of NextLevel or of any third party.  This software
 * module is subject to change without notice.  NextLevel assumes no
 * responsibility for any errors that may appear in this software
 * module.  NEXTLEVEL DISCLAIMS ALL WARRANTIES, EXPRESS AND IMPLIED,
 * INCLUDING, BUT NOT LIMITED TO ANY WARRANTY THAT COMPLIANCE WITH OR
 * PRACTICE OF THE SPECIFICATIONS OR USE OF THIS SOFTWARE MODULE WILL
 * NOT INFRINGE THE INTELLECTUAL PROPERTY RIGHTS OF NEXTLEVEL OR ANY
 * THIRD PARTY, AND ANY IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS FOR A PARTICULAR PURPOSE.
 * 
 * NextLevel retains the full right to use this software module for its
 * own purposes, to assign or transfer this software module to others,
 * to prevent others from using this software module in connection with
 * products which do not conform to the MPEG-4 Video Standard, and to
 * prevent others from infringing NextLevel's patents.
 * 
 * As an express condition of the above license grant, users are
 * required to include this copyright notice in all copies or derivative
 * works of this software module.
 */
/* MPEG-2 Video TM-5 rate control 
   For the purpose of comparing
   MPEG-4 VM with MPEG-2 TM-5
   27.03.97 By X. Chen in G.I.
 * 23.01.99 David Ruhoff: (double) cast to dj calc per Sarnoff labs
 * 16.03.99 David Ruhoff: conversion to microsoft-vfdis-v10-990124
 * 17.08.99 Yoshinori Suzuki(Hitachi): added S-VOP(GMC)
 */

#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <math.h>
#include "header.h"
#include "typeapi.h"
//#include "codehead.h"
//#include "entropy/bitstrm.hpp"
#include "entropy.hpp"
#include "huffman.hpp"
#include "mode.hpp"
//#include "global.hpp"
#include "vopses.hpp"
#include "vopseenc.hpp"
#include "tm5rc.hpp"



void TM5rc::tm5rc_init_seq( char       *pchQname,
							UInt       rc_type_arg, 
							AlphaUsage fAUsage, 
							UInt       uiWidth, 
							UInt       uiHeight, 
							UInt       uiBitRate,
							Double     dFrameRate
						   )
{
  if (fAUsage != RECTANGLE) {
      fprintf(stderr, "TM5 rate control requires a rectangular VOP\n");
      exit(1);
  }

  rc_type = rc_type_arg;

  switch (rc_type) {

  case RC_TM5:
      break;

  case RC_TM5+1:
      assert(pchQname);
      assert(*pchQname);
      if ((Qfile = fopen(pchQname, "r")) == NULL) {
          fprintf(stderr, "Cannot open %s\n", pchQname);
          exit(1);
      }
      break;

  case RC_TM5+2:
      assert(pchQname);
      assert(*pchQname);
      if ((Qfile = fopen(pchQname, "w")) == NULL) {
          fprintf(stderr, "Cannot open %s\n", pchQname);
          exit(1);
      }
      break;

  default:
      fprintf(stderr, "Invalid rate control code: %d\n", rc_type);
      exit(1);
  }

  mb_width = (uiWidth + MB_SIZE - 1) / MB_SIZE;
  mb_height = (uiHeight + MB_SIZE - 1) / MB_SIZE;
  bitrate = uiBitRate;
  pic_rate = dFrameRate;
  /* memory leak -- need to free when done */
  mbact = new double [mb_width * mb_height];

  /* reaction parameter (constant) */
  /* if (r_tm5==0)  */
      r_tm5 = (int)floor(2.0*bitrate/pic_rate + 0.5);

  /* average activity */
  if (avg_act==0.0)  avg_act = 400.0;

  /* remaining # of bits in GOP */
  R_tm5 = 0;

  /* global complexity measure */
  if (Xi==0) Xi = (int)floor(160.0*bitrate/115.0 + 0.5);
  if (Xp==0) Xp = (int)floor( 60.0*bitrate/115.0 + 0.5);
  if (Xb==0) Xb = (int)floor( 42.0*bitrate/115.0 + 0.5);

  /* virtual buffer fullness */
  if (d0i==0) d0i = (int)floor(10.0*r_tm5/31.0 + 0.5); // ***** NOT CORRECT FOR NBIT *****
  if (d0p==0) d0p = (int)floor(10.0*r_tm5/31.0 + 0.5);
  if (d0b==0) d0b = (int)floor(1.4*10.0*r_tm5/31.0 + 0.5);

  cout.sync_with_stdio();
  fprintf(stdout,"rate control: sequence initialization\n");
  fprintf(stdout," initial global complexity measures (I,P,B): Xi=%d, Xp=%d, Xb=%d\n",
    Xi, Xp, Xb);
  fprintf(stdout," reaction parameter: r_tm5=%d\n", r_tm5);
  fprintf(stdout," initial virtual buffer fullness (I,P,B): d0i=%d, d0p=%d, d0b=%d\n",
    d0i, d0p, d0b);
  fprintf(stdout," initial average activity: avg_act=%.1f\n\n", avg_act);

}

void TM5rc::tm5rc_init_GOP(int np,int nb)
{

  if ((Ni + Np + Nb) != 0) {
      fprintf(stderr, "TM5: Short GOP, expected %d/%d/%d more I/P/B-pics\n",
          Ni, Np, Nb);
      exit(1);
  }
  R_tm5 += (int) floor((1 + np + nb) * bitrate / pic_rate + 0.5);
  Np = np;
  Nb = nb;
  Ni = 1;

  cout.sync_with_stdio();
  fprintf(stdout,"\nrate control: new group of pictures (GOP)\n");
  fprintf(stdout," target number of bits for GOP: R_tm5=%d\n",R_tm5);
  fprintf(stdout," number of P pictures in GOP: Np=%d\n",Np);
  fprintf(stdout," number of B pictures in GOP: Nb=%d\n\n",Nb);
}

/* Note: we need to substitute K for the 1.4 and 1.0 constants -- this can
   be modified to fit image content */

/* Step 1: compute target bits for current picture being coded */
void TM5rc::tm5rc_init_pict(VOPpredType vopPredType,
                            const PixelC* ppxlcOrigY,
                            Int row_size,
                            Int iNumMBX,
                            Int iNumMBY)
{
  Double Tmin;
  Int type;

  switch (rc_type) {
  
  case RC_TM5+1:
      type = -1;
      if ((fscanf(Qfile, "%d", &type) != 1) || (type != vopPredType)) {
          fprintf(stderr, "Wrong pictue type: got %d, expected %d\n",
              type, vopPredType);
          exit(1);
      }
      return;

  case RC_TM5+2:
      fprintf(Qfile, "%d ", vopPredType);
      break;
  }

  switch (vopPredType)
  {
  default:
    fprintf(stderr, "Invalid vopPredType code: %d\n", vopPredType);
    exit(1);  	
  case IVOP:
    T_tm5 = (int) floor(R_tm5/(1.0+Np*Xp/(Xi*1.0)+Nb*Xb/(Xi*1.4)) + 0.5);
    d_tm5 = d0i;
    break;
  case PVOP:
  case SPRITE: // GMC_V2
    T_tm5 = (int) floor(R_tm5/(Np+Nb*1.0*Xb/(1.4*Xp)) + 0.5);
    d_tm5 = d0p;
    break;
  case BVOP:
    T_tm5 = (int) floor(R_tm5/(Nb+Np*1.4*Xp/(1.0*Xb)) + 0.5);
    d_tm5 = d0b;
    break;
  }

  Tmin = (int) floor(bitrate/(8.0*pic_rate) + 0.5);

  if (T_tm5<Tmin)
    T_tm5 = (int)Tmin;

  Q_tm5 = 0;

  tm5_calc_actj(ppxlcOrigY, row_size, iNumMBX, iNumMBY);

  actsum = 0.0;

  cout.sync_with_stdio();
  fprintf(stdout,"rate control: start of picture\n");
  fprintf(stdout," target number of bits: T_tm5=%d\n",T_tm5);
  fflush(stdout);
//printf("ppxlcOrigY=%p row_size=%d iNumMBX=%d iNumMBY=%d\n", 
// ppxlcOrigY, row_size, iNumMBX, iNumMBY);
}

void TM5rc::tm5_calc_actj(const PixelC* ppxlcOrigY,
                          Int row_size,
                          Int iNumMBX,
                          Int iNumMBY)
{
  assert (MB_SIZE == 2*BLOCK_SIZE);
  Double actj, var;
  Int    k = 0;
  Int    iMBY, iMBX;
  for (iMBY=0; iMBY<iNumMBY; iMBY++) {
    const PixelC* ppxlcOrigMBY = ppxlcOrigY;
    for (iMBX=0; iMBX<iNumMBX; iMBX++) {

      /* take minimum spatial activity measure of luminance blocks */

      actj = tm5_var_sblk(ppxlcOrigMBY,                                row_size);
      var  = tm5_var_sblk(ppxlcOrigMBY+BLOCK_SIZE,                     row_size);
      if (var<actj) actj = var;
      var  = tm5_var_sblk(ppxlcOrigMBY+BLOCK_SIZE*row_size,            row_size);
      if (var<actj) actj = var;
      var  = tm5_var_sblk(ppxlcOrigMBY+BLOCK_SIZE*row_size+BLOCK_SIZE, row_size);
      if (var<actj) actj = var;
    
      actj += 1.0;
      mbact[k++] = actj;
      
      ppxlcOrigMBY += MB_SIZE;
    }
    ppxlcOrigY += row_size*MB_SIZE;
  }
}


/* compute variance of 8x8 block */
Double TM5rc::tm5_var_sblk(const PixelC *p, Int lx)
{
  int i, j;
  unsigned int v, s, s2;

  assert (BLOCK_SIZE == 8);
  s = s2 = 0;

  for (j=0; j<8; j++)
  {
    for (i=0; i<8; i++)
    {
      v = *p++;
      s+= v;
      s2+= v*v;
    }
    p+= lx - 8;
  }

  return s2/64.0 - (s/64.0)*(s/64.0);
}


/* compute initial quantization stepsize (at the beginning of picture) */
Int TM5rc::tm5rc_start_mb()
{
    Int mquant;

    if (rc_type == RC_TM5+1) {
        if (fscanf(Qfile, "%d", &mquant) != 1) {
            fprintf(stderr, "Q file read error\n");
            exit(1);
        }
        return mquant;
    }

    mquant = (int) floor(d_tm5*31.0/r_tm5 + 0.5); // ***** NOT CORRECT FOR NBIT ************
    
    /* clip mquant to legal range */
    if (mquant<1)
      mquant = 1;
    if (mquant>31)  // ***** NOT CORRECT FOR NBIT ************
      mquant = 31;

    prev_mquant = mquant = mquant;

    if (rc_type == RC_TM5+2) {
        fprintf(Qfile, "%2d\n", mquant);
        linectr = 0;
    }
	cout.sync_with_stdio(); printf(" tm5rc_start_mb mquant=%d\n",mquant);
//fprintf(stdout,"\tj\tsize\tmquant\td_tm5\tT_tm5\tdj\tQj\tactj\tN_actj\n");
    return mquant;
}


/* Step 2: measure virtual buffer - estimated buffer discrepancy */
Int TM5rc::tm5rc_calc_mquant(Int j, UInt size)
{
  int mquant;
  double dj, Qj, actj, N_actj;

  if (rc_type == RC_TM5+1) {
      if (fscanf(Qfile, "%d", &mquant) != 1) {
         fprintf(stderr, "Q file read error\n");
         exit(1);
      }
      return mquant;
  }

  /* measure virtual buffer discrepancy from uniform distribution model */
  dj = (double)((double)d_tm5 + (double)size - (double)j*((double)T_tm5/((double)mb_width*(double)mb_height)));
/*
fprintf(stdout,"dj=%.5f: d_tm5=%.3f, size=%d, j=%d, T_tm5=%.3f, width=%d  height=%d\n",
   (float)dj, (float)d_tm5, (int)size, (int)j, (float)T_tm5, (int)mb_width, (int)mb_height);
*/

  /* scale against dynamic range of mquant and the bits/picture count */
  Qj = dj*31.0/r_tm5; // ***** NOT CORRECT FOR NBIT ************

  actj = mbact[j];
  actsum += actj;

  /* compute normalized activity */
  N_actj = (2.0*actj+avg_act)/(actj+2.0*avg_act);

    /* modulate mquant with combined buffer and local activity measures */
    mquant = (int) floor(Qj*N_actj + 0.5);
   
    /* clip mquant to legal range */
    if (mquant<1)
      mquant = 1;
    if (mquant>31) // ***** NOT CORRECT FOR NBIT ************
      mquant = 31;

    mquant = mquant;
 
  Q_tm5 += mquant; /* for calculation of average mquant */

  if (rc_type == RC_TM5+2) {
      fprintf(Qfile, "%2d ", mquant);
      if (++(linectr) >= mb_width) {
          fprintf(Qfile, "\n");
          linectr = 0;
      }
  }

//fprintf(stdout,"\t%d\t%d\t%d\t%d\t%d\t%g\t%g\t%g\t%g\n",
//   j, size, mquant, d_tm5, T_tm5, dj, Qj, actj, N_actj);

  return mquant;
}


void TM5rc::tm5rc_update_pict(VOPpredType vopPredType, Int iVOPtotalBits)
{
  double X;

  if (rc_type == RC_TM5+1) {
      switch (vopPredType) {
          case IVOP: Ni--; break;
          case PVOP: Np--; break;
          case BVOP: Nb--; break;
          case SPRITE: Np--; break;	// GMC_V2
      default: break;
      }
      return;
  }
                        /* total # of bits in picture */
  R_tm5 -= iVOPtotalBits; /* remaining # of bits in GOV */
  X = (int) floor(iVOPtotalBits*((0.5*(double)(Q_tm5))/
      (mb_width*mb_height)) + 0.5);
  d_tm5 += iVOPtotalBits - T_tm5;
  avg_act = actsum/(mb_width*mb_height);

  switch (vopPredType)
  {
  case IVOP:
    Xi = (int)X;
    d0i = d_tm5;
    if (--(Ni) < 0) {
        fprintf(stderr, "TM5: too many I-pics in GOP\n");
        exit(1);
    }
    break;
  case PVOP:
  case SPRITE:	// GMC_V2
    Xp = (int)X;
    d0p = d_tm5;
    if (--(Np) < 0) {
        fprintf(stderr, "TM5: too many P-pics in GOP\n");
        exit(1);
    }
    break;
  case BVOP:
    Xb = (int)X;
    d0b = d_tm5;
    if (--(Nb) < 0) {
        fprintf(stderr, "TM5: too many B-pics in GOP\n");
        exit(1);
    }
    break;
  default:
    fprintf(stderr, "Invalid vopPredType code: %d\n", vopPredType);
    exit(1);  	
  }

  cout.sync_with_stdio();
  fprintf(stdout,"rate control: end of picture\n");
  fprintf(stdout," actual number of bits=%d\n",iVOPtotalBits);
  fprintf(stdout," average quantization parameter Q_tm5=%.1f\n",
    (double)Q_tm5/(mb_width*mb_height));
  fprintf(stdout," remaining number of bits in GOP: R_tm5=%d\n",R_tm5);
  fprintf(stdout," global complexity measures (I,P,B): Xi=%d, Xp=%d, Xb=%d\n",
    Xi, Xp, Xb);
  fprintf(stdout," virtual buffer fullness (I,P,B): d0i=%d, d0p=%d, d0b=%d\n",
    d0i, d0p, d0b);
  fprintf(stdout," remaining number of P pictures in GOP: Np=%d\n",Np);
  fprintf(stdout," remaining number of B pictures in GOP: Nb=%d\n",Nb);
  fprintf(stdout," average activity: avg_act=%.1f\n", avg_act);
  fflush(stdout);
}


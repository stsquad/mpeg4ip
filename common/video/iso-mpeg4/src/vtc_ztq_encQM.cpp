/* $Id: vtc_ztq_encQM.cpp,v 1.1 2005/05/09 21:29:49 wmaycisco Exp $ */
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
/*                                                                          */
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

/************************************************************/
/*     Sarnoff Very Low Bit Rate Still Image Coder          */
/*     Copyright 1995, 1996, 1997, 1998 Sarnoff Corporation */
/************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>  // hjlee 0901
#include "dataStruct.hpp"
#include "globals.hpp"
#include "states.hpp"
#include "quant.hpp"
#include "Utils.hpp"
#include "QMUtils.hpp"
#include "QM.hpp"


/* 
   Function Name
   -------------
   static Void quantizeCoeff(Int x, Int y)

   Arguments 
   ---------
   Int x, Int y - Index of wavelet coefficient (row, col) to quantize.

   Description
   -----------
   Quantize the coefficent at (x,y). Also get maximum number of residual 
   levels for arithmetic coding in SE module.

   coeffinfo Updates:
     1. quantized_value
     2. qState

   Functions Called
   ----------------
   none

   Return Value
   ------------
   none

   Source File
   -----------
   encQM.c
*/

Void CVTCEncoder::quantizeCoeff(Int x, Int y, Int c)
{
  Int dumQ=0;

  /* assign new quantization value - update quant state */
  if (mzte_codec.m_iQuantType == MULTIPLE_Q)
    COEFF_VAL(x,y,c) = 
      quantSingleStage(USER_Q(c), &COEFF_QSTATE(x,y,c),
		       &prevQList2[c][coordToSpatialLev(x,y,c)], 0);
  else if (mzte_codec.m_iQuantType == SINGLE_Q)
    COEFF_VAL(x,y,c) = quantSingleStage(USER_Q(c), &COEFF_QSTATE(x,y,c),
					&dumQ, 0);

}



/* 
   Function Name
   -------------
   static Int quantizeAndMarkCoeffs(Int x, Int y)

   Arguments 
   ---------
   Int x, Int y - Index of wavelet coefficient (row, col) to mark.

   Description
   -----------
   Quantize and mark the coefficient at (x,y) and all of it's descendents.
   Uses recursion to accomplish this.

   Functions Called
   ----------------
   findChild
   quantizeAndMarkCoeffs (recursive call)
   quantizeCoeff
   markCoeffandUpdateState
   isIndexInRootBands

   Return Value
   ------------
   Returns 1, if there are non-zero descendents, and 0, if not.

   Source File
   -----------
   encQM.c
*/

Int CVTCEncoder::quantizeAndMarkCoeffs(Int x, Int y, Int c)
{
  Int i;
  UChar valDes, valDesOneBranch; /* 'some non-zero descendent' flags */
  UChar isLeaf; /* is (x,y) a leaf (no children) */
  Int xc[4], yc[4]; /* coords of children */
  Int nc; /* number of children */

  valDes = 0;

  /* Are we at a leaf? */
  if ((nc = findChild(x, y, xc, yc, c)) == 0)
    isLeaf = 1;
  else
  {
    isLeaf = 0;
    /* No - mark descendents in all branches */
    for (i = 0; i < nc; ++i)
    {
      valDesOneBranch = quantizeAndMarkCoeffs(xc[i], yc[i], c);
      valDes = valDes || valDesOneBranch;
    }
  }
  
  /* quantize the current coefficent */
  quantizeCoeff(x, y, c);    
  
  /* Updates type in coeffTable */
  markCoeff(x, y, valDes, c);

    /* update maximums */
  if (!IS_RESID(x,y,c))
  {

    {
      Int l,v;

      l = xy2wvtDecompLev(x,y);
      v = ceilLog2(ABS(COEFF_VAL(x,y,c)));
      if (WVTDECOMP_NUMBITPLANES(c,l) < v)
	WVTDECOMP_NUMBITPLANES(c,l) = v;
    }

  }

  /* return zero status of current coeff and all descendents */
   return IS_RESID(x,y,c) || COEFF_VAL(x,y,c) || valDes;

}

/* 
   Function Name
   -------------
   Int encQuantizeDC()

   Arguments 
   ---------
   none

   Description
   -----------
   Quantize all DC coefficients. Get maximum dc value and offset.

   Functions Called
   ----------------
   quantSingleStage

   Return Value
   ------------
   0, if everything's ok. <0, if error.

   Source File
   -----------
   encQM.c
*/


Int CVTCEncoder::encQuantizeDC(Int c)
{
  Int err;
  Int x, y;
 
  err=0;

  /* initialize maximum, offset */
  mzte_codec.m_iOffsetDC = 0;
  mzte_codec.m_iMaxDC = 0;

  /* loop through DC */
  noteDetail("Quantizing DC band....");
  noteDebug("Qdc=%d",mzte_codec.m_iQDC[c]);
  for (x = 0; x < mzte_codec.m_iDCWidth; ++x)
    for (y = 0; y < mzte_codec.m_iDCHeight; ++y)
    {
       /* assign new quantization value */
      {
       Int val = COEFF_ORGVAL(x,y,c);
       Int q = mzte_codec.m_iQDC[c];
       if (val > 0) 
          val = ((val<<1) + q) / (q<<1);
       else if (val < 0) 
          val = ((val<<1) - q) / (q<<1);
       COEFF_VAL(x,y,c) = val;
      }


      if (mzte_codec.m_iOffsetDC > COEFF_VAL(x,y,c))
	mzte_codec.m_iOffsetDC = COEFF_VAL(x,y,c);
      if (mzte_codec.m_iMaxDC < COEFF_VAL(x,y,c))
	mzte_codec.m_iMaxDC = COEFF_VAL(x,y,c);
    }

  noteDetail("Completed quantizing DC bands.");

  return err;
}

/* 
   Function Name
   -------------
   Int encQuantizeAndMarkAC()

   Arguments 
   ---------
   none.

   Description
   -----------
   Quantize and mark of all AC coefficients in current 
   spatial layer. Get maximum values for root, valz, valnz, and resid types.
   Mark all zero flag.

   Functions Called
   ----------------
   quantizeAndMarkCoeffs

   Return Value
   ------------
   0, if everything's ok. <0, if error.

   Source File
   -----------
   encQM.c
*/

Int CVTCEncoder::encQuantizeAndMarkAC(Int c)
{
  Int err;
  Int x, y;
  Int nc;
  Int xc[3], yc[3];

  err=0;

  /* hjlee 0901 */
  /* initialize maximums */
  {
    Int l;

    for (l=0; l<mzte_codec.m_iWvtDecmpLev; ++l)  // hjlee BUG
		WVTDECOMP_NUMBITPLANES(c,l)=0;
  }

  /* loop through DC */
  noteDetail("Quantizing and marking AC bands....");
  ALL_ZERO(c)=1;
  for (x = 0; x < mzte_codec.m_iDCWidth; ++x)
    for (y = 0; y < mzte_codec.m_iDCHeight; ++y)
    {
      if ((nc = findChild(x, y, xc, yc, c)) != 3)
      {
	noteError("DC band coefficient has %d children instead of 3.", nc);
	exit(-1);
      }
	
      ALL_ZERO(c) *= !quantizeAndMarkCoeffs(xc[0], yc[0], c);
      ALL_ZERO(c) *= !quantizeAndMarkCoeffs(xc[1], yc[1], c);
      ALL_ZERO(c) *= !quantizeAndMarkCoeffs(xc[2], yc[2], c);
    }
  noteDetail("Completed quantizing and marking of AC bands.");

  if (ALL_ZERO(c))
    noteProgress("Note: All coefficients are quantized to zero.");

  return err;
}

/* 
   Function Name
   -------------
   Int encUpdateStateAC()

   Arguments 
   ---------
   none.

   Description
   -----------
   Update state and prob model of layer.

   Functions Called
   ----------------
   updateCoeffStateAndModel

   Return Value
   ------------
   0, if everything's ok. <0, if error.

   Source File
   -----------
   encQM.c
*/

Int CVTCEncoder::encUpdateStateAC(Int c)
{
  Int err;
  Int x, y;
  Int nc;
  Int xc[3], yc[3];

  err=0;

  /* loop through DC */
  noteDetail("Updating state of AC bands....");
  for (x = 0; x < mzte_codec.m_iDCWidth; ++x)
    for (y = 0; y < mzte_codec.m_iDCHeight; ++y)
    {
      if ((nc = findChild(x, y, xc, yc, c)) != 3)
      {
	noteError("DC band coefficient has %d children instead of 3.", nc);
	exit(-1);
      }
      
      updateCoeffAndDescState(xc[0], yc[0], c);
      updateCoeffAndDescState(xc[1], yc[1], c);
      updateCoeffAndDescState(xc[2], yc[2], c);
    }
  noteDetail("Completed updating state of AC bands.");

  return err;
}

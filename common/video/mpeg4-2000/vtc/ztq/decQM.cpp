/* $Id: decQM.cpp,v 1.1 2003/05/05 21:24:18 wmaycisco Exp $ */
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
#include "dataStruct.hpp"
#include "globals.hpp"
#include "states.hpp"
#include "quant.hpp"
#include "QMUtils.hpp"
#include "QM.hpp"
#include "errorHandler.hpp"


/* 
   Function Name
   -------------
   static Void iQuantizeCoeff(Int x, Int y)

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
   decQM.c
*/

Void CVTCDecoder::iQuantizeCoeff(Int x, Int y, Int c)
{
  Int dumQ=0;

  /* assign new quantization value - update quant state */
  if (mzte_codec.m_iQuantType == MULTIPLE_Q)
    COEFF_RECVAL(x,y,c) = 
      invQuantSingleStage(COEFF_VAL(x,y,c), USER_Q(c), &COEFF_QSTATE(x,y,c),
			  &prevQList2[c][coordToSpatialLev(x,y,c)], 0);
  else if (mzte_codec.m_iQuantType == SINGLE_Q)
    COEFF_RECVAL(x,y,c) = 
      invQuantSingleStage(COEFF_VAL(x,y,c), USER_Q(c), &COEFF_QSTATE(x,y,c),
			  &dumQ, 0);

}

/* 
   Function Name
   -------------
   static Void iQuantizeCoeffs(Int x, Int y)

   Arguments 
   ---------
   Int x, Int y - Index of wavelet coefficient (row, col) to mark.

   Description
   -----------
   Inverse quantize coefficient at (x,y) and all of it's descendents.
   Uses recursion to accomplish this.

   Functions Called
   ----------------
   findChild
   iQuantizeCoeffs (recursive call)
   iQuantizeCoeff

   Return Value
   ------------
   none.

   Source File
   -----------
   decQM.c
*/

Void CVTCDecoder::iQuantizeCoeffs(Int x, Int y, Int c)
{
  Int i;
  Int xc[4], yc[4]; /* coords of children */
  Int nc; /* number of children */

  /* Are we at a leaf? */
  if ((nc = findChild(x, y, xc, yc, c)) > 0)
  {
    /* No - mark descendents in all branches */
    for (i = 0; i < nc; ++i)
      iQuantizeCoeffs(xc[i], yc[i], c);
  }

  /* inverse quantize the current coefficent */
  iQuantizeCoeff(x,y, c);    
}


/* 
   Function Name
   -------------
   Int decIQuantizeDC()

   Arguments 
   ---------
   none

   Description
   -----------
   Inverse quantize all DC coefficients.

   Functions Called
   ----------------
   invQuantSingleStage

   Return Value
   ------------
   0, if everything's ok. <0, if error.

   Source File
   -----------
   decQM.c
*/

Int CVTCDecoder::decIQuantizeDC(Int c)
{
  Int err;
  Int x, y;
 
  err=0;

  /* loop through DC */
  noteDetail("Inverse Quantizing DC band....");
  for (x = 0; x < mzte_codec.m_iDCWidth; ++x)
    for (y = 0; y < mzte_codec.m_iDCHeight; ++y)
    {
       /* assign new quantization value */

      COEFF_RECVAL(x,y,c) = COEFF_VAL(x,y,c) * mzte_codec.m_iQDC[c];
    }

  noteDetail("Completed inverse Quantizing DC bands.");

  return err;
}

/* 
   Function Name
   -------------
   Int decIQuantizeAC()

   Arguments 
   ---------
   none

   Description
   -----------
   Inverse quantize all AC coefficients.

   Functions Called
   ----------------
   iQuantizeCoeffs

   Return Value
   ------------
   0, if everything's ok. <0, if error.

   Source File
   -----------
   decQM.c
*/

Int CVTCDecoder::decIQuantizeAC(Int c)
{
  Int err;
  Int x, y;
  Int nc;
  Int xc[3], yc[3];

  err=0;

  /* loop through DC */
  noteDetail("Inverse quantizing AC bands....");
  for (x = 0; x < mzte_codec.m_iDCWidth; ++x)
    for (y = 0; y < mzte_codec.m_iDCHeight; ++y)
    {
      if ((nc = findChild(x, y, xc, yc,c)) != 3)
      {
	noteError("DC band coefficient has %d children instead of 3.", nc);
	exit(-1);
      }
	
      iQuantizeCoeffs(xc[0], yc[0],c);
      iQuantizeCoeffs(xc[1], yc[1],c);
      iQuantizeCoeffs(xc[2], yc[2],c);
    }

  noteDetail("Completed inverse quantizing of AC bands.");

  return err;
}


/********************************************
  Inverse quantize the 3 difference bands 
  between two spatial consecutive spatial
  layers.
  *****************************************/
Int CVTCDecoder::decIQuantizeAC_spa(Int spa_lev,Int c)
{
  Int err,h,w,hstart,wstart,hend,wend;

  err=0;

  /* loop through DC */
  noteDetail("Inverse quantizing AC bands (difference)....");

  hend=mzte_codec.m_SPlayer[c].height;
  wend=mzte_codec.m_SPlayer[c].width;

  if (spa_lev==FIRST_SPA_LEV(mzte_codec.m_iSpatialLev,
			     mzte_codec.m_iWvtDecompLev, c))
  {
    hstart=mzte_codec.m_iDCHeight;
    wstart=mzte_codec.m_iDCWidth;
  }
  else
  {
    hstart=hend/2;
    wstart=wend/2;
  }

  for(h=0;h<hstart;h++)
    for(w=wstart;w<wend;w++)
      iQuantizeCoeff(w,h,c);

  for(h=hstart;h<hend;h++)
    for(w=0;w<wend;w++)
      iQuantizeCoeff(w,h,c);

  noteDetail("Completed inverse quantizing of AC bands.");

  return(err);

}


/* 
   Function Name
   -------------
   Int decUpdateStateAC()

   Arguments 
   ---------
   none

   Description
   -----------
   Update state and prob model of all AC coefficients.

   Functions Called
   ----------------
   updateCoeffStateAndModel

   Return Value
   ------------
   0, if everything's ok. <0, if error.

   Source File
   -----------
   decQM.c
*/

Int CVTCDecoder::decUpdateStateAC(Int c)
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
      if ((nc = findChild(x, y, xc, yc,c)) != 3)
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


/* 
   Function Name
   -------------
   Int decUpdateStateAC_spa()

   Arguments 
   ---------
   none

   Description
   -----------
   Update state and prob model of all AC coefficients.

   Functions Called
   ----------------
   updateCoeffStateAndModel

   Return Value
   ------------
   0, if everything's ok. <0, if error.

   Source File
   -----------
   decQM.c
*/

/* hjlee 0901 */
Int CVTCDecoder::decUpdateStateAC_spa(Int c)
{  
  Int err,h,w,hstart,wstart,hend,wend;

  err=0;

  /* loop through DC */
  noteDetail("Updating state of AC bands (difference)....");

  hstart=mzte_codec.m_SPlayer[c].height/2;
  wstart=mzte_codec.m_SPlayer[c].width/2;
  hend=mzte_codec.m_SPlayer[c].height;
  wend=mzte_codec.m_SPlayer[c].width;

  for(h=0;h<hstart;h++)
    for(w=wstart;w<wend;w++)
      updateCoeffAndDescState(w, h, c);

  for(h=hstart;h<hend;h++)
    for(w=0;w<wend;w++)
      updateCoeffAndDescState(w, h, c);
   
  noteDetail("Completed updating state of AC bands.");

  return err;
}


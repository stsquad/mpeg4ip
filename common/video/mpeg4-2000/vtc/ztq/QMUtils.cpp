/* $Id: QMUtils.cpp,v 1.1 2003/05/05 21:24:17 wmaycisco Exp $ */
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
#include "dataStruct.hpp"
#include "globals.hpp"
#include "states.hpp"
#include "QM.hpp"
#include "QMUtils.hpp"
#include "errorHandler.hpp"

/* 
   Function Name
   -------------
   Int findChild(Int x, Int y, Int xc[], Int yc[])

   Arguments 
   ---------
   Int x, Int y - Index of wavelet coefficient (row, col) to check.
   Int xc[], Int yc[] - Indices of the (maximum of 4) children of (x,y).

   Description
   -----------
   findChild returns the number of children of (x,y). If there are 
   children it puts their indices in (xc, yc).
   
   mzte_codec.dcWidth, mzte_codec.dcHeight, mzte_codec.SPlayer.width, and 
   mzte_codec.SPlayer.height must all be set to their proper values.

   Functions Called
   ----------------
   none

   Return Value
   ------------
   The number of children of (x,y).

   Source File
   -----------
   QMUtils.c
*/

Int CVTCCommon::findChild(Int x, Int y, Int xc[], Int yc[], Int c)
{
   Int numChildren;

   if (x < mzte_codec.m_iDCWidth && y < mzte_codec.m_iDCHeight)
   {
     /*------------------- DC Band ---------------------*/ 
     numChildren = 3;

     xc[0] = x + mzte_codec.m_iDCWidth;
     yc[0] = y;
     
     xc[1] = x;
     yc[1] = y + mzte_codec.m_iDCHeight;
     
     xc[2] = x + mzte_codec.m_iDCWidth;
     yc[2] = y + mzte_codec.m_iDCHeight;
   }
   else if((x *= 2) < mzte_codec.m_SPlayer[c].width && 
	   (y *= 2) < mzte_codec.m_SPlayer[c].height) 
   { 
     /*------------------- Non-Leaf AC Band ---------------------*/ 
     numChildren = 4;
     
     xc[0] = x;
     yc[0] = y;
     
     xc[1] = x+1;
     yc[1] = y  ;
     
     xc[2] = x  ;
     yc[2] = y+1;
     
     xc[3] = x+1;
     yc[3] = y+1;
   }
   else
     /*------------------- Leaf AC Band ---------------------*/ 
     numChildren = 0;
	
   return numChildren;
}


/* 
   Function Name
   -------------
   Int isIndexInRootBands(Int x, Int y)

   Arguments 
   ---------
   Int x, Int y - Index of wavelet coefficient (row, col) to check.

   Description
   -----------
   isIndexInRootBands checks if (x,y) is in a root band (first three AC
   bands which aren't leafs).
   
   mzte_codec.dcWidth, mzte_codec.dcHeight, mzte_codec.SPlayer.width, and 
   mzte_codec.SPlayer.height must all be set to their proper values.

   Functions Called
   ----------------
   none

   Return Value
   ------------
   The number of children of (x,y).

   Source File
   -----------
   QMUtils.c
*/

Int CVTCCommon::isIndexInRootBands(Int x, Int y, Int c)
{  
  Int x1, x2, y1, y2;

  x1 = mzte_codec.m_iDCWidth<<1;
  x2 = mzte_codec.m_SPlayer[c].width>>1;
  y1 = mzte_codec.m_iDCHeight<<1;
  y2 = mzte_codec.m_SPlayer[c].height>>1;

  return x < MIN(x1,x2) && y < MIN(y1,y2) 
    && (x >= mzte_codec.m_iDCWidth || y >= mzte_codec.m_iDCHeight);
}


/* 
   Function Name
   -------------
   Void markCoeff(Int x, Int y, UChar valuedDes)

   Arguments 
   ---------
   Int x, Int y - Index of wavelet coefficient (row, col) to mark.
   UChar valuedDes - Flag for existence of non-zero descendents at
     (x,y). 0 = all descendents are zero, 1 = some descendents are non-zero.

   Description
   -----------
   Mark type of current coefficient.

   Functions Called
   ----------------
   none

   Return Value
   ------------
   none

   Source File
   -----------
   QMUtils.c
*/


Void CVTCCommon::markCoeff(Int x, Int y, UChar valuedDes, Int c)
{		
  if (COEFF_STATE(x,y,c) == S_INIT || COEFF_STATE(x,y,c) == S_ZTR 
      || COEFF_STATE(x,y,c) == S_ZTR_D)
  {
    if (COEFF_VAL(x,y,c))
    {
      if (valuedDes)
	COEFF_TYPE(x,y,c) = VAL;
      else
	COEFF_TYPE(x,y,c) = VZTR;
    }
    else
    {
      if (valuedDes)
	COEFF_TYPE(x,y,c) = IZ;
      else
	COEFF_TYPE(x,y,c) = ZTR;
    }
  }
  else if (COEFF_STATE(x,y,c) == S_IZ)
  {
    if (COEFF_VAL(x,y,c))
      COEFF_TYPE(x,y,c) = VAL;
    else
      COEFF_TYPE(x,y,c) = IZ;
  }
  else if (COEFF_STATE(x,y,c) == S_VZTR)
  {
    if (valuedDes)
      COEFF_TYPE(x,y,c) = VAL;
    else
      COEFF_TYPE(x,y,c) = VZTR;
  }
  else if (COEFF_STATE(x,y,c) == S_VAL)
  {
    COEFF_TYPE(x,y,c) = VAL;
  }
  else if (COEFF_STATE(x,y,c) == S_LINIT || COEFF_STATE(x,y,c) == S_LZTR 
      || COEFF_STATE(x,y,c) == S_LZTR_D)
  {
    if (COEFF_VAL(x,y,c))
      COEFF_TYPE(x,y,c) = VZTR;
    else
      COEFF_TYPE(x,y,c) = ZTR;
  }
  else /* if (COEFF_STATE(x,y,c) == S_LVZTR) */
    COEFF_TYPE(x,y,c) = VZTR;
  
}


/* 
   Function Name
   -------------
   Void updateState(Int x, Int y, Int type)

   Arguments 
   ---------
   Int x, Int y - Index of wavelet coefficient (row, col) to mark.

   Description
   -----------
   Update the state of current coefficient.

   Functions Called
   ----------------
   none

   Return Value
   ------------
   none

   Source File
   -----------
   QMUtils.c
*/



Void CVTCCommon::updateState(Int x, Int y, Int type, Int c)
{
  switch(COEFF_STATE(x,y,c))
  {
    case S_INIT:
    case S_ZTR:
    case S_ZTR_D:
      switch (type)
      {
	case ZTR_D:
	  COEFF_STATE(x,y,c) = S_ZTR_D;
	  break;
	case ZTR:
	  COEFF_STATE(x,y,c) = S_ZTR;
	  break;
	case VZTR:
	  COEFF_STATE(x,y,c) = S_VZTR;
	  break;
	case IZ:
	  COEFF_STATE(x,y,c) = S_IZ;
	  break;
	case VAL:
	  COEFF_STATE(x,y,c) = S_VAL;
	  break;
	default:
	  errorHandler("updateState: type %d for state %d is invalid.",
		       type, COEFF_STATE(x,y,c));
      }
      break;
    case S_IZ:
      switch (type)
      {
	case IZ:
	  COEFF_STATE(x,y,c) = S_IZ;
	  break;
	case VAL:
	  COEFF_STATE(x,y,c) = S_VAL;
	  break;
	default:
	  errorHandler("updateState: type %d for state %d is invalid.",
		       type, COEFF_STATE(x,y,c));
      }
      break;
    case S_VZTR:
      switch (type)
      {
	case VZTR:
	  COEFF_STATE(x,y,c) = S_VZTR;
	  break;
	case VAL:
	  COEFF_STATE(x,y,c) = S_VAL;
	  break;
	default:
	  errorHandler("updateState: type %d for state %d is invalid.",
		       type, COEFF_STATE(x,y,c));
      }
      break;
    case S_LINIT:
    case S_LZTR:
    case S_LZTR_D:
      switch (type)
      {
	case ZTR_D:
	  COEFF_STATE(x,y,c) = S_LZTR_D;
	  break;
	case ZTR:
	  COEFF_STATE(x,y,c) = S_LZTR;
	  break;
	case VZTR:
	  COEFF_STATE(x,y,c) = S_LVZTR;
	  break;
	default:
	  errorHandler("updateState: type %d for state %d is invalid.",
		       type, COEFF_STATE(x,y,c));
      }
      break;
    case S_DC:
    case S_VAL:
    case S_LVZTR:
      break;
    default:
      errorHandler("updateState: state %d is invalid.", COEFF_STATE(x,y,c));
  }
}


/* 
   Function Name
   -------------
   Void updateCoeffAndDescState(Int x, Int y)

   Arguments 
   ---------
   Int x, Int y - Index of wavelet coefficient (row, col) to mark.

   Description
   -----------
   Update the state of coefficient at (x,y) and all of it's descendents. 
   Uses recursion to accomplish this.

   Functions Called
   ----------------
   findChild
   updateCoeffAndDescState (recursive call)
   updateState

   Return Value
   ------------
   none.

   Source File
   -----------
   QMUtils.c
*/

Void CVTCCommon::updateCoeffAndDescState(Int x, Int y, Int c)
{
  Int i;
  Int xc[4], yc[4]; /* coords of children */
  Int nc; /* number of children */

  /* Are we at a leaf? */
  if ((nc = findChild(x, y, xc, yc, c)) > 0)
  {
    /* No - mark descendents in all branches */
    for (i = 0; i < nc; ++i)
      updateCoeffAndDescState(xc[i], yc[i], c);
  }
  
  /* Updates state in coeffTable */
  updateState(x, y, COEFF_TYPE(x, y, c), c);
}

/* 
   Function Name
   -------------
   Void spatialLayerChangeUpdate()

   Arguments 
   ---------
   none.

   Description
   -----------
   Update the state of leaf coefficients when the spatial layer changes.
   Call right *after* spatial layer has increased. It is assummed that
   the increase was a doubling of the size (three more AC bands).

   Functions Called
   ----------------
   none.

   Return Value
   ------------
   none.

   Source File
   -----------
   QMUtils.c
*/



Void CVTCCommon::spatialLayerChangeUpdate(Int c)
{  
  Int x, y;
  Int oldWidth, oldHeight;
  Int xLeafStart, yLeafStart;

  noteDetail("Updating new coefficients in spatial layer for col %d....",c);

  oldWidth  = mzte_codec.m_spaLayerWidth[mzte_codec.m_iCurSpatialLev][c]>>1;
  oldHeight = mzte_codec.m_spaLayerHeight[mzte_codec.m_iCurSpatialLev][c]>>1;

  xLeafStart = mzte_codec.m_spaLayerWidth[mzte_codec.m_iCurSpatialLev-1][c]>>1;
  yLeafStart = mzte_codec.m_spaLayerHeight[mzte_codec.m_iCurSpatialLev-1][c]>>1;
 
  /* loop through leaf coefficients */
  /* HL */
  for (y = 0; y < yLeafStart; ++y)
    for (x = xLeafStart; x < oldWidth; ++x)
      /* update state */
      switch (COEFF_STATE(x,y,c))
      {
	case S_LVZTR:
	  COEFF_STATE(x,y,c) = S_VZTR;
	  break;
	case S_LZTR:
	  COEFF_STATE(x,y,c) = S_ZTR;
	  break;
	case S_LZTR_D:
	  COEFF_STATE(x,y,c) = S_ZTR_D;
	  break;
	case S_LINIT:
	  COEFF_STATE(x,y,c) = S_INIT;
	  break;
	default:
	  errorHandler("Non-leaf state (%d) for leaf coefficient at"\
		       "(x=%d, y=%d).", 
		       COEFF_STATE(x, y,c), x, y);
      }

  /* LH */
  for (y = yLeafStart; y < oldHeight; ++y)
    for (x = 0; x < xLeafStart; ++x)
      /* update state */
      switch (COEFF_STATE(x,y,c))
      {
	case S_LVZTR:
	  COEFF_STATE(x,y,c) = S_VZTR;
	  break;
	case S_LZTR:
	  COEFF_STATE(x,y,c) = S_ZTR;
	  break;
	case S_LZTR_D:
	  COEFF_STATE(x,y,c) = S_ZTR_D;
	  break;
	case S_LINIT:
	  COEFF_STATE(x,y,c) = S_INIT;
	  break;
	default:
	  errorHandler("Non-leaf state (%d) for leaf coefficient at"\
		       "(x=%d, y=%d).", 
		       COEFF_STATE(x, y,c), x, y);
      }
  
  /* HH */
  for (y = yLeafStart; y < oldHeight; ++y)
    for (x = xLeafStart; x < oldWidth; ++x)
      /* update state */
      switch (COEFF_STATE(x,y,c))
      {
	case S_LVZTR:
	  COEFF_STATE(x,y,c) = S_VZTR;
	  break;
	case S_LZTR:
	  COEFF_STATE(x,y,c) = S_ZTR;
	  break;
	case S_LZTR_D:
	  COEFF_STATE(x,y,c) = S_ZTR_D;
	  break;
	case S_LINIT:
	  COEFF_STATE(x,y,c) = S_INIT;
	  break;
	default:
	  errorHandler("Non-leaf state (%d) for leaf coefficient at"\
		       "(x=%d, y=%d).", 
		       COEFF_STATE(x, y,c), x, y);
      }

  noteDetail("Completed updating new coefficients in spatial layer.");

}


Int CVTCCommon::coordToSpatialLev(Int x, Int y, Int c)
{
  Int k;

  for (k = 0; k < mzte_codec.m_iSpatialLev; ++k)
    if (x < mzte_codec.m_spaLayerWidth[k][c] 
	&& y < mzte_codec.m_spaLayerHeight[k][c])
      return k;
  return(0);

}

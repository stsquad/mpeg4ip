/* $Id: QMInit.cpp,v 1.1 2003/05/05 21:24:16 wmaycisco Exp $ */
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
#include "basic.hpp"
#include "dataStruct.hpp"
#include "globals.hpp"
#include "states.hpp"
#include "quant.hpp"
#include "QMUtils.hpp"
#include "QM.hpp"


Int CVTCCommon::ztqQListInit()
{
  Int err, i;

  err=0;

  /* allocatate memory for prevQList */
  for (i = 0; i < mzte_codec.m_iColors; ++i)
    if ((prevQList[i] = (Int *)calloc(mzte_codec.m_iSpatialLev,
				      sizeof(Int)))==NULL)
    {
      noteError("Can't allocate memory for prevQList.");
      err = -1;
      goto ERR;
    }

  /* allocatate memory for prevQList2 */
  for (i = 0; i < mzte_codec.m_iColors; ++i)
    if ((prevQList2[i] = (Int *)calloc(mzte_codec.m_iSpatialLev,
				      sizeof(Int)))==NULL)
    {
      noteError("Can't allocate memory for prevQList.");
      err = -1;
      goto ERR;
    }

  /* allocatate memory for prevQList */
  for (i = 0; i < mzte_codec.m_iColors; ++i)
    if ((scaleLev[i] = (Int *)calloc(mzte_codec.m_iSpatialLev, 
				     sizeof(Int)))==NULL)
    {
      noteError("Can't allocate memory for scaleLev.");
      err = -1;
      goto ERR;
    }

ERR:
  return err;
}


Void CVTCCommon::ztqQListExit()
{
  Int i;

  for (i = 0; i < mzte_codec.m_iColors; ++i)
  {
    if (prevQList[i] != NULL)
    {
      free(prevQList[i]);
      prevQList[i] = NULL;
    }
    if (prevQList2[i] != NULL)
    {
      free(prevQList2[i]);
      prevQList2[i] = NULL;
    }
    if (scaleLev[i] != NULL)
    {
      free(scaleLev[i]);
      scaleLev[i] = NULL;
    }
  }
}


/* 
   Function Name
   -------------
   Int ztqInitDC()

   Arguments 
   ---------
   Int decode - 0, if called for encoding. !0, if called for decoding.

   Description
   -----------
   Initialize all information for all DC wavelet coefficients. This must be
   called once before calling the encQuantizeDC function and once before
   calling the decIQuantizeDC function.

   Functions Called
   ----------------
   initQuantSingleStage
   initInvQuantSingleStage

   Return Value
   ------------
   0, if ok. !0, if error.

   Source File
   -----------
   QMInit.c
*/

Int CVTCCommon::ztqInitDC(Int decode, Int c)
{
  Int err;
  Int x, y;
  Int dummyPrevQ;

  err = 0;

  noteDetail("Initializing DC coefficient information....");
  noteDebug("DC Dimensions: Width=%d, Height=%d", 
	    mzte_codec.m_iDCWidth, mzte_codec.m_iDCHeight);

  for (y = 0; y < mzte_codec.m_iDCHeight; ++y)
    for (x = 0; x < mzte_codec.m_iDCWidth; ++x)
    {
      if (decode)
	initInvQuantSingleStage(&COEFF_QSTATE(x, y, c), &dummyPrevQ);
      else
	initQuantSingleStage(&COEFF_QSTATE(x, y, c), &dummyPrevQ,
			     COEFF_ORGVAL(x,y,c));

      COEFF_TYPE(x, y, c)  = UNTYPED;
      COEFF_STATE(x, y, c) = S_DC;
    }

  noteDetail("Completed initializing of DC coefficient information.");

  return err;
}

/* 
   Function Name
   -------------
   Int ztqInitAC()

   Arguments 
   ---------
   Int decode - 0, if called for encoding. !0, if called for decoding.

   Description
   -----------
   Initialize all information for all AC wavelet coefficients. This must be
   called once before calling any ztq enc/AC functions and once before calling
   any ztq dec/AC functions.

   Functions Called
   ----------------
   initQuantSingleStage
   initInvQuantSingleStage
   findChild
   isIndexInRootBands

   Return Value
   ------------
   0, if ok. !0, if error.

   Source File
   -----------
   QMInit.c
*/

// hjlee 0901
Int CVTCCommon::ztqInitAC(Int decode, Int c)
{
  Int err;
  Int x, y, xc[4], yc[4];
  Int height, width;
  Int dummyPrevQ;

  err = 0;

  noteDetail("Initializing AC coefficient information for col %d....",c);

  height = mzte_codec.m_iHeight >> (int)(c!=0);
  width = mzte_codec.m_iWidth >> (int)(c!=0);

  noteDebug("Image: Width=%d, Height=%d", width, height);

  for (y = 0; y < height; ++y)
    for (x = 0; x < width; ++x)
    {
      if (x >= mzte_codec.m_iDCWidth || y >= mzte_codec.m_iDCHeight)
      {
	if (decode)
	  initInvQuantSingleStage(&COEFF_QSTATE(x, y, c), &dummyPrevQ);
	else
	{
	  initQuantSingleStage(&COEFF_QSTATE(x, y, c), &dummyPrevQ,
			       COEFF_ORGVAL(x,y,c));
	}

	COEFF_TYPE(x, y, c) = UNTYPED;

	/* AC Bands */
	if (findChild(x, y, xc, yc, c)==0 || 
	    x >= mzte_codec.m_SPlayer[c].width ||
	    y >= mzte_codec.m_SPlayer[c].height) /* leaf */
	  COEFF_STATE(x, y, c) = S_LINIT;
	else
	  COEFF_STATE(x, y,c) = S_INIT;
      }
    }

  noteDetail("Completed Initializing of AC coefficient information.");

  return err;
}


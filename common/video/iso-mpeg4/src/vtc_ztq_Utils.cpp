/* $Id: vtc_ztq_Utils.cpp,v 1.1 2005/05/09 21:29:49 wmaycisco Exp $ */
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
#define DEFINE_GLOBALS 
#include "globals.hpp"
#include "Utils.hpp"

Void CVTCCommon::setSpatialLevelAndDimensions(Int spLayer, Int c)
{
  mzte_codec.m_iCurSpatialLev=spLayer;
  mzte_codec.m_SPlayer[c].height
    =mzte_codec.m_spaLayerHeight[spLayer][c];
  mzte_codec.m_SPlayer[c].width
    =mzte_codec.m_spaLayerWidth[spLayer][c];

}

Void CVTCCommon::updateResidMaxAndAssignSkips(Int c)
{
  Int rLev;
  Int i;
  Int x, y, spX, spY, spHeight, spWidth;

  // hjlee 0901

  WVTDECOMP_RES_NUMBITPLANES(c)=0;
  for (i = 0; i <= mzte_codec.m_iCurSpatialLev; ++i)
  {
    prevQList2[c][i] = prevQList[c][i];

    if (prevQList[c][i] == 0)
      prevQList[c][i] = USER_Q(c);

    rLev=quantRefLev(USER_Q(c), prevQList[c]+i, scaleLev[c][i]++)-1;

    {
      /* extern Int ceilLog2(Int x); */ /* defined in encQM.c */
      Int v;

      v = ceilLog2(rLev+1);
      if (v > WVTDECOMP_RES_NUMBITPLANES(c))
	WVTDECOMP_RES_NUMBITPLANES(c) = v;
    }

    /*--- Set skip mode all coefficients in spatial level ---*/
    if (i < mzte_codec.m_iCurSpatialLev)
    {
      /* Get spatial level dimensions of i^{th} spatial layer */

	  // hjlee 0901
      spHeight = mzte_codec.m_spaLayerHeight[i][c];
      spWidth  = mzte_codec.m_spaLayerWidth[i][c];

      if (i==0)
      {
        spY = mzte_codec.m_iDCHeight;
        spX = mzte_codec.m_iDCWidth;
      }
      else
      {
		spY = mzte_codec.m_spaLayerHeight[i-1][c];
		spX = mzte_codec.m_spaLayerWidth[i-1][c];
      }

      /* HL */
      for (y = 0; y < spY; ++y)
		for (x = spX; x < spWidth; ++x)
		{
		  if (rLev > 0)
			COEFF_SKIP(x,y,c) = 0;
		  else if (COEFF_SKIP(x,y,c)==0)	    
			COEFF_SKIP(x,y,c)=1;
		}
      
      /* LH */
      for (y = spY; y < spHeight; ++y)
		for (x = 0; x < spX; ++x)
		{
		  if (rLev > 0)
			COEFF_SKIP(x,y,c) = 0;
		  else if (COEFF_SKIP(x,y,c)==0)	    
			COEFF_SKIP(x,y,c)=1;
		}
      
      /* HH */
      for (y = spY; y < spHeight; ++y)
		for (x = spX; x < spWidth; ++x)
		{
		  if (rLev > 0)
			COEFF_SKIP(x,y,c) = 0;
		  else if (COEFF_SKIP(x,y,c)==0)	    
			COEFF_SKIP(x,y,c)=1;
		}



	
	} 
  }
}


// hjlee 0901 
Int CVTCCommon::xy2wvtDecompLev(Int x, Int y)
{
  register Int i;

  for (i=0; i<mzte_codec.m_iWvtDecmpLev; ++i)
    if (x < (mzte_codec.m_iDCWidth<<i) && y < (mzte_codec.m_iDCHeight<<i))
      break;

  return i-1;
}




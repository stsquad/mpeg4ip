/* $Id: idwt.cpp,v 1.2 2001/05/09 21:14:18 cahighlander Exp $ */
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

/* Inverse DWT for MPEG-4 Still Texture Coding 
Original Coded by Shipeng Li, Sarnoff Corporation, Jan. 1998*/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "basic.hpp"
#include "dwt.h"

/* Function:    iDWT()
   Description: Inverse DWT for MPEG-4 Still Texture Coding 
   Input:

   InCoeff -- Input Wavelet Coefficients at CurLevel decomposition
   InMask -- Input  Mask for InCoeff, if ==1, data inside object, 
              otherwise outside.
   Width  -- Width of Original Image (must be multiples of 2^nLevels);
   Height -- Height of Original Image (must be multiples of 2^nLevels);
   CurLevel -- Currect Decomposition Level of the input wavelet coefficients;
   DstLevel -- Destined decomposition level that the wavelet coefficient is 
               synthesized to. DstLevel always less than or equal to CurLevel;
   OutDataType -- OutData Type: 0 - BYTE; 1 -- SHORT
   Filter     -- Filter Used.
   UpdateInput -- Update the level of decomposition to DstLevel  
                  for InCoeff and/or InMask or Not.
                  0 = No  Update; 1 = Update InCoeff; 2: Update InCoeff and InMask;
   FullSizeOut -- 0 = Output image size equals to Width/2^DstLevel x Height/2^DstLevel;
                  1 = Output Image size equals to Width x Height; 
		  ( Highpass band filled with zero after DstLevel)

   Output:
 
   OutData -- Output Image data of Width x Height or Width/2^DstLevel x Height/2^DstLevel
              size depending on FullSizeOut, data type decided by OutDataType;
   OutMask    -- Output mask corresponding to OutData

   Return: DWT_OK if successful.  
*/
Int VTCIDWT:: do_iDWT(DATA *InCoeff, UChar *InMask, Int Width, Int Height, Int CurLevel,
	 Int DstLevel, Int OutDataType, FILTER **Filter,  Void *OutData, 
	 UChar *OutMask, Int UpdateInput, Int FullSizeOut)
{
  switch(Filter[0]->DWT_Type) {
  case DWT_INT_TYPE:
    return(iDWTInt(InCoeff, InMask, Width, Height, CurLevel, DstLevel,
		   OutDataType, Filter, OutData, OutMask, UpdateInput, FullSizeOut));
  case DWT_DBL_TYPE:
    return(iDWTDbl(InCoeff, InMask, Width, Height, CurLevel, DstLevel,
		   OutDataType, Filter, OutData, OutMask, UpdateInput, FullSizeOut));
  default:
    return(DWT_FILTER_UNSUPPORTED);
  }
}


/* Function: AddDCMean 
   Description: Add the Mean of DC coefficients
   Input:
   
   Mask -- Mask for the transformed coeffs
   Width -- Width of the original image
   Height -- Height of the original image
   nLevels -- levels of DWT decomposition performed
   DCMean -- Mean of the DC components

   Input/Output:

   Coeff -- DWT coefficients after nLevels of decomposition
   
   Return: None */

Void VTCIDWT:: AddDCMean(Int *Coeff, UChar *Mask, Int Width, 
	       Int Height, Int nLevels, Int DCMean)
{
  Int width = (Width >> nLevels), height = (Height >> nLevels);
  Int k;
  Int *a;
  UChar *c;
  DCMean = (DCMean<<nLevels);
  for(k=0; k<Width*height; k+=Width) {
    for(a=Coeff+k,c=Mask+k; a < Coeff+k+width; a++,c++)  {
      if(*c == DWT_IN) *a+=DCMean;
    }
  }
}


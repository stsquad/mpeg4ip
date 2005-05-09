/* $Id: vtc_wavelet_dwt.cpp,v 1.1 2005/05/09 21:29:49 wmaycisco Exp $ */
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

/* Forward DWT for MPEG-4 Still Texture Coding 
Original Coded by Shipeng Li, Sarnoff Corporation, Jan. 1998*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <memory.h>
#include "basic.hpp"
#include "dwt.h"

/* Function:    DWT()
   Description: Forward DWT for MPEG-4 Still Texture Coding 
   Input:

   InData -- Input Image data of Width x Height size, data type decided by
             InDataType;
   InMask -- Input Object Mask for InData, if ==1, data inside object, 
             otherwise outside.
   Width  -- Width of Image (must be multiples of 2^nLevels);
   Height -- Height of Image (must be multiples of 2^nLevels);
   nLevels -- number of decomposition level,0 being original image;
   InDataType -- 0 - BYTE; 1 -- SHORT
   Filter     -- Filter Used.

   Output:

   OutCoeff   -- Ouput wavelet coefficients
   OutMask    -- Output mask corresponding to wavelet coefficients

   Return: DWT_OK if success.  
*/

Int VTCDWT:: do_DWT(Void *InData, UChar *InMask, Int Width, Int Height, Int nLevels,
	 Int InDataType, FILTER **Filter,  DATA *OutCoeff, UChar *OutMask)

{
  Int nBits, MaxCoeff, MinCoeff;
  Int level;
  double *tempCoeff;
  Int ret;
  double *a;
  Int *b;
  UChar *c;
  UShort *s;

   /* Check filter class first */
  for(level=0;level<nLevels;level++) {
    if(Filter[level]->DWT_Class != DWT_ODD_SYMMETRIC && Filter[level]->DWT_Class != DWT_EVEN_SYMMETRIC ) {
      return(DWT_FILTER_UNSUPPORTED);
    }
  }

  /* Check output coefficient buffer capacity */
  nBits = sizeof(Int)*8;
  MaxCoeff = (1<<(nBits-1))-1;
  MinCoeff = -(1<<(nBits-1));
  /* Limit nLevels as: 0 - 15*/
  if(nLevels < 0 || nLevels >=16) return(DWT_INVALID_LEVELS);
  /* Check Width and Height, must be multiples of 2^nLevels */
  if((Width &( (1<<nLevels)-1))!=0) return(DWT_INVALID_WIDTH);
  if((Height &( (1<<nLevels)-1))!=0) return(DWT_INVALID_HEIGHT);
  /* Zero Level of Decomposition */
  /* copy coefficients */
  if(InDataType==0) { /* BYTE */
    for(b=OutCoeff,c=(UChar*)InData;b<OutCoeff+Width*Height;b++,c++) {
     *b =(Int) *c;
    }
  }
  else {
    for(b=OutCoeff,s=(UShort*)InData;b<OutCoeff+Width*Height;b++,s++) {
      *b = (Int) *s;
    }
  }
  /* copy mask */
  memcpy(OutMask, InMask, sizeof(UChar)*Width*Height);
  
  /* Double precision calculation if required */
  if(nLevels>0 && Filter[0]->DWT_Type==DWT_DBL_TYPE) {
    /* allocate temp double buffer */
    tempCoeff = (double *) malloc(sizeof(double)*Width*Height);
    if(tempCoeff == NULL) return(DWT_MEMORY_FAILED);
    /* copy zero level decomposition results to temp buffer */
    for(a=tempCoeff, b=OutCoeff;a<tempCoeff+Width*Height;a++, b++) {
      *a = (double) *b;
    }
    /* Perform the double DWT */
    for(level=1;level<=nLevels;level++) {
      /* Decompose one level */
      ret=DecomposeOneLevelDbl(tempCoeff, OutMask, Width, Height, 
			       level, Filter[level-1]);
      if(ret!=DWT_OK) {
	free(tempCoeff);
	return(ret);
      }
    }
    /* copy the double coefficients back to OutCoeff */
    for(b=OutCoeff,a=tempCoeff;b<OutCoeff+Width*Height;b++,a++) {
      Int iTmp;
      iTmp = (Int)floor(*a+0.5); /* rounding */
      if(iTmp> MaxCoeff || iTmp< MinCoeff) {
	/* fprIntf(stderr,"iTmp=%d %f\n",iTmp, *a); */
	free(tempCoeff);
	return(DWT_COEFF_OVERFLOW);
      }
      else
	*b = (Int) iTmp;
    }
    free(tempCoeff);
  }
  else if(Filter[0]->DWT_Type==DWT_INT_TYPE) {
    /* Perform the Int DWT */
    for(level=1;level<=nLevels;level++) {
      ret=DecomposeOneLevelInt(OutCoeff, OutMask, Width, Height, 
			       level, Filter[level-1], MaxCoeff, MinCoeff);
      if(ret!=DWT_OK) return(ret);
    }
  }
  
  return(DWT_OK);
}
/* Function: RemoveDCMean 
   Description: Remove the Mean of DC coefficients
   Input:
   
   Mask -- Mask for the transformed coeffs
   Width -- Width of the original image
   Height -- Height of the original image
   nLevels -- levels of DWT decomposition performed

   Input/Output:

   Coeff -- DWT coefficients after nLevels of decomposition
   
   Return: Mean of the DC coefficients */

Int VTCDWT:: RemoveDCMean(Int *Coeff, UChar *Mask, Int Width, Int Height, Int nLevels)
{
  Int DCMean=0;
  Int width = (Width >> nLevels), height = (Height >> nLevels);
  Int k;
  Int *a;
  UChar *c;
  Int Count=0;
  for(k=0; k<Width*height; k+=Width) {
    for(a=Coeff+k,c=Mask+k; a < Coeff+k+width; a++,c++) {
      if(*c == DWT_IN) {
		Count++;
		DCMean += (Int) *a;
	/* prIntf("%d\n", *a); */
      }
    }
  }
  if(Count!=0) DCMean = (Int)((Float)DCMean/((Float)(Count<<nLevels))+0.5);
  else DCMean = 0;

  DCMean = DCMean<<nLevels;

  for(k=0; k<Width*height; k+=Width) {
    for(a=Coeff+k,c=Mask+k; a < Coeff+k+width; a++,c++)  {
      if(*c == DWT_IN) *a-=DCMean;
    }
  }
  return(DCMean>>nLevels);
}



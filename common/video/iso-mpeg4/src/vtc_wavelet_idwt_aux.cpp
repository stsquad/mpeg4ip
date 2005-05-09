/* $Id: vtc_wavelet_idwt_aux.cpp,v 1.1 2005/05/09 21:29:49 wmaycisco Exp $ */
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

#ifndef _DWT_DBL_
#define _DWT_INT_
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <memory.h>
#include "basic.hpp"
#include "dwt.h"
#endif

#undef DWTDATA1
#undef DWTDATA

#ifdef _DWT_INT_
#define DWTDATA1 Int
#define DWTDATA Int
#else
#define DWTDATA1 double
#define DWTDATA double
#endif

/* Function:    iDWTInt() or iDWTDbl()
   Description: Integer or Double Inverse DWT for MPEG-4 Still Texture Coding 
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
#ifdef _DWT_INT_  // hjlee 0901
Int VTCIDWT:: iDWTInt(DATA *InCoeff, UChar *InMask, Int Width, Int Height, Int CurLevel,
	 Int DstLevel, Int OutDataType, FILTER **Filter,  Void *OutData, 
	 UChar *OutMask, Int UpdateInput, Int FullSizeOut)
#else
Int VTCIDWT:: iDWTDbl(DATA *InCoeff, UChar *InMask, Int Width, Int Height, Int CurLevel,
	 Int DstLevel, Int OutDataType, FILTER **Filter,  Void *OutData, 
	 UChar *OutMask, Int UpdateInput, Int FullSizeOut)
#endif
{
  Int nBits, MaxCoeff, MinCoeff;
  Int i,level, k;
  DWTDATA1 *tempCoeff; /* Int for Int, double for Dbl */
  Int ret;
  DWTDATA1 *a;
  Int *b;
  UChar *c;
  UShort *s;
  UChar *d,*e, *tempMask;
  /* Check filter class first */
  for(level=CurLevel;level>DstLevel; level--) {
    if(Filter[level-1]->DWT_Class != DWT_ODD_SYMMETRIC && Filter[level-1]->DWT_Class != DWT_EVEN_SYMMETRIC ) {
      return(DWT_FILTER_UNSUPPORTED);
    }
  /* check filter Type */
#ifdef _DWT_INT_
  if(Filter[level-1]->DWT_Type!=DWT_INT_TYPE) return(DWT_INTERNAL_ERROR);
#else
  if(Filter[level-1]->DWT_Type!=DWT_DBL_TYPE) return(DWT_INTERNAL_ERROR);
#endif
  }


  /* Check output coefficient buffer capacity */
  nBits = sizeof(Int)*8;
  MaxCoeff = (1<<(nBits-1))-1;
  MinCoeff = -(1<<(nBits-1));
  /* Limit nLevels as: 0 - 15*/
  if(DstLevel < 0 || CurLevel >=16 || DstLevel >=16 || DstLevel > CurLevel) 
    return(DWT_INVALID_LEVELS);
  /* Check Width and Height, must be multiples of 2^CurLevel */
  if((Width &( (1<<CurLevel)-1))!=0) return(DWT_INVALID_WIDTH);
  if((Height &( (1<<CurLevel)-1))!=0) return(DWT_INVALID_HEIGHT);
  /* copy mask */
  tempMask = (UChar *)malloc(sizeof(UChar)*Width*Height);
  if(tempMask==NULL) return(DWT_MEMORY_FAILED);
  memcpy(tempMask, InMask, sizeof(UChar)*Width*Height);
  
  /* allocate temp  buffer */
  tempCoeff = (DWTDATA1 *) malloc(sizeof(DWTDATA1)*Width*Height);
  if(tempCoeff == NULL) {
    free(tempMask);
    return(DWT_MEMORY_FAILED);
  }
  memset(tempCoeff, (UChar)0, Width*Height*sizeof(DWTDATA1));
  /* copy CurLevel decomposition results to temp buffer */
  for(k=0;k<Width*(Height>>(DstLevel));k+=Width) {
    for(a=tempCoeff+k, b=InCoeff+k;a<tempCoeff+k+(Width>>(DstLevel));a++, b++) {
      *a = (DWTDATA1) *b;
    }
  }
  /* Perform the  iDWT */
  for(level=CurLevel;level>DstLevel;level--) {
    /* Synthesize one level */
#ifdef _DWT_INT_
    ret=SynthesizeOneLevelInt(tempCoeff, tempMask, Width, Height, 
			      level, Filter[level-1], MaxCoeff, MinCoeff, DWT_NONZERO_HIGH);
#else
    ret=SynthesizeOneLevelDbl(tempCoeff, tempMask, Width, Height, 
			      level, Filter[level-1], DWT_NONZERO_HIGH);
#endif      
    if(ret!=DWT_OK) {
      free(tempCoeff);
      free(tempMask);
      return(ret);
    }
  }
  /* check to see if required to update InCoeff and/or InMask */
  if(UpdateInput>0) {
    /* update InCoeff */
    for(k=0;k<Width*(Height>>DstLevel); k+=Width) {
      for(b=InCoeff+k,a=tempCoeff+k;b<InCoeff+k+(Width>>DstLevel);b++,a++) {
#ifdef _DWT_INT_
	*b = (Int) *a;
#else
	Int iTmp;
	iTmp = (Int)floor(*a+0.5); /* rounding */

	if(iTmp> MaxCoeff || iTmp< MinCoeff) {
	  free(tempCoeff);
	  free(tempMask);
	  return(DWT_COEFF_OVERFLOW);
	}
	else
	  *b = (Int) iTmp;
#endif
      }
    }
  }
  if(UpdateInput>1) {
    /* Update InMask */
    for(k=0;k<Width*(Height>>DstLevel); k+=Width) {
      for(d=InMask+k,e=tempMask+k;d<InMask+k+(Width>>DstLevel);d++,e++) {
	*d = *e;
      }
    }
  }
  
  if(FullSizeOut) {
    /* Perform the rest of iDWT till fullsize */
    for(level=DstLevel;level>0;level--) {
      /* Synthesize one level */
#ifdef _DWT_INT_
      ret=SynthesizeOneLevelInt(tempCoeff, tempMask, Width, Height, 
				level, Filter[level-1], MaxCoeff, MinCoeff, DWT_ZERO_HIGH);
#else
      ret=SynthesizeOneLevelDbl(tempCoeff, tempMask, Width, Height, 
				level, Filter[level-1], DWT_ZERO_HIGH);
#endif
      if(ret!=DWT_OK) {
	free(tempCoeff);
	free(tempMask);
	return(ret);
      }
    } 
  }
  
  /* copy the output to OutData and OutMask */
  if(FullSizeOut) level=0;
  else level=DstLevel;
  
  for(i=0,k=0;k<Width*(Height >> level);k+=Width,i+=(Width>>level)) {
    
    if(OutDataType==0) { /* Unsigned CHAR */
      c = (UChar *)OutData; c+=i;
      for(a=tempCoeff+k;a<tempCoeff+k+(Width>>level);c++,a++) {
	Int iTmp;
#ifdef _DWT_INT_
	iTmp = (Int) *a;
	iTmp = level>0?((iTmp+(1<<(level-1))) >> level):iTmp; /* scale and round */
#else
	iTmp = (Int)floor(*a/(1<<level)+0.5); /*scale down by 2^level and  rounding */
#endif
	iTmp = (iTmp >0) ? ((iTmp > 255)?255:iTmp): 0; /* clipping */
	*c = (UChar) iTmp;
      }
    }
    else { /* UShort */
      s = (UShort *)OutData; s+=i;
      for(a=tempCoeff+k;a<tempCoeff+k+(Width>>level);s++,a++) {
	Int iTmp;
#ifdef _DWT_INT_
	iTmp = (Int)*a;
	iTmp = level>0?((iTmp+(1<<(level-1))) >> level):iTmp; /* scale and round */
#else
	iTmp = (Int)floor(*a/(1<<level)+0.5); /*scale down by 2^level and  rounding */
#endif
	iTmp = (iTmp >0) ? ((iTmp > 65535)?65535:iTmp): 0; /* clipping */
	*s = (UShort) iTmp;
      }
    }
    d = OutMask + i;
    for(e=tempMask+k;e<tempMask+k+(Width>>level);e++,d++)  *d=*e;
  }
  free(tempCoeff);
  free(tempMask);
  return(DWT_OK);
}

#ifdef _DWT_INT_
/* Function:   SynthesizeOneLevelInt() 
   Description: Integer Synthesize one level of wavelet coefficients
   Input:
   Width  -- Width of Image; 
   Height -- Height of Image;
   level -- current  level
   Filter     -- Filter Used.
   MaxCoeff, MinCoeff -- bounds of the output data;
   ZeroHigh -- 1 Highpass bands are all zeros;

   Input/Output:

   OutCoeff   -- Ouput wavelet coefficients
   OutMask    -- Output mask corresponding to wavelet coefficients

   Return: DWT_OK if successful.
*/
Int VTCIDWT::SynthesizeOneLevelInt(Int *OutCoeff, UChar *OutMask, Int Width,
				Int Height, Int level, FILTER *Filter,
				Int MaxCoeff, Int MinCoeff, Int ZeroHigh)
{
  Int *InBuf, *OutBuf ;
  UChar *InMaskBuf, *OutMaskBuf;
  Int width = Width>>(level-1);
  Int height = Height>>(level-1);
  Int MaxLength = (width > height)?width:height;
  Int i,k,ret;
  Int *a;
  UChar *c,*d;
  Int *e;

  /* double check filter type */
  if(Filter->DWT_Type != DWT_INT_TYPE) return(DWT_INTERNAL_ERROR);
  /* allocate line buffers */
  InBuf = (Int *) malloc(sizeof(Int)*MaxLength);
  InMaskBuf = (UChar *) malloc(sizeof(UChar)*MaxLength);
  OutBuf = (Int *) malloc(sizeof(Int)*MaxLength);
  OutMaskBuf = (UChar *) malloc(sizeof(UChar)*MaxLength);
  if(InBuf==NULL || InMaskBuf ==NULL || OutBuf == NULL || OutMaskBuf==NULL) 
    return(DWT_MEMORY_FAILED);
  /* vertical synthesis first */
  for(i=0;i<width;i++) {
    /* get a column of coefficients and mask*/
    for(a=InBuf, e=OutCoeff+i, c= InMaskBuf, d= OutMask+i;
	a<InBuf+height; a++, c++, e+=Width, d+=Width) {
      *a=(Int) *e;
      *c = *d;
    }
    /* perform inverse vertical SA-DWT */
    ret=iSADWT1dInt(InBuf, InMaskBuf, OutBuf, OutMaskBuf, height, Filter, 
		    DWT_VERTICAL,  ((i>=(width>>1) && ZeroHigh == DWT_ZERO_HIGH)?DWT_ALL_ZERO:ZeroHigh));
    if(ret!=DWT_OK) {
      free(InBuf);free(OutBuf);free(InMaskBuf);free(OutMaskBuf);
      return(ret);
    }
    /* put the transformed column back */
    for(a=OutBuf, e=OutCoeff+i, c= OutMaskBuf, d= OutMask+i;
	a<OutBuf+height; a++, c++, e+=Width, d+=Width) {
      /* Scale and Leave 3 extra bits for precision reason */
      *a = ROUNDDIV(*a <<3, Filter->Scale); 
      if(*a > MaxCoeff || *a < MinCoeff) {
	free(InBuf);free(OutBuf);free(InMaskBuf);free(OutMaskBuf);
	return(DWT_COEFF_OVERFLOW);
      }
      *e= (Int) *a;
      *d = *c;
    }
  }    
  /* then horizontal synthesis */
  for(i=0,k=0;i<height;i++,k+=Width) {
    /* get a line of coefficients */
    for(a=InBuf, e=OutCoeff+k;a<InBuf+width;a++,e++) {
      *a = (Int) *e;
    }
    /* get a line of mask */
    memcpy(InMaskBuf, OutMask+k, sizeof(UChar)*width);
    /* Perform horizontal inverse SA-DWT */
    ret=iSADWT1dInt(InBuf, InMaskBuf, OutBuf, OutMaskBuf, width, Filter, 
		   DWT_HORIZONTAL, ZeroHigh);
    if(ret!=DWT_OK) {
      free(InBuf);free(OutBuf);free(InMaskBuf);free(OutMaskBuf);
      return(ret);
    }
    /* put the transformed line back */
    for(a=OutBuf,e=OutCoeff+k;a<OutBuf+width;a++,e++) {
      /*Scale and leave the sqrt(2)*sqrt(2) factor in the scale */
      *a = ROUNDDIV(*a , Filter->Scale<<2);
      if(*a > MaxCoeff || *a < MinCoeff) {
	free(InBuf);free(OutBuf);free(InMaskBuf);free(OutMaskBuf);
	return(DWT_COEFF_OVERFLOW);
      }
      *e = (Int) *a;
    }
    memcpy(OutMask+k, OutMaskBuf, sizeof(UChar)*width);
  }


  free(InBuf);free(OutBuf);free(InMaskBuf);free(OutMaskBuf);
  return(DWT_OK);
}
#else
/* Function:   SynthesizeOneLevelDbl()
   Description: Synthesize one level of wavelet coefficients
   Input:
   Width  -- Width of Image; 
   Height -- Height of Image;
   level -- current  level
   Filter     -- Filter Used.
   MaxCoeff, MinCoeff -- bounds of the output data;
   ZeroHigh -- Highpass bands are all zeros 
   Input/Output:

   OutCoeff   -- Ouput wavelet coefficients
   OutMask    -- Output mask corresponding to wavelet coefficients

   Return: DWT_OK if successful.
*/
 Int VTCIDWT:: SynthesizeOneLevelDbl(double *OutCoeff, UChar *OutMask, Int Width,
				Int Height, Int level, FILTER *Filter, Int ZeroHigh)
{
  double *InBuf, *OutBuf ;
  UChar *InMaskBuf, *OutMaskBuf;
  Int width = Width>>(level-1);
  Int height = Height>>(level-1);
  Int MaxLength = (width > height)?width:height;
  Int i,k,ret;
  double *a;
  UChar *c,*d;
  double *e;

  /* double check filter type */
  if(Filter->DWT_Type != DWT_DBL_TYPE) return(DWT_INTERNAL_ERROR);
  /* allocate line buffers */
  InBuf = (double *) malloc(sizeof(double)*MaxLength);
  InMaskBuf = (UChar *) malloc(sizeof(UChar)*MaxLength);
  OutBuf = (double *) malloc(sizeof(double)*MaxLength);
  OutMaskBuf = (UChar *) malloc(sizeof(UChar)*MaxLength);
  if(InBuf==NULL || InMaskBuf ==NULL || OutBuf == NULL || OutMaskBuf==NULL) 
    return(DWT_MEMORY_FAILED);
    

  /* vertical synthesis first */
  for(i=0;i<width;i++) {
    /* get a column of coefficients and mask */
    for(a=InBuf, e=OutCoeff+i, c= InMaskBuf, d= OutMask+i;
	a<InBuf+height; a++, c++, e+=Width, d+=Width) {
      *a = *e;
      *c = *d;
    }
    /* perform vertical inverse SA-DWT */
    ret=iSADWT1dDbl(InBuf, InMaskBuf, OutBuf, OutMaskBuf, height, Filter, 
		    DWT_VERTICAL, ((i>=(width>>1) && ZeroHigh==DWT_ZERO_HIGH)?DWT_ALL_ZERO:ZeroHigh));
    if(ret!=DWT_OK) {
      free(InBuf);free(OutBuf);free(InMaskBuf);free(OutMaskBuf);
      return(ret);
    }
    /* put the transformed column back */
    for(a=OutBuf, e=OutCoeff+i, c= OutMaskBuf, d= OutMask+i;
	a<OutBuf+height; a++, c++, e+=Width, d+=Width) {
      *e = *a;
      *d = *c;
    }
  }

  /* horizontal decomposition first*/
  for(i=0,k=0;i<height;i++,k+=Width) {
    /* get a line of coefficients */
    for(a=InBuf, e=OutCoeff+k;a<InBuf+width;a++,e++) {
      *a =  *e;
    }
    /* get a line of mask */
    memcpy(InMaskBuf, OutMask+k, sizeof(UChar)*width);
    /* Perform horizontal inverse SA-DWT */
    ret=iSADWT1dDbl(InBuf, InMaskBuf, OutBuf, OutMaskBuf, width, Filter, 
		    DWT_HORIZONTAL, ZeroHigh);
    if(ret!=DWT_OK) {
      free(InBuf);free(OutBuf);free(InMaskBuf);free(OutMaskBuf);
      return(ret);
    }
    /* put the transformed line back */
    for(a=OutBuf,e=OutCoeff+k;a<OutBuf+width;a++,e++) {
      *e =  *a;
    }
    memcpy(OutMask+k, OutMaskBuf, sizeof(UChar)*width);
  }

  free(InBuf);free(OutBuf);free(InMaskBuf);free(OutMaskBuf);
  return(DWT_OK);
}
#endif

/* Function: iSADWT1dInt() or iSADWT1dDbl()
   Description:  inverse 1-d SA-DWT
   Input:

   InBuf -- Input 1d data buffer
   InMaskBuf -- Input 1d mask buffer
   Length -- length of the input data
   Filter -- filter used
   Direction -- vertical or horizontal synthesis (used for inversible 
   mask synthsis)
   ZeroHigh -- DWT_ZERO_HIGH indicates the Highpass bands are all zeros
               DWT_ALL_ZERO indicates all the coefficients are zeros
	       DWT_NONZERO_HIGH indicates not all Highpass bands are zeros
   Output:
   
   OutBuf -- Synthesized 1d Data
   OutMask -- Mask for the synthesized 1d Data

   Return: return DWT_OK if successful
  
*/
#ifdef _DWT_INT_
 Int VTCIDWT:: iSADWT1dInt(Int *InBuf, UChar *InMaskBuf, Int *OutBuf, 
	       UChar *OutMaskBuf, Int Length, FILTER *Filter, 
		Int Direction, Int ZeroHigh)
{

  switch(Filter->DWT_Class){
  case DWT_ODD_SYMMETRIC:
    return(iSADWT1dOddSymInt(InBuf, InMaskBuf, OutBuf, OutMaskBuf, 
			    Length, Filter, Direction, ZeroHigh));
  case DWT_EVEN_SYMMETRIC:
    return(iSADWT1dEvenSymInt(InBuf, InMaskBuf, OutBuf, OutMaskBuf, 
			     Length, Filter, Direction, ZeroHigh));
    /*  case ORTHOGONAL:
	return(iSADWT1dOrthInt(InBuf, InMaskBuf, OutBuf, OutMaskBuf, 
			  Length, Filter, Direction, ZeroHigh)); */
  default:
    return(DWT_FILTER_UNSUPPORTED);
  }
}
#else
 Int VTCIDWT:: iSADWT1dDbl(double *InBuf, UChar *InMaskBuf, double *OutBuf, 
		UChar *OutMaskBuf, Int Length, FILTER *Filter, 
		Int Direction, Int ZeroHigh)
{

  switch(Filter->DWT_Class){
  case DWT_ODD_SYMMETRIC:
    return(iSADWT1dOddSymDbl(InBuf, InMaskBuf, OutBuf, OutMaskBuf, 
			    Length, Filter, Direction, ZeroHigh));
  case DWT_EVEN_SYMMETRIC:
    return(iSADWT1dEvenSymDbl(InBuf, InMaskBuf, OutBuf, OutMaskBuf, 
			     Length, Filter, Direction, ZeroHigh));
    /*  case ORTHOGONAL:
	return(iSADWT1dOrthDbl(InBuf, InMaskBuf, OutBuf, OutMaskBuf, 
			  Length, Filter, Direction, ZeroHigh)); */
  default:
    return(DWT_FILTER_UNSUPPORTED);
  }
}
#endif


/* Function: iSADWT1dOddSymInt() or iSADWT1dOddSymDbl()
   Description: 1D  iSA-DWT using Odd Symmetric Filter
   Input:

   InBuf -- Input 1d data buffer
   InMaskBuf -- Input 1d mask buffer
   Length -- length of the input data
   Filter -- filter used
   Direction -- vertical or horizontal synthesis (used for inversible 
   mask synthesis)
   ZeroHigh -- DWT_ZERO_HIGH indicates the Highpass bands are all zeros
               ALL_ZERO indicates all the coefficients are zeros
	       DWT_NONZERO_HIGH indicates not all Highpass bands are zeros

   Output:
   
   OutBuf -- Synthesized 1d Data
   OutMask -- Synthesized 1d Mask

   Return: return DWT_OK if successful

*/



#ifdef _DWT_INT_
 Int VTCIDWT:: iSADWT1dOddSymInt(DWTDATA *InBuf, UChar *InMaskBuf, DWTDATA *OutBuf, 
			    UChar *OutMaskBuf, Int Length, FILTER *Filter, 
			    Int Direction, Int ZeroHigh)
#else
 Int VTCIDWT:: iSADWT1dOddSymDbl(DWTDATA *InBuf, UChar *InMaskBuf, DWTDATA *OutBuf, 
			    UChar *OutMaskBuf, Int Length, FILTER *Filter, 
			    Int Direction, Int ZeroHigh)
#endif

{
  Int i;
  Int SegLength = 0;
  Int odd;
  Int start, end;
  UChar *a, *b, *c;
  Int ret;
  /* static Int count=0; */
  /* double check filter class and type */
  if(Filter->DWT_Class != DWT_ODD_SYMMETRIC) return(DWT_INTERNAL_ERROR);
#ifdef _DWT_INT_
  if(Filter->DWT_Type != DWT_INT_TYPE) return(DWT_INTERNAL_ERROR);
#else
  if(Filter->DWT_Type != DWT_DBL_TYPE) return(DWT_INTERNAL_ERROR);
#endif
  /* double check if Length is even */
  if(Length & 1) return(DWT_INTERNAL_ERROR);
  /* synthesize mask first */
  for(a=OutMaskBuf, b = InMaskBuf, c= InMaskBuf+(Length>>1); a<OutMaskBuf+Length;b++,c++) {
    if(Direction == DWT_VERTICAL) {
      if(*c == DWT_OUT2) { 
	*a++=DWT_OUT0;
	*a++=DWT_IN;
      }
      else if(*c == DWT_OUT3) {
	*a++=DWT_OUT1;
	*a++=DWT_IN;
      }
      else {
	*a++=*b;
	*a++=*c;
      }
    }
    else {
      if(*c == DWT_OUT1) { 
	*a++=DWT_OUT0;
	*a++=DWT_IN;
      }
      else {
	*a++ = *b;
	*a++ = *c;
      }
    }
  }

  /* initial OutBuf to zeros */
  memset(OutBuf, (UChar)0, sizeof(DWTDATA)*Length);
  if(ZeroHigh == DWT_ALL_ZERO) return(DWT_OK);
  
  i = 0;
  a = OutMaskBuf;
  while(i<Length) {
    /* search for a segment */
    while(i<Length && (a[i])!=DWT_IN) i++;
    start = i;
    if(i >= Length) break;
    while(i<Length && (a[i])==DWT_IN) i++;
    end = i;
    SegLength = end-start;
    odd = start%2;
    if(SegLength==1) { /* special case for single poInt */
#ifdef _DWT_INT_
      ret=SynthesizeSegmentOddSymInt(OutBuf+start, InBuf+(start>>1), 
				     InBuf+(Length>>1)+(start>>1), odd, 
				     SegLength, Filter, ZeroHigh);
#else
      ret=SynthesizeSegmentOddSymDbl(OutBuf+start, InBuf+(start>>1), 
				     InBuf+(Length>>1)+(start>>1), odd, 
				     SegLength, Filter,ZeroHigh);
#endif
      if(ret!=DWT_OK) return(ret);
    }
    else {
#ifdef _DWT_INT_
      ret=SynthesizeSegmentOddSymInt(OutBuf+start, InBuf+((start+1)>>1), 
				     InBuf+(Length>>1)+(start>>1), odd, 
				     SegLength, Filter,ZeroHigh);
      
#else
      ret=SynthesizeSegmentOddSymDbl(OutBuf+start, InBuf+((start+1)>>1), 
				     InBuf+(Length>>1)+(start>>1), odd, 
				     SegLength, Filter,ZeroHigh);
#endif
      if(ret!=DWT_OK) return(ret);
    }
  }
  return(DWT_OK);
}

/* 
   Function:  SynthesizeSegmentOddSymInt() or  SynthesizeSegmentOddSymDbl()
   Description: SA-Synthesize a 1D segment based on its InitPos and Length using
                Odd-Symmetric Filters
   Input:

   InL -- Input lowpass data;
   InH -- Input highpass data;
   PosFlag -- Start position of this segment (ODD or EVEN);
   Length -- Length of this Segment;
   Filter -- Filter used;
   ZeroHigh -- Highpass band is zero or not.

   Output:

   Out -- Output synthesized data segment;

   Return: Return DWT_OK if Successful
*/
#ifdef _DWT_INT_
 Int VTCIDWT::  SynthesizeSegmentOddSymInt(DWTDATA *Out, DWTDATA *InL, DWTDATA *InH, 
				     Int PosFlag, Int Length, FILTER *Filter, Int ZeroHigh)
#else
 Int VTCIDWT::  SynthesizeSegmentOddSymDbl(DWTDATA *Out, DWTDATA *InL, DWTDATA *InH, 
				     Int PosFlag, Int Length, FILTER *Filter, Int ZeroHigh)
#endif

{


#ifdef _DWT_INT_
  Short *LPCoeff = (Short *)Filter->LPCoeff, *HPCoeff = (Short *)Filter->HPCoeff; 
  Short *fi;
#else
  double *LPCoeff =(double *) Filter->LPCoeff, *HPCoeff =(double *) Filter->HPCoeff; 
  double *fi; 
#endif
  Int ltaps = Filter->LPLength, htaps = Filter->HPLength; /* filter lengths*/
  Int loffset = ltaps/2, hoffset = htaps/2;  /* Filter offset */
  Int border = (ltaps>htaps)?ltaps:htaps; /*the larger of ltaps and htaps */
  Int m, n;
  DWTDATA *f,*pixel, *pixel1, val, *buf, *a, *tmp_out;
  Int r = Length-1;
  if(Length == 1) {
    PosFlag = 0;
	ZeroHigh = DWT_ZERO_HIGH; // hjlee 0928
  }
  /* allocate proper buffer */
  buf = (DWTDATA *) malloc((Length+2*border)*sizeof(DWTDATA));
  if(buf==NULL)  return(DWT_MEMORY_FAILED);
  
  for(m=0;m<Length;m++) Out[m]= (DWTDATA) 0;
  for(m=0;m<Length+2*border;m++) buf[m]= (DWTDATA) 0;
  /*  fill in the low pass coefficients by upsampling by 2*/
  a = buf+border;
  for(m=PosFlag; m< Length; m+=2) {
    a[m] = InL[m>>1];
  }
  /* symmetric extension */
  for (m=1 ; m<=border; m++) {
    a[-m] =   a[m];  /* to allow Shorter seg */
    a[r+m] =  a[r-m]; 
  }

  a = buf + border;
  f = buf + border + Length;
  tmp_out = Out;
  for (; a<f; a ++) {
    /* filter the pixel with lowpass filter */
    for( fi=LPCoeff, pixel=a-loffset, pixel1=pixel+ltaps-1, val=0, n=0; 
	 n<(ltaps >>1); 
	 n++, fi++, pixel++, pixel1--) {
      val += ( *fi * (*pixel + *pixel1));
    }
    val += ( *fi * *pixel);
    *tmp_out++ = val;
  }

  /* if Highpass are zeros, skip */
  if(ZeroHigh== DWT_NONZERO_HIGH) {
    for(m=0;m<Length+2*border;m++) buf[m]= (DWTDATA) 0;
    a = buf+border;
    for(m=1-PosFlag;m< Length; m+=2) {
      a[m] = InH[m>>1];
    }
    /* symmetric extension */
    for (m=1 ; m<=border; m++) {
      a[-m] =   a[m];  /* to allow Shorter seg */
      a[r+m] =  a[r-m]; 
    } 
    a = buf + border;
    tmp_out = Out;
    for (; a<f; a ++){
      /* filter the pixel with highpass filter */
      for(fi=HPCoeff, pixel=a-hoffset, pixel1=pixel+htaps-1, val=0, n=0; 
	  n<(htaps>>1); n++, fi++, pixel++, pixel1--) {
	val += ( *fi *  (*pixel + *pixel1));
      }
      val += ( *fi *  *pixel);
      *tmp_out++ += val;
    }
  }
  free(buf);
  return(DWT_OK);
}


#ifdef _DWT_INT_
 Int VTCIDWT:: iSADWT1dEvenSymInt(DWTDATA *InBuf, UChar *InMaskBuf, DWTDATA *OutBuf, 
			    UChar *OutMaskBuf, Int Length, FILTER *Filter, 
			    Int Direction, Int ZeroHigh)
#else
 Int VTCIDWT:: iSADWT1dEvenSymDbl(DWTDATA *InBuf, UChar *InMaskBuf, DWTDATA *OutBuf, 
			    UChar *OutMaskBuf, Int Length, FILTER *Filter, 
			    Int Direction, Int ZeroHigh)
#endif

{
  Int i;
  Int SegLength = 0;
  Int odd;
  Int start, end;
  UChar *a, *b, *c;
  Int ret;
  /* double check filter class and type */
  if(Filter->DWT_Class != DWT_EVEN_SYMMETRIC) return(DWT_INTERNAL_ERROR);
#ifdef _DWT_INT_
  if(Filter->DWT_Type != DWT_INT_TYPE) return(DWT_INTERNAL_ERROR);
#else
  if(Filter->DWT_Type != DWT_DBL_TYPE) return(DWT_INTERNAL_ERROR);
#endif
  /* double check if Length is even */
  if(Length & 1) return(DWT_INTERNAL_ERROR);
  /* synthesize mask first */
  for(a=OutMaskBuf, b = InMaskBuf, c= InMaskBuf+(Length>>1); a<OutMaskBuf+Length;b++,c++) {
    if(Direction == DWT_VERTICAL) {
      if(*c == DWT_OUT2) { 
	*a++=DWT_OUT0;
	*a++=DWT_IN;
      }
      else if(*c == DWT_OUT3) {
	*a++=DWT_OUT1;
	*a++=DWT_IN;
      }
      else {
	*a++=*b;
	*a++=*c;
      }
    }
    else {
      if(*c == DWT_OUT1) { 
	*a++=DWT_OUT0;
	*a++=DWT_IN;
      }
      else {
	*a++ = *b;
	*a++ = *c;
      }
    }
  }
  /* initial OutBuf to zeros */
  memset(OutBuf, (UChar)0, sizeof(DWTDATA)*Length);
  if(ZeroHigh == DWT_ALL_ZERO) return(DWT_OK);
  
  i = 0;
  a = OutMaskBuf;
  while(i<Length) {
    /* search for a segment */
    while(i<Length && (a[i])!=DWT_IN) i++;
    start = i;
    if(i >= Length) break;
    while(i<Length && (a[i])==DWT_IN) i++;
    end = i;
    SegLength = end-start;
    odd = start%2;
    if(SegLength==1) { /* special case for single poInt */
#ifdef _DWT_INT_
      ret=SynthesizeSegmentEvenSymInt(OutBuf+start, InBuf+(start>>1), 
				     InBuf+(Length>>1)+(start>>1), odd, 
				     SegLength, Filter, ZeroHigh);
#else
      ret=SynthesizeSegmentEvenSymDbl(OutBuf+start, InBuf+(start>>1), 
				     InBuf+(Length>>1)+(start>>1), odd, 
				     SegLength, Filter,ZeroHigh);
#endif
      if(ret!=DWT_OK) return(ret);
    }
    else {
#ifdef _DWT_INT_
      ret=SynthesizeSegmentEvenSymInt(OutBuf+start, InBuf+(start>>1), 
				     InBuf+(Length>>1)+((start+1)>>1), odd, 
				     SegLength, Filter,ZeroHigh);
      
#else
      ret=SynthesizeSegmentEvenSymDbl(OutBuf+start, InBuf+(start>>1), 
				     InBuf+(Length>>1)+((start+1)>>1), odd, 
				     SegLength, Filter,ZeroHigh);
#endif
      if(ret!=DWT_OK) return(ret);
    }
  }
  return(DWT_OK);
}

/* 
   Function:  SynthesizeSegmentEvenSymInt() or  SynthesizeSegmentEvenSymDbl()
   Description: SA-Synthesize a 1D segment based on its InitPos and Length using
                Even-Symmetric Filters
   Input:

   InL -- Input lowpass data;
   InH -- Input highpass data;
   PosFlag -- Start position of this segment (ODD or EVEN);
   Length -- Length of this Segment;
   Filter -- Filter used;
   ZeroHigh -- Highpass band is zero or not.

   Output:

   Out -- Output synthesized data segment;

   Return: Return DWT_OK if Successful
*/
#ifdef _DWT_INT_
 Int VTCIDWT:: SynthesizeSegmentEvenSymInt(DWTDATA *Out, DWTDATA *InL, DWTDATA *InH, 
				     Int PosFlag, Int Length, FILTER *Filter, Int ZeroHigh)
#else
 Int VTCIDWT:: SynthesizeSegmentEvenSymDbl(DWTDATA *Out, DWTDATA *InL, DWTDATA *InH, 
				     Int PosFlag, Int Length, FILTER *Filter, Int ZeroHigh)
#endif

{


#ifdef _DWT_INT_
  Short *LPCoeff = (Short *)Filter->LPCoeff, *HPCoeff = (Short *)Filter->HPCoeff; 
  Short *fi;
#else
  double *LPCoeff = (double *) Filter->LPCoeff, *HPCoeff = (double *)Filter->HPCoeff; 
  double *fi;
#endif
  Int ltaps = Filter->LPLength, htaps = Filter->HPLength; /* filter lengths*/
  Int loffset = ltaps/2, hoffset = htaps/2;  /* Filter offset */
  Int border = (ltaps>htaps)?ltaps:htaps; /*the larger of ltaps and htaps */
  Int m, n;
  DWTDATA *f,*pixel, *pixel1, val, *buf, *a, *tmp_out;
  Int r = Length-1;
  
  if(Length == 1) {
   /*  *Out = *InL  * Filter->Scale; */
    PosFlag = 0;
	ZeroHigh = DWT_ZERO_HIGH; // hjlee 0928
    /* return(DWT_OK); */
  }
  /* allocate proper buffer */
  buf = (DWTDATA *) malloc((Length+2*border+1)*sizeof(DWTDATA));
  if(buf==NULL)  return(DWT_MEMORY_FAILED);
  
  for(m=0;m<Length;m++) Out[m]= (DWTDATA) 0;
  for(m=0;m<Length+2*border+1;m++) buf[m]= (DWTDATA) 0;
  /*  fill in the low pass coefficients by upsampling by 2*/
  a = buf+border+1;
  
  for(m=-PosFlag; m< Length; m+=2) {
    a[m] = InL[(m+1)>>1];
  }
  
  /* symmetric extension */
  for (m=1 ; m<=border; m++) {
    a[-m-1] =   a[m-1];  /* to allow Shorter seg */ /*symmetric poInt a[-1] */
    a[r+m] =  a[r-m]; /*symmetric poInt a[r] */
  }

  f = buf + border + 1 + Length;
  tmp_out = Out;
  for (; a<f; a ++) {
    /* filter the pixel with lowpass filter */
    for( fi=LPCoeff, pixel=a-loffset, pixel1=pixel+ltaps-1, val=0, n=0; 
	 n<(ltaps>>1); n++, fi++, pixel++, pixel1--)
      val += ( *fi * (*pixel + *pixel1));
    *tmp_out++ = val;
  }
  /* if Highpass are zeros, skip */
  if(ZeroHigh== DWT_NONZERO_HIGH) {
    for(m=0;m<Length+2*border+1;m++) buf[m]= (DWTDATA) 0;
    a = buf+border+1;
    for(m=PosFlag;m< Length; m+=2) {
      a[m] = InH[m>>1];
    }
    /* anti-symmetric extension */
    for (m=1 ; m<=border; m++) {
      a[-m-1] =   -a[m-1];  /* to allow Shorter seg */ /*symmetric poInt a[-1] */
      a[r+m] =  -a[r-m];  /*symmetric poInt a[r] */
    } 
    tmp_out = Out;
    for (; a<f; a ++){
      /* filter the pixel with highpass filter */
      for(fi=HPCoeff, pixel=a-hoffset, pixel1 = pixel+ htaps-1, val=0, n=0; 
	  n<(htaps>>1); n++, fi++, pixel++, pixel1--) {
	val += ( *fi *  (*pixel - *pixel1));
      }
      *tmp_out++ += val;
    }
  }
  free(buf);
  return(DWT_OK);
}

#ifdef _DWT_INT_
#undef _DWT_INT_
#define _DWT_DBL_
#include "vtc_wavelet_idwt_aux.cpp"
#endif

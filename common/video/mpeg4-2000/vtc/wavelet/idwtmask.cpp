/* $Id: idwtmask.cpp,v 1.1 2003/05/05 21:24:03 wmaycisco Exp $ */
/****************************************************************************/
/*   MPEG4 Visual Texture Coding (VTC) Mode Software                        */
/*                                                                          */
/*   This software was developed by                                         */
/*   Sarnoff Corporation                   and    Texas Instruments         */
/*   Iraj Sodagar   (iraj@sarnoff.com)           Jie Liang (liang@ti.com)   */
/*   Hung-Ju Lee    (hjlee@sarnoff.com)                                     */
/*   Paul Hatrack   (hatrack@sarnoff.com)                                   */
/*   Shipeng Li     (shipeng@sarnoff.com)                                   */
/*   Bing-Bing Chai (bchai@sarnoff.com)                                     */
/*                                                                          */
/* In the course of development of the MPEG-4 standard. This software       */
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
/* Permission is granted to MPEG memebers to use, copy, modify,             */
/* and distribute the software modules ( or portions thereof )              */
/* for standardization activity within ISO/IEC JTC1/SC29/WG11.              */
/*                                                                          */
/* Copyright (C) 1998  Sarnoff Corporation and Texas Instruments            */ 
/****************************************************************************/

/************************************************************/
/*     Sarnoff Very Low Bit Rate Still Image Coder          */
/*     Copyright 1995, 1996, 1997, 1998 Sarnoff Corporation */
/************************************************************/

/* Inverse Mask DWT for MPEG-4 Still Texture Coding 
Original Coded by Shipeng Li, Sarnoff Corporation, Jan. 1998*/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include "basic.hpp"
#include "dwt.h"
/* local prototypes */



/* Function:    do_iDWTMask()
   Description: Inverse DWT for MPEG-4 Still Texture Coding 
   Input:

   InMask -- Input  Mask for InCoeff, if ==1, data inside object, 
              otherwise outside.
   Width  -- Width of Original Image (must be multiples of 2^nLevels);
   Height -- Height of Original Image (must be multiples of 2^nLevels);
   CurLevel -- Currect Decomposition Level of the input wavelet mask;
   DstLevel -- Destined decomposition level that the wavelet mask is 
               synthesized to. DstLevel always less than or equal to CurLevel;
   Filter     -- Filter Used.
   UpdateInput -- Update the level of decomposition to DstLevel  
                  for  InMask or Not.
                  0 = No  Update; 1 = Update  and InMask;

   Output:
 
   OutMask    -- Output mask

   Return: DWT_OK if successful.  
*/

Int VTCIDWTMASK::
do_iDWTMask(UChar *InMask, UChar *OutMask, Int Width, Int Height, Int CurLevel,
	 Int DstLevel, FILTER **Filter,  Int UpdateInput, Int FullSizeOut)
{

  Int i,level, k;
  Int ret;
 // UChar *c;
  UChar *d,*e, *tempMask;

  /* Check filter class first */
  for(level=CurLevel;level>DstLevel; level--) {
    if(Filter[level-1]->DWT_Class != DWT_ODD_SYMMETRIC && Filter[level-1]->DWT_Class != DWT_EVEN_SYMMETRIC ) {
      return(DWT_FILTER_UNSUPPORTED);
    }
  }
  /* Limit nLevels as: 0 - 15*/
  if(DstLevel < 0 || CurLevel >=16 || DstLevel >=16 || DstLevel > CurLevel) 
    return(DWT_INVALID_LEVELS);
  /* Check Width and Height, must be multiples of 2^CurLevel */
  if( (Width & ((1<<CurLevel)-1)) !=0) return(DWT_INVALID_WIDTH);
  if( (Height & ((1<<CurLevel)-1)) !=0) return(DWT_INVALID_HEIGHT);
  /* copy mask */
  tempMask = (UChar *)malloc(sizeof(UChar)*Width*Height);
  if(tempMask==NULL) return(DWT_MEMORY_FAILED);
  memcpy(tempMask, InMask, sizeof(UChar)*Width*Height);
  
  /* Perform the  iDWT */
  for(level=CurLevel;level>DstLevel;level--) {
    /* Synthesize one level */
    ret=SynthesizeMaskOneLevel(tempMask, Width, Height, 
			      level, Filter[level-1], DWT_NONZERO_HIGH);
    if(ret!=DWT_OK) {
      free(tempMask);
      return(ret);
    }
  }
  /* check to see if required to update InMask */
  if(UpdateInput>0) {
    /* Update InMask */
    for(k=0;k<Width*(Height>>DstLevel); k+=Width) {
      for(d=InMask+k,e=tempMask+k;d<InMask+k+(Width>>DstLevel);d++,e++) {
	*d = *e;
      }
    }
  }
  


  if(FullSizeOut) {
    /* Perform the rest of iDWT Mask till fullsize */
    for(level=DstLevel;level>0;level--) {
      /* Synthesize one level */
      ret=SynthesizeMaskOneLevel(tempMask, Width, Height, 
				level, Filter[level-1], DWT_ZERO_HIGH);
      if(ret!=DWT_OK) {
	free(tempMask);
	return(ret);
      }
    }
  }

  level=FullSizeOut?0:DstLevel;
  
  for(i=0,k=0;k<Width*(Height >> level);k+=Width,i+=(Width>>level)) {
    d = OutMask + i;
    for(e=tempMask+k;e<tempMask+k+(Width>>level);e++,d++)  *d=*e;
  }
  free(tempMask);
  return(DWT_OK);
}

/* Function:   SynthesizeMaskOneLevel() 
   Description: Synthesize one level of wavelet mask.
   Input:
   Width  -- Width of Image; 
   Height -- Height of Image;
   level -- current  level
   Filter     -- Filter Used.

   Input/Output:

   OutMask    -- Input/Output mask

   Return: DWT_OK if successful.
*/
Int VTCIDWTMASK::
SynthesizeMaskOneLevel(UChar *OutMask, Int Width,
				Int Height, Int level, FILTER *Filter, Int ZeroHigh)
{
  UChar *InMaskBuf, *OutMaskBuf;
  Int width = Width>>(level-1);
  Int height = Height>>(level-1);
  Int MaxLength = (width > height)?width:height;
  Int i,k,ret;
  UChar *c,*d;

  InMaskBuf = (UChar *) malloc(sizeof(UChar)*MaxLength);
  OutMaskBuf = (UChar *) malloc(sizeof(UChar)*MaxLength);
  if(InMaskBuf ==NULL || OutMaskBuf==NULL) 
    return(DWT_MEMORY_FAILED);
  if(ZeroHigh == DWT_ZERO_HIGH) {
    for(i=0;i<(width>>1);i++) {
      /* get a column of mask*/
      for( c= InMaskBuf, d= OutMask+i;
	   c<InMaskBuf+height; c+=2, d+=Width) {
	*c = *(c+1) = *d;
      }
      for(c= InMaskBuf, d= OutMask+i;
	  c<InMaskBuf+height;  c++, d+=Width) {
	*d = *c;
      }
    }
    for(i=0;i<Width*height;i+=Width) {
      for( c= InMaskBuf, d= OutMask+i;
	   c<InMaskBuf+width; c+=2, d++) {
	*c = *(c+1) = *d;
      }
      for(c= InMaskBuf, d= OutMask+i;
	  c<InMaskBuf+width;  c++, d++) {
	*d = *c;
      }
    }
  }
  else {
    /* vertical synthesis first */
    for(i=0;i<width;i++) {
      /* get a column of mask*/
      for( c= InMaskBuf, d= OutMask+i;
	   c<InMaskBuf+height; c++, d+=Width) {
	*c = *d;
      }
      /* perform inverse vertical SA-DWT of Mask*/
      ret=iSADWTMask1d(InMaskBuf,  OutMaskBuf, height, Filter, 
		       DWT_VERTICAL);
      if(ret!=DWT_OK) {
	free(InMaskBuf);free(OutMaskBuf);
	return(ret);
      }
      /* put the transformed column back */
      for(c= OutMaskBuf, d= OutMask+i;
	  c<OutMaskBuf+height;  c++, d+=Width) {
	*d = *c;
      }
    }    
    /* then horizontal synthesis */
    for(i=0,k=0;i<height;i++,k+=Width) {
      /* get a line of mask */
      memcpy(InMaskBuf, OutMask+k, sizeof(UChar)*width);
      /* Perform horizontal inverse SA-DWT */
      ret=iSADWTMask1d(InMaskBuf,  OutMaskBuf, width, Filter, 
		       DWT_HORIZONTAL);
      if(ret!=DWT_OK) {
	free(InMaskBuf);free(OutMaskBuf);
	return(ret);
      }
      /* put the transformed line back */
      memcpy(OutMask+k, OutMaskBuf, sizeof(UChar)*width);
    }
  }

  free(InMaskBuf);free(OutMaskBuf);
  return(DWT_OK);
}

/* Function:   SynthesizeMaskHalfLevel() 
   Description: Synthesize Half level of wavelet mask.
   Input:
   Width  -- Width of Image; 
   Height -- Height of Image;
   level -- current  level
   Filter     -- Filter Used.
   Direction -- VERTICAL or HORIZONTAL
   Input/Output:

   OutMask    -- Input/Output mask

   Return: DWT_OK if successful.
*/
Int VTCIDWTMASK::
SynthesizeMaskHalfLevel(UChar *OutMask, Int Width,
				  Int Height, Int level, FILTER *Filter, 
				  Int ZeroHigh, Int Direction)
{
  UChar *InMaskBuf, *OutMaskBuf;
  Int width = Width>>(level-1);
  Int height = Height>>(level-1);
  Int MaxLength = (width > height)?width:height;
  Int i,k,ret;
  UChar *c,*d;

  InMaskBuf = (UChar *) malloc(sizeof(UChar)*MaxLength);
  OutMaskBuf = (UChar *) malloc(sizeof(UChar)*MaxLength);
  if(InMaskBuf ==NULL || OutMaskBuf==NULL) 
    return(DWT_MEMORY_FAILED);
  if(ZeroHigh == DWT_ZERO_HIGH) {
    if(Direction == DWT_VERTICAL) {
      for(i=0;i<(width>>1);i++) {
	/* get a column of mask*/
	for( c= InMaskBuf, d= OutMask+i;
	     c<InMaskBuf+height; c+=2, d+=Width) {
	  *c = *(c+1) = *d;
	}
	for(c= InMaskBuf, d= OutMask+i;
	    c<InMaskBuf+height;  c++, d+=Width) {
	  *d = *c;
	}
      }
    }
    else {
      for(i=0;i<Width*height;i+=Width) {
	for( c= InMaskBuf, d= OutMask+i;
	     c<InMaskBuf+width; c+=2, d++) {
	  *c = *(c+1) = *d;
	}
	for(c= InMaskBuf, d= OutMask+i;
	    c<InMaskBuf+width;  c++, d++) {
	  *d = *c;
	}
      }
    }
  }
  else {
    if(Direction==DWT_VERTICAL) {
      /* vertical synthesis first */
      for(i=0;i<width;i++) {
	/* get a column of mask*/
	for( c= InMaskBuf, d= OutMask+i;
	     c<InMaskBuf+height; c++, d+=Width) {
	  *c = *d;
	}
	/* perform inverse vertical SA-DWT of Mask*/
	ret=iSADWTMask1d(InMaskBuf,  OutMaskBuf, height, Filter, 
			 DWT_VERTICAL);
	if(ret!=DWT_OK) {
	  free(InMaskBuf);free(OutMaskBuf);
	  return(ret);
	}
	/* put the transformed column back */
	for(c= OutMaskBuf, d= OutMask+i;
	    c<OutMaskBuf+height;  c++, d+=Width) {
	  *d = *c;
	}
      }    
    }
    else {
      /* then horizontal synthesis */
      for(i=0,k=0;i<height;i++,k+=Width) {
	/* get a line of mask */
	memcpy(InMaskBuf, OutMask+k, sizeof(UChar)*width);
	/* Perform horizontal inverse SA-DWT */
	ret=iSADWTMask1d(InMaskBuf,  OutMaskBuf, width, Filter, 
			 DWT_HORIZONTAL);
	if(ret!=DWT_OK) {
	  free(InMaskBuf);free(OutMaskBuf);
	  return(ret);
	}
	/* put the transformed line back */
	memcpy(OutMask+k, OutMaskBuf, sizeof(UChar)*width);
      }
    }
  }
  free(InMaskBuf);free(OutMaskBuf);
  return(DWT_OK);
}


/* Function: iSADWTMask1d() 
   Description:  inverse 1-d SA-DWT of Mask
   Input:

   InMaskBuf -- Input 1d mask buffer
   Length -- length of the input data
   Filter -- filter used
   Direction -- vertical or horizontal synthesis (used for inversible 
   mask synthsis)

   Output:
   
   OutMask -- Mask for the synthesized 1d Data

   Return: return DWT_OK if successful
  
*/
Int VTCIDWTMASK::
iSADWTMask1d(UChar *InMaskBuf,
	       UChar *OutMaskBuf, Int Length, FILTER *Filter, 
		Int Direction)
{

  switch(Filter->DWT_Class){
  case DWT_ODD_SYMMETRIC:
    return(iSADWTMask1dOddSym(InMaskBuf, OutMaskBuf, 
			    Length, Filter, Direction));
  case DWT_EVEN_SYMMETRIC:
    return(iSADWTMask1dEvenSym(InMaskBuf, OutMaskBuf, 
			     Length, Filter, Direction));
  default:
    return(DWT_FILTER_UNSUPPORTED);
  }
}

/* Function: iSADWTMask1dOddSym() 
   Description: 1D  iSA-DWT of Mask using Odd Symmetric Filter
   Input:

   InMaskBuf -- Input 1d mask buffer
   Length -- length of the input data
   Filter -- filter used
   Direction -- vertical or horizontal synthesis (used for inversible 
   mask synthesis)

   Output:
   
   OutMask -- Synthesized 1d Mask

   Return: return DWT_OK if successful

*/



Int VTCIDWTMASK::
iSADWTMask1dOddSym(UChar *InMaskBuf, 
			    UChar *OutMaskBuf, Int Length, FILTER *Filter, 
			    Int Direction)
{
 // Int i;
  //  Int SegLength = 0;
//  Int odd;
//  Int start, even, end;
  UChar *a, *b, *c;
 // Int ret;
  /* double check filter class and type */
  if(Filter->DWT_Class != DWT_ODD_SYMMETRIC) return(DWT_INTERNAL_ERROR);

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

  return(DWT_OK);
}


Int VTCIDWTMASK::
iSADWTMask1dEvenSym(UChar *InMaskBuf, 
			    UChar *OutMaskBuf, Int Length, FILTER *Filter, 
			    Int Direction)

{
  //Int i;
  //  Int SegLength = 0;
 // Int odd;
 // Int start, even, end;
  UChar *a, *b, *c;
 // Int ret;
  /* double check filter class and type */
  if(Filter->DWT_Class != DWT_EVEN_SYMMETRIC) return(DWT_INTERNAL_ERROR);
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
  return(DWT_OK);
}



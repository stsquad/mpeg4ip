/* $Id: dwtmask.cpp,v 1.1 2003/05/05 21:23:59 wmaycisco Exp $ */
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
/* Those Intending to use this software module in hardware or software      */
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

/* Forward SA-DWT Mask Decomposition for MPEG-4 Still Texture Coding 
Original Coded by Shipeng Li, Sarnoff Corporation, Jan. 1998*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <memory.h>
#include "basic.hpp"
#include "dwt.h"

/* Function:    DWTMask()
   Description: Forward DWT Mask Decompsoition(inversible) 
                for MPEG-4 Still Texture Coding 
   Input:

   InMask -- Input Object Mask for InData, if ==1, data inside object, 
             otherwise outside.
   Width  -- Width of Image (must be multiples of 2^nLevels);
   Height -- Height of Image (must be multiples of 2^nLevels);
   nLevels -- number of decomposition level,0 being original image;
   Filter     -- Filter Used.

   Output:

   OutMask    -- Output mask corresponding to wavelet coefficients

   Return: DWT_OK if success.  
*/

Int VTCDWTMASK::do_DWTMask(UChar *InMask, UChar *OutMask, Int Width, Int Height, Int nLevels,
	    FILTER **Filter)

{
  Int level;
  Int ret;
  /* Check filter class first */
  for(level=0;level<nLevels; level++) {
    if(Filter[level]->DWT_Class != DWT_ODD_SYMMETRIC && Filter[level]->DWT_Class != DWT_EVEN_SYMMETRIC ) {
      return(DWT_FILTER_UNSUPPORTED);
    }
  }

  /* Limit nLevels as: 0 - 15*/
  if(nLevels < 0 || nLevels >=16) return(DWT_INVALID_LEVELS);
  /* Check Width and Height, must be multiples of 2^nLevels */
  if((Width &( (1<<nLevels)-1))!=0) return(DWT_INVALID_WIDTH);
  if((Height &( (1<<nLevels)-1))!=0) return(DWT_INVALID_HEIGHT);
  /* Zero Level of Decomposition */
  /* copy mask */
  memcpy(OutMask, InMask, sizeof(UChar)*Width*Height);
  
  /* Perform the  DWT MASK Decomposition*/
  for(level=1;level<=nLevels;level++) {
    ret=DecomposeMaskOneLevel(OutMask, Width, Height, 
			      level, Filter[level-1]);
    if(ret!=DWT_OK) return(ret);
  }
  
  return(DWT_OK);
}

/* Function:   DecomposeMaskOneLevel()
   Description: Pyramid Decompose one level of wavelet coefficients mask
   Input:
   Width  -- Width of Image; 
   Height -- Height of Image;
   level -- current decomposition level
   Filter     -- Filter Used.
   Input/Output:

   OutMask    -- Output mask corresponding to wavelet coefficients

   Return: DWT_OK if success.
*/
Int VTCDWTMASK:: DecomposeMaskOneLevel(UChar *OutMask, Int Width,
				Int Height, Int level, FILTER *Filter)
{
  UChar *InMaskBuf, *OutMaskBuf;
  Int width = Width>>(level-1);
  Int height = Height>>(level-1);
  Int MaxLength = (width > height)?width:height;
  Int i,k,ret;
  UChar *c,*d;
  
  /* allocate line buffers */
  InMaskBuf = (UChar *) malloc(sizeof(UChar)*MaxLength);
  OutMaskBuf = (UChar *) malloc(sizeof(UChar)*MaxLength);
  if(InMaskBuf ==NULL || OutMaskBuf==NULL) 
    return(DWT_MEMORY_FAILED);
    
  /* horizontal decomposition first*/
  for(i=0,k=0;i<height;i++,k+=Width) {
    /* get a line of mask */
    memcpy(InMaskBuf, OutMask+k, sizeof(UChar)*width);
    /* Perform horizontal SA-DWT */
    ret=SADWTMask1d(InMaskBuf, OutMaskBuf, width, Filter, 
		    DWT_HORIZONTAL);
    if(ret!=DWT_OK) {
      free(InMaskBuf);free(OutMaskBuf);
      return(ret);
    }
    memcpy(OutMask+k, OutMaskBuf, sizeof(UChar)*width);
  }

  /* then vertical decomposition */
  for(i=0;i<width;i++) {
    /* get a column of coefficients */
    for(c= InMaskBuf, d= OutMask+i;
	c < InMaskBuf+height; c++,  d+=Width) {
      *c = *d;
    }
    /* perform vertical SA-DWT */
    ret=SADWTMask1d(InMaskBuf, OutMaskBuf, height, Filter, 
		    DWT_VERTICAL);
    if(ret!=DWT_OK) {
      free(InMaskBuf);free(OutMaskBuf);
      return(ret);
    }
    /* put the transformed column back */
    for(c= OutMaskBuf, d= OutMask+i;
	c<OutMaskBuf+height;  c++,  d+=Width) {
      *d = *c;
    }
  }
  free(InMaskBuf);free(OutMaskBuf);
  return(DWT_OK);
}

/* Function: SADWTMask1d()
   Description:  1-d SA-DWT Mask Decomposition
   Input:

   InMaskBuf -- Input 1d mask buffer
   Length -- length of the input data
   Filter -- filter used
   Direction -- vertical or horizontal decomposition (used for inversible 
   mask decomposition)

   Output:
   
   OutMask -- Mask for the Transformed 1d Data

   Return: return DWT_OK if success
  
*/

Int VTCDWTMASK:: SADWTMask1d(UChar *InMaskBuf, UChar *OutMaskBuf, Int Length, 
		FILTER *Filter, Int Direction)
{

  switch(Filter->DWT_Class){
  case DWT_ODD_SYMMETRIC:
    return(SADWTMask1dOddSym(InMaskBuf, OutMaskBuf, 
			    Length, Filter, Direction));
  case DWT_EVEN_SYMMETRIC:
    return(SADWTMask1dEvenSym(InMaskBuf, OutMaskBuf, 
			     Length, Filter, Direction));
    /*  case ORTHOGONAL:
	return(SADWT1dOrthInt(InBuf, InMaskBuf, OutBuf, OutMaskBuf, 
			  Length, Filter, Direction)); */
  default:
    return(DWT_FILTER_UNSUPPORTED);
  }
}

/* Function: SADWTMask1dOddSym()
   Description: 1D  SA-DWT Decomposition using Odd Symmetric Filter
   Input:

   InMaskBuf -- Input 1d mask buffer
   Length -- length of the input data
   Filter -- filter used
   Direction -- vertical or horizontal decomposition (used for inversible 
   mask decomposition)

   Output:
   
   OutMask -- Mask for the Transformed 1d Data

   Return: return DWT_OK if success

*/

Int VTCDWTMASK:: SADWTMask1dOddSym( UChar *InMaskBuf, UChar *OutMaskBuf, 
			      Int Length, FILTER *Filter, Int Direction)
     
{
  Int i;
  Int SegLength = 0;
  Int odd;
  Int start, end;
  UChar *a, *b, *c;
  /* double check filter class and type */
  if(Filter->DWT_Class != DWT_ODD_SYMMETRIC) return(DWT_INTERNAL_ERROR);
  /* double check if Length is even */
  if(Length & 1) return(DWT_INTERNAL_ERROR);
  /* initial mask output */
  for(a=InMaskBuf, b = OutMaskBuf, c= OutMaskBuf+(Length>>1); a<InMaskBuf+Length;) {
    *b++ = *a++;
    *c++ = *a++;
  }
  
  i = 0;
  a = InMaskBuf;
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
      /* swap the subsampled mask for single poInt if highpass is IN */
      if(Direction == DWT_HORIZONTAL) {
	/* After horizontal decomposition, 
	   LP mask symbols: IN, OUT0;
	   HP mask symbols: IN, OUT0, OUT1 */
	if(OutMaskBuf[start>>1]==DWT_OUT0) { 
	  OutMaskBuf[start>>1] = DWT_IN;
	  OutMaskBuf[(start>>1)+(Length>>1)]=DWT_OUT1;
	}
      }
      else { /* vertical */
	/* After vertical decomposition, 
	   LLP mask symbols: IN, OUT0;
	   LHP mask symbols: IN, OUT2, OUT3;
	   HLP mask symbols: IN, OUT0, OUT1;
	   HHP mask symbols: IN, OUT0, OUT1, OUT2, OUT3 */
	if(OutMaskBuf[start>>1] == DWT_OUT0) {
	  OutMaskBuf[(start>>1)+(Length>>1)]= DWT_OUT2 ;
	  OutMaskBuf[start>>1] = DWT_IN;
	}
	else if(OutMaskBuf[start>>1] == DWT_OUT1) {
	  OutMaskBuf[(start>>1)+(Length>>1)]= DWT_OUT3 ;
	  OutMaskBuf[start>>1] = DWT_IN;
	}
      }
    }
  }
  return(DWT_OK);
}

/* Function: SADWTMask1dEvenSym() 
   Description: 1D  SA-DWT Mask Decomposition using Even Symmetric Filter
   Input:

   InMaskBuf -- Input 1d mask buffer
   Length -- length of the input data
   Filter -- filter used
   Direction -- vertical or horizontal decomposition (used for inversible 
   mask decomposition)

   Output:
   
   OutMask -- Mask for the Transformed 1d Data

   Return: return DWT_OK if success

*/

Int VTCDWTMASK:: SADWTMask1dEvenSym( UChar *InMaskBuf,  UChar *OutMaskBuf, 
			       Int Length, FILTER *Filter, Int Direction)

{
  Int i;
  Int SegLength = 0;
  Int odd;
  Int start, end;
  UChar *a, *b, *c;

  /* double check filter class and type */
  if(Filter->DWT_Class != DWT_EVEN_SYMMETRIC) return(DWT_INTERNAL_ERROR);
  /* double check if Length is even */
  if(Length & 1) return(DWT_INTERNAL_ERROR);
  /* initial mask output */
  for(a=InMaskBuf, b = OutMaskBuf, c= OutMaskBuf+(Length>>1); a<InMaskBuf+Length;) {
    *b++ = *a++;
    *c++ = *a++;
  }
  
  i = 0;
  a = InMaskBuf;
  while(i<Length) {
    /* search for a segment */
    while(i<Length && (a[i])!=DWT_IN) i++;
    start = i;
    if(i >= Length) break;
    while(i<Length && (a[i])==DWT_IN) i++;
    end = i;
    SegLength = end-start;
    odd = start%2;
    /* swap the subsampled mask for the start of segment if it is odd*/
    if(odd) {
      if(Direction == DWT_HORIZONTAL) {
	/* After horizontal decomposition, 
	   LP mask symbols: IN, OUT0;
	   HP mask symbols: IN, OUT0, OUT1 */
	if(OutMaskBuf[start>>1]==DWT_OUT0) { 
	  OutMaskBuf[start>>1] = DWT_IN;
	  OutMaskBuf[(start>>1)+(Length>>1)]=DWT_OUT1;
	}
      }
      else { /* vertical */
	/* After vertical decomposition, 
	   LLP mask symbols: IN, OUT0;
	   LHP mask symbols: IN, OUT2, OUT3;
	   HLP mask symbols: IN, OUT0, OUT1;
	   HHP mask symbols: IN, OUT0, OUT1, OUT2, OUT3 */
	if(OutMaskBuf[start>>1] == DWT_OUT0) {
	  OutMaskBuf[(start>>1)+(Length>>1)]= DWT_OUT2 ;
	  OutMaskBuf[start>>1] = DWT_IN;
	}
	else if(OutMaskBuf[start>>1] == DWT_OUT1) {
	  OutMaskBuf[(start>>1)+(Length>>1)]= DWT_OUT3 ;
	  OutMaskBuf[start>>1] = DWT_IN;
	}
      }
    }
  }
  return(DWT_OK);
}

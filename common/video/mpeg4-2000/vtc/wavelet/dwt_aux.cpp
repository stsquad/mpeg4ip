/* $Id: dwt_aux.cpp,v 1.1 2003/05/05 21:23:58 wmaycisco Exp $ */
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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <memory.h>
#include "basic.hpp"
#include "dwt.h"
#define _DWT_INT_
#endif /* _DWT_DBL_ */

#undef DWTDATA

#ifdef _DWT_INT_
#define DWTDATA Int
#else
#define DWTDATA double
#endif

#ifdef _DWT_INT_
/* Function:   DecomposeOneLevelInt()
   Description: Pyramid Decompose one level of wavelet coefficients
   Input:
   Width  -- Width of Image; 
   Height -- Height of Image;
   level -- current decomposition level
   Filter     -- Filter Used.
   MaxCoeff, MinCoeff -- bounds of the output data;
   Input/Output:

   OutCoeff   -- Ouput wavelet coefficients
   OutMask    -- Output mask corresponding to wavelet coefficients

   Return: DWT_OK if success.
*/
Int VTCDWT:: DecomposeOneLevelInt(Int *OutCoeff, UChar *OutMask, Int Width,
				Int Height, Int level, FILTER *Filter,
				Int MaxCoeff, Int MinCoeff)
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
    
  /* horizontal decomposition first*/
  for(i=0,k=0;i<height;i++,k+=Width) {
    /* get a line of coefficients */
    for(a=InBuf, e=OutCoeff+k;a<InBuf+width;a++,e++) {
     
      *a = (Int) *e;
    }
    /* get a line of mask */
    memcpy(InMaskBuf, OutMask+k, sizeof(UChar)*width);
    /* Perform horizontal SA-DWT */
    ret=SADWT1dInt(InBuf, InMaskBuf, OutBuf, OutMaskBuf, width, Filter, 
		   DWT_HORIZONTAL);
    if(ret!=DWT_OK) {
      free(InBuf);free(OutBuf);free(InMaskBuf);free(OutMaskBuf);
      return(ret);
    }
    /* put the transformed line back */
    for(a=OutBuf,e=OutCoeff+k;a<OutBuf+width;a++,e++) {
      /* Scale and Leave 3 extra bits for precision reason */
      *a = ROUNDDIV((*a <<3), Filter->Scale);
      if(*a > MaxCoeff || *a < MinCoeff) {
	free(InBuf);free(OutBuf);free(InMaskBuf);free(OutMaskBuf);
	return(DWT_COEFF_OVERFLOW);
      }
      *e = (Int) *a;
    }
    memcpy(OutMask+k, OutMaskBuf, sizeof(UChar)*width);
  }

  /* then vertical decomposition */
  for(i=0;i<width;i++) {
    /* get a column of coefficients */
    for(a=InBuf, e=OutCoeff+i, c= InMaskBuf, d= OutMask+i;
	a<InBuf+height; a++, c++, e+=Width, d+=Width) {
      *a=(Int) *e;
      *c = *d;
    }
    /* perform vertical SA-DWT */
    ret=SADWT1dInt(InBuf, InMaskBuf, OutBuf, OutMaskBuf, height, Filter, 
		   DWT_VERTICAL);
    if(ret!=DWT_OK) {
      free(InBuf);free(OutBuf);free(InMaskBuf);free(OutMaskBuf);
      return(ret);
    }
    /* put the transformed column back */
    for(a=OutBuf, e=OutCoeff+i, c= OutMaskBuf, d= OutMask+i;
	a<OutBuf+height; a++, c++, e+=Width, d+=Width) {
      /*Scale and leave the sqrt(2)*sqrt(2) factor in the scale */
      *a = ROUNDDIV(*a, (Filter->Scale<<2)); 
      if(*a > MaxCoeff || *a < MinCoeff) {
	free(InBuf);free(OutBuf);free(InMaskBuf);free(OutMaskBuf);
	return(DWT_COEFF_OVERFLOW);
      }
      *e= (Int) *a;
      *d = *c;
    }
  }
  free(InBuf);free(OutBuf);free(InMaskBuf);free(OutMaskBuf);
  return(DWT_OK);
}
#else
/* Function:   DecomposeOneLevelDbl()
   Description: Pyramid Decompose one level of wavelet coefficients
   Input:
   Width  -- Width of Image; 
   Height -- Height of Image;
   level -- current decomposition level
   Filter     -- Filter Used.
   MaxCoeff, MinCoeff -- bounds of the output data;
   Input/Output:

   OutCoeff   -- Ouput wavelet coefficients
   OutMask    -- Output mask corresponding to wavelet coefficients

   Return: DWT_OK if success.
*/
Int VTCDWT:: DecomposeOneLevelDbl(double *OutCoeff, UChar *OutMask, Int Width,
				Int Height, Int level, FILTER *Filter)
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
    
  /* horizontal decomposition first*/
  for(i=0,k=0;i<height;i++,k+=Width) {
    /* get a line of coefficients */
    for(a=InBuf, e=OutCoeff+k;a<InBuf+width;a++,e++) {
      *a =  *e;
    }
    /* get a line of mask */
    memcpy(InMaskBuf, OutMask+k, sizeof(UChar)*width);
    /* Perform horizontal SA-DWT */
    ret=SADWT1dDbl(InBuf, InMaskBuf, OutBuf, OutMaskBuf, width, Filter, 
		   DWT_HORIZONTAL);
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

  /* then vertical decomposition */
  for(i=0;i<width;i++) {
    /* get a column of coefficients */
    for(a=InBuf, e=OutCoeff+i, c= InMaskBuf, d= OutMask+i;
	a<InBuf+height; a++, c++, e+=Width, d+=Width) {
      *a = *e;
      *c = *d;
    }
    /* perform vertical SA-DWT */
    ret=SADWT1dDbl(InBuf, InMaskBuf, OutBuf, OutMaskBuf, height, Filter, 
		   DWT_VERTICAL);
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
  free(InBuf);free(OutBuf);free(InMaskBuf);free(OutMaskBuf);
  return(DWT_OK);
}
#endif
/* Function: SADWT1dInt() or SADWT1dDbl()
   Description:  1-d SA-DWT
   Input:

   InBuf -- Input 1d data buffer
   InMaskBuf -- Input 1d mask buffer
   Length -- length of the input data
   Filter -- filter used
   Direction -- vertical or horizontal decomposition (used for inversible 
   mask decomposition)

   Output:
   
   OutBuf -- Transformed 1d Data
   OutMask -- Mask for the Transformed 1d Data

   Return: return DWT_OK if successful
  
*/
#ifdef _DWT_INT_
Int VTCDWT:: SADWT1dInt(Int *InBuf, UChar *InMaskBuf, Int *OutBuf, 
		      UChar *OutMaskBuf, Int Length, FILTER *Filter, 
		      Int Direction)
{

  switch(Filter->DWT_Class){
  case DWT_ODD_SYMMETRIC:
    return(SADWT1dOddSymInt(InBuf, InMaskBuf, OutBuf, OutMaskBuf, 
			    Length, Filter, Direction));
  case DWT_EVEN_SYMMETRIC:
    return(SADWT1dEvenSymInt(InBuf, InMaskBuf, OutBuf, OutMaskBuf, 
			     Length, Filter, Direction));
    /*  case ORTHOGONAL:
	return(SADWT1dOrthInt(InBuf, InMaskBuf, OutBuf, OutMaskBuf, 
			  Length, Filter, Direction)); */
  default:
    return(DWT_FILTER_UNSUPPORTED);
  }
}
#else
Int VTCDWT:: SADWT1dDbl(double *InBuf, UChar *InMaskBuf, double *OutBuf, 
		      UChar *OutMaskBuf, Int Length, FILTER *Filter, 
		      Int Direction)
{

  switch(Filter->DWT_Class){
  case DWT_ODD_SYMMETRIC:
    return(SADWT1dOddSymDbl(InBuf, InMaskBuf, OutBuf, OutMaskBuf, 
			    Length, Filter, Direction));
  case DWT_EVEN_SYMMETRIC:
    return(SADWT1dEvenSymDbl(InBuf, InMaskBuf, OutBuf, OutMaskBuf, 
			     Length, Filter, Direction));
    /*  case ORTHOGONAL:
	return(SADWT1dOrthDbl(InBuf, InMaskBuf, OutBuf, OutMaskBuf, 
			  Length, Filter, Direction)); */
  default:
    return(DWT_FILTER_UNSUPPORTED);
  }
}
#endif

/* Function: SADWT1dOddSymInt() or SADWT1dOddSymDbl()
   Description: 1D  SA-DWT using Odd Symmetric Filter
   Input:

   InBuf -- Input 1d data buffer
   InMaskBuf -- Input 1d mask buffer
   Length -- length of the input data
   Filter -- filter used
   Direction -- vertical or horizontal decomposition (used for inversible 
   mask decomposition)

   Output:
   
   OutBuf -- Transformed 1d Data
   OutMask -- Mask for the Transformed 1d Data

   Return: return DWT_OK if successful

*/



#ifdef _DWT_INT_
Int VTCDWT:: SADWT1dOddSymInt(DWTDATA *InBuf, UChar *InMaskBuf, DWTDATA *OutBuf, 
			    UChar *OutMaskBuf, Int Length, FILTER *Filter, 
			    Int Direction)
#else
Int VTCDWT:: SADWT1dOddSymDbl(DWTDATA *InBuf, UChar *InMaskBuf, DWTDATA *OutBuf, 
			    UChar *OutMaskBuf, Int Length, FILTER *Filter, 
			    Int Direction)
#endif

{
  Int i;
  Int SegLength = 0;
  Int odd;
  Int start, end;
  UChar *a, *b, *c;
  Int ret;
  /* double check filter class and type */
  if(Filter->DWT_Class != DWT_ODD_SYMMETRIC) return(DWT_INTERNAL_ERROR);
#ifdef _DWT_INT_
  if(Filter->DWT_Type != DWT_INT_TYPE) return(DWT_INTERNAL_ERROR);
#else
  if(Filter->DWT_Type != DWT_DBL_TYPE) return(DWT_INTERNAL_ERROR);
#endif
  /* double check if Length is even */
  if(Length & 1) return(DWT_INTERNAL_ERROR);
  /* initial mask output */
  for(a=InMaskBuf, b = OutMaskBuf, c= OutMaskBuf+(Length>>1); a<InMaskBuf+Length;) {
    *b++ = *a++;
    *c++ = *a++;
  }
  /* initial OutBuf to zeros */
  memset(OutBuf, (UChar)0, sizeof(DWTDATA)*Length);

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
#ifdef _DWT_INT_
      ret=DecomposeSegmentOddSymInt(InBuf+start, OutBuf+(start>>1), 
				    OutBuf+(Length>>1)+(start>>1), odd, 
				    SegLength, Filter);
#else
      ret=DecomposeSegmentOddSymDbl(InBuf+start, OutBuf+(start>>1), 
				    OutBuf+(Length>>1)+(start>>1), odd, 
				    SegLength, Filter);
#endif
	
      

      if(ret!=DWT_OK) return(ret);
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
    else {
#ifdef _DWT_INT_
      ret=DecomposeSegmentOddSymInt(InBuf+start, OutBuf+((start+1)>>1), 
				    OutBuf+(Length>>1)+(start>>1), odd, 
				    SegLength, Filter);

#else
      ret=DecomposeSegmentOddSymDbl(InBuf+start, OutBuf+((start+1)>>1), 
				    OutBuf+(Length>>1)+(start>>1), odd, 
				    SegLength, Filter);
#endif
      if(ret!=DWT_OK) return(ret);
    }
  }
  return(DWT_OK);
}

/* 
   Function:  DecomposeSegmentOddSymInt() or  DecomposeSegmentOddSymDbl()
   Description: SA-Decompose a 1D segment based on its InitPos and Length using
                Odd-Symmetric Filters
   Input:

   In -- Input data segment;
   PosFlag -- Start position of this segment (ODD or EVEN);
   Length -- Length of this Segment;
   Filter -- Filter used;
   
   Output:

   OutL -- Low pass data;
   OutH -- High pass data;

   Return: Return DWT_OK if Successful
*/
#ifdef _DWT_INT_
Int VTCDWT:: DecomposeSegmentOddSymInt(DWTDATA *In, DWTDATA *OutL, DWTDATA *OutH, 
				     Int PosFlag, Int Length, FILTER *Filter)
#else
Int VTCDWT:: DecomposeSegmentOddSymDbl(DWTDATA *In, DWTDATA *OutL, DWTDATA *OutH, 
				     Int PosFlag, Int Length, FILTER *Filter)
#endif

{
  /* filter coefficients */
#ifdef _DWT_INT_
  Short *LPCoeff = (Short *)Filter->LPCoeff, *HPCoeff = (Short *)Filter->HPCoeff; 
  Short *fi;
#else
  double *LPCoeff = (double *)Filter->LPCoeff, *HPCoeff = (double *)Filter->HPCoeff; 
  double *fi;
#endif
  Int ltaps = Filter->LPLength, htaps = Filter->HPLength; /* filter lengths*/
  Int loffset = ltaps/2, hoffset = htaps/2;  /* Filter offset */
  Int border = (ltaps>htaps)?ltaps:htaps; /*the larger of ltaps and htaps */
  Int m, n;
  DWTDATA *f,*pixel, *pixel1, val, *buf, *a;
  Int r = Length-1;
  /* prIntf("Length=%d\n", Length); */
  if(Length == 1) {
	*OutL = 0; // hjlee 0928
	for (m=0; m<ltaps;m++)
		*OutL += *In * (*(LPCoeff+m)); 
    /* *OutL = *In * Filter->Scale; */
    return (DWT_OK);
  }

  /* allocate proper buffer */
  buf = (DWTDATA *) malloc((Length+2*border)*sizeof(DWTDATA));
  if(buf==NULL)  return(DWT_MEMORY_FAILED);
  /* now symmetric extend this segment */
  a = buf+border;
  for(m=0;m< Length; m++) {
    a[m] = In[m];
    /* prIntf("%f ", a[m]); */
  }
  /* prIntf("\n"); */
  /* symmetric extension */
  for (m=1 ; m<=border; m++)
  {
    a[-m] =  a[m];  /* to allow Shorter seg */
    a[r+m] = a[r-m]; 
  }

  f = buf + border + Length;
  /* always subsample even positions in a line for LP coefficents */
  if (PosFlag==DWT_ODD) a = buf + border + 1;
  else a = buf + border;

  for (; a<f; a +=2)
  {
    /* filter the pixel with lowpass filter */
    for( fi=LPCoeff, pixel=a-loffset, pixel1=pixel+ltaps-1,val=0, n=0; 
	 n<(ltaps>>1); 
	 n++, fi++, pixel++, pixel1--)
      val += (*fi * (*pixel + *pixel1)); /* symmetric */
    val += (*fi * *pixel);
    *OutL++ = val;
  }
  /* always  subsample odd positions in a line for HP coefficients */
  if (PosFlag==DWT_ODD) a = buf + border;
  else a = buf + border+1;

  for (; a<f; a +=2)
    {
      /* filter the pixel with highpass filter */
      for(fi=HPCoeff, pixel=a-hoffset, pixel1 = pixel+htaps-1, val=0, n=0; 
	  n<(htaps>>1); 
	  n++, fi++, pixel++, pixel1--)
	val += (*fi *  (*pixel + *pixel1));  /* symmetric */
      val += (*fi * *pixel);
      *OutH++ = val;
    }
  free(buf);
  return(DWT_OK);
}

/* Function: SADWT1dEvenSymInt() or SADWT1dEvenSymDbl()
   Description: 1D  SA-DWT using Even Symmetric Filter
   Input:

   InBuf -- Input 1d data buffer
   InMaskBuf -- Input 1d mask buffer
   Length -- length of the input data
   Filter -- filter used
   Direction -- vertical or horizontal decomposition (used for inversible 
   mask decomposition)

   Output:
   
   OutBuf -- Transformed 1d Data
   OutMask -- Mask for the Transformed 1d Data

   Return: return DWT_OK if successful

*/
#ifdef _DWT_INT_
Int VTCDWT:: SADWT1dEvenSymInt(DWTDATA *InBuf, UChar *InMaskBuf, DWTDATA *OutBuf, 
			     UChar *OutMaskBuf, Int Length, FILTER *Filter, 
			     Int Direction)
#else
Int VTCDWT:: SADWT1dEvenSymDbl(DWTDATA *InBuf, UChar *InMaskBuf, DWTDATA *OutBuf, 
			     UChar *OutMaskBuf, Int Length, FILTER *Filter, 
			     Int Direction)
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
  if(Filter->DWT_Type != DWT_INT_TYPE)  return(DWT_INTERNAL_ERROR);
#else
  if(Filter->DWT_Type != DWT_DBL_TYPE)  return(DWT_INTERNAL_ERROR);
#endif
  /* double check if Length is even */
  if(Length & 1) return(DWT_INTERNAL_ERROR);
  /* initial mask output */
  for(a=InMaskBuf, b = OutMaskBuf, c= OutMaskBuf+(Length>>1); a<InMaskBuf+Length;) {
    *b++ = *a++;
    *c++ = *a++;
  }
  /* initial OutBuf to zeros */
  memset(OutBuf, (UChar)0, sizeof(DWTDATA)*Length);
  
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
#ifdef _DWT_INT_
      ret=DecomposeSegmentEvenSymInt(InBuf+start, OutBuf+(start>>1), 
				     OutBuf+(Length>>1)+(start>>1), odd, 
				     SegLength, Filter);
#else
      ret=DecomposeSegmentEvenSymDbl(InBuf+start, OutBuf+(start>>1), 
				     OutBuf+(Length>>1)+(start>>1), odd, 
				     SegLength, Filter);
#endif

      if(ret!=DWT_OK) return(ret);
 
    }
    else {
#ifdef _DWT_INT_
      ret=DecomposeSegmentEvenSymInt(InBuf+start, OutBuf+(start>>1), 
				     OutBuf+(Length>>1)+((start+1)>>1), odd, 
				     SegLength, Filter);
#else
      ret=DecomposeSegmentEvenSymDbl(InBuf+start, OutBuf+(start>>1), 
				     OutBuf+(Length>>1)+((start+1)>>1), odd, 
				     SegLength, Filter);
#endif
	
      if(ret!=DWT_OK) return(ret);
    }
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

/* 
   Function:  DecomposeSegmentEvenSymInt() or DecomposeSegmentEvenSymDbl()
   Description: SA-Decompose a 1D segment based on its InitPos and Length using
                Even-Symmetric Filters
   Input:

   In -- Input data segment;
   PosFlag -- Start position of this segment (ODD or EVEN);
   Length -- Length of this Segment;
   Filter -- Filter used;
   
   Output:

   OutL -- Low pass data;
   OutH -- High pass data;

   Return: Return DWT_OK if Successful
*/
#ifdef _DWT_INT_
Int VTCDWT:: DecomposeSegmentEvenSymInt(DWTDATA *In, DWTDATA *OutL, DWTDATA *OutH, 
				      Int PosFlag, Int Length, FILTER *Filter)
#else
Int VTCDWT:: DecomposeSegmentEvenSymDbl(DWTDATA *In, DWTDATA *OutL, DWTDATA *OutH, 
				      Int PosFlag, Int Length, FILTER *Filter)
#endif

{
  /* filter coefficients */
#ifdef _DWT_INT_
  Short *LPCoeff = (Short *)Filter->LPCoeff, *HPCoeff = (Short *)Filter->HPCoeff; 
  Short *fi;
#else
  double *LPCoeff = (double *)Filter->LPCoeff, *HPCoeff = (double *)Filter->HPCoeff; 
  double *fi; 
#endif
  Int ltaps = Filter->LPLength, htaps = Filter->HPLength; /* filter lengths*/
  Int loffset = ltaps/2-1, hoffset = htaps/2-1;  /* Filter offset */
  Int border = (ltaps>htaps)?ltaps:htaps; /*the larger of ltaps and htaps */
  Int m, n;
  DWTDATA *f,*pixel, *pixel1, val, *buf, *a;
  Int r = Length-1;

  if(Length == 1) {
	*OutL = 0;  // hjlee 0928
	for (m=0;m<ltaps; m++)
		*OutL += *In * (*(LPCoeff+m));  // hjlee 0928
    /* *OutL = *In * Filter->Scale; */
    return (DWT_OK);
  }
  /* allocate proper buffer */
  buf = (DWTDATA *) malloc((Length+2*border)*sizeof(DWTDATA));
  if(buf==NULL)  return(DWT_MEMORY_FAILED);
  /* now symmetric extend this segment */
  a = buf+border;
  for(m=0;m< Length; m++) {
    a[m] = In[m];
  }
  /* symmetric extension */
  for (m=1 ; m<=border; m++)
  {
    a[-m] =  a[m-1];  /* to allow Shorter seg */
    a[r+m] = a[r-m+1]; 
  }

  f = buf + border + Length;
  /* always subsample even positions in a line for LP coefficents */
  if (PosFlag==DWT_ODD) a = buf + border - 1;
  else a = buf + border;

  for (; a<f; a +=2)
  {
    /* filter the pixel with lowpass filter */
    for( fi=LPCoeff, pixel=a-loffset, pixel1=pixel+ltaps-1, val=0, n=0; 
	 n<(ltaps>>1); 
	 n++, fi++, pixel++, pixel1--)
      val += (*fi * (*pixel + *pixel1));  /* symmetric */

    *OutL++ = val;
  }
  /* always  subsample even positions in a line for HP coefficients */
  if (PosFlag==DWT_ODD) a = buf + border +1;
  else a = buf + border;

  for (; a<f; a +=2)
    {
      /* filter the pixel with highpass filter */
      for(fi=HPCoeff, pixel=a-hoffset, pixel1 = pixel+htaps-1, val=0, n=0; 
	  n<(htaps>>1); 
	  n++, fi++, pixel++, pixel1--)
	val += (*fi *  (*pixel - *pixel1)); /* antisymmetric */
      *OutH++ = val;
  }
  free(buf);
  return(DWT_OK);
}

#ifdef _DWT_INT_
#undef _DWT_INT_
#define _DWT_DBL_
#include "dwt_aux.cpp"
#endif

/* $Id: download_filter.cpp,v 1.2 2003/11/18 18:48:13 wmaycisco Exp $ */
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

/************************************************************/
/*  Filename: download_filter.c                             */
/*  Author: Bing-Bing Chai                                  */
/*  Date: Jan. 30, 1998                                     */
/*                                                          */
/*  Descriptions:                                           */
/*    This file contains the routines to up/download        */
/*    wavelet filters when desired. Proper conversions from */
/*    Floating poInt to Int are taken care of whenever      */
/*    needed.                                               */
/*                                                          */
/*    It is assumed that Short and Float                    */
/*    are 2 bytes, and 4 bytes respectively.                */
/*                                                          */
/************************************************************/
#define DOWN_DEBUG 0

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include "dataStruct.hpp"
#include "startcode.hpp"
#include "dwt.h"
#include "errorHandler.hpp"
#include "msg.hpp"
#include "bitpack.hpp"
//c#include <malloc.h>
#include "download_filter.h"

Void CVTCCommon::check_marker(Int marker_bit)
{
  if(marker_bit != MARKER_BIT)
    errorHandler("Error in download wavelet filters\n");
}


Void CVTCCommon::check_symmetry(FILTER *filter)
{
  Int i,half;

  /* -------- check lowpass filter --------- */
  half=filter->LPLength>>1;
  if(half<<1==filter->LPLength) 
    filter->DWT_Class=DWT_EVEN_SYMMETRIC;      /* even filter */
  else
    filter->DWT_Class=DWT_ODD_SYMMETRIC;      /* odd filter */

  for(i=0;i<half;i++)
    if(filter->DWT_Type == 0){
      if(((Short*)filter->LPCoeff)[i] !=
	 ((Short*)filter->LPCoeff)[filter->LPLength-i-1]) 
	errorHandler("Lowpass filter is not symmetric.\n");
    }
    else
      if(((double*)filter->LPCoeff)[i] !=
	 ((double*)filter->LPCoeff)[filter->LPLength-i-1]) 
	errorHandler("Lowpass filter is not symmetric.\n");


  /* -------- check highpass filter ---------- */
  half=filter->HPLength>>1;
  /* check for error */
  if(half<<1==filter->HPLength && filter->DWT_Class==DWT_ODD_SYMMETRIC)
    errorHandler("Lowpass filter has odd taps, while highpass filter has even"\
               " taps->\n");
  if(half<<1!=filter->HPLength && filter->DWT_Class==DWT_EVEN_SYMMETRIC) 
    errorHandler("Lowpass filter has even taps, while highpass filter has odd"\
               " taps.\n");

  if(filter->DWT_Class==DWT_ODD_SYMMETRIC){  /* ODD_SYMMETRIC, symmetric */
     for(i=0;i<half;i++)
      if(filter->DWT_Type == 0){
        if(((Short*)filter->HPCoeff)[i] !=
	   ((Short*)filter->HPCoeff)[filter->HPLength-i-1]) 
	  errorHandler("Highpass filter is not symmetric.\n");
      }
      else
	if(((double*)filter->HPCoeff)[i] !=
	   ((double*)filter->HPCoeff)[filter->HPLength-i-1]) 
	  errorHandler("Highpass filter is not symmetric.\n");
  }
  else{  /* EVEN_SYMMETRIC, antisymmetric */
     for(i=0;i<half;i++)
      if(filter->DWT_Type == 0){
        if(((Short*)filter->HPCoeff)[i] !=-
	   ((Short*)filter->HPCoeff)[filter->HPLength-i-1]) 
	  errorHandler("Highpass filter is not antisymmetric.\n");
      }
      else
	if(((double*)filter->HPCoeff)[i] !=-
	   ((double*)filter->HPCoeff)[filter->HPLength-i-1]) 
	  errorHandler("Highpass filter is not antisymmetric.\n");
  }

} 



Void CVTCCommon::upload_wavelet_filters(FILTER *filter)
{
  Int i;
  Float f;
  Short s;
  UShort *usptr;
  UInt *uIntptr;

  /* poInters to filter taps */
  usptr=(UShort *)&s;
  uIntptr=(UInt *)&f;

  /* filter lengths */
  emit_bits((UShort)filter->LPLength,4);
  emit_bits((UShort)filter->HPLength,4);

  /* upload lowpass filter */
  for(i=0;i<filter->LPLength;i++)
    if(filter->DWT_Type == DWT_INT_TYPE){
      s=((Short*)(filter->LPCoeff))[i];
      emit_bits(*usptr,16);
      emit_bits(MARKER_BIT,1);
    }
    else{
      f=(Float)((double *)filter->LPCoeff)[i];
      emit_bits((UShort)((*uIntptr)>>16),16);
      emit_bits(MARKER_BIT,1);
      emit_bits((UShort)(*uIntptr),16);
      emit_bits(MARKER_BIT,1);
    }

  /* upload highpass filter */
  for(i=0;i<filter->HPLength;i++)
    if(filter->DWT_Type == DWT_INT_TYPE){
      s=((Short*)(filter->HPCoeff))[i];
      emit_bits(*usptr,16);
      emit_bits(MARKER_BIT,1);
    }
    else{
      f=(Float)((double *)filter->HPCoeff)[i];
      emit_bits((UShort)((*uIntptr)>>16),16);
      emit_bits(MARKER_BIT,1);
      emit_bits((UShort)(*uIntptr),16);
      emit_bits(MARKER_BIT,1);
    }

  if(filter->DWT_Type == DWT_INT_TYPE){
    emit_bits((UShort)filter->Scale,16);
    emit_bits(MARKER_BIT,1);  /* needed to prevent start code emulation. */
                              /* this is not presented in the current syntax */
  }
}


Int CVTCCommon::download_wavelet_filters(FILTER **Filter, Int type) // modified by Sharp (99/2/16)
{
  Int i,marker_bit;
  double *LPD=NULL, *HPD=NULL;
  Short *LPS=NULL, *HPS=NULL,s;
  UShort *usptr;
  UInt *uIntptr;
  Float f;
  FILTER *filter; // hjlee 0901
	Int h_size = 0; // added by Sharp (99/2/16)

  usptr=(UShort *)&s;
  uIntptr=(UInt *)&f;

  /* hjlee 0901 */
  filter = (FILTER *)malloc(sizeof(FILTER));
  if(filter == NULL) 
    errorHandler("Memory allocation error\n");
  filter->DWT_Type = (type ==0)? DWT_INT_TYPE: DWT_DBL_TYPE;
  
  
  /* get filter lengths */
  filter->LPLength=get_X_bits(4);
  filter->HPLength=get_X_bits(4);
	h_size += 8; // added by Sharp (99/2/16)

  /* allocate memory for filters*/
  if(filter->DWT_Type==DWT_INT_TYPE){
    LPS=(Short *)malloc(sizeof(Short)*filter->LPLength);
    HPS=(Short *)malloc(sizeof(Short)*filter->HPLength);
    if(LPS==NULL || HPS==NULL)
      errorHandler("Cannot allocate memory to download wavelet filters\n");
    filter->LPCoeff=LPS;
    filter->HPCoeff=HPS;
  }
  else{
    LPD=(double *)malloc(sizeof(double)*filter->LPLength);
    HPD=(double *)malloc(sizeof(double)*filter->HPLength);
    if(LPD==NULL || HPD==NULL)
      errorHandler("Cannot allocate memory to download wavelet filters\n");
    filter->LPCoeff=LPD;
    filter->HPCoeff=HPD;
  }

  /* get lowpass filter */
  for(i=0;i<filter->LPLength;i++)
    if(filter->DWT_Type==DWT_INT_TYPE){
      *usptr=get_X_bits(16);
      LPS[i]=s;
      marker_bit=get_X_bits(1);
      check_marker(marker_bit);
			h_size += 17; // added by Sharp (99/2/16)
    }
    else{
      *uIntptr=get_X_bits(16);
      marker_bit=get_X_bits(1);
      check_marker(marker_bit);
      *uIntptr=((*uIntptr)<<16)+get_X_bits(16);
      marker_bit=get_X_bits(1);
      check_marker(marker_bit);
      LPD[i]=f;
			h_size += 34; // added by Sharp (99/2/16)
    }      


   /* get highpass filter */
  for(i=0;i<filter->HPLength;i++)
    if(filter->DWT_Type==DWT_INT_TYPE){
      *usptr=get_X_bits(16);
      HPS[i]=s;
      marker_bit=get_X_bits(1);
      check_marker(marker_bit);
			h_size += 17; // added by Sharp (99/2/16)
    }
    else{
      *uIntptr=get_X_bits(16);
      marker_bit=get_X_bits(1);
      check_marker(marker_bit);
      *uIntptr=((*uIntptr)<<16)+get_X_bits(16);
      marker_bit=get_X_bits(1);
      check_marker(marker_bit);
      HPD[i]=f;
			h_size += 34; // added by Sharp (99/2/16)
    }      


  check_symmetry(filter);

  if(filter->DWT_Type==DWT_INT_TYPE){
    filter->Scale=get_X_bits(16);
    marker_bit=get_X_bits(1);
    check_marker(marker_bit);
		h_size += 17; // added by Sharp (99/2/16)
  }
// added for FDAM1 by Samsung AIT on 2000/02/03
  *Filter = filter;
// ~added for FDAM1 by Samsung AIT on 2000/02/03
	return h_size; // added by Sharp (99/2/16)
}



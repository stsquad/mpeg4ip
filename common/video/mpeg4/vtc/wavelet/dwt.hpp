/* $Id: dwt.hpp,v 1.2 2001/05/09 21:14:18 cahighlander Exp $ */
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

/* DWT and inverse DWT header file 
   Created by Shipeng Li, Sarnoff Corporation, Jan. 1998
   Copyright (c) Sarnoff Corporation
*/
#ifndef _DWT_H_
#define _DWT_H_
#include "basic.hpp"

#define UChar UChar

#ifdef DATA
#undef DATA
#endif
#define DATA Int  
#ifdef INT
#undef INT
#endif
#define INT Int

#define ROUNDDIV(x, y) ((x)>0?(Int)(((Int)(x)+((y)>>1))/(y)):(Int)(((Int)(x)-((y)>>1)) /(y)))
enum {  /* DWT or IDWT return values */
  DWT_OK,
  DWT_FILTER_UNSUPPORTED,
  DWT_MEMORY_FAILED,
  DWT_COEFF_OVERFLOW,
  DWT_INVALID_LEVELS,
  DWT_INVALID_WIDTH,
  DWT_INVALID_HEIGHT,
  DWT_INTERNAL_ERROR,
  DWT_NOVALID_PIXEL
};

#ifdef RECTANGULAR
#undef RECTANGULAR
#endif
#define RECTANGULAR -1
/* Image data type */
enum { 
  UCHAR_ENUM,
  USHORT_ENUM
};
enum { /* filter class */
  ODD_SYMMETRIC,
  EVEN_SYMMETRIC,
  ORTHORGONAL
};

enum {
  EVEN,
  ODD
};

enum {
  HORIZONTAL,
  VERTICAL
};

enum {
  NONZERO_HIGH,
  ZERO_HIGH,
  ALL_ZERO
};

#define  OUT0 0
#define  IN   1
#define  OUT1 2
#define  OUT2 3
#define  OUT3 4

enum { /* filter type */
  INT_TYPE,
  DBL_TYPE
};

typedef struct {
  INT Class; 
  /* 0: Odd Symmetric 1: Even Symmetric 2: Orthogonal (not supported)
   Note: This is not defined by MPEG4-CD syntax but should be, since different 
   wavelets corresponds to different extension. 
   Ref. "Shape Adpative Discrete Wavelet Transform for Arbitrarily-Shaped 
   Visual Object Coding" S. Li and W. Li (submitted to IEEE T-CSVT) 
   */
  INT Type;
  /* 0: Short Coeff; 1: Double Coeff */
  INT HPLength;
  INT LPLength;
  Void *HPCoeff;
  Void *LPCoeff;
  INT Scale;
} FILTER;



#endif /*_DWT_H_ */

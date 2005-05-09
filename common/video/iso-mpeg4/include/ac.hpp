/* $Id: ac.hpp,v 1.1 2005/05/09 21:29:45 wmaycisco Exp $ */

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
/*  Filename: ac.h                                          */
/*                                                          */
/*  Descriptions:                                           */
/*    This is the header file of ac.c containing arithmetic */
/*    coding routines.                                      */
/*                                                          */
/************************************************************/

#ifndef AC_HEADER
#define AC_HEADER

//#include <stdio.h>
#include "basic.hpp"

//changed by Sarnoff for error resilience, 3/5/99
//#define MAX_BUFFER 1024
#define MAX_BUFFER 10000
//end changed by Sarnoff for error resilience, 3/5/99

typedef struct {
  FILE *fp;
  long low;
  long high;
  long followBits;
  Int buffer;
  Int bitsLeft;
  long bitCount;
  UChar *bitstream;
  long  bitstreamLength;
} ac_encoder;

typedef struct {
  FILE *fp;
  long value;
  long low;
  long high;
  Int buffer;
  Int bitsLeft;
  UChar *bitstream;
  long bitCount;
  long  bitstream_ptr;
  Int  dc_band;
} ac_decoder;

typedef struct {
  Int nsym;   /* the number of symbols in the symbol set to be encoded */
  Int adapt;  /* 0 => non-adaptive, !0 => adaptive */
  Int inc;

  /* for zeroth and mixed order models only */
  UShort *freq;
  UShort *cfreq;

  UShort Max_frequency;

} ac_model;



#endif

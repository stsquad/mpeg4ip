/* $Id: ztscan_common.hpp,v 1.1 2003/05/05 21:24:11 wmaycisco Exp $ */
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

#ifndef ZTSCAN_COMMON_H
#define ZTSCAN_COMMON_H
#include "basic.hpp"
#include "dataStruct.hpp"

/* definations related to arithmetic coding */
#define NUMCHAR_TYPE 4
#define ADAPT 1

//#ifndef ZTSCAN_UTIL   // hjlee 0901

extern WVT_CODEC mzte_codec;
#define EXTERN extern 

//#else _ZT_UTILS_
//#define EXTERN
//#endif  // hjlee 0901



/* Zeroth Order Model Filter - send NULL if first order (no zeroth order part)
 */
// #define ZOMF(x) (mzte_codec.m_iAcmOrder!=AC_MODEL_FIRST ? (x) : NULL) // hjlee 0901


/* The following functions assume that mzte_codec is a global variable */
// extern WVT_CODEC mzte_codec;  // hjlee 0901


// hjlee 0901
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/*                arithmetic coding probability models              */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

#define DEFAULT_MAX_FREQ 127 // change added from Norio Ito 8/16/99


/* arithmetic models related variables, refer to syntax for meaning */

/*~~~ DC model ~~~*/
EXTERN ac_model acmVZ[NCOLOR], *acm_vz; /* here only for DC */

/*~~~ AC models ~~~*/
#define MAX_NUM_TYPE_CONTEXTS 7
/* meaning of element in acmType */
/* if SQ or MQ 1st pass */
#define CONTEXT_INIT   0
#define CONTEXT_LINIT  1
/* if MQ non-1st pass */
#define CONTEXT_ZTR    2
#define CONTEXT_ZTR_D  3
#define CONTEXT_IZ     4
#define CONTEXT_LZTR   5
#define CONTEXT_LZTR_D 6 

#define Bitplane_Max_frequency 127

EXTERN ac_model acmType[NCOLOR][MAXDECOMPLEV][MAX_NUM_TYPE_CONTEXTS], 
  *acm_type[MAXDECOMPLEV][MAX_NUM_TYPE_CONTEXTS];

EXTERN ac_model acmSign[NCOLOR][MAXDECOMPLEV], *acm_sign[MAXDECOMPLEV];

EXTERN ac_model *acmBPMag[NCOLOR][MAXDECOMPLEV], **acm_bpmag;
EXTERN ac_model *acmBPRes[NCOLOR][MAXDECOMPLEV], **acm_bpres;
EXTERN ac_model *acm_bpdc; // 1127

//Added by Sarnoff for error resilience, 3/5/99
EXTERN ac_model acmSGMK;
EXTERN UShort ifreq[2];
//End: Added by Sarnoff for error resilience, 3/5/99

#endif 

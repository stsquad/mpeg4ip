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

/****************************************************************************/
/*     Texas Instruments Predictive Embedded Zerotree (PEZW) Image Codec    */
/*	   Developed by Jie Liang (liang@ti.com)                                */
/*													                        */ 
/*     Copyright 1996, 1997, 1998 Texas Instruments	      		            */
/****************************************************************************/

/****************************************************************************
   File name:         PEZW_zerotree.h
   Author:            Jie Liang  (liang@ti.com)
   Functions:         Header file for definition of symbols and variables
                      used in PEZW coder.
   Revisions:         v1.0 (10/04/98)
*****************************************************************************/
#ifndef PEZW_ZEROTREE_HPP
#define PEZW_ZEROTREE_HPP

#include "PEZW_ac.hpp"

/* declarations for zerotree coding */

#define ABS(x) (((x)<0)?-(x):(x))

/* symbols actuallry used for AC coding */
#define IZER   0             /* isolated zero */
#define IVAL   1             /* isolated valued coefficient */
#define ZTRZ   2             /* zero tree root: zero */
#define ZTRV   3             /* valued zerotree root */

/* for internal use only, not sent as zerotree symbols  */
#define DZ     4               /* descendant of a ZTRV */
#define DS     4               /* descendant of a ZTRS */
#define SKIP   13			/* out of shape */						
#define SKPZTR 14			/* out of shape and zero tree root */	


#define No_of_states_context0   9
#define No_of_states_context1   1
#define No_of_contexts No_of_states_context0*No_of_states_context1*Maximum_Decomp_Levels

#define NumBands               3
#define NumContext_per_pixel   6
#define NumContexts            NumBands*NumContext_per_pixel


/* SYMBOLS FOR INTERNAL USE */
#define MAXSIZE      1000000    /* maximum bitstream size handled */

/* Models for arithmetic coding */
/* THE SET OF SYMBOLS THAT MAY BE ENCODED. */

#define No_of_chars 4		/* Number of character symbols */
#define No_of_symbols (No_of_chars+1) 

/* maxmum frequence count */
#define Max_frequency_TI 127

/* context models */
extern int freq_dom_ZTRZ[No_of_symbols]; 
extern int freq_dom2_IZER[No_of_symbols];

/* debugging parameters */
#define DEBUG_BILEVEL       0
#define DEBUG_DEC_BS        0
#define DEBUG_BS            0
#define DEBUG_BS_DETAIL     0

#ifdef DEBUG_FILE
/* debugging variable */
FILE    *fp_debug;
#define DEBUG_BITSTREAM   0
#define DEBUG_SYMBOL      1
#define DEBUG_VALUE       1
#endif

#endif

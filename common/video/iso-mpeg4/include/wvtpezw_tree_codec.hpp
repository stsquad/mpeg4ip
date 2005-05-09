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
   File name:         wvtpezw_tree_codec.h
   Author:            Jie Liang  (liang@ti.com)
   Functions:         Header file for definition of global variables used 
                      inside the PEZW module code.
   Revisions:         v1.0 (10/04/98)
*****************************************************************************/

#include <stdio.h>
#include <wvtPEZW.hpp>

#include "PEZW_ac.hpp"

#define MAX_BITPLANE   16
#define MIN_BITPLANE   0

/* the wavelet tree buffer */
extern WINT *the_wvt_tree;

/* data structure for storing the maximum value of the
   wavelet coefficients */
extern WINT *wvt_tree_maxval; 
extern WINT MaxValue; 

/* weighting for different band */
extern int *snr_weight;

/* length of the tree structure */
extern int len_tree_struct;

/* the absolute value of the wavelet coefficients */
extern WINT *abs_wvt_tree;

/* location map for converting wavelets from blocks
   to trees */
extern int *vloc_map, *hloc_map;

/* data structure for information for zerotree coefficients
   that need to be coded */
extern short *ScanTrees;
extern short *next_ScanTrees;

/* the depth of the tree, or decomposition level */  
extern int tree_depth;

/* position and layer of all significant coefficients */
extern short *sig_pos;
extern char *sig_layer;

/* number of significant coefficients */
extern int num_Sig;

/* addition block of memory to allocate if the initial
   buffer is not enough */
#define Initial_Bufsize  100

/* arithmetic encoder data structure for each decomposition
   depth and bitplane */
extern Ac_encoder **Encoder;
extern Ac_decoder **Decoder;
extern unsigned char ***buffer_ptr;
extern unsigned char bptemp;

/* position of the starting point for each depth */
extern short *level_pos;

/* status of previous zerotree */
extern unsigned char *prev_label;

/* sign bit for decoder */
extern char *sign_bit;

/* models */
extern Ac_model *context_model;
extern Ac_model *model_sign;
extern Ac_model *model_sub;

/* mask */
extern unsigned int *maskbit;
extern unsigned char *bitplane;

/* the adaptation mode of the arithmetic coder:
   1- adaptive, 0- fixed  */
#define ADAPTATION_MODE   1

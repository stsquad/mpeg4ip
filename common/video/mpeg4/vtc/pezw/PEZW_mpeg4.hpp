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
   File name:         PEZW_mpeg4.h
   Author:            Jie Liang  (liang@ti.com)
   Functions:         define data structures for interface with other parts 
                      of MPEG4 software
   Revisions:         v1.0 (10/04/98)
*****************************************************************************/
#ifndef PEZW_MPEG4_HPP
#define PEZW_MPEG4_HPP

#include "PEZW_ac.hpp"

#define Maximum_Decomp_Levels   10
#define No_of_states_context0   9
#define No_of_states_context1   1
#define No_of_contexts No_of_states_context0*No_of_states_context1*Maximum_Decomp_Levels

/* data type used for store wavelet coefficients */ 
typedef struct array_2D {
   int width;
   int height;
   void *data;
   unsigned char *mask;
} Array_2D;

typedef struct array_1D {
  int length;
  void *data;
#ifdef _SA_DWT_
   unsigned char *mask;
#endif
} Array_1D;

typedef struct pezw_snr_layer {
  int       Quant;
  int       allzero;
  int       bits_to_go;
  Array_2D  snr_image;
  Array_1D  snr_bitstream;
} PEZW_SNR_LAYER;

typedef struct pezw_spatial_layer {
  int           spatial_bitstream_length;
  int           SNR_scalability_levels;
  PEZW_SNR_LAYER    *SNRlayer;
} PEZW_SPATIAL_LAYER;

/* data structure for the PEZW decoder */
typedef struct pezw_decoder {
	short *valueimage;                /* dequantized coeffs */
	unsigned char *prev_labelimage;  /* zerotree symbols in previous pass */
	unsigned char *labelimage;
	unsigned char **labelimage_spa;  
	unsigned char *mask;
	int *sizes, *SPsizes;            /* decoded sizes */
	Ac_decoder *AC_decoder;          /* AC_decoder data structure */
	int *currthresh;				 /* current thresh */
	int bplane;                      /* the SNR levels */
	int *budget;                     /* budget for each band */
	int levels;						 /* levels of decomposition */ 
	int hsize;                       /* image height */
	int vsize;                       /* image width */

	/* models */
	Ac_model model_context[No_of_contexts];
	Ac_model model_sub, model_sign;

	/* debugging information */
	int countIZER;
	int countIVAL;
	int countZTRZ;
	int countZTRV;
	int cIZ[No_of_contexts];
	int cIS[No_of_contexts];
	int cZTRZ[No_of_contexts];
    int cZTRS[No_of_contexts];
} PEZW_DECODER;

/* maximum number of zero runs for start code violation checking */
#define MAXRUNZERO    22

#endif





 

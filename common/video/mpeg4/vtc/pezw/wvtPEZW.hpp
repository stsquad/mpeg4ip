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
   File name:         wvtPEZW.h
   Author:            Jie Liang  (liang@ti.com)
   Functions:         header file that defines global variables that are the 
                      interface of PEZW module and other parts of MPEG4 code.
   Revisions:         v1.0 (10/04/98)
*****************************************************************************/

#ifndef WVTPEZW_H
#define WVTPEZW_H

#include <stdio.h>
#include <basic.hpp>

/* data type used for store wavelet coefficients */
typedef short WINT;   

/* interface with PEZW modules */
 
/* the pointers to bitstream buffers for different
   spatial and SNR levels */
extern unsigned char ***PEZW_bitstream;

/* the initial size allocated to each buffer
   (each spatial layer, at each bitplane has its buffer)
   when the encoding is done this buffer contains
   the length of each buffer  */
extern int **Init_Bufsize;

/* decoded bytes */
extern int **decoded_bytes;

/* effective bits in last byte of the bitstream buffer */
extern unsigned char **bits_to_go_inBuffer;

/* maxmum number of bitplane */
extern int Max_Bitplane;

/* the minumum bitplane to code */
extern int Min_Bitplane;

/* spatial decoding levels offset */
extern int spatial_leveloff;

/* reate control parameters */
extern unsigned char **reach_budget;

/* decoding parameters imported form main.c.
   they should be defined before calling PEZW decoder. */
extern int PEZW_target_spatial_levels;
extern int PEZW_target_snr_levels;
extern int PEZW_target_bitrate;

#endif

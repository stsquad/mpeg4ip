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
   File name:         PEZW_ac.h
   Author:            Jie Liang  (liang@ti.com)
   Functions:         header file for adaptive arithmetic coding functions
   Revisions:         This file was adopted from public domain
                      arithmetic coder with changes suited to PEZW coder.
*****************************************************************************/

#ifndef PEZW_AC_HEADER
#define PEZW_AC_HEADER

#include <stdio.h>

typedef struct {
  FILE *fp;
  unsigned char *stream;
  long low;
  long high;
  long fbits;
  int buffer;
  int bits_to_go;
  long total_bits;

  /* buffer management */
  unsigned char *original_stream;
  int space_left;  /* in bytes */

} Ac_encoder;

typedef struct {
  FILE *fp;
  unsigned char *stream;
  long value;
  long low;
  long high;
  int  buffer;
  int bits_to_go;
  int garbage_bits;
} Ac_decoder;

typedef struct {
  int nsym;
  int Max_frequency;
#ifndef MODEL_COUNT_LARGE
  unsigned char *freq;
#else
  int *freq;
#endif
  int *cfreq;
  int adapt;
} Ac_model;

void Ac_encoder_init (Ac_encoder *, unsigned char *, int, int);
void Ac_encoder_done (Ac_encoder *);
void Ac_decoder_open (Ac_decoder *, unsigned char *, int);
void Ac_decoder_init (Ac_decoder *, unsigned char *);
void Ac_decoder_done (Ac_decoder *);
void Ac_model_init (Ac_model *, int, int *, int, int);
void Ac_model_save (Ac_model *, int *);
void Ac_model_done (Ac_model *);
long Ac_encoder_bits (Ac_encoder *);
void Ac_encode_symbol (Ac_encoder *, Ac_model *, int);
int Ac_decode_symbol (Ac_decoder *, Ac_model *);
int getc_buffer (unsigned char **buffer);
void putc_buffer (int x, unsigned char **buffer_curr, 
         unsigned char **buffer_start, int *space_len);
int AC_decoder_buffer_adjust (Ac_decoder *acd);
void AC_free_model (Ac_model *acm);

#endif

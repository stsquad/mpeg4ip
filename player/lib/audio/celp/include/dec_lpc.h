/**********************************************************************
MPEG-4 Audio VM
Deocoder cores (parametric, LPC-based, t/f-based)



This software module was originally developed by

Heiko Purnhagen (University of Hannover / ACTS-MoMuSys)

and edited by

in the course of development of the MPEG-2 NBC/MPEG-4 Audio standard
ISO/IEC 13818-7, 14496-1,2 and 3. This software module is an
implementation of a part of one or more MPEG-2 NBC/MPEG-4 Audio tools
as specified by the MPEG-2 NBC/MPEG-4 Audio standard. ISO/IEC gives
users of the MPEG-2 NBC/MPEG-4 Audio standards free license to this
software module or modifications thereof for use in hardware or
software products claiming conformance to the MPEG-2 NBC/ MPEG-4 Audio
standards. Those intending to use this software module in hardware or
software products are advised that this use may infringe existing
patents. The original developer of this software module and his/her
company, the subsequent editors and their companies, and ISO/IEC have
no liability for use of this software module or modifications thereof
in an implementation. Copyright is not released for non MPEG-2
NBC/MPEG-4 Audio conforming products. The original developer retains
full right to use the code for his/her own purpose, assign or donate
the code to a third party and to inhibit third party from using the
code for non MPEG-2 NBC/MPEG-4 Audio conforming products. This
copyright notice must be included in all copies or derivative works.

Copyright (c) 1996.



Header file: dec.h

$Id: dec_lpc.h,v 1.1 2002/05/13 18:57:42 wmaycisco Exp $

Authors:
HP    Heiko Purnhagen, Uni Hannover <purnhage@tnt.uni-hannover.de>
BT    Bodo Teichmann, FhG/IIS <tmn@iis.fhg.de>

Changes:
18-jun-96   HP    first version
24-jun-96   HP    fixed comment
15-aug-96   HP    added DecXxxInfo(), DecXxxFree()
                  changed DecXxxInit(), DecXxxFrame() interfaces to
                  enable multichannel signals / float fSample, bitRate
26-aug-96   HP    CVS
03-sep-96   HP    added speed change & pitch change for parametric core
19-feb-97   HP    added include <stdio.h>
04-apr-97   BT    added DecG729Init() DecG729Frame()
09-apr-99   HP    added DecHvxcXxx() from dec_hvxc.h
22-apr-99   HP    created from dec.h
**********************************************************************/


#ifndef _dec_lpc_h_
#define _dec_lpc_h_


#include <stdio.h>              /* typedef FILE */
#include "bitstreamHandle.h"	/* bit stream handle */
#include "obj_descr.h"
#include "lpc_common.h"         /* TDS */


/* ---------- functions ---------- */

#ifdef __cplusplus
extern "C" {
#endif

/* DecLpcInfo() */
/* Get info about LPC-based decoder core. */

char *DecLpcInfo (
  FILE *helpStream);		/* in: print decPara help text to helpStream */
				/*     if helpStream not NULL */
				/* returns: core version string */


/* DecLpcInit() */
/* Init LPC-based decoder core. */

void DecLpcInit (
  int numChannel,		/* in: num audio channels */
  float fSample,		/* in: sampling frequency [Hz] */
  float bitRate,		/* in: total bit rate [bit/sec] */
  char *decPara,		/* in: decoder parameter string */
  BsBitBuffer *bitHeader,	/* in: header from bit stream */
  int *frameNumSample,		/* out: num samples per frame */
  int *delayNumSample);		/* out: decoder delay (num samples) */

void DecLpcInitNew (
  char *decPara,                   /* in: decoder parameter string     */
  FRAME_DATA*  frameData,
  LPC_DATA*    lpcData,
  int layer
  ) ;            /* out: decoder delay (num samples) */


/* DecLpcFrame() */
/* Decode one bit stream frame into one audio frame with */
/* LPC-based decoder core. */

void DecLpcFrame (
  BsBitBuffer *bitBuf,		/* in: bit stream frame */
  float **sampleBuf,		/* out: audio frame samples */
				/*     sampleBuf[numChannel][frameNumSample] */
  int *usedNumBit);		/* out: num bits used for this frame */


void DecLpcFrameNew (
  BsBitBuffer *bitBuf,    /* in: bit stream frame                      */
  float **sampleBuf,      /* out: audio frame samples                  */
                          /*     sampleBuf[numChannel][frameNumSample] */
  LPC_DATA*    lpcData,
  int *usedNumBit);        /* out: num bits used for this frame         */

/* DecLpcFree() */
/* Free memory allocated by LPC-based decoder core. */

void DecLpcFree (void);

int lpcframelength( CELP_SPECIFIC_CONFIG *celpConf );

#ifdef __cplusplus
}
#endif

#endif	/* #ifndef _dec_lpc_h_ */

/* end of dec_lpc.h */

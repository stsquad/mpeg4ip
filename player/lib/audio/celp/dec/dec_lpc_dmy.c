/**********************************************************************
MPEG-4 Audio VM
Decoder core (LPC-based)



This software module was originally developed by

Heiko Purnhagen (University of Hannover)

and edited by

Ali Nowbakht-Irani (FhG-IIS)

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



Source file: dec_lpc_dmy.c

$Id: dec_lpc_dmy.c,v 1.1 2002/05/13 18:57:42 wmaycisco Exp $

Required modules:
common.o		common module
cmdline.o		command line module
bitstream.o		bits stream module

Authors:
HP    Heiko Purnhagen, Uni Hannover <purnhage@tnt.uni-hannover.de>

Changes:
24-jun-96   HP    dummy core
15-aug-96   HP    adapted to new dec.h
26-aug-96   HP    CVS
**********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "block.h"               /* handler, defines, enums */
#include "buffersHandle.h"       /* handler, defines, enums */
#include "concealmentHandle.h"   /* handler, defines, enums */
#include "interface.h"           /* handler, defines, enums */
#include "mod_bufHandle.h"       /* handler, defines, enums */
#include "reorderspecHandle.h"   /* handler, defines, enums */
#include "resilienceHandle.h"    /* handler, defines, enums */
#include "tf_mainHandle.h"       /* handler, defines, enums */

#include "lpc_common.h"          /* defines, structs */
#include "obj_descr.h"           /* structs */
#include "nok_ltp_common.h"      /* structs*/
#include "tf_mainStruct.h"       /* structs */

#include "common_m4a.h"		/* common module */
#include "cmdline.h"		/* command line module */
#include "bitstream.h"		/* bit stream module */

#include "dec_lpc.h"		/* decoder cores */


/* ---------- declarations ---------- */

#define PROGVER "LPC-based decoder core dummy"


/* ---------- functions ---------- */


/* DecLpcInfo() */
/* Get info about LPC-based decoder core. */

char *DecLpcInfo (
  FILE *helpStream)		/* in: print decPara help text to helpStream */
				/*     if helpStream not NULL */
				/* returns: core version string */
{
  if (helpStream != NULL)
    fprintf(helpStream,PROGVER "\n\n");
  return PROGVER;
}


/* DecLpcInit() */
/* Init LPC-based decoder core. */

void DecLpcInit (
  int numChannel,		/* in: num audio channels */
  float fSample,		/* in: sampling frequency [Hz] */
  float bitRate,		/* in: total bit rate [bit/sec] */
  char *decPara,		/* in: decoder parameter string */
  BsBitBuffer *bitHeader,	/* in: header from bit stream */
  int *frameNumSample,		/* out: num samples per frame */
  int *delayNumSample)		/* out: decoder delay (num samples) */
{
  CommonExit(1,"DecLpcInit: dummy");
}


/* DecLpcFrame() */
/* Decode one bit stream frame into one audio frame with */
/* LPC-based decoder core. */

void DecLpcFrame (
  BsBitBuffer *bitBuf,		/* in: bit stream frame */
  float **sampleBuf,		/* out: audio frame samples */
				/*     sampleBuf[numChannel][frameNumSample] */
  int *usedNumBit)		/* out: num bits used for this frame */
{
  CommonExit(1,"DecLpcFrame: dummy");
}


/* DeLpcrFree() */
/* Free memory allocated by LPC-based decoder core. */

void DecLpcFree (void)
{
  CommonExit(1,"DecLpcFree: dummy");
}

void DecLpcFrameNew (
  BsBitBuffer *bitBuf,    /* in: bit stream frame                      */
  float **sampleBuf,      /* out: audio frame samples                  */
                          /*     sampleBuf[numChannel][frameNumSample] */
  LPC_DATA*    lpcData,
  int *usedNumBit)        /* out: num bits used for this frame         */
{
  CommonExit(1,"DecLpcFrameNew: dummy");
}


void DecLpcInitNew (
  char *decPara,                   /* in: decoder parameter string     */
  FRAME_DATA*  fD,
  LPC_DATA*    lpcData,
  int layer
  )             /* out: decoder delay (num samples) */
{
  CommonExit(1,"DecLpcInitNew: dummy");
}

/* end of dec_lpc_dmy.c */


/**********************************************************************
MPEG-4 Audio VM
audio i/o streams (.au format)



This software module was originally developed by

Heiko Purnhagen (University of Hannover)

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

Copyright (c) 1999.



Header file: austream.h

$Id: austream.h,v 1.1 2002/05/13 18:57:42 wmaycisco Exp $

Authors:
HP    Heiko Purnhagen, Uni Hannover <purnhage@tnt.uni-hannover.de>
NM    Nikolaus Meine, Uni Hannover <meine@tnt.uni-hannover.de>

Changes:
16-sep-98   HP/NM   born, based on au_io.c
**********************************************************************/

/* Audio i/o streaming with stdin/stdout support. */
/* Header format: .au (Sun audio file format, aka SND or AFsp) */
/* Sample format: 16 bit twos complement, uniform quantisation */
/* Data size set to -1 (=unknown) */

/* Multi channel data is interleaved: l0 r0 l1 r1 ... */
/* Total number of samples (over all channels) is used. */


#ifndef _austream_h_
#define _austream_h_


/* ---------- declarations ---------- */

typedef struct AuStreamStruct AuStream;	/* audio stream handle */


/* ---------- functions ---------- */

#ifdef __cplusplus
extern "C" {
#endif


/* AuInit() */
/* Init audio i/o streams. */

void AuInit (
  int debugLevel);		/* in: debug level */
				/*     0=off  1=basic  2=full */


/* AuOpenRead() */
/* Open audio stream for reading. */

AuStream *AuOpenRead (
  char *streamName,		/* in: stream name, "-" for stdin */
  int *numChannel,		/* out: number of channels */
  float *fSample,		/* out: sampling frequency [Hz] */
  long *numSample);		/* out: number of samples in stream */
				/*      or -1 if not available */
				/* returns: */
				/*  audio stream (handle) */
				/*  or NULL if error */


/* AuOpenWrite() */
/* Open audio stream for writing. */

AuStream *AuOpenWrite (
  char *streamName,		/* in: stream name, "-" for stdout */
  int numChannel,		/* in: number of channels */
  float fSample);		/* in: sampling frequency [Hz] */
				/* returns: */
				/*  audio stream (handle) */
				/*  or NULL if error */


/* AuReadData() */
/* Read data from audio stream. */

long AuReadData (
  AuStream *stream,		/* in: audio stream (handle) */
  short *data,			/* out: data[] */
  long numSample);		/* in: number of samples to be read */
				/* returns: */
				/*  number of samples read */


/* AuWriteData() */
/* Write data to audio stream. */

void AuWriteData (
  AuStream *stream,		/* in: audio stream (handle) */
  short *data,			/* in: data[] */
  long numSample);		/* in: number of samples to be written */


/* AuClose() */
/* Close audio stream.*/

void AuClose (
  AuStream *stream);		/* in: audio stream (handle) */


#ifdef __cplusplus
}
#endif

#endif	/* #ifndef _austream_h_ */

/* end of austream.h */


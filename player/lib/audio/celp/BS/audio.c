/**********************************************************************
MPEG-4 Audio VM
Audio i/o module



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

Copyright (c) 1996, 1999.



Source file: audio.c

$Id: audio.c,v 1.2 2002/07/15 22:44:57 wmaycisco Exp $

Required libraries:
libtsp.a		AFsp audio file library

Required modules:
common.o		common module
austream.o		audio i/o streams (.au format)

Authors:
HP    Heiko Purnhagen, Uni Hannover <purnhage@tnt.uni-hannover.de>
BT    Bodo Teichmann, FhG/IIS <tmn@iis.fhg.de>

Changes:
21-jan-97   HP    born (using AFsp-V2R2)
27-jan-97   HP    set unavailable samples to 0 in AudioReadData()
03-feb-97   HP    fix bug AudioInit formatString=NULL
19-feb-97   HP    made internal data structures invisible
21-feb-97   BT    raw: big-endian
12-sep-97   HP    fixed numSample bug for mch files in AudioOpenRead()
30-dec-98   HP    uses austream for stdin/stdout, evaluates USE_AFSP
07-jan-99   HP    AFsp-v4r1 (AFsp-V3R2 still supported)
11-jan-99   HP    clipping & seeking for austream module
17-jan-99   HP    fixed quantisation to 16 bit
26-jan-99   HP    improved output file format evaluation
17-may-99   HP    improved output file format detection
**********************************************************************/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#ifdef USE_AFSP
#include <libtsp.h>		/* AFsp audio file library */
#include <libtsp/AFpar.h>	/* AFsp audio file library - definitions */
#endif

#include "audio.h"		/* audio i/o module */
#include "common_m4a.h"		/* common module */
#include "austream.h"		/* audio i/o streams (.au format) */
#include "bitstream.h"

/* ---------- declarations ---------- */

#define SAMPLE_BUF_SIZE 16384	/* local sample buffer size */

#define min(a,b) ((a) < (b) ? (a) : (b))
#define max(a,b) ((a) > (b) ? (a) : (b))

#ifdef USE_AFSP
#ifdef FW_SUN
/* only AFsp-V3R2 available: map AFsp-V3R2 to AFsp-v4r1 */
#define AFsetNHpar AFsetNH
#define FTW_AU (FW_SUN/256)
#define FTW_WAVE (FW_WAVE/256)
#define FTW_AIFF_C (FW_AIFF_C/256)
#define FTW_NH_EB (FW_NH_EB/256)
#endif
#endif


/* ---------- declarations (structures) ---------- */

struct AudioFileStruct		/* audio file handle */
{
#ifdef USE_AFSP
  AFILE *file;			/* AFILE handle */
#else
  int *file;
#endif
  AuStream *stream;		/* AuStream handle */
				/*   NULL if AFsp used */
  int numChannel;		/* number of channels */
  long currentSample;		/* number of samples read/written */
				/* (samples per channel!) */
  int write;			/* 0=read  1=write */
  long numClip;			/* number of samples clipped */
};


/* ---------- variables ---------- */

static int AUdebugLevel = 0;	/* debug level */


/* ---------- local functions ---------- */

int isfmtstr (char *filename, char *fmtstr)
/* isfmtstr returns true if filename has extension fmtstr */
{
  int i;

  i = strlen(filename)-strlen(fmtstr);
  if (i<0)
    return 0;
  filename += i;
  while (*filename) {
    if (tolower(*filename) != *fmtstr)
      return 0;
    filename++;
    fmtstr++;
  }
  return 1;
}


/* ---------- functions ---------- */


/* AudioInit() */
/* Init audio i/o module. */
/* formatString options: see AFsp documentation */

void AudioInit (
  char *formatString,		/* in: file format for headerless files */
  int debugLevel)		/* in: debug level */
				/*     0=off  1=basic  2=full */
{
  AUdebugLevel = debugLevel;
  if (AUdebugLevel >= 1) {
    printf("AudioInit: formatString=\"%s\"\n",
	   (formatString!=NULL)?formatString:"(null)");
    printf("AudioInit: debugLevel=%d\n",AUdebugLevel);
#ifdef USE_AFSP
    printf("AudioInit: all AFsp file formats supported\n");
#else
    printf("AudioInit: only 16 bit .au format supported\n");
#endif
  }
#ifdef USE_AFSP
  if (formatString!=NULL)
    AFsetNHpar(formatString);   /* headerless file support */
#endif
}


/* AudioOpenRead() */
/* Open audio file for reading. */

AudioFile *AudioOpenRead (
  char *fileName,		/* in: file name */
				/*     "-": stdin (only 16 bit .au) */
  int *numChannel,		/* out: number of channels */
  float *fSample,		/* out: sampling frequency [Hz] */
  long *numSample)		/* out: number of samples in file */
				/*      (samples per channel!) */
				/*      or 0 if not available */
				/* returns: */
				/*  audio file (handle) */
				/*  or NULL if error */
{
  AudioFile *file;
#ifdef USE_AFSP
  AFILE *af;
#else
  int *af;
#endif
  AuStream *as;
  long ns;
  long nc;
  int nci;
  float fs;

  if (AUdebugLevel >= 1)
    printf("AudioOpenRead: fileName=\"%s\"\n",fileName);

  if ((file=(AudioFile*)malloc(sizeof(AudioFile))) == NULL)
    CommonExit(1,"AudioOpenRead: memory allocation error");


#ifdef USE_AFSP
  if (strcmp(fileName,"-")) {
    af = AFopenRead(fileName,&ns,&nc,&fs,
		    AUdebugLevel?stdout:(FILE*)NULL);
    as = NULL;
  }
  else {
#endif
    af = NULL;
    as = AuOpenRead(fileName,&nci,&fs,&ns);
    nc = nci;
    ns = max(0,ns);
#ifdef USE_AFSP
  }
#endif

  if (as==NULL && af==NULL) {
    CommonWarning("AudioOpenRead: error opening audio file %s",fileName);
   FREE(file);
    return (AudioFile*)NULL;
  }

  file->file = af;
  file->stream = as;
  file->numChannel = nc;
  file->currentSample = 0;
  file->write = 0;
  file->numClip = 0;
  *numChannel = nc;
  *fSample = fs;
  *numSample = ns/nc;

  if (AUdebugLevel >= 1)
    printf("AudioOpenRead: numChannel=%d  fSample=%.1f  numSample=%ld\n",
	   *numChannel,*fSample,*numSample);

  return file;
}


/* AudioOpenWrite() */
/* Open audio file for writing. */
/* Sample format: 16 bit twos complement, uniform quantisation */
/* Supported file formats: (matching substring of format) */
/*  au, snd:  Sun (AFsp) audio file */
/*  wav:      RIFF WAVE file */
/*  aif:      AIFF-C audio file */
/*  raw:      headerless (raw) audio file (native byte order) */

AudioFile *AudioOpenWrite (
  char *fileName,		/* in: file name */
				/*     "-": stdout (only 16 bit .au) */
  char *format,			/* in: file format (ignored if stdout) */
				/*     (au, snd, wav, aif, raw) */
  int numChannel,		/* in: number of channels */
  float fSample)		/* in: sampling frequency [Hz] */
				/* returns: */
				/*  audio file (handle) */
				/*  or NULL if error */
{
  AudioFile *file;
#ifdef USE_AFSP
  AFILE *af;
  int fmt;
  int fmti;
  struct {
    char *str;
    int fmt;
  } fmtstr[] = {
    {"au",FTW_AU*256},
    {"snd",FTW_AU*256},
    {"wav",FTW_WAVE*256},
    {"wave",FTW_WAVE*256},
    {"aif",FTW_AIFF_C*256},
    {"aiff",FTW_AIFF_C*256},
    {"aifc",FTW_AIFF_C*256},
    {"raw",FTW_NH_EB*256},	/* no header big-endian */
    {NULL,-1}
  };
#else
  int *af;
  int fmti;
  struct {
    char *str;
    int fmt;
  } fmtstr[] = {
    {"au",1},
    {"snd",1},
    {NULL,-1}
  };
#endif
  AuStream *as;

  if (AUdebugLevel >= 1) {
    printf("AudioOpenWrite: fileName=\"%s\"  format=\"%s\"\n",fileName,format);
    printf("AudioOpenWrite: numChannel=%d  fSample=%.1f\n",
	   numChannel,fSample);
  }

  if (strcmp(fileName,"-")) {
#ifdef USE_AFSP
    fmti = 0;
    while (fmtstr[fmti].str && !isfmtstr(format,fmtstr[fmti].str))
      fmti++;
    if (fmtstr[fmti].str)
      fmt = FD_INT16 + fmtstr[fmti].fmt;
    else {
#else
    fmti = 0;
    while (fmtstr[fmti].str && !isfmtstr(format,fmtstr[fmti].str))
      fmti++;
    if (!fmtstr[fmti].str) {
#endif
      CommonWarning("AudioOpenWrite: unkown audio file format \"%s\"",
		    format);
      return (AudioFile*)NULL;
    }
  }

  if ((file=(AudioFile*)malloc(sizeof(AudioFile))) == NULL)
    CommonExit(1,"AudioOpenWrite: memory allocation error");

#ifdef USE_AFSP
  if (strcmp(fileName,"-")) {
    af = AFopenWrite(fileName,fmt,numChannel,fSample,
		     AUdebugLevel?stdout:(FILE*)NULL);
    as = NULL;
  }
  else {
#endif
    af = NULL;
    as = AuOpenWrite(fileName,numChannel,fSample);
#ifdef USE_AFSP
  }
#endif

  if (as==NULL && af==NULL) {
    CommonWarning("AudioOpenWrite: error opening audio file %s",fileName);
   FREE(file);
    return (AudioFile*)NULL;
  }

  file->file = af;
  file->stream = as;
  file->numChannel = numChannel;
  file->currentSample = 0;
  file->write = 1;
  file->numClip = 0;

  return file;
}


/* AudioReadData() */
/* Read data from audio file. */
/* Requested samples that could not be read from the file are set to 0. */

long AudioReadData (
  AudioFile *file,		/* in: audio file (handle) */
  float **data,			/* out: data[channel][sample] */
				/*      (range [-32768 .. 32767]) */
  long numSample)		/* in: number of samples to be read */
				/*     (samples per channel!) */
				/* returns: */
				/*  number of samples read */
{
  long tot,cur,num,tmp;
  long i;
  long numRead;
#ifdef USE_AFSP
  float buf[SAMPLE_BUF_SIZE];
#endif
  short bufs[SAMPLE_BUF_SIZE];

  if (AUdebugLevel >= 2)
    printf("AudioReadData: numSample=%ld (currentSample=%ld)\n",
	   numSample,file->currentSample);

  if (file->write != 0)
    CommonExit(1,"AudioReadData: audio file not in read mode");

  /* set initial unavailable samples to 0 */
  tot = file->numChannel*numSample;
  cur = 0;
  if (file->stream && file->currentSample < 0) {
    cur = min(-file->numChannel*file->currentSample,tot);
    for (i=0; i<cur; i++)
      data[i%file->numChannel][i/file->numChannel] = 0;
  }

  /* read samples from file */
  while (cur < tot) {
    num = min(tot-cur,SAMPLE_BUF_SIZE);
#ifdef USE_AFSP
    if (file->file) {
      tmp = AFreadData(file->file,
		       file->numChannel*file->currentSample+cur,buf,num);
      for (i=0; i<tmp; i++)
	data[(cur+i)%file->numChannel][(cur+i)/file->numChannel] = buf[i];
    }
#endif
    if (file->stream) {
      tmp = AuReadData(file->stream,bufs,num);
      for (i=0; i<tmp; i++)
	data[(cur+i)%file->numChannel][(cur+i)/file->numChannel] = bufs[i];
    }
    cur += tmp;
    if (tmp < num)
      break;
  }
  numRead = cur/file->numChannel;
  file->currentSample += numRead;

  /* set remaining unavailable samples to 0 */
  for (i=cur; i<tot; i++)
    data[i%file->numChannel][i/file->numChannel] = 0;

  return numRead;
}


/* AudioWriteData() */
/* Write data to audio file. */

void AudioWriteData (
  AudioFile *file,		/* in: audio file (handle) */
  float **data,			/* in: data[channel][sample] */
				/*     (range [-32768 .. 32767]) */
  long numSample)		/* in: number of samples to be written */
				/*     (samples per channel!) */
{
  long tot,cur,num;
  long i;
  long numClip,tmp;
#ifdef USE_AFSP
  float buf[SAMPLE_BUF_SIZE];
#endif
  short bufs[SAMPLE_BUF_SIZE];

  if (AUdebugLevel >= 2)
    printf("AudioWriteData: numSample=%ld (currentSample=%ld)\n",
	   numSample,file->currentSample);

  if (file->write != 1)
    CommonExit(1,"AudioWriteData: audio file not in write mode");

  tot = file->numChannel*numSample;
  cur = max(0,-file->numChannel*file->currentSample);
  while (cur < tot) {
    num = min(tot-cur,SAMPLE_BUF_SIZE);
#ifdef USE_AFSP
    if (file->file) {
      for (i=0; i<num; i++)
	buf[i] = data[(cur+i)%file->numChannel][(cur+i)/file->numChannel];
      AFwriteData(file->file,buf,num);
    }
#endif
    if (file->stream) {
      numClip = 0;
      for (i=0; i<num; i++) {
	tmp = ((long)(data[(cur+i)%file->numChannel]
		      [(cur+i)/file->numChannel]+32768.5))-32768;
	if (tmp>32767) {
	  tmp = 32767;
	  numClip++;
	}
	if (tmp<-32768) {
	    tmp = -32768;
	    numClip++;
	}
	bufs[i] = (short)tmp;
      }
      if (numClip && !file->numClip)
	CommonWarning("AudioWriteData: output samples clipped");
      file->numClip += numClip;
      AuWriteData(file->stream,bufs,num);
    }
    cur += num;
  }
  file->currentSample += tot/file->numChannel;
}


/* AudioSeek() */
/* Set position in audio file to curSample. */
/* (Beginning of file: curSample=0) */
/* NOTE: It is not possible to seek backwards in a output file if */
/*       any samples were already written to the file. */

void AudioSeek (
  AudioFile *file,		/* in: audio file (handle) */
  long curSample)		/* in: new position [samples] */
				/*     (samples per channel!) */
{
  long tot,cur,num,tmp;
#ifdef USE_AFSP
  float buf[SAMPLE_BUF_SIZE];
#endif
  short bufs[SAMPLE_BUF_SIZE];

  if (AUdebugLevel >= 1)
    printf("AudioSeek: curSample=%ld (currentSample=%ld)\n",
	   curSample,file->currentSample);

  if (file->write==0) {
    if (file->stream) {
      if (file->currentSample <= 0) {
	/* nothing read from stream yet */
	if (curSample <= 0)
	  file->currentSample = curSample;
	else
	  file->currentSample = 0;
      }
      if (curSample < file->currentSample)
	CommonWarning("AudioSeek: can not seek backward in input stream");
      else {
	/* read samples to skip */
	tot = file->numChannel*(curSample-file->currentSample);
	cur = 0;
	while (cur < tot) {
	  num = min(tot-cur,SAMPLE_BUF_SIZE);
	  tmp = AuReadData(file->stream,bufs,num);
	  cur += tmp;
	  if (tmp < num)
	    break;
	}
	file->currentSample = curSample;
      }
    }
    else
      file->currentSample = curSample;
  }
  else {
    if (file->currentSample <= 0) {
      /* nothing written to file yet */
      if (curSample <= 0)
	file->currentSample = curSample;
      else
	file->currentSample = 0;
    }
    if (curSample < file->currentSample)
      CommonExit(1,"AudioSeek: error seeking backwards in output file");
    if (curSample > file->currentSample) {
      /* seek forward, fill skipped region with silence */
#ifdef USE_AFSP
      memset(buf,0,SAMPLE_BUF_SIZE*sizeof(float));
#endif
      memset(bufs,0,SAMPLE_BUF_SIZE*sizeof(short));
      tot = file->numChannel*(curSample-file->currentSample);
      cur = 0;
      while (cur < tot) {
	num = min(tot-cur,SAMPLE_BUF_SIZE);
#ifdef USE_AFSP
	if (file->file)
	  AFwriteData(file->file,buf,num);
#endif
	if (file->stream)
	  AuWriteData(file->stream,bufs,num);
	cur += num;
      }
      file->currentSample = curSample;
    }
  }
}


/* AudioClose() */
/* Close audio file.*/

void AudioClose (
  AudioFile *file)		/* in: audio file (handle) */
{
  if (AUdebugLevel >= 1)
    printf("AudioClose: (currentSample=%ld)\n",file->currentSample);

  if (file->numClip)
    CommonWarning("AudioClose: %ld samples clipped",file->numClip);

#ifdef USE_AFSP
  if (file->file)
    AFclose(file->file);
#endif
  if (file->stream)
    AuClose(file->stream);
 FREE(file);
}


/* end of audio.c */


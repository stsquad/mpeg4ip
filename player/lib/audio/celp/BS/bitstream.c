/**********************************************************************
MPEG-4 Audio VM
Bit stream module



This software module was originally developed by

Heiko Purnhagen (University of Hannover / ACTS-MoMuSys)

and edited by

Ralph Sperschneider (Fraunhofer IIS)
Thomas Schaefer (Fraunhofer IIS)

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

Copyright (c) 1996, 1997, 1998.



Source file: bitstream.c

$Id: bitstream.c,v 1.2 2002/07/15 22:44:57 wmaycisco Exp $

Required modules:
common.o		common module

Authors:
HP    Heiko Purnhagen, Uni Hannover <purnhage@tnt.uni-hannover.de>
BT    Bodo Teichmann, FhG/IIS <tmn@iis.fhg.de>

Changes:
06-jun-96   HP    added buffers, ASCII-header and BsGetBufferAhead()
07-jun-96   HP    use CommonWarning(), CommonExit()
11-jun-96   HP    ...
13-jun-96   HP    changed BsGetBit(), BsPutBit(), BsGetBuffer(),
                  BsGetBufferAhead(), BsPutBuffer()
14-jun-96   HP    fixed bug in BsOpenFileRead(), read header
18-jun-96   HP    fixed bug in BsGetBuffer()
26-aug-96   HP    CVS
23-oct-96   HP    free for BsOpenFileRead() info string
07-nov-96   HP    fixed BsBitStream info bug
15-nov-96   HP    changed int to long where required
                  added BsGetBitChar(), BsGetBitShort(), BsGetBitInt()
                  improved file header handling
04-dec-96   HP    fixed bug in BsGetBitXxx() for numBit==0
23-dec-96   HP    renamed mode to write in BsBitStream
10-jan-97   HP    added BsGetBitAhead(), BsGetSkip()
17-jan-97   HP    fixed read file buffer bug
30-jan-97   HP    minor bug fix in read bitstream file magic string
19-feb-97   HP    made internal data structures invisible
04-apr-97   BT/HP added BsGetBufferAppend()
07-max-97   BT    added BsGetBitBack()
14-mrz-98   sfr   added CreateEpInfo(), BsReadInfoFile(), BsGetBitEP(),
                        BsGetBitEP()
20-jan-99   HP    due to the nature of some of the modifications merged
                  into this code, I disclaim any responsibility for this
                  code and/or its readability -- sorry ...
21-jan-99   HP    trying to clean up a bit ...
22-jan-99   HP    added "-" stdin/stdout support
                  variable file buffer size for streaming
05-feb-99   HP    added fflush() after fwrite()
12-feb-99   BT/HP updated ...
19-apr-99   HP    cleaning up some header files again :-(
**********************************************************************/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bitstream.h"		/* bit stream module */
#include "common_m4a.h"		/* common module */

#include "buffersHandle.h"      /* undesired, but ... */

/* ---------- declarations ---------- */

#include "bitstreamStruct.h"    /* structs */


#define MIN_FILE_BUF_SIZE 1024	/* min num bytes in file buffer */
#define HEADER_BUF_SIZE	  2048	/* max size of ASCII header */
#define COMMENT_LENGTH	  1024	/* max nr of comment characters */
				/* in predefinition file */

#ifndef min
#define min(a,b) ((a) < (b) ? (a) : (b))
#endif

#ifndef max
#define max(a,b) ((a) > (b) ? (a) : (b))
#endif

#define BYTE_NUMBIT 8		/* bits in byte (char) */
#define LONG_NUMBIT 32		/* bits in unsigned long */
#define bit2byte(a) (((a)+BYTE_NUMBIT-1)/BYTE_NUMBIT)
#define byte2bit(a) ((a)*BYTE_NUMBIT)


/* ---------- declarations (structures) ---------- */



/* ---------- variables ---------- */

static int  BSdebugLevel  = 0;			/* debug level */
static int  BSaacEOF	  = 0;
static long BSbufSizeByte = MIN_FILE_BUF_SIZE;	/* num bytes file buf */
static long BSstreamId	  = 0;			/* stream id counter */


/* ---------- internal functions ---------- */


/* BsReadFile() */
/* Read one buffer from file. */

static int BsReadFile (
  BsBitStream *stream)		/* in: stream */
				/* returns: 0=OK  1=error */
{
  long numByte;
  long numByteRead;
  long curBuf;

  if (BSdebugLevel >= 3)
    printf("BsReadFile: id=%ld  streamNumByte=%ld  curBit=%ld\n",
	   stream->streamId,stream->numByte,stream->currentBit);

  if (feof(stream->file))
    return 0;

  numByte = bit2byte(stream->buffer[0]->size);
  if (stream->numByte % numByte != 0) {
    CommonWarning("BsReadFile: bit stream buffer error");
    return 1;
  }
  curBuf = (stream->numByte / numByte) % 2;
  numByteRead = fread(stream->buffer[curBuf]->data,sizeof(char),numByte,
		      stream->file);
  if (ferror(stream->file)) {
    CommonWarning("BsReadFile: error reading bit stream file");
    return 1;
  }
  stream->numByte += numByteRead;

  if (BSdebugLevel >= 3)
    printf("BsReadFile: numByte=%ld  numByteRead=%ld\n",numByte,numByteRead);

  return 0;
}


/* BsWriteFile() */
/* Write one buffer to file. */

static int BsWriteFile (
  BsBitStream *stream)		/* in: stream */
				/* returns: 0=OK  1=error */
{
  long numByte;
  long numByteWritten;

  if (BSdebugLevel >= 3)
    printf("BsWriteFile: id=%ld  streamNumByte=%ld  curBit=%ld\n",
	   stream->streamId,stream->numByte,stream->currentBit);

  if (stream->numByte % bit2byte(stream->buffer[0]->size) != 0) {
    CommonWarning("BsWriteFile: bit stream buffer error");
    return 1;
  }
  numByte = bit2byte(stream->currentBit) - stream->numByte;
  numByteWritten = fwrite(stream->buffer[0]->data,sizeof(char),numByte,
			  stream->file);
  fflush(stream->file);
  if (numByteWritten != numByte || ferror(stream->file)) {
    CommonWarning("BsWriteFile: error writing bit stream file");
    return 1;
  }
  stream->numByte += numByteWritten;

  if (BSdebugLevel >= 3)
    printf("BsWriteFile: numByteWritten=%ld\n",numByteWritten);

  return 0;
}


/* BsReadAhead() */
/* If possible, ensure that the next numBit bits are available */
/* in read file buffer. */

static int BsReadAhead (
  BsBitStream *stream,		/* in: bit stream */
  long numBit)			/* in: number of bits */
				/* returns: 0=OK  1=error */
{
  if (stream->write==1 || stream->file==NULL)
    return 0;

  if (bit2byte(stream->currentBit+numBit) > stream->numByte)
    if (BsReadFile(stream)) {
      CommonWarning("BsReadAhead: error reading bit stream file");
      return 1;
    }
  return 0;
}


/* BsCheckRead() */
/* Check if numBit bits could be read from stream. */

static int BsCheckRead (
  BsBitStream *stream,		/* in: bit stream */
  long numBit)			/* in: number of bits */
				/* returns: */
				/*  0=numBit bits could be read */
				/*    (or write mode) */
				/*  1=numBit bits could not be read */
{
  if (stream->write == 1)
    return 0;		/* write mode */
  else
    if (stream->file == NULL)
      return (stream->currentBit+numBit > stream->buffer[0]->numBit) ? 1 : 0;
    else
      return (bit2byte(stream->currentBit+numBit) > stream->numByte) ? 1 : 0;
}


/* BsReadByte() */
/* Read byte from stream. */

static int BsReadByte (
  BsBitStream *stream,		/* in: stream */
  unsigned long *data,		/* out: data (max 8 bit, right justified) */
  int numBit)			/* in: num bits [1..8] */
				/* returns: num bits read or 0 if error */
{
  long numUsed;
  long idx;
  long buf;

  if (stream->file!=NULL && stream->currentBit==stream->numByte*BYTE_NUMBIT)
    if (BsReadFile(stream)) {
      if ( ! BSaacEOF || BSdebugLevel > 0 )
        CommonWarning("BsReadByte: error reading bit stream file");
      return 0;
    }

  if (BsCheckRead(stream,numBit)) {
    if ( ! BSaacEOF || BSdebugLevel > 0  )
      CommonWarning("BsReadByte: not enough bits left in stream");
    return 0;
  }

  idx = (stream->currentBit / BYTE_NUMBIT) % bit2byte(stream->buffer[0]->size);
  buf = (stream->currentBit / BYTE_NUMBIT /
	 bit2byte(stream->buffer[0]->size)) % 2;
  numUsed = stream->currentBit % BYTE_NUMBIT;
  *data = (stream->buffer[buf]->data[idx] >> (BYTE_NUMBIT-numUsed-numBit)) &
    ((1<<numBit)-1);
  stream->currentBit += numBit;
  return numBit;
}


/* BsWriteByte() */
/* Write byte to stream. */

static int BsWriteByte (
  BsBitStream *stream,		/* in: stream */
  unsigned long data,		/* in: data (max 8 bit, right justified) */
  int numBit)			/* in: num bits [1..8] */
				/* returns: 0=OK  1=error */
{
  long numUsed,idx;

  if (stream->file == NULL &&
      stream->buffer[0]->numBit+numBit > stream->buffer[0]->size) {
    CommonWarning("BsWriteByte: not enough bits left in buffer");
    return 1;
  }
  idx = (stream->currentBit / BYTE_NUMBIT) % bit2byte(stream->buffer[0]->size);
  numUsed = stream->currentBit % BYTE_NUMBIT;
  if (numUsed == 0)
    stream->buffer[0]->data[idx] = 0;
  stream->buffer[0]->data[idx] |= (data & ((1<<numBit)-1)) <<
    (BYTE_NUMBIT-numUsed-numBit);
  stream->currentBit += numBit;
  if (stream->file==NULL)
    stream->buffer[0]->numBit = stream->currentBit;
  if (stream->file!=NULL && stream->currentBit%stream->buffer[0]->size==0)
    if (BsWriteFile(stream)) {
      CommonWarning("BsWriteByte: error writing bit stream file");
      return 1;
    }
  return 0;
}


/* ---------- functions ---------- */


/* BsInit() */
/* Init bit stream module. */

void BsInit (
  long maxReadAhead,		/* in: max num bits to be read ahead */
				/*     (determines file buffer size) */
				/*     (0 = default) */
  int debugLevel,		/* in: debug level */
				/*     0=off  1=basic  2=medium	 3=full */
  int aacEOF)			/* in: AAC eof detection (default = 0) */
{
  if (maxReadAhead)
    BSbufSizeByte = max(4,bit2byte(maxReadAhead));
  else
    BSbufSizeByte = MIN_FILE_BUF_SIZE;
  BSdebugLevel = debugLevel;
  BSaacEOF = aacEOF;
  if (BSdebugLevel >= 1 )
    printf("BsInit: debugLevel=%d  aacEOF=%d  bufSizeByte=%ld\n",
	   BSdebugLevel,BSaacEOF,BSbufSizeByte);
}


/* BsOpenFileRead() */
/* Open bit stream file for reading. */

BsBitStream *BsOpenFileRead (
  char *fileName,		/* in: file name */
				/*     "-": stdin */
  char *magic,			/* in: magic string */
				/*     or NULL (no ASCII header in file) */
  char **info)			/* out: info string */
				/*      or NULL (no info string in file) */
				/* returns: */
				/*  bit stream (handle) */
				/*  or NULL if error */
{
  BsBitStream *stream;
  char header[HEADER_BUF_SIZE];
  int tmp = 0;
  long i,len;

  if (BSdebugLevel >= 1) {
    printf("BsOpenFileRead: fileName=\"%s\"  id=%ld  bufSize=%ld  ",
	   fileName,BSstreamId,byte2bit(BSbufSizeByte));
    if (magic != NULL)
      printf("magic=\"%s\"\n",magic);
    else
      printf("no header\n");
  }

  if ((stream=(BsBitStream*)malloc(sizeof(BsBitStream))) == NULL)
    CommonExit(1,"BsOpenFileRead: memory allocation error (stream)");
  stream->buffer[0]=BsAllocBuffer(byte2bit(BSbufSizeByte));
  stream->buffer[1]=BsAllocBuffer(byte2bit(BSbufSizeByte));

  stream->write = 0;
  stream->streamId = BSstreamId++;
  stream->info = NULL;
 
  if (strcmp(fileName,"-"))
    stream->file = fopen(fileName,"rb");
  else
    stream->file = stdin;
  if (stream->file == NULL) {
    CommonWarning("BsOpenFileRead: error opening bit stream file %s",
		  fileName);
    BsFreeBuffer(stream->buffer[0]);
    BsFreeBuffer(stream->buffer[1]);
   FREE(stream);
    return NULL;
  }

  if (magic != NULL) {
    /* read ASCII header */
    /* read magic string */
    len = strlen(magic);
    if (len >= HEADER_BUF_SIZE) {
      CommonWarning("BsOpenFileRead: magic string too long");
      BsClose(stream);
      return NULL;
    }
    for (i=0; i<len; i++)
      header[i] = tmp = fgetc(stream->file);
    if (tmp == EOF) {			/* EOF requires int (not char) */
      CommonWarning("BsOpenFileRead: "
		    "error reading bit stream file (header)");
      BsClose(stream);
      return NULL;
    }
    header[i] = '\0';

    if (strcmp(header,magic) != 0) {
      CommonWarning("BsOpenFileRead: magic string error "
		    "(found \"%s\", need \"%s\")",header,magic);
      BsClose(stream);
      return NULL;
    }
    
    if (info != NULL) {
      /* read info string */
      i = 0;
      while ((header[i]=tmp=fgetc(stream->file)) != '\0') {
	if (tmp == EOF) {		/* EOF requires int (not char) */
	  CommonWarning("BsOpenFileRead: "
			"error reading bit stream file (header)");
	  BsClose(stream);
	  return NULL;
	}
	i++;
	if (i >= HEADER_BUF_SIZE) {
	  CommonWarning("BsOpenFileRead: info string too long");
	  BsClose(stream);
	  return NULL;
	}
      }

      if (BSdebugLevel >= 1)
	printf("BsOpenFileRead: info=\"%s\"\n",header);

      if ((stream->info=(char*)malloc((strlen(header)+1)*sizeof(char)))
	  == NULL)
	CommonExit(1,"BsOpenFileRead: memory allocation error (info)");
      strcpy(stream->info,header);
      *info = stream->info;
    }
  }

  /* init buffer */
  stream->currentBit = 0;
  stream->numByte = 0;

  return stream;
}


/* BsOpenFileWrite() */
/* Open bit stream file for writing. */

BsBitStream *BsOpenFileWrite (
  char *fileName,		/* in: file name */
				/*     "-": stdout */
  char *magic,			/* in: magic string */
				/*     or NULL (no ASCII header) */
  char *info)			/* in: info string */
				/*     or NULL (no info string) */
				/* returns: */
				/*  bit stream (handle) */
				/*  or NULL if error */
{
  BsBitStream *stream;

  if (BSdebugLevel >= 1) {
    printf("BsOpenFileWrite: fileName=\"%s\"  id=%ld  bufSize=%ld  ",
	   fileName,BSstreamId,byte2bit(BSbufSizeByte));
    if (magic != NULL) {
      printf("magic=\"%s\"\n",magic);
      if (info != NULL)
	printf("BsOpenFileWrite: info=\"%s\"\n",info);
      else
	printf("BsOpenFileWrite: no info\n");
    }
    else
      printf("no header\n");
  }

  if ((stream=(BsBitStream*)malloc(sizeof(BsBitStream))) == NULL)
    CommonExit(1,"BsOpenFileWrite: memory allocation error (stream)");
  stream->buffer[0]=BsAllocBuffer(byte2bit(BSbufSizeByte));

  stream->write = 1;
  stream->streamId = BSstreamId++;
  stream->info = NULL;

  if (strcmp(fileName,"-"))
    stream->file = fopen(fileName,"wb");
  else
    stream->file = stdout;
  if (stream->file == NULL) {
    CommonWarning("BsOpenFileWrite: error opening bit stream file %s",
		  fileName);
    BsFreeBuffer(stream->buffer[0]);
   FREE(stream);
    return NULL;
  }

  if (magic!=NULL) {
    /* write ASCII header */
    /* write magic string */
    if (fputs(magic,stream->file) == EOF) {
      CommonWarning("BsOpenFileWrite: error writing bit stream file (header)");
      BsClose(stream);
      return NULL;
    }
    if (info!=NULL) {
      /* write info string */
      if (fputs(info,stream->file) == EOF) {
	CommonWarning("BsOpenFileWrite: "
		      "error writing bit stream file (header)");
	BsClose(stream);
	return NULL;
      }
      if (fputc('\0',stream->file) == EOF) {
	CommonWarning("BsOpenFileWrite: "
		      "error writing bit stream file (header)");
	BsClose(stream);
	return NULL;
      }
    }
  }

  stream->currentBit = 0;
  stream->numByte = 0;

  return stream;
}


/* BsOpenBufferRead() */
/* Open bit stream buffer for reading. */

BsBitStream *BsOpenBufferRead (
  BsBitBuffer *buffer)		/* in: bit buffer */
				/* returns: */
				/*  bit stream (handle) */
{
  BsBitStream *stream;

  if (BSdebugLevel >= 2)
    printf("BsOpenBufferRead: id=%ld  bufNumBit=%ld  bufSize=%ld  "
	   "bufAddr=0x%lx\n",
	   BSstreamId,buffer->numBit,buffer->size,(unsigned long)buffer);

  if ((stream=(BsBitStream*)malloc(sizeof(BsBitStream))) == NULL)
    CommonExit(1,"BsOpenBufferRead: memory allocation error");

  stream->file = NULL;
  stream->write = 0;
  stream->streamId = BSstreamId++;
  stream->info = NULL;

  stream->buffer[0] = buffer;
  stream->currentBit = 0;
  
  return stream;
}


/* BsOpenBufferWrite() */
/* Open bit stream buffer for writing. */
/* (Buffer is cleared first - previous data in buffer is lost !!!) */

BsBitStream *BsOpenBufferWrite (
  BsBitBuffer *buffer)		/* in: bit buffer */
				/* returns: */
				/*  bit stream (handle) */
{
  BsBitStream *stream;

  if (BSdebugLevel >= 2)
    printf("BsOpenBufferWrite: id=%ld  bufNumBit=%ld  bufSize=%ld  "
	   "bufAddr=0x%lx\n",
	   BSstreamId,buffer->numBit,buffer->size,(unsigned long)buffer);

  if ((stream=(BsBitStream*)malloc(sizeof(BsBitStream))) == NULL)
    CommonExit(1,"BsOpenBufferWrite: memory allocation error");

  stream->file = NULL;
  stream->write = 1;
  stream->streamId = BSstreamId++;
  stream->info = NULL;

  stream->buffer[0] = buffer;
  BsClearBuffer(buffer);
  stream->currentBit = 0;
  
  return stream;
}


/* BsCurrentBit() */
/* Get number of bits read/written from/to stream. */

long BsCurrentBit (
  BsBitStream *stream)		/* in: bit stream */
				/* returns: */
				/*  number of bits read/written */
{
  return stream->currentBit;
}


/* BsEof() */
/* Test if end of bit stream file occurs within the next numBit bits. */

int BsEof (
  BsBitStream *stream,		/* in: bit stream */
  long numBit)			/* in: number of bits ahead scanned for EOF */
				/* returns: */
				/*  0=not EOF  1=EOF */
{
  int eof;

  if (BSdebugLevel >= 2)
    printf("BsEof: %s  id=%ld  curBit=%ld  numBit=%ld\n",
	   (stream->file!=NULL)?"file":"buffer",
	   stream->streamId,stream->currentBit,numBit);

  if (stream->file != NULL && numBit > stream->buffer[0]->size)
    CommonExit(1,"BsEof: "
	       "number of bits to look ahead too high (%ld)",numBit);

  if (BsReadAhead(stream,numBit+1)) {
    CommonWarning("BsEof: error reading bit stream");
    eof = 0;
  }
  else
    eof = BsCheckRead(stream,numBit+1);

  if (BSdebugLevel >= 2)
    printf("BsEof: eof=%d\n",eof);

  return eof;
}


static void BsRemoveBufferOffset (BsBitBuffer *buffer, long startPosBit)
{
  int           bitsToCopy;
  BsBitStream*  offset_stream;
  BsBitBuffer*  helpBuffer;

  /* open bit stream buffer for reading */
  offset_stream = BsOpenBufferRead(buffer);

  /* create help buffer */
  helpBuffer = BsAllocBuffer(buffer->size);
  

  /* read bits from bit stream to help buffer */
  offset_stream->currentBit =  startPosBit;
  bitsToCopy = buffer->numBit - startPosBit;
  if (BsGetBuffer(offset_stream, helpBuffer, bitsToCopy))
    CommonExit(1, "BsRemoveBufferOffset: error reading bit stream");

  /* close bit stream (no remove) */
  BsCloseRemove(offset_stream, 0);

  /* memcpy the offset free data from help buffer to buffer */
  memcpy(buffer->data, helpBuffer->data, bit2byte(buffer->size));

  /*FREE help buffer */
  BsFreeBuffer(helpBuffer);

  buffer->numBit = bitsToCopy;
}


/* BsCloseRemove() */
/* Close bit stream. */

int BsCloseRemove (
  BsBitStream *stream,		/* in: bit stream */
  int remove)			/* in: if opened with BsOpenBufferRead(): */
				/*       0 = keep buffer unchanged */
				/*       1 = remove bits read from buffer */
				/* returns: */
				/*  0=OK  1=error */
{
  int returnFlag = 0;
  int tmp,i;
  long startPosBit;

  if ((stream->file != NULL) && (BSdebugLevel >= 1) )
    printf("BsClose: %s  %s  id=%ld  curBit=%ld\n",
	   (stream->write)?"write":"read",
	   (stream->file!=NULL)?"file":"buffer",
	   stream->streamId,stream->currentBit);

  if (stream->file != NULL) {
    if (stream->write == 1)
      /* flush buffer to file */
      if (BsWriteFile(stream)) {
	CommonWarning("BsClose: error writing bit stream");
	returnFlag = 1;
      }

    BsFreeBuffer(stream->buffer[0]);
    if (stream->write == 0)
      BsFreeBuffer(stream->buffer[1]);
    
    if (stream->file!=stdin && stream->file!=stdout)
      if (fclose(stream->file)) {
	CommonWarning("BsClose: error closing bit stream file");
	returnFlag = 1;
      }
  }
  else if (stream->write == 0 && remove){    
    /* remove all completely read bytes from buffer */
    tmp = stream->currentBit/8;
    for (i=0; i<((stream->buffer[0]->size+7)>>3)-tmp; i++)
      stream->buffer[0]->data[i] = stream->buffer[0]->data[i+tmp];
    startPosBit = stream->currentBit - tmp*8;
    if ((startPosBit>7) || (startPosBit<0)){
      CommonExit(1,"BsClose: Error removing bit in buffer");        
    }
    stream->buffer[0]->numBit -= tmp*8;      

    /* better located here ???   HP/BT 990520 */
    if (stream->buffer[0]->numBit <= startPosBit) {
      stream->buffer[0]->numBit=0;
      startPosBit=0;
    }

    /* remove remaining read bits from buffer          */
    /* usually we do not have remaining bits in buffer 
     reply: that not really true, eg. HVXC frames are not byte aligned, thereofore you have remaining bits after every second frame, 
    BT*/
    if (startPosBit != 0)
      {
      /* printf("Remove buffer offset: %li\n", startPosBit); */
      BsRemoveBufferOffset(stream->buffer[0], startPosBit);
      if ((stream->currentBit - startPosBit) < 0)
        CommonExit(1,"BsClose: Error decreasing currentBit");
      else
        stream->currentBit -= startPosBit;
      }
  }

  if (stream->info != NULL)
   FREE(stream->info);
 FREE(stream);
  return returnFlag;
}


/* BsClose() */
/* Close bit stream. */

int BsClose (
  BsBitStream *stream)		/* in: bit stream */
				/* returns: */
				/*  0=OK  1=error */
{
 return BsCloseRemove(stream,0);
}


/* BsGetBit() */
/* Read bits from bit stream. */
/* (Current position in bit stream is advanced !!!) */

int BsGetBit (
  BsBitStream *stream,		/* in: bit stream */
  unsigned long *data,		/* out: bits read */
				/*      (may be NULL if numBit==0) */
  int numBit)			/* in: num bits to read */
				/*     [0..32] */
				/* returns: */
				/*  0=OK  1=error */
{
  int		num;
  int		maxNum;
  int		curNum;
  unsigned long bits;

  if (BSdebugLevel >= 3)
    printf("BsGetBit: %s  id=%ld  numBit=%d  curBit=%ld\n",
	   (stream->file!=NULL)?"file":"buffer",
	   stream->streamId,numBit,stream->currentBit);

  if (stream->write != 0)
    CommonExit(1,"BsGetBit: stream not in read mode");
  if (numBit<0 || numBit>LONG_NUMBIT)
    CommonExit(1,"BsGetBit: number of bits out of range (%d)",numBit);

  if (data != NULL)
    *data = 0;
  if (numBit == 0)
    return 0;

  /* read bits in packets according to buffer byte boundaries */
  num = 0;
  maxNum = BYTE_NUMBIT - stream->currentBit % BYTE_NUMBIT;
  while (num < numBit) {
    curNum = min(numBit-num,maxNum);
    if (BsReadByte(stream,&bits,curNum) != curNum) {
      if ( ! BSaacEOF || BSdebugLevel > 0  ) 
        CommonWarning("BsGetBit: error reading bit stream");

      if ( BSaacEOF  ) {        
        return -1;/* end of stream */
      } else {
        return 1; 
      }
    }
    *data |= bits<<(numBit-num-curNum);
    num += curNum;
    maxNum = BYTE_NUMBIT;
  }

  if (BSdebugLevel >= 3)
    printf("BsGetBit: data=0x%lx\n",*data);

  return 0;
}


/* this function is mainly for debugging purpose, */
/* you can call it from the debugger */
long int BsGetBitBack (
  BsBitStream *stream,		/* in: bit stream */
  int numBit)			/* in: num bits to read */
				/*     [0..32] */
				/* returns: */
				/*  if numBit is positive
				      return the last numBits bit from stream
				    else
				      return the next -numBits from stream 
				    stream->currentBit is always unchanged */
{
  int		num;
  int		maxNum;
  int		curNum;
  unsigned long bits;
  long int	data;
  int		rewind = 0;

  if (BSdebugLevel >= 3)
    printf("BsGetBitBack: %s  id=%ld  numBit=%d  curBit=%ld\n",
	   (stream->file!=NULL)?"file":"buffer",
	   stream->streamId,numBit,stream->currentBit);

  /*   if (stream->write != 0) */
  /*	 CommonWarning("BsGetBitBack: stream not in read mode"); */
  if (numBit<-32 || numBit>LONG_NUMBIT)
    CommonExit(1,"BsGetBitBack: number of bits out of range (%d)",numBit);

  data = 0;
  if (numBit == 0)
    return 0;
  if (numBit > 0)
    stream->currentBit-=numBit;
  else {
    rewind = 1;
    numBit = -numBit;
  }
    
  if (stream->currentBit<0){
    stream->currentBit+=numBit;
    CommonWarning("BsGetBitBack: stream enough bits in stream ");
    return 0;
  }

  /* read bits in packets according to buffer byte boundaries */
  num = 0;
  maxNum = BYTE_NUMBIT - stream->currentBit % BYTE_NUMBIT;
  while (num < numBit) {
    curNum = min(numBit-num,maxNum);
    if (BsReadByte(stream,&bits,curNum) != curNum) {
      CommonWarning("BsGetBitBack: error reading bit stream");
      return 0;
    }
    data |= bits<<(numBit-num-curNum);
    num += curNum;
    maxNum = BYTE_NUMBIT;
  }
  if (rewind) /* rewind */
    stream->currentBit-=numBit;

  if (BSdebugLevel >= 3)
    printf("BsGetBitBack: data=0x%lx\n",data);

  return data;
}


/* BsGetBitChar() */
/* Read bits from bit stream (char). */
/* (Current position in bit stream is advanced !!!) */

int BsGetBitChar (
  BsBitStream *stream,		/* in: bit stream */
  unsigned char *data,		/* out: bits read */
				/*      (may be NULL if numBit==0) */
  int numBit)			/* in: num bits to read */
				/*     [0..8] */
				/* returns: */
				/*  0=OK  1=error */
{
  unsigned long ultmp;
  int result;

  if (numBit > 8)
    CommonExit(1,"BsGetBitChar: number of bits out of range (%d)",numBit);
  if (data != NULL)
    *data = 0;
  if (numBit == 0)
    return 0;
  result = BsGetBit(stream,&ultmp,numBit);
  *data = ultmp;
  return result;
}


/* BsGetBitShort() */
/* Read bits from bit stream (short). */
/* (Current position in bit stream is advanced !!!) */

int BsGetBitShort (
  BsBitStream *stream,		/* in: bit stream */
  unsigned short *data,		/* out: bits read */
				/*      (may be NULL if numBit==0) */
  int numBit)			/* in: num bits to read */
				/*     [0..16] */
				/* returns: */
				/*  0=OK  1=error */
{
  unsigned long ultmp;
  int result;

  if (numBit > 16)
    CommonExit(1,"BsGetBitShort: number of bits out of range (%d)",numBit);
  if (data != NULL)
    *data = 0;
  if (numBit == 0)
    return 0;
  result = BsGetBit(stream,&ultmp,numBit);
  *data = ultmp;
  return result;
}


/* BsGetBitInt() */
/* Read bits from bit stream (int). */
/* (Current position in bit stream is advanced !!!) */

int BsGetBitInt (
  BsBitStream *stream,		/* in: bit stream */
  unsigned int *data,		/* out: bits read */
				/*      (may be NULL if numBit==0) */
  int numBit)			/* in: num bits to read */
				/*     [0..16] */
				/* returns: */
				/*  0=OK  1=error */
{
  unsigned long ultmp;
  int result;

  if (numBit > 16)
    CommonExit(1,"BsGetBitInt: number of bits out of range (%d)",numBit);
  if (data != NULL)
    *data = 0;
  if (numBit == 0)
    return 0;
  result = BsGetBit(stream,&ultmp,numBit);
  *data = ultmp;
  return result;
}


/* BsGetBitAhead() */
/* Read bits from bit stream. */
/* (Current position in bit stream is NOT advanced !!!) */

int BsGetBitAhead (
  BsBitStream *stream,		/* in: bit stream */
  unsigned long *data,		/* out: bits read */
				/*      (may be NULL if numBit==0) */
  int numBit)			/* in: num bits to read */
				/*     [0..32] */
				/* returns: */
				/*  0=OK  1=error */
{
  long oldCurrentBit;
  int  result;

  if (BSdebugLevel >= 3)
    printf("BsGetBitAhead: %s  id=%ld  numBit=%d\n",
	   (stream->file!=NULL)?"file":"buffer",stream->streamId,numBit);

  oldCurrentBit = stream->currentBit;
  result = BsGetBit(stream,data,numBit);
  stream->currentBit = oldCurrentBit;
  if (result)
    CommonWarning("BsGetBitAhead: error reading bit stream");

  return result;
}


/* BsGetBitAheadChar() */
/* Read bits from bit stream (char). */
/* (Current position in bit stream is NOT advanced !!!) */

int BsGetBitAheadChar (
  BsBitStream *stream,		/* in: bit stream */
  unsigned char *data,		/* out: bits read */
				/*      (may be NULL if numBit==0) */
  int numBit)			/* in: num bits to read */
				/*     [0..8] */
				/* returns: */
				/*  0=OK  1=error */
{
  long oldCurrentBit;
  int result;

  if (BSdebugLevel >= 3)
    printf("BsGetBitAheadChar: %s  id=%ld  numBit=%d\n",
	   (stream->file!=NULL)?"file":"buffer",stream->streamId,numBit);

  oldCurrentBit = stream->currentBit;
  result = BsGetBitChar(stream,data,numBit);
  stream->currentBit = oldCurrentBit;
  if (result)
    CommonWarning("BsGetBitAheadChar: error reading bit stream");

  return result;
}


/* BsGetBitAheadShort() */
/* Read bits from bit stream (short). */
/* (Current position in bit stream is NOT advanced !!!) */

int BsGetBitAheadShort (
  BsBitStream *stream,		/* in: bit stream */
  unsigned short *data,		/* out: bits read */
				/*      (may be NULL if numBit==0) */
  int numBit)			/* in: num bits to read */
				/*     [0..16] */
				/* returns: */
				/*  0=OK  1=error */
{
  long oldCurrentBit;
  int result;

  if (BSdebugLevel >= 3)
    printf("BsGetBitAheadShort: %s  id=%ld  numBit=%d\n",
	   (stream->file!=NULL)?"file":"buffer",stream->streamId,numBit);

  oldCurrentBit = stream->currentBit;
  result = BsGetBitShort(stream,data,numBit);
  stream->currentBit = oldCurrentBit;
  if (result)
    CommonWarning("BsGetBitAheadShort: error reading bit stream");

  return result;
}


/* BsGetBitAheadInt() */
/* Read bits from bit stream (int). */
/* (Current position in bit stream is NOT advanced !!!) */

int BsGetBitAheadInt (
  BsBitStream *stream,		/* in: bit stream */
  unsigned int *data,		/* out: bits read */
				/*      (may be NULL if numBit==0) */
  int numBit)			/* in: num bits to read */
				/*     [0..16] */
				/* returns: */
				/*  0=OK  1=error */
{
  long oldCurrentBit;
  int result;

  if (BSdebugLevel >= 3)
    printf("BsGetBitAheadInt: %s  id=%ld  numBit=%d\n",
	   (stream->file!=NULL)?"file":"buffer",stream->streamId,numBit);

  oldCurrentBit = stream->currentBit;
  result = BsGetBitInt(stream,data,numBit);
  stream->currentBit = oldCurrentBit;
  if (result)
    CommonWarning("BsGetBitAheadInt: error reading bit stream");

  return result;
}

long BsGetBufferFullness (BsBitBuffer *buffer)
{
  return (buffer->numBit);
}

/* BsGetBuffer() */
/* Read bits from bit stream to buffer. */
/* (Current position in bit stream is advanced !!!) */
/* (Buffer is cleared first - previous data in buffer is lost !!!) */

int BsGetBuffer (
  BsBitStream *stream,		/* in: bit stream */
  BsBitBuffer *buffer,		/* out: buffer read */
				/*      (may be NULL if numBit==0) */
  long numBit)			/* in: num bits to read */
				/* returns: */
				/*  0=OK  1=error */
{
  long i,numByte,numRemain;
  unsigned long data;

  if (BSdebugLevel >= 2) {
    printf("BsGetBuffer: %s  id=%ld  numBit=%ld  ",
	   (stream->file!=NULL)?"file":"buffer",
	   stream->streamId,numBit);
      if (buffer != NULL)
	printf("bufSize=%ld  bufAddr=0x%lx  ",
	       buffer->size,(unsigned long)buffer);
      else
	printf("(bufAddr=(NULL)  ");
    printf("curBit=%ld\n",stream->currentBit);
  }

  if (stream->write != 0)
    CommonExit(1,"BsGetBuffer: stream not in read mode");

  if (numBit == 0)
    return 0;

  if (stream->buffer[0] == buffer)
    CommonExit(1,"BsGetBuffer: can not get buffer from itself");
  if (numBit < 0 || numBit > buffer->size)
    CommonExit(1,"BsGetBuffer: number of bits out of range (%ld)",numBit);

  BsClearBuffer(buffer);

  numByte = bit2byte(numBit)-1;
  for (i=0; i<numByte; i++) {
    if (BsGetBit(stream,&data,BYTE_NUMBIT)) {
      if ( ! BSaacEOF || BSdebugLevel > 0  )
        CommonWarning("BsGetBuffer: error reading bit stream");
      buffer->numBit = i*BYTE_NUMBIT;
      return 1;
    }
    buffer->data[i] = data;
  }
  numRemain = numBit-numByte*BYTE_NUMBIT;
  if (BsGetBit(stream,&data,numRemain)) {
    if ( ! BSaacEOF || BSdebugLevel > 0  )
      CommonWarning("BsGetBuffer: error reading bit stream");
    buffer->numBit = numByte*BYTE_NUMBIT;
    return 1;
  }
  buffer->data[i] = data<<(BYTE_NUMBIT-numRemain);
  buffer->numBit = numBit;

  return 0;
}


/* BsGetBufferAppend() */
/* Append bits from bit stream to buffer. */
/* (Current position in bit stream is advanced !!!) */

int BsGetBufferAppend (
  BsBitStream *stream,		/* in: bit stream */
  BsBitBuffer *buffer,		/* out: buffer read */
				/*      (may be NULL if numBit==0) */
  int append,			/* in: if != 0: append bits to buffer */
				/*              (don't clear buffer) */
  long numBit)			/* in: num bits to read */
				/* returns: */
				/*  0=OK  1=error */
{
  long i,numByte,last_byte,numRemain;
  unsigned long data;
  int tmp,shift_cnt,eof;

  if (BSdebugLevel >= 2) {
    printf("BsGetBufferAppend: %s  id=%ld  numBit=%ld  ",
	   (stream->file!=NULL)?"file":"buffer",
	   stream->streamId,numBit);
    if (buffer != NULL)
      printf("bufSize=%ld  bufAddr=0x%lx  ",
	     buffer->size,(unsigned long)buffer);
    else
      printf("(bufAddr=(NULL)  ");
    printf("curBit=%ld\n",stream->currentBit);
  }

  if (stream->write != 0)
    CommonExit(1,"BsGetBufferAppend: stream not in read mode");

  if (stream->buffer[0] == buffer)
    CommonExit(1,"BsGetBufferAppend: cannot get buffer from itself");

  if (numBit < 0)
    CommonExit(1,"BsGetBufferAppend: number of bits out of range (%ld)",
	       numBit);

#if 1 	/* AI 990528 */    
  /* check whether number of bits to be read exceeds the end of bitstream */
  eof = BsEof(stream, numBit);
/*  if (BsEof(stream, numBit-1)) { */
  if (eof) {
    numBit = BYTE_NUMBIT*stream->numByte - stream->currentBit;
    if (BSdebugLevel >= 2) {
      printf("*** numBits(modified)=%ld\n", numBit);
    }
  }
#endif

  if (append) {

    /* append to buffer (don't clear buffer) */

    if ((numBit+buffer->numBit) > buffer->size)
      CommonExit(1,"BsGetBufferAppend: number of bits out of range (%ld)",
		 numBit);

    /* fill up the last possible incomplete byte */
    tmp = 8 - buffer->numBit%8;
    if (tmp == 8)
      tmp = 0;

    if (tmp <= numBit) {
      shift_cnt = 0;
    }
    else {
      shift_cnt = tmp - numBit;
      tmp = numBit;
    }

    if (tmp) {
      if (BsGetBit(stream,&data,tmp)) {
	CommonWarning("BsGetBufferAppend: error reading bit stream");
	return 1;
      }
      data <<= shift_cnt;
      numBit -= tmp;
      last_byte = buffer->numBit/8;
      data |= buffer->data[last_byte];
      buffer->numBit += tmp;
      buffer->data[last_byte] = data;
      last_byte++;
    }
    else
      last_byte = buffer->numBit/8;

  }
  else { /* if (append) */
    /* clear buffer */
    if (numBit > buffer->size)
      CommonExit(1,"BsGetBufferAppend: number of bits out of range (%ld)",
                 numBit);
    BsClearBuffer(buffer);
    last_byte = 0;
  }

  if (numBit > 0) {
    numByte = bit2byte(numBit)-1;
    for (i=last_byte; i<last_byte+numByte; i++) {
      if ((tmp = BsGetBit(stream,&data,BYTE_NUMBIT))) {
	buffer->numBit += (i-last_byte)*BYTE_NUMBIT;
        if (tmp==-1)
          return -1;	/* end of file */
	CommonWarning("BsGetBufferAppend: error reading bit stream");
	return 1;
      }
      buffer->data[i] = data;
    }
    numRemain = numBit-numByte*BYTE_NUMBIT;
    if (BsGetBit(stream,&data,numRemain)) {
      CommonWarning("BsGetBufferAppend: error reading bit stream");
      buffer->numBit += numByte*BYTE_NUMBIT;
      return 1;
    }
    buffer->data[i] = data<<(BYTE_NUMBIT-numRemain);
    buffer->numBit += numBit;
  }
#if 1	/* AI 990528 */
  if ( eof ) {
    /* just reached to the end of bitstream */
    if (stream->currentBit == BYTE_NUMBIT*stream->numByte) {
      if (BSdebugLevel >= 2) {
	printf("*** just reached the end of bitstream\n");
      }
      return -1;	/* end of file */
    }
  }
#endif
/*  } */
  return 0;
}

/* BsGetBufferAhead() */
/* Read bits ahead of current position from bit stream to buffer. */
/* (Current position in bit stream is NOT advanced !!!) */
/* (Buffer is cleared first - previous data in buffer is lost !!!) */

int BsGetBufferAhead (
  BsBitStream *stream,		/* in: bit stream */
  BsBitBuffer *buffer,		/* out: buffer read */
				/*      (may be NULL if numBit==0) */
  long numBit)			/* in: num bits to read */
				/* returns: */
				/*  0=OK  1=error */
{
  long oldCurrentBit;
  int result;

  if (BSdebugLevel >= 2)
    printf("BsGetBufferAhead: %s  id=%ld  numBit=%ld\n",
	   (stream->file!=NULL)?"file":"buffer",stream->streamId,numBit);

  if (numBit > stream->buffer[0]->size)
    CommonExit(1,"BsGetBufferAhead: "
	       "number of bits to look ahead too high (%ld)",numBit);

  oldCurrentBit = stream->currentBit;
  result = BsGetBuffer(stream,buffer,numBit);
  stream->currentBit = oldCurrentBit;
  if (result)
    if ( ! BSaacEOF || BSdebugLevel > 0  )
      CommonWarning("BsGetBufferAhead: error reading bit stream");

  return result;
}


/* BsGetSkip() */
/* Skip bits in bit stream (read). */
/* (Current position in bit stream is advanced !!!) */

int BsGetSkip (
  BsBitStream *stream,		/* in: bit stream */
  long numBit)			/* in: num bits to skip */
				/* returns: */
				/*  0=OK  1=error */
{
  if (BSdebugLevel >= 2) {
    printf("BsGetSkip: %s  id=%ld  numBit=%ld  ",
	   (stream->file!=NULL)?"file":"buffer",
	   stream->streamId,numBit);
    printf("curBit=%ld\n",stream->currentBit);
  }

  if (stream->write != 0)
    CommonExit(1,"BsGetSkip: stream not in read mode");
  if (numBit < 0)
    CommonExit(1,"BsGetSkip: number of bits out of range (%ld)",numBit);

  if (numBit == 0)
    return 0;

  if (BsReadAhead(stream,numBit) || BsCheckRead(stream,numBit)) {
    CommonWarning("BsGetSkip: error reading bit stream");
    return 1;
  }
  stream->currentBit += numBit;
  return 0;
}


/* BsPutBit() */
/* Write bits to bit stream. */

int BsPutBit (
  BsBitStream *stream,		/* in: bit stream */
  unsigned long data,		/* in: bits to write */
  int numBit)			/* in: num bits to write */
				/*     [0..32] */
				/* returns: */
				/*  0=OK  1=error */
{
  int num,maxNum,curNum;
  unsigned long bits;

  if (BSdebugLevel > 3)
    printf("BsPutBit: %s  id=%ld  numBit=%d  data=0x%lx,%ld  curBit=%ld\n",
	   (stream->file!=NULL)?"file":"buffer",
	   stream->streamId,numBit,data,data,stream->currentBit);

  if (stream->write != 1)
    CommonExit(1,"BsPutBit: stream not in write mode");
  if (numBit<0 || numBit>LONG_NUMBIT)
    CommonExit(1,"BsPutBit: number of bits out of range (%d)",numBit);
  if (numBit < 32 && data > ((unsigned long)1<<numBit)-1)
    CommonExit(1,"BsPutBit: data requires more than %d bits (0x%lx)",
	       numBit,data);

  if (numBit == 0)
    return 0;

  /* write bits in packets according to buffer byte boundaries */
  num = 0;
  maxNum = BYTE_NUMBIT - stream->currentBit % BYTE_NUMBIT;
  while (num < numBit) {
    curNum = min(numBit-num,maxNum);
    bits = data>>(numBit-num-curNum);
    if (BsWriteByte(stream,bits,curNum)) {
      CommonWarning("BsPutBit: error writing bit stream");
      return 1;
    }
    num += curNum;
    maxNum = BYTE_NUMBIT;
  }

  return 0;
}


/* BsPutBuffer() */
/* Write bits from buffer to bit stream. */

int BsPutBuffer (
  BsBitStream *stream,		/* in: bit stream */
  BsBitBuffer *buffer)		/* in: buffer to write */
				/* returns: */
				/*  0=OK  1=error */
{
  long i,numByte,numRemain;

  if (buffer->numBit == 0)
    return 0;

  if (BSdebugLevel >= 2)
    printf("BsPutBuffer: %s  id=%ld  numBit=%ld  bufAddr=0x%lx  curBit=%ld\n",
	   (stream->file!=NULL)?"file":"buffer",
	   stream->streamId,buffer->numBit,(unsigned long)buffer,
	   stream->currentBit);

  if (stream->write != 1)
    CommonExit(1,"BsPutBuffer: stream not in write mode");
  if (stream->buffer[0] == buffer)
    CommonExit(1,"BsPutBuffer: can not put buffer into itself");

  numByte = bit2byte(buffer->numBit)-1;
  for (i=0; i<numByte; i++)
    if (BsPutBit(stream,buffer->data[i],BYTE_NUMBIT)) {
      CommonWarning("BsPutBuffer: error writing bit stream");
      return 1;
    }
  numRemain = buffer->numBit-numByte*BYTE_NUMBIT;
  if (BsPutBit(stream,buffer->data[i]>>(BYTE_NUMBIT-numRemain),numRemain)) {
    CommonWarning("BsPutBuffer: error reading bit stream");
    return 1;
  }

  return 0;
}


/* BsAllocBuffer() */
/* Allocate bit buffer. */

BsBitBuffer *BsAllocBuffer (
  long numBit)			/* in: buffer size in bits */
				/* returns: */
				/*  bit buffer (handle) */
{
  BsBitBuffer *buffer;

  if (BSdebugLevel >= 2)
    printf("BsAllocBuffer: size=%ld\n",numBit);

  if ((buffer=(BsBitBuffer*)malloc(sizeof(BsBitBuffer))) == NULL)
    CommonExit(1,"BsAllocBuffer: memory allocation error (buffer)");
  if ((buffer->data=(unsigned char*)malloc(bit2byte(numBit)*sizeof(char)))
      == NULL)
    CommonExit(1,"BsAllocBuffer: memory allocation error (data)");
  buffer->numBit = 0;
  buffer->size = numBit;

  if (BSdebugLevel >= 2)
    printf("BsAllocBuffer: bufAddr=0x%lx\n",(unsigned long)buffer);

  return buffer;
}


/* BsFreeBuffer() */
/*FREE bit buffer. */

void BsFreeBuffer (
  BsBitBuffer *buffer)		/* in: bit buffer */
{
  if (BSdebugLevel >= 2)
    printf("BsFreeBuffer: size=%ld  bufAddr=0x%lx\n",
	   buffer->size,(unsigned long)buffer);

 FREE(buffer->data);
 FREE(buffer);
}


/* BsBufferNumBit() */
/* Get number of bits in buffer. */

long BsBufferNumBit (
  BsBitBuffer *buffer)		/* in: bit buffer */
				/* returns: */
				/*  number of bits in buffer */
{
  if (BSdebugLevel >= 2)
    printf("BsBufferNumBit: numBit=%ld  size=%ld  bufAddr=0x%lx\n",
	   buffer->numBit,buffer->size,(unsigned long)buffer);

  return buffer->numBit ;
}


/* BsClearBuffer() */
/* Clear bit buffer (set numBit to 0). */

void BsClearBuffer (
  BsBitBuffer *buffer)		/* in: bit buffer */
{
  if (BSdebugLevel >= 2)
    printf("BsClearBuffer: size=%ld  bufAddr=0x%lx\n",
	   buffer->size,(unsigned long)buffer);

  if (buffer->numBit != 0 ) {    
#if 0 /* removed due to BUG! in mp4dec.c */
    CommonWarning("BsClearBuffer: Buffer not empty!");    
#endif /* removed due to BUG! in mp4dec.c */
  }
  buffer->numBit = 0;
}




/* end of bitstream.c */


/**********************************************************************
MPEG-4 Audio VM
Bit stream module

This software module was originally developed by

Heiko Purnhagen (University of Hannover / ACTS-MoMuSys)

and edited by

Ralph Sperschneider (Fraunhofer Gesellschaft IIS)

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



Authors:
HP    Heiko Purnhagen, Uni Hannover <purnhage@tnt.uni-hannover.de>

Changes:
19-apr-99   HP    cleaning up some header files again :-(
**********************************************************************/

#ifndef _bitstreamStruct_h_
#define _bitstreamStruct_h_

/*
 * Although they should NEVER do so, some functions using the bitstream
 * module still want to have access to internal variables of the
 * BsBitBuffer and BsBitStream structures ...
 * ... and thus want include this file :-(
 *
 * HP 970219 990419
 */

#include "bitstreamHandle.h"	/* handles */

struct BsBitBufferStruct	/* bit buffer */
{
  unsigned char* data;          /* data bits */
  long           numBit;        /* number of bits in buffer */
  long           size;          /* buffer size in bits */
};

struct BsBitStreamStruct	/* bit stream */
{
  FILE *	file;		/* file or NULL for buffer i/o */
  int		write;		/* 0=read  1=write */
  long		streamId;	/* stream id (for debug) */
  char*		info;		/* info string (for read file) */
  BsBitBuffer*	buffer[2];	/* bit buffers */
				/* (buffer[1] is only used for read file) */
  long		currentBit;	/* current bit position in bit stream */
				/* (i.e. number of bits read/written) */
  long		numByte;	/* number of bytes read/written (only file) */
};


#endif	/* _bitstreamStruct_h_ */


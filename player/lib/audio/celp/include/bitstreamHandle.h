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

#ifndef _bitstreamHandle_h_
#define _bitstreamHandle_h_

/*
 * This file may be included instead of bitstream.h, if just the
 * handles are required but no function from bitstream.c is used.
 *
 * RS/HP 990421
 */

typedef struct BsBitBufferStruct BsBitBuffer;   /* bit buffer */

typedef struct BsBitStreamStruct BsBitStream;   /* bit stream */

/* VERSION 2 stuff, yet needed by some version 1 function prototypes  */
typedef struct tagEpInfo *HANDLE_EP_INFO;

#endif	/* _bitstreamHandle_h_ */


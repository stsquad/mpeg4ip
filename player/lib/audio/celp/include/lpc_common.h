/*
This software module was originally developed by
Heiko Purnhagen (University of Hannover)

and edited by

in the course of development of the
MPEG-2 NBC/MPEG-4 Audio standard ISO/IEC 13818-7, 14496-1,2 and 3.
This software module is an implementation of a part of one or more
MPEG-2 NBC/MPEG-4 Audio tools as specified by the MPEG-2 NBC/MPEG-4 Audio
standard. ISO/IEC  gives users of the MPEG-2 NBC/MPEG-4 Audio standards
free license to this software module or modifications thereof for use in
hardware or software products claiming conformance to the MPEG-2 NBC/
MPEG-4 Audio  standards. Those intending to use this software module in
hardware or software products are advised that this use may infringe
existing patents. The original developer of this software module and
his/her company, the subsequent editors and their companies, and ISO/IEC
have no liability for use of this software module or modifications
thereof in an implementation. Copyright is not released for non
MPEG-2 NBC/MPEG-4 Audio conforming products. The original developer
retains full right to use the code for his/her  own purpose, assign or
donate the code to a third party and to inhibit third party from using
the code for non MPEG-2 NBC/MPEG-4 Audio conforming products.
This copyright notice must be included in all copies or derivative works.
Copyright (c)1996.
*/

/**********************************************************************
MPEG-4 Audio VM

Source file: lpc_common.h

$Id: lpc_common.h,v 1.1 2002/05/13 18:57:42 wmaycisco Exp $

Authors:
HP    Heiko Purnhagen, Uni Hannover <purnhage@tnt.uni-hannover.de>

Changes:
25-oct-96   HP    common BITSTREAM and MAX_N_LAG_CANDIDATES
**********************************************************************/

#ifndef _lpc_common_h_
#define _lpc_common_h_

#include "bitstreamHandle.h"     /* handler */

/* see wdbxx.h and lpc_abscomp.ch */
#define MAX_N_LAG_CANDIDATES  15

#define MultiPulseExc  0
#define RegularPulseExc 1

typedef enum {
         fs8kHz=  0,
         fs16kHz= 1
} LPC_FS_MODE;

#define ScalarQuantizer 0
#define VectorQuantizer 1

#define Scalable_VQ 0
#define Optimized_VQ 1

#define OFF 0
#define ON  1

#define NO 0
#define YES 1


typedef struct {
  int          frameNumSample;             /* out: num samples per frame       */
  int          delayNumSample;
  BsBitStream* p_bitstream;
  BsBitBuffer* coreBitBuf;
  float**      sampleBuf;
  int          bitsPerFrame;
} LPC_DATA;  


#endif  /* #ifndef _lpc_common_h_ */

/* end of lpc_common.h */

/**********************************************************************
MPEG-4 Audio VM
Common module



This software module was originally developed by

Bodo Teichmann (FhG)

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

Copyright (c) 1997.



Source file: fir_filt.h

$Id: fir_filt.h,v 1.1 2002/05/13 18:57:42 wmaycisco Exp $

Authors:
tmn       Bodo Teichmann mailto:tmn@iis.fhg.de

Changes:
05-may-98 HP    added extern "C"
**********************************************************************/

#ifndef _fir_filt_h_
#define _fir_filt_h_

typedef struct
{
  float *memoryPtr;
  int writeIdx;
  int readIdx;
  float *filtPtr;
  int filtLength;
} FIR_FILT ;

#ifdef __cplusplus
extern "C" {
#endif

FIR_FILT *initFirLowPass(float stopBand,int taps);

void firLowPass(float* inBuffer,float *outBuffer,int no, FIR_FILT *filter );

void subSampl(float * inBuff, float * outBuff,int factor,int *noOfSampl);

#ifdef __cplusplus
}
#endif

#endif  /* _fir_filt_h_ */


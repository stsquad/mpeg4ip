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



Source file: fir_filt.c

$Id: fir_filt.c,v 1.1 2002/05/13 18:57:42 wmaycisco Exp $

Authors:
tmn       Bodo Teichmann mailto:tmn@iis.fhg.de

Changes:
**********************************************************************/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "common_m4a.h"
#include "fir_filt.h"
static float fir48_4_120[] =
  { -0.0000000000F, -0.0004061767F, -0.0006554074F, -0.0005603479F,
    +0.0000000000F, +0.0010332072F, +0.0024067306F, +0.0038511274F,
    +0.0049972718F, +0.0054446460F, +0.0048504364F, +0.0030230276F,
    -0.0000000000F, -0.0039090311F, -0.0081300062F, -0.0118895363F,
    -0.0143338396F, -0.0146862233F, -0.0124177060F, -0.0073988790F,
    +0.0000000000F, +0.0088885044F, +0.0179316783F, +0.0255223974F,
    +0.0300328067F, +0.0301098647F, +0.0249669378F, +0.0146174761F,
    -0.0000000000F, -0.0170435762F, -0.0339530545F, -0.0477884869F,
    -0.0556838859F, -0.0553529653F, -0.0455668957F, -0.0265189851F,
    +0.0000000000F, +0.0306695549F, +0.0609699140F, +0.0857542392F,
    +0.1000000000F, +0.0996415654F, +0.0823628662F, +0.0482228773F,
    -0.0000000000F, -0.0568263967F, -0.1144916118F, -0.1637363938F,
    -0.1948936008F, -0.1991420526F, -0.1697652726F, -0.1032409730F,
    +0.0000000000F, +0.1352696593F, +0.2938416526F, +0.4636919170F,
    +0.6306889405F, +0.7800976241F, +0.8982140690F, +0.9739261298F,
    +1.0000000000F, +0.9739261298F, +0.8982140690F, +0.7800976241F,
    +0.6306889405F, +0.4636919170F, +0.2938416526F, +0.1352696593F,
    +0.0000000000F, -0.1032409730F, -0.1697652726F, -0.1991420526F,
    -0.1948936008F, -0.1637363938F, -0.1144916118F, -0.0568263967F,
    -0.0000000000F, +0.0482228773F, +0.0823628662F, +0.0996415654F,
    +0.1000000000F, +0.0857542392F, +0.0609699140F, +0.0306695549F,
    +0.0000000000F, -0.0265189851F, -0.0455668957F, -0.0553529653F,
    -0.0556838859F, -0.0477884869F, -0.0339530545F, -0.0170435762F,
    -0.0000000000F, +0.0146174761F, +0.0249669378F, +0.0301098647F,
    +0.0300328067F, +0.0255223974F, +0.0179316783F, +0.0088885044F,
    +0.0000000000F, -0.0073988790F, -0.0124177060F, -0.0146862233F,
    -0.0143338396F, -0.0118895363F, -0.0081300062F, -0.0039090311F,
    -0.0000000000F, +0.0030230276F, +0.0048504364F, +0.0054446460F,
    +0.0049972718F, +0.0038511274F, +0.0024067306F, +0.0010332072F,
    +0.0000000000F, -0.0005603479F, -0.0006554074F, -0.0004061767F,
    -0.0000000000F,
  };
#define SCALE 0.128

FIR_FILT *initFirLowPass(float stopBand,int taps)
{
  FIR_FILT *filter;
  
  filter = (FIR_FILT*)malloc(sizeof(FIR_FILT));
  filter->filtLength=taps;
  filter->memoryPtr = (float*)malloc(sizeof(float)*(taps+1));
  filter->writeIdx =0 ;
  filter->readIdx = taps;
  if ((stopBand == 48000/4000)&& (taps == 120))
    filter->filtPtr=fir48_4_120;
  else 
    CommonExit(-1,"\nthis filter is not yet defined in fir_filt.c");
  
  return filter;
    

}


void firLowPass(float* inBuffer,float *outBuffer,int no, FIR_FILT *filter )
{ 
  int i,j,k;
  float y;
  for (j=0;j<no;j++) { 
    filter->memoryPtr[(filter->readIdx)] = inBuffer[j];
    filter->readIdx=(filter->readIdx + 1)%(filter->filtLength + 1);    
    y = 0.0; 
    k=0;
    for (i = filter->writeIdx; i <= filter->filtLength ; i++) 
      y += (filter->filtPtr[k++] * filter->memoryPtr[i]);
    for (i = 0; i < filter->writeIdx; i++) 
      y += (filter->filtPtr[k++] * filter->memoryPtr[i]);
    filter->writeIdx=(filter->writeIdx+1)%(filter->filtLength+1);    
#if 1 
    outBuffer[j] = y*SCALE;
#else 
    outBuffer[j] = y;
#endif
#if 0
    if ((long)(y*SCALE) != (short)(y*SCALE)){ 
      fprintf(stderr,"\nclipping!"); 
      exit(-1);
    }
#endif
  }
}
void subSampl(float * inBuff, float * outBuff,int factor,int *noOfSampl)
{
  int i ;
  if ((*noOfSampl%6)!= 0)
    CommonExit(-1,"\n Error in downsampling");
  else
    *noOfSampl = *noOfSampl/6;

  for (i=0;i< *noOfSampl;i++){
    outBuff[i]=inBuff[i * factor];
  }
}

/**********************************************************************
MPEG-4 Audio VM
Bit stream module



This software module was originally developed by

Bodo Teichmann (Fraunhofer Institute of Integrated Circuits tmn@iis.fhg.de)
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

Copyright (c) 1998.



$Id: flex_mux.h,v 1.1 2002/05/13 18:57:42 wmaycisco Exp $

BT    Bodo Teichmann, FhG/IIS <tmn@iis.fhg.de>

**********************************************************************/

#ifndef _FLEX_MUX_H_INCLUDED
#define _FLEX_MUX_H_INCLUDED

#include "obj_descr.h"           /* typedef structs */

typedef enum {
  GA   = 0,  /* General Audio, formerly called TF */
  CELP = 1,  /* MP4 Celp speech core */
  PARA = 2,  /* Parametric Audio coding, not supported */
  SA   = 3,  /* Structured Audio, not supported */
  TTS  = 4   /* Text To Speech, not supported */
  ,
  HVXC = 5   /* HVXC(parametric speech) core (AI 990616) */
} AUDIO_DEC_TYPE;

void advanceESDescr ( BsBitStream*      bitStream, 
                      ES_DESCRIPTOR*    es,
                      int               WriteFlag );
void initESDescr( ES_DESCRIPTOR **es);
void presetESDescr( ES_DESCRIPTOR *es, int numLayers);
void initObjDescr( OBJECT_DESCRIPTOR *od);
void presetObjDescr( OBJECT_DESCRIPTOR *od, int numLayers);

void getAccessUnit( BsBitStream*   bitStream,
                    BsBitBuffer*   AUBuffer,
                    unsigned int*  AUIndex,
                    unsigned long* totalLength,
                    ES_DESCRIPTOR  *es );

void  advanceODescr (   BsBitStream*      bitStream, OBJECT_DESCRIPTOR *od, int WriteFlag) ;
void writeFlexMuxPDU(int index,BsBitStream* bitStream , BsBitBuffer* AUBuffer);
int  nextAccessUnit( BsBitStream*      bitStream ,                      
                     unsigned int*     layer, 
                     FRAME_DATA*       frameData);
#endif

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



$Id: obj_descr.h,v 1.1 2002/05/13 18:57:42 wmaycisco Exp $

BT    Bodo Teichmann, FhG/IIS <tmn@iis.fhg.de>

**********************************************************************/

#ifndef _OBJ_DESCR_H_INCLUDED
#define _OBJ_DESCR_H_INCLUDED

#include "bitstreamHandle.h"     /* handler */

typedef unsigned char UINT8;
typedef unsigned long UINT32;


typedef struct  {
        UINT8  length ;/* in bits */
        UINT32 value;
} DESCR_ELE;



typedef struct {
 DESCR_ELE  TFCodingType ;
 DESCR_ELE  frameLength     ;
 DESCR_ELE  dependsOnCoreCoder;
 DESCR_ELE  coreCoderDelay;
 DESCR_ELE  extension;
 /* no channel config yet */
} TF_SPECIFIC_CONFIG;

typedef struct {
  DESCR_ELE  excitationMode ;
  DESCR_ELE  sampleRateMode ;
  DESCR_ELE  fineRateControl ;

  DESCR_ELE  RPE_Configuration ;

  DESCR_ELE  MPE_Configuration ;
  DESCR_ELE  numEnhLayers ;
  DESCR_ELE  bandwidthScalabilityMode ;

/*  DESCR_ELE  BWS_Configuration ; */
 /* no channel config yet */
} CELP_SPECIFIC_CONFIG;

typedef struct {
   DESCR_ELE  dummy;
 /* no channel config yet */
} PARA_SPECIFIC_CONFIG;

/* AI 990616 */
typedef struct {
  DESCR_ELE  HVXCvarMode;
  DESCR_ELE  HVXCrateMode;
  DESCR_ELE  extensionFlag;
} HVXC_SPECIFIC_CONFIG;



typedef struct {
 DESCR_ELE  audioDecoderType        ;
 DESCR_ELE  samplingFreqencyIndex;
 DESCR_ELE  channelConfiguration;
 union {
    TF_SPECIFIC_CONFIG TFSpecificConfig;
    CELP_SPECIFIC_CONFIG celpSpecificConfig;
    PARA_SPECIFIC_CONFIG paraSpecificConfig;
    HVXC_SPECIFIC_CONFIG hvxcSpecificConfig;	/* AI 990616 */
 } specConf;
}  AUDIO_SPECIFIC_CONFIG;



typedef struct {
 DESCR_ELE  profileAndLevelIndication;
 DESCR_ELE  streamType      ;
 DESCR_ELE  upsteam;
 DESCR_ELE  specificInfoFlag;
 DESCR_ELE  bufferSizeDB;
 DESCR_ELE  maxBitrate;
 DESCR_ELE  avgBitrate;
#if 0
 DESCR_ELE  bitresFullness;
#endif
 DESCR_ELE  specificInfoLength;
 AUDIO_SPECIFIC_CONFIG audioSpecificConfig;
} DEC_CONF_DESCRIPTOR ;


typedef struct {
 DESCR_ELE useAccessUnitStartFlag;
 DESCR_ELE useAccessUnitEndFlag;
 DESCR_ELE useRandomAccessPointFlag;
 DESCR_ELE usePaddingFlag;
 DESCR_ELE seqNumLength;
  /* to be completed */
} AL_CONF_DESCRIPTOR ;

typedef struct {
 DESCR_ELE  ESNumber;
 DESCR_ELE  streamDependence;
 DESCR_ELE  URLFlag;
 DESCR_ELE  extensFlag;
 DESCR_ELE  dependsOn_Es_number;
 DEC_CONF_DESCRIPTOR DecConfigDescr;  
 AL_CONF_DESCRIPTOR ALConfigDescriptor;
} ES_DESCRIPTOR ;


typedef struct {
 DESCR_ELE  ODLength; 
 DESCR_ELE  ODescrId;
 DESCR_ELE  streamCount;
 DESCR_ELE  extensionFlag;
 ES_DESCRIPTOR *ESDescriptor[8]; 
} OBJECT_DESCRIPTOR ;

typedef struct {
  BsBitBuffer*      bitBuf;
  int               sampleRate;
  int               bitRate;
  unsigned int NoAUInBuffer;
} LAYER_DATA ;


typedef struct {
  OBJECT_DESCRIPTOR* od ;
  LAYER_DATA layer[8];
  unsigned int scalOutSelect;
} FRAME_DATA ;

typedef struct {
  OBJECT_DESCRIPTOR* od ;
  LAYER_DATA layer[8];
  int        opMode;
} ENC_FRAME_DATA ;


#endif

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



Source file: flex_mux.c

$Id: flex_mux.c,v 1.1 2002/05/13 18:57:42 wmaycisco Exp $

Required modules:
common.o		common module
bitstream.o             bitstream handling module

BT    Bodo Teichmann, FhG/IIS <tmn@iis.fhg.de>

**********************************************************************/
#include <string.h>
#include <stdio.h>

#include "buffersHandle.h"       /* handler, defines, enums */

#include "bitstreamStruct.h"     /* structs */
#include "obj_descr.h"           /* structs */
#include "lpc_common.h"          /* structs */

#include "bitstream.h"
#include "common_m4a.h"
#include "flex_mux.h"

static   BsBitBuffer *tmpBuf;
void initObjDescr( OBJECT_DESCRIPTOR *od) 
{
  od->ODLength.length=32; 
  od->ODescrId.length=10;
  od->streamCount.length=5;
  od->extensionFlag.length=1;

  tmpBuf =BsAllocBuffer(256*8);
  
}

void presetObjDescr( OBJECT_DESCRIPTOR *od, int numLayers)
{

  od->ODLength.value=0; 
  od->ODescrId.value=1;
  od->streamCount.value=numLayers;
  od->extensionFlag.value=0;
  
}

static void initTFspecConf ( TF_SPECIFIC_CONFIG *tfConf )
{
  
  tfConf->TFCodingType.length=2 ;
  tfConf->frameLength.length=1 ;
  tfConf->dependsOnCoreCoder.length=1 ;
  tfConf->dependsOnCoreCoder.value=0 ;
  tfConf->coreCoderDelay.length=14 ;
  tfConf->coreCoderDelay.value=0 ;
  tfConf->extension.length=1 ;
  tfConf->extension.value=0 ;
  
  return;
}


void initCelpSpecConf (CELP_SPECIFIC_CONFIG *celpConf)
{
  celpConf->excitationMode.length = 1;
  celpConf->sampleRateMode.length = 1;
  celpConf->fineRateControl.length = 1;

  /* RPE mode */
  celpConf->RPE_Configuration.length = 3;

  /* MPE mode */
  celpConf->MPE_Configuration.length = 5;
  celpConf->numEnhLayers.length = 2;
  celpConf->bandwidthScalabilityMode.length = 1;
/*  celpConf->BWS_Configuration.length = 2; */
}


/* AI 990616 */
static void initHvxcSpecConf (HVXC_SPECIFIC_CONFIG *hvxcConf)
{
  hvxcConf->HVXCvarMode.length = 1;
  hvxcConf->HVXCrateMode.length = 2;
  hvxcConf->extensionFlag.length = 1;
  return;
}

void initESDescr( ES_DESCRIPTOR **es)
{

  *es = NULL;
  *es = (ES_DESCRIPTOR*) malloc(sizeof(ES_DESCRIPTOR));
  

  if (*es==NULL) CommonExit(-1,"no mem");

  memset (*es, 0, sizeof (ES_DESCRIPTOR)) ;

  (*es)->ESNumber.length=5;

  (*es)->streamDependence.length=1;
  (*es)->URLFlag.length=1;
  (*es)->extensFlag.length=1;
  (*es)->dependsOn_Es_number.length=5;

  (*es)->DecConfigDescr.profileAndLevelIndication.length=8 ;
  (*es)->DecConfigDescr.streamType.length=6 ;
  (*es)->DecConfigDescr.upsteam.length=1 ;
  (*es)->DecConfigDescr.specificInfoFlag.length=1 ;
  (*es)->DecConfigDescr.bufferSizeDB.length=24 ;
  (*es)->DecConfigDescr.maxBitrate.length=32 ;
  (*es)->DecConfigDescr.avgBitrate.length=32 ;
  (*es)->DecConfigDescr.specificInfoLength.length=8 ;
  (*es)->DecConfigDescr.audioSpecificConfig.audioDecoderType.length=3 ;
  (*es)->DecConfigDescr.audioSpecificConfig.samplingFreqencyIndex.length= 4;
  (*es)->DecConfigDescr.audioSpecificConfig.channelConfiguration.length=4 ;
  (*es)->ALConfigDescriptor.useAccessUnitStartFlag.length = 1;
  (*es)->ALConfigDescriptor.useAccessUnitEndFlag.length = 1;
  (*es)->ALConfigDescriptor.useRandomAccessPointFlag.length = 1;
  (*es)->ALConfigDescriptor.usePaddingFlag.length = 1;
  (*es)->ALConfigDescriptor.seqNumLength.length = 4;

}
void presetESDescr( ES_DESCRIPTOR *es,int layer)
{

  es->ESNumber.value=layer+1;

  /* if this is the first layer, there is no dependence */
  es->streamDependence.value=((layer==0)?0:1); 
  es->URLFlag.value=0;
  es->extensFlag.value=0;
  es->dependsOn_Es_number.value=(layer>0)?layer:0;

  es->DecConfigDescr.profileAndLevelIndication.value=0;
  es->DecConfigDescr.streamType.value=6 ; /* audio stream */
  es->DecConfigDescr.upsteam.value=0;
  es->DecConfigDescr.specificInfoFlag.value=1 ;
  es->DecConfigDescr.bufferSizeDB.value=6144;
  es->DecConfigDescr.maxBitrate.value=0;
  es->DecConfigDescr.avgBitrate.value=0;
  es->DecConfigDescr.specificInfoLength.value=2; /* 16 bits if TFcoding */

  /* es->DecConfigDescr.audioSpecificConfig.samplingFreqencyIndex.value= 0x6;*/  /*24 kHz */ /* cause this is patched afterward */
  es->DecConfigDescr.audioSpecificConfig.channelConfiguration.value=1 ;
}


static void BsRWBitWrapper(BsBitStream *stream,		/* in: bit stream */
                      unsigned long *data,		/* out: bits read/write */
                      int numBit,		/* in: num bits to read */
                      int WriteFlag)
{
  if (WriteFlag){
    BsPutBit(stream,*data,numBit);
  } else {
    BsGetBit(stream,data,numBit);
  }
}


static void    advanceTFspecConf (   BsBitStream*      bitStream,TF_SPECIFIC_CONFIG *tfConf,int WriteFlag) 
{
  
  BsRWBitWrapper(bitStream, &(tfConf->TFCodingType.value),  
                 tfConf->TFCodingType.length,WriteFlag);
  
  BsRWBitWrapper(bitStream, &(tfConf->frameLength.value),
                 tfConf->frameLength.length,WriteFlag);
  
  BsRWBitWrapper(bitStream, &(tfConf->dependsOnCoreCoder.value),  
                 tfConf->dependsOnCoreCoder.length,WriteFlag);
  if (tfConf->dependsOnCoreCoder.value != 0) {
    BsRWBitWrapper(bitStream, &(tfConf->coreCoderDelay.value),  
                   tfConf->coreCoderDelay.length,WriteFlag);
  }

  BsRWBitWrapper(bitStream, &(tfConf->extension.value),  
                 tfConf->extension.length,WriteFlag);
  
}

void advanceCelpSpecConf ( BsBitStream*     bitStream,CELP_SPECIFIC_CONFIG *celpConf,int WriteFlag) 
{
  BsRWBitWrapper(bitStream, &(celpConf->excitationMode.value),  
                 celpConf->excitationMode.length,WriteFlag);

  BsRWBitWrapper(bitStream, &(celpConf->sampleRateMode.value),  
                 celpConf->sampleRateMode.length,WriteFlag);
  
  BsRWBitWrapper(bitStream, &(celpConf->fineRateControl.value),  
                 celpConf->fineRateControl.length,WriteFlag);
  
  if (celpConf->excitationMode.value == RegularPulseExc)
  {
    BsRWBitWrapper(bitStream, &(celpConf->RPE_Configuration.value),  
                   celpConf->RPE_Configuration.length,WriteFlag);
  }
  
  if (celpConf->excitationMode.value == MultiPulseExc)
  {
    BsRWBitWrapper(bitStream, &(celpConf->MPE_Configuration.value),  
                   celpConf->MPE_Configuration.length,WriteFlag);

    BsRWBitWrapper(bitStream, &(celpConf->numEnhLayers.value),  
                   celpConf->numEnhLayers.length,WriteFlag);

    BsRWBitWrapper(bitStream, &(celpConf->bandwidthScalabilityMode.value),  
                   celpConf->bandwidthScalabilityMode.length,WriteFlag);
  }
}

/* AI 990616 */
static void advanceHvxcSpecConf(
BsBitStream* bitStream,
HVXC_SPECIFIC_CONFIG *hvxcConf,
int WriteFlag) 
{
  BsRWBitWrapper(bitStream, &(hvxcConf->HVXCvarMode.value),  
                 hvxcConf->HVXCvarMode.length,WriteFlag);
  
  BsRWBitWrapper(bitStream, &(hvxcConf->HVXCrateMode.value),
                 hvxcConf->HVXCrateMode.length,WriteFlag);
  
  BsRWBitWrapper(bitStream, &(hvxcConf->extensionFlag.value),  
                 hvxcConf->extensionFlag.length,WriteFlag);
  return;
}


void  advanceESDescr ( BsBitStream* bitStream, 
                       ES_DESCRIPTOR *es, 
                       int WriteFlag) 
{
  
  BsRWBitWrapper(bitStream,&(es->ESNumber.value),es->ESNumber.length,WriteFlag);
  
  BsRWBitWrapper(bitStream, &(es->streamDependence.value),es->streamDependence.length,WriteFlag);
  
  BsRWBitWrapper(bitStream, &(es->URLFlag.value),  es->URLFlag.length,WriteFlag);

  if (es->streamDependence.value != 0) {
    BsRWBitWrapper(bitStream, &(es->dependsOn_Es_number.value),  es->dependsOn_Es_number.length,WriteFlag);
  }

  BsRWBitWrapper(bitStream, &(es->extensFlag.value),  es->extensFlag.length,WriteFlag);

  BsRWBitWrapper(bitStream, &(es->DecConfigDescr.profileAndLevelIndication.value),  es->DecConfigDescr.profileAndLevelIndication.length,WriteFlag);
  BsRWBitWrapper(bitStream, &(es->DecConfigDescr.streamType.value) ,  es->DecConfigDescr.streamType.length,WriteFlag) ;
  BsRWBitWrapper(bitStream, &(es->DecConfigDescr.upsteam.value) ,  es->DecConfigDescr.upsteam.length,WriteFlag) ;
  BsRWBitWrapper(bitStream, &(es->DecConfigDescr.specificInfoFlag.value) ,  es->DecConfigDescr.specificInfoFlag.length,WriteFlag) ;
  BsRWBitWrapper(bitStream, &(es->DecConfigDescr.bufferSizeDB.value) ,  es->DecConfigDescr.bufferSizeDB.length,WriteFlag) ;

  BsRWBitWrapper(bitStream, &(es->DecConfigDescr.maxBitrate.value) ,  es->DecConfigDescr.maxBitrate.length,WriteFlag) ;
  BsRWBitWrapper(bitStream, &(es->DecConfigDescr.avgBitrate.value) ,  es->DecConfigDescr.avgBitrate.length,WriteFlag) ;

  BsRWBitWrapper(bitStream, &(es->DecConfigDescr.specificInfoLength.value) ,  es->DecConfigDescr.specificInfoLength.length,WriteFlag) ;
  BsRWBitWrapper(bitStream, &(es->DecConfigDescr.audioSpecificConfig.audioDecoderType.value),  
                 es->DecConfigDescr.audioSpecificConfig.audioDecoderType.length,WriteFlag) ;
  
  BsRWBitWrapper(bitStream, &(es->DecConfigDescr.audioSpecificConfig.samplingFreqencyIndex.value),  
                 es->DecConfigDescr.audioSpecificConfig.samplingFreqencyIndex.length,WriteFlag);
  BsRWBitWrapper(bitStream, &(es->DecConfigDescr.audioSpecificConfig.channelConfiguration.value),  
                 es->DecConfigDescr.audioSpecificConfig.channelConfiguration.length,WriteFlag);


  switch (es->DecConfigDescr.audioSpecificConfig.audioDecoderType.value)
    {
    case GA :
      initTFspecConf ( &(es->DecConfigDescr.audioSpecificConfig.specConf.TFSpecificConfig));
      advanceTFspecConf(bitStream,&(es->DecConfigDescr.audioSpecificConfig.specConf.TFSpecificConfig),WriteFlag);
      break;
    case CELP :
      initCelpSpecConf (&(es->DecConfigDescr.audioSpecificConfig.specConf.celpSpecificConfig)); 
      advanceCelpSpecConf(bitStream,&(es->DecConfigDescr.audioSpecificConfig.specConf.celpSpecificConfig),WriteFlag);
      break;
    case HVXC:	/* AI 990616 */
      initHvxcSpecConf (&(es->DecConfigDescr.audioSpecificConfig.specConf.hvxcSpecificConfig)); 
      advanceHvxcSpecConf(bitStream,&(es->DecConfigDescr.audioSpecificConfig.specConf.hvxcSpecificConfig),WriteFlag);
      break;
    default :
      CommonExit(-1,"audioDecoderType not implemented");
      break;
    }

  BsRWBitWrapper(bitStream, &(es->ALConfigDescriptor.useAccessUnitStartFlag.value),es->ALConfigDescriptor.useAccessUnitStartFlag.length,WriteFlag);

  BsRWBitWrapper(bitStream, &(es->ALConfigDescriptor.useAccessUnitEndFlag.value),es->ALConfigDescriptor.useAccessUnitEndFlag.length,WriteFlag);

  BsRWBitWrapper(bitStream, &(es->ALConfigDescriptor.useRandomAccessPointFlag.value),es->ALConfigDescriptor.useRandomAccessPointFlag.length,WriteFlag);

  BsRWBitWrapper(bitStream, &(es->ALConfigDescriptor.usePaddingFlag.value),es->ALConfigDescriptor.usePaddingFlag.length,WriteFlag);

  BsRWBitWrapper(bitStream, &(es->ALConfigDescriptor.seqNumLength.value),es->ALConfigDescriptor.seqNumLength.length,WriteFlag);
}


void  advanceODescr (   BsBitStream*      bitStream, OBJECT_DESCRIPTOR *od,int WriteFlag) 
{
  od->ODLength.length=32; 
  od->ODescrId.length=10;
  od->streamCount.length=5;
  od->extensionFlag.length=1;
  
/*   &(od->ODLength.value); is written later as soon as  length is known*/
  
  BsRWBitWrapper(bitStream, &(od->ODescrId.value),  od->ODescrId.length,WriteFlag);
  
  BsRWBitWrapper(bitStream, &(od->streamCount.value),  od->streamCount.length,WriteFlag);

  BsRWBitWrapper(bitStream, &(od->extensionFlag.value),  od->extensionFlag.length,WriteFlag);
}

void getAccessUnit( BsBitStream*      bitStream ,  BsBitBuffer* AUBuffer,unsigned int *AUIndex, unsigned long *totalLength, ES_DESCRIPTOR *es)
{
  unsigned long  index,length,AUStartFlag,AUEndFlag,dummy;
  unsigned long  seq_number;
  
  /* read default AL-PDU header */
  BsGetBit(bitStream,&index,8);
  BsGetBit(bitStream,&length,8);
  *totalLength += length;

  if (es->ALConfigDescriptor.useAccessUnitStartFlag.value)
    BsGetBit(bitStream,&AUStartFlag,1);
  if (AUStartFlag!=1) CommonExit(-1,"error in getAccessUnit");

  if (es->ALConfigDescriptor.useAccessUnitEndFlag.value)
    BsGetBit(bitStream,&AUEndFlag,1);

  if (es->ALConfigDescriptor.seqNumLength.value > 0)
    BsGetBit(bitStream,&seq_number,es->ALConfigDescriptor.seqNumLength.value);
  else
    BsGetBit(bitStream,&dummy,6);  /*6 padding bits (alm) */

  *AUIndex = index;
  
  if (AUStartFlag!=1) CommonExit(-1,"Error  AL-PDU header ");
#if 0  
  BsGetBuffer( bitStream, AUBuffer,length*8);    
#else
  BsGetBufferAppend( bitStream, AUBuffer,1,length*8);    
#endif
  while (AUEndFlag!=1) {
    BsGetBit(bitStream,&index,8);

    if (*AUIndex != index) 
      CommonExit(-1,"FlexMux index error");

    BsGetBit(bitStream,&length,8);
    *totalLength += length;

    if (es->ALConfigDescriptor.useAccessUnitStartFlag.value)
      BsGetBit(bitStream,&AUStartFlag,1);
    if (AUStartFlag==1) CommonExit(-1,"error in getAccessUnit");

    if (es->ALConfigDescriptor.useAccessUnitEndFlag.value)
      BsGetBit(bitStream,&AUEndFlag,1);

    if (es->ALConfigDescriptor.seqNumLength.value > 0)
      BsGetBit(bitStream,&seq_number,es->ALConfigDescriptor.seqNumLength.value);
    else
      BsGetBit(bitStream,&dummy,6);  /*6 padding bits (alm) */

    BsGetBufferAppend( bitStream, AUBuffer,1,length*8);    
  }  
}

int  nextAccessUnit( BsBitStream*      bitStream ,                      
                     unsigned int*     layer, 
                     FRAME_DATA*       frameData
                     )
{
  unsigned long index, dummy,AUStartFlag,AUEndFlag,AUIndex;
  unsigned long length;
  unsigned long  seq_number;
  ES_DESCRIPTOR*  es;
  BsBitBuffer*    AUBuffer  ;

  /* read default AL-PDU header */
  /* suppose trivial streammap table: index 0 = layer 0 ; index 1 = layer 1 etc.*/ 
  BsGetBit(bitStream,&index,8);
  *layer = index;
  AUIndex = index;
  BsGetBit(bitStream,&length,8);
  es = frameData->od->ESDescriptor[index];

  AUBuffer = frameData->layer[index].bitBuf  ;

  if (es->ALConfigDescriptor.useAccessUnitStartFlag.value)
    if (BsGetBit(bitStream,&AUStartFlag,1)== -1) {
      return -1;
    };
  if (AUStartFlag!=1) CommonExit(-1,"error in getAccessUnit");

  if (es->ALConfigDescriptor.useAccessUnitEndFlag.value)
    BsGetBit(bitStream,&AUEndFlag,1);

  if (es->ALConfigDescriptor.seqNumLength.value > 0)
    BsGetBit(bitStream,&seq_number,es->ALConfigDescriptor.seqNumLength.value);
  else
    BsGetBit(bitStream,&dummy,6);  /*6 padding bits (alm) */
  
  if (AUStartFlag!=1) CommonExit(-1,"Error  AL-PDU header ");
  if (AUBuffer!=0 ) {
    if ((AUBuffer->size - AUBuffer->numBit) > (long)length*8 )  {
      BsGetBufferAppend( bitStream, AUBuffer,1,length*8);    
      frameData->layer[AUIndex].NoAUInBuffer++;/* each decoder must decrease this by the number of decoded AU */
    }  else {
      BsGetSkip(bitStream,length*8);
      CommonWarning ("flexmux input buffer overflow for layer %d ; skiping next AU",index);
    }
  } else {
    BsGetSkip(bitStream,length*8);
  }
  while (AUEndFlag!=1) {
    BsGetBit(bitStream,&index,8);

    if (AUIndex != index) 
      CommonExit(-1,"FlexMux index error");

    BsGetBit(bitStream,&length,8);

    if (es->ALConfigDescriptor.useAccessUnitStartFlag.value)
      BsGetBit(bitStream,&AUStartFlag,1);
    if (AUStartFlag==1) CommonExit(-1,"error in getAccessUnit");

    if (es->ALConfigDescriptor.useAccessUnitEndFlag.value)
      BsGetBit(bitStream,&AUEndFlag,1);

    if (es->ALConfigDescriptor.seqNumLength.value > 0)
      BsGetBit(bitStream,&seq_number,es->ALConfigDescriptor.seqNumLength.value);
    else
      BsGetBit(bitStream,&dummy,6);  /*6 padding bits (alm) */

    if (AUBuffer!=0 ) {
      if ((AUBuffer->size - AUBuffer->numBit) > (long)length*8 )  {
        BsGetBufferAppend( bitStream, AUBuffer,1,length*8);    
      }  else {
        BsGetSkip(bitStream,length*8);
        CommonWarning ("flexmux input buffer overflow for layer %d ; skiping next AU",index);
      }
    } else {
      BsGetSkip(bitStream,length*8);
    }

  }  
  return 0;
}

void writeFlexMuxPDU(int index,BsBitStream* bitStream , BsBitBuffer* AUBuffer)
{
  unsigned long  align,tmp,i;
  unsigned long  length,AUStartFlag,AUEndFlag;
  int maxBytes=255;
  BsBitStream *AUStream;

  AUEndFlag=1;
  AUStartFlag=1;
    
  AUStream=    BsOpenBufferRead ( AUBuffer)	;

  AUStartFlag=1;
  while ( AUBuffer->numBit> maxBytes*8 ) {
    AUEndFlag=0;

    BsGetBuffer ( AUStream,tmpBuf,maxBytes*8);

    /* remove all completely read bytes from AUBuffer, 
       dirty 'cause bitstream.c does not yet have a funktion for that !*/
    tmp = AUStream->currentBit/8;
    for( i=0; i<(AUBuffer->size/8)-tmp; i++ ) {
      AUBuffer->data[i] = AUBuffer->data[i+tmp];
    }
    AUStream->currentBit -= tmp*8;
    AUBuffer->numBit -= tmp*8;  

    BsPutBit(bitStream,index,8);
    BsPutBit(bitStream,maxBytes,8);
    BsPutBit(bitStream,AUStartFlag,1);
    BsPutBit(bitStream,AUEndFlag,1);
    BsPutBit(bitStream,0,6);/* padding bits */

    BsPutBuffer(bitStream,tmpBuf);

    AUStartFlag=0;
    
  } 
  BsCloseRemove ( AUStream,1)	;

  AUEndFlag=1;
  length= AUBuffer->numBit/8;
  align = 8 - AUBuffer->numBit % 8;
  if (align == 8) align = 0;
  if (align != 0) {
    length+=1;
  }
  BsPutBit(bitStream,index,8);
  BsPutBit(bitStream,length,8);
  BsPutBit(bitStream,AUStartFlag,1);
  BsPutBit(bitStream,AUEndFlag,1);
  BsPutBit(bitStream,0,6);/* padding bits */
  BsPutBuffer(bitStream,AUBuffer);
  BsPutBit(bitStream,0,align);
  

  

  
}

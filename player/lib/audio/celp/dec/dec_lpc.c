/**********************************************************************
MPEG-4 Audio VM
Decoder core (LPC-based)



This software module was originally developed by

Heiko Purnhagen (University of Hannover)
Naoya Tanaka (Matsushita Communication Industrial Co., Ltd.)
Rakesh Taori, Andy Gerrits (Philips Research Laboratories, Eindhoven, The Netherlands),
Toshiyuki Nomura (NEC Corporation)

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

Copyright (c) 1996.



Source file: dec_lpc.c

$Id: dec_lpc.c,v 1.2 2002/06/21 23:19:50 wmaycisco Exp $

Required modules:
common.o        common module
cmdline.o        command line module
bitstream.o        bits stream module

Authors:
HP    Heiko Purnhagen, Uni Hannover <purnhage@tnt.uni-hannover.de>
NT    Naoya Tanaka, Panasonic <natanaka@telecom.mci.mei.co.jp>
AG    Andy Gerrits, Philips <gerritsa@natlab.research.philips.com>
PD    Paul Dillen, Philips Consumer Electronics
TN    Toshiyuki Nomura, NEC <t-nomura@dsp.cl.nec.co.jp>
RF    Ralf Funken, Philips <ralf.funken@ehv.ce.philips.com>

Changes:
24-jun-96   HP    dummy core
15-aug-96   HP    adapted to new dec.h
26-aug-96   HP    CVS
04-Sep-96   NT    LPC Core Ver. 1.11
26-Sep-96   AG    adapted for PHILIPS
25-oct-96   HP    joined Panasonic and Philips LPC frameworks
                  made function names unique
                    e.g.: abs_lpc_decode() -> PAN_abs_lpc_decode()
                    NOTE: pan_xxx() and PAN_xxx() are different functions!
		  I changed the PAN (and not the PHI) function names
                  just because thus less files had to be modified ...
28-oct-96   HP    added auto-select for pan/phi lpc modes
08-Nov-96   NT    Narrowband LPC Ver.2.00
                    - replaced synthesis filter module with Philips module
                    - I/Fs revision for common I/Fs with Philips modules
15-nov-96   HP    adapted to new bitstream module
26-nov-96   AG    changed abs_xxx() to celp_xxx()
27-Feb-97   TN    adapted to rate control functionality
27-Mar-97   AG    new functionalities for decoder added
18-Mar-98   PD    Philips private data (for multi-instance) added


**********************************************************************/


/* =====================================================================*/
/* Standard Includes                                                    */
/* =====================================================================*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* =====================================================================*/
/* MPEG-4 VM Includes                                                   */
/* =====================================================================*/
#include "block.h"               /* handler, defines, enums */
#include "buffersHandle.h"       /* handler, defines, enums */
#include "concealmentHandle.h"   /* handler, defines, enums */
#include "interface.h"           /* handler, defines, enums */
#include "mod_bufHandle.h"       /* handler, defines, enums */
#include "reorderspecHandle.h"   /* handler, defines, enums */
#include "resilienceHandle.h"    /* handler, defines, enums */
#include "tf_mainHandle.h"       /* handler, defines, enums */

#include "lpc_common.h"          /* structs */
#include "nok_ltp_common.h"      /* structs */
#include "obj_descr.h"           /* structs */
#include "tf_mainStruct.h"       /* structs */

#include "common_m4a.h"        /* common module                             */
#include "cmdline.h"       /* command line module                       */
#include "bitstream.h"     /* bit stream module                         */
#include "lpc_common.h"
#include "dec_lpc.h"           /* decoder cores                             */
#include "mp4_lpc.h"       /* common lpc defs                           */


/* =====================================================================*/
/* PHILIPS Includes                                                     */
/* =====================================================================*/
#include "phi_cons.h"
#include "celp_decoder.h"

/* =====================================================================*/
/* Panasonic Includes                                                   */
/* =====================================================================*/
#include "celp_proto_dec.h"
#include "pan_celp_const.h"

/* =====================================================================*/
/* NEC Includes                                                         */
/* =====================================================================*/
#include "nec_abs_const.h"

/* =====================================================================*/
/* L O C A L     S Y M B O L     D E C L A R A T I O N S                */
/* =====================================================================*/
#define PROGVER "CELP-based decoder core V5.0 13-nov-97"
#define SEPACHAR " ,="

/* =====================================================================*/
/* L O C A L     D A T A      D E C L A R A T I O N S                   */
/* =====================================================================*/
/* configuration inputs                                                 */
/* =====================================================================*/
extern int samplFreqIndex[];	/* undesired, but ... */


/*
Table 5.2   CELP coder (Mode II) configuration for 8 kHz sampling rate 
MPE_Configuration | Frame_size (#samples) | nrof_subframes | sbfrm_size (#samples) 
0,1,2        320          4   80 
3,4,5        240          3   80 
6 ... 12     160          2   80  
13 ... 21    160          4   40 
22 ... 26    80           2   40 
27           240          4   60 
28 ... 31 Reserved
*/
#if 0
static int MPE_Table[]={
  320,/*0 */
  320,/*1 */
  320,/*2 */
  240,/*3 */
  240,/*4 */
  240,/*5 */
  160,/*6 */
  160,/*7*/
  160,/*8 */
  160,/*9 */
  160,/*10 */
  160,/*11 */
  160,/*12 */
  160,/*13 */
  160,/*14 */
  160,/*15 */
  160,/*16 */
  160,/*17 */
  160,/*18 */
  160,/*19 */
  160,/*20 */
  160,/*21 */
  80,/*22 */
  80,/*23 */
  80,/*24 */
  80,/*25 */
  80,/*26 */
  240,/*27*/
  0,0,0,0,0,0,0,0,0,0,0
};
#endif

static long bit_rate;
static long sampling_frequency;
static long frame_size;
static long n_subframes;
static long sbfrm_size;
static long lpc_order;
static long num_lpc_indices;
static long num_shape_cbks;
static long num_gain_cbks;

static long *org_frame_bit_allocation;    

static long SampleRateMode     = 1;  /* Default: 16 kHz */
static long QuantizationMode   = 1;  /* Default: Vector Quantizer */
static long FineRateControl    = 0;  /* Default: FineRateControl is OFF */
static long LosslessCodingMode = 0;  /* Default: Lossless coding is OFF */
static long RPE_configuration;
static long Wideband_VQ = Optimized_VQ;
static long MPE_Configuration;
static long NumEnhLayers;
static long BandwidthScalabilityMode;
static long BWS_configuration;
static long reduced_order = 0;
static long complexity_level = 0;
static long PostFilterSW;

static long ExcitationMode;

extern int CELPdecDebugLevel;	/* HP 971120 */

static long  DecEnhStage;
static long  DecBwsMode;
static int  dummysw;
static int sysFlag;

static CmdLineSwitch switchList[] = {
  {"h",NULL,NULL,NULL,NULL,"print help"},
  {"lpc_n",&DecEnhStage,"%d","0",NULL,NULL},
  {"lpc_b",&DecBwsMode,"%d","0",NULL,NULL},
  {"lpc_p",&PostFilterSW,"%d","0",NULL,NULL},
  {"lpc_d",&CELPdecDebugLevel,"%d","0",NULL,"debug level"},
  {"-celp_sys",&sysFlag,NULL,NULL,NULL,"use system interface(flexmux)"},
/* to make celp ignor aac scalable switches in case of celp + aac decoding" */
  {"-mp4ff",&dummysw,NULL,NULL,NULL,"do nothing"}, 
  {"-out",&dummysw,"%d",NULL,NULL,"do nothing"},
  {NULL,NULL,NULL,NULL,NULL,NULL}
};

/* =====================================================================*/
/* instance context                                                     */
/* =====================================================================*/

static	void	*InstanceContext;	/*handle to one instance context */


/* =====================================================================*/
/* L O C A L    F U N C T I O N      D E F I N I T I O N S              */
/* =====================================================================*/
/* ---------------------------------------------------------------------*/
/* DecLpcInfo()                                                         */
/* Get info about LPC-based decoder core.                               */
/* ---------------------------------------------------------------------*/
char *DecLpcInfo (
  FILE *helpStream)       /* in: print decPara help text to helpStream */
                          /*     if helpStream not NULL                */
                          /* returns: core version string              */
{
  if (helpStream != NULL)
  {
    fprintf(helpStream, "--------------------------------------------------------\n");
    fprintf(helpStream, PROGVER "\n");
    fprintf(helpStream, "Usage: mp4dec -c \"<options>\" <name bitstream file>\n");
    fprintf(helpStream, "       where <options> are:\n");
    fprintf(helpStream, "             n <DecEnhStage> : Decoded number of enhancement layers (0, 1, 2, 3)\n");
    fprintf(helpStream, "             b <0/1>         : Decoding NarrowBand speech (0) or WideBand speech (1)\n");
    fprintf(helpStream, "             p <0/1>         : Post filter OFF (0) or ON (1). Default: OFF\n");
    fprintf(helpStream, "             -celp_sys       : use system interface(flexmux)\n");
    fprintf(helpStream, "             d <0/1>         : Debug Level OFF (0) or ON (1). Default: OFF\n");
    fprintf(helpStream, "--------------------------------------------------------\n");
  }
  return PROGVER;
}


/* ------------------------------------------------------------------- */
/* DecLpcInit()                                                        */
/* Init LPC-based decoder core.                                        */
/* ------------------------------------------------------------------- */
void DecLpcInit (
  int numChannel,                  /* in: num audio channels           */
  float fSample,                   /* in: sampling frequency [Hz]      */
  float bitRate,                   /* in: total bit rate [bit/sec]     */
  char *decPara,                   /* in: decoder parameter string     */
  BsBitBuffer *bitHeader,          /* in: header from bit stream       */
  int *frameNumSample,             /* out: num samples per frame       */
  int *delayNumSample)             /* out: decoder delay (num samples) */
{
  BsBitStream *hdrStream;
  int parac;
  char **parav;
  int result;
  int mp4ffFlag    =0;

  SampleRateMode     = 1;  /* Default: 16 kHz */
  QuantizationMode   = 1;  /* Default: Vector Quantizer */
  FineRateControl    = 0;  /* Default: FineRateControl is OFF */
  LosslessCodingMode = 0;  /* Default: Lossless coding is OFF */
  Wideband_VQ = Optimized_VQ;
  reduced_order = 0;
  complexity_level = 0;
  InstanceContext = NULL;
  bit_rate = 0;
 sampling_frequency = 0;
 frame_size = 0;
 n_subframes = 0;
 sbfrm_size = 0;
 lpc_order = 0;
 num_lpc_indices = 0;
 num_shape_cbks = 0;
 num_gain_cbks = 0;

 org_frame_bit_allocation = NULL;   

 RPE_configuration = 0;
 MPE_Configuration = 0;
 NumEnhLayers = 0;
 BandwidthScalabilityMode = 0;
 BWS_configuration = 0;
 PostFilterSW = 0;

 ExcitationMode = 0;

  DecEnhStage = 0;
   DecBwsMode = 0;
  dummysw = 0;
 sysFlag = 0;

  if (numChannel != 1 ) 
  {
    CommonExit(1,"EncLpcInit: Multi-channel audio input is not supported");
  }
  
  /* evalute decoder parameter string */
  parav = CmdLineParseString(decPara,SEPACHAR,&parac);
  result = CmdLineEval(parac,parav,NULL,switchList,1,NULL);
  if (result) 
  {
    if (result==1) 
	{
	  DecLpcInfo(stdout);
	  exit (1);
    }
    else {
      
      CommonExit(1,"Decoder parameter string error");
    }
  }  

  if ( (ExcitationMode == MultiPulseExc) && (SampleRateMode==fs16kHz) ) {
    Wideband_VQ = Optimized_VQ;
  }

  /* -------------------------------------------------------------------*/
  /* Memory allocation                                                  */
  /* -------------------------------------------------------------------*/
  hdrStream = BsOpenBufferRead(bitHeader);

  /* ---------------------------------------------------------------- */
  /* Conversion of parameters from float to longs                     */
  /* ---------------------------------------------------------------- */
  bit_rate           = (long)(bitRate+.5);
  sampling_frequency = (long)(fSample+.5);

  /* ---------------------------------------------------------------- */
  /* Decoder Initialisation                                           */
  /* ---------------------------------------------------------------- */
  celp_initialisation_decoder(
      hdrStream, bit_rate, complexity_level, reduced_order, DecEnhStage, DecBwsMode, PostFilterSW, &frame_size, &n_subframes, &sbfrm_size,		
      &lpc_order,&num_lpc_indices,&num_shape_cbks,&num_gain_cbks,&org_frame_bit_allocation,
			      &ExcitationMode,
		  &SampleRateMode, &QuantizationMode, &FineRateControl, &LosslessCodingMode, &RPE_configuration, 
		  &Wideband_VQ, &MPE_Configuration, &NumEnhLayers, &BandwidthScalabilityMode, &BWS_configuration,
		  &InstanceContext,mp4ffFlag);

  *frameNumSample = frame_size;
  *delayNumSample = 0;


	

  BsClose(hdrStream);
}
void DecLpcInitNew (
  char *decPara,                   /* in: decoder parameter string     */
  FRAME_DATA*  fD,
  LPC_DATA*    lpcData,
  int layer
  )             /* out: decoder delay (num samples) */
{
  int parac;
  char **parav;
  int result;
  int numChannel;
#if 0
  int MPE_conf;
#endif
  double frameLengthTime;

#if 0
  BsBitBuffer *bitHeader;
#endif
  BsBitStream *hdrStream=NULL;
  AUDIO_SPECIFIC_CONFIG* audSpC ;
  int mp4ffFlag    =1;
  
  lpcData->coreBitBuf = BsAllocBuffer( 4000 );      /* just some large number. For G729 required: 160 */


  if((lpcData->sampleBuf=(float**)(malloc(1 * sizeof (float*) )))==NULL) 
    CommonExit(1, "Memory allocation error in enc_lpc");

  if((lpcData->sampleBuf[0] =(float*)malloc(sizeof(float)*1024))==NULL)     
    CommonExit(1, "Memory allocation error in enc_lpc");


  audSpC = &fD->od->ESDescriptor[layer]->DecConfigDescr.audioSpecificConfig;
#if 0
  bitHeader=NULL;
#endif
    
  switch (audSpC->channelConfiguration.value) {
  case 1 :numChannel=1;       
    break;
  default: CommonExit(1,"wrong channel config");
  }

  
  /* evalute decoder parameter string */
  parav = CmdLineParseString(decPara,SEPACHAR,&parac);
  result = CmdLineEval(parac,parav,NULL,switchList,1,NULL);
  if (result==1) 
    {
      DecLpcInfo(stdout);
      exit (1);
    }

  if (strstr(decPara, "-celp_sys") != NULL) 
    sysFlag = 1;

  if ( (ExcitationMode == MultiPulseExc) && (SampleRateMode==fs16kHz) ) {
    Wideband_VQ = Optimized_VQ;
  }

#if 0
  /* -------------------------------------------------------------------*/
  /* Memory allocation                                                  */
  /* -------------------------------------------------------------------*/
  if (bitHeader != NULL)
    hdrStream = BsOpenBufferRead(bitHeader);
  /* ---------------------------------------------------------------- */
  /* Conversion of parameters from float to longs                     */
  /* ---------------------------------------------------------------- */
  bit_rate           = (long)(fD->od->ESDescriptor[layer]->DecConfigDescr.avgBitrate.value +.5);
  sampling_frequency = (long)(samplFreqIndex[
                                             fD->od->ESDescriptor[layer]->DecConfigDescr.audioSpecificConfig.samplingFreqencyIndex.value]+.5);

  if (sampling_frequency==7350) {
    sampling_frequency=8000;
  }
  
  MPE_conf=fD->od->ESDescriptor[0]->DecConfigDescr\
       .audioSpecificConfig.specConf.celpSpecificConfig.MPE_Configuration.value;
  lpcData->frameNumSample=MPE_Table[MPE_conf];
  frameLengthTime=(double)lpcData->frameNumSample/(double)sampling_frequency;

  lpcData->bitsPerFrame = (int)(bit_rate*frameLengthTime);     
#endif

  if (mp4ffFlag==1) {
    int layer=0; 
    CELP_SPECIFIC_CONFIG *celpConf=
      &fD->od->ESDescriptor[layer]->DecConfigDescr.audioSpecificConfig.specConf.celpSpecificConfig;
    
    ExcitationMode           =   celpConf->excitationMode.value ;
    SampleRateMode           =   celpConf->sampleRateMode.value ;
    FineRateControl          =   celpConf->fineRateControl.value;
    RPE_configuration        =   celpConf->RPE_Configuration.value;
    MPE_Configuration        =   celpConf->MPE_Configuration.value ;
    NumEnhLayers             =   celpConf->numEnhLayers.value;
    BandwidthScalabilityMode =   celpConf->bandwidthScalabilityMode.value;

    /* celp enhancement layer is not yet supported */
    BWS_configuration       =   0; /*  in celpConf of celp enhancement layer */
    if ( NumEnhLayers != 0 || BandwidthScalabilityMode != 0 ) {
      CommonExit(1,"celp enhancement layer is not yet supported");
    }
  }
  /* ---------------------------------------------------------------- */
  /* Decoder Initialisation                                           */
  /* ---------------------------------------------------------------- */
  celp_initialisation_decoder(
                              hdrStream, 
                              bit_rate, 
                              complexity_level, 
                              reduced_order, 
                              DecEnhStage, 
                              DecBwsMode, 
                              PostFilterSW, 
                              &frame_size, 
                              &n_subframes, 
                              &sbfrm_size,		
                              &lpc_order,
                              &num_lpc_indices,
                              &num_shape_cbks,
                              &num_gain_cbks,
                              &org_frame_bit_allocation,
                              &ExcitationMode,
                              &SampleRateMode, 
                              &QuantizationMode, 
                              &FineRateControl, 
                              &LosslessCodingMode, 
                              &RPE_configuration, 
                              &Wideband_VQ, 
                              &MPE_Configuration, 
                              &NumEnhLayers, 
                              &BandwidthScalabilityMode, 
                              &BWS_configuration,
                              &InstanceContext,
                              mp4ffFlag);
  
  lpcData->frameNumSample = frame_size;
  lpcData->delayNumSample = 0;

  bit_rate           = (long)(fD->od->ESDescriptor[layer]->DecConfigDescr.avgBitrate.value +.5);
  sampling_frequency = (long)(samplFreqIndex[
                                             fD->od->ESDescriptor[layer]->DecConfigDescr.audioSpecificConfig.samplingFreqencyIndex.value]+.5);

  if (sampling_frequency==7350) {
    sampling_frequency=8000;
  }
  
  frameLengthTime=(double)lpcData->frameNumSample/(double)sampling_frequency;
  lpcData->bitsPerFrame = (int)(bit_rate*frameLengthTime);     

/*   free(lpcData->p_bitstream); we use the memory in DecLpcFrameNew*/
}


/* ------------------------------------------------------------------- */
/* DecLpcFrame()                                                       */
/* Decode one bit stream frame into one audio frame with               */
/* LPC-based decoder core.                                             */
/* ------------------------------------------------------------------- */
void DecLpcFrame (
  BsBitBuffer *bitBuf,    /* in: bit stream frame                      */
  float **sampleBuf,      /* out: audio frame samples                  */
                          /*     sampleBuf[numChannel][frameNumSample] */
  int *usedNumBit)        /* out: num bits used for this frame         */
{

  /* ----------------------------------------------------------------- */
  /* Memory allocation                                                 */
  /* ----------------------------------------------------------------- */
  BsBitStream *bitStream = BsOpenBufferRead(bitBuf);
	//printf("\nframe size\n=%d",frame_size);
	//printf("\nsub frame size\n=%d",sbfrm_size);
	//printf("\n n_subframes\n=%d",n_subframes);
  /* ----------------------------------------------------------------- */
  /*Call Decoder                                                       */
  /* ----------------------------------------------------------------- */
  celp_decoder( bitStream, sampleBuf,
	       ExcitationMode,
		  SampleRateMode, QuantizationMode, FineRateControl, LosslessCodingMode,
		  RPE_configuration, Wideband_VQ, MPE_Configuration, NumEnhLayers, BandwidthScalabilityMode,
      BWS_configuration, frame_size,	n_subframes, sbfrm_size, lpc_order,	          
      num_lpc_indices, num_shape_cbks, num_gain_cbks,	          
      org_frame_bit_allocation,
      InstanceContext  );  

  *usedNumBit = BsCurrentBit(bitStream);
  BsCloseRemove(bitStream,1);
#if 0
  {
    BsBitStream* layer_stream ;
    
    layer_stream= BsOpenBufferRead(bitBuf);
    BsGetSkip(layer_stream,p_bitstream->valid_bits); 
    BsCloseRemove(layer_stream,1);
  }
#endif    

}

void DecLpcFrameNew (
  BsBitBuffer *bitBuf,    /* in: bit stream frame                      */
  float **sampleBuf,      /* out: audio frame samples                  */
                          /*     sampleBuf[numChannel][frameNumSample] */
  LPC_DATA*    lpcData,
  int *usedNumBit)        /* out: num bits used for this frame         */
{
  BsBitStream *bitStream = BsOpenBufferRead(bitBuf) ; 
  /* ----------------------------------------------------------------- */
  /*Call Decoder                                                       */
  /* ----------------------------------------------------------------- */
  celp_decoder( bitStream, sampleBuf,
                ExcitationMode,
                SampleRateMode, QuantizationMode, FineRateControl, LosslessCodingMode,
                RPE_configuration, Wideband_VQ, MPE_Configuration, NumEnhLayers, BandwidthScalabilityMode,
                BWS_configuration, frame_size,	n_subframes, sbfrm_size, lpc_order,	          
                num_lpc_indices, num_shape_cbks, num_gain_cbks,	          
                org_frame_bit_allocation,
                InstanceContext  );  

  *usedNumBit = BsCurrentBit(bitStream);
  if (sysFlag) {
    int celp_sys_align;

    celp_sys_align = 8 - (*usedNumBit % 8);
    if (celp_sys_align == 8) celp_sys_align = 0;
    BsGetSkip(bitStream,celp_sys_align);
    *usedNumBit = BsCurrentBit(bitStream);
  }

  BsCloseRemove(bitStream,1);
/*  BsClose(bitStream); */
#pragma warning "BsCloseRemove must be used instead of BsClose"
}


/* ------------------------------------------------------------------- */
/* DeLpcrFree()                                                        */
/* Free memory allocated by LPC-based decoder core.                    */
/* ------------------------------------------------------------------- */
void DecLpcFree ()
{
   celp_close_decoder(ExcitationMode, SampleRateMode, BandwidthScalabilityMode, org_frame_bit_allocation, &InstanceContext);
}

int lpcframelength( CELP_SPECIFIC_CONFIG *celpConf )
{
  int frame_size;

  if ( celpConf->excitationMode.value == RegularPulseExc ) {
    if (celpConf->RPE_Configuration.value == 0)	{
      frame_size = FIFTEEN_MS;
    } else if (celpConf->RPE_Configuration.value == 1) {
      frame_size  = TEN_MS;
    } else if (celpConf->RPE_Configuration.value == 2) {
      frame_size  = FIFTEEN_MS;
    } else if (celpConf->RPE_Configuration.value == 3) {
      frame_size  = FIFTEEN_MS;
    } else {
      fprintf(stderr, "ERROR: Illegal RPE Configuration\n");
      exit(1); 
    }
  }

  if ( celpConf->excitationMode.value == MultiPulseExc ) {
    if ( celpConf->sampleRateMode.value == fs8kHz) {
      if ( celpConf->MPE_Configuration.value < 3 ) {
	frame_size = NEC_FRAME40MS;
      }
      if ( celpConf->MPE_Configuration.value >= 3 &&
	   celpConf->MPE_Configuration.value < 6 ) {
	frame_size = NEC_FRAME30MS;
      }
      if ( celpConf->MPE_Configuration.value >= 6 &&
	   celpConf->MPE_Configuration.value < 22 ) {
	frame_size = NEC_FRAME20MS;
      }
      if ( celpConf->MPE_Configuration.value >= 22 &&
	   celpConf->MPE_Configuration.value < 27 ) {
	frame_size = NEC_FRAME10MS;
      }
      if ( celpConf->MPE_Configuration.value == 27 ) {
	frame_size = NEC_FRAME30MS;
      }
      if ( celpConf->MPE_Configuration.value > 27 ) {
	fprintf(stderr,"Error: Illegal MPE Configuration.\n");
	exit(1); 
      }
      if ( celpConf->bandwidthScalabilityMode.value == ON ) {
	frame_size = 2 * frame_size;
      }
    }
    if ( celpConf->sampleRateMode.value == fs16kHz) {
      if ( celpConf->MPE_Configuration.value < 16 ) {
	frame_size = NEC_FRAME20MS_FRQ16;
      }
      if ( celpConf->MPE_Configuration.value >= 16 &&
	   celpConf->MPE_Configuration.value < 32 ) {
	frame_size = NEC_FRAME10MS_FRQ16;
      }
      if ( celpConf->MPE_Configuration.value == 7 ||
	   celpConf->MPE_Configuration.value == 23 ) {
	fprintf(stderr,"Error: Illegal BitRate configuration.\n");
	exit(1); 
      }
    }
  }

  return( frame_size );
}
/* end of dec_lpc.c */

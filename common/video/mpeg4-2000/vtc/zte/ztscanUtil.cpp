/* $Id: ztscanUtil.cpp,v 1.1 2003/05/05 21:24:10 wmaycisco Exp $ */
/****************************************************************************/
/*   MPEG4 Visual Texture Coding (VTC) Mode Software                        */
/*                                                                          */
/*   This software was jointly developed by the following participants:     */
/*                                                                          */
/*   Single-quant,  multi-quant and flow control                            */
/*   are provided by  Sarnoff Corporation                                   */
/*     Iraj Sodagar   (iraj@sarnoff.com)                                    */
/*     Hung-Ju Lee    (hjlee@sarnoff.com)                                   */
/*     Paul Hatrack   (hatrack@sarnoff.com)                                 */
/*     Shipeng Li     (shipeng@sarnoff.com)                                 */
/*     Bing-Bing Chai (bchai@sarnoff.com)                                   */
/*     B.S. Srinivas  (bsrinivas@sarnoff.com)                               */
/*                                                                          */
/*   Bi-level is provided by Texas Instruments                              */
/*     Jie Liang      (liang@ti.com)                                        */
/*                                                                          */
/*   Shape Coding is provided by  OKI Electric Industry Co., Ltd.           */
/*     Zhixiong Wu    (sgo@hlabs.oki.co.jp)                                 */
/*     Yoshihiro Ueda (yueda@hlabs.oki.co.jp)                               */
/*     Toshifumi Kanamaru (kanamaru@hlabs.oki.co.jp)                        */
/*                                                                          */
/*   OKI, Sharp, Sarnoff, TI and Microsoft contributed to bitstream         */
/*   exchange and bug fixing.                                               */
/*                                                                          */
/*                                                                          */
/* In the course of development of the MPEG-4 standard, this software       */
/* module is an implementation of a part of one or more MPEG-4 tools as     */
/* specified by the MPEG-4 standard.                                        */
/*                                                                          */
/* The copyright of this software belongs to ISO/IEC. ISO/IEC gives use     */
/* of the MPEG-4 standard free license to use this  software module or      */
/* modifications thereof for hardware or software products claiming         */
/* conformance to the MPEG-4 standard.                                      */
/*                                                                          */
/* Those intending to use this software module in hardware or software      */
/* products are advised that use may infringe existing  patents. The        */
/* original developers of this software module and their companies, the     */
/* subsequent editors and their companies, and ISO/IEC have no liability    */
/* and ISO/IEC have no liability for use of this software module or         */
/* modification thereof in an implementation.                               */
/*                                                                          */
/* Permission is granted to MPEG members to use, copy, modify,              */
/* and distribute the software modules ( or portions thereof )              */
/* for standardization activity within ISO/IEC JTC1/SC29/WG11.              */
/*                                                                          */
/* Copyright 1995, 1996, 1997, 1998 ISO/IEC                                 */
/****************************************************************************/

/************************************************************/
/*     Sarnoff Very Low Bit Rate Still Image Coder          */
/*     Copyright 1995, 1996, 1997, 1998 Sarnoff Corporation */
/************************************************************/

//Added by Sarnoff for error resilience, 3/5/99
#define _ERROR_RESI_
//End: Added by Sarnoff for error resilience, 3/5/99

#include <stdio.h>
#include <stdlib.h>
#ifndef WIN32
#include <unistd.h>
#endif
#include "dataStruct.hpp"
#include "globals.hpp"
//#define ZTSCAN_UTIL // hjlee 0901
#include "ztscan_common.hpp"  // hjlee 0901

//Added by Sarnoff for error resilience, 3/5/99
/* all variables related to error resilience routines. bbc, 11/5/98 */
Int TU_first, TU_last;  /* texture unit numbers in a packet */
Int prev_TU_first, prev_TU_last; /* TU range in the previously corrected */
                                 /* decoded packet. */
Int TU_max, TU_max_dc,LTU,CTU_no;  /* max and currently decoded TU */
Int prev_TU_err;  /* <0 indicates that previous packet is erroneous */
Int packet_size,prev_segs_size;
Int errSignal,errWarnSignal,errMagSignal; /* error related signal */
//Int targetPacketLength;  //bbc, 2/19/99

/* var's related to finding the position of a TU, bbc, 11/6/98 */
Int start_h,start_w,band_height,band_width,TU_height,TU_spa_loc,
  TU_color,TU_band,prev_LTU,packet_band,packet_TU_first,packet_top,
  packet_left,wvt_level, level_h,level_w,subband_loc;

/* var's related to finding the position of previous detected good segment */
Int prev_good_TU,prev_good_height,prev_good_width;

ac_model acmSGMK;
UShort ifreq[2] = {1,3};
//End: Added by Sarnoff for error resilience, 3/5/99

ac_model acmVZ[NCOLOR], *acm_vz; /* here only for DC */
ac_model acmType[NCOLOR][MAXDECOMPLEV][MAX_NUM_TYPE_CONTEXTS], 
         *acm_type[MAXDECOMPLEV][MAX_NUM_TYPE_CONTEXTS];
ac_model acmSign[NCOLOR][MAXDECOMPLEV], *acm_sign[MAXDECOMPLEV];
ac_model *acmBPMag[NCOLOR][MAXDECOMPLEV], **acm_bpmag;
ac_model *acmBPRes[NCOLOR][MAXDECOMPLEV], **acm_bpres;
ac_model *acm_bpdc; // 1127

Void CVTCCommon::probModelInitSQ(Int col)
{
  SNR_IMAGE *snr_image;
  Int i,l;
  
  snr_image=&(mzte_codec.m_SPlayer[col].SNRlayer.snr_image);

  for (l=0; l<mzte_codec.m_iWvtDecmpLev;++l)
  {
    mzte_ac_model_init(&acmType[col][l][CONTEXT_INIT],NUMCHAR_TYPE,NULL,
		       ADAPT,1);
    mzte_ac_model_init(&acmType[col][l][CONTEXT_LINIT],2,NULL,ADAPT,1);
    mzte_ac_model_init(&acmSign[col][l],2,NULL,ADAPT,1);
  }

  for (l=0; l<mzte_codec.m_iWvtDecmpLev;++l)
  {
    if ((acmBPMag[col][l]
	 =(ac_model *)calloc(WVTDECOMP_NUMBITPLANES(col,l),sizeof(ac_model)))==NULL&& WVTDECOMP_NUMBITPLANES(col,l)!=0) // modified by Sharp (99/5/10)
      errorHandler("Can't alloc acmBPMag in probModelInitSQ.");
    for(i=0;i<WVTDECOMP_NUMBITPLANES(col,l);i++)
    {
      mzte_ac_model_init(&acmBPMag[col][l][i],2,NULL,ADAPT,1);
      acmBPMag[col][l][i].Max_frequency=Bitplane_Max_frequency;
    }
  }

}

Void CVTCCommon::probModelFreeSQ(Int col)
{
  SNR_IMAGE *snr_image;
  Int i,l;
  
  snr_image=&(mzte_codec.m_SPlayer[col].SNRlayer.snr_image);

  for (l=0; l<mzte_codec.m_iWvtDecmpLev;++l)
  {  
    mzte_ac_model_done(&acmType[col][l][CONTEXT_INIT]);
    mzte_ac_model_done(&acmType[col][l][CONTEXT_LINIT]);
    mzte_ac_model_done(&acmSign[col][l]);
  }

  for (l=0; l<mzte_codec.m_iWvtDecmpLev;++l)
  {
    for(i=0;i<WVTDECOMP_NUMBITPLANES(col,l);i++)
      mzte_ac_model_done(&acmBPMag[col][l][i]);
    free(acmBPMag[col][l]);
  }

}

Void CVTCCommon::setProbModelsSQ(Int col)
{
  Int l;

  /* Set prob model type pointers */
  for (l=0; l<mzte_codec.m_iWvtDecmpLev;++l)
  {  
    acm_type[l][CONTEXT_INIT]=&acmType[col][l][CONTEXT_INIT];
    acm_type[l][CONTEXT_LINIT]=&acmType[col][l][CONTEXT_LINIT];
    acm_sign[l]=&acmSign[col][l];
  }

  acm_bpmag=acmBPMag[col];
}

Void CVTCCommon::probModelInitMQ(Int col)
{
  SNR_IMAGE *snr_image;
  Int i,l;
  
  snr_image=&(mzte_codec.m_SPlayer[col].SNRlayer.snr_image);
  
  for (l=0; l<mzte_codec.m_iWvtDecmpLev;++l)
  {
    mzte_ac_model_init(&acmType[col][l][CONTEXT_INIT],NUMCHAR_TYPE,NULL,
		       ADAPT,1);
    mzte_ac_model_init(&acmType[col][l][CONTEXT_LINIT],2,NULL,ADAPT,1);
    mzte_ac_model_init(&acmType[col][l][CONTEXT_ZTR],NUMCHAR_TYPE,NULL,
		       ADAPT,1);
    mzte_ac_model_init(&acmType[col][l][CONTEXT_ZTR_D],NUMCHAR_TYPE,NULL,
		       ADAPT,1);
    mzte_ac_model_init(&acmType[col][l][CONTEXT_IZ],2,NULL,ADAPT,1);
    mzte_ac_model_init(&acmType[col][l][CONTEXT_LZTR],2,NULL,ADAPT,1);
    mzte_ac_model_init(&acmType[col][l][CONTEXT_LZTR_D],2,NULL,ADAPT,1);
    mzte_ac_model_init(&acmSign[col][l],2,NULL,ADAPT,1);
  }


  for (l=0; l<mzte_codec.m_iWvtDecmpLev;++l)
  {
    //	int j = mzte_codec.m_SPlayer[col].SNRlayer.snr_image.wvtDecompNumBitPlanes[l];
    //	int jj = mzte_codec.m_SPlayer[col].SNRlayer.snr_image.wvtDecompResNumBitPlanes ; 
	if ((acmBPMag[col][l]
	 =(ac_model *)calloc(WVTDECOMP_NUMBITPLANES(col,l),sizeof(ac_model)))==NULL&& WVTDECOMP_NUMBITPLANES(col,l)!=0) // modified by Sharp (99/5/10)
      errorHandler("Can't alloc acmBPMag in probModelInitSQ.");
    for(i=0;i<WVTDECOMP_NUMBITPLANES(col,l);i++)
    {
      mzte_ac_model_init(&acmBPMag[col][l][i],2,NULL,ADAPT,1);
      acmBPMag[col][l][i].Max_frequency=Bitplane_Max_frequency;
    }

    if ((acmBPRes[col][l]
	 =(ac_model *)calloc(WVTDECOMP_RES_NUMBITPLANES(col),sizeof(ac_model)))==NULL&& WVTDECOMP_RES_NUMBITPLANES(col) != 0 ) // modified by Sharp (99/5/10)
      errorHandler("Can't alloc acmBPRes in probModelInitMQ.");
    for(i=0;i<WVTDECOMP_RES_NUMBITPLANES(col);i++)
    {
      mzte_ac_model_init(&acmBPRes[col][l][i],2,NULL,ADAPT,1);
      acmBPRes[col][l][i].Max_frequency=Bitplane_Max_frequency;
    }
  }

}

Void CVTCCommon::probModelFreeMQ(Int col)
{
  SNR_IMAGE *snr_image;
  Int i,l;
  
  snr_image=&(mzte_codec.m_SPlayer[col].SNRlayer.snr_image);

  for (l=0; l<mzte_codec.m_iWvtDecmpLev;++l)
  {  
    for (i=0; i<MAX_NUM_TYPE_CONTEXTS; ++i)
      mzte_ac_model_done(&acmType[col][l][i]);

    mzte_ac_model_done(&acmSign[col][l]);
  }

  for (l=0; l<mzte_codec.m_iWvtDecmpLev;++l)
  {
    for(i=0;i<WVTDECOMP_NUMBITPLANES(col,l);i++)
      mzte_ac_model_done(&acmBPMag[col][l][i]);
    free(acmBPMag[col][l]);

    for(i=0;i<WVTDECOMP_RES_NUMBITPLANES(col);i++)
      mzte_ac_model_done(&acmBPRes[col][l][i]);    
    free(acmBPRes[col][l]);
  }

}

Void CVTCCommon::setProbModelsMQ(Int col)
{
  Int l, i;

  /* Set prob model type pointers */
    
  for (l=0; l<mzte_codec.m_iWvtDecmpLev;++l)
  {  
    for (i=0; i<MAX_NUM_TYPE_CONTEXTS; ++i)
      acm_type[l][i]=&acmType[col][l][i];
  
    acm_sign[l]=&acmSign[col][l];
  }

  acm_bpmag=acmBPMag[col];
  acm_bpres=acmBPRes[col];

}

/*********************************************
  Initialize max frequency for each ac model
*********************************************/
Void CVTCCommon::init_acm_maxf_enc()
{
  Int c, l, i;
  
  if(mzte_codec.m_iAcmMaxFreqChg == 0)
  {
    for (c=0; c<mzte_codec.m_iColors; c++) 
    {  	
      for (l=0; l<mzte_codec.m_iWvtDecmpLev;++l)
      {
	for (i=0; i<MAX_NUM_TYPE_CONTEXTS; ++i)
//	  acmType[c][l][i].Max_frequency=DEFAULT_MAX_FREQ;  // 1127
	  acmType[c][l][i].Max_frequency=Bitplane_Max_frequency;
	
	acmSign[c][l].Max_frequency=DEFAULT_MAX_FREQ;
     }
      
      acmVZ[c].Max_frequency=DEFAULT_MAX_FREQ;
      
    }
  }
  else
  {
    for (c=0; c<mzte_codec.m_iColors; c++) 
    {  	
      for (l=0; l<mzte_codec.m_iWvtDecmpLev;++l)
      {
	for (i=0; i<MAX_NUM_TYPE_CONTEXTS; ++i)
	  acmType[c][l][i].Max_frequency=mzte_codec.m_iAcmMaxFreq[0];
        
      acmSign[c][l].Max_frequency=mzte_codec.m_iAcmMaxFreq[5];

      }

      acmVZ[c].Max_frequency=mzte_codec.m_iAcmMaxFreq[1];
    }
  }
}

Void CVTCCommon::init_acm_maxf_dec()
{
  init_acm_maxf_enc();
}


/********************************************************
  Function Name
  -------------
  Void clear_ZTR_D()

  Arguments
  ---------
  None.

  
  Description
  -----------
  Clear the zerotree descendent marks in the entire image.

  Functions Called
  ----------------
  None.


  Return Value
  ------------
  None.

********************************************************/ 
Void CVTCCommon::clear_ZTR_D(COEFFINFO **coeffinfo, Int width, Int height)
{
  register COEFFINFO **coeff;
  register int i,j,dc_h2,dc_w2;

  coeff=coeffinfo;
  dc_h2=mzte_codec.m_iDCHeight<<1;
  dc_w2=mzte_codec.m_iDCWidth<<1;

  for(i=0;i<dc_h2;i++)
    for(j=dc_w2;j<width;j++)
      if(coeff[i][j].type == ZTR_D)
         coeff[i][j].type = UNTYPED;

  for(i=dc_h2;i<height;i++)
    for(j=0;j<width;j++)
      if(coeff[i][j].type == ZTR_D)
         coeff[i][j].type = UNTYPED;
}


//Added by Sarnoff for error resilience, 3/5/99
/* ----------------------------------------------------------------- */
/* ----------------- Error resilience related routines ------------- */
/* ----------------------------------------------------------------- */

/***************************************************/
/* Get the location of a TU, information includes  */
/* band, coordinate, color, size. bbc, 11/6/98     */
/***************************************************/
Void CVTCDecoder::get_TU_location(Int LTU)
{
  Int a,b,dc_h=mzte_codec.m_iDCHeight,dc_w=mzte_codec.m_iDCWidth;

#ifdef _DC_PACKET_
  LTU-=TU_max_dc;  /* bbc, 11/18/98 */
#endif

  if(mzte_codec.m_iScanDirection==0)
  { 
    /* TD case */
    if(LTU==-1){
      start_w=mzte_codec.m_iDCWidth;
      start_h=0;
      TU_band=TU_color=0;
      return;
    }
    a=LTU/9;
    b=LTU/3;
    start_h=a/dc_w;
    start_w=a%dc_w;
    TU_color=b%3;
    switch(TU_band=(LTU%9)%3){
      case 0:
        start_w +=dc_w;
        break;
      case 1:
        start_h +=dc_h;
          break;
      case 2:
        start_h +=dc_h;
        start_w +=dc_w;
    }
  }
  else
  {
    /* BB case */
    Int l;

    a = LTU / dc_h;
    
    if (a==0)
    {
      l = 0;
      TU_color = 0;
    }
    else
    {
      l = (a-1)/3 + 1;
      TU_color = (a-1)%3;
    }
    
    if (TU_color)
      --l;

    band_height = dc_h<<l;

    b = LTU%dc_h;
    start_h = b<<l;  

#if 0
    /* getting next TU in the same band */
    if(LTU==prev_LTU+1){
      if(start_h<band_height){
	start_h +=TU_height;
	if(start_h<band_height-1){
	  prev_LTU=LTU;
	  goto check_packet_bound;
	}
      }
      else{
	start_h +=TU_height;
	if(start_h<2*band_height-1){
	  prev_LTU=LTU;
	  goto check_packet_bound;
	}
      }
    }

    prev_LTU=LTU;
    a=LTU/mzte_codec.m_iDCHeight;
    b=LTU%mzte_codec.m_iDCHeight; /* location (TU_number) in a band */
    
    if(a<3){  /* Lowest Y */
      TU_spa_loc=0;
      band_height=mzte_codec.m_iDCHeight;
      band_width=mzte_codec.m_iDCWidth;
      TU_color=0; TU_band=a;
      TU_height=1;
    }
    else{
      TU_spa_loc=(a-3)/9+1;
      TU_color=((a-3)/3)%3;
      TU_band=(a-3)%3;
      if(TU_color==0){
	band_height=mzte_codec.m_iDCHeight<<TU_spa_loc;
	band_width=mzte_codec.m_iDCWidth<<TU_spa_loc;
	TU_height=1<<TU_spa_loc;
      }
      else{
	band_height=mzte_codec.m_iDCHeight<<(TU_spa_loc-1);
	band_width=mzte_codec.m_iDCWidth<<(TU_spa_loc-1);
	TU_height=1<<(TU_spa_loc-1);
      }
    }
    
    switch(TU_band){
      case 0:
	start_h=b*TU_height;
	start_w=band_width;
	break;
      case 1:
	start_h=band_height+b*TU_height;
	start_w=0;
	break;
      case 2:
	start_h=band_height+b*TU_height;
	start_w=band_width;
	break;
    }
    
  check_packet_bound:
    /* Find or update the packet boundary, for determining */
    /* context model in arithmetic coding, bbc, 10/5/98 */
    if(TU_first !=packet_TU_first || packet_band !=TU_band){
      packet_TU_first=TU_first;
      packet_band=TU_band;
      packet_top=start_h;
      packet_left=start_w;
    }
#endif
  }
}


#if 0 // not used currently
/**************************************************/
/* Function to check if a coefficient is at the   */
/* position to check end of segment, bbc, 11/6/98 */
/**************************************************/
/* may not need this any more */
Int need_check_segment_end(Int h,Int w,Int color)
{
  int block_size=16;

  if(wvt_level==0)  /* no checking needed for root band */
    return 0;

  switch(subband_loc){
    case 0:  /* LH */
      w-=level_w;
      break;
    case 1:  /* HL */
      h-=level_h;
      break;
    case 2:  /* HH */
      h-=level_h;
      w-=level_w;
      break;
    default:
      errorHandler("%d is not a choice for subband_loc.",subband_loc);
  }

  if(wvt_level<=3)
    block_size=1<<wvt_level;

  if((color == 0 && wvt_level==mzte_codec.wvtDecompLev-1) ||
     (color != 0 && wvt_level==mzte_codec.wvtDecompLev-2)){
    if(!((h+1)%block_size || (w+1)%block_size)){
      if(h+1 != level_h || w+1 != level_w)
        return 1;
    }
    return 0;
  }
  else
    return (!((h+1)%block_size || (w+1)%block_size));
}

#endif 


/**********************************************/
/* Record the location of previously detected */
/* good segment for TD mode, bbc, 10/23/98    */
/**********************************************/
Void CVTCDecoder::set_prev_good_TD_segment(Int TU,int h,int w)
{
  prev_good_TU=TU;
  prev_good_height=h;
  prev_good_width=w;
}

/**********************************************/
/* Record the location of previously detected */
/* good segment for BB mode, ph, 11/19/98     */
/**********************************************/
Void CVTCDecoder::set_prev_good_BB_segment(Int TU,int h,int w)
{
  prev_good_TU=TU;
  prev_good_height=h;
  prev_good_width=w;
}
//End: Added by Sarnoff for error resilience, 3/5/99

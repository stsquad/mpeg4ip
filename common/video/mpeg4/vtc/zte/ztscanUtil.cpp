/* $Id: ztscanUtil.cpp,v 1.2 2001/05/09 21:14:18 cahighlander Exp $ */
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

#include <stdio.h>
#include <stdlib.h>
#ifndef WIN32
#include <unistd.h>
#endif
#include "dataStruct.hpp"
#include "globals.hpp"
//#define ZTSCAN_UTIL // hjlee 0901
#include "ztscan_common.hpp"  // hjlee 0901
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
	 =(ac_model *)calloc(WVTDECOMP_NUMBITPLANES(col,l),sizeof(ac_model)))==NULL)
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
    // int j = mzte_codec.m_SPlayer[col].SNRlayer.snr_image.wvtDecompNumBitPlanes[l];
    // int jj = mzte_codec.m_SPlayer[col].SNRlayer.snr_image.wvtDecompResNumBitPlanes ; 
	if ((acmBPMag[col][l]
	 =(ac_model *)calloc(WVTDECOMP_NUMBITPLANES(col,l),sizeof(ac_model)))==NULL)
      errorHandler("Can't alloc acmBPMag in probModelInitSQ.");
    for(i=0;i<WVTDECOMP_NUMBITPLANES(col,l);i++)
    {
      mzte_ac_model_init(&acmBPMag[col][l][i],2,NULL,ADAPT,1);
      acmBPMag[col][l][i].Max_frequency=Bitplane_Max_frequency;
    }

    if ((acmBPRes[col][l]
	 =(ac_model *)calloc(WVTDECOMP_RES_NUMBITPLANES(col),sizeof(ac_model)))==NULL)
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


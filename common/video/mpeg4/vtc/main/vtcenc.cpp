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

#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#define DEFINE_GLOBALS

#include "basic.hpp"
#include "dataStruct.hpp"
#include "startcode.hpp"
#include "bitpack.hpp"
#include "msg.hpp"
#include "globals.hpp"
#include "states.hpp" // hjlee 0901



CVTCEncoder::CVTCEncoder()
{
	m_cImagePath = new Char[80];
	m_cSegImagePath = new Char[80];
	m_cOutBitsFile = new Char[80];
	mzte_codec.m_cBitFile = new Char[80];
}

CVTCEncoder::~CVTCEncoder()
{
	delete m_cImagePath;
	delete m_cSegImagePath;
	delete m_cOutBitsFile;
	delete mzte_codec.m_cBitFile;
}

//CVTCEncoder::CVTCEncoder(
Void CVTCEncoder::init(
		Char* cImagePath,
	    UInt uiAlphaChannel,
		Char* cSegImagePath,
		UInt uiAlphaTh,
		UInt uiChangeCRDisable,
		Char* cOutBitsFile,
		UInt uiColors,
		UInt uiFrmWidth,
		UInt uiFrmHeight,
		UInt uiWvtType,
		UInt uiWvtDownload,  // hjlee 0901
		UInt uiWvtDecmpLev,
		UInt uiWvtUniform, // hjlee 0901
		Int* iWvtFilters,  // hjlee 0901
		UInt uiQuantType,
		UInt uiScanDirection,
		Bool bStartCodeEnable,
		UInt uiTargetSpatialLev,
		UInt uiTargetSNRLev,
		UInt uiQdcY,
		UInt uiQdcUV,
		UInt uiSpatialLev ,
		UInt defaultSpatialScale, // hjlee 0901
		Int  *lastWvtDecompInSpaLayer, // hjlee 0901
		SNR_PARAM** Qinfo 
	)
{

	// input/output

	strcpy(m_cImagePath,cImagePath);
	strcpy(m_cSegImagePath,cSegImagePath);
	strcpy(m_cOutBitsFile,cOutBitsFile);
	strcpy(mzte_codec.m_cBitFile,cOutBitsFile);
	
	mzte_codec.m_iAlphaChannel = (int)uiAlphaChannel;
	mzte_codec.m_iAlphaTh = (int)uiAlphaTh;
	mzte_codec.m_iChangeCRDisable = (int)uiChangeCRDisable;

	mzte_codec.m_iBitDepth = 8;
	mzte_codec.m_iColorFormat = 0;
	mzte_codec.m_iColors = (int)uiColors;
	mzte_codec.m_iWidth = (int)uiFrmWidth;
	mzte_codec.m_iHeight = (int)uiFrmHeight;
	mzte_codec.m_iWvtType = (int)uiWvtType;
	mzte_codec.m_iWvtDecmpLev = (int)uiWvtDecmpLev;
	mzte_codec.m_iQuantType = (int)uiQuantType;
	mzte_codec.m_iScanDirection = (int)uiScanDirection;
	mzte_codec.m_bStartCodeEnable = bStartCodeEnable;
	mzte_codec.m_iSpatialLev = (int)uiSpatialLev;
	mzte_codec.m_iTargetSpatialLev = (int)uiTargetSpatialLev;
	mzte_codec.m_iTargetSNRLev = (int)uiTargetSNRLev;


	mzte_codec.m_iWvtDownload = (int)uiWvtDownload; // hjlee 0901
	mzte_codec.m_iWvtUniform = (int)uiWvtUniform;// hjlee 0901
	mzte_codec.m_WvtFilters = (int *)iWvtFilters; // hjlee 0901
	mzte_codec.m_defaultSpatialScale = (int)defaultSpatialScale; // hjlee 0901
	if (uiQuantType == 2) {
		if (uiSpatialLev != uiWvtDecmpLev) {
		  /* read in usedefault */
		  mzte_codec.m_defaultSpatialScale=(int)defaultSpatialScale;
		  if (defaultSpatialScale==0) {
			int spa_lev;

			/* read in spatial layer info */
			for (spa_lev=0; spa_lev<(int)uiSpatialLev-1; ++spa_lev)   
			  mzte_codec.m_lastWvtDecompInSpaLayer[spa_lev][0]
				= lastWvtDecompInSpaLayer[spa_lev];
		  }
		}
	}

	for (int i=0; i<(int)uiColors; i++)
		mzte_codec.m_Qinfo[i] = Qinfo[i];

	mzte_codec.m_iQDC[0] = (int)uiQdcY;
	mzte_codec.m_iQDC[1] = mzte_codec.m_iQDC[2] = (int)uiQdcUV;

	mzte_codec.m_iDCWidth = GetDCWidth();
	mzte_codec.m_iDCHeight = GetDCHeight();
  
	mzte_codec.m_iSingleBitFile = 1;
//	mzte_codec.m_cBitFile = NULL;
	mzte_codec.m_cBitFileAC = NULL;

	// check parameter
	if (mzte_codec.m_iQuantType==1 && mzte_codec.m_iScanDirection==0)
		mzte_codec.m_bStartCodeEnable = 0;

}


/*******************************************************/
/*  Flush buffer and close file. Only and must be used */
/*   by encoder to close bitstream file                */
/*******************************************************/
Void CVTCEncoder::flush_buffer_file()
{
  flush_bits();
  flush_bytes();
}

Void CVTCEncoder::close_buffer_file(FILE *fp)
{
  flush_buffer_file();
  fclose(fp);
}

//Void CVTCEncoder::header_Enc(FILTER *wvtfilter)  // hjlee 0901
Void CVTCEncoder::header_Enc(FILTER **wvtfilter) // hjlee 0901
{
  Int  texture_object_id=0;
  Int  texture_object_layer_shape=mzte_codec.m_iAlphaChannel;
  Int  wavelet_stuffing = 0x0f;
//  Int  wavelet_upload;
  Int  wavelet_uniform;  // hjlee 0901
  Int i; // hjlee 0901

  /*------- Write header info to bitstream file -------*/
  emit_bits((UShort)(STILL_TEXTURE_OBJECT_START_CODE>>16), 16);
  emit_bits((UShort)STILL_TEXTURE_OBJECT_START_CODE, 16);
  emit_bits((UShort)texture_object_id, 16);
  emit_bits((UShort)MARKER_BIT, 1);

  emit_bits((UShort)(mzte_codec.m_iWvtType==0?0:1), 1); // hjlee 0901
  emit_bits((UShort)mzte_codec.m_iWvtDownload, 1); // hjlee 0901
  emit_bits((UShort)mzte_codec.m_iWvtDecmpLev, 4); // hjlee 0901
  emit_bits((UShort)mzte_codec.m_iScanDirection,1);
  emit_bits((UShort)mzte_codec.m_bStartCodeEnable, 1);
  emit_bits((UShort)texture_object_layer_shape, 2);
  emit_bits((UShort)mzte_codec.m_iQuantType, 2);

  /* hjlee 0901 */
  if (mzte_codec.m_iQuantType==2) {
    Int i;
    emit_bits((UShort)mzte_codec.m_iSpatialLev, 4); 
	
    /* Calc number decomp layers for all spatial layers */
    if (mzte_codec.m_iSpatialLev == 1)
    {
      mzte_codec.m_lastWvtDecompInSpaLayer[0][0]=mzte_codec.m_iWvtDecmpLev-1;
    }
    else if (mzte_codec.m_iSpatialLev != mzte_codec.m_iWvtDecmpLev)
    {
      emit_bits((UShort)mzte_codec.m_defaultSpatialScale, 1);
      if (mzte_codec.m_defaultSpatialScale==0)
      {
  	     /* For the 1st spatial_scalability_levels-1 layers the luma componant
	        of lastWvtDecompInSpaLayer should have been filled in from the 
	        parameter file. */
	     for (i=0; i<mzte_codec.m_iSpatialLev-1; ++i)
	         emit_bits((UShort)mzte_codec.m_lastWvtDecompInSpaLayer[i][0], 4);

	     mzte_codec.m_lastWvtDecompInSpaLayer[mzte_codec.m_iSpatialLev-1][0]
	         =mzte_codec.m_iWvtDecmpLev-1;
      }
      else
      {
		Int sp0;

		sp0=mzte_codec.m_iWvtDecmpLev-mzte_codec.m_iSpatialLev;
		mzte_codec.m_lastWvtDecompInSpaLayer[0][0]=sp0;
		  
		for (i=1; i<mzte_codec.m_iSpatialLev; ++i)
		  mzte_codec.m_lastWvtDecompInSpaLayer[i][0]=sp0+i;
      }
    }
    else
    {
      for (i=0; i<mzte_codec.m_iSpatialLev; ++i)
		mzte_codec.m_lastWvtDecompInSpaLayer[i][0]=i;
    }
    
    /* Calculate for chroma (one less than luma) */
    for (i=0; i<mzte_codec.m_iSpatialLev; ++i)
      mzte_codec.m_lastWvtDecompInSpaLayer[i][1]
		=mzte_codec.m_lastWvtDecompInSpaLayer[i][2]
		=mzte_codec.m_lastWvtDecompInSpaLayer[i][0]-1;
  }


  if (mzte_codec.m_iWvtDownload == 1) {
	// hjlee 0901
    wavelet_uniform = (mzte_codec.m_iWvtUniform!=0)?1:0;
    emit_bits((UShort)wavelet_uniform, 1);
    if(wavelet_uniform) {
      upload_wavelet_filters(wvtfilter[0]);
    }
    else {
      for(i=0;i<mzte_codec.m_iWvtDecmpLev;i++) 
	    upload_wavelet_filters(wvtfilter[i]);	
	}
  }  

  emit_bits((UShort)wavelet_stuffing, 3);

  if (texture_object_layer_shape == 0x00) {
    emit_bits((UShort)mzte_codec.m_iRealWidth, 15);
    emit_bits((UShort)MARKER_BIT, 1);
    emit_bits((UShort)mzte_codec.m_iRealHeight, 15);
    emit_bits((UShort)MARKER_BIT, 1);
  }
  else { /* Arbitrary shape info, SL */    
    emit_bits((UShort)mzte_codec.m_iOriginX, 15);  /*horizontal_ref */    
    emit_bits((UShort)MARKER_BIT, 1); /* marker_bit */   
    emit_bits((UShort)mzte_codec.m_iOriginY, 15); /*vertical_ref */    
    emit_bits((UShort)MARKER_BIT, 1);  /* marker_bit */    
    emit_bits((UShort)mzte_codec.m_iWidth, 15);  /* object_width */    
    emit_bits((UShort)MARKER_BIT, 1);   /* marker_bit */    
    emit_bits((UShort)mzte_codec.m_iHeight, 15);  /* object_height */    
    emit_bits((UShort)MARKER_BIT, 1);  /* marker_bit */
    noteProgress("Merge Shape Bitstream ....");
    // MergeShapeBitstream();
  }
}

/**********************************************************/

/* put  quant value and maximums on the bitstream */
/* put  quant value and maximums on the bitstream */
Void CVTCEncoder::Put_Quant_and_Max(SNR_IMAGE *snr_image, Int spaLayer, Int color)
{
    /* put  quant value and maximums on the bitstream */
    put_param(snr_image->quant, 7);
//    emit_bits((UShort)MARKER_BIT, 1);  // 1124

    {
      Int l;
      for (l=0; l<=mzte_codec.m_lastWvtDecompInSpaLayer[spaLayer][color];++l)
      {
		emit_bits((UShort)snr_image->wvtDecompNumBitPlanes[l],5);
		if (((l+1) % 4) == 0)
		  emit_bits((UShort)MARKER_BIT, 1);
      }
    }


}



// hjlee 0901
Void CVTCEncoder::Put_Quant_and_Max_SQBB(SNR_IMAGE *snr_image, Int spaLayer,
				   Int color)
{
    /* put  quant value and maximums on the bitstream */
  if ((color==0 && spaLayer==0) || (color>0 && spaLayer==1))
    put_param(snr_image->quant, 7);

  if (color==0)
    emit_bits((UShort)snr_image->wvtDecompNumBitPlanes[spaLayer],5);
  else if (spaLayer)
    emit_bits((UShort)snr_image->wvtDecompNumBitPlanes[spaLayer-1],5);


}



/**********************************************************/

Void CVTCEncoder::textureLayerDC_Enc()
{
  Int col, err;

  noteProgress("Encoding DC coefficients....");

  for (col=0; col<mzte_codec.m_iColors; col++) 
  {
    /* Set global color variable */
    mzte_codec.m_iCurColor=col;

    /* initialize DC coefficient info */
    if ((err=ztqInitDC(0, col)))
      errorHandler("ztqInitDC");

    /* quantize DC coefficients */
    if ((err=encQuantizeDC(col)))
      errorHandler("encQuantizeDC");

    /* losslessly encoding DC coefficients */
    wavelet_dc_encode(col);

    // writeStats();
  }  

  noteProgress("Completed encoding DC coefficients.");
}

/**********************************************************/

Void CVTCEncoder::TextureSpatialLayerSQNSC_enc(Int spa_lev)
{
  Int col;
  SNR_IMAGE *snr_image; // hjlee 0901

  /* hjlee 0901 */
  /* hjlee 0827 */
    for (col=0; col<mzte_codec.m_iColors; col++) {
      snr_image = &(mzte_codec.m_SPlayer[col].SNRlayer.snr_image);
      Put_Quant_and_Max_SQBB(snr_image, spa_lev, col);
    }
	for (col=0; col<mzte_codec.m_iColors; col++) {
        noteProgress("Single-Quant Mode (Band by Band) - Spatial %d, SNR 0, "\
	              "Color %d",spa_lev,col);

        mzte_codec.m_iCurColor = col;
	    if (spa_lev !=0 || col ==0) {
            wavelet_higher_bands_encode_SQ_band(col);
	    }
    }

} 


Void CVTCEncoder::TextureSpatialLayerSQ_enc(Int spa_lev, FILE *bitfile)
{
	Char fname[100]; // hjlee

  /*------- AC: Open and initialize bitstream file -------*/
  if (mzte_codec.m_iSingleBitFile==0)
  {
    sprintf(fname,mzte_codec.m_cBitFileAC,spa_lev,0);
    if ((bitfile=fopen(fname,"wb"))==NULL)
      errorHandler("Can't open file '%s' for writing.",fname);
  }
  
  /* initialize the buffer */
  init_bit_packing_fp(bitfile,1); 

  /*------- AC: Write header info to bitstream file -------*/
  emit_bits(TEXTURE_SPATIAL_LAYER_START_CODE >> 16, 16);
  emit_bits(TEXTURE_SPATIAL_LAYER_START_CODE, 16);
  emit_bits(spa_lev, 5);

  TextureSpatialLayerSQNSC_enc(spa_lev);
  
  /*------- AC: Close bitstream file -------*/
  if (mzte_codec.m_iSingleBitFile)
    flush_buffer_file();
  else
    close_buffer_file(bitfile);
}


Void CVTCEncoder::textureLayerSQ_Enc(FILE *bitfile)
{
  Int col, err, spa_lev;
  SNR_IMAGE *snr_image;
    

  noteProgress("Encoding AC coefficients - Single-Quant Mode....");
  
  /*------- AC: Set spatial and SNR levels to zero -------*/
  mzte_codec.m_iCurSpatialLev = 0;
  mzte_codec.m_iCurSNRLev = 0;
    
//  mzte_codec.m_iSpatialLev=1;  // hjlee 0901
  setSpatialLayerDimsSQ(0);  // hjlee 0901
  
  for (col=0; col<mzte_codec.m_iColors; col++)
  {     
    snr_image=&(mzte_codec.m_SPlayer[col].SNRlayer.snr_image);
    
    /* Set global color variable */
    mzte_codec.m_iCurColor = col;
    snr_image->quant = mzte_codec.m_Qinfo[col][0].Quant[0];
    
    /* initialization of spatial dimensions for each color component */
    setSpatialLevelAndDimensions(0, col);
    
    /* initialize AC coefficient info for each color component */
    if ((err=ztqInitAC(0, col)))
      errorHandler("ztqInitAC");
    
    /* quantize and mark zerotree structure for AC coefficients */
    if ((err=encQuantizeAndMarkAC(col)))
      errorHandler("encQuantizeAndMarkAC");
  }
  

  /*------- AC: encode all color components -------*/
  if (mzte_codec.m_iScanDirection==0) /* tree-depth scan */
  {
    /* put  quant value and maximums on the bitstream */
    for(col=0;col<mzte_codec.m_iColors; col++){
      snr_image=&(mzte_codec.m_SPlayer[col].SNRlayer.snr_image);
      Put_Quant_and_Max(snr_image,0,col);  // hjlee 0901
    }
    
    /* losslessly encoding AC coefficients */
    wavelet_higher_bands_encode_SQ_tree();      
  }
  else  /* band by band scan */
  {
    setSpatialLayerDimsSQ(1);  // hjlee 0901

	/* Assumes all three color components have the same number of SNR 
       levels */
    for (col=0; col<mzte_codec.m_iColors; col++)
      mzte_codec.m_SPlayer[col].SNR_scalability_levels = 1;
    
    /* Loop through spatial layers */
    for (spa_lev=0; spa_lev<mzte_codec.m_iSpatialLev; 
         spa_lev++)
    {
      mzte_codec.m_iCurSpatialLev=spa_lev;
      for (col=0; col<mzte_codec.m_iColors; col++)
		setSpatialLevelAndDimensions(spa_lev,col);

      /*----- AC: Set global spatial layer. -----*/
      mzte_codec.m_iCurSpatialLev = spa_lev;
      
      /* Update spatial level coeff info if changing spatial levels.
         Do this for all color components */
      if (mzte_codec.m_bStartCodeEnable) {
		TextureSpatialLayerSQ_enc(spa_lev,bitfile);
      }
      else
		TextureSpatialLayerSQNSC_enc(spa_lev);
    }
  }
  
  /* store the max snr_lev and spa_lev so that the decoder can
     decode the bitstream up to the max level. */ 
  mzte_codec.m_iTargetSpatialLev = 1;
  mzte_codec.m_iTargetSNRLev = 1;
  
  noteProgress("Completed encoding AC coefficients - Single-Quant Mode.");
}


/**********************************************************/


Void CVTCEncoder::TextureSNRLayerMQ_encode(Int spa_lev, Int snr_lev, FILE *fp)
{
  SNR_IMAGE *snr_image;
  Int col;
  static Int texture_snr_layer_id=0;

  if(mzte_codec.m_bStartCodeEnable){
    noteProgress("Encoding Multi-Quant Mode Layer with SNR start code....");
    /* header */  
    emit_bits((UShort)texture_snr_layer_start_code>>16,16);
    emit_bits((UShort)texture_snr_layer_start_code,16);
    emit_bits((UShort)texture_snr_layer_id++,5);
  }
  else
    noteProgress("Encoding Multi-Quant Mode Layer without SNR start code....");

  noteProgress("Multi-Quant Mode - Spatial %d, SNR %d", spa_lev,snr_lev);

  for(col=0;
      col < NCOL;
      col++)
  {
    noteDetail("width=%d  height=%d",mzte_codec.m_SPlayer[col].width,
	      mzte_codec.m_SPlayer[col].height);    

    /* Set global color variable */
    mzte_codec.m_iCurColor = col;
    
    /* Set quant value */
    snr_image=&(mzte_codec.m_SPlayer[col].SNRlayer.snr_image);
    snr_image->quant = mzte_codec.m_Qinfo[col][spa_lev].Quant[snr_lev];
    noteDebug("AC quant=%d", 
               mzte_codec.m_Qinfo[col][spa_lev].Quant[snr_lev]);
        
    /* initialization of spatial dimensions for each color component */
    if (snr_lev==0) 
      setSpatialLevelAndDimensions(spa_lev, col);

    /* get maximum residual value - this one is derived from user Q inputs not 
       actual values. Also assign skip modes. */
    updateResidMaxAndAssignSkips(col);
    noteDebug("resid_max=%d\n",snr_image->residual_max);

    /* quantize and mark zerotree structure for AC coefficients */
    if (encQuantizeAndMarkAC(col))
      errorHandler("encQuantizeAndMarkAC");

     //   Put_Quant_and_Max(snr_image); // hjlee 0901
    Put_Quant_and_Max(snr_image,spa_lev,col);  // hjlee 0901
	  }

  wavelet_higher_bands_encode_MQ(mzte_codec.m_iScanDirection);    

  for(col=0;
      col < NCOL;
      col++)
  {
    /* Set global color variable */
    mzte_codec.m_iCurColor = col;
    
    /* Update states of AC coefficients */
    if (encUpdateStateAC(mzte_codec.m_iCurColor))
      errorHandler("encUpdateStateAC");
  }
}
 
Void CVTCEncoder::textureLayerMQ_Enc(FILE *bitfile)
{
  Int err, spa_lev, snr_lev=0, snr_scalability_levels;
	Char fname[100]; // hjlee

  getSpatialLayerDims(); // hjlee 0901

  // hjlee 0901
    /*------- AC: Initialize QList Structure -------*/
  if ((err=ztqQListInit()))
    errorHandler("Allocating memory for QList information.");
  
  /* Initialize coeffs */
  setSpatialLevelAndDimensions(0,0);
  if ((err=ztqInitAC(0,0)))
    errorHandler("ztqInitAC");
  
  if (mzte_codec.m_iColors > 1)
  {
    if (mzte_codec.m_lastWvtDecompInSpaLayer[0][1]<0)
      setSpatialLevelAndDimensions(1,1);
    else
      setSpatialLevelAndDimensions(0,1);
    if ((err=ztqInitAC(0,1)))
      errorHandler("ztqInitAC");
  }
  if (mzte_codec.m_iColors > 2)
  {
    if (mzte_codec.m_lastWvtDecompInSpaLayer[0][2]<0)
      setSpatialLevelAndDimensions(1,2);
    else
      setSpatialLevelAndDimensions(0,2);
    if ((err=ztqInitAC(0,2)))
      errorHandler("ztqInitAC");
  }


  /* Loop through spatial layers */
  for (spa_lev=0; spa_lev<mzte_codec.m_iSpatialLev; 
       spa_lev++)
  {
    /*----- AC: Set global spatial layer and SNR scalability level. -----*/
    /* Assumes all three color components have the same number of SNR 
       levels */
    mzte_codec.m_iCurSpatialLev = spa_lev;
    mzte_codec.m_SPlayer[0].SNR_scalability_levels = 
      mzte_codec.m_Qinfo[0][spa_lev].SNR_scalability_levels;
    snr_scalability_levels = mzte_codec.m_SPlayer[0].SNR_scalability_levels;
    
    /* Update spatial level coeff info if changing spatial levels.
       Do this for all color components. */
    if (spa_lev != 0)
    {
      for (mzte_codec.m_iCurColor = 0; mzte_codec.m_iCurColor<mzte_codec.m_iColors;
	   mzte_codec.m_iCurColor++) 
      {
		setSpatialLevelAndDimensions(mzte_codec.m_iCurSpatialLev, 
						 mzte_codec.m_iCurColor);

// hjlee 0901
		if (mzte_codec.m_lastWvtDecompInSpaLayer[spa_lev-1][mzte_codec.m_iCurColor]
			>=0)
		  spatialLayerChangeUpdate(mzte_codec.m_iCurColor);

      }
    }
    
    if (!mzte_codec.m_bStartCodeEnable)
      /*------- AC: Write header info to bitstream file -------*/
      emit_bits(snr_scalability_levels, 5);    
    
    /* Loop through SNR layers */      
    for (snr_lev=0; snr_lev<snr_scalability_levels; snr_lev++) 
    {
      /*----- AC: Set global SNR layer -----*/
      mzte_codec.m_iCurSNRLev = snr_lev;
      
      if (mzte_codec.m_bStartCodeEnable)
      {
	/*------- AC: Open and initialize bitstream file -------*/
	if (mzte_codec.m_iSingleBitFile==0)
	{
	  sprintf(fname,mzte_codec.m_cBitFileAC,
		  mzte_codec.m_iCurSpatialLev, mzte_codec.m_iCurSNRLev);
	  if ((bitfile=fopen(fname,"wb"))==NULL)
	    errorHandler("Can't open file '%s' for writing.",fname);
	}
	
	/* initialize the buffer */
	init_bit_packing_fp(bitfile,1);
	
	if (snr_lev==0) {
	  /*------- AC: Write header info to bitstream file -------*/
	  emit_bits(TEXTURE_SPATIAL_LAYER_START_CODE >> 16, 16);
	  emit_bits(TEXTURE_SPATIAL_LAYER_START_CODE, 16);
	  emit_bits(spa_lev, 5);
	  emit_bits(snr_scalability_levels, 5);
	  flush_bits();     /* byte alignment before start code */
	}
	
      }
      
      /*------- AC: Quantize and encode all color components -------*/
      TextureSNRLayerMQ_encode(spa_lev, snr_lev, bitfile);
      if (mzte_codec.m_bStartCodeEnable)
      {
		if (mzte_codec.m_iSingleBitFile)
			flush_buffer_file();
		else
			close_buffer_file(bitfile);
      }
      
      
    } /* snr_lev */
    
  }  /* spa_lev */
  
  /* store the max snr_lev and spa_lev so that the decoder can
     decode the bitstream up to the max level. */ 
  mzte_codec.m_iTargetSpatialLev = spa_lev;
  mzte_codec.m_iTargetSNRLev = snr_lev;
  
  /*------- AC: Free Qlist structure -------*/
  ztqQListExit();
  
}

/**********************************************************/

Void CVTCEncoder::TextureObjectLayer_enc(FILTER **wvtfilter)
//Void CVTCEncoder::TextureObjectLayer_enc(FILTER *wvtfilter)  // hjlee 0901
{
  FILE *bitfile;

  /*------- DC: Open and initialize bitstream file -------*/
  if ((bitfile=fopen(mzte_codec.m_cBitFile,"wb"))==NULL)
    errorHandler("Can't open file '%s' for writing.",
		mzte_codec.m_cBitFile);

  /* for PEZW, always enabled */
  if (mzte_codec.m_iQuantType == BILEVEL_Q) 
    mzte_codec.m_bStartCodeEnable = 1;

  /* initialize variables */
  init_bit_packing_fp(bitfile,1);

  /* Write header info to bitstream */
  header_Enc(wvtfilter);

  /*------- DC: Quantize and encode all color components -------*/
  textureLayerDC_Enc();

  /*------- DC: Close bitstream file -------*/
  /* hjlee 001 */
  if (mzte_codec.m_bStartCodeEnable){
     if(mzte_codec.m_iSingleBitFile) 
        flush_buffer_file(); 
     else  
       close_buffer_file(bitfile); 
  }


  /*-------------------------------------------------*/
  /*--------------------- AC ------------------------*/
  /*-------------------------------------------------*/

  /*------- AC: SINGLE-QUANT MODE -------*/
  if (mzte_codec.m_iQuantType == SINGLE_Q)
    textureLayerSQ_Enc(bitfile);
  /*------- AC: MULTI-QUANT MODE -------*/
  else if (mzte_codec.m_iQuantType == MULTIPLE_Q)
    textureLayerMQ_Enc(bitfile);
  else if (mzte_codec.m_iQuantType == BILEVEL_Q) {
	  textureLayerBQ_Enc(bitfile);
  }

  if (mzte_codec.m_iSingleBitFile){
    if(!mzte_codec.m_bStartCodeEnable)
      close_buffer_file(bitfile);
    else
      fclose(bitfile);
  }



}


/**************************************************************/

Void CVTCEncoder::encode()
{

	FILTER **synfilter,**anafilter;  // hjlee 0901
	Int i; // hjlee 0901

	// allocate memory for source image
	mzte_codec.m_Image = new PICTURE[3];

	// hjlee 0901
	  anafilter = (FILTER **) malloc(sizeof(FILTER *)*mzte_codec.m_iWvtDecmpLev);
	  synfilter = (FILTER **) malloc(sizeof(FILTER *)*mzte_codec.m_iWvtDecmpLev);
	  if(anafilter == NULL || synfilter == NULL) 
		errorHandler("Error allocating memory for filters\n");
	  for(i=0;i<mzte_codec.m_iWvtDecmpLev; i++) {
		choose_wavelet_filter(&(anafilter[i]),
				  &(synfilter[mzte_codec.m_iWvtDecmpLev-1-i]),
				  mzte_codec.m_WvtFilters[mzte_codec.m_iWvtUniform?0:i]);
	  }
	  
	  // read source image
	read_image(	m_cImagePath,
				mzte_codec.m_iWidth,
				mzte_codec.m_iHeight,
				mzte_codec.m_iColors,
				8,
				mzte_codec.m_Image
				);

	if (mzte_codec.m_iAlphaChannel != 0) {  // arbitrary shape coding

		// allocate meory for segmentation
		mzte_codec.m_SegImage = new PICTURE[3];

		// read the segmentation map of the source image */
		mzte_codec.m_iAlphaChannel = read_segimage(m_cSegImagePath, 
										  mzte_codec.m_Image[0].width, 
										  mzte_codec.m_Image[0].height, 
										  mzte_codec.m_iColors, 
										  mzte_codec.m_Image); 

	}


	get_virtual_image(mzte_codec.m_Image,
					mzte_codec.m_iWvtDecmpLev,
					mzte_codec.m_iAlphaChannel,
					mzte_codec.m_iColors,
					mzte_codec.m_iAlphaTh,
					mzte_codec.m_iChangeCRDisable,
					anafilter[0]);
	int col;
	for (/*int*/ col=0; col<mzte_codec.m_iColors; col++) {
		mzte_codec.m_Image[col].height = mzte_codec.m_iHeight >> (Int)(col>0);
		mzte_codec.m_Image[col].width  = mzte_codec.m_iWidth >> (Int)(col>0);
	}
	mzte_codec.m_iAcmOrder = 0;
	mzte_codec.m_iAcmMaxFreqChg = 0;

	init_acm_maxf_enc();

//	fprintf(stdout,"init ac model!\n");	
	
	int height;
	int width;
	for (col=0; col<mzte_codec.m_iColors; col++) {
		height = mzte_codec.m_Image[col].height; 
		width  = mzte_codec.m_Image[col].width;
    
		mzte_codec.m_SPlayer[col].coeffinfo = new COEFFINFO * [height];
		if (mzte_codec.m_SPlayer[col].coeffinfo == NULL)
			exit(fprintf(stderr,"Allocating memory for coefficient structure (I)."));
		mzte_codec.m_SPlayer[col].coeffinfo[0] = new COEFFINFO [height*width];
		if (mzte_codec.m_SPlayer[col].coeffinfo[0] == NULL)
			exit(fprintf(stderr,"Allocating memory for coefficient structure (II)."));
		int y;
		for (/* int*/ y = 1; y < height; ++y)
		  mzte_codec.m_SPlayer[col].coeffinfo[y] = 
			mzte_codec.m_SPlayer[col].coeffinfo[y-1]+width;
		
		for (y=0; y<height; y++)
			for (int x=0; x<width; x++) {
				mzte_codec.m_SPlayer[col].coeffinfo[y][x].skip =0;
			}

	}

//	fprintf(stdout,"Coeffinfo memory allocation done!\n");

	/* DISCRETE WAVELET TRANSFORM */
	noteProgress("Wavelet Transform....");  



// 	choose_wavelet_filter(&anafilter, &synfilter, mzte_codec.m_iWvtType); // hjlee 0901
	perform_DWT(anafilter);
	noteProgress("Completed wavelet transform.");


	TextureObjectLayer_enc(synfilter); 

	/*----- free up coeff data structure -----*/
	noteDetail("Freeing up encoding data structures....");
	for (col=0; col<mzte_codec.m_iColors; col++) {
		if (mzte_codec.m_SPlayer[col].coeffinfo[0] != NULL)
			delete (mzte_codec.m_SPlayer[col].coeffinfo[0]);
		mzte_codec.m_SPlayer[col].coeffinfo[0] = NULL;
		if (mzte_codec.m_SPlayer[col].coeffinfo)
			delete (mzte_codec.m_SPlayer[col].coeffinfo);
		mzte_codec.m_SPlayer[col].coeffinfo = NULL;
	}
	noteDetail("Completed freeing up encoding data structures.");

	noteProgress("\n----- Encoding Completed. -----\n");
  
}



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

#ifdef _VTC_DECODER_
#define DEFINE_GLOBALS
#endif


#include "basic.hpp"
#include "dataStruct.hpp"
#include "startcode.hpp"
#include "bitpack.hpp"
#include "msg.hpp"
#include "globals.hpp"

extern FILTER DefaultSynthesisFilterInt;
extern FILTER DefaultSynthesisFilterDbl;


CVTCDecoder::CVTCDecoder()
{
  m_cInBitsFile = new Char [80];
  m_cRecImageFile = new Char [80];
}

CVTCDecoder::~CVTCDecoder()
{
  delete m_cInBitsFile;
  delete m_cRecImageFile;
}



void CVTCCommon::getSpatialLayerDims()
{
  Int i, shift;

  /* Calc widths and heights for all spatial layers */
  for (i=0; i<mzte_codec.m_iSpatialLev; ++i)
  {
    shift=mzte_codec.m_iWvtDecmpLev-mzte_codec.m_lastWvtDecompInSpaLayer[i][0]-1;
    mzte_codec.m_spaLayerWidth[i][0]=mzte_codec.m_iWidth>>shift;
    mzte_codec.m_spaLayerHeight[i][0]=mzte_codec.m_iHeight>>shift;

    if (mzte_codec.m_lastWvtDecompInSpaLayer[i][1]<0)
    {
       mzte_codec.m_spaLayerWidth[i][1]=mzte_codec.m_iDCWidth;
       mzte_codec.m_spaLayerHeight[i][1]=mzte_codec.m_iDCHeight;
    }
    else
    {
      shift=mzte_codec.m_iWvtDecmpLev-mzte_codec.m_lastWvtDecompInSpaLayer[i][1]-1;
      mzte_codec.m_spaLayerWidth[i][1]=mzte_codec.m_iWidth>>shift;
      mzte_codec.m_spaLayerHeight[i][1]=mzte_codec.m_iHeight>>shift;
    }

    if (mzte_codec.m_lastWvtDecompInSpaLayer[i][2]<0)
    {
       mzte_codec.m_spaLayerWidth[i][2]=mzte_codec.m_iDCWidth;
       mzte_codec.m_spaLayerHeight[i][2]=mzte_codec.m_iDCHeight;
    }
    else
    {
      shift=mzte_codec.m_iWvtDecmpLev-mzte_codec.m_lastWvtDecompInSpaLayer[i][2]-1;
      mzte_codec.m_spaLayerWidth[i][2]=mzte_codec.m_iWidth>>shift;
      mzte_codec.m_spaLayerHeight[i][2]=mzte_codec.m_iHeight>>shift;
    }
  }
}

void CVTCCommon::setSpatialLayerDimsSQ(Int band)
{
  /* added for compatability for new MQ spatial layer flexability - ph 7/16 */
  Int i;
  
  if (band) /* BAND - wvtDecompLev spatial layers */
  {
    for (i=0; i<mzte_codec.m_iWvtDecmpLev; ++i)
    {
      mzte_codec.m_lastWvtDecompInSpaLayer[i][0] = i;
      mzte_codec.m_lastWvtDecompInSpaLayer[i][1]
	=mzte_codec.m_lastWvtDecompInSpaLayer[i][2] = i-1;
    }

    mzte_codec.m_iSpatialLev=mzte_codec.m_iWvtDecmpLev;
  }
  else /* TREE - 1 spatial layer */
  {
    mzte_codec.m_lastWvtDecompInSpaLayer[0][0] = mzte_codec.m_iWvtDecmpLev-1;
    mzte_codec.m_lastWvtDecompInSpaLayer[0][1]
      =mzte_codec.m_lastWvtDecompInSpaLayer[0][2] = mzte_codec.m_iWvtDecmpLev-2;

    mzte_codec.m_iSpatialLev=1;
  }

  getSpatialLayerDims();
}



/* Number of bitplanes required to x different values 
   (i.e. all values in {0,1, ...,x-1} */
Int CVTCCommon::ceilLog2(Int x)
{
  Int i;

  for (i=0; x > (1<<i); ++i)
    ;
  
  return i;
}


// hjlee 0901
Void CVTCDecoder::header_Dec(FILTER ***wvtfilter, PICTURE **Image)
{
  Int  texture_object_id;
  Int  marker_bit;
  Int  wavelet_download;
  Int  texture_object_layer_shape;
  Int  wavelet_stuffing;
  Int  still_texture_object_start_code;
  Int i; // hjlee 0901
  FILTER **filters; // hjlee 0901
  Int  wavelet_uniform=1; // hjlee 0901
  Int  wavelet_type; // hjlee 0901

  still_texture_object_start_code = get_X_bits(32);
  if (still_texture_object_start_code != STILL_TEXTURE_OBJECT_START_CODE)
    errorHandler("Wrong texture_object_layer_start_code.");

  texture_object_id = get_X_bits(16);
  marker_bit = get_X_bits(1);

// hjlee 0901
//  wvtfilter->DWT_Type = get_X_bits(1);
//  wavelet_download = get_X_bits(1);
  mzte_codec.m_iWvtType = wavelet_type = get_X_bits(1); // hjlee 0901
  wavelet_download= mzte_codec.m_iWvtDownload = get_X_bits(1); // hjlee 0901
  mzte_codec.m_iWvtDecmpLev = get_X_bits(4); // hjlee 0901
  mzte_codec.m_iScanDirection=get_X_bits(1);
  mzte_codec.m_bStartCodeEnable = get_X_bits(1);
//   mzte_codec.m_iWvtDecmpLev = get_X_bits(8);  // hjlee 0901
  texture_object_layer_shape = get_X_bits(2);
  mzte_codec.m_iQuantType = get_X_bits(2); 


  if (mzte_codec.m_iQuantType==2) /* MQ */
  {
    Int i;

    mzte_codec.m_iSpatialLev = get_X_bits(4);
    /* Get/Calc number decomp layers for all spatial layers */
    if (mzte_codec.m_iSpatialLev == 1)
    {
      mzte_codec.m_lastWvtDecompInSpaLayer[0][0]=mzte_codec.m_iWvtDecmpLev-1;
    }
    else if (mzte_codec.m_iSpatialLev != mzte_codec.m_iWvtDecmpLev)
    {
      mzte_codec.m_defaultSpatialScale = get_X_bits(1);
      if (mzte_codec.m_defaultSpatialScale==0)
      {
	/* Fill in the luma componant of lastWvtDecompInSpaLayer. */
	for (i=0; i<mzte_codec.m_iSpatialLev-1; ++i)
	  mzte_codec.m_lastWvtDecompInSpaLayer[i][0]=get_X_bits(4);
	
	mzte_codec.m_lastWvtDecompInSpaLayer\
	  [mzte_codec.m_iSpatialLev-1][0]
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


// hjlee 0901
  filters = (FILTER **)malloc(sizeof(FILTER *)*mzte_codec.m_iWvtDecmpLev);
  if(filters==NULL)  
    errorHandler("Memory allocation error\n");
  if (wavelet_download == 1) {
    mzte_codec.m_iWvtUniform=wavelet_uniform = get_X_bits(1);
    if(wavelet_uniform) {
      download_wavelet_filters(&(filters[0]), wavelet_type);
    }
    else {
      for(i=0;i< mzte_codec.m_iWvtDecmpLev; i++) {
	download_wavelet_filters(&(filters[mzte_codec.m_iWvtDecmpLev-1-i]), wavelet_type);
      }
    }
  }
  else if(wavelet_type==0){
    mzte_codec.m_iWvtType = 0;
    filters[0]=&DefaultSynthesisFilterInt;
  }
  else{
    mzte_codec.m_iWvtType = 1;
    filters[0]=&DefaultSynthesisFilterDbl;
  }
  if(wavelet_uniform) {
     for(i=1;i< mzte_codec.m_iWvtDecmpLev; i++) {
       filters[i] = filters[0];
     }
  }
  *wvtfilter = filters;


  wavelet_stuffing = get_X_bits(3);

  if (texture_object_layer_shape==0) {
    mzte_codec.m_iAlphaChannel       = 0; 
    mzte_codec.m_iWidth = get_X_bits(15);
    marker_bit = get_X_bits(1);
    mzte_codec.m_iHeight = get_X_bits(15);
    marker_bit = get_X_bits(1);
  }
  else { /* Arbitrary shape header */
    mzte_codec.m_iAlphaChannel       = 1; 
    mzte_codec.m_iOriginX = get_X_bits(15);
    marker_bit = get_X_bits(1);
    mzte_codec.m_iOriginY = get_X_bits(15);
    marker_bit = get_X_bits(1);
    mzte_codec.m_iWidth = get_X_bits(15);
    marker_bit = get_X_bits(1);
    mzte_codec.m_iHeight = get_X_bits(15);
    marker_bit = get_X_bits(1);
    mzte_codec.m_iRealWidth = mzte_codec.m_iWidth;
    mzte_codec.m_iRealHeight = mzte_codec.m_iHeight;
  }

  /* decode the shape info from bitstream */
  if(mzte_codec.m_iAlphaChannel)
    noteProgress("Decoding Shape Information...");
  *Image = (PICTURE *)malloc(sizeof(PICTURE)*3);
  

  get_virtual_mask(*Image, mzte_codec.m_iWvtDecmpLev,
		   mzte_codec.m_iWidth, mzte_codec.m_iHeight, 
		   mzte_codec.m_iAlphaChannel, mzte_codec.m_iColors,
		   filters);
}


/* Read quant value from bitstream */
Void CVTCDecoder::Get_Quant_and_Max(SNR_IMAGE *snr_image, Int spaLayer, Int color)


{
//  Int marker_bit;

  snr_image->quant     = get_param(7);
//  marker_bit = get_X_bits(1);  // 1124
 // if (marker_bit != 1)
//	  noteError("marker_bit should be one");

    {
      Int l;
      
      for (l=0; l<=mzte_codec.m_lastWvtDecompInSpaLayer[spaLayer][color];++l)
      {
	snr_image->wvtDecompNumBitPlanes[l] = get_X_bits(5);

	if (((l+1) % 4) == 0)
	  get_X_bits(1);
      }
    }
}

Void CVTCDecoder::Get_Quant_and_Max_SQBB(SNR_IMAGE *snr_image, Int spaLayer, 
				   Int color)
{
 // Int marker_bit;

  if ((color==0 && spaLayer==0) || (color>0 && spaLayer==1))
    snr_image->quant = get_param(7);

  if (color==0)
    snr_image->wvtDecompNumBitPlanes[spaLayer] = get_X_bits(5);
  else if (spaLayer)
    snr_image->wvtDecompNumBitPlanes[spaLayer-1] = get_X_bits(5);


}



Void CVTCDecoder::textureLayerDC_Dec()
{
  Int col, err;

  noteProgress("Decoding DC coefficients....");
  for (col=0; col<mzte_codec.m_iColors; col++) 
  {
    /* initilize all wavelet coefficients */
    mzte_codec.m_iCurColor=col;
    err=ztqInitDC(1, col);

    /* losslessly decoding DC coefficients */
    wavelet_dc_decode(col);

    /* dequantize DC coefficients */
    err=decIQuantizeDC(col);
  }  
  noteProgress("Completed decoding of DC coefficients.");
}


/*******************************************************
  The following two routines are for single quant 
  band by band scan order.
*******************************************************/
Void CVTCDecoder::TextureSpatialLayerSQNSC_dec(Int spa_lev)
{
  Int col;

    /* hjlee 0901 */
   SNR_IMAGE *snr_image;

  /* hjlee 0901 */
    for (col=0; col<mzte_codec.m_iColors; col++) {
      snr_image = &(mzte_codec.m_SPlayer[col].SNRlayer.snr_image);
      Get_Quant_and_Max_SQBB(snr_image,spa_lev, col);
    }
 
  for (col=0; col<mzte_codec.m_iColors; col++){
    noteProgress("Single-Quant Mode (Band by Band) - Spatial %d, SNR 0, "\
	       "Color %d",spa_lev,col); fflush(stderr);

    mzte_codec.m_iCurColor = col;
    if(spa_lev !=0 || col == 0){
      wavelet_higher_bands_decode_SQ_band(col);
      if(decIQuantizeAC_spa(spa_lev,col)) 
	errorHandler("decIQuantizeAC_spa");
    }
  }
  
} 



Void CVTCDecoder::TextureSpatialLayerSQ_dec(Int spa_lev,FILE *bitfile)
{
  Int texture_spatial_layer_start_code,texture_spatial_layer_id;
  Char fname[100]; // hjlee

  /*------- AC: Open and initialize bitstream file -------*/
  if (mzte_codec.m_iSingleBitFile==0)
  {
    sprintf(fname,mzte_codec.m_cBitFileAC,spa_lev,0);
    if ((bitfile=fopen(fname,"rb"))==NULL)
      errorHandler("Can't open file '%s' for reading.",fname);
    init_bit_packing_fp(bitfile,1);
  }
  else
    init_bit_packing_fp(bitfile,0);

  /* header info */
  texture_spatial_layer_start_code = get_X_bits(32);
  if (texture_spatial_layer_start_code != 
      TEXTURE_SPATIAL_LAYER_START_CODE)
    errorHandler("Wrong texture_spatial_layer_start_code %x.",
		 texture_spatial_layer_start_code);
  
  texture_spatial_layer_id = get_X_bits(5);
  if (texture_spatial_layer_id != spa_lev)
    errorHandler("Incorrect texture_spatial_layer_id");
  mzte_codec.m_SPlayer[0].SNR_scalability_levels = 1;

  TextureSpatialLayerSQNSC_dec(spa_lev);

  align_byte();
  if(mzte_codec.m_iSingleBitFile==0)
    fclose(bitfile);
}


Void CVTCDecoder::textureLayerSQ_Dec(FILE *bitfile)
{
  Int col, err, spa_lev;
  SNR_IMAGE *snr_image;

  noteProgress("Decoding AC coefficients - Single-Quant Mode....");
   
  /* added for compatability with MQ spatial layer flexability - ph 7/16 */
  setSpatialLayerDimsSQ(0);  // hjlee 0901
  
  /*------- AC: Set spatial and SNR levels to zero -------*/
  mzte_codec.m_iCurSpatialLev = 0;
  mzte_codec.m_iCurSNRLev = 0;
  
  for (col=0; col<mzte_codec.m_iColors; col++) 
  {     
    
    /* initialization of spatial dimensions for each color component */
    setSpatialLevelAndDimensions(0, col);
    
    /* initialize AC coefficient info */
    if ((err=ztqInitAC(1, col)))
      errorHandler("ztqInitAC");
    
    snr_image=&(mzte_codec.m_SPlayer[col].SNRlayer.snr_image);
  }
  
  
  /*------- AC: Decode and inverse quantize all color components -------*/
  if (mzte_codec.m_iScanDirection==0) /* tree-depth scan */
  {  
    /* Read quant value from bitstream */
    for(col=0;col<mzte_codec.m_iColors;col++){
      snr_image=&(mzte_codec.m_SPlayer[col].SNRlayer.snr_image);
      Get_Quant_and_Max(snr_image,0,col); // hjlee 0901
    }

    /* losslessly decoding AC coefficients */
    wavelet_higher_bands_decode_SQ_tree();
    
    for (col=0; col<mzte_codec.m_iColors; col++) 
    {  	
      /* Inverse quantize AC coefficients */
      if ((err=decIQuantizeAC(col)))
	errorHandler("decIQuantizeAC");
    }
    
  }
  else  /* SQ band by band */
  {

    /* added for compatability with MQ spatial layer flexability - ph 7/16 */
    setSpatialLayerDimsSQ(1);  // hjlee 0901

    /* Assumes all three color components have the same number of SNR
       levels */
    for (col=0; col<mzte_codec.m_iColors; col++)
      mzte_codec.m_SPlayer[col].SNR_scalability_levels = 1;

    /* Loop through spatial layers */
    for (spa_lev=0; spa_lev<mzte_codec.m_iSpatialLev; 
         spa_lev++)
    {
      for (col=0; col<mzte_codec.m_iColors; col++)
	setSpatialLevelAndDimensions(spa_lev, col);

      /*----- AC: Set global spatial layer. -----*/
      mzte_codec.m_iCurSpatialLev = spa_lev;
      
      /* Update spatial level coeff info if changing spatial levels.
         Do this for all color components */
      if (mzte_codec.m_bStartCodeEnable) {
			TextureSpatialLayerSQ_dec(spa_lev,bitfile);
      }
      else
			TextureSpatialLayerSQNSC_dec(spa_lev);
    }
  }
  
  noteProgress("Completed decoding AC coefficients - Single-Quant Mode.");
}


Void CVTCDecoder::TextureSNRLayerMQ_decode(Int spa_lev, Int snr_lev,FILE *fp)
{
  SNR_IMAGE *snr_image;
  Int col;
  Int texture_snr_layer_id;

  mzte_codec.m_iCurSpatialLev=spa_lev;

  if(mzte_codec.m_bStartCodeEnable){
    noteProgress("Decoding Multi-Quant Mode Layer with SNR start code....");
    /* header info */
    if(get_X_bits(32) != texture_snr_layer_start_code)
      errorHandler("Error in decoding texture_snr_layer_start_code");
    texture_snr_layer_id=get_X_bits(5);
  }
  else 
    noteProgress("Decoding Multi-Quant Mode Layer without SNR start code....");

  noteProgress("Multi-Quant Mode - Spatial %d, SNR %d", spa_lev,snr_lev);

  for(col=0;
      col < NCOL;
      col++)
  {        
    /* Set global color variable */
    mzte_codec.m_iCurColor = col;
    
    /* initialization of spatial dimensions for each color component  */
    setSpatialLevelAndDimensions(mzte_codec.m_iCurSpatialLev, col);
    
    snr_image=&(mzte_codec.m_SPlayer[col].SNRlayer.snr_image);

    Get_Quant_and_Max(snr_image,spa_lev,col); // hjlee 0901

    updateResidMaxAndAssignSkips(col);
    noteDebug("resid_max=%d\n",snr_image->residual_max);
  }

  wavelet_higher_bands_decode_MQ(mzte_codec.m_iScanDirection);
 
  for(col=0;
      col < NCOL;      
      col++)
  {        
    /* Set global color variable */
    mzte_codec.m_iCurColor = col;

    /* quantize and mark zerotree structure for AC coefficients */
    if (decIQuantizeAC(col))
      errorHandler("decQuantizeAndMarkAC");

    noteDebug("max_root=%d max_valz=%d max_valnz=%d max_resi=%d",
	      ROOT_MAX(col),VALZ_MAX(col),VALNZ_MAX(col),
	      RESID_MAX(col));
    
    /* Update states of ac coefficients */
    if (decUpdateStateAC(col))
      errorHandler("decUpdateStateAC");
  }
}

#define SNR_INFINITY 99

// hjlee 0901
Void CVTCDecoder::textureLayerMQ_Dec(FILE *bitfile, 
			       Int  target_spatial_levels,
			       Int  target_snr_levels,
			       FILTER **wvtfilter)

{
  Int  err, spa_lev, snr_lev, snr_scalability_levels;
  Int  texture_spatial_layer_start_code;
  Int  texture_spatial_layer_id;
  Char fname[100];

// hjlee 0901

  /* added for spatial layer flexability - ph 7/16 */
  getSpatialLayerDims();  // hjlee 0901


  /*------- AC: Initialize QList Structure -------*/
  if ((err=ztqQListInit()))
    errorHandler("Allocating memory for QList information.");

// hjlee 0901
  /* Initialize coeffs */
  setSpatialLevelAndDimensions(0,0);
  if ((err=ztqInitAC(1,0)))
    errorHandler("ztqInitAC");
  
  if (mzte_codec.m_lastWvtDecompInSpaLayer[0][1]<0)
    setSpatialLevelAndDimensions(1,1);
  else
    setSpatialLevelAndDimensions(0,1);
  if ((err=ztqInitAC(1,1)))
    errorHandler("ztqInitAC");
  
  if (mzte_codec.m_lastWvtDecompInSpaLayer[0][2]<0)
    setSpatialLevelAndDimensions(1,2);
  else
    setSpatialLevelAndDimensions(0,2);
  if ((err=ztqInitAC(1,2)))
    errorHandler("ztqInitAC");


  /* Loop through spatial layers */
  target_spatial_levels=MIN(mzte_codec.m_iSpatialLev,
			    target_spatial_levels);
  for (spa_lev=0; spa_lev<target_spatial_levels; spa_lev++)
  {
    /*----- AC: Set global spatial layer and SNR scalability level. -----*/
    /* Assumes all three color components have the same number of SNR 
       levels */
    mzte_codec.m_iCurSpatialLev = spa_lev;
    mzte_codec.m_SPlayer[0].SNR_scalability_levels = SNR_INFINITY;
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
    {
      snr_scalability_levels = get_X_bits(5);  
      mzte_codec.m_SPlayer[0].SNR_scalability_levels = snr_scalability_levels;
    }

    /* Loop through SNR layers */      
    for (snr_lev=0; snr_lev<snr_scalability_levels; snr_lev++) 
    {
      if (spa_lev==target_spatial_levels-1 && snr_lev==target_snr_levels)
	break;

      /*----- AC: Set global SNR layer -----*/
      mzte_codec.m_iCurSNRLev = snr_lev;
             
      if (mzte_codec.m_bStartCodeEnable)
      {
		/*------- AC: Open and initialize bitstream file -------*/
		if (mzte_codec.m_iSingleBitFile==0)
		{
			sprintf(fname,mzte_codec.m_cBitFileAC,
			  mzte_codec.m_iCurSpatialLev,mzte_codec.m_iCurSNRLev);
		    if ((bitfile=fopen(fname,"rb"))==NULL)
			 errorHandler("Can't open file '%s' for reading.",fname);
	  
			/* initialize the buffer */
			init_bit_packing_fp(bitfile,1);
		}
		else
			init_bit_packing_fp(bitfile,0);
       	
		if (snr_lev==0) {
			 /*------- AC: Read header info to bitstream file -------*/
			 texture_spatial_layer_start_code = get_X_bits(32);
			 if (texture_spatial_layer_start_code != 
				  TEXTURE_SPATIAL_LAYER_START_CODE)
				 errorHandler("Wrong texture_spatial_layer_start_code3 %x.",
						 texture_spatial_layer_start_code);
	  
			 texture_spatial_layer_id = get_X_bits(5);
			 if(texture_spatial_layer_id !=spa_lev)
				errorHandler("Incorrect texture_spatial_layer_id");
			 snr_scalability_levels = get_X_bits(5);  
			 mzte_codec.m_SPlayer[0].SNR_scalability_levels = 
						snr_scalability_levels;
			 align_byte();  /* byte alignment before start code */
		}	
	
      }
      
      /*------- AC: Decode and inverse quantize all color components ------*/
      TextureSNRLayerMQ_decode(spa_lev, snr_lev, bitfile);   
      if (mzte_codec.m_bStartCodeEnable){
		align_byte();  /* byte alignment before start code */
		if (mzte_codec.m_iSingleBitFile==0)
		 fclose(bitfile);
      }
      
    } /* snr_lev */
    
  }  /* spa_lev */
 
  /*------- AC: Free Qlist structure -------*/
  ztqQListExit();
}

// hjlee 0901
Void CVTCDecoder::TextureObjectLayer_dec(
				   Int  target_spatial_levels,
				   Int  target_snr_levels, 
				   FILTER ***pwvtfilter)
{
  FILE *bitfile;
  Int  x,y,w,h,k;
  FILTER **wvtfilter;  // hjlee 0901
  UChar *inmask[3], *outmask[3];
  Int nLevels[3], ret;
  Int Width[3], Height[3];
  Int Nx[3], Ny[3];
  Int usemask;
  Int useInt=1;
  PICTURE *Image;
  Int col,l;


  /*-------------------------------------------------*/
  /*--------- DC (and overall header info) ----------*/
  /*-------------------------------------------------*/

  /*------- DC: Open and initialize bitstream file -------*/
  if ((bitfile=fopen(m_cInBitsFile,"rb"))==NULL)
    errorHandler("Can't open file '%s' for reading.",m_cInBitsFile);

  /* initialize variables */
  init_bit_packing_fp(bitfile,1);

  /*------- DC: Read header info from bitstream file -------*/
  header_Dec(pwvtfilter, &Image); // hjlee 0901
  wvtfilter = *pwvtfilter; // hjlee 0901

  /*--------------- CREATE DATA STRUCTURES -----------------*/
  noteDetail("Creating and initializing data structures....");
  mzte_codec.m_iColors = 3;
  mzte_codec.m_iBitDepth = 8;  
  usemask = mzte_codec.m_iAlphaChannel = 0;
  useInt =1;

  init_acm_maxf_dec();  // hjlee 0901 
  for (col=0; col<mzte_codec.m_iColors; col++) // hjlee 0901 BUG
	  for (l=0; l<mzte_codec.m_iWvtDecmpLev; l++) {
	    mzte_codec.m_SPlayer[col].SNRlayer.snr_image.wvtDecompNumBitPlanes[l] = 0;
//	    mzte_codec.m_SPlayer[col].SNRlayer.snr_image.wvtDecompResNumBitPlanes[l] = 0;
	  }


  
  for (col=0; col<mzte_codec.m_iColors; col++) {

    h = mzte_codec.m_iHeight >> (Int)(col>0);
    w  = mzte_codec.m_iWidth  >> (Int)(col>0);

		mzte_codec.m_SPlayer[col].coeffinfo = new COEFFINFO * [h];
		if (mzte_codec.m_SPlayer[col].coeffinfo == NULL)
			exit(fprintf(stderr,"Allocating memory for coefficient structure (I)."));
		mzte_codec.m_SPlayer[col].coeffinfo[0] = new COEFFINFO [h*w];
		if (mzte_codec.m_SPlayer[col].coeffinfo[0] == NULL)
			exit(fprintf(stderr,"Allocating memory for coefficient structure (II)."));
		for (/*int*/ y = 1; y < h; ++y)
		  mzte_codec.m_SPlayer[col].coeffinfo[y] = 
			mzte_codec.m_SPlayer[col].coeffinfo[y-1]+w;
		
		for (y=0; y<h; y++)
			for (int x=0; x<w; x++) {
				mzte_codec.m_SPlayer[col].coeffinfo[y][x].skip =0;
				mzte_codec.m_SPlayer[col].coeffinfo[y][x].wvt_coeff = 0;
				mzte_codec.m_SPlayer[col].coeffinfo[y][x].rec_coeff = 0;
				mzte_codec.m_SPlayer[col].coeffinfo[y][x].quantized_value = 0;
				// mzte_codec.m_SPlayer[col].coeffinfo[y][x].qState = 0;
				mzte_codec.m_SPlayer[col].coeffinfo[y][x].type = 0;
				mzte_codec.m_SPlayer[col].coeffinfo[y][x].mask = 0;

			}


  }
  noteDetail("Completed creating and initializing data structures.");

  mzte_codec.m_iDCHeight  = mzte_codec.m_iHeight >> mzte_codec.m_iWvtDecmpLev;
  mzte_codec.m_iDCWidth   = mzte_codec.m_iWidth >> mzte_codec.m_iWvtDecmpLev;

 
  /* copy over the inmask[0] */

  Width[0] = mzte_codec.m_iWidth;
  Width[1] = Width[2] = (Width[0] >> 1);
  Height[0] = mzte_codec.m_iHeight;
  Height[1] = Height[2] = (Height[0] >> 1);
  nLevels[0] = mzte_codec.m_iWvtDecmpLev ;
  nLevels[1] = nLevels[2] = nLevels[0]-1;
  
  Nx[0] = Ny[0]=2;
  for(col=1;col<3;col++) Nx[col]=Ny[col]=1;
  
// #ifdef _DECODER_  // hjlee
  mzte_codec.m_Image = Image;
// #endif
  for (col=0; col<mzte_codec.m_iColors; col++) {
    mzte_codec.m_Image[col].height = mzte_codec.m_iHeight >> (Int)(col>0);
    mzte_codec.m_Image[col].width  = mzte_codec.m_iWidth >> (Int)(col>0);

    inmask[col] = mzte_codec.m_Image[col].mask; 
    outmask[col] = (UChar *)malloc(sizeof(UChar) *  Width[col]*Height[col]);

    ret = do_DWTMask(inmask[col], outmask[col], 
		     Width[col], Height[col], 
		     nLevels[col], &(wvtfilter[col==0?0:1])); 
    if (ret!= DWT_OK) 
      errorHandler("DWT Error Code %d\n", ret);
    
    for (k=0,y=0; y<Height[col]; y++)
      for (x=0; x<Width[col]; x++) 
	COEFF_MASK(x,y,col) = outmask[col][k++];
    free(outmask[col]);
    
  }


  if (target_spatial_levels<=0 || target_snr_levels<= 0)
    errorHandler("Neither target_spatial_levels nor target_snr_levels" \
		 "can be zero");

  /*------- DC: Decode and inverse quantize all color components -------*/
  textureLayerDC_Dec();


  /*------- DC: Close bitstream file -------*/

  /* hjlee 0901 */ 
  if (mzte_codec.m_bStartCodeEnable){
    align_byte();
    if(!mzte_codec.m_iSingleBitFile)
      fclose(bitfile);
  }
  

  /*-------------------------------------------------*/
  /*--------------------- AC ------------------------*/
  /*-------------------------------------------------*/

  
  /*------- AC: SINGLE-QUANT MODE -------*/
  if (mzte_codec.m_iQuantType == SINGLE_Q) 
    textureLayerSQ_Dec(bitfile);
  /*------- AC: MULTI-QUANT MODE -------*/
  else if (mzte_codec.m_iQuantType == MULTIPLE_Q)
// hjlee 0901
    textureLayerMQ_Dec(bitfile, target_spatial_levels, target_snr_levels,
		       wvtfilter);
	
	/*------- AC: BILEVEL-QUANT MODE -------*/
  else if (mzte_codec.m_iQuantType == BILEVEL_Q) {
    PEZW_target_spatial_levels=target_spatial_levels;
    PEZW_target_snr_levels=target_snr_levels;
    PEZW_target_bitrate=0;
	textureLayerBQ_Dec(bitfile);
  }

  for(col=0; col< mzte_codec.m_iColors; col++)
    free(Image[col].mask);
  free(Image);

  if (mzte_codec.m_iSingleBitFile==0){
    if(!mzte_codec.m_bStartCodeEnable)
      align_byte();
    fclose(bitfile);
  }
} 

Void CVTCDecoder::decode(Char *InBitsFile, Char *RecImageFile,
						 Int TargetSpaLev, Int TargetSNRLev)
{
  Int col;
    FILTER **wvtfilter; // hjlee 0901

  noteProgress("\n----- MPEG-4 Visual Texture Coding: Decoding -----\n");

  strcpy(m_cInBitsFile, InBitsFile);
  strcpy(m_cRecImageFile, RecImageFile);

  mzte_codec.m_iTargetSpatialLev = TargetSpaLev;
  mzte_codec.m_iTargetSNRLev = TargetSNRLev;

  mzte_codec.m_iScanOrder = 0;
  mzte_codec.m_iAcmMaxFreqChg =0;
  mzte_codec.m_iAcmOrder =0;


  mzte_codec.m_iColors=3;
//  init_acm_maxf_dec();  // hjlee 0901
  
  mzte_codec.m_iSingleBitFile = 1; /* hjlee 0511 */
  mzte_codec.m_cBitFile = NULL;
  mzte_codec.m_cBitFileAC = NULL;



  TextureObjectLayer_dec(mzte_codec.m_iTargetSpatialLev,
			 mzte_codec.m_iTargetSNRLev, &wvtfilter); 

  /* DISCRETE INVERSE WAVELET TRANSFORM */
  
  noteProgress("\nInverse Wavelet Transform....");
  perform_IDWT(wvtfilter, m_cRecImageFile); // hjlee 0901
  noteProgress("Completed inverse wavelet transform.");
  
  noteDetail("Freeing up decoding data structures....");
	/*----- free up coeff data structure -----*/
  for (col=0; col<mzte_codec.m_iColors; col++) {
		if (mzte_codec.m_SPlayer[col].coeffinfo[0] != NULL)
			delete (mzte_codec.m_SPlayer[col].coeffinfo[0]);
		mzte_codec.m_SPlayer[col].coeffinfo[0] = NULL;
		if (mzte_codec.m_SPlayer[col].coeffinfo)
			delete (mzte_codec.m_SPlayer[col].coeffinfo);
		mzte_codec.m_SPlayer[col].coeffinfo = NULL;
  }
  noteDetail("Completed freeing up decoding data structures.");

  noteProgress("\n----- Decoding Completed. -----\n");

}

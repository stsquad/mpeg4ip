/* $Id: wavelet.cpp,v 1.2 2001/05/09 21:14:18 cahighlander Exp $ */
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

/****************************************************************************/
/*     Texas Instruments Predictive Embedded Zerotree (PEZW) Image Codec    */
/*     Copyright 1996, 1997, 1998 Texas Instruments	      		    */
/****************************************************************************/

#include <stdio.h>
#include <math.h>
#include "dataStruct.hpp"
#include "globals.hpp"
#include "dwt.h"
#include "default.h"
#include "download_filter.h"
#include "wvtfilter.h"
// #include "main.h"

//#include "ShapeCodec.h"


/*-----------------------------------------------------------------------*/
/********************************************************************/
/* Select a wavelet filter banks from the ones defined in default.h */
/* There are currently 10 filter banks available                    */
/********************************************************************/
Void CVTCCommon::choose_wavelet_filter(FILTER **anafilter,FILTER **synfilter,
			   Int type)
{
  switch(type){
    case 0:  /* default Int filter, odd symmetric */
      *anafilter=&DefaultAnalysisFilterInt;
      *synfilter=&DefaultSynthesisFilterInt;
      return;
    case 1: /* default double filter, odd symmetric */
      *anafilter=&DefaultAnalysisFilterDbl;
      *synfilter=&DefaultSynthesisFilterDbl;
      return;
    case 2:  
      *anafilter=&DefaultEvenAnalysisFilterInt;
      *synfilter=&DefaultEvenSynthesisFilterInt;
      return;
    case 3:
      *anafilter=&DefaultEvenAnalysisFilterDbl;
      *synfilter=&DefaultEvenSynthesisFilterDbl;
      return;
     case 4:
      *anafilter=&HaarAna;
      *synfilter=&HaarSyn;
      return;
    case 5:
      *anafilter=&qmf9Ana;
      *synfilter=&qmf9Syn;
      return;
    case 6:
      *anafilter=&qmf9aAna;
      *synfilter=&qmf9aSyn;
      return;
    case 7:
      *anafilter=&fpr53Ana;
      *synfilter=&fpr53Syn;
      return;
    case 8:
      *anafilter=&fpr53aAna;
      *synfilter=&fpr53aSyn;
      return;
    case 9:
      *anafilter=&asd93Ana;
      *synfilter=&asd93Syn;
      return;
    case 10:
      *anafilter=&wav97Ana;
      *synfilter=&wav97Syn;
      return;
    default:
      errorHandler("Filter type %d is not available.",type);
  }
}

/*-----------------------------------------------------------------------*/
Void CVTCEncoder::perform_DWT(FILTER **wvtfilter)
{
  //  Int wordsize = (mzte_codec.m_iBitDepth>8)?2:1;
  Int col,j,k,x,y;
  UChar *inimage[3];
  UChar *inmask[3], *outmask[3];
  Int *outcoeff[3];
  Int Nx[3], Ny[3];
  Int width[3], height[3];
  Int useInt=1, usemask=0;
  Int nLevels[3], ret;

  /* for 4:2:0 YUV image only */
  Nx[0] = Ny[0]=2;
  for(col=1; col<mzte_codec.m_iColors; col++) Nx[col]=Ny[col]=1;


  nLevels[0] = mzte_codec.m_iWvtDecmpLev ;
  nLevels[1] = nLevels[2] = nLevels[0]-1;
  width[0] = mzte_codec.m_iWidth;
  width[1] = width[2] = (width[0] >> 1);
  height[0] = mzte_codec.m_iHeight;
  height[1] = height[2] = (height[0] >> 1);

  useInt  = 1;
  usemask = mzte_codec.m_iAlphaChannel;

  
  Nx[0] = Ny[0]= 2;
  for(col=1;col<3;col++)
    Nx[col]=Ny[col]=1;
  
  for (col=0; col<mzte_codec.m_iColors; col++) {

    inimage[col] = (UChar *)mzte_codec.m_Image[col].data;
    inmask[col] = mzte_codec.m_Image[col].mask;
    
    if ((outcoeff[col] = 
      (Int *)malloc(sizeof(Int)*width[col]*height[col]))==NULL)
      errorHandler("Memory error: outcoeff\n");
    if ((outmask[col] = 
      (UChar *)malloc(sizeof(Char)*width[col]*height[col]))==NULL)
      errorHandler("Memory error: outmask\n");

    ret = do_DWT(inimage[col], inmask[col], width[col], height[col], 
		   nLevels[col], 0, &(wvtfilter[col==0?0:1]), 
		   outcoeff[col], outmask[col]);
	  

    if (ret!=DWT_OK) 
      errorHandler("DWT Error Code %d\n", ret);
    
    mzte_codec.m_iMean[col] = RemoveDCMean(outcoeff[col], outmask[col], 
					width[col], height[col], nLevels[col]);
    
    for (j=0; j<width[col]*height[col]; j++)
      if (outmask[col][j] != DWT_IN)
	outcoeff[col][j]=0;
    
    for (k=0, y = 0; y < height[col]; ++y)
      for (x = 0; x < width[col]; ++x,++k) {
	COEFF_ORGVAL(x,y,col) = outcoeff[col][k];
	COEFF_MASK(x,y,col) = outmask[col][k];
      }
  }
  
  for (col=0; col<mzte_codec.m_iColors; col++) {
    if (outmask[col]) free(outmask[col]);
    if (outcoeff[col]) free(outcoeff[col]);
  }
}



/*------------------------------------------------*/

// hjlee 0901

Void CVTCDecoder::perform_IDWT(FILTER **wvtfilter, Char *recImgFile)
{
  Int j,k,x,y,col;
  UChar *outimage[3];
  UChar *inmask[3], *outmask[3];
  Int *incoeff[3];
  Int Mean[3];
  Int Nx[3], Ny[3];
  Int Width[3], Height[3];
  Int useInt=1, usemask=0;
  Int nLevels[3], ret, MinLevel = 0;

  Int fullsize = 0;
 
  fullsize = FULLSIZE;
  Width[0] = mzte_codec.m_iWidth;
  Width[1] = Width[2] = (Width[0]+1)>>1;

  Height[0] = mzte_codec.m_iHeight;
  Height[1] = Height[2] = (Height[0]+1)>>1;

  nLevels[0] = mzte_codec.m_iWvtDecmpLev ;
  nLevels[1] = nLevels[2] = nLevels[0]-1;

  Mean[0] = mzte_codec.m_iMean[0];
  Mean[1] = mzte_codec.m_iMean[1];
  Mean[2] = mzte_codec.m_iMean[2];

  useInt  = 1;
  usemask = mzte_codec.m_iAlphaChannel;

  Nx[0] = Ny[0]= 2;
  for(col=1;col<mzte_codec.m_iColors;col++)
    Nx[col]=Ny[col]=1;


  for (col=0; col<mzte_codec.m_iColors; col++) {

    if ((inmask[col]=(UChar *)malloc(sizeof(UChar)*
					     Width[col]*Height[col]))==NULL)
		errorHandler("Memory Failed\n");
    
    if ((incoeff[col] = (Int *)malloc(sizeof(Int)*
				  Width[col]*Height[col]))==NULL)
		errorHandler("Memory Failed\n");

    
    /* copy dequantized coefficients to incoeff */
    for (k=0, y=0; y<Height[col]; y++) 
      for (x=0; x<Width[col]; x++,k++) {
			incoeff[col][k] = COEFF_RECVAL(x,y,col);
			inmask[col][k]  = COEFF_MASK(x,y,col);
      }

    for (j=0; j<Width[col]*Height[col];j++) 
      if (inmask[col][j]!=DWT_IN) 
		incoeff[col][j]=0;
    
    AddDCMean(incoeff[col], inmask[col], 
	      Width[col], Height[col], 
	      nLevels[col], Mean[col]);
    
    
    if ((outmask[col]  = (UChar *)malloc(sizeof(UChar)*
					    Width[col]*Height[col]))==NULL)
      errorHandler("Memory Failed\n");

    if ((outimage[col] = (UChar *)malloc(sizeof(UChar)*
					    Width[col]*Height[col]))==NULL)
      errorHandler("Memory Failed\n");

    if(mzte_codec.m_iQuantType==2) {
      Int target_spatial_levels;
      target_spatial_levels=MIN(mzte_codec.m_iSpatialLev,
                            mzte_codec.m_iTargetSpatialLev);
      MinLevel =  mzte_codec.m_iWvtDecmpLev -1- 
        mzte_codec.m_lastWvtDecompInSpaLayer[target_spatial_levels-1][0];
    }
    else {
      MinLevel = mzte_codec.m_iSpatialLev -
        mzte_codec.m_iTargetSpatialLev;
    }
    
	if (MinLevel < 0) MinLevel = 0;
    
    ret = do_iDWT(incoeff[col], inmask[col], Width[col], Height[col], 
		  nLevels[col], MinLevel, 0 /* byte */, 
		  &(wvtfilter[col==0?0:1]), outimage[col], outmask[col], 0,  fullsize );
    if (ret!=DWT_OK) 
      errorHandler("DWT Error Code %d\n", ret);
    free(incoeff[col]);
    free(inmask[col]);
  }  /* col */
  
  
  write_image(recImgFile, mzte_codec.m_iColors,
	      mzte_codec.m_iWidth, mzte_codec.m_iHeight,
	      mzte_codec.m_iRealWidth, mzte_codec.m_iRealHeight,
	      mzte_codec.m_iOriginX, mzte_codec.m_iOriginY,
	      outimage,outmask,
	      usemask,  fullsize, MinLevel);

  for(col=0; col< mzte_codec.m_iColors; col++) {
    free(outmask[col]);
    free(outimage[col]);
  }
}


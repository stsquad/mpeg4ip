/* $Id: wavelet.cpp,v 1.2 2003/11/04 21:33:21 wmaycisco Exp $ */
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

/*	printf("%d %d\n", width[0], height[0]);*/

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

  Int fullsize;// = 0; modified by SL 030399
 
  fullsize = mzte_codec.m_iFullSizeOut;//FULLSIZE; modified by SL 030399
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


// begin: added by Sharp (99/2/16)
Void CVTCEncoder::get_orgval(DATA **Dst, Int TileID)

{
  DATA  *dp;
  int nTilesX;    /* number of tiles (horizontal direction) */
  int tileX, tileY;   /* tile's index of (tileX, tileY) style */
  int w, h;     /* tile width and height */
  int dspW, dspH;   /* display width and height */
  int i, j, c;

  nTilesX = (mzte_codec.m_display_width+mzte_codec.m_tile_width-1)/mzte_codec.m_tile_width;
  tileX  = TileID % nTilesX;
  tileY  = TileID / nTilesX;

  for (c=0; c<mzte_codec.m_iColors; c++) {
    if (c==0) {
      w = mzte_codec.m_tile_width;
      h = mzte_codec.m_tile_height;
      dspW = mzte_codec.m_display_width;
      dspH = mzte_codec.m_display_height;
    } else {
      w = (mzte_codec.m_tile_width + 1)>>1;
      h = (mzte_codec.m_tile_height + 1)>>1;
      dspW = (mzte_codec.m_display_width + 1)>>1;
      dspH = (mzte_codec.m_display_height + 1)>>1;
    }

    for (i=0; i<h; i++) {
      dp = Dst[c] + (tileY*h+i)*dspW + tileX*w;
      for (j=0; j<w; j++) {
        *dp++ = COEFF_ORGVAL(j,i,c);
      }
    }
  }
}

Void CVTCDecoder::copy_coeffs(Int iTile, DATA **frm)
{
  Int Height[3],Width[3];
  Int dspHeight[3], dspWidth[3];
  Int col;
  Int /*k,*/x,y;
  Int tileX, tileY, nTilesW;
  Int Mean[3];
  Int nLevels[3];
  DATA  *p_dst;
	UChar *inmask[3];


  Width[0]  = mzte_codec.m_tile_width;
  Height[0] = mzte_codec.m_tile_height;
  Width[1]  = Width[2]  = mzte_codec.m_tile_width>>1;
  Height[1] = Height[2] = mzte_codec.m_tile_height>>1;
  dspWidth[0] = mzte_codec.m_display_width;
  dspWidth[1] = dspWidth[2] = mzte_codec.m_display_width>>1;
  dspHeight[0] = mzte_codec.m_display_height;
  dspHeight[1] = dspHeight[2] = mzte_codec.m_display_height>>1;
  nLevels[0] = mzte_codec.m_iWvtDecmpLev;
  nLevels[1] = nLevels[2] = nLevels[0]-1;


  nTilesW = mzte_codec.m_display_width / mzte_codec.m_tile_width;

  tileY = iTile/nTilesW;
  tileX = iTile%nTilesW;


  for(col=0; col<mzte_codec.m_iColors; col++) {
	inmask[col] = mzte_codec.m_Image[col].mask;
    Mean[col] = mzte_codec.m_iMean[col];
    for (y=0; y<Height[col]; y++) {
      p_dst = frm[col] + (tileY*Height[col]+y)*dspWidth[col] + tileX*Width[col];
      for (x=0; x<Width[col]; x++) {
        *p_dst++ = COEFF_RECVAL(x,y,col);

/*        frm_msk[col][k]  = COEFF_MASK(x,y,col);*/
      }
      /*     for (j=0; j<Width[col]*Height[col];j++)  */
      /*       if (frm_msk[col][j]!=IN)  */
      /*  frm[col][j]=0; */
    }

    AddDCMeanTile(frm[col], NULL,
        dspWidth[col], dspHeight[col],
        nLevels[col], Mean[col],
        Width[col], Height[col],tileX, tileY);

  }
}

Void CVTCEncoder::perform_DWT_Tile(FILTER **wvtfilter, PICTURE *SrcImg, Int TileID)
{
  Int wordsize = (mzte_codec.m_iBitDepth>8)?2:1;
  Int col,/*j,*/k,l,x,y;
  Void *inimage;
  UChar *inmask, *outmask;
  DATA *outcoeff;
  DATA *workcoeff;
  UChar *workmask;
  Int width, height;
  Int nLevels, ret;
  Int orgWidth, orgHeight;
  Int tileX, tileY;
  Int tileWidth, tileHeight;
  Int nTilesW, nTilesH;
  Int TileWidth=mzte_codec.m_tile_width, TileHeight=mzte_codec.m_tile_height;
  Int Flag;
  Int h_pre, h_ape, w_pre, w_ape;
  UChar *p_dst, *p_src;
  UShort *p_dsts, *p_srcs;
  Int level;
  DATA *pHL, *pLH, *pHH;


  if (mzte_codec.m_extension_type == 0) {
    Flag = 0;
  } else {
    Flag = 3;
  }

  nTilesW = SrcImg[0].width / mzte_codec.m_Image[0].width;
  nTilesH = SrcImg[0].height / mzte_codec.m_Image[0].height;

  /* convert TileID into (tileX, tileY) style */
  tileX = TileID % nTilesW;
  tileY = TileID / nTilesW;

  /* memory allocation */
  if (wordsize == 1) {
    if ((inimage = (UChar *)malloc(sizeof(UChar)*TileWidth*3*TileHeight*3))==NULL)
      errorHandler("Memory error: inimage\n");
  } else {
    if ((inimage = (UShort *)malloc(sizeof(UShort)*TileWidth*3*TileHeight*3))==NULL)
      errorHandler("Memory error: inimage\n");
  }
  if ((inmask = (UChar *)malloc(sizeof(UChar)*TileWidth*3*TileHeight*3))==NULL)
    errorHandler("Memory error: inmask\n");
  memset(inmask, 1, sizeof(UChar)*TileWidth*3*TileHeight*3);

  if ((outcoeff = (DATA *)malloc(sizeof(DATA)*TileWidth*3*TileHeight*3))==NULL)
    errorHandler("Memory error: outcoeff\n");
  if ((outmask =  (UChar *)malloc(sizeof(Char)*TileWidth*3*TileHeight*3))==NULL)
    errorHandler("Memory error: outmask\n");
  memset(outmask,1, sizeof(UChar)*TileWidth*3*TileHeight*3);

  if ((workcoeff = (DATA *)malloc(sizeof(DATA)*TileWidth*3*TileHeight*3))==NULL)
    errorHandler("Memory error: workcoeff\n");
  if ((workmask =  (UChar *)malloc(sizeof(Char)*TileWidth*3*TileHeight*3))==NULL)
    errorHandler("Memory error: workmask\n");

  for (col=0; col<mzte_codec.m_iColors; col++) {
    orgWidth = SrcImg[col].width;
    orgHeight = SrcImg[col].height;

    if (col==0) {
      tileWidth = TileWidth;
      tileHeight = TileHeight;
      nLevels = mzte_codec.m_iWvtDecmpLev;
    } else {
      tileWidth = (TileWidth+1)>>1;
      tileHeight = (TileHeight+1)>>1;
      nLevels = mzte_codec.m_iWvtDecmpLev - 1;
    }

    /* determine extension type */
    h_pre = h_ape = tileHeight;
    w_pre = w_ape = tileWidth;

    if ((0x1&Flag)==0) {
      h_pre = 0;
      h_ape = 0;
    } else {
      if (tileY==0) {
        h_pre = 0;
      }
      if (tileY==(nTilesH-1)) {
        h_ape = 0;
      }
    }

    if ((0x2&Flag)==0) {
      w_pre = 0;
      w_ape = 0;
    } else {
      if (tileX==0) {
        w_pre = 0;
      }
      if (tileX==(nTilesW-1)) {
        w_ape = 0;
      }
    }

    width = w_pre + tileWidth + w_ape;
    height = h_pre + tileHeight + h_ape;

    /* copy source data */
    if ( wordsize == 1 ) {  /* BYTE */
      p_dst = (UChar *)inimage;
      for(k=0; k<(tileHeight+h_pre+h_ape); k++) {
        p_src = (UChar *)(SrcImg[col].data) + (tileHeight*tileY-h_pre+k)*orgWidth
            + tileWidth*tileX-w_pre;
        for(l=0; l<(tileWidth+w_pre+w_ape); l++) {
          (*(p_dst++)) = (Int)(*(p_src++));
        }
      }
    } else {    /* Unsigned Short */
      p_dsts = (UShort *)inimage;
      for(k=0; k<(tileHeight+h_pre+h_ape); k++) {
        p_srcs = (UShort *)(SrcImg[col].data) + (tileHeight*tileY-h_pre)*orgWidth
            + tileWidth*tileX-w_pre + orgWidth*k;
        for(l=0; l<(tileWidth+w_pre+w_ape); l++) {
          (*(p_dsts++)) = (Int)(*(p_srcs++));
        }
      }
    }
    ret = do_DWT(inimage, inmask, width, height, nLevels, 0, wvtfilter, workcoeff, workmask);
    if (ret!=DWT_OK)
      errorHandler("DWT Error Code %d\n", ret);


    for(level=1;level<=nLevels;level++) {

      pHL = workcoeff + ((width+w_pre)>>level) + (h_pre>>level)*width;
      pLH = workcoeff + ((height+h_pre)>>level)*width + (w_pre>>level);
      pHH = workcoeff + ((height+h_pre)>>level)*width + ((width+w_pre)>>level);

      for(k=0; k<(tileHeight>>level); k++) {
        memcpy(outcoeff + (tileWidth>>level) + k*tileWidth, pHL,
            sizeof(DATA)*(tileWidth>>level));
        memcpy(outcoeff + ((tileHeight>>level)+k)*tileWidth, pLH,
            sizeof(DATA)*(tileWidth>>level));
        memcpy(outcoeff + ((tileHeight>>level)+k)*tileWidth + (tileWidth>>level), pHH,
            sizeof(DATA)*(tileWidth>>level));
        pHL += width;
        pLH += width;
        pHH += width;
      }
    }

    /* copy LL from WorkCoeff */
    for(k=0; k<(tileHeight>>nLevels);k++) {
      memcpy(outcoeff+k*tileWidth,
          workcoeff+((h_pre>>nLevels)+k)*width+(w_pre>>nLevels),
          sizeof(DATA)*(tileWidth>>nLevels));
    }

    mzte_codec.m_iMean[col] = RemoveDCMean(outcoeff, outmask,
        tileWidth, tileHeight, nLevels);

    /*     for (j=0; j<tileWidth*tileHeight; j++) */
    /*       if (outmask[j] != IN) */
    /*  outcoeff[j]=0; */

    for (k=0, y = 0; y < tileHeight; ++y)
      for (x = 0; x < tileWidth; ++x,++k) {
        COEFF_ORGVAL(x,y,col) = outcoeff[k];
        COEFF_MASK(x,y,col) = outmask[k];
      }
  }

	if (inimage) free(inimage);
	if (inmask) free(inmask);
  if (outmask) free(outmask);
  if (outcoeff) free(outcoeff);
  if (workmask) free(workmask);
  if (workcoeff) free(workcoeff);
}

/*------------------------------------------------*/
// begin: modified by Sharp (99/5/10)
Void CVTCDecoder::perform_IDWT_Tile(FILTER **wvtfilter, UChar **frm, UChar **frm_mask, Int iTile, Int TileW)
{
  Int j,k,x,y,col;
  UChar *outimage[3];
  UChar *inmask[3], *outmask[3];
  Int *incoeff[3];
  Int Mean[3];
  Int Nx[3], Ny[3];
  Int Width[3], Height[3];
  Int useInt=1, usemask=0;
  Int nLevels[3], ret, MinLevel = 0x0;

  Int fullsize;// = 0; modified by SL 030399
 
  fullsize = mzte_codec.m_iFullSizeOut;//FULLSIZE; modified by SL 030399
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
  
  
	noteProgress("Copying reconstructed image ...");

/*	printf("%d %d\n", mzte_codec.m_display_width, mzte_codec.m_display_height);*/
// FPDAM begin : modified by Sharp
  write_image_to_buffer(frm, frm_mask, mzte_codec.m_iObjectWidth, mzte_codec.m_iObjectHeight, iTile, TileW,
				mzte_codec.m_iColors,
	      mzte_codec.m_iWidth, mzte_codec.m_iHeight,
	      mzte_codec.m_iRealWidth, mzte_codec.m_iRealHeight,
	      mzte_codec.m_iOriginX, mzte_codec.m_iOriginY,
	      outimage,outmask,
	      usemask,  fullsize, MinLevel);
// FPDAM end : modified by Sharp
/*	noteProgress("done.");*/

  for(col=0; col< mzte_codec.m_iColors; col++) {
    free(outmask[col]);
    free(outimage[col]);
  }
}

#if 0
Void CVTCDecoder::perform_IDWT_Tile(FILTER **wvtfilter, Char *recImgFile, DATA **frm)
{
  Int j,k,/*x,*/y,col;
  UChar *outimage[3];
  UChar /**inmask[3],*/ *outmask[3];
  //DATA *incoeff[3];
  /*   Int Mean[3]; */
  Int Width[3], Height[3];
  Int tileWidth[3], tileHeight[3];
  //Int curWidth[3], curHeight[3];
  Int useInt=1, usemask=0;
  Int nLevels[3], ret, MinLevel;
  Int tileXfrom, tileYfrom, tileXto, tileYto;
  //Int nTilesX;
  DATA *buf;
  UChar *obuf, *obufmsk, *bufmsk;
  Int i;

  Int fullsize = 0;

  /*   fullsize = FULLSIZE; */
  Width[0] = mzte_codec.m_display_width;
  Width[1] = Width[2] = (Width[0]+1)>>1;

  Height[0] = mzte_codec.m_display_height;
  Height[1] = Height[2] = (Height[0]+1)>>1;

  tileWidth[0] = mzte_codec.m_tile_width;
  tileWidth[1] = tileWidth[2] = (tileWidth[0]+1)>>1;
  tileHeight[0] = mzte_codec.m_tile_height;
  tileHeight[1] = tileHeight[2] = (tileHeight[0]+1)>>1;

//  nTilesX = mzte_codec.m_display_width/mzte_codec.m_tile_width;
//  tileXfrom = mzte_codec.m_target_tile_id_from % nTilesX;
//  tileYfrom = mzte_codec.m_target_tile_id_from / nTilesX;
//  tileXto = mzte_codec.m_target_tile_id_to % nTilesX;
//  tileYto = mzte_codec.m_target_tile_id_to / nTilesX;
  tileXfrom = 0;
  tileYfrom = 0;
  tileXto = mzte_codec.m_display_width/mzte_codec.m_tile_width-1;
  tileYto = mzte_codec.m_display_height/mzte_codec.m_tile_height-1;

  nLevels[0] = mzte_codec.m_iWvtDecmpLev ;
  nLevels[1] = nLevels[2] = nLevels[0]-1;

  /*   Mean[0] = mzte_codec->mean[0]; */
  /*   Mean[1] = mzte_codec->mean[1]; */
  /*   Mean[2] = mzte_codec->mean[2]; */

  useInt  = 1;
  usemask = mzte_codec.m_iAlphaChannel;
  if (mzte_codec.m_extension_type==0){
    buf=(DATA *)malloc(sizeof(DATA)*tileWidth[0]*tileHeight[0]);
    bufmsk=(UChar *)malloc(sizeof(UChar)*tileWidth[0]*tileHeight[0]);
    obuf=(UChar *)malloc(sizeof(DATA)*tileWidth[0]*tileHeight[0]);
    obufmsk=(UChar *)malloc(sizeof(UChar)*tileWidth[0]*tileHeight[0]);
    memset(bufmsk, 1, sizeof(UChar)*tileWidth[0]*tileHeight[0]);
  }

  for (col=0; col<mzte_codec.m_iColors; col++) {

    /*     if ((inmask[col]=(UChar *)malloc(sizeof(UChar)* */
    /*               Width[col]*Height[col]))==NULL) */
    /*       errorHandler("Memory Failed\n"); */

    /*     if ((incoeff[col] = (DATA *)malloc(sizeof(DATA)* */
    /*          Width[col]*Height[col]))==NULL) */
    /*       errorHandler("Memory Failed\n"); */


    /* copy dequantized coefficients to incoeff */
    /*     for (k=0, y=0; y<Height[col]; y++)  */
    /*       for (x=0; x<Width[col]; x++,k++) { */
    /*  incoeff[col][k] = COEFF_RECVAL(x,y,col); */
    /*  inmask[col][k]  = COEFF_MASK(x,y,col); */
    /*       } */
    /*     for (j=0; j<Width[col]*Height[col];j++)  */
    /*       if (inmask[col][j]!=IN)  */
    /*  incoeff[col][j]=0; */

    /*     AddDCMean(incoeff[col], inmask[col],  */
    /*        Width[col], Height[col],  */
    /*        nLevels[col], Mean[col]); */


    if ((outmask[col]  = (UChar *)malloc(sizeof(UChar)* Width[col]*Height[col]))==NULL)
      errorHandler("Memory Failed\n");

    if ((outimage[col] = (UChar *)malloc(sizeof(UChar)* Width[col]*Height[col]))==NULL)
      errorHandler("Memory Failed\n");

    memset(outimage[col],0,sizeof(UChar)*Width[col]*Height[col]);

#if  PROGRESSIVE
    MinLevel = (nLevels[0] > nLevels[1])? nLevels[1]:nLevels[0];
    MinLevel = (MinLevel > nLevels[2])? nLevels[2]: MinLevel;
#else 
    MinLevel = mzte_codec.m_iSpatialLev - mzte_codec.m_iTargetSpatialLev;
#endif 

    if (mzte_codec.m_extension_type==0) {

      for(i=tileYfrom;i<=tileYto;i++){
        for(j=tileXfrom;j<=tileXto;j++){
          for(k=0;k<tileHeight[col];k++){
            memcpy(buf+k*tileWidth[col],
                frm[col]+(i*tileHeight[col]+k)*Width[col]+j*tileWidth[col],
                sizeof(DATA)*tileWidth[col]);
          }
          ret = do_iDWT(buf,bufmsk,tileWidth[col],tileHeight[col],nLevels[col],
              MinLevel, 0, wvtfilter, obuf, obufmsk, 0, 0);
          for(k=0;k<tileHeight[col];k++){
            memcpy(outimage[col]+(i*(tileHeight[col]>>MinLevel)+k)*Width[col]+j*(tileWidth[col]>>MinLevel),
                obuf+k*(tileWidth[col]>>MinLevel),
                sizeof(UChar)*(tileWidth[col]>>MinLevel));
            memcpy(outmask[col]+(i*(tileHeight[col]>>MinLevel)+k)*Width[col]+j*(tileWidth[col]>>MinLevel),
                obufmsk+k*(tileWidth[col]>>MinLevel),
                sizeof(UChar)*(tileWidth[col]>>MinLevel));
          }
        }
      }
    } else {
      ret = do_iDWT_Tile(frm[col], NULL, Width[col], Height[col], nLevels[col],
          MinLevel, 0, wvtfilter, outimage[col], outmask[col],
          tileWidth[col],tileHeight[col],
          0, fullsize, mzte_codec.m_extension_type,
          mzte_codec.m_target_tile_id_from, mzte_codec.m_target_tile_id_to);
    }

    if (ret!=DWT_OK)
      errorHandler("DWT Error Code %d\n", ret);

  }  /* col */

  /*   write_image(recImgFile, mzte_codec->colors, */
  /*        mzte_codec->width, mzte_codec->height, */
  /*        mzte_codec->real_width, mzte_codec->real_height, */
  /*        mzte_codec->origin_x, mzte_codec->origin_y, */
  /*        outimage,outmask, */
  /*        usemask,  fullsize, MinLevel); */

  {
		FILE *fp = fopen(recImgFile,"wb");
		noteProgress("Writing '%s'(%dx%d) ....",recImgFile, 
			(tileWidth[0] >> MinLevel) * (tileXto+1), 
			(tileHeight[0] >> MinLevel) * (tileYto+1));

		for(col=0;col<3;col++){
			Int w = Width[col];
			Int toX = (tileWidth[col] >> MinLevel) * (tileXto+1);
			Int fromX = 0;
			Int toY = (tileHeight[col] >> MinLevel) * (tileYto+1);
			Int fromY = 0;

			Int tw = Width[col] >> MinLevel;
			Int th = Height[col] >> MinLevel;
		
			for ( y=0; y<th; y++ )
			fwrite(outimage[col]+y*w,sizeof(char)*tw,1,fp);
		}
		fclose(fp);
	}

  if (mzte_codec.m_extension_type==0){
    if(buf)free(buf);
    if(obuf)free(obuf);
    if(bufmsk)free(bufmsk);
    if(obufmsk)free(obufmsk);
  }
  for(col=0; col< mzte_codec.m_iColors; col++) {
    free(outmask[col]);
    free(outimage[col]);
  }
}
#endif
// end: modified by Sharp (99/5/10)

// begin: added by Sharp (99/2/16)

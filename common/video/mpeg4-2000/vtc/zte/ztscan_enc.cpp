/* $Id: ztscan_enc.cpp,v 1.1 2003/05/05 21:24:12 wmaycisco Exp $ */
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

/************************************************************/
/*  Filename: ztscan_enc.c                                  */
/*  Author: Bing-Bing Chai                                  */
/*  Date: Dec. 4, 1997                                      */
/*                                                          */
/*  Descriptions:                                           */
/*    This file contains the routines that performs         */
/*    zero tree scanning and entropy encoding.              */
/*                                                          */
/************************************************************/

#include <stdio.h>
#include <stdlib.h>
#ifndef WIN32
#include <unistd.h>
#endif
#include <ctype.h>
#include <string.h>
#include <math.h>
#include "basic.hpp"
#include "startcode.hpp"
#include "dataStruct.hpp"
#include "states.hpp"
#include "globals.hpp"
#include "errorHandler.hpp"
#include "ac.hpp"
#include "bitpack.hpp"
#include "msg.hpp"
#include "ztscan_common.hpp"
#include "ztscanUtil.hpp"

// added for FDAM1 by Samsung AIT on 2000/02/03
#ifndef	_SHAPE_
#define	_SHAPE_
#endif
// ~added for FDAM1 by Samsung AIT 2000/02/03

/* local global variables */
static ac_encoder ace;

static int bit_stream_length;
static SInt **dc_coeff;
/* added by Z. Wu @ OKI for SA-prediction */
static Char **dc_mask;
static COEFFINFO **coeffinfo;
static Int color;
static Int height, width;


/******************************************************************/
/****************************  DC  ********************************/
/******************************************************************/

/*******************************************************/
/**************  Forward DC Prediction  ****************/
/*******************************************************/

/********************************************************
  Function Name
  -------------
  static DATA DC_pred_pix(Int i, Int j)

  Arguments
  ---------
  Int i, Int j: Index of wavelet coefficient (row, col)
  
  Description
  -----------
  DPCM prediction for a DC coefficient, refer to syntax
  for algorithm. 

  Functions Called
  ----------------
  None.

  Return Value
  ------------
    prediction for coeffinfo[i][j].quantized_value
********************************************************/ 
SInt CVTCEncoder::DC_pred_pix(Int i, Int j)
{
//Added by Sarnoff for error resilience, 3/5/99
#ifdef _DC_PACKET_
  if(!mzte_codec.m_usErrResiDisable)
  // error resilience doesn't use prediction
    return 0;
#endif 
//End: Added by Sarnoff for error resilience, 3/5/99

  /*  modified by Z. Wu @ OKI */
  Int pred_i, pred_j, pred_d;

  if ( i==0 || dc_mask[i-1][j] == 0 ) 
    pred_i = 0;
  else	
    pred_i = dc_coeff[i-1][j];

  if ( j==0 || dc_mask[i][j-1] == 0 ) 
    pred_j = 0;
  else 	
    pred_j = dc_coeff[i][j-1];

  if ( i==0 || j== 0 || dc_mask[i-1][j-1] == 0 )
    pred_d = 0;
  else
    pred_d = dc_coeff[i-1][j-1];
  
  if ( abs(pred_d-pred_j) < abs(pred_d-pred_i))	
    return(pred_i);
  else
    return(pred_j);

}



/*****************************************************
  Function Name
  -------------
  Void DC_predict()

  Arguments
  ---------
  None
  
  Description
  -----------
  control program for DC prediction

  Functions Called
  ----------------
  DC_pred_pix(i,j).

  Return Value
  ------------
  none
******************************************************/

Void CVTCEncoder::DC_predict(Int color)
{
  Int i,j,dc_h,dc_w,offset_dc,max_dc;

  dc_h=mzte_codec.m_iDCHeight;
  dc_w=mzte_codec.m_iDCWidth;

  dc_coeff=(SInt **)calloc(dc_h,sizeof(SInt *));
  for(i=0;i<dc_h;i++)
    dc_coeff[i]=(SInt *)calloc(dc_w,sizeof(SInt));

  dc_mask=(Char **)calloc(dc_h,sizeof(Char *));
  for(i=0;i<dc_h;i++)
    dc_mask[i]=(Char *)calloc(dc_w,sizeof(Char));


  coeffinfo=mzte_codec.m_SPlayer[color].coeffinfo;

  for(i=0;i<dc_h;i++)
    for(j=0;j<dc_w;j++) {
      dc_coeff[i][j]=coeffinfo[i][j].quantized_value;
      dc_mask [i][j]=coeffinfo[i][j].mask;
    }

  /* prediction */    
  offset_dc=0;
	
  for(i=0;i<dc_h;i++)
    for(j=0;j<dc_w;j++){
      if ( dc_mask[i][j] != 0 ) {
	if(offset_dc>(coeffinfo[i][j].quantized_value-=DC_pred_pix(i,j)))
	  offset_dc=coeffinfo[i][j].quantized_value;
      }
    }

  if(offset_dc>0)
    offset_dc=0;

  /* adjust coeff's by offset_dc */
  max_dc=0;
  for(i=0;i<dc_h;i++)
    for(j=0;j<dc_w;j++){
      if ( dc_mask[i][j] != 0 ) {
	coeffinfo[i][j].quantized_value -=offset_dc;
	/* find max_dc */
	if (max_dc<coeffinfo[i][j].quantized_value)
	  max_dc=coeffinfo[i][j].quantized_value;
      }
    }
    
  mzte_codec.m_iOffsetDC=offset_dc;
  mzte_codec.m_iMaxDC=max_dc;  /* hjlee */
  noteDebug("DC pred: offset=%d, max_dc=%d",
	    mzte_codec.m_iOffsetDC,mzte_codec.m_iMaxDC);  

  for(i=0;i<dc_h;i++) {
    free(dc_coeff[i]);
    free(dc_mask[i]);
  }
  free(dc_coeff);
  free(dc_mask);
}




/********************************************************
  Function Name
  -------------
  Void wavelet_dc_encode(Int c)


  Arguments
  ---------
  Int c - color component.
  
  Description
  -----------
  Control program for encode DC information for one 
  color component.

  Functions Called
  ----------------
  None.
  DC_predict()
  put_param()
  cacll_encode()
  
  Return Value
  ------------
  None.

********************************************************/ 
Void CVTCEncoder::wavelet_dc_encode(Int c)
{

  noteDetail("Encoding DC (wavelet_dc_encode)....");
  color=c;

//Added by Sarnoff for error resilience, 3/5/99
#ifdef _DC_PACKET_
  if(!mzte_codec.m_usErrResiDisable)
    acmSGMK.Max_frequency = Bitplane_Max_frequency;
#endif
//End: Added by Sarnoff for error resilience, 3/5/99

  emit_bits((UShort)mzte_codec.m_iMean[color], 8);
  put_param((UShort)mzte_codec.m_iQDC[color], 7);
  /* emit_bits(mzte_codec.Qdc[color], 8); */

  DC_predict(color);
  put_param(-mzte_codec.m_iOffsetDC,7);
  put_param(mzte_codec.m_iMaxDC,7); /* hjlee */
  /* put_param(mzte_codec.m_iMaxDC+mzte_codec.m_iOffsetDC,7); */ /* hjlee */
  //printf("%d %d %d %d\n", mzte_codec.m_iMean[color],mzte_codec.m_iQDC[color],-mzte_codec.m_iOffsetDC,mzte_codec.m_iMaxDC);
  cacll_encode();
  noteDetail("Completed encoding DC.");

}



/********************************************************
  Function Name
  -------------
  static Void cacll_encode()

  Arguments
  ---------
  None.

  
  Description
  -----------
  Encode DC information for one color component.

  Functions Called
  ----------------
  mzte_ac_encoder_init()
  mzte_ac_model_init()
  mzte_ac_encode_symbol()
  mzte_ac_model_done()
  mzte_ac_encoder_done()

  Return Value
  ------------
  None.

********************************************************/ 
Void CVTCEncoder::cacll_encode()
{
  Int dc_h, dc_w,i,j;
	Int numBP, bp; // 1124

  dc_w=mzte_codec.m_iDCWidth;
  dc_h=mzte_codec.m_iDCHeight;

  // 1124
  /* init arithmetic coder */
  numBP = ceilLog2(mzte_codec.m_iMaxDC+1); // modified by Sharp (99/2/16)
  mzte_ac_encoder_init(&ace);
  if ((acm_bpdc=(ac_model *)calloc(numBP,sizeof(ac_model)))==NULL)
    errorHandler("Can't allocate memory for prob model.");
  
  for (i=0; i<numBP; i++) {
    acm_bpdc[i].Max_frequency = Bitplane_Max_frequency;
    mzte_ac_model_init(&(acm_bpdc[i]),2,NULL,ADAPT,1);
  }
  coeffinfo=mzte_codec.m_SPlayer[color].coeffinfo;

//Modified by Sarnoff for error resilience, 3/5/99
 if(mzte_codec.m_usErrResiDisable){ //no error resi case
  for (bp=numBP-1; bp>=0; bp--) {
    for(i=0;i<dc_h;i++)
      for(j=0;j<dc_w;j++){
		 // printf("%d %d \n", i,j);
	if( coeffinfo[i][j].mask == 1) 
	  mzte_ac_encode_symbol(&ace, &(acm_bpdc[bp]),
				((coeffinfo[i][j].quantized_value)>>bp)&1);
      }
  }
  /* close arithmetic coder */
  for (i=0; i<numBP; i++) 
    mzte_ac_model_done(&(acm_bpdc[i]));
  free(acm_bpdc); 
  bit_stream_length=mzte_ac_encoder_done(&ace);
 }
 else{ //error resilience case
#ifdef _DC_PACKET_
  TU_max_dc += numBP;
  mzte_ac_model_init(&acmType[color][0][CONTEXT_INIT],NUMCHAR_TYPE,
					NULL,ADAPT,1);	

  for (bp=numBP-1; bp>=0; bp--) {
	for(i=0;i<dc_h;i++){
      for(j=0;j<dc_w;j++){
	if( coeffinfo[i][j].mask == 1) 
	  mzte_ac_encode_symbol(&ace, &(acm_bpdc[bp]),
				((coeffinfo[i][j].quantized_value)>>bp)&1);
      }
		check_segment_size(color);
	}
	check_end_of_DC_packet(numBP);
  }

  /* close arithmetic coder */
  if (packet_size+ace.bitCount+ace.followBits>0){
	  for (i=0; i<numBP; i++) 
		mzte_ac_model_done(&(acm_bpdc[i]));
	  mzte_ac_model_done(&(acmType[color][0][CONTEXT_INIT]));
	  free(acm_bpdc); 
	  bit_stream_length=mzte_ac_encoder_done(&ace);
  }
#else 
  //same as no error resi case
  for (bp=numBP-1; bp>=0; bp--) {
    for(i=0;i<dc_h;i++)
      for(j=0;j<dc_w;j++){
		 // printf("%d %d \n", i,j);
	if( coeffinfo[i][j].mask == 1) 
	  mzte_ac_encode_symbol(&ace, &(acm_bpdc[bp]),
				((coeffinfo[i][j].quantized_value)>>bp)&1);
      }
  }
  /* close arithmetic coder */
  for (i=0; i<numBP; i++) 
    mzte_ac_model_done(&(acm_bpdc[i]));

  free(acm_bpdc); 
  bit_stream_length=mzte_ac_encoder_done(&ace);
#endif 
 }
//End: Added by Sarnoff for error resilience, 3/5/99
}

/*********************************************************************/
/*****************************  AC  **********************************/
/*********************  Single and Multi Quant  **********************/
/*********************************************************************/

Void CVTCEncoder::bitplane_encode(Int val,Int l,Int max_bplane)
{
  register int i,k=0;

  for(i=max_bplane-1;i>=0;i--,k++)
    mzte_ac_encode_symbol(&ace,&acm_bpmag[l][k],(val>>i)&1);
}


/*********************************************************************/
/*****************************  AC  **********************************/
/*************************  Single quant  ****************************/
/*********************************************************************/
/*******************************************************
  The following single quant routines are for band by
  band scan order.
*******************************************************/
/********************************************************
  Function Name
  -------------
  Void wavelet_higher_bands_encode_SQ_band(Int col)

  Arguments
  ---------
  None.

  Description
  -----------
  Control program for encoding AC information for one 
  color component. Single quant mode.

  Functions Called
  ----------------
  cachb_encode_SQ_band()
  mzte_ac_encoder_init()
  mzte_ac_model_init()
  mzte_ac_model_done()
  mzte_ac_encoder_done()

  Return Value
  ------------
  None.

********************************************************/ 
Void CVTCEncoder::wavelet_higher_bands_encode_SQ_band(Int col)
{
  SNR_IMAGE *snr_image;
    
  noteDetail("Encoding AC (wavelet_higher_bands_encode_SQ)....");

  color=col;
  snr_image=&(mzte_codec.m_SPlayer[color].SNRlayer.snr_image);
  
//Modified by Sarnoff for error resilience, 3/5/99
 if(mzte_codec.m_usErrResiDisable){ //no error resi case
  /* init arithmetic coder */
  mzte_ac_encoder_init(&ace);

  probModelInitSQ(color);  // hjlee 0901

  cachb_encode_SQ_band(snr_image);
  
  probModelFreeSQ(color);  // hjlee 0901
  
  bit_stream_length=mzte_ac_encoder_done(&ace);
 }
 else{ //error resilience case
  /* init arithmetic coder */
  init_arith_encoder_model(color);

  cachb_encode_SQ_band(snr_image);
  
  if (packet_size+ace.bitCount>0)
  {
	  /* if color is V then write packet header */
	  TU_last--;
	  close_arith_encoder_model(color,
		mzte_codec.m_iCurSpatialLev==0 || color==2);
	  if (mzte_codec.m_iCurSpatialLev==0 || color==2)
		  force_end_of_packet();
	  else
		  TU_last++;
  }
 }
//End modified by Sarnoff for error resilience, 3/5/99

  noteDetail("Completed encoding AC.");
}


/********************************************************
  Function Name
  -------------
  static Void cachb_encode_SQ_band()

  Arguments
  ---------
  None.

  Description
  -----------
  Encode AC information for single quant mode, tree-depth scan.

  Functions Called
  ----------------
  codeBlocks();
  encode_pixel_SQ()

  Return Value
  ------------
  None.

********************************************************/ 
Void CVTCEncoder::cachb_encode_SQ_band(SNR_IMAGE *snr_image)
{
  Int h,w,ac_h,ac_w,ac_h2,ac_w2;
  Int n; /* layer index - for codeBlocks function */
  Int k; /* block jump for the layer */  // hjlee 0928

  /* ac_h, ac_w init */
  ac_h2=mzte_codec.m_SPlayer[color].height;
  ac_w2=mzte_codec.m_SPlayer[color].width;
  ac_h=ac_h2>>1;
  ac_w=ac_w2>>1;

  height=mzte_codec.m_Image[color].height;
  width=mzte_codec.m_Image[color].width;

  /* Get layer index - for codeBlocks function */  // hjlee 0928
  n = -1;
  for (w=mzte_codec.m_iDCWidth; w < ac_w2; w<<=1)
    n++;

  setProbModelsSQ(color); // hjlee 0901

  coeffinfo=mzte_codec.m_SPlayer[color].coeffinfo;
	
  /* scan each coefficients in the spatial layer */  
  k = 1<<n;  // hjlee 0928

//Modified by Sarnoff for error resilience, 3/5/99
 if(mzte_codec.m_usErrResiDisable){ //no error resilience case
  for(h=0;h<ac_h;h+=k)
    for(w=ac_w;w<ac_w2;w+=k)
    {
      /* LH */
      encodeSQBlocks(h,w,n);

      /* HL */
      h += ac_h;
      w -= ac_w;
      encodeSQBlocks(h,w,n);

      /* HH */
      w += ac_w;
      encodeSQBlocks(h,w,n);

      /* Set h back to where it started. w is already there */
      h -= ac_h;
    }
 }
 else{ //error resilience case 
  for(h=0;h<ac_h;h+=k)
  {
    for(w=ac_w;w<ac_w2;w+=k)
    {
      /* LH */
      encodeSQBlocks_ErrResi(h,w,n,color);
	  if (n>0 && n<5)
		  check_segment_size(color);

      /* HL */
      h += ac_h;
      w -= ac_w;
      encodeSQBlocks_ErrResi(h,w,n,color);
	  if (n>0 && n<5)
		  check_segment_size(color);

      /* HH */
      w += ac_w;
      encodeSQBlocks_ErrResi(h,w,n,color);
	  if (n>0 && n<5)
		  check_segment_size(color);

      /* Set h back to where it started. w is already there */
      h -= ac_h;
    }
	/* new texture unit */
	check_end_of_packet(color);
  }
 }
//End modified by Sarnoff for error resilience, 3/5/99
}

     
/*******************************************************
  The following single quant routines are for tree
  depth scan order.
*******************************************************/
/********************************************************
  Function Name
  -------------
  Void wavelet_higher_bands_encode_SQ_tree()

  Arguments
  ---------
  None.

  Description
  -----------
  Control program for encoding AC information for one 
  color component. Single quant mode.

  Functions Called
  ----------------
  cachb_encode_SQ_tree()
  mzte_ac_encoder_init()
  mzte_ac_model_init()
  mzte_ac_model_done()
  mzte_ac_encoder_done()

  Return Value
  ------------
  None.

********************************************************/ 
Void CVTCEncoder::wavelet_higher_bands_encode_SQ_tree() // hjlee 0928
{
  noteDetail("Encoding AC (wavelet_higher_bands_encode_SQ)....");

//Modified by Sarnoff for error resilience, 3/5/99
 if(mzte_codec.m_usErrResiDisable){ //no error resilience case
  /* init arithmetic coder */
  mzte_ac_encoder_init(&ace);

  // hjlee 0901
  for (color=0; color<mzte_codec.m_iColors; color++) 
    probModelInitSQ(color);
  
  cachb_encode_SQ_tree(); // hjlee 0928
  
  // hjlee 0901
  for (color=0; color<mzte_codec.m_iColors; color++) 
    /* close arithmetic coder */
    probModelFreeSQ(color);

  bit_stream_length=mzte_ac_encoder_done(&ace);
 }
 else{ //error resilience
  init_arith_encoder_model(-1);

  cachb_encode_SQ_tree(); // hjlee 0928
  
  if (packet_size+ace.bitCount>0){
	  TU_last--;
	  close_arith_encoder_model(-1,1); // write packet header
  }
 }
//End modified by Sarnoff for error resilience, 3/5/99

  noteDetail("Completed encoding AC.");
}


/********************************************************
  Function Name
  -------------
  static Void cachb_encode_SQ_tree()

  Arguments
  ---------
  None.

  Description
  -----------
  Encode AC information for single quant mode, tree-depth scan.

  Functions Called
  ----------------
  encode_pixel_SQ_tree()

  Return Value
  ------------
  None.

********************************************************/ 
Void CVTCEncoder::cachb_encode_SQ_tree()
{
  Int h,w,dc_h,dc_w,dc_h2,dc_w2;

  dc_h=mzte_codec.m_iDCHeight;
  dc_w=mzte_codec.m_iDCWidth;
  dc_h2=dc_h<<1;
  dc_w2=dc_w<<1;

//Modified by Sarnoff for error resilience, 3/5/99
 if(mzte_codec.m_usErrResiDisable){ //no error resi case
  for(h=0;h<dc_h;h++)
    for(w=0;w<dc_w;w++)  // 1124
      for (color=0; color<mzte_codec.m_iColors; color++) 
      {  	
	SNR_IMAGE *snr_image;
	Int tw,sw,sh,n; // 1124
	
	snr_image=&(mzte_codec.m_SPlayer[color].SNRlayer.snr_image);
	
	height=mzte_codec.m_Image[color].height;
	width=mzte_codec.m_Image[color].width;
	
	setProbModelsSQ(color);  // hjlee 0901

	coeffinfo=mzte_codec.m_SPlayer[color].coeffinfo;
	
	/* LH */
	n = 0;
	for (tw=mzte_codec.m_iDCWidth; tw < width; tw<<=1)
	{
	  sh = h << n;
	  sw = (w+dc_w) << n;
	  encodeSQBlocks(sh,sw,n);
	  n++;
	}
	/* HL */
	n = 0;
	for (tw=mzte_codec.m_iDCWidth; tw < width; tw<<=1)
	{
	  sh = (h+dc_h) << n;
	  sw = w << n;
	  encodeSQBlocks(sh,sw,n);
	  n++;
	}
	/* HH */
	n = 0;
	for (tw=mzte_codec.m_iDCWidth; tw < width; tw<<=1)
	{
	  sh = (h+dc_h) << n;
	  sw = (w+dc_w) << n;
	  encodeSQBlocks(sh,sw,n);
	  n++;
	}
      }
 }
 else{ //error resilience
  for(h=0;h<dc_h;h++)
    for(w=0;w<dc_w;w++)  // 1124
      for (color=0; color<mzte_codec.m_iColors; color++) 
      {  	
		SNR_IMAGE *snr_image;
		Int tw,sw,sh,n; // 1124
	
		snr_image=&(mzte_codec.m_SPlayer[color].SNRlayer.snr_image);
	
		height=mzte_codec.m_Image[color].height;
		width=mzte_codec.m_Image[color].width;
	
		setProbModelsSQ(color);  // hjlee 0901

		coeffinfo=mzte_codec.m_SPlayer[color].coeffinfo;

		/* LH */
		n = 0;
		for (tw=mzte_codec.m_iDCWidth; tw < width; tw<<=1)
		{
		  sh = h << n;
		  sw = (w+dc_w) << n;
		  encodeSQBlocks_ErrResi(sh,sw,n,color);
		  if (n>0 && n<5)
			  check_segment_size(color);
		  n++;
		}
		check_end_of_packet(-1);

		/* HL */
		n = 0;
		for (tw=mzte_codec.m_iDCWidth; tw < width; tw<<=1)
		{
		  sh = (h+dc_h) << n;
		  sw = w << n;
		  encodeSQBlocks_ErrResi(sh,sw,n,color);
		  if (n>0 && n<5)
			  check_segment_size(color);
		  n++;
		}
		if(TU_last==91)
			printf("Debug.\n");
		check_end_of_packet(-1);
		
		/* HH */
		n = 0;
		for (tw=mzte_codec.m_iDCWidth; tw < width; tw<<=1)
		{
		  sh = (h+dc_h) << n;	
		  sw = (w+dc_w) << n;
		  encodeSQBlocks_ErrResi(sh,sw,n,color);
		  if (n>0 && n<5)
			  check_segment_size(color);
		  n++;
		}
		check_end_of_packet(-1);
	  }
 }
//End modified by Sarnoff for error resilience, 3/5/99

}


/********************************************************
  Function Name
  -------------
  static Void encode_pixel_SQ_tree(Int h,Int w)

  Arguments
  ---------
  Int h,Int w - position of a pixel in height and width
  
  Description
  -----------
  Encoding the type and/or value of a coefficient, a
  recursive function.

  Functions Called
  ----------------
  mag_sign_encode_SQ()
  mzte_ac_encode_symbol()
  encode_pixel_SQ_tree()

  Return Value
  ------------
  None.

********************************************************/ 

Void CVTCEncoder::encode_pixel_SQ(Int h,Int w)
{
  UChar zt_type;
  Int l;

  if(coeffinfo[h][w].type == ZTR_D)
    return;

  l=xy2wvtDecompLev(w,h);


  /* code leave coefficients, value only, no type */
  if(IS_STATE_LEAF(coeffinfo[h][w].state)){
 
      /* Map type to leaf code word ZTR->0, VZTR->1 */
      zt_type = (coeffinfo[h][w].type!=ZTR);
      mzte_ac_encode_symbol(&ace,acm_type[l][CONTEXT_LINIT],zt_type);
      if (coeffinfo[h][w].type==VZTR)


	mag_sign_encode_SQ(h,w);

    return;
  }
 
  /* code zerotree symbol */

    mzte_ac_encode_symbol(&ace,acm_type[l][CONTEXT_INIT],
			  zt_type=coeffinfo[h][w].type);

  /* code magnitude and sign */
  /* For Arbitrary-Shape, out-node will always has zero coefficient,
     so only IZ or ZTR may be the zt_type -- SL*/

  switch(zt_type){
    case IZ : 
      break; /* will code the four children */
    case VZTR:
      mag_sign_encode_SQ(h,w);
    case ZTR:
	mark_ZTR_D(h,w);  /* necessary, for bandwise scan */
      break;
    case VAL:
      mag_sign_encode_SQ(h,w);
      break;
    default: 
      errorHandler("invalid zerotree symbol in single quant encode");
  }
}

#if 0
Void CVTCEncoder::encode_pixel_SQ_tree(Int h0,Int w0)
{
  UChar zt_type;
  Int h, w, k;
  Int dcc[4]; /* Don't Code Children */
  Int nSib; /* number siblings */
  Int l;

  l=xy2wvtDecompLev(w0,h0);

  nSib = (h0<(mzte_codec.m_iDCHeight<<1) && w0<(mzte_codec.m_iDCWidth<<1)) ? 1 : 4;

  /********************* CODE SIBLINGS *****************************/
  for (k=0; k<nSib; ++k)
  {
    h = h0 + (k/2);
    w = w0 + (k%2);

    /* code leave coefficients, value only, no type */
    if(IS_STATE_LEAF(coeffinfo[h][w].state))
    {
#ifdef _SHAPE_ /* skip out-node */
      if(coeffinfo[h][w].mask == 1) 
      {
#endif
	/* Map type to leaf code word ZTR->0, VZTR->1 */
	zt_type = (coeffinfo[h][w].type!=ZTR);
	mzte_ac_encode_symbol(&ace,acm_type[l][CONTEXT_LINIT],zt_type);
	if (coeffinfo[h][w].type==VZTR)
	  mag_sign_encode_SQ(h,w);

#ifdef _SHAPE_
      }
#endif
      
      continue;
    }
    
    /* code zerotree symbol */
#ifdef _SHAPE_ /* skip out-node */
    if(coeffinfo[h][w].mask == 1) 
#endif

      mzte_ac_encode_symbol(&ace,acm_type[l][CONTEXT_INIT],
			    zt_type=coeffinfo[h][w].type);

#ifdef _SHAPE_
    else
      zt_type=coeffinfo[h][w].type;
#endif
    /* code magnitude and sign */
    /* For Arbitrary-Shape, out-node will always has zero coefficient,
       so only IZ or ZTR may be the zt_type -- SL*/
    
    switch(zt_type){
      case IZ : 
	dcc[k]=0;
	break; /* will code the four children */
      case VZTR:
	mag_sign_encode_SQ(h,w);
      case ZTR:
	dcc[k]=1;
#ifdef _SHAPE_
	if(coeffinfo[h][w].mask != 1) { /* TBE for four children of out-node */
	  dcc[k] = 0;
	}
#endif
	break;
      case VAL:
	dcc[k]=0;
	mag_sign_encode_SQ(h,w);
	break;
      default: 
	errorHandler("invalid zerotree symbol in single quant encode");
    }
  }

  /********************* CODE CHILDREN *****************************/
  if (!IS_STATE_LEAF(coeffinfo[h0][w0].state) )
  {
    Int i, j;

    for (k=0; k<nSib; ++k)
    {
      if (dcc[k]==0)
      {
	h = h0 + (k/2);
	w = w0 + (k%2);  

	/* scan children */
	i=h<<1; j=w<<1;
	encode_pixel_SQ_tree(i,j);
      }
    }
  }
}

#endif

     
/********************************************************
  Function Name
  -------------
  static Void  mag_sign_encode_SQ(Int h,Int w)

  Arguments
  ---------
  Int h,Int w - position of a pixel

  Description
  -----------
  Encode the value of a coefficient.

  Functions Called
  ----------------
  mzte_ac_encode_symbol()

  Return Value
  ------------
  None.

********************************************************/ 

Void CVTCEncoder::mag_sign_encode_SQ(Int h,Int w)
{
  Int val,v_sign;
  Int l;

  if((val=coeffinfo[h][w].quantized_value)>0)
    v_sign=0;
  else
  {
    val=-val;
    v_sign=1;
  }
    
  l=xy2wvtDecompLev(w,h);

  bitplane_encode(val-1,l,WVTDECOMP_NUMBITPLANES(color,l));
  mzte_ac_encode_symbol(&ace,acm_sign[l],v_sign);

}



/*********************************************************************/
/******************************  AC  *********************************/
/**************************  Multi quant  ****************************/
/*********************************************************************/

Void CVTCEncoder::bitplane_res_encode(Int val,Int l,Int max_bplane)
{
  register int  i,k=0;

  for(i=max_bplane-1;i>=0;i--,k++)
    mzte_ac_encode_symbol(&ace,&acm_bpres[l][k],(val>>i)&1);
}

/*********************************************************************/
/******************************  AC  *********************************/
/**************************  Multi quant  ****************************/
/*********************************************************************/


/********************************************************
  Function Name
  -------------
  Void wavelet_higher_bands_encode_MQ(Int scanDirection)

  Arguments
  ---------
  Int scanDirection - 0 <=> tree, 1 <=> band

  Description
  -----------
  Control program for encoding AC information for one 
  color component. Multi quant mode.

  Functions Called
  ----------------
  cachb_encode_MQ_band()
  mzte_ac_encoder_init()
  mzte_ac_model_init()
  mzte_ac_model_done()
  mzte_ac_encoder_done()
  initContext_ * ()
  freeContext_ * ()

  Return Value
  ------------
  None.

********************************************************/ 
Void CVTCEncoder::wavelet_higher_bands_encode_MQ(Int scanDirection)
{
  noteDetail("Encoding AC (wavelet_higher_bands_encode_MQ)....");

  /* init arithmetic coder */
  mzte_ac_encoder_init(&ace);

  if (scanDirection==0)
    cachb_encode_MQ_tree();
  else
    cachb_encode_MQ_band();

  /* close arithmetic coder */
  bit_stream_length=mzte_ac_encoder_done(&ace);
}



/********************************************************
  Function Name
  -------------
  static Void mark_ZTR_D(Int h,Int w)

  Arguments
  ---------
  Int h,Int w - position of a pixel

  
  Description
  -----------
  Mark the coefficient at (h,w) and its descendents as
  zerotree descendents. 

  Functions Called
  ----------------
  mark_ZTR_D()


  Return Value
  ------------
  None.

********************************************************/ 
Void CVTCEncoder::mark_ZTR_D(Int h,Int w)
{
  Int i,j;

  i=h<<1; j=w<<1;

  if(i<height && j<width){
    coeffinfo[i][j].type     = ZTR_D;
    coeffinfo[i+1][j].type   = ZTR_D; 
    coeffinfo[i][j+1].type   = ZTR_D; 
    coeffinfo[i+1][j+1].type = ZTR_D; 
    mark_ZTR_D(i,j);
    mark_ZTR_D(i+1,j);
    mark_ZTR_D(i,j+1);
    mark_ZTR_D(i+1,j+1);
  }
}



/**********************************************************************/
/***************       MQ BAND         ********************************/
/**********************************************************************/


/********************************************************
  Function Name
  -------------
  static Void cachb_encode_MQ_band()

  Arguments
  ---------
  None.

  Description
  -----------
  Encode AC information for all color components for spatial level. 
  Multiple quant, bandwise scan.

  Functions Called
  ----------------
  clear_ZTR_D();
  codeBlocks();
  encode_pixel_MQ()

  Return Value
  ------------
  None.

********************************************************/ 
Void CVTCEncoder::cachb_encode_MQ_band()
{
  Int h,w;
  Int ac_h,ac_w,ac_h2,ac_w2;
  Int acH,acW,acH2,acW2;
  Int layer, nCol;
  Int n; /* layer index - for codeBlocks function */
  Int k; /* block jump for the layer */
     
  /* clear the ZTR_D type from the previous pass */
  for (color=0; color<NCOL; ++color)
  {      
    coeffinfo=mzte_codec.m_SPlayer[color].coeffinfo;
    height=mzte_codec.m_SPlayer[color].height;
    width=mzte_codec.m_SPlayer[color].width;

    clear_ZTR_D(coeffinfo, width, height);
  }
  for (color=0; color<NCOL; ++color)
    probModelInitMQ(color); // hjlee 0901

  acH=mzte_codec.m_iDCHeight;
  acW=mzte_codec.m_iDCWidth;
  acH2=acH<<1;
  acW2=acW<<1;

  /* scan each coefficients in the spatial layer */
  /* assume luma dimensions are >= chroma dimensions */
  layer=0;
  while(acH2<=mzte_codec.m_SPlayer[0].height 
	&& acW2<=mzte_codec.m_SPlayer[0].width)
  {
    nCol = (layer==0) ? 1 : NCOL;
    for (color=0; color < nCol; ++color)
    {      
      SNR_IMAGE *snr_image;

      noteProgress("  Coding Layer %d, Color %d", layer - (color!=0), color);

      ac_h2=acH2;
      ac_w2=acW2;
      ac_h=acH;
      ac_w=acW;

      if (color)
      {
	ac_h2>>=1;
	ac_w2>>=1;
	ac_h>>=1;
	ac_w>>=1;
      }
    
      snr_image=&(mzte_codec.m_SPlayer[color].SNRlayer.snr_image);

      coeffinfo=mzte_codec.m_SPlayer[color].coeffinfo;
      height=mzte_codec.m_SPlayer[color].height;
      width=mzte_codec.m_SPlayer[color].width;


      setProbModelsMQ(color);      
	  
      /* Go through bands */
      n = layer - (color>0);
      k = 1<<n;
      for(h=0;h<ac_h;h+=k)
		for(w=ac_w;w<ac_w2;w+=k)
		{
		  /* LH */
		  encodeMQBlocks(h,w,n);
		  
		  /* HL */
		  h += ac_h;
		  w -= ac_w;
		  encodeMQBlocks(h,w,n);
		  
		  /* HH */
		  w += ac_w;
		  encodeMQBlocks(h,w,n);
		  
		  /* Set h back to where it started. w is already there */
		  h -= ac_h;
		}
    }

    /* update ranges */
    acH=acH2;
    acW=acW2;
    acW2<<=1;
    acH2<<=1;

    ++layer;
  }

  for (color=0; color<NCOL; ++color)
    probModelFreeMQ(color);

}




/********************************************************
  Function Name
  -------------
  static Void encode_pixel_MQ_band(Int h,Int w)

  Arguments
  ---------
  Int h,Int w - position of a pixel in height and width
  
  Description
  -----------
  Encoding the type and/or value of a coefficient, a
  recursive function, multi quant mode.

  Functions Called
  ----------------
  mzte_ac_encode_symbol()
  mark_ZTR_D()
  mag_sign_encode_MQ()
 
  Return Value
  ------------
  None.

********************************************************/ 

Void CVTCEncoder::encode_pixel_MQ(Int h,Int w)
{
  Int zt_type;

  /*~~~~~~~~~~~~~~~~~ zerotree descendent or skip  ~~~~~~~~~~~~~~~~~~~*/
  if(coeffinfo[h][w].type==ZTR_D)
    return;

  /*~~~~~~~~~~~~~~ encode zero tree symbol ~~~~~~~~~~~~~~~~~~*/
  if (IS_RESID(w,h,color))
  {
    zt_type = VAL;
  }
  else
  {
    Int czt_type; /* what to put on bitstream */
    Int l;

    l=xy2wvtDecompLev(w,h);  

    zt_type = coeffinfo[h][w].type;
#ifdef _SHAPE_
    if(coeffinfo[h][w].mask==1) /* skip out-node */  
#endif
      switch(coeffinfo[h][w].state)
      {
	case S_INIT:
	  czt_type=zt_type;
	  mzte_ac_encode_symbol(&ace,acm_type[l][CONTEXT_INIT],czt_type);
	  break;
	case S_ZTR:
	  czt_type=zt_type;
	  mzte_ac_encode_symbol(&ace,acm_type[l][CONTEXT_ZTR],czt_type);
	  break;
	case S_ZTR_D:
	  czt_type=zt_type;
	  mzte_ac_encode_symbol(&ace,acm_type[l][CONTEXT_ZTR_D],czt_type);
	  break;
	case S_IZ:
	  czt_type = (zt_type!=IZ);
	  mzte_ac_encode_symbol(&ace,acm_type[l][CONTEXT_IZ],czt_type);
	  break;
	case S_LINIT: 
	  czt_type = (zt_type!=ZTR);
	  mzte_ac_encode_symbol(&ace,acm_type[l][CONTEXT_LINIT],czt_type);
	  break;
	case S_LZTR:
	  czt_type = (zt_type!=ZTR);
	  mzte_ac_encode_symbol(&ace,acm_type[l][CONTEXT_LZTR],czt_type);
	  break;
	case S_LZTR_D:
	  czt_type = (zt_type!=ZTR);
	  mzte_ac_encode_symbol(&ace,acm_type[l][CONTEXT_LZTR_D],czt_type);
	  break;
	default:
	  errorHandler("Invalid state (%d) in multi-quant encoding.", 
		       coeffinfo[h][w].state);
      }
  }


  /*~~~~~~~~~~~~~~~~ mark ztr_d and encode magnitudes ~~~~~~~~~~~~~~~~~*/
  switch(zt_type)
  {
    case ZTR:
    case ZTR_D:
#ifdef _SHAPE_
      if(coeffinfo[h][w].mask==1) /* mark ZTR-D for in-node root */
#endif
	mark_ZTR_D(h,w);
    case IZ:
      return;
    case VZTR:
      mark_ZTR_D(h,w);
    case VAL:
#ifdef _SHAPE_
      if(coeffinfo[h][w].mask==1) /* only code for in-node*/
#endif
	mag_sign_encode_MQ(h,w);
      break;
    default:
      errorHandler("Invalid type (%d) in multi-quant encoding.", zt_type);     
  }
}



/**********************************************************************/
/***************       MQ TREE         ********************************/
/**********************************************************************/


/********************************************************
  Function Name
  -------------
  static Void cachb_encode_MQ_tree()

  Arguments
  ---------
  None.

  Description
  -----------
  Encode AC information for all color components for spatial level. 
  Multiple quant, bandwise scan.

  Functions Called
  ----------------
  clear_ZTR_D();
  encode_pixel_MQ_tree()

  Return Value
  ------------
  None.

********************************************************/ 
// hjlee 0901
Void CVTCEncoder::cachb_encode_MQ_tree()
{
  Int h,w, dc_h, dc_w;

  /* clear the ZTR_D type from the previous pass */
  for (color=0; color<NCOL; ++color)
  {      
    coeffinfo=mzte_codec.m_SPlayer[color].coeffinfo;
    height=mzte_codec.m_SPlayer[color].height;
    width=mzte_codec.m_SPlayer[color].width;

    clear_ZTR_D(coeffinfo, width, height);
  }

  for (color=0; color<NCOL; ++color)
    probModelInitMQ(color);

  /* ac_h, ac_w init */
  dc_h=mzte_codec.m_iDCHeight;
  dc_w=mzte_codec.m_iDCWidth;

  for (h=0; h<dc_h; ++h)
    for (w=0; w<dc_w; ++w)
    {
      for (color=0; color<NCOL; ++color)
      {      
	SNR_IMAGE *snr_image;
	Int tw,sw,sh,n; // 1124
	
	snr_image=&(mzte_codec.m_SPlayer[color].SNRlayer.snr_image);
	
	coeffinfo=mzte_codec.m_SPlayer[color].coeffinfo;
	height=mzte_codec.m_SPlayer[color].height;
	width=mzte_codec.m_SPlayer[color].width;
      
	setProbModelsMQ(color);

	/* LH */
	n = 0;
	for (tw=mzte_codec.m_iDCWidth; tw < width; tw<<=1)
	{
	  sh = h << n;
	  sw = (w+dc_w) << n;
	  encodeMQBlocks(sh,sw,n);
	  n++;
	}
	/* HL */
	n = 0;
	for (tw=mzte_codec.m_iDCWidth; tw < width; tw<<=1)
	{
	  sh = (h+dc_h) << n;
	  sw = w << n;
	  encodeMQBlocks(sh,sw,n);
	  n++;
	}
	/* HH */
	n = 0;
	for (tw=mzte_codec.m_iDCWidth; tw < width; tw<<=1)
	{
	  sh = (h+dc_h) << n;
	  sw = (w+dc_w) << n;
	  encodeMQBlocks(sh,sw,n);
	  n++;
	}
	
#if 0
	encode_pixel_MQ_tree(h,w+dc_w);           /* LH */
	encode_pixel_MQ_tree(h+dc_h,w);           /* HL */
	encode_pixel_MQ_tree(h+dc_h,w+dc_w);      /* HH */
#endif

      }
    }

  for (color=0; color<NCOL; ++color)
    probModelFreeMQ(color);
}


#if 0
/********************************************************
  Function Name
  -------------
  static Void encode_pixel_MQ_tree(Int h,Int w)

  Arguments
  ---------
  Int h,Int w - position of a pixel in height and width
  
  Description
  -----------
  Encoding the type and/or value of a coefficient, a
  recursive function, multi quant mode.

  Functions Called
  ----------------
  mzte_ac_encode_symbol()
  mark_ZTR_D()
  mag_sign_encode_MQ()
 
  Return Value
  ------------
  None.

********************************************************/ 
// hjlee 0901


Void CVTCEncoder::encode_pixel_MQ_tree(Int h0,Int w0)
{
  Int zt_type, h, w, k;
  Int dcc[4]; /* Don't Code Children */
  Int nSib; /* number siblings */

  nSib = (h0<(mzte_codec.m_iDCHeight<<1) && w0<(mzte_codec.m_iDCWidth<<1)) ? 1 : 4;

  /********************* CODE SIBLINGS *****************************/
  for (k=0; k<nSib; ++k)
  {
    h = h0 + (k/2);
    w = w0 + (k%2);

    /* encode zero tree symbol */  
    if (IS_RESID(w,h,color))
    {
      zt_type = VAL;
    }
    else
    {
      Int czt_type; /* what to put on bitstream */
      Int l;

      l=xy2wvtDecompLev(w,h);  

      zt_type = coeffinfo[h][w].type;
#ifdef _SHAPE_
      if(coeffinfo[h][w].mask==1) /* skip out-node */  
#endif      
	switch(coeffinfo[h][w].state)
	{
	  case S_INIT:
	    czt_type=zt_type;
	    mzte_ac_encode_symbol(&ace,acm_type[l][CONTEXT_INIT],czt_type);
	    break;
	  case S_ZTR:
	    czt_type=zt_type;
	    mzte_ac_encode_symbol(&ace,acm_type[l][CONTEXT_ZTR],czt_type);
	    break;
	  case S_ZTR_D:
	    czt_type=zt_type;
	    mzte_ac_encode_symbol(&ace,acm_type[l][CONTEXT_ZTR_D],czt_type);
	    break;
	  case S_IZ:
	    czt_type = (zt_type!=IZ);
	    mzte_ac_encode_symbol(&ace,acm_type[l][CONTEXT_IZ],czt_type);
	    break;
	  case S_LINIT: 
	    czt_type = (zt_type!=ZTR);
	    mzte_ac_encode_symbol(&ace,acm_type[l][CONTEXT_LINIT],czt_type);
	    break;
	  case S_LZTR:
	    czt_type = (zt_type!=ZTR);
	    mzte_ac_encode_symbol(&ace,acm_type[l][CONTEXT_LZTR],czt_type);
	    break;
	  case S_LZTR_D:
	    czt_type = (zt_type!=ZTR);
	    mzte_ac_encode_symbol(&ace,acm_type[l][CONTEXT_LZTR_D],czt_type);
	    break;
	  default:
	    errorHandler("Invalid state (%d) in multi-quant encoding.", 
			 coeffinfo[h][w].state);
	}
    }
    
    /* mark ztr_d and encode magnitudes */
    switch(zt_type)
    {
      case ZTR:
#ifdef _SHAPE_
	if(coeffinfo[h][w].mask==1) {
#endif
	  dcc[k]=1;
	  mark_ZTR_D(h,w);
#ifdef _SHAPE_
	}
	else
	  dcc[k]=0;
#endif
	break;
      case IZ:
	dcc[k]=0;
	break;
      case VZTR:
	dcc[k]=1;
	mark_ZTR_D(h,w);
#ifdef _SHAPE_
	if(coeffinfo[h][w].mask==1) /* only code for in-node */
#endif
	  mag_sign_encode_MQ(h,w);
	break;
      case VAL:
	dcc[k]=0;
#ifdef _SHAPE_
	if(coeffinfo[h][w].mask==1) /* only code for in-node*/
#endif
	  mag_sign_encode_MQ(h,w);
	break;
      default:
	errorHandler("Invalid type in multi quant decoding.");     
    }
  }


  /********************* CODE CHILDREN *****************************/
  if (!IS_STATE_LEAF(coeffinfo[h0][w0].state))
  {
    Int i, j;

    for (k=0; k<nSib; ++k)
    {
      if (dcc[k]==0)
      {
	h = h0 + (k/2);
	w = w0 + (k%2);  
	
	/* scan children */
	i=h<<1; j=w<<1;
	encode_pixel_MQ_tree(i,j);
      }
    }
  }
}

#endif


/********************************************************
  Function Name
  -------------
  static Void  mag_sign_encode_MQ(Int h,Int w)

  Arguments
  ---------
  Int h,Int w - position of a pixel

  Description
  -----------
  Encode the value of a coefficient.

  Functions Called
  ----------------
  mzte_ac_encode_symbol()

  Return Value
  ------------
  None.

********************************************************/ 


Void CVTCEncoder::mag_sign_encode_MQ(Int h,Int w)
{
  Int val,v_sign;
  Int l;

  if(coeffinfo[h][w].skip)
    return;
    
  l=xy2wvtDecompLev(w,h);  

  if((val=coeffinfo[h][w].quantized_value)>=0)
    v_sign=0;
  else
  {
    val=-val;
    v_sign=1;
  }
  
  /* code magnitude */

  if (IS_RESID(w,h,color))
  {
    bitplane_res_encode(val,l,WVTDECOMP_RES_NUMBITPLANES(color));
  }
  else
  {
    bitplane_encode(val-1,l,WVTDECOMP_NUMBITPLANES(color,l));
    mzte_ac_encode_symbol(&ace,acm_sign[l],v_sign);
  }
}


/*************************************************************
  Function Name
  -------------
  Void encodeBlocks()

  Arguments
  ---------
  Int y, Int x  - Coordinate of upper left hand corner of block.
  Int n - Number of 4 blocks in a side of the total block. 0 means do only 
   pixel at (x,y).
  Void (*pixelFunc)(Int, Int) - Function to call for pixel locations.
  
  Description
  -----------
  Call function pixelFunc(y,x) for all pixels (x,y) in block in band scan
  manner.

  Functions Called
  ----------------
  encodeBlocks recursively.

  Return Value
  ------------
  None.

*************************************************************/
Void CVTCEncoder::encodeSQBlocks(Int y, Int x, Int n)
{
  /* Call the encoding function for the 4 block pixels */
  if (n == 0)
  {
    /* For checking scan-order : use 16x16 mono image
       for comparison with Figure 7-38 scan order table in 
       document.
    static Int i=4;

    noteStat("%d: y=%d, x=%d\n",i++, y,x);
    */

    encode_pixel_SQ(y,x);
  }
  else
  {
    Int k;

    --n;
    k = 1<<n;

    encodeSQBlocks(y,x,n);
    x += k;
    encodeSQBlocks(y,x,n);
    x -= k;
    y += k;
    encodeSQBlocks(y,x,n);
    x += k;
    encodeSQBlocks(y,x,n);
  }
}



/*************************************************************
  Function Name
  -------------
  Void encodeBlocks()

  Arguments
  ---------
  Int y, Int x  - Coordinate of upper left hand corner of block.
  Int n - Number of 4 blocks in a side of the total block. 0 means do only 
   pixel at (x,y).
  Void (*pixelFunc)(Int, Int) - Function to call for pixel locations.
  
  Description
  -----------
  Call function pixelFunc(y,x) for all pixels (x,y) in block in band scan
  manner.

  Functions Called
  ----------------
  encodeBlocks recursively.

  Return Value
  ------------
  None.

*************************************************************/
Void CVTCEncoder::encodeMQBlocks(Int y, Int x, Int n)
{
  /* Call the encoding function for the 4 block pixels */
  if (n == 0)
  {
    /* For checking scan-order : use 16x16 mono image
       for comparison with Figure 7-38 scan order table in 
       document.
    static Int i=4;

    noteStat("%d: y=%d, x=%d\n",i++, y,x);
    */

    encode_pixel_MQ(y,x);
  }
  else
  {
    Int k;

    --n;
    k = 1<<n;

    encodeMQBlocks(y,x,n);
    x += k;
    encodeMQBlocks(y,x,n);
    x -= k;
    y += k;
    encodeMQBlocks(y,x,n);
    x += k;
    encodeMQBlocks(y,x,n);
  }
}



//Added by Sarnoff for error resilience, 3/5/99

/* ----------------------------------------------------------------- */
/* ----------------- Error resilience related routines ------------- */
/* ----------------------------------------------------------------- */

/*************************************************************
  Function Name
  -------------
  Void encodeSQBlocks_ErrResi()

  Arguments
  ---------
  Int y, Int x  - Coordinate of upper left hand corner of block.
  Int n - Number of 4 blocks in a side of the total block. 0 means do only 
   pixel at (x,y).
  Int c - color.
  
  Description
  -----------
  Call function encode_pixel_SQ for all pixels (x,y) in block in band scan
  manner.

  Functions Called
  ----------------
  encodeSQBlocks recursively.

  Return Value
  ------------
  None.

*************************************************************/
Void CVTCEncoder::encodeSQBlocks_ErrResi(Int y, Int x, Int n, Int c)
{
  /* Call the encoding function for the 4 block pixels */
  if (n == 0)
  {
    /* For checking scan-order : use 16x16 mono image
       for comparison with Figure 7-38 scan order table in 
       document.
    static Int i=4;

    noteStat("%d: y=%d, x=%d\n",i++, y,x);
    */

    encode_pixel_SQ(y,x);
  }
  else
  {
    Int k;

    --n;
    k = 1<<n;

    encodeSQBlocks_ErrResi(y,x,n,c);
	if (n==4)
		check_segment_size(c);

    x += k;
    encodeSQBlocks_ErrResi(y,x,n,c);
    if (n==4)
		check_segment_size(c);

	x -= k;
    y += k;
    encodeSQBlocks_ErrResi(y,x,n,c);
    if (n==4)
		check_segment_size(c);

	x += k;
    encodeSQBlocks_ErrResi(y,x,n,c);
	if (n==4)
		check_segment_size(c);
  }
}



/* bbc, 11/5/98 */
/* ph, 11/13/98 - added color argument for band-by-band */
void CVTCEncoder::init_arith_encoder_model(Int color)
{
  /* init arithmetic coder */
  mzte_ac_encoder_init(&ace);

  if(mzte_codec.m_iScanDirection ==0 ){ /* tree depth */
    for (color=0; color<mzte_codec.m_iColors; color++) 
      probModelInitSQ(color);
  }
  else{  /* band-by-band */
      probModelInitSQ(color); /* ph - 11/13/98 */
  }
}


/* bbc, 11/6/98 */
/* ph, 11/13/98 - added color argument for band-by-band */
Void CVTCEncoder::close_arith_encoder_model(Int color, Int mode)
{
  noteProgress("  ==>E packet [TU_%d,TU_%d], l=%d bits",TU_first,TU_last,
	       packet_size+ace.bitCount+ace.followBits);

  if(mzte_codec.m_iScanDirection == 0)
  {
    /* tree depth */
    for (color=0; color<mzte_codec.m_iColors; color++) 
      /* close arithmetic coder */
      probModelFreeSQ(color);
  }
  else
  {
      probModelFreeSQ(color); /* ph - 11/13/98 */
  }
    
  bit_stream_length=mzte_ac_encoder_done(&ace);

  if(mode==1)
    write_packet_header_to_file();

  ace.bitCount=ace.followBits=0;   
}




/****************************************************/
/* to check if a segment in a packet has exceeded a */
/* threshold. Add a marker if so. bbc, 11/6/98      */
/****************************************************/
Void CVTCEncoder::check_segment_size(Int col)
{
  /* segment not long enough, bbc, 11/16/98 */
  if(packet_size+ace.bitCount+ace.followBits-prev_segs_size<
     (Int)mzte_codec.m_usSegmentThresh)
    return;

  prev_segs_size=packet_size+ace.bitCount+ace.followBits;

  /* add marker, use ZTR, bbc, 11/10/98  */
  mzte_ac_encode_symbol(&ace,&acmType[col][0][CONTEXT_INIT],ZTR);
}



/* check if end of packet is reached, bbc, 11/6/98 */
Void CVTCEncoder::check_end_of_packet(Int color)
{
  if(packet_size+ace.bitCount+ace.followBits>=
	  mzte_codec.m_usPacketThresh){
    /* write full packet to file, assume that output_buffer */
    /* is large enough to hold the arith part without writing to file */
    close_arith_encoder_model(color,1);

    flush_bits();
    flush_bytes();

    prev_segs_size=0;  /* reset segment size */

    /* start new packet */
    emit_bits((UShort)0,2); /* first bit dummy, second bit HEC=0 */
    packet_size=0;

    if (mzte_codec.m_iScanDirection==0)
      init_arith_encoder_model(color);
    else
    {
      /* don't reinitialize if color change */
      if ((TU_last-TU_max_dc+1) % mzte_codec.m_iDCHeight != 0)
	  init_arith_encoder_model(color);
    }

    TU_first=TU_last+1;
  }
  TU_last++;
}


#ifdef _DC_PACKET_
/* check if end of packet is reached, bbc, 11/18/98 */
Void CVTCEncoder::check_end_of_DC_packet(int numBP)
{
  int i;

  if(packet_size+ace.bitCount+ace.followBits>=
	  mzte_codec.m_usPacketThresh){
    noteProgress("  ==>E packet [TU_%d,TU_%d], l=%d bits",TU_first,TU_last,
		 packet_size+ace.bitCount+ace.followBits);
    
    for (i=0; i<numBP; i++) 
      mzte_ac_model_done(&(acm_bpdc[i]));
	mzte_ac_model_done(&acmType[color][0][CONTEXT_INIT]);

    bit_stream_length=mzte_ac_encoder_done(&ace);
    write_packet_header_to_file();
    
    flush_bits();
    flush_bytes();
    
    prev_segs_size=0;  /* reset segment size */
    
    /* start new packet */
    emit_bits((UShort)0,2); /* first bit dummy, second bit HEC=0 */
    packet_size=0;
    
    mzte_ac_encoder_init(&ace);
    for (i=0; i<numBP; i++) {
      acm_bpdc[i].Max_frequency = Bitplane_Max_frequency;
      mzte_ac_model_init(&(acm_bpdc[i]),2,NULL,ADAPT,1);
    }
    mzte_ac_model_init(&acmType[color][0][CONTEXT_INIT],NUMCHAR_TYPE,
						NULL,ADAPT,1); /* bbc, 11/20/98 */
    TU_first=TU_last+1;
  }
  TU_last++;
}
#endif 

/* force end of packet is reached, ph, 11/19/98 */
Void CVTCEncoder::force_end_of_packet()
{
  flush_bits();
  flush_bytes();
  
  prev_segs_size=0;  /* reset segment size */
  
  /* start new packet */
  emit_bits((UShort)0,2); /* first bit dummy, second bit HEC=0 */
  packet_size=0;
  TU_first=TU_last+1;

  TU_last++;
}
//End: Added by Sarnoff for error resilience, 3/5/99



/* $Id: ztscan_dec.cpp,v 1.1 2003/05/05 21:24:11 wmaycisco Exp $ */
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
/*  Filename: ztscan_dec.c                                  */
/*  Author: Bing-Bing CHai                                  */
/*  Date: Dec. 17, 1997                                     */
/*                                                          */
/*  Descriptions:                                           */
/*    This file contains the routines that performs         */
/*    zero tree scanning and entropy decoding.              */
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
#include "Utils.hpp"
#include "startcode.hpp"
#include "dataStruct.hpp"
#include "states.hpp"
#include "globals.hpp"
#include "errorHandler.hpp"
#include "ac.hpp"
#include "bitpack.hpp"
//#include "context.hpp"
#include "ztscan_common.hpp"
#include "ztscanUtil.hpp"

// added for FDAM1 by Samsung AIT on 2000/02/03
#ifndef	_SHAPE_
#define	_SHAPE_
#endif
// ~added for FDAM1 by Samsung AIT on 2000/02/03

static ac_decoder acd;
static COEFFINFO **coeffinfo;
static Int color;
static Int height, width;
/* The following functions assume that mzte_codec is a global variable */

//Added by Sarnoff for error resilience, 3/5/99
static Int init_ac=0 /*, prev_good_TU, prev_good_height,
		       prev_good_width */;
//End: Added by Sarnoff for error resilience, 3/5/99

/******************************************************************/
/****************************  DC  ********************************/
/******************************************************************/

/*******************************************************/
/**************  Inverse DC Prediction  ****************/
/*******************************************************/

/********************************************************
  Function Name
  -------------
  static DATA iDC_pred_pix(Int i, Int j)

  Arguments
  ---------
  Int i, Int j: Index of wavelet coefficient (row, col)
  
  Description
  -----------
  Inverse DPCM prediction for a DC coefficient, refer 
  to syntax for algorithm. 

  Functions Called
  ----------------
  None.

  Return Value
  ------------
    inverse prediction for coeffinfo[i][j].quantized_value
********************************************************/ 
Short  CVTCDecoder::iDC_pred_pix(Int i, Int j)
{
  /*  modified by Z. Wu @ OKI */

  Int pred_i, pred_j, pred_d;

//Added by Sarnoff for error resilience, 3/5/99
#ifdef _DC_PACKET_
  if(!mzte_codec.m_usErrResiDisable)
	  return 0; // no predicition currently
#endif
//End: Added by Sarnoff for error resilience, 3/5/99

  if ( i==0 || coeffinfo[i-1][j].mask == 0 )	
    pred_i = 0;
  else
    pred_i = coeffinfo[i-1][j].quantized_value;

  if ( j==0 || coeffinfo[i][j-1].mask == 0 )	
    pred_j = 0;
  else 
    pred_j = coeffinfo[i][j-1].quantized_value;

  if ( i==0 || j== 0 || coeffinfo[i-1][j-1].mask == 0 )	
    pred_d = 0;
  else	
    pred_d = coeffinfo[i-1][j-1].quantized_value;

  if ( abs(pred_d-pred_j) < abs(pred_d-pred_i))	
    return(pred_i);
  else
    return(pred_j);
}



/*****************************************************
  Function Name
  -------------
  Void iDC_predict()

  Arguments
  ---------
  None
  
  Description
  -----------
  control program for inverse DC prediction

  Functions Called
  ----------------
  iDC_pred_pix(i,j).

  Return Value
  ------------
  none
******************************************************/
Void CVTCDecoder::iDC_predict(Int color)
{
  Int i,j,dc_h,dc_w,offset_dc;

  dc_h=mzte_codec.m_iDCHeight;
  dc_w=mzte_codec.m_iDCWidth;
 
  coeffinfo=mzte_codec.m_SPlayer[color].coeffinfo;
  offset_dc=mzte_codec.m_iOffsetDC;

  for(i=0;i<dc_h;i++)
    for(j=0;j<dc_w;j++)
      if (coeffinfo[i][j].mask != 0)
	coeffinfo[i][j].quantized_value += offset_dc;

  for(i=0;i<dc_h;i++)
    for(j=0;j<dc_w;j++)
      if (coeffinfo[i][j].mask != 0)
	coeffinfo[i][j].quantized_value+=iDC_pred_pix(i,j);
}




/********************************************************
  Function Name
  -------------
  Void wavelet_dc_decode(Int c)


  Arguments
  ---------
  Int c - color component.
  
  Description
  -----------
  Control program for decode DC information for one 
  color component.

  Functions Called
  ----------------
  None.
  iDC_predict()a*
  get_param()
  cacll_decode()
  
  Return Value
  ------------
  None.

********************************************************/ 
Void CVTCDecoder::wavelet_dc_decode(Int c)
{
  noteDetail("Decoding DC (wavelet_dc_decode)....");
  color=c;

//Added by Sarnoff for error resilience, 3/5/99
  if(!mzte_codec.m_usErrResiDisable){
#ifdef _DC_PACKET_
	acmSGMK.Max_frequency = Bitplane_Max_frequency;
#endif
	// Maximum TU number excluding DC part
	if (color==0){
	  if (mzte_codec.m_iScanDirection==0) // TD
		  TU_max = mzte_codec.m_iDCHeight*mzte_codec.m_iDCWidth*9-1;
	  else // BB
		  TU_max = mzte_codec.m_iDCHeight*(3*mzte_codec.m_iWvtDecmpLev-2) - 1;
	}
  }
//End: Added by Sarnoff for error resilience, 3/5/99

  mzte_codec.m_iMean[color] = get_X_bits(8);
  /* mzte_codec.m_iQDC[color]  = get_X_bits(8); */
  mzte_codec.m_iQDC[color]  = get_param(7);

  mzte_codec.m_iOffsetDC=-get_param(7);
  mzte_codec.m_iMaxDC=get_param(7); 
  /* mzte_codec.m_iMaxDC=get_param(7)-mzte_codec.m_iOffsetDC; */ /* hjlee */

  callc_decode();
  iDC_predict(color);
  noteDetail("Completed decoding DC.");

}



/********************************************************
  Function Name
  -------------
  static Void cacll_decode()

  Arguments
  ---------
  None.

  
  Description
  -----------
  Decode DC information for one color component.

  Functions Called
  ----------------
  mzte_ac_decoder_init()
  mzte_ac_model_init()
  mzte_ac_decode_symbol()
  mzte_ac_model_done()
  mzte_ac_decoder_done()

  Return Value
  ------------
  None.

********************************************************/ 
Void CVTCDecoder::callc_decode()
{
  Int dc_h, dc_w,i,j;
  Int numBP, bp; // 1127

  dc_w=mzte_codec.m_iDCWidth;
  dc_h=mzte_codec.m_iDCHeight;

  /* init arithmetic model */
  /* ac_decoder_open(acd,NULL); */
  mzte_ac_decoder_init(&acd);

  // 1127
  numBP = ceilLog2(mzte_codec.m_iMaxDC+1); // modified by Sharp
  if ((acm_bpdc=(ac_model *)calloc(numBP,sizeof(ac_model)))==NULL)
    errorHandler("Can't allocate memory for prob model.");

  for (i=0; i<numBP; i++) {
    acm_bpdc[i].Max_frequency = Bitplane_Max_frequency;
    mzte_ac_model_init(&(acm_bpdc[i]),2,NULL,ADAPT,1);
  }

  coeffinfo=mzte_codec.m_SPlayer[color].coeffinfo;

//Modified by Sarnoff for error resilience, 3/5/99
 if(mzte_codec.m_usErrResiDisable){  //no error resilience case
  for (bp=numBP-1; bp>=0; bp--) {
    for(i=0;i<dc_h;i++)
      for(j=0;j<dc_w;j++){
		//   printf("%d %d \n", i,j);
	if(coeffinfo[i][j].mask == 1) 
	   coeffinfo[i][j].quantized_value +=
	     (mzte_ac_decode_symbol(&acd,&(acm_bpdc[bp])) << bp);
	else
	  coeffinfo[i][j].quantized_value=-mzte_codec.m_iOffsetDC;
      }
  }

  /* close arithmetic coder */
  for (i=0; i<numBP; i++) 
    mzte_ac_model_done(&(acm_bpdc[i]));
  free(acm_bpdc);

  mzte_ac_decoder_done(&acd);
 }
 else{  //error resilience case
#ifdef _DC_PACKET_
  TU_max_dc += numBP;
  TU_max += numBP;
  mzte_ac_model_init(&acmType[color][0][CONTEXT_INIT],NUMCHAR_TYPE,
					NULL,ADAPT,1);	
  while (LTU<TU_max_dc){
	bp=TU_max_dc-1-LTU;
	for(i=0;i<dc_h;i++){
		for(j=0;j<dc_w;j++){
			if(coeffinfo[i][j].mask == 1) 
			  coeffinfo[i][j].quantized_value +=
				(mzte_ac_decode_symbol(&acd,&(acm_bpdc[bp])) << bp);
			else
			  coeffinfo[i][j].quantized_value=-mzte_codec.m_iOffsetDC;
		}
		if (found_segment_error(TU_color)==1){
			// handle error 
		}
	}
	check_end_of_DC_packet(numBP);
	LTU++;
  }

  if (LTU-1 != TU_last){	  
	  for (i=0; i<numBP; ++i)
		  mzte_ac_model_done(&(acm_bpdc[i]));
	  mzte_ac_model_done(&(acmType[color][0][CONTEXT_INIT]));
	  mzte_ac_decoder_done(&acd);
  }

  mzte_ac_model_done(&acmType[color][0][CONTEXT_INIT]);
#else 
  //same as no error resi case
  for (bp=numBP-1; bp>=0; bp--) {
    for(i=0;i<dc_h;i++)
      for(j=0;j<dc_w;j++){
		//   printf("%d %d \n", i,j);
	if(coeffinfo[i][j].mask == 1) 
	   coeffinfo[i][j].quantized_value +=
	     (mzte_ac_decode_symbol(&acd,&(acm_bpdc[bp])) << bp);
	else
	  coeffinfo[i][j].quantized_value=-mzte_codec.m_iOffsetDC;
      }

  }

  /* close arithmetic coder */
  for (i=0; i<numBP; i++) 
    mzte_ac_model_done(&(acm_bpdc[i]));
  free(acm_bpdc);

  mzte_ac_decoder_done(&acd);
#endif 
 }
//End: modified by Sarnoff for error resilience, 3/5/99

}



/*********************************************************************/
/*****************************  AC  **********************************/
/*************************  Single quant  ****************************/
/*********************************************************************/

  
Int CVTCDecoder::bitplane_decode(Int l,Int max_bplane)
{
  register Int i,val=0,k=0;

  for(i=max_bplane-1;i>=0;i--,k++)
    val+=mzte_ac_decode_symbol(&acd,&acm_bpmag[l][k])<<i;

  return val;
}



/*******************************************************
  The following single quant routines are for band by
  band scan order.
*******************************************************/
/********************************************************
  Function Name
  -------------
  Void wavelet_higher_bands_decode_SQ_band(Int col)

  Arguments
  ---------
  None.

  Description
  -----------
  Control program for decoding AC information for one 
  color component. Single quant mode.

  Functions Called
  ----------------
  cachb_encode_SQ_band()
  ac_encoder_init()
  mzte_ac_model_init()
  mzte_ac_model_done()
  ac_encoder_done()

  Return Value
  ------------
  None.

********************************************************/ 
Void CVTCDecoder::wavelet_higher_bands_decode_SQ_band(Int col)
{
  SNR_IMAGE *snr_image;
    
  noteDetail("Encoding AC (wavelet_higher_bands_encode_SQ)....");

  color=col;
  snr_image=&(mzte_codec.m_SPlayer[color].SNRlayer.snr_image);
  
//Modified by Sarnoff for error resilience, 3/5/99
  if(mzte_codec.m_usErrResiDisable){  //no error resilience
  /* init arithmetic coder */
  mzte_ac_decoder_init(&acd);

  probModelInitSQ(color);  // hjlee 0901


  cachb_decode_SQ_band(snr_image);
  
    probModelFreeSQ(color);  // hjlee 0901

  mzte_ac_decoder_done(&acd);
 }
 else{ //error resilience
  init_arith_decoder_model(color);
  cachb_decode_SQ_band(snr_image);
  close_arith_decoder_model(color);
 }
//End modified by Sarnoff for error resilience, 3/5/99

  noteDetail("Completed encoding AC.");
}


/********************************************************
  Function Name
  -------------
  static Void cachb_decode_SQ_band(SNR_IMAGE *snr_img)

  Arguments
  ---------
  None.

  Description
  -----------
  Decode AC information for single quant mode, tree-depth scan.

  Functions Called
  ----------------
  codeBlocks();
  decode_pixel_SQ_band()

  Return Value
  ------------
  None.

********************************************************/ 
Void CVTCDecoder::cachb_decode_SQ_band(SNR_IMAGE *snr_image)
{
  Int h,w,ac_h,ac_w,ac_h2,ac_w2;
  Int n; /* layer index - for codeBlocks function */
  Int k; /* block jump for the layer */

  /* ac_h, ac_w init */
  ac_h2=mzte_codec.m_SPlayer[color].height;;
  ac_w2=mzte_codec.m_SPlayer[color].width;
  ac_h=ac_h2>>1;
  ac_w=ac_w2>>1;

  height=mzte_codec.m_Image[color].height;
  width=mzte_codec.m_Image[color].width;

  /* Get layer index - for codeBlocks function */
  n = -1;
  for (w=mzte_codec.m_iDCWidth; w < ac_w2; w<<=1)
    n++;

  setProbModelsSQ(color);  //  hjlee 0901

  coeffinfo=mzte_codec.m_SPlayer[color].coeffinfo;

  /* scan each coefficients in the spatial layer */  
  k = 1<<n;

//Modified by Sarnoff for error resilience, 3/5/99
 if(mzte_codec.m_usErrResiDisable){ //no error resi case
  for(h=0;h<ac_h;h+=k)
    for(w=ac_w;w<ac_w2;w+=k)
    {
      /* LH */
      decodeSQBlocks(h,w,n);

      /* HL */
      h += ac_h;
      w -= ac_w;
      decodeSQBlocks(h,w,n);

      /* HH */
      w += ac_w;
      decodeSQBlocks(h,w,n);

      /* Set h back to where it started. w is already there */
      h -= ac_h;
    }
 }
 else{ //error resi case
  while (1)  /* ph, 11/17/98 - removed: for(h=0;h<ac_h;h+=k) */
  {
    /***** ph, 11/17/98 - check for error *****/
    if (LTU>TU_max)
    {
      /* error */
      return;
    }

    /* check that current TU and this function are ok */
    get_TU_location(LTU);
    
    /* if color or band_height (spatial layer) are mismatched then leave */
    if (TU_color!=color || band_height!=ac_h)
      return;
    
    /* color and spatial layer are correct test row. Set row of TU */
    h = start_h;

    /*******************************************/

    for(w=ac_w;w<ac_w2;w+=k)
    {
      /* LH */
      decodeSQBlocks_ErrResi(h,w,n,color);
      if(n>0 && n<5) /* ph, 11/17/98 */
		if (found_segment_error(color)==1)
		{
		  /* found error */
		}
 
      /* HL */
      h += ac_h;
      w -= ac_w;
      decodeSQBlocks_ErrResi(h,w,n,color);
      if(n>0 && n<5) /* ph, 11/17/98 */
		if (found_segment_error(color)==1)
		{
		  /* found error */
		}

      /* HH */
      w += ac_w;
      decodeSQBlocks_ErrResi(h,w,n,color);
      if(n>0 && n<5) /* ph, 11/17/98 */
		if (found_segment_error(color)==1)
		{
		  /* found error */
		}


      /* Set h back to where it started. w is already there */
      h -= ac_h;  /* ph, 11/19/98 - removed not needed */
    }

    check_end_of_packet(); /* ph, 11/17/98 */
    ++LTU;
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
  Void wavelet_higher_bands_decode_SQ_tree()

  Arguments
  ---------
  None.

  Description
  -----------
  Control program for decoding AC information for single quant mode.
  All colors decoded. 

  Functions Called
  ----------------
  cachb_decode_SQ_tree()
  mzte_ac_decoder_init()
  mzte_ac_model_init()
  mzte_ac_model_done()
  mzte_ac_decoder_done()

  Return Value
  ------------
  None.

********************************************************/ 
Void CVTCDecoder::wavelet_higher_bands_decode_SQ_tree()
{
  noteDetail("Decoding AC band (wavelet_higher_bands_decode_SQ)....");
  
//Modified by Sarnoff for error resilience, 3/5/99
 if(mzte_codec.m_usErrResiDisable){
  /* init arithmetic coder */
  mzte_ac_decoder_init(&acd);
  
  for (color=0; color<mzte_codec.m_iColors; color++) 
  {  
	    probModelInitSQ(color);  // hjlee 0901
  
  }

//  cachb_decode_SQ();  // hjlee 0901
  cachb_decode_SQ_tree();

  for (color=0; color<mzte_codec.m_iColors; color++) 
    /* close arithmetic coder */
    probModelFreeSQ(color);

  mzte_ac_decoder_done(&acd);
 }
 else{ //error resi case
  /* init arithmetic coder */
  init_arith_decoder_model(color);
  cachb_decode_SQ_tree();
  close_arith_decoder_model(color);
 }
//End modified by Sarnoff for error resilience, 3/5/99

  noteDetail("Completed decoding AC band.");
}



/********************************************************
  Function Name
  -------------
  static Void cachb_decode_SQ()

  Arguments
  ---------
  None.

  Description
  -----------
  Decode AC information for one color component. 
  Single quant mode, tree-depth scan

  Functions Called
  ----------------
  decode_pixel_SQ()

  Return Value
  ------------
  None.

********************************************************/ 

Void CVTCDecoder::cachb_decode_SQ_tree()
{
//Modified by Sarnoff for error resilience, 3/5/99
 if(mzte_codec.m_usErrResiDisable){  //no error resi case
  Int h,w,dc_h,dc_w,dc_h2,dc_w2;

  dc_h=mzte_codec.m_iDCHeight;
  dc_w=mzte_codec.m_iDCWidth;
  dc_h2=dc_h<<1;
  dc_w2=dc_w<<1;

  for(h=0;h<dc_h;h++)
    for(w=0;w<dc_w;w++)  // 1127    
      for (color=0; color<mzte_codec.m_iColors; color++) 
      {  	
	SNR_IMAGE *snr_image;
	int tw,sw,sh,n; // 1127
	
	snr_image=&(mzte_codec.m_SPlayer[color].SNRlayer.snr_image);
 
	height=mzte_codec.m_SPlayer[color].height;
	width=mzte_codec.m_SPlayer[color].width;

	setProbModelsSQ(color);  // hjlee 0901


	coeffinfo=mzte_codec.m_SPlayer[color].coeffinfo;

	/* LH */
	n = 0;
	for (tw=mzte_codec.m_iDCWidth; tw < width; tw<<=1)
	{
	  sh = h << n;
	  sw = (w+dc_w) << n;
	  decodeSQBlocks(sh,sw,n);
	  n++;
	}
	/* HL */
	n = 0;
	for (tw=mzte_codec.m_iDCWidth; tw < width; tw<<=1)
	{
	  sh = (h+dc_h) << n;
	  sw = w << n;
	  decodeSQBlocks(sh,sw,n);
	  n++;
	}
	/* HH */
	n = 0;
	for (tw=mzte_codec.m_iDCWidth; tw < width; tw<<=1)
	{
	  sh = (h+dc_h) << n;
	  sw = (w+dc_w) << n;
	  decodeSQBlocks(sh,sw,n);
	  n++;
	}

      }
 }
 else{ //error resilience case
  Int dc_h,dc_w;
  Int tw,sw,sh,n;

  dc_h=mzte_codec.m_iDCHeight;
  dc_w=mzte_codec.m_iDCWidth;

  /* rewrote for error resilience, bbc, 11/9/98 */
  while(LTU<=TU_max){
    get_TU_location(LTU);
    height=mzte_codec.m_SPlayer[TU_color].height;
    width=mzte_codec.m_SPlayer[TU_color].width;
	
    setProbModelsSQ(TU_color);
    coeffinfo=mzte_codec.m_SPlayer[TU_color].coeffinfo;
    color=TU_color;

    /* decoding one TU */
    n = 0;
    for (tw=mzte_codec.m_iDCWidth; tw < width; tw<<=1){
      sh = start_h << n;
      sw = start_w << n;
      decodeSQBlocks_ErrResi(sh,sw,n,TU_color);
      if(n>0 && n<5)
		found_segment_error(TU_color);
      n++;
    }

    check_end_of_packet();  /* error resilience code, bbc, 11/9/98 */
    LTU++;
  }    
 }
//End modified by Sarnoff for error resilience, 3/5/99
}



/********************************************************
  Function Name
  -------------
  static Void decode_pixel_SQ(Int h,Int w)

  Arguments
  ---------
  Int h,Int w - position of a pixel in height and width
  
  Description
  -----------
  Decoding the type and/or value of a coefficient, a
  recursive function.

  Functions Called
  ----------------
  mag_sign_decode_SQ()
  mzte_ac_decode_symbol()
  decode_pixel_SQ()

  Return Value
  ------------
  None.

********************************************************/ 
// hjlee 0901
Void CVTCDecoder::decode_pixel_SQ(Int h,Int w)
{

  UChar zt_type;
  Int l;

  if(coeffinfo[h][w].type == ZTR_D)
    return;


  l=xy2wvtDecompLev(w,h);


  /* decode leave coefficients, value only */  
  if(IS_STATE_LEAF(coeffinfo[h][w].state)){ /* zero value. no sign */
   


      /* Map leaf code word to type 0->ZTR, 1->VZTR */
      coeffinfo[h][w].type = 
	mzte_ac_decode_symbol(&acd,acm_type[l][CONTEXT_LINIT]) ? VZTR : ZTR;
      if (coeffinfo[h][w].type==VZTR)


	mag_sign_decode_SQ(h,w);

      else
	coeffinfo[h][w].quantized_value = 0;  
      
      return;
  }
  
  
  /* decode zero tree symbols */

    coeffinfo[h][w].type=zt_type=


    mzte_ac_decode_symbol(&acd,acm_type[l][CONTEXT_INIT]);

   
  /* code magnitude and sign */
  switch(zt_type){
    case IZ :
      break;
    case VZTR:
      mag_sign_decode_SQ(h,w);
    case ZTR:
      mark_ZTR_D(h,w);  /* necessary for checking purpose bandwise scan */
      return;
    case VAL:
      mag_sign_decode_SQ(h,w);
      break;
    default: 
      errorHandler("Invalid zerotree symbol in single quant decode");
  }

#if 0
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

    /* decode leave coefficients, value only */
    if(IS_STATE_LEAF(coeffinfo[h][w].state)){ /* zero value. no sign */
#ifdef _SHAPE_
      if(coeffinfo[h][w].mask==1)
      {
#endif      

	/* Map leaf code word to type 0->ZTR, 1->VZTR */
	coeffinfo[h][w].type = 
	  mzte_ac_decode_symbol(&acd,acm_type[l][CONTEXT_LINIT]) ? VZTR : ZTR;
	if (coeffinfo[h][w].type==VZTR)


	  mag_sign_decode_SQ(h,w);
	else
	  coeffinfo[h][w].quantized_value = 0;

#ifdef _SHAPE_
      }
      else 
	coeffinfo[h][w].quantized_value = 0;
#endif      
      
      continue;
    }
    
    
    /* decode zero tree symbols */
#ifdef _SHAPE_
    if(coeffinfo[h][w].mask==1)
#endif
      coeffinfo[h][w].type=zt_type=

      mzte_ac_decode_symbol(&acd,acm_type[l][CONTEXT_INIT]);
#ifdef _SHAPE_
    else
      coeffinfo[h][w].type=zt_type = IZ;
#endif

    /* code magnitude and sign */
    switch(zt_type){
      case IZ :
	dcc[k]=0;
	break;
      case VZTR:
	mag_sign_decode_SQ(h,w);
      case ZTR:
	dcc[k]=1;
	break;
      case VAL:
	dcc[k]=0;
	mag_sign_decode_SQ(h,w);
	break;
      default: 
	errorHandler("Invalid zerotree symbol in single quant decode");
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
	decode_pixel_SQ_tree(i,j);
      }
    }
  }

#endif

}


     
/********************************************************
  Function Name
  -------------
  static Void  mag_sign_decode_SQ(Int h,Int w)

  Arguments
  ---------
  Int h,Int w - position of a pixel

  Description
  -----------
  Decode the value of a coefficient.

  Functions Called
  ----------------
  mzte_ac_decode_symbol()

  Return Value
  ------------
  None.

********************************************************/ 



Void  CVTCDecoder::mag_sign_decode_SQ(Int h,Int w)
{
  Int value,v_sign;
  Int l;
    
  l=xy2wvtDecompLev(w,h);


  value=bitplane_decode(l,WVTDECOMP_NUMBITPLANES(color,l))+1;
  v_sign=mzte_ac_decode_symbol(&acd,acm_sign[l]);
  coeffinfo[h][w].quantized_value=(v_sign==0) ? value : -value;

}


/*********************************************************************/
/******************************  AC  *********************************/
/**************************  Multi quant  ****************************/
/*********************************************************************/


Int CVTCDecoder::bitplane_res_decode(Int l,Int max_bplane)
{
  register Int i,val=0,k=0;

  for(i=max_bplane-1;i>=0;i--,k++)
    val+=mzte_ac_decode_symbol(&acd,&acm_bpres[l][k])<<i;

  return val;
}

/********************************************************
  Function Name
  -------------
  Void wavelet_higher_bands_decode_MQ(Int scanDirection)

  Arguments
  ---------
  Int scanDirection - 0 <=> tree, 1 <=> band
  
  Description
  -----------
  Control program for decoding AC information for one 
  color component. Multi quant mode.

  Functions Called
  ----------------
  cachb_decode_MQ_band()
  mzte_ac_decoder_init()
  mzte_ac_model_init()
  mzte_ac_model_done()
  mzte_ac_decoder_done()

  Return Value
  ------------
  None.

********************************************************/ 
Void CVTCDecoder::wavelet_higher_bands_decode_MQ(Int scanDirection)
{
  noteDetail("Decoding AC band (wavelet_higher_bands_decode_MQ)....");

  /* init arithmetic coder */
  mzte_ac_decoder_init(&acd);
  
  if (scanDirection==0)
    cachb_decode_MQ_tree();
  else
    cachb_decode_MQ_band();

  /* close arithmetic coder */
  mzte_ac_decoder_done(&acd);
}



/**********************************************************************/
/***************       MQ BAND         ********************************/
/**********************************************************************/


/********************************************************
  Function Name
  -------------
  static Void cachb_decode_MQ_band()

  Arguments
  ---------
  None.

  Description
  -----------
  Decode AC information for one color component. 
  Multiple quant, bandwise scan.

  Functions Called
  ----------------
  clear_ZTR_D();
  codeBlocks();
  decode_pixel_MQ_band()

  Return Value
  ------------
  None.

********************************************************/ 
Void CVTCDecoder::cachb_decode_MQ_band()
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
    probModelInitMQ(color);
 
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
		  decodeMQBlocks(h,w,n);
		  
		  /* HL */
		  h += ac_h;
		  w -= ac_w;
		  decodeMQBlocks(h,w,n);
		  
		  /* HH */
		  w += ac_w;
		  decodeMQBlocks(h,w,n);
		  
		  /* Set h back to where it started. w is already there */
		  h -= ac_h;
		}
    }

    /* update ranges */
    acH=acH2;
    acW=acW2;
    acW2<<=1;
    acH2<<=1;

    layer++;
  }
  for (color=0; color<NCOL; ++color)
    probModelFreeMQ(color);

}


/********************************************************
  Function Name
  -------------
  static Void decode_pixel_MQ_band(Int h,Int w)

  Arguments
  ---------
  Int h,Int w - position of a pixel in height and width
  
  Description
  -----------
  Decoding the type and/or value of a coefficient, a
  recursive function, multi quant mode.

  Functions Called
  ----------------
  mzte_ac_decode_symbol()
  mark_ZTR_D()
  mag_sign_decode_MQ()

  Return Value
  ------------
  None.

********************************************************/ 

//Void CVTCDecoder::decode_pixel_MQ_band(Int h,Int w)
Void CVTCDecoder::decode_pixel_MQ(Int h,Int w) // 1124
{
  Int zt_type;

  /*~~~~~~~~~~~~~~~~~ zerotree descendent ~~~~~~~~~~~~~~~~~~~*/  
  if(coeffinfo[h][w].type==ZTR_D)
    return;

  /*~~~~~~~~~~~~~~ decode zero tree symbol ~~~~~~~~~~~~~~~~~~*/
  if (IS_RESID(w,h,color))
  {
    zt_type=VAL; /* tmp assign. for next switch statement */
  }
  else
  {
    Int czt_type; /* what to put on bitstream */
    Int l;

    l=xy2wvtDecompLev(w,h);  

    zt_type = coeffinfo[h][w].type;
#ifdef _SHAPE_
    if(coeffinfo[h][w].mask==1) /* skip out-node */  
    {
#endif
      switch(coeffinfo[h][w].state)
      {
	case S_INIT:
	  czt_type=mzte_ac_decode_symbol(&acd,acm_type[l][CONTEXT_INIT]);
	  coeffinfo[h][w].type=zt_type=czt_type;
	  break;
	case S_ZTR:
	  czt_type=mzte_ac_decode_symbol(&acd,acm_type[l][CONTEXT_ZTR]);
	  coeffinfo[h][w].type=zt_type=czt_type;
	  break;
	case S_ZTR_D:
	  czt_type=mzte_ac_decode_symbol(&acd,acm_type[l][CONTEXT_ZTR_D]);
	  coeffinfo[h][w].type=zt_type=czt_type;
	  break;
	case S_IZ:
	  czt_type=mzte_ac_decode_symbol(&acd,acm_type[l][CONTEXT_IZ]);
	  coeffinfo[h][w].type=zt_type = czt_type ? VAL : IZ;
	  break;
	case S_LINIT: 
	  czt_type=mzte_ac_decode_symbol(&acd,acm_type[l][CONTEXT_LINIT]);
	  coeffinfo[h][w].type=zt_type = czt_type ? VZTR : ZTR;
	  break;
	case S_LZTR:
	  czt_type=mzte_ac_decode_symbol(&acd,acm_type[l][CONTEXT_LZTR]);
	  coeffinfo[h][w].type=zt_type = czt_type ? VZTR : ZTR;
	  break;
	case S_LZTR_D:
	  czt_type=mzte_ac_decode_symbol(&acd,acm_type[l][CONTEXT_LZTR_D]);
	  coeffinfo[h][w].type=zt_type = czt_type ? VZTR : ZTR;
	  break;
	default:
	  errorHandler("Invalid state (%d) in multi-quant encoding.", 
		       coeffinfo[h][w].state);
      }
#ifdef _SHAPE_
    }
    else /* treat out-node as isolated zero for decoding purpose */
    {
      switch(coeffinfo[h][w].state)
      {
      case S_INIT:
      case S_ZTR:
      case S_ZTR_D:
      case S_IZ:
	zt_type = coeffinfo[h][w].type = IZ;
	break;
      case S_LINIT: 
      case S_LZTR:
      case S_LZTR_D:
	zt_type = coeffinfo[h][w].type = ZTR;
	break;
      default:
	errorHandler("Invalid state (%d) in multi-quant encoding.", 
		     coeffinfo[h][w].state);
      }
    }
#endif

  }

  /*~~~~~~~~~~~~~~~~ mark ztr_d and encode magnitudes ~~~~~~~~~~~~~~~~~*/
  switch(zt_type)
  {
    case ZTR:
#ifdef _SHAPE_
      if(coeffinfo[h][w].mask!=1)
	return;
#endif
    case ZTR_D:
      mark_ZTR_D(h,w);
    case IZ:
      coeffinfo[h][w].quantized_value=0;
      return;
    case VZTR:
      mark_ZTR_D(h,w);
    case VAL:
#ifdef _SHAPE_
      if(coeffinfo[h][w].mask==1) 
#endif
	mag_sign_decode_MQ(h,w);
      break;
    default:
      errorHandler("Invalid type in multi quant decoding.");     
  }
}



/**********************************************************************/
/***************       MQ TREE         ********************************/
/**********************************************************************/

/********************************************************
  Function Name
  -------------
  static Void cachb_decode_MQ_tree()

  Arguments
  ---------
  None.

  Description
  -----------
  Decode AC information for one color component. 
  Multiple quant, treewise scan.

  Functions Called
  ----------------
  clear_ZTR_D();
  decode_pixel_MQ_tree()

  Return Value
  ------------
  None.

********************************************************/ 
Void CVTCDecoder::cachb_decode_MQ_tree()
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
	int tw,sw,sh,n;  // 1124
	
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
	  decodeMQBlocks(sh,sw,n);
	  n++;
	}
	/* HL */
	n = 0;
	for (tw=mzte_codec.m_iDCWidth; tw < width; tw<<=1)
	{
	  sh = (h+dc_h) << n;
	  sw = w << n;
	  decodeMQBlocks(sh,sw,n);
	  n++;
	}
	/* HH */
	n = 0;
	for (tw=mzte_codec.m_iDCWidth; tw < width; tw<<=1)
	{
	  sh = (h+dc_h) << n;
	  sw = (w+dc_w) << n;
	  decodeMQBlocks(sh,sw,n);
	  n++;
	}

#if 0
	decode_pixel_MQ_tree(h,w+dc_w);      /* LH */
	decode_pixel_MQ_tree(h+dc_h,w);      /* HL */
	decode_pixel_MQ_tree(h+dc_h,w+dc_w); /* HH */
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
  static Void decode_pixel_MQ_tree(Int h,Int w)

  Arguments
  ---------
  Int h,Int w - position of a pixel in height and width
  
  Description
  -----------
  Decoding the type and/or value of a coefficient, a
  recursive function, multi quant mode.

  Functions Called
  ----------------
  mzte_ac_decode_symbol()
  mark_ZTR_D()
  mag_sign_decode_MQ()

  Return Value
  ------------
  None.

********************************************************/ 


Void CVTCDecoder::decode_pixel_MQ_tree(Int h0,Int w0)
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

    /* decode zero tree symbol */  
    if (IS_RESID(w,h,color))
    {
      zt_type=VAL;
    }
    else
    {
      Int czt_type; /* what to put on bitstream */
      Int l;

      l=xy2wvtDecompLev(w,h);  

      zt_type = coeffinfo[h][w].type;
#ifdef _SHAPE_
      if(coeffinfo[h][w].mask==1) /* skip out-node */  
      {
#endif
	switch(coeffinfo[h][w].state)
	{
	  case S_INIT:
	    czt_type=mzte_ac_decode_symbol(&acd,acm_type[l][CONTEXT_INIT]);
	    coeffinfo[h][w].type=zt_type=czt_type;
	    break;
	  case S_ZTR:
	    czt_type=mzte_ac_decode_symbol(&acd,acm_type[l][CONTEXT_ZTR]);
	    coeffinfo[h][w].type=zt_type=czt_type;
	    break;
	  case S_ZTR_D:
	    czt_type=mzte_ac_decode_symbol(&acd,acm_type[l][CONTEXT_ZTR_D]);
	    coeffinfo[h][w].type=zt_type=czt_type;
	    break;
	  case S_IZ:
	    czt_type=mzte_ac_decode_symbol(&acd,acm_type[l][CONTEXT_IZ]);
	    coeffinfo[h][w].type=zt_type = czt_type ? VAL : IZ;
	    break;
	  case S_LINIT: 
	    czt_type=mzte_ac_decode_symbol(&acd,acm_type[l][CONTEXT_LINIT]);
	    coeffinfo[h][w].type=zt_type = czt_type ? VZTR : ZTR;
	    break;
	  case S_LZTR:
	  czt_type=mzte_ac_decode_symbol(&acd,acm_type[l][CONTEXT_LZTR]);
	  coeffinfo[h][w].type=zt_type = czt_type ? VZTR : ZTR;
	  break;
	  case S_LZTR_D:
	    czt_type=mzte_ac_decode_symbol(&acd,acm_type[l][CONTEXT_LZTR_D]);
	    coeffinfo[h][w].type=zt_type = czt_type ? VZTR : ZTR;
	    break;
	default:
	  errorHandler("Invalid state (%d) in multi-quant encoding.", 
		       coeffinfo[h][w].state);
	}
#ifdef _SHAPE_
      }
      else /* treat out-node as isolated zero for decoding purpose */
      {
	switch(coeffinfo[h][w].state)
	  {
	  case S_INIT:
	  case S_ZTR:
	  case S_ZTR_D:
	  case S_IZ:
	    zt_type = coeffinfo[h][w].type = IZ;
	    break;
	  case S_LINIT: 
	  case S_LZTR:
	  case S_LZTR_D:
	    zt_type = coeffinfo[h][w].type = ZTR;
	    break;
	  default:
	    errorHandler("Invalid state (%d) in multi-quant encoding.", 
			 coeffinfo[h][w].state);
	  }
      }
#endif
      
    }
    /* mark ztr_d and decode magnitudes */
    switch(zt_type)
    {
      case ZTR:
#ifdef _SHAPE_
	if(coeffinfo[h][w].mask==1) { 
#endif
	  dcc[k]=1;
	  mark_ZTR_D(h,w); /* here it's just to zero out descendents */
#ifdef _SHAPE_
	}
	else {
	  dcc[k]=0;
	}
#endif
	coeffinfo[h][w].quantized_value=0;
	break;
      case IZ:
	dcc[k]=0;
	coeffinfo[h][w].quantized_value=0;
	break;
      case VZTR:
	dcc[k]=1;
	mark_ZTR_D(h,w); /*  here it's just to zero out descendents */
#ifdef _SHAPE_
	if(coeffinfo[h][w].mask==1) 
#endif
	mag_sign_decode_MQ(h,w);
	break;
      case VAL:
	dcc[k]=0;
#ifdef _SHAPE_
	if(coeffinfo[h][w].mask==1) 
#endif
	  mag_sign_decode_MQ(h,w);
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
	decode_pixel_MQ_tree(i,j);
      }
    }
  }
}


#endif


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
Void CVTCDecoder::mark_ZTR_D(Int h,Int w)
{
  Int i,j;

  i=h<<1; j=w<<1;

  if(i<height && j<width)
  {
    coeffinfo[i][j].quantized_value     = 0;
    coeffinfo[i+1][j].quantized_value   = 0;
    coeffinfo[i][j+1].quantized_value   = 0;
    coeffinfo[i+1][j+1].quantized_value = 0;
    
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




/********************************************************
  Function Name
  -------------
  static Void  mag_sign_decode_MQ(Int h,Int w)

  Arguments
  ---------
  Int h,Int w - position of a pixel

  Description
  -----------
  Decode the value of a coefficient.

  Functions Called
  ----------------
  mzte_ac_decode_symbol()

  Return Value
  ------------
  None.

********************************************************/ 
// hjlee 0901


 
Void CVTCDecoder::mag_sign_decode_MQ(Int h,Int w)
{
  Int val,v_sign;
  Int l;

  if(coeffinfo[h][w].skip)
  {
    coeffinfo[h][w].quantized_value=0;
    return;
  }
    
  l=xy2wvtDecompLev(w,h);


  if (IS_RESID(w,h,color))
  {
    val=bitplane_res_decode(l,WVTDECOMP_RES_NUMBITPLANES(color));
    coeffinfo[h][w].quantized_value=val;
  }
  else
  {
    val=bitplane_decode(l,WVTDECOMP_NUMBITPLANES(color,l))+1;
    v_sign=mzte_ac_decode_symbol(&acd,acm_sign[l]);
    coeffinfo[h][w].quantized_value=(v_sign==0) ? val : -val;
  }

}


/*************************************************************
  Function Name
  -------------
  Void decodeSQBlocks()

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
  decodeBlocks recursively.

  Return Value
  ------------
  None.

*************************************************************/
Void CVTCDecoder::decodeSQBlocks(Int y, Int x, Int n)
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

    decode_pixel_SQ(y,x);
  }
  else
  {
    Int k;

    --n;
    k = 1<<n;

    decodeSQBlocks(y,x,n);
    x += k;
    decodeSQBlocks(y,x,n);
    x -= k;
    y += k;
    decodeSQBlocks(y,x,n);
    x += k;
    decodeSQBlocks(y,x,n);
  }
}


/*************************************************************
  Function Name
  -------------
  Void decodeSQBlocks()

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
  decodeBlocks recursively.

  Return Value
  ------------
  None.

*************************************************************/
Void CVTCDecoder::decodeMQBlocks(Int y, Int x, Int n)
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

    decode_pixel_MQ(y,x);
  }
  else
  {
    Int k;

    --n;
    k = 1<<n;

    decodeMQBlocks(y,x,n);
    x += k;
    decodeMQBlocks(y,x,n);
    x -= k;
    y += k;
    decodeMQBlocks(y,x,n);
    x += k;
    decodeMQBlocks(y,x,n);
  }
}


//Added by Sarnoff for error resilience, 3/5/99

/* ----------------------------------------------------------------- */
/* ----------------- Error resilience related routines ------------- */
/* ----------------------------------------------------------------- */

/*************************************************************
  Function Name
  -------------
  Void decodeSQBlocks_ErrResi()

  Arguments
  ---------
  Int y, Int x  - Coordinate of upper left hand corner of block.
  Int n - Number of 4 blocks in a side of the total block. 0 means do only 
   pixel at (x,y).
  Int c - color.
  
  Description
  -----------
  Call function decode_pixel_SQ for all pixels (x,y) in block in band scan
  manner.

  Functions Called
  ----------------
  decodeSQBlocks recursively.

  Return Value
  ------------
  None.

*************************************************************/
Void CVTCDecoder::decodeSQBlocks_ErrResi(Int y, Int x, Int n, Int c)
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

    decode_pixel_SQ(y,x);
  }
  else
  {
    Int k;

    --n;
    k = 1<<n;

    decodeSQBlocks_ErrResi(y,x,n,c);
	if (n==4)
		found_segment_error(c);

    x += k;
    decodeSQBlocks_ErrResi(y,x,n,c);
	if (n==4)
		found_segment_error(c);

    x -= k;
    y += k;
    decodeSQBlocks_ErrResi(y,x,n,c);
	if (n==4)
		found_segment_error(c);

    x += k;
    decodeSQBlocks_ErrResi(y,x,n,c);
	if (n==4)
		found_segment_error(c);
  }
}



/* init all ac_decoder and models, bbc, 11/6/98 */
/* ph, 11/13/98 - added color argument for band-by-band */
Void CVTCDecoder::init_arith_decoder_model(Int color)
{
  if(init_ac!=0) /* check for not closed ac coder. bbc, 7/2/98 */
    errorHandler("didn't close arithmetic decoder before.");
  else
    init_ac=1;

   /* init arithmetic coder */
  mzte_ac_decoder_init(&acd);
  
  if(mzte_codec.m_iScanDirection ==0 ){ /* tree depth */
    for (color=0; color<mzte_codec.m_iColors; color++) 
      probModelInitSQ(color);
  }
  else {   /* band-by-band */
    probModelInitSQ(color); /* ph - 11/13/98 */
  }
}  


/* close all ac_decoder and models, bbc, 11/6/98 */
/* ph, 11/13/98 - added color argument for band-by-band */
Void CVTCDecoder::close_arith_decoder_model(Int color)
{
  if(init_ac==0) /* didn't init ac before. */
    return;
  else
    init_ac=0;

  if(errSignal ==0)
    noteProgress("  ==>D found packet at [TU_%d,TU_%d], l=%d bits",TU_first,
                 TU_last,packet_size-16);

  if(mzte_codec.m_iScanDirection==0){ /* TD */
    for (color=0; color<mzte_codec.m_iColors; color++) 
      /* close arithmetic coder */
      probModelFreeSQ(color);
  }
  else{    /* BB */
      probModelFreeSQ(color); /* ph - 11/13/98 */
  }

  mzte_ac_decoder_done(&acd);
}

/*****************************************************/
/* to check if a segment in a packet has exceeded a  */
/* threshold. Look for a marker if so. bbc, 11/6/98  */
/*****************************************************/
Int CVTCDecoder::found_segment_error(Int col)
{
  /* segment not long enough, bbc, 11/16/98 */
  if(packet_size-16-prev_segs_size<(Int)mzte_codec.m_usSegmentThresh) 
    return 2;

  noteProgress("\tDecode segment marker.");

  prev_segs_size=packet_size-16;

  /* search for the marker, bbc, 11/10/98
  if(mzte_ac_decode_symbol(&acd,acm_type[0][CONTEXT_INIT])==ZTR)
    return 0; */

  if(mzte_ac_decode_symbol(&acd,&acmType[col][0][CONTEXT_INIT])==ZTR)
    return 0;

  prev_segs_size=0;
  return 1;
}



/* Check if a new packet will start, bbc, 11/9/98 */
Void CVTCDecoder::check_end_of_packet()
{
  if(LTU==TU_last){  /* reach the end of a packet */
    close_arith_decoder_model(color);
    align_byte();

    if(TU_last == TU_max){  /* successfully decoded last packet */
      if(end_of_stream())
         error_bits_stat(0);
      else{
        while(!end_of_stream())
          get_X_bits(8);
        rewind_bits(16);
        error_bits_stat(1);
      }
      return;
    }

    packet_size=0; 
    prev_segs_size=0;
    /* start new packet */

    CTU_no=get_err_resilience_header();
    LTU=CTU_no-1;
      
    get_TU_location(TU_first-1);
    if(mzte_codec.m_iScanDirection==0)
    { 
      /* TD */
      if(TU_color==0)
        set_prev_good_TD_segment(TU_first-1,
                               ((start_h+1)<<(mzte_codec.m_iWvtDecmpLev-1))-1,
                               ((start_w+1)<<(mzte_codec.m_iWvtDecmpLev-1))-1);
      else
        set_prev_good_TD_segment(TU_first-1,
				 ((start_h+1)<<(mzte_codec.m_iWvtDecmpLev-2))-1,
				 ((start_w+1)<<(mzte_codec.m_iWvtDecmpLev-2))-1);
    }
    else
    {
      /* BB */

    }

    if(CTU_no>TU_max)
      return;

    /*	Int iTmp = */get_X_bits(1);
    //if(iTmp != 0) /* no repeated header info for now, bbc, 6/27/98 */
      /* errorHandler("Error in decoding HEC.") */;

    if (mzte_codec.m_iScanDirection==0)
      init_arith_decoder_model(color);
    else
    {
      /* don't reinitialize if color change */
      if ((LTU-TU_max_dc+1) % mzte_codec.m_iDCHeight != 0)
	init_arith_decoder_model(color);
    }
  }
}

    
#ifdef _DC_PACKET_
/* check if end of packet is reached, bbc, 11/18/98 */
Void CVTCDecoder::check_end_of_DC_packet(int numBP)
{
  int i;

  if(LTU==TU_last){  
    if(errSignal ==0)
      noteProgress("  ==>D found packet at [TU_%d,TU_%d], l=%d bits",
		   TU_first,TU_last,packet_size-16);
    
    for (i=0; i<numBP; i++) 
      mzte_ac_model_done(&(acm_bpdc[i]));
	mzte_ac_model_done(&acmType[color][0][CONTEXT_INIT]);
    mzte_ac_decoder_done(&acd);
    align_byte();
    
    /* start new packet */
    packet_size=0; 
    prev_segs_size=0;
    
    CTU_no=get_err_resilience_header();
    LTU=CTU_no-1;
    
    if(get_X_bits(1) != 0) 
      /* errorHandler("Error in decoding HEC.") */;
    
    mzte_ac_decoder_init(&acd);
    for (i=0; i<numBP; i++) {
      acm_bpdc[i].Max_frequency = Bitplane_Max_frequency;
      mzte_ac_model_init(&(acm_bpdc[i]),2,NULL,ADAPT,1);
    }
    mzte_ac_model_init(&acmType[color][0][CONTEXT_INIT],NUMCHAR_TYPE,
						NULL,ADAPT,1); /* bbc, 11/20/98 */
    coeffinfo=mzte_codec.m_SPlayer[color].coeffinfo;
  }
}
#endif 

//End: Added by Sarnoff for error resilience, 3/5/99

/* $Id: vtc_main_computePSNR.cpp,v 1.1 2005/05/09 21:29:47 wmaycisco Exp $ */
/****************************************************************************/
/*   MPEG4 Visual Texture Coding (VTC) Mode Software                        */
/*                                                                          */
/*   This software was developed by                                         */
/*   Sarnoff Corporation                   and    Texas Instruments         */
/*   Iraj Sodagar   (iraj@sarnoff.com)           Jie Liang (liang@ti.com)   */
/*   Hung-Ju Lee    (hjlee@sarnoff.com)                                     */
/*   Paul Hatrack   (hatrack@sarnoff.com)                                   */
/*   Shipeng Li     (shipeng@sarnoff.com)                                   */
/*   Bing-Bing Chai (bchai@sarnoff.com)                                     */
/*                                                                          */
/* In the course of development of the MPEG-4 standard. This software       */
/* module is an implementation of a part of one or more MPEG-4 tools as     */
/* specified by the MPEG-4 standard.                                        */
/*                                                                          */
/* The copyright of this software belongs to ISO/IEC. ISO/IEC gives use     */
/* of the MPEG-4 standard free license to use this  software module or      */
/* modifications thereof for hardware or software products claiming         */
/* conformance to the MPEG-4 standard.                                      */
/*                                                                          */
/* Those Intending to use this software module in hardware or software      */
/* products are advised that use may infringe existing  patents. The        */
/* original developers of this software module and their companies, the     */
/* subsequent editors and their companies, and ISO/IEC have no liability    */
/* and ISO/IEC have no liability for use of this software module or         */
/* modification thereof in an implementation.                               */
/*                                                                          */
/* Permission is granted to MPEG memebers to use, copy, modify,             */
/* and distribute the software modules ( or portions thereof )              */
/* for standardization activity within ISO/IEC JTC1/SC29/WG11.              */
/*                                                                          */
/* Copyright (C) 1998  Sarnoff Corporation and Texas Instruments            */ 
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
#include "dwt.h"
#include "states.hpp"



Void CVTCCommon::ComputePSNR(UChar *orgY, UChar *recY, 
		 UChar *maskY,
		 UChar *orgU, UChar *recU, 
		 UChar *maskU,
		 UChar *orgV, UChar *recV, 
		 UChar *maskV,
		 Int width, Int height, Int stat)
{
    Int i;
    Double quad, quad_Cr, quad_Cb, diff;
    Double SNR_l, SNR_Cb=0, SNR_Cr=0;
    Int  num,colors;
    Int peakvalue = 255;
    Int infSNR_l, infSNR_Cb=0, infSNR_Cr=0;

    quad  = quad_Cr = quad_Cb = 0.0;
    
    if (orgU==NULL || recU==NULL || orgV==NULL || recV==NULL)
      colors=1;
    else
      colors=3;

    /* Calculate luminance PSNR */
    num = 0;
    for (i=0; i<width*height; i++) {
      if (maskY[i] == DWT_IN) {
	diff = (Int)((UChar *)orgY)[i] -
	  (Int)((UChar *)recY)[i];
	quad += diff * diff;
	num ++;
      }
    }
    
    /* SNR_l = quad/(pels*lines); */
    SNR_l = quad/num;
    if (SNR_l) 
    {
      SNR_l = (peakvalue*peakvalue) / SNR_l;
      SNR_l = 10 * log10(SNR_l);
      infSNR_l = 0;
    }
    else
    {
      infSNR_l = 1;
    }


    /* Calculate chroma PSNRs */
    if (colors==3) {
      num = 0;
      for (i=0; i<width*height/4; i++) {
	if (maskU[i] == DWT_IN) {
	  diff = (Int)((UChar *)orgU)[i] -
	    (Int)((UChar *)recU)[i]; 
	  quad_Cb += diff * diff;
	  num ++;
	}
      }
      SNR_Cb = quad_Cb/num;

      /* SNR_Cb = quad_Cb/(pels*lines/4.0); */
      if (orgV != NULL && recV != NULL) {
	SNR_Cb = quad_Cb/num;
	if (SNR_Cb) 
	{
	  SNR_Cb = (peakvalue*peakvalue) / SNR_Cb;
	  SNR_Cb = 10 * log10(SNR_Cb);
	  infSNR_Cb = 0;
	}
	else 
	{
	  infSNR_Cb = 1;
	}
      }

      num = 0;
      for (i=0; i<width*height/4; i++) {
	if (maskV[i] == DWT_IN) {
	  diff = (Int)((UChar *)orgV)[i] -
	    (Int)((UChar *)recV)[i];
	  quad_Cr += diff * diff;
	  num ++;
	}
      }
      
      SNR_Cr = quad_Cr/num;
      
      if (SNR_Cr) 
      {
	SNR_Cr = (peakvalue*peakvalue) / SNR_Cr;
	SNR_Cr = 10 * log10(SNR_Cr);
	infSNR_Cr = 0;
      }
      else
      {
	infSNR_Cr = 1;
      }
    }
    
    /* Display PSNRs */
    if (stat)
    {
      if (infSNR_l==0)
	noteStat("\nPSNR_Y: %.4f dB\n",SNR_l);
      else
	noteStat("\nPSNR_Y: +INF dB\n");
      
      if (colors == 3) {
	if (infSNR_Cb==0)
	  noteStat("PSNR_U: %.4f dB\n",SNR_Cb);
	else
	  noteStat("PSNR_U: +INF dB\n");
	
	if (infSNR_Cr==0)
	  noteStat("PSNR_V: %.4f dB\n",SNR_Cr);
	else
	  noteStat("PSNR_V: +INF dB\n");
      }
    }
    else
    {
      if (infSNR_l==0)
	noteProgress("\nPSNR_Y: %.4f dB",SNR_l);
      else
	noteProgress("\nPSNR_Y: +INF dB");
      
      if (colors == 3) {
	if (infSNR_Cb==0)
	  noteProgress("PSNR_U: %.4f dB",SNR_Cb);
	else
	  noteProgress("PSNR_U: +INF dB");
	
	if (infSNR_Cr==0)
	  noteProgress("PSNR_V: %.4f dB",SNR_Cr);
	else
	  noteProgress("PSNR_V: +INF dB");
      }
    }
}

/* $Id: write_image.cpp,v 1.2 2001/05/09 21:14:18 cahighlander Exp $ */
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


extern Int STO_const_alpha;
extern UChar STO_const_alpha_value;

Void CVTCDecoder::write_image(Char *recImgFile, Int colors,
		 Int width, Int height,
		 Int real_width, Int real_height,
		 Int rorigin_x, Int rorigin_y,
		 UChar *outimage[3], UChar *outmask[3],
		 Int usemask, Int fullsize, Int MinLevel)

     /* write outimage */
{
  FILE *outfptr, *maskfptr = NULL;
  Char recSegFile[200];
  UChar *ptr;
  UChar *recmask[3];
  UChar *recimage[3];
  Int status,i,col,ret;
  Int w,h;
  Int rwidth[3],rheight[3];
  Int Width[3], Height[3];
  Int origin_x[3],origin_y[3];
  Int l, lt;
  Int j,k,n, count, sum[3];  // hjlee 0901


  Width[0] = width;
  Width[1] = Width[2] = (Width[0]+1)>>1;
  Height[0] = height;
  Height[1] = Height[2] = (Height[0]+1)>>1;
  origin_x[0] = rorigin_x;
  origin_x[1] = origin_x[2] = origin_x[0]>>1;
  origin_y[0] = rorigin_y;
  origin_y[1] = origin_y[2] = origin_y[0]>>1;
  outfptr = fopen(recImgFile,"wb");
  if(usemask) {
    sprintf(recSegFile, "%s.seg",recImgFile);
    maskfptr = fopen(recSegFile,"wb");
  }
  noteProgress("Writing the reconstruction image: '%s'",recImgFile);
  l = (fullsize ? 0 : MinLevel);
  lt =  (1<<l) -1;

// hjlee 0901
    /* adjust the image for pixel w/ mismatched luma and chroma */
  if(colors >1) {
    for(i=0;i<Height[0]>>l;i++) {
      for(j=0; j<Width[0]>>l; j++) {
	if(outmask[0][i*(Width[0]>>l)+j]==DWT_IN && 
		outmask[1][(i>>1)*(Width[1]>>l)+(j>>1)]!=DWT_IN) {
	  count=0;
	  for(col=1;col<colors;col++) 
	    sum[col] = 0;
	  for(k=0;k<2;k++) {
	    for(n=0;n<2;n++) {
	      if(outmask[1][((i>>1)+k)*(Width[1]>>l)+((j>>1)+n)]==DWT_IN) {
		count++;
		for(col=1;col<colors; col++)
		  sum[col]+=outimage[col][((i>>1)+k)*(Width[1]>>l)+((j>>1)+n)];
	      }
	    }
	  }
	  if(count==0)  errorHandler("Impossible case occured, check program\n") ; /* impossible! no chroma available for this pixel ignore it */
	  else {
	    for(col=1; col< colors; col++) {
	      outmask[col][(i>>1)*(Width[col]>>l)+(j>>1)] = DWT_IN;
	      outimage[col][(i>>1)*(Width[col]>>l)+(j>>1)] = sum[col]/count;
	    }
	  }
	}
      }
    }
  }


  for (col=0; col<colors; col++) {

    if(col==0) { /* chroma */
      rwidth[0] = (real_width+lt)>>l;
      rheight[0] = (real_height+lt)>>l;
    }
    else {
      rwidth[col] = (rwidth[0] +1) >> 1;
      rheight[col] = (rheight[0] +1) >>1;
    }

    recmask[col]  = (UChar *)malloc(sizeof(UChar)*
					    rwidth[col]*rheight[col]);
    recimage[col] = (UChar *)malloc(sizeof(UChar)*
					    rwidth[col]*rheight[col]);
    
    
    
    ret=PutBox(outimage[col], outmask[col], recimage[col], recmask[col], 
	       rwidth[col], rheight[col], 
	       (Width[col]>>l), (Height[col]>>l),
	       origin_x[col]>>l, origin_y[col]>>l, 0, 
	       (usemask)?(STO_const_alpha?STO_const_alpha_value:MASK_VAL):RECTANGULAR);
    if(ret!= DWT_OK)
      errorHandler("DWT Error code %d", ret);

    ptr = recimage[col];
    w = rwidth[col];
    h = rheight[col];
    for (i=0; i<h; i++) {
      if ((status = fwrite((UChar *)ptr, sizeof(Char), 
			   w, outfptr)) != w)
	errorHandler("Error in writing image file.");
      
      ptr += w;
    } 

    if(usemask && col==0) {
      ptr = recmask[col];
      for (i=0; i<h; i++) {
	if ((status = fwrite((UChar *)ptr, sizeof(Char), 
			     w, maskfptr)) != w)
	  errorHandler("Error in writing image file.");
	ptr += w;
      } 
    }
    free(recmask[col]);
    free(recimage[col]);
  }  /* col */
  fclose(outfptr);
  if(usemask) fclose(maskfptr);
}


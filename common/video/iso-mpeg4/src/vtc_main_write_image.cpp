/* $Id: vtc_main_write_image.cpp,v 1.1 2005/05/09 21:29:47 wmaycisco Exp $ */
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


/*extern Int STO_const_alpha;
extern UChar STO_const_alpha_value; delete by SL 030399
*/
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
    
    
/*		   printf("%d %d %d %d\n", rwidth[col], rheight[col], (Width[col]>>l), (Height[col]>>l));*/

    
    ret=PutBox(outimage[col], outmask[col], recimage[col], recmask[col], 
	       rwidth[col], rheight[col], 
	       (Width[col]>>l), (Height[col]>>l),
	       origin_x[col]>>l, origin_y[col]>>l, 0, 
	       (usemask)?(STO_const_alpha?STO_const_alpha_value:MASK_VAL):RECTANGULAR,(col==0)?0:127);
    if(ret!= DWT_OK)
      errorHandler("DWT Error code %d", ret);

    ptr = recimage[col];
    w = rwidth[col];
    h = rheight[col];
		if ( col == 0 )
		noteProgress("Writing the reconstruction image: '%s(%dx%d)'",recImgFile,w,h);
    for (i=0; i<h; i++) {
      if ((status = fwrite((UChar *)ptr, sizeof(Char), w, outfptr)) != w)
	errorHandler("Error in writing image file.");
      
      ptr += w;
    } 

    if(usemask && col==0) {
      ptr = recmask[col];
      for (i=0; i<h; i++) {
	if ((status = fwrite((UChar *)ptr, sizeof(Char), w, maskfptr)) != w)
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

// begin: added by Sharp (99/5/10)
Void CVTCDecoder::write_image_to_buffer(UChar **DstImage, 
		UChar *DstMask[3], // FPDAM added by Sharp
		Int DstWidth, Int DstHeight, Int iTile, Int TileW,
		/* Char *recImgFile,*/ Int colors,
		 Int width, Int height,
		 Int real_width, Int real_height,
		 Int rorigin_x, Int rorigin_y,
		 UChar *outimage[3], UChar *outmask[3],
		 Int usemask, Int fullsize, Int MinLevel)

     /* write outimage */
{

// FPDAM begin : deleted by Sharp
//  FILE *outfptr, *maskfptr;
//  Char recSegFile[200];
// FPDAM end : deleted by Sharp
  UChar *ptr;
  UChar *recmask[3];
  UChar *recimage[3];
  Int i,col,ret;
  Int w,h;
  Int rwidth[3],rheight[3];
  Int Width[3], Height[3];
  Int origin_x[3],origin_y[3];
  Int l, lt;
	Int iTileXPos, iTileYPos;
	UChar *DstPtr;
	Int Dw[3];


  Width[0] = width;
  Width[1] = Width[2] = (Width[0]+1)>>1;
  Height[0] = height;
  Height[1] = Height[2] = (Height[0]+1)>>1;
  origin_x[0] = rorigin_x;
  origin_x[1] = origin_x[2] = origin_x[0]>>1;
  origin_y[0] = rorigin_y;
  origin_y[1] = origin_y[2] = origin_y[0]>>1;
	Dw[0] = DstWidth;
	Dw[1] = Dw[2] = (DstWidth+1)>>1;
/*  outfptr = fopen(recImgFile,"wb");*/
/*  if(usemask) {*/
/*    sprintf(recSegFile, "%s.seg",recImgFile);*/
/*    maskfptr = fopen(recSegFile,"wb");*/
/*  }*/
/*  noteProgress("Writing the reconstruction image: '%s'",recImgFile);*/
  l = (fullsize ? 0 : MinLevel);
  lt =  (1<<l) -1;
	iTileXPos = iTile % TileW - mzte_codec.m_target_tile_id_from % TileW;
	iTileYPos = iTile / TileW - mzte_codec.m_target_tile_id_from / TileW;

/*	printf("%d %d\n", iTileXPos, iTileYPos);*/
/*	printf("%d %d %d\n", Dw[0], Dw[1], Dw[2]);*/
/*	printf("%d %d\n", real_width, real_height);*/

// hjlee 0901
    /* adjust the image for pixel w/ mismatched luma and chroma */

// FPDAM begin: deleted by Sharp
#if 0
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
#endif
// FPDAM end: deleted by Sharp

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
    
// FPDAM begin: modified by Sharp
//    ret=PutBox(outimage[col], outmask[col], recimage[col], recmask[col], 
//	       rwidth[col], rheight[col], 
//	       (Width[col]>>l), (Height[col]>>l),
//	       origin_x[col]>>l, origin_y[col]>>l, 0, 
//	       (usemask)?(STO_const_alpha?STO_const_alpha_value:MASK_VAL):RECTANGULAR);
    ret=PutBox(outimage[col], outmask[col], recimage[col], recmask[col], 
	       rwidth[col], rheight[col], 
	       (Width[col]>>l), (Height[col]>>l),
	       origin_x[col]>>l, origin_y[col]>>l, 0, 
	       DWT_IN,(col==0)?0:127);
// FPDAM begin: modified by Sharp

    if(ret!= DWT_OK)
      errorHandler("DWT Error code %d", ret);

    ptr = recimage[col];
    w = rwidth[col];
    h = rheight[col];
		DstPtr = (UChar *)(DstImage[col]) + iTileXPos*((col==0)?mzte_codec.m_tile_width:(mzte_codec.m_tile_width+1)>>1)
			+ iTileYPos*((col==0)?mzte_codec.m_tile_height:(mzte_codec.m_tile_height+1)>>1)*Dw[col];
    for (i=0; i<h; i++) {
			memcpy(DstPtr, ptr, w);
			DstPtr += Dw[col];
/*      if ((status = fwrite((UChar *)ptr, sizeof(Char), */
/*			   w, outfptr)) != w)*/
/*	errorHandler("Error in writing image file.");*/
      
      ptr += w;
    } 

/* FPDAM begin: added by Sharp */
/*    if(usemask) {*/
       ptr = recmask[col];
       DstPtr = DstMask[col] + iTileXPos*((col==0)?mzte_codec.m_tile_width:(mzte_codec.m_tile_width+1)>>1) 
			+ iTileYPos*((col==0)?mzte_codec.m_tile_height:(mzte_codec.m_tile_height+1)>>1)*Dw[col];

       for (i=0; i<h; i++) {
         memcpy(DstPtr, ptr, w);
         DstPtr += Dw[col];
         ptr += w;
       } 
/*    }*/
/* FPDAM end: added by Sharp */

/*    if(usemask && col==0) {*/
/*      ptr = recmask[col];*/
/*      for (i=0; i<h; i++) {*/
/*	if ((status = fwrite((UChar *)ptr, sizeof(Char), */
/*			     w, maskfptr)) != w)*/
/*	  errorHandler("Error in writing image file.");*/
/*	ptr += w;*/
/*      } */
/*    }*/
    free(recmask[col]);
    free(recimage[col]);
  }  /* col */
/*  fclose(outfptr);*/
//  if(usemask) fclose(maskfptr); // FPDAM deleted by Sharp
}

Void CVTCDecoder::write_image_tile(Char *recImgFile, UChar **frm)
{
		FILE *fp;
		Int col;
		Int dWidth[3], dHeight[3];

		dWidth[0] = mzte_codec.m_display_width;
		dWidth[1] = (mzte_codec.m_display_width+1)>>1;
		dWidth[2] = (mzte_codec.m_display_width+1)>>1;
		dHeight[0] = mzte_codec.m_display_height;
		dHeight[1] = (mzte_codec.m_display_height+1)>>1;
		dHeight[2] = (mzte_codec.m_display_height+1)>>1;

		fp = fopen(recImgFile, "w");

		noteProgress("Writing reconstructed image '%s'(%dx%d) ...",recImgFile, dWidth[0], dHeight[0]);
		for ( col=0; col<mzte_codec.m_iColors; col++ )
			fwrite(frm[col], 1, dWidth[col]*dHeight[col], fp);
		fclose(fp);
}

// end: added by Sharp (99/5/10)

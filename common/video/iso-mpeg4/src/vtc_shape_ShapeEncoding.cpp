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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "basic.hpp"
#include "dataStruct.hpp"
//#include "ShapeBaseCommon.h"
//#include "BinArCodec.h"
//#include "bitpack.h"
//#include "errorHandler.h"
#include "ShapeEnhDef.hpp"
//#include "dwt.h"
//#include "ShapeEncoding.h"
//#include "ShapeBaseCodec.h"
//#include "ShapeEnhEncode.h"




Int CVTCEncoder::
ShapeEnCoding(UChar *inmask, Int width, Int height, 
		  Int levels, 
		  Int constAlpha,
		  UChar constAlphaValue, 
		  Int change_CR_disable,
		  Int shapeScalable,
		  Int startCodeEnable,
		  FILTER **filter)
{
  UChar *outmask, *recmask;
  Int k;// sl, sm, ret;
  Int i,j;//l,m;
  //Int c;
  Int coded_height, coded_width;
  Int minbsize;
  Int totalbits,previoustotalbits;
  //  static int first=0;
//   if(!shapeScalable) levels = 0; // FPDAM : deleted by Sharp

  minbsize = 1<< levels;
  coded_width = (((width+minbsize-1)>>levels)<<levels);
  coded_height = (((height+minbsize-1)>>levels)<<levels);
  if(coded_width!=width || coded_height!=height) {
    printf("Object width or height is not multiples of 2^levels\n");
    exit(1);
  }
  outmask = (UChar *)
    malloc(coded_width*coded_height*sizeof(UChar));
  recmask = (UChar *)
    malloc(coded_width*coded_height*sizeof(UChar));
  if(outmask == NULL || recmask == NULL ) {
    errorHandler("Memory allocation failed\n");
  }
  
  memset(recmask, 0, coded_width*coded_height);
  for(i=0;i<height;i++) 
    for(j=0;j<width;j++)
      recmask[i*coded_width+j] = inmask[i*width+j]!=0?1:0;
#if 1
  printf("Coding Shape Header...\n");
#endif
  EncodeShapeHeader(constAlpha, constAlphaValue, change_CR_disable /*, shapeScalable*/ ); // FPDAM : modified by Sharp
  

  do_DWTMask(recmask, outmask, coded_width, coded_height, levels, filter);
#if 1
  printf("Coding Shape Base Layer...\n");
#endif
  EncodeShapeBaseLayer(outmask, change_CR_disable, coded_width, coded_height,  levels);
  previoustotalbits = 0;
  totalbits = get_total_bit_rate();
#ifdef DEBUG
  printf("Base Layer bits = %d\n", totalbits);
#endif
  previoustotalbits = totalbits;
//   if(shapeScalable) { // FPDAM : modified by Sharp
    if(!startCodeEnable) {
      PutBitstoStream_Still(4, levels);
      PutBitstoStream_Still(1,MARKER_BIT);
    }
    for(k=levels;k>0;k--) {
#if 1
      printf("Coding Shape Enhanced Layer %d...\n", levels-k+1);
#endif
      EncodeShapeEnhancedLayer(outmask, 
			       coded_width, coded_height,   k, 
			       filter[k-1],
			       startCodeEnable);
      
      totalbits = get_total_bit_rate();
#ifdef DEBUG
      printf("Enhanced Layer %d bits = %d\n", levels-k, totalbits-previoustotalbits);
#endif
      previoustotalbits = totalbits;
    }
    if(startCodeEnable) {
      ByteAlignmentEnc_Still();
      PutBitstoStream_Still(32,TEXTURE_SPATIAL_START_CODE);
      PutBitstoStream_Still(5, 0);
      PutBitstoStream_Still(1, MARKER_BIT);    
    }
//  } // FPDAM 
  free(outmask);
  free(recmask);
  return(0);
}

Int CVTCEncoder::
EncodeShapeHeader(Int constAlpha, UChar constAlphaValue, Int change_CR_disable /*, Int shapeScalable*/ ) // FPDAM : deleted by Sharp
{   
  
  PutBitstoStream_Still(1, change_CR_disable?1:0);
  PutBitstoStream_Still(1, constAlpha?1:0);
  if(constAlpha)
    PutBitstoStream_Still(8, (Int)constAlphaValue);
//   PutBitstoStream_Still(1, shapeScalable); // FPDAM deleted by Sharp
  PutBitstoStream_Still(1,MARKER_BIT);
  return(0);

}


Int CVTCEncoder::
EncodeShapeBaseLayer(UChar *outmask, Int  change_CR_disable,
			 Int coded_width, Int coded_height, Int levels)
{
  Int w,h;
  Int m,l;
  UChar *inmask;

  w = coded_width >> levels;
  h = coded_height >> levels;

  inmask =(UChar *) malloc(sizeof(UChar)*w*h);
  if(inmask == (UChar *)NULL) {
    errorHandler("Memory allocation failed\n");    
  }
  /* low low band context */
  for(l=0; l<h; l++) {
    for(m=0; m<w; m++) {
      inmask[l*w+m]=outmask[l*coded_width+m];
    }
  }
  ShapeBaseEnCoding(inmask, w, h, 0, change_CR_disable);
  MergeShapeBaseBitstream();
  PutBitstoStream_Still(1,MARKER_BIT);
  free(inmask);
  return(0);
}

Int CVTCEncoder::
EncodeShapeEnhancedLayer( UChar *outmask, 
			      Int coded_width, Int coded_height,
			      Int k, FILTER *filter,
			      Int startCodeEnable)
{
  Int w,h, w2, h2;
  UChar *low_mask, *half_mask, *cur_mask;
  Int m,l;
  Int ret;
  //static first=0;
  if(startCodeEnable) {
    ByteAlignmentEnc_Still();
    PutBitstoStream_Still(32,TEXTURE_SHAPE_START_CODE);
    PutBitstoStream_Still(5, k);
    PutBitstoStream_Still(1, MARKER_BIT);    
  }

  w = (coded_width >>k);
  h = (coded_height >> k);
  w2 = w <<1;
  h2 = h <<1;

  low_mask = (UChar *) calloc(w*h, sizeof(UChar));
  half_mask = (UChar *) calloc(w*h2, sizeof(UChar));
  cur_mask = (UChar *) calloc(w2*h2, sizeof(UChar));
  if(low_mask==NULL || cur_mask==NULL || half_mask==NULL){
    errorHandler("memory alloc. error: spa_mask!\n");
  }
 
  for(l=0;l<h;l++){
    for(m=0; m<w;m++){
      low_mask[l*w+m]= outmask[l*coded_width+m];
    }
  }
#if 0
  if(first==0) {
    int x, y;
    FILE *ftest;
    ftest=fopen("test.txt", "w");
    for(y=0;y<h;y++) {
      for(x=0;x<w;x++) {
	fprintf(ftest,"%d ", low_mask[y*w+x]);
      }
      fprintf(ftest,"\n");
    }
    fprintf(ftest,"\n");
    first = 1;
    fclose(ftest);
  }
#endif
  /* vertical first */
  if((ret=SynthesizeMaskHalfLevel(outmask, coded_width, coded_height, k, filter, DWT_NONZERO_HIGH, DWT_VERTICAL))!=0) {
    errorHandler("Error Code=%d\n", ret);
  }
  
  for(l=0;l<h2;l++){
    for(m=0; m<w;m++){
      half_mask[l*w+m]= outmask[l*coded_width+m];
    }
  }

  if((ret=SynthesizeMaskHalfLevel(outmask, coded_width, coded_height, k, filter, DWT_NONZERO_HIGH, DWT_HORIZONTAL))!=0) {
    errorHandler("Error Code=%d\n", ret);
  }
  
  for(l=0;l<h2;l++){
    for(m=0; m<w2;m++){
      cur_mask[l*w2+m]= outmask[l*coded_width+m];
    }
  }
  
  ShapeEnhEnCoding(low_mask, half_mask, cur_mask, w2, h2, filter);
  MergeEnhShapeBitstream();
  
  PutBitstoStream_Still(1,MARKER_BIT);
  free(low_mask);
  free(half_mask);
  free(cur_mask);
  return(0);
}






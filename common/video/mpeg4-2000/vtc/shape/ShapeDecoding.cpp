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
#include "basic.hpp"
#include "dwt.h"
#include "dataStruct.hpp"
//#include "ShapeBaseCommon.h"
#include "ShapeEnhDef.hpp"
//#include "BinArCodec.h"
//#include "bitpack.h"
//#include "errorHandler.h"
//#include "ShapeDecoding.h"
//#include "ShapeBaseCodec.h"
//#include "ShapeEnhDecode.h"

Int CVTCDecoder::
ShapeDeCoding(UChar *mask, Int width, Int height, 
		  Int levels,  Int *targetLevel, 
		  Int *constAlpha,
		  UChar *constAlphaValue,
		  Int startCodeEnable,
		  Int fullSizeOut,
		  FILTER **filter)
{
  UChar *outmask, *recmask;
  Int k,   ret;
  Int i,j;
  Int w,h;
//  Int c;
  Int coded_height, coded_width;
  Int minbsize;
  Int change_CR_disable;
//  Int shapeScalable; // FPDAM : deleted by Sharp
  Int codedLevels;

#if 1
  fprintf(stderr,"Decoding Shape Header...\n");
#endif
  DecodeShapeHeader(constAlpha, constAlphaValue, &change_CR_disable /*, &shapeScalable*/ ); // FPDAM : modified by Sharp
//  if(!shapeScalable) levels = 0; // FPDAM : deleted by Sharp
  minbsize = 1<< levels;
  
  coded_width = (width+minbsize-1)/minbsize*minbsize;
  coded_height = (height+minbsize-1)/minbsize*minbsize; 
  if(coded_width!=width || coded_height!=height) {
    printf("Object width or height is not multiples of 2^levels\n");
    exit(1);
  }
  outmask = (UChar *)
    malloc(coded_width*coded_height*sizeof(UChar));
  recmask = (UChar *)
    malloc(coded_width*coded_height*sizeof(UChar));
  if(outmask == NULL || recmask == NULL) {
    errorHandler("Memory allocation failed\n");
  }
  
#if 1
  fprintf(stderr,"Decoding Shape Base Layer...\n");
#endif
  DecodeShapeBaseLayer(outmask, change_CR_disable,  coded_width, coded_height, levels);

//  if(shapeScalable) { // FPDAM: deleted by Sharp
    ret = 0;
    if(!startCodeEnable) {
      codedLevels = GetBitsFromStream_Still(4);
      if(GetBitsFromStream_Still(1)!=MARKER_BIT)
	errorHandler("Incorrect Marker bit in shape enhanced layer decoding.\n");
      /* if(*targetLevel < levels-codedLevels ) */ 
      *targetLevel = levels-codedLevels;
    }

    if(*targetLevel <0) *targetLevel =0;

    for(k=levels;k>*targetLevel;k--) {
      w = (coded_width >>k);
      h = (coded_height >> k);
#if 1 /*def DEBUG*/
      fprintf(stderr,"Decoding Shape Enhanced Layer %d...\n", levels-k+1);
#endif
      ret=DecodeShapeEnhancedLayer(outmask, 
				   coded_width, coded_height,  k,
				   filter[k-1],
				   startCodeEnable);
      if(startCodeEnable && ret) break; /* end of enhanced layer */
    }
    if(startCodeEnable) {
      int code32=0;
      *targetLevel = k;
      if(!ret) ByteAlignmentDec_Still();
      /* search for texture layer start code */
      code32 = GetBitsFromStream_Still(32);
      
      while(code32!=TEXTURE_SPATIAL_START_CODE) {
	code32 = (code32 <<8)|  GetBitsFromStream_Still(8);
      }
      /*  errorHandler("Error in retrieving TEXTURE_SPATIAL_START_CODE\n"); */
      GetBitsFromStream_Still(5);
      if(GetBitsFromStream_Still(1)!= MARKER_BIT) 
	errorHandler("Incorrect Marker bit in the end of Shape Bitstream\n");
    }
// FPDAM begin: deleted by Sharp
/*
  }
  else {
    *targetLevel = 0;
  }
*/
// FPDAM end: deleted by Sharp

  if(fullSizeOut) { /* interpolate shape to full size */
    for(i=0;i<(height>>*targetLevel);i++) 
      for(j=0;j<(width>>*targetLevel);j++)
	recmask[i*coded_width+j]=outmask[i*(width>>*targetLevel)+j];
    do_iDWTMask(recmask, mask, width, height, *targetLevel,
		*targetLevel, filter, 0, 1);
  }
  else {
    for(i=0;i<(height>>*targetLevel);i++) 
      for(j=0;j<(width>>*targetLevel);j++)
	mask[i*(width>>*targetLevel)+j] = outmask[i*(width>>*targetLevel)+j];
  }    
  free(outmask);
  free(recmask);
  return(0);
}

Int CVTCDecoder::
DecodeShapeHeader(Int *constAlpha, UChar *constAlphaValue, 
		      Int *change_CR_disable /*, Int *shapeScalable*/ ) // FPDAM : modified by Sharp
{   
  
   *change_CR_disable = GetBitsFromStream_Still(1);
   *constAlpha = GetBitsFromStream_Still(1);
    if(*constAlpha) 
      *constAlphaValue = (UChar) GetBitsFromStream_Still(8);
//     *shapeScalable = GetBitsFromStream_Still(1); // FPDAM : deleted by Sharp
    if(GetBitsFromStream_Still(1)!=MARKER_BIT)
      errorHandler("Incorrect Marker bit in header decoding.\n");
    return(0);
    
}

Int CVTCDecoder::
DecodeShapeBaseLayer(UChar *outmask, Int change_CR_disable,
			 Int coded_width, Int coded_height, Int levels)
{
  Int w,h;
 // Int m,l;

  w = coded_width >> levels;
  h = coded_height >> levels;

  ShapeBaseDeCoding(outmask, w, h, change_CR_disable);

  if(GetBitsFromStream_Still(1)!=MARKER_BIT)
    errorHandler("Incorrect Marker bit in shape base layer decoding.\n");
  return(0);
}

Int CVTCDecoder::
DecodeShapeEnhancedLayer( UChar *outmask, 
			      Int coded_width, 
			      Int coded_height, 
			      Int k, FILTER *filter, Int startCodeEnable)
{
  Int w,h;
  Int m,l;
  Int w2, h2;
  UChar *low_mask, *half_mask, *cur_mask;

  if(startCodeEnable) {
    ByteAlignmentDec_Still();
    
    if(LookBitsFromStream_Still(32)!=TEXTURE_SHAPE_START_CODE)
      return(1);
    GetBitsFromStream_Still(32);
    GetBitsFromStream_Still(5);
    if(GetBitsFromStream_Still(1)!=MARKER_BIT)
      errorHandler("Incorrect Marker bit in shape enhanced layer decoding.\n");
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
      low_mask[l*w+m]= outmask[l*w+m];
    }
  }
  
  ShapeEnhDeCoding(low_mask, half_mask, cur_mask, w2, h2, filter);

  if(GetBitsFromStream_Still(1)!=MARKER_BIT)
    errorHandler("Incorrect Marker bit in shape enhanced layer decoding.\n");

  for(l=0;l<h2;l++){
    for(m=0; m<w2;m++){
      outmask[l*w2+m]= cur_mask[l*w2+m];
    }
  }
  free(low_mask);
  free(half_mask);
  free(cur_mask);
  return(0);
}


UInt CVTCDecoder::
GetBitsFromStream_Still(Int bits)
{
  UInt code;
  code = get_X_bits(bits);
  return ( code );
}

Int CVTCDecoder::
ByteAlignmentDec_Still()
{
  align_byte();
  return(0);
}

Int CVTCDecoder::
BitstreamLookBit_Still(Int pos)
{
  return((Int )(LookBitFromStream(pos) &1 ) );
}


UInt CVTCDecoder::
LookBitsFromStream_Still(Int bits)
{
    Int	i;
    UInt code=0;

    for ( i=1; i<=bits; i++ ) {
        code = (code << 1) + ( BitstreamLookBit_Still(i) & 1 );
    }
    return ( code );
}

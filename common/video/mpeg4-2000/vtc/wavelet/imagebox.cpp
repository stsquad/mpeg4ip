/* $Id: imagebox.cpp,v 1.1 2003/05/05 21:24:03 wmaycisco Exp $ */
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

/* imagebox.c -- extend/crop an image to a box of the minimal multiples of 
                 2^Decomposition Level size that contains the object, according
		 to the mask information */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <memory.h>
#include "basic.hpp"
#include "dwt.h"

#include "dataStruct.hpp" // FPDAM added by Sharp

#ifndef __cplusplus
static Int LCM(Int x, Int y);
static Int GCD(Int x, Int y);
#endif

/* Function: GetBox()
   Description: get image object with the bounding box 
   Input:
   
   InImage -- Input image data, data type defined by DataType;
   
   RealWidth, RealHeight  -- the size of the actual image;
   VirtualWidth, VirtualHeight -- size of the output image;
   OriginX, OriginY -- origins of the output image relative to the original
                       image
   DataType -- 0 - UChar 1- UShort for InImage and OutImage
   
   Output:
   
   OutImage -- Output image within the bounding box (can be cropped or 
               extended version of the InImage)
   
   
   
		       
   Return:  DWT_NOVALID_PIXEL if the mask are all zeros, otherwise DWT_OK 
            if no error;	       
   Note: The caller should free the memory OutImage  allocated by this program 
         after it finishes using them.
*/

Int VTCIMAGEBOX::GetBox(Void *InImage, Void **OutImage, 
	   Int RealWidth, Int RealHeight, 
	   Int VirtualWidth, Int VirtualHeight, 
	   Int OriginX, Int OriginY, Int DataType)
{
  Int origin_x, origin_y;
  Int virtual_width, virtual_height;
  UChar *data, *indata;
  Int wordsize = (DataType==DWT_USHORT_ENUM)?2:1;
  Int i, j;
  Int real_width, real_height; 
  Int max_x, max_y;

  Int rows, cols;
  

  real_width = RealWidth;
  real_height = RealHeight;
  origin_x = OriginX;
  origin_y = OriginY;
  virtual_width = VirtualWidth;
  virtual_height = VirtualHeight;
  
  /* allocate proper memory and initialize */
  if ((data = (UChar *)malloc(sizeof(UChar)*virtual_width*virtual_height*wordsize)) == NULL) {
    return(DWT_MEMORY_FAILED);
  }
  memset(data, (Char )0, sizeof(UChar)*virtual_width*virtual_height*wordsize);
  /* calculate clip area */
  max_y = origin_y+virtual_height;
  max_y = (max_y<real_height)?max_y:real_height;
  rows = max_y - origin_y;
  max_x = origin_x+virtual_width;
  max_x = (max_x<real_width)?max_x:real_width;
  cols = max_x - origin_x;
  indata = (UChar *)InImage;
  /* fill out data */
  for(i=0, j=origin_y*real_width+origin_x; i< rows*virtual_width;
      i+=virtual_width, j+=real_width) {
    memcpy(data+i, indata+j, wordsize*cols);
  }
  
  *OutImage = data;
  return(DWT_OK);
}
  


/* Function: GetMaskBox()
   Description: get the bounding box of the mask of image object
   Input:
   
   InMask -- Input image mask, can be NULL if Shape == RECTANGULAR;
   RealWidth, RealHeight  -- the size of the actual image;
   Nx, Ny -- specify  that OriginX and OriginY must be multiple of Nx and Ny
             to accomdate different color compoents configuration:
	     Examples:
	     for 420:   Nx  Ny 
	           Y:   2   2
                   U,V: 1   1
	     for 422:   
	           Y:   2   1
             for 444:
	      Y,U,V:    1   1
	     for mono:
                  Y:    1   1
	     for YUV9:  
                  Y:    4   4
                  U,V:  1   1  
  
   Shape   -- if -1, the SHAPE is rectangular, else arbitary shape and
              Shape defines the mask value of an object, useful for
	      multiple objects coding 
   nLevels -- levels of decomposition
   Output:
   
   OutMask -- Output image mask, extended area is marked as Don't-care
   VirtualWidth, VirtualHeight -- size of the output image
   OriginX, OriginY -- origins of the output image relative to the original
                       image
		       
   Return:  DWT_NOVALID_PIXEL if the mask are all zeros, otherwise DWT_OK 
            if no error;	       
   Note: The caller should free the memory OutMask allocated by this program 
         after it finishes using them.
*/

Int VTCIMAGEBOX:: GetMaskBox(UChar *InMask,  UChar **OutMask, 
	       Int RealWidth, Int RealHeight, 
	       Int Nx, Int Ny,
	       Int *VirtualWidth, Int *VirtualHeight, 
	       Int *OriginX, Int *OriginY,  Int Shape, Int nLevels)
{
  Int origin_x, origin_y;
  Int virtual_width, virtual_height;
  UChar *mask;
  Int blocksize =  1 << nLevels;
  Int i, j;
  Int real_width, real_height; 
  Int max_x, max_y;
  Int min_x, min_y;
  Int rows, cols;
  UChar *a, *b, *f;
  
  if(blocksize%Nx!=0) blocksize = LCM(blocksize,Nx);
  if(blocksize%Ny!=0) blocksize = LCM(blocksize,Ny);
  
  real_width = RealWidth;
  real_height = RealHeight;
  if(Shape != RECTANGULAR) {
    /* to search for the upper left corner of the bounding box of 
       arbitrarily shaped object */
    min_x = real_width;
    min_y = real_height;
    max_x = 0;
    max_y =0;

    
    for(i=0,j=0;j < real_height; j++,i+=real_width){
      a=InMask+i;
      f = InMask+i+real_width;
      for(; a< f; a++){
	if(*a == Shape) {
	  min_y = j;
	  goto minx;
	}
      }
    }

  minx:
    for(i=0;i < real_width;i++){
      a=InMask+i;
      f=InMask+i+real_width*real_height;
      for(; a<f;  a+=real_width){
	if(*a == Shape) {
	  min_x = i;
	  goto maxy;
	}
      }
    }

  maxy:
    for(j=real_height-1,i= (real_height-1)*real_width;j>=0 ;j--, i-=real_width){
      a = InMask+i;
      f = InMask+i+real_width;;
      for(; a <f;  a++) {
	if(*a == Shape) {
	  max_y = j;
	  goto maxx;
	}
      }
    }
    
  maxx:
    for(i=real_width-1;i >= 0;i--){
      a=InMask+i;
      f =  InMask+i+real_width*real_height;
      for(; a < f; a+=real_width){
	if(*a == Shape) {
	  max_x = i;
	  goto next;
	}
      }
    }
    
  next:
    /* quantize the min_x and min_y with Nx and Ny */
    if(min_x%Nx!=0) min_x=min_x/Nx*Nx;
    if(min_y%Ny!=0) min_y=min_y/Ny*Ny;
    if(min_x>max_x || min_y> max_y) {
      return(DWT_NOVALID_PIXEL); /* no valid pixel */
    }
    origin_x = min_x;
    origin_y = min_y;
    virtual_width = max_x-min_x+1;
    virtual_height = max_y-min_y+1;
   /*  fprIntf(stderr, "x=%d y=%d w=%d h=%d x2=%d y2=%d\n", min_x, min_y, virtual_width, virtual_height, max_x, max_y); */
  }
  else { /* rectangular region */
    origin_x = 0; 
    origin_y = 0;
    virtual_width = RealWidth;
    virtual_height = RealHeight;
  }  
  
  /* first ajust the dimension to be multiple of blocksize */
  virtual_width =  (virtual_width+(blocksize)-1)/blocksize*blocksize;
  virtual_height =  (virtual_height+(blocksize)-1)/blocksize*blocksize;
  if ((mask = (UChar *)malloc(sizeof(UChar)*virtual_width*virtual_height)) == NULL) {
    return(DWT_MEMORY_FAILED);
  }
  memset(mask, (Char )0, sizeof(UChar)*virtual_width*virtual_height);
  /* calculate clip area */
  max_y = origin_y+virtual_height;
  max_y = (max_y<real_height)?max_y:real_height;
  rows = max_y - origin_y;
  max_x = origin_x+virtual_width;
  max_x = (max_x<real_width)?max_x:real_width;
  cols = max_x - origin_x;
  /* fill out data */
  for(i=0, j=origin_y*real_width+origin_x; i< rows*virtual_width;
      i+=virtual_width, j+=real_width) {
    if(Shape != RECTANGULAR) {
      f = InMask+j+cols;
      for(a = InMask+j, b=mask+i; a < f; a++, b++) {
	if(*a == (UChar) Shape) *b = DWT_IN;
      }
    }
    else
      memset(mask+i, (Char )DWT_IN, cols);
  }
  *VirtualWidth = virtual_width;
  *VirtualHeight = virtual_height;
  *OriginX = origin_x;
  *OriginY = origin_y;
  *OutMask = mask;
  return(DWT_OK);
}

// FPDAM begin: added by Sharp
Int VTCIMAGEBOX:: GetRealMaskBox(UChar *InMask,  UChar **OutMask, 
	       Int RealWidth, Int RealHeight, 
	       Int Nx, Int Ny,
	       Int *VirtualWidth, Int *VirtualHeight, 
	       Int *OriginX, Int *OriginY,  Int Shape, Int nLevels)
{
  Int origin_x, origin_y;
  Int virtual_width, virtual_height;
  UChar *mask;
  //  Int blocksize =  1 << nLevels;
  Int i, j;
  Int real_width, real_height; 
  Int max_x, max_y;
  Int min_x, min_y;
  Int rows, cols;
  UChar *a, *b, *f;
  
  real_width = RealWidth;
  real_height = RealHeight;
  if(Shape != RECTANGULAR) {
    /* to search for the upper left corner of the bounding box of 
       arbitrarily shaped object */
    min_x = real_width;
    min_y = real_height;
    max_x = 0;
    max_y =0;

    
    for(i=0,j=0;j < real_height; j++,i+=real_width){
      a=InMask+i;
      f = InMask+i+real_width;
      for(; a< f; a++){
	if(*a == Shape) {
	  min_y = j;
	  goto minx;
	}
      }
    }

  minx:
    for(i=0;i < real_width;i++){
      a=InMask+i;
      f=InMask+i+real_width*real_height;
      for(; a<f;  a+=real_width){
	if(*a == Shape) {
	  min_x = i;
	  goto maxy;
	}
      }
    }

  maxy:
    for(j=real_height-1,i= (real_height-1)*real_width;j>=0 ;j--, i-=real_width){
      a = InMask+i;
      f = InMask+i+real_width;;
      for(; a <f;  a++) {
	if(*a == Shape) {
	  max_y = j;
	  goto maxx;
	}
      }
    }
    
  maxx:
    for(i=real_width-1;i >= 0;i--){
      a=InMask+i;
      f =  InMask+i+real_width*real_height;
      for(; a < f; a+=real_width){
	if(*a == Shape) {
	  max_x = i;
	  goto next;
	}
      }
    }
    
  next:
    /* quantize the min_x and min_y with Nx and Ny */
    if(min_x%Nx!=0) min_x=min_x/Nx*Nx;
    if(min_y%Ny!=0) min_y=min_y/Ny*Ny;
    if(min_x>max_x || min_y> max_y) {
      return(DWT_NOVALID_PIXEL); /* no valid pixel */
    }
    origin_x = min_x;
    origin_y = min_y;
    virtual_width = max_x-min_x+1;
    virtual_height = max_y-min_y+1;
   /*  fprIntf(stderr, "x=%d y=%d w=%d h=%d x2=%d y2=%d\n", min_x, min_y, virtual_width, virtual_height, max_x, max_y); */
  }
  else { /* rectangular region */
    origin_x = 0; 
    origin_y = 0;
    virtual_width = RealWidth;
    virtual_height = RealHeight;
  }  
  
  /* first ajust the dimension to be multiple of blocksize */
  virtual_width =  (virtual_width+1)/2*2;
  virtual_height =  (virtual_height+1)/2*2;
  if ((mask = (UChar *)malloc(sizeof(UChar)*virtual_width*virtual_height)) == NULL) {
    return(DWT_MEMORY_FAILED);
  }
  memset(mask, (Char )0, sizeof(UChar)*virtual_width*virtual_height);
  /* calculate clip area */
  max_y = origin_y+virtual_height;
  max_y = (max_y<real_height)?max_y:real_height;
  rows = max_y - origin_y;
  max_x = origin_x+virtual_width;
  max_x = (max_x<real_width)?max_x:real_width;
  cols = max_x - origin_x;
  /* fill out data */
  for(i=0, j=origin_y*real_width+origin_x; i< rows*virtual_width;
      i+=virtual_width, j+=real_width) {
    if(Shape != RECTANGULAR) {
      f = InMask+j+cols;
      for(a = InMask+j, b=mask+i; a < f; a++, b++) {
	if(*a == (UChar) Shape) *b = DWT_IN;
      }
    }
    else
      memset(mask+i, (Char )DWT_IN, cols);
  }
  *VirtualWidth = virtual_width;
  *VirtualHeight = virtual_height;
  *OriginX = origin_x;
  *OriginY = origin_y;
  *OutMask = mask;
  return(DWT_OK);
}

// FPDAM end : added by Sharp

/* Function: ExtendMaskBox()
   Description: extend the size of bounding box of the mask of image object
                to 2^nLevels;
   Input:
   
   InMask -- Input image mask
   InWidth, InHeight  -- the size of the Input Mask;
   Nx, Ny -- specify  that OriginX and OriginY must be multiple of Nx and Ny
             to accomdate different color compoents configuration:
	     Examples:
	     for 420:   Nx  Ny 
	           Y:   2   2
                   U,V: 1   1
	     for 422:   
	           Y:   2   1
             for 444:
	      Y,U,V:    1   1
	     for mono:
                  Y:    1   1
	     for YUV9:  
                  Y:    4   4
                  U,V:  1   1     nLevels -- levels of decomposition
   Output:
   
   OutMask -- Output image mask, extended area is marked as Don't-care
   OutWidth, OutHeight -- size of the output mask
		       
   Return:  DWT_OK if no error;	       
   Note: The caller should free the memory OutMask allocated by this program 
         after it finishes using them.
*/

Int VTCIMAGEBOX:: ExtendMaskBox(UChar *InMask,  UChar **OutMask, 
		  Int InWidth, Int InHeight, 
		  Int Nx, Int Ny,
		  Int *OutWidth, Int *OutHeight, 
		  Int nLevels)
{
  Int out_width, out_height;
  UChar *mask;
  Int blocksize =  1 << nLevels;
  Int i, j;
  UChar *a, *b, *f;

  if(blocksize%Nx!=0) blocksize = LCM(blocksize,Nx);
  if(blocksize%Ny!=0) blocksize = LCM(blocksize,Ny);
  
  /* first ajust the dimension to be multiple of blocksize */
  out_width =  (InWidth+(blocksize)-1)/blocksize*blocksize;
  out_height =  (InHeight+(blocksize)-1)/blocksize*blocksize;
  if ((mask = (UChar *)malloc(sizeof(UChar)*out_width*out_height)) == NULL) {
    return(DWT_MEMORY_FAILED);
  }
  memset(mask, (Char )0, sizeof(UChar)*out_width*out_height);

  /* fill out data */
  for(i=0, j=0; i< InHeight*out_width;
      i+=out_width, j+=InWidth) {
    f = InMask+j+InWidth;
    for(a = InMask+j, b=mask+i; a < f; a++, b++) {
      if(*a == (UChar) DWT_IN) *b = DWT_IN;
    }
  }
  *OutWidth = out_width;
  *OutHeight = out_height;
  *OutMask = mask;
  return(DWT_OK);
}


/* Function: PutBox()
   Description: put the bounding box of the image object back
   Input:
   
   InImage -- Input image data, data type defined by DataType;
   InMask -- Input image mask
   RealWidth, RealHeight  -- the size of the actual image;
   DataType -- 0 - UChar 1- UShort for InImage and OutImage
   Shape   -- if -1, the SHAPE is rectangular, else arbitary shape and
              Shape defines the mask value of an object, useful for
	      multiple objects coding 

   VirtualWidth, VirtualHeight -- size of the output image
   OriginX, OriginY -- origins of the output image relative to the original
                       image

   Output:
   
   OutImage -- Output image contains the bounding box image
   OutMask -- Output image mask
              NULL if Shape == RECTANGULAR;
		       
   Return:  DWT_OK if no error;	       
*/

Int VTCIMAGEBOX:: PutBox(Void *InImage, UChar *InMask, Void *OutImage, UChar *OutMask, 
	   Int RealWidth, Int RealHeight, 
	   Int VirtualWidth, Int VirtualHeight, 
	   Int OriginX, Int OriginY, Int DataType, Int Shape,Int OutValue)
{
  Int origin_x, origin_y;
  Int virtual_width, virtual_height;
  UChar *data, *indata;
  UChar *mask = NULL;
  Int wordsize = (DataType==DWT_USHORT_ENUM)?2:1;
  Int i, j;
  Int real_width, real_height; 
  Int max_x, max_y;
  Int rows, cols;
  UChar *a, *b, *c, *f;

  real_width = RealWidth;
  real_height = RealHeight;
  virtual_width = VirtualWidth;
  virtual_height = VirtualHeight;
  origin_x = OriginX;
  origin_y = OriginY;

  /* allocate proper memory and initialize to zero*/
  data = (UChar *)OutImage;
  memset(data, (Char )OutValue, sizeof(UChar)*real_width*real_height*wordsize); // FPDAM modified by Sharp
  if(Shape != RECTANGULAR) {
    mask = OutMask;
    memset(mask, (Char)0, sizeof(UChar)*real_width*real_height);
  }
      
  /* calculate clip area */
  max_y = origin_y+virtual_height;
  max_y = (max_y<real_height)?max_y:real_height;
  rows = max_y - origin_y;
  max_x = origin_x+virtual_width;
  max_x = (max_x<real_width)?max_x:real_width;
  cols = max_x - origin_x;
  indata = (UChar *)InImage;
  /* fill out data */
  for(i=0, j=origin_y*real_width+origin_x; i< rows*virtual_width;
      i+=virtual_width, j+=real_width) {
    f = InMask+i+cols;
    for(a = data +j*wordsize, b = indata + i*wordsize, c= InMask+i;  
	c< f; c++, a+=wordsize, b+=wordsize) {
      if(*c == DWT_IN) {
	memcpy(a, b, wordsize);
      }
    }
    if(Shape != RECTANGULAR) {
      for(a=InMask+i, b = mask+j; a<f; a++, b++) {
	if(*a == DWT_IN) *b=(UChar) Shape;
      }
    }
  }
  return(DWT_OK);
}

/* FPDAM begin : added by Sharp */
Int VTCIMAGEBOX::CheckTextureTileType (UChar *mask, Int width, Int height, Int real_width, Int real_height)
{
  Int i, j, cnt, size=real_width*real_height;
  Int texture_tile_type;

  cnt = 0;
  for (i=0; i<real_height; i++) {
     for (j=0; j<real_width; j++) {
  if ( *(mask+i*width+j) == (UChar)DWT_IN ) cnt ++;
     }
  }
  if( cnt == 0 ) texture_tile_type = TRANSP_TILE;
  else if( cnt == size ) texture_tile_type = OPAQUE_TILE;
  else       texture_tile_type = BOUNDA_TILE;

  return (texture_tile_type);
}
/* FPDAM end : added by Sharp */


/* find the least common multiples of two Integers */
Int VTCIMAGEBOX:: LCM(Int x, Int y)
{
  return(x*y/GCD(x,y));
}
/* find the greatest common divisor of two Integers */
Int VTCIMAGEBOX:: GCD(Int x, Int y)
{
  Int i;
  Int k;
  Int d=x<y?x:y; /* the lesser of x and y */
  d = (Int) sqrt((double)d)+1;
  k = 1;
  for(i=d;i>1;i--) {
    if(x%i==0 && y%i==0) {
      k=i;
      break;
    }
  }
  return(k);
}

/* Function: SubsampleMask()
   Description: Subsample the Mask;
   
   Input:
   InMask -- Input mask;
   Nx -- Horizontal subsampling rate;
   Ny -- vertical subsampling rate;
   Shape -- Integer number specify the object in a mask;

   Output:
   OutMask -- Output mask, memory space provided by caller function;

   return: DWT_OK if no error;
*/

Void  VTCIMAGEBOX:: SubsampleMask(UChar *InMask, UChar **OutMask, 
		    Int Width, Int Height,
		    FILTER *filter)
{
  UChar *a, *b;
  Int width, height;
  Int i,j, k;
  Int ret;
  VTCDWTMASK dummy;
  width = (Width >>1);
  height = (Height >>1);
  a = (UChar *)malloc(Width*Height*sizeof(UChar));
  b = (UChar *)malloc(width*height*sizeof(UChar));
  if(a == NULL || b == NULL) 
    exit(printf("Error allocation memory\n"));
  ret= dummy.do_DWTMask(InMask, a, Width, Height, 1, &filter);
  if(ret!=DWT_OK) 
    exit(printf("DWT error code = %d\n", ret));
  for(i=0,j=0, k=0; i< height; i++,j+=Width, k+=width){
    memcpy(b+k, a+j, width);
  }
  free(a);
  *OutMask = b;
}

/* FPDAM begin: added by SAIT */
Int VTCIMAGEBOX:: ExtendImageSize(Int InWidth, Int InHeight, 
		  Int Nx, Int Ny,
		  Int *OutWidth, Int *OutHeight, 
		  Int nLevels)
{
  Int out_width, out_height;
  Int blocksize =  1 << nLevels;

  if(blocksize%Nx!=0) blocksize = LCM(blocksize,Nx);
  if(blocksize%Ny!=0) blocksize = LCM(blocksize,Ny);
  
  /* first ajust the dimension to be multiple of blocksize */
  out_width =  (InWidth+(blocksize)-1)/blocksize*blocksize;
  out_height =  (InHeight+(blocksize)-1)/blocksize*blocksize;

  *OutWidth = out_width;
  *OutHeight = out_height;

  return(DWT_OK);
}

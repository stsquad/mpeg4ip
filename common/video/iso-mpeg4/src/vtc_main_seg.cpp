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


#include <stdio.h>
#include <math.h>
#include <string.h>
#include "dataStruct.hpp"

// #include "errorHandler.h"
//#include "globals.h"
//#include "dwt.h"
//#include "ShapeCodec.h"



/*--------------------------------------------------------------------
  read_segimage()
  
  Return Values
  -------------
  0: full-frame  1: shape-adaptive
  
  Comments
  --------
  Exits program if error occurs.
  --------------------------------------------------------------------*/
Int CVTCEncoder::read_segimage(Char *seg_path, Int seg_width, Int seg_height, 
		  Int img_colors,
		  PICTURE *MyImage)
{
  FILE *infptr = NULL;
  Int  seg_size;
  Int  status,col;
  UChar *inmask;

  if ((infptr = fopen(seg_path,"rb")) == NULL) { /* full-frame */
    MyImage[0].mask = MyImage[1].mask = MyImage[2].mask = NULL;
    return (0);
  }
  

  seg_size = seg_width*seg_height; 
  inmask = (UChar *)malloc(sizeof(UChar)*seg_size);

  if(inmask==NULL)
    errorHandler("Couldn't allocate memory to image mask\n");
  
  /* Read image */
  if ((status=fread(inmask, sizeof(UChar), seg_size,
		    infptr))
      != seg_size)
    errorHandler("Error in reading image file: %s\n",seg_path);
  fclose(infptr);
  
  MyImage[0].mask = inmask;
  for (col=1; col<img_colors; col++) 
    MyImage[col].mask = NULL;

  return 1;
}



#if 0 //deleted by SL 030399
Void CVTCEncoder::get_virtual_image(PICTURE *MyImage, Int wvtDecompLev, 
		       Int usemask, Int colors, Int alphaTH, 
		       //Int change_CR_disable,  //modified by SL 030399
			   FILTER *Filter) 
{
  Int Nx[3], Ny[3];
  Int Width[3], Height[3];
  Int nLevels[3];
  Int col,i;

  /* for 4:2:0 YUV image only */
  Nx[0] = Ny[0]=2;
  for(col=1; col<colors; col++) Nx[col]=Ny[col]=1;


  Width[0] = MyImage[0].width;
  Width[1] = Width[2] = (Width[0]+1)>>1;

  Height[0] = MyImage[0].height;
  Height[1] = Height[2] = (Height[0]+1)>>1;

  nLevels[0] = wvtDecompLev ;
  nLevels[1] = nLevels[2] = nLevels[0]-1;
  

  for (col=0; col<colors; col++) {
	  MyImage[col].mask = (UChar *)malloc(sizeof(UChar)*Width[col]*Height[col]);
	  for(i=0;i<Width[col]*Height[col];i++) {
		 MyImage[col].mask[i] = DWT_IN;
	  }
  }
  mzte_codec.m_iWidth = Width[0];
  mzte_codec.m_iHeight = Height[0];
  mzte_codec.m_iOriginX = Width[0];
  mzte_codec.m_iOriginY = Height[0];
  mzte_codec.m_iRealWidth = Width[0];
  mzte_codec.m_iRealHeight = Height[0];

}





Void CVTCDecoder::get_virtual_mask(PICTURE *MyImage,  Int wvtDecompLev,
		      Int w, Int h, Int usemask, Int colors, 
			  Int *target_shape_layer, Int StartCodeEnable,
			  FILTER **filters) 
{
//  Int width[3], height[3];
  Int Width[3], Height[3];
  Int nLevels[3];
  Int col,i;
  Int Nx[3], Ny[3];

  /* for 4:2:0 YUV image only */
  Nx[0] = Ny[0]=2;
  for(col=1; col<3; col++) Nx[col]=Ny[col]=1;

  nLevels[0] = wvtDecompLev;
  nLevels[1] = nLevels[2] = nLevels[0]-1;
  Width[0]= w;
  Width[1]=Width[2]= (w+1)>>1;
  
  Height[0]= h;
  Height[1]=Height[2] = (h+1)>>1;




	if (!usemask) {

		for (col=0; col<mzte_codec.m_iColors; col++) {
			MyImage[col].mask 
				=(UChar *)malloc(sizeof(UChar)*Width[col]*Height[col]);
			if (MyImage[col].mask==NULL)
				errorHandler("Couldn't allocate memory to image\n");
			for (i=0; i<Width[col]*Height[col]; i++)
				MyImage[col].mask[i] = DWT_IN;
		}
		mzte_codec.m_iWidth = Width[0];
		mzte_codec.m_iHeight = Height[0];
		mzte_codec.m_iOriginX = 0;
		mzte_codec.m_iOriginY = 0;
		mzte_codec.m_iRealWidth = Width[0];
		mzte_codec.m_iRealHeight = Height[0]; 
  }
//  else {
//    mzte_codec.m_iWidth = width[0];
//   mzte_codec.m_iHeight = height[0];
//  }

}
#endif //deleted by SL 030399

// FPDAM begin : added by Sharp (99/8/19)
Void CVTCEncoder::get_real_image(PICTURE *MyImage, Int wvtDecompLev, 
		       Int usemask, Int colors, Int alphaTH, 
		       /* Int change_CR_disable,*/ FILTER *Filter) 
{
  Int Nx[3], Ny[3];
  Int Width[3], Height[3], width[3], height[3];
  Int nLevels[3];
  UChar *inimage[3], *outimage[3];
  UChar *inmask[3], *outmask[3];
  Int ret,col;
  Int origin_x[3], origin_y[3];
  Int Shape;
  /* for 4:2:0 YUV image only */
  Nx[0] = Ny[0]=2;
  for(col=1; col<colors; col++) Nx[col]=Ny[col]=1;


  Width[0] = MyImage[0].width;
  Width[1] = Width[2] = (Width[0]+1)>>1;

  Height[0] = MyImage[0].height;
  Height[1] = Height[2] = (Height[0]+1)>>1;

  nLevels[0] = wvtDecompLev ;
  nLevels[1] = nLevels[2] = nLevels[0]-1;
  

  for (col=0; col<colors; col++) {
    
    inimage[col] = (UChar *)MyImage[col].data; 
    inmask[col] = (UChar *)MyImage[col].mask; 
    if(col==0) {
      Shape = usemask? ((STO_const_alpha) ? STO_const_alpha_value : MASK_VAL):RECTANGULAR;
      ret = GetRealMaskBox(inmask[col], &outmask[col],
		       Width[col], Height[col], Nx[col],Ny[col],
		       &width[col], &height[col], &origin_x[col], &origin_y[col],
		       Shape,
		       nLevels[col]);
			
      if (ret!=DWT_OK) 
	errorHandler("DWT Error code = %d\n", ret);
      if (usemask) free(inmask[col]);
    }
    else {
      width[col] = width[0]/Nx[0];
      height[col] = height[0]/Ny[0]; 
      origin_x[col] = origin_x[0]/Nx[0]; 
      origin_y[col] = origin_y[0]/Ny[0];
    }
    ret=GetBox(inimage[col], 
	       (Void **)&outimage[col][0], 
	       Width[col], Height[col], 
	       width[col], height[col], origin_x[col], origin_y[col], 0);
    
    if (ret!=DWT_OK) 
      errorHandler("DWT Error code = %d\n", ret);
    if(col==0) {
      if(usemask) {  /* do shape quantization */
	QuantizeShape(outmask[0], width[0], height[0], alphaTH);
      }
    }
    
    free(inimage[col]);
    MyImage[col].data = outimage[col];
    MyImage[col].mask = outmask[col];
  }

/*	{*/
/*    FILE *fp;*/
/*    if ((fp = fopen("ttt.yuv","w")) == NULL ){*/
/*      puts("Error");*/
/*      exit(1);*/
/*    }*/
/*    printf("%d %d\n", width[0], height[0]);*/
/*    printf("%d %d\n", Width[0], Height[0]);*/
/*    printf("%d\n",fwrite(MyImage[0].mask, sizeof(UChar), width[0]*height[0], fp));*/
/*    fclose(fp);*/
/*  }*/

  mzte_codec.m_iObjectWidth = mzte_codec.m_iWidth = width[0];
  mzte_codec.m_iObjectHeight = mzte_codec.m_iHeight = height[0];
  mzte_codec.m_iObjectOriginX = mzte_codec.m_iOriginX = origin_x[0];
  mzte_codec.m_iObjectOriginY = mzte_codec.m_iOriginY = origin_y[0];
  mzte_codec.m_iRealWidth = Width[0];
  mzte_codec.m_iRealHeight = Height[0];

}
// FPDAM end : added by Sharp (99/8/19)

//begin added by SL

Void CVTCEncoder::get_virtual_image(PICTURE *MyImage, Int wvtDecompLev, 
		       Int usemask, Int colors, Int alphaTH, 
		       /* Int change_CR_disable,*/ FILTER *Filter) 
{
  Int Nx[3], Ny[3];
  Int Width[3], Height[3], width[3], height[3];
  Int nLevels[3];
  UChar *inimage[3], *outimage[3];
  UChar *inmask[3], *outmask[3];
  Int ret,col,i;
  Int origin_x[3], origin_y[3];
  Int Shape;
  /* for 4:2:0 YUV image only */
  Nx[0] = Ny[0]=2;
  for(col=1; col<colors; col++) Nx[col]=Ny[col]=1;


  Width[0] = MyImage[0].width;
  Width[1] = Width[2] = (Width[0]+1)>>1;

  Height[0] = MyImage[0].height;
  Height[1] = Height[2] = (Height[0]+1)>>1;

  nLevels[0] = wvtDecompLev ;
  nLevels[1] = nLevels[2] = nLevels[0]-1;
  
  for (col=0; col<colors; col++) {
    
    inimage[col] = (UChar *)MyImage[col].data; 
    inmask[col] = (UChar *)MyImage[col].mask; 
    if(col==0) {
      Shape = usemask? ((STO_const_alpha) ? STO_const_alpha_value : MASK_VAL):RECTANGULAR;
      ret = GetMaskBox(inmask[col], &outmask[col],
		       Width[col], Height[col], Nx[col],Ny[col],
		       &width[col], &height[col], &origin_x[col], &origin_y[col],
		       Shape,
		       nLevels[col]);
      if (ret!=DWT_OK) 
	errorHandler("DWT Error code = %d\n", ret);
      if (usemask) free(inmask[col]);
    }
    else {
      width[col] = width[0]/Nx[0];
      height[col] = height[0]/Ny[0]; 
      origin_x[col] = origin_x[0]/Nx[0]; 
      origin_y[col] = origin_y[0]/Ny[0];
    }
    ret=GetBox(inimage[col], 
	       (Void **)&outimage[col][0], 
	       Width[col], Height[col], 
	       width[col], height[col], origin_x[col], origin_y[col], 0);
    
    if (ret!=DWT_OK) 
      errorHandler("DWT Error code = %d\n", ret);
    if(col==0) {
      if(usemask) {  /* do shape quantization */
	QuantizeShape(outmask[0], width[0], height[0], alphaTH);
      }
    }
    else {
      /* obtain new quantized mask for other color components */
      SubsampleMask(outmask[0], &(outmask[col]), width[0], 
		    height[0], Filter);
    }
    
    free(inimage[col]);
    MyImage[col].data = outimage[col];
    MyImage[col].mask = outmask[col];
    /* zero out outnode pixels */
    for(i=0;i<width[col]*height[col];i++)
      if(outmask[col][i]!=DWT_IN) outimage[col][i]=0;
  }


  mzte_codec.m_iWidth = width[0];
  mzte_codec.m_iHeight = height[0];
  mzte_codec.m_iOriginX = origin_x[0];
  mzte_codec.m_iOriginY = origin_y[0];
  mzte_codec.m_iRealWidth = Width[0];
  mzte_codec.m_iRealHeight = Height[0];
}

Void CVTCEncoder::get_virtual_image_V1(PICTURE *MyImage, Int wvtDecompLev, 
		       Int usemask, Int colors, Int alphaTH, 
		       Int change_CR_disable, FILTER *Filter) 
{
  Int Nx[3], Ny[3];
  Int Width[3], Height[3];
  Int nLevels[3];
  Int col,i;

  /* for 4:2:0 YUV image only */
  Nx[0] = Ny[0]=2;
  for(col=1; col<colors; col++) Nx[col]=Ny[col]=1;


  Width[0] = MyImage[0].width;
  Width[1] = Width[2] = (Width[0]+1)>>1;

  Height[0] = MyImage[0].height;
  Height[1] = Height[2] = (Height[0]+1)>>1;

  nLevels[0] = wvtDecompLev ;
  nLevels[1] = nLevels[2] = nLevels[0]-1;
  

  for (col=0; col<colors; col++) {
	  MyImage[col].mask = (UChar *)malloc(sizeof(UChar)*Width[col]*Height[col]);
	  for(i=0;i<Width[col]*Height[col];i++) {
		 MyImage[col].mask[i] = DWT_IN;
	  }
  }
  mzte_codec.m_iWidth = Width[0];
  mzte_codec.m_iHeight = Height[0];
  mzte_codec.m_iOriginX = Width[0];
  mzte_codec.m_iOriginY = Height[0];
  mzte_codec.m_iRealWidth = Width[0];
  mzte_codec.m_iRealHeight = Height[0];

}


Void CVTCDecoder::get_virtual_mask(PICTURE *MyImage,  Int wvtDecompLev,
		      Int w, Int h, Int usemask, Int colors, 
		      Int *target_levels, Int startCodeEnable,
		      FILTER **filters) 
{
  Int width[3], height[3];
  Int Width[3], Height[3];
  Int nLevels[3];
  UChar *outmask[3], *tempmask;
  Int ret,col;
  Int Nx[3], Ny[3];
 // Int origin_x[3], origin_y[3];

  /* for 4:2:0 YUV image only */
  Nx[0] = Ny[0]=2;
  for(col=1; col<3; col++) Nx[col]=Ny[col]=1;

  nLevels[0] = wvtDecompLev;
  nLevels[1] = nLevels[2] = nLevels[0]-1;
  Width[0]= w;
  Width[1]=Width[2]= (w+1)>>1;
  
  Height[0]= h;
  Height[1]=Height[2] = (h+1)>>1;
  
  for (col=0; col<colors; col++) {
    if(col==0) { /* get decoded shape of Y */
      outmask[col] = MyImage[col].mask =(UChar *)malloc(sizeof(UChar)*Width[col]*Height[col]);
      if(outmask[col]==NULL)
				errorHandler("Couldn't allocate memory to image\n");
      if (usemask) {
	/* ShapeDeCoding(outmask[0], Width[0], Height[0]); */
				ShapeDeCoding(outmask[0], Width[0], Height[0], 
								wvtDecompLev,  target_levels, 
								&STO_const_alpha,
								&STO_const_alpha_value,
								startCodeEnable,
								1, /* always get fullsize*/
								filters);
      }
      else {
				memset(outmask[0], (UChar) 1, Width[0]*Height[0]);
				*target_levels = 0;
      }
      if((Width[0] &((1<<wvtDecompLev)-1))!=0 || (Height[0] &((1<<wvtDecompLev)-1))!=0 ) {
				ret=ExtendMaskBox(outmask[0], &tempmask, Width[0], Height[0], Nx[0], Ny[0], &width[0], &height[0], nLevels[0]);
				if (ret!=DWT_OK) 
					errorHandler("ExtendMaskBox: DWT Error code = %d\n", ret);
				free(outmask[0]);
				outmask[0]=tempmask;
      }
      else {
				width[0] = Width[0];
				height[0] = Height[0];
      }
    }
    else { /* subsample shape for U,V*/
      SubsampleMask(outmask[0], &(outmask[col]), width[0], 
		    height[0],  filters[0]);
    }
    MyImage[col].mask = outmask[col];
  }

  if(!usemask) {
    mzte_codec.m_iWidth = width[0];
    mzte_codec.m_iHeight = height[0];
    mzte_codec.m_iOriginX = 0;
    mzte_codec.m_iOriginY = 0;
    mzte_codec.m_iRealWidth = Width[0];
    mzte_codec.m_iRealHeight = Height[0]; 
  }
  else {
    mzte_codec.m_iWidth = width[0];
    mzte_codec.m_iHeight = height[0];
  }
}


Void CVTCDecoder::get_virtual_mask_V1(PICTURE *MyImage,  Int wvtDecompLev,
		      Int w, Int h, Int usemask, Int colors, FILTER **filters) 
{
//  Int width[3], height[3];
  Int Width[3], Height[3];
  Int nLevels[3];
  Int col,i;
  Int Nx[3], Ny[3];

  /* for 4:2:0 YUV image only */
  Nx[0] = Ny[0]=2;
  for(col=1; col<3; col++) Nx[col]=Ny[col]=1;

  nLevels[0] = wvtDecompLev;
  nLevels[1] = nLevels[2] = nLevels[0]-1;
  Width[0]= w;
  Width[1]=Width[2]= (w+1)>>1;
  
  Height[0]= h;
  Height[1]=Height[2] = (h+1)>>1;




	if (!usemask) {

		for (col=0; col<mzte_codec.m_iColors; col++) {
			MyImage[col].mask 
				=(UChar *)malloc(sizeof(UChar)*Width[col]*Height[col]);
			if (MyImage[col].mask==NULL)
				errorHandler("Couldn't allocate memory to image\n");
			for (i=0; i<Width[col]*Height[col]; i++)
				MyImage[col].mask[i] = DWT_IN;
		}
		mzte_codec.m_iWidth = Width[0];
		mzte_codec.m_iHeight = Height[0];
		mzte_codec.m_iOriginX = 0;
		mzte_codec.m_iOriginY = 0;
		mzte_codec.m_iRealWidth = Width[0];
		mzte_codec.m_iRealHeight = Height[0]; 
  }
//  else {
//    mzte_codec.m_iWidth = width[0];
 //   mzte_codec.m_iHeight = height[0];
//  }

}
Void CVTCDecoder::get_virtual_mask_TILE(PICTURE *MyImage,  Int wvtDecompLev,
		      Int w, Int h, Int usemask, Int colors, 
			  Int *target_shape_layer, Int StartCodeEnable,
			  FILTER **filters) 
{
//  Int width[3], height[3];
  Int Width[3], Height[3];
  Int nLevels[3];
  Int col,i;
  Int Nx[3], Ny[3];

  /* for 4:2:0 YUV image only */
  Nx[0] = Ny[0]=2;
  for(col=1; col<3; col++) Nx[col]=Ny[col]=1;

  nLevels[0] = wvtDecompLev;
  nLevels[1] = nLevels[2] = nLevels[0]-1;
  Width[0]= w;
  Width[1]=Width[2]= (w+1)>>1;
  
  Height[0]= h;
  Height[1]=Height[2] = (h+1)>>1;

	if (!usemask) {

		for (col=0; col<mzte_codec.m_iColors; col++) {
			MyImage[col].mask 
				=(UChar *)malloc(sizeof(UChar)*Width[col]*Height[col]);
			if (MyImage[col].mask==NULL)
				errorHandler("Couldn't allocate memory to image\n");
			for (i=0; i<Width[col]*Height[col]; i++)
				MyImage[col].mask[i] = DWT_IN;
		}
		mzte_codec.m_iWidth = Width[0];
		mzte_codec.m_iHeight = Height[0];
		mzte_codec.m_iOriginX = 0;
		mzte_codec.m_iOriginY = 0;
		mzte_codec.m_iRealWidth = Width[0];
		mzte_codec.m_iRealHeight = Height[0]; 
  }
}


Int CVTCEncoder::
QuantizeShape(UChar *inmask, Int width, Int height, Int alphaTH)
{
  /* quantize the mask according to a 4x4 block alphaTH, should always favour zeros 
   since it will reduce the coefficients to be coded*/
 
  return(0);
}


Void CVTCEncoder::
PutBitstoStream_Still(Int bits,UInt code)
{
  while(bits>16) {
    emit_bits((UShort)(code>>(bits-16)), 16);
    bits-=16;
  }
  
  emit_bits((UShort)code, bits);
  return;
}

Int CVTCEncoder::
ByteAlignmentEnc_Still()
{
  flush_bits();
  return(0);
}
//end; added by SL 030399

// begin: added by Sharp (99/5/10)
// FPDAM begin : modified by Sharp
Void CVTCEncoder::cut_tile_image(PICTURE *DstImage, PICTURE *SrcImage, Int iTile, Int colors, Int tile_width, Int tile_height, FILTER *Filter)
// FPDAM end : modified by Sharp
{
	Int SrcWidth[3], DstWidth[3], SrcHeight[3], DstHeight[3];
	Int RealWidth[3], RealHeight[3];
	Int Width, Height;
	Int iTileX, iTileY, iTileW, iTileH;
	UChar *SrcPtr, *DstPtr, *DstMask;
/* FPDAM begin: added by Sharp */
	UChar *SrcMask, *OutMask;
/* FPDAM end: added by Sharp */
	Int iW, iH, i;

	iTileW = SrcImage[0].width / tile_width + ((SrcImage[0].width%tile_width)==0?0:1);
	iTileH = SrcImage[0].height / tile_height + ((SrcImage[0].height%tile_height)==0?0:1);

	iTileX = iTile % iTileW;
	iTileY = iTile / iTileW;

/*	printf("iTileW=%d\niTileX=%d\niTileY=%d\n", iTileW, iTileX, iTileY);*/
#ifdef  _FPDAM_DBG_
fprintf(stderr,"\n..............iTile=%d iTileW=%d iTileH=%d iTileX=%d iTileY=%d\n",
    iTile, iTileW, iTileH, iTileX, iTileY);
getchar();
#endif

	for ( i=0; i<colors; i++ ){
		SrcWidth[i] = SrcImage[i].width;
		SrcHeight[i] = SrcImage[i].height;
		DstWidth[i] = (i==0)?tile_width:tile_width/2;
		DstHeight[i] = (i==0)?tile_height:tile_height/2;
	
/*		printf("Source Size: %d %d\n", SrcWidth[i], SrcHeight[i]);*/
/*		printf("Destination Size: %d %d\n", DstWidth[i], DstHeight[i]);*/

		SrcPtr = (UChar *)(SrcImage[i].data) + iTileX * DstWidth[i] + iTileY * DstHeight[i] * SrcWidth[i];
/* FPDAM begin: added by Sharp */
		if (i==0) {
			SrcMask = (UChar *)(SrcImage[i].mask) + iTileX*DstWidth[i] + iTileY*DstHeight[i]*SrcWidth[i];
		} else {
			SrcMask = (UChar *)(DstImage[0].mask);
		}
/* FPDAM end: added by Sharp */

		DstPtr = (UChar *)(DstImage[i].data);
		DstMask = (UChar *)(DstImage[i].mask);

		/* Reset Mask */
		memset(DstMask, (UChar)0, DstImage[i].width*DstImage[i].height);
/*		memset(DstPtr, (UChar)0, DstImage[i].width*DstImage[i].height);*/

		/* For the case tile locates in boundary */
		if ( iTileX == iTileW-1 ) 
			RealWidth[i] = Width = SrcWidth[i] - DstWidth[i]*iTileX;
		else
			RealWidth[i] = Width = DstWidth[i];
		if ( iTileY == iTileH-1 ) 
			RealHeight[i] = Height = SrcHeight[i] - DstHeight[i]*iTileY;
		else
			RealHeight[i] = Height = DstHeight[i];
		
/*		printf("Cut region: %d %d\n", Width, Height);*/

		if ( i== 0 ) { 
			Int blocksize =  1 << mzte_codec.m_iWvtDecmpLev;
			Int virtual_width, virtual_height;

			blocksize = LCM(blocksize,2);
		
			virtual_width = mzte_codec.m_iRealWidth = Width;
			virtual_height = mzte_codec.m_iRealHeight = Height;
			mzte_codec.m_iOriginX = 0;
			mzte_codec.m_iOriginY = 0;
		
		/* first ajust the dimension to be multiple of blocksize */
			mzte_codec.m_iWidth = (virtual_width+(blocksize)-1)/blocksize*blocksize;
			mzte_codec.m_iHeight = (virtual_height+(blocksize)-1)/blocksize*blocksize;
  		mzte_codec.m_iDCWidth = GetDCWidth();
			mzte_codec.m_iDCHeight = GetDCHeight();
		}

/*		printf("Extended region: %d %d\n", mzte_codec.m_iWidth, mzte_codec.m_iHeight);*/
/*		printf("DC size : %d %d\n", mzte_codec.m_iDCWidth, mzte_codec.m_iDCHeight);*/

		DstImage[i].width = (i==0)?mzte_codec.m_iWidth:mzte_codec.m_iWidth/2;
		DstImage[i].height = (i==0)?mzte_codec.m_iHeight:mzte_codec.m_iHeight/2;
		
		/* Copying, and setting mask */
// FPDAM begin : modified by Sharp
#if 0
		for ( iH=0; iH<Height; iH++ ){
			for ( iW=0; iW<Width; iW++ ){
				*(DstPtr++) = *(SrcPtr++);
				*(DstMask++) = 1;
			}
			for ( iW=0; iW<DstImage[i].width-Width; iW++ ){
				*(DstMask++) = 0;
				*(DstPtr++) = 0;
			}
			SrcPtr += SrcWidth[i] - Width;
/*			DstPtr += DstImage[i].width - Width;*/
/*			DstMask += DstImage[i].width - Width;*/
		}
#endif
    for ( iH=0; iH<Height; iH++ ){
      for ( iW=0; iW<Width; iW++ ){
        *(DstPtr++) = *(SrcPtr++);
      }
      for (iW=0; iW<mzte_codec.m_Image[i].width - Width; iW++ ){
        *(DstPtr++) = 0;
      }
      SrcPtr += SrcWidth[i] - Width;
    }
    /* get mask value */
    if (i==0) {
      for ( iH=0; iH<Height; iH++ ){
				for ( iW=0; iW<Width; iW++ ){
					*(DstMask++) = *(SrcMask++);
				}
				for (iW=0; iW<mzte_codec.m_Image[i].width - Width; iW++ ){
					*(DstMask++) = 0;
				}
				SrcMask += SrcWidth[i] - Width;
			}
    } else {
			SubsampleMask(SrcMask, &OutMask,
        mzte_codec.m_Image[0].width, mzte_codec.m_Image[0].height, Filter);

      for ( iH=0; iH<mzte_codec.m_Image[i].height; iH++ )
				for ( iW=0; iW<mzte_codec.m_Image[i].width; iW++ )
         *(DstMask++) = *(OutMask+iH*mzte_codec.m_Image[i].width + iW);

      free(OutMask);
    }
// FPDAM begin : modified by Sharp

/*		for (iH=0; iH<DstImage[i].height; iH++){*/
/*			for ( iW=0; iW<DstImage[i].width; iW++ )*/
/*				printf("%d", *(DstImage[i].mask + iW + iH*DstImage[i].width));*/
/*			puts("");*/
/*		}*/
	}

/*	{*/
/*		FILE *fp;*/
/*		char test[100];*/
/*		sprintf(test, "ttt.yuv%d", iTile);*/
/*		fp = fopen(test, "w");*/
/*		fwrite(DstImage[0].data, 1, RealWidth[0]*RealHeight[0], fp);*/
/*		fwrite(DstImage[1].data, 1, RealWidth[1]*RealHeight[1], fp);*/
/*		fwrite(DstImage[2].data, 1, RealWidth[2]*RealHeight[2], fp);*/
/*		fclose(fp);*/
/*	}*/
}


// FPDAM begin: modified by Sharp
Void CVTCDecoder::get_virtual_tile_mask(
				PICTURE *MyImage,  Int wvtDecompLev,
				Int object_width, Int object_height, 
				Int tile_width, Int tile_height,
				Int iTile, Int TileX, Int TileY,
				Int usemask, Int tile_type, Int colors, 
			  Int *target_levels, 
				Int StartCodeEnable,
			  FILTER **filters)
{
			Int TileXPos, TileYPos;
//			Int Width, Height;
//			Int RealWidth, RealHeight, i;
//			UChar *DstMask;
			Int Width[3], Height[3];
			Int RealWidth[3], RealHeight[3], i;
			UChar *outmask[3], *DstMask;
			Int iH, iW;
//			Int blocksize =  1 << mzte_codec.m_iWvtDecmpLev;
//			Int virtual_width, virtual_height;
			Int blocksize = 1 << wvtDecompLev;

			Int x = blocksize;
			Int y = 2;
			Int k;
			Int d=x<y?x:y; /* the lesser of x and y */

			TileXPos = iTile%TileX;
			TileYPos = iTile/TileX;

#ifdef	_FPDAM_DBG_
fprintf(stderr,"\n..............iTile=%d iTileX=%d iTileY=%d TileXPos=%d TileYPos=%d\n", 
		iTile, TileX, TileY, TileXPos, TileYPos);
getchar();
#endif

/*			printf("TILE position %d %d\n", TileXPos, TileYPos);*/
/*			printf("%d\n", mzte_codec.m_iWidth);*/
/*			printf("%d %d\n", mzte_codec.m_iPictWidth, mzte_codec.m_iPictHeight);*/
/*			printf("%d\n", mzte_codec.m_tile_width);*/

/*			UPDATE width, height, tile_width and tile_height here*/
/*
			if ( TileXPos == TileX-1 )
				Width = mzte_codec.m_iPictWidth - mzte_codec.m_tile_width*TileXPos;
			else
				Width = mzte_codec.m_tile_width;
			if ( TileYPos == TileY-1 ) 
				Height = mzte_codec.m_iPictHeight - mzte_codec.m_tile_height*TileYPos;
			else
				Height = mzte_codec.m_tile_height;
*/
			if ( TileXPos == TileX-1 )
				mzte_codec.m_iRealWidth = mzte_codec.m_iObjectWidth - tile_width*TileXPos;
			else
				mzte_codec.m_iRealWidth = tile_width;
			if ( TileYPos == TileY-1 ) 
				mzte_codec.m_iRealHeight = mzte_codec.m_iObjectHeight - tile_height*TileYPos;
			else
				mzte_codec.m_iRealHeight = tile_height;
			
			/*
printf("%d %d\n",mzte_codec.m_iObjectWidth, mzte_codec.m_iObjectHeight);
printf("%d %d\n",mzte_codec.m_iRealWidth,mzte_codec.m_iRealHeight);
getchar();
*/
/*			printf("%d %d\n", Width, Height);*/

			d = (Int) sqrt((double)d)+1;
			k = 1;
			for(i=d;i>1;i--) {
				if(x%i==0 && y%i==0) {
					k=i;
					break;
				}
			}
			blocksize = blocksize*2/k;
			
//			virtual_width = mzte_codec.m_iRealWidth = Width;
//			virtual_height = mzte_codec.m_iRealHeight = Height;
			mzte_codec.m_iOriginX = 0;
			mzte_codec.m_iOriginY = 0;

			/* first ajust the dimension to be multiple of blocksize */
			mzte_codec.m_iWidth = (mzte_codec.m_iRealWidth+(blocksize)-1)/blocksize*blocksize;
			mzte_codec.m_iHeight = (mzte_codec.m_iRealHeight+(blocksize)-1)/blocksize*blocksize;

/*			printf("%d %d\n", mzte_codec.m_iWidth, mzte_codec.m_iHeight);*/

			for ( i=0; i<mzte_codec.m_iColors; i++ ){
//				RealWidth = (i==0)?mzte_codec.m_iRealWidth:((mzte_codec.m_iRealWidth+1)>>1);
//				RealHeight = (i==0)?mzte_codec.m_iRealHeight:((mzte_codec.m_iRealHeight+1)>>1);
//				Width = (i==0)?mzte_codec.m_iWidth:((mzte_codec.m_iWidth+1)>>1);
//				Height = (i==0)?mzte_codec.m_iHeight:((mzte_codec.m_iHeight+1)>>1);
				RealWidth[i] = (i==0)?mzte_codec.m_iRealWidth:((mzte_codec.m_iRealWidth+1)>>1);
				RealHeight[i] = (i==0)?mzte_codec.m_iRealHeight:((mzte_codec.m_iRealHeight+1)>>1);
				Width[i] = (i==0)?mzte_codec.m_iWidth:((mzte_codec.m_iWidth+1)>>1);
				Height[i] = (i==0)?mzte_codec.m_iHeight:((mzte_codec.m_iHeight+1)>>1);

/*				printf("RealSize = (%dx%d)\n", RealWidth, RealHeight);*/
/*				printf("Size = (%dx%d)\n", Width, Height);*/
			
//				DstMask = Image[i].mask;
				/* Reset Mask */
//				memset(DstMask, (UChar)0, Width*Height);
				
				outmask[i] = MyImage[i].mask;
				memset(outmask[i], (UChar)0, Width[i]*Height[i]);

				/* Copying, and setting mask */

/*
				for ( iH=0; iH<RealHeight; iH++ ){
					for ( iW=0; iW<RealWidth; iW++ ){
						*(DstMask++) = 1;
					}
					for (iW=0; iW<Width-RealWidth; iW++ ){
						*(DstMask++) = 0;
					}
				}
*/


      if (usemask) {
				switch (tile_type) {
					case BOUNDA_TILE:
						if ( i==0 ) {
							ShapeDeCoding(outmask[0], Width[0], Height[0], 
											wvtDecompLev,  target_levels, 
											&STO_const_alpha,
											&STO_const_alpha_value,
											StartCodeEnable,
											1, /* always get fullsize*/
											filters);
						} else {
							SubsampleMask(outmask[0], &DstMask, Width[0], 
								Height[0],  filters[0]);
							for ( iH=0; iH<Height[i]; iH++ )
								for ( iW=0; iW<Width[i]; iW++ )
									*(outmask[i]++) = *(DstMask+iH*Width[i]+iW);
							free(DstMask);
						}
						break;
					case OPAQUE_TILE:
						for ( iH=0; iH<RealHeight[i]; iH++ ){
							for ( iW=0; iW<RealWidth[i]; iW++ ){
								*(outmask[i]++) = DWT_IN;
							}
							for (iW=0; iW<Width[i]-RealWidth[i]; iW++ ){
								*(outmask[i]++) = 0;
							}
						}
						*target_levels=0;/* ???*/
						break;
					case TRANSP_TILE:
						for ( iH=0; iH<Height[i]; iH++ ){
							for ( iW=0; iW<Width[i]; iW++ ){
								*(outmask[i]++) = 0;
							}
						}
						*target_levels=0; /* ??? */
						break;
					default:
						errorHandler("Wrong texture_object_layer_start_code.");
				}
      }
			else {
				for ( iH=0; iH<RealHeight[i]; iH++ ){
					for ( iW=0; iW<RealWidth[i]; iW++ ){
						*(outmask[i]++) = DWT_IN;
					}
					for (iW=0; iW<Width[i]-RealWidth[i]; iW++ ){
						*(outmask[i]++) = 0;
					}
				}
				*target_levels=0; /* ??? */
			}

/*				DstMask = Image[i].mask;*/
/*				for ( iH=0; iH<Height; iH++ ){*/
/*					for ( iW=0; iW<Width; iW++ )*/
/*						printf("%d", *(DstMask + iW + iH*Width));*/
/*					puts("");*/
/*				}*/
			}

}

// end: added by Sharp (99/5/10)

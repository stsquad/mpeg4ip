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
#include "dataStruct.hpp"

// #include "errorHandler.h"
//#include "globals.h"
//#include "dwt.h"
//#include "ShapeCodec.h"

Int STO_const_alpha;
UChar STO_const_alpha_value;

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




Void CVTCEncoder::get_virtual_image(PICTURE *MyImage, Int wvtDecompLev, 
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




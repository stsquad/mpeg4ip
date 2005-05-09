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

#include "stdlib.h"
#include "stdio.h"
#include "math.h"
#include "basic.hpp"
#include "dataStruct.hpp"


Void CVTCEncoder::read_image(Char *img_path, 
							Int img_width, 
							Int img_height, 
							Int img_colors, 
							Int img_bit_depth,
							PICTURE *img)
{
  FILE *infptr = NULL;
  int  img_size;
   /* chroma  width and height  to handle odd image size */
  int img_cwidth  = (img_width+1)/2, 
      img_cheight = (img_height+1)/2; 
  int wordsize = (img_bit_depth>8)?2:1;
  unsigned char *srcimg; /* SL: to handle 1-16 bit */
  int  i,k,status;
  
  if (img_colors==1) { /*SL: image size for mono */
     img_size = img_width*img_height; /* bit_depth>8 occupies 2 bytes */
  }
  else { /* only for 420 color for now */
    img_size = img_width*img_height+2*img_cwidth*img_cheight; 
  }
  
  srcimg = new UChar[img_size*wordsize];
  if ((infptr = fopen(img_path,"rb")) == NULL) 
    exit(fprintf(stderr,"Unable to open image_file: %s\n",img_path));

  if ((status=fread(srcimg, sizeof(unsigned char)*wordsize, img_size, infptr))
       != img_size)
    exit(fprintf(stderr,"Error in reading image file: %s\n",img_path));

  fclose(infptr);
  
  /* put into VM structure */
  img[0].width  = img_width;
  img[0].height = img_height;
  if(img_colors != 1) { /* SL: only for color image */
    img[1].width  = img_cwidth; /* to handle odd image size */
    img[1].height = img_cheight;
    img[2].width  = img_cwidth;
    img[2].height = img_cheight;
  }

  img[0].data = (void *)new UChar[img_width*img_height*wordsize];
  if (img_colors==3) {
	img[1].data = (void *)new UChar[img_cwidth*img_cheight*wordsize];
    img[2].data = (void *)new UChar[img_cwidth*img_cheight*wordsize];
  }

  k=0;
  for (i=0; i<img_width*img_height*wordsize; i++) { 
    if (img_bit_depth > 8){ /* mask the input */
      if (i%2==0)
        ((UChar *)img[0].data)[i] =
	      (UChar) (srcimg[k++]&((1<<(img_bit_depth-8))-1));
      else
        ((unsigned char *)img[0].data)[i] = srcimg[k++];
    }
    else {
      ((unsigned char *)img[0].data)[i] = 
	(unsigned char) (srcimg[k++]&((1<<img_bit_depth)-1));
    }
  }
  if (img_colors != 1) { /* SL: only for color image */
    for (i=0; i<img_cwidth*img_cheight*wordsize; i++) {
      if (img_bit_depth > 8) { /* mask the input */
        if (i%2==0) 
           ((unsigned char *)img[1].data)[i] = 
	     (unsigned char) (srcimg[k++]&((1<<(img_bit_depth-8))-1));
        else
           ((unsigned char *)img[1].data)[i] = srcimg[k++];
      }
      else {
        ((unsigned char *)img[1].data)[i] = 
	  (unsigned char) (srcimg[k++]&((1<<img_bit_depth)-1));
      }
    }
    for (i=0; i<img_cwidth*img_cheight*wordsize; i++){
      if (img_bit_depth > 8) { /* mask the input */
        if (i%2==0) 
           ((unsigned char *)img[2].data)[i] = 
	     (unsigned char) (srcimg[k++]&((1<<(img_bit_depth-8))-1));
        else
          ((unsigned char *)img[2].data)[i] = srcimg[k++];
      }
      else {
        ((unsigned char *)img[2].data)[i] = 
	  (unsigned char) (srcimg[k++]&((1<<img_bit_depth)-1));
      }
    }
  }
  if (srcimg) delete (srcimg);

}

// begin: added by Sharp (99/2/16)
Void CVTCEncoder::init_tile(Int tile_width, Int tile_height)
{ 
  PICTURE *img;
  Int img_cwidth, img_cheight;
  Int wordsize;
  Int img_size;
  Int img_colors = mzte_codec.m_iColors;
  Int img_bit_depth = mzte_codec.m_iBitDepth;
  Int col;
  
  img_cwidth = (tile_width+1)/2;
  img_cheight = (tile_height+1)/2;
  wordsize = (img_bit_depth > 8) ? 2 : 1;
  
  if (img_colors==MONO) { /*SL: image size for mono */
    img_size = tile_width*tile_height; /* bit_depth>8 occupies 2 bytes */
  }
  else { /* only for 420 color for now */
    img_size = tile_width*tile_height+2*img_cwidth*img_cheight;
  } 
    
/*  vm_param_org->Image = vm_param->Image;*/

	mzte_codec.m_ImageOrg = mzte_codec.m_Image;
    
  if ((img = (PICTURE *)malloc(sizeof(PICTURE)*img_colors))==NULL)
    errorHandler("error allocating memory \n");
  
  img[0].width = tile_width;
  img[0].height = tile_height;
  if(img_colors != MONO){
    img[1].width  = img_cwidth;
    img[1].height = img_cheight;
    img[2].width  = img_cwidth;
    img[2].height = img_cheight;
  } 

  if ((img[0].data=(Void *)
      malloc(sizeof(UChar)*tile_width*tile_height*wordsize)) == NULL)
    errorHandler("Couldn't allocate memory to image->Y->data\n");
  if (img_colors != MONO) { /* SL: only for color image */
    if ((img[1].data = (Void *)
        malloc(sizeof(UChar)*img_cwidth*img_cheight*wordsize))==NULL)
      errorHandler("Couldn't allocate memory to image->U->data\n");
    if ((img[2].data = (Void *)
        malloc(sizeof(UChar)*img_cwidth*img_cheight*wordsize))==NULL)
      errorHandler("Couldn't allocate memory to image->V->data\n");
  }

// FPDAM begin: modified by Sharp
#if 0
  for(col=0; col<img_colors; col++){
    img[col].mask = (UChar *)malloc(sizeof(UChar)*tile_width*tile_height);
    if (img[col].mask == NULL)
      errorHandler("error allocating memory \n");
    memset(img[col].mask, 1, sizeof(UChar)*tile_width*tile_height);
  }
#endif
  for(col=0; col<img_colors; col++){
    if(col==0) {
      img[col].mask = (UChar *)malloc(sizeof(UChar)*tile_width*tile_height);
      if (img[col].mask == NULL)
        errorHandler("error allocating memory \n");
      memset(img[col].mask, 1, sizeof(UChar)*tile_width*tile_height);
    } else {
      img[col].mask=NULL;
    }
  }
// FPDAM end: modified by Sharp

/*  vm_param->Image = img;*/
	mzte_codec.m_Image = img;
} 
// end: added by Sharp (99/2/16)

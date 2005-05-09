/*************************************************************************

This software module was originally developed by 

	Hiroyuki Katata (katata@imgsl.mkhar.sharp.co.jp), Sharp Corporation
	Norio Ito (norio@imgsl.mkhar.sharp.co.jp), Sharp Corporation
	Shuichi Watanabe (watanabe@imgsl.mkhar.sharp.co.jp), Sharp Corporation
	(date: August, 1997)

  and edited by
      Sehoon Son (shson@unitel.co.kr) Samsung AIT
	  Simon Winder (swinder@microsoft.com)

in the course of development of the MPEG-4 Video (ISO/IEC 14496-2). This
software module is an implementation of a part of one or more MPEG-4 Video
tools as specified by the MPEG-4 Video. ISO/IEC gives users of the MPEG-4
Video free license to this software module or modifications thereof for use
in hardware or software products claiming conformance to the MPEG-4
Video. Those intending to use this software module in hardware or software
products are advised that its use may infringe existing patents. The
original developer of this software module and his/her company, the
subsequent editors and their companies, and ISO/IEC have no liability for
use of this software module or modifications thereof in an
implementation. Copyright is not released for non MPEG-4 Video conforming
products. Sharp retains full right to use the code for his/her own
purpose, assign or donate the code to a third party and to inhibit third
parties from using the code for non <MPEG standard> conforming
products. This copyright notice must be included in all copies or derivative
works.

Copyright (c) 1997.

Module Name:

	tps_bfshape.cpp

Abstract:

	set volmd, vopmd for backward/forward shape coding
	 (for Object based Temporal Scalability)

Revision History:

    Feb.16 1999 : add Quarter Sample 
                  Mathias Wien (wien@ient.rwth-aachen.de) 
	Feb.01 2000 : Bug fixed of Spatial Scalability by Takefumi Nagumo (SONY)

*************************************************************************/

#include "typeapi.h"
#include "entropy.hpp"
#include "huffman.hpp"
#include "bitstrm.hpp"
#include "global.hpp"
#include "mode.hpp"
#include "codehead.h"
#include "cae.h"
#include "vopses.hpp"
#include "vopseenc.hpp"
#include "tps_enhcbuf.hpp"

#ifdef __MFC_
#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif


#define new DEBUG_NEW				   
#endif // __MFC_

Int QMatrix [BLOCK_SQUARE_SIZE] = {
	16, 16, 16, 16, 16, 16, 16, 16,
	16, 16, 16, 16, 16, 16, 16, 16,
	16, 16, 16, 16, 16, 16, 16, 16,
	16, 16, 16, 16, 16, 16, 16, 16,
	16, 16, 16, 16, 16, 16, 16, 16,
	16, 16, 16, 16, 16, 16, 16, 16,
	16, 16, 16, 16, 16, 16, 16, 16,
	16, 16, 16, 16, 16, 16, 16, 16
};

Void set_modes(VOLMode* volmd, VOPMode* vopmd) 
{
    ///// set VOL modes
	volmd->volType			= (VOLtype) BASE_LAYER; // should not be used.
	volmd->fAUsage			= ONE_BIT;
	volmd->bShapeOnly		= FALSE; // should be changed to TRUE after checked.
//	volmd->iBinaryAlphaTH		= rgiBinaryAlphaTH [iVO] * 16;  //magic no. from the vm
	volmd->bNoCrChange		= TRUE; // mb level binary alpha size conversion disable
	volmd->bOriginalForME		= TRUE;
	volmd->bAdvPredDisable		= TRUE;
	volmd->bQuarterSample		= FALSE; //QuarterSample
// Modified by Toshiba (98-04-07) 
//	volmd->bErrorResilientDisable 	= TRUE;
// End Toshiba
	volmd->fQuantizer		= Q_H263; // should not be used. H.263/MPEG quantizer.
	volmd->bNoGrayQuantUpdate 	= FALSE; // should not be used.
	volmd->bLoadIntraMatrix		= FALSE; // should not be used.
	volmd->bLoadInterMatrix		= FALSE; // should not be used.
	volmd->bLoadIntraMatrixAlpha  = FALSE; // should not be used.
	volmd->bLoadInterMatrixAlpha  = FALSE; // should not be used.
	volmd->bVPBitTh = -1; // added by Sharp (98/4/13)
	memcpy (volmd->rgiIntraQuantizerMatrix, QMatrix, BLOCK_SQUARE_SIZE * sizeof (Int));
//	memcpy (volmd->rgiInterQuantizerMatrix,		rgppiInterQuantizerMatrix [iLayer][iVO], BLOCK_SQUARE_SIZE * sizeof (Int));
//	memcpy (volmd->rgiIntraQuantizerMatrixAlpha,	rgppiIntraQuantizerMatrixAlpha [iLayer][iVO], BLOCK_SQUARE_SIZE * sizeof (Int));
//	memcpy (volmd->rgiInterQuantizerMatrixAlpha,	rgppiInterQuantizerMatrixAlpha [iLayer][iVO], BLOCK_SQUARE_SIZE * sizeof (Int));
	volmd->bDeblockFilterDisable	= FALSE; // should not be used.
	volmd->fEntropyType		= huffman; // huffman VLC for texture ? ... may not be used 
//	volmd->iPeriodOfIVOP	= (rgiNumOfBbetweenPVOP [iVO] + 1) * (rgiNumOfPbetweenIVOP [iVO] + 1);	//in temporally sumsampled domain
//	volmd->iPeriodOfPVOP	= rgiNumOfBbetweenPVOP [iVO] + 1;			//in temporally sumsampled domain
//	volmd->iTemporalRate	= rgiTemporalRate [iVO];
//	volmd->iClockRate		= SOURCE_FRAME_RATE / volmd->iTemporalRate;	//default to frame per second
	volmd->bDumpMB			= FALSE; // should not be used.
	volmd->bTrace			= FALSE; // if changed to TRUE, Trace files for backward/forward shape should be prepared like in getInputFiles() and CVideoObjectEncoderTPS().

    ///// set VOP modes
//	vopmd->iSearchRangeForward	= rguiSearchRange [iLayer][iVO];
//	vopmd->iSearchRangeBackward	= rguiSearchRange [iLayer][iVO];

// Q-step sizes
	vopmd->intStepI			= 10; // set temporalily, should not be used after binary_shape_only mode will be integrated.
//	vopmd->intStep			= rgiStep [iLayer][iVO];
//	vopmd->intStepIAlpha		= rgiStepIAlpha [iLayer][iVO];
//	vopmd->intStepAlpha		= rgiStepAlpha [iLayer][iVO];

//	vopmd->intDBQuant		= rgiStepBCode [iLayer][iVO];
	vopmd->iIntraDcSwitchThr        = 0; // threhold to code Intra DC as AC coefs. 0=never do that

	vopmd->vopPredType      = IVOP;
}

Void write420_sep(Int num, char *name, PixelC* destY, PixelC* destU, PixelC* destV, Int width, Int height) // modified by Sharp (98/10/26)
{
#ifdef __OUT_ONE_FRAME_
  char file[100];
#endif
  FILE *fp;

#ifndef __OUT_ONE_FRAME_
  fp = fopen(name, "ab");
#else
  sprintf(file, "%s%d", name ,num);
  fp = fopen(file, "wb");
#endif
  fwrite(destY, sizeof (PixelC), width*height, fp);
  fwrite(destU, sizeof (PixelC), width*height/4, fp);
  fwrite(destV, sizeof (PixelC), width*height/4, fp);
  fclose(fp);
}

// begin: added by Sharp (98/10/26)
Void write420_jnt(FILE *fp, PixelC* destY, PixelC* destU, PixelC* destV, Int width, Int height)
{
  fwrite(destY, sizeof (PixelC), width*height, fp);
  fwrite(destU, sizeof (PixelC), width*height/4, fp);
  fwrite(destV, sizeof (PixelC), width*height/4, fp);
}
// end: added by Sharp (98/10/26)
//OBSSFIX_MODE3
Void write420_jnt_withMask(FILE *fp, PixelC* destY, PixelC* destU, PixelC* destV, PixelC* destBY, PixelC* destBUV, Int width, Int height)
{
	unsigned char pxlZeroY = 0 ;
	unsigned char pxlZeroUV = 128;
	unsigned char* destBUV_tmp = destBUV;
	Int i =0 , j =0;

	for (i=0; i < height; i++){
		for (j=0; j < width; j++){
			if ( *destBY == MPEG4_OPAQUE)	fwrite(destY, sizeof (unsigned char), 1, fp);
			else					fwrite(&pxlZeroY, sizeof (unsigned char), 1, fp);
			destY++; 
			destBY++;
		}
	}
	for (i=0; i < height/2; i++){
		for (j=0; j < width/2; j++){
			if ( *destBUV == MPEG4_OPAQUE)fwrite(destU, sizeof (unsigned char), 1, fp);
			else					fwrite(&pxlZeroUV, sizeof (unsigned char), 1, fp);
			destU++; 
			destBUV++;
		}
	}
	for (i=0; i < height/2; i++){
		for (j=0; j < width/2; j++){
			if ( *destBUV_tmp == MPEG4_OPAQUE)fwrite(destV, sizeof (unsigned char), 1, fp);
			else						fwrite(&pxlZeroUV, sizeof (unsigned char), 1, fp);
			destV++;
			destBUV_tmp++;
		}
	}
}
//~OBSSFIX_MODE3
Void write_seg_test(Int num, char *name, PixelC* destY, Int width, Int height)
{
  char file[100];
  FILE *fp;

  sprintf(file, "%s%d", name ,num);
  fp = fopen(file, "a");
  fwrite(destY, sizeof (PixelC), width*height, fp);
  fclose(fp);
}



Void bg_comp_each(PixelC* f_curr, PixelC* f_prev, PixelC* f_next, PixelC* mask_curr, PixelC* mask_prev, PixelC* mask_next, Int curr_t, Int prev_t, Int next_t, Int width, Int height, Int CoreMode)
{
  Int i;
  PixelC* out_image    = new PixelC [width*height];
  PixelC* mask_overlap = new PixelC [width*height];
/* NBIT: change unsigned char to PixelC
  Void pre_pad(unsigned char *mask, unsigned char *curr, int width, int height);
*/
  Void pre_pad(PixelC *mask, PixelC *curr, int width, int height);

// begin: added by Sharp (98/3/24)
	if ( CoreMode ){
		for(i=0; i<width*height; i++)
			if(mask_curr[i] == 0 )         // should be used after Type 1 is implemented 
				f_curr[i] = f_prev[i];
	}
	else {
// end: added by Sharp (98/3/24)
  /* ----- put nearest frame ----- */
  if( (abs(curr_t-prev_t) > abs(curr_t-next_t) ) && (abs(abs(curr_t-prev_t) - abs(curr_t-next_t) ) < 1 ))
    for(i=0; i<width*height; i++)
	out_image[i]=f_next[i];
  else
    for(i=0; i<width*height; i++)
	 out_image[i]=f_prev[i];

  /* ----- perform background composition ----- */
  for(i=0; i<width*height; i++) {
    if(mask_prev[i] > 0 && mask_next[i] == 0)	  /* ----- f_next is used ----- */
	out_image[i]=f_next[i];
    else if(mask_prev[i] == 0 && mask_next[i] > 0)/* ----- f_prev is used ----- */
	out_image[i]=f_prev[i];

    if(mask_prev[i] > 0 && mask_next[i] > 0)	  /* ----- overlapped area ----- */
	mask_overlap[i]=0;
    else
	mask_overlap[i]=1;
  }
  pre_pad(mask_overlap, out_image, width, height); /* for padding */

  /* ----- overlap current frame to back ground ----- */
  for(i=0; i<width*height; i++)
    if(mask_curr[i] == 0 )         // should be used after Type 1 is implemented 
      f_curr[i] = out_image[i];
	} // added by Sharp (98/3/24)

  delete out_image;
  delete mask_overlap;
}

//OBSS_SAIT_991015	//for OBSS partial enhancement mode
Void bg_comp_each(PixelC* f_curr, PixelC* f_prev, PixelC* mask_curr, PixelC* mask_prev, Int curr_t, Int width, Int height, CRct rctCurr)
{
  Int i,j;
  PixelC* out_image    = new PixelC [width*height];
  PixelC* out_mask     = new PixelC [width*height];

  for(i=0; i<height; i++){
	for(j=0; j<width; j++){
		if( j>=rctCurr.left && j<rctCurr.right && i>=rctCurr.top && i<rctCurr.bottom){
			out_image[i*width+j] = f_curr[i*width+j];
			out_mask[i*width+j] = mask_curr[i*width+j];
		}
		else {
			out_image[i*width+j] = f_prev[i*width+j];
			out_mask[i*width+j] = mask_prev[i*width+j];
		}
	}
  }
	for(i=0; i<width*height; i++){
		f_curr[i] = out_image[i];
		mask_curr[i] = out_mask[i];
	} 

  delete out_image;
  delete out_mask;
}
//OBSSFIX_MODE3
Void bg_comp_each_mode3(PixelC* f_curr, PixelC* f_prev, PixelC* mask_curr, PixelC* mask_prev, Int curr_t, Int width, Int height, CRct rctCurr)
{
  Int i,j;
  PixelC* out_image    = new PixelC [width*height];
  PixelC* out_mask     = new PixelC [width*height];

  for(i=0; i<height; i++){
	for(j=0; j<width; j++){
		if( j>=rctCurr.left && j<rctCurr.right && i>=rctCurr.top && i<rctCurr.bottom && mask_curr[i*width+j] !=0 ){
			out_image[i*width+j] = f_curr[i*width+j];
			out_mask[i*width+j] = mask_curr[i*width+j];
		}
		else {
			out_image[i*width+j] = f_prev[i*width+j];
			out_mask[i*width+j] = mask_prev[i*width+j];
		}
	}
  }
	for(i=0; i<width*height; i++){
		f_curr[i] = out_image[i];
		mask_curr[i] = out_mask[i];
	} 

  delete out_image;
  delete out_mask;
}
//~OBSSFIX_MODE3
//~OBSS_SAIT_991015

Void inv_convertYuv(const CVOPU8YUVBA* pvopcSrc, PixelC* destY, PixelC* destU, PixelC* destV, Int width, Int height)
{
    CoordI x, y;
    Int Fwidth   = pvopcSrc->whereY ().width;
    Int FwidthUV = pvopcSrc->whereUV ().width;

    Int nSkipYPixel = Fwidth  * EXPANDY_REF_FRAME  + EXPANDY_REF_FRAME;
    Int nSkipUVPixel = FwidthUV * EXPANDUV_REF_FRAME + EXPANDUV_REF_FRAME;
    PixelC* ppxlcY = (PixelC*)((pvopcSrc->pixelsY ()) + nSkipYPixel );
    PixelC* ppxlcU = (PixelC*)((pvopcSrc->pixelsU ()) + nSkipUVPixel );
    PixelC* ppxlcV = (PixelC*)((pvopcSrc->pixelsV ()) + nSkipUVPixel );
    PixelC* pdY = destY;    PixelC* pdU = destU;    PixelC* pdV = destV;
    PixelC* psY;    PixelC* psU;    PixelC* psV;
	
  // convert
    for (y = 0; y < height; y++) {
      psY = ppxlcY;
      for (x = 0; x < width; x++) {
	*psY = *pdY;
	pdY++;
	psY++;
      }
      ppxlcY += Fwidth;
    }
    for (y = 0; y < height/2; y++) {
      psU = ppxlcU;
      for (x = 0; x < width/2; x++) {
	*psU = *pdU;
	pdU++;
	psU++;
      }
      ppxlcU += FwidthUV;
    }
    for (y = 0; y < height/2; y++) {
      psV = ppxlcV;
      for (x = 0; x < width/2; x++) {
	*psV = *pdV;
	pdV++;
	psV++;
      }
      ppxlcV += FwidthUV;
    }
/*
  printf("======== width = %d, height = %d\n", width, height);
  FILE *fp;
  fp = fopen("bbb", "w");
  fwrite(destY, sizeof (PixelC), width*height, fp);
  fwrite(destU, sizeof (PixelC), width*height/4, fp);
  fwrite(destV, sizeof (PixelC), width*height/4, fp);
  fclose(fp);
  exit(1);
*/
}

Void convertYuv(const CVOPU8YUVBA* pvopcSrc, PixelC* destY, PixelC* destU, PixelC* destV, Int width, Int height)
{
    CoordI x, y;
    Int Fwidth   = pvopcSrc->whereY ().width;
    Int FwidthUV = pvopcSrc->whereUV ().width;

    Int nSkipYPixel = Fwidth  * EXPANDY_REF_FRAME  + EXPANDY_REF_FRAME;
    Int nSkipUVPixel = FwidthUV * EXPANDUV_REF_FRAME + EXPANDUV_REF_FRAME;
    PixelC* ppxlcY = (PixelC*)((pvopcSrc->pixelsY ()) + nSkipYPixel );
    PixelC* ppxlcU = (PixelC*)((pvopcSrc->pixelsU ()) + nSkipUVPixel );
    PixelC* ppxlcV = (PixelC*)((pvopcSrc->pixelsV ()) + nSkipUVPixel );
    PixelC* pdY = destY;    PixelC* pdU = destU;    PixelC* pdV = destV;
    PixelC* psY;    PixelC* psU;    PixelC* psV;
	
  // convert
    for (y = 0; y < height; y++) {
      psY = ppxlcY;
      for (x = 0; x < width; x++) {
	*pdY = *psY;
	pdY++;
	psY++;
      }
      ppxlcY += Fwidth;
    }
    for (y = 0; y < height/2; y++) {
      psU = ppxlcU;
      for (x = 0; x < width/2; x++) {
	*pdU = *psU;
	pdU++;
	psU++;
      }
      ppxlcU += FwidthUV;
    }
    for (y = 0; y < height/2; y++) {
      psV = ppxlcV;
      for (x = 0; x < width/2; x++) {
	*pdV = *psV;
	pdV++;
	psV++;
      }
      ppxlcV += FwidthUV;
    }
}

Void convertSeg(const CVOPU8YUVBA* pvopcSrc, PixelC* destBY, PixelC* destBUV, Int width, Int height,
		Int left, Int right, Int top, Int bottom)
{
    CoordI x, y;
    Int Fwidth  = pvopcSrc->whereY ().width;
    Int sum, color = 0;

    Int nSkipYPixel  = Fwidth  * EXPANDY_REF_FRAME  + EXPANDY_REF_FRAME;
    PixelC* ppxlcBY  = (PixelC*)((pvopcSrc->pixelsBY ()) + nSkipYPixel );
    PixelC* pdBY = destBY;    PixelC* pdBUV = destBUV;
    PixelC* psBY;

  // convert
    for (y = 0; y < height; y++) {
      psBY = ppxlcBY;
      for (x = 0; x < width; x++) {
	if(left <= x && x < right && top <= y && y < bottom)
	  *pdBY = *psBY;
	else{
	  *pdBY = *psBY = 0; // set zero for out side of VOP rectangle
	}
	if(*pdBY>0) color = *pdBY;
	pdBY++;
	psBY++;
      }
      ppxlcBY += Fwidth;
    }
//    if(color == 0){
//      printf("!!! No object !!!\n");
//      exit(1);
//    }
    for (y = 0; y < height/2; y++) {
      for (x = 0; x < width/2; x++) {
	sum = *(destBY + 2*y*width + 2*x) + *(destBY + (2*y+1)*width + 2*x) +
	      *(destBY + 2*y*width + 2*x+1) + *(destBY + (2*y+1)*width + 2*x+1);
	if(sum>0)
	  *pdBUV = color;
	else
	  *pdBUV = 0;
	pdBUV++;
      }
    }
}

// modified Oct. 1 '97 (by Watanabe and Katata)
/* NBIT: change unsigned char to PixelC
Void pre_pad(unsigned char *mask, unsigned char *curr, int width, int height)
*/
Void pre_pad(PixelC *mask, PixelC *curr, int width, int height)
{
  int		i, j;
  int		flag_cnt = 0;
  int *flag_blk, *mask_blk;
  double *curr_blk;
  double	pad_val = 0.0;
  int		ic, jc;

  double *hori_blk, *vert_blk;
	int *hori_flag_blk, *vert_flag_blk;
  
  flag_blk = (int *)malloc(sizeof(int)*width*height);
  mask_blk = (int *)malloc(sizeof(int)*width*height);
  curr_blk = (double *)malloc(sizeof(double)*width*height);

 // Oct. 1 '97
  hori_blk = (double *)malloc(sizeof(double)*width*height);
  vert_blk = (double *)malloc(sizeof(double)*width*height);
  hori_flag_blk = (int *)malloc(sizeof(int)*width*height);
  vert_flag_blk = (int *)malloc(sizeof(int)*width*height);
  
  
  for(j = 0; j < height; j++)
		for(i = 0; i < width; i++) {
		  mask_blk[j * width + i] = (mask[j * width + i] != 0);
		  if(mask_blk[j * width + i])
				hori_blk[j * width + i] = vert_blk[j * width + i] = curr_blk[j * width + i] = (double) curr[j * width + i];
			else
				hori_blk[j * width + i] = vert_blk[j * width + i] = curr_blk[j * width + i] = 0;
	}
  
  for(j = 0; j < height; j++)
	for(i = 0; i < width; i++)
	  flag_cnt+= (flag_blk[j * width + i] = mask_blk[j * width + i]);
  
  if(flag_cnt == 0)  return;

  
	while(flag_cnt != 0) {

	for(j = 0; j < height; j++)
		for(i = 0; i < width; i++)
			hori_flag_blk[j * width + i] = vert_flag_blk[j * width + i] = flag_blk[j * width + i];

	for(j = 0; j < height; j++)
		for(i = 0; i < width; i++){
		  mask_blk[j * width + i] = flag_blk[j * width + i];
			hori_blk[j * width + i] = vert_blk[j * width + i] = curr_blk[j * width + i];
	}

	for(j = 0; j < height; j++) {/* horizontal scan */
		for(ic = 1; ic < width; ic++)
			if(mask_blk[j * width + ic-1] - mask_blk[j * width + ic] == 1)  break;
		for(i = ic; i < width; i++) {
			if(mask_blk[j * width + i-1] - mask_blk[j * width + i] == 1)
				pad_val = hori_blk[j * width + i-1];
			if(!mask_blk[j * width + i]) {
				hori_blk[j * width + i]+= pad_val;
				hori_flag_blk[j * width + i]++;
			}
	  }
	  for(ic = width-2; ic >= 0; ic--)
			if(mask_blk[j * width + ic+1] - mask_blk[j * width + ic] == 1)  break;
	  for(i = ic; i >= 0; i--) {
			if(mask_blk[j * width + i+1] - mask_blk[j * width + i] == 1)
				pad_val = hori_blk[j * width + i+1];
			if(!mask_blk[j * width + i]) {
				hori_blk[j * width + i]+= pad_val;
				hori_flag_blk[j * width + i]++;
			}
	  }
	}/* end of horizontal scan */

	for(j = 0; j < height; j++)
	  for(i = 0; i < width; i++)
			if(hori_flag_blk[j * width + i] != 0) {
			  hori_blk[j * width + i] = (int)(hori_blk[j * width + i]/(double) hori_flag_blk[j * width + i]); // Oct.1 '97
			  hori_flag_blk[j * width + i]/= hori_flag_blk[j * width + i];
			}

	for(i = 0; i < width; i++) {/* vertical scan */
	  for(jc = 1; jc < height; jc++)
			if(mask_blk[(jc-1) * width + i] - mask_blk[jc * width + i] == 1)  break;
	  for(j = jc; j < height; j++) {
			if(mask_blk[(j-1) * width + i] - mask_blk[j * width + i] == 1)
			  pad_val = vert_blk[(j-1) * width + i];
			if(!mask_blk[j * width + i]) {
				vert_blk[j * width + i]+= pad_val;
				vert_flag_blk[j * width + i]++;
			}
	  }
	  for(jc = height-2; jc >= 0; jc--)
			if(mask_blk[(jc+1) * width + i] - mask_blk[jc * width + i] == 1)  break;
	  for(j = jc; j >= 0; j--) {
			if(mask_blk[(j+1) * width + i] - mask_blk[j * width + i] == 1)
			  pad_val = vert_blk[(j+1) * width + i];
			if(!mask_blk[j * width + i]) {
				vert_blk[j * width + i]+= pad_val;
				vert_flag_blk[j * width + i]++;
			}
	  }
	}/* end of vertical scan */
	
	for(j = 0; j < height; j++)
	  for(i = 0; i < width; i++)
		if(vert_flag_blk[j * width + i] != 0) {
		  vert_blk[j * width + i] = (int)(vert_blk[j * width + i]/(double) vert_flag_blk[j * width + i]); // Oct.1 '97
		  vert_flag_blk[j * width + i]/= vert_flag_blk[j * width + i];
		}

	for(j = 0; j < height; j++)
	  for(i = 0; i < width; i++)
			if(hori_flag_blk[j * width + i] == 1 && vert_flag_blk[j * width + i] == 1)
				curr_blk[j * width + i] = (int)((hori_blk[j * width + i] + vert_blk[j * width + i])/2.0);
			else if (hori_flag_blk[j * width + i] == 1)
				curr_blk[j * width + i] = hori_blk[j * width + i];
			else if (vert_flag_blk[j * width + i] == 1)
				curr_blk[j * width + i] = vert_blk[j * width + i];


	flag_cnt = width * height;
	for(j = 0; j < height; j++)
	  for(i = 0; i < width; i++)
			if(hori_flag_blk[j * width + i] == 1 || vert_flag_blk[j * width + i] == 1){
				flag_blk[j * width + i] = 1;
			  flag_cnt--;
			}

  }/* end of while() */
  
  for(j = 0; j < height; j++)
	for(i = 0; i < width; i++)
	  curr[j * width + i] = (unsigned char) curr_blk[j * width + i];

  free(flag_blk);
  free(mask_blk);
  free(curr_blk);
  free(hori_blk);
  free(vert_blk);
  free(hori_flag_blk);
  free(vert_flag_blk);
}



/* $Id: idwt.cpp,v 1.1 2003/05/05 21:24:00 wmaycisco Exp $ */
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

/* Inverse DWT for MPEG-4 Still Texture Coding 
Original Coded by Shipeng Li, Sarnoff Corporation, Jan. 1998*/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "basic.hpp"
#include "dwt.h"

/* Function:    iDWT()
   Description: Inverse DWT for MPEG-4 Still Texture Coding 
   Input:

   InCoeff -- Input Wavelet Coefficients at CurLevel decomposition
   InMask -- Input  Mask for InCoeff, if ==1, data inside object, 
              otherwise outside.
   Width  -- Width of Original Image (must be multiples of 2^nLevels);
   Height -- Height of Original Image (must be multiples of 2^nLevels);
   CurLevel -- Currect Decomposition Level of the input wavelet coefficients;
   DstLevel -- Destined decomposition level that the wavelet coefficient is 
               synthesized to. DstLevel always less than or equal to CurLevel;
   OutDataType -- OutData Type: 0 - BYTE; 1 -- SHORT
   Filter     -- Filter Used.
   UpdateInput -- Update the level of decomposition to DstLevel  
                  for InCoeff and/or InMask or Not.
                  0 = No  Update; 1 = Update InCoeff; 2: Update InCoeff and InMask;
   FullSizeOut -- 0 = Output image size equals to Width/2^DstLevel x Height/2^DstLevel;
                  1 = Output Image size equals to Width x Height; 
		  ( Highpass band filled with zero after DstLevel)

   Output:
 
   OutData -- Output Image data of Width x Height or Width/2^DstLevel x Height/2^DstLevel
              size depending on FullSizeOut, data type decided by OutDataType;
   OutMask    -- Output mask corresponding to OutData

   Return: DWT_OK if successful.  
*/
Int VTCIDWT:: do_iDWT(DATA *InCoeff, UChar *InMask, Int Width, Int Height, Int CurLevel,
	 Int DstLevel, Int OutDataType, FILTER **Filter,  Void *OutData, 
	 UChar *OutMask, Int UpdateInput, Int FullSizeOut)
{
  switch(Filter[0]->DWT_Type) {
  case DWT_INT_TYPE:
    return(iDWTInt(InCoeff, InMask, Width, Height, CurLevel, DstLevel,
		   OutDataType, Filter, OutData, OutMask, UpdateInput, FullSizeOut));
  case DWT_DBL_TYPE:
    return(iDWTDbl(InCoeff, InMask, Width, Height, CurLevel, DstLevel,
		   OutDataType, Filter, OutData, OutMask, UpdateInput, FullSizeOut));
  default:
    return(DWT_FILTER_UNSUPPORTED);
  }
}


/* Function: AddDCMean 
   Description: Add the Mean of DC coefficients
   Input:
   
   Mask -- Mask for the transformed coeffs
   Width -- Width of the original image
   Height -- Height of the original image
   nLevels -- levels of DWT decomposition performed
   DCMean -- Mean of the DC components

   Input/Output:

   Coeff -- DWT coefficients after nLevels of decomposition
   
   Return: None */

Void VTCIDWT:: AddDCMean(DATA *Coeff, UChar *Mask, Int Width, 
	       Int Height, Int nLevels, Int DCMean) // modified by Sharp (99/2/16)
{
  Int width = (Width >> nLevels), height = (Height >> nLevels);
  Int k;
  Int *a;
  UChar *c;
  DCMean = (DCMean<<nLevels);
  for(k=0; k<Width*height; k+=Width) {
    for(a=Coeff+k,c=Mask+k; a < Coeff+k+width; a++,c++)  {
      if(*c == DWT_IN) *a+=DCMean;
    }
  }
}


// begin: added by Sharp (99/2/16)
Void VTCIDWT::AddDCMeanTile(DATA *Coeff, UChar *Mask, Int Width,
	Int Height, Int nLevels, Int DCMean,
	Int TileWidth, Int TileHeight, Int TileX, Int TileY)
{
  Int width = (TileWidth >> nLevels), height = (TileHeight >> nLevels);
  Int k;
  DATA *a;
  DATA *p;
  //	Int i=0;

  p=Coeff+TileY*TileHeight*Width+TileX*TileWidth;


  DCMean = (DCMean<<nLevels);
  for(k=0; k<Width*height; k+=Width) {
    for(a=p+k; a<p+k+width; a++){
/*       for(a=Coeff+k,c=Mask+k; a < Coeff+k+width; a++,c++)   {*/
      *a += DCMean;
//           if(*c == DWT_IN) *a+=DCMean; //modified by SL@Sarnoff (03/03/99)
/*					 printf("%d\n", i++);*/
/*					}*/
//      *a += DCMean;
    }
  }
}

Int VTCIDWT::do_iDWT_Tile(DATA *InCoeff, UChar *InMask, Int Width, Int Height, Int CurLevel,
	Int DstLevel, Int OutDataType, FILTER **Filter,  Void *OutData,
	UChar *OutMask, Int TileWidth, Int TileHeight,
	Int UpdateInput, Int FullSizeOut, Int orgFlag, Int dcpTile1, Int dcpTile2)
{
  Int width, height;
  //  Int Flag = (orgFlag==0?0:3);

  Int nBits, MaxCoeff, MinCoeff;
  Int i,level, k, j, x, y;
  DATA *tempCoeff;
  Int ret;
  DATA *d_LL, *d_HL, *d_LH, *d_HH, *a;
  DATA *s_LL, *s_HL, *s_LH, *s_HH, *b;
  UChar *c;
  UShort *s;
  UChar *d,*e, *tempMask;
  Int nTilex, nTiley, x1, x2, y1, y2;

  /* Check filter class first */
  if(Filter[0]->DWT_Class != DWT_ODD_SYMMETRIC && Filter[0]->DWT_Class != DWT_EVEN_SYMMETRIC ) {
    return(DWT_FILTER_UNSUPPORTED);
  }
  /* check filter Type */
  if(Filter[0]->DWT_Type!=DWT_INT_TYPE) return(DWT_INTERNAL_ERROR);
  /* Check output coefficient buffer capacity */
  nBits = sizeof(DATA)*8;
  MaxCoeff = (1<<(nBits-1))-1;
  MinCoeff = -(1<<(nBits-1));
  /* Limit nLevels as: 0 - 15*/
  if(DstLevel < 0 || CurLevel >=16 || DstLevel >=16 || DstLevel > CurLevel)
    return(DWT_INVALID_LEVELS);
  /* Check Width and Height, must be multiples of 2^CurLevel */
  if((Width &( (1<<CurLevel)-1))!=0) return(DWT_INVALID_WIDTH);
  if((Height &( (1<<CurLevel)-1))!=0) return(DWT_INVALID_HEIGHT);

  /* calculate size of target area */
  nTilex = (Width+TileWidth-1)/TileWidth;
  nTiley = (Height+TileHeight-1)/TileHeight;
  x1 = dcpTile1%nTilex;
  y1 = dcpTile1/nTilex;
  x2 = dcpTile2%nTilex;
  y2 = dcpTile2/nTilex;
  /*  if ((1&Flag)==1) {*/
  /*    if (x1 != 0) x1--;*/
  /*    if (x2 != (nTilex-1)) x2++;*/
  /*  }*/
  /*  if ((2&Flag)==2) {*/
  /*    if (y1 != 0) y1--;*/
  /*    if (y2 != (nTiley-1)) y2++;*/
  /*  }*/

  width = (x2-x1+1)*TileWidth;
  height = (y2-y1+1)*TileHeight;
  /*   width = Width; */
  /*   height = Height; */

  /* copy mask */
  tempMask = (UChar *)malloc(sizeof(UChar)*width*height);
  if(tempMask==NULL) return(DWT_MEMORY_FAILED);
  /*   memcpy(tempMask, InMask, sizeof(UChar)*Width*Height); */
  memset(tempMask, (UChar)1, width*height);

  /* allocate temp  buffer */
  tempCoeff = (DATA *) malloc(sizeof(DATA)*width*height);
  if(tempCoeff == NULL) {
    free(tempMask);
    return(DWT_MEMORY_FAILED);
  }
  memset(tempCoeff, (UChar)0, width*height*sizeof(DATA));

  /* copy DC band */
  for(y=y1;y<=y2;y++){
    for(x=x1;x<=x2;x++){
      d_LL = tempCoeff+(y-y1)*(TileHeight>>CurLevel)*width+(x-x1)*(TileWidth>>CurLevel);
      /*       d_LL = tempCoeff+y*(TileHeight>>CurLevel)*width+x*(TileWidth>>CurLevel); */
      s_LL = InCoeff+y*TileHeight*Width+x*TileWidth;
      for(i=0;i<(TileHeight>>CurLevel);i++){
        for(j=0;j<(TileWidth>>CurLevel);j++){
          *(d_LL+j) = *(s_LL+j);
        }
        d_LL+= width;
        s_LL+= Width;
      }
    }
  }

  /* copy AC bands */
  for(level=CurLevel;level>DstLevel;level--){
    for(y=y1;y<=y2;y++){
      for(x=x1;x<=x2;x++){
        d_HL=tempCoeff+(width>>level)+(y-y1)*(TileHeight>>level)*width+(x-x1)*(TileWidth>>level);
        d_LH=tempCoeff+(height>>level)*width+(y-y1)*(TileHeight>>level)*width+(x-x1)*(TileWidth>>level);
        /*    d_HL=tempCoeff+(width>>level)+y*(TileHeight>>level)*width+x*(TileWidth>>level);  */
        /*    d_LH=tempCoeff+(height>>level)*width+y*(TileHeight>>level)*width+x*(TileWidth>>level
            ); */
        /*  d_HH=tempCoeff+(width>>level)+(height>>level)*width+(y-y1)*(TileHeight>>level)*width+



                                                      (x-x1)*(Tile
            Width>>level); */
        d_HH =d_HL+(height>>level)*width;

        s_HL=InCoeff+y*TileHeight*Width+x*TileWidth+(TileWidth>>level);   /* HL */
        s_LH=InCoeff+(y*TileHeight+(TileHeight>>level))*Width+x*TileWidth;  /* LH */
        s_HH=s_HL+(TileHeight>>level)*Width;          /* HH */

        for(i=0;i<(TileHeight>>level);i++){
          for(j=0;j<(TileWidth>>level);j++){

            /*      if( d_HL+j > tempCoeff+width*height || d_LH+j > tempCoeff+width*h
                eight || d_HH+j > tempCoeff+width*height ){ */
            /*        printf("level:%d, y:%d, x:%d, i:%d, j:%d\n", level, y, x, i, j)
                ; */
            /*        exit(1); */
            /*      } */
            *(d_LH+j) =  *(s_LH+j);
            *(d_HL+j) =  *(s_HL+j);
            *(d_HH+j) =  *(s_HH+j);
          }
          d_HL+=width;
          d_LH+=width;
          d_HH+=width;
          s_HL+=Width;
          s_LH+=Width;
          s_HH+=Width;
        }
      }
    }
  }


  /* Perform the  iDWT */
  for(level=CurLevel;level>DstLevel;level--) {
    /* Synthesize one level */

    ret=SynthesizeOneLevelInt(tempCoeff, tempMask, width, height,
        level, Filter[level], MaxCoeff, MinCoeff, DWT_NONZERO_HIGH);

    if(ret!=DWT_OK) {
      free(tempCoeff);
      free(tempMask);
      return(ret);
    }
  }
  /* check to see if required to update InCoeff and/or InMask */
  if(UpdateInput>0) {
    /* update InCoeff */
    for(k=0;k<Width*(Height>>DstLevel); k+=Width) {
      for(b=InCoeff+k,a=tempCoeff+k;b<InCoeff+k+(Width>>DstLevel);b++,a++) {

        *b = (DATA) *a;

      }
    }
  }
  if(UpdateInput>1) {
    /* Update InMask */
    for(k=0;k<Width*(Height>>DstLevel); k+=Width) {
      for(d=InMask+k,e=tempMask+k;d<InMask+k+(Width>>DstLevel);d++,e++) {
        *d = *e;
      }
    }
  }

  if(FullSizeOut) {
    /* Perform the rest of iDWT till fullsize */
    for(level=DstLevel;level>0;level--) {
      /* Synthesize one level */

      ret=SynthesizeOneLevelInt(tempCoeff, tempMask, Width, Height,
          level, Filter[level], MaxCoeff, MinCoeff, DWT_ZERO_HIGH);

      if(ret!=DWT_OK) {
        free(tempCoeff);
        free(tempMask);
        return(ret);
      }
    }
  }

  /* copy the output to OutData and OutMask */
  if(FullSizeOut) level=0;
  else level=DstLevel;

  for(i=0,k=0;k<width*(height >> level);k+=width,i++) {
    if(OutDataType==0) { /* Unsigned CHAR */
      c = (UChar *)OutData;
      c = c+y1*TileHeight*Width+x1*TileWidth;
      c+=i*Width;
      for(a=tempCoeff+k;a<tempCoeff+k+(width>>level);c++,a++) {
        Int iTmp;

        iTmp = (Int) *a;
        iTmp = level>0?((iTmp+(1<<(level-1))) >> level):iTmp; /* scale and round */

        iTmp = (iTmp >0) ? ((iTmp > 255)?255:iTmp):
        0; /* clipping */
        *c = (UChar) iTmp;
      }
    }
    else { /* UShort */
      s = (UShort *)OutData;
      s+=i;
      for(a=tempCoeff+k;a<tempCoeff+k+(width>>level);s++,a++) {
        Int iTmp;

        iTmp = (Int)*a;
        iTmp = level>0?((iTmp+(1<<(level-1))) >> level):iTmp; /* scale and round */

        iTmp = (iTmp >0) ? ((iTmp > 65535)?65535:iTmp):
        0; /* clipping */
        *s = (UShort) iTmp;
      }
    }
    d = OutMask + i;
    for(e=tempMask+k;e<tempMask+k+(width>>level);e++,d++)  *d=*e;
  }
  free(tempCoeff);
  free(tempMask);
  return(DWT_OK);
}
// begin: added by Sharp (99/2/16)


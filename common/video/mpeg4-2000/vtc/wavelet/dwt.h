/* $Id: dwt.h,v 1.1 2003/05/05 21:23:57 wmaycisco Exp $ */
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

/* DWT and inverse DWT header file 
   Created by Shipeng Li, Sarnoff Corporation, Jan. 1998
   Copyright (c) Sarnoff Corporation
*/
#ifndef _DWT_H_
#define _DWT_H_
#define UChar UChar

// hjlee 0901
#ifdef DATA
#undef DATA
#endif
#define DATA Int  

 
#ifdef INT
#undef INT
#endif
#define INT Int

#define ROUNDDIV(x, y) ((x)>0?(Int)(((Int)(x)+((y)>>1))/(y)):(Int)(((Int)(x)-((y)>>1)) /(y)))
enum {  /* DWT or IDWT return values */
  DWT_OK,
  DWT_FILTER_UNSUPPORTED,
  DWT_MEMORY_FAILED,
  DWT_COEFF_OVERFLOW,
  DWT_INVALID_LEVELS,
  DWT_INVALID_WIDTH,
  DWT_INVALID_HEIGHT,
  DWT_INTERNAL_ERROR,
  DWT_NOVALID_PIXEL
};

#ifdef RECTANGULAR
#undef RECTANGULAR
#endif
#define RECTANGULAR -1
/* Image data type */
enum { 
  DWT_UCHAR_ENUM,
  DWT_USHORT_ENUM
};
enum { /* filter class */
  DWT_ODD_SYMMETRIC,
  DWT_EVEN_SYMMETRIC,
  DWT_ORTHORGONAL
};

enum {
  DWT_EVEN,
  DWT_ODD
};

enum {
  DWT_HORIZONTAL,
  DWT_VERTICAL
};

enum {
  DWT_NONZERO_HIGH,
  DWT_ZERO_HIGH,
  DWT_ALL_ZERO
};

#define  DWT_OUT0 0
#define  DWT_IN   1
#define  DWT_OUT1 2
#define  DWT_OUT2 3
#define  DWT_OUT3 4

enum { /* filter type */
  DWT_INT_TYPE,
  DWT_DBL_TYPE
};

typedef struct {
  Int DWT_Class; 
  /* 0: Odd Symmetric 1: Even Symmetric 2: Orthogonal (not supported)
   Note: This is not defined by MPEG4-CD syntax but should be, since different 
   wavelets corresponds to different extension. 
   Ref. "Shape Adpative Discrete Wavelet Transform for Arbitrarily-Shaped 
   Visual Object Coding" S. Li and W. Li (submitted to IEEE T-CSVT) 
   */
  Int DWT_Type;
  /* 0: Short Coeff; 1: Double Coeff */
  Int HPLength;
  Int LPLength;
  Void *HPCoeff;
  Void *LPCoeff;
  Int Scale;
} FILTER;

#ifdef __cplusplus
#define DWT_S VTCDWT::
#define IDWT_S VTCIDWT::
#define DWTMASK_S VTCDWTMASK::
#define IMAGEBOX_S VTCIMAGEBOX::
class VTCDWT {
public:
  /* Wavelet Decomposition function used by encoder */


  // hjlee 0901
  INT do_DWT(Void *InData, UChar *InMask, Int Width, Int Height, Int nLevels,
	 Int InDataType, FILTER **Filter,  DATA *OutCoeff, UChar *OutMask);

	 
	 /* Remove DC Mean */
  INT RemoveDCMean(Int *Coeff, UChar *Mask, INT Width, INT Height, INT nLevels);

private:

Int DecomposeOneLevelDbl(double *OutCoeff, UChar *OutMask, Int Width,
			 Int Height, Int level, FILTER *Filter);
Int DecomposeOneLevelInt(Int *OutCoeff, UChar *OutMask, Int Width,
			 Int Height, Int level, FILTER *Filter,
			 Int MaxCoeff, Int MinCoeff);
Int SADWT1dInt(Int *InBuf, UChar *InMaskBuf, Int *OutBuf, 
		      UChar *OutMaskBuf, Int Length, FILTER *Filter, 
		      Int Direction);
Int SADWT1dOddSymInt(Int *InBuf, UChar *InMaskBuf, Int *OutBuf, 
			    UChar *OutMaskBuf, Int Length, FILTER *Filter, 
			    Int Direction);
Int DecomposeSegmentOddSymInt(Int *In, Int *OutL, Int *OutH, 
				     Int PosFlag, Int Length, FILTER *Filter);
Int SADWT1dEvenSymInt(Int *InBuf, UChar *InMaskBuf, Int *OutBuf, 
			     UChar *OutMaskBuf, Int Length, FILTER *Filter, 
			     Int Direction);
Int DecomposeSegmentEvenSymInt(Int *In, Int *OutL, Int *OutH, 
				      Int PosFlag, Int Length, FILTER *Filter);
Int SADWT1dDbl(double *InBuf, UChar *InMaskBuf, double *OutBuf, 
		      UChar *OutMaskBuf, Int Length, FILTER *Filter, 
		      Int Direction);
Int SADWT1dOddSymDbl(double *InBuf, UChar *InMaskBuf, double *OutBuf, 
			    UChar *OutMaskBuf, Int Length, FILTER *Filter, 
			    Int Direction);
Int DecomposeSegmentOddSymDbl(double *In, double *OutL, double *OutH, 
				     Int PosFlag, Int Length, FILTER *Filter);
Int SADWT1dEvenSymDbl(double *InBuf, UChar *InMaskBuf, double *OutBuf, 
			     UChar *OutMaskBuf, Int Length, FILTER *Filter, 
			     Int Direction);
Int DecomposeSegmentEvenSymDbl(double *In, double *OutL, double *OutH, 
				      Int PosFlag, Int Length, FILTER *Filter);
};

class VTCIDWT {
public:
/* Wavelet Synthesis function used by decoder */
// hjlee 0901
INT do_iDWT(DATA *InCoeff, UChar *InMask, Int Width, Int Height, Int CurLevel,
	 Int DstLevel, Int OutDataType, FILTER **Filter,  Void *OutData, 
	 UChar *OutMask, Int UpdateInput, Int FullSizeOut);

	 /* Addback DC Mean */
Void AddDCMean(Int *Coeff, UChar *Mask, INT Width, 
	       INT Height, INT nLevels, INT DCMean);
// begin : added by Sharp (99/2/16)
  Void AddDCMeanTile(DATA *Coeff, UChar *Mask, Int Width,
    Int Height, Int nLevels, Int DCMean,
    Int TileWidth, Int TileHeight, Int TileX, Int TileY);
  Int do_iDWT_Tile(DATA *InCoeff, UChar *InMask, Int Width, Int Height, Int CurLevel,
    Int DstLevel, Int OutDataType, FILTER **Filter,  Void *OutData,
    UChar *OutMask, Int TileWidth, Int TileHeight,
    Int UpdateInput, Int FullSizeOut, Int orgFlag, Int dcpTile1, Int dcpTile2);
// end : added by Sharp (99/2/16)

private:
//hjlee 0901
  Int iDWTInt(Int *InCoeff, UChar *InMask, Int Width, Int Height, Int CurLevel,
	 Int DstLevel, Int OutDataType, FILTER **Filter,  Void *OutData, 
	 UChar *OutMask, Int UpdateInput, Int FullSizeOut);
  Int iDWTDbl(Int *InCoeff, UChar *InMask, Int Width, Int Height, Int CurLevel,
	    Int DstLevel, Int OutDataType, FILTER **Filter,  Void *OutData, 
	 UChar *OutMask, Int UpdateInput, Int FullSizeOut);

 Int SynthesizeOneLevelInt(Int *OutCoeff, UChar *OutMask, Int Width,
				Int Height, Int level, FILTER *Filter,
				Int MaxCoeff, Int MinCoeff, Int ZeroHigh);
 Int iSADWT1dInt(Int *InBuf, UChar *InMaskBuf, Int *OutBuf, 
		       UChar *OutMaskBuf, Int Length, FILTER *Filter, 
		       Int Direction, Int ZeroHigh);
 Int iSADWT1dOddSymInt(Int *InBuf, UChar *InMaskBuf, Int *OutBuf, 
			    UChar *OutMaskBuf, Int Length, FILTER *Filter, 
			    Int Direction, Int ZeroHigh);
 Int SynthesizeSegmentOddSymInt(Int *Out, Int *InL, Int *InH, 
				     Int PosFlag, Int Length, FILTER *Filter, Int ZeroHigh);
 Int iSADWT1dEvenSymInt(Int *InBuf, UChar *InMaskBuf, Int *OutBuf, 
			    UChar *OutMaskBuf, Int Length, FILTER *Filter, 
			    Int Direction, Int ZeroHigh);
 Int SynthesizeSegmentEvenSymInt(Int *Out, Int *InL, Int *InH, 
				       Int PosFlag, Int Length, FILTER *Filter, Int ZeroHigh);

 Int SynthesizeOneLevelDbl(double *OutCoeff, UChar *OutMask, Int Width,
				Int Height, Int level, FILTER *Filter, Int ZeroHigh);
 Int iSADWT1dDbl(double *InBuf, UChar *InMaskBuf, double *OutBuf, 
		       UChar *OutMaskBuf, Int Length, FILTER *Filter, 
		       Int Direction, Int ZeroHigh);
 Int iSADWT1dOddSymDbl(double *InBuf, UChar *InMaskBuf, double *OutBuf, 
			    UChar *OutMaskBuf, Int Length, FILTER *Filter, 
			    Int Direction, Int ZeroHigh);
 Int SynthesizeSegmentOddSymDbl(double *Out, double *InL, double *InH, 
				      Int PosFlag, Int Length, FILTER *Filter, Int ZeroHigh);
 Int iSADWT1dEvenSymDbl(double *InBuf, UChar *InMaskBuf, double *OutBuf, 
			      UChar *OutMaskBuf, Int Length, FILTER *Filter, 
			      Int Direction, Int ZeroHigh);
 Int SynthesizeSegmentEvenSymDbl(double *Out, double *InL, double *InH, 
				     Int PosFlag, Int Length, FILTER *Filter, Int ZeroHigh);

};

class VTCDWTMASK {
public:
/* Mask Decomposition function used by Decoder */
Int do_DWTMask(UChar *InMask, UChar *OutMask, Int Width, Int Height, Int nLevels,
	    FILTER **Filter);  // hjlee 0901

private:
  Int DecomposeMaskOneLevel(UChar *OutMask, Int Wdith,
				Int Height, Int level, FILTER *Filter);
 Int SADWTMask1d(UChar *InMaskBuf, UChar *OutMaskBuf, Int Length, 
		FILTER *Filter, Int Direction);
 Int SADWTMask1dOddSym( UChar *InMaskBuf, UChar *OutMaskBuf, 
			      Int Length, FILTER *Filter, Int Direction);
 Int SADWTMask1dEvenSym( UChar *InMaskBuf,  UChar *OutMaskBuf, 
			       Int Length, FILTER *Filter, Int Direction);
};

class VTCIDWTMASK {
public:
Int do_iDWTMask(UChar *InMask, UChar *OutMask, Int Width, Int Height, Int CurLevel,
	 Int DstLevel, FILTER **Filter,  Int UpdateInput, Int FullSizeOut);
Int SynthesizeMaskHalfLevel(UChar *OutMask, Int Width,
				  Int Height, Int level, FILTER *Filter, 
				  Int ZeroHigh, Int Direction);
private:
Int SynthesizeMaskOneLevel(UChar *OutMask, Int Width,
				Int Height, Int level, FILTER *Filter, Int zeroHigh);
Int iSADWTMask1d(UChar *InMaskBuf,  
		       UChar *OutMaskBuf, Int Length, FILTER *Filter, 
		       Int Direction);
Int iSADWTMask1dOddSym(UChar *InMaskBuf,
			    UChar *OutMaskBuf, Int Length, FILTER *Filter, 
			    Int Direction);
Int iSADWTMask1dEvenSym(UChar *InMaskBuf,
			    UChar *OutMaskBuf, Int Length, FILTER *Filter, 
			    Int Direction);
};

class VTCIMAGEBOX {
public:

// hjlee 0901
Int GetBox(Void *InImage, Void **OutImage, 
	   Int RealWidth, Int RealHeight, 
	   Int VirtualWidth, Int VirtualHeight, 
	   Int OriginX, Int OriginY, Int DataType);
	   
INT GetMaskBox(UChar *InMask,  UChar **OutMask, 
	       INT RealWidth, INT RealHeight, 
	       INT Nx, INT Ny,
	       INT *VirtualWidth, INT *VirtualHeight, 
	       INT *OriginX, INT *OriginY,  INT Shape, INT nLevels);

// FPDAM begin: added by Sharp
INT GetRealMaskBox(UChar *InMask,  UChar **OutMask, 
	       INT RealWidth, INT RealHeight, 
	       INT Nx, INT Ny,
	       INT *VirtualWidth, INT *VirtualHeight, 
	       INT *OriginX, INT *OriginY,  INT Shape, INT nLevels);
// FPDAM end: added by Sharp

INT PutBox(Void *InImage, UChar *InMask, Void *OutImage, UChar *OutMask, 
	   INT RealWidth, INT RealHeight, 
	   INT VirtualWidth, INT VirtualHeight, 
	   INT OriginX, INT OriginY, INT DataType, INT Shape, Int OutValue);

Int ExtendImageSize(Int InWidth, Int InHeight, 
		  Int Nx, Int Ny,
		  Int *OutWidth, Int *OutHeight, 
		  Int nLevels);

// FPDAM begin: added by Sharp
Int CheckTextureTileType (UChar *mask, Int width, 
	Int height, Int real_width, Int real_height);
// FPDAM end: added by Sharp

// hjlee 0901
Void  SubsampleMask(UChar *InMask, UChar **OutMask, 
		    Int Width, Int Height,
		    FILTER *filter);
			
INT ExtendMaskBox(UChar *InMask,  UChar **OutMask, 
		  INT InWidth, INT InHeight, 
		  INT Nx, INT Ny,
		  INT *OutWidth, INT *OutHeight, 
		  INT nLevels);
// private: // deleted by Sharp (99/5/10)
Int LCM(Int x, Int y);
Int GCD(Int x, Int y);
};

#endif /* __cplusplus  */ 

#endif /*_DWT_H_ */

/*************************************************************************

This software module was originally developed by 

	Ming-Chieh Lee (mingcl@microsoft.com), Microsoft Corporation
	Wei-ge Chen (wchen@microsoft.com), Microsoft Corporation
	Bruce Lin (blin@microsoft.com), Microsoft Corporation
	Chuang Gu (chuanggu@microsoft.com), Microsoft Corporation
	(date: March, 1996)

in the course of development of the MPEG-4 Video (ISO/IEC 14496-2). 
This software module is an implementation of a part of one or more MPEG-4 Video tools 
as specified by the MPEG-4 Video. 
ISO/IEC gives users of the MPEG-4 Video free license to this software module or modifications 
thereof for use in hardware or software products claiming conformance to the MPEG-4 Video. 
Those intending to use this software module in hardware or software products are advised that its use may infringe existing patents. 
The original developer of this software module and his/her company, 
the subsequent editors and their companies, 
and ISO/IEC have no liability for use of this software module or modifications thereof in an implementation. 
Copyright is not released for non MPEG-4 Video conforming products. 
Microsoft retains full right to use the code for his/her own purpose, 
assign or donate the code to a third party and to inhibit third parties from using the code for non MPEG-4 Video conforming products. 
This copyright notice must be included in all copies or derivative works. 

Copyright (c) 1996, 1997.


Module Name:

    typeapi.h

Abstract:

    Glue for TYPE called from elsewhere

Revision History:
	
NOTE: CALLER OWNS ALL THE RETURNED POINTERS!!

*************************************************************************/

#ifndef __TYPEAPI_H__
#define __TYPEAPI_H__

#include <stdio.h>
#include <stdlib.h>
#include "header.h"
#include "basic.hpp"
#include "geom.hpp"
#include "transf.hpp"
#include "warp.hpp"
#include "vop.hpp"
#include "grayf.hpp"
#include "grayi.hpp"
#include "yuvai.hpp"
#include "grayc.hpp"
#include "yuvac.hpp"

// handy functions
API Int nint (Double x); // returns nearest integer to x, analog of FORTRAN NINT
// NBIT: add checkrange for U16
API U16 checkrange (U16 x, U16 cMin, U16 cMax);	// returns cMin if x < cMin, cMax if x > cMax, otherwise x
API Int checkrange (Int x, Int cMin, Int cMax);	// returns cMin if x < cMin, cMax if x > cMax, otherwise x
API Long checkrange (Long x, Long cMin, Long cMax);	// returns cMin if x < cMin, cMax if x > cMax, otherwise x

#ifndef __DOUBLE_PRECISION_
API Double checkrange (Double x, Double cMin, Double cMax);
#endif // __DOUBLE_PRECISION_

API Float checkrange (Float x, Float cMin, Float cMax);
API U8 checkrangeU8 (U8 x, U8 cMin, U8 cMax);	// returns cMin if x < cMin, cMax if x > cMax, otherwise x

// handy numerical functions
API Int sad (
	const CIntImage& fi0, // first float image
	const CIntImage& fi1, // second float image
	const CIntImage& fiMsk // float image indicating the transparency information
);
API Void mse (const CVideoObjectPlane& vop1, const CVideoObjectPlane& vop2, Double dmse [3]);
API Void snr (const CVideoObjectPlane& vop1, const CVideoObjectPlane& vop2, Double dsnr [3]);

// CRct
API CRct rctFromIndex (UInt indexX, UInt indexY, const CRct& rct, UInt size); // starting from 0
API CRct rctDivide (const CRct& rctBlk, const CRct& rctVOP, UInt rate); // divide the rctBlk by rate, with refererence to rvtVOP

// for MB CVOPIntYUVBA
API Void getBlockDataFromMB ( // will create new objects for pfiBlk and pfiA, the caller then owns these two.
	const CVOPIntYUVBA* pvopfMB, // MacroBlock VOPF data
	CIntImage*& pfiBlk, // texture of the block
	CIntImage*& pfiB, // bainry mask of the block 
	Int blkNum // block number
);

API Void getTextureDataFromMB ( // will create new objects for pfiBlk, the caller then owns these two.
	const CVOPIntYUVBA* pvopfMB, // MacroBlock VOPF data
	CIntImage*& pfiBlk, // texture of the block
	Int blkNum // block number
);

API Void getBinaryDataFromMB ( // will create new objects for pfiA, the caller then owns these two.
	const CVOPIntYUVBA* pvopfMB, // MacroBlock VOPF data
	CIntImage*& pfiY, // alpha of the block
	Int blkNum // block number
);

API Void getAlphaYDataFromMB (const CVOPIntYUVBA* pvopfMB, CIntImage*& pfiA, Int blkNum);

// expand
API CRct fitToMulOfSize (const CRct& rctOrg, UInt size);
API own CIntImage* fiFitToMulOfSize (const CIntImage* pfi, UInt size, const CSite* pstLeftTop = NULL);
API own CVOPIntYUVBA* vopfFitToMulOfMBSize (const CVOPIntYUVBA* pvopf, const CSite* pstLeftTop = NULL);

// load data from MPEG4 file format
API own CIntImage* alphaFromCompFile (
	const Char* pchSeg, // file name
	UInt ifr, // frame number 
	UInt iobj, // object number 
	const CRct& rct, // rect
	UInt nszHeader = 0 // number of bytes to skip
);

#ifdef __NBIT_
void pxlcmemset(PixelC *ppxlc, PixelC pxlcVal, Int iCount);
#else
#define pxlcmemset(ppxlc, pxlcVal, iCount)  memset(ppxlc, pxlcVal, iCount)
#endif

Int divroundnearest(Int i, Int iDenom);
Void dumpNonCodedFrame(FILE* pfYUV, FILE* pfSeg, FILE **ppfAux, Int iAuxCompCount, CRct& rct, UInt nBits);
Void fatal_error(char *pchMessage, Int iCont = 0);

// for unix compability
#ifndef SEEK_CUR
#define SEEK_CUR    1
#endif
#ifndef SEEK_END
#define SEEK_END    2
#endif
#ifndef SEEK_SET
#define SEEK_SET    0
#endif

#endif // __TYPEAPI_H__






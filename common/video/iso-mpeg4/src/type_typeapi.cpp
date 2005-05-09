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

    typeapi.cpp

Abstract:

    Glue for TYPE called from elsewhere

Revision History:
	
NOTE: CALLER OWNS ALL THE RETURNED POINTERS!!

*************************************************************************/

#include "typeapi.h"
#include <stdlib.h>
#include <math.h>

#ifdef __MFC_
#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

#define new DEBUG_NEW				   
#endif // __MFC_

//	handy functions
Int nint (Double x)  // returns nearest integer to x, analog of FORTRAN NINT
{
	return x > 0 ? (Int) (x + .5) : (Int) (x - .5);
}

#ifndef __DOUBLE_PRECISION_
Double checkrange (Double x, Double cMin, Double cMax)	// returns cMin if x < cMin, cMax if x > cMax, otherwise x
{
	if (x < cMin)	return cMin;
	if (x > cMax)	return cMax;
	return x;
}
#endif // __DOUBLE_PRECISION_

U8 checkrangeU8 (U8 x, U8 cMin, U8 cMax)	// returns cMin if x < cMin, cMax if x > cMax, otherwise x
{
	if (x < cMin)	return cMin;
	if (x > cMax)	return cMax;
	return x;
}

// NBIT: add checkrange for U16
U16 checkrange (U16 x, U16 cMin, U16 cMax)	// returns cMin if x < cMin, cMax if x > cMax, otherwise x
{
	if (x < cMin)	return cMin;
	if (x > cMax)	return cMax;
	return x;
}

Int checkrange (Int x, Int cMin, Int cMax)	// returns cMin if x < cMin, cMax if x > cMax, otherwise x
{
	if (x < cMin)	return cMin;
	if (x > cMax)	return cMax;
	return x;
}

Long checkrange (Long x, Long cMin, Long cMax)	// returns cMin if x < cMin, cMax if x > cMax, otherwise x
{
	if (x < cMin)	return cMin;
	if (x > cMax)	return cMax;
	return x;
}

Float checkrange (Float x, Float cMin, Float cMax)	// returns cMin if x < cMin, cMax if x > cMax, otherwise x
{
	if (x < cMin)	return cMin;
	if (x > cMax)	return cMax;
	return x;
}

Double dist (Double x0, Double y0, Double x1, Double y1)	// returns distance between (x0,y0) and (x1,y1)
{
	Double dx = x0 - x1;
	Double dy = y0 - y1;
	return sqrt (dx * dx + dy * dy);
}

// handy numerical functions
Int sad (const CIntImage& fi0, const CIntImage& fi1, const CIntImage& fiMsk)
{
	assert (fi0.where () == fi1.where ());
	const UInt area = fi0.where ().area ();
	const PixelI* ppxlf0 = fi0.pixels ();
	const PixelI* ppxlf1 = fi1.pixels ();
	const PixelI* ppxlfMsk = fiMsk.pixels ();
	Int sadRet = 0;
	for (UInt ip = 0; ip < area; ip++, ppxlf0++, ppxlf1++, ppxlfMsk++) {
		if (*ppxlfMsk != (PixelI) transpValue)
			sadRet += abs (*ppxlf0 - *ppxlf1);
	}
	return sadRet;
}

Void mse (const CVideoObjectPlane& vop1, const CVideoObjectPlane& vop2, Double dmse [3])
{
	assert (vop1.where () == vop2.where ());
	Int sqrR = 0, sqrG = 0, sqrB = 0;
	const CPixel* ppxlVopThis = vop1.pixels ();
	const CPixel* ppxlVopCompare = vop2.pixels ();
	UInt area = vop1.where ().area ();
	for (UInt i = 0; i < area; i++) {
		Int diffR = ppxlVopThis -> pxlU.rgb.r - ppxlVopCompare -> pxlU.rgb.r;
		Int diffG = ppxlVopThis -> pxlU.rgb.g - ppxlVopCompare -> pxlU.rgb.g;
		Int diffB = ppxlVopThis -> pxlU.rgb.b - ppxlVopCompare -> pxlU.rgb.b;
		sqrR += diffR * diffR;
		sqrG += diffG * diffG;
		sqrB += diffB * diffB;
		ppxlVopThis++;
		ppxlVopCompare++;
	}
	dmse [0] = (Double) sqrR / (Double) area;	 
	dmse [1] = (Double) sqrG / (Double) area;
	dmse [2] = (Double) sqrB / (Double) area;
}

Void snr (const CVideoObjectPlane& vop1, const CVideoObjectPlane& vop2, Double dsnr [3])
{
	Double msError [3];

	mse (vop1, vop2, msError);
	for (UInt i = 0; i < 3; i++) {
		if (msError [i] == 0.0)
			dsnr [i] = 1000.0;
		else 
			dsnr [i] = (log10 (255 * 255 / msError [i]) * 10.0);
	}
}

// CRct
CRct rctFromIndex (UInt indexX, UInt indexY, const CRct& rct, UInt size) // starting from 0
{
	assert (rct.width % size == 0 &&	rct.height () % size == 0);
	CoordI left = rct.left + indexX * size;
	CoordI right = left + size;
	CoordI top = rct.top + indexY * size;
	CoordI bottom = top + size;
	return CRct (left, top, right, bottom); 
}

CRct rctDivide (const CRct& rctBlk, const CRct& rctVOP, UInt rate) // divide the rctBlk by rate, with refererence to rvtVOP
{
	assert (rctBlk <= rctVOP);
	CoordI left = (rctBlk.left - rctVOP.left) / rate + rctVOP.left;
	CoordI top = (rctBlk.top - rctVOP.top) / rate + rctVOP.top;
	CoordI right = left + rctBlk.width / rate;
	CoordI bottom = top + rctBlk.height () / rate;
	return CRct (left, top, right, bottom);
}

// CVOPIntYUVBA ----------  probably unused (SB)
Void getBlockDataFromMB (const CVOPIntYUVBA* pvopfMB, CIntImage*& pfiBlk, CIntImage*& pfiB, Int blkNum)
{	
	if (blkNum == ALL_Y_BLOCKS) {
		pfiB = new CIntImage (*pvopfMB -> getPlane (BY_PLANE));
		pfiBlk = new CIntImage (*pvopfMB -> getPlane (Y_PLANE));
			
	}
	else if (blkNum == ALL_A_BLOCKS) {
		pfiB = new CIntImage (*pvopfMB -> getPlane (BY_PLANE));
		pfiBlk = new CIntImage (*pvopfMB -> getPlaneA (0));
			
	}
	else if (blkNum == U_BLOCK || blkNum == V_BLOCK) {
		pfiB = new CIntImage (*pvopfMB -> getPlane (BUV_PLANE));
		pfiBlk = 
			(blkNum == U_BLOCK) ? new CIntImage (*pvopfMB -> getPlane (U_PLANE)) :
			new CIntImage (*pvopfMB -> getPlane (V_PLANE));
	}
	else if ((Int) blkNum >= (Int) Y_BLOCK1 && (Int) blkNum <= (Int) Y_BLOCK4)  {
		UInt idx = (blkNum == Y_BLOCK1 || blkNum == Y_BLOCK3) ? 0 : 1;
		UInt idy = (blkNum == Y_BLOCK1 || blkNum == Y_BLOCK2) ? 0 : 1;
		CRct rct = pvopfMB -> whereY ();
		CoordI left = rct.left + idx * BLOCK_SIZE;
		CoordI top = rct.top + idy * BLOCK_SIZE;
		CRct rctF (left, top, left + BLOCK_SIZE, top + BLOCK_SIZE);
		pfiBlk = new CIntImage (*pvopfMB -> getPlane (Y_PLANE), rctF);
		pfiB = new CIntImage (*pvopfMB -> getPlane (BY_PLANE), rctF);
	}
	else { // alpha plane
		assert (pvopfMB -> fAUsage () == EIGHT_BIT);
		UInt idx = (blkNum == A_BLOCK1 || blkNum == A_BLOCK3) ? 0 : 1;
		UInt idy = (blkNum == A_BLOCK1 || blkNum == A_BLOCK2) ? 0 : 1;
		CRct rct = pvopfMB -> whereY ();
		CoordI left = rct.left + idx * BLOCK_SIZE;
		CoordI top = rct.top + idy * BLOCK_SIZE;
		CRct rctF (left, top, left + BLOCK_SIZE, top + BLOCK_SIZE);
		pfiBlk = new CIntImage (*pvopfMB -> getPlaneA (0), rctF);
		pfiB = new CIntImage (*pvopfMB -> getPlane (BY_PLANE), rctF);
	}
}

Void getTextureDataFromMB (const CVOPIntYUVBA* pvopfMB, CIntImage*& pfiBlk, Int blkNum)
{	
	if (blkNum == ALL_Y_BLOCKS) {
		pfiBlk = new CIntImage (*pvopfMB -> getPlane (Y_PLANE));
			
	}
	else if (blkNum == ALL_A_BLOCKS) {
		pfiBlk = new CIntImage (*pvopfMB -> getPlaneA (0));
			
	}
	else if (blkNum == U_BLOCK || blkNum == V_BLOCK) {
		pfiBlk = 
			(blkNum == U_BLOCK) ? new CIntImage (*pvopfMB -> getPlane (U_PLANE)) :
			new CIntImage (*pvopfMB -> getPlane (V_PLANE));
	}
	else if ((Int) blkNum >= (Int) Y_BLOCK1 && (Int) blkNum <= (Int) Y_BLOCK4)  {
		UInt idx = (blkNum == Y_BLOCK1 || blkNum == Y_BLOCK3) ? 0 : 1;
		UInt idy = (blkNum == Y_BLOCK1 || blkNum == Y_BLOCK2) ? 0 : 1;
		CRct rct = pvopfMB -> whereY ();
		CoordI left = rct.left + idx * BLOCK_SIZE;
		CoordI top = rct.top + idy * BLOCK_SIZE;
		CRct rctF (left, top, left + BLOCK_SIZE, top + BLOCK_SIZE);
		pfiBlk = new CIntImage (*pvopfMB -> getPlane (Y_PLANE), rctF);
	}
	else { // alpha plane
		assert (pvopfMB -> fAUsage () == EIGHT_BIT);
		UInt idx = (blkNum == A_BLOCK1 || blkNum == A_BLOCK3) ? 0 : 1;
		UInt idy = (blkNum == A_BLOCK1 || blkNum == A_BLOCK2) ? 0 : 1;
		CRct rct = pvopfMB -> whereY ();
		CoordI left = rct.left + idx * BLOCK_SIZE;
		CoordI top = rct.top + idy * BLOCK_SIZE;
		CRct rctF (left, top, left + BLOCK_SIZE, top + BLOCK_SIZE);
		pfiBlk = new CIntImage (*pvopfMB -> getPlaneA (0), rctF);
	}
}

Void getBinaryDataFromMB (const CVOPIntYUVBA* pvopfMB, CIntImage*& pfiA, Int blkNum)
{	
	if (blkNum == U_BLOCK || blkNum == V_BLOCK) {
		pfiA = new CIntImage (*pvopfMB -> getPlane (BUV_PLANE));
	}
	else if ((Int) blkNum >= (Int) Y_BLOCK1 && (Int) blkNum <= (Int) Y_BLOCK4)  {
		UInt idx = (blkNum == Y_BLOCK1 || blkNum == Y_BLOCK3) ? 0 : 1;
		UInt idy = (blkNum == Y_BLOCK1 || blkNum == Y_BLOCK2) ? 0 : 1;
		CRct rct = pvopfMB -> whereY ();
		CoordI left = rct.left + idx * BLOCK_SIZE;
		CoordI top = rct.top + idy * BLOCK_SIZE;
		CRct rctF (left, top, left + BLOCK_SIZE, top + BLOCK_SIZE);
		pfiA = new CIntImage (*pvopfMB -> getPlane (BY_PLANE), rctF);
	}
	else { // alpha plane
		assert (pvopfMB -> fAUsage () == EIGHT_BIT);
		UInt idx = (blkNum == A_BLOCK1 || blkNum == A_BLOCK3) ? 0 : 1;
		UInt idy = (blkNum == A_BLOCK1 || blkNum == A_BLOCK2) ? 0 : 1;
		CRct rct = pvopfMB -> whereY ();
		CoordI left = rct.left + idx * BLOCK_SIZE;
		CoordI top = rct.top + idy * BLOCK_SIZE;
		CRct rctF (left, top, left + BLOCK_SIZE, top + BLOCK_SIZE);
		pfiA = new CIntImage (*pvopfMB -> getPlane (BY_PLANE), rctF);
	}
}

CRct fitToMulOfSize (const CRct& rctOrg, UInt size)
{
	CRct rct = rctOrg;
	Int remW = rct.width % size;
	Int right = rct.right;
	if (remW != 0) 
		right = rct.left + rct.width + size - remW;
	Int remH = rct.height () % size;
	Int bottom = rct.bottom;
	if (remH != 0) 
		bottom = rct.top + rct.height () + size - remH;
	return CRct (rct.left, rct.top, right, bottom);
}

own CIntImage* fiFitToMulOfSize (const CIntImage* pfi, UInt size, const CSite* pstLeftTop)
{
	CRct rctOrig = pfi -> where ();
	if (pstLeftTop != NULL)	{
		rctOrig.left = pstLeftTop->x;
		rctOrig.top = pstLeftTop->y;
	}
	CRct rctExpanded = fitToMulOfSize (rctOrig, size); 
	return new CIntImage (*pfi, rctExpanded);
}

own CVOPIntYUVBA* vopfFitToMulOfMBSize (const CVOPIntYUVBA* pvopf, const CSite* pstLeftTop)
{
	if (!pvopf -> valid ()) return NULL;
	CVOPIntYUVBA* pvopfRet = new CVOPIntYUVBA (pvopf -> fAUsage ());
	UInt iPln;
	for (iPln = Y_PLANE; iPln <= BUV_PLANE; iPln++) {
		if (pvopf -> fAUsage () != EIGHT_BIT) {
			if (iPln == A_PLANE)
				continue;
		}
		UInt size;
		CSite stOrig;
		if (iPln == U_PLANE || iPln == V_PLANE  || iPln == BUV_PLANE)		{
			size = MB_SIZE >> 1;
			if (pstLeftTop != NULL)
				stOrig = *pstLeftTop / 2;
			else
				stOrig = CSite (pvopf -> whereUV ().left, pvopf -> whereUV ().top);
		}
		else {
			size = MB_SIZE;
			if (pstLeftTop != NULL)
				stOrig = *pstLeftTop;
			else
				stOrig = CSite (pvopf -> whereY ().left, pvopf -> whereY ().top);
		}
		CIntImage* pfiNew = fiFitToMulOfSize (pvopf -> getPlane ((PlaneType) iPln), size, &stOrig);
		pvopfRet -> setPlane (pfiNew, (PlaneType) iPln);
		delete pfiNew;
	}
	return pvopfRet;
}


own CIntImage* alphaFromCompFile (
	const Char* pchSeg, 
	UInt ifr, UInt iobj, 
	const CRct& rct, 
	UInt nszHeader
)
{
	CIntImage* pfiRet = new CIntImage (pchSeg, ifr, rct, nszHeader);
	PixelI* ppxlf = (PixelI*) pfiRet -> pixels ();
	UInt area = pfiRet -> where ().area ();
	for (UInt ip = 0; ip < area; ip++, ppxlf++) {
		if (*ppxlf == (PixelI) iobj)
			*ppxlf = (PixelI) opaqueValue;
		else
			*ppxlf = (PixelI) transpValue;
	}
	return pfiRet;
}

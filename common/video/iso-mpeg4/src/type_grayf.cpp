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
assign or donate the code to a third party and to inhibit third parties from using the code for non <MPEG standard> conforming products. 
This copyright notice must be included in all copies or derivative works. 

Copyright (c) 1996, 1997.


Module Name:

	grayf.hpp

Abstract:

	Float image class for gray (one-plane) pictures 

Revision History:

*************************************************************************/

#include <stdlib.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include "typeapi.h"

#ifdef __MFC_
#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

#define new DEBUG_NEW				   
#endif // __MFC_


CFloatImage::~CFloatImage ()
{
	delete [] m_ppxlf;
	m_ppxlf = NULL;
}

Void CFloatImage::allocate (const CRct& r, PixelF pxlf) 
{
	m_rc = r;
	delete [] m_ppxlf, m_ppxlf = NULL;

	// allocate pixels and initialize
	if (m_rc.empty ()) return;
	m_ppxlf = new PixelF [m_rc.area ()];
	PixelF* ppxlf = m_ppxlf;
	UInt area = where ().area ();
	for (UInt i = 0; i < area; i++)
		*ppxlf++ = pxlf;
}

Void CFloatImage::copyConstruct (const CFloatImage& fi, const CRct& rct) 
{
	CRct r = rct;
	if (!r.valid()) 
		r = fi.where ();
	if (!fi.valid () || (!fi.m_rc.empty () && fi.m_ppxlf == NULL))
		assert (FALSE);
	allocate (r, (PixelF) 0);
	if (!valid ()) return; 

	// Copy data
	if (r == fi.where ())
		memcpy (m_ppxlf, fi.pixels (), m_rc.area () * sizeof (PixelF));
	else {
		r.clip (fi.where ()); // find the intersection
		CoordI x = r.left; // copy pixels
		Int cbLine = r.width * sizeof (PixelF);
		PixelF* ppxl = (PixelF*) pixels (x, r.top);
		const PixelF* ppxlFi = fi.pixels (x, r.top);
		Int widthCurr = where ().width;
		Int widthFi = fi.where ().width;
		for (CoordI y = r.top; y < r.bottom; y++) {
			memcpy (ppxl, ppxlFi, cbLine);
			ppxl += widthCurr;
			ppxlFi += widthFi;
		}
	}
}

Void CFloatImage::swap (CFloatImage& fi) 
{
	assert (this && &fi);
	CRct rcT = fi.m_rc; 
	fi.m_rc = m_rc; 
	m_rc = rcT; 
	PixelF* ppxlfT = fi.m_ppxlf; 
	fi.m_ppxlf = m_ppxlf; 
	m_ppxlf = ppxlfT; 
}

CFloatImage::CFloatImage (const CFloatImage& fi, const CRct& r) : m_ppxlf (NULL)
{
	copyConstruct (fi, r);
}

CFloatImage::CFloatImage (const CIntImage& ii, const CRct& rct) : m_ppxlf (NULL)
{
	CRct r = rct;
	if (!r.valid()) 
		r = ii.where ();
	if (!ii.valid ())
		assert (FALSE);
	allocate (r, (PixelF) 0);
	if (!valid ()) return; 

	// Copy data
	if (r == ii.where ())
	{
		Int i;
		PixelF *ppxlf = m_ppxlf;
		const PixelI *ppxli = ii.pixels();
		for(i=m_rc.area();i;i--)
			*ppxlf++ = (PixelF)*ppxli++;
	}
	else {
		r.clip (ii.where ()); // find the intersection
		CoordI x = r.left; // copy pixels
		Int cbLine = r.width;
		PixelF* ppxlf = (PixelF*) pixels (x, r.top);
		const PixelI* ppxli = ii.pixels (x, r.top);
		Int widthCurr = where ().width;
		Int widthii = ii.where ().width;
		for (CoordI y = r.top; y < r.bottom; y++) {
			for(x = 0;x<cbLine;x++)
				ppxlf[x]=(PixelF)ppxli[x];
			ppxlf += widthCurr;
			ppxli += widthii;
		}
	}
}

CFloatImage::CFloatImage (const CRct& r, PixelF px) : m_ppxlf (NULL)
{
	allocate (r, px);
}

CFloatImage::CFloatImage (const CVideoObjectPlane& vop, RGBA comp, const CRct& r) : m_ppxlf (NULL)
{
	if (!vop.valid ()) return;
	CFloatImage* pfi = new CFloatImage (vop.where ());
	PixelF* ppxlf = (PixelF*) pfi -> pixels ();
	const CPixel* ppxl = vop.pixels ();
	UInt area = pfi -> where ().area ();
	for (UInt ip = 0; ip < area; ip++, ppxl++)
		*ppxlf++ = (PixelF) ppxl -> pxlU.color [comp];
	copyConstruct (*pfi, r);
	delete pfi;
}

CFloatImage::CFloatImage (
		const Char* pchFileName, 
		UInt ifr,
		const CRct& rct,
		UInt nszHeader ) : m_ppxlf (NULL)
{
	assert (!rct.empty ());
	assert (ifr >= 0);
	assert (nszHeader >= 0);
	UInt area = rct.area ();
	// allocate mem for buffers
	U8* pchBuffer = new U8 [area];
	U8* pchBuffer0 = pchBuffer;

	// read data from a file
	FILE* fpSrc = fopen (pchFileName, "rb");
	assert (fpSrc != NULL);
	fseek (fpSrc, nszHeader + ifr * sizeof (U8) * area, SEEK_SET);
	Int size = (Int) fread (pchBuffer, sizeof (U8), area, fpSrc);
	assert (size != 0);
	fclose (fpSrc);

	// assign buffers to a vframe
	allocate (rct, .0f); 
	PixelF* p = (PixelF*) pixels ();
	UInt areaThis = where ().area ();
	for (UInt ip = 0; ip < areaThis; ip++)
		*p++ = (PixelF) *pchBuffer0++;
	delete [] pchBuffer;
}

CFloatImage::CFloatImage (const Char* vdlFileName) : m_ppxlf (NULL)// read from a VM file.
{
	CVideoObjectPlane vop (vdlFileName);
	allocate (vop.where (), 0.0f);
	const CPixel* ppxlVop = vop.pixels ();
	PixelF* ppxlfRet = (PixelF*) pixels ();
	UInt area = where ().area ();
	for (UInt ip = 0; ip < area; ip++, ppxlfRet++, ppxlVop++)
		*ppxlfRet = (PixelF) ppxlVop -> pxlU.rgb.r;
}

Void CFloatImage::where (const CRct& r) 
{
	if (!valid ()) return; 
	if (where () == r) return; 
	CFloatImage* pfi = new CFloatImage (*this, r);
	swap (*pfi);
	delete pfi;
}


CRct CFloatImage::boundingBox (const PixelF pxlfOutsideColor) const
{
	if (allValue ((PixelF) pxlfOutsideColor))	
		return CRct ();
	CoordI left = where ().right - 1;
	CoordI top = where ().bottom - 1;
	CoordI right = where ().left;
	CoordI bottom = where ().top;
	const PixelF* ppxlfThis = pixels ();
	for (CoordI y = where ().top; y < where ().bottom; y++) {
		for (CoordI x = where ().left; x < where ().right; x++) {
			if (*ppxlfThis != (PixelF) pxlfOutsideColor) {
				left = min (left, x);
				top = min (top, y);
				right = max (right, x);
				bottom = max (bottom, y);
			}								
			ppxlfThis++;
		}
	}
	right++;
	bottom++;
	return (CRct (left, top, right, bottom));				
}


Double CFloatImage::mean () const
{
	if (where ().empty ()) return 0;
	Double meanRet = 0;
	PixelF* ppxlf = (PixelF*) pixels ();
	UInt area = where ().area ();
	for (UInt ip = 0; ip < area; ip++)
		meanRet += *ppxlf++;
	meanRet /= area;
	return meanRet;
}

Double CFloatImage::mean (const CFloatImage* pfiMsk) const
{
	assert (where () == pfiMsk -> where ()); // no compute if rects are different
	if (where ().empty ()) return 0;
	Double meanRet = 0;
	PixelF* ppxlf = (PixelF*) pixels ();
	const PixelF* ppxlfMsk = pfiMsk -> pixels ();
	UInt area = where ().area ();
	UInt uiNumNonTransp = 0;
	for (UInt ip = 0; ip < area; ip++, ppxlf++, ppxlfMsk++) {
		if (*ppxlfMsk != transpValueF) {
			uiNumNonTransp++;
			meanRet += *ppxlf;
		}
	}
	meanRet /= uiNumNonTransp;
	return meanRet;
}

Double CFloatImage::sumAbs (const CRct& rct) const 
{
	CRct rctToDo = (!rct.valid ()) ? where () : rct;
	Double dblRet = 0;
	if (rctToDo == where ())	{	
		const PixelF* ppxlf = pixels ();
		UInt area = where ().area ();
		for (UInt ip = 0; ip < area; ip++, ppxlf++) {
			if (*ppxlf > 0)
				dblRet += *ppxlf;
			else
				dblRet -= *ppxlf;
		}
	}
	else	{
		Int width = where ().width;
		const PixelF* ppxlfRow = pixels (rct.left, rct.top);
		for (CoordI y = rctToDo.top; y < rctToDo.bottom; y++) {
			const PixelF* ppxlf = ppxlfRow;
			for (CoordI x = rctToDo.left; x < rctToDo.right; x++, ppxlf++) {
				if (*ppxlf > 0)
					dblRet += *ppxlf;
				else
					dblRet -= *ppxlf;
			}
			ppxlfRow += width;
		}
	}
	return dblRet;
}


Double CFloatImage::sumDeviation (const CFloatImage* pfiMsk) const // sum of first-order deviation
{
	Double meanPxl = mean (pfiMsk);
	Double devRet = 0;
	PixelF* ppxlf = (PixelF*) pixels ();
	const PixelF* ppxlfMsk = pfiMsk -> pixels ();
	UInt area = where ().area ();
	for (UInt ip = 0; ip < area; ip++, ppxlf++, ppxlfMsk++) {
		if (*ppxlfMsk != transpValueF) {
			Float fDiff = *ppxlf - (Float) meanPxl;
			if (fDiff > 0)
				devRet += fDiff;
			else 
				devRet -= fDiff;
		}
	}
	return devRet;
}


Double CFloatImage::sumDeviation () const // sum of first-order deviation
{
	Double meanPxl = mean ();
	Double devRet = 0;
	PixelF* ppxlf = (PixelF*) pixels ();
	UInt area = where ().area ();
	for (UInt ip = 0; ip < area; ip++, ppxlf++) {
		Float fDiff = *ppxlf - (Float) meanPxl;
		if (fDiff > 0)
			devRet += fDiff;
		else 
			devRet -= fDiff;
	}
	return devRet;
}


Bool CFloatImage::allValue (Float vl, const CRct& rct) const
{
	Int ivl = (Int) vl;
	Bool bRet = TRUE;
	CRct rctToDo = (!rct.valid ()) ? where () : rct;
	if (rctToDo == where ())	{	
		const PixelF* ppxlf = pixels ();
		UInt area = where ().area ();
		for (UInt ip = 0; ip < area; ip++, ppxlf++) {
			Int ipxl = (Int) *ppxlf; //truncation
//			if (*ppxlf != vl) {
			if (ipxl != ivl) {
				bRet = FALSE;
				return bRet;
			}
		}
	}
	else	{
		Int width = where ().width;
		const PixelF* ppxlf = pixels (rct.left, rct.top);
		for (CoordI y = rctToDo.top; y < rctToDo.bottom; y++) {
			const PixelF* ppxlfRow = ppxlf;
			for (CoordI x = rctToDo.left; x < rctToDo.right; x++, ppxlfRow++) {
				Int ipxl = (Int) *ppxlfRow; //truncation
//				if (*ppxlfRow != vl)	{
				if (ipxl != ivl)	{
					bRet = FALSE;
					return bRet;
				}
			}
			ppxlf += width;
		}
	}
	return bRet;
}

Bool CFloatImage::biLevel (const CRct& rct) const
{
	Bool bRet = TRUE;
	CRct rctToDo = (!rct.valid ()) ? where () : rct;
	if (rctToDo == where ())	{	
		const PixelF* ppxlf = pixels ();
		UInt area = where ().area ();
		for (UInt ip = 0; ip < area; ip++, ppxlf++) {
			Int ipxl = (Int) *ppxlf; //truncation
			if (ipxl != opaqueValue && ipxl != transpValue ) {
				bRet = FALSE;
				return bRet;
			}
		}
	}
	else	{
		Int width = where ().width;
		const PixelF* ppxlf = pixels (rct.left, rct.top);
		for (CoordI y = rctToDo.top; y < rctToDo.bottom; y++) {
			const PixelF* ppxlfRow = ppxlf;
			for (CoordI x = rctToDo.left; x < rctToDo.right; x++, ppxlfRow++) {
				Int ipxl = (Int) *ppxlfRow; //truncation
				if (ipxl != opaqueValue && ipxl != transpValue ) {
					bRet = FALSE;
					return bRet;
				}
			}
			ppxlf += width;
		}
	}
	return bRet;
}

CRct CFloatImage::whereVisible () const
{
	CoordI left = where ().right - 1;
	CoordI top = where ().bottom - 1;
	CoordI right = where ().left;
	CoordI bottom = where ().top;
	const PixelF* ppxlfThis = pixels ();
	for (CoordI y = where ().top; y < where ().bottom; y++) {
		for (CoordI x = where ().left; x < where ().right; x++) {
			if (*ppxlfThis != (PixelF) transpValue) {
				left = min (left, x);
				top = min (top, y);
				right = max (right, x);
				bottom = max (bottom, y);
			}								
			ppxlfThis++;
		}
	}
	right++;
	bottom++;
	return CRct (left, top, right, bottom);
}


Bool CFloatImage::atLeastOneValue (Float vl, const CRct& rct) const
{
	CRct rctRegionOfInterest = (!rct.valid ()) ? where () : rct;
	assert (rctRegionOfInterest <= where ());
	if (rctRegionOfInterest == where ()) {
		const PixelF* ppxlf = pixels ();
		UInt area = where ().area ();
		for (UInt ip = 0; ip < area; ip++, ppxlf++) {
			if (*ppxlf == vl) {
				return (TRUE);
			}
		}
	}
	else {
		Int width = where ().width;
		const PixelF* ppxlf = pixels (rctRegionOfInterest.left, rctRegionOfInterest.top);
		for (CoordI y = rctRegionOfInterest.top; y < rctRegionOfInterest.bottom; y++) {
			const PixelF* ppxlfRow = ppxlf;
			for (CoordI x = rctRegionOfInterest.left; x < rctRegionOfInterest.right; x++, ppxlfRow++) {
				if (*ppxlfRow == vl) {
					return (TRUE);
					break;
				}
			}
			ppxlf += width;
		}
	}
	return (FALSE);
}


UInt CFloatImage::numPixelsNotValued (Float vl, const CRct& rct) const // number of pixels not valued vl in region rct
{
	CRct rctInterest = (!rct.valid ()) ? where () : rct;
	assert (rctInterest <= where ());
	UInt nRet = 0;
	if (rctInterest == where ()) {
		const PixelF* ppxlf = pixels ();
		UInt area = where ().area ();
		for (UInt ip = 0; ip < area; ip++, ppxlf++) {
			if (*ppxlf != vl)
				nRet++;
		}
	}
	else {
		Int width = where ().width;
		const PixelF* ppxlf = pixels (rctInterest.left, rctInterest.top);
		for (CoordI y = rctInterest.top; y < rctInterest.bottom; y++) {
			const PixelF* ppxlfRow = ppxlf;
			for (CoordI x = rctInterest.left; x < rctInterest.right; x++, ppxlfRow++) {
				if (*ppxlfRow != vl)
					nRet++;
			}
			ppxlf += width;
		}
	}

	return nRet;
}


Void CFloatImage::setRect (const CRct& rct)
{
	assert (rct.area () == m_rc.area ());
	m_rc = rct;
}

Void CFloatImage::threshold (Float thresh)
{
	PixelF* ppxlThis = (PixelF*) pixels ();
	UInt area = where ().area ();
	for (UInt id = 0; id < area; id++)
	{
		if (fabs (*ppxlThis) < thresh)
			*ppxlThis = (PixelF) 0;
		ppxlThis++;
	}
}

Void CFloatImage::binarize (Float fltThresh)
{
	PixelF* ppxlThis = (PixelF*) pixels ();
	UInt area = where ().area ();
	for (UInt id = 0; id < area; id++)
	{
		if (fabs (*ppxlThis) < fltThresh)
			*ppxlThis = (PixelF) transpValue;
		else
			*ppxlThis = (PixelF) opaqueValue;
		ppxlThis++;
	}
}


Void CFloatImage::checkRange (Float fltMin, Float fltMax)
{
	PixelF* ppxlThis = (PixelF*) pixels ();
	UInt area = where ().area ();
	for (UInt id = 0; id < area; id++, ppxlThis++)
		*ppxlThis = checkrange (*ppxlThis, fltMin, fltMax);
}

own CFloatImage* CFloatImage::decimate (UInt rateX, UInt rateY) const
{
	const CoordI left = where ().left / (CoordI) rateX;
	const CoordI top = where ().top / (CoordI)  rateY;
	const CoordI right = (where ().right >= 0) ? (where ().right + (CoordI) rateX - 1) / (CoordI) rateX 
							: (where ().right - (CoordI) rateX + 1) / (CoordI) rateX;
	const CoordI bottom = (where ().bottom >= 0) ? (where ().bottom + (CoordI) rateX - 1) / (CoordI) rateY
							: (where ().bottom - (CoordI) rateX + 1) / (CoordI) rateY;

	CFloatImage* pfiRet = new CFloatImage (CRct (left, top, right, bottom));
	PixelF* ppxlfRet = (PixelF*) pfiRet -> pixels ();
	const PixelF* ppxlfOrgY = pixels ();
	Int skipY = rateY * where ().width;
	
	for (CoordI y = top; y < bottom; y++) {
		const PixelF* ppxlfOrgX = ppxlfOrgY;
		for (CoordI x = left; x < right; x++) {
			*ppxlfRet++ = *ppxlfOrgX;
			ppxlfOrgX += rateX;
		}
		ppxlfOrgY += skipY;
	}
	return pfiRet;
}

own CFloatImage* CFloatImage::decimateBinaryShape (UInt rateX, UInt rateY) const
{
	const CoordI left = where ().left / (CoordI) rateX;
	const CoordI top = where ().top / (CoordI) rateY;
	Int roundR = (where ().right >= 0) ? rateX - 1 : 1 - rateX;
	Int roundB = (where ().bottom >= 0) ? rateY - 1 : 1 - rateY;
	const CoordI right = (where ().right + roundR) / (CoordI) rateX;
	const CoordI bottom = (where ().bottom + roundB) / (CoordI) rateY;

	CFloatImage* pfiRet = new CFloatImage (CRct (left, top, right, bottom));
	PixelF* ppxlfRet = (PixelF*) pfiRet -> pixels ();
	const PixelF* ppxlfOrgY = pixels ();
	Int skipY = rateY * where ().width;
	CoordI x, y;
	CoordI iXOrigLeft, iYOrigTop;	//left top of a sampling square

	for (y = top, iYOrigTop = where().top; y < bottom; y++, iYOrigTop += rateY) {
		const PixelF* ppxlfOrgX = ppxlfOrgY;
		for (x = left, iXOrigLeft = where().left; x < right; x++, iXOrigLeft += rateX) {

			CoordI iXOrig, iYOrig;						//for scanning of the sampling square
			const PixelF* ppxlfOrigScanY = ppxlfOrgX;	//same
			*ppxlfRet = transpValueF;

			for (iYOrig = iYOrigTop; iYOrig < (iYOrigTop + (Int) rateY); iYOrig++)	{

				if (iYOrig >= where().bottom || *ppxlfRet == opaqueValueF)
					break;
				const PixelF* ppxlfOrigScanX = ppxlfOrigScanY; //for scan also
				for (iXOrig = iXOrigLeft; iXOrig < (iXOrigLeft + (Int) rateX); iXOrig++)	{
					if (iXOrig >= where().right)
						break;
					assert (*ppxlfOrigScanX == transpValueF || *ppxlfOrigScanX == opaqueValueF);
					if (*ppxlfOrigScanX == opaqueValueF)	{
						*ppxlfRet = opaqueValueF;
						break;
					}
					ppxlfOrigScanX++;
				}
				ppxlfOrigScanY += where().width;
			}

			assert (*ppxlfRet == transpValueF || *ppxlfRet == opaqueValueF);			
			ppxlfRet++;
//			*ppxlfRet++ = *ppxlfOrgX;
			ppxlfOrgX += rateX;
		}
		ppxlfOrgY += skipY;
	}
	return pfiRet;
}

own CFloatImage* CFloatImage::zoomup (UInt rateX, UInt rateY) const
{
	const CoordI left = where ().left * rateX; // left-top coordinate remain the same
	const CoordI top = where ().top * rateY;
	const CoordI right = where ().right * rateX;
	const CoordI bottom = where ().bottom * rateY;

	CFloatImage* pfiRet = new CFloatImage (CRct (left, top, right, bottom));
	PixelF* ppxlRet = (PixelF*) pfiRet -> pixels ();
	for (CoordI y = top; y < bottom; y++)
	{
		for (CoordI x = left; x < right; x++)
			*ppxlRet++ = pixel ((CoordI) (x / rateX), (CoordI) (y / rateY));
	}
	return pfiRet;
}

own CFloatImage* CFloatImage::expand (UInt rateX, UInt rateY) const // expand by putting zeros in between
{
	const CoordI left = where ().left * rateX; // left-top coordinate remain the same
	const CoordI top = where ().top * rateY;
	const CoordI right = where ().right * rateX;
	const CoordI bottom = where ().bottom * rateY;

	CFloatImage* pfiRet = new CFloatImage (CRct (left, top, right, bottom));
	PixelF* ppxlRet = (PixelF*) pfiRet -> pixels ();
	const PixelF* ppxlThis = pixels ();
	for (CoordI y = top; y < bottom; y++) {
		for (CoordI x = left; x < right; x++) {
			if (x % rateX == 0 && y % rateY == 0) 
				*ppxlRet++ = *ppxlThis++;
			else
				*ppxlRet++ = (PixelF) 0;
		}
	}
	return pfiRet;
}

own CFloatImage* CFloatImage::biInterpolate () const // bilinearly interpolate the vframe
{
	const CoordI left = where ().left << 1; // left-top coordinate remain the same
	const CoordI top = where ().top << 1;
	const CoordI right = where ().right << 1;
	const CoordI bottom = where ().bottom << 1;
	const CoordI width = right - left;

	CoordI x, y;
	CFloatImage* pfiRet = new CFloatImage (CRct (left, top, right, bottom));
	PixelF* ppxlfRet = (PixelF*) pfiRet -> pixels ();
	const PixelF* ppxlf = pixels ();
	const CoordI right1 = right - 2;
	for (y = top; y < bottom; y += 2) { // x-direction interpolation
		for (x = left; x < right1; x += 2) {
			*ppxlfRet++ = *ppxlf++;
			*ppxlfRet++ = (*ppxlf + *(ppxlf - 1)) * .5f;
		}
		*ppxlfRet++ = *ppxlf;
		*ppxlfRet++ = *ppxlf++; // the last pixel of every row do not need average
		ppxlfRet += width;
	}
	ppxlfRet = (PixelF*) pfiRet -> pixels ();
	ppxlfRet += width; // start from the second row
	const CoordI width2 = width << 1;
	const CoordI bottom1 = bottom - 1;
	for (x = left; x < right; x++) { // y-direction interpolation
		PixelF* ppxlfRett = ppxlfRet++;
		for (y = top + 1; y < bottom1; y += 2) {
			*ppxlfRett = (*(ppxlfRett - width) + *(ppxlfRett + width)) * .5f;
			ppxlfRett += width2;
		}
		*ppxlfRett = *(ppxlfRett - width); // the last pixel of every column do not need average
	}
	return pfiRet;
}

own CFloatImage* CFloatImage::downsampleForSpatialScalability () const
{
	static Int rgiFilterVertical[13] = {2, 0, -4, -3, 5, 19, 26, 19, 5, -3, -4, 0, 2};
	static Int rgiFilterHorizontal[4] = {5, 11, 11, 5};
	Int iWidthSrc = where (). width;
	Int iHeightSrc = where (). height ();
	assert (iWidthSrc % 2 == 0 && iHeightSrc % 2 == 0);
	Int iWidthDst = iWidthSrc / 2;
	Int iHeightDst = iHeightSrc / 2;
	CFloatImage* pfiBuffer = new CFloatImage (CRct (0, 0, iWidthSrc, iHeightDst));
	CFloatImage* pfiRet = new CFloatImage (CRct (0, 0, iWidthDst, iHeightDst));
	assert (pfiBuffer != NULL);
	assert (pfiRet != NULL);

	//filter and downsample vertically
	const PixelF* ppxlfSrc;	
	const PixelF* ppxlfColumnHeadSrc = pixels ();
	PixelF* ppxlfDst = (PixelF*) pfiBuffer->pixels ();
	PixelF* ppxlfColumnHeadDst = (PixelF*) pfiBuffer->pixels ();
	Int i, j, k;
	for (i = 0; i < iWidthSrc; i++)	{
		ppxlfSrc = ppxlfColumnHeadSrc;	
		ppxlfDst = ppxlfColumnHeadDst;	
		for (j = 0; j < iHeightDst; j++)	{
			k = j * 2;         
			const PixelF* ppxlfMinusOne = ( k < 1 ) ? ppxlfSrc : ppxlfSrc - iWidthSrc;		
			const PixelF* ppxlfMinusTwo = ( k < 2 ) ? ppxlfSrc : ppxlfMinusOne - iWidthSrc;
			const PixelF* ppxlfMinusThree = ( k < 3 ) ? ppxlfSrc : ppxlfMinusTwo - iWidthSrc;
			const PixelF* ppxlfMinusFour = ( k < 4 ) ? ppxlfSrc : ppxlfMinusThree - iWidthSrc;
			const PixelF* ppxlfMinusFive = ( k < 5 ) ? ppxlfSrc : ppxlfMinusFour - iWidthSrc;
			const PixelF* ppxlfMinusSix = ( k < 6 ) ? ppxlfSrc : ppxlfMinusFive - iWidthSrc;
			const PixelF* ppxlfPlusOne = ( k >= iHeightSrc - 1) ? ppxlfSrc : ppxlfSrc + iWidthSrc;
			const PixelF* ppxlfPlusTwo = ( k >= iHeightSrc - 2) ? ppxlfPlusOne : ppxlfPlusOne + iWidthSrc;
			const PixelF* ppxlfPlusThree = ( k >= iHeightSrc - 3) ? ppxlfPlusTwo : ppxlfPlusTwo + iWidthSrc;
			const PixelF* ppxlfPlusFour = ( k >= iHeightSrc - 4) ? ppxlfPlusThree : ppxlfPlusThree + iWidthSrc;
			const PixelF* ppxlfPlusFive = ( k >= iHeightSrc - 5) ? ppxlfPlusFour : ppxlfPlusFour + iWidthSrc;
			const PixelF* ppxlfPlusSix = ( k >= iHeightSrc - 6) ? ppxlfPlusFive : ppxlfPlusFive + iWidthSrc;
			*ppxlfDst = checkrange ((*ppxlfMinusSix  * rgiFilterVertical [0] +
									*ppxlfMinusFive * rgiFilterVertical [1] +
									*ppxlfMinusFour * rgiFilterVertical [2] +
									*ppxlfMinusThree* rgiFilterVertical [3] +
									*ppxlfMinusTwo  * rgiFilterVertical [4] +
									*ppxlfMinusOne  * rgiFilterVertical [5] +
									*ppxlfSrc	   * rgiFilterVertical [6] +
									*ppxlfPlusOne   * rgiFilterVertical [7] +
									*ppxlfPlusTwo   * rgiFilterVertical [8] +
									*ppxlfPlusThree * rgiFilterVertical [9] +
									*ppxlfPlusFour  * rgiFilterVertical [10] +
									*ppxlfPlusFive  * rgiFilterVertical [11] +
									*ppxlfPlusSix   * rgiFilterVertical [12]) / 64,
									0.0F, 255.0F);
			ppxlfSrc += 2 * iWidthSrc;
			ppxlfDst += iWidthSrc;	
		}
		ppxlfColumnHeadSrc++;
		ppxlfColumnHeadDst++;
	} 
	//filter and downsample horizontally
	ppxlfSrc = pfiBuffer->pixels ();	
	ppxlfDst = (PixelF*) pfiRet->pixels ();
	for (j = 0; j < iHeightDst; j++)	{
		for (i = 0; i < iWidthDst; i++)	{
			k = i * 2;         
			const PixelF* ppxlfMinusOne = ( k < 1 ) ? ppxlfSrc : ppxlfSrc - 1;		
			const PixelF* ppxlfPlusOne = ( k >= iWidthSrc - 1) ? ppxlfSrc : ppxlfSrc + 1;
			const PixelF* ppxlfPlusTwo = ( k >= iWidthSrc - 2) ? ppxlfSrc : ppxlfSrc + 2;
			*ppxlfDst = checkrange ((*ppxlfMinusOne  * rgiFilterHorizontal [0] +
									*ppxlfSrc	   * rgiFilterHorizontal [1] +
									*ppxlfPlusOne   * rgiFilterHorizontal [2] +
									*ppxlfPlusTwo   * rgiFilterHorizontal [3]) / 32,
									0.0F, 255.0F);
			ppxlfSrc += 2;	
			ppxlfDst++;	
		}
	} 
	delete pfiBuffer;
	return pfiRet;
}

own CFloatImage* CFloatImage::upsampleForSpatialScalability () const
{
	CRct rctDst = where () * 2;
	Int iWidthSrc = where (). width;
	Int iHeightSrc = where (). height ();
	Int iHeightDst = iHeightSrc * 2;
	CFloatImage* pfiBuffer = new CFloatImage (CRct (where().left, rctDst.top, 
													 where().right, rctDst.bottom));
	CFloatImage* pfiRet = new CFloatImage (CRct (rctDst.left, rctDst.top, 
												  rctDst.right, rctDst.bottom));

	//upsample vertically
	const PixelF* ppxlfSrc;	
	const PixelF* ppxlfSrcPlusOne;	
	const PixelF* ppxlfColumnHeadSrc = pixels ();
	PixelF* ppxlfDst;
	PixelF* ppxlfColumnHeadDst = (PixelF*) pfiBuffer->pixels ();
	Int i, j;
	for (i = 0; i < iWidthSrc; i++)	{
		ppxlfSrc = ppxlfColumnHeadSrc;	
		ppxlfSrcPlusOne = ppxlfSrc + iWidthSrc;
		ppxlfDst = ppxlfColumnHeadDst;	
		for (j = 0; j < iHeightSrc; j++)	{
			*ppxlfDst = checkrange ( (*ppxlfSrc * 3 + *ppxlfSrcPlusOne) / 4,
									0.0F, 255.0F);
			ppxlfDst += iWidthSrc;
			*ppxlfDst = checkrange ( (*ppxlfSrc + *ppxlfSrcPlusOne * 3) / 4,
									0.0F, 255.0F);
			ppxlfDst += iWidthSrc;
			ppxlfSrc += iWidthSrc;
			ppxlfSrcPlusOne = (j >= iHeightSrc - 2) ? ppxlfSrc : ppxlfSrc + iWidthSrc;
		}
		ppxlfColumnHeadSrc++;
		ppxlfColumnHeadDst++;
	} 

	//upsample horizontally	
	ppxlfSrc = pfiBuffer->pixels ();
	ppxlfDst = (PixelF*) pfiRet->pixels ();
	for (j = 0; j < iHeightDst; j++)	{
		ppxlfSrcPlusOne = ppxlfSrc + 1;
		for (i = 0; i < iWidthSrc; i++)	{
			*ppxlfDst = checkrange ( (*ppxlfSrc * 3 + *ppxlfSrcPlusOne) / 4,
									0.0F, 255.0F);
			ppxlfDst++;
			*ppxlfDst = checkrange ( (*ppxlfSrc + *ppxlfSrcPlusOne * 3) / 4,
									0.0F, 255.0F);
			ppxlfDst++;
			ppxlfSrc++;
			ppxlfSrcPlusOne = (i >= iWidthSrc - 2) ? ppxlfSrc : ppxlfSrc + 1;
		}
	} 
	delete pfiBuffer;
	return pfiRet;
}

own CFloatImage* CFloatImage::biInterpolate (UInt accuracy) const // bilinearly interpolate the vframe
{
	const CoordI left = where ().left * accuracy;
	const CoordI top = where ().top * accuracy;
	const CoordI right = where ().right * accuracy;
	const CoordI bottom = where ().bottom * accuracy;

	CFloatImage* pfiRet = new CFloatImage (CRct (left, top, right, bottom));
	PixelF* ppxlfRet = (PixelF*) pfiRet -> pixels ();
	for (CoordI y = top; y < bottom; y++) { // x-direction interpolation
		for (CoordI x = left; x < right; x++) {
			*ppxlfRet = pixel (x, y, accuracy);
			ppxlfRet++;
		}
	}
	return pfiRet;
}

own CFloatImage* CFloatImage::transpose () const
{
	CRct rctDst = where ();
	rctDst.transpose ();
	CFloatImage* pfiDst = new CFloatImage (rctDst);
	const PixelF* ppxlSrc = pixels ();
	PixelF* ppxlDstRow = (PixelF*) pfiDst->pixels ();
	PixelF* ppxlDst;
	UInt height = where ().height ();
	for (CoordI iy = where ().top; iy < where ().bottom; iy++)	{
		ppxlDst = ppxlDstRow;
		for (CoordI ix = where ().left; ix < where ().right; ix++)	{
			*ppxlDst = *ppxlSrc++;
			ppxlDst += height;
		}
		ppxlDstRow++;
	}
	return pfiDst;
}

own CFloatImage* CFloatImage::warp (const CAffine2D& aff) const // affine warp
{
	CSiteD stdLeftTopWarp = aff * CSiteD (where ().left, where ().top);
	CSiteD stdRightTopWarp = aff * CSiteD (where ().right, where ().top);
	CSiteD stdLeftBottomWarp = aff * CSiteD (where ().left, where ().bottom);
	CSiteD stdRightBottomWarp = aff * CSiteD (where ().right, where ().bottom);
	CRct rctWarp (stdLeftTopWarp, stdRightTopWarp, stdLeftBottomWarp, stdRightBottomWarp);

	CFloatImage* pfiRet = new CFloatImage (rctWarp);
	PixelF* ppxlfRet = (PixelF*) pfiRet -> pixels ();
	CAffine2D affInv = aff.inverse ();
	for (CoordI y = rctWarp.top; y != rctWarp.bottom; y++) {
		for (CoordI x = rctWarp.left; x != rctWarp.right; x++) {
			CSiteD src = affInv * CSiteD (x, y); 
			CoordI fx = (CoordI) floor (src.x); //.5 is for better truncation
			CoordI fy = (CoordI) floor (src.y); //.5 is for better truncation
			CoordI cx = (CoordI) ceil (src.x); //.5 is for better truncation
			CoordI cy = (CoordI) ceil (src.y); //.5 is for better truncation
			if (
				where ().includes (fx, fy) && 
				where ().includes (fx, cy) && 
				where ().includes (cx, fy) && 
				where ().includes (cx, cy)
			)
				*ppxlfRet = pixel (src);
			ppxlfRet++;
		}
	}
	return pfiRet;
}


own CFloatImage* CFloatImage::warp (const CPerspective2D& persp) const // perspective warp
{
	CSiteD src [4], dest [4];
	src [0] = CSiteD (where ().left, where ().top);
	src [1] = CSiteD (where ().right, where ().top);
	src [2] = CSiteD (where ().left, where ().bottom);
	src [3] = CSiteD (where ().right, where ().bottom);
	for (UInt i = 0; i < 4; i++) {
		dest [i] = (persp * src [i]).s;
	}
	CRct rctWarp (dest [0], dest [1], dest [2], dest [3]);

	CFloatImage* pfiRet = new CFloatImage (rctWarp);
	PixelF* ppxlfRet = (PixelF*) pfiRet -> pixels ();
	CPerspective2D perspInv = CPerspective2D (dest, src);
	for (CoordI y = rctWarp.top; y != rctWarp.bottom; y++) {
		for (CoordI x = rctWarp.left; x != rctWarp.right; x++) {
			CSiteD src = (perspInv * CSiteD (x, y)).s; 
			CoordI fx = (CoordI) floor (src.x); //.5 is for better truncation
			CoordI fy = (CoordI) floor (src.y); //.5 is for better truncation
			CoordI cx = (CoordI) ceil (src.x); //.5 is for better truncation
			CoordI cy = (CoordI) ceil (src.y); //.5 is for better truncation
			if (
				where ().includes (fx, fy) && 
				where ().includes (fx, cy) && 
				where ().includes (cx, fy) && 
				where ().includes (cx, cy)
			)
				*ppxlfRet = pixel (src);
			ppxlfRet++;
		}
	}
	return pfiRet;
}

own CFloatImage* CFloatImage::warp (const CPerspective2D& persp, const CRct& rctWarp) const // perspective warp
{
	CFloatImage* pfiRet = new CFloatImage (rctWarp);
	PixelF* ppxlfRet = (PixelF*) pfiRet -> pixels ();
	CPerspective2D perspInv = persp.inverse ();
	for (CoordI y = rctWarp.top; y != rctWarp.bottom; y++) {
		for (CoordI x = rctWarp.left; x != rctWarp.right; x++) {
			CSiteD src = (perspInv * CSiteD (x, y)).s; 
			CoordI fx = (CoordI) floor (src.x); //.5 is for better truncation
			CoordI fy = (CoordI) floor (src.y); //.5 is for better truncation
			CoordI cx = (CoordI) ceil (src.x); //.5 is for better truncation
			CoordI cy = (CoordI) ceil (src.y); //.5 is for better truncation
			if (
				where ().includes (fx, fy) && 
				where ().includes (fx, cy) && 
				where ().includes (cx, fy) && 
				where ().includes (cx, cy)
			)
				*ppxlfRet = pixel (src);
			ppxlfRet++;
		}
	}
	return pfiRet;
}

own CFloatImage* CFloatImage::warp (const CPerspective2D& persp, const CRct& rctWarp, const UInt accuracy) const // perspective warp
{
	CFloatImage* pfiRet = new CFloatImage (rctWarp);
	PixelF* ppxlfRet = (PixelF*) pfiRet -> pixels ();
	for (CoordI y = rctWarp.top; y != rctWarp.bottom; y++) {
		for (CoordI x = rctWarp.left; x != rctWarp.right; x++) {
			CSite src = (persp * CSite (x, y)).s; 
			CoordI fx = (CoordI) floor ((CoordD) src.x / (CoordD) accuracy); //.5 is for better truncation
			CoordI fy = (CoordI) floor ((CoordD) src.y / (CoordD) accuracy); //.5 is for better truncation
			CoordI cx = (CoordI) ceil ((CoordD) src.x / (CoordD) accuracy); //.5 is for better truncation
			CoordI cy = (CoordI) ceil ((CoordD) src.y / (CoordD) accuracy); //.5 is for better truncation
			if (
				where ().includes (fx, fy) && 
				where ().includes (fx, cy) && 
				where ().includes (cx, fy) && 
				where ().includes (cx, cy)
			)
				*ppxlfRet = pixel (src, accuracy);
			ppxlfRet++;
		}
	}
	return pfiRet;
}

PixelF CFloatImage::pixel (CoordD x, CoordD y) const
{
	CoordI left = (CoordI) floor (x); // find the coordinates of the four corners
	CoordI wLeft = where ().left, wRight1 = where ().right - 1, wTop = where ().top, wBottom1 = where ().bottom - 1;
	left = checkrange (left, wLeft, wRight1);
	CoordI right = (CoordI) ceil (x);
	right = checkrange (right, wLeft, wRight1);
	CoordI top = (CoordI) floor (y);
	top	= checkrange (top, wTop, wBottom1);
	CoordI bottom = (CoordI) ceil (y);
	bottom = checkrange (bottom, wTop, wBottom1);

	const PixelF lt = pixel (left, top);
	const PixelF rt = pixel (right, top);
	const PixelF lb = pixel (left, bottom);
	const PixelF rb = pixel (right, bottom);
	const Double distX = x - left;
	const Double distY = y - top;
	Double x01 = distX * (rt - lt) + lt; // use p.59's notation (Wolberg, Digital Image Warping)
	Double x23 = distX * (rb - lb) + lb;
	PixelF pxlRet = checkrange ((Float) (x01 + (x23 - x01) * distY), 0.0f, 255.0f);
	return pxlRet;
}

PixelF CFloatImage::pixel (CoordI x, CoordI y, UInt accuracy) const	// accuracy indicates the 2/4/8/16 quantization
{
	CoordI left = (CoordI) floor ((CoordD)x / (CoordD)accuracy); // find the coordinates of the four corners
	CoordI wLeft = where ().left, wRight1 = where ().right - 1, wTop = where ().top, wBottom1 = where ().bottom - 1;
	left = checkrange (left, wLeft, wRight1);
	CoordI right = (CoordI) ceil ((CoordD)x / (CoordD)accuracy);
	right = checkrange (right, wLeft, wRight1);
	CoordI top = (CoordI) floor ((CoordD)y / (CoordD)accuracy);
	top	= checkrange (top, wTop, wBottom1);
	CoordI bottom = (CoordI) ceil ((CoordD)y / (CoordD)accuracy);
	bottom = checkrange (bottom, wTop, wBottom1);

	const PixelF lt = pixel (left, top);
	const PixelF rt = pixel (right, top);
	const PixelF lb = pixel (left, bottom);
	const PixelF rb = pixel (right, bottom);
	const CoordI distX = x - left * accuracy;
	const CoordI distY = y - top * accuracy;
	Double x01 = distX * (rt - lt) + accuracy * lt; // use p.59's notation (Wolberg, Digital Image Warping)
	Double x23 = distX * (rb - lb) + accuracy * lb;
	PixelF pxlRet = checkrange ((Float) ((accuracy * x01 + (x23 - x01) * distY)) / (accuracy * accuracy), 0.0f, 255.0f);
	return pxlRet;
}

own CFloatImage* CFloatImage::complement () const
{
	CFloatImage* pfiDst = new CFloatImage (where(), (PixelF) transpValue);
	const PixelF* ppxlfSrc = pixels ();
	PixelF* ppxlfDst = (PixelF*) pfiDst->pixels ();
	for (UInt iPxl = 0; iPxl < where ().area (); iPxl++)	{
		if (*ppxlfSrc == (PixelF) opaqueValue)
			*ppxlfDst = (PixelF) transpValue;
		else if (*ppxlfSrc == (PixelF) transpValue)
			*ppxlfDst = (PixelF) opaqueValue;
		else
			assert (FALSE);				//complemetn only work on pseudo-binary data
		ppxlfSrc++;
		ppxlfDst++;
	}
	return pfiDst;
}

Void CFloatImage::overlay (const CFloatImage& fi)
{
	if (!valid () || !fi.valid () || fi.where ().empty ()) return;
	CRct r = m_rc; 
	r.include (fi.m_rc); // overlay is defined on union of rects
	where (r); 
	if (!valid ()) return; 
	assert (fi.m_ppxlf != NULL); 
	CRct rctFi = fi.m_rc;

	Int widthFi = rctFi.width;
	Int widthCurr = where ().width;
	PixelF* ppxlfThis = (PixelF*) pixels (rctFi.left, rctFi.top);
	const PixelF* ppxlfFi = fi.pixels ();
	for (CoordI y = rctFi.top; y < rctFi.bottom; y++) { // loop through VOP CRct
		memcpy (ppxlfThis, ppxlfFi, rctFi.width * sizeof (PixelF));
		ppxlfThis += widthCurr; 
		ppxlfFi += widthFi;
	}
}


own CFloatImage* CFloatImage::smooth_ (UInt window) const
{
	const UInt offset = window >> 1;
	const UInt offset2 = offset << 1;
	const UInt size = window * window; // array size to be sorted
	const UInt med = size >> 1;
	CFloatImage* pfmgRet = new CFloatImage (*this);
	// bound of the image to be filtered.
	const CoordI left = where ().left + offset;
	const CoordI top = where ().top + offset;
	const CoordI right = where ().right - offset;
	const CoordI bottom = where ().bottom - offset;
	const Int width = where ().width;
	const Int dist = offset + offset * width;
	const Int wwidth = width - window;
	PixelF* rgValues = new PixelF [size];
	PixelF* pRet = (PixelF*) pfmgRet -> pixels (left, top);
	const PixelF* p = pixels (left, top);
	for (CoordI y = top; y != bottom; y++) {
		for (CoordI x = left; x != right; x++) {
			const PixelF* pp = p - dist; // get correct index
			UInt numTransp = 0;
			for (UInt sy = 0; sy != window; sy++) {
				for (UInt sx = 0; sx != window; sx++) {
					if (*pp == (PixelF) transpValue)
						numTransp++;
					pp++;
				}
				pp += wwidth;
			}
			*pRet++ = (PixelF) ((numTransp <= med) ? opaqueValue : transpValue);
			p++;
		}
		pRet += offset2;
		p += offset2;
	}
	delete [] rgValues;
	return pfmgRet;
}

own CFloatImage* CFloatImage::smooth (UInt window) const
{
	UInt offset = window >> 1;
	CRct rctExp (where ());
	rctExp.expand (offset);
	CFloatImage* pfiExp = new CFloatImage (*this, rctExp);
	CFloatImage* pfiSmooth = pfiExp -> smooth_ (window);
	pfiSmooth -> where (where ());
	delete pfiExp;
	return pfiSmooth;
}

Void CFloatImage::xorFi (const CFloatImage& fi)
{
	CRct rctIntersect = m_rc;
	rctIntersect.clip (fi.where());
	if (!rctIntersect.valid () || rctIntersect.empty ()) return;

	PixelF* ppxlfRowStart1 = (PixelF*) pixels (rctIntersect.left, rctIntersect.top);
	const PixelF* ppxlfRowStart2 = fi.pixels (rctIntersect.left, rctIntersect.top);
	for (CoordI iy = rctIntersect.top; iy < rctIntersect.bottom; iy++)	  {
		PixelF* ppxlf1 = ppxlfRowStart1;
		const PixelF* ppxlf2 = ppxlfRowStart2;
		for (CoordI ix = rctIntersect.left; ix < rctIntersect.right; ix++)	{
			assert (*ppxlf1 == (PixelF) transpValue || *ppxlf1 == (PixelF) opaqueValue);
			assert (*ppxlf2 == (PixelF) transpValue || *ppxlf2 == (PixelF) opaqueValue);		
			if (*ppxlf1 == *ppxlf2)	
				*ppxlf1 = (PixelF) transpValue;
			else 
				*ppxlf1 = (PixelF) opaqueValue;
			ppxlf1++;
			ppxlf2++;
		}
		ppxlfRowStart1 += where().width;
		ppxlfRowStart2 += fi.where().width;
	}
}

Void CFloatImage::orFi (const CFloatImage& fi)
{
	CRct rctIntersect = m_rc;
	rctIntersect.clip (fi.where());
	if (!rctIntersect.valid () || rctIntersect.empty ()) return;

	PixelF* ppxlfRowStart1 = (PixelF*) pixels (rctIntersect.left, rctIntersect.top);
	const PixelF* ppxlfRowStart2 = fi.pixels (rctIntersect.left, rctIntersect.top);
	for (CoordI iy = rctIntersect.top; iy < rctIntersect.bottom; iy++)	  {
		PixelF* ppxlf1 = ppxlfRowStart1;
		const PixelF* ppxlf2 = ppxlfRowStart2;
		for (CoordI ix = rctIntersect.left; ix < rctIntersect.right; ix++)	{
			assert (*ppxlf1 == (PixelF) transpValue || *ppxlf1 == (PixelF) opaqueValue);
			assert (*ppxlf2 == (PixelF) transpValue || *ppxlf2 == (PixelF) opaqueValue);		
			if (*ppxlf2 == opaqueValue)
				*ppxlf1 = (PixelF) opaqueValue;
			ppxlf1++;
			ppxlf2++;
		}
		ppxlfRowStart1 += where().width;
		ppxlfRowStart2 += fi.where().width;
	}
}

Void CFloatImage::andFi (const CFloatImage& fi)
{
	CRct rctIntersect = m_rc;
	rctIntersect.clip (fi.where());
	if (!rctIntersect.valid () || rctIntersect.empty ()) return;

	PixelF* ppxlfRowStart1 = (PixelF*) pixels (rctIntersect.left, rctIntersect.top);
	const PixelF* ppxlfRowStart2 = fi.pixels (rctIntersect.left, rctIntersect.top);
	for (CoordI iy = rctIntersect.top; iy < rctIntersect.bottom; iy++)	  {
		PixelF* ppxlf1 = ppxlfRowStart1;
		const PixelF* ppxlf2 = ppxlfRowStart2;
		for (CoordI ix = rctIntersect.left; ix < rctIntersect.right; ix++)	{
			assert (*ppxlf1 == (PixelF) transpValue || *ppxlf1 == (PixelF) opaqueValue);
			assert (*ppxlf2 == (PixelF) transpValue || *ppxlf2 == (PixelF) opaqueValue);		
			if (*ppxlf2 == transpValue)
				*ppxlf1 = (PixelF) transpValue;
			ppxlf1++;
			ppxlf2++;
		}
		ppxlfRowStart1 += where().width;
		ppxlfRowStart2 += fi.where().width;
	}
}


Void CFloatImage::maskOut (const CFloatImage& fi)
{
	CRct rctIntersect = m_rc;
	rctIntersect.clip (fi.where());
	if (!rctIntersect.valid () || rctIntersect.empty ()) return;

	PixelF* ppxlfRowStart = (PixelF*) pixels (rctIntersect.left, rctIntersect.top);
	const PixelF* ppxlfRowStartMask = fi.pixels (rctIntersect.left, rctIntersect.top);
	for (CoordI iy = rctIntersect.top; iy < rctIntersect.bottom; iy++)	  {
		PixelF* ppxlf = ppxlfRowStart;
		const PixelF* ppxlfMask = ppxlfRowStartMask;
		for (CoordI ix = rctIntersect.left; ix < rctIntersect.right; ix++)	{
			assert (*ppxlfMask == (PixelF) transpValue || *ppxlfMask == (PixelF) opaqueValue);
			assert (*ppxlf == (PixelF) transpValue || *ppxlf == (PixelF) opaqueValue);		
			if (*ppxlfMask == (PixelF) transpValue)	{
			}
			else
				*ppxlf = (PixelF) transpValue;
			ppxlf++;
			ppxlfMask++;
		}
		ppxlfRowStart += where().width;
		ppxlfRowStartMask += fi.where().width;

	}
}

Void CFloatImage::mutiplyAlpha (const CFloatImage& fi)
{
	CRct rctIntersect = m_rc;
	rctIntersect.clip (fi.where());
	if (!rctIntersect.valid () || rctIntersect.empty ()) return;

	PixelF* ppxlfRowStart = (PixelF*) pixels (rctIntersect.left, rctIntersect.top);
	const PixelF* ppxlfRowStartMask = fi.pixels (rctIntersect.left, rctIntersect.top);
	for (CoordI iy = rctIntersect.top; iy < rctIntersect.bottom; iy++)	  {
		PixelF* ppxlf = ppxlfRowStart;
		const PixelF* ppxlfMask = ppxlfRowStartMask;
		for (CoordI ix = rctIntersect.left; ix < rctIntersect.right; ix++)	{
			assert (*ppxlfMask == transpValueF || *ppxlfMask == opaqueValueF);
			if (*ppxlfMask == transpValueF)	{
				*ppxlf = transpValueF;
			}
			else
				*ppxlf = *ppxlf * *ppxlfMask / opaqueValueF; //normalize
			ppxlf++;
			ppxlfMask++;
		}
		ppxlfRowStart += where().width;
		ppxlfRowStartMask += fi.where().width;

	}
}

Void CFloatImage::cropOnAlpha ()
{
	CRct rctVisible = whereVisible ();
	where (rctVisible);				
}

own CFloatImage& CFloatImage::operator += (const CFloatImage& fiSrc)
{
	assert (valid() && fiSrc.valid());
	assert (where() == fiSrc.where());
	PixelF *ppxlfDst = (PixelF *) pixels();
	const PixelF *ppxlfSrc = fiSrc.pixels();
	Int i,area = where().area();
	
	for(i=0;i<area;i++)
		*ppxlfDst++ += *ppxlfSrc++;
	
	return *this;
}

own CFloatImage* CFloatImage::operator + (const CFloatImage& fi) const
{
	if (!valid () || !fi.valid ()) return NULL;
	assert (where () == fi.where ());
	CFloatImage* pfiSumRet = new CFloatImage (where ());
	PixelF* ppxlfRet = (PixelF*) pfiSumRet -> pixels ();
	const PixelF* ppxlfThis = pixels ();
	const PixelF* ppxlfFi = fi.pixels ();
	UInt area = where ().area ();
	for (UInt ip = 0; ip < area; ip++, ppxlfRet++, ppxlfThis++, ppxlfFi++)
		*ppxlfRet = *ppxlfThis + *ppxlfFi;
	return pfiSumRet;
}

own CFloatImage* CFloatImage::operator - (const CFloatImage& fi) const
{
	if (!valid () || !fi.valid ()) return NULL;
	assert (where () == fi.where ());
	CFloatImage* pfiSumRet = new CFloatImage (where ());
	PixelF* ppxlfRet = (PixelF*) pfiSumRet -> pixels ();
	const PixelF* ppxlfThis = pixels ();
	const PixelF* ppxlfFi = fi.pixels ();
	UInt area = where ().area ();
	for (UInt ip = 0; ip < area; ip++, ppxlfRet++, ppxlfThis++, ppxlfFi++)
		*ppxlfRet = *ppxlfThis - *ppxlfFi;
	return pfiSumRet;
}

own CFloatImage* CFloatImage::operator * (Float scale) const
{
	if (!valid ()) return NULL;
	CFloatImage* pfiSumRet = new CFloatImage (where ());
	PixelF* ppxlfRet = (PixelF*) pfiSumRet -> pixels ();
	const PixelF* ppxlfThis = pixels ();
	UInt area = where ().area ();
	for (UInt ip = 0; ip < area; ip++, ppxlfRet++, ppxlfThis++)
		*ppxlfRet = *ppxlfThis * scale;
	return pfiSumRet;
}

own CFloatImage* CFloatImage::operator * (const CTransform& tf) const
{
	CFloatImage* pfiRet = tf.transform_apply (*this);
	return pfiRet; 
}

own CFloatImage* CFloatImage::operator / (Float scale) const
{
	if (!valid ()) return NULL;
	assert (scale != .0f);
	CFloatImage* pfiSumRet = new CFloatImage (where ());
	PixelF* ppxlfRet = (PixelF*) pfiSumRet -> pixels ();
	const PixelF* ppxlfThis = pixels ();
	UInt area = where ().area ();
	for (UInt ip = 0; ip < area; ip++, ppxlfRet++, ppxlfThis++)
		*ppxlfRet = *ppxlfThis / scale;
	return pfiSumRet;
}

Void CFloatImage::operator = (const CFloatImage& fi)
{
	delete [] m_ppxlf;
	copyConstruct (fi, fi.where ());
}

Bool CFloatImage::operator == (const CFloatImage& fi) const
{
	if (fi.where () != where ())
		return FALSE;
	UInt area = where ().area ();
	const PixelF* ppxlf = fi.pixels ();
	const PixelF* ppxlfThis = pixels ();
	for (UInt ip = 0; ip < area; ip++, ppxlf++, ppxlfThis++)
		if (*ppxlf != *ppxlfThis)
			return FALSE;
	return TRUE;
}

Double CFloatImage::mse (const CFloatImage& fiCompare) const
{
	assert (fiCompare.where () == where ());
	Double sqr = 0;
	const PixelF* ppxlfThis = pixels ();
	const PixelF* ppxlfCompare = fiCompare.pixels ();
	UInt area = where ().area ();
	UInt uiNonTransp = 0;
	for (UInt i = 0; i < area; i++) {
			Double diffR = (Double) (*ppxlfThis - *ppxlfCompare);
			sqr += diffR * diffR;
			uiNonTransp++;
		ppxlfThis++;
		ppxlfCompare++;
	}
	sqr /= (Double) uiNonTransp;
	return sqr;
}

Double CFloatImage::mse (const CFloatImage& fiCompare, const CFloatImage& fiMsk) const
{
	assert (fiCompare.where () == where () && fiMsk.where () == where ());
	Double sqr = 0;
	const PixelF* ppxlfThis = pixels ();
	const PixelF* ppxlfCompare = fiCompare.pixels ();
	const PixelF* ppxlfMsk = fiMsk.pixels ();
	UInt area = where ().area ();
	UInt uiNonTransp = 0;
	for (UInt i = 0; i < area; i++) {
		if (*ppxlfMsk != transpValue) {
			Double diffR = (Double) (*ppxlfThis - ((Int) *ppxlfCompare));
			sqr += diffR * diffR;
			uiNonTransp++;
		}
		ppxlfThis++;
		ppxlfCompare++;
		ppxlfMsk++;
	}
	if (uiNonTransp == 0) 
		return 0;
	sqr /= (Double) uiNonTransp;
	return sqr;
}

Double CFloatImage::snr (const CFloatImage& fiCompare) const
{
	Double msError = 0;
	msError = mse (fiCompare);
	if (msError == 0.0)
		return 1000000.0;
	else 
		return (log10 (255 * 255 / msError) * 10.0);
}

Double CFloatImage::snr (const CFloatImage& fiCompare, const CFloatImage& fiMsk) const
{
	CFloatImage* pfiMskOp = NULL;
	Double msError = 0;
	if (&fiMsk == NULL) {
		pfiMskOp = new CFloatImage (where (), (PixelF) opaqueValue);		
		msError = mse (fiCompare, *pfiMskOp);
		delete pfiMskOp;
	}
	else
		msError = mse (fiCompare, fiMsk);
	if (msError == 0.0)
		return 1000000.0;
	else 
		return (log10 (255 * 255 / msError) * 10.0);
}


Void CFloatImage::vdlDump (const Char* fileName) const
{
	CVideoObjectPlane vop (where (), opaquePixel);
	CPixel* ppxl = (CPixel*) vop.pixels ();
	const PixelF* ppxlf = pixels ();
	UInt area = where ().area ();
	for (UInt ip = 0; ip < area; ip++, ppxl++, ppxlf++) {
		U8 vl = (U8) (checkrange (*ppxlf, 0.0f, 255.0f) + 0.5f);
		*ppxl = CPixel (vl, vl, vl, opaqueValue);
	}
	vop.vdlDump (fileName);
}
	
Void CFloatImage::dump (FILE* pf) const
{
	assert (pf != NULL);
	UInt area = where ().area ();
	U8* rguchPixelData = new U8 [where().area()];
	U8* puch = rguchPixelData;
	const PixelF* ppxlf = pixels ();
	for (UInt ip = 0; ip < area; ip++, puch++, ppxlf++) {
		U8 vl = (U8) checkrange (*ppxlf, 0.0f, 255.0f);
		*puch = vl;
	}
	fwrite (rguchPixelData, sizeof (U8), area, pf);
	delete [] rguchPixelData;
}
	
Void CFloatImage::txtDump (const Char* fileName) const
{
	FILE* pfTxt;
	const PixelF* ppxlf = pixels ();
	if (fileName != NULL)
		pfTxt = fopen (fileName, "w");
	else
		pfTxt = NULL;
	for (CoordI y = 0; y < where ().height (); y++) {
		for (CoordI x = 0; x < where ().width; x++) {
			if (pfTxt != NULL)
				fprintf (pfTxt, "%6.2f  ", *ppxlf);
			else
				printf ("%d  ", (Int) *ppxlf);
			ppxlf++;
		}
		if (pfTxt != NULL)
			fprintf (pfTxt, "\n");
		else
			printf ("\n");
	}
	if (pfTxt != NULL)
		fclose (pfTxt);

}


Void CFloatImage::txtDump(FILE *pf) const
{
	const PixelF *ppxlf = pixels();
	for (CoordI y = 0; y < where ().height (); y++) {
		for (CoordI x = 0; x < where ().width; x++) {
			fprintf (pf, "%6.2f ", *ppxlf);
			ppxlf++;
		}
		fprintf (pf,"\n");
	}
	fprintf (pf,"\n");
}

Void CFloatImage::txtDumpMask(FILE *pf) const
{
	const PixelF *ppxlf = pixels();
	for (CoordI y = 0; y < where ().height (); y++) {
		for (CoordI x = 0; x < where ().width; x++) {
			if(*ppxlf==transpValue)
				fprintf (pf, "..");
			else
				fprintf (pf, "[]");
			ppxlf++;
		}
		fprintf (pf,"\n");
	}
	fprintf (pf,"\n");
}

Void CFloatImage::quantize (Int stepsize, Bool dpcm)
{
	CoordI x, y;
	const Int owidth = where ().width;
	const CoordI left = where ().left;
	const CoordI top = where ().top;
	const CoordI right = where ().right;
	const CoordI bottom = where ().bottom;
	PixelF* px = (PixelF*) pixels ();
	if (TRUE){
		//Float halfStepsize = (Float) stepsize / 2;
		for (y = top; y != bottom; y++) {
			for (x = left; x != right; x++) {
				//Float round = (*px >= 0) ? halfStepsize : -halfStepsize;
				//*px++ = (Float) ((Int) ((*px + round) / stepsize));
				*px = (Float) ((Int) (*px / stepsize));
				px++;
			}
		}
	}
	else {
		Float halfStepsize = (Float) stepsize / 4;
		//Float halfStepsize = (Float) stepsize / 2;
		for (y = top; y != bottom; y++) {
			for (x = left; x != right; x++) {
				Float round = (*px >= 0) ? halfStepsize : -halfStepsize;
				*px = (Float) ((Int) ((*px - round) / stepsize));
				px++; // wmay
				//*px++ = (Float) ((Int) ((*px + round) / stepsize));
				//*px++ = (Float) ((Int) ((*px ) / stepsize));
			}
		}
	}

	if (dpcm) { // zig-zag dpcm
		CFloatImage* buf1 = new CFloatImage (*this);
		for (y = top; y != bottom; y++) {
			if (((y - top) & 1) == 0) { // even
				const Float* pbuf = buf1 -> pixels (left, y);
				PixelF* psig = (PixelF*) pixels (left, y);
				*psig++ = (y != top) ? *pbuf - *(pbuf - owidth) : *pbuf;
				pbuf++;
				for (x = left + 1; x != right; x++) {
					*psig = *pbuf - *(pbuf - 1);
					psig++;
					pbuf++;
				}
			}
			else {
				const PixelF* pbuf = buf1 -> pixels (right - 1, y);
				PixelF* psig = (PixelF*) pixels (right - 1, y);
				*psig-- = *pbuf - *(pbuf - owidth);
				pbuf--;
				for (x = right - 2; x >= left; x--) {
					*psig = *pbuf - *(pbuf + 1);
					psig--;
					pbuf--;
				}
			}
		}
		delete buf1;
	}
}


Void CFloatImage::deQuantize (Int stepsize, Bool dpcm)
{
	CoordI x, y;
	const Int owidth = where ().width;
	const CoordI left = where ().left;
	const CoordI top = where ().top;
	const CoordI right = where ().right;
	const CoordI bottom = where ().bottom;
	if (dpcm) {
		for (y = top; y != bottom; y++) {
			if (((y - top) & 1) == 0) {
				PixelF* psig = (PixelF*) pixels (left, y);
				if (y != top)
					*psig += *(psig - owidth);
				psig++;
				for (x = left + 1; x != right; x++) {
					*psig += *(psig - 1);
					psig++;
				}
			}
			else {
				PixelF* psig = (PixelF*) pixels (right - 1, y);
				*psig += *(psig - owidth);
				psig--;
				for (x = right - 2; x >= left; x--) {
					*psig += *(psig + 1);
					psig--;
				}
			}
		}
	}
	Float halfStepsize ;
	if ( stepsize % 4 == 0)
		halfStepsize = (Float) stepsize / 2 - 1;
	else
		halfStepsize = (Float) stepsize / 2;

	PixelF* px = (PixelF*) pixels ();
	for (y = top; y != bottom; y++) {
		for (x = left; x != right; x++){
			//Int round = (*px >= 0) ? (stepsize >> 1) : -(stepsize >> 1);
			Float round = (*px >= 0) ? (halfStepsize) : -(halfStepsize);
			*px = (*px == 0)? *px: (*px * (Float) stepsize + round);
			px++;
			//*px++ *= (Float) stepsize;
		}
	}
}

Void CFloatImage::roundNearestInt()
{
	Int i;
	PixelF fVal,*ppxlf = (PixelF *)pixels();

	for(i=where().area();i;i--,ppxlf++)
	{
		fVal = *ppxlf;
		if(fVal>=0)
			*ppxlf = (PixelF)floor(fVal+0.5);
		else
			*ppxlf = (PixelF)ceil(fVal-0.5);
	}
}

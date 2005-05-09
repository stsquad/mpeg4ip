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


CIntImage::~CIntImage ()
{
	delete [] m_ppxli;
	m_ppxli = NULL;
}

Void CIntImage::allocate (const CRct& r, PixelI pxli) 
{
	m_rc = r;
	delete [] m_ppxli, m_ppxli = NULL;

	// allocate pixels and initialize
	if (m_rc.empty ()) return;
	m_ppxli = new PixelI [m_rc.area ()];
	assert (m_ppxli);
	for (UInt ic = 0; ic < where ().area (); ic++)
		m_ppxli [ic] = pxli;
}

Void CIntImage::allocate (const CRct& r) 
{
	m_rc = r;
	delete [] m_ppxli, m_ppxli = NULL;

	// allocate pixels and initialize
	if (m_rc.empty ()) return;
	m_ppxli = new PixelI [m_rc.area ()];
	assert (m_ppxli);
}

Void CIntImage::copyConstruct (const CIntImage& ii, const CRct& rct) 
{
	CRct r = rct;
	if (!r.valid()) 
		r = ii.where ();
	if (!ii.valid () || (!ii.m_rc.empty () && ii.m_ppxli == NULL))
		assert (FALSE);
	allocate (r, (PixelI) 0);
	if (!valid ()) return; 

	// Copy data
	if (r == ii.where ())
		memcpy (m_ppxli, ii.pixels (), m_rc.area () * sizeof (PixelI));
	else {
		r.clip (ii.where ()); // find the intersection
		CoordI x = r.left; // copy pixels
		Int cbLine = r.width * sizeof (PixelI);
		PixelI* ppxl = (PixelI*) pixels (x, r.top);
		const PixelI* ppxlFi = ii.pixels (x, r.top);
		Int widthCurr = where ().width;
		Int widthFi = ii.where ().width;
		for (CoordI y = r.top; y < r.bottom; y++) {
			memcpy (ppxl, ppxlFi, cbLine);
			ppxl += widthCurr;
			ppxlFi += widthFi;
		}
	}
}

Void CIntImage::swap (CIntImage& ii) 
{
	assert (this && &ii);
	CRct rcT = ii.m_rc; 
	ii.m_rc = m_rc; 
	m_rc = rcT; 
	PixelI* ppxliT = ii.m_ppxli; 
	ii.m_ppxli = m_ppxli; 
	m_ppxli = ppxliT; 
}

CIntImage::CIntImage (const CIntImage& ii, const CRct& r) : m_ppxli (NULL)
{
	copyConstruct (ii, r);
}

CIntImage::CIntImage (const CRct& r, PixelI px) : m_ppxli (NULL)
{
	allocate (r, px);
}

CIntImage::CIntImage (const CVideoObjectPlane& vop, RGBA comp) : m_ppxli (NULL)
{
	if (!vop.valid ()) return;
	
	allocate (vop.where ());
	const CPixel* ppxl = vop.pixels ();
	for (UInt ip = 0; ip < where ().area (); ip++)
		m_ppxli [ip] = ppxl [ip].pxlU.color [comp];
}

CIntImage::CIntImage (
	const Char* pchFileName, 
	UInt ifr,
	const CRct& rct,
	UInt nszHeader
) : m_ppxli (NULL), m_rc (rct)
{
	assert (!rct.empty ());
	allocate (rct);
	UInt uiArea = rct.area ();
	
	// read data from a file
	FILE* fpSrc = fopen (pchFileName, "rb");
	assert (fpSrc != NULL);
	fseek (fpSrc, nszHeader + ifr * sizeof (U8) * uiArea, SEEK_SET);
	for (UInt ip = 0; ip < uiArea; ip++)
		m_ppxli [ip] = getc (fpSrc);
	fclose (fpSrc);
}

CIntImage::CIntImage (const Char* vdlFileName) : m_ppxli (NULL)// read from a VM file.
{
	CVideoObjectPlane vop (vdlFileName);
	allocate (vop.where ());
	const CPixel* ppxl = vop.pixels ();
	for (UInt ip = 0; ip < where ().area (); ip++)
		m_ppxli [ip] = ppxl [ip].pxlU.rgb.r;
}

Void CIntImage::where (const CRct& r) 
{
	if (!valid ()) return; 
	if (where () == r) return; 
	CIntImage* pii = new CIntImage (*this, r);
	swap (*pii);
	delete pii;
}


CRct CIntImage::boundingBox (const PixelI pxliOutsideColor) const
{
	if (allValue ((PixelI) pxliOutsideColor))	
		return CRct ();
	CoordI left = where ().right - 1;
	CoordI top = where ().bottom - 1;
	CoordI right = where ().left;
	CoordI bottom = where ().top;
	const PixelI* ppxliThis = pixels ();
	for (CoordI y = where ().top; y < where ().bottom; y++) {
		for (CoordI x = where ().left; x < where ().right; x++) {
			if (*ppxliThis != (PixelI) pxliOutsideColor) {
				left = min (left, x);
				top = min (top, y);
				right = max (right, x);
				bottom = max (bottom, y);
			}								
			ppxliThis++;
		}
	}
	right++;
	bottom++;
	return (CRct (left, top, right, bottom));				
}


Int CIntImage::mean () const
{
	if (where ().empty ()) 
		return 0;
	Int meanRet = 0;
	PixelI* ppxli = (PixelI*) pixels ();
	UInt area = where ().area ();
	for (UInt ip = 0; ip < area; ip++)
		meanRet += ppxli [ip];
	return (Int) (meanRet / area);
}

Int CIntImage::mean (const CIntImage* piiMsk) const
{
	assert (where () == piiMsk -> where ()); // no compute if rects are different
	if (where ().empty ()) return 0;
	Int meanRet = 0;
	PixelI* ppxli = (PixelI*) pixels ();
	const PixelI* ppxliMsk = piiMsk -> pixels ();
	UInt area = where ().area ();
	UInt uiNumNonTransp = 0;
	for (UInt ip = 0; ip < area; ip++) {
		if (ppxliMsk [ip] != transpValue) {
			uiNumNonTransp++;
			meanRet += ppxli [ip];
		}
	}
	return (Int) (meanRet / uiNumNonTransp);
}

Int CIntImage::sumAbs (const CRct& rct) const 
{
	CRct rctToDo = (!rct.valid ()) ? where () : rct;
	Int intRet = 0;
	if (rctToDo == where ())	{	
		const PixelI* ppxli = pixels ();
		UInt area = where ().area ();
		for (UInt ip = 0; ip < area; ip++, ppxli++)
			intRet += (*ppxli);
	}
	else	{
		Int width = where ().width;
		const PixelI* ppxliRow = pixels (rct.left, rct.top);
		for (CoordI y = rctToDo.top; y < rctToDo.bottom; y++) {
			const PixelI* ppxli = ppxliRow;
			for (CoordI x = rctToDo.left; x < rctToDo.right; x++, ppxli++)
				intRet += abs (*ppxli);
			ppxliRow += width;
		}
	}
	return intRet;
}

Int CIntImage::sumDeviation (const CIntImage* piiMsk) const // sum of first-order deviation
{
	Int meanPxl = mean (piiMsk);
	Int devRet = 0;
	PixelI* ppxli = (PixelI*) pixels ();
	const PixelI* ppxliMsk = piiMsk -> pixels ();
	UInt area = where ().area ();
	for (UInt ip = 0; ip < area; ip++) {
		if (ppxliMsk [ip] != transpValue)
			devRet += abs (meanPxl - ppxli [ip]);
	}
	return devRet;
}


Int CIntImage::sumDeviation () const // sum of first-order deviation
{
	Int meanPxl = mean ();
	Int devRet = 0;
	PixelI* ppxli = (PixelI*) pixels ();
	UInt area = where ().area ();
	for (UInt ip = 0; ip < area; ip++)
		devRet += abs (meanPxl - ppxli [ip]);
	return devRet;
}


Bool CIntImage::allValue (Int intVl, const CRct& rct) const
{
	CRct rctToDo = (!rct.valid ()) ? where () : rct;
	if (rctToDo == where ()) {	
		const PixelI* ppxli = pixels ();
		UInt area = where ().area ();
		for (UInt ip = 0; ip < area; ip++) {
			if (ppxli [ip] != intVl)
				return FALSE;
		}
	}
	else {
		Int width = where ().width;
		const PixelI* ppxli = pixels (rct.left, rct.top);
		for (CoordI y = rctToDo.top; y < rctToDo.bottom; y++) {
			const PixelI* ppxliRow = ppxli;
			for (CoordI x = rctToDo.left; x < rctToDo.right; x++, ppxliRow++) {
				if (*ppxliRow != intVl)
					return FALSE;
			}
			ppxli += width;
		}
	}
	return TRUE;
}

Bool CIntImage::biLevel (const CRct& rct) const
{
	CRct rctToDo = (!rct.valid ()) ? where () : rct;
	if (rctToDo == where ())	{	
		const PixelI* ppxli = pixels ();
		UInt area = where ().area ();
		for (UInt ip = 0; ip < area; ip++) {
			if (ppxli [ip] != opaqueValue && ppxli [ip] != transpValue)
				return FALSE;
		}
	}
	else {
		Int width = where ().width;
		const PixelI* ppxli = pixels (rct.left, rct.top);
		for (CoordI y = rctToDo.top; y < rctToDo.bottom; y++) {
			const PixelI* ppxliRow = ppxli;
			for (CoordI x = rctToDo.left; x < rctToDo.right; x++, ppxliRow++) {
				if (*ppxliRow != opaqueValue && *ppxliRow != transpValue)
					return FALSE;
			}
			ppxli += width;
		}
	}
	return TRUE;
}

CRct CIntImage::whereVisible () const
{
	CoordI left = where ().right - 1;
	CoordI top = where ().bottom - 1;
	CoordI right = where ().left;
	CoordI bottom = where ().top;
	const PixelI* ppxliThis = pixels ();
	for (CoordI y = where ().top; y < where ().bottom; y++) {
		for (CoordI x = where ().left; x < where ().right; x++) {
			if (*ppxliThis != transpValue) {
				left = min (left, x);
				top = min (top, y);
				right = max (right, x);
				bottom = max (bottom, y);
			}								
			ppxliThis++;
		}
	}
	right++;
	bottom++;
	return CRct (left, top, right, bottom);
}


Bool CIntImage::atLeastOneValue (Int ucVl, const CRct& rct) const
{
	CRct rctRegionOfInterest = (!rct.valid ()) ? where () : rct;
	assert (rctRegionOfInterest <= where ());
	if (rctRegionOfInterest == where ()) {
		const PixelI* ppxli = pixels ();
		UInt area = where ().area ();
		for (UInt ip = 0; ip < area; ip++) {
			if (ppxli [ip] == ucVl)
				return TRUE;
		}
	}
	else {
		Int width = where ().width;
		const PixelI* ppxli = pixels (rctRegionOfInterest.left, rctRegionOfInterest.top);
		for (CoordI y = rctRegionOfInterest.top; y < rctRegionOfInterest.bottom; y++) {
			const PixelI* ppxliRow = ppxli;
			for (CoordI x = rctRegionOfInterest.left; x < rctRegionOfInterest.right; x++, ppxliRow++) {
				if (*ppxliRow == ucVl)
					return TRUE;
			}
			ppxli += width;
		}
	}
	return FALSE;
}


UInt CIntImage::numPixelsNotValued (Int ucVl, const CRct& rct) const // number of pixels not valued vl in region rct
{
	CRct rctInterest = (!rct.valid ()) ? where () : rct;
	assert (rctInterest <= where ());
	UInt nRet = 0;
	if (rctInterest == where ()) {
		const PixelI* ppxli = pixels ();
		UInt area = where ().area ();
		for (UInt ip = 0; ip < area; ip++) {
			if (ppxli [ip] != ucVl)
				nRet++;
		}
	}
	else {
		Int width = where ().width;
		const PixelI* ppxli = pixels (rctInterest.left, rctInterest.top);
		for (CoordI y = rctInterest.top; y < rctInterest.bottom; y++) {
			const PixelI* ppxliRow = ppxli;
			for (CoordI x = rctInterest.left; x < rctInterest.right; x++, ppxliRow++) {
				if (*ppxliRow != ucVl)
					nRet++;
			}
			ppxli += width;
		}
	}

	return nRet;
}


Void CIntImage::setRect (const CRct& rct)
{
	assert (rct.area () == m_rc.area ());
	m_rc = rct;
}

Void CIntImage::threshold (Int ucThresh)
{
	PixelI* ppxlThis = (PixelI*) pixels ();
	UInt area = where ().area ();
	for (UInt id = 0; id < area; id++) {
		if (ppxlThis [id] < ucThresh)
			ppxlThis [id] = 0;
	}
}

Void CIntImage::binarize (Int ucThresh)
{
	PixelI* ppxliThis = (PixelI*) pixels ();
	UInt area = where ().area ();
	for (UInt id = 0; id < area; id++/* wmay, ppxliThis */) {
		if (*ppxliThis < ucThresh)
			*ppxliThis = transpValue;
		else
			*ppxliThis = opaqueValue;
	}
}


Void CIntImage::checkRange (Int ucMin, Int ucMax)
{
	PixelI* ppxliThis = (PixelI*) pixels ();
	UInt area = where ().area ();
	for (UInt id = 0; id < area; id++, ppxliThis++)
		*ppxliThis = checkrange (*ppxliThis, ucMin, ucMax);
}

own CIntImage* CIntImage::decimate (UInt rateX, UInt rateY) const
{
	const CoordI left = where ().left / (CoordI) rateX;
	const CoordI top = where ().top / (CoordI)  rateY;
	const CoordI right = 
		(where ().right >= 0) ? (where ().right + (CoordI) rateX - 1) / (CoordI) rateX :
		(where ().right - (CoordI) rateX + 1) / (CoordI) rateX;
	const CoordI bottom = 
		(where ().bottom >= 0) ? (where ().bottom + (CoordI) rateX - 1) / (CoordI) rateY :
		(where ().bottom - (CoordI) rateX + 1) / (CoordI) rateY;

	CIntImage* piiRet = new CIntImage (CRct (left, top, right, bottom));
	PixelI* ppxliRet = (PixelI*) piiRet -> pixels ();
	const PixelI* ppxliOrgY = pixels ();
	Int skipY = rateY * where ().width;
	
	for (CoordI y = top; y < bottom; y++) {
		const PixelI* ppxliOrgX = ppxliOrgY;
		for (CoordI x = left; x < right; x++) {
			*ppxliRet++ = *ppxliOrgX;
			ppxliOrgX += rateX;
		}
		ppxliOrgY += skipY;
	}
	return piiRet;
}

own CIntImage* CIntImage::decimateBinaryShape (UInt rateX, UInt rateY) const
{
	const CoordI left = where ().left / (CoordI) rateX;
	const CoordI top = where ().top / (CoordI) rateY;
	Int roundR = (where ().right >= 0) ? rateX - 1 : 1 - rateX;
	Int roundB = (where ().bottom >= 0) ? rateY - 1 : 1 - rateY;
	const CoordI right = (where ().right + roundR) / (CoordI) rateX;
	const CoordI bottom = (where ().bottom + roundB) / (CoordI) rateY;

	CIntImage* piiRet = new CIntImage (CRct (left, top, right, bottom));
	PixelI* ppxliRet = (PixelI*) piiRet -> pixels ();
	const PixelI* ppxliOrgY = pixels ();
	Int skipY = rateY * where ().width;
	CoordI x, y;
	CoordI iXOrigLeft, iYOrigTop;	//left top of a sampling square

	for (y = top, iYOrigTop = where().top; y < bottom; y++, iYOrigTop += rateY) {
		const PixelI* ppxliOrgX = ppxliOrgY;
		for (x = left, iXOrigLeft = where().left; x < right; x++, iXOrigLeft += rateX) {
			CoordI iXOrig, iYOrig;						//for scanning of the sampling square
			const PixelI* ppxliOrigScanY = ppxliOrgX;	//same
			*ppxliRet = transpValue;
			for (iYOrig = iYOrigTop; iYOrig < (iYOrigTop + (Int) rateY); iYOrig++)	{
				if (iYOrig >= where().bottom || *ppxliRet == opaqueValue)
					break;
				const PixelI* ppxliOrigScanX = ppxliOrigScanY; //for scan also
				for (iXOrig = iXOrigLeft; iXOrig < (iXOrigLeft + (Int) rateX); iXOrig++)	{
					if (iXOrig >= where().right)
						break;
					assert (*ppxliOrigScanX == transpValue || *ppxliOrigScanX == opaqueValue);
					if (*ppxliOrigScanX == opaqueValue) {
						*ppxliRet = opaqueValue;
						break;
					}
					ppxliOrigScanX++;
				}
				ppxliOrigScanY += where().width;
			}

			assert (*ppxliRet == transpValue || *ppxliRet == opaqueValue);
			ppxliRet++;
			ppxliOrgX += rateX;
		}
		ppxliOrgY += skipY;
	}
	return piiRet;
}

own CIntImage* CIntImage::zoomup (UInt rateX, UInt rateY) const
{
	const CoordI left = where ().left * rateX; // left-top coordinate remain the same
	const CoordI top = where ().top * rateY;
	const CoordI right = where ().right * rateX;
	const CoordI bottom = where ().bottom * rateY;

	CIntImage* piiRet = new CIntImage (CRct (left, top, right, bottom));
	PixelI* ppxlRet = (PixelI*) piiRet -> pixels ();
	for (CoordI y = top; y < bottom; y++) {
		for (CoordI x = left; x < right; x++)
			*ppxlRet++ = pixel ((CoordI) (x / rateX), (CoordI) (y / rateY));
	}
	return piiRet;
}

own CIntImage* CIntImage::expand (UInt rateX, UInt rateY) const // expand by putting zeros in between
{
	const CoordI left = where ().left * rateX; // left-top coordinate remain the same
	const CoordI top = where ().top * rateY;
	const CoordI right = where ().right * rateX;
	const CoordI bottom = where ().bottom * rateY;

	CIntImage* piiRet = new CIntImage (CRct (left, top, right, bottom));
	PixelI* ppxlRet = (PixelI*) piiRet -> pixels ();
	const PixelI* ppxlThis = pixels ();
	for (CoordI y = top; y < bottom; y++) {
		for (CoordI x = left; x < right; x++) {
			if (x % rateX == 0 && y % rateY == 0) 
				*ppxlRet++ = *ppxlThis++;
			else
				*ppxlRet++ = 0;
		}
	}
	return piiRet;
}

own CIntImage* CIntImage::biInterpolate () const // bilinearly interpolate the vframe
{
	const CoordI left = where ().left << 1; // left-top coordinate remain the same
	const CoordI top = where ().top << 1;
	const CoordI right = where ().right << 1;
	const CoordI bottom = where ().bottom << 1;
	const CoordI width = right - left;

	CoordI x, y;
	CIntImage* piiRet = new CIntImage (CRct (left, top, right, bottom));
	PixelI* ppxliRet = (PixelI*) piiRet -> pixels ();
	const PixelI* ppxli = pixels ();
	const CoordI right1 = right - 2;
	for (y = top; y < bottom; y += 2) { // x-direction interpolation
		for (x = left; x < right1; x += 2) {
			*ppxliRet++ = *ppxli++;
			*ppxliRet++ = (*ppxli + *(ppxli - 1) + 1) >> 1;
		}
		*ppxliRet++ = *ppxli;
		*ppxliRet++ = *ppxli++; // the last pixel of every row do not need average
		ppxliRet += width;
	}
	ppxliRet = (PixelI*) piiRet -> pixels ();
	ppxliRet += width; // start from the second row
	const CoordI width2 = width << 1;
	const CoordI bottom1 = bottom - 1;
	for (x = left; x < right; x++) { // y-direction interpolation
		PixelI* ppxliRett = ppxliRet++;
		for (y = top + 1; y < bottom1; y += 2) {
			*ppxliRett = (*(ppxliRett - width) + *(ppxliRett + width) + 1) >> 1;
			ppxliRett += width2;
		}
		*ppxliRett = *(ppxliRett - width); // the last pixel of every column do not need average
	}
	return piiRet;
}

own CIntImage* CIntImage::downsampleForSpatialScalability () const
{
	static Int rgiFilterVertical[13] = {2, 0, -4, -3, 5, 19, 26, 19, 5, -3, -4, 0, 2};
	static Int rgiFilterHorizontal[4] = {5, 11, 11, 5};
	Int iWidthSrc = where (). width;
	Int iHeightSrc = where (). height ();
	assert (iWidthSrc % 2 == 0 && iHeightSrc % 2 == 0);
	Int iWidthDst = iWidthSrc / 2;
	Int iHeightDst = iHeightSrc / 2;
	CIntImage* piiBuffer = new CIntImage (CRct (0, 0, iWidthSrc, iHeightDst));
	CIntImage* piiRet = new CIntImage (CRct (0, 0, iWidthDst, iHeightDst));
	assert (piiBuffer != NULL);
	assert (piiRet != NULL);

	//filter and downsample vertically
	const PixelI* ppxliSrc;	
	const PixelI* ppxliColumnHeadSrc = pixels ();
	PixelI* ppxliDst = (PixelI*) piiBuffer->pixels ();
	PixelI* ppxliColumnHeadDst = (PixelI*) piiBuffer->pixels ();
	Int i, j, k;
	for (i = 0; i < iWidthSrc; i++)	{
		ppxliSrc = ppxliColumnHeadSrc;	
		ppxliDst = ppxliColumnHeadDst;	
		for (j = 0; j < iHeightDst; j++)	{
			k = j * 2;         
			const PixelI* ppxliMinusOne = ( k < 1 ) ? ppxliSrc : ppxliSrc - iWidthSrc;		
			const PixelI* ppxliMinusTwo = ( k < 2 ) ? ppxliSrc : ppxliMinusOne - iWidthSrc;
			const PixelI* ppxliMinusThree = ( k < 3 ) ? ppxliSrc : ppxliMinusTwo - iWidthSrc;
			const PixelI* ppxliMinusFour = ( k < 4 ) ? ppxliSrc : ppxliMinusThree - iWidthSrc;
			const PixelI* ppxliMinusFive = ( k < 5 ) ? ppxliSrc : ppxliMinusFour - iWidthSrc;
			const PixelI* ppxliMinusSix = ( k < 6 ) ? ppxliSrc : ppxliMinusFive - iWidthSrc;
			const PixelI* ppxliPlusOne = ( k >= iHeightSrc - 1) ? ppxliSrc : ppxliSrc + iWidthSrc;
			const PixelI* ppxliPlusTwo = ( k >= iHeightSrc - 2) ? ppxliPlusOne : ppxliPlusOne + iWidthSrc;
			const PixelI* ppxliPlusThree = ( k >= iHeightSrc - 3) ? ppxliPlusTwo : ppxliPlusTwo + iWidthSrc;
			const PixelI* ppxliPlusFour = ( k >= iHeightSrc - 4) ? ppxliPlusThree : ppxliPlusThree + iWidthSrc;
			const PixelI* ppxliPlusFive = ( k >= iHeightSrc - 5) ? ppxliPlusFour : ppxliPlusFour + iWidthSrc;
			const PixelI* ppxliPlusSix = ( k >= iHeightSrc - 6) ? ppxliPlusFive : ppxliPlusFive + iWidthSrc;
			*ppxliDst = checkrange (
				(Int) ((*ppxliMinusSix * rgiFilterVertical [0] +
				*ppxliMinusFive * rgiFilterVertical [1] +
				*ppxliMinusFour * rgiFilterVertical [2] +
				*ppxliMinusThree * rgiFilterVertical [3] +
				*ppxliMinusTwo * rgiFilterVertical [4] +
				*ppxliMinusOne * rgiFilterVertical [5] +
				*ppxliSrc * rgiFilterVertical [6] +
				*ppxliPlusOne * rgiFilterVertical [7] +
				*ppxliPlusTwo * rgiFilterVertical [8] +
				*ppxliPlusThree * rgiFilterVertical [9] +
				*ppxliPlusFour * rgiFilterVertical [10] +
				*ppxliPlusFive * rgiFilterVertical [11] +
				*ppxliPlusSix * rgiFilterVertical [12] + 32) >> 6),
				0, 255
			);
			ppxliSrc += 2 * iWidthSrc;
			ppxliDst += iWidthSrc;	
		}
		ppxliColumnHeadSrc++;
		ppxliColumnHeadDst++;
	} 
	//filter and downsample horizontally
	ppxliSrc = piiBuffer->pixels ();	
	ppxliDst = (PixelI*) piiRet->pixels ();
	for (j = 0; j < iHeightDst; j++)	{
		for (i = 0; i < iWidthDst; i++)	{
			k = i * 2;         
			const PixelI* ppxliMinusOne = ( k < 1 ) ? ppxliSrc : ppxliSrc - 1;		
			const PixelI* ppxliPlusOne = ( k >= iWidthSrc - 1) ? ppxliSrc : ppxliSrc + 1;
			const PixelI* ppxliPlusTwo = ( k >= iWidthSrc - 2) ? ppxliSrc : ppxliSrc + 2;
			*ppxliDst = checkrange (
				(Int) ((*ppxliMinusOne * rgiFilterHorizontal [0] +
				*ppxliSrc * rgiFilterHorizontal [1] +
				*ppxliPlusOne * rgiFilterHorizontal [2] +
				*ppxliPlusTwo * rgiFilterHorizontal [3] + 16) >> 5),
				0, 255
			);
			ppxliSrc += 2;	
			ppxliDst++;	
		}
	} 
	delete piiBuffer;
	return piiRet;
}

own CIntImage* CIntImage::upsampleForSpatialScalability () const
{
	CRct rctDst = where () * 2;
	Int iWidthSrc = where (). width;
	Int iHeightSrc = where (). height ();
	Int iHeightDst = iHeightSrc * 2;
	CIntImage* piiBuffer = new CIntImage (CRct (where().left, rctDst.top, where().right, rctDst.bottom));
	CIntImage* piiRet = new CIntImage (CRct (rctDst.left, rctDst.top, rctDst.right, rctDst.bottom));

	//upsample vertically
	const PixelI* ppxliSrc;	
	const PixelI* ppxliSrcPlusOne;	
	const PixelI* ppxliColumnHeadSrc = pixels ();
	PixelI* ppxliDst;
	PixelI* ppxliColumnHeadDst = (PixelI*) piiBuffer->pixels ();
	Int i, j;
	for (i = 0; i < iWidthSrc; i++)	{
		ppxliSrc = ppxliColumnHeadSrc;	
		ppxliSrcPlusOne = ppxliSrc + iWidthSrc;
		ppxliDst = ppxliColumnHeadDst;	
		for (j = 0; j < iHeightSrc; j++)	{
			*ppxliDst = checkrange (
				(Int) ((*ppxliSrc * 3 + *ppxliSrcPlusOne + 2) >> 2),
				0, 255
			);
			ppxliDst += iWidthSrc;
			*ppxliDst = checkrange ( 
				(Int) ((*ppxliSrc + *ppxliSrcPlusOne * 3 + 2) >> 2),
				0, 255
			);
			ppxliDst += iWidthSrc;
			ppxliSrc += iWidthSrc;
			ppxliSrcPlusOne = (j >= iHeightSrc - 2) ? ppxliSrc : ppxliSrc + iWidthSrc;
		}
		ppxliColumnHeadSrc++;
		ppxliColumnHeadDst++;
	} 

	//upsample horizontally	
	ppxliSrc = piiBuffer->pixels ();
	ppxliDst = (PixelI*) piiRet->pixels ();
	for (j = 0; j < iHeightDst; j++)	{
		ppxliSrcPlusOne = ppxliSrc + 1;
		for (i = 0; i < iWidthSrc; i++)	{
			*ppxliDst = checkrange ( 
				(Int) ((*ppxliSrc * 3 + *ppxliSrcPlusOne + 2) >> 2),
				0, 255
			);
			ppxliDst++;
			*ppxliDst = checkrange ( 
				(Int) ((*ppxliSrc + *ppxliSrcPlusOne * 3 + 2) >> 2),
				0, 255
			);
			ppxliDst++;
			ppxliSrc++;
			ppxliSrcPlusOne = (i >= iWidthSrc - 2) ? ppxliSrc : ppxliSrc + 1;
		}
	} 
	delete piiBuffer;
	return piiRet;
}

own CIntImage* CIntImage::biInterpolate (UInt accuracy) const // bilinearly interpolate the vframe
{
	const CoordI left = where ().left * accuracy;
	const CoordI top = where ().top * accuracy;
	const CoordI right = where ().right * accuracy;
	const CoordI bottom = where ().bottom * accuracy;

	CIntImage* piiRet = new CIntImage (CRct (left, top, right, bottom));
	PixelI* ppxliRet = (PixelI*) piiRet -> pixels ();
	for (CoordI y = top; y < bottom; y++) { // x-direction interpolation
		for (CoordI x = left; x < right; x++) {
			*ppxliRet = pixel (x, y, accuracy);
			ppxliRet++;
		}
	}
	return piiRet;
}

own CIntImage* CIntImage::transpose () const
{
	CRct rctDst = where ();
	rctDst.transpose ();
	CIntImage* piiDst = new CIntImage (rctDst);
	const PixelI* ppxlSrc = pixels ();
	PixelI* ppxlDstRow = (PixelI*) piiDst->pixels ();
	PixelI* ppxlDst;
	UInt height = where ().height ();
	for (CoordI iy = where ().top; iy < where ().bottom; iy++)	{
		ppxlDst = ppxlDstRow;
		for (CoordI ix = where ().left; ix < where ().right; ix++)	{
			*ppxlDst = *ppxlSrc++;
			ppxlDst += height;
		}
		ppxlDstRow++;
	}
	return piiDst;
}

own CIntImage* CIntImage::warp (const CAffine2D& aff) const // affine warp
{
	CSiteD stdLeftTopWarp = aff * CSiteD (where ().left, where ().top);
	CSiteD stdRightTopWarp = aff * CSiteD (where ().right, where ().top);
	CSiteD stdLeftBottomWarp = aff * CSiteD (where ().left, where ().bottom);
	CSiteD stdRightBottomWarp = aff * CSiteD (where ().right, where ().bottom);
	CRct rctWarp (stdLeftTopWarp, stdRightTopWarp, stdLeftBottomWarp, stdRightBottomWarp);

	CIntImage* piiRet = new CIntImage (rctWarp);
	PixelI* ppxliRet = (PixelI*) piiRet -> pixels ();
	CAffine2D affInv = aff.inverse ();
	for (CoordI y = rctWarp.top; y < rctWarp.bottom; y++) {
		for (CoordI x = rctWarp.left; x < rctWarp.right; x++) {
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
				*ppxliRet = pixel (src);
			ppxliRet++;
		}
	}
	return piiRet;
}


own CIntImage* CIntImage::warp (const CPerspective2D& persp) const // perspective warp
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

	CIntImage* piiRet = new CIntImage (rctWarp);
	PixelI* ppxliRet = (PixelI*) piiRet -> pixels ();
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
				*ppxliRet = pixel (src);
			ppxliRet++;
		}
	}
	return piiRet;
}

own CIntImage* CIntImage::warp (const CPerspective2D& persp, const CRct& rctWarp) const // perspective warp
{
	CIntImage* piiRet = new CIntImage (rctWarp);
	PixelI* ppxliRet = (PixelI*) piiRet -> pixels ();
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
				*ppxliRet = pixel (src);
			ppxliRet++;
		}
	}
	return piiRet;
}

own CIntImage* CIntImage::warp (const CPerspective2D& persp, const CRct& rctWarp, const UInt accuracy) const // perspective warp
{
	CIntImage* piiRet = new CIntImage (rctWarp);
	PixelI* ppxliRet = (PixelI*) piiRet -> pixels ();
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
				*ppxliRet = pixel (src, accuracy);
			ppxliRet++;
		}
	}
	return piiRet;
}

PixelI CIntImage::pixel (CoordD x, CoordD y) const
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

	const PixelI lt = pixel (left, top);
	const PixelI rt = pixel (right, top);
	const PixelI lb = pixel (left, bottom);
	const PixelI rb = pixel (right, bottom);
	const Double distX = x - left;
	const Double distY = y - top;
	Double x01 = distX * (rt - lt) + lt; // use p.59's notation (Wolberg, Digital Image Warping)
	Double x23 = distX * (rb - lb) + lb;
	PixelI pxlRet = checkrange ((Int) (x01 + (x23 - x01) * distY + .5), 0, 255);
	return pxlRet;
}

PixelI CIntImage::pixel (CoordI x, CoordI y, UInt accuracy) const	// accuracy indicates the 2/4/8/16 quantization
{
	CoordI left = (CoordI) floor ((CoordD) x / (CoordD) accuracy); // find the coordinates of the four corners
	CoordI wLeft = where ().left, wRight1 = where ().right - 1, wTop = where ().top, wBottom1 = where ().bottom - 1;
	left = checkrange (left, wLeft, wRight1);
	CoordI right = (CoordI) ceil ((CoordD) x / (CoordD) accuracy);
	right = checkrange (right, wLeft, wRight1);
	CoordI top = (CoordI) floor ((CoordD) y / (CoordD) accuracy);
	top	= checkrange (top, wTop, wBottom1);
	CoordI bottom = (CoordI) ceil ((CoordD) y / (CoordD) accuracy);
	bottom = checkrange (bottom, wTop, wBottom1);

	const PixelI lt = pixel (left, top);
	const PixelI rt = pixel (right, top);
	const PixelI lb = pixel (left, bottom);
	const PixelI rb = pixel (right, bottom);
	const CoordI distX = x - left * accuracy;
	const CoordI distY = y - top * accuracy;
	Double x01 = distX * (rt - lt) + accuracy * lt; // use p.59's notation (Wolberg, Digital Image Warping)
	Double x23 = distX * (rb - lb) + accuracy * lb;
	PixelI pxlRet = checkrange ((Int) ((accuracy * x01 + (x23 - x01) * distY)) / (accuracy * accuracy), 0, 255);
	return pxlRet;
}

own CIntImage* CIntImage::complement () const
{
	CIntImage* piiDst = new CIntImage (where(), (PixelI) transpValue);
	const PixelI* ppxliSrc = pixels ();
	PixelI* ppxliDst = (PixelI*) piiDst->pixels ();
	for (UInt iPxl = 0; iPxl < where ().area (); iPxl++)
		*ppxliDst++ = *ppxliSrc++ ^ 0xFF;
	return piiDst;
}

Void CIntImage::overlay (const CIntImage& ii)
{
	if (!valid () || !ii.valid () || ii.where ().empty ()) return;
	CRct r = m_rc; 
	r.include (ii.m_rc); // overlay is defined on union of rects
	where (r); 
	if (!valid ()) return; 
	assert (ii.m_ppxli != NULL); 
	CRct rctFi = ii.m_rc;

	Int widthFi = rctFi.width;
	Int widthCurr = where ().width;
	PixelI* ppxliThis = (PixelI*) pixels (rctFi.left, rctFi.top);
	const PixelI* ppxliFi = ii.pixels ();
	for (CoordI y = rctFi.top; y < rctFi.bottom; y++) { // loop through VOP CRct
		memcpy (ppxliThis, ppxliFi, rctFi.width * sizeof (PixelI));
		ppxliThis += widthCurr; 
		ppxliFi += widthFi;
	}
}

Void CIntImage::overlay (const CFloatImage& fi)
{
	if (!valid () || !fi.valid () || fi.where ().empty ()) return;
	CRct r = m_rc; 
	r.include (fi.where()); // overlay is defined on union of rects
	where (r); 
	if (!valid ()) return; 
	assert (fi.pixels() != NULL); 
	CRct rctFi = fi.where();

	PixelF fVal;
	Int widthFi = rctFi.width;
	Int widthCurr = where ().width;
	PixelI* ppxliThis = (PixelI*) pixels (rctFi.left, rctFi.top);
	const PixelF* ppxliFi = fi.pixels ();
	CoordI x;
	for (CoordI y = rctFi.top; y < rctFi.bottom; y++) { // loop through VOP CRct
		for(x=0;x<rctFi.width;x++)
		{
			fVal=ppxliFi[x];
			ppxliThis[x]=(fVal>=0)?(Int)(fVal+0.5):(Int)(fVal-0.5);
		}
		ppxliThis += widthCurr; 
		ppxliFi += widthFi;
	}
}

own CIntImage* CIntImage::smooth_ (UInt window) const
{
	const UInt offset = window >> 1;
	const UInt offset2 = offset << 1;
	const UInt size = window * window; // array size to be sorted
	const UInt med = size >> 1;
	CIntImage* pfmgRet = new CIntImage (*this);
	// bound of the image to be filtered.
	const CoordI left = where ().left + offset;
	const CoordI top = where ().top + offset;
	const CoordI right = where ().right - offset;
	const CoordI bottom = where ().bottom - offset;
	const Int width = where ().width;
	const Int dist = offset + offset * width;
	const Int wwidth = width - window;
	PixelI* rgValues = new PixelI [size];
	PixelI* pRet = (PixelI*) pfmgRet -> pixels (left, top);
	const PixelI* p = pixels (left, top);
	for (CoordI y = top; y != bottom; y++) {
		for (CoordI x = left; x != right; x++) {
			const PixelI* pp = p - dist; // get correct index
			UInt numTransp = 0;
			for (UInt sy = 0; sy < window; sy++) {
				for (UInt sx = 0; sx < window; sx++) {
					if (*pp == transpValue)
						numTransp++;
					pp++;
				}
				pp += wwidth;
			}
			*pRet++ = (PixelI) ((numTransp <= med) ? opaqueValue : transpValue);
			p++;
		}
		pRet += offset2;
		p += offset2;
	}
	delete [] rgValues;
	return pfmgRet;
}

own CIntImage* CIntImage::smooth (UInt window) const
{
	UInt offset = window >> 1;
	CRct rctExp (where ());
	rctExp.expand (offset);
	CIntImage* piiExp = new CIntImage (*this, rctExp);
	CIntImage* piiSmooth = piiExp -> smooth_ (window);
	piiSmooth -> where (where ());
	delete piiExp;
	return piiSmooth;
}

Void CIntImage::xorIi (const CIntImage& ii)
{
	CRct rctIntersect = m_rc;
	rctIntersect.clip (ii.where());
	if (!rctIntersect.valid () || rctIntersect.empty ()) return;

	PixelI* ppxliRowStart1 = (PixelI*) pixels (rctIntersect.left, rctIntersect.top);
	const PixelI* ppxliRowStart2 = ii.pixels (rctIntersect.left, rctIntersect.top);
	for (CoordI iy = rctIntersect.top; iy < rctIntersect.bottom; iy++)	  {
		PixelI* ppxli1 = ppxliRowStart1;
		const PixelI* ppxli2 = ppxliRowStart2;
		for (CoordI ix = rctIntersect.left; ix < rctIntersect.right; ix++)	{
			assert (*ppxli1 == transpValue || *ppxli1 == opaqueValue);
			assert (*ppxli2 == transpValue || *ppxli2 == opaqueValue);		
			if (*ppxli1 == *ppxli2)	
				*ppxli1 = (PixelI) transpValue;
			else 
				*ppxli1 = (PixelI) opaqueValue;
			ppxli1++;
			ppxli2++;
		}
		ppxliRowStart1 += where().width;
		ppxliRowStart2 += ii.where().width;
	}
}

Void CIntImage::orIi (const CIntImage& ii)
{
	CRct rctIntersect = m_rc;
	rctIntersect.clip (ii.where());
	if (!rctIntersect.valid () || rctIntersect.empty ()) return;

	PixelI* ppxliRowStart1 = (PixelI*) pixels (rctIntersect.left, rctIntersect.top);
	const PixelI* ppxliRowStart2 = ii.pixels (rctIntersect.left, rctIntersect.top);
	for (CoordI iy = rctIntersect.top; iy < rctIntersect.bottom; iy++)	  {
		PixelI* ppxli1 = ppxliRowStart1;
		const PixelI* ppxli2 = ppxliRowStart2;
		for (CoordI ix = rctIntersect.left; ix < rctIntersect.right; ix++)	{
			assert (*ppxli1 == transpValue || *ppxli1 == opaqueValue);
			assert (*ppxli2 == transpValue || *ppxli2 == opaqueValue);		
			if (*ppxli2 == opaqueValue)
				*ppxli1 = opaqueValue;
			ppxli1++;
			ppxli2++;
		}
		ppxliRowStart1 += where ().width;
		ppxliRowStart2 += ii.where ().width;
	}
}

Void CIntImage::andIi (const CIntImage& ii)
{
	CRct rctIntersect = m_rc;
	rctIntersect.clip (ii.where());
	if (!rctIntersect.valid () || rctIntersect.empty ()) return;

	PixelI* ppxliRowStart1 = (PixelI*) pixels (rctIntersect.left, rctIntersect.top);
	const PixelI* ppxliRowStart2 = ii.pixels (rctIntersect.left, rctIntersect.top);
	for (CoordI iy = rctIntersect.top; iy < rctIntersect.bottom; iy++)	  {
		PixelI* ppxli1 = ppxliRowStart1;
		const PixelI* ppxli2 = ppxliRowStart2;
		for (CoordI ix = rctIntersect.left; ix < rctIntersect.right; ix++)	{
			assert (*ppxli1 == transpValue || *ppxli1 == opaqueValue);
			assert (*ppxli2 == transpValue || *ppxli2 == opaqueValue);		
			if (*ppxli2 == transpValue)
				*ppxli1 = transpValue;
			ppxli1++;
			ppxli2++;
		}
		ppxliRowStart1 += where().width;
		ppxliRowStart2 += ii.where().width;
	}
}


Void CIntImage::maskOut (const CIntImage& ii)
{
	CRct rctIntersect = m_rc;
	rctIntersect.clip (ii.where());
	if (!rctIntersect.valid () || rctIntersect.empty ()) return;

	PixelI* ppxliRowStart = (PixelI*) pixels (rctIntersect.left, rctIntersect.top);
	const PixelI* ppxliRowStartMask = ii.pixels (rctIntersect.left, rctIntersect.top);
	for (CoordI iy = rctIntersect.top; iy < rctIntersect.bottom; iy++)	  {
		PixelI* ppxli = ppxliRowStart;
		const PixelI* ppxliMask = ppxliRowStartMask;
		for (CoordI ix = rctIntersect.left; ix < rctIntersect.right; ix++)	{
			assert (*ppxliMask == transpValue || *ppxliMask == opaqueValue);
			assert (*ppxli == transpValue || *ppxli == opaqueValue);		
			if (*ppxliMask != transpValue)
				*ppxli = transpValue;
			ppxli++;
			ppxliMask++;
		}
		ppxliRowStart += where ().width;
		ppxliRowStartMask += ii.where ().width;
	}
}

Void CIntImage::mutiplyAlpha (const CIntImage& ii)
{
	CRct rctIntersect = m_rc;
	rctIntersect.clip (ii.where());
	if (!rctIntersect.valid () || rctIntersect.empty ()) return;

	PixelI* ppxliRowStart = (PixelI*) pixels (rctIntersect.left, rctIntersect.top);
	const PixelI* ppxliRowStartMask = ii.pixels (rctIntersect.left, rctIntersect.top);
	for (CoordI iy = rctIntersect.top; iy < rctIntersect.bottom; iy++)	  {
		PixelI* ppxli = ppxliRowStart;
		const PixelI* ppxliMask = ppxliRowStartMask;
		for (CoordI ix = rctIntersect.left; ix < rctIntersect.right; ix++)	{
			assert (*ppxliMask == transpValue || *ppxliMask == opaqueValue);
			if (*ppxliMask == transpValue)
				*ppxli = transpValue;
			else
				*ppxli = (PixelI) ((*ppxli * *ppxliMask + 127) / opaqueValueF); //normalize
			ppxli++;
			ppxliMask++;
		}
		ppxliRowStart += where ().width;
		ppxliRowStartMask += ii.where ().width;

	}
}

Void CIntImage::cropOnAlpha ()
{
	CRct rctVisible = whereVisible ();
	where (rctVisible);				
}

Void CIntImage::operator = (const CIntImage& ii)
{
	delete [] m_ppxli;
	copyConstruct (ii, ii.where ());
}

Bool CIntImage::operator == (const CIntImage& ii) const
{
	if (ii.where () != where ())
		return FALSE;
	UInt area = where ().area ();
	const PixelI* ppxli = ii.pixels ();
	const PixelI* ppxliThis = pixels ();
	for (UInt ip = 0; ip < area; ip++, ppxli++, ppxliThis++)
		if (*ppxli != *ppxliThis)
			return FALSE;
	return TRUE;
}

Double CIntImage::mse (const CIntImage& iiCompare) const
{
	assert (iiCompare.where () == where ());
	Int sqr = 0;
	const PixelI* ppxliThis = pixels ();
	const PixelI* ppxliCompare = iiCompare.pixels ();
	UInt area = where ().area ();
	for (UInt i = 0; i < area; i++) {
		Int diffR = *ppxliThis - *ppxliCompare;
		sqr += diffR * diffR;
		ppxliThis++;
		ppxliCompare++;
	}
	return ((Double) sqr / area);
}

Double CIntImage::mse (const CIntImage& iiCompare, const CIntImage& iiMsk) const
{
	assert (iiCompare.where () == where () && iiMsk.where () == where ());
	Int sqr = 0;
	const PixelI* ppxliThis = pixels ();
	const PixelI* ppxliCompare = iiCompare.pixels ();
	const PixelI* ppxliMsk = iiMsk.pixels ();
	UInt area = where ().area ();
	UInt uiNonTransp = 0;
	for (UInt i = 0; i < area; i++) {
		if (*ppxliMsk != transpValue) {
			Int diffR = *ppxliThis - *ppxliCompare;
			sqr += diffR * diffR;
			uiNonTransp++;
		}
		ppxliThis++;
		ppxliCompare++;
		ppxliMsk++;
	}
	if (uiNonTransp == 0) 
		return 0;
	return ((Double) sqr / area);
}

Double CIntImage::snr (const CIntImage& iiCompare) const
{
	Double msError = (Double) mse (iiCompare);
	if (msError == 0.0)
		return 1000000.0;
	else 
		return (log10 (255 * 255 / msError) * 10.0);
}

Double CIntImage::snr (const CIntImage& iiCompare, const CIntImage& iiMsk) const
{
	CIntImage* piiMskOp = NULL;
	Double msError = 0;
	if (&iiMsk == NULL) {
		piiMskOp = new CIntImage (where (), (PixelI) opaqueValue);		
		msError = mse (iiCompare, *piiMskOp);
		delete piiMskOp;
	}
	else
		msError = mse (iiCompare, iiMsk);
	if (msError == 0.0)
		return 1000000.0;
	else 
		return (log10 (255 * 255 / msError) * 10.0);
}


Void CIntImage::vdlDump (const Char* fileName) const
{
	CVideoObjectPlane vop (where (), opaquePixel);
	CPixel* ppxl = (CPixel*) vop.pixels ();
	const PixelI* ppxli = pixels ();
	UInt area = where ().area ();
	for (UInt ip = 0; ip < area; ip++, ppxl++, ppxli++)
		*ppxl = CPixel (*ppxli, *ppxli, *ppxli, opaqueValue);
	vop.vdlDump (fileName);
}
	
Void CIntImage::dump (FILE* pf) const
{
	// was a binary write, but now reflects code in CFloatImage::dump()
	//fwrite (m_ppxli, sizeof (Int), where ().area (), pf);

	assert (pf != NULL);
	UInt area = where ().area ();
	U8* rguchPixelData = new U8 [where().area()];
	U8* puch = rguchPixelData;
	const PixelI* ppxli = pixels ();
	for (UInt ip = 0; ip < area; ip++, puch++, ppxli++) {
		U8 vl = (U8) checkrange ((Int)*ppxli, (Int)0, (Int)255);
		*puch = vl;
	}
	fwrite (rguchPixelData, sizeof (U8), area, pf);
	delete [] rguchPixelData;
}
	
Void CIntImage::txtDump (const Char* fileName) const
{
	FILE* pfTxt;
	const PixelI* ppxli = pixels ();
	if (fileName != NULL)
		pfTxt = fopen (fileName, "w");
	else
		pfTxt = NULL;
	for (CoordI y = 0; y < where ().height (); y++) {
		for (CoordI x = 0; x < where ().width; x++) {
			if (pfTxt != NULL)
				fprintf (pfTxt, "%3d  ", *ppxli);
			else
				printf ("%3d  ", *ppxli);
			ppxli++;
		}
		if (pfTxt != NULL)
			fprintf (pfTxt, "\n");
		else
			printf ("\n");
	}
	if (pfTxt != NULL)
		fclose (pfTxt);
}


Void CIntImage::txtDump (FILE *pf) const
{
	const PixelI *ppxli = pixels();
	for (CoordI y = 0; y < where ().height (); y++) {
		for (CoordI x = 0; x < where ().width; x++) {
			fprintf (pf, "%3d ", *ppxli);
			ppxli++;
		}
		fprintf (pf,"\n");
	}
	fprintf (pf,"\n");
}

Void CIntImage::txtDumpMask (FILE *pf) const
{
	const PixelI *ppxli = pixels ();
	for (CoordI y = 0; y < where ().height (); y++) {
		for (CoordI x = 0; x < where ().width; x++) {
			if (*ppxli == transpValue)
				fprintf (pf, "..");
			else
				fprintf (pf, "[]");
			ppxli++;
		}
		fprintf (pf,"\n");
	}
	fprintf (pf,"\n");
}

own CIntImage* CIntImage::operator + (const CIntImage& ii) const
{
	if (!valid () || !ii.valid ()) return NULL;
	assert (where () == ii.where ());
	CIntImage* piiSumRet = new CIntImage (where ());
	PixelI* ppxliRet = (PixelI*) piiSumRet -> pixels ();
	const PixelI* ppxliThis = pixels ();
	const PixelI* ppxliFi = ii.pixels ();
	UInt area = where ().area ();
	for (UInt ip = 0; ip < area; ip++, ppxliRet++, ppxliThis++, ppxliFi++)
		*ppxliRet = *ppxliThis + *ppxliFi;
	return piiSumRet;
}

own CIntImage* CIntImage::operator - (const CIntImage& ii) const
{
	if (!valid () || !ii.valid ()) return NULL;
	assert (where () == ii.where ());
	CIntImage* piiSumRet = new CIntImage (where ());
	PixelI* ppxliRet = (PixelI*) piiSumRet -> pixels ();
	const PixelI* ppxliThis = pixels ();
	const PixelI* ppxliFi = ii.pixels ();
	UInt area = where ().area ();
	for (UInt ip = 0; ip < area; ip++, ppxliRet++, ppxliThis++, ppxliFi++)
		*ppxliRet = *ppxliThis - *ppxliFi;
	return piiSumRet;
}

own CIntImage* CIntImage::operator * (Int scale) const
{
	if (!valid ()) return NULL;
	CIntImage* piiSumRet = new CIntImage (where ());
	PixelI* ppxliRet = (PixelI*) piiSumRet -> pixels ();
	const PixelI* ppxliThis = pixels ();
	UInt area = where ().area ();
	for (UInt ip = 0; ip < area; ip++, ppxliRet++, ppxliThis++)
		*ppxliRet = *ppxliThis * scale;
	return piiSumRet;
}

own CIntImage* CIntImage::operator / (Int scale) const
{
	if (!valid ()) return NULL;
	assert (scale != .0f);
	CIntImage* piiSumRet = new CIntImage (where ());
	PixelI* ppxliRet = (PixelI*) piiSumRet -> pixels ();
	const PixelI* ppxliThis = pixels ();
	UInt area = where ().area ();
	for (UInt ip = 0; ip < area; ip++, ppxliRet++, ppxliThis++)
		*ppxliRet = *ppxliThis / scale;
	return piiSumRet;
}

own CIntImage* CIntImage::average (const CIntImage& ii) const
{
	if (!valid () || !ii.valid ()) return NULL;
	assert (where () == ii.where ());
	CIntImage* piiSumRet = new CIntImage (where ());
	PixelI* ppxliRet = (PixelI*) piiSumRet -> pixels ();
	const PixelI* ppxliThis = pixels ();
	const PixelI* ppxliFi = ii.pixels ();
	UInt area = where ().area ();
	for (UInt ip = 0; ip < area; ip++, ppxliRet++, ppxliThis++, ppxliFi++)
		*ppxliRet = (*ppxliThis + *ppxliFi + 1) / 2;
	return piiSumRet;
}

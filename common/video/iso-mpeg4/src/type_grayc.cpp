/*************************************************************************

This software module was originally developed by 

	Ming-Chieh Lee (mingcl@microsoft.com), Microsoft Corporation
	Wei-ge Chen (wchen@microsoft.com), Microsoft Corporation
	Bruce Lin (blin@microsoft.com), Microsoft Corporation
	Chuang Gu (chuanggu@microsoft.com), Microsoft Corporation
	(date: March, 1996)

  and edited by
    Sehoon Son (shson@unitel.co.kr) Samsung AIT

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

	grayc.hpp

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

#ifdef __NBIT_
void pxlcmemset(PixelC *ppxlc, PixelC pxlcVal, Int iCount)
{
	Int i;
	for(i=0; i<iCount;i++)
		ppxlc[i] = pxlcVal;
}
#endif

CU8Image::~CU8Image ()
{
	delete [] m_ppxlc;
	m_ppxlc = NULL;
}

Void CU8Image::allocate (const CRct& r, PixelC pxlc) 
{
	m_rc = r;
	delete [] m_ppxlc, m_ppxlc = NULL;

	// allocate pixels and initialize
	if (m_rc.empty ()) return;
	m_ppxlc = new PixelC [m_rc.area ()];
	memset (m_ppxlc, pxlc, where ().area () * sizeof (PixelC));
}

Void CU8Image::allocate (const CRct& r) 
{
	m_rc = r;

	// allocate pixels and initialize
	m_ppxlc = new PixelC [m_rc.area ()];
	assert (m_ppxlc != NULL);
}

Void CU8Image::copyConstruct (const CU8Image& uci, const CRct& rct) 
{
	CRct r = rct;
	if (!r.valid()) 
		r = uci.where ();
	if (!uci.valid () || (!uci.m_rc.empty () && uci.m_ppxlc == NULL))
		assert (FALSE);
	allocate (r, (PixelC) 0);
	if (!valid ()) return; 

	// Copy data
	if (r == uci.where ())
		memcpy (m_ppxlc, uci.pixels (), m_rc.area () * sizeof (PixelC));
	else {
		r.clip (uci.where ()); // find the intersection
		CoordI x = r.left; // copy pixels
		Int cbLine = r.width * sizeof (PixelC);
		PixelC* ppxl = (PixelC*) pixels (x, r.top);
		const PixelC* ppxlFi = uci.pixels (x, r.top);
		Int widthCurr = where ().width;
		Int widthFi = uci.where ().width;
		for (CoordI y = r.top; y < r.bottom; y++) {
			memcpy (ppxl, ppxlFi, cbLine);
			ppxl += widthCurr;
			ppxlFi += widthFi;
		}
	}
}

Void CU8Image::swap (CU8Image& uci) 
{
	assert (this && &uci);
	CRct rcT = uci.m_rc; 
	uci.m_rc = m_rc; 
	m_rc = rcT; 
	PixelC* ppxlcT = uci.m_ppxlc; 
	uci.m_ppxlc = m_ppxlc; 
	m_ppxlc = ppxlcT; 
}

CU8Image::CU8Image (const CU8Image& uci, const CRct& r) : m_ppxlc (NULL)
{
	copyConstruct (uci, r);
}

CU8Image::CU8Image (const CRct& r, PixelC px) : m_ppxlc (NULL)
{
	allocate (r, px);
}

CU8Image::CU8Image (const CRct& r) : m_ppxlc (NULL)
{
	allocate (r);
}

CU8Image::CU8Image (const CVideoObjectPlane& vop, RGBA comp, const CRct& r) : m_ppxlc (NULL)
{
	if (!vop.valid ()) return;
	CU8Image* puci = new CU8Image (vop.where ());
	PixelC* ppxlc = (PixelC*) puci -> pixels ();
	const CPixel* ppxl = vop.pixels ();
	UInt area = puci -> where ().area ();
	for (UInt ip = 0; ip < area; ip++, ppxl++)
		*ppxlc++ = (PixelC) ppxl -> pxlU.color [comp];
	copyConstruct (*puci, r);
	delete puci;
}

CU8Image::CU8Image (
	const Char* pchFileName, 
	UInt ifr,
	const CRct& rct,
	UInt nszHeader
) : m_ppxlc (NULL), m_rc (rct)
{
	assert (!rct.empty ());
	assert (ifr >= 0);
	assert (nszHeader >= 0);
	UInt uiArea = rct.area ();
	delete [] m_ppxlc;
	m_ppxlc = new PixelC [uiArea];
	assert (m_ppxlc);
	
	// read data from a file
	FILE* fpSrc = fopen (pchFileName, "rb");
	assert (fpSrc != NULL);
	fseek (fpSrc, nszHeader + ifr * sizeof (U8) * uiArea, SEEK_SET);
	Int size = (Int) fread (m_ppxlc, sizeof (U8), uiArea, fpSrc);
	assert (size != 0);
	fclose (fpSrc);
}

CU8Image::CU8Image (const Char* vdlFileName) : m_ppxlc (NULL)// read from a VM file.
{
	CVideoObjectPlane vop (vdlFileName);
	m_rc = vop.where ();
	UInt uiArea = where ().area ();
	delete [] m_ppxlc;
	m_ppxlc = new PixelC [uiArea];
	assert (m_ppxlc);

	const CPixel* ppxlVop = vop.pixels ();
	PixelC* ppxlcRet = (PixelC*) pixels ();
	for (UInt ip = 0; ip < uiArea; ip++, ppxlcRet++, ppxlVop++)
		*ppxlcRet = (PixelC) ppxlVop -> pxlU.rgb.r;
}

Void CU8Image::where (const CRct& r) 
{
	if (!valid ()) return; 
	if (where () == r) return; 
	CU8Image* puci = new CU8Image (*this, r);
	swap (*puci);
	delete puci;
}


CRct CU8Image::boundingBox (const PixelC pxlcOutsideColor) const
{
	if (allValue ((PixelC) pxlcOutsideColor))	
		return CRct ();
	CoordI left = where ().right - 1;
	CoordI top = where ().bottom - 1;
	CoordI right = where ().left;
	CoordI bottom = where ().top;
	const PixelC* ppxlcThis = pixels ();
	for (CoordI y = where ().top; y < where ().bottom; y++) {
		for (CoordI x = where ().left; x < where ().right; x++) {
			if (*ppxlcThis != (PixelC) pxlcOutsideColor) {
				left = min (left, x);
				top = min (top, y);
				right = max (right, x);
				bottom = max (bottom, y);
			}								
			ppxlcThis++;
		}
	}
	right++;
	bottom++;
	return (CRct (left, top, right, bottom));				
}


U8 CU8Image::mean () const
{
	if (where ().empty ()) return 0;
	Int meanRet = 0;
	PixelC* ppxlc = (PixelC*) pixels ();
	UInt area = where ().area ();
	for (UInt ip = 0; ip < area; ip++)
		meanRet += ppxlc [ip];
	return (U8) (meanRet / area);
}

U8 CU8Image::mean (const CU8Image* puciMsk) const
{
	assert (where () == puciMsk -> where ()); // no compute if rects are different
	if (where ().empty ()) return 0;
	Int meanRet = 0;
	PixelC* ppxlc = (PixelC*) pixels ();
	const PixelC* ppxlcMsk = puciMsk -> pixels ();
	UInt area = where ().area ();
	UInt uiNumNonTransp = 0;
	for (UInt ip = 0; ip < area; ip++) {
		if (ppxlcMsk [ip] != transpValue) {
			uiNumNonTransp++;
			meanRet += ppxlc [ip];
		}
	}
	return (U8) (meanRet / uiNumNonTransp);
}

Int CU8Image::sumDeviation (const CU8Image* puciMsk) const // sum of first-order deviation
{
	U8 meanPxl = mean (puciMsk);
	Int devRet = 0;
	PixelC* ppxlc = (PixelC*) pixels ();
	const PixelC* ppxlcMsk = puciMsk -> pixels ();
	UInt area = where ().area ();
	for (UInt ip = 0; ip < area; ip++) {
		if (ppxlcMsk [ip] != transpValue)
			devRet += abs (meanPxl - ppxlc [ip]);
	}
	return devRet;
}


Int CU8Image::sumDeviation () const // sum of first-order deviation
{
	U8 meanPxl = mean ();
	Int devRet = 0;
	PixelC* ppxlc = (PixelC*) pixels ();
	UInt area = where ().area ();
	for (UInt ip = 0; ip < area; ip++)
		devRet += abs (meanPxl - ppxlc [ip]);
	return devRet;
}


UInt CU8Image::sumAbs (const CRct& rct) const // sum of absolute value
{
	CRct rctToDo = (!rct.valid ()) ? where () : rct;
	UInt uiRet = 0;
	if (rctToDo == where ())	{	
		const PixelC* ppxlc = pixels ();
		UInt area = where ().area ();
		for (UInt ip = 0; ip < area; ip++, ppxlc++) {
			if (*ppxlc > 0) 
				uiRet += *ppxlc;
			else
				uiRet -= *ppxlc;
		}
	}
	else	{
		Int width = where ().width;
		const PixelC* ppxlcRow = pixels (rct.left, rct.top);
		for (CoordI y = rctToDo.top; y < rctToDo.bottom; y++) {
			const PixelC* ppxlc = ppxlcRow;
			for (CoordI x = rctToDo.left; x < rctToDo.right; x++, ppxlc++) {
				if (*ppxlc > 0)
					uiRet += *ppxlc;
				else
					uiRet -= *ppxlc;
			}
			ppxlcRow += width;
		}
	}
	return uiRet;
}

Bool CU8Image::allValue (PixelC ucVl, const CRct& rct) const
{
	CRct rctToDo = (!rct.valid ()) ? where () : rct;
	if (rctToDo == where ()) {	
		const PixelC* ppxlc = pixels ();
		UInt area = where ().area ();
		for (UInt ip = 0; ip < area; ip++) {
			if (ppxlc [ip] != ucVl)
				return FALSE;
		}
	}
	else {
		Int width = where ().width;
		const PixelC* ppxlc = pixels (rct.left, rct.top);
		for (CoordI y = rctToDo.top; y < rctToDo.bottom; y++) {
			const PixelC* ppxlcRow = ppxlc;
			for (CoordI x = rctToDo.left; x < rctToDo.right; x++, ppxlcRow++) {
				if (*ppxlcRow != ucVl)
					return FALSE;
			}
			ppxlc += width;
		}
	}
	return TRUE;
}

Bool CU8Image::biLevel (const CRct& rct) const
{
	CRct rctToDo = (!rct.valid ()) ? where () : rct;
	if (rctToDo == where ())	{	
		const PixelC* ppxlc = pixels ();
		UInt area = where ().area ();
		for (UInt ip = 0; ip < area; ip++) {
			if (ppxlc [ip] != opaqueValue && ppxlc [ip] != transpValue )
				return FALSE;
		}
	}
	else {
		Int width = where ().width;
		const PixelC* ppxlc = pixels (rct.left, rct.top);
		for (CoordI y = rctToDo.top; y < rctToDo.bottom; y++) {
			const PixelC* ppxlcRow = ppxlc;
			for (CoordI x = rctToDo.left; x < rctToDo.right; x++, ppxlcRow++) {
				if (*ppxlcRow != opaqueValue && *ppxlcRow != transpValue )
					return FALSE;
			}
			ppxlc += width;
		}
	}
	return TRUE;
}

CRct CU8Image::whereVisible () const
{
	CoordI left = where ().right - 1;
	CoordI top = where ().bottom - 1;
	CoordI right = where ().left;
	CoordI bottom = where ().top;
	const PixelC* ppxlcThis = pixels ();
	for (CoordI y = where ().top; y < where ().bottom; y++) {
		for (CoordI x = where ().left; x < where ().right; x++) {
			if (*ppxlcThis != transpValue) {
				left = min (left, x);
				top = min (top, y);
				right = max (right, x);
				bottom = max (bottom, y);
			}								
			ppxlcThis++;
		}
	}
	right++;
	bottom++;
	return CRct (left, top, right, bottom);
}


Bool CU8Image::atLeastOneValue (PixelC ucVl, const CRct& rct) const
{
	CRct rctRegionOfInterest = (!rct.valid ()) ? where () : rct;
	assert (rctRegionOfInterest <= where ());
	if (rctRegionOfInterest == where ()) {
		const PixelC* ppxlc = pixels ();
		UInt area = where ().area ();
		for (UInt ip = 0; ip < area; ip++) {
			if (ppxlc [ip] == ucVl)
				return TRUE;
		}
	}
	else {
		Int width = where ().width;
		const PixelC* ppxlc = pixels (rctRegionOfInterest.left, rctRegionOfInterest.top);
		for (CoordI y = rctRegionOfInterest.top; y < rctRegionOfInterest.bottom; y++) {
			const PixelC* ppxlcRow = ppxlc;
			for (CoordI x = rctRegionOfInterest.left; x < rctRegionOfInterest.right; x++, ppxlcRow++) {
				if (*ppxlcRow == ucVl)
					return TRUE;
			}
			ppxlc += width;
		}
	}
	return FALSE;
}


UInt CU8Image::numPixelsNotValued (PixelC ucVl, const CRct& rct) const // number of pixels not valued vl in region rct
{
	CRct rctInterest = (!rct.valid ()) ? where () : rct;
	assert (rctInterest <= where ());
	UInt nRet = 0;
	if (rctInterest == where ()) {
		const PixelC* ppxlc = pixels ();
		UInt area = where ().area ();
		for (UInt ip = 0; ip < area; ip++) {
			if (ppxlc [ip] != ucVl)
				nRet++;
		}
	}
	else {
		Int width = where ().width;
		const PixelC* ppxlc = pixels (rctInterest.left, rctInterest.top);
		for (CoordI y = rctInterest.top; y < rctInterest.bottom; y++) {
			const PixelC* ppxlcRow = ppxlc;
			for (CoordI x = rctInterest.left; x < rctInterest.right; x++, ppxlcRow++) {
				if (*ppxlcRow != ucVl)
					nRet++;
			}
			ppxlc += width;
		}
	}

	return nRet;
}


Void CU8Image::setRect (const CRct& rct)
{
	assert (rct.area () == m_rc.area ());
	m_rc = rct;
}

Void CU8Image::threshold (PixelC ucThresh)
{
	PixelC* ppxlThis = (PixelC*) pixels ();
	UInt area = where ().area ();
	for (UInt id = 0; id < area; id++) {
		if (ppxlThis [id] < ucThresh)
			ppxlThis [id] = 0;
	}
}

Void CU8Image::binarize (PixelC ucThresh)
{
	PixelC* ppxlcThis = (PixelC*) pixels ();
	UInt area = where ().area ();
	for (UInt id = 0; id < area; id++) {
		if (*ppxlcThis < ucThresh)
			*ppxlcThis = transpValue;
		else
			*ppxlcThis = opaqueValue;
	}
}


/* NBIT: change U8 to PixelC
Void CU8Image::checkRange (U8 ucMin, U8 ucMax)
*/
Void CU8Image::checkRange (PixelC ucMin, PixelC ucMax)
{
	PixelC* ppxlcThis = (PixelC*) pixels ();
	UInt area = where ().area ();
	for (UInt id = 0; id < area; id++, ppxlcThis++)
		*ppxlcThis = checkrange (*ppxlcThis, ucMin, ucMax);
}

own CU8Image* CU8Image::decimate (UInt rateX, UInt rateY) const
{
	const CoordI left = where ().left / (CoordI) rateX;
	const CoordI top = where ().top / (CoordI)  rateY;
	const CoordI right = 
		(where ().right >= 0) ? (where ().right + (CoordI) rateX - 1) / (CoordI) rateX :
		(where ().right - (CoordI) rateX + 1) / (CoordI) rateX;
	const CoordI bottom = 
		(where ().bottom >= 0) ? (where ().bottom + (CoordI) rateX - 1) / (CoordI) rateY :
		(where ().bottom - (CoordI) rateX + 1) / (CoordI) rateY;

	CU8Image* puciRet = new CU8Image (CRct (left, top, right, bottom));
	PixelC* ppxlcRet = (PixelC*) puciRet -> pixels ();
	const PixelC* ppxlcOrgY = pixels ();
	Int skipY = rateY * where ().width;
	
	for (CoordI y = top; y < bottom; y++) {
		const PixelC* ppxlcOrgX = ppxlcOrgY;
		for (CoordI x = left; x < right; x++) {
			*ppxlcRet++ = *ppxlcOrgX;
			ppxlcOrgX += rateX;
		}
		ppxlcOrgY += skipY;
	}
	return puciRet;
}

own CU8Image* CU8Image::decimateBinaryShape (UInt rateX, UInt rateY) const
{
	const CoordI left = where ().left / (CoordI) rateX;
	const CoordI top = where ().top / (CoordI) rateY;
	Int roundR = (where ().right >= 0) ? rateX - 1 : 1 - rateX;
	Int roundB = (where ().bottom >= 0) ? rateY - 1 : 1 - rateY;
	const CoordI right = (where ().right + roundR) / (CoordI) rateX;
	const CoordI bottom = (where ().bottom + roundB) / (CoordI) rateY;

	CU8Image* puciRet = new CU8Image (CRct (left, top, right, bottom));
	PixelC* ppxlcRet = (PixelC*) puciRet -> pixels ();
	const PixelC* ppxlcOrgY = pixels ();
	Int skipY = rateY * where ().width;
	CoordI x, y;
	CoordI iXOrigLeft, iYOrigTop;	//left top of a sampling square

	for (y = top, iYOrigTop = where().top; y < bottom; y++, iYOrigTop += rateY) {
		const PixelC* ppxlcOrgX = ppxlcOrgY;
		for (x = left, iXOrigLeft = where().left; x < right; x++, iXOrigLeft += rateX) {
			CoordI iXOrig, iYOrig;						//for scanning of the sampling square
			const PixelC* ppxlcOrigScanY = ppxlcOrgX;	//same
			*ppxlcRet = transpValue;
			for (iYOrig = iYOrigTop; iYOrig < (iYOrigTop + (Int) rateY); iYOrig++)	{
				if (iYOrig >= where().bottom || *ppxlcRet == opaqueValue)
					break;
				const PixelC* ppxlcOrigScanX = ppxlcOrigScanY; //for scan also
				//Bool bStopScan = FALSE;
				for (iXOrig = iXOrigLeft; iXOrig < (iXOrigLeft + (Int) rateX); iXOrig++)	{
					if (iXOrig >= where().right)
						break;
					assert (*ppxlcOrigScanX == transpValue || *ppxlcOrigScanX == opaqueValue);
					if (*ppxlcOrigScanX == opaqueValue) {
						*ppxlcRet = opaqueValue;
						break;
					}
					ppxlcOrigScanX++;
				}
				ppxlcOrigScanY += where().width;
			}

			assert (*ppxlcRet == transpValue || *ppxlcRet == opaqueValue);
			ppxlcRet++;
			ppxlcOrgX += rateX;
		}
		ppxlcOrgY += skipY;
	}
	return puciRet;
}

own CU8Image* CU8Image::zoomup (UInt rateX, UInt rateY) const
{
	const CoordI left = where ().left * rateX; // left-top coordinate remain the same
	const CoordI top = where ().top * rateY;
	const CoordI right = where ().right * rateX;
	const CoordI bottom = where ().bottom * rateY;

	CU8Image* puciRet = new CU8Image (CRct (left, top, right, bottom));
	PixelC* ppxlRet = (PixelC*) puciRet -> pixels ();
	for (CoordI y = top; y < bottom; y++) {
		for (CoordI x = left; x < right; x++)
			*ppxlRet++ = pixel ((CoordI) (x / (CoordI) rateX), (CoordI) (y / (CoordI) rateY));
	}
	return puciRet;
}

own CU8Image* CU8Image::expand (UInt rateX, UInt rateY) const // expand by putting zeros in between
{
	const CoordI left = where ().left * rateX; // left-top coordinate remain the same
	const CoordI top = where ().top * rateY;
	const CoordI right = where ().right * rateX;
	const CoordI bottom = where ().bottom * rateY;

	CU8Image* puciRet = new CU8Image (CRct (left, top, right, bottom));
	PixelC* ppxlRet = (PixelC*) puciRet -> pixels ();
	const PixelC* ppxlThis = pixels ();
	for (CoordI y = top; y < bottom; y++) {
		for (CoordI x = left; x < right; x++) {
			if (x % rateX == 0 && y % rateY == 0) 
				*ppxlRet++ = *ppxlThis++;
			else
				*ppxlRet++ = 0;
		}
	}
	return puciRet;
}

own CU8Image* CU8Image::biInterpolate () const // bilinearly interpolate the vframe
{
	const CoordI left = where ().left << 1; // left-top coordinate remain the same
	const CoordI top = where ().top << 1;
	const CoordI right = where ().right << 1;
	const CoordI bottom = where ().bottom << 1;
	const CoordI width = right - left;

	CoordI x, y;
	CU8Image* puciRet = new CU8Image (CRct (left, top, right, bottom));
	PixelC* ppxlcRet = (PixelC*) puciRet -> pixels ();
	const PixelC* ppxlc = pixels ();
	const CoordI right1 = right - 2;
	for (y = top; y < bottom; y += 2) { // x-direction interpolation
		for (x = left; x < right1; x += 2) {
			*ppxlcRet++ = *ppxlc++;
			*ppxlcRet++ = (*ppxlc + *(ppxlc - 1) + 1) >> 1;
		}
		*ppxlcRet++ = *ppxlc;
		*ppxlcRet++ = *ppxlc++; // the last pixel of every row do not need average
		ppxlcRet += width;
	}
	ppxlcRet = (PixelC*) puciRet -> pixels ();
	ppxlcRet += width; // start from the second row
	const CoordI width2 = width << 1;
	const CoordI bottom1 = bottom - 1;
	for (x = left; x < right; x++) { // y-direction interpolation
		PixelC* ppxlcRett = ppxlcRet++;
		for (y = top + 1; y < bottom1; y += 2) {
			*ppxlcRett = (*(ppxlcRett - width) + *(ppxlcRett + width) + 1) >> 1;
			ppxlcRett += width2;
		}
		*ppxlcRett = *(ppxlcRett - width); // the last pixel of every column do not need average
	}
	return puciRet;
}

own CU8Image* CU8Image::downsampleForSpatialScalability () const
{
	static Int rgiFilterVertical[13] = {2, 0, -4, -3, 5, 19, 26, 19, 5, -3, -4, 0, 2};
	static Int rgiFilterHorizontal[4] = {5, 11, 11, 5};
	Int iWidthSrc = where (). width;
	Int iHeightSrc = where (). height ();
	assert (iWidthSrc % 2 == 0 && iHeightSrc % 2 == 0);
	Int iWidthDst = iWidthSrc / 2;
	Int iHeightDst = iHeightSrc / 2;
	CU8Image* puciBuffer = new CU8Image (CRct (0, 0, iWidthSrc, iHeightDst));
	CU8Image* puciRet = new CU8Image (CRct (0, 0, iWidthDst, iHeightDst));
	assert (puciBuffer != NULL);
	assert (puciRet != NULL);

	//filter and downsample vertically
	const PixelC* ppxlcSrc;	
	const PixelC* ppxlcColumnHeadSrc = pixels ();
	PixelC* ppxlcDst = (PixelC*) puciBuffer->pixels ();
	PixelC* ppxlcColumnHeadDst = (PixelC*) puciBuffer->pixels ();
	Int i, j, k;
	for (i = 0; i < iWidthSrc; i++)	{
		ppxlcSrc = ppxlcColumnHeadSrc;	
		ppxlcDst = ppxlcColumnHeadDst;	
		for (j = 0; j < iHeightDst; j++)	{
			k = j * 2;         
			const PixelC* ppxlcMinusOne = ( k < 1 ) ? ppxlcSrc : ppxlcSrc - iWidthSrc;		
			const PixelC* ppxlcMinusTwo = ( k < 2 ) ? ppxlcSrc : ppxlcMinusOne - iWidthSrc;
			const PixelC* ppxlcMinusThree = ( k < 3 ) ? ppxlcSrc : ppxlcMinusTwo - iWidthSrc;
			const PixelC* ppxlcMinusFour = ( k < 4 ) ? ppxlcSrc : ppxlcMinusThree - iWidthSrc;
			const PixelC* ppxlcMinusFive = ( k < 5 ) ? ppxlcSrc : ppxlcMinusFour - iWidthSrc;
			const PixelC* ppxlcMinusSix = ( k < 6 ) ? ppxlcSrc : ppxlcMinusFive - iWidthSrc;
			const PixelC* ppxlcPlusOne = ( k >= iHeightSrc - 1) ? ppxlcSrc : ppxlcSrc + iWidthSrc;
			const PixelC* ppxlcPlusTwo = ( k >= iHeightSrc - 2) ? ppxlcPlusOne : ppxlcPlusOne + iWidthSrc;
			const PixelC* ppxlcPlusThree = ( k >= iHeightSrc - 3) ? ppxlcPlusTwo : ppxlcPlusTwo + iWidthSrc;
			const PixelC* ppxlcPlusFour = ( k >= iHeightSrc - 4) ? ppxlcPlusThree : ppxlcPlusThree + iWidthSrc;
			const PixelC* ppxlcPlusFive = ( k >= iHeightSrc - 5) ? ppxlcPlusFour : ppxlcPlusFour + iWidthSrc;
			const PixelC* ppxlcPlusSix = ( k >= iHeightSrc - 6) ? ppxlcPlusFive : ppxlcPlusFive + iWidthSrc;
			*ppxlcDst = checkrangeU8 (
				(U8) ((*ppxlcMinusSix * rgiFilterVertical [0] +
				*ppxlcMinusFive * rgiFilterVertical [1] +
				*ppxlcMinusFour * rgiFilterVertical [2] +
				*ppxlcMinusThree * rgiFilterVertical [3] +
				*ppxlcMinusTwo * rgiFilterVertical [4] +
				*ppxlcMinusOne * rgiFilterVertical [5] +
				*ppxlcSrc * rgiFilterVertical [6] +
				*ppxlcPlusOne * rgiFilterVertical [7] +
				*ppxlcPlusTwo * rgiFilterVertical [8] +
				*ppxlcPlusThree * rgiFilterVertical [9] +
				*ppxlcPlusFour * rgiFilterVertical [10] +
				*ppxlcPlusFive * rgiFilterVertical [11] +
				*ppxlcPlusSix * rgiFilterVertical [12] + 32) >> 6),
				0, 255
			);
			ppxlcSrc += 2 * iWidthSrc;
			ppxlcDst += iWidthSrc;	
		}
		ppxlcColumnHeadSrc++;
		ppxlcColumnHeadDst++;
	} 
	//filter and downsample horizontally
	ppxlcSrc = puciBuffer->pixels ();	
	ppxlcDst = (PixelC*) puciRet->pixels ();
	for (j = 0; j < iHeightDst; j++)	{
		for (i = 0; i < iWidthDst; i++)	{
			k = i * 2;         
			const PixelC* ppxlcMinusOne = ( k < 1 ) ? ppxlcSrc : ppxlcSrc - 1;		
			const PixelC* ppxlcPlusOne = ( k >= iWidthSrc - 1) ? ppxlcSrc : ppxlcSrc + 1;
			const PixelC* ppxlcPlusTwo = ( k >= iWidthSrc - 2) ? ppxlcSrc : ppxlcSrc + 2;
			*ppxlcDst = checkrangeU8 (
				(U8) ((*ppxlcMinusOne * rgiFilterHorizontal [0] +
				*ppxlcSrc * rgiFilterHorizontal [1] +
				*ppxlcPlusOne * rgiFilterHorizontal [2] +
				*ppxlcPlusTwo * rgiFilterHorizontal [3] + 16) >> 5),
				0, 255
			);
			ppxlcSrc += 2;	
			ppxlcDst++;	
		}
	} 
	delete puciBuffer;
	return puciRet;
}

//OBSS_SAIT_991015
own CU8Image* CU8Image::upsampleForSpatialScalability (	Int iVerticalSamplingFactorM,
														Int iVerticalSamplingFactorN,
														Int iHorizontalSamplingFactorM,
														Int iHorizontalSamplingFactorN,
														Int iFrmWidth_SS,			
														Int iFrmHeight_SS,			
														Int iType,
														Int iExpandYRefFrame,
														Bool bShapeOnly		
														) const
{

	CRct rctDst = where ();

	rctDst.right = iFrmWidth_SS/iType+iExpandYRefFrame/iType;					
	rctDst.bottom= iFrmHeight_SS/iType+iExpandYRefFrame/iType;				
	rctDst.width = rctDst.right - rctDst.left;								

	Int iWidthSrc = where (). width ;
	/* wmay
	Int iHeightSrc = where (). height ();
	Int iWidthDst = iFrmWidth_SS/iType+(iExpandYRefFrame*2)/iType;		
	Int iHeightDst = iFrmHeight_SS/iType+(iExpandYRefFrame*2)/iType;	
	*/
	CU8Image* puciRet = new CU8Image (CRct (rctDst.left, rctDst.top, rctDst.right, rctDst.bottom));


	//upsample vertically
	PixelC* ppxlcDst;	
	const PixelC* ppxlcColumnHeadSrc = pixels ();

	Int x,y,y1,y2,x1,x2;
	Int phase = 0;
  
	Int *pict_vert;

	const PixelC* pict= ppxlcColumnHeadSrc +iWidthSrc*iExpandYRefFrame/iType+iExpandYRefFrame/iType;
	ppxlcDst = (PixelC *)puciRet->pixels();
	//wchen: should not malloc at every call; need to change later
	//pict_vert = (Int *) malloc (sizeof(Int)*((where().width-iExpandYRefFrame/iType*2)*(iHeightDst-iExpandYRefFrame/iType*2)));
	// bug fix from Takefumi Nagumo 6/21/99
	pict_vert = (Int *) malloc (sizeof(Int) * (Int)(1 + (where().width-iExpandYRefFrame/iType*2)
		* (where().height()-iExpandYRefFrame/iType*2) * ((Float)iVerticalSamplingFactorN / (Float)iVerticalSamplingFactorM) ) );

	if(bShapeOnly==FALSE) {		
		for(x=0;x< (where().width-iExpandYRefFrame/iType*2); x++)
			for(y=0;y< (rctDst.height()-iExpandYRefFrame/iType*2); y++) {
				y1 = (y*iVerticalSamplingFactorM )/iVerticalSamplingFactorN;
				if(y1 < where().height() - iExpandYRefFrame/iType*2 - 1)
					y2 = y1+1;
				else
					y2 = y1;

				phase = (16*(((y)*iVerticalSamplingFactorM) %
						iVerticalSamplingFactorN) + iVerticalSamplingFactorN/2 /*for rounding*/)
						/iVerticalSamplingFactorN;
				*(pict_vert+y*(where().width-iExpandYRefFrame/iType*2)+x) = (Int)(16-phase)*(*(pict+y1*where().width+x))
						+ phase *(*(pict+y2*where().width+x));
			}
  
		for(y=0;y< rctDst.height()-iExpandYRefFrame*2/iType; y++)
			for(x=0;x< rctDst.width -iExpandYRefFrame*2/iType; x++)	{
				x1 = (x*iHorizontalSamplingFactorM )/iHorizontalSamplingFactorN;
				if(x1 < where().width  - iExpandYRefFrame/iType*2 - 1)
					x2 = x1+1;
				else
					x2 = x1;
				// Warning:no rounding
				phase = (16*(((x)*iHorizontalSamplingFactorM) %
						iHorizontalSamplingFactorN) + iHorizontalSamplingFactorN/2 /*for rounding*/)
						/ iHorizontalSamplingFactorN;
				*(ppxlcDst+(x+iExpandYRefFrame/iType)+(y+iExpandYRefFrame/iType)*rctDst.width)
					  =(PixelC)(((16-phase)*(*(pict_vert+y*(where().width-iExpandYRefFrame/iType*2)+x1))
						+ phase *(*(pict_vert+y*(where().width-iExpandYRefFrame/iType*2)+x2)) +128 )>>8 );
			}
		
	}			

	free(pict_vert);  
	return puciRet;
}
//OBSS_SAIT_FIX000602
own CU8Image* CU8Image::upsampleSegForSpatialScalability (	Int iVerticalSamplingFactorM,
														Int iVerticalSamplingFactorN,
														Int iHorizontalSamplingFactorM,
														Int iHorizontalSamplingFactorN,
														Int iFrmWidth_SS,			
														Int iFrmHeight_SS,			
														Int iType,
														Int iExpandYRefFrame
														) const
{
	Int x,y;

	CRct rctDst;

	rctDst.right = iFrmWidth_SS/iType+iExpandYRefFrame/iType;		
	rctDst.bottom = iFrmHeight_SS/iType+iExpandYRefFrame/iType;		
	rctDst.left  = (where ().left);
	rctDst.top   = (where ().top);
	rctDst.width = rctDst.right - rctDst.left;

	//	CRct rctBuf2 = rctDst;
	Int iWidthSrc = where (). width ;
	Int iHeightSrc = where (). height ();
	//Int iWidthDst = rctDst.width;
	Int iHeightDst = rctDst.bottom - rctDst.top;

	double h_factor = log((double)iHorizontalSamplingFactorN/iHorizontalSamplingFactorM)/log((double)2);
	int h_factor_int = (int)floor(h_factor+0.000001);
	double v_factor = log((double)iVerticalSamplingFactorN/iVerticalSamplingFactorM)/log((double)2);
	int v_factor_int = (int)floor(v_factor+0.000001);

	//wchen: this approach is very slow; will have to change later
	CU8Image* puciRet = new CU8Image (CRct (rctDst.left, rctDst.top, rctDst.right, rctDst.bottom));
	CU8Image* puciBuffX = new CU8Image (CRct (0,0, (int)((rctDst.right - iExpandYRefFrame/iType)/(1<<h_factor_int)), where ().bottom - iExpandYRefFrame/iType));
	CU8Image* puciBuffX2= new CU8Image (CRct (0,0, (int)(rctDst.right - iExpandYRefFrame/iType), where ().bottom - iExpandYRefFrame/iType));
	CU8Image* puciBuffY = new CU8Image (CRct (0,0, (int)(rctDst.right - iExpandYRefFrame/iType), (int)((rctDst.bottom-iExpandYRefFrame/iType)/(1<<v_factor_int))));

	Bool * Xcheck1;			
	Bool * Xcheck2;			
	Bool * Ycheck1;				
	Bool * Ycheck2;			

	Xcheck1 = new Bool [(int)((rctDst.right - iExpandYRefFrame/iType)/(1<<h_factor_int))+1024];		
	Xcheck2 = new Bool [(int)(rctDst.right - iExpandYRefFrame/iType)+1024];	
	Ycheck1 = new Bool [(int)((rctDst.bottom - iExpandYRefFrame/iType)/(1<<v_factor_int))+1024];			
	Ycheck2 = new Bool [(int)(rctDst.bottom - iExpandYRefFrame/iType)+1024];							


	//upsample vertically
	PixelC* ppxlcDst;	
	const PixelC* ppxlcColumnHeadSrc = pixels ();
	const PixelC* pict= ppxlcColumnHeadSrc + iWidthSrc*iExpandYRefFrame/iType+iExpandYRefFrame/iType;

	ppxlcDst = (PixelC *)puciRet->pixels();
	PixelC* ppxlcBuffX = (PixelC *)puciBuffX->pixels();
	PixelC* ppxlcBuffX2 = (PixelC *)puciBuffX2->pixels();
	PixelC* ppxlcBuffY = (PixelC *)puciBuffY->pixels();
	//wchen: should not malloc at every call; need to change later

	//for clear memory
	for(y=0;y< rctDst.height(); y++) 
		for(x=0;x< rctDst.width; x++)	
			*(ppxlcDst+x+y*rctDst.width) = 0;

	for(y=0;y< where ().bottom - iExpandYRefFrame/iType; y++) 
		for(x=0;x< (int)((rctDst.right - iExpandYRefFrame/iType)/(1<<h_factor_int)); x++)	
			*(ppxlcBuffX+x+y*((int)((rctDst.right - iExpandYRefFrame/iType)/(1<<h_factor_int)))) = 0;

	for(y=0;y< where ().bottom - iExpandYRefFrame/iType; y++) 
		for(x=0;x< (int)(rctDst.right - iExpandYRefFrame/iType); x++)	
			*(ppxlcBuffX2+x+y*((int)(rctDst.right - iExpandYRefFrame/iType))) = 0;

	for(y=0;y< (int)((rctDst.bottom-iExpandYRefFrame/iType)/(1<<v_factor_int)); y++) 
		for(x=0;x< (int)(rctDst.right - iExpandYRefFrame/iType); x++)	
			*(ppxlcBuffY+x+y*((int)(rctDst.right - iExpandYRefFrame/iType))) = 0;
	//for clear memory

	Int copy_pel=0;
	Int dst_x=0;
	Int iBuffXWidth = (int)((rctDst.right - iExpandYRefFrame/iType)/(1<<h_factor_int));

	Int iHeightSrcNet = iHeightSrc-iExpandYRefFrame/iType*2;
	Int iWidthSrcNet = iWidthSrc-iExpandYRefFrame/iType*2;
	//	Int iWidthDstNet = iWidthDst - iExpandYRefFrame/iType*2;
	//	Int iHeightDstNet = iHeightDst - iExpandYRefFrame/iType*2;

	if(h_factor - h_factor_int > 0.000001 ){
	  int j; // wmay
		for(j=0; j<(iHeightSrc-iExpandYRefFrame/iType);j++){
			dst_x = 0;
			copy_pel = 0;
			Int next_copy_pel = 0;
			for(int i=0; i<(iWidthSrc-iExpandYRefFrame/iType);i++){		
				if(i==next_copy_pel){
					if(i<iWidthSrcNet && j<iHeightSrcNet)		
						*(ppxlcBuffX+dst_x+j*iBuffXWidth) = (PixelC) *(pict+j*where().width+i);
					Xcheck1[dst_x] = 0 ;	
					dst_x++;
					copy_pel++;
					next_copy_pel = (int)floor(	((double) (iHorizontalSamplingFactorM*(1<<h_factor_int))/(iHorizontalSamplingFactorN-iHorizontalSamplingFactorM*(1<<h_factor_int))*copy_pel));
				}
				if(i<iWidthSrcNet && j<iHeightSrcNet)		
					*(ppxlcBuffX+dst_x+j*iBuffXWidth) = (PixelC) *(pict+j*where().width+i);
				Xcheck1[dst_x] = 1 ;		
				dst_x++;
			}
		}
		for(j=0; j<(iHeightSrc-iExpandYRefFrame/iType);j++){			
			for(int i=0; i<iBuffXWidth+iExpandYRefFrame/iType;i++){	
				dst_x = (1<<h_factor_int);
				do{
					if(i<iBuffXWidth && j<iHeightSrcNet)		
						*(ppxlcBuffX2-(dst_x-1)+(i+1)*(1<<h_factor_int)-1+j*iBuffXWidth*(1<<h_factor_int)) = (PixelC) *(ppxlcBuffX+j*iBuffXWidth+i);
					if(dst_x !=1)	Xcheck2[(i+1)*(1<<h_factor_int)-1-(dst_x-1)] = 0;		
					else			Xcheck2[(i+1)*(1<<h_factor_int)-1-(dst_x-1)] = Xcheck1[i];		
					dst_x--;
				} while ( dst_x != 0);
			}
		}
	}
	else { 
		for(int j=0; j<(iHeightSrc-iExpandYRefFrame/iType);j++){		
			for(int i=0; i<iBuffXWidth+iExpandYRefFrame/iType;i++){
				dst_x = (1<<h_factor_int);
				do{
					if(i<iBuffXWidth && j<iHeightSrcNet)		
						*(ppxlcBuffX2-(dst_x-1)+(i+1)*(1<<h_factor_int)-1+j*iBuffXWidth*(1<<h_factor_int)) = (PixelC) *(pict+j*where().width+i);
					if(dst_x !=1)	Xcheck2[(i+1)*(1<<h_factor_int)-1-(dst_x-1)] = 0;		
					else			Xcheck2[(i+1)*(1<<h_factor_int)-1-(dst_x-1)] = 1; 
					dst_x--;
				} while ( dst_x != 0);
			}
		}
	}

	Int copy_line=0;
	Int dst_y=0;
	Int next_copy_line = 0 ;	
	if(v_factor - v_factor_int > 0.000001 ){
	  int j; // wmay
		for(j=0; j<(iHeightSrc-iExpandYRefFrame/iType);j++){		
			if(j == next_copy_line){
				if(j<iHeightSrcNet) {			
					for(int i=0; i<rctDst.width - iExpandYRefFrame/iType*2;i++)
						*(ppxlcBuffY+i+dst_y*iBuffXWidth*(1<<h_factor_int)) = (PixelC) *(ppxlcBuffX2+j*iBuffXWidth*(1<<h_factor_int)+i);
				}		
				Ycheck1[dst_y] = 0 ;
				dst_y++;
				copy_line++;
				next_copy_line = (int)floor	((	(double) (iVerticalSamplingFactorM*(1<<v_factor_int))/(iVerticalSamplingFactorN-iVerticalSamplingFactorM*(1<<v_factor_int))
															*copy_line));
			}
			if(j<iHeightSrcNet) {		
				for(int i=0; i<iBuffXWidth*(1<<h_factor_int);i++)
					*(ppxlcBuffY+i+dst_y*iBuffXWidth*(1<<h_factor_int)) = (PixelC) *(ppxlcBuffX2+j*iBuffXWidth*(1<<h_factor_int)+i);
			}		
			Ycheck1[dst_y] = 1 ;	
			dst_y++;
		}
		for(j=0; j<(iHeightDst-iExpandYRefFrame/iType*2)/(1<<v_factor_int)+iExpandYRefFrame/iType;j++){		
			dst_y = (1<<v_factor_int);
			do {
				if(j<(iHeightDst-iExpandYRefFrame/iType*2)/(1<<v_factor_int)){		
					for(int i=0; i<iBuffXWidth*(1<<h_factor_int);i++)
						*(ppxlcDst+(i+iExpandYRefFrame/iType)+((j+1)*(1<<v_factor_int)-1-(dst_y-1)+iExpandYRefFrame/iType)*rctDst.width) 
																		= (PixelC) *(ppxlcBuffY+j*iBuffXWidth*(1<<h_factor_int)+i);
				}		
				if(dst_y !=1)	Ycheck2[(j+1)*(1<<v_factor_int)-1-(dst_y-1)] = 0;		
				else			Ycheck2[(j+1)*(1<<v_factor_int)-1-(dst_y-1)] = Ycheck1[j];		
				dst_y--;
			} while ( dst_y != 0);
		}
	}
	else {	
		for(int j=0; j<(iHeightDst-iExpandYRefFrame/iType*2)/(1<<v_factor_int)+iExpandYRefFrame/iType;j++){
			dst_y = (1<<v_factor_int);
			do {
				if(j<(iHeightDst-iExpandYRefFrame/iType*2)/(1<<v_factor_int)){		
					for(int i=0; i<iBuffXWidth*(1<<h_factor_int);i++)
						*(ppxlcDst+(i+iExpandYRefFrame/iType)+((j+1)*(1<<v_factor_int)-1-(dst_y-1)+iExpandYRefFrame/iType)*rctDst.width) 
																		= (PixelC) *(ppxlcBuffX2+j*iBuffXWidth*(1<<h_factor_int)+i);
				}
				if(dst_y !=1)	Ycheck2[(j+1)*(1<<v_factor_int)-1-(dst_y-1)] = 0;	
				else			Ycheck2[(j+1)*(1<<v_factor_int)-1-(dst_y-1)] = 1; 
				dst_y--;
			} while ( dst_y != 0);
		}
	}		
	puciRet->m_pbHorSamplingChk = Xcheck2; 
	puciRet->m_pbVerSamplingChk = Ycheck2;

	delete puciBuffX;
	delete puciBuffX2;
	delete puciBuffY;

	return puciRet;

}
//~OBSS_SAIT_FIX000602
//~OBSS_SAIT_991015

own CU8Image* CU8Image::biInterpolate (UInt accuracy) const // bilinearly interpolate the vframe
{
	const CoordI left = where ().left * accuracy;
	const CoordI top = where ().top * accuracy;
	const CoordI right = where ().right * accuracy;
	const CoordI bottom = where ().bottom * accuracy;

	CU8Image* puciRet = new CU8Image (CRct (left, top, right, bottom));
	PixelC* ppxlcRet = (PixelC*) puciRet -> pixels ();
	for (CoordI y = top; y < bottom; y++) { // x-direction interpolation
		for (CoordI x = left; x < right; x++) {
			*ppxlcRet = pixel (x, y, accuracy);
			ppxlcRet++;
		}
	}
	return puciRet;
}

own CU8Image* CU8Image::transpose () const
{
	CRct rctDst = where ();
	rctDst.transpose ();
	CU8Image* puciDst = new CU8Image (rctDst);
	const PixelC* ppxlSrc = pixels ();
	PixelC* ppxlDstRow = (PixelC*) puciDst->pixels ();
	PixelC* ppxlDst;
	UInt height = where ().height ();
	for (CoordI iy = where ().top; iy < where ().bottom; iy++)	{
		ppxlDst = ppxlDstRow;
		for (CoordI ix = where ().left; ix < where ().right; ix++)	{
			*ppxlDst = *ppxlSrc++;
			ppxlDst += height;
		}
		ppxlDstRow++;
	}
	return puciDst;
}

own CU8Image* CU8Image::warp (const CAffine2D& aff) const // affine warp
{
	CSiteD stdLeftTopWarp = aff * CSiteD (where ().left, where ().top);
	CSiteD stdRightTopWarp = aff * CSiteD (where ().right, where ().top);
	CSiteD stdLeftBottomWarp = aff * CSiteD (where ().left, where ().bottom);
	CSiteD stdRightBottomWarp = aff * CSiteD (where ().right, where ().bottom);
	CRct rctWarp (stdLeftTopWarp, stdRightTopWarp, stdLeftBottomWarp, stdRightBottomWarp);

	CU8Image* puciRet = new CU8Image (rctWarp);
	PixelC* ppxlcRet = (PixelC*) puciRet -> pixels ();
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
				*ppxlcRet = pixel (src);
			ppxlcRet++;
		}
	}
	return puciRet;
}


own CU8Image* CU8Image::warp (const CPerspective2D& persp) const // perspective warp
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

	CU8Image* puciRet = new CU8Image (rctWarp);
	PixelC* ppxlcRet = (PixelC*) puciRet -> pixels ();
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
				*ppxlcRet = pixel (src);
			ppxlcRet++;
		}
	}
	return puciRet;
}

own CU8Image* CU8Image::warp (const CPerspective2D& persp, const CRct& rctWarp) const // perspective warp
{
	CU8Image* puciRet = new CU8Image (rctWarp);
	PixelC* ppxlcRet = (PixelC*) puciRet -> pixels ();
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
				*ppxlcRet = pixel (src);
			ppxlcRet++;
		}
	}
	return puciRet;
}

own CU8Image* CU8Image::warp (const CPerspective2D& persp, const CRct& rctWarp, const UInt accuracy) const // perspective warp
{
	CU8Image* puciRet = new CU8Image (rctWarp);
	PixelC* ppxlcRet = (PixelC*) puciRet -> pixels ();
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
				*ppxlcRet = pixel (src, accuracy);
			ppxlcRet++;
		}
	}
	return puciRet;
}

PixelC CU8Image::pixel (CoordD x, CoordD y) const
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

	const PixelC lt = pixel (left, top);
	const PixelC rt = pixel (right, top);
	const PixelC lb = pixel (left, bottom);
	const PixelC rb = pixel (right, bottom);
	const Double distX = x - left;
	const Double distY = y - top;
	Double x01 = distX * (rt - lt) + lt; // use p.59's notation (Wolberg, Digital Image Warping)
	Double x23 = distX * (rb - lb) + lb;
	PixelC pxlRet = checkrangeU8 ((U8) (x01 + (x23 - x01) * distY + .5), 0, 255);
	return pxlRet;
}

PixelC CU8Image::pixel (CoordI x, CoordI y, UInt accuracy) const
{
	UInt uis = 1 << (accuracy + 1);
	UInt accuracy1 = accuracy + 1;
	CoordD dx = (CoordD) ((CoordD) x / uis);
	CoordD dy = (CoordD) ((CoordD) y / uis);
	CoordI left = (CoordI) floor (dx); // find the coordinates of the four corners
	CoordI wLeft = where ().left, wRight1 = where ().right - 1, wTop = where ().top, wBottom1 = where ().bottom - 1;
	left = checkrange (left, wLeft, wRight1);
	CoordI right = (CoordI) ceil (dx);
	right = checkrange (right, wLeft, wRight1);
	CoordI top = (CoordI) floor (dy);
	top	= checkrange (top, wTop, wBottom1);
	CoordI bottom = (CoordI) ceil (dy);
	bottom = checkrange (bottom, wTop, wBottom1);

	UInt accuracy2 = (accuracy << 1) + 2;
	const PixelC lt = pixel (left, top);
	const PixelC rt = pixel (right, top);
	const PixelC lb = pixel (left, bottom);
	const PixelC rb = pixel (right, bottom);
	const CoordI distX = x - (left << accuracy1);
	const CoordI distY = y - (top << accuracy1);
	Int dRet = ((uis-distY)*((uis-distX)*lt + distX*rt) + distY*((uis-distX)*lb + distX*rb) + (1<<(accuracy2-1))) >> accuracy2;
	PixelC pxlRet = (U8) checkrange ((PixelC)dRet, 0U, 255U);
	return pxlRet;
}

own CU8Image* CU8Image::complement () const
{
	CU8Image* puciDst = new CU8Image (where(), (PixelC) transpValue);
	const PixelC* ppxlcSrc = pixels ();
	PixelC* ppxlcDst = (PixelC*) puciDst->pixels ();
	for (UInt iPxl = 0; iPxl < where ().area (); iPxl++)
		*ppxlcDst++ = *ppxlcSrc++ ^ 0xFF;
	return puciDst;
}

Void CU8Image::overlay (const CU8Image& uci)
{
	if (!valid () || !uci.valid () || uci.where ().empty ()) return;
	CRct r = m_rc; 
	r.include (uci.m_rc); // overlay is defined on union of rects
	where (r); 
	if (!valid ()) return; 
	assert (uci.m_ppxlc != NULL); 
	CRct rctFi = uci.m_rc;

	Int widthFi = rctFi.width;
	Int widthCurr = where ().width;
	PixelC* ppxlcThis = (PixelC*) pixels (rctFi.left, rctFi.top);
	const PixelC* ppxlcFi = uci.pixels ();
	for (CoordI y = rctFi.top; y < rctFi.bottom; y++) { // loop through VOP CRct
		memcpy (ppxlcThis, ppxlcFi, rctFi.width * sizeof (PixelC));
		ppxlcThis += widthCurr; 
		ppxlcFi += widthFi;
	}
}

Void CU8Image::overlay (const CU8Image& uci, const CRct& rctSrc)
{
	if (!valid () || !uci.valid () || uci.where ().empty () || !rctSrc.valid ()) return;
	if (!(uci.m_rc >= rctSrc))
		return;

	CRct r = m_rc; 
	r.include (rctSrc); // overlay is defined on union of rects
	where (r); 
	if (!valid ()) return; 

	assert (uci.m_ppxlc != NULL); 

	Int widthSrc = rctSrc.width;
	Int widthDst = where ().width;
	PixelC* ppxlcThis = (PixelC*) pixels (rctSrc.left, rctSrc.top);
	const PixelC* ppxlcSrc = uci.pixels (rctSrc.left, rctSrc.top);
	for (CoordI y = rctSrc.top; y < rctSrc.bottom; y++) { // loop through VOP CRct
		memcpy (ppxlcThis, ppxlcSrc, widthSrc * sizeof (PixelC));
		ppxlcThis += widthDst; 
		ppxlcSrc += widthSrc;
	}
}

own CU8Image* CU8Image::smooth_ (UInt window) const
{
	const UInt offset = window >> 1;
	const UInt offset2 = offset << 1;
	const UInt size = window * window; // array size to be sorted
	const UInt med = size >> 1;
	CU8Image* pfmgRet = new CU8Image (*this);
	// bound of the image to be filtered.
	const CoordI left = where ().left + offset;
	const CoordI top = where ().top + offset;
	const CoordI right = where ().right - offset;
	const CoordI bottom = where ().bottom - offset;
	const Int width = where ().width;
	const Int dist = offset + offset * width;
	const Int wwidth = width - window;
	PixelC* rgValues = new PixelC [size];
	PixelC* pRet = (PixelC*) pfmgRet -> pixels (left, top);
	const PixelC* p = pixels (left, top);
	for (CoordI y = top; y != bottom; y++) {
		for (CoordI x = left; x != right; x++) {
			const PixelC* pp = p - dist; // get correct index
			UInt numTransp = 0;
			for (UInt sy = 0; sy < window; sy++) {
				for (UInt sx = 0; sx < window; sx++) {
					if (*pp == transpValue)
						numTransp++;
					pp++;
				}
				pp += wwidth;
			}
			*pRet++ = (PixelC) ((numTransp <= med) ? opaqueValue : transpValue);
			p++;
		}
		pRet += offset2;
		p += offset2;
	}
	delete [] rgValues;
	return pfmgRet;
}

own CU8Image* CU8Image::smooth (UInt window) const
{
	UInt offset = window >> 1;
	CRct rctExp (where ());
	rctExp.expand (offset);
	CU8Image* puciExp = new CU8Image (*this, rctExp);
	CU8Image* puciSmooth = puciExp -> smooth_ (window);
	puciSmooth -> where (where ());
	delete puciExp;
	return puciSmooth;
}

Void CU8Image::CU8Image_xor (const CU8Image& uci)
{
	CRct rctIntersect = m_rc;
	rctIntersect.clip (uci.where());
	if (!rctIntersect.valid () || rctIntersect.empty ()) return;

	PixelC* ppxlcRowStart1 = (PixelC*) pixels (rctIntersect.left, rctIntersect.top);
	const PixelC* ppxlcRowStart2 = uci.pixels (rctIntersect.left, rctIntersect.top);
	for (CoordI iy = rctIntersect.top; iy < rctIntersect.bottom; iy++)	  {
		PixelC* ppxlc1 = ppxlcRowStart1;
		const PixelC* ppxlc2 = ppxlcRowStart2;
		for (CoordI ix = rctIntersect.left; ix < rctIntersect.right; ix++)	{
			assert (*ppxlc1 == transpValue || *ppxlc1 == opaqueValue);
			assert (*ppxlc2 == transpValue || *ppxlc2 == opaqueValue);		
			if (*ppxlc1 == *ppxlc2)	
				*ppxlc1 = (PixelC) transpValue;
			else 
				*ppxlc1 = (PixelC) opaqueValue;
			ppxlc1++;
			ppxlc2++;
		}
		ppxlcRowStart1 += where ().width;
		ppxlcRowStart2 += uci.where ().width;
	}
}

Void CU8Image::CU8Image_or (const CU8Image& uci)
{
	CRct rctIntersect = m_rc;
	rctIntersect.clip (uci.where());
	if (!rctIntersect.valid () || rctIntersect.empty ()) return;

	PixelC* ppxlcRowStart1 = (PixelC*) pixels (rctIntersect.left, rctIntersect.top);
	const PixelC* ppxlcRowStart2 = uci.pixels (rctIntersect.left, rctIntersect.top);
	for (CoordI iy = rctIntersect.top; iy < rctIntersect.bottom; iy++)	  {
		PixelC* ppxlc1 = ppxlcRowStart1;
		const PixelC* ppxlc2 = ppxlcRowStart2;
		for (CoordI ix = rctIntersect.left; ix < rctIntersect.right; ix++)	{
			assert (*ppxlc1 == transpValue || *ppxlc1 == opaqueValue);
			assert (*ppxlc2 == transpValue || *ppxlc2 == opaqueValue);		
			if (*ppxlc2 == opaqueValue)
				*ppxlc1 = opaqueValue;
			ppxlc1++;
			ppxlc2++;
		}
		ppxlcRowStart1 += where ().width;
		ppxlcRowStart2 += uci.where ().width;
	}
}

Void CU8Image::CU8Image_and (const CU8Image& uci)
{
	CRct rctIntersect = m_rc;
	rctIntersect.clip (uci.where());
	if (!rctIntersect.valid () || rctIntersect.empty ()) return;

	PixelC* ppxlcRowStart1 = (PixelC*) pixels (rctIntersect.left, rctIntersect.top);
	const PixelC* ppxlcRowStart2 = uci.pixels (rctIntersect.left, rctIntersect.top);
	for (CoordI iy = rctIntersect.top; iy < rctIntersect.bottom; iy++)	  {
		PixelC* ppxlc1 = ppxlcRowStart1;
		const PixelC* ppxlc2 = ppxlcRowStart2;
		for (CoordI ix = rctIntersect.left; ix < rctIntersect.right; ix++)	{
			assert (*ppxlc1 == transpValue || *ppxlc1 == opaqueValue);
			assert (*ppxlc2 == transpValue || *ppxlc2 == opaqueValue);		
			if (*ppxlc2 == transpValue)
				*ppxlc1 = transpValue;
			ppxlc1++;
			ppxlc2++;
		}
		ppxlcRowStart1 += where ().width;
		ppxlcRowStart2 += uci.where ().width;
	}
}


Void CU8Image::maskOut (const CU8Image& uci)
{
	CRct rctIntersect = m_rc;
	rctIntersect.clip (uci.where());
	if (!rctIntersect.valid () || rctIntersect.empty ()) return;

	PixelC* ppxlcRowStart = (PixelC*) pixels (rctIntersect.left, rctIntersect.top);
	const PixelC* ppxlcRowStartMask = uci.pixels (rctIntersect.left, rctIntersect.top);
	for (CoordI iy = rctIntersect.top; iy < rctIntersect.bottom; iy++)	  {
		PixelC* ppxlc = ppxlcRowStart;
		const PixelC* ppxlcMask = ppxlcRowStartMask;
		for (CoordI ix = rctIntersect.left; ix < rctIntersect.right; ix++)	{
			assert (*ppxlcMask == transpValue || *ppxlcMask == opaqueValue);
			assert (*ppxlc == transpValue || *ppxlc == opaqueValue);		
			if (*ppxlcMask != transpValue)
				*ppxlc = transpValue;
			ppxlc++;
			ppxlcMask++;
		}
		ppxlcRowStart += where ().width;
		ppxlcRowStartMask += uci.where ().width;
	}
}

Void CU8Image::mutiplyAlpha (const CU8Image& uci)
{
	CRct rctIntersect = m_rc;
	rctIntersect.clip (uci.where());
	if (!rctIntersect.valid () || rctIntersect.empty ()) return;

	PixelC* ppxlcRowStart = (PixelC*) pixels (rctIntersect.left, rctIntersect.top);
	const PixelC* ppxlcRowStartMask = uci.pixels (rctIntersect.left, rctIntersect.top);
	for (CoordI iy = rctIntersect.top; iy < rctIntersect.bottom; iy++)	  {
		PixelC* ppxlc = ppxlcRowStart;
		const PixelC* ppxlcMask = ppxlcRowStartMask;
		for (CoordI ix = rctIntersect.left; ix < rctIntersect.right; ix++)	{
			assert (*ppxlcMask == transpValue || *ppxlcMask == opaqueValue);
			if (*ppxlcMask == transpValue)
				*ppxlc = transpValue;
			else
				*ppxlc = (PixelC) ((*ppxlc * *ppxlcMask + 127) / opaqueValueF); //normalize
			ppxlc++;
			ppxlcMask++;
		}
		ppxlcRowStart += where ().width;
		ppxlcRowStartMask += uci.where ().width;

	}
}

Void CU8Image::cropOnAlpha ()
{
	CRct rctVisible = whereVisible ();
	where (rctVisible);				
}

Void CU8Image::operator = (const CU8Image& uci)
{
	delete [] m_ppxlc;
	copyConstruct (uci, uci.where ());
}

Void CU8Image::decimateBinaryShapeFrom (const CU8Image& uciSrc, const Bool bInterlace)  // decimate by rateX and rateY in a conservative way
{
	const CoordI left = uciSrc.where ().left / 2;
	const CoordI top = uciSrc.where ().top / 2;
	Int roundR = (uciSrc.where ().right >= 0) ? 1 : -1;
	Int roundB = (uciSrc.where ().bottom >= 0) ? 1 : -1;
	const CoordI right = (uciSrc.where ().right + roundR) / 2;
	const CoordI bottom = (uciSrc.where ().bottom + roundB) / 2;
	assert (where () == CRct (left, top, right, bottom));

	if(bInterlace){
		PixelC* ppxlc = (PixelC*) pixels ();
		const PixelC* ppxlcSrc11 = uciSrc.pixels ();
		const PixelC* ppxlcSrc12 = ppxlcSrc11 + 1;
		const PixelC* ppxlcSrc21 = ppxlcSrc11 + 2 * uciSrc.where ().width;
		const PixelC* ppxlcSrc22 = ppxlcSrc21 + 1;
		CoordI x, y;

		for (y = top; y < top + (bottom-top)/2 ; y++) {
			for (x = left; x < right; x++) {
				assert (*ppxlcSrc11 == transpValue || *ppxlcSrc11 == opaqueValue);
				assert (*ppxlcSrc12 == transpValue || *ppxlcSrc12 == opaqueValue);
				assert (*ppxlcSrc21 == transpValue || *ppxlcSrc21 == opaqueValue);
				assert (*ppxlcSrc22 == transpValue || *ppxlcSrc22 == opaqueValue);
				*ppxlc++ = *ppxlcSrc11 | *ppxlcSrc12 | *ppxlcSrc21 | *ppxlcSrc22;
				ppxlcSrc11 += 2;
				ppxlcSrc12 += 2;
				ppxlcSrc21 += 2;
				ppxlcSrc22 += 2;
			}
			for (x = left; x < right; x++) {
				assert (*ppxlcSrc11 == transpValue || *ppxlcSrc11 == opaqueValue);
				assert (*ppxlcSrc12 == transpValue || *ppxlcSrc12 == opaqueValue);
				assert (*ppxlcSrc21 == transpValue || *ppxlcSrc21 == opaqueValue);
				assert (*ppxlcSrc22 == transpValue || *ppxlcSrc22 == opaqueValue);
				*ppxlc++ = *ppxlcSrc11 | *ppxlcSrc12 | *ppxlcSrc21 | *ppxlcSrc22;
				ppxlcSrc11 += 2;
				ppxlcSrc12 += 2;
				ppxlcSrc21 += 2;
				ppxlcSrc22 += 2;
			}
			ppxlcSrc11 += 2 * uciSrc.where ().width;
			ppxlcSrc12 += 2 * uciSrc.where ().width;
			ppxlcSrc21 += 2 * uciSrc.where ().width;
			ppxlcSrc22 += 2 * uciSrc.where ().width;
		}
	}else{
		PixelC* ppxlc = (PixelC*) pixels ();
		const PixelC* ppxlcSrc11 = uciSrc.pixels ();
		const PixelC* ppxlcSrc12 = ppxlcSrc11 + 1;
		const PixelC* ppxlcSrc21 = ppxlcSrc11 + uciSrc.where ().width;
		const PixelC* ppxlcSrc22 = ppxlcSrc21 + 1;
		CoordI x, y;

		for (y = top; y < bottom; y++) {
			for (x = left; x < right; x++) {
				assert (*ppxlcSrc11 == transpValue || *ppxlcSrc11 == opaqueValue);
				assert (*ppxlcSrc12 == transpValue || *ppxlcSrc12 == opaqueValue);
				assert (*ppxlcSrc21 == transpValue || *ppxlcSrc21 == opaqueValue);
				assert (*ppxlcSrc22 == transpValue || *ppxlcSrc22 == opaqueValue);
				*ppxlc++ = *ppxlcSrc11 | *ppxlcSrc12 | *ppxlcSrc21 | *ppxlcSrc22;
				ppxlcSrc11 += 2;
				ppxlcSrc12 += 2;
				ppxlcSrc21 += 2;
				ppxlcSrc22 += 2;
			}
			ppxlcSrc11 += uciSrc.where ().width;
			ppxlcSrc12 += uciSrc.where ().width;
			ppxlcSrc21 += uciSrc.where ().width;
			ppxlcSrc22 += uciSrc.where ().width;
		}
	}
}

Bool CU8Image::operator == (const CU8Image& uci) const
{
	if (uci.where () != where ())
		return FALSE;
	UInt area = where ().area ();
	const PixelC* ppxlc = uci.pixels ();
	const PixelC* ppxlcThis = pixels ();
	for (UInt ip = 0; ip < area; ip++, ppxlc++, ppxlcThis++)
		if (*ppxlc != *ppxlcThis)
			return FALSE;
	return TRUE;
}

Double CU8Image::mse (const CU8Image& uciCompare) const
{
	assert (uciCompare.where () == where ());
	Int sqr = 0;
	const PixelC* ppxlcThis = pixels ();
	const PixelC* ppxlcCompare = uciCompare.pixels ();
	UInt area = where ().area ();
	for (UInt i = 0; i < area; i++) {
		Int diffR = *ppxlcThis - *ppxlcCompare;
		sqr += diffR * diffR;
		ppxlcThis++;
		ppxlcCompare++;
	}
	return ((Double) sqr / area);
}

Double CU8Image::mse (const CU8Image& uciCompare, const CU8Image& uciMsk) const
{
	assert (uciCompare.where () == where () && uciMsk.where () == where ());
	Int sqr = 0;
	const PixelC* ppxlcThis = pixels ();
	const PixelC* ppxlcCompare = uciCompare.pixels ();
	const PixelC* ppxlcMsk = uciMsk.pixels ();
	UInt area = where ().area ();
	UInt uiNonTransp = 0;
	for (UInt i = 0; i < area; i++) {
		if (*ppxlcMsk != transpValue) {
			Int diffR = *ppxlcThis - *ppxlcCompare;
			sqr += diffR * diffR;
			uiNonTransp++;
		}
		ppxlcThis++;
		ppxlcCompare++;
		ppxlcMsk++;
	}
	if (uiNonTransp == 0) 
		return 0.0;
	return ((Double) sqr / area);
}

Double CU8Image::snr (const CU8Image& uciCompare) const
{
	Double msError = mse (uciCompare);
	if (msError == 0.0)
		return 1000000.0;
	else 
		return (log10 (255 * 255 / msError) * 10.0);
}

Double CU8Image::snr (const CU8Image& uciCompare, const CU8Image& uciMsk) const
{
	CU8Image* puciMskOp = NULL;
	Double msError = 0;
	if (&uciMsk == NULL) {
		puciMskOp = new CU8Image (where (), (PixelC) opaqueValue);		
		msError = mse (uciCompare, *puciMskOp);
		delete puciMskOp;
	}
	else
		msError = mse (uciCompare, uciMsk);
	if (msError == 0.0)
		return 1000000.0;
	else 
		return (log10 (255 * 255 / msError) * 10.0);
}

// vdldump doesnt work for >8bit video
Void CU8Image::vdlDump (const Char* fileName, const CRct& rct) const
{
	CRct rctRegionOfInterest = (!rct.valid ()) ? where () : rct;
	assert (rctRegionOfInterest <= where ());
	if (rctRegionOfInterest == where ()) {
		CVideoObjectPlane vop (where (), opaquePixel);
		CPixel* ppxl = (CPixel*) vop.pixels ();
		const PixelC* ppxlc = pixels ();
		UInt area = where ().area ();
		for (UInt ip = 0; ip < area; ip++, ppxl++, ppxlc++)
			*ppxl = CPixel ((U8)*ppxlc, (U8)*ppxlc, (U8)*ppxlc, opaqueValue);
		vop.vdlDump (fileName);
	}
	else {
		CVideoObjectPlane vop (rct, opaquePixel);
		Int offset = where ().width - rct.width;
		CPixel* ppxl = (CPixel*) vop.pixels ();
		const PixelC* ppxlc = pixels (rctRegionOfInterest.left, rctRegionOfInterest.top);
		for (CoordI y = rctRegionOfInterest.top; y < rctRegionOfInterest.bottom; y++) {
			for (CoordI x = rctRegionOfInterest.left; x < rctRegionOfInterest.right; x++, ppxl++, ppxlc++) {
				*ppxl = CPixel ((U8)*ppxlc, (U8)*ppxlc, (U8)*ppxlc, opaqueValue);
			}
			ppxlc += offset;
		}
		vop.vdlDump (fileName);
	}
}
	
Void CU8Image::dump (FILE* pf, const CRct& rct, Int iScale) const
{
	CRct rctRegionOfInterest = (!rct.valid ()) ? where () : rct;
	assert (rctRegionOfInterest <= where ());
	iScale++;
	if(iScale!=256)
	{
		Int i,j;
		PixelC pxlTmp;
		const PixelC* ppxlc = pixels (rctRegionOfInterest.left, rctRegionOfInterest.top);
		for (i = rctRegionOfInterest.top; i < rctRegionOfInterest.bottom; i++)	{
			for(j = 0; j <rctRegionOfInterest.width; j++)
			{
				pxlTmp = (ppxlc[j]*iScale)>>8;
				fwrite(&pxlTmp, sizeof(PixelC), 1, pf);
			}
			ppxlc += where ().width;
		}
	}
	else
	{
		if (rctRegionOfInterest == where ()) {
	/* NBIT: change U8 to PixelC
			fwrite (m_ppxlc, sizeof (U8), where ().area (), pf);
	*/
			fwrite (m_ppxlc, sizeof (PixelC), where ().area (), pf);
		} else {
			const PixelC* ppxlc = pixels (rctRegionOfInterest.left, rctRegionOfInterest.top);
			for (Int i = rctRegionOfInterest.top; i < rctRegionOfInterest.bottom; i++)	{
	/* NBIT: change U8 to PixelC
				fwrite (ppxlc, sizeof (U8), rctRegionOfInterest.width, pf);
	*/
				fwrite (ppxlc, sizeof (PixelC), rctRegionOfInterest.width, pf);
				ppxlc += where ().width;
			}
		}
	}
}

Void CU8Image::dumpWithMask (FILE* pf, const CU8Image *puciMask, const CRct& rct, Int iScale, PixelC pxlZero) const
{
	PixelC pxlTmp;
	CRct rctRegionOfInterest = (!rct.valid ()) ? where () : rct;
	assert (rctRegionOfInterest <= where ());
	assert (rctRegionOfInterest <= puciMask->where ());
	Int x;
	const PixelC* ppxlc = pixels (rctRegionOfInterest.left, rctRegionOfInterest.top);
	const PixelC* ppxlcMask = puciMask->pixels(rctRegionOfInterest.left, rctRegionOfInterest.top);
	iScale++;
	for (Int i = rctRegionOfInterest.top; i < rctRegionOfInterest.bottom; i++)	{
		for(x = 0; x<rctRegionOfInterest.width; x++)
			if(ppxlcMask[x] == 0)
/* NBIT: change
				fputc(0, pf);
*/
				fwrite(&pxlZero, sizeof(PixelC), 1, pf);
			else if(iScale==256)
/* NBIT: change
				fputc(ppxlc[x], pf);
*/
				fwrite(&ppxlc[x], sizeof(PixelC), 1, pf);
			else
			{
				pxlTmp = (ppxlc[x] * iScale)>>8;
				fwrite(&pxlTmp, sizeof(PixelC), 1, pf);
			}

		ppxlc += where ().width;
		ppxlcMask += puciMask->where().width;
	}
}

Void CU8Image::txtDump (const Char* fileName) const
{
	FILE* pfTxt;
	const PixelC* ppxlc = pixels ();
	if (fileName != NULL)
		pfTxt = fopen (fileName, "w");
	else
		pfTxt = NULL;
	for (CoordI y = 0; y < where ().height (); y++) {
		for (CoordI x = 0; x < where ().width; x++) {
			if (pfTxt != NULL)
				fprintf (pfTxt, "%3d  ", *ppxlc);
			else
				printf ("%3d  ", *ppxlc);
			ppxlc++;
		}
		if (pfTxt != NULL)
			fprintf (pfTxt, "\n");
		else
			printf ("\n");
	}
	if (pfTxt != NULL)
		fclose (pfTxt);
}


Void CU8Image::txtDump (FILE *pf) const
{
	const PixelC *ppxlc = pixels();
	for (CoordI y = 0; y < where ().height (); y++) {
		for (CoordI x = 0; x < where ().width; x++) {
			fprintf (pf, "%3d ", *ppxlc);
			ppxlc++;
		}
		fprintf (pf,"\n");
	}
	fprintf (pf,"\n");
}

Void CU8Image::txtDumpMask (FILE *pf) const
{
	const PixelC *ppxlc = pixels ();
	for (CoordI y = 0; y < where ().height (); y++) {
		for (CoordI x = 0; x < where ().width; x++) {
			if (*ppxlc == transpValue)
				fprintf (pf, "..");
			else
				fprintf (pf, "[]");
			ppxlc++;
		}
		fprintf (pf,"\n");
	}
	fprintf (pf,"\n");
}


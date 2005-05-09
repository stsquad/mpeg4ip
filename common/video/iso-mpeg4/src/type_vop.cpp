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

	vop.hpp

Abstract:

	class for Video Object Plane 

Revision History:

*************************************************************************/

#include <math.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "basic.hpp"
#include "transf.hpp"
#include "warp.hpp"
#include "vop.hpp"
#include "typeapi.h"

#ifdef __MFC_
#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

#define new DEBUG_NEW				   
#endif // __MFC_

Int numPln = 4;

CVideoObjectPlane::~CVideoObjectPlane ()
{
	delete [] m_ppxl;
}

Void CVideoObjectPlane::allocate (CRct r, CPixel pxl) 
{
	m_rc = r;
	delete [] m_ppxl, m_ppxl = NULL;

	// allocate pixels and initialize
	if (m_rc.empty()) return;
	m_ppxl = new CPixel [m_rc.area ()];
	for (UInt i = 0; i < m_rc.area (); i++)
		m_ppxl [i] = pxl;
//	memset (m_ppxl, pxl, m_rc.area () * sizeof (CPixel));
}

Void CVideoObjectPlane::copyConstruct (const CVideoObjectPlane& vop, CRct r) 
{
	if (!r.valid()) 
		r = vop.where ();
	if (!vop.valid () || (!vop.m_rc.empty () && vop.m_ppxl == NULL))
		assert (FALSE);
	allocate (r, transpPixel);
	if (!valid ()) return; 

	// Copy data
	if (r == vop.where ())
		memcpy (m_ppxl, vop.pixels (), m_rc.area () * sizeof (PixelF));
	else {
		r.clip (vop.where ()); // find the intersection
		CoordI x = r.left; // copy pixels
		Int cbLine = r.width * numPln;
		CPixel* ppxl = (CPixel*) pixels (x, r.top);
		const CPixel* ppxlVop = vop.pixels (x, r.top);
		Int widthCurr = where ().width;
		Int widthVop = vop.where ().width;
		for (CoordI y = r.top; y < r.bottom; y++) {
			memcpy (ppxl, ppxlVop, cbLine);
			ppxl += widthCurr;
			ppxlVop += widthVop;
		}
	}
}

Void CVideoObjectPlane::swap (CVideoObjectPlane& vop) 
{
	assert (this && &vop);
	CRct rcT = vop.m_rc; 
	vop.m_rc = m_rc; 
	m_rc = rcT; 
	CPixel* ppxlT = vop.m_ppxl; 
	vop.m_ppxl = m_ppxl; 
	m_ppxl = ppxlT; 
}

CVideoObjectPlane::CVideoObjectPlane (const CVideoObjectPlane& vop, CRct r) : m_ppxl (NULL)
{
	copyConstruct (vop, r);
}

CVideoObjectPlane::CVideoObjectPlane (CRct r, CPixel pxl) : m_ppxl (NULL)
{
	allocate (r, pxl);
}

CVideoObjectPlane::CVideoObjectPlane (const Char* vdlFileName) : m_ppxl (NULL)
{
	FILE* pf = fopen (vdlFileName, "rb");
	// write overhead
	Int c0 = getc (pf);
	Int c1 = getc (pf);
	assert (c0 == 'V' && (c1 == 'M' || c1 == 'B') );
	CRct rc;
	if (c1 == 'M'){
		fread (&rc.left, sizeof (CoordI), 1, pf);
		fread (&rc.top, sizeof (CoordI), 1, pf);
		fread (&rc.right, sizeof (CoordI), 1, pf);
		fread (&rc.bottom, sizeof (CoordI), 1, pf);
		rc.width = rc.right - rc.left;
	}
	else {
		U8 byte1, byte2;
		Int sign;
		byte1 = getc (pf);
		byte2 = getc (pf);
		sign = ((byte1 / 128) == 1) ? 1 : -1;
		rc.left = byte1 % 128;
		rc.left = (rc.left * 256) + byte2;
		rc.left *= sign; 

		byte1 = getc (pf);
		byte2 = getc (pf);
		sign = ((byte1 / 128) > 0) ? 1 : -1;
		rc.top = byte1 % 128;
		rc.top = (rc.top * 256) + byte2;
		rc.top *= sign; 

		byte1 = getc (pf);
		byte2 = getc (pf);
		sign = ((byte1 / 128) > 0) ? 1 : -1;
		rc.right = byte1 % 128;
		rc.right = (rc.right * 256) + byte2;
		rc.right *= sign; 

		byte1 = getc (pf);
		byte2 = getc (pf);
		sign = ((byte1 / 128) > 0) ? 1 : -1;
		rc.bottom = byte1 % 128;
		rc.bottom = (rc.bottom * 256) + byte2;
		rc.bottom *= sign;
		rc.width = rc.right - rc.left;
	}
	allocate (rc);

	// read the actual data
	fread (m_ppxl, sizeof (CPixel), where ().area (), pf);
	fclose (pf);
}

CVideoObjectPlane::CVideoObjectPlane (
	const Char* pchFileName, 
	UInt ifr, 
	const CRct& rct,
	ChromType chrType,
	Int nszHeader
) : m_ppxl (NULL)
{
	assert (!rct.empty ());
	assert (ifr >= 0);
	assert (nszHeader >= 0);
	Int iWidth = rct.width, iHeight = rct.height ();
	UInt area = rct.area ();
	Int iUVWidth = iWidth, iUVHeight = iHeight;
	Int uiXSubsample = 1, uiYSubsample = 1;
	if (chrType == FOUR_TWO_TWO)	{
		uiXSubsample = 2;
		iUVWidth = (iWidth + 1) / uiXSubsample;
	}
	else if (chrType == FOUR_TWO_ZERO)	{
		uiXSubsample = 2;
		uiYSubsample = 2;
		iUVWidth = (iWidth + 1) / uiXSubsample;
		iUVHeight = (iHeight + 1) / uiYSubsample;
	}	
	UInt iUVArea = iUVWidth * iUVHeight;

	// allocate mem for buffers
	U8* pchyBuffer = new U8 [area];
	U8* pchuBuffer = new U8 [iUVArea];
	U8* pchvBuffer = new U8 [iUVArea];
	U8* pchyBuffer0 = pchyBuffer;
	U8* pchuBuffer0 = pchuBuffer;
	U8* pchvBuffer0 = pchvBuffer;

	// read data from a file
	FILE* fpYuvSrc = fopen (pchFileName, "rb");
	assert (fpYuvSrc != NULL);
	UInt skipSize = (UInt) ((Float) (ifr * sizeof (U8) * (area + 2 * iUVArea)));
	fseek (fpYuvSrc, nszHeader + skipSize, SEEK_SET);
	Int size = (Int) fread (pchyBuffer, sizeof (U8), area, fpYuvSrc);
	assert (size != 0);
	size = (Int) fread (pchuBuffer, 1, iUVArea, fpYuvSrc);
	assert (size != 0);
	size = (Int) fread (pchvBuffer, 1, iUVArea, fpYuvSrc);
	assert (size != 0);
	fclose(fpYuvSrc);

	// assign buffers to a vframe
	allocate (rct, opaquePixel); 
	CPixel* p = (CPixel*) pixels ();
	for (CoordI y = 0; y < iHeight; y++) {					
		if ((y % uiYSubsample) == 1) { 
			pchuBuffer -= iUVWidth; 
			pchvBuffer -= iUVWidth;	
		}
		for (CoordI x = 0; x < iWidth; x++) {				
			p -> pxlU.yuv.y = *pchyBuffer++;
			p -> pxlU.yuv.u = *pchuBuffer;
			p -> pxlU.yuv.v = *pchvBuffer;
			if (chrType == FOUR_FOUR_FOUR || (x % uiXSubsample) != 0)	{
				pchuBuffer++;
				pchvBuffer++;
			}
			p++;
		}
	}
	delete [] pchyBuffer0;
	delete [] pchuBuffer0;
	delete [] pchvBuffer0;
}

Void CVideoObjectPlane::where (const CRct& r) 
{
	if (!valid ()) return; 
	if (where () == r) return; 
	CVideoObjectPlane* pvop = new CVideoObjectPlane (*this, r);
	swap (*pvop);
	delete pvop;
}

CRct CVideoObjectPlane::whereVisible() const 
{
	if (!valid () || !m_rc.valid ()) return CRct ();

	CoordI left = where ().right - 1;
	CoordI top = where ().bottom - 1;
	CoordI right = where ().left;
	CoordI bottom = where ().top;
	const CPixel* ppxlThis = pixels ();
	for (CoordI y = where ().top; y < where ().bottom; y++) {
		for (CoordI x = where ().left; x < where ().right; x++) {
			if (ppxlThis -> pxlU.rgb.a != transpValue) {
				left = min (left, x);
				top = min (top, y);
				right = max (right, x);
				bottom = max (bottom, y);
			}
			ppxlThis++;
		}
	}
	right++;
	bottom++;
	return CRct (left, top, right, bottom);

	if (!valid () || !m_rc.valid ()) return CRct ();}

CPixel CVideoObjectPlane::pixel (CoordI x, CoordI y, UInt accuracy_ori) const
{	
	UInt accuracy1 = accuracy_ori + 1;
	CoordI left = (CoordI) floor ((Double) (x >> accuracy1)); // find the coordinates of the four corners
	CoordI wLeft = where ().left, wRight1 = where ().right - 1, wTop = where ().top, wBottom1 = where ().bottom - 1;
	left = checkrange (left, wLeft, wRight1);
	CoordI right = (CoordI) ceil ((Double) (x >> accuracy1));
	right = checkrange (right, wLeft, wRight1);
	CoordI top = (CoordI) floor ((Double) (y >> accuracy1));
	top	= checkrange (top, wTop, wBottom1);
	CoordI bottom = (CoordI) ceil ((Double) (y >> accuracy1));
	bottom = checkrange (bottom, wTop, wBottom1);

	UInt accuracy2 = (accuracy_ori << 1) + 2;
	const CPixel lt = pixel (left, top);
	const CPixel rt = pixel (right, top);
	const CPixel lb = pixel (left, bottom);
	const CPixel rb = pixel (right, bottom);
	const Int distX = (x - left) << accuracy1;
	const Int distY = (y - top) << accuracy1;
	Int x01 = (distX * (rt.pxlU.rgb.r - lt.pxlU.rgb.r) + (Int) lt.pxlU.rgb.r) << accuracy1; // use p.59's notation (Wolberg, Digital Image Warping)
	Int x23 = (distX * (rb.pxlU.rgb.r - lb.pxlU.rgb.r) + (Int) lb.pxlU.rgb.r) << accuracy1; 
	Int r = checkrange (((x01 << accuracy1) + (x23 - x01) * distY) >> accuracy2, 0, 255);
	x01 = (distX * (rt.pxlU.rgb.g - lt.pxlU.rgb.g) + (Int) lt.pxlU.rgb.g) << accuracy1; // use p.59's notation
	x23 = (distX * (rb.pxlU.rgb.g - lb.pxlU.rgb.g) + (Int) lb.pxlU.rgb.g) << accuracy1;
	Int g = checkrange (((x01 << accuracy1) + (x23 - x01) * distY) >> accuracy2, 0, 255);
	x01 = (distX * (rt.pxlU.rgb.b - lt.pxlU.rgb.b) + (Int) lt.pxlU.rgb.b) << accuracy1; // use p.59's notation
	x23 = (distX * (rb.pxlU.rgb.b - lb.pxlU.rgb.b) + (Int) lb.pxlU.rgb.b) << accuracy1;
	Int b = checkrange (((x01 << accuracy1) + (x23 - x01) * distY) >> accuracy2, 0, 255);
	x01 = (distX * (rt.pxlU.rgb.a - lt.pxlU.rgb.a) + (Int) lt.pxlU.rgb.a) << accuracy1; // use p.59's notation
	x23 = (distX * (rb.pxlU.rgb.a - lb.pxlU.rgb.a) + (Int) lb.pxlU.rgb.a) << accuracy1;
	Int a = checkrange (((x01 << accuracy1) + (x23 - x01) * distY) >> accuracy2, 0, 255);
	return CPixel ((U8) r, (U8) g, (U8) b, (U8) a);
}

CPixel CVideoObjectPlane::pixel (CoordD x, CoordD y) const
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

	const CPixel lt = pixel (left, top);
	const CPixel rt = pixel (right, top);
	const CPixel lb = pixel (left, bottom);
	const CPixel rb = pixel (right, bottom);
	const Double distX = x - left;
	const Double distY = y - top;
	Double x01 = distX * (rt.pxlU.rgb.r - lt.pxlU.rgb.r) + lt.pxlU.rgb.r; // use p.59's notation (Wolberg, Digital Image Warping)
	Double x23 = distX * (rb.pxlU.rgb.r - lb.pxlU.rgb.r) + lb.pxlU.rgb.r;
	Int r = checkrange ((Int) (x01 + (x23 - x01) * distY + .5), 0, 255);
	x01 = distX * (rt.pxlU.rgb.g - lt.pxlU.rgb.g) + lt.pxlU.rgb.g; // use p.59's notation
	x23 = distX * (rb.pxlU.rgb.g - lb.pxlU.rgb.g) + lb.pxlU.rgb.g;
	Int g = checkrange ((Int) (x01 + (x23 - x01) * distY + .5), 0, 255);
	x01 = distX * (rt.pxlU.rgb.b - lt.pxlU.rgb.b) + lt.pxlU.rgb.b; // use p.59's notation
	x23 = distX * (rb.pxlU.rgb.b - lb.pxlU.rgb.b) + lb.pxlU.rgb.b;
	Int b = checkrange ((Int) (x01 + (x23 - x01) * distY + .5), 0, 255);
	x01 = distX * (rt.pxlU.rgb.a - lt.pxlU.rgb.a) + lt.pxlU.rgb.a; // use p.59's notation
	x23 = distX * (rb.pxlU.rgb.a - lb.pxlU.rgb.a) + lb.pxlU.rgb.a;
	Int a = checkrange ((Int) (x01 + (x23 - x01) * distY + .5), 0, 255);
	return CPixel ((U8) r, (U8) g, (U8) b, (U8) a);
}

own CVideoObjectPlane* CVideoObjectPlane::warp (const CAffine2D& aff) const // affine warp
{
	CSiteD stdLeftTopWarp = aff * CSiteD (where ().left, where ().top);
	CSiteD stdRightTopWarp = aff * CSiteD (where ().right, where ().top);
	CSiteD stdLeftBottomWarp = aff * CSiteD (where ().left, where ().bottom);
	CSiteD stdRightBottomWarp = aff * CSiteD (where ().right, where ().bottom);
	CRct rctWarp (stdLeftTopWarp, stdRightTopWarp, stdLeftBottomWarp, stdRightBottomWarp);

	CVideoObjectPlane* pvopRet = new CVideoObjectPlane (rctWarp);
	CPixel* ppxlRet = (CPixel*) pvopRet -> pixels ();
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
				*ppxlRet = pixel (src);
			ppxlRet++;
		}
	}
	return pvopRet;
}

own CVideoObjectPlane* CVideoObjectPlane::warp (const CAffine2D& aff, const CRct& rctWarp) const // affine warp
{
	CVideoObjectPlane* pvopRet = new CVideoObjectPlane (rctWarp);
	CPixel* ppxlRet = (CPixel*) pvopRet -> pixels ();
	//CAffine2D affInv = aff.inverse ();
	for (CoordI y = rctWarp.top; y < rctWarp.bottom; y++) {
		for (CoordI x = rctWarp.left; x < rctWarp.right; x++) {
			CSiteD src = aff * CSiteD (x, y); 
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
				*ppxlRet = pixel (src);
			ppxlRet++;
		}
	}
	return pvopRet;
}

own CVideoObjectPlane* CVideoObjectPlane::warp (const CPerspective2D& persp) const // perspective warp
{
	CSiteD stdLeftTopWarp = (persp * CSiteD (where ().left, where ().top)).s;
	CSiteD stdRightTopWarp = (persp * CSiteD (where ().right, where ().top)).s;
	CSiteD stdLeftBottomWarp = (persp * CSiteD (where ().left, where ().bottom)).s;
	CSiteD stdRightBottomWarp = (persp * CSiteD (where ().right, where ().bottom)).s;
	CRct rctWarp (stdLeftTopWarp, stdRightTopWarp, stdLeftBottomWarp, stdRightBottomWarp);
	CVideoObjectPlane* pvopRet = new CVideoObjectPlane (rctWarp);
	CPixel* ppxlRet = (CPixel*) pvopRet -> pixels ();
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
				*ppxlRet = pixel (src);
			ppxlRet++;
		}
	}
	return pvopRet;
}		 

own CVideoObjectPlane* CVideoObjectPlane::warp (const CPerspective2D& persp, const CRct& rctWarp) const // perspective warp
{
	CVideoObjectPlane* pvopRet = new CVideoObjectPlane (rctWarp);
	CPixel* ppxlRet = (CPixel*) pvopRet -> pixels ();
	//CPerspective2D perspInv = persp.inverse ();
	for (CoordI y = rctWarp.top; y < rctWarp.bottom; y++) {
		for (CoordI x = rctWarp.left; x < rctWarp.right; x++) {
			CSiteD src = (persp * CSiteD (x, y)).s; 
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
				*ppxlRet = pixel (src);
			ppxlRet++;
		}
	}
	return pvopRet;
}		 

own CVideoObjectPlane* CVideoObjectPlane::warp (const CPerspective2D& persp, const CRct& rctWarp, UInt accuracy) const // perspective warp
{
	UInt accuracy1 = accuracy + 1;
	CVideoObjectPlane* pvopRet = new CVideoObjectPlane (rctWarp);
	CPixel* ppxlRet = (CPixel*) pvopRet -> pixels ();
	for (CoordI y = rctWarp.top; y < rctWarp.bottom; y++) {
		for (CoordI x = rctWarp.left; x < rctWarp.right; x++) {
			CSite src = (persp * (CSite (x, y))).s; 
			CoordI fx = (CoordI) floor ((Double) (src.x >> accuracy1)); 
			CoordI fy = (CoordI) floor ((Double) (src.y >> accuracy1)); 
			CoordI cx = (CoordI) ceil ((Double) (src.x >> accuracy1));
			CoordI cy = (CoordI) ceil ((Double) (src.y >> accuracy1));
			if (
				where ().includes (fx, fy) && 
				where ().includes (fx, cy) && 
				where ().includes (cx, fy) && 
				where ().includes (cx, cy)
			)
			*ppxlRet = pixel (src, accuracy);
			ppxlRet++;
		}
	}
	return pvopRet;
}		 

Void CVideoObjectPlane::multiplyAlpha ()
{
	if (!valid ()) return;
	CPixel* ppxlThis = (CPixel*) pixels ();
	Float inv255 = (Float) 1. / (Float) 255.;
	UInt area = where ().area ();
	for (UInt i = 0; i < area; i++) {
		Float alpha = inv255 * ppxlThis -> pxlU.rgb.a;
		ppxlThis -> pxlU.rgb.r = (U8) (alpha * ppxlThis -> pxlU.rgb.r + .5);
		ppxlThis -> pxlU.rgb.g = (U8) (alpha * ppxlThis -> pxlU.rgb.g + .5);
		ppxlThis -> pxlU.rgb.b = (U8) (alpha * ppxlThis -> pxlU.rgb.b + .5);
		ppxlThis++;
	}
} 

Void CVideoObjectPlane::multiplyBiAlpha ()
{
	if (!valid ()) return;
	CPixel* ppxlThis = (CPixel*) pixels ();
	UInt area = where ().area ();
	for (UInt i = 0; i < area; i++) {
		if (ppxlThis -> pxlU.rgb.a == 0) {
			ppxlThis -> pxlU.rgb.r = 0;
			ppxlThis -> pxlU.rgb.g = 0;
			ppxlThis -> pxlU.rgb.b = 0;
		}
		ppxlThis++;
	}
} 

Void CVideoObjectPlane::unmultiplyAlpha ()
{
	if (!valid ()) return;
	CPixel* ppxlThis = (CPixel*) pixels ();
	UInt area = where ().area ();
	for (UInt i = 0; i < area; i++) {
		if (ppxlThis->pxlU.rgb.a != 0)	{			
			Float invAlpha = (Float) (255 / ppxlThis -> pxlU.rgb.a);
			ppxlThis -> pxlU.rgb.r = (U8) checkrange (invAlpha * ppxlThis -> pxlU.rgb.r + 0.5f, 0.0F, 255.0F);
			ppxlThis -> pxlU.rgb.g = (U8) checkrange (invAlpha * ppxlThis -> pxlU.rgb.g + 0.5f, 0.0F, 255.0F);
			ppxlThis -> pxlU.rgb.b = (U8) checkrange (invAlpha * ppxlThis -> pxlU.rgb.b + 0.5f, 0.0F, 255.0F);
		}
		ppxlThis++;
	}
} 

Void CVideoObjectPlane::thresholdAlpha (U8 uThresh)
{
	CPixel* ppxlThis = (CPixel*) pixels ();
	UInt area = where ().area ();
	for (UInt i = 0; i < area; i++) {
		ppxlThis -> pxlU.rgb.a = (ppxlThis -> pxlU.rgb.a > uThresh) ? opaqueValue : transpValue;
		ppxlThis++;
	}
} 


Void CVideoObjectPlane::thresholdRGB (U8 uThresh)
{
	CPixel* ppxlThis = (CPixel*) pixels ();
	UInt area = where ().area ();
	for (UInt i = 0; i < area; i++) {
		if (ppxlThis -> pxlU.rgb.r < uThresh && ppxlThis -> pxlU.rgb.g < uThresh && ppxlThis -> pxlU.rgb.b	< uThresh) {
			ppxlThis -> pxlU.rgb.r = 0;
			ppxlThis -> pxlU.rgb.g = 0;
			ppxlThis -> pxlU.rgb.b = 0;
		}
		ppxlThis++;
	}
} 


Void CVideoObjectPlane::cropOnAlpha ()
{
	CoordI left = where ().right - 1;
	CoordI top = where ().bottom - 1;
	CoordI right = where ().left;
	CoordI bottom = where ().top;
	const CPixel* ppxlThis = pixels ();
	for (CoordI y = where ().top; y < where ().bottom; y++) {
		for (CoordI x = where ().left; x < where ().right; x++) {
			if (ppxlThis -> pxlU.rgb.a != transpValue) {
				left = min (left, x);
				top = min (top, y);
				right = max (right, x);
				bottom = max (bottom, y);
			}
			ppxlThis++;
		}
	}
	right++;
	bottom++;
	where (CRct (left, top, right, bottom));
} 


own CVideoObjectPlane* CVideoObjectPlane::decimate (UInt rateX, UInt rateY) const
{
	const CoordI left = where ().left / (CoordI) rateX;
	const CoordI top = where ().top / (CoordI)  rateY;
	const CoordI right = 
		(where ().right >= 0) ? (where ().right + (CoordI) rateX - 1) / (CoordI) rateX :
		(where ().right - (CoordI) rateX + 1) / (CoordI) rateX;
	const CoordI bottom = 
		(where ().bottom >= 0) ? (where ().bottom + (CoordI) rateX - 1) / (CoordI) rateY :
		(where ().bottom - (CoordI) rateX + 1) / (CoordI) rateY;	

	CVideoObjectPlane* pvopRet = new CVideoObjectPlane (CRct (left, top, right, bottom), opaquePixel);
	CPixel* pret = (CPixel*) pvopRet -> pixels ();
	const CPixel* pori = pixels ();
	Int skipY = rateY * where ().width;
	for (CoordI y = top; y != bottom; y++)
	{
		const CPixel* pvf = pori;
		for (CoordI x = left; x != right; x++)
		{
			*pret++ = *pvf;
			pvf += rateX;
		} 
		pori += skipY;
	}
	return pvopRet;
}

Void CVideoObjectPlane::falseColor (CPixel pxl)
{
	CPixel* ppxl = m_ppxl;
	UInt area = where ().area ();
	for (UInt i = 0; i < area; i++) {
		if (ppxl -> pxlU.rgb.a == transpValue) {
			ppxl -> pxlU.rgb.r = pxl.pxlU.rgb.r;
			ppxl -> pxlU.rgb.g = pxl.pxlU.rgb.g;
			ppxl -> pxlU.rgb.b = pxl.pxlU.rgb.b;
		}
		ppxl++;
	}
}

Void CVideoObjectPlane::falseColor (U8 r, U8 g, U8 b)
{
	CPixel* ppxl = m_ppxl;
	UInt area = where ().area ();
	for (UInt i = 0; i < area; i++) {
		if (ppxl -> pxlU.rgb.a == transpValue) {
			ppxl -> pxlU.rgb.r = r;
			ppxl -> pxlU.rgb.g = g;
			ppxl -> pxlU.rgb.b = b;
		}
		ppxl++;
	}
}

own CVideoObjectPlane* CVideoObjectPlane::zoomup (UInt rateX, UInt rateY) const
{
	const CoordI left = where ().left * rateX;
	const CoordI top = where ().top * rateY;
	const CoordI right = where ().right * rateX;
	const CoordI bottom = where ().bottom * rateY;

	CVideoObjectPlane* retVf = new CVideoObjectPlane (CRct (left, top, right, bottom), opaquePixel);
	CPixel* pret = (CPixel*) retVf -> pixels ();
	for (CoordI y = top; y != bottom; y++)
	{
		for (CoordI x = left; x != right; x++)
		{
			*pret++ = pixel ((CoordI) (x / rateX), (CoordI)  (y / rateY));
		}
	}
	return retVf;
}


own CVideoObjectPlane* CVideoObjectPlane::expand (UInt rateX, UInt rateY) const
{
	const CoordI left = where ().left; // left-top coordinate remain the same
	const CoordI top = where ().top;
	const CoordI right = left + where ().width * rateX;
	const CoordI bottom = top + where ().height () * rateY;

	CVideoObjectPlane* pvopRet = new CVideoObjectPlane (CRct (left, top, right, bottom), transpPixel);
	CPixel* ppxlRet = (CPixel*) pvopRet -> pixels ();
	const CPixel* ppxlThis = pixels ();
	for (CoordI y = top; y != bottom; y++) {
		for (CoordI x = left; x != right; x++) {
			if (x % rateX == 0 && y % rateY == 0) 
				*ppxlRet++ = *ppxlThis++;
			else
				*ppxlRet++ = CPixel (0, 0, 0, opaqueValue);
		}
	}
	return pvopRet;
}


inline CPixel averageP (const CPixel& p1, const CPixel& p2)
{
	return CPixel (
		(p1.pxlU.rgb.r + p2.pxlU.rgb.r + 1) >> 1,
		(p1.pxlU.rgb.g + p2.pxlU.rgb.g + 1) >> 1,
		(p1.pxlU.rgb.b + p2.pxlU.rgb.b + 1) >> 1,
		p1.pxlU.rgb.a
	);
}


own CVideoObjectPlane* CVideoObjectPlane::biInterpolate () const
{
	const CoordI left = where ().left; // left-top coordinate remain the same
	const CoordI top = where ().top;
	const CoordI right = left + where ().width * 2;
	const CoordI bottom = top + where ().height () * 2;
	const CoordI width = right - left;

	CoordI x, y;
	CVideoObjectPlane* pvopRet = new CVideoObjectPlane (CRct (left, top, right, bottom), opaquePixel);
	CPixel* ppxlRet = (CPixel*) pvopRet -> pixels ();
	const CPixel* ppxl = pixels ();
	const CoordI right1 = right - 2;
	for (y = top; y < bottom; y += 2) { // x-direction interpolation
		for (x = left; x < right1; x += 2) {
			*ppxlRet++ = *ppxl++;
			*ppxlRet++ = averageP (*ppxl, *(ppxl - 1));
		}
		*ppxlRet++ = *ppxl;
		*ppxlRet++ = *ppxl++; // the last pixel of every row do not need average
		ppxlRet += width;
	}
	ppxlRet = (CPixel*) pvopRet -> pixels ();
	ppxlRet += width; // start from the second row
	const CoordI width2 = width << 1;
	const CoordI bottom1 = bottom - 1;
	for (x = left; x < right; x++) { // y-direction interpolation
		CPixel* ppxlRett = ppxlRet++;
		for (y = top + 1; y < bottom1; y += 2) {
			*ppxlRett = averageP (*(ppxlRett - width), *(ppxlRett + width));
			ppxlRett += width2;
		}
		*ppxlRett = *(ppxlRett - width); // the last pixel of every column do not need average
	}
	return pvopRet;
}

own CVideoObjectPlane* CVideoObjectPlane::biInterpolate (UInt accuracy) const
{
	const CoordI left = where ().left * accuracy;
	const CoordI top = where ().top * accuracy;
	const CoordI right = where ().right * accuracy;
	const CoordI bottom = where ().bottom * accuracy;

	CVideoObjectPlane* pvopRet = new CVideoObjectPlane (CRct (left, top, right, bottom));
	CPixel* ppxlRet = (CPixel*) pvopRet -> pixels ();
	for (CoordI y = top; y < bottom; y++) { // x-direction interpolation
		for (CoordI x = left; x < right; x++) {
			*ppxlRet = pixel (x, y, accuracy);
			ppxlRet++;
		}
	}
	return pvopRet;
}


own CVideoObjectPlane* CVideoObjectPlane::biInterpolate (const CRct& r) const
{
	CVideoObjectPlane* pvopRet = new CVideoObjectPlane (r, opaquePixel);
	for (CoordI x = r.left; x < r.right; x++)
	for (CoordI y = r.top; y < r.bottom; y++) {
		CoordD dx = (Double) ((where ().right - 1 - where ().left) / (Double) (r.right  - 1 - r.left)) * (x - r.left) + where ().left;
		CoordD dy = (Double) ((where ().bottom - 1 - where ().top) / (Double) (r.bottom - 1 - r.top)) * (y - r.top) + where ().top;
		CPixel result = pixel (dx, dy);
		pvopRet -> pixel (x, y, result); 
	}
	return pvopRet;
}

own CFloatImage* CVideoObjectPlane::plane (RGBA pxlCom) const
{
	if (!valid ()) return NULL;
	CFloatImage* pfiRet = new CFloatImage (where ());
	const CPixel* ppxlVop = pixels ();
	PixelF* ppxlRet = (PixelF*) pfiRet -> pixels ();
	UInt area = where ().area ();
	for (UInt ip = 0; ip < area; ip++) {
		*ppxlRet = (PixelF) ppxlVop -> pxlU.color [pxlCom];
		ppxlRet++;
		ppxlVop++;
	}
	return pfiRet;
}

Void CVideoObjectPlane::lightChange (Int rShift, Int gShift, Int bShift)
{
	CPixel* ppix = (CPixel*) pixels ();
	UInt area = where ().area ();
	for (UInt i = 0; i < area; i++) {
		ppix -> pxlU.rgb.r = (U8) checkrange (rShift + ppix -> pxlU.rgb.r, 0, 255);
		ppix -> pxlU.rgb.g = (U8) checkrange (gShift + ppix -> pxlU.rgb.g, 0, 255);
		ppix -> pxlU.rgb.b = (U8) checkrange (bShift + ppix -> pxlU.rgb.b, 0, 255);
		ppix++;
	}
}	

Void CVideoObjectPlane::overlay (const CVideoObjectPlane& vop)
{
	if (!valid () || !vop.valid () || vop.where ().empty ()) return;
	CRct r = m_rc; 
	r.include (vop.m_rc); // overlay is defined on union of rects
	where (r); 
	if (!valid ()) return; 
	assert (vop.m_ppxl != NULL); 
	CRct rctVop = vop.m_rc;
	Float inv255 = 1.0f / 255.0f;

	const CPixel* ppxlVop = vop.pixels (); 
	CPixel* ppxlThisY = (CPixel*) pixels (rctVop.left, rctVop.top);
	Int widthCurr = where ().width;
	for (CoordI y = rctVop.top; y < rctVop.bottom; y++) { // loop through VOP CRct
		CPixel* ppxlThisX = ppxlThisY;
		for (CoordI x = rctVop.left; x < rctVop.right; x++) {
			Float beta = inv255 * (Float) ppxlVop -> pxlU.rgb.a;
			Float alpha = inv255 * (Float) ppxlThisX -> pxlU.rgb.a;
			for (UInt ip = 0; ip < 3; ip++) {
				Int value = (Int) (
					beta * (Float) ppxlVop -> pxlU.color [ip] + 
					alpha * (Float) ppxlThisX -> pxlU.color [ip] - 
					alpha * beta * (Float) ppxlThisX -> pxlU.color [ip] + .5f
				);
				ppxlThisX -> pxlU.color [ip] = (U8) checkrange (value, 0, 255);
			}
			Int val = (Int) ((beta + alpha - alpha * beta) * 255. + .5f);
			ppxlThisX -> pxlU.rgb.a = (U8) checkrange (val, 0, 255);
			ppxlThisX++;
			ppxlVop++;
		}
		ppxlThisY += widthCurr;
	}
}

Void CVideoObjectPlane::setPlane (const CFloatImage& fi, RGBA pxlCom)
{
	if (!valid ()) return;
	assert (where () == fi.where ());
	CPixel* ppxlVop = (CPixel*) pixels ();
	const PixelF* ppxlFi = fi.pixels ();
	UInt area = where ().area ();
	for (UInt ip = 0; ip < area; ip++) {
		Int value = checkrange ((Int) (*ppxlFi + .5), 0, 255);
		ppxlVop -> pxlU.color [pxlCom] = (U8) value;
		ppxlVop++;
		ppxlFi++;
	}
}


Void CVideoObjectPlane::setPlane (const CIntImage& ii, RGBA pxlCom)
{
	if (!valid ()) return;
	assert (where () == ii.where ());
	CPixel* ppxlVop = (CPixel*) pixels ();
	const PixelI* ppxli = ii.pixels ();
	UInt area = where ().area ();
	for (UInt ip = 0; ip < area; ip++) {
		Int value = checkrange (*ppxli, 0, 255);
		ppxlVop -> pxlU.color [pxlCom] = (U8) value;
		ppxlVop++;
		ppxli++;
	}
}

Void CVideoObjectPlane::setPlane (const CU8Image& ci, RGBA pxlCom)
{
	if (!valid ()) return;
	assert (where () == ci.where ());
	CPixel* ppxlVop = (CPixel*) pixels ();
	const PixelC* ppxlc = ci.pixels ();
	UInt area = where ().area ();
	for (UInt ip = 0; ip < area; ip++) {
		Int value = checkrange (*ppxlc, 0U, 255U);
		ppxlVop -> pxlU.color [pxlCom] = (U8) value;
		ppxlVop++;
		ppxlc++;
	}
}

Void CVideoObjectPlane::getDownSampledPlane(CFloatImage &fiDst,Int iPlane,Int iSx,Int iSy)
{
	Int iDstWidth = fiDst.where().width;
	Int iDstHeight = fiDst.where().height();
	Int iSrcWidth = where().width;
	Int iSrcHeight = where().height();
	PixelF *ppxlfDst = (PixelF *)fiDst.pixels();
	const CPixel *ppxlSrc = pixels();
	assert(iDstWidth == iSrcWidth/iSx && iDstHeight == iSrcHeight/iSy);
	Int iSrcStep = iSrcWidth*iSy;
	Int iX,iY,iXX;

	for(iY=0;iY<iDstHeight;iY++,ppxlSrc+=iSrcStep)
		for(iX=0,iXX=0;iX<iDstWidth;iX++,iXX+=iSx)
			*ppxlfDst++ = ppxlSrc[iXX].pxlU.color[iPlane];
}

Void CVideoObjectPlane::setUpSampledPlane(const CFloatImage &fiSrc,Int iPlane,Int iSx,Int iSy)
{
	Int iSrcWidth = fiSrc.where().width;
	Int iSrcHeight = fiSrc.where().height();
	Int iDstWidth = where().width;
	Int iDstHeight = where().height();
	const PixelF *ppxlfSrc = fiSrc.pixels();
	CPixel *ppxlDst = (CPixel *)pixels();
	assert(iSrcWidth == iDstWidth/iSx && iSrcHeight == iDstHeight/iSy);
	Int iCx,iCy,iXX,iX,iY;

	for(iCy=0,iY=0;iY<iDstHeight;iY++)
	{
		for(iCx=0,iXX=iX=0;iX<iDstWidth;iX++,ppxlDst++)
		{
			ppxlDst->pxlU.color[iPlane] =
				checkrange ((Int) (ppxlfSrc[iXX] + .5), 0, 255);
			iCx++;
			if(iCx==iSx)
			{
				iXX++;
				iCx=0;
			}
		}
		iCy++;
		if(iCy==iSy)
		{
			ppxlfSrc+=iSrcWidth;
			iCy=0;
		}
	}
}

Void CVideoObjectPlane::getDownSampledPlane(CIntImage &iiDst,Int iPlane,Int iSx,Int iSy)
{
	Int iDstWidth = iiDst.where().width;
	Int iDstHeight = iiDst.where().height();
	Int iSrcWidth = where().width;
	Int iSrcHeight = where().height();
	PixelI *ppxliDst = (PixelI *)iiDst.pixels();
	const CPixel *ppxlSrc = pixels();
	assert(iDstWidth == iSrcWidth/iSx && iDstHeight == iSrcHeight/iSy);
	Int iSrcStep = iSrcWidth*iSy;
	Int iX,iY,iXX;

	for(iY=0;iY<iDstHeight;iY++,ppxlSrc+=iSrcStep)
		for(iX=0,iXX=0;iX<iDstWidth;iX++,iXX+=iSx)
			*ppxliDst++ = ppxlSrc[iXX].pxlU.color[iPlane];
}

Void CVideoObjectPlane::setUpSampledPlane(const CIntImage &iiSrc,Int iPlane,Int iSx,Int iSy)
{
	Int iSrcWidth = iiSrc.where().width;
	Int iSrcHeight = iiSrc.where().height();
	Int iDstWidth = where().width;
	Int iDstHeight = where().height();
	const PixelI *ppxliSrc = iiSrc.pixels();
	CPixel *ppxlDst = (CPixel *)pixels();
	assert(iSrcWidth == iDstWidth/iSx && iSrcHeight == iDstHeight/iSy);
	Int iCx,iCy,iXX,iX,iY,iVal;

	for(iCy=0,iY=0;iY<iDstHeight;iY++)
	{
		for(iCx=0,iXX=iX=0;iX<iDstWidth;iX++,ppxlDst++)
		{
			iVal=ppxliSrc[iXX];
			ppxlDst->pxlU.color[iPlane] = ( (iVal>255) ? 255 : ((iVal<0)?0:iVal) );
			iCx++;
			if(iCx==iSx)
			{
				iXX++;
				iCx=0;
			}
		}
		iCy++;
		if(iCy==iSy)
		{
			ppxliSrc+=iSrcWidth;
			iCy=0;
		}
	}
}

Void CVideoObjectPlane::rgbToYUV () // transform from rgb to yuv
{
	if (!valid ()) return;
	CPixel* ppxl = (CPixel*) pixels ();
	UInt area = where ().area ();
	for (UInt ip = 0; ip < area; ip++) {
		Int y = (Int) ((Double) (0.257 * ppxl -> pxlU.rgb.r + 0.504 * ppxl -> pxlU.rgb.g + 0.098 * ppxl -> pxlU.rgb.b + 16) + .5);
		Int cb = (Int) ((Double) (0.439 * ppxl -> pxlU.rgb.r - 0.368 * ppxl -> pxlU.rgb.g - 0.071 * ppxl -> pxlU.rgb.b + 128) + .5);
		Int cr = (Int) ((Double) (-0.148 * ppxl -> pxlU.rgb.r - 0.291 * ppxl -> pxlU.rgb.g + 0.439 * ppxl -> pxlU.rgb.b + 128) + .5);
		ppxl -> pxlU.yuv.y = (U8) y;
		ppxl -> pxlU.yuv.u = (U8) cr;
		ppxl -> pxlU.yuv.v = (U8) cb;
		ppxl++;
	}
}


Void CVideoObjectPlane::yuvToRGB () // transform from yuv to rgb
{
	if (!valid ()) return;
	CPixel* ppxl = (CPixel*) pixels ();
	UInt area = where ().area ();
	for (UInt ip = 0; ip < area; ip++) {
		Double var = 1.164 * (ppxl -> pxlU.yuv.y - 16);
		Int r = (Int) ((Double) (var + 1.596 * (ppxl -> pxlU.yuv.v - 128)) + .5);
		Int g = (Int) ((Double) (var - 0.813 * (ppxl -> pxlU.yuv.v - 128) - 0.391 * (ppxl -> pxlU.yuv.u - 128)) + .5);
		Int b = (Int) ((Double) (var + 2.018 * (ppxl -> pxlU.yuv.u - 128)) + .5);
		ppxl -> pxlU.rgb.r = (U8) checkrange (r, 0, 255);
		ppxl -> pxlU.rgb.g = (U8) checkrange (g, 0, 255);
		ppxl -> pxlU.rgb.b = (U8) checkrange (b, 0, 255);
		ppxl++;
	}
}

own CVideoObjectPlane* CVideoObjectPlane::operator * (const CTransform& tf) const
{
	CVideoObjectPlane* pvopRet = tf.transform_apply (*this);
	return pvopRet; 
}

own CVideoObjectPlane* CVideoObjectPlane::operator - (const CVideoObjectPlane& vop) const
{
	// make the difference in the intersection area
	if (!valid () || !vop.valid ()) return NULL;
	CRct rctSrc = vop.where ();
	CRct rctDes = where ();
	CRct rctInt = rctSrc;
	rctInt.clip (rctDes);
	CVideoObjectPlane* pvopDiff = new CVideoObjectPlane (rctInt);
	UInt offsetSrc = rctSrc.width - rctInt.width;
	UInt offsetDes = rctDes.width - rctInt.width;
	CPixel* ppxlRet = (CPixel*) pvopDiff -> pixels ();
	CPixel* ppxlDes = (CPixel*) pixels (rctInt.left, rctInt.top);
	CPixel* ppxlSrc = (CPixel*) vop.pixels (rctInt.left, rctInt.top);
	for (CoordI j = rctInt.top; j < rctInt.bottom; j++) { 
		for (CoordI i = rctInt.left; i < rctInt.right; i++)	{
			ppxlRet -> pxlU.rgb.r = (U8) checkrange (ppxlDes -> pxlU.rgb.r - ppxlSrc -> pxlU.rgb.r + 128, 0, 255);
			ppxlRet -> pxlU.rgb.g = (U8) checkrange (ppxlDes -> pxlU.rgb.g - ppxlSrc -> pxlU.rgb.g + 128, 0, 255);
			ppxlRet -> pxlU.rgb.b = (U8) checkrange (ppxlDes -> pxlU.rgb.b - ppxlSrc -> pxlU.rgb.b + 128, 0, 255);
			ppxlRet -> pxlU.rgb.a = (ppxlDes -> pxlU.rgb.a == ppxlSrc -> pxlU.rgb.a) ? opaqueValue : transpValue;
			ppxlRet++;
			ppxlDes++;
			ppxlSrc++;
		}
		ppxlDes += offsetDes;
		ppxlSrc += offsetSrc;
	}
	return pvopDiff;
}

Void CVideoObjectPlane::vdlDump (const Char* fileName, CPixel ppxlFalse) const
{
	if (!valid ()) return;
	FILE* pf = fopen (fileName, "wb");
	// write overhead
	putc ('V', pf);
	putc ('M', pf);
	CoordI cord = (CoordI) where ().left;
	fwrite (&cord, sizeof (CoordI), 1, pf);
	cord = (CoordI) where ().top;
	fwrite (&cord, sizeof (CoordI), 1, pf);
	cord = (CoordI) where ().right;
	fwrite (&cord, sizeof (CoordI), 1, pf);
	cord = (CoordI) where ().bottom;
	fwrite (&cord, sizeof (CoordI), 1, pf);

	// dump the actual data
	UInt area = where ().area ();
	const CPixel* ppxl = pixels ();
	for (UInt ip = 0; ip < area; ip++, ppxl++) {
		CPixel pp = *ppxl;
		if (ppxl -> pxlU.rgb.a == transpValue) {
			pp.pxlU.rgb.r = ppxlFalse.pxlU.rgb.r;
			pp.pxlU.rgb.g = ppxlFalse.pxlU.rgb.g;
			pp.pxlU.rgb.b = ppxlFalse.pxlU.rgb.b;
		}
		fwrite (&pp, sizeof (CPixel), 1, pf);
	}
	fclose (pf);
}

Void CVideoObjectPlane::vdlByteDump (const Char* fileName, CPixel ppxlFalse) const
{
	if (!valid ()) return;
	FILE* pf = fopen (fileName, "wb");
	// write overhead
	putc ('V', pf);
	putc ('B', pf);
	int left, top, right, bottom;
	left = where ().left;
	top = where ().top;
	right = where ().right;
	bottom = where ().bottom;

	U8 byte1, byte2;
	byte1 = (left>0)? 128:0;
	left = abs (left);
	byte1 += left / 256;
	byte2 =  left % 256;
	putc (byte1,pf);
	putc (byte2,pf);

	byte1 = (top>0)? 128:0;
	top = abs (top);
	byte1 += top / 256;
	byte2 = top % 256;
	putc (byte1, pf);
	putc (byte2, pf);

	byte1 = (right>0)? 128:0;
	right = abs (right);
	byte1 += right / 256;
	byte2 =  right % 256;
	putc(byte1,pf);
	putc(byte2,pf);

	byte1 = (bottom>0)? 128:0;
	bottom = abs (bottom);
	byte1 += bottom / 256;
	byte2 =  bottom % 256;
	putc(byte1,pf);
	putc(byte2,pf);

	// dump the actual data
	UInt area = where ().area ();
	const CPixel* ppxl = pixels ();
	for (UInt ip = 0; ip < area; ip++, ppxl++) {
		CPixel pp = *ppxl;
		if (ppxl -> pxlU.rgb.a == transpValue) {
			pp.pxlU.rgb.r = ppxlFalse.pxlU.rgb.r;
			pp.pxlU.rgb.g = ppxlFalse.pxlU.rgb.g;
			pp.pxlU.rgb.b = ppxlFalse.pxlU.rgb.b;
		}
		fwrite (&pp, sizeof (CPixel), 1, pf);
	}
	fclose (pf);
}

Void CVideoObjectPlane::dump (FILE* pfFile, ChromType chrType) const
{
	if (!valid ()) return;
	Int iWidth = where ().width, iHeight = where ().height ();
	Int iUVWidth = iWidth, iUVHeight = iHeight;
	UInt uiXSubsample = 1, uiYSubsample = 1;
	if (chrType == FOUR_TWO_TWO)	{
		uiXSubsample = 2;
		iUVWidth = (iWidth + 1) / uiXSubsample;
	}
	else if (chrType == FOUR_TWO_ZERO)	{
		uiXSubsample = 2;
		uiYSubsample = 2;
		iUVWidth = (iWidth + 1) / uiXSubsample;
		iUVHeight = (iHeight + 1) / uiYSubsample;
	}
	UInt iUVArea = iUVWidth * iUVHeight;

	UInt area = where ().area ();
	// allocate mem for buffers
	U8* pchyBuffer = new U8 [area];
	U8* pchuBuffer = new U8 [iUVArea];
	U8* pchvBuffer = new U8 [iUVArea];
	U8* pchyBuffer0 = pchyBuffer;
	U8* pchuBuffer0 = pchuBuffer;
	U8* pchvBuffer0 = pchvBuffer;

	// assign buffers to a vframe
	const CPixel* p = pixels ();
	for (CoordI y = 0; y < iHeight; y++) {					
		if ((y % uiYSubsample) == 1) { 
			pchuBuffer -= iUVWidth; 
			pchvBuffer -= iUVWidth;	
		}
		for (CoordI x = 0; x < iWidth; x++) {				
			if ((x % uiXSubsample) == 0)	{
				*pchuBuffer = p -> pxlU.yuv.u;
				*pchvBuffer = p -> pxlU.yuv.v;		
				pchuBuffer++;
				pchvBuffer++;
			}
			*pchyBuffer++ = p -> pxlU.yuv.y;
			p++;
		}
	}
	// write data from a file
	Int size = fwrite (pchyBuffer0, sizeof (U8), area, pfFile);
	assert (size != 0);
	size = (Int) fwrite (pchuBuffer0, sizeof (U8), iUVArea, pfFile);
	assert (size != 0);
	size = (Int) fwrite (pchvBuffer0, sizeof (U8), iUVArea, pfFile);
	assert (size != 0);

	delete [] pchyBuffer0;
	delete [] pchuBuffer0;
	delete [] pchvBuffer0;
}


Void CVideoObjectPlane::dumpAlpha (FILE* pfFile) const
{						  
	if (!valid ()) return;
	UInt area = where ().area ();
	const CPixel* ppxl = pixels ();
	for (UInt ip = 0; ip < area; ip++, ppxl++)
		putc (ppxl -> pxlU.rgb.a, pfFile);
}

Void CVideoObjectPlane::dumpAbekas (FILE* pfFile) const
{
	//dump in YCbCr but interlaced as per Abekas machine
	assert (valid ());
	Int iWidth = where ().width, iHeight = where ().height ();
	assert (iWidth == 720);
	assert (iHeight == 486);		//ccir 601

	UInt uiXSubsample = 2;	//always has to be FOUR_TWO_TWO)
	const CPixel* p = pixels ();
	for (CoordI y = 0; y < iHeight; y++) {					
		for (CoordI x = 0; x < iWidth; x++) {
			if (x % uiXSubsample == 0)	{
				putc(p -> pxlU.yuv.u, pfFile);
				putc(p -> pxlU.yuv.y, pfFile);
			}
			else {
				putc(p -> pxlU.yuv.v, pfFile);
				putc(p -> pxlU.yuv.y, pfFile);
			}
			p++;
		}
	}
}


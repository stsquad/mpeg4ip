/*************************************************************************

This software module was originally developed by 

	Ming-Chieh Lee (mingcl@microsoft.com), Microsoft Corporation
	Wei-ge Chen (wchen@microsoft.com), Microsoft Corporation
	Bruce Lin (blin@microsoft.com), Microsoft Corporation
	Chuang Gu (chuanggu@microsoft.com), Microsoft Corporation
	(date: March, 1996)

and edited by
    Fujitsu Laboratories Ltd. (contact: Eishi Morimatsu)

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

	basic.cpp

Abstract:

    Basic types:
		Data types, CSite, CVector2D, CRct, CPixel, CMotionVector, CMatrix3x3D

Revision History:
	Sep.06	1999 : RRV added by Eishi Morimatsu (Fujitsu Laboratories Ltd.) 

*************************************************************************/

#include "header.h"
#include "basic.hpp"
#include "string.h"
#include "stdlib.h"
#include "math.h"

#ifdef __MFC_
#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

#define new DEBUG_NEW				   
#endif // __MFC_


/////////////////////////////////////////////
// 
//  Space
// 
/////////////////////////////////////////////

CSite CSite::operator + (const CSite& st) const
{
	return CSite (x + st.x, y + st.y);
}

CSite CSite::operator - (const CSite& st) const
{
	return CSite (x - st.x, y - st.y);
}

CSite CSite::operator * (const CSite& st) const
{
	return CSite (x * st.x, y * st.y);
}

CSite CSite::operator * (Int scale) const
{
	return CSite (x * scale, y * scale);
}

CSite CSite::operator / (const CSite& st) const
{
	assert (st.x != 0 && st.y != 0);
	return CSite (x / st.x, y / st.y);
}

CSite CSite::operator / (Int scale) const
{	
	assert (scale != 0);
	CoordI xNew = (x > 0) ? (Int) ((Double) (x / scale) + .5)
						  : (Int) ((Double) (x / scale) - .5);
	CoordI yNew = (y > 0) ? (Int) ((Double) (y / scale) + .5)
						  : (Int) ((Double) (y / scale) - .5);
	return CSite (xNew, yNew);
}

CSite CSite::operator % (const CSite& st) const
{
	return CSite (x % st.x, y % st.y);
}

Void CSite::operator = (const CSite& st)
{
	x = st.x;
	y = st.y;
}

CRct::CRct (const CSite& st1, const CSite& st2)
{
	left = min (st1.x, st2.x);
	right = max (st1.x, st2.x);
	top = min (st1.y, st2.y);
	bottom = max (st1.y, st2.y);
	width = right - left;
}


CRct::CRct (const CSite& st, Int radiusX, Int radiusY)
{
	left = st.x - radiusX; 
	top = st.y - radiusY; 
	right= st.x + radiusX + 1; 
	bottom = st.y + radiusY + 1;
	width = 2 * radiusX + 1;
}


CRct::CRct (const CSiteD& stdLeftTop, const CSiteD& stdRightTop, const CSiteD& stdLeftBottom, const CSiteD& stdRightBottom)
{
	left = min ((CoordI) floor (stdLeftTop.x), (CoordI) floor (stdRightTop.x));
	left = min (left, (CoordI) floor (stdLeftBottom.x));
	left = min (left, (CoordI) floor (stdRightBottom.x));
	top = min ((CoordI) floor (stdLeftTop.y), (CoordI) floor (stdRightTop.y));
	top = min (top, (CoordI) floor (stdLeftBottom.y));
	top = min (top, (CoordI) floor (stdRightBottom.y));
	right = max ((CoordI) ceil (stdLeftTop.x), (CoordI) floor (stdRightTop.x));
	right = max (right, (CoordI) ceil (stdLeftBottom.x));
	right = max (right, (CoordI) ceil (stdRightBottom.x));
	bottom = max ((CoordI) ceil (stdLeftTop.y), (CoordI) floor (stdRightTop.y));
	bottom = max (bottom, (CoordI) ceil (stdLeftBottom.y));
	bottom = max (bottom, (CoordI) ceil (stdRightBottom.y));
	width = right - left;
}

Void CRct::transpose ()
{
	Int wid = width;
	Int hei = height ();
	right = left + hei;
	bottom = top + wid;
	width = right - left;
}


Void CRct::rightRotate ()
{
	CSite st = center ();
	Int radiusY = width >> 1;
	Int radiusX = height () >> 1;

	left = st.x - radiusX; 
	top = st.y - radiusY; 
	right= st.x + radiusX + 1; 
	bottom = st.y + radiusY + 1;
	width = right - left;
}


Void CRct::clip (const CRct& rc) // intersection operation
{
	if (empty ()) return; 
	if (rc.empty ()) *this = rc;
	if (left < rc.left) left = rc.left; 
	if (top < rc.top) top = rc.top; 
	if (right > rc.right) right = rc.right; 
	if (bottom > rc.bottom) bottom = rc.bottom; 
	width = right - left;
}

Void CRct::include (const CRct& rc) // union operation
{
	if (empty ()) *this = rc;
	if (rc.empty ()) return; 
	if (left > rc.left) left = rc.left; 
	if (top > rc.top) top = rc.top; 
	if (right < rc.right) right = rc.right; 
	if (bottom < rc.bottom) bottom = rc.bottom; 
	width = right - left;
}

Void CRct::include (const CSite& s)
{
	if (!valid ()) 
		*this = CRct (s); 
	else {
		left = min (s.x, left);
		top  = min (s.y, top);
		right = max (s.x + 1, right);
		bottom = max (s.y + 1, bottom);
	}
	width = right - left;
}

Void CRct::operator = (const CRct& rc)
{
	left = rc.left;
	top = rc.top; 
	right = rc.right;
	bottom = rc.bottom;
	width = rc.width;
}

Bool CRct::operator == (const CRct& rc) const
{
	return 
		left == rc.left && 
		top == rc.top && 
		right == rc.right && 
		bottom == rc.bottom; 
}

Bool CRct::operator <= (const CRct& rc) const
{
	return 
		left >= rc.left && 
		top >= rc.top && 
		right <= rc.right && 
		bottom <= rc.bottom; 
}

Bool CRct::operator >= (const CRct& rc) const
{
	return 
		left <= rc.left && 
		top <= rc.top && 
		right >= rc.right && 
		bottom >= rc.bottom; 
}	

CRct CRct::operator / (Int scale) const
{
	Int roundR = (right >= 0) ? scale - 1 : 1 - scale;
	Int roundB = (bottom >= 0) ? scale - 1 : 1 - scale;
	return CRct (left / scale, top / scale, (right + roundR) / scale, (bottom + roundB) / scale);
}


CRct CRct::downSampleBy2 () const
{
	return CRct (left / 2, top / 2, right / 2, bottom / 2);
}

CRct CRct::upSampleBy2 () const
{
	return CRct (left * 2, top * 2, right * 2, bottom * 2);
}

CRct CRct::operator * (Int scale) const
{
	return CRct (left * scale, top * scale, right * scale, bottom * scale);
}

Void CMotionVector::operator = (const CMotionVector& mv)
{
	memcpy (this, &mv, sizeof (CMotionVector));
}

Void CMotionVector::operator = (const CVector& vctHalfPel)
{
	m_vctTrueHalfPel = vctHalfPel;
	computeMV ();	
}

CMotionVector CMotionVector::operator + (const CMotionVector& mv)	const
{
	CVector vctHalfPel = trueMVHalfPel () + mv.trueMVHalfPel ();
	return CMotionVector (vctHalfPel);
}

CMotionVector CMotionVector::operator - (const CMotionVector& mv)	const
{
	CVector vctHalfPel = trueMVHalfPel () - mv.trueMVHalfPel ();
	return CMotionVector (vctHalfPel);
}

CMotionVector::CMotionVector (const CVector& vctHalfPel)
{
	m_vctTrueHalfPel = vctHalfPel;
	computeMV ();	
}

Bool CMotionVector::isZero () const
{
	if (m_vctTrueHalfPel.x == 0 && m_vctTrueHalfPel.y == 0)
		return TRUE;
	else 
		return FALSE;
}

Void CMotionVector::computeTrueMV ()
{
	m_vctTrueHalfPel.x = iMVX * 2 + iHalfX;
	m_vctTrueHalfPel.y = iMVY * 2 + iHalfY;
}

Void CMotionVector::computeMV ()
{
	iMVX = m_vctTrueHalfPel.x / 2;
	iMVY = m_vctTrueHalfPel.y / 2;
	iHalfX = m_vctTrueHalfPel.x - iMVX * 2;
	iHalfY = m_vctTrueHalfPel.y - iMVY * 2;
}

Void CMotionVector::setToZero (Void)
{
	memset (this, 0, sizeof (*this));
}

// RRV insertion
Void CMotionVector::scaleup ()
{
	if(m_vctTrueHalfPel.x == 0){
		m_vctTrueHalfPel_x2.x = 0;
	}
	else if(m_vctTrueHalfPel.x > 0){
		m_vctTrueHalfPel_x2.x = 2 *m_vctTrueHalfPel.x -1;
	}
	else{
		m_vctTrueHalfPel_x2.x = 2 *m_vctTrueHalfPel.x +1;
	}
	if(m_vctTrueHalfPel.y == 0){
		m_vctTrueHalfPel_x2.y = 0;
	}
	else if(m_vctTrueHalfPel.y > 0){
		m_vctTrueHalfPel_x2.y = 2 *m_vctTrueHalfPel.y -1;
	}
	else{
		m_vctTrueHalfPel_x2.y = 2 *m_vctTrueHalfPel.y +1;
	}
}
// ~RRV

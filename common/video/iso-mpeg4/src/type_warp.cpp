/*************************************************************************

This software module was originally developed by 

	Ming-Chieh Lee (mingcl@microsoft.com), Microsoft Corporation
	Wei-ge Chen (wchen@microsoft.com), Microsoft Corporation
	Bruce Lin (blin@microsoft.com), Microsoft Corporation
	Chuang Gu (chuanggu@microsoft.com), Microsoft Corporation
	(date: March, 1996)

and also edited by

    Yuichiro Nakaya (Hitachi, Ltd.)
    Yoshinori Suzuki (Hitachi, Ltd.)

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

	warp.cpp

Abstract:

	Geometry transformation:
		2D Affine
		2D Perspective

Revision History:
	Jan. 13, 1999: Code for disallowing zero demoninators in perspective
                       warping added by Hitachi, Ltd.

*************************************************************************/

#include <math.h>
#include "basic.hpp"
#include "svd.h"
#include "warp.hpp"

#ifdef __MFC_
#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

#define new DEBUG_NEW				   
#endif // __MFC_


CVector2D CMatrix2x2D::apply (const CVector2D& v) const
{
 	return CVector2D (
		m_value [0] [0] * v.x + m_value [0] [1] * v.y, 
		m_value [1] [0] * v.x + m_value [1] [1] * v.y 
	); 
}


CMatrix2x2D::CMatrix2x2D (Double d)
{
	m_value [0] [0] = d; 
	m_value [1] [1] = d; 
	m_value [0] [1] = 0; 
	m_value [1] [0] = 0; 
}

CMatrix2x2D::CMatrix2x2D (Double d00, Double d01, Double d10, Double d11)
{
	m_value [0] [0] = d00; 
	m_value [1] [1] = d11; 
	m_value [0] [1] = d01; 
	m_value [1] [0] = d10; 
}

CMatrix2x2D::CMatrix2x2D (const CVector2D& v0, const CVector2D& v1, Bool fAsColumns)
{
	m_value [0] [0] = v0.x; 
	m_value [1] [0] = v0.y; 
	m_value [0] [1] = v1.x; 
	m_value [1] [1] = v1.y; 

	if (!fAsColumns) transpose (); 
}

CMatrix2x2D::CMatrix2x2D (const CVector2D v [2])
{
	*this = CMatrix2x2D (v [0], v [1]); 
}


CMatrix2x2D CMatrix2x2D::operator * (const CMatrix2x2D& x) const 
{
	CMatrix2x2D r; 
	r.m_value [0] [0] = m_value [0] [0] * x.m_value [0] [0] + m_value [0] [1] * x.m_value [1] [0]; 
	r.m_value [1] [0] = m_value [1] [0] * x.m_value [0] [0] + m_value [1] [1] * x.m_value [1] [0]; 
	r.m_value [0] [1] = m_value [0] [0] * x.m_value [0] [1] + m_value [0] [1] * x.m_value [1] [1]; 
	r.m_value [1] [1] = m_value [1] [0] * x.m_value [0] [1] + m_value [1] [1] * x.m_value [1] [1]; 
	return r; 
}

CMatrix2x2D::CMatrix2x2D (const CVector2D& s0, const CVector2D& s1, const CVector2D& d0, const CVector2D& d1) 
{
	*this = CMatrix2x2D (d0, d1) * CMatrix2x2D (s0, s1).inverse (); 
}

CMatrix2x2D::CMatrix2x2D (const CVector2D source [2], const CVector2D dest [2])
{
	*this = CMatrix2x2D (source [0], source [1], dest [0], dest [1]); 
}

CMatrix2x2D CMatrix2x2D::inverse () const
{
	Double det = determinant (); 
	if (det == 0) // ill conditioned -- make this a better test
	{
		return CMatrix2x2D ((Double) 0); 
	}

	CMatrix2x2D r; 
	Double detInv = (Double) 1.0 / det; 
	r.m_value [0] [0] = m_value [1] [1] * detInv; 
	r.m_value [1] [1] = m_value [0] [0] * detInv; 
	r.m_value [0] [1] = - m_value [0] [1] * detInv; 
	r.m_value [1] [0] = - m_value [1] [0] * detInv; 
	return r; 
}

Void CMatrix2x2D::transpose ()
{
	Double tmp = m_value [0] [1]; 
	m_value [0] [1] = m_value [1] [0]; 
	m_value [1] [0] = tmp; 
}


CAffine2D::CAffine2D (const CSiteD& source, const CSiteD& dest) : 
	m_mtx(1.), m_stdSrc (source), m_stdDst (dest)
{
	// thats all folks
}


CAffine2D::CAffine2D (const CSiteD& s0, const CSiteD& s1, const CSiteD& d0, const CSiteD& d1) : 
	m_mtx(s1 - s0, (s1 - s0).rot90 (), d1 - d0, (d1 - d0).rot90 ()),
	m_stdSrc (s0), 
	m_stdDst (d0)
{
}


CAffine2D::CAffine2D (
	const CSiteD& s, const CVector2D& sv0, const CVector2D& sv1, 
	const CSiteD& d, const CVector2D& dv0, const CVector2D& dv1) : 
	m_mtx(sv0, sv1, dv0, dv1), m_stdSrc (s), m_stdDst (d)
{
}


CAffine2D::CAffine2D (const CSiteD source [3], const CSiteD dest [3]) : 
	m_mtx (
		source [1] - source [0], 
		source [2] - source [0], 
		dest [1] - dest [0], 
		dest [2] - dest [0]
	),
	m_stdSrc (source [0]), 
	m_stdDst (dest [0])

{
}

CAffine2D::CAffine2D (const CoordD params [6]) :
	m_mtx (CVector2D (params [0], params [3]), CVector2D (params [1], params [4])),
	m_stdSrc (0, 0),
	m_stdDst (params [2], params [5])
{
}



CAffine2D CAffine2D::inverse () const
{
	CAffine2D affInv; 
	affInv.m_mtx= m_mtx.inverse (); 
	affInv.m_stdSrc = m_stdDst; 
	affInv.m_stdDst = m_stdSrc; 
	return affInv; 
}

CAffine2D CAffine2D::setOrigin (const CSiteD& std) const
{
 	CAffine2D r;
	CVector2D vec;
	r.m_mtx = m_mtx;
	vec = m_mtx.apply (std - m_stdDst);
	r.m_stdDst.x = std.x - vec.x;
	r.m_stdDst.y = std.y - vec.y;
	r.m_stdSrc = m_stdSrc;
	return r;
}


Void CAffine2D::getParams (CoordD params [6]) const
{
	params [0] = m_mtx.element (0, 0);
	params [1] = m_mtx.element (0, 1);
	params [3] = m_mtx.element (1, 0);
	params [4] = m_mtx.element (1, 1);
	params [2] = m_stdDst.x - params [0] * m_stdSrc.x - params [1] * m_stdSrc.y;
	params [5] = m_stdDst.y - params [3] * m_stdSrc.x - params [4] * m_stdSrc.y;

}


CPerspective2D::~CPerspective2D ()
{
	delete [] m_rgCoeff;
}

CPerspective2D::CPerspective2D ()
{
	m_rgCoeff = NULL;
}

CPerspective2D::CPerspective2D (const UInt pntNum, const CSiteD source [4], const CSiteD dest [4], const UInt accuracy) : m_rgCoeff (NULL)
{
	UInt uiScale = 1 << (accuracy + 1);
	m_x0 = source [0].x;
	m_y0 = source [0].y;
	m_rgCoeff = new Double [9];
	Double x [4], dx [4], y [4], dy [4], a[4][4];
	memset(a, 0, sizeof(a));
	for (UInt k = 0; k < pntNum; k++) {
		m_rgstdSrc [k] = source [k];
		m_rgstdDst [k] = dest [k] * uiScale;
		x [k] = m_rgstdDst [k].x; 
		y [k] = m_rgstdDst [k].y;		
	}
	
	Int width = (Int) (m_rgstdSrc [1].x - m_rgstdSrc [0].x);
	Int height = (Int) (m_rgstdSrc [2].y - m_rgstdSrc [0].y);
	if (pntNum == 1) {
		a[1][1] = (Double) uiScale;
		a[2][1] = 0;
		a[3][1] = x [0];
		a[1][2] = 0;
		a[2][2] = (Double) uiScale;
		a[3][2] = y [0];
		a[1][3] = 0;
		a[2][3] = 0;
		a[3][3] = 1.0;
	} else if (pntNum == 2) {
		a[1][1] = (x [1] - x [0]) ;
		a[2][1] = (y [0] - y [1]) ;
		a[3][1] = x [0] * width;
		a[1][2] = (y [1] - y [0]) ;
		a[2][2] = (x [1] - x [0]) ;
		a[3][2] = y [0] * width;
		a[1][3] = 0;
		a[2][3] = 0;
		a[3][3] = width;
	} else if (pntNum == 3) {
		a[1][1] = (x [1] - x [0]) * height;
		a[2][1] = (x [2] - x [0]) * width;
		a[3][1] = x [0] * height * width;
		a[1][2] = (y [1] - y [0]) * height;
		a[2][2] = (y [2] - y [0]) * width;
		a[3][2] = y [0] * height * width;
		a[1][3] = 0;
		a[2][3] = 0;
		a[3][3] = height * width;	
	} else if (pntNum == 4) {
		dx [1] = x [1] - x [3]; dx [2] = x [2] - x [3]; dx [3] = x [0] - x [1] + x [3] - x [2];
		dy [1] = y [1] - y [3]; dy [2] = y [2] - y [3]; dy [3] = y [0] - y [1] + y [3] - y [2];
		if ((dx [3] == 0 && dy [3] == 0) || pntNum == 3) {
			a[1][1] = (x [1] - x [0]) * height;
			a[2][1] = (x [2] - x [0]) * width;
			a[3][1] = x [0] * height * width;
			a[1][2] = (y [1] - y [0]) * height;
			a[2][2] = (y [2] - y [0]) * width;
			a[3][2] = y [0] * height * width;
			a[1][3] = 0;
			a[2][3] = 0;
			a[3][3] = height * width;
		} else {
			Double deltaD = dx [1] * dy [2] - dx [2] * dy [1];
			a[1][3] = (dx [3] * dy [2] - dx [2] * dy [3]) * height;
			a[2][3] = (dx [1] * dy [3] - dx [3] * dy [1]) * width;
			a[1][1] = (x [1] - x [0]) * height * deltaD + a[1][3] * x [1];
			a[2][1] = (x [2] - x [0]) * width * deltaD + a[2][3] * x [2];
			a[3][1] = x [0] * height * width * deltaD;
			a[1][2] = (y [1] - y [0]) * deltaD * height + a[1][3] * y [1];
			a[2][2] = (y [2] - y [0]) * deltaD * width + a[2][3] * y [2];
			a[3][2] = y [0] * height * width * deltaD;
			a[3][3] = height * width * deltaD;
		}
	}
	m_rgCoeff [0] = a[1][1];
	m_rgCoeff [1] =	a[2][1];
	m_rgCoeff [2] =	a[3][1];
	m_rgCoeff [3] =	a[1][2];
	m_rgCoeff [4] =	a[2][2];
	m_rgCoeff [5] =	a[3][2];
	m_rgCoeff [6] =	a[1][3];
	m_rgCoeff [7] =	a[2][3];
	m_rgCoeff [8] =	a[3][3];
}

CPerspective2D::CPerspective2D (const CSiteD source [4], const CSiteD dest [4]) : m_rgCoeff (NULL)
{
	m_x0 = m_y0 = 0.0;
	UInt pts = 4;
	UInt rows = 2 * pts, cols = 8, i, j;
	for (i = 0; i < 4; i++) {
		m_rgstdSrc [i] = source [i];
		m_rgstdDst [i] = dest [i];
	}

	Double* B = new Double [rows];
	Double** A = (Double**) new Double* [rows]; 
	for (i = 0; i < rows; i++)
		A [i] =  new Double [cols] ;

	for (i = 0; i < pts; i++) {
	 	A [i] [0] = m_rgstdSrc [i].x ;
		A [i] [1] = m_rgstdSrc [i].y ;
		A [i] [2] = 1; 
		A [i] [3] = A [i] [4] = A[i] [5] =0;
	 	A [i] [6] = (-1) * (m_rgstdSrc [i].x * m_rgstdDst [i].x );
		A [i] [7] = (-1) * (m_rgstdSrc [i].y * m_rgstdDst [i].x );

		B [i] = m_rgstdDst [i].x;
	}

	for (i = pts, j = 0; i < rows; i++, j++) {
		A [i] [0] = A [i] [1] = A [i] [2] = 0;
	 	A [i] [3] = m_rgstdSrc [j].x ;
		A [i] [4] = m_rgstdSrc [j].y ;
		A [i] [5] = 1; 
	 	A [i] [6] = (-1) * (m_rgstdSrc [j].x * m_rgstdDst [j].y);
		A [i] [7] = (-1) * (m_rgstdSrc [j].y * m_rgstdDst [j].y);

		B [i] = m_rgstdDst [j].y;
	}
	m_rgCoeff = linearLS (A, B, rows, cols);

	delete [] B;
	for (i = 0; i < rows; i++)
		delete [] A [i];

	delete [] A;
}

CPerspective2D::CPerspective2D (Double* rgCoeff) : m_rgCoeff (NULL)
{
	m_rgCoeff = new Double [8];
	UInt i;
	for (i = 0; i < 8; i++) 
		m_rgCoeff [i] = rgCoeff [i];
	m_rgstdSrc [0] = CSiteD (0, 0);
	m_rgstdSrc [1] = CSiteD (176, 0);
	m_rgstdSrc [2] = CSiteD (0, 144);
	m_rgstdSrc [3] = CSiteD (176, 144);
	for (i = 0; i < 4; i++) {
		m_rgstdDst [i] = (*this * m_rgstdSrc [i]).s;
	}
}

CSiteDWFlag CPerspective2D::apply (const CSiteD& s0) const
{
	Double destX, destY, destW;
	CSiteDWFlag destST; 
	
	destX = (s0.x - m_x0) * m_rgCoeff [0] + (s0.y - m_y0) * m_rgCoeff [1] + m_rgCoeff [2];
	destY = (s0.x - m_x0 ) * m_rgCoeff [3] + (s0.y - m_y0 ) * m_rgCoeff [4] + m_rgCoeff [5]; 
	destW = (s0.x - m_x0) * m_rgCoeff [6] + (s0.y - m_y0) * m_rgCoeff [7] + m_rgCoeff [8]; 
	if (destW != 0.0) {
		destST.s.x = destX / destW; 
		destST.s.y = destY / destW; 
		destST.f = FALSE;
	} else    // destW == 0.0
		destST.f = TRUE;

	return destST;
}

CSiteWFlag CPerspective2D::apply (const CSite& s0) const
{
	Double destX, destY, destW;
	CSiteWFlag destST; 
	
	destX = (s0.x - m_x0) * m_rgCoeff [0] + (s0.y - m_y0) * m_rgCoeff [1] + m_rgCoeff [2];
	destY = (s0.x - m_x0 ) * m_rgCoeff [3] + (s0.y - m_y0 ) * m_rgCoeff [4] + m_rgCoeff [5]; 
	destW = (s0.x - m_x0) * m_rgCoeff [6] + (s0.y - m_y0) * m_rgCoeff [7] + m_rgCoeff [8];

	if (destW != 0.0) {
		if ((destX >= 0 && destW > 0) || (destX <= 0 && destW < 0)) {
			destST.s.x = (CoordI) ((destX + destW / 2) / destW);
		} else {
			if (destX > 0 && destW < 0) {
				destST.s.x = (CoordI) ((destX - (destW + 1) / 2) / destW);
			} else {
				destST.s.x = (CoordI) ((destX - (destW - 1) / 2) / destW);
			}
		}
		if ((destY >= 0 && destW > 0) || (destY <= 0 && destW < 0)) {
			destST.s.y = (CoordI) ((destY + destW / 2) / destW);
		} else {
			if (destY > 0 && destW < 0) {
				destST.s.y = (CoordI) ((destY - (destW + 1) / 2) / destW);
			} else {
				destST.s.y = (CoordI) ((destY - (destW - 1) / 2) / destW);
			}
		}
		destST.f = FALSE;
	} else    // destW == 0.0
		destST.f = TRUE;

	return destST; 
}

CPerspective2D CPerspective2D::inverse () const
{
	return CPerspective2D (m_rgstdDst, m_rgstdSrc);
}

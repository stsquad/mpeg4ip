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

    geom.cpp

Abstract:

    2D geometry

Revision History:

*************************************************************************/
#include <stdlib.h>
#include <stdio.h>
#include "basic.hpp"
#include <float.h>
#include "geom.hpp"
#include <string.h>
#include <math.h>

#ifdef __MFC_
#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

#define new DEBUG_NEW				   
#endif // __MFC_


// CPolygonI

CPolygonI::CPolygonI(const CPolygonI& poly) : m_csit(0), m_rgsit(NULL) {
	allocate(poly.m_csit);
	memcpy (m_rgsit,poly.m_rgsit,sizeof(CSite)*m_csit);
}

CPolygonI::CPolygonI (UInt csit, const CSite* rgsit, Bool bCheckCorner, const CRct& rc) : m_csit(0), m_rgsit(NULL) {
	allocate(csit);
	memcpy(m_rgsit,rgsit,sizeof(CSite)*m_csit);
	if (bCheckCorner) {
		assert (rc.valid ());
		checkCorner (rc);
	}
	close ();
}

CPolygonI::CPolygonI(const CRct& r) : m_csit(0), m_rgsit(NULL) {
	allocate(4);

	m_rgsit[0].set(r.left,r.top); 
	m_rgsit[1].set(r.right,r.top); 
	m_rgsit[2].set(r.right,r.bottom); 
	m_rgsit[3].set(r.left,r.bottom); 
}

inline Int isign (Int i) {return i >= 0 ? 1 : -1;};	// returns 1 if i>=0, -1 if i<0

Void CPolygonI::crop (const CRct& rct)	
{
	for (UInt i = 0; i < m_csit; i++) {
		if (m_rgsit[i].x < rct.left)	{
			m_rgsit[i].x = rct.left;
		}
		else if (m_rgsit[i].x >= rct.right)	{
			m_rgsit[i].x = rct.right - 1;
		}

		if (m_rgsit[i].y < rct.top)	{
			m_rgsit[i].y = rct.top;
		}
		else if (m_rgsit[i].y >= rct.bottom)	{
			m_rgsit[i].y = rct.bottom - 1;
		}
	}
}


Void CPolygonI::close ()
{
	UInt nStsMax = 4 * m_csit + 1000;
	CSite* rgSts = new CSite [nStsMax];	// I hope this is big enough!
	UInt nStNoGap = 0;	// in general, will have to add some sites

	for (UInt i = 0; i < m_csit; i++) {
		CSite s0, s1;
		if (i != 0)	{
			s0 = m_rgsit[i-1];
		}
		else	{
			s0 = m_rgsit[m_csit - 1];
		}
		s1 = m_rgsit[i];
		CoordI dx = s1.x - s0.x;
		CoordI dy = s1.y - s0.y;

		if (abs(dx) <=1 && abs(dy) <= 1) {	// no gap
			if (i > 0)	{				//added by wchen
				rgSts[nStNoGap] = s0;
				nStNoGap++; 
				assert (nStNoGap < nStsMax);
			}
		}
		else {	// there is a gap
			CoordI j, xnew, ynew;
			CoordI x = s0.x;
			CoordI y = s0.y;
			float slope;
			if (i > 0)	{// added by wchen
				rgSts[nStNoGap] = s0;
				nStNoGap++; 
			}
			assert (nStNoGap < nStsMax);
			// three cases |dx|=|dy|, |dx|>|dy|, |dx|<|dy|
			if (abs(dx) == abs(dy)){ // |dx|=|dy|
				int islope = dx / dy;
				for (j = isign(dx); j != dx; j+=isign(dx)) {
					xnew = x + j;
					ynew = y + islope*j;
					rgSts[nStNoGap] = CSite (xnew, ynew);
					nStNoGap++;
					assert (nStNoGap < nStsMax);
				}
			}
			else if (abs(dx) > abs(dy)) { // |dx|>|dy|
				slope = (float) dy / (float) dx;
				for (j = isign(dx); j != dx; j+=isign(dx)) {
					xnew = x + j;
					ynew = y + (int) (slope * j);
					rgSts[nStNoGap] = CSite (xnew, ynew);
					nStNoGap++;
					assert (nStNoGap < nStsMax);
				}
			}
			else { // |dx|<|dy|
				slope = (float) dx / (float) dy;
				for (j = isign(dy); j != dy; j+=isign(dy)) {
					ynew = y + j;
					xnew = x + (int) (slope * j);
					rgSts[nStNoGap] = CSite (xnew, ynew);
					nStNoGap++;
					assert (nStNoGap < nStsMax);
				}
			}
		}
	}
	rgSts[nStNoGap] = m_rgsit[m_csit-1];
	nStNoGap++; 
	assert (nStNoGap <= nStsMax);
	// Now have a new set of sites with no gaps.  Convert to codewords.

	m_csit = nStNoGap;		//added by wchen
	delete [] m_rgsit;
	m_rgsit = new CSite [m_csit];
	for (UInt iSts = 0; iSts < m_csit; iSts++)
		m_rgsit [iSts] = rgSts [iSts];
	delete [] rgSts;

}

void CPolygonI::unpack(UInt& csit, CSite*& rgsit) const {
	csit = m_csit;
	rgsit = new CSite[csit];
	memcpy(rgsit,m_rgsit,sizeof(CSite)*csit);
}


Void CPolygonI::dump (const Char* pchFileName) const
{
	FILE* pf = fopen (pchFileName, "w");
	fprintf (pf, "%d\n", m_csit);
	for (UInt ist = 0; ist < m_csit; ist++)
		fprintf (pf, "%ld %ld\n", m_rgsit [ist].x, m_rgsit [ist].y);
	fclose (pf);
}


CPolygonI* CPolygonI::sample(int rate, const CRct& rc) {
	if (m_csit == 0) return new CPolygonI();

	/* Bool fEdges = */ rc.valid();

	CSite* rgsit = new CSite[m_csit+5];

	CoordI x0 = rc.left;
	CoordI x1 = rc.right-1;
	CoordI y0 = rc.top;
	CoordI y1 = rc.bottom-1;
	
#define EDGE(sit) (sit.x == x0 || sit.x == x1 || sit.y == y0 || sit.y == y1)
#define CORNER(sit) ((sit.x == x0 && (sit.y == y0 || sit.y == y1)) ||(sit.x == x1 && (sit.y == y0 || sit.y == y1)))

	rgsit[0] = m_rgsit[0];
	UInt isitS = 1;
	for (UInt isit = 1; isit < m_csit; isit++) {
		if (CORNER(m_rgsit[isit]) || 
			(isit % rate == 0 &&
				!(EDGE(m_rgsit[isit]) || EDGE(m_rgsit[isit-1]) || EDGE(m_rgsit[isit+1]))
			)
		) {
			rgsit[isitS++] = m_rgsit[isit];
		}
	}

	CPolygonI* ppoly = new CPolygonI(isitS,rgsit);
	delete [] rgsit;
	return ppoly;
}

void CPolygonI::allocate(UInt csit) {
	assert (csit == (UInt) (int) csit);
	m_csit = csit;
	delete [] m_rgsit;
	m_rgsit = new CSite[csit];
}


Void CPolygonI::checkCorner (const CRct& rc)
{
	if (m_csit == 0) return;

	Int range = 2;

	CoordI x0 = rc.left + range;
	CoordI x1 = rc.right - 1 - range;
	CoordI y0 = rc.top + range;
	CoordI y1 = rc.bottom - 1 - range;

	CSite* rgsit = new CSite [m_csit + 1];
	UInt isitS = 0;
	CSite* psit = m_rgsit;
	for (UInt isit = 1; isit < m_csit; isit++) {
		CoordI xCurr = psit -> x, yNext = (psit + 1) -> y;
		CoordI xNext = (psit + 1) -> x, yCurr = psit -> y;
		if (
			(xCurr <= x0 && yNext <= y0) ||
			(xCurr <= x0 && yNext >= y1) ||
			(xCurr >= x1 && yNext <= y0) ||
			(xCurr >= x1 && yNext >= y1)
		)
			rgsit[isitS++] = CSite (xCurr, yNext);
		else if (
			(xNext <= x0 && yCurr <= y0) ||
			(xNext <= x0 && yCurr >= y1) ||
			(xNext >= x1 && yCurr <= y0) ||
			(xNext >= x1 && yCurr >= y1)
		)
			rgsit[isitS++] = CSite (xNext, yCurr);
		else
			rgsit[isitS++] = m_rgsit[isit];
		psit++;
	}
	m_csit = isitS;
	delete [] m_rgsit;
	m_rgsit = rgsit;
}

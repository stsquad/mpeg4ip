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

	yuva.hpp

Abstract:

	YUVA (4:2:0) VOP 

Revision History:

*************************************************************************/

#include "typeapi.h"
#include <iostream>

#ifdef __MFC_
#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

#define new DEBUG_NEW				   
#endif // __MFC_


CVOPIntYUVBA::~CVOPIntYUVBA ()
{
	delete m_piiY;
	delete m_piiU;
	delete m_piiV;
	delete m_piiBY;
	delete m_piiBUV;
	delete [] m_ppiiA;
}

Void CVOPIntYUVBA::constructFromVOPF (const CVOPIntYUVBA& vopi, const CRct& rc)
{
  m_iAuxCompCount = vopi.m_iAuxCompCount;
	if (rc.valid ()) {
		CRct rY = (rc.valid ()) ? rc : vopi.whereY ();
		CRct rUV = rY / 2;
		m_piiY = new CIntImage (*vopi.getPlane (Y_PLANE), rY);
		m_piiU = new CIntImage (*vopi.getPlane (U_PLANE), rUV);
		m_piiV = new CIntImage (*vopi.getPlane (V_PLANE), rUV);
		m_piiBY = new CIntImage (*vopi.getPlane (BY_PLANE), rY);
		m_piiBUV = new CIntImage (*vopi.getPlane (BUV_PLANE), rUV);
		if (m_fAUsage == EIGHT_BIT) {
      m_ppiiA = new CIntImage* [m_iAuxCompCount];
      for(Int iAuxComp=0;iAuxComp<m_iAuxCompCount;iAuxComp++) { // MAC (SB) 26-Nov-99
  			m_ppiiA[iAuxComp] = new CIntImage (*vopi.getPlaneA(iAuxComp), rY);
	  		assert (m_ppiiA[iAuxComp] != NULL);
      }
		}
	}
	else {
		m_piiY = new CIntImage (*vopi.getPlane (Y_PLANE));
		m_piiU = new CIntImage (*vopi.getPlane (U_PLANE));
		m_piiV = new CIntImage (*vopi.getPlane (V_PLANE));
		m_piiBY = new CIntImage (*vopi.getPlane (BY_PLANE));
		m_piiBUV = new CIntImage (*vopi.getPlane (BUV_PLANE));
		if (m_fAUsage == EIGHT_BIT) {
      m_ppiiA = new CIntImage* [m_iAuxCompCount];
      for(Int iAuxComp=0;iAuxComp<m_iAuxCompCount;iAuxComp++) { // MAC (SB) 26-Nov-99
			  m_ppiiA[iAuxComp] = new CIntImage (*vopi.getPlaneA (iAuxComp));
			  assert (m_ppiiA[iAuxComp] != NULL);
      }
		}
	}
	assert (m_piiY != NULL);
	assert (m_piiU != NULL);
	assert (m_piiV != NULL);
	assert (m_piiBY != NULL);
	assert (m_piiBUV != NULL);
}

CVOPIntYUVBA::CVOPIntYUVBA (const CVOPIntYUVBA& vopi, AlphaUsage fAUsage, const CRct& rc) : 
	m_fAUsage (fAUsage), m_piiY (NULL), m_piiU (NULL), m_piiV (NULL), m_piiBY (NULL), m_piiBUV (NULL), m_ppiiA (NULL)
	
{
	constructFromVOPF (vopi, rc);
}

		
CVOPIntYUVBA::CVOPIntYUVBA (const CVOPIntYUVBA& vopi, const CRct& rc) : 
	m_piiY (NULL), m_piiU (NULL), m_piiV (NULL), m_piiBY (NULL), m_piiBUV (NULL), m_ppiiA (NULL)
{
	m_fAUsage = vopi.fAUsage ();
	constructFromVOPF (vopi, rc);
}

CVOPIntYUVBA::CVOPIntYUVBA (const CVideoObjectPlane& vop, AlphaUsage fAUsage, Int iAuxCompCount, const CRct& rc) : 
	m_fAUsage (fAUsage), m_iAuxCompCount(iAuxCompCount), m_piiY (NULL), m_piiU (NULL), m_piiV (NULL), m_piiBY (NULL), m_piiBUV (NULL), m_ppiiA (NULL)
	
{
	CRct r = (rc.valid ()) ? rc : vop.where ();
	m_piiY = new CIntImage (r);
	CIntImage* piiU = new CIntImage (r);
	CIntImage* piiV = new CIntImage (r);
	m_piiBY = new CIntImage (r); 
  if (m_fAUsage == EIGHT_BIT) {
    m_ppiiA = new CIntImage* [m_iAuxCompCount];
    for(Int iAuxComp=0;iAuxComp<m_iAuxCompCount;iAuxComp++) { // MAC (SB) 26-Nov-99
		  m_ppiiA[iAuxComp] = new CIntImage (r);
    }
  }
	if (r == vop.where ()) { // faster operation if these two has the same rects
		PixelI* ppxlfY = (PixelI*) m_piiY -> pixels ();
		PixelI* ppxlfU = (PixelI*) piiU -> pixels ();
		PixelI* ppxlfV = (PixelI*) piiV -> pixels ();
		PixelI* ppxlfB = (PixelI*) m_piiBY -> pixels ();
		const CPixel* ppxl = vop.pixels ();
		UInt area = vop.where ().area ();
		UInt ip;
		for (ip = 0; ip < area; ip++, ppxl++, ppxlfY++, ppxlfU++, ppxlfV++, ppxlfB++) {
			*ppxlfY = ppxl -> pxlU.yuv.y;
			*ppxlfU = ppxl -> pxlU.yuv.u;
			*ppxlfV = ppxl -> pxlU.yuv.v;
			*ppxlfB = (ppxl -> pxlU.yuv.a == transpValue) ? 0 : 255;
		}
		if (m_fAUsage == EIGHT_BIT) {
      ppxl = vop.pixels ();
      for(Int iAuxComp=0;iAuxComp<m_iAuxCompCount;iAuxComp++) { // MAC (SB) 26-Nov-99
        PixelI* ppxlfA = (PixelI*) m_ppiiA[iAuxComp] -> pixels ();
        for (ip = 0; ip < area; ip++, ppxl++, ppxlfA++)
          *ppxlfA = ppxl -> pxlU.yuv.a;
      }
		}
	}
	else {
		for (CoordI y = r.top; y < r.bottom; y++) {
			PixelI* ppxlfY = (PixelI*) m_piiY -> pixels (r.left, y);
			PixelI* ppxlfU = (PixelI*) piiU -> pixels (r.left, y);
			PixelI* ppxlfV = (PixelI*) piiV -> pixels (r.left, y);
			PixelI* ppxlfB = (PixelI*) m_piiBY -> pixels (r.left, y);
			const CPixel* ppxl = vop.pixels (r.left, y);
			for (CoordI x = r.left; x < r.right; x++, ppxl++, ppxlfY++, ppxlfU++, ppxlfV++, ppxlfB++) {
				*ppxlfY = ppxl -> pxlU.yuv.y;
				*ppxlfU = ppxl -> pxlU.yuv.u;
				*ppxlfV = ppxl -> pxlU.yuv.v;
				*ppxlfB++ = (ppxl -> pxlU.yuv.a == transpValue) ? 0 : 255;
			}
			if (m_fAUsage == EIGHT_BIT) {
				ppxl = vop.pixels ();
        for(Int iAuxComp=0;iAuxComp<m_iAuxCompCount;iAuxComp++) { // MAC (SB) 26-Nov-99
				  PixelI* ppxlfA = (PixelI*) m_ppiiA[iAuxComp] -> pixels ();
				  for (CoordI x = r.left; x < r.right; x++, ppxl++, ppxlfA++)
					  *ppxlfA = ppxl -> pxlU.yuv.a;
        }
			}
		}
	}
	CRct rctBoxY = m_piiBY -> whereVisible ();
	if (rctBoxY.left % 2 != 0)
		rctBoxY.left--;
	if (rctBoxY.top % 2 != 0)
		rctBoxY.top--;
	m_piiBY -> where (rctBoxY);
	m_piiY -> where (rctBoxY);
  if (m_fAUsage == EIGHT_BIT) {
    for(Int iAuxComp=0;iAuxComp<m_iAuxCompCount;iAuxComp++) // MAC (SB) 26-Nov-99
	  	m_ppiiA[iAuxComp] -> where (rctBoxY);
  }
	piiU -> where (rctBoxY);
	piiV -> where (rctBoxY);
	m_piiU = piiU -> decimate (2, 2);
	delete piiU;
	m_piiV = piiV -> decimate (2, 2);	
	delete piiV;
	m_piiBUV = m_piiBY -> decimateBinaryShape (2, 2);	//conservative decimation
}


CVOPIntYUVBA::CVOPIntYUVBA (AlphaUsage fAUsage, Int iAuxCompCount, const CRct& rc) :
	m_fAUsage (fAUsage), m_iAuxCompCount(iAuxCompCount), m_piiY (NULL), m_piiU (NULL), m_piiV (NULL), m_piiBY (NULL), m_piiBUV (NULL), m_ppiiA (NULL)
{
	CRct rcY = rc;
	CRct rcUV = rc / 2;
	m_piiY = new CIntImage (rcY);
	assert (m_piiY != NULL);
	m_piiU = new CIntImage (rcUV);
	assert (m_piiU != NULL);
	m_piiV = new CIntImage (rcUV);
	assert (m_piiV != NULL);
	m_piiBY = new CIntImage (rcY);
	assert (m_piiBY != NULL);
	m_piiBUV = new CIntImage (rcUV);
	assert (m_piiBUV != NULL);
	if (m_fAUsage == EIGHT_BIT) {
    m_ppiiA = new CIntImage* [m_iAuxCompCount];
    for(Int iAuxComp=0;iAuxComp<m_iAuxCompCount;iAuxComp++) { // MAC (SB) 26-Nov-99
		  m_ppiiA[iAuxComp] = new CIntImage (rcY);
		  assert (m_ppiiA[iAuxComp] != NULL);
    }
	}
}

CVOPIntYUVBA::CVOPIntYUVBA (AlphaUsage fAUsage, Int iAuxCompCount, const CRct& rcY, const CRct& rcUV) :
	m_fAUsage (fAUsage), m_iAuxCompCount(iAuxCompCount), m_piiY (NULL), m_piiU (NULL), m_piiV (NULL), m_piiBY (NULL), m_piiBUV (NULL), m_ppiiA (NULL)
	
{
	m_piiY = new CIntImage (rcY);
	assert (m_piiY != NULL);
	m_piiU = new CIntImage (rcUV);
	assert (m_piiU != NULL);
	m_piiV = new CIntImage (rcUV);
	assert (m_piiV != NULL);
	m_piiBY = new CIntImage (rcY);
	assert (m_piiBY != NULL);
	m_piiBUV = new CIntImage (rcUV);
	assert (m_piiBUV != NULL);
	if (m_fAUsage == EIGHT_BIT) {
    m_ppiiA = new CIntImage* [m_iAuxCompCount];
    for(Int iAuxComp=0;iAuxComp<m_iAuxCompCount;iAuxComp++) { // MAC (SB) 26-Nov-99
		  m_ppiiA[iAuxComp] = new CIntImage (rcY);
		  assert (m_ppiiA[iAuxComp] != NULL);
    }
	}
}

CVOPIntYUVBA::CVOPIntYUVBA (AlphaUsage fAUsage) : 
	m_fAUsage (fAUsage), m_iAuxCompCount(0), m_piiY (NULL), m_piiU (NULL), m_piiV (NULL), m_piiBY (NULL), m_piiBUV (NULL), m_ppiiA (NULL)
	
{
}

const CIntImage* CVOPIntYUVBA::getPlane (PlaneType plnType) const
{
  if (plnType==A_PLANE) {
    printf("For A-Planes please use CVOPIntYUVBA::getPlaneA()!\n");
    assert( plnType!=A_PLANE );
  }

  if (plnType == Y_PLANE) return m_piiY;
	else if (plnType == U_PLANE) return m_piiU;
	else if (plnType == V_PLANE) return m_piiV;
	else if (plnType == BY_PLANE) return m_piiBY;
	else if (plnType == BUV_PLANE) return m_piiBUV;
	else /*if (plnType == A_PLANE) return m_piiA;
	else*/ return NULL;
}

const CIntImage* CVOPIntYUVBA::getPlaneA (Int iAuxComp) const
{
  assert( iAuxComp<m_iAuxCompCount && iAuxComp>=0 );
  return m_ppiiA[iAuxComp];
}

Void CVOPIntYUVBA::where (const CRct& rct)
{
	whereY (rct);
	CRct rctUV = rct / 2;
	whereUV (rctUV);
}

Void CVOPIntYUVBA::whereY (const CRct& rct)
{
	m_piiY -> where (rct);
	m_piiBY -> where (rct);
  if (m_fAUsage == EIGHT_BIT) {
    for(Int iAuxComp=0;iAuxComp<m_iAuxCompCount;iAuxComp++) // MAC (SB) 26-Nov-99
		  m_ppiiA[iAuxComp] -> where (rct);
  }
}

Void CVOPIntYUVBA::whereUV (const CRct& rct)
{
	m_piiU -> where (rct);
	m_piiV -> where (rct);
	m_piiBUV -> where (rct);
}

Void CVOPIntYUVBA::setPlaneA (const CIntImage* pii, Int iAuxComp )
{
  if (pii == NULL)
    return;
  
  assert( iAuxComp<m_iAuxCompCount && iAuxComp>=0 );
  
  delete m_ppiiA[iAuxComp];
  m_ppiiA[iAuxComp] = new CIntImage (*pii); 
}

Void CVOPIntYUVBA::setPlane (const CIntImage* pii, PlaneType plnType, Bool bBUV)
{
  if (plnType==A_PLANE) {
    printf("For A-Planes please use CVOPIntYUVBA::setPlaneA()!\n");
    assert( plnType!=A_PLANE );
  }

  if (pii == NULL)
		return;
	if (plnType == Y_PLANE) {
		delete m_piiY;
		m_piiY = new CIntImage (*pii);
	}
	else if (plnType == U_PLANE) {
		delete m_piiU;
		m_piiU = new CIntImage (*pii);
	}
	else if (plnType == V_PLANE) {
		delete m_piiV;
		m_piiV = new CIntImage (*pii);
	}
	else if (plnType == BY_PLANE) {
		delete m_piiBY;
		m_piiBY = new CIntImage (*pii);
		if (bBUV) {
			delete m_piiBUV;
			m_piiBUV = m_piiBY -> decimate (2, 2);
			m_piiBUV -> setRect (whereUV ());
		}
	}
	else if (plnType == BUV_PLANE) {
		delete m_piiBUV;
		m_piiBUV = new CIntImage (*pii);
	}
	else /*if (plnType == A_PLANE) {
		delete m_piiA;
		m_piiA = new CIntImage (*pii);
	}
	else*/ assert (FALSE);
}

Void CVOPIntYUVBA::cropOnAlpha ()
{
	m_piiBY -> cropOnAlpha ();
	m_piiBUV -> cropOnAlpha ();
	m_piiY -> where (m_piiBY -> where ());
	m_piiU -> where (m_piiBUV -> where ());
	m_piiV -> where (m_piiBUV -> where ());
  if (m_fAUsage == EIGHT_BIT) {
    for(Int iAuxComp=0;iAuxComp<m_iAuxCompCount;iAuxComp++) // MAC (SB) 26-Nov-99
		  m_ppiiA[iAuxComp] -> where (m_piiBY -> where ());
  }
}

Void overlayMB (CIntImage* piiBig, const CIntImage* piiSmall, const CIntImage* piiMskSmall)
{
	if (piiSmall == NULL)
		return;
	PixelI* ppxlfBig = (PixelI*) piiBig -> pixels (piiSmall -> where ().left, piiSmall -> where ().top);
	Int unit = piiSmall -> where ().width;
	Int skipBig = piiBig -> where ().width - unit;
	const PixelI* ppxlfSmall = piiSmall -> pixels ();
	const PixelI* ppxlfMsk = piiMskSmall -> pixels ();
	for (Int i = 0; i < unit; i++) {
		for (Int j = 0; j < unit; j++) {
			*ppxlfBig = *ppxlfSmall;
			ppxlfBig++;
			ppxlfSmall++;
			ppxlfMsk++;
		}
		ppxlfBig += skipBig;
	}
}

Void CVOPIntYUVBA::overlay (const CVOPIntYUVBA& vopi)
{
	if (!vopi.valid ()) return;
	m_piiBY -> overlay (*vopi.getPlane (BY_PLANE));
	m_piiBUV -> overlay (*vopi.getPlane (BUV_PLANE));
	overlayMB (m_piiY, vopi.getPlane (Y_PLANE), vopi.getPlane (BY_PLANE));
	overlayMB (m_piiU, vopi.getPlane (U_PLANE), vopi.getPlane (BUV_PLANE));
	overlayMB (m_piiV, vopi.getPlane (V_PLANE), vopi.getPlane (BUV_PLANE));
  if (m_fAUsage == EIGHT_BIT) {
    for(Int iAuxComp=0;iAuxComp<m_iAuxCompCount;iAuxComp++) // MAC (SB) 26-Nov-99
  		overlayMB (m_ppiiA[iAuxComp], vopi.getPlaneA (iAuxComp), vopi.getPlane (BY_PLANE));
  }
}

Void CVOPIntYUVBA::overlayBY (const CIntImage& iiBY)
{
	if (!iiBY.valid ()) return;
	m_piiBY -> overlay (iiBY);
}

/*
own CVOPIntYUVBA* CVOPIntYUVBA::warp (const CAffine2D& aff) const
{
	CVideoObjectPlane* pvop = pvopYUV ();
	CVideoObjectPlane* pvopWarp = pvop -> warp (aff);
	CVOPIntYUVBA* pvopfRet = new CVOPIntYUVBA (*pvopWarp, m_fAUsage);
	delete pvop;
	delete pvopWarp;
	return pvopfRet;
}


own CVOPIntYUVBA* CVOPIntYUVBA::warp (const CAffine2D& aff, const CRct& rctWarp) const
{
	CVideoObjectPlane* pvop = pvopYUV ();
	CVideoObjectPlane* pvopWarp = pvop -> warp (aff, rctWarp);
	CVOPIntYUVBA* pvopfRet = new CVOPIntYUVBA (*pvopWarp, m_fAUsage);
	delete pvop;
	delete pvopWarp;
	return pvopfRet;
}

own CVOPIntYUVBA* CVOPIntYUVBA::warp (const CPerspective2D& persp) const
{
	CVideoObjectPlane* pvop = pvopYUV ();
	CVideoObjectPlane* pvopWarp = pvop -> warp (persp);
	CVOPIntYUVBA* pvopfRet = new CVOPIntYUVBA (*pvopWarp, m_fAUsage);
	delete pvop;
	delete pvopWarp;
	return pvopfRet;
}


own CVOPIntYUVBA* CVOPIntYUVBA::warp (const CPerspective2D& persp, const CRct& rctWarp) const
{
	CVideoObjectPlane* pvop = pvopYUV ();
	CVideoObjectPlane* pvopWarp = pvop -> warp (persp, rctWarp);
	CVOPIntYUVBA* pvopfRet = new CVOPIntYUVBA (*pvopWarp, m_fAUsage);
	delete pvop;
	delete pvopWarp;
	return pvopfRet;
}

own CVOPIntYUVBA* CVOPIntYUVBA::warp (const CPerspective2D& persp, const CRct& rctWarp, UInt accuracy) const
{
	CVideoObjectPlane* pvop = pvopYUV ();
	CVideoObjectPlane* pvopWarp = pvop -> warp (persp, rctWarp, accuracy);
	CVOPIntYUVBA* pvopfRet = new CVOPIntYUVBA (*pvopWarp, m_fAUsage);
	delete pvop;
	delete pvopWarp;
	return pvopfRet;
}
*/

own CVOPIntYUVBA* CVOPIntYUVBA::warpYUV (const CPerspective2D& persp, const CRct& rctWarp, UInt accuracy) const
{
	CVOPIntYUVBA* pvopfRet = new CVOPIntYUVBA (RECTANGLE, rctWarp);
	CIntImage* piiwarpedY = (CIntImage*) getPlane (Y_PLANE) -> warp (persp, rctWarp, accuracy);
	pvopfRet -> setPlane (piiwarpedY, Y_PLANE, FALSE);
	delete piiwarpedY;

	CIntImage* piiUZoom = (CIntImage*) getPlane (U_PLANE) -> zoomup (2, 2);
	CIntImage* piiUZoomWarp = piiUZoom -> warp (persp, rctWarp, accuracy);
	delete piiUZoom;
	CIntImage* piiUWarped = piiUZoomWarp -> decimate (2, 2);
	delete piiUZoomWarp;
	pvopfRet -> setPlane (piiUWarped, U_PLANE, FALSE);
	delete piiUWarped;

	CIntImage* piiVZoom = (CIntImage*) getPlane (V_PLANE) -> zoomup (2, 2);
	CIntImage* piiVZoomWarp = piiVZoom -> warp (persp, rctWarp, accuracy);
	delete piiVZoom;
	CIntImage* piiVWarp = piiVZoomWarp -> decimate (2, 2);
	delete piiVZoomWarp;
	pvopfRet -> setPlane (piiVWarp, V_PLANE, FALSE);	
	delete piiVWarp;
	return pvopfRet;
}

own CVOPIntYUVBA* CVOPIntYUVBA::warpYUV (const CPerspective2D& persp, const CRct& rctWarp) const
{
	CVOPIntYUVBA* pvopfRet = new CVOPIntYUVBA (RECTANGLE, rctWarp);
	CIntImage* piiwarpedY = getPlane (Y_PLANE) -> warp (persp, rctWarp);
	pvopfRet -> setPlane (piiwarpedY, Y_PLANE, FALSE);
	delete piiwarpedY;

	CIntImage* piiUZoom = getPlane (U_PLANE) -> zoomup (2, 2);
	CIntImage* piiUZoomWarp = piiUZoom -> warp (persp, rctWarp);
	delete piiUZoom;
	CIntImage* piiUWarped = piiUZoomWarp -> decimate (2, 2);
	delete piiUZoomWarp;
	pvopfRet -> setPlane (piiUWarped, U_PLANE, FALSE);
	delete piiUWarped;

	CIntImage* piiVZoom = getPlane (V_PLANE) -> zoomup (2, 2);
	CIntImage* piiVZoomWarp = piiVZoom -> warp (persp, rctWarp);
	delete piiVZoom;
	CIntImage* piiVWarp = piiVZoomWarp -> decimate (2, 2);
	delete piiVZoomWarp;
	pvopfRet -> setPlane (piiVWarp, V_PLANE, FALSE);	
	delete piiVWarp;
	return pvopfRet;
}

own CVOPIntYUVBA* CVOPIntYUVBA::downsampleForSpatialScalability () const
{
	//only handle CIF->QCIF for RECTANGULAR VOP
	assert (m_fAUsage == RECTANGLE);
	/* CRct rctDown = */ whereY ();
	assert (whereY().left == 0 && whereY().top == 0);
	CVOPIntYUVBA* pvopfRet = new CVOPIntYUVBA (m_fAUsage);
	assert (pvopfRet != NULL);
	pvopfRet -> m_piiY = m_piiY->downsampleForSpatialScalability ();
	pvopfRet -> m_piiU = m_piiU->downsampleForSpatialScalability ();
	pvopfRet -> m_piiV = m_piiV->downsampleForSpatialScalability ();
	pvopfRet -> m_piiBY = new CIntImage (pvopfRet->whereY (), opaqueValue);
	pvopfRet -> m_piiBUV = new CIntImage (pvopfRet->whereUV (), opaqueValue);
	return pvopfRet;
}
own CVOPIntYUVBA* CVOPIntYUVBA::upsampleForSpatialScalability () const
{
	//only handle QCIF->CIF for RECTANGULAR VOP
	assert (m_fAUsage == RECTANGLE);
	CVOPIntYUVBA* pvopfRet = new CVOPIntYUVBA (m_fAUsage);
	assert (pvopfRet != NULL);
	pvopfRet -> m_piiY = m_piiY->upsampleForSpatialScalability ();
	pvopfRet -> m_piiU = m_piiU->upsampleForSpatialScalability ();
	pvopfRet -> m_piiV = m_piiV->upsampleForSpatialScalability ();
	pvopfRet -> m_piiBY = new CIntImage (pvopfRet->whereY (), opaqueValue);
	pvopfRet -> m_piiBUV = new CIntImage (pvopfRet->whereUV (), opaqueValue);
	return pvopfRet;
}

/*
Void CVOPIntYUVBA::vdlDump (const Char* fileName, CPixel ppxlFalse) const
{
	CVideoObjectPlane* pvop = pvopYUV ();
	pvop -> vdlDump (fileName, ppxlFalse);
	delete pvop;
}
*/

own Double* CVOPIntYUVBA::mse (const CVOPIntYUVBA& vopi) const
{
	assert (whereY () == vopi.whereY () && whereUV () == vopi.whereUV ());
	Double* rgdblMeanSquareError = new Double [3+m_iAuxCompCount]; // mean square
	CIntImage* piiBOr = new CIntImage (*m_piiBY);
	piiBOr -> orIi (*vopi.getPlane (BY_PLANE));
	CIntImage* piiExp0 = new CIntImage (*m_piiY, piiBOr -> where ());
	CIntImage* piiExp1 = new CIntImage (*vopi.getPlane (Y_PLANE), piiBOr -> where ());
	rgdblMeanSquareError [0] = piiExp1 -> mse (*piiExp0, *piiBOr);
	delete piiExp0;
	delete piiExp1;
	if (fAUsage () == EIGHT_BIT) {
    assert( m_iAuxCompCount==vopi.m_iAuxCompCount );
    for(Int iAuxComp=0;iAuxComp<m_iAuxCompCount;iAuxComp++) { // MAC (SB) 26-Nov-99
  		piiExp0 = new CIntImage (*m_ppiiA[iAuxComp], piiBOr -> where ());
	  	piiExp1 = new CIntImage (*vopi.getPlaneA(iAuxComp), piiBOr -> where ());
		  rgdblMeanSquareError [3+iAuxComp] = piiExp1 -> mse (*piiExp0, *piiBOr);
		  delete piiExp0;
		  delete piiExp1;
    }
	}
	delete piiBOr;
	piiBOr = new CIntImage (*m_piiBUV);
	piiBOr -> orIi (*vopi.getPlane (BUV_PLANE));
	piiExp0 = new CIntImage (*m_piiU, piiBOr -> where ());
	piiExp1 = new CIntImage (*vopi.getPlane (U_PLANE), piiBOr -> where ());
	rgdblMeanSquareError [1] = piiExp1 -> mse (*piiExp0, *piiBOr);
	delete piiExp0;
	delete piiExp1;

	piiExp0 = new CIntImage (*m_piiV, piiBOr -> where ());
	piiExp1 = new CIntImage (*vopi.getPlane (V_PLANE), piiBOr -> where ());
	rgdblMeanSquareError [2] = piiExp1 -> mse (*piiExp0, *piiBOr);
	delete piiExp0;
	delete piiExp1;
	delete piiBOr;
	return rgdblMeanSquareError;
}

own Double* CVOPIntYUVBA::snr (const CVOPIntYUVBA& vopi) const
{
	assert (whereY () == vopi.whereY () && whereUV () == vopi.whereUV ());
	Double* psnr = new Double [3+m_iAuxCompCount]; // mean square
	CIntImage* piiBOr = new CIntImage (*m_piiBY);
	piiBOr -> orIi (*vopi.getPlane (BY_PLANE));
	CIntImage* piiExp0 = new CIntImage (*m_piiY, piiBOr -> where ());
	CIntImage* piiExp1 = new CIntImage (*vopi.getPlane (Y_PLANE), piiBOr -> where ());
	psnr [0] = piiExp1 -> snr (*piiExp0, *piiBOr);
	delete piiExp0;
	delete piiExp1;
	if (fAUsage () == EIGHT_BIT) {
    assert( m_iAuxCompCount==vopi.m_iAuxCompCount );
    for(Int iAuxComp=0;iAuxComp<m_iAuxCompCount;iAuxComp++) { // MAC (SB) 26-Nov-99
		  piiExp0 = new CIntImage (*m_ppiiA[iAuxComp], piiBOr -> where ());
		  piiExp1 = new CIntImage (*vopi.getPlaneA (iAuxComp), piiBOr -> where ());
		  psnr [3+iAuxComp] = piiExp1 -> snr (*piiExp0, *piiBOr);
		  delete piiExp0;
		  delete piiExp1;
    }

	}
	delete piiBOr;
	piiBOr = new CIntImage (*m_piiBUV);
	piiBOr -> orIi (*vopi.getPlane (BUV_PLANE));
	piiExp0 = new CIntImage (*m_piiU, piiBOr -> where ());
	piiExp1 = new CIntImage (*vopi.getPlane (U_PLANE), piiBOr -> where ());
	psnr [1] = piiExp1 -> snr (*piiExp0, *piiBOr);
	delete piiExp0;
	delete piiExp1;

	piiExp0 = new CIntImage (*m_piiV, piiBOr -> where ());
	piiExp1 = new CIntImage (*vopi.getPlane (V_PLANE), piiBOr -> where ());
	psnr [2] = piiExp1 -> snr (*piiExp0, *piiBOr);
	delete piiExp0;
	delete piiExp1;
	delete piiBOr;
	return psnr;
}

own CVOPIntYUVBA* CVOPIntYUVBA::operator + (const CVOPIntYUVBA& vopi) const
{
	assert (whereY () == vopi.whereY () && whereUV () == vopi.whereUV ());
	CVOPIntYUVBA* pvopfRet = new CVOPIntYUVBA (whereY (), m_fAUsage, m_iAuxCompCount);
	assert (pvopfRet != NULL);
	delete pvopfRet -> m_piiY;
	pvopfRet -> m_piiY = *m_piiY + *vopi.getPlane (Y_PLANE);
	delete pvopfRet -> m_piiU;
	pvopfRet -> m_piiU = *m_piiU + *vopi.getPlane (U_PLANE);
	delete pvopfRet -> m_piiV;
	pvopfRet -> m_piiV = *m_piiV + *vopi.getPlane (V_PLANE);
  assert( m_iAuxCompCount==vopi.m_iAuxCompCount );
  for(Int iAuxComp=0;iAuxComp<m_iAuxCompCount;iAuxComp++) { // MAC (SB) 26-Nov-99
    delete pvopfRet -> m_ppiiA[iAuxComp];
	  pvopfRet -> m_ppiiA[iAuxComp] = *m_ppiiA[iAuxComp] + *vopi.getPlaneA(iAuxComp);
  }
  delete pvopfRet -> m_piiBY;
	pvopfRet -> m_piiBY = new CIntImage (*m_piiBY); // alpha is defined as the first one
	delete pvopfRet -> m_piiBUV;
	pvopfRet -> m_piiBUV = new CIntImage (*m_piiBUV); // alpha is defined as the first one
	return pvopfRet;
}

own CVOPIntYUVBA* CVOPIntYUVBA::operator - (const CVOPIntYUVBA& vopi) const
{
	assert (whereY () == vopi.whereY () && whereUV () == vopi.whereUV ());
//	CVOPIntYUVBA* pvopfRet = new CVOPIntYUVBA (whereY (), m_fAUsage);
	CVOPIntYUVBA* pvopfRet = new CVOPIntYUVBA (m_fAUsage, m_iAuxCompCount, whereY ());
	assert (pvopfRet != NULL);
	delete pvopfRet -> m_piiY;
	pvopfRet -> m_piiY = *m_piiY - *vopi.getPlane (Y_PLANE);
	delete pvopfRet -> m_piiU;
	pvopfRet -> m_piiU = *m_piiU - *vopi.getPlane (U_PLANE);
	delete pvopfRet -> m_piiV;
	pvopfRet -> m_piiV = *m_piiV - *vopi.getPlane (V_PLANE);
  assert( m_iAuxCompCount==vopi.m_iAuxCompCount );
  for(Int iAuxComp=0;iAuxComp<m_iAuxCompCount;iAuxComp++) { // MAC (SB) 26-Nov-99
    delete pvopfRet -> m_ppiiA[iAuxComp];
	  pvopfRet -> m_ppiiA[iAuxComp] = *m_ppiiA[iAuxComp] - *vopi.getPlaneA(iAuxComp);
  }
  delete pvopfRet -> m_piiBY;
	pvopfRet -> m_piiBY = new CIntImage (*m_piiBY); // alpha is defined as the first one
	delete pvopfRet -> m_piiBUV;
	pvopfRet -> m_piiBUV = new CIntImage (*m_piiBUV); // alpha is defined as the first one
	return pvopfRet;
}

own CVOPIntYUVBA* CVOPIntYUVBA::operator * (Int vl) const
{
	CVOPIntYUVBA* pvopfRet = new CVOPIntYUVBA (*this);
	assert (pvopfRet != NULL);
	delete pvopfRet -> m_piiY;
	pvopfRet -> m_piiY = *m_piiY * vl;
	delete pvopfRet -> m_piiU;
	pvopfRet -> m_piiU = *m_piiU * vl;
	delete pvopfRet -> m_piiV;
	pvopfRet -> m_piiV = *m_piiV * vl;
  for(Int iAuxComp=0;iAuxComp<m_iAuxCompCount;iAuxComp++) { // MAC (SB) 26-Nov-99
    delete pvopfRet -> m_ppiiA[iAuxComp];
	  pvopfRet -> m_ppiiA[iAuxComp] = *m_ppiiA[iAuxComp] * vl;
  }
  delete pvopfRet -> m_piiBY;
	pvopfRet -> m_piiBY = new CIntImage (*m_piiBY); // alpha is unchanged
	delete pvopfRet -> m_piiBUV;
	pvopfRet -> m_piiBUV = new CIntImage (*m_piiBUV); // alpha is unchanged
	return pvopfRet;
}

own CVOPIntYUVBA* CVOPIntYUVBA::operator / (Int vl) const
{
	assert (vl != .0f);
	CVOPIntYUVBA* pvopfRet = new CVOPIntYUVBA (*this);
	assert (pvopfRet != NULL);
	delete pvopfRet -> m_piiY;
	pvopfRet -> m_piiY = *m_piiY / vl;
	delete pvopfRet -> m_piiU;
	pvopfRet -> m_piiU = *m_piiU / vl;
	delete pvopfRet -> m_piiV;
	pvopfRet -> m_piiV = *m_piiV / vl;
  for(Int iAuxComp=0;iAuxComp<m_iAuxCompCount;iAuxComp++) { // MAC (SB) 26-Nov-99
    delete pvopfRet -> m_ppiiA[iAuxComp];
	  pvopfRet -> m_ppiiA[iAuxComp] = *m_ppiiA[iAuxComp] / vl;
  }
  delete pvopfRet -> m_piiBY;
	pvopfRet -> m_piiBY = new CIntImage (*m_piiBY); // alpha is unchanged
	delete pvopfRet -> m_piiBUV;
	pvopfRet -> m_piiBUV = new CIntImage (*m_piiBUV); // alpha is unchanged
	return pvopfRet;
}

own CVOPIntYUVBA* CVOPIntYUVBA::average (const CVOPIntYUVBA& vopi) const
{
	assert (whereY () == vopi.whereY () && whereUV () == vopi.whereUV ());
//	CVOPIntYUVBA* pvopfRet = new CVOPIntYUVBA (whereY (), m_fAUsage);
	CVOPIntYUVBA* pvopfRet = new CVOPIntYUVBA (m_fAUsage, whereY ());
	assert (pvopfRet != NULL);
	delete pvopfRet -> m_piiY;
	pvopfRet -> m_piiY = m_piiY -> average (*vopi.getPlane (Y_PLANE));
	delete pvopfRet -> m_piiU;
	pvopfRet -> m_piiU = m_piiU -> average (*vopi.getPlane (U_PLANE));
	delete pvopfRet -> m_piiV;
	pvopfRet -> m_piiV = m_piiV -> average (*vopi.getPlane (V_PLANE));
  for(Int iAuxComp=0;iAuxComp<m_iAuxCompCount;iAuxComp++) { // MAC (SB) 26-Nov-99
    delete pvopfRet -> m_ppiiA[iAuxComp];
	  pvopfRet -> m_ppiiA[iAuxComp] = m_ppiiA[iAuxComp] -> average (*vopi.getPlaneA(iAuxComp));
  }
  delete pvopfRet -> m_piiBY;
	pvopfRet -> m_piiBY = new CIntImage (*m_piiBY); // alpha is defined as the first one
	delete pvopfRet -> m_piiBUV;
	pvopfRet -> m_piiBUV = new CIntImage (*m_piiBUV); // alpha is defined as the first one
	return pvopfRet;
}

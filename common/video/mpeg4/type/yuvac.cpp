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

#ifdef __MFC_
#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

#define new DEBUG_NEW				   
#endif // __MFC_

CVOPU8YUVBA::~CVOPU8YUVBA ()
{
	delete m_puciY;
	delete m_puciU;
	delete m_puciV;
	delete m_puciBY;
	delete m_puciBUV;
	delete m_puciA;
}

Void CVOPU8YUVBA::constructFromVOPU8 (const CVOPU8YUVBA& vopf, const CRct& rc)
{
	if (rc.valid ()) {
		m_rctY = rc;
		m_rctUV = m_rctY.downSampleBy2 ();
		m_puciY = new CU8Image (*vopf.getPlane (Y_PLANE), m_rctY);
		m_puciU = new CU8Image (*vopf.getPlane (U_PLANE), m_rctUV);
		m_puciV = new CU8Image (*vopf.getPlane (V_PLANE), m_rctUV);
		m_ppxlcY = (PixelC*) m_puciY->pixels ();
		m_ppxlcU = (PixelC*) m_puciU->pixels ();
		m_ppxlcV = (PixelC*) m_puciV->pixels ();
		if (m_fAUsage != RECTANGLE) {
			m_puciBY = new CU8Image (*vopf.getPlane (BY_PLANE), m_rctY);
			m_ppxlcBY = (PixelC*) m_puciBY->pixels ();
			m_puciBUV = new CU8Image (*vopf.getPlane (BUV_PLANE), m_rctUV);
			m_ppxlcBUV = (PixelC*) m_puciBUV->pixels ();
			assert (m_puciBY != NULL);
			assert (m_puciBUV != NULL);
			if (m_fAUsage == EIGHT_BIT) {
				m_puciA = new CU8Image (*vopf.getPlane (A_PLANE), m_rctY);
				assert (m_puciA != NULL);
				m_ppxlcA = (PixelC*) m_puciA->pixels ();
			}
		}
	}
	else {
		m_rctY = vopf.whereY ();
		m_rctUV = vopf.whereUV ();
		m_puciY = new CU8Image (*vopf.getPlane (Y_PLANE));
		m_puciU = new CU8Image (*vopf.getPlane (U_PLANE));
		m_puciV = new CU8Image (*vopf.getPlane (V_PLANE));
		m_ppxlcY = (PixelC*) m_puciY->pixels ();
		m_ppxlcU = (PixelC*) m_puciU->pixels ();
		m_ppxlcV = (PixelC*) m_puciV->pixels ();
		if (m_fAUsage != RECTANGLE) {
			m_puciBY = new CU8Image (*vopf.getPlane (BY_PLANE));
			m_ppxlcBY = (PixelC*) m_puciBY->pixels ();
			m_puciBUV = new CU8Image (*vopf.getPlane (BUV_PLANE));
			m_ppxlcBUV = (PixelC*) m_puciBUV->pixels ();
			assert (m_puciBY != NULL);
			assert (m_puciBUV != NULL);
			if (m_fAUsage == EIGHT_BIT) {
				m_puciA = new CU8Image (*vopf.getPlane (A_PLANE));
				assert (m_puciA != NULL);
				m_ppxlcA = (PixelC*) m_puciA->pixels ();
			}
		}
	}
	assert (m_puciY != NULL);
	assert (m_puciU != NULL);
	assert (m_puciV != NULL);
}

CVOPU8YUVBA::CVOPU8YUVBA (const CVOPU8YUVBA& vopf, AlphaUsage fAUsage, const CRct& rc)
{
  m_fAUsage = fAUsage;
  m_puciY = NULL;
  m_puciU = NULL; 
  m_puciV  = NULL; 
  m_puciBY  = NULL; 
  m_puciBUV = NULL; 
  m_puciA = NULL;

  constructFromVOPU8 (vopf, rc);
}

		
CVOPU8YUVBA::CVOPU8YUVBA (const CVOPU8YUVBA& vopf, const CRct& rc) : 
	m_puciY (NULL), m_puciU (NULL), m_puciV (NULL), m_puciBY (NULL), m_puciBUV (NULL), m_puciA (NULL)
{
	m_fAUsage = vopf.fAUsage ();
	constructFromVOPU8 (vopf, rc);
}

CVOPU8YUVBA::CVOPU8YUVBA (const Char* sptFilename) :
	m_puciY (NULL), m_puciU (NULL), m_puciV (NULL), m_puciBY (NULL), m_puciBUV (NULL), m_puciA (NULL)
{
	FILE* pf = fopen (sptFilename, "rb");
	// read overhead
	Int c0 = getc (pf);
	Int c1 = getc (pf);
	Int c2 = getc (pf);
	assert (c0 == 'S' && (c1 == 'P' || c2 == 'T') );
	fread (&m_rctY.left, sizeof (CoordI), 1, pf);
	fread (&m_rctY.top, sizeof (CoordI), 1, pf);
	fread (&m_rctY.right, sizeof (CoordI), 1, pf);
	fread (&m_rctY.bottom, sizeof (CoordI), 1, pf);
	fread (&m_fAUsage, sizeof (Int), 1, pf);
	m_rctY.width = m_rctY.right - m_rctY.left;
	m_rctUV = m_rctY.downSampleBy2 ();
	m_puciY = new CU8Image (m_rctY);
	assert (m_puciY != NULL);
	m_puciU = new CU8Image (m_rctUV);
	assert (m_puciU != NULL);
	m_puciV = new CU8Image (m_rctUV);
	assert (m_puciV != NULL);
	m_ppxlcY = (PixelC*) m_puciY->pixels ();
	m_ppxlcU = (PixelC*) m_puciU->pixels ();
	m_ppxlcV = (PixelC*) m_puciV->pixels ();
	if (m_fAUsage != RECTANGLE) {
		m_puciBY = new CU8Image (m_rctY, TRANSPARENT);		//initialize so that outside VOP is transp
		assert (m_puciBY != NULL);
		m_puciBUV = new CU8Image (m_rctUV, TRANSPARENT);	//initialize so that outside VOP is transp
		assert (m_puciBUV != NULL);
		m_ppxlcBY = (PixelC*) m_puciBY->pixels ();
		m_ppxlcBUV = (PixelC*) m_puciBUV->pixels ();
		if (m_fAUsage == EIGHT_BIT) {
			m_puciA = new CU8Image (m_rctY, TRANSPARENT);	//initialize so that outside VOP is transp
			assert (m_puciA != NULL);
			m_ppxlcA = (PixelC*) m_puciA->pixels ();
		}
	}
	// read the actual data
	fread (m_ppxlcY, sizeof (U8), m_rctY.area (), pf);
	fread (m_ppxlcU, sizeof (U8), m_rctUV.area (), pf);
	fread (m_ppxlcV, sizeof (U8), m_rctUV.area (), pf);
	if (m_fAUsage != RECTANGLE) {
		if (m_fAUsage == EIGHT_BIT)
			fread ((PixelC*) m_ppxlcA, sizeof (U8), m_rctY.area (), pf);
		else
			fread ((PixelC*) m_ppxlcBY, sizeof (U8), m_rctY.area (), pf);
	}
	fclose (pf);
}

CVOPU8YUVBA::CVOPU8YUVBA (AlphaUsage fAUsage, const CRct& rc)
{
	m_puciY = NULL;
	m_puciU = NULL;
	m_puciV = NULL;
	m_puciBY = NULL;
	m_puciBUV = NULL;
	m_puciA = NULL;
	m_fAUsage = fAUsage;

	m_rctY = rc;
	m_rctUV = m_rctY.downSampleBy2 ();
	m_puciY = new CU8Image (m_rctY);
	assert (m_puciY != NULL);
	m_puciU = new CU8Image (m_rctUV);
	assert (m_puciU != NULL);
	m_puciV = new CU8Image (m_rctUV);
	assert (m_puciV != NULL);
	m_ppxlcY = (PixelC*) m_puciY->pixels ();
	m_ppxlcU = (PixelC*) m_puciU->pixels ();
	m_ppxlcV = (PixelC*) m_puciV->pixels ();
	if (m_fAUsage != RECTANGLE) {
		m_puciBY = new CU8Image (m_rctY, TRANSPARENT);		//initialize so that outside VOP is transp
		assert (m_puciBY != NULL);
		m_puciBUV = new CU8Image (m_rctUV, TRANSPARENT);	//initialize so that outside VOP is transp
		assert (m_puciBUV != NULL);
		m_ppxlcBY = (PixelC*) m_puciBY->pixels ();
		m_ppxlcBUV = (PixelC*) m_puciBUV->pixels ();
		if (m_fAUsage == EIGHT_BIT) {
			m_puciA = new CU8Image (m_rctY, TRANSPARENT);	//initialize so that outside VOP is transp
			assert (m_puciA != NULL);
			m_ppxlcA = (PixelC*) m_puciA->pixels ();
		}
	}
}

CVOPU8YUVBA::CVOPU8YUVBA (AlphaUsage fAUsage)
{
	m_puciY = NULL;
	m_puciU = NULL;
	m_puciV = NULL;
	m_puciBY = NULL;
	m_puciBUV = NULL;
	m_puciA = NULL;
	m_fAUsage = fAUsage;
}

Void CVOPU8YUVBA::shift (CoordI left, CoordI top)
{
	m_rctY.shift (left, top);
	m_rctUV.shift (left / 2, top / 2);
	m_puciY -> shift (left, top);
	m_puciU -> shift (left / 2, top / 2);
	m_puciV -> shift (left / 2, top / 2);
	if (m_fAUsage == EIGHT_BIT) 
		m_puciA -> shift (left, top);
	else if (m_fAUsage == ONE_BIT) {
		m_puciBY -> shift (left, top);
		m_puciBUV -> shift (left / 2, top / 2);
	}
}

Void CVOPU8YUVBA::setBoundRct (const CRct& rctBoundY)
{
	assert (rctBoundY <= m_rctY);
	m_rctBoundY = rctBoundY;
	m_rctBoundUV = m_rctBoundY.downSampleBy2 ();
	Int iOffsetY = m_rctY.offset (m_rctBoundY.left, m_rctBoundY.top);
	Int iOffsetUV = m_rctUV.offset (m_rctBoundUV.left, m_rctBoundUV.top);
	m_ppxlcBoundY = (PixelC*) m_puciY->pixels () + iOffsetY;
	m_ppxlcBoundU = (PixelC*) m_puciU->pixels () + iOffsetUV;
	m_ppxlcBoundV = (PixelC*) m_puciV->pixels () + iOffsetUV;
	if (m_fAUsage != RECTANGLE) {
		m_ppxlcBoundBY = (PixelC*) m_puciBY->pixels () + iOffsetY;
		m_ppxlcBoundBUV = (PixelC*) m_puciBUV->pixels () + iOffsetUV;
		if (m_fAUsage == EIGHT_BIT)
			m_ppxlcBoundA = (PixelC*) m_puciA->pixels () + iOffsetY;
	}
}

Void CVOPU8YUVBA::setAndExpandBoundRctOnly (const CRct& rctBoundY, Int iExpand)
{
	assert (rctBoundY <= m_rctY);
	m_rctBoundY = rctBoundY;
	m_rctBoundY.expand (iExpand);
	m_rctBoundUV = m_rctBoundY.downSampleBy2 ();
}

Void CVOPU8YUVBA::cropOnAlpha ()
{
	m_puciBY -> cropOnAlpha ();
	m_puciBUV -> cropOnAlpha ();
	m_puciY -> where (m_puciBY -> where ());
	m_puciU -> where (m_puciBUV -> where ());
	m_puciV -> where (m_puciBUV -> where ());
	if (m_fAUsage == EIGHT_BIT)
		m_puciA -> where (m_puciBY -> where ());
}

Void CVOPU8YUVBA::overlay (const CVOPU8YUVBA& vopc)
{
	if (!vopc.valid ()) return;
	if (m_puciBY)		//sometime rectangular vops dont have by
		m_puciBY -> overlay (*vopc.getPlane (BY_PLANE));
	if (m_puciBUV)
		m_puciBUV -> overlay (*vopc.getPlane (BUV_PLANE));
	m_puciY -> overlay (*vopc.getPlane (Y_PLANE));
	m_puciU -> overlay (*vopc.getPlane (U_PLANE));
	m_puciV -> overlay (*vopc.getPlane (V_PLANE));
	if (m_fAUsage == EIGHT_BIT)
		m_puciA -> overlay (*vopc.getPlane (A_PLANE));
}

Void CVOPU8YUVBA::overlay (const CVOPU8YUVBA& vopc, const CRct& rctY)
{	
	if (!vopc.valid ()) return;
	if (!rctY.valid ()) return;
	CRct rctUV = rctY.downSampleBy2 ();
	m_puciBY -> overlay (*vopc.getPlane (BY_PLANE), rctY);
	m_puciBUV -> overlay (*vopc.getPlane (BUV_PLANE), rctUV);
	m_puciY -> overlay (*vopc.getPlane (Y_PLANE), rctY);
	m_puciU -> overlay (*vopc.getPlane (U_PLANE), rctUV);
	m_puciV -> overlay (*vopc.getPlane (V_PLANE), rctUV);
	if (m_fAUsage == EIGHT_BIT)
		m_puciA -> overlay (*vopc.getPlane (A_PLANE), rctY);
}

own CVOPU8YUVBA* CVOPU8YUVBA::downsampleForSpatialScalability () const
{
	//only handle CIF->QCIF for RECTANGULAR VOP
	assert (m_fAUsage == RECTANGLE);
	//CRct rctDown = whereY ();
	assert (whereY().left == 0 && whereY().top == 0);
	CVOPU8YUVBA* pvopfRet = new CVOPU8YUVBA (m_fAUsage);
	assert (pvopfRet != NULL);
	pvopfRet -> m_puciY = m_puciY->downsampleForSpatialScalability ();
	pvopfRet -> m_puciU = m_puciU->downsampleForSpatialScalability ();
	pvopfRet -> m_puciV = m_puciV->downsampleForSpatialScalability ();
	pvopfRet -> m_puciBY = new CU8Image (pvopfRet->whereY (), opaqueValue);
	pvopfRet -> m_puciBUV = new CU8Image (pvopfRet->whereUV (), opaqueValue);
	return pvopfRet;
}

//wchen: maybe these upsampling functions should be in sys; do it later
own CVOPU8YUVBA* CVOPU8YUVBA::upsampleForSpatialScalability (Int iVerticalSamplingFactorM,
															 Int iVerticalSamplingFactorN,
															 Int iHorizontalSamplingFactorM,
															 Int iHorizontalSamplingFactorN,
															 Int iExpandYRefFrame,
															 Int iExpandUVRefFrame) const
{
	//only handle QCIF->CIF for RECTANGULAR VOP
	//assert (m_fAUsage == RECTANGLE);
	CVOPU8YUVBA* pvopfRet = new CVOPU8YUVBA (m_fAUsage);
	assert (pvopfRet != NULL);
	pvopfRet -> m_puciY = m_puciY->upsampleForSpatialScalability (iVerticalSamplingFactorM, iVerticalSamplingFactorN, iHorizontalSamplingFactorM, 
																  iHorizontalSamplingFactorN, 1, iExpandYRefFrame);
	pvopfRet -> m_puciU = m_puciU->upsampleForSpatialScalability (iVerticalSamplingFactorM, iVerticalSamplingFactorN, iHorizontalSamplingFactorM, 
																  iHorizontalSamplingFactorN, 2, iExpandYRefFrame);
	pvopfRet -> m_puciV = m_puciV->upsampleForSpatialScalability (iVerticalSamplingFactorM, iVerticalSamplingFactorN, iHorizontalSamplingFactorM, 
																  iHorizontalSamplingFactorN, 2, iExpandYRefFrame);
	pvopfRet -> m_puciBY = new CU8Image (pvopfRet->whereY (), opaqueValue);
	pvopfRet -> m_puciBUV = new CU8Image (pvopfRet->whereUV (), opaqueValue);
	pvopfRet->m_ppxlcY = (PixelC *) pvopfRet->m_puciY->pixels();
	pvopfRet->m_ppxlcU = (PixelC *) pvopfRet->m_puciU->pixels();
	pvopfRet->m_ppxlcV = (PixelC *) pvopfRet->m_puciV->pixels();
	pvopfRet->m_rctY   = pvopfRet->m_puciY->where();
	pvopfRet->m_rctUV  = pvopfRet->m_puciU->where();
	pvopfRet->m_rctBoundY.expand(-iExpandYRefFrame, -iExpandYRefFrame,-iExpandYRefFrame,-iExpandYRefFrame);
	pvopfRet->m_rctBoundUV.expand(-iExpandUVRefFrame,-iExpandUVRefFrame,-iExpandUVRefFrame,-iExpandUVRefFrame);

	pvopfRet->m_ppxlcBoundY=pvopfRet->m_ppxlcY+16+16*pvopfRet->m_rctY.width;
	pvopfRet->m_ppxlcBoundU=pvopfRet->m_ppxlcU +8 +8*pvopfRet->m_rctUV.width;
	pvopfRet->m_ppxlcBoundY=pvopfRet->m_ppxlcV +8 +8*pvopfRet->m_rctUV.width;
	return pvopfRet;
}


own Double* CVOPU8YUVBA::mse (const CVOPU8YUVBA& vopf) const
{
	assert (whereY () == vopf.whereY () && whereUV () == vopf.whereUV ());
	Double* rgdblMeanSquareError = new Double [4]; // mean square
	CU8Image* puciBOr = new CU8Image (*m_puciBY);
	puciBOr -> CU8Image_or (*vopf.getPlane (BY_PLANE));
	CU8Image* puciExp0 = new CU8Image (*m_puciY, puciBOr -> where ());
	CU8Image* puciExp1 = new CU8Image (*vopf.getPlane (Y_PLANE), puciBOr -> where ());
	rgdblMeanSquareError [0] = puciExp1 -> mse (*puciExp0, *puciBOr);
	delete puciExp0;
	delete puciExp1;
	if (fAUsage () == EIGHT_BIT) {
		puciExp0 = new CU8Image (*m_puciA, puciBOr -> where ());
		puciExp1 = new CU8Image (*vopf.getPlane (A_PLANE), puciBOr -> where ());
		rgdblMeanSquareError [3] = puciExp1 -> mse (*puciExp0, *puciBOr);
		delete puciExp0;
		delete puciExp1;
	}
	delete puciBOr;
	puciBOr = new CU8Image (*m_puciBUV);
	puciBOr -> CU8Image_or (*vopf.getPlane (BUV_PLANE));
	puciExp0 = new CU8Image (*m_puciU, puciBOr -> where ());
	puciExp1 = new CU8Image (*vopf.getPlane (U_PLANE), puciBOr -> where ());
	rgdblMeanSquareError [1] = puciExp1 -> mse (*puciExp0, *puciBOr);
	delete puciExp0;
	delete puciExp1;

	puciExp0 = new CU8Image (*m_puciV, puciBOr -> where ());
	puciExp1 = new CU8Image (*vopf.getPlane (V_PLANE), puciBOr -> where ());
	rgdblMeanSquareError [2] = puciExp1 -> mse (*puciExp0, *puciBOr);
	delete puciExp0;
	delete puciExp1;
	delete puciBOr;
	return rgdblMeanSquareError;
}

own Double* CVOPU8YUVBA::snr (const CVOPU8YUVBA& vopf) const
{
	assert (whereY () == vopf.whereY () && whereUV () == vopf.whereUV ());
	Double* psnr = new Double [4]; // mean square
	CU8Image* puciBOr = new CU8Image (*m_puciBY);
	puciBOr -> CU8Image_or (*vopf.getPlane (BY_PLANE));
	CU8Image* puciExp0 = new CU8Image (*m_puciY, puciBOr -> where ());
	CU8Image* puciExp1 = new CU8Image (*vopf.getPlane (Y_PLANE), puciBOr -> where ());
	psnr [0] = puciExp1 -> snr (*puciExp0, *puciBOr);
	delete puciExp0;
	delete puciExp1;
	if (fAUsage () == EIGHT_BIT) {
		puciExp0 = new CU8Image (*m_puciA, puciBOr -> where ());
		puciExp1 = new CU8Image (*vopf.getPlane (A_PLANE), puciBOr -> where ());
		psnr [3] = puciExp1 -> snr (*puciExp0, *puciBOr);
		delete puciExp0;
		delete puciExp1;
	}
	delete puciBOr;
	puciBOr = new CU8Image (*m_puciBUV);
	puciBOr -> CU8Image_or (*vopf.getPlane (BUV_PLANE));
	puciExp0 = new CU8Image (*m_puciU, puciBOr -> where ());
	puciExp1 = new CU8Image (*vopf.getPlane (U_PLANE), puciBOr -> where ());
	psnr [1] = puciExp1 -> snr (*puciExp0, *puciBOr);
	delete puciExp0;
	delete puciExp1;

	puciExp0 = new CU8Image (*m_puciV, puciBOr -> where ());
	puciExp1 = new CU8Image (*vopf.getPlane (V_PLANE), puciBOr -> where ());
	psnr [2] = puciExp1 -> snr (*puciExp0, *puciBOr);
	delete puciExp0;
	delete puciExp1;
	delete puciBOr;
	return psnr;
}

const CU8Image* CVOPU8YUVBA::getPlane (PlaneType plnType) const
{
	if (plnType == Y_PLANE) return m_puciY;
	else if (plnType == U_PLANE) return m_puciU;
	else if (plnType == V_PLANE) return m_puciV;
	else if (plnType == BY_PLANE) return m_puciBY;
	else if (plnType == BUV_PLANE) return m_puciBUV;
	else if (plnType == A_PLANE) return m_puciA;
	else return NULL;
}

Void CVOPU8YUVBA::dump (FILE* pfFile) const
{
	fwrite (m_ppxlcY, m_rctY .area(), 1, pfFile);
	fwrite (m_ppxlcU, m_rctUV.area(), 1, pfFile);
	fwrite (m_ppxlcV, m_rctUV.area(), 1, pfFile);
}

Void CVOPU8YUVBA::dump (const Char* pchFileName) const
{
	FILE* pfOut = fopen (pchFileName, "wb");
	dump (pfOut);
	fclose (pfOut);
}

Void CVOPU8YUVBA::vdlDump (const Char* pchFileName, const CRct& rct) const
{
	CRct rctROI = (!rct.valid ()) ? m_rctY : rct;
	assert (rctROI <= m_rctY);
	CVideoObjectPlane vop (rctROI, opaquePixel);
	Int offset = (rctROI == m_rctY)? 0 : m_rctY.width - rct.width;
	CU8Image* puciZoomedU = m_puciU -> zoomup (2, 2);
	CU8Image* puciZoomedV = m_puciV -> zoomup (2, 2);
	PixelC* ppxlucY = (PixelC*) m_puciY -> pixels ();
	PixelC* ppxlucU = (PixelC*) puciZoomedU -> pixels ();
	PixelC* ppxlucV = (PixelC*) puciZoomedV -> pixels ();
	PixelC* ppxlucA = NULL;
	if (m_fAUsage == EIGHT_BIT)
		ppxlucA = (PixelC*) m_puciA -> pixels ();
	else if (m_fAUsage == ONE_BIT)
		ppxlucA = (PixelC*) m_puciBY -> pixels ();
	CPixel* ppxl = (CPixel*) vop.pixels ();
	for (CoordI y = rctROI.top; y < rctROI.bottom; y++) {
		for (CoordI x = rctROI.left; x < rctROI.right; x++, ppxlucY++, ppxlucU++, ppxlucV++, ppxlucA++) {
			Double var = 1.164 * (*ppxlucY - 16);
			Int r = (Int) ((Double) (var + 1.596 * (*ppxlucV - 128)) + .5);
			Int g = (Int) ((Double) (var - 0.813 * (*ppxlucV - 128) - 0.391 * (*ppxlucU - 128)) + .5);
			Int b = (Int) ((Double) (var + 2.018 * (*ppxlucU - 128)) + .5);
			Int a = (m_fAUsage == RECTANGLE)? opaqueValue : *ppxlucA;	
			ppxl -> pxlU.rgb.r = (U8) checkrange (r, 0, 255);
			ppxl -> pxlU.rgb.g = (U8) checkrange (g, 0, 255);
			ppxl -> pxlU.rgb.b = (U8) checkrange (b, 0, 255);
			ppxl -> pxlU.rgb.a = (U8) checkrange (a, 0, 255);
			ppxl++;
		}
		ppxlucY += offset; ppxlucU += offset; ppxlucV += offset; ppxlucA += offset;
	}
	delete puciZoomedU;
	delete puciZoomedV;
	vop.vdlDump (pchFileName);
}

Void CVOPU8YUVBA::addBYPlain ()
{
	m_puciBY = new CU8Image (m_rctY, 255);		//initialize so that outside VOP is transp
	assert (m_puciBY != NULL);
	m_puciBUV = new CU8Image (m_rctUV, 255);	//initialize so that outside VOP is transp
	assert (m_puciBUV != NULL);
	m_ppxlcBY = (PixelC*) m_puciBY->pixels ();
	m_ppxlcBUV = (PixelC*) m_puciBUV->pixels ();
	if (m_fAUsage == EIGHT_BIT) {
		m_puciA = new CU8Image (m_rctY, 255);	//initialize so that outside VOP is transp
		assert (m_puciA != NULL);
		m_ppxlcA = (PixelC*) m_puciA->pixels ();
	}
}

Void CVOPU8YUVBA::addBYPlain (const CRct& rct, const CRct& rctUV)
{
	CU8Image* puciTemp = new CU8Image ( rct, OPAQUE );
	//m_puciBY = new CU8Image (m_rctY, 255);		//initialize so that outside VOP is transp
	m_puciBY = new CU8Image (m_rctY, TRANSPARENT);		//initialize so that outside VOP is transp
	m_puciBY->CU8Image_or( *puciTemp );				// for background composition, mask of whole image is generated here
	assert (m_puciBY != NULL);

	CU8Image* puciTempUV = new CU8Image ( rctUV, OPAQUE );
	//m_puciBUV = new CU8Image (m_rctUV, 255);		//initialize so that outside VOP is transp
	m_puciBUV = new CU8Image (m_rctUV, TRANSPARENT);	//initialize so that outside VOP is transp
	m_puciBUV->CU8Image_or( *puciTempUV );				// for background composition, mask of whole image is generated here
	assert (m_puciBUV != NULL);

	m_ppxlcBY = (PixelC*) m_puciBY->pixels ();
	m_ppxlcBUV = (PixelC*) m_puciBUV->pixels ();
	if (m_fAUsage == EIGHT_BIT) {
		//m_puciA = new CU8Image (m_rctY, 255);		//initialize so that outside VOP is transp
		m_puciA = new CU8Image (m_rctY, TRANSPARENT);	//initialize so that outside VOP is transp
		m_puciA->CU8Image_or( *puciTemp );
		assert (m_puciA != NULL);
		m_ppxlcA = (PixelC*) m_puciA->pixels ();
	}
}

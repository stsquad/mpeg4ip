/*************************************************************************

This software module was originally developed by 

	Ming-Chieh Lee (mingcl@microsoft.com), Microsoft Corporation
	Wei-ge Chen (wchen@microsoft.com), Microsoft Corporation
	Bruce Lin (blin@microsoft.com), Microsoft Corporation
	Chuang Gu (chuanggu@microsoft.com), Microsoft Corporation
	(date: March, 1996)
and edited by
        Wei Wu (weiwu@stallion.risc.rockwell.com) Rockwell Science Center

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

	vopSes.cpp

Abstract:

	Base class for the encoder for one VOP session.

Revision History:
	December 20, 1997:	Interlaced tools added by NextLevel Systems (GI)
                        X.Chen (xchen@nlvl.com), B. Eifrig (beifrig@nlvl.com)
        May. 9   1998:  add boundary by Hyundai Electronics 
                                  Cheol-Soo Park (cspark@super5.hyundai.co.kr) 
        May. 9   1998:  add field based unrestricted MC padding by Hyundai Electronics 
                                  Cheol-Soo Park (cspark@super5.hyundai.co.kr) 
*************************************************************************/

#include <stdio.h>

#include "typeapi.h"
#include "basic.hpp"
#include "header.h"
#include "codehead.h"
#include "mode.hpp"
#include "dct.hpp"
#include "cae.h"
#include "vopses.hpp"

#ifdef __MFC_
#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

#define new DEBUG_NEW				   
#endif // __MFC_

CVideoObject::~CVideoObject ()
{
	delete m_pvopcCurrQ;
	delete m_pvopcRefQ0;
	delete m_pvopcRefQ1;       // modified by Shu 12/12/97
							   // wchen: unmodified 12/16/97

	delete m_pvopcCurrMB;
	delete m_pvopcPredMB;
	delete m_pvopiErrorMB;
	delete [] m_rgmbmd;
	delete [] m_rgmv;
	delete [] m_rgmvBY;
    delete [] m_rgmvRef;
	delete m_pidct;

	Int nBlk = (m_volmd.fAUsage == EIGHT_BIT) ? 10 : 6;
	Int iBlk;
	for (iBlk = 0; iBlk < nBlk; iBlk++)
		delete [] m_rgpiCoefQ [iBlk];
	delete [] m_rgpiCoefQ;
//	delete m_pvopfCurrFilteredQ;
//	delete m_pdrtmdRef1;

	delete [] m_rgiQPpred;

	if (m_volmd.fAUsage == RECTANGLE) {
		Int nBlk = (m_volmd.fAUsage == EIGHT_BIT) ? 10 : 6;
		delete [] m_rgblkmCurrMB;
        if (m_rgpmbmAbove != NULL)
		    for (Int iMB = 0; iMB < m_iNumMBX; iMB++)	{
			    for (iBlk = 0; iBlk < nBlk; iBlk++)	{
				    delete [] (m_rgpmbmAbove [iMB]->rgblkm) [iBlk];
				    delete [] (m_rgpmbmCurr  [iMB]->rgblkm) [iBlk];
			    }
			    delete [] m_rgpmbmAbove [iMB]->rgblkm;
			    delete m_rgpmbmAbove [iMB];
			    delete [] m_rgpmbmCurr [iMB]->rgblkm;
			    delete m_rgpmbmCurr [iMB];
		    }
		delete [] m_rgpmbmAbove;
		delete [] m_rgpmbmCurr;
	}

	// sprite
	if (m_uiSprite == 1) {
		delete [] m_rgstSrcQ;
		delete [] m_rgstDstQ;
		if (m_sptMode == BASIC_SPRITE)
			delete m_pvopcSptQ;
	}
	delete m_pbEmptyRowArray;

/* NBIT: change
	m_rgiClipTab -= 400;
*/
	m_rgiClipTab -= m_iOffset;
	delete [] m_rgiClipTab;
	m_rgiClipTab = NULL;

	// shape
	if(m_rgshpmd!=NULL)
		delete m_rgshpmd;
	delete m_puciPredBAB;
	delete m_parcodec;
	delete [] m_ppxlcCurrMBBYDown4;
	delete [] m_ppxlcCurrMBBYDown2;
	delete [] m_ppxlcReconCurrBAB;
	delete [] m_ppxlcPredBABDown2;
	delete [] m_ppxlcPredBABDown4;
}

CVideoObject::CVideoObject ()
{
	m_t = 0;
	m_iBCount = 0;
	m_tPastRef = 0;
	m_tFutureRef = 0;
	m_bCodedFutureRef = TRUE;
	m_rgmbmd = NULL;
	m_rgmv = NULL;
	m_rgmvBackward = NULL;
	m_rgmvBY = NULL;
	m_rgmbmdRef = NULL;
	m_rgmvRef = NULL;
	m_tModuloBaseDecd = 0;
	m_tModuloBaseDisp = 0;
	m_pvopcCurrQ = NULL;
	m_pvopcRefQ0 = NULL; 
	m_pvopcRefQ1 = NULL;
	m_puciPredBAB = NULL;
	m_ppxlcReconCurrBAB = NULL;
	m_parcodec = NULL;
	m_ppxlcCurrMBBYDown4 = NULL;
	m_ppxlcCurrMBBYDown2 = NULL;
	m_ppxlcPredBABDown4 = NULL;
	m_ppxlcPredBABDown2 = NULL;
	m_pvopcSptQ = NULL;
	m_uiSprite = 0;
	m_rgstSrcQ = NULL;
	m_rgstDstQ = NULL;
	m_iOffsetForPadY = 0;
	m_iOffsetForPadUV = 0;
	m_rgshpmd = NULL;
	m_pbEmptyRowArray = new Bool [MB_SIZE];

/* NBIT: replaced by setClipTab() function
	m_rgiClipTab = new U8 [1024];
	m_rgiClipTab += 400;
	Int i;
	for (i = -400; i < 624; i++)
		m_rgiClipTab [i] = (i < 0) ? 0 : (i > 255) ? 255 : i;
*/

}

// NBIT: added function
Void CVideoObject::setClipTab()
{
	Int TabSize = 1<<(m_volmd.nBits+2);
	Int maxVal = (1<<m_volmd.nBits)-1;
	m_iOffset = TabSize/2;
	m_rgiClipTab = new PixelC [TabSize];
	m_rgiClipTab += m_iOffset;
	Int i;
	for (i = -m_iOffset; i < m_iOffset; i++)
		m_rgiClipTab [i] = (i < 0) ? 0 : (i > maxVal) ? maxVal : i;
}

Void CVideoObject::allocateVOLMembers (Int iSessionWidth, Int iSessionHeight)
{
	m_pvopcCurrMB = new CVOPU8YUVBA (m_volmd.fAUsage, CRct (0, 0, MB_SIZE, MB_SIZE));
	m_ppxlcCurrMBY = (PixelC*) m_pvopcCurrMB->pixelsY ();
	m_ppxlcCurrMBU = (PixelC*) m_pvopcCurrMB->pixelsU ();
	m_ppxlcCurrMBV = (PixelC*) m_pvopcCurrMB->pixelsV ();
	m_ppxlcCurrMBBY = (PixelC*) m_pvopcCurrMB->pixelsBY ();
	m_ppxlcCurrMBBUV = (PixelC*) m_pvopcCurrMB->pixelsBUV ();
	m_ppxlcCurrMBA = (PixelC*) m_pvopcCurrMB->pixelsA ();

	m_pvopcPredMB = new CVOPU8YUVBA (m_volmd.fAUsage, CRct (0, 0, MB_SIZE, MB_SIZE));
	m_ppxlcPredMBY = (PixelC*) m_pvopcPredMB->pixelsY ();
	m_ppxlcPredMBU = (PixelC*) m_pvopcPredMB->pixelsU ();
	m_ppxlcPredMBV = (PixelC*) m_pvopcPredMB->pixelsV ();
	m_ppxlcPredMBA = (PixelC*) m_pvopcPredMB->pixelsA ();
	
	// B-VOP MB buffer data
	m_pvopcPredMBBack = new CVOPU8YUVBA (m_volmd.fAUsage, CRct (0, 0, MB_SIZE, MB_SIZE));
	m_ppxlcPredMBBackY = (PixelC*) m_pvopcPredMBBack->pixelsY ();
	m_ppxlcPredMBBackU = (PixelC*) m_pvopcPredMBBack->pixelsU ();
	m_ppxlcPredMBBackV = (PixelC*) m_pvopcPredMBBack->pixelsV ();
	m_ppxlcPredMBBackA = (PixelC*) m_pvopcPredMBBack->pixelsA ();

	m_pvopiErrorMB = new CVOPIntYUVBA (m_volmd.fAUsage, CRct (0, 0, MB_SIZE, MB_SIZE));
	m_ppxliErrorMBY = (PixelI*) m_pvopiErrorMB->getPlane (Y_PLANE)->pixels ();
	m_ppxliErrorMBU = (PixelI*) m_pvopiErrorMB->getPlane (U_PLANE)->pixels ();
	m_ppxliErrorMBV = (PixelI*) m_pvopiErrorMB->getPlane (V_PLANE)->pixels ();
	m_ppxliErrorMBA = (PixelI*) m_pvopiErrorMB->getPlane (A_PLANE)->pixels ();

	if (m_uiSprite == 0)
		m_pvopcCurrQ = new CVOPU8YUVBA (m_volmd.fAUsage, m_rctRefFrameY);
	else //for sprite m_rctRefFrameY is for whole sprite, m_rctOrg is the display window 
// dshu: begin of modification
	//	m_pvopcCurrQ = new CVOPU8YUVBA (m_volmd.fAUsage, m_rctDisplayWindow);
	{
		CRct rctCurrQ = CRct (
			-EXPANDY_REF_FRAME, -EXPANDY_REF_FRAME, 
			EXPANDY_REF_FRAME + m_rctDisplayWindow.width, EXPANDY_REF_FRAME + m_rctDisplayWindow.height()
		);

		m_pvopcCurrQ = new CVOPU8YUVBA (m_volmd.fAUsage, rctCurrQ);
	}
// dshu: end of modification

    assert (m_pvopcCurrQ != NULL);
	m_pvopcRefQ0 = new CVOPU8YUVBA (m_volmd.fAUsage, m_rctRefFrameY);
	assert (m_pvopcRefQ0 != NULL);
	m_pvopcRefQ1 = new CVOPU8YUVBA (m_volmd.fAUsage, m_rctRefFrameY);
	assert (m_pvopcRefQ1 != NULL);

	m_iFrameWidthY = m_pvopcRefQ0->whereY ().width;
	m_iFrameWidthUV = m_pvopcRefQ0->whereUV ().width;
	m_iFrameWidthYxMBSize = MB_SIZE * m_pvopcRefQ0->whereY ().width; 
	m_iFrameWidthYxBlkSize = BLOCK_SIZE * m_pvopcRefQ0->whereY ().width; 
	m_iFrameWidthUVxBlkSize = BLOCK_SIZE * m_pvopcRefQ0->whereUV ().width;

	// MB data	
	Int nBlk = (m_volmd.fAUsage == EIGHT_BIT) ? 10 : 6;
	m_rgpiCoefQ = new Int* [nBlk];
	m_rgiQPpred = new Int [nBlk];
	Int iBlk;
	for (iBlk = 0; iBlk < nBlk; iBlk++)
		m_rgpiCoefQ [iBlk] = new Int [BLOCK_SQUARE_SIZE];
/* NBIT: change
	m_pidct = new CInvBlockDCT;
*/
	m_pidct = new CInvBlockDCT(m_volmd.nBits);

	// motion vectors and MBMode
	Int iNumMBX = iSessionWidth / MB_SIZE; 
	if (iSessionWidth  % MB_SIZE != 0)				//round up
		iNumMBX++;
	Int iNumMBY = iSessionHeight / MB_SIZE;
	if (iSessionHeight % MB_SIZE != 0)				//deal with frational MB
		iNumMBY++;
	Int iNumMB = m_iSessNumMB = iNumMBX * iNumMBY;

	m_rgmbmd = new CMBMode [iNumMB];
	m_rgmv = new CMotionVector [max(PVOP_MV_PER_REF_PER_MB, 2*BVOP_MV_PER_REF_PER_MB) * iNumMB];
	m_rgmvBackward = m_rgmv + BVOP_MV_PER_REF_PER_MB * m_iSessNumMB;
	m_rgmvRef = new CMotionVector [max(PVOP_MV_PER_REF_PER_MB, 2*BVOP_MV_PER_REF_PER_MB) * iNumMB];
	m_rgmvBY = new CMotionVector [iNumMB]; // for shape
	m_rgmbmdRef = new CMBMode [iNumMB];

	if (m_volmd.volType == ENHN_LAYER)
		m_rgshpmd = new ShapeMode [iNumMB];

	// shape data
	if (m_volmd.fAUsage != RECTANGLE) {
		m_puciPredBAB = new CU8Image (CRct (0, 0, MC_BAB_SIZE, MC_BAB_SIZE));
		m_ppxlcReconCurrBAB = new PixelC [BAB_SIZE * BAB_SIZE];
		m_parcodec = new ArCodec;
		m_ppxlcCurrMBBYDown4 = new PixelC [8 * 8];
		m_ppxlcCurrMBBYDown2 = new PixelC [12 * 12];
		m_ppxlcPredBABDown4 = new PixelC [6*6];
		m_ppxlcPredBABDown2 = new PixelC [10*10];
	}
}

Void CVideoObject::computeVOLConstMembers ()
{
	m_iOffsetForPadY = m_rctRefFrameY.offset (m_rctCurrVOPY.left, m_rctCurrVOPY.top);
	m_iOffsetForPadUV = m_rctRefFrameUV.offset (m_rctCurrVOPUV.left, m_rctCurrVOPUV.top);
	m_rctPrevNoExpandY = m_rctCurrVOPY;
	m_rctPrevNoExpandUV = m_rctCurrVOPUV;
	m_iVOPWidthY = m_rctCurrVOPY.width;
	m_iVOPWidthUV = m_rctCurrVOPUV.width;
	m_iNumMBX = m_iNumMBXRef = m_iVOPWidthY / MB_SIZE; 
	m_iNumMBY = m_iNumMBYRef = m_rctCurrVOPY.height () / MB_SIZE;
	m_iNumMB = m_iNumMBRef = m_iNumMBX * m_iNumMBY;
	m_iNumOfTotalMVPerRow = PVOP_MV_PER_REF_PER_MB * m_iNumMBX;
	setRefStartingPointers ();
	m_pvopcCurrQ->setBoundRct (m_rctCurrVOPY);
	m_pvopcRefQ0->setBoundRct (m_rctRefVOPY0);
	m_pvopcRefQ1->setBoundRct (m_rctRefVOPY1);

	Int iMB, iBlk;
	Int nBlk = (m_volmd.fAUsage == EIGHT_BIT) ? 10 : 6;
	m_rgblkmCurrMB = new BlockMemory [nBlk];

	m_rgpmbmAbove = new MacroBlockMemory* [m_iNumMBX];
	m_rgpmbmCurr  = new MacroBlockMemory* [m_iNumMBX];
	for (iMB = 0; iMB < m_iNumMBX; iMB++)	{
		m_rgpmbmAbove [iMB] = new MacroBlockMemory;
		m_rgpmbmAbove [iMB]->rgblkm = new BlockMemory [nBlk];
		m_rgpmbmCurr  [iMB] = new MacroBlockMemory;
		m_rgpmbmCurr  [iMB]->rgblkm = new BlockMemory [nBlk];
		for (iBlk = 0; iBlk < nBlk; iBlk++)	{
			(m_rgpmbmAbove [iMB]->rgblkm) [iBlk] = new Int [(BLOCK_SIZE << 1) - 1];
			(m_rgpmbmCurr  [iMB]->rgblkm) [iBlk] = new Int [(BLOCK_SIZE << 1) - 1];
		}
	}
}

Void CVideoObject::computeVOPMembers ()
{
	m_iVOPWidthY = m_rctCurrVOPY.width;
	m_iVOPWidthUV = m_rctCurrVOPUV.width;
	m_iNumMBX = m_iVOPWidthY / MB_SIZE; 
	m_iNumMBY = m_rctCurrVOPY.height () / MB_SIZE;
	m_iNumMB = m_iNumMBX * m_iNumMBY;
	m_iNumOfTotalMVPerRow = PVOP_MV_PER_REF_PER_MB * m_iNumMBX;

	Int iMB, iBlk;
	Int nBlk = (m_volmd.fAUsage == EIGHT_BIT) ? 10 : 6;
	m_rgblkmCurrMB = new BlockMemory [nBlk];

	m_rgpmbmAbove = new MacroBlockMemory* [m_iNumMBX];
	m_rgpmbmCurr  = new MacroBlockMemory* [m_iNumMBX];
	for (iMB = 0; iMB < m_iNumMBX; iMB++)	{
		m_rgpmbmAbove [iMB] = new MacroBlockMemory;
		m_rgpmbmAbove [iMB]->rgblkm = new BlockMemory [nBlk];
		m_rgpmbmCurr  [iMB] = new MacroBlockMemory;
		m_rgpmbmCurr  [iMB]->rgblkm = new BlockMemory [nBlk];
		for (iBlk = 0; iBlk < nBlk; iBlk++)	{
			// BLOCK_SIZE*2-1 is 15 Ints for dc/ac prediction of coefficients
			(m_rgpmbmAbove [iMB]->rgblkm) [iBlk] = new Int [(BLOCK_SIZE << 1) - 1];
			(m_rgpmbmCurr  [iMB]->rgblkm) [iBlk] = new Int [(BLOCK_SIZE << 1) - 1];
		}
	}
}

Void CVideoObject::repeatPadYOrA (PixelC* ppxlcOldLeft, CVOPU8YUVBA* pvopcRef)
{
	PixelC* ppxlcLeftTop = ppxlcOldLeft - EXPANDY_REFVOP * m_iFrameWidthY - EXPANDY_REFVOP;
		// point to the left top of the expanded bounding box (+- EXPANDY_REFVOP)
	const PixelC* ppxlcOldRight = ppxlcOldLeft + ((m_volmd.fAUsage == RECTANGLE)?m_ivolWidth:m_rctPrevNoExpandY.width) - 1; // modified by Sharp (99/1/14)
		// point to the right-top of the bounding box
	const PixelC* ppxlcOldTopLn = ppxlcOldLeft - EXPANDY_REFVOP;
		// point to the (left, top) - EXPANDY_REFVOP
	PixelC* ppxlcNewLeft = (PixelC*) ppxlcOldTopLn;
	PixelC* ppxlcNewRight = (PixelC*) ppxlcOldRight + 1;
	CoordI y;

	// begin:  modified by Sharp (99/1/14)
	Int PrevNoExpandY_height = (m_volmd.fAUsage == RECTANGLE)?m_ivolHeight:m_rctPrevNoExpandY.height ();
	for (y = 0; y < PrevNoExpandY_height; y++) { // x-direction interpolation
	// end: modified by Sharp
		for (Int i=0; i<EXPANDY_REFVOP; i++) {
			ppxlcNewLeft[i] = *ppxlcOldLeft;
			ppxlcNewRight[i] = *ppxlcOldRight;
		}
		ppxlcNewLeft += pvopcRef->whereY ().width;		
		ppxlcNewRight += pvopcRef->whereY ().width;
		ppxlcOldLeft += pvopcRef->whereY ().width;
		ppxlcOldRight += pvopcRef->whereY ().width;
	}
	/* commented out due to fact of no field padding now
	// Added for field based unrestricted MC padding by Hyundai(1998-5-9)
        if (m_vopmd.bInterlace) {      
		Int 	iUnit = sizeof (PixelC); 
                Int     width = pvopcRef->whereY ().width;
                Int     iFieldSkip = 2*width;
                Int     iPadArea = (m_rctPrevNoExpandY.width + 2 * EXPANDY_REFVOP)*iUnit;
                const PixelC
                        *ppxlcSrcTopFieldLn1 = ppxlcOldTopLn,
                        *ppxlcSrcTopFieldLn2 = ppxlcNewLeft  - iFieldSkip,
                        *ppxlcSrcBotFieldLn1 = ppxlcOldTopLn + width,
                        *ppxlcSrcBotFieldLn2 = ppxlcNewLeft  - width;
                PixelC  *ppxlcDstTopFieldLn1 = ppxlcLeftTop,
                        *ppxlcDstTopFieldLn2 = ppxlcNewLeft,
                        *ppxlcDstBotFieldLn1 = ppxlcLeftTop + width,
                        *ppxlcDstBotFieldLn2 = ppxlcNewLeft + width;
                for (y = 0; y < EXPANDY_REFVOP/2; y++) {
                        memcpy (ppxlcDstTopFieldLn1, ppxlcSrcTopFieldLn1, iPadArea);
                        memcpy (ppxlcDstTopFieldLn2, ppxlcSrcTopFieldLn2, iPadArea);
                        memcpy (ppxlcDstBotFieldLn1, ppxlcSrcBotFieldLn1, iPadArea);
                        memcpy (ppxlcDstBotFieldLn2, ppxlcSrcBotFieldLn2, iPadArea);
                        ppxlcDstTopFieldLn1 += iFieldSkip;
                        ppxlcDstTopFieldLn2 += iFieldSkip;
                        ppxlcDstBotFieldLn1 += iFieldSkip;
                        ppxlcDstBotFieldLn2 += iFieldSkip;
                }
                return;
        }
	// End of Hyundai(1998-5-9)
	*/

	const PixelC* ppxlcOldBotLn = ppxlcNewLeft - pvopcRef->whereY ().width;
	for (y = 0; y < EXPANDY_REFVOP; y++) {
		memcpy (ppxlcLeftTop, ppxlcOldTopLn, (m_rctPrevNoExpandY.width + 2 * EXPANDY_REFVOP)*sizeof(PixelC));
		memcpy (ppxlcNewLeft, ppxlcOldBotLn, (m_rctPrevNoExpandY.width + 2 * EXPANDY_REFVOP)*sizeof(PixelC));
		ppxlcNewLeft += pvopcRef->whereY ().width;
		ppxlcLeftTop += pvopcRef->whereY ().width;
	}
}

Void CVideoObject::repeatPadUV (CVOPU8YUVBA* pvopcRef)
{
	const PixelC* ppxlcOldLeftU = pvopcRef->pixelsU () + m_iOffsetForPadUV; // point to the left-top of bounding box
	const PixelC* ppxlcOldLeftV = pvopcRef->pixelsV () + m_iOffsetForPadUV; // point to the left-top of bounding box
	PixelC* ppxlcLeftTopU = (PixelC*) ppxlcOldLeftU - EXPANDUV_REFVOP * m_iFrameWidthUV - EXPANDUV_REFVOP;
	PixelC* ppxlcLeftTopV = (PixelC*) ppxlcOldLeftV - EXPANDUV_REFVOP * m_iFrameWidthUV - EXPANDUV_REFVOP;
		// point to the left top of the expanded bounding box (+- EXPANDY_REFVOP)
	const PixelC* ppxlcOldRightU = ppxlcOldLeftU + ((m_volmd.fAUsage == RECTANGLE)?m_ivolWidth/2:m_rctPrevNoExpandUV.width) - 1; // modified by Sharp (99/1/14)
	const PixelC* ppxlcOldRightV = ppxlcOldLeftV + ((m_volmd.fAUsage == RECTANGLE)?m_ivolWidth/2:m_rctPrevNoExpandUV.width) - 1; // modified by Sharp (99/1/14)
		// point to the right-top of the bounding box
	const PixelC* ppxlcOldTopLnU = ppxlcOldLeftU - EXPANDUV_REFVOP;
	const PixelC* ppxlcOldTopLnV = ppxlcOldLeftV - EXPANDUV_REFVOP;
		// point to the (left, top) - EXPANDY_REFVOP
	PixelC* ppxlcNewLeftU = (PixelC*) ppxlcOldTopLnU;
	PixelC* ppxlcNewLeftV = (PixelC*) ppxlcOldTopLnV;
	PixelC* ppxlcNewRightU = (PixelC*) ppxlcOldRightU + 1;
	PixelC* ppxlcNewRightV = (PixelC*) ppxlcOldRightV + 1;
	
	CoordI y;
	// begin:  modified by Sharp (99/1/14)
	Int PrevNoExpandUV_height = (m_volmd.fAUsage == RECTANGLE)?m_ivolHeight/2:m_rctPrevNoExpandUV.height ();
	for (y = 0; y < PrevNoExpandUV_height; y++) { // x-direction interpolation
	// end:  modified by Sharp (99/1/14)
		for (Int i=0; i<EXPANDUV_REFVOP; i++) {
			ppxlcNewLeftU[i] = *ppxlcOldLeftU;
			ppxlcNewLeftV[i] = *ppxlcOldLeftV;
			ppxlcNewRightU[i] = *ppxlcOldRightU;
			ppxlcNewRightV[i] = *ppxlcOldRightV;
		}
		
		ppxlcNewLeftU += pvopcRef->whereUV ().width;		
		ppxlcNewLeftV += pvopcRef->whereUV ().width;		
		ppxlcNewRightU += pvopcRef->whereUV ().width;
		ppxlcNewRightV += pvopcRef->whereUV ().width;
		ppxlcOldLeftU += pvopcRef->whereUV ().width;
		ppxlcOldLeftV += pvopcRef->whereUV ().width;
		ppxlcOldRightU += pvopcRef->whereUV ().width;
		ppxlcOldRightV += pvopcRef->whereUV ().width;
	}
	/* no longer any field based padding
	// Added for field based unrestricted MC padding by Hyundai(1998-5-9)
        if (m_vopmd.bInterlace) {
		Int	iUnit = sizeof(PixelC);
                Int     width = pvopcRef->whereUV ().width;
                Int     iFieldSkip = 2*width;
                Int     iPadArea = (m_rctPrevNoExpandUV.width + 2 * EXPANDUV_REFVOP)*iUnit;
                const PixelC 
                        *ppxlcSrcTopFieldLn1U = ppxlcOldTopLnU,
                        *ppxlcSrcTopFieldLn2U = ppxlcNewLeftU  - iFieldSkip,
                        *ppxlcSrcBotFieldLn1U = ppxlcOldTopLnU + width,
                        *ppxlcSrcBotFieldLn2U = ppxlcNewLeftU  - width,
                        *ppxlcSrcTopFieldLn1V = ppxlcOldTopLnV,
                        *ppxlcSrcTopFieldLn2V = ppxlcNewLeftV  - iFieldSkip,
                        *ppxlcSrcBotFieldLn1V = ppxlcOldTopLnV + width,
                        *ppxlcSrcBotFieldLn2V = ppxlcNewLeftV  - width;
                PixelC  *ppxlcDstTopFieldLn1U = ppxlcLeftTopU,
                        *ppxlcDstTopFieldLn2U = ppxlcNewLeftU,
                        *ppxlcDstBotFieldLn1U = ppxlcLeftTopU + width,
                        *ppxlcDstBotFieldLn2U = ppxlcNewLeftU + width,
                        *ppxlcDstTopFieldLn1V = ppxlcLeftTopV,
                        *ppxlcDstTopFieldLn2V = ppxlcNewLeftV,
                        *ppxlcDstBotFieldLn1V = ppxlcLeftTopV + width,
                        *ppxlcDstBotFieldLn2V = ppxlcNewLeftV + width;
                for (y = 0; y < EXPANDUV_REFVOP/2; y++) {
                        memcpy (ppxlcDstTopFieldLn1U, ppxlcSrcTopFieldLn1U, iPadArea);
                        memcpy (ppxlcDstTopFieldLn2U, ppxlcSrcTopFieldLn2U, iPadArea);
                        memcpy (ppxlcDstBotFieldLn1U, ppxlcSrcBotFieldLn1U, iPadArea);
                        memcpy (ppxlcDstBotFieldLn2U, ppxlcSrcBotFieldLn2U, iPadArea);
                        memcpy (ppxlcDstTopFieldLn1V, ppxlcSrcTopFieldLn1V, iPadArea);
                        memcpy (ppxlcDstTopFieldLn2V, ppxlcSrcTopFieldLn2V, iPadArea);
                        memcpy (ppxlcDstBotFieldLn1V, ppxlcSrcBotFieldLn1V, iPadArea);
                        memcpy (ppxlcDstBotFieldLn2V, ppxlcSrcBotFieldLn2V, iPadArea);
                        ppxlcDstTopFieldLn1U += iFieldSkip;
                        ppxlcDstTopFieldLn2U += iFieldSkip;
                        ppxlcDstBotFieldLn1U += iFieldSkip;
                        ppxlcDstBotFieldLn2U += iFieldSkip;
                        ppxlcDstTopFieldLn1V += iFieldSkip;
                        ppxlcDstTopFieldLn2V += iFieldSkip;
                        ppxlcDstBotFieldLn1V += iFieldSkip;
                        ppxlcDstBotFieldLn2V += iFieldSkip;
                }
                return;
        }
	// End of Hyundai(1998-5-9)
	*/

	const PixelC* ppxlcOldBotLnU = ppxlcNewLeftU - pvopcRef->whereUV ().width;
	const PixelC* ppxlcOldBotLnV = ppxlcNewLeftV - pvopcRef->whereUV ().width;
	for (y = 0; y < EXPANDUV_REFVOP; y++) {
		memcpy (ppxlcLeftTopU, ppxlcOldTopLnU, (m_rctPrevNoExpandUV.width + 2 * EXPANDUV_REFVOP)*sizeof(PixelC));
		memcpy (ppxlcLeftTopV, ppxlcOldTopLnV, (m_rctPrevNoExpandUV.width + 2 * EXPANDUV_REFVOP)*sizeof(PixelC));
		memcpy (ppxlcNewLeftU, ppxlcOldBotLnU, (m_rctPrevNoExpandUV.width + 2 * EXPANDUV_REFVOP)*sizeof(PixelC));
		memcpy (ppxlcNewLeftV, ppxlcOldBotLnV, (m_rctPrevNoExpandUV.width + 2 * EXPANDUV_REFVOP)*sizeof(PixelC));
		ppxlcNewLeftU += pvopcRef->whereUV ().width;
		ppxlcNewLeftV += pvopcRef->whereUV ().width;
		ppxlcLeftTopU += pvopcRef->whereUV ().width;
		ppxlcLeftTopV += pvopcRef->whereUV ().width;
	}
}

Void CVideoObject::saveShapeMode()
{
	// called after reference frame encode/decode
	if(m_rgshpmd == NULL)
	{
		// first time
		m_iRefShpNumMBX = m_iNumMBX;
		m_iRefShpNumMBY = m_iNumMBY;
		m_rgshpmd = new ShapeMode [m_iNumMB];
	}
	else
	{
		// update if changed
		if(m_iRefShpNumMBX!=m_iNumMBXRef || m_iRefShpNumMBY != m_iNumMBYRef)
		{
			delete [] m_rgshpmd;
			m_rgshpmd = new ShapeMode [m_iNumMBRef];
			m_iRefShpNumMBX = m_iNumMBXRef;
			m_iRefShpNumMBY = m_iNumMBYRef;
		}
		// copy shape mode from previous ref to save
		Int i;
		for (i=0; i<m_iNumMBRef; i++ )
			m_rgshpmd[i] = m_rgmbmdRef[i].m_shpmd;
	}
}

Void CVideoObject::resetBYPlane ()
{
	if (m_vopmd.vopPredType == PVOP || m_vopmd.vopPredType == IVOP) {
		PixelC* ppxlcBY = (PixelC*) m_pvopcRefQ1->pixelsBY ();
		memset (ppxlcBY, 0, m_pvopcRefQ1->whereY().area() * sizeof(PixelC));
	}
	else {
		PixelC* ppxlcBY = (PixelC*) m_pvopcCurrQ->pixelsBY ();
		memset (ppxlcBY, 0, m_pvopcCurrQ->whereY().area() * sizeof(PixelC));
	}
}

Void CVideoObject::updateAllRefVOPs ()
// perform this after VOP prediction type decided and before encoding
{
	if (m_vopmd.vopPredType != BVOP) {
		m_rctRefVOPY0 = m_rctRefVOPY1;
		swapVOPU8Pointers (m_pvopcRefQ0, m_pvopcRefQ1);
	}
}

// for spatial scalability
Void CVideoObject::updateAllRefVOPs (const CVOPU8YUVBA* pvopcRefBaseLayer)
{
	CVOPU8YUVBA *pvopcUpSampled = NULL;
	//	Int fEnhancementLayer = 0;

	assert (m_volmd.volType == ENHN_LAYER);

	pvopcUpSampled = pvopcRefBaseLayer->upsampleForSpatialScalability (
													m_volmd.iver_sampling_factor_m,
													m_volmd.iver_sampling_factor_n,
													m_volmd.ihor_sampling_factor_m,
													m_volmd.ihor_sampling_factor_n,
													EXPANDY_REF_FRAME,
													EXPANDUV_REF_FRAME);
	if(m_vopmd.vopPredType == PVOP) {
		m_rctRefVOPY0 = m_rctRefVOPY1;
		swapVOPU8Pointers (m_pvopcRefQ0,pvopcUpSampled);
		m_pvopcRefQ0->setBoundRct(m_rctRefVOPY0);
		delete pvopcUpSampled;
	}
	else if(m_vopmd.vopPredType == BVOP){
		CRct tmp;
		tmp = m_rctRefVOPY0;
		m_rctRefVOPY0 = m_rctRefVOPY1;
		m_rctRefVOPY1 = tmp;
		swapVOPU8Pointers (m_pvopcRefQ0,m_pvopcRefQ1);
		swapVOPU8Pointers (m_pvopcRefQ1,pvopcUpSampled);
		m_pvopcRefQ0->setBoundRct(m_rctRefVOPY0);
		m_pvopcRefQ1->setBoundRct(m_rctRefVOPY1);
		delete pvopcUpSampled;
	}
}

Void CVideoObject::swapVOPU8Pointers (CVOPU8YUVBA*& pvopc0, CVOPU8YUVBA*& pvopc1)
{
	CVOPU8YUVBA* pvopcTmp = pvopc0;
	pvopc0 = pvopc1;
	pvopc1 = pvopcTmp;
}

Void CVideoObject::swapVOPIntPointers (CVOPIntYUVBA*& pvopi0, CVOPIntYUVBA*& pvopi1)
{
	CVOPIntYUVBA* pvopiTmp = pvopi0;
	pvopi0 = pvopi1;
	pvopi1 = pvopiTmp;
}

Void CVideoObject::setRefStartingPointers ()
{
	m_iStartInRefToCurrRctY = m_rctRefFrameY.offset (m_rctCurrVOPY.left, m_rctCurrVOPY.top);
	m_iStartInRefToCurrRctUV = m_rctRefFrameUV.offset (m_rctCurrVOPUV.left, m_rctCurrVOPUV.top);
}

const CVOPU8YUVBA* CVideoObject::pvopcReconCurr () const
{
	if (m_vopmd.vopPredType == SPRITE && m_iNumOfPnts > 0)
		return m_pvopcCurrQ;
	else if (m_vopmd.vopPredType == SPRITE && m_iNumOfPnts == 0) {
		if (m_sptMode != BASIC_SPRITE) 
			return m_pvopcSptQ;
		else 
			return m_pvopcRefQ1;
	}
	else if ((m_vopmd.vopPredType == BVOP && m_volmd.volType == BASE_LAYER) 
			 ||(m_vopmd.vopPredType == BVOP && m_vopmd.iRefSelectCode != 0))
		return m_pvopcCurrQ;
	else
		return m_pvopcRefQ1;
}
////////// 97/12/22 start
Void CVideoObject::compute_bfShapeMembers ()
{
	m_iVOPWidthY = m_rctCurrVOPY.width;
	m_iVOPWidthUV = m_rctCurrVOPUV.width;
	m_iNumMBX = m_iVOPWidthY / MB_SIZE; 
	m_iNumMBY = m_rctCurrVOPY.height () / MB_SIZE;
	m_iNumMB = m_iNumMBX * m_iNumMBY;
//	m_iNumOfTotalMVPerRow = 5 * m_iNumMBX;
//  wchen: changed to const as discussed with Bob.
	m_iNumOfTotalMVPerRow = PVOP_MV_PER_REF_PER_MB * m_iNumMBX;
}

Void CVideoObject::copyVOPU8YUVBA(CVOPU8YUVBA*& pvopc0, CVOPU8YUVBA*& pvopc1)
{
	delete pvopc0;  pvopc0 = NULL;
	pvopc0 = new CVOPU8YUVBA (*pvopc1);	/* i.e. pvopc0 = pvopc1; */
}

Void CVideoObject::copyVOPU8YUVBA(CVOPU8YUVBA*& pvopc0, CVOPU8YUVBA*& pvopc1, CVOPU8YUVBA*& pvopc2)
{
	delete pvopc0;  pvopc0 = NULL;
	pvopc0 = (pvopc1 != NULL) ?
		new CVOPU8YUVBA (*pvopc1) : new CVOPU8YUVBA (*pvopc2);
}
///// 97/12/22 end

Void dumpNonCodedFrame(FILE* pfYUV, FILE* pfSeg, CRct& rct, UInt nBits)
{
	Int iW = rct.width;
	Int iH = rct.height();
	Int i;
	PixelC pxlcVal = 1<<(nBits-1);

	PixelC *ppxlcPix = new PixelC [iW];

	pxlcmemset(ppxlcPix, pxlcVal, iW);
	for(i=0; i<iH; i++) // Y
		fwrite(ppxlcPix, sizeof(PixelC), iW, pfYUV);
	for(i=0; i<iH; i++) // UV
		fwrite(ppxlcPix, sizeof(PixelC), iW>>1, pfYUV);
	if(pfSeg!=NULL)
	{
		pxlcmemset(ppxlcPix, 0, iW);
		for(i=0; i<iH; i++) // A
			fwrite(ppxlcPix, sizeof(PixelC), iW, pfSeg);
	}

	delete ppxlcPix;
}


/*BBM// Added for Boundary by Hyundai(1998-5-9)

Void CVideoObject::boundaryMacroBlockMerge (CMBMode* pmbmd)
{
        if ((pmbmd->m_rgTranspStatus[1] & pmbmd->m_rgTranspStatus[2]) == PARTIAL) {
                pmbmd->m_bMerged[1] = checkMergedStatus (7, 8, 7, 0);
                if (pmbmd->m_bMerged[1]) {
                        pmbmd->m_rgTranspStatusBBM[2] = ALL;
                        overlayBlocks (7, 8, 7, 0, pmbmd->m_dctMd);
                }
        }
        if ((pmbmd->m_rgTranspStatus[3] & pmbmd->m_rgTranspStatus[4]) == PARTIAL) {
                pmbmd->m_bMerged[2] = checkMergedStatus (7, 8, 15, 8);
                if (pmbmd->m_bMerged[2]) {
                        pmbmd->m_rgTranspStatusBBM[4] = ALL;
                        overlayBlocks (7, 8, 15, 8, pmbmd->m_dctMd);
                }
        }
        if (pmbmd->m_bMerged[1] | pmbmd->m_bMerged[2]) goto MERGE;
        else {
                if ((pmbmd->m_rgTranspStatus[1] & pmbmd->m_rgTranspStatus[3]) == PARTIAL) {
                        pmbmd->m_bMerged[3] = checkMergedStatus (7, 0, 7, 8);
                        if (pmbmd->m_bMerged[3]) {
                                pmbmd->m_rgTranspStatusBBM[3] = ALL;
                                overlayBlocks (7, 0, 7, 8, pmbmd->m_dctMd);
                        }
                }
                if ((pmbmd->m_rgTranspStatus[2] & pmbmd->m_rgTranspStatus[4]) == PARTIAL) {
                        pmbmd->m_bMerged[4] = checkMergedStatus (15, 8, 7, 8);
                        if (pmbmd->m_bMerged[4]) {
                                pmbmd->m_rgTranspStatusBBM[4] = ALL;
                                overlayBlocks (15, 8, 7, 8, pmbmd->m_dctMd);
                        }
                }
                if (pmbmd->m_bMerged[3] | pmbmd->m_bMerged[4]) goto MERGE;
                else {
                        if ((pmbmd->m_rgTranspStatus[1] & pmbmd->m_rgTranspStatus[4]) == PARTIAL) {
                                pmbmd->m_bMerged[5] = checkMergedStatus (7, 8, 7, 8);
                                if (pmbmd->m_bMerged[5]) {
                                        pmbmd->m_rgTranspStatusBBM[4] = ALL;
                                        overlayBlocks (7, 8, 7, 8, pmbmd->m_dctMd);
                                }
                        }
                        if ((pmbmd->m_rgTranspStatus[2] & pmbmd->m_rgTranspStatus[3]) == PARTIAL) {
                                pmbmd->m_bMerged[6] = checkMergedStatus (15, 0, 7, 8);
                                if (pmbmd->m_bMerged[6]) {
                                        pmbmd->m_rgTranspStatusBBM[3] = ALL;
                                        overlayBlocks (15, 0, 7, 8, pmbmd->m_dctMd);
                                }
                        }
                }
        }
MERGE:
        for (UInt x=1; x<7; x++)
                pmbmd->m_bMerged[0] |= pmbmd->m_bMerged[x];
        if (pmbmd->m_bMerged[0])
                swapTransparentModes (pmbmd, BBM);
}

Void CVideoObject::isBoundaryMacroBlockMerged (CMBMode* pmbmd)
{
        if ((pmbmd->m_rgTranspStatus[1] & pmbmd->m_rgTranspStatus[2]) == PARTIAL) {
                pmbmd->m_bMerged[1] = checkMergedStatus (7, 8, 7, 0);
                if (pmbmd->m_bMerged[1])
                        pmbmd->m_rgTranspStatusBBM[2] = ALL;
        }
        if ((pmbmd->m_rgTranspStatus[3] & pmbmd->m_rgTranspStatus[4]) == PARTIAL) {
                pmbmd->m_bMerged[2] = checkMergedStatus (7, 8, 15, 8);
                if (pmbmd->m_bMerged[2])
                        pmbmd->m_rgTranspStatusBBM[4] = ALL;
        }
        if (pmbmd->m_bMerged[1] | pmbmd->m_bMerged[2]) goto MERGE;
        else {
                if ((pmbmd->m_rgTranspStatus[1] & pmbmd->m_rgTranspStatus[3]) == PARTIAL) {
                        pmbmd->m_bMerged[3] = checkMergedStatus (7, 0, 7, 8);
                        if (pmbmd->m_bMerged[3])
                                pmbmd->m_rgTranspStatusBBM[3] = ALL;
                }
                if ((pmbmd->m_rgTranspStatus[2] & pmbmd->m_rgTranspStatus[4]) == PARTIAL) {
                        pmbmd->m_bMerged[4] = checkMergedStatus (15, 8, 7, 8);
                        if (pmbmd->m_bMerged[4])
                                pmbmd->m_rgTranspStatusBBM[4] = ALL;
                }
                if (pmbmd->m_bMerged[3] | pmbmd->m_bMerged[4]) goto MERGE;
                else {
                        if ((pmbmd->m_rgTranspStatus[1] & pmbmd->m_rgTranspStatus[4]) == PARTIAL) {
                                pmbmd->m_bMerged[5] = checkMergedStatus (7, 8, 7, 8);
                                if (pmbmd->m_bMerged[5])
                                        pmbmd->m_rgTranspStatusBBM[4] = ALL;
                        }
                        if ((pmbmd->m_rgTranspStatus[2] & pmbmd->m_rgTranspStatus[3]) == PARTIAL) {
                                pmbmd->m_bMerged[6] = checkMergedStatus (15, 0, 7, 8);
                                if (pmbmd->m_bMerged[6])
                                        pmbmd->m_rgTranspStatusBBM[3] = ALL;
                        }
                }
        }
MERGE:
        for (UInt x=1; x<7; x++)
                pmbmd->m_bMerged[0] |= pmbmd->m_bMerged[x];
        if (pmbmd->m_bMerged[0])
                swapTransparentModes (pmbmd, BBM);
}

Void CVideoObject::isBoundaryMacroBlockMerged (CMBMode* pmbmd, PixelC* ppxlcBY)
{
        if ((pmbmd->m_rgTranspStatus[1] & pmbmd->m_rgTranspStatus[2]) == PARTIAL) {
                pmbmd->m_bMerged[1] = checkMergedStatus (7, 8, 7, 0, ppxlcBY);
                if (pmbmd->m_bMerged[1])
                        pmbmd->m_rgTranspStatusBBM[2] = ALL;
        }
        if ((pmbmd->m_rgTranspStatus[3] & pmbmd->m_rgTranspStatus[4]) == PARTIAL) {
                pmbmd->m_bMerged[2] = checkMergedStatus (7, 8, 15, 8, ppxlcBY);
                if (pmbmd->m_bMerged[2])
                        pmbmd->m_rgTranspStatusBBM[4] = ALL;
        }
        if (pmbmd->m_bMerged[1] | pmbmd->m_bMerged[2]) goto MERGE;
        else {
                if ((pmbmd->m_rgTranspStatus[1] & pmbmd->m_rgTranspStatus[3]) == PARTIAL) {
                        pmbmd->m_bMerged[3] = checkMergedStatus (7, 0, 7, 8, ppxlcBY);
                        if (pmbmd->m_bMerged[3])
                                pmbmd->m_rgTranspStatusBBM[3] = ALL;
                }
                if ((pmbmd->m_rgTranspStatus[2] & pmbmd->m_rgTranspStatus[4]) == PARTIAL) {
                        pmbmd->m_bMerged[4] = checkMergedStatus (15, 8, 7, 8, ppxlcBY);
                        if (pmbmd->m_bMerged[4])
                                pmbmd->m_rgTranspStatusBBM[4] = ALL;
                }
                if (pmbmd->m_bMerged[3] | pmbmd->m_bMerged[4]) goto MERGE;
                else {
                        if ((pmbmd->m_rgTranspStatus[1] & pmbmd->m_rgTranspStatus[4]) == PARTIAL) {
                                pmbmd->m_bMerged[5] = checkMergedStatus (7, 8, 7, 8, ppxlcBY);
                                if (pmbmd->m_bMerged[5])
                                        pmbmd->m_rgTranspStatusBBM[4] = ALL;
                        }
                        if ((pmbmd->m_rgTranspStatus[2] & pmbmd->m_rgTranspStatus[3]) == PARTIAL) {
                                pmbmd->m_bMerged[6] = checkMergedStatus (15, 0, 7, 8, ppxlcBY);
                                if (pmbmd->m_bMerged[6])
                                        pmbmd->m_rgTranspStatusBBM[3] = ALL;
                        }
                }
        }
MERGE: 
        for (UInt x=1; x<7; x++)
                pmbmd->m_bMerged[0] |= pmbmd->m_bMerged[x];
        if (pmbmd->m_bMerged[0])
                swapTransparentModes (pmbmd, BBM);
}

Void CVideoObject::overlayBlocks (UInt x1, UInt x2, UInt y1, UInt y2, DCTMode dctMd)
{
        UInt    pos1 = y1*MB_SIZE+x1,
                pos2 = y2*MB_SIZE+x2;
        PixelC  *SB1 = m_ppxlcCurrMBBY+pos1,
                *SB2 = m_ppxlcCurrMBBY+pos2;
 
        if (dctMd == INTRA || dctMd == INTRAQ) {
                overlayBlocks (SB1, SB2, m_ppxlcCurrMBY+pos1, m_ppxlcCurrMBY+pos2);
                if (m_volmd.fAUsage == EIGHT_BIT)
                        overlayBlocks (SB1, SB2, m_ppxlcCurrMBA+pos1, m_ppxlcCurrMBA+pos2);
        }
        else {
                overlayBlocks (SB2, m_ppxliErrorMBY+pos1, m_ppxliErrorMBY+pos2);
                if (m_volmd.fAUsage == EIGHT_BIT)
                        overlayBlocks (SB2, m_ppxliErrorMBA+pos1, m_ppxliErrorMBA+pos2);
        }
}

Void CVideoObject::overlayBlocks (PixelC* SB1, PixelC* SB2, PixelC* ppxlcB1, PixelC* ppxlcB2)
{
        UInt    x, y;
        PixelC  *ppxlcS1, *ppxlcS2;
 
        for (y = 0, ppxlcS1 = SB1, ppxlcS2 = SB2; y < BLOCK_SIZE; y++) {
                for (x = 0; x < BLOCK_SIZE; x++, ppxlcS1--, ppxlcS2++, ppxlcB1--, ppxlcB2++)
                        if (*ppxlcS2)
                                *ppxlcB1 = *ppxlcB2;
                        else if (!*ppxlcS1) {
                                *ppxlcB1 += *ppxlcB2;
                                *ppxlcB1 >>= 1;
                        }
                ppxlcS1 -= BLOCK_SIZE;
                ppxlcS2 += BLOCK_SIZE;
                ppxlcB1 -= BLOCK_SIZE;
                ppxlcB2 += BLOCK_SIZE;
        }
}
 
Void CVideoObject::overlayBlocks (PixelC* SB2, PixelI* ppxlcB1, PixelI* ppxlcB2)
{
        UInt    x, y;
        PixelC  *ppxlcS2;
 
        for (y = 0, ppxlcS2 = SB2; y < BLOCK_SIZE; y++) {
                for (x = 0; x < BLOCK_SIZE; x++, ppxlcS2++, ppxlcB1--, ppxlcB2++)
                        if (*ppxlcS2)
                                *ppxlcB1 = *ppxlcB2;
                ppxlcS2 += BLOCK_SIZE;
                ppxlcB1 -= BLOCK_SIZE;
                ppxlcB2 += BLOCK_SIZE;
        }
}

Bool CVideoObject::checkMergedStatus (UInt x1, UInt x2, UInt y1, UInt y2)
{
        PixelC  *ppxlcS1 = m_ppxlcCurrMBBY + y1 * MB_SIZE + x1,
                *ppxlcS2 = m_ppxlcCurrMBBY + y2 * MB_SIZE + x2;
 
        for (UInt y = 0; y < BLOCK_SIZE; y++) {
                for (UInt x = 0; x < BLOCK_SIZE; x++, ppxlcS1--, ppxlcS2++)
                        if (*ppxlcS1 && *ppxlcS2)
                                return FALSE;
                ppxlcS1 -= BLOCK_SIZE;
                ppxlcS2 += BLOCK_SIZE;
        }
        return TRUE;
}
 
Bool CVideoObject::checkMergedStatus (UInt x1, UInt x2, UInt y1, UInt y2, PixelC* ppxlcBY)
{
        PixelC  *ppxlcS1 = ppxlcBY + y1 * MB_SIZE + x1,
                *ppxlcS2 = ppxlcBY + y2 * MB_SIZE + x2;
 
        for (UInt y = 0; y < BLOCK_SIZE; y++) {
                for (UInt x = 0; x < BLOCK_SIZE; x++, ppxlcS1--, ppxlcS2++)
                        if (*ppxlcS1 && *ppxlcS2)
                                return FALSE;
                ppxlcS1 -= BLOCK_SIZE;
                ppxlcS2 += BLOCK_SIZE;
        }
        return TRUE;
}

Void CVideoObject::mergedMacroBlockSplit (CMBMode* pmbmd, PixelC* ppxlcRefMBY, PixelC* ppxlcRefMBA)
{
        Bool    isIntra = (pmbmd->m_dctMd==INTRA || pmbmd->m_dctMd==INTRAQ) ? TRUE : FALSE;
 
        swapTransparentModes (pmbmd, BBS);
        if (pmbmd->m_bMerged[1])
                if (isIntra)
                        splitTwoMergedBlocks (7, 8, 7, 0, ppxlcRefMBY, ppxlcRefMBA);
                else    splitTwoMergedBlocks (7, 8, 7, 0, m_ppxliErrorMBY, m_ppxliErrorMBA);
        if (pmbmd->m_bMerged[2])
                if (isIntra)
                        splitTwoMergedBlocks (7, 8, 15, 8, ppxlcRefMBY, ppxlcRefMBA);
                else    splitTwoMergedBlocks (7, 8, 15, 8, m_ppxliErrorMBY, m_ppxliErrorMBA);
        if (pmbmd->m_bMerged[1] || pmbmd->m_bMerged[2]) goto SPLIT;
        else {
                if (pmbmd->m_bMerged[3])
                        if (isIntra)
                                splitTwoMergedBlocks (7, 0, 7, 8, ppxlcRefMBY, ppxlcRefMBA);
                        else    splitTwoMergedBlocks (7, 0, 7, 8, m_ppxliErrorMBY, m_ppxliErrorMBA);
                if (pmbmd->m_bMerged[4])
                        if (isIntra)
                                splitTwoMergedBlocks (15, 8, 7, 8, ppxlcRefMBY, ppxlcRefMBA);
                        else    splitTwoMergedBlocks (15, 8, 7, 8, m_ppxliErrorMBY, m_ppxliErrorMBA);
                if (pmbmd->m_bMerged[3] || pmbmd->m_bMerged[4]) goto SPLIT;
                else {
                        if (pmbmd->m_bMerged[5])
                                if (isIntra)
                                        splitTwoMergedBlocks (7, 8, 7, 8, ppxlcRefMBY, ppxlcRefMBA);
                                else    splitTwoMergedBlocks (7, 8, 7, 8, m_ppxliErrorMBY, m_ppxliErrorMBA);
                        if (pmbmd->m_bMerged[6])
                                if (isIntra)
                                        splitTwoMergedBlocks(15, 0, 7, 8, ppxlcRefMBY, ppxlcRefMBA);
                                else    splitTwoMergedBlocks (15, 0, 7, 8, m_ppxliErrorMBY, m_ppxliErrorMBA);
                }
        }
SPLIT:
        return;
}

Void CVideoObject::splitTwoMergedBlocks (UInt x1, UInt x2, UInt y1, UInt y2, PixelC* ppxlcIn1, PixelC* ppxlcIn2)
{
        UInt    SKIP = m_iFrameWidthY - BLOCK_SIZE,
                pos1 = y1 * m_iFrameWidthY + x1,
                pos2 = y2 * m_iFrameWidthY + x2,
                pos3 = y2 * MB_SIZE + x2;
        PixelC  *ppxlcS2, *ppxlcB1, *ppxlcB2;
 
        ppxlcB1 = ppxlcIn1 + pos1;
        ppxlcB2 = ppxlcIn1 + pos2;
        ppxlcS2 = m_ppxlcCurrMBBY + pos3;
        for (UInt y = 0; y < BLOCK_SIZE; y++) {
                for (UInt x = 0; x < BLOCK_SIZE; x++, ppxlcS2++, ppxlcB1--, ppxlcB2++)
                        if (*ppxlcS2)
                                *ppxlcB2 = *ppxlcB1;
                ppxlcS2 += BLOCK_SIZE;
                ppxlcB1 -= SKIP;
                ppxlcB2 += SKIP;
        }
        if (m_volmd.fAUsage == EIGHT_BIT) {
                ppxlcB1 = ppxlcIn2 + pos1;
                ppxlcB2 = ppxlcIn2 + pos2;
                ppxlcS2 = m_ppxlcCurrMBBY + pos3;
                for (UInt y = 0; y < BLOCK_SIZE; y++) {
                        for (UInt x = 0; x < BLOCK_SIZE; x++, ppxlcS2++, ppxlcB1--, ppxlcB2++)
                                if (*ppxlcS2)
                                        *ppxlcB2 = *ppxlcB1;
                        ppxlcS2 += BLOCK_SIZE;
                        ppxlcB1 -= SKIP;
                        ppxlcB2 += SKIP;
                }
        }
}

Void CVideoObject::splitTwoMergedBlocks (UInt x1, UInt x2, UInt y1, UInt y2, PixelI* ppxlcIn1, PixelI* ppxlcIn2)
{
        UInt    pos1 = y1 * MB_SIZE + x1,
                pos2 = y2 * MB_SIZE + x2;
        PixelC  *ppxlcS2;
        PixelI  *ppxlcB1, *ppxlcB2;
 
        ppxlcB1 = ppxlcIn1 + pos1;
        ppxlcB2 = ppxlcIn1 + pos2;
        ppxlcS2 = m_ppxlcCurrMBBY + pos2;
        for (UInt y = 0; y < BLOCK_SIZE; y++) {
                for (UInt x = 0; x < BLOCK_SIZE; x++, ppxlcS2++, ppxlcB1--, ppxlcB2++)
                        if (*ppxlcS2)
                                *ppxlcB2 = *ppxlcB1;
                ppxlcS2 += BLOCK_SIZE;
                ppxlcB1 -= BLOCK_SIZE;
                ppxlcB2 += BLOCK_SIZE;
        }
        if (m_volmd.fAUsage == EIGHT_BIT) {
                ppxlcB1 = ppxlcIn2 + pos1;
                ppxlcB2 = ppxlcIn2 + pos2;
                ppxlcS2 = m_ppxlcCurrMBBY + pos2;
                for (UInt y = 0; y < BLOCK_SIZE; y++) {
                        for (UInt x = 0; x < BLOCK_SIZE; x++, ppxlcS2++, ppxlcB1--, ppxlcB2++)
                                if (*ppxlcS2)
                                        *ppxlcB2 = *ppxlcB1;
                        ppxlcS2 += BLOCK_SIZE;
                        ppxlcB1 -= BLOCK_SIZE;
                        ppxlcB2 += BLOCK_SIZE;
                }
        }
}
 
Void CVideoObject::swapTransparentModes (CMBMode* pmbmd, Bool mode) {
        TransparentStatus rgTranspStatusTmp[5];
        memcpy (rgTranspStatusTmp, pmbmd->m_rgTranspStatusBBM, 5*sizeof(TransparentStatus));
        memcpy (pmbmd->m_rgTranspStatusBBM, pmbmd->m_rgTranspStatus, 5*sizeof(TransparentStatus));
        memcpy (pmbmd->m_rgTranspStatus, rgTranspStatusTmp, 5*sizeof(TransparentStatus));
}
 
Void CVideoObject::setMergedTransparentModes (CMBMode* pmbmd) {
        memcpy (pmbmd->m_rgTranspStatusBBM, pmbmd->m_rgTranspStatus, 11*sizeof(TransparentStatus));
}
 
Void CVideoObject::initMergedMode (CMBMode* pmbmd) {
        setMergedTransparentModes (pmbmd);
        memset (pmbmd->m_bMerged, FALSE, 7*sizeof (Bool));
}

// End of Hyundai(1998-5-9)*/

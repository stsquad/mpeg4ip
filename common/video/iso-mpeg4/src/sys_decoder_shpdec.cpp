/*************************************************************************

This software module was originally developed by 

	Ming-Chieh Lee (mingcl@microsoft.com), Microsoft Corporation
	Wei-ge Chen (wchen@microsoft.com), Microsoft Corporation
	Bruce Lin (blin@microsoft.com), Microsoft Corporation
	Chuang Gu (chuanggu@microsoft.com), Microsoft Corporation
	(date: July, 1997)

and edited by
	Yoshihiro Kikuchi (TOSHIBA CORPORATION)
	Takeshi Nagai (TOSHIBA CORPORATION)
	Toshiaki Watanabe (TOSHIBA CORPORATION)
	Noboru Yamaguchi (TOSHIBA CORPORATION)
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

	shpdec.cpp

Abstract:

	Block decoding functions

Revision History:

*************************************************************************/

#include <stdlib.h>
#include <math.h>
#include "typeapi.h"
#include "codehead.h"
#include "mode.hpp"
#include "global.hpp"
#include "bitstrm.hpp"
#include "entropy.hpp"
#include "huffman.hpp"
#include "dct.hpp"
#include "cae.h"
#include "vopses.hpp"
#include "vopsedec.hpp"

#ifdef __MFC_
#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

#define new DEBUG_NEW				   
#endif // __MFC_

#define SIGN(n)	(n>0)?1:((n<0)?-1:0)				//OBSS_SAIT_991015

// HHI Klaas Schueuer: sadct
Void CVideoObjectDecoder::deriveSADCTRowLengths(Int **piCoeffWidths, const PixelC* ppxlcMBBY, const PixelC* ppxlcCurrMBBUV, 
												const TransparentStatus *pTransparentStatus)
{
	CInvSADCT *pidct = (CInvSADCT *)m_pidct;
	const PixelC *pSrc = NULL;
	int i;
	for (i=Y_BLOCK1; i<U_BLOCK; i++) {
		if (pTransparentStatus[i] != PARTIAL) {
        	memset(piCoeffWidths[i], 0, BLOCK_SIZE * sizeof(piCoeffWidths[i][0]));
			continue;
		}
		switch (i) {
		case Y_BLOCK1: 
			pSrc = ppxlcMBBY;
			break;
		case Y_BLOCK2:	
			pSrc = ppxlcMBBY + BLOCK_SIZE;
			break;
		case Y_BLOCK3:
			pSrc = ppxlcMBBY + BLOCK_SIZE*MB_SIZE;
			break;
		case Y_BLOCK4:
			pSrc = ppxlcMBBY + BLOCK_SIZE*MB_SIZE + BLOCK_SIZE;
			break;
		}

		pidct->getRowLength(piCoeffWidths[i], pSrc, MB_SIZE);
#ifdef _DEBUG
		for (int iy=0; iy<BLOCK_SIZE; iy++) {
        	int l = *(piCoeffWidths[i] + iy);
        	assert(l>=0 && l<=BLOCK_SIZE); 
		}
#endif		
	}
    if (pTransparentStatus[U_BLOCK] == PARTIAL) {
 		pidct->getRowLength(piCoeffWidths[U_BLOCK], ppxlcCurrMBBUV, BLOCK_SIZE);    
#ifdef _DEBUG
		for (int iy=0; iy<BLOCK_SIZE; iy++) {
        	int l = *(piCoeffWidths[i] + iy);
        	assert(l>=0 && l<=BLOCK_SIZE); 
		}
#endif	 
    }
	else
    	memset(piCoeffWidths[U_BLOCK], 0, BLOCK_SIZE * sizeof(piCoeffWidths[i][0]));
      
#ifdef __TRACE_DECODING_
	m_ptrcer->dumpSADCTRowLengths(piCoeffWidths);
#endif
}
// end 

Void CVideoObjectDecoder::decodeIntraShape (CMBMode* pmbmd, Int iMBX, Int iMBY, PixelC* ppxlcCurrMBBY, PixelC* ppxlcCurrMBBYFrm)
{
	static Int rgiShapeMdCode [3];
	Int iMBnum = VPMBnum(iMBX, iMBY);
	m_bVPNoLeft = bVPNoLeft(iMBnum, iMBX);
	m_bVPNoTop = bVPNoTop(iMBnum);
	m_bVPNoRightTop = bVPNoRightTop(iMBnum, iMBX);
	m_bVPNoLeftTop = bVPNoLeftTop(iMBnum, iMBX);

	//printf("(%d,%d)",iMBX,iMBY);
	Int iTblIndex = shpMdTableIndex (pmbmd, iMBX, iMBY);
	rgiShapeMdCode [ALL_TRANSP] = grgchFirstShpCd [iTblIndex + ALL_TRANSP];
	rgiShapeMdCode [ALL_OPAQUE] = grgchFirstShpCd [iTblIndex + ALL_OPAQUE];
	rgiShapeMdCode [INTRA_CAE]  = grgchFirstShpCd [iTblIndex + INTRA_CAE];
	//	Modified for error resilient mode by Toshiba(1997-11-14)
	//Int iBits = m_pbitstrmIn->peekBits (2);
	//iBits = (iBits > 1) ? m_pbitstrmIn->getBits (2) : m_pbitstrmIn->getBits (1);
	Int iBits = m_pbitstrmIn->peekBits (3);
	assert (iBits != 0);
	if (iBits >= 4) {
		iBits = m_pbitstrmIn->getBits (1);
		iBits = 0;
	}
	else if (iBits >= 2) {
		iBits = m_pbitstrmIn->getBits (2);
		iBits = 2;
	}
	else if (iBits == 1) {
		iBits = m_pbitstrmIn->getBits (3);
		iBits = 3;
	}
	//	End Toshiba(1997-11-14)
	assert (iBits == 0 || iBits == 2 || iBits == 3);
	//printf("[%08x]",m_pbitstrmIn->peekBits(32));
	if (iBits == rgiShapeMdCode [ALL_TRANSP])	{
		pmbmd->m_shpmd = ALL_TRANSP;//printf("_");
		pxlcmemset (ppxlcCurrMBBY, MPEG4_TRANSPARENT, MB_SQUARE_SIZE);
		copyReconShapeToMbAndRef (ppxlcCurrMBBY, ppxlcCurrMBBYFrm, MPEG4_TRANSPARENT);
		pmbmd->m_rgTranspStatus [0] = pmbmd->m_rgTranspStatus [1] = pmbmd->m_rgTranspStatus [2] = pmbmd->m_rgTranspStatus [3] =
		pmbmd->m_rgTranspStatus [4] = pmbmd->m_rgTranspStatus [5] = pmbmd->m_rgTranspStatus [6] = ALL;
	}
	else if (iBits == rgiShapeMdCode [ALL_OPAQUE])	{
		pmbmd->m_shpmd = ALL_OPAQUE;//printf("X");
		copyReconShapeToMbAndRef (ppxlcCurrMBBY, ppxlcCurrMBBYFrm, MPEG4_OPAQUE);
		pmbmd->m_rgTranspStatus [0] = pmbmd->m_rgTranspStatus [1] = pmbmd->m_rgTranspStatus [2] = pmbmd->m_rgTranspStatus [3] =
		pmbmd->m_rgTranspStatus [4] = pmbmd->m_rgTranspStatus [5] = pmbmd->m_rgTranspStatus [6] = NONE;
	}
	else if (iBits == rgiShapeMdCode [INTRA_CAE])	{	
		pmbmd->m_shpmd = INTRA_CAE;//printf("S");
		decodeIntraCaeBAB (ppxlcCurrMBBY, ppxlcCurrMBBYFrm);
		decideTransparencyStatus (pmbmd, ppxlcCurrMBBY);
		assert (pmbmd->m_rgTranspStatus [0] == PARTIAL);
	}
	else
		assert (FALSE);
}

Void CVideoObjectDecoder::decodeInterShape (CVOPU8YUVBA* pvopcRefQ, CMBMode* pmbmd, CoordI iMBX, CoordI iMBY, CoordI iX, CoordI iY, CMotionVector* pmv, CMotionVector* pmvBY, PixelC* ppxlcMBBY, PixelC* ppxlcMBBYFrm, const ShapeMode& shpmdColocatedMB)
{
	assert (shpmdColocatedMB != UNKNOWN);
	//	Added for error resilient mode by Toshiba(1997-11-14)
	//printf("m_iVPMBnum=%d\n",m_iVPMBnum);
	Int iMBnum = VPMBnum(iMBX, iMBY);
	m_bVPNoLeft = bVPNoLeft(iMBnum, iMBX);
	m_bVPNoTop = bVPNoTop(iMBnum);
	m_bVPNoRightTop = bVPNoRightTop(iMBnum, iMBX);
	m_bVPNoLeftTop = bVPNoLeftTop(iMBnum, iMBX);

	//printf("(%d,%d)",iMBX,iMBY);
	CEntropyDecoder* pentrdec = m_pentrdecSet->m_ppentrdecShapeMode [shpmdColocatedMB];
	pmbmd->m_shpmd = (ShapeMode) pentrdec->decodeSymbol ();
	//printf("[%08x]",m_pbitstrmIn->peekBits(32));
	if (pmbmd->m_shpmd == ALL_TRANSP)	{//printf("_");
		copyReconShapeToMbAndRef (ppxlcMBBY, ppxlcMBBYFrm, MPEG4_TRANSPARENT);
//OBSS_SAIT_991015 //for OBSS
		pmvBY->iMVX = NOT_MV;
		pmvBY->iMVY = NOT_MV;
//~OBSS_SAIT_991015
		pmbmd->m_rgTranspStatus [0] = pmbmd->m_rgTranspStatus [1] = pmbmd->m_rgTranspStatus [2] = pmbmd->m_rgTranspStatus [3] =
		pmbmd->m_rgTranspStatus [4] = pmbmd->m_rgTranspStatus [5] = pmbmd->m_rgTranspStatus [6] = ALL;
	}
	else if (pmbmd->m_shpmd == ALL_OPAQUE)	{//printf("X");
		copyReconShapeToMbAndRef (ppxlcMBBY, ppxlcMBBYFrm, MPEG4_OPAQUE);
//OBSS_SAIT_991015 //for OBSS
		pmvBY->iMVX = NOT_MV;
		pmvBY->iMVY = NOT_MV;
//~OBSS_SAIT_991015
		pmbmd->m_rgTranspStatus [0] = pmbmd->m_rgTranspStatus [1] = pmbmd->m_rgTranspStatus [2] = pmbmd->m_rgTranspStatus [3] =
		pmbmd->m_rgTranspStatus [4] = pmbmd->m_rgTranspStatus [5] = pmbmd->m_rgTranspStatus [6] = NONE;
	}
	else if (pmbmd->m_shpmd == INTRA_CAE)	{//printf("S");
		decodeIntraCaeBAB (ppxlcMBBY, ppxlcMBBYFrm);
//OBSS_SAIT_991015 //for OBSS
		pmvBY->iMVX = NOT_MV;
		pmvBY->iMVY = NOT_MV;
//~OBSS_SAIT_991015
		decideTransparencyStatus (pmbmd, ppxlcMBBY);
		assert (pmbmd->m_rgTranspStatus [0] == PARTIAL || pmbmd->m_rgTranspStatus [0] == NONE);
	}
	else {//printf("M");
		CMotionVector mvShapeDiff (0, 0);
		if (pmbmd->m_shpmd == MVDNZ_NOUPDT || pmbmd->m_shpmd == INTER_CAE_MVDNZ)	
			decodeMVDS (mvShapeDiff);
		CMotionVector mvShapeMVP = findShapeMVP (pmv, pmvBY, pmbmd, iMBX, iMBY);
		*pmvBY = mvShapeMVP + mvShapeDiff;
		//printf("(%d,%d)",pmvBY->iMVX*2,pmvBY->iMVY*2);
		//motion comp
//OBSSFIX_MODE3
		if(m_volmd.volType == ENHN_LAYER && m_vopmd.vopPredType == PVOP &&m_volmd.bSpatialScalability == 1 && m_volmd.iHierarchyType == 0 && m_volmd.iEnhnType == 1 && m_volmd.iuseRefShape == 1)
			memset((void *)m_puciPredBAB->pixels(),255, MC_BAB_SIZE*MC_BAB_SIZE);
		else
			motionCompBY ((PixelC*) m_puciPredBAB->pixels (),
						  (PixelC*) pvopcRefQ->getPlane (BY_PLANE)->pixels (),
						  pmvBY->iMVX + iX - 1,		
						  pmvBY->iMVY + iY - 1);	//-1 due to 18x18 motion comp
//		motionCompBY ((PixelC*) m_puciPredBAB->pixels (),
//					  (PixelC*) pvopcRefQ->getPlane (BY_PLANE)->pixels (),
//					  pmvBY->iMVX + iX - 1,		
//					  pmvBY->iMVY + iY - 1);	//-1 due to 18x18 motion comp
//~OBSSFIX_MODE3	
		if (pmbmd->m_shpmd == INTER_CAE_MVDZ || pmbmd->m_shpmd == INTER_CAE_MVDNZ)
		{
			m_iInverseCR = 1;
			m_iWidthCurrBAB = BAB_SIZE;
			const PixelC *ppxlcPredBAB = m_puciPredBAB->pixels ();
			if(!m_volmd.bNoCrChange && m_pbitstrmIn->getBits (1) != 0)
			{
				if(m_pbitstrmIn->getBits (1) == 0)
				{
					m_iInverseCR = 2;
					m_iWidthCurrBAB = 12;
					downSampleShapeMCPred(ppxlcPredBAB,m_ppxlcPredBABDown2,2);
					ppxlcPredBAB = m_ppxlcPredBABDown2;
					subsampleLeftTopBorderFromVOP (ppxlcMBBYFrm, m_ppxlcCurrMBBYDown2);
					m_rgpxlcCaeSymbol = m_ppxlcCurrMBBYDown2;
				}
				else
				{
					m_iInverseCR = 4;
					m_iWidthCurrBAB = 8;
					downSampleShapeMCPred(ppxlcPredBAB,m_ppxlcPredBABDown4,4);
					ppxlcPredBAB = m_ppxlcPredBABDown4;
					subsampleLeftTopBorderFromVOP (ppxlcMBBYFrm, m_ppxlcCurrMBBYDown4);
					m_rgpxlcCaeSymbol = m_ppxlcCurrMBBYDown4;
				}	
			}
			else
			{
				copyLeftTopBorderFromVOP (ppxlcMBBYFrm, m_ppxlcReconCurrBAB);  //used for upsample and CAE
				m_rgpxlcCaeSymbol = m_ppxlcReconCurrBAB;		//assign encoding buffer
			}

			if (m_pbitstrmIn->getBits (1) == 1)
				decodeInterCAEH (ppxlcPredBAB);			//right bottom border made on the fly
			else
				decodeInterCAEV (ppxlcPredBAB); 

			if(m_iInverseCR>1)
				upSampleShape(ppxlcMBBYFrm,m_rgpxlcCaeSymbol,m_ppxlcReconCurrBAB);
			copyReconShapeToMbAndRef (ppxlcMBBY, ppxlcMBBYFrm, m_ppxlcReconCurrBAB, TOTAL_BAB_SIZE, BAB_BORDER);	//copy rounded data
		}
		else {
			copyReconShapeToMbAndRef (ppxlcMBBY, ppxlcMBBYFrm, m_puciPredBAB->pixels (), MC_BAB_SIZE, MC_BAB_BORDER);
		}
		
		decideTransparencyStatus (pmbmd, ppxlcMBBY);
		assert (pmbmd->m_rgTranspStatus [0] != ALL);
	}//putchar('\n');
}

//OBSS_SAIT_991015
//for OBSS shape decoding -PVOP
Void CVideoObjectDecoder::decodeSIShapePVOP (CVOPU8YUVBA* pvopcRefQ, CMBMode* pmbmd, CoordI iMBX, CoordI iMBY, CoordI iX, CoordI iY, CMotionVector* pmv, CMotionVector* pmvBY, CMotionVector* pmvBaseBY, PixelC* ppxlcMBBY, PixelC* ppxlcMBBYFrm, const ShapeMode& shpmdColocatedMB)
{
	assert (shpmdColocatedMB != UNKNOWN);
	//	Added for error resilient mode by Toshiba(1997-11-14)
	Int iMBnum = VPMBnum(iMBX, iMBY);
	m_bVPNoLeft = bVPNoLeft(iMBnum, iMBX);
	m_bVPNoTop = bVPNoTop(iMBnum);
	m_bVPNoRightTop = bVPNoRightTop(iMBnum, iMBX);
	m_bVPNoLeftTop = bVPNoLeftTop(iMBnum, iMBX);
	m_bVPNoRight = bVPNoRight(iMBX);
	m_bVPNoBottom = bVPNoBottom(iMBY);

	assert ( m_vopmd.vopPredType==PVOP );

	if((m_volmd.iuseRefShape ==1) && !m_volmd.bShapeOnly) {
		motionCompBY ((PixelC*) m_puciPredBAB->pixels (),
					  (PixelC*) pvopcRefQ->getPlane(BY_PLANE)->pixels (),
					  iX-1, iY-1);	

		copyReconShapeToMbAndRef (ppxlcMBBY, ppxlcMBBYFrm, m_puciPredBAB->pixels (), MC_BAB_SIZE, MC_BAB_BORDER);
		decideTransparencyStatus (pmbmd, ppxlcMBBY);
		return;
	}

	CEntropyDecoder* pentrdec = m_pentrdecSet->m_ppentrdecShapeSSModeIntra [0];
	pmbmd->m_shpssmd = (ShapeSSMode) pentrdec->decodeSymbol ();
	
	if (pmbmd->m_shpssmd == INTRA_NOT_CODED)	{//printf("S");

		motionCompBY ((PixelC*) m_puciPredBAB->pixels (),
					  (PixelC*) pvopcRefQ->getPlane(BY_PLANE)->pixels (),
					  iX-1, iY-1);	

		copyReconShapeToMbAndRef (ppxlcMBBY, ppxlcMBBYFrm, m_puciPredBAB->pixels (), MC_BAB_SIZE, MC_BAB_BORDER);

		decideTransparencyStatus (pmbmd, ppxlcMBBY);
	}
	else if (pmbmd->m_shpssmd == INTRA_CODED)	{
		motionCompBY ((PixelC*) m_puciPredBAB->pixels (),
					  (PixelC*) pvopcRefQ->getPlane (BY_PLANE)->pixels (),
					  iX-1, iY-1);	
	
		copyReconShapeToMbAndRef (ppxlcMBBY, ppxlcMBBYFrm, m_puciPredBAB->pixels (), MC_BAB_SIZE, MC_BAB_BORDER);
		
		decodeSIBAB (	ppxlcMBBY, ppxlcMBBYFrm, 
						(pvopcRefQ->getPlane (BY_PLANE)->m_pbHorSamplingChk)+iX, 
						(pvopcRefQ->getPlane (BY_PLANE)->m_pbVerSamplingChk)+iY,
						iMBX, iMBY, 
						(PixelC*)((pvopcRefQ->getPlane (BY_PLANE)->pixels ())+(iY + EXPANDY_REF_FRAME) * m_iFrameWidthY + iX + EXPANDY_REF_FRAME));
		decideTransparencyStatus (pmbmd, ppxlcMBBY);
	}
}

//for OBSS shape decoding -BVOP
Void CVideoObjectDecoder::decodeSIShapeBVOP (CVOPU8YUVBA* pvopcRefQ0, //previous VOP
											 CVOPU8YUVBA* pvopcRefQ1, //lower reference layer
											 CMBMode* pmbmd, CoordI iMBX, CoordI iMBY, CoordI iX, CoordI iY, CMotionVector* pmv, CMotionVector* pmvBY, CMotionVector* pmvBaseBY, PixelC* ppxlcMBBY, PixelC* ppxlcMBBYFrm, const ShapeMode& shpmdColocatedMB)
{
	assert (shpmdColocatedMB != UNKNOWN);
	//	Added for error resilient mode by Toshiba(1997-11-14)
	Int iMBnum = VPMBnum(iMBX, iMBY);
	m_bVPNoLeft = bVPNoLeft(iMBnum, iMBX);
	m_bVPNoTop = bVPNoTop(iMBnum);
	m_bVPNoRightTop = bVPNoRightTop(iMBnum, iMBX);
	m_bVPNoLeftTop = bVPNoLeftTop(iMBnum, iMBX);
	m_bVPNoRight = bVPNoRight(iMBX);
	m_bVPNoBottom = bVPNoBottom(iMBY);

	if((m_volmd.iuseRefShape ==1) && !m_volmd.bShapeOnly) {
		motionCompLowerBY ((PixelC*) m_puciPredBAB->pixels (),
					  (PixelC*) pvopcRefQ1->getPlane(BY_PLANE)->pixels (),
					  iX-1, iY-1);	
		copyReconShapeToMbAndRef (ppxlcMBBY, ppxlcMBBYFrm, m_puciPredBAB->pixels (), MC_BAB_SIZE, MC_BAB_BORDER);
		decideTransparencyStatus (pmbmd, ppxlcMBBY);
		return;
	}

	if(m_vopmd.vopPredType==BVOP){
		Int colocatedIndex[7]={0,0,1,3,3,2,2};
		CEntropyDecoder* pentrdec = m_pentrdecSet->m_ppentrdecShapeSSModeInter [colocatedIndex[shpmdColocatedMB]];
		pmbmd->m_shpssmd = (ShapeSSMode) pentrdec->decodeSymbol ();
	}

	if (pmbmd->m_shpssmd == INTRA_NOT_CODED)	{
		motionCompLowerBY ((PixelC*) m_puciPredBAB->pixels (),
					  (PixelC*) pvopcRefQ1->getPlane(BY_PLANE)->pixels (),
					  iX-1, iY-1);	

		copyReconShapeToMbAndRef (ppxlcMBBY, ppxlcMBBYFrm, m_puciPredBAB->pixels (), MC_BAB_SIZE, MC_BAB_BORDER);
		decideTransparencyStatus (pmbmd, ppxlcMBBY);
	}
	else if (pmbmd->m_shpssmd == INTRA_CODED)	{
		motionCompLowerBY ((PixelC*) m_puciPredBAB->pixels (),
					  (PixelC*) pvopcRefQ1->getPlane (BY_PLANE)->pixels (),
					  iX-1, iY-1);	

		copyReconShapeToMbAndRef (ppxlcMBBY, ppxlcMBBYFrm, m_puciPredBAB->pixels (), MC_BAB_SIZE, MC_BAB_BORDER);
		decodeSIBAB (	ppxlcMBBY, ppxlcMBBYFrm, 
						(pvopcRefQ1->getPlane (BY_PLANE)->m_pbHorSamplingChk)+iX, 
						(pvopcRefQ1->getPlane (BY_PLANE)->m_pbVerSamplingChk)+iY,
						iMBX, iMBY,
						(PixelC*)((pvopcRefQ1->getPlane (BY_PLANE)->pixels ())+(iY + EXPANDY_REF_FRAME) * m_iFrameWidthY + iX + EXPANDY_REF_FRAME));
		decideTransparencyStatus (pmbmd, ppxlcMBBY);
	}
	else {
//if lower layer has no MV then current MV is set to (0,0)
		if( pmvBaseBY->iMVX	== NOT_MV && pmvBaseBY->iMVY == NOT_MV) {
			pmvBaseBY->iMVX = 0;
			pmvBaseBY->iMVY = 0;
		}

		motionCompBY ((PixelC*) m_puciPredBAB->pixels (),
					  (PixelC*) pvopcRefQ0->getPlane (BY_PLANE)->pixels (),
					  (((pmvBaseBY->iMVX)*m_volmd.ihor_sampling_factor_n_shape+(SIGN(pmvBaseBY->iMVX))*m_volmd.ihor_sampling_factor_m_shape/2)/m_volmd.ihor_sampling_factor_m_shape) + iX - 1,		
					  (((pmvBaseBY->iMVY)*m_volmd.iver_sampling_factor_n_shape+(SIGN(pmvBaseBY->iMVY))*m_volmd.iver_sampling_factor_m_shape/2)/m_volmd.iver_sampling_factor_m_shape) + iY - 1);	//-1 due to 18x18 motion comp

		if (pmbmd->m_shpssmd == INTER_NOT_CODED)
			copyReconShapeToMbAndRef (ppxlcMBBY, ppxlcMBBYFrm, m_puciPredBAB->pixels (), MC_BAB_SIZE, MC_BAB_BORDER);
		else if (pmbmd->m_shpssmd == INTER_CODED){
			m_iInverseCR = 1;
			m_iWidthCurrBAB = BAB_SIZE;
			const PixelC *ppxlcPredBAB = m_puciPredBAB->pixels ();
//for bordering
			copyLeftTopBorderFromVOP (ppxlcMBBYFrm, m_ppxlcReconCurrBAB);  //used for upsample and CAE
			m_rgpxlcCaeSymbol = m_ppxlcReconCurrBAB;		//assign encoding buffer

			if (m_pbitstrmIn->getBits (1) == 1)	decodeInterCAEH (ppxlcPredBAB);			//right bottom border made on the fly
			else								decodeInterCAEV (ppxlcPredBAB); 

			copyReconShapeToMbAndRef (ppxlcMBBY, ppxlcMBBYFrm, m_ppxlcReconCurrBAB, TOTAL_BAB_SIZE, BAB_BORDER);	//copy rounded data
		}
		decideTransparencyStatus (pmbmd, ppxlcMBBY);
	}
}

//for SI decoding
Void CVideoObjectDecoder::decodeSIBAB (PixelC* ppxlcBYMB, PixelC* ppxlcBYFrm, Bool* HorSamplingChk, Bool* VerSamplingChk, Int iMBX, Int iMBY, PixelC* ppxlcRefSrc)
{
	Int 		q0, si_coding_mode;
	m_iWidthCurrBAB = BAB_SIZE;
	PixelC* ppxlcReconCurrBABTr = NULL;		//for st_order
	Int scan_order = 0;						
	
	copyLeftTopBorderFromVOP (ppxlcBYFrm, m_ppxlcReconCurrBAB);  //used for upsample and CAE
	Int i; Int j;
	PixelC* PredSrc = ppxlcBYMB;
	PixelC* PredDst = m_ppxlcReconCurrBAB+20*2+2;

	double h_factor = log((double)m_volmd.ihor_sampling_factor_n_shape/m_volmd.ihor_sampling_factor_m_shape)/log((double)2);
	int h_factor_int = (int)floor(h_factor+0.000001);
	double v_factor = log((double)m_volmd.iver_sampling_factor_n_shape/m_volmd.iver_sampling_factor_m_shape)/log((double)2);
	int v_factor_int = (int)floor(v_factor+0.000001);

	Int NumTwoPowerLoopX = h_factor_int;
	Bool ResidualLoopX = 0;
	if(h_factor - h_factor_int > 0.000001 ) ResidualLoopX = 1;
	Int NumTwoPowerLoopY = v_factor_int ; 
	Bool ResidualLoopY = 0 ;
	if(v_factor - v_factor_int > 0.000001 )	ResidualLoopY = 1;

	for(j=0;j<16;j++){
		for(i=0;i<16;i++){
			*PredDst =*PredSrc;
			PredDst++;PredSrc++;
		}
		PredDst+=4;
	}

	makeRightBottomBorder (m_ppxlcReconCurrBAB, TOTAL_BAB_SIZE,(PixelC*)(ppxlcRefSrc),m_iFrameWidthY);	
	m_rgpxlcCaeSymbol = m_ppxlcReconCurrBAB;		//assign encoding buffer

	if ( m_volmd.ihor_sampling_factor_n_shape  == 2  && m_volmd.ihor_sampling_factor_m_shape == 1 && 
		m_volmd.iver_sampling_factor_n_shape  == 2  && m_volmd.iver_sampling_factor_m_shape == 1) {			//for st_order	
		scan_order = decideScanOrder((PixelC*) m_puciPredBAB->pixels ());
		/*--  Curr BAB transposing --*/
		if (scan_order){
			ppxlcReconCurrBABTr = new PixelC [BAB_SIZE * BAB_SIZE];

			for(Int j=0; j<BAB_SIZE; j++)
			   for(Int i=0; i<BAB_SIZE; i++)
				ppxlcReconCurrBABTr[j*BAB_SIZE+i]=m_ppxlcReconCurrBAB[i*BAB_SIZE+j];
			m_rgpxlcCaeSymbol = ppxlcReconCurrBABTr;

			Bool *tmp;
			tmp = HorSamplingChk;
			HorSamplingChk = VerSamplingChk;
			VerSamplingChk = tmp;
		}
	}		

	StartArDecoder (m_parcodec, m_pbitstrmIn);
	q0=SI_bab_type_prob[0];
	si_coding_mode=ArDecodeSymbol(q0,m_parcodec,m_pbitstrmIn); //SISC

	if(si_coding_mode==0) {
		if(ResidualLoopX == 1 || h_factor_int>0) VerticalXORdecoding(NumTwoPowerLoopX,NumTwoPowerLoopY,ResidualLoopX,ResidualLoopY,HorSamplingChk,VerSamplingChk); // SISC 
		if(ResidualLoopY == 1 || v_factor_int>0) HorizontalXORdecoding(NumTwoPowerLoopX,NumTwoPowerLoopY,ResidualLoopX,ResidualLoopY,HorSamplingChk,VerSamplingChk); // SISC 
	}
	else {
		if(ResidualLoopX == 1 || h_factor_int>0) VerticalFulldecoding(NumTwoPowerLoopX,NumTwoPowerLoopY,ResidualLoopX,ResidualLoopY,HorSamplingChk,VerSamplingChk); // SISC 
		if(ResidualLoopY == 1 || v_factor_int>0) HorizontalFulldecoding(NumTwoPowerLoopX,NumTwoPowerLoopY,ResidualLoopX,ResidualLoopY,HorSamplingChk,VerSamplingChk); // SISC
	}

	StopArDecoder (m_parcodec, m_pbitstrmIn);
	if ( m_volmd.ihor_sampling_factor_n_shape  == 2  && m_volmd.ihor_sampling_factor_m_shape == 1 && 
		m_volmd.iver_sampling_factor_n_shape  == 2  && m_volmd.iver_sampling_factor_m_shape == 1) {			//for st_order	
		/*--  Curr BAB transposing --*/
		if (scan_order){
			for(j=0; j<BAB_SIZE; j++)
			   for(Int i=0; i<BAB_SIZE; i++)
				m_ppxlcReconCurrBAB[j*BAB_SIZE+i]=ppxlcReconCurrBABTr[i*BAB_SIZE+j];
			delete [] ppxlcReconCurrBABTr;

			Bool *tmp;
			tmp = HorSamplingChk;
			HorSamplingChk = VerSamplingChk;
			VerSamplingChk = tmp;
		}
	}	
	copyReconShapeToMbAndRef (ppxlcBYMB, ppxlcBYFrm, m_ppxlcReconCurrBAB, TOTAL_BAB_SIZE, BAB_BORDER);
}

Void CVideoObjectDecoder::VerticalXORdecoding (Int num_loop_hor, Int num_loop_ver, Bool residual_scanning_hor, Bool residual_scanning_ver, Bool* HorSamplingChk, Bool* VerSamplingChk)
{
  //Int iSizePredBAB = m_iWidthCurrBAB - 2*BAB_BORDER + 2*MC_BAB_BORDER;
  //	Int iSizeMB = m_iWidthCurrBAB - 2*BAB_BORDER;
	PixelC* ppxlcDst  = m_rgpxlcCaeSymbol + m_iWidthCurrBAB * BAB_BORDER + BAB_BORDER;

	{	
	  Int		i, j; //, xor_type=0;
		Int		start_v=0, start_h=0;
		Int		width=20, height=20;
		//		UChar	xor_value=0;
		Int		prev=0, next=0;
		PixelC* smb_data = ppxlcDst;	
		Int skip_upper=0, skip_bottom=0, skip_hor=0, skip_right=0, skip_left=0;     
		Int h_scan_freq=1, v_scan_freq=1;

		i=0;	
		while(*(HorSamplingChk+i) == 0)	i++;
		int tmp = i;

		if (residual_scanning_hor) {
			start_v = 0;
			start_h = 0;
			h_scan_freq=1<<num_loop_hor; 
			v_scan_freq=1<<num_loop_ver;
			skip_hor  = 1<<num_loop_hor;

			if( tmp-(1<<num_loop_hor) >= 0 )				start_h = tmp-(1<<num_loop_hor);
			else if( tmp+(1<<num_loop_hor) <= MB_SIZE-1 )	start_h = tmp+(1<<(num_loop_hor)) ;
			else printf("Out of Sampling Ratio\n");

			i=0;
			while(*(VerSamplingChk+i) == 0)	i++;
			start_v = i;

			for(j=start_h;j<width-4;j+=h_scan_freq){
				if( *(HorSamplingChk+j) == 1) continue;
				skip_upper = start_v+1;
				for(i=start_v;i<height-4;i+=v_scan_freq) {
					if( *(VerSamplingChk+i) == 1) {
						if ( i+v_scan_freq >15) skip_bottom = MB_SIZE+1-i;
						else {
							if( *(VerSamplingChk+i+v_scan_freq) == 0) {
								if (i+v_scan_freq*2 >15) skip_bottom = MB_SIZE+1-i;
								else                     skip_bottom = v_scan_freq*2; 
							}
							else	skip_bottom = v_scan_freq; 
						}
					}
					else	continue;

					if(j-h_scan_freq < -2)	prev=*(smb_data+i*width-2);
					else 					prev=*(smb_data+i*width+j-h_scan_freq);
					if(j+h_scan_freq > 17)	next=*(smb_data+i*width+17);
					else					next=*(smb_data+i*width+j+h_scan_freq);

					if ( j+skip_hor >15)	skip_right = MB_SIZE+1-j;
					else					skip_right = skip_hor;
					if ( j-skip_hor <0)		skip_left = j+2;
					else					skip_left = skip_hor;

					if(prev==next)	*(smb_data+i*width+j)=prev;
					else 			*(smb_data+i*width+j)= (ArDecodeSymbol (enh_intra_v_prob [contextSIVertical(smb_data+i*width+j,skip_right,skip_left,skip_upper,skip_bottom)], m_parcodec, m_pbitstrmIn)) ? MPEG4_OPAQUE : MPEG4_TRANSPARENT;

					if(skip_bottom == v_scan_freq)	skip_upper = v_scan_freq;
					else							skip_upper = v_scan_freq*2;
				}
			}
		}

		i=0;	
		while(*(HorSamplingChk+i) == 0)		i++;
		if(i>start_h && residual_scanning_hor) tmp = start_h;
		else tmp = i;

		while (num_loop_hor >0){
			start_v = 0;
			start_h = 0;
			h_scan_freq=1<<num_loop_hor; 
			v_scan_freq=1<<num_loop_ver;
			skip_hor  = 1<<(num_loop_hor-1);

			if( tmp-(1<<(num_loop_hor-1)) >= 0 ) 	tmp = start_h = tmp-(1<<(num_loop_hor-1));
			else									start_h = tmp+(1<<(num_loop_hor-1)) ;

			i=0;	
			while(*(VerSamplingChk+i) == 0)	i++;
			start_v = i;

			for(j=start_h;j<(width-4);j+=h_scan_freq){
				skip_upper = start_v+1;
				for(i=start_v;i<(height-4);i+=v_scan_freq){
					if( *(VerSamplingChk+i) == 1) {
						if ( i+v_scan_freq >15) skip_bottom = MB_SIZE+1-i;
						else {
							if( *(VerSamplingChk+i+v_scan_freq) == 0) {
								if (i+v_scan_freq*2 >15) skip_bottom = MB_SIZE+1-i;
								else                     skip_bottom = v_scan_freq*2; 
							}
							else	skip_bottom = v_scan_freq; 
						}
					}
					else	continue;

					prev=*(smb_data+i*width+j-(1<<(num_loop_hor-1)));
					if(j==width-1) next=prev;
					else           next=*(smb_data+i*width+j+(1<<(num_loop_hor-1)));

					if(j-(1<<(num_loop_hor-1)) < -2)	prev=*(smb_data+i*width-2);
					else 								prev=*(smb_data+i*width+j-(1<<(num_loop_hor-1)));
					if(j+(1<<(num_loop_hor-1)) > 17)	next=*(smb_data+i*width+17);
					else								next=*(smb_data+i*width+j+(1<<(num_loop_hor-1)));

					if ( j+skip_hor >15)		skip_right = MB_SIZE+1-j;
					else						skip_right = skip_hor;
					if ( j-skip_hor <0)			skip_left = j+2;
					else						skip_left = skip_hor;

					if(prev==next)	*(smb_data+i*width+j)=prev;
					else 			*(smb_data+i*width+j)= (ArDecodeSymbol (enh_intra_v_prob [contextSIVertical(smb_data+i*width+j,skip_right,skip_left,skip_upper,skip_bottom)], m_parcodec, m_pbitstrmIn)) ? MPEG4_OPAQUE : MPEG4_TRANSPARENT;

					if(skip_bottom == v_scan_freq)	skip_upper = v_scan_freq;
					else							skip_upper = v_scan_freq*2;
				}
			}
			num_loop_hor--;
		}
	}
}

Void CVideoObjectDecoder::VerticalFulldecoding (Int num_loop_hor, Int num_loop_ver, Bool residual_scanning_hor, Bool residual_scanning_ver, Bool* HorSamplingChk, Bool* VerSamplingChk)
{
  //	Int iSizePredBAB = m_iWidthCurrBAB - 2*BAB_BORDER + 2*MC_BAB_BORDER;
  //	Int iSizeMB = m_iWidthCurrBAB - 2*BAB_BORDER;
	PixelC* ppxlcDst  = m_rgpxlcCaeSymbol + m_iWidthCurrBAB * BAB_BORDER + BAB_BORDER;

	{	
	  Int		i, j; //, xor_type=0;
		Int		start_v=0, start_h=0;
		Int		width=20, height=20;
		//		UChar	xor_value=0;
		//		Int		prev=0, next=0;
		PixelC* smb_data = ppxlcDst;	
		Int skip_upper=0, skip_bottom=0, skip_hor=0, skip_right=0, skip_left=0;     
		Int h_scan_freq=1, v_scan_freq=1;

		i=0;	
		while(*(HorSamplingChk+i) == 1)	i++;
		while(*(HorSamplingChk+i) == 0)	i++;
		int tmp = i;

		if (residual_scanning_hor) {
			start_v = 0;
			start_h = 0;
			h_scan_freq=1<<num_loop_hor; 
			v_scan_freq=1<<num_loop_ver;
			skip_hor  = 1<<num_loop_hor;

			if( tmp-(1<<num_loop_hor) >= 0 )				start_h = tmp-(1<<num_loop_hor);
			else if( tmp+(1<<num_loop_hor) <= MB_SIZE-1 )	start_h = tmp+(1<<(num_loop_hor)) ;
			else printf("Out of Sampling Ratio\n");

			i=0;
			while(*(VerSamplingChk+i) == 0)	i++;
			start_v = i;

			for(j=start_h;j<width-4;j+=h_scan_freq){
				if( *(HorSamplingChk+j) == 1) continue;
				skip_upper = start_v+1;
				for(i=start_v;i<height-4;i+=v_scan_freq) {
					if( *(VerSamplingChk+i) == 1) {
						if ( i+v_scan_freq >15) skip_bottom = MB_SIZE+1-i;
						else {
							if( *(VerSamplingChk+i+v_scan_freq) == 0) {
								if (i+v_scan_freq*2 >15) skip_bottom = MB_SIZE+1-i;
								else                     skip_bottom = v_scan_freq*2; 
							}
							else	skip_bottom = v_scan_freq; 
						}
					}
					else	continue;

					if ( j+skip_hor >15)		skip_right = MB_SIZE+1-j;
					else						skip_right = skip_hor;
					if ( j-skip_hor <0)			skip_left = j+2;
					else						skip_left = skip_hor;

					*(smb_data+i*width+j)= (ArDecodeSymbol (enh_intra_v_prob [contextSIVertical(smb_data+i*width+j,skip_right,skip_left,skip_upper,skip_bottom)], m_parcodec, m_pbitstrmIn)) ? MPEG4_OPAQUE : MPEG4_TRANSPARENT;
						
					if(skip_bottom == v_scan_freq)	skip_upper = v_scan_freq;
					else							skip_upper = v_scan_freq*2;
				}
			}
		}

		i=0;	
		while(*(HorSamplingChk+i) == 1)	i++;
		while(*(HorSamplingChk+i) == 0)	i++;
		if(i>start_h && residual_scanning_hor) tmp = start_h;
		else tmp = i;

		while (num_loop_hor >0){
			start_v = 0;
			start_h = 0;
			h_scan_freq=1<<num_loop_hor; 
			v_scan_freq=1<<num_loop_ver;
			skip_hor  = 1<<(num_loop_hor-1);

			if( tmp-(1<<(num_loop_hor-1)) >= 0 ) 	tmp = start_h = tmp-(1<<(num_loop_hor-1));
			else									start_h = tmp+(1<<(num_loop_hor-1)) ;

			i=0;	
			while(*(VerSamplingChk+i) == 0)	i++;
			start_v = i;

			for(j=start_h;j<(width-4);j+=h_scan_freq){
				skip_upper = start_v+1;
				for(i=start_v;i<(height-4);i+=v_scan_freq){
					if( *(VerSamplingChk+i) == 1) {
						if ( i+v_scan_freq >15) skip_bottom = MB_SIZE+1-i;
						else {
							if( *(VerSamplingChk+i+v_scan_freq) == 0) {
								if (i+v_scan_freq*2 >15) skip_bottom = MB_SIZE+1-i;
								else                     skip_bottom = v_scan_freq*2; 
							}
							else	skip_bottom = v_scan_freq; 
						}
					}
					else	continue;

					if ( j+skip_hor > 15)	skip_right = MB_SIZE+1-j;
					else						skip_right = skip_hor;
					if ( j-skip_hor <0)			skip_left = j+2;
					else						skip_left = skip_hor;

					*(smb_data+i*width+j)= (ArDecodeSymbol (enh_intra_v_prob [contextSIVertical(smb_data+i*width+j,skip_right, skip_left,skip_upper,skip_bottom)], m_parcodec, m_pbitstrmIn)) ? MPEG4_OPAQUE : MPEG4_TRANSPARENT;

					if(skip_bottom == v_scan_freq)	skip_upper = v_scan_freq;
					else							skip_upper = v_scan_freq*2;
				}
			}
			num_loop_hor--;
		}
	}
}

Void CVideoObjectDecoder::HorizontalXORdecoding (Int num_loop_hor, Int num_loop_ver, Bool residual_scanning_hor, Bool residual_scanning_ver, Bool* HorSamplingChk, Bool* VerSamplingChk)
{
  //	Int iSizePredBAB = m_iWidthCurrBAB - 2*BAB_BORDER + 2*MC_BAB_BORDER;
  //	Int iSizeMB = m_iWidthCurrBAB - 2*BAB_BORDER;
	PixelC* ppxlcDst  = m_rgpxlcCaeSymbol + m_iWidthCurrBAB * BAB_BORDER + BAB_BORDER;

	{	
	  Int		i, j; // , xor_type=0;
		Int		start_v=0, start_h=0;
		Int		width=20, height=20;
		//		UChar	xor_value=0;
		Int		prev=0, next=0;
		PixelC* smb_data = ppxlcDst;	
		Int skip_ver=0, skip_upper=0, skip_bottom=0;
		Int h_scan_freq=1, v_scan_freq=1;

		i=0;	
		while(*(VerSamplingChk+i) == 1)	i++;
		while(*(VerSamplingChk+i) == 0)	i++;
		int tmp = i;

		if ( residual_scanning_ver) {
			start_h = 0;
			start_v = 0;
			v_scan_freq=1<<num_loop_ver; 
			h_scan_freq=1;
			skip_ver = v_scan_freq;

		if( tmp-(1<<num_loop_ver) >= 0 )				start_v = tmp-(1<<num_loop_ver);
		else if( tmp+(1<<num_loop_ver) <= MB_SIZE-1 )	start_v = tmp+(1<<(num_loop_ver)) ;
		else printf("Out of Sampling Ratio\n");

			for(i=start_v;i<(height-4);i+=v_scan_freq){
				if( *(VerSamplingChk+i) == 1) continue;
				for(j=start_h;j<(width-4);j+=h_scan_freq){
					if(i-v_scan_freq < -2)	prev=*(smb_data+(-2)*width+j);
					else 				prev=*(smb_data+(i-v_scan_freq)*width+j);
					if(i+v_scan_freq > 17)	next=*(smb_data+(17)*width+j);
					else				next=*(smb_data+(i+v_scan_freq)*width+j);

					if ( i+skip_ver >15)	skip_bottom = MB_SIZE+1-i;
					else					skip_bottom = skip_ver;
					if ( i-skip_ver <0)		skip_upper = i+2;
					else					skip_upper = skip_ver;

					if(prev==next)  *(smb_data+i*width+j)=prev;
					else			*(smb_data+i*width+j)= (ArDecodeSymbol (enh_intra_h_prob [contextSIHorizontal(smb_data+i*width+j,skip_upper,skip_bottom)], m_parcodec, m_pbitstrmIn)) ? MPEG4_OPAQUE : MPEG4_TRANSPARENT;
				}
			}
		}

		i=0;	
		while(*(VerSamplingChk+i) == 1)	i++;
		while(*(VerSamplingChk+i) == 0)	i++;
		if(i>start_v && residual_scanning_ver) tmp = start_v;
		else tmp = i;

		while (num_loop_ver >0){
			start_h = 0;
			start_v = 0;
			v_scan_freq=1<<num_loop_ver; 
			h_scan_freq=1;
			skip_ver = 1<<(num_loop_ver-1);

			if( tmp-(1<<(num_loop_ver-1)) >= 0 ) 	tmp = start_v = tmp-(1<<(num_loop_ver-1));
			else									start_v = tmp+(1<<(num_loop_ver-1)) ;

			skip_ver = 1<<(num_loop_ver-1);

			for(i=start_v;i<(height-4);i+=v_scan_freq){
				for(j=start_h;j<(width-4);j+=h_scan_freq){
					if(i-skip_ver < -2)	prev=*(smb_data+(-2)*width+j);
					else 					prev=*(smb_data+(i-skip_ver)*width+j);
					if(i+skip_ver > 17)	next=*(smb_data+(17)*width+j);
					else					next=*(smb_data+(i+skip_ver)*width+j);
					if(i - skip_ver < 0)	skip_upper = i +2;
					else					skip_upper = skip_ver;
					if(i + skip_ver > 15)	skip_bottom = MB_SIZE+1 - i ;
					else					skip_bottom = skip_ver;

					if(prev==next)	*(smb_data+i*width+j)=prev;
					else			*(smb_data+i*width+j)= (ArDecodeSymbol (enh_intra_h_prob [contextSIHorizontal(smb_data+i*width+j,skip_upper,skip_bottom)], m_parcodec, m_pbitstrmIn)) ? MPEG4_OPAQUE : MPEG4_TRANSPARENT;
				}
			}
			num_loop_ver--;
		}
	}

}

Void CVideoObjectDecoder::HorizontalFulldecoding (Int num_loop_hor, Int num_loop_ver, Bool residual_scanning_hor, Bool residual_scanning_ver, Bool* HorSamplingChk, Bool* VerSamplingChk)
{
  //	Int iSizeMB = m_iWidthCurrBAB - 2*BAB_BORDER;
	PixelC* ppxlcDst  = m_rgpxlcCaeSymbol + m_iWidthCurrBAB * BAB_BORDER + BAB_BORDER;

	{	
	  Int		i, j;// , xor_type=0;
		Int		start_v=0, start_h=0;
		Int		width=20, height=20;
		//		UChar	xor_value=0;
		//		Int		prev=0, next=0;
		PixelC* smb_data = ppxlcDst;	
		Int		skip_ver=0, skip_upper=0, skip_bottom=0;
		Int		h_scan_freq=1, v_scan_freq=1;

		i=0;	
		while(*(VerSamplingChk+i) == 1)	i++;
		while(*(VerSamplingChk+i) == 0)	i++;
		int tmp = i;

		if ( residual_scanning_ver) {
			start_h = 0;
			start_v = 0;
			v_scan_freq = 1<<num_loop_ver; 
			h_scan_freq = 1;
			skip_ver = v_scan_freq;

			if( tmp-(1<<num_loop_ver) >= 0 ) 				start_v = tmp-(1<<num_loop_ver);
			else if( tmp+(1<<num_loop_ver) <= MB_SIZE-1 )	start_v = tmp+(1<<(num_loop_ver)) ;
			else printf("Out of Sampling Ratio\n");

			for(i=start_v;i<(height-4);i+=v_scan_freq){
				if( *(VerSamplingChk+i) == 1) continue;
				for(j=start_h;j<(width-4);j+=h_scan_freq){
					if ( i+skip_ver >15)	skip_bottom = MB_SIZE+1-i;
					else					skip_bottom = skip_ver;
					if ( i-skip_ver <0)		skip_upper = i+2;
					else					skip_upper = skip_ver;
					*(smb_data+i*width+j)= (ArDecodeSymbol (enh_intra_h_prob [contextSIHorizontal(smb_data+i*width+j,skip_upper,skip_bottom)], m_parcodec, m_pbitstrmIn)) ? MPEG4_OPAQUE : MPEG4_TRANSPARENT;
				}
			}
		}

		i=0;	
		while(*(VerSamplingChk+i) == 1)	i++;
		while(*(VerSamplingChk+i) == 0)	i++;
		if(i>start_v && residual_scanning_ver) tmp = start_v;
		else tmp = i;

		while (num_loop_ver >0){
			start_h = 0;
			start_v = 0;
			v_scan_freq=1<<num_loop_ver; 
			h_scan_freq=1;
			skip_ver = 1<<(num_loop_ver-1);

			if( tmp-(1<<(num_loop_ver-1)) >= 0 )	tmp = start_v = tmp-(1<<(num_loop_ver-1));
			else									start_v = tmp+(1<<(num_loop_ver-1)) ;

			skip_ver = 1<<(num_loop_ver-1);

			for(i=start_v;i<(height-4);i+=v_scan_freq){
				for(j=start_h;j<(width-4);j+=h_scan_freq){

				if(i - skip_ver < 0)	skip_upper = i +2;
				else					skip_upper = skip_ver;
				if(i + skip_ver > 15)	skip_bottom = MB_SIZE+1 - i ;
				else					skip_bottom = skip_ver;

				*(smb_data+i*width+j)= (ArDecodeSymbol (enh_intra_h_prob [contextSIHorizontal(smb_data+i*width+j,skip_upper,skip_bottom)], m_parcodec, m_pbitstrmIn)) ? MPEG4_OPAQUE : MPEG4_TRANSPARENT;
				}
			}
			num_loop_ver--;
		}
	}
}
//~OBSS_SAIT_991015

Int CVideoObjectDecoder::shpMdTableIndex (const CMBMode* pmbmd, Int iMBX, Int iMBY)
{	
	//get previous shape codes; 
	//	Added for error resilient mode by Toshiba(1997-11-14)
	Bool bLeftBndry, bRightTopBndry, bTopBndry, bLeftTopBndry;
	Int iMBnum = VPMBnum(iMBX, iMBY);
	bLeftBndry = bVPNoLeft(iMBnum, iMBX);
	bTopBndry = bVPNoTop(iMBnum);
	bRightTopBndry = bVPNoRightTop(iMBnum, iMBX);
	bLeftTopBndry = bVPNoLeftTop(iMBnum, iMBX);

	ShapeMode shpmdTop	= !bTopBndry ? (pmbmd - m_iNumMBX)->m_shpmd : ALL_TRANSP;
	ShapeMode shpmdTopR	= !bRightTopBndry ? (pmbmd - m_iNumMBX + 1)->m_shpmd : ALL_TRANSP;
	ShapeMode shpmdTopL	= !bLeftTopBndry ? (pmbmd - m_iNumMBX - 1)->m_shpmd : ALL_TRANSP;
	ShapeMode shpmdLeft	= !bLeftBndry ? (pmbmd - 1)->m_shpmd : ALL_TRANSP;
	return (shpmdTopL * 81 +  shpmdTop * 27 + shpmdTopR * 9 + shpmdLeft * 3);

	//const CMBMode* pmbmdTop = pmbmd - m_iNumMBX;
	//ShapeMode shpmdTop		= (iMBY > 0) ? pmbmdTop->m_shpmd : ALL_TRANSP;
	//ShapeMode shpmdTopR	= (iMBY > 0 && iMBX < m_iNumMBX - 1) ? (pmbmdTop + 1)->m_shpmd : ALL_TRANSP;
	//ShapeMode shpmdTopL	= (iMBY > 0 && iMBX > 0) ? (pmbmdTop - 1)->m_shpmd : ALL_TRANSP;
	//ShapeMode shpmdLeft		= (iMBX > 0) ? (pmbmd - 1)->m_shpmd : ALL_TRANSP;
	//return (shpmdTopL * 81 +  shpmdTop * 27 + shpmdTopR * 9 + shpmdLeft * 3);
	// End Toshiba(1997-11-14)
}

Void CVideoObjectDecoder::decodeIntraCaeBAB (PixelC* ppxlcBYMB, PixelC* ppxlcBYFrm)
{
	if (m_volmd.bNoCrChange == TRUE || m_pbitstrmIn->getBits (1) == 0)	{
		m_iInverseCR = 1;
		m_iWidthCurrBAB = BAB_SIZE;
		copyLeftTopBorderFromVOP (ppxlcBYFrm, m_ppxlcReconCurrBAB);  //used for upsample and CAE
		m_rgpxlcCaeSymbol = m_ppxlcReconCurrBAB;		//assign encoding buffer
		if (m_pbitstrmIn->getBits (1) == 1) 
			decodeIntraCAEH ();			//right bottom border made on the fly
		else
			decodeIntraCAEV ();
	}
	else	{
		if (m_pbitstrmIn->getBits (1) == 0)	{
			m_iInverseCR = 2;
			m_iWidthCurrBAB = 12;
			subsampleLeftTopBorderFromVOP (ppxlcBYFrm, m_ppxlcCurrMBBYDown2);
			m_rgpxlcCaeSymbol = m_ppxlcCurrMBBYDown2;		//assign encoding buffer
		}
		else {
			m_iInverseCR = 4;
			m_iWidthCurrBAB = 8;
			subsampleLeftTopBorderFromVOP (ppxlcBYFrm, m_ppxlcCurrMBBYDown4);
			m_rgpxlcCaeSymbol = m_ppxlcCurrMBBYDown4;		//assign encoding buffer
		}
		if (m_pbitstrmIn->getBits (1) == 1) 
			decodeIntraCAEH ();     //right bottom border made on the fly
		else
			decodeIntraCAEV (); 

		upSampleShape(ppxlcBYFrm,m_rgpxlcCaeSymbol,m_ppxlcReconCurrBAB);
	}
	
	copyReconShapeToMbAndRef (ppxlcBYMB, ppxlcBYFrm, m_ppxlcReconCurrBAB, TOTAL_BAB_SIZE, BAB_BORDER);
}

Void CVideoObjectDecoder::decodeIntraCAEH ()
{
	StartArDecoder (m_parcodec, m_pbitstrmIn);
	PixelC* ppxlcDstRow = m_rgpxlcCaeSymbol + m_iWidthCurrBAB * BAB_BORDER + BAB_BORDER;
	for (Int iRow = BAB_BORDER; iRow < m_iWidthCurrBAB - BAB_BORDER; iRow++)	{
		PixelC* ppxlcDst = ppxlcDstRow;
		for (Int iCol = BAB_BORDER; iCol < m_iWidthCurrBAB - BAB_BORDER; iCol++)	{
			Int iContext = contextIntra (ppxlcDst);//printf("%d ",iContext);
			*ppxlcDst++ = (ArDecodeSymbol (gCAEintraProb [iContext], m_parcodec, m_pbitstrmIn)) ? MPEG4_OPAQUE : MPEG4_TRANSPARENT;
		}
		*ppxlcDst = *(ppxlcDst - 1);
		*(ppxlcDst + 1) = *ppxlcDst;		//make right border on the fly
		ppxlcDstRow += m_iWidthCurrBAB;
	}//printf("\n\n");
	StopArDecoder (m_parcodec, m_pbitstrmIn);
	PixelC* ppxlcDst = m_rgpxlcCaeSymbol + 
		m_iWidthCurrBAB * (m_iWidthCurrBAB - BAB_BORDER) + BAB_BORDER;
	for (Int iCol = BAB_BORDER; iCol < m_iWidthCurrBAB; iCol++)	{
		//make bottom border
		*ppxlcDst = *(ppxlcDst - m_iWidthCurrBAB);
		*(ppxlcDst + m_iWidthCurrBAB) = *ppxlcDst;
		ppxlcDst++;
	}
}

Void CVideoObjectDecoder::decodeIntraCAEV ()
{
	StartArDecoder (m_parcodec, m_pbitstrmIn);
	PixelC* ppxlcDstCol = m_rgpxlcCaeSymbol + m_iWidthCurrBAB * BAB_BORDER + BAB_BORDER;
	for (Int iCol = BAB_BORDER; iCol < m_iWidthCurrBAB - BAB_BORDER; iCol++)	{
		PixelC* ppxlcDst = ppxlcDstCol;
		for (Int iRow = BAB_BORDER; iRow < m_iWidthCurrBAB - BAB_BORDER; iRow++)	{
			Int iContext = contextIntraTranspose (ppxlcDst);
			*ppxlcDst = (ArDecodeSymbol (gCAEintraProb [iContext], m_parcodec, m_pbitstrmIn)) ? MPEG4_OPAQUE : MPEG4_TRANSPARENT;
			ppxlcDst += m_iWidthCurrBAB;
		}
		*ppxlcDst = *(ppxlcDst - m_iWidthCurrBAB);
		*(ppxlcDst + m_iWidthCurrBAB) = *ppxlcDst;		//make bottom border on the fly
		ppxlcDstCol++;
	}
	StopArDecoder (m_parcodec, m_pbitstrmIn);
	PixelC* ppxlcDst = m_rgpxlcCaeSymbol + (m_iWidthCurrBAB - BAB_BORDER) + m_iWidthCurrBAB * BAB_BORDER;
	for (Int iRow = BAB_BORDER; iRow < m_iWidthCurrBAB; iRow++)	{
		//make right border
		*ppxlcDst = *(ppxlcDst - 1);
		*(ppxlcDst + 1) = *ppxlcDst;
		ppxlcDst += m_iWidthCurrBAB;
	}
}


Void CVideoObjectDecoder::decodeInterCAEH (const PixelC *ppxlcPredBAB)
{
	StartArDecoder (m_parcodec, m_pbitstrmIn);
	Int iSizePredBAB = m_iWidthCurrBAB - 2*BAB_BORDER + 2*MC_BAB_BORDER;
	Int iSizeMB = m_iWidthCurrBAB - 2*BAB_BORDER;
	PixelC* ppxlcDstRow  = m_rgpxlcCaeSymbol + m_iWidthCurrBAB * BAB_BORDER + BAB_BORDER;
	const PixelC* ppxlcPredRow = ppxlcPredBAB + iSizePredBAB * MC_BAB_BORDER + MC_BAB_BORDER;
	for (Int iRow = 0; iRow < iSizeMB; iRow++)	{
		PixelC* ppxlcDst = ppxlcDstRow;
		const PixelC* ppxlcPred = ppxlcPredRow;
		for (Int iCol = 0; iCol < iSizeMB; iCol++)	{
			Int iContext = contextInter (ppxlcDst, ppxlcPred);
			*ppxlcDst++ = (ArDecodeSymbol (gCAEinterProb [iContext], m_parcodec, m_pbitstrmIn)) ? MPEG4_OPAQUE : MPEG4_TRANSPARENT;
			ppxlcPred++;
		}
		*ppxlcDst = *(ppxlcDst - 1);
		*(ppxlcDst + 1) = *ppxlcDst;		//make right border on the fly
		ppxlcDstRow += m_iWidthCurrBAB;
		ppxlcPredRow += iSizePredBAB;
	}
	StopArDecoder (m_parcodec, m_pbitstrmIn);
	PixelC* ppxlcDst = m_rgpxlcCaeSymbol + 
		m_iWidthCurrBAB * (m_iWidthCurrBAB - BAB_BORDER) + BAB_BORDER;
	for (Int iCol = BAB_BORDER; iCol < m_iWidthCurrBAB; iCol++)	{
		//make bottom border
		*ppxlcDst = *(ppxlcDst - m_iWidthCurrBAB);
		*(ppxlcDst + m_iWidthCurrBAB) = *ppxlcDst;
		ppxlcDst++;
	}
}

Void CVideoObjectDecoder::decodeInterCAEV (const PixelC *ppxlcPredBAB)
{
	StartArDecoder (m_parcodec, m_pbitstrmIn);
	Int iSizePredBAB = m_iWidthCurrBAB - 2*BAB_BORDER + 2*MC_BAB_BORDER;
	Int iSizeMB = m_iWidthCurrBAB - 2*BAB_BORDER;
	PixelC* ppxlcDstCol  = m_rgpxlcCaeSymbol + m_iWidthCurrBAB * BAB_BORDER + BAB_BORDER;
	const PixelC* ppxlcPredCol = ppxlcPredBAB + iSizePredBAB * MC_BAB_BORDER + MC_BAB_BORDER;
	for (Int iCol = 0; iCol < iSizeMB;iCol++)	{
		PixelC* ppxlcDst = ppxlcDstCol;
		const PixelC* ppxlcPred = ppxlcPredCol;
		for (Int iRow = 0; iRow < iSizeMB; iRow++)	{
			Int iContext = contextInterTranspose (ppxlcDst, ppxlcPred);
			*ppxlcDst = (ArDecodeSymbol (gCAEinterProb [iContext], m_parcodec, m_pbitstrmIn)) ? MPEG4_OPAQUE : MPEG4_TRANSPARENT;
			ppxlcDst += m_iWidthCurrBAB;
			ppxlcPred += iSizePredBAB;
		}
		*ppxlcDst = *(ppxlcDst - m_iWidthCurrBAB);
		*(ppxlcDst + m_iWidthCurrBAB) = *ppxlcDst;		//make bottom border on the fly
		ppxlcDstCol++;
		ppxlcPredCol++;
	}
	StopArDecoder (m_parcodec, m_pbitstrmIn);
	PixelC* ppxlcDst = m_rgpxlcCaeSymbol + (m_iWidthCurrBAB - BAB_BORDER) + m_iWidthCurrBAB * BAB_BORDER;
	for (Int iRow = BAB_BORDER; iRow < m_iWidthCurrBAB; iRow++)	{
		//make right border
		*ppxlcDst = *(ppxlcDst - 1);
		*(ppxlcDst + 1) = *ppxlcDst;
		ppxlcDst += m_iWidthCurrBAB;
	}
}

Void CVideoObjectDecoder::decodeMVDS (CMotionVector& mvDiff)
{
	mvDiff.iMVX = m_pentrdecSet->m_pentrdecShapeMV1->decodeSymbol ();			
	if (mvDiff.iMVX != 0)
		if (m_pbitstrmIn->getBits (1) == 0)
			mvDiff.iMVX *= -1;
	if (mvDiff.iMVX == 0)	
		mvDiff.iMVY = m_pentrdecSet->m_pentrdecShapeMV2->decodeSymbol () + 1;			
	else
		mvDiff.iMVY = m_pentrdecSet->m_pentrdecShapeMV1->decodeSymbol ();			
	if (mvDiff.iMVY != 0)
		if (m_pbitstrmIn->getBits (1) == 0)
			mvDiff.iMVY *= -1;
	mvDiff.computeTrueMV ();
}

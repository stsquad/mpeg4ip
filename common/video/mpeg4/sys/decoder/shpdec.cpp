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
#include "entropy/bitstrm.hpp"
#include "entropy/entropy.hpp"
#include "entropy/huffman.hpp"
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
		pxlcmemset (ppxlcCurrMBBY, TRANSPARENT, MB_SQUARE_SIZE);
		copyReconShapeToMbAndRef (ppxlcCurrMBBY, ppxlcCurrMBBYFrm, TRANSPARENT);
		pmbmd->m_rgTranspStatus [0] = pmbmd->m_rgTranspStatus [1] = pmbmd->m_rgTranspStatus [2] = pmbmd->m_rgTranspStatus [3] =
		pmbmd->m_rgTranspStatus [4] = pmbmd->m_rgTranspStatus [5] = pmbmd->m_rgTranspStatus [6] = ALL;
	}
	else if (iBits == rgiShapeMdCode [ALL_OPAQUE])	{
		pmbmd->m_shpmd = ALL_OPAQUE;//printf("X");
		copyReconShapeToMbAndRef (ppxlcCurrMBBY, ppxlcCurrMBBYFrm, OPAQUE);
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
		copyReconShapeToMbAndRef (ppxlcMBBY, ppxlcMBBYFrm, TRANSPARENT);
		pmbmd->m_rgTranspStatus [0] = pmbmd->m_rgTranspStatus [1] = pmbmd->m_rgTranspStatus [2] = pmbmd->m_rgTranspStatus [3] =
		pmbmd->m_rgTranspStatus [4] = pmbmd->m_rgTranspStatus [5] = pmbmd->m_rgTranspStatus [6] = ALL;
	}
	else if (pmbmd->m_shpmd == ALL_OPAQUE)	{//printf("X");
		copyReconShapeToMbAndRef (ppxlcMBBY, ppxlcMBBYFrm, OPAQUE);
		pmbmd->m_rgTranspStatus [0] = pmbmd->m_rgTranspStatus [1] = pmbmd->m_rgTranspStatus [2] = pmbmd->m_rgTranspStatus [3] =
		pmbmd->m_rgTranspStatus [4] = pmbmd->m_rgTranspStatus [5] = pmbmd->m_rgTranspStatus [6] = NONE;
	}
	else if (pmbmd->m_shpmd == INTRA_CAE)	{//printf("S");
		decodeIntraCaeBAB (ppxlcMBBY, ppxlcMBBYFrm);
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
		motionCompBY ((PixelC*) m_puciPredBAB->pixels (),
					  (PixelC*) pvopcRefQ->getPlane (BY_PLANE)->pixels (),
					  pmvBY->iMVX + iX - 1,		
					  pmvBY->iMVY + iY - 1);	//-1 due to 18x18 motion comp
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
			*ppxlcDst++ = (ArDecodeSymbol (gCAEintraProb [iContext], m_parcodec, m_pbitstrmIn)) ? OPAQUE : TRANSPARENT;
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
			*ppxlcDst = (ArDecodeSymbol (gCAEintraProb [iContext], m_parcodec, m_pbitstrmIn)) ? OPAQUE : TRANSPARENT;
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
			*ppxlcDst++ = (ArDecodeSymbol (gCAEinterProb [iContext], m_parcodec, m_pbitstrmIn)) ? OPAQUE : TRANSPARENT;
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
			*ppxlcDst = (ArDecodeSymbol (gCAEinterProb [iContext], m_parcodec, m_pbitstrmIn)) ? OPAQUE : TRANSPARENT;
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

/*************************************************************************

This software module was originally developed by 

	Ming-Chieh Lee (mingcl@microsoft.com), Microsoft Corporation
	Wei-ge Chen (wchen@microsoft.com), Microsoft Corporation
	Bruce Lin (blin@microsoft.com), Microsoft Corporation
	Chuang Gu (chuanggu@microsoft.com), Microsoft Corporation
	Simon Winder (swinder@microsoft.com), Microsoft Corporation
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

	MBDec.cpp

Abstract:

	MacroBlock level decoder

Revision History:
        May. 9   1998:  add boundary by Hyundai Electronics 
                                  Cheol-Soo Park (cspark@super5.hyundai.co.kr) 

*************************************************************************/

#include "typeapi.h"
#include "codehead.h"
#include "mode.hpp"
#include "global.hpp"
#include "entropy/bitstrm.hpp"
#include "entropy/entropy.hpp"
#include "entropy/huffman.hpp"
#include "vopses.hpp"
#include "vopsedec.hpp"

#ifdef __MFC_
#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

#define new DEBUG_NEW				   
#endif // __MFC_

Void CVideoObjectDecoder::swapCurrAndRightMBForShape ()
{
	CVOPU8YUVBA* pvopcTmp = m_pvopcCurrMB;
	m_pvopcCurrMB = m_pvopcRightMB;
	m_pvopcRightMB = pvopcTmp;
	
	m_ppxlcCurrMBBY = (PixelC*) m_pvopcCurrMB->pixelsBY ();
	m_ppxlcCurrMBBUV = (PixelC*) m_pvopcCurrMB->pixelsBUV ();

	m_ppxlcRightMBBY = (PixelC*) m_pvopcRightMB->pixelsBY ();
	m_ppxlcRightMBBUV = (PixelC*) m_pvopcRightMB->pixelsBUV ();

}

Void CVideoObjectDecoder::copyFromPredForYAndRefForCToCurrQ (
	CoordI x, CoordI y, 
	PixelC* ppxlcCurrQMBY, PixelC* ppxlcCurrQMBU, PixelC* ppxlcCurrQMBV,
	CRct *prctMVLimit
)
{
	Int iUnit = sizeof(PixelC); // NBIT: for memcpy
	// needs limiting to reference area bounding box
	limitMVRangeToExtendedBBFullPel(x,y,prctMVLimit,MB_SIZE);

	Int iOffsetUV = (y / 2 + EXPANDUV_REF_FRAME) * m_iFrameWidthUV + x / 2 + EXPANDUV_REF_FRAME;
	const PixelC* ppxlcPredMBY = m_ppxlcPredMBY;
	const PixelC* ppxlcRefMBU = m_pvopcRefQ0->pixelsU () + iOffsetUV;
	const PixelC* ppxlcRefMBV = m_pvopcRefQ0->pixelsV () + iOffsetUV;

	CoordI iY;
	for (iY = 0; iY < BLOCK_SIZE; iY++) {
		memcpy (ppxlcCurrQMBY, ppxlcPredMBY, MB_SIZE*iUnit);
		memcpy (ppxlcCurrQMBU, ppxlcRefMBU, BLOCK_SIZE*iUnit);
		memcpy (ppxlcCurrQMBV, ppxlcRefMBV, BLOCK_SIZE*iUnit);

		ppxlcCurrQMBY += m_iFrameWidthY; ppxlcPredMBY += MB_SIZE;
		ppxlcCurrQMBU += m_iFrameWidthUV; ppxlcRefMBU += m_iFrameWidthUV;
		ppxlcCurrQMBV += m_iFrameWidthUV; ppxlcRefMBV += m_iFrameWidthUV;

		memcpy (ppxlcCurrQMBY, ppxlcPredMBY, MB_SIZE*iUnit);
		ppxlcCurrQMBY += m_iFrameWidthY; ppxlcPredMBY += MB_SIZE;
	}
}

Void CVideoObjectDecoder::copyAlphaFromPredToCurrQ (
	CoordI x, CoordI y, 
	PixelC* ppxlcCurrQMBA
)
{
	const PixelC* ppxlcPredMBA = m_ppxlcPredMBA;

	CoordI iY;
	for (iY = 0; iY < MB_SIZE; iY++) {
		memcpy (ppxlcCurrQMBA, ppxlcPredMBA, MB_SIZE);
		ppxlcCurrQMBA += m_iFrameWidthY; 
		ppxlcPredMBA += MB_SIZE;
	}
}

// for Direct and Interpolate mode in B-VOP
Void CVideoObjectDecoder::averagePredAndAddErrorToCurrQ (
	PixelC* ppxlcCurrQMBY, PixelC* ppxlcCurrQMBU, PixelC* ppxlcCurrQMBV
)
{
	CoordI ix, iy, ic = 0;
	for (iy = 0; iy < MB_SIZE; iy++) {
		for (ix = 0; ix < MB_SIZE; ix++, ic++) {
			ppxlcCurrQMBY [ix] = m_rgiClipTab [
				((m_ppxlcPredMBY [ic] + m_ppxlcPredMBBackY [ic] + 1) >> 1) + 
				m_ppxliErrorMBY [ic]
			];
		}
		ppxlcCurrQMBY += m_iFrameWidthY;
	}

	ic = 0;
	for (iy = 0; iy < BLOCK_SIZE; iy++) {
		for (ix = 0; ix < BLOCK_SIZE; ix++, ic++) {
			ppxlcCurrQMBU [ix] = m_rgiClipTab [
				((m_ppxlcPredMBU [ic] + m_ppxlcPredMBBackU [ic] + 1) >> 1) + 
				m_ppxliErrorMBU [ic]
			];
			ppxlcCurrQMBV [ix] = m_rgiClipTab [
				((m_ppxlcPredMBV [ic] + m_ppxlcPredMBBackV [ic] + 1) >> 1) + 
				m_ppxliErrorMBV [ic]
			];
		}
		ppxlcCurrQMBU += m_iFrameWidthUV;
		ppxlcCurrQMBV += m_iFrameWidthUV;
	}
}

Void CVideoObjectDecoder::averagePredAndAssignToCurrQ (
	PixelC* ppxlcCurrQMBY, PixelC* ppxlcCurrQMBU, PixelC* ppxlcCurrQMBV
)
{
	CoordI ix, iy, ic = 0;
	for (iy = 0; iy < MB_SIZE; iy++) {
		for (ix = 0; ix < MB_SIZE; ix++, ic++)
			ppxlcCurrQMBY [ix] = (m_ppxlcPredMBY [ic] + m_ppxlcPredMBBackY [ic] + 1) >> 1; // don't need to clip
		ppxlcCurrQMBY += m_iFrameWidthY;
	}

	ic = 0;
	for (iy = 0; iy < BLOCK_SIZE; iy++) {
		for (ix = 0; ix < BLOCK_SIZE; ix++, ic++) {
			ppxlcCurrQMBU [ix] = (m_ppxlcPredMBU [ic] + m_ppxlcPredMBBackU [ic] + 1) >> 1;
			ppxlcCurrQMBV [ix] = (m_ppxlcPredMBV [ic] + m_ppxlcPredMBBackV [ic] + 1) >> 1;
		}
		ppxlcCurrQMBU += m_iFrameWidthUV;
		ppxlcCurrQMBV += m_iFrameWidthUV;
	}
}

// texture and overhead
// Intra
Void CVideoObjectDecoder::decodeTextureIntraMB (
	CMBMode* pmbmd, CoordI iMBX, CoordI iMBY, 
	PixelC* ppxlcCurrFrmQY, PixelC* ppxlcCurrFrmQU, PixelC* ppxlcCurrFrmQV)
{
	assert (pmbmd != NULL);
	if (pmbmd -> m_rgTranspStatus [0] == ALL) 
		return;

	assert (pmbmd->m_dctMd == INTRA || pmbmd->m_dctMd == INTRAQ);
	Int iQP = pmbmd->m_stepSize;
	Int iDcScalerY, iDcScalerC;

	if (iQP <= 4)	{
		iDcScalerY = 8;
		iDcScalerC = 8;
	}
	else if (iQP >= 5 && iQP <= 8)	{
		iDcScalerY = 2 * iQP;
		iDcScalerC = (iQP + 13) / 2;
	}
	else if (iQP >= 9 && iQP <= 24)	{
		iDcScalerY = iQP + 8;
		iDcScalerC = (iQP + 13) / 2;
	}
	else	{
		iDcScalerY = 2 * iQP - 16;
		iDcScalerC = iQP - 6;
	}

	assert (iQP > 0);
	// more bogus technology
	assert (pmbmd -> m_stepSizeDelayed > 0);
	if (pmbmd -> m_stepSizeDelayed >= grgiDCSwitchingThreshold [m_vopmd.iIntraDcSwitchThr])
		pmbmd->m_bCodeDcAsAc = TRUE;
	else
		pmbmd->m_bCodeDcAsAc = FALSE;

	//for intra pred
	MacroBlockMemory* pmbmLeft = NULL;
	MacroBlockMemory* pmbmTop = NULL;
	MacroBlockMemory* pmbmLeftTop = NULL;
	CMBMode* pmbmdLeft = NULL;
	CMBMode* pmbmdTop = NULL;
	CMBMode* pmbmdLeftTop = NULL;
											 
	Int iMBTop	= iMBY - 1;

// dshu: begin of modification
	if (m_uiSprite == 1 && m_vopmd.SpriteXmitMode != STOP) {
		assert (pmbmd->m_iVideoPacketNumber == 0);
		if (iMBTop >= 0)
			(pmbmd - m_iNumMBX)->m_iVideoPacketNumber = 0;
		if (iMBX > 0)
			(pmbmd - 1)->m_iVideoPacketNumber = 0;
		if (iMBTop >= 0 && iMBX > 0)
			(pmbmd - m_iNumMBX - 1)->m_iVideoPacketNumber = 0 ;
	}
// dshu: end of modification
	if (iMBTop >= 0)	{
		if (pmbmd->m_iVideoPacketNumber == (pmbmd - m_iNumMBX)->m_iVideoPacketNumber)	{
			pmbmTop  = m_rgpmbmAbove [iMBX];
			pmbmdTop = pmbmd - m_iNumMBX; 
		}
	}

	if (iMBX > 0)	{
		if (pmbmd->m_iVideoPacketNumber == (pmbmd - 1)->m_iVideoPacketNumber)	{
			pmbmLeft  = m_rgpmbmCurr [iMBX - 1];
			pmbmdLeft = pmbmd -  1; 
		}
	}

	if (iMBTop >= 0 && iMBX > 0)	{
		if (pmbmd->m_iVideoPacketNumber == (pmbmd - m_iNumMBX - 1)->m_iVideoPacketNumber)	{
			pmbmLeftTop  = m_rgpmbmAbove [iMBX - 1];
			pmbmdLeftTop = pmbmd - m_iNumMBX - 1;
		}
	}

	PixelC* rgchBlkDst = NULL;
	Int iWidthDst;
	Int iDcScaler;
	Int* rgiCoefQ;
	for (Int iBlk = Y_BLOCK1; iBlk <= V_BLOCK; iBlk++) {
		if (iBlk < U_BLOCK) {
			if (pmbmd -> m_rgTranspStatus [iBlk] == ALL) 
				continue;
			switch (iBlk) 
			{
			case (Y_BLOCK1): 
				rgchBlkDst = ppxlcCurrFrmQY;
				break;
			case (Y_BLOCK2): 
				rgchBlkDst = ppxlcCurrFrmQY + BLOCK_SIZE;
				break;
			case (Y_BLOCK3): 
				rgchBlkDst = ppxlcCurrFrmQY + m_iFrameWidthYxBlkSize;
				break;
			case (Y_BLOCK4): 
				rgchBlkDst = ppxlcCurrFrmQY + m_iFrameWidthYxBlkSize + BLOCK_SIZE;
				break;
			}
			iWidthDst = m_iFrameWidthY;
			iDcScaler = iDcScalerY;
		}
		else {
			iWidthDst = m_iFrameWidthUV;
			rgchBlkDst = (iBlk == U_BLOCK) ? ppxlcCurrFrmQU: ppxlcCurrFrmQV;
			iDcScaler = iDcScalerC;
		}

		rgiCoefQ = m_rgpiCoefQ [iBlk - 1];
		const BlockMemory blkmPred = NULL;
		Int iQpPred = iQP; //default to current if no pred
		decideIntraPred (blkmPred, 
						 pmbmd,
						 iQpPred,
						 (BlockNum) iBlk,
						 pmbmLeft, 
  						 pmbmTop, 
						 pmbmLeftTop,
						 m_rgpmbmCurr [iMBX],
						 pmbmdLeft,
						 pmbmdTop,
						 pmbmdLeftTop);
		decodeIntraBlockTexture (rgchBlkDst,
								 iWidthDst,
								 iQP, 
								 iDcScaler,
								 iBlk,	
								 m_rgpmbmCurr [iMBX],
								 pmbmd,
 								 blkmPred, //for intra-pred
								 iQpPred);	
		
		/*BBM// Added for Boundary by Hyundai(1998-5-9)
                if (m_vopmd.bInterlace && pmbmd->m_bMerged [0]) {
                        Int iDstBlk = 0;
                        switch (iBlk) {
                                case (Y_BLOCK1):
                                        if (pmbmd->m_bMerged [1])       iDstBlk = (Int) Y_BLOCK2;
                                        else if (pmbmd->m_bMerged [3])  iDstBlk = (Int) Y_BLOCK3;
                                        else if (pmbmd->m_bMerged [5])  iDstBlk = (Int) Y_BLOCK4;
                                        break;
                                case (Y_BLOCK2):
                                        if (pmbmd->m_bMerged [4])       iDstBlk = (Int) Y_BLOCK4;
                                        else if (pmbmd->m_bMerged [6])  iDstBlk = (Int) Y_BLOCK3;
                                        break;
                                case (Y_BLOCK3):
                                        if (pmbmd->m_bMerged [2])       iDstBlk = (Int) Y_BLOCK4;
                                        break;
                        }
                        if (iDstBlk) {
                                MacroBlockMemory* pmbmCurr = m_rgpmbmCurr [iMBX];
                                pmbmCurr->rgblkm [iDstBlk-1][0] = pmbmCurr->rgblkm [iBlk-1][0];
                                for (UInt x = 1; x < (BLOCK_SIZE<<1)-1; x++)
                                        pmbmCurr->rgblkm [iDstBlk-1][x] = 0;
                        }
                }
		// End of Hyundai(1998-5-9)*/
	}
}

Void CVideoObjectDecoder::decodeAlphaIntraMB (CMBMode* pmbmd, Int iMBX, Int iMBY, PixelC* ppxlcRefMBA)
{
	assert (pmbmd != NULL);
	if (pmbmd -> m_rgTranspStatus [0] == ALL) 
		return;

	assert (pmbmd->m_dctMd == INTRA || pmbmd->m_dctMd == INTRAQ);
	Int iQP = pmbmd->m_stepSizeAlpha;
	Int iDcScalerA;

	if (pmbmd -> m_stepSizeAlpha < 1)
		pmbmd -> m_stepSizeAlpha = 1;
	if (iQP <= 4)	{
		iDcScalerA = 8;
	}
	else if (iQP >= 5 && iQP <= 8)	{
		iDcScalerA = 2 * iQP;
	}
	else if (iQP >= 9 && iQP <= 24)	{
		iDcScalerA = iQP + 8;
	}
	else	{
		iDcScalerA = 2 * iQP - 16;
	}

	assert (iQP > 0 && iQP < 64);

	// should be stepSizeAlpha and a different table
	//if (pmbmd -> m_stepSize >= grgiDCSwitchingThreshold [m_vopmd.iIntraDcSwitchThr])
	//	pmbmd->m_bCodeDcAsAcAlpha = TRUE;
	//else
	pmbmd->m_bCodeDcAsAcAlpha = FALSE;

	if(pmbmd->m_CODAlpha==ALPHA_ALL255)
	{
		// fill curr macroblock with 255
		Int iY;
		PixelC *ppxlc = ppxlcRefMBA;
		for(iY = 0; iY<MB_SIZE; iY++, ppxlc += m_iFrameWidthY)
			pxlcmemset(ppxlc, 255, MB_SIZE);

		// fix intra prediction
		Int iBlk;
		MacroBlockMemory* pmbmCurr = m_rgpmbmCurr [iMBX];
		for(iBlk = A_BLOCK1; iBlk<=A_BLOCK4; iBlk++)
		{
			Int i;
			pmbmCurr->rgblkm [iBlk - 1] [0] = divroundnearest(255 * 8, iDcScalerA) * iDcScalerA;
			//save recon value of DC for intra pred								//save Qcoef in memory
			for (i = 1; i < BLOCK_SIZE; i++)	{
				pmbmCurr->rgblkm [iBlk - 1] [i] = 0;
				pmbmCurr->rgblkm [iBlk - 1] [i + BLOCK_SIZE - 1] = 0;
			}				
		}

		return;
	}


	//for intra pred
	MacroBlockMemory* pmbmLeft = NULL;
	MacroBlockMemory* pmbmTop = NULL;
	MacroBlockMemory* pmbmLeftTop = NULL;
	CMBMode* pmbmdLeft = NULL;
	CMBMode* pmbmdTop = NULL;
	CMBMode* pmbmdLeftTop = NULL;
											 
	Int iMBTop	= iMBY - 1;
	if (iMBTop >= 0)	{
		if (pmbmd->m_iVideoPacketNumber == (pmbmd - m_iNumMBX)->m_iVideoPacketNumber)	{
			pmbmTop  = m_rgpmbmAbove [iMBX];
			pmbmdTop = pmbmd - m_iNumMBX; 
		}
	}

	if (iMBX > 0)	{
		if (pmbmd->m_iVideoPacketNumber == (pmbmd - 1)->m_iVideoPacketNumber)	{
			pmbmLeft  = m_rgpmbmCurr [iMBX - 1];
			pmbmdLeft = pmbmd -  1; 
		}
	}

	if (iMBTop >= 0 && iMBX > 0)	{
		if (pmbmd->m_iVideoPacketNumber == (pmbmd - m_iNumMBX - 1)->m_iVideoPacketNumber)	{
			pmbmLeftTop  = m_rgpmbmAbove [iMBX - 1];
			pmbmdLeftTop = pmbmd - m_iNumMBX - 1;
		}
	}

	PixelC* rgchBlkDst = NULL;
	Int* rgiCoefQ;
	for (Int iBlk = A_BLOCK1; iBlk <= A_BLOCK4; iBlk++) {
		if (pmbmd -> m_rgTranspStatus [iBlk - 6] == ALL) 
			continue;
		switch (iBlk) 
		{
		case (A_BLOCK1): 
			rgchBlkDst = ppxlcRefMBA;
			break;
		case (A_BLOCK2): 
			rgchBlkDst = ppxlcRefMBA + BLOCK_SIZE;
			break;
		case (A_BLOCK3): 
			rgchBlkDst = ppxlcRefMBA + m_iFrameWidthYxBlkSize;
			break;
		case (A_BLOCK4): 
			rgchBlkDst = ppxlcRefMBA + m_iFrameWidthYxBlkSize + BLOCK_SIZE;
			break;
		}

		rgiCoefQ = m_rgpiCoefQ [iBlk - 1];
		const BlockMemory blkmPred = NULL;
		Int iQpPred = iQP;
		decideIntraPred (blkmPred, 
						 pmbmd,
						 iQpPred,
						 (BlockNum) iBlk,
						 pmbmLeft, 
  						 pmbmTop, 
						 pmbmLeftTop,
						 m_rgpmbmCurr [iMBX], // save for curr coefs
						 pmbmdLeft,
						 pmbmdTop,
						 pmbmdLeftTop);
		decodeIntraBlockTexture (rgchBlkDst,
								 m_iFrameWidthY,
								 iQP, 
								 iDcScalerA,
								 iBlk,	
								 m_rgpmbmCurr [iMBX],
								 pmbmd,
 								 blkmPred, //for intra-pred
								 iQpPred);	
		/*BBM// Added for Boundary by Hyundai(1998-5-9)
                if (m_vopmd.bInterlace && pmbmd->m_bMerged [0]) {
                        Int iDstBlk = 0;
                        switch (iBlk) {
                                case (A_BLOCK1):
                                        if (pmbmd->m_bMerged [1])       iDstBlk = (Int) A_BLOCK2;
                                        else if (pmbmd->m_bMerged [3])  iDstBlk = (Int) A_BLOCK3;
                                        else if (pmbmd->m_bMerged [5])  iDstBlk = (Int) A_BLOCK4;
                                        break;
                                case (A_BLOCK2):
                                        if (pmbmd->m_bMerged [4])       iDstBlk = (Int) A_BLOCK4;
                                        else if (pmbmd->m_bMerged [6])  iDstBlk = (Int) A_BLOCK3;
                                        break;
                                case (A_BLOCK3):
                                        if (pmbmd->m_bMerged [2])       iDstBlk = (Int) A_BLOCK4;
                                        break;
                        }
                        if (iDstBlk) {
                                MacroBlockMemory* pmbmCurr = m_rgpmbmCurr [iMBX];
                                pmbmCurr->rgblkm [iDstBlk-1][0] = pmbmCurr->rgblkm [iBlk-1][0];
                                for (UInt x = 1; x < (BLOCK_SIZE<<1)-1; x++)
                                        pmbmCurr->rgblkm [iDstBlk-1][x] = 0;
                        }
                }
		// End of Hyundai(1998-5-9)*/
	}
}

Void CVideoObjectDecoder::decodeTextureInterMB (CMBMode* pmbmd)
{
	assert (pmbmd != NULL);
	if (pmbmd->m_rgTranspStatus [0] == ALL || pmbmd->m_bSkip) 
		return;

	assert (pmbmd->m_dctMd == INTER || pmbmd->m_dctMd == INTERQ);
	Int iQP = pmbmd->m_stepSize;
	Int* rgiBlkCurrQ = m_ppxliErrorMBY;
	Int iWidthCurrQ;
	for (Int iBlk = Y_BLOCK1; iBlk <= V_BLOCK; iBlk++) {
		if (iBlk < U_BLOCK) {
			if (pmbmd -> m_rgTranspStatus [iBlk] == ALL) 
				continue;
			switch (iBlk) 
			{
			case (Y_BLOCK1): 
				rgiBlkCurrQ = m_ppxliErrorMBY;
				break;
			case (Y_BLOCK2): 
				rgiBlkCurrQ = m_ppxliErrorMBY + BLOCK_SIZE;
				break;
			case (Y_BLOCK3): 
				rgiBlkCurrQ = m_ppxliErrorMBY + MB_SIZE * BLOCK_SIZE;
				break;
			case (Y_BLOCK4): 
				rgiBlkCurrQ = m_ppxliErrorMBY + MB_SIZE * BLOCK_SIZE + BLOCK_SIZE;
				break;
			}
			iWidthCurrQ = MB_SIZE;
		}
		else {
			iWidthCurrQ = BLOCK_SIZE;
			rgiBlkCurrQ = (iBlk == U_BLOCK) ? m_ppxliErrorMBU: m_ppxliErrorMBV;
		}
		if (pmbmd->getCodedBlockPattern ((BlockNum) iBlk))
			decodeTextureInterBlock (rgiBlkCurrQ, iWidthCurrQ, iQP, FALSE); //all for intra-pred
		else 
			for (Int i = 0; i < BLOCK_SIZE; i++)	{
				memset (rgiBlkCurrQ, 0, sizeof (Int) * BLOCK_SIZE);
				rgiBlkCurrQ += iWidthCurrQ;
			}
	}
}

Void CVideoObjectDecoder::decodeAlphaInterMB (CMBMode* pmbmd, PixelC *ppxlcRefMBA)
{
	assert (pmbmd != NULL);

	if (pmbmd->m_rgTranspStatus [0] == ALL)
		return;
	if(pmbmd->m_CODAlpha == ALPHA_ALL255) 
	{
		// fill curr macroblock with 255
		Int iY;
		PixelC *ppxlc = ppxlcRefMBA;
		for(iY = 0; iY<MB_SIZE; iY++, ppxlc += m_iFrameWidthY)
			pxlcmemset(ppxlc, 255, MB_SIZE);
		return;
	}
	if(pmbmd->m_CODAlpha != ALPHA_CODED)
		return;

	assert (pmbmd->m_dctMd == INTER || pmbmd->m_dctMd == INTERQ);

	Int iQP = pmbmd->m_stepSizeAlpha;
	Int* piBlkCurrQ = NULL;
	for (Int iBlk = A_BLOCK1; iBlk <= A_BLOCK4; iBlk++) {
		if (pmbmd -> m_rgTranspStatus [iBlk - 6] == ALL) 
			continue;
		switch (iBlk) 
		{
		case (A_BLOCK1): 
			piBlkCurrQ = m_ppxliErrorMBA;
			break;
		case (A_BLOCK2): 
			piBlkCurrQ = m_ppxliErrorMBA + BLOCK_SIZE;
			break;
		case (A_BLOCK3): 
			piBlkCurrQ = m_ppxliErrorMBA + MB_SIZE * BLOCK_SIZE;
			break;
		case (A_BLOCK4): 
			piBlkCurrQ = m_ppxliErrorMBA + MB_SIZE * BLOCK_SIZE + BLOCK_SIZE;
			break;
		}

		//Int *pix = piBlkCurrQ;
		Int i;
		if (pmbmd->getCodedBlockPattern ((BlockNum) iBlk))
		{
			decodeTextureInterBlock (piBlkCurrQ, MB_SIZE, iQP, TRUE);
			/*printf("\nBlock %d DCT Coefs Quant\n",iBlk);
			for(i=0;i<64;i++)
				printf("%d ",m_rgpiCoefQ [0][i]);
			printf("\n");
			
			printf("Block %d DCT Coefs Inv Quant\n",iBlk);
			for(i=0;i<64;i++)
				printf("%d ",m_rgiDCTcoef[i]);
			printf("\n");*/
		}
		else 
			for (i = 0; i < BLOCK_SIZE; i++)	{
				memset (piBlkCurrQ, 0, sizeof (Int) * BLOCK_SIZE);
				piBlkCurrQ += MB_SIZE;
			}
		/*Int j;
		printf("\nblock %d update\n",iBlk);
		for(i=0;i<BLOCK_SIZE;i++,pix+=BLOCK_SIZE){
			for(j=0;j<BLOCK_SIZE;j++,pix++)
				printf("%3d ",*pix);
			printf("\n");
		}*/
	}
}

Void CVideoObjectDecoder::setCBPYandC (CMBMode* pmbmd, Int iCBPC, Int iCBPY, Int cNonTrnspBlk)
{
	pmbmd->setCodedBlockPattern (U_BLOCK, (iCBPC >> 1) & 1) ;
	pmbmd->setCodedBlockPattern (V_BLOCK, iCBPC & 1) ;
	Int iBitPos = 1, iBlk;
	for (iBlk = Y_BLOCK1; iBlk <= Y_BLOCK4; iBlk++)	{
		if (pmbmd->m_rgTranspStatus [iBlk] != ALL)	{
			pmbmd->setCodedBlockPattern (
				(BlockNum) iBlk, 
				(iCBPY >> (cNonTrnspBlk - iBitPos)) & 1
			);
			iBitPos++;
		}
		else
			pmbmd->setCodedBlockPattern ((BlockNum) iBlk, 0);
	}
}

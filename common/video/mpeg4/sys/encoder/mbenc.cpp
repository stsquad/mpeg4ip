/*************************************************************************

This software module was originally developed by 

	Ming-Chieh Lee (mingcl@microsoft.com), Microsoft Corporation
	Wei-ge Chen (wchen@microsoft.com), Microsoft Corporation
	Simon Winder (swinder@microsoft.com), Microsoft Corporation
	(date: June, 1997)
and edited by
        Wei Wu (weiwu@stallion.risc.rockwell.com) Rockwell Science Center

and also edited by
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

	MBenc.cpp

Abstract:

	MacroBlock encoder

Revision History:

	This software module was edited by

		Hiroyuki Katata (katata@imgsl.mkhar.sharp.co.jp), Sharp Corporation
		Norio Ito (norio@imgsl.mkhar.sharp.co.jp), Sharp Corporation
		Shuichi Watanabe (watanabe@imgsl.mkhar.sharp.co.jp), Sharp Corporation
		(date: October, 1997)

	for object based temporal scalability.

	Dec 20, 1997:	Interlaced tools added by NextLevel Systems X.Chen, B. Eifrig
        May. 9   1998:  add boundary by Hyundai Electronics 
                                  Cheol-Soo Park (cspark@super5.hyundai.co.kr) 
	May 9, 1999:	tm5 rate control by DemoGraFX, duhoff@mediaone.net

NOTE:
	
	m_pvopfCurrQ holds the original data until it is texture quantized

*************************************************************************/

#include <stdlib.h>
#include <math.h>
#include <iostream.h>
#include "typeapi.h"
#include "codehead.h"
#include "mode.hpp"
#include "global.hpp"
#include "entropy/bitstrm.hpp"
#include "entropy/entropy.hpp"
#include "entropy/huffman.hpp"
#include "vopses.hpp"
#include "vopseenc.hpp"

#ifdef __MFC_
#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

#define new DEBUG_NEW				   
#endif // __MFC_

/*Void CVideoObjectEncoder::encodePVOPMBWithShape (
	PixelC* ppxlcRefMBY, PixelC* ppxlcRefMBU, PixelC* ppxlcRefMBV,
	PixelC* ppxlcRefMBA, PixelC* ppxlcRefBY,
	CMBMode* pmbmd, const CMotionVector* pmv, CMotionVector* pmvBY,
	ShapeMode shpmdColocatedMB,
	Int imbX, Int imbY,
	CoordI x, CoordI y, Int& iQPPrev)
{
	encodePVOPMBJustShape(ppxlcRefBY,pmbmd,shpmdColocatedMB,pmv,pmvBY,x,y,imbX,imbY);
	dumpCachedShapeBits();
	encodePVOPMBTextureWithShape(ppxlcRefMBY,ppxlcRefMBU,ppxlcRefMBV,ppxlcRefMBA,pmbmd,
		pmv,imbX,imbY,x,y,iQPPrev);
}*/

Void CVideoObjectEncoder::encodePVOPMBJustShape(
	PixelC* ppxlcRefBY, CMBMode* pmbmd, ShapeMode shpmdColocatedMB,
		const CMotionVector* pmv, CMotionVector* pmvBY,
		CoordI x, CoordI y, Int imbX, Int imbY)
{
	m_statsMB.nBitsShape += codeInterShape (
		ppxlcRefBY, m_pvopcRefQ0, pmbmd, shpmdColocatedMB,
		pmv, pmvBY, x, y, imbX, imbY
	);

	// change pmbmd to inter if all transparent
	decideTransparencyStatus (pmbmd, m_ppxlcCurrMBBY);
}

Void CVideoObjectEncoder::dumpCachedShapeBits()
{
#ifdef __TRACE_AND_STATS_
	m_pbitstrmOut->trace("INSERTING PRE-ENCODED MB SHAPE STREAM HERE\n");
	m_pbitstrmOut->trace(m_pbitstrmOut->getCounter(),"Location Before");
#endif // __TRACE_AND_STATS_
	m_pbitstrmOut->putBitStream(*m_pbitstrmShapeMBOut);
#ifdef __TRACE_AND_STATS_
	m_pbitstrmOut->trace(m_pbitstrmOut->getCounter(),"Location After");
#endif // __TRACE_AND_STATS_
	m_pbitstrmShapeMBOut->flush();
	m_pbitstrmShapeMBOut->resetAll();
}

Void CVideoObjectEncoder::encodePVOPMBTextureWithShape(
	PixelC* ppxlcRefMBY, PixelC* ppxlcRefMBU, PixelC* ppxlcRefMBV,
	PixelC* ppxlcRefMBA, CMBMode* pmbmd, const CMotionVector* pmv,
	Int imbX, Int imbY,	CoordI x, CoordI y, Int& iQPPrev,
	Int &iQPPrevAlpha, Bool &bUseNewQPForVlcThr)
{
	// update quantiser
	pmbmd->m_stepSize = iQPPrev + pmbmd->m_intStepDelta;
	if(bUseNewQPForVlcThr)
		pmbmd->m_stepSizeDelayed = pmbmd->m_stepSize;
	else
		pmbmd->m_stepSizeDelayed = iQPPrev;
	iQPPrev = pmbmd->m_stepSize;

	if (pmbmd -> m_dctMd == INTRA || pmbmd -> m_dctMd == INTRAQ) {
		if(pmbmd -> m_rgTranspStatus [0] == PARTIAL)
			LPEPadding (pmbmd);

		if (m_volmd.fAUsage == EIGHT_BIT) {
			// update alpha quant
			if(!m_volmd.bNoGrayQuantUpdate)
			{
				iQPPrevAlpha = (iQPPrev * m_vopmd.intStepPAlpha) / m_vopmd.intStep;
				if(iQPPrevAlpha<1)
					iQPPrevAlpha=1;
			}
			pmbmd->m_stepSizeAlpha = iQPPrevAlpha;
		}

		assert (pmbmd -> m_rgTranspStatus [0] != ALL);
		/*BBM// Added for Boundary by Hyundai(1998-5-9)
                if (m_vopmd.bInterlace && pmbmd -> m_rgTranspStatus [0] == PARTIAL)
                        boundaryMacroBlockMerge (pmbmd);
		// End of Hyundai(1998-5-9)*/
		quantizeTextureIntraMB (imbX, imbY, pmbmd, ppxlcRefMBY, ppxlcRefMBU, ppxlcRefMBV, ppxlcRefMBA);
		codeMBTextureHeadOfPVOP (pmbmd);
		sendDCTCoefOfIntraMBTexture (pmbmd);
		if (m_volmd.fAUsage == EIGHT_BIT) {
			codeMBAlphaHeadOfPVOP (pmbmd);
			sendDCTCoefOfIntraMBAlpha (pmbmd);
		}
		/*BBM// Added for Boundary by Hyundai(1998-5-9)
                if (m_vopmd.bInterlace && pmbmd -> m_bMerged[0])
                        mergedMacroBlockSplit (pmbmd, ppxlcRefMBY, ppxlcRefMBA);
		// End of Hyundai(1998-5-9)*/

		bUseNewQPForVlcThr = FALSE;
	}
	else { // INTER or skipped
		if (m_volmd.fAUsage == EIGHT_BIT) {
			// update alpha quant
			if(!m_volmd.bNoGrayQuantUpdate)
				iQPPrevAlpha = (iQPPrev * m_vopmd.intStepPAlpha) / m_vopmd.intStep;
			pmbmd->m_stepSizeAlpha = iQPPrevAlpha;
		}
		if (pmbmd -> m_rgTranspStatus [0] == PARTIAL) {
			CoordI xRefUV, yRefUV;

// INTERLACE
		// new changes 
			if(pmbmd->m_bFieldMV) {
				CoordI xRefUV1, yRefUV1;
				mvLookupUV (pmbmd, pmv, xRefUV, yRefUV, xRefUV1, yRefUV1);
				motionCompFieldUV(m_ppxlcPredMBU, m_ppxlcPredMBV,
					m_pvopcRefQ0, x, y, xRefUV, yRefUV, pmbmd->m_bForwardTop);
				motionCompFieldUV(m_ppxlcPredMBU + BLOCK_SIZE, m_ppxlcPredMBV + BLOCK_SIZE,
					m_pvopcRefQ0, x, y, xRefUV1, yRefUV1, pmbmd->m_bForwardBottom);
			}
			else {
// ~INTERLACE
			mvLookupUVWithShape (pmbmd, pmv, xRefUV, yRefUV);
			motionCompUV (m_ppxlcPredMBU, m_ppxlcPredMBV, m_pvopcRefQ0, x, y,
				xRefUV, yRefUV, m_vopmd.iRoundingControl,&m_rctRefVOPY0);
			}
			motionCompMBYEnc (pmv, pmbmd, imbX, imbY, x, y, &m_rctRefVOPY0);
			computeTextureErrorWithShape ();
		}
		else {
			// not partial
			CoordI xRefUV, yRefUV, xRefUV1, yRefUV1;
			mvLookupUV (pmbmd, pmv, xRefUV, yRefUV, xRefUV1, yRefUV1);
// INTERLACE
		// new changes 
			if(pmbmd->m_bFieldMV) {
				motionCompFieldUV(m_ppxlcPredMBU, m_ppxlcPredMBV,
					m_pvopcRefQ0, x, y, xRefUV, yRefUV, pmbmd->m_bForwardTop);
				motionCompFieldUV(m_ppxlcPredMBU + BLOCK_SIZE, m_ppxlcPredMBV + BLOCK_SIZE,
					m_pvopcRefQ0, x, y, xRefUV1, yRefUV1, pmbmd->m_bForwardBottom);
			}
			else {
// ~INTERLACE
			motionCompUV (m_ppxlcPredMBU, m_ppxlcPredMBV, m_pvopcRefQ0, x, y,
				xRefUV, yRefUV, m_vopmd.iRoundingControl, &m_rctRefVOPY0);
			}
			motionCompMBYEnc (pmv, pmbmd, imbX, imbY, x, y, &m_rctRefVOPY0);
			computeTextureError ();
		}
		Bool bSkip = pmbmd->m_bhas4MVForward ? (pmv [1].isZero () && pmv [2].isZero () && pmv [3].isZero () && pmv [4].isZero ())
			: (!pmbmd->m_bFieldMV && pmv->isZero()) ;
		/*BBM// Added for Boundary by Hyundai(1998-5-9)
		if (m_vopmd.bInterlace && pmbmd -> m_rgTranspStatus [0] == PARTIAL)
                        boundaryMacroBlockMerge (pmbmd);
		// End of Hyundai(1998-5-9)*/

		if(!m_volmd.bAllowSkippedPMBs)
			bSkip = FALSE;
		quantizeTextureInterMB (pmbmd, pmv, ppxlcRefMBA, bSkip); // decide COD here
		codeMBTextureHeadOfPVOP (pmbmd);
		if (!pmbmd -> m_bSkip) {
			/*BBM// Added for Boundary by Hyundai(1998-5-9)
            if (m_vopmd.bInterlace && pmbmd -> m_bMerged[0])
                    swapTransparentModes (pmbmd, BBS);
			// End of Hyundai(1998-5-9)*/
			m_statsMB.nBitsMV += encodeMVWithShape (pmv, pmbmd, imbX, imbY);
			/*BBM// Added for Boundary by Hyundai(1998-5-9)
			if (m_vopmd.bInterlace && pmbmd -> m_bMerged[0])
					swapTransparentModes (pmbmd, BBM);
			// End of Hyundai(1998-5-9)*/
			sendDCTCoefOfInterMBTexture (pmbmd);
			// addErrorAndPredToCurrQ (ppxlcRefMBY, ppxlcRefMBU, ppxlcRefMBV); // delete by Hyundai, ok swinder
		}
		else
			assignPredToCurrQ (ppxlcRefMBY, ppxlcRefMBU, ppxlcRefMBV);
		if (m_volmd.fAUsage == EIGHT_BIT) {
			codeMBAlphaHeadOfPVOP (pmbmd);
			if (pmbmd -> m_CODAlpha == ALPHA_CODED) {
				sendDCTCoefOfInterMBAlpha (pmbmd);
				// addAlphaErrorAndPredToCurrQ (ppxlcRefMBA); // delete by Hyundai. ok swinder
			}
			else if(pmbmd -> m_CODAlpha == ALPHA_SKIPPED)
				assignAlphaPredToCurrQ (ppxlcRefMBA);
		}
		/*BBM// Added for Boundary by Hyundai(1998-5-9)
                if (m_vopmd.bInterlace && pmbmd -> m_bMerged[0])
                        mergedMacroBlockSplit (pmbmd);
		// End of Hyundai(1998-5-9)*/
        if (!pmbmd -> m_bSkip)
		{
			bUseNewQPForVlcThr = FALSE;
            addErrorAndPredToCurrQ (ppxlcRefMBY, ppxlcRefMBU, ppxlcRefMBV);
		}
        if (m_volmd.fAUsage == EIGHT_BIT && pmbmd -> m_CODAlpha == ALPHA_CODED)
            addAlphaErrorAndPredToCurrQ (ppxlcRefMBA);
	}
}

Void CVideoObjectEncoder::encodePVOPMB (
	PixelC* ppxlcRefMBY, PixelC* ppxlcRefMBU, PixelC* ppxlcRefMBV,
	CMBMode* pmbmd, const CMotionVector* pmv,
	Int iMBX, Int iMBY,
	CoordI x, CoordI y
)
{
	// For Sprite update macroblock there is no motion compensation 
	if (m_uiSprite == 1 && m_vopmd.SpriteXmitMode != STOP) {
		if ( m_bSptMB_NOT_HOLE ) {
			m_iNumSptMB++ ;	   	 
			CopyCurrQToPred(ppxlcRefMBY, ppxlcRefMBU, ppxlcRefMBV);
			computeTextureError ();
			Bool bSkip = pmbmd->m_bhas4MVForward ? (pmv [1].isZero () && pmv [2].isZero () && pmv [3].isZero () && pmv [4].isZero ())
											 : pmv->isZero ();
			quantizeTextureInterMB (pmbmd, pmv, NULL, bSkip); // decide COD here
			codeMBTextureHeadOfPVOP (pmbmd);
			if (!pmbmd -> m_bSkip) {
				sendDCTCoefOfInterMBTexture (pmbmd);
				addErrorAndPredToCurrQ (ppxlcRefMBY, ppxlcRefMBU, ppxlcRefMBV);
			}
		}
		else {
			pmbmd -> m_bSkip = TRUE;
			codeMBTextureHeadOfPVOP (pmbmd);
		}
		return;
	}

	if (pmbmd->m_dctMd == INTRA || pmbmd->m_dctMd == INTRAQ) {
		pmbmd->m_bSkip = FALSE;			//in case use by direct mode in the future
		quantizeTextureIntraMB (iMBX, iMBY, pmbmd, ppxlcRefMBY, ppxlcRefMBU, ppxlcRefMBV, NULL);
		codeMBTextureHeadOfPVOP (pmbmd);
		sendDCTCoefOfIntraMBTexture (pmbmd);
	}
	else {
		CoordI xRefUV, yRefUV, xRefUV1, yRefUV1;
		mvLookupUV (pmbmd, pmv, xRefUV, yRefUV, xRefUV1, yRefUV1);
// INTERLACE
		// pmbmd->m_rgTranspStatus[0] = NONE;			// This a rectangular VOP 
		if(pmbmd->m_bFieldMV) {
			motionCompFieldUV(m_ppxlcPredMBU, m_ppxlcPredMBV,
				m_pvopcRefQ0, x, y, xRefUV, yRefUV, pmbmd->m_bForwardTop);
			motionCompFieldUV(m_ppxlcPredMBU + BLOCK_SIZE, m_ppxlcPredMBV + BLOCK_SIZE,
				m_pvopcRefQ0, x, y, xRefUV1, yRefUV1, pmbmd->m_bForwardBottom);
		}
		else 
// ~INTERLACE
			motionCompUV (m_ppxlcPredMBU, m_ppxlcPredMBV, m_pvopcRefQ0, x, y,
				xRefUV, yRefUV, m_vopmd.iRoundingControl,&m_rctRefVOPY0);
		motionCompMBYEnc (pmv, pmbmd, iMBX, iMBY, x, y, &m_rctRefVOPY0);
		computeTextureError ();
		Bool bSkip = pmbmd->m_bhas4MVForward ? (pmv [1].isZero () && pmv [2].isZero () && pmv [3].isZero () && pmv [4].isZero ())
            : (!pmbmd->m_bFieldMV && pmv->isZero());
		if(!m_volmd.bAllowSkippedPMBs)
			bSkip = FALSE;
		quantizeTextureInterMB (pmbmd, pmv, NULL, bSkip); // decide COD here
		codeMBTextureHeadOfPVOP (pmbmd);
		if (!pmbmd -> m_bSkip) {
			if(!(m_volmd.volType == ENHN_LAYER && m_vopmd.iRefSelectCode == 3))
				m_statsMB.nBitsMV += encodeMVVP (pmv, pmbmd, iMBX, iMBY);
			sendDCTCoefOfInterMBTexture (pmbmd);
			addErrorAndPredToCurrQ (ppxlcRefMBY, ppxlcRefMBU, ppxlcRefMBV);
		}
		else {
			assignPredToCurrQ (ppxlcRefMBY, ppxlcRefMBU, ppxlcRefMBV);
		}
	}
}


Void CVideoObjectEncoder::encodeBVOPMB (
	PixelC* ppxlcCurrQMBY, PixelC* ppxlcCurrQMBU, PixelC* ppxlcCurrQMBV,
	CMBMode* pmbmd, const CMotionVector* pmv, const CMotionVector* pmvBackward,
	const CMBMode* pmbmdRef, const CMotionVector* pmvRef,
	Int iMBX, Int iMBY,
	CoordI x, CoordI y
)
{
	motionCompAndDiff_BVOP_MB (pmv, pmvBackward, pmbmd, x, y, &m_rctRefVOPY0, &m_rctRefVOPY1);
	Bool bSkip;
	if (m_volmd.volType == ENHN_LAYER && m_vopmd.iRefSelectCode == 0 &&
	    pmbmd->m_mbType == FORWARD)
		bSkip = (pmv->m_vctTrueHalfPel.x ==0 && pmv->m_vctTrueHalfPel.y == 0);
	else if (pmbmd->m_mbType == DIRECT)
		bSkip = (pmbmd->m_vctDirectDeltaMV.x ==0 && pmbmd->m_vctDirectDeltaMV.y == 0);
	else 
		bSkip = FALSE;
	quantizeTextureInterMB (pmbmd, pmv, NULL, bSkip); // decide COD here; skip not allowed in non-direct mode
	codeMBTextureHeadOfBVOP (pmbmd);
	if (!pmbmd->m_bSkip) {
		m_statsMB.nBitsMV += encodeMVofBVOP (pmv, pmvBackward, pmbmd, iMBX, iMBY, pmvRef, pmbmdRef);
		sendDCTCoefOfInterMBTexture (pmbmd);
		addErrorAndPredToCurrQ (ppxlcCurrQMBY, ppxlcCurrQMBU, ppxlcCurrQMBV);
	}
	else
		assignPredToCurrQ (ppxlcCurrQMBY, ppxlcCurrQMBU, ppxlcCurrQMBV);
}

Void CVideoObjectEncoder::encodeBVOPMB_WithShape (
	PixelC* ppxlcCurrQMBY, PixelC* ppxlcCurrQMBU, PixelC* ppxlcCurrQMBV, PixelC* ppxlcCurrQMBA, PixelC* ppxlcCurrQBY,
	CMBMode* pmbmd, const CMotionVector* pmv, const CMotionVector* pmvBackward, 
	CMotionVector* pmvBY, ShapeMode shpmdColocatedMB,
	const CMBMode* pmbmdRef, const CMotionVector* pmvRef,
	Int iMBX, Int iMBY,
	CoordI x, CoordI y, Int &iQPPrev, Int &iQPPrevAlpha
)
{
	pmbmd->m_stepSize = iQPPrev;

	if(!m_volmd.bNoGrayQuantUpdate)
	{
		iQPPrevAlpha = (iQPPrev * m_vopmd.intStepBAlpha) / m_vopmd.intStepB;
		if(iQPPrevAlpha<1)
			iQPPrevAlpha=1;
	}
	pmbmd->m_stepSizeAlpha = iQPPrevAlpha;

	m_statsMB.nBitsShape += codeInterShape (
		ppxlcCurrQBY, 
		m_vopmd.fShapeBPredDir==B_FORWARD ? m_pvopcRefQ0 : m_pvopcRefQ1,
		pmbmd, shpmdColocatedMB,
		NULL, pmvBY, 
		x, y, 
		iMBX, iMBY
	);
	downSampleBY (m_ppxlcCurrMBBY, m_ppxlcCurrMBBUV);
	decideTransparencyStatus (pmbmd, m_ppxlcCurrMBBY); // change pmbmd to inter if all transparent
	/*BBM// Added for Boundary by Hyundai(1998-5-9)
	if (m_vopmd.bInterlace) initMergedMode (pmbmd);
	// End of Hyundai(1998-5-9)*/
	if (m_volmd.bShapeOnly == FALSE && pmbmd->m_rgTranspStatus [0] != ALL) {
		if (pmbmd->m_rgTranspStatus [0] == PARTIAL)
			motionCompAndDiff_BVOP_MB_WithShape (pmv, pmvBackward, pmbmd, x, y, &m_rctRefVOPY0, &m_rctRefVOPY1);
		else
			motionCompAndDiff_BVOP_MB (pmv, pmvBackward, pmbmd, x, y, &m_rctRefVOPY0, &m_rctRefVOPY1);
		Bool bSkip = FALSE;
		if (pmbmd->m_mbType == DIRECT)	{
			if(pmvRef == NULL)	//just to be safe
				pmbmd->m_vctDirectDeltaMV = pmv->m_vctTrueHalfPel;
			bSkip = (pmbmd->m_vctDirectDeltaMV.x == 0) && (pmbmd->m_vctDirectDeltaMV.y == 0);
		}
		if(m_volmd.fAUsage == EIGHT_BIT)
			motionCompAndDiffAlpha_BVOP_MB (
				pmv,
				pmvBackward, 
				pmbmd, 
				x, y,
				&m_rctRefVOPY0, &m_rctRefVOPY1
			);
		/*BBM// Added for Boundary by Hyundai(1998-5-9)
		if (m_vopmd.bInterlace && pmbmd -> m_rgTranspStatus [0] == PARTIAL)
                        boundaryMacroBlockMerge (pmbmd);
		// End of Hyundai(1998-5-9)*/
		quantizeTextureInterMB (pmbmd, pmv, ppxlcCurrQMBA, bSkip); // decide COD here; skip not allowed in non-direct mode
		codeMBTextureHeadOfBVOP (pmbmd);
		if (!pmbmd -> m_bSkip) {
			m_statsMB.nBitsMV += encodeMVofBVOP (pmv, pmvBackward, pmbmd,
				iMBX, iMBY, pmvRef, pmbmdRef);	// need to see the MB type to decide what MV to be sent
			sendDCTCoefOfInterMBTexture (pmbmd);
			//addErrorAndPredToCurrQ (ppxlcCurrQMBY, ppxlcCurrQMBU, ppxlcCurrQMBV); // delete by Hyundai, ok swinder
		}
		else
			assignPredToCurrQ (ppxlcCurrQMBY, ppxlcCurrQMBU, ppxlcCurrQMBV);
		if (m_volmd.fAUsage == EIGHT_BIT) {
			codeMBAlphaHeadOfBVOP (pmbmd);
			if (pmbmd -> m_CODAlpha == ALPHA_CODED) {
				sendDCTCoefOfInterMBAlpha (pmbmd);
				//addAlphaErrorAndPredToCurrQ (ppxlcCurrQMBA); // delete by Hyundai, ok swinder
			}
			else if(pmbmd -> m_CODAlpha == ALPHA_SKIPPED)
				assignAlphaPredToCurrQ (ppxlcCurrQMBA);
		}
		/*BBM// Added for Boundary by Hyundai(1998-5-9)
                if (m_vopmd.bInterlace && pmbmd -> m_bMerged[0])
                        mergedMacroBlockSplit (pmbmd);
		// End of Hyundai(1998-5-9)*/
        if (!pmbmd -> m_bSkip)
			addErrorAndPredToCurrQ (ppxlcCurrQMBY, ppxlcCurrQMBU, ppxlcCurrQMBV);
        if (m_volmd.fAUsage == EIGHT_BIT && pmbmd -> m_CODAlpha == ALPHA_CODED)
		    addAlphaErrorAndPredToCurrQ (ppxlcCurrQMBA);
	}
}

Void CVideoObjectEncoder::codeMBTextureHeadOfIVOP (const CMBMode* pmbmd)
{
	UInt CBPC = (pmbmd->getCodedBlockPattern (U_BLOCK) << 1)
				| pmbmd->getCodedBlockPattern (V_BLOCK);	
														//per defintion of H.263's CBPC 
	assert (CBPC >= 0 && CBPC <= 3);
	Int CBPY = 0;
	UInt cNonTrnspBlk = 0, iBlk;
	for (iBlk = (UInt) Y_BLOCK1; iBlk <= (UInt) Y_BLOCK4; iBlk++) {
		if (pmbmd->m_rgTranspStatus [iBlk] != ALL)	
			cNonTrnspBlk++;
	}
	UInt iBitPos = 1;
	for (iBlk = (UInt) Y_BLOCK1; iBlk <= (UInt) Y_BLOCK4; iBlk++)	{
		if (pmbmd->m_rgTranspStatus [iBlk] != ALL)	{
			CBPY |= pmbmd->getCodedBlockPattern ((BlockNum) iBlk) << (cNonTrnspBlk - iBitPos);
			iBitPos++;
		}
	}
	assert (CBPY >= 0 && CBPY <= 15);								//per defintion of H.263's CBPY 

	Int iSymbol = 4 * pmbmd->m_dctMd + CBPC;
	assert (iSymbol >= 0 && iSymbol <= 7);			//send MCBPC
#ifdef __TRACE_AND_STATS_
	m_pbitstrmOut->trace (iSymbol, "MB_MCBPC");
#endif // __TRACE_AND_STATS_
	m_statsMB.nBitsMCBPC += m_pentrencSet->m_pentrencMCBPCintra->encodeSymbol (iSymbol, "MB_MCBPC");
	//fprintf(stderr,"MCBPC = %d\n",iSymbol);

	m_pbitstrmOut->putBits (pmbmd->m_bACPrediction, 1, "MB_ACPRED");
	m_statsMB.nBitsIntraPred++;
	//fprintf(stderr,"ACPred = %d\n",pmbmd->m_bACPrediction);

#ifdef __TRACE_AND_STATS_
	m_pbitstrmOut->trace (cNonTrnspBlk, "MB_NumNonTranspBlks");
	m_pbitstrmOut->trace (CBPY, "MB_CBPY (I-style)");
#endif // __TRACE_AND_STATS_
	switch (cNonTrnspBlk) {
	case 1:
		m_statsMB.nBitsCBPY += m_pentrencSet->m_pentrencCBPY1->encodeSymbol (CBPY, "MB_CBPY");
		break;
	case 2:
		m_statsMB.nBitsCBPY += m_pentrencSet->m_pentrencCBPY2->encodeSymbol (CBPY, "MB_CBPY");
		break;
	case 3:
		m_statsMB.nBitsCBPY += m_pentrencSet->m_pentrencCBPY3->encodeSymbol (CBPY, "MB_CBPY");
		break;
	case 4:
		m_statsMB.nBitsCBPY += m_pentrencSet->m_pentrencCBPY->encodeSymbol (CBPY, "MB_CBPY");
		break;
	default:
		assert (FALSE);
	}
	//fprintf(stderr,"CBPY = %d\n",CBPY);
	m_statsMB.nIntraMB++;
	if (pmbmd->m_dctMd == INTRAQ)	{
		Int DQUANT = pmbmd->m_intStepDelta;			//send DQUANT
		assert (DQUANT >= -2 && DQUANT <= 2);
		if (DQUANT != 0)	{	
			if (sign (DQUANT) == 1)
				m_pbitstrmOut->putBits (DQUANT + 1, 2, "MB_DQUANT");
			else
				m_pbitstrmOut->putBits (-1 - DQUANT, 2, "MB_DQUANT");
			m_statsMB.nBitsDQUANT += 2;
		}
	}
// INTERLACE
	if(m_vopmd.bInterlace==TRUE) {
		m_pbitstrmOut->putBits (pmbmd->m_bFieldDCT, 1, "DCT_TYPE"); // send dct_type
		m_statsMB.nBitsInterlace += 1;
	}
// ~INTERLACE
}

Void CVideoObjectEncoder::codeMBTextureHeadOfPVOP (const CMBMode* pmbmd)
{
	UInt CBPC = (pmbmd->getCodedBlockPattern (U_BLOCK) << 1)
				| pmbmd->getCodedBlockPattern (V_BLOCK);	
														//per defintion of H.263's CBPC 
	assert (CBPC >= 0 && CBPC <= 3);
	Int CBPY = 0;
	UInt cNonTrnspBlk = 0, iBlk;
	for (iBlk = (UInt) Y_BLOCK1; iBlk <= (UInt) Y_BLOCK4; iBlk++) {
		if (pmbmd->m_rgTranspStatus [iBlk] != ALL)	
			cNonTrnspBlk++;
	}
	UInt iBitPos = 1;
	for (iBlk = (UInt) Y_BLOCK1; iBlk <= (UInt) Y_BLOCK4; iBlk++)	{
		if (pmbmd->m_rgTranspStatus [iBlk] != ALL)	{
			CBPY |= pmbmd->getCodedBlockPattern ((BlockNum) iBlk) << (cNonTrnspBlk - iBitPos);
			iBitPos++;
		}
	}
	assert (CBPY >= 0 && CBPY <= 15);								//per defintion of H.263's CBPY 
	m_pbitstrmOut->putBits (pmbmd->m_bSkip, 1, "MB_Skip");
	m_statsMB.nBitsCOD++;
	if (pmbmd->m_bSkip)
		m_statsMB.nSkipMB++;
	else {
		Int iMBtype;								//per H.263's MBtype
		if (pmbmd->m_dctMd == INTRA || pmbmd->m_dctMd == INTRAQ)
			iMBtype = pmbmd->m_dctMd + 3;
		else
			iMBtype = (pmbmd -> m_dctMd - 2) | pmbmd -> m_bhas4MVForward << 1;
		assert (iMBtype >= 0 && iMBtype <= 4);
#ifdef __TRACE_AND_STATS_
		m_pbitstrmOut->trace (iMBtype, "MB_MBtype");
		m_pbitstrmOut->trace (CBPC, "MB_CBPC");
#endif // __TRACE_AND_STATS_
		m_statsMB.nBitsMCBPC += m_pentrencSet->m_pentrencMCBPCinter->encodeSymbol (iMBtype * 4 + CBPC, "MCBPC");
		if (pmbmd->m_dctMd == INTRA || pmbmd->m_dctMd == INTRAQ)	{
			m_pbitstrmOut->putBits (pmbmd->m_bACPrediction, 1, "MB_ACPRED");
			m_statsMB.nBitsIntraPred++;
#ifdef __TRACE_AND_STATS_
			m_pbitstrmOut->trace (cNonTrnspBlk, "MB_NumNonTranspBlks");
			m_pbitstrmOut->trace (CBPY, "MB_CBPY (I-style)");
#endif // __TRACE_AND_STATS_
			switch (cNonTrnspBlk) {
			case 1:
				m_statsMB.nBitsCBPY += m_pentrencSet->m_pentrencCBPY1->encodeSymbol (CBPY, "MB_CBPY");
				break;
			case 2:
				m_statsMB.nBitsCBPY += m_pentrencSet->m_pentrencCBPY2->encodeSymbol (CBPY, "MB_CBPY");
				break;
			case 3:
				m_statsMB.nBitsCBPY += m_pentrencSet->m_pentrencCBPY3->encodeSymbol (CBPY, "MB_CBPY");
				break;
			case 4:
				m_statsMB.nBitsCBPY += m_pentrencSet->m_pentrencCBPY->encodeSymbol (CBPY, "MB_CBPY");
				break;
			default:
				assert (FALSE);
			}
			m_statsMB.nIntraMB++;
		}
		else {
#ifdef __TRACE_AND_STATS_
			m_pbitstrmOut->trace (cNonTrnspBlk, "MB_NumNonTranspBlks");
			m_pbitstrmOut->trace (CBPY, "MB_CBPY (P-style)");
#endif // __TRACE_AND_STATS_
			switch (cNonTrnspBlk) {
			case 1:
				m_statsMB.nBitsCBPY += m_pentrencSet->m_pentrencCBPY1->encodeSymbol (1 - CBPY, "MB_CBPY");
				break;
			case 2:
				m_statsMB.nBitsCBPY += m_pentrencSet->m_pentrencCBPY2->encodeSymbol (3 - CBPY, "MB_CBPY");
				break;
			case 3:
				m_statsMB.nBitsCBPY += m_pentrencSet->m_pentrencCBPY3->encodeSymbol (7 - CBPY, "MB_CBPY");
				break;
			case 4:
				m_statsMB.nBitsCBPY += m_pentrencSet->m_pentrencCBPY->encodeSymbol (15 - CBPY, "MB_CBPY");
				break;
			default:
				assert (FALSE);
			}
			if(pmbmd ->m_bhas4MVForward)
				m_statsMB.nInter4VMB++;
// INTERLACE
			else if (pmbmd-> m_bFieldMV)
				m_statsMB.nFieldForwardMB++;
// ~INTERLACE
			else
				m_statsMB.nInterMB++;
		}
		if (pmbmd->m_dctMd == INTERQ || pmbmd->m_dctMd == INTRAQ) {
			Int DQUANT = pmbmd->m_intStepDelta;			//send DQUANT
			assert (DQUANT >= -2 && DQUANT <= 2);
			if (DQUANT != 0) {	
				if (sign (DQUANT) == 1)
					m_pbitstrmOut->putBits (DQUANT + 1, 2, "MB_DQUANT");
				else
					m_pbitstrmOut->putBits (-1 - DQUANT, 2, "MB_DQUANT");
				m_statsMB.nBitsDQUANT += 2;
			}
		}
// INTERLACE
		if (m_vopmd.bInterlace == TRUE) {
			if((pmbmd->m_dctMd == INTRA) || (pmbmd->m_dctMd == INTRAQ) || (CBPC || CBPY)) {
				m_pbitstrmOut->putBits (pmbmd->m_bFieldDCT, 1, "DCT_Type");
				m_statsMB.nBitsInterlace += 1;
			}
			if((pmbmd->m_dctMd == INTER || pmbmd->m_dctMd == INTERQ )&&(pmbmd -> m_bhas4MVForward == FALSE)) {
				m_pbitstrmOut->putBits (pmbmd->m_bFieldMV, 1, "Field_Prediction");
				m_statsMB.nBitsInterlace += 1;
				if(pmbmd->m_bFieldMV) {
					m_pbitstrmOut->putBits (pmbmd->m_bForwardTop, 1, "Forward_Top_Field_Ref");
					m_pbitstrmOut->putBits (pmbmd->m_bForwardBottom, 1, "Forward_Bot_Field_Ref");
					m_statsMB.nBitsInterlace += 2;
				}
			}
		}
// ~INTERLACE	
	}
}

Void CVideoObjectEncoder::codeMBTextureHeadOfBVOP (const CMBMode* pmbmd)
{
  U8 uchCBPB = 0;
	if (pmbmd->m_bSkip) {
		if (m_volmd.volType == BASE_LAYER)	{
			assert (pmbmd -> m_rgTranspStatus [0] != ALL);
			if (pmbmd->m_bColocatedMBSkip)
				return;
		}
		m_pbitstrmOut->putBits (1, 1, "MB_MODB");
		m_statsMB.nBitsMODB++;
		m_statsMB.nSkipMB++;
		if(m_volmd.volType == ENHN_LAYER && m_vopmd.iRefSelectCode == 0)
			return;
	}
	else {
		Int iCBPC = (pmbmd->getCodedBlockPattern (U_BLOCK) << 1) | 
					 pmbmd->getCodedBlockPattern (V_BLOCK);			//per defintion of H.263's CBPC 														
		assert (iCBPC >= 0 && iCBPC <= 3);
		Int iCBPY = 0;
		Int iNumNonTrnspBlk = 0, iBlk;
		for (iBlk = Y_BLOCK1; iBlk <= Y_BLOCK4; iBlk++) 
			iNumNonTrnspBlk += (pmbmd->m_rgTranspStatus [iBlk] != ALL);
		Int iBitPos = 1;
		for (iBlk = Y_BLOCK1; iBlk <= Y_BLOCK4; iBlk++)	{
			if (pmbmd->m_rgTranspStatus [iBlk] != ALL)	{
				iCBPY |= pmbmd->getCodedBlockPattern ((BlockNum) iBlk) << (iNumNonTrnspBlk - iBitPos);
				iBitPos++;
			}
		}
		assert (iCBPY >= 0 && iCBPY <= 15);								//per defintion of H.263's CBPY 
		uchCBPB = (iCBPY << 2 | iCBPC) & 0x3F;

		if (uchCBPB == 0)	{
			m_pbitstrmOut->putBits (1, 2, "MB_MODB");
			m_statsMB.nBitsMODB += 2;
			if (m_volmd.volType == BASE_LAYER || (m_volmd.volType == ENHN_LAYER && m_vopmd.iRefSelectCode != 0))
				m_statsMB.nBitsMBTYPE += m_pentrencSet->m_pentrencMbTypeBVOP->encodeSymbol (pmbmd->m_mbType, "MB_TYPE");
			else {
				Int iMbtypeBits =0;
				if(pmbmd->m_mbType == FORWARD)			iMbtypeBits = 1;
				else if(pmbmd->m_mbType == INTERPOLATE)	iMbtypeBits = 2;
				else if(pmbmd->m_mbType == BACKWARD)	iMbtypeBits = 3;
				Int iMbtypeValue = 1;
				m_statsMB.nBitsMBTYPE += iMbtypeBits;
				m_pbitstrmOut->putBits (iMbtypeValue, iMbtypeBits, "MB_TYPE");				
			}
		}
		else	{
			m_pbitstrmOut->putBits (0, 2,"MB_MODB");
			m_statsMB.nBitsMODB += 2;
			//m_stat.nBitsMBTYPE += m_pentrencMbTypeBVOP->encodeSymbol (pmbmd->m_mbType, "MB_TYPE");
			if (m_volmd.volType == BASE_LAYER || (m_volmd.volType == ENHN_LAYER && m_vopmd.iRefSelectCode != 0))
				m_statsMB.nBitsMBTYPE += m_pentrencSet->m_pentrencMbTypeBVOP->encodeSymbol (pmbmd->m_mbType, "MB_TYPE");
			else {
				Int iMbtypeBits =0;
				if(pmbmd->m_mbType == FORWARD)			iMbtypeBits = 1;
				else if(pmbmd->m_mbType == INTERPOLATE)	iMbtypeBits = 2;
				else if(pmbmd->m_mbType == BACKWARD)	iMbtypeBits = 3;
				Int iMbtypeValue = 1;
				m_statsMB.nBitsMBTYPE += iMbtypeBits;
				m_pbitstrmOut->putBits (iMbtypeValue, iMbtypeBits, "MB_TYPE");
			}
			m_pbitstrmOut -> putBits (uchCBPB, iNumNonTrnspBlk + 2, "MB_CBPB");
			m_statsMB.nBitsCBPB += iNumNonTrnspBlk + 2;
		}	
		switch (pmbmd->m_mbType) {
			case FORWARD:
				m_statsMB.nForwardMB++;
				break;
			case BACKWARD:
				m_statsMB.nBackwardMB++;
				break;
			case DIRECT:
				m_statsMB.nDirectMB++;
				break;
			case INTERPOLATE:
				m_statsMB.nInterpolateMB++;
				break;
			default:
				assert (FALSE);
		}
	}
	if (pmbmd->m_mbType != DIRECT)	{
		if (uchCBPB != 0)		{	//no need to send when MODB = 10
			Int DQUANT = pmbmd->m_intStepDelta;			//send DQUANT
			assert (DQUANT == -2 || DQUANT == 2 || DQUANT == 0);
			if (DQUANT == 0)	{
				m_pbitstrmOut->putBits ((Int) 0, (UInt) 1, "MB_DQUANT");
				m_statsMB.nBitsDQUANT += 1;
			}
			else {
				if (DQUANT == 2)	
					m_pbitstrmOut->putBits (3, 2, "MB_DQUANT");
				else 
					m_pbitstrmOut->putBits (2, 2, "MB_DQUANT");
				m_statsMB.nBitsDQUANT += 2;
			}
		}
	}
// INTERLACE
	if (pmbmd->m_bSkip) {
		if (m_vopmd.bInterlace && (pmbmd->m_mbType == DIRECT) && pmbmd->m_bFieldMV)
			m_statsMB.nFieldDirectMB++;
	} else if (m_vopmd.bInterlace) {
		if (uchCBPB) {
			m_pbitstrmOut->putBits((Int) pmbmd->m_bFieldDCT, 1, "DCT_TYPE");
			m_statsMB.nBitsInterlace++;
		}
		if (pmbmd->m_bFieldMV) {
			switch (pmbmd->m_mbType) {

			case FORWARD:
				m_pbitstrmOut->putBits(1, 1, "FIELD_PREDICTION");
				m_pbitstrmOut->putBits((Int)pmbmd->m_bForwardTop,    1, "FORWARD_TOP_FIELD_REFERENCE");
				m_pbitstrmOut->putBits((Int)pmbmd->m_bForwardBottom, 1, "FORWARD_BOTTOM_FIELD_REFERENCE");
				m_statsMB.nFieldForwardMB++;
				m_statsMB.nBitsInterlace += 3;
				break;

			case BACKWARD:
				m_pbitstrmOut->putBits(1, 1, "FIELD_PREDICTION");
				m_pbitstrmOut->putBits((Int)pmbmd->m_bBackwardTop,    1, "BACKWARD_TOP_FIELD_REFERENCE");
				m_pbitstrmOut->putBits((Int)pmbmd->m_bBackwardBottom, 1, "BACKWARD_BOTTOM_FIELD_REFERENCE");
				m_statsMB.nFieldBackwardMB++;
				m_statsMB.nBitsInterlace += 3;
				break;

			case INTERPOLATE:
				m_pbitstrmOut->putBits(1, 1, "FIELD_PREDICTION");
				m_pbitstrmOut->putBits((Int)pmbmd->m_bForwardTop,     1, "FORWARD_TOP_FIELD_REFERENCE");
				m_pbitstrmOut->putBits((Int)pmbmd->m_bForwardBottom,  1, "FORWARD_BOTTOM_FIELD_REFERENCE");
				m_pbitstrmOut->putBits((Int)pmbmd->m_bBackwardTop,    1, "BACKWARD_TOP_FIELD_REFERENCE");
				m_pbitstrmOut->putBits((Int)pmbmd->m_bBackwardBottom, 1, "BACKWARD_BOTTOM_FIELD_REFERENCE");
				m_statsMB.nFieldInterpolateMB++;
				m_statsMB.nBitsInterlace += 5;
				break;

			case DIRECT:
				m_statsMB.nFieldDirectMB++;
				break;
			}
		} else if (pmbmd->m_mbType != DIRECT) {
			m_pbitstrmOut->putBits(0, 1, "FIELD_PREDICTION");
			m_statsMB.nBitsInterlace++;
		}
	}
// ~INTERLACE
}

Void CVideoObjectEncoder::copyToCurrBuffWithShape (
	const PixelC* ppxlcCurrY, const PixelC* ppxlcCurrU, const PixelC* ppxlcCurrV,
	const PixelC* ppxlcCurrBY, const PixelC* ppxlcCurrA,
	Int iWidthY, Int iWidthUV
)
{
	PixelC* ppxlcCurrMBY = m_ppxlcCurrMBY;
	PixelC* ppxlcCurrMBU = m_ppxlcCurrMBU;
	PixelC* ppxlcCurrMBV = m_ppxlcCurrMBV;
	PixelC* ppxlcCurrMBBY = m_ppxlcCurrMBBY;
	Int ic;
	for (ic = 0; ic < BLOCK_SIZE; ic++) {
		memcpy (ppxlcCurrMBY, ppxlcCurrY, MB_SIZE*sizeof(PixelC)); 
		memcpy (ppxlcCurrMBU, ppxlcCurrU, BLOCK_SIZE*sizeof(PixelC));
		memcpy (ppxlcCurrMBV, ppxlcCurrV, BLOCK_SIZE*sizeof(PixelC));
		memcpy (ppxlcCurrMBBY, ppxlcCurrBY, MB_SIZE*sizeof(PixelC));
		ppxlcCurrMBY += MB_SIZE; ppxlcCurrY += iWidthY;
		ppxlcCurrMBU += BLOCK_SIZE; ppxlcCurrU += iWidthUV;
		ppxlcCurrMBV += BLOCK_SIZE;	ppxlcCurrV += iWidthUV;
		ppxlcCurrMBBY += MB_SIZE; ppxlcCurrBY += iWidthY;
		
		memcpy (ppxlcCurrMBY, ppxlcCurrY, MB_SIZE*sizeof(PixelC)); // two rows for Y
		memcpy (ppxlcCurrMBBY, ppxlcCurrBY, MB_SIZE*sizeof(PixelC));
		ppxlcCurrMBY += MB_SIZE; ppxlcCurrY += iWidthY;
		ppxlcCurrMBBY += MB_SIZE; ppxlcCurrBY += iWidthY;
	}
	if (m_volmd.fAUsage == EIGHT_BIT) {
		PixelC* ppxlcCurrMBA = m_ppxlcCurrMBA;
		for (ic = 0; ic < MB_SIZE; ic++) {
			memcpy (ppxlcCurrMBA, ppxlcCurrA, MB_SIZE*sizeof(PixelC));
			ppxlcCurrMBA += MB_SIZE; 
			ppxlcCurrA += iWidthY;
		}
	}
}

Void CVideoObjectEncoder::copyToCurrBuffJustShape(const PixelC* ppxlcCurrBY,
												  Int iWidthY)
{
	PixelC* ppxlcCurrMBBY = m_ppxlcCurrMBBY;
	Int ic;
	for (ic = 0; ic < BLOCK_SIZE; ic++) {
		memcpy (ppxlcCurrMBBY, ppxlcCurrBY, MB_SIZE*sizeof(PixelC));
		ppxlcCurrMBBY += MB_SIZE; ppxlcCurrBY += iWidthY;
		memcpy (ppxlcCurrMBBY, ppxlcCurrBY, MB_SIZE*sizeof(PixelC));
		ppxlcCurrMBBY += MB_SIZE; ppxlcCurrBY += iWidthY;
	}
}

Void CVideoObjectEncoder::copyToCurrBuff (
	const PixelC* ppxlcCurrY, const PixelC* ppxlcCurrU, const PixelC* ppxlcCurrV,
	Int iWidthY, Int iWidthUV
)
{
	PixelC* ppxlcCurrMBY = m_ppxlcCurrMBY;
	PixelC* ppxlcCurrMBU = m_ppxlcCurrMBU;
	PixelC* ppxlcCurrMBV = m_ppxlcCurrMBV;
	Int ic;
	for (ic = 0; ic < BLOCK_SIZE; ic++) {
		memcpy (ppxlcCurrMBY, ppxlcCurrY, MB_SIZE*sizeof(PixelC));
		memcpy (ppxlcCurrMBU, ppxlcCurrU, BLOCK_SIZE*sizeof(PixelC));
		memcpy (ppxlcCurrMBV, ppxlcCurrV, BLOCK_SIZE*sizeof(PixelC));
		ppxlcCurrMBY += MB_SIZE; ppxlcCurrY += iWidthY;
		ppxlcCurrMBU += BLOCK_SIZE; ppxlcCurrU += iWidthUV;
		ppxlcCurrMBV += BLOCK_SIZE;	ppxlcCurrV += iWidthUV;
		
		memcpy (ppxlcCurrMBY, ppxlcCurrY, MB_SIZE*sizeof(PixelC)); // two rows for Y
		ppxlcCurrMBY += MB_SIZE; ppxlcCurrY += iWidthY;
	}
}

// compute error signal

Void CVideoObjectEncoder::computeTextureError ()
{
	CoordI ix;
	// Y
	for (ix = 0; ix < MB_SQUARE_SIZE; ix++)
		m_ppxliErrorMBY [ix] = m_ppxlcCurrMBY [ix] - m_ppxlcPredMBY [ix];

	// UV
	for (ix = 0; ix < BLOCK_SQUARE_SIZE; ix++) {
		m_ppxliErrorMBU [ix] = m_ppxlcCurrMBU [ix] - m_ppxlcPredMBU [ix];
		m_ppxliErrorMBV [ix] = m_ppxlcCurrMBV [ix] - m_ppxlcPredMBV [ix];
	}

	// Alpha
	if(m_volmd.fAUsage==EIGHT_BIT)
		for (ix = 0; ix < MB_SQUARE_SIZE; ix++)
			m_ppxliErrorMBA [ix] = m_ppxlcCurrMBA [ix] - m_ppxlcPredMBA [ix];
}

Void CVideoObjectEncoder::computeTextureErrorWithShape ()
{
	CoordI ix;
	// Y
	for (ix = 0; ix < MB_SQUARE_SIZE; ix++) {
		if (m_ppxlcCurrMBBY [ix] == transpValue)
			m_ppxliErrorMBY [ix] = 0; // zero padding
		else
			m_ppxliErrorMBY [ix] = m_ppxlcCurrMBY [ix] - m_ppxlcPredMBY [ix];
	}

	// UV
	for (ix = 0; ix < BLOCK_SQUARE_SIZE; ix++) {
		if (m_ppxlcCurrMBBUV [ix] == transpValue)
			m_ppxliErrorMBU [ix] = m_ppxliErrorMBV [ix] = 0;
		else {
			m_ppxliErrorMBU [ix] = m_ppxlcCurrMBU [ix] - m_ppxlcPredMBU [ix];
			m_ppxliErrorMBV [ix] = m_ppxlcCurrMBV [ix] - m_ppxlcPredMBV [ix];
		}
	}

	if(m_volmd.fAUsage==EIGHT_BIT)
		for (ix = 0; ix < MB_SQUARE_SIZE; ix++) {
			if (m_ppxlcCurrMBBY [ix] == transpValue)
				m_ppxliErrorMBA [ix] = 0; // zero padding
			else
				m_ppxliErrorMBA [ix] = m_ppxlcCurrMBA [ix] - m_ppxlcPredMBA [ix];
		}
}

Void CVideoObjectEncoder::computeAlphaError ()
{
	CoordI ix;

	for (ix = 0; ix < MB_SQUARE_SIZE; ix++) {
		if (m_ppxlcCurrMBBY [ix] == transpValue)
			m_ppxliErrorMBA [ix] = 0; // zero padding
		else
			m_ppxliErrorMBA [ix] = m_ppxlcCurrMBA [ix] - m_ppxlcPredMBA [ix];
	}
}

Void CVideoObjectEncoder::quantizeTextureIntraMB (	
	Int imbX, Int imbY,
	CMBMode* pmbmd, 
	PixelC* ppxlcCurrQMBY, PixelC* ppxlcCurrQMBU, PixelC* ppxlcCurrQMBV,
	PixelC* ppxlcCurrQMBA
)
{
	assert (pmbmd != NULL);
	assert (pmbmd -> m_dctMd == INTRA || pmbmd->m_dctMd == INTRAQ);
	assert (pmbmd -> m_rgTranspStatus [0] != ALL);

	Int iQP = pmbmd->m_stepSize;
#ifdef __TRACE_AND_STATS_
	m_statsMB.nQMB++;
	m_statsMB.nQp += iQP;
#endif // __TRACE_AND_STATS_

	Int iDcScalerY, iDcScalerC, iDcScalerA = 0;
	if (iQP <= 4)
	{
		iDcScalerY = 8;
		iDcScalerC = 8;
	}
	else if (iQP >= 5 && iQP <= 8)
	{
		iDcScalerY = 2 * iQP;
		iDcScalerC = (iQP + 13) / 2;
	}
	else if (iQP >= 9 && iQP <= 24)
	{
		iDcScalerY = iQP + 8;
		iDcScalerC = (iQP + 13) / 2;
	}
	else
	{
		iDcScalerY = 2 * iQP - 16;
		iDcScalerC = iQP - 6;
	}

	Int iQPA = 0;
	pmbmd->m_CODAlpha = ALPHA_CODED;
	if(m_volmd.fAUsage == EIGHT_BIT)
	{
		if (pmbmd -> m_stepSizeAlpha < 1)
			pmbmd -> m_stepSizeAlpha = 1;
		iQPA = pmbmd->m_stepSizeAlpha;

		if (iQPA <= 4)
			iDcScalerA = 8;
		else if (iQPA >= 5 && iQPA <= 8)
			iDcScalerA = 2 * iQPA;
		else if (iQPA >= 9 && iQPA <= 24)
			iDcScalerA = iQPA + 8;
		else
			iDcScalerA = 2 * iQPA - 16;

		if(pmbmd->m_rgTranspStatus [0] == NONE)
		{
			// need to test gray alpha vals
			// CODA = 1 if all==255, can't use TranspStatus, has to be 255
			Int i;
			Int iThresh = 256 - iQPA;
			pmbmd->m_CODAlpha = ALPHA_ALL255;
			for(i = 0; i<MB_SQUARE_SIZE; i++)
				if(m_ppxlcCurrMBA[i]<=iThresh)
				{
					pmbmd->m_CODAlpha = ALPHA_CODED;
					break;
				}
			if(pmbmd->m_CODAlpha == ALPHA_ALL255)
			{
				pxlcmemset(m_ppxlcCurrMBA, 255, MB_SQUARE_SIZE);
				PixelC *ppxlc = ppxlcCurrQMBA;
				for(i = 0; i<MB_SIZE; i++, ppxlc += m_iFrameWidthY)
					pxlcmemset(ppxlc, 255, MB_SIZE);
			}
		}	
	}

	Int iCoefToStart;
	assert (pmbmd -> m_stepSizeDelayed > 0);
	if (pmbmd -> m_stepSizeDelayed >= grgiDCSwitchingThreshold [m_vopmd.iIntraDcSwitchThr])
	{
		pmbmd->m_bCodeDcAsAc = TRUE;
		//pmbmd->m_bCodeDcAsAcAlpha = TRUE; // decision should really be based on alpha quantiser
		iCoefToStart = 0;
	}
	else {
		pmbmd->m_bCodeDcAsAc = FALSE;
		//pmbmd->m_bCodeDcAsAcAlpha = FALSE;
		iCoefToStart = 1;
	}
	pmbmd->m_bCodeDcAsAcAlpha = FALSE;

	//for intra pred
	MacroBlockMemory* pmbmLeft = NULL;
	MacroBlockMemory* pmbmTop = NULL;
	MacroBlockMemory* pmbmLeftTop = NULL;
	CMBMode* pmbmdLeft = NULL;
	CMBMode* pmbmdTop = NULL;
	CMBMode* pmbmdLeftTop = NULL;											 
	Int iMBnum = imbY * m_iNumMBX + imbX;
	if (!bVPNoTop(iMBnum))	{
		pmbmTop  = m_rgpmbmAbove [imbX];
		pmbmdTop = pmbmd - m_iNumMBX;
	}
	if (!bVPNoLeft(iMBnum, imbX))	{
		pmbmLeft  = m_rgpmbmCurr [imbX - 1];
		pmbmdLeft = pmbmd -  1;
	}
	if (!bVPNoLeftTop(iMBnum, imbX))	{
		pmbmLeftTop  = m_rgpmbmAbove [imbX - 1];
		pmbmdLeftTop = pmbmd - m_iNumMBX - 1;
	}

// INTERLACE
	if((pmbmd->m_rgTranspStatus [0] == NONE)&&(m_vopmd.bInterlace == TRUE) ) {
		pmbmd->m_bFieldDCT = FrameFieldDCTDecideC(m_ppxlcCurrMBY);
		m_statsMB.nFieldDCTMB += (Int) pmbmd->m_bFieldDCT;
	}
	else
		pmbmd->m_bFieldDCT = 0;

	pmbmd->m_bSkip = FALSE;		// for direct mode reference 
// ~INTERLACE

	PixelC* rgchBlkDst = NULL;
	PixelC* rgchBlkSrc = NULL;
	Int iWidthDst, iWidthSrc;
	Int iDcScaler;
	Int* rgiCoefQ;
	Int iSumErr = 0; //sum of error to determine intra ac prediction
	Int iBlk;
	Int iBlkEnd;
	if(m_volmd.fAUsage == EIGHT_BIT)
		iBlkEnd = A_BLOCK4;
	else
		iBlkEnd = V_BLOCK;

	for (iBlk = (Int) Y_BLOCK1; iBlk <= iBlkEnd; iBlk++) { // + 1 is because of the indexing
		if (iBlk < (Int) U_BLOCK || iBlk > (Int) V_BLOCK) {
			if(iBlk==A_BLOCK1)
				iSumErr = 0; // start again for alpha
			if (pmbmd -> m_rgTranspStatus [iBlk % 6] == ALL) // %6 hack!!
				continue;
			switch (iBlk) 
			{
			case (Y_BLOCK1): 
				rgchBlkDst = ppxlcCurrQMBY;
				rgchBlkSrc = m_ppxlcCurrMBY;
				break;
			case (Y_BLOCK2): 
				rgchBlkDst = ppxlcCurrQMBY + BLOCK_SIZE;
				rgchBlkSrc = m_ppxlcCurrMBY + BLOCK_SIZE;
				break;
			case (Y_BLOCK3): 
				rgchBlkDst = ppxlcCurrQMBY + m_iFrameWidthYxBlkSize;
				rgchBlkSrc = m_ppxlcCurrMBY + MB_SIZE * BLOCK_SIZE;
				break;
			case (Y_BLOCK4): 
				rgchBlkDst = ppxlcCurrQMBY + m_iFrameWidthYxBlkSize + BLOCK_SIZE;
				rgchBlkSrc = m_ppxlcCurrMBY + MB_SIZE * BLOCK_SIZE + BLOCK_SIZE;
				break;
			case (A_BLOCK1):
				rgchBlkDst = ppxlcCurrQMBA;
				rgchBlkSrc = m_ppxlcCurrMBA;
				break;
			case (A_BLOCK2):
				rgchBlkDst = ppxlcCurrQMBA + BLOCK_SIZE;
				rgchBlkSrc = m_ppxlcCurrMBA + BLOCK_SIZE;
				break;
			case (A_BLOCK3):
				rgchBlkDst = ppxlcCurrQMBA + m_iFrameWidthYxBlkSize;
				rgchBlkSrc = m_ppxlcCurrMBA + MB_SIZE * BLOCK_SIZE;
				break;
			case (A_BLOCK4):
				rgchBlkDst = ppxlcCurrQMBA + m_iFrameWidthYxBlkSize + BLOCK_SIZE;
				rgchBlkSrc = m_ppxlcCurrMBA + MB_SIZE * BLOCK_SIZE + BLOCK_SIZE;
				break;
			}
			iWidthDst = m_iFrameWidthY;
			iWidthSrc = MB_SIZE;
			if(iBlk<=V_BLOCK)
				iDcScaler = iDcScalerY; //m_rgiDcScalerY [iQP];
			else
				iDcScaler = iDcScalerA;
		}
		else {
			iWidthDst = m_iFrameWidthUV;
			iWidthSrc = BLOCK_SIZE;
			rgchBlkDst = (iBlk == U_BLOCK) ? ppxlcCurrQMBU: ppxlcCurrQMBV;
			rgchBlkSrc = (iBlk == U_BLOCK) ? m_ppxlcCurrMBU: m_ppxlcCurrMBV;
			iDcScaler = iDcScalerC; //m_rgiDcScalerC [iQP];
		}
		
		if (m_volmd.nBits<=8) { // NBIT: not always valid when nBits>8
			assert(iDcScaler > 0 && iDcScaler < 128);
		}

		rgiCoefQ = m_rgpiCoefQ [iBlk - 1];
		iSumErr += quantizeIntraBlockTexture (
			rgchBlkSrc,
			iWidthSrc,
			rgchBlkDst,
			iWidthDst,
			rgiCoefQ, 
			(iBlk<=V_BLOCK ? iQP : iQPA), 
			iDcScaler,
			iBlk,	//from here til last
			pmbmLeft, 
			pmbmTop, 
			pmbmLeftTop, 
			m_rgpmbmCurr [imbX],
			pmbmdLeft, 
			pmbmdTop, 
			pmbmdLeftTop, 
			pmbmd
		); //all for intra-pred
		if(iBlk>=A_BLOCK1)
			pmbmd->m_bACPredictionAlpha	= (pmbmd->m_CODAlpha == ALPHA_CODED && iSumErr >= 0);
		else
			pmbmd->m_bACPrediction =(iSumErr >= 0);
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
                        MacroBlockMemory* pmbmCurr = m_rgpmbmCurr [imbX];
                        pmbmCurr->rgblkm [iDstBlk-1][0] = pmbmCurr->rgblkm [iBlk-1][0];
                        for (UInt x = 1; x < (BLOCK_SIZE<<1)-1; x++)
                                pmbmCurr->rgblkm [iDstBlk-1][x] = 0;
                }
        }
		// End of Hyundai(1998-5-9)*/
	}

// INTERLACE
	if ((pmbmd->m_rgTranspStatus [0] == NONE) && (m_vopmd.bInterlace == TRUE) && (pmbmd->m_bFieldDCT == TRUE))
		fieldDCTtoFrameC(ppxlcCurrQMBY);
// ~INTERLACE

	for (iBlk = (UInt) Y_BLOCK1; iBlk <= iBlkEnd; iBlk++) { // + 1 is because of the indexing
		if (pmbmd->m_rgTranspStatus [iBlk % 6] == ALL) { // hack %6 ok if [6]==[0]
			pmbmd->setCodedBlockPattern ((BlockNum) iBlk, FALSE);
			continue;
		}
		rgiCoefQ = m_rgpiCoefQ [iBlk - 1];
		if (iBlk < (Int) U_BLOCK) 
			iDcScaler = iDcScalerY; //m_rgiDcScalerY [iQP];
		else if(iBlk < (Int) A_BLOCK1)
			iDcScaler = iDcScalerC; //m_rgiDcScalerC [iQP];
		else
			iDcScaler = iDcScalerA;

		intraPred ((BlockNum) iBlk, pmbmd, rgiCoefQ,
			(iBlk<=V_BLOCK ? iQP : iQPA), iDcScaler, m_rgblkmCurrMB [iBlk - 1], m_rgiQPpred [iBlk - 1]);
		Bool bCoded = FALSE;
		UInt i;
		if(iBlk >=(Int) A_BLOCK1)
			iCoefToStart = pmbmd->m_bCodeDcAsAcAlpha==TRUE ? 0 : 1;

		for (i = iCoefToStart; i < BLOCK_SQUARE_SIZE; i++) {
			if (rgiCoefQ [i] != 0)	{
				bCoded = TRUE;
				break;
			}
		}
		pmbmd->setCodedBlockPattern ((BlockNum) iBlk, bCoded);
	}
}

Void CVideoObjectEncoder::quantizeTextureInterMB (CMBMode* pmbmd, 
												  const CMotionVector* pmv, 
												  PixelC *ppxlcCurrQMBA,
												  Bool bSkip)	//bSkip: tested mv is zero
{
	assert (pmbmd != NULL);
	assert (pmbmd -> m_dctMd == INTER || pmbmd -> m_dctMd == INTERQ);

	assert (pmbmd->m_rgTranspStatus [0] != ALL);
	Int iQuantMax = (1<<m_volmd.uiQuantPrecision) - 1;
	if (pmbmd -> m_stepSize < 1)
		pmbmd -> m_stepSize = 1;
	else if (pmbmd -> m_stepSize > iQuantMax)
		pmbmd -> m_stepSize = iQuantMax;
	Int iQP = pmbmd->m_stepSize;
#ifdef __TRACE_AND_STATS_
	m_statsMB.nQMB++;
	m_statsMB.nQp += iQP;
#endif // __TRACE_AND_STATS_

// INTERLACE
	if ((pmbmd->m_rgTranspStatus [0] == NONE) && (m_vopmd.bInterlace == TRUE)) {
		pmbmd->m_bFieldDCT = FrameFieldDCTDecideI(m_ppxliErrorMBY);
		m_statsMB.nFieldDCTMB += (Int) pmbmd->m_bFieldDCT;
	}
	else
		pmbmd->m_bFieldDCT = 0;
	Bool bMBCoded = FALSE;
// ~INTERLACE

	Int iQPA = 0;
	pmbmd->m_CODAlpha = ALPHA_CODED;
	Int iBlkEnd = V_BLOCK;
	if(m_volmd.fAUsage == EIGHT_BIT)
	{
		iBlkEnd = A_BLOCK4;
		if (pmbmd -> m_stepSizeAlpha < 1)
			pmbmd -> m_stepSizeAlpha = 1;
		iQPA = pmbmd->m_stepSizeAlpha;

		Int i, iThresh = 256 - iQPA;
		pmbmd->m_CODAlpha = ALPHA_ALL255;
		for(i = 0; i<MB_SQUARE_SIZE; i++)
			if(m_ppxlcCurrMBA[i] <= iThresh)
			{
				pmbmd->m_CODAlpha = ALPHA_CODED;
				break;
			}
	}

//	Bool bSkip = pmbmd->m_bhas4MVForward ? (bSkipAllowed && pmv [1].isZero () && pmv [2].isZero () && pmv [3].isZero () && pmv [4].isZero ())
//										 : bSkipAllowed && pmv->isZero ();
	Int* rgiBlkCurrQ = m_ppxliErrorMBY;
	Int* rgiCoefQ;
	Int iWidthCurrQ;
	Bool bSkipAlpha = TRUE;
	for (UInt iBlk = (UInt) Y_BLOCK1; iBlk <= (UInt)iBlkEnd; iBlk++) { 
		if (iBlk < (UInt) U_BLOCK || iBlk> (UInt) V_BLOCK) {
			if (pmbmd -> m_rgTranspStatus [iBlk % 6] == ALL) 
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
			case (A_BLOCK1): 
				rgiBlkCurrQ = m_ppxliErrorMBA;
				break;
			case (A_BLOCK2): 
				rgiBlkCurrQ = m_ppxliErrorMBA + BLOCK_SIZE;
				break;
			case (A_BLOCK3): 
				rgiBlkCurrQ = m_ppxliErrorMBA + MB_SIZE * BLOCK_SIZE;
				break;
			case (A_BLOCK4): 
				rgiBlkCurrQ = m_ppxliErrorMBA + MB_SIZE * BLOCK_SIZE + BLOCK_SIZE;
				break;
			}
			iWidthCurrQ = MB_SIZE;
		}
		else {
			iWidthCurrQ = BLOCK_SIZE;
			rgiBlkCurrQ = (iBlk == U_BLOCK) ? m_ppxliErrorMBU: m_ppxliErrorMBV;
		}

		rgiCoefQ = m_rgpiCoefQ [iBlk - 1];
		if(iBlk>=A_BLOCK1)
			quantizeTextureInterBlock (rgiBlkCurrQ, iWidthCurrQ, rgiCoefQ, iQPA, TRUE);
		else
			quantizeTextureInterBlock (rgiBlkCurrQ, iWidthCurrQ, rgiCoefQ, iQP, FALSE);
		Bool bCoded = FALSE;
		UInt i;
		for (i = 0; i < BLOCK_SQUARE_SIZE; i++) {
			if (rgiCoefQ [i] != 0)	{
				bCoded = TRUE;
				bMBCoded = TRUE;
				break;
			}
		}
		if(iBlk<A_BLOCK1)
			bSkip = bSkip & !bCoded;
		else
			bSkipAlpha = bSkipAlpha & !bCoded;

		pmbmd->setCodedBlockPattern ((BlockNum) iBlk, bCoded);
	}
	
	pmbmd->m_bSkip = bSkip; 

	if(m_volmd.fAUsage == EIGHT_BIT)
	{
		if(bSkipAlpha == TRUE)
			pmbmd->m_CODAlpha = ALPHA_SKIPPED;
		else if(pmbmd->m_CODAlpha == ALPHA_ALL255)
		{
			PixelC *ppxlc = ppxlcCurrQMBA;
			Int i;
			for(i = 0; i<MB_SIZE; i++, ppxlc += m_iFrameWidthY)
				pxlcmemset(ppxlc, 255, MB_SIZE);
		}
	}

// INTERLACE
	if ((pmbmd->m_rgTranspStatus [0] == NONE)
		&&(m_vopmd.bInterlace == TRUE) && (pmbmd->m_bFieldDCT == TRUE) 
		&& (bMBCoded == TRUE))
		fieldDCTtoFrameI(m_ppxliErrorMBY);
// ~INTERLACE
}

Void CVideoObjectEncoder::sendDCTCoefOfInterMBTexture (const CMBMode* pmbmd) 
{
	assert (pmbmd != NULL);
	assert (pmbmd -> m_dctMd == INTER || pmbmd -> m_dctMd == INTERQ);
	assert (pmbmd -> m_rgTranspStatus [0] != ALL);

	UInt nBits, iBlk = 0;
	for (iBlk = Y_BLOCK1; iBlk <= V_BLOCK; iBlk++) {
#ifdef __TRACE_AND_STATS_
		m_pbitstrmOut->trace (iBlk, "BLK_NO");
#endif // __TRACE_AND_STATS_
		if (iBlk < U_BLOCK)
			if (pmbmd -> m_rgTranspStatus [iBlk] == ALL) continue;
		if (pmbmd->getCodedBlockPattern ((BlockNum) iBlk))	{
			Int* rgiCoefQ = m_rgpiCoefQ [iBlk - 1];
#ifdef __TRACE_AND_STATS_
			m_pbitstrmOut->trace (rgiCoefQ, BLOCK_SQUARE_SIZE, "BLK_QUANTIZED_COEF");
#endif // __TRACE_AND_STATS_
// Modified for data partitioning mode by Toshiba(1998-1-16)
			if(m_volmd.bDataPartitioning && m_volmd.bReversibleVlc && m_vopmd.vopPredType != BVOP)
				nBits = sendTCOEFInterRVLC (rgiCoefQ, 0, grgiStandardZigzag, TRUE);
			else
			nBits = sendTCOEFInter (rgiCoefQ, 0,
                m_vopmd.bAlternateScan ? grgiVerticalZigzag : grgiStandardZigzag);
// End Toshiba(1998-1-16)
			switch (iBlk) {
			case U_BLOCK: 
				m_statsMB.nBitsCr += nBits;
				break;
			case V_BLOCK: 
				m_statsMB.nBitsCb += nBits;
				break;
			default:
				m_statsMB.nBitsY += nBits;
			}
		}
	}	
}

Void CVideoObjectEncoder::sendDCTCoefOfIntraMBTexture (const CMBMode* pmbmd) 
{
	assert (pmbmd != NULL);
	assert (pmbmd -> m_dctMd == INTRA || pmbmd -> m_dctMd == INTRAQ);
	assert (pmbmd->m_rgTranspStatus [0] != ALL);

	UInt iBlk = 0;
	for (iBlk = Y_BLOCK1; iBlk <= V_BLOCK; iBlk++) {
		UInt nBits = 0;
#ifdef __TRACE_AND_STATS_
		m_pbitstrmOut->trace (iBlk, "BLK_NO");
#endif // __TRACE_AND_STATS_
		if (iBlk < U_BLOCK)
			if (pmbmd -> m_rgTranspStatus [iBlk] == ALL) continue;
		Int* rgiCoefQ = m_rgpiCoefQ [iBlk - 1];
#ifdef __TRACE_AND_STATS_
		m_pbitstrmOut->trace (rgiCoefQ, BLOCK_SQUARE_SIZE, "BLK_QUANTIZED_COEF");
#endif // __TRACE_AND_STATS_
		Int iCoefStart = 0;
		if (pmbmd->m_bCodeDcAsAc != TRUE)	{
			iCoefStart = 1;
			nBits = sendIntraDC (rgiCoefQ, (BlockNum) iBlk);
		}
		if (pmbmd->getCodedBlockPattern ((BlockNum) iBlk))	{
			Int* rgiZigzag = grgiStandardZigzag;
            if (m_vopmd.bAlternateScan)
                rgiZigzag = grgiVerticalZigzag;
            else if (pmbmd->m_bACPrediction)	
				rgiZigzag = (pmbmd->m_preddir [iBlk - 1] == HORIZONTAL) ? grgiVerticalZigzag : grgiHorizontalZigzag;
// Modified for data partitioning mode by Toshiba(1998-1-16)
				if(m_volmd.bDataPartitioning && m_volmd.bReversibleVlc && m_vopmd.vopPredType != BVOP)
					nBits += sendTCOEFIntraRVLC (rgiCoefQ, iCoefStart, rgiZigzag, TRUE);
				else
					nBits += sendTCOEFIntra (rgiCoefQ, iCoefStart, rgiZigzag);
// End Toshiba(1998-1-16)
		}
		switch (iBlk) {
		case U_BLOCK: 
			m_statsMB.nBitsCr += nBits;
			break;
		case V_BLOCK: 
			m_statsMB.nBitsCb += nBits;
			break;
		default:
			m_statsMB.nBitsY += nBits;
		}
	}	
}

UInt CVideoObjectEncoder::sumAbsCurrMB ()
{
	PixelC* ppxlcCurrMBY = m_ppxlcCurrMBY;
	UInt uisumAbs = 0;
	Int ic;
	for (ic = 0; ic < MB_SQUARE_SIZE; ic++) {
		uisumAbs += ppxlcCurrMBY [ic];
	}
	return uisumAbs;
}

Void CVideoObjectEncoder::codeMBAlphaHeadOfIVOP (const CMBMode* pmbmd)
{
	// get CBPA
	Int CBPA = 0;
	UInt cNonTrnspBlk = 0, iBlk;
	for (iBlk = (UInt) A_BLOCK1; iBlk <= (UInt) A_BLOCK4; iBlk++) {
		if (pmbmd->m_rgTranspStatus [iBlk - 6] != ALL)	
			cNonTrnspBlk++;
	}
	UInt iBitPos = 1;
	for (iBlk = (UInt) A_BLOCK1; iBlk <= (UInt) A_BLOCK4; iBlk++)	{
		if (pmbmd->m_rgTranspStatus [iBlk - 6] != ALL)	{
			CBPA |= pmbmd->getCodedBlockPattern ((BlockNum) iBlk) << (cNonTrnspBlk - iBitPos);
			iBitPos++;
		}
	}
	assert (CBPA >= 0 && CBPA <= 15);
	
	Int iCODA = 0;

	if(pmbmd->m_CODAlpha==ALPHA_ALL255)
		iCODA = 1;

	m_pbitstrmOut->putBits(iCODA, 1, "MB_CODA");
	m_statsMB.nBitsCODA++;

	if(iCODA)
		return;

	m_pbitstrmOut->putBits (pmbmd->m_bACPredictionAlpha, 1, "MB_ACPRED_ALPHA");
	m_statsMB.nBitsIntraPred++;

#ifdef __TRACE_AND_STATS_
	m_pbitstrmOut->trace (CBPA, "MB_CBPA");
#endif // __TRACE_AND_STATS_
	switch (cNonTrnspBlk) {
	case 1:
		m_statsMB.nBitsCBPA += m_pentrencSet->m_pentrencCBPY1->encodeSymbol (1 - CBPA, "MB_CBPA");
		break;
	case 2:
		m_statsMB.nBitsCBPA += m_pentrencSet->m_pentrencCBPY2->encodeSymbol (3 - CBPA, "MB_CBPA");
		break;
	case 3:
		m_statsMB.nBitsCBPA += m_pentrencSet->m_pentrencCBPY3->encodeSymbol (7 - CBPA, "MB_CBPA");
		break;
	case 4:
		m_statsMB.nBitsCBPA += m_pentrencSet->m_pentrencCBPY->encodeSymbol (15 - CBPA, "MB_CBPA");
		break;
	default:
		assert (FALSE);
	}
}

Void CVideoObjectEncoder::codeMBAlphaHeadOfPVOP (const CMBMode* pmbmd)
{
	if(pmbmd -> m_dctMd == INTRA || pmbmd -> m_dctMd == INTRAQ)
		codeMBAlphaHeadOfIVOP(pmbmd);
	else
	{
		// get CBPA
		Int CBPA = 0;
		UInt cNonTrnspBlk = 0, iBlk;
		for (iBlk = (UInt) A_BLOCK1; iBlk <= (UInt) A_BLOCK4; iBlk++) {
			if (pmbmd->m_rgTranspStatus [iBlk - 6] != ALL)	
				cNonTrnspBlk++;
		}
		UInt iBitPos = 1;
		for (iBlk = (UInt) A_BLOCK1; iBlk <= (UInt) A_BLOCK4; iBlk++)	{
			if (pmbmd->m_rgTranspStatus [iBlk - 6] != ALL)	{
				CBPA |= pmbmd->getCodedBlockPattern ((BlockNum) iBlk) << (cNonTrnspBlk - iBitPos);
				iBitPos++;
			}
		}
		assert (CBPA >= 0 && CBPA <= 15);
		
		if(pmbmd->m_CODAlpha==ALPHA_CODED)
		{
			m_pbitstrmOut->putBits(0, 2, "MB_CODA");
			m_statsMB.nBitsCODA += 2;
		}
		else if(pmbmd->m_CODAlpha==ALPHA_ALL255)
		{
			m_pbitstrmOut->putBits(1, 2, "MB_CODA");
			m_statsMB.nBitsCODA += 2;
			return;
		}
		else // ALPHA_SKIPPED
		{
			m_pbitstrmOut->putBits(1, 1, "MB_CODA");
			m_statsMB.nBitsCODA ++;
			return;
		}

#ifdef __TRACE_AND_STATS_
		m_pbitstrmOut->trace (CBPA, "MB_CBPA");
#endif // __TRACE_AND_STATS_
		switch (cNonTrnspBlk) {
		case 1:
			m_statsMB.nBitsCBPA += m_pentrencSet->m_pentrencCBPY1->encodeSymbol (1 - CBPA, "MB_CBPA");
			break;
		case 2:
			m_statsMB.nBitsCBPA += m_pentrencSet->m_pentrencCBPY2->encodeSymbol (3 - CBPA, "MB_CBPA");
			break;
		case 3:
			m_statsMB.nBitsCBPA += m_pentrencSet->m_pentrencCBPY3->encodeSymbol (7 - CBPA, "MB_CBPA");
			break;
		case 4:
			m_statsMB.nBitsCBPA += m_pentrencSet->m_pentrencCBPY->encodeSymbol (15 - CBPA, "MB_CBPA");
			break;
		default:
			assert (FALSE);
		}
	}
}

Void CVideoObjectEncoder::sendDCTCoefOfIntraMBAlpha (const CMBMode* pmbmd)
{
	assert (pmbmd != NULL);
	assert (pmbmd -> m_dctMd == INTRA || pmbmd -> m_dctMd == INTRAQ);
	assert (pmbmd->m_rgTranspStatus [0] != ALL);

	if(pmbmd->m_CODAlpha != ALPHA_CODED)
		return;

	UInt iBlk;
	for (iBlk = A_BLOCK1; iBlk <= A_BLOCK4; iBlk++) {
		UInt nBits = 0;
#ifdef __TRACE_AND_STATS_
		m_pbitstrmOut->trace (iBlk, "ALPHA_BLK_NO");
#endif // __TRACE_AND_STATS_
		if (pmbmd -> m_rgTranspStatus [iBlk - 6 ] == ALL)
			continue;
		Int* rgiCoefQ = m_rgpiCoefQ [iBlk - 1];
#ifdef __TRACE_AND_STATS_
		m_pbitstrmOut->trace (rgiCoefQ, BLOCK_SQUARE_SIZE, "BLK_QUANTIZED_COEF");
#endif // __TRACE_AND_STATS_
		
		Int iCoefStart = 0;
		if (pmbmd->m_bCodeDcAsAcAlpha != TRUE)	{
			iCoefStart = 1;
			nBits = sendIntraDC (rgiCoefQ, (BlockNum) iBlk);
		}

		if (pmbmd->getCodedBlockPattern ((BlockNum) iBlk))	{
			Int* rgiZigzag = grgiStandardZigzag;
			if (pmbmd->m_bACPredictionAlpha)	
				rgiZigzag = (pmbmd->m_preddir [iBlk - 1] == HORIZONTAL) ? grgiVerticalZigzag : grgiHorizontalZigzag;
			nBits += sendTCOEFIntra (rgiCoefQ, iCoefStart, rgiZigzag);
		}
		m_statsMB.nBitsA += nBits;
	}	
}

Void CVideoObjectEncoder::sendDCTCoefOfInterMBAlpha (const CMBMode* pmbmd)
{
	assert (pmbmd != NULL);
	assert (pmbmd -> m_dctMd == INTER || pmbmd -> m_dctMd == INTERQ);
	assert (pmbmd -> m_rgTranspStatus [0] != ALL);
	assert (pmbmd -> m_CODAlpha == ALPHA_CODED);

	UInt nBits, iBlk = 0;
	for (iBlk = A_BLOCK1; iBlk <= A_BLOCK4; iBlk++) {
#ifdef __TRACE_AND_STATS_
		m_pbitstrmOut->trace (iBlk, "BLK_NO");
#endif // __TRACE_AND_STATS_
		if (pmbmd -> m_rgTranspStatus [iBlk - 6] == ALL) continue;
		if (pmbmd->getCodedBlockPattern ((BlockNum) iBlk))	{
			Int* rgiCoefQ = m_rgpiCoefQ [iBlk - 1];
#ifdef __TRACE_AND_STATS_
			m_pbitstrmOut->trace (rgiCoefQ, BLOCK_SQUARE_SIZE, "BLK_QUANTIZED_COEF");
#endif // __TRACE_AND_STATS_
			nBits = sendTCOEFInter (rgiCoefQ, 0, grgiStandardZigzag);
			m_statsMB.nBitsA += nBits;
		}
	}	
}

Void CVideoObjectEncoder::codeMBAlphaHeadOfBVOP (const CMBMode* pmbmd)
{
	// get CBPA
	Int CBPA = 0;
	UInt cNonTrnspBlk = 0, iBlk;
	for (iBlk = (UInt) A_BLOCK1; iBlk <= (UInt) A_BLOCK4; iBlk++) {
		if (pmbmd->m_rgTranspStatus [iBlk - 6] != ALL)	
			cNonTrnspBlk++;
	}
	UInt iBitPos = 1;
	for (iBlk = (UInt) A_BLOCK1; iBlk <= (UInt) A_BLOCK4; iBlk++)	{
		if (pmbmd->m_rgTranspStatus [iBlk - 6] != ALL)	{
			CBPA |= pmbmd->getCodedBlockPattern ((BlockNum) iBlk) << (cNonTrnspBlk - iBitPos);
			iBitPos++;
		}
	}
	assert (CBPA >= 0 && CBPA <= 15);
	
	if(pmbmd->m_CODAlpha==ALPHA_CODED)
	{
		m_pbitstrmOut->putBits(0, 2, "MB_CODBA");
		m_statsMB.nBitsCODA += 2;
	}
	else if(pmbmd->m_CODAlpha==ALPHA_ALL255)
	{
		m_pbitstrmOut->putBits(1, 2, "MB_CODBA");
		m_statsMB.nBitsCODA += 2;
		return;
	}
	else // ALPHA_SKIPPED
	{
		m_pbitstrmOut->putBits(1, 1, "MB_CODBA");
		m_statsMB.nBitsCODA ++;
		return;
	}

#ifdef __TRACE_AND_STATS_
	m_pbitstrmOut->trace (CBPA, "MB_CBPBA");
#endif // __TRACE_AND_STATS_
	switch (cNonTrnspBlk) {
	case 1:
		m_statsMB.nBitsCBPA += m_pentrencSet->m_pentrencCBPY1->encodeSymbol (1 - CBPA, "MB_CBPA");
		break;
	case 2:
		m_statsMB.nBitsCBPA += m_pentrencSet->m_pentrencCBPY2->encodeSymbol (3 - CBPA, "MB_CBPA");
		break;
	case 3:
		m_statsMB.nBitsCBPA += m_pentrencSet->m_pentrencCBPY3->encodeSymbol (7 - CBPA, "MB_CBPA");
		break;
	case 4:
		m_statsMB.nBitsCBPA += m_pentrencSet->m_pentrencCBPY->encodeSymbol (15 - CBPA, "MB_CBPA");
		break;

	default:

		assert (FALSE);

	}

}




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

	and also edited by
	Yoshinori Suzuki (Hitachi, Ltd.)

	and also edited by
	Fujitsu Laboratories Ltd. (contact: Eishi Morimatsu)

	and also edited by 
	Takefumi Nagumo (nagumo@av.crl.sony.co.jp) Sony Corporation
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
	Feb. 23   1999:  GMC added by Yoshinori Suzuki(Hitachi, Ltd.) 
	May 9, 1999:	tm5 rate control by DemoGraFX, duhoff@mediaone.net
	NOTE:

	m_pvopfCurrQ holds the original data until it is texture quantized

	Sep.06	1999 : RRV added by Eishi Morimatsu (Fujitsu Laboratories Ltd.) 
	Feb.01	2000 : Bug fixed OBSS by Takefumi Nagumo (Sony)
*************************************************************************/

#include <stdlib.h>
#include <math.h>
#include <iostream>
#include "typeapi.h"
#include "codehead.h"
#include "mode.hpp"
#include "global.hpp"
#include "bitstrm.hpp"
#include "entropy.hpp"
#include "huffman.hpp"
#include "vopses.hpp"
#include "vopseenc.hpp"
// RRV insertion
#include "rrv.hpp"
// ~RRV

#ifdef __MFC_
#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

#define new DEBUG_NEW				   
#endif // __MFC_

Void CVideoObjectEncoder::updateQP(CMBMode* pmbmd, Int iPrevQP, Int iNewQP)
{
	Int iNewStepSize;
	
	if(pmbmd->m_dctMd==INTER && pmbmd->m_bhas4MVForward)
		pmbmd->m_intStepDelta = 0;
	// cant change if 4mv
	// we do not know yet if the mb is not-coded, or transparent
	// therefore we must cancel the qp update later if we find we are skipping the mb
	// to do this we call cancelQPUpdate
	else
		pmbmd->m_intStepDelta = iNewQP - iPrevQP;
	
	// bounds check QP and delta QP
	if(pmbmd->m_intStepDelta>2)
		pmbmd->m_intStepDelta = 2;
	else if(pmbmd->m_intStepDelta<-2)
		pmbmd->m_intStepDelta = -2;
	iNewStepSize = iPrevQP + pmbmd->m_intStepDelta;
	
	Int iMaxQP = (1<<m_volmd.uiQuantPrecision)-1;
	
	if(iNewStepSize > iMaxQP)
		iNewStepSize = iMaxQP;
	else if(iNewStepSize < 1)
		iNewStepSize = 1;
	
	// set values
	pmbmd->m_stepSize = iNewStepSize;
	pmbmd->m_intStepDelta = iNewStepSize - iPrevQP;
	
	// set dct mode (robustly)
	if (pmbmd->m_intStepDelta == 0)
	{
		if(pmbmd->m_dctMd==INTRAQ)
			pmbmd->m_dctMd = INTRA;
		else if(pmbmd->m_dctMd==INTERQ)
			pmbmd->m_dctMd = INTER;
	}
	else
	{
		if(pmbmd->m_dctMd==INTRA)
			pmbmd->m_dctMd = INTRAQ;
		else if(pmbmd->m_dctMd==INTER)
			pmbmd->m_dctMd = INTERQ;
	}

	if(m_volmd.fAUsage == EIGHT_BIT)
		setAlphaQP(pmbmd);	
}

// this is called if we wanted a dquant but later found the mb was skipped
Void CVideoObjectEncoder::cancelQPUpdate(CMBMode* pmbmd)
{
	// we set the delta qp to be zero
	pmbmd->m_stepSize -= pmbmd->m_intStepDelta;
	pmbmd->m_intStepDelta = 0;
	if(pmbmd->m_dctMd == INTERQ)
		pmbmd->m_dctMd = INTER;

	if(m_volmd.fAUsage == EIGHT_BIT)
		setAlphaQP(pmbmd);
}

Void CVideoObjectEncoder::setAlphaQP(CMBMode* pmbmd)
{
	Int iQPA;
	if(!m_volmd.bNoGrayQuantUpdate)
	{
		if(m_vopmd.vopPredType == BVOP)
			iQPA = (pmbmd->m_stepSize * m_vopmd.intStepBAlpha[0]) / m_vopmd.intStepB;
		else if(m_vopmd.vopPredType == IVOP)
			iQPA = (pmbmd->m_stepSize * m_vopmd.intStepIAlpha[0]) / m_vopmd.intStepI;
		else
			iQPA = (pmbmd->m_stepSize * m_vopmd.intStepPAlpha[0]) / m_vopmd.intStep;
	}
	else
	{
		if(m_vopmd.vopPredType == BVOP)
			iQPA = m_vopmd.intStepBAlpha[0];
		else if(m_vopmd.vopPredType == IVOP)
			iQPA = m_vopmd.intStepIAlpha[0];
		else
			iQPA = m_vopmd.intStepPAlpha[0];
	}
	if(iQPA<1)
		iQPA=1;
	pmbmd->m_stepSizeAlpha = iQPA;
}

Void CVideoObjectEncoder::encodePVOPMBJustShape(
												PixelC* ppxlcRefBY, CMBMode* pmbmd, ShapeMode shpmdColocatedMB,
												const CMotionVector* pmv, CMotionVector* pmvBY,
												CoordI x, CoordI y, Int imbX, Int imbY)
{
	
	//OBSS_SAIT_991015	
	//OBSSFIX_MODE3
	if(!(m_volmd.bSpatialScalability && m_volmd.iHierarchyType==0) ||
		(m_volmd.bSpatialScalability && m_volmd.iHierarchyType==0 && m_volmd.volType == ENHN_LAYER && m_volmd.iEnhnType!=0 && m_volmd.iuseRefShape ==1)){
		//	if(!(m_volmd.bSpatialScalability && m_volmd.iHierarchyType==0)){
		//~OBSSFIX_MODE3
		m_statsMB.nBitsShape += codeInterShape (
			ppxlcRefBY, m_pvopcRefQ0, pmbmd, shpmdColocatedMB,
			pmv, pmvBY, x, y, imbX, imbY);
	}
	else{
		if((m_volmd.volType == BASE_LAYER) || (!(m_volmd.iEnhnType==0 || m_volmd.iuseRefShape ==0) && !m_volmd.bShapeOnly) )			
			m_statsMB.nBitsShape += codeInterShape (
			ppxlcRefBY, m_pvopcRefQ0, pmbmd, shpmdColocatedMB,
			pmv, pmvBY, x, y, imbX, imbY);
		
		else
			m_statsMB.nBitsShape += codeSIShapePVOP (
			ppxlcRefBY, m_pvopcRefQ0, pmbmd, shpmdColocatedMB,
			pmv, pmvBY, x, y, imbX, imbY);
	}
	//~OBSS_SAIT_991015
	
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

// HHI Schueuer: added  const PixelC *ppxlcCurrMBBY, const PixelC *ppxlcCurrMBBUV for sadct
Void CVideoObjectEncoder::encodePVOPMBTextureWithShape(
													   PixelC* ppxlcRefMBY, 
													   PixelC* ppxlcRefMBU,
													   PixelC* ppxlcRefMBV,
													   PixelC** pppxlcRefMBA,
													   CMBMode* pmbmd,
													   const CMotionVector* pmv,
													   Int imbX,
													   Int imbY,
													   CoordI x,
													   CoordI y,
													   Bool* pbRestart,
													   const PixelC *ppxlcCurrMBBY,
													   const PixelC *ppxlcCurrMBBUV)
{
	if (pmbmd -> m_dctMd == INTRA || pmbmd -> m_dctMd == INTRAQ) {
		if(pmbmd -> m_rgTranspStatus [0] == PARTIAL)
			LPEPadding (pmbmd);
	
		assert (pmbmd -> m_rgTranspStatus [0] != ALL);

		// vlc mode needs to be set up provisionally before quantisation
		// to ensure that the correct cbp is generated if using ac coefs
		// however codembtextureheadofpvop also calls the function, so we
		// save the restart value to avoid problems on the second call
		Bool bTemp = *pbRestart;
		setDCVLCMode(pmbmd, pbRestart);
		*pbRestart = bTemp;

		if (!m_volmd.bSadctDisable)
			quantizeTextureIntraMB (imbX, imbY, pmbmd, ppxlcRefMBY, ppxlcRefMBU, ppxlcRefMBV, pppxlcRefMBA, ppxlcCurrMBBY, ppxlcCurrMBBUV);
		else
			quantizeTextureIntraMB (imbX, imbY, pmbmd, ppxlcRefMBY, ppxlcRefMBU, ppxlcRefMBV, pppxlcRefMBA);

		codeMBTextureHeadOfPVOP (pmbmd, pbRestart);
		sendDCTCoefOfIntraMBTexture (pmbmd);

		if (m_volmd.fAUsage == EIGHT_BIT) {
			for(Int iAuxComp=0;iAuxComp<m_volmd.iAuxCompCount;iAuxComp++) { // MAC (SB) 29-Nov-99
				codeMBAlphaHeadOfPVOP (pmbmd,iAuxComp);
				sendDCTCoefOfIntraMBAlpha (pmbmd,iAuxComp);
			}
		}
	}
	else { // INTER or skipped
		if (pmbmd -> m_rgTranspStatus [0] == PARTIAL) {
			CoordI xRefUV, yRefUV;
			
			if(pmbmd->m_bMCSEL) {
				FindGlobalChromPredForGMC(x,y,m_ppxlcPredMBU,m_ppxlcPredMBV);
			}else{
				if(pmbmd->m_bFieldMV) {
					CoordI xRefUV1, yRefUV1;
					mvLookupUV (pmbmd, pmv, xRefUV, yRefUV, xRefUV1, yRefUV1);
					motionCompFieldUV(m_ppxlcPredMBU, m_ppxlcPredMBV,
						m_pvopcRefQ0, x, y, xRefUV, yRefUV, pmbmd->m_bForwardTop,
						&m_rctRefVOPY0); // added by Y.Suzuki for the extended bounding box support
					motionCompFieldUV(m_ppxlcPredMBU + BLOCK_SIZE, m_ppxlcPredMBV + BLOCK_SIZE,
						m_pvopcRefQ0, x, y, xRefUV1, yRefUV1, pmbmd->m_bForwardBottom,
						&m_rctRefVOPY0); // added by Y.Suzuki for the extended bounding box support
				}
				else {
					mvLookupUVWithShape (pmbmd, pmv, xRefUV, yRefUV);
					motionCompUV (m_ppxlcPredMBU, m_ppxlcPredMBV, m_pvopcRefQ0, x, y,
						xRefUV, yRefUV, m_vopmd.iRoundingControl,&m_rctRefVOPY0);
				}
			}
			motionCompMBYEnc (pmv, pmbmd, imbX, imbY, x, y, &m_rctRefVOPY0);
			computeTextureErrorWithShape ();
		}
		else {
			// not partial
			CoordI xRefUV, yRefUV, xRefUV1, yRefUV1;
			if(pmbmd->m_bMCSEL) {
				FindGlobalChromPredForGMC(x,y,m_ppxlcPredMBU,m_ppxlcPredMBV);
			}else{
				mvLookupUV (pmbmd, pmv, xRefUV, yRefUV, xRefUV1, yRefUV1);

				if(pmbmd->m_bFieldMV) {
					motionCompFieldUV(m_ppxlcPredMBU, m_ppxlcPredMBV,
						m_pvopcRefQ0, x, y, xRefUV, yRefUV, pmbmd->m_bForwardTop,
						&m_rctRefVOPY0); // added by Y.Suzuki for the extended bounding box support
					motionCompFieldUV(m_ppxlcPredMBU + BLOCK_SIZE, m_ppxlcPredMBV + BLOCK_SIZE,
						m_pvopcRefQ0, x, y, xRefUV1, yRefUV1, pmbmd->m_bForwardBottom,
						&m_rctRefVOPY0); // added by Y.Suzuki for the extended bounding box support
				}
				else {
					motionCompUV (m_ppxlcPredMBU, m_ppxlcPredMBV, m_pvopcRefQ0, x, y,
						xRefUV, yRefUV, m_vopmd.iRoundingControl, &m_rctRefVOPY0);
				}
			}

			motionCompMBYEnc (pmv, pmbmd, imbX, imbY, x, y, &m_rctRefVOPY0);
			computeTextureError ();
		}
		Bool bSkip = pmbmd->m_bhas4MVForward ? (pmv [1].isZero () && pmv [2].isZero () && pmv [3].isZero () && pmv [4].isZero ())
			: (!pmbmd->m_bFieldMV && pmv->isZero()) ;

		if(pmbmd -> m_bMCSEL)
			bSkip= TRUE;
		
		if(!m_volmd.bAllowSkippedPMBs)
			bSkip = FALSE;

		if (!m_volmd.bSadctDisable)
			quantizeTextureInterMB (pmbmd, pmv, pppxlcRefMBA, bSkip, ppxlcCurrMBBY, ppxlcCurrMBBUV); // decide COD here
		else
			quantizeTextureInterMB (pmbmd, pmv, pppxlcRefMBA, bSkip); // decide COD here

		if((m_uiSprite == 2 && m_vopmd.vopPredType == SPRITE))
			if(pmbmd -> m_bSkip && !pmbmd -> m_bMCSEL)
				pmbmd -> m_bSkip = FALSE;

		codeMBTextureHeadOfPVOP (pmbmd, pbRestart);
		if (!(pmbmd -> m_bSkip && !pmbmd -> m_bMCSEL)) {
			if(!(m_volmd.volType == ENHN_LAYER && m_vopmd.iRefSelectCode == 3) && !pmbmd -> m_bMCSEL)
				//for zero-MV of OBSS(P-VOP)
				m_statsMB.nBitsMV += encodeMVWithShape (pmv, pmbmd, imbX, imbY);
			if (pmbmd -> m_bSkip)
				assignPredToCurrQ (ppxlcRefMBY, ppxlcRefMBU, ppxlcRefMBV);
			else
				sendDCTCoefOfInterMBTexture (pmbmd);
		}
		else
			assignPredToCurrQ (ppxlcRefMBY, ppxlcRefMBU, ppxlcRefMBV);

		if (m_volmd.fAUsage == EIGHT_BIT) {
			for(Int iAuxComp=0;iAuxComp<m_volmd.iAuxCompCount;iAuxComp++) { // MAC (SB) 29-Nov-99
				codeMBAlphaHeadOfPVOP (pmbmd, iAuxComp);
				if (pmbmd -> m_pCODAlpha[iAuxComp] == ALPHA_CODED) {
					sendDCTCoefOfInterMBAlpha (pmbmd, iAuxComp);
				}
				else if(pmbmd -> m_pCODAlpha[iAuxComp] == ALPHA_SKIPPED)
					assignAlphaPredToCurrQ (pppxlcRefMBA[iAuxComp],iAuxComp);
			}
		}

		if (!pmbmd -> m_bSkip)
		{
			addErrorAndPredToCurrQ (ppxlcRefMBY, ppxlcRefMBU, ppxlcRefMBV);
		}

		if (m_volmd.fAUsage == EIGHT_BIT) { 
			for(Int iAuxComp=0;iAuxComp<m_volmd.iAuxCompCount;iAuxComp++) { // MAC (SB) 29-Nov-99
				if (pmbmd -> m_pCODAlpha[iAuxComp] == ALPHA_CODED)
					addAlphaErrorAndPredToCurrQ (pppxlcRefMBA[iAuxComp],iAuxComp);
			}
		}
	}
}

Void CVideoObjectEncoder::encodePVOPMB (
										PixelC* ppxlcRefMBY, PixelC* ppxlcRefMBU, PixelC* ppxlcRefMBV,
										CMBMode* pmbmd, const CMotionVector* pmv, const CMotionVector* pmv_RRV,
										Int iMBX, Int iMBY,
										CoordI x, CoordI y, Bool *pbRestart
										)
{
	// For Sprite update macroblock there is no motion compensation 
	if (m_uiSprite == 1 && m_vopmd.SpriteXmitMode != STOP) {
		if ( m_bSptMB_NOT_HOLE ) {
			m_iNumSptMB++;	   	 
			CopyCurrQToPred(ppxlcRefMBY, ppxlcRefMBU, ppxlcRefMBV);
			computeTextureError ();
			Bool bSkip = pmbmd->m_bhas4MVForward ? (pmv [1].isZero () && pmv [2].isZero () && pmv [3].isZero () && pmv [4].isZero ())
				: pmv->isZero ();
			quantizeTextureInterMB (pmbmd, pmv, NULL, bSkip); // decide COD here
			
			codeMBTextureHeadOfPVOP (pmbmd, pbRestart);
			if (!pmbmd -> m_bSkip) {
				sendDCTCoefOfInterMBTexture (pmbmd);
				addErrorAndPredToCurrQ (ppxlcRefMBY, ppxlcRefMBU, ppxlcRefMBV);
			}
		}
		else {
			pmbmd -> m_bSkip = TRUE;
			codeMBTextureHeadOfPVOP (pmbmd, pbRestart);
		}

		return;
	}
	
	if (pmbmd->m_dctMd == INTRA || pmbmd->m_dctMd == INTRAQ) {
		pmbmd->m_bSkip = FALSE;			//in case use by direct mode in the future
		// RRV insertion
		if(m_vopmd.RRVmode.iRRVOnOff == 1)
		{
			DownSamplingTextureForRRV(m_ppxlcCurrMBY,
				m_ppxlcCurrMBY,
				(MB_SIZE *m_iRRVScale),
				(MB_SIZE *m_iRRVScale));
			DownSamplingTextureForRRV(m_ppxlcCurrMBU,
				m_ppxlcCurrMBU,
				(BLOCK_SIZE* m_iRRVScale),
				(BLOCK_SIZE* m_iRRVScale));
			DownSamplingTextureForRRV(m_ppxlcCurrMBV,
				m_ppxlcCurrMBV,
				(BLOCK_SIZE* m_iRRVScale),
				(BLOCK_SIZE* m_iRRVScale));
		}
		// ~RRV	
		// vlc mode needs to be set up provisionally before quantisation
		// to ensure that the correct cbp is generated if using ac coefs
		// however codembtextureheadofpvop also calls the function, so we
		// save the restart value to avoid problems on the second call
		Bool bTemp = *pbRestart;
		setDCVLCMode(pmbmd, pbRestart);
		*pbRestart = bTemp;
		quantizeTextureIntraMB (iMBX, iMBY, pmbmd, ppxlcRefMBY, ppxlcRefMBU, ppxlcRefMBV, NULL);
		// RRV insertion
		if(m_vopmd.RRVmode.iRRVOnOff == 1)
		{
			UpSamplingTextureForRRV(ppxlcRefMBY, ppxlcRefMBY,
				MB_SIZE, MB_SIZE,
				m_iFrameWidthY);
			UpSamplingTextureForRRV(ppxlcRefMBU, ppxlcRefMBU,
				BLOCK_SIZE, BLOCK_SIZE,
				m_iFrameWidthUV);
			UpSamplingTextureForRRV(ppxlcRefMBV, ppxlcRefMBV,
				BLOCK_SIZE, BLOCK_SIZE,
				m_iFrameWidthUV);
		}
		// ~RRV
		codeMBTextureHeadOfPVOP (pmbmd, pbRestart);
		sendDCTCoefOfIntraMBTexture (pmbmd);
	}
	else {
		CoordI xRefUV, yRefUV, xRefUV1, yRefUV1;
		// GMC
		if(pmbmd->m_bMCSEL) {
			FindGlobalChromPredForGMC(x,y,m_ppxlcPredMBU,m_ppxlcPredMBV);
		}else{
			// ~GMC
			// RRV modification
			if(m_vopmd.RRVmode.iRRVOnOff == 1)
			{
				mvLookupUV(pmbmd, pmv_RRV, xRefUV, yRefUV, xRefUV1, yRefUV1);
			}
			else
			{
				mvLookupUV(pmbmd, pmv, xRefUV, yRefUV, xRefUV1, yRefUV1);
			}
			//		mvLookupUV (pmbmd, pmv, xRefUV, yRefUV, xRefUV1, yRefUV1);
			// ~RRV		
			// INTERLACE
			// pmbmd->m_rgTranspStatus[0] = NONE;			// This a rectangular VOP 
			if(pmbmd->m_bFieldMV) {
				motionCompFieldUV(m_ppxlcPredMBU, m_ppxlcPredMBV,
					m_pvopcRefQ0, x, y, xRefUV, yRefUV, pmbmd->m_bForwardTop,
					&m_rctRefVOPY0); // added by Y.Suzuki for the extended bounding box support
				motionCompFieldUV(m_ppxlcPredMBU + BLOCK_SIZE, m_ppxlcPredMBV + BLOCK_SIZE,
					m_pvopcRefQ0, x, y, xRefUV1, yRefUV1, pmbmd->m_bForwardBottom,
					&m_rctRefVOPY0); // added by Y.Suzuki for the extended bounding box support
			}
			else 
				// ~INTERLACE
				motionCompUV (m_ppxlcPredMBU, m_ppxlcPredMBV, m_pvopcRefQ0, x, y,
				xRefUV, yRefUV, m_vopmd.iRoundingControl,&m_rctRefVOPY0);
			// GMC
		}
		// ~GMC

		if(m_vopmd.RRVmode.iRRVOnOff == 1)
		{
			motionCompMBYEnc(pmv_RRV, pmbmd, iMBX, iMBY,
				x, y, &m_rctRefVOPY0);
		}
		else
		{
			motionCompMBYEnc(pmv, pmbmd, iMBX, iMBY, x, y, &m_rctRefVOPY0);
		}

		computeTextureError ();

		if(m_vopmd.RRVmode.iRRVOnOff == 1)
		{
			DownSamplingTextureForRRV(m_ppxliErrorMBY, m_ppxliErrorMBY, 
				(MB_SIZE *m_iRRVScale),
				(MB_SIZE *m_iRRVScale));
			DownSamplingTextureForRRV(m_ppxliErrorMBU, m_ppxliErrorMBU,
				(BLOCK_SIZE *m_iRRVScale),
				(BLOCK_SIZE *m_iRRVScale));
			DownSamplingTextureForRRV(m_ppxliErrorMBV, m_ppxliErrorMBV,
				(BLOCK_SIZE *m_iRRVScale),
				(BLOCK_SIZE *m_iRRVScale));
		}

		Bool bSkip = pmbmd->m_bhas4MVForward ? (pmv [1].isZero () && pmv [2].isZero () && pmv [3].isZero () && pmv [4].isZero ())
            : (!pmbmd->m_bFieldMV && pmv->isZero());

		if(pmbmd -> m_bMCSEL)
			bSkip= TRUE;

		if(!m_volmd.bAllowSkippedPMBs)
			bSkip = FALSE;
		
		quantizeTextureInterMB (pmbmd, pmv, NULL, bSkip); // decide COD here

		if(m_vopmd.RRVmode.iRRVOnOff == 1)
		{
			UpSamplingTextureForRRV(m_ppxliErrorMBY,m_ppxliErrorMBY,
				MB_SIZE, MB_SIZE, 
				(MB_SIZE *m_iRRVScale));
			UpSamplingTextureForRRV(m_ppxliErrorMBU, m_ppxliErrorMBU,
				BLOCK_SIZE, BLOCK_SIZE,
				BLOCK_SIZE *(m_iRRVScale));
			UpSamplingTextureForRRV(m_ppxliErrorMBV, m_ppxliErrorMBV,
				BLOCK_SIZE, BLOCK_SIZE, 
				BLOCK_SIZE *(m_iRRVScale));
		}

		if((m_uiSprite == 2 && m_vopmd.vopPredType == SPRITE))
			if(pmbmd -> m_bSkip && !pmbmd -> m_bMCSEL)
				pmbmd -> m_bSkip = FALSE;

		codeMBTextureHeadOfPVOP (pmbmd, pbRestart);
		if (!(pmbmd -> m_bSkip && !pmbmd -> m_bMCSEL)) { // GMC
			if(!(m_volmd.volType == ENHN_LAYER && m_vopmd.iRefSelectCode == 3))
				// GMC
				if(!pmbmd -> m_bMCSEL)
					// ~GMC
					m_statsMB.nBitsMV += encodeMVVP (pmv, pmbmd, iMBX, iMBY);
				// GMC
				if (pmbmd -> m_bSkip) {
					assignPredToCurrQ (ppxlcRefMBY, ppxlcRefMBU, ppxlcRefMBV);
				}else{
					// ~GMC
					sendDCTCoefOfInterMBTexture (pmbmd);
					addErrorAndPredToCurrQ (ppxlcRefMBY, ppxlcRefMBU, ppxlcRefMBV);
					// GMC
				}
				// ~GMC
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

// HHI Schueuer: added  const PixelC *ppxlcCurrMBBY, const PixelC *ppxlcCurrMBBUV for sadct
Void CVideoObjectEncoder::encodeBVOPMB_WithShape (
												  PixelC* ppxlcCurrQMBY,
												  PixelC* ppxlcCurrQMBU,
												  PixelC* ppxlcCurrQMBV, 
												  PixelC** pppxlcCurrQMBA,
												  PixelC* ppxlcCurrQBY,
												  CMBMode* pmbmd,
												  const CMotionVector* pmv,
												  const CMotionVector* pmvBackward, 
												  CMotionVector* pmvBY,
												  ShapeMode shpmdColocatedMB,
												  const CMBMode* pmbmdRef,
												  const CMotionVector* pmvRef,
												  Int iMBX,
												  Int iMBY,
												  CoordI x, 
												  CoordI y,
												  Int index,	//OBSS_SAIT_991015 //for OBSS
												  const PixelC *ppxlcCurrMBBY,
												  const PixelC *ppxlcCurrMBBUV
												  )
{
	//OBSS_SAIT_991015
	if(!(m_volmd.bSpatialScalability && m_volmd.iHierarchyType==0)) 		
		m_statsMB.nBitsShape += codeInterShape (
		ppxlcCurrQBY, m_vopmd.fShapeBPredDir==B_FORWARD ? m_pvopcRefQ0 : m_pvopcRefQ1,
		pmbmd, shpmdColocatedMB, NULL, pmvBY, x, y, iMBX, iMBY);
	
	else{	//for spatial scalability in shape
		if((m_volmd.volType == BASE_LAYER) || (!(m_volmd.iEnhnType==0 || m_volmd.iuseRefShape ==0) && !m_volmd.bShapeOnly) )
			m_statsMB.nBitsShape += codeInterShape (
			ppxlcCurrQBY, m_vopmd.fShapeBPredDir==B_FORWARD ? m_pvopcRefQ0 : m_pvopcRefQ1,
			pmbmd, shpmdColocatedMB, NULL, pmvBY, x, y, iMBX, iMBY);
		else if(m_volmd.volType == ENHN_LAYER){			
			m_statsMB.nBitsShape += codeSIShapeBVOP(		
				ppxlcCurrQBY,
				m_pvopcRefQ0,		//previous VOP
				m_pvopcRefQ1,		//reference layer VOP
				pmbmd, shpmdColocatedMB, NULL, pmvBY, m_rgmvBaseBY+index, x, y, iMBX, iMBY);
		}
	}
	//OBSS_SAIT_991015
	
	//Changed HHI 2000-04-11
	decideTransparencyStatus (pmbmd, m_ppxlcCurrMBBY); // change pmbmd to inter if all transparent
	downSampleBY (m_ppxlcCurrMBBY, m_ppxlcCurrMBBUV, pmbmd);

	if(m_volmd.bShapeOnly==FALSE) {	//OBSS_SAIT_991015	//for shape_only spatial scalability
		if (pmbmd->m_rgTranspStatus [0] != ALL) {
			if (pmbmd->m_rgTranspStatus [0] == PARTIAL)
				motionCompAndDiff_BVOP_MB_WithShape (pmv, pmvBackward, pmbmd, x, y, &m_rctRefVOPY0, &m_rctRefVOPY1);
			else
				motionCompAndDiff_BVOP_MB (pmv, pmvBackward, pmbmd, x, y, &m_rctRefVOPY0, &m_rctRefVOPY1);
			Bool bSkip = FALSE;
			//OBSS_SAIT_991015 //_SONY_SS_  //In the case of Foward && MV=0 for spatial scalable, Skip mode shuld be selected. 
			if (m_volmd.volType == ENHN_LAYER && m_vopmd.iRefSelectCode == 0 &&
				pmbmd->m_mbType == FORWARD)
				bSkip = (pmv->m_vctTrueHalfPel.x ==0 && pmv->m_vctTrueHalfPel.y == 0);
			else if (pmbmd->m_mbType == DIRECT)	{
				if(pmvRef == NULL)	//just to be safe
					pmbmd->m_vctDirectDeltaMV = pmv->m_vctTrueHalfPel;
				bSkip = (pmbmd->m_vctDirectDeltaMV.x == 0) && (pmbmd->m_vctDirectDeltaMV.y == 0);
			}
			//~OBSS_SAIT_991015
			if(m_volmd.fAUsage == EIGHT_BIT)
				motionCompAndDiffAlpha_BVOP_MB (
				pmv,
				pmvBackward, 
				pmbmd, 
				x, y,
				&m_rctRefVOPY0, &m_rctRefVOPY1
				);

			// HHI Schueuer: sadct
			if (!m_volmd.bSadctDisable)
				deriveSADCTRowLengths (m_rgiCurrMBCoeffWidth, m_ppxlcCurrMBBY, m_ppxlcCurrMBBUV, pmbmd->m_rgTranspStatus);
		
			if (!m_volmd.bSadctDisable)
				quantizeTextureInterMB (pmbmd, pmv, pppxlcCurrQMBA, bSkip, ppxlcCurrMBBY, ppxlcCurrMBBUV); // decide COD here; skip not allowed in non-direct mode
			else
				quantizeTextureInterMB (pmbmd, pmv, pppxlcCurrQMBA, bSkip); // decide COD here; skip not allowed in non-direct mode
			
			// end HHI		
			codeMBTextureHeadOfBVOP (pmbmd);
			
			if (!pmbmd -> m_bSkip) {
				m_statsMB.nBitsMV += encodeMVofBVOP (pmv, pmvBackward, pmbmd,
					iMBX, iMBY, pmvRef, pmbmdRef);	// need to see the MB type to decide what MV to be sent
				sendDCTCoefOfInterMBTexture (pmbmd);
			}
			else
				assignPredToCurrQ (ppxlcCurrQMBY, ppxlcCurrQMBU, ppxlcCurrQMBV);
			if (m_volmd.fAUsage == EIGHT_BIT) {
				for(Int iAuxComp=0;iAuxComp<m_volmd.iAuxCompCount;iAuxComp++) { // MAC (SB) 29-Nov-99
					codeMBAlphaHeadOfBVOP (pmbmd, iAuxComp);
					if (pmbmd -> m_pCODAlpha[iAuxComp] == ALPHA_CODED) {
						sendDCTCoefOfInterMBAlpha (pmbmd, iAuxComp );
					}
					else if(pmbmd -> m_pCODAlpha[iAuxComp] == ALPHA_SKIPPED)
						assignAlphaPredToCurrQ (pppxlcCurrQMBA[iAuxComp],iAuxComp);
				}
			}

			if (!pmbmd -> m_bSkip)
				addErrorAndPredToCurrQ (ppxlcCurrQMBY, ppxlcCurrQMBU, ppxlcCurrQMBV);
			if (m_volmd.fAUsage == EIGHT_BIT) {
				for(Int iAuxComp=0;iAuxComp<m_volmd.iAuxCompCount;iAuxComp++) { // MAC (SB) 29-Nov-99
					if (pmbmd -> m_pCODAlpha[iAuxComp] == ALPHA_CODED)
						addAlphaErrorAndPredToCurrQ (pppxlcCurrQMBA[iAuxComp],iAuxComp);
				}
			}
		}
		else
			// all transparent case
			cancelQPUpdate(pmbmd);

	}		//OBSS_SAIT_991015	//for BSO of OBSS
}

Void CVideoObjectEncoder::codeMBTextureHeadOfIVOP (CMBMode* pmbmd)
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
			CBPY |= pmbmd->getCodedBlockPattern (iBlk) << (cNonTrnspBlk - iBitPos);
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

Void CVideoObjectEncoder::codeMBTextureHeadOfPVOP (CMBMode* pmbmd, Bool *pbRestart)
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
			CBPY |= pmbmd->getCodedBlockPattern (iBlk) << (cNonTrnspBlk - iBitPos);
			iBitPos++;
		}
	}
	assert (CBPY >= 0 && CBPY <= 15);								//per defintion of H.263's CBPY 
	m_pbitstrmOut->putBits (pmbmd->m_bSkip, 1, "MB_Skip");
	m_statsMB.nBitsCOD++;

	if (pmbmd->m_bSkip)
	{
		// prevent delta qp from being used
		cancelQPUpdate(pmbmd);
		
		m_statsMB.nSkipMB++;
		// GMC
		if((m_uiSprite == 2 && m_vopmd.vopPredType == SPRITE))
			m_statsMB.nMCSELMB++;
		// ~GMC
	}
	else {
#ifdef __TRACE_AND_STATS_
	m_statsMB.nQMB++;
	m_statsMB.nQp += pmbmd->m_stepSize;
#endif // __TRACE_AND_STATS_

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
		// GMC
		if((pmbmd->m_dctMd == INTER || pmbmd->m_dctMd == INTERQ) && (pmbmd -> m_bhas4MVForward == FALSE) && (m_uiSprite == 2 && m_vopmd.vopPredType == SPRITE))
		{
			m_pbitstrmOut->putBits (pmbmd->m_bMCSEL, 1, "MCSEL");
			m_statsMB.nBitsMCSEL++;
		}
		// ~GMC
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
			// GMC
			else if (pmbmd-> m_bMCSEL)
				m_statsMB.nMCSELMB++;
			// ~GMC
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
			if((pmbmd->m_dctMd == INTER || pmbmd->m_dctMd == INTERQ )&&(pmbmd -> m_bhas4MVForward == FALSE)&&(pmbmd -> m_bMCSEL==FALSE)) { // GMC
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

	setDCVLCMode(pmbmd, pbRestart);
}

Void CVideoObjectEncoder::codeMBTextureHeadOfBVOP (CMBMode* pmbmd)
{
	U8 uchCBPB = 0;
	if (pmbmd->m_bSkip) {
		if (m_volmd.volType == BASE_LAYER)	{
			assert (pmbmd -> m_rgTranspStatus [0] != ALL);
			if (pmbmd->m_bColocatedMBSkip && !(pmbmd->m_bColocatedMBMCSEL)) // GMC
			{
				cancelQPUpdate(pmbmd);
				return;
			}
		}
		m_pbitstrmOut->putBits (1, 1, "MB_MODB");
		m_statsMB.nBitsMODB++;
		m_statsMB.nSkipMB++;
		if(m_volmd.volType == ENHN_LAYER && m_vopmd.iRefSelectCode == 0)
		{
			cancelQPUpdate(pmbmd);
			return;
		}
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
				iCBPY |= pmbmd->getCodedBlockPattern (iBlk) << (iNumNonTrnspBlk - iBitPos);
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
#ifdef __TRACE_AND_STATS_
	m_statsMB.nQMB++;
	m_statsMB.nQp += pmbmd->m_stepSize;
#endif // __TRACE_AND_STATS_
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
		else
			cancelQPUpdate(pmbmd);
	}
	else
		cancelQPUpdate(pmbmd);

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
												   const PixelC* ppxlcCurrBY, const PixelC** pppxlcCurrA,
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
		for(Int iAuxComp=0;iAuxComp<m_volmd.iAuxCompCount;iAuxComp++) { // MAC (SB) 26-Nov-99
			PixelC* ppxlcCurrMBA = m_ppxlcCurrMBA[iAuxComp];
			const PixelC* ppxlcCurrA   = pppxlcCurrA[iAuxComp];
			for (ic = 0; ic < MB_SIZE; ic++) {
				memcpy (ppxlcCurrMBA, ppxlcCurrA, MB_SIZE*sizeof(PixelC));
				ppxlcCurrMBA += MB_SIZE; 
				ppxlcCurrA += iWidthY;
			}
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
	// RRV modification
	for (ic = 0; ic < (BLOCK_SIZE *m_iRRVScale); ic++) {
		memcpy (ppxlcCurrMBY, ppxlcCurrY,
			(MB_SIZE *sizeof(PixelC) *m_iRRVScale));
		memcpy (ppxlcCurrMBU, ppxlcCurrU,
			(BLOCK_SIZE *sizeof(PixelC) *m_iRRVScale));
		memcpy (ppxlcCurrMBV, ppxlcCurrV,
			(BLOCK_SIZE* sizeof(PixelC) *m_iRRVScale));
		ppxlcCurrMBY += (MB_SIZE *m_iRRVScale);		ppxlcCurrY += iWidthY;
		ppxlcCurrMBU += (BLOCK_SIZE *m_iRRVScale);	ppxlcCurrU += iWidthUV;
		ppxlcCurrMBV += (BLOCK_SIZE *m_iRRVScale);	ppxlcCurrV += iWidthUV;
		
		memcpy (ppxlcCurrMBY, ppxlcCurrY,
			(MB_SIZE *sizeof(PixelC) *m_iRRVScale));	// two rows for Y
		ppxlcCurrMBY += (MB_SIZE *m_iRRVScale); ppxlcCurrY += iWidthY;
		//	for (ic = 0; ic < BLOCK_SIZE; ic++) {
		//		memcpy (ppxlcCurrMBY, ppxlcCurrY, MB_SIZE*sizeof(PixelC));
		//		memcpy (ppxlcCurrMBU, ppxlcCurrU, BLOCK_SIZE*sizeof(PixelC));
		//		memcpy (ppxlcCurrMBV, ppxlcCurrV, BLOCK_SIZE*sizeof(PixelC));
		//		ppxlcCurrMBY += MB_SIZE; ppxlcCurrY += iWidthY;
		//		ppxlcCurrMBU += BLOCK_SIZE; ppxlcCurrU += iWidthUV;
		//		ppxlcCurrMBV += BLOCK_SIZE;	ppxlcCurrV += iWidthUV;
		//
		//		memcpy (ppxlcCurrMBY, ppxlcCurrY, MB_SIZE*sizeof(PixelC)); // two rows for Y
		//		ppxlcCurrMBY += MB_SIZE; ppxlcCurrY += iWidthY;
		// ~RRV
	}
}

// compute error signal

Void CVideoObjectEncoder::computeTextureError ()
{
	CoordI ix;
	// RRV insertion
	Int	iTmp	= (m_iRRVScale *m_iRRVScale);
	// ~RRV
	// Y
	// RRV modification
	for (ix = 0; ix < (MB_SQUARE_SIZE *iTmp); ix++)
		//	for (ix = 0; ix < MB_SQUARE_SIZE; ix++)
		// ~RRV
		m_ppxliErrorMBY [ix] = m_ppxlcCurrMBY [ix] - m_ppxlcPredMBY [ix];
	
	// UV
	// RRV modification
	for (ix = 0; ix < (BLOCK_SQUARE_SIZE *iTmp); ix++) {
		//	for (ix = 0; ix < BLOCK_SQUARE_SIZE; ix++) {
		// ~RRV
		m_ppxliErrorMBU [ix] = m_ppxlcCurrMBU [ix] - m_ppxlcPredMBU [ix];
		m_ppxliErrorMBV [ix] = m_ppxlcCurrMBV [ix] - m_ppxlcPredMBV [ix];
	}
	
	// Alpha
	if(m_volmd.fAUsage==EIGHT_BIT) {
		for(Int iAuxComp=0;iAuxComp<m_volmd.iAuxCompCount;iAuxComp++) { // MAC (SB) 29-Nov-99
			
			for (ix = 0; ix < (MB_SQUARE_SIZE *iTmp); ix++)
				m_ppxliErrorMBA[iAuxComp][ix] = m_ppxlcCurrMBA[iAuxComp][ix] - m_ppxlcPredMBA[iAuxComp][ix];
		}
	}
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
	
	if(m_volmd.fAUsage==EIGHT_BIT) {
		for(Int iAuxComp=0;iAuxComp<m_volmd.iAuxCompCount;iAuxComp++) { // MAC (SB) 29-Nov-99
			for (ix = 0; ix < MB_SQUARE_SIZE; ix++) {
				if (m_ppxlcCurrMBBY [ix] == transpValue)
					m_ppxliErrorMBA[iAuxComp] [ix] = 0; // zero padding
				else
					m_ppxliErrorMBA[iAuxComp] [ix] = m_ppxlcCurrMBA[iAuxComp] [ix] - m_ppxlcPredMBA[iAuxComp] [ix];
			}
		}
	}
}

Void CVideoObjectEncoder::computeAlphaError ()
{
	CoordI ix;
	
	for(Int iAuxComp=0;iAuxComp<m_volmd.iAuxCompCount;iAuxComp++) { // MAC (SB) 29-Nov-99
		for (ix = 0; ix < MB_SQUARE_SIZE; ix++) {
			if (m_ppxlcCurrMBBY [ix] == transpValue)
				m_ppxliErrorMBA[iAuxComp] [ix] = 0; // zero padding
			else
				m_ppxliErrorMBA[iAuxComp] [ix] = m_ppxlcCurrMBA[iAuxComp] [ix] - m_ppxlcPredMBA[iAuxComp] [ix];
		}
	}
}

// HHI Schueuer: added const PixelC *ppxlcCurrMBBY, const PixelC *ppxlcCurrMBBUV for sadct
Void CVideoObjectEncoder::quantizeTextureIntraMB (	
												  Int imbX,
												  Int imbY,
												  CMBMode* pmbmd, 
												  PixelC* ppxlcCurrQMBY,
												  PixelC* ppxlcCurrQMBU,
												  PixelC* ppxlcCurrQMBV,
												  PixelC** pppxlcCurrQMBA,
												  const PixelC *ppxlcCurrMBBY,
												  const PixelC *ppxlcCurrMBBUV
												  )
{
	assert (pmbmd != NULL);
	assert (pmbmd -> m_dctMd == INTRA || pmbmd->m_dctMd == INTRAQ);
	assert (pmbmd -> m_rgTranspStatus [0] != ALL);
	
	pmbmd->m_bSkip = FALSE;		// for direct mode reference 
	
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
	
	Int iAuxComp;
	for(iAuxComp=0;iAuxComp<m_volmd.iAuxCompCount;iAuxComp++) // MAC (SB) 29-Nov-99
		pmbmd->m_pCODAlpha[iAuxComp] = ALPHA_CODED;
	
	if(m_volmd.fAUsage == EIGHT_BIT)
	{
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
			
			Int iAuxComp;
			for(iAuxComp=0;iAuxComp<m_volmd.iAuxCompCount;iAuxComp++) { // MAC (SB) 29-Nov-99
				
				pmbmd->m_pCODAlpha[iAuxComp] = ALPHA_ALL255;
				for(i = 0; i<MB_SQUARE_SIZE; i++) {
					if(m_ppxlcCurrMBA[iAuxComp][i]<=iThresh)
					{
						pmbmd->m_pCODAlpha[iAuxComp] = ALPHA_CODED;
						break;
					}
				}
				if(pmbmd->m_pCODAlpha[iAuxComp] == ALPHA_ALL255)
				{
					pxlcmemset(m_ppxlcCurrMBA[iAuxComp], 255, MB_SQUARE_SIZE);
					PixelC *ppxlc = pppxlcCurrQMBA[iAuxComp];
					for(i = 0; i<MB_SIZE; i++, ppxlc += m_iFrameWidthY)
						pxlcmemset(ppxlc, 255, MB_SIZE);
				}        
			}
		}	
	}
	
	Int iCoefToStart;
	if(pmbmd->m_bCodeDcAsAc)
		iCoefToStart = 0;
	else
		iCoefToStart = 1;

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
	
	// ~INTERLACE
	
	PixelC* rgchBlkDst = NULL;
	PixelC* rgchBlkSrc = NULL;
	Int iWidthDst, iWidthSrc;
	Int iDcScaler;
	Int* rgiCoefQ;
	Int iSumErr = 0; //sum of error to determine intra ac prediction
	Int iBlk;
	//	Int iBlkEnd;
	//	if(m_volmd.fAUsage == EIGHT_BIT)
	//		iBlkEnd = A_BLOCK4;
	//	else
	//		iBlkEnd = V_BLOCK;
	
	// HHI Schueuer: for sadct, see also the cond. assignments inside the next switch stmt.
	const PixelC* rgchBlkShape = NULL;
	// end
	
	for (iBlk = (Int) Y_BLOCK1; iBlk <= (Int)pmbmd->blkEnd(); iBlk++) { // + 1 is because of the indexing
		if (iBlk < (Int) U_BLOCK || iBlk > (Int) V_BLOCK) {
			if(iBlk>=A_BLOCK1 && ((iBlk-7)%4)==0)
				iSumErr = 0; // start again for alpha planes
			if (iBlk>=A_BLOCK1 && pmbmd->m_rgTranspStatus [((iBlk-7)%4)+1] == ALL)
				continue;
			else if (iBlk<A_BLOCK1 && pmbmd -> m_rgTranspStatus [iBlk % 6] == ALL) // %6 hack!!
				continue;
			
			
			switch (iBlk) 
			{
			case (Y_BLOCK1): 
				rgchBlkDst = ppxlcCurrQMBY;
				rgchBlkSrc = m_ppxlcCurrMBY;
				rgchBlkShape = (ppxlcCurrMBBY && pmbmd -> m_rgTranspStatus[iBlk] == PARTIAL) ? ppxlcCurrMBBY : NULL;
				break;
			case (Y_BLOCK2): 
				rgchBlkDst = ppxlcCurrQMBY + BLOCK_SIZE;
				rgchBlkSrc = m_ppxlcCurrMBY + BLOCK_SIZE;
				rgchBlkShape = (ppxlcCurrMBBY && pmbmd -> m_rgTranspStatus[iBlk] == PARTIAL) ? ppxlcCurrMBBY + BLOCK_SIZE : NULL;
				break;
			case (Y_BLOCK3): 
				// RRV modification
				rgchBlkDst = ppxlcCurrQMBY + (m_iFrameWidthYxBlkSize /m_iRRVScale);
				//				rgchBlkDst = ppxlcCurrQMBY + m_iFrameWidthYxBlkSize;
				// ~RRV
				rgchBlkSrc = m_ppxlcCurrMBY + MB_SIZE * BLOCK_SIZE;
				rgchBlkShape = (ppxlcCurrMBBY && pmbmd -> m_rgTranspStatus[iBlk] == PARTIAL) ? ppxlcCurrMBBY + BLOCK_SIZE*MB_SIZE : NULL;
				break;
			case (Y_BLOCK4): 
				// RRV modification
				rgchBlkDst = ppxlcCurrQMBY + (m_iFrameWidthYxBlkSize /m_iRRVScale) + BLOCK_SIZE;
				//				rgchBlkDst = ppxlcCurrQMBY + m_iFrameWidthYxBlkSize + BLOCK_SIZE;
				// ~RRV
				rgchBlkSrc = m_ppxlcCurrMBY + MB_SIZE * BLOCK_SIZE + BLOCK_SIZE;
				rgchBlkShape = (ppxlcCurrMBBY && pmbmd -> m_rgTranspStatus[iBlk] == PARTIAL) ? ppxlcCurrMBBY + BLOCK_SIZE*MB_SIZE + BLOCK_SIZE : NULL;
				break;
			}
			
			// MAC (SB) 29-Nov-99
			if (iBlk>=A_BLOCK1) { // alpha blocks
				Int iABlk = ((iBlk-7)&0x3);
				iAuxComp = (iBlk-7)/4;
				iABlk++;
				
				rgchBlkDst = pppxlcCurrQMBA[iAuxComp];
				rgchBlkSrc = m_ppxlcCurrMBA[iAuxComp];
				
				switch (iABlk) {
				case 1: rgchBlkShape = (ppxlcCurrMBBY && pmbmd -> m_rgTranspStatus[1] == PARTIAL) ? ppxlcCurrMBBY : NULL;
					break;
				case 2: rgchBlkDst += BLOCK_SIZE;
					rgchBlkSrc += BLOCK_SIZE;
					rgchBlkShape = (ppxlcCurrMBBY && pmbmd -> m_rgTranspStatus[2] == PARTIAL) ? ppxlcCurrMBBY + BLOCK_SIZE : NULL;        
					break;
				case 3: rgchBlkDst += m_iFrameWidthYxBlkSize;
					rgchBlkSrc += MB_SIZE * BLOCK_SIZE;
					rgchBlkShape = (ppxlcCurrMBBY && pmbmd -> m_rgTranspStatus[3] == PARTIAL) ? ppxlcCurrMBBY + BLOCK_SIZE*MB_SIZE : NULL;
					break;
				case 4: rgchBlkDst += m_iFrameWidthYxBlkSize + BLOCK_SIZE;
					rgchBlkSrc += MB_SIZE * BLOCK_SIZE + BLOCK_SIZE;
					rgchBlkShape = (ppxlcCurrMBBY && pmbmd -> m_rgTranspStatus[4] == PARTIAL) ? ppxlcCurrMBBY + BLOCK_SIZE*MB_SIZE + BLOCK_SIZE : NULL;
					break;
				}
			}
			//~MAC
			
			iWidthDst = m_iFrameWidthY;
			iWidthSrc = MB_SIZE;
			if(iBlk<=V_BLOCK)
				iDcScaler = iDcScalerY;
			else
				iDcScaler = iDcScalerA;
		}
		else {
			iWidthDst = m_iFrameWidthUV;
			iWidthSrc = BLOCK_SIZE;
			rgchBlkDst = (iBlk == U_BLOCK) ? ppxlcCurrQMBU: ppxlcCurrQMBV;
			rgchBlkSrc = (iBlk == U_BLOCK) ? m_ppxlcCurrMBU: m_ppxlcCurrMBV;
			// HHI: for sadct
			rgchBlkShape = (ppxlcCurrMBBUV && pmbmd -> m_rgTranspStatus[iBlk] == PARTIAL) ? ppxlcCurrMBBUV : NULL;
			iDcScaler = iDcScalerC;
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
			pmbmd,
			rgchBlkShape, ((iBlk<U_BLOCK) || (iBlk > V_BLOCK)) ? MB_SIZE : BLOCK_SIZE,	// HHI
			iAuxComp
			); //all for intra-pred
		if(iBlk>=A_BLOCK1)
			pmbmd->m_pbACPredictionAlpha[iAuxComp]	= (pmbmd->m_pCODAlpha[iAuxComp] == ALPHA_CODED && iSumErr >= 0);
		else
			pmbmd->m_bACPrediction =(iSumErr >= 0);
	}
	
	// INTERLACE
	if ((pmbmd->m_rgTranspStatus [0] == NONE) && (m_vopmd.bInterlace == TRUE) && (pmbmd->m_bFieldDCT == TRUE))
		fieldDCTtoFrameC(ppxlcCurrQMBY);
	// ~INTERLACE
	
	for (iBlk = (UInt) Y_BLOCK1; iBlk <= (Int)pmbmd->blkEnd(); iBlk++) { // + 1 is because of the indexing
		Int iBlkMap;
		if (iBlk>=A_BLOCK1) // alpha blocks
			iBlkMap = ((iBlk-7)&0x3)+1;
		else
			iBlkMap = iBlk;
		if (pmbmd->m_rgTranspStatus [iBlkMap % 6] == ALL) { // hack %6 ok if [6]==[0]
			pmbmd->setCodedBlockPattern (iBlk, FALSE);
			continue;
		}
		rgiCoefQ = m_rgpiCoefQ [iBlk - 1];
		if (iBlk < (Int) U_BLOCK) 
			iDcScaler = iDcScalerY; //m_rgiDcScalerY [iQP];
		else if(iBlk < (Int) A_BLOCK1)
			iDcScaler = iDcScalerC; //m_rgiDcScalerC [iQP];
		else
			iDcScaler = iDcScalerA;
		
		intraPred ( iBlk, pmbmd, rgiCoefQ,
			(iBlk<=V_BLOCK ? iQP : iQPA), iDcScaler, m_rgblkmCurrMB [iBlk - 1],
			m_rgiQPpred [iBlk - 1]);
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
		pmbmd->setCodedBlockPattern ( iBlk, bCoded);
	}
}

// HHI Schueuer: added const PixelC *ppxlcCurrMBBY, const PixelC *ppxlcCurrMBBUV 
Void CVideoObjectEncoder::quantizeTextureInterMB (CMBMode* pmbmd, 
                                                  const CMotionVector* pmv, 
                                                  PixelC **pppxlcCurrQMBA,
                                                  Bool bSkip,
                                                  const PixelC *ppxlcCurrMBBY,
                                                  const PixelC *ppxlcCurrMBBUV)	//bSkip: tested mv is zero
{
	// in case of qp changes, the dct is done with the new qp, and if the status is skip
	// then we keep this as a skip mb, but do not actually use the new qp, since it cannot
	// be sent - swinder
	assert (pmbmd != NULL);
	assert (pmbmd -> m_dctMd == INTER || pmbmd -> m_dctMd == INTERQ);
	assert (pmbmd->m_rgTranspStatus [0] != ALL);
	Int iQP = pmbmd->m_stepSize; // new (including dquant)
	
	if ((pmbmd->m_rgTranspStatus [0] == NONE) && (m_vopmd.bInterlace == TRUE)) {
		pmbmd->m_bFieldDCT = FrameFieldDCTDecideI(m_ppxliErrorMBY);
		m_statsMB.nFieldDCTMB += (Int) pmbmd->m_bFieldDCT;
	}
	else
		pmbmd->m_bFieldDCT = 0;
	Bool bMBCoded = FALSE;
	
	Int iQPA = 0;

	Int iAuxComp;
	for(iAuxComp=0;iAuxComp<m_volmd.iAuxCompCount;iAuxComp++)  // MAC (SB) 29-Nov-99 
		pmbmd->m_pCODAlpha[iAuxComp] = ALPHA_CODED;
	
	if(m_volmd.fAUsage == EIGHT_BIT)
	{
		iQPA = pmbmd->m_stepSizeAlpha;
		Int i, iThresh = 256 - iQPA;
		
		for(Int iAuxComp=0;iAuxComp<m_volmd.iAuxCompCount;iAuxComp++) { // MAC (SB) 29-Nov-99
			pmbmd->m_pCODAlpha[iAuxComp] = ALPHA_ALL255;
			for(i = 0; i<MB_SQUARE_SIZE; i++) {
				if(m_ppxlcCurrMBA[iAuxComp][i] <= iThresh)
				{
					pmbmd->m_pCODAlpha[iAuxComp] = ALPHA_CODED;
					break;
				}
			}
		}
	}
	
	// Bool bSkip = pmbmd->m_bhas4MVForward ? (bSkipAllowed && pmv [1].isZero () && pmv [2].isZero () && pmv [3].isZero () && pmv [4].isZero ())
	//										 : bSkipAllowed && pmv->isZero ();
	// the above is actually already passed into this function
	Int* rgiBlkCurrQ = m_ppxliErrorMBY;
	Int* rgiCoefQ;
	Int iWidthCurrQ;
	
	Bool* pbSkipAlpha = new Bool [m_volmd.iAuxCompCount];
	for(; iAuxComp<m_volmd.iAuxCompCount; iAuxComp++ )
		pbSkipAlpha[iAuxComp] = TRUE;
	
	// HHI Schueuer: sadct
	const PixelC* rgchBlkShape = NULL;
	// end HHI
	
	Bool bAlphaQPReset = FALSE;
	Bool bCBPY = FALSE;
	for (UInt iBlk = (UInt) Y_BLOCK1; iBlk <= (UInt) pmbmd->blkEnd(); iBlk++) { 
		if (iBlk < (UInt) U_BLOCK || iBlk> (UInt) V_BLOCK) {
			
			if (iBlk>=A_BLOCK1 && pmbmd->m_rgTranspStatus [((iBlk-7)%4)+1] == ALL) // MAC patch
				continue;
			else if (iBlk<A_BLOCK1 && pmbmd -> m_rgTranspStatus [iBlk % 6] == ALL) // %6 hack!!
				continue;
			
			switch (iBlk) 
			{
			case (Y_BLOCK1): 
				rgiBlkCurrQ = m_ppxliErrorMBY;
				rgchBlkShape = (ppxlcCurrMBBY && pmbmd -> m_rgTranspStatus[iBlk] == PARTIAL) ? ppxlcCurrMBBY : NULL;
				break;
			case (Y_BLOCK2): 
				rgiBlkCurrQ = m_ppxliErrorMBY + BLOCK_SIZE;
				rgchBlkShape = (ppxlcCurrMBBY && pmbmd -> m_rgTranspStatus[iBlk] == PARTIAL) ? ppxlcCurrMBBY + BLOCK_SIZE: NULL;
				break;
			case (Y_BLOCK3): 
				rgiBlkCurrQ = m_ppxliErrorMBY + MB_SIZE * BLOCK_SIZE;
				rgchBlkShape = (ppxlcCurrMBBY && pmbmd -> m_rgTranspStatus[iBlk] == PARTIAL) ? ppxlcCurrMBBY + BLOCK_SIZE*MB_SIZE : NULL;
				break;
			case (Y_BLOCK4): 
				rgiBlkCurrQ = m_ppxliErrorMBY + MB_SIZE * BLOCK_SIZE + BLOCK_SIZE;
				rgchBlkShape = (ppxlcCurrMBBY && pmbmd -> m_rgTranspStatus[iBlk] == PARTIAL) ? ppxlcCurrMBBY + BLOCK_SIZE*MB_SIZE + BLOCK_SIZE : NULL;
				break;
				
			}
			// MAC (SB) 29-Nov-99
			if (iBlk>=A_BLOCK1) {
				Int iBlkA = (iBlk-7)&0x3;
				iAuxComp  = iBlkA/4;
				
				rgiBlkCurrQ = m_ppxliErrorMBA[iAuxComp];
				
				switch (iBlkA) {
				case 0: rgchBlkShape = (ppxlcCurrMBBY && pmbmd -> m_rgTranspStatus[1] == PARTIAL) ? ppxlcCurrMBBY : NULL;
					break;
				case 1: rgiBlkCurrQ += BLOCK_SIZE;
					rgchBlkShape = (ppxlcCurrMBBY && pmbmd -> m_rgTranspStatus[2] == PARTIAL) ? ppxlcCurrMBBY + BLOCK_SIZE: NULL;
					break;
				case 2: rgiBlkCurrQ += MB_SIZE * BLOCK_SIZE;
					rgchBlkShape = (ppxlcCurrMBBY && pmbmd -> m_rgTranspStatus[3] == PARTIAL) ? ppxlcCurrMBBY + BLOCK_SIZE*MB_SIZE : NULL;
					break;
				case 3: rgiBlkCurrQ += MB_SIZE * BLOCK_SIZE + BLOCK_SIZE;
					rgchBlkShape = (ppxlcCurrMBBY && pmbmd -> m_rgTranspStatus[4] == PARTIAL) ? ppxlcCurrMBBY + BLOCK_SIZE*MB_SIZE + BLOCK_SIZE : NULL;
					break;
				}
			}
			//~MAC
			
			iWidthCurrQ = MB_SIZE;
		}
		else {
			iWidthCurrQ = BLOCK_SIZE;
			rgiBlkCurrQ = (iBlk == U_BLOCK) ? m_ppxliErrorMBU: m_ppxliErrorMBV;
			rgchBlkShape = (ppxlcCurrMBBUV && pmbmd -> m_rgTranspStatus[iBlk] == PARTIAL) ? ppxlcCurrMBBUV : NULL;
		}
		
		rgiCoefQ = m_rgpiCoefQ [iBlk - 1];
		if(iBlk>=A_BLOCK1 && !bAlphaQPReset)
		{
			if(bSkip || m_vopmd.vopPredType == BVOP && !bCBPY)
			{
				// if found that texture is not coded, then must cancel qp updates and recalculate quantiser for alpha
				// because alpha may still be coded but we cant change alpha qp if alpha qp is slaved to texture qp
				// in bvops, cbpy=0 also prevents qp updates, but not in pvops
				cancelQPUpdate(pmbmd);
				iQPA = pmbmd->m_stepSizeAlpha;

				bAlphaQPReset = TRUE;
			}
		}

		if(iBlk>=A_BLOCK1)
			quantizeTextureInterBlock (rgiBlkCurrQ, iWidthCurrQ, rgiCoefQ, iQPA, TRUE, rgchBlkShape, ((iBlk<U_BLOCK) || (iBlk >V_BLOCK)) ? MB_SIZE : BLOCK_SIZE, iBlk); // HHI Schueuer
		else
			quantizeTextureInterBlock (rgiBlkCurrQ, iWidthCurrQ, rgiCoefQ, iQP, FALSE, rgchBlkShape, (iBlk<U_BLOCK) ? MB_SIZE : BLOCK_SIZE, iBlk);
		// end
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
			pbSkipAlpha[(iBlk-7)/4] = pbSkipAlpha[(iBlk-7)/4] & !bCoded;
		
		if(iBlk <= A_BLOCK1)
		{
			bCBPY |= bCoded;
		}
		pmbmd->setCodedBlockPattern (iBlk, bCoded);
	}
	
	pmbmd->m_bSkip = bSkip; 
	
	if(m_volmd.fAUsage == EIGHT_BIT)
	{ 
		for(Int iAuxComp=0;iAuxComp<m_volmd.iAuxCompCount;iAuxComp++) { // MAC (SB) 30-Nov-99
			if(pbSkipAlpha[iAuxComp] == TRUE)
				pmbmd->m_pCODAlpha[iAuxComp] = ALPHA_SKIPPED;
			else if(pmbmd->m_pCODAlpha[iAuxComp] == ALPHA_ALL255)
			{
				PixelC *ppxlc = pppxlcCurrQMBA[iAuxComp];
				Int i;
				for(i = 0; i<MB_SIZE; i++, ppxlc += m_iFrameWidthY)
					pxlcmemset(ppxlc, 255, MB_SIZE);
			}
		}
	}
	
	delete [] pbSkipAlpha;
	
	// INTERLACE
	if ((pmbmd->m_rgTranspStatus [0] == NONE)
		&&(m_vopmd.bInterlace == TRUE) && (pmbmd->m_bFieldDCT == TRUE) 
		&& (bMBCoded == TRUE))
		fieldDCTtoFrameI(m_ppxliErrorMBY);
	// ~INTERLACE
}

//OBSS_SAIT_991015
Bool CVideoObjectEncoder::quantizeTextureMBBackwardCheck (CMBMode* pmbmd, 
														  PixelC *ppxlcCurrMBY,
														  PixelC *ppxlcCurrMBBY,
														  const PixelC *ppxlcRef1MBY)
														  
{
	Int bSkip = TRUE;
	Int *ppxliErrorMBY;
	ppxliErrorMBY = new Int [MB_SIZE*MB_SIZE];
	pmbmd->m_stepSize = m_vopmd.intStepB;
	
	Int iQP = pmbmd->m_stepSize;
	
	Int iBlkEnd = Y_BLOCK4;
	
	for(int i=0;i<MB_SIZE;i++){
		for(int j=0;j<MB_SIZE;j++){
			if(*(ppxlcCurrMBBY+i*MB_SIZE+j) == transpValue)
				*(ppxliErrorMBY+i*MB_SIZE+j) = 0;
			else
				*(ppxliErrorMBY+i*MB_SIZE+j) = *(ppxlcCurrMBY +i*MB_SIZE+j) - *(ppxlcRef1MBY +i*448+j);
		}
	}
	Int* rgiBlkCurrQ = ppxliErrorMBY;
	Int* rgiCoefQ;
	Int iWidthCurrQ;
	for (UInt iBlk = (UInt) Y_BLOCK1; iBlk <= (UInt)iBlkEnd; iBlk++) { 
		if (pmbmd -> m_rgTranspStatus [iBlk % 6] == ALL) 
			continue;		
		switch (iBlk) 
		{
		case (Y_BLOCK1): 
			rgiBlkCurrQ = ppxliErrorMBY;
			break;
		case (Y_BLOCK2): 
			rgiBlkCurrQ = ppxliErrorMBY + BLOCK_SIZE;
			break;
		case (Y_BLOCK3): 
			rgiBlkCurrQ = ppxliErrorMBY + MB_SIZE * BLOCK_SIZE;
			break;
		case (Y_BLOCK4): 
			rgiBlkCurrQ = ppxliErrorMBY + MB_SIZE * BLOCK_SIZE + BLOCK_SIZE;
			break;
		}
		iWidthCurrQ = MB_SIZE;
		rgiCoefQ = m_rgpiCoefQ [iBlk - 1];
		quantizeTextureInterBlock (rgiBlkCurrQ, iWidthCurrQ, rgiCoefQ, iQP, FALSE, NULL, (iBlk<U_BLOCK) ? MB_SIZE : BLOCK_SIZE, iBlk);
		Bool bCoded = FALSE;
		UInt i;
		for (i = 0; i < BLOCK_SQUARE_SIZE; i++) {
			if (rgiCoefQ [i] != 0)	{
				bCoded = TRUE;
				break;
			}
		}
		bSkip = bSkip & !bCoded;
	}
	
	return (bSkip);
}
//~OBSS_SAIT_991015

Void CVideoObjectEncoder::sendDCTCoefOfInterMBTexture (const CMBMode* pmbmd) 
{
	assert (pmbmd != NULL);
	assert (pmbmd -> m_dctMd == INTER || pmbmd -> m_dctMd == INTERQ);
	assert (pmbmd -> m_rgTranspStatus [0] != ALL);
	
	Int* scan = grgiStandardZigzag;  // HHI Schueuer
	UInt nBits, iBlk = 0;
	for (iBlk = Y_BLOCK1; iBlk <= V_BLOCK; iBlk++) {
#ifdef __TRACE_AND_STATS_
		m_pbitstrmOut->trace (iBlk, "BLK_NO");
#endif // __TRACE_AND_STATS_
		if (iBlk < U_BLOCK)
			if (pmbmd -> m_rgTranspStatus [iBlk] == ALL) continue;
			if (pmbmd->getCodedBlockPattern (iBlk))	{
				Int* rgiCoefQ = m_rgpiCoefQ [iBlk - 1];
#ifdef __TRACE_AND_STATS_
				m_pbitstrmOut->trace (rgiCoefQ, BLOCK_SQUARE_SIZE, "BLK_QUANTIZED_COEF");
#endif // __TRACE_AND_STATS_
				// Modified for data partitioning mode by Toshiba(1998-1-16)
				if(m_volmd.bDataPartitioning && m_volmd.bReversibleVlc && m_vopmd.vopPredType != BVOP) {
					// HHI Schueuer: modified scan for sadcdt
					if (!m_volmd.bSadctDisable)
						scan = m_pscanSelector->select(scan, (pmbmd->m_rgTranspStatus[0] == PARTIAL), iBlk);
					// nBits = sendTCOEFInterRVLC (rgiCoefQ, 0, grgiStandardZigzag, TRUE);
					nBits = sendTCOEFInterRVLC (rgiCoefQ, 0, scan, TRUE);
					// end HHI
				}				
				else {
					// HHI Schueuer: modified scan for sadct
					// nBits = sendTCOEFInter (rgiCoefQ, 0,
					// m_vopmd.bAlternateScan ? grgiVerticalZigzag : grgiStandardZigzag);
					scan = (m_vopmd.bAlternateScan) ? grgiVerticalZigzag : grgiStandardZigzag;
					if (!m_volmd.bSadctDisable) 
						scan = m_pscanSelector->select(scan, (pmbmd->m_rgTranspStatus[0] == PARTIAL), iBlk);
					nBits = sendTCOEFInter (rgiCoefQ, 0, scan );
					// end HHI
				}
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
				nBits = sendIntraDC (rgiCoefQ, iBlk);
			}
			if (pmbmd->getCodedBlockPattern (iBlk))	{
				Int* rgiZigzag = grgiStandardZigzag;
				if (m_vopmd.bAlternateScan)
					rgiZigzag = grgiVerticalZigzag;
				else if (pmbmd->m_bACPrediction)	
					rgiZigzag = (pmbmd->m_preddir [iBlk - 1] == HORIZONTAL) ? grgiVerticalZigzag : grgiHorizontalZigzag;
				// HHI Schueuer: modified scan for sadct
				if (!m_volmd.bSadctDisable)
					rgiZigzag = m_pscanSelector->select(rgiZigzag, (pmbmd->m_rgTranspStatus[0] == PARTIAL), iBlk); 
				// end HHI
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

Void CVideoObjectEncoder::codeMBAlphaHeadOfIVOP (const CMBMode* pmbmd, Int iAuxComp)
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
			CBPA |= pmbmd->getCodedBlockPattern (iBlk+iAuxComp*4) << (cNonTrnspBlk - iBitPos);
			iBitPos++;
		}
	}
	assert (CBPA >= 0 && CBPA <= 15);
	
	Int iCODA = 0;
	
	if(pmbmd->m_pCODAlpha[iAuxComp]==ALPHA_ALL255)
		iCODA = 1;
	
	m_pbitstrmOut->putBits(iCODA, 1, "MB_CODA");
	m_statsMB.nBitsCODA++;
	
	if(iCODA)
		return;
	
	m_pbitstrmOut->putBits (pmbmd->m_pbACPredictionAlpha[iAuxComp], 1, "MB_ACPRED_ALPHA");
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

Void CVideoObjectEncoder::codeMBAlphaHeadOfPVOP (const CMBMode* pmbmd, Int iAuxComp)
{
	if(pmbmd -> m_dctMd == INTRA || pmbmd -> m_dctMd == INTRAQ)
		codeMBAlphaHeadOfIVOP(pmbmd,iAuxComp);
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
				CBPA |= pmbmd->getCodedBlockPattern (iBlk+iAuxComp*4) << (cNonTrnspBlk - iBitPos);
				iBitPos++;
			}
		}
		assert (CBPA >= 0 && CBPA <= 15);
		
		if(pmbmd->m_pCODAlpha[iAuxComp]==ALPHA_CODED)
		{
			m_pbitstrmOut->putBits(0, 2, "MB_CODA");
			m_statsMB.nBitsCODA += 2;
		}
		else if(pmbmd->m_pCODAlpha[iAuxComp]==ALPHA_ALL255)
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

Void CVideoObjectEncoder::sendDCTCoefOfIntraMBAlpha (const CMBMode* pmbmd, Int iAuxComp)
{
	assert (pmbmd != NULL);
	assert (pmbmd -> m_dctMd == INTRA || pmbmd -> m_dctMd == INTRAQ);
	assert (pmbmd->m_rgTranspStatus [0] != ALL);
	
	if(pmbmd->m_pCODAlpha[iAuxComp] != ALPHA_CODED)
		return;
	
	UInt iBlk;
	for (iBlk = A_BLOCK1; iBlk <= A_BLOCK4; iBlk++) {
		UInt nBits = 0;
#ifdef __TRACE_AND_STATS_
		m_pbitstrmOut->trace (iBlk+iAuxComp*4, "ALPHA_BLK_NO");
#endif // __TRACE_AND_STATS_
		if (pmbmd -> m_rgTranspStatus [iBlk - 6 ] == ALL)
			continue;
		Int* rgiCoefQ = m_rgpiCoefQ [iBlk+iAuxComp*4 - 1];
#ifdef __TRACE_AND_STATS_
		m_pbitstrmOut->trace (rgiCoefQ, BLOCK_SQUARE_SIZE, "BLK_QUANTIZED_COEF");
#endif // __TRACE_AND_STATS_
		
		Int iCoefStart = 0;
		if (pmbmd->m_bCodeDcAsAcAlpha != TRUE)	{
			iCoefStart = 1;
			nBits = sendIntraDC (rgiCoefQ, iBlk+iAuxComp*4); // MAC ok - iBlk is used for Y-U,V switch
		}
		
		if (pmbmd->getCodedBlockPattern (iBlk+iAuxComp*4))	{
			Int* rgiZigzag = grgiStandardZigzag;
			if (pmbmd->m_pbACPredictionAlpha[iAuxComp])	
				rgiZigzag = (pmbmd->m_preddir [iBlk - 1] == HORIZONTAL) ? grgiVerticalZigzag : grgiHorizontalZigzag;
			// HHI Schueuer: sadct
			if (!m_volmd.bSadctDisable)
				rgiZigzag = m_pscanSelector->select(rgiZigzag, (pmbmd->m_rgTranspStatus[0] == PARTIAL), iBlk); 
			// end HHI
			nBits += sendTCOEFIntra (rgiCoefQ, iCoefStart, rgiZigzag);
		}
		m_statsMB.nBitsA += nBits;
	}	
}

Void CVideoObjectEncoder::sendDCTCoefOfInterMBAlpha (const CMBMode* pmbmd, Int iAuxComp)
{
	assert (pmbmd != NULL);
	assert (pmbmd -> m_dctMd == INTER || pmbmd -> m_dctMd == INTERQ);
	assert (pmbmd -> m_rgTranspStatus [0] != ALL);
	assert (pmbmd -> m_pCODAlpha[iAuxComp] == ALPHA_CODED);
	
	UInt nBits, iBlk = 0;
	Int* scan = grgiStandardZigzag;  // HHI Schueuer: sadct
	for (iBlk = A_BLOCK1; iBlk <= A_BLOCK4; iBlk++) {
#ifdef __TRACE_AND_STATS_
		m_pbitstrmOut->trace (iBlk, "BLK_NO");
#endif // __TRACE_AND_STATS_
		if (pmbmd -> m_rgTranspStatus [iBlk - 6] == ALL) continue;
		if (pmbmd->getCodedBlockPattern(iBlk) )	{
			Int* rgiCoefQ = m_rgpiCoefQ [iBlk - 1];
#ifdef __TRACE_AND_STATS_
			m_pbitstrmOut->trace (rgiCoefQ, BLOCK_SQUARE_SIZE, "BLK_QUANTIZED_COEF");
#endif // __TRACE_AND_STATS_
			// HHI Schueuer: sadct
			if (!m_volmd.bSadctDisable) 
				scan = m_pscanSelector->select(grgiStandardZigzag, (pmbmd->m_rgTranspStatus[0] == PARTIAL), iBlk);
			// nBits = sendTCOEFInter (rgiCoefQ, 0, grgiStandardZigzag);
			nBits = sendTCOEFInter (rgiCoefQ, 0, scan);
			// end HHI
			m_statsMB.nBitsA += nBits;
		}
	}	
}

Void CVideoObjectEncoder::codeMBAlphaHeadOfBVOP (const CMBMode* pmbmd, Int iAuxComp)
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
			CBPA |= pmbmd->getCodedBlockPattern (iBlk+iAuxComp*4) << (cNonTrnspBlk - iBitPos);
			iBitPos++;
		}
	}
	assert (CBPA >= 0 && CBPA <= 15);
	
	if(pmbmd->m_pCODAlpha[iAuxComp]==ALPHA_CODED)
	{
		m_pbitstrmOut->putBits(0, 2, "MB_CODBA");
		m_statsMB.nBitsCODA += 2;
	}
	else if(pmbmd->m_pCODAlpha[iAuxComp]==ALPHA_ALL255)
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




/*************************************************************************

This software module was originally developed by 

	Yoshihiro Kikuchi (TOSHIBA CORPORATION)
	Takeshi Nagai (TOSHIBA CORPORATION)

    and edited by:

	Toshiaki Watanabe (TOSHIBA CORPORATION)
	Noboru Yamaguchi (TOSHIBA CORPORATION)

  in the course of development of the <MPEG-4 Video(ISO/IEC 14496-2)>. This
  software module is an implementation of a part of one or more <MPEG-4 Video
  (ISO/IEC 14496-2)> tools as specified by the <MPEG-4 Video(ISO/IEC 14496-2)
  >. ISO/IEC gives users of the <MPEG-4 Video(ISO/IEC 14496-2)> free license
  to this software module or modifications thereof for use in hardware or
  software products claiming conformance to the <MPEG-4 Video(ISO/IEC 14496-2
  )>. Those intending to use this software module in hardware or software
  products are advised that its use may infringe existing patents. The
  original developer of this software module and his/her company, the
  subsequent editors and their companies, and ISO/IEC have no liability for
  use of this software module or modifications thereof in an implementation.
  Copyright is not released for non <MPEG-4 Video(ISO/IEC 14496-2)>
  conforming products. TOSHIBA CORPORATION retains full right to use the code
  for his/her own purpose, assign or donate the code to a third party and to
  inhibit third parties from using the code for non <MPEG-4 Video(ISO/IEC
  14496-2)> conforming products. This copyright notice must be included in
  all copies or derivative works.
  Copyright (c)1997.

*************************************************************************/

// Added for error resilience mode By Toshiba
#include <stdio.h>
#include <math.h>
#include <stdlib.h>

#include "typeapi.h"
#include "mode.hpp"
#include "codehead.h"
#include "entropy/bitstrm.hpp"
#include "entropy/entropy.hpp"
#include "entropy/huffman.hpp"
#include "global.hpp"
#include "vopses.hpp"
#include "vopsedec.hpp"

#include "dct.hpp"

#ifdef __MFC_
#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif
#define new DEBUG_NEW				   
#endif // __MFC_

Bool CVideoObjectDecoder::checkResyncMarker()
{
	Int nBitsPeeked;
	Int iStuffedBits = m_pbitstrmIn->peekBitsTillByteAlign (nBitsPeeked);
	Int nBitsResyncMarker = NUMBITS_VP_RESYNC_MARKER;
	if(m_volmd.bShapeOnly==FALSE)
	{
		if(m_vopmd.vopPredType == PVOP)
			nBitsResyncMarker += (m_vopmd.mvInfoForward.uiFCode - 1);
		else if(m_vopmd.vopPredType == BVOP)
			nBitsResyncMarker += MAX(m_vopmd.mvInfoForward.uiFCode, m_vopmd.mvInfoBackward.uiFCode) - 1;
	}
		
	assert (nBitsPeeked > 0 && nBitsPeeked <= 8);
	if (iStuffedBits == ((1 << (nBitsPeeked - 1)) - 1))
		return (m_pbitstrmIn->peekBitsFromByteAlign (nBitsResyncMarker) == RESYNC_MARKER);
	return FALSE;
}

#if 0 // revised HEC for shape
Int	CVideoObjectDecoder::decodeVideoPacketHeader(Int& iCurrentQP)
{
	m_pbitstrmIn -> flush(8);

	Int nBitsResyncMarker = NUMBITS_VP_RESYNC_MARKER;
	if(m_volmd.bShapeOnly==FALSE)
	{
		if(m_vopmd.vopPredType == PVOP)
			nBitsResyncMarker += (m_vopmd.mvInfoForward.uiFCode - 1);
		else if(m_vopmd.vopPredType == BVOP)
			nBitsResyncMarker += MAX(m_vopmd.mvInfoForward.uiFCode, m_vopmd.mvInfoBackward.uiFCode) - 1;
	}

	UInt uiResyncMarker = m_pbitstrmIn -> getBits (nBitsResyncMarker);

	Int	NumOfMB = m_iNumMBX * m_iNumMBY;
	assert (NumOfMB>0);

	//Int LengthOfMBNumber = (Int)(log(NumOfMB-1)/log(2)) + 1;

	Int iVal = NumOfMB - 1;
	Int iLengthOfMBNumber = 0;
	for(; iVal; iLengthOfMBNumber++)
		iVal>>=1;

	UInt uiMBNumber = 0;
	
	if(NumOfMB>1)
		uiMBNumber = m_pbitstrmIn -> getBits (iLengthOfMBNumber);
	
	m_iVPMBnum = uiMBNumber;
	if(m_volmd.bShapeOnly==FALSE) {
		Int stepDecoded = m_pbitstrmIn -> getBits (NUMBITS_VP_QUANTIZER);
		iCurrentQP = stepDecoded; 
	}

	UInt uiHEC = m_pbitstrmIn -> getBits (NUMBITS_VP_HEC);
	if (uiHEC){

		// Time reference and VOP_pred_type
		Int iModuloInc = 0;
		while (m_pbitstrmIn -> getBits (1) != 0)
			iModuloInc++;
		Time tCurrSec = iModuloInc + (m_vopmd.vopPredType != BVOP ? m_tOldModuloBaseDecd : m_tOldModuloBaseDisp);
		UInt uiMarker = m_pbitstrmIn -> getBits (1);
		assert(uiMarker == 1);
		Time tVopIncr = m_pbitstrmIn -> getBits (m_iNumBitsTimeIncr);
		uiMarker = m_pbitstrmIn -> getBits (1);
		assert(uiMarker == 1);
		// this is bogus - swinder.
		//assert (m_t == tCurrSec * 60 + tVopIncr * 60 / m_volmd.iClockRate); //in terms of 60Hz clock ticks

		VOPpredType vopPredType = (VOPpredType) m_pbitstrmIn -> getBits (NUMBITS_VP_PRED_TYPE);
		assert(m_vopmd.vopPredType == vopPredType);

		if(m_volmd.bShapeOnly==FALSE) {
			Int	iIntraDcSwitchThr = m_pbitstrmIn->getBits (NUMBITS_VP_INTRA_DC_SWITCH_THR);
			assert(m_vopmd.iIntraDcSwitchThr == iIntraDcSwitchThr);
			if (m_vopmd.vopPredType == PVOP) {
				UInt uiFCode = m_pbitstrmIn -> getBits (NUMBITS_VOP_FCODE);
				assert(uiFCode == m_vopmd.mvInfoForward.uiFCode);
			}
			else if (m_vopmd.vopPredType == BVOP) {
				UInt uiForwardFCode = m_pbitstrmIn -> getBits (NUMBITS_VOP_FCODE);
				UInt uiBackwardFCode = m_pbitstrmIn -> getBits (NUMBITS_VOP_FCODE);
				assert(uiForwardFCode == m_vopmd.mvInfoForward.uiFCode);
				assert(uiBackwardFCode == m_vopmd.mvInfoBackward.uiFCode);
			}
		}
	}
	return	TRUE;
}
#else
Int	CVideoObjectDecoder::decodeVideoPacketHeader(Int& iCurrentQP)
{
  UInt uiHEC = 0;
	m_pbitstrmIn -> flush(8);

	Int nBitsResyncMarker = NUMBITS_VP_RESYNC_MARKER;
	if(m_volmd.bShapeOnly==FALSE)
	{
		if(m_vopmd.vopPredType == PVOP)
			nBitsResyncMarker += (m_vopmd.mvInfoForward.uiFCode - 1);
		else if(m_vopmd.vopPredType == BVOP)
			nBitsResyncMarker += MAX(m_vopmd.mvInfoForward.uiFCode, m_vopmd.mvInfoBackward.uiFCode) - 1;
	}

	/* UInt uiResyncMarker = wmay */ m_pbitstrmIn -> getBits (nBitsResyncMarker);

	Int	NumOfMB = m_iNumMBX * m_iNumMBY;
	assert (NumOfMB>0);

	//Int LengthOfMBNumber = (Int)(log(NumOfMB-1)/log(2)) + 1;

	Int iVal = NumOfMB - 1;
	Int iLengthOfMBNumber = 0;
	for(; iVal; iLengthOfMBNumber++)
		iVal>>=1;

	UInt uiMBNumber = 0;
	
	if (m_volmd.fAUsage != RECTANGLE) {
		uiHEC = m_pbitstrmIn -> getBits (NUMBITS_VP_HEC);
	  if (uiHEC && !(m_uiSprite == 1 && m_vopmd.vopPredType == IVOP)) {
	    /* Int width = wmay */ m_pbitstrmIn -> getBits (NUMBITS_VOP_WIDTH);
		Int marker;
		marker = m_pbitstrmIn -> getBits (1); // marker bit
		assert(marker==1);
		/* Int height = wmay */ m_pbitstrmIn -> getBits (NUMBITS_VOP_HEIGHT);
		marker = m_pbitstrmIn -> getBits (1); // marker bit
		assert(marker==1);
		//wchen: cd changed to 2s complement
		Int left = (m_pbitstrmIn -> getBits (1) == 0) ?
					m_pbitstrmIn->getBits (NUMBITS_VOP_HORIZONTAL_SPA_REF - 1) : 
					((Int)m_pbitstrmIn->getBits (NUMBITS_VOP_HORIZONTAL_SPA_REF - 1) - (1 << (NUMBITS_VOP_HORIZONTAL_SPA_REF - 1)));
		marker = m_pbitstrmIn -> getBits (1); // marker bit
		assert(marker==1);
		Int top = (m_pbitstrmIn -> getBits (1) == 0) ?
				   m_pbitstrmIn->getBits (NUMBITS_VOP_VERTICAL_SPA_REF - 1) : 
				   ((Int)m_pbitstrmIn->getBits (NUMBITS_VOP_VERTICAL_SPA_REF - 1) - (1 << (NUMBITS_VOP_VERTICAL_SPA_REF - 1)));
		marker = m_pbitstrmIn -> getBits (1); // marker bit
		assert(marker==1);
		assert(((left | top)&1)==0); // must be even pix unit
	  }
	}

	if(NumOfMB>1)
		uiMBNumber = m_pbitstrmIn -> getBits (iLengthOfMBNumber);
	
	m_iVPMBnum = uiMBNumber;
	if(m_volmd.bShapeOnly==FALSE) {
		Int stepDecoded = m_pbitstrmIn -> getBits (NUMBITS_VP_QUANTIZER);
		iCurrentQP = stepDecoded; 
	}

	if (m_volmd.fAUsage == RECTANGLE)
		uiHEC = m_pbitstrmIn -> getBits (NUMBITS_VP_HEC);
		
	if (uiHEC){

		// Time reference and VOP_pred_type
		Int iModuloInc = 0;
		while (m_pbitstrmIn -> getBits (1) != 0)
			iModuloInc++;
		//Time tCurrSec = iModuloInc + (m_vopmd.vopPredType != BVOP ? m_tOldModuloBaseDecd : m_tOldModuloBaseDisp);
		UInt uiMarker = m_pbitstrmIn -> getBits (1);
		assert(uiMarker == 1);
		/* Time tVopIncr = wmay */ m_pbitstrmIn -> getBits (m_iNumBitsTimeIncr);
		uiMarker = m_pbitstrmIn -> getBits (1);
		assert(uiMarker == 1);
		// this is bogus - swinder.
		//assert (m_t == tCurrSec * 60 + tVopIncr * 60 / m_volmd.iClockRate); //in terms of 60Hz clock ticks

		VOPpredType vopPredType = (VOPpredType) m_pbitstrmIn -> getBits (NUMBITS_VP_PRED_TYPE);
		assert(m_vopmd.vopPredType == vopPredType);

		if (m_volmd.fAUsage != RECTANGLE) {
			m_volmd.bNoCrChange = m_pbitstrmIn -> getBits (1);	//VOP_CR_Change_Disable
	        if (m_volmd.bShapeOnly==FALSE && m_vopmd.vopPredType != IVOP)
			{	
				m_vopmd.bShapeCodingType = m_pbitstrmIn -> getBits (1);
        	}
		}

		if(m_volmd.bShapeOnly==FALSE) {
			Int	iIntraDcSwitchThr = m_pbitstrmIn->getBits (NUMBITS_VP_INTRA_DC_SWITCH_THR);
			assert(m_vopmd.iIntraDcSwitchThr == iIntraDcSwitchThr);
			if (m_vopmd.vopPredType == PVOP) {
				UInt uiFCode = m_pbitstrmIn -> getBits (NUMBITS_VOP_FCODE);
				assert(uiFCode == m_vopmd.mvInfoForward.uiFCode);
			}
			else if (m_vopmd.vopPredType == BVOP) {
				UInt uiForwardFCode = m_pbitstrmIn -> getBits (NUMBITS_VOP_FCODE);
				UInt uiBackwardFCode = m_pbitstrmIn -> getBits (NUMBITS_VOP_FCODE);
				assert(uiForwardFCode == m_vopmd.mvInfoForward.uiFCode);
				assert(uiBackwardFCode == m_vopmd.mvInfoBackward.uiFCode);
			}
		}
	}
	return	TRUE;
}
#endif

Int	CVideoObjectDecoder::checkMotionMarker()
{
	return (m_pbitstrmIn -> peekBits (NUMBITS_DP_MOTION_MARKER) == MOTION_MARKER);
}
Int	CVideoObjectDecoder::checkDCMarker()
{
	return (m_pbitstrmIn -> peekBits (NUMBITS_DP_DC_MARKER) == DC_MARKER);
}

Void CVideoObjectDecoder::decodeIVOP_DataPartitioning ()	
{
//	assert (m_volmd.nBits==8);
	//in case the IVOP is used as an ref for direct mode
// bug fix by toshiba 98-9-24 start
//	memset (m_rgmv, 0, m_iNumMB * 5 * sizeof (CMotionVector));
	memset (m_rgmv, 0, m_iNumMB * PVOP_MV_PER_REF_PER_MB * sizeof (CMotionVector));
// bug fix by toshiba 98-9-24 end

	Int iMBX=0, iMBY=0;
	CMBMode* pmbmd = m_rgmbmd;
	pmbmd->m_stepSize = m_vopmd.intStepI;
	PixelC* ppxlcRefY = (PixelC*) m_pvopcRefQ1->pixelsY () + m_iStartInRefToCurrRctY;
	PixelC* ppxlcRefU = (PixelC*) m_pvopcRefQ1->pixelsU () + m_iStartInRefToCurrRctUV;
	PixelC* ppxlcRefV = (PixelC*) m_pvopcRefQ1->pixelsV () + m_iStartInRefToCurrRctUV;

	Int iCurrentQP  = m_vopmd.intStepI;		
	Int	iVideoPacketNumber = 0;
	m_iVPMBnum = 0;

	m_piMCBPC = new Int[m_iNumMBX*m_iNumMBY];
	Int*	piMCBPC = m_piMCBPC;
	m_piIntraDC = new Int[m_iNumMBX*m_iNumMBY*V_BLOCK];
	Int*	piIntraDC = m_piIntraDC;

	Int	i;
	Int	mbn = 0, mbnFirst = 0;
	Bool bRestartDelayedQP = TRUE;

	do{
		if( checkResyncMarker() ){
			decodeVideoPacketHeader(iCurrentQP);
			iVideoPacketNumber++;
			bRestartDelayedQP = TRUE;
		}

		CMBMode* pmbmdFirst = pmbmd;
		Int* piMCBPCFirst = piMCBPC;
		Int* piIntraDCFirst = piIntraDC;
		mbnFirst = mbn;
		do{
			pmbmd->m_iVideoPacketNumber = iVideoPacketNumber;

			*piMCBPC = m_pentrdecSet->m_pentrdecMCBPCintra->decodeSymbol ();
			assert (*piMCBPC >= 0 && *piMCBPC <= 7);			
			pmbmd->m_dctMd = INTRA;
			if (*piMCBPC > 3)
				pmbmd->m_dctMd = INTRAQ;
			decodeMBTextureDCOfIVOP_DataPartitioning (pmbmd, iCurrentQP, piIntraDC, bRestartDelayedQP);
			pmbmd++;
			mbn++;
			piMCBPC++;
			piIntraDC+=V_BLOCK;
		} while( !checkDCMarker() );
		m_pbitstrmIn -> getBits (NUMBITS_DP_DC_MARKER);

		pmbmd = pmbmdFirst;
		piMCBPC = piMCBPCFirst;
		for(i=mbnFirst;i<mbn;i++) {
			decodeMBTextureHeadOfIVOP_DataPartitioning (pmbmd, piMCBPC);
			pmbmd++;
			piMCBPC++;
		}

		pmbmd = pmbmdFirst;
		piIntraDC = piIntraDCFirst;
		ppxlcRefY = (PixelC*) m_pvopcRefQ1->pixelsY () + m_iStartInRefToCurrRctY + (mbnFirst/m_iNumMBX)*m_iFrameWidthYxMBSize;
		ppxlcRefU = (PixelC*) m_pvopcRefQ1->pixelsU () + m_iStartInRefToCurrRctUV + (mbnFirst/m_iNumMBX)*m_iFrameWidthUVxBlkSize;
		ppxlcRefV = (PixelC*) m_pvopcRefQ1->pixelsV () + m_iStartInRefToCurrRctUV + (mbnFirst/m_iNumMBX)*m_iFrameWidthUVxBlkSize;
		PixelC* ppxlcRefMBY = ppxlcRefY + (mbnFirst%m_iNumMBX)*MB_SIZE;
		PixelC* ppxlcRefMBU = ppxlcRefU + (mbnFirst%m_iNumMBX)*BLOCK_SIZE;
		PixelC* ppxlcRefMBV = ppxlcRefV + (mbnFirst%m_iNumMBX)*BLOCK_SIZE;
		for(i=mbnFirst;i<mbn;i++) {
			iMBX = i % m_iNumMBX;
			iMBY = i / m_iNumMBX;
			if(iMBX == 0 ) {
				ppxlcRefMBY = ppxlcRefY;
				ppxlcRefMBU = ppxlcRefU;
				ppxlcRefMBV = ppxlcRefV;
			}

			decodeTextureIntraMB_DataPartitioning (pmbmd, iMBX, iMBY, ppxlcRefMBY, ppxlcRefMBU, ppxlcRefMBV, piIntraDC);

			pmbmd++;
			piIntraDC += V_BLOCK;
			ppxlcRefMBY += MB_SIZE;
			ppxlcRefMBU += BLOCK_SIZE;
			ppxlcRefMBV += BLOCK_SIZE;
			if(iMBX == m_iNumMBX - 1) {
				MacroBlockMemory** ppmbmTemp = m_rgpmbmAbove;
				m_rgpmbmAbove = m_rgpmbmCurr;
				m_rgpmbmCurr  = ppmbmTemp;
				ppxlcRefY += m_iFrameWidthYxMBSize;
				ppxlcRefU += m_iFrameWidthUVxBlkSize;
				ppxlcRefV += m_iFrameWidthUVxBlkSize;
			}
		}
	} while( checkResyncMarker() );
	delete m_piMCBPC;
}

Void CVideoObjectDecoder::decodeMBTextureDCOfIVOP_DataPartitioning (CMBMode* pmbmd, Int& iCurrentQP,
																	Int* piIntraDC, Bool &bUseNewQPForVlcThr)
{
	Int iBlk = 0;
	pmbmd->m_stepSize = iCurrentQP;
	pmbmd->m_stepSizeDelayed = iCurrentQP;
	if (pmbmd->m_dctMd == INTRAQ)	{
		Int iDQUANT = m_pbitstrmIn->getBits (2);
		switch (iDQUANT) {
		case 0:
			pmbmd->m_intStepDelta = -1;
			break;
		case 1:
			pmbmd->m_intStepDelta = -2;
			break;
		case 2:
			pmbmd->m_intStepDelta = 1;
			break;
		case 3:
			pmbmd->m_intStepDelta = 2;
			break;
		default:
			assert (FALSE);
		}
		pmbmd->m_stepSize += pmbmd->m_intStepDelta;
		if(bUseNewQPForVlcThr)
			pmbmd->m_stepSizeDelayed = pmbmd->m_stepSize;
		Int iQuantMax = (1<<m_volmd.uiQuantPrecision) - 1;
		checkrange (pmbmd->m_stepSize, 1, iQuantMax);
		iCurrentQP = pmbmd->m_stepSize;
	}

	assert (pmbmd != NULL);
	if (pmbmd -> m_rgTranspStatus [0] == ALL) 
		return;

	bUseNewQPForVlcThr = FALSE;

	assert (pmbmd->m_dctMd == INTRA || pmbmd->m_dctMd == INTRAQ);
	Int iQP = pmbmd->m_stepSize;
	for (Int i = 0; i <= 31; i++)	{
		if (iQP <= 4)	{
			m_rgiDcScalerY [i] = 8;
			m_rgiDcScalerC [i] = 8;
		}
		else if (iQP >= 5 && iQP <= 8)	{
			m_rgiDcScalerY [i] = 2 * iQP;
			m_rgiDcScalerC [i] = (iQP + 13) / 2;
		}
		else if (iQP >= 9 && iQP <= 24)	{
			m_rgiDcScalerY [i] = iQP + 8;
			m_rgiDcScalerC [i] = (iQP + 13) / 2;
		}
		else	{
			m_rgiDcScalerY [i] = 2 * iQP - 16;
			m_rgiDcScalerC [i] = iQP - 6;
		}
	}

	assert (iQP > 0);
	assert (pmbmd -> m_stepSizeDelayed > 0);
	if (pmbmd -> m_stepSizeDelayed >= grgiDCSwitchingThreshold [m_vopmd.iIntraDcSwitchThr])
		pmbmd->m_bCodeDcAsAc = TRUE;
	else
		pmbmd->m_bCodeDcAsAc = FALSE;

	for (iBlk = Y_BLOCK1; iBlk < U_BLOCK; iBlk++) {
		if (pmbmd->m_rgTranspStatus [iBlk] != ALL)
			decodeIntraBlockTexture_DataPartitioning (iBlk, pmbmd, piIntraDC);	
	}
	for (iBlk = U_BLOCK; iBlk <= V_BLOCK; iBlk++) {
		decodeIntraBlockTexture_DataPartitioning (iBlk, pmbmd, piIntraDC);	
	}
}

Void CVideoObjectDecoder::decodeMBTextureHeadOfIVOP_DataPartitioning (CMBMode* pmbmd, Int* piMCBPC)
{
	assert (pmbmd->m_rgTranspStatus [0] != ALL);
	Int iBlk = 0, cNonTrnspBlk = 0;
	for (iBlk = (Int) Y_BLOCK1; iBlk <= (Int) Y_BLOCK4; iBlk++) {
		if (pmbmd->m_rgTranspStatus [iBlk] != ALL)	
			cNonTrnspBlk++;
	}
	Int iCBPC = 0;
	Int iCBPY = 0;
// bug fix by toshiba 98-9-24 start
	pmbmd->m_dctMd = INTRA;
	pmbmd->m_bSkip = FALSE; //reset for direct mode 
	if (*piMCBPC > 3)
		pmbmd->m_dctMd = INTRAQ;
// bug fix by toshiba 98-9-24 end
	iCBPC = *piMCBPC % 4;
	pmbmd->m_bACPrediction = m_pbitstrmIn->getBits (1);
	switch (cNonTrnspBlk) {
	case 1:
		iCBPY = m_pentrdecSet->m_pentrdecCBPY1->decodeSymbol ();
		break;
	case 2:
		iCBPY = m_pentrdecSet->m_pentrdecCBPY2->decodeSymbol ();
		break;
	case 3:
		iCBPY = m_pentrdecSet->m_pentrdecCBPY3->decodeSymbol ();
		break;
	case 4:
		iCBPY = m_pentrdecSet->m_pentrdecCBPY->decodeSymbol ();
		break;
	default:
		assert (FALSE);
	}
	setCBPYandC (pmbmd, iCBPC, iCBPY, cNonTrnspBlk);
}

Void CVideoObjectDecoder::decodeTextureIntraMB_DataPartitioning (
	CMBMode* pmbmd, CoordI iMBX, CoordI iMBY, 
	PixelC* ppxlcCurrFrmQY, PixelC* ppxlcCurrFrmQU, PixelC* ppxlcCurrFrmQV, Int* piIntraDC)
{
	assert (pmbmd != NULL);
	if (pmbmd -> m_rgTranspStatus [0] == ALL) 
		return;

	assert (pmbmd->m_dctMd == INTRA || pmbmd->m_dctMd == INTRAQ);
	Int iQP = pmbmd->m_stepSize;
	for (Int i = 0; i <= 31; i++)	{
		if (iQP <= 4)	{
			m_rgiDcScalerY [i] = 8;
			m_rgiDcScalerC [i] = 8;
		}
		else if (iQP >= 5 && iQP <= 8)	{
			m_rgiDcScalerY [i] = 2 * iQP;
			m_rgiDcScalerC [i] = (iQP + 13) / 2;
		}
		else if (iQP >= 9 && iQP <= 24)	{
			m_rgiDcScalerY [i] = iQP + 8;
			m_rgiDcScalerC [i] = (iQP + 13) / 2;
		}
		else	{
			m_rgiDcScalerY [i] = 2 * iQP - 16;
			m_rgiDcScalerC [i] = iQP - 6;
		}
	}

	assert (iQP > 0);
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
	if (iMBTop >= 0)	{
		if(pmbmd->m_iVideoPacketNumber==(pmbmd - m_iNumMBX)->m_iVideoPacketNumber)	{
			pmbmTop  = m_rgpmbmAbove [iMBX];
			pmbmdTop = pmbmd - m_iNumMBX;
		}
	}

	if (iMBX > 0)	{
		if(pmbmd->m_iVideoPacketNumber==(pmbmd - 1)->m_iVideoPacketNumber)	{
			pmbmLeft  = m_rgpmbmCurr [iMBX - 1];
			pmbmdLeft = pmbmd -  1;
		}
	}

	if (iMBTop >= 0 && iMBX > 0)	{
		if(pmbmd->m_iVideoPacketNumber==(pmbmd - m_iNumMBX - 1)->m_iVideoPacketNumber)	{
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
			iDcScaler = m_rgiDcScalerY [iQP];
		}
		else {
			iWidthDst = m_iFrameWidthUV;
			rgchBlkDst = (iBlk == U_BLOCK) ? ppxlcCurrFrmQU: ppxlcCurrFrmQV;
			iDcScaler = m_rgiDcScalerC [iQP];
		}
		rgiCoefQ = m_rgpiCoefQ [iBlk - 1];
		const BlockMemory blkmPred = NULL;
		Int iQpPred = iQP; //default to current if no pred (use 128 case)

		decideIntraPred (blkmPred, 
						 pmbmd,
						 iQpPred,
						 (BlockNum) iBlk,
						 pmbmLeft, 
  						 pmbmTop, 
						 pmbmLeftTop,
						 m_rgpmbmCurr[iMBX],
						 pmbmdLeft,
						 pmbmdTop,
						 pmbmdLeftTop);
		decodeIntraBlockTextureTcoef_DataPartitioning (rgchBlkDst,
								 iWidthDst,
								 iQP, 
								 iDcScaler,
								 iBlk,	
								 *(m_rgpmbmCurr + iMBX),
								 pmbmd,
 								 blkmPred, //for intra-pred
								 iQpPred,
								 piIntraDC);
	}
}

Void CVideoObjectDecoder::decodeIntraBlockTexture_DataPartitioning (Int iBlk, CMBMode* pmbmd, Int* piIntraDC)
{
	if (!pmbmd->m_bCodeDcAsAc)
		piIntraDC[iBlk - 1] = decodeIntraDCmpeg (iBlk <= Y_BLOCK4 || iBlk >=A_BLOCK1);
}

Void	CVideoObjectDecoder::decodeIntraBlockTextureTcoef_DataPartitioning (PixelC* rgpxlcBlkDst,
								 Int iWidthDst,
								 Int iQP,
								 Int iDcScaler,
								 Int iBlk,
								 MacroBlockMemory* pmbmCurr,
								 CMBMode* pmbmd,
 								 const BlockMemory blkmPred,
								 Int iQpPred,
								 Int* piIntraDC)

{
	Int	iCoefStart = 0;
	if (!pmbmd->m_bCodeDcAsAc) {
		iCoefStart++;
	}

	Int* rgiCoefQ = m_rgpiCoefQ [iBlk - 1];
	rgiCoefQ[0] = piIntraDC [iBlk - 1];
	if (pmbmd->getCodedBlockPattern ((BlockNum) iBlk))	{
		Int* rgiZigzag = grgiStandardZigzag;
		if (pmbmd->m_bACPrediction)	
			rgiZigzag = (pmbmd->m_preddir [iBlk - 1] == HORIZONTAL) ? grgiVerticalZigzag : grgiHorizontalZigzag;
 		if(m_volmd.bReversibleVlc)
 			decodeIntraRVLCTCOEF (rgiCoefQ, iCoefStart, rgiZigzag);
 		else
			decodeIntraTCOEF (rgiCoefQ, iCoefStart, rgiZigzag);
	}
	else
		memset (rgiCoefQ + iCoefStart, 0, sizeof (Int) * (BLOCK_SQUARE_SIZE - iCoefStart));
	inverseDCACPred (pmbmd, iBlk - 1, rgiCoefQ, iQP, iDcScaler, blkmPred, iQpPred);
	inverseQuantizeIntraDc (rgiCoefQ, iDcScaler);
	if (m_volmd.fQuantizer == Q_H263)
		inverseQuantizeDCTcoefH263 (rgiCoefQ, 1, iQP);
	else
		inverseQuantizeIntraDCTcoefMPEG (rgiCoefQ, 1, iQP, iBlk>=A_BLOCK1);

	Int i, j;							//save coefQ (ac) for intra pred
	pmbmCurr->rgblkm [iBlk - 1] [0] = m_rgiDCTcoef [0];	//save recon value of DC for intra pred								//save Qcoef in memory
	for (i = 1, j = 8; i < BLOCK_SIZE; i++, j += BLOCK_SIZE)	{
		pmbmCurr->rgblkm [iBlk - 1] [i] = rgiCoefQ [i];
		pmbmCurr->rgblkm [iBlk - 1] [i + BLOCK_SIZE - 1] = rgiCoefQ [j];
	}


	m_pidct->apply (m_rgiDCTcoef, BLOCK_SIZE, rgpxlcBlkDst, iWidthDst);
}

Void CVideoObjectDecoder::decodePVOP_DataPartitioning ()
{
//	assert (m_volmd.nBits==8);
	Int iMBX, iMBY;
	//CoordI y = 0; 
	CMBMode* pmbmd = m_rgmbmd;
	CMotionVector* pmv = m_rgmv;
	PixelC* ppxlcCurrQY = (PixelC*) m_pvopcRefQ1->pixelsY () + m_iStartInRefToCurrRctY;
	PixelC* ppxlcCurrQU = (PixelC*) m_pvopcRefQ1->pixelsU () + m_iStartInRefToCurrRctUV;
	PixelC* ppxlcCurrQV = (PixelC*) m_pvopcRefQ1->pixelsV () + m_iStartInRefToCurrRctUV;

	Int iCurrentQP  = m_vopmd.intStep;		
	Int	iVideoPacketNumber = 0;
	m_iVPMBnum = 0;
	Bool bLeftBndry;
	Bool bRightBndry;
	Bool bTopBndry;

	m_piMCBPC = new Int[m_iNumMBX*m_iNumMBY];
	Int*	piMCBPC = m_piMCBPC;
	m_piIntraDC = new Int[m_iNumMBX*m_iNumMBY*V_BLOCK];
	Int*	piIntraDC = m_piIntraDC;
	//	End Toshiba

	Int	i;
	Int	mbn = 0, mbnFirst = 0;

	CoordI x = 0;
	CoordI y = 0;
	PixelC* ppxlcCurrQMBY = NULL;
	PixelC* ppxlcCurrQMBU = NULL;
	PixelC* ppxlcCurrQMBV = NULL;

	Bool bMBBackup = FALSE;
	CMBMode* pmbmdBackup = NULL;
	Int iMBXBackup = 0, iMBYBackup = 0;
	CMotionVector* pmvBackup = 0;
	PixelC* ppxlcCurrQMBYBackup = NULL;
	PixelC* ppxlcCurrQMBUBackup = NULL;
	PixelC* ppxlcCurrQMBVBackup = NULL;

	Bool bRestartDelayedQP = TRUE;

	do{
		CMBMode* pmbmdFirst = pmbmd;
		CMotionVector* pmvFirst = pmv;
		Int* piMCBPCFirst = piMCBPC;
		Int* piIntraDCFirst = piIntraDC;
		mbnFirst = mbn;

		if( checkResyncMarker() ){
			decodeVideoPacketHeader(iCurrentQP);
			iVideoPacketNumber++;
			bRestartDelayedQP = TRUE;
		}

		do{
			pmbmd->m_iVideoPacketNumber = iVideoPacketNumber;
			iMBX = mbn % m_iNumMBX;
			iMBY = mbn / m_iNumMBX;

			pmbmd->m_bSkip = m_pbitstrmIn->getBits (1);
			if (!pmbmd->m_bSkip) {
				*piMCBPC = m_pentrdecSet->m_pentrdecMCBPCinter->decodeSymbol ();
				assert (*piMCBPC >= 0 && *piMCBPC <= 20);			
				Int iMBtype = *piMCBPC / 4;					//per H.263's MBtype
				switch (iMBtype) {			
				case 0:
					pmbmd->m_dctMd = INTER;
					pmbmd -> m_bhas4MVForward = FALSE;
					break;
				case 1:
					pmbmd->m_dctMd = INTERQ;
					pmbmd -> m_bhas4MVForward = FALSE;
					break;
				case 2:
					pmbmd -> m_dctMd = INTER;
					pmbmd -> m_bhas4MVForward = TRUE;
					break;
				case 3:
					pmbmd->m_dctMd = INTRA;
					break;
				case 4:
					pmbmd->m_dctMd = INTRAQ;
					break;
				default:
					assert (FALSE);
				}
			} else	{									//skipped
				pmbmd->m_dctMd = INTER;
				pmbmd -> m_bhas4MVForward = FALSE;
			}
				if(iMBX == 0) {
					bLeftBndry = TRUE;
				} else {
					bLeftBndry = !((pmbmd - 1) -> m_iVideoPacketNumber == pmbmd -> m_iVideoPacketNumber);
				}
				if(iMBY == 0) {
					bTopBndry = TRUE;
				} else {
					bTopBndry = !((pmbmd - m_iNumMBX) -> m_iVideoPacketNumber == pmbmd -> m_iVideoPacketNumber);
				}
				if((iMBX == m_iNumMBX - 1) || (iMBY == 0)) {
					bRightBndry = TRUE;
				} else {
					bRightBndry = !((pmbmd - m_iNumMBX + 1) -> m_iVideoPacketNumber == pmbmd -> m_iVideoPacketNumber);
				}
			decodeMV (pmbmd, pmv, bLeftBndry, bRightBndry, bTopBndry, FALSE, iMBX, iMBY);

			if(bMBBackup){
				if (pmbmdBackup->m_dctMd == INTER || pmbmdBackup->m_dctMd == INTERQ) {
					motionCompMB (
						m_ppxlcPredMBY, m_pvopcRefQ0->pixelsY (),
						pmvBackup, pmbmdBackup, 
						iMBXBackup, iMBYBackup, 
						x, y,
						pmbmdBackup->m_bSkip, FALSE,
						&m_rctRefVOPY0
					);
					if (!pmbmdBackup->m_bSkip) {
						CoordI iXRefUV, iYRefUV, iXRefUV1, iYRefUV1;
						mvLookupUV (pmbmdBackup, pmvBackup, iXRefUV, iYRefUV, iXRefUV1, iYRefUV1);
						motionCompUV (m_ppxlcPredMBU, m_ppxlcPredMBV, m_pvopcRefQ0, x, y, iXRefUV, iYRefUV, m_vopmd.iRoundingControl, &m_rctRefVOPY0);
						addErrorAndPredToCurrQ (ppxlcCurrQMBYBackup, ppxlcCurrQMBUBackup, ppxlcCurrQMBVBackup);
					}
					else {
						if (m_volmd.bAdvPredDisable)
	
							copyFromRefToCurrQ (m_pvopcRefQ0, x, y, ppxlcCurrQMBYBackup, ppxlcCurrQMBUBackup, ppxlcCurrQMBVBackup, NULL);
						else
							copyFromPredForYAndRefForCToCurrQ (x, y, ppxlcCurrQMBYBackup, ppxlcCurrQMBUBackup, ppxlcCurrQMBVBackup, NULL);
					}
				}
				bMBBackup = FALSE;
			}

			pmbmd++;
			pmv += PVOP_MV_PER_REF_PER_MB;
			mbn++;
			piMCBPC++;
			assert(mbn<=(m_iNumMBX*m_iNumMBY));
		} while( !checkMotionMarker() );
		m_pbitstrmIn -> getBits (NUMBITS_DP_MOTION_MARKER);

		pmbmd = pmbmdFirst;
		piMCBPC = piMCBPCFirst;
		piIntraDC = piIntraDCFirst;
		for(i=mbnFirst;i<mbn;i++) {
			decodeMBTextureHeadOfPVOP_DataPartitioning (pmbmd, iCurrentQP, piMCBPC, piIntraDC, bRestartDelayedQP);
			pmbmd++;
			piMCBPC++;
			piIntraDC += V_BLOCK;
		}

		pmbmd = pmbmdFirst;
		pmv = pmvFirst;
		piIntraDC = piIntraDCFirst;

		for(i=mbnFirst;i<mbn;i++) {
			iMBX = i % m_iNumMBX;
			iMBY = i / m_iNumMBX;
			if(iMBX == 0 ) {
				ppxlcCurrQMBY = ppxlcCurrQY;
				ppxlcCurrQMBU = ppxlcCurrQU;
				ppxlcCurrQMBV = ppxlcCurrQV;
				x = 0;
				if(iMBY != 0)	y += MB_SIZE;
			} else {
				x += MB_SIZE;
			}

			if (pmbmd->m_dctMd == INTRA || pmbmd->m_dctMd == INTRAQ)
				decodeTextureIntraMB_DataPartitioning (pmbmd, iMBX, iMBY, ppxlcCurrQMBY, ppxlcCurrQMBU, ppxlcCurrQMBV, piIntraDC);
			else {
				if (!pmbmd->m_bSkip)
					decodeTextureInterMB (pmbmd);
			}

			if(i==mbn-1){
				bMBBackup = TRUE;
				pmbmdBackup = pmbmd;
				pmvBackup = pmv;
				iMBXBackup = iMBX;
				iMBYBackup = iMBY;
				ppxlcCurrQMBYBackup = ppxlcCurrQMBY;
				ppxlcCurrQMBUBackup = ppxlcCurrQMBU;
				ppxlcCurrQMBVBackup = ppxlcCurrQMBV;
			}

			if (pmbmd->m_dctMd == INTER || pmbmd->m_dctMd == INTERQ) {
				motionCompMB (
					m_ppxlcPredMBY, m_pvopcRefQ0->pixelsY (),
					pmv, pmbmd, 
					iMBX, iMBY, 
					x, y,
					pmbmd->m_bSkip, FALSE,
					&m_rctRefVOPY0
				);
				if (!pmbmd->m_bSkip) {
					CoordI iXRefUV, iYRefUV, iXRefUV1, iYRefUV1;
					mvLookupUV (pmbmd, pmv, iXRefUV, iYRefUV, iXRefUV1, iYRefUV1);
					motionCompUV (m_ppxlcPredMBU, m_ppxlcPredMBV, m_pvopcRefQ0, x, y, iXRefUV, iYRefUV, m_vopmd.iRoundingControl, &m_rctRefVOPY0);
					addErrorAndPredToCurrQ (ppxlcCurrQMBY, ppxlcCurrQMBU, ppxlcCurrQMBV);
				}
				else {
					if (m_volmd.bAdvPredDisable)

						copyFromRefToCurrQ (m_pvopcRefQ0, x, y, ppxlcCurrQMBY, ppxlcCurrQMBU, ppxlcCurrQMBV, NULL);
					else
						copyFromPredForYAndRefForCToCurrQ (x, y, ppxlcCurrQMBY, ppxlcCurrQMBU, ppxlcCurrQMBV, NULL);
				}
			}

			pmbmd++;
			pmv += PVOP_MV_PER_REF_PER_MB;
			piIntraDC += V_BLOCK;
			ppxlcCurrQMBY += MB_SIZE;
			ppxlcCurrQMBU += BLOCK_SIZE;
			ppxlcCurrQMBV += BLOCK_SIZE;

			if(iMBX == m_iNumMBX - 1) {
				MacroBlockMemory** ppmbmTemp = m_rgpmbmAbove;
				m_rgpmbmAbove = m_rgpmbmCurr;
				m_rgpmbmCurr  = ppmbmTemp;
				ppxlcCurrQY += m_iFrameWidthYxMBSize;
				ppxlcCurrQU += m_iFrameWidthUVxBlkSize;
				ppxlcCurrQV += m_iFrameWidthUVxBlkSize;
			}
		}
	} while( checkResyncMarker() );
	delete m_piIntraDC;
	delete m_piMCBPC;
}

Void CVideoObjectDecoder::decodeMBTextureHeadOfPVOP_DataPartitioning (CMBMode* pmbmd, Int& iCurrentQP, Int* piMCBPC,
																	  Int* piIntraDC, Bool &bUseNewQPForVlcThr)
{
	assert (pmbmd->m_rgTranspStatus [0] != ALL);
	Int iBlk = 0, cNonTrnspBlk = 0;
	for (iBlk = (Int) Y_BLOCK1; iBlk <= (Int) Y_BLOCK4; iBlk++) {
		if (pmbmd->m_rgTranspStatus [iBlk] != ALL)	
			cNonTrnspBlk++;
	}
	Int iCBPC = 0;
	Int iCBPY = 0;
	if (!pmbmd->m_bSkip) {
	  //Int iMBtype = *piMCBPC / 4;					//per H.263's MBtype
		iCBPC = *piMCBPC % 4;
		if (pmbmd->m_dctMd == INTRA || pmbmd->m_dctMd == INTRAQ)	{
			pmbmd->m_bACPrediction = m_pbitstrmIn->getBits (1);
			switch (cNonTrnspBlk) {
			case 1:
				iCBPY = m_pentrdecSet->m_pentrdecCBPY1->decodeSymbol ();
				break;
			case 2:
				iCBPY = m_pentrdecSet->m_pentrdecCBPY2->decodeSymbol ();
				break;
			case 3:
				iCBPY = m_pentrdecSet->m_pentrdecCBPY3->decodeSymbol ();
				break;
			case 4:
				iCBPY = m_pentrdecSet->m_pentrdecCBPY->decodeSymbol ();
				break;
			default:
				assert (FALSE);
			}
		}
		else {
			switch (cNonTrnspBlk) {
			case 1:
				iCBPY = 1 - m_pentrdecSet->m_pentrdecCBPY1->decodeSymbol ();
				break;
			case 2:
				iCBPY = 3 - m_pentrdecSet->m_pentrdecCBPY2->decodeSymbol ();
				break;
			case 3:
				iCBPY = 7 - m_pentrdecSet->m_pentrdecCBPY3->decodeSymbol ();
				break;
			case 4:
				iCBPY = 15 - m_pentrdecSet->m_pentrdecCBPY->decodeSymbol ();
				break;
			default:
				assert (FALSE);
			}
		}

		assert (iCBPY >= 0 && iCBPY <= 15);			
	}
	else	{									//skipped
		pmbmd->m_dctMd = INTER;
		pmbmd -> m_bhas4MVForward = FALSE;
	}
	setCBPYandC (pmbmd, iCBPC, iCBPY, cNonTrnspBlk);
	pmbmd->m_stepSize = iCurrentQP;
	pmbmd->m_stepSizeDelayed = iCurrentQP;
	if (pmbmd->m_dctMd == INTERQ || pmbmd->m_dctMd == INTRAQ) {
		assert (!pmbmd->m_bSkip);
		Int iDQUANT = m_pbitstrmIn->getBits (2);
		switch (iDQUANT) {
		case 0:
			pmbmd->m_intStepDelta = -1;
			break;
		case 1:
			pmbmd->m_intStepDelta = -2;
			break;
		case 2:
			pmbmd->m_intStepDelta = 1;
			break;
		case 3:
			pmbmd->m_intStepDelta = 2;
			break;
		default:
			assert (FALSE);
		}
		pmbmd->m_stepSize += pmbmd->m_intStepDelta;
		if(bUseNewQPForVlcThr)
			pmbmd->m_stepSizeDelayed = pmbmd->m_stepSize;
		Int iQuantMax = (1<<m_volmd.uiQuantPrecision) - 1;
		if (pmbmd->m_stepSize < 1)
			pmbmd->m_stepSize = 1;
		else if (pmbmd->m_stepSize > iQuantMax)
			pmbmd->m_stepSize = iQuantMax;
		iCurrentQP = pmbmd->m_stepSize;
	}

	if (!pmbmd->m_bSkip)
		bUseNewQPForVlcThr = FALSE;

	if(pmbmd->m_dctMd == INTRA || pmbmd->m_dctMd == INTRAQ)	{
		Int iQP = pmbmd->m_stepSize;
		for (Int i = 0; i <= 31; i++)	{
			if (iQP <= 4)	{
				m_rgiDcScalerY [i] = 8;
				m_rgiDcScalerC [i] = 8;
			}
			else if (iQP >= 5 && iQP <= 8)	{
				m_rgiDcScalerY [i] = 2 * iQP;
				m_rgiDcScalerC [i] = (iQP + 13) / 2;
			}
			else if (iQP >= 9 && iQP <= 24)	{
				m_rgiDcScalerY [i] = iQP + 8;
				m_rgiDcScalerC [i] = (iQP + 13) / 2;
			}
			else	{
				m_rgiDcScalerY [i] = 2 * iQP - 16;
				m_rgiDcScalerC [i] = iQP - 6;
			}
		}

		assert (iQP > 0);
		assert (pmbmd -> m_stepSizeDelayed > 0);
		if (pmbmd -> m_stepSizeDelayed >= grgiDCSwitchingThreshold [m_vopmd.iIntraDcSwitchThr])
			pmbmd->m_bCodeDcAsAc = TRUE;
		else
			pmbmd->m_bCodeDcAsAc = FALSE;

		for (iBlk = Y_BLOCK1; iBlk < U_BLOCK; iBlk++) {
			if (pmbmd->m_rgTranspStatus [iBlk] != ALL)
				decodeIntraBlockTexture_DataPartitioning (iBlk, pmbmd, piIntraDC);	
		}
		for (iBlk = U_BLOCK; iBlk <= V_BLOCK; iBlk++) {
			decodeIntraBlockTexture_DataPartitioning (iBlk, pmbmd, piIntraDC);	
		}
	}
}

Void CVideoObjectDecoder::decodeIVOP_WithShape_DataPartitioning ()	
{
	//assert (m_volmd.nBits==8);
	assert (m_volmd.fAUsage!=EIGHT_BIT);
	//in case the IVOP is used as an ref for direct mode
	memset (m_rgmv, 0, m_iNumMB * PVOP_MV_PER_REF_PER_MB * sizeof (CMotionVector));

	Int iMBX, iMBY;
	CMBMode* pmbmd = m_rgmbmd;

	PixelC* ppxlcRefY  = (PixelC*) m_pvopcRefQ1->pixelsY () + m_iStartInRefToCurrRctY;
	PixelC* ppxlcRefU  = (PixelC*) m_pvopcRefQ1->pixelsU () + m_iStartInRefToCurrRctUV;
	PixelC* ppxlcRefV  = (PixelC*) m_pvopcRefQ1->pixelsV () + m_iStartInRefToCurrRctUV;
	PixelC* ppxlcRefBY = (PixelC*) m_pvopcRefQ1->pixelsBY () + m_iStartInRefToCurrRctY;
	PixelC* ppxlcRefA  = (PixelC*) m_pvopcRefQ1->pixelsA () + m_iStartInRefToCurrRctY;
	PixelC* ppxlcRefBUV = (PixelC*) m_pvopcRefQ1->pixelsBUV () + m_iStartInRefToCurrRctUV;

	Int iCurrentQP  = m_vopmd.intStepI;	
	Int	iVideoPacketNumber = 0;
	m_iVPMBnum = 0;
	m_piMCBPC = new Int[m_iNumMBX*m_iNumMBY];
	Int*	piMCBPC = m_piMCBPC;
	m_piIntraDC = new Int[m_iNumMBX*m_iNumMBY*V_BLOCK];
	Int*	piIntraDC = m_piIntraDC;

	Bool bRestartDelayedQP = TRUE;
	Int	i;
	Int mbn = 0, mbnFirst = 0;

	PixelC* ppxlcRefMBBY = NULL;
	PixelC* ppxlcRefMBBUV;
	PixelC* ppxlcRefMBY = NULL;
	PixelC* ppxlcRefMBU = NULL;
	PixelC* ppxlcRefMBV = NULL;
	PixelC* ppxlcRefMBA = NULL;
	do	{
		if( checkResyncMarker() )	{
			decodeVideoPacketHeader(iCurrentQP);
			iVideoPacketNumber++;
			bRestartDelayedQP = TRUE;
		}

		CMBMode* pmbmdFirst = pmbmd;
		Int* piMCBPCFirst = piMCBPC;
		Int* piIntraDCFirst = piIntraDC;
		mbnFirst = mbn;

		do{
			iMBX = mbn % m_iNumMBX;
			iMBY = mbn / m_iNumMBX;
			if(iMBX == 0 ) {
				ppxlcRefMBBY = ppxlcRefBY;
				ppxlcRefMBBUV = ppxlcRefBUV;
			}

			pmbmd->m_iVideoPacketNumber = iVideoPacketNumber;

			decodeIntraShape (pmbmd, iMBX, iMBY, m_ppxlcCurrMBBY, ppxlcRefMBBY);
			downSampleBY (m_ppxlcCurrMBBY, m_ppxlcCurrMBBUV); // downsample original BY now for LPE padding (using original shape)
			if(m_volmd.bShapeOnly==FALSE)
			{
				pmbmd->m_bPadded=FALSE;
				if (pmbmd->m_rgTranspStatus [0] != ALL) {
					*piMCBPC = m_pentrdecSet->m_pentrdecMCBPCintra->decodeSymbol ();
					assert (*piMCBPC >= 0 && *piMCBPC <= 7);			
					pmbmd->m_dctMd = INTRA;
					if (*piMCBPC > 3)
						pmbmd->m_dctMd = INTRAQ;
					decodeMBTextureDCOfIVOP_DataPartitioning (pmbmd, iCurrentQP,
						piIntraDC, bRestartDelayedQP);
				}
			}
			else	{
				assert(FALSE);
			}
			pmbmd++;
			mbn++;
			piMCBPC++;
			piIntraDC+=V_BLOCK;
			ppxlcRefMBBY += MB_SIZE;
			ppxlcRefMBBUV += BLOCK_SIZE;
			if(iMBX == m_iNumMBX - 1) {
				ppxlcRefBY += m_iFrameWidthYxMBSize;
				ppxlcRefBUV += m_iFrameWidthUVxBlkSize;
			}
		} while( !checkDCMarker() );
		m_pbitstrmIn -> getBits (NUMBITS_DP_DC_MARKER);

		pmbmd = pmbmdFirst;
		piMCBPC = piMCBPCFirst;
		for(i=mbnFirst;i<mbn;i++) {
			if (pmbmd->m_rgTranspStatus [0] != ALL)
				decodeMBTextureHeadOfIVOP_DataPartitioning (pmbmd, piMCBPC);
			pmbmd++;
			piMCBPC++;
		}

		pmbmd = pmbmdFirst;
		piIntraDC = piIntraDCFirst;
		ppxlcRefBY = (PixelC*) m_pvopcRefQ1->pixelsBY () + m_iStartInRefToCurrRctY + (mbnFirst/m_iNumMBX)*m_iFrameWidthYxMBSize;
		ppxlcRefBUV = (PixelC*) m_pvopcRefQ1->pixelsBUV () + m_iStartInRefToCurrRctUV + (mbnFirst/m_iNumMBX)*m_iFrameWidthUVxBlkSize;
		ppxlcRefMBBY = ppxlcRefBY + (mbnFirst%m_iNumMBX)*MB_SIZE;
		ppxlcRefMBBUV = ppxlcRefBUV + (mbnFirst%m_iNumMBX)*BLOCK_SIZE;
		for(i=mbnFirst;i<mbn;i++) {
			pmbmd->m_bPadded = FALSE;
			iMBX = i % m_iNumMBX;
			iMBY = i / m_iNumMBX;
			if(iMBX == 0 ) {
				ppxlcRefMBY = ppxlcRefY;
				ppxlcRefMBU = ppxlcRefU;
				ppxlcRefMBV = ppxlcRefV;
				ppxlcRefMBA  = ppxlcRefA;
				ppxlcRefMBBY = ppxlcRefBY;
				ppxlcRefMBBUV = ppxlcRefBUV;
			}

			copyRefShapeToMb(m_ppxlcCurrMBBY, ppxlcRefMBBY);
			downSampleBY (m_ppxlcCurrMBBY, m_ppxlcCurrMBBUV); // downsample original BY now for LPE padding (using original shape)

			if (pmbmd->m_rgTranspStatus [0] != ALL) {
				decodeTextureIntraMB_DataPartitioning (pmbmd, iMBX, iMBY, ppxlcRefMBY, ppxlcRefMBU, ppxlcRefMBV, piIntraDC);

				// MC padding
				if (pmbmd -> m_rgTranspStatus [0] == PARTIAL)
					mcPadCurrMB (ppxlcRefMBY, ppxlcRefMBU, ppxlcRefMBV, ppxlcRefMBA);
				padNeighborTranspMBs (
					iMBX, iMBY,
					pmbmd,
					ppxlcRefMBY, ppxlcRefMBU, ppxlcRefMBV, ppxlcRefMBA
				);
			}
			else {
				padCurrAndTopTranspMBFromNeighbor (
					iMBX, iMBY,
					pmbmd,
					ppxlcRefMBY, ppxlcRefMBU, ppxlcRefMBV, ppxlcRefMBA
				);
			}

			ppxlcRefMBA += MB_SIZE;
			ppxlcRefMBBY += MB_SIZE;
			ppxlcRefMBBUV += BLOCK_SIZE;
			pmbmd++;
			piIntraDC += V_BLOCK;
			ppxlcRefMBY += MB_SIZE;
			ppxlcRefMBU += BLOCK_SIZE;
			ppxlcRefMBV += BLOCK_SIZE;
			if(iMBX == m_iNumMBX - 1) {
				MacroBlockMemory** ppmbmTemp = m_rgpmbmAbove;
				m_rgpmbmAbove = m_rgpmbmCurr;
				m_rgpmbmCurr  = ppmbmTemp;
				ppxlcRefY += m_iFrameWidthYxMBSize;
				ppxlcRefU += m_iFrameWidthUVxBlkSize;
				ppxlcRefV += m_iFrameWidthUVxBlkSize;
				ppxlcRefA += m_iFrameWidthYxMBSize;
				ppxlcRefBY += m_iFrameWidthYxMBSize;
				ppxlcRefBUV += m_iFrameWidthUVxBlkSize;
			}
		}
	} while( checkResyncMarker() );
	delete m_piMCBPC;
}

Void CVideoObjectDecoder::decodePVOP_WithShape_DataPartitioning ()
{
	//assert (m_volmd.nBits==8);
	assert (m_volmd.fAUsage!=EIGHT_BIT);
	Int iMBX, iMBY;
	CoordI y = m_rctCurrVOPY.top; 
	CoordI x = m_rctCurrVOPY.left;
	CoordI y_s = m_rctCurrVOPY.top; 
	CoordI x_s = m_rctCurrVOPY.left;
	CMBMode* pmbmd = m_rgmbmd;
	CMotionVector* pmv = m_rgmv;
	CMotionVector* pmvBY = m_rgmvBY;
	PixelC* ppxlcCurrQY  = (PixelC*) m_pvopcRefQ1->pixelsY () + m_iStartInRefToCurrRctY;
	PixelC* ppxlcCurrQU  = (PixelC*) m_pvopcRefQ1->pixelsU () + m_iStartInRefToCurrRctUV;
	PixelC* ppxlcCurrQV  = (PixelC*) m_pvopcRefQ1->pixelsV () + m_iStartInRefToCurrRctUV;
	PixelC* ppxlcCurrQBY = (PixelC*) m_pvopcRefQ1->pixelsBY () + m_iStartInRefToCurrRctY;
	PixelC* ppxlcCurrQA  = (PixelC*) m_pvopcRefQ1->pixelsA () + m_iStartInRefToCurrRctY;
	PixelC* ppxlcCurrQBUV = (PixelC*) m_pvopcRefQ1->pixelsBUV () + m_iStartInRefToCurrRctUV;
	
	Int iCurrentQP  = m_vopmd.intStep;	
	Int	iVideoPacketNumber = 0; 			//	added for error resilience mode by Toshiba
	m_iVPMBnum = 0;

	m_piMCBPC = new Int[m_iNumMBX*m_iNumMBY];
	Int*	piMCBPC = m_piMCBPC;
	m_piIntraDC = new Int[m_iNumMBX*m_iNumMBY*V_BLOCK];
	Int*	piIntraDC = m_piIntraDC;

	Int	i;
	Int	mbn = 0, mbnFirst = 0;

	PixelC* ppxlcCurrQMBY = NULL;
	PixelC* ppxlcCurrQMBU = NULL;
	PixelC* ppxlcCurrQMBV = NULL;
	PixelC* ppxlcCurrQMBBY = NULL;
	PixelC* ppxlcCurrQMBBUV;
	PixelC* ppxlcCurrQMBA = NULL;

	Bool bMBBackup = FALSE;
	CMBMode* pmbmdBackup = NULL;
	Int iMBXBackup = 0, iMBYBackup = 0;
	CMotionVector* pmvBackup = NULL;
	PixelC* ppxlcCurrQMBYBackup = NULL;
	PixelC* ppxlcCurrQMBUBackup = NULL;
	PixelC* ppxlcCurrQMBVBackup = NULL;
	PixelC* ppxlcCurrQMBBYBackup = NULL;
	PixelC* ppxlcCurrQMBBUVBackup;
	PixelC* ppxlcCurrQMBABackup = NULL;
	Bool bPaddedLBackup = FALSE;
	Bool bPaddedTBackup = FALSE;

	Bool bRestartDelayedQP = TRUE;

	do{
		CMBMode* pmbmdFirst = pmbmd;
		CMotionVector* pmvFirst = pmv;
		Int* piMCBPCFirst = piMCBPC;
		Int* piIntraDCFirst = piIntraDC;
		mbnFirst = mbn;

		if( checkResyncMarker() ){
			decodeVideoPacketHeader(iCurrentQP);
			iVideoPacketNumber++;
			bRestartDelayedQP = TRUE;
		}

		do{
			pmbmd->m_iVideoPacketNumber = iVideoPacketNumber;
			iMBX = mbn % m_iNumMBX;
			iMBY = mbn / m_iNumMBX;
			if(iMBX == 0 ) {
				ppxlcCurrQMBBY = ppxlcCurrQBY;
				ppxlcCurrQMBBUV = ppxlcCurrQBUV;
				x_s = m_rctCurrVOPY.left;
				if(iMBY != 0)	y_s += MB_SIZE;
			} else {
				x_s += MB_SIZE;
			}

			ShapeMode shpmdColocatedMB;
			if(m_vopmd.bShapeCodingType) {
				shpmdColocatedMB = m_rgmbmdRef [
					MIN (MAX (0, iMBX), m_iNumMBXRef - 1) + 
 					MIN (MAX (0, iMBY), m_iNumMBYRef - 1) * m_iNumMBXRef
				].m_shpmd;
				decodeInterShape (
					m_pvopcRefQ0,
					pmbmd, 
					iMBX, iMBY, 
					x_s, y_s, 
					pmv, pmvBY, 
					m_ppxlcCurrMBBY, ppxlcCurrQMBBY,
					shpmdColocatedMB
				);
			}
			else	{
				decodeIntraShape (pmbmd, iMBX, iMBY, m_ppxlcCurrMBBY,
					ppxlcCurrQMBBY);
			}
			downSampleBY (m_ppxlcCurrMBBY, m_ppxlcCurrMBBUV);

			if (pmbmd->m_rgTranspStatus [0] != ALL && m_volmd.bShapeOnly==FALSE) {
				pmbmd->m_bSkip = m_pbitstrmIn->getBits (1);
				if (!pmbmd->m_bSkip) {
					*piMCBPC = m_pentrdecSet->m_pentrdecMCBPCinter->decodeSymbol ();
					assert (*piMCBPC >= 0 && *piMCBPC <= 20);			
					Int iMBtype = *piMCBPC / 4;					//per H.263's MBtype
					switch (iMBtype) {			
					case 0:
						pmbmd->m_dctMd = INTER;
						pmbmd -> m_bhas4MVForward = FALSE;
						break;
					case 1:
						pmbmd->m_dctMd = INTERQ;
						pmbmd -> m_bhas4MVForward = FALSE;
						break;
					case 2:
						pmbmd -> m_dctMd = INTER;
						pmbmd -> m_bhas4MVForward = TRUE;
						break;
					case 3:
						pmbmd->m_dctMd = INTRA;
						break;
					case 4:
						pmbmd->m_dctMd = INTRAQ;
						break;
					default:
						assert (FALSE);
					}
				} else	{									//skipped
					pmbmd->m_dctMd = INTER;
					pmbmd -> m_bhas4MVForward = FALSE;
				}
				decodeMVWithShape (pmbmd, iMBX, iMBY, pmv);
				if(pmbmd->m_bhas4MVForward)
					padMotionVectors(pmbmd,pmv);
			}

			if(bMBBackup){
				copyRefShapeToMb(m_ppxlcCurrMBBY, ppxlcCurrQMBBYBackup);
				downSampleBY (m_ppxlcCurrMBBY, m_ppxlcCurrMBBUV); // downsample original BY now for LPE padding (using original shape)
				pmbmdBackup->m_bPadded = FALSE;
				if(iMBXBackup > 0) (pmbmdBackup-1)->m_bPadded = bPaddedLBackup;
				if(iMBYBackup > 0) (pmbmdBackup-m_iNumMBX)->m_bPadded = bPaddedTBackup;
				if (pmbmdBackup->m_rgTranspStatus [0] != ALL) {
					if (pmbmdBackup->m_dctMd == INTER || pmbmdBackup->m_dctMd == INTERQ)	{
			 			motionCompMB (
							m_ppxlcPredMBY, m_pvopcRefQ0->pixelsY (),
							pmvBackup, pmbmdBackup, 
							iMBXBackup, iMBYBackup, 
							x, y,
							pmbmdBackup->m_bSkip, FALSE,
							&m_rctRefVOPY0
						);
						if (!pmbmdBackup->m_bSkip) {
							CoordI iXRefUV, iYRefUV;
							mvLookupUVWithShape (pmbmdBackup, pmvBackup, iXRefUV, iYRefUV);
							motionCompUV (m_ppxlcPredMBU, m_ppxlcPredMBV, m_pvopcRefQ0,
								x, y, iXRefUV, iYRefUV, m_vopmd.iRoundingControl,&m_rctRefVOPY0);
							addErrorAndPredToCurrQ (ppxlcCurrQMBYBackup, ppxlcCurrQMBUBackup, ppxlcCurrQMBVBackup);
						}
						else {
							if (m_volmd.bAdvPredDisable)
								copyFromRefToCurrQ (m_pvopcRefQ0, x, y, ppxlcCurrQMBYBackup, ppxlcCurrQMBUBackup, ppxlcCurrQMBVBackup, &m_rctRefVOPY0);
							else
								copyFromPredForYAndRefForCToCurrQ (x, y, ppxlcCurrQMBYBackup, ppxlcCurrQMBUBackup, ppxlcCurrQMBVBackup, &m_rctRefVOPY0);
						}
					}

					if (pmbmdBackup->m_rgTranspStatus [0] == PARTIAL)
						mcPadCurrMB (ppxlcCurrQMBYBackup, ppxlcCurrQMBUBackup, ppxlcCurrQMBVBackup, ppxlcCurrQMBABackup);
					padNeighborTranspMBs (
						iMBXBackup, iMBYBackup,
						pmbmdBackup,
						ppxlcCurrQMBYBackup, ppxlcCurrQMBUBackup, ppxlcCurrQMBVBackup, ppxlcCurrQMBABackup
					);
				}
				else {
					padCurrAndTopTranspMBFromNeighbor (
						iMBXBackup, iMBYBackup,
						pmbmdBackup,
						ppxlcCurrQMBYBackup, ppxlcCurrQMBUBackup, ppxlcCurrQMBVBackup, ppxlcCurrQMBABackup
					);
				}

				bMBBackup = FALSE;
			}

			pmbmd++;
			pmv += PVOP_MV_PER_REF_PER_MB;
			pmvBY++;
			mbn++;
			piMCBPC++;
			ppxlcCurrQMBBY += MB_SIZE;
			ppxlcCurrQMBBUV += BLOCK_SIZE;
			if(iMBX == m_iNumMBX - 1) {
				ppxlcCurrQBY += m_iFrameWidthYxMBSize;
				ppxlcCurrQBUV += m_iFrameWidthUVxBlkSize;
			}
			assert(mbn<=(m_iNumMBX*m_iNumMBY));
		} while( !checkMotionMarker() );
		m_pbitstrmIn -> getBits (NUMBITS_DP_MOTION_MARKER);

		pmbmd = pmbmdFirst;
		piMCBPC = piMCBPCFirst;
		piIntraDC = piIntraDCFirst;
		for(i=mbnFirst;i<mbn;i++) {
			if (pmbmd->m_rgTranspStatus [0] != ALL && m_volmd.bShapeOnly==FALSE) {
				decodeMBTextureHeadOfPVOP_DataPartitioning (pmbmd, iCurrentQP, piMCBPC, piIntraDC, bRestartDelayedQP);
			}
			pmbmd++;
			piMCBPC++;
			piIntraDC += V_BLOCK;
		}

		pmbmd = pmbmdFirst;
		pmv = pmvFirst;
		piIntraDC = piIntraDCFirst;
		ppxlcCurrQBY = (PixelC*) m_pvopcRefQ1->pixelsBY () + m_iStartInRefToCurrRctY + (mbnFirst/m_iNumMBX)*m_iFrameWidthYxMBSize;
		ppxlcCurrQBUV = (PixelC*) m_pvopcRefQ1->pixelsBUV () + m_iStartInRefToCurrRctUV + (mbnFirst/m_iNumMBX)*m_iFrameWidthUVxBlkSize;
		ppxlcCurrQMBBY = ppxlcCurrQBY + (mbnFirst%m_iNumMBX)*MB_SIZE;
		ppxlcCurrQMBBUV = ppxlcCurrQBUV + (mbnFirst%m_iNumMBX)*BLOCK_SIZE;

		for(i=mbnFirst;i<mbn;i++) {
			pmbmd->m_bPadded = FALSE;
			iMBX = i % m_iNumMBX;
			iMBY = i / m_iNumMBX;
			if(iMBX == 0 ) {
				ppxlcCurrQMBY = ppxlcCurrQY;
				ppxlcCurrQMBU = ppxlcCurrQU;
				ppxlcCurrQMBV = ppxlcCurrQV;
				ppxlcCurrQMBBY = ppxlcCurrQBY;
				ppxlcCurrQMBA  = ppxlcCurrQA;
				ppxlcCurrQMBBUV = ppxlcCurrQBUV;
				x = m_rctCurrVOPY.left;
				if(iMBY != 0)	y += MB_SIZE;
			} else {
				x += MB_SIZE;
			}

			copyRefShapeToMb(m_ppxlcCurrMBBY, ppxlcCurrQMBBY);
			downSampleBY (m_ppxlcCurrMBBY, m_ppxlcCurrMBBUV); // downsample original BY now for LPE padding (using original shape)

			if (pmbmd->m_rgTranspStatus [0] != ALL && m_volmd.bShapeOnly==FALSE) {
				if (pmbmd->m_dctMd == INTRA || pmbmd->m_dctMd == INTRAQ)
					decodeTextureIntraMB_DataPartitioning (pmbmd, iMBX, iMBY, ppxlcCurrQMBY, ppxlcCurrQMBU, ppxlcCurrQMBV, piIntraDC);
				else {
					if (!pmbmd->m_bSkip)
						decodeTextureInterMB (pmbmd);
				}
			}

			if(i==mbn-1){
				bMBBackup = TRUE;
				pmbmdBackup = pmbmd;
				pmvBackup = pmv;
				iMBXBackup = iMBX;
				iMBYBackup = iMBY;
				ppxlcCurrQMBYBackup = ppxlcCurrQMBY;
				ppxlcCurrQMBUBackup = ppxlcCurrQMBU;
				ppxlcCurrQMBVBackup = ppxlcCurrQMBV;
				ppxlcCurrQMBBYBackup = ppxlcCurrQMBBY;
				ppxlcCurrQMBBUVBackup = ppxlcCurrQMBBUV;
				ppxlcCurrQMBABackup = ppxlcCurrQMBA;
				bPaddedLBackup = (pmbmdBackup-1)->m_bPadded;
				bPaddedTBackup = (pmbmdBackup-m_iNumMBX)->m_bPadded;
			}

			if (pmbmd->m_rgTranspStatus [0] != ALL) {
				if (pmbmd->m_dctMd == INTER || pmbmd->m_dctMd == INTERQ) {
					//	Addded for data partitioning mode by Toshiba(1997-11-26:DP+RVLC)
		 			motionCompMB (
						m_ppxlcPredMBY, m_pvopcRefQ0->pixelsY (),
						pmv, pmbmd, 
						iMBX, iMBY, 
						x, y,
						pmbmd->m_bSkip, FALSE,
						&m_rctRefVOPY0
					);
					if (!pmbmd->m_bSkip) {
						CoordI iXRefUV, iYRefUV;
						mvLookupUVWithShape (pmbmd, pmv, iXRefUV, iYRefUV);
						motionCompUV (m_ppxlcPredMBU, m_ppxlcPredMBV, m_pvopcRefQ0,
							x, y, iXRefUV, iYRefUV, m_vopmd.iRoundingControl,&m_rctRefVOPY0);
						addErrorAndPredToCurrQ (ppxlcCurrQMBY, ppxlcCurrQMBU, ppxlcCurrQMBV);						
					}
					else {
						if (m_volmd.bAdvPredDisable)
							copyFromRefToCurrQ (m_pvopcRefQ0, x, y, ppxlcCurrQMBY, ppxlcCurrQMBU, ppxlcCurrQMBV, &m_rctRefVOPY0);
						else
							copyFromPredForYAndRefForCToCurrQ (x, y, ppxlcCurrQMBY, ppxlcCurrQMBU, ppxlcCurrQMBV, &m_rctRefVOPY0);
					}
				}
				if (pmbmd->m_rgTranspStatus [0] == PARTIAL)
					mcPadCurrMB (ppxlcCurrQMBY, ppxlcCurrQMBU, ppxlcCurrQMBV, ppxlcCurrQMBA);
				padNeighborTranspMBs (
					iMBX, iMBY,
					pmbmd,
					ppxlcCurrQMBY, ppxlcCurrQMBU, ppxlcCurrQMBV, ppxlcCurrQMBA
				);
			}
			else {
				padCurrAndTopTranspMBFromNeighbor (
					iMBX, iMBY,
					pmbmd,
					ppxlcCurrQMBY, ppxlcCurrQMBU, ppxlcCurrQMBV, ppxlcCurrQMBA
				);
			}

			pmbmd++;
			pmv += PVOP_MV_PER_REF_PER_MB;
			piIntraDC += V_BLOCK;
			ppxlcCurrQMBY += MB_SIZE;
			ppxlcCurrQMBU += BLOCK_SIZE;
			ppxlcCurrQMBV += BLOCK_SIZE;
			ppxlcCurrQMBBY += MB_SIZE;
			ppxlcCurrQMBBUV += BLOCK_SIZE;
			ppxlcCurrQMBA += MB_SIZE;

			if(iMBX == m_iNumMBX - 1) {
				MacroBlockMemory** ppmbmTemp = m_rgpmbmAbove;
				m_rgpmbmAbove = m_rgpmbmCurr;
				m_rgpmbmCurr  = ppmbmTemp;
				ppxlcCurrQY += m_iFrameWidthYxMBSize;
				ppxlcCurrQU += m_iFrameWidthUVxBlkSize;
				ppxlcCurrQV += m_iFrameWidthUVxBlkSize;
				ppxlcCurrQBY += m_iFrameWidthYxMBSize;
				ppxlcCurrQBUV += m_iFrameWidthYxBlkSize;
				ppxlcCurrQA  += m_iFrameWidthYxMBSize;
			}
		}
	} while( checkResyncMarker() );
	delete m_piIntraDC;
	delete m_piMCBPC;
}

// End Toshiba

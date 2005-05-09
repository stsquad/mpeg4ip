/*************************************************************************

This software module was originally developed by 

	Yoshihiro Kikuchi (TOSHIBA CORPORATION)
	Takeshi Nagai (TOSHIBA CORPORATION)

    and edited by:

	Toshiaki Watanabe (TOSHIBA CORPORATION)
	Noboru Yamaguchi (TOSHIBA CORPORATION)

    and also edited by:
	Yoshinori Suzuki (Hitachi, Ltd.)

    and also edited by:
	Hideaki Kimata (NTT)

and also edited by
    Fujitsu Laboratories Ltd. (contact: Eishi Morimatsu)

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

Module Name:

	errdec.cpp

Revision History:

	Sep.06	1999 : RRV added by Eishi Morimatsu (Fujitsu Laboratories Ltd.)
	Mar.13	2000 : MB stuffing decoding added by Hideaki Kimata (NTT)
	May.25  2000 : MB stuffing decoding on the last MB by Hideaki Kimata (NTT)
*************************************************************************/

// Added for error resilience mode By Toshiba
#include <stdio.h>
#include <math.h>
#include <stdlib.h>

#include "typeapi.h"
#include "mode.hpp"
#include "codehead.h"
#include "bitstrm.hpp"
#include "entropy.hpp"
#include "huffman.hpp"
#include "global.hpp"
#include "vopses.hpp"
#include "vopsedec.hpp"

#include "dct.hpp"

// NEWPRED
#include "newpred.hpp"
// ~NEWPRED

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

Bool CVideoObjectDecoder::checkResyncMarker()
{
	if(short_video_header)
		return FALSE; // added by swinder

	Int nBitsPeeked;
	Int iStuffedBits = m_pbitstrmIn->peekBitsTillByteAlign (nBitsPeeked);
	Int nBitsResyncMarker = NUMBITS_VP_RESYNC_MARKER;
	if(m_volmd.bShapeOnly==FALSE)
	{
		if(m_vopmd.vopPredType == PVOP || (m_uiSprite == 2 && m_vopmd.vopPredType == SPRITE)) // GMC
			nBitsResyncMarker += (m_vopmd.mvInfoForward.uiFCode - 1);
		else if(m_vopmd.vopPredType == BVOP)
			nBitsResyncMarker += MAX(m_vopmd.mvInfoForward.uiFCode, m_vopmd.mvInfoBackward.uiFCode) - 1;
	}
		
	assert (nBitsPeeked > 0 && nBitsPeeked <= 8);
	if (iStuffedBits == ((1 << (nBitsPeeked - 1)) - 1))
		return (m_pbitstrmIn->peekBitsFromByteAlign (nBitsResyncMarker) == RESYNC_MARKER);
	return FALSE;
}

// May.25 2000 for MB stuffing decoding on the last MB
Bool CVideoObjectDecoder::checkStartCode()
{
	Int nBitsPeeked;
	Int iStuffedBits = m_pbitstrmIn->peekBitsTillByteAlign (nBitsPeeked);
		
	assert (nBitsPeeked > 0 && nBitsPeeked <= 8);
	if (iStuffedBits == ((1 << (nBitsPeeked - 1)) - 1))
		return (m_pbitstrmIn->peekBitsFromByteAlign (NUMBITS_START_CODE_PREFIX) == START_CODE_PREFIX);
	return FALSE;
}
// ~May.25 2000 for MB stuffing decoding on the last MB

Int	CVideoObjectDecoder::decodeVideoPacketHeader(Int& iCurrentQP)
{
	UInt uiHEC = 0;
	m_pbitstrmIn -> flush(8);

	Int nBitsResyncMarker = NUMBITS_VP_RESYNC_MARKER;
	if(m_volmd.bShapeOnly==FALSE)
	{
		if(m_vopmd.vopPredType == PVOP || (m_uiSprite == 2 && m_vopmd.vopPredType == SPRITE)) // GMC_V2
			nBitsResyncMarker += (m_vopmd.mvInfoForward.uiFCode - 1);
		else if(m_vopmd.vopPredType == BVOP)
			nBitsResyncMarker += MAX(m_vopmd.mvInfoForward.uiFCode, m_vopmd.mvInfoBackward.uiFCode) - 1;
	}

	/* UInt uiResyncMarker = wmay */ m_pbitstrmIn -> getBits (nBitsResyncMarker);

// RRV modification
	Int	NumOfMB = m_iNumMBX * m_iNumMBY *m_iRRVScale *m_iRRVScale;
//	Int	NumOfMB = m_iNumMBX * m_iNumMBY;
// ~RRV
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
		Time tVopIncr = 0;
		if(m_iNumBitsTimeIncr!=0)
			tVopIncr = m_pbitstrmIn -> getBits (m_iNumBitsTimeIncr);
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
// GMC_V2
			if (m_uiSprite == 2 && m_vopmd.vopPredType == SPRITE && m_iNumOfPnts > 0)
				decodeWarpPoints ();
// ~GMC_V2
// RRV insertion
			if((m_volmd.breduced_resolution_vop_enable == 1)&&(m_volmd.fAUsage == RECTANGLE)&&
			   ((m_vopmd.vopPredType == PVOP)||(m_vopmd.vopPredType == IVOP)))
			{
				UInt uiVOP_RR	= m_pbitstrmIn -> getBits (1);
				assert(uiVOP_RR == (UInt)m_vopmd.RRVmode.iRRVOnOff);
			}	
// ~RRV
			if (m_vopmd.vopPredType == PVOP || (m_uiSprite == 2 && m_vopmd.vopPredType == SPRITE)) { // GMC_V2
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

// NEWPRED
	if (m_volmd.bNewpredEnable) {
		m_vopmd.m_iVopID = m_pbitstrmIn -> getBits(m_vopmd.m_iNumBitsVopID);
		m_vopmd.m_iVopID4Prediction_Indication = m_pbitstrmIn -> getBits(NUMBITS_VOP_ID_FOR_PREDICTION_INDICATION);
		if( m_vopmd.m_iVopID4Prediction_Indication )
			m_vopmd.m_iVopID4Prediction = m_pbitstrmIn -> getBits(m_vopmd.m_iNumBitsVopID);
		m_pbitstrmIn -> getBits(MARKER_BIT);
		g_pNewPredDec->GetRef(
				NP_VP_HEADER,
				m_vopmd.vopPredType,
				m_vopmd.m_iVopID,	
				m_vopmd.m_iVopID4Prediction_Indication,
				m_vopmd.m_iVopID4Prediction
		);
	}
// ~NEWPRED

	return	TRUE;
}

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
	char	pSlicePoint[128];
	pSlicePoint[0] = '0';
	pSlicePoint[1] = '0'; //NULL;

	memset (m_rgmv, 0, m_iNumMB * PVOP_MV_PER_REF_PER_MB * sizeof (CMotionVector));

	Int iMBX=0, iMBY=0;
	CMBMode* pmbmd = m_rgmbmd;
	PixelC* ppxlcRefY = (PixelC*) m_pvopcRefQ1->pixelsY () + m_iStartInRefToCurrRctY;
	PixelC* ppxlcRefU = (PixelC*) m_pvopcRefQ1->pixelsU () + m_iStartInRefToCurrRctUV;
	PixelC* ppxlcRefV = (PixelC*) m_pvopcRefQ1->pixelsV () + m_iStartInRefToCurrRctUV;

	Int iCurrentQP  = m_vopmd.intStepI;		
	Int	iVideoPacketNumber = 0;
	m_iVPMBnum = 0;

	m_piMCBPC = new Int[m_iNumMBX*m_iNumMBY+1];

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
			//printf("VP");
			if (m_volmd.bNewpredEnable) {
				if (m_volmd.bNewpredSegmentType == 0)
					if(m_iRRVScale == 2)
					{
						Int iMBX_t = mbn % m_iNumMBX;
						Int iMBY_t = mbn / m_iNumMBX;
						Int i_mbn	= (iMBY_t *m_iRRVScale) *(m_iNumMBX *m_iRRVScale) + (iMBX_t *m_iRRVScale);
						sprintf(pSlicePoint, "%s,%d",pSlicePoint, i_mbn); // set slice number
					}
					else
					{
						sprintf(pSlicePoint, "%s,%d",pSlicePoint, mbn); // set slice number
					}
				else
					pSlicePoint[0] = '1';
			}
		}

		CMBMode* pmbmdFirst = pmbmd;
		Int* piMCBPCFirst = piMCBPC;
		Int* piIntraDCFirst = piIntraDC;
		mbnFirst = mbn;
		
		do{
			pmbmd->m_iVideoPacketNumber = iVideoPacketNumber;

			*piMCBPC = m_pentrdecSet->m_pentrdecMCBPCintra->decodeSymbol ();
			assert (*piMCBPC >= 0 && *piMCBPC <= 8);
			if (*piMCBPC == 8) {
				if( checkDCMarker() )
					break;
				continue;
			}
			pmbmd->m_dctMd = INTRA;
			if (*piMCBPC > 3)
				pmbmd->m_dctMd = INTRAQ;

			decodeMBTextureDCOfIVOP_DataPartitioning (pmbmd, iCurrentQP, piIntraDC, &bRestartDelayedQP);
			//printf("(%d:%d:%d)", *piMCBPC, pmbmd->m_bCodeDcAsAc, iCurrentQP);
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
// RRV modification
		PixelC* ppxlcRefMBY = ppxlcRefY
			+ (mbnFirst%m_iNumMBX)*(MB_SIZE *m_iRRVScale);
		PixelC* ppxlcRefMBU = ppxlcRefU 
			+ (mbnFirst%m_iNumMBX)*(BLOCK_SIZE *m_iRRVScale);
		PixelC* ppxlcRefMBV = ppxlcRefV 
			+ (mbnFirst%m_iNumMBX)*(BLOCK_SIZE *m_iRRVScale);

// ~RRV
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
// RRV modification
			ppxlcRefMBY += (MB_SIZE *m_iRRVScale);
			ppxlcRefMBU += (BLOCK_SIZE *m_iRRVScale);
			ppxlcRefMBV += (BLOCK_SIZE *m_iRRVScale);

// ~RRV
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
	
// RRV insertion
	if(m_vopmd.RRVmode.iRRVOnOff == 1)
	{
		PixelC* ppxlcCurrQY = (PixelC*) m_pvopcRefQ1->pixelsY () + m_iStartInRefToCurrRctY;
		PixelC* ppxlcCurrQU = (PixelC*) m_pvopcRefQ1->pixelsU () + m_iStartInRefToCurrRctUV;
		PixelC* ppxlcCurrQV = (PixelC*) m_pvopcRefQ1->pixelsV () + m_iStartInRefToCurrRctUV;
	    filterCodedPictureForRRV(ppxlcCurrQY, ppxlcCurrQU, ppxlcCurrQV,
								 m_iVOPWidthY, m_rctCurrVOPY.height(),
								 m_iNumMBX,  m_iNumMBY,
								 m_pvopcRefQ0->whereY ().width,
								 m_pvopcRefQ0->whereUV ().width);
	}

// ~RRV
// NEWPRED
	if (m_volmd.bNewpredEnable) {
		int iCurrentVOP_id = g_pNewPredDec->GetCurrentVOP_id();		// temporarily store Vop_id from old NEWPRED object
		if (g_pNewPredDec != NULL) delete g_pNewPredDec;		// delete old NEWPRED object
		g_pNewPredDec = new CNewPredDecoder();			// make new NEWPRED object
		g_pNewPredDec->SetObject(
			m_iNumBitsTimeIncr,
// RRV modification
			m_iNumMBX*MB_SIZE *m_iRRVScale,
			m_iNumMBY*MB_SIZE *m_iRRVScale,

// ~RRV
			pSlicePoint,
			m_volmd.bNewpredSegmentType,
			m_volmd.fAUsage,
			m_volmd.bShapeOnly,
			m_pvopcRefQ0,
			m_pvopcRefQ1,
			m_rctRefFrameY,
			m_rctRefFrameUV
		);
	  	g_pNewPredDec->ResetObject(iCurrentVOP_id); // restore Vop_id in new NEWPRED object
	
		Int i;
		Int noStore_vop_id;	// stored Vop_id

		// set NEWPRED reference picture memory
		g_pNewPredDec->SetQBuf( m_pvopcRefQ0, m_pvopcRefQ1 );

		// store decoded picture in  NEWPRED reference picture memory
		for (i=0; i < g_pNewPredDec->m_iNumSlice; i++ ) {
		    noStore_vop_id = g_pNewPredDec->make_next_decbuf(g_pNewPredDec->m_pNewPredControl, 
											g_pNewPredDec->GetCurrentVOP_id(), i);
		}
	}
// ~NEWPRED

	delete m_piMCBPC;
	delete m_piIntraDC;
}

Void CVideoObjectDecoder::decodePVOP_DataPartitioning ()
{
	Int iMBX, iMBY;
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

	m_piMCBPC = new Int[m_iNumMBX*m_iNumMBY+1];
	Int*	piMCBPC = m_piMCBPC;
	m_piIntraDC = new Int[m_iNumMBX*m_iNumMBY*V_BLOCK];
	Int*	piIntraDC = m_piIntraDC;

	Int	i;
	Int	mbn = 0, mbnFirst = 0;

	CoordI x = 0;
	CoordI y = 0;
	PixelC* ppxlcCurrQMBY = NULL;
	PixelC* ppxlcCurrQMBU = NULL;
	PixelC* ppxlcCurrQMBV = NULL;

	Bool bMBBackup = FALSE;
	CMBMode* pmbmdBackup = NULL;
	Int iMBXBackup  = 0, iMBYBackup = 0;
	CMotionVector* pmvBackup = NULL;
	PixelC* ppxlcCurrQMBYBackup = NULL;
	PixelC* ppxlcCurrQMBUBackup = NULL;
	PixelC* ppxlcCurrQMBVBackup = NULL;

	Bool bRestartDelayedQP = TRUE;

	Bool bRet;
	const PixelC* RefbufY = m_pvopcRefQ0-> pixelsY ();
	const PixelC* RefbufU = m_pvopcRefQ0-> pixelsU ();
	const PixelC* RefbufV = m_pvopcRefQ0-> pixelsV ();
	PixelC *RefpointY, *RefpointU, *RefpointV;
	PixelC *pRefpointY, *pRefpointU, *pRefpointV;

	if (m_volmd.bNewpredEnable) {
		// temporarily store present reference picture
		g_pNewPredDec->CopyReftoBuf(RefbufY, RefbufU, RefbufV, m_rctRefFrameY, m_rctRefFrameUV);
		bRet = FALSE;

		iMBX = mbn % m_iNumMBX;
		iMBY = mbn / m_iNumMBX;

		// copy picture from NEWPRED reference picture memory
		RefpointY = (PixelC*) g_pNewPredDec->m_pchNewPredRefY +
				(EXPANDY_REF_FRAME * m_rctRefFrameY.width);
		RefpointU = (PixelC*) g_pNewPredDec->m_pchNewPredRefU +
				((EXPANDY_REF_FRAME/2)*m_rctRefFrameUV.width);
		RefpointV = (PixelC*) g_pNewPredDec->m_pchNewPredRefV +
				((EXPANDY_REF_FRAME/2)*m_rctRefFrameUV.width);
		bRet = g_pNewPredDec->CopyNPtoVM(iVideoPacketNumber, RefpointY, RefpointU, RefpointV);

		// padding around VP
		pRefpointY = (PixelC*) m_pvopcRefQ0->pixelsY () + m_iStartInRefToCurrRctY;
		pRefpointU = (PixelC*) m_pvopcRefQ0->pixelsU () + m_iStartInRefToCurrRctUV;
		pRefpointV = (PixelC*) m_pvopcRefQ0->pixelsV () + m_iStartInRefToCurrRctUV;
// RRV modification
		g_pNewPredDec->ChangeRefOfSlice((const PixelC* )pRefpointY, RefbufY,(const PixelC* )pRefpointU, RefbufU,
			(const PixelC* )pRefpointV, RefbufV, (iMBX *m_iRRVScale), (iMBY *m_iRRVScale),m_rctRefFrameY, m_rctRefFrameUV);
//		g_pNewPredDec->ChangeRefOfSlice((const PixelC* )pRefpointY, RefbufY,(const PixelC* )pRefpointU, RefbufU,
//			(const PixelC* )pRefpointV, RefbufV, iMBX, iMBY,m_rctRefFrameY, m_rctRefFrameUV);
// ~RRV
		repeatPadYOrA ((PixelC*) m_pvopcRefQ0->pixelsY () + m_iOffsetForPadY, m_pvopcRefQ0);
		repeatPadUV (m_pvopcRefQ0);

		for (iMBY = 0; iMBY < m_iNumMBY; iMBY++) { // count slice number
			for (iMBX = 0; iMBX < m_iNumMBX; iMBX++)	{
// RRV modification
				(pmbmd + iMBX + m_iNumMBX*iMBY) -> m_iNPSegmentNumber = g_pNewPredDec->GetSliceNum((iMBX *m_iRRVScale),(iMBY *m_iRRVScale));
//				(pmbmd + iMBX + m_iNumMBX*iMBY) -> m_iNPSegmentNumber = g_pNewPredDec->GetSliceNum(iMBX,iMBY);
// ~RRV
			}
		}

	}
// ~NEWPRED

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

			Bool bStuffing = decodeMBTextureModeOfPVOP_DataPartitioning(pmbmd, piMCBPC);
			if(bStuffing)
			{
				if ( checkMotionMarker() )
					break;
				continue;
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
// GMC
			if(!pmbmd -> m_bMCSEL)
// ~GMC
				decodeMV (pmbmd, pmv, bLeftBndry, bRightBndry, bTopBndry, FALSE, iMBX, iMBY);
// GMC
			else
			{
				Int iPmvx, iPmvy, iHalfx, iHalfy;
				globalmv (iPmvx, iPmvy, iHalfx, iHalfy,
					iMBX*MB_SIZE,iMBY*MB_SIZE,
					m_vopmd.mvInfoForward.uiRange, m_volmd.bQuarterSample);
				CVector vctOrg;
				vctOrg.x = iPmvx*2 + iHalfx;
				vctOrg.y = iPmvy*2 + iHalfy;
				*pmv= CMotionVector (iPmvx, iPmvy);
				pmv -> iHalfX = iHalfx;
				pmv -> iHalfY = iHalfy;
				pmv -> computeTrueMV ();
				pmv -> computeMV ();
				for (UInt i = 1; i < PVOP_MV_PER_REF_PER_MB; i++)
					pmv[i] = *pmv;
			}
// ~GMC

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
					if (!(pmbmdBackup->m_bSkip && !pmbmdBackup->m_bMCSEL)) { // GMC
						CoordI iXRefUV, iYRefUV, iXRefUV1, iYRefUV1;
// GMC
					    if (!pmbmdBackup->m_bMCSEL)
// ~GMC
						mvLookupUV (pmbmdBackup, pmvBackup, iXRefUV, iYRefUV, iXRefUV1, iYRefUV1);
// GMC
					    if(pmbmdBackup->m_bMCSEL) {
							FindGlobalChromPredForGMC(x,y,m_ppxlcPredMBU,m_ppxlcPredMBV);
						} 
						else
// ~GMC
							motionCompUV (m_ppxlcPredMBU, m_ppxlcPredMBV, m_pvopcRefQ0, x, y, iXRefUV, iYRefUV, m_vopmd.iRoundingControl, &m_rctRefVOPY0);
// GMC
					    if (pmbmdBackup->m_bSkip)
							assignPredToCurrQ (ppxlcCurrQMBYBackup, ppxlcCurrQMBUBackup, ppxlcCurrQMBVBackup);
					    else
// ~GMC
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

// NEWPRED
				if (m_volmd.bNewpredEnable && (m_volmd.bNewpredSegmentType == 0)) {				
					bRet = FALSE;
					// copy picture from NEWPRED reference picture memory
// RRV modification
					RefpointY = (PixelC*) g_pNewPredDec->m_pchNewPredRefY +
				  		(EXPANDY_REF_FRAME * m_rctRefFrameY.width) + 
				  		iMBY * (MB_SIZE *m_iRRVScale)* m_rctRefFrameY.width;
					RefpointU = (PixelC*) g_pNewPredDec->m_pchNewPredRefU +
				  		((EXPANDY_REF_FRAME/2)*m_rctRefFrameUV.width) + 
					 	iMBY * (BLOCK_SIZE *m_iRRVScale) * m_rctRefFrameUV.width;
					RefpointV = (PixelC*) g_pNewPredDec->m_pchNewPredRefV +
				 		((EXPANDY_REF_FRAME/2)*m_rctRefFrameUV.width) + 
				 		iMBY * (BLOCK_SIZE *m_iRRVScale) * m_rctRefFrameUV.width;
					bRet = g_pNewPredDec->CopyNPtoVM(iVideoPacketNumber, RefpointY, RefpointU, RefpointV);

					pRefpointY = (PixelC*) m_pvopcRefQ0->pixelsY () + m_iStartInRefToCurrRctY + iMBY * (MB_SIZE *m_iRRVScale) * m_rctRefFrameY.width;
					pRefpointU = (PixelC*) m_pvopcRefQ0->pixelsU () + m_iStartInRefToCurrRctUV + iMBY * (BLOCK_SIZE *m_iRRVScale) * m_rctRefFrameUV.width;
					pRefpointV = (PixelC*) m_pvopcRefQ0->pixelsV () + m_iStartInRefToCurrRctUV + iMBY * (BLOCK_SIZE *m_iRRVScale) * m_rctRefFrameUV.width;
					// padding around VP
					g_pNewPredDec->ChangeRefOfSlice((const PixelC* )pRefpointY, RefbufY,(const PixelC* )pRefpointU, RefbufU,
				  		(const PixelC* )pRefpointV, RefbufV, (iMBX *m_iRRVScale), (iMBY *m_iRRVScale), m_rctRefFrameY, m_rctRefFrameUV);
					repeatPadYOrA ((PixelC*) m_pvopcRefQ0->pixelsY () + m_iOffsetForPadY, m_pvopcRefQ0);
					repeatPadUV (m_pvopcRefQ0);
				}		
// ~NEWPRED
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
		//printf("#");
		for(i=mbnFirst;i<mbn;i++) {
			decodeMBTextureHeadOfPVOP_DataPartitioning (pmbmd, iCurrentQP, piMCBPC, piIntraDC, &bRestartDelayedQP);
			//if(pmbmd->m_bSkip)
			//	printf("(Skip)");
			//else
			//	printf("(%d:%d:%d)", *piMCBPC, pmbmd->m_bCodeDcAsAc, pmbmd->m_stepSize);
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
// RRV modification
				if(iMBY != 0)	y += (MB_SIZE *m_iRRVScale);
//				if(iMBY != 0)	y += MB_SIZE;
// ~RRV
			} else {
// RRV modification
				x += (MB_SIZE *m_iRRVScale);
//				x += MB_SIZE;
// ~RRV
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
				if (!(pmbmd->m_bSkip && !pmbmd->m_bMCSEL)) { // GMC
					CoordI iXRefUV, iYRefUV, iXRefUV1, iYRefUV1;
// GMC
				   if(!pmbmd->m_bMCSEL)
// ~GMC
					mvLookupUV (pmbmd, pmv, iXRefUV, iYRefUV, iXRefUV1, iYRefUV1);
// GMC
				   if(pmbmd->m_bMCSEL){
					FindGlobalChromPredForGMC(x,y,m_ppxlcPredMBU,m_ppxlcPredMBV);
				   }else
// ~GMC
					motionCompUV (m_ppxlcPredMBU, m_ppxlcPredMBV, m_pvopcRefQ0, x, y, iXRefUV, iYRefUV, m_vopmd.iRoundingControl, &m_rctRefVOPY0);
// GMC
				   if(pmbmd->m_bSkip)
					assignPredToCurrQ (ppxlcCurrQMBY, ppxlcCurrQMBU, ppxlcCurrQMBV);
				   else
// ~GMC
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
// RRV modification
			ppxlcCurrQMBY += (MB_SIZE *m_iRRVScale);
			ppxlcCurrQMBU += (BLOCK_SIZE *m_iRRVScale);
			ppxlcCurrQMBV += (BLOCK_SIZE *m_iRRVScale);
//			ppxlcCurrQMBY += MB_SIZE;
//			ppxlcCurrQMBU += BLOCK_SIZE;
//			ppxlcCurrQMBV += BLOCK_SIZE;
// ~RRV
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

// RRV insertion
	if(m_vopmd.RRVmode.iRRVOnOff == 1)
	{
		PixelC* ppxlcCurrQY = (PixelC*) m_pvopcRefQ1->pixelsY () + m_iStartInRefToCurrRctY;
		PixelC* ppxlcCurrQU = (PixelC*) m_pvopcRefQ1->pixelsU () + m_iStartInRefToCurrRctUV;
		PixelC* ppxlcCurrQV = (PixelC*) m_pvopcRefQ1->pixelsV () + m_iStartInRefToCurrRctUV;
	    filterCodedPictureForRRV(ppxlcCurrQY, ppxlcCurrQU, ppxlcCurrQV,
								 m_iVOPWidthY, m_rctCurrVOPY.height(),
								 m_iNumMBX,  m_iNumMBY,
								 m_pvopcRefQ0->whereY ().width,
								 m_pvopcRefQ0->whereUV ().width);
	}

// ~RRV
// NEWPRED
	if (m_volmd.bNewpredEnable) {
		Int i;
		Int noStore_vop_id;	// stored Vop_id

		// store decoded picture in  NEWPRED reference picture memory
		g_pNewPredDec->SetQBuf( m_pvopcRefQ0, m_pvopcRefQ1 );
		for (i=0; i < g_pNewPredDec->m_iNumSlice; i++ ) {
			noStore_vop_id = g_pNewPredDec->make_next_decbuf(g_pNewPredDec->m_pNewPredControl, 
				g_pNewPredDec->GetCurrentVOP_id(), i);
		}
		// copy previous decoded picutre to reference picture memory because of output ording
		for( int iSlice = 0; iSlice < g_pNewPredDec->m_iNumSlice; iSlice++ ) {
			int		iMBY = g_pNewPredDec->NowMBA(iSlice)/((g_pNewPredDec->getwidth())/MB_SIZE);
			PixelC* RefpointY = (PixelC*) m_pvopcRefQ0->pixelsY () + (m_iStartInRefToCurrRctY-EXPANDY_REF_FRAME) + iMBY * MB_SIZE * m_rctRefFrameY.width;
			PixelC* RefpointU = (PixelC*) m_pvopcRefQ0->pixelsU () + (m_iStartInRefToCurrRctUV-EXPANDUV_REF_FRAME) + iMBY * BLOCK_SIZE * m_rctRefFrameUV.width;
			PixelC* RefpointV = (PixelC*) m_pvopcRefQ0->pixelsV () + (m_iStartInRefToCurrRctUV-EXPANDUV_REF_FRAME) + iMBY * BLOCK_SIZE * m_rctRefFrameUV.width;
			g_pNewPredDec->CopyNPtoPrev(iSlice, RefpointY, RefpointU, RefpointV);	
		}
		repeatPadYOrA ((PixelC*) m_pvopcRefQ0->pixelsY () + m_iOffsetForPadY, m_pvopcRefQ0);
		repeatPadUV (m_pvopcRefQ0);
	}
// ~NEWPRED

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
//	PixelC* ppxlcRefA  = (PixelC*) m_pvopcRefQ1->pixelsA (0) + m_iStartInRefToCurrRctY;
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
	//PixelC* ppxlcRefMBA;
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
			// Changed HHI 2000-04-11
			downSampleBY (m_ppxlcCurrMBBY, m_ppxlcCurrMBBUV, pmbmd); // downsample original BY now for LPE padding (using original shape)
			if(m_volmd.bShapeOnly==FALSE)
			{
				pmbmd->m_bPadded=FALSE;
				if (pmbmd->m_rgTranspStatus [0] != ALL) {
					*piMCBPC = m_pentrdecSet->m_pentrdecMCBPCintra->decodeSymbol ();
					assert (*piMCBPC >= 0 && *piMCBPC <= 8);
					while (*piMCBPC == 8) {
						*piMCBPC = m_pentrdecSet->m_pentrdecMCBPCintra->decodeSymbol ();
					};

					pmbmd->m_dctMd = INTRA;
					if (*piMCBPC > 3)
						pmbmd->m_dctMd = INTRAQ;
					decodeMBTextureDCOfIVOP_DataPartitioning (pmbmd, iCurrentQP, piIntraDC, &bRestartDelayedQP);
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
//				ppxlcRefMBA  = ppxlcRefA;
				ppxlcRefMBBY = ppxlcRefBY;
				ppxlcRefMBBUV = ppxlcRefBUV;
			}

			copyRefShapeToMb(m_ppxlcCurrMBBY, ppxlcRefMBBY);
			// Changed HHI 2000-04-11
			downSampleBY (m_ppxlcCurrMBBY, m_ppxlcCurrMBBUV, pmbmd); // downsample original BY now for LPE padding (using original shape)

			if (pmbmd->m_rgTranspStatus [0] != ALL) {
				// 09/16/99 HHI Schueuer: added for sadct
				if (!m_volmd.bSadctDisable)
					deriveSADCTRowLengths(m_rgiCurrMBCoeffWidth, m_ppxlcCurrMBBY, m_ppxlcCurrMBBUV, pmbmd ->m_rgTranspStatus);
				// end HHI
				// 09/16/99 HHI Schueuer: added for sadct
				if (!m_volmd.bSadctDisable)
					decodeTextureIntraMB_DataPartitioning (pmbmd, iMBX, iMBY, ppxlcRefMBY, ppxlcRefMBU, ppxlcRefMBV, piIntraDC, m_ppxlcCurrMBBY, m_ppxlcCurrMBBUV);
				else
					// end HHI 
					decodeTextureIntraMB_DataPartitioning (pmbmd, iMBX, iMBY, ppxlcRefMBY, ppxlcRefMBU, ppxlcRefMBV, piIntraDC); 

				// MC padding
				if (pmbmd -> m_rgTranspStatus [0] == PARTIAL)
					mcPadCurrMB (ppxlcRefMBY, ppxlcRefMBU, ppxlcRefMBV, NULL/*ppxlcRefMBA*/);
				padNeighborTranspMBs (
					iMBX, iMBY,
					pmbmd,
					ppxlcRefMBY, ppxlcRefMBU, ppxlcRefMBV, NULL /*ppxlcRefMBA*/
				);
			}
			else {
				padCurrAndTopTranspMBFromNeighbor (
					iMBX, iMBY,
					pmbmd,
					ppxlcRefMBY, ppxlcRefMBU, ppxlcRefMBV, NULL /*ppxlcRefMBA*/
				);
			}

//			ppxlcRefMBA += MB_SIZE;
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
//				ppxlcRefA += m_iFrameWidthYxMBSize;
				ppxlcRefBY += m_iFrameWidthYxMBSize;
				ppxlcRefBUV += m_iFrameWidthUVxBlkSize;
			}
		}
	} while( checkResyncMarker() );

	delete m_piMCBPC;
	delete m_piIntraDC;
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
//	PixelC* ppxlcCurrQA  = (PixelC*) m_pvopcRefQ1->pixelsA () + m_iStartInRefToCurrRctY;
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
	//PixelC* ppxlcCurrQMBA;

	Bool bMBBackup = FALSE;
	CMBMode* pmbmdBackup = NULL;
	Int iMBXBackup = 0, iMBYBackup = 0;
	CMotionVector* pmvBackup = NULL;
	PixelC* ppxlcCurrQMBYBackup = NULL;
	PixelC* ppxlcCurrQMBUBackup = NULL;
	PixelC* ppxlcCurrQMBVBackup = NULL;
	PixelC* ppxlcCurrQMBBYBackup = NULL;
	PixelC* ppxlcCurrQMBBUVBackup;
	//PixelC* ppxlcCurrQMBABackup;
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

			downSampleBY (m_ppxlcCurrMBBY, m_ppxlcCurrMBBUV, pmbmd);

			if (pmbmd->m_rgTranspStatus [0] != ALL && m_volmd.bShapeOnly==FALSE) {
				Bool bStuffing;
				do {
					bStuffing = decodeMBTextureModeOfPVOP_DataPartitioning(pmbmd, piMCBPC);
				} while(bStuffing);

				if(!pmbmd -> m_bMCSEL)
					decodeMVWithShape (pmbmd, iMBX, iMBY, pmv);
				else
				{
					Int iPmvx, iPmvy, iHalfx, iHalfy;
					globalmv (iPmvx, iPmvy, iHalfx, iHalfy,
						m_rctCurrVOPY.left+iMBX*MB_SIZE,
						m_rctCurrVOPY.top+iMBY*MB_SIZE,
						m_vopmd.mvInfoForward.uiRange,
						m_volmd.bQuarterSample);
					CVector vctOrg;
					vctOrg.x = iPmvx*2 + iHalfx;
					vctOrg.y = iPmvy*2 + iHalfy;
					*pmv= CMotionVector (iPmvx, iPmvy);
					pmv -> iHalfX = iHalfx;
					pmv -> iHalfY = iHalfy;
					pmv -> computeTrueMV ();
					pmv -> computeMV ();
					for (UInt i = 1; i < PVOP_MV_PER_REF_PER_MB; i++)
						pmv[i] = *pmv;
				}
				if(pmbmd->m_bhas4MVForward)
					padMotionVectors(pmbmd,pmv);
			}

			if(bMBBackup){
				copyRefShapeToMb(m_ppxlcCurrMBBY, ppxlcCurrQMBBYBackup);
				// Changed HHI 2000-04-11
				downSampleBY (m_ppxlcCurrMBBY, m_ppxlcCurrMBBUV, pmbmdBackup); // downsample original BY now for LPE padding (using original shape)
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
						if (!(pmbmdBackup->m_bSkip && !pmbmdBackup->m_bMCSEL)) { // GMC
							CoordI iXRefUV, iYRefUV;
// GMC
						   if (!pmbmdBackup->m_bMCSEL)
// ~GMC
							mvLookupUVWithShape (pmbmdBackup, pmvBackup, iXRefUV, iYRefUV);
// GMC
						   if(pmbmdBackup->m_bMCSEL) {
							FindGlobalChromPredForGMC(x,y,m_ppxlcPredMBU,m_ppxlcPredMBV);
						   }else
// ~GMC
							motionCompUV (m_ppxlcPredMBU, m_ppxlcPredMBV, m_pvopcRefQ0,
								x, y, iXRefUV, iYRefUV, m_vopmd.iRoundingControl,&m_rctRefVOPY0);
// GMC
						   if (pmbmdBackup->m_bSkip)
							assignPredToCurrQ (ppxlcCurrQMBYBackup, ppxlcCurrQMBUBackup, ppxlcCurrQMBVBackup);
						   else
// ~GMC
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
						mcPadCurrMB (ppxlcCurrQMBYBackup, ppxlcCurrQMBUBackup, ppxlcCurrQMBVBackup, NULL/*ppxlcCurrQMBABackup*/);
					padNeighborTranspMBs (
						iMBXBackup, iMBYBackup,
						pmbmdBackup,
						ppxlcCurrQMBYBackup, ppxlcCurrQMBUBackup, ppxlcCurrQMBVBackup, NULL /*ppxlcCurrQMBABackup*/
					);
				}
				else {
					padCurrAndTopTranspMBFromNeighbor (
						iMBXBackup, iMBYBackup,
						pmbmdBackup,
						ppxlcCurrQMBYBackup, ppxlcCurrQMBUBackup, ppxlcCurrQMBVBackup, NULL/*ppxlcCurrQMBABackup*/
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
				decodeMBTextureHeadOfPVOP_DataPartitioning (pmbmd, iCurrentQP, piMCBPC, piIntraDC, &bRestartDelayedQP);
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
//				ppxlcCurrQMBA  = ppxlcCurrQA;
				ppxlcCurrQMBBUV = ppxlcCurrQBUV;
				x = m_rctCurrVOPY.left;
				if(iMBY != 0)	y += MB_SIZE;
			} else {
				x += MB_SIZE;
			}

			copyRefShapeToMb(m_ppxlcCurrMBBY, ppxlcCurrQMBBY);
			// Changed HHI 2000-04-11
			downSampleBY (m_ppxlcCurrMBBY, m_ppxlcCurrMBBUV, pmbmd); // downsample original BY now for LPE padding (using original shape)

			if (pmbmd->m_rgTranspStatus [0] != ALL && m_volmd.bShapeOnly==FALSE) {
				// 09/16/99 HHI Schueuer: added for sadct
				if (!m_volmd.bSadctDisable)
					deriveSADCTRowLengths(m_rgiCurrMBCoeffWidth, m_ppxlcCurrMBBY, m_ppxlcCurrMBBUV, pmbmd ->m_rgTranspStatus);
				// end HHI
				if (pmbmd->m_dctMd == INTRA || pmbmd->m_dctMd == INTRAQ){
					// 09/16/99 HHI Schueuer: inserted for sadct
					if (!m_volmd.bSadctDisable)
						decodeTextureIntraMB_DataPartitioning (pmbmd, iMBX, iMBY, ppxlcCurrQMBY, ppxlcCurrQMBU, ppxlcCurrQMBV, piIntraDC, m_ppxlcCurrMBBY, m_ppxlcCurrMBBUV);
					else
						decodeTextureIntraMB_DataPartitioning (pmbmd, iMBX, iMBY, ppxlcCurrQMBY, ppxlcCurrQMBU, ppxlcCurrQMBV, piIntraDC);
				}
				// end HHI
				else {
					if (!pmbmd->m_bSkip){
						// 09/16/99 HHI Schueuer: inserted for sadct
						if (!m_volmd.bSadctDisable)
							decodeTextureInterMB (pmbmd, m_ppxlcCurrMBBY, m_ppxlcCurrMBBUV);
						else
						// end HHI 		
							decodeTextureInterMB (pmbmd);
					}
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
				//ppxlcCurrQMBABackup = ppxlcCurrQMBA;
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
					if (!(pmbmd->m_bSkip && !pmbmd->m_bMCSEL)) { // GMC
						CoordI iXRefUV, iYRefUV;
// GMC
					   if(!pmbmd->m_bMCSEL)
// ~GMC
						mvLookupUVWithShape (pmbmd, pmv, iXRefUV, iYRefUV);
// GMC
					   if(pmbmd->m_bMCSEL){
							FindGlobalChromPredForGMC(x,y,m_ppxlcPredMBU,m_ppxlcPredMBV);
					   }
					   else
// ~GMC
						motionCompUV (m_ppxlcPredMBU, m_ppxlcPredMBV, m_pvopcRefQ0,
							x, y, iXRefUV, iYRefUV, m_vopmd.iRoundingControl,&m_rctRefVOPY0);
// GMC
					   if(pmbmd->m_bSkip)
						assignPredToCurrQ (ppxlcCurrQMBY, ppxlcCurrQMBU, ppxlcCurrQMBV);
					   else
// ~GMC
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
					mcPadCurrMB (ppxlcCurrQMBY, ppxlcCurrQMBU, ppxlcCurrQMBV, NULL/*ppxlcCurrQMBA*/);
				padNeighborTranspMBs (
					iMBX, iMBY,
					pmbmd,
					ppxlcCurrQMBY, ppxlcCurrQMBU, ppxlcCurrQMBV, NULL /*ppxlcCurrQMBA*/
				);
			}
			else {
				padCurrAndTopTranspMBFromNeighbor (
					iMBX, iMBY,
					pmbmd,
					ppxlcCurrQMBY, ppxlcCurrQMBU, ppxlcCurrQMBV, NULL /*ppxlcCurrQMBA*/
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
			//ppxlcCurrQMBA += MB_SIZE;

			if(iMBX == m_iNumMBX - 1) {
				MacroBlockMemory** ppmbmTemp = m_rgpmbmAbove;
				m_rgpmbmAbove = m_rgpmbmCurr;
				m_rgpmbmCurr  = ppmbmTemp;
				ppxlcCurrQY += m_iFrameWidthYxMBSize;
				ppxlcCurrQU += m_iFrameWidthUVxBlkSize;
				ppxlcCurrQV += m_iFrameWidthUVxBlkSize;
				ppxlcCurrQBY += m_iFrameWidthYxMBSize;
				ppxlcCurrQBUV += m_iFrameWidthYxBlkSize;
//				ppxlcCurrQA  += m_iFrameWidthYxMBSize;
			}
		}
	} while( checkResyncMarker() );

	delete m_piIntraDC;
	delete m_piMCBPC;
}

// End Toshiba

Void CVideoObjectDecoder::decodeMBTextureDCOfIVOP_DataPartitioning (CMBMode* pmbmd, Int& iCurrentQP,
																	Int* piIntraDC, Bool *pbRestart)
{
	Int iBlk = 0;
	
	pmbmd->m_intStepDelta = 0;
	pmbmd->m_bSkip = FALSE;

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
		iCurrentQP += pmbmd->m_intStepDelta;
		Int iQuantMax = (1<<m_volmd.uiQuantPrecision) - 1;
		iCurrentQP = checkrange (iCurrentQP, 1, iQuantMax);
	}

	pmbmd->m_stepSize = iCurrentQP;

	assert (pmbmd != NULL);
	if (pmbmd -> m_rgTranspStatus [0] == ALL) 
		return;
	assert (pmbmd->m_dctMd == INTRA || pmbmd->m_dctMd == INTRAQ);

	setDCVLCMode(pmbmd, pbRestart);	

	if (!pmbmd->m_bCodeDcAsAc)
	{
		for (iBlk = Y_BLOCK1; iBlk < U_BLOCK; iBlk++) {
			if (pmbmd->m_rgTranspStatus [iBlk] != ALL)
				piIntraDC[iBlk - 1] = decodeIntraDCmpeg (1);	
		}
		for (iBlk = U_BLOCK; iBlk <= V_BLOCK; iBlk++) {
			piIntraDC[iBlk - 1] = decodeIntraDCmpeg (0);
		}
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

	pmbmd->m_dctMd = INTRA;
	pmbmd->m_bSkip = FALSE; //reset for direct mode 
	if (*piMCBPC > 3)
		pmbmd->m_dctMd = INTRAQ;
	pmbmd->m_bMCSEL = FALSE; //reset for direct mode

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

Bool CVideoObjectDecoder::decodeMBTextureModeOfPVOP_DataPartitioning (CMBMode* pmbmd, Int* piMCBPC)
{
	pmbmd->m_bSkip = m_pbitstrmIn->getBits (1);
	if (!pmbmd->m_bSkip) {
		*piMCBPC = m_pentrdecSet->m_pentrdecMCBPCinter->decodeSymbol ();
		assert (*piMCBPC >= 0 && *piMCBPC <= 20);			
		if (*piMCBPC == 20)
			return TRUE; // stuffing

		Int iMBtype = *piMCBPC / 4;					//per H.263's MBtype
		switch (iMBtype) {			
		case 0:
			pmbmd->m_dctMd = INTER;
			pmbmd->m_bhas4MVForward = FALSE;
			break;
		case 1:
			pmbmd->m_dctMd = INTERQ;
			pmbmd->m_bhas4MVForward = FALSE;
			break;
		case 2:
			pmbmd->m_dctMd = INTER;
			pmbmd->m_bhas4MVForward = TRUE;
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
		
		pmbmd->m_bMCSEL = FALSE;
		if((pmbmd->m_dctMd == INTER || pmbmd->m_dctMd == INTERQ) && (pmbmd -> m_bhas4MVForward == FALSE) && (m_uiSprite == 2 && m_vopmd.vopPredType == SPRITE))
			pmbmd->m_bMCSEL = m_pbitstrmIn->getBits (1);
	} else	{									//skipped
		pmbmd->m_dctMd = INTER;
		pmbmd->m_bhas4MVForward = FALSE;
		pmbmd->m_bMCSEL = FALSE;
		if(m_uiSprite == 2 && m_vopmd.vopPredType == SPRITE)
			pmbmd -> m_bMCSEL = TRUE;
	}

	return FALSE;
}

Void CVideoObjectDecoder::decodeMBTextureHeadOfPVOP_DataPartitioning (CMBMode* pmbmd, Int& iCurrentQP, Int* piMCBPC,
																	  Int* piIntraDC, Bool *pbRestart)
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
	  //		Int iMBtype = *piMCBPC / 4;					//per H.263's MBtype
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
		pmbmd->m_bhas4MVForward = FALSE;
	}
	setCBPYandC (pmbmd, iCBPC, iCBPY, cNonTrnspBlk);
	
	pmbmd->m_intStepDelta = 0;

	if (pmbmd->m_dctMd == INTERQ || pmbmd->m_dctMd == INTRAQ) {
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
		iCurrentQP += pmbmd->m_intStepDelta;

		Int iQuantMax = (1<<m_volmd.uiQuantPrecision) - 1;
		if (iCurrentQP < 1)
			iCurrentQP = 1;
		else if (iCurrentQP > iQuantMax)
			iCurrentQP = iQuantMax;
	}

	pmbmd->m_stepSize = iCurrentQP;

	setDCVLCMode(pmbmd, pbRestart);

	if(pmbmd->m_dctMd == INTRA || pmbmd->m_dctMd == INTRAQ)	{
		if (!pmbmd->m_bCodeDcAsAc)
		{
			for (iBlk = Y_BLOCK1; iBlk < U_BLOCK; iBlk++) {
				if (pmbmd->m_rgTranspStatus [iBlk] != ALL)
					piIntraDC[iBlk - 1] = decodeIntraDCmpeg (1);	
			}
			for (iBlk = U_BLOCK; iBlk <= V_BLOCK; iBlk++) {
				piIntraDC[iBlk - 1] = decodeIntraDCmpeg (0);
			}
		}
	}
}

// HHI Schueuer: added const PixelC *rgpxlcBlkShape,Int iBlkShapeWidth for sadct
Void	CVideoObjectDecoder::decodeIntraBlockTextureTcoef_DataPartitioning (PixelC* rgpxlcBlkDst,
								 Int iWidthDst,
								 Int iQP,
								 Int iDcScaler,
								 Int iBlk,
								 MacroBlockMemory* pmbmCurr,
								 CMBMode* pmbmd,
 								 const BlockMemory blkmPred,
								 Int iQpPred,
								 Int* piIntraDC,
								 const PixelC *rgpxlcBlkShape,
                                 Int iBlkShapeWidth)

{
	Int	iCoefStart = 0;
	if (!pmbmd->m_bCodeDcAsAc) {
		iCoefStart++;
	}

	Int* rgiCoefQ = m_rgpiCoefQ [iBlk - 1];
	rgiCoefQ[0] = piIntraDC [iBlk - 1];
	if (pmbmd->getCodedBlockPattern (iBlk))	{
		Int* rgiZigzag = grgiStandardZigzag;
		if (pmbmd->m_bACPrediction)	
			rgiZigzag = (pmbmd->m_preddir [iBlk - 1] == HORIZONTAL) ? grgiVerticalZigzag : grgiHorizontalZigzag;
		// 09/16/99 HHI Schueuer:added for sadct
		if (!m_volmd.bSadctDisable)
				rgiZigzag = m_pscanSelector->select(rgiZigzag, (pmbmd->m_rgTranspStatus[0] == PARTIAL), iBlk); 
		// end HHI
 		if(m_volmd.bReversibleVlc)
 			decodeIntraRVLCTCOEF (rgiCoefQ, iCoefStart, rgiZigzag);
 		else
			decodeIntraTCOEF (rgiCoefQ, iCoefStart, rgiZigzag);
	}
	else
		memset (rgiCoefQ + iCoefStart, 0, sizeof (Int) * (BLOCK_SQUARE_SIZE - iCoefStart));
	inverseDCACPred (pmbmd, iBlk - 1, rgiCoefQ, iQP, iDcScaler, blkmPred, iQpPred);
	inverseQuantizeIntraDc (rgiCoefQ, iDcScaler);
	// 09/16/99 HHI Schueuer: sadct 
	if (rgpxlcBlkShape) {
		// HHI Schueuer: for greyscale coding (iBlk-6)
		// assert(pmbmd->m_rgTranspStatus [iBlk] == PARTIAL);
		// end HHI 02/04/99

       	// brute force method to clean out mispredictions outside the active region
           Int *lx = m_rgiCurrMBCoeffWidth[iBlk];
           Int iy, ix;
           
           for (ix=lx[0]; ix<BLOCK_SIZE; ix++)
           	rgiCoefQ[ix] = 0;
			for (iy=1; iy<BLOCK_SIZE; iy++) {
            	if (!lx[iy])
                	rgiCoefQ[iy*BLOCK_SIZE] = 0;
            }
	}
	// end HHI

	if (m_volmd.fQuantizer == Q_H263)
		inverseQuantizeDCTcoefH263 (rgiCoefQ, 1, iQP);
	else
		inverseQuantizeIntraDCTcoefMPEG (rgiCoefQ, 1, iQP, iBlk>=A_BLOCK1, 0);

	Int i, j;							//save coefQ (ac) for intra pred
	pmbmCurr->rgblkm [iBlk - 1] [0] = m_rgiDCTcoef [0];	//save recon value of DC for intra pred								//save Qcoef in memory
	for (i = 1, j = 8; i < BLOCK_SIZE; i++, j += BLOCK_SIZE)	{
		pmbmCurr->rgblkm [iBlk - 1] [i] = rgiCoefQ [i];
		pmbmCurr->rgblkm [iBlk - 1] [i + BLOCK_SIZE - 1] = rgiCoefQ [j];
	}


// RRV modification
	PixelC *pc_block8x8, *pc_block16x16;
	if(m_vopmd.RRVmode.iRRVOnOff == 1)
	{
		pc_block8x8		= new PixelC [64];
		pc_block16x16	= new PixelC [256];
		// 09/16/99 HHI Schueuer: added for sadct
		// m_pidct->apply (m_rgiDCTcoef, BLOCK_SIZE, pc_block8x8, BLOCK_SIZE);
		m_pidct->apply (m_rgiDCTcoef, BLOCK_SIZE, pc_block8x8, BLOCK_SIZE, rgpxlcBlkShape, iBlkShapeWidth);
		MeanUpSampling(pc_block8x8, pc_block16x16, 8, 8);
		writeCubicRct(16, iWidthDst, pc_block16x16, rgpxlcBlkDst);
		delete pc_block8x8;
		delete pc_block16x16;
	}
	else
	{
		// 09/16/99 HHI Schueuer: added for sadct
		// m_pidct->apply (m_rgiDCTcoef, BLOCK_SIZE, rgpxlcBlkDst, iWidthDst);
		m_pidct->apply (m_rgiDCTcoef, BLOCK_SIZE, rgpxlcBlkDst, iWidthDst, rgpxlcBlkShape, iBlkShapeWidth);
	}
//	m_pidct->apply (m_rgiDCTcoef, BLOCK_SIZE, rgpxlcBlkDst, iWidthDst);
// ~RRV
}

// 09/16/99 HHI schueuer: added const PixelC* m_ppxlcCurrMBBY = NULL, const PixelC* m_ppxlcCurrMBBUV = NULL
Void CVideoObjectDecoder::decodeTextureIntraMB_DataPartitioning (
	CMBMode* pmbmd, CoordI iMBX, CoordI iMBY, 
	PixelC* ppxlcCurrFrmQY, PixelC* ppxlcCurrFrmQU, PixelC* ppxlcCurrFrmQV, Int* piIntraDC,
	const PixelC* ppxlcCurrMBBY, const PixelC* ppxlcCurrMBBUV)
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
	// 09/16/99 HHI Schueuer : for sadct, see also the cond. assignments inside the next switch stmt.
	const PixelC* rgchBlkShape = NULL;
	// end HHI
	Int iWidthDst;
	Int iDcScaler;
	Int* rgiCoefQ;
	for (Int iBlk = Y_BLOCK1; iBlk <= V_BLOCK; iBlk++) {
	  if (iBlk < /* (UInt)*/ U_BLOCK) {
			if (pmbmd -> m_rgTranspStatus [iBlk] == ALL) 
				continue;
			switch (iBlk) 
			{
			case (Y_BLOCK1): 
				rgchBlkDst = ppxlcCurrFrmQY;
				rgchBlkShape = (ppxlcCurrMBBY && pmbmd -> m_rgTranspStatus[iBlk] == PARTIAL) ? ppxlcCurrMBBY : NULL;
				break;
			case (Y_BLOCK2): 
// RRV modification
				rgchBlkDst = ppxlcCurrFrmQY + (BLOCK_SIZE *m_iRRVScale);
//				rgchBlkDst = ppxlcCurrFrmQY + BLOCK_SIZE;
// ~RRV
				rgchBlkShape = (ppxlcCurrMBBY && pmbmd -> m_rgTranspStatus[iBlk] == PARTIAL) ? ppxlcCurrMBBY + BLOCK_SIZE : NULL;
				break;
			case (Y_BLOCK3): 
				rgchBlkDst = ppxlcCurrFrmQY + m_iFrameWidthYxBlkSize;
				rgchBlkShape = (ppxlcCurrMBBY && pmbmd -> m_rgTranspStatus[iBlk] == PARTIAL) ? ppxlcCurrMBBY + BLOCK_SIZE*MB_SIZE : NULL;
				break;
			case (Y_BLOCK4): 
// RRV modification
				rgchBlkDst = ppxlcCurrFrmQY + m_iFrameWidthYxBlkSize
					+ (BLOCK_SIZE *m_iRRVScale);
//				rgchBlkDst = ppxlcCurrFrmQY + m_iFrameWidthYxBlkSize + BLOCK_SIZE;
// ~RRV
				rgchBlkShape = (ppxlcCurrMBBY && pmbmd -> m_rgTranspStatus[iBlk] == PARTIAL) ? ppxlcCurrMBBY + BLOCK_SIZE*MB_SIZE + BLOCK_SIZE : NULL;
				break;
			}
			iWidthDst = m_iFrameWidthY;
			iDcScaler = iDcScalerY;
		}
		else {
			iWidthDst = m_iFrameWidthUV;
			rgchBlkDst = (iBlk == U_BLOCK) ? ppxlcCurrFrmQU: ppxlcCurrFrmQV;
			rgchBlkShape = (ppxlcCurrMBBUV && pmbmd -> m_rgTranspStatus[iBlk] == PARTIAL) ? ppxlcCurrMBBUV : NULL; // 09/16/99 HHI Schueuer: added for sadct
			iDcScaler = iDcScalerC;
		}
		rgiCoefQ = m_rgpiCoefQ [iBlk - 1];
		const BlockMemory blkmPred = NULL;
		Int iQpPred = iQP; //default to current if no pred (use 128 case)

		decideIntraPred (blkmPred, 
						 pmbmd,
						 iQpPred,
						 (BlockNum)iBlk,
						 pmbmLeft, 
  						 pmbmTop, 
						 pmbmLeftTop,
						 m_rgpmbmCurr [iMBX],
						 pmbmdLeft,
						 pmbmdTop,
						 pmbmdLeftTop);
		decodeIntraBlockTextureTcoef_DataPartitioning (rgchBlkDst,
								 iWidthDst,
								 iQP, 
								 iDcScaler,
								 iBlk,	
								 m_rgpmbmCurr [iMBX],
								 pmbmd,
 								 blkmPred, //for intra-pred
								 iQpPred,
								 piIntraDC,
								 rgchBlkShape,
								 (iBlk<U_BLOCK) ? MB_SIZE : BLOCK_SIZE); // HHI Schueuer
	}
}

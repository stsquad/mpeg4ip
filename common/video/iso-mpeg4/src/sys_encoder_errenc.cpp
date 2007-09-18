/*************************************************************************

This software module was originally developed by 

	Yoshihiro Kikuchi (TOSHIBA CORPORATION)
	Takeshi Nagai (TOSHIBA CORPORATION)

    and edited by:

	Toshiaki Watanabe (TOSHIBA CORPORATION)
	Noboru Yamaguchi (TOSHIBA CORPORATION)

	and also edited by 

	Xuemin Chen (xchen@nlvl.com) Next Level System, Inc.
	Bob Eifrig (beifrig@nlvl.com) Next Level Systems, Inc.

    and also edited by 
	Yoshinori Suzuki (Hitachi, Ltd.)

    and also edited by 
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

Revision History:

	May 9, 1999:	tm5 rate control by DemoGraFX, duhoff@mediaone.net
	Aug.23, 1999:   NEWPRED added by Hideaki Kimata (NTT) 
	Sep.06	1999 : RRV added by Eishi Morimatsu (Fujitsu Laboratories Ltd.) 

*************************************************************************/

#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <iostream>

#include "typeapi.h"
#include "codehead.h"
#include "bitstrm.hpp"
#include "entropy.hpp"
#include "huffman.hpp"
#include "mode.hpp"
#include "global.hpp"
#include "vopses.hpp"
#include "vopseenc.hpp"
#include "tm5rc.hpp"    // added by mwi 

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

// inline functions
inline Int clog2(Int a)	{return ( ((a)<=0) ? 0 : (int) ceil( log10( (double) (a) ) / log10(2.0) ) );}


Void CVideoObjectEncoder::VideoPacketResetVOP(
)
{
	m_iVPCounter = m_pbitstrmOut -> getCounter();		// reset coding bits counter
	assert(m_iNumMBX>0 && m_iNumMBY>0);
// RRV modification
	m_numBitsVPMBnum = clog2 (m_iNumMBX * m_iNumMBY *m_iRRVScale *m_iRRVScale);	// number of bits for macroblock_number
//	m_numBitsVPMBnum = clog2 (m_iNumMBX * m_iNumMBY);	// number of bits for macroblock_number
// ~RRV
	m_numVideoPacket = 1;								// reset number of video packet in a VOP
	m_iVPMBnum = 0;									// start MB in the 1st VP
}


Void CVideoObjectEncoder::codeVideoPacketHeader (
	Int iMBX, Int iMBY, Int QuantScale
)
{
	m_iVPMBnum = VPMBnum(iMBX, iMBY);
	m_statsVOP.nBitsHead += codeVideoPacketHeader(QuantScale);
}

#if 0 // revised HEC for shape
Int CVideoObjectEncoder::codeVideoPacketHeader (Int QuantScale)
{
#ifdef __TRACE_AND_STATS_
	m_pbitstrmOut->trace ("Video_Packet_Header\n");
#endif // __TRACE_AND_STATS_
	UInt nBits = 0;
	assert(m_iVPMBnum > 0 && m_iVPMBnum < m_iNumMBX * m_iNumMBY);
	assert(QuantScale > 0 && QuantScale < 32);

	m_pbitstrmOut->flush ();
	Int nBitsResyncMarker = NUMBITS_VP_RESYNC_MARKER;
	if(m_volmd.bShapeOnly==FALSE)
	{
		if(m_vopmd.vopPredType == PVOP || (m_uiSprite == 2 && m_vopmd.vopPredType == SPRITE)) // GMC
			nBitsResyncMarker += (m_vopmd.mvInfoForward.uiFCode - 1);
		else if(m_vopmd.vopPredType == BVOP)
			nBitsResyncMarker += max(m_vopmd.mvInfoForward.uiFCode, m_vopmd.mvInfoBackward.uiFCode) - 1;
	}
	m_pbitstrmOut -> putBits (RESYNC_MARKER, nBitsResyncMarker, "resync_marker");
	m_pbitstrmOut -> putBits (m_iVPMBnum, m_numBitsVPMBnum, "VPH_macroblock_number");
	if(m_volmd.bShapeOnly==FALSE) {
		m_pbitstrmOut -> putBits (QuantScale, NUMBITS_VP_QUANTIZER, "VPH_quant_scale");
	}

	Bool	HEC = ((++m_numVideoPacket) == 2);	// use HEC in 2nd VP
	m_pbitstrmOut -> putBits (HEC, NUMBITS_VP_HEC, "VPH_header_extension_code");
#ifdef __TRACE_AND_STATS_
	nBits += nBitsResyncMarker + m_numBitsVPMBnum + NUMBITS_VP_QUANTIZER + NUMBITS_VP_HEC;
#endif // __TRACE_AND_STATS_
	if( HEC == TRUE ) {
		m_pbitstrmOut -> putBits ((Int) 0xFFFFFFFE, (UInt) m_nBitsModuloBase + 1, "VPH_Modulo_Time_Base");
		m_pbitstrmOut->putBits ((Int) 1, 1, "Marker"); // marker bit
		if(m_iNumBitsTimeIncr!=0)
			m_pbitstrmOut -> putBits (m_iVopTimeIncr, m_iNumBitsTimeIncr, "VPH_Time_Incr");
		m_pbitstrmOut->putBits ((Int) 1, 1, "Marker"); // marker bit
		m_pbitstrmOut -> putBits (m_vopmd.vopPredType, NUMBITS_VP_PRED_TYPE, "VPH_Pred_Type");
#ifdef __TRACE_AND_STATS_
		nBits += m_nBitsModuloBase+1 + m_iNumBitsTimeIncr +2 + NUMBITS_VP_PRED_TYPE + NUMBITS_VP_INTRA_DC_SWITCH_THR;
#endif // __TRACE_AND_STATS_
		if(m_volmd.bShapeOnly==FALSE) {
			m_pbitstrmOut -> putBits (m_vopmd.iIntraDcSwitchThr, NUMBITS_VP_INTRA_DC_SWITCH_THR, "VOP_DC_VLC_Switch");
// GMC
 			if ((m_uiSprite == 2) && (m_vopmd.vopPredType == SPRITE) && (m_iNumOfPnts>0))
#ifdef __TRACE_AND_STATS_
 				nBits += codeWarpPoints ();
#else
 				codeWarpPoints ();
#endif
// ~GMC
// RRV insertion
			if((m_volmd.breduced_resolution_vop_enable == 1)&&(m_volmd.fAUsage == RECTANGLE)&&((m_vopmd.vopPredType == PVOP)||(m_vopmd.vopPredType == IVOP)))
			{
				m_pbitstrmOut -> putBits (m_vopmd.RRVmode.iRRVOnOff, 1, "VOP_Reduced_Resolution");
#ifdef __TRACE_AND_STATS_
				nBits ++;
#endif // __TRACE_AND_STATS_
			}	
// ~RRV
			if(m_vopmd.vopPredType != IVOP) {
				m_pbitstrmOut -> putBits (m_vopmd.mvInfoForward.uiFCode, NUMBITS_VOP_FCODE, "VPH_Fcode_Forward");
#ifdef __TRACE_AND_STATS_
				nBits += NUMBITS_VOP_FCODE;
#endif // __TRACE_AND_STATS_
				if (m_vopmd.vopPredType == BVOP) {
					m_pbitstrmOut -> putBits (m_vopmd.mvInfoBackward.uiFCode, NUMBITS_VOP_FCODE, "VOP_Fcode_Backward");
#ifdef __TRACE_AND_STATS_
					nBits += NUMBITS_VOP_FCODE;
#endif // __TRACE_AND_STATS_
				}
			}
		}
	}

// NEWPRED
	if (m_volmd.bNewpredEnable) {
		g_pNewPredEnc->SetVPData(
				NP_VP_HEADER,
				&m_vopmd.m_iVopID,	
				&m_vopmd.m_iNumBitsVopID,
				&m_vopmd.m_iVopID4Prediction_Indication,
				&m_vopmd.m_iVopID4Prediction
		);
		m_pbitstrmOut -> putBits(m_vopmd.m_iVopID, m_vopmd.m_iNumBitsVopID, "VOP_Id");
		m_pbitstrmOut->putBits(m_vopmd.m_iVopID4Prediction_Indication,
					NUMBITS_VOP_ID_FOR_PREDICTION_INDICATION, "VOP_Id_For_Prediction_Indication");
		if( m_vopmd.m_iVopID4Prediction_Indication )
		{
			m_pbitstrmOut->putBits(m_vopmd.m_iVopID4Prediction, m_vopmd.m_iNumBitsVopID, "VOP_Id_For_Prediction");
#ifdef __TRACE_AND_STATS_
			nBits += m_vopmd.m_iNumBitsVopID;
#endif // __TRACE_AND_STATS_
		}
		m_pbitstrmOut->putBits(MARKER_BIT, MARKER_BIT, "Marker_Bit");
#ifdef __TRACE_AND_STATS_
		nBits += m_vopmd.m_iNumBitsVopID;
		nBits += NUMBITS_VOP_ID_FOR_PREDICTION_INDICATION;
		nBits += MARKER_BIT;
#endif // __TRACE_AND_STATS_
	
	}
// ~NEWPRED

	return nBits;
}
#else
Int CVideoObjectEncoder::codeVideoPacketHeader (Int QuantScale)
{
#ifdef __TRACE_AND_STATS_
	m_pbitstrmOut->trace ("Video_Packet_Header\n");
#endif // __TRACE_AND_STATS_
	UInt nBits = 0;
	assert(m_iVPMBnum > 0 && m_iVPMBnum < m_iNumMBX * m_iNumMBY);
	assert(QuantScale > 0 && QuantScale < 32);

	m_pbitstrmOut->flush ();
	Int nBitsResyncMarker = NUMBITS_VP_RESYNC_MARKER;
	Bool	HEC = ((++m_numVideoPacket) == 2);	// use HEC in 2nd VP
	if(m_volmd.bShapeOnly==FALSE)
	{
		if(m_vopmd.vopPredType == PVOP || (m_uiSprite == 2 && m_vopmd.vopPredType == SPRITE)) // GMC_V2
			nBitsResyncMarker += (m_vopmd.mvInfoForward.uiFCode - 1);
		else if(m_vopmd.vopPredType == BVOP)
			nBitsResyncMarker += max(m_vopmd.mvInfoForward.uiFCode, m_vopmd.mvInfoBackward.uiFCode) - 1;
	}
	m_pbitstrmOut -> putBits (RESYNC_MARKER, nBitsResyncMarker, "resync_marker");

	if(m_volmd.fAUsage != RECTANGLE) {
		m_pbitstrmOut -> putBits (HEC, NUMBITS_VP_HEC, "VPH_header_extension_code");
		if (HEC == TRUE && !(m_uiSprite == 1 && m_vopmd.vopPredType == IVOP)) {
			m_pbitstrmOut->putBits (m_rctCurrVOPY.width, NUMBITS_VOP_WIDTH, "VPH_VOP_Width");
			m_pbitstrmOut->putBits ((Int) 1, 1, "Marker"); // marker bit
			m_pbitstrmOut->putBits (m_rctCurrVOPY.height (), NUMBITS_VOP_HEIGHT, "VPH_VOP_Height");
			m_pbitstrmOut->putBits ((Int) 1, 1, "Marker"); // marker bit
#ifdef __TRACE_AND_STATS_
			nBits += NUMBITS_VOP_WIDTH + NUMBITS_VOP_HEIGHT + 2;
#endif // __TRACE_AND_STATS_

			if (m_rctCurrVOPY.left >= 0) {
				m_pbitstrmOut -> putBits (m_rctCurrVOPY.left, NUMBITS_VOP_HORIZONTAL_SPA_REF, "VPH_VOP_Hori_Ref");
			} else {
				m_pbitstrmOut -> putBits (m_rctCurrVOPY.left & 0x00001FFFF, NUMBITS_VOP_HORIZONTAL_SPA_REF, "VPH_VOP_Hori_Ref");
			}
			m_pbitstrmOut->putBits ((Int) 1, 1, "Marker"); // marker bit
			if (m_rctCurrVOPY.top >= 0) {
				m_pbitstrmOut -> putBits (m_rctCurrVOPY.top, NUMBITS_VOP_VERTICAL_SPA_REF, "VPH_VOP_Vert_Ref");
			} else {
				m_pbitstrmOut -> putBits (m_rctCurrVOPY.top & 0x00001FFFF, NUMBITS_VOP_VERTICAL_SPA_REF, "VPH_VOP_Vert_Ref");
			}
			m_pbitstrmOut->putBits ((Int) 1, 1, "Marker"); // marker bit
#ifdef __TRACE_AND_STATS_
			nBits += NUMBITS_VOP_HORIZONTAL_SPA_REF + NUMBITS_VOP_VERTICAL_SPA_REF + 2;
#endif // __TRACE_AND_STATS_
		}
	}

	m_pbitstrmOut -> putBits (m_iVPMBnum, m_numBitsVPMBnum, "VPH_macroblock_number");
	if(m_volmd.bShapeOnly==FALSE) {
		m_pbitstrmOut -> putBits (QuantScale, NUMBITS_VP_QUANTIZER, "VPH_quant_scale");
	}

	if(m_volmd.fAUsage == RECTANGLE)
		m_pbitstrmOut -> putBits (HEC, NUMBITS_VP_HEC, "VPH_header_extension_code");
#ifdef __TRACE_AND_STATS_
	nBits += nBitsResyncMarker + m_numBitsVPMBnum + NUMBITS_VP_QUANTIZER + NUMBITS_VP_HEC;
#endif // __TRACE_AND_STATS_
	if( HEC == TRUE ) {
		m_pbitstrmOut -> putBits ((Int) 0xFFFFFFFE, (UInt) m_nBitsModuloBase + 1, "VPH_Modulo_Time_Base");
		m_pbitstrmOut->putBits ((Int) 1, 1, "Marker"); // marker bit
		if(m_iNumBitsTimeIncr!=0)
			m_pbitstrmOut -> putBits (m_iVopTimeIncr, m_iNumBitsTimeIncr, "VPH_Time_Incr");
		m_pbitstrmOut->putBits ((Int) 1, 1, "Marker"); // marker bit
		m_pbitstrmOut -> putBits (m_vopmd.vopPredType, NUMBITS_VP_PRED_TYPE, "VPH_Pred_Type");
#ifdef __TRACE_AND_STATS_
		nBits += m_nBitsModuloBase+1 + m_iNumBitsTimeIncr +2 + NUMBITS_VP_PRED_TYPE + NUMBITS_VP_INTRA_DC_SWITCH_THR;
#endif // __TRACE_AND_STATS_

		if(m_volmd.fAUsage != RECTANGLE) {
			m_pbitstrmOut->putBits (m_volmd.bNoCrChange, 1, "VPH_VOP_CR_Change_Disable");
#ifdef __TRACE_AND_STATS_
			nBits ++;
#endif // __TRACE_AND_STATS_
			if (m_volmd.bShapeOnly==FALSE && m_vopmd.vopPredType != IVOP)  
			{  
				m_pbitstrmOut -> putBits (m_vopmd.bShapeCodingType, 1, "VPH_shape_coding_type");
#ifdef __TRACE_AND_STATS_
				nBits ++;
#endif // __TRACE_AND_STATS_
			}
		}
		if(m_volmd.bShapeOnly==FALSE) {
			m_pbitstrmOut -> putBits (m_vopmd.iIntraDcSwitchThr, NUMBITS_VP_INTRA_DC_SWITCH_THR, "VOP_DC_VLC_Switch");
// GMC_V2
			if ((m_uiSprite == 2) && (m_vopmd.vopPredType == SPRITE) && (m_iNumOfPnts>0))
#ifdef __TRACE_AND_STATS_
				nBits += codeWarpPoints ();
#else
				codeWarpPoints ();
#endif
// ~GMC_V2
// RRV insertion
			if((m_volmd.breduced_resolution_vop_enable == 1)&&(m_volmd.fAUsage == RECTANGLE)&&((m_vopmd.vopPredType == PVOP)||(m_vopmd.vopPredType == IVOP)))
			{
				m_pbitstrmOut -> putBits (m_vopmd.RRVmode.iRRVOnOff, 1, "VOP_Reduced_Resolution");
#ifdef __TRACE_AND_STATS_
				nBits ++;
#endif // __TRACE_AND_STATS_
			}	
// ~RRV
			if(m_vopmd.vopPredType != IVOP) {
				m_pbitstrmOut -> putBits (m_vopmd.mvInfoForward.uiFCode, NUMBITS_VOP_FCODE, "VPH_Fcode_Forward");
#ifdef __TRACE_AND_STATS_
				nBits += NUMBITS_VOP_FCODE;
#endif // __TRACE_AND_STATS_
				if (m_vopmd.vopPredType == BVOP) {
					m_pbitstrmOut -> putBits (m_vopmd.mvInfoBackward.uiFCode, NUMBITS_VOP_FCODE, "VOP_Fcode_Backward");
#ifdef __TRACE_AND_STATS_
					nBits += NUMBITS_VOP_FCODE;
#endif // __TRACE_AND_STATS_
				}
			}
		}
	}

// NEWPRED
	if (m_volmd.bNewpredEnable) {
		g_pNewPredEnc->SetVPData(
				NP_VP_HEADER,
				&m_vopmd.m_iVopID,	
				&m_vopmd.m_iNumBitsVopID,
				&m_vopmd.m_iVopID4Prediction_Indication,
				&m_vopmd.m_iVopID4Prediction
		);
		m_pbitstrmOut -> putBits(m_vopmd.m_iVopID, m_vopmd.m_iNumBitsVopID, "VOP_Id");
		m_pbitstrmOut->putBits(m_vopmd.m_iVopID4Prediction_Indication,
					NUMBITS_VOP_ID_FOR_PREDICTION_INDICATION, "VOP_Id_For_Prediction_Indication");
		if( m_vopmd.m_iVopID4Prediction_Indication )
		{
			m_pbitstrmOut->putBits(m_vopmd.m_iVopID4Prediction, m_vopmd.m_iNumBitsVopID, "VOP_Id_For_Prediction");
#ifdef __TRACE_AND_STATS_
			nBits += m_vopmd.m_iNumBitsVopID;
#endif // __TRACE_AND_STATS_
		}
		m_pbitstrmOut->putBits(MARKER_BIT, MARKER_BIT, "Marker_Bit");
#ifdef __TRACE_AND_STATS_
		nBits += m_vopmd.m_iNumBitsVopID;
		nBits += NUMBITS_VOP_ID_FOR_PREDICTION_INDICATION;
		nBits += MARKER_BIT;
#endif // __TRACE_AND_STATS_
	}
// ~NEWPRED

	return nBits;
}
#endif

UInt CVideoObjectEncoder::encodeMVVP (const CMotionVector* pmv, const CMBMode* pmbmd, Int iMBX, Int iMBY)
{
	Int iMBnum = iMBY * m_iNumMBX + iMBX;
	assert(iMBnum >= m_iVPMBnum);
	return encodeMV (pmv, pmbmd, bVPNoLeft(iMBnum, iMBX), bVPNoRightTop(iMBnum, iMBX), bVPNoTop(iMBnum),
		iMBX, iMBY);
}


//////////////////////////////////////////////////////////////////
////  The following functions are for Data partitioning mode  ////
//////////////////////////////////////////////////////////////////

Void CVideoObjectEncoder::encodeNSForIVOP_DP ()	
{
	assert( m_volmd.bDataPartitioning );
	assert( m_vopmd.vopPredType==IVOP );
	memset (m_rgmv, 0, m_iNumMB * PVOP_MV_PER_REF_PER_MB * sizeof (CMotionVector));

	CMBMode* pmbmd = m_rgmbmd;
	Int iQPPrev = m_vopmd.intStepI;	//initialization
	PixelC* ppxlcRefY  = (PixelC*) m_pvopcRefQ1->pixelsY () + m_iStartInRefToCurrRctY;
	PixelC* ppxlcRefU  = (PixelC*) m_pvopcRefQ1->pixelsU () + m_iStartInRefToCurrRctUV;
	PixelC* ppxlcRefV  = (PixelC*) m_pvopcRefQ1->pixelsV () + m_iStartInRefToCurrRctUV;
	
	PixelC* ppxlcOrigY = (PixelC*) m_pvopcOrig->pixelsBoundY ();
	PixelC* ppxlcOrigU = (PixelC*) m_pvopcOrig->pixelsBoundU ();
	PixelC* ppxlcOrigV = (PixelC*) m_pvopcOrig->pixelsBoundV ();

	Int iMBX, iMBY, iMB = 0;
    Int iCurrMBPos;
	Int iVideoPacketNumber	= 0;
	Int iVPCounter = m_statsVOP.total();
	Int iVPtotal;
	m_iVPMBnum = 0;
	CStatistics m_statsVP(0);
	// DCT coefficient buffer for Data Partitioning mode
	Int*** iCoefQ_DP = new Int** [m_iNumMB];

	// Set not to output but count bitstream
	m_pbitstrmOut->SetDontSendBits(TRUE);

	Bool bRestartDelayedQP = TRUE;

	for (iMBY = 0; iMBY < m_iNumMBY; iMBY++) {
		PixelC* ppxlcRefMBY  = ppxlcRefY;
		PixelC* ppxlcRefMBU  = ppxlcRefU;
		PixelC* ppxlcRefMBV  = ppxlcRefV;
		PixelC* ppxlcOrigMBY = ppxlcOrigY;
		PixelC* ppxlcOrigMBU = ppxlcOrigU;
		PixelC* ppxlcOrigMBV = ppxlcOrigV;
		for (iMBX = 0; iMBX < m_iNumMBX; iMBX++, iMB++) {
#ifdef __TRACE_AND_STATS_
			m_pbitstrmOut->trace (CSite (iMBX, iMBY), "MB_X_Y");
#endif // __TRACE_AND_STATS_

			m_statsMB.reset ();
			pmbmd->m_bFieldMV = 0;
			pmbmd->m_dctMd = INTRA;
			pmbmd->m_bSkip = FALSE;			//reset for direct mode
			pmbmd->m_bMCSEL = FALSE;			//reset for direct mode
			pmbmd->m_iVideoPacketNumber	= iVideoPacketNumber;

			copyToCurrBuff (ppxlcOrigMBY, ppxlcOrigMBU, ppxlcOrigMBV, m_iFrameWidthY, m_iFrameWidthUV); 
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

			// MB level rate control section
			// here is where we calculate the delta QP
			if (m_uiRateControl>=RC_TM5) {
				// TM5 rate control
				updateQP(pmbmd, iQPPrev, m_tm5rc.tm5rc_calc_mquant(iMB, m_statsVOP.total())	);
			}
			else
			{
#ifdef _MBQP_CHANGE_
				Int iDQuant = (rand() % 5) - 2;
				updateQP(pmbmd, iQPPrev, iQPPrev + iDQuant);
#else
				// no change in step size, but still need to call...
				updateQP(pmbmd, iQPPrev, iQPPrev);
#endif //_MBQP_CHANGE_
			}

			// call this since we know new qp for certain
			setDCVLCMode(pmbmd, &bRestartDelayedQP);

			quantizeTextureIntraMB (iMBX, iMBY, pmbmd, ppxlcRefMBY, ppxlcRefMBU, ppxlcRefMBV, NULL);

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

			codeMBTextureHeadOfIVOP (pmbmd);
			sendDCTCoefOfIntraMBTexture (pmbmd);

			Int iVPlastMBnum = iMBY * m_iNumMBX + iMBX;

			// copy DCT coefficient to buffer
			iCoefQ_DP[iVPlastMBnum] = new Int* [6];
			Int iBlk;
			for (iBlk = 0; iBlk < 6; iBlk++) {
				iCoefQ_DP [iVPlastMBnum] [iBlk] = new Int [BLOCK_SQUARE_SIZE];

				for( Int t = 0; t < BLOCK_SQUARE_SIZE; t++ )
					iCoefQ_DP[iVPlastMBnum][iBlk][t] = m_rgpiCoefQ[iBlk][t];
			}

			// update qp
			iQPPrev = pmbmd->m_stepSize;

			pmbmd++;
#ifdef __TRACE_AND_STATS_
			m_statsVOP   += m_statsMB;
#endif // __TRACE_AND_STATS_

			ppxlcRefMBY  += (MB_SIZE *m_iRRVScale);
			ppxlcRefMBU  += (BLOCK_SIZE *m_iRRVScale);
			ppxlcRefMBV  += (BLOCK_SIZE *m_iRRVScale);
			ppxlcOrigMBY += (MB_SIZE *m_iRRVScale);
			ppxlcOrigMBU += (BLOCK_SIZE *m_iRRVScale);
			ppxlcOrigMBV += (BLOCK_SIZE *m_iRRVScale);


			iVPtotal = (int) m_statsVOP.total() - iVPCounter;
			iCurrMBPos	= (m_iRRVScale == 2) ? (2 *iMBX +1 +(2 *iMBY +1) *(2 *m_iNumMBX)) : ((iMBY *m_iNumMBX) +iMBX);

			if( (m_volmd.bNewpredEnable && m_volmd.bNewpredSegmentType == 0) ?
				( iCurrMBPos == g_pNewPredEnc->SliceTailMBA((iMBX*m_iRRVScale), (iMBY*m_iRRVScale)) )
				: ( iVPtotal > m_volmd.bVPBitTh || iVPlastMBnum == m_iNumMB-1) )
			{

				// Set to output bitstream
				m_pbitstrmOut->SetDontSendBits(FALSE);
				// encode video packet
				iVPCounter = m_statsVOP.total();
				m_statsVP.reset();
// RRV_2 insertion
				iVideoPacketNumber ++;
// ~RRV_2
				if( m_iVPMBnum > 0 )
				{
					m_statsVP.nBitsHead 
						= codeVideoPacketHeader (m_rgmbmd[m_iVPMBnum].m_stepSize - m_rgmbmd[m_iVPMBnum].m_intStepDelta);
					//printf("VP");
				}

				// needs to be set even if we dont code the first vp because we are currently
				// encoding after the second vp
				bRestartDelayedQP = TRUE;

				DataPartitioningMotionCoding(m_iVPMBnum, iVPlastMBnum, &m_statsVP, iCoefQ_DP);

				m_pbitstrmOut -> putBits (DC_MARKER, NUMBITS_DP_DC_MARKER, "DC_marker");
				m_statsVP.nBitsHead += NUMBITS_DP_DC_MARKER;

				DataPartitioningTextureCoding(m_iVPMBnum, iVPlastMBnum, &m_statsVP, iCoefQ_DP);

				//assert( iVPtotal + m_statsVP.nBitsHead == (int) m_statsVP.total() );

				m_iVPMBnum = iVPlastMBnum + 1;

				// Set not to output but count bitstream
				m_pbitstrmOut->SetDontSendBits(TRUE);
			}
		}
		
		MacroBlockMemory** ppmbmTemp = m_rgpmbmAbove;
		m_rgpmbmAbove = m_rgpmbmCurr;
		m_rgpmbmCurr  = ppmbmTemp;
		ppxlcRefY  += m_iFrameWidthYxMBSize;
		ppxlcRefU  += m_iFrameWidthUVxBlkSize;
		ppxlcRefV  += m_iFrameWidthUVxBlkSize;
		
		ppxlcOrigY += m_iFrameWidthYxMBSize;
		ppxlcOrigU += m_iFrameWidthUVxBlkSize;
		ppxlcOrigV += m_iFrameWidthUVxBlkSize;
	}

// RRV insertion
	if(m_vopmd.RRVmode.iRRVOnOff == 1)
	{
          ppxlcRefY	= (PixelC*) m_pvopcRefQ1->pixelsY () 
			  + m_iStartInRefToCurrRctY;
          ppxlcRefU = (PixelC*) m_pvopcRefQ1->pixelsU ()
			  + m_iStartInRefToCurrRctUV;
          ppxlcRefV = (PixelC*) m_pvopcRefQ1->pixelsV ()
			  + m_iStartInRefToCurrRctUV;
          filterCodedPictureForRRV(ppxlcRefY, ppxlcRefU, ppxlcRefV,
								   m_ivolWidth, m_ivolHeight,
								   m_iNumMBX,
								   m_iNumMBY,
								   m_iFrameWidthY, m_iFrameWidthUV);
      }
// ~RRV
	// delete CoefQ_DP
	for(iMB = 0; iMB < m_iNumMB; iMB++ )  {
		for (Int iBlk = 0; iBlk < 6; iBlk++) {
			delete [] iCoefQ_DP [iMB] [iBlk];
		}
		delete [] iCoefQ_DP[iMB];
	}
	delete [] iCoefQ_DP;

	// Set to output bitstream
	m_pbitstrmOut->SetDontSendBits(FALSE);
}

Void CVideoObjectEncoder::encodeNSForPVOP_DP ()	
{
	assert( m_volmd.bDataPartitioning );
	assert( m_vopmd.vopPredType==PVOP || (m_uiSprite == 2 && m_vopmd.vopPredType==SPRITE) ); // GMC
	//assert(m_volmd.nBits==8);

// RRV insertion
//	CMotionVector pmv_RRV[m_iNumMB *PVOP_MV_PER_REF_PER_MB];
	CMotionVector *pmv_RRV	= new CMotionVector[m_iNumMB *PVOP_MV_PER_REF_PER_MB];
// ~RRV
	if (m_uiSprite == 0 || m_uiSprite == 2) // GMC
			motionEstPVOP ();

// RRV insertion
	if(m_vopmd.RRVmode.iRRVOnOff == 1)
	{
		for(Int i = 0; i < (m_iNumMB *PVOP_MV_PER_REF_PER_MB); i ++)
		{
			pmv_RRV[i]	= m_rgmv[i];
		}
		MotionVectorScalingUp(pmv_RRV, m_iNumMB, PVOP_MV_PER_REF_PER_MB);

		for(Int j = 0; j < (m_iNumMB *PVOP_MV_PER_REF_PER_MB); j ++)
		{
			m_rgmv[j].m_vctTrueHalfPel_x2.x	= pmv_RRV[j].m_vctTrueHalfPel.x;
			m_rgmv[j].m_vctTrueHalfPel_x2.y = pmv_RRV[j].m_vctTrueHalfPel.y;
			pmv_RRV[j].m_vctTrueHalfPel_x2.x= pmv_RRV[j].m_vctTrueHalfPel.x;
			pmv_RRV[j].m_vctTrueHalfPel_x2.y= pmv_RRV[j].m_vctTrueHalfPel.y;
		}
	}
// ~RRV
// RRV_2 insertion
	Int iVideoPacketNumber	= 0;
// ~RRV_2	

	UInt newQStep = m_vopmd.intStep; // for frame based rate control
	// vopmd.intStep is updated at bottom of this function

	// Rate Control
	if (m_uiRateControl==RC_MPEG4) {
		Double Ec = m_iMAD / (Double) (m_iNumMBY * m_iNumMBX * 16 * 16 * m_iRRVScale *m_iRRVScale);
		m_statRC.setMad (Ec);
		// calculate for next frame (should be this frame, but its too late to send vop qp!)
		newQStep = m_statRC.updateQuanStepsize (m_vopmd.intStep);
		// this is not the correct way to use rate control
		m_statRC.setQc (m_vopmd.intStep);
	}


	Int iMBX, iMBY;
// RRV insertion
    Int iCurrMBPos;
// ~RRV	
	CoordI y = 0; 
	CMBMode* pmbmd = m_rgmbmd;
	Int iQPPrev = m_vopmd.intStep;
	CMotionVector* pmv = m_rgmv;
// RRV insertion
	CMotionVector* plmv_RRV = pmv_RRV;
// ~RRV
	PixelC* ppxlcRefY = (PixelC*) m_pvopcRefQ1->pixelsY () + m_iStartInRefToCurrRctY;
	PixelC* ppxlcRefU = (PixelC*) m_pvopcRefQ1->pixelsU () + m_iStartInRefToCurrRctUV;
	PixelC* ppxlcRefV = (PixelC*) m_pvopcRefQ1->pixelsV () + m_iStartInRefToCurrRctUV;
	
	PixelC* ppxlcOrigY = (PixelC*) m_pvopcOrig->pixelsBoundY ();
	PixelC* ppxlcOrigU = (PixelC*) m_pvopcOrig->pixelsBoundU ();
	PixelC* ppxlcOrigV = (PixelC*) m_pvopcOrig->pixelsBoundV ();
		
// NEWPRED
	const PixelC* RefbufY = (PixelC*)m_pvopcRefQ0-> pixelsY ();
	const PixelC* RefbufU = (PixelC*)m_pvopcRefQ0-> pixelsU ();
	const PixelC* RefbufV = (PixelC*)m_pvopcRefQ0-> pixelsV ();

	if (m_volmd.bNewpredEnable) {
		g_pNewPredEnc->CopyReftoBuf(RefbufY, RefbufU, RefbufV, m_rctRefFrameY, m_rctRefFrameUV);
		for (iMBY = 0; iMBY < m_iNumMBY; iMBY++) {
			for (iMBX = 0; iMBX < m_iNumMBX; iMBX++)	{
// RRV modification
				(pmbmd + iMBX + m_iNumMBX*iMBY) -> m_iNPSegmentNumber = g_pNewPredEnc->GetSliceNum((iMBX *m_iRRVScale),(iMBY *m_iRRVScale));
//				(pmbmd + iMBX + m_iNumMBX*iMBY) -> m_iNPSegmentNumber = g_pNewPredEnc->GetSliceNum(iMBX,iMBY);
// ~RRV
			}
		}
	}
// ~NEWPRED
	
	Int iVPCounter = m_statsVOP.total();
	Int iVPtotal;
	m_iVPMBnum = 0;
	CStatistics m_statsVP(0);
	// DCT coefficient buffer for Data Partitioning mode
	Int*** iCoefQ_DP = new Int** [m_iNumMB];

	// Set not to output but count bitstream
	m_pbitstrmOut->SetDontSendBits(TRUE);

	Bool bRestartDelayedQP = TRUE;

	Int iMB = 0;

// RRV modification
	for (iMBY = 0; iMBY < m_iNumMBY; iMBY++, y += (MB_SIZE *m_iRRVScale)) {
//	for (iMBY = 0; iMBY < m_iNumMBY; iMBY++, y += MB_SIZE) {
// ~RRV
		PixelC* ppxlcRefMBY = ppxlcRefY;
		PixelC* ppxlcRefMBU = ppxlcRefU;
		PixelC* ppxlcRefMBV = ppxlcRefV;
		PixelC* ppxlcOrigMBY = ppxlcOrigY;
		PixelC* ppxlcOrigMBU = ppxlcOrigU;
		PixelC* ppxlcOrigMBV = ppxlcOrigV;
		CoordI x = 0; 

		for (iMBX = 0; iMBX < m_iNumMBX; iMBX++, x += (MB_SIZE *m_iRRVScale), iMB++)
		{
			if((m_volmd.bNewpredEnable) && g_pNewPredEnc->CheckSlice((iMBX *m_iRRVScale),(iMBY *m_iRRVScale))){
				PixelC* RefpointY = (PixelC*) m_pvopcRefQ0->pixelsY () + m_iStartInRefToCurrRctY + iMBY * (MB_SIZE *m_iRRVScale) * m_rctRefFrameY.width;
				PixelC* RefpointU = (PixelC*) m_pvopcRefQ0->pixelsU () + m_iStartInRefToCurrRctUV + iMBY * (BLOCK_SIZE *m_iRRVScale) * m_rctRefFrameUV.width;
				PixelC* RefpointV = (PixelC*) m_pvopcRefQ0->pixelsV () + m_iStartInRefToCurrRctUV + iMBY * (BLOCK_SIZE *m_iRRVScale) * m_rctRefFrameUV.width;
				g_pNewPredEnc->ChangeRefOfSlice((const PixelC* )RefpointY, RefbufY,(const PixelC* ) RefpointU, RefbufU,
					(const PixelC* )RefpointV, RefbufV, (iMBX *m_iRRVScale), (iMBY *m_iRRVScale),m_rctRefFrameY, m_rctRefFrameUV);
				m_rctRefVOPZoom0 = m_rctRefVOPY0.upSampleBy2 ();
				biInterpolateY (m_pvopcRefQ0, m_rctRefVOPY0, m_puciRefQZoom0, m_rctRefVOPZoom0, m_vopmd.iRoundingControl);
			}

			m_statsMB.reset ();
			pmbmd->m_iVideoPacketNumber	= iVideoPacketNumber;


			// MB level rate control section
			// here is where we calculate the delta QP
			if (m_uiRateControl>=RC_TM5) {
				// TM5 rate control
				updateQP(pmbmd, iQPPrev, m_tm5rc.tm5rc_calc_mquant(iMB, m_statsVOP.total())	);
			}
			else
			{
#ifdef _MBQP_CHANGE_
				Int iDQuant = (rand() % 5) - 2;
				updateQP(pmbmd, iQPPrev, iQPPrev + iDQuant);
#else
				// no change in step size, but still need to call...
				updateQP(pmbmd, iQPPrev, iQPPrev);
#endif //_MBQP_CHANGE_
			}
			

			copyToCurrBuff (ppxlcOrigMBY, ppxlcOrigMBU, ppxlcOrigMBV, m_iFrameWidthY, m_iFrameWidthUV); 
			encodePVOPMB (
				ppxlcRefMBY, ppxlcRefMBU, ppxlcRefMBV,
				pmbmd, pmv, plmv_RRV, 
				iMBX, iMBY,
				x, y, &bRestartDelayedQP
			);

			Int iVPlastMBnum = iMBY * m_iNumMBX + iMBX;

			// copy DCT coefficient to buffer
			iCoefQ_DP[iVPlastMBnum] = new Int* [6];
			Int iBlk;
			for (iBlk = 0; iBlk < 6; iBlk++) {
				iCoefQ_DP [iVPlastMBnum] [iBlk] = new Int [BLOCK_SQUARE_SIZE];

				for( Int t = 0; t < BLOCK_SQUARE_SIZE; t++ )
					iCoefQ_DP[iVPlastMBnum][iBlk][t] = m_rgpiCoefQ[iBlk][t];
			}

			iQPPrev = pmbmd->m_stepSize;

			pmbmd++;
			pmv += PVOP_MV_PER_REF_PER_MB;

			if (m_vopmd.RRVmode.iRRVOnOff == 1)
			{
				plmv_RRV	+= PVOP_MV_PER_REF_PER_MB;
			}

#ifdef __TRACE_AND_STATS_
			m_statsVOP += m_statsMB;
#endif // __TRACE_AND_STATS_

			ppxlcRefMBY  += (MB_SIZE *m_iRRVScale);
			ppxlcRefMBU  += (BLOCK_SIZE *m_iRRVScale);
			ppxlcRefMBV  += (BLOCK_SIZE *m_iRRVScale);
			ppxlcOrigMBY += (MB_SIZE *m_iRRVScale);
			ppxlcOrigMBU += (BLOCK_SIZE *m_iRRVScale);
			ppxlcOrigMBV += (BLOCK_SIZE *m_iRRVScale);

			iVPtotal = (int) m_statsVOP.total() - iVPCounter;

			iCurrMBPos	= (m_iRRVScale == 2) ? (2 *iMBX +1 +(2 *iMBY +1) *(2 *m_iNumMBX)) : ((iMBY *m_iNumMBX) +iMBX);

			if( ((m_volmd.bNewpredEnable) && (m_volmd.bNewpredSegmentType == 0)) ?
				(iCurrMBPos == g_pNewPredEnc->SliceTailMBA((iMBX *m_iRRVScale), (iMBY *m_iRRVScale))) : (iVPtotal>m_volmd.bVPBitTh || iVPlastMBnum==m_iNumMB-1))
			{
				// Set to output bitstream
				m_pbitstrmOut->SetDontSendBits(FALSE);
				// encode video packet
				iVPCounter = m_statsVOP.total();
				m_statsVP.reset();

				iVideoPacketNumber ++;

				if( m_iVPMBnum > 0 )
				{
					m_statsVP.nBitsHead = codeVideoPacketHeader (m_rgmbmd[m_iVPMBnum].m_stepSize - m_rgmbmd[m_iVPMBnum].m_intStepDelta);	
				}

				bRestartDelayedQP = TRUE;

				DataPartitioningMotionCoding(m_iVPMBnum, iVPlastMBnum, &m_statsVP, iCoefQ_DP);

				m_pbitstrmOut -> putBits (MOTION_MARKER, NUMBITS_DP_MOTION_MARKER, "motion_marker");
				m_statsVP.nBitsHead += NUMBITS_DP_MOTION_MARKER;

				DataPartitioningTextureCoding(m_iVPMBnum, iVPlastMBnum, &m_statsVP, iCoefQ_DP);

				m_iVPMBnum = iVPlastMBnum + 1;

				// Set not to output but count bitstream
				m_pbitstrmOut->SetDontSendBits(TRUE);
			}

		}
		MacroBlockMemory** ppmbmTemp = m_rgpmbmAbove;
		m_rgpmbmAbove = m_rgpmbmCurr;
		m_rgpmbmCurr  = ppmbmTemp;

		ppxlcRefY += m_iFrameWidthYxMBSize;
		ppxlcRefU += m_iFrameWidthUVxBlkSize;
		ppxlcRefV += m_iFrameWidthUVxBlkSize;
		
		ppxlcOrigY += m_iFrameWidthYxMBSize;
		ppxlcOrigU += m_iFrameWidthUVxBlkSize;
		ppxlcOrigV += m_iFrameWidthUVxBlkSize;
	}
// RRV insertion
	if(m_vopmd.RRVmode.iRRVOnOff == 1)
	{
          ppxlcRefY	= (PixelC*) m_pvopcRefQ1->pixelsY () 
			  + m_iStartInRefToCurrRctY;
          ppxlcRefU = (PixelC*) m_pvopcRefQ1->pixelsU ()
			  + m_iStartInRefToCurrRctUV;
          ppxlcRefV = (PixelC*) m_pvopcRefQ1->pixelsV ()
			  + m_iStartInRefToCurrRctUV;
          filterCodedPictureForRRV(ppxlcRefY, ppxlcRefU, ppxlcRefV,
								   m_ivolWidth, m_ivolHeight,
								   m_iNumMBX,
								   m_iNumMBY,
								   m_iFrameWidthY, m_iFrameWidthUV);
      }
// ~RRV
	// delete CoefQ_DP
	for(iMB = 0; iMB < m_iNumMB; iMB++ )  {
		for (Int iBlk = 0; iBlk < 6; iBlk++) {
			delete [] iCoefQ_DP [iMB] [iBlk];
		}
		delete [] iCoefQ_DP[iMB];
	}
	delete [] iCoefQ_DP;

	// Set to output bitstream
	m_pbitstrmOut->SetDontSendBits(FALSE);

// NEWPRED
	// copy previous picture to reference picture memory because of output ordering
	if(m_volmd.bNewpredEnable) {
		for( int iSlice = 0; iSlice < g_pNewPredEnc->m_iNumSlice; iSlice++ ) {
			int		iMBY = g_pNewPredEnc->NowMBA(iSlice)/((g_pNewPredEnc->getwidth())/MB_SIZE);
			PixelC* RefpointY = (PixelC*) m_pvopcRefQ0->pixelsY () + (m_iStartInRefToCurrRctY-EXPANDY_REF_FRAME) + iMBY * MB_SIZE * m_rctRefFrameY.width;
			PixelC* RefpointU = (PixelC*) m_pvopcRefQ0->pixelsU () + (m_iStartInRefToCurrRctUV-EXPANDUV_REF_FRAME) + iMBY * BLOCK_SIZE * m_rctRefFrameUV.width;
			PixelC* RefpointV = (PixelC*) m_pvopcRefQ0->pixelsV () + (m_iStartInRefToCurrRctUV-EXPANDUV_REF_FRAME) + iMBY * BLOCK_SIZE * m_rctRefFrameUV.width;
			g_pNewPredEnc->CopyNPtoPrev(iSlice, RefpointY, RefpointU, RefpointV);	
		}
		repeatPadYOrA ((PixelC*) m_pvopcRefQ0->pixelsY () + m_iOffsetForPadY, m_pvopcRefQ0);
		repeatPadUV (m_pvopcRefQ0);
	}
// ~NEWPRED

	m_vopmd.intStep = newQStep;
}


Void CVideoObjectEncoder::DataPartitioningMotionCoding(Int iVPMBnum, Int iVPlastMBnum, CStatistics* m_statsVP, Int*** iCoefQ_DP)
{
	assert( m_volmd.bDataPartitioning );

	Int	iMBnum;
	CMBMode* pmbmd;
	CMotionVector* pmv = m_rgmv;
	Int iMBX, iMBY;

	for(iMBnum = iVPMBnum, pmbmd = m_rgmbmd+iVPMBnum, pmv = m_rgmv+iVPMBnum*PVOP_MV_PER_REF_PER_MB;
		iMBnum <= iVPlastMBnum; iMBnum++, pmbmd++, pmv+=PVOP_MV_PER_REF_PER_MB) {
		iMBX = iMBnum % m_iNumMBX;
		iMBY = iMBnum / m_iNumMBX;
#ifdef __TRACE_AND_STATS_
		m_pbitstrmOut->trace (CSite (iMBX, iMBY), "Motion_MB_X_Y");
#endif // __TRACE_AND_STATS_
		if( m_volmd.fAUsage != RECTANGLE ) {
			m_statsVP->nBitsShape += dumpCachedShapeBits_DP(iMBnum);
		}
		
		if (pmbmd -> m_rgTranspStatus [0] != ALL) {
			if( m_vopmd.vopPredType==PVOP || (m_uiSprite == 2 && m_vopmd.vopPredType == SPRITE)) { // GMC
				m_pbitstrmOut->putBits (pmbmd->m_bSkip, 1, "MB_Skip");
				m_statsVP->nBitsCOD++;
// GMC
			if(m_uiSprite == 2 && m_vopmd.vopPredType == SPRITE && pmbmd->m_bSkip == TRUE)
				m_statsVP->nMCSELMB++;
// ~GMC
			}

			//if(pmbmd->m_bSkip)
			//	printf("(Skip)");

			if (!pmbmd->m_bSkip) {
				UInt CBPC = (pmbmd->getCodedBlockPattern (U_BLOCK) << 1)
							| pmbmd->getCodedBlockPattern (V_BLOCK);	
														//per defintion of H.263's CBPC 
				assert (CBPC >= 0 && CBPC <= 3);

				Int iMBtype;								//per H.263's MBtype
				Int iSymbol;
				switch( m_vopmd.vopPredType ) {
				case PVOP:
					if (pmbmd->m_dctMd == INTRA || pmbmd->m_dctMd == INTRAQ)
						iMBtype = pmbmd->m_dctMd + 3;
					else
						iMBtype = (pmbmd -> m_dctMd - 2) | pmbmd -> m_bhas4MVForward << 1;
					assert (iMBtype >= 0 && iMBtype <= 4);
#ifdef __TRACE_AND_STATS_
					m_pbitstrmOut->trace (iMBtype, "MB_MBtype");
					m_pbitstrmOut->trace (CBPC, "MB_CBPC");
#endif // __TRACE_AND_STATS_
					iSymbol = iMBtype * 4 + CBPC;
					m_statsVP->nBitsMCBPC += m_pentrencSet->m_pentrencMCBPCinter->encodeSymbol (iSymbol, "MCBPC");
					//printf("(%d:%d:%d)", iSymbol, pmbmd->m_bCodeDcAsAc, pmbmd->m_stepSize);
					break;
// GMC
				case SPRITE:
					if (pmbmd->m_dctMd == INTRA || pmbmd->m_dctMd == INTRAQ)
						iMBtype = pmbmd->m_dctMd + 3;
					else
						iMBtype = (pmbmd -> m_dctMd - 2) | pmbmd -> m_bhas4MVForward << 1;
					assert (iMBtype >= 0 && iMBtype <= 4);
#ifdef __TRACE_AND_STATS_
					m_pbitstrmOut->trace (iMBtype, "MB_MBtype");
					m_pbitstrmOut->trace (CBPC, "MB_CBPC");
#endif // __TRACE_AND_STATS_
					m_statsVP->nBitsMCBPC += m_pentrencSet->m_pentrencMCBPCinter->encodeSymbol (iMBtype * 4 + CBPC, "MCBPC");
					if((pmbmd->m_dctMd == INTER || pmbmd->m_dctMd == INTERQ) && (pmbmd -> m_bhas4MVForward == FALSE)){
						m_pbitstrmOut->putBits (pmbmd->m_bMCSEL, 1, "MCSEL");
						m_statsVP->nMCSELMB ++;
						m_statsVP->nBitsMCSEL ++;
					}
					break;
// ~GMC
				case IVOP:
					iSymbol = 4 * pmbmd->m_dctMd + CBPC;
					assert (iSymbol >= 0 && iSymbol <= 7);			//send MCBPC
#ifdef __TRACE_AND_STATS_
					m_pbitstrmOut->trace (CBPC, "MB_CBPC");
#endif // __TRACE_AND_STATS_
					m_statsVP->nBitsMCBPC += m_pentrencSet->m_pentrencMCBPCintra->encodeSymbol (iSymbol, "MB_MCBPC");
					//printf("(%d:%d:%d)", iSymbol, pmbmd->m_bCodeDcAsAc, pmbmd->m_stepSize);
					break;
				default:
					assert(FALSE);
				}

				if ( m_vopmd.vopPredType == IVOP ) {
					if( pmbmd->m_dctMd == INTRAQ) {
						Int DQUANT = pmbmd->m_intStepDelta;			//send DQUANT
						assert (DQUANT >= -2 && DQUANT <= 2);
						if (DQUANT != 0) {	
							if (sign (DQUANT) == 1)
								m_pbitstrmOut->putBits (DQUANT + 1, 2, "MB_DQUANT");
							else
								m_pbitstrmOut->putBits (-1 - DQUANT, 2, "MB_DQUANT");
							m_statsVP->nBitsDQUANT += 2;
						}
					}				 
		
					UInt iBlk = 0;
					for (iBlk = Y_BLOCK1; iBlk <= V_BLOCK; iBlk++) {
						UInt nBits = 0;
#ifdef __TRACE_AND_STATS_
						m_pbitstrmOut->trace (iBlk, "BLK_NO");
#endif // __TRACE_AND_STATS_
						if (iBlk < U_BLOCK)
							if (pmbmd -> m_rgTranspStatus [iBlk] == ALL) continue;
						Int* rgiCoefQ = iCoefQ_DP [iMBnum][iBlk - 1];
#ifdef __TRACE_AND_STATS_
						m_pbitstrmOut->trace (rgiCoefQ[0], "IntraDC");
#endif // __TRACE_AND_STATS_
						if (pmbmd->m_bCodeDcAsAc != TRUE)	{
							nBits = sendIntraDC (rgiCoefQ, iBlk);
						}
						switch (iBlk) {
						case U_BLOCK: 
							m_statsVP->nBitsCr += nBits;
							break;
						case V_BLOCK: 
							m_statsVP->nBitsCb += nBits;
							break;
						default:
							m_statsVP->nBitsY += nBits;
						}
					}
				}

				if (pmbmd->m_dctMd != INTRA && pmbmd->m_dctMd != INTRAQ && !(m_uiSprite == 2 && m_vopmd.vopPredType == SPRITE && pmbmd -> m_bMCSEL)) { // GMC
					if( m_volmd.fAUsage == RECTANGLE )
						m_statsVP->nBitsMV += encodeMVVP (pmv, pmbmd, iMBX, iMBY);
					else
						m_statsVP->nBitsMV += encodeMVWithShape (pmv, pmbmd, iMBX, iMBY);
				}
			}
		}
	}
}

Void CVideoObjectEncoder::DataPartitioningTextureCoding(Int iVPMBnum, Int iVPlastMBnum, CStatistics* m_statsVP, Int*** iCoefQ_DP, Int*** iRowLength_DP)
{
	assert( m_volmd.bDataPartitioning );

	Int	iMBnum;
	CMBMode* pmbmd;
	CMotionVector* pmv = m_rgmv;
	Int iMBX, iMBY;

	for(iMBnum = iVPMBnum, pmbmd = m_rgmbmd+iVPMBnum, pmv = m_rgmv+iVPMBnum*PVOP_MV_PER_REF_PER_MB;
			iMBnum <= iVPlastMBnum; iMBnum++, pmbmd++, pmv+=PVOP_MV_PER_REF_PER_MB) {
		if (pmbmd->m_bSkip || pmbmd -> m_rgTranspStatus [0] == ALL)
			continue;

		iMBX = iMBnum % m_iNumMBX;
		iMBY = iMBnum / m_iNumMBX;
#ifdef __TRACE_AND_STATS_
		m_pbitstrmOut->trace (CSite (iMBX, iMBY), "TextureHeader_MB_X_Y");
#endif // __TRACE_AND_STATS_
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
		if (m_volmd.fAUsage == RECTANGLE)
			assert (cNonTrnspBlk==4); // Only all opaque is only supportedin DP mode at present

		if (pmbmd->m_dctMd == INTRA || pmbmd->m_dctMd == INTRAQ)	{
			m_pbitstrmOut->putBits (pmbmd->m_bACPrediction, 1, "MB_ACPRED");
			m_statsVP->nBitsIntraPred++;
#ifdef __TRACE_AND_STATS_
			m_pbitstrmOut->trace (cNonTrnspBlk, "MB_NumNonTranspBlks");
			m_pbitstrmOut->trace (CBPY, "MB_CBPY (I-style)");
#endif // __TRACE_AND_STATS_
			switch (cNonTrnspBlk) {
			case 1:
				m_statsVP->nBitsCBPY += m_pentrencSet->m_pentrencCBPY1->encodeSymbol (CBPY, "MB_CBPY1");
				break;
			case 2:
				m_statsVP->nBitsCBPY += m_pentrencSet->m_pentrencCBPY2->encodeSymbol (CBPY, "MB_CBPY2");
				break;
			case 3:
				m_statsVP->nBitsCBPY += m_pentrencSet->m_pentrencCBPY3->encodeSymbol (CBPY, "MB_CBPY3");
				break;
			case 4:
				m_statsVP->nBitsCBPY += m_pentrencSet->m_pentrencCBPY->encodeSymbol (CBPY, "MB_CBPY");
				break;
			default:
				assert (FALSE);
			}
			m_statsVP->nIntraMB++;
		}
		else {
#ifdef __TRACE_AND_STATS_
			m_pbitstrmOut->trace (cNonTrnspBlk, "MB_NumNonTranspBlks");
			m_pbitstrmOut->trace (CBPY, "MB_CBPY (P-style)");
#endif // __TRACE_AND_STATS_
			switch (cNonTrnspBlk) {
			case 1:
				m_statsVP->nBitsCBPY += m_pentrencSet->m_pentrencCBPY1->encodeSymbol (1 - CBPY, "MB_CBPY1");
				break;
			case 2:
				m_statsVP->nBitsCBPY += m_pentrencSet->m_pentrencCBPY2->encodeSymbol (3 - CBPY, "MB_CBPY2");
				break;
			case 3:
				m_statsVP->nBitsCBPY += m_pentrencSet->m_pentrencCBPY3->encodeSymbol (7 - CBPY, "MB_CBPY3");
				break;
			case 4:
				m_statsVP->nBitsCBPY += m_pentrencSet->m_pentrencCBPY->encodeSymbol (15 - CBPY, "MB_CBPY");
				break;
			default:
				assert (FALSE);
			}
		}

		if ( m_vopmd.vopPredType != IVOP &&
				(pmbmd->m_dctMd == INTERQ || pmbmd->m_dctMd == INTRAQ)) {
			Int DQUANT = pmbmd->m_intStepDelta;			//send DQUANT
			assert (DQUANT >= -2 && DQUANT <= 2);
			if (DQUANT != 0) {	
				if (sign (DQUANT) == 1)
					m_pbitstrmOut->putBits (DQUANT + 1, 2, "MB_DQUANT");
				else
					m_pbitstrmOut->putBits (-1 - DQUANT, 2, "MB_DQUANT");
				m_statsVP->nBitsDQUANT += 2;
			}
		}				 
		
		if (m_vopmd.vopPredType	!= IVOP &&
				(pmbmd->m_dctMd == INTRA || pmbmd->m_dctMd == INTRAQ))	{
			UInt iBlk = 0;
			for (iBlk = Y_BLOCK1; iBlk <= V_BLOCK; iBlk++) {
				UInt nBits = 0;
#ifdef __TRACE_AND_STATS_
				m_pbitstrmOut->trace (iBlk, "BLK_NO");
#endif // __TRACE_AND_STATS_
				if (iBlk < U_BLOCK)
					if (pmbmd -> m_rgTranspStatus [iBlk] == ALL) continue;
				Int* rgiCoefQ = iCoefQ_DP [iMBnum][iBlk - 1];
#ifdef __TRACE_AND_STATS_
				m_pbitstrmOut->trace (rgiCoefQ[0], "IntraDC");
#endif // __TRACE_AND_STATS_
				if (pmbmd->m_bCodeDcAsAc != TRUE)	{
					nBits = sendIntraDC (rgiCoefQ, iBlk);
				}
				switch (iBlk) {
				case U_BLOCK: 
					m_statsVP->nBitsCr += nBits;
					break;
				case V_BLOCK: 
					m_statsVP->nBitsCb += nBits;
					break;
				default:
					m_statsVP->nBitsY += nBits;
				}
			}	
		}
	}

	for(iMBnum = iVPMBnum, pmbmd = m_rgmbmd+iVPMBnum, pmv = m_rgmv+iVPMBnum*PVOP_MV_PER_REF_PER_MB;
			iMBnum <= iVPlastMBnum; iMBnum++, pmbmd++, pmv+=PVOP_MV_PER_REF_PER_MB) {
		if (pmbmd->m_bSkip || pmbmd -> m_rgTranspStatus [0] == ALL)
			continue;

		iMBX = iMBnum % m_iNumMBX;
		iMBY = iMBnum / m_iNumMBX;
#ifdef __TRACE_AND_STATS_
		m_pbitstrmOut->trace (CSite (iMBX, iMBY), "TextureTcoef_MB_X_Y");
#endif // __TRACE_AND_STATS_
		if (pmbmd->m_dctMd == INTRA || pmbmd->m_dctMd == INTRAQ)	{
			UInt iBlk = 0;
			for (iBlk = Y_BLOCK1; iBlk <= V_BLOCK; iBlk++) {
				UInt nBits = 0;
#ifdef __TRACE_AND_STATS_
				m_pbitstrmOut->trace (iBlk, "BLK_NO");
#endif // __TRACE_AND_STATS_
				if (iBlk < U_BLOCK)
					if (pmbmd -> m_rgTranspStatus [iBlk] == ALL) continue;
////				Int* rgiCoefQ = m_rgpiCoefQ [iBlk - 1];
				Int* rgiCoefQ = iCoefQ_DP [iMBnum][iBlk - 1];
#ifdef __TRACE_AND_STATS_
				m_pbitstrmOut->trace (rgiCoefQ, BLOCK_SQUARE_SIZE, "BLK_QUANTIZED_COEF");
#endif // __TRACE_AND_STATS_
				Int iCoefStart;
				if (pmbmd->m_bCodeDcAsAc != TRUE)	{
					iCoefStart = 1;
				} else {
					iCoefStart = 0;
				}
				if (pmbmd->getCodedBlockPattern (iBlk))	{
					Int* rgiZigzag = grgiStandardZigzag;
					if (pmbmd->m_bACPrediction)	
						rgiZigzag = (pmbmd->m_preddir [iBlk - 1] == HORIZONTAL) ? grgiVerticalZigzag : grgiHorizontalZigzag;
					// 09/19/99 HHI Schueuer: added for sadct
					if (!m_volmd.bSadctDisable)
						rgiZigzag = m_pscanSelector->select_DP (rgiZigzag, (pmbmd->m_rgTranspStatus[0] == PARTIAL), iBlk, iRowLength_DP[iMBnum]);
					// end HHI
					if( m_volmd.bReversibleVlc == TRUE )
						nBits += sendTCOEFIntraRVLC (rgiCoefQ, iCoefStart, rgiZigzag, FALSE);
					else
						nBits += sendTCOEFIntra (rgiCoefQ, iCoefStart, rgiZigzag);
				}
				switch (iBlk) {
				case U_BLOCK: 
					m_statsVP->nBitsCr += nBits;
					break;
				case V_BLOCK: 
					m_statsVP->nBitsCb += nBits;
					break;
				default:
					m_statsVP->nBitsY += nBits;
				}
			}	
		} else {
			UInt nBits, iBlk = 0;
			// 09/19/99 HHI Schueuer
			Int* scan = grgiStandardZigzag;
			// end HHI
			for (iBlk = Y_BLOCK1; iBlk <= V_BLOCK; iBlk++) {
				// 09/19/99 HHI Schueuer
				scan = grgiStandardZigzag;
				// end HHI
#ifdef __TRACE_AND_STATS_
				m_pbitstrmOut->trace (iBlk, "BLK_NO");
#endif // __TRACE_AND_STATS_
				if (iBlk < U_BLOCK)
					if (pmbmd -> m_rgTranspStatus [iBlk] == ALL) 
						continue;
				if (pmbmd->getCodedBlockPattern (iBlk))	{
					Int* rgiCoefQ = iCoefQ_DP [iMBnum][iBlk - 1];
#ifdef __TRACE_AND_STATS_
					m_pbitstrmOut->trace (rgiCoefQ, BLOCK_SQUARE_SIZE, "BLK_QUANTIZED_COEF");
#endif // __TRACE_AND_STATS_

					if (!m_volmd.bSadctDisable) 
						scan = m_pscanSelector->select_DP (scan, (pmbmd->m_rgTranspStatus[0] == PARTIAL), iBlk, iRowLength_DP[iMBnum]);

					if( m_volmd.bReversibleVlc == TRUE )
						nBits = sendTCOEFInterRVLC (rgiCoefQ, 0, scan, FALSE);
					else
						nBits = sendTCOEFInter (rgiCoefQ, 0, scan);

					switch (iBlk) {
					case U_BLOCK: 
						m_statsVP->nBitsCr += nBits;
						break;
					case V_BLOCK: 
						m_statsVP->nBitsCb += nBits;
						break;
					default:
						m_statsVP->nBitsY += nBits;
					}
				}
			}	
		}
	}
}


Void CVideoObjectEncoder::encodeNSForIVOP_WithShape_DP ()
{
	assert( m_volmd.bDataPartitioning );
	assert( m_vopmd.vopPredType==IVOP );
	if(!(m_volmd.fAUsage == ONE_BIT && m_volmd.bShapeOnly == FALSE ))
		fatal_error("Grayscale alpha mode cannot be used with data partitioning");
	assert(m_volmd.nBits==8);

	//in case the IVOP is used as an ref for direct mode
	memset (m_rgmv, 0, m_iNumMB * PVOP_MV_PER_REF_PER_MB * sizeof (CMotionVector));
//#ifdef __SHARP_FIX_
	memset (m_rgmvBY, 0, m_iNumMB * sizeof (CMotionVector));
//#endif

	Int iMBX, iMBY;
	CMBMode* pmbmd = m_rgmbmd;
	Int iQPPrev = m_vopmd.intStepI;	//initialization

	Int iVPCounter = m_statsVOP.total();
	Int iVPtotal;
	m_iVPMBnum = 0;
	CStatistics m_statsVP(0);
	// DCT coefficient buffer for Data Partitioning mode
	Int*** iCoefQ_DP = new Int** [m_iNumMB];

	// 09/17/99 HHI  Schueuer: buffer for rowlength needed by sadct
	Int*** iRowLength_DP;
	if (!m_volmd.bSadctDisable) 
		iRowLength_DP =  new Int** [m_iNumMB];
	else
		iRowLength_DP = NULL;
	// end HHI

	// Set not to output but count bitstream
//	m_pbitstrmOut->SetDontSendBits(TRUE);

	PixelC* ppxlcRefY  = (PixelC*) m_pvopcRefQ1->pixelsY () + m_iStartInRefToCurrRctY;
	PixelC* ppxlcRefU  = (PixelC*) m_pvopcRefQ1->pixelsU () + m_iStartInRefToCurrRctUV;
	PixelC* ppxlcRefV  = (PixelC*) m_pvopcRefQ1->pixelsV () + m_iStartInRefToCurrRctUV;
	PixelC* ppxlcRefBY = (PixelC*) m_pvopcRefQ1->pixelsBY () + m_iStartInRefToCurrRctY;
	PixelC* ppxlcRefBUV = (PixelC*) m_pvopcRefQ1->pixelsBUV () + m_iStartInRefToCurrRctUV;

	PixelC* ppxlcOrigY = (PixelC*) m_pvopcOrig->pixelsBoundY ();
	PixelC* ppxlcOrigU = (PixelC*) m_pvopcOrig->pixelsBoundU ();
	PixelC* ppxlcOrigV = (PixelC*) m_pvopcOrig->pixelsBoundV ();
	PixelC* ppxlcOrigBY = (PixelC*) m_pvopcOrig->pixelsBoundBY ();

	Bool bRestartDelayedQP = TRUE;

	Int iMB = 0;

	for (iMBY = 0; iMBY < m_iNumMBY; iMBY++) {
		PixelC*  ppxlcRefMBY  = ppxlcRefY;
		PixelC*  ppxlcRefMBU  = ppxlcRefU;
		PixelC*  ppxlcRefMBV  = ppxlcRefV;
		PixelC*  ppxlcRefMBBY = ppxlcRefBY;
		PixelC*  ppxlcRefMBBUV = ppxlcRefBUV;
		PixelC*  ppxlcOrigMBY = ppxlcOrigY;
		PixelC*  ppxlcOrigMBU = ppxlcOrigU;
		PixelC*  ppxlcOrigMBV = ppxlcOrigV;
		PixelC*  ppxlcOrigMBBY = ppxlcOrigBY;
		for (iMBX = 0; iMBX < m_iNumMBX; iMBX++, iMB++) {
#ifdef __TRACE_AND_STATS_
			m_pbitstrmOut->trace (CSite (iMBX, iMBY), "MB_X_Y");
#endif // __TRACE_AND_STATS_

			Int iVPlastMBnum = iMBY * m_iNumMBX + iMBX;
			// shape bitstream is set to shape cache
			m_pbitstrmShapeMBOut = m_pbitstrmShape_DP[iVPlastMBnum];
			m_statsMB.reset ();
			pmbmd->m_bSkip = FALSE;	//reset for direct mode 
			pmbmd->m_bMCSEL = FALSE; //reset for direct mode
			pmbmd->m_bPadded = FALSE;
			pmbmd->m_dctMd = INTRA;

			// MB level rate control section
			// here is where we calculate the delta QP
			if (m_uiRateControl>=RC_TM5) {
				// TM5 rate control
				updateQP(pmbmd, iQPPrev, m_tm5rc.tm5rc_calc_mquant(iMB, m_statsVOP.total())	);
			}
			else
			{
#ifdef _MBQP_CHANGE_
				Int iDQuant = (rand() % 5) - 2;
				updateQP(pmbmd, iQPPrev, iQPPrev + iDQuant);
#else
				// no change in step size, but still need to call...
				updateQP(pmbmd, iQPPrev, iQPPrev);
#endif //_MBQP_CHANGE_
			}

			copyToCurrBuffWithShape (
				ppxlcOrigMBY, ppxlcOrigMBU, ppxlcOrigMBV, 
				ppxlcOrigMBBY, NULL,
				m_iFrameWidthY, m_iFrameWidthUV
			);

			decideTransparencyStatus (pmbmd, m_ppxlcCurrMBBY);
			downSampleBY (m_ppxlcCurrMBBY, m_ppxlcCurrMBBUV, pmbmd); // downsample original BY now for LPE padding (using original shape)
			if (pmbmd -> m_rgTranspStatus [0] == PARTIAL) {
				LPEPadding (pmbmd);
				m_statsMB.nBitsShape += codeIntraShape (ppxlcRefMBBY, pmbmd, iMBX, iMBY);
				decideTransparencyStatus (pmbmd, m_ppxlcCurrMBBY); // need to modify it a little (NONE block won't change)
				downSampleBY (m_ppxlcCurrMBBY, m_ppxlcCurrMBBUV, pmbmd);
			}
			else
				m_statsMB.nBitsShape += codeIntraShape (ppxlcRefMBBY, pmbmd, iMBX, iMBY);

			// Set not to output but count bitstream
			m_pbitstrmOut->SetDontSendBits(TRUE);

			if (pmbmd -> m_rgTranspStatus [0] != ALL) {
				if (!m_volmd.bSadctDisable) 
					deriveSADCTRowLengths (m_rgiCurrMBCoeffWidth, m_ppxlcCurrMBBY, m_ppxlcCurrMBBUV, pmbmd->m_rgTranspStatus);

				setDCVLCMode(pmbmd, &bRestartDelayedQP);

				// 09/17/99 HHI Schueuer
				if (!m_volmd.bSadctDisable)  
					quantizeTextureIntraMB (iMBX, iMBY, pmbmd, ppxlcRefMBY, ppxlcRefMBU, ppxlcRefMBV, NULL, m_ppxlcCurrMBBY, m_ppxlcCurrMBBUV);
				else
					quantizeTextureIntraMB (iMBX, iMBY, pmbmd, ppxlcRefMBY, ppxlcRefMBU, ppxlcRefMBV, NULL);
				// end HHI
				codeMBTextureHeadOfIVOP (pmbmd);
				sendDCTCoefOfIntraMBTexture (pmbmd);

				// MC padding
				if (pmbmd -> m_rgTranspStatus [0] == PARTIAL)
					mcPadCurrMB (ppxlcRefMBY, ppxlcRefMBU, ppxlcRefMBV, NULL);
				padNeighborTranspMBs (
					iMBX, iMBY,
					pmbmd,
					ppxlcRefMBY, ppxlcRefMBU, ppxlcRefMBV, NULL
				);
			}
			else {
				cancelQPUpdate(pmbmd);

				padCurrAndTopTranspMBFromNeighbor (
					iMBX, iMBY,
					pmbmd,
					ppxlcRefMBY, ppxlcRefMBU, ppxlcRefMBV, NULL
				);
			}
//				// Set to output bitstream
			m_pbitstrmOut->SetDontSendBits(FALSE);


			// copy DCT coefficient to buffer
			iCoefQ_DP[iVPlastMBnum] = new Int* [6];
			Int iBlk;
			for (iBlk = 0; iBlk < 6; iBlk++) {
				iCoefQ_DP [iVPlastMBnum] [iBlk] = new Int [BLOCK_SQUARE_SIZE];

				for( Int t = 0; t < BLOCK_SQUARE_SIZE; t++ )
					iCoefQ_DP[iVPlastMBnum][iBlk][t] = m_rgpiCoefQ[iBlk][t];
			}

			// 09/17/99 HHI  Schueuer: copy rowlength for sadct
			if (!m_volmd.bSadctDisable) {
				iRowLength_DP[iVPlastMBnum] = new Int* [11];
				Int iBlk;
				for (iBlk = ALL_Y_BLOCKS; iBlk < ALL_A_BLOCKS; iBlk++)
					iRowLength_DP[iVPlastMBnum][iBlk] = 0;

				if(pmbmd -> m_rgTranspStatus [0] != ALL) { 
					for (iBlk = Y_BLOCK1; iBlk <= A_BLOCK4; iBlk++) {
						iRowLength_DP [iVPlastMBnum][iBlk] = new Int [BLOCK_SIZE];
						for ( Int t = 0; t < BLOCK_SIZE; t++)
							iRowLength_DP [iVPlastMBnum][iBlk][t] = m_rgiCurrMBCoeffWidth[iBlk][t];
					}
				}
			}
			// end HHI

			iQPPrev = pmbmd->m_stepSize;

			ppxlcOrigMBBY += MB_SIZE;
			pmbmd++;
#ifdef __TRACE_AND_STATS_
			m_statsVOP   += m_statsMB;
#endif // __TRACE_AND_STATS_
			ppxlcRefMBY  += MB_SIZE;
			ppxlcRefMBU  += BLOCK_SIZE;
			ppxlcRefMBV  += BLOCK_SIZE;
			ppxlcRefMBBY += MB_SIZE;
			ppxlcRefMBBUV += BLOCK_SIZE;
			ppxlcOrigMBY += MB_SIZE;
			ppxlcOrigMBU += BLOCK_SIZE;
			ppxlcOrigMBV += BLOCK_SIZE;

			iVPtotal = (int) m_statsVOP.total() - iVPCounter;
			if( iVPtotal > m_volmd.bVPBitTh || iVPlastMBnum == m_iNumMB-1 /* last MB in a VOP */) {
				// Set to output bitstream
				m_pbitstrmOut->SetDontSendBits(FALSE);
				// encode video packet
				iVPCounter = m_statsVOP.total();
				m_statsVP.reset();
				if( m_iVPMBnum > 0 )
				{
					m_statsVP.nBitsHead = codeVideoPacketHeader (m_rgmbmd[m_iVPMBnum].m_stepSize - m_rgmbmd[m_iVPMBnum].m_intStepDelta);	
				}

				bRestartDelayedQP = TRUE;

				DataPartitioningMotionCoding(m_iVPMBnum, iVPlastMBnum, &m_statsVP, iCoefQ_DP);

				m_pbitstrmOut -> putBits (DC_MARKER, NUMBITS_DP_DC_MARKER, "DC_marker");
				m_statsVP.nBitsHead += NUMBITS_DP_DC_MARKER;

				// 09/19/99 HHI Schueuer: added for sadct
				if (!m_volmd.bSadctDisable) 
					DataPartitioningTextureCoding(m_iVPMBnum, iVPlastMBnum, &m_statsVP, iCoefQ_DP, iRowLength_DP);
				else
					DataPartitioningTextureCoding(m_iVPMBnum, iVPlastMBnum, &m_statsVP, iCoefQ_DP);

				assert( iVPtotal + m_statsVP.nBitsHead == m_statsVP.total() );

				m_iVPMBnum = iVPlastMBnum + 1;
			}
		}
		
		MacroBlockMemory** ppmbmTemp = m_rgpmbmAbove;
		m_rgpmbmAbove = m_rgpmbmCurr;
		m_rgpmbmCurr  = ppmbmTemp;
		ppxlcRefY  += m_iFrameWidthYxMBSize;
		ppxlcRefU  += m_iFrameWidthUVxBlkSize;
		ppxlcRefV  += m_iFrameWidthUVxBlkSize;
		ppxlcRefBY += m_iFrameWidthYxMBSize;
		ppxlcRefBUV += m_iFrameWidthUVxBlkSize;

		ppxlcOrigY += m_iFrameWidthYxMBSize;
		ppxlcOrigBY += m_iFrameWidthYxMBSize;
		ppxlcOrigU += m_iFrameWidthUVxBlkSize;
		ppxlcOrigV += m_iFrameWidthUVxBlkSize;
	}

	// delete CoefQ_DP
	for(iMB = 0; iMB < m_iNumMB; iMB++ )  {
		for (Int iBlk = 0; iBlk < 6; iBlk++) {
			delete [] iCoefQ_DP [iMB] [iBlk];
		}
		delete [] iCoefQ_DP[iMB];
	}
	delete [] iCoefQ_DP;

	// 09/17/99 HHI  Schueuer: delete iRowLength _DP sadct
	if (!m_volmd.bSadctDisable) {
		for (Int iMB = 0; iMB < m_iNumMB; iMB++) {
			for (Int iBlk = 0; iBlk < 11; iBlk++) 
				delete [] iRowLength_DP[iMB][iBlk];
			delete [] iRowLength_DP[iMB];
		}
		delete [] iRowLength_DP;
	}
	else {
		delete []iRowLength_DP;
	}
	// end HHI;

	// restore normal output stream
	m_pbitstrmShapeMBOut = m_pbitstrmOut;

	// Set to output bitstream
	m_pbitstrmOut->SetDontSendBits(FALSE);
}

Void CVideoObjectEncoder::encodeNSForPVOP_WithShape_DP ()
{
	assert( m_volmd.bDataPartitioning );
	assert( m_vopmd.vopPredType==PVOP || (m_uiSprite == 2 && m_vopmd.vopPredType==SPRITE) ); // GMC
	if(!(m_volmd.fAUsage == ONE_BIT && m_volmd.bShapeOnly == FALSE ))
		fatal_error("Grayscale alpha mode cannot be used with data partitioning");
	assert(m_volmd.nBits==8);

	Int iMBX, iMBY;

	motionEstPVOP_WithShape ();

	UInt newQStep = m_vopmd.intStep; // for frame based rate control
	// vopmd.intStep is updated at bottom of this function

	// Rate Control
	if (m_uiRateControl==RC_MPEG4) {
		Double Ec = m_iMAD / (Double) (m_iNumMBY * m_iNumMBX * 16 * 16 * m_iRRVScale *m_iRRVScale);
		m_statRC.setMad (Ec);
		// calculate for next frame (should be this frame, but its too late to send vop qp!)
		newQStep = m_statRC.updateQuanStepsize (m_vopmd.intStep);
		// this is not the correct way to use rate control
		m_statRC.setQc (m_vopmd.intStep);
	}

	CoordI y = m_rctCurrVOPY.top; 
	CMBMode* pmbmd = m_rgmbmd;
	Int iQPPrev = m_vopmd.intStep;
	CMotionVector* pmv = m_rgmv;
	CMotionVector* pmvBY = m_rgmvBY;

	Int iVPCounter = m_statsVOP.total();
	Int iVPtotal;
	m_iVPMBnum = 0;
	CStatistics m_statsVP(0);
	// DCT coefficient buffer for Data Partitioning mode
	Int*** iCoefQ_DP = new Int** [m_iNumMB];

	// 09/19/99 HHI  Schueuer: buffer for rowlength needed by sadct
	Int*** iRowLength_DP;
	if (!m_volmd.bSadctDisable) 
		iRowLength_DP =  new Int** [m_iNumMB];
	else
		iRowLength_DP = NULL;
	// end HHI

	PixelC* ppxlcRefY = (PixelC*) m_pvopcRefQ1->pixelsY () + m_iStartInRefToCurrRctY;
	PixelC* ppxlcRefU = (PixelC*) m_pvopcRefQ1->pixelsU () + m_iStartInRefToCurrRctUV;
	PixelC* ppxlcRefV = (PixelC*) m_pvopcRefQ1->pixelsV () + m_iStartInRefToCurrRctUV;
	PixelC* ppxlcRefBY = (PixelC*) m_pvopcRefQ1->pixelsBY () + m_iStartInRefToCurrRctY;
	PixelC* ppxlcRefBUV = (PixelC*) m_pvopcRefQ1->pixelsBUV () + m_iStartInRefToCurrRctUV;
		
	PixelC* ppxlcOrigY = (PixelC*) m_pvopcOrig->pixelsBoundY ();
	PixelC* ppxlcOrigU = (PixelC*) m_pvopcOrig->pixelsBoundU ();
	PixelC* ppxlcOrigV = (PixelC*) m_pvopcOrig->pixelsBoundV ();
	PixelC* ppxlcOrigBY = (PixelC*) m_pvopcOrig->pixelsBoundBY ();

// Added for error resilient mode by Toshiba(1997-11-14)
	Bool bCodeVPHeaderNext = FALSE;		// needed only for OBMC
	Int	iTempVPMBnum = 0;
	Int iCounter;
// End Toshiba(1997-11-14)

	Bool bRestartDelayedQP = TRUE;

	Int iMB = 0;
	for (iMBY = 0; iMBY < m_iNumMBY; iMBY++, y += MB_SIZE) {
		PixelC* ppxlcRefMBY = ppxlcRefY;
		PixelC* ppxlcRefMBU = ppxlcRefU;
		PixelC* ppxlcRefMBV = ppxlcRefV;
		PixelC* ppxlcRefMBBY = ppxlcRefBY;
		PixelC* ppxlcRefMBBUV = ppxlcRefBUV;
		PixelC* ppxlcOrigMBY = ppxlcOrigY;
		PixelC* ppxlcOrigMBU = ppxlcOrigU;
		PixelC* ppxlcOrigMBV = ppxlcOrigV;
		PixelC* ppxlcOrigMBBY = ppxlcOrigBY;
		CoordI x = m_rctCurrVOPY.left;

#ifdef __TRACE_AND_STATS_
		m_statsMB.reset ();
#endif // __TRACE_AND_STATS_

		// initiate advance shape coding
		// shape bitstream is set to shape cache
		m_pbitstrmShapeMBOut = m_pbitstrmShape_DP[iMBY * m_iNumMBX];

		copyToCurrBuffJustShape (ppxlcOrigMBBY, m_iFrameWidthY);
		// Modified for error resilient mode by Toshiba(1997-11-14)
		ShapeMode shpmdColocatedMB;
		if(m_vopmd.bShapeCodingType) {
			shpmdColocatedMB = m_rgmbmdRef [
				min (max (0, iMBY), m_iNumMBYRef - 1) * m_iNumMBXRef].m_shpmd;
			encodePVOPMBJustShape(ppxlcRefMBBY, pmbmd, shpmdColocatedMB, pmv, pmvBY, x, y, 0, iMBY);
		}
		else {
			m_statsMB.nBitsShape += codeIntraShape (ppxlcRefMBBY, pmbmd, 0, iMBY);
			decideTransparencyStatus (pmbmd, m_ppxlcCurrMBBY); 
		}

		if(pmbmd->m_bhas4MVForward)
			padMotionVectors(pmbmd,pmv);

		for (iMBX = 0; iMBX < m_iNumMBX; iMBX++, x += MB_SIZE, iMB++)	{
#ifdef __TRACE_AND_STATS_
			m_pbitstrmOut->trace (CSite (iMBX, iMBY), "MB_X_Y (Texture)");
			// shape quantization part
			m_statsMB.reset ();
#endif // __TRACE_AND_STATS_

			Int iVPlastMBnum = iMBY * m_iNumMBX + iMBX;

			pmbmd->m_bPadded=FALSE;

			// MB level rate control section
			// here is where we calculate the delta QP
			if (m_uiRateControl>=RC_TM5) {
				// TM5 rate control
				updateQP(pmbmd, iQPPrev, m_tm5rc.tm5rc_calc_mquant(iMB, m_statsVOP.total())	);
			}
			else
			{
#ifdef _MBQP_CHANGE_
				Int iDQuant = (rand() % 5) - 2;
				updateQP(pmbmd, iQPPrev, iQPPrev + iDQuant);
#else
				// no change in step size, but still need to call...
				updateQP(pmbmd, iQPPrev, iQPPrev);
#endif //_MBQP_CHANGE_
			}

			if(iMBX<m_iNumMBX-1)
			{
				// shape bitstream is set to shape cache
				m_pbitstrmShapeMBOut = m_pbitstrmShape_DP[iMBY * m_iNumMBX + iMBX + 1];

				// The following operation is needed only for OBMC
				if( bCodeVPHeaderNext ) {
					iTempVPMBnum  = m_iVPMBnum;
					m_iVPMBnum = VPMBnum(iMBX+1, iMBY);
				}

				// code shape 1mb in advance
				copyToCurrBuffJustShape (ppxlcOrigMBBY+MB_SIZE,m_iFrameWidthY);
				// Modified for error resilient mode by Toshiba(1997-11-14)
				if(m_vopmd.bShapeCodingType) {
					shpmdColocatedMB = m_rgmbmdRef [
						min (max (0, iMBX+1), m_iNumMBXRef-1) + 
 						min (max (0, iMBY), m_iNumMBYRef-1) * m_iNumMBXRef
					].m_shpmd;
					encodePVOPMBJustShape(
						ppxlcRefMBBY+MB_SIZE, pmbmd+1,
						 shpmdColocatedMB,
						 pmv+PVOP_MV_PER_REF_PER_MB,
						 pmvBY+1, x+MB_SIZE, y,
						 iMBX+1, iMBY
					);
				}
				else {
					m_statsMB.nBitsShape
						+= codeIntraShape (
							ppxlcRefMBBY+MB_SIZE,
							pmbmd+1, iMBX+1, iMBY
						);
					decideTransparencyStatus (pmbmd+1,
						 m_ppxlcCurrMBBY); 
				}

				// The following operation is needed only for OBMC
				if( bCodeVPHeaderNext )
					m_iVPMBnum = iTempVPMBnum;
				if((pmbmd+1)->m_bhas4MVForward)
					padMotionVectors(pmbmd+1, pmv+PVOP_MV_PER_REF_PER_MB);
			}
			
			// Set not to output but count bitstream
			m_pbitstrmOut->SetDontSendBits(TRUE);

			// copy DCT coefficient to buffer
			iCoefQ_DP[iVPlastMBnum] = new Int* [6];
			Int iBlk;
			for (iBlk = 0; iBlk < 6; iBlk++) {
				iCoefQ_DP [iVPlastMBnum] [iBlk] = new Int [BLOCK_SQUARE_SIZE];
			}

			// 09/19/99 HHI  Schueuer: copy rowlength for sadct
			if (!m_volmd.bSadctDisable) {
				iRowLength_DP[iVPlastMBnum] = new Int* [11];
				Int iBlk;
				for (iBlk = ALL_Y_BLOCKS; iBlk < ALL_A_BLOCKS; iBlk++)
					iRowLength_DP[iVPlastMBnum][iBlk] = 0;
			}
			// end HHI

			if (pmbmd -> m_rgTranspStatus [0] != ALL) {
				// need to copy binary shape too since curr buff is future shape
				copyToCurrBuffWithShape(ppxlcOrigMBY, ppxlcOrigMBU, ppxlcOrigMBV,
				ppxlcRefMBBY, NULL, m_iFrameWidthY, m_iFrameWidthUV);

				//Changed HHI 2000-04-11
				downSampleBY (m_ppxlcCurrMBBY, m_ppxlcCurrMBBUV, pmbmd);

				//  09/19/99 HHI  Schueuer: sadct
				if (!m_volmd.bSadctDisable){ 
					deriveSADCTRowLengths (m_rgiCurrMBCoeffWidth, m_ppxlcCurrMBBY, m_ppxlcCurrMBBUV, pmbmd->m_rgTranspStatus);
					encodePVOPMBTextureWithShape(ppxlcRefMBY, ppxlcRefMBU, ppxlcRefMBV, NULL, pmbmd, pmv, iMBX, iMBY, x, y,
						&bRestartDelayedQP, m_ppxlcCurrMBBY, m_ppxlcCurrMBBUV);
				}
				else
					encodePVOPMBTextureWithShape(ppxlcRefMBY, ppxlcRefMBU, ppxlcRefMBV, NULL, pmbmd, pmv, iMBX, iMBY, x, y,
						&bRestartDelayedQP);

				// copy DCT coefficient to buffer
				for (iBlk = 0; iBlk < 6; iBlk++) {
					for( Int t = 0; t < BLOCK_SQUARE_SIZE; t++ )
						iCoefQ_DP[iVPlastMBnum][iBlk][t] = m_rgpiCoefQ[iBlk][t];
				}

				// 09/19/99 HHI  Schueuer: copy rowlength for sadct
				if (!m_volmd.bSadctDisable) {
					for (iBlk = Y_BLOCK1; iBlk <= A_BLOCK4; iBlk++) {
						iRowLength_DP [iVPlastMBnum][iBlk] = new Int [BLOCK_SIZE];
						for ( Int t = 0; t < BLOCK_SIZE; t++)
							iRowLength_DP [iVPlastMBnum][iBlk][t] = m_rgiCurrMBCoeffWidth[iBlk][t];
					}
				}
				// end HHI

				if (pmbmd -> m_rgTranspStatus [0] == PARTIAL)
					mcPadCurrMB (ppxlcRefMBY, ppxlcRefMBU, ppxlcRefMBV, NULL);
				padNeighborTranspMBs (
					iMBX, iMBY,
					pmbmd,
					ppxlcRefMBY, ppxlcRefMBU, ppxlcRefMBV, NULL
				);
			}
			else {
				cancelQPUpdate(pmbmd);

				padCurrAndTopTranspMBFromNeighbor (
					iMBX, iMBY,
					pmbmd,
					ppxlcRefMBY, ppxlcRefMBU, ppxlcRefMBV, NULL
				);
			}

			// Set to output bitstream
			m_pbitstrmOut->SetDontSendBits(FALSE);

			iQPPrev = pmbmd->m_stepSize;

			pmbmd++;
			pmv += PVOP_MV_PER_REF_PER_MB;
			pmvBY++;
			ppxlcRefMBBY += MB_SIZE;
			ppxlcRefMBBUV += BLOCK_SIZE;
			ppxlcOrigMBBY += MB_SIZE;
#ifdef __TRACE_AND_STATS_
			m_statsVOP += m_statsMB;
#endif // __TRACE_AND_STATS_
			ppxlcRefMBY += MB_SIZE;
			ppxlcRefMBU += BLOCK_SIZE;
			ppxlcRefMBV += BLOCK_SIZE;
			ppxlcOrigMBY += MB_SIZE;
			ppxlcOrigMBU += BLOCK_SIZE;
			ppxlcOrigMBV += BLOCK_SIZE;
			iVPtotal = (int) m_statsVOP.total() - iVPCounter;
			if( bCodeVPHeaderNext || iVPlastMBnum == m_iNumMB-1 /* last MB in a VOP */) {
				// Set to output bitstream
				m_pbitstrmOut->SetDontSendBits(FALSE);
				// encode video packet
				iVPCounter = m_statsVOP.total();
				m_statsVP.reset();
				if( m_iVPMBnum > 0 )
				{
					m_statsVP.nBitsHead = codeVideoPacketHeader (m_rgmbmd[m_iVPMBnum].m_stepSize - m_rgmbmd[m_iVPMBnum].m_intStepDelta);
				}

				bRestartDelayedQP = TRUE;

				DataPartitioningMotionCoding(m_iVPMBnum, iVPlastMBnum, &m_statsVP, iCoefQ_DP);

				m_pbitstrmOut -> putBits (MOTION_MARKER, NUMBITS_DP_MOTION_MARKER, "motion_marker");
				m_statsVP.nBitsHead += NUMBITS_DP_MOTION_MARKER;

				if (!m_volmd.bSadctDisable) 
					DataPartitioningTextureCoding(m_iVPMBnum, iVPlastMBnum, &m_statsVP, iCoefQ_DP, iRowLength_DP);
				else
					DataPartitioningTextureCoding(m_iVPMBnum, iVPlastMBnum, &m_statsVP, iCoefQ_DP);

				m_iVPMBnum = iVPlastMBnum + 1;
			}
			// The following operation is needed only for OBMC
			iCounter = m_statsVOP.total();
			bCodeVPHeaderNext = iCounter - iVPCounter > m_volmd.bVPBitTh;
		}
		MacroBlockMemory** ppmbmTemp = m_rgpmbmAbove;
		m_rgpmbmAbove = m_rgpmbmCurr;
		m_rgpmbmCurr  = ppmbmTemp;

		ppxlcRefY += m_iFrameWidthYxMBSize;
		ppxlcRefU += m_iFrameWidthUVxBlkSize;
		ppxlcRefV += m_iFrameWidthUVxBlkSize;
		ppxlcRefBY += m_iFrameWidthYxMBSize;
		ppxlcRefBUV += m_iFrameWidthUVxBlkSize;
		
		ppxlcOrigY += m_iFrameWidthYxMBSize;
		ppxlcOrigBY += m_iFrameWidthYxMBSize;
		ppxlcOrigU += m_iFrameWidthUVxBlkSize;
		ppxlcOrigV += m_iFrameWidthUVxBlkSize;
	}

	// delete CoefQ_DP
	for(iMB = 0; iMB < m_iNumMB; iMB++ )  {
		for (Int iBlk = 0; iBlk < 6; iBlk++) {
			delete [] iCoefQ_DP [iMB] [iBlk];
		}
		delete [] iCoefQ_DP[iMB];
	}
	delete [] iCoefQ_DP;

	// 09/19/99 HHI  Schueuer: delete iRowLength _DP sadct
	if (!m_volmd.bSadctDisable) {
		for (iMB = 0; iMB < m_iNumMB; iMB++) {
			for (Int iBlk = 0; iBlk < 11; iBlk++) 
				delete [] iRowLength_DP[iMB][iBlk];
			delete [] iRowLength_DP[iMB];
		}
		delete [] iRowLength_DP;
	}
	else {
		delete [] iRowLength_DP;
	}
	// end HHI;

	// restore normal output stream
	m_pbitstrmShapeMBOut = m_pbitstrmOut;

	// Set to output bitstream
	m_pbitstrmOut->SetDontSendBits(FALSE);

	m_vopmd.intStep = newQStep;
}

//////////////////////////////////////////////////////////
////  The following functions are for Reversible VLC  ////
//////////////////////////////////////////////////////////

UInt CVideoObjectEncoder::sendTCOEFIntraRVLC (const Int* rgiCoefQ, Int iStart, Int* rgiZigzag, Bool bDontSendBits)
{
	assert( m_volmd.bDataPartitioning && m_volmd.bReversibleVlc );

	Bool bIsFirstRun = TRUE;
	Bool bIsLastRun = FALSE;
	UInt uiCurrRun = 0;
	UInt uiPrevRun = 0;
	//	Int	 iCurrLevel = 0;
	Int  iPrevLevel = 0;
	//	UInt uiCoefToStart = 0;
	UInt numBits = 0;

	for (Int j = iStart; j < BLOCK_SQUARE_SIZE; j++) {
		if (rgiCoefQ [rgiZigzag [j]] == 0)								// zigzag here
			uiCurrRun++;											// counting zeros
		else {
			if (!bIsFirstRun)
				numBits += putBitsOfTCOEFIntraRVLC (uiPrevRun, iPrevLevel, bIsLastRun, bDontSendBits);
			uiPrevRun = uiCurrRun; 									// reset for next run
			iPrevLevel = rgiCoefQ [rgiZigzag [j]]; 
			uiCurrRun = 0;											
			bIsFirstRun = FALSE;										
		}
	}
	assert (uiPrevRun <= (BLOCK_SQUARE_SIZE - 1) - 1);				// Some AC must be non-zero; at least for inter
	bIsLastRun = TRUE;
	numBits += putBitsOfTCOEFIntraRVLC (uiPrevRun, iPrevLevel, bIsLastRun, bDontSendBits);
	return numBits;
}

UInt CVideoObjectEncoder::putBitsOfTCOEFIntraRVLC (UInt uiRun, Int iLevel, Bool bIsLastRun, Bool bDontSendBits)
{
	assert(m_volmd.bDataPartitioning && m_volmd.bReversibleVlc );

	UInt nBits = 0;
	Long lVLCtableIndex;
	if (bIsLastRun == FALSE) {
		lVLCtableIndex = findVLCtableIndexOfNonLastEventIntraRVLC (bIsLastRun, uiRun, abs(iLevel));
		if (lVLCtableIndex != NOT_IN_TABLE)  {
			nBits += m_pentrencSet->m_pentrencDCTIntraRVLC->encodeSymbol(lVLCtableIndex, "Vlc_TCOEF_RVLC", bDontSendBits);	// huffman encode 
			if( !bDontSendBits )
				m_pentrencSet->m_pentrencDCTIntraRVLC->bitstream()->putBits ((Char) invSignOf (iLevel), 1, "Sign_TCOEF_RVLC");	
			nBits++;
		}
		else
			escapeEncodeRVLC (uiRun, iLevel, bIsLastRun, bDontSendBits);
	}
	else	{
		lVLCtableIndex = findVLCtableIndexOfLastEventIntraRVLC (bIsLastRun, uiRun, abs(iLevel));
		if (lVLCtableIndex != NOT_IN_TABLE)  {
			nBits += m_pentrencSet->m_pentrencDCTIntraRVLC->encodeSymbol(lVLCtableIndex, "Vlc_TCOEF_Last_RVLC", bDontSendBits);	// huffman encode 
			if( !bDontSendBits )
				m_pentrencSet->m_pentrencDCTIntraRVLC->bitstream()->putBits ((Char) invSignOf (iLevel), 1, "Sign_TCOEF_Last_RVLC");	
			nBits++;
		}
		else
			escapeEncodeRVLC (uiRun, iLevel, bIsLastRun, bDontSendBits);
	}
	return nBits;
}

Int CVideoObjectEncoder::findVLCtableIndexOfNonLastEventIntraRVLC (Bool bIsLastRun, UInt uiRun, UInt uiLevel)
{
	assert( m_volmd.bDataPartitioning && m_volmd.bReversibleVlc );
	assert (uiRun >= 0);
	if (uiRun > 19 || (uiLevel > grgIfNotLastNumOfLevelAtRunIntraRVLC [uiRun]))
		return NOT_IN_TABLE;
	else	{
		UInt uiTableIndex = 0;	
		for (UInt i = 0; i < uiRun; i++)
			uiTableIndex += grgIfNotLastNumOfLevelAtRunIntraRVLC [i];
		uiTableIndex += uiLevel;
		uiTableIndex--;												// make it zero-based; see Table H13/H.263
		return uiTableIndex;
	}
}

Int CVideoObjectEncoder::findVLCtableIndexOfLastEventIntraRVLC (Bool bIsLastRun, UInt uiRun, UInt uiLevel)
{
	assert( m_volmd.bDataPartitioning && m_volmd.bReversibleVlc );
	assert (uiRun >= 0);
	if (uiRun > 44 || (uiLevel > grgIfLastNumOfLevelAtRunIntraRVLC [uiRun]))
		return NOT_IN_TABLE;
	else	{
		UInt uiTableIndex = 0;	
		for (UInt i = 0; i < uiRun; i++)
			uiTableIndex += grgIfLastNumOfLevelAtRunIntraRVLC [i];
		uiTableIndex += uiLevel;		
		uiTableIndex += 102;									// make it zero-based and 
		return uiTableIndex;								//correction of offset; see Table H13/H.263
	}
}

UInt CVideoObjectEncoder::sendTCOEFInterRVLC (const Int* rgiCoefQ, Int iStart, Int* rgiZigzag, Bool bDontSendBits)
{
	assert( m_volmd.bDataPartitioning && m_volmd.bReversibleVlc );

	Bool bIsFirstRun = TRUE;
	Bool bIsLastRun = FALSE;
	UInt uiCurrRun = 0;
	UInt uiPrevRun = 0;
	//	Int	 iCurrLevel = 0;
	Int  iPrevLevel = 0;
	UInt numBits = 0;

	for (Int j = iStart; j < BLOCK_SQUARE_SIZE; j++) {
		if (rgiCoefQ [rgiZigzag [j]] == 0)								// zigzag here
			uiCurrRun++;											// counting zeros
		else {
			if (!bIsFirstRun)
				numBits += putBitsOfTCOEFInterRVLC (uiPrevRun, iPrevLevel, bIsLastRun, bDontSendBits);
			uiPrevRun = uiCurrRun; 									// reset for next run
			iPrevLevel = rgiCoefQ [rgiZigzag [j]]; 
			uiCurrRun = 0;											
			bIsFirstRun = FALSE;										
		}
	}
	assert (uiPrevRun <= (BLOCK_SQUARE_SIZE - 1));				// Some AC must be non-zero; at least for inter
	bIsLastRun = TRUE;
	numBits += putBitsOfTCOEFInterRVLC (uiPrevRun, iPrevLevel, bIsLastRun, bDontSendBits);
	return numBits;
}

UInt CVideoObjectEncoder::putBitsOfTCOEFInterRVLC (UInt uiRun, Int iLevel, Bool bIsLastRun, Bool bDontSendBits)
{
	assert( m_volmd.bDataPartitioning && m_volmd.bReversibleVlc );

	UInt nBits = 0;
	Long lVLCtableIndex;
	if (bIsLastRun == FALSE) {
		lVLCtableIndex = findVLCtableIndexOfNonLastEventInterRVLC (bIsLastRun, uiRun, abs(iLevel));
		if (lVLCtableIndex != NOT_IN_TABLE)  {
			nBits += m_pentrencSet->m_pentrencDCTRVLC->encodeSymbol(lVLCtableIndex, "Vlc_TCOEF_RVLC", bDontSendBits);	// huffman encode 
			if( !bDontSendBits )
				m_pentrencSet->m_pentrencDCTRVLC->bitstream()->putBits ((Char) invSignOf (iLevel), 1, "Sign_TCOEF_RVLC");
			nBits++;
		}
		else
			escapeEncodeRVLC (uiRun, iLevel, bIsLastRun, bDontSendBits);
	}
	else	{
		lVLCtableIndex = findVLCtableIndexOfLastEventInterRVLC (bIsLastRun, uiRun, abs(iLevel));
		if (lVLCtableIndex != NOT_IN_TABLE)  {
			nBits += m_pentrencSet->m_pentrencDCTRVLC->encodeSymbol(lVLCtableIndex, "Vlc_TCOEF_Last_RVLC", bDontSendBits);	// huffman encode 
			if( !bDontSendBits )
				m_pentrencSet->m_pentrencDCTRVLC->bitstream()->putBits ((Char) invSignOf (iLevel), 1, "Sign_TCOEF_Last_RVLC");			
			nBits++;
		}
		else
			escapeEncodeRVLC (uiRun, iLevel, bIsLastRun, bDontSendBits);
	}
	return nBits;
}

Int CVideoObjectEncoder::findVLCtableIndexOfNonLastEventInterRVLC (Bool bIsLastRun, UInt uiRun, UInt uiLevel)
{
	assert( m_volmd.bDataPartitioning && m_volmd.bReversibleVlc );
	assert (uiRun >= 0);
	if (uiRun > 38 || (uiLevel > grgIfNotLastNumOfLevelAtRunInterRVLC [uiRun]))
		return NOT_IN_TABLE;
	else	{
		UInt uiTableIndex = 0;	
		for (UInt i = 0; i < uiRun; i++)
			uiTableIndex += grgIfNotLastNumOfLevelAtRunInterRVLC [i];
		uiTableIndex += uiLevel;
		uiTableIndex--;												// make it zero-based; see Table H13/H.263
		return uiTableIndex;
	}
}

Int CVideoObjectEncoder::findVLCtableIndexOfLastEventInterRVLC (Bool bIsLastRun, UInt uiRun, UInt uiLevel)
{
	assert( m_volmd.bDataPartitioning && m_volmd.bReversibleVlc );
	assert (uiRun >= 0);
	if (uiRun > 44 || (uiLevel > grgIfLastNumOfLevelAtRunInterRVLC [uiRun]))
		return NOT_IN_TABLE;
	else	{
		UInt uiTableIndex = 0;	
		for (UInt i = 0; i < uiRun; i++)
			uiTableIndex += grgIfLastNumOfLevelAtRunInterRVLC [i];
		uiTableIndex += uiLevel;		
		uiTableIndex += 102;									// make it zero-based and 
		return uiTableIndex;								//correction of offset; see Table H13/H.263
	}
}

UInt CVideoObjectEncoder::escapeEncodeRVLC (UInt uiRun, Int iLevel, Bool bIsLastRun, Bool bDontSendBits)
{
	assert( m_volmd.bDataPartitioning && m_volmd.bReversibleVlc );

	UInt nBits = 0;

	Int iLevelAbs = abs (iLevel);
	Char iLevelSign = (Char) invSignOf (iLevel);

	nBits += m_pentrencSet->m_pentrencDCTRVLC->encodeSymbol(TCOEF_RVLC_ESCAPE, "FEsc_TCOEF_RVLC", bDontSendBits);
	if( !bDontSendBits )
		m_pentrencSet->m_pentrencDCTRVLC->bitstream()->putBits (1, 1, "FEsc1_TCOEF_RVLC");
	nBits ++;

	if( !bDontSendBits )
		m_pentrencSet->m_pentrencDCTRVLC->bitstream()->putBits ((Char) bIsLastRun, 1, "Last_Esc_TCOEF_RVLC");
	nBits ++;

	assert (uiRun < BLOCK_SQUARE_SIZE);
	if( !bDontSendBits )
		m_pentrencSet->m_pentrencDCTRVLC->bitstream()->putBits (uiRun, NUMBITS_RVLC_ESC_RUN, "Run_Esc_TCOEF_RVLC");	
	nBits += NUMBITS_RVLC_ESC_RUN;

	Int iLevelBits = 12; // = m_volmd.nBits;
	Int iMaxAC = (1<<(iLevelBits - 1)) - 1;
	assert (iLevelAbs <= iMaxAC && iLevelAbs > 0);
	if( !bDontSendBits )
	{
		m_pentrencSet->m_pentrencDCTRVLC->bitstream()->putBits (1, 1, "Marker");
		m_pentrencSet->m_pentrencDCTRVLC->bitstream()->putBits (iLevelAbs, iLevelBits - 1, "Level_Esc_TCOEF_RVLC");	
		m_pentrencSet->m_pentrencDCTRVLC->bitstream()->putBits (1, 1, "Marker");
	}
	nBits += iLevelBits - 1 + 2;

	nBits += m_pentrencSet->m_pentrencDCTRVLC->encodeSymbol(TCOEF_RVLC_ESCAPE, "LEsc_TCOEF_RVLC", bDontSendBits);
	if( !bDontSendBits )
		m_pentrencSet->m_pentrencDCTRVLC->bitstream()->putBits (iLevelSign, 1, "Sign_LEsc_TCOEF_RVLC");
	nBits ++;

	return nBits;
}

Int CVideoObjectEncoder::dumpCachedShapeBits_DP(Int iMBnum)
{
	Int ret;
#ifdef __TRACE_AND_STATS_
	m_pbitstrmOut->trace("INSERTING PRE-ENCODED MB SHAPE STREAM HERE\n");
	m_pbitstrmOut->trace(m_pbitstrmOut->getCounter(),"Location Before");
#endif // __TRACE_AND_STATS_
	m_pbitstrmOut->putBitStream(*m_pbitstrmShape_DP[iMBnum]);
	ret = m_pbitstrmShape_DP[iMBnum]->getCounter();
#ifdef __TRACE_AND_STATS_
	m_pbitstrmOut->trace(m_pbitstrmOut->getCounter(),"Location After");
#endif // __TRACE_AND_STATS_
	m_pbitstrmShape_DP[iMBnum]->flush();
	m_pbitstrmShape_DP[iMBnum]->resetAll();

	return(ret);
}


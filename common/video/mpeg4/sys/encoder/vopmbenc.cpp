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

	vopmbEnc.cpp

Abstract:

	Encode MB's for each VOP (I-, P-, and B-VOP's)

Revision History:
	Dec. 11, 1997:	Interlaced tools added by NextLevel Systems (GI)
                    B. Eifrig, (beifrig@nlvl.com) X. Chen, (xchen@nlvl.com)
        May. 9   1998:  add boundary by Hyundai Electronics 
                                  Cheol-Soo Park (cspark@super5.hyundai.co.kr) 
        May. 9   1998:  add field based MC padding by Hyundai Electronics 
                                  Cheol-Soo Park (cspark@super5.hyundai.co.kr) 
	May 9, 1999:	tm5 rate control by DemoGraFX, duhoff@mediaone.net

*************************************************************************/

#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <iostream.h>

#include "typeapi.h"
#include "codehead.h"
#include "entropy/bitstrm.hpp"
#include "entropy/entropy.hpp"
#include "entropy/huffman.hpp"
#include "mode.hpp"
#include "global.hpp"
#include "vopses.hpp"
#include "vopseenc.hpp"

#ifdef __MFC_
#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

#define new DEBUG_NEW				   
#endif // __MFC_


Void CVideoObjectEncoder::encodeVOP ()	
{

	if (m_uiRateControl>=RC_TM5) {
		m_tm5rc.tm5rc_init_pict(m_vopmd.vopPredType,
								m_pvopcOrig->pixelsBoundY (),
								m_iFrameWidthY,
								m_iNumMBX,
								m_iNumMBY);
	}

// Added for Data Partitioning mode by Toshiba(1998-1-16)
	if( m_volmd.bDataPartitioning ) {
		if (m_volmd.fAUsage == RECTANGLE) {
			switch( m_vopmd.vopPredType ) {
			case PVOP:
				encodeNSForPVOP_DP ();
				break;
			case IVOP:
				encodeNSForIVOP_DP ();
				break;
			case BVOP:
				encodeNSForBVOP ();
				break;
			default:
					assert(FALSE);			// B-VOP and Sprite-VOP are not supported in DP-mode
			}
		} else {
			switch( m_vopmd.vopPredType ) {
			case PVOP:
				encodeNSForPVOP_WithShape_DP ();
				break;
			case IVOP:
				encodeNSForIVOP_WithShape_DP ();
				break;
			case BVOP:
				encodeNSForBVOP_WithShape ();
				break;
			default:
				assert(FALSE);			// B-VOP and Sprite-VOP are not supported in DP-mode
			}
		}
	} else
// End Toshiba(1998-1-16)
	if (m_volmd.fAUsage == RECTANGLE) {
		if (m_vopmd.vopPredType == PVOP)
			encodeNSForPVOP ();
		else if (m_vopmd.vopPredType == IVOP)
			encodeNSForIVOP ();
		else
			encodeNSForBVOP ();
	}
	else {
		if (m_vopmd.vopPredType == PVOP)	{
			if (m_uiSprite == 1 && m_sptMode != BASIC_SPRITE && m_vopmd.SpriteXmitMode != STOP)
				encodeNSForPVOP ();
			else
				encodeNSForPVOP_WithShape ();
		}
		else if (m_vopmd.vopPredType == IVOP)
			encodeNSForIVOP_WithShape ();
		else
			encodeNSForBVOP_WithShape ();
	}
	if (m_uiRateControl>=RC_TM5) {
		m_tm5rc.tm5rc_update_pict(m_vopmd.vopPredType, m_statsVOP.total());
	}
}

//
// size:
//		m_pvopcCurr: original size (QCIF or CIF), same memory through out the session
// 		m_pvopfRef: expanded original size (QCIF or CIF expanded by 16), same memory through out the session
//		pmbmd and pmv: VOP.  need to reallocate every time
//
// things don't need to recompute:
//		m_iWidthY, m_iWidthUV, m_iWidthRefY, m_iWidthRefUV
// need to recompute:
//		m_uintNumMBX, m_uintNumMBY, m_uintNumMB
//

Void CVideoObjectEncoder::encodeNSForIVOP ()	
{
	//in case the IVOP is used as an ref for direct mode
	memset (m_rgmv, 0, m_iNumMB * PVOP_MV_PER_REF_PER_MB * sizeof (CMotionVector));

	CMBMode* pmbmd = m_rgmbmd;
	Int iQPPrev = m_vopmd.intStepI;	//initialization
	PixelC* ppxlcRefY  = (PixelC*) m_pvopcRefQ1->pixelsY () + m_iStartInRefToCurrRctY;
	PixelC* ppxlcRefU  = (PixelC*) m_pvopcRefQ1->pixelsU () + m_iStartInRefToCurrRctUV;
	PixelC* ppxlcRefV  = (PixelC*) m_pvopcRefQ1->pixelsV () + m_iStartInRefToCurrRctUV;
	
	PixelC* ppxlcOrigY = (PixelC*) m_pvopcOrig->pixelsBoundY ();
	PixelC* ppxlcOrigU = (PixelC*) m_pvopcOrig->pixelsBoundU ();
	PixelC* ppxlcOrigV = (PixelC*) m_pvopcOrig->pixelsBoundV ();

	// MB rate control
	//Int iIndexofQ = 0;
	//Int rgiQ [4] = {-1, -2, 1, 2};
	if (m_uiRateControl>=RC_TM5) iQPPrev = m_tm5rc.tm5rc_start_mb();
	// -----

	Bool bRestartDelayedQP = TRUE;

		Int iMBX, iMBY, iMB = 0;
	Int iMaxQP = (1<<m_volmd.uiQuantPrecision)-1; // NBIT
	for (iMBY = 0; iMBY < m_iNumMBY; iMBY++) {
		PixelC* ppxlcRefMBY  = ppxlcRefY;
		PixelC* ppxlcRefMBU  = ppxlcRefU;
		PixelC* ppxlcRefMBV  = ppxlcRefV;
		PixelC* ppxlcOrigMBY = ppxlcOrigY;
		PixelC* ppxlcOrigMBU = ppxlcOrigU;
		PixelC* ppxlcOrigMBV = ppxlcOrigV;

		// In a given Sprite object piece, identify whether current macroblock is not a hole and should be coded ?
		Bool  bSptMB_NOT_HOLE= TRUE;
		if (m_uiSprite == 1 && m_sptMode != BASIC_SPRITE && m_vopmd.SpriteXmitMode != STOP) {
			bSptMB_NOT_HOLE = SptPieceMB_NOT_HOLE(0, iMBY, pmbmd);	
			RestoreMBmCurrRow (iMBY, m_rgpmbmCurr);	
		}

		for (iMBX = 0; iMBX < m_iNumMBX; iMBX++, iMB++) {

            // code current macroblock only if it is not a hole 
			m_bSptMB_NOT_HOLE	 = 	bSptMB_NOT_HOLE;
			if (!m_bSptMB_NOT_HOLE)		//low latency
				goto EOMB1;

#ifdef __TRACE_AND_STATS_
			m_statsMB.reset ();
			m_pbitstrmOut->trace (CSite (iMBX, iMBY), "MB_X_Y");
#endif // __TRACE_AND_STATS_

            pmbmd->m_intStepDelta = 0;
			// MB rate control
			if (m_uiRateControl>=RC_TM5) {
				pmbmd->m_intStepDelta = m_tm5rc.tm5rc_calc_mquant(iMB,
				 m_statsVOP.total()) - iQPPrev;
				if (pmbmd->m_intStepDelta>2) pmbmd->m_intStepDelta = 2;
				else if (pmbmd->m_intStepDelta<-2) pmbmd->m_intStepDelta = -2;
			}
#ifdef _MBQP_CHANGE_
			iIndexofQ = (iIndexofQ + 1) % 4;
			pmbmd->m_intStepDelta = rgiQ [iIndexofQ];
#endif //_MBQP_CHANGE_
			pmbmd->m_bSkip = FALSE;			//reset for direct mode
			pmbmd->m_stepSize = iQPPrev + pmbmd->m_intStepDelta;
/* NBIT: change 31 to iMaxQP
			assert (pmbmd->m_stepSize <= 31 && pmbmd->m_stepSize > 0);
*/
			assert (pmbmd->m_stepSize <= iMaxQP && pmbmd->m_stepSize > 0);
			if ( m_volmd.bVPBitTh >= 0) { // modified by Sharp (98/4/13)
				Int iCounter = m_pbitstrmOut -> getCounter();
				if( iCounter - m_iVPCounter > m_volmd.bVPBitTh ) {
					pmbmd->m_intStepDelta = 0;	// reset DQ to use QP in VP header
					codeVideoPacketHeader (iMBX, iMBY, pmbmd->m_stepSize);	// video packet header
					m_iVPCounter = iCounter;
					bRestartDelayedQP = TRUE;
				}
			}
// End Toshiba(1998-1-16)

			if(bRestartDelayedQP)
			{
				pmbmd->m_stepSizeDelayed = pmbmd->m_stepSize;
				bRestartDelayedQP = FALSE;
			}
			else
				pmbmd->m_stepSizeDelayed = iQPPrev;

			iQPPrev = pmbmd->m_stepSize;
			if (pmbmd->m_intStepDelta == 0)
				pmbmd->m_dctMd = INTRA;
			else
				pmbmd->m_dctMd = INTRAQ;
			pmbmd->m_bFieldMV = 0;

			copyToCurrBuff (ppxlcOrigMBY, ppxlcOrigMBU, ppxlcOrigMBV, m_iFrameWidthY, m_iFrameWidthUV); 
			quantizeTextureIntraMB (iMBX, iMBY, pmbmd, ppxlcRefMBY, ppxlcRefMBU, ppxlcRefMBV, NULL);
			codeMBTextureHeadOfIVOP (pmbmd);
			sendDCTCoefOfIntraMBTexture (pmbmd);

EOMB1:
			pmbmd++;
#ifdef __TRACE_AND_STATS_
			m_statsVOP   += m_statsMB;
#endif // __TRACE_AND_STATS_
			ppxlcRefMBY  += MB_SIZE;
			ppxlcRefMBU  += BLOCK_SIZE;
			ppxlcRefMBV  += BLOCK_SIZE;
			ppxlcOrigMBY += MB_SIZE;
			ppxlcOrigMBU += BLOCK_SIZE;
			ppxlcOrigMBV += BLOCK_SIZE;

			if (m_uiSprite == 1 && m_sptMode != BASIC_SPRITE)	{
				bSptMB_NOT_HOLE = SptPieceMB_NOT_HOLE(iMBX+1, iMBY, pmbmd);
				m_iNumSptMB++ ;
			}
		}
		
		MacroBlockMemory** ppmbmTemp = m_rgpmbmAbove;
		m_rgpmbmAbove = m_rgpmbmCurr;
		
		// dshu: begin of modification
 		if (m_uiSprite == 1 && m_sptMode != BASIC_SPRITE)
			SaveMBmCurrRow (iMBY, m_rgpmbmCurr);		 //	  save current row pointed by *m_rgpmbmCurr 
		// dshu: end of modification

		m_rgpmbmCurr  = ppmbmTemp;
		ppxlcRefY  += m_iFrameWidthYxMBSize;
		ppxlcRefU  += m_iFrameWidthUVxBlkSize;
		ppxlcRefV  += m_iFrameWidthUVxBlkSize;
		ppxlcOrigY += m_iFrameWidthYxMBSize;
		ppxlcOrigU += m_iFrameWidthUVxBlkSize;
		ppxlcOrigV += m_iFrameWidthUVxBlkSize;
	}
}

Void CVideoObjectEncoder::encodeNSForIVOP_WithShape ()
{
	//in case the IVOP is used as an ref for direct mode
	memset (m_rgmv, 0, m_iNumMB * PVOP_MV_PER_REF_PER_MB * sizeof (CMotionVector));
	memset (m_rgmvBY, 0, m_iNumMB * sizeof (CMotionVector));

	Int iMBX, iMBY, iMB = 0;
	Int iMaxQP = (1<<m_volmd.uiQuantPrecision)-1; // NBIT
	CMBMode* pmbmd = m_rgmbmd;
	// Added for field based MC padding by Hyundai(1998-5-9)
        CMBMode* field_pmbmd = m_rgmbmd;
	// End of Hyundai(1998-5-9)
	Int iQPPrev = m_vopmd.intStepI;	//initialization
	Int iQPPrevAlpha = m_vopmd.intStepIAlpha;
	if (m_uiRateControl>=RC_TM5) iQPPrev = m_tm5rc.tm5rc_start_mb();

	PixelC* ppxlcRefY  = (PixelC*) m_pvopcRefQ1->pixelsY () + m_iStartInRefToCurrRctY;
	PixelC* ppxlcRefU  = (PixelC*) m_pvopcRefQ1->pixelsU () + m_iStartInRefToCurrRctUV;
	PixelC* ppxlcRefV  = (PixelC*) m_pvopcRefQ1->pixelsV () + m_iStartInRefToCurrRctUV;
	PixelC* ppxlcRefBY = (PixelC*) m_pvopcRefQ1->pixelsBY () + m_iStartInRefToCurrRctY;
	PixelC* ppxlcRefBUV = (PixelC*) m_pvopcRefQ1->pixelsBUV () + m_iStartInRefToCurrRctUV;
	PixelC *ppxlcRefA = NULL, *ppxlcOrigA = NULL;
	
	PixelC* ppxlcOrigY = (PixelC*) m_pvopcOrig->pixelsBoundY ();
	PixelC* ppxlcOrigU = (PixelC*) m_pvopcOrig->pixelsBoundU ();
	PixelC* ppxlcOrigV = (PixelC*) m_pvopcOrig->pixelsBoundV ();
	PixelC* ppxlcOrigBY = (PixelC*) m_pvopcOrig->pixelsBoundBY ();

	if (m_volmd.fAUsage == EIGHT_BIT) {
		ppxlcRefA = (PixelC*) m_pvopcRefQ1->pixelsA () + m_iStartInRefToCurrRctY;
		ppxlcOrigA = (PixelC*) m_pvopcOrig->pixelsBoundA ();
	}

	Bool bRestartDelayedQP = TRUE;

	for (iMBY = 0; iMBY < m_iNumMBY; iMBY++) {
		PixelC* ppxlcRefMBY  = ppxlcRefY;
		PixelC* ppxlcRefMBU  = ppxlcRefU;
		PixelC* ppxlcRefMBV  = ppxlcRefV;
		PixelC* ppxlcRefMBBY = ppxlcRefBY;
		PixelC* ppxlcRefMBBUV = ppxlcRefBUV;
		PixelC* ppxlcRefMBA  = ppxlcRefA;
		PixelC* ppxlcOrigMBY = ppxlcOrigY;
		PixelC* ppxlcOrigMBU = ppxlcOrigU;
		PixelC* ppxlcOrigMBV = ppxlcOrigV;
		PixelC* ppxlcOrigMBBY = ppxlcOrigBY;
		PixelC* ppxlcOrigMBA = ppxlcOrigA;

	    // In a given Sprite object piece, identify whether current macroblock is not a hole and should be coded ?
		Bool  bSptMB_NOT_HOLE= TRUE;
		if (m_uiSprite == 1 && m_sptMode != BASIC_SPRITE && m_vopmd.SpriteXmitMode != STOP) {
			bSptMB_NOT_HOLE = SptPieceMB_NOT_HOLE(0, iMBY, pmbmd);	
			RestoreMBmCurrRow (iMBY, m_rgpmbmCurr);	
		}

		for (iMBX = 0; iMBX < m_iNumMBX; iMBX++, iMB++) {
		  // code current macroblock only if it is not a hole 
			m_bSptMB_NOT_HOLE	 = 	bSptMB_NOT_HOLE;
			if (!m_bSptMB_NOT_HOLE) 
				goto EOMB2;

			// MB rate control
			if (m_uiRateControl>=RC_TM5) {
				pmbmd->m_intStepDelta = m_tm5rc.tm5rc_calc_mquant(iMB,
				 m_statsVOP.total()) - iQPPrev;
				if (pmbmd->m_intStepDelta>2) pmbmd->m_intStepDelta = 2;
				else if (pmbmd->m_intStepDelta<-2) pmbmd->m_intStepDelta = -2;
			}

            // Added for error resilient mode by Toshiba(1997-11-14)
			pmbmd->m_stepSize = iQPPrev + pmbmd->m_intStepDelta;
			if ( m_volmd.bVPBitTh >= 0) {
				Int iCounter = m_pbitstrmOut -> getCounter();
				if( iCounter - m_iVPCounter > m_volmd.bVPBitTh ) {
					codeVideoPacketHeader (iMBX, iMBY, pmbmd->m_stepSize);	// video packet header
					m_iVPCounter = iCounter;
					bRestartDelayedQP = TRUE;
				}
			}
            // End Toshiba(1997-11-14)
			
#ifdef __TRACE_AND_STATS_
			m_statsMB.reset ();
			m_pbitstrmOut->trace (CSite (iMBX, iMBY), "MB_X_Y");
#endif // __TRACE_AND_STATS_

			pmbmd->m_bSkip = FALSE;	//reset for direct mode 
			pmbmd->m_bPadded=FALSE;
			copyToCurrBuffWithShape (
				ppxlcOrigMBY, ppxlcOrigMBU, ppxlcOrigMBV, 
				ppxlcOrigMBBY, ppxlcOrigMBA,
				m_iFrameWidthY, m_iFrameWidthUV
			);
			downSampleBY (m_ppxlcCurrMBBY, m_ppxlcCurrMBBUV); // downsample original BY now for LPE padding (using original shape)
			decideTransparencyStatus (pmbmd, m_ppxlcCurrMBBY);
			if (pmbmd -> m_rgTranspStatus [0] == PARTIAL) {
				LPEPadding (pmbmd);
				m_statsMB.nBitsShape += codeIntraShape (ppxlcRefMBBY, pmbmd, iMBX, iMBY);
				downSampleBY (m_ppxlcCurrMBBY, m_ppxlcCurrMBBUV);
				decideTransparencyStatus (pmbmd, m_ppxlcCurrMBBY); // need to modify it a little (NONE block won't change)
			}
			else
				m_statsMB.nBitsShape += codeIntraShape (ppxlcRefMBBY, pmbmd, iMBX, iMBY);
			/*BBM// Added for Boundary by Hyundai(1998-5-9)
			if (m_vopmd.bInterlace) initMergedMode (pmbmd);
			// End of Hyundai(1998-5-9)*/
			if(m_volmd.bShapeOnly == FALSE) {// shape only mode
				pmbmd->m_stepSizeDelayed = iQPPrev;

				if (pmbmd -> m_rgTranspStatus [0] != ALL) {
					pmbmd->m_stepSize = iQPPrev + pmbmd->m_intStepDelta;
					assert (pmbmd->m_stepSize <= iMaxQP && pmbmd->m_stepSize > 0);
					
					if(bRestartDelayedQP)
					{
						pmbmd->m_stepSizeDelayed = pmbmd->m_stepSize;
						bRestartDelayedQP = FALSE;
					}
					
					iQPPrev = pmbmd->m_stepSize;
					if (pmbmd->m_intStepDelta == 0)
						pmbmd->m_dctMd = INTRA;
					else
						pmbmd->m_dctMd = INTRAQ;
					if (m_volmd.fAUsage == EIGHT_BIT) {
						// update alpha quant
						if(!m_volmd.bNoGrayQuantUpdate)
						{
							iQPPrevAlpha = (iQPPrev * m_vopmd.intStepIAlpha) / m_vopmd.intStepI;
							if(iQPPrevAlpha<1)
								iQPPrevAlpha=1;
						}
						pmbmd->m_stepSizeAlpha = iQPPrevAlpha;
					}
					/*BBM// Added for Boundary by Hyundai(1998-5-9)
					if (m_vopmd.bInterlace && pmbmd->m_rgTranspStatus[0] == PARTIAL)
						boundaryMacroBlockMerge (pmbmd);
					// End of Hyundai(1998-5-9)*/
					quantizeTextureIntraMB (iMBX, iMBY, pmbmd, ppxlcRefMBY, ppxlcRefMBU, ppxlcRefMBV, ppxlcRefMBA);
					codeMBTextureHeadOfIVOP (pmbmd);
					sendDCTCoefOfIntraMBTexture (pmbmd);
					if (m_volmd.fAUsage == EIGHT_BIT) {
						codeMBAlphaHeadOfIVOP (pmbmd);
						sendDCTCoefOfIntraMBAlpha (pmbmd);
					}
					/*BBM// Added for Boundary by Hyundai(1998-5-9)
                    if (m_vopmd.bInterlace && pmbmd->m_bMerged[0])
                            mergedMacroBlockSplit (pmbmd, ppxlcRefMBY, ppxlcRefMBA);
					// End of Hyundai(1998-5-9)*/
					// MC padding
					// Added for field based MC padding by Hyundai(1998-5-9)
					if (!m_vopmd.bInterlace) {
						if (pmbmd -> m_rgTranspStatus [0] == PARTIAL)
							mcPadCurrMB (ppxlcRefMBY, ppxlcRefMBU, ppxlcRefMBV, ppxlcRefMBA);
						padNeighborTranspMBs (
							iMBX, iMBY,
							pmbmd,
							ppxlcRefMBY, ppxlcRefMBU, ppxlcRefMBV, ppxlcRefMBA
						);
					}
					// End of Hyundai(1998-5-9)
				}
				else {
					// Added for field based MC padding by Hyundai(1998-5-9)
					if (!m_vopmd.bInterlace) {
						padCurrAndTopTranspMBFromNeighbor (
							iMBX, iMBY,
							pmbmd,
							ppxlcRefMBY, ppxlcRefMBU, ppxlcRefMBV, ppxlcRefMBA
						);
					}
					// End of Hyundai(1998-5-9)
				}
			}

EOMB2:
	    	ppxlcRefMBA += MB_SIZE;
			ppxlcOrigMBBY += MB_SIZE;
			ppxlcOrigMBA += MB_SIZE;
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

			// Identify whether next Sprite object macroblock is not a hole and should be coded ?
			if (m_uiSprite == 1 && m_sptMode != BASIC_SPRITE)
				bSptMB_NOT_HOLE = SptPieceMB_NOT_HOLE(iMBX+1, iMBY, pmbmd);
		}
		
		MacroBlockMemory** ppmbmTemp = m_rgpmbmAbove;
		m_rgpmbmAbove = m_rgpmbmCurr;
		// dshu: begin of modification
 		if (m_uiSprite == 1 && m_sptMode != BASIC_SPRITE)
			SaveMBmCurrRow (iMBY, m_rgpmbmCurr);		 //	  save current row pointed by *m_rgpmbmCurr 
		// dshu: end of modification		
		m_rgpmbmCurr  = ppmbmTemp;
		ppxlcRefY  += m_iFrameWidthYxMBSize;
		ppxlcRefA  += m_iFrameWidthYxMBSize;
		ppxlcRefU  += m_iFrameWidthUVxBlkSize;
		ppxlcRefV  += m_iFrameWidthUVxBlkSize;
		ppxlcRefBY += m_iFrameWidthYxMBSize;
		ppxlcRefBUV += m_iFrameWidthUVxBlkSize;

		ppxlcOrigY += m_iFrameWidthYxMBSize;
		ppxlcOrigBY += m_iFrameWidthYxMBSize;
		ppxlcOrigA += m_iFrameWidthYxMBSize;
		ppxlcOrigU += m_iFrameWidthUVxBlkSize;
		ppxlcOrigV += m_iFrameWidthUVxBlkSize;
	}
	// Added for field based MC padding by Hyundai(1998-5-9)
        if (m_vopmd.bInterlace && m_volmd.bShapeOnly == FALSE)
                fieldBasedMCPadding (field_pmbmd, m_pvopcRefQ1);
	// End of Hyundai(1998-5-9)
}

Void CVideoObjectEncoder::encodeNSForPVOP ()	
{
    if (m_uiSprite == 0)	//low latency: not motion esti
	    motionEstPVOP ();

	// Rate Control
	if (m_uiRateControl==RC_MPEG4) {
		Double Ec = m_iMAD / (Double) (m_iNumMBY * m_iNumMBX * 16 * 16);
		m_statRC.setMad (Ec);
		if (m_statRC.noCodedFrame () >= RC_START_RATE_CONTROL) {
			UInt newQStep = m_statRC.updateQuanStepsize ();	
			m_vopmd.intStepDiff = newQStep - m_vopmd.intStep;	// DQUANT
			m_vopmd.intStep = newQStep;
		}
		cout << "\t" << "Q:" << "\t\t\t" << m_vopmd.intStep << "\n";
		m_statRC.setQc (m_vopmd.intStep);
	}

	// MB rate control
	//Int iIndexofQ = 0;
	//Int rgiQ [4] = {-1, -2, 1, 2};
	// -----

	Int iMBX, iMBY, iMB = 0;
	Int iMaxQP = (1<<m_volmd.uiQuantPrecision)-1; // NBIT
	CoordI y = 0; 
	CMBMode* pmbmd = m_rgmbmd;
	Int iQPPrev = m_vopmd.intStep;
	if (m_uiRateControl>=RC_TM5) iQPPrev = m_tm5rc.tm5rc_start_mb();
	CMotionVector* pmv = m_rgmv;
	PixelC* ppxlcRefY = (PixelC*) m_pvopcRefQ1->pixelsY () + m_iStartInRefToCurrRctY;
	PixelC* ppxlcRefU = (PixelC*) m_pvopcRefQ1->pixelsU () + m_iStartInRefToCurrRctUV;
	PixelC* ppxlcRefV = (PixelC*) m_pvopcRefQ1->pixelsV () + m_iStartInRefToCurrRctUV;
	
	PixelC* ppxlcOrigY = (PixelC*) m_pvopcOrig->pixelsBoundY ();
	PixelC* ppxlcOrigU = (PixelC*) m_pvopcOrig->pixelsBoundU ();
	PixelC* ppxlcOrigV = (PixelC*) m_pvopcOrig->pixelsBoundV ();

	Bool bRestartDelayedQP = TRUE;

	for (iMBY = 0; iMBY < m_iNumMBY; iMBY++, y += MB_SIZE) {
		PixelC* ppxlcRefMBY = ppxlcRefY;
		PixelC* ppxlcRefMBU = ppxlcRefU;
		PixelC* ppxlcRefMBV = ppxlcRefV;
		PixelC* ppxlcOrigMBY = ppxlcOrigY;
		PixelC* ppxlcOrigMBU = ppxlcOrigU;
		PixelC* ppxlcOrigMBV = ppxlcOrigV;
		CoordI x = 0; 

		// In a given Sprite update piece, identify whether current macroblock is not a hole and should be coded ?
		Bool  bSptMB_NOT_HOLE= TRUE;
		if (m_uiSprite == 1 && m_vopmd.SpriteXmitMode != STOP) {
			bSptMB_NOT_HOLE = SptUpdateMB_NOT_HOLE(0, iMBY, pmbmd);
			m_statsMB.nBitsShape = 0;		 	
		}

		for (iMBX = 0; iMBX < m_iNumMBX; iMBX++, x += MB_SIZE, iMB++)	{

            // skip current Sprite update macroblock if it is transparent
			if (m_uiSprite == 1 && m_vopmd.SpriteXmitMode != STOP && pmbmd -> m_rgTranspStatus [0] == ALL)
				goto EOMB3;
			else if (m_uiSprite == 1)	{
				m_bSptMB_NOT_HOLE = bSptMB_NOT_HOLE;
				pmv -> setToZero ();
				pmbmd->m_bhas4MVForward = FALSE ;
				pmbmd -> m_dctMd = INTER ;
			}

#ifdef __TRACE_AND_STATS_
			m_pbitstrmOut->trace (CSite (iMBX, iMBY), "MB_X_Y");
			m_statsMB.reset ();
#endif // __TRACE_AND_STATS_

			pmbmd->m_intStepDelta = 0;
			// MB rate control
			if (m_uiRateControl>=RC_TM5) {
				pmbmd->m_intStepDelta = m_tm5rc.tm5rc_calc_mquant(iMB,
				 m_statsVOP.total()) - iQPPrev;
				if (pmbmd->m_intStepDelta>2) pmbmd->m_intStepDelta = 2;
				else if (pmbmd->m_intStepDelta<-2) pmbmd->m_intStepDelta = -2;
			}
#ifdef _MBQP_CHANGE_
			if (!pmbmd ->m_bhas4MVForward && !pmbmd ->m_bSkip) {
				iIndexofQ = (iIndexofQ + 1) % 4;
				pmbmd->m_intStepDelta = rgiQ [iIndexofQ];
				Int iQuantMax = (1<<m_volmd.uiQuantPrecision) - 1;
				if ((iQPPrev + pmbmd->m_intStepDelta) > iQuantMax) 
					pmbmd->m_intStepDelta = 0;
				if ((iQPPrev + pmbmd->m_intStepDelta) <= 0)
					pmbmd->m_intStepDelta = 0;
				if (pmbmd->m_intStepDelta != 0) {
					if (pmbmd->m_dctMd == INTRA)
						pmbmd->m_dctMd = INTRAQ;
					else if (pmbmd ->m_dctMd == INTER)
						pmbmd->m_dctMd = INTERQ;
				}
			}
			// -----
#endif //_MBQP_CHANGE_
			pmbmd->m_stepSize = iQPPrev + pmbmd->m_intStepDelta;
/* NBIT: change 31 to iMaxQP
			assert (pmbmd->m_stepSize <= 31 && pmbmd->m_stepSize > 0);
*/
			assert (pmbmd->m_stepSize <= iMaxQP && pmbmd->m_stepSize > 0);

			if ( m_volmd.bVPBitTh >= 0) {
				Int iCounter = m_pbitstrmOut -> getCounter();
				if( iCounter - m_iVPCounter > m_volmd.bVPBitTh ) {
					// reset DQ to use QP in VP header
					pmbmd->m_intStepDelta = 0;
					if (pmbmd->m_dctMd == INTRAQ)
							pmbmd->m_dctMd = INTRA;
					else if (pmbmd ->m_dctMd == INTERQ)
						pmbmd->m_dctMd = INTER;
					iQPPrev = pmbmd->m_stepSize;
					codeVideoPacketHeader (iMBX, iMBY, pmbmd->m_stepSize);	// video packet header
					m_iVPCounter = iCounter;
					bRestartDelayedQP = TRUE;
				}
			}

			if(bRestartDelayedQP)
				pmbmd->m_stepSizeDelayed = pmbmd->m_stepSize;
			else
				pmbmd->m_stepSizeDelayed = iQPPrev;

			copyToCurrBuff (ppxlcOrigMBY, ppxlcOrigMBU, ppxlcOrigMBV, m_iFrameWidthY, m_iFrameWidthUV); 
			encodePVOPMB (
				ppxlcRefMBY, ppxlcRefMBU, ppxlcRefMBV,
				pmbmd, pmv,
				iMBX, iMBY,
				x, y
			);
			if (!pmbmd->m_bSkip)
			{
				iQPPrev = pmbmd->m_stepSize;
				bRestartDelayedQP = FALSE;
			}

EOMB3:
			pmbmd++;
			pmv += PVOP_MV_PER_REF_PER_MB;
#ifdef __TRACE_AND_STATS_
			m_statsVOP += m_statsMB;
#endif // __TRACE_AND_STATS_
			ppxlcRefMBY += MB_SIZE;
			ppxlcRefMBU += BLOCK_SIZE;
			ppxlcRefMBV += BLOCK_SIZE;
			ppxlcOrigMBY += MB_SIZE;
			ppxlcOrigMBU += BLOCK_SIZE;
			ppxlcOrigMBV += BLOCK_SIZE;
			// Identify whether next Sprite update macroblock is not a hole and should be coded ?
			if (m_uiSprite == 1 && m_vopmd.SpriteXmitMode != STOP && iMBX < (m_iNumMBX-1))
				bSptMB_NOT_HOLE = SptUpdateMB_NOT_HOLE(iMBX+1, iMBY, pmbmd);   
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
}

Void CVideoObjectEncoder::encodeNSForPVOP_WithShape ()
{
	Int iMBX, iMBY, iMB = 0;

	motionEstPVOP_WithShape ();

	// shape bitstream is set to shape cache
	m_pbitstrmShapeMBOut = m_pbitstrmShape;

	// Rate Control
	if (m_uiRateControl==RC_MPEG4) {
		Double Ec = m_iMAD / (Double) (m_iNumMBY * m_iNumMBX * 16 * 16);
		m_statRC.setMad (Ec);
		if (m_statRC.noCodedFrame () >= RC_START_RATE_CONTROL) {
			UInt newQStep = m_statRC.updateQuanStepsize ();	
			m_vopmd.intStepDiff = newQStep - m_vopmd.intStep;	// DQUANT
			m_vopmd.intStep = newQStep;
		}
		cout << "\t" << "Q:" << "\t\t\t" << m_vopmd.intStep << "\n";
		m_statRC.setQc (m_vopmd.intStep);
	}

	CoordI y = m_rctCurrVOPY.top; 
	CMBMode* pmbmd = m_rgmbmd;
	// Added for field based MC padding by Hyundai(1998-5-9)
	CMBMode* field_pmbmd = m_rgmbmd;
	// End of Hyundai(1998-5-9)

	Int iQPPrev = m_vopmd.intStep;
	Int iQPPrevAlpha = m_vopmd.intStepPAlpha;
	if (m_uiRateControl>=RC_TM5) iQPPrev = m_tm5rc.tm5rc_start_mb();

	CMotionVector* pmv = m_rgmv;
	CMotionVector* pmvBY = m_rgmvBY;
	PixelC* ppxlcRefY = (PixelC*) m_pvopcRefQ1->pixelsY () + m_iStartInRefToCurrRctY;
	PixelC* ppxlcRefU = (PixelC*) m_pvopcRefQ1->pixelsU () + m_iStartInRefToCurrRctUV;
	PixelC* ppxlcRefV = (PixelC*) m_pvopcRefQ1->pixelsV () + m_iStartInRefToCurrRctUV;
	PixelC* ppxlcRefBY = (PixelC*) m_pvopcRefQ1->pixelsBY () + m_iStartInRefToCurrRctY;
	PixelC *ppxlcRefA = NULL, * ppxlcOrigA = NULL;
	
	PixelC* ppxlcOrigY = (PixelC*) m_pvopcOrig->pixelsBoundY ();
	PixelC* ppxlcOrigU = (PixelC*) m_pvopcOrig->pixelsBoundU ();
	PixelC* ppxlcOrigV = (PixelC*) m_pvopcOrig->pixelsBoundV ();
	PixelC* ppxlcOrigBY = (PixelC*) m_pvopcOrig->pixelsBoundBY ();

	if (m_volmd.fAUsage == EIGHT_BIT) {
		ppxlcRefA = (PixelC*) m_pvopcRefQ1->pixelsA () + m_iStartInRefToCurrRctY;
		ppxlcOrigA = (PixelC*) m_pvopcOrig->pixelsBoundA ();
	}

	Bool bRestartDelayedQP = TRUE;

// Added for error resilient mode by Toshiba(1997-11-14)
	Bool bCodeVPHeaderNext = FALSE;		// needed only for OBMC
	Int	iTempVPMBnum = 0;
// End Toshiba(1997-11-14)
	for (iMBY = 0; iMBY < m_iNumMBY; iMBY++, y += MB_SIZE) {
		PixelC* ppxlcRefMBY = ppxlcRefY;
		PixelC* ppxlcRefMBU = ppxlcRefU;
		PixelC* ppxlcRefMBV = ppxlcRefV;
		PixelC* ppxlcRefMBBY = ppxlcRefBY;
		PixelC* ppxlcRefMBA = ppxlcRefA;
		PixelC* ppxlcOrigMBY = ppxlcOrigY;
		PixelC* ppxlcOrigMBU = ppxlcOrigU;
		PixelC* ppxlcOrigMBV = ppxlcOrigV;
		PixelC* ppxlcOrigMBBY = ppxlcOrigBY;
		PixelC* ppxlcOrigMBA = ppxlcOrigA;
		CoordI x = m_rctCurrVOPY.left;

#ifdef __TRACE_AND_STATS_
		m_statsMB.reset ();
#endif // __TRACE_AND_STATS_

		// initiate advance shape coding
        // Added for error resilient mode by Toshiba(1997-11-14)
		// The following operation is needed only for OBMC
		if ( m_volmd.bVPBitTh >= 0) {
			Int iCounter = m_pbitstrmOut -> getCounter();
			bCodeVPHeaderNext = iCounter - m_iVPCounter > m_volmd.bVPBitTh;
			if( bCodeVPHeaderNext ) {
				iTempVPMBnum  = m_iVPMBnum;
				m_iVPMBnum = VPMBnum(0, iMBY);
				m_iVPCounter = iCounter;
			}
		} else {
			bCodeVPHeaderNext = FALSE;
		}
// End Toshiba(1997-11-14)
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
		// End Toshiba(1997-11-14)
        // Added for error resilient mode by Toshiba(1997-11-14)
		// The following operation is needed only for OBMC
			if( bCodeVPHeaderNext )
				m_iVPMBnum = iTempVPMBnum;
        // End Toshiba(1997-11-14)
		
		if(pmbmd->m_bhas4MVForward)
			padMotionVectors(pmbmd,pmv);

		for (iMBX = 0; iMBX < m_iNumMBX; iMBX++, x += MB_SIZE, iMB++)	{
			// MB rate control
			if (m_uiRateControl>=RC_TM5) {
				pmbmd->m_intStepDelta = m_tm5rc.tm5rc_calc_mquant(iMB,
				 m_statsVOP.total()) - iQPPrev;
				if (pmbmd->m_intStepDelta>2) pmbmd->m_intStepDelta = 2;
				else if (pmbmd->m_intStepDelta<-2) pmbmd->m_intStepDelta = -2;
			}
            // Added for error resilient mode by Toshiba(1997-11-14)
			pmbmd->m_stepSize = iQPPrev + pmbmd->m_intStepDelta;
			if ( m_volmd.bVPBitTh >= 0) {
				if( bCodeVPHeaderNext ) {
					codeVideoPacketHeader (iMBX, iMBY, pmbmd->m_stepSize);	// video packet header
					bRestartDelayedQP = TRUE;
				}
			} else {
				bCodeVPHeaderNext = FALSE;
			}
            // End Toshiba(1997-11-14)
			
#ifdef __TRACE_AND_STATS_
			m_pbitstrmOut->trace (CSite (iMBX, iMBY), "MB_X_Y (Texture)");
			// shape quantization part
			m_statsMB.reset ();
#endif // __TRACE_AND_STATS_

			pmbmd->m_bPadded=FALSE;

			dumpCachedShapeBits();

			if(iMBX<m_iNumMBX-1)
			{
                // Added for error resilient mode by Toshiba(1997-11-14)
				// The following operation is needed only for OBMC
				if ( m_volmd.bVPBitTh >= 0) {
					Int iCounter = m_pbitstrmOut -> getCounter();
					bCodeVPHeaderNext = iCounter - m_iVPCounter > m_volmd.bVPBitTh;
					if( bCodeVPHeaderNext ) {
						iTempVPMBnum  = m_iVPMBnum;
						m_iVPMBnum = VPMBnum(iMBX+1, iMBY);
						m_iVPCounter = iCounter;
					}
				} else {
					bCodeVPHeaderNext = FALSE;
				}
                // End Toshiba(1997-11-14)
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
						 shpmdColocatedMB, pmv + PVOP_MV_PER_REF_PER_MB,
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
			    // End Toshiba(1997-11-14)

                // Added for error resilient mode by Toshiba(1997-11-14)
				// The following operation is needed only for OBMC
				if( bCodeVPHeaderNext )
					m_iVPMBnum = iTempVPMBnum;
                // End Toshiba(1997-11-14)
				// shape needs padded mvs ready for next mb
				if((pmbmd+1)->m_bhas4MVForward)
					padMotionVectors(pmbmd+1, pmv + PVOP_MV_PER_REF_PER_MB);
			}
			/*BBM// Added for Boundary by Hyundai(1998-5-9)
			if (m_vopmd.bInterlace) initMergedMode (pmbmd);
			// End of Hyundai(1998-5-9)*/
			if(m_volmd.bShapeOnly==FALSE)
			{
				pmbmd->m_stepSizeDelayed = iQPPrev;

				if (pmbmd -> m_rgTranspStatus [0] != ALL) {
					// need to copy binary shape too since curr buff is future shape
					copyToCurrBuffWithShape(ppxlcOrigMBY, ppxlcOrigMBU, ppxlcOrigMBV,
						ppxlcRefMBBY, ppxlcOrigMBA, m_iFrameWidthY, m_iFrameWidthUV);
					downSampleBY (m_ppxlcCurrMBBY, m_ppxlcCurrMBBUV);
					encodePVOPMBTextureWithShape(ppxlcRefMBY, ppxlcRefMBU, ppxlcRefMBV,
						ppxlcRefMBA, pmbmd, pmv, iMBX, iMBY, x, y, iQPPrev, iQPPrevAlpha,
						bRestartDelayedQP);
					// Added for field based MC padding by Hyundai(1998-5-9)
					if (!m_vopmd.bInterlace) { 
						if (pmbmd -> m_rgTranspStatus [0] == PARTIAL)
							mcPadCurrMB (ppxlcRefMBY, ppxlcRefMBU, ppxlcRefMBV, ppxlcRefMBA);
						padNeighborTranspMBs (
							iMBX, iMBY,
							pmbmd,
							ppxlcRefMBY, ppxlcRefMBU, ppxlcRefMBV, ppxlcRefMBA
						);
					}
					// End of Hyundai(1998-5-9)
				}
				else {
					// Added for field based MC padding by Hyundai(1998-5-9)
					if (!m_vopmd.bInterlace) { 
						padCurrAndTopTranspMBFromNeighbor (
							iMBX, iMBY,
							pmbmd,
							ppxlcRefMBY, ppxlcRefMBU, ppxlcRefMBV, ppxlcRefMBA
						);
					}
					// End of Hyundai(1998-5-9)
				}
			}

			pmbmd++;
			pmv += PVOP_MV_PER_REF_PER_MB;
			pmvBY++;
			ppxlcRefMBBY += MB_SIZE;
			ppxlcRefMBA += MB_SIZE;
			ppxlcOrigMBBY += MB_SIZE;
			ppxlcOrigMBA += MB_SIZE;
#ifdef __TRACE_AND_STATS_
			m_statsVOP += m_statsMB;
#endif // __TRACE_AND_STATS_
			ppxlcRefMBY += MB_SIZE;
			ppxlcRefMBU += BLOCK_SIZE;
			ppxlcRefMBV += BLOCK_SIZE;
			ppxlcOrigMBY += MB_SIZE;
			ppxlcOrigMBU += BLOCK_SIZE;
			ppxlcOrigMBV += BLOCK_SIZE;
		}
		MacroBlockMemory** ppmbmTemp = m_rgpmbmAbove;
		m_rgpmbmAbove = m_rgpmbmCurr;
		m_rgpmbmCurr  = ppmbmTemp;

		ppxlcRefY += m_iFrameWidthYxMBSize;
		ppxlcRefU += m_iFrameWidthUVxBlkSize;
		ppxlcRefV += m_iFrameWidthUVxBlkSize;
		ppxlcRefBY += m_iFrameWidthYxMBSize;
		ppxlcRefA += m_iFrameWidthYxMBSize;
		
		ppxlcOrigY += m_iFrameWidthYxMBSize;
		ppxlcOrigBY += m_iFrameWidthYxMBSize;
		ppxlcOrigA += m_iFrameWidthYxMBSize;
		ppxlcOrigU += m_iFrameWidthUVxBlkSize;
		ppxlcOrigV += m_iFrameWidthUVxBlkSize;
	}

	// Added for field based MC padding by Hyundai(1998-5-9)
        if (m_vopmd.bInterlace && m_volmd.bShapeOnly == FALSE)
                fieldBasedMCPadding (field_pmbmd, m_pvopcRefQ1);
	// End of Hyundai(1998-5-9)

	// restore normal output stream
	m_pbitstrmShapeMBOut = m_pbitstrmOut;
}

Void CVideoObjectEncoder::encodeNSForBVOP ()
{
	motionEstBVOP ();
	
	CoordI y = 0; 
	CMBMode* pmbmd = m_rgmbmd;
	CMotionVector* pmv = m_rgmv;
	CMotionVector* pmvBackward = m_rgmvBackward;
	const CMBMode* pmbmdRef = m_rgmbmdRef;			//MB mode in ref frame for direct mode
	const CMotionVector* pmvRef = m_rgmvRef;		//MV in ref frame for direct mode
	PixelC* ppxlcCurrQY = (PixelC*) m_pvopcCurrQ->pixelsY () + m_iStartInRefToCurrRctY;
	PixelC* ppxlcCurrQU = (PixelC*) m_pvopcCurrQ->pixelsU () + m_iStartInRefToCurrRctUV;
	PixelC* ppxlcCurrQV = (PixelC*) m_pvopcCurrQ->pixelsV () + m_iStartInRefToCurrRctUV;
	
	PixelC* ppxlcOrigY = (PixelC*) m_pvopcOrig->pixelsBoundY ();
	PixelC* ppxlcOrigU = (PixelC*) m_pvopcOrig->pixelsBoundU ();
	PixelC* ppxlcOrigV = (PixelC*) m_pvopcOrig->pixelsBoundV ();

	// MB rate control
	//Int iIndexofQ = 0;
	//Int rgiQ [4] = {-2, 0, 0, 2};
	// -----
	
	Int iQPPrev = m_vopmd.intStepB;
	Int iMBX, iMBY, iMB = 0;
	Int iMaxQP = (1<<m_volmd.uiQuantPrecision)-1; // NBIT
	for (iMBY = 0; iMBY < m_iNumMBY; iMBY++, y += MB_SIZE) {
		PixelC* ppxlcCurrQMBY = ppxlcCurrQY;
		PixelC* ppxlcCurrQMBU = ppxlcCurrQU;
		PixelC* ppxlcCurrQMBV = ppxlcCurrQV;
		PixelC* ppxlcOrigMBY = ppxlcOrigY;
		PixelC* ppxlcOrigMBU = ppxlcOrigU;
		PixelC* ppxlcOrigMBV = ppxlcOrigV;
		CoordI x = 0;
		m_vctForwardMvPredBVOP[0].x  = m_vctForwardMvPredBVOP[0].y  = 0;
		m_vctBackwardMvPredBVOP[0].x = m_vctBackwardMvPredBVOP[0].y = 0;
		m_vctForwardMvPredBVOP[1].x  = m_vctForwardMvPredBVOP[1].y  = 0;
		m_vctBackwardMvPredBVOP[1].x = m_vctBackwardMvPredBVOP[1].y = 0;
		for (iMBX = 0; iMBX < m_iNumMBX; iMBX++, x += MB_SIZE, iMB++)	{
#ifdef __TRACE_AND_STATS_
			m_pbitstrmOut->trace (CSite (iMBX, iMBY), "MB_X_Y");
			// shape quantization part
			m_statsMB.reset ();
#endif // __TRACE_AND_STATS_

			pmbmd->m_intStepDelta = 0;
			// MB rate control
			if (m_uiRateControl>=RC_TM5) {
				int k = m_tm5rc.tm5rc_calc_mquant(iMB, m_statsVOP.total());
				// special case for BVOP copied from MoMuSys
				if (k > iQPPrev) {
					if (iQPPrev+2 <= iMaxQP) pmbmd->m_intStepDelta = 2;
				} else if (k < iQPPrev) {
					if (iQPPrev-2 > 0) pmbmd->m_intStepDelta = -2;
				}
			}
#ifdef _MBQP_CHANGE_
			if (!pmbmd->m_bColocatedMBSkip && pmbmd->m_mbType != DIRECT) {
				iIndexofQ = (iIndexofQ + 1) % 4;
				pmbmd->m_dctMd = INTERQ;
				pmbmd->m_intStepDelta = rgiQ [iIndexofQ];
				Int iQuantMax = (1<<m_volmd.uiQuantPrecision) - 1;
				if ((iQPPrev + pmbmd->m_intStepDelta) > iQuantMax)
					pmbmd->m_intStepDelta = 0;
				if ((iQPPrev + pmbmd->m_intStepDelta) <= 0)
					pmbmd->m_intStepDelta = 0;
			}
			// -----
#endif //_MBQP_CHANGE_

			pmbmd->m_stepSize = iQPPrev + pmbmd->m_intStepDelta;
/* NBIT: change 31 to iMaxQP
			assert (pmbmd->m_stepSize <= 31 && pmbmd->m_stepSize > 0);
*/
			assert (pmbmd->m_stepSize <= iMaxQP && pmbmd->m_stepSize > 0);

			if ( m_volmd.bVPBitTh >= 0) {
				Int iCounter = m_pbitstrmOut -> getCounter();
				if( iCounter - m_iVPCounter > m_volmd.bVPBitTh ) {
					// reset DQ to use QP in VP header
					pmbmd->m_intStepDelta = 0;
					if (pmbmd ->m_dctMd == INTERQ)
						pmbmd->m_dctMd = INTER;
					iQPPrev = pmbmd->m_stepSize;

					codeVideoPacketHeader (iMBX, iMBY, pmbmd->m_stepSize);	// video packet header
					m_iVPCounter = iCounter;
					//reset MV predictor
					m_vctForwardMvPredBVOP[0].x  = m_vctForwardMvPredBVOP[0].y  = 0;
					m_vctBackwardMvPredBVOP[0].x = m_vctBackwardMvPredBVOP[0].y = 0;
					m_vctForwardMvPredBVOP[1].x  = m_vctForwardMvPredBVOP[1].y  = 0;
					m_vctBackwardMvPredBVOP[1].x = m_vctBackwardMvPredBVOP[1].y = 0;

				}
			}

			if (pmbmd->m_bColocatedMBSkip &&
			(m_volmd.volType == BASE_LAYER || (m_volmd.volType == ENHN_LAYER && m_vopmd.iRefSelectCode == 3 && m_volmd.iEnhnType == 0)))
			{ // don't need to send any bit for this mode
				copyFromRefToCurrQ (
					m_pvopcRefQ0,
					x, y, 
					ppxlcCurrQMBY, ppxlcCurrQMBU, ppxlcCurrQMBV, NULL
				);
			}
			else {
				copyToCurrBuff (ppxlcOrigMBY, ppxlcOrigMBU, ppxlcOrigMBV, m_iFrameWidthY, m_iFrameWidthUV); 
				encodeBVOPMB (
					ppxlcCurrQMBY, ppxlcCurrQMBU, ppxlcCurrQMBV,
					pmbmd, pmv, pmvBackward,
					pmbmdRef, pmvRef,
					iMBX, iMBY,
					x, y
				);
				if (pmbmd->getCodedBlockPattern (U_BLOCK) ||
					pmbmd->getCodedBlockPattern (V_BLOCK) ||
					pmbmd->getCodedBlockPattern (Y_BLOCK1) ||
					pmbmd->getCodedBlockPattern (Y_BLOCK2) ||
					pmbmd->getCodedBlockPattern (Y_BLOCK3) ||
					pmbmd->getCodedBlockPattern (Y_BLOCK4))
					iQPPrev = pmbmd->m_stepSize;
			}

			pmbmd++;
			pmv         += BVOP_MV_PER_REF_PER_MB;
			pmvBackward += BVOP_MV_PER_REF_PER_MB;
			pmvRef      += PVOP_MV_PER_REF_PER_MB;
			pmbmdRef++;
			m_statsVOP += m_statsMB;
			ppxlcCurrQMBY += MB_SIZE;
			ppxlcCurrQMBU += BLOCK_SIZE;
			ppxlcCurrQMBV += BLOCK_SIZE;
			ppxlcOrigMBY += MB_SIZE;
			ppxlcOrigMBU += BLOCK_SIZE;
			ppxlcOrigMBV += BLOCK_SIZE;
		}
		ppxlcCurrQY += m_iFrameWidthYxMBSize;
		ppxlcCurrQU += m_iFrameWidthUVxBlkSize;
		ppxlcCurrQV += m_iFrameWidthUVxBlkSize;
		
		ppxlcOrigY += m_iFrameWidthYxMBSize;
		ppxlcOrigU += m_iFrameWidthUVxBlkSize;
		ppxlcOrigV += m_iFrameWidthUVxBlkSize;
	}
}

Void CVideoObjectEncoder::encodeNSForBVOP_WithShape ()
{
	motionEstBVOP_WithShape ();

	// decide prediction direction for shape
	if(m_bCodedFutureRef==FALSE)
		m_vopmd.fShapeBPredDir = B_FORWARD;
	else
	{
		if(m_tFutureRef - m_t >= m_t - m_tPastRef)
			m_vopmd.fShapeBPredDir = B_FORWARD;
		else
			m_vopmd.fShapeBPredDir = B_BACKWARD;
	}

	Int iQPPrev = m_vopmd.intStepB;
	Int iQPPrevAlpha = m_vopmd.intStepBAlpha;
	if (m_uiRateControl>=RC_TM5) iQPPrev = m_tm5rc.tm5rc_start_mb();

	CoordI y = m_rctCurrVOPY.top; 
	CMBMode*	   pmbmd = m_rgmbmd;
	// Added for field based MC padding by Hyundai(1998-5-9)
	CMBMode* field_pmbmd = m_rgmbmd;
	// End of Hyundai(1998-5-9)

	CMotionVector* pmv = m_rgmv;
	CMotionVector* pmvBackward = m_rgmvBackward;
	CMotionVector* pmvBY = m_rgmvBY;
	PixelC* ppxlcCurrQY = (PixelC*) m_pvopcCurrQ->pixelsY () + m_iStartInRefToCurrRctY;
	PixelC* ppxlcCurrQU = (PixelC*) m_pvopcCurrQ->pixelsU () + m_iStartInRefToCurrRctUV;
	PixelC* ppxlcCurrQV = (PixelC*) m_pvopcCurrQ->pixelsV () + m_iStartInRefToCurrRctUV;
	PixelC* ppxlcCurrQBY = (PixelC*) m_pvopcCurrQ->pixelsBY () + m_iStartInRefToCurrRctY;
	PixelC *ppxlcCurrQA = NULL, * ppxlcOrigA = NULL;
	
	PixelC* ppxlcOrigY = (PixelC*) m_pvopcOrig->pixelsBoundY ();
	PixelC* ppxlcOrigU = (PixelC*) m_pvopcOrig->pixelsBoundU ();
	PixelC* ppxlcOrigV = (PixelC*) m_pvopcOrig->pixelsBoundV ();
	PixelC* ppxlcOrigBY = (PixelC*) m_pvopcOrig->pixelsBoundBY ();

	if (m_volmd.fAUsage == EIGHT_BIT) {
		ppxlcCurrQA = (PixelC*) m_pvopcCurrQ->pixelsA () + m_iStartInRefToCurrRctY;
		ppxlcOrigA = (PixelC*) m_pvopcOrig->pixelsBoundA ();
	}
	Int iMBX, iMBY, iMB = 0;
	const CMBMode* pmbmdRef;
	const CMotionVector* pmvRef;
	for (iMBY = 0; iMBY < m_iNumMBY; iMBY++, y += MB_SIZE) {
		PixelC* ppxlcCurrQMBY = ppxlcCurrQY;
		PixelC* ppxlcCurrQMBU = ppxlcCurrQU;
		PixelC* ppxlcCurrQMBV = ppxlcCurrQV;
		PixelC* ppxlcCurrQMBBY = ppxlcCurrQBY;
		PixelC* ppxlcCurrQMBA = ppxlcCurrQA;
		PixelC* ppxlcOrigMBY = ppxlcOrigY;
		PixelC* ppxlcOrigMBU = ppxlcOrigU;
		PixelC* ppxlcOrigMBV = ppxlcOrigV;
		PixelC* ppxlcOrigMBBY = ppxlcOrigBY;
		PixelC* ppxlcOrigMBA = ppxlcOrigA;
		CoordI x = m_rctCurrVOPY.left;
		m_vctForwardMvPredBVOP[0].x  = m_vctForwardMvPredBVOP[0].y  = 0;
		m_vctBackwardMvPredBVOP[0].x = m_vctBackwardMvPredBVOP[0].y = 0;
		m_vctForwardMvPredBVOP[1].x  = m_vctForwardMvPredBVOP[1].y  = 0;
		m_vctBackwardMvPredBVOP[1].x = m_vctBackwardMvPredBVOP[1].y = 0;
		Int iMaxQP = (1<<m_volmd.uiQuantPrecision)-1; // NBIT
		for (iMBX = 0; iMBX < m_iNumMBX; iMBX++, x += MB_SIZE, iMB++)	{
			pmbmd->m_intStepDelta = 0;	//sharp added this
			pmbmd->m_bPadded=FALSE;
			// shape quantization part
			m_statsMB.reset ();

// Modified for error resilience mode by Toshiba(1998-1-16)
			pmbmd->m_stepSize = m_vopmd.intStepB;
			// MB rate control  -- should this be here??????
			if (m_uiRateControl>=RC_TM5) {
				int k = m_tm5rc.tm5rc_calc_mquant(iMB, m_statsVOP.total());
				// special case for BVOP copied from MoMuSys
				if (k > iQPPrev) {
					if (iQPPrev+2 <= iMaxQP) pmbmd->m_intStepDelta = 2;
				} else if (k < iQPPrev) {
					if (iQPPrev-2 > 0) pmbmd->m_intStepDelta = -2;
				}
			}
			if ( m_volmd.bVPBitTh >= 0) {
				Int iCounter = m_pbitstrmOut -> getCounter();
				if( iCounter - m_iVPCounter > m_volmd.bVPBitTh ) {
//					// reset DQ to use QP in VP header
//					pmbmd->m_intStepDelta = 0;
//					if (pmbmd ->m_dctMd == INTERQ)
//						pmbmd->m_dctMd = INTER;
//					iQPPrev = pmbmd->m_stepSize;

					codeVideoPacketHeader (iMBX, iMBY, pmbmd->m_stepSize);	// video packet header
					m_iVPCounter = iCounter;
					//reset MV predictor
					m_vctForwardMvPredBVOP[0].x  = m_vctForwardMvPredBVOP[0].y  = 0;
					m_vctBackwardMvPredBVOP[0].x = m_vctBackwardMvPredBVOP[0].y = 0;
					m_vctForwardMvPredBVOP[1].x  = m_vctForwardMvPredBVOP[1].y  = 0;
					m_vctBackwardMvPredBVOP[1].x = m_vctBackwardMvPredBVOP[1].y = 0;
				}
			}
// End Toshiba(1998-1-16)

			findColocatedMB (iMBX, iMBY,  pmbmdRef, pmvRef);

			ShapeMode shpmdColocatedMB;
			if(m_vopmd.fShapeBPredDir==B_FORWARD)
				shpmdColocatedMB = m_rgshpmd [min (max (0, iMBX), m_iRefShpNumMBX - 1) 
					+ min (max (0, iMBY), m_iRefShpNumMBY - 1) * m_iRefShpNumMBX];
			else
				shpmdColocatedMB = m_rgmbmdRef [min (max (0, iMBX), m_iNumMBXRef - 1)
					+ min (max (0, iMBY), m_iNumMBYRef - 1) * m_iNumMBXRef].m_shpmd;

			if (pmbmd->m_bColocatedMBSkip &&
			(m_volmd.volType == BASE_LAYER || (m_volmd.volType == ENHN_LAYER && m_vopmd.iRefSelectCode == 3 && m_volmd.iEnhnType == 0)))
			{ // don't need to send any bit for this mode
				copyToCurrBuffJustShape( ppxlcOrigMBBY, m_iFrameWidthY );
				m_statsMB.nBitsShape += codeInterShape (
					ppxlcCurrQMBBY,
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
				// reset motion vectors to zero because of skip
				memset (pmv, 0, BVOP_MV_PER_REF_PER_MB * sizeof (CMotionVector));
				memset (pmvBackward, 0, BVOP_MV_PER_REF_PER_MB * sizeof (CMotionVector));
				if (m_volmd.fAUsage == EIGHT_BIT)
					copyAlphaFromRefToCurrQ(m_pvopcRefQ0, x, y, ppxlcCurrQMBA, &m_rctRefVOPY0);
				copyFromRefToCurrQ(
					m_pvopcRefQ0,
					x, y, 
					ppxlcCurrQMBY, ppxlcCurrQMBU, ppxlcCurrQMBV, &m_rctRefVOPY0
				);
			}
			else {
				copyToCurrBuffWithShape (
					ppxlcOrigMBY, ppxlcOrigMBU, ppxlcOrigMBV, 
					ppxlcOrigMBBY, ppxlcOrigMBA,
					m_iFrameWidthY, m_iFrameWidthUV
				);
				encodeBVOPMB_WithShape (
					ppxlcCurrQMBY, ppxlcCurrQMBU, ppxlcCurrQMBV, ppxlcCurrQMBA, ppxlcCurrQMBBY,
					pmbmd, pmv,	pmvBackward,
					pmvBY, shpmdColocatedMB,
					pmbmdRef, pmvRef,
					iMBX, iMBY,
					x, y, iQPPrev, iQPPrevAlpha
				);
			}
			//padding of bvop is necessary for temporal scalability: bvop in base layer is used for prediction in enhancement layer
			if(m_volmd.bShapeOnly == FALSE)	{
				// Added for field based MC padding by Hyundai(1998-5-9)
				if (!m_vopmd.bInterlace) {
					if (pmbmd -> m_rgTranspStatus [0] != ALL) {
						if (pmbmd -> m_rgTranspStatus [0] == PARTIAL)
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
				}
				// End of Hyundai(1998-5-9)
			}
			pmvBY++;
			pmbmd++;
			pmv         += BVOP_MV_PER_REF_PER_MB;
			pmvBackward += BVOP_MV_PER_REF_PER_MB;
			m_statsVOP += m_statsMB;
			ppxlcCurrQMBY += MB_SIZE;
			ppxlcCurrQMBU += BLOCK_SIZE;
			ppxlcCurrQMBV += BLOCK_SIZE;
			ppxlcCurrQMBBY += MB_SIZE;
			ppxlcCurrQMBA += MB_SIZE;
			ppxlcOrigMBY += MB_SIZE;
			ppxlcOrigMBU += BLOCK_SIZE;
			ppxlcOrigMBV += BLOCK_SIZE;
			ppxlcOrigMBBY += MB_SIZE;
			ppxlcOrigMBA += MB_SIZE;
		}
		ppxlcCurrQY += m_iFrameWidthYxMBSize;
		ppxlcCurrQU += m_iFrameWidthUVxBlkSize;
		ppxlcCurrQV += m_iFrameWidthUVxBlkSize;
		ppxlcCurrQBY += m_iFrameWidthYxMBSize;
		ppxlcCurrQA += m_iFrameWidthYxMBSize;
		
		ppxlcOrigY += m_iFrameWidthYxMBSize;
		ppxlcOrigU += m_iFrameWidthUVxBlkSize;
		ppxlcOrigV += m_iFrameWidthUVxBlkSize;
		ppxlcOrigBY += m_iFrameWidthYxMBSize;
		ppxlcOrigA += m_iFrameWidthYxMBSize;
	}
	// Added for field based MC padding by Hyundai(1998-5-9)
        if (m_vopmd.bInterlace && m_volmd.bShapeOnly == FALSE)
                fieldBasedMCPadding (field_pmbmd, m_pvopcCurrQ);
	// End of Hyundai(1998-5-9)

}

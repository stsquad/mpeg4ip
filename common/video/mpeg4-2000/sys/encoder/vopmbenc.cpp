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

and also edited by
	Yoshinori Suzuki (Hitachi, Ltd.)

and also edited by
	Hideaki Kimata (NTT)

and also edited by
    Fujitsu Laboratories Ltd. (contact: Eishi Morimatsu)

and also edited by
	Takefumi Nagumo	(naugmo@av.crl.sony.co.jp)
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
        Feb.24   1999:  GMC added by Yoshinori Suzuki (Hitachi, Ltd.)
	May 9, 1999:	tm5 rate control by DemoGraFX, duhoff@mediaone.net
	Aug.24, 1999:   NEWPRED added by Hideaki Kimata (NTT) 
	Sep.06	1999 : RRV added by Eishi Morimatsu (Fujitsu Laboratories Ltd.) 
	Feb.01	2000 : Bug fix OBSS by Takefumi Nagumo (SONY)

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


Void CVideoObjectEncoder::encodeVOP ()	
{
// RRV insertion
	assert((m_iRRVScale == 1)||(m_iRRVScale == 2));
// ~RRV	

// Added for Data Partitioning mode by Toshiba(1998-1-16)
	if( m_volmd.bDataPartitioning ) {
		if (m_volmd.fAUsage == RECTANGLE) {
			switch( m_vopmd.vopPredType ) {
			case PVOP:
				encodeNSForPVOP_DP ();
				break;
// GMC
			case SPRITE:
				encodeNSForPVOP_DP ();
				break;
// ~GMC
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
// GMC
			case SPRITE:
				encodeNSForPVOP_WithShape_DP ();
				break;
// ~GMC
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
		if (m_vopmd.vopPredType == PVOP || (m_uiSprite == 2 && m_vopmd.vopPredType == SPRITE)) // GMC
			encodeNSForPVOP ();
		else if (m_vopmd.vopPredType == IVOP)
			encodeNSForIVOP ();
		else
			encodeNSForBVOP ();
	}
	else {
		if (m_vopmd.vopPredType == PVOP || (m_uiSprite == 2 && m_vopmd.vopPredType == SPRITE))  { // GMC
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

	PixelC* ppxlcRefY  = (PixelC*) m_pvopcRefQ1->pixelsY () + m_iStartInRefToCurrRctY;
	PixelC* ppxlcRefU  = (PixelC*) m_pvopcRefQ1->pixelsU () + m_iStartInRefToCurrRctUV;
	PixelC* ppxlcRefV  = (PixelC*) m_pvopcRefQ1->pixelsV () + m_iStartInRefToCurrRctUV;
	
	PixelC* ppxlcOrigY = (PixelC*) m_pvopcOrig->pixelsBoundY ();
	PixelC* ppxlcOrigU = (PixelC*) m_pvopcOrig->pixelsBoundU ();
	PixelC* ppxlcOrigV = (PixelC*) m_pvopcOrig->pixelsBoundV ();

	Int iQPPrev = m_vopmd.intStepI;

	Bool bRestartDelayedQP = TRUE;
	Int iVideoPacketNumber	= 0;
	Int iMBX, iMBY, iMB = 0;

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
			// init
			pmbmd->m_dctMd = INTRA;
			pmbmd->m_bSkip = FALSE;			//reset for direct mode
			pmbmd->m_bMCSEL = FALSE;		//reset for direct mode

            // code current macroblock only if it is not a hole 
			m_bSptMB_NOT_HOLE	 = 	bSptMB_NOT_HOLE;
			if (!m_bSptMB_NOT_HOLE)		//low latency
				goto EOMB1;

#ifdef __TRACE_AND_STATS_
			m_statsMB.reset ();
			m_pbitstrmOut->trace (CSite (iMBX, iMBY), "MB_X_Y");
#endif // __TRACE_AND_STATS_

			// MB level rate control section
			// here is where we calculate the delta QP
			if (m_uiRateControl>=RC_TM5) {
				// TM5 rate control
				updateQP(pmbmd, iQPPrev,
					m_tm5rc.tm5rc_calc_mquant(iMB, m_statsVOP.total())
					);
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

			if ( m_volmd.bVPBitTh >= 0) { // modified by Sharp (98/4/13)
				Int iCounter = m_pbitstrmOut -> getCounter();
				if( ((m_volmd.bNewpredEnable) && (m_volmd.bNewpredSegmentType == 0)) ?
					g_pNewPredEnc->CheckSlice((iMBX *m_iRRVScale),(iMBY *m_iRRVScale),FALSE) : (iCounter - m_iVPCounter > m_volmd.bVPBitTh) ){
					codeVideoPacketHeader (iMBX, iMBY, iQPPrev);	// video packet header
					//printf("VP");
					m_iVPCounter = iCounter;
					bRestartDelayedQP = TRUE;
					iVideoPacketNumber ++;
				}
			}
			
			// call this since we know new qp for certain
			setDCVLCMode(pmbmd, &bRestartDelayedQP);

			pmbmd->m_bFieldMV = 0;

			copyToCurrBuff (ppxlcOrigMBY, ppxlcOrigMBU, ppxlcOrigMBV, m_iFrameWidthY, m_iFrameWidthUV); 
// RRV insertion
			if(m_vopmd.RRVmode.iRRVOnOff == 1)
			{
// RRV_2 insertion
			    pmbmd->m_iVideoPacketNumber	= iVideoPacketNumber;
// ~RRV_2
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
			codeMBTextureHeadOfIVOP (pmbmd);
			sendDCTCoefOfIntraMBTexture (pmbmd);

			//printf("(%d)", pmbmd->m_stepSize);
			iQPPrev = pmbmd->m_stepSize;

EOMB1:
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
}

Void CVideoObjectEncoder::encodeNSForIVOP_WithShape ()
{
	//in case the IVOP is used as an ref for direct mode
	memset (m_rgmv, 0, m_iNumMB * PVOP_MV_PER_REF_PER_MB * sizeof (CMotionVector));
	memset (m_rgmvBY, 0, m_iNumMB * sizeof (CMotionVector));

	Int iMBX, iMBY, iMB = 0;
	CMBMode* pmbmd = m_rgmbmd;
	// Added for field based MC padding by Hyundai(1998-5-9)
    CMBMode* field_pmbmd = m_rgmbmd;
	// End of Hyundai(1998-5-9)
	Int iQPPrev = m_vopmd.intStepI;	//initialization
	//	Int iQPAlpha = m_vopmd.intStepIAlpha[0];

	PixelC* ppxlcRefY  = (PixelC*) m_pvopcRefQ1->pixelsY () + m_iStartInRefToCurrRctY;
	PixelC* ppxlcRefU  = (PixelC*) m_pvopcRefQ1->pixelsU () + m_iStartInRefToCurrRctUV;
	PixelC* ppxlcRefV  = (PixelC*) m_pvopcRefQ1->pixelsV () + m_iStartInRefToCurrRctUV;
	PixelC* ppxlcRefBY = (PixelC*) m_pvopcRefQ1->pixelsBY () + m_iStartInRefToCurrRctY;
	PixelC* ppxlcRefBUV = (PixelC*) m_pvopcRefQ1->pixelsBUV () + m_iStartInRefToCurrRctUV;
	PixelC **pppxlcRefA = NULL, **pppxlcOrigA = NULL;
	
	PixelC* ppxlcOrigY = (PixelC*) m_pvopcOrig->pixelsBoundY ();
	PixelC* ppxlcOrigU = (PixelC*) m_pvopcOrig->pixelsBoundU ();
	PixelC* ppxlcOrigV = (PixelC*) m_pvopcOrig->pixelsBoundV ();
	PixelC* ppxlcOrigBY = (PixelC*) m_pvopcOrig->pixelsBoundBY ();

//OBSS_SAIT_991015
	if(m_volmd.volType == BASE_LAYER && m_volmd.bSpatialScalability)  
		m_rctBase = m_rctCurrVOPY;
	for(Int i=0; i<m_iNumMB; i++) {
		m_rgmvBaseBY[i].iMVX = 0; // for shape
		m_rgmvBaseBY[i].iMVY = 0; // for shape
		m_rgmvBaseBY[i].iHalfX = 0; // for shape
		m_rgmvBaseBY[i].iHalfY = 0; // for shape
	}
//~OBSS_SAIT_991015

// MAC (SB) 26-Nov-99
	PixelC** pppxlcRefMBA = NULL;
	const PixelC** pppxlcOrigMBA = NULL;
	if (m_volmd.fAUsage == EIGHT_BIT) {
		pppxlcRefA = new PixelC* [m_volmd.iAuxCompCount];
		pppxlcOrigA = new PixelC* [m_volmd.iAuxCompCount];
		pppxlcRefMBA = new PixelC* [m_volmd.iAuxCompCount];
		pppxlcOrigMBA = new const PixelC* [m_volmd.iAuxCompCount];
		for(Int iAuxComp=0;iAuxComp<m_volmd.iAuxCompCount;iAuxComp++) {
  			pppxlcRefA[iAuxComp]  = (PixelC*) m_pvopcRefQ1->pixelsA (iAuxComp) + m_iStartInRefToCurrRctY;
			pppxlcOrigA[iAuxComp] = (PixelC*) m_pvopcOrig->pixelsBoundA (iAuxComp);
		}
	}
// ~MAC

	Bool bRestartDelayedQP = TRUE;
	CoordI y = m_rctCurrVOPY.top; 
	for (iMBY = 0; iMBY < m_iNumMBY; iMBY++, y+=16) {
		PixelC* ppxlcRefMBY  = ppxlcRefY;
		PixelC* ppxlcRefMBU  = ppxlcRefU;
		PixelC* ppxlcRefMBV  = ppxlcRefV;
		PixelC* ppxlcRefMBBY = ppxlcRefBY;
		PixelC* ppxlcRefMBBUV = ppxlcRefBUV;
		//PixelC* ppxlcRefMBA  = ppxlcRefA;
		PixelC* ppxlcOrigMBY = ppxlcOrigY;
		PixelC* ppxlcOrigMBU = ppxlcOrigU;
		PixelC* ppxlcOrigMBV = ppxlcOrigV;
		PixelC* ppxlcOrigMBBY = ppxlcOrigBY;
		//PixelC* ppxlcOrigMBA = ppxlcOrigA;

		if (m_volmd.fAUsage == EIGHT_BIT) {
			for(Int iAuxComp=0;iAuxComp<m_volmd.iAuxCompCount;iAuxComp++) {
				pppxlcRefMBA[iAuxComp] = pppxlcRefA[iAuxComp];
				pppxlcOrigMBA[iAuxComp] = pppxlcOrigA[iAuxComp];
			}
		}


	    // In a given Sprite object piece, identify whether current macroblock is not a hole and should be coded ?
		Bool  bSptMB_NOT_HOLE= TRUE;
		if (m_uiSprite == 1 && m_sptMode != BASIC_SPRITE && m_vopmd.SpriteXmitMode != STOP) {
			bSptMB_NOT_HOLE = SptPieceMB_NOT_HOLE(0, iMBY, pmbmd);	
			RestoreMBmCurrRow (iMBY, m_rgpmbmCurr);	
		}
	CoordI x = m_rctCurrVOPY.left; 
		for (iMBX = 0; iMBX < m_iNumMBX; iMBX++, iMB++, x+=16) {
		  // code current macroblock only if it is not a hole 
			pmbmd->m_dctMd = INTRA;
			pmbmd->m_bSkip = FALSE;	//reset for direct mode 
			pmbmd->m_bMCSEL = FALSE;	//reset for direct mode 
			pmbmd->m_bPadded = FALSE;

			//printf("(%d,%d)",iMBX,iMBY);

			m_bSptMB_NOT_HOLE	 = 	bSptMB_NOT_HOLE;
			if (!m_bSptMB_NOT_HOLE) 
				goto EOMB2;

			if (m_uiRateControl>=RC_TM5)
				// mb rate control
				updateQP(pmbmd, iQPPrev, m_tm5rc.tm5rc_calc_mquant(iMB, m_statsVOP.total()));
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

			if ( m_volmd.bVPBitTh >= 0) {
				Int iCounter = m_pbitstrmOut -> getCounter();
				if( iCounter - m_iVPCounter > m_volmd.bVPBitTh ) {
					codeVideoPacketHeader (iMBX, iMBY, iQPPrev);	// video packet header
					m_iVPCounter = iCounter;
					bRestartDelayedQP = TRUE;
				}
			}
            // End Toshiba(1997-11-14)
			
#ifdef __TRACE_AND_STATS_
			m_statsMB.reset ();
			m_pbitstrmOut->trace (CSite (iMBX, iMBY), "MB_X_Y");
#endif // __TRACE_AND_STATS_

			copyToCurrBuffWithShape (
				ppxlcOrigMBY, ppxlcOrigMBU, ppxlcOrigMBV, 
				ppxlcOrigMBBY, pppxlcOrigMBA,
				m_iFrameWidthY, m_iFrameWidthUV
			);
			//Changed HHI 2000-04-11
			decideTransparencyStatus (pmbmd, m_ppxlcCurrMBBY);
			downSampleBY (m_ppxlcCurrMBBY, m_ppxlcCurrMBBUV, pmbmd); // downsample original BY now for LPE padding (using original shape)
			if (pmbmd -> m_rgTranspStatus [0] == PARTIAL) {
				LPEPadding (pmbmd);
				m_statsMB.nBitsShape += codeIntraShape (ppxlcRefMBBY, pmbmd, iMBX, iMBY);
				//Changed HHI 2000-04-11
				decideTransparencyStatus (pmbmd, m_ppxlcCurrMBBY); // need to modify it a little (NONE block won't change)
				downSampleBY (m_ppxlcCurrMBBY, m_ppxlcCurrMBBUV, pmbmd);
			}
			else
				m_statsMB.nBitsShape += codeIntraShape (ppxlcRefMBBY, pmbmd, iMBX, iMBY);

			if(m_volmd.bShapeOnly == FALSE) {// not shape only mode
				if (pmbmd -> m_rgTranspStatus [0] != ALL) {
					setDCVLCMode(pmbmd, &bRestartDelayedQP);

					// HHI  Schueuer: sadct
					if (!m_volmd.bSadctDisable)
						deriveSADCTRowLengths (m_rgiCurrMBCoeffWidth, m_ppxlcCurrMBBY, m_ppxlcCurrMBBUV, pmbmd->m_rgTranspStatus);
					// end HHI

					// HHI Schueuer: sadct
					if (!m_volmd.bSadctDisable)  
						quantizeTextureIntraMB (iMBX, iMBY, pmbmd, ppxlcRefMBY, ppxlcRefMBU, ppxlcRefMBV, pppxlcRefMBA, m_ppxlcCurrMBBY, m_ppxlcCurrMBBUV);
					else
						quantizeTextureIntraMB (iMBX, iMBY, pmbmd, ppxlcRefMBY, ppxlcRefMBU, ppxlcRefMBV, pppxlcRefMBA);
					// end HHI
					codeMBTextureHeadOfIVOP (pmbmd);
					sendDCTCoefOfIntraMBTexture (pmbmd);
					if (m_volmd.fAUsage == EIGHT_BIT) {
						for(Int iAuxComp=0;iAuxComp<m_volmd.iAuxCompCount;iAuxComp++) { // MAC (SB) 29-Nov-99
							codeMBAlphaHeadOfIVOP (pmbmd,iAuxComp);
							sendDCTCoefOfIntraMBAlpha (pmbmd,iAuxComp);
						}
					}

					// MC padding
					if (!m_vopmd.bInterlace) {
						if (pmbmd -> m_rgTranspStatus [0] == PARTIAL)
							mcPadCurrMB (ppxlcRefMBY, ppxlcRefMBU, ppxlcRefMBV, pppxlcRefMBA);
						padNeighborTranspMBs (
							iMBX, iMBY,
							pmbmd,
							ppxlcRefMBY, ppxlcRefMBU, ppxlcRefMBV, pppxlcRefMBA
						);
					}
				}
				else {
					// all transparent so no update possible (set dquant=0)
					cancelQPUpdate(pmbmd);

					if (!m_vopmd.bInterlace) {
						padCurrAndTopTranspMBFromNeighbor (
							iMBX, iMBY,
							pmbmd,
							ppxlcRefMBY, ppxlcRefMBU, ppxlcRefMBV, pppxlcRefMBA
						);
					}
				}

				iQPPrev = pmbmd->m_stepSize;
			}

EOMB2:
	    //ppxlcRefMBA += MB_SIZE;
			ppxlcOrigMBBY += MB_SIZE;
			//ppxlcOrigMBA += MB_SIZE;
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

      if (m_volmd.fAUsage == EIGHT_BIT) {
        for(Int iAuxComp=0;iAuxComp<m_volmd.iAuxCompCount;iAuxComp++) {
          pppxlcRefMBA[iAuxComp] += MB_SIZE;
          pppxlcOrigMBA[iAuxComp] += MB_SIZE;
        }
      }


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
		//ppxlcRefA  += m_iFrameWidthYxMBSize;
		ppxlcRefU  += m_iFrameWidthUVxBlkSize;
		ppxlcRefV  += m_iFrameWidthUVxBlkSize;
		ppxlcRefBY += m_iFrameWidthYxMBSize;
		ppxlcRefBUV += m_iFrameWidthUVxBlkSize;

		ppxlcOrigY += m_iFrameWidthYxMBSize;
		ppxlcOrigBY += m_iFrameWidthYxMBSize;
		//ppxlcOrigA += m_iFrameWidthYxMBSize;
		ppxlcOrigU += m_iFrameWidthUVxBlkSize;
		ppxlcOrigV += m_iFrameWidthUVxBlkSize;

    if (m_volmd.fAUsage == EIGHT_BIT) {
      for(Int iAuxComp=0;iAuxComp<m_volmd.iAuxCompCount;iAuxComp++) {
        pppxlcRefA[iAuxComp]  += m_iFrameWidthYxMBSize;
        pppxlcOrigA[iAuxComp] += m_iFrameWidthYxMBSize;
      }
    }


	}
	// Added for field based MC padding by Hyundai(1998-5-9)
        if (m_vopmd.bInterlace && m_volmd.bShapeOnly == FALSE)
                fieldBasedMCPadding (field_pmbmd, m_pvopcRefQ1);
	// End of Hyundai(1998-5-9)

	if (m_volmd.fAUsage == EIGHT_BIT) {
    delete [] pppxlcRefA;
    delete [] pppxlcOrigA;
    delete [] pppxlcRefMBA;
    delete [] pppxlcOrigMBA;
	}
}

Void CVideoObjectEncoder::encodeNSForPVOP ()	
{
    if (m_uiSprite == 0 || m_uiSprite == 2)     // GMC
	    motionEstPVOP ();

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

	//printf("(%d %d)\n", m_vopmd.intStep, newQStep);

	CMotionVector *pmv_RRV	= new CMotionVector[m_iNumMB *PVOP_MV_PER_REF_PER_MB];
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
		    pmv_RRV[j].m_vctTrueHalfPel_x2.x = pmv_RRV[j].m_vctTrueHalfPel.x;
		    pmv_RRV[j].m_vctTrueHalfPel_x2.y = pmv_RRV[j].m_vctTrueHalfPel.y;
		}
	}

	Int iVideoPacketNumber	= 0;

	Int iMBX, iMBY, iMB = 0;
	CoordI y = 0; 
	CMBMode* pmbmd = m_rgmbmd;
	
	Int iQPPrev = m_vopmd.intStep;

	CMotionVector* pmv = m_rgmv;
	CMotionVector* plmv_RRV = pmv_RRV;

	PixelC* ppxlcRefY = (PixelC*) m_pvopcRefQ1->pixelsY () + m_iStartInRefToCurrRctY;
	PixelC* ppxlcRefU = (PixelC*) m_pvopcRefQ1->pixelsU () + m_iStartInRefToCurrRctUV;
	PixelC* ppxlcRefV = (PixelC*) m_pvopcRefQ1->pixelsV () + m_iStartInRefToCurrRctUV;
	
	PixelC* ppxlcOrigY = (PixelC*) m_pvopcOrig->pixelsBoundY ();
	PixelC* ppxlcOrigU = (PixelC*) m_pvopcOrig->pixelsBoundU ();
	PixelC* ppxlcOrigV = (PixelC*) m_pvopcOrig->pixelsBoundV ();

	Bool bRestartDelayedQP = TRUE;

	const PixelC* RefbufY = m_pvopcRefQ0-> pixelsY ();
	const PixelC* RefbufU = m_pvopcRefQ0-> pixelsU ();
	const PixelC* RefbufV = m_pvopcRefQ0-> pixelsV ();

	if (m_volmd.bNewpredEnable) {
		g_pNewPredEnc->CopyReftoBuf(RefbufY, RefbufU, RefbufV, m_rctRefFrameY, m_rctRefFrameUV);
		for (iMBY = 0; iMBY < m_iNumMBY; iMBY++) {
			for (iMBX = 0; iMBX < m_iNumMBX; iMBX++)	{
				(pmbmd + iMBX + m_iNumMBX*iMBY) -> m_iNPSegmentNumber = g_pNewPredEnc->GetSliceNum((iMBX *m_iRRVScale),(iMBY *m_iRRVScale));
			}
		}
	}

	for (iMBY = 0; iMBY < m_iNumMBY; iMBY++, y += (MB_SIZE *m_iRRVScale)) {
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

		for (iMBX = 0; iMBX < m_iNumMBX; iMBX++, x += (MB_SIZE *m_iRRVScale), iMB++)	{
			if((m_volmd.bNewpredEnable) && g_pNewPredEnc->CheckSlice((iMBX *m_iRRVScale),(iMBY *m_iRRVScale))) {
				PixelC* RefpointY = (PixelC*) m_pvopcRefQ0->pixelsY () + m_iStartInRefToCurrRctY + iMBY * (MB_SIZE *m_iRRVScale) * m_rctRefFrameY.width;
				PixelC* RefpointU = (PixelC*) m_pvopcRefQ0->pixelsU () + m_iStartInRefToCurrRctUV + iMBY * (BLOCK_SIZE *m_iRRVScale) * m_rctRefFrameUV.width;
				PixelC* RefpointV = (PixelC*) m_pvopcRefQ0->pixelsV () + m_iStartInRefToCurrRctUV + iMBY * (BLOCK_SIZE *m_iRRVScale) * m_rctRefFrameUV.width;
				g_pNewPredEnc->ChangeRefOfSlice((const PixelC* )RefpointY, RefbufY,(const PixelC* ) RefpointU, RefbufU,
						(const PixelC* )RefpointV, RefbufV, (iMBX *m_iRRVScale), (iMBY *m_iRRVScale),m_rctRefFrameY, m_rctRefFrameUV);

				m_rctRefVOPZoom0 = m_rctRefVOPY0.upSampleBy2 ();
				biInterpolateY (m_pvopcRefQ0, m_rctRefVOPY0, m_puciRefQZoom0, m_rctRefVOPZoom0, m_vopmd.iRoundingControl);
			}


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

			// MB rate control
			if (m_uiRateControl>=RC_TM5) 
			{
				updateQP(pmbmd, iQPPrev, m_tm5rc.tm5rc_calc_mquant(iMB, m_statsVOP.total()) );
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

			if ( m_volmd.bVPBitTh >= 0) {
				Int iCounter = m_pbitstrmOut -> getCounter();

				if( ((m_volmd.bNewpredEnable) && (m_volmd.bNewpredSegmentType == 0)) ?
					g_pNewPredEnc->CheckSlice((iMBX *m_iRRVScale),(iMBY *m_iRRVScale),FALSE) : (iCounter - m_iVPCounter > m_volmd.bVPBitTh) )
				{
					codeVideoPacketHeader (iMBX, iMBY, iQPPrev);	// video packet header
					// prev qp is used in header, since it can be updated by dq of next mb
					//printf("VP");
					m_iVPCounter = iCounter;
					bRestartDelayedQP = TRUE;
					iVideoPacketNumber++;
				}
			}

			copyToCurrBuff (ppxlcOrigMBY, ppxlcOrigMBU, ppxlcOrigMBV, m_iFrameWidthY, m_iFrameWidthUV); 

			pmbmd->m_iVideoPacketNumber	= iVideoPacketNumber;

			encodePVOPMB (
				ppxlcRefMBY, ppxlcRefMBU, ppxlcRefMBV,
				pmbmd, pmv, plmv_RRV, 
				iMBX, iMBY,
				x, y, &bRestartDelayedQP
			);

			//printf("(%d)", pmbmd->m_stepSize);

			iQPPrev = pmbmd->m_stepSize;

EOMB3:
			pmbmd++;
			pmv += PVOP_MV_PER_REF_PER_MB;
// RRV insertion
			if (m_vopmd.RRVmode.iRRVOnOff == 1)
			{
				plmv_RRV	+= PVOP_MV_PER_REF_PER_MB;
			}
// ~RRV
#ifdef __TRACE_AND_STATS_
			m_statsVOP += m_statsMB;
#endif // __TRACE_AND_STATS_
// RRV modification
			ppxlcRefMBY  += (MB_SIZE *m_iRRVScale);
			ppxlcRefMBU  += (BLOCK_SIZE *m_iRRVScale);
			ppxlcRefMBV  += (BLOCK_SIZE *m_iRRVScale);
			ppxlcOrigMBY += (MB_SIZE *m_iRRVScale);
			ppxlcOrigMBU += (BLOCK_SIZE *m_iRRVScale);
			ppxlcOrigMBV += (BLOCK_SIZE *m_iRRVScale);
//			ppxlcRefMBY += MB_SIZE;
//			ppxlcRefMBU += BLOCK_SIZE;
//			ppxlcRefMBV += BLOCK_SIZE;
//			ppxlcOrigMBY += MB_SIZE;
//			ppxlcOrigMBU += BLOCK_SIZE;
//			ppxlcOrigMBV += BLOCK_SIZE;
// ~RRV
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

// NEWPRED
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

	delete pmv_RRV;

	// update the vop header qp if changed by rate control
	m_vopmd.intStep = newQStep;
}

Void CVideoObjectEncoder::encodeNSForPVOP_WithShape ()
{
	Int iMBX, iMBY, iMB = 0;

	motionEstPVOP_WithShape ();

	// shape bitstream is set to shape cache
	m_pbitstrmShapeMBOut = m_pbitstrmShape;

	// Rate Control
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
	// Added for field based MC padding by Hyundai(1998-5-9)
	CMBMode* field_pmbmd = m_rgmbmd;
	// End of Hyundai(1998-5-9)

	Int iQPPrev = m_vopmd.intStep;
	//	Int iQPAlpha = m_vopmd.intStepPAlpha[0];

	CMotionVector* pmv = m_rgmv;
	CMotionVector* pmvBY = m_rgmvBY;

//OBSS_SAIT_991015
	if(m_volmd.volType == BASE_LAYER && m_volmd.bSpatialScalability) {
		m_rgmvBaseBY = m_rgmvBY;
		m_rctBase = m_rctCurrVOPY;
	}
	Int xIndex, yIndex;
	xIndex = yIndex = 0;
//OBSSFIX_MODE3
    if( m_volmd.volType == ENHN_LAYER && (m_volmd.bSpatialScalability && m_volmd.iHierarchyType==0) &&
      !(m_volmd.iEnhnType != 0 && m_volmd.iuseRefShape == 1)) {
//	if(m_volmd.volType == ENHN_LAYER && (m_volmd.bSpatialScalability && m_volmd.iHierarchyType==0)) {
//~OBSSFIX_MODE3
		xIndex = (m_rctCurrVOPY.left - (m_rctBase.left*m_volmd.ihor_sampling_factor_n_shape/m_volmd.ihor_sampling_factor_m_shape) );	
		yIndex = (m_rctCurrVOPY.top - (m_rctBase.top*m_volmd.iver_sampling_factor_n_shape/m_volmd.iver_sampling_factor_m_shape) );		
		xIndex /= MB_SIZE;
		yIndex /= MB_SIZE;
	}
//~OBSS_SAIT_991015

	PixelC* ppxlcRefY = (PixelC*) m_pvopcRefQ1->pixelsY () + m_iStartInRefToCurrRctY;
	PixelC* ppxlcRefU = (PixelC*) m_pvopcRefQ1->pixelsU () + m_iStartInRefToCurrRctUV;
	PixelC* ppxlcRefV = (PixelC*) m_pvopcRefQ1->pixelsV () + m_iStartInRefToCurrRctUV;
	PixelC* ppxlcRefBY = (PixelC*) m_pvopcRefQ1->pixelsBY () + m_iStartInRefToCurrRctY;
	PixelC **pppxlcRefA = NULL, **pppxlcOrigA = NULL;
	
	PixelC* ppxlcOrigY = (PixelC*) m_pvopcOrig->pixelsBoundY ();
	PixelC* ppxlcOrigU = (PixelC*) m_pvopcOrig->pixelsBoundU ();
	PixelC* ppxlcOrigV = (PixelC*) m_pvopcOrig->pixelsBoundV ();
	PixelC* ppxlcOrigBY = (PixelC*) m_pvopcOrig->pixelsBoundBY ();

// MAC (SB) 26-Nov-99
    PixelC** pppxlcRefMBA = NULL;
    const PixelC** pppxlcOrigMBA = NULL;
	if (m_volmd.fAUsage == EIGHT_BIT) {
		pppxlcRefA = new PixelC* [m_volmd.iAuxCompCount];
		pppxlcOrigA = new PixelC* [m_volmd.iAuxCompCount];
		pppxlcRefMBA = new PixelC* [m_volmd.iAuxCompCount];
		pppxlcOrigMBA = new const PixelC* [m_volmd.iAuxCompCount];
		for(Int iAuxComp=0;iAuxComp<m_volmd.iAuxCompCount;iAuxComp++) {
  			pppxlcRefA[iAuxComp]  = (PixelC*) m_pvopcRefQ1->pixelsA (iAuxComp) + m_iStartInRefToCurrRctY;
	  		pppxlcOrigA[iAuxComp] = (PixelC*) m_pvopcOrig->pixelsBoundA (iAuxComp);
		}
	}
// ~MAC
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
		//PixelC* ppxlcRefMBA = ppxlcRefA;
		PixelC* ppxlcOrigMBY = ppxlcOrigY;
		PixelC* ppxlcOrigMBU = ppxlcOrigU;
		PixelC* ppxlcOrigMBV = ppxlcOrigV;
		PixelC* ppxlcOrigMBBY = ppxlcOrigBY;
		//PixelC* ppxlcOrigMBA = ppxlcOrigA;
		CoordI x = m_rctCurrVOPY.left;

		if (m_volmd.fAUsage == EIGHT_BIT) {
		  for(Int iAuxComp=0;iAuxComp<m_volmd.iAuxCompCount;iAuxComp++) {
			pppxlcRefMBA[iAuxComp] = pppxlcRefA[iAuxComp];
			pppxlcOrigMBA[iAuxComp] = pppxlcOrigA[iAuxComp];
		  }
		}


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

		copyToCurrBuffJustShape (ppxlcOrigMBBY, m_iFrameWidthY);

		ShapeMode shpmdColocatedMB = UNKNOWN;
		if(m_vopmd.bShapeCodingType) {
			if(!(m_volmd.bSpatialScalability && m_volmd.iHierarchyType==0)){	
				shpmdColocatedMB = m_rgmbmdRef [
					min (max (0, iMBY), m_iNumMBYRef - 1) * m_iNumMBXRef].m_shpmd;
			}
			else {
				if(m_volmd.volType == BASE_LAYER)                       
					shpmdColocatedMB = m_rgmbmdRef [min (max (0, iMBY), m_iNumMBYRef - 1) * m_iNumMBXRef].m_shpmd;
                else if(m_volmd.volType == ENHN_LAYER && m_volmd.iEnhnType!=0 && m_volmd.iuseRefShape ==1 )       
					shpmdColocatedMB = ALL_OPAQUE;
                else if(m_volmd.volType == ENHN_LAYER){ // if SpatialScalability
					shpmdColocatedMB = m_rgBaseshpmd [
										min (max (0, (iMBY+yIndex)*m_volmd.iver_sampling_factor_m_shape/m_volmd.iver_sampling_factor_n_shape), (m_iNumMBBaseYRef-1)) * m_iNumMBBaseXRef];                
				}
			}
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
			if (m_uiRateControl>=RC_TM5) 
			{
				updateQP(pmbmd, iQPPrev, m_tm5rc.tm5rc_calc_mquant(iMB, m_statsVOP.total()) );
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

			if ( m_volmd.bVPBitTh >= 0) {
				if( bCodeVPHeaderNext ) {
					codeVideoPacketHeader (iMBX, iMBY, iQPPrev);	// video packet header
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
//OBSS_SAIT_991015
					if(!(m_volmd.bSpatialScalability && m_volmd.iHierarchyType==0)){	
						shpmdColocatedMB = m_rgmbmdRef [
							min (max (0, iMBX+1), m_iNumMBXRef-1) + 
 							min (max (0, iMBY), m_iNumMBYRef-1) * m_iNumMBXRef
						].m_shpmd;
					}
					else {
//OBSSFIX_MODE3
						if(m_volmd.volType == BASE_LAYER)                       
							shpmdColocatedMB = m_rgmbmdRef [
								min (max (0, iMBX+1), m_iNumMBXRef-1) + 
								min (max (0, iMBY), m_iNumMBYRef-1) * m_iNumMBXRef
							].m_shpmd;
						else if(m_volmd.volType == ENHN_LAYER && m_volmd.iEnhnType!=0 && m_volmd.iuseRefShape ==1 )       
							shpmdColocatedMB = ALL_OPAQUE;
						else if(m_volmd.volType == ENHN_LAYER){ // if SpatialScalability
							shpmdColocatedMB = m_rgBaseshpmd [																														
								min (max (0, ((iMBX+xIndex+1)*m_volmd.ihor_sampling_factor_m_shape/m_volmd.ihor_sampling_factor_n_shape)), (m_iNumMBBaseXRef-1)) +					
								min (max (0, (iMBY+yIndex)*m_volmd.iver_sampling_factor_m_shape/m_volmd.iver_sampling_factor_n_shape), (m_iNumMBBaseYRef-1)) * m_iNumMBBaseXRef];	
						}

					}
//~OBSS_SAIT_991015
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

			if(m_volmd.bShapeOnly==FALSE)
			{
				if (pmbmd -> m_rgTranspStatus [0] != ALL) {
					// need to copy binary shape too since curr buff is future shape
					copyToCurrBuffWithShape(ppxlcOrigMBY, ppxlcOrigMBU, ppxlcOrigMBV,
						ppxlcRefMBBY, pppxlcOrigMBA, m_iFrameWidthY, m_iFrameWidthUV);
					//Changed HHI 2000-04-11
					downSampleBY (m_ppxlcCurrMBBY, m_ppxlcCurrMBBUV, pmbmd);

					// HHI Schueuer: sadct
					if (!m_volmd.bSadctDisable)	{				
						deriveSADCTRowLengths (m_rgiCurrMBCoeffWidth, m_ppxlcCurrMBBY, m_ppxlcCurrMBBUV, pmbmd->m_rgTranspStatus);
						encodePVOPMBTextureWithShape(ppxlcRefMBY, ppxlcRefMBU, ppxlcRefMBV,
							pppxlcRefMBA, pmbmd, pmv, iMBX, iMBY, x, y,
							&bRestartDelayedQP, m_ppxlcCurrMBBY, m_ppxlcCurrMBBUV );
					}
					else
						encodePVOPMBTextureWithShape(ppxlcRefMBY, ppxlcRefMBU, ppxlcRefMBV,
							pppxlcRefMBA, pmbmd, pmv, iMBX, iMBY, x, y,
							&bRestartDelayedQP);
					// end HHI
					// Added for field based MC padding by Hyundai(1998-5-9)
					if (!m_vopmd.bInterlace) { 
						if (pmbmd -> m_rgTranspStatus [0] == PARTIAL)
							mcPadCurrMB (ppxlcRefMBY, ppxlcRefMBU, ppxlcRefMBV, pppxlcRefMBA);
						padNeighborTranspMBs (
							iMBX, iMBY,
							pmbmd,
							ppxlcRefMBY, ppxlcRefMBU, ppxlcRefMBV, pppxlcRefMBA
						);
					}
					// End of Hyundai(1998-5-9)
				}
				else {
					// all transparent so no update possible (set dquant=0)
					cancelQPUpdate(pmbmd);
					
					// Added for field based MC padding by Hyundai(1998-5-9)
					if (!m_vopmd.bInterlace) { 
						padCurrAndTopTranspMBFromNeighbor (
							iMBX, iMBY,
							pmbmd,
							ppxlcRefMBY, ppxlcRefMBU, ppxlcRefMBV, pppxlcRefMBA
						);
					}
					// End of Hyundai(1998-5-9)
				}

				iQPPrev = pmbmd->m_stepSize;
			}

			pmbmd++;
			pmv += PVOP_MV_PER_REF_PER_MB;
			pmvBY++;
			ppxlcRefMBBY += MB_SIZE;
//			ppxlcRefMBA += MB_SIZE;
			ppxlcOrigMBBY += MB_SIZE;
//			ppxlcOrigMBA += MB_SIZE;
#ifdef __TRACE_AND_STATS_
			m_statsVOP += m_statsMB;
#endif // __TRACE_AND_STATS_
			ppxlcRefMBY += MB_SIZE;
			ppxlcRefMBU += BLOCK_SIZE;
			ppxlcRefMBV += BLOCK_SIZE;
			ppxlcOrigMBY += MB_SIZE;
			ppxlcOrigMBU += BLOCK_SIZE;
			ppxlcOrigMBV += BLOCK_SIZE;

      if (m_volmd.fAUsage == EIGHT_BIT) {
        for(Int iAuxComp=0;iAuxComp<m_volmd.iAuxCompCount;iAuxComp++) {
          pppxlcRefMBA[iAuxComp] += MB_SIZE;
          pppxlcOrigMBA[iAuxComp] += MB_SIZE;
        }
      }


		}
		MacroBlockMemory** ppmbmTemp = m_rgpmbmAbove;
		m_rgpmbmAbove = m_rgpmbmCurr;
		m_rgpmbmCurr  = ppmbmTemp;

		ppxlcRefY += m_iFrameWidthYxMBSize;
		ppxlcRefU += m_iFrameWidthUVxBlkSize;
		ppxlcRefV += m_iFrameWidthUVxBlkSize;
		ppxlcRefBY += m_iFrameWidthYxMBSize;
		//ppxlcRefA += m_iFrameWidthYxMBSize;
		
		ppxlcOrigY += m_iFrameWidthYxMBSize;
		ppxlcOrigBY += m_iFrameWidthYxMBSize;
//		ppxlcOrigA += m_iFrameWidthYxMBSize;
		ppxlcOrigU += m_iFrameWidthUVxBlkSize;
		ppxlcOrigV += m_iFrameWidthUVxBlkSize;


    if (m_volmd.fAUsage == EIGHT_BIT) {
      for(Int iAuxComp=0;iAuxComp<m_volmd.iAuxCompCount;iAuxComp++) {
        pppxlcRefA[iAuxComp]  += m_iFrameWidthYxMBSize;
        pppxlcOrigA[iAuxComp] += m_iFrameWidthYxMBSize;
      }
    }

	}

  // Added for field based MC padding by Hyundai(1998-5-9)
  if (m_vopmd.bInterlace && m_volmd.bShapeOnly == FALSE)
    fieldBasedMCPadding (field_pmbmd, m_pvopcRefQ1);
  // End of Hyundai(1998-5-9)
  //OBSS_SAIT_991015
//OBSSFIX_MODE3
   if((m_volmd.bSpatialScalability && m_volmd.iHierarchyType==0) && m_volmd.volType == ENHN_LAYER && !(m_volmd.iEnhnType != 0 && m_volmd.iuseRefShape == 1) ) { // if SpatialScalability
// if((m_volmd.bSpatialScalability && m_volmd.iHierarchyType==0) && m_volmd.volType == ENHN_LAYER) {	// if SpatialScalability
//~OBSSFIX_MODE3
	 delete m_pvopcRefQ0->getPlane (BY_PLANE)->m_pbHorSamplingChk;
	 delete m_pvopcRefQ0->getPlane (BY_PLANE)->m_pbVerSamplingChk;
  }
  //~OBSS_SAIT_991015
  // MAC
 	if (m_volmd.fAUsage == EIGHT_BIT) {
    delete [] pppxlcRefA;
    delete [] pppxlcOrigA;
    delete [] pppxlcRefMBA;
    delete [] pppxlcOrigMBA;
  }
  //~MAC
	// restore normal output stream
	m_pbitstrmShapeMBOut = m_pbitstrmOut;

	// update the vop header qp if changed by rate control
	m_vopmd.intStep = newQStep;
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

	Int iQPPrev = m_vopmd.intStepB;
	Int iMBX, iMBY, iMB = 0;

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


			// MB rate control
			if (m_uiRateControl>=RC_TM5) 
			{
				Int iNewQP = m_tm5rc.tm5rc_calc_mquant(iMB, m_statsVOP.total());
				if(iNewQP>iQPPrev)
					iNewQP = iQPPrev + 2;
				else if(iNewQP<iQPPrev)
					iNewQP = iQPPrev - 2;
				updateQP(pmbmd, iQPPrev, iNewQP);
			}
			else
			{
#ifdef _MBQP_CHANGE_		
				Int iDQuant = 2*(rand() % 3) - 2;
				updateQP(pmbmd, iQPPrev, iQPPrev + iDQuant);
#else
				// no change in step size, but still need to call...
				updateQP(pmbmd, iQPPrev, iQPPrev);
#endif //_MBQP_CHANGE_
			}

			if(pmbmd->m_intStepDelta==-1 || pmbmd->m_intStepDelta==1 
				|| pmbmd->m_bColocatedMBSkip || pmbmd->m_mbType == DIRECT || pmbmd->m_bColocatedMBMCSEL)
				cancelQPUpdate(pmbmd);

			if ( m_volmd.bVPBitTh >= 0) {
				Int iCounter = m_pbitstrmOut -> getCounter();
				if( iCounter - m_iVPCounter > m_volmd.bVPBitTh ) {
					codeVideoPacketHeader (iMBX, iMBY, iQPPrev);	// video packet header
					m_iVPCounter = iCounter;
					//reset MV predictor
					m_vctForwardMvPredBVOP[0].x  = m_vctForwardMvPredBVOP[0].y  = 0;
					m_vctBackwardMvPredBVOP[0].x = m_vctBackwardMvPredBVOP[0].y = 0;
					m_vctForwardMvPredBVOP[1].x  = m_vctForwardMvPredBVOP[1].y  = 0;
					m_vctBackwardMvPredBVOP[1].x = m_vctBackwardMvPredBVOP[1].y = 0;

				}
			}

			if ((pmbmd->m_bColocatedMBSkip && !(pmbmd->m_bColocatedMBMCSEL)) && // GMC
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
			}
		
			//printf("(%d)", pmbmd->m_stepSize);
			iQPPrev = pmbmd->m_stepSize;

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
	//OBSSFIX_MODE3
	if(m_volmd.volType == ENHN_LAYER && m_volmd.bSpatialScalability && m_volmd.iHierarchyType == 0 &&
		m_volmd.iEnhnType != 0 && m_volmd.iuseRefShape == 1)
	{
		m_vopmd.fShapeBPredDir = B_FORWARD;
	}
	else if(m_bCodedFutureRef==FALSE)
		m_vopmd.fShapeBPredDir = B_FORWARD;
	else
	{
		if(m_tFutureRef - m_t >= m_t - m_tPastRef)
			m_vopmd.fShapeBPredDir = B_FORWARD;
		else
			m_vopmd.fShapeBPredDir = B_BACKWARD;
	}
	
	Int iQPPrev = m_vopmd.intStepB;
	
	CoordI y = m_rctCurrVOPY.top; 
	CMBMode*	   pmbmd = m_rgmbmd;
	// Added for field based MC padding by Hyundai(1998-5-9)
	CMBMode* field_pmbmd = m_rgmbmd;
	// End of Hyundai(1998-5-9)
	
	CMotionVector* pmv = m_rgmv;
	CMotionVector* pmvBackward = m_rgmvBackward;
	CMotionVector* pmvBY = m_rgmvBY;
	
	//OBSS_SAIT_991015
	if(m_volmd.volType == BASE_LAYER && m_volmd.bSpatialScalability) {
		m_rgmvBaseBY = m_rgmvBY;
		m_rctBase = m_rctCurrVOPY;
	}
	Int xIndex, yIndex;
	xIndex = yIndex = 0;
	//OBSSFIX_MODE3
	if( m_volmd.volType == ENHN_LAYER && (m_volmd.bSpatialScalability && m_volmd.iHierarchyType==0) &&
		!(m_volmd.iEnhnType != 0 && m_volmd.iuseRefShape == 1)) {
		//	if(m_volmd.volType == ENHN_LAYER && (m_volmd.bSpatialScalability && m_volmd.iHierarchyType==0)) {
		//~OBSSFIX_MODE3
		xIndex = (m_rctCurrVOPY.left - (m_rctBase.left*	m_volmd.ihor_sampling_factor_n_shape/m_volmd.ihor_sampling_factor_m_shape));			
		yIndex = (m_rctCurrVOPY.top - (m_rctBase.top*m_volmd.iver_sampling_factor_n_shape/m_volmd.iver_sampling_factor_m_shape));			
		xIndex /= MB_SIZE;
		yIndex /= MB_SIZE;
	}
	//~OBSS_SAIT_991015
	
	PixelC* ppxlcCurrQY = (PixelC*) m_pvopcCurrQ->pixelsY () + m_iStartInRefToCurrRctY;
	PixelC* ppxlcCurrQU = (PixelC*) m_pvopcCurrQ->pixelsU () + m_iStartInRefToCurrRctUV;
	PixelC* ppxlcCurrQV = (PixelC*) m_pvopcCurrQ->pixelsV () + m_iStartInRefToCurrRctUV;
	PixelC* ppxlcCurrQBY = (PixelC*) m_pvopcCurrQ->pixelsBY () + m_iStartInRefToCurrRctY;
	PixelC **pppxlcCurrQA = NULL, **pppxlcOrigA = NULL;
	
	PixelC* ppxlcOrigY = (PixelC*) m_pvopcOrig->pixelsBoundY ();
	PixelC* ppxlcOrigU = (PixelC*) m_pvopcOrig->pixelsBoundU ();
	PixelC* ppxlcOrigV = (PixelC*) m_pvopcOrig->pixelsBoundV ();
	PixelC* ppxlcOrigBY = (PixelC*) m_pvopcOrig->pixelsBoundBY ();
	
	// MAC (SB) 26-Nov-99
	PixelC** pppxlcCurrQMBA = NULL;
	const PixelC** pppxlcOrigMBA = NULL;
	if (m_volmd.fAUsage == EIGHT_BIT) {
		pppxlcCurrQA   = new PixelC* [m_volmd.iAuxCompCount];
		pppxlcOrigA    = new PixelC* [m_volmd.iAuxCompCount];
		pppxlcCurrQMBA = new PixelC* [m_volmd.iAuxCompCount];
		pppxlcOrigMBA  = new const PixelC* [m_volmd.iAuxCompCount];
		for(Int iAuxComp=0;iAuxComp<m_volmd.iAuxCompCount;iAuxComp++) { // MAC (SB) 26-Nov-99
			pppxlcCurrQA[iAuxComp] = (PixelC*) m_pvopcCurrQ->pixelsA (iAuxComp) + m_iStartInRefToCurrRctY;
			pppxlcOrigA[iAuxComp]  = (PixelC*) m_pvopcOrig->pixelsBoundA (iAuxComp);
		}
	}
	//~MAC
	
	Int iMBX, iMBY, iMB = 0;
	const CMBMode* pmbmdRef;
	const CMotionVector* pmvRef;
	for (iMBY = 0; iMBY < m_iNumMBY; iMBY++, y += MB_SIZE) {
		PixelC* ppxlcCurrQMBY = ppxlcCurrQY;
		PixelC* ppxlcCurrQMBU = ppxlcCurrQU;
		PixelC* ppxlcCurrQMBV = ppxlcCurrQV;
		PixelC* ppxlcCurrQMBBY = ppxlcCurrQBY;
		//PixelC* ppxlcCurrQMBA = ppxlcCurrQA;
		PixelC* ppxlcOrigMBY = ppxlcOrigY;
		PixelC* ppxlcOrigMBU = ppxlcOrigU;
		PixelC* ppxlcOrigMBV = ppxlcOrigV;
		PixelC* ppxlcOrigMBBY = ppxlcOrigBY;
		//PixelC* ppxlcOrigMBA = ppxlcOrigA;
		if (m_volmd.fAUsage == EIGHT_BIT) {
			for(Int iAuxComp=0;iAuxComp<m_volmd.iAuxCompCount;iAuxComp++) { // MAC (SB) 26-Nov-99
				pppxlcCurrQMBA[iAuxComp] = pppxlcCurrQA[iAuxComp];
				pppxlcOrigMBA[iAuxComp]  = pppxlcOrigA[iAuxComp];
			}
		}
		
		CoordI x = m_rctCurrVOPY.left;
		m_vctForwardMvPredBVOP[0].x  = m_vctForwardMvPredBVOP[0].y  = 0;
		m_vctBackwardMvPredBVOP[0].x = m_vctBackwardMvPredBVOP[0].y = 0;
		m_vctForwardMvPredBVOP[1].x  = m_vctForwardMvPredBVOP[1].y  = 0;
		m_vctBackwardMvPredBVOP[1].x = m_vctBackwardMvPredBVOP[1].y = 0;
		
		for (iMBX = 0; iMBX < m_iNumMBX; iMBX++, x += MB_SIZE, iMB++)	{
			pmbmd->m_intStepDelta = 0;	//sharp added this
			pmbmd->m_bPadded=FALSE;
			// shape quantization part
			m_statsMB.reset ();
			
			// MB rate control
			if (m_uiRateControl>=RC_TM5) 
			{
				Int iNewQP = m_tm5rc.tm5rc_calc_mquant(iMB, m_statsVOP.total());
				if(iNewQP>iQPPrev)
					iNewQP = iQPPrev + 2;
				else if(iNewQP<iQPPrev)
					iNewQP = iQPPrev - 2;
				updateQP(pmbmd, iQPPrev, iNewQP);
			}
			else
			{
#ifdef _MBQP_CHANGE_		
				Int iDQuant = 2*(rand() % 3) - 2;
				updateQP(pmbmd, iQPPrev, iQPPrev + iDQuant);
#else
				// no change in step size, but still need to call...
				updateQP(pmbmd, iQPPrev, iQPPrev);
#endif //_MBQP_CHANGE_
			}
			
			if(pmbmd->m_intStepDelta==-1 || pmbmd->m_intStepDelta==1 
				|| pmbmd->m_bColocatedMBSkip || pmbmd->m_mbType == DIRECT || pmbmd->m_bColocatedMBMCSEL)
				cancelQPUpdate(pmbmd);
			
			if ( m_volmd.bVPBitTh >= 0) {
				Int iCounter = m_pbitstrmOut -> getCounter();
				if( iCounter - m_iVPCounter > m_volmd.bVPBitTh ) {
					codeVideoPacketHeader (iMBX, iMBY, iQPPrev);	// video packet header
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
			if(!(m_volmd.bSpatialScalability && m_volmd.iHierarchyType==0)){		
				if(m_vopmd.fShapeBPredDir==B_FORWARD)
					shpmdColocatedMB = m_rgshpmd [min (max (0, iMBX), m_iRefShpNumMBX - 1) 
					+ min (max (0, iMBY), m_iRefShpNumMBY - 1) * m_iRefShpNumMBX];
				else
					shpmdColocatedMB = m_rgmbmdRef [min (max (0, iMBX), m_iNumMBXRef - 1)
					+ min (max (0, iMBY), m_iNumMBYRef - 1) * m_iNumMBXRef].m_shpmd;
			}
			else {
				if(m_volmd.volType == BASE_LAYER){                      
					if(m_vopmd.fShapeBPredDir==B_FORWARD)
						shpmdColocatedMB = m_rgshpmd [min (max (0, iMBX), m_iRefShpNumMBX - 1) 
						+ min (max (0, iMBY), m_iRefShpNumMBY - 1) * m_iRefShpNumMBX];
					else
						shpmdColocatedMB = m_rgmbmdRef [min (max (0, iMBX), m_iNumMBXRef - 1)
						+ min (max (0, iMBY), m_iNumMBYRef - 1) * m_iNumMBXRef].m_shpmd;
				}
				else if (m_volmd.volType == ENHN_LAYER){
					if(m_volmd.iEnhnType!=0 &&m_volmd.iuseRefShape ==1){
						shpmdColocatedMB = m_rgmbmdRef [min (max (0, iMBX), m_iNumMBXRef - 1)
							+ min (max (0, iMBY), m_iNumMBYRef - 1) * m_iNumMBXRef].m_shpmd;
					}else{
						Int index = min (max (0, (iMBX+xIndex)*m_volmd.ihor_sampling_factor_m_shape/m_volmd.ihor_sampling_factor_n_shape), (m_iNumMBBaseXRef-1)) + min (max (0, (iMBY+yIndex)*m_volmd.iver_sampling_factor_m_shape/m_volmd.iver_sampling_factor_n_shape), (m_iNumMBBaseYRef-1)) * (m_iNumMBBaseXRef);                       
						shpmdColocatedMB = m_rgBaseshpmd [index];
					}
				}
			}
			
			if ((pmbmd->m_bColocatedMBSkip && !(pmbmd->m_bColocatedMBMCSEL)) && // GMC
				(m_volmd.volType == BASE_LAYER || (m_volmd.volType == ENHN_LAYER && m_vopmd.iRefSelectCode == 3 && m_volmd.iEnhnType == 0)))
			{ 
				// don't need to send any bit for this mode
				copyToCurrBuffJustShape( ppxlcOrigMBBY, m_iFrameWidthY );
				m_statsMB.nBitsShape += codeInterShape (
					ppxlcCurrQMBBY,
					m_vopmd.fShapeBPredDir==B_FORWARD ? m_pvopcRefQ0 : m_pvopcRefQ1,
					pmbmd, shpmdColocatedMB,
					NULL, pmvBY,
					x, y, 
					iMBX, iMBY
					);
				//Changed HHI 2000-04-11
				decideTransparencyStatus (pmbmd, m_ppxlcCurrMBBY); // change pmbmd to inter if all transparent
				downSampleBY (m_ppxlcCurrMBBY, m_ppxlcCurrMBBUV, pmbmd);
				
				// reset motion vectors to zero because of skip
				memset (pmv, 0, BVOP_MV_PER_REF_PER_MB * sizeof (CMotionVector));
				memset (pmvBackward, 0, BVOP_MV_PER_REF_PER_MB * sizeof (CMotionVector));
				if (m_volmd.fAUsage == EIGHT_BIT)
					copyAlphaFromRefToCurrQ(m_pvopcRefQ0, x, y, pppxlcCurrQMBA, &m_rctRefVOPY0);
				copyFromRefToCurrQ(
					m_pvopcRefQ0,
					x, y, 
					ppxlcCurrQMBY, ppxlcCurrQMBU, ppxlcCurrQMBV, &m_rctRefVOPY0
					);
			}
			else {
				copyToCurrBuffWithShape (
					ppxlcOrigMBY, ppxlcOrigMBU, ppxlcOrigMBV, 
					ppxlcOrigMBBY, pppxlcOrigMBA,
					m_iFrameWidthY, m_iFrameWidthUV
					);
				
				// HHI Schueuer: added for sadct
				if (!m_volmd.bSadctDisable) {
					Int index=0;						
					//					if(!(m_volmd.volType == BASE_LAYER))
					if(!(m_volmd.volType == BASE_LAYER) && (m_volmd.bSpatialScalability && m_volmd.iHierarchyType==0))    //OBSSFIX_V2-8_after
						index = min (max (0,(iMBX+xIndex)*m_volmd.ihor_sampling_factor_m_shape/m_volmd.ihor_sampling_factor_n_shape),((m_iNumMBBaseXRef-1)))
						+ min (max (0,(iMBY+yIndex)*m_volmd.iver_sampling_factor_m_shape/m_volmd.iver_sampling_factor_n_shape),(m_iNumMBBaseYRef-1)) 
						* (m_iNumMBBaseXRef);	
					encodeBVOPMB_WithShape (
						ppxlcCurrQMBY, ppxlcCurrQMBU, ppxlcCurrQMBV, pppxlcCurrQMBA, ppxlcCurrQMBBY,
						pmbmd, pmv,	pmvBackward,
						pmvBY, shpmdColocatedMB,
						pmbmdRef, pmvRef,
						iMBX, iMBY,
						x, y, index,				//for OBSS
						m_ppxlcCurrMBBY, m_ppxlcCurrMBBUV);
				}
				else {
					Int index=0;						
					//					if(!(m_volmd.volType == BASE_LAYER))
					if(!(m_volmd.volType == BASE_LAYER) && (m_volmd.bSpatialScalability && m_volmd.iHierarchyType==0))    //OBSSFIX_V2-8_after
						index = min (max (0,(iMBX+xIndex)*m_volmd.ihor_sampling_factor_m_shape/m_volmd.ihor_sampling_factor_n_shape),((m_iNumMBBaseXRef-1)))
						+ min (max (0,(iMBY+yIndex)*m_volmd.iver_sampling_factor_m_shape/m_volmd.iver_sampling_factor_n_shape),(m_iNumMBBaseYRef-1)) 
						* (m_iNumMBBaseXRef);	
					
					encodeBVOPMB_WithShape (
						ppxlcCurrQMBY, ppxlcCurrQMBU, ppxlcCurrQMBV, pppxlcCurrQMBA, ppxlcCurrQMBBY,
						pmbmd, pmv,	pmvBackward,
						pmvBY, shpmdColocatedMB,
						pmbmdRef, pmvRef,
						iMBX, iMBY,
						x, y, index		//for OBSS 
						);
				}
				
			}
			//padding of bvop is necessary for temporal scalability: bvop in base layer is used for prediction in enhancement layer
			if(m_volmd.bShapeOnly == FALSE)	{
				// Added for field based MC padding by Hyundai(1998-5-9)
				if (!m_vopmd.bInterlace) {
					if (pmbmd -> m_rgTranspStatus [0] != ALL) {
						if (pmbmd -> m_rgTranspStatus [0] == PARTIAL)
							mcPadCurrMB (ppxlcCurrQMBY, ppxlcCurrQMBU, ppxlcCurrQMBV, pppxlcCurrQMBA);
						padNeighborTranspMBs (
							iMBX, iMBY,
							pmbmd,
							ppxlcCurrQMBY, ppxlcCurrQMBU, ppxlcCurrQMBV, pppxlcCurrQMBA
							);
					}
					else {
						padCurrAndTopTranspMBFromNeighbor (
							iMBX, iMBY,
							pmbmd,
							ppxlcCurrQMBY, ppxlcCurrQMBU, ppxlcCurrQMBV, pppxlcCurrQMBA
							); 
					}
				}
				// End of Hyundai(1998-5-9)
			}
			
			// update qp
			iQPPrev = pmbmd->m_stepSize;
			
			pmvBY++;
			pmbmd++;
			pmv         += BVOP_MV_PER_REF_PER_MB;
			pmvBackward += BVOP_MV_PER_REF_PER_MB;
			m_statsVOP += m_statsMB;
			ppxlcCurrQMBY += MB_SIZE;
			ppxlcCurrQMBU += BLOCK_SIZE;
			ppxlcCurrQMBV += BLOCK_SIZE;
			ppxlcCurrQMBBY += MB_SIZE;
			//ppxlcCurrQMBA += MB_SIZE;
			ppxlcOrigMBY += MB_SIZE;
			ppxlcOrigMBU += BLOCK_SIZE;
			ppxlcOrigMBV += BLOCK_SIZE;
			ppxlcOrigMBBY += MB_SIZE;
			//ppxlcOrigMBA += MB_SIZE;
			if (m_volmd.fAUsage == EIGHT_BIT) {  // MAC (SB) 26-Nov-99
				for(Int iAuxComp=0;iAuxComp<m_volmd.iAuxCompCount;iAuxComp++) {
					pppxlcCurrQMBA[iAuxComp] += MB_SIZE;
					pppxlcOrigMBA[iAuxComp] += MB_SIZE;
				}
			}
		}
		ppxlcCurrQY += m_iFrameWidthYxMBSize;
		ppxlcCurrQU += m_iFrameWidthUVxBlkSize;
		ppxlcCurrQV += m_iFrameWidthUVxBlkSize;
		ppxlcCurrQBY += m_iFrameWidthYxMBSize;
		//	ppxlcCurrQA += m_iFrameWidthYxMBSize;
		
		ppxlcOrigY += m_iFrameWidthYxMBSize;
		ppxlcOrigU += m_iFrameWidthUVxBlkSize;
		ppxlcOrigV += m_iFrameWidthUVxBlkSize;
		ppxlcOrigBY += m_iFrameWidthYxMBSize;
		//	ppxlcOrigA += m_iFrameWidthYxMBSize;
		
		if (m_volmd.fAUsage == EIGHT_BIT) { // MAC (SB) 26-Nov-99
			for(Int iAuxComp=0;iAuxComp<m_volmd.iAuxCompCount;iAuxComp++) { 
				pppxlcCurrQA[iAuxComp] += m_iFrameWidthYxMBSize;
				pppxlcOrigA[iAuxComp] += m_iFrameWidthYxMBSize;
			}
		}
	}
	// Added for field based MC padding by Hyundai(1998-5-9)
	if (m_vopmd.bInterlace && m_volmd.bShapeOnly == FALSE)
		fieldBasedMCPadding (field_pmbmd, m_pvopcCurrQ);
	// End of Hyundai(1998-5-9)
	//OBSS_SAIT_991015
	//OBSSFIX_MODE3
	if((m_volmd.bSpatialScalability && m_volmd.iHierarchyType==0) && m_volmd.volType == ENHN_LAYER &&!(m_volmd.iEnhnType != 0 && m_volmd.iuseRefShape == 1)) {        // if SpatialScalability
		//		if((m_volmd.bSpatialScalability && m_volmd.iHierarchyType==0) && m_volmd.volType == ENHN_LAYER) {	// if SpatialScalability
		//~OBSSFIX_MODE3
		delete m_pvopcRefQ1->getPlane (BY_PLANE)->m_pbHorSamplingChk;
		delete m_pvopcRefQ1->getPlane (BY_PLANE)->m_pbVerSamplingChk;
	}
	//~OBSS_SAIT_991015
	
    //MAC
    if (m_volmd.fAUsage == EIGHT_BIT) {
		delete [] pppxlcCurrQA;
		delete [] pppxlcOrigA;
		delete [] pppxlcCurrQMBA;
		delete [] pppxlcOrigMBA;
    }
    //~MAC
}



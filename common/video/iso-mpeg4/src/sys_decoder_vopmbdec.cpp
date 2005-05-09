/*************************************************************************

	This software module was originally developed by 
  
	Ming-Chieh Lee (mingcl@microsoft.com), Microsoft Corporation
	Wei-ge Chen (wchen@microsoft.com), Microsoft Corporation
	Bruce Lin (blin@microsoft.com), Microsoft Corporation
	Chuang Gu (chuanggu@microsoft.com), Microsoft Corporation
	Simon Winder (swinder@microsoft.com), Microsoft Corporation
	(date: March, 1996)
		
	and edited by
	Yoshihiro Kikuchi (TOSHIBA CORPORATION)
	Takeshi Nagai (TOSHIBA CORPORATION)
	Toshiaki Watanabe (TOSHIBA CORPORATION)
	Noboru Yamaguchi (TOSHIBA CORPORATION)
	  
	and also edited by
	David B. Shu (dbshu@hrl.com), Hughes Electronics/HRL Laboratories

	and edited by
	Xuemin Chen (xchen@gi.com), General Instrument Corp.

	and also edited by
	Dick van Smirren (D.vanSmirren@research.kpn.com), KPN Research
	Cor Quist (C.P.Quist@research.kpn.com), KPN Research
	(date: July, 1998)

	and also edited by
	Yoshinori Suzuki (Hitachi, Ltd.)

	and also edited by
	Hideaki Kimata (NTT)

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

	vopmbDec.cpp

	Abstract:

	Decoder for VOP composed of MB's

	Revision History:

	Nov. 26 , 1997: modified for error resilient by Toshiba

	Nov. 30 , 1997: modified for Spatial Scalable by Takefumi Nagumo
	(nagumo@av.crl.sony.co.jp) SONY Corporation 
	Dec 20, 1997:	Interlaced tools added by GI
	X. Chen (xchen@gi.com), B. Eifrig (beifrig@gi.com)
	May. 9   1998:  add boundary by Hyundai Electronics 
	Cheol-Soo Park (cspark@super5.hyundai.co.kr) 
	May. 9   1998:  add field based MC padding by Hyundai Electronics 
	Cheol-Soo Park (cspark@super5.hyundai.co.kr) 
	Feb.24   1999:  GMC added by Y. Suzuki (Hitachi, Ltd.) 
	Aug.24, 1999 : NEWPRED added by Hideaki Kimata (NTT) 
	Sep.06	1999 : RRV added by Eishi Morimatsu (Fujitsu Laboratories Ltd.) 
	Feb.01	2000 : Bug fixed OBSS by Takefumi Nagumo (SONY)
	May.25  2000 : MB stuffing decoding on the last MB by Hideaki Kimata (NTT)
									  
*************************************************************************/

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

// NEWPRED
#include "newpred.hpp"
// ~NEWPRED

#ifdef __MFC_
#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif
#define new DEBUG_NEW				   
#endif // __MFC_

Bool CVideoObjectDecoder::checkGOBMarker()
{
  Int iBitsLeft; // = 8 - (m_pbitstrmIn->getCounter() & 7);
  m_pbitstrmIn->peekBitsTillByteAlign(iBitsLeft);
	if(iBitsLeft==8)
		iBitsLeft = 0;
	UInt uiGOBMarker = m_pbitstrmIn->peekBits (17 + iBitsLeft);
	uiGOBMarker &= ((1 << 17) - 1);
	return (uiGOBMarker == 1);
}

Void CVideoObjectDecoder::decodeVOP ()	
{
	if (m_volmd.fAUsage != RECTANGLE) {
		if(m_volmd.bDataPartitioning && !m_volmd.bShapeOnly){
			if (m_vopmd.vopPredType == PVOP || (m_uiSprite == 2 && m_vopmd.vopPredType == SPRITE))
				decodePVOP_WithShape_DataPartitioning ();
			else if (m_vopmd.vopPredType == IVOP)
				decodeIVOP_WithShape_DataPartitioning ();
			else
				decodeBVOP_WithShape ();
		}
		else {
			if (m_vopmd.vopPredType == PVOP || (m_uiSprite == 2 && m_vopmd.vopPredType == SPRITE))
			{
				if (m_uiSprite == 1 && m_vopmd.SpriteXmitMode != STOP)
					decodePVOP ();
				else
					decodePVOP_WithShape ();
			}
			else if (m_vopmd.vopPredType == IVOP)
				decodeIVOP_WithShape ();
			else
				decodeBVOP_WithShape ();
		}
	}
	else {
		if(m_volmd.bDataPartitioning){
			if (m_vopmd.vopPredType == PVOP || (m_uiSprite == 2 && m_vopmd.vopPredType == SPRITE))
				decodePVOP_DataPartitioning ();
			else if (m_vopmd.vopPredType == IVOP)
				decodeIVOP_DataPartitioning ();
			else
				decodeBVOP ();
		}
		else {
			if (m_vopmd.vopPredType == PVOP || (m_uiSprite == 2 && m_vopmd.vopPredType == SPRITE))
				decodePVOP ();
			else if (m_vopmd.vopPredType == IVOP)
				decodeIVOP ();
			else
				decodeBVOP ();
		}
	}
}

Void CVideoObjectDecoder::decodeIVOP_WithShape ()	
{
	//in case the IVOP is used as an ref for direct mode
	memset (m_rgmv, 0, m_iNumMB * PVOP_MV_PER_REF_PER_MB * sizeof (CMotionVector));
	
	Int iMBX, iMBY;
	CMBMode* pmbmd = m_rgmbmd;
	// Added for field based MC padding by Hyundai(1998-5-9)
	CMBMode* field_pmbmd = m_rgmbmd;
	// End of Hyundai(1998-5-9)
	
	PixelC* ppxlcRefY  = (PixelC*) m_pvopcRefQ1->pixelsY () + m_iStartInRefToCurrRctY;
	PixelC* ppxlcRefU  = (PixelC*) m_pvopcRefQ1->pixelsU () + m_iStartInRefToCurrRctUV;
	PixelC* ppxlcRefV  = (PixelC*) m_pvopcRefQ1->pixelsV () + m_iStartInRefToCurrRctUV;
	PixelC* ppxlcRefBY = (PixelC*) m_pvopcRefQ1->pixelsBY () + m_iStartInRefToCurrRctY;
	//	PixelC* ppxlcRefA  = (PixelC*) m_pvopcRefQ1->pixelsA () + m_iStartInRefToCurrRctY;
	PixelC* ppxlcRefBUV = (PixelC*) m_pvopcRefQ1->pixelsBUV () + m_iStartInRefToCurrRctUV;
	
	pmbmd->m_bFieldDCT=0;	// new changes by X. Chen
	
	Int iCurrentQP  = m_vopmd.intStepI;	
	Int iCurrentQPA[MAX_MAC];
	for(Int iAuxComp=0; iAuxComp<m_volmd.iAuxCompCount; iAuxComp++ ) // MAC (SB) 2-Dec-99
		iCurrentQPA[iAuxComp] = m_vopmd.intStepIAlpha[iAuxComp];	
	//	Added for error resilient mode by Toshiba(1997-11-14)
	Int	iVideoPacketNumber = 0;
	m_iVPMBnum = 0;
	Bool bRestartDelayedQP = TRUE;
	
	PixelC** pppxlcRefMBA  = new PixelC* [m_volmd.iAuxCompCount];
	
	for(Int i=0; i<m_iNumMB; i++) {
		m_rgmvBaseBY[i].iMVX = 0; // for shape
		m_rgmvBaseBY[i].iMVY = 0; // for shape
		m_rgmvBaseBY[i].iHalfX = 0; // for shape
		m_rgmvBaseBY[i].iHalfY = 0; // for shape
	}
	if(m_volmd.volType == BASE_LAYER) 									
		m_rctBase = m_rctCurrVOPY;
	for (iMBY = 0; iMBY < m_iNumMBY; iMBY++) {
		PixelC* ppxlcRefMBY  = ppxlcRefY;
		PixelC* ppxlcRefMBU  = ppxlcRefU;
		PixelC* ppxlcRefMBV  = ppxlcRefV;
		PixelC* ppxlcRefMBBY = ppxlcRefBY;
		PixelC* ppxlcRefMBBUV = ppxlcRefBUV;
		
		Bool  bSptMB_NOT_HOLE= TRUE;
		if (m_uiSprite == 1 && m_sptMode != BASIC_SPRITE && m_vopmd.SpriteXmitMode != STOP) {
			bSptMB_NOT_HOLE = SptPieceMB_NOT_HOLE(0, iMBY, pmbmd);		 
			RestoreMBmCurrRow (iMBY, m_rgpmbmCurr);  // restore current row pointed by *m_rgpmbmCurr
		}
		
		for (iMBX = 0; iMBX < m_iNumMBX; iMBX++) {
			
			for(Int iAuxComp=0; iAuxComp<m_volmd.iAuxCompCount; iAuxComp++ ) { // MAC (SB) 1-Dec-99
				pppxlcRefMBA[iAuxComp] = ((PixelC*)m_pvopcRefQ1->pixelsA (iAuxComp) + m_iStartInRefToCurrRctY)
					+ iMBY*m_iFrameWidthYxMBSize + iMBX*MB_SIZE;
			}
			
			m_bSptMB_NOT_HOLE	 = 	bSptMB_NOT_HOLE;	
			if (!m_bSptMB_NOT_HOLE ) // current Sprite macroblock is not a hole and should be decoded			
				goto END_OF_DECODING1;
			
			//	Added for error resilient mode by Toshiba(1997-11-14)
			if	(checkResyncMarker())	{
				decodeVideoPacketHeader(iCurrentQP);
				iVideoPacketNumber++;
				bRestartDelayedQP = TRUE;
				//printf("VP");
			}
			pmbmd->m_iVideoPacketNumber = iVideoPacketNumber;

			decodeIntraShape (pmbmd, iMBX, iMBY, m_ppxlcCurrMBBY, ppxlcRefMBBY);

			downSampleBY (m_ppxlcCurrMBBY, m_ppxlcCurrMBBUV, pmbmd); // downsample original BY now for LPE padding (using original shape)
			if(m_volmd.bShapeOnly==FALSE)
			{
				pmbmd->m_bPadded=FALSE;
				if (pmbmd->m_rgTranspStatus [0] != ALL) {
					if (!m_volmd.bSadctDisable)
						deriveSADCTRowLengths(m_rgiCurrMBCoeffWidth, m_ppxlcCurrMBBY, m_ppxlcCurrMBBUV, pmbmd ->m_rgTranspStatus);

					decodeMBTextureHeadOfIVOP (pmbmd, iCurrentQP, &bRestartDelayedQP);

					if (!m_volmd.bSadctDisable)
						decodeTextureIntraMB (pmbmd, iMBX, iMBY, ppxlcRefMBY, ppxlcRefMBU, ppxlcRefMBV, m_ppxlcCurrMBBY, m_ppxlcCurrMBBUV);
					else
						decodeTextureIntraMB (pmbmd, iMBX, iMBY, ppxlcRefMBY, ppxlcRefMBU, ppxlcRefMBV);

					if ((pmbmd->m_rgTranspStatus [0] == NONE) && 
						(m_vopmd.bInterlace == TRUE) && (pmbmd->m_bFieldDCT == TRUE))
						fieldDCTtoFrameC(ppxlcRefMBY);

					if (m_volmd.fAUsage == EIGHT_BIT) {
						for(Int iAuxComp=0; iAuxComp<m_volmd.iAuxCompCount; iAuxComp++ ) { // MAC (SB) 1-Dec-99
							decodeMBAlphaHeadOfIVOP (pmbmd, iCurrentQP, iCurrentQPA[iAuxComp], m_vopmd.intStepI, m_vopmd.intStepIAlpha[iAuxComp], iAuxComp);

							if (!m_volmd.bSadctDisable)
								decodeAlphaIntraMB (pmbmd, iMBX, iMBY, pppxlcRefMBA[iAuxComp], iAuxComp, m_ppxlcCurrMBBY);
							else
								decodeAlphaIntraMB (pmbmd, iMBX, iMBY, pppxlcRefMBA[iAuxComp], iAuxComp );					
						}
					}
					if (m_uiSprite == 0 || m_uiSprite == 2 ||  m_sptMode == BASIC_SPRITE)         { //   delay the padding until ready for sprite warping // GMC
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
				}
				else {
					if (m_uiSprite == 0 || m_uiSprite == 2 || m_sptMode == BASIC_SPRITE) //  delay the padding until ready for sprite warping // GMC
						if (!m_vopmd.bInterlace) {
							padCurrAndTopTranspMBFromNeighbor (
								iMBX, iMBY,
								pmbmd,
								ppxlcRefMBY, ppxlcRefMBU, ppxlcRefMBV, pppxlcRefMBA
								);
						}
				}
			}
			
END_OF_DECODING1:
			//	ppxlcRefMBA += MB_SIZE;
			ppxlcRefMBBY += MB_SIZE;
			ppxlcRefMBBUV += BLOCK_SIZE;
			pmbmd++;
			ppxlcRefMBY += MB_SIZE;
			ppxlcRefMBU += BLOCK_SIZE;
			ppxlcRefMBV += BLOCK_SIZE;
			
			if (m_uiSprite == 1 && m_sptMode != BASIC_SPRITE && m_vopmd.SpriteXmitMode != STOP)
				bSptMB_NOT_HOLE = SptPieceMB_NOT_HOLE(iMBX+1, iMBY, pmbmd);	 			
			else
				bSptMB_NOT_HOLE= TRUE;
		}
		MacroBlockMemory** ppmbmTemp = m_rgpmbmAbove;
		m_rgpmbmAbove = m_rgpmbmCurr;
		
		if  (m_uiSprite == 1 && m_sptMode != BASIC_SPRITE)
			SaveMBmCurrRow (iMBY, m_rgpmbmCurr);		 //	  save current row pointed by *m_rgpmbmCurr 
		
		m_rgpmbmCurr  = ppmbmTemp;
		ppxlcRefY += m_iFrameWidthYxMBSize;
		ppxlcRefU += m_iFrameWidthUVxBlkSize;
		ppxlcRefV += m_iFrameWidthUVxBlkSize;
		ppxlcRefBY += m_iFrameWidthYxMBSize;
		//		ppxlcRefA += m_iFrameWidthYxMBSize;
		ppxlcRefBUV += m_iFrameWidthUVxBlkSize;
	}
	// Added for field based MC padding by Hyundai(1998-5-9)
	if (m_vopmd.bInterlace && m_volmd.bShapeOnly == FALSE)
		fieldBasedMCPadding (field_pmbmd, m_pvopcRefQ1);
	// End of Hyundai(1998-5-9)
	
	delete [] pppxlcRefMBA;
}

Void CVideoObjectDecoder::decodeIVOP ()	
{
	// NEWPRED
	char	pSlicePoint[128];
	pSlicePoint[0] = '0';
	pSlicePoint[1] = '0'; //NULL;
	// ~NEWPRED
	
	//in case the IVOP is used as an ref for direct mode
	memset (m_rgmv, 0, m_iNumMB * PVOP_MV_PER_REF_PER_MB * sizeof (CMotionVector));
	
	//	Int macrotellertje=0;  // [FDS]
	
	Int iMBX, iMBY;
	Int iMBXstart, iMBXstop, iMBYstart, iMBYstop; // Added by KPN for short headers
	UInt uiNumberOfGobs; // Added by KPN
	UInt uiGobNumber; // Added by KPN
	CMBMode* pmbmd = m_rgmbmd;
	pmbmd->m_stepSize = m_vopmd.intStepI;
	PixelC* ppxlcRefY = (PixelC*) m_pvopcRefQ1->pixelsY () + m_iStartInRefToCurrRctY;
	PixelC* ppxlcRefU = (PixelC*) m_pvopcRefQ1->pixelsU () + m_iStartInRefToCurrRctUV;
	PixelC* ppxlcRefV = (PixelC*) m_pvopcRefQ1->pixelsV () + m_iStartInRefToCurrRctUV;
	pmbmd->m_bFieldDCT = 0;
	
	Int iCurrentQP  = m_vopmd.intStepI;		
	Int	iVideoPacketNumber = 0; 			//	added for error resilience mode by Toshiba
	m_iVPMBnum = 0;	//	Added for error resilient mode by Toshiba(1997-11-14)
	
	if (!short_video_header) { 
		uiNumberOfGobs = 1;
		iMBXstart=0; 
		iMBXstop= m_iNumMBX;
		iMBYstart=0;
		iMBYstop= m_iNumMBY;
	}
	else { // short_header
		uiNumberOfGobs = uiNumGobsInVop;
		iMBXstart=0; 
		iMBXstop= 0;
		iMBYstart=0;
		iMBYstop= 0;
	}
	
	Bool bRestartDelayedQP = TRUE; // decodeMBTextureHeadOfIVOP sets this to false
	
	uiGobNumber=0;
	while (uiGobNumber < uiNumberOfGobs) { 
		if (short_video_header) {
			uiGobHeaderEmpty=1;
			if (uiGobNumber != 0) {
				skipAnyStuffing();
				if (checkGOBMarker()) {
					uiGobHeaderEmpty=0;
					m_pbitstrmIn -> flush();
					/*UInt uiGobResyncMarker= */m_pbitstrmIn -> getBits (17);
					uiGobNumber=m_pbitstrmIn -> getBits(5);
					/* UInt uiGobFrameId = */m_pbitstrmIn -> getBits(2);
					/* UInt uiVopQuant= */m_pbitstrmIn -> getBits(5); 
					uiGobNumber++; 
				} else {
					uiGobNumber++;
				}
			} else {
				uiGobNumber++;
			}
			
			iMBXstart=0; 
			iMBXstop= m_ivolWidth/16;
			iMBYstart=(uiGobNumber*(m_ivolHeight/16)/uiNumberOfGobs)-1;
			iMBYstop= iMBYstart+(m_ivolHeight/16)/uiNumberOfGobs;
		} else {
			uiGobNumber++; 
		}
		
		for (iMBY = iMBYstart; iMBY < iMBYstop; iMBY++) { // [FDS]
			PixelC* ppxlcRefMBY = ppxlcRefY;
			PixelC* ppxlcRefMBU = ppxlcRefU;
			PixelC* ppxlcRefMBV = ppxlcRefV;
			
			Bool  bSptMB_NOT_HOLE= TRUE;
			if (m_uiSprite == 1 && m_sptMode != BASIC_SPRITE && m_vopmd.SpriteXmitMode != STOP) {
				
				bSptMB_NOT_HOLE = SptPieceMB_NOT_HOLE(0, iMBY, pmbmd);		 
				RestoreMBmCurrRow (iMBY, m_rgpmbmCurr);  // restore current row pointed by *m_rgpmbmCurr
			}
			
			for (iMBX = iMBXstart; iMBX < iMBXstop; iMBX++) {	
				
				m_bSptMB_NOT_HOLE	 = 	bSptMB_NOT_HOLE;	
				if (!m_bSptMB_NOT_HOLE ) 	// current Sprite macroblock is not a hole and should be decoded			
					goto END_OF_DECODING2;
				
				skipAnyStuffing();
				if	(checkResyncMarker())	{
					decodeVideoPacketHeader(iCurrentQP);
					iVideoPacketNumber++;
					bRestartDelayedQP = TRUE;
					
					// NEWPRED
					if (m_volmd.bNewpredEnable && (m_volmd.bNewpredSegmentType == 0))
						// RRV modification
						sprintf(pSlicePoint, "%s,%d",pSlicePoint, (iMBY *m_iRRVScale)*(m_iNumMBX *m_iRRVScale)+(iMBX *m_iRRVScale));
					//						sprintf(pSlicePoint, "%s,%d",pSlicePoint, iMBY*m_iNumMBX+iMBX);
					// ~RRV
					else
						pSlicePoint[0] = '1';
					// ~NEWPRED
				}
				pmbmd->m_iVideoPacketNumber = iVideoPacketNumber;	//mv out of if by wchen to set even when errR is off; always used in mbdec 
				decodeMBTextureHeadOfIVOP (pmbmd, iCurrentQP, &bRestartDelayedQP);
				decodeTextureIntraMB (pmbmd, iMBX, iMBY, ppxlcRefMBY, ppxlcRefMBU, ppxlcRefMBV);
				// INTERLACE
				if ((m_vopmd.bInterlace == TRUE) && (pmbmd->m_bFieldDCT == TRUE))
					fieldDCTtoFrameC(ppxlcRefMBY);
				// ~INTERLACE
				
END_OF_DECODING2:
				pmbmd++;
				ppxlcRefMBY += (MB_SIZE *m_iRRVScale);
				ppxlcRefMBU += (BLOCK_SIZE *m_iRRVScale);
				ppxlcRefMBV += (BLOCK_SIZE *m_iRRVScale);
				
				if (m_uiSprite == 1 && m_sptMode != BASIC_SPRITE && m_vopmd.SpriteXmitMode != STOP)  // get the hole status for the next MB
					
					bSptMB_NOT_HOLE = SptPieceMB_NOT_HOLE(iMBX+1, iMBY, pmbmd);	 			
				else
					bSptMB_NOT_HOLE= TRUE;
			}
			
			// May.25 2000 for MB stuffing decoding on the last MB
			if	(checkStartCode()) { // this MB contains only MCBPC stuffing
				break;
			}
			// ~May.25 2000 for MB stuffing decoding on the last MB
			
			MacroBlockMemory** ppmbmTemp = m_rgpmbmAbove;
			m_rgpmbmAbove = m_rgpmbmCurr;
			
			if  (m_uiSprite == 1 && m_sptMode != BASIC_SPRITE)
				SaveMBmCurrRow (iMBY, m_rgpmbmCurr);		 //	  save current row pointed by *m_rgpmbmCurr 
			
			m_rgpmbmCurr  = ppmbmTemp;
			ppxlcRefY += m_iFrameWidthYxMBSize;
			ppxlcRefU += m_iFrameWidthUVxBlkSize;
			ppxlcRefV += m_iFrameWidthUVxBlkSize;
		}
	} // KPN Terminate while loop Gob layer [FDS]
	
	skipAnyStuffing();
	
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
		int iCurrentVOP_id = g_pNewPredDec->GetCurrentVOP_id();
		if (g_pNewPredDec != NULL) delete g_pNewPredDec;
		g_pNewPredDec = new CNewPredDecoder();
		g_pNewPredDec->SetObject(
			m_iNumBitsTimeIncr,
			m_iNumMBX*(MB_SIZE *m_iRRVScale),
			m_iNumMBY*(MB_SIZE *m_iRRVScale),
			pSlicePoint,
			m_volmd.bNewpredSegmentType,
			m_volmd.fAUsage,
			m_volmd.bShapeOnly,
			m_pvopcRefQ0,
			m_pvopcRefQ1,
			m_rctRefFrameY,
			m_rctRefFrameUV
			);
		g_pNewPredDec->ResetObject(iCurrentVOP_id);
		
		Int i;
		Int noStore_vop_id;
		
		g_pNewPredDec->SetQBuf( m_pvopcRefQ0, m_pvopcRefQ1 );
		for (i=0; i < g_pNewPredDec->m_iNumSlice; i++ ) {
			noStore_vop_id = g_pNewPredDec->make_next_decbuf(g_pNewPredDec->m_pNewPredControl, 
				g_pNewPredDec->GetCurrentVOP_id(), i);
		}
	}
	// ~NEWPRED
}

Void CVideoObjectDecoder::decodePVOP_WithShape ()
{
	Int iMBX, iMBY;
	CoordI y = m_rctCurrVOPY.top; 
	CMBMode* pmbmd = m_rgmbmd;
	// Added for field based MC padding by Hyundai(1998-5-9)
    CMBMode* field_pmbmd = m_rgmbmd;
	// End of Hyundai(1998-5-9)
	CMotionVector* pmv = m_rgmv;
	CMotionVector* pmvBY = m_rgmvBY;
	
	//OBSS_SAIT_991015
	if(m_volmd.volType == BASE_LAYER) {										
		m_rgmvBaseBY = m_rgmvBY;
		m_rctBase = m_rctCurrVOPY;
	}
	Int xIndex, yIndex;
	xIndex = yIndex = 0;
	if(m_volmd.volType == ENHN_LAYER && m_volmd.bSpatialScalability) {
		xIndex = (m_rctCurrVOPY.left - (m_rctBase.left*m_volmd.ihor_sampling_factor_n_shape/m_volmd.ihor_sampling_factor_m_shape));	
		yIndex = (m_rctCurrVOPY.top - (m_rctBase.top*m_volmd.iver_sampling_factor_n_shape/m_volmd.iver_sampling_factor_m_shape));
		xIndex /= MB_SIZE;
		yIndex /= MB_SIZE;
	}
	//~OBSS_SAIT_991015
	
	PixelC* ppxlcCurrQY  = (PixelC*) m_pvopcRefQ1->pixelsY () + m_iStartInRefToCurrRctY;
	PixelC* ppxlcCurrQU  = (PixelC*) m_pvopcRefQ1->pixelsU () + m_iStartInRefToCurrRctUV;
	PixelC* ppxlcCurrQV  = (PixelC*) m_pvopcRefQ1->pixelsV () + m_iStartInRefToCurrRctUV;
	PixelC* ppxlcCurrQBY = (PixelC*) m_pvopcRefQ1->pixelsBY () + m_iStartInRefToCurrRctY;
	//	PixelC* ppxlcCurrQA  = (PixelC*) m_pvopcRefQ1->pixelsA () + m_iStartInRefToCurrRctY;
	PixelC** pppxlcCurrQMBA = new PixelC* [m_volmd.iAuxCompCount];  // MAC (SB) 1-Dec-99
	
	Int iCurrentQP  = m_vopmd.intStep;	
	Int iCurrentQPA[MAX_MAC];
	for(Int iAuxComp=0; iAuxComp<m_volmd.iAuxCompCount; iAuxComp++ )  // MAC (SB) 2-Dec-99
		iCurrentQPA[iAuxComp] = m_vopmd.intStepPAlpha[iAuxComp];	
	//	Added for error resilient mode by Toshiba(1997-11-14)
	Int	iVideoPacketNumber = 0; 			//	added for error resilience mode by Toshiba
	m_iVPMBnum = 0;
	
	Bool bRestartDelayedQP = TRUE;
	
	//	End Toshiba(1997-11-14)
	
	for (iMBY = 0; iMBY < m_iNumMBY; iMBY++, y += MB_SIZE) {
		PixelC* ppxlcCurrQMBY  = ppxlcCurrQY;
		PixelC* ppxlcCurrQMBU  = ppxlcCurrQU;
		PixelC* ppxlcCurrQMBV  = ppxlcCurrQV;
		PixelC* ppxlcCurrQMBBY = ppxlcCurrQBY;
		//		PixelC* ppxlcCurrQMBA  = ppxlcCurrQA;
		//	Added for error resilient mode by Toshiba(1997-11-14)
		if	(iMBY != 0)	{
			if	(checkResyncMarker()) {
				decodeVideoPacketHeader(iCurrentQP);
				iVideoPacketNumber++;
				bRestartDelayedQP = TRUE;
			}
		}
		pmbmd->m_iVideoPacketNumber = iVideoPacketNumber;
		//	End Toshiba(1997-11-14)
		// Modified for error resilient mode by Toshiba(1997-11-14)
		ShapeMode shpmdColocatedMB;
		if(m_vopmd.bShapeCodingType) {
			//OBSS_SAIT_991015
			//OBSSFIX_MODE3
            if(!(m_volmd.bSpatialScalability && m_volmd.iHierarchyType == 0)){
				//			if(!(m_volmd.bSpatialScalability)){
				//~OBSSFIX_MODE3
				shpmdColocatedMB = m_rgmbmdRef [
					MIN (iMBY, m_iNumMBYRef-1) * m_iNumMBXRef
					].m_shpmd;
				decodeInterShape (m_pvopcRefQ0, pmbmd, 0, iMBY,
					m_rctCurrVOPY.left, y, pmv, pmvBY,
					m_ppxlcCurrMBBY, ppxlcCurrQMBBY,
					shpmdColocatedMB);
			}
			else {
				//OBSSFIX_MODE3
				if(m_volmd.volType == ENHN_LAYER && m_volmd.iEnhnType==1 && m_volmd.iuseRefShape == 1 ){
					shpmdColocatedMB = ALL_OPAQUE;
					pmv->setToZero();		
					pmvBY->setToZero();		
                    decodeInterShape(m_pvopcRefQ0, pmbmd, 0, iMBY,
						m_rctCurrVOPY.left, y, pmv, pmvBY,
						m_ppxlcCurrMBBY, ppxlcCurrQMBBY,
						shpmdColocatedMB);
				} else 
					//~OBSSFIX_MODE3
					if((m_volmd.volType == BASE_LAYER) || (!(m_volmd.iEnhnType==0 || m_volmd.iuseRefShape ==0) && !m_volmd.bShapeOnly) ){			
						shpmdColocatedMB = m_rgmbmdRef [
							MIN (iMBY, m_iNumMBYRef-1) * m_iNumMBXRef
							].m_shpmd;
						decodeInterShape (m_pvopcRefQ0, pmbmd, 0, iMBY,
							m_rctCurrVOPY.left, y, pmv, pmvBY,
							m_ppxlcCurrMBBY, ppxlcCurrQMBBY,
							shpmdColocatedMB);
					}
					else if(m_volmd.volType == ENHN_LAYER) { // for spatial scalability
						Int index = MIN (MAX (0, (iMBY+yIndex)*m_volmd.iver_sampling_factor_m_shape/m_volmd.iver_sampling_factor_n_shape), (m_iNumMBBaseYRef-1)) * m_iNumMBBaseXRef; 
						shpmdColocatedMB = m_rgBaseshpmd [index];
						decodeSIShapePVOP (m_pvopcRefQ0, pmbmd, 0, iMBY,
							m_rctCurrVOPY.left, y, pmv, pmvBY, (m_rgmvBaseBY+index), 
							m_ppxlcCurrMBBY, ppxlcCurrQMBBY,
							shpmdColocatedMB);
					}
			}
			//~OBSS_SAIT_991015		
		}
		else {
			decodeIntraShape (pmbmd, 0, iMBY, m_ppxlcCurrMBBY,
				ppxlcCurrQMBBY);
		}
		// End Toshiba(1997-11-14)
		// Changed HHI 2000-04-11
		downSampleBY (m_ppxlcCurrMBBY, m_ppxlcCurrMBBUV, pmbmd);
		if (pmbmd->m_rgTranspStatus [0] != ALL && m_volmd.bShapeOnly==FALSE) {
			decodeMBTextureHeadOfPVOP (pmbmd, iCurrentQP, &bRestartDelayedQP);
			// GMC
			if(!pmbmd -> m_bMCSEL)
				// ~GMC
				decodeMVWithShape (pmbmd, 0, iMBY, pmv);
			// GMC
			else
			{
				Int iPmvx, iPmvy, iHalfx, iHalfy;
				globalmv (iPmvx, iPmvy, iHalfx, iHalfy,
					m_rctCurrVOPY.left,y,
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
			// ~GMC
			if(pmbmd->m_bhas4MVForward)
				padMotionVectors(pmbmd,pmv);
		}
		CoordI x = m_rctCurrVOPY.left;
		
		for (iMBX = 0; iMBX < m_iNumMBX; iMBX++, x += MB_SIZE) {
			for(Int iAuxComp=0; iAuxComp<m_volmd.iAuxCompCount; iAuxComp++ ) { // MAC (SB) 1-Dec-99                                   
				pppxlcCurrQMBA[iAuxComp] = ((PixelC*)m_pvopcRefQ1->pixelsA (iAuxComp) + m_iStartInRefToCurrRctY)
					+ iMBY*m_iFrameWidthYxMBSize + iMBX*MB_SIZE;
			}	
			
			pmbmd->m_bPadded = FALSE;
			if (pmbmd->m_rgTranspStatus [0] != ALL && m_volmd.bShapeOnly==FALSE) {
				// HHI Schueuer sadct support		
				if (!m_volmd.bSadctDisable)
					deriveSADCTRowLengths(m_rgiCurrMBCoeffWidth, m_ppxlcCurrMBBY, m_ppxlcCurrMBBUV, pmbmd ->m_rgTranspStatus);
				// end HHI   
				if ((pmbmd->m_dctMd == INTRA || pmbmd->m_dctMd == INTRAQ) && !pmbmd->m_bSkip) {
					// HHI Schueuer: sadct
					if (!m_volmd.bSadctDisable)
						decodeTextureIntraMB (pmbmd, iMBX, iMBY, ppxlcCurrQMBY, ppxlcCurrQMBU, ppxlcCurrQMBV, m_ppxlcCurrMBBY, m_ppxlcCurrMBBUV);
					else
						decodeTextureIntraMB (pmbmd, iMBX, iMBY, ppxlcCurrQMBY, ppxlcCurrQMBU, ppxlcCurrQMBV);
					// end HHI
					// decodeTextureIntraMB (pmbmd, iMBX, iMBY, ppxlcCurrQMBY, ppxlcCurrQMBU, ppxlcCurrQMBV);
					// INTERLACE
					//new changes
					if ((pmbmd->m_rgTranspStatus [0] == NONE) && 
						(m_vopmd.bInterlace == TRUE) && (pmbmd->m_bFieldDCT == TRUE))
						fieldDCTtoFrameC(ppxlcCurrQMBY);
					//end of new changes
					// ~INTERLACE
					if (m_volmd.fAUsage == EIGHT_BIT) {
						for(Int iAuxComp=0; iAuxComp<m_volmd.iAuxCompCount; iAuxComp++ ) { // MAC (SB) 1-Dec-99
							decodeMBAlphaHeadOfPVOP (pmbmd, iCurrentQP, iCurrentQPA[iAuxComp], iAuxComp);
							// HHI Schueuer
							if (!m_volmd.bSadctDisable)
								decodeAlphaIntraMB (pmbmd, iMBX, iMBY, pppxlcCurrQMBA[iAuxComp], iAuxComp, m_ppxlcCurrMBBY);
							else
								decodeAlphaIntraMB (pmbmd, iMBX, iMBY, pppxlcCurrQMBA[iAuxComp], iAuxComp);
						}
						// end HHI						
					}
				}
				else {
					if (!pmbmd->m_bSkip) {
						// HHI Schueuer: sadct
						if (!m_volmd.bSadctDisable)
							decodeTextureInterMB (pmbmd, m_ppxlcCurrMBBY, m_ppxlcCurrMBBUV);
						else
							decodeTextureInterMB (pmbmd);
						// end HHI
						// decodeTextureInterMB (pmbmd);
						// INTERLACE
						//new changes
						if ((pmbmd->m_rgTranspStatus [0] == NONE) && 
							(m_vopmd.bInterlace == TRUE) && (pmbmd->m_bFieldDCT == TRUE))
							fieldDCTtoFrameI(m_ppxliErrorMBY);
						//end of new changes
						// ~INTERLACE
					}
					if (m_volmd.fAUsage == EIGHT_BIT) {
						for(Int iAuxComp=0; iAuxComp<m_volmd.iAuxCompCount; iAuxComp++ ) { // MAC (SB) 1-Dec-99
							decodeMBAlphaHeadOfPVOP (pmbmd, iCurrentQP, iCurrentQPA[iAuxComp], iAuxComp);
							// HHI Schueuer
							if (!m_volmd.bSadctDisable)
								decodeAlphaInterMB (pmbmd, pppxlcCurrQMBA[iAuxComp], iAuxComp, m_ppxlcCurrMBBY);
							else
								decodeAlphaInterMB (pmbmd, pppxlcCurrQMBA[iAuxComp], iAuxComp);
							// end HHI						
						}
					}
				}
			}
			// decode shape, overhead, and MV for the right MB
			if (iMBX != m_iNumMBX - 1) {
				CMBMode* pmbmdRight = pmbmd + 1;
				//	Added for error resilient mode by Toshiba(1997-11-14)
				if (checkResyncMarker())	{
					decodeVideoPacketHeader(iCurrentQP);
					iVideoPacketNumber++;
					bRestartDelayedQP = TRUE;
				}
				pmbmdRight->m_iVideoPacketNumber = iVideoPacketNumber;
				//	End Toshiba(1997-11-14)
				// Modified for error resilient mode by Toshiba(1997-11-14)
				if(m_vopmd.bShapeCodingType) {
					//OBSS_SAIT_991015
					//OBSSFIX_MODE3
                    if(!(m_volmd.bSpatialScalability && m_volmd.iHierarchyType== 0)){       
						//                  if(!(m_volmd.bSpatialScalability)){     
						//~OBSSFIX_MODE3
						shpmdColocatedMB = m_rgmbmdRef [
							MIN (MAX (0, iMBX+1), m_iNumMBXRef-1) + 
							MIN (MAX (0, iMBY), m_iNumMBYRef-1) * m_iNumMBXRef
							].m_shpmd;
						decodeInterShape (
							m_pvopcRefQ0,
							pmbmdRight, 
							iMBX + 1, iMBY, 
							x + MB_SIZE, y, 
							pmv + PVOP_MV_PER_REF_PER_MB, pmvBY + 1, 
							m_ppxlcRightMBBY,
							ppxlcCurrQMBBY + MB_SIZE, 
							shpmdColocatedMB
							);
					}
					else {
						//OBSSFIX_MODE3
						if(m_volmd.volType == ENHN_LAYER && m_volmd.iEnhnType==1 && m_volmd.iuseRefShape == 1 ){
							shpmdColocatedMB = ALL_OPAQUE;
                            decodeInterShape (  m_pvopcRefQ0,
								pmbmdRight, 
								iMBX + 1, iMBY, 
								x + MB_SIZE, y, 
								pmv + PVOP_MV_PER_REF_PER_MB, pmvBY + 1, 
								m_ppxlcRightMBBY,
								ppxlcCurrQMBBY + MB_SIZE, 
								shpmdColocatedMB);
						}else 
							//~OBSSFIX_MODE3          
							if((m_volmd.volType == BASE_LAYER) || (!(m_volmd.iEnhnType==0 || m_volmd.iuseRefShape ==0) && !m_volmd.bShapeOnly)){		
								shpmdColocatedMB = m_rgmbmdRef [
									MIN (MAX (0, iMBX+1), m_iNumMBXRef-1) + 
									MIN (MAX (0, iMBY), m_iNumMBYRef-1) * m_iNumMBXRef
									].m_shpmd;
								decodeInterShape (
									m_pvopcRefQ0,
									pmbmdRight, 
									iMBX + 1, iMBY, 
									x + MB_SIZE, y, 
									pmv + PVOP_MV_PER_REF_PER_MB, pmvBY + 1, 
									m_ppxlcRightMBBY,
									ppxlcCurrQMBBY + MB_SIZE, 
									shpmdColocatedMB
									);
							}
							else if(m_volmd.volType == ENHN_LAYER) { // for spatial scalability
								Int index = MIN (MAX (0, ((iMBX+xIndex+1)*m_volmd.ihor_sampling_factor_m_shape/m_volmd.ihor_sampling_factor_n_shape)), (m_iNumMBBaseXRef-1)) +				
									MIN (MAX (0, (iMBY+yIndex)*m_volmd.iver_sampling_factor_m_shape/m_volmd.iver_sampling_factor_n_shape), (m_iNumMBBaseYRef-1)) * m_iNumMBBaseXRef;		
								shpmdColocatedMB = m_rgBaseshpmd [index];
								decodeSIShapePVOP (
									m_pvopcRefQ0,
									pmbmdRight, 
									iMBX + 1, iMBY, 
									x + MB_SIZE, y, 
									pmv + PVOP_MV_PER_REF_PER_MB, pmvBY + 1, 
									m_rgmvBaseBY,
									m_ppxlcRightMBBY,
									ppxlcCurrQMBBY + MB_SIZE, 
									shpmdColocatedMB								
									);
							}
					}
					//~OBSS_SAIT_991015
				}
				else {
					decodeIntraShape (
						pmbmdRight, iMBX + 1, iMBY,
						m_ppxlcRightMBBY,
						ppxlcCurrQMBBY + MB_SIZE
						);
				}
				// End Toshiba(1997-11-14)
				// Changed HHI 2000-04-11
				downSampleBY (m_ppxlcRightMBBY, m_ppxlcRightMBBUV, pmbmdRight);
				if (pmbmdRight->m_rgTranspStatus [0] != ALL && m_volmd.bShapeOnly==FALSE) {
					decodeMBTextureHeadOfPVOP (pmbmdRight, iCurrentQP, &bRestartDelayedQP);
					// GMC
					if(!pmbmdRight -> m_bMCSEL)
						// ~GMC
						decodeMVWithShape (pmbmdRight, iMBX + 1, iMBY, pmv + PVOP_MV_PER_REF_PER_MB);
					// GMC
					else
					{
						Int iPmvx, iPmvy, iHalfx, iHalfy;
						globalmv (iPmvx, iPmvy, iHalfx, iHalfy,
							x+16,y,m_vopmd.mvInfoForward.uiRange,
							m_volmd.bQuarterSample);
						CMotionVector* pmvcurr = pmv + PVOP_MV_PER_REF_PER_MB;
						*pmvcurr = CMotionVector (iPmvx, iPmvy);
						pmvcurr -> iHalfX = iHalfx;
						pmvcurr -> iHalfY = iHalfy;
						pmvcurr -> computeTrueMV ();
						pmvcurr -> computeMV ();
						for (UInt i = 1; i < PVOP_MV_PER_REF_PER_MB; i++)
							pmvcurr[i] = *pmvcurr;
					}
					// ~GMC
					if(pmbmdRight->m_bhas4MVForward)
						padMotionVectors (pmbmdRight,pmv + PVOP_MV_PER_REF_PER_MB);
				}
			}
			
			if(m_volmd.bShapeOnly==FALSE) {
				if (pmbmd->m_rgTranspStatus [0] != ALL) {
					if (pmbmd->m_dctMd == INTER || pmbmd->m_dctMd == INTERQ)	{
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
							// INTERLACE //new changes
							if(pmbmd->m_bFieldMV) {
								CoordI iXRefUV1, iYRefUV1;
								mvLookupUV (pmbmd, pmv, iXRefUV, iYRefUV, iXRefUV1, iYRefUV1);
								motionCompFieldUV(m_ppxlcPredMBU, m_ppxlcPredMBV, m_pvopcRefQ0, x, y,
									iXRefUV, iYRefUV, pmbmd->m_bForwardTop,
									&m_rctRefVOPY0); // added by Y.Suzuki for the extended bounding box support
								motionCompFieldUV(m_ppxlcPredMBU + BLOCK_SIZE, m_ppxlcPredMBV + BLOCK_SIZE,
									m_pvopcRefQ0, x, y, iXRefUV1, iYRefUV1, pmbmd->m_bForwardBottom,
									&m_rctRefVOPY0); // added by Y.Suzuki for the extended bounding box support
							}
							// GMC
							else if(pmbmd->m_bMCSEL) {
								FindGlobalChromPredForGMC(x,y,m_ppxlcPredMBU,m_ppxlcPredMBV);
							}
							// ~GMC
							else {
								// INTERALCE //end of new changes		
								// GMC
								if (!pmbmd->m_bMCSEL)
									// ~GMC
									mvLookupUVWithShape (pmbmd, pmv, iXRefUV, iYRefUV);
								motionCompUV (m_ppxlcPredMBU, m_ppxlcPredMBV, m_pvopcRefQ0,
									x, y, iXRefUV, iYRefUV, m_vopmd.iRoundingControl,&m_rctRefVOPY0);
							}
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
						
						if (m_volmd.fAUsage == EIGHT_BIT) {
							for(Int iAuxComp=0; iAuxComp<m_volmd.iAuxCompCount; iAuxComp++ ) { // MAC (SB) 1-Dec-99
								if (pmbmd->m_pCODAlpha[iAuxComp]!=ALPHA_ALL255)
								{
									motionCompMB (
										m_ppxlcPredMBA[iAuxComp], m_pvopcRefQ0->pixelsA (iAuxComp),
										pmv, pmbmd, 
										iMBX, iMBY, 
										x, y,
										FALSE, TRUE,
										&m_rctRefVOPY0
										);
									
									if(pmbmd->m_pCODAlpha[iAuxComp]==ALPHA_SKIPPED)
										assignAlphaPredToCurrQ (pppxlcCurrQMBA[iAuxComp],iAuxComp);
									else
										addAlphaErrorAndPredToCurrQ (pppxlcCurrQMBA[iAuxComp],iAuxComp);
								}
							}
						}
					}
					// Added for field based MC padding by Hyundai(1998-5-9)
					if (!m_vopmd.bInterlace) {
						if (pmbmd->m_rgTranspStatus [0] == PARTIAL)
							mcPadCurrMB (ppxlcCurrQMBY, ppxlcCurrQMBU, ppxlcCurrQMBV, pppxlcCurrQMBA);
						padNeighborTranspMBs (
							iMBX, iMBY,
							pmbmd,
							ppxlcCurrQMBY, ppxlcCurrQMBU, ppxlcCurrQMBV, pppxlcCurrQMBA
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
							ppxlcCurrQMBY, ppxlcCurrQMBU, ppxlcCurrQMBV, pppxlcCurrQMBA
							);
					}
					// End of Hyundai(1998-5-9)
				}
			}
			if (iMBX != m_iNumMBX - 1)
				swapCurrAndRightMBForShape ();
			
			pmbmd++;
			pmv += PVOP_MV_PER_REF_PER_MB;
			pmvBY++;
			ppxlcCurrQMBY += MB_SIZE;
			ppxlcCurrQMBU += BLOCK_SIZE;
			ppxlcCurrQMBV += BLOCK_SIZE;
			ppxlcCurrQMBBY += MB_SIZE;
			//			ppxlcCurrQMBA += MB_SIZE;
		}
		MacroBlockMemory** ppmbmTemp = m_rgpmbmAbove;
		m_rgpmbmAbove = m_rgpmbmCurr;
		m_rgpmbmCurr  = ppmbmTemp;
		
		ppxlcCurrQY  += m_iFrameWidthYxMBSize;
		ppxlcCurrQU  += m_iFrameWidthUVxBlkSize;
		ppxlcCurrQV  += m_iFrameWidthUVxBlkSize;
		ppxlcCurrQBY += m_iFrameWidthYxMBSize;
		//		ppxlcCurrQA  += m_iFrameWidthYxMBSize;
	}
	// Added for field based MC padding by Hyundai(1998-5-9)
	if (m_vopmd.bInterlace && m_volmd.bShapeOnly == FALSE)
		fieldBasedMCPadding (field_pmbmd, m_pvopcRefQ1);
    if(m_volmd.bSpatialScalability && m_volmd.volType == ENHN_LAYER && m_volmd.iHierarchyType==0 &&
		!(m_volmd.iEnhnType != 0 && m_volmd.iuseRefShape == 1) ){ // if SpatialScalability
        delete m_pvopcRefQ0->getPlane (BY_PLANE)->m_pbHorSamplingChk;
        delete m_pvopcRefQ0->getPlane (BY_PLANE)->m_pbVerSamplingChk;
    }
    delete [] pppxlcCurrQMBA;
}

Void CVideoObjectDecoder::decodePVOP ()
{
	Int iMBX, iMBY;
	CoordI y = 0; 
	Int iMBXstart, iMBXstop, iMBYstart, iMBYstop; // added by KPN [FDS]
	UInt uiNumberOfGobs;
	Bool bFirstGobRow;
	CMBMode* pmbmd = m_rgmbmd;
	CMotionVector* pmv = m_rgmv;
	PixelC* ppxlcCurrQY = (PixelC*) m_pvopcRefQ1->pixelsY () + m_iStartInRefToCurrRctY;
	PixelC* ppxlcCurrQU = (PixelC*) m_pvopcRefQ1->pixelsU () + m_iStartInRefToCurrRctUV;
	PixelC* ppxlcCurrQV = (PixelC*) m_pvopcRefQ1->pixelsV () + m_iStartInRefToCurrRctUV;
	// sprite update piece uses binary mask of the object piece
	PixelC* ppxlcCurrQBY = (PixelC*) m_pvopcRefQ1->pixelsBY () + m_iStartInRefToCurrRctY; 
	
	Int iCurrentQP  = m_vopmd.intStep;
	Int	iVideoPacketNumber = 0;
	//	Added for error resilient mode by Toshiba(1997-11-14)
	m_iVPMBnum = 0;
	//	End Toshiba(1997-11-14)
	Bool bLeftBndry;
	Bool bRightBndry;
	Bool bTopBndry;
	Bool bZeroMV = (m_volmd.volType == ENHN_LAYER && m_vopmd.iRefSelectCode == 3) ? TRUE : FALSE;
	
	bFirstGobRow=FALSE;
	
	Bool bRestartDelayedQP = TRUE;
	
	// NEWPRED
	const PixelC* RefbufY = m_pvopcRefQ0-> pixelsY ();
	const PixelC* RefbufU = m_pvopcRefQ0-> pixelsU ();
	const PixelC* RefbufV = m_pvopcRefQ0-> pixelsV ();
	PixelC *RefpointY, *RefpointU, *RefpointV;
	PixelC *pRefpointY, *pRefpointU, *pRefpointV;
	Bool bRet;
	int newpred_resync;
	// ~NEWPRED
	
	if (!short_video_header) { 
		uiNumberOfGobs = 1;
		iMBXstart=0; 
		iMBXstop= m_iNumMBX;
		iMBYstart=0;
		iMBYstop= m_iNumMBY;
	}
	else { // short_header
		uiNumberOfGobs = uiNumGobsInVop;
		iMBXstart=0; 
		iMBXstop= 0;
		iMBYstart=0;
		iMBYstop= 0;
	}
	
	uiGobNumber=0;
	while (uiGobNumber < uiNumberOfGobs) { 
		if (short_video_header) {
			uiGobHeaderEmpty=1;
			if (uiGobNumber != 0) {
				if (checkGOBMarker()) {
					uiGobHeaderEmpty=0;
					m_pbitstrmIn -> flush();
					/* UInt uiGobResyncMarker= */m_pbitstrmIn -> getBits (17);
					uiGobNumber=m_pbitstrmIn -> getBits(5);
					
					/* UInt uiGobFrameId = */m_pbitstrmIn -> getBits(2);
					
					/* UInt uiVopQuant= */m_pbitstrmIn -> getBits(5); 
					
					bFirstGobRow=TRUE; 
					uiGobNumber++; 
					
					
				} else {
					uiGobNumber++;
					
				}
				
			} else {
				uiGobNumber++;
			} 
			
			iMBXstart=0; 
			iMBXstop= m_ivolWidth/16;
			iMBYstart=(uiGobNumber*(m_ivolHeight/16)/uiNumberOfGobs)-1;
			iMBYstop= iMBYstart+(m_ivolHeight/16)/uiNumberOfGobs;
		} else {
			uiGobNumber++; 
		}
		
		// NEWPRED
		if (m_volmd.bNewpredEnable) {
			g_pNewPredDec->CopyReftoBuf(RefbufY, RefbufU, RefbufV, m_rctRefFrameY, m_rctRefFrameUV);
			bRet = FALSE;
			
			RefpointY = (PixelC*) g_pNewPredDec->m_pchNewPredRefY +
				(EXPANDY_REF_FRAME * m_rctRefFrameY.width);
			RefpointU = (PixelC*) g_pNewPredDec->m_pchNewPredRefU +
				((EXPANDY_REF_FRAME/2)*m_rctRefFrameUV.width);
			RefpointV = (PixelC*) g_pNewPredDec->m_pchNewPredRefV +
				((EXPANDY_REF_FRAME/2)*m_rctRefFrameUV.width);
			bRet = g_pNewPredDec->CopyNPtoVM(iVideoPacketNumber, RefpointY, RefpointU, RefpointV);
			
			pRefpointY = (PixelC*) m_pvopcRefQ0->pixelsY () + m_iStartInRefToCurrRctY;
			pRefpointU = (PixelC*) m_pvopcRefQ0->pixelsU () + m_iStartInRefToCurrRctUV;
			pRefpointV = (PixelC*) m_pvopcRefQ0->pixelsV () + m_iStartInRefToCurrRctUV;
			g_pNewPredDec->ChangeRefOfSlice((const PixelC* )pRefpointY,	RefbufY,
				(const PixelC* )pRefpointU,	RefbufU,
				(const PixelC* )pRefpointV,	RefbufV,
				0, 0,	m_rctRefFrameY, m_rctRefFrameUV);
			repeatPadYOrA ((PixelC*) m_pvopcRefQ0->pixelsY () + m_iOffsetForPadY, m_pvopcRefQ0);
			repeatPadUV (m_pvopcRefQ0);
			
			for (iMBY = 0; iMBY < m_iNumMBY; iMBY++) {
				for (iMBX = 0; iMBX < m_iNumMBX; iMBX++)	{
					// RRV modification
					(pmbmd + iMBX + m_iNumMBX*iMBY) -> m_iNPSegmentNumber = g_pNewPredDec->GetSliceNum((iMBX *m_iRRVScale),(iMBY *m_iRRVScale));
					//					(pmbmd + iMBX + m_iNumMBX*iMBY) -> m_iNPSegmentNumber = g_pNewPredDec->GetSliceNum(iMBX,iMBY);
					// ~RRV
				}
			}
		}
		// ~NEWPRED
		
		for (iMBY = iMBYstart; iMBY < iMBYstop; iMBY++, y += (MB_SIZE *m_iRRVScale)) {
			PixelC* ppxlcCurrQMBY = ppxlcCurrQY;
			PixelC* ppxlcCurrQMBU = ppxlcCurrQU;
			PixelC* ppxlcCurrQMBV = ppxlcCurrQV;
			//	  In a given Sprite update piece, Check whether current macroblock is not a hole and should be coded ?
			PixelC* ppxlcCurrQMBBY = ppxlcCurrQBY;
			Bool  bSptMB_NOT_HOLE= TRUE;
			if (m_uiSprite == 1 && m_vopmd.SpriteXmitMode != STOP) 
				bSptMB_NOT_HOLE = SptUpdateMB_NOT_HOLE(0, iMBY, pmbmd); 
			
			//	Added for error resilience mode By Toshiba
			skipAnyStuffing();
			if	(checkResyncMarker()) {
				decodeVideoPacketHeader(iCurrentQP);
				iVideoPacketNumber++;
				bRestartDelayedQP = TRUE;
				// NEWPRED
				bRet = FALSE;
				if (m_volmd.bNewpredEnable && (m_volmd.bNewpredSegmentType == 0))
				{ 	
					// RRV modification
					RefpointY = (PixelC*) g_pNewPredDec->m_pchNewPredRefY +
						(EXPANDY_REF_FRAME * m_rctRefFrameY.width) + 
						iMBY * (MB_SIZE *m_iRRVScale) * m_rctRefFrameY.width;
					RefpointU = (PixelC*) g_pNewPredDec->m_pchNewPredRefU +
						((EXPANDY_REF_FRAME/2)*m_rctRefFrameUV.width) + 
						iMBY * (BLOCK_SIZE *m_iRRVScale) * m_rctRefFrameUV.width;
					RefpointV = (PixelC*) g_pNewPredDec->m_pchNewPredRefV +
						((EXPANDY_REF_FRAME/2)*m_rctRefFrameUV.width) + 
						iMBY * (BLOCK_SIZE *m_iRRVScale) * m_rctRefFrameUV.width;
					bRet = g_pNewPredDec->CopyNPtoVM(iVideoPacketNumber, RefpointY, RefpointU, RefpointV);
					
					pRefpointY = (PixelC*) m_pvopcRefQ0->pixelsY () + m_iStartInRefToCurrRctY + iMBY * (MB_SIZE *m_iRRVScale) * m_rctRefFrameY.width;
					pRefpointU = (PixelC*) m_pvopcRefQ0->pixelsU () + m_iStartInRefToCurrRctUV + iMBY * (BLOCK_SIZE *m_iRRVScale) * m_rctRefFrameUV.width;
					pRefpointV = (PixelC*) m_pvopcRefQ0->pixelsV () + m_iStartInRefToCurrRctUV + iMBY * (BLOCK_SIZE *m_iRRVScale)* m_rctRefFrameUV.width;
					g_pNewPredDec->ChangeRefOfSlice((const PixelC* )pRefpointY, RefbufY,(const PixelC* )pRefpointU, RefbufU,
						(const PixelC* )pRefpointV, RefbufV, 0, (iMBY *m_iRRVScale),m_rctRefFrameY, m_rctRefFrameUV);
					repeatPadYOrA ((PixelC*) m_pvopcRefQ0->pixelsY () + m_iOffsetForPadY, m_pvopcRefQ0);
					repeatPadUV (m_pvopcRefQ0);
				}
				// ~NEWPRED
			}

			pmbmd->m_iVideoPacketNumber = iVideoPacketNumber;
			if (iMBY==0)	{
				bRightBndry = TRUE;
				bTopBndry = TRUE;
			}
			else	{
				bRightBndry = !((pmbmd - m_iNumMBX + 1) -> m_iVideoPacketNumber == pmbmd->m_iVideoPacketNumber);
				bTopBndry = !((pmbmd - m_iNumMBX) -> m_iVideoPacketNumber == pmbmd->m_iVideoPacketNumber);
			}
			if (bFirstGobRow) // Added by KPN
			{
				bTopBndry=TRUE;
				bRightBndry=TRUE;
			}
			
			// only RECTANGLE or non-transparent MB  will be decoded  for the update piece
			if (m_uiSprite == 0 || m_uiSprite == 2) { // GMC
				decodeMBTextureHeadOfPVOP (pmbmd, iCurrentQP, &bRestartDelayedQP);

				// GMC
				if(!pmbmd -> m_bMCSEL)
					// ~GMC
					decodeMV (pmbmd, pmv, TRUE, bRightBndry, bTopBndry, bZeroMV, 0, iMBY);
				// GMC
				else
				{
					Int iPmvx, iPmvy, iHalfx, iHalfy;
					globalmv (iPmvx, iPmvy, iHalfx, iHalfy,
						0,y,m_vopmd.mvInfoForward.uiRange,
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
				// ~GMC
			} 
			else if (m_uiSprite == 1 && m_vopmd.SpriteXmitMode != STOP) {
				if (m_volmd.fAUsage != RECTANGLE) {
					if (pmbmd -> m_rgTranspStatus [0] != ALL)
						decodeMBTextureHeadOfPVOP (pmbmd, iCurrentQP, &bRestartDelayedQP);
				}
				else
					decodeMBTextureHeadOfPVOP (pmbmd, iCurrentQP, &bRestartDelayedQP);
			} 
			
			CoordI x = 0;
			// RRV modification
			for (iMBX = iMBXstart; iMBX < iMBXstop; iMBX++, x += (MB_SIZE *m_iRRVScale)) { // [FDS]
				newpred_resync = 0;
				if (m_uiSprite == 0 || m_uiSprite == 2) { // GMC
					if (pmbmd->m_dctMd == INTRA || pmbmd->m_dctMd == INTRAQ) {
						decodeTextureIntraMB (pmbmd, iMBX, iMBY, ppxlcCurrQMBY, ppxlcCurrQMBU, ppxlcCurrQMBV);
						if ((m_vopmd.bInterlace == TRUE) && (pmbmd->m_bFieldDCT == TRUE))
							fieldDCTtoFrameC(ppxlcCurrQMBY);
					} else {
						if (!pmbmd->m_bSkip) {
							decodeTextureInterMB (pmbmd);
							if ((m_vopmd.bInterlace == TRUE) && (pmbmd->m_bFieldDCT == TRUE))
								fieldDCTtoFrameI(m_ppxliErrorMBY);
						}
					}
				}
				// only non-transparent MB and COD == 0 will be decoded
				else if (m_uiSprite == 1 && m_vopmd.SpriteXmitMode != STOP)
					// sprite update piece use P_VOP syntax with no MV
				{
					pmv -> setToZero ();  
					pmbmd->m_bhas4MVForward = FALSE ;  
					pmbmd -> m_dctMd = INTER ;			
					if ((pmbmd -> m_rgTranspStatus [0] != ALL) && (!pmbmd->m_bSkip))
						decodeTextureInterMB (pmbmd);
				}
				
				if (iMBX != m_iNumMBX - 1) {				
					CMBMode* pmbmdRight = pmbmd + 1;
					skipAnyStuffing();
					if (checkResyncMarker())	{
						decodeVideoPacketHeader(iCurrentQP);
						iVideoPacketNumber++;
						bRestartDelayedQP = TRUE;
						
						// NEWPRED
						newpred_resync = 1;
						// ~NEWPRED
					}
					pmbmdRight->m_iVideoPacketNumber = iVideoPacketNumber;
					if (iMBY == 0 || bFirstGobRow==TRUE)	{  // [FDS]
						bLeftBndry = !(pmbmd -> m_iVideoPacketNumber == pmbmdRight -> m_iVideoPacketNumber);
						bRightBndry = TRUE;
						bTopBndry = TRUE;
					}
					else	{
						bLeftBndry =  !(pmbmd -> m_iVideoPacketNumber == pmbmdRight -> m_iVideoPacketNumber);
						if ((iMBX + 1) == m_iNumMBX - 1)
							bRightBndry = TRUE;
						else
							bRightBndry = !((pmbmdRight - m_iNumMBX + 1) -> m_iVideoPacketNumber == pmbmdRight->m_iVideoPacketNumber);
						bTopBndry = !((pmbmdRight - m_iNumMBX) -> m_iVideoPacketNumber == pmbmdRight->m_iVideoPacketNumber);
					}
					if (m_uiSprite == 0 || m_uiSprite == 2) { // GMC
						decodeMBTextureHeadOfPVOP (pmbmdRight, iCurrentQP, &bRestartDelayedQP);
						// GMC
						if(!pmbmdRight -> m_bMCSEL)
							// ~GMC
							decodeMV (pmbmdRight, pmv + PVOP_MV_PER_REF_PER_MB, bLeftBndry, bRightBndry, bTopBndry, bZeroMV, iMBX+1, iMBY);
						// GMC
						else
						{
							Int iPmvx, iPmvy, iHalfx, iHalfy;
							globalmv (iPmvx, iPmvy, iHalfx, iHalfy,
								x+16,y,m_vopmd.mvInfoForward.uiRange,
								m_volmd.bQuarterSample);
							CMotionVector* pmvcurr = pmv + PVOP_MV_PER_REF_PER_MB;
							*pmvcurr = CMotionVector (iPmvx, iPmvy);
							pmvcurr -> iHalfX = iHalfx;
							pmvcurr -> iHalfY = iHalfy;
							pmvcurr -> computeTrueMV ();
							pmvcurr -> computeMV ();
							for (UInt i = 1; i < PVOP_MV_PER_REF_PER_MB; i++)
								pmvcurr[i] = *pmvcurr;
						}
						// ~GMC
					}
					// sprite update piece use P_VOP syntax with no MV
					else if (m_uiSprite == 1 && m_vopmd.SpriteXmitMode != STOP) 	
					{
						bSptMB_NOT_HOLE = SptUpdateMB_NOT_HOLE(iMBX+1, iMBY, pmbmdRight);
						ppxlcCurrQMBBY += MB_SIZE;
						if (pmbmdRight -> m_rgTranspStatus [0] != ALL)
							decodeMBTextureHeadOfPVOP (pmbmdRight, iCurrentQP, &bRestartDelayedQP);
					}
				}
				if (pmbmd->m_dctMd == INTER || pmbmd->m_dctMd == INTERQ) {
					if (m_uiSprite == 0 || m_uiSprite == 2) { // GMC
						motionCompMB (
							m_ppxlcPredMBY, m_pvopcRefQ0->pixelsY (),
							pmv, pmbmd, 
							iMBX, iMBY, 
							x, y,
							pmbmd->m_bSkip,
							FALSE,
							&m_rctRefVOPY0
							);
						if (!(pmbmd->m_bSkip && !pmbmd->m_bMCSEL)) { // GMC
							CoordI iXRefUV, iYRefUV, iXRefUV1, iYRefUV1;
							// GMC
							if (!pmbmd->m_bMCSEL)
								// ~GMC
								mvLookupUV (pmbmd, pmv, iXRefUV, iYRefUV, iXRefUV1, iYRefUV1);
							// INTERLACE
							if(pmbmd->m_bFieldMV) {
								motionCompFieldUV(m_ppxlcPredMBU, m_ppxlcPredMBV, m_pvopcRefQ0, x, y,
									iXRefUV, iYRefUV, pmbmd->m_bForwardTop,
									&m_rctRefVOPY0); // added by Y.Suzuki for the extended bounding box support
								motionCompFieldUV(m_ppxlcPredMBU + BLOCK_SIZE, m_ppxlcPredMBV + BLOCK_SIZE,
									m_pvopcRefQ0, x, y, iXRefUV1, iYRefUV1, pmbmd->m_bForwardBottom,
									&m_rctRefVOPY0); // added by Y.Suzuki for the extended bounding box support
							}
							// GMC
							else if(pmbmd->m_bMCSEL) {
								FindGlobalChromPredForGMC(x,y,m_ppxlcPredMBU,m_ppxlcPredMBV);
							}
							// ~GMC
							else 
								// ~INTERLACE
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
								copyFromRefToCurrQ (m_pvopcRefQ0, x, y, ppxlcCurrQMBY, ppxlcCurrQMBU, ppxlcCurrQMBV, NULL);
							else
								copyFromPredForYAndRefForCToCurrQ (x, y, ppxlcCurrQMBY, ppxlcCurrQMBU, ppxlcCurrQMBV, NULL);
						}
					} // end else from "if ( m_vopmd.SpriteXmitMode != STOP)" 
					// sprite update piece use P_VOP syntax with no motionCompMB
					else if (m_uiSprite == 1 && m_vopmd.SpriteXmitMode != STOP) 	
					{
						CopyCurrQToPred(ppxlcCurrQMBY, ppxlcCurrQMBU, ppxlcCurrQMBV);
						if ((!pmbmd->m_bSkip) && (pmbmd -> m_rgTranspStatus [0] != ALL)) {
							addErrorAndPredToCurrQ (ppxlcCurrQMBY, ppxlcCurrQMBU, ppxlcCurrQMBV);
						}
					}
				}
				
				if (newpred_resync) {
					bRet = FALSE;
					if (m_volmd.bNewpredEnable && (m_volmd.bNewpredSegmentType == 0)) {
						RefpointY = (PixelC*) g_pNewPredDec->m_pchNewPredRefY +
							(EXPANDY_REF_FRAME * m_rctRefFrameY.width) + 
							iMBY * (MB_SIZE *m_iRRVScale) * m_rctRefFrameY.width;
						RefpointU = (PixelC*) g_pNewPredDec->m_pchNewPredRefU +
							((EXPANDY_REF_FRAME/2)*m_rctRefFrameUV.width) + 
							iMBY * (BLOCK_SIZE *m_iRRVScale) * m_rctRefFrameUV.width;
						RefpointV = (PixelC*) g_pNewPredDec->m_pchNewPredRefV +
							((EXPANDY_REF_FRAME/2)*m_rctRefFrameUV.width) + 
							iMBY * (BLOCK_SIZE *m_iRRVScale) * m_rctRefFrameUV.width;
						bRet = g_pNewPredDec->CopyNPtoVM(iVideoPacketNumber, RefpointY, RefpointU, RefpointV);
						
						pRefpointY = (PixelC*) m_pvopcRefQ0->pixelsY () + m_iStartInRefToCurrRctY + iMBY * (MB_SIZE *m_iRRVScale) * m_rctRefFrameY.width;
						pRefpointU = (PixelC*) m_pvopcRefQ0->pixelsU () + m_iStartInRefToCurrRctUV + iMBY * (BLOCK_SIZE *m_iRRVScale) * m_rctRefFrameUV.width;
						pRefpointV = (PixelC*) m_pvopcRefQ0->pixelsV () + m_iStartInRefToCurrRctUV + iMBY * (BLOCK_SIZE *m_iRRVScale)* m_rctRefFrameUV.width;
						g_pNewPredDec->ChangeRefOfSlice((const PixelC* )pRefpointY, RefbufY,(const PixelC* ) pRefpointU, RefbufU,
							(const PixelC* )pRefpointV, RefbufV, ((iMBX+1) *m_iRRVScale), (iMBY*m_iRRVScale),m_rctRefFrameY, m_rctRefFrameUV);
						repeatPadYOrA ((PixelC*) m_pvopcRefQ0->pixelsY () + m_iOffsetForPadY, m_pvopcRefQ0);
						repeatPadUV (m_pvopcRefQ0);
					}
				}
				pmbmd++;
				pmv += PVOP_MV_PER_REF_PER_MB;
				ppxlcCurrQMBY += (MB_SIZE *m_iRRVScale);
				ppxlcCurrQMBU += (BLOCK_SIZE *m_iRRVScale);
				ppxlcCurrQMBV += (BLOCK_SIZE *m_iRRVScale);
			}
			
			MacroBlockMemory** ppmbmTemp = m_rgpmbmAbove;
			m_rgpmbmAbove = m_rgpmbmCurr;
			m_rgpmbmCurr  = ppmbmTemp;
			ppxlcCurrQY += m_iFrameWidthYxMBSize;
			ppxlcCurrQU += m_iFrameWidthUVxBlkSize;
			ppxlcCurrQV += m_iFrameWidthUVxBlkSize;
			ppxlcCurrQBY += m_iFrameWidthYxMBSize;
			bFirstGobRow=FALSE; // KPN
		}
	} // Terminate while loop
	
	skipAnyStuffing();
	
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
	
	if (m_volmd.bNewpredEnable) {
		Int i;
		Int noStore_vop_id;
		
		g_pNewPredDec->SetQBuf( m_pvopcRefQ0, m_pvopcRefQ1 );
		for (i=0; i < g_pNewPredDec->m_iNumSlice; i++ ) {
			noStore_vop_id = g_pNewPredDec->make_next_decbuf(g_pNewPredDec->m_pNewPredControl, 
				g_pNewPredDec->GetCurrentVOP_id(), i);
		}
		
		// copy previous picture to reference picture memory because of output ordering
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
}

Void CVideoObjectDecoder::decodeBVOP ()
{
	Int iMBX, iMBY;
	CoordI y = 0; 
	CMBMode* pmbmd = m_rgmbmd;
	CMBMode* pmbmdRef = m_rgmbmdRef;
	CMotionVector* pmv = m_rgmv;
	CMotionVector* pmvRef = m_rgmvRef;
	CMotionVector* pmvBackward = m_rgmvBackward;
	PixelC* ppxlcCurrQY = (PixelC*) m_pvopcCurrQ->pixelsY () + m_iStartInRefToCurrRctY;
	PixelC* ppxlcCurrQU = (PixelC*) m_pvopcCurrQ->pixelsU () + m_iStartInRefToCurrRctUV;
	PixelC* ppxlcCurrQV = (PixelC*) m_pvopcCurrQ->pixelsV () + m_iStartInRefToCurrRctUV;
	
	Int iCurrentQP  = m_vopmd.intStepB;
	//	Added for error resilient mode by Toshiba(1998-1-16:B-VOP+Error)
	Int	iVideoPacketNumber = 0;
	m_iVPMBnum = 0;
	
	if(m_bCodedFutureRef==FALSE)
	{
		pmbmdRef = NULL;
		pmvRef = NULL;
	}
	
	//	End Toshiba(1998-1-16:B-VOP+Error)
	for (iMBY = 0; iMBY < m_iNumMBY; iMBY++, y += MB_SIZE) {
		PixelC* ppxlcCurrQMBY = ppxlcCurrQY;
		PixelC* ppxlcCurrQMBU = ppxlcCurrQU;
		PixelC* ppxlcCurrQMBV = ppxlcCurrQV;
		CoordI x = 0;
		m_vctForwardMvPredBVOP[0].x  = m_vctForwardMvPredBVOP[0].y  = 0;
		m_vctBackwardMvPredBVOP[0].x = m_vctBackwardMvPredBVOP[0].y = 0;
		m_vctForwardMvPredBVOP[1].x  = m_vctForwardMvPredBVOP[1].y  = 0;
		m_vctBackwardMvPredBVOP[1].x = m_vctBackwardMvPredBVOP[1].y = 0;
		for (iMBX = 0; iMBX < m_iNumMBX; iMBX++, x += MB_SIZE) {
			if(!(m_volmd.volType == ENHN_LAYER && m_vopmd.iRefSelectCode == 0))
				pmbmd->m_bColocatedMBSkip = (pmbmdRef==NULL ? FALSE : pmbmdRef->m_bSkip);
			else
				pmbmd->m_bColocatedMBSkip = FALSE;
			// GMC
			if(!(m_volmd.volType == ENHN_LAYER && m_vopmd.iRefSelectCode == 0))
				pmbmd->m_bColocatedMBMCSEL = (pmbmdRef==NULL ? FALSE : pmbmdRef->m_bMCSEL); // GMC_V2
			else
				pmbmd->m_bColocatedMBMCSEL = FALSE;
			// ~GMC
			if ((pmbmd->m_bColocatedMBSkip  && !(pmbmd->m_bColocatedMBMCSEL)) && // GMC
				(m_volmd.volType == BASE_LAYER || (m_volmd.volType == ENHN_LAYER && m_vopmd.iRefSelectCode == 3 && m_volmd.iEnhnType == 0)))
			{
				copyFromRefToCurrQ (
					m_pvopcRefQ0,
					x, y, 
					ppxlcCurrQMBY, ppxlcCurrQMBU, ppxlcCurrQMBV, NULL
					);
				pmbmd->m_bSkip = TRUE;
				memset (pmv, 0, BVOP_MV_PER_REF_PER_MB * sizeof (CMotionVector));
				memset (pmvBackward, 0, BVOP_MV_PER_REF_PER_MB * sizeof (CMotionVector));
				pmbmd->m_mbType = FORWARD; // can be set to FORWARD mode since the result is the same
			}
			else {
				if	(checkResyncMarker())	{
					decodeVideoPacketHeader(iCurrentQP);
					iVideoPacketNumber++;
					m_vctForwardMvPredBVOP[0].x  = m_vctForwardMvPredBVOP[0].y  = 0;
					m_vctBackwardMvPredBVOP[0].x = m_vctBackwardMvPredBVOP[0].y = 0;
					m_vctForwardMvPredBVOP[1].x  = m_vctForwardMvPredBVOP[1].y  = 0;
					m_vctBackwardMvPredBVOP[1].x = m_vctBackwardMvPredBVOP[1].y = 0;
				}
				pmbmd->m_iVideoPacketNumber = iVideoPacketNumber;	//mv out of if by wchen to set even when errR is off; always used in mbdec 
				//	End Toshiba(1998-1-16:B-VOP+Error)
				decodeMBTextureHeadOfBVOP (pmbmd, iCurrentQP);
				decodeMVofBVOP (pmv, pmvBackward, pmbmd, iMBX, iMBY, pmvRef, pmbmdRef);
				if (!pmbmd->m_bSkip) {
					decodeTextureInterMB (pmbmd);
					// INTERLACE
					if (m_vopmd.bInterlace == TRUE && pmbmd->m_bFieldDCT == TRUE)
						fieldDCTtoFrameI(m_ppxliErrorMBY);
					// ~INTERLACE
					motionCompAndAddErrorMB_BVOP (
						pmv, pmvBackward,
						pmbmd, 
						iMBX, iMBY, 
						x, y,
						ppxlcCurrQMBY, ppxlcCurrQMBU, ppxlcCurrQMBV,
						&m_rctRefVOPY0,&m_rctRefVOPY1);
				}
				else {
                    if (m_vopmd.bInterlace) {                       // Need to remove this 'if' (Bob Eifrig)
						assert(pmbmd->m_mbType == DIRECT);
						pmbmd->m_vctDirectDeltaMV.x = 0; 
						pmbmd->m_vctDirectDeltaMV.y = 0;
						memset (m_ppxliErrorMBY, 0, MB_SQUARE_SIZE * sizeof(Int));
						memset (m_ppxliErrorMBU, 0, BLOCK_SQUARE_SIZE * sizeof(Int));
						memset (m_ppxliErrorMBV, 0, BLOCK_SQUARE_SIZE * sizeof(Int));
						motionCompAndAddErrorMB_BVOP (
							pmv, pmvBackward,
							pmbmd, 
							iMBX, iMBY, 
							x, y,
							ppxlcCurrQMBY, ppxlcCurrQMBU, ppxlcCurrQMBV,
							&m_rctRefVOPY0, &m_rctRefVOPY1);
                    } else {
						motionCompSkipMB_BVOP (pmbmd, pmv, pmvBackward,
							x, y, ppxlcCurrQMBY, ppxlcCurrQMBU, ppxlcCurrQMBV, &m_rctRefVOPY0, &m_rctRefVOPY1);
                    }
				}
			}
			
			if(m_bCodedFutureRef!=FALSE)
			{
				pmbmdRef++;
				pmvRef += PVOP_MV_PER_REF_PER_MB;
			}
			
			pmbmd++;
			pmv         += BVOP_MV_PER_REF_PER_MB;
			pmvBackward += BVOP_MV_PER_REF_PER_MB;
			ppxlcCurrQMBY += MB_SIZE;
			ppxlcCurrQMBU += BLOCK_SIZE;
			ppxlcCurrQMBV += BLOCK_SIZE;
		}
		ppxlcCurrQY += m_iFrameWidthYxMBSize;
		ppxlcCurrQU += m_iFrameWidthUVxBlkSize;
		ppxlcCurrQV += m_iFrameWidthUVxBlkSize;
	}
	if(m_volmd.bSpatialScalability && m_volmd.volType == ENHN_LAYER && m_volmd.fAUsage==ONE_BIT) {	// if SpatialScalability
		delete m_pvopcRefQ0->getPlane (BY_PLANE)->m_pbHorSamplingChk;
		delete m_pvopcRefQ0->getPlane (BY_PLANE)->m_pbVerSamplingChk;
	}
}

Void CVideoObjectDecoder::decodeBVOP_WithShape ()
{
	Int iMBX, iMBY;
	CoordI y = m_rctCurrVOPY.top; 
	CMBMode* pmbmd = m_rgmbmd;
    CMBMode* field_pmbmd = m_rgmbmd;
	CMotionVector* pmv = m_rgmv;
	CMotionVector* pmvBackward = m_rgmvBackward;
	CMotionVector* pmvBY = m_rgmvBY;
	if(m_volmd.volType == BASE_LAYER)  {	
		m_rgmvBaseBY = m_rgmvBY;
		m_rctBase = m_rctCurrVOPY;
	}
	Int xIndex, yIndex;
	xIndex = yIndex = 0;
	if(m_volmd.volType == ENHN_LAYER && m_volmd.bSpatialScalability) {
		xIndex = m_rctCurrVOPY.left - (m_rctBase.left*m_volmd.ihor_sampling_factor_n_shape/m_volmd.ihor_sampling_factor_m_shape);		
		yIndex = m_rctCurrVOPY.top - (m_rctBase.top*m_volmd.iver_sampling_factor_n_shape/m_volmd.iver_sampling_factor_m_shape);			
		xIndex /= MB_SIZE;
		yIndex /= MB_SIZE;
	}
	const CMotionVector* pmvRef;
	const CMBMode* pmbmdRef;
	
	PixelC* ppxlcCurrQY = (PixelC*) m_pvopcCurrQ->pixelsY () + m_iStartInRefToCurrRctY;
	PixelC* ppxlcCurrQU = (PixelC*) m_pvopcCurrQ->pixelsU () + m_iStartInRefToCurrRctUV;
	PixelC* ppxlcCurrQV = (PixelC*) m_pvopcCurrQ->pixelsV () + m_iStartInRefToCurrRctUV;
	PixelC* ppxlcCurrQBY = (PixelC*) m_pvopcCurrQ->pixelsBY () + m_iStartInRefToCurrRctY;
	PixelC** pppxlcCurrQMBA = new PixelC* [m_volmd.iAuxCompCount];
	
	// decide prediction direction for shape
    if(m_bCodedFutureRef==FALSE){
		m_vopmd.fShapeBPredDir = B_FORWARD;
	}
    else if (m_volmd.volType == ENHN_LAYER && m_volmd.iHierarchyType == 0 && m_volmd.bSpatialScalability == 1 &&
		m_volmd.iEnhnType != 0 && m_volmd.iuseRefShape == 1){
        m_vopmd.fShapeBPredDir = B_FORWARD;
    }
    else{
		if(m_tFutureRef - m_t >= m_t - m_tPastRef)
			m_vopmd.fShapeBPredDir = B_FORWARD;
		else
			m_vopmd.fShapeBPredDir = B_BACKWARD;
	}
	
	Int iCurrentQP  = m_vopmd.intStepB;		
	Int iCurrentQPA[MAX_MAC];
	for(Int iAuxComp=0; iAuxComp<m_volmd.iAuxCompCount; iAuxComp++ )
		iCurrentQPA[iAuxComp] = m_vopmd.intStepBAlpha[iAuxComp];
	Int	iVideoPacketNumber = 0;
	m_iVPMBnum = 0;
	for (iMBY = 0; iMBY < m_iNumMBY; iMBY++, y += MB_SIZE) {
		PixelC* ppxlcCurrQMBY = ppxlcCurrQY;
		PixelC* ppxlcCurrQMBU = ppxlcCurrQU;
		PixelC* ppxlcCurrQMBV = ppxlcCurrQV;
		PixelC* ppxlcCurrQMBBY = ppxlcCurrQBY;
		CoordI x = m_rctCurrVOPY.left; 
		m_vctForwardMvPredBVOP[0].x  = m_vctForwardMvPredBVOP[0].y  = 0;
		m_vctBackwardMvPredBVOP[0].x = m_vctBackwardMvPredBVOP[0].y = 0;
		m_vctForwardMvPredBVOP[1].x  = m_vctForwardMvPredBVOP[1].y  = 0;
		m_vctBackwardMvPredBVOP[1].x = m_vctBackwardMvPredBVOP[1].y = 0;
		
		for (iMBX = 0; iMBX < m_iNumMBX; iMBX++, x += MB_SIZE) {
			
			for(Int iAuxComp=0; iAuxComp<m_volmd.iAuxCompCount; iAuxComp++ ) { // MAC (SB) 1-Dec-99
				pppxlcCurrQMBA[iAuxComp] = (PixelC*) m_pvopcCurrQ->pixelsA (iAuxComp) + m_iStartInRefToCurrRctY
					+ iMBY*m_iFrameWidthYxMBSize + iMBX*MB_SIZE;
			}
			
			if	(checkResyncMarker())	{
				decodeVideoPacketHeader(iCurrentQP);
				iVideoPacketNumber++;
				m_vctForwardMvPredBVOP[0].x  = m_vctForwardMvPredBVOP[0].y  = 0;
				m_vctBackwardMvPredBVOP[0].x = m_vctBackwardMvPredBVOP[0].y = 0;
				m_vctForwardMvPredBVOP[1].x  = m_vctForwardMvPredBVOP[1].y  = 0;
				m_vctBackwardMvPredBVOP[1].x = m_vctBackwardMvPredBVOP[1].y = 0;
			}
			pmbmd->m_iVideoPacketNumber = iVideoPacketNumber;	//mv out of if by wchen to set even when errR is off; always used in mbdec 
			pmbmd->m_bPadded=FALSE;
			findColocatedMB (iMBX, iMBY, pmbmdRef, pmvRef);
			pmbmd->m_bColocatedMBSkip = (pmbmdRef!=NULL && pmbmdRef->m_bSkip);
			pmbmd->m_bColocatedMBMCSEL = (pmbmdRef!=NULL && pmbmdRef->m_bMCSEL);
			if(m_vopmd.bShapeCodingType)
			{
				ShapeMode shpmdColocatedMB;
				if(!(m_volmd.bSpatialScalability)){	
					if(m_vopmd.fShapeBPredDir==B_FORWARD)
						shpmdColocatedMB = m_rgshpmd [MIN (MAX (0, iMBX), m_iRefShpNumMBX - 1)
						+ MIN (MAX (0, iMBY), m_iRefShpNumMBY - 1) * m_iRefShpNumMBX];
					else
						shpmdColocatedMB = m_rgmbmdRef [MIN (MAX (0, iMBX), m_iNumMBXRef - 1)
						+ MIN (MAX (0, iMBY), m_iNumMBYRef - 1) * m_iNumMBXRef].m_shpmd;
					decodeInterShape (
						m_vopmd.fShapeBPredDir==B_FORWARD ? m_pvopcRefQ0 : m_pvopcRefQ1,
						pmbmd, iMBX, iMBY, x, y,
						NULL, pmvBY, m_ppxlcCurrMBBY, ppxlcCurrQMBBY, shpmdColocatedMB);
				}
				else {
					if((m_volmd.volType == BASE_LAYER) || (!(m_volmd.iEnhnType==0 || m_volmd.iuseRefShape ==0) && !m_volmd.bShapeOnly) ){
						if(m_volmd.iEnhnType!=0 &&m_volmd.iuseRefShape ==1){
							shpmdColocatedMB = m_rgmbmdRef [ MIN (MAX (0, iMBX), m_iNumMBXRef - 1)+
								MIN (MAX (0, iMBY), m_iNumMBYRef - 1) * m_iNumMBXRef].m_shpmd;
                        }else
							if(m_vopmd.fShapeBPredDir==B_FORWARD)
#ifndef min
#define min MIN
#endif
#ifndef max
#define max MAX
#endif
								shpmdColocatedMB = m_rgshpmd [MIN (MAX (0, iMBX), m_iRefShpNumMBX - 1)
								+ min (max (0, iMBY), m_iRefShpNumMBY - 1) * m_iRefShpNumMBX];
							else
								shpmdColocatedMB = m_rgmbmdRef [min (max (0, iMBX), m_iNumMBXRef - 1)
								+ min (max (0, iMBY), m_iNumMBYRef - 1) * m_iNumMBXRef].m_shpmd;
							decodeInterShape (
								m_vopmd.fShapeBPredDir==B_FORWARD ? m_pvopcRefQ0 : m_pvopcRefQ1,
								pmbmd, iMBX, iMBY, x, y,
								NULL, pmvBY, m_ppxlcCurrMBBY, ppxlcCurrQMBBY, shpmdColocatedMB);
					}
					else if(m_volmd.volType == ENHN_LAYER) { // for spatial scalability
						Int index = min (	max (0, (iMBX+xIndex)*m_volmd.ihor_sampling_factor_m_shape/m_volmd.ihor_sampling_factor_n_shape),			
							((m_iNumMBBaseXRef-1))	) 
							+ min (	max (0, (iMBY+yIndex)*m_volmd.iver_sampling_factor_m_shape/m_volmd.iver_sampling_factor_n_shape),			
							(m_iNumMBBaseYRef-1)  ) * (m_iNumMBBaseXRef);
						shpmdColocatedMB = m_rgBaseshpmd [index];
						
						decodeSIShapeBVOP (
							m_pvopcRefQ0,	//previous VOP
							m_pvopcRefQ1,	//lower reference layer
							pmbmd, iMBX, iMBY, x, y,
							NULL, pmvBY, (m_rgmvBaseBY+index), m_ppxlcCurrMBBY, ppxlcCurrQMBBY, shpmdColocatedMB);
					}
				}
			} else {
				decodeIntraShape (pmbmd, iMBX, iMBY, m_ppxlcCurrMBBY, ppxlcCurrQMBBY);
			}
			
			if(m_volmd.bShapeOnly==FALSE)
			{
				downSampleBY (m_ppxlcCurrMBBY, m_ppxlcCurrMBBUV, pmbmd);
				if ((pmbmd->m_bColocatedMBSkip && !(pmbmd->m_bColocatedMBMCSEL)) && // GMC
					(m_volmd.volType == BASE_LAYER || (m_volmd.volType == ENHN_LAYER && m_vopmd.iRefSelectCode == 3 && m_volmd.iEnhnType == 0))) { 
					// don't need to send any bit for this mode except enhn_layer
					copyFromRefToCurrQ (
						m_pvopcRefQ0,
						x, y, 
						ppxlcCurrQMBY, ppxlcCurrQMBU, ppxlcCurrQMBV,
						&m_rctRefVOPY0
						);
					if(m_volmd.fAUsage == EIGHT_BIT) {
						copyAlphaFromRefToCurrQ(m_pvopcRefQ0, x, y, pppxlcCurrQMBA, &m_rctRefVOPY0);
					}
					pmbmd->m_bSkip = TRUE;
					memset (pmv, 0, BVOP_MV_PER_REF_PER_MB * sizeof (CMotionVector));
					memset (pmvBackward, 0, BVOP_MV_PER_REF_PER_MB * sizeof (CMotionVector));
					pmbmd->m_mbType = FORWARD; // can be set to FORWARD mode since the result is the same
				}
				else {
					if (pmbmd->m_rgTranspStatus [0] != ALL) {
						if (!m_volmd.bSadctDisable)
							deriveSADCTRowLengths(m_rgiCurrMBCoeffWidth, m_ppxlcCurrMBBY, m_ppxlcCurrMBBUV, pmbmd ->m_rgTranspStatus);
						decodeMBTextureHeadOfBVOP (pmbmd, iCurrentQP);
						decodeMVofBVOP (pmv, pmvBackward, pmbmd, iMBX, iMBY, pmvRef, pmbmdRef);
						if (!pmbmd->m_bSkip) {
							if (!m_volmd.bSadctDisable)
								decodeTextureInterMB (pmbmd, m_ppxlcCurrMBBY, m_ppxlcCurrMBBUV);    
							else
								decodeTextureInterMB (pmbmd);
							if ((pmbmd->m_rgTranspStatus [0] == NONE) && 
								(m_vopmd.bInterlace == TRUE) && (pmbmd->m_bFieldDCT == TRUE))
								fieldDCTtoFrameI(m_ppxliErrorMBY);
						}
						else {
							if (m_vopmd.bInterlace) {                       
								assert(pmbmd->m_mbType == DIRECT);
								pmbmd->m_vctDirectDeltaMV.x = 0; 
								pmbmd->m_vctDirectDeltaMV.y = 0;
								memset (m_ppxliErrorMBY, 0, MB_SQUARE_SIZE * sizeof(Int));
								memset (m_ppxliErrorMBU, 0, BLOCK_SQUARE_SIZE * sizeof(Int));
								memset (m_ppxliErrorMBV, 0, BLOCK_SQUARE_SIZE * sizeof(Int));
								motionCompAndAddErrorMB_BVOP (
									pmv, pmvBackward,
									pmbmd, 
									iMBX, iMBY, 
									x, y,
									ppxlcCurrQMBY, ppxlcCurrQMBU, ppxlcCurrQMBV,
									&m_rctRefVOPY0, &m_rctRefVOPY1);
							} else 
							{
								motionCompSkipMB_BVOP (pmbmd, pmv, pmvBackward, x, y,
									ppxlcCurrQMBY, ppxlcCurrQMBU, ppxlcCurrQMBV, &m_rctRefVOPY0, &m_rctRefVOPY1);
								
							}
						}
						if(m_volmd.fAUsage == EIGHT_BIT) {
							for(Int iAuxComp=0; iAuxComp<m_volmd.iAuxCompCount; iAuxComp++ ) { // MAC (SB) 1-Dec-99
								decodeMBAlphaHeadOfBVOP(pmbmd, iCurrentQP, iCurrentQPA[iAuxComp], iAuxComp);
								if (!m_volmd.bSadctDisable)
									decodeAlphaInterMB(pmbmd, pppxlcCurrQMBA[iAuxComp], iAuxComp, m_ppxlcCurrMBBY);
								else
									decodeAlphaInterMB(pmbmd, pppxlcCurrQMBA[iAuxComp], iAuxComp);
							}
						}
						if (!pmbmd->m_bSkip) {
							motionCompAndAddErrorMB_BVOP (
								pmv, pmvBackward,
								pmbmd,
								iMBX, iMBY,
								x, y,
								ppxlcCurrQMBY, ppxlcCurrQMBU, ppxlcCurrQMBV,
								&m_rctRefVOPY0, &m_rctRefVOPY1
								);
						}          
						if (m_volmd.fAUsage == EIGHT_BIT) {
							for(Int iAuxComp=0; iAuxComp<m_volmd.iAuxCompCount; iAuxComp++ ) { // MAC (SB) 1-Dec-99
								if (pmbmd->m_pCODAlpha[iAuxComp]!=ALPHA_ALL255)
								{  
									motionCompAlphaMB_BVOP(
										pmv, pmvBackward,
										pmbmd,
										iMBX, iMBY,
										x, y,
										pppxlcCurrQMBA[iAuxComp],
										&m_rctRefVOPY0, &m_rctRefVOPY1,
										iAuxComp
										);
									
									if(pmbmd->m_pCODAlpha[iAuxComp]==ALPHA_SKIPPED)
										assignAlphaPredToCurrQ (pppxlcCurrQMBA[iAuxComp], iAuxComp);
									else
										addAlphaErrorAndPredToCurrQ (pppxlcCurrQMBA[iAuxComp], iAuxComp);
								}
							}
						}
					}
				}
				//padding of bvop is necessary for temporal scalability: bvop in base layer is used for prediction in enhancement layer
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
			}
			pmbmd++;
			pmv += BVOP_MV_PER_REF_PER_MB;
			pmvBY++;
			pmvBackward += BVOP_MV_PER_REF_PER_MB;
			ppxlcCurrQMBY += MB_SIZE;
			ppxlcCurrQMBU += BLOCK_SIZE;
			ppxlcCurrQMBV += BLOCK_SIZE;
			ppxlcCurrQMBBY += MB_SIZE;
		}
		ppxlcCurrQY += m_iFrameWidthYxMBSize;
		ppxlcCurrQU += m_iFrameWidthUVxBlkSize;
		ppxlcCurrQV += m_iFrameWidthUVxBlkSize;
		ppxlcCurrQBY += m_iFrameWidthYxMBSize;
	}
	if (m_vopmd.bInterlace && m_volmd.bShapeOnly == FALSE)
		fieldBasedMCPadding (field_pmbmd, m_pvopcCurrQ);
    if(m_volmd.bSpatialScalability && m_volmd.volType == ENHN_LAYER &&
		!(m_volmd.iEnhnType != 0 && m_volmd.iuseRefShape == 1) ){        // if SpatialScalability
        delete m_pvopcRefQ1->getPlane (BY_PLANE)->m_pbHorSamplingChk;
        delete m_pvopcRefQ1->getPlane (BY_PLANE)->m_pbVerSamplingChk;
    }
	delete [] pppxlcCurrQMBA;
}


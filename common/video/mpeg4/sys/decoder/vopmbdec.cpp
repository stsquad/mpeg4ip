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

*************************************************************************/
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

#ifdef __MFC_
#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif
#define new DEBUG_NEW				   
#endif // __MFC_

Void CVideoObjectDecoder::decodeVOP ()	
{
	if (m_volmd.fAUsage != RECTANGLE) {
		//	Added for data partitioning mode By Toshiba(1998-1-16:DP+RVLC)
		if(m_volmd.bDataPartitioning && !m_volmd.bShapeOnly){
			if (m_vopmd.vopPredType == PVOP)
				decodePVOP_WithShape_DataPartitioning ();
			else if (m_vopmd.vopPredType == IVOP)
				decodeIVOP_WithShape_DataPartitioning ();
			else
				decodeBVOP_WithShape ();
		}
		else {
		//	End Toshiba(1998-1-16:DP+RVLC)
			if (m_vopmd.vopPredType == PVOP)
			{	// Sprite update piece contains no shape information which has been included in the object piece
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
		//	Modified for data partitioning mode By Toshiba(1998-1-16:DP+RVLC)
		if(m_volmd.bDataPartitioning){
			if (m_vopmd.vopPredType == PVOP)
				decodePVOP_DataPartitioning ();
			else if (m_vopmd.vopPredType == IVOP)
				decodeIVOP_DataPartitioning ();
			else
				decodeBVOP ();
		}
		else {
			if (m_vopmd.vopPredType == PVOP)
				decodePVOP ();
			else if (m_vopmd.vopPredType == IVOP)
				decodeIVOP ();
			else
				decodeBVOP ();
		}
		//	End Toshiba(1998-1-16:DP+RVLC)
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
	PixelC* ppxlcRefA  = (PixelC*) m_pvopcRefQ1->pixelsA () + m_iStartInRefToCurrRctY;
	PixelC* ppxlcRefBUV = (PixelC*) m_pvopcRefQ1->pixelsBUV () + m_iStartInRefToCurrRctUV;

	pmbmd->m_bFieldDCT=0;	// new changes by X. Chen

	Int iCurrentQP  = m_vopmd.intStepI;	
	Int iCurrentQPA = m_vopmd.intStepIAlpha;	
	//	Added for error resilient mode by Toshiba(1997-11-14)
	Int	iVideoPacketNumber = 0;
	m_iVPMBnum = 0;
	Bool bRestartDelayedQP = TRUE;

	//	End Toshiba(1997-11-14)
	for (iMBY = 0; iMBY < m_iNumMBY; iMBY++) {
		PixelC* ppxlcRefMBY  = ppxlcRefY;
		PixelC* ppxlcRefMBU  = ppxlcRefU;
		PixelC* ppxlcRefMBV  = ppxlcRefV;
		PixelC* ppxlcRefMBBY = ppxlcRefBY;
		PixelC* ppxlcRefMBBUV = ppxlcRefBUV;
		PixelC* ppxlcRefMBA  = ppxlcRefA;

		Bool  bSptMB_NOT_HOLE= TRUE;
// Begin: modified by Hughes	  4/9/98
//		if (m_uiSprite == 1 && m_vopmd.SpriteXmitMode != STOP) {
		if (m_uiSprite == 1 && m_sptMode != BASIC_SPRITE && m_vopmd.SpriteXmitMode != STOP) {
// end:  modified by Hughes	  4/9/98

			bSptMB_NOT_HOLE = SptPieceMB_NOT_HOLE(0, iMBY, pmbmd);		 
			RestoreMBmCurrRow (iMBY, m_rgpmbmCurr);  // restore current row pointed by *m_rgpmbmCurr
		}

		for (iMBX = 0; iMBX < m_iNumMBX; iMBX++) {
			m_bSptMB_NOT_HOLE	 = 	bSptMB_NOT_HOLE;	
			if (!m_bSptMB_NOT_HOLE ) // current Sprite macroblock is not a hole and should be decoded			
				goto END_OF_DECODING1;

			//	Added for error resilient mode by Toshiba(1997-11-14)
			if	(checkResyncMarker())	{
				decodeVideoPacketHeader(iCurrentQP);
				iVideoPacketNumber++;
				bRestartDelayedQP = TRUE;
			}
			pmbmd->m_iVideoPacketNumber = iVideoPacketNumber;
			//	End Toshiba(1997-11-14)
			decodeIntraShape (pmbmd, iMBX, iMBY, m_ppxlcCurrMBBY, ppxlcRefMBBY);
			downSampleBY (m_ppxlcCurrMBBY, m_ppxlcCurrMBBUV); // downsample original BY now for LPE padding (using original shape)
			/*BBM// Added for Boundary by Hyundai(1998-5-9)
                        if (m_vopmd.bInterlace) initMergedMode (pmbmd);
			// End of Hyundai(1998-5-9)*/
			if(m_volmd.bShapeOnly==FALSE)
			{
				pmbmd->m_bPadded=FALSE;
				if (pmbmd->m_rgTranspStatus [0] != ALL) {
					/*BBM// Added for Boundary by Hyundai(1998-5-9)
                    if (m_vopmd.bInterlace && pmbmd->m_rgTranspStatus [0] == PARTIAL)
                            isBoundaryMacroBlockMerged (pmbmd);
					// End of Hyundai(1998-5-9)*/
					decodeMBTextureHeadOfIVOP (pmbmd, iCurrentQP, bRestartDelayedQP);
					decodeTextureIntraMB (pmbmd, iMBX, iMBY, ppxlcRefMBY, ppxlcRefMBU, ppxlcRefMBV);
// INTERLACE
			//new changes
				if ((pmbmd->m_rgTranspStatus [0] == NONE) && 
					(m_vopmd.bInterlace == TRUE) && (pmbmd->m_bFieldDCT == TRUE))
					fieldDCTtoFrameC(ppxlcRefMBY);
			//end of new changes
// ~INTERLACE
					if (m_volmd.fAUsage == EIGHT_BIT) {
						decodeMBAlphaHeadOfIVOP (pmbmd, iCurrentQP, iCurrentQPA, m_vopmd.intStepI, m_vopmd.intStepIAlpha);
						decodeAlphaIntraMB (pmbmd, iMBX, iMBY, ppxlcRefMBA);
					}
					/*BBM// Added for Boundary by Hyundai(1998-5-9)
                        if (m_vopmd.bInterlace && pmbmd->m_bMerged[0])
                                mergedMacroBlockSplit (pmbmd, ppxlcRefMBY, ppxlcRefMBA);
					// End of Hyundai(1998-5-9)*/
// Begin: modified by Hughes	  4/9/98
//				  if (m_uiSprite == 0) 	{  // added for sprite by dshu: [v071]	to delay the padding until ready for warping
				  if (m_uiSprite == 0 || m_sptMode == BASIC_SPRITE) 	{  //  delay the padding until ready for sprite warping
// end:  modified by Hughes	  4/9/98
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
				  }   // added for sprite by dshu: [v071]
				}
				else {
// Begin: modified by Hughes	  4/9/98
//				  if (m_uiSprite == 0) 	  // added for sprite by dshu: [v071]	to delay the padding until ready for warping
				  if (m_uiSprite == 0 || m_sptMode == BASIC_SPRITE) //  delay the padding until ready for sprite warping
// end:  modified by Hughes	  4/9/98
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

END_OF_DECODING1:
			ppxlcRefMBA += MB_SIZE;
			ppxlcRefMBBY += MB_SIZE;
			ppxlcRefMBBUV += BLOCK_SIZE;
			pmbmd++;
			ppxlcRefMBY += MB_SIZE;
			ppxlcRefMBU += BLOCK_SIZE;
			ppxlcRefMBV += BLOCK_SIZE;

// Begin: modified by Hughes	  4/9/98
//			if (m_uiSprite == 1 && m_vopmd.SpriteXmitMode != STOP)
			if (m_uiSprite == 1 && m_sptMode != BASIC_SPRITE && m_vopmd.SpriteXmitMode != STOP)
// end:  modified by Hughes	  4/9/98
				bSptMB_NOT_HOLE = SptPieceMB_NOT_HOLE(iMBX+1, iMBY, pmbmd);	 			
			else
				bSptMB_NOT_HOLE= TRUE;
		}
		MacroBlockMemory** ppmbmTemp = m_rgpmbmAbove;
		m_rgpmbmAbove = m_rgpmbmCurr;
				
// dshu: [v071] begin of modification	  1/18/98
// Begin: modified by Hughes	  4/9/98
// 		if  (m_uiSprite == 1)
 		if  (m_uiSprite == 1 && m_sptMode != BASIC_SPRITE)
// end:  modified by Hughes	  4/9/98
			SaveMBmCurrRow (iMBY, m_rgpmbmCurr);		 //	  save current row pointed by *m_rgpmbmCurr 
// dshu: [v071] end of modification

		m_rgpmbmCurr  = ppmbmTemp;
		ppxlcRefY += m_iFrameWidthYxMBSize;
		ppxlcRefU += m_iFrameWidthUVxBlkSize;
		ppxlcRefV += m_iFrameWidthUVxBlkSize;
		ppxlcRefBY += m_iFrameWidthYxMBSize;
		ppxlcRefA += m_iFrameWidthYxMBSize;
		ppxlcRefBUV += m_iFrameWidthUVxBlkSize;
	}
	// Added for field based MC padding by Hyundai(1998-5-9)
        if (m_vopmd.bInterlace && m_volmd.bShapeOnly == FALSE)
                fieldBasedMCPadding (field_pmbmd, m_pvopcRefQ1);
        // End of Hyundai(1998-5-9)

}

Void CVideoObjectDecoder::decodeIVOP ()	
{
	//in case the IVOP is used as an ref for direct mode
	memset (m_rgmv, 0, m_iNumMB * PVOP_MV_PER_REF_PER_MB * sizeof (CMotionVector));

	//Int macrotellertje=0;  // [FDS]


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
		//uiNumMacroblocksInGob=8;
		// dx_in_gob = ;
		// dy_in_gob = ;
	}

	Bool bRestartDelayedQP = TRUE; // decodeMBTextureHeadOfIVOP sets this to false

	uiGobNumber=0;
	while (uiGobNumber < uiNumberOfGobs) { 
		if (short_video_header) {
			uiGobHeaderEmpty=1;
			if (uiGobNumber != 0) {
				if (m_pbitstrmIn->peekBits(17)==1) {
					uiGobHeaderEmpty=0;
					/* UInt uiGobResyncMarker= wmay */m_pbitstrmIn -> getBits (17);
					uiGobNumber=m_pbitstrmIn -> getBits(5);
					/* UInt uiGobFrameId = wmay */ m_pbitstrmIn -> getBits(2);
					/* UInt uiVopQuant= wmay */m_pbitstrmIn -> getBits(5); 
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
	// Begin: modified by Hughes	  4/9/98
	//		if (m_uiSprite == 1 && m_vopmd.SpriteXmitMode != STOP) {
			if (m_uiSprite == 1 && m_sptMode != BASIC_SPRITE && m_vopmd.SpriteXmitMode != STOP) {
	// end:  modified by Hughes	  4/9/98
				bSptMB_NOT_HOLE = SptPieceMB_NOT_HOLE(0, iMBY, pmbmd);		 
				RestoreMBmCurrRow (iMBY, m_rgpmbmCurr);  // restore current row pointed by *m_rgpmbmCurr
			}

			for (iMBX = iMBXstart; iMBX < iMBXstop; iMBX++) {	
            
			  m_bSptMB_NOT_HOLE	 = 	bSptMB_NOT_HOLE;	
			  if (!m_bSptMB_NOT_HOLE ) 	// current Sprite macroblock is not a hole and should be decoded			
				  goto END_OF_DECODING2;

				if	(checkResyncMarker())	{
					decodeVideoPacketHeader(iCurrentQP);
					iVideoPacketNumber++;
					bRestartDelayedQP = TRUE;
				}
				pmbmd->m_iVideoPacketNumber = iVideoPacketNumber;	//mv out of if by wchen to set even when errR is off; always used in mbdec 
				decodeMBTextureHeadOfIVOP (pmbmd, iCurrentQP, bRestartDelayedQP);
				decodeTextureIntraMB (pmbmd, iMBX, iMBY, ppxlcRefMBY, ppxlcRefMBU, ppxlcRefMBV);
	// INTERLACE
				if ((m_vopmd.bInterlace == TRUE) && (pmbmd->m_bFieldDCT == TRUE))
					fieldDCTtoFrameC(ppxlcRefMBY);
	// ~INTERLACE

	END_OF_DECODING2:
				pmbmd++;
				ppxlcRefMBY += MB_SIZE;
				ppxlcRefMBU += BLOCK_SIZE;
				ppxlcRefMBV += BLOCK_SIZE;
	// Begin: modified by Hughes	  4/9/98
	//			if (m_uiSprite == 1 && m_vopmd.SpriteXmitMode != STOP)
				if (m_uiSprite == 1 && m_sptMode != BASIC_SPRITE && m_vopmd.SpriteXmitMode != STOP)  // get the hole status for the next MB
	// end:  modified by Hughes	  4/9/98
					bSptMB_NOT_HOLE = SptPieceMB_NOT_HOLE(iMBX+1, iMBY, pmbmd);	 			
				else
					bSptMB_NOT_HOLE= TRUE;
			}
			MacroBlockMemory** ppmbmTemp = m_rgpmbmAbove;
			m_rgpmbmAbove = m_rgpmbmCurr;
			
	// dshu: [v071] begin of modification	   1/18/98
	// Begin: modified by Hughes	  4/9/98
	// 		if  (m_uiSprite == 1)
 			if  (m_uiSprite == 1 && m_sptMode != BASIC_SPRITE)
	// end:  modified by Hughes	  4/9/98
				SaveMBmCurrRow (iMBY, m_rgpmbmCurr);		 //	  save current row pointed by *m_rgpmbmCurr 
	// dshu: [v071] end of modification

			m_rgpmbmCurr  = ppmbmTemp;
			ppxlcRefY += m_iFrameWidthYxMBSize;
			ppxlcRefU += m_iFrameWidthUVxBlkSize;
			ppxlcRefV += m_iFrameWidthUVxBlkSize;
	// dshu: [v071] begin of modification	   1/18/98	move these statements up
	//		if  (m_uiSprite == 1)
	//			SaveMBmCurrRow (iMBY, m_rgpmbmCurr);		 //	  save current row pointed by *m_rgpmbmCurr 
	// dshu: [v071] end of modification
		}
	} // KPN Terminate while loop Gob layer [FDS]
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

	PixelC* ppxlcCurrQY  = (PixelC*) m_pvopcRefQ1->pixelsY () + m_iStartInRefToCurrRctY;
	PixelC* ppxlcCurrQU  = (PixelC*) m_pvopcRefQ1->pixelsU () + m_iStartInRefToCurrRctUV;
	PixelC* ppxlcCurrQV  = (PixelC*) m_pvopcRefQ1->pixelsV () + m_iStartInRefToCurrRctUV;
	PixelC* ppxlcCurrQBY = (PixelC*) m_pvopcRefQ1->pixelsBY () + m_iStartInRefToCurrRctY;
	PixelC* ppxlcCurrQA  = (PixelC*) m_pvopcRefQ1->pixelsA () + m_iStartInRefToCurrRctY;
	
	Int iCurrentQP  = m_vopmd.intStep;	
	Int iCurrentQPA = m_vopmd.intStepPAlpha;	
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
		PixelC* ppxlcCurrQMBA  = ppxlcCurrQA;
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
			shpmdColocatedMB = m_rgmbmdRef [
				min (iMBY, m_iNumMBYRef-1) * m_iNumMBXRef
			].m_shpmd;
			decodeInterShape (m_pvopcRefQ0, pmbmd, 0, iMBY,
				m_rctCurrVOPY.left, y, pmv, pmvBY,
				m_ppxlcCurrMBBY, ppxlcCurrQMBBY,
				shpmdColocatedMB);
		}
		else {
			decodeIntraShape (pmbmd, 0, iMBY, m_ppxlcCurrMBBY,
				ppxlcCurrQMBBY);
		}
		// End Toshiba(1997-11-14)
		downSampleBY (m_ppxlcCurrMBBY, m_ppxlcCurrMBBUV);
		/*BBM// Added for Boundary by Hyundai(1998-5-9)
		if (m_vopmd.bInterlace) initMergedMode (pmbmd);
		// End of Hyundai(1998-5-9)*/
		if (pmbmd->m_rgTranspStatus [0] != ALL && m_volmd.bShapeOnly==FALSE) {
			/*BBM// Added for Boundary by Hyundai(1998-5-9)
            if (m_vopmd.bInterlace && pmbmd->m_rgTranspStatus [0] == PARTIAL)
                    isBoundaryMacroBlockMerged (pmbmd);
			// End of Hyundai(1998-5-9)*/
			decodeMBTextureHeadOfPVOP (pmbmd, iCurrentQP, bRestartDelayedQP);
			/*BBM// Added for Boundary by Hyundai(1998-5-9)
                        if (m_vopmd.bInterlace && pmbmd->m_bMerged[0])
                                swapTransparentModes (pmbmd, BBS);
			// End of Hyundai(1998-5-9)*/
			decodeMVWithShape (pmbmd, 0, iMBY, pmv);
			if(pmbmd->m_bhas4MVForward)
				padMotionVectors(pmbmd,pmv);
		}
		CoordI x = m_rctCurrVOPY.left;
		for (iMBX = 0; iMBX < m_iNumMBX; iMBX++, x += MB_SIZE) {
			pmbmd->m_bPadded = FALSE;
			if (pmbmd->m_rgTranspStatus [0] != ALL && m_volmd.bShapeOnly==FALSE) {
				/*BBM// Added for Boundary by Hyundai(1998-5-9)
                if (m_vopmd.bInterlace && pmbmd->m_bMerged[0])
                    swapTransparentModes (pmbmd, BBM);
				// End of Hyundai(1998-5-9)*/
				if ((pmbmd->m_dctMd == INTRA || pmbmd->m_dctMd == INTRAQ) && !pmbmd->m_bSkip) {
					decodeTextureIntraMB (pmbmd, iMBX, iMBY, ppxlcCurrQMBY, ppxlcCurrQMBU, ppxlcCurrQMBV);
// INTERLACE
					//new changes
					if ((pmbmd->m_rgTranspStatus [0] == NONE) && 
						(m_vopmd.bInterlace == TRUE) && (pmbmd->m_bFieldDCT == TRUE))
						fieldDCTtoFrameC(ppxlcCurrQMBY);
					//end of new changes
// ~INTERLACE
					if (m_volmd.fAUsage == EIGHT_BIT) {
						decodeMBAlphaHeadOfPVOP (pmbmd, iCurrentQP, iCurrentQPA);
						decodeAlphaIntraMB (pmbmd, iMBX, iMBY, ppxlcCurrQMBA);
					}
				}
				else {
					if (!pmbmd->m_bSkip) {
						decodeTextureInterMB (pmbmd);
// INTERLACE
					//new changes
					if ((pmbmd->m_rgTranspStatus [0] == NONE) && 
						(m_vopmd.bInterlace == TRUE) && (pmbmd->m_bFieldDCT == TRUE))
						fieldDCTtoFrameI(m_ppxliErrorMBY);
			//end of new changes
// ~INTERLACE
					}
					if (m_volmd.fAUsage == EIGHT_BIT) {
						decodeMBAlphaHeadOfPVOP (pmbmd, iCurrentQP, iCurrentQPA);
						decodeAlphaInterMB (pmbmd, ppxlcCurrQMBA);
					}
				}
				/*BBM// Added for Boundary by Hyundai(1998-5-9)
                if (m_vopmd.bInterlace && pmbmd->m_bMerged[0])
                        mergedMacroBlockSplit (pmbmd, ppxlcCurrQMBY, ppxlcCurrQMBA);
				// End of Hyundai(1998-5-9)*/
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
					shpmdColocatedMB = m_rgmbmdRef [
						min (max (0, iMBX+1), m_iNumMBXRef-1) + 
 						min (max (0, iMBY), m_iNumMBYRef-1) * m_iNumMBXRef
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
					decodeIntraShape (
						pmbmdRight, iMBX + 1, iMBY,
						m_ppxlcRightMBBY,
						ppxlcCurrQMBBY + MB_SIZE
					);
				}
				// End Toshiba(1997-11-14)
				downSampleBY (m_ppxlcRightMBBY, m_ppxlcRightMBBUV);
				/*BBM// Added for Boundary by Hyundai(1998-5-9)
				if (m_vopmd.bInterlace) initMergedMode (pmbmdRight); 
				// End of Hyundai(1998-5-9)*/
				if (pmbmdRight->m_rgTranspStatus [0] != ALL && m_volmd.bShapeOnly==FALSE) {
					/*BBM// Added for Boundary by Hyundai(1998-5-9)	
                    if (m_vopmd.bInterlace && pmbmdRight->m_rgTranspStatus [0] == PARTIAL)
                            isBoundaryMacroBlockMerged (pmbmdRight, m_ppxlcRightMBBY);
					// End of Hyundai(1998-5-9)*/
					decodeMBTextureHeadOfPVOP (pmbmdRight, iCurrentQP, bRestartDelayedQP);
					/*BBM// Added for Boundary by Hyundai(1998-5-9)
                    if (m_vopmd.bInterlace && pmbmdRight->m_bMerged[0])
                            swapTransparentModes (pmbmdRight, BBS);
					// End of Hyundai(1998-5-9)*/
					decodeMVWithShape (pmbmdRight, iMBX + 1, iMBY, pmv + PVOP_MV_PER_REF_PER_MB);
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
						if (!pmbmd->m_bSkip) {
							CoordI iXRefUV, iYRefUV;
// INTERLACE //new changes
							if(pmbmd->m_bFieldMV) {
								CoordI iXRefUV1, iYRefUV1;
								mvLookupUV (pmbmd, pmv, iXRefUV, iYRefUV, iXRefUV1, iYRefUV1);
							    motionCompFieldUV(m_ppxlcPredMBU, m_ppxlcPredMBV, m_pvopcRefQ0, x, y,
								    iXRefUV, iYRefUV, pmbmd->m_bForwardTop);
							    motionCompFieldUV(m_ppxlcPredMBU + BLOCK_SIZE, m_ppxlcPredMBV + BLOCK_SIZE,
								    m_pvopcRefQ0, x, y, iXRefUV1, iYRefUV1, pmbmd->m_bForwardBottom);
						    }
							else {
// INTERALCE //end of new changes		
							mvLookupUVWithShape (pmbmd, pmv, iXRefUV, iYRefUV);
							motionCompUV (m_ppxlcPredMBU, m_ppxlcPredMBV, m_pvopcRefQ0,
								x, y, iXRefUV, iYRefUV, m_vopmd.iRoundingControl,&m_rctRefVOPY0);
							}
							addErrorAndPredToCurrQ (ppxlcCurrQMBY, ppxlcCurrQMBU, ppxlcCurrQMBV);
						}
						else {
							if (m_volmd.bAdvPredDisable)
								copyFromRefToCurrQ (m_pvopcRefQ0, x, y, ppxlcCurrQMBY, ppxlcCurrQMBU, ppxlcCurrQMBV, &m_rctRefVOPY0);
							else
								copyFromPredForYAndRefForCToCurrQ (x, y, ppxlcCurrQMBY, ppxlcCurrQMBU, ppxlcCurrQMBV, &m_rctRefVOPY0);
						}

						if (m_volmd.fAUsage == EIGHT_BIT
							&& pmbmd->m_CODAlpha!=ALPHA_ALL255)
						{
			 				motionCompMB (
								m_ppxlcPredMBA, m_pvopcRefQ0->pixelsA (),
								pmv, pmbmd, 
								iMBX, iMBY, 
								x, y,
								FALSE, TRUE,
								&m_rctRefVOPY0
							);

							if(pmbmd->m_CODAlpha==ALPHA_SKIPPED)
								assignAlphaPredToCurrQ (ppxlcCurrQMBA);
							else
								addAlphaErrorAndPredToCurrQ (ppxlcCurrQMBA);
						}

					}
					// Added for field based MC padding by Hyundai(1998-5-9)
                                        if (!m_vopmd.bInterlace) {
						if (pmbmd->m_rgTranspStatus [0] == PARTIAL)
							mcPadCurrMB (ppxlcCurrQMBY, ppxlcCurrQMBU, ppxlcCurrQMBV, ppxlcCurrQMBA);
						padNeighborTranspMBs (
							iMBX, iMBY,
							pmbmd,
							ppxlcCurrQMBY, ppxlcCurrQMBU, ppxlcCurrQMBV, ppxlcCurrQMBA
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
							ppxlcCurrQMBY, ppxlcCurrQMBU, ppxlcCurrQMBV, ppxlcCurrQMBA
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
			ppxlcCurrQMBA += MB_SIZE;
		}
		MacroBlockMemory** ppmbmTemp = m_rgpmbmAbove;
		m_rgpmbmAbove = m_rgpmbmCurr;
		m_rgpmbmCurr  = ppmbmTemp;

		ppxlcCurrQY  += m_iFrameWidthYxMBSize;
		ppxlcCurrQU  += m_iFrameWidthUVxBlkSize;
		ppxlcCurrQV  += m_iFrameWidthUVxBlkSize;
		ppxlcCurrQBY += m_iFrameWidthYxMBSize;
		ppxlcCurrQA  += m_iFrameWidthYxMBSize;
	}
        // Added for field based MC padding by Hyundai(1998-5-9)
        if (m_vopmd.bInterlace && m_volmd.bShapeOnly == FALSE)
                fieldBasedMCPadding (field_pmbmd, m_pvopcRefQ1);
        // End of Hyundai(1998-5-9)
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
		//uiNumMacroblocksInGob=8;
		// dx_in_gob = ;
		// dy_in_gob = ;
	}
	
	uiGobNumber=0;
	while (uiGobNumber < uiNumberOfGobs) { 
		if (short_video_header) {
			uiGobHeaderEmpty=1;
			if (uiGobNumber != 0) {
				if (m_pbitstrmIn->peekBits(17)==1) {
					uiGobHeaderEmpty=0;
					/* UInt uiGobResyncMarker= wmay */m_pbitstrmIn -> getBits (17);
					uiGobNumber=m_pbitstrmIn -> getBits(5);
				
					/* UInt uiGobFrameId = wmay */m_pbitstrmIn -> getBits(2);
				
					/* UInt uiVopQuant= wmay */ m_pbitstrmIn -> getBits(5); 
				
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

		for (iMBY = iMBYstart; iMBY < iMBYstop; iMBY++, y += MB_SIZE) {
			PixelC* ppxlcCurrQMBY = ppxlcCurrQY;
			PixelC* ppxlcCurrQMBU = ppxlcCurrQU;
			PixelC* ppxlcCurrQMBV = ppxlcCurrQV;
			//	  In a given Sprite update piece, Check whether current macroblock is not a hole and should be coded ?
			PixelC* ppxlcCurrQMBBY = ppxlcCurrQBY;
			Bool  bSptMB_NOT_HOLE= TRUE;
			if (m_uiSprite == 1 && m_vopmd.SpriteXmitMode != STOP) 
				bSptMB_NOT_HOLE = SptUpdateMB_NOT_HOLE(0, iMBY, pmbmd); 

			//	Added for error resilience mode By Toshiba
			if	(iMBY != 0)	{
				if	(checkResyncMarker()) {
					decodeVideoPacketHeader(iCurrentQP);
					iVideoPacketNumber++;
					bRestartDelayedQP = TRUE;
				}
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
			if (m_uiSprite == 0) {
				decodeMBTextureHeadOfPVOP (pmbmd, iCurrentQP, bRestartDelayedQP);
				decodeMV (pmbmd, pmv, TRUE, bRightBndry, bTopBndry, bZeroMV, 0, iMBY);
			} 
			else if (m_uiSprite == 1 && m_vopmd.SpriteXmitMode != STOP) {
				if (m_volmd.fAUsage != RECTANGLE) {
					if (pmbmd -> m_rgTranspStatus [0] != ALL)
						decodeMBTextureHeadOfPVOP (pmbmd, iCurrentQP, bRestartDelayedQP);
				}
				else
					decodeMBTextureHeadOfPVOP (pmbmd, iCurrentQP, bRestartDelayedQP);
			} 

			CoordI x = 0;
			for (iMBX = iMBXstart; iMBX < iMBXstop; iMBX++, x += MB_SIZE) { // [FDS]

				if (m_uiSprite == 0) {
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
					if (checkResyncMarker())	{
						decodeVideoPacketHeader(iCurrentQP);
						iVideoPacketNumber++;
						bRestartDelayedQP = TRUE;
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
					if (m_uiSprite == 0)	{
						decodeMBTextureHeadOfPVOP (pmbmdRight, iCurrentQP, bRestartDelayedQP);
						decodeMV (pmbmdRight, pmv + PVOP_MV_PER_REF_PER_MB, bLeftBndry, bRightBndry, bTopBndry, bZeroMV, iMBX+1, iMBY);
					}
					// sprite update piece use P_VOP syntax with no MV
					else if (m_uiSprite == 1 && m_vopmd.SpriteXmitMode != STOP) 	
					{
						bSptMB_NOT_HOLE = SptUpdateMB_NOT_HOLE(iMBX+1, iMBY, pmbmdRight);
						ppxlcCurrQMBBY += MB_SIZE;
						if (pmbmdRight -> m_rgTranspStatus [0] != ALL)
							decodeMBTextureHeadOfPVOP (pmbmdRight, iCurrentQP, bRestartDelayedQP);
					}
				}
				if (pmbmd->m_dctMd == INTER || pmbmd->m_dctMd == INTERQ) {
					if (m_uiSprite == 0)	{
						motionCompMB (
							m_ppxlcPredMBY, m_pvopcRefQ0->pixelsY (),
							pmv, pmbmd, 
							iMBX, iMBY, 
							x, y,
							pmbmd->m_bSkip,
							FALSE,
							&m_rctRefVOPY0
						);
						if (!pmbmd->m_bSkip) {
							CoordI iXRefUV, iYRefUV, iXRefUV1, iYRefUV1;
							mvLookupUV (pmbmd, pmv, iXRefUV, iYRefUV, iXRefUV1, iYRefUV1);
	// INTERLACE
							if(pmbmd->m_bFieldMV) {
								motionCompFieldUV(m_ppxlcPredMBU, m_ppxlcPredMBV, m_pvopcRefQ0, x, y,
									iXRefUV, iYRefUV, pmbmd->m_bForwardTop);
								motionCompFieldUV(m_ppxlcPredMBU + BLOCK_SIZE, m_ppxlcPredMBV + BLOCK_SIZE,
									m_pvopcRefQ0, x, y, iXRefUV1, iYRefUV1, pmbmd->m_bForwardBottom);
							}
							else 
	// ~INTERLACE
								motionCompUV (m_ppxlcPredMBU, m_ppxlcPredMBV, m_pvopcRefQ0,
									x, y, iXRefUV, iYRefUV, m_vopmd.iRoundingControl,&m_rctRefVOPY0);
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
				pmbmd++;
				pmv += PVOP_MV_PER_REF_PER_MB;
				ppxlcCurrQMBY += MB_SIZE;
				ppxlcCurrQMBU += BLOCK_SIZE;
				ppxlcCurrQMBV += BLOCK_SIZE;
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
			if (pmbmd->m_bColocatedMBSkip &&
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
}

Void CVideoObjectDecoder::decodeBVOP_WithShape ()
{
	Int iMBX, iMBY;
	CoordI y = m_rctCurrVOPY.top; 
	CMBMode* pmbmd = m_rgmbmd;
        // Added for field based MC padding by Hyundai(1998-5-9)
        CMBMode* field_pmbmd = m_rgmbmd;
        // End of Hyundai(1998-5-9)
	CMotionVector* pmv = m_rgmv;
	CMotionVector* pmvBackward = m_rgmvBackward;
	CMotionVector* pmvBY = m_rgmvBY;
	const CMotionVector* pmvRef;
	const CMBMode* pmbmdRef;

	PixelC* ppxlcCurrQY = (PixelC*) m_pvopcCurrQ->pixelsY () + m_iStartInRefToCurrRctY;
	PixelC* ppxlcCurrQU = (PixelC*) m_pvopcCurrQ->pixelsU () + m_iStartInRefToCurrRctUV;
	PixelC* ppxlcCurrQV = (PixelC*) m_pvopcCurrQ->pixelsV () + m_iStartInRefToCurrRctUV;
	PixelC* ppxlcCurrQBY = (PixelC*) m_pvopcCurrQ->pixelsBY () + m_iStartInRefToCurrRctY;
	PixelC* ppxlcCurrQA  = (PixelC*) m_pvopcCurrQ->pixelsA () + m_iStartInRefToCurrRctY;

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

	Int iCurrentQP  = m_vopmd.intStepB;		
	Int iCurrentQPA = m_vopmd.intStepBAlpha;
	//	Added for error resilient mode by Toshiba(1998-1-16:B-VOP+Error)
	Int	iVideoPacketNumber = 0;
	m_iVPMBnum = 0;
	//	End Toshiba(1998-1-16:B-VOP+Error)
	for (iMBY = 0; iMBY < m_iNumMBY; iMBY++, y += MB_SIZE) {
		PixelC* ppxlcCurrQMBY = ppxlcCurrQY;
		PixelC* ppxlcCurrQMBU = ppxlcCurrQU;
		PixelC* ppxlcCurrQMBV = ppxlcCurrQV;
		PixelC* ppxlcCurrQMBBY = ppxlcCurrQBY;
		PixelC* ppxlcCurrQMBA  = ppxlcCurrQA;
		CoordI x = m_rctCurrVOPY.left; 
		m_vctForwardMvPredBVOP[0].x  = m_vctForwardMvPredBVOP[0].y  = 0;
		m_vctBackwardMvPredBVOP[0].x = m_vctBackwardMvPredBVOP[0].y = 0;
		m_vctForwardMvPredBVOP[1].x  = m_vctForwardMvPredBVOP[1].y  = 0;
		m_vctBackwardMvPredBVOP[1].x = m_vctBackwardMvPredBVOP[1].y = 0;
		for (iMBX = 0; iMBX < m_iNumMBX; iMBX++, x += MB_SIZE) {
			//	Added for error resilient mode by Toshiba(1998-1-16:B-VOP+Error)
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
			pmbmd->m_bPadded=FALSE;
			findColocatedMB (iMBX, iMBY, pmbmdRef, pmvRef);
			pmbmd->m_bColocatedMBSkip = (pmbmdRef!=NULL && pmbmdRef->m_bSkip);
			if(m_vopmd.bShapeCodingType)
			{
				ShapeMode shpmdColocatedMB;
				if(m_vopmd.fShapeBPredDir==B_FORWARD)
					shpmdColocatedMB = m_rgshpmd [min (max (0, iMBX), m_iRefShpNumMBX - 1)
					+ min (max (0, iMBY), m_iRefShpNumMBY - 1) * m_iRefShpNumMBX];
				else
					shpmdColocatedMB = m_rgmbmdRef [min (max (0, iMBX), m_iNumMBXRef - 1)
						+ min (max (0, iMBY), m_iNumMBYRef - 1) * m_iNumMBXRef].m_shpmd;
					
				decodeInterShape (
					m_vopmd.fShapeBPredDir==B_FORWARD ? m_pvopcRefQ0 : m_pvopcRefQ1,
					pmbmd, iMBX, iMBY, x, y,
					NULL, pmvBY, m_ppxlcCurrMBBY, ppxlcCurrQMBBY, shpmdColocatedMB);
			} else {
				decodeIntraShape (pmbmd, iMBX, iMBY, m_ppxlcCurrMBBY, ppxlcCurrQMBBY);
			}

			if(m_volmd.bShapeOnly==FALSE)
			{
				downSampleBY (m_ppxlcCurrMBBY, m_ppxlcCurrMBBUV);
				/*BBM// Added for Boundary by Hyundai(1998-5-9)
				if (m_vopmd.bInterlace) initMergedMode (pmbmd);
				// End of Hyundai(1998-5-9)*/
				if (pmbmd->m_bColocatedMBSkip &&
					(m_volmd.volType == BASE_LAYER || (m_volmd.volType == ENHN_LAYER && m_vopmd.iRefSelectCode == 3 && m_volmd.iEnhnType == 0))) { 
					// don't need to send any bit for this mode except enhn_layer
					copyFromRefToCurrQ (
						m_pvopcRefQ0,
						x, y, 
						ppxlcCurrQMBY, ppxlcCurrQMBU, ppxlcCurrQMBV,
						&m_rctRefVOPY0
					);
					if(m_volmd.fAUsage == EIGHT_BIT)
						copyAlphaFromRefToCurrQ(m_pvopcRefQ0, x, y, ppxlcCurrQMBA, &m_rctRefVOPY0);
					pmbmd->m_bSkip = TRUE;
					memset (pmv, 0, BVOP_MV_PER_REF_PER_MB * sizeof (CMotionVector));
					memset (pmvBackward, 0, BVOP_MV_PER_REF_PER_MB * sizeof (CMotionVector));
					pmbmd->m_mbType = FORWARD; // can be set to FORWARD mode since the result is the same
				}
				else {
					if (pmbmd->m_rgTranspStatus [0] != ALL) {
						/*BBM// Added for Boundary by Hyundai(1998-5-9)
						if (m_vopmd.bInterlace && pmbmd->m_rgTranspStatus [0] == PARTIAL)
								isBoundaryMacroBlockMerged (pmbmd);
						// End of Hyundai(1998-5-9)*/
						decodeMBTextureHeadOfBVOP (pmbmd, iCurrentQP);
						decodeMVofBVOP (pmv, pmvBackward, pmbmd, iMBX, iMBY, pmvRef, pmbmdRef);
						if (!pmbmd->m_bSkip) {
							decodeTextureInterMB (pmbmd);
	// INTERLACE
				//new changes
							if ((pmbmd->m_rgTranspStatus [0] == NONE) && 
								(m_vopmd.bInterlace == TRUE) && (pmbmd->m_bFieldDCT == TRUE))
								fieldDCTtoFrameI(m_ppxliErrorMBY);
				//end of new changes
	// ~INTERLACE
							//motionCompAndAddErrorMB_BVOP ( // delete by Hyundai, ok swinder
							//	pmv, pmvBackward,
							//	pmbmd, 
							//	iMBX, iMBY, 
							//	x, y,
							//	ppxlcCurrQMBY, ppxlcCurrQMBU, ppxlcCurrQMBV,
							//	&m_rctRefVOPY0, &m_rctRefVOPY1
							//);
						}
						else {
				// new changes 10/20/98
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
				// end of new changes 10/20/98
							{
							//assert(pmbmdRef!=NULL);
							motionCompSkipMB_BVOP (pmbmd, pmv, pmvBackward, x, y,
								ppxlcCurrQMBY, ppxlcCurrQMBU, ppxlcCurrQMBV, &m_rctRefVOPY0, &m_rctRefVOPY1);

							}
						}
						if(m_volmd.fAUsage == EIGHT_BIT)
						{
							decodeMBAlphaHeadOfBVOP(pmbmd, iCurrentQP, iCurrentQPA);
							decodeAlphaInterMB(pmbmd, ppxlcCurrQMBA);

							/* delete by Hyundai, ok swinder
							if (pmbmd->m_CODAlpha!=ALPHA_ALL255)
							{
								motionCompAlphaMB_BVOP(
									pmv, pmvBackward,
									pmbmd, 
									iMBX, iMBY, 
									x, y,
									ppxlcCurrQMBA,
									&m_rctRefVOPY0, &m_rctRefVOPY1
								);

								if(pmbmd->m_CODAlpha==ALPHA_SKIPPED)
									assignAlphaPredToCurrQ (ppxlcCurrQMBA);
								else
									addAlphaErrorAndPredToCurrQ (ppxlcCurrQMBA);
							}
							*/
						}
						/*BBM// Added for Boundary by Hyundai(1998-5-9)
						if (m_vopmd.bInterlace && pmbmd->m_bMerged[0])
								mergedMacroBlockSplit (pmbmd);
						// End of Hyundai(1998-5-9)*/
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
								if (pmbmd->m_CODAlpha!=ALPHA_ALL255)
								{  
										motionCompAlphaMB_BVOP(
												pmv, pmvBackward,
												pmbmd,
												iMBX, iMBY,
												x, y,
												ppxlcCurrQMBA,
												&m_rctRefVOPY0, &m_rctRefVOPY1
										);

										if(pmbmd->m_CODAlpha==ALPHA_SKIPPED)
												assignAlphaPredToCurrQ (ppxlcCurrQMBA);
										else
												addAlphaErrorAndPredToCurrQ (ppxlcCurrQMBA);
								}
						}
					}
				}
				//padding of bvop is necessary for temporal scalability: bvop in base layer is used for prediction in enhancement layer

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
			pmbmd++;
			pmv += BVOP_MV_PER_REF_PER_MB;
			pmvBY++;
			pmvBackward += BVOP_MV_PER_REF_PER_MB;
			ppxlcCurrQMBY += MB_SIZE;
			ppxlcCurrQMBU += BLOCK_SIZE;
			ppxlcCurrQMBV += BLOCK_SIZE;
			ppxlcCurrQMBBY += MB_SIZE;
			ppxlcCurrQMBA += MB_SIZE;
		}
		ppxlcCurrQY += m_iFrameWidthYxMBSize;
		ppxlcCurrQU += m_iFrameWidthUVxBlkSize;
		ppxlcCurrQV += m_iFrameWidthUVxBlkSize;
		ppxlcCurrQBY += m_iFrameWidthYxMBSize;
		ppxlcCurrQA += m_iFrameWidthYxMBSize;
	}
        // Added for field based MC padding by Hyundai(1998-5-9)
        if (m_vopmd.bInterlace && m_volmd.bShapeOnly == FALSE)
                fieldBasedMCPadding (field_pmbmd, m_pvopcCurrQ);
        // End of Hyundai(1998-5-9)
}


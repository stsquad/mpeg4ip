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
	David B. Shu (dbshu@hrl.com), Hughes Electronics/HRL Laboratories

	Marc Mongenet (Marc.Mongenet@epfl.ch), Swiss Federal Institute of Technology, Lausanne (EPFL)

    Mathias Wien (wien@ient.rwth-aachen.de) RWTH Aachen / Robert BOSCH GmbH

and also edited by
	Yoshinori Suzuki (Hitachi, Ltd.)

and also edited by
	Hideaki Kimata (NTT)

and also edited by
    Fujitsu Laboratories Ltd. (contact: Eishi Morimatsu)

and also edited by
    Massimo Ravasi (Massimo.Ravasi@epfl.ch), Swiss Federal Institute of Technology, Lausanne (EPFL)

and also edited by 
	Takefumi Nagumo (nagumo@av.crl.sony.co.jp), Sony Corporation
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

	vopSeEnc.hpp

Abstract:

	Encoder for one VO.

Revision History:
	Sept. 30, 1997: Error resilient tools added by Toshiba
	Dec.  11, 1997:	Interlaced tools added by NextLevel Systems
					X. Chen, xchen@nlvl.com B. Eifrig, beifrig@nlvl.com
	Jun.  16, 1998: add Complexity Estimation syntax support
					Marc Mongenet (Marc.Mongenet@epfl.ch) - EPFL
	Jan.  28, 1999: Rounding control parameters added by Hitachi, Ltd.
    Feb.  16, 1999: add Quarter Sample 
                    Mathias Wien (wien@ient.rwth-aachen.de) 
	Feb.  24, 1999: GMC added by Hitachi, Ltd.
	May    9, 1999:	tm5 rate control by DemoGraFX, duhoff@mediaone.net (added by mwi)
	Aug.24, 1999:   NEWPRED added by Hideaki Kimata (NTT) 
	Sep.06	1999 : RRV added by Eishi Morimatsu (Fujitsu Laboratories Ltd.) 
	Nov.11  1999 : Fixed Complexity Estimation syntax support, version 2 (Massimo Ravasi, EPFL)
	Feb.12  2000 : Fixed OBSS Bug by Takefumi Nagumo (Sony)

*************************************************************************/

#include <stdio.h>
#include <fstream>
#include <iostream>
#include <math.h>
#include <stdlib.h>

#include "typeapi.h"
#include "codehead.h"
#include "global.hpp"
#include "bitstrm.hpp"
#include "entropy.hpp"
#include "huffman.hpp"
#include "mode.hpp"
#include "dct.hpp"
#include "tps_enhcbuf.hpp" // added by Sharp (98/2/12)
#include "vopses.hpp"
#include "vopseenc.hpp"
#include "enhcbufenc.hpp" // added by Sharp (98/2/12)
#include "cae.h" // Added for error resilient mode by Toshiba(1997-11-14)

// NEWPRED
#include "newpred.hpp"
// ~NEWPRED

// RRV
#include "rrv.hpp"
// ~RRV

#ifdef __MFC_
#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

#define new DEBUG_NEW				   
#endif // __MFC_

CVideoObjectEncoder::~CVideoObjectEncoder ()
{
	delete m_pvopcOrig;
	delete m_pvopcRefOrig1;
	delete m_pvopcRefOrig0;

	delete m_puciRefQZoom0;
	delete m_puciRefQZoom1;
	delete m_pfdct;
	delete [] m_rgdSNR;

	// bitstream stuff
	delete [] m_pchBitsBuffer; 
	delete m_pbitstrmOut;
	delete m_pentrencSet;
	delete [] m_pchShapeBitsBuffer;
	delete m_pbitstrmShape;

//	Added for data partitioning mode By Toshiba(1998-1-16:DP+RVLC)
	delete [] *m_pchShapeBitsBuffer_DP;
	delete [] *m_pbitstrmShape_DP;
	delete m_pchShapeBitsBuffer_DP;
	delete m_pbitstrmShape_DP;
//	End Toshiba(1998-1-16)

	// shape
	delete [] m_rgiSubBlkIndx16x16;
	delete [] m_rgiSSubBlkIndx16x16;
	delete [] m_rgiSubBlkIndx18x18;
	delete [] m_rgiSubBlkIndx20x20;
	delete [] m_rgiPxlIndx12x12;
	delete [] m_rgiPxlIndx8x8;

	// B-VOP MB buffer
	delete m_puciDirectPredMB;
	delete m_puciInterpPredMB;
	delete m_piiDirectErrorMB; 
	delete m_piiInterpErrorMB; 
}

CVideoObjectEncoder::CVideoObjectEncoder (
	UInt uiVOId, 
	VOLMode& volmd, 
	VOPMode& vopmd, 
	UInt nFirstFrame,
	UInt nLastFrame,
	Int iSessionWidth,
	Int iSessionHeight,
	UInt uiRateControl,
	UInt uiBudget,
	ostream* pstrmTrace, // trace outstream
	UInt uiSprite, // for sprite usage // GMC
	UInt uiWarpingAccuracy, // for sprite warping
	Int iNumOfPnts, // for sprite warping
	CSiteD** rgstDest, // for sprite warping destination
	SptMode SpriteMode,					// sprite reconstruction mode
	CRct rctFrame,						// sprite warping source
    CRct rctSpt,     	// rct sprite
	Int iMVFileUsage,
	Char* pchMVFileName
) :
	CVideoObject ()
{
	m_nFirstFrame = nFirstFrame;
	m_nLastFrame = nLastFrame;
	m_iBufferSize = uiBudget;
	m_uiRateControl = uiRateControl;
	m_pvopcOrig = NULL;
	m_rgiSubBlkIndx16x16 =NULL;
	m_rgiSSubBlkIndx16x16 = NULL;
	m_rgiSubBlkIndx18x18 = NULL;
	m_rgiSubBlkIndx20x20 = NULL;
	m_rgiPxlIndx12x12 = NULL;
	m_rgiPxlIndx8x8 = NULL;
	m_pscanSelector = 0; // HHI Schueuer
	m_statsVOL = volmd.iAuxCompCount;
	m_statsVOP= volmd.iAuxCompCount;
	m_statsMB  = volmd.iAuxCompCount;
	m_ivolWidth = iSessionWidth;
	m_ivolHeight = iSessionHeight;		
		
	// sprite stuff
	if (iNumOfPnts >= 0) {
		m_uiSprite = uiSprite; // GMC
		m_uiWarpingAccuracy = uiWarpingAccuracy;
		m_iNumOfPnts = iNumOfPnts;
		m_rgstSrcQ = new CSiteD [m_iNumOfPnts];
		m_rgstDstQ = new CSiteD [m_iNumOfPnts];
// GMC
	    if(m_uiSprite == 1)
	    {
// ~GMC
		m_rctSpt = rctSpt;
		//#ifdef __LOW_LATENCY_SPRITE_
		m_pprgstDest = rgstDest;
		m_sptMode = SpriteMode ;
		m_rctOrg = 	rctFrame;
		m_rctDisplayWindow = m_rctOrg;	//only used for sprite, will be combined with Org later
// GMC
	    }
// ~GMC
	}
	else 
		m_uiSprite = 0;

	m_pchBitsBuffer = new Char [iSessionWidth * iSessionHeight * 2];	//we think this is enof for 4:2:0; if crushes, increase the buffer
	m_pbitstrmOut = new COutBitStream ((char *)m_pchBitsBuffer, 0, 
#ifdef _WIN32
		(ostream *)
#else
		(std::ostream *)
#endif
		pstrmTrace);
	m_pentrencSet = new CEntropyEncoderSet (*m_pbitstrmOut);

	// shape cache
	m_pchShapeBitsBuffer = new Char [MB_SIZE * MB_SIZE * 2]; // same as above
	m_pbitstrmShape = new COutBitStream ((char *)m_pchShapeBitsBuffer, 0, 
#ifndef _WIN32
		(std::ostream *)
#else
		(ostream *)
#endif
		pstrmTrace);
	m_pbitstrmShapeMBOut = m_pbitstrmOut; // initially the output stream (only changes for inter)

	m_uiVOId = uiVOId;
	m_volmd = volmd;

	// NBIT: set right clip table
	setClipTab();

	Int iClockRate = m_volmd.iClockRate-1;
	assert (iClockRate < 65536);
	if(iClockRate>0)
	{
		for (m_iNumBitsTimeIncr = 1; m_iNumBitsTimeIncr < NUMBITS_TIME_RESOLUTION; m_iNumBitsTimeIncr++)	{	
			if (iClockRate == 1)			
				break;
			iClockRate = (iClockRate >> 1);
		}
	}
	else
		m_iNumBitsTimeIncr = 1;

	m_vopmd = vopmd;
//	decideMVInfo ();                // MV search radius now varys with GOP
// GMC
	if(m_uiSprite == 1){
	    m_vopmd.iRoundingControlEncSwitch = 0;
	    m_vopmd.iRoundingControl = 0;
	}else{
// ~GMC
	    m_vopmd.iRoundingControlEncSwitch = m_volmd.iInitialRoundingType;
	    if(!m_volmd.bRoundingControlDisable)
		m_vopmd.iRoundingControlEncSwitch ^= 0x00000001;
	    m_vopmd.iRoundingControl = m_vopmd.iRoundingControlEncSwitch;
// GMC
	}
// ~GMC
//OBSS_SAIT_991015
	Int iSessionWidthRound=0;
	Int iSessionHeightRound=0;
	//if(!(m_volmd.bSpatialScalability && m_volmd.iHierarchyType==0)) {
	if(!(m_volmd.bSpatialScalability && m_volmd.iHierarchyType==0 && m_volmd.fAUsage == ONE_BIT)) {    //OBSSFIX_V2-8_after
		Int iMod = iSessionWidth % MB_SIZE; 
		iSessionWidthRound = (iMod > 0) ? iSessionWidth + MB_SIZE - iMod : iSessionWidth;
		iMod = iSessionHeight % MB_SIZE;
		iSessionHeightRound = (iMod > 0) ? iSessionHeight + MB_SIZE - iMod : iSessionHeight;
	}
	else {
		iSessionWidthRound = iSessionWidth;
		iSessionHeightRound = iSessionHeight;
	}
/*
	Int iMod = iSessionWidth % MB_SIZE;
	Int iSessionWidthRound = (iMod > 0) ? iSessionWidth + MB_SIZE - iMod : iSessionWidth;
	iMod = iSessionHeight % MB_SIZE;
	Int iSessionHeightRound = (iMod > 0) ? iSessionHeight + MB_SIZE - iMod : iSessionHeight;
	*/
//~OBSS_SAIT_991015

//	Added for data partitioning mode By Toshiba(1998-1-16:DP+RVLC)
	Int iMB;
	Int iMBN = (iSessionWidthRound/MB_SIZE)*(iSessionHeightRound/MB_SIZE);
	m_pchShapeBitsBuffer_DP = new Char* [iMBN];
	m_pbitstrmShape_DP = new COutBitStream* [iMBN];
	for (iMB = 0; iMB < iMBN; iMB++) {
		m_pchShapeBitsBuffer_DP[iMB] = new Char [MB_SIZE * MB_SIZE * 2]; // same as above
		m_pbitstrmShape_DP[iMB] = new COutBitStream ((char *)m_pchShapeBitsBuffer_DP[iMB], 0, 
#ifdef _WIN32
			(ostream *)
#else
			(std::ostream *)
#endif
			pstrmTrace);
	}
// End Toshiba(1998-1-16:DP+RVLC)
	m_rctRefFrameY = CRct (
		-EXPANDY_REF_FRAME, -EXPANDY_REF_FRAME, 
		EXPANDY_REF_FRAME + iSessionWidthRound, EXPANDY_REF_FRAME + iSessionHeightRound
	);
	m_rctRefFrameUV = m_rctRefFrameY.downSampleBy2 ();
	m_pvopcOrig = new CVOPU8YUVBA( m_volmd.fAUsage, m_rctRefFrameY, m_volmd.iAuxCompCount);
  m_pvopcRefOrig0 = new CVOPU8YUVBA( m_volmd.fAUsage, m_rctRefFrameY, m_volmd.iAuxCompCount);
	m_pvopcRefOrig1 = new CVOPU8YUVBA (m_volmd.fAUsage, m_rctRefFrameY, m_volmd.iAuxCompCount);
	allocateVOLMembers (iSessionWidthRound, iSessionHeightRound);

    //#ifdef __LOW_LATENCY_SPRITE_
	// identify initial sprite piece
	if ((m_uiSprite == 1) && (m_sptMode != BASIC_SPRITE) ){
		initialSpritePiece (iSessionWidthRound, iSessionHeightRound);
	}

    CRct rctFrameZoom = m_rctRefFrameY.upSampleBy2 ();
	m_puciRefQZoom0 = new CU8Image (rctFrameZoom);

	m_puciRefQZoom1 = new CU8Image (rctFrameZoom);

	// HHI Schueuer: sadct
	if (m_volmd.fAUsage != RECTANGLE && (!m_volmd.bSadctDisable)) {
		m_pfdct = new CFwdSADCT(volmd.nBits);
		m_pscanSelector = new CScanSelectorForSADCT(m_rgiCurrMBCoeffWidth);
	}
	else {
		m_pfdct = new CFwdBlockDCT(volmd.nBits);
		m_pscanSelector = new CScanSelector;
	}
	// end HHI
	// m_pfdct = new CFwdBlockDCT(volmd.nBits);

	// B-VOP MB buffer
	m_puciDirectPredMB = new CU8Image (CRct (0, 0, MB_SIZE, MB_SIZE));
	m_puciInterpPredMB = new CU8Image (CRct (0, 0, MB_SIZE, MB_SIZE));
	m_ppxlcDirectPredMBY = (PixelC*) m_puciDirectPredMB->pixels (); 
	m_ppxlcInterpPredMBY = (PixelC*) m_puciInterpPredMB->pixels (); 

	m_piiDirectErrorMB = new CIntImage (CRct (0, 0, MB_SIZE, MB_SIZE)); 
	m_piiInterpErrorMB = new CIntImage (CRct (0, 0, MB_SIZE, MB_SIZE)); 
	m_ppxliDirectErrorMBY = (PixelI*) m_piiDirectErrorMB->pixels (); 
	m_ppxliInterpErrorMBY = (PixelI*) m_piiInterpErrorMB->pixels (); 

	// with shape
	if (m_volmd.fAUsage != RECTANGLE) {
		m_rgiSubBlkIndx16x16 = computeShapeSubBlkIndex (4, 16);
		m_rgiSSubBlkIndx16x16 = computeShapeSubBlkIndex (2, 16);
		m_rgiSubBlkIndx18x18 = computeShapeSubBlkIndex (4, 18);
		m_rgiSubBlkIndx20x20 = computeShapeSubBlkIndex (4, 20);
		m_rgiPxlIndx12x12 = new Int [8 * 8];		
		Int* piPxl = m_rgiPxlIndx12x12;
		UInt i, j;
		for (i = 2; i < 10; i++)
			for (j = 2; j < 10; j++)
				*piPxl++ = i * 12 + j;
		m_rgiPxlIndx8x8 = new Int [4 * 4];		
		piPxl = m_rgiPxlIndx8x8;
		for (i = 2; i < 6; i++)
			for (j = 2; j < 6; j++)
				*piPxl++ = i * 8 + j;
	}
	
// NEWPRED
	if(m_volmd.bNewpredEnable) {
		g_pNewPredEnc->SetObject(
					m_iNumBitsTimeIncr,
					iSessionWidth,	
					iSessionHeight,
					uiVOId,
					m_volmd.cNewpredRefName,
					m_volmd.cNewpredSlicePoint,
					m_volmd.bNewpredSegmentType,
					m_volmd.fAUsage,
					m_volmd.bShapeOnly,
					m_pvopcRefQ0,
					m_pvopcRefQ1,
					m_rctRefFrameY,	
					m_rctRefFrameUV
		);
	}
// ~NEWPRED

	m_statsVOL.reset ();
	if(m_volmd.bCodeSequenceHead)
		codeSequenceHead ();

	codeVOHead ();
	codeVOLHead (iSessionWidth, iSessionHeight);//, rctSprite);

	// Added for error resilient mode by Toshiba(1997-11-14): Moved (1998-1-16)
	g_iMaxHeading = MAXHEADING_ERR;
	g_iMaxMiddle = MAXMIDDLE_ERR;
	g_iMaxTrailing = MAXTRAILING_ERR;

	// End Toshiba(1997-11-14)

	m_statsVOL.nBitsStuffing += m_pbitstrmOut->flush ();
	m_statRC.resetSkipMode ();
	m_rgdSNR = (m_volmd.fAUsage == EIGHT_BIT) ? new Double [3+m_volmd.iAuxCompCount] : new Double [3];

	// some fixed variables	
	m_iFrameWidthZoomY = m_iFrameWidthY * 2;
	m_iFrameWidthZoomUV = m_iFrameWidthUV * 2;
	m_iFrameWidthZoomYx2Minus2MB = m_iFrameWidthY * 2 * 2 - 2 * MB_SIZE;
	m_iFrameWidthZoomYx2Minus2Blk = m_iFrameWidthY * 2 * 2 - 2 * BLOCK_SIZE;

// RRV insertion
	Int iScale	= (m_vopmd.RRVmode.iOnOff == 1) ? (2) : (1);
// ~RRV 
	if (m_volmd.fAUsage == RECTANGLE) {
		m_rctCurrVOPY = CRct (0, 0, iSessionWidthRound, iSessionHeightRound);
		m_pvopcOrig->setBoundRct (m_rctCurrVOPY);
		m_rctCurrVOPUV = m_rctCurrVOPY.downSampleBy2 ();
		
		m_rctRefVOPY0 = m_rctCurrVOPY;
// RRV modification
		m_rctRefVOPY0.expand (EXPANDY_REFVOP *iScale);
//		m_rctRefVOPY0.expand (EXPANDY_REFVOP);
// ~RRV
		m_rctRefVOPUV0 = m_rctRefVOPY0.downSampleBy2 ();
		
		m_rctRefVOPY1 = m_rctRefVOPY0;
		m_rctRefVOPUV1 = m_rctRefVOPUV0;
		
		m_rctRefVOPZoom0 = m_rctRefVOPY0.upSampleBy2 ();
		m_rctRefVOPZoom1 = m_rctRefVOPY1.upSampleBy2 ();
		m_pvopcRefOrig0->setBoundRct (m_rctRefVOPY0);
		m_pvopcRefOrig1->setBoundRct (m_rctRefVOPY1);

	if ((m_uiSprite == 0) || (m_uiSprite == 2) || ((m_uiSprite == 1) && (m_sptMode == BASIC_SPRITE)) ) // GMC

		computeVOLConstMembers (); // these VOP members are the same for all frames
	}
//OBSS_SAIT_991015
	else if(m_volmd.fAUsage == ONE_BIT && m_volmd.volType == ENHN_LAYER) {
		m_rctCurrVOPY = CRct (0, 0, iSessionWidthRound, iSessionHeightRound);
		m_pvopcOrig->setBoundRct (m_rctCurrVOPY);
		m_rctCurrVOPUV = m_rctCurrVOPY.downSampleBy2 ();
		
		m_rctRefVOPY0 = m_rctCurrVOPY;
		m_rctRefVOPY0.expand (EXPANDY_REFVOP);
		m_rctRefVOPUV0 = m_rctRefVOPY0.downSampleBy2 ();
		
		m_rctRefVOPY1 = m_rctRefVOPY0;
		m_rctRefVOPUV1 = m_rctRefVOPUV0;
		
		m_rctRefVOPZoom0 = m_rctRefVOPY0.upSampleBy2 ();
		m_rctRefVOPZoom1 = m_rctRefVOPY1.upSampleBy2 ();
		m_pvopcRefOrig0->setBoundRct (m_rctRefVOPY0);
		m_pvopcRefOrig1->setBoundRct (m_rctRefVOPY1);
	}
//~OBSS_SAIT_991015
	m_vopmd.SpriteXmitMode = STOP ;	
    // tentative solution for indicating the first Sprite VOP
    // tentativeFirstSpriteVop = 0;

	// Open motion vector file if used
	m_pchMVFileName = pchMVFileName;
	if (m_iMVFileUsage == iMVFileUsage) {
		m_fMVFile = fopen(m_pchMVFileName, (m_iMVFileUsage == 1) ? "r" : "w");
		assert(m_fMVFile != NULL);
		m_iMVLineNo = 0;
	}
}

// for back/forward shape
CVideoObjectEncoder::CVideoObjectEncoder (
	UInt uiVOId, 
	VOLMode& volmd, 
	VOPMode& vopmd,
	Int iSessionWidth,
	Int iSessionHeight
) :
  CVideoObject (),  
  m_statsVOL(volmd.iAuxCompCount),
  m_statsVOP(volmd.iAuxCompCount),
  m_statsMB (volmd.iAuxCompCount)

{
  m_pvopcOrig = NULL;
  m_rgiSubBlkIndx16x16 =NULL;
  m_rgiSSubBlkIndx16x16 = NULL;
  m_rgiSubBlkIndx18x18 = NULL;
  m_rgiSubBlkIndx20x20 = NULL;
  m_rgiPxlIndx12x12 = NULL;
  m_rgiPxlIndx8x8  = NULL;

	// sprite stuff
    	m_uiSprite = 0; // sprite off

	m_pchBitsBuffer = NULL;
	m_pbitstrmOut = NULL;
	m_pentrencSet = NULL;

        // shape cache
	m_pchShapeBitsBuffer = NULL;
	m_pbitstrmShape = NULL;
	m_pbitstrmShapeMBOut = NULL;
	
	m_uiVOId = uiVOId;
	m_volmd = volmd;

	m_vopmd = vopmd;
//	decideMVInfo ();
	m_vopmd.iRoundingControl = 0;
	m_vopmd.iRoundingControlEncSwitch = 0;
	Int iMod = iSessionWidth % MB_SIZE;
	Int iSessionWidthRound = (iMod > 0) ? iSessionWidth + MB_SIZE - iMod : iSessionWidth;
	iMod = iSessionHeight % MB_SIZE;
	Int iSessionHeightRound = (iMod > 0) ? iSessionHeight + MB_SIZE - iMod : iSessionHeight;
	m_rctRefFrameY = CRct (
		-EXPANDY_REF_FRAME, -EXPANDY_REF_FRAME, 
		EXPANDY_REF_FRAME + iSessionWidthRound, EXPANDY_REF_FRAME + iSessionHeightRound
	);
	m_rctRefFrameUV = m_rctRefFrameY.downSampleBy2 ();
	m_pvopcOrig = new CVOPU8YUVBA (m_volmd.fAUsage, m_rctRefFrameY, m_volmd.iAuxCompCount);
	m_pvopcRefOrig0 = new CVOPU8YUVBA (m_volmd.fAUsage, m_rctRefFrameY, m_volmd.iAuxCompCount);
	m_pvopcRefOrig1 = new CVOPU8YUVBA (m_volmd.fAUsage, m_rctRefFrameY, m_volmd.iAuxCompCount);
	allocateVOLMembers (iSessionWidth, iSessionHeight);

	CRct rctFrameZoom = m_rctRefFrameY.upSampleBy2 ();
	m_puciRefQZoom0 = new CU8Image (rctFrameZoom);

	m_puciRefQZoom1 = new CU8Image (rctFrameZoom);

	// m_pfdct = new CFwdBlockDCT;
	// HHI Schueuer: sadct
	if (m_volmd.fAUsage != RECTANGLE && (!m_volmd.bSadctDisable)) {
		m_pfdct = new CFwdSADCT();
		m_pscanSelector = new CScanSelectorForSADCT(m_rgiCurrMBCoeffWidth);
	}
	else {
		m_pfdct = new CFwdBlockDCT();
		m_pscanSelector = new CScanSelector;
	}
	// end HHI

	// B-VOP MB buffer
	m_puciDirectPredMB = new CU8Image (CRct (0, 0, MB_SIZE, MB_SIZE));
	m_puciInterpPredMB = new CU8Image (CRct (0, 0, MB_SIZE, MB_SIZE));
	m_ppxlcDirectPredMBY = (PixelC*) m_puciDirectPredMB->pixels (); 
	m_ppxlcInterpPredMBY = (PixelC*) m_puciInterpPredMB->pixels (); 

	m_piiDirectErrorMB = new CIntImage (CRct (0, 0, MB_SIZE, MB_SIZE)); 
	m_piiInterpErrorMB = new CIntImage (CRct (0, 0, MB_SIZE, MB_SIZE)); 
	m_ppxliDirectErrorMBY = (PixelI*) m_piiDirectErrorMB->pixels (); 
	m_ppxliInterpErrorMBY = (PixelI*) m_piiInterpErrorMB->pixels (); 

	// with shape
	if (m_volmd.fAUsage != RECTANGLE) {
		m_rgiSubBlkIndx16x16 = computeShapeSubBlkIndex (4, 16);
		m_rgiSSubBlkIndx16x16 = computeShapeSubBlkIndex (2, 16);
		m_rgiSubBlkIndx18x18 = computeShapeSubBlkIndex (4, 18);
		m_rgiSubBlkIndx20x20 = computeShapeSubBlkIndex (4, 20);
		m_rgiPxlIndx12x12 = new Int [8 * 8];		
		Int* piPxl = m_rgiPxlIndx12x12;
		UInt i, j;
		for (i = 2; i < 10; i++)
			for (j = 2; j < 10; j++)
				*piPxl++ = i * 12 + j;
		m_rgiPxlIndx8x8 = new Int [4 * 4];		
		piPxl = m_rgiPxlIndx8x8;
		for (i = 2; i < 6; i++)
			for (j = 2; j < 6; j++)
				*piPxl++ = i * 8 + j;
	}

	m_statRC.resetSkipMode ();
	m_rgdSNR = (m_volmd.fAUsage == EIGHT_BIT) ? new Double [4] : new Double [3];

	// some fixed variables	
	m_iFrameWidthZoomY = m_iFrameWidthY * 2;
	m_iFrameWidthZoomUV = m_iFrameWidthUV * 2;
	m_iFrameWidthZoomYx2Minus2MB = m_iFrameWidthY * 2 * 2 - 2 * MB_SIZE;
	m_iFrameWidthZoomYx2Minus2Blk = m_iFrameWidthY * 2 * 2 - 2 * BLOCK_SIZE;
	
	if (m_volmd.fAUsage == RECTANGLE) {
		m_rctCurrVOPY = CRct (0, 0, iSessionWidthRound, iSessionHeightRound);
		m_pvopcOrig->setBoundRct (m_rctCurrVOPY);
		m_rctCurrVOPUV = m_rctCurrVOPY.downSampleBy2 ();
		
		m_rctRefVOPY0 = m_rctCurrVOPY;
		m_rctRefVOPY0.expand (EXPANDY_REFVOP);
		m_rctRefVOPUV0 = m_rctRefVOPY0.downSampleBy2 ();
		
		m_rctRefVOPY1 = m_rctRefVOPY0;
		m_rctRefVOPUV1 = m_rctRefVOPUV0;
		
		m_rctRefVOPZoom0 = m_rctRefVOPY0.upSampleBy2 ();
		m_pvopcRefOrig0->setBoundRct (m_rctRefVOPY0);
		m_pvopcRefOrig1->setBoundRct (m_rctRefVOPY1);
		computeVOLConstMembers (); // these VOP members are the same for all frames
	}
}

Bool CVideoObjectEncoder::skipTest(Time t,VOPpredType vopPredType)
{
	if (m_uiRateControl==RC_MPEG4) {
		if(vopPredType!=IVOP)
			if (m_statRC.skipThisFrame ())
				return TRUE;
	}
	return FALSE;
}

Void CVideoObjectEncoder::encode (
	Bool bVOPVisible, 
	Time t,
	VOPpredType vopPredType,
	const CVOPU8YUVBA* pvopfRefBaseLayer)
{
	m_t = t;
	m_vopmd.vopPredType = vopPredType;
// RRV insertion
    if(m_vopmd.RRVmode.iOnOff == 1)
	{
		m_vopmd.RRVmode.iQave		= 0;
		if(m_statsVOP.nQMB != 0)
		{
			m_vopmd.RRVmode.iQave	= m_statsVOP.nQp /m_statsVOP.nQMB;
		}
		m_vopmd.RRVmode.iNumBits	= m_statsVOP.nBitsTotal;
	}
	m_iRRVScale	= 1;	// default value
// ~RRV	
    m_statsVOP.reset ();

	Bool bTemporalScalability = m_volmd.bTemporalScalability; // added by Sharp (98/2/10)
	Bool bPrevRefVopWasCoded = m_bCodedFutureRef;

//OBSS_SAIT_991015
	if (m_volmd.volType == ENHN_LAYER && m_volmd.fAUsage == ONE_BIT && (m_volmd.bSpatialScalability && m_volmd.iHierarchyType==0) && m_volmd.iuseRefShape ==0 ) { 
		if (!m_bCodedBaseRef  || !bPrevRefVopWasCoded ) 
			m_vopmd.vopPredType = PVOP;
	}
//~OBSS_SAIT_991015

	//__LOW_LATENCY_SPRITE_
	if (m_uiSprite == 1 && m_sptMode != BASIC_SPRITE && m_vopmd.SpriteXmitMode != STOP)	{
		setRefStartingPointers (); // need to compute the starting pointers of the refVOP's (corresponding to the current Rct)
		computeVOPMembers ();
		m_iNumSptMB = 0;
		encodeVOP ();
	}
	else {
	    if (m_uiSprite == 1 && m_t>=0)
		    m_vopmd.vopPredType = SPRITE;
	
	    m_pbitstrmOut->reset ();
#if 0
	    cout << "\nVOP " << m_uiVOId << "\n";
	    cout << "\tVOL " << m_volmd.volType << "\n"; // added by Sharp (98/2/10)
#endif

		if(m_vopmd.vopPredType==IVOP || m_vopmd.vopPredType==PVOP || ((m_uiSprite == 2) && (m_vopmd.vopPredType == SPRITE))) // GMC
			m_bCodedFutureRef = bVOPVisible; // flag used by bvop prediction
//OBSS_SAIT_991015
		if (m_volmd.volType == ENHN_LAYER && m_volmd.fAUsage == ONE_BIT && (m_volmd.bSpatialScalability && m_volmd.iHierarchyType==0) && m_volmd.iuseRefShape ==0 )
			m_bCodedFutureRef = bVOPVisible; // flag used by OBSS bvop prediction
//~OBSS_SAIT_991015

	    if(!bVOPVisible)
	    {
#ifdef __TRACE_AND_STATS_
		    m_pbitstrmOut -> trace (m_t, "VOP_Time");
#endif // __TRACE_AND_STATS_
		    codeNonCodedVOPHead ();
#if 0
		    cout << "\tTime..." << m_t << " (" << m_t/m_volmd.dFrameHz << " sec)" 
				<< "\n\tNot coded.\n\n";
		    cout.flush ();
#endif
		    m_statsVOP.nBitsStuffing += m_pbitstrmOut->flush ();
#ifdef __TRACE_AND_STATS_
		    m_statsVOP.dSNRY = m_statsVOP.dSNRU = m_statsVOP.dSNRV = 0;
		    m_statsVOP.print (TRUE);
		    m_statsVOL += m_statsVOP;
#endif // __TRACE_AND_STATS_
	    }

		if(bPrevRefVopWasCoded) // only swap references if previous vop was coded
		{
			if (m_volmd.volType == BASE_LAYER)
				updateAllRefVOPs ();					// update all reconstructed VOP's
			else if (!bTemporalScalability) { // modified by Sharp (98/2/10)
				if(m_volmd.volType == ENHN_LAYER && m_vopmd.vopPredType == PVOP)
					m_vopmd.iRefSelectCode = 3;
				else if(m_volmd.volType == ENHN_LAYER && m_vopmd.vopPredType == BVOP)
					m_vopmd.iRefSelectCode = 0;
				updateAllRefVOPs (pvopfRefBaseLayer);	// for spatial scalability
			}

// NEWPRED
			if(m_volmd.bNewpredEnable) {
				// set NEWPRED reference picture memory
				g_pNewPredEnc->SetQBuf( m_pvopcRefQ0, m_pvopcRefQ1 );
				if (m_volmd.fAUsage == RECTANGLE) { //rectangular shape
					for( int iSlice = 0; iSlice < g_pNewPredEnc->m_iNumSlice; iSlice++ ) {
						// make next reference picture
						g_pNewPredEnc->makeNextRef(g_pNewPredEnc->m_pNewPredControl, iSlice);
						int		iMBY = g_pNewPredEnc->NowMBA(iSlice)/((g_pNewPredEnc->getwidth())/MB_SIZE);
						PixelC* RefpointY = (PixelC*) m_pvopcRefQ0->pixelsY () + (m_iStartInRefToCurrRctY-EXPANDY_REF_FRAME) + iMBY * MB_SIZE * m_rctRefFrameY.width;
						PixelC* RefpointU = (PixelC*) m_pvopcRefQ0->pixelsU () + (m_iStartInRefToCurrRctUV-EXPANDUV_REF_FRAME) + iMBY * BLOCK_SIZE * m_rctRefFrameUV.width;
						PixelC* RefpointV = (PixelC*) m_pvopcRefQ0->pixelsV () + (m_iStartInRefToCurrRctUV-EXPANDUV_REF_FRAME) + iMBY * BLOCK_SIZE * m_rctRefFrameUV.width;
						g_pNewPredEnc->CopyNPtoVM(iSlice, RefpointY, RefpointU, RefpointV);	
					}
					repeatPadYOrA ((PixelC*) m_pvopcRefQ0->pixelsY () + m_iOffsetForPadY, m_pvopcRefQ0);
					repeatPadUV (m_pvopcRefQ0);
				}
			}
// ~NEWPRED
    		if (m_volmd.bOriginalForME) {
			if (m_vopmd.vopPredType == PVOP || ((m_uiSprite == 2) && (m_vopmd.vopPredType == SPRITE)) || // GMC
    				(m_volmd.volType == ENHN_LAYER && m_vopmd.vopPredType == BVOP && m_vopmd.iRefSelectCode == 0))
    				swapVOPU8Pointers (m_pvopcRefOrig0, m_pvopcRefOrig1);
    		}
		}
//OBSS_SAIT_991015
		else if(m_volmd.volType == ENHN_LAYER && m_volmd.fAUsage == ONE_BIT){
			if (!bTemporalScalability) { 
				if(m_volmd.volType == ENHN_LAYER && m_vopmd.vopPredType == PVOP)
					m_vopmd.iRefSelectCode = 3;
				else if(m_volmd.volType == ENHN_LAYER && m_vopmd.vopPredType == BVOP)
					m_vopmd.iRefSelectCode = 0;
				updateAllRefVOPs (pvopfRefBaseLayer);	// for spatial scalability

    			if (m_volmd.bOriginalForME) {
    				if (m_vopmd.vopPredType == PVOP || 
    					(m_volmd.volType == ENHN_LAYER && m_vopmd.vopPredType == BVOP && m_vopmd.iRefSelectCode == 0))
    					swapVOPU8Pointers (m_pvopcRefOrig0, m_pvopcRefOrig1);
    			}
			}
		}
//~OBSS_SAIT_991015

	    if (m_vopmd.vopPredType != BVOP) {
			if(bPrevRefVopWasCoded)
			{
				// swap zoomed ref images
				CU8Image *puciTmp = m_puciRefQZoom0;
				m_puciRefQZoom0 = m_puciRefQZoom1;
				m_puciRefQZoom1 = puciTmp;
				m_rctRefVOPZoom0 = m_rctRefVOPZoom1;
			}
		    m_vopmd.iSearchRangeBackward = 0;
			if(bPrevRefVopWasCoded)
				if ( !bTemporalScalability || m_volmd.volType == BASE_LAYER ) // added by Sharp (99/1/28)
					m_tPastRef = m_tFutureRef;
		    m_tDistanceBetwIPVOP = (m_t - m_tPastRef) / m_volmd.iTemporalRate;
            m_tFutureRef = m_t;
		    m_iBCount = 0;
			if(m_vopmd.vopPredType == PVOP &&
				m_volmd.volType == ENHN_LAYER && m_vopmd.iRefSelectCode ==3)
				m_tPastRef = m_t;
			if(!m_volmd.bShapeOnly){	//OBSS_SAIT_991015	//BSO of OBSS
				if (m_vopmd.vopPredType == PVOP || (m_uiSprite == 2 && m_vopmd.vopPredType == SPRITE)) { // GMC
					// swap rounding control 0 <-> 1 every coded PVOP
					if( bPrevRefVopWasCoded && !m_volmd.bRoundingControlDisable)
						m_vopmd.iRoundingControlEncSwitch ^= 0x00000001;
					m_vopmd.iRoundingControl = m_vopmd.iRoundingControlEncSwitch;

					m_rctRefVOPZoom0 = m_rctRefVOPY0.upSampleBy2 ();
					biInterpolateY (m_pvopcRefQ0, m_rctRefVOPY0, m_puciRefQZoom0,
						m_rctRefVOPZoom0, m_vopmd.iRoundingControl);

					m_vopmd.iSearchRangeForward = 
						checkrange (m_volmd.iMVRadiusPerFrameAwayFromRef * (m_t - m_tPastRef) / m_volmd.iTemporalRate, 1, 1024);
				} else
					m_vopmd.iSearchRangeForward = 0;
			}//OBSS_SAIT_991015
	    } else {
			// BVOP, set rounding control to zero
			m_vopmd.iRoundingControl = 0;
			if (m_volmd.volType==ENHN_LAYER && m_vopmd.iRefSelectCode==0){
				m_tPastRef = m_tFutureRef;
				m_tFutureRef = m_t;
			}
			m_iBCount++;
			if (m_iBCount == 1 || (m_volmd.volType == ENHN_LAYER && m_vopmd.iRefSelectCode==0))
			{
				if(!m_volmd.bShapeOnly){	//OBSS_SAIT_991015	//BSO of OBSS
					if(bPrevRefVopWasCoded)
					{
						m_rctRefVOPZoom1 = m_rctRefVOPY1.upSampleBy2 ();
						biInterpolateY (m_pvopcRefQ1,m_rctRefVOPY1,
							m_puciRefQZoom1, m_rctRefVOPZoom1, 0);
					}
					if (m_vopmd.iRoundingControlEncSwitch==1 // modified by Sharp (98/2/10) &&	// Need to re-compute because iRoundingControl was 1
						|| (m_volmd.volType == ENHN_LAYER && m_vopmd.iRefSelectCode == 0) || bTemporalScalability) 
					{
						m_rctRefVOPZoom0 = m_rctRefVOPY0.upSampleBy2 ();
						biInterpolateY (m_pvopcRefQ0, m_rctRefVOPY0, m_puciRefQZoom0, m_rctRefVOPZoom0, 0);
					}
				}//OBSS_SAIT_991015
			}
//m_vopmd.iSearchRangeForward = 15;
//m_vopmd.iSearchRangeBackward = 15;
			m_vopmd.iSearchRangeForward = 
				checkrange ((m_t - m_tPastRef) * m_volmd.iMVRadiusPerFrameAwayFromRef / m_volmd.iTemporalRate, 1, 1024);
			m_vopmd.iSearchRangeBackward = 
				checkrange ((m_tFutureRef - m_t) * m_volmd.iMVRadiusPerFrameAwayFromRef / m_volmd.iTemporalRate, 1, 1024);
	    }
	    decideMVInfo();
	    m_statsVOP.nVOPs = 1;
    	if (m_volmd.fAUsage != RECTANGLE)
    		resetBYPlane ();

		if(!bVOPVisible)
		{
			if (m_vopmd.vopPredType != BVOP
				&& m_volmd.fAUsage != RECTANGLE && bPrevRefVopWasCoded)
			{
				// give the current object a dummy size
				m_iNumMBX = m_iNumMBY = m_iNumMB = 1;
				saveShapeMode(); // save the previous reference vop shape mode
			}
//OBSS_SAIT_991015
			if (m_volmd.fAUsage != RECTANGLE ){						
				// give the current object a dummy size						
				m_iNumMBX = m_iNumMBY = m_iNumMB = 1;						
				saveBaseShapeMode(); // save the base layer shape mode		
			}														
//~OBSS_SAIT_991015
			return;   // return if not coded
        }
        
    	// prepare for the current original data
    	// extend the currVOP size to multiples of MBSize, will put zero on extended pixels
    	if (m_volmd.fAUsage != RECTANGLE) {
    		if (m_uiSprite != 1) {		// add by cgu to deal with vop-mc-left-top 12/5
    			findTightBoundingBox ();
//OBSSFIX_MODE3
				if(!(m_volmd.bSpatialScalability && m_volmd.iHierarchyType==0 && !(m_volmd.iEnhnType != 0 && m_volmd.iuseRefShape ==1)))
//				if(!(m_volmd.bSpatialScalability && m_volmd.iHierarchyType==0))//OBSS_SAIT_991015
//~OBSSFIX_MODE3
	    			findBestBoundingBox ();	

			} else if (m_sptMode == BASIC_SPRITE) {
				Int iSpriteWidth =  m_rctSpt.right - m_rctSpt.left;
				if(iSpriteWidth%16)
					iSpriteWidth += 16 - (iSpriteWidth%16);
				Int iSpriteHeight =  m_rctSpt.bottom - m_rctSpt.top;
				if(iSpriteHeight%16)
					iSpriteHeight += 16 - (iSpriteHeight%16);
				m_pvopcOrig->setBoundRct (CRct (0, 0, iSpriteWidth, iSpriteHeight));
			}
			m_rctCurrVOPY = m_pvopcOrig->whereBoundY ();
			m_rctCurrVOPUV = m_pvopcOrig->whereBoundUV ();
			setRefStartingPointers (); // need to compute the starting pointers of the refVOP's (corresponding to the current Rct)
			computeVOPMembers ();
			// Modified for error resilient mode by Toshiba(1998-1-16)
			if (m_volmd.bVPBitTh < 0 || m_volmd.bShapeOnly==TRUE || m_volmd.volType == ENHN_LAYER) { // FIX for C2-1
				m_vopmd.bShapeCodingType = (m_vopmd.vopPredType == IVOP) ? 0 : 1;
			} else {
				m_vopmd.bShapeCodingType = (m_volmd.iBinaryAlphaRR<0)?1:((m_t % m_volmd.iBinaryAlphaRR)==0) ? 0 : 1;
			}
	//		printf("vop_shape_coding_type=%d\n",m_vopmd.bShapeCodingType);
			// End Toshiba(1998-1-16)
		}

#ifdef __TRACE_AND_STATS_
	    m_pbitstrmOut -> trace (m_t, "VOP_Time");
#endif // __TRACE_AND_STATS_
// GMC
	if(m_uiSprite == 2 && vopPredType == SPRITE)
	   if(m_volmd.bOriginalForME==TRUE)
		GlobalMotionEstimation(m_rgstDstQ, m_pvopcRefOrig0, m_pvopcOrig, m_rctRefFrameY, m_rctRefVOPY0, m_rctCurrVOPY, m_iNumOfPnts);
	   else
		GlobalMotionEstimation(m_rgstDstQ, m_pvopcRefOrig1, m_pvopcOrig, m_rctRefFrameY, m_rctRefVOPY0, m_rctCurrVOPY, m_iNumOfPnts);
// ~GMC
// RRV
		if(m_vopmd.RRVmode.iOnOff == 1)
		{
			resetAndCalcRRV();
			redefineVOLMembersRRV();
		}
		else
		{
			m_vopmd.RRVmode.iRRVOnOff	= 0;
			m_iRRVScale	= 1;
		}
// ~RRV

		if (m_uiRateControl>=RC_TM5) {
			m_tm5rc.tm5rc_init_pict(m_vopmd.vopPredType,
								m_pvopcOrig->pixelsBoundY (),
								m_iFrameWidthY,
								m_iNumMBX,
								m_iNumMBY);

			Int iQP = m_tm5rc.tm5rc_start_mb();
			// set them all to avoid testing pred type
			m_vopmd.intStep = m_vopmd.intStepB = m_vopmd.intStepI = iQP;
		}

	    codeVOPHead ();
#if 0
	    cout << "\t" << "Time..." << m_t << " (" << m_t/m_volmd.dFrameHz << " sec)";

	    switch(m_vopmd.vopPredType)		//OBSS_SAIT_991015
	    {
	    case IVOP:
		    cout << "\tIVOP";
	    	break;
	    case PVOP:
	    	cout << "\tPVOP (reference: t=" << m_tPastRef <<")";
	    	break;
// GMC
	    case SPRITE:
		cout << "\tSVOP(GMC) (reference: t=" << m_tPastRef <<")";
	    	break;
// ~GMC
	    case BVOP:
	    	cout << "\tBVOP (past ref: t=" << m_tPastRef
	    		<< ", future ref: t=" << m_tFutureRef <<")";
	    	break;
	    default:
	    	break;
	    }
	    cout << "\n";
	    cout.flush ();
	#endif
	    encodeVOP ();

		if (m_uiRateControl>=RC_TM5) {
			m_tm5rc.tm5rc_update_pict(m_vopmd.vopPredType, m_statsVOP.total());
		}

	    m_statsVOP.nBitsStuffing += m_pbitstrmOut->flush ();

// NEWPRED
		if(m_volmd.bNewpredEnable) {
			g_pNewPredEnc->SetQBuf( m_pvopcRefQ0, m_pvopcRefQ1 );
			for( int iSlice = 0; iSlice < g_pNewPredEnc->m_iNumSlice; iSlice++ ) {	
				g_pNewPredEnc->makeNextBuf(
					g_pNewPredEnc->m_pNewPredControl,
					g_pNewPredEnc->GetCurrentVOP_id(),
					iSlice
				);
			}
		}
// ~NEWPRED

    }
#ifdef __TRACE_AND_STATS_
	if(!m_volmd.bShapeOnly){	//OBSS_SAIT_991015	//BSO of OBSS
		if (m_vopmd.vopPredType == BVOP)
			computeSNRs (m_pvopcCurrQ);
		else
			computeSNRs (m_pvopcRefQ1);
		m_statsVOP.dSNRY = m_rgdSNR [0];
		m_statsVOP.dSNRU = m_rgdSNR [1];
		m_statsVOP.dSNRV = m_rgdSNR [2];
    if (m_volmd.fAUsage == EIGHT_BIT) {
      for(Int iAuxComp=0;iAuxComp<m_volmd.iAuxCompCount;iAuxComp++) // MAC (SB) 26-Nov-99
        m_statsVOP.dSNRA[iAuxComp] = m_rgdSNR[3+iAuxComp];
    }
	}	//OBSS_SAIT_991015
	m_statsVOP.print (TRUE);
	m_statsVOL += m_statsVOP;
#endif // __TRACE_AND_STATS_
	
	// rate control
	if (m_uiRateControl==RC_MPEG4) {
		assert (m_volmd.volType == BASE_LAYER);	//don't no know to do RC for scalability yet
#ifndef __TRACE_AND_STATS_
		cerr << "Compile flag __TRACE_AND_STATS_ required for stats for rate control." << endl;
		exit(1);
#endif // __TRACE_AND_STATS_
		if (m_vopmd.vopPredType == IVOP) { // !!!! check whether CRct has been expanded
			Double dMad = 
				(m_pvopcOrig->getPlane (Y_PLANE)->sumAbs (m_rctCurrVOPY)) / 
				(Double) (m_rctCurrVOPY.area ());
			m_statRC.reset (
				m_nFirstFrame, 
				m_nLastFrame, 
				m_volmd.iTemporalRate, 
				m_volmd.iPbetweenI,
				&m_vopmd.intStep,  // gets saved the first frame, and restored after every later ivop
				m_iBufferSize, 
				dMad, 
				m_statsVOP.total (),
				m_volmd.dFrameHz
			);
		}
		else if (m_vopmd.vopPredType == PVOP || ((m_uiSprite == 2) && (m_vopmd.vopPredType == SPRITE))) // GMC
			m_statRC.updateRCModel (m_statsVOP.total (), m_statsVOP.head ());
	}

	// store the direct mode data (by swaping pointers)
//OBSSFIX_MODE3
	if (m_vopmd.vopPredType != BVOP ||
		(m_volmd.volType == ENHN_LAYER && m_volmd.bSpatialScalability && m_volmd.iHierarchyType == 0 && m_volmd.iEnhnType != 0 && m_volmd.iuseRefShape == 1)) {
//	if (m_vopmd.vopPredType != BVOP) {
//~OBSSFIX_MODE3
		if(m_volmd.fAUsage != RECTANGLE && bPrevRefVopWasCoded)
			saveShapeMode();
//OBSS_SAIT_991015
		if(m_volmd.fAUsage != RECTANGLE)		
			saveBaseShapeMode();				
//~OBSS_SAIT_991015
		CMBMode* pmbmdTmp = m_rgmbmd;
		m_rgmbmd = m_rgmbmdRef;
		m_rgmbmdRef = pmbmdTmp;
		CMotionVector* pmvTmp = m_rgmv;
		m_rgmv = m_rgmvRef;
		m_rgmvRef = pmvTmp;
		//wchen: should not only for pvop
		//if (m_vopmd.vopPredType == PVOP) 
		m_rgmvBackward = m_rgmv + BVOP_MV_PER_REF_PER_MB * m_iSessNumMB;
		
#if 0 // Debugging .. use the same anchor VOPs as reference implementation
		static FILE *pfRefPicInputFile = NULL;
		if (pfRefPicInputFile == NULL) {
			pfRefPicInputFile = fopen("ref.yuv", "rb");
			assert(pfRefPicInputFile != NULL);
		}
		Int iLeadingPixels = m_t * m_rctPrevNoExpandY.area ();
		fseek (pfRefPicInputFile, iLeadingPixels + iLeadingPixels / 2, SEEK_SET);	//4:2:0
		static Int iYDataHeight = m_rctPrevNoExpandY.height ();
		static Int iUVDataHeight = m_rctPrevNoExpandY.height () / 2;
		static Int iYFrmWidth = m_pvopcRefQ1->whereY ().width;
		static Int iUvFrmWidth = m_pvopcRefQ1->whereUV ().width;
		static Int nSkipYPixel = iYFrmWidth * EXPANDY_REF_FRAME + EXPANDY_REF_FRAME;
		static Int nSkipUvPixel = iUvFrmWidth * EXPANDUV_REF_FRAME + EXPANDUV_REF_FRAME;
		PixelC* ppxlcY = (PixelC*) m_pvopcRefQ1->pixelsY () + nSkipYPixel;
		PixelC* ppxlcU = (PixelC*) m_pvopcRefQ1->pixelsU () + nSkipUvPixel;
		PixelC* ppxlcV = (PixelC*) m_pvopcRefQ1->pixelsV () + nSkipUvPixel;
		for (CoordI y = 0; y < iYDataHeight; y++) {
			Int size = (Int) fread (ppxlcY, sizeof (U8), m_rctPrevNoExpandY.width, pfRefPicInputFile);
			if (size == 0)
				fprintf (stderr, "Unexpected end of file\n");
			ppxlcY += iYFrmWidth;
		}
		for (y = 0; y < iUVDataHeight; y++) {
			Int size = (Int) fread (ppxlcU, sizeof (U8), m_rctPrevNoExpandY.width / 2, pfRefPicInputFile);
			if (size == 0)
				fprintf (stderr, "Unexpected end of file\n");
			ppxlcU += iUvFrmWidth;
		}

		for (y = 0; y < iUVDataHeight; y++) {
			Int size = (Int) fread (ppxlcV, sizeof (U8), m_rctPrevNoExpandY.width / 2, pfRefPicInputFile);
			if (size == 0)
				fprintf (stderr, "Unexpected end of file\n");
			ppxlcV += iUvFrmWidth;
		}
#endif
	}
//OBSS_SAIT_991015	//for OBSS BVOP_BASE
	else {
		if(m_volmd.volType == BASE_LAYER && m_volmd.fAUsage != RECTANGLE) 
			saveBaseShapeMode();			
	}
//~OBSS_SAIT_991015
	
	if (m_volmd.fAUsage != RECTANGLE) {
		if (m_vopmd.vopPredType != BVOP) {
			m_iNumMBRef = m_iNumMB;
			m_iNumMBXRef = m_iNumMBX;
			m_iNumMBYRef = m_iNumMBY;
			m_iOffsetForPadY = m_rctRefFrameY.offset (m_rctCurrVOPY.left, m_rctCurrVOPY.top);
			m_iOffsetForPadUV = m_rctRefFrameUV.offset (m_rctCurrVOPUV.left, m_rctCurrVOPUV.top);
			m_rctPrevNoExpandY = m_rctCurrVOPY;
			m_rctPrevNoExpandUV = m_rctCurrVOPUV;
			
			m_rctRefVOPY1 = m_rctCurrVOPY;
			m_rctRefVOPY1.expand (EXPANDY_REFVOP);
			m_rctRefVOPUV1 = m_rctCurrVOPUV;
			m_rctRefVOPUV1.expand (EXPANDUV_REFVOP);
			m_pvopcRefQ1->setBoundRct (m_rctRefVOPY1);
		}
//OBSS_SAIT_991015
		else if ((m_volmd.bSpatialScalability && m_volmd.iHierarchyType==0) && m_volmd.volType == ENHN_LAYER && m_vopmd.vopPredType == BVOP) {
			m_iNumMBRef = m_iNumMB;
			m_iNumMBXRef = m_iNumMBX;
			m_iNumMBYRef = m_iNumMBY;
			m_iOffsetForPadY = m_rctRefFrameY.offset (m_rctCurrVOPY.left, m_rctCurrVOPY.top);
			m_iOffsetForPadUV = m_rctRefFrameUV.offset (m_rctCurrVOPUV.left, m_rctCurrVOPUV.top);
			m_rctPrevNoExpandY = m_rctCurrVOPY;
			m_rctPrevNoExpandUV = m_rctCurrVOPUV;
			
			m_rctRefVOPY1 = m_rctCurrVOPY;
			m_rctRefVOPY1.expand (EXPANDY_REFVOP);
			m_rctRefVOPUV1 = m_rctCurrVOPUV;
			m_rctRefVOPUV1.expand (EXPANDUV_REFVOP);
		}
		else if (m_volmd.volType == BASE_LAYER && m_vopmd.vopPredType == BVOP) {			//for Base layer BVOP padding 
			if(!m_volmd.bShapeOnly){	
				m_iOffsetForPadY = m_rctRefFrameY.offset (m_rctCurrVOPY.left, m_rctCurrVOPY.top);
				m_iOffsetForPadUV = m_rctRefFrameUV.offset (m_rctCurrVOPUV.left, m_rctCurrVOPUV.top);
				m_rctPrevNoExpandY = m_rctCurrVOPY;
				m_rctPrevNoExpandUV = m_rctCurrVOPUV;
			}
		}
//~OBSS_SAIT_991015

// begin:  added by by Sharp (98/2/10)
		if ( bTemporalScalability && m_vopmd.vopPredType == BVOP ) {
			m_iBVOPOffsetForPadY = m_rctRefFrameY.offset (m_rctCurrVOPY.left, m_rctCurrVOPY.top);
			m_iBVOPOffsetForPadUV = m_rctRefFrameUV.offset (m_rctCurrVOPUV.left, m_rctCurrVOPUV.top);
			m_rctBVOPPrevNoExpandY = m_rctCurrVOPY;
			m_rctBVOPPrevNoExpandUV = m_rctCurrVOPUV;

			m_rctBVOPRefVOPY1 = m_rctCurrVOPY;
			m_rctBVOPRefVOPY1.expand (EXPANDY_REFVOP);
			m_rctBVOPRefVOPUV1 = m_rctCurrVOPUV;
			m_rctBVOPRefVOPUV1.expand (EXPANDUV_REFVOP);
		}
// end: added by Sharp (98/2/10)

		//this is ac/dc pred stuff; clean the memory
		Int nBlk = (m_volmd.fAUsage == EIGHT_BIT) ? 6+m_volmd.iAuxCompCount*4 : 6;
		delete [] m_rgblkmCurrMB;
		for (Int iMB = 0; iMB < m_iNumMBX; iMB++)	{
			for (Int iBlk = 0; iBlk < nBlk; iBlk++)	{
				delete [] (m_rgpmbmAbove [iMB]->rgblkm) [iBlk];
				delete [] (m_rgpmbmCurr  [iMB]->rgblkm) [iBlk];
			}
			delete [] m_rgpmbmAbove [iMB]->rgblkm;
			delete m_rgpmbmAbove [iMB];
			delete [] m_rgpmbmCurr [iMB]->rgblkm;
			delete m_rgpmbmCurr [iMB];
		}
		delete [] m_rgpmbmAbove;
		delete [] m_rgpmbmCurr;
	}

// begin: added by Sharp (98/2/10)
	if ( bTemporalScalability && m_volmd.fAUsage == RECTANGLE ) {
		m_iBVOPOffsetForPadY = m_iOffsetForPadY;
		m_iBVOPOffsetForPadUV = m_iOffsetForPadUV;
		m_rctBVOPPrevNoExpandY = m_rctPrevNoExpandY;
		m_rctBVOPPrevNoExpandUV = m_rctPrevNoExpandUV;

		m_rctBVOPRefVOPY1 = m_rctRefVOPY1;
		m_rctBVOPRefVOPUV1 = m_rctRefVOPUV1;
	}
// end: added by Sharp (98/2/10)
	
	// __LOW_LATENCY_SPRITE_
	if (m_uiSprite == 1 && m_sptMode != BASIC_SPRITE && m_vopmd.SpriteXmitMode != STOP)
		return;

//OBSS_SAIT_991015
	if (m_volmd.bShapeOnly)
		return;
//~OBSS_SAIT_991015

    if (m_vopmd.vopPredType != BVOP || (m_volmd.volType == ENHN_LAYER && !((m_volmd.bSpatialScalability && m_volmd.iHierarchyType==0) && m_volmd.fAUsage == ONE_BIT)) )	{	//OBSS_SAIT_991015
		repeatPadYOrA ((PixelC*) m_pvopcRefQ1->pixelsY () + m_iOffsetForPadY, m_pvopcRefQ1);
		repeatPadUV (m_pvopcRefQ1);
		if (m_volmd.fAUsage != RECTANGLE) {
      if (m_volmd.fAUsage == EIGHT_BIT) {
			  for(Int iAuxComp=0;iAuxComp<m_volmd.iAuxCompCount;iAuxComp++) { // MAC (SB) 26-Nov-99
          repeatPadYOrA ((PixelC*) m_pvopcRefQ1->pixelsA (iAuxComp) + m_iOffsetForPadY, m_pvopcRefQ1);
        }
      }
		}
	}

	if( m_volmd.volType == ENHN_LAYER && 
		m_vopmd.vopPredType == BVOP &&
		m_vopmd.iRefSelectCode == 0){
		repeatPadYOrA ((PixelC*) m_pvopcCurrQ->pixelsY () + m_iOffsetForPadY, m_pvopcCurrQ);
		repeatPadUV (m_pvopcCurrQ);
	}

//OBSS_SAIT_991015	//Base layer BVOP padding for OBSS
    if (m_volmd.volType == BASE_LAYER && m_vopmd.vopPredType == BVOP) {
		if(!m_volmd.bShapeOnly){	
			repeatPadYOrA ((PixelC*) m_pvopcCurrQ->pixelsY () + m_iOffsetForPadY, m_pvopcCurrQ);
			repeatPadUV (m_pvopcCurrQ);
		}
		if (m_volmd.fAUsage != RECTANGLE) {
      if (m_volmd.fAUsage == EIGHT_BIT) {
  			for(Int iAuxComp=0;iAuxComp<m_volmd.iAuxCompCount;iAuxComp++) { // MAC (SB) 26-Nov-99
          repeatPadYOrA ((PixelC*) m_pvopcCurrQ->pixelsA (iAuxComp) + m_iOffsetForPadY, m_pvopcCurrQ);
        }
      }
		}
	}
//OBSS_SAIT_991015	

// GMC
	if(m_uiSprite == 2 && m_vopmd.vopPredType != BVOP)
		updateAllOrigRefVOPs (); // update all original reference (MotionEsti) VOP's
	else
// ~GMC
	if (m_volmd.bOriginalForME) {
		if (m_vopmd.vopPredType == IVOP || m_vopmd.vopPredType == PVOP || 
		    (m_volmd.volType == ENHN_LAYER && m_vopmd.vopPredType == BVOP && m_vopmd.iRefSelectCode == 0))
			updateAllOrigRefVOPs (); // update all original reference (MotionEsti) VOP's
	}
}


Void CVideoObjectEncoder::codeSequenceHead ()
{
	m_pbitstrmOut -> putBits (START_CODE_PREFIX, NUMBITS_START_CODE_PREFIX, "VSS_Start_Code");
	m_pbitstrmOut -> putBits (VSS_START_CODE, NUMBITS_START_CODE_SUFFIX, "VSS_Start_Code");
	m_pbitstrmOut -> putBits (m_volmd.uiProfileAndLevel, NUMBITS_VSS_PROFILE, "VSS_Profile_Level");

	m_pbitstrmOut -> putBits (START_CODE_PREFIX, NUMBITS_START_CODE_PREFIX, "VSO_Start_Code");
	m_pbitstrmOut -> putBits (VSO_START_CODE, NUMBITS_START_CODE_SUFFIX, "VSO_Start_Code");
	m_pbitstrmOut -> putBits (0, 1, "VSO_IsVisualObjectIdentifier");
	m_pbitstrmOut -> putBits (VSO_TYPE, NUMBITS_VSO_TYPE, "VSO_Type");
	m_pbitstrmOut -> putBits (0, 1, "VSO_VideoSignalType");

	UInt uiNumBits = NUMBITS_START_CODE_PREFIX + NUMBITS_START_CODE_SUFFIX + NUMBITS_VSS_PROFILE
		+ NUMBITS_START_CODE_PREFIX + 1 + NUMBITS_VSO_TYPE + 1;
	m_statsVOL.nBitsHead += uiNumBits;

	m_statsVOL.nBitsStuffing += m_pbitstrmOut->flush ();
}

Void CVideoObjectEncoder::codeVOHead ()
{
	m_pbitstrmOut -> putBits (START_CODE_PREFIX, NUMBITS_START_CODE_PREFIX, "VO_Start_Code");
	m_pbitstrmOut -> putBits (VO_START_CODE, NUMBITS_VO_START_CODE, "VO_Start_Code");		//plus 3 bits
	m_pbitstrmOut -> putBits (m_uiVOId, NUMBITS_VO_ID, "VO_Id");
	UInt uiNumBits = NUMBITS_START_CODE_PREFIX + NUMBITS_VO_START_CODE + NUMBITS_VO_ID;;
	m_statsVOL.nBitsHead += uiNumBits;
#if 0
	cout << "VO Overhead" << "\t\t" << uiNumBits << "\n\n";
	cout.flush ();
#endif
}

Void CVideoObjectEncoder::codeVOLHead (Int iSessionWidth, Int iSessionHeight)
{
	m_pbitstrmOut -> putBits (START_CODE_PREFIX, NUMBITS_START_CODE_PREFIX, "VOL_Start_Code");
	m_pbitstrmOut -> putBits (VOL_START_CODE, NUMBITS_VOL_START_CODE, "VOL_Start_Code");		//plus 4 bits
	m_pbitstrmOut -> putBits ((Int) (m_volmd.volType == ENHN_LAYER), NUMBITS_VOL_ID, "VOL_Id"); // by katata
	m_statsVOL.nBitsHead+=NUMBITS_START_CODE_PREFIX+NUMBITS_VOL_START_CODE+NUMBITS_VOL_ID;

// Begin: modified by Hughes	  4/9/98	  per clause 2.1.7. in N2171 document
	m_pbitstrmOut -> putBits ((Int) 0, 1, "VOL_Random_Access");		//isn't this a system level flg?
	m_statsVOL.nBitsHead++;
// End: modified by Hughes	  4/9/98

	m_pbitstrmOut -> putBits ((Int) 1, 8, "VOL_Type_Indicator"); // Set to indicate SIMPLE profile.
	m_statsVOL.nBitsHead+=8;

// GMC
	if(m_volmd.uiVerID == 2){
		m_pbitstrmOut -> putBits ((Int) 1, 1, "VOL_Is_Object_Layer_Identifier"); 
	// Here, is_object_layer_identifier is used for Version1/Version2
	// identification at this moment (tentative solution).
	// vol_type_indicator is set to indicate Main profile (not version 2).
	// need discussion at Video Group about this issue.
		m_pbitstrmOut -> putBits ((Int) 2, 4, "VOL_Verid");
		m_pbitstrmOut -> putBits ((Int) 1, 3, "VOL_Priority");
		m_statsVOL.nBitsHead+=8;
	}else{
		m_pbitstrmOut -> putBits ((Int) 0, 1, "VOL_Is_Object_Layer_Identifier"); //useless flag for now
		m_statsVOL.nBitsHead++;
	}
// ~GMC
	m_pbitstrmOut -> putBits ((Int) 1, 4, "aspect_ratio_info"); // square pix
	m_statsVOL.nBitsHead+=4;
	
	m_pbitstrmOut -> putBits (m_volmd.uiVolControlParameters, 1, "VOL_Control_Parameters"); //useless flag for now
	m_statsVOL.nBitsHead++;
	
	if(m_volmd.uiVolControlParameters)
	{
		m_pbitstrmOut -> putBits (m_volmd.uiChromaFormat, 2, "Chroma_Format");
		m_pbitstrmOut -> putBits (m_volmd.uiLowDelay, 1, "Low_Delay");
		m_pbitstrmOut -> putBits (m_volmd.uiVBVParams, 1, "VBV_Params");
		m_statsVOL.nBitsHead += 2 + 1 + 1;

		if(m_volmd.uiVBVParams)
		{
			UInt uiFirstHalfBitRate = (m_volmd.uiBitRate >> 15);
			UInt uiLatterHalfBitRate = (m_volmd.uiBitRate & 0x7fff);
			m_pbitstrmOut -> putBits (uiFirstHalfBitRate, 15, "FirstHalf_BitRate");
			m_pbitstrmOut -> putBits (1, 1, "Marker");
			m_pbitstrmOut -> putBits (uiLatterHalfBitRate, 15, "LatterHalf_BitRate");
			m_pbitstrmOut -> putBits (1, 1, "Marker");

			UInt uiFirstHalfVbvBufferSize = (m_volmd.uiVbvBufferSize >> 3);
			UInt uiLatterHalfVbvBufferSize = (m_volmd.uiVbvBufferSize & 7);
			m_pbitstrmOut -> putBits (uiFirstHalfVbvBufferSize, 15, "FirstHalf_VbvBufferSize");
			m_pbitstrmOut -> putBits (1, 1, "Marker");
			m_pbitstrmOut -> putBits (uiLatterHalfVbvBufferSize, 3, "LatterHalf_VbvBufferSize");
			
			UInt uiFirstHalfVbvBufferOccupany = (m_volmd.uiVbvBufferOccupany >> 15);
			UInt uiLatterHalfVbvBufferOccupany = (m_volmd.uiVbvBufferOccupany & 0x7fff);
			m_pbitstrmOut -> putBits (uiFirstHalfVbvBufferOccupany, 11, "FirstHalf_VbvBufferOccupany");
			m_pbitstrmOut -> putBits (1, 1, "Marker");
			m_pbitstrmOut -> putBits (uiLatterHalfVbvBufferOccupany, 15, "LatterHalf_VbvBufferOccupany");
			m_pbitstrmOut -> putBits (1, 1, "Marker");

			m_statsVOL.nBitsHead += 15 + 15 + 15 + 3 + 11 + 15;
			m_statsVOL.nBitsStuffing += 5;
		}

	}

	if(m_volmd.bShapeOnly==TRUE)
	{
		m_pbitstrmOut -> putBits ((Int) 2, NUMBITS_VOL_SHAPE, "VOL_Shape_Type");
		m_pbitstrmOut -> putBits (1, 1, "Marker");
		m_pbitstrmOut -> putBits (m_volmd.iClockRate, NUMBITS_TIME_RESOLUTION, "VOL_Time_Increment_Resolution"); 
		m_pbitstrmOut -> putBits (1, 1, "Marker");
		m_pbitstrmOut -> putBits (0, 1, "VOL_Fixed_Vop_Rate");
//OBSS_SAIT_991015
		if(m_volmd.uiVerID != 1){
			m_pbitstrmOut -> putBits (m_volmd.volType == ENHN_LAYER, 1, "VOL_Scalability");
			m_statsVOL.nBitsHead++;
			if (m_volmd.volType == ENHN_LAYER)	{
				m_pbitstrmOut -> putBits (m_volmd.ihor_sampling_factor_n_shape, 5, "VOL_Horizontal_Sampling_Factor_SHAPE");		
				m_pbitstrmOut -> putBits (m_volmd.ihor_sampling_factor_m_shape, 5, "VOL_Horizontal_Sampling_Factor_Ref_SHAPE");	
				m_pbitstrmOut -> putBits (m_volmd.iver_sampling_factor_n_shape, 5, "VOL_Vertical_Sampling_Factor_SHAPE");	
				m_pbitstrmOut -> putBits (m_volmd.iver_sampling_factor_m_shape, 5, "VOL_Vertical_Sampling_Factor_Ref_SHAPE");
				m_statsVOL.nBitsHead += 20;
//RESYNC_MARKER_FIX
				m_pbitstrmOut -> putBits (m_volmd.bResyncMarkerDisable, 1, "VOL_resync_marker_disable");
//				m_pbitstrmOut -> putBits (0, 1, "VOL_resync_marker_disable");
//~RESYNC_MARKER_FIX
			}
			else
				m_pbitstrmOut -> putBits (0, 1, "VOL_resync_marker_disable");
		}
		else
//RESYNC_MARKER_FIX
			m_pbitstrmOut -> putBits (m_volmd.bResyncMarkerDisable, 1, "VOL_resync_marker_disable");
//			m_pbitstrmOut -> putBits (0, 1, "VOL_resync_marker_disable");
//~RESYNC_MARKER_FIX

//~OBSS_SAIT_991015
		m_statsVOL.nBitsHead += NUMBITS_VOL_SHAPE + NUMBITS_TIME_RESOLUTION + 2;
		m_statsVOL.nBitsStuffing +=2;
#if 0
		cout << "VOL Overhead" << "\t\t" << m_statsVOL.nBitsHead << "\n\n";
		cout.flush ();
#endif
		return;
	}
	else
	{
		Int iAUsage = (Int)m_volmd.fAUsage;
		if(iAUsage==2) // CD: 0 = rect, 1 = binary, 2 = shape only, 3 = grey alpha
			iAUsage = 3;
		m_pbitstrmOut -> putBits (iAUsage, NUMBITS_VOL_SHAPE, "VOL_Shape_Type");
		if (iAUsage==3 && m_volmd.uiVerID!=1) // MAC (SB) 2-Dec-99
		{
			if(m_volmd.iAlphaShapeExtension>=0)
				m_pbitstrmOut -> putBits ( m_volmd.iAlphaShapeExtension, 4, "VOL_Shape_Extension");
			else
				m_pbitstrmOut -> putBits ( 0, 4, "VOL_Shape_Extension");
		}
		m_pbitstrmOut -> putBits (1, 1, "Marker");
// GMC
		m_statsVOL.nBitsStuffing ++;
// ~GMC
		m_pbitstrmOut -> putBits (m_volmd.iClockRate, NUMBITS_TIME_RESOLUTION, "VOL_Time_Increment_Resolution"); 
		m_pbitstrmOut -> putBits (1, 1, "Marker");
// GMC
		m_statsVOL.nBitsStuffing ++;
// ~GMC
		m_pbitstrmOut -> putBits (0, 1, "VOL_Fixed_Vop_Rate");
		m_statsVOL.nBitsHead += NUMBITS_VOL_SHAPE + NUMBITS_TIME_RESOLUTION + 1;
		if (m_volmd.fAUsage == RECTANGLE) {
			m_pbitstrmOut -> putBits (MARKER_BIT, 1, "Marker_Bit");
			m_statsVOL.nBitsStuffing ++;
			m_pbitstrmOut -> putBits (iSessionWidth, NUMBITS_VOP_WIDTH, "VOL_Width"); 
			m_pbitstrmOut -> putBits (MARKER_BIT, 1, "Marker_Bit");
			m_statsVOL.nBitsStuffing ++;
			m_pbitstrmOut -> putBits (iSessionHeight, NUMBITS_VOP_HEIGHT, "VOL_Height"); 
			m_pbitstrmOut -> putBits (MARKER_BIT, 1, "Marker_Bit");
			m_statsVOL.nBitsStuffing ++;
			m_statsVOL.nBitsHead += NUMBITS_VOP_WIDTH + NUMBITS_VOP_HEIGHT;
		}
	}
	m_pbitstrmOut -> putBits (m_vopmd.bInterlace, (UInt) 1, "VOL_interlace");
	m_statsVOL.nBitsHead++;
	m_pbitstrmOut -> putBits (m_volmd.bAdvPredDisable, (UInt) 1, "VOL_OBMC_Disable");
	m_statsVOL.nBitsHead++;

	// code sprite info
// GMC
	if(m_volmd.uiVerID == 2){
		m_pbitstrmOut -> putBits (m_uiSprite, 2, "VOL_Sprite_Usage");
		m_statsVOL.nBitsHead += 2;
	}else{
// ~GMC
		m_pbitstrmOut -> putBits (m_uiSprite, 1, "VOL_Sprite_Usage");
		m_statsVOL.nBitsHead ++;
// GMC
	}
// ~GMC
	if (m_uiSprite == 1) {
		m_pbitstrmOut -> putBits (m_rctSpt.width, NUMBITS_SPRITE_HDIM, "SPRT_hdim");
		m_statsVOL.nBitsHead += NUMBITS_SPRITE_HDIM; 
		m_pbitstrmOut -> putBits (MARKER_BIT, 1, "Marker_Bit");
		m_statsVOL.nBitsStuffing ++;
		m_pbitstrmOut -> putBits (m_rctSpt.height (), NUMBITS_SPRITE_VDIM, "SPRT_vdim");
		m_statsVOL.nBitsHead += NUMBITS_SPRITE_VDIM;
		m_pbitstrmOut -> putBits (MARKER_BIT, 1, "Marker_Bit");
		m_statsVOL.nBitsStuffing ++;
		if (m_rctSpt.left >= 0) {
			m_pbitstrmOut -> putBits (m_rctSpt.left, NUMBITS_SPRITE_LEFT_EDGE, "SPRT_Left_Edge");
		} else {
			m_pbitstrmOut -> putBits (m_rctSpt.left & 0x00001FFFF, NUMBITS_SPRITE_LEFT_EDGE, "SPRT_Left_Edge");
		}
		m_statsVOL.nBitsHead += NUMBITS_SPRITE_LEFT_EDGE;
		m_pbitstrmOut -> putBits (MARKER_BIT, 1, "Marker_Bit");
		m_statsVOL.nBitsStuffing += 1;
		if (m_rctSpt.top >= 0) {
			m_pbitstrmOut -> putBits (m_rctSpt.top, NUMBITS_SPRITE_TOP_EDGE, "SPRT_Top_Edge");
		} else {
			m_pbitstrmOut -> putBits (m_rctSpt.top & 0x00001FFFF, NUMBITS_SPRITE_TOP_EDGE, "SPRT_Top_Edge");
		}
		m_statsVOL.nBitsHead += NUMBITS_SPRITE_TOP_EDGE; 
		m_pbitstrmOut -> putBits (MARKER_BIT, 1, "Marker_Bit");
		m_statsVOL.nBitsStuffing += 1;
// GMC
	}
	if (m_uiSprite == 1 || m_uiSprite == 2) {
// ~GMC
		m_pbitstrmOut -> putBits (m_iNumOfPnts, NUMBITS_NUM_SPRITE_POINTS, "SPRT_Num_Pnt");
		m_statsVOL.nBitsHead += NUMBITS_NUM_SPRITE_POINTS;
		m_pbitstrmOut -> putBits (m_uiWarpingAccuracy, NUMBITS_WARPING_ACCURACY, "SPRT_Warping_Accuracy");
		m_statsVOL.nBitsHead += NUMBITS_WARPING_ACCURACY;
		Bool bLightChange = 0;
		m_pbitstrmOut -> putBits (bLightChange, 1, "SPRT_Brightness_Change");
		m_statsVOL.nBitsHead++;
// GMC
	}
	if (m_uiSprite == 1) {
// ~GMC

// Begin: modified by Hughes	  4/9/98
		if (m_sptMode == BASIC_SPRITE)
			m_pbitstrmOut -> putBits (FALSE, 1, "Low_latency_sprite_enable");
		else
			m_pbitstrmOut -> putBits (TRUE, 1, "Low_latency_sprite_enable");
		m_statsVOL.nBitsHead++;
// End: modified by Hughes	  4/9/98
	}

	// HHI Schueuer sadct_disable flag inserted
	// HHI Suehring 991022 only if bitstream version=2
    if(m_volmd.uiVerID == 2) {
		if (m_volmd.fAUsage != RECTANGLE)
		{
			if (m_volmd.bSadctDisable)
				m_pbitstrmOut -> putBits (TRUE, 1, "sadct_disable");
			else
				m_pbitstrmOut -> putBits (FALSE, 1, "sadct_disable");
		}
	}
	else if(!m_volmd.bSadctDisable)
		fatal_error("VersionID must = 2 for SADCT to work");
	// end sadct

	// NBIT
	m_pbitstrmOut -> putBits ((Int) m_volmd.bNot8Bit, 1, "VOL_NOT_8_BIT_VIDEO");
	m_statsVOL.nBitsHead++;
	if (m_volmd.bNot8Bit) {
		m_pbitstrmOut->putBits ((Int) m_volmd.uiQuantPrecision, 4, "QUANT_PRECISION");
		m_pbitstrmOut->putBits ((Int) m_volmd.nBits, 4, "BITS_PER_PIXEL");
		m_statsVOL.nBitsHead+=8;
	}

	if (m_volmd.fAUsage == EIGHT_BIT)	{
		m_pbitstrmOut -> putBits (m_volmd.bNoGrayQuantUpdate, 1, "VOL_Disable_Gray_Q_Update");
		m_pbitstrmOut -> putBits (0, 1, "Composition_Method");
		m_pbitstrmOut -> putBits (1, 1, "Linear_Composition");
		m_statsVOL.nBitsHead+=3;
	}
	m_pbitstrmOut -> putBits ((Int) m_volmd.fQuantizer, 1, "VOL_Quant_Type"); 
	m_statsVOL.nBitsHead++;
	if (m_volmd.fQuantizer == Q_MPEG) {
		m_pbitstrmOut -> putBits (m_volmd.bLoadIntraMatrix, 1, "VOL_Load_Q_Matrix (intra)");
		m_statsVOL.nBitsHead++;
		if (m_volmd.bLoadIntraMatrix) {
			UInt i = 0;
			do {
				m_pbitstrmOut -> putBits (m_volmd.rgiIntraQuantizerMatrix [grgiStandardZigzag[i]], NUMBITS_QMATRIX, "VOL_Quant_Matrix(intra)");
				m_statsVOL.nBitsHead += NUMBITS_QMATRIX;
			} while (m_volmd.rgiIntraQuantizerMatrix [grgiStandardZigzag[i]] != 0 && ++i < BLOCK_SQUARE_SIZE);
			for (UInt j = i; j < BLOCK_SQUARE_SIZE; j++) {
				m_volmd.rgiIntraQuantizerMatrix [grgiStandardZigzag[j]] = m_volmd.rgiIntraQuantizerMatrix [grgiStandardZigzag[i - 1]];
			}
		}
		m_pbitstrmOut -> putBits (m_volmd.bLoadInterMatrix, 1, "VOL_Load_Q_Matrix (inter)");
		m_statsVOL.nBitsHead++;
		if (m_volmd.bLoadInterMatrix) {
			UInt i = 0;
			do {
				m_pbitstrmOut -> putBits (m_volmd.rgiInterQuantizerMatrix [grgiStandardZigzag[i]], NUMBITS_QMATRIX, "VOL_Quant_Matrix(intra)");
				m_statsVOL.nBitsHead += NUMBITS_QMATRIX;
			} while (m_volmd.rgiInterQuantizerMatrix [grgiStandardZigzag[i]] != 0 && ++i < BLOCK_SQUARE_SIZE);
			for (UInt j = i; j < BLOCK_SQUARE_SIZE; j++) {
				m_volmd.rgiInterQuantizerMatrix [grgiStandardZigzag[j]] = m_volmd.rgiInterQuantizerMatrix [grgiStandardZigzag[i - 1]];
			}
		}
		if (m_volmd.fAUsage == EIGHT_BIT)	{
      for(Int iAuxComp=0; iAuxComp<m_volmd.iAuxCompCount; iAuxComp++ ) { // MAC (SB) 2-Dec-99
        m_pbitstrmOut -> putBits (m_volmd.bLoadIntraMatrixAlpha, 1, "VOL_Load_Alpha_Q_Matrix (intra)");
        m_statsVOL.nBitsHead++;
        if (m_volmd.bLoadIntraMatrixAlpha) {
          for (UInt i = 0; i < BLOCK_SQUARE_SIZE; i++) {
            m_pbitstrmOut -> putBits (m_volmd.rgiIntraQuantizerMatrixAlpha[0] [grgiStandardZigzag[i]], NUMBITS_QMATRIX, "VOL_Alpha_Quant_Matrix(intra)");
            m_statsVOL.nBitsHead += NUMBITS_QMATRIX;
          }
        }
        m_pbitstrmOut -> putBits (m_volmd.bLoadInterMatrixAlpha, 1, "VOL_Load_Alpha_Q_Matrix (inter)");
        m_statsVOL.nBitsHead++;
        if (m_volmd.bLoadInterMatrixAlpha) {
          for (UInt i = 0; i < BLOCK_SQUARE_SIZE; i++) {
            m_pbitstrmOut -> putBits (m_volmd.rgiInterQuantizerMatrixAlpha[0] [grgiStandardZigzag[i]], NUMBITS_QMATRIX, "VOL_Alpha_Quant_Matrix(inter)");
            m_statsVOL.nBitsHead += NUMBITS_QMATRIX;
          }
        }
			}
		}
	}

  if(m_volmd.uiVerID == 2){ // GMC for Version ID
    // m_volmd.uiVerID is added for compatibility with version 1 video
    // this is tentative integration
    // please change codes if there is any problems
    // Quarter Sample
	m_pbitstrmOut -> putBits (m_volmd.bQuarterSample, 1, "VOL_Quarter_Sample");
	m_statsVOL.nBitsHead += 1;
    // ~Quarter Sample
  }
  else if(m_volmd.bQuarterSample)
	  fatal_error("VersionID must = 2 for quarter sample motion to work");

	// START: Complexity Estimation syntax support - Marc Mongenet (EPFL) - 16 Jun 1998
	m_pbitstrmOut -> putBits (m_volmd.bComplexityEstimationDisable,
							  1, "complexity_estimation_disable");
	m_statsVOL.nBitsHead += 1;
	if (! m_volmd.bComplexityEstimationDisable) {

		// START: Complexity Estimation syntax support - Update version 2 - Massimo Ravasi (EPFL) - 5 Nov 1999
		assert ((m_volmd.iEstimationMethod == 0) || (m_volmd.iEstimationMethod == 1));
		// Replaced line: m_volmd.iEstimationMethod = 0; // only known estimation method at now (18 Jun 1998)
		// iEstimationMethod is now initialized in CSessionEncoder::CSessionEncoder(...)
		// END: Complexity Estimation syntax support - Update version 2

		// START: Complexity Estimation syntax support - Update version 2 - Massimo Ravasi (EPFL) - 11 Nov 1999
		// Following lines not necessary: redundant flags to avoid start code emulation
		//  are already computed in sesenc.cpp

		/*
		m_volmd.bShapeComplexityEstimationDisable =
		  !(m_volmd.bOpaque ||
			m_volmd.bTransparent ||
			m_volmd.bIntraCAE ||
			m_volmd.bInterCAE ||
			m_volmd.bNoUpdate ||
			m_volmd.bUpsampling);

		m_volmd.bTextureComplexityEstimationSet1Disable =
		  !(m_volmd.bIntraBlocks ||
			m_volmd.bInterBlocks ||
			m_volmd.bInter4vBlocks ||
			m_volmd.bNotCodedBlocks);

		m_volmd.bTextureComplexityEstimationSet2Disable =
		  !(m_volmd.bDCTCoefs ||
			m_volmd.bDCTLines ||
			m_volmd.bVLCSymbols ||
			m_volmd.bVLCBits);

		m_volmd.bMotionCompensationComplexityDisable =
		  !(m_volmd.bAPM ||
			m_volmd.bNPM ||
			m_volmd.bInterpolateMCQ ||
			m_volmd.bForwBackMCQ ||
			m_volmd.bHalfpel2 ||
			m_volmd.bHalfpel4);

		if (m_volmd.iEstimationMethod == 1) {
			m_volmd.bVersion2ComplexityEstimationDisable =
			  !(m_volmd.bSadct ||
				m_volmd.bQuarterpel);
		}
		*/
		// END: Complexity Estimation syntax support - Update version 2

		// START: Complexity Estimation syntax support - Update version 2 - Massimo Ravasi (EPFL) - 5 Nov 1999
		// line: assert ((m_volmd.iEstimationMethod == 0) || (m_volmd.iEstimationMethod == 1));
		//  changed and moved at the beginning of this block
		// END: Complexity Estimation syntax support - Update version 2

		m_pbitstrmOut -> putBits (m_volmd.iEstimationMethod, 2, "estimation_method");
		m_statsVOL.nBitsHead += 2;

		m_pbitstrmOut -> putBits (m_volmd.bShapeComplexityEstimationDisable,
								  1, "shape_complexity_estimation_disable");
		m_statsVOL.nBitsHead += 1;
		if (! m_volmd.bShapeComplexityEstimationDisable) {
			m_pbitstrmOut -> putBits (m_volmd.bOpaque, 1, "opaque");
			m_pbitstrmOut -> putBits (m_volmd.bTransparent, 1, "transparent");
			m_pbitstrmOut -> putBits (m_volmd.bIntraCAE, 1, "intra_cae");
			m_pbitstrmOut -> putBits (m_volmd.bInterCAE, 1, "inter_cae");
			m_pbitstrmOut -> putBits (m_volmd.bNoUpdate, 1, "no_update");
			m_pbitstrmOut -> putBits (m_volmd.bUpsampling, 1, "upsampling");
			m_statsVOL.nBitsHead += 6;
		}

		m_pbitstrmOut -> putBits (m_volmd.bTextureComplexityEstimationSet1Disable,
								  1, "texture_complexity_estimation_set_1_disable");
		m_statsVOL.nBitsHead += 1;
		if (! m_volmd.bTextureComplexityEstimationSet1Disable) {
			m_pbitstrmOut -> putBits (m_volmd.bIntraBlocks, 1, "intra_blocks");
			m_pbitstrmOut -> putBits (m_volmd.bInterBlocks, 1, "inter_blocks");
			m_pbitstrmOut -> putBits (m_volmd.bInter4vBlocks, 1, "inter4v_blocks");
			m_pbitstrmOut -> putBits (m_volmd.bNotCodedBlocks, 1, "not_coded_blocks");
			m_statsVOL.nBitsHead += 4;
		}

		m_pbitstrmOut -> putBits (1, 1, "Marker_Bit");
		m_statsVOL.nBitsStuffing ++;

		m_pbitstrmOut -> putBits (m_volmd.bTextureComplexityEstimationSet2Disable,
								  1, "texture_complexity_estimation_set_2_disable");
		m_statsVOL.nBitsHead += 1;
		if (! m_volmd.bTextureComplexityEstimationSet2Disable) {
			m_pbitstrmOut -> putBits (m_volmd.bDCTCoefs, 1, "dct_coefs");
			m_pbitstrmOut -> putBits (m_volmd.bDCTLines, 1, "dct_lines");
			m_pbitstrmOut -> putBits (m_volmd.bVLCSymbols, 1, "vlc_symbols");
			m_pbitstrmOut -> putBits (m_volmd.bVLCBits, 1, "vlc_bits");
			m_statsVOL.nBitsHead += 4;
		}

		m_pbitstrmOut -> putBits (m_volmd.bMotionCompensationComplexityDisable,
								  1, "motion_compensation_complexity_disable");
		m_statsVOL.nBitsHead += 1;
		if (! m_volmd.bMotionCompensationComplexityDisable) {
			m_pbitstrmOut -> putBits (m_volmd.bAPM, 1, "apm");
			m_pbitstrmOut -> putBits (m_volmd.bNPM, 1, "npm");
			m_pbitstrmOut -> putBits (m_volmd.bInterpolateMCQ, 1, "interpolate_mc_q");
			m_pbitstrmOut -> putBits (m_volmd.bForwBackMCQ, 1, "forw_back_mc_q");
			m_pbitstrmOut -> putBits (m_volmd.bHalfpel2, 1, "halfpel2");
			m_pbitstrmOut -> putBits (m_volmd.bHalfpel4, 1, "halfpel4");
			m_statsVOL.nBitsHead += 6;
		}

		m_pbitstrmOut -> putBits (1, 1, "Marker_Bit");
		m_statsVOL.nBitsStuffing ++;

		// START: Complexity Estimation syntax support - Update version 2 - Massimo Ravasi (EPFL) - 5 Nov 1999
		if (m_volmd.iEstimationMethod == 1) {
			m_pbitstrmOut -> putBits (m_volmd.bVersion2ComplexityEstimationDisable,
									  1, "version2_complexity_estimation_disable");
			m_statsVOL.nBitsHead += 1;
			if (! m_volmd.bVersion2ComplexityEstimationDisable) {
				m_pbitstrmOut -> putBits (m_volmd.bSadct, 1, "sadct");
				m_pbitstrmOut -> putBits (m_volmd.bQuarterpel, 1, "quarterpel");
				m_statsVOL.nBitsHead += 2;
			}
		}
		// END: Complexity Estimation syntax support - Update version 2
	}
	// END: Complexity Estimation syntax support
//RESYNC_MARKER_FIX
	m_pbitstrmOut -> putBits (m_volmd.bResyncMarkerDisable , 1, "VOL_resync_marker_disable");
//	m_pbitstrmOut -> putBits (0, 1, "VOL_resync_marker_disable");
//~RESYNC_MARKER_FIX
	m_statsVOL.nBitsHead ++;

	//	Modified by Toshiba(1998-4-7)
	m_pbitstrmOut -> putBits (m_volmd.bDataPartitioning, 1, "VOL_data_partitioning");
	m_statsVOL.nBitsHead ++;
	if( m_volmd.bDataPartitioning )
	{
		m_pbitstrmOut -> putBits (m_volmd.bReversibleVlc, 1, "VOL_reversible_vlc");
		m_statsVOL.nBitsHead ++;
	}
	//	End Toshiba

// NEWPRED
	if(m_volmd.uiVerID != 1) { 
		m_pbitstrmOut -> putBits(m_volmd.bNewpredEnable, NUMBITS_NEWPRED_ENABLE, "*VOL_newpred_enable");
		m_statsVOL.nBitsHead += NUMBITS_NEWPRED_ENABLE;
		if(m_volmd.bNewpredEnable) {
			m_volmd.iRequestedBackwardMessegeType = NP_REQUESTED_BACKWARD_MESSAGE_TYPE;
			m_pbitstrmOut -> putBits(m_volmd.iRequestedBackwardMessegeType, NUMBITS_REQUESTED_BACKWARD_MESSAGE_TYPE, "*VOL_requested_backward_message_type");
			m_statsVOL.nBitsHead += NUMBITS_REQUESTED_BACKWARD_MESSAGE_TYPE;
			m_pbitstrmOut -> putBits(m_volmd.bNewpredSegmentType, NUMBITS_NEWPRED_SEGMENT_TYPE, "*VOL_newpred_segment_type");
			m_statsVOL.nBitsHead += NUMBITS_NEWPRED_SEGMENT_TYPE;
		}
// RRV insertion
		m_pbitstrmOut -> putBits (m_volmd.breduced_resolution_vop_enable, 1, "reduced_resolution_vop_enable");
		m_statsVOL.nBitsHead ++;
// ~RRV
	}
// ~NEWPRED

	m_pbitstrmOut -> putBits (m_volmd.volType == ENHN_LAYER, 1, "VOL_Scalability");
	m_statsVOL.nBitsHead++;
	if (m_volmd.volType == ENHN_LAYER)	{
//#ifdef _Scalable_SONY_
		m_pbitstrmOut -> putBits (m_volmd.iHierarchyType, 1, "VOL_Hierarchy_Type");
//#endif _Scalable_SONY_
		m_pbitstrmOut -> putBits (0, 4, "VOL_Ref_Layer_Id");
		m_pbitstrmOut -> putBits (0, 1, "VOL_Ref_Layer_Sampling_Dir");
/*Added*/
		m_pbitstrmOut -> putBits (m_volmd.ihor_sampling_factor_n, 5, "VOL_Horizontal_Sampling_Factor");			//The vm is not very clear about this.
		m_pbitstrmOut -> putBits (m_volmd.ihor_sampling_factor_m, 5, "VOL_Horizontal_Sampling_Factor_Ref");		//Always 1

		m_pbitstrmOut -> putBits (m_volmd.iver_sampling_factor_n, 5, "VOL_Vertical_Sampling_Factor");			//The vm is not very clear about this.
		m_pbitstrmOut -> putBits (m_volmd.iver_sampling_factor_m, 5, "VOL_Vertical_Sampling_Factor_Ref");		//Always 1
//OBSS_SAIT_991015		//for OBSS partial enhancement mode
//OBSSFIX_MODE3
		m_pbitstrmOut -> putBits (m_volmd.iEnhnType!=0?1:0, 1, "VOL_Ehnancement_Type"); // enhancement_type for scalability
//		if ( m_volmd.fAUsage == ONE_BIT && m_volmd.iHierarchyType==0)	
//			m_pbitstrmOut -> putBits (m_volmd.iEnhnTypeSpatial!=0?1:0, 1, "VOL_Ehnancement_Type"); // enhancement_type for scalability // modified by Sharp (98/3/24)
//		else
//			m_pbitstrmOut -> putBits (m_volmd.iEnhnType!=0?1:0, 1, "VOL_Ehnancement_Type"); // enhancement_type for scalability // modified by Sharp (98/3/24)
//~OBSSFIX_MODE3
//~OBSS_SAIT_991015
		m_statsVOL.nBitsHead += 26;
//OBSS_SAIT_991015
		if ( m_volmd.fAUsage == ONE_BIT && m_volmd.iHierarchyType==0)	{
			m_pbitstrmOut -> putBits (m_volmd.iuseRefShape, 1, "VOL_Use_Ref_Shape");
			m_pbitstrmOut -> putBits (m_volmd.iuseRefTexture, 1, "VOL_Use_Ref_Texture");
			m_pbitstrmOut -> putBits (m_volmd.ihor_sampling_factor_n_shape, 5, "VOL_Horizontal_Sampling_Factor_SHAPE");		
			m_pbitstrmOut -> putBits (m_volmd.ihor_sampling_factor_m_shape, 5, "VOL_Horizontal_Sampling_Factor_Ref_SHAPE");	
			m_pbitstrmOut -> putBits (m_volmd.iver_sampling_factor_n_shape, 5, "VOL_Vertical_Sampling_Factor_SHAPE");	
			m_pbitstrmOut -> putBits (m_volmd.iver_sampling_factor_m_shape, 5, "VOL_Vertical_Sampling_Factor_Ref_SHAPE");
			m_statsVOL.nBitsHead += 22;
		}
//~OBSS_SAIT_991015
	}
// Begin: modified by Hughes	  4/9/98	  per clause 2.1.7. in N2171 document
//	m_pbitstrmOut -> putBits ((Int) 0, 1, "VOL_Random_Access");		//isn't this a system level flg?
//	m_statsVOL.nBitsHead++;
// End: modified by Hughes	  4/9/98
#if 0
	cout << "VOL Overhead" << "\t\t" << m_statsVOL.nBitsHead << "\n\n";
	cout.flush ();
#endif
}

//encode GOV header, modified by SONY 980212
Void CVideoObjectEncoder::codeGOVHead (Time t)
{
#if 0
	cout << "GOV Header (t=" << t << ")\n\n";
#endif

	m_pbitstrmOut->reset ();		
	
	// Start code
	m_pbitstrmOut -> putBits (START_CODE_PREFIX, NUMBITS_START_CODE_PREFIX, "GOV_Start_Code");
	m_pbitstrmOut -> putBits (GOV_START_CODE, NUMBITS_GOV_START_CODE, "GOV_Start_Code");
	m_statsVOL.nBitsHead += NUMBITS_START_CODE_PREFIX + NUMBITS_GOV_START_CODE;

	// Time code
/*modified by SONY (98/03/30) */
/*	Time tGOVCode = ((Int)(t * m_volmd.iClockRate / m_volmd.dFrameHz)) / m_volmd.iClockRate;*/
/*modified by SONY (98/03/30) End*/
/*              ORIGINAL
	Time tGOVCode = ((Int)(t * m_volmd.iClockRate / m_volmd.dFrameHz) - m_volmd.iBbetweenP) / m_volmd.iClockRate;
*/

	// modified by swinder 980511
	Time tGOVCode = (Time)(t / m_volmd.dFrameHz);
	// this is because t is the source frame count and dFrameHz is the source frame rate,
	// tGOVCode is therefore in seconds rounded down. I see no reason for clock rate to be
	// used here - swinder

	if(tGOVCode<0) tGOVCode=0;
	Time tCodeHou = (tGOVCode / 3600)%24;
	Time tCodeMin = (tGOVCode / 60) % 60;
	Time tCodeSec = tGOVCode % 60;

	m_pbitstrmOut -> putBits (tCodeHou, NUMBITS_GOV_TIMECODE_HOUR, "GOV_Time_Code_Hour");
	m_statsVOL.nBitsHead += NUMBITS_GOV_TIMECODE_HOUR;
	m_pbitstrmOut -> putBits (tCodeMin, NUMBITS_GOV_TIMECODE_MIN, "GOV_Time_Code_Min");
	m_statsVOL.nBitsHead += NUMBITS_GOV_TIMECODE_MIN;
	m_pbitstrmOut -> putBits (MARKER_BIT, 1, "Marker_Bit");
	m_statsVOL.nBitsStuffing += 1;
	m_pbitstrmOut -> putBits (tCodeSec, NUMBITS_GOV_TIMECODE_SEC, "GOV_Time_Code_Sec");
	m_statsVOL.nBitsHead += NUMBITS_GOV_TIMECODE_SEC;

	Int iClosedGOV;
	if ((m_vopmd.vopPredType==IVOP)&&(m_volmd.iBbetweenP==0))
		iClosedGOV=1;
	else
		iClosedGOV=0;
	m_pbitstrmOut -> putBits (iClosedGOV, 1, "GOV_Closed");
	m_statsVOL.nBitsHead += 1;
	m_pbitstrmOut -> putBits (0, 1, "GOV_Broken_Link");
	m_statsVOL.nBitsHead += 1;
	
	m_statsVOL.nBitsStuffing += m_pbitstrmOut->flush ();
	//m_pbitstrmOut -> putBits (15, 4, "byte_align");
	//m_statsVOL.nBitsHead += 4;
	
	m_tModuloBaseDisp = tGOVCode;
	m_tModuloBaseDecd = tGOVCode;
}
//980212

Void CVideoObjectEncoder::codeVOPHeadInitial()
{
	// Start code	
	m_pbitstrmOut -> putBits (START_CODE_PREFIX, NUMBITS_START_CODE_PREFIX, "VOP_Start_Code");
	m_pbitstrmOut -> putBits (VOP_START_CODE, NUMBITS_VOP_START_CODE, "VOP_Start_Code");
	m_statsVOP.nBitsHead += NUMBITS_START_CODE_PREFIX + NUMBITS_VOP_START_CODE;

	// prediction type
	m_pbitstrmOut -> putBits (m_vopmd.vopPredType, NUMBITS_VOP_PRED_TYPE, "VOP_Pred_Type");
	m_statsVOP.nBitsHead += NUMBITS_VOP_PRED_TYPE;

	// Time reference
	Time tCurrSec = (Int)(m_t / m_volmd.dFrameHz);		// current time in seconds
//	Time tCurrSec = m_t / SOURCE_FRAME_RATE;		    // current time in seconds
	m_nBitsModuloBase = tCurrSec - (((m_vopmd.vopPredType != BVOP) ||
		(m_vopmd.vopPredType == BVOP && m_volmd.volType == ENHN_LAYER )) ?
			m_tModuloBaseDecd : m_tModuloBaseDisp);
	assert(m_nBitsModuloBase<=31);

	m_pbitstrmOut -> putBits ((Int) 0xFFFFFFFE, (UInt) m_nBitsModuloBase + 1, "VOP_Modulo_Time_Base");
	m_statsVOP.nBitsHead += m_nBitsModuloBase + 1;

	m_iVopTimeIncr = (Int)(m_t * m_volmd.iClockRate / m_volmd.dFrameHz - tCurrSec * m_volmd.iClockRate);
//	m_iVopTimeIncr = m_t * m_volmd.iClockRate / SOURCE_FRAME_RATE - tCurrSec * m_volmd.iClockRate;

	m_pbitstrmOut->putBits ((Int) 1, 1, "Marker"); // marker bit	Added for error resilient mode by Toshiba(1997-11-14)
	m_statsVOP.nBitsStuffing ++;

	if(m_iNumBitsTimeIncr!=0)
		m_pbitstrmOut -> putBits (m_iVopTimeIncr, m_iNumBitsTimeIncr, "VOP_Time_Incr");
	m_statsVOP.nBitsHead += m_iNumBitsTimeIncr;// Modified for error resilient mode by Toshiba(1997-11-14)

	m_pbitstrmOut->putBits ((Int) 1, 1, "Marker"); // marker bit
	m_statsVOP.nBitsStuffing ++;

	if ( m_vopmd.vopPredType != BVOP ||
		(m_vopmd.vopPredType == BVOP && m_volmd.volType == ENHN_LAYER ))
	{	//update modulo time base
		m_tModuloBaseDisp = m_tModuloBaseDecd;							//of the most recently displayed I/Pvop
		m_tModuloBaseDecd = tCurrSec;							//of the most recently decoded I/Pvop
	}
}

Void CVideoObjectEncoder::codeNonCodedVOPHead ()
{
	codeVOPHeadInitial();

	m_pbitstrmOut -> putBits (0, 1, "VOP_Coded");
	m_statsVOP.nBitsHead++;
}


//OBSSFIX_MODE3
//Bool BGComposition = FALSE;		//OBSS_SAIT_991015 //for OBSS partial enhancement mode
//~OBSSFIX_MODE3

Void CVideoObjectEncoder::codeVOPHead ()
{
	codeVOPHeadInitial();

	m_pbitstrmOut -> putBits (1, 1, "VOP_Coded");
	m_statsVOP.nBitsHead++;

// NEWPRED
	if(m_volmd.bNewpredEnable) {
		g_pNewPredEnc->SetVPData(
				NP_VOP_HEADER,
				&m_vopmd.m_iVopID,	
				&m_vopmd.m_iNumBitsVopID,
				&m_vopmd.m_iVopID4Prediction_Indication,
				&m_vopmd.m_iVopID4Prediction
		);

		m_pbitstrmOut -> putBits(m_vopmd.m_iVopID, m_vopmd.m_iNumBitsVopID, "VOP_Id");
		m_statsVOP.nBitsHead += m_vopmd.m_iNumBitsVopID;

		m_pbitstrmOut->putBits(m_vopmd.m_iVopID4Prediction_Indication,
					NUMBITS_VOP_ID_FOR_PREDICTION_INDICATION, "VOP_Id_For_Prediction_Indication");
		m_statsVOP.nBitsHead += NUMBITS_VOP_ID_FOR_PREDICTION_INDICATION;
		if( m_vopmd.m_iVopID4Prediction_Indication )
		{
			m_pbitstrmOut->putBits(m_vopmd.m_iVopID4Prediction, m_vopmd.m_iNumBitsVopID, "VOP_Id_For_Prediction");
			m_statsVOP.nBitsHead += m_vopmd.m_iNumBitsVopID;
		}
		m_pbitstrmOut->putBits(MARKER_BIT, MARKER_BIT, "Marker_Bit");
		m_statsVOP.nBitsHead += MARKER_BIT;
	}
// ~NEWPRED

	if(m_volmd.bShapeOnly==FALSE)
	{
		if ((m_vopmd.vopPredType == PVOP) || ((m_uiSprite == 2) && (m_vopmd.vopPredType == SPRITE))) { // GMC
			m_pbitstrmOut -> putBits (m_vopmd.iRoundingControl, 1, "VOP_Rounding_Type");
			m_statsVOP.nBitsHead++;
		}
	}
// RRV insertion
	if((m_volmd.breduced_resolution_vop_enable == 1)&&(m_volmd.fAUsage == RECTANGLE)&&
	   ((m_vopmd.vopPredType == PVOP)||(m_vopmd.vopPredType == IVOP)))
	{
	  	m_pbitstrmOut -> putBits (m_vopmd.RRVmode.iRRVOnOff, 1, "VOP_Reduced_Resolution");
		m_statsVOP.nBitsHead ++;
	}	
// ~RRV
	if (m_volmd.fAUsage != RECTANGLE) {
		if (!(m_uiSprite == 1 && m_vopmd.vopPredType == IVOP)) {
			m_pbitstrmOut->putBits (m_rctCurrVOPY.width, NUMBITS_VOP_WIDTH, "VOP_Width");
			m_pbitstrmOut->putBits ((Int) 1, 1, "Marker"); // marker bit
			m_pbitstrmOut->putBits (m_rctCurrVOPY.height (), NUMBITS_VOP_HEIGHT, "VOP_Height");
			m_pbitstrmOut->putBits ((Int) 1, 1, "Marker"); // marker bit
			m_statsVOP.nBitsHead += NUMBITS_VOP_WIDTH + NUMBITS_VOP_HEIGHT;
			m_statsVOP.nBitsStuffing += 2;

			if (m_rctCurrVOPY.left >= 0) {
				m_pbitstrmOut -> putBits (m_rctCurrVOPY.left, NUMBITS_VOP_HORIZONTAL_SPA_REF, "VOP_Hori_Ref");
			} else {
				m_pbitstrmOut -> putBits (m_rctCurrVOPY.left & 0x00001FFFF, NUMBITS_VOP_HORIZONTAL_SPA_REF, "VOP_Hori_Ref");
			}
			m_pbitstrmOut->putBits ((Int) 1, 1, "Marker"); // marker bit
			if (m_rctCurrVOPY.top >= 0) {
				m_pbitstrmOut -> putBits (m_rctCurrVOPY.top, NUMBITS_VOP_VERTICAL_SPA_REF, "VOP_Vert_Ref");
			} else {
				m_pbitstrmOut -> putBits (m_rctCurrVOPY.top & 0x00001FFFF, NUMBITS_VOP_VERTICAL_SPA_REF, "VOP_Vert_Ref");
			}
			m_pbitstrmOut->putBits ((Int) 1, 1, "Marker"); // marker bit added Nov 10 1999, swinder
			m_statsVOP.nBitsHead += NUMBITS_VOP_HORIZONTAL_SPA_REF + NUMBITS_VOP_VERTICAL_SPA_REF;
			m_statsVOP.nBitsStuffing += 2;
		}

//OBSS_SAIT_991015		//for OBSS partial enhancement mode
//OBSSFIX_MODE3
		if ( m_volmd.uiVerID != 1 && (m_volmd.volType == ENHN_LAYER && m_volmd.iHierarchyType==0 && m_volmd.iEnhnType != 0) ) {
			if(m_volmd.iEnhnType == 1)
				m_vopmd.bBGComposition = TRUE;	//Spatial Scalability Composition 
			else
				m_vopmd.bBGComposition = FALSE;	//Spatial Scalability Composition 
			m_pbitstrmOut->putBits ((m_vopmd.bBGComposition == TRUE? 1 : 0), 1, "Background Composition");
			m_statsVOP.nBitsHead++;
		} else

//		if ( m_volmd.uiVerID != 1 && (m_volmd.volType == ENHN_LAYER && m_volmd.iHierarchyType==0 && m_volmd.iEnhnTypeSpatial == 1 && m_volmd.iuseRefShape == 0) ) {
//			BGComposition = TRUE;	//Spatial Scalability Composition 
//			m_pbitstrmOut->putBits ((BGComposition == TRUE? 1 : 0), 1, "Background Composition");
//			m_statsVOP.nBitsHead++;
//		} else
//~OBSSFIX_MODE3
//~OBSS_SAIT_991015
		if ( m_volmd.volType == ENHN_LAYER && m_volmd.iEnhnType == 1 ) {
			m_pbitstrmOut->putBits (1, 1, "Background Composition");
			m_statsVOP.nBitsHead++;
		}
// begin: added by Sharp (98/3/24)
		else if ( m_volmd.volType == ENHN_LAYER && m_volmd.iEnhnType == 2 ) {
			m_pbitstrmOut->putBits (0, 1, "Background Composition");
			m_statsVOP.nBitsHead++;
		}
// end: added by Sharp (98/3/24)

		m_pbitstrmOut->putBits (m_volmd.bNoCrChange, 1, "VOP_CR_Change_Disable");
		m_statsVOP.nBitsHead++;
        // tentative solution for normal sprite bitstream exchange
		m_pbitstrmOut->putBits (0, 1, "VOP_Constant_Alpha");
		m_statsVOP.nBitsHead++;
	}

	// START: Complexity Estimation syntax support - Marc Mongenet (EPFL) - 15 Jun 1998	
	if (! m_volmd.bComplexityEstimationDisable) {
		// START: Complexity Estimation syntax support - Update version 2 - Massimo Ravasi (EPFL) - 5 Nov 1999
		assert ((m_volmd.iEstimationMethod == 0) || (m_volmd.iEstimationMethod == 1));
		// Replaced line: assert (m_volmd.iEstimationMethod == 0);
		// END: Complexity Estimation syntax support - Update version 2

		// In real life the following values should be estimated by the encoder,
		// but this is only a syntax support so arbitrary complexity values are assigned.
		m_vopmd.iOpaque = 77;
		m_vopmd.iTransparent = 97;
		m_vopmd.iIntraCAE = 114;
		m_vopmd.iInterCAE = 99;
		m_vopmd.iNoUpdate = 32;
		m_vopmd.iUpsampling = 32;
		m_vopmd.iIntraBlocks = 171;
		m_vopmd.iNotCodedBlocks = 171;
		m_vopmd.iDCTCoefs = 215;
		m_vopmd.iDCTLines = 187;
		m_vopmd.iVLCSymbols = 187;
		m_vopmd.iVLCBits = 10;
		m_vopmd.iInterBlocks = 77;
		m_vopmd.iInter4vBlocks = 111;
		m_vopmd.iAPM = 110;
		m_vopmd.iNPM = 103;
		m_vopmd.iForwBackMCQ = 101;
		m_vopmd.iHalfpel2 = 110;
		m_vopmd.iHalfpel4 = 101;
		m_vopmd.iInterpolateMCQ = 116;
		// START: Complexity Estimation syntax support - Update version 2 - Massimo Ravasi (EPFL) - 5 Nov 1999
		m_vopmd.iSadct = 17;
		m_vopmd.iQuarterpel = 71;
		// END: Complexity Estimation syntax support - Update version 2

		if (m_vopmd.vopPredType == IVOP ||
			m_vopmd.vopPredType == PVOP ||
			m_vopmd.vopPredType == BVOP)
		{	
			if (m_volmd.bOpaque) {
				m_pbitstrmOut -> putBits (codedDCECS (m_vopmd.iOpaque, 8), 8, "dcecs_opaque");
				m_statsVOP.nBitsHead += 8;
			}
			if (m_volmd.bTransparent) {
				m_pbitstrmOut -> putBits (codedDCECS (m_vopmd.iTransparent, 8), 8, "dcecs_transparent");
				m_statsVOP.nBitsHead += 8;
			}
			if (m_volmd.bIntraCAE) {
				m_pbitstrmOut -> putBits (codedDCECS (m_vopmd.iIntraCAE, 8), 8 , "dcecs_intra_cae");
				m_statsVOP.nBitsHead += 8;
			}
			if (m_volmd.bInterCAE) {
				m_pbitstrmOut -> putBits (codedDCECS (m_vopmd.iInterCAE, 8), 8 , "dcecs_inter_cae");
				m_statsVOP.nBitsHead += 8;
			}
			if (m_volmd.bNoUpdate) {
				m_pbitstrmOut -> putBits (codedDCECS (m_vopmd.iNoUpdate, 8), 8 , "dcecs_no_update");
				m_statsVOP.nBitsHead += 8;
			}
			if (m_volmd.bUpsampling) {
				m_pbitstrmOut -> putBits (codedDCECS (m_vopmd.iUpsampling, 8), 8 , "dcecs_upsampling");
				m_statsVOP.nBitsHead += 8;
			}
		}
		
		if (m_volmd.bIntraBlocks) {
			m_pbitstrmOut -> putBits (codedDCECS (m_vopmd.iIntraBlocks, 8), 8 , "dcecs_intra_blocks");
			m_statsVOP.nBitsHead += 8;
		}
		if (m_volmd.bNotCodedBlocks) {
			m_pbitstrmOut -> putBits (codedDCECS (m_vopmd.iNotCodedBlocks, 8), 8 , "dcecs_not_coded_blocks");
			m_statsVOP.nBitsHead += 8;
		}
		if (m_volmd.bDCTCoefs) {
			m_pbitstrmOut -> putBits (codedDCECS (m_vopmd.iDCTCoefs, 8), 8 , "dcecs_dct_coefs");
			m_statsVOP.nBitsHead += 8;
		}
		if (m_volmd.bDCTLines) {
			m_pbitstrmOut -> putBits (codedDCECS (m_vopmd.iDCTLines, 8), 8 , "dcecs_dct_lines");
			m_statsVOP.nBitsHead += 8;
		}
		if (m_volmd.bVLCSymbols) {
			m_pbitstrmOut -> putBits (codedDCECS (m_vopmd.iVLCSymbols, 8), 8 , "dcecs_vlc_symbols");
			m_statsVOP.nBitsHead += 8;
		}
		if (m_volmd.bVLCBits) {
			m_pbitstrmOut -> putBits (codedDCECS (m_vopmd.iVLCBits, 4), 4, "dcecs_vlc_bits");
			m_statsVOP.nBitsHead += 4;
		}
		
		if (m_vopmd.vopPredType == PVOP ||
			m_vopmd.vopPredType == BVOP ||
			// START: Complexity Estimation syntax support - Update version 2 - Massimo Ravasi (EPFL) - 11 Nov 1999
  			((m_vopmd.vopPredType) == SPRITE && (m_uiSprite == 1)) ) {
			// END: Complexity Estimation syntax support - Update version 2
			if (m_volmd.bInterBlocks) {
				m_pbitstrmOut -> putBits (codedDCECS (m_vopmd.iInterBlocks, 8), 8 , "dcecs_inter_blocks");
				m_statsVOP.nBitsHead += 8;
			}
			if (m_volmd.bInter4vBlocks) {
				m_pbitstrmOut -> putBits (codedDCECS (m_vopmd.iInter4vBlocks, 8), 8 , "dcecs_inter4v_blocks");
				m_statsVOP.nBitsHead += 8;
			}
			if (m_volmd.bAPM) {
				m_pbitstrmOut -> putBits (codedDCECS (m_vopmd.iAPM, 8), 8 , "dcecs_apm");
				m_statsVOP.nBitsHead += 8;
			}
			if (m_volmd.bNPM) {
				m_pbitstrmOut -> putBits (codedDCECS (m_vopmd.iNPM, 8), 8 , "dcecs_npm");
				m_statsVOP.nBitsHead += 8;
			}
			if (m_volmd.bForwBackMCQ) {
				m_pbitstrmOut -> putBits (codedDCECS (m_vopmd.iForwBackMCQ, 8), 8 , "dcecs_forw_back_mc_q");
				m_statsVOP.nBitsHead += 8;
			}
			if (m_volmd.bHalfpel2) {
				m_pbitstrmOut -> putBits (codedDCECS (m_vopmd.iHalfpel2, 8), 8 , "dcecs_halfpel2");
				m_statsVOP.nBitsHead += 8;
			}
			if (m_volmd.bHalfpel4) {
				m_pbitstrmOut -> putBits (codedDCECS (m_vopmd.iHalfpel4, 8), 8 , "dcecs_halfpel4");
				m_statsVOP.nBitsHead += 8;
			}
		}

		if (m_vopmd.vopPredType == BVOP ||
			// START: Complexity Estimation syntax support - Update version 2 - Massimo Ravasi (EPFL) - 11 Nov 1999
  			((m_vopmd.vopPredType) == SPRITE && (m_uiSprite == 1)) ) {
			// END: Complexity Estimation syntax support - Update version 2
			if (m_volmd.bInterpolateMCQ) {
				m_pbitstrmOut -> putBits (codedDCECS (m_vopmd.iInterpolateMCQ, 8), 8 , "dcecs_interpolate_mc_q");
				m_statsVOP.nBitsHead += 8;
			}
		}
		// START: Complexity Estimation syntax support - Update version 2 - Massimo Ravasi (EPFL) - 5 Nov 1999
		if (m_vopmd.vopPredType == IVOP ||
			m_vopmd.vopPredType == PVOP ||
			m_vopmd.vopPredType == BVOP) {
			if(m_volmd.bSadct) {
				m_pbitstrmOut -> putBits (codedDCECS (m_vopmd.iSadct, 8), 8 , "dcecs_sadct");
				m_statsVOP.nBitsHead += 8;
			}
		}

		if (m_vopmd.vopPredType == PVOP ||
			m_vopmd.vopPredType == BVOP ||
			// START: Complexity Estimation syntax support - Update version 2 - Massimo Ravasi (EPFL) - 11 Nov 1999
  			((m_vopmd.vopPredType) == SPRITE && (m_uiSprite == 1)) ) {
			// END: Complexity Estimation syntax support - Update version 2
			if(m_volmd.bQuarterpel) {
				m_pbitstrmOut -> putBits (codedDCECS (m_vopmd.iQuarterpel, 8), 8 , "dcecs_quarterpel");
				m_statsVOP.nBitsHead += 8;
			}
		}
		// END: Complexity Estimation syntax support - Update version 2
	}
	// END: Complexity Estimation syntax support

	// Modified for error resilient mode by Toshiba(1998-1-16)
	if(m_volmd.bShapeOnly==TRUE) {
		VideoPacketResetVOP();
		return;
	}

	// End Toshiba(1998-1-16)

	m_pbitstrmOut->putBits (m_vopmd.iIntraDcSwitchThr, 3, "IntraDCSwitchThr");
	m_statsVOP.nBitsHead+=3;

// INTERLACE_
	//m_pbitstrmOut->putBits (m_vopmd.bInterlace, 1, "InterlaceEnable");
	//m_statsVOP.nBitsHead++; 
	if (m_vopmd.bInterlace == TRUE) {
		m_pbitstrmOut->putBits (m_vopmd.bTopFieldFirst, 1, "Top_Field_First");
		m_pbitstrmOut->putBits (m_vopmd.bAlternateScan, 1, "Alternate_Scan");
		m_statsVOP.nBitsHead += 2; 
	} else
        m_vopmd.bAlternateScan = FALSE;

// INTERLACE	

// GMC
	if ((m_uiSprite == 2) && (m_vopmd.vopPredType == SPRITE) && (m_iNumOfPnts>0)) {
		CSiteD* rgstDstQ= new CSiteD [m_iNumOfPnts];
		Int i;
		for(i=0;i<m_iNumOfPnts;i++)
		{
			rgstDstQ[i].x = m_rgstDstQ[i].x;
			rgstDstQ[i].y = m_rgstDstQ[i].y;
		}
		quantizeSptTrajectory (rgstDstQ, m_rctCurrVOPY);
		m_statsVOP.nBitsHead += codeWarpPoints ();
		delete rgstDstQ; rgstDstQ = NULL;
	}
// ~GMC

	if (m_vopmd.vopPredType == IVOP) {
		m_pbitstrmOut -> putBits (m_vopmd.intStepI, m_volmd.uiQuantPrecision, "VOP_QUANT");
		m_statsVOP.nBitsHead += m_volmd.uiQuantPrecision;
		if(m_volmd.fAUsage == EIGHT_BIT) {
      for(Int iAuxComp=0; iAuxComp<m_volmd.iAuxCompCount; iAuxComp++ ) { // MAC (SB) 2-Dec-99
        m_pbitstrmOut -> putBits (m_vopmd.intStepIAlpha[0], NUMBITS_VOP_ALPHA_QUANTIZER, "VOP_GREY_QUANT");
        m_statsVOP.nBitsHead += NUMBITS_VOP_ALPHA_QUANTIZER;
      }
		}
	}
	else if (m_vopmd.vopPredType == PVOP || ((m_uiSprite == 2) && (m_vopmd.vopPredType == SPRITE))) { // GMC
		m_pbitstrmOut -> putBits (m_vopmd.intStep, m_volmd.uiQuantPrecision, "VOP_QUANT");
		m_statsVOP.nBitsHead += m_volmd.uiQuantPrecision;
		if(m_volmd.fAUsage == EIGHT_BIT) {
      for(Int iAuxComp=0; iAuxComp<m_volmd.iAuxCompCount; iAuxComp++ ) { // MAC (SB) 2-Dec-99
        m_pbitstrmOut -> putBits (m_vopmd.intStepPAlpha[0], NUMBITS_VOP_ALPHA_QUANTIZER, "VOP_GREY_QUANT");
        m_statsVOP.nBitsHead += NUMBITS_VOP_ALPHA_QUANTIZER;
      }
		}
        m_pbitstrmOut -> putBits (m_vopmd.mvInfoForward.uiFCode, NUMBITS_VOP_FCODE, "VOP_Fcode_Forward");
        m_statsVOP.nBitsHead += NUMBITS_VOP_FCODE;
	}
	else if (m_vopmd.vopPredType == BVOP) {
		m_pbitstrmOut -> putBits (m_vopmd.intStepB, m_volmd.uiQuantPrecision, "VOP_QUANT");
		m_statsVOP.nBitsHead += m_volmd.uiQuantPrecision;
		if(m_volmd.fAUsage == EIGHT_BIT) {
      for(Int iAuxComp=0; iAuxComp<m_volmd.iAuxCompCount; iAuxComp++ ) { // MAC (SB) 2-Dec-99
			  m_pbitstrmOut -> putBits (m_vopmd.intStepBAlpha[0], NUMBITS_VOP_ALPHA_QUANTIZER, "VOP_GREY_QUANT");
			  m_statsVOP.nBitsHead += NUMBITS_VOP_ALPHA_QUANTIZER;
      }
		}
		m_pbitstrmOut -> putBits (m_vopmd.mvInfoForward.uiFCode, NUMBITS_VOP_FCODE, "VOP_Fcode_Forward");
		m_statsVOP.nBitsHead += NUMBITS_VOP_FCODE;
		m_pbitstrmOut -> putBits (m_vopmd.mvInfoBackward.uiFCode, NUMBITS_VOP_FCODE, "VOP_Fcode_Backward");
		m_statsVOP.nBitsHead += NUMBITS_VOP_FCODE;
	}
	// Added for error resilient mode by Toshiba(1997-11-14)
	if(m_volmd.volType != ENHN_LAYER){
		if (m_volmd.fAUsage != RECTANGLE && m_vopmd.vopPredType != IVOP
				&& m_uiSprite != 1)  
		{  
				m_pbitstrmOut -> putBits (m_vopmd.bShapeCodingType, 1, "VOP_shape_coding_type");
				m_statsVOP.nBitsHead++;
		}
		VideoPacketResetVOP();
	}
	else
	{
///// 97/12/22 start
//OBSS_SAIT_991015		//for OBSS partial enhancement mode
//OBSSFIX_MODE3
		if ( m_volmd.iEnhnType != 0 || (m_volmd.uiVerID != 1 && m_volmd.iHierarchyType==0 && m_volmd.iEnhnType != 0) ) {
			if (m_volmd.uiVerID == 2 && m_volmd.iHierarchyType==0 && m_volmd.iEnhnType != 0)
				m_vopmd.iLoadBakShape = 0;
//		if ( m_volmd.iEnhnType != 0 || (m_volmd.uiVerID != 1 && m_volmd.iHierarchyType==0 && m_volmd.iEnhnTypeSpatial == 1 && m_volmd.iuseRefShape == 0) ) {
//			if (m_volmd.uiVerID == 2 && m_volmd.iHierarchyType==0 && m_volmd.iEnhnTypeSpatial == 1 && m_volmd.iuseRefShape == 0)
//				m_vopmd.iLoadBakShape = 0;
//~OBSSFIX_MODE3
//~OBSS_SAIT_991015
			m_pbitstrmOut->putBits (m_vopmd.iLoadBakShape, 1, "load_backward_shape");
			m_statsVOP.nBitsHead ++;
			if(m_vopmd.iLoadBakShape){
				delete rgpbfShape [1]->m_pvopcRefQ1;	
				// previous backward shape is saved to current forward shape
				rgpbfShape [1]->m_pvopcRefQ1 = new CVOPU8YUVBA (*(rgpbfShape [0]->pvopcReconCurr()));
				rgpbfShape [1]->m_rctCurrVOPY.left   = rgpbfShape [0]->m_rctCurrVOPY.left;
				rgpbfShape [1]->m_rctCurrVOPY.right  = rgpbfShape [0]->m_rctCurrVOPY.right;
				rgpbfShape [1]->m_rctCurrVOPY.top    = rgpbfShape [0]->m_rctCurrVOPY.top;
				rgpbfShape [1]->m_rctCurrVOPY.bottom = rgpbfShape [0]->m_rctCurrVOPY.bottom;

				rgpbfShape [0]->findTightBoundingBox ();
				rgpbfShape [0]->findBestBoundingBox ();
				rgpbfShape [0]->m_rctCurrVOPY  = rgpbfShape [0]->m_pvopcOrig->whereBoundY();
				rgpbfShape [0]->m_rctCurrVOPUV = rgpbfShape [0]->m_pvopcOrig->whereBoundUV ();
				rgpbfShape [0]->setRefStartingPointers ();
				rgpbfShape [0]->compute_bfShapeMembers ();

				// backward shape width/height, hori/vert reference
				m_pbitstrmOut->putBits (rgpbfShape [0]->m_rctCurrVOPY.width, NUMBITS_VOP_WIDTH, "back_Width");
				
				m_pbitstrmOut->putBits ((Int) 1, 1, "Marker"); // marker bit
				m_statsVOP.nBitsStuffing ++;
				
				m_pbitstrmOut->putBits (rgpbfShape [0]->m_rctCurrVOPY.height (), NUMBITS_VOP_HEIGHT, "back_Height");
				
				m_pbitstrmOut->putBits ((Int) 1, 1, "Marker"); // marker bit
				m_statsVOP.nBitsStuffing ++;

				Int iSign = (rgpbfShape [0]->m_rctCurrVOPY.left < 0) ? 1 : 0;
				m_pbitstrmOut->putBits (iSign, 1, "back_Hori_Ref_Sgn");
				m_pbitstrmOut->putBits (abs (rgpbfShape [0]->m_rctCurrVOPY.left), NUMBITS_VOP_HORIZONTAL_SPA_REF - 1, "back_Hori_Ref");
				
				m_pbitstrmOut->putBits ((Int) 1, 1, "Marker"); // marker bit
				m_statsVOP.nBitsStuffing++;

				iSign = (rgpbfShape [0]->m_rctCurrVOPY.top < 0) ? 1 : 0;
				m_pbitstrmOut->putBits (iSign, 1, "back_Vert_Ref_Sgn");
				m_pbitstrmOut->putBits (abs (rgpbfShape [0]->m_rctCurrVOPY.top), NUMBITS_VOP_VERTICAL_SPA_REF - 1, "back_Vert_Ref");
				m_statsVOP.nBitsHead += NUMBITS_VOP_WIDTH + NUMBITS_VOP_HEIGHT 
					+ NUMBITS_VOP_HORIZONTAL_SPA_REF + NUMBITS_VOP_VERTICAL_SPA_REF;
				
				rgpbfShape [0]->resetBYPlane ();

				printf("================ backward shape coding\n");
				rgpbfShape [0]->m_volmd.bShapeOnly = TRUE;
				rgpbfShape [0]->encodeNSForIVOP_WithShape ();

				// backward shape coding

				m_pbitstrmOut->putBits (m_vopmd.iLoadForShape, 1, "load_forward_shape");
				m_statsVOP.nBitsHead++;
				if(m_vopmd.iLoadForShape){
					rgpbfShape [1]->findTightBoundingBox ();
					rgpbfShape [1]->findBestBoundingBox ();
					rgpbfShape [1]->m_rctCurrVOPY  = rgpbfShape [1]->m_pvopcOrig->whereBoundY ();
					rgpbfShape [1]->m_rctCurrVOPUV = rgpbfShape [1]->m_pvopcOrig->whereBoundUV ();
					rgpbfShape [1]->setRefStartingPointers ();
					rgpbfShape [1]->compute_bfShapeMembers ();

					// forward shape width/height, hori/vert reference
					m_pbitstrmOut->putBits (rgpbfShape [1]->m_rctCurrVOPY.width, NUMBITS_VOP_WIDTH, "for_Width"); 
				
					m_pbitstrmOut->putBits ((Int) 1, 1, "Marker"); // marker bit
					m_statsVOP.nBitsStuffing ++;
					
					m_pbitstrmOut->putBits (rgpbfShape [1]->m_rctCurrVOPY.height (), NUMBITS_VOP_HEIGHT, "for_Height");
					
					m_pbitstrmOut->putBits ((Int) 1, 1, "Marker"); // marker bit
					m_statsVOP.nBitsStuffing ++;
					
					Int iSign = (rgpbfShape [1]->m_rctCurrVOPY.left < 0) ? 1 : 0;
					m_pbitstrmOut->putBits (iSign, 1, "for_Hori_Ref_Sgn");
					m_pbitstrmOut->putBits (abs (rgpbfShape [1]->m_rctCurrVOPY.left), NUMBITS_VOP_HORIZONTAL_SPA_REF - 1, "for_Hori_Ref");
					
					m_pbitstrmOut->putBits ((Int) 1, 1, "Marker"); // marker bit
					m_statsVOP.nBitsStuffing++;

					iSign = (rgpbfShape [1]->m_rctCurrVOPY.top < 0) ? 1 : 0;
					m_pbitstrmOut->putBits (iSign, 1, "for_Vert_Ref_Sgn");
					m_pbitstrmOut->putBits (abs (rgpbfShape [1]->m_rctCurrVOPY.top), NUMBITS_VOP_VERTICAL_SPA_REF - 1, "for_Vert_Ref");
					m_statsVOP.nBitsHead += NUMBITS_VOP_WIDTH + NUMBITS_VOP_HEIGHT 
						+ NUMBITS_VOP_HORIZONTAL_SPA_REF + NUMBITS_VOP_VERTICAL_SPA_REF;

					rgpbfShape [1]->resetBYPlane ();

					printf("================ forward shape coding\n");
					rgpbfShape [1]->m_volmd.bShapeOnly = TRUE;
					rgpbfShape [1]->encodeNSForIVOP_WithShape ();
					// forward shape coding
				}
			} // end of "if (m_vopmd.iLoadBakShape)"
		} // end of "if (iEnhnType == 1)"
///// 97/12/22 end
		m_pbitstrmOut->putBits(m_vopmd.iRefSelectCode, 2, "RefSelectCode");
		m_statsVOP.nBitsHead +=2;
	}
	// End Toshiba(1997-11-14)
}

Void CVideoObjectEncoder::decideMVInfo ()
{
	assert (m_vopmd.iSearchRangeForward <= 1024);		//seems a reasonable constraint
	assert (m_vopmd.iSearchRangeBackward <= 1024);		//seems a reasonable constraint

	if(m_vopmd.iSearchRangeForward <= 16)
		m_vopmd.mvInfoForward.uiFCode = 1 + (m_volmd.bQuarterSample ? 1 : 0);
	else if(m_vopmd.iSearchRangeForward <= 32)
		m_vopmd.mvInfoForward.uiFCode = 2 + (m_volmd.bQuarterSample ? 1 : 0);
	else if(m_vopmd.iSearchRangeForward <= 64)
		m_vopmd.mvInfoForward.uiFCode = 3 + (m_volmd.bQuarterSample ? 1 : 0);
	else if(m_vopmd.iSearchRangeForward <= 128)
		m_vopmd.mvInfoForward.uiFCode = 4 + (m_volmd.bQuarterSample ? 1 : 0);
	else if(m_vopmd.iSearchRangeForward <= 256)
		m_vopmd.mvInfoForward.uiFCode = 5 + (m_volmd.bQuarterSample ? 1 : 0);
	else if(m_vopmd.iSearchRangeForward <= 512)
		m_vopmd.mvInfoForward.uiFCode = 6 + (m_volmd.bQuarterSample ? 1 : 0);
	else // 1024
		m_vopmd.mvInfoForward.uiFCode = 7 + (m_volmd.bQuarterSample ? 1 : 0);

	m_vopmd.mvInfoForward.uiRange = 1 << (m_vopmd.mvInfoForward.uiFCode + 4);
	m_vopmd.mvInfoForward.uiScaleFactor = 1 << (m_vopmd.mvInfoForward.uiFCode - 1);
	
	if ((UInt)m_vopmd.iSearchRangeForward == (m_vopmd.mvInfoForward.uiRange >> (m_volmd.bQuarterSample ? 2 : 1)))
		m_vopmd.iSearchRangeForward--; // avoid out of range half pel

	if(m_vopmd.iSearchRangeBackward <= 16)
		m_vopmd.mvInfoBackward.uiFCode = 1 + (m_volmd.bQuarterSample ? 1 : 0);
	else if(m_vopmd.iSearchRangeBackward <= 32)
		m_vopmd.mvInfoBackward.uiFCode = 2 + (m_volmd.bQuarterSample ? 1 : 0);
	else if(m_vopmd.iSearchRangeBackward <= 64)
		m_vopmd.mvInfoBackward.uiFCode = 3 + (m_volmd.bQuarterSample ? 1 : 0);
	else if(m_vopmd.iSearchRangeBackward <= 128)
		m_vopmd.mvInfoBackward.uiFCode = 4 + (m_volmd.bQuarterSample ? 1 : 0);
	else if(m_vopmd.iSearchRangeBackward <= 256)
		m_vopmd.mvInfoBackward.uiFCode = 5 + (m_volmd.bQuarterSample ? 1 : 0);
	else if(m_vopmd.iSearchRangeBackward <= 512)
		m_vopmd.mvInfoBackward.uiFCode = 6 + (m_volmd.bQuarterSample ? 1 : 0);
	else // 1024
		m_vopmd.mvInfoBackward.uiFCode = 7 + (m_volmd.bQuarterSample ? 1 : 0);

	m_vopmd.mvInfoBackward.uiRange = 1 << (m_vopmd.mvInfoBackward.uiFCode + 4);
	m_vopmd.mvInfoBackward.uiScaleFactor = 1 << (m_vopmd.mvInfoBackward.uiFCode - 1);
	
	if ((UInt)m_vopmd.iSearchRangeBackward == (m_vopmd.mvInfoBackward.uiRange >> (m_volmd.bQuarterSample ? 2 : 1)))
		m_vopmd.iSearchRangeBackward--; // avoid out of range half pel
}


// START: Complexity Estimation syntax support - Marc Mongenet (EPFL) - 19 Jun 1998
Int CVideoObjectEncoder::codedDCECS (Int ioriginal, UInt uinb_bits)
{
	ioriginal &= (1 << uinb_bits) - 1; // mask raw data
	return ioriginal != 0 ? ioriginal : 1;
}
// END: Complexity Estimation syntax support

Void CVideoObjectEncoder::updateAllOrigRefVOPs ()
{
	assert (m_vopmd.vopPredType != BVOP||(
			m_vopmd.vopPredType == BVOP && m_volmd.volType == ENHN_LAYER && m_vopmd.iRefSelectCode == 0));
	swapVOPU8Pointers (m_pvopcOrig, m_pvopcRefOrig1);
	m_pvopcRefOrig1->setBoundRct (m_rctRefVOPY1);
	m_pvopcOrig->setBoundRct (m_rctCurrVOPY);
	repeatPadYOrA ((PixelC*) m_pvopcRefOrig1->pixelsY () + m_iOffsetForPadY, m_pvopcRefOrig1);
}

Void CVideoObjectEncoder::copyCurrToRefOrig1Y ()
{
// RRV insertion
	Int iScale	= (m_vopmd.RRVmode.iOnOff == 1) ? (2) : (1);
// ~RRV 
// RRV modification
	m_pvopcRefOrig0->setAndExpandBoundRctOnly (m_pvopcOrig->whereBoundY (), (EXPANDY_REFVOP *iScale)); // will adjust the starting pixel pointer too
//	m_pvopcRefOrig0->setAndExpandBoundRctOnly (m_pvopcOrig->whereBoundY (), EXPANDY_REFVOP); // will adjust the starting pixel pointer too
// ~RRV
	const PixelC* ppxlcOrigY = m_pvopcOrig->pixelsBoundY ();
	PixelC* ppxlcRefOrig0Y = (PixelC*) m_pvopcRefOrig0->pixelsBoundY ();
	for (CoordI y = 0; y < m_pvopcOrig->whereBoundY ().height (); y++) {
		memcpy (ppxlcRefOrig0Y, ppxlcOrigY, m_pvopcOrig->whereBoundY ().width*sizeof(PixelC)); // NBIT: add sizeof(PixelC)
		ppxlcOrigY += m_pvopcOrig->whereBoundY ().width;
		ppxlcRefOrig0Y += m_pvopcRefOrig0->whereBoundY ().width;
	}
}

Void CVideoObjectEncoder::findTightBoundingBox ()
{
	const CRct& rctOrig = m_pvopcOrig->whereY ();
	CoordI left = rctOrig.right - 1;
	CoordI top = rctOrig.bottom - 1;
	CoordI right = rctOrig.left;
	CoordI bottom = rctOrig.top;
	const PixelC* ppxlcBY = m_pvopcOrig->pixelsBY ();
	for (CoordI y = rctOrig.top; y < rctOrig.bottom; y++) {
		for (CoordI x = rctOrig.left; x < rctOrig.right; x++) {
			if (*ppxlcBY != transpValue) {
				left = min (left, x);
				top = min (top, y);
				right = max (right, x);
				bottom = max (bottom, y);
			}								
			ppxlcBY++;
		}
	}
	right++;
	bottom++;
	if (left % 2 != 0)
		left--;
	if (top % 2 != 0)
		top--;

//OBSS_SAIT_991015		
	//for OBSS partial enhancement mode
	//This part is set vertually. In the real applications, the ROI(Region Of Interest) position will be received for each time frame.
	if(m_volmd.volType ==ENHN_LAYER && (m_volmd.bSpatialScalability && m_volmd.iHierarchyType==0) && m_volmd.fAUsage==ONE_BIT && m_volmd.iEnhnTypeSpatial && m_volmd.iuseRefShape==0) {
		left =110 ; 
		top = 42 ;
		right = 240 ;
		bottom = 176;
	}

	// if Enhancement Layer, the starting point must be 4X : 01/08 - start
//OBSSFIX_MODE3
	if(m_volmd.volType == ENHN_LAYER && m_volmd.bSpatialScalability && m_volmd.iHierarchyType == 0 && !(m_volmd.iEnhnType != 0 && m_volmd.iuseRefShape == 1)) {
//	if(m_volmd.volType == ENHN_LAYER) {
//~OBSSFIX_MODE3
		if(left % 4 != 0) {
			left -= 4 - (left % 4);
		}
		if(top % 4 != 0) {
			top -= 4 - (top % 4);
		}
	}
	// if Enhancement Layer, the starting point must be 4X : 01/08 - end
//OBSSFIX_MODE3
	if((m_volmd.bSpatialScalability && m_volmd.iHierarchyType==0) && !(m_volmd.iEnhnType != 0 && m_volmd.iuseRefShape == 1)) {
//	if((m_volmd.bSpatialScalability && m_volmd.iHierarchyType==0)) {
//~OBSSFIX_MODE3
		Int dummy;
		dummy = (right-left)%MB_SIZE;
		if(dummy != 0) {
			dummy = MB_SIZE - dummy;
			right = right+dummy;
		}
		dummy = (bottom-top)%MB_SIZE;
		if(dummy != 0) {
			dummy = MB_SIZE - dummy;
			bottom = bottom+dummy;
		}
	}
//~OBSS_SAIT_991015
	m_pvopcOrig->setBoundRct (CRct (left, top, right, bottom));
}

Void CVideoObjectEncoder::findBestBoundingBox ()
{
	CoordI iLeftCurr   = m_pvopcOrig->whereBoundY ().left;
	CoordI iTopCurr    = m_pvopcOrig->whereBoundY ().top;
		// Added for Chroma Subsampling by Hyundai(1998-5-9)
        if (m_vopmd.bInterlace) iTopCurr -= iTopCurr%4;
		// End of Hyundai(1998-5-9)
	CoordI iRightCurr  = m_pvopcOrig->whereBoundY ().right;
	CoordI iBottomCurr = m_pvopcOrig->whereBoundY ().bottom;
	CoordI iLeftMost   = max (iLeftCurr - MB_SIZE + 2, 0);
		// Added for Chroma Subsampling by Hyundai(1998-5-9)
        Int iTopSkip = (m_vopmd.bInterlace) ? 4 : 2;
	CoordI iTopMost = max (iTopCurr - MB_SIZE + iTopSkip, 0);
		// End of Hyundai(1998-5-9)
	// CoordI iTopMost = max (iTopCurr  - MB_SIZE + 2, 0);	// delete by Hyundai		//can't be negative

	Int nNonTransparentMBMin = m_pvopcOrig->whereBoundY ().area ();		//abitrary large number
	Int iLeftOfBBox, iTopOfBBox;										//control postions
	Int iLeftBest = iLeftMost;
	Int iTopBest  = iTopMost;
	for (iLeftOfBBox = iLeftMost; iLeftOfBBox <= iLeftCurr; iLeftOfBBox += 2)	{
		// for (iTopOfBBox = iTopMost; iTopOfBBox <= iTopCurr; iTopOfBBox += 2)	{ // delete by Hyundai
			// Added for Chroma Subsampling by Hyundai(1998-5-9)
		for (iTopOfBBox = iTopMost; iTopOfBBox <= iTopCurr; iTopOfBBox += iTopSkip)	{
			// End of Hyundai(1998-5-9)
			// counting for each control position
			const PixelC* ppxlcBYMBRow = m_pvopcOrig->getPlane(BY_PLANE)->pixels (iLeftOfBBox, iTopOfBBox);
			Int nNonTransparentMB = 0;
			Bool bKeepCounting = TRUE;
			Int iLeftOfMB, iTopOfMB;
			for (iTopOfMB = iTopOfBBox; iTopOfMB < iBottomCurr; iTopOfMB += MB_SIZE) {
				const PixelC* ppxlcBYMB = ppxlcBYMBRow;
				for (iLeftOfMB = iLeftOfBBox; iLeftOfMB < iRightCurr; iLeftOfMB += MB_SIZE) {
					const PixelC* ppxlcBY = ppxlcBYMB;
					UInt nOpaquePixel = 0;
					for (UInt iY = 0; iY < MB_SIZE; iY++)	{
						for (UInt iX = 0; iX < MB_SIZE; iX++)	{
							nOpaquePixel += *ppxlcBY++;
						}
						ppxlcBY += m_iFrameWidthY - MB_SIZE;
					}
					if (nOpaquePixel != 0)
						nNonTransparentMB++;
					if (nNonTransparentMB > nNonTransparentMBMin)	{
						bKeepCounting = FALSE;
						break;
					}
					ppxlcBYMB += MB_SIZE;
				}
				if (!bKeepCounting)
					break;
				ppxlcBYMBRow += m_iFrameWidthYxMBSize;
			}
			if (nNonTransparentMB <= nNonTransparentMBMin)	{
				nNonTransparentMBMin = nNonTransparentMB;
				iLeftBest = iLeftOfBBox;
				iTopBest = iTopOfBBox;
			}
		}
	}
	m_pvopcOrig->setBoundRct (CRct (
		iLeftBest, 
		iTopBest, 
		iLeftBest + ((iRightCurr - iLeftBest - 1) / MB_SIZE + 1) * MB_SIZE, 
		iTopBest  + ((iBottomCurr - iTopBest - 1) / MB_SIZE + 1) * MB_SIZE
	));
}

Void CVideoObjectEncoder::computeSNRs (const CVOPU8YUVBA* pvopcCurrQ)
{
	if (m_volmd.fAUsage == RECTANGLE) {
		SNRYorA (m_pvopcOrig->pixelsBoundY (), pvopcCurrQ->pixelsY () + m_iStartInRefToCurrRctY, m_rgdSNR [0]);
		SNRUV (pvopcCurrQ);
	}
	else {
		SNRYorAWithShape (m_pvopcOrig->pixelsBoundY (), pvopcCurrQ->pixelsY () + m_iStartInRefToCurrRctY, m_rgdSNR [0]);
		SNRUVWithShape (pvopcCurrQ);
    if (m_volmd.fAUsage == EIGHT_BIT) {
      for(Int iAuxComp=0;iAuxComp<m_volmd.iAuxCompCount;iAuxComp++) { // MAC (SB) 26-Nov-99
			  SNRYorAWithShape (m_pvopcOrig->pixelsBoundA (iAuxComp), pvopcCurrQ->pixelsA (iAuxComp) + m_iStartInRefToCurrRctY, m_rgdSNR [3+iAuxComp]);
      }
    }
	}
}

Void CVideoObjectEncoder::SNRYorA (
	const PixelC* ppxlcOrig, const PixelC* ppxlcRef1, // these two should point to the left-top of the rctOrig's bounding box 
	Double& dSNR
)
{
	CoordI x, y;
	Int iSqrDiff = 0;
	Double dMaxVal = (Double) ((1<<m_volmd.nBits)-1); // NBIT
	for (y = 0; y < m_pvopcOrig->whereBoundY ().height (); y++) {
		for (x = 0; x < m_pvopcOrig->whereBoundY ().width; x++) {
			Int iDiff = ppxlcOrig [x] - ppxlcRef1 [x];
			iSqrDiff += iDiff * iDiff;
		}
		ppxlcOrig += m_pvopcOrig->whereY ().width;
		ppxlcRef1 += m_pvopcRefQ1->whereY ().width;
	}
	if (iSqrDiff == 0) 
		dSNR = 1000000.0;
	else
/* NBIT: change 255.0 to dMaxVal
		dSNR = (log10 ((255.0 * 255.0 * m_pvopcOrig->whereBoundY ().area ()) / (Double) iSqrDiff) * 10.0);
*/
		dSNR = (log10 ((dMaxVal * dMaxVal * m_pvopcOrig->whereBoundY ().area ()) / (Double) iSqrDiff) * 10.0);
}

Void CVideoObjectEncoder::SNRUV (const CVOPU8YUVBA* pvopcCurrQ)
{
	CoordI x, y;
	Int iSqrDiffU = 0, iSqrDiffV = 0, iDiff;
	Double dMaxVal = (Double) ((1<<m_volmd.nBits)-1); // NBIT
	const PixelC* ppxlcOrigU = m_pvopcOrig->pixelsBoundU (); 
	const PixelC* ppxlcOrigV = m_pvopcOrig->pixelsBoundV (); 
	const PixelC* ppxlcRef1U = pvopcCurrQ->pixelsU () + m_iStartInRefToCurrRctUV;
	const PixelC* ppxlcRef1V = pvopcCurrQ->pixelsV () + m_iStartInRefToCurrRctUV;
	for (y = 0; y < m_pvopcOrig->whereBoundUV ().height (); y++) {
		for (x = 0; x < m_pvopcOrig->whereBoundUV ().width; x++) {
			iDiff = ppxlcOrigU [x] - ppxlcRef1U [x];
			iSqrDiffU += iDiff * iDiff;
			iDiff = ppxlcOrigV [x] - ppxlcRef1V [x];
			iSqrDiffV += iDiff * iDiff;
		}
		ppxlcOrigU += m_pvopcOrig->whereUV ().width;
		ppxlcRef1U += m_pvopcRefQ1->whereUV ().width;
		
		ppxlcOrigV += m_pvopcOrig->whereUV ().width;
		ppxlcRef1V += m_pvopcRefQ1->whereUV ().width;
	}
	if (iSqrDiffU == 0) 
		m_rgdSNR [1] = 1000000.0;
	else
/* NBIT: change 255.0 to dMaxVal
		m_rgdSNR [1] = (log10 ((Double) (255.0 * 255.0 * m_pvopcOrig->whereBoundUV ().area ()) / (Double) iSqrDiffU) * 10.0);
*/
		m_rgdSNR [1] = (log10 ((Double) (dMaxVal * dMaxVal * m_pvopcOrig->whereBoundUV ().area ()) / (Double) iSqrDiffU) * 10.0);

	if (iSqrDiffV == 0) 
		m_rgdSNR [2] = 1000000.0;
	else
/* NBIT: change 255.0 to dMaxVal
		m_rgdSNR [2] = (log10 ((Double) (255.0 * 255.0 * m_pvopcOrig->whereBoundUV ().area ()) / (Double) iSqrDiffV) * 10.0);
*/
		m_rgdSNR [2] = (log10 ((Double) (dMaxVal * dMaxVal * m_pvopcOrig->whereBoundUV ().area ()) / (Double) iSqrDiffV) * 10.0);
}

Void CVideoObjectEncoder::SNRYorAWithShape (
	const PixelC* ppxlcOrig, const PixelC* ppxlcRef1, // these two should point to the left-top of the rctOrig's bounding box 
	Double& dSNR
)
{
	CoordI x, y;
	Int iSqrDiff = 0, iNumNonTransp = 0;
	Double dMaxVal = (Double) ((1<<m_volmd.nBits)-1); // NBIT
	const PixelC* ppxlcOrigBY = m_pvopcOrig->pixelsBoundBY ();
	for (y = 0; y < m_pvopcOrig->whereBoundY ().height (); y++) {
		for (x = 0; x < m_pvopcOrig->whereBoundY ().width; x++) {
			if (ppxlcOrigBY [x] != transpValue) {
				Int iDiff = ppxlcOrig [x] - ppxlcRef1 [x];
				iSqrDiff += iDiff * iDiff;
				iNumNonTransp++;
			}
		}
		ppxlcOrig += m_pvopcOrig->whereY ().width;
		ppxlcOrigBY += m_pvopcOrig->whereY ().width;
		ppxlcRef1 += m_pvopcRefQ1->whereY ().width;
	}
	if (iSqrDiff == 0) 
		dSNR = 1000000.0;
	else
/* NBIT: change 255.0 to dMaxVal
		dSNR = (log10 ((Double) (255.0 * 255.0 * iNumNonTransp) / (Double) iSqrDiff) * 10.0);
*/
		dSNR = (log10 ((Double) (dMaxVal * dMaxVal * iNumNonTransp) / (Double) iSqrDiff) * 10.0);
}

Void CVideoObjectEncoder::SNRUVWithShape (const CVOPU8YUVBA* pvopcCurrQ)
{
	CoordI x, y;
	Int iSqrDiffU = 0, iSqrDiffV = 0, iNumNonTransp = 0, iDiff;
	Double dMaxVal = (Double) ((1<<m_volmd.nBits)-1); // NBIT
	const PixelC* ppxlcOrigU = m_pvopcOrig->pixelsBoundU (); 
	const PixelC* ppxlcOrigV = m_pvopcOrig->pixelsBoundV (); 
	const PixelC* ppxlcRef1U = pvopcCurrQ->pixelsU () + m_iStartInRefToCurrRctUV;
	const PixelC* ppxlcRef1V = pvopcCurrQ->pixelsV () + m_iStartInRefToCurrRctUV;
	const PixelC* ppxlcOrigBUV = m_pvopcOrig->pixelsBoundBUV ();
	for (y = 0; y < m_pvopcOrig->whereBoundUV ().height (); y++) {
		for (x = 0; x < m_pvopcOrig->whereBoundUV ().width; x++) {
			if (ppxlcOrigBUV [x] != transpValue) {
				iDiff = ppxlcOrigU [x] - ppxlcRef1U [x];
				iSqrDiffU += iDiff * iDiff;
				iDiff = ppxlcOrigV [x] - ppxlcRef1V [x];
				iSqrDiffV += iDiff * iDiff;
				iNumNonTransp++;
			}
		}
		ppxlcOrigBUV += m_pvopcOrig->whereUV ().width;
		ppxlcOrigU += m_pvopcOrig->whereUV ().width;
		ppxlcRef1U += m_pvopcRefQ1->whereUV ().width;
		
		ppxlcOrigV += m_pvopcOrig->whereUV ().width;
		ppxlcRef1V += m_pvopcRefQ1->whereUV ().width;
	}
	if (iSqrDiffU == 0) 
		m_rgdSNR [1] = 1000000.0;
	else
/* NBIT: change 255.0 to dMaxVal
		m_rgdSNR [1] = (log10 ((Double) (255.0 * 255.0 * iNumNonTransp) / (Double) iSqrDiffU) * 10.0);
*/
		m_rgdSNR [1] = (log10 ((Double) (dMaxVal * dMaxVal * iNumNonTransp) / (Double) iSqrDiffU) * 10.0);

	if (iSqrDiffV == 0) 
		m_rgdSNR [2] = 1000000.0;
	else
/* NBIT: change 255.0 to dMaxVal
		m_rgdSNR [2] = (log10 ((Double) (255.0 * 255.0 * iNumNonTransp) / (Double) iSqrDiffV) * 10.0);
*/
		m_rgdSNR [2] = (log10 ((Double) (dMaxVal * dMaxVal * iNumNonTransp) / (Double) iSqrDiffV) * 10.0);
}

Void CVideoObjectEncoder::biInterpolateY (
	const CVOPU8YUVBA* pvopcRefQ, const CRct& rctRefVOP,
	CU8Image* puciRefQZoom, const CRct& rctRefVOPZoom, Int iRoundingControl
)
{
	const PixelC* ppxliOrg = pvopcRefQ->pixelsBoundY ();
	const PixelC* ppxliOrgBot;
	PixelC* ppxliZoom = (PixelC*) puciRefQZoom->pixels (rctRefVOPZoom.left, rctRefVOPZoom.top);
	PixelC* ppxliZoomBot;
	ppxliOrgBot = ppxliOrg + m_iFrameWidthY;
	ppxliZoomBot = ppxliZoom + m_iFrameWidthZoomY;
	Int iHeightPrevMinus1 = rctRefVOP.height () - 1;
	Int iWidthPrevMinus1 = rctRefVOP.width - 1;
	CoordI x, y;
	for (y = 0; y < iHeightPrevMinus1; y++) {
		const PixelC* ppxliOrgRow = ppxliOrg;
		const PixelC* ppxliOrgBotRow = ppxliOrgBot;
		PixelC* ppxliZoomRow = ppxliZoom;
		PixelC* ppxliZoomBotRow = ppxliZoomBot;
		for (x = 0; x < iWidthPrevMinus1; x++, ppxliZoomRow += 2, ppxliZoomBotRow += 2) {
			*ppxliZoomRow = *ppxliOrgRow;
			*(ppxliZoomRow + 1) = (*ppxliOrgRow + *(ppxliOrgRow + 1) + 1 - iRoundingControl) >> 1;
			*ppxliZoomBotRow = (*ppxliOrgRow + *ppxliOrgBotRow + 1 - iRoundingControl) >> 1;
			*(ppxliZoomBotRow + 1) = 
				(*ppxliOrgRow + *ppxliOrgBotRow + 
				*(ppxliOrgRow + 1) + *(ppxliOrgBotRow + 1) + 2 - iRoundingControl
			) >> 2;
			ppxliOrgRow++;
			ppxliOrgBotRow++;

		}
		*ppxliZoomRow = *(ppxliZoomRow + 1) = *ppxliOrgRow;
		*ppxliZoomBotRow = *(ppxliZoomBotRow + 1) = (*ppxliOrgRow++ + *ppxliOrgBotRow++ + 1 - iRoundingControl) / 2; // the last pixels of every row do not need average
		
		ppxliOrg += m_iFrameWidthY;
		ppxliOrgBot += m_iFrameWidthY;
		ppxliZoom += m_iFrameWidthZoomY * 2;
		ppxliZoomBot += m_iFrameWidthZoomY * 2;
	}
	PixelC* ppxliZoomLastLnToSec = ppxliZoom;
	for (x = 0; x < iWidthPrevMinus1; x++, ppxliZoomLastLnToSec += 2) {
		*ppxliZoomLastLnToSec = *ppxliOrg;
		*(ppxliZoomLastLnToSec + 1) = (*ppxliOrg + *(ppxliOrg + 1) + 1 - iRoundingControl) >> 1;
		ppxliOrg++;
	}
	*ppxliZoomLastLnToSec = *(ppxliZoomLastLnToSec + 1) = *ppxliOrg;
	// NBIT: add sizeof(PixelC)
	memcpy (ppxliZoomBot, ppxliZoom, rctRefVOPZoom.width*sizeof(PixelC));
}

Void CVideoObjectEncoder::swapSpatialScalabilityBVOP () //for Rectangular shape only
{
	if (m_volmd.volType == BASE_LAYER ||
	    !(m_volmd.volType == ENHN_LAYER && m_vopmd.vopPredType == BVOP && m_vopmd.iRefSelectCode == 0))
		return;
	swapVOPU8Pointers(m_pvopcCurrQ,m_pvopcRefQ1);

//OBSSFIX_MODE3
	if(m_pvopcCurrQ->fAUsage() == RECTANGLE && m_pvopcCurrQ->fAUsage()!= m_pvopcRefQ1->fAUsage()){
		delete m_pvopcCurrQ;
		m_pvopcCurrQ = new CVOPU8YUVBA(m_volmd.fAUsage, m_rctRefFrameY, m_volmd.iAuxCompCount);
	}
//~OBSSFIX_MODE3

	if(!m_volmd.bShapeOnly){//OBSS_SAIT_991015	
		repeatPadYOrA ((PixelC*) m_pvopcRefQ1->pixelsY () + m_iOffsetForPadY, m_pvopcRefQ1);
		repeatPadUV (m_pvopcRefQ1);		
	}//OBSS_SAIT_991015
}

// begin: added by Sharp (98/2/10)
Void CVideoObjectEncoder::setPredType(VOPpredType vopPredType)
{
	m_vopmd.vopPredType = vopPredType;
}

Void CVideoObjectEncoder::setRefSelectCode(Int refSelectCode)
{
	m_vopmd.iRefSelectCode = refSelectCode;
}

/******************************************
***	class CEnhcBufferEncoder	***
******************************************/
CEnhcBufferEncoder::CEnhcBufferEncoder (Int iSessionWidth, Int iSessionHeight)
:	CEnhcBuffer (iSessionWidth, iSessionHeight)
{
}

Void CEnhcBufferEncoder::copyBuf (const CEnhcBufferEncoder& srcBuf)
{
	m_iNumMBRef = srcBuf.m_iNumMBRef;
	m_iNumMBXRef = srcBuf.m_iNumMBXRef;
	m_iNumMBYRef = srcBuf.m_iNumMBYRef;
	m_iOffsetForPadY = srcBuf.m_iOffsetForPadY;
	m_iOffsetForPadUV = srcBuf.m_iOffsetForPadUV;
	m_rctPrevNoExpandY = srcBuf.m_rctPrevNoExpandY;
	m_rctPrevNoExpandUV = srcBuf.m_rctPrevNoExpandUV;

	m_rctRefVOPY0 = srcBuf.m_rctRefVOPY0;
	m_rctRefVOPUV0 = srcBuf.m_rctRefVOPUV0;
	m_rctRefVOPY1 = srcBuf.m_rctRefVOPY1;
	m_rctRefVOPUV1 = srcBuf.m_rctRefVOPUV1;
	m_rctRefVOPZoom = srcBuf.m_rctRefVOPZoom; // 9/26 by Norio
	m_bCodedFutureRef = srcBuf.m_bCodedFutureRef; // added by Sharp (99/1/26)

	CMBMode* pmbmdRefSrc = srcBuf.m_rgmbmdRef;
	CMBMode* pmbmdRef    = m_rgmbmdRef;
	CMotionVector* pmvRefSrc = srcBuf.m_rgmvRef;
	CMotionVector* pmvRef = m_rgmvRef;

	// TPS FIX
	Int iMaxIndex = max(PVOP_MV_PER_REF_PER_MB, 2*BVOP_MV_PER_REF_PER_MB);

	for (Int iMB=0; iMB<m_iNumMBRef; iMB++){
		*pmbmdRef = *pmbmdRefSrc;
		pmbmdRef++;
		pmbmdRefSrc++;

		// TPS FIX
		for (Int iVecIndex=0; iVecIndex<iMaxIndex; iVecIndex++ ){
			*pmvRef = *pmvRefSrc;
			pmvRef++;
			pmvRefSrc++;
		}
	}

	CVOPU8YUVBA* pvop = srcBuf.m_pvopcBuf;
	delete m_pvopcBuf;
	m_pvopcBuf = NULL;
	m_pvopcBuf = new CVOPU8YUVBA( *pvop );

	m_t = srcBuf.m_t;
}

Void CEnhcBufferEncoder::getBuf(const CVideoObjectEncoder* pvopc)  // get params from Base layer
{
//	assert(pvopc->m_volmd.volType == BASE_LAYER);
	CMBMode* pmbmdRef;
	CMBMode* pmbmdRefSrc;
	CMotionVector* pmvRef;
	CMotionVector* pmvRefSrc;

	m_bCodedFutureRef = pvopc->m_bCodedFutureRef; // added by Sharp (99/1/26)
	if ( pvopc->m_vopmd.vopPredType != BVOP ){
		pmbmdRef = m_rgmbmdRef;
		pmbmdRefSrc = pvopc->m_rgmbmdRef;
		pmvRef = m_rgmvRef;
		pmvRefSrc = pvopc->m_rgmvRef;
		// This part is stored in CVideoObjectEncoder::encode()
		m_iNumMBRef = pvopc->m_iNumMBRef;
		m_iNumMBXRef = pvopc->m_iNumMBXRef;
		m_iNumMBYRef = pvopc->m_iNumMBYRef;
	} else {
		pmbmdRef = m_rgmbmdRef;
		pmbmdRefSrc = pvopc->m_rgmbmd;
		pmvRef = m_rgmvRef;
		pmvRefSrc = pvopc->m_rgmv;
		m_iNumMBRef = pvopc->m_iNumMB;
		m_iNumMBXRef = pvopc->m_iNumMBX;
		m_iNumMBYRef = pvopc->m_iNumMBY;
	}

	// TPS FIX
	Int iMaxIndex = max(PVOP_MV_PER_REF_PER_MB, 2*BVOP_MV_PER_REF_PER_MB);

	for (Int iMB=0; iMB<m_iNumMBRef; iMB++ ) {
		*pmbmdRef = *pmbmdRefSrc;
		pmbmdRef++;
		pmbmdRefSrc++;

		// TPS FIX
		for (Int iVecIndex=0; iVecIndex<iMaxIndex; iVecIndex++ ){
			*pmvRef = *pmvRefSrc;
			pmvRef++;
			pmvRefSrc++;
		}
	}

	m_t = pvopc->m_t;
	delete m_pvopcBuf;
	m_pvopcBuf = NULL;
	m_pvopcBuf = new CVOPU8YUVBA( *(pvopc->pvopcReconCurr()) );
	if ( pvopc->m_vopmd.vopPredType != BVOP ) {
		m_iOffsetForPadY = pvopc->m_iOffsetForPadY;
		m_iOffsetForPadUV = pvopc->m_iOffsetForPadUV;
		m_rctPrevNoExpandY = pvopc->m_rctPrevNoExpandY;
		m_rctPrevNoExpandUV = pvopc->m_rctPrevNoExpandUV;
		m_rctRefVOPY1 = pvopc->m_rctRefVOPY1;
		m_rctRefVOPUV1 = pvopc->m_rctRefVOPUV1;
	}
	else {
		m_iOffsetForPadY = pvopc->m_iBVOPOffsetForPadY;
		m_iOffsetForPadUV = pvopc->m_iBVOPOffsetForPadUV;
		m_rctPrevNoExpandY = pvopc->m_rctBVOPPrevNoExpandY;
		m_rctPrevNoExpandUV = pvopc->m_rctBVOPPrevNoExpandUV;
		m_rctRefVOPY1 = pvopc->m_rctBVOPRefVOPY1;
		m_rctRefVOPUV1 = pvopc->m_rctBVOPRefVOPUV1;
	}
}

Void CEnhcBufferEncoder::putBufToQ0(CVideoObjectEncoder* pvopc)  // store params to Enhancement layer pvopcRefQ0
{
	assert(pvopc->m_volmd.volType == ENHN_LAYER);

	delete pvopc->m_pvopcRefQ0;
	pvopc->m_pvopcRefQ0 = NULL;
	pvopc->m_pvopcRefQ0 = new CVOPU8YUVBA ( *m_pvopcBuf );
	pvopc->m_tPastRef = m_t;

	pvopc->m_bCodedFutureRef = m_bCodedFutureRef; // added by Sharp (99/1/26)
	if ( pvopc->m_volmd.iEnhnType != 0  && //modified by Sharp (98/3/24)
	(((pvopc->m_vopmd.iRefSelectCode == 1 || pvopc->m_vopmd.iRefSelectCode == 2) && pvopc->m_vopmd.vopPredType == PVOP) ||
	 (pvopc->m_vopmd.iRefSelectCode == 3 && pvopc->m_vopmd.vopPredType == BVOP))
	)
//	if ( pvopc->m_volmd.iEnhnType == 1 )
	{
		CRct rctRefFrameYTemp = pvopc->m_rctRefFrameY;
		CRct rctRefFrameUVTemp = pvopc->m_rctRefFrameUV;
		rctRefFrameYTemp.expand(-EXPANDY_REF_FRAME);
		rctRefFrameUVTemp.expand(-EXPANDY_REF_FRAME/2);
		pvopc->m_pvopcRefQ0->addBYPlain(rctRefFrameYTemp, rctRefFrameUVTemp);
	}
	
		CMBMode* pmbmdRefSrc = m_rgmbmdRef;
		CMBMode* pmbmdRef = pvopc->m_rgmbmdRef;
		CMotionVector* pmvRefSrc = m_rgmvRef;
		CMotionVector* pmvRef = pvopc->m_rgmvRef;

		// This part is stored in CVideoObjectEncoder::encode()
		pvopc->m_iNumMBRef = m_iNumMBRef;
		pvopc->m_iNumMBXRef = m_iNumMBXRef;
		pvopc->m_iNumMBYRef = m_iNumMBYRef;

		// TPS FIX
		Int iMaxIndex = max(PVOP_MV_PER_REF_PER_MB, 2*BVOP_MV_PER_REF_PER_MB);

		for (Int iMB=0; iMB<m_iNumMBRef; iMB++ ) {
			*pmbmdRef = *pmbmdRefSrc;

			if ( pvopc->m_volmd.iEnhnType != 0  && // modified by Sharp (98/3/24)
			(((pvopc->m_vopmd.iRefSelectCode == 1 || pvopc->m_vopmd.iRefSelectCode == 2) && pvopc->m_vopmd.vopPredType == PVOP) ||
			 (pvopc->m_vopmd.iRefSelectCode == 3 && pvopc->m_vopmd.vopPredType == BVOP))
			) {
				pmbmdRef->m_shpmd = ALL_OPAQUE;
			}
			pmbmdRef++;
			pmbmdRefSrc++;

			// TPS FIX
			for (Int iVecIndex=0; iVecIndex<iMaxIndex; iVecIndex++ ){
				*pmvRef = *pmvRefSrc;
				pmvRef ++;
				pmvRefSrc ++;
			}
		}
		pvopc->saveShapeMode ();

	// for padding operation
		pvopc->m_iOffsetForPadY = m_iOffsetForPadY;
		pvopc->m_iOffsetForPadUV = m_iOffsetForPadUV;
		pvopc->m_rctPrevNoExpandY = m_rctPrevNoExpandY;
		pvopc->m_rctPrevNoExpandUV = m_rctPrevNoExpandUV;

	// This part is needed for storing RefQ0
		pvopc->m_rctRefVOPY0 = m_rctRefVOPY1;
		pvopc->m_rctRefVOPUV0 = m_rctRefVOPUV1;

		pvopc->m_pvopcRefQ0->setBoundRct(m_rctRefVOPY1);

	pvopc->repeatPadYOrA ((PixelC*) pvopc->m_pvopcRefQ0->pixelsY () + m_iOffsetForPadY, pvopc->m_pvopcRefQ0);
	pvopc->repeatPadUV(pvopc->m_pvopcRefQ0);
	if ( pvopc->m_volmd.fAUsage != RECTANGLE ){	
    if ( pvopc->m_volmd.fAUsage == EIGHT_BIT ) {
//      for(Int iAuxComp=0;iAuxComp<m_volmd.iAuxCompCount;iAuxComp++) { // MAC (SB) 26-Nov-99
			  pvopc->repeatPadYOrA ((PixelC*) pvopc->m_pvopcRefQ0->pixelsA (0/*iAuxComp*/) + m_iOffsetForPadY, pvopc->m_pvopcRefQ0);
//      }
    }
	}
}

Void CEnhcBufferEncoder::putBufToQ1(CVideoObjectEncoder* pvopc)  // store params to Enhancement layer pvopcRefQ1
{
	assert(pvopc->m_volmd.volType == ENHN_LAYER);

	delete pvopc->m_pvopcRefQ1;
	pvopc->m_pvopcRefQ1 = NULL;
	pvopc->m_pvopcRefQ1 = new CVOPU8YUVBA ( *m_pvopcBuf );
	pvopc->m_tFutureRef = m_t;
	pvopc->m_bCodedFutureRef = m_bCodedFutureRef; // added by Sharp (99/1/26)
	if ( pvopc->m_volmd.iEnhnType != 0  && // modified by Sharp (98/3/24)
	(((pvopc->m_vopmd.iRefSelectCode == 1 || pvopc->m_vopmd.iRefSelectCode == 2) && pvopc->m_vopmd.vopPredType == PVOP) ||
	((pvopc->m_vopmd.iRefSelectCode == 1 || pvopc->m_vopmd.iRefSelectCode == 3) && pvopc->m_vopmd.vopPredType == BVOP)) // modified by Sharp (98/5/25)
	)
//	if ( pvopc->m_volmd.iEnhnType == 1 )
	{
		CRct rctRefFrameYTemp = pvopc->m_rctRefFrameY;
		CRct rctRefFrameUVTemp = pvopc->m_rctRefFrameUV;
		rctRefFrameYTemp.expand(-EXPANDY_REF_FRAME);
		rctRefFrameUVTemp.expand(-EXPANDY_REF_FRAME/2);
		pvopc->m_pvopcRefQ1->addBYPlain(rctRefFrameYTemp, rctRefFrameUVTemp);
	}

//	if ( pvopc->m_volmd.fAUsage != RECTANGLE ){	
		CMBMode* pmbmdRefSrc = m_rgmbmdRef;
		CMBMode* pmbmdRef = pvopc->m_rgmbmdRef;
		CMotionVector* pmvRefSrc = m_rgmvRef;
		CMotionVector* pmvRef = pvopc->m_rgmvRef;

		// This part is stored in CVideoObjectEncoder::encode()
		pvopc->m_iNumMBRef = m_iNumMBRef;
		pvopc->m_iNumMBXRef = m_iNumMBXRef;
		pvopc->m_iNumMBYRef = m_iNumMBYRef;

		// TPS FIX
		Int iMaxIndex = max(PVOP_MV_PER_REF_PER_MB, 2*BVOP_MV_PER_REF_PER_MB);

		for (Int iMB=0; iMB<m_iNumMBRef; iMB++ ) {
			*pmbmdRef = *pmbmdRefSrc;
			if ( pvopc->m_volmd.iEnhnType != 0 ) { // modified by Sharp (98/3/24)
				pmbmdRef->m_shpmd = ALL_OPAQUE;
			}
			pmbmdRef++;
			pmbmdRefSrc++;
			// TPS FIX
			for (Int iVecIndex=0; iVecIndex<iMaxIndex; iVecIndex++ ){
				*pmvRef = *pmvRefSrc;
				pmvRef ++;
				pmvRefSrc++;
			}
		}

	// for padding operation
		pvopc->m_iOffsetForPadY = m_iOffsetForPadY;
		pvopc->m_iOffsetForPadUV = m_iOffsetForPadUV;
		pvopc->m_rctPrevNoExpandY = m_rctPrevNoExpandY;
		pvopc->m_rctPrevNoExpandUV = m_rctPrevNoExpandUV;

	// This part is needed for storing RefQ0
		pvopc->m_rctRefVOPY1 = m_rctRefVOPY1;
		pvopc->m_rctRefVOPUV1 = m_rctRefVOPUV1;

		pvopc->m_pvopcRefQ1->setBoundRct(m_rctRefVOPY1);
//	}

	pvopc->repeatPadYOrA ((PixelC*) pvopc->m_pvopcRefQ1->pixelsY () + m_iOffsetForPadY, pvopc->m_pvopcRefQ1);
	pvopc->repeatPadUV(pvopc->m_pvopcRefQ1);
	if ( pvopc->m_volmd.fAUsage != RECTANGLE ){	
    if ( pvopc->m_volmd.fAUsage == EIGHT_BIT ) {
//		  for(Int iAuxComp=0;iAuxComp<m_volmd.iAuxCompCount;iAuxComp++) { // MAC (SB) 26-Nov-99
        pvopc->repeatPadYOrA ((PixelC*) pvopc->m_pvopcRefQ1->pixelsA (0/*iAuxComp*/) + m_iOffsetForPadY, pvopc->m_pvopcRefQ1);
//      }
    }
	}
}

Void CVideoObjectEncoder::set_LoadShape(
    Int* ieFramebShape, Int* ieFramefShape, // frame number for back/forward shape
    const Int iRate,	      // rate of base layer
    const Int ieFrame,	      // current frame number
    const Int iFirstFrame,    // first frame number of sequence
    const Int iFirstFrameLoop // first frame number of the enhancement loop
)
{
  if(m_volmd.iEnhnType==0){
      m_vopmd.iLoadBakShape = 0;
      m_vopmd.iLoadForShape = 0; // do not care
      return;
  }

  if(ieFrame == iFirstFrameLoop){
    *ieFramefShape = ((ieFrame-iFirstFrame)/iRate)*iRate + iFirstFrame;
    *ieFramebShape = *ieFramefShape + iRate;

//    if(*ieFramefShape != iFirstFrame){
      m_vopmd.iLoadBakShape = 1;
      m_vopmd.iLoadForShape = 0;
//		} else {
//      m_vopmd.iLoadBakShape = 1;
//      m_vopmd.iLoadForShape = 1;
//		}
  }
  else{
      m_vopmd.iLoadBakShape = 0;
      m_vopmd.iLoadForShape = 0; // do not care
  }
}

Void CVideoObjectEncoder::BackgroundComposition(
    const Int width, Int height,
    const Int iFrame,	  // current frame number
    const Int iPrev,	  // previous base frame 
    const Int iNext,      // next     base frame 
    const CVOPU8YUVBA* pvopcBuffP1,
    const CVOPU8YUVBA* pvopcBuffP2,
    const CVOPU8YUVBA* pvopcBuffB1,
    const CVOPU8YUVBA* pvopcBuffB2,
		const Char* pchReconYUVDir, Int iobj, const Char* pchPrefix, // for output file name // modified by Sharp (98/10/26)
		FILE *pchfYUV  // added by Sharp (98/10/26)
)
{
    Void convertYuv(const CVOPU8YUVBA* pvopcSrc, PixelC* destY, PixelC* destU, PixelC* destV, Int width, Int height);
    Void convertSeg(const CVOPU8YUVBA* pvopcSrc, PixelC* destBY,PixelC* destBUV,              Int width, Int height,
			    Int left, Int right, Int top, Int bottom
		    );
    Void bg_comp_each(PixelC* curr, PixelC* prev, PixelC* next, PixelC* mask_curr, PixelC* mask_prev, PixelC* mask_next, 
		      Int curr_t, Int prev_t, Int next_t, Int width, Int height, Int CoreMode); // modified by Sharp (98/3/24)
    Void inv_convertYuv(const CVOPU8YUVBA* pvopcSrc, PixelC* destY, PixelC* destU, PixelC* destV, Int width, Int height);
    Void write420_sep(Int num, char *name, PixelC* destY, PixelC* destU, PixelC* destV, Int width, Int height); // modified by Sharp (98/10/26)
    Void write420_jnt(FILE *pchfYUV, PixelC* destY, PixelC* destU, PixelC* destV, Int width, Int height); // added by Sharp (98/10/26)
    Void write_seg_test(Int num, char *name, PixelC* destY, Int width, Int height);

    PixelC* currY = new PixelC [width*height];
    PixelC* currU = new PixelC [width*height/4];
    PixelC* currV = new PixelC [width*height/4];
    PixelC* currBY  = new PixelC [width*height];
    PixelC* currBUV = new PixelC [width*height/4];

    PixelC* prevY = new PixelC [width*height];
    PixelC* prevU = new PixelC [width*height/4];
    PixelC* prevV = new PixelC [width*height/4];
    PixelC* prevBY  = new PixelC [width*height];
    PixelC* prevBUV = new PixelC [width*height/4];

    PixelC* nextY = new PixelC [width*height];
    PixelC* nextU = new PixelC [width*height/4];
    PixelC* nextV = new PixelC [width*height/4];
    PixelC* nextBY  = new PixelC [width*height];
    PixelC* nextBUV = new PixelC [width*height/4];

    // current frame
    convertYuv(pvopcReconCurr(), currY, currU, currV, width, height);
    if(pvopcReconCurr ()->pixelsBY () != NULL)
      convertSeg(pvopcReconCurr(), currBY, currBUV, width, height,
	       m_rctCurrVOPY.left,
	       m_rctCurrVOPY.right,
	       m_rctCurrVOPY.top,
	       m_rctCurrVOPY.bottom); /* forward shape */

    // previouse frame
    if(pvopcBuffB1 != NULL)
      convertYuv(pvopcBuffB1, prevY, prevU, prevV, width, height);
    else
      convertYuv(pvopcBuffP1, prevY, prevU, prevV, width, height);
    convertSeg(rgpbfShape [1]->pvopcReconCurr (), prevBY, prevBUV, width, height,
	       rgpbfShape [1]->m_rctCurrVOPY.left,
	       rgpbfShape [1]->m_rctCurrVOPY.right,
	       rgpbfShape [1]->m_rctCurrVOPY.top,
	       rgpbfShape [1]->m_rctCurrVOPY.bottom); /* forward shape */

 //   write_test(iPrev, "aaa", prevY, prevU, prevV, width, height);
 //   write_seg_test(iPrev, "aaa", prevBY, width, height);

    // next frame
    if(pvopcBuffB2 != NULL)
      convertYuv(pvopcBuffB2, nextY, nextU, nextV, width, height);
    else
      convertYuv(pvopcBuffP2, nextY, nextU, nextV, width, height);
    convertSeg(rgpbfShape [0]->pvopcReconCurr (), nextBY, nextBUV, width, height,
	       rgpbfShape [0]->m_rctCurrVOPY.left,
	       rgpbfShape [0]->m_rctCurrVOPY.right,
	       rgpbfShape [0]->m_rctCurrVOPY.top,
	       rgpbfShape [0]->m_rctCurrVOPY.bottom); /* backward shape */

//    write_test(iNext, "aaa", nextY, nextU, nextV, width, height);
//    write_seg_test(iNext, "aaa", nextBY, width, height);

// begin: modified by Sharp (98/3/24)
    bg_comp_each(currY, prevY, nextY, currBY,  prevBY,  nextBY,  iFrame, iPrev, iNext, width,   height, m_volmd.iEnhnType == 2);
    bg_comp_each(currU, prevU, nextU, currBUV, prevBUV, nextBUV, iFrame, iPrev, iNext, width/2, height/2, m_volmd.iEnhnType == 2);
    bg_comp_each(currV, prevV, nextV, currBUV, prevBUV, nextBUV, iFrame, iPrev, iNext, width/2, height/2, m_volmd.iEnhnType == 2);
// end: modified by Sharp (98/3/24)

//    inv_convertYuv(pvopcReconCurr(), currY, currU, currV, width, height);

#ifdef __OUT_ONE_FRAME_ // added by Sharp (98/10/26)
		// write background compsition image
		static Char pchYUV [100];
		sprintf (pchYUV, "%s/%2.2d/%s.yuv", pchReconYUVDir, iobj, pchPrefix); // modified by Sharp (98/10/26)
    write420_sep(iFrame, pchYUV, currY, currU, currV, width, height); // modified by Sharp (98/10/26)
// begin: added by Sharp (98/9/27)
#else
    write420_jnt(pchfYUV, currY, currU, currV, width, height);
#endif
// end: added by Sharp (98/9/27)

    delete currY;
    delete currU;
    delete currV;
    delete currBY;
    delete currBUV;

    delete prevY;
    delete prevU;
    delete prevV;
    delete prevBY;
    delete prevBUV;

    delete nextY;
    delete nextU;
    delete nextV;
    delete nextBY;
    delete nextBUV;
}
// end: added by Sharp (98/2/10)

//OBSS_SAIT_991015		//for OBSS partial enhancement mode
Bool CVideoObjectEncoder::BackgroundComposition(
    const Int width, Int height,
    const Int iFrame,	  // current frame number
    const CVOPU8YUVBA* pvopcBuffP1,
	const Char* pchReconYUVDir, Int iobj, const Char* pchPrefix, // for output file name // modified by Sharp (98/10/26)
	FILE *pchfYUV,  // added by Sharp (98/10/26)
	FILE *pchfSeg
)
{
    Void convertYuv(const CVOPU8YUVBA* pvopcSrc, PixelC* destY, PixelC* destU, PixelC* destV, Int width, Int height);
    Void convertSeg(const CVOPU8YUVBA* pvopcSrc, PixelC* destBY,PixelC* destBUV,              Int width, Int height,
			    Int left, Int right, Int top, Int bottom
		    );

    Void bg_comp_each(PixelC* curr, PixelC* prev, PixelC* mask_curr, PixelC* mask_prev, Int curr_t, Int width, Int height, CRct rctCurrVOPY); 
    Void inv_convertYuv(const CVOPU8YUVBA* pvopcSrc, PixelC* destY, PixelC* destU, PixelC* destV, Int width, Int height);
    Void write420_sep(Int num, char *name, PixelC* destY, PixelC* destU, PixelC* destV, Int width, Int height); // modified by Sharp (98/10/26)
    Void write420_jnt(FILE *pchfYUV, PixelC* destY, PixelC* destU, PixelC* destV, Int width, Int height); // added by Sharp (98/10/26)
//OBSSFIX_MODE3
    Void write420_jnt_withMask(FILE *pfYUV, PixelC* destY, PixelC* destU, PixelC* destV, PixelC* destBY, PixelC* destBUV, Int width, Int height); // added by Sharp (98/10/26)
//~OBSSFIX_MODE3
    Void write_seg_test(Int num, char *name, PixelC* destY, Int width, Int height);
//OBSSFIX_MODE3
    Void bg_comp_each_mode3(PixelC* curr, PixelC* prev, PixelC* mask_curr, PixelC* mask_prev, Int curr_t, Int width, Int height, CRct rctCurrVOPY); 
//~OBSSFIX_MODE3

//OBSSFIX_MODE3
	if(m_vopmd.bBGComposition == FALSE)
		return FALSE;
//	if(BGComposition == FALSE)
//		return FALSE;
//~OBSSFIX_MODE3
    PixelC* currY = new PixelC [width*height];
    PixelC* currU = new PixelC [width*height/4];
    PixelC* currV = new PixelC [width*height/4];
    PixelC* currBY  = new PixelC [width*height];
    PixelC* currBUV = new PixelC [width*height/4];

    PixelC* prevY = new PixelC [width*height];
    PixelC* prevU = new PixelC [width*height/4];
    PixelC* prevV = new PixelC [width*height/4];
    PixelC* prevBY  = new PixelC [width*height];
    PixelC* prevBUV = new PixelC [width*height/4];

//OBSSFIX_MODE3
	PixelC* currBUV_tmp = new PixelC[width*height/4];
//~OBSSFIX_MODE3

    // current frame
	if (m_vopmd.vopPredType!=BVOP){
		convertYuv(pvopcReconCurr(), currY, currU, currV, width, height);
		if(pvopcReconCurr ()->pixelsBY () != NULL)
		  convertSeg(pvopcReconCurr(), currBY, currBUV, width, height,
			   m_rctCurrVOPY.left,
			   m_rctCurrVOPY.right,
			   m_rctCurrVOPY.top,
			   m_rctCurrVOPY.bottom); /* forward shape */
	}
	else {
		convertYuv(m_pvopcCurrQ, currY, currU, currV, width, height);
		if(m_pvopcCurrQ->pixelsBY () != NULL)
		  convertSeg(m_pvopcCurrQ, currBY, currBUV, width, height,
			   m_rctCurrVOPY.left,
			   m_rctCurrVOPY.right,
			   m_rctCurrVOPY.top,
			   m_rctCurrVOPY.bottom); /* forward shape */
	}


    // previouse frame
    convertYuv(pvopcBuffP1, prevY, prevU, prevV, width, height);
//OBSSFIX_MODE3
	if(pvopcBuffP1->fAUsage() != RECTANGLE)
		convertSeg(pvopcBuffP1, prevBY, prevBUV, width, height,
			   (pvopcBuffP1->whereBoundY ()).left,
			   (pvopcBuffP1->whereBoundY ()).right,
			   (pvopcBuffP1->whereBoundY ()).top,
			   (pvopcBuffP1->whereBoundY ()).bottom); /* forward shape */
	else{
		memset(prevBY, 255, width*height);
		memset(prevBUV, 255, width*height/4);
	}
//    convertSeg(pvopcBuffP1, prevBY, prevBUV, width, height,
//	       (pvopcBuffP1->whereBoundY ()).left,
//	       (pvopcBuffP1->whereBoundY ()).right,
//	       (pvopcBuffP1->whereBoundY ()).top,
//	       (pvopcBuffP1->whereBoundY ()).bottom); /* forward shape */
//OBSSFIX_MODE3
 //   write_test(iPrev, "aaa", prevY, prevU, prevV, width, height);
 //   write_seg_test(iPrev, "aaa", prevBY, width, height);

// begin: modified by Sharp (98/3/24)
//OBSSFIX_MODE3
	memcpy(currBUV_tmp, currBUV, width*height/4);
	if (m_volmd.iEnhnType == 1 && m_volmd.iuseRefShape ==1) {
		bg_comp_each_mode3(currY, prevY, currBY,  prevBY,  iFrame, width,   height, m_rctCurrVOPY);
		bg_comp_each_mode3(currU, prevU, currBUV_tmp, prevBUV, iFrame, width/2, height/2, m_rctCurrVOPUV);
		bg_comp_each_mode3(currV, prevV, currBUV, prevBUV, iFrame, width/2, height/2, m_rctCurrVOPUV);
	}
	else {
		bg_comp_each(currY, prevY, currBY,  prevBY,  iFrame, width,   height, m_rctCurrVOPY);
		bg_comp_each(currU, prevU, currBUV_tmp, prevBUV, iFrame, width/2, height/2, m_rctCurrVOPUV);
		bg_comp_each(currV, prevV, currBUV, prevBUV, iFrame, width/2, height/2, m_rctCurrVOPUV);
	}
//	bg_comp_each(currY, prevY, currBY,  prevBY,  iFrame, width,   height, m_rctCurrVOPY);
//	bg_comp_each(currU, prevU, currBUV, prevBUV, iFrame, width/2, height/2, m_rctCurrVOPUV);
//	bg_comp_each(currV, prevV, currBUV, prevBUV, iFrame, width/2, height/2, m_rctCurrVOPUV);
//~OBSSFIX_MODE3
// end: modified by Sharp (98/3/24)

//    inv_convertYuv(pvopcReconCurr(), currY, currU, currV, width, height);

#ifdef __OUT_ONE_FRAME_ // added by Sharp (98/10/26)
		// write background compsition image
		static Char pchYUV [100];
		sprintf (pchYUV, "%s/%2.2d/%s.yuv", pchReconYUVDir, iobj, pchPrefix); // modified by Sharp (98/10/26)
    write420_sep(iFrame, pchYUV, currY, currU, currV, width, height); // modified by Sharp (98/10/26)
// begin: added by Sharp (98/9/27)
#else
//OBSSFIX_MODE3
//  write420_jnt(pchfYUV, currY, currU, currV, width, height);
	if (m_volmd.iEnhnType == 1 && m_volmd.iuseRefShape ==1) 
		write420_jnt(pchfYUV, currY, currU, currV, width, height);
	else
		write420_jnt_withMask(pchfYUV, currY, currU, currV, currBY, currBUV, width, height);
//~OBSSFIX_MODE3

    fwrite(currBY, sizeof (PixelC), width*height, pchfSeg);
#endif
// end: added by Sharp (98/9/27)

    delete currY;
    delete currU;
    delete currV;
    delete currBY;
    delete currBUV;

//OBSSFIX_MODE3
	delete currBUV_tmp;
//~OBSSFIX_MODE3

    delete prevY;
    delete prevU;
    delete prevV;
    delete prevBY;
    delete prevBUV;

	return TRUE;
}
// end: added by Sharp (98/2/10)
//~OBSS_SAIT_991015

/*************************************************************************

This software module was originally developed by 

	Ming-Chieh Lee (mingcl@microsoft.com), Microsoft Corporation
	Wei-ge Chen (wchen@microsoft.com), Microsoft Corporation
	Bruce Lin (blin@microsoft.com), Microsoft Corporation
	Chuang Gu (chuanggu@microsoft.com), Microsoft Corporation
	(date: March, 1996)
and edited by
        Wei Wu (weiwu@stallion.risc.rockwell.com) Rockwell Science Center

and also edited by
	Yoshihiro Kikuchi (TOSHIBA CORPORATION)
	Takeshi Nagai (TOSHIBA CORPORATION)
	Toshiaki Watanabe (TOSHIBA CORPORATION)
	Noboru Yamaguchi (TOSHIBA CORPORATION)

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

	sesEnc.cpp

Abstract:

	Encoder for one video session.

Revision History:
	Sept. 30, 1997: Error resilient tools added by Toshiba
	Nov.  27, 1997: Spatial Scalable tools added, and modified for Spatial Scalable
						by Takefumi Nagumo(nagumo@av.crl.sony.co.jp) SONY
	Dec 11, 1997:	Interlaced tools added by NextLevel Systems (GI)
						(B.Eifrig, beifrig@nlvl.com, X.Chen, xchen@nlvl.com)
	Jun 17, 1998:	add Complexity Estimation syntax support
					Marc Mongenet (Marc.Mongenet@epfl) - EPFL
    Feb.16, 1999:   add Quarter Sample 
                    Mathias Wien (wien@ient.rwth-aachen.de) 
    Feb.24, 1999:   GMC added by Yoshinori Suzuki (Hitachi, Ltd.) 
	May  9, 1999:	tm5 rate control by DemoGraFX, duhoff@mediaone.net (added by mwi)
	Aug.24, 1999:   NEWPRED added by Hideaki Kimata (NTT) 
	Sep.06	1999 : RRV added by Eishi Morimatsu (Fujitsu Laboratories Ltd.) 
	Nov.11  1999 : Fixed Complexity Estimation syntax support, version 2 (Massimo Ravasi, EPFL)
	Feb.12  2000 : Bugfix of OBSS by Takefumi Nagumo (Sony)

*************************************************************************/

#include "stdlib.h"
#include "stdio.h"
#include "math.h"
#include "fstream.h"
#include "ostream.h"

#include "typeapi.h"
#include "codehead.h"
#include "global.hpp"
#include "entropy/bitstrm.hpp"
#include "entropy/entropy.hpp"
#include "entropy/huffman.hpp"

#include "mode.hpp"
#include "sesenc.hpp"

#include "tps_enhcbuf.hpp" // added by Sharp (98/2/12)
#include "vopses.hpp"
#include "vopseenc.hpp"
#include "enhcbufenc.hpp" // added by Sharp (98/2/12)

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

#ifdef __PC_COMPILER_					//OS specific stuff
#define SUB_CMPFILE "%s\\%2.2d\\%s.cmp"
#define SUB_TRCFILE "%s\\%2.2d\\%s.trc"
#define SUB_YUVFILE	"%s\\%2.2d\\%s.yuv"
#define SUB_SEGFILE	"%s\\%2.2d\\%s.seg"
#define SUB_AUXFILE "%s\\%2.2d\\%s.%d.aux"
#define ENHN_SUB_CMPFILE "%s\\%2.2d\\%s_e.cmp"
#define ENHN_SUB_TRCFILE "%s\\%2.2d\\%s_e.trc"
#define ENHN_SUB_YUVFILE "%s\\%2.2d\\%s_e.yuv"
#define ENHN_SUB_SEGFILE "%s\\%2.2d\\%s_e.seg"
#define SUB_VDLFILE "%s\\%2.2d\\%s.spt"	//for sprite i/o
#define SUB_PNTFILE "%s\\%2.2d\\%s.pnt"	//for sprite i/o
#define ROOT_YUVFILE "%s\\%s.yuv"
#define ROOT_SEGFILE "%s\\%s.seg"
#define ROOT_AUXFILE "%s\\%s.%d.aux"
#define IOS_BINARY ios::binary
//#define IOS_BINARY 0
#define MKDIR "mkdir %s\\%2.2d"
#else
#define SUB_CMPFILE "%s/%2.2d/%s.cmp"
#define SUB_TRCFILE "%s/%2.2d/%s.trc"
#define SUB_YUVFILE	"%s/%2.2d/%s.yuv"
#define SUB_SEGFILE	"%s/%2.2d/%s.seg"
#define SUB_AUXFILE "%s/%2.2d/%s.%d.aux"
#define ENHN_SUB_CMPFILE "%s/%2.2d/%s_e.cmp"
#define ENHN_SUB_TRCFILE "%s/%2.2d/%s_e.trc"
#define ENHN_SUB_YUVFILE "%s/%2.2d/%s_e.yuv"
#define ENHN_SUB_SEGFILE "%s/%2.2d/%s_e.seg"
#define SUB_VDLFILE "%s/%2.2d/%s.vdl"	//for sprite i/o
#define SUB_PNTFILE "%s/%2.2d/%s.pnt"	//for sprite i/o
#define ROOT_YUVFILE "%s/%s.yuv"
#define ROOT_SEGFILE "%s/%s.seg"
#define ROOT_AUXFILE "%s/%s.%d.aux"
#define IOS_BINARY ios::binary
#define MKDIR "mkdir %s/%2.2d"
#endif

#define BASE_LAYER 0
#define ENHN_LAYER 1

#define _FOR_GSSP_

Bool fileExistence (const Char* pchFile) 
{
	Bool ret = 1;
	FILE* pf = fopen (pchFile, "r");
	if (pf == NULL)
		ret = 0;
	else
		fclose (pf);
	return ret;
}

Void CSessionEncoder::createReconDir (UInt idx) const
{
	Char pchTmp [100];
	sprintf (pchTmp, SUB_YUVFILE, m_pchReconYUVDir, idx, m_pchPrefix);
	FILE* pf = fopen (pchTmp, "wb");
	if (pf == NULL) {
		sprintf (pchTmp, MKDIR, m_pchReconYUVDir, idx);
		system (pchTmp);																	
	}
	else
		fclose (pf);
}

Void CSessionEncoder::createCmpDir (UInt idx) const
{
	Char pchTmp [100];
	sprintf (pchTmp, SUB_CMPFILE, m_pchOutStrFiles, idx, m_pchPrefix);
	FILE* pf = fopen (pchTmp, "wb");
	if (pf == NULL) {
		sprintf (pchTmp, MKDIR, m_pchOutStrFiles, idx);
		system (pchTmp);																	
	}
	else
		fclose (pf);
}

CSessionEncoder::~CSessionEncoder ()
{
	delete [] m_rgvolmd [BASE_LAYER];
	delete [] m_rgvolmd [ENHN_LAYER];
	delete [] m_rgvopmd [BASE_LAYER];
	delete [] m_rgvopmd [ENHN_LAYER];

	// sprite 
	for (Int iVO = 0; iVO < m_iNumVO; iVO++) {
		delete [] m_ppstSrc [iVO];
		for (Int ifr = 0; ifr < m_iNumFrame; ifr++)
			delete [] m_pppstDst [iVO] [ifr];
		delete [] m_pppstDst [iVO];
	}
	delete [] m_ppstSrc;
	delete [] m_pppstDst;
}


CSessionEncoder::CSessionEncoder (SessionEncoderArgList *pArgs)
{
 	m_iFirstFrame = pArgs->iFirstFrm;
	m_iLastFrame =pArgs->iLastFrm;
	m_iFirstVO = pArgs->iFirstVO;
	m_iLastVO = pArgs->iLastVO;
	m_rgbSpatialScalability = pArgs->rgbSpatialScalability;
	m_rguiRateControl = pArgs->rguiRateControl;
	m_rguiBudget = pArgs->rguiBudget;
	m_pchPrefix = pArgs->pchPrefix;
	m_pchBmpFiles = pArgs->pchBmpFiles;
	m_pchReconYUVDir = pArgs->pchOutBmpFiles;
	m_rgfChrType = pArgs->rgfChrType;
	m_pchOutStrFiles =pArgs->pchOutStrFiles;
	m_rguiSpriteUsage =pArgs->rguiSpriteUsage;
	m_rguiWarpingAccuracy = pArgs->rguiWarpingAccuracy;
	m_rgNumOfPnts = pArgs->rgNumOfPnts;
	m_pchSptDir = pArgs->pchSptDir;
	m_pchSptPntDir = pArgs->pchSptPntDir;
	m_rgiTemporalScalabilityType = pArgs->rgiTemporalScalabilityType;
	// data preparation
	m_iNumFrame = m_iLastFrame - m_iFirstFrame + 1;
	assert (m_iNumFrame > 0);
	m_rctOrg = CRct (0, 0, pArgs->uiFrmWidth, pArgs->uiFrmHeight);
	if( pArgs->uiFrameWidth_SS !=0 && pArgs->uiFrameHeight_SS!=0)
		m_rctOrgSpatialEnhn = CRct(0, 0, pArgs->uiFrameWidth_SS, pArgs->uiFrameHeight_SS);
	assert (pArgs->uiFrmWidth <= 8192 && pArgs->uiFrmHeight <= 8192); //2^13 maximum size
	m_iNumVO = m_iLastVO - m_iFirstVO + 1;
	assert (m_iNumVO > 0);
	m_rgvolmd [BASE_LAYER] = new VOLMode [m_iNumVO];
	m_rgvolmd [ENHN_LAYER] = new VOLMode [m_iNumVO];
	m_rgvopmd [BASE_LAYER] = new VOPMode [m_iNumVO];
	m_rgvopmd [ENHN_LAYER] = new VOPMode [m_iNumVO];
	Int iVO;
	for (iVO = 0; iVO < m_iNumVO; iVO++) {
		// set VOL and VOP mode values
		for (Int iLayer = BASE_LAYER; iLayer <= m_rgbSpatialScalability [iVO] * ENHN_LAYER; iLayer++)	{
// GMC
			m_rgvolmd [iLayer][iVO].uiVerID	= pArgs->rguiVerID [iVO];  // version id
// ~GMC
			// NBIT
			m_rgvolmd [iLayer][iVO].bNot8Bit 		= pArgs->bNot8Bit;
			m_rgvolmd [iLayer][iVO].uiQuantPrecision 	= pArgs->uiQuantPrecision;
			m_rgvolmd [iLayer][iVO].nBits 			= pArgs->nBits;

			m_rgvolmd [iLayer][iVO].volType			= (VOLtype) iLayer;
			m_rgvolmd [iLayer][iVO].fAUsage			= pArgs->rgfAlphaUsage [iVO];
// MAC 
			if (m_rgvolmd [iLayer][iVO].fAUsage==EIGHT_BIT) {
				m_rgvolmd [iLayer][iVO].iAlphaShapeExtension = pArgs->rgiAlphaShapeExtension [iVO];
				m_rgvolmd [iLayer][iVO].iAuxCompCount = CVideoObject::getAuxCompCount(pArgs->rgiAlphaShapeExtension [iVO]);
			} else
				m_rgvolmd [iLayer][iVO].iAuxCompCount = 0;
// ~MAC
			m_rgvolmd [iLayer][iVO].bShapeOnly		= pArgs->rgbShapeOnly [iVO];
			m_rgvolmd [iLayer][iVO].iBinaryAlphaTH	= pArgs->rgiBinaryAlphaTH [iVO] * 16;  //magic no. from the vm
			m_rgvolmd [iLayer][iVO].iBinaryAlphaRR	= (pArgs->rgiBinaryAlphaRR [iVO]<0) ? -1 : 
				(pArgs->rgiBinaryAlphaRR [iVO] +1)*(pArgs->rgiNumOfBbetweenPVOP [iVO] +1)*pArgs->rgiTemporalRate [iVO];  // Binary shape refresh rate: Added for error resilient mode by Toshiba(1997-11-14): Modified (1998-1-16)
			m_rgvolmd [iLayer][iVO].iGrayToBinaryTH = pArgs->rgiGrayToBinaryTH [iVO];
			m_rgvolmd [iLayer][iVO].bNoCrChange		= pArgs->rgbNoCrChange [iVO];
			m_rgvolmd [iLayer][iVO].bOriginalForME	= pArgs->rgbOriginalForME [iLayer][iVO];
			m_rgvolmd [iLayer][iVO].bAdvPredDisable = pArgs->rgbAdvPredDisable [iLayer][iVO];
			m_rgvolmd [iLayer][iVO].bQuarterSample  = pArgs->rgbQuarterSample [iLayer][iVO]; // QuarterSample mwi 
// NEWPRED
			m_rgvolmd [iLayer][iVO].bNewpredEnable = pArgs->rgbNewpredEnable [iLayer][iVO]; 
			m_rgvolmd [iLayer][iVO].bNewpredSegmentType = pArgs->rgbNewpredSegmentType [iLayer][iVO]; 
			m_rgvolmd [iLayer][iVO].cNewpredRefName = pArgs->rgcNewpredRefName [iLayer][iVO]; 
			m_rgvolmd [iLayer][iVO].cNewpredSlicePoint = pArgs->rgcNewpredSlicePoint [iLayer][iVO]; 
// ~NEWPRED

			// START: Complexity Estimation syntax support - Marc Mongenet (EPFL) - 17 Jun 1998
			m_rgvolmd [iLayer][iVO].bComplexityEstimationDisable = pArgs->rgbComplexityEstimationDisable [iLayer][iVO];
			// START: Complexity Estimation syntax support - Update version 2 - Massimo Ravasi (EPFL) - 11 Nov 1999
			//m_rgvolmd [iLayer][iVO].bVersion2ComplexityEstimationDisable = pArgs->rgbComplexityEstimationDisable [iLayer][iVO] || (pArgs->rguiVerID [iVO]==1);
			// see last lines of current "Complexity Estimation syntax support" section
			// END: Complexity Estimation syntax support - Update version 2

			m_rgvolmd [iLayer][iVO].iEstimationMethod = (Int)pArgs->rguiEstimationMethod [iLayer][iVO];
			m_rgvolmd [iLayer][iVO].bOpaque = pArgs->rgbOpaque [iLayer][iVO];
			m_rgvolmd [iLayer][iVO].bTransparent = pArgs->rgbTransparent [iLayer][iVO];
			m_rgvolmd [iLayer][iVO].bIntraCAE = pArgs->rgbIntraCAE [iLayer][iVO];
			m_rgvolmd [iLayer][iVO].bInterCAE = pArgs->rgbInterCAE [iLayer][iVO];
			m_rgvolmd [iLayer][iVO].bNoUpdate = pArgs->rgbNoUpdate [iLayer][iVO];
			m_rgvolmd [iLayer][iVO].bUpsampling = pArgs->rgbUpsampling [iLayer][iVO];
			m_rgvolmd [iLayer][iVO].bIntraBlocks = pArgs->rgbIntraBlocks [iLayer][iVO];
			m_rgvolmd [iLayer][iVO].bInterBlocks = pArgs->rgbInterBlocks [iLayer][iVO];
			m_rgvolmd [iLayer][iVO].bInter4vBlocks = pArgs->rgbInter4vBlocks [iLayer][iVO];
			m_rgvolmd [iLayer][iVO].bNotCodedBlocks = pArgs->rgbNotCodedBlocks [iLayer][iVO];
			m_rgvolmd [iLayer][iVO].bDCTCoefs = pArgs->rgbDCTCoefs [iLayer][iVO];
			m_rgvolmd [iLayer][iVO].bDCTLines = pArgs->rgbDCTLines [iLayer][iVO];
			m_rgvolmd [iLayer][iVO].bVLCSymbols = pArgs->rgbVLCSymbols [iLayer][iVO];
			m_rgvolmd [iLayer][iVO].bVLCBits = pArgs->rgbVLCBits [iLayer][iVO];
			m_rgvolmd [iLayer][iVO].bAPM = pArgs->rgbAPM [iLayer][iVO];
			m_rgvolmd [iLayer][iVO].bNPM = pArgs->rgbNPM [iLayer][iVO];
			m_rgvolmd [iLayer][iVO].bInterpolateMCQ = pArgs->rgbInterpolateMCQ [iLayer][iVO];
			m_rgvolmd [iLayer][iVO].bForwBackMCQ = pArgs->rgbForwBackMCQ [iLayer][iVO];
			m_rgvolmd [iLayer][iVO].bHalfpel2 = pArgs->rgbHalfpel2 [iLayer][iVO];
			m_rgvolmd [iLayer][iVO].bHalfpel4 = pArgs->rgbHalfpel4 [iLayer][iVO];
			m_rgvolmd [iLayer][iVO].bSadct = pArgs->rgbSadct [iLayer][iVO];
			m_rgvolmd [iLayer][iVO].bQuarterpel = pArgs->rgbQuarterpel [iLayer][iVO];
			// START: Complexity Estimation syntax support - Update version 2 - Massimo Ravasi (EPFL) - 11 Nov 1999
			// redundant flags to avoid start code emulation
			m_rgvolmd [iLayer][iVO].bShapeComplexityEstimationDisable = ! (
				   m_rgvolmd [iLayer][iVO].bOpaque
				|| m_rgvolmd [iLayer][iVO].bTransparent
				|| m_rgvolmd [iLayer][iVO].bIntraCAE
				|| m_rgvolmd [iLayer][iVO].bInterCAE
				|| m_rgvolmd [iLayer][iVO].bNoUpdate
				|| m_rgvolmd [iLayer][iVO].bUpsampling);
			m_rgvolmd [iLayer][iVO].bTextureComplexityEstimationSet1Disable = ! (
				   m_rgvolmd [iLayer][iVO].bIntraBlocks
				|| m_rgvolmd [iLayer][iVO].bInterBlocks
				|| m_rgvolmd [iLayer][iVO].bInter4vBlocks
				|| m_rgvolmd [iLayer][iVO].bNotCodedBlocks);
			m_rgvolmd [iLayer][iVO].bTextureComplexityEstimationSet2Disable = ! (
				   m_rgvolmd [iLayer][iVO].bDCTCoefs
				|| m_rgvolmd [iLayer][iVO].bDCTLines
				|| m_rgvolmd [iLayer][iVO].bVLCSymbols
				|| m_rgvolmd [iLayer][iVO].bVLCBits);
			m_rgvolmd [iLayer][iVO].bMotionCompensationComplexityDisable = ! (
				   m_rgvolmd [iLayer][iVO].bAPM
				|| m_rgvolmd [iLayer][iVO].bNPM
				|| m_rgvolmd [iLayer][iVO].bInterpolateMCQ
				|| m_rgvolmd [iLayer][iVO].bForwBackMCQ
				|| m_rgvolmd [iLayer][iVO].bHalfpel2
				|| m_rgvolmd [iLayer][iVO].bHalfpel4);
			m_rgvolmd [iLayer][iVO].bVersion2ComplexityEstimationDisable = ! (
				   m_rgvolmd [iLayer][iVO].bSadct
				|| m_rgvolmd [iLayer][iVO].bQuarterpel);
			// END: Complexity Estimation syntax support - Update version 2
			// END: Complexity Estimation syntax support

			m_rgvolmd [iLayer][iVO].bAllowSkippedPMBs = pArgs->rgbAllowSkippedPMBs [iVO];


			// HHI Schueuer for sadct
			m_rgvolmd [iLayer][iVO].bSadctDisable		= pArgs->rgbSadctDisable [iLayer][iVO];
			// end

			m_rgvolmd [iLayer][iVO].uiProfileAndLevel	= pArgs->uiProfileAndLevel;
			m_rgvolmd [iLayer][iVO].bCodeSequenceHead	= pArgs->bCodeSequenceHead;

			// START: Vol Control Parameters
			if(pArgs->rguiVolControlParameters[0]!=NULL)
			{
				m_rgvolmd [iLayer][iVO].uiVolControlParameters = pArgs->rguiVolControlParameters [iLayer][iVO];
				m_rgvolmd [iLayer][iVO].uiChromaFormat = pArgs->rguiChromaFormat [iLayer][iVO];
				m_rgvolmd [iLayer][iVO].uiLowDelay = pArgs->rguiLowDelay [iLayer][iVO];
				m_rgvolmd [iLayer][iVO].uiVBVParams = pArgs->rguiVBVParams [iLayer][iVO];
				m_rgvolmd [iLayer][iVO].uiBitRate = pArgs->rguiBitRate [iLayer][iVO];
				m_rgvolmd [iLayer][iVO].uiVbvBufferSize = pArgs->rguiVbvBufferSize [iLayer][iVO];
				m_rgvolmd [iLayer][iVO].uiVbvBufferOccupany = pArgs->rguiVbvBufferOccupany [iLayer][iVO];
			}
			else
				m_rgvolmd [iLayer][iVO].uiVolControlParameters = 0;
			// END: Vol Control Parameters
			m_rgvolmd [iLayer][iVO].bRoundingControlDisable	= (iLayer== BASE_LAYER) ? pArgs->rgbRoundingControlDisable[iVO] : 0;
			m_rgvolmd [iLayer][iVO].iInitialRoundingType = (iLayer== BASE_LAYER) ? pArgs->rgiInitialRoundingType[iVO] : 0;
			m_rgvolmd [iLayer][iVO].bVPBitTh		= pArgs->rgbVPBitTh[iLayer][iVO];
			m_rgvolmd [iLayer][iVO].bDataPartitioning = pArgs->rgbDataPartitioning [iLayer][iVO];
			m_rgvolmd [iLayer][iVO].bReversibleVlc  = pArgs->rgbReversibleVlc [iLayer][iVO];
//RESYNC_MARKER_FIX
			m_rgvolmd [iLayer][iVO].bResyncMarkerDisable = pArgs->rgbResyncMarkerDisable [iLayer][iVO];
//~RESYNC_MARKER_FIX
// RRV insertion
			m_rgvopmd [iLayer][iVO].RRVmode = pArgs->RRVmode[iLayer][iVO];
			m_rgvolmd [iLayer][iVO].breduced_resolution_vop_enable
				= pArgs->RRVmode[iLayer][iVO].iOnOff;
// ~RRV
			m_rgvolmd [iLayer][iVO].fQuantizer		= pArgs->rgfQuant [iLayer][iVO];
			m_rgvolmd [iLayer][iVO].bNoGrayQuantUpdate 
													= pArgs->rgbNoAlphaQuantUpdate [iLayer][iVO];
			m_rgvolmd [iLayer][iVO].dFrameHz		= pArgs->rgdFrameFrequency [iLayer][iVO];
			m_rgvolmd [iLayer][iVO].iMVRadiusPerFrameAwayFromRef = pArgs->rguiSearchRange [iLayer][iVO];
			m_rgvopmd [iLayer][iVO].iSearchRangeForward	= pArgs->rguiSearchRange [iLayer][iVO];
			m_rgvopmd [iLayer][iVO].iSearchRangeBackward= pArgs->rguiSearchRange [iLayer][iVO];
			m_rgvopmd [iLayer][iVO].intStepI		= pArgs->rgiStepI [iLayer][iVO];
			m_rgvopmd [iLayer][iVO].intStep			= pArgs->rgiStepP [iLayer][iVO];
			m_rgvopmd [iLayer][iVO].intStepB		= pArgs->rgiStepB [iLayer][iVO];
			m_rgvopmd [iLayer][iVO].intStepIAlpha[0]	= pArgs->rgiStepIAlpha [iLayer][iVO];
			m_rgvopmd [iLayer][iVO].intStepPAlpha[0]	= pArgs->rgiStepPAlpha [iLayer][iVO];
			m_rgvopmd [iLayer][iVO].intStepBAlpha[0]	= pArgs->rgiStepBAlpha [iLayer][iVO];
			m_rgvopmd [iLayer][iVO].iIntraDcSwitchThr 
													= pArgs->rgiIntraDCSwitchingThr [iLayer][iVO];
			m_rgvopmd [iLayer][iVO].bInterlace		= pArgs->rgbInterlacedCoding [iLayer][iVO];
			m_rgvopmd [iLayer][iVO].bTopFieldFirst	= pArgs->rgbTopFieldFirst [iLayer][iVO];
			m_rgvopmd [iLayer][iVO].bAlternateScan	= pArgs->rgbAlternateScan [iLayer][iVO];
			m_rgvopmd [iLayer][iVO].iDirectModeRadius = pArgs->rgiDirectModeRadius [iLayer][iVO];
			m_rgvolmd [iLayer][iVO].bLoadIntraMatrix= pArgs->rgbLoadIntraMatrix [iLayer][iVO];
			m_rgvolmd [iLayer][iVO].bLoadInterMatrix= pArgs->rgbLoadInterMatrix [iLayer][iVO];
			m_rgvolmd [iLayer][iVO].bLoadIntraMatrixAlpha 
													= pArgs->rgbLoadIntraMatrixAlpha [iLayer][iVO];
			m_rgvolmd [iLayer][iVO].bLoadInterMatrixAlpha 
													= pArgs->rgbLoadInterMatrixAlpha [iLayer][iVO];
			memcpy (m_rgvolmd [iLayer][iVO].rgiIntraQuantizerMatrix,		pArgs->rgppiIntraQuantizerMatrix [iLayer][iVO], BLOCK_SQUARE_SIZE * sizeof (Int));
			memcpy (m_rgvolmd [iLayer][iVO].rgiInterQuantizerMatrix,		pArgs->rgppiInterQuantizerMatrix [iLayer][iVO], BLOCK_SQUARE_SIZE * sizeof (Int));
			memcpy (m_rgvolmd [iLayer][iVO].rgiIntraQuantizerMatrixAlpha[0],	pArgs->rgppiIntraQuantizerMatrixAlpha [iLayer][iVO], BLOCK_SQUARE_SIZE * sizeof (Int));
			memcpy (m_rgvolmd [iLayer][iVO].rgiInterQuantizerMatrixAlpha[0],	pArgs->rgppiInterQuantizerMatrixAlpha [iLayer][iVO], BLOCK_SQUARE_SIZE * sizeof (Int));

			m_rgvolmd [iLayer][iVO].bDeblockFilterDisable 
													= pArgs->rgbDeblockFilterDisable [iVO];
			m_rgvolmd [iLayer][iVO].iBbetweenP	= pArgs->rgiNumOfBbetweenPVOP [iVO];
			m_rgvolmd [iLayer][iVO].iPbetweenI	= pArgs->rgiNumOfPbetweenIVOP [iVO];
			m_rgvolmd [iLayer][iVO].iGOVperiod = pArgs->rgiGOVperiod [iVO]; // count of o/p frames between govs
			// set up temporal sampling rate and clock rate from source frame rate.
			m_rgvolmd [iLayer][iVO].iTemporalRate	= pArgs->rgiTemporalRate [iVO]; // frames to skip
			Double dFrameHz = m_rgvolmd [iLayer][iVO].dFrameHz;
			// this code copes with e.g. 12.5 fps and 29.97 fps by multiplying up the ticks per second
			// count appropriately
			Int iClockRate = (int)dFrameHz;
			if(dFrameHz != (double)iClockRate)
			{
				iClockRate = (int)(dFrameHz * 10.0);
				if(dFrameHz * 10.0 != (double)iClockRate)
					iClockRate = (int)(dFrameHz * 100.0);	 // not much point going to 1000*
			}
			m_rgvolmd [iLayer][iVO].iClockRate = iClockRate;

			m_rgvolmd [iLayer][iVO].bDumpMB			= pArgs->rgbDumpMB [iVO];
			m_rgvolmd [iLayer][iVO].bTrace			= pArgs->rgbTrace [iVO];
			m_rgvolmd [iLayer][iVO].bTemporalScalability = FALSE;
			m_rgvolmd [iLayer][iVO].bSpatialScalability = m_rgbSpatialScalability[iVO];		//OBSS_SAIT_991015 //OBSS
			m_rgvolmd [iLayer][iVO].iHierarchyType = 0;										//OBSSFIX_V2-8
			if(m_rgvolmd[iLayer][iVO].volType == ENHN_LAYER && m_rgbSpatialScalability [iVO] && pArgs->rgiTemporalRate[iVO] == pArgs->rgiEnhcTemporalRate[iVO]){
				// modified by Sharp (98/02/09)
//#ifdef _Scalable_SONY_
				m_rgvolmd[iLayer][iVO].iHierarchyType = 0; //iHierarchyType 0 means spatial scalability
//#endif _Scalable_SONY_
				m_rgvolmd[iLayer][iVO].iEnhnType = 0; // Spatial Scalable Enhancement layer shuold be set to Rectangular shape
				m_rgvolmd[iLayer][iVO].iSpatialOption = pArgs->iSpatialOption; // This Option is for Spatial Scalable
				m_rgvolmd[iLayer][iVO].iver_sampling_factor_n = pArgs->uiVer_sampling_n;
				m_rgvolmd[iLayer][iVO].iver_sampling_factor_m = pArgs->uiVer_sampling_m;
				m_rgvolmd[iLayer][iVO].ihor_sampling_factor_n = pArgs->uiHor_sampling_n;
				m_rgvolmd[iLayer][iVO].ihor_sampling_factor_m = pArgs->uiHor_sampling_m;
//OBSS_SAIT_991015
				m_rgvolmd[iLayer][iVO].iEnhnTypeSpatial = 0;		//for OBSS partial enhancement mode
				m_rgvolmd[iLayer][iVO].iFrmWidth_SS = pArgs->uiFrameWidth_SS;
				m_rgvolmd[iLayer][iVO].iFrmHeight_SS = pArgs->uiFrameHeight_SS;
//OBSSFIX_MODE3
				if(iLayer == ENHN_LAYER && m_rgvolmd[0][iVO].fAUsage == RECTANGLE &&
					m_rgvolmd[iLayer][iVO].iHierarchyType == 0 && pArgs->rgiEnhancementTypeSpatial[iVO] != 0){
					m_rgvolmd[iLayer][iVO].iEnhnType = pArgs->rgiEnhancementTypeSpatial[iVO]; //for OBSS partial enhancement mode
					m_rgvolmd[iLayer][iVO].iEnhnTypeSpatial = pArgs->rgiEnhancementTypeSpatial[iVO]; //for OBSS partial enhancement mode
					m_rgvolmd[iLayer][iVO].fAUsage   = ONE_BIT;
				}
//~OBSSFIX_MODE3
				if(	m_rgvolmd [iLayer][iVO].fAUsage	== ONE_BIT && m_rgvolmd[iLayer][iVO].iHierarchyType ==0 ) {
					//for OBSS partial enhancement mode
//OBSSFIX_MODE3
					m_rgvolmd[iLayer][iVO].iEnhnTypeSpatial = pArgs->rgiEnhancementTypeSpatial [iVO];
					m_rgvolmd[iLayer][iVO].iEnhnType = pArgs->rgiEnhancementTypeSpatial [iVO]; 
//~OBSSFIX_MODE3
					m_rgvolmd[iLayer][iVO].iuseRefShape = pArgs->uiUse_ref_shape; 
					m_rgvolmd[iLayer][iVO].iuseRefTexture = pArgs->uiUse_ref_texture; 
					m_rgvolmd[iLayer][iVO].iver_sampling_factor_n_shape = pArgs->uiVer_sampling_n_shape;
					m_rgvolmd[iLayer][iVO].iver_sampling_factor_m_shape = pArgs->uiVer_sampling_m_shape;
					m_rgvolmd[iLayer][iVO].ihor_sampling_factor_n_shape = pArgs->uiHor_sampling_n_shape;
					m_rgvolmd[iLayer][iVO].ihor_sampling_factor_m_shape = pArgs->uiHor_sampling_m_shape;
				}
//~OBSS_SAIT_991015
			} 
// begin: added by Sharp (98/2/12)
			else if (m_rgbSpatialScalability [iVO] && pArgs->rgiTemporalRate[iVO] != pArgs->rgiEnhcTemporalRate[iVO]){
				m_rgvolmd [iLayer][iVO].bTemporalScalability = TRUE;
//#ifdef _Scalable_SONY_
				m_rgvolmd[iLayer][iVO].iHierarchyType = 1; //iHierarchyType 1 means temporal scalability
//#endif _Scalable_SONY_
				m_rgvolmd [iLayer][iVO].iEnhnType	= pArgs->rgiEnhancementType [iVO];
				if ( pArgs->rgiEnhancementType[iVO] != 0 ) // modified by Sharp (98/3/24)
					m_rgvolmd [BASE_LAYER][iVO].fAUsage = RECTANGLE;
				if ( iLayer == ENHN_LAYER ) {
					m_rgvolmd [ENHN_LAYER][iVO].iTemporalRate	= pArgs->rgiEnhcTemporalRate [iVO];
					Double dFrameHz = m_rgvolmd [ENHN_LAYER][iVO].dFrameHz;
					// this code copes with e.g. 12.5 fps and 29.97 fps by multiplying up the ticks per second
					// count appropriately
					Int iClockRate = (int)dFrameHz;
					if(dFrameHz != (double)iClockRate)
					{
						iClockRate = (int)(dFrameHz * 10.0);
						if(dFrameHz * 10.0 != (double)iClockRate)
							iClockRate = (int)(dFrameHz * 100.0);	 // not much point going to 1000*
					}
					m_rgvolmd [ENHN_LAYER][iVO].iClockRate = iClockRate;
				}
				m_rgvolmd[iLayer][iVO].iver_sampling_factor_n = 1;
				m_rgvolmd[iLayer][iVO].iver_sampling_factor_m = 1;
				m_rgvolmd[iLayer][iVO].ihor_sampling_factor_n = 1;
				m_rgvolmd[iLayer][iVO].ihor_sampling_factor_m = 1;
			}
// end: added by Sharp (98/2/12)
			else {
				m_rgvolmd[iLayer][iVO].iver_sampling_factor_n = 1;
				m_rgvolmd[iLayer][iVO].iver_sampling_factor_m = 1;
				m_rgvolmd[iLayer][iVO].ihor_sampling_factor_n = 1;
				m_rgvolmd[iLayer][iVO].ihor_sampling_factor_m = 1;
			}
		}
	}
	static Char pchYUV [100], pchSeg [100];
	// check whether there are data for separate layers
	sprintf (pchYUV, ROOT_YUVFILE, m_pchBmpFiles, m_pchPrefix);
	sprintf (pchSeg, ROOT_SEGFILE, m_pchBmpFiles, m_pchPrefix);
	m_bTexturePerVOP = !fileExistence (pchYUV);
	m_bAlphaPerVOP = !fileExistence (pchSeg);
	m_bPrevObjectExists = FALSE; // added by Sharp (98/2/13)

	// sprite info
	m_ppstSrc = new CSiteD* [m_iNumVO];
	m_pppstDst = new CSiteD** [m_iNumVO];
	for (iVO = 0; iVO < m_iNumVO; iVO++) {
		m_ppstSrc [iVO] = (m_rgNumOfPnts [iVO] > 0) ? new CSiteD [m_rgNumOfPnts [iVO]] : NULL;
		m_pppstDst [iVO] = new CSiteD* [m_iNumFrame];
		for (Int ifr = 0; ifr < m_iNumFrame; ifr++) {
			m_pppstDst [iVO] [ifr] = 
				(m_rgNumOfPnts [iVO] > 0) ? new CSiteD [m_rgNumOfPnts [iVO]] : 
				NULL;
		}
	}
	for (iVO = m_iFirstVO; iVO <= m_iLastVO; iVO++)
		if (m_rguiSpriteUsage[iVO - m_iFirstVO] == 1 && m_rgNumOfPnts [iVO - m_iFirstVO] > 0) // GMC
				readPntFile (iVO);

    m_SptMode =	pArgs->pSpriteMode[0];
	m_rgiMVFileUsage = pArgs->rgiMVFileUsage;
	m_pchMVFileName  = pArgs->pchMVFileName;
}

Void CSessionEncoder::encode ()
{
	FILE* pfYuvSrc = NULL;
	FILE* pfYuvSrcSpatialEnhn = NULL;
	FILE* pfSegSrc = NULL;
	FILE* rgpfReconYUV [2];
	FILE* rgpfReconSeg [2];
	Int iVO;
	UInt iVOrelative = 0;
//OBSS_SAIT_991015
	FILE* pfSegSrcSpatialEnhn = NULL;	
	static CRct pBase_stack_rctBase;
	static ShapeMode* pBase_stack_Baseshpmd = NULL;
	static CMotionVector* pBase_stack_mvBaseBY = NULL;	
	static ShapeMode* pBase_tmp_Baseshpmd = NULL;
	static CMotionVector* pBase_tmp_mvBaseBY = NULL;	
	static Int iBase_stack_x, iBase_stack_y;
//~OBSS_SAIT_991015

// begin: added by Sharp (98/2/12)
	Bool bTemporalScalability = m_rgvolmd[BASE_LAYER][0].bTemporalScalability;
//	Bool bTemporalScalability = TRUE;

	// for a buffer management of temporal scalability
	CEnhcBufferEncoder* pBufP1 = NULL;
	CEnhcBufferEncoder* pBufP2 = NULL;
	CEnhcBufferEncoder* pBufB1 = NULL;
	CEnhcBufferEncoder* pBufB2 = NULL;
	CEnhcBufferEncoder* pBufE = NULL;

	// for backward/forward shape
	Void set_modes(VOLMode* volmd, VOPMode* vopmd);
	VOLMode* volmd_backShape = NULL;
	VOLMode* volmd_forwShape = NULL;
	VOPMode* vopmd_backShape = NULL;
	VOPMode* vopmd_forwShape = NULL;

	if ( bTemporalScalability ){
		pBufP1 = new CEnhcBufferEncoder(m_rctOrg.width, m_rctOrg.height());
		pBufP2 = new CEnhcBufferEncoder(m_rctOrg.width, m_rctOrg.height());
		pBufB1 = new CEnhcBufferEncoder(m_rctOrg.width, m_rctOrg.height());
		pBufB2 = new CEnhcBufferEncoder(m_rctOrg.width, m_rctOrg.height());
		pBufE  = new CEnhcBufferEncoder(m_rctOrg.width, m_rctOrg.height());

		volmd_backShape = new VOLMode;
		volmd_forwShape = new VOLMode;
		vopmd_backShape = new VOPMode;
		vopmd_forwShape = new VOPMode;
		set_modes(volmd_backShape, vopmd_backShape);
		set_modes(volmd_forwShape, vopmd_forwShape);
	}
// end: added by Sharp (98/2/12)
	FILE** ppfAuxSrc = NULL;
	FILE** rgpfReconAux[2];
	rgpfReconAux[0] = rgpfReconAux[1] = NULL;


	for (iVO = m_iFirstVO; iVO <= (Int) m_iLastVO; iVO++, iVOrelative++) {
		ofstream* rgpostrm [2];
		ofstream* rgpostrmTrace [2];
		PixelC pxlcObjColor;
		VOLMode volmd = m_rgvolmd [BASE_LAYER] [iVOrelative];
		VOLMode volmd_enhn = m_rgvolmd [ENHN_LAYER] [iVOrelative]; // added by Sharp (98/2/10)
		ppfAuxSrc = new FILE* [volmd.iAuxCompCount];
		rgpfReconAux[0] = new FILE* [volmd.iAuxCompCount];
		rgpfReconAux[1] = new FILE* [volmd.iAuxCompCount]; // actually never used

		getInputFiles (	pfYuvSrc, pfSegSrc, ppfAuxSrc, pfYuvSrcSpatialEnhn,
						pfSegSrcSpatialEnhn,//OBSS_SAIT_991015
						rgpfReconYUV, rgpfReconSeg, rgpfReconAux,
						rgpostrm, rgpostrmTrace, pxlcObjColor, iVO, volmd, volmd_enhn); // modified by Sharp(98/2/10)
		CRct rctOrg;
		CVideoObjectEncoder* rgpvoenc [2];
		if (m_rguiSpriteUsage [iVOrelative] == 1) {
			// change m_rctOrg to sprite size
			rctOrg = m_rctOrg;
			m_rctFrame = m_rctOrg;
			m_rctOrg = findSptRct (iVO);
		}

// NEWPRED
		if (m_rgvolmd [BASE_LAYER][iVOrelative].bNewpredEnable) 
			g_pNewPredEnc = new CNewPredEncoder();
// ~NEWPRED

		initVOEncoder (rgpvoenc, iVO, rgpostrmTrace);
#undef write
		rgpostrm [BASE_LAYER]->write (rgpvoenc [BASE_LAYER]->pOutStream ()->str (),			//VO and VOL header
									  rgpvoenc [BASE_LAYER]->pOutStream ()->pcount ());

		if(m_rgbSpatialScalability[iVOrelative]){
			rgpostrm[ENHN_LAYER]->write(rgpvoenc [ENHN_LAYER]->pOutStream () ->str(),
										rgpvoenc [ENHN_LAYER]->pOutStream () ->pcount ());
		}
	// begin: added by Sharp (98/2/12)
		// for back/forward shape
		if ( bTemporalScalability ){
		  initVObfShape (rgpvoenc[ENHN_LAYER]->rgpbfShape, iVO, *volmd_backShape, *vopmd_backShape, *volmd_forwShape, *vopmd_forwShape);
		  // copy pointers
		  rgpvoenc[ENHN_LAYER]->rgpbfShape[0]->m_pchBitsBuffer = rgpvoenc[ENHN_LAYER]->m_pchBitsBuffer;
		  rgpvoenc[ENHN_LAYER]->rgpbfShape[0]->m_pbitstrmOut   = rgpvoenc[ENHN_LAYER]->m_pbitstrmOut;
		  rgpvoenc[ENHN_LAYER]->rgpbfShape[0]->m_pentrencSet   = rgpvoenc[ENHN_LAYER]->m_pentrencSet;

		  rgpvoenc[ENHN_LAYER]->rgpbfShape[0]->m_pchShapeBitsBuffer = rgpvoenc[ENHN_LAYER]->m_pchShapeBitsBuffer;	// Oct 8
		  rgpvoenc[ENHN_LAYER]->rgpbfShape[0]->m_pbitstrmShape      = rgpvoenc[ENHN_LAYER]->m_pbitstrmShape;		//
		  rgpvoenc[ENHN_LAYER]->rgpbfShape[0]->m_pbitstrmShapeMBOut = rgpvoenc[ENHN_LAYER]->m_pbitstrmShapeMBOut;	//

		  rgpvoenc[ENHN_LAYER]->rgpbfShape[1]->m_pchBitsBuffer = rgpvoenc[ENHN_LAYER]->m_pchBitsBuffer;
		  rgpvoenc[ENHN_LAYER]->rgpbfShape[1]->m_pbitstrmOut   = rgpvoenc[ENHN_LAYER]->m_pbitstrmOut;
		  rgpvoenc[ENHN_LAYER]->rgpbfShape[1]->m_pentrencSet   = rgpvoenc[ENHN_LAYER]->m_pentrencSet;

		  rgpvoenc[ENHN_LAYER]->rgpbfShape[1]->m_pchShapeBitsBuffer = rgpvoenc[ENHN_LAYER]->m_pchShapeBitsBuffer;	// Oct 8
		  rgpvoenc[ENHN_LAYER]->rgpbfShape[1]->m_pbitstrmShape	    = rgpvoenc[ENHN_LAYER]->m_pbitstrmShape;		//
		  rgpvoenc[ENHN_LAYER]->rgpbfShape[1]->m_pbitstrmShapeMBOut = rgpvoenc[ENHN_LAYER]->m_pbitstrmShapeMBOut;	//
		}
	// end: added by Sharp (98/2/12)
		if (m_rguiSpriteUsage [iVO - m_iFirstVO] == 1) {
			// load sprite data into m_pvopcOrig
			loadSpt (iVO, rgpvoenc [BASE_LAYER] -> m_pvopcOrig);
			// encode the initial sprite
			if (m_SptMode == BASIC_SPRITE) 
				rgpvoenc [BASE_LAYER] -> encode (TRUE, -1, IVOP);
			else
				rgpvoenc [BASE_LAYER] -> encodeInitSprite (rctOrg);

			if (m_rgNumOfPnts [iVOrelative] > 0) {
				// change m_pvopcRefQ1 to m_pvopcSptQ for warping
				rgpvoenc [BASE_LAYER] -> swapRefQ1toSpt ();
				// restore m_pvopcCurrQ size to the normal one
				m_rctOrg = rctOrg;
				rgpvoenc [BASE_LAYER] -> changeSizeofCurrQ (rctOrg);
			}
			rgpostrm [BASE_LAYER]->write (rgpvoenc [BASE_LAYER]->pOutStream ()->str (), //write sprite unit
										  rgpvoenc [BASE_LAYER]->pOutStream ()->pcount ());
		}
		assert (!m_rctOrg.empty ());

		if (rgpvoenc [BASE_LAYER]->m_uiRateControl>=RC_TM5) {
			assert (!m_rgbSpatialScalability [iVOrelative]);
			assert (!bTemporalScalability);
			//assert (volmd.iTemporalRate==1);
#ifndef __TRACE_AND_STATS_
			cerr << "Compile flag __TRACE_AND_STATS_ required for stats for TM5 rate control." << endl;
			exit(1);
#endif // __TRACE_AND_STATS_
			char pchQname[100];
			pchQname[0] = 0;
			//if( m_bRGBfiles ) {
			//	if( m_pchOutStrFiles && m_pchOutStrFiles[0]) sprintf (pchQname, m_pchOutStrFiles, "q", iVO);
			//} else {
				if( m_pchOutStrFiles && m_pchOutStrFiles[0]) {
					sprintf (pchQname, SUB_CMPFILE, m_pchOutStrFiles, iVO, m_pchPrefix);
				}
			//}
			rgpvoenc [BASE_LAYER]->m_tm5rc.tm5rc_init_seq ( pchQname,
				rgpvoenc [BASE_LAYER]->m_uiRateControl,
				m_rgvolmd [BASE_LAYER] [iVOrelative].fAUsage,
				m_rctOrg.width,
				m_rctOrg.height(),
				m_rguiBudget [BASE_LAYER] [iVOrelative], // bits per second
				m_rgvolmd [BASE_LAYER][iVOrelative].dFrameHz / volmd.iTemporalRate // actual frame rate
			);
		}

		// algorithm for generation of IPB sequences with arbitrary shape
		 //   (transparency allows for possible skipped frames) 
		 //   P prediction across skips is not allowed 

#define DUMP_CURR	0
#define DUMP_PREV	1
#define DUMP_NONE	2

		Int iRefFrame;
		Int iDT = volmd.iTemporalRate;
		Int iDT_enhn = volmd_enhn.iTemporalRate; // added by Sharp (98/2/12)
		Int iRefInterval = volmd.iBbetweenP + 1;
		Int iPCount;
		Bool bObjectExists;
		const CVOPU8YUVBA* pvopcBaseQuant = NULL;
		Int iEcount = 0; // added by Sharp (98/2/12)
		Bool bCachedRefDump = FALSE;
		Bool bCachedRefCoded = TRUE;
		Bool bPrevObjectExists = FALSE; // added by Sharp (99/1/27)
		m_rgpvopcPrevDisp[BASE_LAYER] = NULL; // previously displayed vop pointers
		m_rgpvopcPrevDisp[ENHN_LAYER] = NULL;

		for(iRefFrame = m_iFirstFrame; iRefFrame <= m_iLastFrame; iRefFrame += iDT)
		{
			// encode initial I frame or non-coded sequence
			if(rgpvoenc [BASE_LAYER] -> skipTest(iRefFrame,IVOP)) // rate control
				continue;

			//encode GOV header added by SONY 980212
			// moved down slightly by swinder 980219
			//CAUTION:I don't know how GOV header is encoded in sprite mode
			// re-done by swinder 980511
			// gov header is output every n output frames, where n is iGOVperiod
			if (volmd.iGOVperiod != 0 
				&& ((iRefFrame-m_iFirstFrame) % (volmd.iGOVperiod * volmd.iTemporalRate)) == 0)
			{
				rgpvoenc [BASE_LAYER] -> codeGOVHead (iRefFrame - m_iFirstFrame);
				rgpostrm [BASE_LAYER]->write (rgpvoenc [BASE_LAYER]->pOutStream ()->str (),
					rgpvoenc [BASE_LAYER]->pOutStream ()->pcount ());
			}
			//980211

			// first dump any cached frames, otherwise a non coded frame will be out of order
			if(bCachedRefDump && (m_rguiSpriteUsage [iVOrelative] == 0 || m_rguiSpriteUsage [iVOrelative] == 2)) // GMC
			{
				bCachedRefDump = FALSE;
				// last ref frame needs to be output
#ifndef __OUT_ONE_FRAME_

				if ( bCachedRefCoded )
					dumpData (rgpfReconYUV [BASE_LAYER], rgpfReconSeg [BASE_LAYER], 
                    rgpfReconAux [BASE_LAYER], BASE_LAYER,
                    rgpvoenc[BASE_LAYER] ->pvopcRefQLater(), m_rctOrg, volmd,
                    rgpvoenc[BASE_LAYER] ->m_vopmd.bInterlace);
				else
					dumpDataNonCoded(rgpfReconYUV [BASE_LAYER], rgpfReconSeg [BASE_LAYER], 
						rgpfReconAux [BASE_LAYER], BASE_LAYER, m_rctOrg, volmd);

				if (m_rgbSpatialScalability [iVOrelative] && !bTemporalScalability)
					dumpData (rgpfReconYUV [ENHN_LAYER], rgpfReconSeg [ENHN_LAYER],
                    rgpfReconAux [ENHN_LAYER], ENHN_LAYER,
                    rgpvoenc[ENHN_LAYER] ->pvopcRefQLater(), m_rctOrgSpatialEnhn, volmd, 0);
#endif
			}

			int iGOPperiod = (volmd.iPbetweenI + 1) * (volmd.iBbetweenP + 1);
			if (rgpvoenc [BASE_LAYER]->m_uiRateControl >= RC_TM5) 
			{
				assert(iGOPperiod!=0);
				Int npic = (m_iLastFrame - iRefFrame + iDT) / iDT;
				Int nppic;
				if(npic>iGOPperiod)
					npic = iGOPperiod;
				if (iRefFrame == m_iFirstFrame) {
					if (npic > (iGOPperiod - volmd.iBbetweenP))
						npic = iGOPperiod - volmd.iBbetweenP;
					nppic = (npic + 1)/(volmd.iBbetweenP + 1);
				} else {
					nppic = (npic - 1)/(volmd.iBbetweenP + 1);
				}
				rgpvoenc [BASE_LAYER] -> m_tm5rc.tm5rc_init_GOP(nppic, npic - nppic - 1); //  np, nb remain
			}

			// encode non-coded frames or initial IVOPs
			// we always dump these out
//OBSS_SAIT_991015
			if(m_rgbSpatialScalability [iVOrelative] && !bTemporalScalability && (volmd.fAUsage == ONE_BIT))
				bObjectExists = loadDataSpriteCheck (iVOrelative,iRefFrame, pfYuvSrc, pfSegSrcSpatialEnhn, pxlcObjColor, rgpvoenc [BASE_LAYER]->m_pvopcOrig, volmd, volmd_enhn);
			else
//~OBSS_SAIT_991015
			bObjectExists = loadDataSpriteCheck (iVOrelative,iRefFrame, pfYuvSrc, pfSegSrc, ppfAuxSrc, pxlcObjColor, rgpvoenc [BASE_LAYER]->m_pvopcOrig, volmd);
			encodeVideoObject(bObjectExists, bObjectExists, iRefFrame, IVOP, DUMP_CURR,
							  iVO, iVOrelative, BASE_LAYER, 
							  pfYuvSrc,pfSegSrc,ppfAuxSrc,
                rgpfReconYUV,rgpfReconSeg,rgpfReconAux,
							  pxlcObjColor, rgpvoenc, volmd, rgpostrm);

			if (m_rgbSpatialScalability [iVOrelative] && !bTemporalScalability ) { // modified by Sharp (98/2/12)
				pvopcBaseQuant = rgpvoenc [BASE_LAYER]->pvopcReconCurr ();
//OBSS_SAIT_991015
				encodeVideoObject (bObjectExists, bObjectExists, iRefFrame, PVOP, DUMP_CURR,
								   iVO, iVOrelative, ENHN_LAYER,
								   pfYuvSrcSpatialEnhn, pfSegSrcSpatialEnhn, /*NULL*/ppfAuxSrc,		//OBSSFIX_MAC
                   rgpfReconYUV, rgpfReconSeg, /*NULL*/rgpfReconAux,
								   pxlcObjColor, rgpvoenc, volmd, rgpostrm,
								   pvopcBaseQuant);
//~OBSS_SAIT_991015
			}
// begin: added by Sharp (98/2/12)
				else if (m_rgbSpatialScalability [iVOrelative] && bTemporalScalability)
					pBufP2->getBuf( rgpvoenc[BASE_LAYER] );
// end: added by Sharp (98/2/12)

			// go to next frame if this was not coded or we are just coding sprites
			if(!bObjectExists)
				continue; 

			// we dumped first frame so rest must be delayed by one for re-order
			iPCount = volmd.iPbetweenI;			
			Int iWaitInterval = 0;

			while (TRUE) {
				// search for next reference frame
				Int iSearchFrame;
				for(iSearchFrame = iRefFrame + iDT * iRefInterval + iWaitInterval;
						iSearchFrame > iRefFrame; iSearchFrame -= iDT)
					if(iSearchFrame <= m_iLastFrame)
					{
//OBSS_SAIT_991015
						if(m_rgbSpatialScalability [iVOrelative] && !bTemporalScalability && (volmd.fAUsage == ONE_BIT))
							bObjectExists = loadDataSpriteCheck(iVOrelative,iSearchFrame, pfYuvSrc, pfSegSrcSpatialEnhn, pxlcObjColor, rgpvoenc [BASE_LAYER]->m_pvopcOrig, volmd,volmd_enhn);
						else
//~OBSS_SAIT_991015
						bObjectExists = loadDataSpriteCheck(iVOrelative,iSearchFrame, pfYuvSrc, pfSegSrc, ppfAuxSrc, pxlcObjColor, rgpvoenc [BASE_LAYER]->m_pvopcOrig, volmd);
						break;  // found a suitable reference frame
						// may not be coded
					}

				if(iSearchFrame==iRefFrame)
					break;

				if (rgpvoenc [BASE_LAYER] -> skipTest(iSearchFrame,iPCount ? PVOP : IVOP)) // rate control
				{
					// too early! need to wait a frame
					iWaitInterval += iDT;
					continue;
				}
				iWaitInterval = 0;
				CVOPU8YUVBA* pvopcBasePVOPQuant = NULL;

// begin: added by Sharp (98/2/12)
				if ( bTemporalScalability )
					if ( pBufP2 -> m_bCodedFutureRef == 1 ) // added by Sharp (99/1/28)
					pBufP1->copyBuf ( *pBufP2 );
// end: added by Sharp (98/2/12)
				// encode the next reference frame
				//				Bool bCachedRefDumpSaveForSpatialScalability = bCachedRefDump;
				if(iPCount==0)
				{
					//added to encode GOV header by SONY 980212
					// moved to here by swinder 980219
					//CAUTION:I don't know how GOV header is encoded in sprite mode - SONY
					// update by swinder 980511
					if (volmd.iGOVperiod != 0 
						&& ((iSearchFrame-m_iFirstFrame) % (volmd.iGOVperiod * volmd.iTemporalRate)) == 0)
					{
//modified by SONY (98/03/30)
						rgpvoenc [BASE_LAYER] -> codeGOVHead (iRefFrame - m_iFirstFrame + iDT);
//modified by SONY (98/03/30) End
			/*				rgpvoenc [BASE_LAYER] -> codeGOVHead (iSearchFrame - m_iFirstFrame);
					Original*/ // why was this changed? - swinder
						rgpostrm [BASE_LAYER]->write (rgpvoenc [BASE_LAYER]->pOutStream ()->str (),
							rgpvoenc [BASE_LAYER]->pOutStream ()->pcount ());
					}
					//980212

					int iGOPperiod = (volmd.iPbetweenI + 1) * (volmd.iBbetweenP + 1);
					if (rgpvoenc [BASE_LAYER]->m_uiRateControl >= RC_TM5) 
					{
						assert(iGOPperiod!=0);
						Int npic = (m_iLastFrame - iRefFrame) / iDT; // include prior b pics (before i-vop)
						Int nppic;
						if(npic>iGOPperiod)
							npic = iGOPperiod;
						if (iSearchFrame == m_iFirstFrame) {
							if (npic > (iGOPperiod - volmd.iBbetweenP))
								npic = iGOPperiod - volmd.iBbetweenP;
							nppic = (npic + 1)/(volmd.iBbetweenP + 1);
						} else {
							nppic = (npic - 1)/(volmd.iBbetweenP + 1);
						}
						rgpvoenc [BASE_LAYER] -> m_tm5rc.tm5rc_init_GOP(nppic, npic - nppic - 1); //  np, nb remain
					}

					// encode IVOP

					encodeVideoObject(bObjectExists, bPrevObjectExists, iSearchFrame, IVOP, bCachedRefDump ? DUMP_PREV : DUMP_NONE, // modified by Sharp (99/1/27)
									  iVO, iVOrelative, BASE_LAYER, 
									  pfYuvSrc, pfSegSrc, ppfAuxSrc,
                    rgpfReconYUV, rgpfReconSeg, rgpfReconAux,
 									  pxlcObjColor, rgpvoenc, volmd, rgpostrm);

					bCachedRefDump = TRUE; // need to output this frame later
					bCachedRefCoded = bObjectExists;

					iPCount = volmd.iPbetweenI;
//OBSS_SAIT_991015
					if (m_rgbSpatialScalability [iVOrelative] && !bTemporalScalability ) { // modified by Sharp
                        pvopcBasePVOPQuant = new CVOPU8YUVBA (*(rgpvoenc [BASE_LAYER]->pvopcReconCurr ()),
												        	rgpvoenc [BASE_LAYER]->pvopcReconCurr ()->whereY());
//for OBSS BVOP_BASE : stack	
//OBSSFIX_MODE3
						if(rgpvoenc[ENHN_LAYER]->volmd().fAUsage == ONE_BIT &&
						 !(rgpvoenc[ENHN_LAYER]->volmd().iEnhnType != 0 && rgpvoenc[ENHN_LAYER]->volmd().iuseRefShape == 1)) {
//						if(rgpvoenc[ENHN_LAYER]->volmd().fAUsage == ONE_BIT) {
//~OBSSFIX_MODE3
							pBase_stack_rctBase = rgpvoenc [BASE_LAYER] -> getBaseRct();
							iBase_stack_x = rgpvoenc [BASE_LAYER] -> m_iNumMBX;		
							iBase_stack_y = rgpvoenc [BASE_LAYER] -> m_iNumMBY;		
							pBase_stack_Baseshpmd = new ShapeMode [iBase_stack_x*iBase_stack_y];
							Int i;
							for(i=0;i<iBase_stack_x*iBase_stack_y;i++)
								pBase_stack_Baseshpmd[i] = (rgpvoenc[BASE_LAYER] ->shapemd())[i];

							pBase_stack_mvBaseBY = new CMotionVector [iBase_stack_x*iBase_stack_y];
							for(i=0;i<iBase_stack_x*iBase_stack_y;i++)
								pBase_stack_mvBaseBY[i] = (rgpvoenc[BASE_LAYER] ->getmvBaseBY())[i];
						} 
//~for OBSS BVOP_BASE : stack	
					}
//~OBSS_SAIT_991015
// begin: added by Sharp (98/2/12)
					else if (m_rgbSpatialScalability [iVOrelative] && bTemporalScalability)
						pBufP2->getBuf( rgpvoenc[BASE_LAYER] );
// end: added by Sharp (98/2/12)
				}
				else
				{
					// encoder PVOP
					encodeVideoObject(bObjectExists, bPrevObjectExists, iSearchFrame, (m_rguiSpriteUsage [iVOrelative]==0 ? PVOP : SPRITE), bCachedRefDump ? DUMP_PREV : DUMP_NONE, // modified by Sharp (99/1/27) // GMC
									  iVO, iVOrelative, BASE_LAYER, 
									  pfYuvSrcSpatialEnhn, pfSegSrc, ppfAuxSrc,
                    rgpfReconYUV, rgpfReconSeg, rgpfReconAux,
									  pxlcObjColor, rgpvoenc, volmd, rgpostrm);
					bCachedRefDump = TRUE; // need to output this frame later
					bCachedRefCoded = bObjectExists;
//OBSS_SAIT_991015
					if (m_rgbSpatialScalability [iVOrelative] && !bTemporalScalability ) { // modified by Sharp (98/2/12)
						pvopcBasePVOPQuant = new CVOPU8YUVBA (*(rgpvoenc [BASE_LAYER]->pvopcReconCurr ()),
												        	rgpvoenc [BASE_LAYER]->pvopcReconCurr ()->whereY());
						//OBSS BVOP_BASE : stack
//OBSSFIX_MODE3
						if(rgpvoenc[ENHN_LAYER]->volmd().fAUsage == ONE_BIT &&
						  !(rgpvoenc[ENHN_LAYER]->volmd().iEnhnType != 0 && rgpvoenc[ENHN_LAYER]->volmd().iuseRefShape == 1 )) {
//						if(rgpvoenc[ENHN_LAYER]->volmd().fAUsage == ONE_BIT) {
//~OBSSFIX_MODE3
							pBase_stack_rctBase = rgpvoenc [BASE_LAYER] -> getBaseRct();
							iBase_stack_x = rgpvoenc [BASE_LAYER] -> m_iNumMBX;		
							iBase_stack_y = rgpvoenc [BASE_LAYER] -> m_iNumMBY;		
							pBase_stack_Baseshpmd = new ShapeMode [iBase_stack_x*iBase_stack_y];
							Int i;
							for(i=0;i<iBase_stack_x*iBase_stack_y;i++)
								pBase_stack_Baseshpmd[i] = (rgpvoenc[BASE_LAYER] ->shapemd())[i];

							pBase_stack_mvBaseBY = new CMotionVector [iBase_stack_x*iBase_stack_y];
							for(i=0;i<iBase_stack_x*iBase_stack_y;i++)
								pBase_stack_mvBaseBY[i] = (rgpvoenc[BASE_LAYER] ->getmvBaseBY())[i];
						} 
					}
//~OBSS_SAIT_991015
// begin: added by Sharp (98/2/12)
						else if (m_rgbSpatialScalability [iVOrelative] && bTemporalScalability)
							pBufP2->getBuf( rgpvoenc[BASE_LAYER] );
// end: added by Sharp (98/2/12)

					if (iPCount>0)  // needed to handle iPCount = -1
						iPCount--;
				}
				bPrevObjectExists = bObjectExists;

				// encode B frames if needed
				Int iBFrame = iRefFrame + iDT; // added by Sharp (98/2/12)
				if(iRefInterval>1)
				{
					Bool bCachedBVOP = FALSE; // added by Sharp (98/11/11)
//						Int iBFrame;  // deleted by Sharp (98/2/12)
					for(iBFrame = iRefFrame + iDT; iBFrame < iSearchFrame; iBFrame += iDT)
					{
						if(rgpvoenc [BASE_LAYER] -> skipTest(iBFrame,BVOP))
							continue;
//OBSS_SAIT_991015
//						if(m_rgbSpatialScalability [iVOrelative] && (volmd.fAUsage == ONE_BIT))                            //OBSSFIX_V2-8_before
						if(m_rgbSpatialScalability [iVOrelative] && (volmd.fAUsage == ONE_BIT) && !bTemporalScalability)   //OBSSFIX_V2-8_after
							bObjectExists = loadDataSpriteCheck(iVOrelative,iBFrame, pfYuvSrc, pfSegSrcSpatialEnhn, pxlcObjColor, rgpvoenc [BASE_LAYER]->m_pvopcOrig, volmd,volmd_enhn);
						else
//~OBSS_SAIT_991015
						bObjectExists = loadDataSpriteCheck(iVOrelative,iBFrame, pfYuvSrc, pfSegSrc, ppfAuxSrc, pxlcObjColor, rgpvoenc [BASE_LAYER]->m_pvopcOrig, volmd);
						encodeVideoObject (bObjectExists, bObjectExists, iBFrame, BVOP, bTemporalScalability ? DUMP_NONE: DUMP_CURR, // modified by Sharp (98/11/11)
										   iVO, iVOrelative, BASE_LAYER,
										   pfYuvSrc,pfSegSrc,ppfAuxSrc,
                       rgpfReconYUV,rgpfReconSeg,rgpfReconAux,
										   pxlcObjColor,rgpvoenc,volmd,rgpostrm);
						bCachedBVOP = bTemporalScalability ? TRUE : FALSE; // added by Sharp (98/11/11)
						if (m_rgbSpatialScalability [iVOrelative] && !bTemporalScalability) { // modified by Sharp (98/2/12)
							pvopcBaseQuant = rgpvoenc [BASE_LAYER]->pvopcReconCurr ();
//OBSS_SAIT_991015
							// Spatial Scalabe BVOP and PVOP
							VOPpredType PrevType = (rgpvoenc[ENHN_LAYER]->m_volmd.iSpatialOption == 0 )? BVOP: PVOP;	//for BVOP or PVOP enhn layer coding(OBSS) 						
							encodeVideoObject (bObjectExists, bObjectExists, iBFrame, PrevType, DUMP_CURR, 
											   iVO, iVOrelative, ENHN_LAYER,
											   pfYuvSrcSpatialEnhn, pfSegSrcSpatialEnhn, /*NULL*/ppfAuxSrc,		//OBSSFIX_MAC
                         rgpfReconYUV, rgpfReconSeg, /*NULL*/rgpfReconAux,										//OBSSFIX_MAC
											   pxlcObjColor, rgpvoenc, volmd, rgpostrm,
											   pvopcBaseQuant);
//~OBSS_SAIT_991015
						}
// begin: added by Sharp (98/2/12)
						else if (m_rgbSpatialScalability [iVOrelative] && bTemporalScalability)
							pBufB2->getBuf( rgpvoenc[BASE_LAYER] );
						
						if ( m_rgbSpatialScalability [iVOrelative] && bTemporalScalability ) {  // for TPS enhancement layer
							rgpvoenc [ENHN_LAYER] -> m_iBCount = 0;
							for (Int iEFrame = iBFrame - iDT + iDT_enhn; iEFrame < iBFrame; iEFrame += iDT_enhn ) {

								updateRefForTPS( rgpvoenc[ENHN_LAYER], pBufP1, pBufP2, pBufB1, pBufB2, pBufE,
									0, iVOrelative, iEcount, iBFrame-iDT+iDT_enhn, iEFrame, 0 );
								iEcount++;
								encodeEnhanceVideoObject(bObjectExists, iEFrame, rgpvoenc[ENHN_LAYER]->m_vopmd.vopPredType, DUMP_CURR,
									iVO,iVOrelative, pfYuvSrc,pfSegSrc,rgpfReconYUV,rgpfReconSeg,
									pxlcObjColor,rgpvoenc[ENHN_LAYER],volmd, volmd_enhn, iBFrame - iDT + iDT_enhn, rgpostrm,
									*pBufP1, *pBufP2, *pBufB1, *pBufB2, *pBufE
									);
							
								if ( !pBufB2->empty() ){
									if ( pBufB2 -> m_bCodedFutureRef == 1 ) // added by Sharp (99/1/28)
									pBufB1->copyBuf( *pBufB2 );
									pBufB2->dispose();
								}
							}
						}
// end: added by Sharp (98/2/12)

// begin: added by Sharp (98/11/11)
						if(bCachedBVOP && (m_rguiSpriteUsage [iVOrelative] == 0 || m_rguiSpriteUsage [iVOrelative] == 2)) // GMC
						{
							// only for temporal scalability
#ifndef __OUT_ONE_FRAME_
							// last ref frame needs to be output
							dumpData (rgpfReconYUV [BASE_LAYER], rgpfReconSeg [BASE_LAYER],
                rgpfReconAux [BASE_LAYER], BASE_LAYER,
								rgpvoenc[BASE_LAYER] ->pvopcReconCurr(), m_rctOrg, volmd, rgpvoenc[BASE_LAYER] ->m_vopmd.bInterlace);
							if (m_rgbSpatialScalability [iVOrelative] && !bTemporalScalability)
								dumpData (rgpfReconYUV [BASE_LAYER], rgpfReconSeg [BASE_LAYER],
                  rgpfReconAux [BASE_LAYER], BASE_LAYER,
									rgpvoenc[BASE_LAYER] ->pvopcReconCurr(), m_rctOrgSpatialEnhn, volmd, rgpvoenc[BASE_LAYER] ->m_vopmd.bInterlace);
#endif
							bCachedBVOP = FALSE;
						}
// end: added by Sharp (98/11/11)

					}
				}

				if (m_rgbSpatialScalability [iVOrelative] && !bTemporalScalability) { // modified by Sharp (98/2/12)
/* (98/3/30) modified by SONY*/
					if (iPCount == volmd.iPbetweenI)        {
/* (98/3/20) modified by SONY(end)*/
/*            ORIGINAL
					if (iPCount == 0)       {
					*/
//OBSS_SAIT_991015
//for OBSS_BASE_BVOP : stack out	
//OBSSFIX_MODE3
						if(rgpvoenc[ENHN_LAYER]->volmd().fAUsage == ONE_BIT &&
						   !(rgpvoenc[ENHN_LAYER]->volmd().iEnhnType != 0 && rgpvoenc[ENHN_LAYER]->volmd().iuseRefShape == 1)) {
//						if(rgpvoenc[ENHN_LAYER]->volmd().fAUsage == ONE_BIT) {
//~OBSSFIX_MODE3
							pBase_tmp_Baseshpmd =  (rgpvoenc[BASE_LAYER] -> m_rgBaseshpmd);
							rgpvoenc[BASE_LAYER] -> m_rgBaseshpmd = pBase_stack_Baseshpmd;
							pBase_tmp_mvBaseBY = (rgpvoenc[BASE_LAYER] -> m_rgmvBaseBY);
							rgpvoenc[BASE_LAYER] -> m_rgmvBaseBY = pBase_stack_mvBaseBY;

							rgpvoenc[BASE_LAYER] -> m_iNumMBX = iBase_stack_x;
							rgpvoenc[BASE_LAYER] -> m_iNumMBY =iBase_stack_y;
							rgpvoenc[BASE_LAYER] -> m_rctBase = pBase_stack_rctBase;
						} 
//~for OBSS_BASE_BVOP : stack out	
					encodeVideoObject(TRUE, TRUE, iSearchFrame, PVOP,
										  DUMP_CURR, // sony
										  iVO, iVOrelative, ENHN_LAYER, 
										  pfYuvSrcSpatialEnhn, pfSegSrcSpatialEnhn, /*NULL*/ppfAuxSrc,	//OBSSFIX_MAC
                      rgpfReconYUV, rgpfReconSeg, /*NULL*/rgpfReconAux,	
										  pxlcObjColor,rgpvoenc,volmd,rgpostrm,
										  pvopcBasePVOPQuant);
//for OBSS_BASE_BVOP :  stack free	
//OBSSFIX_MODE3
					if(rgpvoenc[ENHN_LAYER]->volmd().fAUsage == ONE_BIT &&
					  !(rgpvoenc[ENHN_LAYER]->volmd().iEnhnType != 0 && rgpvoenc[ENHN_LAYER]->volmd().iuseRefShape == 1)) {
//					if(rgpvoenc[ENHN_LAYER]->volmd().fAUsage == ONE_BIT) {
//~OBSSFIX_MODE3
							delete [] pBase_stack_Baseshpmd;
							pBase_stack_Baseshpmd = NULL;
							rgpvoenc[BASE_LAYER] -> m_rgBaseshpmd = pBase_tmp_Baseshpmd;
							delete [] pBase_stack_mvBaseBY;
							pBase_stack_mvBaseBY = NULL;
							rgpvoenc[BASE_LAYER] -> m_rgmvBaseBY = pBase_tmp_mvBaseBY;
						} 
//~for OBSS_BASE_BVOP :  stack free	
//~OBSS_SAIT_991015					
					}
					else {
						VOPpredType PrevType = (rgpvoenc[ENHN_LAYER]->m_volmd.iSpatialOption == 0 )? BVOP: PVOP;
//OBSS_SAIT_991015
						//for OBSS BASE_BVOP : stack out
//OBSSFIX_MODE3
						if(rgpvoenc[ENHN_LAYER]->volmd().fAUsage == ONE_BIT &&
							!(rgpvoenc[ENHN_LAYER]->volmd().iEnhnType != 0 && rgpvoenc[ENHN_LAYER]->volmd().iuseRefShape == 1)) {
//						if(rgpvoenc[ENHN_LAYER]->volmd().fAUsage == ONE_BIT) {
//~OBSSFIX_MODE3
							pBase_tmp_Baseshpmd =  (rgpvoenc[BASE_LAYER] -> m_rgBaseshpmd);
							rgpvoenc[BASE_LAYER] -> m_rgBaseshpmd = pBase_stack_Baseshpmd;
							pBase_tmp_mvBaseBY = (rgpvoenc[BASE_LAYER] -> m_rgmvBaseBY);
							rgpvoenc[BASE_LAYER] -> m_rgmvBaseBY = pBase_stack_mvBaseBY;

							rgpvoenc[BASE_LAYER] -> m_iNumMBX = iBase_stack_x;
							rgpvoenc[BASE_LAYER] -> m_iNumMBY =iBase_stack_y;
							rgpvoenc[BASE_LAYER] -> m_rctBase = pBase_stack_rctBase;
						} 
						encodeVideoObject (bObjectExists, bObjectExists, iSearchFrame, PrevType,
										   DUMP_CURR, // sony
										   iVO, iVOrelative, ENHN_LAYER, 
										   pfYuvSrcSpatialEnhn, pfSegSrcSpatialEnhn, /*NULL*/ppfAuxSrc,		//OBSSFIX_MAC
                       rgpfReconYUV, rgpfReconSeg, /*NULL*/rgpfReconAux,	
										   pxlcObjColor, rgpvoenc, volmd, rgpostrm,
										   pvopcBasePVOPQuant);
						//for OBSS BASE_BVOP : stack free
//OBSSFIX_MODE3
						if(rgpvoenc[ENHN_LAYER]->volmd().fAUsage == ONE_BIT &&
						 !(rgpvoenc[ENHN_LAYER]->volmd().iEnhnType != 0 && rgpvoenc[ENHN_LAYER]->volmd().iuseRefShape == 1)) {
//						if(rgpvoenc[ENHN_LAYER]->volmd().fAUsage == ONE_BIT) {
//~OBSSFIX_MODE3
							delete [] pBase_stack_Baseshpmd;
							pBase_stack_Baseshpmd = NULL;
							rgpvoenc[BASE_LAYER] -> m_rgBaseshpmd = pBase_tmp_Baseshpmd;
							delete [] pBase_stack_mvBaseBY;
							pBase_stack_mvBaseBY = NULL;
							rgpvoenc[BASE_LAYER] -> m_rgmvBaseBY = pBase_tmp_mvBaseBY;
						} 
//~OBSS_SAIT_991015
					}
					delete pvopcBasePVOPQuant;
				}
// begin: added by Sharp (98/2/12)
				else if ( m_rgbSpatialScalability [iVOrelative] && bTemporalScalability ){  // loop for TPS enhancement layer
					rgpvoenc [ENHN_LAYER] -> m_iBCount = 0;
					for (Int iEFrame = iSearchFrame - iDT + iDT_enhn; iEFrame < iSearchFrame; iEFrame += iDT_enhn ) {

					updateRefForTPS( rgpvoenc[ENHN_LAYER], pBufP1, pBufP2, pBufB1, pBufB2, pBufE,
						0, iVOrelative, iEcount, iBFrame-iDT+iDT_enhn, iEFrame, 0 );

					iEcount++;
					encodeEnhanceVideoObject(bObjectExists, iEFrame, rgpvoenc[ENHN_LAYER]->m_vopmd.vopPredType, DUMP_CURR,
						iVO,iVOrelative,pfYuvSrc,pfSegSrc,rgpfReconYUV,rgpfReconSeg,
						pxlcObjColor,rgpvoenc[ENHN_LAYER],volmd, volmd_enhn, iSearchFrame - iDT + iDT_enhn, rgpostrm,
						*pBufP1, *pBufP2, *pBufB1, *pBufB2, *pBufE
						);
				}
				pBufB1->dispose();
				}
// end: added by Sharp (98/2/12)
				// move onwards
				iRefFrame = iSearchFrame;
			}
		}

		if(bCachedRefDump && (m_rguiSpriteUsage [iVOrelative] == 0 || m_rguiSpriteUsage [iVOrelative] == 2)) // GMC
		{
			// last ref frame needs to be output
#ifndef __OUT_ONE_FRAME_
			if ( bCachedRefCoded )
				dumpData (rgpfReconYUV [BASE_LAYER], rgpfReconSeg [BASE_LAYER], rgpfReconAux [BASE_LAYER], BASE_LAYER,
				rgpvoenc[BASE_LAYER] ->pvopcRefQLater(), m_rctOrg, volmd, rgpvoenc[BASE_LAYER] ->m_vopmd.bInterlace);
			else
				dumpDataNonCoded(rgpfReconYUV [BASE_LAYER], rgpfReconSeg [BASE_LAYER],
					rgpfReconAux [BASE_LAYER], BASE_LAYER, m_rctOrg, volmd);
			
			bCachedRefDump = FALSE; // added by Sharp (98/11/11)
#endif
		}

// NEWPRED
		if (m_rgvolmd [BASE_LAYER][iVOrelative].bNewpredEnable) 
			delete g_pNewPredEnc;
// ~NEWPRED

		cout << "\nBASE VOL " << iVO << "\n";
		CStatistics sts = rgpvoenc [BASE_LAYER] -> statVOL ();
#ifdef __TRACE_AND_STATS_
		sts.print (TRUE); // dumping statistics
#endif // __TRACE_AND_STATS_
		delete rgpvoenc [BASE_LAYER];
		fclose (rgpfReconYUV [BASE_LAYER]);
		if (m_rgvolmd [BASE_LAYER] [iVOrelative].fAUsage != RECTANGLE)	{
			fclose (rgpfReconSeg [BASE_LAYER]);
		}
		if (volmd.fAUsage == EIGHT_BIT) {   // MAC (SB) 24-Nov-99
			for(Int iAuxComp=0; iAuxComp<volmd.iAuxCompCount; iAuxComp++) {
				if(volmd.iAlphaShapeExtension >= 0)
					fclose (ppfAuxSrc[iAuxComp]);
				fclose (rgpfReconAux[0][iAuxComp]);
				//fclose (rgpfReconAux[1][iAuxComp]); //not needed
			}
		}
		delete ppfAuxSrc;
		delete rgpfReconAux[0];
		delete rgpfReconAux[1];
		delete rgpostrm [BASE_LAYER];
		delete rgpostrmTrace [BASE_LAYER];

		if (m_rgbSpatialScalability [iVOrelative] == TRUE)	{
			cout << "\nEHNANCED VOL " << iVO << "\n";
			sts = rgpvoenc [ENHN_LAYER] -> statVOL ();
#ifdef __TRACE_AND_STATS_
			sts.print (TRUE); // dumping statistics
#endif // __TRACE_AND_STATS_
			delete rgpvoenc [ENHN_LAYER];
			fclose (rgpfReconYUV [ENHN_LAYER]);
			if (m_rgvolmd [BASE_LAYER] [iVOrelative].fAUsage != RECTANGLE)	{
				fclose (rgpfReconSeg [ENHN_LAYER]);
			}
			delete rgpostrm [ENHN_LAYER];
			delete rgpostrmTrace [ENHN_LAYER];
		}
	} // VO loop
}

Bool CSessionEncoder::loadDataSpriteCheck(UInt iVOrelative,UInt iFrame, FILE* pfYuvSrc, FILE* pfSegSrc, FILE** ppfAuxSrc, PixelC pxlcObjColor, CVOPU8YUVBA* pvopcDst, const VOLMode& volmd)
{
	Bool bObjectExists = TRUE;
	if(m_rguiSpriteUsage [iVOrelative] == 0 || m_rguiSpriteUsage [iVOrelative] == 2) // GMC
		bObjectExists = loadData (iFrame,pfYuvSrc,pfSegSrc,ppfAuxSrc,pxlcObjColor, pvopcDst, m_rctOrg, volmd);
	return bObjectExists;
}

//OBSS_SAIT_991015
Bool CSessionEncoder::loadDataSpriteCheck(UInt iVOrelative,UInt iFrame, FILE* pfYuvSrc, FILE* pfSegSrc, PixelC pxlcObjColor, CVOPU8YUVBA* pvopcDst, const VOLMode& volmd,const VOLMode& volmd_enhn)
{
	Bool bObjectExists = TRUE;
	if(m_rguiSpriteUsage [iVOrelative] == 0)
		bObjectExists = loadData (iFrame,pfYuvSrc,pfSegSrc, pxlcObjColor, pvopcDst, m_rctOrg, volmd, volmd_enhn);
	return bObjectExists;
}
//~OBSS_SAIT_991015

Void CSessionEncoder::encodeVideoObject(Bool bObjectExists,
										Bool bPrevObjectExists,
										Int iFrame,
										VOPpredType predType,
										Int iDumpMode,
										Int iVO,
										Int iVOrelative,
										Int iLayer,
										FILE* pfYuvSrc,
										FILE* pfSegSrc,
                    FILE** ppfAuxSrc,
										FILE* rgpfReconYUV[],
										FILE* rgpfReconSeg[],
                    FILE** rgpfReconAux[2],
										PixelC pxlcObjColor,
										CVideoObjectEncoder** rgpvoenc,
										const VOLMode& volmd,
										ofstream* rgpostrm[],
										const CVOPU8YUVBA* pvopcBaseQuant)
{
	CRct rctOrg;
	Bool BGComposition = FALSE;	//OBSS_SAIT_991015		//for OBSS partial enhancement mode
	if (m_rguiSpriteUsage [iVOrelative] == 0 || m_rguiSpriteUsage [iVOrelative] == 2) { // GMC
        rctOrg = m_rctOrg;
		if (iLayer == ENHN_LAYER) { 
			rctOrg = m_rctOrgSpatialEnhn;
			if((rgpvoenc [iLayer] -> skipTest((Time)iFrame,predType)))  // rate control
				return;
//OBSSFIX_MODE3
			if(iLayer == ENHN_LAYER && rgpvoenc[iLayer]->m_volmd.bSpatialScalability && rgpvoenc[iLayer]->m_volmd.iHierarchyType == 0)
				bObjectExists = loadData(iFrame, pfYuvSrc, pfSegSrc, ppfAuxSrc, pxlcObjColor, rgpvoenc [iLayer]->m_pvopcOrig, rctOrg, rgpvoenc[iLayer]->m_volmd);
			else
				bObjectExists = loadData(iFrame, pfYuvSrc, pfSegSrc, ppfAuxSrc, pxlcObjColor, rgpvoenc [iLayer]->m_pvopcOrig, rctOrg, volmd);
//			bObjectExists = loadData(iFrame, pfYuvSrc, pfSegSrc, ppfAuxSrc, pxlcObjColor, rgpvoenc [iLayer]->m_pvopcOrig, rctOrg, volmd);
//~OBSSFIX_MODE3
//OBSS_SAIT_991015
			if(volmd.bSpatialScalability) {
				rgpvoenc[ENHN_LAYER] -> m_rgBaseshpmd = rgpvoenc[BASE_LAYER] -> m_rgBaseshpmd;
				rgpvoenc[ENHN_LAYER] -> m_rgmvBaseBY = rgpvoenc[BASE_LAYER] -> m_rgmvBaseBY;
				rgpvoenc[ENHN_LAYER] -> m_iNumMBBaseXRef = rgpvoenc[BASE_LAYER] -> m_iNumMBX;
				rgpvoenc[ENHN_LAYER] -> m_iNumMBBaseYRef = rgpvoenc[BASE_LAYER] -> m_iNumMBY;
				rgpvoenc[ENHN_LAYER] -> m_rctBase = rgpvoenc[BASE_LAYER] -> m_rctBase;
				rgpvoenc[ENHN_LAYER] -> m_bCodedBaseRef = rgpvoenc[BASE_LAYER] -> m_bCodedFutureRef;	
			}
//~OBSS_SAIT_991015
		}
		rgpvoenc [iLayer] -> encode (bObjectExists, (Time) (iFrame - m_iFirstFrame), predType, pvopcBaseQuant);
//OBSS_SAIT_991015
		// for Spatial Scalability composition(OBSS partial enhancement mode)
//OBSSFIX_MODE3
		if(iLayer == ENHN_LAYER && rgpvoenc[ENHN_LAYER] -> m_volmd.iEnhnType == 1 ) {
//		if(iLayer == ENHN_LAYER && rgpvoenc[ENHN_LAYER] -> m_volmd.iEnhnTypeSpatial == 1 && rgpvoenc[ENHN_LAYER] -> m_volmd.iuseRefShape == 0) {
//~OBSSFIX_MODE3
			if (predType != BVOP)
				BGComposition = rgpvoenc[ENHN_LAYER] -> BackgroundComposition(rctOrg.width, rctOrg.height (),
					iFrame, rgpvoenc[ENHN_LAYER] ->m_pvopcRefQ0, m_pchReconYUVDir, iVO, m_pchPrefix,
					rgpfReconYUV[iLayer],rgpfReconSeg[iLayer]); 
			else 
				BGComposition = rgpvoenc[ENHN_LAYER] -> BackgroundComposition(rctOrg.width, rctOrg.height (),
					iFrame, rgpvoenc[ENHN_LAYER]->m_pvopcRefQ1, m_pchReconYUVDir, iVO, m_pchPrefix,
					rgpfReconYUV[iLayer],rgpfReconSeg[iLayer]); 
		}
//~OBSS_SAIT_991015
		if (iLayer == ENHN_LAYER)
			rgpvoenc [iLayer]-> swapSpatialScalabilityBVOP ();
	}
	else {
        rctOrg = m_rctFrame;
		CRct rctWarp = (m_rgNumOfPnts [iVO - m_iFirstVO] > 0)? 
                        findBoundBoxInAlpha (iFrame, iVO) : m_rctOrg;
		rgpvoenc [BASE_LAYER]->encodeSptTrajectory (iFrame, m_pppstDst [iVO - m_iFirstVO] [iFrame - m_iFirstFrame], rctWarp);
		bObjectExists = TRUE;
	}
	//dump the output

// begin: added by Sharp (99/1/28)
#ifndef __OUT_ONE_FRAME_
	if(iDumpMode==DUMP_CURR || m_rguiSpriteUsage [iVOrelative] == 1) // GMC
	{
//OBSS_SAIT_991015	//for OBSS partial enhancement mode
		if (bObjectExists) {
//OBSSFIX_MODE3
			if(!(iLayer == ENHN_LAYER && rgpvoenc[ENHN_LAYER] -> m_volmd.iEnhnType == 1 && BGComposition)) 
				dumpData (rgpfReconYUV [iLayer], rgpfReconSeg [iLayer], rgpfReconAux [iLayer], iLayer, rgpvoenc[iLayer] ->pvopcReconCurr(), rctOrg, rgpvoenc[iLayer]->m_volmd, rgpvoenc[iLayer] ->m_vopmd.bInterlace);
//			if(!(iLayer == ENHN_LAYER && rgpvoenc[ENHN_LAYER] -> m_volmd.iEnhnTypeSpatial == 1 && rgpvoenc[ENHN_LAYER] -> m_volmd.iuseRefShape == 0 && BGComposition)) 
//				dumpData (rgpfReconYUV [iLayer], rgpfReconSeg [iLayer], rgpfReconAux [iLayer], iLayer, rgpvoenc[iLayer] ->pvopcReconCurr(), rctOrg, volmd);
//~OBSSFIX_MODE3
		}
//~OBSS_SAIT_991015
		else
			dumpDataNonCoded(rgpfReconYUV [iLayer], rgpfReconSeg [iLayer],
				rgpfReconAux [iLayer], iLayer, rctOrg, volmd);
	}
	else if(iDumpMode==DUMP_PREV){ // dump previous reference frame
		if ( bPrevObjectExists )
				dumpData (rgpfReconYUV [iLayer], rgpfReconSeg [iLayer], rgpfReconAux [iLayer], iLayer, rgpvoenc[iLayer] ->pvopcRefQPrev(), rctOrg, volmd, rgpvoenc[iLayer] ->m_vopmd.bInterlace);
		else
				dumpDataNonCoded(rgpfReconYUV [iLayer], rgpfReconSeg [iLayer],
					rgpfReconAux [iLayer], iLayer, rctOrg, volmd);
	}
#else
	if (bObjectExists) 
		dumpDataOneFrame (iFrame, iVO, rgpvoenc[iLayer] ->pvopcReconCurr(), volmd); // save one frame
	else
		dumpDataNonCoded(rgpfReconYUV [iLayer], rgpfReconSeg [iLayer],
			rgpfReconAux [iLayer], iLayer, rctOrg, volmd);
#endif
// end: added by Sharp (99/1/28)

	rgpostrm [iLayer]->write (rgpvoenc [iLayer]->pOutStream ()->str (),
		rgpvoenc [iLayer]->pOutStream ()->pcount ());
}

Void CSessionEncoder::readPntFile (UInt iobj)
{
	Char pchtmp [100];
	sprintf (pchtmp, SUB_PNTFILE, m_pchSptPntDir, iobj, m_pchPrefix);
	FILE* pfPnt = fopen (pchtmp, "r");
	if(pfPnt==NULL)
	{
		char rgch[256];
		sprintf(rgch, "Can't open sprite point file: %s", pchtmp);
		fatal_error(rgch);
	}

	Int numPnt;
	Int iVOidx = iobj - m_iFirstVO;
	fscanf (pfPnt, "%d", &numPnt);
	assert (numPnt == m_rgNumOfPnts [iVOidx]);
	Int ifrF;
	Double dblX, dblY;
	for (Int ip = 0; ip < numPnt; ip++)
		fscanf (pfPnt, "%lf%lf", &m_ppstSrc [iVOidx] [ip].x, &m_ppstSrc [iVOidx] [ip].y); // not used
	while (fscanf (pfPnt, "%d", &ifrF) != EOF) {
		if ((ifrF >= m_iFirstFrame) && (ifrF <= m_iLastFrame)) {
			for (Int ip = 0; ip < numPnt; ip++)	{
				fscanf (pfPnt, "%lf %lf", &dblX, &dblY);
				Int iFrmIndex = ifrF - m_iFirstFrame;
				if (iFrmIndex % (m_rgvolmd [BASE_LAYER] [iVOidx].iTemporalRate) == 0)	{
					m_pppstDst [iVOidx] [iFrmIndex] [ip].x = dblX;
					m_pppstDst [iVOidx] [iFrmIndex] [ip].y = dblY;
				}
			}
		} else {
			for (Int ip = 0; ip < numPnt; ip++)	{
				fscanf (pfPnt, "%lf %lf", &dblX, &dblY);
			}
		}
	}
}

Void CSessionEncoder::loadSpt (UInt iobj, CVOPU8YUVBA* pvopcDst)
{
	Char pchSpt [100];
	sprintf (pchSpt, SUB_VDLFILE, m_pchSptDir, iobj, m_pchPrefix);

	Int iGrayToBinaryTH = m_rgvolmd[BASE_LAYER][iobj - m_iFirstVO].iGrayToBinaryTH;

	FILE* pf = fopen (pchSpt, "rb");
	// read overhead
	Int c0 = getc (pf);
	Int c1 = getc (pf);
	Int c2 = getc (pf);
	assert (c0 == 'S' && (c1 == 'P' || c2 == 'T') );
	CRct rctSpt;
	Bool bAUsage;
	fread (&rctSpt.left, sizeof (CoordI), 1, pf);
	fread (&rctSpt.top, sizeof (CoordI), 1, pf);
	fread (&rctSpt.right, sizeof (CoordI), 1, pf);
	fread (&rctSpt.bottom, sizeof (CoordI), 1, pf);
	fread (&bAUsage, sizeof (Int), 1, pf);
#ifndef _FOR_GSSP_
	assert (bAUsage != EIGHT_BIT); // sprite with Alpha channel is not supported at this moment
#endif
	Int iYDataHeight = m_rctOrg.height ();
	Int iUVDataHeight = m_rctOrg.height () / 2;
	Int iYFrmWidth = pvopcDst->whereY ().width;
	Int iUvFrmWidth = pvopcDst->whereUV ().width;
	Int nSkipYPixel = iYFrmWidth * EXPANDY_REF_FRAME + EXPANDY_REF_FRAME;
	Int nSkipUvPixel = iUvFrmWidth * EXPANDUV_REF_FRAME + EXPANDUV_REF_FRAME;
	PixelC* ppxlcY = (PixelC*) pvopcDst->pixelsY () + nSkipYPixel;
	PixelC* ppxlcU = (PixelC*) pvopcDst->pixelsU () + nSkipUvPixel;
	PixelC* ppxlcV = (PixelC*) pvopcDst->pixelsV () + nSkipUvPixel;
	
	CoordI y;
	for (y = 0; y < iYDataHeight; y++) {
		Int size = (Int) fread (ppxlcY, sizeof (U8), m_rctOrg.width, pf);
		if (size == 0)
		{
			fprintf (stderr, "Error!  Unexpected end of file while reading source sequence\n");
			exit(1);
		}
		ppxlcY += iYFrmWidth;
	}

	for (y = 0; y < iUVDataHeight; y++) {
		Int size = (Int) fread (ppxlcU, sizeof (U8), m_rctOrg.width / 2, pf);
		if (size == 0)
		{
			fprintf (stderr, "Error!  Unexpected end of file while reading source sequence\n");
			exit(1);
		}
		ppxlcU += iUvFrmWidth;
	}

	for (y = 0; y < iUVDataHeight; y++) {
		Int size = (Int) fread (ppxlcV, sizeof (U8), m_rctOrg.width / 2, pf);
		if (size == 0)
		{
			fprintf (stderr, "Error!  Unexpected end of file while reading source sequence\n");
			exit(1);
		}
		ppxlcV += iUvFrmWidth;
	}

	PixelC* ppxlcBY = (PixelC*) pvopcDst->pixelsBY () + nSkipYPixel;
	//	Int iObjectExist = 0;
	if ((bAUsage == ONE_BIT) && (pvopcDst -> fAUsage () == ONE_BIT)) { // load Alpha
		//binary
		for (CoordI y = 0; y < iYDataHeight; y++) {
			Int size = (Int) fread (ppxlcBY, sizeof (U8), m_rctOrg.width, pf);
			if (size == 0)
			{
				fprintf (stderr, "Error!  Unexpected end of file while reading source sequence\n");
				exit(1);
			}
			ppxlcBY += iYFrmWidth;
		}
		((CU8Image*) pvopcDst->getPlane (BUV_PLANE))->decimateBinaryShapeFrom (*(pvopcDst->getPlane (BY_PLANE)), 0);
	}
#ifdef _FOR_GSSP_
	else if ((bAUsage == EIGHT_BIT) && (pvopcDst -> fAUsage () == EIGHT_BIT)) { // load Alpha
		//grayscale alpha
		PixelC* ppxlcA = (PixelC*) pvopcDst->pixelsA (0) + nSkipYPixel;
		for (CoordI y = 0; y < iYDataHeight; y++) {
			for (CoordI x = 0; x < m_rctOrg.width; x++) {
				PixelC pxlcCurr = getc (pf);
				ppxlcA [x]  = (pxlcCurr >= iGrayToBinaryTH) ? pxlcCurr : transpValue;
				ppxlcBY [x] = (pxlcCurr >= iGrayToBinaryTH) ? opaqueValue : transpValue;
			}
			ppxlcBY += iYFrmWidth;
			ppxlcA  += iYFrmWidth;
		}
		((CU8Image*) pvopcDst->getPlane (BUV_PLANE))->decimateBinaryShapeFrom (*(pvopcDst->getPlane (BY_PLANE)), 0);
	}
#endif
}

Void CSessionEncoder::getInputFiles (FILE*& pfYuvSrc, 
                                     FILE*& pfAlpSrc, 
                                     FILE** ppfAuxSrc,
                                     FILE*& pfYuvSrcSpatialEnhn,
                                     FILE*& pfAlpSrcSpatialEnhn, 	//OBSS_SAIT_991015
                                     FILE* rgpfReconYUV [],
                                     FILE* rgpfReconSeg [], 
                                     FILE** rgpfReconAux [], 
                                     ofstream* rgpostrm [], 
                                     ofstream* rgpostrmTrace [],
                                     PixelC& pxlcObjColor, 
                                     Int iobj, 
                                     const VOLMode& volmd, 
                                     const VOLMode& volmd_enhn)
{
	static Char pchYUV [100], pchSeg [100], pchCmp [100], pchTrace [100], pchAux[100];

	//output files
	createCmpDir (iobj);
	sprintf (pchCmp, SUB_CMPFILE, m_pchOutStrFiles, iobj, m_pchPrefix);
	sprintf (pchTrace, SUB_TRCFILE, m_pchOutStrFiles, iobj, m_pchPrefix);
	rgpostrm [BASE_LAYER] = new ofstream (pchCmp, IOS_BINARY | ios::out);	
	rgpostrmTrace [BASE_LAYER] = NULL;
	if (volmd.bTrace)
		rgpostrmTrace [BASE_LAYER] = new ofstream (pchTrace);

	// prepare for the reconstructed files
	createReconDir (iobj); // create a directory for the reconstructed YUV in case it doesn't exist
	sprintf (pchYUV, SUB_YUVFILE, m_pchReconYUVDir, iobj, m_pchPrefix);
	sprintf (pchSeg, SUB_SEGFILE, m_pchReconYUVDir, iobj, m_pchPrefix);

	rgpfReconSeg [BASE_LAYER] = NULL;
	rgpfReconSeg [ENHN_LAYER] = NULL;

	if((rgpfReconYUV [BASE_LAYER] = fopen (pchYUV, "wb"))==NULL) // reconstructed YUV file
	{
		fprintf (stderr, "can't open %s\n", pchYUV);
		exit (1);
	}

	if(volmd.fAUsage != RECTANGLE)	{
		if((rgpfReconSeg [BASE_LAYER] = fopen (pchSeg, "wb"))==NULL) // reconstructed seg file
		{
			fprintf (stderr, "can't open %s\n", pchSeg);
			exit (1);
		}
	}

	// open alpha plane reconstructed files
	if (volmd.fAUsage == EIGHT_BIT) {   // MAC (SB) 24-Nov-99
		for(Int iAuxComp=0; iAuxComp<volmd.iAuxCompCount; iAuxComp++) {
			sprintf (pchAux, SUB_AUXFILE, m_pchReconYUVDir, iobj, m_pchPrefix, iAuxComp);  
			rgpfReconAux [BASE_LAYER][iAuxComp] = fopen (pchAux, "wb"); // reconstructed aux files
			if(rgpfReconAux [BASE_LAYER][iAuxComp] == NULL)
			{
				fprintf (stderr, "can't open %s\n", pchAux);
				exit (1);
			}
		}
	}

//OBSS_SAIT_991015
	if (!m_bTexturePerVOP) {
		sprintf (pchYUV, ROOT_YUVFILE, m_pchBmpFiles, m_pchPrefix);
		if (volmd.fAUsage != RECTANGLE)	
			sprintf (pchSeg, ROOT_SEGFILE, m_pchBmpFiles, m_pchPrefix);	
	}
	else {
		sprintf (pchYUV, SUB_YUVFILE, m_pchBmpFiles, iobj, m_pchPrefix);
		if (volmd.fAUsage != RECTANGLE)	
			sprintf (pchSeg, SUB_SEGFILE, m_pchBmpFiles, iobj, m_pchPrefix);	
	}
//~OBSS_SAIT_991015
	pfYuvSrc = fopen (pchYUV, "rb");
	if (pfYuvSrc == NULL)	{
		fprintf (stderr, "can't open %s\n", pchYUV);
		exit (1);
	}
//OBSS_SAIT_991015
	if (volmd.fAUsage != RECTANGLE)	{
		pfAlpSrc = fopen (pchSeg, "rb");
		if (pfAlpSrc == NULL)	{
			fprintf (stderr, "can't open %s\n", pchSeg);
			exit (1);
		}
	}
//~OBSS_SAIT_991015
  
	// open alpha plane source files if MAC is enabled
	if (volmd.fAUsage == EIGHT_BIT && volmd.iAlphaShapeExtension >= 0) {   // MAC (SB) 24-Nov-99
		for(Int iAuxComp=0; iAuxComp<volmd.iAuxCompCount; iAuxComp++) {
			sprintf (pchAux, ROOT_AUXFILE, m_pchBmpFiles, m_pchPrefix, iAuxComp );
			ppfAuxSrc[iAuxComp] = fopen (pchAux, "rb");
			if (ppfAuxSrc[iAuxComp] == NULL)	{
				fprintf (stderr, "can't open %s\n", pchAux);
				exit (1);
			}
		}
	}

	if (m_rgbSpatialScalability [iobj - m_iFirstVO] == TRUE)	{
		sprintf (pchCmp, ENHN_SUB_CMPFILE, m_pchOutStrFiles, iobj, m_pchPrefix);
		sprintf (pchTrace, ENHN_SUB_TRCFILE, m_pchOutStrFiles, iobj, m_pchPrefix);
		rgpostrm [ENHN_LAYER] = new ofstream (pchCmp, IOS_BINARY | ios::out);
		rgpostrmTrace [ENHN_LAYER] = NULL;
		if (volmd_enhn.bTrace) // modified by Sharp (98/2/12)
			rgpostrmTrace [ENHN_LAYER] = new ofstream (pchTrace);

// begin: added by Sharp (98/10/26)
		if ( volmd_enhn.bTemporalScalability == TRUE ){
			rgpfReconYUV [ENHN_LAYER] = rgpfReconYUV[BASE_LAYER];
			if (volmd_enhn.fAUsage != RECTANGLE)	{
				if ( volmd_enhn.iEnhnType == 0 )
					rgpfReconSeg [ENHN_LAYER] = rgpfReconSeg[BASE_LAYER];
				else{
					sprintf (pchSeg, ENHN_SUB_SEGFILE, m_pchReconYUVDir, iobj, m_pchPrefix);
					if((rgpfReconSeg [ENHN_LAYER] = fopen (pchSeg, "wb"))==NULL) // reconstructed seg file
					{	
						fprintf (stderr, "can't open %s\n", pchSeg);
						exit (1);
					}
				}
			}
		} else {
// end: added by Sharp (98/10/26)
			sprintf (pchYUV, ENHN_SUB_YUVFILE, m_pchReconYUVDir, iobj, m_pchPrefix);
			sprintf (pchSeg, ENHN_SUB_SEGFILE, m_pchReconYUVDir, iobj, m_pchPrefix);
			rgpfReconYUV [ENHN_LAYER] = fopen (pchYUV, "wb");		// reconstructed YUV file
			if (volmd_enhn.fAUsage != RECTANGLE)	{ // modified by Sharp (98/2/12)
				if((rgpfReconSeg [ENHN_LAYER] = fopen (pchSeg, "wb"))==NULL) // reconstructed seg file
				{	
					fprintf (stderr, "can't open %s\n", pchSeg);
					exit (1);
				}
			}
		} // added by Sharp (98/10/26)

		if ( volmd_enhn.bTemporalScalability == FALSE ){ // added by Sharp (98/2/10)
			if (!m_bTexturePerVOP) { 
				fprintf (stderr,"m_bTexturePerVOP != 0 is not applyed for spatial scalable coding\n");
			}
	//OBSS_SAIT_991015
			else {
				sprintf (pchYUV, ENHN_SUB_YUVFILE, m_pchBmpFiles, iobj, m_pchPrefix);
//OBSSFIX_MODE3
				if (volmd.fAUsage != RECTANGLE || (volmd_enhn.bSpatialScalability == TRUE && volmd_enhn.fAUsage != RECTANGLE))
//				if (volmd.fAUsage != RECTANGLE)
//~OBSSFIX_MODE3
					sprintf (pchSeg, ENHN_SUB_SEGFILE, m_pchBmpFiles, iobj, m_pchPrefix);	
			}
	//~OBSS_SAIT_991015
			pfYuvSrcSpatialEnhn = fopen (pchYUV, "rb");
			if (pfYuvSrcSpatialEnhn == NULL){
				fprintf (stderr,"can't open %s\n", pchYUV);
				exit(1);
			}
	//OBSS_SAIT_991015
//OBSSFIX_MODE3
			if (volmd.fAUsage != RECTANGLE || (volmd_enhn.bSpatialScalability == TRUE && volmd_enhn.fAUsage != RECTANGLE)) {
//			if (volmd.fAUsage != RECTANGLE) {
//~OBSSFIX_MODE3
				pfAlpSrcSpatialEnhn = fopen (pchSeg, "rb");						
				if (pfAlpSrcSpatialEnhn == NULL){								
					fprintf (stderr,"can't open %s\n", pchSeg);					
					exit(1);													
				}		
			}
	//~OBSS_SAIT_991015
		} // added by Sharp (98/2/10)
	}

	if (volmd.fAUsage != RECTANGLE ||
		m_rgbSpatialScalability [iobj - m_iFirstVO] == TRUE && volmd_enhn.fAUsage != RECTANGLE) {
		// load Alpha // modified by Sharp (98/2/12)
		if (!m_bAlphaPerVOP) {
			pxlcObjColor = iobj;
			sprintf (pchSeg, ROOT_SEGFILE, m_pchBmpFiles, m_pchPrefix);
		}
		else	{
			pxlcObjColor = opaqueValue;		  // out data notation
			sprintf (pchSeg, SUB_SEGFILE, m_pchBmpFiles, iobj, m_pchPrefix);
		}
		pfAlpSrc = fopen (pchSeg, "rb");
		if (pfAlpSrc == NULL)	{
			fprintf (stderr, "can't open %s\n", pchSeg);
			exit (1);
		}
	}
}

Void CSessionEncoder::initVOEncoder (CVideoObjectEncoder** rgpvoenc, Int iobj, ofstream* rgpostrmTrace [])
{
#if 0
	Int iVOidx = iobj - m_iFirstVO;
	Bool bTemporalScalability = m_rgvolmd[BASE_LAYER][iVOidx].bTemporalScalability; // added by Sharp (98/2/10)
//	Bool bTemporalScalability = TRUE; // added by Sharp (98/2/10)
	if (m_rgbSpatialScalability [iVOidx] == TRUE)	{
		rgpvoenc [BASE_LAYER] = new CVideoObjectEncoder (
								 iobj, 
								 m_rgvolmd [BASE_LAYER] [iVOidx], 
								 m_rgvopmd [BASE_LAYER] [iVOidx], 
								 m_iFirstFrame,
								 m_iLastFrame,
								 m_rctOrg.width,
								 m_rctOrg.height(),
								 m_rguiRateControl [BASE_LAYER] [iVOidx],
								 m_rguiBudget [BASE_LAYER] [iVOidx],
								 rgpostrmTrace [BASE_LAYER],
								 // GMC
								 m_rguiSpriteUsage [iVOidx],
								 // ~GMC
								 m_rguiWarpingAccuracy [iVOidx],
								 m_rgNumOfPnts [iVOidx],
								 m_pppstDst [iVOidx],
								 m_SptMode,
								 m_rctFrame,
								 m_rctOrg,
								 m_rgiMVFileUsage [BASE_LAYER][iVOidx],
								 m_pchMVFileName  [BASE_LAYER][iVOidx]);
// begin: added by Sharp (98/2/10)
		if ( bTemporalScalability )
			rgpvoenc [ENHN_LAYER] = new CVideoObjectEncoder (
														iobj, 
														m_rgvolmd [ENHN_LAYER] [iVOidx],
														m_rgvopmd [ENHN_LAYER] [iVOidx], 
														m_iFirstFrame,
														m_iLastFrame,
														m_rctOrg.width,
														m_rctOrg.height(),
														m_rguiRateControl [ENHN_LAYER] [iVOidx],
														m_rguiBudget [ENHN_LAYER] [iVOidx],
														rgpostrmTrace [ENHN_LAYER],
// GMC
														0,
// ~GMC
														0,
														-1,
                                                        m_pppstDst [iVOidx],
														m_SptMode,
														m_rctFrame,
														m_rctOrgSpatialEnhn,
														m_rgiMVFileUsage [ENHN_LAYER][iVOidx],
														m_pchMVFileName  [ENHN_LAYER][iVOidx]);
		else
// end: added by Sharp (98/2/10)
			rgpvoenc [ENHN_LAYER] = new CVideoObjectEncoder (
														iobj, 
														m_rgvolmd [ENHN_LAYER] [iVOidx],
														m_rgvopmd [ENHN_LAYER] [iVOidx], 
														m_iFirstFrame,
														m_iLastFrame,
														m_rctOrgSpatialEnhn.width,
														m_rctOrgSpatialEnhn.height (),
														m_rguiRateControl [ENHN_LAYER] [iVOidx],
														m_rguiBudget [ENHN_LAYER] [iVOidx],
														rgpostrmTrace [ENHN_LAYER],
// GMC
														0,
// ~GMC
														0,
														-1,
                                                        m_pppstDst [iVOidx],
														m_SptMode,
														m_rctFrame,
														m_rctOrgSpatialEnhn,
														m_rgiMVFileUsage [ENHN_LAYER][iVOidx],
														m_pchMVFileName  [ENHN_LAYER][iVOidx]);
	}
	else	{
		rgpvoenc [BASE_LAYER] = new CVideoObjectEncoder (
														iobj, 
														m_rgvolmd [BASE_LAYER] [iVOidx], 
														m_rgvopmd [BASE_LAYER] [iVOidx], 
														m_iFirstFrame,
														m_iLastFrame,
														m_rctOrg.width,
														m_rctOrg.height (),
														m_rguiRateControl [BASE_LAYER] [iVOidx],
														m_rguiBudget [BASE_LAYER] [iVOidx],
														rgpostrmTrace [BASE_LAYER],
// GMC
														m_rguiSpriteUsage [iVOidx],
// ~GMC
														m_rguiWarpingAccuracy [iVOidx],
														m_rgNumOfPnts [iVOidx],
														m_pppstDst [iVOidx],
														m_SptMode,
														m_rctFrame,
                                                        m_rctOrg,
														m_rgiMVFileUsage [BASE_LAYER][iVOidx],
														m_pchMVFileName  [BASE_LAYER][iVOidx]);
	}
#endif
}

Bool CSessionEncoder::loadData (UInt iFrame, FILE* pfYuvSrc, FILE* pfSegSrc, FILE** ppfAuxSrc,
                                PixelC pxlcObjColor, CVOPU8YUVBA* pvopcDst, CRct& rctOrg, const VOLMode& volmd)
{
	Int iLeadingPixels = iFrame * rctOrg.area ();
	if (volmd.nBits<=8) {
		fseek (pfYuvSrc, iLeadingPixels + iLeadingPixels / 2, SEEK_SET);	//4:2:0
	} else { // NBIT: 2 bytes per pixel, Y component plus UV
		fseek (pfYuvSrc, iLeadingPixels * 3, SEEK_SET);
	}
	if (volmd.fAUsage != RECTANGLE)
		fseek (pfSegSrc, iLeadingPixels, SEEK_SET);	

	if (volmd.fAUsage == EIGHT_BIT && volmd.iAlphaShapeExtension >= 0) { // MAC (SB) 24-Nov-99
		for(Int iAuxComp=0; iAuxComp<volmd.iAuxCompCount; iAuxComp++)
			fseek (ppfAuxSrc[iAuxComp], iLeadingPixels, SEEK_SET); 
	}

	Int iYDataHeight = rctOrg.height ();
	Int iUVDataHeight = rctOrg.height () / 2;
	Int iYFrmWidth = pvopcDst->whereY ().width;
	Int iUvFrmWidth = pvopcDst->whereUV ().width;
	Int nSkipYPixel = iYFrmWidth * EXPANDY_REF_FRAME + EXPANDY_REF_FRAME;
	Int nSkipUvPixel = iUvFrmWidth * EXPANDUV_REF_FRAME + EXPANDUV_REF_FRAME;
	PixelC* ppxlcY = (PixelC*) pvopcDst->pixelsY () + nSkipYPixel;
	PixelC* ppxlcU = (PixelC*) pvopcDst->pixelsU () + nSkipUvPixel;
	PixelC* ppxlcV = (PixelC*) pvopcDst->pixelsV () + nSkipUvPixel;

	CoordI y,x;
	for (y = 0; y < iYDataHeight; y++) {
		Int size = (Int) fread (ppxlcY, sizeof (PixelC), rctOrg.width, pfYuvSrc);
		if (size == 0)
		{
			fprintf (stderr, "Error!  Unexpected end of file while reading source sequence\n");
			exit(1);
		}
		ppxlcY += iYFrmWidth;
	}

 /* modified by Rockwell (98/05/08)
	if (volmd.nBits<=8) {*/
	for (y = 0; y < iUVDataHeight; y++) {
		Int size = (Int) fread (ppxlcU, sizeof (PixelC), rctOrg.width / 2, pfYuvSrc);
		if (size == 0)
		{
			fprintf (stderr, "Error!  Unexpected end of file while reading source sequence\n");
			exit(1);
		}
		ppxlcU += iUvFrmWidth;
	}

	for (y = 0; y < iUVDataHeight; y++) {
		Int size = (Int) fread (ppxlcV, sizeof (PixelC), rctOrg.width / 2, pfYuvSrc);
		if (size == 0)
		{
			fprintf (stderr, "Error!  Unexpected end of file while reading source sequence\n");
			exit(1);
		}
		ppxlcV += iUvFrmWidth;
	}
  /* modified by Rockwell (98/05/08)
  } else { // NBIT: fill UV space with default pixel value
	Int iDefval = 1<<(volmd.nBits-1);
	Int iUVDataWidth = rctOrg.width / 2;
	for (y = 0; y < iUVDataHeight; y++) {
		for (x = 0; x < iUVDataWidth; x++) {
			ppxlcU[x] = iDefval;
			ppxlcV[x] = iDefval;
		}
		ppxlcU += iUvFrmWidth;
		ppxlcV += iUvFrmWidth;
	}
    }
  */

	PixelC* ppxlcBY = (PixelC*) pvopcDst->pixelsBY () + nSkipYPixel;
	Int iObjectExist = 0;
	if (volmd.fAUsage == ONE_BIT || volmd.fAUsage==EIGHT_BIT) { // load Alpha
		//binary or single plane gray
		PixelC* ppxlcA = NULL;
		if(volmd.fAUsage==EIGHT_BIT && volmd.iAlphaShapeExtension < 0) // MAC turned off
			ppxlcA = (PixelC*) pvopcDst->pixelsA(0) + nSkipYPixel;
		for (y = 0; y < iYDataHeight; y++) {
			for (x = 0; x < rctOrg.width; x++) {
				PixelC pxlcCurr = getc (pfSegSrc);
				if (volmd.fAUsage==EIGHT_BIT)
				{
					ppxlcBY [x] = (pxlcCurr >= volmd.iGrayToBinaryTH) ? opaqueValue : transpValue;
					if(volmd.iAlphaShapeExtension < 0) // MAC disabled
					{
						ppxlcA [x]  = (pxlcCurr >= volmd.iGrayToBinaryTH) ? pxlcCurr : transpValue;
					}
				}
				else
  					ppxlcBY [x] = (pxlcCurr == pxlcObjColor) ? opaqueValue : transpValue;
				iObjectExist += ppxlcBY [x];
			}
			ppxlcBY += iYFrmWidth;
			ppxlcA  += iYFrmWidth;
		}
		((CU8Image*) pvopcDst->getPlane (BUV_PLANE))->decimateBinaryShapeFrom (*(pvopcDst->getPlane (BY_PLANE)), 0);
	} else                                                                                                        //rectangle
		iObjectExist = 1;
	
	if (volmd.fAUsage == EIGHT_BIT && volmd.iAlphaShapeExtension >= 0) { // load grayscale Alpha for MAC
		for(Int iAuxComp=0; iAuxComp<volmd.iAuxCompCount; iAuxComp++) { // MAC (SB) 24-Nov-1999
			PixelC* ppxlcA = (PixelC*) pvopcDst->pixelsA(iAuxComp) + nSkipYPixel;
			for (y = 0; y < iYDataHeight; y++) {
				for (x = 0; x < rctOrg.width; x++) {
					PixelC pxlcCurr = getc (ppfAuxSrc[iAuxComp]);
					ppxlcA [x]  = (pxlcCurr >= volmd.iGrayToBinaryTH) ? pxlcCurr : transpValue;
				}
				ppxlcA  += iYFrmWidth;
			}
		}
	}
	
	/*static int ini = 0;
	if(ini = 0)
	{
		srand(123421);
		ini = 1;
	}

	int k = rand();
	if((k%4)==0)
		iObjectExist = 0;*/

	return (iObjectExist != 0);
}

//OBSS_SAIT_991015
Bool CSessionEncoder::loadData (UInt iFrame, FILE* pfYuvSrc, FILE* pfSegSrc, PixelC pxlcObjColor, CVOPU8YUVBA* pvopcDst, CRct& rctOrg, const VOLMode& volmd, const VOLMode& volmd_enhn)
{
	Int iLeadingPixels = iFrame * rctOrg.area ();

	if (volmd.nBits<=8) {
		fseek (pfYuvSrc, iLeadingPixels + iLeadingPixels / 2, SEEK_SET);	//4:2:0
	} else { // NBIT: 2 bytes per pixel, Y component plus UV
		fseek (pfYuvSrc, iLeadingPixels * 3, SEEK_SET);
	}

	Int iYDataHeight = rctOrg.height ();
	Int iUVDataHeight = rctOrg.height () / 2;
	Int iYFrmWidth = pvopcDst->whereY ().width;
	Int iUvFrmWidth = pvopcDst->whereUV ().width;
	Int nSkipYPixel = iYFrmWidth * EXPANDY_REF_FRAME + EXPANDY_REF_FRAME;
	Int nSkipUvPixel = iUvFrmWidth * EXPANDUV_REF_FRAME + EXPANDUV_REF_FRAME;
	PixelC* ppxlcY = (PixelC*) pvopcDst->pixelsY () + nSkipYPixel;
	PixelC* ppxlcU = (PixelC*) pvopcDst->pixelsU () + nSkipUvPixel;
	PixelC* ppxlcV = (PixelC*) pvopcDst->pixelsV () + nSkipUvPixel;

	CoordI y,x;
	for (y = 0; y < iYDataHeight; y++) {
		Int size = (Int) fread (ppxlcY, sizeof (PixelC), rctOrg.width, pfYuvSrc);
		if (size == 0)
		{
			fprintf (stderr, "Error!  Unexpected end of file while reading source sequence\n");
			exit(1);
		}
		ppxlcY += iYFrmWidth;
	}

	for (y = 0; y < iUVDataHeight; y++) {
		Int size = (Int) fread (ppxlcU, sizeof (PixelC), rctOrg.width / 2, pfYuvSrc);
		if (size == 0)
		{
			fprintf (stderr, "Error!  Unexpected end of file while reading source sequence\n");
			exit(1);
		}
		ppxlcU += iUvFrmWidth;
	}

	for (y = 0; y < iUVDataHeight; y++) {
		Int size = (Int) fread (ppxlcV, sizeof (PixelC), rctOrg.width / 2, pfYuvSrc);
		if (size == 0)
		{
			fprintf (stderr, "Error!  Unexpected end of file while reading source sequence\n");
			exit(1);
		}
		ppxlcV += iUvFrmWidth;
	}
	Int iLeadingSegPixels = iFrame * volmd_enhn.iFrmWidth_SS * volmd_enhn.iFrmHeight_SS ;			 
								   
	if (volmd.fAUsage != RECTANGLE)
		fseek (pfSegSrc, iLeadingSegPixels, SEEK_SET);	
	
	Int iOrgBYFrmWidth = volmd_enhn.iFrmWidth_SS;	
	Int iBYFrmWidth = (pvopcDst->whereY ().width);																			
	Int nSkipBYPixel = iBYFrmWidth * EXPANDY_REF_FRAME + EXPANDY_REF_FRAME;													
	PixelC* ppxlcBY = (PixelC*) pvopcDst->pixelsBY () + nSkipBYPixel;
	Int iObjectExist = 0;

	if (volmd.fAUsage == ONE_BIT) { // load Alpha
		//binary
		if(!volmd.bSpatialScalability){
			for (y = 0; y < iYDataHeight; y++) {
				for (x = 0; x < rctOrg.width; x++) {
					PixelC pxlcCurr = getc (pfSegSrc);
					ppxlcBY [x] = (pxlcCurr == pxlcObjColor) ? opaqueValue : transpValue;
					iObjectExist += ppxlcBY [x];
				}
				ppxlcBY += iBYFrmWidth;
			}
		}
		else{
			int *HorizontalSamplingMatrix = NULL;
			int *VerticalSamplingMatrix = NULL;
			double h_factor = log((double)volmd_enhn.ihor_sampling_factor_n_shape/volmd_enhn.ihor_sampling_factor_m_shape)/log(2);
			int h_factor_int = (int)floor(h_factor+0.000001);
			if(h_factor - h_factor_int > 0.000001) {
				HorizontalSamplingMatrix = new int [volmd_enhn.ihor_sampling_factor_n_shape-volmd_enhn.ihor_sampling_factor_m_shape*(1<<h_factor_int)];
				double HorizontalSamplingFactor = (double)volmd_enhn.ihor_sampling_factor_n_shape/(volmd_enhn.ihor_sampling_factor_n_shape-volmd_enhn.ihor_sampling_factor_m_shape*(1<<h_factor_int));

				for (int i=1;i<=(volmd_enhn.ihor_sampling_factor_n_shape-volmd_enhn.ihor_sampling_factor_m_shape*(1<<h_factor_int));i++){
					HorizontalSamplingMatrix[i-1] = 
						((int)(volmd_enhn.ihor_sampling_factor_n_shape - HorizontalSamplingFactor*i))*(1<<h_factor_int)+(1<<h_factor_int)-1;		
				}

			}

			double v_factor = log((double)volmd_enhn.iver_sampling_factor_n_shape/volmd_enhn.iver_sampling_factor_m_shape)/log(2);
			int v_factor_int = (int)floor(v_factor+0.000001);
			if(v_factor - v_factor_int > 0.000001) {
				VerticalSamplingMatrix = new int [volmd_enhn.iver_sampling_factor_n_shape-volmd_enhn.iver_sampling_factor_m_shape*(1<<v_factor_int)];
				double VerticalSamplingFactor = (double)volmd_enhn.iver_sampling_factor_n_shape/(volmd_enhn.iver_sampling_factor_n_shape-volmd_enhn.iver_sampling_factor_m_shape*(1<<v_factor_int));
				for ( int i=1;i<=(volmd_enhn.iver_sampling_factor_n_shape-volmd_enhn.iver_sampling_factor_m_shape*(1<<v_factor_int));i++){
					VerticalSamplingMatrix[i-1] = 
						((int)(volmd_enhn.iver_sampling_factor_n_shape - VerticalSamplingFactor*i))*(1<<v_factor_int)+(1<<v_factor_int)-1;			
				}
			}
			int /* xx=0,*/ skip_pixel=0, skip_line=0, read_x =0;
			Int x_size = 0, y_size=0;
			for (y = 0; y < volmd_enhn.iFrmHeight_SS; y++) {	
				read_x = 0;
				do{
					skip_line = 0;
					if(v_factor - v_factor_int > 0.000001) {
						for( int i=0; i<(volmd_enhn.iver_sampling_factor_n_shape-volmd_enhn.iver_sampling_factor_m_shape*(1<<v_factor_int));i++){
							if (y%((1<<v_factor_int)*volmd_enhn.iver_sampling_factor_n_shape) == VerticalSamplingMatrix[i])
								skip_line = 1;
						}
					}
					if( (y%(1<<v_factor_int) != (1<<v_factor_int)-1) || skip_line == 1 ){
							fseek(pfSegSrc, iOrgBYFrmWidth, SEEK_CUR);
							y++;
					}
					else {
						read_x = 1;
						y_size++;
					}
				} while(read_x != 1);
				x_size = 0;
				for (x = 0; x < volmd_enhn.iFrmWidth_SS; x++) {    
					skip_pixel = 0;
					if(h_factor - h_factor_int > 0.000001) {
						for( int i=0; i<(volmd_enhn.ihor_sampling_factor_n_shape-volmd_enhn.ihor_sampling_factor_m_shape*(1<<h_factor_int));i++){
							if (HorizontalSamplingMatrix[i] == x%((1<<h_factor_int)*volmd_enhn.ihor_sampling_factor_n_shape))
								skip_pixel = 1;
						}
					}
					if( x% (1<<h_factor_int) == (1<<h_factor_int)-1 && skip_pixel == 0) {
						PixelC pxlcCurr = getc (pfSegSrc);
						ppxlcBY [x_size] = (pxlcCurr == pxlcObjColor) ? opaqueValue : transpValue;
						iObjectExist += ppxlcBY [x_size];
						x_size++;
					}
					else 	getc (pfSegSrc);
				}
				if(x_size%2 ==1) x_size--;						
				ppxlcBY += (x_size + EXPANDY_REF_FRAME*2);
			}
		}

		((CU8Image*) pvopcDst->getPlane (BUV_PLANE))->decimateBinaryShapeFrom (*(pvopcDst->getPlane (BY_PLANE)), 0);
	}
	else if (volmd.fAUsage == EIGHT_BIT) { // load Alpha
		//gray
		PixelC* ppxlcA = (PixelC*) pvopcDst->pixelsA(0) + nSkipYPixel;
		for (y = 0; y < iYDataHeight; y++) {
			for (x = 0; x < rctOrg.width; x++) {
				PixelC pxlcCurr = getc (pfSegSrc);
				ppxlcA [x]  = (pxlcCurr >= volmd.iGrayToBinaryTH) ? pxlcCurr : transpValue;
				ppxlcBY [x] = (pxlcCurr >= volmd.iGrayToBinaryTH) ? opaqueValue : transpValue;
				iObjectExist += ppxlcBY [x];
			}
			ppxlcBY += iYFrmWidth;
			ppxlcA  += iYFrmWidth;
		}
		((CU8Image*) pvopcDst->getPlane (BUV_PLANE))->decimateBinaryShapeFrom (*(pvopcDst->getPlane (BY_PLANE)), 0);
	}
	else  //rectangle
		iObjectExist = 1;
	return (iObjectExist != 0);
}
//~OBSS_SAIT_991015

CRct CSessionEncoder::findBoundBoxInAlpha (UInt ifr, UInt iobj)
{
	CIntImage* pfiARet = NULL;
	Char pchSeg [100];
	if (m_rgvolmd [BASE_LAYER] [iobj].fAUsage != RECTANGLE) { // load Alpha
		if (!m_bAlphaPerVOP) {
			sprintf (pchSeg, ROOT_SEGFILE, m_pchBmpFiles, m_pchPrefix);
			pfiARet = alphaFromCompFile (pchSeg, ifr, iobj, m_rctOrg);
		}
		else {
			sprintf (pchSeg, SUB_SEGFILE, m_pchBmpFiles, iobj, m_pchPrefix);
			pfiARet = new CIntImage (pchSeg, ifr, m_rctOrg);
			pfiARet -> threshold (64);
		}
		CRct rctBoundingBox = pfiARet -> whereVisible ();
		if (rctBoundingBox.left % 2 != 0)
			rctBoundingBox.left--;
		if (rctBoundingBox.top % 2 != 0)
			rctBoundingBox.top--;
		if (rctBoundingBox.right % 2 != 0)
			rctBoundingBox.right++;
		if (rctBoundingBox.bottom % 2 != 0)
			rctBoundingBox.bottom++;
		rctBoundingBox.width = rctBoundingBox.right - rctBoundingBox.left;
		delete pfiARet;
		return rctBoundingBox;
	}
	else
		return m_rctOrg;
}

CRct CSessionEncoder::findSptRct (UInt iobj)
{
	CRct rctSpt;
	Char pchSpt [100];
	sprintf (pchSpt, SUB_VDLFILE, m_pchSptDir, iobj, m_pchPrefix);
	FILE* pf = fopen (pchSpt, "rb");
	// read overhead
	Int c0 = getc (pf);
	Int c1 = getc (pf);
	Int c2 = getc (pf);
	assert (c0 == 'S' && (c1 == 'P' || c2 == 'T') );
	fread (&rctSpt.left, sizeof (CoordI), 1, pf);
	fread (&rctSpt.top, sizeof (CoordI), 1, pf);
	fread (&rctSpt.right, sizeof (CoordI), 1, pf);
	fread (&rctSpt.bottom, sizeof (CoordI), 1, pf);
	assert (rctSpt.left % 2 == 0);
	assert (rctSpt.top % 2 == 0);
	rctSpt.width = rctSpt.right - rctSpt.left;
	fclose (pf);
	return rctSpt;
}

Void CSessionEncoder::dumpDataNonCoded (FILE* pfYUV, FILE* pfSeg, FILE **ppfAux, Int iLayer,
										CRct& rct, const VOLMode& volmd)
{
	// in rectangular vop mode, non-coded frame is black if no frames preceeded it
	// otherwise you repeat the previous frame
	// in shaped vop mode, non-coded frame has zero binary plane, with black yuv

	const CVOPU8YUVBA* pvopcPrevDisp = m_rgpvopcPrevDisp[iLayer];

	if(volmd.fAUsage != RECTANGLE)
	{
		// dump black frame, with zero shape
		if(volmd.fAUsage == EIGHT_BIT)
			dumpNonCodedFrame(pfYUV, pfSeg, ppfAux, volmd.iAuxCompCount, rct, volmd.nBits);
		else
			dumpNonCodedFrame(pfYUV, pfSeg, NULL, 0, rct, volmd.nBits);
	}
	else if ( volmd.iEnhnType == 0 || volmd.volType == BASE_LAYER )
	{
		if(pvopcPrevDisp==NULL)
			dumpNonCodedFrame(pfYUV, pfSeg, NULL, 0, rct, volmd.nBits);
		else
		{
			// rectangular output
			// save previously displayed frame again
			Int iYDataHeight = rct.height ();
			Int iUVDataHeight = rct.height () / 2;
			Int iYFrmWidth = pvopcPrevDisp->whereY ().width;
			Int iUvFrmWidth = pvopcPrevDisp->whereUV ().width;
			Int nSkipYPixel = iYFrmWidth * EXPANDY_REF_FRAME + EXPANDY_REF_FRAME;
			Int nSkipUvPixel = iUvFrmWidth * EXPANDUV_REF_FRAME + EXPANDUV_REF_FRAME;
			const PixelC* ppxlcY = pvopcPrevDisp->pixelsY () + nSkipYPixel;
			const PixelC* ppxlcU = pvopcPrevDisp->pixelsU () + nSkipUvPixel;
			const PixelC* ppxlcV = pvopcPrevDisp->pixelsV () + nSkipUvPixel;

			CoordI y;
			for (y = 0; y < iYDataHeight; y++) {
				Int size = (Int) fwrite (ppxlcY, sizeof (PixelC), rct.width, pfYUV);
				if (size == 0)
					exit(fprintf (stderr, "Can't write to file\n"));
				ppxlcY += iYFrmWidth;
			}

			// NBIT: now there is a UV component when nBits>8
			for (y = 0; y < iUVDataHeight; y++) {
				Int size = (Int) fwrite (ppxlcU, sizeof (PixelC), rct.width / 2, pfYUV);
				if (size == 0)
					exit(fprintf (stderr, "Can't write to file\n"));
				ppxlcU += iUvFrmWidth;
			}

			for (y = 0; y < iUVDataHeight; y++) {
				Int size = (Int) fwrite (ppxlcV, sizeof (PixelC), rct.width / 2, pfYUV);
				if (size == 0)
					exit(fprintf (stderr, "Can't write to file\n"));
				ppxlcV += iUvFrmWidth;
			}
		}
	}
}

Void CSessionEncoder::dumpData (FILE* pfYuvDst, FILE* pfSegDst, 
                                FILE** ppfAuxDst, Int iLayer, 
								                const CVOPU8YUVBA* pvopcSrc, 
								                const CRct& rctOrg, const VOLMode& volmd, const Bool bInterlace)
{
	Int iYDataHeight = rctOrg.height ();
	Int iUVDataHeight = rctOrg.height () / 2;
	Int iYFrmWidth = pvopcSrc->whereY ().width;
	Int iUvFrmWidth = pvopcSrc->whereUV ().width;
	Int nSkipYPixel = iYFrmWidth * EXPANDY_REF_FRAME + EXPANDY_REF_FRAME;
	Int nSkipUvPixel = iUvFrmWidth * EXPANDUV_REF_FRAME + EXPANDUV_REF_FRAME;
	const PixelC* ppxlcY = pvopcSrc->pixelsY () + nSkipYPixel;
	const PixelC* ppxlcU = pvopcSrc->pixelsU () + nSkipUvPixel;
	const PixelC* ppxlcV = pvopcSrc->pixelsV () + nSkipUvPixel;

//OBSSFIX_MODE3	
	if ( volmd.iEnhnType == 0 || volmd.volType == BASE_LAYER || (volmd.iHierarchyType == 0 && volmd.iEnhnType != 1)){ // added by Sharp (99/1/20)
//	if ( volmd.iEnhnType == 0 || volmd.volType == BASE_LAYER ){ // added by Sharp (99/1/20)
//~OBSSFIX_MODE3
		if(!volmd.bShapeOnly){				//OBSS_SAIT_991015
			if(volmd.fAUsage != RECTANGLE)
			{
				// windowed output
				const PixelC* ppxlcBY = pvopcSrc->pixelsBY () + nSkipYPixel;
				PixelC pxlcZero = 0;
				CoordI x,y;
				Int iW = rctOrg.width;
				for (y = 0; y < iYDataHeight; y++) {
					for(x = 0; x < iW; x++) {
						if(ppxlcBY[x])
						{
							if(fwrite (ppxlcY + x, sizeof (PixelC), 1, pfYuvDst)==0)
								exit(fprintf (stderr, "Can't write to file\n"));
						}
						else
						{
							if(fwrite (&pxlcZero, sizeof (PixelC), 1, pfYuvDst)==0)
								exit(fprintf (stderr, "Can't write to file\n"));
						}
					}
					ppxlcY += iYFrmWidth;
					ppxlcBY += iYFrmWidth;
				}

				((CU8Image*) pvopcSrc->getPlane (BUV_PLANE))->decimateBinaryShapeFrom (*(pvopcSrc->getPlane (BY_PLANE)), bInterlace);
				const PixelC* ppxlcBUVTop = pvopcSrc->pixelsBUV () + nSkipUvPixel;			
				iW = rctOrg.width / 2;
				// NBIT: now there is a UV component when nBits>8
				pxlcZero = 128;
				const PixelC* ppxlcBUV = ppxlcBUVTop;
				for (y = 0; y < iUVDataHeight; y++) {
					for(x = 0; x < iW; x++) {
						if(ppxlcBUV[x])
						{
							if(fwrite (ppxlcU + x, sizeof (PixelC), 1, pfYuvDst)==0)
								exit(fprintf (stderr, "Can't write to file\n"));
						}
						else
						{
							if(fwrite (&pxlcZero, sizeof (PixelC), 1, pfYuvDst)==0)
								exit(fprintf (stderr, "Can't write to file\n"));
						}
					}
					ppxlcU += iUvFrmWidth;
					ppxlcBUV += iUvFrmWidth;
				}

				ppxlcBUV = ppxlcBUVTop;
				for (y = 0; y < iUVDataHeight; y++) {
					for(x = 0; x < iW; x++) {
						if(ppxlcBUV[x])
						{
							if(fwrite (ppxlcV + x, sizeof (PixelC), 1, pfYuvDst)==0)
								exit(fprintf (stderr, "Can't write to file\n"));
						}
						else
						{
							if(fwrite (&pxlcZero, sizeof (PixelC), 1, pfYuvDst)==0)
								exit(fprintf (stderr, "Can't write to file\n"));
						}
					}
					ppxlcV += iUvFrmWidth;
					ppxlcBUV += iUvFrmWidth;
				}
			}
			else
			{
				// keep a record of the vop that we save
				m_rgpvopcPrevDisp[iLayer] = pvopcSrc;
				
				// rectangular output
				CoordI y;
				for (y = 0; y < iYDataHeight; y++) {
					Int size = (Int) fwrite (ppxlcY, sizeof (PixelC), rctOrg.width, pfYuvDst);
					if (size == 0)
						exit(fprintf (stderr, "Can't write to file\n"));
					ppxlcY += iYFrmWidth;
				}

				// NBIT: now there is a UV component when nBits>8
				for (y = 0; y < iUVDataHeight; y++) {
					Int size = (Int) fwrite (ppxlcU, sizeof (PixelC), rctOrg.width / 2, pfYuvDst);
					if (size == 0)
						exit(fprintf (stderr, "Can't write to file\n"));
					ppxlcU += iUvFrmWidth;
				}

				for (y = 0; y < iUVDataHeight; y++) {
					Int size = (Int) fwrite (ppxlcV, sizeof (PixelC), rctOrg.width / 2, pfYuvDst);
					if (size == 0)
						exit(fprintf (stderr, "Can't write to file\n"));
					ppxlcV += iUvFrmWidth;
				}
			}
		}	//OBSS_SAIT_991015										
	} // added by Sharp (99/1/20)

	
	if (volmd.fAUsage != RECTANGLE)	{
		const PixelC* ppxlcBY = pvopcSrc->pixelsBY () + nSkipYPixel;
		for (CoordI y = 0; y < iYDataHeight; y++) {
			if (volmd.bNot8Bit==0) {
				
			  /*Int size =*/ (Int) fwrite (ppxlcBY, sizeof (PixelC), rctOrg.width, pfSegDst);
				
			} else { // NBIT: here PixelC is unsigned short
				const PixelC* ppxlcBYAux = ppxlcBY;
				for (CoordI x = 0; x < m_rctOrg.width; x++, ppxlcBYAux++) {
					fwrite(ppxlcBYAux, sizeof(PixelC), 1, pfSegDst);
				}
			}
			ppxlcBY += iYFrmWidth;
		}
	} 
	
	if (volmd.fAUsage==EIGHT_BIT && ppfAuxDst!=NULL)	{
		
		for(Int iAuxComp=0; iAuxComp<volmd.iAuxCompCount; iAuxComp++ ) { // MAC (SB) 1-Dec-99
			const PixelC* ppxlcBY = pvopcSrc->pixelsBY () + nSkipYPixel;
			const PixelC* ppxlcA = pvopcSrc->pixelsA (iAuxComp) + nSkipYPixel;
			const PixelC pxlcZero = 0;
			for (CoordI y = 0; y < iYDataHeight; y++) {     
				const PixelC* ppxlcBYAux = ppxlcBY;
				const PixelC* ppxlcAAux = ppxlcA;
				CoordI x;
				for( x = 0; x < m_rctOrg.width; x++, ppxlcBYAux++, ppxlcAAux++)
					if(*ppxlcBYAux)
						fwrite(ppxlcAAux, sizeof(PixelC), 1, ppfAuxDst[iAuxComp]);
					else
						fwrite(&pxlcZero, sizeof(PixelC), 1, ppfAuxDst[iAuxComp]);
					ppxlcA  += iYFrmWidth;
					ppxlcBY += iYFrmWidth;
			}
		}
	}
}

Void CSessionEncoder::updateRefForTPS (
	CVideoObjectEncoder* pvopc,
	CEnhcBufferEncoder* BufP1,
	CEnhcBufferEncoder* BufP2,
	CEnhcBufferEncoder* BufB1,
	CEnhcBufferEncoder* BufB2,
	CEnhcBufferEncoder* BufE,
	Int bNoNextVOP,
	Int iVOrelative,
	Int iEcount,
	Int ibFrameWithRate,
	Int ieFrame,
	Bool bupdateForLastLoop
)
{
	if (	(m_rgiTemporalScalabilityType[iVOrelative] == 2 && iEcount == 0) ||
		(m_rgiTemporalScalabilityType[iVOrelative] == 0 && ieFrame == ibFrameWithRate) ){

		if ( BufP1 -> m_bCodedFutureRef == 1 ){ // added by Sharp (99/1/26)
			pvopc -> setPredType(PVOP);
			pvopc -> setRefSelectCode(1);
			if ( BufB1 -> empty() )
				BufP1 -> putBufToQ0 ( pvopc );
			else
				BufB1 -> putBufToQ0 ( pvopc );
		} else // added by Sharp (99/1/26)
			pvopc -> setPredType(IVOP); // added by Sharp (99/1/26)

	} else if ( m_rgiTemporalScalabilityType[iVOrelative] == 2 ){

		pvopc -> setPredType(BVOP);
		pvopc -> setRefSelectCode(1);
		BufE -> putBufToQ0 ( pvopc );
		if ( BufB1 -> empty() )
			BufP1 -> putBufToQ1( pvopc );
		else
			BufB1 -> putBufToQ1( pvopc );

	} else if ( m_rgiTemporalScalabilityType[iVOrelative] == 1 ){

		if ( bNoNextVOP && !bupdateForLastLoop ) {
			pvopc -> setPredType(BVOP);
			pvopc -> setRefSelectCode(3);
			if ( BufB1 -> empty() )
				BufP1 -> putBufToQ0( pvopc );
			else
				BufB1 -> putBufToQ0( pvopc );
			if ( BufB2 -> empty() )
				BufP2 -> putBufToQ1( pvopc );
			else
				BufB2 -> putBufToQ1( pvopc );
		}
		else if ( bNoNextVOP && bupdateForLastLoop ) {
		if ( BufP1 -> m_bCodedFutureRef == 1 ){ // added by Sharp (99/1/26)
			pvopc -> setPredType(PVOP);
			pvopc -> setRefSelectCode(1);

			if ( BufB1 -> empty() )
				BufP1 -> putBufToQ0 ( pvopc );
			else
				BufB1 -> putBufToQ0 ( pvopc );
		} else // added by Sharp (99/1/26)
			pvopc -> setPredType(IVOP); // added by Sharp (99/1/26)
		}
		else {
			Time prevTime;
			Time nextTime;

			pvopc -> setPredType(BVOP);
			pvopc -> setRefSelectCode(3);
			if ( BufB1 -> empty() ){
				BufP1 -> putBufToQ0( pvopc );
//				cout << "Time of Q0:" << BufP1 -> m_t << "\n";
				prevTime = BufP1 -> m_t;
			}
			else{
				BufB1 -> putBufToQ0( pvopc );
//				cout << "Time of Q0:" << BufB1 -> m_t << "\n";
				prevTime = BufB1 -> m_t;
			}
			if ( BufB2 -> empty() ){
				BufP2 -> putBufToQ1( pvopc );
//				cout << "Time of Q1:" << BufP2 -> m_t << "\n";
				nextTime = BufP2 -> m_t;
			}
			else{
				BufB2 -> putBufToQ1( pvopc );
//				cout << "Time of Q1:" << BufB2 -> m_t << "\n";
				nextTime = BufB2 -> m_t;
			}
			pvopc -> m_tPastRef = prevTime;
			pvopc -> m_tFutureRef = nextTime;
		}

	} 
// begin: added by Sharp (99/1/25)
  else if ( m_rgiTemporalScalabilityType[iVOrelative] == 3 && iEcount == 0) {
		if ( BufP1 -> m_bCodedFutureRef == 1 ){
        pvopc -> setPredType(PVOP);
        pvopc -> setRefSelectCode(2);
        if ( BufB2 -> empty() )
          BufP2 -> putBufToQ1 ( pvopc );
        else
          BufB2 -> putBufToQ1 ( pvopc );
    } else {
      pvopc -> setPredType(IVOP);
    }
  } else if ( m_rgiTemporalScalabilityType[iVOrelative] == 3 ){
    pvopc -> setPredType(BVOP);
    pvopc -> setRefSelectCode(2);
    BufE -> putBufToQ0 ( pvopc );
    if ( BufB2 -> empty() )
      BufP2 -> putBufToQ1( pvopc );
    else
      BufB2 -> putBufToQ1( pvopc );
  } else if (m_rgiTemporalScalabilityType[iVOrelative] == 4 ){
    pvopc -> setPredType(PVOP);
    pvopc -> setRefSelectCode(2);
    if ( BufB2 -> empty() )
      BufP2 -> putBufToQ1 ( pvopc );
    else
      BufB2 -> putBufToQ1 ( pvopc );
  }
// end: added by Sharp (99/1/25)
	
	else {

		if ( BufE -> m_bCodedFutureRef == 1 ){ // added by Sharp (99/1/26)
			pvopc -> setPredType(PVOP);
			pvopc -> setRefSelectCode(0);
			BufE -> putBufToQ0 ( pvopc );
		} else // added by Sharp (99/1/26)
			pvopc -> setPredType(IVOP); // added by Sharp (99/1/26)
	}
}

// for back/foward shape
Void CSessionEncoder::initVObfShape (CVideoObjectEncoder** rgpbfShape, 
					Int iobj, // ofstream* rgpostrmTrace [],
					VOLMode& volmd_back, 
					VOPMode& vopmd_back,
					VOLMode& volmd_forw, 
					VOPMode& vopmd_forw
) // made from initVOEncoder()
{
	rgpbfShape [0] = new CVideoObjectEncoder (	iobj, 
							volmd_back,
							vopmd_back,
							m_rctOrg.width,
							m_rctOrg.height () //,
		    				);
	rgpbfShape [1] = new CVideoObjectEncoder (	iobj, 
							volmd_forw,
							vopmd_forw,
							m_rctOrg.width,
							m_rctOrg.height () //,
		    				);
}

Void CSessionEncoder::dumpDataOneFrame (UInt iFrame, Int iobj, const CVOPU8YUVBA* pvopcSrc, const VOLMode& volmd)
{
	static Int iYDataHeight = m_rctOrg.height ();
	static Int iUVDataHeight = m_rctOrg.height () / 2;
	static Int iYFrmWidth = pvopcSrc->whereY ().width;
	static Int iUvFrmWidth = pvopcSrc->whereUV ().width;
	static Int nSkipYPixel = iYFrmWidth * EXPANDY_REF_FRAME + EXPANDY_REF_FRAME;
	static Int nSkipUvPixel = iUvFrmWidth * EXPANDUV_REF_FRAME + EXPANDUV_REF_FRAME;
	const PixelC* ppxlcY = pvopcSrc->pixelsY () + nSkipYPixel;
	const PixelC* ppxlcU = pvopcSrc->pixelsU () + nSkipUvPixel;
	const PixelC* ppxlcV = pvopcSrc->pixelsV () + nSkipUvPixel;

//	cout << "\n=== write in separate format : frame no. =" << iFrame << "\n";

	static Char pchYUV [100], pchSeg [100];
	CoordI y;

//	if(volmd.volType == BASE_LAYER) { // deleted by Sharp (98/11/11)
		sprintf (pchYUV, SUB_YUVFILE, m_pchReconYUVDir, iobj, m_pchPrefix);
		sprintf (pchSeg, SUB_SEGFILE, m_pchReconYUVDir, iobj, m_pchPrefix);
//	} // deleted by Sharp (98/11/11)
// begin: deleted by Sharp (98/11/11)
//	else {
//		sprintf (pchYUV, ENHN_SUB_YUVFILE, m_pchReconYUVDir, iobj, m_pchPrefix);
//		sprintf (pchSeg, ENHN_SUB_SEGFILE, m_pchReconYUVDir, iobj, m_pchPrefix);
//	}
// end: deleted by Sharp (98/11/11)
	sprintf (pchYUV, "%s%d", pchYUV, iFrame);
	sprintf (pchSeg, "%s%d", pchSeg, iFrame);

	FILE* pfYuvDst = fopen(pchYUV, "wb");

	for (y = 0; y < iYDataHeight; y++) {
		Int size = (Int) fwrite (ppxlcY, sizeof (PixelC), m_rctOrg.width, pfYuvDst);
		if (size == 0)
			exit(fprintf (stderr, "Can't write to file(Y)\n"));
		ppxlcY += iYFrmWidth;
	}

	for (y = 0; y < iUVDataHeight; y++) {
		Int size = (Int) fwrite (ppxlcU, sizeof (PixelC), m_rctOrg.width / 2, pfYuvDst);
		if (size == 0)
			exit(fprintf (stderr, "Can't write to file(U)\n"));
		ppxlcU += iUvFrmWidth;
	}

	for (y = 0; y < iUVDataHeight; y++) {
		Int size = (Int) fwrite (ppxlcV, sizeof (PixelC), m_rctOrg.width / 2, pfYuvDst);
		if (size == 0)
			exit(fprintf (stderr, "Can't write to file(V)\n"));
		ppxlcV += iUvFrmWidth;
	}
	fclose(pfYuvDst);

	if (volmd.fAUsage != RECTANGLE)	{
		FILE* pfSegDst = fopen(pchSeg, "wb");
		const PixelC* ppxlcA = (volmd.fAUsage == ONE_BIT) ? pvopcSrc->pixelsBY () + nSkipYPixel
														  : pvopcSrc->pixelsA (0) + nSkipYPixel;
		for (y = 0; y < iYDataHeight; y++) {
			Int size = (Int) fwrite (ppxlcA, sizeof (PixelC), m_rctOrg.width, pfSegDst);
			if (size == 0)
				exit(fprintf (stderr, "Can't write to file(B)\n"));
			ppxlcA += iYFrmWidth;
		}
		fclose(pfSegDst);
	}
}

Void CSessionEncoder::encodeEnhanceVideoObject(Bool bObjectExists,
										Int iFrame,
										VOPpredType predType,
										Int iDumpMode,
										Int iVO,Int iVOrelative,
										FILE* pfYuvSrc,
										FILE* pfSegSrc,
										FILE* rgpfReconYUV[],
										FILE* rgpfReconSeg[],
										PixelC pxlcObjColor,
										CVideoObjectEncoder* rgpvoenc,
										const VOLMode& volmd,
										const VOLMode& volmd_enhn,
										Int iEnhnFirstFrame,
										ofstream* rgpostrm[],
										CEnhcBufferEncoder& BufP1,
										CEnhcBufferEncoder& BufP2,
										CEnhcBufferEncoder& BufB1,
										CEnhcBufferEncoder& BufB2,
										CEnhcBufferEncoder& BufE
										)
{
	Int ieFramebShape; // for back/forward shape : frame number of backward shape
	Int ieFramefShape; // for back/forward shape : frame number of forward shape
//	m_bPrevObjectExists = FALSE; // added by Sharp (98/2/10)
	
	Int iLayer = 1;
	if (m_rguiSpriteUsage [iVOrelative] == 0) {
		bObjectExists = loadData (iFrame, pfYuvSrc, pfSegSrc, NULL, pxlcObjColor, rgpvoenc ->m_pvopcOrig, m_rctOrg, volmd_enhn);

		if ( rgpvoenc -> m_volmd.iEnhnType == 1) {
	
			rgpvoenc -> set_LoadShape(&ieFramebShape, &ieFramefShape, volmd.iTemporalRate, 
													iFrame, m_iFirstFrame, iEnhnFirstFrame );
			if ( bObjectExists && !m_bPrevObjectExists && rgpvoenc -> m_volmd.iEnhnType == 1 )
			{ // bVOPVisible reflect a value of VOP_CODED
				rgpvoenc -> m_vopmd.iLoadBakShape = 1;
				rgpvoenc -> m_vopmd.iLoadForShape = 1;
			}
			m_bPrevObjectExists = bObjectExists;
						
//			printf("FLAG: %d, %d\n", rgpvoenc -> m_vopmd.iLoadBakShape, rgpvoenc -> m_vopmd.iLoadForShape );

			if(rgpvoenc -> m_vopmd.iLoadBakShape) {
				rgpvoenc  -> m_vopmd.iLoadBakShape = loadData (ieFramebShape, pfYuvSrc, pfSegSrc, NULL, pxlcObjColor, 
					rgpvoenc ->rgpbfShape [0]->m_pvopcOrig, m_rctOrg, rgpvoenc ->rgpbfShape[0]->m_volmd);
				if ( !rgpvoenc -> m_vopmd.iLoadBakShape )
					cout << "Load_backward_shape was ON, but turned off because of no shape\n";
			}
			if(rgpvoenc -> m_vopmd.iLoadForShape) {
				rgpvoenc -> m_vopmd.iLoadForShape = loadData (ieFramefShape, pfYuvSrc, pfSegSrc, NULL, pxlcObjColor, 
					rgpvoenc ->rgpbfShape [1]->m_pvopcOrig, m_rctOrg, rgpvoenc ->rgpbfShape[1]->m_volmd);
				if ( !rgpvoenc -> m_vopmd.iLoadForShape )
					cout << "Load_forward_shape was ON, but turned off because of no shape\n";
			}
		}
// begin: added by Sharp (98/3/24)
		else if ( rgpvoenc -> m_volmd.iEnhnType == 2 ) {
			rgpvoenc -> m_vopmd.iLoadBakShape = 0;
			rgpvoenc -> m_vopmd.iLoadForShape = 0;
		}
// end: added by Sharp (98/3/24)

		rgpvoenc -> encode (bObjectExists, (Time) (iFrame - m_iFirstFrame), predType); //, pvopcBaseQuant

		// for background composition
		if(rgpvoenc -> m_volmd.iEnhnType != 0) {
			// should be changed to background_composition flag later. // modified by Sharp (98/3/24)
//			printf("Performing B.C. ... Time = (%d, %d, %d)\n", iFrame, ieFramefShape, ieFramebShape);
			rgpvoenc -> BackgroundComposition(m_rctOrg.width, m_rctOrg.height (),
				iFrame, ieFramefShape, ieFramebShape,
				BufP1.m_pvopcBuf,
				BufP2.m_pvopcBuf,
				BufB1.m_pvopcBuf,
				BufB2.m_pvopcBuf,
				m_pchReconYUVDir, iVO, m_pchPrefix,
				rgpfReconYUV[iLayer]); // modified by Sharp (98/10/26)
		}
	} // sprite

	//dump the output
	if (bObjectExists)	{
//		if ( rgpvoenc -> m_volmd.iEnhnType == 0 ){ // added by Sharp (98/10/26) // deleted by Sharp (99/1/25)
// begin: modified by Sharp (98/11/11)
#ifndef __OUT_ONE_FRAME_
			if(iDumpMode==DUMP_CURR)
			{
				dumpData (rgpfReconYUV [iLayer], rgpfReconSeg [iLayer], NULL, iLayer, rgpvoenc ->pvopcReconCurr (), m_rctOrg, volmd_enhn, 0);
			}
			else if(iDumpMode==DUMP_PREV)
			{
				dumpData (rgpfReconYUV [iLayer], rgpfReconSeg [iLayer], NULL, iLayer, rgpvoenc ->pvopcRefQPrev (), m_rctOrg, volmd_enhn, 0);
			}
#else
				dumpDataOneFrame (iFrame, iVO, rgpvoenc ->pvopcReconCurr(), volmd_enhn); // save one frame
#endif
// end: modified by Sharp (98/11/11)
//		} // added by Sharp (98/10/26) // deleted by Sharp (99/1/25)
	}
#ifndef __OUT_ONE_FRAME_ // added by Sharp (98/11/11)
	else
		dumpDataNonCoded(rgpfReconYUV [iLayer], rgpfReconSeg [iLayer], NULL, iLayer, m_rctOrg, volmd_enhn);
#endif // added by Sharp (98/11/11)

	rgpostrm[ENHN_LAYER] ->write (rgpvoenc ->pOutStream ()->str (), rgpvoenc ->pOutStream ()->pcount ());
	if ( rgpvoenc -> m_bCodedFutureRef == 1 ) // added by Sharp (99/1/28)
	BufE.getBuf( rgpvoenc );
}

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

and also edited by
	Dick van Smirren (D.vanSmirren@research.kpn.com), KPN Research
	Cor Quist (C.P.Quist@research.kpn.com), KPN Research
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
	Takefumi Nagumo	(nagumo@av.crl.sony.co.jp), Sony Corporation
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

	vopSeDec.cpp

Abstract:

	Decoder for one Video Object.

Revision History:
	Dec 20, 1997:	Interlaced tools added by NextLevel Systems
                    X. Chen (xchen@nlvl.com) B. Eifrig (beifrig@nlvl.com)
    Feb.16, 1999:   (use of rgiDefaultInterQMatrixAlpha) 
                    Mathias Wien (wien@ient.rwth-aachen.de) 
	Feb.24, 1999:   GMC added by Yoshinori Suzuki (Hitachi, Ltd.) 
	Aug.24, 1999:   NEWPRED added by Hideaki Kimata (NTT) 
	Sep.06	1999 : RRV added by Eishi Morimatsu (Fujitsu Laboratories Ltd.) 
	Nov.11  1999 : Fixed Complexity Estimation syntax support, version 2 (Massimo Ravasi, EPFL)
	Feb.01	2000 : Bug fixed OBSS by Takefumi Nagumo (Sony)

*************************************************************************/
#include <stdio.h>
#include <fstream.h>
#include <iostream.h>
#ifdef __GNUC__
//#include <strstream.h>
#else
#include <strstrea.h>
#endif
#include <math.h>

#include "typeapi.h"
#include "codehead.h"
#include "entropy/bitstrm.hpp"
#include "entropy/entropy.hpp"
#include "entropy/huffman.hpp"
#include "global.hpp"
#include "mode.hpp"
#include "vopses.hpp"
#include "cae.h" //	Added for error resilient mode by Toshiba(1997-11-14)

#include "tps_enhcbuf.hpp"	// Added by Sharp(1998-02-10)
#include "enhcbufdec.hpp"	//	
#include "vopsedec.hpp"
#include "idct.hpp" // yrchen

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

#ifdef __PC_COMPILER_
#define IOSBINARY ios::binary
#else
#define IOSBINARY ios::bin
#endif // __PC_COMPILER_

#define _FOR_GSSP_

#define ASSERT(a) if (!(a)) { printf("iso throw %d\n", __LINE__);throw((int)__LINE__);}

CVideoObjectDecoder::~CVideoObjectDecoder ()
{

// NEWPRED
	if (g_pNewPredDec != NULL) delete g_pNewPredDec;
// ~NEWPRED

//	delete m_pistrm;
	if (m_pistrm >= 0) close(m_pistrm);
	delete m_pbitstrmIn;
	delete m_pentrdecSet;
	delete m_pvopcRightMB;
	// HHI Schueuer
	delete m_pscanSelector;
	// end HHI
	if (m_pinvdct) delete m_pinvdct;
}


Int CVideoObjectDecoder::h263_decode (bool read_header)
{
	if (read_header != FALSE) 
	{
		while ( m_pbitstrmIn -> peekBits(NUMBITS_SHORT_HEADER_START_CODE) != SHORT_VIDEO_START_MARKER)
		{
			if(m_pbitstrmIn->eof()==EOF)  // [FDS] 
				return EOF;
			m_pbitstrmIn -> getBits(1);
		}
		// moved outside m_pbitstrmIn -> getBits(22);
		m_t = video_plane_with_short_header(); 
	}
	else
	{
		m_tPastRef = m_tFutureRef = m_t;
	}

	m_bUseGOV=FALSE; 
	m_bLinkisBroken=FALSE;
	m_vopmd.iRoundingControl=0;
	m_vopmd.iIntraDcSwitchThr=0; 
	m_vopmd.bInterlace=FALSE;
	m_vopmd.bAlternateScan=FALSE;
	m_t = 1;
	m_vopmd.mvInfoForward.uiFCode=1; 
	m_vopmd.mvInfoForward.uiScaleFactor = 1 << (m_vopmd.mvInfoForward.uiFCode - 1);
	m_vopmd.mvInfoForward.uiRange = 16 << m_vopmd.mvInfoForward.uiFCode;
	//	Added for error resilient mode by Toshiba(1998-1-16)
	m_vopmd.mvInfoBackward.uiFCode = 1;
	//	End Toshiba(1998-1-16)
	m_vopmd.bShapeCodingType=1;

	m_tPastRef = m_tFutureRef; 
	m_tFutureRef = m_t; 
	m_iBCount = 0;
	
	updateAllRefVOPs ();
	
	// select reference frames for Base/Temporal-Enhc/Spatial-Enhc Layer	End	    Sharp(1998-02-10)

	switch(m_vopmd.vopPredType)
	{
	case IVOP:
	  //		cout << "\tIVOP";
/*Added by SONY (98/03/30)*/
		if(m_bLinkisBroken == TRUE && m_bUseGOV == TRUE)        m_bLinkisBroken = FALSE;
/*Added by SONY (98/03/30) END*/
		break;
	case PVOP:
	  //cout << "\tPVOP (reference: t=" << m_tPastRef <<")";
		break;
	default:
		break;
	}
	//cout << "\n";
	//cout.flush ();
/* Added by SONY (98/03/30)*/
	if(m_bLinkisBroken == TRUE && m_bUseGOV == TRUE)
		fprintf(stderr,"WARNING: broken_link = 1  --- Output image must be broken.\n");
/*Added by SONY (98/03/30) End*/

	decodeVOP ();  

	CMBMode* pmbmdTmp = m_rgmbmd;
	m_rgmbmd = m_rgmbmdRef;
	m_rgmbmdRef = pmbmdTmp;
	CMotionVector* pmvTmp = m_rgmv;
	m_rgmv = m_rgmvRef;
	m_rgmvRef = pmvTmp;
	m_rgmvBackward = m_rgmv + BVOP_MV_PER_REF_PER_MB * m_iSessNumMB;
// ? For Temporal Scalability	End	 Sharp(1998-02-10)
	m_iBVOPOffsetForPadY = m_iOffsetForPadY;
	m_iBVOPOffsetForPadUV = m_iOffsetForPadUV;
	m_rctBVOPPrevNoExpandY = m_rctPrevNoExpandY;
	m_rctBVOPPrevNoExpandUV = m_rctPrevNoExpandUV;

	m_rctBVOPRefVOPY1 = m_rctRefVOPY1;
	m_rctBVOPRefVOPUV1 = m_rctRefVOPUV1;
	// For Temporal Scalability	End	 Sharp(1998-02-10)

	repeatPadYOrA ((PixelC*) m_pvopcRefQ1->pixelsY () + m_iOffsetForPadY, m_pvopcRefQ1);
	repeatPadUV (m_pvopcRefQ1);

	//reset by in RefQ1 so that no left-over from last frame

	return TRUE; 
}

UInt CVideoObjectDecoder::video_plane_with_short_header()
{
  short_video_header = 1;
  m_pbitstrmIn->getBits(22); // read header
  UInt uiTemporalReference = m_pbitstrmIn -> getBits (8);
	m_pbitstrmIn -> getBits(5);
	UInt uiSourceFormat = m_pbitstrmIn -> getBits (3);

	if (uiSourceFormat==1) {
		//fprintf(stderr,"Sub-QCIF, 128x96, 8 macroblocks/gob, 6 gobs in vop\n");
		uiNumGobsInVop=6;
		uiNumMacroblocksInGob=8;
		m_ivolWidth=128;
		m_ivolHeight=96;
	} else if (uiSourceFormat==2) {
		//fprintf(stderr,"QCIF, 176x144, 11 macroblocks/gob, 9 gobs in vop\n");
		uiNumGobsInVop=9;
		uiNumMacroblocksInGob=11;
		m_ivolWidth=176;
		m_ivolHeight=144;
	} else if (uiSourceFormat==3) {
		//fprintf(stderr,"CIF, 352x288, 22 macroblocks/gob, 18 gobs in vop\n");
		uiNumGobsInVop=18;
		uiNumMacroblocksInGob=22;
		m_ivolWidth=352;
		m_ivolHeight=288;
	} else if (uiSourceFormat==4) {
		//fprintf(stderr,"4CIF, 704x576, 88 macroblocks/gob, 18 gobs in vop\n");
		uiNumGobsInVop=18;
		uiNumMacroblocksInGob=88;
		m_ivolWidth=704;
		m_ivolHeight=576;
	} else if (uiSourceFormat==5) {
		//fprintf(stderr,"16CIF, 1408x1152, 352 macroblocks/gob, 18 gobs in vop\n");
		uiNumGobsInVop=18;
		uiNumMacroblocksInGob=352;
		m_ivolWidth=1408;
		m_ivolHeight=1152;
	} else {
		fprintf(stderr,"Wrong Source Format in video_plane_with_short_header()\n");
		exit (0);
	}
	UInt uiPictureCodingType = m_pbitstrmIn -> getBits(1);
	if (uiPictureCodingType==0) 
		m_vopmd.vopPredType=IVOP;
	else
		m_vopmd.vopPredType=PVOP;

	m_pbitstrmIn -> getBits(4);
	UInt uiVopQuant = m_pbitstrmIn -> getBits(5);
	//fprintf(stderr,"vop_quant (0..31)             %d\n",uiVopQuant);
	m_vopmd.intStepI=uiVopQuant; 
	m_vopmd.intStep=uiVopQuant; // idem
	m_pbitstrmIn -> getBits(1);
	do {
		uiPei = m_pbitstrmIn -> getBits(1);
		//fprintf(stderr,"pei gelezen %d\n",uiPei);
		if (uiPei==1) {
			
			m_pbitstrmIn -> getBits(8);
		}
// [bezig]
	} while (uiPei==1);

	m_uiVOId = 1; 
	m_volmd.iClockRate = 30; 
	m_volmd.dFrameHz = 30;
	m_iNumBitsTimeIncr = 4; 
	m_volmd.bShapeOnly=FALSE; 
	m_volmd.fAUsage = RECTANGLE; 
	m_volmd.bAdvPredDisable = TRUE; 
	m_uiSprite = FALSE; 
	m_volmd.bNot8Bit=FALSE; 
	m_volmd.uiQuantPrecision=5;
	m_volmd.nBits=8;
	m_volmd.fQuantizer=Q_H263; 
	m_volmd.bDataPartitioning=FALSE;
	m_volmd.bReversibleVlc=FALSE;
	m_volmd.volType=BASE_LAYER;
	m_volmd.ihor_sampling_factor_n=1;
	m_volmd.ihor_sampling_factor_m=1;
	m_volmd.iver_sampling_factor_n=1;
	m_volmd.iver_sampling_factor_m=1;
	m_volmd.bDeblockFilterDisable=TRUE;
	m_volmd.bQuarterSample = 0;
	m_volmd.bRoundingControlDisable = 0;
	m_volmd.iInitialRoundingType = 0;
	m_volmd.bResyncMarkerDisable = 1;
	m_volmd.bVPBitTh = 0;
	m_volmd.bSadctDisable = 1;
	m_volmd.bComplexityEstimationDisable = 1;
	m_volmd.bTrace = 0;
	m_volmd.bDumpMB = 0;
	m_volmd.breduced_resolution_vop_enable = 0;

// NEWPRED
	m_volmd.bNewpredEnable=FALSE;
// ~NEWPRED

	return uiTemporalReference;
}


void CVideoObjectDecoder::decodeShortHeaderIntraMBDC(Int *rgiCoefQ)
{
	UInt uiIntraDC;
	uiIntraDC=m_pbitstrmIn->getBits(8);
	//printf("%d ", uiIntraDC);
	if (uiIntraDC==128||uiIntraDC==0) fprintf(stderr,"IntraDC = 0 of 128, not allowed in H.263 mode\n");
	if (uiIntraDC==255) uiIntraDC=128;
	
	rgiCoefQ[0]=uiIntraDC;
}

CVideoObjectDecoder::CVideoObjectDecoder (void) : 
  CVideoObject(), m_pscanSelector(0)
{
  m_t = m_tPastRef = m_tFutureRef = 0;
  m_iBCount = 0;
  m_vopmd.iVopConstantAlphaValue = 255;
  short_video_header = FALSE;
  m_pbitstrmIn = new CInBitStream();
  m_pentrdecSet = new CEntropyDecoderSet (m_pbitstrmIn);
}


CVideoObjectDecoder::CVideoObjectDecoder (
	const Char* pchStrFile,
	Int iDisplayWidth, Int iDisplayHeight,
	Bool *pbSpatialScalability,
	Bool *p_short_video_header//,
	//strstreambuf* pistrm
	) : CVideoObject (), m_pscanSelector(0)  // HHI Schueuer: m_pscanSelector(0) added
{
#if 0
	if (pistrm == NULL) {
#endif
	  m_pistrm = open(pchStrFile, O_RDONLY);
		if(m_pistrm < 0)
			fatal_error("Can't open bitstream file");
#if 0
	}
	else {
		m_pistrm = (ifstream *)new istream (pistrm);
	}
#endif


	m_pbitstrmIn = new CInBitStream (m_pistrm);
	m_pentrdecSet = new CEntropyDecoderSet (m_pbitstrmIn);

	m_t = m_tPastRef = m_tFutureRef = 0;
	m_iBCount = 0;

	m_vopmd.iVopConstantAlphaValue = 255;

	*p_short_video_header=FALSE; // Added by KPN for short headers


	if (m_pbitstrmIn->peekBits(NUMBITS_SHORT_HEADER_START_CODE) == SHORT_VIDEO_START_MARKER) 
 
	{ 

		fprintf(stderr, "\nBitstream with short header format detected\n"); 
		*p_short_video_header=TRUE;  
		// moved inside routine m_pbitstrmIn -> getBits(22);
		m_t = video_plane_with_short_header();
	}
	else {

	fprintf(stderr,"\nBitstream without short headers detected\n");
	decodeVOHead (); // also decodes vss, vso headers if present
	printf ("VO %d...\n", m_uiVOId);
	decodeVOLHead ();

	} 

	short_video_header=*p_short_video_header; 
	postVO_VOLHeadInit(iDisplayWidth, iDisplayHeight, pbSpatialScalability);
}


void CVideoObjectDecoder::postVO_VOLHeadInit (Int iDisplayWidth, 
					      Int iDisplayHeight,
					      Bool *pbSpatialScalability)
  {
/* (98/03/30 added by SONY)*/
	Int iDisplayWidthRound = 0;
	Int iDisplayHeightRound = 0;

	m_bLinkisBroken = FALSE;
	m_bUseGOV = FALSE;
/* (98/03/30 added by SONY)*/
	//	Added for error resilient mode by Toshiba(1997-11-14): Moved (1998-1-16)
	g_iMaxHeading = MAXHEADING_ERR;
	g_iMaxMiddle = MAXMIDDLE_ERR;
	g_iMaxTrailing = MAXTRAILING_ERR;
	//	End Toshiba(1997-11-14)

	setClipTab(); // NBIT

	if(m_volmd.volType == ENHN_LAYER){ // check scalability type
//OBSS_SAIT_991015
		m_volmd.bSpatialScalability = FALSE;
		if(pbSpatialScalability != NULL){
			if(m_volmd.iHierarchyType == 0 || m_volmd.bShapeOnly) {
				*pbSpatialScalability = TRUE;
				m_volmd.bSpatialScalability = TRUE;
			}
			else { 	
				*pbSpatialScalability = FALSE;
				m_volmd.bSpatialScalability = FALSE;
			}
		}
//~OBSS_SAIT_991015
	}
	if (m_volmd.fAUsage == RECTANGLE) {
		if (m_volmd.volType == ENHN_LAYER &&
			(m_volmd.ihor_sampling_factor_n != m_volmd.ihor_sampling_factor_m ||
	     	 m_volmd.iver_sampling_factor_n != m_volmd.iver_sampling_factor_m )){
			iDisplayWidth = m_ivolWidth;
			iDisplayHeight= m_ivolHeight;
//OBSS_SAIT_991015
			m_volmd.iFrmWidth_SS = iDisplayWidth;		
			m_volmd.iFrmHeight_SS = iDisplayHeight;		
//~OBSS_SAIT_991015
		}
		else if (iDisplayWidth == -1 && iDisplayHeight == -1) {
		  iDisplayWidth = m_ivolWidth;
		  iDisplayHeight = m_ivolHeight;
		}
		else if (iDisplayWidth != m_ivolWidth || iDisplayHeight != m_ivolHeight){
			fprintf(stderr, "\nDecode aborted! This rectangular VOP stream requires display\nwidth and height to be set to %dx%d.\n",
				m_ivolWidth, m_ivolHeight);
			exit(1);
		}
	}
//OBSS_SAIT_991015
	else if (m_volmd.fAUsage == ONE_BIT) {
		if (m_volmd.volType == ENHN_LAYER &&
			(m_volmd.ihor_sampling_factor_n_shape!=m_volmd.ihor_sampling_factor_m_shape  ||
	     	 m_volmd.iver_sampling_factor_n_shape!=m_volmd.iver_sampling_factor_m_shape )){
			m_volmd.iFrmWidth_SS = iDisplayWidth;		
			m_volmd.iFrmHeight_SS = iDisplayHeight;		
			if(pbSpatialScalability!=NULL) {
				*pbSpatialScalability = TRUE;
				m_volmd.bSpatialScalability = TRUE;
			}
		}
		else {
			if(pbSpatialScalability!=NULL) {
				*pbSpatialScalability = FALSE;
				m_volmd.bSpatialScalability = FALSE;
			}
		}
	}
//~OBSS_SAIT_991015
	m_rctDisplayWindow = CRct (0, 0, iDisplayWidth, iDisplayHeight); //same as m_rctOrg? will fixe later
/*
	if (m_volmd.fAUsage == RECTANGLE) {
		if (m_volmd.volType == ENHN_LAYER &&
			(m_volmd.ihor_sampling_factor_n/m_volmd.ihor_sampling_factor_m != 1||
	     	 m_volmd.iver_sampling_factor_n/m_volmd.iver_sampling_factor_m != 1)){
			iDisplayWidth = m_ivolWidth;
			iDisplayHeight= m_ivolHeight;
			if(pbSpatialScalability!=NULL)
				*pbSpatialScalability = TRUE;
			fprintf(stderr,"display size %d %d \n",iDisplayWidth, iDisplayHeight);
		}
		else if (iDisplayWidth != m_ivolWidth || iDisplayHeight != m_ivolHeight){
			fprintf(stderr, "\nDecode aborted! This rectangular VOP stream requires display\nwidth and height to be set to %dx%d.\n",
				m_ivolWidth, m_ivolHeight);
			exit(1);
		}
		else if(pbSpatialScalability!=NULL)
			*pbSpatialScalability = FALSE;
	}
	m_rctDisplayWindow = CRct (0, 0, iDisplayWidth, iDisplayHeight); //same as m_rctOrg? will fixe later
*/
	
	if (m_uiSprite == 1) { // change iDisplay size in order to get the first sprite piece
		iDisplayWidth = (Int) m_rctSpt.width;
		iDisplayHeight = (Int) m_rctSpt.height ();
	}

//OBSS_SAIT_991015
//	if (m_volmd.volType == ENHN_LAYER && m_volmd.fAUsage == ONE_BIT) {

	// I put this back in - swinder
	// takind it our breaks the ability to decode arbitrary sized sequences
		iDisplayWidthRound = ((iDisplayWidth + MB_SIZE - 1)>>4)<<4;
		iDisplayHeightRound = ((iDisplayHeight + MB_SIZE - 1)>>4)<<4;
//	}
//	else {
//		Int iMod = iDisplayWidth % MB_SIZE;
//		iDisplayWidthRound = (iMod > 0) ? iDisplayWidth + MB_SIZE - iMod : iDisplayWidth;
//		iMod = iDisplayHeight % MB_SIZE;
//		iDisplayHeightRound = (iMod > 0) ? iDisplayHeight + MB_SIZE - iMod : iDisplayHeight;
//	}
/*
	Int iMod = iDisplayWidth % MB_SIZE;
	Int iDisplayWidthRound = (iMod > 0) ? iDisplayWidth + MB_SIZE - iMod : iDisplayWidth;
	iMod = iDisplayHeight % MB_SIZE;
	Int iDisplayHeightRound = (iMod > 0) ? iDisplayHeight + MB_SIZE - iMod : iDisplayHeight;
*/
//~OBSS_SAIT_991015

	m_rctRefFrameY = CRct (
		-EXPANDY_REF_FRAME, -EXPANDY_REF_FRAME, 
		EXPANDY_REF_FRAME + iDisplayWidthRound, EXPANDY_REF_FRAME + iDisplayHeightRound
	);
	m_rctRefFrameUV = m_rctRefFrameY.downSampleBy2 ();
	allocateVOLMembers (iDisplayWidth, iDisplayHeight);

	// HHI Schueuer
	if (m_volmd.bSadctDisable)
		m_pscanSelector = new CInvScanSelector;
	else
		m_pscanSelector = new CInvScanSelectorForSADCT(m_rgiCurrMBCoeffWidth);
	// end HHI

// RRV insertion
	Int iScale	= (m_vopmd.RRVmode.iOnOff == 1) ? (2) : (1);
// ~RRV 
	if (m_volmd.fAUsage == RECTANGLE) {

		//wchen: if sprite; set it according to the initial piece instead
		m_rctCurrVOPY = (m_uiSprite == 0 || m_uiSprite == 2) ? CRct (0, 0, iDisplayWidthRound, iDisplayHeightRound) : m_rctSpt; // GMC

		m_rctCurrVOPUV = m_rctCurrVOPY.downSampleBy2 ();

		m_rctRefVOPY0 = m_rctCurrVOPY;
// RRV modification
		m_rctRefVOPY0.expand (EXPANDY_REFVOP *iScale);
//		m_rctRefVOPY0.expand (EXPANDY_REFVOP);
// ~RRV
		m_rctRefVOPUV0 = m_rctRefVOPY0.downSampleBy2 ();
		
		m_rctRefVOPY1 = m_rctRefVOPY0;
		m_rctRefVOPUV1 = m_rctRefVOPUV0;

		computeVOLConstMembers (); // these VOP members are the same for all frames
	}
//OBSS_SAIT_991015
	else if (m_volmd.fAUsage == ONE_BIT) {
		m_rctCurrVOPY = (m_uiSprite == 0) ? CRct (0, 0, iDisplayWidthRound, iDisplayHeightRound) : m_rctSpt;
		m_rctCurrVOPUV = m_rctCurrVOPY.downSampleBy2 ();

		m_rctRefVOPY0 = m_rctCurrVOPY;
		m_rctRefVOPY0.expand (EXPANDY_REFVOP);
		m_rctRefVOPUV0 = m_rctRefVOPY0.downSampleBy2 ();
		
		m_rctRefVOPY1 = m_rctRefVOPY0;
		m_rctRefVOPUV1 = m_rctRefVOPUV0;
	}
//~OBSS_SAIT_991015
	// buffer for shape decoding
	m_pvopcRightMB = new CVOPU8YUVBA (m_volmd.fAUsage, CRct (0, 0, MB_SIZE, MB_SIZE), m_volmd.iAuxCompCount);
	m_ppxlcRightMBBY = (PixelC*) m_pvopcRightMB->pixelsBY ();
	m_ppxlcRightMBBUV = (PixelC*) m_pvopcRightMB->pixelsBUV ();

// NEWPRED
	if (m_volmd.bNewpredEnable) {
		g_pNewPredDec->SetObject(
			m_iNumBitsTimeIncr,
			iDisplayWidth,
			iDisplayHeight,
			"",
			m_volmd.bNewpredSegmentType,
			m_volmd.fAUsage,
			m_volmd.bShapeOnly,
			m_pvopcRefQ0,
			m_pvopcRefQ1,
			m_rctRefFrameY,
			m_rctRefFrameUV
		);
		m_vopmd.m_iNumBitsVopID = m_iNumBitsTimeIncr + NUMBITS_VOP_ID_PLUS;
	}
// ~NEWPRED

	// buffers for Temporal Scalabe Decoding	Added by Sharp(1998-02-10)
	if (m_volmd.volType == ENHN_LAYER) {
		m_pBuffP1 = new CEnhcBufferDecoder (m_rctRefFrameY.width, m_rctRefFrameY.height ());
		m_pBuffP2 = new CEnhcBufferDecoder (m_rctRefFrameY.width, m_rctRefFrameY.height ());
		m_pBuffB1 = new CEnhcBufferDecoder (m_rctRefFrameY.width, m_rctRefFrameY.height ());
		m_pBuffB2 = new CEnhcBufferDecoder (m_rctRefFrameY.width, m_rctRefFrameY.height ());
		m_pBuffE  = new CEnhcBufferDecoder (m_rctRefFrameY.width, m_rctRefFrameY.height ());
	}
	// buffers for Temporal Scalabe Decoding	End	 Sharp(1998-02-10)
	m_iClockRateScale = 1; // added by Sharp (98/6/26)

	// Set sprite_transmit_mode to STOP	for the duration of VOL if (fSptUsage () == 0),
	// and later set to PIECE by decode_init_sprite () if (fSptUsage () == 1)
	m_vopmd.SpriteXmitMode = STOP;	  
 	//yrchen initialization of idct 10.21.2003
 	m_pinvdct=new idct;
 	assert(m_pinvdct);
 	m_pinvdct->init();
	
}

// for back/forward shape	Added by Sharp(1998-02-10)
CVideoObjectDecoder::CVideoObjectDecoder (
	Int iDisplayWidth, Int iDisplayHeight
) : CVideoObject ()
{
	m_pistrm = -1;
	m_pbitstrmIn = NULL;
	m_pentrdecSet = NULL;

	m_uiVOId = 0;
	Void set_modes(VOLMode* volmd, VOPMode* vopmd);
	set_modes(&m_volmd, &m_vopmd); // set VOL modes, VOP modes

	m_vopmd.iVopConstantAlphaValue = 255;

	Int iMod = iDisplayWidth % MB_SIZE;
	Int iDisplayWidthRound = (iMod > 0) ? iDisplayWidth + MB_SIZE - iMod : iDisplayWidth;
	iMod = iDisplayHeight % MB_SIZE;
	Int iDisplayHeightRound = (iMod > 0) ? iDisplayHeight + MB_SIZE - iMod : iDisplayHeight;
	m_rctRefFrameY = CRct (
		-EXPANDY_REF_FRAME, -EXPANDY_REF_FRAME,
		EXPANDY_REF_FRAME + iDisplayWidthRound, EXPANDY_REF_FRAME + iDisplayHeightRound
	);
	m_rctRefFrameUV = m_rctRefFrameY.downSampleBy2 ();
	allocateVOLMembers (iDisplayWidth, iDisplayHeight);

	// HHI Schueuer
	if (m_volmd.bSadctDisable)
		m_pscanSelector = new CInvScanSelector;
	else
		m_pscanSelector = new CInvScanSelectorForSADCT(m_rgiCurrMBCoeffWidth);
	//end HHI

	// buffer for shape decoding
	m_pvopcRightMB = new CVOPU8YUVBA (m_volmd.fAUsage, CRct (0, 0, MB_SIZE, MB_SIZE), m_volmd.iAuxCompCount);
	m_ppxlcRightMBBY = (PixelC*) m_pvopcRightMB->pixelsBY ();
	m_ppxlcRightMBBUV = (PixelC*) m_pvopcRightMB->pixelsBUV ();
 	
 	//yrchen initialization of idct 10.21.2003
 	m_pinvdct = new idct;
 	assert(m_pinvdct);
 	m_pinvdct->init();
}
// for back/forward shape	End	 Sharp(1998-02-10)

Int CVideoObjectDecoder::decode (const CVOPU8YUVBA* pvopcBVOPQuant, /*strstreambuf* pistrm */
				 Bool waitForI, Bool drop)
{
#if 0
	if (pistrm != NULL) {
		delete (istream *)m_pistrm;
		delete m_pbitstrmIn;
		delete m_pentrdecSet;
		m_pistrm = (ifstream *)new istream (pistrm);
		m_pbitstrmIn = new CInBitStream (*m_pistrm);
		m_pentrdecSet = new CEntropyDecoderSet (*m_pbitstrmIn);
	}
#endif

// RRV
	m_iRRVScale	= 1;	// default value
// ~RRV

	//sprite piece should not come here
	ASSERT ((m_vopmd.SpriteXmitMode == STOP) || ( m_vopmd.SpriteXmitMode == PAUSE));

	if (findStartCode () == EOF)
		return EOF;

	UInt uiCheck = m_pbitstrmIn -> peekBits (NUMBITS_VOP_START_CODE);
	if(uiCheck==VSS_END_CODE)
		return EOF;

	Bool bCoded = decodeVOPHead (); // set the bounding box here
	
	if (waitForI && 
	    !(m_vopmd.vopPredType == IVOP)) {
	  return -1;
	}
	if (drop && m_vopmd.vopPredType == BVOP) {
	  return -1;
	}
	//cout << "\t" << "Time..." << m_t << " (" << m_t / (double)m_volmd.iClockRate << " sec)\n";
#ifdef DEBUG_OUTPUT
	if(bCoded == FALSE)
		cout << "\tNot coded.\n";
	cout.flush ();
#endif

	Bool bPrevRefVopWasCoded = m_bCodedFutureRef;
	if(m_vopmd.vopPredType==IVOP || m_vopmd.vopPredType==PVOP || (m_uiSprite == 2 && m_vopmd.vopPredType==SPRITE)) // GMC
		m_bCodedFutureRef = bCoded; // flag used by bvop prediction

	if (m_vopmd.vopPredType == SPRITE && m_uiSprite == 1)   { // GMC
		decodeSpt ();
		return TRUE;
	}

	// set time stamps for Base/Temporal-Enhc/Spatial-Enhc Layer	Modified by Sharp(1998-02-10)
	if(m_volmd.volType == BASE_LAYER) {
		if(m_vopmd.vopPredType==IVOP || m_vopmd.vopPredType==PVOP || (m_uiSprite == 2 && m_vopmd.vopPredType==SPRITE)) { // GMC
			if(bPrevRefVopWasCoded)
				m_tPastRef = m_tFutureRef;
			m_tFutureRef = m_t;
			m_iBCount = 0;
		}
		// count B-VOPs
		if(m_vopmd.vopPredType==BVOP)
			m_iBCount++;
	}
	else if (pvopcBVOPQuant != NULL) {	// Spatial Scalability Enhancement Layer
/* (98/03/30) modified by SONY */
		if(m_vopmd.vopPredType == BVOP && m_vopmd.iRefSelectCode == 0) {
			if(bPrevRefVopWasCoded)
				m_tPastRef = m_tFutureRef;
			m_tFutureRef = m_t;
			m_iBCount = 0;
		} if(m_vopmd.vopPredType == PVOP && m_vopmd.iRefSelectCode == 3) {
			m_tPastRef = m_t;
			m_tFutureRef = m_t;
			m_iBCount = 0;
		}
/* (98/03/30) modified by SONY */

		// count B-VOPs
		if(m_vopmd.vopPredType==BVOP)
			m_iBCount++;
	}
	// set time stamps for Base/Temporal-Enhc/Spatial-Enhc Layer	End 	    Sharp(1998-02-10)

	// select reference frames for Base/Temporal-Enhc/Spatial-Enhc Layer	Modified by Sharp(1998-02-10)
	if(bPrevRefVopWasCoded)
	{
		if(m_volmd.volType == BASE_LAYER) {
			updateAllRefVOPs ();            // update all reconstructed VOP's
		}
		else {
//OBSS_SAIT_991015
			if (pvopcBVOPQuant == NULL && !m_volmd.bSpatialScalability)		// Temporal Scalability Enhancement Layer
				updateRefVOPsEnhc ();
			else {					// Spatial Scalability Enhancement Layer
				if (pvopcBVOPQuant != NULL && m_volmd.bSpatialScalability)	
					updateAllRefVOPs (pvopcBVOPQuant);
			}
//~OBSS_SAIT_991015
		}
	}
//OBSS_SAIT_991015
	else if(m_volmd.volType == ENHN_LAYER && m_volmd.bSpatialScalability && pvopcBVOPQuant != NULL)	// Spatial Scalability Enhancement Layer
		updateAllRefVOPs (pvopcBVOPQuant);
//~OBSS_SAIT_991015
	// select reference frames for Base/Temporal-Enhc/Spatial-Enhc Layer	End	    Sharp(1998-02-10)

	switch(m_vopmd.vopPredType)
	{
	case IVOP:
#ifdef DEBUG_OUTPUT
		cout << "\tIVOP";
#endif
/*Added by SONY (98/03/30)*/
		if(m_bLinkisBroken == TRUE && m_bUseGOV == TRUE)        m_bLinkisBroken = FALSE;
/*Added by SONY (98/03/30) END*/
		break;
	case PVOP:
#ifdef DEBUG_OUTPUT
		cout << "\tPVOP (reference: t=" << m_tPastRef <<")";
#endif
		break;
// GMC
	case SPRITE:
#ifdef DEBUG_OUTPUT
		cout << "\tSVOP(GMC) (reference: t=" << m_tPastRef <<")";
#endif
		break;
// ~GMC
	case BVOP:
#ifdef DEBUG_OUTPUT
		cout << "\tBVOP (past ref: t=" << m_tPastRef
			<< ", future ref: t=" << m_tFutureRef <<")";
#endif
		break;
	default:
		break;
	}
#ifdef DEBUG_OUTPUT
	cout << "\n";
	cout.flush ();
#endif
/* Added by SONY (98/03/30)*/
	if(m_bLinkisBroken == TRUE && m_bUseGOV == TRUE)
		fprintf(stderr,"WARNING: broken_link = 1  --- Output image must be broken.\n");
/*Added by SONY (98/03/30) End*/

	if(bCoded==FALSE)
	{
		if (m_vopmd.vopPredType != BVOP
			&& m_volmd.fAUsage != RECTANGLE && bPrevRefVopWasCoded)
		{
			// give the current object a dummy size
			m_iNumMBX = m_iNumMBY = m_iNumMB = 1;
			saveShapeMode(); // save the previous reference vop shape mode
		}
//OBSS_SAIT_991015
		if (m_volmd.fAUsage != RECTANGLE){								
			// give the current object a dummy size						
			m_iNumMBX = m_iNumMBY = m_iNumMB = 1;						
			saveBaseShapeMode(); // save the base layer shape mode		
		}																
//~OBSS_SAIT_991015
		return FALSE;
	}

	if (m_volmd.fAUsage != RECTANGLE)
		resetBYPlane ();

	if (m_volmd.fAUsage != RECTANGLE) {
		setRefStartingPointers ();
		computeVOPMembers ();
	}

// RRV insertion
	redefineVOLMembersRRV ();
// ~RRV	

	decodeVOP ();

	//wchen: added by sony-probably not the best way
	if(m_volmd.volType == ENHN_LAYER &&
		(m_vopmd.vopPredType == BVOP && m_vopmd.iRefSelectCode == 0))
		swapVOPU8Pointers(m_pvopcCurrQ,m_pvopcRefQ1);

//OBSSFIX_MODE3
	//Case for base layer has rectangular shape & enhancement layer has arbitrary shape
    if(m_pvopcCurrQ->fAUsage() == RECTANGLE &&   m_pvopcCurrQ->fAUsage() != m_pvopcRefQ1->fAUsage() ){
     delete m_pvopcCurrQ;
     m_pvopcCurrQ = new CVOPU8YUVBA(m_volmd.fAUsage, m_rctRefFrameY, m_volmd.iAuxCompCount);
    }
//~OBSSFIX_MODE3

	// store the direct mode data
	if (m_vopmd.vopPredType != BVOP ||
		(m_volmd.volType == ENHN_LAYER && m_vopmd.vopPredType == BVOP && m_vopmd.iRefSelectCode == 0)) {
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
		m_rgmvBackward = m_rgmv + BVOP_MV_PER_REF_PER_MB * m_iSessNumMB;
	}
//OBSS_SAIT_991015
	else if (m_volmd.volType == BASE_LAYER && m_vopmd.vopPredType == BVOP ){
		if( m_volmd.fAUsage != RECTANGLE) 
			saveBaseShapeMode();	
	}
//~OBSS_SAIT_991015

	if (m_volmd.fAUsage != RECTANGLE)	{
		if (m_vopmd.vopPredType != BVOP ||
			(m_volmd.volType == ENHN_LAYER && m_vopmd.vopPredType == BVOP && m_vopmd.iRefSelectCode == 0)) {
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
		else {				// For Temporal Scalability	Added by Sharp(1998-02-10)
//OBSS_SAIT_991015		//for Base layer BVOP padding 
		    if (m_volmd.volType == BASE_LAYER && m_vopmd.vopPredType == BVOP) {
				if(!m_volmd.bShapeOnly){	
					m_iOffsetForPadY = m_rctRefFrameY.offset (m_rctCurrVOPY.left, m_rctCurrVOPY.top);
					m_iOffsetForPadUV = m_rctRefFrameUV.offset (m_rctCurrVOPUV.left, m_rctCurrVOPUV.top);
					m_rctPrevNoExpandY = m_rctCurrVOPY;
					m_rctPrevNoExpandUV = m_rctCurrVOPUV;
				}
			}
//~OBSS_SAIT_991015		
			m_iBVOPOffsetForPadY = m_rctRefFrameY.offset (m_rctCurrVOPY.left, m_rctCurrVOPY.top);
			m_iBVOPOffsetForPadUV = m_rctRefFrameUV.offset (m_rctCurrVOPUV.left, m_rctCurrVOPUV.top);
			m_rctBVOPPrevNoExpandY = m_rctCurrVOPY;
			m_rctBVOPPrevNoExpandUV = m_rctCurrVOPUV;

			m_rctBVOPRefVOPY1 = m_rctCurrVOPY;
			m_rctBVOPRefVOPY1.expand (EXPANDY_REFVOP);
			m_rctBVOPRefVOPUV1 = m_rctCurrVOPUV;
			m_rctBVOPRefVOPUV1.expand (EXPANDUV_REFVOP);
		}				// For Temporal Scalability	End	 Sharp(1998-02-10)

		//give a comment that this is ac/dc pred stuff
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
	else {				// For Temporal Scalability	Added by Sharp(1998-02-10)
		m_iBVOPOffsetForPadY = m_iOffsetForPadY;
		m_iBVOPOffsetForPadUV = m_iOffsetForPadUV;
		m_rctBVOPPrevNoExpandY = m_rctPrevNoExpandY;
		m_rctBVOPPrevNoExpandUV = m_rctPrevNoExpandUV;

		m_rctBVOPRefVOPY1 = m_rctRefVOPY1;
		m_rctBVOPRefVOPUV1 = m_rctRefVOPUV1;
	}				// For Temporal Scalability	End	 Sharp(1998-02-10)


    if (m_vopmd.vopPredType != BVOP ||
		(m_volmd.volType == ENHN_LAYER && m_vopmd.vopPredType == BVOP && m_vopmd.iRefSelectCode == 0)) {
		if(!m_volmd.bShapeOnly){	//OBSS_SAIT_991015
			repeatPadYOrA ((PixelC*) m_pvopcRefQ1->pixelsY () + m_iOffsetForPadY, m_pvopcRefQ1);
			repeatPadUV (m_pvopcRefQ1);
		}							//OBSS_SAIT_991015

		//reset by in RefQ1 so that no left-over from last frame
		if (m_volmd.fAUsage != RECTANGLE)       {
      if (m_volmd.fAUsage == EIGHT_BIT) {
        for(Int iAuxComp=0; iAuxComp<m_volmd.iAuxCompCount; iAuxComp++ ) { // MAC (SB) 1-Dec-99
				  repeatPadYOrA ((PixelC*) m_pvopcRefQ1->pixelsA (iAuxComp) + m_iOffsetForPadY, m_pvopcRefQ1);
        }
      }
		}
	}
//OBSS_SAIT_991015	//Base layer BVOP padding for OBSS
    if (m_volmd.volType == BASE_LAYER && m_vopmd.vopPredType == BVOP) {
		if(!m_volmd.bShapeOnly){	
			repeatPadYOrA ((PixelC*) m_pvopcCurrQ->pixelsY () + m_iOffsetForPadY, m_pvopcCurrQ);
			repeatPadUV (m_pvopcCurrQ);
		}

		if (m_volmd.fAUsage != RECTANGLE)  {
      if (m_volmd.fAUsage == EIGHT_BIT) {
        for(Int iAuxComp=0; iAuxComp<m_volmd.iAuxCompCount; iAuxComp++ ) { // MAC (SB) 1-Dec-99
				  repeatPadYOrA ((PixelC*) m_pvopcCurrQ->pixelsA (iAuxComp) + m_iOffsetForPadY, m_pvopcCurrQ);
        }
      }
		}
	}
//~OBSS_SAIT_991015	
	// update buffers for temporal scalability	Added by Sharp(1998-02-10)
	if(m_volmd.volType != BASE_LAYER)  updateBuffVOPsEnhc ();

	return TRUE;
}

Int CVideoObjectDecoder::findStartCode( int dont_loop )
{
	// ensure byte alignment
	m_pbitstrmIn->flush ();

	Int bUserData;
	do {
		bUserData = 0;
		while(m_pbitstrmIn->peekBits(NUMBITS_START_CODE_PREFIX)!=START_CODE_PREFIX)
		{
			m_pbitstrmIn->getBits(8);
			if(m_pbitstrmIn->eof()==EOF || dont_loop != 0)
				return EOF;
		}
		m_pbitstrmIn->getBits(NUMBITS_START_CODE_PREFIX);
		if(m_pbitstrmIn->peekBits(NUMBITS_START_CODE_SUFFIX)==USER_DATA_START_CODE)
			bUserData = 1;
	} while(bUserData);

	return 0;
}


Int CVideoObjectDecoder::decodeVSHead ()
{
	UInt uiSC = m_pbitstrmIn->getBits (NUMBITS_START_CODE_SUFFIX);
	if (uiSC != VSS_START_CODE)
		return 1;

	// visual session header
	/*UInt uiProfile = */m_pbitstrmIn->getBits (NUMBITS_VSS_PROFILE);

	if (findStartCode())
		return 1;

	uiSC = m_pbitstrmIn->getBits (NUMBITS_START_CODE_SUFFIX);
	if (uiSC != VSO_START_CODE)
		return 1;

	// visual object header
	UInt uiIsVisualObjectIdent = m_pbitstrmIn->getBits (1);
	if (uiIsVisualObjectIdent) {
		UInt uiVSOVerID = m_pbitstrmIn->getBits (NUMBITS_VSO_VERID);
		if (uiVSOVerID != VSO_VERID)
			return 1;
		/* UInt uiVSOPriority = */m_pbitstrmIn->getBits (NUMBITS_VSO_PRIORITY);
	}
	UInt uiVSOType = m_pbitstrmIn->getBits (NUMBITS_VSO_TYPE);
	if (uiVSOType != VSO_TYPE)
		return 1;
	UInt uiVideoSignalType = m_pbitstrmIn->getBits (1);
	if (uiVideoSignalType) {
	  /*UInt uiFormat = */m_pbitstrmIn->getBits (3);
	  /*UInt uiRange = */ m_pbitstrmIn->getBits (1);
		UInt uiColor = m_pbitstrmIn->getBits (1);
		if (uiColor) {
		  /*UInt uiPrimaries = */m_pbitstrmIn->getBits (8);
		  /* UInt uiCharacter = */m_pbitstrmIn->getBits (8);
		  /*UInt uiMatrix = */m_pbitstrmIn->getBits (8);
		}
	}

	if (findStartCode())
		return 1;
	
	return 0;
}

Void CVideoObjectDecoder::decodeVOHead ()
{
	findStartCode();
 	if( m_pbitstrmIn->peekBits(NUMBITS_START_CODE_SUFFIX)==VSS_START_CODE){
		if(decodeVSHead())
			exit(fprintf(stderr,"Failed to decoder visual sequence headers\n"));
 	}
 	//	End Toshiba(1998-1-16:DP+RVLC)
	UInt uiVoStartCode = m_pbitstrmIn -> getBits (NUMBITS_VO_START_CODE);
	ASSERT(uiVoStartCode == VO_START_CODE);

	m_uiVOId = m_pbitstrmIn -> getBits (NUMBITS_VO_ID);
}

Void CVideoObjectDecoder::decodeVOLHead ()
{
	findStartCode();
	UInt uiVolStartCode = m_pbitstrmIn -> getBits (NUMBITS_VOL_START_CODE);
	ASSERT(uiVolStartCode == VOL_START_CODE);
	decodeVOLBody();
}

Void CVideoObjectDecoder::decodeVOLBody()
{
   /* UInt uiVOLId =  wmay */ m_pbitstrmIn -> getBits (NUMBITS_VOL_ID);
// Begin: modified by Hughes	  4/9/98	  per clause 2.1.7. in N2171 document
   /* Bool bRandom = wmay */  m_pbitstrmIn->getBits (1); //VOL_Random_Access
// End: modified by Hughes	  4/9/98
   /* UInt uiOLType = wmay */ m_pbitstrmIn -> getBits(8); // VOL_type_indication
	UInt uiOLI = m_pbitstrmIn -> getBits (1); //VOL_Is_Object_Layer_Identifier, useless flag for now
	if(uiOLI)
	{
// GMC
		m_volmd.uiVerID = m_pbitstrmIn -> getBits (4); // video_oject_layer_verid
	    // Here, is_object_layer_identifier is used for Version1/Version2
	    // identification at this moment (tentative solution).
	    // vol_type_indicator is not useless for version 2 at present.
	    // need discussion at Video Group about this issue.
//		m_pbitstrmIn -> getBits (4); // video_oject_layer_verid
		m_pbitstrmIn -> getBits (3); // video_oject_layer_priority
	}
	else{
		m_volmd.uiVerID = 1;
// ~GMC
	}
	//ASSERT(uiOLI == 0);
	
	m_ivolAspectRatio = m_pbitstrmIn -> getBits (4);
	if(m_ivolAspectRatio==15) // extended PAR
	{
	  m_ivolAspectWidth = m_pbitstrmIn -> getBits (8);
	  m_ivolAspectHeight = m_pbitstrmIn -> getBits (8);
	}

	UInt uiMark;
	UInt uiCTP = m_pbitstrmIn -> getBits (1); //VOL_Control_Parameter, useless flag for now
	if(uiCTP)
	{
	  /* UInt uiChromaFormat = */ m_pbitstrmIn -> getBits (2);
	  /* UInt uiLowDelay = */ m_pbitstrmIn -> getBits (1);
		UInt uiVBVParams = m_pbitstrmIn -> getBits (1);
		
		if(uiVBVParams)
		{
		  /* UInt uiFirstHalfBitRate = */  m_pbitstrmIn -> getBits (15);
			uiMark = m_pbitstrmIn -> getBits (1);
			ASSERT(uiMark==1);
			/* UInt uiLatterHalfBitRate = */ m_pbitstrmIn -> getBits (15);
			uiMark = m_pbitstrmIn -> getBits (1);
			ASSERT(uiMark==1);
			/* UInt uiFirstHalfVbvBufferSize = */ m_pbitstrmIn -> getBits (15);
			uiMark = m_pbitstrmIn -> getBits (1);
			ASSERT(uiMark==1);
			/* UInt uiLatterHalfVbvBufferSize = */m_pbitstrmIn -> getBits (3);
			/* UInt uiFirstHalfVbvBufferOccupany = */m_pbitstrmIn -> getBits (11);
			uiMark = m_pbitstrmIn -> getBits (1);
			ASSERT(uiMark==1);
			/* UInt uiLatterHalfVbvBufferOccupany = */ m_pbitstrmIn -> getBits (15);
			uiMark = m_pbitstrmIn -> getBits (1);
			ASSERT(uiMark==1);
		}
	}


  UInt uiAUsage = m_pbitstrmIn -> getBits (NUMBITS_VOL_SHAPE);

// MAC (SB) 1-Dec-99
  if (uiAUsage==3) { // gray scale
    if (m_volmd.uiVerID!=1) {
      m_volmd.iAlphaShapeExtension = m_pbitstrmIn -> getBits (4);
      m_volmd.iAuxCompCount =  CVideoObject::getAuxCompCount(m_volmd.iAlphaShapeExtension);
    } else {
      m_volmd.iAuxCompCount = 1;
    }
  } else
    m_volmd.iAuxCompCount = 0;
//~MAC

	uiMark = m_pbitstrmIn -> getBits (1);
	ASSERT(uiMark==1);
	m_volmd.iClockRate = m_pbitstrmIn -> getBits (NUMBITS_TIME_RESOLUTION);
#ifdef DEBUG_OUTPUT
	cout << m_volmd.iClockRate << "\n";
#endif
	uiMark = m_pbitstrmIn -> getBits (1);
	ASSERT(uiMark==1);
#if 0
	Int iClockRate = m_volmd.iClockRate - 1;
#else
	Int iClockRate = m_volmd.iClockRate;
#endif
	ASSERT (iClockRate < 65536);
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

	Bool bFixFrameRate = m_pbitstrmIn -> getBits (1);
	//ASSERT (bFixFrameRate == FALSE);
	if(bFixFrameRate)
	{
		UInt uiFixedVOPTimeIncrement = 1;
		if(m_iNumBitsTimeIncr!=0)
			uiFixedVOPTimeIncrement = m_pbitstrmIn -> getBits (m_iNumBitsTimeIncr) + 1;
		m_volmd.iClockRate = m_volmd.iClockRate / uiFixedVOPTimeIncrement;
		// not used
		//
		//
	}

	if(uiAUsage==2)  // shape-only mode
	{
//OBSS_SAIT_991015
		if(m_volmd.uiVerID == 2) {
			m_volmd.volType = (m_pbitstrmIn -> getBits (1) == 0) ? BASE_LAYER : ENHN_LAYER;
			m_volmd.iEnhnType = 0;			//OBSSFIX_BSO
			m_volmd.iHierarchyType = 0;		//OBSSFIX_BSO
			m_volmd.ihor_sampling_factor_n = 1;
			m_volmd.ihor_sampling_factor_m = 1;
			m_volmd.iver_sampling_factor_n = 1;
			m_volmd.iver_sampling_factor_m = 1;	
			m_volmd.ihor_sampling_factor_n_shape = 1;
			m_volmd.ihor_sampling_factor_m_shape = 1;
			m_volmd.iver_sampling_factor_n_shape = 1;
			m_volmd.iver_sampling_factor_m_shape = 1;	
			if (m_volmd.volType == ENHN_LAYER)	{
				m_volmd.ihor_sampling_factor_n_shape = m_pbitstrmIn->getBits (5);
				m_volmd.ihor_sampling_factor_m_shape = m_pbitstrmIn->getBits (5);
				m_volmd.iver_sampling_factor_n_shape = m_pbitstrmIn->getBits (5);
				m_volmd.iver_sampling_factor_m_shape = m_pbitstrmIn->getBits (5);
				m_volmd.ihor_sampling_factor_n = m_volmd.ihor_sampling_factor_n_shape;
				m_volmd.ihor_sampling_factor_m = m_volmd.ihor_sampling_factor_m_shape;
				m_volmd.iver_sampling_factor_n = m_volmd.iver_sampling_factor_n_shape;
				m_volmd.iver_sampling_factor_m = m_volmd.iver_sampling_factor_m_shape;	
			}
		}
//~OBSS_SAIT_991015
		/* UInt uiResyncMarkerDisable = */ m_pbitstrmIn -> getBits (1);

		// default to some values - probably not all needed
		m_volmd.bShapeOnly=TRUE;
		m_volmd.fAUsage=ONE_BIT;
		m_volmd.bAdvPredDisable = 0;
		m_volmd.fQuantizer = Q_H263;
//OBSS_SAIT_991015
		m_volmd.bSadctDisable = 1;
		m_volmd.bNewpredEnable = 0;
		m_volmd.bQuarterSample = 0;
//~OBSS_SAIT_991015
		m_volmd.bDeblockFilterDisable = TRUE;		
		m_uiSprite = 0;
		m_volmd.bNot8Bit = 0;
		m_volmd.bComplexityEstimationDisable = 1;
		m_volmd.bDataPartitioning = 0;
		m_volmd.bReversibleVlc = FALSE;
		m_volmd.bDeblockFilterDisable = TRUE;
		return;
	}

	m_volmd.bShapeOnly=FALSE;
	if(uiAUsage==3)
		uiAUsage=2;
	m_volmd.fAUsage = (AlphaUsage) uiAUsage;
	if (m_volmd.fAUsage == RECTANGLE) {
		UInt uiMarker = m_pbitstrmIn -> getBits (1);
		// wmay for divx ASSERT(uiMarker==1);
		m_ivolWidth = m_pbitstrmIn -> getBits (NUMBITS_VOP_WIDTH);
		uiMarker  = m_pbitstrmIn -> getBits (1);
		// wmay for divx ASSERT(uiMarker==1);
		m_ivolHeight = m_pbitstrmIn -> getBits (NUMBITS_VOP_HEIGHT);
		uiMarker  = m_pbitstrmIn -> getBits (1);
		// wmay for divx ASSERT(uiMarker==1);
	}

	m_vopmd.bInterlace = m_pbitstrmIn -> getBits (1); // interlace (was vop flag)
	m_volmd.bAdvPredDisable = m_pbitstrmIn -> getBits (1);  //VOL_obmc_Disable

	// decode sprite info
// GMC
	if(m_volmd.uiVerID == 1)
// ~GMC
		m_uiSprite = m_pbitstrmIn -> getBits (1);
// GMC
	else // wmay - this is wrong...if(m_volmd.uiVerID == 2)
		m_uiSprite = m_pbitstrmIn -> getBits (2);
// ~GMC
	if (m_uiSprite == 1) { // sprite information
		Int isprite_hdim = m_pbitstrmIn -> getBits (NUMBITS_SPRITE_HDIM);
		Int iMarker = m_pbitstrmIn -> getBits (MARKER_BIT);
		ASSERT (iMarker == 1);
		Int isprite_vdim = m_pbitstrmIn -> getBits (NUMBITS_SPRITE_VDIM);
		iMarker = m_pbitstrmIn -> getBits (MARKER_BIT);
		ASSERT (iMarker == 1);
		Int isprite_left_edge = (m_pbitstrmIn -> getBits (1) == 0) ?
		    m_pbitstrmIn->getBits (NUMBITS_SPRITE_LEFT_EDGE - 1) : ((Int)m_pbitstrmIn->getBits (NUMBITS_SPRITE_LEFT_EDGE - 1) - (1 << 12));
		ASSERT(isprite_left_edge%2 == 0);
		iMarker = m_pbitstrmIn -> getBits (MARKER_BIT);
		ASSERT (iMarker == 1);
		Int isprite_top_edge = (m_pbitstrmIn -> getBits (1) == 0) ?
		    m_pbitstrmIn->getBits (NUMBITS_SPRITE_TOP_EDGE - 1) : ((Int)m_pbitstrmIn->getBits (NUMBITS_SPRITE_LEFT_EDGE - 1) - (1 << 12));
		ASSERT(isprite_top_edge%2 == 0);
		iMarker = m_pbitstrmIn -> getBits (MARKER_BIT);
		ASSERT (iMarker == 1);
		m_rctSpt.left = isprite_left_edge;
		m_rctSpt.right = isprite_left_edge + isprite_hdim;
		m_rctSpt.top = isprite_top_edge;
		m_rctSpt.bottom = isprite_top_edge + isprite_vdim;
		m_rctSpt.width = isprite_hdim;
        m_rctSptPieceY = m_rctSpt; //initialization; will be overwritten by first vop header
// GMC
	}
	if (m_uiSprite == 1 || m_uiSprite == 2) { // sprite information
// ~GMC
		m_iNumOfPnts = m_pbitstrmIn -> getBits (NUMBITS_NUM_SPRITE_POINTS);
// GMC_V2
		if(m_uiSprite == 2)
			ASSERT (m_iNumOfPnts == 0 ||
				m_iNumOfPnts == 1 ||
				m_iNumOfPnts == 2 ||
				m_iNumOfPnts == 3);
// ~GM_V2
		m_rgstSrcQ = new CSiteD [m_iNumOfPnts];
		m_rgstDstQ = new CSiteD [m_iNumOfPnts];
		m_uiWarpingAccuracy = m_pbitstrmIn -> getBits (NUMBITS_WARPING_ACCURACY);
		/* Bool bLightChange = */ m_pbitstrmIn -> getBits (1);
// GMC
	}
	if (m_uiSprite == 1) { // sprite information
// ~GMC

// Begin: modified by Hughes	  4/9/98
		Bool bsptMode = m_pbitstrmIn -> getBits (1);  // Low_latency_sprite_enable
		if (  bsptMode)
			m_sptMode = LOW_LATENCY ;
		else			
			m_sptMode = BASIC_SPRITE ;
// End: modified by Hughes	  4/9/98
	}

// HHI Suehring 991022
// HHI Schueuer: sadct
	m_volmd.bSadctDisable = 1;
    if (m_volmd.fAUsage != RECTANGLE ) {
            if (m_volmd.uiVerID == 1)
                    m_volmd.bSadctDisable = TRUE;
            else
                    m_volmd.bSadctDisable = m_pbitstrmIn -> getBits(1);
	}
// end HHI
// end HHI Suehring 991022

/* 
//OBSS_SAIT_991015
// HHI Schueuer: sadct
	if (m_volmd.fAUsage != RECTANGLE ) {
		if (m_volmd.uiVerID == 1)
			m_volmd.bSadctDisable = TRUE;
		else 
			m_volmd.bSadctDisable = m_pbitstrmIn -> getBits (1);
	}		
// end HHI
//~OBSS_SAIT_991015 
*/
	m_volmd.bNot8Bit = (Bool) m_pbitstrmIn -> getBits(1);
	if (m_volmd.bNot8Bit) {
		m_volmd.uiQuantPrecision = (UInt) m_pbitstrmIn -> getBits (4);
		m_volmd.nBits = (UInt) m_pbitstrmIn -> getBits (4);
		ASSERT(m_volmd.nBits>3);
	} else {
		m_volmd.uiQuantPrecision = 5;
		m_volmd.nBits = 8;
	}

	// wmay - this doesn't look right, either - should be grayscale
	if (m_volmd.fAUsage == EIGHT_BIT)
	{
		m_volmd.bNoGrayQuantUpdate = m_pbitstrmIn -> getBits (1);
		/* UInt uiCompMethod = */m_pbitstrmIn -> getBits (1);
		/*UInt uiLinearComp = */m_pbitstrmIn -> getBits (1);
	}

	m_volmd.fQuantizer = (Quantizer) m_pbitstrmIn -> getBits (1); 
	if (m_volmd.fQuantizer == Q_MPEG) {
		m_volmd.bLoadIntraMatrix = m_pbitstrmIn -> getBits (1);
		if (m_volmd.bLoadIntraMatrix) {
			UInt i = 0;
			Int iElem;
			do {
				iElem = m_pbitstrmIn -> getBits (NUMBITS_QMATRIX);
				m_volmd.rgiIntraQuantizerMatrix [grgiStandardZigzag[i]] = iElem;
			} while (iElem != 0 && ++i < BLOCK_SQUARE_SIZE);
			for (UInt j = i; j < BLOCK_SQUARE_SIZE; j++) {
				m_volmd.rgiIntraQuantizerMatrix [grgiStandardZigzag[j]] = m_volmd.rgiIntraQuantizerMatrix [grgiStandardZigzag[i - 1]];
			}
		}
		else {
			memcpy (m_volmd.rgiIntraQuantizerMatrix, rgiDefaultIntraQMatrix, BLOCK_SQUARE_SIZE * sizeof (Int));
		}
		m_volmd.bLoadInterMatrix = m_pbitstrmIn -> getBits (1);
		if (m_volmd.bLoadInterMatrix) {
			UInt i = 0;
			Int iElem;
			do {
				iElem = m_pbitstrmIn -> getBits (NUMBITS_QMATRIX);
				m_volmd.rgiInterQuantizerMatrix [grgiStandardZigzag[i]] = iElem;
			} while (iElem != 0 && ++i < BLOCK_SQUARE_SIZE);
			for (UInt j = i; j < BLOCK_SQUARE_SIZE; j++) {
				m_volmd.rgiInterQuantizerMatrix [grgiStandardZigzag[j]] = m_volmd.rgiInterQuantizerMatrix [grgiStandardZigzag[i - 1]];
			}
		}
		else {
			memcpy (m_volmd.rgiInterQuantizerMatrix, rgiDefaultInterQMatrix, BLOCK_SQUARE_SIZE * sizeof (Int));
		}
		if (m_volmd.fAUsage == EIGHT_BIT)	{      
      for(Int iAuxComp=0; iAuxComp<m_volmd.iAuxCompCount; iAuxComp++ ) { // MAC (SB) 2-Dec-99
        m_volmd.bLoadIntraMatrixAlpha = m_pbitstrmIn -> getBits (1);
        if (m_volmd.bLoadIntraMatrixAlpha) {
          for (UInt i = 0; i < BLOCK_SQUARE_SIZE; i++)	{
            Int iElem = m_pbitstrmIn -> getBits (NUMBITS_QMATRIX);
            if (iElem != 0)
              m_volmd.rgiIntraQuantizerMatrixAlpha[iAuxComp] [grgiStandardZigzag[i]] = iElem;
            else
              m_volmd.rgiIntraQuantizerMatrixAlpha[iAuxComp] [i] = m_volmd.rgiIntraQuantizerMatrixAlpha[iAuxComp] [grgiStandardZigzag[i - 1]];
          }
        }
        else {
#ifdef _FOR_GSSP_
          // memcpy (m_volmd.rgiIntraQuantizerMatrixAlpha, rgiDefaultIntraQMatrix, BLOCK_SQUARE_SIZE * sizeof (Int));
          // use rgiDefaultIntraQMatrixAlpha instead of rgiDefaultIntraQMatrix (both defined in ./sys/global.hpp), mwi
          memcpy (m_volmd.rgiIntraQuantizerMatrixAlpha[iAuxComp], rgiDefaultIntraQMatrixAlpha, BLOCK_SQUARE_SIZE * sizeof (Int));
#else
          for (UInt i = 0; i < BLOCK_SQUARE_SIZE; i++)
            m_volmd.rgiIntraQuantizerMatrixAlpha[iAuxComp] [i] = 16;
#endif
        }
        m_volmd.bLoadInterMatrixAlpha = m_pbitstrmIn -> getBits (1);
        if (m_volmd.bLoadInterMatrixAlpha) {
          for (UInt i = 0; i < BLOCK_SQUARE_SIZE; i++)	{
            Int iElem = m_pbitstrmIn -> getBits (NUMBITS_QMATRIX);
            if (iElem != 0)
              m_volmd.rgiInterQuantizerMatrixAlpha[iAuxComp] [grgiStandardZigzag[i]] = iElem;
            else
              m_volmd.rgiInterQuantizerMatrixAlpha[iAuxComp] [i] = m_volmd.rgiInterQuantizerMatrixAlpha[iAuxComp] [grgiStandardZigzag[i - 1]];
          }
        }
        else {
#ifdef _FOR_GSSP_
          // memcpy (m_volmd.rgiInterQuantizerMatrixAlpha, rgiDefaultInterQMatrix, BLOCK_SQUARE_SIZE * sizeof (Int));
          // use rgiDefaultInterQMatrixAlpha instead of rgiDefaultInterQMatrix (both defined in ./sys/global.hpp), mwi
          memcpy (m_volmd.rgiInterQuantizerMatrixAlpha[iAuxComp], rgiDefaultInterQMatrixAlpha, BLOCK_SQUARE_SIZE * sizeof (Int));
#else
          for (UInt i = 0; i < BLOCK_SQUARE_SIZE; i++)
            m_volmd.rgiInterQuantizerMatrixAlpha[iAuxComp] [i] = 16;
#endif
        }
      }
		}
	}
	
// GMC
    if(m_volmd.uiVerID == 1)
// added for compatibility with version 1 video
// this is tentative integration
// please change codes if there is any problems
	m_volmd.bQuarterSample = 0; //Quarter sample	
    else// wmay - same error - if(m_volmd.uiVerID == 2)
// ~GMC
	m_volmd.bQuarterSample = m_pbitstrmIn -> getBits (1); //Quarter sample	

	// Bool bComplxityEsti = m_pbitstrmIn->getBits (1); //Complexity estimation; don't know how to use it

// START: Complexity Estimation syntax support - Marc Mongenet (EPFL) - 15 Jun 1998

	m_volmd.bComplexityEstimationDisable = m_pbitstrmIn -> getBits (1);
	if (! m_volmd.bComplexityEstimationDisable) {

		m_volmd.iEstimationMethod = m_pbitstrmIn -> getBits (2);
		// START: Complexity Estimation syntax support - Update version 2 - Massimo Ravasi (EPFL) - 5 Nov 1999
		if ( (m_volmd.iEstimationMethod != 0) && (m_volmd.iEstimationMethod != 1)) {
		//// Replaced line: if (m_volmd.iEstimationMethod != 0) {
		// END: Complexity Estimation syntax support - Update version 2
			fprintf (stderr, "ERROR: Unknown complexity estimation method number %d.\n", m_volmd.iEstimationMethod);
			exit (1);
		}

		m_volmd.bShapeComplexityEstimationDisable = m_pbitstrmIn -> getBits (1);
		if (! m_volmd.bShapeComplexityEstimationDisable) {
			m_volmd.bOpaque = m_pbitstrmIn -> getBits (1);
			m_volmd.bTransparent = m_pbitstrmIn -> getBits (1);
			m_volmd.bIntraCAE = m_pbitstrmIn -> getBits (1);
			m_volmd.bInterCAE = m_pbitstrmIn -> getBits (1);
			m_volmd.bNoUpdate = m_pbitstrmIn -> getBits (1);
			m_volmd.bUpsampling = m_pbitstrmIn -> getBits (1);
			if (!(m_volmd.bOpaque ||
				  m_volmd.bTransparent ||
				  m_volmd.bIntraCAE ||
				  m_volmd.bInterCAE ||
				  m_volmd.bNoUpdate ||
				  m_volmd.bUpsampling)) {
				fatal_error("Shape complexity estimation is enabled,\nbut no corresponding flag is enabled.");
			}
		}
		else
			m_volmd.bOpaque =
			m_volmd.bTransparent =
			m_volmd.bIntraCAE =
			m_volmd.bInterCAE =
			m_volmd.bNoUpdate =
			m_volmd.bUpsampling = false;

		m_volmd.bTextureComplexityEstimationSet1Disable = m_pbitstrmIn -> getBits (1);
// GMC
		if(m_uiSprite == 2) ASSERT(m_volmd.bTextureComplexityEstimationSet1Disable == TRUE);
// ~GMC
		if (! m_volmd.bTextureComplexityEstimationSet1Disable) {
			m_volmd.bIntraBlocks = m_pbitstrmIn -> getBits (1);
			m_volmd.bInterBlocks = m_pbitstrmIn -> getBits (1);
			m_volmd.bInter4vBlocks = m_pbitstrmIn -> getBits (1);
			m_volmd.bNotCodedBlocks = m_pbitstrmIn -> getBits (1);
			if (!(m_volmd.bIntraBlocks ||
				  m_volmd.bInterBlocks ||
				  m_volmd.bInter4vBlocks ||
				  m_volmd.bNotCodedBlocks)) {
				fatal_error("Texture complexity estimation set 1 is enabled,\nbut no corresponding flag is enabled.");
			}
		}
		else
			m_volmd.bIntraBlocks =
			m_volmd.bInterBlocks =
			m_volmd.bInter4vBlocks =
			m_volmd.bNotCodedBlocks = false;
		
		uiMark = m_pbitstrmIn -> getBits (1);
		ASSERT (uiMark == 1);

		m_volmd.bTextureComplexityEstimationSet2Disable = m_pbitstrmIn -> getBits (1);
		if (! m_volmd.bTextureComplexityEstimationSet2Disable) {
			m_volmd.bDCTCoefs = m_pbitstrmIn -> getBits (1);
			m_volmd.bDCTLines = m_pbitstrmIn -> getBits (1);
			m_volmd.bVLCSymbols = m_pbitstrmIn -> getBits (1);
			m_volmd.bVLCBits = m_pbitstrmIn -> getBits (1);
			if (!(m_volmd.bDCTCoefs ||
				  m_volmd.bDCTLines ||
				  m_volmd.bVLCSymbols ||
				  m_volmd.bVLCBits)) {
				fatal_error("Texture complexity estimation set 2 is enabled,\nbut no corresponding flag is enabled.");
			}
		}
		else
			m_volmd.bDCTCoefs =
			m_volmd.bDCTLines =
			m_volmd.bVLCSymbols =
			m_volmd.bVLCBits = false;

		m_volmd.bMotionCompensationComplexityDisable = m_pbitstrmIn -> getBits (1);
		if (! m_volmd.bMotionCompensationComplexityDisable) {
			m_volmd.bAPM = m_pbitstrmIn -> getBits (1);
			m_volmd.bNPM = m_pbitstrmIn -> getBits (1);
			m_volmd.bInterpolateMCQ = m_pbitstrmIn -> getBits (1);
			m_volmd.bForwBackMCQ = m_pbitstrmIn -> getBits (1);
			m_volmd.bHalfpel2 = m_pbitstrmIn -> getBits (1);
			m_volmd.bHalfpel4 = m_pbitstrmIn -> getBits (1);
			if (!(m_volmd.bAPM ||
				  m_volmd.bNPM ||
				  m_volmd.bInterpolateMCQ ||
				  m_volmd.bForwBackMCQ ||
				  m_volmd.bHalfpel2 ||
				  m_volmd.bHalfpel4)) {
				fatal_error("Motion complexity estimation is enabled,\nbut no corresponding flag is enabled.");
			}
		}
		else
			m_volmd.bAPM =
			m_volmd.bNPM =
			m_volmd.bInterpolateMCQ =
			m_volmd.bForwBackMCQ =
			m_volmd.bHalfpel2 =
			m_volmd.bHalfpel4 = false;
		
		uiMark = m_pbitstrmIn -> getBits (1);
		ASSERT (uiMark == 1);

		// START: Complexity Estimation syntax support - Update version 2 - Massimo Ravasi (EPFL) - 5 Nov 1999
		if (m_volmd.iEstimationMethod == 1) {
			m_volmd.bVersion2ComplexityEstimationDisable = m_pbitstrmIn -> getBits (1);
			if (! m_volmd.bVersion2ComplexityEstimationDisable) {
				m_volmd.bSadct = m_pbitstrmIn -> getBits (1);
				m_volmd.bQuarterpel = m_pbitstrmIn -> getBits (1);
				if (!(m_volmd.bSadct ||
					  m_volmd.bQuarterpel)) {
					fatal_error("Version 2 complexity estimation is enabled,\nbut no corresponding flag is enabled.");
				}
			}
			else
				m_volmd.bSadct =
				m_volmd.bQuarterpel = false;
		
		} else {
			m_volmd.bVersion2ComplexityEstimationDisable = true;
			m_volmd.bSadct =
			m_volmd.bQuarterpel = false;
		}
		// END: Complexity Estimation syntax support - Update version 2
		
		// START: Complexity Estimation syntax support - Update version 2 - Massimo Ravasi (EPFL) - 11 Nov 1999
		// Main complexity estimation flag test
		if (m_volmd.bShapeComplexityEstimationDisable &&
		    m_volmd.bTextureComplexityEstimationSet1Disable &&
		    m_volmd.bTextureComplexityEstimationSet2Disable &&
		    m_volmd.bMotionCompensationComplexityDisable &&
			m_volmd.bVersion2ComplexityEstimationDisable) {
			fatal_error("Complexity estimation is enabled,\nbut no correponding flag is enabled.");
		}
		// END: Complexity Estimation syntax support - Update version 2
	}
	// END: Complexity Estimation syntax support

	/*UInt uiResyncMarkerDisable = */m_pbitstrmIn -> getBits (1);

	//	Modified by Toshiba(1998-4-7)
	m_volmd.bDataPartitioning = m_pbitstrmIn -> getBits (1);
	if( m_volmd.bDataPartitioning )
		m_volmd.bReversibleVlc = m_pbitstrmIn -> getBits (1);
	else
		m_volmd.bReversibleVlc = FALSE;
	//	End Toshiba

// NEWPRED
    if(m_volmd.uiVerID == 1)
		m_volmd.bNewpredEnable = FALSE;
    else if(m_volmd.uiVerID != 1)
		m_volmd.bNewpredEnable = m_pbitstrmIn -> getBits (1);

	if (m_volmd.bNewpredEnable) {
		ASSERT (m_volmd.fAUsage == RECTANGLE);
		ASSERT (m_vopmd.bInterlace == 0);
		ASSERT (m_uiSprite == 0);
		m_volmd.iRequestedBackwardMessegeType = m_pbitstrmIn -> getBits (2);
		m_volmd.bNewpredSegmentType = m_pbitstrmIn -> getBits (1);

		// generate NEWPRED object
		g_pNewPredDec = new CNewPredDecoder();
	}
// ~NEWPRED
// RRV insertion
	if(m_volmd.uiVerID != 1)
	{
		m_volmd.breduced_resolution_vop_enable = m_pbitstrmIn -> getBits (1);
	}
	else
	{
		m_volmd.breduced_resolution_vop_enable = 0;
	}
	m_vopmd.RRVmode.iOnOff	= m_volmd.breduced_resolution_vop_enable;
// ~RRV

	//wchen: wd changes
	m_volmd.volType = (m_pbitstrmIn -> getBits (1) == 0) ? BASE_LAYER : ENHN_LAYER;

// NEWPRED
	if (m_volmd.bNewpredEnable)
		ASSERT (m_volmd.volType == BASE_LAYER);
// ~NEWPRED

// GMC
	if(m_uiSprite == 2) ASSERT(m_volmd.volType == BASE_LAYER);
// ~GMC
	m_volmd.ihor_sampling_factor_n = 1;
	m_volmd.ihor_sampling_factor_m = 1;
	m_volmd.iver_sampling_factor_n = 1;
	m_volmd.iver_sampling_factor_m = 1;	
//OBSS_SAIT_991015
	m_volmd.ihor_sampling_factor_n_shape = 1;
	m_volmd.ihor_sampling_factor_m_shape = 1;
	m_volmd.iver_sampling_factor_n_shape = 1;
	m_volmd.iver_sampling_factor_m_shape = 1;	
//~OBSS_SAIT_991015
	if (m_volmd.volType == ENHN_LAYER)	{
//#ifdef _Scalable_SONY_
		m_volmd.iHierarchyType = m_pbitstrmIn->getBits (1); // 
		if(m_volmd.iHierarchyType == 0)
			fprintf(stdout,"Hierarchy_Type == 0 (Spatial scalability)\n");
		else if(m_volmd.iHierarchyType == 1)
			fprintf(stdout,"Hierarchy_type == 1 (Temporal scalability)\n");
//#endif _Scalable_SONY_
		m_pbitstrmIn->getBits (4);						// ref_layer_id
		m_pbitstrmIn->getBits (1);						// ref_layer_samping_director
		m_volmd.ihor_sampling_factor_n = m_pbitstrmIn->getBits (5);
		m_volmd.ihor_sampling_factor_m = m_pbitstrmIn->getBits (5);
		m_volmd.iver_sampling_factor_n = m_pbitstrmIn->getBits (5);
		m_volmd.iver_sampling_factor_m = m_pbitstrmIn->getBits (5);
		m_volmd.iEnhnType     = m_pbitstrmIn->getBits (1);			//enhancement_type
//OBSS_SAIT_991015
		if (m_volmd.fAUsage == ONE_BIT && m_volmd.iHierarchyType == 0) {			
			m_volmd.iuseRefShape = m_pbitstrmIn->getBits (1);						// use_ref_shape
			m_volmd.iuseRefTexture = m_pbitstrmIn->getBits (1);						// use_ref_texture
			m_volmd.ihor_sampling_factor_n_shape = m_pbitstrmIn->getBits (5);
			m_volmd.ihor_sampling_factor_m_shape = m_pbitstrmIn->getBits (5);
			m_volmd.iver_sampling_factor_n_shape = m_pbitstrmIn->getBits (5);
			m_volmd.iver_sampling_factor_m_shape = m_pbitstrmIn->getBits (5);
		}
//~OBSS_SAIT_991015		
	}

	m_volmd.bDeblockFilterDisable = TRUE;							//no deblocking filter
}
Void CVideoObjectDecoder::FakeOutVOVOLHead (int h, int w, int fr, Bool *pbSpatialScalability)
{
  m_volmd.iClockRate = fr;
  Int iClockRate = m_volmd.iClockRate;
  assert (iClockRate >= 1 && iClockRate < 65536);
  for (m_iNumBitsTimeIncr = 1; m_iNumBitsTimeIncr < NUMBITS_TIME_RESOLUTION; m_iNumBitsTimeIncr++)	{	
    if (iClockRate == 1)			
      break;
    iClockRate = (iClockRate >> 1);
  }
  m_volmd.bShapeOnly = FALSE;
  m_volmd.fAUsage = RECTANGLE;
  m_ivolWidth = w;
  m_ivolHeight = h;
  m_vopmd.bInterlace = 0;
  m_volmd.bAdvPredDisable = 1;
  m_uiSprite = 0;
  m_volmd.bNot8Bit = 0;
  m_volmd.uiQuantPrecision = 5;
  m_volmd.nBits = 8;
  m_volmd.fQuantizer = Q_H263;
#if 0
  m_volmd.bLoadIntraMatrix = 0;
  memcpy (m_volmd.rgiIntraQuantizerMatrix, rgiDefaultIntraQMatrix, BLOCK_SQUARE_SIZE * sizeof (Int));
  m_volmd.bLoadInterMatrix = 0;
  memcpy (m_volmd.rgiInterQuantizerMatrix, rgiDefaultInterQMatrix, BLOCK_SQUARE_SIZE * sizeof (Int));
#endif
  m_volmd.bComplexityEstimationDisable = 1; // huh ?
  m_volmd.bDataPartitioning = 0;
  m_volmd.bReversibleVlc = FALSE;
  m_volmd.volType = BASE_LAYER;
  m_volmd.ihor_sampling_factor_n = 1;
  m_volmd.ihor_sampling_factor_m = 1;
  m_volmd.iver_sampling_factor_n = 1;
  m_volmd.iver_sampling_factor_m = 1;	
  m_volmd.bDeblockFilterDisable = TRUE;
  m_volmd.bNewpredEnable = FALSE;
  m_vopmd.RRVmode.iOnOff = m_volmd.breduced_resolution_vop_enable = 0;
  m_volmd.bQuarterSample = 0;
  m_volmd.bSadct = m_volmd.bQuarterpel = 0;
			m_volmd.bAPM =
			m_volmd.bNPM =
			m_volmd.bInterpolateMCQ =
			m_volmd.bForwBackMCQ =
			m_volmd.bHalfpel2 =
			m_volmd.bHalfpel4 = false;
			m_volmd.bDCTCoefs =
			m_volmd.bDCTLines =
			m_volmd.bVLCSymbols =
			m_volmd.bVLCBits = false;
  postVO_VOLHeadInit(w, h, pbSpatialScalability);
}
//OBSSFIX_MODE3 //TPS_FIX_BGC
//Int BGComposition; // added by Sharp (98/3/24)
//~OBSSFIX_MODE3 //~TPS_FIX_BGC
Bool CVideoObjectDecoder::decodeVOPHead ()
{
	// Start code
	UInt uiVopStartCode = m_pbitstrmIn -> getBits (NUMBITS_VOP_START_CODE);
	if (uiVopStartCode == GOV_START_CODE) {
/*Added by SONY (98/03/30)*/
		m_bUseGOV = TRUE;
		m_bLinkisBroken = FALSE;
/*Added by SONY (98/03/30) End */
		Int timecode;
		timecode = m_pbitstrmIn -> getBits (5) * 3600;
		timecode += m_pbitstrmIn -> getBits (6) * 60;
		m_pbitstrmIn -> getBits (1);
		timecode += m_pbitstrmIn -> getBits (6);

		m_tModuloBaseDecd = timecode;
		m_tModuloBaseDisp = timecode;

#ifdef DEBUG_OUTPUT
		cout << "GOV Header (t=" << timecode << ")\n\n";
#endif
		Int closed_gov = m_pbitstrmIn -> getBits (1);
		Int broken_link = m_pbitstrmIn -> getBits (1);
/*modified by SONY (98/03/30)*/
		if ((closed_gov == 0)&&(broken_link == 1))
			m_bLinkisBroken = TRUE;
/*modified by SONY (98/03/30) End*/
		
		findStartCode();
/*
		m_pbitstrmIn -> getBits (4);
		Int uiPrefix = m_pbitstrmIn -> getBits (NUMBITS_START_CODE_PREFIX);
		ASSERT(uiPrefix == START_CODE_PREFIX);
*/
		uiVopStartCode = m_pbitstrmIn -> getBits (NUMBITS_VOP_START_CODE);
	}
//980212
	ASSERT(uiVopStartCode == VOP_START_CODE);

	m_vopmd.vopPredType = (VOPpredType) m_pbitstrmIn -> getBits (NUMBITS_VOP_PRED_TYPE);

// NEWPRED
	if (m_volmd.bNewpredEnable)
		ASSERT (m_vopmd.vopPredType != BVOP);
// ~NEWPRED

	// Time reference and VOP_pred_type
	Int iModuloInc = 0;
	while (m_pbitstrmIn -> getBits (1) != 0)
		iModuloInc++;
	Time tCurrSec = iModuloInc + ((m_vopmd.vopPredType != BVOP ||
				  (m_vopmd.vopPredType == BVOP && m_volmd.volType == ENHN_LAYER ))?
				m_tModuloBaseDecd : m_tModuloBaseDisp);
	//	Added for error resilient mode by Toshiba(1997-11-14)
	UInt uiMarker = m_pbitstrmIn -> getBits (1);
	ASSERT(uiMarker == 1);
	//	End Toshiba(1997-11-14)
	Time tVopIncr = 0;
	if(m_iNumBitsTimeIncr!=0)
		tVopIncr = m_pbitstrmIn -> getBits (m_iNumBitsTimeIncr);
	uiMarker = m_pbitstrmIn->getBits (1); // marker bit
	ASSERT(uiMarker == 1);
	m_tOldModuloBaseDecd = m_tModuloBaseDecd;
	m_tOldModuloBaseDisp = m_tModuloBaseDisp;
	if (m_vopmd.vopPredType != BVOP ||
		(m_vopmd.vopPredType == BVOP && m_volmd.volType == ENHN_LAYER ))

	{
		m_tModuloBaseDisp = m_tModuloBaseDecd;		//update most recently displayed time base
		m_tModuloBaseDecd = tCurrSec;
	}

    m_t = tCurrSec * m_volmd.iClockRate*m_iClockRateScale + tVopIncr*m_iClockRateScale;
	
	if (m_pbitstrmIn->getBits (1) == 0)		{		//vop_coded == false
		m_vopmd.bInterlace = FALSE;	//wchen: temporary solution
		return FALSE;
	}

// NEWPRED
	if (m_volmd.bNewpredEnable) {
		m_vopmd.m_iVopID = m_pbitstrmIn -> getBits(m_vopmd.m_iNumBitsVopID);
		m_vopmd.m_iVopID4Prediction_Indication = m_pbitstrmIn -> getBits(NUMBITS_VOP_ID_FOR_PREDICTION_INDICATION);
		if( m_vopmd.m_iVopID4Prediction_Indication )
			m_vopmd.m_iVopID4Prediction = m_pbitstrmIn -> getBits(m_vopmd.m_iNumBitsVopID);
		m_pbitstrmIn -> getBits(MARKER_BIT);	
		g_pNewPredDec->GetRef(
				NP_VOP_HEADER,
				m_vopmd.vopPredType,
				m_vopmd.m_iVopID,	
				m_vopmd.m_iVopID4Prediction_Indication,
				m_vopmd.m_iVopID4Prediction
		);
	}
// ~NEWPRED

	if ((m_vopmd.vopPredType == PVOP  || (m_uiSprite == 2 && m_vopmd.vopPredType == SPRITE)) && m_volmd.bShapeOnly==FALSE) // GMC
		m_vopmd.iRoundingControl = m_pbitstrmIn->getBits (1); //"VOP_Rounding_Type"
	else
		m_vopmd.iRoundingControl = 0;

// RRV insertion
	if((m_volmd.breduced_resolution_vop_enable == 1)&&(m_volmd.fAUsage == RECTANGLE)&&
	   ((m_vopmd.vopPredType == PVOP)||(m_vopmd.vopPredType == IVOP)))
	{
	  	m_vopmd.RRVmode.iRRVOnOff	= m_pbitstrmIn->getBits (1);
	}	
	else
	{
		m_vopmd.RRVmode.iRRVOnOff	= 0;
	}
// ~RRV
	if (m_volmd.fAUsage != RECTANGLE) {
// Begin: modified by Hughes	  4/9/98
	  if (!(m_uiSprite == 1 && m_vopmd.vopPredType == IVOP)) {
// End: modified by Hughes	  4/9/98

		Int width = m_pbitstrmIn -> getBits (NUMBITS_VOP_WIDTH);
// spt VOP		ASSERT (width % MB_SIZE == 0); // for sprite, may not be multiple of MB_SIZE
		Int marker;
		marker = m_pbitstrmIn -> getBits (1); // marker bit
		ASSERT(marker==1);
		Int height = m_pbitstrmIn -> getBits (NUMBITS_VOP_HEIGHT);
// spt VOP		ASSERT (height % MB_SIZE == 0); // for sprite, may not be multiple of MB_SIZE
		marker = m_pbitstrmIn -> getBits (1); // marker bit
		ASSERT(marker==1);
		//wchen: cd changed to 2s complement
		Int left = (m_pbitstrmIn -> getBits (1) == 0) ?
					m_pbitstrmIn->getBits (NUMBITS_VOP_HORIZONTAL_SPA_REF - 1) : 
					((Int)m_pbitstrmIn->getBits (NUMBITS_VOP_HORIZONTAL_SPA_REF - 1) - (1 << (NUMBITS_VOP_HORIZONTAL_SPA_REF - 1)));
		marker = m_pbitstrmIn -> getBits (1); // marker bit
		ASSERT(marker==1);
		Int top = (m_pbitstrmIn -> getBits (1) == 0) ?
				   m_pbitstrmIn->getBits (NUMBITS_VOP_VERTICAL_SPA_REF - 1) : 
				   ((Int)m_pbitstrmIn->getBits (NUMBITS_VOP_VERTICAL_SPA_REF - 1) - (1 << (NUMBITS_VOP_VERTICAL_SPA_REF - 1)));
		ASSERT(((left | top)&1)==0); // must be even pix unit
		marker = m_pbitstrmIn -> getBits (1); // marker bit added Nov 10, swinder
		ASSERT(marker==1);

		m_rctCurrVOPY = CRct (left, top, left + width, top + height);
		m_rctCurrVOPUV = m_rctCurrVOPY.downSampleBy2 ();
	  }


	  if ( m_volmd.volType == ENHN_LAYER && m_volmd.iEnhnType != 0 ){ // change added for Norio Ito
//OBSSFIX_MODE3 //TPS_FIX_BGC
			m_vopmd.bBGComposition = m_pbitstrmIn -> getBits (1); // modified by Sharp (98/3/24)
//			BGComposition = m_pbitstrmIn -> getBits (1); // modified by Sharp (98/3/24)
//~OBSSFIX_MODE3 //~TPS_FIX_BGC
//			ASSERT(BackgroundComposition==1); // modified by Sharp (98/3/24)
		}

		m_volmd.bNoCrChange = m_pbitstrmIn -> getBits (1);	//VOP_CR_Change_Disable
		Int iVopConstantAlpha = m_pbitstrmIn -> getBits (1);
		if(iVopConstantAlpha==1)
			m_vopmd.iVopConstantAlphaValue = m_pbitstrmIn -> getBits (8);
		else
			m_vopmd.iVopConstantAlphaValue = 255;
		m_vopmd.bShapeCodingType = (m_vopmd.vopPredType == IVOP) ? 0 : 1; //	Added error resilient mode by Toshiba(1998-1-16)
	}

	// START: Complexity Estimation syntax support - Marc Mongenet (EPFL) - 15 Jun 1998

	if (! m_volmd.bComplexityEstimationDisable) {

		// START: Complexity Estimation syntax support - Update version 2 - Massimo Ravasi (EPFL) - 5 Nov 1999
		if (  (m_volmd.iEstimationMethod != 0) && (m_volmd.iEstimationMethod != 1) ) {
		//// Replaced line: if (m_volmd.iEstimationMethod != 0) {
		// END: Complexity Estimation syntax support - Update version 2
			fprintf (stderr, "ERROR: Unknown estimation method number %d.\n", m_volmd.iEstimationMethod);
			exit (1);
		}
		
		if (m_vopmd.vopPredType == IVOP ||
			m_vopmd.vopPredType == PVOP ||
			m_vopmd.vopPredType == BVOP) {

			if (m_volmd.bOpaque) {
				printf ("dcecs_opaque = %d\n", m_vopmd.iOpaque = m_pbitstrmIn -> getBits (8));
				if (m_vopmd.iOpaque == 0) {
					fprintf (stderr, "ERROR: Illegal null value for 'opaque' complexity estimation.\n");
					exit (1);
				}
			}
			if (m_volmd.bTransparent) {
				printf ("dcecs_transparent = %d\n", m_vopmd.iTransparent = m_pbitstrmIn -> getBits (8));
				if (m_vopmd.iTransparent == 0) {
					fprintf (stderr, "ERROR: Illegal null value for 'transparent' complexity estimation.\n");
					exit (1);
				}
			}
			if (m_volmd.bIntraCAE) {
				printf ("dcecs_intra_cae = %d\n", m_vopmd.iIntraCAE = m_pbitstrmIn -> getBits (8));
				if (m_vopmd.iIntraCAE == 0) {
					fprintf (stderr, "ERROR: Illegal null value for 'intra_cae' complexity estimation.\n");
					exit (1);
				}
			}
			if (m_volmd.bInterCAE) {
				printf ("dcecs_inter_cae = %d\n", m_vopmd.iInterCAE = m_pbitstrmIn -> getBits (8));
				if (m_vopmd.iInterCAE == 0) {
					fprintf (stderr, "ERROR: Illegal null value for 'inter_cae' complexity estimation.\n");
					exit (1);
				}
			}
			if (m_volmd.bNoUpdate) {
				printf ("dcecs_no_update = %d\n", m_vopmd.iNoUpdate = m_pbitstrmIn -> getBits (8));
				if (m_vopmd.iNoUpdate == 0) {
					fprintf (stderr, "ERROR: Illegal null value for 'no_update' complexity estimation.\n");
					exit (1);
				}
			}
			if (m_volmd.bUpsampling) {
				printf ("dcecs_upsampling = %d\n", m_vopmd.iUpsampling = m_pbitstrmIn -> getBits (8));
				if (m_vopmd.iUpsampling == 0) {
					fprintf (stderr, "ERROR: Illegal null value for 'upsampling' complexity estimation.\n");
					exit (1);
				}
			}
		}

		if (m_volmd.bIntraBlocks) {
			printf ("dcecs_intra_blocks = %d\n", m_vopmd.iIntraBlocks = m_pbitstrmIn -> getBits (8));
			if (m_vopmd.iIntraBlocks == 0) {
					fprintf (stderr, "ERROR: Illegal null value for 'intra_blocks' complexity estimation.\n");
					exit (1);
				}
		}
		if (m_volmd.bNotCodedBlocks) {
			printf ("dcecs_not_coded_blocks = %d\n", m_vopmd.iNotCodedBlocks = m_pbitstrmIn -> getBits (8));
			if (m_vopmd.iNotCodedBlocks == 0) {
					fprintf (stderr, "ERROR: Illegal null value for 'not_coded_blocks' complexity estimation.\n");
					exit (1);
				}
		}
		if (m_volmd.bDCTCoefs) {
			printf ("dcecs_dct_coefs = %d\n", m_vopmd.iDCTCoefs = m_pbitstrmIn -> getBits (8));
			if (m_vopmd.iDCTCoefs == 0) {
					fprintf (stderr, "ERROR: Illegal null value for 'dct_coefs' complexity estimation.\n");
					exit (1);
				}
		}
		if (m_volmd.bDCTLines) {
			printf ("dcecs_dct_lines = %d\n", m_vopmd.iDCTLines = m_pbitstrmIn -> getBits (8));
			if (m_vopmd.iDCTLines == 0) {
					fprintf (stderr, "ERROR: Illegal null value for 'dct_lines' complexity estimation.\n");
					exit (1);
				}
		}
		if (m_volmd.bVLCSymbols) {
			printf ("dcecs_vlc_symbols = %d\n", m_vopmd.iVLCSymbols = m_pbitstrmIn -> getBits (8));
			if (m_vopmd.iVLCSymbols == 0) {
					fprintf (stderr, "ERROR: Illegal null value for 'vlc_symbols' complexity estimation.\n");
					exit (1);
				}
		}
		if (m_volmd.bVLCBits) {
			printf ("dcecs_vlc_bits = %d\n", m_vopmd.iVLCBits = m_pbitstrmIn -> getBits (4));
			if (m_vopmd.iVLCBits == 0) {
					fprintf (stderr, "ERROR: Illegal null value for 'vlc_bits' complexity estimation.\n");
					exit (1);
				}
		}

		if (m_vopmd.vopPredType == PVOP ||
			m_vopmd.vopPredType == BVOP ||
			// START: Complexity Estimation syntax support - Update version 2 - Massimo Ravasi (EPFL) - 11 Nov 1999
  			((m_vopmd.vopPredType) == SPRITE && (m_uiSprite == 1)) ) {
			// END: Complexity Estimation syntax support - Update version 2
			if (m_volmd.bInterBlocks) {
				printf ("dcecs_inter_blocks = %d\n", m_vopmd.iInterBlocks = m_pbitstrmIn -> getBits (8));
				if (m_vopmd.iInterBlocks == 0) {
					fprintf (stderr, "ERROR: Illegal null value for 'inter_blocks' complexity estimation.\n");
					exit (1);
				}
			}
			if (m_volmd.bInter4vBlocks) {
				printf ("dcecs_inter4v_blocks = %d\n", m_vopmd.iInter4vBlocks = m_pbitstrmIn -> getBits (8));
				if (m_vopmd.iInter4vBlocks == 0) {
					fprintf (stderr, "ERROR: Illegal null value for 'inter4v_blocks' complexity estimation.\n");
					exit (1);
				}
			}
			if (m_volmd.bAPM) {
				printf ("dcecs_apm = %d\n", m_vopmd.iAPM = m_pbitstrmIn -> getBits (8));
				if (m_vopmd.iAPM == 0) {
					fprintf (stderr, "ERROR: Illegal null value for 'apm' complexity estimation.\n");
					exit (1);
				}
			}
			if (m_volmd.bNPM) {
				printf ("dcecs_npm = %d\n", m_vopmd.iNPM = m_pbitstrmIn -> getBits (8));
				if (m_vopmd.iNPM == 0) {
					fprintf (stderr, "ERROR: Illegal null value for 'npm' complexity estimation.\n");
					exit (1);
				}
			}
			if (m_volmd.bForwBackMCQ) {
				printf ("dcecs_forw_back_mc_q = %d\n", m_vopmd.iForwBackMCQ = m_pbitstrmIn -> getBits (8));
				if (m_vopmd.iForwBackMCQ == 0) {
					fprintf (stderr, "ERROR: Illegal null value for 'forw_back_mc_q' complexity estimation.\n");
					exit (1);
				}
			}
			if (m_volmd.bHalfpel2) {
				printf ("dcecs_halfpel2 = %d\n", m_vopmd.iHalfpel2 = m_pbitstrmIn -> getBits (8));
				if (m_vopmd.iHalfpel2 == 0) {
					fprintf (stderr, "ERROR: Illegal null value for 'halfpel2' complexity estimation.\n");
					exit (1);
				}
			}
			if (m_volmd.bHalfpel4) {
				printf ("dcecs_halfpel4 = %d\n", m_vopmd.iHalfpel4 = m_pbitstrmIn -> getBits (8));
				if (m_vopmd.iHalfpel4 == 0) {
					fprintf (stderr, "ERROR: Illegal null value for 'halfpel4' complexity estimation.\n");
					exit (1);
				}
			}
		}
		if (m_vopmd.vopPredType == BVOP ||
			// START: Complexity Estimation syntax support - Update version 2 - Massimo Ravasi (EPFL) - 11 Nov 1999
  			((m_vopmd.vopPredType) == SPRITE && (m_uiSprite == 1)) ) {
			// END: Complexity Estimation syntax support - Update version 2
			if (m_volmd.bInterpolateMCQ) {
				printf ("dcecs_interpolate_mc_q = %d\n", m_vopmd.iInterpolateMCQ = m_pbitstrmIn -> getBits (8));
				if (m_vopmd.iInterpolateMCQ == 0) {
					fprintf (stderr, "ERROR: Illegal null value for 'interpolate_mc_q' complexity estimation.\n");
					exit (1);
				}
			}
		}
		// START: Complexity Estimation syntax support - Update version 2 - Massimo Ravasi (EPFL) - 5 Nov 1999
		if (m_vopmd.vopPredType == IVOP ||
			m_vopmd.vopPredType == PVOP ||
			m_vopmd.vopPredType == BVOP) {
			if(m_volmd.bSadct) {
				printf ("dcecs_bSadct = %d\n", m_vopmd.iSadct = m_pbitstrmIn -> getBits (8));
				if (m_vopmd.iSadct == 0) {
					fprintf (stderr, "ERROR: Illegal null value for 'sadct' complexity estimation.\n");
					exit (1);
				}
			}
		}

		if (m_vopmd.vopPredType == PVOP ||
			m_vopmd.vopPredType == BVOP ||
			// START: Complexity Estimation syntax support - Update version 2 - Massimo Ravasi (EPFL) - 11 Nov 1999
  			((m_vopmd.vopPredType) == SPRITE && (m_uiSprite == 1)) ) {
			// END: Complexity Estimation syntax support - Update version 2
			if(m_volmd.bQuarterpel) {
				printf ("dcecs_bQuarterpel = %d\n", m_vopmd.iQuarterpel = m_pbitstrmIn -> getBits (8));
				if (m_vopmd.iQuarterpel == 0) {
					fprintf (stderr, "ERROR: Illegal null value for 'quarterpel' complexity estimation.\n");
					exit (1);
				}
			}
		}
		// END: Complexity Estimation syntax support - Update version 2
	}
	// END: Complexity Estimation syntax support

	if(m_volmd.bShapeOnly==TRUE)
	{
		m_vopmd.intStep = 10; // probably not needed 
		m_vopmd.intStepI = 10;
		m_vopmd.intStepB = 10;
		m_vopmd.mvInfoForward.uiFCode = m_vopmd.mvInfoBackward.uiFCode = 1;	//	Modified error resilient mode by Toshiba (1998-1-16)
		m_vopmd.bInterlace = FALSE;	//wchen: temporary solution
//OBSS_SAIT_991015
		if(m_volmd.volType == ENHN_LAYER && m_vopmd.vopPredType == PVOP)
			m_vopmd.iRefSelectCode = 3;
		else if(m_volmd.volType == ENHN_LAYER && m_vopmd.vopPredType == BVOP)	
			m_vopmd.iRefSelectCode = 0;
//~OBSS_SAIT_991015
		return TRUE;
	}
	m_vopmd.iIntraDcSwitchThr = m_pbitstrmIn->getBits (3);

// INTERLACE
	if (m_vopmd.bInterlace) {
		m_vopmd.bTopFieldFirst = m_pbitstrmIn -> getBits (1);
        m_vopmd.bAlternateScan = m_pbitstrmIn -> getBits (1);
		ASSERT(m_volmd.volType == BASE_LAYER);
	} else
// ~INTERLACE
    {
        m_vopmd.bAlternateScan = FALSE;
    }

// GMC
	if (m_uiSprite == 2 && m_vopmd.vopPredType == SPRITE) {
		if(m_iNumOfPnts > 0)
			decodeWarpPoints ();
	}
// ~GMC

	if (m_vopmd.vopPredType == IVOP)	{ 
		m_vopmd.intStepI = m_vopmd.intStep = m_pbitstrmIn -> getBits (m_volmd.uiQuantPrecision);	//also assign intStep to be safe
		m_vopmd.mvInfoForward.uiFCode = m_vopmd.mvInfoBackward.uiFCode = 1;		//	Modified error resilient mode by Toshiba(1998-1-16)
		if(m_volmd.fAUsage == EIGHT_BIT)
      for(Int iAuxComp=0; iAuxComp<m_volmd.iAuxCompCount; iAuxComp++ ) // MAC (SB) 2-Dec-99
			  m_vopmd.intStepIAlpha[iAuxComp] = m_pbitstrmIn -> getBits (NUMBITS_VOP_ALPHA_QUANTIZER);
	}
	else if (m_vopmd.vopPredType == PVOP || (m_uiSprite == 2 && m_vopmd.vopPredType == SPRITE)) { // GMC
		m_vopmd.intStep = m_pbitstrmIn -> getBits (m_volmd.uiQuantPrecision);
		if(m_volmd.fAUsage == EIGHT_BIT)
      for(Int iAuxComp=0; iAuxComp<m_volmd.iAuxCompCount; iAuxComp++ ) // MAC (SB) 2-Dec-99
			  m_vopmd.intStepPAlpha[iAuxComp] = m_pbitstrmIn -> getBits (NUMBITS_VOP_ALPHA_QUANTIZER);

		m_vopmd.mvInfoForward.uiFCode = m_pbitstrmIn -> getBits (NUMBITS_VOP_FCODE);
        m_vopmd.mvInfoForward.uiScaleFactor = 1 << (m_vopmd.mvInfoForward.uiFCode - 1);
		m_vopmd.mvInfoForward.uiRange = 16 << m_vopmd.mvInfoForward.uiFCode;
		//	Added for error resilient mode by Toshiba(1998-1-16)
		m_vopmd.mvInfoBackward.uiFCode = 1;
		//	End Toshiba(1998-1-16)
	}
	else if (m_vopmd.vopPredType == BVOP) {
		m_vopmd.intStepB = m_vopmd.intStep = m_pbitstrmIn -> getBits (m_volmd.uiQuantPrecision); //also assign intStep to be safe

		if(m_volmd.fAUsage == EIGHT_BIT)
      for(Int iAuxComp=0; iAuxComp<m_volmd.iAuxCompCount; iAuxComp++ )  // MAC (SB) 2-Dec-99
			  m_vopmd.intStepBAlpha[iAuxComp] = m_pbitstrmIn -> getBits (NUMBITS_VOP_ALPHA_QUANTIZER);
		
		m_vopmd.mvInfoForward.uiFCode = m_pbitstrmIn -> getBits (NUMBITS_VOP_FCODE);
        m_vopmd.mvInfoForward.uiScaleFactor = 1 << (m_vopmd.mvInfoForward.uiFCode - 1);
		m_vopmd.mvInfoForward.uiRange = 16 << m_vopmd.mvInfoForward.uiFCode;
		m_vopmd.mvInfoBackward.uiFCode = m_pbitstrmIn -> getBits (NUMBITS_VOP_FCODE);
        m_vopmd.mvInfoBackward.uiScaleFactor = 1 << (m_vopmd.mvInfoBackward.uiFCode - 1);
		m_vopmd.mvInfoBackward.uiRange = 16 << m_vopmd.mvInfoBackward.uiFCode;
	}
	//	Added for error resilient mode by Toshiba(1997-11-14)
//	m_vopmd.bShapeCodingType = 1; // FIX for C2-1
	if ( m_volmd.volType == BASE_LAYER ) // added by Sharp (98/4/13)
        if (m_volmd.fAUsage != RECTANGLE && m_vopmd.vopPredType != IVOP
				&& m_uiSprite != 1)
		{	
			m_vopmd.bShapeCodingType = m_pbitstrmIn -> getBits (1);
        }
	// End Toshiba(1997-11-14)
	if(m_volmd.volType == ENHN_LAYER) {					//sony
		m_vopmd.bShapeCodingType = (m_vopmd.vopPredType == IVOP?0:1); // FIX for C2-1, added 
		if( m_volmd.iEnhnType != 0 ){  // change for Norio Ito
			m_vopmd.iLoadBakShape = m_pbitstrmIn -> getBits (1); // load_backward_shape
			if(m_vopmd.iLoadBakShape){
				CVOPU8YUVBA* pvopcCurr = new CVOPU8YUVBA (*(rgpbfShape [0]->pvopcReconCurr()));
				copyVOPU8YUVBA(rgpbfShape [1]->m_pvopcRefQ1, pvopcCurr);
					// previous backward shape is saved to current forward shape
				rgpbfShape [1]->m_rctCurrVOPY.left   = rgpbfShape [0]->m_rctCurrVOPY.left;
				rgpbfShape [1]->m_rctCurrVOPY.right  = rgpbfShape [0]->m_rctCurrVOPY.right;
				rgpbfShape [1]->m_rctCurrVOPY.top    = rgpbfShape [0]->m_rctCurrVOPY.top;
				rgpbfShape [1]->m_rctCurrVOPY.bottom = rgpbfShape [0]->m_rctCurrVOPY.bottom;

				Int width = m_pbitstrmIn -> getBits (NUMBITS_VOP_WIDTH); ASSERT (width % MB_SIZE == 0); // has to be multiples of MB_SIZE
				UInt uiMark = m_pbitstrmIn -> getBits (1);
				ASSERT (uiMark == 1);
				Int height = m_pbitstrmIn -> getBits (NUMBITS_VOP_HEIGHT); ASSERT (height % MB_SIZE == 0); // has to be multiples of MB_SIZE
				uiMark = m_pbitstrmIn -> getBits (1);
				ASSERT (uiMark == 1);
				width = ((width+15)>>4)<<4; // not needed if the ASSERTs are present
				height = ((height+15)>>4)<<4;
				Int iSign = (m_pbitstrmIn -> getBits (1) == 1)? -1 : 1;
				Int left = iSign * m_pbitstrmIn -> getBits (NUMBITS_VOP_HORIZONTAL_SPA_REF - 1);
				Int marker = m_pbitstrmIn -> getBits (1); // marker bit
				iSign = (m_pbitstrmIn -> getBits (1) == 1)? -1 : 1;
				Int top = iSign * m_pbitstrmIn -> getBits (NUMBITS_VOP_VERTICAL_SPA_REF - 1);
			
				rgpbfShape[0]->m_rctCurrVOPY = CRct (left, top, left + width, top + height);
				rgpbfShape[0]->m_rctCurrVOPUV = rgpbfShape[0]->m_rctCurrVOPY.downSampleBy2 ();

				// decode backward shape
				rgpbfShape[0]->setRefStartingPointers ();
				rgpbfShape[0]->compute_bfShapeMembers (); // clear m_pvopcRefQ1
				rgpbfShape[0]->resetBYPlane ();// clear BY of RefQ1 (katata)
				rgpbfShape[0]->m_volmd.bShapeOnly = TRUE;
				rgpbfShape[0]->m_volmd.bNoCrChange = m_volmd.bNoCrChange; // set CR change disable(Oct. 9 1997)
				rgpbfShape[0]->m_vopmd.bInterlace = FALSE;
				rgpbfShape[0]->decodeIVOP_WithShape ();

				m_vopmd.iLoadForShape = m_pbitstrmIn -> getBits (1); // load_forward_shape
				if(m_vopmd.iLoadForShape){
					width = m_pbitstrmIn -> getBits (NUMBITS_VOP_WIDTH); ASSERT (width % MB_SIZE == 0); // has to be multiples of MB_SIZE
					uiMark = m_pbitstrmIn -> getBits (1);
					ASSERT (uiMark == 1);
					height = m_pbitstrmIn -> getBits (NUMBITS_VOP_HEIGHT); ASSERT (height % MB_SIZE == 0); // has to be multiples of MB_SIZE
					uiMark = m_pbitstrmIn -> getBits (1);
					ASSERT (uiMark == 1);
					width = ((width+15)>>4)<<4; // not needed if the ASSERTs are present
					height = ((height+15)>>4)<<4;
					iSign = (m_pbitstrmIn -> getBits (1) == 1)? -1 : 1;
					left = iSign * m_pbitstrmIn -> getBits (NUMBITS_VOP_HORIZONTAL_SPA_REF - 1);
					marker = m_pbitstrmIn -> getBits (1); // marker bit
					iSign = (m_pbitstrmIn -> getBits (1) == 1)? -1 : 1;
					top = iSign * m_pbitstrmIn -> getBits (NUMBITS_VOP_VERTICAL_SPA_REF - 1);
		
					rgpbfShape[1]->m_rctCurrVOPY = CRct (left, top, left + width, top + height);
					rgpbfShape[1]->m_rctCurrVOPUV = rgpbfShape[1]->m_rctCurrVOPY.downSampleBy2 ();

					// decode forward shape
					rgpbfShape[1]->setRefStartingPointers ();
					rgpbfShape[1]->compute_bfShapeMembers (); // clear m_pvopcRefQ1
					rgpbfShape[1]->resetBYPlane ();// clear BY of RefQ1 (katata)
					rgpbfShape[1]->m_volmd.bShapeOnly = TRUE;
					rgpbfShape[1]->m_volmd.bNoCrChange = m_volmd.bNoCrChange; // set CR change disable(Oct. 9 1997)
					rgpbfShape[1]->m_vopmd.bInterlace = FALSE;
					rgpbfShape[1]->decodeIVOP_WithShape ();
				}
			} // end of "if(m_vopmd.iLoadBakShape)"
			else
				m_vopmd.iLoadForShape = 0; // no forward shape when backward shape is not decoded
		}
		else {
			m_vopmd.iLoadForShape = 0;
			m_vopmd.iLoadBakShape = 0;
		}
        m_vopmd.iRefSelectCode = m_pbitstrmIn ->getBits (2) ;
	}
	return TRUE;
}

//added by sony for spatial scalability decoding loop
Int CVideoObjectDecoder::ReadNextVopPredType ()
{
	m_pbitstrmIn -> setBookmark ();

	if (findStartCode () == EOF) {
		m_pbitstrmIn -> gotoBookmark ();
		return EOF;
	}

	// Start code
	UInt uiVopStartCode = m_pbitstrmIn -> getBits (NUMBITS_VOP_START_CODE);
	ASSERT(uiVopStartCode == VOP_START_CODE);

	Int	vopPredType = (VOPpredType) m_pbitstrmIn -> getBits (NUMBITS_VOP_PRED_TYPE);
	// Time reference and VOP_pred_type
	
	m_pbitstrmIn -> gotoBookmark ();

	return(vopPredType);
}

Void CVideoObjectDecoder::errorInBitstream (Char* rgchErorrMsg)
{
  //	fprintf (stderr, "%s at %d\n", rgchErorrMsg, m_pbitstrmIn->getCounter ());
	ASSERT (FALSE);
	exit (1);
}


// Added by Sharp(1998-02-10)
void CVideoObjectDecoder::updateBuffVOPsBase (
	CVideoObjectDecoder* pvodecEnhc
)
{
	switch(m_vopmd.vopPredType) {
		case IVOP:	
			if (!pvodecEnhc -> m_pBuffP2 -> empty ())	// added by Sharp (98/3/11)
				if ( pvodecEnhc -> m_pBuffP2 -> m_bCodedFutureRef == 1 ) // added by Sharp (99/1/28)
				pvodecEnhc -> m_pBuffP1 -> copyBuf (*(pvodecEnhc -> m_pBuffP2)); // added by Sharp (98/3/11)
			pvodecEnhc -> m_pBuffP2 -> getBuf (this);
			break;
		case PVOP:
				if ( pvodecEnhc -> m_pBuffP2 -> m_bCodedFutureRef == 1 ) // added by Sharp (99/1/28)
			pvodecEnhc -> m_pBuffP1 -> copyBuf (*(pvodecEnhc -> m_pBuffP2));
			pvodecEnhc -> m_pBuffP2 -> getBuf (this);
			break;
		case BVOP:
			if(!(pvodecEnhc -> m_pBuffB2 -> empty ())) {
				if ( pvodecEnhc -> m_pBuffB2 -> m_bCodedFutureRef == 1 ) // added by Sharp (99/1/28)
				pvodecEnhc -> m_pBuffB1 -> copyBuf (*(pvodecEnhc -> m_pBuffB2));
			}
			pvodecEnhc -> m_pBuffB2 -> getBuf (this);
			break;
		default:	exit(1);
	}
}

void CVideoObjectDecoder::updateRefVOPsEnhc ()
{
	Int tQ0 = 0, tQ1 = 0;
	switch(m_vopmd.vopPredType) {
		case IVOP:	// printf(" Not defined in updateRefVOPsEnhc. \n");  exit(1); // deleted by Sharp (99/1/25)
			break; // added by Sharp (99/1/25)
		case PVOP:
			switch(m_vopmd.iRefSelectCode) {
				case 0:	m_pBuffE -> putBufToQ0 (this);  tQ0 = m_pBuffE -> m_t;
					break;
				case 1:	if(!(m_pBuffB1 -> empty ()))	{ m_pBuffB1 -> putBufToQ0 (this); tQ0 = m_pBuffB1 -> m_t; }
					else				{ m_pBuffP1 -> putBufToQ0 (this); tQ0 = m_pBuffP1 -> m_t; }
					break;
				case 2:	if(!(m_pBuffB2 -> empty ()))	{ m_pBuffB2 -> putBufToQ0 (this); tQ0 = m_pBuffB2 -> m_t; }
					else				{ m_pBuffP2 -> putBufToQ0 (this); tQ0 = m_pBuffP2 -> m_t; }
					break;
				case 3:	// printf(" For Spatial Scalability -- Not defined\n"); exit(1);
					if(!(m_pBuffB1 -> empty ()))	{ m_pBuffB1 -> putBufToQ0 (this); tQ0 = m_pBuffB1 -> m_t; }
					else				{ m_pBuffP1 -> putBufToQ0 (this); tQ0 = m_pBuffP1 -> m_t; }
					break;
			}
			m_tPastRef   = tQ0;
			break;
		case BVOP:
			switch(m_vopmd.iRefSelectCode) {
				case 0:	// printf(" For Spatial Scalability -- Not defined\n"); exit(1);
					m_pBuffE -> putBufToQ0 (this);	tQ0 = m_pBuffE -> m_t;
					if(!(m_pBuffB1 -> empty ()))	{ m_pBuffB1 -> putBufToQ1 (this); tQ1 = m_pBuffB1 -> m_t; }
					else				{ m_pBuffP1 -> putBufToQ1 (this); tQ1 = m_pBuffP1 -> m_t; }
					break;
				case 1:	m_pBuffE -> putBufToQ0 (this);	tQ0 = m_pBuffE -> m_t;
					if(!(m_pBuffB1 -> empty ()))	{ m_pBuffB1 -> putBufToQ1 (this); tQ1 = m_pBuffB1 -> m_t; }
					else				{ m_pBuffP1 -> putBufToQ1 (this); tQ1 = m_pBuffP1 -> m_t; }
					break;
				case 2:	m_pBuffE -> putBufToQ0 (this);	tQ0 = m_pBuffE -> m_t;
					if(!(m_pBuffB2 -> empty ()))	{ m_pBuffB2 -> putBufToQ1 (this); tQ1 = m_pBuffB2 -> m_t; }
					else				{ m_pBuffP2 -> putBufToQ1 (this); tQ1 = m_pBuffP2 -> m_t; }
					break;
				case 3:	if(!(m_pBuffB1 -> empty ()))	{ m_pBuffB1 -> putBufToQ0 (this); tQ0 = m_pBuffB1 -> m_t; }
					else				{ m_pBuffP1 -> putBufToQ0 (this); tQ0 = m_pBuffP1 -> m_t; }
					if(!(m_pBuffB2 -> empty ()))	{ m_pBuffB2 -> putBufToQ1 (this); tQ1 = m_pBuffB2 -> m_t; }
					else				{ m_pBuffP2 -> putBufToQ1 (this); tQ1 = m_pBuffP2 -> m_t; }
					m_iBCount = (getTime () - tQ0);
					break;
			}
			m_tPastRef   = tQ0;
			m_tFutureRef = tQ1;
			break;
		default:	exit(1);
	}
}

void CVideoObjectDecoder::updateBuffVOPsEnhc ()
{
	if ( this -> m_bCodedFutureRef == 1 ){ // added by Sharp (99/1/28)
	switch(m_vopmd.vopPredType) {
		case IVOP:	// printf(" Not defined in updateBuffVOPsEnhc. \n");  exit(1); // deleted by Sharp (99/1/25)
			        m_pBuffE -> getBuf (this); // added by Sharp (99/1/22)
							break; //added by Sharp (99/1/25)
		case PVOP:	m_pBuffE -> getBuf (this);
				break;
		case BVOP:	m_pBuffE -> getBuf (this);
				break;
		default:	exit(1);
	}
	} // added by Sharp (99/1/28)
}

void CVideoObjectDecoder::bufferB2flush ()
{
	if (!(m_pBuffB2 -> empty ())) {
	if ( m_pBuffB2 -> m_bCodedFutureRef == 1 ) // added by Sharp (99/1/28)
		m_pBuffB1 -> copyBuf (*m_pBuffB2);
		m_pBuffB2 -> dispose ();
	}
}
/* Added */
void CVideoObjectDecoder::set_enh_display_size(CRct rctDisplay, CRct *rctDisplay_SSenh)
{
//OBSS_SAIT_991015
	Int tmp_right,tmp_left,tmp_top,tmp_bottom,tmp_width;

	tmp_right = m_volmd.iFrmWidth_SS;	
	tmp_left  = 0;		
	tmp_width = m_volmd.iFrmWidth_SS;	

	tmp_top     = 0;	
	tmp_bottom  = m_volmd.iFrmHeight_SS;	
/*
	float tmp_right,tmp_left,tmp_top,tmp_bottom,tmp_width;

	tmp_right = (float) rctDisplay.right * ((float) m_volmd.ihor_sampling_factor_n / m_volmd.ihor_sampling_factor_m);
	tmp_left  = (float) rctDisplay.left  * ((float) m_volmd.ihor_sampling_factor_n / m_volmd.ihor_sampling_factor_m);
	tmp_width = (float) rctDisplay.width * ((float) m_volmd.ihor_sampling_factor_n / m_volmd.ihor_sampling_factor_m);

	tmp_top     = (float) rctDisplay.top * ((float) m_volmd.iver_sampling_factor_n / m_volmd.iver_sampling_factor_m);
	tmp_bottom  = (float) rctDisplay.bottom  * ((float) m_volmd.iver_sampling_factor_n / m_volmd.iver_sampling_factor_m);
*/
//~OBSS_SAIT_991015

	rctDisplay_SSenh->right  = (Int) tmp_right;
	rctDisplay_SSenh->left   = (Int) tmp_left;

	rctDisplay_SSenh->width  = (Int) tmp_width;

	rctDisplay_SSenh->top    = (Int) tmp_top;
	rctDisplay_SSenh->bottom = (Int) tmp_bottom;

}


void CVideoObjectDecoder::bufferB1flush ()
{
	m_pBuffB1 -> dispose ();
}

void CVideoObjectDecoder::bufferP1flush ()
{
	m_pBuffP1 -> dispose ();
}

void CVideoObjectDecoder::copyBufP2ToB1 ()
{
	if ( m_pBuffP2 -> m_bCodedFutureRef == 1 ) // added by Sharp (99/1/28)
	m_pBuffB1 -> copyBuf (*m_pBuffP2);
}

void CVideoObjectDecoder::copyRefQ1ToQ0 ()
{
	copyVOPU8YUVBA(m_pvopcRefQ0, m_pvopcRefQ1);
}

Time CVideoObjectDecoder::senseTime ()
{
	m_pbitstrmIn -> setBookmark ();

	if (findStartCode () == EOF) {
		m_pbitstrmIn -> gotoBookmark ();
		return EOF;
	}
	// Start code
	UInt uiVopStartCode = m_pbitstrmIn -> getBits (NUMBITS_VOP_START_CODE);
	if (uiVopStartCode == GOV_START_CODE) {
		m_bUseGOV = TRUE;
		m_bLinkisBroken = FALSE;
		Int timecode;
		timecode = m_pbitstrmIn -> getBits (5) * 3600;
		timecode += m_pbitstrmIn -> getBits (6) * 60;
		m_pbitstrmIn -> getBits (1);
		timecode += m_pbitstrmIn -> getBits (6);
		m_tModuloBaseDecd = timecode;
		m_tModuloBaseDisp = timecode;

		Int closed_gov = m_pbitstrmIn -> getBits (1);
		Int broken_link = m_pbitstrmIn -> getBits (1);
		if ((closed_gov == 0)&&(broken_link == 1))
			m_bLinkisBroken = TRUE;

		findStartCode();
/*
		m_pbitstrmIn -> getBits (4);
		Int uiPrefix = m_pbitstrmIn -> getBits (NUMBITS_START_CODE_PREFIX);
		ASSERT(uiPrefix == START_CODE_PREFIX);
*/
		uiVopStartCode = m_pbitstrmIn -> getBits (NUMBITS_VOP_START_CODE);
	}
	ASSERT(uiVopStartCode == VOP_START_CODE);

	m_vopmd.vopPredType = (VOPpredType) m_pbitstrmIn -> getBits (NUMBITS_VOP_PRED_TYPE);
	// Time reference and VOP_pred_type
	Int iModuloInc = 0;
	while (m_pbitstrmIn -> getBits (1) != 0)
		iModuloInc++;
	Time tCurrSec = iModuloInc + ((m_vopmd.vopPredType != BVOP || 
		(m_vopmd.vopPredType == BVOP && m_volmd.volType == ENHN_LAYER )) ?
		m_tModuloBaseDecd : m_tModuloBaseDisp);
	UInt uiMarker = m_pbitstrmIn -> getBits (1);
	ASSERT(uiMarker == 1);
	Time tVopIncr = 0;
	if(m_iNumBitsTimeIncr!=0)
		tVopIncr = m_pbitstrmIn -> getBits (m_iNumBitsTimeIncr);

	m_pbitstrmIn -> gotoBookmark ();

	Time t = tCurrSec * m_volmd.iClockRate*m_iClockRateScale + tVopIncr*m_iClockRateScale;
	return(t);
}

Time CVideoObjectDecoder::getTime ()
{
	return m_t;
}

int CVideoObjectDecoder::getPredType ()
{
	return m_vopmd.vopPredType;
}

Void CVideoObjectDecoder::BackgroundComposition(char* argv[], Bool bScalability, Int width, Int height, FILE *pfYUV) // modified by Sharp (98/10/26)
{
    Int iFrame = getTime ();	// current frame nubmer
    Int iPrev;			// previous base frame 
    Int iNext;			// next     base frame 

    if(!(m_pBuffB1 -> empty ()))  iPrev = m_pBuffB1 -> m_t;
    else		          iPrev = m_pBuffP1 -> m_t;
    if(!(m_pBuffB2 -> empty ()))  iNext = m_pBuffB2 -> m_t;
    else		          iNext = m_pBuffP2 -> m_t;

    Void convertYuv(const CVOPU8YUVBA* pvopcSrc, PixelC* destY, PixelC* destU, PixelC* destV, Int width, Int height);
    Void convertSeg(const CVOPU8YUVBA* pvopcSrc, PixelC* destBY,PixelC* destBUV,              Int width, Int height,
			    Int left, Int right, Int top, Int bottom
		    );
    Void bg_comp_each(PixelC* curr, PixelC* prev, PixelC* next, PixelC* mask_curr, PixelC* mask_prev, PixelC* mask_next, 
		      Int curr_t, Int prev_t, Int next_t, Int width, Int height, Int CoreMode); // modified by Sharp (98/3/24)
    Void inv_convertYuv(const CVOPU8YUVBA* pvopcSrc, PixelC* destY, PixelC* destU, PixelC* destV, Int width, Int height);
    //Void write420(Int num, char *name, PixelC* destY, PixelC* destU, PixelC* destV, Int width, Int height);
    Void write420_sep(Int num, char *name, PixelC* destY, PixelC* destU, PixelC* destV, Int width, Int height); // modified by Sharp (98/10/26)
    Void write420_jnt(FILE *pfYUV, PixelC* destY, PixelC* destU, PixelC* destV, Int width, Int height); // added by Sharp (98/10/26)
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
    if(!(m_pBuffB1 -> empty ()))
      convertYuv(m_pBuffB1->m_pvopcBuf, prevY, prevU, prevV, width, height);
    else
      convertYuv(m_pBuffP1->m_pvopcBuf, prevY, prevU, prevV, width, height);

    convertSeg(rgpbfShape [1]->pvopcReconCurr (), prevBY, prevBUV, width, height,
	       rgpbfShape [1]->m_rctCurrVOPY.left,
	       rgpbfShape [1]->m_rctCurrVOPY.right,
	       rgpbfShape [1]->m_rctCurrVOPY.top,
	       rgpbfShape [1]->m_rctCurrVOPY.bottom); // forward shape

//    write_test(iPrev, "bbb", prevY, prevU, prevV, width, height);
//    write_seg_test(iPrev, "bbb", prevBY, width, height);

    // next frame
    if(!(m_pBuffB2 -> empty ()))
      convertYuv(m_pBuffB2->m_pvopcBuf, nextY, nextU, nextV, width, height);
    else
      convertYuv(m_pBuffP2->m_pvopcBuf, nextY, nextU, nextV, width, height);
    convertSeg(rgpbfShape [0]->pvopcReconCurr (), nextBY, nextBUV, width, height,
	       rgpbfShape [0]->m_rctCurrVOPY.left,
	       rgpbfShape [0]->m_rctCurrVOPY.right,
	       rgpbfShape [0]->m_rctCurrVOPY.top,
	       rgpbfShape [0]->m_rctCurrVOPY.bottom); // backward shape

//    write_test(iNext, "bbb", nextY, nextU, nextV, width, height);
//    write_seg_test(iNext, "bbb", nextBY, width, height);

// begin: modified by Sharp (98/3/24)
//OBSSFIX_MODE3
    bg_comp_each(currY, prevY, nextY, currBY,  prevBY,  nextBY,  iFrame, iPrev, iNext, width,   height, m_vopmd.bBGComposition == 0);
    bg_comp_each(currU, prevU, nextU, currBUV, prevBUV, nextBUV, iFrame, iPrev, iNext, width/2, height/2, m_vopmd.bBGComposition == 0);
    bg_comp_each(currV, prevV, nextV, currBUV, prevBUV, nextBUV, iFrame, iPrev, iNext, width/2, height/2, m_vopmd.bBGComposition == 0);
//    bg_comp_each(currY, prevY, nextY, currBY,  prevBY,  nextBY,  iFrame, iPrev, iNext, width,   height, BGComposition == 0);
//    bg_comp_each(currU, prevU, nextU, currBUV, prevBUV, nextBUV, iFrame, iPrev, iNext, width/2, height/2, BGComposition == 0);
//    bg_comp_each(currV, prevV, nextV, currBUV, prevBUV, nextBUV, iFrame, iPrev, iNext, width/2, height/2, BGComposition == 0);
//~OBSSFIX_MODE3
// end: modified by Sharp (98/3/24)

//    inv_convertYuv(pvopcReconCurr(), currY, currU, currV, width, height);

		// write background compsition image
#ifdef __OUT_ONE_FRAME_ // added by Sharp (98/10/26)
	static Char pchYUV [100];
	sprintf (pchYUV, "%s.yuv", argv [2 + bScalability]); // modified by Sharp (98/10/26)
    write420_sep(iFrame, pchYUV, currY, currU, currV, width, height);
// begin: added by Sharp (98/10/26)
#else
    write420_jnt(pfYUV, currY, currU, currV, width, height);
#endif
// end: added by Sharp (98/10/26)

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

//OBSS_SAIT_991015		//for OBSS partial enhancement mode
//OBSSFIX_MODE3
Bool CVideoObjectDecoder::BackgroundCompositionSS(Int width, Int height, FILE *pfYUV, FILE *pfSeg, const CVOPU8YUVBA *pvopcRefBaseLayer) 
//Bool CVideoObjectDecoder::BackgroundComposition(Int width, Int height, FILE *pfYUV, FILE *pfSeg) 
//~OBSSFIX_MODE3
{
    Int iFrame = getTime ();	// current frame nubmer

    Void convertYuv(const CVOPU8YUVBA* pvopcSrc, PixelC* destY, PixelC* destU, PixelC* destV, Int width, Int height);
    Void convertSeg(const CVOPU8YUVBA* pvopcSrc, PixelC* destBY,PixelC* destBUV,              Int width, Int height,
			    Int left, Int right, Int top, Int bottom
		    );
    Void bg_comp_each(PixelC* curr, PixelC* prev, PixelC* mask_curr, PixelC* mask_prev, Int curr_t, Int width, Int height, CRct rctCurrVOPY); 
//OBSSFIX_MODE3
    Void bg_comp_each_mode3(PixelC* curr, PixelC* prev, PixelC* mask_curr, PixelC* mask_prev, Int curr_t, Int width, Int height, CRct rctCurrVOPY); 
//~OBSSFIX_MODE3
    Void inv_convertYuv(const CVOPU8YUVBA* pvopcSrc, PixelC* destY, PixelC* destU, PixelC* destV, Int width, Int height);
    Void write420_sep(Int num, char *name, PixelC* destY, PixelC* destU, PixelC* destV, Int width, Int height); // modified by Sharp (98/10/26)
    Void write420_jnt(FILE *pfYUV, PixelC* destY, PixelC* destU, PixelC* destV, Int width, Int height); // added by Sharp (98/10/26)
//OBSSFIX_MODE3
    Void write420_jnt_withMask(FILE *pfYUV, PixelC* destY, PixelC* destU, PixelC* destV, PixelC* destBY, PixelC* destBUV, Int width, Int height); // added by Sharp (98/10/26)
//~OBSSFIX_MODE3
    Void write_seg_test(Int num, char *name, PixelC* destY, Int width, Int height);

//OBSSFIX_MODE3
	CVOPU8YUVBA *pvopcUpSampled = NULL;
	if (m_volmd.iEnhnType == 1 && m_volmd.iuseRefShape ==1) 
		pvopcUpSampled = pvopcRefBaseLayer->upsampleForSpatialScalability (                                                                                
														  m_volmd.iver_sampling_factor_m,  
														  m_volmd.iver_sampling_factor_n, 
														  m_volmd.ihor_sampling_factor_m,         
														  m_volmd.ihor_sampling_factor_n,         
														  m_volmd.iver_sampling_factor_m_shape,         
														  m_volmd.iver_sampling_factor_n_shape,         
														  m_volmd.ihor_sampling_factor_m_shape,         
														  m_volmd.ihor_sampling_factor_n_shape,         
														  m_volmd.iFrmWidth_SS, 
														  m_volmd.iFrmHeight_SS,
														  m_volmd.bShapeOnly,   
														  EXPANDY_REF_FRAME,
														  EXPANDUV_REF_FRAME);
//~OBSSFIX_MODE3

//OBSSFIX_MODE3
	if (m_vopmd.bBGComposition == 0)	
		return FALSE;
//	if (BGComposition == 0)	
//		return FALSE;
//~OBSSFIX_MODE3
    PixelC* currY = new PixelC [width*height];
    PixelC* currU = new PixelC [width*height/4];
    PixelC* currV = new PixelC [width*height/4];
    PixelC* currBY  = new PixelC [width*height];
    PixelC* currBUV = new PixelC [width*height/4];
//OBSSFIX_MODE3
    PixelC* currBUV_tmp = new PixelC [width*height/4];
//~OBSSFIX_MODE3

    PixelC* prevY = new PixelC [width*height];
    PixelC* prevU = new PixelC [width*height/4];
    PixelC* prevV = new PixelC [width*height/4];
    PixelC* prevBY  = new PixelC [width*height];
    PixelC* prevBUV = new PixelC [width*height/4];

    // current frame
    convertYuv(pvopcReconCurr(), currY, currU, currV, width, height);
    if(pvopcReconCurr ()->pixelsBY () != NULL)
      convertSeg(pvopcReconCurr(), currBY, currBUV, width, height,
	       m_rctCurrVOPY.left,
	       m_rctCurrVOPY.right,
	       m_rctCurrVOPY.top,
	       m_rctCurrVOPY.bottom); 
	else
		memset(currBY,255,width*height);  


//OBSSFIX_MODE3
	if (m_volmd.iEnhnType == 1 && m_volmd.iuseRefShape ==1) {
		convertYuv(pvopcUpSampled, prevY, prevU, prevV, width, height);
		if(pvopcUpSampled->fAUsage() != RECTANGLE)
			convertSeg(pvopcUpSampled, prevBY, prevBUV, width, height,
						(pvopcUpSampled->whereBoundY ()).left,
						(pvopcUpSampled->whereBoundY ()).right,
						(pvopcUpSampled->whereBoundY ()).top,
						(pvopcUpSampled->whereBoundY ()).bottom); 
		else
			memset (prevBY, 255, width*height);
	}
	else {
		// previouse frame
		if(m_vopmd.vopPredType != BVOP){
			convertYuv(m_pvopcRefQ0, prevY, prevU, prevV, width, height);
			convertSeg(m_pvopcRefQ0, prevBY, prevBUV, width, height,
				   (m_pvopcRefQ0->whereBoundY ()).left,
				   (m_pvopcRefQ0->whereBoundY ()).right,
				   (m_pvopcRefQ0->whereBoundY ()).top,
				   (m_pvopcRefQ0->whereBoundY ()).bottom); 
		}
		else {
			convertYuv(m_pvopcCurrQ, prevY, prevU, prevV, width, height);
			convertSeg(m_pvopcCurrQ, prevBY, prevBUV, width, height,
				   (m_pvopcCurrQ->whereBoundY ()).left,
				   (m_pvopcCurrQ->whereBoundY ()).right,
				   (m_pvopcCurrQ->whereBoundY ()).top,
				   (m_pvopcCurrQ->whereBoundY ()).bottom); 
		}
	}
//    // previouse frame
//	if(m_vopmd.vopPredType != BVOP){
//		convertYuv(m_pvopcRefQ0, prevY, prevU, prevV, width, height);
//		convertSeg(m_pvopcRefQ0, prevBY, prevBUV, width, height,
//			   (m_pvopcRefQ0->whereBoundY ()).left,
//			   (m_pvopcRefQ0->whereBoundY ()).right,
//			   (m_pvopcRefQ0->whereBoundY ()).top,
//			   (m_pvopcRefQ0->whereBoundY ()).bottom); 
//	}
//	else {
//		convertYuv(m_pvopcCurrQ, prevY, prevU, prevV, width, height);
//		convertSeg(m_pvopcCurrQ, prevBY, prevBUV, width, height,
//			   (m_pvopcCurrQ->whereBoundY ()).left,
//			   (m_pvopcCurrQ->whereBoundY ()).right,
//			   (m_pvopcCurrQ->whereBoundY ()).top,
//			   (m_pvopcCurrQ->whereBoundY ()).bottom); 
//	}
//~OBSSFIX_MODE3

//OBSSFIX_MODE3
    memcpy(currBUV_tmp, currBUV, width*height/4); //add
	if (m_volmd.iEnhnType == 1 && m_volmd.iuseRefShape ==1) {
	    bg_comp_each_mode3(currY, prevY, currBY,  prevBY,  iFrame, width,   height, m_rctCurrVOPY);
	    bg_comp_each_mode3(currU, prevU, currBUV_tmp, prevBUV, iFrame, width/2, height/2, m_rctCurrVOPUV); //modified
		bg_comp_each_mode3(currV, prevV, currBUV, prevBUV, iFrame, width/2, height/2, m_rctCurrVOPUV);
	}
	else {
	    bg_comp_each(currY, prevY, currBY,  prevBY,  iFrame, width,   height, m_rctCurrVOPY);
	    bg_comp_each(currU, prevU, currBUV_tmp, prevBUV, iFrame, width/2, height/2, m_rctCurrVOPUV); //modified
		bg_comp_each(currV, prevV, currBUV, prevBUV, iFrame, width/2, height/2, m_rctCurrVOPUV);
	}
//    bg_comp_each(currY, prevY, currBY,  prevBY,  iFrame, width,   height, m_rctCurrVOPY);
//    bg_comp_each(currU, prevU, currBUV, prevBUV, iFrame, width/2, height/2, m_rctCurrVOPUV);
//    bg_comp_each(currV, prevV, currBUV, prevBUV, iFrame, width/2, height/2, m_rctCurrVOPUV);
//~OBSSFIX_MODE3

		// write background compsition image
#ifdef __OUT_ONE_FRAME_ // added by Sharp (98/10/26)
		// write background compsition image
		static Char pchYUV [100];
		sprintf (pchYUV, "%s/%2.2d/%s.yuv", pchReconYUVDir, iobj, pchPrefix); // modified by Sharp (98/10/26)
    write420_sep(iFrame, pchYUV, currY, currU, currV, width, height); // modified by Sharp (98/10/26)
// begin: added by Sharp (98/9/27)
#else
//OBSSFIX_MODE3
//	write420_jnt(pfYUV, currY, currU, currV, width, height);
	if (m_volmd.iEnhnType == 1 && m_volmd.iuseRefShape ==1) 
		write420_jnt(pfYUV, currY, currU, currV, width, height);
	else
		write420_jnt_withMask(pfYUV, currY, currU, currV, currBY, currBUV, width, height);
//~OBSSFIX_MODE3
    fwrite(currBY, sizeof (PixelC), width*height, pfSeg);
#endif

    delete currY;
    delete currU;
    delete currV;
    delete currBY;
    delete currBUV;
//OBSSFIX_MODE3
	delete currBUV_tmp;
	delete pvopcUpSampled;
//~OBSSFIX_MODE3

    delete prevY;
    delete prevU;
    delete prevV;
    delete prevBY;
    delete prevBUV;

	return TRUE;
}
//~OBSS_SAIT_991015

Void CVideoObjectDecoder::dumpDataOneFrame(char* argv[], Bool bScalability, CRct& rctDisplay)
{
	Int iFrame = getTime ();

//	cout << "\n=== write with Sharp's format : frame no. = " << iFrame << "\n";

	static Char pchYUV [100], pchSeg [100];

//	if(m_volmd.volType == BASE_LAYER) { // deleted by Sharp (98/11/11)
		sprintf (pchYUV, "%s.yuv", argv [2 + bScalability]);
// begin: added by Sharp (99/1/25)
		if ( m_volmd.iEnhnType == 1 && m_volmd.volType == ENHN_LAYER )
			sprintf (pchSeg, "%s_e.seg", argv [2 + bScalability]);
		else
// end: added by Sharp (99/1/25)
			sprintf (pchSeg, "%s.seg", argv [2 + bScalability]);
// begin: deleted by Sharp (98/11/11)
//	}
//	else {
//		sprintf (pchYUV, "%s_e.yuv", argv [2 + bScalability]);
//		sprintf (pchSeg, "%s_e.seg", argv [2 + bScalability]);
//	}
// end: deleted by Sharp (98/11/11)

	sprintf (pchYUV, "%s%d", pchYUV, iFrame);
	sprintf (pchSeg, "%s%d", pchSeg, iFrame);

	FILE* pfYuvDst = fopen(pchYUV, "wb");

	const CVOPU8YUVBA* pvopcQuant = pvopcReconCurr ();
	if ( m_volmd.iEnhnType != 1 ){ // added by Sharp (99/1/18)
	pvopcQuant->getPlane (Y_PLANE)->dump (pfYuvDst, rctDisplay);
	pvopcQuant->getPlane (U_PLANE)->dump (pfYuvDst, rctDisplay / 2);
	pvopcQuant->getPlane (V_PLANE)->dump (pfYuvDst, rctDisplay / 2);
	} // added by Sharp (99/1/18)
	fclose(pfYuvDst);

	if (m_volmd.fAUsage != RECTANGLE) {
		FILE* pfSegDst = fopen(pchSeg, "wb");
		pvopcQuant->getPlane (BY_PLANE)->dump (pfSegDst, rctDisplay, m_vopmd.iVopConstantAlphaValue);
		fclose(pfSegDst);
	}
}

Void CVideoObjectDecoder::dumpDataAllFrame(FILE* pfReconYUV, FILE* pfReconSeg, CRct& rctDisplay)
{
	const CVOPU8YUVBA* pvopcQuant = pvopcReconCurr ();
	  if ( m_volmd.iEnhnType != 1 ){ // added by Sharp (99/1/18)
	pvopcQuant->getPlane (Y_PLANE)->dump (pfReconYUV, rctDisplay);
	pvopcQuant->getPlane (U_PLANE)->dump (pfReconYUV, rctDisplay / 2);
	pvopcQuant->getPlane (V_PLANE)->dump (pfReconYUV, rctDisplay / 2);
	} // added by Sharp (99/1/18)
	if (m_volmd.fAUsage != RECTANGLE)
		pvopcQuant->getPlane (BY_PLANE)->dump (pfReconSeg, rctDisplay, m_vopmd.iVopConstantAlphaValue);
}

void CVideoObjectDecoder::copyTobfShape ()
{
            rgpbfShape[0]->m_pistrm      = m_pistrm;
            rgpbfShape[0]->m_pbitstrmIn  = m_pbitstrmIn;
            rgpbfShape[0]->m_pentrdecSet = m_pentrdecSet;
            rgpbfShape[1]->m_pistrm      = m_pistrm;
            rgpbfShape[1]->m_pbitstrmIn  = m_pbitstrmIn;
            rgpbfShape[1]->m_pentrdecSet = m_pentrdecSet;
}

// begin: added by Sharp (98/6/26)
void CVideoObjectDecoder::setClockRateScale ( CVideoObjectDecoder *pvopdec )
{
	m_iClockRateScale = pvopdec->m_volmd.iClockRate / m_volmd.iClockRate;
}
	// end: added by Sharp (98/6/26)


/******************************************
***	class CEnhcBufferDecoder	***
******************************************/
CEnhcBufferDecoder::CEnhcBufferDecoder (Int iSessionWidth, Int iSessionHeight)
:	CEnhcBuffer (iSessionWidth, iSessionHeight)
{
}

Void CEnhcBufferDecoder::copyBuf (const CEnhcBufferDecoder& srcBuf)
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

Void CEnhcBufferDecoder::getBuf(CVideoObjectDecoder* pvopc)  // get params from Base layer
{
//	ASSERT(pvopc->m_volmd.volType == BASE_LAYER);
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
	m_iNumMBRef = pvopc->m_iNumMBRef;
	m_iNumMBXRef = pvopc->m_iNumMBXRef;
	m_iNumMBYRef = pvopc->m_iNumMBYRef;
	}
	else {

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
///// 97/12/22 start
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
///// 97/12/22 end
}

Void CEnhcBufferDecoder::putBufToQ0(CVideoObjectDecoder* pvopc)  // store params to Enhancement layer pvopcRefQ0
{
	ASSERT(pvopc->m_volmd.volType == ENHN_LAYER);

	delete pvopc->m_pvopcRefQ0;
	pvopc->m_pvopcRefQ0 = NULL;
	pvopc->m_pvopcRefQ0 = new CVOPU8YUVBA ( *m_pvopcBuf );

	pvopc->m_bCodedFutureRef = m_bCodedFutureRef; // added by Sharp (99/1/26)
	if ( pvopc->m_volmd.iEnhnType == 1  &&
	(((pvopc->m_vopmd.iRefSelectCode == 1 || pvopc->m_vopmd.iRefSelectCode == 2) && pvopc->m_vopmd.vopPredType == PVOP) ||
	 (pvopc->m_vopmd.iRefSelectCode == 3 && pvopc->m_vopmd.vopPredType == BVOP))
	)
	{
		CRct rctRefFrameYTemp = pvopc->m_rctRefFrameY;
		CRct rctRefFrameUVTemp = pvopc->m_rctRefFrameUV;
		rctRefFrameYTemp.expand(-EXPANDY_REF_FRAME);
		rctRefFrameUVTemp.expand(-EXPANDY_REF_FRAME/2);
		pvopc->m_pvopcRefQ0->addBYPlain(rctRefFrameYTemp, rctRefFrameUVTemp);
	}

//	if ( pvopc->m_volmd.fAUsage != RECTANGLE ){	
		CMBMode* pmbmdRefSrc = m_rgmbmdRef;
		CMBMode* pmbmdRef = pvopc->m_rgmbmdRef;
		CMotionVector* pmvRefSrc = m_rgmvRef;
		CMotionVector* pmvRef = pvopc->m_rgmvRef;
		pvopc->m_iNumMBRef = m_iNumMBRef;
		pvopc->m_iNumMBXRef = m_iNumMBXRef;
		pvopc->m_iNumMBYRef = m_iNumMBYRef;

		// TPS FIX
		Int iMaxIndex = max(PVOP_MV_PER_REF_PER_MB, 2*BVOP_MV_PER_REF_PER_MB);

		for (Int iMB=0; iMB<m_iNumMBRef; iMB++ ) {
			*pmbmdRef = *pmbmdRefSrc;
			if ( pvopc->m_volmd.iEnhnType == 1  &&
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
//	}

	pvopc->repeatPadYOrA ((PixelC*) pvopc->m_pvopcRefQ0->pixelsY () + m_iOffsetForPadY, pvopc->m_pvopcRefQ0);
	pvopc->repeatPadUV(pvopc->m_pvopcRefQ0);
	if ( pvopc->m_volmd.fAUsage != RECTANGLE ){
		if ( pvopc->m_volmd.fAUsage == EIGHT_BIT )
			pvopc->repeatPadYOrA ((PixelC*) pvopc->m_pvopcRefQ0->pixelsA (0) + m_iOffsetForPadY, pvopc->m_pvopcRefQ0);
	}
}

Void CEnhcBufferDecoder::putBufToQ1(CVideoObjectDecoder* pvopc)  // store params to Enhancement layer pvopcRefQ1
{
	ASSERT(pvopc->m_volmd.volType == ENHN_LAYER);

	delete pvopc->m_pvopcRefQ1;
	pvopc->m_pvopcRefQ1 = NULL;
	pvopc->m_pvopcRefQ1 = new CVOPU8YUVBA ( *m_pvopcBuf );
	pvopc->m_bCodedFutureRef = m_bCodedFutureRef; // added by Sharp (99/1/26)
	if ( pvopc->m_volmd.iEnhnType == 1  &&
	(((pvopc->m_vopmd.iRefSelectCode == 1 || pvopc->m_vopmd.iRefSelectCode == 2) && pvopc->m_vopmd.vopPredType == PVOP) ||
	((pvopc->m_vopmd.iRefSelectCode == 1 || pvopc->m_vopmd.iRefSelectCode == 3) && pvopc->m_vopmd.vopPredType == BVOP)) // modified by Sharp (98/5/25)
	)
	{
		CRct rctRefFrameYTemp = pvopc->m_rctRefFrameY;
		CRct rctRefFrameUVTemp = pvopc->m_rctRefFrameUV;
		rctRefFrameYTemp.expand(-EXPANDY_REF_FRAME);
		rctRefFrameUVTemp.expand(-EXPANDY_REF_FRAME/2);
		pvopc->m_pvopcRefQ1->addBYPlain(rctRefFrameYTemp, rctRefFrameUVTemp); // modified by Sharp (98/5/25)
	}

//	if ( pvopc->m_volmd.fAUsage != RECTANGLE ){
		CMBMode* pmbmdRefSrc = m_rgmbmdRef;
		CMBMode* pmbmdRef = pvopc->m_rgmbmdRef;
		CMotionVector* pmvRefSrc = m_rgmvRef;
		CMotionVector* pmvRef = pvopc->m_rgmvRef;

		// This part is stored in CVideoObjectDecoder::decode()
		pvopc->m_iNumMBRef = m_iNumMBRef;
		pvopc->m_iNumMBXRef = m_iNumMBXRef;
		pvopc->m_iNumMBYRef = m_iNumMBYRef;

		// TPS FIX
		Int iMaxIndex = max(PVOP_MV_PER_REF_PER_MB, 2*BVOP_MV_PER_REF_PER_MB);

		for (Int iMB=0; iMB<m_iNumMBRef; iMB++ ) {
			*pmbmdRef = *pmbmdRefSrc;
			if ( pvopc->m_volmd.iEnhnType == 1 ) {
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
		if ( pvopc->m_volmd.fAUsage == EIGHT_BIT )
			pvopc->repeatPadYOrA ((PixelC*) pvopc->m_pvopcRefQ1->pixelsA (0) + m_iOffsetForPadY, pvopc->m_pvopcRefQ1);
	}
}





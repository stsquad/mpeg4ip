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

*************************************************************************/
#include <stdio.h>
#include <fstream.h>
#ifdef __GNUC__
#include <strstream.h>
#else
#include <strstrea.h>
#endif
#include <math.h>

#include "typeapi.h"
#include "codehead.h"
#include "entropy/bytestrm_file.hpp"
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

#ifdef __MFC_
#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

#define new DEBUG_NEW				   
#endif // __MFC_

#define _FOR_GSSP_
#undef assert
#define assert(a) if (!(a)) throw((int)0);
CVideoObjectDecoder::~CVideoObjectDecoder ()
{
//	delete m_pistrm;
  if (m_bcreatedByteStream)
    delete m_pbytestrmIn;
	delete m_pbitstrmIn;
	delete m_pentrdecSet;
	delete m_pvopcRightMB;
}


Int CVideoObjectDecoder::h263_decode ()
{
	static Bool first_time = TRUE;

	if (!first_time) 
	{
		while ( m_pbitstrmIn -> peekBits(NUMBITS_SHORT_HEADER_START_CODE) != SHORT_VIDEO_START_MARKER)
		{
			if(m_pbitstrmIn->eof()==EOF)  // [FDS] 
				return EOF;
			m_pbitstrmIn -> getBits(1);
		}

	
		m_pbitstrmIn -> getBits(22);
		video_plane_with_short_header(); 
	}
	else
		first_time = FALSE;

	m_bUseGOV=FALSE; 
	m_bLinkisBroken=FALSE;
	m_vopmd.iRoundingControl=0;
	m_vopmd.iIntraDcSwitchThr=0; 
	m_vopmd.bInterlace=FALSE;
	m_vopmd.bAlternateScan=FALSE;
	m_t=1; 
	m_vopmd.mvInfoForward.uiFCode=1; 
	m_vopmd.mvInfoForward.uiScaleFactor = 1 << (m_vopmd.mvInfoForward.uiFCode - 1);
	m_vopmd.mvInfoForward.uiRange = 16 << m_vopmd.mvInfoForward.uiFCode;
	//	Added for error resilient mode by Toshiba(1998-1-16)
	m_vopmd.mvInfoBackward.uiFCode = 1;
	//	End Toshiba(1998-1-16)
	m_vopmd.bShapeCodingType=1;

	// set time stamps for Base/Temporal-Enhc/Spatial-Enhc Layer	Modified by Sharp(1998-02-10)
	if(m_volmd.volType == BASE_LAYER) {
		if(m_vopmd.vopPredType==IVOP || m_vopmd.vopPredType==PVOP) {
			m_tPastRef = m_tFutureRef; 
			m_tFutureRef = m_t; 
			m_iBCount = 0;
		}
		
	}
	
	// set time stamps for Base/Temporal-Enhc/Spatial-Enhc Layer	End 	    Sharp(1998-02-10)

	// select reference frames for Base/Temporal-Enhc/Spatial-Enhc Layer	Modified by Sharp(1998-02-10)
	if(m_volmd.volType == BASE_LAYER) {
		updateAllRefVOPs ();            // update all reconstructed VOP'sm_pvopcRefQ1->vdlDump ("c:\\refq1.vdl");
	}
	
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

Void CVideoObjectDecoder::video_plane_with_short_header()
{
  /* UInt uiTemporalReference = wmay */ m_pbitstrmIn -> getBits (8);
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
	

}


void CVideoObjectDecoder::decodeShortHeaderIntraMBDC(Int *rgiCoefQ)
{
	UInt uiIntraDC;
	uiIntraDC=m_pbitstrmIn->getBits(8);
	if (uiIntraDC==128||uiIntraDC==0) fprintf(stderr,"IntraDC = 0 of 128, not allowed in H.263 mode\n");
	if (uiIntraDC==255) uiIntraDC=128;
	
	rgiCoefQ[0]=uiIntraDC;
}


CVideoObjectDecoder::CVideoObjectDecoder (CInByteStreamBase *pbytestrmIn) : CVideoObject()
{
  m_t = m_tPastRef = m_tFutureRef = 0;
  m_iBCount = 0;
  m_vopmd.iVopConstantAlphaValue = 255;
  short_video_header = FALSE;
  m_bcreatedByteStream = FALSE;
  m_pbytestrmIn = pbytestrmIn;	
  m_pbitstrmIn = new CInBitStream (m_pbytestrmIn);
  m_pentrdecSet = new CEntropyDecoderSet (*m_pbitstrmIn);

}

CVideoObjectDecoder::CVideoObjectDecoder (
	const Char* pchStrFile,
	Int iDisplayWidth, Int iDisplayHeight,
	Bool *pbSpatialScalability,
	Bool *p_short_video_header//,
	//strstreambuf* pistrm
	) : CVideoObject ()
{
#if 0
	if (pistrm == NULL) {
#endif
		m_pistrm = new ifstream (pchStrFile, ios::binary | ios::in);
		if(!m_pistrm->is_open())
			fatal_error("Can't open bitstream file");
#if 0
	}
	else {
		m_pistrm = (ifstream *)new istream (pistrm);
	}
#endif
	m_pbytestrmIn = new CInByteStreamFile(*m_pistrm);
	m_bcreatedByteStream = TRUE;
	m_pbitstrmIn = new CInBitStream (m_pbytestrmIn);
	m_pentrdecSet = new CEntropyDecoderSet (*m_pbitstrmIn);

	m_t = m_tPastRef = m_tFutureRef = 0;
	m_iBCount = 0;

	m_vopmd.iVopConstantAlphaValue = 255;

	*p_short_video_header=FALSE; // Added by KPN for short headers


	if (m_pbitstrmIn->peekBits(NUMBITS_SHORT_HEADER_START_CODE) == SHORT_VIDEO_START_MARKER) 
 
	{ 

		fprintf(stderr, "\nBitstream with short header format detected\n"); 
		*p_short_video_header=TRUE;  
		m_pbitstrmIn -> getBits(22);
		video_plane_with_short_header();
	}
	else {

	fprintf(stderr,"\nBitstream without short headers detected\n");
	decodeVOHead ();
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
		if(pbSpatialScalability != NULL)
			if(m_volmd.iHierarchyType == 0)
				*pbSpatialScalability = TRUE;
			else 	
				*pbSpatialScalability = FALSE;
	}
	if (m_volmd.fAUsage == RECTANGLE) {
		if (m_volmd.volType == ENHN_LAYER &&
			(m_volmd.ihor_sampling_factor_n != m_volmd.ihor_sampling_factor_m ||
	     	 m_volmd.iver_sampling_factor_n != m_volmd.iver_sampling_factor_m )){
			iDisplayWidth = m_ivolWidth;
			iDisplayHeight= m_ivolHeight;

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

	if (m_volmd.fAUsage == RECTANGLE) {

		//wchen: if sprite; set it according to the initial piece instead
		m_rctCurrVOPY = (m_uiSprite == 0) ? CRct (0, 0, iDisplayWidthRound, iDisplayHeightRound) : m_rctSpt;

		m_rctCurrVOPUV = m_rctCurrVOPY.downSampleBy2 ();

		m_rctRefVOPY0 = m_rctCurrVOPY;
		m_rctRefVOPY0.expand (EXPANDY_REFVOP);
		m_rctRefVOPUV0 = m_rctRefVOPY0.downSampleBy2 ();
		
		m_rctRefVOPY1 = m_rctRefVOPY0;
		m_rctRefVOPUV1 = m_rctRefVOPUV0;

		computeVOLConstMembers (); // these VOP members are the same for all frames
	}
	// buffer for shape decoding
	m_pvopcRightMB = new CVOPU8YUVBA (m_volmd.fAUsage, CRct (0, 0, MB_SIZE, MB_SIZE));
	m_ppxlcRightMBBY = (PixelC*) m_pvopcRightMB->pixelsBY ();
	m_ppxlcRightMBBUV = (PixelC*) m_pvopcRightMB->pixelsBUV ();

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
}
// for back/forward shape	Added by Sharp(1998-02-10)
CVideoObjectDecoder::CVideoObjectDecoder (
	Int iDisplayWidth, Int iDisplayHeight
) : CVideoObject ()
{
	m_pistrm = NULL;
	m_bcreatedByteStream = FALSE;
	m_pbytestrmIn = NULL;
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

	// buffer for shape decoding
	m_pvopcRightMB = new CVOPU8YUVBA (m_volmd.fAUsage, CRct (0, 0, MB_SIZE, MB_SIZE));
	m_ppxlcRightMBBY = (PixelC*) m_pvopcRightMB->pixelsBY ();
	m_ppxlcRightMBBUV = (PixelC*) m_pvopcRightMB->pixelsBUV ();
}
// for back/forward shape	End	 Sharp(1998-02-10)

Int CVideoObjectDecoder::decode (const CVOPU8YUVBA* pvopcBVOPQuant, /*strstreambuf* pistrm, */ Bool waitForI, Bool drop)
{
#if 0
	if (pistrm != NULL) {
		delete (istream *)m_pistrm;
		delete m_pbytestrmIn;
		delete m_pbitstrmIn;
		delete m_pentrdecSet;
		m_pistrm = (ifstream *)new istream (pistrm);
		m_pbytestrmIn = new CInByteStreamFile(*m_pistrm);
		m_pbitstrmIn = new CInBitStream (m_pbytestrmIn);
		m_pentrdecSet = new CEntropyDecoderSet (*m_pbitstrmIn);
	}
#endif
	//sprite piece should not come here
	assert ((m_vopmd.SpriteXmitMode == STOP) || ( m_vopmd.SpriteXmitMode == PAUSE));

	if (findStartCode () == EOF)
		return EOF;

	Bool bCoded = decodeVOPHead (); // set the bounding box here
	if (waitForI && 
	    !(m_vopmd.vopPredType == IVOP)) {
#ifdef DEBUG_OUTPUT
	  cout << "\tFrame is not IVOP " << m_vopmd.vopPredType << "\n";
	  cout.flush();
#endif
	  return -1;
	}
	if (drop && m_vopmd.vopPredType == BVOP) {
	  return -1;
	}
#ifdef DEBUG_OUTPUT
	cout << "\t" << "Time..." << m_t << " (" << m_t / (double)m_volmd.iClockRate << " sec)\n";
	if(bCoded == FALSE)
		cout << "\tNot coded.\n";
	cout.flush ();
#endif

	Bool bPrevRefVopWasCoded = m_bCodedFutureRef;
	if(m_vopmd.vopPredType==IVOP || m_vopmd.vopPredType==PVOP)
		m_bCodedFutureRef = bCoded; // flag used by bvop prediction

	if (m_vopmd.vopPredType == SPRITE)	{
		decodeSpt ();
		return TRUE;
	}

	// set time stamps for Base/Temporal-Enhc/Spatial-Enhc Layer	Modified by Sharp(1998-02-10)
	if(m_volmd.volType == BASE_LAYER) {
		if(m_vopmd.vopPredType==IVOP || m_vopmd.vopPredType==PVOP) {
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
			if (pvopcBVOPQuant == NULL)		// Temporal Scalability Enhancement Layer
				updateRefVOPsEnhc ();
			else {					// Spatial Scalability Enhancement Layer
					updateAllRefVOPs (pvopcBVOPQuant);
			}
		}
	}
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
		return FALSE;
	}

	if (m_volmd.fAUsage != RECTANGLE)
		resetBYPlane ();
	
	if (m_volmd.fAUsage != RECTANGLE) {
		setRefStartingPointers ();
		computeVOPMembers ();
	}

	decodeVOP ();

	//wchen: added by sony-probably not the best way
	if(m_volmd.volType == ENHN_LAYER &&
		(m_vopmd.vopPredType == BVOP && m_vopmd.iRefSelectCode == 0))
		swapVOPU8Pointers(m_pvopcCurrQ,m_pvopcRefQ1);

	// store the direct mode data
	if (m_vopmd.vopPredType != BVOP ||
		(m_volmd.volType == ENHN_LAYER && m_vopmd.vopPredType == BVOP && m_vopmd.iRefSelectCode == 0)) {
		if(m_volmd.fAUsage != RECTANGLE && bPrevRefVopWasCoded)
			saveShapeMode();
		CMBMode* pmbmdTmp = m_rgmbmd;
		m_rgmbmd = m_rgmbmdRef;
		m_rgmbmdRef = pmbmdTmp;
		CMotionVector* pmvTmp = m_rgmv;
		m_rgmv = m_rgmvRef;
		m_rgmvRef = pmvTmp;
		m_rgmvBackward = m_rgmv + BVOP_MV_PER_REF_PER_MB * m_iSessNumMB;
	}

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
		Int nBlk = (m_volmd.fAUsage == EIGHT_BIT) ? 10 : 6;
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
		repeatPadYOrA ((PixelC*) m_pvopcRefQ1->pixelsY () + m_iOffsetForPadY, m_pvopcRefQ1);
		repeatPadUV (m_pvopcRefQ1);

		//reset by in RefQ1 so that no left-over from last frame
		if (m_volmd.fAUsage != RECTANGLE)       {
			if (m_volmd.fAUsage == EIGHT_BIT)
				repeatPadYOrA ((PixelC*) m_pvopcRefQ1->pixelsA () + m_iOffsetForPadY, m_pvopcRefQ1);
		}
	}

	// update buffers for temporal scalability	Added by Sharp(1998-02-10)
	if(m_volmd.volType != BASE_LAYER)  updateBuffVOPsEnhc ();

	return TRUE;
}

Int CVideoObjectDecoder::findStartCode()
{
	// ensure byte alignment
	m_pbitstrmIn->flush ();

	Int bUserData;
	do {
		bUserData = 0;
		while(m_pbitstrmIn->peekBits(NUMBITS_START_CODE_PREFIX)!=START_CODE_PREFIX)
		{
			m_pbitstrmIn->getBits(8);
			if(m_pbitstrmIn->eof()==EOF)
				return EOF;
		}
		m_pbitstrmIn->getBits(NUMBITS_START_CODE_PREFIX);
		if(m_pbitstrmIn->peekBits(NUMBITS_START_CODE_SUFFIX)==USER_DATA_START_CODE)
			bUserData = 1;
	} while(bUserData);

	return 0;
}

Void CVideoObjectDecoder::decodeVOHead ()
{
	findStartCode();
 	//	Added for data partitioning mode By Toshiba(1998-1-16:DP+RVLC)(for Skip Session_Start_Code)
 	if( m_pbitstrmIn->peekBits(NUMBITS_START_CODE_SUFFIX)==SESSION_START_CODE){
 		m_pbitstrmIn -> getBits (NUMBITS_START_CODE_SUFFIX);
 		if (findStartCode () == EOF)
 			assert(FALSE);
 	}
 	//	End Toshiba(1998-1-16:DP+RVLC)
	UInt uiVoStartCode = m_pbitstrmIn -> getBits (NUMBITS_VO_START_CODE);
	assert(uiVoStartCode == VO_START_CODE);

	m_uiVOId = m_pbitstrmIn -> getBits (NUMBITS_VO_ID);
}

Void CVideoObjectDecoder::decodeVOLHead ()
{
  int ver_id;

	findStartCode();
	UInt uiVolStartCode = m_pbitstrmIn -> getBits (NUMBITS_VOL_START_CODE);
	assert(uiVolStartCode == VOL_START_CODE);
	/* UInt uiVOLId = wmay */m_pbitstrmIn -> getBits (NUMBITS_VOL_ID);
// Begin: modified by Hughes	  4/9/98	  per clause 2.1.7. in N2171 document
	/* Bool bRandom = wmay */m_pbitstrmIn->getBits (1); //VOL_Random_Access
// End: modified by Hughes	  4/9/98
	/* UInt uiOLType = wmay*/m_pbitstrmIn -> getBits(8); // VOL_type_indication
	UInt uiOLI = m_pbitstrmIn -> getBits (1); //VOL_Is_Object_Layer_Identifier, useless flag for now
	if(uiOLI)
	{
	  ver_id = m_pbitstrmIn -> getBits (4); // video_oject_layer_verid
		m_pbitstrmIn -> getBits (3); // video_oject_layer_priority
	} else {
	  ver_id = 1;
	}

	//assert(uiOLI == 0);
	
	UInt uiAspect = m_pbitstrmIn -> getBits (4);
	if(uiAspect==15) // extended PAR
	{
	  /* UInt iParWidth = wmay */ m_pbitstrmIn -> getBits (8);
	  /* UInt iParHeight = wmay */ m_pbitstrmIn -> getBits (8);
	}

	UInt uiMark;
	UInt uiCTP = m_pbitstrmIn -> getBits (1); //VOL_Control_Parameter, useless flag for now
	if(uiCTP)
	{
	  /* UInt uiChromaFormat = wmay */ m_pbitstrmIn -> getBits (2);
	  /* UInt uiLowDelay = wmay */m_pbitstrmIn -> getBits (1);
		UInt uiVBVParams = m_pbitstrmIn -> getBits (1);
		
		if(uiVBVParams)
		{
		  /* UInt uiFirstHalfBitRate = wmay */m_pbitstrmIn -> getBits (15);
			uiMark = m_pbitstrmIn -> getBits (1);
			assert(uiMark==1);
			/* UInt uiLatterHalfBitRate = wmay */ m_pbitstrmIn -> getBits (15);
			uiMark = m_pbitstrmIn -> getBits (1);
			assert(uiMark==1);
			/* UInt uiFirstHalfVbvBufferSize = wmay */m_pbitstrmIn -> getBits (15);
			uiMark = m_pbitstrmIn -> getBits (1);
			assert(uiMark==1);
			/* UInt uiLatterHalfVbvBufferSize = wmay */m_pbitstrmIn -> getBits (3);
			/* UInt uiFirstHalfVbvBufferOccupany = wmay */m_pbitstrmIn -> getBits (11);
			uiMark = m_pbitstrmIn -> getBits (1);
			assert(uiMark==1);
			/* UInt uiLatterHalfVbvBufferOccupany = wmay */m_pbitstrmIn -> getBits (15);
			uiMark = m_pbitstrmIn -> getBits (1);
			assert(uiMark==1);
		}
	}

	//assert(uiCTP == 0);

	UInt uiAUsage = m_pbitstrmIn -> getBits (NUMBITS_VOL_SHAPE);
	uiMark = m_pbitstrmIn -> getBits (1);
	assert(uiMark==1);
	m_volmd.iClockRate = m_pbitstrmIn -> getBits (NUMBITS_TIME_RESOLUTION);
#ifdef DEBUG_OUTPUT
	cout << m_volmd.iClockRate << "\n";
#endif
	uiMark = m_pbitstrmIn -> getBits (1);
	assert(uiMark==1);
	Int iClockRate = m_volmd.iClockRate;
	assert (iClockRate >= 1 && iClockRate < 65536);
	for (m_iNumBitsTimeIncr = 1; m_iNumBitsTimeIncr < NUMBITS_TIME_RESOLUTION; m_iNumBitsTimeIncr++)	{	
		if (iClockRate == 1)			
			break;
		iClockRate = (iClockRate >> 1);
	}

	Bool bFixFrameRate = m_pbitstrmIn -> getBits (1);
	//assert (bFixFrameRate == FALSE);
	if(bFixFrameRate)
	{
	  UInt uiFixedVOPTimeIncrement = m_pbitstrmIn -> getBits (m_iNumBitsTimeIncr);
	  m_volmd.iClockRate = m_volmd.iClockRate / uiFixedVOPTimeIncrement;
		// not used
		//
		//
	}

	if(uiAUsage==2)  // shape-only mode
	{
	  /* UInt uiResyncMarkerDisable = wmay */m_pbitstrmIn -> getBits (1);

		// default to some values - probably not all needed
		m_volmd.bShapeOnly=TRUE;
		m_volmd.fAUsage=ONE_BIT;
		m_volmd.bAdvPredDisable = 0;
		m_volmd.fQuantizer = Q_H263;
		m_volmd.volType = BASE_LAYER;
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
		//wmay for divx assert(uiMarker==1);
		m_ivolWidth = m_pbitstrmIn -> getBits (NUMBITS_VOP_WIDTH);
		uiMarker  = m_pbitstrmIn -> getBits (1);
		// wmay for divx assert(uiMarker==1);
		m_ivolHeight = m_pbitstrmIn -> getBits (NUMBITS_VOP_HEIGHT);
		uiMarker  = m_pbitstrmIn -> getBits (1);
		// wmay for dixv assert(uiMarker==1);
	}

	m_vopmd.bInterlace = m_pbitstrmIn -> getBits (1); // interlace (was vop flag)
	m_volmd.bAdvPredDisable = m_pbitstrmIn -> getBits (1);  //VOL_obmc_Disable

	// decode sprite info
	// wmay m_uiSprite = m_pbitstrmIn -> getBits (NUMBITS_SPRITE_USAGE);
	m_uiSprite = m_pbitstrmIn->getBits(ver_id == 1 ? 1 : 2);
	if (m_uiSprite == 1) { // sprite information
		Int isprite_hdim = m_pbitstrmIn -> getBits (NUMBITS_SPRITE_HDIM);
		Int iMarker = m_pbitstrmIn -> getBits (MARKER_BIT);
		assert (iMarker == 1);
		Int isprite_vdim = m_pbitstrmIn -> getBits (NUMBITS_SPRITE_VDIM);
		iMarker = m_pbitstrmIn -> getBits (MARKER_BIT);
		assert (iMarker == 1);
		Int isprite_left_edge = (m_pbitstrmIn -> getBits (1) == 0) ?
		    m_pbitstrmIn->getBits (NUMBITS_SPRITE_LEFT_EDGE - 1) : ((Int)m_pbitstrmIn->getBits (NUMBITS_SPRITE_LEFT_EDGE - 1) - (1 << 12));
		assert(isprite_left_edge%2 == 0);
		iMarker = m_pbitstrmIn -> getBits (MARKER_BIT);
		assert (iMarker == 1);
		Int isprite_top_edge = (m_pbitstrmIn -> getBits (1) == 0) ?
		    m_pbitstrmIn->getBits (NUMBITS_SPRITE_TOP_EDGE - 1) : ((Int)m_pbitstrmIn->getBits (NUMBITS_SPRITE_LEFT_EDGE - 1) - (1 << 12));
		assert(isprite_top_edge%2 == 0);
		iMarker = m_pbitstrmIn -> getBits (MARKER_BIT);
		assert (iMarker == 1);
		m_rctSpt.left = isprite_left_edge;
		m_rctSpt.right = isprite_left_edge + isprite_hdim;
		m_rctSpt.top = isprite_top_edge;
		m_rctSpt.bottom = isprite_top_edge + isprite_vdim;
		m_rctSpt.width = isprite_hdim;
        m_rctSptPieceY = m_rctSpt; //initialization; will be overwritten by first vop header
		m_iNumOfPnts = m_pbitstrmIn -> getBits (NUMBITS_NUM_SPRITE_POINTS);
		m_rgstSrcQ = new CSiteD [m_iNumOfPnts];
		m_rgstDstQ = new CSiteD [m_iNumOfPnts];
		m_uiWarpingAccuracy = m_pbitstrmIn -> getBits (NUMBITS_WARPING_ACCURACY);
		/* Bool bLightChange = wmay */m_pbitstrmIn -> getBits (1);

// Begin: modified by Hughes	  4/9/98
		Bool bsptMode = m_pbitstrmIn -> getBits (1);  // Low_latency_sprite_enable
		if (  bsptMode)
			m_sptMode = LOW_LATENCY ;
		else			
			m_sptMode = BASIC_SPRITE ;
// End: modified by Hughes	  4/9/98
	}

	m_volmd.bNot8Bit = (Bool) m_pbitstrmIn -> getBits(1);
	if (m_volmd.bNot8Bit) {
		m_volmd.uiQuantPrecision = (UInt) m_pbitstrmIn -> getBits (4);
		m_volmd.nBits = (UInt) m_pbitstrmIn -> getBits (4);
		assert(m_volmd.nBits>3);
	} else {
		m_volmd.uiQuantPrecision = 5;
		m_volmd.nBits = 8;
	}

	if (m_volmd.fAUsage == EIGHT_BIT)
	{
		m_volmd.bNoGrayQuantUpdate = m_pbitstrmIn -> getBits (1);
		/* UInt uiCompMethod = wmay */m_pbitstrmIn -> getBits (1);
		/* UInt uiLinearComp = wmay */m_pbitstrmIn -> getBits (1);
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
			m_volmd.bLoadIntraMatrixAlpha = m_pbitstrmIn -> getBits (1);
			if (m_volmd.bLoadIntraMatrixAlpha) {
				for (UInt i = 0; i < BLOCK_SQUARE_SIZE; i++)	{
					Int iElem = m_pbitstrmIn -> getBits (NUMBITS_QMATRIX);
					if (iElem != 0)
						m_volmd.rgiIntraQuantizerMatrixAlpha [grgiStandardZigzag[i]] = iElem;
					else
						m_volmd.rgiIntraQuantizerMatrixAlpha [i] = m_volmd.rgiIntraQuantizerMatrixAlpha [grgiStandardZigzag[i - 1]];
				}
			}
			else {
#ifdef _FOR_GSSP_
				memcpy (m_volmd.rgiIntraQuantizerMatrixAlpha, rgiDefaultIntraQMatrix, BLOCK_SQUARE_SIZE * sizeof (Int));
#else
				for (UInt i = 0; i < BLOCK_SQUARE_SIZE; i++)
					m_volmd.rgiIntraQuantizerMatrixAlpha [i] = 16;
#endif
			}
			m_volmd.bLoadInterMatrixAlpha = m_pbitstrmIn -> getBits (1);
			if (m_volmd.bLoadInterMatrixAlpha) {
				for (UInt i = 0; i < BLOCK_SQUARE_SIZE; i++)	{
					Int iElem = m_pbitstrmIn -> getBits (NUMBITS_QMATRIX);
					if (iElem != 0)
						m_volmd.rgiInterQuantizerMatrixAlpha [grgiStandardZigzag[i]] = iElem;
					else
						m_volmd.rgiInterQuantizerMatrixAlpha [i] = m_volmd.rgiInterQuantizerMatrixAlpha [grgiStandardZigzag[i - 1]];
				}
			}
			else {
#ifdef _FOR_GSSP_
				memcpy (m_volmd.rgiInterQuantizerMatrixAlpha, rgiDefaultInterQMatrix, BLOCK_SQUARE_SIZE * sizeof (Int));
#else
				for (UInt i = 0; i < BLOCK_SQUARE_SIZE; i++)
					m_volmd.rgiInterQuantizerMatrixAlpha [i] = 16;
#endif
			}
		}
	}
	if (ver_id != 1) // wmay
	  m_pbitstrmIn->getBits(1); // wmay vol quarter pixel
	
	// Bool bComplxityEsti = m_pbitstrmIn->getBits (1); //Complexity estimation; don't know how to use it

// START: Complexity Estimation syntax support - Marc Mongenet (EPFL) - 15 Jun 1998

	m_volmd.bComplexityEstimationDisable = m_pbitstrmIn -> getBits (1);
	if (! m_volmd.bComplexityEstimationDisable) {

		m_volmd.iEstimationMethod = m_pbitstrmIn -> getBits (2);
		if (m_volmd.iEstimationMethod != 0) {
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
				fatal_error("Shape complexity estimation is enabled,\nbut no correponding flag is enabled.");
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
		if (! m_volmd.bTextureComplexityEstimationSet1Disable) {
			m_volmd.bIntraBlocks = m_pbitstrmIn -> getBits (1);
			m_volmd.bInterBlocks = m_pbitstrmIn -> getBits (1);
			m_volmd.bInter4vBlocks = m_pbitstrmIn -> getBits (1);
			m_volmd.bNotCodedBlocks = m_pbitstrmIn -> getBits (1);
			if (!(m_volmd.bIntraBlocks ||
				  m_volmd.bInterBlocks ||
				  m_volmd.bInter4vBlocks ||
				  m_volmd.bNotCodedBlocks)) {
				fatal_error("Texture complexity estimation set 1 is enabled,\nbut no correponding flag is enabled.");
			}
		}
		else
			m_volmd.bIntraBlocks =
			m_volmd.bInterBlocks =
			m_volmd.bInter4vBlocks =
			m_volmd.bNotCodedBlocks = false;
		
		uiMark = m_pbitstrmIn -> getBits (1);
		assert (uiMark == 1);

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
				fatal_error("Texture complexity estimation set 2 is enabled,\nbut no correponding flag is enabled.");
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
				fatal_error("Motion complexity estimation is enabled,\nbut no correponding flag is enabled.");
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
		assert (uiMark == 1);
	}
	// END: Complexity Estimation syntax support

	/* UInt uiResyncMarkerDisable = wmay */m_pbitstrmIn -> getBits (1);

	//	Modified by Toshiba(1998-4-7)
	m_volmd.bDataPartitioning = m_pbitstrmIn -> getBits (1);
	if( m_volmd.bDataPartitioning )
		m_volmd.bReversibleVlc = m_pbitstrmIn -> getBits (1);
	else
		m_volmd.bReversibleVlc = FALSE;
	//	End Toshiba
	//wchen: wd changes
	m_volmd.volType = (m_pbitstrmIn -> getBits (1) == 0) ? BASE_LAYER : ENHN_LAYER;
	m_volmd.ihor_sampling_factor_n = 1;
	m_volmd.ihor_sampling_factor_m = 1;
	m_volmd.iver_sampling_factor_n = 1;
	m_volmd.iver_sampling_factor_m = 1;	
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
  postVO_VOLHeadInit(w, h, pbSpatialScalability);
}


Int BGComposition; // added by Sharp (98/3/24)

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
		assert(uiPrefix == START_CODE_PREFIX);
*/
		uiVopStartCode = m_pbitstrmIn -> getBits (NUMBITS_VOP_START_CODE);
	}
//980212
	assert(uiVopStartCode == VOP_START_CODE);

	m_vopmd.vopPredType = (VOPpredType) m_pbitstrmIn -> getBits (NUMBITS_VOP_PRED_TYPE);
	// Time reference and VOP_pred_type
	Int iModuloInc = 0;
	while (m_pbitstrmIn -> getBits (1) != 0)
		iModuloInc++;
	Time tCurrSec = iModuloInc + ((m_vopmd.vopPredType != BVOP ||
				  (m_vopmd.vopPredType == BVOP && m_volmd.volType == ENHN_LAYER ))?
				m_tModuloBaseDecd : m_tModuloBaseDisp);
	//	Added for error resilient mode by Toshiba(1997-11-14)
	UInt uiMarker = m_pbitstrmIn -> getBits (1);
	assert(uiMarker== 1);
	//	End Toshiba(1997-11-14)
	Time tVopIncr = m_pbitstrmIn -> getBits (m_iNumBitsTimeIncr);
	uiMarker = m_pbitstrmIn->getBits (1); // marker bit
	assert(uiMarker ==1);
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

	if (m_vopmd.vopPredType == PVOP && m_volmd.bShapeOnly==FALSE)
		m_vopmd.iRoundingControl = m_pbitstrmIn->getBits (1); //"VOP_Rounding_Type"
	else
		m_vopmd.iRoundingControl = 0;

	if (m_volmd.fAUsage != RECTANGLE) {
// Begin: modified by Hughes	  4/9/98
	  if (!(m_uiSprite == 1 && m_vopmd.vopPredType == IVOP)) {
// End: modified by Hughes	  4/9/98

		Int width = m_pbitstrmIn -> getBits (NUMBITS_VOP_WIDTH);
// spt VOP		assert (width % MB_SIZE == 0); // for sprite, may not be multiple of MB_SIZE
		Int marker;
		marker = m_pbitstrmIn -> getBits (1); // marker bit
		assert(marker==1);
		Int height = m_pbitstrmIn -> getBits (NUMBITS_VOP_HEIGHT);
// spt VOP		assert (height % MB_SIZE == 0); // for sprite, may not be multiple of MB_SIZE
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
		assert(((left | top)&1)==0); // must be even pix unit

		m_rctCurrVOPY = CRct (left, top, left + width, top + height);
		m_rctCurrVOPUV = m_rctCurrVOPY.downSampleBy2 ();
	  }


		if ( m_volmd.volType == ENHN_LAYER && m_volmd.iEnhnType == 1 ){
			BGComposition = m_pbitstrmIn -> getBits (1); // modified by Sharp (98/3/24)
//			assert(BackgroundComposition==1); // modified by Sharp (98/3/24)
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

		if (m_volmd.iEstimationMethod != 0) {
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
			m_vopmd.vopPredType == SPRITE) {

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
			m_vopmd.vopPredType == SPRITE) {

			if (m_volmd.bInterpolateMCQ) {
				printf ("dcecs_interpolate_mc_q = %d\n", m_vopmd.iInterpolateMCQ = m_pbitstrmIn -> getBits (8));
				if (m_vopmd.iInterpolateMCQ == 0) {
					fprintf (stderr, "ERROR: Illegal null value for 'interpolate_mc_q' complexity estimation.\n");
					exit (1);
				}
			}
		}
	}
	// END: Complexity Estimation syntax support

	if(m_volmd.bShapeOnly==TRUE)
	{
		m_vopmd.intStep = 10; // probably not needed 
		m_vopmd.intStepI = 10;
		m_vopmd.intStepB = 10;
		m_vopmd.mvInfoForward.uiFCode = m_vopmd.mvInfoBackward.uiFCode = 1;	//	Modified error resilient mode by Toshiba (1998-1-16)
		m_vopmd.bInterlace = FALSE;	//wchen: temporary solution
		return TRUE;
	}
	m_vopmd.iIntraDcSwitchThr = m_pbitstrmIn->getBits (3);

// INTERLACE
	if (m_vopmd.bInterlace) {
		m_vopmd.bTopFieldFirst = m_pbitstrmIn -> getBits (1);
        m_vopmd.bAlternateScan = m_pbitstrmIn -> getBits (1);
		assert(m_volmd.volType == BASE_LAYER);
	} else
// ~INTERLACE
    {
        m_vopmd.bAlternateScan = FALSE;
    }

	if (m_vopmd.vopPredType == IVOP)	{ 
		m_vopmd.intStepI = m_vopmd.intStep = m_pbitstrmIn -> getBits (m_volmd.uiQuantPrecision);	//also assign intStep to be safe
		m_vopmd.mvInfoForward.uiFCode = m_vopmd.mvInfoBackward.uiFCode = 1;		//	Modified error resilient mode by Toshiba(1998-1-16)
		if(m_volmd.fAUsage == EIGHT_BIT)
			m_vopmd.intStepIAlpha = m_pbitstrmIn -> getBits (NUMBITS_VOP_ALPHA_QUANTIZER);
	}
	else if (m_vopmd.vopPredType == PVOP) {
		m_vopmd.intStep = m_pbitstrmIn -> getBits (m_volmd.uiQuantPrecision);
		if(m_volmd.fAUsage == EIGHT_BIT)
			m_vopmd.intStepPAlpha = m_pbitstrmIn -> getBits (NUMBITS_VOP_ALPHA_QUANTIZER);

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
			m_vopmd.intStepBAlpha = m_pbitstrmIn -> getBits (NUMBITS_VOP_ALPHA_QUANTIZER);
		
		m_vopmd.mvInfoForward.uiFCode = m_pbitstrmIn -> getBits (NUMBITS_VOP_FCODE);
		m_vopmd.mvInfoForward.uiScaleFactor = 1 << (m_vopmd.mvInfoForward.uiFCode - 1);
		m_vopmd.mvInfoForward.uiRange = 16 << m_vopmd.mvInfoForward.uiFCode;
		m_vopmd.mvInfoBackward.uiFCode = m_pbitstrmIn -> getBits (NUMBITS_VOP_FCODE);
		m_vopmd.mvInfoBackward.uiScaleFactor = 1 << (m_vopmd.mvInfoBackward.uiFCode - 1);
		m_vopmd.mvInfoBackward.uiRange = 16 << m_vopmd.mvInfoBackward.uiFCode;
	}
	//	Added for error resilient mode by Toshiba(1997-11-14)
	m_vopmd.bShapeCodingType = 1;
	if ( m_volmd.volType == BASE_LAYER ) // added by Sharp (98/4/13)
        if (m_volmd.fAUsage != RECTANGLE && m_vopmd.vopPredType != IVOP
				&& m_uiSprite != 1)
		{	
			m_vopmd.bShapeCodingType = m_pbitstrmIn -> getBits (1);
        }
	// End Toshiba(1997-11-14)
	if(m_volmd.volType == ENHN_LAYER) {					//sony
		if( m_volmd.iEnhnType == 1 ){
			m_vopmd.iLoadBakShape = m_pbitstrmIn -> getBits (1); // load_backward_shape
			if(m_vopmd.iLoadBakShape){
				CVOPU8YUVBA* pvopcCurr = new CVOPU8YUVBA (*(rgpbfShape [0]->pvopcReconCurr()));
				copyVOPU8YUVBA(rgpbfShape [1]->m_pvopcRefQ1, pvopcCurr);
					// previous backward shape is saved to current forward shape
				rgpbfShape [1]->m_rctCurrVOPY.left   = rgpbfShape [0]->m_rctCurrVOPY.left;
				rgpbfShape [1]->m_rctCurrVOPY.right  = rgpbfShape [0]->m_rctCurrVOPY.right;
				rgpbfShape [1]->m_rctCurrVOPY.top    = rgpbfShape [0]->m_rctCurrVOPY.top;
				rgpbfShape [1]->m_rctCurrVOPY.bottom = rgpbfShape [0]->m_rctCurrVOPY.bottom;

				Int width = m_pbitstrmIn -> getBits (NUMBITS_VOP_WIDTH); assert (width % MB_SIZE == 0); // has to be multiples of MB_SIZE
				UInt uiMark = m_pbitstrmIn -> getBits (1);
				assert (uiMark == 1);
				Int height = m_pbitstrmIn -> getBits (NUMBITS_VOP_HEIGHT); assert (height % MB_SIZE == 0); // has to be multiples of MB_SIZE
				uiMark = m_pbitstrmIn -> getBits (1);
				assert (uiMark == 1);
				width = ((width+15)>>4)<<4; // not needed if the asserts are present
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
					width = m_pbitstrmIn -> getBits (NUMBITS_VOP_WIDTH); assert (width % MB_SIZE == 0); // has to be multiples of MB_SIZE
					uiMark = m_pbitstrmIn -> getBits (1);
					assert (uiMark == 1);
					height = m_pbitstrmIn -> getBits (NUMBITS_VOP_HEIGHT); assert (height % MB_SIZE == 0); // has to be multiples of MB_SIZE
					uiMark = m_pbitstrmIn -> getBits (1);
					assert (uiMark == 1);
					width = ((width+15)>>4)<<4; // not needed if the asserts are present
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
	assert(uiVopStartCode == VOP_START_CODE);

	Int	vopPredType = (VOPpredType) m_pbitstrmIn -> getBits (NUMBITS_VOP_PRED_TYPE);
	// Time reference and VOP_pred_type
	
	m_pbitstrmIn -> gotoBookmark ();

	return(vopPredType);
}

Void CVideoObjectDecoder::errorInBitstream (Char* rgchErorrMsg)
{
	fprintf (stderr, "%s at %d\n", rgchErorrMsg, m_pbitstrmIn->getCounter ());
	assert (FALSE);
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

	float tmp_right,tmp_left,tmp_top,tmp_bottom,tmp_width;
	
	tmp_right = (float) rctDisplay.right * ((float) m_volmd.ihor_sampling_factor_n / m_volmd.ihor_sampling_factor_m);
	tmp_left  = (float) rctDisplay.left  * ((float) m_volmd.ihor_sampling_factor_n / m_volmd.ihor_sampling_factor_m);
	tmp_width = (float) rctDisplay.width * ((float) m_volmd.ihor_sampling_factor_n / m_volmd.ihor_sampling_factor_m);

	tmp_top     = (float) rctDisplay.top * ((float) m_volmd.iver_sampling_factor_n / m_volmd.iver_sampling_factor_m);
	tmp_bottom  = (float) rctDisplay.bottom  * ((float) m_volmd.iver_sampling_factor_n / m_volmd.iver_sampling_factor_m);

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
		assert(uiPrefix == START_CODE_PREFIX);
*/
		uiVopStartCode = m_pbitstrmIn -> getBits (NUMBITS_VOP_START_CODE);
	}
	assert(uiVopStartCode == VOP_START_CODE);

	m_vopmd.vopPredType = (VOPpredType) m_pbitstrmIn -> getBits (NUMBITS_VOP_PRED_TYPE);
	// Time reference and VOP_pred_type
	Int iModuloInc = 0;
	while (m_pbitstrmIn -> getBits (1) != 0)
		iModuloInc++;
	Time tCurrSec = iModuloInc + ((m_vopmd.vopPredType != BVOP || 
		(m_vopmd.vopPredType == BVOP && m_volmd.volType == ENHN_LAYER )) ?
		m_tModuloBaseDecd : m_tModuloBaseDisp);
	UInt uiMarker = m_pbitstrmIn -> getBits (1);
	assert(uiMarker == 1);
	Time tVopIncr = m_pbitstrmIn -> getBits (m_iNumBitsTimeIncr);

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
    bg_comp_each(currY, prevY, nextY, currBY,  prevBY,  nextBY,  iFrame, iPrev, iNext, width,   height, BGComposition == 0);
    bg_comp_each(currU, prevU, nextU, currBUV, prevBUV, nextBUV, iFrame, iPrev, iNext, width/2, height/2, BGComposition == 0);
    bg_comp_each(currV, prevV, nextV, currBUV, prevBUV, nextBUV, iFrame, iPrev, iNext, width/2, height/2, BGComposition == 0);
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
void CVideoObjectDecoder::set_byte_stream(CInByteStreamBase *p)
{ m_pbytestrmIn = p;
 m_pbitstrmIn->attach(p);
}


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
	assert(pvopc->m_volmd.volType == ENHN_LAYER);

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
			pvopc->repeatPadYOrA ((PixelC*) pvopc->m_pvopcRefQ0->pixelsA () + m_iOffsetForPadY, pvopc->m_pvopcRefQ0);
	}
}

Void CEnhcBufferDecoder::putBufToQ1(CVideoObjectDecoder* pvopc)  // store params to Enhancement layer pvopcRefQ1
{
	assert(pvopc->m_volmd.volType == ENHN_LAYER);

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
			pvopc->repeatPadYOrA ((PixelC*) pvopc->m_pvopcRefQ1->pixelsA () + m_iOffsetForPadY, pvopc->m_pvopcRefQ1);
	}
}


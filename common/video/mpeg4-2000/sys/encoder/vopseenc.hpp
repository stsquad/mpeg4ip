/*************************************************************************

This software module was originally developed by 

	Ming-Chieh Lee (mingcl@microsoft.com), Microsoft Corporation
	Wei-ge Chen (wchen@microsoft.com), Microsoft Corporation
	Bruce Lin (blin@microsoft.com), Microsoft Corporation
	Chuang Gu (chuanggu@microsoft.com), Microsoft Corporation
	(date: March, 1996)

and edited by
	Yoshihiro Kikuchi (TOSHIBA CORPORATION)
	Takeshi Nagai (TOSHIBA CORPORATION)
	Toshiaki Watanabe (TOSHIBA CORPORATION)
	Noboru Yamaguchi (TOSHIBA CORPORATION)

	Marc Mongenet (Marc.Mongenet@epfl.ch), Swiss Federal Institute of Technology, Lausanne (EPFL)

    Mathias Wien (wien@ient.rwth-aachen.de), RWTH Aachen / Robert BOSCH GmbH 

and also edited by
	Yoshinori Suzuki (Hitachi, Ltd.)

and also edited by
    Fujitsu Laboratories Ltd. (contact: Eishi Morimatsu)
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
	Nov.  27, 1997: Spatial Scalable tools added 
						by Takefumi Nagumo(nagumo@av.crl.sony.co.jp) SONY corporation
	Dec 20, 1997:	Interlaced tools added by NextLevel Systems 
                    X. Chen (xchen@nlvl.com) B. Eifrig (beifrig@nlvl.com)
	Jun 16, 1998:	add Complexity Estimation syntax support
					Marc Mongenet (Marc.Mongenet@epfl.ch) - EPFL
    Feb.16, 1999:   add Quarter Sample 
                    Mathias Wien (wien@ient.rwth-aachen.de) 
	May  9, 1999:	tm5 rate control by DemoGraFX, duhoff@mediaone.net (added by mwi)
	Sep.06	1999 : RRV added by Eishi Morimatsu (Fujitsu Laboratories Ltd.) 

*************************************************************************/

#ifndef __VOPSEENC_HPP_ 
#define __VOPSEENC_HPP_

#ifndef SOURCE_FRAME_RATE				//assuming input source is always 30f/s
#define SOURCE_FRAME_RATE 30
#endif
#ifndef min
#define min MIN
#endif
#ifndef max
#define max MAX
#endif

#include "tm5rc.hpp"

class CFwdBlockDCT;

// HHI Schueuer:  scan selection classes to support the sadct
class CScanSelector {
public:
	virtual ~CScanSelector() {}

	//virtual const Int *select(const Int *scan, const Int *piCoeffWidths) { return scan; } 
	// Stefan Rauthenberg: <rauthenberg@HHI.DE> from the semantic point of view I'd prefer a return value of const Int* but 
	// this would cause conflicts with methods like decodeIntraTCOEF. 
	virtual Int *select(Int *scan, Bool bIsBoundary, Int iBlk) { return scan; } 
	// 09/19/99 HHI Schueuer
	virtual Int *select_DP(Int *scan, Bool bIsBoundary, Int iBlk, Int** rgiCurrMBCoeffWidth) {return scan; };
	// end 09/19/99
};

class CScanSelectorForSADCT: public CScanSelector {
public:
	CScanSelectorForSADCT(Int **rgiCurrMBCoeffWidth); 
	virtual ~CScanSelectorForSADCT();
	virtual Int *select(Int *scan, Bool bIsBoundary, Int iBlk);
	// 09/19/99 HHI Schueuer
	virtual Int *select_DP(Int *scan, Bool bIsBoundary, Int iBlk, Int** rgiCurrMBCoeffWidth);
	// end 09/19/99

private:
	Int **m_rgiCurrMBCoeffWidth;
	Int *m_adaptedScan;
};
// End HHI 

class CVideoObjectEncoder : public CVideoObject
{
friend class CSecurityDescriptor;
friend class CSessionEncoder;
// friend class CSessionEncoderTPS; ///// 97/12/22 // deleted by Sharp (98/2/12)
// friend class CVideoObjectEncoderTPS; ///// 97/12/22 // deleted by Sharp (98/2/12)
	friend class CEnhcBufferEncoder; // added by Sharp (98/2/10)
public:
	// Constructors
	~CVideoObjectEncoder ();
private:
  CVideoObjectEncoder ();				// default constructor - forbidden
public:
	CVideoObjectEncoder (
		UInt uiVOId,						// VO id
		VOLMode& volmd,						// VOL mode
		VOPMode& vopmd,						// VOP mode
		UInt nFirstFrame,					// number of total frames
		UInt nLastFrame,					// number of total frames
		Int iSessionWidth,					// session width, in case it's needed
		Int iSessionHeight,					// session height
		UInt uiRateControl,					// rate control type
		UInt uiBudget,						// bit budget for vop
		ostream* pstrmTrace,				// trace outstream
		UInt uiSprite,				// for sprite type // GMC
		UInt uiWarpAccuracy,				// for sprite warping
		Int iNumOfPnts,						// for sprite warping
		CSiteD** rgstDest,					// for sprite warping destination
		SptMode SpriteMode,					// sprite reconstruction mode
		CRct rctFrame,						// sprite warping source
		CRct rctSpt,                        // rct Sprite
		Int iMVFileUsage,					// 0==>no usage, 1==>read from MV file, 2==>write to MV file
		Char* pchMVFileName	    			// MV file name
	); // VOP mode

	// for back/forward shape
	CVideoObjectEncoder (
		UInt uiVOId,						// VO id
		VOLMode& volmd,						// VOL mode
		VOPMode& vopmd,						// VOP mode
		Int iSessionWidth,					// session width, in case it's needed
		Int iSessionHeight //,					// session height
	); // VOP mode

	// Attributes
	const COutBitStream* pOutStream () const {return m_pbitstrmOut;} // output bitstream
	const CStatistics& statVOL () const {return m_statsVOL;}
	const CStatistics& statVOP () const {return m_statsVOP;}

	// Operations
	Bool skipTest(
		Time t,
		VOPpredType vopPredType
	);
	Void swapSpatialScalabilityBVOP ();
	Void encode (
		Bool bVOP_Visible,				// whether the VOP at this time is encoded
		Time t,							// relative frame number for the current encoding
		VOPpredType vopPredType,
		const CVOPU8YUVBA* pvopfRefBaseLayer = NULL	//Reference image frm the base layer for spatial scalability
	);

// begin: added by Sharp (98/2/10)
	// for background composition
	Void BackgroundComposition (
				const Int width, Int height,
				const Int iFrame,	  
				const Int iPrev,	  
				const Int iNext,    
				const CVOPU8YUVBA* pvopcBuffP1,
				const CVOPU8YUVBA* pvopcBuffP2,
				const CVOPU8YUVBA* pvopcBuffB1,
				const CVOPU8YUVBA* pvopcBuffB2,
				const Char* pchReconYUVDir, Int iobj, const Char* pchPrefix, // for output file name
				FILE *pchfYUV  // added by Sharp (98/10/26)
	);
//OBSS_SAIT_991015
	//for base + enhancement layer composition(OBSS partial enhancement mode)
	Bool BackgroundComposition (
				const Int width, Int height,
				const Int iFrame,	  
				const CVOPU8YUVBA* pvopcBuffP1,
				const Char* pchReconYUVDir, Int iobj, const Char* pchPrefix, // for output file name
				FILE *pchfYUV,  // added by Sharp (98/10/26)
				FILE *pchfSeg
	);
//~OBSS_SAIT_991015
	Void set_LoadShape(
          Int* ieFramebShape, Int* ieFramefShape, // frame number for back/forward shape
          const Int iRate,	      // rate of enhancement layer
          const Int ieFrame,	      // current frame number
          const Int iFirstFrame,    // first frame number of sequence
          const Int iFirstFrameLoop // first frame number of the enhancement loop
        );
// end: added by Sharp (98/2/10)
///////////////// implementation /////////////////
///// 97/12/22 start
public:
	CVideoObjectEncoder* rgpbfShape [2]; // 0 : backward, 1: forward
///// 97/12/22 end

protected:
	//Time m_tIVOPCounter;
	//Time m_tEncodedVOPCounter;                      // for TPS only
	UInt m_nFirstFrame, m_nLastFrame, m_iBufferSize; //for rate control
	Int m_uiRateControl; // rate control type
	UInt m_uiGMCQP; // GMC_V2
	
	// bitstream stuff
	Char* m_pchBitsBuffer;
	COutBitStream* m_pbitstrmOut; // output bitstream
	Char* m_pchShapeBitsBuffer;
	COutBitStream* m_pbitstrmShape;
	COutBitStream* m_pbitstrmShapeMBOut;

//	Added for data partitioning mode By Toshiba(1998-1-16:DP+RVLC)
	Char** m_pchShapeBitsBuffer_DP;
	COutBitStream** m_pbitstrmShape_DP;
//	End Toshiba(1998-1-16:DP+RVLC)

	CEntropyEncoderSet* m_pentrencSet;
	
	// statistics 
	CStatistics m_statsVOL, m_statsVOP, m_statsMB;	// accumulated number of bits
	Double* m_rgdSNR;

	// for rate control
	CRCMode m_statRC;		// Rate control mode status
	TM5rc   m_tm5rc;
//	UInt m_uiTotalPrev;

	own CVOPU8YUVBA* m_pvopcOrig; // original reference VOP in a previous time
	own CVOPU8YUVBA* m_pvopcRefOrig0; // original reference VOP in a previous time
	own CVOPU8YUVBA* m_pvopcRefOrig1; // original reference VOP in a later time
	own CU8Image* m_puciRefQZoom0; // zoomed reference VOP in a previous time
	own CU8Image* m_puciRefQZoom1; // zoomed reference VOP in a later time

	// some fixed variables (VOL)
	Int m_iFrameWidthZoomYx2Minus2MB, m_iFrameWidthZoomYx2Minus2Blk;
	Int m_iFrameWidthZoomY, m_iFrameWidthZoomUV;

	// VOP variables
	CRct m_rctRefVOPZoom0, m_rctRefVOPZoom1;

	// for B-VOP
	// MB buffer data
	CU8Image *m_puciDirectPredMB, *m_puciInterpPredMB;
	PixelC *m_ppxlcDirectPredMBY, *m_ppxlcInterpPredMBY;

	CIntImage *m_piiDirectErrorMB, *m_piiInterpErrorMB;
	PixelI *m_ppxliDirectErrorMBY, *m_ppxliInterpErrorMBY;
	//moved to vopses.hpp
	//CVector m_vctDirectDeltaMV;	//MVDB for current MB
	
	// block data
	CFwdBlockDCT* m_pfdct;

	// error resilient variables
	Int m_iVopTimeIncr;
	UInt m_nBitsModuloBase;
	Int m_iVPCounter;//	Added for error resilient mode by Toshiba(1997-11-14)


	//	Added for data partitioning mode by Toshiba(1998-1-16)
	Int	m_numBitsVPMBnum;
	Int	m_numVideoPacket;
	//	End Toshiba(1998-1-16)

	// HHI Schueuer
	CScanSelector *m_pscanSelector;
	// end HHI

	// VO and VOL routines
	Void codeSequenceHead ();
	Void codeVOHead ();
	Void codeVOLHead (Int iSessionWidth, Int iSessionHeight);//, constt CRct& rctSprite);
	Void codeGOVHead (Time t);
	
// VOP routines
//	Void codeVOPHead (const CSiteD* rgstDest = NULL, CRct rctWarp = NULL);
	Int m_iMAD;  // for Rate Control
	Void codeVOPHead ();
	Void codeNonCodedVOPHead ();
	Void codeVOPHeadInitial();

	//Void decidePredType ();
	Void findTightBoundingBox ();
	Void findBestBoundingBox ();
	Void copyCurrToRefOrig1Y ();
	Void updateAllOrigRefVOPs ();
	Void biInterpolateY (
		const CVOPU8YUVBA* pvopcRefQ, const CRct& rctRefVOP, // reference VOP
		CU8Image* puciRefQZoom, const CRct& rctRefVOPZoom, Int iRoundingControl // reference zoomed VOP
	);

	Void encodeVOP ();
	Void encodeNSForIVOP ();
	Void encodeNSForIVOP_WithShape ();
	Void encodeNSForPVOP ();
	Void encodeNSForPVOP_WithShape ();
	Void encodeNSForBVOP ();
	Void encodeNSForBVOP_WithShape ();

    //classical sprite stuff
	Void encodeSptTrajectory (Time t, const CSiteD* rgstDest, const CRct& rctWarp); // code sprite info
	Void quantizeSptTrajectory (const CSiteD* rgstDest, CRct rctWarp);
	UInt codeWarpPoints ();
	//low latency sprite stuff
#define AVGPIECEMB 0
#define AVGUPDATEMB 1
	own CVOPU8YUVBA* m_pvopcSpt; // original sprite object
	//SptMode m_sptMode;  // sprite reconstruction mode : 0 -- basic sprite , 1 -- Object piece only, 2 -- Update piece only, 3 -- intermingled	
	CSiteD** m_pprgstDest; // destination sites
	Bool m_bSptZoom;  // the type of sprite warping(zoom/pan)
    Bool m_bSptHvPan; // the type of sprite warping(Horizontal or vertical panning)
	Bool m_bSptRightPiece; 	
	Int m_pSptmbBits[2]; // bits used by a sprite macroblock
	Int m_iNumSptMB; // bits used by a sprite macroblock
	CRct findTightBoundingBox (CVOPU8YUVBA* vopuc);
	CRct PieceExpand (const CRct& rctOrg);
	Void encodeInitSprite (const CRct& rctOrg) ;
	Void initialSpritePiece (Int iSessionWidth, Int iSessionHeight) ;
	CRct InitialPieceRect (Time ts);
	CRct CornerWarp (const CSiteD* rgstDest, const CSiteD* rgstSrcQ);	
	Void encodeSpritePiece (Time t) ; // code sprite pieces 
	Void codeVOSHead () ; // code sprite piece overhead
	Void encSptPiece (CRct rctSptQ, UInt uiSptPieceSize);		
	Void encodeP (Bool bVOPVisible, Time t)	 ; 
	CRct PieceSize (Bool rightpiece, UInt uiSptPieceSize);
	CRct encPiece (CRct rctpiece);
	CRct ZoomOrPan ();

	// GMC
	Void GlobalMotionEstimation(CSiteD* m_rgstDestQ,
		const CVOPU8YUVBA* pvopref, const CVOPU8YUVBA* pvopcurr,
		const CRct rctframe, const CRct rctref,const CRct rctcurr,
		Int iNumOfPnts);
	Void TranslationalGME(const CVOPU8YUVBA* pvopcurr,
		const CVOPU8YUVBA* pvopref, const CRct rctframe, const CRct rctcurr,
		const CRct rctref, GME_para *pparameter);
	Void IsotropicGME(const CVOPU8YUVBA* pvopcurr,
		const CVOPU8YUVBA* pvopref, const CRct rctframe, const CRct rctcurr,
		const CRct rctref, GME_para *pparameter);
	Void AffineGME(const CVOPU8YUVBA* pvopcurr,
		const CVOPU8YUVBA* pvopref, const CRct rctframe, const CRct rctcurr,
		const CRct rctref, GME_para *pparameter);
	Void ModifiedThreeStepSearch(PixelC *pref_work, PixelC *pcurr_work,
		Int icurr_width, Int icurr_height, Int iref_width, Int iref_height,
		Int icurr_left, Int icurr_top, Int icurr_right, Int icurr_bottom,
		Int iref_left, Int iref_top, Int iref_right, Int iref_bottom,
		Int *ibest_locationx, Int *ibest_locationy);
	Void ModifiedThreeStepSearch_WithShape(PixelC *pref_work,PixelC *pcurr_work,
		PixelC *pref_alpha_work, PixelC *pcurr_alpha_work,
		Int icurr_width, Int icurr_height, Int iref_width, Int iref_height,
		Int icurr_left, Int icurr_top, Int icurr_right, Int icurr_bottom,
		Int iref_left, Int iref_top, Int iref_right, Int iref_bottom,
		Int *ibest_locationx, Int *ibest_locationy);
	Void ThreeTapFilter(PixelC *pLow, PixelC *pHight,
		Int iwidth, Int iheight);
	Int DeltaMP(Double *dA, Int in, Double *db, Double *dm);
	Int CVideoObjectEncoder::globalme(CoordI iXCurr, CoordI iYCurr,
					PixelC *pref);
	Void CVideoObjectEncoder::StationalWarpME(Int iXCurr, Int iYCurr,
		PixelC* pref, Int& iSumofAD);
	Void CVideoObjectEncoder::TranslationalWarpME(Int iXCurr, Int iYCurr,
		PixelC* pref, Int& iSumofAD);
	Void CVideoObjectEncoder::FastAffineWarpME(Int iXCurr, Int iYCurr,
		PixelC* pref, Int& iSumofAD);
	// ~GMC

	// motion estimation
	Void motionEstPVOP ();
	Void motionEstPVOP_WithShape ();
	virtual Void motionEstBVOP ();
	virtual Void motionEstBVOP_WithShape ();

	//
	// MB routines
	//
	Void updateQP(CMBMode* pmbmd, Int iQPPrev, Int iQPNew);
	Void cancelQPUpdate(CMBMode* pmbmd);
	Void setAlphaQP(CMBMode* pmbmd);
	
	UInt sumAbsCurrMB (); // for Rate Control
	Void copyToCurrBuff (
		const PixelC* ppxlcCurrY, const PixelC* ppxlcCurrU, const PixelC* ppxlcCurrV,
		Int iWidthY, Int iWidthUV
	); 
	Void copyToCurrBuffWithShape (
		const PixelC* ppxlcCurrY, const PixelC* ppxlcCurrU, const PixelC* ppxlcCurrV,
		const PixelC* ppxlcCurrBY, const PixelC** ppxlcCurrA,
		Int iWidthY, Int iWidthUV
	);
	Void copyToCurrBuffJustShape(const PixelC* ppxlcCurrBY,Int iWidthY);

	Void LPEPadding (const CMBMode* pmbmd);
	Void LPEPaddingBlk (
		PixelC* ppxlcBlk, const PixelC* ppxlcBlkB,
		UInt uiSize
	);
	/*Void encodePVOPMBWithShape (
		PixelC* ppxlcRefMBY, PixelC* ppxlcRefMBU, PixelC* ppxlcRefMBV, PixelC* ppxlcRefMBA, PixelC* ppxlcRefBY,
		CMBMode* pmbmd, const CMotionVector* pmv, CMotionVector* pmvBY, ShapeMode shpmdColocatedMB,
		Int imbX, Int imbY,
		CoordI x, CoordI y, Int& iQPPrev
	);*/
	
	Void encodePVOPMBTextureWithShape(
		PixelC* ppxlcRefMBY, PixelC* ppxlcRefMBU, PixelC* ppxlcRefMBV,
		PixelC** ppxlcRefMBA, CMBMode* pmbmd, const CMotionVector* pmv,
		Int imbX, Int imbY,	CoordI x, CoordI y,
		Bool *pbUseNewQPForVlcThr,
		const PixelC *ppxlcCurrMBBY = NULL, const PixelC *ppxlcCurrMBBUV = NULL // HHI Schueuer
	);

	Void encodePVOPMBJustShape(
		PixelC* ppxlcRefBY, CMBMode* pmbmd, ShapeMode shpmdColocatedMB,
		const CMotionVector* pmv, CMotionVector* pmvBY,
		CoordI x, CoordI y, Int imbX, Int imbY
	);

	Void dumpCachedShapeBits();

	Int dumpCachedShapeBits_DP(Int iMBnum); //	Added for data partitioning mode By Toshiba(1998-1-16:DP+RVLC)

	Void encodePVOPMB (
		PixelC* ppxlcRefMBY, PixelC* ppxlcRefMBU, PixelC* ppxlcRefMBV,
// RRV modification
		CMBMode* pmbmd, const CMotionVector* pmv, const CMotionVector* pmv_RRV, 
// ~RRV
		Int imbX, Int imbY,
		CoordI x, CoordI y, Bool *pbRestart
	);
	// B-VOP MB encoding
	Void encodeBVOPMB (
		PixelC* ppxlcCurrQMBY, PixelC* ppxlcCurrQMBU, PixelC* ppxlcCurrQMBV,
		CMBMode* pmbmd, 
		const CMotionVector* pmv, const CMotionVector* pmvBackward,
		const CMBMode* pmbmdRef, const CMotionVector* pmvRef,
		Int imbX, Int imbY,
		CoordI x, CoordI y
	);

	// HHI schueuer: sadct
	// const PixelC *ppxlcCurrMBBY = NULL, const PixelC *ppxlcCurrMBBUV = NULL added
	Void encodeBVOPMB_WithShape (
		PixelC* ppxlcCurrQMBY, PixelC* ppxlcCurrQMBU, PixelC* ppxlcCurrQMBV, PixelC** ppxlcCurrQMBA, PixelC* ppxlcCurrQBY,
		CMBMode* pmbmd, const CMotionVector* pmv, const CMotionVector* pmvBackward, 
		CMotionVector* pmvBY, ShapeMode shpmdColocatedMB,
		const CMBMode* pmbmdRef, const CMotionVector* pmvRef,
		Int imbX, Int imbY,
		CoordI x, CoordI y,
		Int index,	//OBSS_SAIT_991015	//OBSS
		const PixelC *ppxlcCurrMBBY = NULL, const PixelC *ppxlcCurrMBBUV = NULL
	);
	// texture coding
	// HHI schueuer: sadct
	// const PixelC *ppxlcCurrMBBY = NULL, const PixelC *ppxlcCurrMBBUV = NULL added
	Void quantizeTextureIntraMB (
		Int imbX, Int imbY,
		CMBMode* pmbmd, 
		PixelC* ppxlcCurrQMBY, PixelC* ppxlcCurrQMBU, PixelC* ppxlcCurrQMBV,
		PixelC** ppxlcCurrQMBA, const PixelC *ppxlcCurrMBBY = NULL, const PixelC *ppxlcCurrMBBUV = NULL
	);
	// HHI schueuer: sadct
	// const PixelC *ppxlcCurrMBBY = NULL, const PixelC *ppxlcCurrMBBUV = NULL added
	Void quantizeTextureInterMB (CMBMode* pmbmd, const CMotionVector* pmv,
		PixelC **ppxlcCurrQMBA, Bool bSkipAllowed = TRUE,
		const PixelC *ppxlcCurrMBBY = NULL, const PixelC *ppxlcCurrMBBUV = NULL); // decide COD here
	// end HHI
	Void codeMBTextureHeadOfIVOP (CMBMode* pmbmd);
	Void codeMBTextureHeadOfPVOP (CMBMode* pmbmd, Bool *pbRestart);
//	Added for data partitioning mode By Toshiba(1998-1-16:DP+RVLC)
	Void codeMBTextureHeadOfIVOP_DP (const CMBMode* pmbmd);
	Void codeMBTextureHeadOfPVOP_DP (const CMBMode* pmbmd);
// End Toshiba(1998-1-16:DP+RVLC)
	Void codeMBTextureHeadOfBVOP (CMBMode* pmbmd);
	Void sendDCTCoefOfIntraMBTexture (const CMBMode* pmbmd);
	Void sendDCTCoefOfInterMBTexture (const CMBMode* pmbmd);

	Bool FrameFieldDCTDecideC(PixelC* m_ppxlcCurrMBY);
	Void fieldDCTtoFrameC(PixelC* ppxlcCurrQMBY);
	Bool FrameFieldDCTDecideI(PixelI* m_ppxliErrorMBY);
	Void fieldDCTtoFrameI(PixelI* m_ppxliErrorMBY);
	Void averagePredAndComputeErrorY();
	Void averagePredAndComputeErrorY_WithShape(); // new chnages
	Int interpolateAndDiffYField(
		const CMotionVector* pmvFwdTop,
		const CMotionVector* pmvFwdBot,
		const CMotionVector* pmvBakTop,
		const CMotionVector* pmvBakBot,
		CoordI x, CoordI y,
		CMBMode *pmbmd
	);
	Int directSAD(
		CoordI x, CoordI y,
		CMBMode *pmbmd,
		const CMBMode *pmbmdRef,
		const CMotionVector *pmvRef
	);
	Int directSADField(
		CoordI x, CoordI y,
		CMBMode *pmbmd,
		const CMBMode *pmbmdRef,
		const CMotionVector *pmvRef,
		const PixelC* ppxlcRef0MBY,
		const PixelC* ppxlcRef1MBY
	);
	// block level encoding
	Int quantizeIntraBlockTexture (
		PixelC* ppxlcBlkSrc, 
		Int iWidthSrc,													 
		PixelC* ppxlcCurrQBlock, 
		Int iWidthCurrQ,
		Int* rgiCoefQ, 
		Int iQP,
		Int iDcScaler,
		Int iBlk, 
		MacroBlockMemory* pmbmLeft, 
		MacroBlockMemory* pmbmTop, 
		MacroBlockMemory* pmbmLeftTop,
		MacroBlockMemory* pmbmCurr,
		CMBMode* pmbmdLeft,
		CMBMode* pmbmdTop,
		CMBMode* pmbmdLeftTop,
		CMBMode* pmbmdCurr,
		const PixelC *rgpxlcBlkShape, Int iBlkShapeWidth, // HHI Schueuer:added for sadct
    Int iAuxComp
	);	
	Void quantizeTextureInterBlock (
		PixelI* ppxliCurrQBlock, 
		Int iWidthCurrQ,
		Int* rgiCoefQ, 
		Int iQP,
		Bool bUseAlphaMatrix,
		const PixelC *rgpxlcBlkShape,
		Int iBlkShapeWidth,
		Int iBlk  // HHI Schueuer:added for sadct
	);
	Void quantizeIntraDCcoef (Int* rgiCoefQ, Float fltDcScaler, Bool bClip = TRUE);
	Void quantizeIntraDCTcoefH263 (Int* rgiCoefQ, Int iStart, Int iQP);
	Void quantizeInterDCTcoefH263 (Int* rgiCoefQ, Int iStart, Int iQP);
	Void quantizeIntraDCTcoefMPEG (Int* rgiCoefQ, Int iStart, Int iQP, Bool bUseAlphaMatrix);
	Void quantizeInterDCTcoefMPEG (Int* rgiCoefQ, Int iStart, Int iQP, Bool bUseAlphaMatrix);
	Int findVLCtableIndexOfNonLastEvent (Bool bIsLastRun, UInt uiRun, UInt uiLevel);
	Int findVLCtableIndexOfLastEvent (Bool bIsLastRun,UInt uiRun, UInt uiLevel);
	Int findVLCtableIndexOfIntra (Bool bIsLastRun, UInt uiRun, UInt uiLevel);
	UInt sendIntraDC (const Int* rgiCoefQ, Int blkn);
	UInt sendTCOEFIntra (const Int* rgiCoefQ, Int iStart, Int* rgiZigzag);
	UInt sendTCOEFInter (const Int* rgiCoefQ, Int iStart, Int* rgiZigzag);
	UInt putBitsOfTCOEFIntra (UInt uiRun, Int iLevel, Bool bIsLastRun);
	UInt putBitsOfTCOEFInter (UInt uiRun, Int iLevel, Bool bIsLastRun);
	typedef Int (CVideoObjectEncoder::*FIND_TABLE_INDEX)(Bool bIsLastRun, UInt uiRun, UInt uiLevel);	//func ptr code escp. coding
	UInt escapeEncode (UInt uiRun, Int iLevel, Bool bIsLastRun, Int* rgiLMAX, Int* rgiRMAX, FIND_TABLE_INDEX findVLCtableIndex);
	UInt fixLengthCode (UInt uiRun, Int iLevel, Bool bIsLastRun);
	Void intraPred ( 
		Int blkn, const CMBMode* pmbmd, 
		Int* rgiCoefQ, Int iQPcurr, Int iDcScaler,
		const BlockMemory pblkmPred, Int iQPpred
	);

	
	// gray-scale alpha coding
	Void quantizeAlphaInterMB (CMBMode* pmbmd);
	Void codeMBAlphaHeadOfIVOP (const CMBMode* pmbmd, Int iAuxComp);
	Void codeMBAlphaHeadOfPVOP (const CMBMode* pmbmd, Int iAuxComp);
	Void codeMBAlphaHeadOfBVOP (const CMBMode* pmbmd, Int iAuxComp);
	Void sendDCTCoefOfIntraMBAlpha (const CMBMode* pmbmd, Int iAuxComp );
	Void sendDCTCoefOfInterMBAlpha (const CMBMode* pmbmd, Int iAuxComp );

	// HHI Klaas Schueuer: SADCT
	Void deriveSADCTRowLengths(Int **piCoeffWidths, const PixelC* ppxlcMBBY, const PixelC* ppxlcCurrMBBUV, 
												const TransparentStatus *pTransparentStatus);

	// MB shape coding
	Int codeIntraShape (PixelC* ppxlcSrcFrm, CMBMode* pmbmd, Int iMBX, Int iMBY);
	Int codeInterShape (
		PixelC* ppxlcSrc, CVOPU8YUVBA* pvopcRefQ, CMBMode* pmbmd,
		const ShapeMode& shpmdColocatedMB,
		const CMotionVector* pmv, CMotionVector* pmvBY, 
		CoordI iX, CoordI iY, Int iMBX, Int IMBY
	);
//OBSS_SAIT_991015
	Bool quantizeTextureMBBackwardCheck (CMBMode* pmbmd, PixelC *ppxlcCurrMBY, PixelC *ppxlcCurrMBBY, const PixelC *ppxlcRef1MBY); // check backward skip
	Int codeSIShapePVOP (
		PixelC* ppxlcSrc, CVOPU8YUVBA* pvopcRefQ, CMBMode* pmbmd,
		const ShapeMode& shpmdColocatedMB,
		const CMotionVector* pmv, CMotionVector* pmvBY, 
		CoordI iX, CoordI iY, Int iMBX, Int IMBY
	);

	Int codeSIShapeBVOP (
		PixelC* ppxlcSrc, CVOPU8YUVBA* pvopcRefQ0, CVOPU8YUVBA* pvopcRefQ1, CMBMode* pmbmd,
		const ShapeMode& shpmdColocatedMB,
		const CMotionVector* pmv, CMotionVector* pmvBY, CMotionVector* pmvBaseBY,
		CoordI iX, CoordI iY, Int iMBX, Int IMBY
	);
	Void FullCoding(Int no_match, Int type_id_mis[256][4], SIDirection md_scan);
	Void ExclusiveORcoding(Int no_xor, Int type_id_mis[256][4], SIDirection md_scan);
	UInt codeShapeModeSSIntra (const ShapeSSMode& shpmd, const ShapeMode& shpmdColocatedMB);
	UInt codeShapeModeSSInter (const ShapeSSMode& shpmd, const ShapeMode& shpmdColocatedMB);
	UInt encodeSIIntra (ShapeSSMode shpmd, Bool* HorSamplingChk, Bool* VerSamplingChk);	
	Int motionEstMB_BVOP_WithShape (
		CoordI x, CoordI y, 
		CMotionVector* pmvForward, CMotionVector* pmvBackward,
		CMBMode* pmbmd, 
		const PixelC* ppxlcRef0MBY, const PixelC* ppxlcRef1MBY
	);

	Int sad16x16At0WithShape (const PixelC* ppxliRefY, const CMBMode* pmbmd, Int *numForePixel) const;
//~OBSS_SAIT_991015
	UInt codeShapeModeIntra (ShapeMode shpmd, const CMBMode* pmbmd, Int iMBX, Int iMBY);
	UInt codeShapeModeInter (const ShapeMode& shpmd, const ShapeMode& shpmdColocatedMB);
	ShapeMode round (PixelC* ppxlcSrcFrm, const CMBMode* pmbmd);
	Int downSampleShape (const PixelC* ppxlcSrc, Int* rgiSrcSubBlk,
						 PixelC* ppxlcDst, Int* piDstPxl, Int iRate, Int iThreshold, Int nSubBlk);
	Bool isErrorLarge (const PixelC* rgppxlcSrc, const Int* rgiSubBlkIndx, Int iWidthSrc, PixelC pxlcRecon, const CMBMode* pmbmd);
	Bool isErrorLarge (const PixelC* rgppxlcSrc, const Int* rgiSubBlkIndxSrc, const Int iSizeSrc,
					   const PixelC* rgppxlcDst, const Int* rgiSubBlkIndxDst, const Int iSizeDst, const CMBMode* pmbmd);
	UInt encodeCAEIntra (ShapeMode shpmd, CAEScanDirection m_shpdir);
	UInt encodeCAEInter (ShapeMode shpmd, CAEScanDirection m_shpdir);
	UInt encodeMVDS (CMotionVector mvBYD);
	Bool sameBlockTranspStatus (const CMBMode* pmbmd, PixelC pxlcRecon);
	Bool sameBlockTranspStatus (const CMBMode* pmbmd, const PixelC* pxlcRecon, Int iSizeRecon);
	UInt codeCrAndSt (CAEScanDirection shpdir, Int iInverseCR);
	Void copyReconShapeToRef (PixelC* ppxlcRef, PixelC pxlcSrc);		//no need to reset MB buffer
	Void copyReconShapeToRef (PixelC* ppxlcDstMB, const PixelC* ppxlcSrc, Int iSrcWidth, Int iBorder);
	Int sadForShape (const PixelC* ppxlcRefBY) const;
	Void blkmatchForShape (CVOPU8YUVBA* pvopcRefQ,CMotionVector* pmv, const CVector& mvPredHalfPel, CoordI iX, CoordI iY);
	Bool m_bNoShapeChg;
	Int *m_rgiSubBlkIndx16x16, *m_rgiSSubBlkIndx16x16, *m_rgiSubBlkIndx18x18, *m_rgiSubBlkIndx20x20;
	Int *m_rgiPxlIndx12x12, *m_rgiPxlIndx8x8;



	// motion estimation
	Void copyToCurrBuffWithShapeY (
		const PixelC* ppxlcCurrY,
		const PixelC* ppxlcCurrBY
	);
	Void copyToCurrBuffY (const PixelC* ppxlcCurrY);
	Void motionEstMB_PVOP (CoordI x, CoordI y, 
						   CMotionVector* pmv, CMBMode* pmbmd); //for spatial scalablity only
	Int motionEstMB_PVOP (
		CoordI x, CoordI y, 
		CMotionVector* pmv, CMBMode* pmbmd, 
		const PixelC* ppxliRefMBY
	);
	Int motionEstMB_PVOP_WithShape (
		CoordI x, CoordI y, 
		CMotionVector* pmv, CMBMode* pmbmd, 
		const PixelC* ppxliRefMBY
	);
	Int motionEstMB_BVOP_Interlaced (
		CoordI x, CoordI y, 
		CMotionVector* pmvForward, CMotionVector* pmvBackward,
		CMBMode* pmbmd,
		const CMBMode* pmbmdRef, const CMotionVector* pmvRef,
		const PixelC* ppxlcRef0MBY, const PixelC* ppxlcRef1MBY,
		Bool bColocatedMBExist
	);
	Int motionEstMB_BVOP (
		CoordI x, CoordI y, 
		CMotionVector* pmvForward, CMotionVector* pmvBackward,
		CMBMode* pmbmd,
		const CMBMode* pmbmdRef, const CMotionVector* pmvRef,
		const PixelC* ppxlcRef0MBY, const PixelC* ppxlcRef1MBY,
		Bool bColocatedMBExist
	);
	// for spatial scalability only
	Int CVideoObjectEncoder::motionEstMB_BVOP(
		CoordI x, CoordI y, 
		CMotionVector* pmvForward, CMotionVector* pmvBackward,
		CMBMode* pmbmd,
		const PixelC* ppxlcRef0MBY, const PixelC* ppxlcRef1MBY
	);
	Int motionEstMB_BVOP_WithShape (
		CoordI x, CoordI y, 
		CMotionVector* pmvForward, CMotionVector* pmvBackward,
		CMBMode* pmbmd, 
		const CMBMode* pmbmdRef, const CMotionVector* pmvRef,
		const PixelC* ppxlcRef0MBY, const PixelC* ppxlcRef1MBY,
		Bool bColocatedMBExist
	);
// new changes
	Int motionEstMB_BVOP_InterlacedWithShape (
		CoordI x, CoordI y, 
		CMotionVector* pmvForward, CMotionVector* pmvBackward,
		CMBMode* pmbmd, 
		const CMBMode* pmbmdRef, const CMotionVector* pmvRef,
		const PixelC* ppxlcRef0MBY, const PixelC* ppxlcRef1MBY,
		Bool bColocatedMBExist
	);
	Int blkmatch16 (
		CMotionVector* pmv, 
		CoordI iXRef, CoordI iYRef,
		CoordI iXCurr, CoordI iYCurr,
		Int iMinSAD,
		const PixelC* ppxlcRefMBY,
		const CU8Image* puciRefQZoomY,
		Int iSearchRange,
        Bool bQuarterSample // added by mwi
	);
	Int blkmatch16WithShape (
		CMotionVector* pmv, 
		CoordI iXRef, CoordI iYRef,
		CoordI iXCurr, CoordI iYCurr,
		Int iMinSAD,
		const PixelC* ppxlcRefMBY,
		const CU8Image* puciRefQZoomY,
        const CMBMode* pmbmd,
		Int iSearchRange,
        Bool bQuarterSample, // added by mwi
		Int iDirection
	);
	Int blockmatch8 (
		const PixelC* ppxlcCodedBlkY, 
		CMotionVector* pmv8, 
		CoordI x, CoordI y,
		const CMotionVector* pmvPred,
		Int iSearchRange,
        Bool bQuarterSample // added by mwi
	);
	Int blockmatch8WithShape (
		const PixelC* ppxlcCodedBlkY, 
		const PixelC* ppxlcCodedBlkBY, 
		CMotionVector* pmv8, 
		CoordI x, CoordI y,
		const CMotionVector* pmvPred,
		Int iSearchRange,
        Bool bQuarterSample, // added by mwi
		Int iDirection
	);
	Int blkmatch16x8 (
		CMotionVector* pmv, 
		CoordI iXMB, CoordI iYMB,
		Int iFeildSelect,
		const PixelC* ppxlcRefMBY,
		const PixelC* ppxlcRefHalfPel,
		Int iSearchRange,
        Bool bQuarterSample // added by mwi
	);
	// new chnages
	Int blkmatch16x8WithShape (
		CMotionVector* pmv, 
		CoordI iXMB, CoordI iYMB,
		Int iFeildSelect,
		const PixelC* ppxlcRefMBY,
		const PixelC* ppxlcRefHalfPel,
		Int iSearchRange,
        Bool bQuarterSample, // added by mwi
		Int iDirection
	);
	Int sumDev () const; // compute sum of deviation of an MB
	Int sumDevWithShape (UInt uiNumTranspPels) const; // compute sum of deviation of an MB
	Int sad16x16At0 (const PixelC* ppxliRef0Y) const;
	Int sad16x16At0WithShape (const PixelC* ppxliRefY, const CMBMode* pmbmd) const;
	Int sad8x8At0 (const PixelC* ppxlcCurrY, const PixelC* ppxliRef0Y) const; // 0: predictor
	Int sad8x8At0WithShape (const PixelC* ppxlcCurrY, const PixelC* ppxlcCurrBY, const PixelC* ppxlcRefY) const;

	// motion compensation
	Void motionCompMBYEnc (
		const CMotionVector* pmv, const CMBMode* pmbmd, 
		Int imbX, Int imbY,
		CoordI x, CoordI y,
		CRct *prctMVLimit
	);
	Void motionCompMBAEnc (
		const CMotionVector* pmv, const CMBMode* pmbmd, 
		PixelC ** pppxlcPredMBA,
		CVOPU8YUVBA* pvopcRefQ,
		CoordI x, CoordI y,
		Int iRoundingControl,
		CRct *prctMVLimit,
		Int direction, //12.22.98
    Int iAuxComp
	);
	Void motionCompMBAEncDirect ( // added by mwi
		const CMotionVector* pmv, const CMBMode* pmbmd, 
		PixelC ** pppxlcPredMBA,
		CVOPU8YUVBA* pvopcRefQ,
		CoordI x, CoordI y,
		Int iRoundingControl,
		CRct *prctMVLimit,
    Int iAuxComp
	);
	Void motionCompEncY (
		const PixelC* ppxlcRef, const PixelC* ppxlcRefZoom,
		PixelC* ppxlcPred, // can be either Y or A
		Int iSize, // either MB or BLOCK size
		const CMotionVector* pmv, // motion vector
		CoordI x, CoordI y, // current coordinate system
		CRct *prctMVLimit
	);
	Void motionCompOverLapEncY (
		const CMotionVector* pmv, // motion vector
		const CMBMode* pmbmd, // macroblk mode	
		Bool bLeftBndry, Bool bRightBndry, Bool bTopBndry,
		CoordI x, // current coordinate system
		CoordI y, // current coordinate system
		CRct *prctMVLimit
	);
	Void motionComp8Y (PixelC* ppxlcPredBlk, const CMotionVector* pmv, CoordI x, CoordI y);
	Void motionComp8A (PixelC* ppxlcPredBlk, const CMotionVector* pmv, CoordI x, CoordI y);

	// B-VOP MC
	Void motionCompAndDiff_BVOP_MB (
		const CMotionVector* pmvForward, const CMotionVector* pmvBackward, 
		CMBMode* pmbmd, 
		CoordI x, CoordI y,
		CRct *prctMVLimitForward,CRct *prctMVLimitBackward
	);
	Void motionCompAndDiff_BVOP_MB_WithShape (
		const CMotionVector* pmvForward, const CMotionVector* pmvBackward, 
		CMBMode* pmbmd, 
		CoordI x, CoordI y,
		CRct *prctMVLimitForward,CRct *prctMVLimitBackward
	);
	Void motionCompAndDiffAlpha_BVOP_MB (
		const CMotionVector* pmvForward, const CMotionVector* pmvBackward, 
		const CMBMode* pmbmd, 
		CoordI x, CoordI y,
		CRct *prctMVLimitForward,CRct *prctMVLimitBackward
	);

	Void motionCompInterpAndError (
		const CMotionVector* pmvForward, const CMotionVector* pmvBackward, 
		CoordI x, CoordI y,
		CRct *prctMVLimitForward,CRct *prctMVLimitBackward
	);
    // for QuarterPel mwi
	Void motionCompInterpAndErrorDirect (
		const CMotionVector* pmvForward, const CMotionVector* pmvBackward, 
		CoordI x, CoordI y,
		CRct *prctMVLimitForward,CRct *prctMVLimitBackward
	);
    // ~for QuarterPel mwi
	Void motionCompInterpAndError_WithShape (
		const CMotionVector* pmvForward, const CMotionVector* pmvBackward, 
		CoordI x, CoordI y,
		CRct *prctMVLimitForward,CRct *prctMVLimitBackward
	);
    // for QuarterPel mwi
    Void motionCompInterpAndError_WithShapeDirect (
		const CMotionVector* pmvForward, const CMotionVector* pmvBackward, 
		CoordI x, CoordI y,
		CRct *prctMVLimitForward,CRct *prctMVLimitBackward
	);
    // ~for QuarterPel mwi

	// error signal
	Void computeTextureErrorWithShape ();
	Void computeTextureError ();
	Void computeAlphaError ();

	// B-VOP stuff
	Int interpolateAndDiffY (
		const CMotionVector* pmvForward, const CMotionVector* pmvBackward, 
		CoordI x, CoordI y, // the coordinate of the MB
		CRct *prctMVLimitForward,CRct *prctMVLimitBackward
	);
	Int interpolateAndDiffY_WithShape (
		const CMotionVector* pmvForward, const CMotionVector* pmvBackward, 
		CoordI x, CoordI y, // the coordinate of the MB
		CRct *prctMVLimitForward,CRct *prctMVLimitBackward
	);
    // for QuarterPel mwi
	Int interpolateAndDiffY8 (
		const CMotionVector* pmvForward, const CMotionVector* pmvBackward, 
		CoordI x, CoordI y, // the coordinate of the 8x8 Block
        Int iBlkNo, // number of the 8x8 block in the current MB [1,2 / 3,4]
		CRct *prctMVLimitForward,CRct *prctMVLimitBackward
	);
    // ~for QuarterPel mwi
	Void averagePredAndComputeErrorUV ();
	Void averagePredAndComputeErrorUV_WithShape ();

	// MV
	UInt encodeMV (
		const CMotionVector* pmv, 
		const CMBMode* pmbmd,
		Bool bLeftMB, Bool bRightMB, Bool bTopMB,Int iMBX, Int iMBY); //revision 980901, mwi

	UInt encodeMVWithShape (const CMotionVector* pmv, const CMBMode* pmbmd, Int iXMB, Int iYMB);
	UInt encodeMVofBVOP (const CMotionVector* pmvForward, const CMotionVector* pmvBackward, 
						 const CMBMode*	pmbmd, Int iMBX, Int iMBY, const CMotionVector* pmvRef, 
	                     const CMBMode* pmbmdRef);  // encode motion vectors for b-vop
	UInt sendDiffMV (const CVector& vctDiffMVHalfPel, const MVInfo *pmviDir);
	Void scaleMV (Int& iVLC, UInt& uiResidual, Int iDiffMVcomponent, const MVInfo *pmviDir);
	// direct mode
	Void computeDirectDeltaMV (CVector& vctDiff, const CMotionVector* pmv, const CMotionVector* pmvRef,
        const CMBMode* pmbmdRef);	//for (progressive) B-VOP only
	
	// Statistics routines
	Void computeSNRs (const CVOPU8YUVBA* pvopcCurrQ);
	Void SNRYorA (
		const PixelC* ppxlcOrig, const PixelC* ppxlcRef1, // these two should point to the left-top of the rctOrig's bounding box 
		Double& dSNR
	);
	Void SNRUV (const CVOPU8YUVBA* pvopcCurrQ);
	Void SNRYorAWithShape (
		const PixelC* ppxlcOrig, const PixelC* ppxlcRef1, // these two should point to the left-top of the rctOrig's bounding box 
		Double& dSNR
	);
	Void SNRUVWithShape (const CVOPU8YUVBA* pvopcCurrQ);

	// Motion Vector I/O
	Int	m_iMVLineNo, m_iMVFileUsage;
	FILE *m_fMVFile;
	Char *m_pchMVFileName;
	Void readPVOPMVs(), writePVOPMVs(), readBVOPMVs(), writeBVOPMVs();

	Void decideMVInfo ();

	// Complexity Estimation syntax support - Marc Mongenet (EPFL) - 16 Jun 1998
	Int codedDCECS (Int,   // complexity estimation data to code before writing it into bitstream
					UInt); // number of bitstream bits for this data

	// error resilient tools added by Toshiba
	//	Modified for error resilient mode by Toshiba(1997-11-14)
//	Bool bVPNoLeft(Int iMBnum, Int iMBX);
//	Bool bVPNoRightTop(Int iMBnum, Int iMBX);
//	Bool bVPNoTop(Int iMBnum);
//	Bool bVPNoLeftTop(Int iMBnum, Int iMBX);
	//	End Toshiba(1997-11-14)
	Void codeVideoPacketHeader (Int iMBX, Int iMBY, Int iQuantScale);
	Int codeVideoPacketHeader (Int iQuantScale); // Added by Toshiba(1998-1-16)
	Void VideoPacketResetVOP ();//UInt nBitsModuloBase, Int iVopTimeIncr);
	UInt encodeMVVP (const CMotionVector* pmv, const CMBMode* pmbmd, Int iMBX, Int iMBY);
	//  Added for Data partitioning mode by Toshiba(1998-1-16)
	Void encodeNSForPVOP_DP ();
	Void encodeNSForIVOP_DP ();
	Void encodeNSForIVOP_WithShape_DP ();
	Void encodeNSForPVOP_WithShape_DP ();
	Void DataPartitioningMotionCoding(Int iVPMBnum, Int iVPlastMBnum, CStatistics* m_statsVP, Int*** iCoefQ_DP);
	// 09/19/99 HHI Schueuer: added  Int*** iRowLength_DP
	Void DataPartitioningTextureCoding(Int iVPMBnum, Int iVPlastMBnum, CStatistics* m_statsVP, Int*** iCoefQ_DP,  Int*** iRowLength_DP = NULL);
	// Added for RVLC by Toshiba
	UInt sendTCOEFIntraRVLC (const Int* rgiCoefQ, Int iStart, Int* rgiZigzag, Bool bDontSendBits);
	UInt putBitsOfTCOEFIntraRVLC (UInt uiRun, Int iLevel, Bool bIsLastRun, Bool bDontSendBits);
	Int findVLCtableIndexOfNonLastEventIntraRVLC (Bool bIsLastRun, UInt uiRun, UInt uiLevel);
	Int findVLCtableIndexOfLastEventIntraRVLC (Bool bIsLastRun, UInt uiRun, UInt uiLevel);
	UInt sendTCOEFInterRVLC (const Int* rgiCoefQ, Int iStart, Int* rgiZigzag, Bool bDontSendBits);
	UInt putBitsOfTCOEFInterRVLC (UInt uiRun, Int iLevel, Bool bIsLastRun, Bool bDontSendBits);
	Int findVLCtableIndexOfNonLastEventInterRVLC (Bool bIsLastRun, UInt uiRun, UInt uiLevel);
	Int findVLCtableIndexOfLastEventInterRVLC (Bool bIsLastRun, UInt uiRun, UInt uiLevel);
	UInt escapeEncodeRVLC (UInt uiRun, Int iLevel, Bool bIsLastRun, Bool bDontSendBits);
	// End Toshiba(1998-1-16)

	// begin: added by Sharp (98/2/10)
	Void setPredType(VOPpredType vopPredType);
	Void setRefSelectCode(Int refSelectCode);
	// end: added by Sharp (98/2/10)

// RRV insertion
	Void redefineVOLMembersRRV();
	Void resetAndCalcRRV();
	Void cutoffDCTcoef();
// ~RRV
};

#endif // __VOPSEENC_HPP_

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

and also edited by
	Dick van Smirren (D.vanSmirren@research.kpn.com), KPN Research
	Cor Quist (C.P.Quist@research.kpn.com), KPN Research
	(date: July, 1998)

and also edited by
    Fujitsu Laboratories Ltd. (contact: Eishi Morimatsu)

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

	vopSeDec.hpp

Abstract:

	Decoder for one Video Object.

Revision History:

	09-30-97: merge error resilient changes from Toshiba by wchen@microsoft.com
	11-30-97: added Spatial tools by Takefumi Nagumo(nagumo@av.crl.sony.co.jp) SONY corporation
	12-20-97: Interlaced tools added by NextLevel Systems
        X. Chen (xchen@nlvl.com), B. Eifrig (beifrig@nlvl.com)
	Sep.06	1999 : RRV added by Eishi Morimatsu (Fujitsu Laboratories Ltd.) 
	Feb.01	2000 : Bug fixed OBSS by Takefumi Nagumo (Sony)
	May.25  2000 : MB stuffing decoding on the last MB by Hideaki Kimata (NTT)

*************************************************************************/

#ifndef __VOPSEDEC_HPP_ 
#define __VOPSEDEC_HPP_

class CVideoObject;
class ifstream;
class strstreambuf;
class CInBitStream;
class CEntropyDecoderSet;
class CEntropyDecoder;
class CVOPU8YUVBA;
class CEnhcBufferDecoder;
class idct; // yrchen 10.21.2003

// HHI Schueuer:  scan selection classes to support the sadct
class CInvScanSelector {
public:
	virtual ~CInvScanSelector() {}

	//virtual const Int *select(const Int *scan, const Int *piCoeffWidths) { return scan; } 
	// Stefan Rauthenberg: <rauthenberg@HHI.DE> from the semantic point of view I'd prefer a return value of const Int* but 
	// this would cause conflicts with methods like decodeIntraTCOEF. 
	virtual Int *select(Int *scan, Bool bIsBoundary, Int iBlk) { return scan; } 
};

class CInvScanSelectorForSADCT: public CInvScanSelector {
public:
	CInvScanSelectorForSADCT(Int **rgiCurrMBCoeffWidth); 
	virtual ~CInvScanSelectorForSADCT();
	virtual Int *select(Int *scan, Bool bIsBoundary, Int iBlk);

private:
	Int **m_rgiCurrMBCoeffWidth;
	Int *m_adaptedScan;
};
// End HHI 


class CVideoObjectDecoder : public CVideoObject
{
	friend class CVideoObjectDecoderTPS; ///// 97/12/22
	friend class CEnhcBufferDecoder;
public:
	// Constructors
	~CVideoObjectDecoder ();
	CVideoObjectDecoder (void);
	CVideoObjectDecoder ( 
		const Char* pchStrFile, // bitstream file
		Int iDisplayWidth, Int iDisplayHeight,
		Bool *pbSpatialScalability = NULL,
		Bool *p_short_video_header=FALSE //,
		//strstreambuf* pistrm = NULL
	);
        CVideoObjectDecoder (	// for back/forward shape
		int iDisplayWidth, int iDisplayHeight
	);
       Void SetUpBitstreamBuffer(unsigned char *bptr,
                                 uint32_t blen) {
         m_pbitstrmIn->set_buffer(bptr, blen);
       };
       int get_used_bytes(void) { return m_pbitstrmIn->get_used_bytes(); };
       Void FakeOutVOVOLHead(int h, int w, int fr, Bool *pbSpatialScalability);
       void postVO_VOLHeadInit(Int iDisplayWidth, Int iDisplayHeight,Bool *pbSpatialScalability);
       // Operations
       Int decode (const CVOPU8YUVBA* pvopcBVOPQuant = NULL, /* strstreambuf* pistrm = NULL, */ Bool waitForI = FALSE, Bool drop = FALSE);
	Int ReadNextVopPredType ();  //for Spatial Scalable Coding
	Int h263_decode(bool first_time); // [FDS]

	Void decodeInitSprite ();
	Void decodeSpritePieces ();
	Int decodeOneSpritePiece ();
	CRct decodeVOSHead ();
	SptXmitMode m_oldSptXmitMode ;

///////////////// implementation /////////////////
///// 97/12/22 start
public:
	CVideoObjectDecoder* rgpbfShape [2]; // 0 : backward, 1: forward
///// 97/12/22 end

// Added by KPN for short headers
	UInt video_plane_with_short_header();
	void gob_layer();
	void decodeShortHeaderIntraMBDC(Int *rgiCoefQ);
	Bool checkGOBMarker(); // added by swinder
	Void skipAnyStuffing(); // added by swinder
//Added by KPN for short headers - END


//protected:
public:
	own int m_pistrm;
	CInBitStream* m_pbitstrmIn;		//bitstream
	own CEntropyDecoderSet* m_pentrdecSet;	//huffman decoder set
							  //used for video packet header decoding by Toshiba
							  //they are basically cached values of not update after the header decoding
	Time m_tOldModuloBaseDisp;			  //of the most recently displayed I/Pvop
	Time m_tOldModuloBaseDecd;			  //of the most recently decoded I/Pvop
	// buffer data
	CVOPU8YUVBA* m_pvopcRightMB;
	PixelC *m_ppxlcRightMBBY, *m_ppxlcRightMBBUV;

	// buffers for temporal scalability	Added by Sharp(1998-02-10)
	CEnhcBufferDecoder* m_pBuffP1;
	CEnhcBufferDecoder* m_pBuffP2;
	CEnhcBufferDecoder* m_pBuffB1;
	CEnhcBufferDecoder* m_pBuffB2;
	CEnhcBufferDecoder* m_pBuffE;
	int m_coded;
	// buffers for temporal scalability	End	 Sharp(1998-02-10)

	// Added for short headers by KPN (1998-02-07, DS)
	Bool short_video_header;
	UInt uiPei; // [FDS]
	UInt uiGobNumber;
	UInt uiNumGobsInVop;
	UInt uiNumMacroblocksInGob;
	UInt uiGobHeaderEmpty;

	// Added for short headers by KPN - END
	int m_iClockRateScale; // added by Sharp (98/6/26)

 	//	Added for error resilience mode By Toshiba(1998-1-16:DP+RVLC)
 	Int* m_piMCBPC;
 	Int* m_piIntraDC;
 	//	End Toshiba(1998-1-16:DP+RVLC)

	// HHI Schueuer
	CInvScanSelector *m_pscanSelector;
	// end HHI

	// error handling
	Void errorInBitstream (Char* rgchErorrMsg);

	Int findStartCode(int dontloop = 0);

	Int decodeVSHead (); // visual sequence, visual object layers

	// VO and VOL routines
	Void decodeVOHead ();
	Void decodeVOLHead ();
  Void decodeVOLBody();

	// VOP routines
	Bool decodeVOPHead ();
	Void decodeVOP ();
	Void decodeIVOP ();
	Void decodePVOP ();
	Void decodeBVOP ();
	Void decodeIVOP_WithShape ();
	Void decodePVOP_WithShape ();
	Void decodeBVOP_WithShape ();
//	Void computeMVInfo ();
 	//	Added for error resilience mode By Toshiba(1998-1-16:DP+RVLC)
 	Void decodeIVOP_DataPartitioning ();
 	Void decodePVOP_DataPartitioning ();
	Void decodeIVOP_WithShape_DataPartitioning ();
	Void decodePVOP_WithShape_DataPartitioning ();
 	//	End Toshiba(1998-1-16:DP+RVLC)

	// MB routines	: texture and overhead
	// I-VOP
	Void decodeMBTextureHeadOfIVOP (CMBMode* pmbmd, Int& iCurrQP, Bool *pbUseNewQPForVlcThr);
	Void decodeTextureIntraMB (CMBMode* pmbmd, CoordI iMBX, CoordI iMBY, 
			PixelC* ppxlcCurrFrmQY, PixelC* ppxlcCurrFrmQU, PixelC* ppxlcCurrFrmQV,
			const PixelC *ppxlcCurrMBBY = NULL, const PixelC *ppxlcCurrMBBUV = NULL); // HHI Schueuer
	Void decodeMBAlphaHeadOfIVOP (CMBMode* pmbmd, Int iCurrQP, Int iCurrQPA, Int iVopQP, Int iVopQPA, Int iAuxComp);
	Void decodeAlphaIntraMB (CMBMode* pmbmd, Int iMBX, Int iMBY, PixelC* ppxlcRefMBA, Int iAuxComp, const PixelC *ppxlcCurrMBBY = NULL); // HHI Schueuer: const PixelC *ppxlcCurrMBBY added		
	
	// P-VOP
	Void decodeMBTextureHeadOfPVOP (CMBMode* pmbmd, Int& iCurrQP, Bool *pbUseNewQPForVlcThr);
	Void decodeMBAlphaHeadOfPVOP (CMBMode* pmbmd, Int iCurrQP, Int iCurrQPA, Int iAuxComp);
	Void decodeTextureInterMB (CMBMode* pmbmd, const PixelC *ppxlcCurrMBBY = NULL, const PixelC *ppxlcCurrMBBUV = NULL);//HHI Schueuer:const PixelC *ppxlcCurrMBBY, const PixelC *ppxlcCurrMBBUV
	Void decodeAlphaInterMB (CMBMode* pmbmd, PixelC *ppxlcRefMBA, Int iAuxComp, const PixelC *ppxlcCurrMBBY = NULL); // HHI Schueuer: const PixelC *ppxlcCurrMBBY added	
	
	// B-VOP
	Void decodeMBTextureHeadOfBVOP (CMBMode* pmbmd, Int& iCurrQP);
	Void decodeMBAlphaHeadOfBVOP (CMBMode* pmbmd, Int iCurrQP, Int iCurrQPA, Int iAuxComp);
	
	Void setCBPYandC (CMBMode* pmbmd, Int iCBPC, Int iCBPY, Int cNonTrnspBlk);

	// motion compensation
	Void motionCompTexture (
		const CMotionVector* pmv, const CMBMode* pmbmd, 
		Int imbX, Int imbY,
		CoordI x, CoordI y
	);
	Void motionCompAlpha (
		const CMotionVector* pmv, const CMBMode* pmbmd, 
		Int imbX, Int imbY,
		CoordI x, CoordI y,
    Int iAuxComp
	);
	// B-VOP
	Void motionCompAndAddErrorMB_BVOP (
		const CMotionVector* pmvForward, const CMotionVector* pmvBackward,
		 CMBMode* pmbmd,		// new change 02-19-99
		Int iMBX, Int iMBY, 
		CoordI x, CoordI y,
		PixelC* ppxlcCurrQMBY, PixelC* ppxlcCurrQMBU, PixelC* ppxlcCurrQMBV,
		CRct *prctMVLimitForward,CRct *prctMVLimitBackward
	); 
	Void motionCompAlphaMB_BVOP(
		const CMotionVector* pmvForward, const CMotionVector* pmvBackward,
		 CMBMode* pmbmd,		// new change 02-19-99
		Int iMBX, Int iMBY, 
		CoordI x, CoordI y,
		PixelC* ppxlcCurrQMBA,
		CRct *prctMVLimitForward,CRct *prctMVLimitBackward,
    Int iAuxComp
	);
	Void copyFromRefToCurrQ_BVOP (
		const CMBMode* pmbmd,
		CoordI x, CoordI y, 
		PixelC* ppxlcCurrQMBY, PixelC* ppxlcCurrQMBU, PixelC* ppxlcCurrQMBV,
		CRct *prctMVLimitForward, CRct *prctMVLimitBackward
	);
	Void motionCompSkipMB_BVOP (
		const CMBMode* pmbmd, const CMotionVector* pmvForward, const CMotionVector* pmvBackward,
		CoordI x, CoordI y, 
		PixelC* ppxlcCurrQMBY, PixelC* ppxlcCurrQMBU, PixelC* ppxlcCurrQMBV,
		CRct *prctMVLimitForward,CRct *prctMVLimitBackward
	);

	Void copyFromPredForYAndRefForCToCurrQ (
		CoordI x, CoordI y, 
		PixelC* ppxlcCurrQMBY, PixelC* ppxlcCurrQMBU, PixelC* ppxlcCurrQMBV,
		CRct *prctMVLimit
	); // for obmc mode
	Void copyAlphaFromPredToCurrQ (
		CoordI x, CoordI y, 
		PixelC* ppxlcCurrQMBA,
    Int iAuxComp
	);

	// error signal for B-VOP MB
	Void averagePredAndAddErrorToCurrQ (
		PixelC* ppxlcCurrQMBY, PixelC* ppxlcCurrQMBU, PixelC* ppxlcCurrQMBV
	);
	Void averagePredAndAssignToCurrQ (
		PixelC* ppxlcCurrQMBY, PixelC* ppxlcCurrQMBU, PixelC* ppxlcCurrQMBV
	);

	// motion vector
	Void decodeMV (const CMBMode* pmbmd, CMotionVector* pmv, Bool bLeftBndry, Bool bRightBndry, Bool bTopBndry, Bool bZeroMV, Int iMBX, Int iMBY);
	Void decodeMVWithShape (const CMBMode* pmbmd, CoordI iMBX, CoordI iMBY, CMotionVector* pmv);
	Void getDiffMV (CVector& vctDiffMV, MVInfo mvinfo);		//get half pel
	Int  deScaleMV (Int iVLC, Int iResidual, Int iScaleFactor);
	Void fitMvInRange (CVector& vctSrc, MVInfo mvinfo);
	Void decodeMVofBVOP (CMotionVector* pmvForward, CMotionVector* pmvBackward, CMBMode* pmbmd, 
						 Int iMBX, Int iMBY, const CMotionVector* pmvRef, const CMBMode* pmbmdRef);
	Void computeDirectForwardMV (CVector vctDiff, CMotionVector* pmv, const CMotionVector* pmvRef, const CMBMode* pmbmdRef);

	// shape decoding functions
	Void swapCurrAndRightMBForShape ();
	Void decodeIntraShape (CMBMode* pmbmd, Int iMBX, Int iMBY, PixelC* ppxlcCurrMBBY, PixelC* ppxlcCurrMBBYFrm);
	Void decodeInterShape (CVOPU8YUVBA* pvopcRefQ,CMBMode* pmbmd, CoordI iMBX, CoordI iMBY, CoordI iX, CoordI iY, CMotionVector* pmv, CMotionVector* pmvBY, 
						   PixelC* ppxlcMBBY, PixelC* ppxlcMBBYFrm, const ShapeMode& shpmdColocatedMB);
	Int  shpMdTableIndex (const CMBMode* pmbmd, Int iMBX, Int iMBY);
	Void decodeIntraCaeBAB (PixelC* ppxlcCurrMBBY, PixelC* ppxlcCurrMBBYFrm);
	Void decodeIntraCAEH ();
	Void decodeIntraCAEV ();
	Void decodeInterCAEH (const PixelC *);
	Void decodeInterCAEV (const PixelC *);
//OBSS_SAIT_991015
	Void decodeSIShapePVOP (CVOPU8YUVBA* pvopcRefQ, CMBMode* pmbmd, CoordI iMBX, CoordI iMBY, CoordI iX, CoordI iY, CMotionVector* pmv, CMotionVector* pmvBY, CMotionVector* pmvBaseBY, PixelC* ppxlcMBBY, PixelC* ppxlcMBBYFrm, const ShapeMode& shpmdColocatedMB);
	Void decodeSIShapeBVOP (CVOPU8YUVBA* pvopcRefQ0, CVOPU8YUVBA* pvopcRefQ1, CMBMode* pmbmd, CoordI iMBX, CoordI iMBY, CoordI iX, CoordI iY, CMotionVector* pmv, CMotionVector* pmvBY, CMotionVector* pmvBaseBY, PixelC* ppxlcMBBY, PixelC* ppxlcMBBYFrm, const ShapeMode& shpmdColocatedMB);
	Void decodeSIBAB (PixelC* ppxlcCurrMBBY, PixelC* ppxlcCurrMBBYFrm, Bool* HorSamplingChk, Bool* VerSamplingChk,Int iMBX,Int iMBY, PixelC* ppxlcRefSrc);
	Void VerticalXORdecoding(Int num_loop_hor, Int num_loop_ver, Bool residual_scanning_hor, Bool residual_scanning_ver, Bool* HorSamplingChk, Bool* VerSamplingChk);		
	Void VerticalFulldecoding(Int num_loop_hor, Int num_loop_ver, Bool residual_scanning_hor, Bool residual_scanning_ver, Bool* HorSamplingChk, Bool* VerSamplingChk);		
	Void HorizontalXORdecoding(Int num_loop_hor, Int num_loop_ver, Bool residual_scanning_hor, Bool residual_scanning_ver, Bool* HorSamplingChk, Bool* VerSamplingChk);		
	Void HorizontalFulldecoding(Int num_loop_hor, Int num_loop_ver, Bool residual_scanning_hor, Bool residual_scanning_ver, Bool* HorSamplingChk, Bool* VerSamplingChk);	
//~OBSS_SAIT_991015
	Void decodeMVDS (CMotionVector& mvDiff);

	//for temporal scalability	Added by Sharp(1998-02-10)
	void updateBuffVOPsBase (CVideoObjectDecoder* pvodecEnhc);
	void updateRefVOPsEnhc ();
	void updateBuffVOPsEnhc ();
	void copyRefQ1ToQ0 ();
	void copyTobfShape ();
	Void decode_IShape (); // for back/forward shape
//	Void compute_bfShapeMembers ();
	Void BackgroundComposition(char* argv[], Bool bScalability, Int width, Int height, FILE *pfYUV); // for background composition // modified by Sharp (98/10/26)
//OBSS_SAIT_991015		//for OBSS partial enhancement mode
//OBSSFIX_MODE3
	Bool BackgroundCompositionSS(Int width, Int height, FILE *pfYUV, FILE *pfSeg, const CVOPU8YUVBA *pvopcRefBaseLayer) ;
//	Int  BackgroundComposition  (Int width, Int height, FILE *pfYUV, FILE *pfSeg); // for OBSS background composition (base + patial enhancement layer)
//~OBSSFIX_MODE3
	Void dumpDataOneFrame(char* argv[], Bool bScalability, CRct& rctDisplay); // write pvopcReconCurr () with Sharp's format
	Void dumpDataAllFrame(FILE* pfReconYUV, FILE* pfReconSeg, CRct& rctDisplay);
	// I/O
	Time senseTime ();
	Time getTime ();
	int getPredType ();
	// bufferControl
	void set_enh_display_size(CRct rctDisplay, CRct *rctDisplay_SSenh);
	void bufferP1flush ();
	void bufferB1flush ();
	void bufferB2flush ();
	void copyBufP2ToB1 ();
	//for temporal scalability	End	 Sharp(1998-02-10)
	void setClockRateScale ( CVideoObjectDecoder *pvopdec ); // added by Sharp (98/6/26)

	// INTERLACE
	Void fieldDCTtoFrameC(PixelC* ppxlcRefMBY);
	Void fieldDCTtoFrameI(PixelI* ppxlcCurrMBY);
	// ~INTERLACE

	// HHI Schueuer
	Void deriveSADCTRowLengths(Int **coeffWidths, const PixelC* ppxlcMBBY, const PixelC* ppxlcCurrMBBUV, const TransparentStatus *pTransparentStatus);
	// end HHI

	// Block decoding functions
	Void decodeIntraBlockTexture (PixelC* rgchBlkDst,
								 Int iWidthDst,
								 Int iQP, 
								 Int iDcScaler,
								 Bool bIsYBlock,
								 MacroBlockMemory* pmbmCurr,
								 CMBMode* pmbmd,
 								 const BlockMemory blkmPred,
								 Int iQPPred,
								 const PixelC *rgpxlcBlkShape,
                 Int iBlkShapeWidth, // HHI Schueuer: added const PixelC *rgpxlcBlkShape,Int iBlkShapeWidth
                 Int iAuxComp=0 );
	Void decideIntraPred (const BlockMemory& blkmRet, 
						   CMBMode* pmbmdCurr,
						   Int&	iQPpred,
						   Int blkn,									   
						   const MacroBlockMemory* pmbmLeft, 
  						   const MacroBlockMemory* pmbmTop, 
						   const MacroBlockMemory* pmbmLeftTop,
						   const MacroBlockMemory* pmbmCurr,
						   const CMBMode* pmbmdLeft,
						   const CMBMode* pmbmdTop,
						   const CMBMode* pmbmdLeftTop);
	// HHI Schueuer: const CMBMode *pmbmd, Int iBlk, const PixelC *rgpxlcBlkShape, Int iBlkShapeWidth added
	Void decodeTextureInterBlock (Int* rgiBlkCurrQ, Int iWidthCurrQ, Int iQP, Bool bAlphaBlock, const CMBMode *pmbmd, Int iBlk, const PixelC *rgpxlcBlkShape, Int iBlkShapeWidth, Int iAuxComp=0);
	Void decodeIntraTCOEF (Int* rgiCoefQ, Int iCoefStart, Int* rgiZigzag);
	Void decodeInterTCOEF (Int* rgiCoefQ, Int iCoefStart, Int* rgiZigzag);
	Int decodeIntraDCmpeg (Bool bIsYBlk);					//decode the intra-dc: mpeg2 method
	Void decodeInterVLCtableIndex (Int iIndex, Int&  iLevel, Int&  iRun, Bool& bIsLastRun);  // return islastrun, run and level
	Void decodeIntraVLCtableIndex (Int iIndex, Int&  iLevel, Int&  iRun, Bool& bIsLastRun);  // return islastrun, run and level
	typedef Void (CVideoObjectDecoder::*DECODE_TABLE_INDEX)(Int iIndex, Bool& bIsLastRun, Int&  iRun, Int&  iLevel);	//func ptr code escp. decoding
	Void decodeEscape (Int& iLevel, Int& iRun, Int& bIsLastRun, const Int* rgiLMAX, const Int* rgiRMAX, 
					   CEntropyDecoder* pentrdec, DECODE_TABLE_INDEX decodeVLCtableIndex);
	Void inverseDCACPred (const CMBMode* pmbmd, Int iBlkIdx, Int* rgiCoefQ, Int iQP, Int iDcScaler, const BlockMemory blkmPred, Int iQpPred);

	// sprite decoding
	Int decodeSpt ();
	Void decodeWarpPoints ();

	// added for error resilience mode By Toshiba
	Int	checkResyncMarker();
	Bool	decodeVideoPacketHeader(Int& iCurrentQP);

 	// added for error resilience mode(DP+RVLC) By Toshiba(1998-1-16:DP+RVLC)
 	Int	checkMotionMarker();
 	Int	checkDCMarker();
 	// I-VOP
 	Void decodeMBTextureDCOfIVOP_DataPartitioning (CMBMode* pmbmd, Int& iCurrentQP,
		Int* piIntraDC, Bool *pbUseNewQPForVlcThr);
 	Void decodeMBTextureHeadOfIVOP_DataPartitioning (CMBMode* pmbmd, Int* piMCBPC);
// 09/16/99 HHI schueuer: added const PixelC* m_ppxlcCurrMBBY = NULL, const PixelC* m_ppxlcCurrMBBUV = NULL
 	Void decodeTextureIntraMB_DataPartitioning (CMBMode* pmbmd, CoordI iMBX, CoordI iMBY,
 			PixelC* ppxlcCurrFrmQY, PixelC* ppxlcCurrFrmQU, PixelC* ppxlcCurrFrmQV, Int* piIntraDC,
			const PixelC* m_ppxlcCurrMBBY = NULL, const PixelC* m_ppxlcCurrMBBUV = NULL);
 	
 	// P-VOP
	Bool decodeMBTextureModeOfPVOP_DataPartitioning (CMBMode* pmbmd, Int* piMCBPC);
 	Void decodeMBTextureHeadOfPVOP_DataPartitioning (CMBMode* pmbmd, Int& iCurrentQP, Int* piMCBPC,
		Int* piIntraDC, Bool *pbUseNewQPForVlcThr);
 	Void decodeTextureInterMB_DataPartitioning (CMBMode* pmbmd);
 	
 	// B-VOP
 	Void decodeMBTextureHeadOfBVOP_DataPartitioning (CMBMode* pmbmd, Int& iCurrQP);
 	
 	Void setCBPYandC_DataPartitioning (CMBMode* pmbmd, Int iCBPC, Int iCBPY, Int cNonTrnspBlk);
 
 	Void decodeIntraBlockTexture_DataPartitioning (Int iBlk, CMBMode* pmbmd, Int* piINtraDC);
	// HHI Schueuer: added const PixelC *rgpxlcBlkShape,Int iBlkShapeWidth for sadct
	Void decodeIntraBlockTextureTcoef_DataPartitioning (PixelC* rgchBlkDst,
 								 Int iWidthDst,
 								 Int iQP, 
 								 Int iDcScaler,
 								 Bool bIsYBlock,
 								 MacroBlockMemory* pmbmCurr,
 								 CMBMode* pmbmd,
 								 const BlockMemory blkmPred,
 								 Int iQPPred,
 								 Int* piIntraDC,
								 const PixelC *rgpxlcBlkShape=NULL,
								 Int iBlkShapeWidth=0);
 
 	Void decodeIntraRVLCTCOEF (Int* rgiCoefQ, Int iCoefStart, Int* rgiZigzag);
 	Void decodeInterRVLCTCOEF (Int* rgiCoefQ, Int iCoefStart, Int* rgiZigzag);
 	Void decodeInterRVLCtableIndex (Int iIndex, Int&  iLevel, Int&  iRun, Int& bIsLastRun);  // return islastrun, run and level
 	Void decodeIntraRVLCtableIndex (Int iIndex, Int&  iLevel, Int&  iRun, Bool& bIsLastRun);  // return islastrun, run and level
 	Void decodeRVLCEscape (Int& iLevel, Int& iRun, Int& bIsLastRun, const Int* rgiLMAX, const Int* rgiRMAX, 
 					   CEntropyDecoder* pentrdec, DECODE_TABLE_INDEX decodeVLCtableIndex);
 	//	End Toshiba(DP+RVLC)(1998-1-16:DP+RVLC)

// RRV insertion
	Void redefineVOLMembersRRV();
// ~RRV
// May.25 2000 for MB stuffing decoding on the last MB
	Int	checkStartCode();
// ~May.25 2000 for MB stuffing decoding on the last MB
  // yrchen 10.21.2003
  idct *m_pinvdct;
};

#endif // __VOPSEDEC_HPP_

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

and edited by Xuemin Chen (General Instrument Corp.)

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

	vopSes.hpp

Abstract:

	Base class for the encoder for one Video Object.

Revision History:
    December 20, 1997   Interlaced tools added by General Instrument Corp.(GI)
                        X. Chen (xchen@gi.com), B. Eifrig (beifrig@gi.com)
        May. 9   1998:  add boundary by Hyundai Electronics 
                                  Cheol-Soo Park (cspark@super5.hyundai.co.kr) 
        May. 9   1998:  add field based MC padding by Hyundai Electronics 
                                  Cheol-Soo Park (cspark@super5.hyundai.co.kr) 
*************************************************************************/

#ifndef __VOPSES_HPP_ 
#define __VOPSES_HPP_

typedef enum {NOT_DONE, PIECE_DONE, UPDATE_DONE} SptMBstatus;

#define BlockMemory Int*
typedef struct MacroBlockMemory {
	BlockMemory* rgblkm;
} MacroBlockMemory;

Class ArCodec;

Class CInvBlockDCT;

Class CVideoObject
{
public:
	// Constructors
	virtual ~CVideoObject ();
	CVideoObject ();

	// Attributes
	const CVOPU8YUVBA* pvopcQuantCurr () const {return m_pvopcCurrQ;}
	const CVOPU8YUVBA* pvopcRefQPrev () const {return m_pvopcRefQ0;}
	const CVOPU8YUVBA* pvopcRefQLater () const {return m_pvopcRefQ1;}
	//This modification is for reconstructed BVOP image in SS coding should be used as refrence layer 
	const CVOPU8YUVBA* pvopcReconCurr () const; 
	//						{return   ((m_vopmd.vopPredType == BVOP && m_volmd.volType == BASE_LAYER) 
	//								 ||(m_vopmd.vopPredType == BVOP && m_vopmd.iRefSelectCode != 0)
	//								 ||(m_vopmd.vopPredType == SPRITE && m_volmd.fAUsage != RECTANGLE)) 
	//								 ? m_pvopcCurrQ : m_pvopcRefQ1;}	//reconstructed current frame
	const VOLMode& volmd () const {return m_volmd;}
	const VOPMode& vopmd () const {return m_vopmd;}
	UInt fSptUsage () const {return m_uiSprite;}

	// Operations
	virtual Void encode () {}
	virtual Int decode () {return 0;}

	Void swapRefQ1toSpt ();
	Void changeSizeofCurrQ (CRct rctOrg);
	Void setRctDisp (CRct rctDisp) {m_rctSptDisp = rctDisp;}

	Int iNumOfPnts () const {return m_iNumOfPnts;}
	const CVOPU8YUVBA* pvopcSptQ () const {return m_pvopcSptQ;}	  

	//low latency sprite stuff
	Void Overlay (CU8Image& uci1, CU8Image& uci2, float fscale); //1 on 2
	Void VOPOverlay (CVOPU8YUVBA& pvopc1, CVOPU8YUVBA& pvopc2, Int iscale = 0); //1 on 2

// dshu: begin of modification			
	Void U8iGet (CU8Image& uci1, CU8Image& uci2, CRct rctPieceY); 
	Void PieceGet (CVOPU8YUVBA& pvopc1, CVOPU8YUVBA& pvopc2, CRct rctPieceY);			
	Void U8iPut (CU8Image& uci1, CU8Image& uci2, CRct rctPieceY); 
	Void PiecePut (CVOPU8YUVBA& pvopc1, CVOPU8YUVBA& pvopc2, CRct rctPieceY); 
// dshu: end of modification

// dshu: [v071]  Begin of modification
	Void padSprite();
	Void copySptQShapeYToMb (PixelC* ppxlcDstMB, const PixelC* ppxlcSrc)	;
// dshu: [v071] end of modification

	Bool SptPieceMB_NOT_HOLE(Int iMBXoffset, Int iMBYoffset, CMBMode* pmbmd); 
	Bool SptUpdateMB_NOT_HOLE(Int iMBXoffset, Int iMBYoffset, CMBMode* pmbmd);	
	Void SaveMBmCurrRow (Int iMBYoffset, MacroBlockMemory** rgpmbmCurr);
	Void RestoreMBmCurrRow (Int iMBYoffset, MacroBlockMemory** rgpmbmCurr);
	Void CopyCurrQToPred (PixelC* ppxlcQMBY, PixelC* ppxlcQMBU,	PixelC* ppxlcQMBV);
///// 97/12/22 start
	Void copyVOPU8YUVBA (CVOPU8YUVBA*& pvopc0, CVOPU8YUVBA*& pvopc1);
	Void copyVOPU8YUVBA (CVOPU8YUVBA*& pvopc0, CVOPU8YUVBA*& pvopc1, CVOPU8YUVBA*& pvopc2);
	Void compute_bfShapeMembers ();
///// 97/12/22 end
///////////////// implementation /////////////////
	Int getWidth(void) { return m_ivolWidth;};
	Int getHeight(void) { return m_ivolHeight;};
	double getFrameTime(void) { return (m_t / (double)m_volmd.iClockRate); };
	Int getClockRate(void) { return m_volmd.iClockRate; };
protected:
	// general routines
	Void swapVOPU8Pointers (CVOPU8YUVBA*& pvopc0, CVOPU8YUVBA*& pvopc1);
	Void swapVOPIntPointers (CVOPIntYUVBA*& pvopi0, CVOPIntYUVBA*& pvopi1);

	// VOL routines
	Void allocateVOLMembers (Int iSessionWidth, Int iSessionHeight);
	Void computeVOLConstMembers ();

	// VOP routines
	Void computeVOPMembers ();
	Void setRefStartingPointers ();
	Void updateAllRefVOPs ();
	Void updateAllRefVOPs (const CVOPU8YUVBA *pvopcRefBaselayer); // for spatial scalability
	Void repeatPadYOrA (PixelC* ppxlcOldLeft, CVOPU8YUVBA* pvopcRef);
	Void repeatPadUV (CVOPU8YUVBA* pvopcRef);
	Void resetBYPlane ();

	// MB routines
	Void decideTransparencyStatus (CMBMode* pmbmd, const PixelC* ppxlcMBBY);
	Void decideMBTransparencyStatus (CMBMode* pmbmd);
	Void downSampleBY (const PixelC* ppxlcMBBY, PixelC* ppxlcMBBUV); // downsample original BY now for LPE padding (using original shape)
	
	// Added for field based MC padding by Hyundai(1998-5-9)
        Void decideFieldTransparencyStatus (
                CMBMode* pmbmd,
                const PixelC* ppxlcMBBY,
                const PixelC* ppxlcMBBUV
        );
        Void fieldBasedDownSampleBY (
                const PixelC* ppxlcMBBY,
                PixelC* ppxlcMBBUV
        );
	// End of Hyundai(1998-5-9)

	// error signal processing
	Void addErrorAndPredToCurrQ (PixelC* ppxlcRefMBY, PixelC* ppxlcRefMBU, PixelC* ppxlcRefMBV);
	Void addAlphaErrorAndPredToCurrQ (PixelC* ppxlcRefMBA);

	// shape coding functions
	Void saveShapeMode();
	own Int* computeShapeSubBlkIndex (Int iSubBlkSize, Int iSrcSize);
	Int getContextUS (PixelC a, PixelC b, PixelC c, PixelC d, PixelC e, PixelC f, PixelC g, PixelC h);
	PixelC getRefValue (const PixelC* ppxlcRow, Int x_adr, Int y_adr, Int h_size, Int v_size);
	Void adaptiveUpSampleShape (const PixelC* rgpxlcSrc, PixelC* rgpxlcDst, Int h_size, Int v_size);
	Void downSampleShapeMCPred(const PixelC* ppxlcSrc,PixelC* ppxlcDst,Int iRate);
	Void upSampleShape (PixelC* ppxlcBYFrm,const PixelC* rgpxlcSrc, PixelC* rgpxlcDst);
	Void copyLeftTopBorderFromVOP (PixelC* ppxlcSrc, PixelC* ppxlcDst);
	Void subsampleLeftTopBorderFromVOP (PixelC* ppxlcSrc, PixelC* ppxlcDst);
	Void makeRightBottomBorder (PixelC* ppxlcSrc, Int iWidth);
	Int contextIntra (const PixelC* ppxlcSrc);
	Int contextIntraTranspose (const PixelC* ppxlcSrc);
	Int contextInter (const PixelC* ppxlcSrc, const PixelC* ppxlcPred);
	Int contextInterTranspose (const PixelC* ppxlcSrc, const PixelC* ppxlcPred);
	Void copyReconShapeToMbAndRef (PixelC* ppxlcDstMB, PixelC* ppxlcRefFrm, PixelC pxlcSrc);
	Void copyReconShapeToMbAndRef (PixelC* ppxlcDstMB, PixelC* ppxlcRefFrm, const PixelC* ppxlcSrc, Int iSrcWidth, Int iBorder);
	Void copyRefShapeToMb (PixelC* ppxlcDstMB, const PixelC* ppxlcSrc); //	Added for data partitioning mode By Toshiba(1998-1-16:DP+RVLC)
	Void copyReconShapeUVToRef (PixelC* ppxlcRefFrm, const PixelC* ppxlcSrc);
	CMotionVector findShapeMVP (const CMotionVector* pmv, const CMotionVector* pmvBY, 
								const CMBMode* pmbmd, Int iMBX, Int iMBY) const;
	Int m_iInverseCR;

	// B-VOP stuff
	Void findColocatedMB (Int iMBX, Int iMBY, const CMBMode*& pmbmdRef, const CMotionVector*& pmvRef);
	
	// motion compensation
	Void limitMVRangeToExtendedBBHalfPel (CoordI &x,CoordI &y,CRct *prct,Int iBlkSize);
	Void limitMVRangeToExtendedBBFullPel (CoordI &x,CoordI &y,CRct *prct,Int iBlkSize);
	Void motionCompMB (
		PixelC* ppxlcPredMB,
		const PixelC* ppxlcRefLeftTop,
		const CMotionVector* pmv, const CMBMode* pmbmd, 
		Int imbX, Int imbY,
		CoordI x, CoordI y,
		Bool bSkipNonOBMC,
		Bool bAlphaMB,
		CRct *prctMVLimit
	);
	Void motionComp (
		PixelC* ppxlcPred, // can be either Y or A
		const PixelC* ppxlcRefLeftTop,
		Int iSize, // either MB or BLOCK size
		CoordI xRef, CoordI yRef, // x + mvX, in half pel unit
		Int iRoundingControl,
		CRct *prctMVLimit
	);
	Void motionCompUV (
		PixelC* ppxlcPredMBU, PixelC* ppxlcPredMBV,
		const CVOPU8YUVBA* pvopcRef,
		CoordI x, CoordI y, 
		CoordI xRefUV, CoordI yRefUV, Int iRoundingControl,
		CRct *prctMVLimit
	);

// INTERLACE
	Void motionCompFieldUV (PixelC* ppxlcPredMBU, PixelC* ppxlcPredMBV,
							const CVOPU8YUVBA* pvopcRef,CoordI x, CoordI y, 
							CoordI xRefUV, CoordI yRefUV, Int iRefFieldSelect);
	Void motionCompYField ( PixelC* ppxlcPred, const PixelC* ppxlcRefLeftTop,
							CoordI xRef, CoordI yRef);	
	// new change 02-19-99
	Void motionCompDirectMode(CoordI x, CoordI y,  CMBMode *pmbmd, const CMotionVector *pmvRef,
                            CRct *prctMVLimitFwd, CRct *prctMVLimitBak, Int plane);
	Void motionCompOneBVOPReference(CVOPU8YUVBA *pvopcPred, MBType type, CoordI x, CoordI y,
                            const CMBMode *pmbmd, const CMotionVector *pmv, CRct *prctMVLimit);
// ~INTERLACE

	Void motionCompOverLap (
		PixelC* ppxlcPredMB, 
		const PixelC* ppxlcRefLeftTop,
		const CMotionVector* pmv, // motion vector
		const CMBMode* pmbmd, // macroblk mode	
		Int imbx, // current macroblk index
		Int imby, // current macroblk index
		CoordI x, // current coordinate system
		CoordI y, // current coordinate system
		CRct *prctMVLimit
	);
	Void motionCompBY (
		PixelC* ppxlcPred,
		const PixelC* ppxlcRefLeftTop,
		CoordI iXRef, CoordI iYRef
	);

	// B-VOP stuff
	Void motionCompInterp (
		const CMotionVector* pmvForward, const CMotionVector* pmvBackward, 
		CoordI x, CoordI y
	);

	Void bilnrMCVH (UInt* PredY, const PixelC* ppxliPrevYC, UInt* pMWght, UInt xlow, UInt xhigh, UInt ylow, UInt yhigh, Bool bAdd);
	Void bilnrMCV (UInt* PredY, const PixelC* ppxliPrevYC, UInt* pMWght, UInt xlow, UInt xhigh, UInt ylow, UInt yhigh, Bool bAdd);
	Void bilnrMCH (UInt* PredY, const PixelC* ppxliPrevYC, UInt* pMWght, UInt xlow, UInt xhigh, UInt ylow, UInt yhigh, Bool bAdd);
	Void bilnrMC (UInt* PredY, const PixelC* ppxliPrevYC, UInt* pMWght, UInt xlow, UInt xhigh, UInt ylow, UInt yhigh, Bool bAdd);
	Void assignPredToCurrQ (PixelC* ppxlcRefMBY, PixelC* ppxlcRefMBU, PixelC* ppxlcRefMBV);
	Void assignAlphaPredToCurrQ (PixelC* ppxlcRefMBA);

	Void copyFromRefToCurrQ ( // zero motion case
		const CVOPU8YUVBA* pvopcRef, // reference VOP
		CoordI x, CoordI y, 
		PixelC* ppxlcCurrQMBY, PixelC* ppxlcCurrQMBU, PixelC* ppxlcCurrQMBV,
		CRct *prctMVLimit
	); // for non-obmc mode
	Void copyFromRefToCurrQ_WithShape ( // zero motion case
		const CVOPU8YUVBA* pvopcRef, // reference VOP
		CoordI x, CoordI y, 
		PixelC* ppxlcCurrQMBY, PixelC* ppxlcCurrQMBU, PixelC* ppxlcCurrQMBV, PixelC* ppxlcCurrQMBBY
	); // for non-obmc mode
	Void copyAlphaFromRefToCurrQ (
		const CVOPU8YUVBA* pvopcRef,
		CoordI x, CoordI y, 
		PixelC* ppxlcCurrQMBA,
		CRct *prctMVLimit
	);

	// MC padding 
	Void mcPadCurrMB (
		PixelC* ppxlcRefMBY, 
		PixelC* ppxlcRefMBU, PixelC* ppxlcRefMBV,
		PixelC* ppxlcRefMBA 
	);
	Void mcPadCurr (
		PixelC *ppxlcTextureBase, // (uiStride X ???)
		const PixelC *ppxlcAlphaBase, // uiBlkSize X uiBlkSize
		UInt uiBlkSize, UInt uiStride
	);
	Void padNeighborTranspMBs (
		CoordI xb, CoordI yb,
		CMBMode* pmbmd,
		PixelC* ppxlcRefMBY, PixelC* ppxlcRefMBU, PixelC* ppxlcRefMBV,
		PixelC* ppxlcRefMBA
	);
	Void padCurrAndTopTranspMBFromNeighbor (
		CoordI xb, CoordI yb,
		CMBMode* pmbmd,
		PixelC* ppxlcRefMBY, PixelC* ppxlcRefMBU, PixelC* ppxlcRefMBV,
		PixelC* ppxlcRefMBA
	);
	Void mcPadLeftMB (PixelC* ppxlcMBY, PixelC* ppxlcMBU, PixelC* ppxlcMBV, PixelC* ppxlcRefMBA);
	Void mcPadTopMB (PixelC* ppxlcMBY, PixelC* ppxlcMBU, PixelC* ppxlcMBV, PixelC* ppxlcRefMBA);
	Void mcPadCurrMBFromLeft (PixelC* ppxlcMBY, PixelC* ppxlcMBU, PixelC* ppxlcMBV, PixelC* ppxlcRefMBA);
	Void mcPadCurrMBFromTop (PixelC* ppxlcMBY, PixelC* ppxlcMBU, PixelC* ppxlcMBV, PixelC* ppxlcRefMBA);
	Void mcSetTopMBGray (PixelC* ppxlcMBY, PixelC* ppxlcMBU, PixelC* ppxlcMBV, PixelC* ppxlcRefMBA);
	Void mcSetCurrMBGray (PixelC* ppxlcMBY, PixelC* ppxlcMBU, PixelC* ppxlcMBV, PixelC* ppxlcRefMBA);
	Void mcSetLeftMBGray (PixelC* ppxlcMBY, PixelC* ppxlcMBU, PixelC* ppxlcMBV, PixelC* ppxlcRefMBA);

	// Added for field based MC padding by Hyundai(1998-5-9)
        Void fieldBasedMCPadding (
                CMBMode* pmbmd,
                CVOPU8YUVBA* pvopcCurrQ
        );
        Void mcPadCurrAndNeighborsMBFields (
                Int     iMBX,
                Int     iMBY,
                CMBMode *pmbmd,
                PixelC* ppxlcRefMBY,
                PixelC* ppxlcRefMBU,
                PixelC* ppxlcRefMBV,
                PixelC* ppxlcRefMBBY,
                PixelC* ppxlcRefMBBUV,
				PixelC* ppxlcRefMBA
        );
        Void mcPadFieldsCurr (
                Int     iMBX,
                Int     iMBY,
                CMBMode *pmbmd,
                Int     mode,
                PixelC  *ppxlcCurrMB1,
                PixelC  *ppxlcCurrMB2,
                PixelC  *ppxlcCurrMBB,
                Int     uiBlkXSize,
                Int     uiStride
        );
        Void mcPadOneField (
                        PixelC *ppxlcTextureBase,
                        const PixelC *ppxlcAlphaBase,
                        Int uiBlkXSize,
                        Int uiStride
        );
        Void padNeighborTranspMBFields (
                CoordI  xb,
                CoordI  yb,
                CMBMode *pmbmd,
                Int     mode,
                PixelC  *ppxlcMBField1,
                PixelC  *ppxlcMBField2,
                Int     uiBlkXSize,
                Int     uiStride
        );         
        Void padCurrAndTopTranspMBFieldsFromNeighbor (
                CoordI  xb,
                CoordI  yb,
                CMBMode *pmbmd,
                Int     mode,
                PixelC* ppxlcMBField1,
                PixelC* ppxlcMBField2,
                Int     uiBlkXSize,
                Int     uiStride
        );         
        Void mcPadLeftMBFields (
                PixelC* ppxlcMBField1,
                PixelC* ppxlcMBField2,
                Int uiBlkXSize,
                Int uiStride
        );         
        Void mcPadTopMBFields (
                PixelC* ppxlcMBField1,
                PixelC* ppxlcMBField2,
                Int uiBlkXSize,
                Int uiStride
        );         
        Void mcPadCurrMBFieldsFromLeft (
                PixelC* ppxlcMBField1,
                PixelC* ppxlcMBField2,
                Int uiBlkXSize,
                Int uiStride
        );         
        Void mcPadCurrMBFieldsFromTop (
                PixelC* ppxlcMBField1,
                PixelC* ppxlcMBField2,
                Int uiBlkXSize,
                Int uiStride
        );         
        Void mcSetTopMBFieldsGray (
                PixelC* ppxlcMBField1,
                PixelC* ppxlcMBField2,
                Int uiBlkXSize,
                Int uiStride
        );         
        Void mcSetLeftMBFieldsGray (
                PixelC* ppxlcMBField1,
                PixelC* ppxlcMBField2,
                Int uiBlkXSize,
                Int uiStride
        );         
        Void mcSetCurrMBFieldsGray (
                PixelC* ppxlcMBField1,
                PixelC* ppxlcMBField2,
                Int uiBlkXSize,
                Int uiStride
        );         
	// End of Hyundai(1998-5-9)

	// motion vector
	Void find16x16MVpred (CVector& vctPred, const CMotionVector* pmv, Bool bLeftBndry, Bool bRightBndry, Bool bTopBndry) const;
	Void find8x8MVpredAtBoundary (CVector& vctPred, const CMotionVector* pmv, Bool bLeftBndry, Bool bRightBndry, Bool bTopBndry, BlockNum blknCurr) const;
	Void find8x8MVpredInterior (CVector& vctPred, const CMotionVector* pmv, BlockNum blknCurr) const;
	Void findMVpredGeneric (CVector& vctPred, const CMotionVector* pmv, const CMBMode* pmbmd, Int iBlk, Int iXMB, Int iYMB) const;
	Void findMVpredictorOfBVOP (CVector& vctPred, const CMotionVector* pmv, const CMBMode* pmbmd, Int iMBX) const;  		    //for B-VOP only
	Void mvLookupUV (const CMBMode* pmbmd, const CMotionVector* pmv, CoordI& xRefUV, CoordI& yRefUV,
        CoordI& xRefUV1, CoordI& yRefUV1);
	Void mvLookupUVWithShape (const CMBMode* pmbmd, const CMotionVector* pmv, CoordI& xRefUV, CoordI& yRefUV);
	Bool validBlock (const CMBMode* pmbmdCurr, const CMBMode* pmbmd, BlockNum blkn) const;
	Void computeDirectDeltaMV (CVector& vctDiff, const CMotionVector* pmv, const CMotionVector* pmvRef, const CMBMode* pmbmdRef);	//for B-VOP only
	CVector averageOfRefMV (const CMotionVector* pmvRef, const CMBMode* pmbmdRef);													//for B-VOP only
	Void padMotionVectors(const CMBMode *pmbmd,CMotionVector *pmv);
	Void backwardMVFromForwardMV (								//compute back mv from forward mv and ref mv for direct mode
					CMotionVector& mvBackward, const CMotionVector& mvForward, const CMotionVector& mvRef,
					CVector vctDirectDeltaMV);


//	Added for error resilient mode by Toshiba(1997-11-14)
	Bool bVPNoLeft(Int iMBnum, Int iMBX) const;
	Bool bVPNoRightTop(Int iMBnum, Int iMBX) const;
	Bool bVPNoTop(Int iMBnum) const;
	Bool bVPNoLeftTop(Int iMBnum, Int iMBX) const;
	Int VPMBnum(Int iMBX, Int iMBY) const;
// End Toshiba(1997-11-14)

	// block level
	Void inverseQuantizeIntraDc (Int* rgiCoefQ, Int iDcScaler);
	Void inverseQuantizeDCTcoefH263 (Int* rgiCoefQ, Int iStart, Int iQP);
	Void inverseQuantizeIntraDCTcoefMPEG (Int* rgiCoefQ, Int iStart, Int iQP, Bool bUseAlphaMatrix);
	Void inverseQuantizeInterDCTcoefMPEG (Int* rgiCoefQ, Int iStart, Int iQP, Bool bUseAlphaMatrix);
	const BlockMemory findPredictorBlock (
		BlockNum iBlk, 
		IntraPredDirection predDir,
		const MacroBlockMemory* pmbmLeft, 
		const MacroBlockMemory* pmbmTop,
		const MacroBlockMemory* pmbmLeftTop,
		const MacroBlockMemory* pmbmCurr,
		const CMBMode* pmbmdLeft, 
		const CMBMode* pmbmdTop,
		const CMBMode* pmbmdLeftTop,
		const CMBMode* pmbmdCurr,
		Int& iQPpred
	);
	Int decideIntraPredDir (
		Int* rgiCoefQ,
		BlockNum blkn,
		const BlockMemory& blkmRet, 
		const MacroBlockMemory* pmbmLeft, 
		const MacroBlockMemory* pmbmTop, 
		const MacroBlockMemory* pmbmLeftTop,
		const MacroBlockMemory* pmbmCurr,
		const CMBMode* pmbmdLeft,
		const CMBMode* pmbmdTop,
		const CMBMode* pmbmdLeftTop,
		CMBMode* pmbmdCurr,
		Int&	iQPpred,
		Int iQPcurr,
		Bool bDecideDCOnly = FALSE
	);

	Time m_t; // current time
	Time m_tPastRef; // time of reference frame in past (for P/B)
	Time m_tFutureRef; // time of reference frame in future (for B)
	Int m_iBCount; // counts 1,2,3 with B frames, used to pad ref vop once.
	Bool m_bCodedFutureRef;

    CRct m_rctDisplayWindow;						//display windoew position

	Int	 m_iNumBitsTimeIncr;
	Time m_tDistanceBetwIPVOP; //no. of frms betw. I/PVOP = PeriodOfPVOP except for at irregular period (end of sequence)
	Time m_tPrevIorPVOPCounter; //frm. no of previous encoded I/PVOP
	Time m_tModuloBaseDisp;							//of the most recently displayed I/Pvop
	Time m_tModuloBaseDecd;							//of the most recently decoded I/Pvop
/*Added by SONY (98/03/30)*/
	Bool m_bUseGOV ;
	Bool m_bLinkisBroken;
/*Added by SONY (98/03/30)*/

	// MB buffer data
	CVOPU8YUVBA* m_pvopcPredMB;
	PixelC *m_ppxlcPredMBY, *m_ppxlcPredMBU, *m_ppxlcPredMBV, *m_ppxlcPredMBA;
	CVOPIntYUVBA* m_pvopiErrorMB;
	PixelI *m_ppxliErrorMBY, *m_ppxliErrorMBU, *m_ppxliErrorMBV, *m_ppxliErrorMBA;

	// B-VOP MB buffer
	CVOPU8YUVBA* m_pvopcPredMBBack; // backward buffer data
	PixelC *m_ppxlcPredMBBackY, *m_ppxlcPredMBBackU, *m_ppxlcPredMBBackV, *m_ppxlcPredMBBackA;

	// MB shape data
	ArCodec* m_parcodec;			//arithmatic coder
	CU8Image* m_puciPredBAB;		//motion compensated binary shape
	PixelC *m_ppxlcPredBABDown2, *m_ppxlcPredBABDown4;
	PixelC* m_ppxlcReconCurrBAB;
	Int m_iWidthCurrBAB;
	PixelC* m_rgpxlcCaeSymbol;
	PixelC *m_ppxlcCurrMBBYDown4, *m_ppxlcCurrMBBYDown2;

	ShapeMode* m_rgshpmd;  // for saving reference shape mode
	Int m_iRefShpNumMBX;
	Int m_iRefShpNumMBY;

	CVOPU8YUVBA* m_pvopcCurrMB;
	PixelC *m_ppxlcCurrMBY, *m_ppxlcCurrMBU, *m_ppxlcCurrMBV, *m_ppxlcCurrMBBY, *m_ppxlcCurrMBBUV, *m_ppxlcCurrMBA;

	/*BBM// Added for Boundary by Hyundai(1998-5-9)
        Void boundaryMacroBlockMerge (CMBMode* pmbmd);
        Void isBoundaryMacroBlockMerged (CMBMode* pmbmd);
        Void isBoundaryMacroBlockMerged (CMBMode* pmbmd, PixelC* ppxlcRightMBBY);
        Void overlayBlocks (UInt x1, UInt x2, UInt y1, UInt y2, DCTMode dctMd);
        Void overlayBlocks (PixelC* SB2, PixelI* ppxlcB1, PixelI* ppxlcB2);
        Void overlayBlocks (PixelC* SB1, PixelC* SB2, PixelC* ppxlcB1, PixelC* ppxlcB2);
        Bool checkMergedStatus (UInt x1, UInt x2, UInt y1, UInt y2);
        Bool checkMergedStatus (UInt x1, UInt x2, UInt y1, UInt y2, PixelC* ppxlcBY);
        Void mergedMacroBlockSplit (CMBMode* pmbmd, PixelC* ppxlcRefMBY = NULL, PixelC* ppxlcRefMBA = NULL);
        Void splitTwoMergedBlocks (UInt x1, UInt x2, UInt y1, UInt y2, PixelC* ppxlcIn1, PixelC* ppxlcIn2 = NULL);
        Void splitTwoMergedBlocks (UInt x1, UInt x2, UInt y1, UInt y2, PixelI* ppxlcIn1, PixelI* ppxlcIn2 = NULL);
        Void swapTransparentModes (CMBMode* pmbmd, Bool mode);
        Void setMergedTransparentModes (CMBMode* pmbmd);
        Void initMergedMode (CMBMode* pmbmd);
	// End of Hyundai(1998-5-9)*/
	
	// for MC-padding
	Bool* m_pbEmptyRowArray;
	Bool* m_pbEmptyColArray;

	// VOP mode 
	VOLMode	m_volmd; // vol mode
	VOPMode	m_vopmd; // vop mode
	UInt m_uiVOId; // VOP ID

	// sprite info
	UInt m_uiSprite; // whether this is a sprite VOP: 1 - yes; other - no
	UInt m_uiWarpingAccuracy; // accuracy for sprite warping
	Int m_iNumOfPnts; // for sprite warping
	own CSiteD* m_rgstSrcQ; // quantized src sts for sprite warping
	own CSiteD* m_rgstDstQ; // quantized dst sts for sprite warping
	own CVOPU8YUVBA* m_pvopcSptQ; // Loaded first as original sprite and then become quantized sprite. 
	CRct m_rctSpt; // rct of m_pvopcSptQ
	CRct m_rctSptDisp; //rct to display sprite (decoder side)
	SptMode m_sptMode;  // sprite reconstruction mode : 0 -- basic sprite , 1 -- Object piece only, 2 -- Update piece only, 3 -- intermingled	
	own MBSptMode* m_rgsptmd; // sprite mode 0: untransmitted; 1: transmitted; 2: updated; 3: done

	// internal data
	own CVOPU8YUVBA* m_pvopcRefQ0; // original reference VOP in a previous time
	own CVOPU8YUVBA* m_pvopcRefQ1; // original reference VOP in a later time
	own CVOPU8YUVBA* m_pvopcCurrQ; // original reference VOP in a later time
	Int m_iStartInRefToCurrRctY, m_iStartInRefToCurrRctUV;

	// motion data
	own CDirectModeData* m_pdrtmdRef1; // for direct mode
	CVector	m_vctForwardMvPredBVOP[2];		// motion vector predictors for B-VOP
	CVector	m_vctBackwardMvPredBVOP[2];     // [2] added for interlace (top & bottom fields)

	// some fixed variables (VOL)
	CRct m_rctRefFrameY, m_rctRefFrameUV;
	Int m_iFrameWidthYxMBSize, m_iFrameWidthYxBlkSize, m_iFrameWidthUVxBlkSize;
	Int m_iFrameWidthY, m_iFrameWidthUV;
	Int m_ivolWidth, m_ivolHeight;

	// VOP variables
	CRct m_rctCurrVOPY, m_rctCurrVOPUV;
	CRct m_rctRefVOPY0, m_rctRefVOPUV0;
	CRct m_rctRefVOPY1, m_rctRefVOPUV1;
	Int m_iOffsetForPadY, m_iOffsetForPadUV;
	CRct m_rctPrevNoExpandY, m_rctPrevNoExpandUV;
////// 97/12/22 start
	CRct m_rctBVOPRefVOPY1, m_rctBVOPRefVOPUV1;
	Int m_iBVOPOffsetForPadY, m_iBVOPOffsetForPadUV;
	CRct m_rctBVOPPrevNoExpandY, m_rctBVOPPrevNoExpandUV;
///// 97/12/22 end

	Int m_iVOPWidthY, m_iVOPWidthUV;
	Int m_iNumMB, m_iNumMBX, m_iNumMBY;
	Int m_iNumOfTotalMVPerRow;
	Int m_iSessNumMB;

	CMBMode* m_rgmbmd; // VOP size.  need to renew every VOP
	CMotionVector* m_rgmv; // VOP size.  need to renew every VOP
	CMotionVector* m_rgmvBackward; // VOP size.  need to renew every VOP
	CMotionVector* m_rgmvBY; //Motion vectors for BY plane
	
	CMBMode* m_rgmbmdRef;
	CMotionVector* m_rgmvRef; // VOP size.  need to renew every VOP
	Int m_iNumMBRef, m_iNumMBXRef, m_iNumMBYRef;

	// clipping tables
/* NBIT: change U8 to PixelC
	U8* m_rgiClipTab; // clapping the reconstructed pixels
*/
	PixelC* m_rgiClipTab; // clapping the reconstructed pixels
	Int m_iOffset; // NBIT
	Void setClipTab(); // NBIT

//	Added for error resilient mode by Toshiba(1997-11-14)
	Bool 	m_bVPNoLeft;
	Bool 	m_bVPNoRightTop;
	Bool 	m_bVPNoTop;
	Bool 	m_bVPNoLeftTop;
	Int	m_iVPMBnum;			// start MB in a VP
// End Toshiba(1997-11-14)

	// MB data
	Int** m_rgpiCoefQ;
	MacroBlockMemory** m_rgpmbmAbove;
	MacroBlockMemory** m_rgpmbmCurr;
	BlockMemory* m_rgblkmCurrMB;	//predictor blocks for each current block
	Int* m_rgiQPpred;				//QP from previous block for intra ac prediction
	Int m_rgiDcScalerY [32];		//intra dc quantization scaler; qp dependent	
	Int m_rgiDcScalerC [32];		//[0] elem never used

	// block data
	Int m_rgiDCTcoef [BLOCK_SQUARE_SIZE];
	CInvBlockDCT* m_pidct;

	// Sprite routines
	Void warpYA (const CPerspective2D& persp, const CRct& rctWarpedBound, UInt accuracy);
	Void warpUV (const CPerspective2D& persp, const CRct& rctWarpedBound, UInt accuracy);

    // Fast Affine Warping
    Void FastAffineWarp (const CRct& rctWarpedBound, const CRct& rctWarpedBoundUV, UInt accuracy, UInt pntNum);
    PixelC CInterpolatePixelValue (PixelC* F, Int pos, Int width, Int rx, Int ry, Int wpc, Int bias, Int pow_denom);
    Int LinearExtrapolation(Int x0, Int x1, Int x0p, Int x1p, Int W, Int VW);
    Void FourSlashes(Int num, Int denom, Int *quot, Int *res);
    // tentative solution for indicating the first Sprite VOP
    Int tentativeFirstSpriteVop;

	// Low-latency sprite info
    Bool m_bFirstSpriteVop ;// flag for indicating the first Sprite VOP     
	MacroBlockMemory*** m_rgpmbmCurr_Spt;	 //  save MB data to be used as holes
	SptMBstatus** m_ppPieceMBstatus;	 // MB transmission status (done or not-done)
	SptMBstatus** m_ppUpdateMBstatus;	
		
	CMBMode* m_rgmbmdSprite;  // dshu: [v071]  added to store mbmd array for sprite

	CMBMode** m_rgmbmdSpt; // Sprite size. 
	Bool m_bSptMB_NOT_HOLE;	   // Current MB is not a hole
	own CSiteD* m_rgstSrcQP; // for sprite piece warping
	Time m_tPiece; // current sprite piece encode time
	Time m_tUpdate; // current sprite update encode time 
	own CVOPU8YUVBA* m_pvopcSptP; //  sprite piece 	  
	CRct m_rctSptQ;	  //  rct of m_pvopcSptQ 
	CRct m_rctOrg;	  // Frame size
	CRct m_rctPieceQ;  // Region of pieces which have been encoded
	CRct m_rctUpdateQ;  // Region of updates which have been encoded
	CRct m_rctSptPieceY;  // piece of sprite which is currently been encoded
	CRct m_rctSptPieceUV;  // piece of sprite which is currently been encoded
	Int m_iStepI;
	Int m_iStep; 
	Int m_iStep_VOP;
	Int m_iPieceXoffset;
	Int m_iPieceYoffset;
	Int m_iPieceWidth;
	Int m_iPieceHeight; 
	Bool m_bSptZoom;  // the type of sprite warping(zoom/pan)
    Bool m_bSptHvPan; // the type of sprite warping(Horizontal or vertical panning)
	Bool m_bSptRightPiece; 	//direction (up/down or right/left) for encoding sprite piece
	Int *m_pSptmbBits; // bits used by a sprite macroblock
	CRct m_rctSptExp;  // Extend the sprite size to multiples of MBSize

};

#endif // __VOPSES_HPP_

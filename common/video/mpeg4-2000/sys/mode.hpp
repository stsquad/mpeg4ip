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

	Marc Mongenet (Marc.Mongenet@epfl.ch), Swiss Federal Institute of Technology, Lausanne (EPFL)
    
and also edited by
    Mathias Wien (wien@ient.rwth-aachen.de) RWTH Aachen / Robert BOSCH GmbH

and also edited by
	Yoshinori Suzuki (Hitachi, Ltd.)

and also edited by
	Hideaki Kimata (NTT)

and also edited by
    Fujitsu Laboratories Ltd. (contact: Eishi Morimatsu)

and also edited by 
	Takefumi Nagumo (nagumo@av.crl.sony.co.jp) SONY corporation
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

	mode.h

Abstract:

	basic coding modes for VO, VOL, VOP, MB	and RD

Revision History:
	Sept. 29, 1997: add Video Packet, data partition, RVLC by Toshiba
	Nov.  27, 1997: add horizontal & vertical sampling factor by Takefumi Nagumo 
									(nagumo@av.crl.sony.co.jp) SONY
	Dec.12 1997 : add interlace tools by NextLevel Systems (General Instrucment), 
				  X. Chen (xchen@nlvl.com) and B. Eifrig (beifrig@nlvl.com)
	May. 9 1998 : add boundary by Hyundai Electronics 
				  Cheol-Soo Park (cspark@super5.hyundai.co.kr)
	May. 9 1998 : add field based MC padding by Hyundai Electronics 
				  Cheol-Soo Park (cspark@super5.hyundai.co.kr)
	Jun.15 1998 : add Complexity Estimation syntax support
				  Marc Mongenet (Marc.Mongenet@epfl.ch) - EPFL
    Feb.16 1999 : add Quarter Sample 
                  Mathias Wien (wien@ient.rwth-aachen.de) 
	Feb.23 1999 : GMC added by Yoshinori Suzuki (Hitachi, Ltd.) 
	May 9, 1999 : tm5 rate control by DemoGraFX, duhoff@mediaone.net, (added by mwi)
	Aug.24, 1999 : NEWPRED added by Hideaki Kimata (NTT) 
	Sep.06	1999 : RRV added by Eishi Morimatsu (Fujitsu Laboratories Ltd.) 
	Feb.01	2000 : Added bBGComposition flag by Takefumi Nagumo (Sony Corporation)
	Mar.22  2000 : Added bResyncMarkerDisable by Takefumi Nagumo (Sony Corporation)

*************************************************************************/

#ifndef __MODE_H_
#define __MODE_H_
class CMBMode;
class CDirectModeData;
class CStatistics;
typedef enum {BASE_LAYER, ENHN_LAYER} VOLtype; // will be generlized later
typedef enum {INTRA, INTRAQ, INTER, INTERQ} DCTMode; // define pixel component
typedef enum {DIRECT, INTERPOLATE, BACKWARD, FORWARD} MBType; // define MB type
typedef enum {UNKNOWN_DIR, HORIZONTAL, VERTICAL, DIAGONAL} Direction;	
typedef enum {ALL_TRANSP, ALL_OPAQUE, INTRA_CAE, INTER_CAE_MVDZ, INTER_CAE_MVDNZ, MVDZ_NOUPDT, MVDNZ_NOUPDT, UNKNOWN} ShapeMode;

//OBSS_SAIT_991015
typedef enum {INTRA_NOT_CODED, INTRA_CODED, INTER_NOT_CODED, INTER_CODED, UNDEFINED} ShapeSSMode; //for OBSS shape mode
typedef enum {V_SCANNING, H_SCANNING} SIDirection;	//for OBSS shape coding
//~OBSS_SAIT_991015

typedef enum {UNTRANSMITTED, TRANSMITTED, UPDATED, FINISHED} MBSptMode; // MB sprite mode
typedef enum {ALPHA_CODED, ALPHA_SKIPPED, ALPHA_ALL255} CODAlpha;
typedef Direction IntraPredDirection;								//for readability
typedef Direction CAEScanDirection;								//for readability

#ifndef NOT_IN_TABLE
#define NOT_IN_TABLE -1
#endif
#ifndef	TCOEF_ESCAPE
#define TCOEF_ESCAPE 102							// see table.13/H.263
#endif
// Added for error resilience mode By Toshiba(1998-1-16:DP+RVLC)
#define TCOEF_RVLC_ESCAPE 169	  	// see table.
//	End Toshiba(1998-1-16:DP+RVLC)

// VM 5.1 Rate Control
#define RC_MAX_SLIDING_WINDOW	20
#define RC_PAST_PERCENT  		0.05
#define RC_SAFETY_MARGIN 		0.10
#define RC_SKIP_MARGIN 			80
#define RC_MAX_Q_INCREASE		0.25
#define RC_MAX_QUANT 			31
#define RC_MIN_QUANT 			1

#define MAX_MAC 10

// RRV insertion
typedef struct {
//for parameter input, see Documents
	Int		iOnOff;
	Int		iOnOffForI;
	Float	fC1;
	Float	fC2;
	Float	fFR1;
	Float	fFR2;
	Int		iQP1;
	Int		iQP2;
	Int		iCycle;
//for parameter calculate,decide
	Int		iRRVOnOff;		//1==ON,0==OFF
	Int		iQcount;		//Quantizer counter
	Int		iQave;			//average of Q
	Int		iPrevOnOff;		//OnOff of previous VOP
	Int		iNumBits;		//number of previous bits
    Int		iCutoffThr;		// for encoder
} RRVmodeStr;
// ~RRV

typedef struct MVInfo					 // for motion vector coding
{
	UInt uiRange;						 // search range
	UInt uiFCode;						 // f-code 
	UInt uiScaleFactor;					 // scale factor
} MVInfo;

#define PVOP_MV_PER_REF_PER_MB  9
#define BVOP_MV_PER_REF_PER_MB  5


typedef struct VOLMode					 // VideoObjectLayer Mode
{
	UInt		uiVerID;		 // Version identification // GMC
	// type of VOL
	VOLtype		volType;				 // what type of VOL

	// NBIT: nbit information
	Bool bNot8Bit;
	UInt uiQuantPrecision;
	UInt nBits;

	// time info
	Int			iClockRate;				 //rate of clock used to count vops in Hz
	Double	    dFrameHz;				 // Frame frequency (Hz), (floating point in case of 29.97 Hz)

	// shape info
	AlphaUsage	fAUsage;				 //binary or gray level alpha; or no alpha (rectangle VO)
  Int     iAlphaShapeExtension;
  Int     iAuxCompCount;
	Bool		bShapeOnly;				 // code only the shape
	Int			iBinaryAlphaTH;			 //binary shaperounding parameter
	Int			iBinaryAlphaRR;			 //binary shaperounding refresh rate: for Added error resilient mode by Toshiba(1997-11-14)
	Int			iGrayToBinaryTH;
	Bool		bNoCrChange;			 //nobinary shape size conversion
	
	// motion info
	Bool	bOriginalForME;			// flag indicating whether use the original previous VOP for ME
	UInt	uiWarpingAccuracy;		// indicates the quantization accuracy of motion vector for sprite warping
	Bool	bAdvPredDisable;		// No OBMC, (8x8 in the future).
  Bool    bQuarterSample;         // Quarter sample
	Bool	bRoundingControlDisable;
	Int	iInitialRoundingType;

// NEWPRED
	Bool	bNewpredEnable;
	int 	iRequestedBackwardMessegeType;
	Bool	bNewpredSegmentType;
	char	*cNewpredRefName;
	char	*cNewpredSlicePoint;
// ~NEWPRED
//RESYNC_MARKER_FIX
	Bool	bResyncMarkerDisable;		// resync marker Disable
//~RESYNC_MARKER_FIX
	Bool	bVPBitTh;				// Bit threshold for video packet spacing control
	Bool	bDataPartitioning;		// data partitioning
	Bool	bReversibleVlc;			// reversible VLC

	Bool    bCodeSequenceHead;
	UInt    uiProfileAndLevel;

	// texture coding info
	Quantizer fQuantizer;			// either H.263 or MPEG
	Bool bLoadIntraMatrix;			// flag indicating whether to load intra Q-matrix
	Int rgiIntraQuantizerMatrix [BLOCK_SQUARE_SIZE]; // Intra Q-Matrix
	Bool bLoadInterMatrix;			// flag indicating whether to load inter Q-matrix
	Int rgiInterQuantizerMatrix [BLOCK_SQUARE_SIZE]; // Inter Q-Matrix
	Bool bLoadIntraMatrixAlpha;			// flag indicating whether to load intra Q-matrix
	Int  rgiIntraQuantizerMatrixAlpha[MAX_MAC][BLOCK_SQUARE_SIZE];
	Bool bLoadInterMatrixAlpha;			// flag indicating whether to load inter Q-matrix
	Int  rgiInterQuantizerMatrixAlpha[MAX_MAC][BLOCK_SQUARE_SIZE];
	Bool bDeblockFilterDisable;		// apply deblocking filter or not.
	Bool bNoGrayQuantUpdate;		// decouple gray quant and dquant
	EntropyCodeType fEntropyType;	// Entropy code type

	// HHI Klaas Schueuer sadct flag
	Bool	bSadctDisable;
	// end HHI 

	// START: Complexity Estimation syntax support - Marc Mongenet (EPFL) - 15 Jun 1998
	Bool bComplexityEstimationDisable;
	Int iEstimationMethod;
	Bool bShapeComplexityEstimationDisable;
	Bool bOpaque;
	Bool bTransparent;
	Bool bIntraCAE;
	Bool bInterCAE;
	Bool bNoUpdate;
	Bool bUpsampling;
	Bool bTextureComplexityEstimationSet1Disable;
	Bool bIntraBlocks;
	Bool bInterBlocks;
	Bool bInter4vBlocks;
	Bool bNotCodedBlocks;
	Bool bTextureComplexityEstimationSet2Disable;
	Bool bDCTCoefs;
	Bool bDCTLines;
	Bool bVLCSymbols;
	Bool bVLCBits;
	Bool bMotionCompensationComplexityDisable;
	Bool bAPM;
	Bool bNPM;
	Bool bInterpolateMCQ;
	Bool bForwBackMCQ;
	Bool bHalfpel2;
	Bool bHalfpel4;
	// START: Complexity Estimation syntax support - Update version 2 - Massimo Ravasi (EPFL) - 5 Nov 1999
	Bool bVersion2ComplexityEstimationDisable;
	Bool bSadct;
	Bool bQuarterpel;
	// END: Complexity Estimation syntax support - Update version 2

	// START: Vol Control Parameters
	UInt uiVolControlParameters;
	UInt uiChromaFormat;
	UInt uiLowDelay;
	UInt uiVBVParams;
	UInt uiBitRate;
	UInt uiVbvBufferSize;
	UInt uiVbvBufferOccupany;
	// END: Vol Control Parameters

	// frame rate info
	Int iTemporalRate;				//no. of input frames between two encoded VOP's assuming 30Hz input
	Int iPbetweenI;
	Int iBbetweenP;
	Int iGOVperiod;					//number of VOP from GOV header to next GOV header
									//added by SONY 980212

	Bool bAllowSkippedPMBs;

	// scalability info
//#ifdef _Scalable_SONY_
	Int iHierarchyType;
//#endif _Scalable_SONY_
	Int iEnhnType;					//enhancement type

	Int iEnhnTypeSpatial;					//for OBSS partial enhancement mode		//OBSS_SAIT_991015

	Int iSpatialOption;
	Int ihor_sampling_factor_m ;
	Int ihor_sampling_factor_n ;
	Int iver_sampling_factor_m ;
	Int iver_sampling_factor_n ;

//OBSS_SAIT_991015
    // object based spatial scalability
	Int iFrmWidth_SS;						
	Int iFrmHeight_SS;						
	Int iuseRefShape;	
	Int iuseRefTexture;
	Int ihor_sampling_factor_m_shape ;
	Int ihor_sampling_factor_n_shape ;
	Int iver_sampling_factor_m_shape ;
	Int iver_sampling_factor_n_shape ;
	Bool bSpatialScalability;	
//~OBSS_SAIT_991015

	// temporal scalability  // added by Sharp (98/2/10)
	Bool bTemporalScalability;

	// statistics dumping options
	Bool bDumpMB;			// dump statitstics at MB level
	Bool bTrace;			// dumping trace file

	Int	iMVRadiusPerFrameAwayFromRef;	// MV serach radius per frame away from reference VOP
// RRV insertion
	Bool	breduced_resolution_vop_enable;
// ~RRV	
} VOLMode;

typedef struct VOPMode // VideoObjectPlane Mode
{
	// user specify, per VOP
	Int intStepI;	// I-VOP stepsize for DCT
	Int intStep;	// P-VOP stepsize for DCT
	Int intStepB;	// B-VOP stepsize for DCT
	
  Int intStepIAlpha[MAX_MAC];	// I-VOP stepsize for DCT alpha
	Int intStepPAlpha[MAX_MAC];	// P-VOP stepsize for DCT alpha
	Int intStepBAlpha[MAX_MAC];	// B-VOP stepsize for DCT alpha

	Int intStepDiff;	// stepsize difference for updating for DCT (DQUANT)
//	Int intDBQuant;
	VOPpredType vopPredType; //whether IVOP, PVOP, BVOP, or Sprite
	Int iIntraDcSwitchThr;	 //threshold to code intraDC as with AC VLCs
	Int iRoundingControl;	 //rounding control
	Int iRoundingControlEncSwitch;
	ShapeBPredDir fShapeBPredDir;  // shape prediction direction BVOP

	Int iVopConstantAlphaValue;   // for binary or grayscale shape pk val

//OBSSFIX_MODE3
	Bool bBGComposition;
//~OBSSFIX_MODE3

	// Complexity Estimation syntax support - Marc Mongenet (EPFL) - 15 Jun 1998
	Int iOpaque;
	Int iTransparent;
	Int iIntraCAE;
	Int iInterCAE;
	Int iNoUpdate;
	Int iUpsampling;
	Int iIntraBlocks;
	Int iInterBlocks;
	Int iInter4vBlocks;
	Int iNotCodedBlocks;
	Int iDCTCoefs;
	Int iDCTLines;
	Int iVLCSymbols;
	Int iVLCBits;
	Int iAPM;
	Int iNPM;
	Int iInterpolateMCQ;
	Int iForwBackMCQ;
	Int iHalfpel2;
	Int iHalfpel4;
	// START: Complexity Estimation syntax support - Update version 2 - Massimo Ravasi (EPFL) - 5 Nov 1999
	Int iSadct;
	Int iQuarterpel;
	// END: Complexity Estimation syntax support - Update version 2

	// motion search info
	MVInfo	mvInfoForward;					// motion search info
	MVInfo	mvInfoBackward;					// motion search info
	Int	iSearchRangeForward;				// maximum search range for motion estimation
	Int	iSearchRangeBackward;				// maximum search range for motion estimation

	Bool bInterlace;						// interlace coding flag
	Bool bTopFieldFirst;					// Top field first
    Bool bAlternateScan;                    // Alternate Scan
	Int  iDirectModeRadius;					// Direct mode search radius (half luma pels)

	// for scalability
	Int iRefSelectCode;
	Int iLoadForShape; // load_forward_shape
	Int iLoadBakShape; // load_backward_shape
	Bool bShapeCodingType; // vop_shape_coding_type (0:intra, 1:inter): Added for error resilient mode by Toshiba(1997-11-14)
	SptXmitMode SpriteXmitMode;	// sprite transmit mode 

// NEWPRED
	int		m_iVopID;
	int		m_iNumBitsVopID;
	int		m_iVopID4Prediction_Indication;
	int		m_iVopID4Prediction;
// ~NEWPRED

// RRV insertion
	RRVmodeStr		RRVmode;
// ~RRV
} VOPMode;



class CMBMode // MacroBlock Mode
{
public:
	// Constructors
	~CMBMode ();
    //CMBMode () {}  // forbidden (SB)
	CMBMode (Int iAuxCompCount=0); 
	CMBMode (const CMBMode& md);

	// Operations
	Void setCodedBlockPattern (Int blkn, Bool bisCoded) 
								{m_rgbCodedBlockPattern[(UInt) blkn - 1] = bisCoded;}
	Void setCodedBlockPattern (const Bool* rgbblockNum);
	Void setMinError (Int blkn, Int iminError)
								{m_rgfltMinErrors[(UInt) blkn - 1] = (Float) iminError;}
	Void setMinError (const Float* pfltminError);
	Void operator = (const CMBMode& md);

	// Attributes
	Bool getCodedBlockPattern (Int blkn) const 
								{return m_rgbCodedBlockPattern[(UInt) blkn - 1];};
	Bool* getCodedBlockPattern () const {return m_rgbCodedBlockPattern;}
	Float getMinError (Int blkn) const
									{return m_rgfltMinErrors[(UInt) blkn - 1];};
	Float* getMinError () const {return m_rgfltMinErrors;}
  UInt    blkEnd() const {return m_uiNumBlks;}

	// Some extra data member
	TransparentStatus m_rgTranspStatus [11]; 
		// indicating the transparency status of the MB, either ALL, PARTIAL, or NONE transparent
		// 0: 16x16,  1-4: 8x8
	Int m_rgNumNonTranspPixels [11]; // number of non-transparent pixels

	/*BBM// Added for Boundary by Hyundai(1998-5-9)
        TransparentStatus m_rgTranspStatusBBM [11];
        Bool m_bMerged [7];
	// End of Hyundai(1998-5-9)*/

	// Added for field based MC padding by Hyundai(1998-5-9)
	TransparentStatus m_rgFieldTranspStatus [5];
	Bool m_rgbFieldPadded[5];
	// End of Hyundai(1998-5-9)

	Bool m_bPadded;				// to see whether this all-transparent has been padded
	Bool m_bSkip;				// is the Macroblock skiped. = COD in H.263
// GMC
	Bool m_bMCSEL;                          // whether the MB is the GMC macroblock
// ~GMC
	CODAlpha* m_pCODAlpha;		// alpha Macroblock coded status
	MBType m_mbType;			// macroblock type, DIRECT, FORWARD, BACKWARD, or INTERPOLATE
	DCTMode m_dctMd;			// is the Macroblock inter- or intra- coded
	ShapeMode m_shpmd;			//different context for the first MMR code

	ShapeSSMode m_shpssmd;		//context for the spatial scalable shape code		//OBSS_SAIT_991015

	Int m_intStepDelta;			// change of quantization stepsize = DQUANT in h.263
	Bool m_bhas4MVForward;		//whether the MB has four motion vectors (for forward vectors)
	Bool m_bhas4MVBackward;		//whether the MB has four motion vectors (for backward vectors)
	Bool m_bFieldMV;			// whether the MB is compensated by field motion vectors (for forward vectors) : yes=1
	Bool m_bForwardTop;			// TRUE iff Current Forward Top field MV references the BOTTOM field
	Bool m_bForwardBottom;		// TRUE iff Current Forward Bottom field MV references the BOTTOM field
	Bool m_bBackwardTop;		// TRUE iff Current Backward Top field MV references the BOTTOM field
	Bool m_bBackwardBottom;		// TRUE iff Current Backward Bottom field MV references the BOTTOM field
	Bool m_bFieldDCT;			// use field DCT or not : yes=1
	Bool m_bPerspectiveForward; //whether the MB uses forward perspective motion
	Bool m_bPerspectiveBackward;//whether the MB uses backward perspective motion
	Int  m_stepSize;				//qp for texture
	Int  m_stepSizeAlpha;		//qp for alpha
	IntraPredDirection* m_preddir; // horizonal or vertical
	Bool m_bACPrediction;		// use AC prediction or not
	Bool* m_pbACPredictionAlpha;  // alpha version of above
	Bool m_bInterShapeCoding;	//use predicted binary shape
	Bool m_bCodeDcAsAc;			//code Intra DC with Ac VLC
	Bool m_bCodeDcAsAcAlpha;	// alpha version of above
	Bool m_bColocatedMBSkip;	// for B-VOP, colocated MB skipped or not
// GMC
	Bool m_bColocatedMBMCSEL;       // for B-VOP, colocated MB is a GMC macroblock or not
// ~GMC
	Int  m_iVideoPacketNumber;	//	Video Packet Number; added by Toshiba
	CVector m_vctDirectDeltaMV;	// delta vector for direct mode

// kimata
	Int  m_iNPSegmentNumber;

private:

	Bool* m_rgbCodedBlockPattern; //for each block, 1 = some non-DC components are coded
	Float* m_rgfltMinErrors; //mininal prediction errors for each luminance block
  Int m_iAuxCompCount; // number of alpha planes (MAC)
  UInt m_uiNumBlks;
};


class CDirectModeData // to handle data for direct mode in B-VOP
{
public:
	// Constructor
	~CDirectModeData ();
	CDirectModeData ();

	// Attributes
	UInt numMB () const {return m_uiNumMB;}
	UInt numMBX () const {return m_uiNumMBX;}
	UInt numMBY () const {return m_uiNumMBY;}
	Bool inBound (UInt iMbIdx) const; // check whether the index is inbound
	Bool inBound (UInt idX, UInt idY) const; // check whether the index is inbound
	CMBMode** mbMode () const {return m_ppmbmd;}
	const CMBMode* mbMode (UInt iMbIdx) const 
		{assert (inBound (iMbIdx)); return m_ppmbmd [iMbIdx];}
	const CMBMode* mbMode (UInt idX, UInt idY) const 
		{assert (inBound (idX, idY)); return m_ppmbmd [idX + idY * m_uiNumMBX];}
	CMotionVector** mv () const {return m_prgmv;}
	const CMotionVector* mv (UInt iMbIdx) const 
		{assert (inBound (iMbIdx)); return m_prgmv [iMbIdx];}
	const CMotionVector* mv (UInt idX, UInt idY) const 
		{assert (inBound (idX, idY)); return m_prgmv [idX + idY * m_uiNumMBX];}

	// Operations
	Void reassign (UInt numMBX, UInt numMBY, Int iAuxCompCount);
	Void assign (UInt imb, const CMBMode& mbmd, const CMotionVector* rgmv);

///////////////// implementation /////////////////

private:
	own CMBMode** m_ppmbmd;
	own CMotionVector** m_prgmv; // [m_uiNumMB][5]
	UInt m_uiNumMB, m_uiNumMBX, m_uiNumMBY;

	Void destroyMem ();

};

class CStatistics
{
public:

	//constructor 
	CStatistics ( Int iAuxCompCount );
  CStatistics ( const CStatistics& statSrc );
	~CStatistics ();

	//resultant
	Void print (Bool bVOPPrint = FALSE);
	CStatistics& operator = (const CStatistics& statSrc);

	//Operation
	Void operator += (const CStatistics& statSrc);
	Void reset ();
	UInt total ();
	UInt head ();
  Void setAuxCompCount(Int iAuxCompCount) {m_iAuxCompCount=iAuxCompCount;}

	//data members
	UInt nBitsHead;
	UInt nBitsY;
	UInt nBitsCr;
	UInt nBitsCb;
	UInt nBitsA;
	UInt nBitsShapeMode;
	UInt nBitsCOD;
	UInt nBitsCBPY;
	UInt nBitsMCBPC;
// GMC
	UInt nBitsMCSEL;
// ~GMC
	UInt nBitsDQUANT;
	UInt nBitsMODB;
	UInt nBitsCBPB;
	UInt nBitsMBTYPE;
	UInt nBitsIntraPred;	//intra ac/dc switch
	UInt nBitsNoDCT;		//no. of DCT in sepr m-s-t mode
	UInt nBitsCODA;
	UInt nBitsCBPA;
	UInt nBitsMODBA;
	UInt nBitsCBPBA;
	UInt nBitsStuffing;
	UInt nSkipMB;
// GMC
	UInt nMCSELMB;
// ~GMC
	UInt nInterMB;
	UInt nInter4VMB;
	UInt nIntraMB;
	UInt nDirectMB;
	UInt nForwardMB;
	UInt nBackwardMB;
	UInt nInterpolateMB;
	UInt nBitsInterlace;	// incl all interlaced info in MB header
	UInt nFieldForwardMB;
	UInt nFieldBackwardMB;
	UInt nFieldInterpolateMB;
	UInt nFieldDirectMB;
	UInt nFieldDCTMB;
	UInt nVOPs;				// VOP counter for normalizing statistics
	UInt nBitsMV;
	UInt nBitsShape;
	UInt nBitsTotal;
	Double dSNRY;
	Double dSNRU;
	Double dSNRV;
	Double* dSNRA;
	UInt nQMB;
	UInt nQp;

  CStatistics () {}
protected:

	UInt nBitsTexture;
  Int  m_iAuxCompCount;
};

class CRCMode
{
public:

	//constructor 
	CRCMode () { m_B = 0; m_FirstTime = TRUE; }; // initialise buffer
	~CRCMode () {};

	//resultant
	UInt updateQuanStepsize (UInt uiCurrQP);	// Target bits and quantization level calculation
	Bool skipNextFrame () const {return m_skipNextFrame;}

	//Operation
	Void resetSkipMode () {m_skipNextFrame = FALSE;}
	Bool skipThisFrame ();
	Void reset (UInt uiFirstFrame, UInt uiLastFrame, UInt uiTemporalRate, Int iPCount, Int *piPVopQP,
				UInt uiBufferSize, Double mad, UInt uiBitsFirstFrame, Double dFrameHz);
	Void setMad (Double mad) {m_Ep = m_Ec; m_Ec = mad;}
	Void setQc (UInt QStep) {m_Qc = QStep;}
	Void updateRCModel (UInt uiBitsTotalCurr, UInt uiBitsHeadCurr);		// Update RD model

private:

	Void RCModelEstimator (UInt nWindowSize);	// Rate Control: RD model estimator

	Double m_X1;// 1st order coefficient
	Double m_X2;// 2nd order coefficient
	UInt m_Rs;	// bit rate for sequence. e.g. 24000 bits/sec	
	UInt m_Rf;	// bits used for the first frame, e.g. 10000 bits
	UInt m_Rc;	// bits used for the current frame after encoding
	UInt m_Rp;	// bits to be removed from the buffer per picture
	Double m_Ts;// number of seconds for the sequence, e.g. 10 sec
	Double m_Ec;// mean absolute difference for the current frame after motion compensation
	Double m_Ep;// mean absolute difference for the previous frame after motion compensation
	UInt m_Qc;	// quantization level used for the current frame
	UInt m_Qp;	// quantization level used for the previous frame
	UInt m_Nr;	// number of P frames remaining for encoding
	UInt m_Nc;	// number of P frames coded
	UInt m_Ns;	// distance between encoded frames
	Int m_Rr;	// number of bits remaining for encoding this sequence 
	UInt m_T;	// target bit to be used for the current frame
	UInt m_S;	// number of bits used for encoding the previous frame
	UInt m_Hc;	// header and motion vector bits used in the current frame
	UInt m_Hp;	// header and motion vector bits used in the previous frame
	UInt m_Bs;	// buffer size 
	Int m_B;	// current buffer level
	Bool m_NrFlag;
	Bool m_skipNextFrame;						// TRUE if buffer is full
	Bool m_FirstTime;
	Int m_iPVopQP;
	UInt m_rgQp[RC_MAX_SLIDING_WINDOW];			// quantization levels for the past frames
	Double m_rgRp[RC_MAX_SLIDING_WINDOW];		// scaled encoding complexity used for the past frames;
	Bool m_rgRejected[RC_MAX_SLIDING_WINDOW];	// outliers
};

#endif	//__MODE_H


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

    Mathias Wien (wien@ient.rwth-aachen.de), RWTH Aachen / Robert BOSCH GmbH 

and also edited by
	Yoshinori Suzuki (Hitachi, Ltd.)

and also edited by
	Hideaki Kimata (NTT)

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

	sesEnc.hpp

Abstract:

	Encoder for one image session.

Revision History:
	Sept. 30, 1997: Error resilient tools added by Toshiba
	Nov.  27, 1997: Spatial Scalable tools added by SONY
	Dec.  11, 1997:	Interlaced tools added by NextLevel Systems
	Jun.  17, 1998: Complexity Estimation syntax support added by EPFL
    Feb.  16, 1999: add Quarter Sample 
                    Mathias Wien (wien@ient.rwth-aachen.de) 
	Feb.  24, 1999: GMC added by Hitachi 
	May    9, 1999:	tm5 rate control by DemoGraFX, duhoff@mediaone.net (added by mwi)
	Aug.24, 1999 : NEWPRED added by Hideaki Kimata (NTT) 
	Sep.06	1999 : RRV added by Eishi Morimatsu (Fujitsu Laboratories Ltd.) 
	Feb.12  2000 : Added Enhancement Type for OBSS by Takefumi Nagumo (Sony)
*************************************************************************/

#ifndef __SESENC_HPP_ 
#define __SESENC_HPP_

class COutBitStream;
class CEntropyEncoder;
class CEntropyEncoderSet;
class CVideoObjectEncoder;
class CEnhcBufferEncoder;

typedef struct {
		// general info
		UInt uiFrmWidth;			// frame width
		UInt uiFrmHeight;			// frame height
		Int iFirstFrm;			// first frame number
		Int iLastFrm;				// last frame number
		Bool bNot8Bit;				// NBIT
		UInt uiQuantPrecision;			// NBIT
		UInt nBits;				// NBIT
		Int iFirstVO;				// first VOP index
		Int iLastVO;				// last VOP index
		UInt* rguiVerID;			// Version ID // GMC
		Bool* rgbSpatialScalability; // spatial scalability indicator

		Int* rgiTemporalScalabilityType; // temporal scalability formation case // added by Sharp (98/02/09)
		Int* rgiEnhancementType;  // enhancement_type for scalability // added by Sharp (98/02/09)
//OBSSFIX_MODE3
		Int* rgiEnhancementTypeSpatial;
//~OBSSFIX_MODE3

		UInt **rguiRateControl;	// rate control type
		UInt **rguiBudget;		// for rate control
		// for shape coding
		AlphaUsage* rgfAlphaUsage;// alpha usage for each VOP.  0: binary; 1: 8-bit
    Int*  rgiAlphaShapeExtension; // alpha shape extension (V2)
		Bool* rgbShapeOnly;		// shape only mode
		Int* rgiBinaryAlphaTH;
		Int* rgiBinaryAlphaRR;		// refresh rate: Added for error resilient mode by Toshiba(1997-11-14)
		Int *rgiGrayToBinaryTH;
		Bool* rgbNoCrChange;
		// motion estimation part for each VOP
		UInt** rguiSearchRange;			// motion search range
		Bool** rgbOriginalForME;		// flag indicating whether use the original previous VOP for ME
		Bool** rgbAdvPredDisable;		// no advanced MC (currenly = obmc; later = obmc + 8x8)
        Bool** rgbQuarterSample;        // QuarterSample; mwi
		// START: Complexity Estimation syntax support - Marc Mongenet (EPFL) - 17 Jun 1998
		Bool ** rgbComplexityEstimationDisable;
		UInt ** rguiEstimationMethod;
		Bool ** rgbOpaque;
		Bool ** rgbTransparent;
		Bool ** rgbIntraCAE;
		Bool ** rgbInterCAE;
		Bool ** rgbNoUpdate;
		Bool ** rgbUpsampling;
		Bool ** rgbIntraBlocks;
		Bool ** rgbInterBlocks;
		Bool ** rgbInter4vBlocks;
		Bool ** rgbNotCodedBlocks;
		Bool ** rgbDCTCoefs;
		Bool ** rgbDCTLines;
		Bool ** rgbVLCSymbols;
		Bool ** rgbVLCBits;
		Bool ** rgbAPM;
		Bool ** rgbNPM;
		Bool ** rgbInterpolateMCQ;
		Bool ** rgbForwBackMCQ;
		Bool ** rgbHalfpel2;
		Bool ** rgbHalfpel4;
		Bool ** rgbSadct;
		Bool ** rgbQuarterpel;
		// END: Complexity Estimation syntax support
				// START: VOL Control Parameters
		UInt ** rguiVolControlParameters;
		UInt ** rguiChromaFormat;
		UInt ** rguiLowDelay;
		UInt ** rguiVBVParams;
		UInt ** rguiBitRate;
		UInt ** rguiVbvBufferSize;
		UInt ** rguiVbvBufferOccupany;
		// END: VOL Control Parameters
		Double** rgdFrameFrequency;	// Frame Frequency

		Bool** rgbInterlacedCoding;	// interlace coding flag
		Bool** rgbTopFieldFirst;	// top field first flag
        Bool** rgbAlternateScan;    // alternate scan flag

		// HHI Klaas Schueuer for sadct
		Bool** rgbSadctDisable;

		Int** rgiDirectModeRadius;	// direct mode search radius

		Int** rgiMVFileUsage;		// 0- not used; 1: read from motion file; 2- write to motion file
		Char*** pchMVFileName;		// Motion vector filenames
		// major syntax mode

// NEWPRED
		Bool** rgbNewpredEnable;
		Bool** rgbNewpredSegmentType;
		char*** rgcNewpredRefName;
		char*** rgcNewpredSlicePoint;
// ~NEWPRED
// RRV
  		RRVmodeStr** RRVmode; 
// ~RRV
//RESYNC_MARKER_FIX
		Bool**  rgbResyncMarkerDisable;
//~RESYNC_MARKER_FIX
		Int**	rgbVPBitTh;				// Bit threshold for video packet spacing control
		Bool**	rgbDataPartitioning;		// data partitioning
		Bool**	rgbReversibleVlc;			// reversible VLC

		// for texture coding
		Quantizer** rgfQuant; // quantizer selection; either H.263 or MPEG
		Bool** rgbLoadIntraMatrix; // load user-defined intra Q-Matrix
		Int*** rgppiIntraQuantizerMatrix; // Intra Q-Matrix
		Bool** rgbLoadInterMatrix; // load user-defined inter Q-Matrix
		Int*** rgppiInterQuantizerMatrix; // Inter Q-Matrix
		Int** rgiIntraDCSwitchingThr;		//threshold to code dc as ac when pred. is on
		Int** rgiStepI; // I-VOP quantization stepsize
		Int** rgiStepP; // P-VOP quantization stepsize
		Bool**	rgbLoadIntraMatrixAlpha;
		Int*** rgppiIntraQuantizerMatrixAlpha;
		Bool**	rgbLoadInterMatrixAlpha;
		Int*** rgppiInterQuantizerMatrixAlpha;
		Int** rgiStepIAlpha; // I-VOP quantization stepsize for Alpha
		Int** rgiStepPAlpha; // P-VOP quantization stepsize for Alpha
		Int** rgiStepBAlpha; // B-VOP quantization stepsize for Alpha
		Int** rgbNoAlphaQuantUpdate; // discouple gray quant update with tex. quant
		Int** rgiStepB; // quantization stepsize for B-VOP
		Int* rgiNumOfBbetweenPVOP;		// no of B-VOPs between P-VOPs
		Int* rgiNumOfPbetweenIVOP;		// no of P-VOPs between I-VOPs
//added to encode GOV header by SONY 980212
		Int* rgiGOVperiod;
		Bool* rgbDeblockFilterDisable; //deblocking filter disable
		Bool *rgbAllowSkippedPMBs;

		// file information
		Char* pchPrefix; // prefix name of the movie
		Char* pchBmpFiles; // bmp file directory location
		ChromType* rgfChrType; // input chrominance type. 0 - 4:4:4; 1 - 4:2:2; 0 - 4:2:0
		Char* pchOutBmpFiles; // quantized frame file directory
		Char* pchOutStrFiles; // output bitstream file
		Int* rgiTemporalRate; // temporal subsampling rate
		Int* rgiEnhcTemporalRate; // temporal subsampling rate for enhancement layer // added by Sharp (98/02/09)
		
		// statistics dumping options
		Bool* rgbDumpMB;
		Bool* rgbTrace;

		// rounding control
		Bool* rgbRoundingControlDisable; 
		Int* rgiInitialRoundingType; 

		// for sprite info
		UInt* rguiSpriteUsage; // sprite usage
		UInt* rguiWarpingAccuracy; // warping accuracy
		Int* rgNumOfPnts; // number of points for sprite; 0 for stationary and -1 for no sprite
		Char* pchSptDir; // sprite directory
		Char* pchSptPntDir; // sprite point file
        SptMode *pSpriteMode;	// sprite reconstruction mode
		Int iSpatialOption;
		UInt uiFrameWidth_SS;
		UInt uiFrameHeight_SS;
		UInt uiHor_sampling_n;
		UInt uiHor_sampling_m;
		UInt uiVer_sampling_n;
		UInt uiVer_sampling_m;
//OBSS_SAIT_991015
		UInt uiUse_ref_shape;			
		UInt uiUse_ref_texture;			
		UInt uiHor_sampling_n_shape;	
		UInt uiHor_sampling_m_shape;	
		UInt uiVer_sampling_n_shape;	
		UInt uiVer_sampling_m_shape;
		
		Bool bCodeSequenceHead;
		UInt uiProfileAndLevel;
//~OBSS_SAIT_991015
} SessionEncoderArgList;

class CSessionEncoder
{
	friend class CEnhcBufferEncoder;
public:
	// Constructors
	~CSessionEncoder ();
	CSessionEncoder (SessionEncoderArgList *pArgs);

	// Attributes
	UInt compressedSize () const {return m_uCmpSize;} // in bits

	// Operations
	Void encode ();
///////////////// implementation /////////////////

protected:
	CRct m_rctOrg;
	CRct m_rctOrgSpatialEnhn;
	Int m_iFirstFrame;				// first frame number
	Int m_iLastFrame;					// last frame number
	Int m_iNumFrame;					// number of frames
	Int m_iFirstVO;					// first VO index
	Int m_iLastVO;					// last VO index
	Int m_iNumVO;					//  number of VO's

	const Bool* m_rgbSpatialScalability;	//spatial scalability on/off
	UInt** m_rguiRateControl;		// rate control type
	UInt** m_rguiBudget;			// for rate control
	own VOLMode* m_rgvolmd [2];		// VOL modes
	own VOPMode* m_rgvopmd [2];		// VOP modes
	const CVOPU8YUVBA* m_rgpvopcPrevDisp [2];	// pointers to previously displayed vops
	
	// file information
	const Char* m_pchPrefix; // prefix name of the movie
	const Char* m_pchBmpFiles; // bmp file directory location
	const ChromType* m_rgfChrType; // input chrominance type. 0 - 4:4:4, 1 - 4:2:2, 0 - 4:2:0
	const Char* m_pchReconYUVDir; // quantized frame file directory
	const Char* m_pchOutStrFiles; // output bitstream file
	UInt m_uCmpSize; // in bits
	Bool m_bTexturePerVOP; // Bool for whether there are texture data for each VOP
	Bool m_bAlphaPerVOP; // Bool for whether there are alpha data for each VOP

	// for sprite info
	const Char* m_pchSptDir; // sprite directory
	const Char* m_pchSptPntDir; // sprite point file
	const UInt* m_rguiSpriteUsage; // sprite usage
	const UInt* m_rguiWarpingAccuracy; // warping accuracy
	const Int* m_rgNumOfPnts; // number of points for each VOP
	own CSiteD** m_ppstSrc; // source sites, [numVOP] [numPnt]
	own CSiteD*** m_pppstDst; // destination sites, [numVOP] [numFrm] [numPnt]
	// low latency stuff
	SptMode m_SptMode;  // sprite reconstruction mode : 0 -- basic sprite , 1 -- Object piece only, 2 -- Update piece only, 3 -- intermingled	
	CRct m_rctFrame;	// to save the frame rectangle

	Int** m_rgiMVFileUsage;		// MV file usage [iLayer][iObj]
	Char*** m_pchMVFileName;	// MV file names [iLayer][iObj]

	// temporal scalability
	const Int* m_rgiTemporalScalabilityType; // added by Sharp (98/2/12)
	Bool m_bPrevObjectExists;

  Void getInputFiles (FILE*& pfYuvSrc, FILE*& pfAlpSrc, 
                      FILE** ppfAuxSrc, // MAC (SB) 24-Nov-99
                      FILE*& pfYuvSrcSpatialEnhn,
                      FILE*& pfAlpSrcSpatialEnhn,//OBSS_SAIT_991015
                      FILE* rgpfReconYUV [], FILE* rgpfReconSeg [], 
                      FILE** rgpfReconAux [],
					  std::ofstream* rgpostrm [], std::ofstream* rgpostrmTrace [],
                      PixelC& pxlcObjColor, Int iobj, const VOLMode& volmd, const VOLMode& volmd_enhn);
  Void initVOEncoder (CVideoObjectEncoder** rgpvoenc, Int iobj, std::ofstream* rgpostrmTrace []);
	Bool loadDataSpriteCheck(UInt iVOrelative,UInt iFrame, FILE* pfYuvSrc, FILE* pfSegSrc,
                           FILE** ppfAuxSrc,
					                 PixelC pxlcObjColor, CVOPU8YUVBA* pvopcDst, const VOLMode& volmd);

	Bool loadData (UInt iFrame, FILE* pfYuvSrc, FILE* pfSegSrc, 
                 FILE** ppfAuxSrc, PixelC pxlcObjColor, 
					       CVOPU8YUVBA* pvopcDst, CRct& rctOrg,const VOLMode& volmd);
//OBSS_SAIT_991015
	//for load enhancement layer shape
	Bool loadDataSpriteCheck(UInt iVOrelative,UInt iFrame, FILE* pfYuvSrc, FILE* pfSegSrc,
					PixelC pxlcObjColor, CVOPU8YUVBA* pvopcDst, const VOLMode& volmd,const VOLMode& volmd_enhn);
	//for load enhancement layer shape
	Bool loadData (UInt iFrame, FILE* pfYuvSrc, FILE* pfSegSrc, PixelC pxlcObjColor, 
					CVOPU8YUVBA* pvopcDst, CRct& rctOrg,const VOLMode& volmd,const VOLMode& volmd_enhn);
//~OBSS_SAIT_991015
	Void dumpData (FILE* pfYuvDst, FILE* pfSegDst, FILE** ppfAuxDst, Int iLayer,
		const CVOPU8YUVBA* pvopcSrc, const CRct& rctOrg, const VOLMode& volmd, const Bool bInterlace);
	Void dumpDataNonCoded (FILE* pfYUV, FILE* pfSeg, FILE ** ppfAux, Int iLayer, CRct& rct, const VOLMode& volmd);

	Void createReconDir (UInt idx) const;
	Void createCmpDir (UInt idx) const;
	Void encodeVideoObject(	Bool bObjectExists,
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
              FILE** rgpfReconAux[],
							PixelC pxlcObjColor,
							CVideoObjectEncoder** rgpvoenc,
							const VOLMode& volmd,
							std::ofstream* rgpostrm[],
							const CVOPU8YUVBA* pvopcBaseQuant = NULL);

// begin: added by Sharp (98/2/12)
	Void updateRefForTPS (CVideoObjectEncoder* pvopc,
		CEnhcBufferEncoder* BufP1, CEnhcBufferEncoder* BufP2, CEnhcBufferEncoder* BufB1, CEnhcBufferEncoder* BufB2, CEnhcBufferEncoder* BufE,
		Int bNoNextVOP, Int iVOrelative, Int iEcount, Int ibFrameWithRate, Int ieFrame, Bool bupdateForLastLoop);
	Void dumpDataOneFrame (UInt iFrame, Int iobj, const CVOPU8YUVBA* pvopcSrc, const VOLMode& volmd);
	Void initVObfShape (CVideoObjectEncoder** rgpvobfShape, Int iobj, 
			    VOLMode& volmd_back, VOPMode& vopmd_back, VOLMode& volmd_forw, VOPMode& vopmd_forw);
	Void encodeEnhanceVideoObject(Bool bObjectExists,
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
								  std::ofstream* rgpostrm[],
                                  CEnhcBufferEncoder& BufP1,
                                  CEnhcBufferEncoder& BufP2,
                                  CEnhcBufferEncoder& BufB1,
                                  CEnhcBufferEncoder& BufB2,
                                  CEnhcBufferEncoder& BufE
                                  );
// end: added by Sharp (98/2/12)
	// sprite
	Void readPntFile (UInt iobj);
	Void loadSpt (UInt iobj, CVOPU8YUVBA* pvopcDst);
	CRct findBoundBoxInAlpha (UInt ifr, UInt iobj);
	CRct findSptRct (UInt iobj);
};

#endif // __SESENC_HPP_

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
	May 9, 1999:	tm5 rate control by DemoGraFX, duhoff@mediaone.net

*************************************************************************/

#include "stdlib.h"
#include "stdio.h"
#include "math.h"
#include "fstream.h"
#include "iostream.h"

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
#define ENHN_SUB_CMPFILE "%s\\%2.2d\\%s_e.cmp"
#define ENHN_SUB_TRCFILE "%s\\%2.2d\\%s_e.trc"
#define ENHN_SUB_YUVFILE "%s\\%2.2d\\%s_e.yuv"
#define ENHN_SUB_SEGFILE "%s\\%2.2d\\%s_e.seg"
#define SUB_VDLFILE "%s\\%2.2d\\%s.spt"	//for sprite i/o
#define SUB_PNTFILE "%s\\%2.2d\\%s.pnt"	//for sprite i/o
#define ROOT_YUVFILE "%s\\%s.yuv"
#define ROOT_SEGFILE "%s\\%s.seg"
#define IOS_BINARY ios::binary
#define MKDIR "mkdir %s\\%2.2d"
#else
#define SUB_CMPFILE "%s/%2.2d/%s.cmp"
#define SUB_TRCFILE "%s/%2.2d/%s.trc"
#define SUB_YUVFILE	"%s/%2.2d/%s.yuv"
#define SUB_SEGFILE	"%s/%2.2d/%s.seg"
#define ENHN_SUB_CMPFILE "%s/%2.2d/%s_e.cmp"
#define ENHN_SUB_TRCFILE "%s/%2.2d/%s_e.trc"
#define ENHN_SUB_YUVFILE "%s/%2.2d/%s_e.yuv"
#define ENHN_SUB_SEGFILE "%s/%2.2d/%s_e.seg"
#define SUB_VDLFILE "%s/%2.2d/%s.vdl"	//for sprite i/o
#define SUB_PNTFILE "%s/%2.2d/%s.pnt"	//for sprite i/o
#define ROOT_YUVFILE "%s/%s.yuv"
#define ROOT_SEGFILE "%s/%s.seg"
//#define IOS_BINARY ios::bin
#define IOS_BINARY 0
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


CSessionEncoder::CSessionEncoder (
		// general info
		UInt uiFrmWidth, // frame width
		UInt uiFrmHeight, // frame height
		Int iFirstFrm, // first frame number
		Int iLastFrm, // last frame number
		Bool bNot8Bit, // NBIT: not 8-bit flag
		UInt uiQuantPrecision, // NBIT: quant precision
		UInt nBits, // NBIT: number of bits per pixel
		Int iFirstVO, // first VOP index
		Int iLastVO, // last VOP index
		const Bool* rgbSpatialScalability, // spatial scalability indicator
		const Int* rgiTemporalScalabilityType, // temporal scalability formation case // added by Sharp (98/02/09)
		const Int* rgiEnhancementType,	// enhancement_type for scalability // added by Sharp (98/02/09)
		UInt* rguiRateControl [], // rate control type
		UInt* rguiBudget [], // for rate control
		// for shape coding
		const AlphaUsage* rgfAlphaUsage, // alpha usage for each VOP.  0: binary, 1: 8-bit
		const Bool* rgbShapeOnly,		 // disable texture motion coding
		const Int* rgiBinaryAlphaTH,	 // threhold is shape size conversion
		const Int* rgiBinaryAlphaRR,	 // refrash rate for binary shape coding:	Added for error resilient mode by Toshiba(1997-11-14)
		const Bool* rgbNoCrChange,		 // no change of cr from mb to mb

		// motion estimation part for each VOP
		UInt** rguiSearchRange,  // motion search range
		Bool** rgbOriginalForME, // flag indicating whether use the original previous VOP for ME
		Bool** rgbAdvPredDisable,//no advanced MC (currenly = obmc, later = obmc + 8x8)
		// START: Complexity Estimation syntax support - Marc Mongenet (EPFL) - 17 Jun 1998
		Bool ** rgbComplexityEstimationDisable,
		Bool ** rgbOpaque,
		Bool ** rgbTransparent,
		Bool ** rgbIntraCAE,
		Bool ** rgbInterCAE,
		Bool ** rgbNoUpdate,
		Bool ** rgbUpsampling,
		Bool ** rgbIntraBlocks,
		Bool ** rgbInterBlocks,
		Bool ** rgbInter4vBlocks,
		Bool ** rgbNotCodedBlocks,
		Bool ** rgbDCTCoefs,
		Bool ** rgbDCTLines,
		Bool ** rgbVLCSymbols,
		Bool ** rgbVLCBits,
		Bool ** rgbAPM,
		Bool ** rgbNPM,
		Bool ** rgbInterpolateMCQ,
		Bool ** rgbForwBackMCQ,
		Bool ** rgbHalfpel2,
		Bool ** rgbHalfpel4,
		// END: Complexity Estimation syntax support

		// START: VOL Control Parameters
		UInt ** rguiVolControlParameters,
		UInt ** rguiChromaFormat,
		UInt ** rguiLowDelay,
		UInt ** rguiVBVParams,
		UInt ** rguiBitRate,
		UInt ** rguiVbvBufferSize,
		UInt ** rguiVbvBufferOccupany,
		// END: VOL Control Parameters

		Double** rgdFrameFrequency,

		// interlace coding
		Bool** rgbInterlacedCoding, // interlace flag
		Bool** rgbTopFieldFirst,    // top field first flag
        Bool** rgbAlternateScan,    // alternate scan flag

		Int** rgiDirectModeRadius,	// Direct mode search radius

		// motion vector file I/O
		Int** rgiMVFileUsage,
		Char*** pchMVFileName,
		// major syntax mode
		Int**	rgbVPBitTh,			// Bit threshold for video packet spacing control
		Bool** rgbDataPartitioning,  //data partitioning
		Bool** rgbReversibleVlc, //reversible VLC

		// for texture coding
		Quantizer** rgfQuant, // quantizer selection, either H.263 or MPEG
		Bool** rgbLoadIntraMatrix, // load user-defined intra Q-Matrix
		Int*** rgppiIntraQuantizerMatrix, // Intra Q-Matrix
		Bool** rgbLoadInterMatrix, // load user-defined inter Q-Matrix
		Int*** rgppiInterQuantizerMatrix, // Inter Q-Matrix
		Int** rgiIntraDCSwitchingThr,		//threshold to code dc as ac when pred. is on
		Int** rgiStepI, // I-VOP quantization stepsize
		Int** rgiStepP, // P-VOP quantization stepsize
		Bool**	rgbLoadIntraMatrixAlpha,
		Int*** rgppiIntraQuantizerMatrixAlpha,
		Bool**	rgbLoadInterMatrixAlpha,
		Int*** rgppiInterQuantizerMatrixAlpha,
		Int** rgiStepIAlpha, // I-VOP quantization stepsize for Alpha
		Int** rgiStepPAlpha, // P-VOP quantization stepsize for Alpha
		Int** rgiStepBAlpha, // P-VOP quantization stepsize for Alpha
		Int** rgbNoAlphaQuantUpdate, // discouple gray quant update with tex. quant
		Int** rgiStepB, // quantization stepsize for B-VOP
		const Int* rgiNumOfBbetweenPVOP,		// no of B-VOPs between P-VOPs
		const Int* rgiNumOfPbetweenIVOP,		// no of P-VOPs between I-VOPs
//added to encode GOVheader by SONY 980212
		const Int* rgiGOVperiod,
//980212
		const Bool* rgbDeblockFilterDisable, //deblocing filter disable
		const Bool *rgbAllowSkippedPMBs,

		// file information
		const Char* pchPrefix, // prefix name of the movie
		const Char* pchBmpFiles, // bmp file directory location
		const ChromType* rgfChrType, // input chrominance type. 0 - 4:4:4, 1 - 4:2:2, 0 - 4:2:0
		const Char* pchOutBmpFiles, // quantized frame file directory
		const Char* pchOutStrFiles, // output bitstream file
		const Int* rgiTemporalRate, // temporal subsampling rate
		const Int* rgiEnhnTemporalRate, // temporal subsampling rate // added by Sharp (98/02/09)
		
		// statistics dumping options
		const Bool* rgbDumpMB,
		const Bool* rgbTrace,

		// rounding control
		const Bool* rgbRoundingControlDisable,
		const Int* rgiInitialRoundingType,
		// for sprite info
		const UInt* rguiSpriteUsage, // sprite usage
		const UInt* rguiWarpingAccuracy, // warping accuracy
		const Int* rgNumOfPnts, // number of points for sprite, 0 for stationary and -1 for no sprite
		const Char* pchSptDir, // sprite directory
		const Char* pchSptPntDir, // sprite point file
		SptMode *pSpriteMode,		// sprite reconstruction mode, i.e. Low-latency-sprite-enable 
		Int iSpatialOption,
		UInt uiFrmWidth_SS,
		UInt uiFrmHeight_SS,
		UInt uiHor_sampling_n,
		UInt uiHor_sampling_m,
		UInt uiVer_sampling_n,
		UInt uiVer_sampling_m
) :
	m_iFirstFrame (iFirstFrm), m_iLastFrame (iLastFrm),
	m_iFirstVO (iFirstVO), m_iLastVO (iLastVO),
	m_rgbSpatialScalability (rgbSpatialScalability),
	m_rguiRateControl (rguiRateControl), 
	m_rguiBudget (rguiBudget),
	m_pchPrefix (pchPrefix), m_pchBmpFiles (pchBmpFiles),
	m_rgfChrType (rgfChrType), m_pchReconYUVDir (pchOutBmpFiles), 
	m_pchOutStrFiles (pchOutStrFiles),
	m_pchSptDir (pchSptDir), 
	m_pchSptPntDir (pchSptPntDir),
	m_rguiSpriteUsage (rguiSpriteUsage), 
	m_rguiWarpingAccuracy (rguiWarpingAccuracy), 
	m_rgNumOfPnts (rgNumOfPnts), 
	m_rgiTemporalScalabilityType (rgiTemporalScalabilityType)
{
	// data preparation
	m_iNumFrame = m_iLastFrame - m_iFirstFrame + 1;
	assert (m_iNumFrame > 0);
	m_rctOrg = CRct (0, 0, uiFrmWidth, uiFrmHeight);
	if( uiFrmWidth_SS !=0 && uiFrmHeight_SS!=0)
		m_rctOrgSpatialEnhn = CRct(0, 0, uiFrmWidth_SS, uiFrmHeight_SS);
	assert (uiFrmWidth <= 8192 && uiFrmHeight <= 8192); //2^13 maximum size
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
			// NBIT
			m_rgvolmd [iLayer][iVO].bNot8Bit 		= bNot8Bit;
			m_rgvolmd [iLayer][iVO].uiQuantPrecision 	= uiQuantPrecision;
			m_rgvolmd [iLayer][iVO].nBits 			= nBits;

			m_rgvolmd [iLayer][iVO].volType			= (VOLtype) iLayer;
			m_rgvolmd [iLayer][iVO].fAUsage			= rgfAlphaUsage [iVO];
			m_rgvolmd [iLayer][iVO].bShapeOnly		= rgbShapeOnly [iVO];
			m_rgvolmd [iLayer][iVO].iBinaryAlphaTH	= rgiBinaryAlphaTH [iVO] * 16;  //magic no. from the vm
			m_rgvolmd [iLayer][iVO].iBinaryAlphaRR	= (rgiBinaryAlphaRR [iVO]<0)?-1:(rgiBinaryAlphaRR [iVO] +1)*(rgiNumOfBbetweenPVOP [iVO] +1)*rgiTemporalRate [iVO];  // Binary shape refresh rate: Added for error resilient mode by Toshiba(1997-11-14): Modified (1998-1-16)
			m_rgvolmd [iLayer][iVO].bNoCrChange		= rgbNoCrChange [iVO];
			m_rgvolmd [iLayer][iVO].bOriginalForME	= rgbOriginalForME [iLayer][iVO];
			m_rgvolmd [iLayer][iVO].bAdvPredDisable = rgbAdvPredDisable [iLayer][iVO];
			// START: Complexity Estimation syntax support - Marc Mongenet (EPFL) - 17 Jun 1998
			m_rgvolmd [iLayer][iVO].bComplexityEstimationDisable = rgbComplexityEstimationDisable [iLayer][iVO];
			m_rgvolmd [iLayer][iVO].bOpaque = rgbOpaque [iLayer][iVO];
			m_rgvolmd [iLayer][iVO].bTransparent = rgbTransparent [iLayer][iVO];
			m_rgvolmd [iLayer][iVO].bIntraCAE = rgbIntraCAE [iLayer][iVO];
			m_rgvolmd [iLayer][iVO].bInterCAE = rgbInterCAE [iLayer][iVO];
			m_rgvolmd [iLayer][iVO].bNoUpdate = rgbNoUpdate [iLayer][iVO];
			m_rgvolmd [iLayer][iVO].bUpsampling = rgbUpsampling [iLayer][iVO];
			m_rgvolmd [iLayer][iVO].bIntraBlocks = rgbIntraBlocks [iLayer][iVO];
			m_rgvolmd [iLayer][iVO].bInterBlocks = rgbInterBlocks [iLayer][iVO];
			m_rgvolmd [iLayer][iVO].bInter4vBlocks = rgbInter4vBlocks [iLayer][iVO];
			m_rgvolmd [iLayer][iVO].bNotCodedBlocks = rgbNotCodedBlocks [iLayer][iVO];
			m_rgvolmd [iLayer][iVO].bDCTCoefs = rgbDCTCoefs [iLayer][iVO];
			m_rgvolmd [iLayer][iVO].bDCTLines = rgbDCTLines [iLayer][iVO];
			m_rgvolmd [iLayer][iVO].bVLCSymbols = rgbVLCSymbols [iLayer][iVO];
			m_rgvolmd [iLayer][iVO].bVLCBits = rgbVLCBits [iLayer][iVO];
			m_rgvolmd [iLayer][iVO].bAPM = rgbAPM [iLayer][iVO];
			m_rgvolmd [iLayer][iVO].bNPM = rgbNPM [iLayer][iVO];
			m_rgvolmd [iLayer][iVO].bInterpolateMCQ = rgbInterpolateMCQ [iLayer][iVO];
			m_rgvolmd [iLayer][iVO].bForwBackMCQ = rgbForwBackMCQ [iLayer][iVO];
			m_rgvolmd [iLayer][iVO].bHalfpel2 = rgbHalfpel2 [iLayer][iVO];
			m_rgvolmd [iLayer][iVO].bHalfpel4 = rgbHalfpel4 [iLayer][iVO];
			m_rgvolmd [iLayer][iVO].bAllowSkippedPMBs = rgbAllowSkippedPMBs [iVO];

			// END: Complexity Estimation syntax support
			// START: Vol Control Parameters
			if(rguiVolControlParameters[0]!=NULL)
			{
				m_rgvolmd [iLayer][iVO].uiVolControlParameters = rguiVolControlParameters [iLayer][iVO];
				m_rgvolmd [iLayer][iVO].uiChromaFormat = rguiChromaFormat [iLayer][iVO];
				m_rgvolmd [iLayer][iVO].uiLowDelay = rguiLowDelay [iLayer][iVO];
				m_rgvolmd [iLayer][iVO].uiVBVParams = rguiVBVParams [iLayer][iVO];
				m_rgvolmd [iLayer][iVO].uiBitRate = rguiBitRate [iLayer][iVO];
				m_rgvolmd [iLayer][iVO].uiVbvBufferSize = rguiVbvBufferSize [iLayer][iVO];
				m_rgvolmd [iLayer][iVO].uiVbvBufferOccupany = rguiVbvBufferOccupany [iLayer][iVO];
			}
			else
				m_rgvolmd [iLayer][iVO].uiVolControlParameters = 0;
			// END: Vol Control Parameters
			m_rgvolmd [iLayer][iVO].bRoundingControlDisable	= (iLayer== BASE_LAYER) ? rgbRoundingControlDisable[iVO] : 0;
			m_rgvolmd [iLayer][iVO].iInitialRoundingType = (iLayer== BASE_LAYER) ? rgiInitialRoundingType[iVO] : 0;
			m_rgvolmd [iLayer][iVO].bVPBitTh		= rgbVPBitTh[iLayer][iVO];
			m_rgvolmd [iLayer][iVO].bDataPartitioning = rgbDataPartitioning [iLayer][iVO];
			m_rgvolmd [iLayer][iVO].bReversibleVlc  = rgbReversibleVlc [iLayer][iVO];
			m_rgvolmd [iLayer][iVO].fQuantizer		= rgfQuant [iLayer][iVO];
			m_rgvolmd [iLayer][iVO].bNoGrayQuantUpdate 
													= rgbNoAlphaQuantUpdate [iLayer][iVO];
			m_rgvolmd [iLayer][iVO].dFrameHz		= rgdFrameFrequency [iLayer][iVO];
			m_rgvolmd [iLayer][iVO].iMVRadiusPerFrameAwayFromRef = rguiSearchRange [iLayer][iVO];
			m_rgvopmd [iLayer][iVO].iSearchRangeForward	= rguiSearchRange [iLayer][iVO];
			m_rgvopmd [iLayer][iVO].iSearchRangeBackward= rguiSearchRange [iLayer][iVO];
			m_rgvopmd [iLayer][iVO].intStepI		= rgiStepI [iLayer][iVO];
			m_rgvopmd [iLayer][iVO].intStep			= rgiStepP [iLayer][iVO];
			m_rgvopmd [iLayer][iVO].intStepB		= rgiStepB [iLayer][iVO];
			m_rgvopmd [iLayer][iVO].intStepIAlpha	= rgiStepIAlpha [iLayer][iVO];
			m_rgvopmd [iLayer][iVO].intStepPAlpha	= rgiStepPAlpha [iLayer][iVO];
			m_rgvopmd [iLayer][iVO].intStepBAlpha	= rgiStepBAlpha [iLayer][iVO];
			m_rgvopmd [iLayer][iVO].iIntraDcSwitchThr 
													= rgiIntraDCSwitchingThr [iLayer][iVO];
			m_rgvopmd [iLayer][iVO].bInterlace		= rgbInterlacedCoding [iLayer][iVO];
			m_rgvopmd [iLayer][iVO].bTopFieldFirst	= rgbTopFieldFirst [iLayer][iVO];
			m_rgvopmd [iLayer][iVO].bAlternateScan	= rgbAlternateScan [iLayer][iVO];
			m_rgvopmd [iLayer][iVO].iDirectModeRadius = rgiDirectModeRadius [iLayer][iVO];
			m_rgvolmd [iLayer][iVO].bLoadIntraMatrix= rgbLoadIntraMatrix [iLayer][iVO];
			m_rgvolmd [iLayer][iVO].bLoadInterMatrix= rgbLoadInterMatrix [iLayer][iVO];
			m_rgvolmd [iLayer][iVO].bLoadIntraMatrixAlpha 
													= rgbLoadIntraMatrixAlpha [iLayer][iVO];
			m_rgvolmd [iLayer][iVO].bLoadInterMatrixAlpha 
													= rgbLoadInterMatrixAlpha [iLayer][iVO];
			memcpy (m_rgvolmd [iLayer][iVO].rgiIntraQuantizerMatrix,		rgppiIntraQuantizerMatrix [iLayer][iVO], BLOCK_SQUARE_SIZE * sizeof (Int));
			memcpy (m_rgvolmd [iLayer][iVO].rgiInterQuantizerMatrix,		rgppiInterQuantizerMatrix [iLayer][iVO], BLOCK_SQUARE_SIZE * sizeof (Int));
			memcpy (m_rgvolmd [iLayer][iVO].rgiIntraQuantizerMatrixAlpha,	rgppiIntraQuantizerMatrixAlpha [iLayer][iVO], BLOCK_SQUARE_SIZE * sizeof (Int));
			memcpy (m_rgvolmd [iLayer][iVO].rgiInterQuantizerMatrixAlpha,	rgppiInterQuantizerMatrixAlpha [iLayer][iVO], BLOCK_SQUARE_SIZE * sizeof (Int));

			m_rgvolmd [iLayer][iVO].bDeblockFilterDisable 
													= rgbDeblockFilterDisable [iVO];
			m_rgvolmd [iLayer][iVO].iBbetweenP	= rgiNumOfBbetweenPVOP [iVO];
			m_rgvolmd [iLayer][iVO].iPbetweenI	= rgiNumOfPbetweenIVOP [iVO];
			m_rgvolmd [iLayer][iVO].iGOVperiod = rgiGOVperiod [iVO]; // count of o/p frames between govs
			// set up temporal sampling rate and clock rate from source frame rate.
			m_rgvolmd [iLayer][iVO].iTemporalRate	= rgiTemporalRate [iVO]; // frames to skip
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

			m_rgvolmd [iLayer][iVO].bDumpMB			= rgbDumpMB [iVO];
			m_rgvolmd [iLayer][iVO].bTrace			= rgbTrace [iVO];
			m_rgvolmd [iLayer][iVO].bTemporalScalability = FALSE;
			if(m_rgvolmd[iLayer][iVO].volType == ENHN_LAYER && m_rgbSpatialScalability [iVO] && rgiTemporalRate[iVO] == rgiEnhnTemporalRate[iVO]){
				// modified by Sharp (98/02/09)
//#ifdef _Scalable_SONY_
				m_rgvolmd[iLayer][iVO].iHierarchyType = 0; //iHierarchyType 0 means spatial scalability
//#endif _Scalable_SONY_
				m_rgvolmd[iLayer][iVO].iEnhnType = 0; // Spatial Scalable Enhancement layer shuold be set to Rectangular shape
				m_rgvolmd[iLayer][iVO].iSpatialOption = iSpatialOption; // This Option is for Spatial Scalable
				m_rgvolmd[iLayer][iVO].iver_sampling_factor_n = uiVer_sampling_n;
				m_rgvolmd[iLayer][iVO].iver_sampling_factor_m = uiVer_sampling_m;
				m_rgvolmd[iLayer][iVO].ihor_sampling_factor_n = uiHor_sampling_n;
				m_rgvolmd[iLayer][iVO].ihor_sampling_factor_m = uiHor_sampling_m;
			} 
// begin: added by Sharp (98/2/12)
			else if (m_rgbSpatialScalability [iVO] && rgiTemporalRate[iVO] != rgiEnhnTemporalRate[iVO]){
				m_rgvolmd [iLayer][iVO].bTemporalScalability = TRUE;
//#ifdef _Scalable_SONY_
				m_rgvolmd[iLayer][iVO].iHierarchyType = 1; //iHierarchyType 1 means temporal scalability
//#endif _Scalable_SONY_
				m_rgvolmd [iLayer][iVO].iEnhnType	= rgiEnhancementType [iVO];
				if ( rgiEnhancementType[iVO] != 0 ) // modified by Sharp (98/3/24)
					m_rgvolmd [BASE_LAYER][iVO].fAUsage = RECTANGLE;
				if ( iLayer == ENHN_LAYER ) {
					m_rgvolmd [ENHN_LAYER][iVO].iTemporalRate	= rgiEnhnTemporalRate [iVO];
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
		if (m_rgNumOfPnts [iVO - m_iFirstVO] > 0)
				readPntFile (iVO);

    m_SptMode =	pSpriteMode[0];
	m_rgiMVFileUsage = rgiMVFileUsage;
	m_pchMVFileName  = pchMVFileName;
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

	for (iVO = m_iFirstVO; iVO <= (Int) m_iLastVO; iVO++, iVOrelative++) {
		ofstream* rgpostrm [2];
		ofstream* rgpostrmTrace [2];
		PixelC pxlcObjColor;
		VOLMode volmd = m_rgvolmd [BASE_LAYER] [iVOrelative];
		VOLMode volmd_enhn = m_rgvolmd [ENHN_LAYER] [iVOrelative]; // added by Sharp (98/2/10)
		getInputFiles (	pfYuvSrc, pfSegSrc, pfYuvSrcSpatialEnhn,
						rgpfReconYUV, rgpfReconSeg, 
						rgpostrm, rgpostrmTrace, pxlcObjColor, iVO, volmd, volmd_enhn); // modified by Sharp(98/2/10)
		CRct rctOrg;
		CVideoObjectEncoder* rgpvoenc [2];
		if (m_rguiSpriteUsage [iVOrelative] == 1) {
			// change m_rctOrg to sprite size
			rctOrg = m_rctOrg;
			m_rctFrame = m_rctOrg;
			m_rctOrg = findSptRct (iVO);
		}

		initVOEncoder (rgpvoenc, iVO, rgpostrmTrace);
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
			assert (volmd.iTemporalRate==1);
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
				(UInt)((m_rguiBudget [BASE_LAYER] [iVOrelative] * m_rgvolmd [BASE_LAYER][iVOrelative].dFrameHz)
				 / (m_iLastFrame - m_iFirstFrame + 1)),
				m_rgvolmd [BASE_LAYER][iVOrelative].dFrameHz
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
			if(bCachedRefDump && m_rguiSpriteUsage [iVOrelative] == 0)
			{
				bCachedRefDump = FALSE;
				// last ref frame needs to be output
#ifndef __OUT_ONE_FRAME_

				if ( bCachedRefCoded )
					dumpData (rgpfReconYUV [BASE_LAYER], rgpfReconSeg [BASE_LAYER], rgpvoenc[BASE_LAYER] ->pvopcRefQLater(), m_rctOrg, volmd);
				else
					dumpNonCodedFrame(rgpfReconYUV [BASE_LAYER], rgpfReconSeg [BASE_LAYER], m_rctOrg, volmd.nBits);

				if (m_rgbSpatialScalability [iVOrelative] && !bTemporalScalability)
					dumpData (rgpfReconYUV [ENHN_LAYER], rgpfReconSeg [ENHN_LAYER], rgpvoenc[ENHN_LAYER] ->pvopcRefQLater(), m_rctOrgSpatialEnhn, volmd);
// begin: deleted by Sharp (98/11/11)
// #else
// 					dumpDataOneFrame (iFrame, iVO, rgpvoenc[BASE_LAYER] ->pvopcRefQLater(), volmd); // save one frame
// 					if (m_rgbSpatialScalability [iVOrelative] && !bTemporalScalability)
// 						dumpDataOneFrame (iFrame, iVO, rgpvoenc[BASE_LAYER] ->pvopcRefQLater(), volmd); // save one frame
// end: deleted by Sharp (98/11/11)
#endif
			}

			int iGOPperiod = (volmd.iPbetweenI + 1) * (volmd.iBbetweenP + 1);
			if (rgpvoenc [BASE_LAYER]->m_uiRateControl >= RC_TM5 && iGOPperiod != 0 
				&& ((iRefFrame-m_iFirstFrame) % (iGOPperiod * volmd.iTemporalRate)) == 0)
			{
				Int nppic, npic = (m_iLastFrame - iRefFrame + 1) / volmd.iTemporalRate;
				if (iRefFrame == m_iFirstFrame) {
					if (npic > (iGOPperiod - volmd.iBbetweenP))
						npic = iGOPperiod - volmd.iBbetweenP;
				} else {
					npic += volmd.iBbetweenP;
					if (npic > iGOPperiod)
						npic = iGOPperiod;
				}
				nppic = (npic + volmd.iBbetweenP) / (volmd.iBbetweenP + 1) - 1;
				rgpvoenc [BASE_LAYER] -> m_tm5rc.tm5rc_init_GOP(nppic, npic - nppic - 1); //  np, nb remain
			}

			// encode non-coded frames or initial IVOPs
			// we always dump these out
			bObjectExists = loadDataSpriteCheck (iVOrelative,iRefFrame, pfYuvSrc, pfSegSrc, pxlcObjColor, rgpvoenc [BASE_LAYER]->m_pvopcOrig, volmd);
			encodeVideoObject(bObjectExists, bObjectExists, iRefFrame, IVOP, DUMP_CURR,
							  iVO, iVOrelative, BASE_LAYER, 
							  pfYuvSrc,pfSegSrc,rgpfReconYUV,rgpfReconSeg,
							  pxlcObjColor, rgpvoenc, volmd, rgpostrm);

			if (m_rgbSpatialScalability [iVOrelative] && !bTemporalScalability ) { // modified by Sharp (98/2/12)
				pvopcBaseQuant = rgpvoenc [BASE_LAYER]->pvopcReconCurr ();
				encodeVideoObject (bObjectExists, bObjectExists, iRefFrame, PVOP, DUMP_CURR,
								   iVO, iVOrelative, ENHN_LAYER,
								   pfYuvSrcSpatialEnhn, pfSegSrc, rgpfReconYUV, rgpfReconSeg,
								   pxlcObjColor, rgpvoenc, volmd, rgpostrm,
								   pvopcBaseQuant);
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
						bObjectExists = loadDataSpriteCheck(iVOrelative,iSearchFrame, pfYuvSrc, pfSegSrc, pxlcObjColor, rgpvoenc [BASE_LAYER]->m_pvopcOrig, volmd);
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
				//Bool bCachedRefDumpSaveForSpatialScalability = bCachedRefDump;
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
					if (rgpvoenc [BASE_LAYER]->m_uiRateControl >= RC_TM5 && iGOPperiod != 0 
						&& ((iSearchFrame-m_iFirstFrame) % (iGOPperiod * volmd.iTemporalRate)) == 0)
					{
						Int nppic, npic = (m_iLastFrame - iSearchFrame + 1) / volmd.iTemporalRate;
						if (iRefFrame == m_iFirstFrame) {
							if (npic > (iGOPperiod - volmd.iBbetweenP))
								npic = iGOPperiod - volmd.iBbetweenP;
						} else {
							npic += volmd.iBbetweenP;
							if (npic > iGOPperiod)
								npic = iGOPperiod;
						}
						nppic = (npic + volmd.iBbetweenP) / (volmd.iBbetweenP + 1) - 1;
						rgpvoenc [BASE_LAYER] -> m_tm5rc.tm5rc_init_GOP(nppic, npic - nppic - 1); //  np, nb remain
					}

					// encode IVOP

					encodeVideoObject(bObjectExists, bPrevObjectExists, iSearchFrame, IVOP, bCachedRefDump ? DUMP_PREV : DUMP_NONE, // modified by Sharp (99/1/27)
									  iVO, iVOrelative, BASE_LAYER, 
									  pfYuvSrc, pfSegSrc, rgpfReconYUV, rgpfReconSeg,
 									  pxlcObjColor, rgpvoenc, volmd, rgpostrm);

					bCachedRefDump = TRUE; // need to output this frame later
					bCachedRefCoded = bObjectExists;

					iPCount = volmd.iPbetweenI;
					if (m_rgbSpatialScalability [iVOrelative] && !bTemporalScalability )  // modified by Sharp
                        pvopcBasePVOPQuant = new CVOPU8YUVBA (*(rgpvoenc [BASE_LAYER]->pvopcReconCurr ()),
												        	rgpvoenc [BASE_LAYER]->pvopcReconCurr ()->whereY());
// begin: added by Sharp (98/2/12)
					else if (m_rgbSpatialScalability [iVOrelative] && bTemporalScalability)
						pBufP2->getBuf( rgpvoenc[BASE_LAYER] );
// end: added by Sharp (98/2/12)
				}
				else
				{
					// encoder PVOP
					encodeVideoObject(bObjectExists, bPrevObjectExists, iSearchFrame, PVOP, bCachedRefDump ? DUMP_PREV : DUMP_NONE, // modified by Sharp (99/1/27)
									  iVO, iVOrelative, BASE_LAYER, 
									  pfYuvSrcSpatialEnhn, pfSegSrc, rgpfReconYUV, rgpfReconSeg,
									  pxlcObjColor, rgpvoenc, volmd, rgpostrm);
					bCachedRefDump = TRUE; // need to output this frame later
					bCachedRefCoded = bObjectExists;

					if (m_rgbSpatialScalability [iVOrelative] && !bTemporalScalability )  // modified by Sharp (98/2/12)
						pvopcBasePVOPQuant = new CVOPU8YUVBA (*(rgpvoenc [BASE_LAYER]->pvopcReconCurr ()),
												        	rgpvoenc [BASE_LAYER]->pvopcReconCurr ()->whereY());
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
						bObjectExists = loadDataSpriteCheck(iVOrelative,iBFrame, pfYuvSrc, pfSegSrc, pxlcObjColor, rgpvoenc [BASE_LAYER]->m_pvopcOrig, volmd);
						encodeVideoObject (bObjectExists, bObjectExists, iBFrame, BVOP, bTemporalScalability ? DUMP_NONE: DUMP_CURR, // modified by Sharp (98/11/11)
										   iVO, iVOrelative, BASE_LAYER,
										   pfYuvSrc,pfSegSrc,rgpfReconYUV,rgpfReconSeg,
										   pxlcObjColor,rgpvoenc,volmd,rgpostrm);
						bCachedBVOP = bTemporalScalability ? TRUE : FALSE; // added by Sharp (98/11/11)
						if (m_rgbSpatialScalability [iVOrelative] && !bTemporalScalability) { // modified by Sharp (98/2/12)
							pvopcBaseQuant = rgpvoenc [BASE_LAYER]->pvopcReconCurr ();
							// Spatial Scalabe BVOP 
							encodeVideoObject (bObjectExists, bObjectExists, iBFrame, BVOP, DUMP_CURR, 
											   iVO, iVOrelative, ENHN_LAYER,
											   pfYuvSrcSpatialEnhn, pfSegSrc, rgpfReconYUV, rgpfReconSeg,
											   pxlcObjColor, rgpvoenc, volmd, rgpostrm,
											   pvopcBaseQuant);
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
						if(bCachedBVOP && m_rguiSpriteUsage [iVOrelative] == 0)
						{
							// only for temporal scalability
#ifndef __OUT_ONE_FRAME_
							// last ref frame needs to be output
							dumpData (rgpfReconYUV [BASE_LAYER], rgpfReconSeg [BASE_LAYER],
								rgpvoenc[BASE_LAYER] ->pvopcReconCurr(), m_rctOrg, volmd);
							if (m_rgbSpatialScalability [iVOrelative] && !bTemporalScalability)
								dumpData (rgpfReconYUV [BASE_LAYER], rgpfReconSeg [BASE_LAYER],
									rgpvoenc[BASE_LAYER] ->pvopcReconCurr(), m_rctOrgSpatialEnhn, volmd);
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
					encodeVideoObject(TRUE, TRUE, iSearchFrame, PVOP,
										  DUMP_CURR, // sony
										  iVO, iVOrelative, ENHN_LAYER, 
										  pfYuvSrcSpatialEnhn, pfSegSrc, rgpfReconYUV, rgpfReconSeg,
										  pxlcObjColor,rgpvoenc,volmd,rgpostrm,
										  pvopcBasePVOPQuant);
					}
					else {
						VOPpredType PrevType = (rgpvoenc[ENHN_LAYER]->m_volmd.iSpatialOption == 0 )? BVOP: PVOP;
						encodeVideoObject (bObjectExists, bObjectExists, iSearchFrame, PrevType,
										   DUMP_CURR, // sony
										   iVO, iVOrelative, ENHN_LAYER, 
										   pfYuvSrcSpatialEnhn, pfSegSrc, rgpfReconYUV, rgpfReconSeg,
										   pxlcObjColor, rgpvoenc, volmd, rgpostrm,
										   pvopcBasePVOPQuant);
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

		if(bCachedRefDump && m_rguiSpriteUsage [iVOrelative] == 0)
		{
			// last ref frame needs to be output
#ifndef __OUT_ONE_FRAME_
		if ( bCachedRefCoded )
				dumpData (rgpfReconYUV [BASE_LAYER], rgpfReconSeg [BASE_LAYER], rgpvoenc[BASE_LAYER] ->pvopcRefQLater(), m_rctOrg, volmd);
		else
				dumpNonCodedFrame(rgpfReconYUV [BASE_LAYER], rgpfReconSeg [BASE_LAYER], m_rctOrg, volmd.nBits);

// begin deleted by sony
//			if (m_rgbSpatialScalability [iVOrelative] && !bTemporalScalability)
//				dumpData (rgpfReconYUV [BASE_LAYER], rgpfReconSeg [BASE_LAYER], rgpvoenc[BASE_LAYER] ->pvopcRefQLater(), m_rctOrgSpatialEnhn, volmd);
// end deleted by sony
			bCachedRefDump = FALSE; // added by Sharp (98/11/11)
// begin: deleted by Sharp (98/11/11)
//#else
//				dumpDataOneFrame (iFrame, iVO, rgpvoenc[BASE_LAYER] ->pvopcRefQLater(), volmd); // save one frame
//				if (m_rgbSpatialScalability [iVOrelative] && !bTemporalScalability)
//					dumpDataOneFrame (iFrame, iVO, rgpvoenc[ENHN_LAYER] ->pvopcRefQLater(), volmd); // save one frame
// end: deleted by Sharp (98/11/11)
#endif
		}

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
	}
}

Bool CSessionEncoder::loadDataSpriteCheck(UInt iVOrelative,UInt iFrame, FILE* pfYuvSrc, FILE* pfSegSrc, PixelC pxlcObjColor, CVOPU8YUVBA* pvopcDst, const VOLMode& volmd)
{
	Bool bObjectExists = TRUE;
	if(m_rguiSpriteUsage [iVOrelative] == 0)
		bObjectExists = loadData (iFrame,pfYuvSrc,pfSegSrc, pxlcObjColor, pvopcDst, m_rctOrg, volmd);
	return bObjectExists;
}

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
										FILE* rgpfReconYUV[],
										FILE* rgpfReconSeg[],
										PixelC pxlcObjColor,
										CVideoObjectEncoder** rgpvoenc,
										const VOLMode& volmd,
										ofstream* rgpostrm[],
										const CVOPU8YUVBA* pvopcBaseQuant)
{
	CRct rctOrg;
	if (m_rguiSpriteUsage [iVOrelative] == 0) {
        rctOrg = m_rctOrg;
		if (iLayer == ENHN_LAYER) { 
			rctOrg = m_rctOrgSpatialEnhn;
			if((rgpvoenc [iLayer] -> skipTest((Time)iFrame,predType)))  // rate control
				return;
			bObjectExists = loadData(iFrame, pfYuvSrc, pfSegSrc, pxlcObjColor, rgpvoenc [iLayer]->m_pvopcOrig, rctOrg, volmd);
		}
		rgpvoenc [iLayer] -> encode (bObjectExists, (Time) (iFrame - m_iFirstFrame), predType, pvopcBaseQuant);
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
	if(iDumpMode==DUMP_CURR || m_rguiSpriteUsage [iVOrelative] != 0)
	{
		if (bObjectExists) 
				dumpData (rgpfReconYUV [iLayer], rgpfReconSeg [iLayer], rgpvoenc[iLayer] ->pvopcReconCurr(), rctOrg, volmd);
		else
			dumpNonCodedFrame(rgpfReconYUV [iLayer], rgpfReconSeg [iLayer], rctOrg, volmd.nBits);
	}
	else if(iDumpMode==DUMP_PREV){ // dump previous reference frame
		if ( bPrevObjectExists )
				dumpData (rgpfReconYUV [iLayer], rgpfReconSeg [iLayer], rgpvoenc[iLayer] ->pvopcRefQPrev(), rctOrg, volmd);
		else
				dumpNonCodedFrame(rgpfReconYUV [iLayer], rgpfReconSeg [iLayer], rctOrg, volmd.nBits);
	}
#else
	if (bObjectExists) 
		dumpDataOneFrame (iFrame, iVO, rgpvoenc[iLayer] ->pvopcReconCurr(), volmd); // save one frame
	else
		dumpNonCodedFrame(rgpfReconYUV [iLayer], rgpfReconSeg [iLayer], rctOrg, volmd.nBits);
#endif
// end: added by Sharp (99/1/28)

// begin: deleted by Sharp (99/1/28)
/*
	if (bObjectExists)	{
// begin: modified by Sharp (98/11/11)
// begin: modification by Sharp (98/2/12)
#ifndef __OUT_ONE_FRAME_
		if(iDumpMode==DUMP_CURR || m_rguiSpriteUsage [iVOrelative] != 0)
		{
			// dump current reconstructed frame
			dumpData (rgpfReconYUV [iLayer], rgpfReconSeg [iLayer], rgpvoenc[iLayer] ->pvopcReconCurr(), rctOrg, volmd);
		}
		else if(iDumpMode==DUMP_PREV) // dump previous reference frame
		{
			dumpData (rgpfReconYUV [iLayer], rgpfReconSeg [iLayer], rgpvoenc[iLayer] ->pvopcRefQPrev(), rctOrg, volmd);
		}
// end: modification by Sharp (98/2/12)
#else
			dumpDataOneFrame (iFrame, iVO, rgpvoenc[iLayer] ->pvopcReconCurr(), volmd); // save one frame
#endif
// end: modified by Sharp (98/11/11)
	}
#ifndef __OUT_ONE_FRAME_ // added by Sharp (98/11/11)
	else
		dumpNonCodedFrame(rgpfReconYUV [iLayer], rgpfReconSeg [iLayer], rctOrg, volmd.nBits);
#endif // added by Sharp (98/11/11)
*/
// end: deleted by Sharp (99/1/28)
	rgpostrm [iLayer]->write (rgpvoenc [iLayer]->pOutStream ()->str (),
		rgpvoenc [iLayer]->pOutStream ()->pcount ());
}

Void CSessionEncoder::readPntFile (UInt iobj)
{
	Char pchtmp [100];
	sprintf (pchtmp, SUB_PNTFILE, m_pchSptPntDir, iobj, m_pchPrefix);
	FILE* pfPnt = fopen (pchtmp, "r");
	if(pfPnt==NULL)
		fatal_error("Can't open sprite point file");

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
			fprintf (stderr, "Unexpected end of file\n");
		ppxlcY += iYFrmWidth;
	}

	for (y = 0; y < iUVDataHeight; y++) {
		Int size = (Int) fread (ppxlcU, sizeof (U8), m_rctOrg.width / 2, pf);
		if (size == 0)
			fprintf (stderr, "Unexpected end of file\n");
		ppxlcU += iUvFrmWidth;
	}

	for (y = 0; y < iUVDataHeight; y++) {
		Int size = (Int) fread (ppxlcV, sizeof (U8), m_rctOrg.width / 2, pf);
		if (size == 0)
			fprintf (stderr, "Unexpected end of file\n");
		ppxlcV += iUvFrmWidth;
	}

	PixelC* ppxlcBY = (PixelC*) pvopcDst->pixelsBY () + nSkipYPixel;
	//Int iObjectExist = 0;
	if ((bAUsage == ONE_BIT) && (pvopcDst -> fAUsage () == ONE_BIT)) { // load Alpha
		//binary
		for (CoordI y = 0; y < iYDataHeight; y++) {
			Int size = (Int) fread (ppxlcBY, sizeof (U8), m_rctOrg.width, pf);
			if (size == 0)
				fprintf (stderr, "Unexpected end of file\n");
			ppxlcBY += iYFrmWidth;
		}
		((CU8Image*) pvopcDst->getPlane (BUV_PLANE))->decimateBinaryShapeFrom (*(pvopcDst->getPlane (BY_PLANE)));
	}
#ifdef _FOR_GSSP_
	else if ((bAUsage == EIGHT_BIT) && (pvopcDst -> fAUsage () == EIGHT_BIT)) { // load Alpha
		//grayscale alpha
		PixelC* ppxlcA = (PixelC*) pvopcDst->pixelsA () + nSkipYPixel;
		for (CoordI y = 0; y < iYDataHeight; y++) {
			for (CoordI x = 0; x < m_rctOrg.width; x++) {
				PixelC pxlcCurr = getc (pf);
				ppxlcA [x]  = (pxlcCurr >= GRAY_ALPHA_THRESHOLD) ? pxlcCurr : transpValue;
				ppxlcBY [x] = (pxlcCurr >= GRAY_ALPHA_THRESHOLD) ? opaqueValue : transpValue;
			}
			ppxlcBY += iYFrmWidth;
			ppxlcA  += iYFrmWidth;
		}
		((CU8Image*) pvopcDst->getPlane (BUV_PLANE))->decimateBinaryShapeFrom (*(pvopcDst->getPlane (BY_PLANE)));
	}
#endif
}

Void CSessionEncoder::getInputFiles (FILE*& pfYuvSrc, FILE*& pfAlpSrc, FILE*& pfYuvSrcSpatialEnhn,
									 FILE* rgpfReconYUV [], FILE* rgpfReconSeg [], 
									 ofstream* rgpostrm [], ofstream* rgpostrmTrace [],
									 PixelC& pxlcObjColor, Int iobj, const VOLMode& volmd, const VOLMode& volmd_enhn)
{
	static Char pchYUV [100], pchSeg [100], pchCmp [100], pchTrace [100];

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

	rgpfReconYUV [BASE_LAYER] = fopen (pchYUV, "wb"); // reconstructed YUV file
	assert (rgpfReconYUV [BASE_LAYER] != NULL);
	if (volmd.fAUsage != RECTANGLE)	{
		rgpfReconSeg [BASE_LAYER] = fopen (pchSeg, "wb"); // reconstructed seg file
		assert (rgpfReconSeg [BASE_LAYER] != NULL);
	}
	/*
	if (m_rgbSpatialScalability [iobj - m_iFirstVO] == TRUE)	{
		sprintf (pchCmp, ENHN_SUB_CMPFILE, m_pchOutStrFiles, iobj, m_pchPrefix);
		sprintf (pchTrace, ENHN_SUB_TRCFILE, m_pchOutStrFiles, iobj, m_pchPrefix);
		rgpostrm [ENHN_LAYER] = new ofstream (pchCmp, IOS_BINARY | ios::out);
		rgpostrmTrace [ENHN_LAYER] = NULL;
		if (volmd.bTrace)
			rgpostrmTrace [ENHN_LAYER] = new ofstream (pchTrace);

			sprintf (pchYUV, ENHN_SUB_YUVFILE, m_pchReconYUVDir, iobj, m_pchPrefix);
			sprintf (pchSeg, ENHN_SUB_SEGFILE, m_pchReconYUVDir, iobj, m_pchPrefix);
			rgpfReconYUV [ENHN_LAYER] = fopen (pchYUV, "wb");		// reconstructed YUV file
			if (volmd.fAUsage != RECTANGLE)	{
				rgpfReconSeg [ENHN_LAYER] = fopen (pchSeg, "wb"); // reconstructed seg file
				assert (FALSE);									// no spatial scalability for shape
			}
	}
	*/ //wchen: moved down 
	if (!m_bTexturePerVOP) 
		sprintf (pchYUV, ROOT_YUVFILE, m_pchBmpFiles, m_pchPrefix);
	else 
		sprintf (pchYUV, SUB_YUVFILE, m_pchBmpFiles, iobj, m_pchPrefix);
	pfYuvSrc = fopen (pchYUV, "rb");
	if (pfYuvSrc == NULL)	{
		fprintf (stderr, "can't open %s\n", pchYUV);
		exit (1);
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
					rgpfReconSeg [ENHN_LAYER] = fopen (pchSeg, "wb"); // reconstructed seg file
				}
			}
		} else {
// end: added by Sharp (98/10/26)
		sprintf (pchYUV, ENHN_SUB_YUVFILE, m_pchReconYUVDir, iobj, m_pchPrefix);
		sprintf (pchSeg, ENHN_SUB_SEGFILE, m_pchReconYUVDir, iobj, m_pchPrefix);
		rgpfReconYUV [ENHN_LAYER] = fopen (pchYUV, "wb");		// reconstructed YUV file
		if (volmd_enhn.fAUsage != RECTANGLE)	{ // modified by Sharp (98/2/12)
			rgpfReconSeg [ENHN_LAYER] = fopen (pchSeg, "wb"); // reconstructed seg file
// deleted by Sharp (98/10/26)
	// begin: added by Sharp (98/2/10)
	//			if ( volmd.bTemporalScalability == TRUE )
	//				assert (rgpfReconSeg [ENHN_LAYER] != NULL);
	//			else
	// end: added by Sharp (98/2/10)
// end: deleted by Sharp (98/10/26)
			assert (FALSE);									// no spatial scalability for shape
		}
		} // added by Sharp (98/10/26)

		if ( volmd_enhn.bTemporalScalability == FALSE ){ // added by Sharp (98/2/10)
		if (!m_bTexturePerVOP) { 
			fprintf (stderr,"m_bTexturePerVOP != 0 is not applyed for spatial scalable coding\n");
		}
		else 
			sprintf (pchYUV, ENHN_SUB_YUVFILE, m_pchBmpFiles, iobj, m_pchPrefix);
		pfYuvSrcSpatialEnhn = fopen (pchYUV, "rb");
		if (pfYuvSrcSpatialEnhn == NULL){
			fprintf (stderr,"can't open %s\n", pchYUV);
			exit(1);
		}
		} // added by Sharp (98/2/10)
	}

	if (volmd.fAUsage != RECTANGLE ||
		m_rgbSpatialScalability [iobj - m_iFirstVO] == TRUE && volmd_enhn.fAUsage != RECTANGLE) { // load Alpha // modified by Sharp (98/2/12)
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
// begin: deleted by Sharp (98/10/26)
	// begin: added by Sharp (98/2/10)
	// #ifndef __OUT_ONE_FRAME_
	// 	if (m_rgbSpatialScalability [iobj - m_iFirstVO] == TRUE && volmd_enhn.bTemporalScalability == TRUE && volmd_enhn.iEnhnType != 0) { // modified by Sharp (98/3/24)
	// 		FILE *pfTemp;
	// 		sprintf (pchYUV, "%s/%2.2d/%s_bgc.yuv", m_pchReconYUVDir, iobj, m_pchPrefix);
	// 		pfTemp = fopen (pchYUV, "wb");			 // clear file pointer for background composition
	// 		fclose(pfTemp);
	// 	}
	// #endif
	// end: added by Sharp (98/2/10)
// end: deleted by Sharp (98/10/26)
}

Void CSessionEncoder::initVOEncoder (CVideoObjectEncoder** rgpvoenc, Int iobj, ofstream* rgpostrmTrace [])
{
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
														m_rguiWarpingAccuracy [iVOidx],
														m_rgNumOfPnts [iVOidx],
														m_pppstDst [iVOidx],
														m_SptMode,
														m_rctFrame,
                                                        m_rctOrg,
														m_rgiMVFileUsage [BASE_LAYER][iVOidx],
														m_pchMVFileName  [BASE_LAYER][iVOidx]);
	}
}

Bool CSessionEncoder::loadData (UInt iFrame, FILE* pfYuvSrc, FILE* pfSegSrc, PixelC pxlcObjColor, CVOPU8YUVBA* pvopcDst, CRct& rctOrg, const VOLMode& volmd)
{
	Int iLeadingPixels = iFrame * rctOrg.area ();
	if (volmd.nBits<=8) {
		fseek (pfYuvSrc, iLeadingPixels + iLeadingPixels / 2, SEEK_SET);	//4:2:0
	} else { // NBIT: 2 bytes per pixel, Y component plus UV
		fseek (pfYuvSrc, iLeadingPixels * 3, SEEK_SET);
	}
	if (volmd.fAUsage != RECTANGLE)
		fseek (pfSegSrc, iLeadingPixels, SEEK_SET);	

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
			fprintf (stderr, "Unexpected end of file\n");
		ppxlcY += iYFrmWidth;
	}

 /* modified by Rockwell (98/05/08)
	if (volmd.nBits<=8) {*/
	for (y = 0; y < iUVDataHeight; y++) {
		Int size = (Int) fread (ppxlcU, sizeof (PixelC), rctOrg.width / 2, pfYuvSrc);
		if (size == 0)
			fprintf (stderr, "Unexpected end of file\n");
		ppxlcU += iUvFrmWidth;
	}

	for (y = 0; y < iUVDataHeight; y++) {
		Int size = (Int) fread (ppxlcV, sizeof (PixelC), rctOrg.width / 2, pfYuvSrc);
		if (size == 0)
			fprintf (stderr, "Unexpected end of file\n");
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
	if (volmd.fAUsage == ONE_BIT) { // load Alpha
		//binary
		for (y = 0; y < iYDataHeight; y++) {
			for (x = 0; x < rctOrg.width; x++) {
				PixelC pxlcCurr = getc (pfSegSrc);
				ppxlcBY [x] = (pxlcCurr == pxlcObjColor) ? opaqueValue : transpValue;
				iObjectExist += ppxlcBY [x];
			}
			ppxlcBY += iYFrmWidth;
		}
		((CU8Image*) pvopcDst->getPlane (BUV_PLANE))->decimateBinaryShapeFrom (*(pvopcDst->getPlane (BY_PLANE)));
	}
	else if (volmd.fAUsage == EIGHT_BIT) { // load Alpha
		//gray
		PixelC* ppxlcA = (PixelC*) pvopcDst->pixelsA () + nSkipYPixel;
		for (y = 0; y < iYDataHeight; y++) {
			for (x = 0; x < rctOrg.width; x++) {
				PixelC pxlcCurr = getc (pfSegSrc);
				ppxlcA [x]  = (pxlcCurr >= GRAY_ALPHA_THRESHOLD) ? pxlcCurr : transpValue;
				ppxlcBY [x] = (pxlcCurr >= GRAY_ALPHA_THRESHOLD) ? opaqueValue : transpValue;
				iObjectExist += ppxlcBY [x];
			}
			ppxlcBY += iYFrmWidth;
			ppxlcA  += iYFrmWidth;
		}
		((CU8Image*) pvopcDst->getPlane (BUV_PLANE))->decimateBinaryShapeFrom (*(pvopcDst->getPlane (BY_PLANE)));
	}
	else                                                                                                        //rectangle
		iObjectExist = 1;

/*	static int ini = 0;
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

Void CSessionEncoder::dumpData (FILE* pfYuvDst, FILE* pfSegDst, 
								const CVOPU8YUVBA* pvopcSrc, 
								const CRct& rctOrg, const VOLMode& volmd)
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
	
	if ( volmd.iEnhnType == 0 || volmd.volType == BASE_LAYER ){ // added by Sharp (99/1/20)
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
	  } // added by Sharp (99/1/20)

	
//	if ( volmd.iEnhnType == 0) { // added by Sharp (98/10/26)
	if (volmd.fAUsage == ONE_BIT)	{
		const PixelC* ppxlcBY = pvopcSrc->pixelsBY () + nSkipYPixel;
		for (CoordI y = 0; y < iYDataHeight; y++) {
		    if (volmd.bNot8Bit==0) {
				/* Int size = (Int)*/ fwrite (ppxlcBY, sizeof (PixelC), m_rctOrg.width, pfSegDst);
		    } else { // NBIT: here PixelC is unsigned short
				const PixelC* ppxlcBYAux = ppxlcBY;
				for (CoordI x = 0; x < m_rctOrg.width; x++, ppxlcBYAux++) {
					fwrite(ppxlcBYAux, sizeof(PixelC), 1, pfSegDst);
				}
		    }
			ppxlcBY += iYFrmWidth;
		}
	} else if (volmd.fAUsage == EIGHT_BIT)	{
		const PixelC* ppxlcBY = pvopcSrc->pixelsBY () + nSkipYPixel;
		const PixelC* ppxlcA = pvopcSrc->pixelsA () + nSkipYPixel;
		const PixelC pxlcZero = 0;
		for (CoordI y = 0; y < iYDataHeight; y++) {
			const PixelC* ppxlcBYAux = ppxlcBY;
			const PixelC* ppxlcAAux = ppxlcA;
			CoordI x;
			for( x = 0; x < m_rctOrg.width; x++, ppxlcBYAux++, ppxlcAAux++)
				if(*ppxlcBYAux)
					fwrite(ppxlcAAux, sizeof(PixelC), 1, pfSegDst);
				else
					fwrite(&pxlcZero, sizeof(PixelC), 1, pfSegDst);
			ppxlcA += iYFrmWidth;
			ppxlcBY += iYFrmWidth;
		}
	}
//	} // added by Sharp (98/10/26)
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
														  : pvopcSrc->pixelsA () + nSkipYPixel;
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
		bObjectExists = loadData (iFrame, pfYuvSrc, pfSegSrc, pxlcObjColor, rgpvoenc ->m_pvopcOrig, m_rctOrg, volmd_enhn);

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
				rgpvoenc  -> m_vopmd.iLoadBakShape = loadData (ieFramebShape, pfYuvSrc, pfSegSrc, pxlcObjColor, 
					rgpvoenc ->rgpbfShape [0]->m_pvopcOrig, m_rctOrg, rgpvoenc ->rgpbfShape[0]->m_volmd);
				if ( !rgpvoenc -> m_vopmd.iLoadBakShape )
					cout << "Load_backward_shape was ON, but turned off because of no shape\n";
			}
			if(rgpvoenc -> m_vopmd.iLoadForShape) {
				rgpvoenc -> m_vopmd.iLoadForShape = loadData (ieFramefShape, pfYuvSrc, pfSegSrc, pxlcObjColor, 
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
				dumpData (rgpfReconYUV [iLayer], rgpfReconSeg [iLayer], rgpvoenc ->pvopcReconCurr (), m_rctOrg, volmd_enhn);
			}
			else if(iDumpMode==DUMP_PREV)
			{
				dumpData (rgpfReconYUV [iLayer], rgpfReconSeg [iLayer], rgpvoenc ->pvopcRefQPrev (), m_rctOrg, volmd_enhn);
			}
#else
				dumpDataOneFrame (iFrame, iVO, rgpvoenc ->pvopcReconCurr(), volmd_enhn); // save one frame
#endif
// end: modified by Sharp (98/11/11)
//		} // added by Sharp (98/10/26) // deleted by Sharp (99/1/25)
	}
#ifndef __OUT_ONE_FRAME_ // added by Sharp (98/11/11)
	else
		dumpNonCodedFrame(rgpfReconYUV [iLayer], rgpfReconSeg [iLayer], m_rctOrg, volmd_enhn.nBits);
#endif // added by Sharp (98/11/11)

	rgpostrm[ENHN_LAYER] ->write (rgpvoenc ->pOutStream ()->str (), rgpvoenc ->pOutStream ()->pcount ());
	if ( rgpvoenc -> m_bCodedFutureRef == 1 ) // added by Sharp (99/1/28)
	BufE.getBuf( rgpvoenc );
}

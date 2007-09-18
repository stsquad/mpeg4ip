/*************************************************************************

This software module was originally developed by 

	Ming-Chieh Lee (mingcl@microsoft.com), Microsoft Corporation
	Wei-ge Chen (wchen@microsoft.com), Microsoft Corporation
	Bruce Lin (blin@microsoft.com), Microsoft Corporation
	Chuang Gu (chuanggu@microsoft.com), Microsoft Corporation
	Simon Winder (swinder@microsoft.com), Microsoft Corporation
	(date: March, 1996)

and edited by

	Hiroyuki Katata (katata@imgsl.mkhar.sharp.co.jp), Sharp Corporation
	Norio Ito (norio@imgsl.mkhar.sharp.co.jp), Sharp Corporation
	Shuichi Watanabe (watanabe@imgsl.mkhar.sharp.co.jp), Sharp Corporation
	(date: October, 1997)

and also edited by
	Xuemin Chen (xchen@nlvl.com). Next Level Systems, Inc.
	Bob Eifrig (beifrig@nlvl.com) Next Level Systems, Inc.

and also edited by
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
Sharp retains full right to use the code for his/her own purpose, 
assign or donate the code to a third party and to inhibit third parties from using the code for non <MPEG standard> conforming products. 
This copyright notice must be included in all copies or derivative works. 

Copyright (c) 1997.

Module Name:

	encoder.cpp

Abstract:

	caller for encoder

Revision History:

	May.09, 1999:	tm5 rate control by DemoGraFX, duhoff@mediaone.net
	Aug.24,	1999 : NEWPRED added by Hideaki Kimata (NTT) 
	Aug.30,	1999 : set _MAX_PATH to 128 (directly specified) by Hideaki Kimata (NTT) 
	Sep.06	1999 : RRV added by Eishi Morimatsu (Fujitsu Laboratories Ltd.) 
	Nov.05  1999 : New parameter set file format (Simon Winder, Microsoft)
	Nov.11  1999 : Fixed Complexity Estimation syntax support, version 2 (Massimo Ravasi, EPFL)
	Feb.12  2000 : Bug Fix of OBSS by Takefumi Nagumo (Sony)

*************************************************************************/


#include <stdio.h>
#include <stdlib.h>

#ifdef __PC_COMPILER_
#include <windows.h>
#include <mmsystem.h>
#endif // __PC_COMPILER_

#include "typeapi.h"
#include "codehead.h"
#include "paramset.h"

#include "mode.hpp"
#include "tm5rc.hpp"
#include "iostream"
#include "sesenc.hpp"
// #include "encoder/tps_sesenc.hpp" // deleted by Sharp (98/2/12)

///// WAVELET VTC: begin ////////////////////////////////
#include "dataStruct.hpp"     // hjlee 
///// WAVELET VTC: end ////////////////////////////////


#ifndef __GLOBAL_VAR_
//#define __GLOBAL_VAR_
#endif
#include "global.hpp"
//#define DEFINE_GLOBALS
#include "globals.hpp"

#ifdef __MFC_
#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

#define new DEBUG_NEW				   
#endif // __MFC_

#define _FOR_QUARTERSAMPLE

#ifndef _FOR_QUARTERSAMPLE // to use rgiDefaultIntraQMatrixAlpha from global.hpp now 

#define _FOR_GSSP_

#ifdef _FOR_GSSP_
Int rgiDefaultIntraQMatrixAlpha [BLOCK_SQUARE_SIZE] = {
	8,  17, 18, 19, 21, 23, 25, 27,
	17, 18, 19, 21, 23, 25, 27, 28,
	20, 21, 22, 23, 24, 26, 28, 30,
	21, 22, 23, 24, 26, 28, 30, 32,
	22, 23, 24, 26, 28, 30, 32, 35,
	23, 24, 26, 28, 30, 32, 35, 38,
	25, 26, 28, 30, 32, 35, 38, 41,
	27, 28, 30, 32, 35, 38, 41, 45
};
Int rgiDefaultInterQMatrixAlpha [BLOCK_SQUARE_SIZE] = {
	16, 17, 18, 19, 20, 21, 22, 23,
	17, 18, 19, 20, 21, 22, 23, 24,
	18, 19, 20, 21, 22, 23, 24, 25,
	19, 20, 21, 22, 23, 24, 26, 27,
	20, 21, 22, 23, 25, 26, 27, 28,
	21, 22, 23, 24, 26, 27, 28, 30,
	22, 23, 24, 26, 27, 28, 30, 31,
	23, 24, 25, 27, 28, 30, 31, 33
};
#else
Int rgiDefaultIntraQMatrixAlpha [BLOCK_SQUARE_SIZE] = {
	16, 16, 16, 16, 16, 16, 16, 16,
	16, 16, 16, 16, 16, 16, 16, 16,
	16, 16, 16, 16, 16, 16, 16, 16,
	16, 16, 16, 16, 16, 16, 16, 16,
	16, 16, 16, 16, 16, 16, 16, 16,
	16, 16, 16, 16, 16, 16, 16, 16,
	16, 16, 16, 16, 16, 16, 16, 16,
	16, 16, 16, 16, 16, 16, 16, 16
};
Int rgiDefaultInterQMatrixAlpha [BLOCK_SQUARE_SIZE] = {
	16, 16, 16, 16, 16, 16, 16, 16,
	16, 16, 16, 16, 16, 16, 16, 16,
	16, 16, 16, 16, 16, 16, 16, 16,
	16, 16, 16, 16, 16, 16, 16, 16,
	16, 16, 16, 16, 16, 16, 16, 16,
	16, 16, 16, 16, 16, 16, 16, 16,
	16, 16, 16, 16, 16, 16, 16, 16,
	16, 16, 16, 16, 16, 16, 16, 16
};
#endif

#endif // _FOR_QUARTERSAMPLE

Void nextValidLine (FILE *pfPara, UInt* pnLine);
Void readVTCParam(CVTCEncoder *pvtcenc, FILE *pfPara, UInt* pnLine, UInt *frmWidth, UInt *frmHeight);

Void RunVTCCodec(char *VTCCtlFile);
Void GetIVal(CxParamSet *pPar, char *pchName, Int iInd, Int *piRtn);
Void GetDVal(CxParamSet *pPar, char *pchName, Int iInd, Double *pdRtn);
Void GetSVal(CxParamSet *pPar, char *pchName, Int iInd, char **ppchRtn);
Void GetAVal(CxParamSet *pPar, char *pchName, Int iInd, Double **ppdRtn, Int *piCount);
Void readBoolParam(CxParamSet *pPar, char *pchName, Int iInd, Bool *pbVal);

Int bPrint = 0;

#define VERSION_STRING "Version 2.3.0 (001213)"
// please update version number when you
// make changes in the following

#define BASE_LAYER 0
#define ENHN_LAYER 1
#define NO_SCALABILITY		 0
#define TEMPORAL_SCALABILITY 1
#define SPATIAL_SCALABILITY  2

int main (Int argc, Char* argv[])
{
  //	UInt nLine = 1;
	//	UInt* pnLine = &nLine;
	FILE *pfPara;

	if (argc != 2)	{
		printf ("Usage: %s parameter_file\n       %s -version", argv [0], argv[0]);
		exit (1);
	}
	if(argv[1][0]=='-' && argv[1][1]=='v')
	{
		printf("Microsoft MPEG-4 Visual CODEC %s\n",VERSION_STRING);
		exit(0);
	}

	if ((pfPara = fopen (argv[1], "r")) == NULL ){
		printf ("Parameter File Not Found\n");
		exit (1);
	}

#ifdef _MBQP_CHANGE_
	srand(9184756); // for testing purposes - random dquant
#endif

	// all the parameters to the encoder
	Int iVersion;
	Int iVTCFlag;
	UInt uiFrmWidth, uiFrmHeight;
	UInt firstFrm, lastFrm;
	Bool bNot8Bit;
	UInt uiQuantPrecision;
	UInt nBits;
	UInt firstVO, lastVO;
	UInt nVO;
	UInt uiFrmWidth_SS,uiFrmHeight_SS;
	UInt uiHor_sampling_m,uiHor_sampling_n;
	UInt uiVer_sampling_m,uiVer_sampling_n;
	Bool bAnyScalability = false;
	Int iSpatialOption = 0;
	char *pchPrefix;		
	char *pchBmpDir;
	char *pchOutBmpDir;
	char *pchOutStrFile;
	char *pchSptDir;
	char *pchSptPntDir;

	// version 2 start
	UInt uiHor_sampling_m_shape,uiHor_sampling_n_shape;
	UInt uiVer_sampling_m_shape,uiVer_sampling_n_shape;
	UInt uiUseRefShape;
	UInt uiUseRefTexture;
	// version 2 end

	Int *rgiTemporalScalabilityType;
	Bool *rgbSpatialScalability;
	Bool *rgbScalability;
	Int *rgiEnhancementType;
//OBSSFIX_MODE3
	Int *rgiEnhancementTypeSpatial;
//~OBSSFIX_MODE3
	AlphaUsage *rgfAlphaUsage;
	Int  *rgiAlphaShapeExtension; // MAC-V2 (SB)
	Bool *rgbShapeOnly;
	Int *rgiBinaryAlphaTH;
	Int *rgiGrayToBinaryTH;
	Int *rgbNoCrChange;
	Int *rgiBinaryAlphaRR;
	Bool *rgbRoundingControlDisable;
	Int *rgiInitialRoundingType;
	Int *rgiNumPbetweenIVOP;
	Int *rgiNumBbetweenPVOP;
	Int *rgiGOVperiod;
	Bool *rgbDeblockFilterDisable;
	Int *rgiTSRate;
	Int *rgiEnhcTSRate;
	ChromType *rgfChrType;
	Bool *rgbAllowSkippedPMBs;
	SptMode *rgSpriteMode;
	Bool *rgbDumpMB;
	Bool *rgbTrace;
	UInt *rguiSpriteUsage; 
	UInt *rguiWarpingAccuracy; 
	Int *rgiNumPnts;
	// version 2 start
	UInt *rguiVerID;
	// version 2 end

	UInt *rguiRateControl [2];
	UInt *rguiBitsBudget [2];
	Bool *rgbAdvPredDisable [2];
	Bool *rgbErrorResilientDisable [2];
	Bool *rgbDataPartitioning [2];
	Bool *rgbReversibleVlc [2];
	Int *rgiVPBitTh [2];
	Bool *rgbInterlacedCoding [2];
	Quantizer* rgfQuant [2];
	Bool *rgbLoadIntraMatrix [2];
	Int **rgppiIntraQuantizerMatrix [2];
	Bool *rgbLoadInterMatrix [2];
	Int **rgppiInterQuantizerMatrix [2];
	Bool *rgiIntraDCSwitchingThr [2];
	Int *rgiIStep [2];
	Int *rgiPStep [2];
	Int *rgiStepBCode [2];
	Bool *rgbLoadIntraMatrixAlpha [2];
	Int **rgppiIntraQuantizerMatrixAlpha [2];
	Bool *rgbLoadInterMatrixAlpha [2];
	Int **rgppiInterQuantizerMatrixAlpha [2];
	Int *rgiIStepAlpha [2];		
	Int *rgiPStepAlpha [2];
	Int *rgiBStepAlpha [2];
	Bool *rgbNoGrayQuantUpdate [2];
	UInt *rguiSearchRange [2];
	Bool *rgbOriginalME [2];
	Bool *rgbComplexityEstimationDisable [2];
	Bool *rgbOpaque [2];
	Bool *rgbTransparent [2];
	Bool *rgbIntraCAE [2];
	Bool *rgbInterCAE [2];
	Bool *rgbNoUpdate [2];
	Bool *rgbUpsampling [2];
	Bool *rgbIntraBlocks [2];
	Bool *rgbInterBlocks [2];
	Bool *rgbInter4vBlocks [2];
	Bool *rgbNotCodedBlocks [2];
	Bool *rgbDCTCoefs [2];
	Bool *rgbDCTLines [2];
	Bool *rgbVLCSymbols [2];
	Bool *rgbVLCBits [2];
	Bool *rgbAPM [2];
	Bool *rgbNPM [2];
	Bool *rgbInterpolateMCQ [2];
	Bool *rgbForwBackMCQ [2];
	Bool *rgbHalfpel2 [2];
	Bool *rgbHalfpel4 [2];

	UInt *rguiVolControlParameters[2];
	UInt *rguiChromaFormat[2];
	UInt *rguiLowDelay[2];
	UInt *rguiVBVParams[2];
	UInt *rguiBitRate[2];
	UInt *rguiVbvBufferSize[2];
	UInt *rguiVbvBufferOccupany[2];
	Double *rgdFrameFrequency[2];
	Bool *rgbTopFieldFirst [2];
	Bool *rgbAlternateScan [2];
	Int *rgiDirectModeRadius [2];
	Int *rgiMVFileUsage[2];
	char **pchMVFileName[2];

	// version 2 start
	Bool *rgbNewpredEnable[2];
	Bool *rgbNewpredSegType[2];
	char **pchNewpredRefName[2];
	char **pchNewpredSlicePoint[2];
	Bool *rgbSadctDisable[2];
	Bool *rgbQuarterSample[2];
	RRVmodeStr *RRVmode[2];
	UInt *rguiEstimationMethod [2];
	Bool *rgbSadct [2];
	Bool *rgbQuarterpel [2];
	// version 2 end

	UInt iObj;

	Int iCh = getc(pfPara);
	if(iCh!='!')
	{
		fprintf(stderr, "Old-style parameter format is no longer supported.\n");
		fprintf(stderr, "Use convertpar tool to convert you files to the new format.\n");
		exit(1);
	}

	// NEW STYLE PARAMETER FILE
	CxParamSet par;

	char rgBuf[80];
	char *pch = fgets(rgBuf, 79, pfPara);
	if(pch==NULL)
		fatal_error("Can't read magic number in parameter file");
	if(strcmp("!!MS!!!\n", rgBuf)!=0)
		fatal_error("Bad magic number at start of parameter file");

	int iLine;
	int er = par.Load(pfPara, &iLine);
	if(er!=ERES_OK)
		exit(printf("error (%d) in format at line %d of parameter file\n", er, iLine));
	
	fclose(pfPara);

	// process the parameters
	GetIVal(&par, "Version", -1, &iVersion);
	if(iVersion<901 || iVersion>906)
		fatal_error("Incorrect parameter file version number for this compilation");

	GetIVal(&par, "VTC.Enable", -1, &iVTCFlag);
	if(iVTCFlag)
	{
		char *VTCCtlFile;
		GetSVal(&par, "VTC.Filename", -1, &VTCCtlFile);
		RunVTCCodec(VTCCtlFile);
		return 0;
	}

	GetIVal(&par, "Source.Width", -1, (Int *)&uiFrmWidth);
	GetIVal(&par, "Source.Height", -1, (Int *)&uiFrmHeight);
	GetIVal(&par, "Source.FirstFrame", -1, (Int *)&firstFrm);
	GetIVal(&par, "Source.LastFrame", -1, (Int *)&lastFrm);

	fatal_error("Last frame number cannot be smaller than first frame number", lastFrm >= firstFrm);

	GetIVal(&par, "Source.BitsPerPel", -1, (Int *)&nBits);
	readBoolParam(&par, "Not8Bit.Enable", -1, &bNot8Bit);
	GetIVal(&par, "Not8Bit.QuantPrecision", -1, (Int *)&uiQuantPrecision);

	if(bNot8Bit)
		fatal_error("When Not8Bit is enabled, the __NBIT_ compile flag must be used.", sizeof(PixelC)!=sizeof(char));
	else
		fatal_error("When Not8Bit is disabled, the __NBIT_ compile flag must not be used.", sizeof(PixelC)==sizeof(char));

	if(bNot8Bit==0)
	{
		uiQuantPrecision = 5;
		nBits = 8;
	}

	fatal_error("Number of bits per pel is out of range", nBits>=4 && nBits<=12);

	Int iLen;

	char *pchTmp;
	GetSVal(&par, "Source.FilePrefix", -1, &pchTmp);
	iLen = strlen(pchTmp) + 1;
	pchPrefix = new char [iLen];
	memcpy(pchPrefix, pchTmp, iLen);

	GetSVal(&par, "Source.Directory", -1, &pchTmp);
	iLen = strlen(pchTmp) + 1;
	pchBmpDir = new char [iLen];
	memcpy(pchBmpDir, pchTmp, iLen);

	GetSVal(&par, "Output.Directory.DecodedFrames", -1, &pchTmp);	
	iLen = strlen(pchTmp) + 1;
	pchOutBmpDir = new char [iLen];
	memcpy(pchOutBmpDir, pchTmp, iLen);

	GetSVal(&par, "Output.Directory.Bitstream", -1, &pchTmp);
	iLen = strlen(pchTmp) + 1;
	pchOutStrFile = new char [iLen];
	memcpy(pchOutStrFile, pchTmp, iLen);

	GetSVal(&par, "Sprite.Directory", -1, &pchTmp);
	iLen = strlen(pchTmp) + 1;
	pchSptDir = new char [iLen];
	memcpy(pchSptDir, pchTmp, iLen);

	GetSVal(&par, "Sprite.Points.Directory", -1, &pchTmp);
	iLen = strlen(pchTmp) + 1;
	pchSptPntDir = new char [iLen];
	memcpy(pchSptPntDir, pchTmp, iLen);

	GetIVal(&par, "Source.ObjectIndex.First", -1, (Int *)&firstVO);
	//GetIVal(&par, "Source.ObjectIndex.Last", -1, (Int *)&lastVO);
	lastVO = firstVO;

	fatal_error("First and last object indices don't make sense", lastVO>=firstVO);
	nVO = lastVO - firstVO + 1;

	Bool bCodeSequenceHead = FALSE;
	UInt uiProfileAndLevel = 0;

	if(iVersion>=906)
	{
		readBoolParam(&par, "Header.EncodeSessionInfo.Enable", -1, &bCodeSequenceHead);
		GetIVal(&par, "Header.ProfileAndLevel", -1, (Int *)&uiProfileAndLevel);
	}

	// allocate per-vo parameters
	rgiTemporalScalabilityType = new Int [nVO];
	rgbSpatialScalability = new Bool [nVO];
	rgbScalability = new Bool [nVO];
	rgiEnhancementType = new Int [nVO];
//OBSSFIX_MODE3
	rgiEnhancementTypeSpatial = new Int [nVO];
//~OBSSFIX_MODE3
	rgfAlphaUsage = new AlphaUsage [nVO];
	rgiAlphaShapeExtension = new Int [nVO];
	rgbShapeOnly = new Bool [nVO];
	rgiBinaryAlphaTH = new Int [nVO];
	rgiGrayToBinaryTH = new Int [nVO];
	rgbNoCrChange = new Bool [nVO];
	rgiBinaryAlphaRR = new Int [nVO];
	rgbRoundingControlDisable = new Bool [nVO];
	rgiInitialRoundingType = new Int [nVO];
	rgiNumPbetweenIVOP = new Int [nVO];
	rgiNumBbetweenPVOP = new Int [nVO];
	rgiGOVperiod = new Int [nVO];
	rgbDeblockFilterDisable = new Bool [nVO];
	rgiTSRate = new Int [nVO];
	rgiEnhcTSRate = new Int [nVO];
	rgfChrType = new ChromType [nVO];
	rgbAllowSkippedPMBs = new Bool [nVO];
	rgSpriteMode = new SptMode [nVO];
	rgbDumpMB = new Bool [nVO];
	rgbTrace = new Bool [nVO];
	rguiSpriteUsage = new UInt [nVO]; 
	rguiWarpingAccuracy = new UInt [nVO]; 
	rgiNumPnts = new Int [nVO]; 
	
	// version 2 start
	rguiVerID = new UInt [nVO];
	// version 2 end
	
	Int iL;
	for(iL = BASE_LAYER; iL<=ENHN_LAYER; iL++)
	{
		// allocate per-layer parameters
		rguiRateControl [iL] = new UInt [nVO];
		rguiBitsBudget [iL] = new UInt [nVO];
		rgbAdvPredDisable [iL] = new Bool [nVO];
		rgbErrorResilientDisable [iL] = new Bool [nVO];
		rgbDataPartitioning [iL] = new Bool [nVO];
		rgbReversibleVlc [iL] = new Bool [nVO];
		rgiVPBitTh [iL] = new Int [nVO];
		rgbInterlacedCoding [iL] = new Bool [nVO];	
		rgfQuant [iL] = new Quantizer [nVO]; 
		rgbLoadIntraMatrix [iL] = new Bool [nVO];
		rgppiIntraQuantizerMatrix [iL] = new Int * [nVO];
		rgbLoadInterMatrix [iL] = new Bool [nVO];
		rgppiInterQuantizerMatrix [iL] = new Int * [nVO];
		rgiIntraDCSwitchingThr [iL] = new Int [nVO]; 
		rgiIStep [iL] = new Int [nVO]; 
		rgiPStep [iL] = new Int [nVO]; 
		rgiStepBCode [iL] = new Int [nVO]; 
		rgbLoadIntraMatrixAlpha [iL] = new Bool [nVO]; 	
		rgppiIntraQuantizerMatrixAlpha [iL] = new Int * [nVO];
		rgbLoadInterMatrixAlpha [iL] = new Bool [nVO]; 	
		rgppiInterQuantizerMatrixAlpha [iL] = new Int * [nVO];
		rgiIStepAlpha [iL] = new Int [nVO]; 
		rgiPStepAlpha [iL] = new Int [nVO]; 
		rgiBStepAlpha [iL] = new Int [nVO]; 
		rgbNoGrayQuantUpdate [iL] = new Bool [nVO];
		rguiSearchRange [iL] = new UInt [nVO];
		rgbOriginalME [iL] = new Bool [nVO];
		rgbComplexityEstimationDisable [iL] = new Bool [nVO];
		rgbOpaque [iL] = new Bool [nVO];
		rgbTransparent [iL] = new Bool [nVO];
		rgbIntraCAE [iL] = new Bool [nVO];
		rgbInterCAE [iL] = new Bool [nVO];
		rgbNoUpdate [iL] = new Bool [nVO];
		rgbUpsampling [iL] = new Bool [nVO];
		rgbIntraBlocks [iL] = new Bool [nVO];
		rgbInterBlocks [iL] = new Bool [nVO];
		rgbInter4vBlocks [iL] = new Bool [nVO];
		rgbNotCodedBlocks [iL] = new Bool [nVO];
		rgbDCTCoefs [iL] = new Bool [nVO];
		rgbDCTLines [iL] = new Bool [nVO];
		rgbVLCSymbols [iL] = new Bool [nVO];
		rgbVLCBits [iL] = new Bool [nVO];
		rgbAPM [iL] = new Bool [nVO];
		rgbNPM [iL] = new Bool [nVO];
		rgbInterpolateMCQ [iL] = new Bool [nVO];
		rgbForwBackMCQ [iL] = new Bool [nVO];
		rgbHalfpel2 [iL] = new Bool [nVO];
		rgbHalfpel4 [iL] = new Bool [nVO];
		rguiVolControlParameters [iL] = new UInt [nVO];
		rguiChromaFormat [iL] = new UInt [nVO];
		rguiLowDelay [iL] = new UInt [nVO];
		rguiVBVParams [iL] = new UInt [nVO];
		rguiBitRate [iL] = new UInt [nVO];
		rguiVbvBufferSize [iL] = new UInt [nVO];
		rguiVbvBufferOccupany [iL] = new UInt [nVO];
		rgdFrameFrequency [iL] = new Double [nVO];
		rgbTopFieldFirst [iL] = new Bool [nVO]; 
		rgbAlternateScan [iL] = new Bool [nVO]; 
		rgiDirectModeRadius [iL] = new Bool [nVO]; 
		rgiMVFileUsage[iL] = new Int [nVO];
		pchMVFileName[iL] = new char * [nVO];

		// version 2 start
		rgbNewpredEnable[iL] = new Bool [nVO];
		rgbNewpredSegType[iL] = new Bool [nVO];
		pchNewpredRefName[iL] = new char * [nVO];
		pchNewpredSlicePoint[iL] = new char * [nVO];
		rgbSadctDisable[iL] = new Bool [nVO];
		rgbQuarterSample[iL] = new Bool [nVO];
		RRVmode[iL] = new RRVmodeStr [nVO];
		rguiEstimationMethod [iL] = new UInt [nVO];
		rgbSadct [iL] = new Bool [nVO];
		rgbQuarterpel [iL] = new Bool [nVO];
		// version 2 end
	}

	for (iObj = 0; iObj < nVO; iObj++)
	{
		// per object alloc
		rgppiIntraQuantizerMatrix [BASE_LAYER] [iObj] = new Int [BLOCK_SQUARE_SIZE];
		rgppiIntraQuantizerMatrix [ENHN_LAYER] [iObj] = new Int [BLOCK_SQUARE_SIZE];
		rgppiInterQuantizerMatrix [BASE_LAYER] [iObj] = new Int [BLOCK_SQUARE_SIZE];
		rgppiInterQuantizerMatrix [ENHN_LAYER] [iObj] = new Int [BLOCK_SQUARE_SIZE];
		rgppiIntraQuantizerMatrixAlpha [BASE_LAYER] [iObj] = new Int [BLOCK_SQUARE_SIZE];
		rgppiIntraQuantizerMatrixAlpha [ENHN_LAYER] [iObj] = new Int [BLOCK_SQUARE_SIZE];
		rgppiInterQuantizerMatrixAlpha [BASE_LAYER] [iObj] = new Int [BLOCK_SQUARE_SIZE];
		rgppiInterQuantizerMatrixAlpha [ENHN_LAYER] [iObj] = new Int [BLOCK_SQUARE_SIZE];
		pchMVFileName[BASE_LAYER] [iObj] = NULL;
		pchMVFileName[ENHN_LAYER] [iObj] = NULL;
		pchNewpredRefName [BASE_LAYER][iObj] = NULL;
		pchNewpredRefName [ENHN_LAYER][iObj] = NULL;
		pchNewpredSlicePoint [BASE_LAYER][iObj] = NULL;
		pchNewpredSlicePoint [ENHN_LAYER][iObj] = NULL;

		// per object parameters
		GetSVal(&par, "Scalability", iObj, &pchTmp);
		if(strcmp(pchTmp, "Temporal")==0)
		{
			bAnyScalability = TRUE;
			rgbScalability[iObj] = TEMPORAL_SCALABILITY;

		}
		else if(strcmp(pchTmp, "Spatial")==0)
		{
			bAnyScalability = TRUE;
			rgbScalability[iObj] = SPATIAL_SCALABILITY;
		}
		else if(strcmp(pchTmp, "None")==0)
		{
			bAnyScalability = FALSE;
			rgbScalability[iObj] = NO_SCALABILITY;
		}
		else
			fatal_error("Unknown scalability type");

		if(rgbScalability[iObj] == SPATIAL_SCALABILITY || rgbScalability[iObj] == TEMPORAL_SCALABILITY)
			rgbSpatialScalability[iObj] = TRUE; 
		else 
			rgbSpatialScalability[iObj] = FALSE; 
	}

	//coded added by Sony, only deals with ONE VO.
	//Type option of Spatial Scalable Coding
	//This parameter is used for dicision VOP prediction types of Enhancement layer in Spatial Scalable Coding
	//If this option is set to 0, Enhancement layer is coded as "PPPPPP......",
	//else if set to 1 ,It's coded as "PBBBB......."

	if(rgbScalability[0] == SPATIAL_SCALABILITY)
	{
		fatal_error("Spatial scalability currently only works with one VO", nVO==1);
		
		GetSVal(&par, "Scalability.Spatial.PredictionType", 0, &pchTmp);
		if(strcmp(pchTmp, "PPP")==0)
			iSpatialOption = 1;
		else if(strcmp(pchTmp, "PBB")==0)
			iSpatialOption = 0;
		else
			fatal_error("Unknown spatial scalability prediction type");
	
		if (iSpatialOption == 1)
				fprintf(stdout,"Enhancement layer is coded as \"PPPPP.....\"\n");
		else
				fprintf(stdout,"Enhancement layer is coded as \"PBBBB.....\"\n");
	}

	if(bAnyScalability)
	{
		//Load enhancement layer (Spatial Scalable) size
		// should really be inside above if-statement
		GetIVal(&par, "Scalability.Spatial.Width", 0, (Int *)&uiFrmWidth_SS);
		GetIVal(&par, "Scalability.Spatial.Height", 0, (Int *)&uiFrmHeight_SS);
		
		//load upsampling factor 
		GetIVal(&par, "Scalability.Spatial.HorizFactor.N", 0, (Int *)&uiHor_sampling_n);
		GetIVal(&par, "Scalability.Spatial.HorizFactor.M", 0, (Int *)&uiHor_sampling_m);
		GetIVal(&par, "Scalability.Spatial.VertFactor.N", 0, (Int *)&uiVer_sampling_n);
		GetIVal(&par, "Scalability.Spatial.VertFactor.M", 0, (Int *)&uiVer_sampling_m);

		if(iVersion>=902)
		{
			// version 2 parameters
			readBoolParam(&par, "Scalability.Spatial.UseRefShape.Enable", 0, (Bool *)&uiUseRefShape);
			readBoolParam(&par, "Scalability.Spatial.UseRefTexture.Enable", 0, (Bool *)&uiUseRefTexture);
			GetIVal(&par, "Scalability.Spatial.Shape.HorizFactor.N", 0, (Int *)&uiHor_sampling_n_shape);
			GetIVal(&par, "Scalability.Spatial.Shape.HorizFactor.M", 0, (Int *)&uiHor_sampling_m_shape);
			GetIVal(&par, "Scalability.Spatial.Shape.VertFactor.N", 0, (Int *)&uiVer_sampling_n_shape);
			GetIVal(&par, "Scalability.Spatial.Shape.VertFactor.M", 0, (Int *)&uiVer_sampling_m_shape);
//OBSSFIX_MODE3
			if(iVersion >=904){
				GetSVal(&par, "Scalability.Spatial.EnhancementType", 0, &pchTmp);
				if(strcmp(pchTmp, "Full") == 0)
					rgiEnhancementTypeSpatial [0] = 0;
				else if(strcmp(pchTmp, "PartC") == 0)
					rgiEnhancementTypeSpatial [0] = 1;
				else if(strcmp(pchTmp, "PartNC") == 0)
					rgiEnhancementTypeSpatial [0] = 2;
				else
					fatal_error("Unknown Spatial Scalability Enhancement Type.");
			}
//~OBSSFIX_MODE3
		}
		else
		{
			// version 2 default values
			uiUseRefShape = 0;
			uiUseRefTexture = 0;
			uiHor_sampling_n_shape = 0;
			uiHor_sampling_m_shape = 0;
			uiVer_sampling_n_shape = 0;
			uiVer_sampling_m_shape = 0;
//OBSSFIX_MODE3
			rgiEnhancementType[0] = 0;
			rgiEnhancementTypeSpatial[0] = 0;
//~OBSSFIX_MODE3
		}
	}

	for (iObj = 0; iObj < nVO; iObj++)
	{
		if(bAnyScalability)
		{
			// temporal scalability
			// form of temporal scalability indicators
			// case 0  Enhn    P   P   ....
			//         Base  I   P   P ....
			// case 1  Enhn    B B   B B   ....
			//         Base  I     P     P ....
			// case 2  Enhn    P   B   B   ....
			//         Base  I   B   P   B ....
			GetIVal(&par, "Scalability.Temporal.PredictionType", iObj, &rgiTemporalScalabilityType [iObj]);
			fatal_error("Scalability.Temporal.PredictionType must range from 0 to 4",
				rgiTemporalScalabilityType [iObj]>=0 && rgiTemporalScalabilityType [iObj]<=4);

			GetSVal(&par, "Scalability.Temporal.EnhancementType", iObj, &pchTmp);
			if(strcmp(pchTmp, "Full")==0)
				rgiEnhancementType [iObj] = 0;
			else if(strcmp(pchTmp, "PartC")==0)
				rgiEnhancementType [iObj] = 1;
			else if(strcmp(pchTmp, "PartNC")==0)
				rgiEnhancementType [iObj] = 2;
			else
				fatal_error("Unknown temporal scalability enhancement type");
		}

		// alpha usage
		GetSVal(&par, "Alpha.Type", iObj, &pchTmp);
		rgbShapeOnly[iObj] = FALSE;
		rgiAlphaShapeExtension[iObj] = -1;  // default

		if(strcmp(pchTmp, "None")==0)
			rgfAlphaUsage[iObj] = RECTANGLE;
		else if(strcmp(pchTmp, "Binary")==0)
			rgfAlphaUsage[iObj] = ONE_BIT;
		else if(strcmp(pchTmp, "Gray")==0)
		{
			rgfAlphaUsage[iObj] = EIGHT_BIT;
			if(iVersion>=903)
			{
				Bool bMACEnable;
				readBoolParam(&par, "Alpha.MAC.Enable", iObj, &bMACEnable);
				GetIVal(&par, "Alpha.ShapeExtension", iObj, &rgiAlphaShapeExtension[iObj]); // MAC-V2 (SB)
				if(!bMACEnable)
					rgiAlphaShapeExtension[iObj] = -1;
			}
		}
		else if(strcmp(pchTmp, "ShapeOnly")==0)
		{
			rgbShapeOnly[iObj] = TRUE;
			rgfAlphaUsage[iObj] = ONE_BIT;
		}
		else
			fatal_error("Unknown alpha type");
		
		GetIVal(&par, "Alpha.Binary.RoundingThreshold", iObj, &rgiBinaryAlphaTH [iObj]);
		fatal_error("Binary rounding threshold must be >= 0", rgiBinaryAlphaTH [iObj] >= 0);

		if(iVersion>=905)
		{
			GetIVal(&par, "Alpha.Binary.FromGrayThreshold", iObj, &rgiGrayToBinaryTH [iObj]);
			fatal_error("Binary from Gray threshold must be >= 0 (beware of the grays!)",
				rgiGrayToBinaryTH [iObj] >= 0);
		}
		else
		{
			rgiGrayToBinaryTH [iObj] = GRAY_ALPHA_THRESHOLD;
		}

		Bool bSizeConvEnable;
		readBoolParam(&par, "Alpha.Binary.SizeConversion.Enable", iObj, &bSizeConvEnable);
		rgbNoCrChange [iObj] = !bSizeConvEnable;
		if (rgiBinaryAlphaTH [iObj] == 0 && rgbNoCrChange [iObj] == 0)
		{
			printf("Size conversion was disabled since binary rounding threshold is zero");
			rgbNoCrChange [iObj] = TRUE;
		}

		GetIVal(&par, "ErrorResil.AlphaRefreshRate", iObj, &rgiBinaryAlphaRR [iObj]);
		fatal_error("Alpha refresh rate must be >= 0", rgiBinaryAlphaRR [iObj] >= 0);

		Bool bRoundingControlEnable;
		readBoolParam(&par, "Motion.RoundingControl.Enable", iObj, &bRoundingControlEnable);
		rgbRoundingControlDisable [iObj] = !bRoundingControlEnable;

		GetIVal(&par, "Motion.RoundingControl.StartValue", iObj, &rgiInitialRoundingType [iObj]);
		fatal_error("Rounding control start value must be either 0 or 1",
			rgiInitialRoundingType [iObj]==0 || rgiInitialRoundingType [iObj]==1);

		GetIVal(&par, "Motion.PBetweenICount", iObj, &rgiNumPbetweenIVOP [iObj]);
		if(rgiNumPbetweenIVOP [iObj] < 0)
			rgiNumPbetweenIVOP [iObj] = (lastFrm - firstFrm + 1);		//only 1 I at the beginning
		
		GetIVal(&par, "Motion.BBetweenPCount", iObj, &rgiNumBbetweenPVOP [iObj]);
		fatal_error("Motion.BBetweenPCount must be >= 0", rgiNumBbetweenPVOP [iObj]>=0);

		//number of VOP between GOV header
		//CAUTION:GOV period is not used in spatial scalable coding
		Bool bGOVEnable;
		readBoolParam(&par, "GOV.Enable", iObj, &bGOVEnable);
		if(!bGOVEnable)
			rgiGOVperiod[iObj] = 0;
		else
			GetIVal(&par, "GOV.Period", iObj, &rgiGOVperiod[iObj]);

		Bool bDeblockingFilterEnable;
		readBoolParam(&par, "Motion.DeblockingFilter.Enable", iObj, &bDeblockingFilterEnable);
		rgbDeblockFilterDisable[iObj] = !bDeblockingFilterEnable;

		GetSVal(&par, "Source.Format", iObj, &pchTmp);
		if(strcmp(pchTmp, "444")==0)
			rgfChrType[iObj] = FOUR_FOUR_FOUR;
		else if(strcmp(pchTmp, "422")==0)
			rgfChrType[iObj] = FOUR_TWO_TWO;
		else if(strcmp(pchTmp, "420")==0)
			rgfChrType[iObj] = FOUR_TWO_ZERO;
		else
			fatal_error("Unrecognised source format");

		if(iVersion>=902)
		{
			// version 2 parameters
			GetIVal(&par, "VersionID", iObj, (Int *)&rguiVerID[iObj]);
		}
		else
		{
			// default parameter values
			rguiVerID[iObj] = 1;
		}
		
		GetSVal(&par, "Sprite.Type", iObj, &pchTmp);
		if(strcmp(pchTmp, "None")==0)
			rguiSpriteUsage[iObj] = 0;
		else if(strcmp(pchTmp, "Static")==0)
			rguiSpriteUsage[iObj] = 1;
		else if(strcmp(pchTmp, "GMC")==0)
			rguiSpriteUsage[iObj] = 2;
		else
			fatal_error("Unknown value for Sprite.Type");
		if (rgbScalability [iObj] == TRUE && rguiSpriteUsage[iObj]==1)
			fatal_error("Sprite and scalability modes cannot be combined");
		if (rguiVerID[iObj]==1 && rguiSpriteUsage[iObj]==2)
			fatal_error("GMC cannot be enabled in a Version 1 stream");

		GetSVal(&par, "Sprite.WarpAccuracy", iObj, &pchTmp);
		if(strcmp(pchTmp, "1/2")==0)
			rguiWarpingAccuracy[iObj] = 0;
		else if(strcmp(pchTmp, "1/4")==0)
			rguiWarpingAccuracy[iObj] = 1;
		else if(strcmp(pchTmp, "1/8")==0)
			rguiWarpingAccuracy[iObj] = 2;
		else if(strcmp(pchTmp, "1/16")==0)
			rguiWarpingAccuracy[iObj] = 3;
		else
			fatal_error("Unknown value for Sprite.WarpAccuracy");

		GetIVal(&par, "Sprite.Points", iObj, &rgiNumPnts[iObj]);
		if(rguiSpriteUsage[iObj] == 2)
			fatal_error("Sprite.Points must lie in the range 0 to 3", rgiNumPnts[iObj]>=0 && rgiNumPnts[iObj]<4);
		else
			fatal_error("Sprite.Points must lie in the range -1 to 4", rgiNumPnts[iObj]>=-1 && rgiNumPnts[iObj]<5);

		GetSVal(&par, "Sprite.Mode", iObj, &pchTmp);
		if(strcmp(pchTmp, "Basic")==0)
			rgSpriteMode[iObj] = BASIC_SPRITE;
		else if(strcmp(pchTmp, "LowLatency")==0)
			rgSpriteMode[iObj] = LOW_LATENCY;
		else if(strcmp(pchTmp, "PieceObject")==0)
			rgSpriteMode[iObj] = PIECE_OBJECT;
		else if(strcmp(pchTmp, "PieceUpdate")==0)
			rgSpriteMode[iObj] = PIECE_UPDATE;
		else
			fatal_error("Unkown value for Sprite.Mode");

		if(rguiSpriteUsage[iObj]==0)
		{
			rgiNumPnts[iObj] = -1;
			//rgSpriteMode[iObj] = -1;
		}

		readBoolParam(&par, "Trace.CreateFile.Enable", iObj, &rgbTrace[iObj]);
		readBoolParam(&par, "Trace.DetailedDump.Enable", iObj, &rgbDumpMB[iObj]);

#ifndef __TRACE_AND_STATS_
		if(rgbTrace[iObj])
		{
			printf("Warning: this encoder will not generate trace information.\n");
			printf("If you want trace information, re-compile with __TRACE_AND_STATS_ defined.\n");
		}
#endif

		readBoolParam(&par, "Motion.SkippedMB.Enable", iObj, &rgbAllowSkippedPMBs[iObj]);



		// parameters that can be different for each scalability layer
		Int iLayer, iLayerCount;
		if(rgbScalability[iObj]!=NO_SCALABILITY)
			iLayerCount = 2;
		else
			iLayerCount = 1;

		for(iLayer = 0; iLayer<iLayerCount; iLayer++)
		{
			Int iLayerObj = iObj + iLayer * nVO;

			// rate control
			GetSVal(&par, "RateControl.Type", iLayerObj, &pchTmp);
			if(strcmp(pchTmp, "None")==0)
				rguiRateControl [iLayer][iObj] = 0;
			else if(strcmp(pchTmp, "MP4")==0)
				rguiRateControl [iLayer][iObj] = RC_MPEG4;
			else if(strcmp(pchTmp, "TM5")==0)
				rguiRateControl [iLayer][iObj] = RC_TM5;
			else
				fatal_error("Unknown rate control type");

			// bit budget
			Double dVal;
			Int er = par.GetDouble("RateControl.BitsPerSecond", iLayerObj, &dVal);
			if(er!=ERES_OK)
			{
				// previously was erroneously called bitspervop, but it should be per second
				// the value is still the same
				er = par.GetDouble("RateControl.BitsPerVOP", iLayerObj, &dVal);
				if(er!=ERES_OK)
					printf("Error: can't find RateControl.BitsPerSecond in parameter file.\n");
			}
			rguiBitsBudget[iLayer][iObj] = (Int)dVal;

			Bool bRVLCEnable, bDPEnable, bVPEnable;
			readBoolParam(&par, "ErrorResil.RVLC.Enable", iLayerObj, &bRVLCEnable);
			readBoolParam(&par, "ErrorResil.DataPartition.Enable", iLayerObj, &bDPEnable);
			readBoolParam(&par, "ErrorResil.VideoPacket.Enable", iLayerObj, &bVPEnable);
			if(bDPEnable)
				bVPEnable = TRUE;
			
			rgbErrorResilientDisable [iLayer][iObj] = bVPEnable ? (iLayer!=0) : TRUE;
			rgbReversibleVlc [iLayer] [iObj] = bRVLCEnable ? (iLayer==0) : FALSE;
			rgbDataPartitioning [iLayer][iObj] = bDPEnable ? (iLayer==0) : FALSE;

			if(rgbErrorResilientDisable [BASE_LAYER][iObj])
				rgiBinaryAlphaRR [iObj] = -1;

			GetIVal(&par, "ErrorResil.VideoPacket.Length", iLayerObj, &rgiVPBitTh[iLayer][iObj]);
			if(rgbErrorResilientDisable [iLayer][iObj])
				rgiVPBitTh [iLayer][iObj] = -1;

			readBoolParam(&par, "Motion.Interlaced.Enable", iLayerObj, &rgbInterlacedCoding [iLayer][iObj]);

			GetSVal(&par, "Quant.Type", iLayerObj, &pchTmp);
			if(strcmp(pchTmp, "H263")==0)
				rgfQuant[iLayer][iObj] = Q_H263;
			else if(strcmp(pchTmp, "MPEG")==0)
				rgfQuant[iLayer][iObj] = Q_MPEG;
			else
				fatal_error("Unknown quantizer type");

			readBoolParam(&par, "Texture.QuantMatrix.Intra.Enable", iLayerObj, &rgbLoadIntraMatrix [iLayer][iObj]);
			if(rgbLoadIntraMatrix [iLayer][iObj])
			{
				Double *pdTmp;
				Int iCount;
				GetAVal(&par, "Texture.QuantMatrix.Intra", iLayerObj, &pdTmp, &iCount);
				fatal_error("Wrong number of intra quant matrix values (should be 64)", iCount<=64);
				Int i;
				for(i=0; i<iCount; i++)
					rgppiIntraQuantizerMatrix[iLayer][iObj][i] = (Int)pdTmp[i];
				if(iCount<64)
					rgppiIntraQuantizerMatrix[iLayer][iObj][iCount] = 0;
			}
			else
				memcpy (rgppiIntraQuantizerMatrix [iLayer][iObj], rgiDefaultIntraQMatrix, BLOCK_SQUARE_SIZE * sizeof(Int));
		
			readBoolParam(&par, "Texture.QuantMatrix.Inter.Enable", iLayerObj, &rgbLoadInterMatrix [iLayer][iObj]);
			if(rgbLoadInterMatrix [iLayer][iObj])
			{
				Double *pdTmp;
				Int iCount;
				GetAVal(&par, "Texture.QuantMatrix.Inter", iLayerObj, &pdTmp, &iCount);
				fatal_error("Wrong number of inter quant matrix values (should be 64)", iCount<=64);
				Int i;
				for(i=0; i<iCount; i++)
					rgppiInterQuantizerMatrix[iLayer][iObj][i] = (Int)pdTmp[i];
				if(iCount<64)
					rgppiInterQuantizerMatrix[iLayer][iObj][iCount] = 0;
			}
			else
				memcpy (rgppiInterQuantizerMatrix [iLayer][iObj], rgiDefaultInterQMatrix, BLOCK_SQUARE_SIZE * sizeof(Int));
		
			GetIVal(&par, "Texture.IntraDCThreshold", iLayerObj, &rgiIntraDCSwitchingThr [iLayer] [iObj]);
			fatal_error("Intra DC Threshold must be in range 0 - 7",
				rgiIntraDCSwitchingThr [iLayer] [iObj] >= 0 && rgiIntraDCSwitchingThr [iLayer] [iObj] <= 7);
		
			GetIVal(&par, "Texture.QuantStep.IVOP", iLayerObj, &rgiIStep [iLayer] [iObj]);
			fatal_error("IVOP quantizer stepsize is out of range", 
				rgiIStep [iLayer] [iObj] > 0 && rgiIStep [iLayer] [iObj] < (1<<uiQuantPrecision));

			GetIVal(&par, "Texture.QuantStep.PVOP", iLayerObj, &rgiPStep [iLayer] [iObj]);
			fatal_error("PVOP quantizer stepsize is out of range", 
				rgiPStep [iLayer] [iObj] > 0 && rgiPStep [iLayer] [iObj] < (1<<uiQuantPrecision));

			GetIVal(&par, "Texture.QuantStep.BVOP", iLayerObj, &rgiStepBCode [iLayer] [iObj]);
			fatal_error("BVOP quantizer stepsize is out of range", 
				rgiStepBCode [iLayer] [iObj] > 0 && rgiStepBCode [iLayer] [iObj] < (1<<uiQuantPrecision));

			readBoolParam(&par, "Alpha.QuantMatrix.Intra.Enable", iLayerObj, &rgbLoadIntraMatrixAlpha [iLayer][iObj]);
			if(rgbLoadIntraMatrixAlpha [iLayer][iObj])
			{
				Double *pdTmp;
				Int iCount;
				GetAVal(&par, "Alpha.QuantMatrix.Intra", iLayerObj, &pdTmp, &iCount);
				fatal_error("Wrong number of alpha intra quant matrix values (should be 64)", iCount<=64);
				Int i;
				for(i=0; i<iCount; i++)
					rgppiIntraQuantizerMatrixAlpha[iLayer][iObj][i] = (Int)pdTmp[i];
				if(iCount<64)
					rgppiIntraQuantizerMatrixAlpha[iLayer][iObj][iCount] = 0;
			}
			else
				memcpy (rgppiIntraQuantizerMatrixAlpha [iLayer][iObj], rgiDefaultIntraQMatrixAlpha,
					BLOCK_SQUARE_SIZE * sizeof(Int));
		
			readBoolParam(&par, "Alpha.QuantMatrix.Inter.Enable", iLayerObj, &rgbLoadInterMatrixAlpha [iLayer][iObj]);
			if(rgbLoadInterMatrixAlpha [iLayer][iObj])
			{
				Double *pdTmp;
				Int iCount;
				GetAVal(&par, "Alpha.QuantMatrix.Inter", iLayerObj, &pdTmp, &iCount);
				fatal_error("Wrong number of alpha inter quant matrix values (should be 64)", iCount<=64);
				Int i;
				for(i=0; i<iCount; i++)
					rgppiInterQuantizerMatrixAlpha[iLayer][iObj][i] = (Int)pdTmp[i];
				if(iCount<64)
					rgppiInterQuantizerMatrixAlpha[iLayer][iObj][iCount] = 0;
			}
			else
				memcpy (rgppiInterQuantizerMatrixAlpha [iLayer][iObj], rgiDefaultInterQMatrixAlpha,
					BLOCK_SQUARE_SIZE * sizeof(Int));
			
			GetIVal(&par, "Alpha.QuantStep.IVOP", iLayerObj, &rgiIStepAlpha [iLayer] [iObj]);
			fatal_error("IVOP alpha quantizer stepsize is out of range", 
				rgiIStepAlpha [iLayer] [iObj] > 0 && rgiIStepAlpha [iLayer] [iObj] < 64);

			GetIVal(&par, "Alpha.QuantStep.PVOP", iLayerObj, &rgiPStepAlpha [iLayer] [iObj]);
			fatal_error("PVOP alpha quantizer stepsize is out of range", 
				rgiPStepAlpha [iLayer] [iObj] > 0 && rgiPStepAlpha [iLayer] [iObj] < 64);

			GetIVal(&par, "Alpha.QuantStep.BVOP", iLayerObj, &rgiBStepAlpha [iLayer] [iObj]);
			fatal_error("BVOP alpha quantizer stepsize is out of range", 
				rgiBStepAlpha [iLayer] [iObj] > 0 && rgiBStepAlpha [iLayer] [iObj] < 64);

			readBoolParam(&par, "Alpha.QuantDecouple.Enable", iLayerObj, &rgbNoGrayQuantUpdate [iLayer] [iObj]);

			Int iRate;
			GetIVal(&par, "Source.SamplingRate", iLayerObj, &iRate);
			fatal_error("Source sampling rate must be > 0", iRate>0);
			if(iLayer==BASE_LAYER)
				rgiTSRate[iObj] = iRate;
			else
				rgiEnhcTSRate[iObj] = iRate;

			GetIVal(&par, "Motion.SearchRange", iLayerObj, (Int *)&rguiSearchRange[iLayer][iObj]);
			fatal_error("Motion.SearchRange must be > 0", rguiSearchRange[iLayer][iObj]>0);

			readBoolParam(&par, "Motion.UseSourceForME.Enable", iLayerObj, &rgbOriginalME [iLayer] [iObj]);

			Bool bAdvPredEnable;
			readBoolParam(&par, "Motion.AdvancedPrediction.Enable", iLayerObj, &bAdvPredEnable);
			rgbAdvPredDisable [iLayer][iObj] = !bAdvPredEnable;

			Bool bComplexityEnable;
			readBoolParam(&par, "Complexity.Enable", iLayerObj, &bComplexityEnable);
			rgbComplexityEstimationDisable[iLayer][iObj] = !bComplexityEnable;

			if(bComplexityEnable)
			{
				readBoolParam(&par, "Complexity.Opaque.Enable", iLayerObj, &rgbOpaque[iLayer][iObj]);
				readBoolParam(&par, "Complexity.Transparent.Enable", iLayerObj, &rgbTransparent[iLayer][iObj]);
				readBoolParam(&par, "Complexity.IntraCAE.Enable", iLayerObj, &rgbIntraCAE[iLayer][iObj]);
				readBoolParam(&par, "Complexity.InterCAE.Enable", iLayerObj, &rgbInterCAE[iLayer][iObj]);
				readBoolParam(&par, "Complexity.NoUpdate.Enable", iLayerObj, &rgbNoUpdate[iLayer][iObj]);
				readBoolParam(&par, "Complexity.UpSampling.Enable", iLayerObj, &rgbUpsampling[iLayer][iObj]);
				readBoolParam(&par, "Complexity.IntraBlocks.Enable", iLayerObj, &rgbIntraBlocks[iLayer][iObj]);
				readBoolParam(&par, "Complexity.InterBlocks.Enable", iLayerObj, &rgbInterBlocks[iLayer][iObj]);
				readBoolParam(&par, "Complexity.Inter4VBlocks.Enable", iLayerObj, &rgbInter4vBlocks[iLayer][iObj]);
				readBoolParam(&par, "Complexity.NotCodedBlocks.Enable", iLayerObj, &rgbNotCodedBlocks[iLayer][iObj]);
				readBoolParam(&par, "Complexity.DCTCoefs.Enable", iLayerObj, &rgbDCTCoefs[iLayer][iObj]);
				readBoolParam(&par, "Complexity.DCTLines.Enable", iLayerObj, &rgbDCTLines[iLayer][iObj]);
				readBoolParam(&par, "Complexity.VLCSymbols.Enable", iLayerObj, &rgbVLCSymbols[iLayer][iObj]);
				readBoolParam(&par, "Complexity.VLCBits.Enable", iLayerObj, &rgbVLCBits[iLayer][iObj]);
				readBoolParam(&par, "Complexity.APM.Enable", iLayerObj, &rgbAPM[iLayer][iObj]);
				readBoolParam(&par, "Complexity.NPM.Enable", iLayerObj, &rgbNPM[iLayer][iObj]);
				readBoolParam(&par, "Complexity.InterpMCQ.Enable", iLayerObj, &rgbInterpolateMCQ[iLayer][iObj]);
				readBoolParam(&par, "Complexity.ForwBackMCQ.Enable", iLayerObj, &rgbForwBackMCQ[iLayer][iObj]);
				readBoolParam(&par, "Complexity.HalfPel2.Enable", iLayerObj, &rgbHalfpel2[iLayer][iObj]);
				readBoolParam(&par, "Complexity.HalfPel4.Enable", iLayerObj, &rgbHalfpel4[iLayer][iObj]);
				// START: Complexity Estimation syntax support - Update version 2 - Massimo Ravasi (EPFL) - 11 Nov 1999
				if(iVersion>=902) // version 2
					GetIVal(&par, "Complexity.EstimationMethod", iLayerObj, (Int *)(&rguiEstimationMethod[iLayer][iObj]));
				else
					rguiEstimationMethod[iLayer][iObj] = 0;					
				if(rguiEstimationMethod[iLayer][iObj] == 1)
				{
					readBoolParam(&par, "Complexity.SADCT.Enable", iLayerObj, &rgbSadct[iLayer][iObj]);
					readBoolParam(&par, "Complexity.QuarterPel.Enable", iLayerObj, &rgbQuarterpel[iLayer][iObj]);
				}
				else
				{
					rgbSadct[iLayer][iObj] = 0;
					rgbQuarterpel[iLayer][iObj] = 0;
 				}

				if ( ! (rgbOpaque[iLayer][iObj] ||
						rgbTransparent[iLayer][iObj] ||
						rgbIntraCAE[iLayer][iObj] ||
						rgbInterCAE[iLayer][iObj] ||
						rgbNoUpdate[iLayer][iObj] ||
						rgbUpsampling[iLayer][iObj] ||
						rgbIntraBlocks[iLayer][iObj] ||
						rgbInterBlocks[iLayer][iObj] ||
						rgbInter4vBlocks[iLayer][iObj] ||
						rgbNotCodedBlocks[iLayer][iObj] ||
						rgbDCTCoefs[iLayer][iObj] ||
						rgbDCTLines[iLayer][iObj] ||
						rgbVLCSymbols[iLayer][iObj] ||
						rgbVLCBits[iLayer][iObj] ||
						rgbAPM[iLayer][iObj] ||
						rgbNPM[iLayer][iObj] ||
						rgbInterpolateMCQ[iLayer][iObj] ||
						rgbForwBackMCQ[iLayer][iObj] ||
						rgbHalfpel2[iLayer][iObj] ||
						rgbHalfpel4[iLayer][iObj] ||
 						rguiEstimationMethod[iLayer][iObj] ||
						rgbSadct[iLayer][iObj] ||
						rgbQuarterpel[iLayer][iObj]) ) {
					fprintf(stderr,"Complexity estimation is enabled,\n"
						           "but no corresponding flag is enabled.");
					exit(1);
				}
				// END: Complexity Estimation syntax support - Update version 2
			}
			else
			{
				rgbOpaque[iLayer][iObj] = 0;
				rgbTransparent[iLayer][iObj] = 0;
				rgbIntraCAE[iLayer][iObj] = 0;
				rgbInterCAE[iLayer][iObj] = 0;
				rgbNoUpdate[iLayer][iObj] = 0;
				rgbUpsampling[iLayer][iObj] = 0;
				rgbIntraBlocks[iLayer][iObj] = 0;
				rgbInterBlocks[iLayer][iObj] = 0;
				rgbInter4vBlocks[iLayer][iObj] = 0;
				rgbNotCodedBlocks[iLayer][iObj] = 0;
				rgbDCTCoefs[iLayer][iObj] = 0;
				rgbDCTLines[iLayer][iObj] = 0;
				rgbVLCSymbols[iLayer][iObj] = 0;
				rgbVLCBits[iLayer][iObj] = 0;
				rgbAPM[iLayer][iObj] = 0;
				rgbNPM[iLayer][iObj] = 0;
				rgbInterpolateMCQ[iLayer][iObj] = 0;
				rgbForwBackMCQ[iLayer][iObj] = 0;
				rgbHalfpel2[iLayer][iObj] = 0;
				rgbHalfpel4[iLayer][iObj] = 0;
				// START: Complexity Estimation syntax support - Update version 2 - Massimo Ravasi (EPFL) - 11 Nov 1999
				rguiEstimationMethod[iLayer][iObj] = 0;
				rgbSadct[iLayer][iObj] = 0;
				rgbQuarterpel[iLayer][iObj] = 0;
				// END: Complexity Estimation syntax support - Update version 2
			}

			GetDVal(&par, "Source.FrameRate", iLayerObj, &rgdFrameFrequency[iLayer][iObj]);
			fatal_error("Source.FrameRate is invalid", rgdFrameFrequency[iLayer][iObj] > 0.0);

			readBoolParam(&par, "Motion.Interlaced.TopFieldFirst.Enable", iLayerObj, &rgbTopFieldFirst [iLayer] [iObj]);
			
			readBoolParam(&par, "Motion.Interlaced.AlternativeScan.Enable", iLayerObj, &rgbAlternateScan[iLayer][iObj]);

			GetIVal(&par, "Motion.SearchRange.DirectMode", iLayerObj, &rgiDirectModeRadius[iLayer][iObj]);

			GetSVal(&par, "Motion.ReadWriteMVs", iLayerObj, &pchTmp);
			if(strcmp(pchTmp, "Off")==0)
				rgiMVFileUsage[iLayer][iObj] = 0;
			else if(strcmp(pchTmp, "Read")==0)
				rgiMVFileUsage[iLayer][iObj] = 1;
			else if(strcmp(pchTmp, "Write")==0)
				rgiMVFileUsage[iLayer][iObj] = 2;
			else
				fatal_error("Unknown mode for Motion.ReadWriteMVs");

			if(rgiMVFileUsage[iLayer][iObj]!=0)
			{
				GetSVal(&par, "Motion.ReadWriteMVs.Filename", iLayerObj, &pchTmp);
				iLen = strlen(pchTmp) + 1;
				pchMVFileName [iLayer] [iObj] = new char [iLen];
				memcpy(pchMVFileName [iLayer] [iObj], pchTmp, iLen);
			}

			Bool bVolControlEnable;
			readBoolParam(&par, "VOLControl.Enable", iLayerObj, &bVolControlEnable);
			rguiVolControlParameters[iLayer][iObj] = (UInt)bVolControlEnable;

			GetIVal(&par, "VOLControl.ChromaFormat", iLayerObj, (Int *)&rguiChromaFormat[iLayer][iObj]);
			GetIVal(&par, "VOLControl.LowDelay", iLayerObj, (Int *)&rguiLowDelay[iLayer][iObj]);

			Bool bVBVParams;
			readBoolParam(&par, "VOLControl.VBVParams.Enable", iLayerObj, &bVBVParams);
			rguiVBVParams[iLayer][iObj] = (UInt)bVBVParams;

			GetIVal(&par, "VOLControl.Bitrate", iLayerObj, (Int *)&rguiBitRate[iLayer][iObj]);
			GetIVal(&par, "VOLControl.VBVBuffer.Size", iLayerObj, (Int *)&rguiVbvBufferSize[iLayer][iObj]);
			GetIVal(&par, "VOLControl.VBVBuffer.Occupancy", iLayerObj, (Int *)&rguiVbvBufferOccupany[iLayer][iObj]);

			// version 2 start
			if(iVersion>=902)
			{
				readBoolParam(&par, "Newpred.Enable", iLayerObj, &rgbNewpredEnable [iLayer][iObj]);
				
				GetSVal(&par, "Newpred.SegmentType", iLayerObj, &pchTmp);
				if(strcmp(pchTmp, "VideoPacket")==0)
					rgbNewpredSegType [iLayer][iObj] = 0;
				else if(strcmp(pchTmp, "VOP")==0)
					rgbNewpredSegType [iLayer][iObj] = 1;
				else
					fatal_error("Unknown value for Newpred.SegmentType");

				GetSVal(&par, "Newpred.Filename", iLayerObj, &pchTmp);
				iLen = strlen(pchTmp) + 1;
				pchNewpredRefName [iLayer][iObj] = new char [iLen];
				memcpy(pchNewpredRefName [iLayer][iObj], pchTmp, iLen);

				GetSVal(&par, "Newpred.SliceList", iLayerObj, &pchTmp);
				iLen = strlen(pchTmp) + 1;
				pchNewpredSlicePoint [iLayer][iObj] = new char [iLen];
				memcpy(pchNewpredSlicePoint [iLayer][iObj], pchTmp, iLen);

				Bool bTmp;
				readBoolParam(&par, "Texture.SADCT.Enable", iLayerObj, &bTmp);
				rgbSadctDisable [iLayer][iObj] = !bTmp;

				readBoolParam(&par, "Motion.QuarterSample.Enable", iLayerObj, &rgbQuarterSample [iLayer] [iObj]);

				readBoolParam(&par, "RRVMode.Enable", iLayerObj, &bTmp);
				RRVmode[iLayer][iObj].iOnOff = (Int)bTmp;

				GetIVal(&par, "RRVMode.Cycle", iLayerObj, &RRVmode[iLayer][iObj].iCycle);

				// START: Complexity Estimation syntax support - Update version 2 - Massimo Ravasi (EPFL) - 11 Nov 1999
				// moved in if(bComplexityEnable) { ... } block above
				/*if(bComplexityEnable)
				{
					// complexity estimation
					GetIVal(&par, "Complexity.EstimationMethod", iLayerObj, (Int *)(&rguiEstimationMethod[iLayer][iObj]));
					readBoolParam(&par, "Complexity.SADCT.Enable", iLayerObj, &rgbSadct[iLayer][iObj]);
					readBoolParam(&par, "Complexity.QuarterPel.Enable", iLayerObj, &rgbQuarterpel[iLayer][iObj]);
				}
				*/
				// END: Complexity Estimation syntax support - Update version 2
			}
			else
			{
				rgbNewpredEnable [iLayer][iObj] = 0;
				rgbNewpredSegType [iLayer][iObj] = 0;

				rgbSadctDisable [iLayer][iObj] = 1;
				
				rgbQuarterSample [iLayer][iObj] = 0;

				RRVmode[iLayer][iObj].iOnOff = 0;
				RRVmode[iLayer][iObj].iCycle = 0;

				// START: Complexity Estimation syntax support - Update version 2 - Massimo Ravasi (EPFL) - 11 Nov 1999
				// moved in if(bComplexityEnable) { ... } block above
				/*
				rguiEstimationMethod[iLayer][iObj] = 0;
				rgbSadct[iLayer][iObj] = 0;
				rgbQuarterpel[iLayer][iObj] = 0;
				*/
				// END: Complexity Estimation syntax support - Update version 2
			}
			// version 2 end
		}

	}

	// should have used this at top instead of all the extra variables
	// but too much work now to change - swinder
	SessionEncoderArgList args;

	args.uiFrmWidth				= uiFrmWidth;			// frame width
	args.uiFrmHeight			= uiFrmHeight;			// frame height
	args.iFirstFrm				= firstFrm;			// first frame number
	args.iLastFrm				= lastFrm;				// last frame number
	args.bNot8Bit				= bNot8Bit;				// NBIT
	args.uiQuantPrecision		= uiQuantPrecision;			// NBIT
	args.nBits					= nBits;				// NBIT
	args.iFirstVO				= firstVO;				// first VOP index
	args.iLastVO				= lastVO;				// last VOP index
	args.rguiVerID				= rguiVerID;			// Version ID // GMC
	args.rgbSpatialScalability	= rgbSpatialScalability; // spatial scalability indicator
	args.rgiTemporalScalabilityType	= rgiTemporalScalabilityType; // temporal scalability formation case
	args.rgiEnhancementType		= rgiEnhancementType;  // enhancement_type for scalability
//OBSSFIX_MODE3
	args.rgiEnhancementTypeSpatial= rgiEnhancementTypeSpatial;  // enhancement_type for scalability
//~OBSSFIX_MODE3
	args.rguiRateControl		= rguiRateControl;	// rate control type
	args.rguiBudget				= rguiBitsBudget;		// for rate control
	args.rgfAlphaUsage			= rgfAlphaUsage;// alpha usage for each VOP.  0: binary, 1: 8-bit
	args.rgiAlphaShapeExtension = rgiAlphaShapeExtension; // MAC-V2 (SB)
	args.rgbShapeOnly			= rgbShapeOnly;	// shape only mode
	args.rgiBinaryAlphaTH		= rgiBinaryAlphaTH;
	args.rgiBinaryAlphaRR		= rgiBinaryAlphaRR;		// refresh rate
	args.rgiGrayToBinaryTH		= rgiGrayToBinaryTH;
	args.rgbNoCrChange			= rgbNoCrChange;
	args.rguiSearchRange		= rguiSearchRange;			// motion search range
	args.rgbOriginalForME		= rgbOriginalME;		// flag indicating whether use the original previous VOP for ME
	args.rgbAdvPredDisable		= rgbAdvPredDisable;		// no advanced MC (currenly = obmc, later = obmc + 8x8)
    args.rgbQuarterSample		= rgbQuarterSample;        // QuarterSample, mwi
	
	args.bCodeSequenceHead		= bCodeSequenceHead;
	args.uiProfileAndLevel		= uiProfileAndLevel;

	// complexity estimation
	args.rgbComplexityEstimationDisable	= rgbComplexityEstimationDisable;
	args.rguiEstimationMethod	= rguiEstimationMethod;
	args.rgbOpaque				= rgbOpaque;
	args.rgbTransparent			= rgbTransparent;
	args.rgbIntraCAE			= rgbIntraCAE;
	args.rgbInterCAE			= rgbInterCAE;
	args.rgbNoUpdate			= rgbNoUpdate;
	args.rgbUpsampling			= rgbUpsampling;
	args.rgbIntraBlocks			= rgbIntraBlocks;
	args.rgbInterBlocks			= rgbInterBlocks;
	args.rgbInter4vBlocks		= rgbInter4vBlocks;
	args.rgbNotCodedBlocks		= rgbNotCodedBlocks;
	args.rgbDCTCoefs			= rgbDCTCoefs;
	args.rgbDCTLines			= rgbDCTLines;
	args.rgbVLCSymbols			= rgbVLCSymbols;
	args.rgbVLCBits				= rgbVLCBits;
	args.rgbAPM					= rgbAPM;
	args.rgbNPM					= rgbNPM;
	args.rgbInterpolateMCQ		= rgbInterpolateMCQ;
	args.rgbForwBackMCQ			= rgbForwBackMCQ;
	args.rgbHalfpel2			= rgbHalfpel2;
	args.rgbHalfpel4			= rgbHalfpel4;
	args.rgbSadct				= rgbSadct;
	args.rgbQuarterpel			= rgbQuarterpel;

	args.rguiVolControlParameters	= rguiVolControlParameters;
	args.rguiChromaFormat		= rguiChromaFormat;
	args.rguiLowDelay			= rguiLowDelay;
	args.rguiVBVParams			= rguiVBVParams;
	args.rguiBitRate			= rguiBitRate;
	args.rguiVbvBufferSize		= rguiVbvBufferSize;
	args.rguiVbvBufferOccupany	= rguiVbvBufferOccupany;
	args.rgdFrameFrequency		= rgdFrameFrequency;	// Frame Frequency
	args.rgbInterlacedCoding	= rgbInterlacedCoding;	// interlace coding flag
	args.rgbTopFieldFirst		= rgbTopFieldFirst;	// top field first flag
    args.rgbAlternateScan		= rgbAlternateScan;    // alternate scan flag
	args.rgbSadctDisable		= rgbSadctDisable;
	args.rgiDirectModeRadius	= rgiDirectModeRadius;	// direct mode search radius
	args.rgiMVFileUsage			= rgiMVFileUsage;		// 0- not used, 1: read from motion file, 2- write to motion file
	args.pchMVFileName			= &pchMVFileName[0];		// Motion vector filenames
	args.rgbNewpredEnable			= rgbNewpredEnable;
	args.rgbNewpredSegmentType	= rgbNewpredSegType;
	args.rgcNewpredRefName		= pchNewpredRefName;
	args.rgcNewpredSlicePoint		= pchNewpredSlicePoint;
  	args.RRVmode				= RRVmode;

//RESYNC_MARKER_FIX
	args.rgbResyncMarkerDisable = rgbErrorResilientDisable;
//~RESYNC_MARKER_FIX

	args.rgbVPBitTh				= rgiVPBitTh;				// Bit threshold for video packet spacing control
	args.rgbDataPartitioning	= rgbDataPartitioning;		// data partitioning
	args.rgbReversibleVlc		= rgbReversibleVlc;			// reversible VLC
	args.rgfQuant				= rgfQuant; // quantizer selection, either H.263 or MPEG
	args.rgbLoadIntraMatrix		= rgbLoadIntraMatrix; // load user-defined intra Q-Matrix
	args.rgppiIntraQuantizerMatrix	= rgppiIntraQuantizerMatrix; // Intra Q-Matrix
	args.rgbLoadInterMatrix		= rgbLoadInterMatrix; // load user-defined inter Q-Matrix
	args.rgppiInterQuantizerMatrix	= rgppiInterQuantizerMatrix; // Inter Q-Matrix
	args.rgiIntraDCSwitchingThr	= rgiIntraDCSwitchingThr;		//threshold to code dc as ac when pred. is on
	args.rgiStepI				= rgiIStep; // I-VOP quantization stepsize
	args.rgiStepP				= rgiPStep; // P-VOP quantization stepsize
	args.rgbLoadIntraMatrixAlpha	= rgbLoadIntraMatrixAlpha;
	args.rgppiIntraQuantizerMatrixAlpha	= rgppiIntraQuantizerMatrixAlpha;
	args.rgbLoadInterMatrixAlpha	= rgbLoadInterMatrixAlpha;
	args.rgppiInterQuantizerMatrixAlpha	= rgppiInterQuantizerMatrixAlpha;
	args.rgiStepIAlpha			= rgiIStepAlpha; // I-VOP quantization stepsize for Alpha
	args.rgiStepPAlpha			= rgiPStepAlpha; // P-VOP quantization stepsize for Alpha
	args.rgiStepBAlpha			= rgiBStepAlpha; // B-VOP quantization stepsize for Alpha
	args.rgbNoAlphaQuantUpdate	= rgbNoGrayQuantUpdate; // discouple gray quant update with tex. quant
	args.rgiStepB				= rgiStepBCode; // quantization stepsize for B-VOP
	args.rgiNumOfBbetweenPVOP	= rgiNumBbetweenPVOP;		// no of B-VOPs between P-VOPs
	args.rgiNumOfPbetweenIVOP	= rgiNumPbetweenIVOP;		// no of P-VOPs between I-VOPs
	args.rgiGOVperiod			= rgiGOVperiod;
	args.rgbDeblockFilterDisable	= rgbDeblockFilterDisable; //deblocking filter disable
	args.rgbAllowSkippedPMBs	= rgbAllowSkippedPMBs;
	args.pchPrefix				= pchPrefix; // prefix name of the movie
	args.pchBmpFiles			= pchBmpDir; // bmp file directory location
	args.rgfChrType				= rgfChrType; // input chrominance type. 0 - 4:4:4, 1 - 4:2:2, 0 - 4:2:0
	args.pchOutBmpFiles			= pchOutBmpDir; // quantized frame file directory
	args.pchOutStrFiles			= pchOutStrFile; // output bitstream file
	args.rgiTemporalRate		= rgiTSRate; // temporal subsampling rate
	args.rgiEnhcTemporalRate	= rgiEnhcTSRate; // temporal subsampling rate for enhancement layer
	args.rgbDumpMB				= rgbDumpMB;
	args.rgbTrace				= rgbTrace;
	args.rgbRoundingControlDisable	= rgbRoundingControlDisable;
	args.rgiInitialRoundingType	= rgiInitialRoundingType;
	args.rguiSpriteUsage		= rguiSpriteUsage; // sprite usage
	args.rguiWarpingAccuracy	= rguiWarpingAccuracy; // warping accuracy
	args.rgNumOfPnts			= rgiNumPnts; // number of points for sprite, 0 for stationary and -1 for no sprite
	args.pchSptDir				= pchSptDir; // sprite directory
	args.pchSptPntDir			= pchSptPntDir; // sprite point file
    args.pSpriteMode			= rgSpriteMode;	// sprite reconstruction mode
	args.iSpatialOption			= iSpatialOption;
	args.uiFrameWidth_SS		= uiFrmWidth_SS;
	args.uiFrameHeight_SS		= uiFrmHeight_SS;
	args.uiHor_sampling_n		= uiHor_sampling_n;
	args.uiHor_sampling_m		= uiHor_sampling_m;
	args.uiVer_sampling_n		= uiVer_sampling_n;
	args.uiVer_sampling_m		= uiVer_sampling_m;
	args.uiUse_ref_shape		= uiUseRefShape;	
	args.uiUse_ref_texture		= uiUseRefTexture;	
	args.uiHor_sampling_n_shape	= uiHor_sampling_n_shape;	
	args.uiHor_sampling_m_shape	= uiHor_sampling_m_shape;	
	args.uiVer_sampling_n_shape	= uiVer_sampling_n_shape;	
	args.uiVer_sampling_m_shape	= uiVer_sampling_m_shape;	

	CSessionEncoder* penc = new CSessionEncoder (&args);

#ifdef __PC_COMPILER_
	Int tickBegin = ::GetTickCount ();
#endif // __PC_COMPILER_

	penc->encode();

#ifdef __PC_COMPILER_
	Int tickAfter = ::GetTickCount ();
	Int nFrames = 1 + (lastFrm - firstFrm) / rgiTSRate [0];
	printf ("Total time: %d\n", tickAfter - tickBegin);
	Double dAverage = (Double) (tickAfter - tickBegin) / (Double) nFrames;
	printf ("Total frame %d\tAverage time: %.6lf\n", nFrames, dAverage);
	printf ("FPS %.6lf\n", 1000.0 / dAverage);
#endif // __PC_COMPILER_
	delete penc;

	for (Int iLayer = BASE_LAYER; iLayer <= ENHN_LAYER; iLayer++)	{
		delete [] rguiRateControl [iLayer];
		delete [] rguiBitsBudget [iLayer];
		delete [] rgbErrorResilientDisable [iLayer];
		delete [] rgbDataPartitioning [iLayer];
		delete [] rgbReversibleVlc [iLayer];
		delete [] rgiVPBitTh [iLayer];

		// for texture coding
		delete [] rgfQuant [iLayer];
		delete [] rgiIntraDCSwitchingThr [iLayer];
		delete [] rgiIStep [iLayer];				// error signal quantization stepsize
		delete [] rgiPStep [iLayer];				// error signal quantization stepsize
		delete [] rgiIStepAlpha [iLayer];		// error signal quantization stepsize
		delete [] rgiPStepAlpha [iLayer];		// error signal quantization stepsize
		delete [] rgiBStepAlpha [iLayer];
		delete [] rgbNoGrayQuantUpdate [iLayer]; //discouple change of gray quant with tex. quant
		delete [] rgiStepBCode [iLayer];			// error signal quantization stepsize
		delete [] rgbLoadIntraMatrix [iLayer];
		delete [] rgbLoadInterMatrix [iLayer];
		delete [] rgbLoadIntraMatrixAlpha [iLayer];
		delete [] rgbLoadInterMatrixAlpha [iLayer];
		for (iObj = 0; iObj < nVO; iObj++) {
			delete [] rgppiIntraQuantizerMatrix  [iLayer] [iObj];
			delete [] rgppiInterQuantizerMatrix  [iLayer] [iObj];
			delete [] rgppiIntraQuantizerMatrixAlpha  [iLayer] [iObj];
			delete [] rgppiInterQuantizerMatrixAlpha  [iLayer] [iObj];
			if(pchMVFileName [iLayer] [iObj]!=NULL)
				delete [] pchMVFileName [iLayer] [iObj];

		}
		delete [] rgppiIntraQuantizerMatrix [iLayer];
		delete [] rgppiInterQuantizerMatrix [iLayer];
		delete [] rgppiIntraQuantizerMatrixAlpha [iLayer];
		delete [] rgppiInterQuantizerMatrixAlpha [iLayer];
		delete [] pchMVFileName [iLayer];

		// for motion esti.
		delete [] rgbOriginalME [iLayer];
		delete [] rgbAdvPredDisable [iLayer];
		delete [] rguiSearchRange [iLayer];

		// for interlace coding
		delete [] rgbInterlacedCoding [iLayer];
		delete [] rgbTopFieldFirst [iLayer];
		delete [] rgiDirectModeRadius [iLayer];
		delete [] rgiMVFileUsage [iLayer];
		
		delete [] rgdFrameFrequency [iLayer];
		delete [] rgbAlternateScan [iLayer];

		delete [] rgbComplexityEstimationDisable [iLayer];
		delete [] rguiEstimationMethod [iLayer];
		delete [] rgbOpaque [iLayer];
		delete [] rgbTransparent [iLayer];
		delete [] rgbIntraCAE [iLayer];
		delete [] rgbInterCAE [iLayer];
		delete [] rgbNoUpdate [iLayer];
		delete [] rgbUpsampling [iLayer];
		delete [] rgbIntraBlocks [iLayer];
		delete [] rgbInterBlocks [iLayer];
		delete [] rgbInter4vBlocks [iLayer];
		delete [] rgbNotCodedBlocks [iLayer];
		delete [] rgbDCTCoefs [iLayer];
		delete [] rgbDCTLines [iLayer];
		delete [] rgbVLCSymbols [iLayer];
		delete [] rgbVLCBits [iLayer];
		delete [] rgbAPM [iLayer];
		delete [] rgbNPM [iLayer];
		delete [] rgbInterpolateMCQ [iLayer];
		delete [] rgbForwBackMCQ [iLayer];
		delete [] rgbHalfpel2 [iLayer];
		delete [] rgbHalfpel4 [iLayer];
		delete [] rgbSadct [iLayer];
		delete [] rgbQuarterpel [iLayer];

		delete [] rguiVolControlParameters [iLayer];
		delete [] rguiChromaFormat [iLayer];
		delete [] rguiLowDelay [iLayer];
		delete [] rguiVBVParams [iLayer];
		delete [] rguiBitRate [iLayer];
		delete [] rguiVbvBufferSize [iLayer];
		delete [] rguiVbvBufferOccupany [iLayer];



		delete [] rgbNewpredEnable[iLayer];
		delete [] rgbNewpredSegType[iLayer];

		delete [] rgbSadctDisable[iLayer];
		delete [] rgbQuarterSample[iLayer];

		for (iObj = 0; iObj < nVO; iObj++)
		{
			if(pchNewpredRefName [iLayer][iObj]!=NULL)
				delete pchNewpredRefName [iLayer][iObj];
			if(pchNewpredSlicePoint [iLayer][iObj]!=NULL)
				delete pchNewpredSlicePoint [iLayer][iObj];
		}

		delete [] pchNewpredRefName[iLayer];
		delete [] pchNewpredSlicePoint[iLayer];

		delete [] RRVmode[iLayer];
	}

	delete [] rgbScalability;
    delete [] rgbSpatialScalability;
	delete [] rgiTemporalScalabilityType;
	delete [] rgiEnhancementType;
//OBSSFIX_MODE3
	delete [] rgiEnhancementTypeSpatial;
//~OBSSFIX_MODE3
	// for mask coding, should fill in later on
	delete [] rgfAlphaUsage; // alpha usage for each VO.  0: binary, 1: 8-bit
	delete [] rgiAlphaShapeExtension;
	delete [] rgbShapeOnly;
	delete [] rgiBinaryAlphaTH;
	delete [] rgiBinaryAlphaRR;	//	Added for error resilient mode by Toshiba(1997-11-14)
	delete [] rgiGrayToBinaryTH;
	delete [] rgbNoCrChange;

	delete [] rgbDeblockFilterDisable;	
	delete [] rgfChrType;
	delete [] rgiNumBbetweenPVOP;			// no of B-VOPs between P-VOPs
	delete [] rgiNumPbetweenIVOP;			// no of P-VOPs between I-VOPs
//added to encode GOV header by SONY 980212
	delete [] rgiGOVperiod;
//980212
	delete [] rgiTSRate;
	delete [] rgiEnhcTSRate; // added by Norio Ito
	delete [] rgbAllowSkippedPMBs;

	// rounding control
	delete [] rgbRoundingControlDisable;
	delete [] rgiInitialRoundingType;

	// sprite
	delete [] rguiSpriteUsage;
	delete [] rguiWarpingAccuracy;
	delete [] rgiNumPnts;
	delete [] rgbDumpMB;
	delete [] rgbTrace;
	delete [] rgSpriteMode;

	delete pchPrefix;
	delete pchBmpDir;
	delete pchOutBmpDir;
	delete pchOutStrFile;
	delete pchSptDir;
	delete pchSptPntDir;

	// version 2
	delete [] rguiVerID;


	return 0;
}


Void nextValidLine (FILE *pfPara, UInt* pnLine)	{

	U8 chBoL = 0;
	Char pchPlaceHolder[200];
	fgets (pchPlaceHolder, 200, pfPara); // get the next line 
	(*pnLine)++;
	while (feof (pfPara) == 0 && (chBoL = fgetc(pfPara)) == '%') {
		// skip a line
		fgets (pchPlaceHolder, 200, pfPara); // get the next line 
		(*pnLine)++;
	}
	ungetc (chBoL, pfPara);				

}

///// WAVELET VTC: begin ///////////////////////////////

Void readVTCParam(CVTCEncoder *pvtcenc, FILE *pfVTCPara, UInt* pnLine, UInt *uiFrmWidth, UInt *uiFrmHeight)
{ // FPDAM
 
 
 // begin: added by Sharp (99/11/18)
 	UInt uiVerID;
 	nextValidLine (pfVTCPara, pnLine);
 	if ( fscanf (pfVTCPara, "%d", &uiVerID) != 1)	{
 		fprintf(stderr, "wrong parameter on line %d\n", *pnLine);
 		assert (FALSE);
 	}
 	if ( uiVerID != 1 && uiVerID != 2 ){
 		fprintf(stderr,"Unsupported .cfg file. Version ID is required at the beginning of .cfg\n");
 		assert ( uiVerID == 1 || uiVerID == 2 );
 	}
 // end: added by Sharp (99/11/18)
 
	// source image path
	Char *cImagePath;
	cImagePath = new char[80];
	nextValidLine (pfVTCPara, pnLine);
	if ( fscanf (pfVTCPara, "%s", cImagePath) != 1)	{
		fprintf(stderr, "wrong parameter on line %d\n", *pnLine);
		assert (FALSE);
	}
	
	// Alpha Channel 
	UInt uiAlphaChannel;
	nextValidLine (pfVTCPara, pnLine);
	if ( fscanf (pfVTCPara, "%d", &uiAlphaChannel) != 1)	{
		fprintf(stderr, "wrong parameter on line %d\n", *pnLine);
		assert (FALSE);
	}
	
	// segmentation map image path
	Char *cSegImagePath;
	cSegImagePath = new char[80];
	nextValidLine (pfVTCPara, pnLine);
	if ( fscanf (pfVTCPara, "%s", cSegImagePath) != 1)	{
		fprintf(stderr, "wrong parameter on line %d\n", *pnLine);
		assert (FALSE);
	}
	
	// FPDAM begin: deleted by Sharp 
	//	//begin: added by SL@Sarnoff (03/02/99)
	//	UInt uiShapeScalable; //scalable shape coding ?
	//	nextValidLine (pfVTCPara, pnLine);
	//	if ( fscanf (pfVTCPara, "%d", &uiShapeScalable) != 1)	{
	//		fprintf(stderr, "wrong parameter on line %d\n", *pnLine);
	//		assert (FALSE);
	//	}
	// FPDAM end: deleted by Sharp 
	
	UInt uiSTOConstAlpha; // constant alpha value?
	nextValidLine (pfVTCPara, pnLine);
	if ( fscanf (pfVTCPara, "%d", &uiSTOConstAlpha) != 1)	{
		fprintf(stderr, "wrong parameter on line %d\n", *pnLine);
		assert (FALSE);
	}
	
	UInt uiSTOConstAlphaValue; //what is the value if const alpha?
	nextValidLine (pfVTCPara, pnLine);
	if ( fscanf (pfVTCPara, "%d", &uiSTOConstAlphaValue) != 1)	{
		fprintf(stderr, "wrong parameter on line %d\n", *pnLine);
		assert (FALSE);
	}
	//end: added by SL@Sarnoff (03/02/99)
	
	// Alpha threshold
	UInt uiAlphaTh;
	nextValidLine (pfVTCPara, pnLine);
	if ( fscanf (pfVTCPara, "%d", &uiAlphaTh) != 1)	{
		fprintf(stderr, "wrong parameter on line %d\n", *pnLine);
		assert (FALSE);
	}
	
	// Alpha Change CR Disable 
	UInt uiChangeCRDisable;
	nextValidLine (pfVTCPara, pnLine);
	if ( fscanf (pfVTCPara, "%d", &uiChangeCRDisable) != 1)	{
		fprintf(stderr, "wrong parameter on line %d\n", *pnLine);
		assert (FALSE);
	}
	
	// output bitstream file 
	Char *cOutBitsFile;
	cOutBitsFile = new char[80];
	nextValidLine (pfVTCPara, pnLine);
	if ( fscanf (pfVTCPara, "%s", cOutBitsFile) != 1)	{
		fprintf(stderr, "wrong parameter on line %d\n", *pnLine);
		assert (FALSE);
	}
	
	// color components
	UInt uiColors;
	nextValidLine (pfVTCPara, pnLine);
	if ( fscanf (pfVTCPara, "%d", &uiColors) != 1)	{
		fprintf(stderr, "wrong parameter on line %d\n", *pnLine);
		assert (FALSE);
	}
	
	// Frame width
	//	UInt uiFrmWidth; //FPDAM
	nextValidLine (pfVTCPara, pnLine);
	if ( fscanf (pfVTCPara, "%d", uiFrmWidth) != 1)	{
		fprintf(stderr, "wrong parameter on line %d\n", *pnLine);
		assert (FALSE);
	}
	
	// Frame height
	//	UInt uiFrmHeight;			// frame height // FPDAM
	nextValidLine (pfVTCPara, pnLine);
	if ( fscanf (pfVTCPara, "%d", uiFrmHeight) != 1)	{
		fprintf(stderr, "wrong parameter on line %d\n", *pnLine);
		assert (FALSE);
	}
	
	// begin: added by Sharp (99/2/16)
	UInt uiTilingDisable; // modified by Sharp (99/3/29)
	nextValidLine (pfVTCPara, pnLine);
	if ( fscanf (pfVTCPara, "%d", &uiTilingDisable) != 1)  { // modified by Sharp (99/3/29)
		fprintf(stderr, "wrong parameter on line %d\n", *pnLine);
		assert (FALSE);
	}
	
	UInt uiTilingJump;
	nextValidLine (pfVTCPara, pnLine);
	if ( fscanf (pfVTCPara, "%d", &uiTilingJump) != 1)  {
		fprintf(stderr, "wrong parameter on line %d\n", *pnLine);
		assert (FALSE);
	}
	
	UInt uiTileWidth;
	nextValidLine (pfVTCPara, pnLine);
	if ( fscanf (pfVTCPara, "%d", &uiTileWidth) != 1) {
		fprintf(stderr, "wrong parameter on line %d\n", *pnLine);
		assert (FALSE);
	}
	
	UInt uiTileHeight;
	nextValidLine (pfVTCPara, pnLine);
	if ( fscanf (pfVTCPara, "%d", &uiTileHeight) != 1)  {
		fprintf(stderr, "wrong parameter on line %d\n", *pnLine);
		assert (FALSE);
	}
	// end: added by Sharp (99/2/16)

	// wavelet decomposition
	UInt uiWvtDecmpLev;
	nextValidLine (pfVTCPara, pnLine);
	if ( fscanf (pfVTCPara, "%d", &uiWvtDecmpLev) != 1)	{
		fprintf(stderr, "wrong parameter on line %d\n", *pnLine);
		assert (FALSE);
	}

	// wavelet type
	UInt uiWvtType;
	nextValidLine (pfVTCPara, pnLine);
	if ( fscanf (pfVTCPara, "%d", &uiWvtType) != 1)	{
		fprintf(stderr, "wrong parameter on line %d\n", *pnLine);
		assert (FALSE);
	}
	
	// hjlee 0901
	// wavelet download ?
	UInt uiWvtDownload;
	nextValidLine (pfVTCPara, pnLine);
	if ( fscanf (pfVTCPara, "%d", &uiWvtDownload) != 1)	{
		fprintf(stderr, "wrong parameter on line %d\n", *pnLine);
		assert (FALSE);
	}
	
	// hjlee 0901
	// wavelet uniform ?
	UInt uiWvtUniform;
	Int  *iWvtFilters;
	nextValidLine (pfVTCPara, pnLine);
	if ( fscanf (pfVTCPara, "%d", &uiWvtUniform) != 1)	{
		fprintf(stderr, "wrong parameter on line %d\n", *pnLine);
		assert (FALSE);
	}
	
	
	/* default wavelet filters are uniform */
    if (!uiWvtDownload) 
		uiWvtUniform=1;
    if (uiWvtUniform)
		iWvtFilters = (Int *)malloc(sizeof(Int));
    else
		iWvtFilters = (Int *)malloc(sizeof(Int)*uiWvtDecmpLev);
	
    if (iWvtFilters==NULL) 
		exit(printf("error allocating memory:  iWvtFilters\n"));
	/* read in filter numbers if applicable */
	/* for int type filter: 0, 2, 4 */
	/* for float type: 1, 3, 5-10 */
	
    if (!uiWvtDownload) {
		if (uiWvtType==0)
			iWvtFilters[0]=0;
		else
			iWvtFilters[0]=1;
		nextValidLine(pfVTCPara, pnLine); //added by SL 030399 to remove nextline 
	}
    else if (uiWvtUniform) {
		nextValidLine (pfVTCPara, pnLine);
		fscanf(pfVTCPara,"%d", iWvtFilters);
		/* check to see if the filters match its type */
		if (iWvtFilters[0] !=0 &&
			iWvtFilters[0] !=2 &&
			iWvtFilters[0] !=4) {
			if (uiWvtType==0)
				exit(printf("Filter %d is not integer filter.\n", iWvtFilters[0]));
		}
		else {
			if (uiWvtType!=0)
				exit(printf("Filter %d is not float filter.\n", iWvtFilters[0]));
		}
	}
    else {
		nextValidLine (pfVTCPara, pnLine);
		Int spa_lev;
		for (spa_lev=0; spa_lev<(Int)uiWvtDecmpLev; spa_lev++) {
			fscanf(pfVTCPara,"%d", iWvtFilters+spa_lev);
			/* check to see if the filters match its type */
			if (iWvtFilters[spa_lev] !=0  &&
				iWvtFilters[spa_lev] !=2 &&
				iWvtFilters[spa_lev] !=4) {
				if (uiWvtType==0)
					exit(printf("Filter %d is not integer filter.\n", iWvtFilters[spa_lev]));
			}
			else {
				if (uiWvtType!=0)
					exit(printf("Filter %d is not float filter.\n", iWvtFilters[spa_lev]));
			}
		}
	}
	
	// quantization type 
	UInt uiQuantType;
	nextValidLine (pfVTCPara, pnLine);
	if ( fscanf (pfVTCPara, "%d", &uiQuantType) != 1)	{
		fprintf(stderr, "wrong parameter on line %d\n", *pnLine);
		assert (FALSE);
	}
	
	// scan direction
	UInt uiScanDirection;
	nextValidLine (pfVTCPara, pnLine);
	if ( fscanf (pfVTCPara, "%d", &uiScanDirection) != 1)	{
		fprintf(stderr, "wrong parameter on line %d\n", *pnLine);
		assert (FALSE);
	}
	
	// Start Code Enable 
	UInt bStartCodeEnable;
	nextValidLine (pfVTCPara, pnLine);
	if ( fscanf (pfVTCPara, "%d", &bStartCodeEnable) != 1)	{
		fprintf(stderr, "wrong parameter on line %d\n", *pnLine);
		assert (FALSE);
	}
	
	// target spatial level 
	UInt uiTargetSpatialLev;
	nextValidLine (pfVTCPara, pnLine);
	if ( fscanf (pfVTCPara, "%d", &uiTargetSpatialLev) != 1)	{
		fprintf(stderr, "wrong parameter on line %d\n", *pnLine);
		assert (FALSE);
	}
	
	// target SNR level
	UInt uiTargetSNRLev;
	nextValidLine (pfVTCPara, pnLine);
	if ( fscanf (pfVTCPara, "%d", &uiTargetSNRLev) != 1)	{
		fprintf(stderr, "wrong parameter on line %d\n", *pnLine);
		assert (FALSE);
	}
	
	//begin: added by SL@Sarnoff (03/02/99)
	// target Shape level
	UInt uiTargetShapeLev;
	nextValidLine (pfVTCPara, pnLine);
	if ( fscanf (pfVTCPara, "%d", &uiTargetShapeLev) != 1)	{
		fprintf(stderr, "wrong parameter on line %d\n", *pnLine);
		assert (FALSE);
	}
	
	// interpolated to fullsize as output?
	UInt uiFullSizeOut;
	nextValidLine (pfVTCPara, pnLine);
	if ( fscanf (pfVTCPara, "%d", &uiFullSizeOut) != 1)	{
		fprintf(stderr, "wrong parameter on line %d\n", *pnLine);
		assert (FALSE);
	}
	//end: added by SL@Sarnoff (03/02/99)
	
	// begin: added by Sharp (99/2/16)
	UInt uiTargetTileFrom, uiTargetTileTo;
	nextValidLine (pfVTCPara, pnLine);
	if ( fscanf (pfVTCPara, "%d", &uiTargetTileFrom) != 1)  {
		fprintf(stderr, "wrong parameter on line %d\n", *pnLine);
		assert (FALSE);
	}
	nextValidLine (pfVTCPara, pnLine);
	if ( fscanf (pfVTCPara, "%d", &uiTargetTileTo) != 1)  {
		fprintf(stderr, "wrong parameter on line %d\n", *pnLine);
		assert (FALSE);
	}
	// end: added by Sharp (99/2/16)
	
	//Added by Sarnoff for error resilience, 3/5/99
	//error resilience related param, bbc, 2/19/99
	Int uiErrResiDisable;
	Int uiPacketThresh;  
	Int uiSegmentThresh;
	
	//error resilience disable
	nextValidLine (pfVTCPara, pnLine);
	fscanf(pfVTCPara, "%d",&uiErrResiDisable);
	if ( uiErrResiDisable!= 0 && uiErrResiDisable != 1){
		fprintf(stderr, "wrong parameter on line %d\n", *pnLine);
		assert (FALSE);
	}
	
	//target packet size
	nextValidLine (pfVTCPara, pnLine);
	fscanf(pfVTCPara, "%d",&uiPacketThresh);
	if ( uiPacketThresh < 0){
		fprintf(stderr, "wrong parameter on line %d\n", *pnLine);
		assert (FALSE);
	}
	
	//target segment size
	nextValidLine (pfVTCPara, pnLine);
	fscanf(pfVTCPara, "%d",&uiSegmentThresh);
	if ( uiSegmentThresh < 0){
		fprintf(stderr, "wrong parameter on line %d\n", *pnLine);
		assert (FALSE);
	}
	//End Added by Sarnoff for error resilience, 3/5/99
	
	// DC-Y quantization
	UInt uiQdcY;
	nextValidLine (pfVTCPara, pnLine);
	if ( fscanf (pfVTCPara, "%d", &uiQdcY) != 1)	{
		fprintf(stderr, "wrong parameter on line %d\n", *pnLine);
		assert (FALSE);
	}
	
	// DC-UV quantization
	UInt uiQdcUV;
	nextValidLine (pfVTCPara, pnLine);
	if ( fscanf (pfVTCPara, "%d", &uiQdcUV) != 1)	{
		fprintf(stderr, "wrong parameter on line %d\n", *pnLine);
		assert (FALSE);
	}
	
	// number of spatial scalabilities
	UInt uiSpatialLev;
	nextValidLine (pfVTCPara, pnLine);
	if ( fscanf (pfVTCPara, "%d", &uiSpatialLev) != 1)	{
		fprintf(stderr, "wrong parameter on line %d\n", *pnLine);
		assert (FALSE);
	}
	
	// hjlee 0901
    /* spatial layer flexability  */
	UInt defaultSpatialScale;
	Int  lastWvtDecompInSpaLayer[MAXDECOMPLEV];
	
    if (uiQuantType == 2)
	{
		if (uiSpatialLev != uiWvtDecmpLev)
		{
			/* read in usedefault */
			fscanf(pfVTCPara,"%d",&defaultSpatialScale);
			if (defaultSpatialScale==0)
			{
				/* read in spatial layer info */
				Int spa_lev;
				for (spa_lev=0; spa_lev<(Int)uiSpatialLev-1; ++spa_lev)   
					fscanf(pfVTCPara,"%d",&lastWvtDecompInSpaLayer[spa_lev]);
			}
		}
	}
	
	
	UInt uispa_lev;
	UInt uisnr_lev;
	SNR_PARAM *Qinfo[3];
	UInt uiSNRLev;
	UInt uiQuant;
	Int i;
	// hjlee 0901
	
	/* SQ-BB scanning */
	if (uiQuantType==1 && uiScanDirection==1) {
		
		nextValidLine (pfVTCPara, pnLine);
		if ( fscanf (pfVTCPara, "%d", &uiSNRLev) != 1)	{
			fprintf(stderr, "wrong parameter on line %d\n", *pnLine);
			assert (FALSE);
		}
		
		uiSpatialLev = uiWvtDecmpLev;
		for (i=0; i<3; i++)
			Qinfo[i] = new SNR_PARAM[uiSpatialLev];
		for (uispa_lev=0; uispa_lev<uiSpatialLev; uispa_lev++) {
			Int col;
			for (col=0; col<3; col++) {
				Qinfo[col][uispa_lev].SNR_scalability_levels = (Int)1;
				Qinfo[col][uispa_lev].Quant = new Int[uiSNRLev];
			}
		}
		// read quant-Y
		if ( fscanf (pfVTCPara, "%d", &uiQuant) != 1)	{
			fprintf(stderr, "wrong parameter on line %d\n", *pnLine);
			assert (FALSE);
		}
		Qinfo[0][0].Quant[0] = uiQuant;
		
		// read quant-UV
		if (uiColors==3) {
			if ( fscanf (pfVTCPara, "%d", &uiQuant) != 1)	{
				fprintf(stderr, "wrong parameter on line %d\n", *pnLine);
				assert (FALSE);
			}
			Qinfo[1][0].Quant[0] = uiQuant;
			Qinfo[2][0].Quant[0] = uiQuant;
		}
		
	}
	else {
		
		for (i=0; i<3; i++)
			Qinfo[i] = new SNR_PARAM[uiSpatialLev];
		for (uispa_lev=0; uispa_lev<uiSpatialLev; uispa_lev++) {
			// number of spatial scalabilities
			nextValidLine (pfVTCPara, pnLine);
			if ( fscanf (pfVTCPara, "%d", &uiSNRLev) != 1)	{
				fprintf(stderr, "wrong parameter on line %d\n", *pnLine);
				assert (FALSE);
			}
			
			Int col;
			for (col=0; col<3; col++) {
				Qinfo[col][uispa_lev].SNR_scalability_levels = (Int)uiSNRLev;
				Qinfo[col][uispa_lev].Quant = new Int[uiSNRLev];
			}
			
			for (uisnr_lev=0; uisnr_lev<uiSNRLev; uisnr_lev++) {
				
				// read quant-Y
				if ( fscanf (pfVTCPara, "%d", &uiQuant) != 1)	{
					fprintf(stderr, "wrong parameter on line %d\n", *pnLine);
					assert (FALSE);
				}
				Qinfo[0][uispa_lev].Quant[uisnr_lev] = uiQuant;
				
				// read quant-UV
				if (uiColors==3) {
					if ( fscanf (pfVTCPara, "%d", &uiQuant) != 1)	{
						fprintf(stderr, "wrong parameter on line %d\n", *pnLine);
						assert (FALSE);
					}
					Qinfo[1][uispa_lev].Quant[uisnr_lev] = uiQuant;
					Qinfo[2][uispa_lev].Quant[uisnr_lev] = uiQuant;
				}
			}
		}
	}
	pvtcenc -> init(
		uiVerID, // added by Sharp (99/11/18)
		cImagePath,
		uiAlphaChannel,
		cSegImagePath,
		uiAlphaTh,
		uiChangeCRDisable,
		// FPDAM begin: deleted by Sharp 
		//		 uiShapeScalable, //added by SL@Sarnoff (03/02/99)
		// FPDAM end: deleted by Sharp 
		uiSTOConstAlpha,//added by SL@Sarnoff (03/02/99)
		uiSTOConstAlphaValue, //added by SL@Sarnoff (03/02/99)
		cOutBitsFile,
		uiColors,
		*uiFrmWidth,
		*uiFrmHeight,
		// begin: added by Sharp (99/2/16)
		uiTilingDisable, // modified by Sharp (99/3/29)
		uiTilingJump,
		uiTileWidth,
		uiTileHeight,
		// end: added by Sharp (99/2/16)
		uiWvtType,
		uiWvtDownload, // hjlee 0901
		uiWvtDecmpLev,
		uiWvtUniform, // hjlee 0901
		iWvtFilters,  // hjlee 0901
		uiQuantType,
		uiScanDirection,
		bStartCodeEnable,
		uiTargetSpatialLev,
		uiTargetSNRLev,
		uiTargetShapeLev,//added by SL@Sarnoff (03/02/99)
		uiFullSizeOut, //added by SL@Sarnoff (03/02/99)
		// begin: added by Sharp (99/2/16)
		uiTargetTileFrom, uiTargetTileTo,
		// end: added by Sharp (99/2/16)
		uiQdcY,
		uiQdcUV, 
		uiSpatialLev,
		defaultSpatialScale, // hjlee 0901
		lastWvtDecompInSpaLayer, // hjlee 0901
		Qinfo,
		//Added by Sarnoff for error resilience, 3/5/99
		uiErrResiDisable,  //bbc, 2/19/99
		uiPacketThresh,  
		uiSegmentThresh		 
		//End Added by Sarnoff for error resilience, 3/5/99
		);
}

///// WAVELET VTC: end ///////////////////////////////

Void RunVTCCodec(char *VTCCtlFile)
{
	UInt nLine = 1;
	UInt* pnLine = &nLine;
	FILE *pfVTCPara;
	if ((pfVTCPara = fopen (VTCCtlFile, "r")) == NULL ){
		printf (" vtc Parameter File Not Found\n");
		exit (1);
	}

	CVTCEncoder vtcenc;

	// read VTC control parameters
	UInt uiFrmWidth, uiFrmHeight;
	readVTCParam(&vtcenc, pfVTCPara, pnLine, &uiFrmWidth, &uiFrmHeight);

	fclose(pfVTCPara);

	// start VTC encoding
	Int targetSpatialLev = vtcenc.mzte_codec.m_iTargetSpatialLev;
	Int targetSNRLev = vtcenc.mzte_codec.m_iTargetSNRLev;
 	Int targetShapeLev = vtcenc.mzte_codec.m_iTargetShapeLev;
 	Int fullSizeOut = vtcenc.mzte_codec.m_iFullSizeOut;

	vtcenc.encode();

	// start VTC decoding 
	CVTCDecoder vtcdec;
  	vtcdec.decode(vtcenc.m_cOutBitsFile, "encRec.yuv", 
 						uiFrmWidth, uiFrmHeight, // FPDAM
 						targetSpatialLev, targetSNRLev,
 						targetShapeLev, fullSizeOut, //added by SL@Sarnoff (03/02/99)
 						vtcenc.mzte_codec.m_target_tile_id_from, vtcenc.mzte_codec.m_target_tile_id_to); // modified by Sharp (99/2/16)


}

Void GetIVal(CxParamSet *pPar, char *pchName, Int iInd, Int *piRtn)
{
	Double dVal;
	Int er = pPar->GetDouble(pchName, iInd, &dVal);
	if(er!=ERES_OK)
	{
		if(iInd<0)
			printf("Error: can't find %s in parameter file.\n", pchName);
		else
			printf("Error: can't find %s[%d] in parameter file.\n", pchName, iInd);
		exit(1);
	}
	*piRtn = (Int)dVal;
}

Void GetDVal(CxParamSet *pPar, char *pchName, Int iInd, Double *pdRtn)
{
	Double dVal;
	Int er = pPar->GetDouble(pchName, iInd, &dVal);
	if(er!=ERES_OK)
	{
		if(iInd<0)
			printf("Error: can't find %s in parameter file.\n", pchName);
		else
			printf("Error: can't find %s[%d] in parameter file.\n", pchName, iInd);
		exit(1);
	}
	*pdRtn = dVal;
}

Void GetSVal(CxParamSet *pPar, char *pchName, Int iInd, char **ppchRtn)
{
	char *pchVal;
	Int er = pPar->GetString(pchName, iInd, &pchVal);
	if(er!=ERES_OK)
	{
		if(iInd<0)
			printf("Error: can't find %s in parameter file.\n", pchName);
		else
			printf("Error: can't find %s[%d] in parameter file.\n", pchName, iInd);
		exit(1);
	}
	*ppchRtn = pchVal;
}

Void GetAVal(CxParamSet *pPar, char *pchName, Int iInd, Double **ppdRtn, Int *piCount)
{
	Int er = pPar->GetArray(pchName, iInd, ppdRtn, piCount);
	if(er!=ERES_OK)
	{
		if(iInd<0)
			printf("Error: can't find %s in parameter file.\n", pchName);
		else
			printf("Error: can't find %s[%d] in parameter file.\n", pchName, iInd);
		exit(1);
	}
}

Void readBoolParam(CxParamSet *pPar, char *pchName, Int iInd, Bool *pbVal)
{
	Int iVal;
	GetIVal(pPar, pchName, iInd, &iVal);
	if(iVal!=0 && iVal!=1)
	{
		char rgchErr[100];
		sprintf(rgchErr, "%s must be 0 or 1", pchName);
		fatal_error(rgchErr);
	}

	*pbVal = (Bool)iVal;
}

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

	May 9, 1999:	tm5 rate control by DemoGraFX, duhoff@mediaone.net

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
#include "fstream.h"
#include "sesenc.hpp"
// #include "encoder/tps_sesenc.hpp" // deleted by Sharp (98/2/12)

///// WAVELET VTC: begin ////////////////////////////////
#include "dataStruct.hpp"     // hjlee 
///// WAVELET VTC: end ////////////////////////////////


#ifndef __GLOBAL_VAR_
#define __GLOBAL_VAR_
#endif
#include "global.hpp"

#ifdef __MFC_
#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

#define new DEBUG_NEW				   
#endif // __MFC_

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

Void nextValidLine (FILE *pfPara, UInt* pnLine);
///// WAVELET VTC: begin ////////////////////////////////
Void readVTCParam(CVTCEncoder *pvtcenc, FILE *pfPara, UInt* pnLine);
///// WAVELET VTC: end ////////////////////////////////
Void readBoolVOLFlag (Bool * rgbTable [2], UInt nVO, FILE * pfCfg, UInt * pnLine, Bool bAnyScalability);
Void readItem(UInt *rguiTable [2], UInt nVO, FILE * pfCfg, UInt * pnLine, Bool bAnyScalability);
Void RunVTCCodec(char *VTCCtlFile);
Void GetIVal(CxParamSet *pPar, char *pchName, Int iInd, Int *piRtn);
Void GetDVal(CxParamSet *pPar, char *pchName, Int iInd, Double *pdRtn);
Void GetSVal(CxParamSet *pPar, char *pchName, Int iInd, char **ppchRtn);
Void GetAVal(CxParamSet *pPar, char *pchName, Int iInd, Double **ppdRtn, Int *piCount);
Void readBoolParam(CxParamSet *pPar, char *pchName, Int iInd, Bool *pbVal);

Int bPrint = 0;

#define VERSION_STRING "FDIS 1.02 (990812)"
// please update version number when you
// make changes in the following

#define BASE_LAYER 0
#define ENHN_LAYER 1
#define NO_SCALABILITY		 0
#define TEMPORAL_SCALABILITY 1
#define SPATIAL_SCALABILITY  2

int main (int argc, Char* argv[])
{
	UInt nLine = 1;
	UInt* pnLine = &nLine;
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


	// all the parameters to the encoder
	Int iVersion;
	Int iVTCFlag;
	UInt uiFrmWidth, uiFrmHeight;
	UInt firstFrm, lastFrm;
	Bool bNot8Bit;
	UInt uiQuantPrecision;
	UInt nBits;
	UInt firstVO, lastVO;
	UInt nVO = 0;
	UInt uiFrmWidth_SS,uiFrmHeight_SS;
	UInt uiHor_sampling_m,uiHor_sampling_n;
	UInt uiVer_sampling_m,uiVer_sampling_n;
	Bool bAnyScalability = FALSE;
	Int iSpatialOption;
	char *pchPrefix;		
	char *pchBmpDir;
	char *pchOutBmpDir;
	char *pchOutStrFile;
	char *pchSptDir;
	char *pchSptPntDir;
	
	Int *rgiTemporalScalabilityType;
	Bool *rgbSpatialScalability;
	Bool *rgbScalability;
	Int *rgiEnhancementType;
	AlphaUsage *rgfAlphaUsage;
	Bool *rgbShapeOnly;
	Int *rgiBinaryAlphaTH;
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

	UInt iObj;

	Int iCh = getc(pfPara);
	if(iCh=='!')
	{
		// NEW STYLE PARAMETER FILE
		printf("New style parameter file.\n");
		CxParamSet par;

		char rgBuf[80];
		char *pch = fgets(rgBuf, 79, pfPara);
		if(pch==NULL)
			fatal_error("Can't read magic number in parameter file");
		//if(strcmp("!!MS!!!\n", rgBuf)!=0)
		//	fatal_error("Bad magic number at start of parameter file");

		int iLine;
		int er = par.Load(pfPara, &iLine);
		if(er!=ERES_OK)
			exit(printf("error %d at line %d of parameter file\n",er, iLine));
		
		fclose(pfPara);

		// process the parameters
		GetIVal(&par, "Version", -1, &iVersion);
		if(iVersion!=901)
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
		GetIVal(&par, "Source.ObjectIndex.Last", -1, (Int *)&lastVO);

		fatal_error("First and last object indices don't make sense", lastVO>=firstVO);
		nVO = lastVO - firstVO + 1;

		// allocate per-vo parameters
		rgiTemporalScalabilityType = new Int [nVO];
		rgbSpatialScalability = new Bool [nVO];
		rgbScalability = new Bool [nVO];
		rgiEnhancementType = new Int [nVO];
		rgfAlphaUsage = new AlphaUsage [nVO];
		rgbShapeOnly = new Bool [nVO];
		rgiBinaryAlphaTH = new Int [nVO];
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
			if(strcmp(pchTmp, "None")==0)
				rgfAlphaUsage[iObj] = RECTANGLE;
			else if(strcmp(pchTmp, "Binary")==0)
				rgfAlphaUsage[iObj] = ONE_BIT;
			else if(strcmp(pchTmp, "Gray")==0)
				rgfAlphaUsage[iObj] = EIGHT_BIT;				
			else if(strcmp(pchTmp, "ShapeOnly")==0)
			{
				rgbShapeOnly[iObj] = TRUE;
				rgfAlphaUsage[iObj] = ONE_BIT;
			}
			else
				fatal_error("Unknown alpha type");
			
			GetIVal(&par, "Alpha.Binary.RoundingThreshold", iObj, &rgiBinaryAlphaTH [iObj]);
			fatal_error("Binary rounding threshold must be >= 0", rgiBinaryAlphaTH [iObj] >= 0);


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
				rgiNumPbetweenIVOP [iObj] = lastFrm - firstFrm + 1;		//only 1 I at the beginning
			
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

			GetSVal(&par, "Sprite.Type", iObj, &pchTmp);
			if(strcmp(pchTmp, "None")==0)
				rguiSpriteUsage[iObj] = 0;
			else if(strcmp(pchTmp, "Static")==0)
				rguiSpriteUsage[iObj] = 1;
			else
				fatal_error("Unknown value for Sprite.Type");
			if (rgbScalability [iObj] == TRUE && rguiSpriteUsage[iObj]==1)
				fatal_error("Sprite and scalability modes cannot be combined");

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
				GetIVal(&par, "RateControl.BitsPerVOP", iLayerObj, (Int *)&(rguiBitsBudget[iLayer][iObj]));

				Bool bRVLCEnable, bDPEnable, bVPEnable;
				readBoolParam(&par, "ErrorResil.RVLC.Enable", iLayerObj, &bRVLCEnable);
				readBoolParam(&par, "ErrorResil.DataPartition.Enable", iLayerObj, &bDPEnable);
				readBoolParam(&par, "ErrorResil.VideoPacket.Enable", iLayerObj, &bVPEnable);
				
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
				GetIVal(&par, "VOLControl.LowDelay", iObj, (Int *)&rguiLowDelay[iLayer][iObj]);

				Bool bVBVParams;
				readBoolParam(&par, "VOLControl.VBVParams.Enable", iLayerObj, &bVBVParams);
				rguiVBVParams[iLayer][iObj] = (UInt)bVBVParams;

				GetIVal(&par, "VOLControl.Bitrate", iLayerObj, (Int *)&rguiBitRate[iLayer][iObj]);
				GetIVal(&par, "VOLControl.VBVBuffer.Size", iLayerObj, (Int *)&rguiVbvBufferSize[iLayer][iObj]);
				GetIVal(&par, "VOLControl.VBVBuffer.Occupancy", iLayerObj, (Int *)&rguiVbvBufferOccupany[iLayer][iObj]);
			}

		}
	}
	else
	{
		// OLD STYLE PARAMETER FILE
		ungetc(iCh, pfPara);
		printf("Old style parameter file.\n");

		// verify version number
		nextValidLine (pfPara, pnLine);
		if ( fscanf (pfPara, "%u", &iVersion) != 1 )	{
			fprintf(stderr, "wrong parameter file format on line %d\n", *pnLine);
			exit(1);
		}

		if (iVersion != 813 && iVersion != 812 && iVersion != 814 && iVersion != 815)	{
			// version 812 does not have rounding control flags
			// version 813 does not have VOL control parameters
			// version 814 does not have skipped mb enable
			fprintf(stderr, "wrong parameter file version for %s\n",VERSION_STRING);
			exit(1);
		}

	///// WAVELET VTC: begin ///////////////////////////////
		// sarnoff: wavelet visual texture coding 
		nextValidLine (pfPara, pnLine);
		if ( fscanf (pfPara, "%d", &iVTCFlag) != 1)	{
			fprintf(stderr, "wrong parameter VTC flag on line %d\n", *pnLine);
			assert (FALSE);
		}
		assert (iVTCFlag==0 || iVTCFlag==1);

		// read VTC control file

		char VTCCtlFile[80];
		nextValidLine (pfPara, pnLine);
		if ( fscanf (pfPara, "%s", VTCCtlFile) != 1)	{
			fprintf(stderr, "wrong parameter VTC flag on line %d\n", *pnLine);
			assert (FALSE);
		}

		if (iVTCFlag==1) {
			fclose(pfPara);

			RunVTCCodec(VTCCtlFile);
			return 0;
		}

	///// WAVELET VTC: end ///////////////////////////////


		
		// frame size code

		nextValidLine (pfPara, pnLine);
		if (fscanf (pfPara, "%u", &uiFrmWidth) != 1)	{
			fprintf(stderr, "wrong parameter file format on line %d\n", *pnLine);
			exit(1);
		}
		nextValidLine (pfPara, pnLine);
		if (fscanf (pfPara, "%u", &uiFrmHeight) != 1)	{
			fprintf(stderr, "wrong parameter file format on line %d\n", *pnLine);
			exit(1);
		}

		// first and last frame number
		nextValidLine (pfPara, pnLine);
		if ( fscanf (pfPara, "%u", &firstFrm) != 1) {
			fprintf(stderr, "wrong parameter file format on line %d\n", *pnLine);
			exit(1);
		}
		nextValidLine (pfPara, pnLine);
		if ( fscanf (pfPara, "%u", &lastFrm) != 1) {
			fprintf(stderr, "wrong parameter file format on line %d\n", *pnLine);
			exit(1);
		}
		assert (lastFrm >= firstFrm);

		// NBIT: not 8-bit flag
		nextValidLine (pfPara, pnLine);
		if ( fscanf (pfPara, "%d", &bNot8Bit) != 1)	{
			fprintf(stderr, "wrong parameter file format on line %d\n", *pnLine);
			exit(1);
		}
		assert (bNot8Bit==0 || bNot8Bit==1);
		if(bNot8Bit==1)
		{
			assert(sizeof(PixelC)!=sizeof(unsigned char));
		}
		else
		{
			assert(sizeof(PixelC)==sizeof(unsigned char));
		}

		// NBIT: quant precision
		nextValidLine (pfPara, pnLine);
		if ( fscanf (pfPara, "%d", &uiQuantPrecision) != 1)	{
			fprintf(stderr, "wrong parameter file format on line %d\n", *pnLine);
			exit(1);
		}
		if (bNot8Bit==0) {
			uiQuantPrecision = 5;
		}

		// NBIT: number of bits per pixel
		nextValidLine (pfPara, pnLine);
		if ( fscanf (pfPara, "%d", &nBits) != 1)	{
			fprintf(stderr, "wrong parameter file format on line %d\n", *pnLine);
			exit(1);
		}
		assert (nBits>=4 && nBits<=12);
		if (bNot8Bit==0) {
			nBits = 8;
		}
		
		// object indexes
		nextValidLine (pfPara, pnLine);
		if ( fscanf (pfPara, "%u", &firstVO) != 1)	{
			fprintf(stderr, "wrong parameter file format on line %d\n", *pnLine);
			exit(1);
		}
		nextValidLine (pfPara, pnLine);
		if ( fscanf (pfPara, "%u", &lastVO) != 1)	{
			fprintf(stderr, "wrong parameter file format on line %d\n", *pnLine);
			exit(1);
		}
		assert (lastVO >= firstVO);
		UInt nVO = lastVO - firstVO + 1;

		// allocate per-vo parameters
		rgiTemporalScalabilityType = new Int [nVO];
		rgbSpatialScalability = new Bool [nVO];
		rgbScalability = new Bool [nVO];
		rgiEnhancementType = new Int [nVO];
		rgfAlphaUsage = new AlphaUsage [nVO];
		rgbShapeOnly = new Bool [nVO];
		rgiBinaryAlphaTH = new Int [nVO];
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
		}


		//scalability indicators: 1 = temporal, 2 = spatial scalability


		nextValidLine (pfPara, pnLine);
		bAnyScalability = FALSE;
		for (iObj = 0; iObj < nVO; iObj++)	{
			if (fscanf (pfPara, "%d", &rgbScalability [iObj]) != 1)	{
				fprintf(stderr, "wrong parameter file format on line %d\n", *pnLine);
				exit (1);
			}
			if (rgbScalability[iObj] == TEMPORAL_SCALABILITY || 
				rgbScalability[iObj] == SPATIAL_SCALABILITY)
				bAnyScalability = TRUE;
			else
				assert (rgbScalability[iObj] == NO_SCALABILITY);
			if(rgbScalability[iObj] == SPATIAL_SCALABILITY || rgbScalability[iObj] == TEMPORAL_SCALABILITY) // modifiedy by Sharp(98/2/12)
				rgbSpatialScalability[iObj] = TRUE; 
			else 
				rgbSpatialScalability[iObj] = FALSE; 
		}

		//coded added by Sony, only deals with ONE VO.
		//Type option of Spatial Scalable Coding
		//This parameter is used for dicision VOP prediction types of Enhancement layer in Spatial Scalable Coding
		//If this option is set to 0, Enhancement layer is coded as "PPPPPP......",
		//else if set to 1 ,It's coded as "PBBBB......."
		nextValidLine(pfPara,pnLine);
		assert (nVO == 1);
		fscanf(pfPara,"%d",&iSpatialOption);
		if(rgbScalability[0] == SPATIAL_SCALABILITY)
			if (iSpatialOption == 1)
					fprintf(stdout,"Enhancement layer is coded as \"PPPPP.....\"\n");
			else if (iSpatialOption == 0)
					fprintf(stdout,"Enhancement layer is coded as \"PBBBB.....\"\n");
			else {
				fprintf(stderr,"The parameter \"SpatialOption\" is not set correctly\n");
				exit(1);
			}

		//Load enhancement layer (Spatial Scalable) size
		nextValidLine(pfPara,pnLine);
		fscanf(pfPara,"%d",&uiFrmWidth_SS);
		fscanf(pfPara,"%d",&uiFrmHeight_SS);

		//load upsampling factor 
		nextValidLine(pfPara,pnLine);
		fscanf(pfPara,"%d",&uiHor_sampling_n);
		fscanf(pfPara,"%d",&uiHor_sampling_m);

		nextValidLine(pfPara,pnLine);
		fscanf(pfPara,"%d",&uiVer_sampling_n);
		fscanf(pfPara,"%d",&uiVer_sampling_m);

		// form of temporal scalability indicators
		// case 0  Enhn    P   P   ....
		//         Base  I   P   P ....
		// case 1  Enhn    B B   B B   ....
		//         Base  I     P     P ....
		// case 2  Enhn    P   B   B   ....
		//         Base  I   B   P   B ....
		nextValidLine (pfPara, pnLine);
		for (iObj = 0; iObj < nVO; iObj++)	{
			if (fscanf (pfPara, "%d", &rgiTemporalScalabilityType [iObj]) != 1)	{
				fprintf(stderr, "wrong parameter file format on line %d\n", *pnLine);
				exit (1);
			}
			assert (rgiTemporalScalabilityType [iObj] == 0 || 
				rgiTemporalScalabilityType [iObj] == 1 || 
				rgiTemporalScalabilityType [iObj] == 2 ||
				rgiTemporalScalabilityType [iObj] == 3 ||
				rgiTemporalScalabilityType [iObj] == 4);
		}

		// enhancement_type for scalability
		nextValidLine (pfPara, pnLine);
		for (iObj = 0; iObj < nVO; iObj++)	{
			if (fscanf (pfPara, "%d", &rgiEnhancementType [iObj]) != 1)	{
				fprintf(stderr, "wrong parameter file format on line %d\n", *pnLine);
				exit (1);
			}
			assert (rgiEnhancementType [iObj] == 0 || //  entire region of the base layer is enhanced
	// begin: modified by Sharp (98/3/24)
				rgiEnhancementType [iObj] == 1 ||  // partial region of the base layer is enhanced (with background composition)
				rgiEnhancementType [iObj] == 2);  // partial region of the base layer is enhanced (without background composition)
	// end: modified by Sharp (98/3/24)
		}

		// rate control flag
		nextValidLine (pfPara, pnLine);
		for (iObj = 0; iObj < nVO; iObj++)	{
			if (fscanf (pfPara, "%u", &rguiRateControl [BASE_LAYER] [iObj] ) != 1)	{
				fprintf(stderr, "wrong parameter file format on line %d\n", *pnLine);
				exit (1);
			}
			assert (rguiRateControl [BASE_LAYER] [iObj]  == RC_MPEG4 || 
					rguiRateControl [BASE_LAYER] [iObj]  == RC_TM5 ||
					rguiRateControl [BASE_LAYER] [iObj]  == 0);
		}
		if (bAnyScalability)	{
			for (iObj = 0; iObj < nVO; iObj++)	{
				if (fscanf (pfPara, "%u", &rguiRateControl [ENHN_LAYER] [iObj]) != 1)	{
					fprintf(stderr, "wrong parameter file format on line %d\n", *pnLine);
					exit (1);
				}
				assert (rguiRateControl [ENHN_LAYER] [iObj]  == 0 || 
						rguiRateControl [ENHN_LAYER] [iObj]  == RC_MPEG4 ||
						rguiRateControl [ENHN_LAYER] [iObj]  == RC_TM5);
			}
		}

		// bit budget for each object.
		nextValidLine (pfPara, pnLine);
		for (iObj = 0; iObj < nVO; iObj++)	{
			if (fscanf (pfPara, "%d", &rguiBitsBudget [BASE_LAYER] [iObj]) != 1)	{
				fprintf(stderr, "wrong parameter file format on line %d\n", *pnLine);
				exit (1);
			}
		}
		if (bAnyScalability)	{
			for (iObj = 0; iObj < nVO; iObj++)	{
				if (fscanf (pfPara, "%d", &rguiBitsBudget [ENHN_LAYER] [iObj]) != 1)	{
					fprintf(stderr, "wrong parameter file format on line %d\n", *pnLine);
					exit (1);
				}
				assert (rguiBitsBudget [ENHN_LAYER] [iObj]  > 0);
			}
		}


		// alpha usage for each object.  0: rectangle, 1: binary, 2: 8-bit, 3: shape only
		nextValidLine (pfPara, pnLine);
		for (iObj = 0; iObj < nVO; iObj++)	{
			UInt uiAlpha;
			if (fscanf (pfPara, "%d", &uiAlpha) != 1)	{
				fprintf(stderr, "wrong parameter file format on line %d\n", *pnLine);
				exit (1);
			}
			else {
				assert (uiAlpha == 0 || uiAlpha == 1 || uiAlpha == 2 || uiAlpha == 3);
				if(uiAlpha<3)
				{
					rgfAlphaUsage [iObj] = (AlphaUsage) uiAlpha;
					rgbShapeOnly [iObj] = FALSE;
				}
				else
				{
					rgfAlphaUsage [iObj] = ONE_BIT;
					rgbShapeOnly [iObj] = TRUE;
				}
			}
		}

		// binary shape rounding para
		nextValidLine (pfPara, pnLine);
		for (iObj = 0; iObj < nVO; iObj++)	{
			if (fscanf (pfPara, "%d", &rgiBinaryAlphaTH [iObj]) != 1)	{
				fprintf	(stderr, "wrong parameter file format on line %d\n", *pnLine);
				exit (1);
			}
			assert (rgiBinaryAlphaTH [iObj] >= 0);
		}

		// binary shape size conversion flag
		nextValidLine (pfPara, pnLine);
		for (iObj = 0; iObj < nVO; iObj++)	{
			if (fscanf (pfPara, "%d", &rgbNoCrChange [iObj]) != 1)	{
				fprintf	(stderr, "wrong parameter file format on line %d\n", *pnLine);
				exit (1);
			}
			assert (rgbNoCrChange [iObj] == TRUE || rgbNoCrChange [iObj] == FALSE); //boolean value
			if (rgiBinaryAlphaTH [iObj] == 0)
				assert (rgbNoCrChange [iObj] == TRUE); //MB-level size conversion of shape is off in lossless mode\n");
		}

		//	Added for error resilient mode by Toshiba(1997-11-14)
		nextValidLine (pfPara, pnLine);
		for (iObj = 0; iObj < nVO; iObj++)	{
			if (fscanf (pfPara, "%d", &rgiBinaryAlphaRR [iObj]) != 1)	{
				fprintf	(stderr, "wrong parameter file format on line %d\n", *pnLine);
				exit (1);
			}
			assert (rgiBinaryAlphaRR [iObj] >= 0);
		}
		// End Toshiba(1997-11-14)

		// rounding control disable
		if(iVersion > 812)
			nextValidLine (pfPara, pnLine);
		for (iObj = 0; iObj < nVO; iObj++)	{
			if(iVersion > 812)
			{
				if (fscanf (pfPara, "%d", &rgbRoundingControlDisable [iObj]) != 1)	{
					fprintf(stderr, "wrong parameter file format on line %d\n", *pnLine);
					exit (1);
				}
			}
			else
				rgbRoundingControlDisable [iObj] = 0;
			assert (rgbRoundingControlDisable [iObj]  == 0 || rgbRoundingControlDisable [iObj] == 1);
		}
		if(iVersion > 812)
			nextValidLine (pfPara, pnLine);
		for (iObj = 0; iObj < nVO; iObj++)	{
			if(iVersion > 812)
			{
				if (fscanf (pfPara, "%d", &rgiInitialRoundingType [iObj]) != 1)	{
					fprintf	(stderr, "wrong parameter file format on line %d\n", *pnLine);
					exit (1);
				}
			}
			else
				rgiInitialRoundingType [iObj] = 0;
			assert (rgiInitialRoundingType [iObj] == 0 || rgiInitialRoundingType [iObj] == 1);
		}

		// error resilient coding disable
		nextValidLine (pfPara, pnLine);
		Int iErrorResilientFlags;
		for (iObj = 0; iObj < nVO; iObj++)	{
			if (fscanf (pfPara, "%d", &iErrorResilientFlags) != 1)	{
				fprintf(stderr, "wrong parameter file format on line %d\n", *pnLine);
				exit (1);
			}
			rgbErrorResilientDisable [BASE_LAYER] [iObj] = iErrorResilientFlags & 0x1;
	//	Modified for error resilient mode by Toshiba(1998-1-16)
			rgbDataPartitioning [BASE_LAYER] [iObj]		 = (iErrorResilientFlags & 0x2) ? TRUE : FALSE;
			rgbReversibleVlc [BASE_LAYER] [iObj]		 = (iErrorResilientFlags & 0x4) ? TRUE : FALSE;
	//	End Toshiba(1998-1-16)
			if( rgbErrorResilientDisable [BASE_LAYER] [iObj])
				rgiBinaryAlphaRR [iObj] = -1;
		}
		if (bAnyScalability)	{
			for (iObj = 0; iObj < nVO; iObj++)	{
				if (fscanf (pfPara, "%d", &rgbErrorResilientDisable [ENHN_LAYER] [iObj]) != 1)	{
					fprintf(stderr, "wrong parameter file format on line %d\n", *pnLine);
					exit (1);
				}
				assert (rgbErrorResilientDisable [ENHN_LAYER] [iObj] == TRUE); //error resilient coding not supported as for now
				rgbErrorResilientDisable [ENHN_LAYER] [iObj] = TRUE;
				rgbDataPartitioning [ENHN_LAYER] [iObj] = FALSE;
				rgbReversibleVlc [ENHN_LAYER] [iObj] = FALSE;
			}
		}

		// Bit threshold for video packet spacing control
		nextValidLine (pfPara, pnLine);
		for (iObj = 0; iObj < nVO; iObj++)	{
			if (fscanf (pfPara, "%d", &rgiVPBitTh [BASE_LAYER] [iObj]) != 1)	{
				fprintf(stderr, "wrong parameter file format on line %d\n", *pnLine);
				exit (1);
			}
			if (rgbErrorResilientDisable [BASE_LAYER] [iObj])
				rgiVPBitTh [BASE_LAYER] [iObj] = -1;	// set VPBitTh to negative value
		}
		if (bAnyScalability)	{
			for (iObj = 0; iObj < nVO; iObj++)	{
				if (fscanf (pfPara, "%d", &rgiVPBitTh [ENHN_LAYER] [iObj]) != 1)	{
					fprintf(stderr, "wrong parameter file format on line %d\n", *pnLine);
					exit (1);
				}
				rgiVPBitTh [ENHN_LAYER] [iObj] = -1;
			}
		}

		// Interlaced coding
		nextValidLine (pfPara, pnLine);
		for (iObj = 0; iObj < nVO; iObj++)	{
			if (fscanf (pfPara, "%d", &rgbInterlacedCoding [BASE_LAYER] [iObj]) != 1)	{
				fprintf(stderr, "wrong parameter file format on line %d\n", *pnLine);
				exit (1);
			}
			assert (rgbInterlacedCoding [BASE_LAYER] [iObj] == 0 || rgbInterlacedCoding [BASE_LAYER] [iObj] == 1);
		}
		if (bAnyScalability)	{
			for (iObj = 0; iObj < nVO; iObj++)	{
				if (fscanf (pfPara, "%d", &rgbInterlacedCoding [ENHN_LAYER] [iObj]) != 1)	{
					fprintf(stderr, "wrong parameter file format on line %d\n", *pnLine);
					exit (1);
				}
				assert (rgbInterlacedCoding [ENHN_LAYER] [iObj] == 0 || rgbInterlacedCoding [ENHN_LAYER] [iObj] == 1);
			}
		}
		
		// quantizer selection: 0 -- H.263, 1 -- MPEG
		nextValidLine (pfPara, pnLine);
		for (iObj = 0; iObj < nVO; iObj++)	{
		  int read;
		  if (fscanf (pfPara, "%d", &read) != 1) { 

				fprintf(stderr, "wrong parameter file format on line %d\n", *pnLine);
				exit (1);
			}
		  rgfQuant [BASE_LAYER] [iObj] = (Quantizer)read;
		  assert (rgfQuant [BASE_LAYER] [iObj] ==0 || rgfQuant [BASE_LAYER] [iObj] == 1);
		}
		if (bAnyScalability)	{
			for (iObj = 0; iObj < nVO; iObj++)	{
			  int read;
			  if (fscanf (pfPara, "%d", &read) != 1) {
					fprintf(stderr, "wrong parameter file format on line %d\n", *pnLine);
					exit (1);
			  }
			  rgfQuant [ENHN_LAYER] [iObj] = (Quantizer)read;
				assert (rgfQuant [ENHN_LAYER] [iObj] ==0 || rgfQuant [ENHN_LAYER] [iObj] == 1);
			}
		}

		// load non-default intra Q-Matrix, 0 -- FALSE, 1 -- TRUE
		nextValidLine (pfPara, pnLine);
		for (iObj = 0; iObj < nVO; iObj++)	{
			if (fscanf (pfPara, "%d", &rgbLoadIntraMatrix [BASE_LAYER] [iObj]) != 1)	{
				fprintf(stderr, "wrong parameter file format on line %d\n", *pnLine);
				exit (1);
			}
			assert (rgbLoadIntraMatrix [BASE_LAYER] [iObj] ==0 || 
					rgbLoadIntraMatrix [BASE_LAYER] [iObj] == 1);
			if (rgbLoadIntraMatrix [BASE_LAYER] [iObj]) {
				UInt i = 0;
				do {
					if (fscanf (pfPara, "%d", &rgppiIntraQuantizerMatrix [BASE_LAYER] [iObj] [i]) != 1)	{
						fprintf(stderr, "wrong parameter file format on line %d\n", *pnLine);
						exit (1);
					}
				} while (rgppiIntraQuantizerMatrix [BASE_LAYER] [iObj] [i] != 0 && ++i < BLOCK_SQUARE_SIZE);
			}
			else
				memcpy (rgppiIntraQuantizerMatrix [BASE_LAYER] [iObj], rgiDefaultIntraQMatrix, BLOCK_SQUARE_SIZE * sizeof (Int));
		}
		if (bAnyScalability)	{
			for (iObj = 0; iObj < nVO; iObj++)	{
				if (fscanf (pfPara, "%d", &rgbLoadIntraMatrix [ENHN_LAYER] [iObj]) != 1)	{
					fprintf(stderr, "wrong parameter file format on line %d\n", *pnLine);
					exit (1);
				}
				assert (rgbLoadIntraMatrix [ENHN_LAYER] [iObj] ==0 || 
						rgbLoadIntraMatrix [ENHN_LAYER] [iObj] == 1);
				if (rgbLoadIntraMatrix [ENHN_LAYER] [iObj]) {
					UInt i = 0;
					do {
						if (fscanf (pfPara, "%d", &rgppiIntraQuantizerMatrix [ENHN_LAYER] [iObj] [i]) != 1)	{
							fprintf(stderr, "wrong parameter file format on line %d\n", *pnLine);
							exit (1);
						}
					} while (rgppiIntraQuantizerMatrix [ENHN_LAYER] [iObj] [i] != 0 && ++i < BLOCK_SQUARE_SIZE);
				}
				else
					memcpy (rgppiIntraQuantizerMatrix [ENHN_LAYER] [iObj], rgiDefaultIntraQMatrix, BLOCK_SQUARE_SIZE * sizeof (Int));
			}
		}

		// load non-default inter Q-Matrix, 0 -- FALSE, 1 -- TRUE
		nextValidLine (pfPara, pnLine);
		for (iObj = 0; iObj < nVO; iObj++)	{
			if (fscanf (pfPara, "%d", &rgbLoadInterMatrix [BASE_LAYER] [iObj]) != 1)	{
				fprintf(stderr, "wrong parameter file format on line %d\n", *pnLine);
				exit (1);
			}
			assert (rgbLoadInterMatrix [BASE_LAYER] [iObj] ==0 || 
					rgbLoadInterMatrix [BASE_LAYER] [iObj] == 1);
			if (rgbLoadInterMatrix [BASE_LAYER] [iObj]) {
				UInt i = 0;
				do {
					if (fscanf (pfPara, "%d", &rgppiInterQuantizerMatrix [BASE_LAYER] [iObj] [i]) != 1)	{
						fprintf(stderr, "wrong parameter file format on line %d\n", *pnLine);
						exit (1);
					}
				} while (rgppiInterQuantizerMatrix [BASE_LAYER] [iObj] [i] != 0 && ++i < BLOCK_SQUARE_SIZE);
			}
			else
				memcpy (rgppiInterQuantizerMatrix [BASE_LAYER] [iObj], rgiDefaultInterQMatrix, BLOCK_SQUARE_SIZE * sizeof (Int));
		}
		if (bAnyScalability)	{
			for (iObj = 0; iObj < nVO; iObj++)	{
				if (fscanf (pfPara, "%d", &rgbLoadInterMatrix [ENHN_LAYER] [iObj]) != 1)	{
					fprintf(stderr, "wrong parameter file format on line %d\n", *pnLine);
					exit (1);
				}
				assert (rgbLoadInterMatrix [ENHN_LAYER] [iObj] ==0 || 
						rgbLoadInterMatrix [ENHN_LAYER] [iObj] == 1);
				if (rgbLoadInterMatrix [ENHN_LAYER] [iObj]) {
					UInt i = 0;
					do {
						if (fscanf (pfPara, "%d", &rgppiInterQuantizerMatrix [ENHN_LAYER] [iObj] [i]) != 1)	{
							fprintf(stderr, "wrong parameter file format on line %d\n", *pnLine);
							exit (1);
						}
					} while (rgppiInterQuantizerMatrix [ENHN_LAYER] [iObj] [i] != 0 && ++i < BLOCK_SQUARE_SIZE);
				}
				else
					memcpy (rgppiInterQuantizerMatrix [ENHN_LAYER] [iObj], rgiDefaultInterQMatrix, BLOCK_SQUARE_SIZE * sizeof (Int));
			}
		}

		// threhold to code Intra-DC as AC
		nextValidLine (pfPara, pnLine);
		for (iObj = 0; iObj < nVO; iObj++)	{
			if (fscanf (pfPara, "%d", &(rgiIntraDCSwitchingThr [BASE_LAYER] [iObj])) != 1)	{
				fprintf(stderr, "wrong parameter file format on line %d\n", *pnLine);
				exit (1);
			}
			assert (rgiIntraDCSwitchingThr [BASE_LAYER] [iObj] >= 0 && 
					rgiIntraDCSwitchingThr [BASE_LAYER] [iObj] <= 7);
		}
		if (bAnyScalability)	{
			for (iObj = 0; iObj < nVO; iObj++)	{
				if (fscanf (pfPara, "%d", &(rgiIntraDCSwitchingThr [ENHN_LAYER] [iObj])) != 1)	{
					fprintf(stderr, "wrong parameter file format on line %d\n", *pnLine);
					exit (1);
				}
				assert (rgiIntraDCSwitchingThr [ENHN_LAYER] [iObj] >= 0 && 
						rgiIntraDCSwitchingThr [ENHN_LAYER] [iObj] <= 7);
			}
		}

		// I-VO quantization stepsize
		nextValidLine (pfPara, pnLine);
		for (iObj = 0; iObj < nVO; iObj++)	{
			if (fscanf (pfPara, "%d", &(rgiIStep [BASE_LAYER] [iObj])) != 1)	{
				fprintf(stderr, "wrong parameter file format on line %d\n", *pnLine);
				exit (1);
			}
			assert (rgiIStep [BASE_LAYER] [iObj] > 0 && rgiIStep [BASE_LAYER] [iObj] < (1<<uiQuantPrecision));
		}
		if (bAnyScalability)	{
			for (iObj = 0; iObj < nVO; iObj++)	{
				if (fscanf (pfPara, "%d", &(rgiIStep [ENHN_LAYER] [iObj])) != 1)	{
					fprintf(stderr, "wrong parameter file format on line %d\n", *pnLine);
					exit (1);
				}
				assert (rgiIStep [ENHN_LAYER] [iObj] > 0 && rgiIStep [ENHN_LAYER] [iObj] < (1<<uiQuantPrecision));
			}
		}

		// P-VO quantization stepsize
		nextValidLine (pfPara, pnLine);
		for (iObj = 0; iObj < nVO; iObj++)	{
			if (fscanf (pfPara, "%d", &(rgiPStep [BASE_LAYER] [iObj])) != 1)	{
				fprintf(stderr, "wrong parameter file format on line %d\n", *pnLine);
				exit (1);
			}
			assert (rgiPStep [BASE_LAYER] [iObj] > 0 && rgiPStep [BASE_LAYER] [iObj] < (1<<uiQuantPrecision));
		}
		if (bAnyScalability)	{
			for (iObj = 0; iObj < nVO; iObj++)	{
				if (fscanf (pfPara, "%d", &(rgiPStep [ENHN_LAYER] [iObj])) != 1)	{
					fprintf(stderr, "wrong parameter file format on line %d\n", *pnLine);
					exit (1);
				}
				assert (rgiPStep [ENHN_LAYER] [iObj] > 0 && rgiPStep [ENHN_LAYER] [iObj] < (1<<uiQuantPrecision));
			}
		}

		// quantization stepsize for B-VOP
		nextValidLine (pfPara, pnLine);
		for (iObj = 0; iObj < nVO; iObj++)	{
			if (fscanf (pfPara, "%d", &(rgiStepBCode [BASE_LAYER] [iObj])) != 1)	{
				fprintf(stderr, "wrong parameter file format on line %d\n", *pnLine);
				exit (1);
			}
			assert (rgiStepBCode [BASE_LAYER] [iObj] > 0 && rgiStepBCode [BASE_LAYER] [iObj] < (1<<uiQuantPrecision));
		}
		if (bAnyScalability)	{
			for (iObj = 0; iObj < nVO; iObj++)	{
				if (fscanf (pfPara, "%d", &(rgiStepBCode [ENHN_LAYER] [iObj])) != 1)	{
					fprintf(stderr, "wrong parameter file format on line %d\n", *pnLine);
					exit (1);
				}
				assert (rgiStepBCode [ENHN_LAYER] [iObj] > 0 && rgiStepBCode [ENHN_LAYER] [iObj] < (1<<uiQuantPrecision));
			}
		}

		// load non-default gray alpha intra Q-Matrix, 0 -- FALSE, 1 -- TRUE
		nextValidLine (pfPara, pnLine);
		for (iObj = 0; iObj < nVO; iObj++)	{
			if (fscanf (pfPara, "%d", &rgbLoadIntraMatrixAlpha [BASE_LAYER] [iObj]) != 1)	{
				fprintf(stderr, "wrong parameter file format on line %d\n", *pnLine);
				exit (1);
			}
			assert (rgbLoadIntraMatrixAlpha [BASE_LAYER] [iObj] ==0 || 
					rgbLoadIntraMatrixAlpha [BASE_LAYER] [iObj] == 1);
			if (rgbLoadIntraMatrixAlpha [BASE_LAYER] [iObj]) {
				for (UInt i = 0; i < BLOCK_SQUARE_SIZE; i++) {
					if (fscanf (pfPara, "%d", &rgppiIntraQuantizerMatrixAlpha [BASE_LAYER] [iObj] [i]) != 1)	{
						fprintf(stderr, "wrong parameter file format on line %d\n", *pnLine);
						exit (1);
					}
				}
			}
			else
				memcpy (rgppiIntraQuantizerMatrixAlpha [BASE_LAYER] [iObj], rgiDefaultIntraQMatrixAlpha, BLOCK_SQUARE_SIZE * sizeof (Int));
		}
		if (bAnyScalability)	{
			for (iObj = 0; iObj < nVO; iObj++)	{
				if (fscanf (pfPara, "%d", &rgbLoadIntraMatrixAlpha [ENHN_LAYER] [iObj]) != 1)	{
					fprintf(stderr, "wrong parameter file format on line %d\n", *pnLine);
					exit (1);
				}
				assert (rgbLoadIntraMatrixAlpha [ENHN_LAYER] [iObj] ==0 || 
						rgbLoadIntraMatrixAlpha [ENHN_LAYER] [iObj] == 1);
				if (rgbLoadIntraMatrixAlpha [ENHN_LAYER] [iObj]) {
					for (UInt i = 0; i < BLOCK_SQUARE_SIZE; i++) {
						if (fscanf (pfPara, "%d", &rgppiIntraQuantizerMatrixAlpha [ENHN_LAYER] [iObj] [i]) != 1)	{
							fprintf(stderr, "wrong parameter file format on line %d\n", *pnLine);
							exit (1);
						}
					}
				}
				else
					memcpy (rgppiIntraQuantizerMatrixAlpha [ENHN_LAYER] [iObj], rgiDefaultIntraQMatrixAlpha, BLOCK_SQUARE_SIZE * sizeof (Int));
			}
		}

		// load non-default  gray alpha inter Q-Matrix, 0 -- FALSE, 1 -- TRUE
		nextValidLine (pfPara, pnLine);
		for (iObj = 0; iObj < nVO; iObj++)	{
			if (fscanf (pfPara, "%d", &rgbLoadInterMatrixAlpha [BASE_LAYER] [iObj]) != 1)	{
				fprintf(stderr, "wrong parameter file format on line %d\n", *pnLine);
				exit (1);
			}
			assert (rgbLoadInterMatrixAlpha [BASE_LAYER] [iObj] ==0 || 
					rgbLoadInterMatrixAlpha [BASE_LAYER] [iObj] == 1);
			if (rgbLoadInterMatrixAlpha [BASE_LAYER] [iObj]) {
				for (UInt i = 0; i < BLOCK_SQUARE_SIZE; i++) {
					if (fscanf (pfPara, "%d", &rgppiInterQuantizerMatrixAlpha [BASE_LAYER] [iObj] [i]) != 1)	{
						fprintf(stderr, "wrong parameter file format on line %d\n", *pnLine);
						exit (1);
					}
				}
			}
			else
				memcpy (rgppiInterQuantizerMatrixAlpha [BASE_LAYER] [iObj], rgiDefaultInterQMatrixAlpha, BLOCK_SQUARE_SIZE * sizeof (Int));
		}
		if (bAnyScalability)	{
			for (iObj = 0; iObj < nVO; iObj++)	{
				if (fscanf (pfPara, "%d", &rgbLoadInterMatrixAlpha [ENHN_LAYER] [iObj]) != 1)	{
					fprintf(stderr, "wrong parameter file format on line %d\n", *pnLine);
					exit (1);
				}
				assert (rgbLoadInterMatrixAlpha [ENHN_LAYER] [iObj] ==0 || 
						rgbLoadInterMatrixAlpha [ENHN_LAYER] [iObj] == 1);
				if (rgbLoadInterMatrixAlpha [ENHN_LAYER] [iObj]) {
					for (UInt i = 0; i < BLOCK_SQUARE_SIZE; i++) {
						if (fscanf (pfPara, "%d", &rgppiInterQuantizerMatrixAlpha [ENHN_LAYER] [iObj] [i]) != 1)	{
							fprintf(stderr, "wrong parameter file format on line %d\n", *pnLine);
							exit (1);
						}
					}
				}
				else
					memcpy (rgppiInterQuantizerMatrixAlpha [ENHN_LAYER] [iObj], rgiDefaultInterQMatrixAlpha, BLOCK_SQUARE_SIZE * sizeof (Int));
			}
		}

		// I-VO quantization stepsize for Alpha
		nextValidLine (pfPara, pnLine);
		for (iObj = 0; iObj < nVO; iObj++)	{
			if (fscanf (pfPara, "%d", &(rgiIStepAlpha [BASE_LAYER] [iObj])) != 1)	{
				fprintf(stderr, "wrong parameter file format on line %d\n", *pnLine);
				exit (1);
			}
			assert (rgiIStepAlpha [BASE_LAYER] [iObj] > 0 && rgiIStepAlpha [BASE_LAYER] [iObj] < 32);
		}
		if (bAnyScalability)	{
			for (iObj = 0; iObj < nVO; iObj++)	{
				if (fscanf (pfPara, "%d", &(rgiIStepAlpha [ENHN_LAYER] [iObj])) != 1)	{
					fprintf(stderr, "wrong parameter file format on line %d\n", *pnLine);
					exit (1);
				}
				assert (rgiIStepAlpha [ENHN_LAYER] [iObj] > 0 && rgiIStepAlpha [ENHN_LAYER] [iObj] < 32);
			}
		}

		// P-VO quantization stepsize for Alpha
		nextValidLine (pfPara, pnLine);
		for (iObj = 0; iObj < nVO; iObj++)	{
			if (fscanf (pfPara, "%d", &(rgiPStepAlpha [BASE_LAYER] [iObj])) != 1)	{
				fprintf(stderr, "wrong parameter file format on line %d\n", *pnLine);
				exit (1);
			}
			assert (rgiPStepAlpha [BASE_LAYER] [iObj] > 0 && rgiPStepAlpha [BASE_LAYER] [iObj] < 64);
		}
		if (bAnyScalability)	{
			for (iObj = 0; iObj < nVO; iObj++)	{
				if (fscanf (pfPara, "%d", &(rgiPStepAlpha [ENHN_LAYER] [iObj])) != 1)	{
					fprintf(stderr, "wrong parameter file format on line %d\n", *pnLine);
					exit (1);
				}
				assert (rgiPStepAlpha [ENHN_LAYER] [iObj] > 0 && rgiPStepAlpha [ENHN_LAYER] [iObj] < 64);
			}
		}

		// B-VO quantization stepsize for Alpha
		nextValidLine (pfPara, pnLine);
		for (iObj = 0; iObj < nVO; iObj++)	{
			if (fscanf (pfPara, "%d", &(rgiBStepAlpha [BASE_LAYER] [iObj])) != 1)	{
				fprintf(stderr, "wrong parameter file format on line %d\n", *pnLine);
				exit (1);
			}
			assert (rgiBStepAlpha [BASE_LAYER] [iObj] > 0 && rgiBStepAlpha [BASE_LAYER] [iObj] < 64);
		}
		if (bAnyScalability)	{
			for (iObj = 0; iObj < nVO; iObj++)	{
				if (fscanf (pfPara, "%d", &(rgiBStepAlpha [ENHN_LAYER] [iObj])) != 1)	{
					fprintf(stderr, "wrong parameter file format on line %d\n", *pnLine);
					exit (1);
				}
				assert (rgiBStepAlpha [ENHN_LAYER] [iObj] > 0 && rgiBStepAlpha [ENHN_LAYER] [iObj] < 64);
			}
		}

		// disable_gray_quant_update
		nextValidLine (pfPara, pnLine);
		for (iObj = 0; iObj < nVO; iObj++)	{
			if (fscanf (pfPara, "%d", &(rgbNoGrayQuantUpdate [BASE_LAYER] [iObj])) != 1)	{
				fprintf(stderr, "wrong parameter file format on line %d\n", *pnLine);
				exit (1);
			}
		}
		if (bAnyScalability)	{
			for (iObj = 0; iObj < nVO; iObj++)	{
				if (fscanf (pfPara, "%d", &(rgbNoGrayQuantUpdate [ENHN_LAYER] [iObj])) != 1)	{
					fprintf(stderr, "wrong parameter file format on line %d\n", *pnLine);
					exit (1);
				}
			}
		}

		// number of P-VOP's between 2 I-VOP's; if there are B-VOPs, the no. of encode frames will multiply
		nextValidLine (pfPara, pnLine);
		for (iObj = 0; iObj < nVO; iObj++)	{
			if (fscanf (pfPara, "%d", &rgiNumPbetweenIVOP [iObj]) != 1)	{
				fprintf(stderr, "wrong parameter file format on line %d\n", *pnLine);
				exit (1);
			}
			if (rgiNumPbetweenIVOP [iObj] < 0)
				rgiNumPbetweenIVOP [iObj] = lastFrm - firstFrm + 1;		//only 1 I at the beginning
		}

		// number of B-VOP's between 2 P-VOP's
		nextValidLine (pfPara, pnLine);
		for (iObj = 0; iObj < nVO; iObj++)	{
			if (fscanf (pfPara, "%d", &rgiNumBbetweenPVOP [iObj]) != 1)	{
				fprintf(stderr, "wrong parameter file format on line %d\n", *pnLine);
				exit (1);
			}
			assert (rgiNumBbetweenPVOP [iObj] >= 0);
		}

		if(iVersion>814)
		{
			// rgbAllowSkippedPMBs
			nextValidLine (pfPara, pnLine);
			for (iObj = 0; iObj < nVO; iObj++)	{
				if (fscanf (pfPara, "%d", &rgbAllowSkippedPMBs [iObj]) != 1)	{
					fprintf(stderr, "wrong parameter file format on line %d\n", *pnLine);
					exit (1);
				}
				assert (rgbAllowSkippedPMBs [iObj] == 0 || rgbAllowSkippedPMBs[iObj]==1);
			}
		}
		else
			for (iObj = 0; iObj < nVO; iObj++)
				rgbAllowSkippedPMBs [iObj] = 1;

		//added to encode GOV header by SONY 980212
		//number of VOP between GOV header
		//CAUTION:GOV period is not used in spatial scalable coding
		nextValidLine (pfPara, pnLine);
		for (iObj = 0; iObj < nVO; iObj++)	{
			if (fscanf (pfPara, "%d", &rgiGOVperiod [iObj]) != 1)	{
				fprintf(stderr, "wrong parameter file format on line %d\n", *pnLine);
				exit (1);
			}
			assert (rgiGOVperiod [iObj] >= 0);
		}
		//980212

		// deblocking filter disable
		nextValidLine (pfPara, pnLine);
		for (iObj = 0; iObj < nVO; iObj++)	{
			if (fscanf (pfPara, "%d", &rgbDeblockFilterDisable [iObj]) != 1)	{
				fprintf(stderr, "wrong parameter file format on line %d\n", *pnLine);
				exit (1);
			}
			if (rgbDeblockFilterDisable [iObj] != TRUE)	{
				fprintf(stderr, "Deblocking filter is not supported in this version\n");
				exit (1);
			}
		}

		// Temporal sampling rate
		nextValidLine (pfPara, pnLine);
		for (iObj = 0; iObj < nVO; iObj++)	{
			if (fscanf (pfPara, "%d", &rgiTSRate [iObj]) != 1)	{
				fprintf(stderr, "wrong parameter file format on line %d\n", *pnLine);
				exit (1);
			}
			assert (rgiTSRate [iObj] >= 1);
		}
		if ( bAnyScalability ){ // This part is added by Norio Ito
			for (iObj = 0; iObj < nVO; iObj++)	{
				if (fscanf (pfPara, "%d", &rgiEnhcTSRate [iObj]) != 1)	{
					fprintf(stderr, "wrong parameter file format on line %d\n", *pnLine);
					exit (1);
				}
				assert (rgiEnhcTSRate [iObj] >= 1);
			}
		}

		// maximum displacement
		nextValidLine (pfPara, pnLine);
		for (iObj = 0; iObj < nVO; iObj++) {
			if (fscanf (pfPara, "%d", &rguiSearchRange [BASE_LAYER] [iObj]) != 1)	{
				fprintf(stderr, "wrong parameter file format on line %d\n", *pnLine);
				exit (1);
			}
			assert (rguiSearchRange [BASE_LAYER] [iObj] >= 1);
		}
		if (bAnyScalability)	{
			for (iObj = 0; iObj < nVO; iObj++) {
				if (fscanf (pfPara, "%d", &rguiSearchRange [ENHN_LAYER] [iObj]) != 1)	{
					fprintf(stderr, "wrong parameter file format on line %d\n", *pnLine);
					exit (1);
				}
				assert (rguiSearchRange [ENHN_LAYER] [iObj] >= 1);
			}
		}

		// use original or reconstructed previous VO for ME
		nextValidLine (pfPara, pnLine);
		for (iObj = 0; iObj < nVO; iObj++)	{
			if (fscanf (pfPara, "%d", &rgbOriginalME [BASE_LAYER] [iObj]) != 1)	{
				fprintf(stderr, "wrong parameter file format on line %d\n", *pnLine);
				exit (1);
			}
			assert (rgbOriginalME [BASE_LAYER] [iObj] == 0 || rgbOriginalME [BASE_LAYER] [iObj] == 1);
		}
		if (bAnyScalability)	{
			for (iObj = 0; iObj < nVO; iObj++)	{
				if (fscanf (pfPara, "%d", &rgbOriginalME [ENHN_LAYER] [iObj]) != 1)	{
					fprintf(stderr, "wrong parameter file format on line %d\n", *pnLine);
					exit (1);
				}
				assert (rgbOriginalME [ENHN_LAYER] [iObj] == 0 || rgbOriginalME [ENHN_LAYER] [iObj] == 1);
			}
		}

		// disable advance prediction (obmc only)?
		nextValidLine (pfPara, pnLine);
		for (iObj = 0; iObj < nVO; iObj++)	{
			if (fscanf (pfPara, "%d", &rgbAdvPredDisable [BASE_LAYER] [iObj]) != 1)	{
				fprintf(stderr, "wrong parameter file format on line %d\n", *pnLine);
				exit (1);
			}
			assert (rgbAdvPredDisable [BASE_LAYER] [iObj] == 0 || rgbAdvPredDisable [BASE_LAYER] [iObj] == 1);
		}
		if (bAnyScalability)	{
			for (iObj = 0; iObj < nVO; iObj++)	{
				if (fscanf (pfPara, "%d", &rgbAdvPredDisable [ENHN_LAYER] [iObj]) != 1)	{
					fprintf(stderr, "wrong parameter file format on line %d\n", *pnLine);
					exit (1);
				}
				assert (rgbAdvPredDisable [ENHN_LAYER] [iObj] == 0 || rgbAdvPredDisable [ENHN_LAYER] [iObj] == 1);
			}
		}

		// START: Complexity Estimation syntax support - Marc Mongenet (EPFL) - 16 Jun 1998
		readBoolVOLFlag (rgbComplexityEstimationDisable, nVO, pfPara, pnLine, bAnyScalability);
		readBoolVOLFlag (rgbOpaque, nVO, pfPara, pnLine, bAnyScalability);
		readBoolVOLFlag (rgbTransparent, nVO, pfPara, pnLine, bAnyScalability);
		readBoolVOLFlag (rgbIntraCAE, nVO, pfPara, pnLine, bAnyScalability);
		readBoolVOLFlag (rgbInterCAE, nVO, pfPara, pnLine, bAnyScalability);
		readBoolVOLFlag (rgbNoUpdate, nVO, pfPara, pnLine, bAnyScalability);
		readBoolVOLFlag (rgbUpsampling, nVO, pfPara, pnLine, bAnyScalability);
		readBoolVOLFlag (rgbIntraBlocks, nVO, pfPara, pnLine, bAnyScalability);
		readBoolVOLFlag (rgbInterBlocks, nVO, pfPara, pnLine, bAnyScalability);
		readBoolVOLFlag (rgbInter4vBlocks, nVO, pfPara, pnLine, bAnyScalability);
		readBoolVOLFlag (rgbNotCodedBlocks, nVO, pfPara, pnLine, bAnyScalability);
		readBoolVOLFlag (rgbDCTCoefs, nVO, pfPara, pnLine, bAnyScalability);
		readBoolVOLFlag (rgbDCTLines, nVO, pfPara, pnLine, bAnyScalability);
		readBoolVOLFlag (rgbVLCSymbols, nVO, pfPara, pnLine, bAnyScalability);
		readBoolVOLFlag (rgbVLCBits, nVO, pfPara, pnLine, bAnyScalability);
		readBoolVOLFlag (rgbAPM, nVO, pfPara, pnLine, bAnyScalability);
		readBoolVOLFlag (rgbNPM, nVO, pfPara, pnLine, bAnyScalability);
		readBoolVOLFlag (rgbInterpolateMCQ, nVO, pfPara, pnLine, bAnyScalability);
		readBoolVOLFlag (rgbForwBackMCQ, nVO, pfPara, pnLine, bAnyScalability);
		readBoolVOLFlag (rgbHalfpel2, nVO, pfPara, pnLine, bAnyScalability);
		readBoolVOLFlag (rgbHalfpel4, nVO, pfPara, pnLine, bAnyScalability);
		// END: Complexity Estimation syntax support

		// START: VOL Control Parameters
		if(iVersion>813)
		{
			readItem(rguiVolControlParameters, nVO, pfPara, pnLine, bAnyScalability);
			readItem(rguiChromaFormat, nVO, pfPara, pnLine, bAnyScalability);
			readItem(rguiLowDelay, nVO, pfPara, pnLine, bAnyScalability);
			readItem(rguiVBVParams, nVO, pfPara, pnLine, bAnyScalability);
			readItem(rguiBitRate, nVO, pfPara, pnLine, bAnyScalability);
			readItem(rguiVbvBufferSize, nVO, pfPara, pnLine, bAnyScalability);
			readItem(rguiVbvBufferOccupany, nVO, pfPara, pnLine, bAnyScalability);
		}
		else
		{
			for(iObj = 0; iObj<nVO; iObj++)
			{
				rguiVolControlParameters[BASE_LAYER][iObj] = 0;
				rguiVolControlParameters[ENHN_LAYER][iObj] = 0;
				rguiVBVParams[BASE_LAYER][iObj] = 0;
				rguiVBVParams[ENHN_LAYER][iObj] = 0;
			}

		}

		// END: VOL Control Parameters

		// Interlaced CHANGE
		// Sequence frame frequency (Hz).  Use double type because of 29.97Hz (etc)
		nextValidLine (pfPara, pnLine);
		for (iObj = 0; iObj < nVO; iObj++)	{
			if (fscanf (pfPara, "%lf", &(rgdFrameFrequency [BASE_LAYER] [iObj])) != 1)	{
				fprintf(stderr, "wrong parameter file format on line %d\n", *pnLine);
				exit (1);
			}
			assert(rgdFrameFrequency [BASE_LAYER] [iObj] > 0.0);
		}
		if (bAnyScalability)	{
			for (iObj = 0; iObj < nVO; iObj++)	{
				if (fscanf (pfPara, "%lf", &(rgdFrameFrequency [ENHN_LAYER] [iObj])) != 1)	{
					fprintf(stderr, "wrong parameter file format on line %d\n", *pnLine);
					exit (1);
				}
				assert(rgdFrameFrequency [ENHN_LAYER] [iObj] > 0.0);
			}
		}

		// top field first flag
		nextValidLine (pfPara, pnLine);
		for (iObj = 0; iObj < nVO; iObj++)	{
			if (fscanf (pfPara, "%d", &(rgbTopFieldFirst [BASE_LAYER] [iObj])) != 1)	{
				fprintf(stderr, "wrong parameter file format on line %d\n", *pnLine);
				exit (1);
			}
			assert (rgbTopFieldFirst [BASE_LAYER] [iObj] == 0 || 
					rgbTopFieldFirst [BASE_LAYER] [iObj] == 1);
		}
		if (bAnyScalability)	{
			for (iObj = 0; iObj < nVO; iObj++)	{
				if (fscanf (pfPara, "%d", &(rgbTopFieldFirst [ENHN_LAYER] [iObj])) != 1)	{
					fprintf(stderr, "wrong parameter file format on line %d\n", *pnLine);
					exit (1);
				}
				assert (rgbTopFieldFirst [ENHN_LAYER] [iObj] == 0 || 
						rgbTopFieldFirst [ENHN_LAYER] [iObj] == 1);
			}
		}

		// alternate scan flag
		nextValidLine (pfPara, pnLine);
		for (iObj = 0; iObj < nVO; iObj++)	{
			if (fscanf (pfPara, "%d", &(rgbAlternateScan [BASE_LAYER] [iObj])) != 1)	{
				fprintf(stderr, "wrong parameter file format on line %d\n", *pnLine);
				exit (1);
			}
			assert (rgbAlternateScan [BASE_LAYER] [iObj] == 0 || 
					rgbAlternateScan [BASE_LAYER] [iObj] == 1);
		}
		if (bAnyScalability)	{
			for (iObj = 0; iObj < nVO; iObj++)	{
				if (fscanf (pfPara, "%d", &(rgbAlternateScan [ENHN_LAYER] [iObj])) != 1)	{
					fprintf(stderr, "wrong parameter file format on line %d\n", *pnLine);
					exit (1);
				}
				assert (rgbAlternateScan [ENHN_LAYER] [iObj] == 0 || 
						rgbAlternateScan [ENHN_LAYER] [iObj] == 1);
			}
		}

		// Direct Mode search radius (half luma pixel units)
		nextValidLine (pfPara, pnLine);
		for (iObj = 0; iObj < nVO; iObj++)	{
			if (fscanf (pfPara, "%d", &(rgiDirectModeRadius [BASE_LAYER] [iObj])) != 1)	{
				fprintf(stderr, "wrong parameter file format on line %d\n", *pnLine);
				exit (1);
			}
		}
		if (bAnyScalability)	{
			for (iObj = 0; iObj < nVO; iObj++)	{
				if (fscanf (pfPara, "%d", &(rgiDirectModeRadius [ENHN_LAYER] [iObj])) != 1)	{
					fprintf(stderr, "wrong parameter file format on line %d\n", *pnLine);
					exit (1);
				}
			}
		}

		// motion vector file
		nextValidLine(pfPara, pnLine);
		for (iObj = 0; iObj < nVO; iObj++)	{
			if (fscanf (pfPara, "%d", &(rgiMVFileUsage [BASE_LAYER] [iObj])) != 1)	{
				fprintf(stderr, "wrong parameter file format on line %d\n", *pnLine);
				exit (1);
			}
			assert (rgiMVFileUsage [BASE_LAYER] [iObj] == 0 ||	// 0= not used
					rgiMVFileUsage [BASE_LAYER] [iObj] == 1 ||	// 1= read motion vectors from file
					rgiMVFileUsage [BASE_LAYER] [iObj] == 2 );	// 2= write motion vectors to file
			
			pchMVFileName [BASE_LAYER] [iObj] = NULL;
			pchMVFileName [ENHN_LAYER] [iObj] = NULL;
			
			char cFileName[256];
			cFileName[0] = 0;
			if ((rgiMVFileUsage [BASE_LAYER] [iObj] != 0) &&
				(fscanf (pfPara, "%200s", cFileName) != 1))	{
				fprintf(stderr, "wrong parameter file format on line %d\n", *pnLine);
				exit (1);
			}
			Int iLength = strlen(cFileName) + 1;
			pchMVFileName [BASE_LAYER] [iObj] = new char [iLength];
			memcpy(pchMVFileName [BASE_LAYER] [iObj], cFileName, iLength);
		}
		if (bAnyScalability) {
			for (iObj = 0; iObj < nVO; iObj++)	{
				if (fscanf (pfPara, "%d", &(rgiMVFileUsage [ENHN_LAYER] [iObj])) != 1)	{
					fprintf(stderr, "wrong parameter file format on line %d\n", *pnLine);
					exit (1);
				}
				assert (rgiMVFileUsage [ENHN_LAYER] [iObj] == 0 || 
						rgiMVFileUsage [ENHN_LAYER] [iObj] == 1 ||
						rgiMVFileUsage [ENHN_LAYER] [iObj] == 2 );

				char cFileName[256];
				cFileName[0] = 0;
				if ((rgiMVFileUsage [ENHN_LAYER] [iObj] != 0) &&
					(fscanf (pfPara, "%s", cFileName) != 1))	{
					fprintf(stderr, "wrong parameter file format on line %d\n", *pnLine);
					exit (1);
				}
				Int iLength = strlen(cFileName) + 1;
				pchMVFileName [ENHN_LAYER] [iObj] = new char [iLength];
				memcpy(pchMVFileName [ENHN_LAYER] [iObj], cFileName, iLength);
			}
		}
		
		// file information		
		pchPrefix = new char [256];
		nextValidLine (pfPara, pnLine);
		fscanf (pfPara, "%s", pchPrefix);

		// original file directory
		pchBmpDir = new char [256];
		nextValidLine (pfPara, pnLine);
		fscanf (pfPara, "%100s", pchBmpDir);

		// chrominance format
		nextValidLine (pfPara, pnLine);
		for (iObj = 0; iObj < nVO; iObj++)	{
			UInt uiChrType;
			if (fscanf (pfPara, "%d", &uiChrType) != 1)	{
				fprintf(stderr, "wrong parameter file format on line %d\n", *pnLine);
				exit (1);
			}
			else {
				assert (uiChrType == 0 || uiChrType == 1 || uiChrType == 2);
				rgfChrType [iObj] = (ChromType) uiChrType;
			}
		}

		// output file directory
		pchOutBmpDir = new char [256];
		nextValidLine (pfPara, pnLine);
		fscanf (pfPara, "%100s", pchOutBmpDir);

		// output bitstream file
		pchOutStrFile = new char [256];
		nextValidLine (pfPara, pnLine);
		fscanf (pfPara, "%100s", pchOutStrFile);

		// statistics dumping options
		nextValidLine (pfPara, pnLine);
		for (iObj = 0; iObj < nVO; iObj++)	{
			if (fscanf (pfPara, "%d", &rgbDumpMB [iObj]) != 1)	{
				fprintf(stderr, "wrong parameter file format on line %d\n", *pnLine);
				exit (1);
			}
			assert (rgbDumpMB [iObj] == 0 || rgbDumpMB [iObj] == 1);
		}

		// trace file options
		nextValidLine (pfPara, pnLine);
		for (iObj = 0; iObj < nVO; iObj++)	{
			if (fscanf (pfPara, "%d", &rgbTrace [iObj]) != 1)	{
				fprintf(stderr, "wrong parameter file format on line %d\n", *pnLine);
				exit (1);
			}
			assert (rgbTrace [iObj] == 0 || rgbTrace [iObj] == 1);
		}

		// sprite info
		// sprite usage
		nextValidLine (pfPara, pnLine);
		for (iObj = 0; iObj < nVO; iObj++)	{
			if (fscanf (pfPara, "%d", &rguiSpriteUsage [iObj]) != 1)	{
				fprintf(stderr, "wrong parameter file format on line %d\n", *pnLine);
				exit (1);
			}
			assert ((rguiSpriteUsage [iObj] == 0) || (rguiSpriteUsage [iObj] == 1)); // only support static sprite at the moment
			if (rgbScalability [iObj] == TRUE)
				assert (rguiSpriteUsage [iObj] == 0);
		}

		// warping accuracy
		nextValidLine (pfPara, pnLine);
		for (iObj = 0; iObj < nVO; iObj++)	{
			if (fscanf (pfPara, "%d", &rguiWarpingAccuracy [iObj]) != 1)	{
				fprintf(stderr, "wrong parameter file format on line %d\n", *pnLine);
				exit (1);
			}
			if (rguiSpriteUsage [iObj] != 0)
				assert (rguiWarpingAccuracy [iObj] == 0 || rguiWarpingAccuracy [iObj] == 1 || rguiWarpingAccuracy [iObj] == 2 || rguiWarpingAccuracy [iObj] == 3);
		}

		// number of points
		nextValidLine (pfPara, pnLine);
		for (iObj = 0; iObj < nVO; iObj++)	{
			if (fscanf (pfPara, "%d", &rgiNumPnts [iObj]) != 1)	{
				fprintf(stderr, "wrong parameter file format on line %d\n", *pnLine);
				exit (1);
			}
			if (rgbScalability [iObj] == TRUE)
				assert (rgiNumPnts [iObj] == -1);
			if (rguiSpriteUsage [iObj] == 1) 
				assert (rgiNumPnts [iObj] == 0 ||
						rgiNumPnts [iObj] == 1 ||
						rgiNumPnts [iObj] == 2 ||
						rgiNumPnts [iObj] == 3 ||
						rgiNumPnts [iObj] == 4);
		}

		// sprite directory
		pchSptDir = new char [256];
		nextValidLine (pfPara, pnLine);
		fscanf (pfPara, "%100s", pchSptDir);

		// point directory
		pchSptPntDir = new char [256];
		nextValidLine (pfPara, pnLine);
		fscanf (pfPara, "%100s", pchSptPntDir);

		//  sprite reconstruction mode, i.e. Low-latency-sprite-enable
		nextValidLine (pfPara, pnLine);
		for (iObj = 0; iObj < nVO; iObj++)
		{
			UInt uiSptMode;
			if (fscanf (pfPara, "%d", &uiSptMode) != 1)	{
				fprintf(stderr, "wrong parameter file format on line %d\n", *pnLine);
				exit (1);
			}
			else {
				assert (uiSptMode == 0 || uiSptMode == 1 );
				rgSpriteMode[iObj] = (SptMode) uiSptMode;
			}
		}

		fclose(pfPara);	
	}

	CSessionEncoder* penc = new CSessionEncoder (
			// general info
			uiFrmWidth, // frame width
			uiFrmHeight, // frame height
			firstFrm, // first frame number
			lastFrm, // last frame number
			bNot8Bit, // NBIT: not 8-bit flag
			uiQuantPrecision, // NBIT: quant precision
			nBits, // NBIT: number of bits per pixel
			firstVO, // first VO index
			lastVO, // last VO index
			rgbSpatialScalability, // spatial scalability indicator
			rgiTemporalScalabilityType, // form of temporal scalability // added by Sharp (98/02/09)
			rgiEnhancementType,	// enhancement_type for scalability // added by Sharp (98/02/09)

			rguiRateControl,
			rguiBitsBudget,
			rgfAlphaUsage, // alpha usage for each VO.
			rgbShapeOnly,
			rgiBinaryAlphaTH,
			rgiBinaryAlphaRR, //	Added for error resilient mode by Toshiba(1997-11-14)
			rgbNoCrChange, 

			// motion estimation part for each VO
			rguiSearchRange,
			rgbOriginalME, // flag indicating whether use the original previous VO for ME
			rgbAdvPredDisable,

            // START: Complexity Estimation syntax support - Marc Mongenet (EPFL) - 17 Jun 1998
			rgbComplexityEstimationDisable,
			rgbOpaque,
			rgbTransparent,
			rgbIntraCAE,
			rgbInterCAE,
			rgbNoUpdate,
			rgbUpsampling,
			rgbIntraBlocks,
			rgbInterBlocks,
			rgbInter4vBlocks,
			rgbNotCodedBlocks,
			rgbDCTCoefs,
			rgbDCTLines,
			rgbVLCSymbols,
			rgbVLCBits,
			rgbAPM,
			rgbNPM,
			rgbInterpolateMCQ,
			rgbForwBackMCQ,
			rgbHalfpel2,
			rgbHalfpel4,
			// END: Complexity Estimation syntax support
		
			// START: VOL Control Parameters
			rguiVolControlParameters,
			rguiChromaFormat,
			rguiLowDelay,
			rguiVBVParams,
			rguiBitRate,
			rguiVbvBufferSize,
			rguiVbvBufferOccupany,
			// END: VOL Control Parameters

			rgdFrameFrequency,

			// interlace coding flag
			rgbInterlacedCoding,
			rgbTopFieldFirst,
            rgbAlternateScan,

			// direct mode search radius
			rgiDirectModeRadius,

			// motion vector file I/O
			rgiMVFileUsage,
			&pchMVFileName[0],

			// systax mode
			rgiVPBitTh,	// Bit threshold for video packet spacing control
			rgbDataPartitioning,  //data partitioning
			rgbReversibleVlc, //reversible VLC

			// for texture coding
			rgfQuant,
			rgbLoadIntraMatrix,
			rgppiIntraQuantizerMatrix,
			rgbLoadInterMatrix,
			rgppiInterQuantizerMatrix,
			rgiIntraDCSwitchingThr,		//threshold to code dc as ac when pred. is on
			rgiIStep,					// I-VOP quantization stepsize
			rgiPStep,					// P-VOP quantization stepsize
			rgbLoadIntraMatrixAlpha,
			rgppiIntraQuantizerMatrixAlpha,
			rgbLoadInterMatrixAlpha,
			rgppiInterQuantizerMatrixAlpha,
			rgiIStepAlpha,				// I-VOP gray alpha quantization stepsize
			rgiPStepAlpha,				// P-VOP gray alpha quantization stepsize
			rgiBStepAlpha,				// B-VOP gray alpha quantization stepsize
			rgbNoGrayQuantUpdate,		//disable gray quant update within VOP
			rgiStepBCode,				// code for quantization stepsize for B-VOP
			rgiNumBbetweenPVOP,			// no of B-VOPs between P-VOPs
			rgiNumPbetweenIVOP,			// no of P-VOPs between I-VOPs
			rgiGOVperiod,
			rgbDeblockFilterDisable,	//disable deblocing filter
			rgbAllowSkippedPMBs,

			// file information
			pchPrefix, // prefix name of the movie
			pchBmpDir, // bmp file directory location
			rgfChrType,
			pchOutBmpDir, // quantized frame file directory
			pchOutStrFile, // output bitstream file
			rgiTSRate,
			rgiEnhcTSRate, // frame rate for enhancement layer // added by Sharp (98/02/09)

			rgbDumpMB,
			rgbTrace,
			rgbRoundingControlDisable,
			rgiInitialRoundingType,
			rguiSpriteUsage,
			rguiWarpingAccuracy,
			rgiNumPnts,
			pchSptDir,
			pchSptPntDir,
            rgSpriteMode,
			iSpatialOption,
			uiFrmWidth_SS,
			uiFrmHeight_SS,
			uiHor_sampling_n,
			uiHor_sampling_m,
			uiVer_sampling_n,
			uiVer_sampling_m
		);

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

		delete [] rguiVolControlParameters [iLayer];
		delete [] rguiChromaFormat [iLayer];
		delete [] rguiLowDelay [iLayer];
		delete [] rguiVBVParams [iLayer];
		delete [] rguiBitRate [iLayer];
		delete [] rguiVbvBufferSize [iLayer];
		delete [] rguiVbvBufferOccupany [iLayer];
	}

	delete [] rgbScalability;
    delete [] rgbSpatialScalability;
	delete [] rgiTemporalScalabilityType;
	delete [] rgiEnhancementType;

	// for mask coding, should fill in later on
	delete [] rgfAlphaUsage; // alpha usage for each VO.  0: binary, 1: 8-bit
	delete [] rgbShapeOnly;
	delete [] rgiBinaryAlphaTH;
	delete [] rgiBinaryAlphaRR;	//	Added for error resilient mode by Toshiba(1997-11-14)
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

Void readVTCParam(CVTCEncoder *pvtcenc, FILE *pfVTCPara, UInt* pnLine)	{

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
	UInt uiFrmWidth;
	nextValidLine (pfVTCPara, pnLine);
	if ( fscanf (pfVTCPara, "%d", &uiFrmWidth) != 1)	{
		fprintf(stderr, "wrong parameter on line %d\n", *pnLine);
		assert (FALSE);
	}

	// Frame height
	UInt uiFrmHeight;			// frame height
	nextValidLine (pfVTCPara, pnLine);
	if ( fscanf (pfVTCPara, "%d", &uiFrmHeight) != 1)	{
		fprintf(stderr, "wrong parameter on line %d\n", *pnLine);
		assert (FALSE);
	}

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
       for (int spa_lev=0; spa_lev<(int)uiWvtDecmpLev; spa_lev++) {
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
	       for (int spa_lev=0; spa_lev<(int)uiSpatialLev-1; ++spa_lev)   
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
		for (int i=0; i<3; i++)
			Qinfo[i] = new SNR_PARAM[uiSpatialLev];
		for (uispa_lev=0; uispa_lev<uiSpatialLev; uispa_lev++) {
			for (int col=0; col<3; col++) {
				Qinfo[col][uispa_lev].SNR_scalability_levels = (int)1;
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

			for (int col=0; col<3; col++) {
				Qinfo[col][uispa_lev].SNR_scalability_levels = (int)uiSNRLev;
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
		 cImagePath,
		 uiAlphaChannel,
		 cSegImagePath,
		 uiAlphaTh,
		 uiChangeCRDisable,
		 cOutBitsFile,
		 uiColors,
		 uiFrmWidth,
		 uiFrmHeight,
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
		 uiQdcY,
		 uiQdcUV, 
		 uiSpatialLev,
		 defaultSpatialScale, // hjlee 0901
	     lastWvtDecompInSpaLayer, // hjlee 0901
		 Qinfo  
		);
}

///// WAVELET VTC: end ///////////////////////////////

Void readBoolVOLFlag (Bool * rgbTable [2], UInt nVO, FILE * pfCfg, UInt * pnLine, Bool bAnyScalability)
{
	nextValidLine (pfCfg, pnLine);
	for (UInt i = 0; i < nVO; ++i) {
		if (fscanf (pfCfg, "%d", & rgbTable [BASE_LAYER] [i]) != 1) {
			fprintf(stderr, "wrong parameter file format on line %d\n", *pnLine);
			exit (1);
		}
		assert (rgbTable [BASE_LAYER] [i] == 0 || rgbTable [BASE_LAYER] [i] == 1);
	}
	if (bAnyScalability) {
		for (UInt i = 0; i < nVO; ++i)	{
			if (fscanf (pfCfg, "%d", & rgbTable [ENHN_LAYER] [i]) != 1)	{
				fprintf(stderr, "wrong parameter file format on line %d\n", *pnLine);
				exit (1);
			}
			assert (rgbTable [ENHN_LAYER] [i] == 0 || rgbTable [ENHN_LAYER] [i] == 1);
		}
	}
}

Void readItem(UInt *rguiTable [2], UInt nVO, FILE * pfCfg, UInt * pnLine, Bool bAnyScalability)
{
	nextValidLine (pfCfg, pnLine);
	for (UInt i = 0; i < nVO; ++i) {
		if (fscanf (pfCfg, "%d", & rguiTable [BASE_LAYER] [i]) != 1) {
			fprintf(stderr, "wrong parameter file format on line %d\n", *pnLine);
			exit (1);
		}
	}
	if (bAnyScalability) 
	{
		for (UInt i = 0; i < nVO; ++i)	{
			if (fscanf (pfCfg, "%d", & rguiTable [ENHN_LAYER] [i]) != 1)	{
				fprintf(stderr, "wrong parameter file format on line %d\n", *pnLine);
				exit (1);
			}
		}
	}
}

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
	readVTCParam(&vtcenc, pfVTCPara, pnLine);

	fclose(pfVTCPara);

	// start VTC encoding
	int targetSpatialLev = vtcenc.mzte_codec.m_iTargetSpatialLev;
	int targetSNRLev = vtcenc.mzte_codec.m_iTargetSNRLev;

	vtcenc.encode();

	// start VTC decoding 
	CVTCDecoder vtcdec;
	vtcdec.decode(vtcenc.m_cOutBitsFile, "encRec.yuv", 
					targetSpatialLev, targetSNRLev );
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

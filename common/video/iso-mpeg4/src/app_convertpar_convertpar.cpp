/*************************************************************************

This software module was originally developed by 

	Simon Winder (swinder@microsoft.com), Microsoft Corporation


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


*************************************************************************/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef unsigned int UInt;
typedef int Int;
typedef void Void;
typedef int Bool;
typedef double Double;
typedef enum {Q_H263, Q_MPEG} Quantizer; 
typedef enum AlphaUsage {RECTANGLE, ONE_BIT, EIGHT_BIT} AlphaUsage;
typedef enum ChromType {FOUR_FOUR_FOUR, FOUR_TWO_TWO, FOUR_TWO_ZERO} ChromType;
typedef enum {BASIC_SPRITE, LOW_LATENCY, PIECE_OBJECT, PIECE_UPDATE} SptMode;
typedef char Char;

typedef struct {
	Int iOnOff;
	Int iCycle;
} RRVmodeStr;

Void nextValidLine (FILE *pfPara, UInt* pnLine);
Void readBoolVOLFlag (Bool * rgbTable [2], UInt nVO, FILE * pfCfg, UInt * pnLine, Bool bAnyScalability);
Void readItem(UInt *rguiTable [2], UInt nVO, FILE * pfCfg, UInt * pnLine, Bool bAnyScalability);

#define BASE_LAYER 0
#define ENHN_LAYER 1
#define NO_SCALABILITY		 0
#define TEMPORAL_SCALABILITY 1
#define SPATIAL_SCALABILITY  2
#define FALSE 0
#define TRUE 1
#define BLOCK_SQUARE_SIZE    64
#define RC_MPEG4			1
#define RC_TM5				3

Void fatal_error(char *pchError, Bool bFlag = FALSE);

void my_assert(int iFlag)
{
	if(!iFlag)
		fatal_error("Some assert failed! Check original par file format.\nSorry to be non-specific, but monkeys wrote this section.\n");
}

int main (Int argc, Char* argv[])
{
	UInt nLine = 1;
	UInt* pnLine = &nLine;
	FILE *pfPara;
	FILE *pfOut = stdout;

	if (argc != 2 && argc !=3)
	{
		fprintf (stderr,"Usage: %s old_par_file [new_par_file]\n", argv[0]);
		fatal_error("Conversion aborted");
	}
	
	if ((pfPara = fopen (argv[1], "r")) == NULL )
	{
		fprintf (stderr,"Source parameter file not found\n");
		fatal_error("Conversion aborted");
	}

	if(argc==3)
	{
		if ((pfOut = fopen (argv[2], "w")) == NULL )
		{
			fprintf (stderr,"Could not open %s for writing.\n", argv[2]);
			fatal_error("Conversion aborted");
		}
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
	UInt nVO;
	UInt uiFrmWidth_SS,uiFrmHeight_SS;
	UInt uiHor_sampling_m,uiHor_sampling_n;
	UInt uiVer_sampling_m,uiVer_sampling_n;
	Bool bAnyScalability;
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
	
	// version 2
	UInt *rguiVerID;
	UInt uiUse_ref_shape, uiUse_ref_texture;
	UInt uiHor_sampling_m_shape, uiHor_sampling_n_shape;
	UInt uiVer_sampling_m_shape, uiVer_sampling_n_shape;

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

	// version 2
	Bool *rgbNewpredEnable [2];
	Bool *rgbNewpredSegmentType [2];
	char **rgcNewpredRefName [2];
	char **rgcNewpredSlicePoint [2];
	Bool *rgbSadctDisable [2];
	Bool *rgbQuarterSample [2];
	RRVmodeStr *RRVmode[2];	

	UInt iObj;

	// verify version number
	nextValidLine (pfPara, pnLine);
	if ( fscanf (pfPara, "%u", &iVersion) != 1 )	{
		fprintf(stderr, "wrong parameter file format on line %d\n", *pnLine);
		fatal_error("Conversion aborted");
	}

	if (iVersion < 812 || iVersion > 818)
	{
		// version 812 basic
		// version 813 adds rounding control flags
		// version 814 adds VOL control parameters
		// version 815 adds skipped mb enable
		// in v2 software version 815 adds Version ID Flag (GMC) - someone screwed up here!
		// version 816 adds RRV related parameters
		// version 817 adds SADCT disable flag
		// version 818 adds OBSS related parameters

		fprintf(stderr, "Unknown parameter file version\n");

		fatal_error("Conversion aborted");
	}

	if(iVersion==815)
	{
		fprintf(stderr, "The version 815 parameter file format is not self-consistent due to\n");
		fprintf(stderr, "mistakes by integrators made during the version 1 to version 2 handover.\n");
		fprintf(stderr, "This program assumes you are using the version 1 parameter file.\n");
		fprintf(stderr, "Please upgrade to version 816 by adding the RRV parameters if you\n");
		fprintf(stderr, "want version 2.\n");
	}

//	if(iVersion>815)
//	{
//		fprintf(stderr, "At present, conversion of version 2 (>815) parameter files is not supported.\n");
//		exit(1);
//	}

	/*************/
	fprintf(pfOut,"!!!MS!!!\n\n// This is the new parameter file format.  The parameters in this file can be\n// specified in any order.\n");
	fprintf(pfOut,"\nVersion = 902\n\n");
	/*************/

///// WAVELET VTC: begin ///////////////////////////////
	// sarnoff: wavelet visual texture coding 
	nextValidLine (pfPara, pnLine);
	if ( fscanf (pfPara, "%d", &iVTCFlag) != 1)	{
		fprintf(stderr, "wrong parameter VTC flag on line %d\n", *pnLine);
		fatal_error("Conversion aborted");
	}
	fatal_error("iVTCFlag must be 0 or 1", iVTCFlag==0 || iVTCFlag==1);

	// read VTC control file

	char VTCCtlFile[80];
	nextValidLine (pfPara, pnLine);
	if ( fscanf (pfPara, "%s", VTCCtlFile) != 1)	{
		fprintf(stderr, "wrong parameter VTC flag on line %d\n", *pnLine);
		fatal_error("Conversion aborted");
	}

	/*************/
	fprintf(pfOut, "VTC.Enable = %d\nVTC.Filename = \"%s\"\n", iVTCFlag, VTCCtlFile);
	/*************/

	if (iVTCFlag==1) {
		fclose(pfPara);
		if(argc==3)
			fclose(pfOut);

		return 0;
	}

///// WAVELET VTC: end ///////////////////////////////


	
	// frame size code

	nextValidLine (pfPara, pnLine);
	if (fscanf (pfPara, "%u", &uiFrmWidth) != 1)	{
		fprintf(stderr, "wrong parameter file format on line %d\n", *pnLine);
		fatal_error("Conversion aborted");
	}
	nextValidLine (pfPara, pnLine);
	if (fscanf (pfPara, "%u", &uiFrmHeight) != 1)	{
		fprintf(stderr, "wrong parameter file format on line %d\n", *pnLine);
		fatal_error("Conversion aborted");
	}

	// first and last frame number
	nextValidLine (pfPara, pnLine);
	if ( fscanf (pfPara, "%u", &firstFrm) != 1) {
		fprintf(stderr, "wrong parameter file format on line %d\n", *pnLine);
		fatal_error("Conversion aborted");
	}
	nextValidLine (pfPara, pnLine);
	if ( fscanf (pfPara, "%u", &lastFrm) != 1) {
		fprintf(stderr, "wrong parameter file format on line %d\n", *pnLine);
		fatal_error("Conversion aborted");
	}
	my_assert (lastFrm >= firstFrm);

	/*************/
	fprintf(pfOut, "\nSource.Width = %d\nSource.Height = %d\nSource.FirstFrame = %d\nSource.LastFrame = %d\n",
		uiFrmWidth, uiFrmHeight, firstFrm, lastFrm);
	/*************/

	// NBIT: not 8-bit flag
	nextValidLine (pfPara, pnLine);
	if ( fscanf (pfPara, "%d", &bNot8Bit) != 1)	{
		fprintf(stderr, "wrong parameter file format on line %d\n", *pnLine);
		fatal_error("Conversion aborted");
	}
	my_assert (bNot8Bit==0 || bNot8Bit==1);

	// NBIT: quant precision
	nextValidLine (pfPara, pnLine);
	if ( fscanf (pfPara, "%d", &uiQuantPrecision) != 1)	{
		fprintf(stderr, "wrong parameter file format on line %d\n", *pnLine);
		fatal_error("Conversion aborted");
	}
	if (bNot8Bit==0) {
		uiQuantPrecision = 5;
	}

	// NBIT: number of bits per pixel
	nextValidLine (pfPara, pnLine);
	if ( fscanf (pfPara, "%d", &nBits) != 1)	{
		fprintf(stderr, "wrong parameter file format on line %d\n", *pnLine);
		fatal_error("Conversion aborted");
	}
	my_assert (nBits>=4 && nBits<=12);
	if (bNot8Bit==0) {
		nBits = 8;
	}
	
	/*************/
	fprintf(pfOut, "Source.BitsPerPel = %d\nNot8Bit.QuantPrecision = %d\nNot8Bit.Enable = %d\n",
		nBits, uiQuantPrecision, bNot8Bit);
	/*************/

	// object indexes
	nextValidLine (pfPara, pnLine);
	if ( fscanf (pfPara, "%u", &firstVO) != 1)	{
		fprintf(stderr, "wrong parameter file format on line %d\n", *pnLine);
		fatal_error("Conversion aborted");
	}
	nextValidLine (pfPara, pnLine);
	if ( fscanf (pfPara, "%u", &lastVO) != 1)	{
		fprintf(stderr, "wrong parameter file format on line %d\n", *pnLine);
		fatal_error("Conversion aborted");
	}
	my_assert (lastVO >= firstVO);
	nVO = lastVO - firstVO + 1;

	/*************/
	fprintf(pfOut, "Source.ObjectIndex.First = %d\nSource.ObjectIndex.Last = %d\n",
		firstVO, lastVO);
	/*************/

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
	
	// version 2
	rguiVerID = new UInt [nVO];

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

		// version 2
		rgbNewpredEnable [iL] = new Bool [nVO];
		rgbNewpredSegmentType [iL] = new Bool [nVO];
		rgcNewpredRefName [iL] = new char *[nVO];
		rgcNewpredSlicePoint [iL] = new char *[nVO];
		rgbSadctDisable [iL] = new Bool [nVO];
		rgbQuarterSample [iL] = new Bool [nVO];
		RRVmode[iL]	= new RRVmodeStr [nVO];
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


	// Video Version ID
	if(iVersion > 815) // do not include in parameter file version 815
	{       // GMC_V2
		nextValidLine (pfPara, pnLine);
		for (iObj = 0; iObj < nVO; iObj++)	{
			if (fscanf (pfPara, "%d", &rguiVerID [iObj]) != 1)	{
				fprintf	(stderr, "wrong parameter file format on line %d\n", *pnLine);
				fatal_error("Conversion aborted");
			}
			fatal_error("Bad value for version ID", rguiVerID [iObj] == 1 || rguiVerID [iObj] == 2);
			
			/*************/
			fprintf(pfOut, "VersionID [%d] = %d\n", iObj, rguiVerID [iObj]);
			/*************/
		}
	}
	else
	{
		// default to version 1
		for (iObj = 0; iObj < nVO; iObj++)
			/*************/
			fprintf(pfOut, "VersionID [%d] = 1\n", iObj);
			/*************/
	}

	//scalability indicators: 1 = temporal, 2 = spatial scalability


	nextValidLine (pfPara, pnLine);
	bAnyScalability = FALSE;

	for (iObj = 0; iObj < nVO; iObj++)	{
		if (fscanf (pfPara, "%d", &rgbScalability [iObj]) != 1)	{
			fprintf(stderr, "wrong parameter file format on line %d\n", *pnLine);
			fatal_error("Conversion aborted");
		}
		if (rgbScalability[iObj] == TEMPORAL_SCALABILITY || 
			rgbScalability[iObj] == SPATIAL_SCALABILITY)
			bAnyScalability = TRUE;
		else
			my_assert (rgbScalability[iObj] == NO_SCALABILITY);
		if(rgbScalability[iObj] == SPATIAL_SCALABILITY || rgbScalability[iObj] == TEMPORAL_SCALABILITY) // modifiedy by Sharp(98/2/12)
			rgbSpatialScalability[iObj] = TRUE; 
		else 
			rgbSpatialScalability[iObj] = FALSE;

		/*************/
		fprintf(pfOut, "Scalability [%d] = \"%s\"\n", iObj,
			rgbScalability[iObj] == TEMPORAL_SCALABILITY ? "Temporal" : (rgbScalability[iObj] == SPATIAL_SCALABILITY ? "Spatial" : "None"));
		/*************/
	}

	//coded added by Sony, only deals with ONE VO.
	//Type option of Spatial Scalable Coding
	//This parameter is used for dicision VOP prediction types of Enhancement layer in Spatial Scalable Coding
	//If this option is set to 0, Enhancement layer is coded as "PPPPPP......",
	//else if set to 1 ,It's coded as "PBBBB......."
	nextValidLine(pfPara,pnLine);
	my_assert (nVO == 1);
	fscanf(pfPara,"%d",&iSpatialOption);
	if(rgbScalability[0] == SPATIAL_SCALABILITY)
		if (iSpatialOption == 1)
				fprintf(stdout,"Enhancement layer is coded as \"PPPPP.....\"\n");
		else if (iSpatialOption == 0)
				fprintf(stdout,"Enhancement layer is coded as \"PBBBB.....\"\n");
		else {
			fprintf(stderr,"The parameter \"SpatialOption\" is not set correctly\n");
			fatal_error("Conversion aborted");
		}

	/*************/
	fprintf(pfOut, "Scalability.Spatial.PredictionType [0] = \"%s\"\n", iSpatialOption==0 ? "PBB" : "PPP");
	/*************/

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

	/*************/
	fprintf(pfOut, "Scalability.Spatial.Width [0] = %d\n", uiFrmWidth_SS);
	fprintf(pfOut, "Scalability.Spatial.Height [0] = %d\n", uiFrmHeight_SS);
	fprintf(pfOut, "Scalability.Spatial.HorizFactor.N [0] = %d\n", uiHor_sampling_n);
	fprintf(pfOut, "Scalability.Spatial.HorizFactor.M [0] = %d\n", uiHor_sampling_m);
	fprintf(pfOut, "Scalability.Spatial.VertFactor.N [0] = %d\n", uiVer_sampling_n);
	fprintf(pfOut, "Scalability.Spatial.VertFactor.M [0] = %d\n", uiVer_sampling_m);
	/*************/

	if(iVersion>=818)
	{
		//OBSS_SAIT_991015
		//load reference status
		nextValidLine(pfPara,pnLine);
		fscanf(pfPara,"%d",&uiUse_ref_shape);
		fscanf(pfPara,"%d",&uiUse_ref_texture);

		//load upsampling factor for shape
		nextValidLine(pfPara,pnLine);
		fscanf(pfPara,"%d",&uiHor_sampling_n_shape);
		fscanf(pfPara,"%d",&uiHor_sampling_m_shape);

		nextValidLine(pfPara,pnLine);
		fscanf(pfPara,"%d",&uiVer_sampling_n_shape);
		fscanf(pfPara,"%d",&uiVer_sampling_m_shape);
		//~OBSS_SAIT_991015
	}
	else
	{
		uiUse_ref_shape = 0;
		uiUse_ref_texture = 0;
		uiHor_sampling_n_shape = 2;
		uiHor_sampling_m_shape = 1;
		uiVer_sampling_n_shape = 2;
		uiVer_sampling_m_shape = 1;
	}


	/*************/
	fprintf(pfOut, "Scalability.Spatial.UseRefShape.Enable [0] = %d\n", uiUse_ref_shape);
	fprintf(pfOut, "Scalability.Spatial.UseRefTexture.Enable [0] = %d\n", uiUse_ref_texture);
	fprintf(pfOut, "Scalability.Spatial.Shape.HorizFactor.N [0] = %d\n", uiHor_sampling_n_shape);
	fprintf(pfOut, "Scalability.Spatial.Shape.HorizFactor.M [0] = %d\n", uiHor_sampling_m_shape);
	fprintf(pfOut, "Scalability.Spatial.Shape.VertFactor.N [0] = %d\n", uiVer_sampling_n_shape);
	fprintf(pfOut, "Scalability.Spatial.Shape.VertFactor.M [0] = %d\n", uiVer_sampling_m_shape);
	/*************/

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
			fatal_error("Conversion aborted");
		}
		my_assert (rgiTemporalScalabilityType [iObj] == 0 || 
			rgiTemporalScalabilityType [iObj] == 1 || 
			rgiTemporalScalabilityType [iObj] == 2 ||
			rgiTemporalScalabilityType [iObj] == 3 ||
			rgiTemporalScalabilityType [iObj] == 4);
		/*************/
		fprintf(pfOut, "Scalability.Temporal.PredictionType [%d] = %d\n", iObj, rgiTemporalScalabilityType [iObj]);
		/*************/
	}

	// enhancement_type for scalability
	nextValidLine (pfPara, pnLine);
	for (iObj = 0; iObj < nVO; iObj++)	{
		if (fscanf (pfPara, "%d", &rgiEnhancementType [iObj]) != 1)	{
			fprintf(stderr, "wrong parameter file format on line %d\n", *pnLine);
			fatal_error("Conversion aborted");
		}
		my_assert (rgiEnhancementType [iObj] == 0 || //  entire region of the base layer is enhanced
// begin: modified by Sharp (98/3/24)
			rgiEnhancementType [iObj] == 1 ||  // partial region of the base layer is enhanced (with background composition)
			rgiEnhancementType [iObj] == 2);  // partial region of the base layer is enhanced (without background composition)
// end: modified by Sharp (98/3/24)
		/*************/
		fprintf(pfOut, "Scalability.Temporal.EnhancementType [%d] = \"%s\"\n", iObj,
			rgiEnhancementType [iObj] == 0 ? "Full" : (rgiEnhancementType [iObj] == 1 ? "PartC" : "PartNC"));
		/*************/
	}


	if(iVersion>815)
	{
	// NEWPRED
		// NewpredEnable
		nextValidLine (pfPara, pnLine);

		for (iObj = 0; iObj < nVO; iObj++)	{
			if (fscanf (pfPara, "%d", &rgbNewpredEnable [BASE_LAYER] [iObj] ) != 1)	{
				fprintf(stderr, "wrong parameter file format on line %d\n", *pnLine);
				exit (1);
			}
			if(rguiVerID[iObj] == 1)
				my_assert (rgbNewpredEnable [BASE_LAYER] [iObj]  == 0);
			else if(rguiVerID[iObj] != 1)
				my_assert (rgbNewpredEnable [BASE_LAYER] [iObj]  == 0 || 
						rgbNewpredEnable [BASE_LAYER] [iObj]  == 1);
		}
		if (bAnyScalability)
		{

			for (iObj = 0; iObj < nVO; iObj++)	{
				if (fscanf (pfPara, "%d", &rgbNewpredEnable [ENHN_LAYER] [iObj]) != 1)	{
					fprintf(stderr, "wrong parameter file format on line %d\n", *pnLine);
					exit (1);
				}
				rgbNewpredEnable [ENHN_LAYER] [iObj] = FALSE;
			}
		}

		// NewpredSegmentType
		nextValidLine (pfPara, pnLine);

		for (iObj = 0; iObj < nVO; iObj++)	{
			if (fscanf (pfPara, "%d", &rgbNewpredSegmentType [BASE_LAYER] [iObj] ) != 1)	{
				fprintf(stderr, "wrong parameter file format on line %d\n", *pnLine);
				exit (1);
			}
			my_assert (rgbNewpredSegmentType [BASE_LAYER] [iObj]  == 0 || 
					rgbNewpredSegmentType [BASE_LAYER] [iObj]  == 1);
		}
		if (bAnyScalability)	{
			for (iObj = 0; iObj < nVO; iObj++)	{
				if (fscanf (pfPara, "%d", &rgbNewpredSegmentType [ENHN_LAYER] [iObj]) != 1)	{
					fprintf(stderr, "wrong parameter file format on line %d\n", *pnLine);
					exit (1);
				}
				rgbNewpredSegmentType [ENHN_LAYER] [iObj] = FALSE;
			}
		}

		// NewpredRefName
		nextValidLine (pfPara, pnLine);

		for (iObj = 0; iObj < nVO; iObj++)	{
			rgcNewpredRefName [BASE_LAYER] [iObj] = new char [128];
			if (fscanf (pfPara, "%s", rgcNewpredRefName [BASE_LAYER] [iObj] ) != 1)	{
				fprintf(stderr, "wrong parameter file format on line %d\n", *pnLine);
				exit (1);
			}
		}
		if (bAnyScalability)	{

			for (iObj = 0; iObj < nVO; iObj++)	{
				rgcNewpredRefName [ENHN_LAYER] [iObj] = new char [128];
				if (fscanf (pfPara, "%s", rgcNewpredRefName [ENHN_LAYER] [iObj]) != 1)	{
					fprintf(stderr, "wrong parameter file format on line %d\n", *pnLine);
					exit (1);
				}
			}
		}

		// NewpredSlicePoint
		nextValidLine (pfPara, pnLine);

		for (iObj = 0; iObj < nVO; iObj++)	{
			rgcNewpredSlicePoint [BASE_LAYER] [iObj] = new char [128];
			if (fscanf (pfPara, "%s", rgcNewpredSlicePoint [BASE_LAYER] [iObj]) != 1)	{
				fprintf(stderr, "wrong parameter file format on line %d\n", *pnLine);
				exit (1);
			}
			if (rgbNewpredSegmentType [BASE_LAYER] [iObj])
				strcpy(rgcNewpredSlicePoint [BASE_LAYER] [iObj], "0");
		}
		if (bAnyScalability)	{

			for (iObj = 0; iObj < nVO; iObj++)	{
				rgcNewpredSlicePoint [ENHN_LAYER] [iObj] = new char [128];
				if (fscanf (pfPara, "%s", rgcNewpredSlicePoint [ENHN_LAYER] [iObj]) != 1)	{
					fprintf(stderr, "wrong parameter file format on line %d\n", *pnLine);
					exit (1);
				}
			}
		}
	}
	else
	{
		for (iObj = 0; iObj < nVO; iObj++)	{
				// default values
				rgbNewpredEnable [BASE_LAYER] [iObj] = 0;
				rgbNewpredEnable [ENHN_LAYER] [iObj] = 0;
				rgbNewpredSegmentType [BASE_LAYER] [iObj] = 0;
				rgbNewpredSegmentType [ENHN_LAYER] [iObj] = 0;
				rgcNewpredRefName [BASE_LAYER] [iObj] = NULL;
				rgcNewpredRefName [ENHN_LAYER] [iObj] = NULL;
				rgcNewpredSlicePoint [BASE_LAYER] [iObj] = NULL;
				rgcNewpredSlicePoint [ENHN_LAYER] [iObj] = NULL;
		}
	}

	for (iObj = 0; iObj < nVO; iObj++)
	{
		/*************/
		fprintf(pfOut, "Newpred.Enable [%d] = %d\n", iObj, rgbNewpredEnable [BASE_LAYER] [iObj]);
		fprintf(pfOut, "Newpred.SegmentType [%d] = \"%s\"\n", iObj, rgbNewpredSegmentType [BASE_LAYER] [iObj]==0 ? "VideoPacket" : "VOP");
		fprintf(pfOut, "Newpred.Filename [%d] = \"%s\"\n", iObj, rgcNewpredRefName [BASE_LAYER] [iObj]==NULL ? "" : rgcNewpredRefName [BASE_LAYER] [iObj]);
		fprintf(pfOut, "Newpred.SliceList [%d] = \"%s\"\n", iObj, rgcNewpredSlicePoint [BASE_LAYER] [iObj]==NULL ? "" : rgcNewpredSlicePoint [BASE_LAYER] [iObj]);
		/*************/
		if(bAnyScalability)
		{
			/*************/
			fprintf(pfOut, "Newpred.Enable [%d] = %d\n", iObj + nVO, rgbNewpredEnable [ENHN_LAYER] [iObj]);
			fprintf(pfOut, "Newpred.SegmentType [%d] = \"%s\"\n", iObj + nVO, rgbNewpredSegmentType [ENHN_LAYER] [iObj]==0 ? "VideoPacket" : "VOP");
			fprintf(pfOut, "Newpred.Filename [%d] = \"%s\"\n", iObj + nVO, rgcNewpredRefName [ENHN_LAYER] [iObj]==NULL ? "" : rgcNewpredRefName [ENHN_LAYER] [iObj]);
			fprintf(pfOut, "Newpred.SliceList [%d] = \"%s\"\n", iObj + nVO, rgcNewpredSlicePoint [ENHN_LAYER] [iObj]==NULL ? "" : rgcNewpredSlicePoint [ENHN_LAYER] [iObj]);
			/*************/
		}
	}


	// rate control flag
	nextValidLine (pfPara, pnLine);
	for (iObj = 0; iObj < nVO; iObj++)	{
		if (fscanf (pfPara, "%u", &rguiRateControl [BASE_LAYER] [iObj] ) != 1)	{
			fprintf(stderr, "wrong parameter file format on line %d\n", *pnLine);
			fatal_error("Conversion aborted");
		}
		my_assert (rguiRateControl [BASE_LAYER] [iObj]  == RC_MPEG4 || 
				rguiRateControl [BASE_LAYER] [iObj]  == RC_TM5 ||
				rguiRateControl [BASE_LAYER] [iObj]  == 0);

		/*************/
		fprintf(pfOut, "RateControl.Type [%d] = \"%s\"\n", iObj,
			rguiRateControl [BASE_LAYER] [iObj] == 0 ? "None" : (rguiRateControl [BASE_LAYER] [iObj] == RC_MPEG4 ? "MP4" : "TM5"));
		/*************/
	}
	if (bAnyScalability)	{
		for (iObj = 0; iObj < nVO; iObj++)	{
			if (fscanf (pfPara, "%u", &rguiRateControl [ENHN_LAYER] [iObj]) != 1)	{
				fprintf(stderr, "wrong parameter file format on line %d\n", *pnLine);
				fatal_error("Conversion aborted");
			}
			my_assert (rguiRateControl [ENHN_LAYER] [iObj]  == 0 || 
					rguiRateControl [ENHN_LAYER] [iObj]  == RC_MPEG4 ||
					rguiRateControl [ENHN_LAYER] [iObj]  == RC_TM5);
			/*************/
			fprintf(pfOut, "RateControl.Type [%d] = \"%s\"\n", iObj + nVO,
				rguiRateControl [ENHN_LAYER] [iObj] == 0 ? "None" : (rguiRateControl [ENHN_LAYER] [iObj] == RC_MPEG4 ? "MP4" : "TM5"));
			/*************/
		}
	}

	// bit budget for each object.
	nextValidLine (pfPara, pnLine);
	for (iObj = 0; iObj < nVO; iObj++)	{
		if (fscanf (pfPara, "%d", &rguiBitsBudget [BASE_LAYER] [iObj]) != 1)	{
			fprintf(stderr, "wrong parameter file format on line %d\n", *pnLine);
			fatal_error("Conversion aborted");
		}
		/*************/
		fprintf(pfOut, "RateControl.BitsPerVOP [%d] = %d\n", iObj, rguiBitsBudget [BASE_LAYER] [iObj]);
		/*************/
	}
	if (bAnyScalability)	{
		for (iObj = 0; iObj < nVO; iObj++)	{
			if (fscanf (pfPara, "%d", &rguiBitsBudget [ENHN_LAYER] [iObj]) != 1)	{
				fprintf(stderr, "wrong parameter file format on line %d\n", *pnLine);
				fatal_error("Conversion aborted");
			}
			my_assert (rguiBitsBudget [ENHN_LAYER] [iObj]  > 0);
			/*************/
			fprintf(pfOut, "RateControl.BitsPerVOP [%d] = %d\n", iObj + nVO, rguiBitsBudget [ENHN_LAYER] [iObj]);
			/*************/
		}
	}


	// alpha usage for each object.  0: rectangle, 1: binary, 2: 8-bit, 3: shape only
	nextValidLine (pfPara, pnLine);
	for (iObj = 0; iObj < nVO; iObj++)	{
		UInt uiAlpha;
		if (fscanf (pfPara, "%d", &uiAlpha) != 1)	{
			fprintf(stderr, "wrong parameter file format on line %d\n", *pnLine);
			fatal_error("Conversion aborted");
		}
		else {
			my_assert (uiAlpha == 0 || uiAlpha == 1 || uiAlpha == 2 || uiAlpha == 3);
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
		/*************/
		fprintf(pfOut, "Alpha.Type [%d] = \"%s\"\n", iObj,
			uiAlpha==0 ? "None" : (uiAlpha==1 ? "Binary" : (uiAlpha==2 ? "Gray" : "ShapeOnly")));
		/*************/
	}

	// binary shape rounding para
	nextValidLine (pfPara, pnLine);
	for (iObj = 0; iObj < nVO; iObj++)	{
		if (fscanf (pfPara, "%d", &rgiBinaryAlphaTH [iObj]) != 1)	{
			fprintf	(stderr, "wrong parameter file format on line %d\n", *pnLine);
			fatal_error("Conversion aborted");
		}
		my_assert (rgiBinaryAlphaTH [iObj] >= 0);
		/*************/
		fprintf(pfOut, "Alpha.Binary.RoundingThreshold [%d] = %d\n", iObj, rgiBinaryAlphaTH [iObj]);
		/*************/
	}

	// binary shape size conversion flag
	nextValidLine (pfPara, pnLine);
	for (iObj = 0; iObj < nVO; iObj++)	{
		if (fscanf (pfPara, "%d", &rgbNoCrChange [iObj]) != 1)	{
			fprintf	(stderr, "wrong parameter file format on line %d\n", *pnLine);
			fatal_error("Conversion aborted");
		}
		my_assert (rgbNoCrChange [iObj] == TRUE || rgbNoCrChange [iObj] == FALSE); //boolean value
		if (rgiBinaryAlphaTH [iObj] == 0)
			my_assert (rgbNoCrChange [iObj] == TRUE); //MB-level size conversion of shape is off in lossless mode\n");
		/*************/
		fprintf(pfOut, "Alpha.Binary.SizeConversion.Enable [%d] = %d\n", iObj, !rgbNoCrChange [iObj]);
		/*************/
	}

	//	Added for error resilient mode by Toshiba(1997-11-14)
	nextValidLine (pfPara, pnLine);
	for (iObj = 0; iObj < nVO; iObj++)	{
		if (fscanf (pfPara, "%d", &rgiBinaryAlphaRR [iObj]) != 1)	{
			fprintf	(stderr, "wrong parameter file format on line %d\n", *pnLine);
			fatal_error("Conversion aborted");
		}
		my_assert (rgiBinaryAlphaRR [iObj] >= 0);
		/*************/
		fprintf(pfOut, "ErrorResil.AlphaRefreshRate [%d] = %d\n", iObj, rgiBinaryAlphaRR [iObj]);
		/*************/
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
				fatal_error("Conversion aborted");
			}
		}
		else
			rgbRoundingControlDisable [iObj] = 0;
		my_assert (rgbRoundingControlDisable [iObj]  == 0 || rgbRoundingControlDisable [iObj] == 1);
		/*************/
		fprintf(pfOut, "Motion.RoundingControl.Enable [%d] = %d\n", iObj, !rgbRoundingControlDisable [iObj]);
		/*************/
	}
	if(iVersion > 812)
		nextValidLine (pfPara, pnLine);
	for (iObj = 0; iObj < nVO; iObj++)	{
		if(iVersion > 812)
		{
			if (fscanf (pfPara, "%d", &rgiInitialRoundingType [iObj]) != 1)	{
				fprintf	(stderr, "wrong parameter file format on line %d\n", *pnLine);
				fatal_error("Conversion aborted");
			}
		}
		else
			rgiInitialRoundingType [iObj] = 0;
		my_assert (rgiInitialRoundingType [iObj] == 0 || rgiInitialRoundingType [iObj] == 1);
		/*************/
		fprintf(pfOut, "Motion.RoundingControl.StartValue [%d] = %d\n", iObj, rgiInitialRoundingType [iObj]);
		/*************/
	}

	// error resilient coding disable
	nextValidLine (pfPara, pnLine);
	Int iErrorResilientFlags;
	for (iObj = 0; iObj < nVO; iObj++)	{
		if (fscanf (pfPara, "%d", &iErrorResilientFlags) != 1)	{
			fprintf(stderr, "wrong parameter file format on line %d\n", *pnLine);
			fatal_error("Conversion aborted");
		}
		rgbErrorResilientDisable [BASE_LAYER] [iObj] = iErrorResilientFlags & 0x1;
//	Modified for error resilient mode by Toshiba(1998-1-16)
		rgbDataPartitioning [BASE_LAYER] [iObj]		 = (iErrorResilientFlags & 0x2) ? TRUE : FALSE;
		rgbReversibleVlc [BASE_LAYER] [iObj]		 = (iErrorResilientFlags & 0x4) ? TRUE : FALSE;
//	End Toshiba(1998-1-16)
		if( rgbErrorResilientDisable [BASE_LAYER] [iObj])
			rgiBinaryAlphaRR [iObj] = -1;

		/*************/
		fprintf(pfOut, "ErrorResil.RVLC.Enable [%d] = %d\n", iObj, rgbReversibleVlc [BASE_LAYER] [iObj]);
		fprintf(pfOut, "ErrorResil.DataPartition.Enable [%d] = %d\n", iObj, rgbDataPartitioning [BASE_LAYER] [iObj]);
		fprintf(pfOut, "ErrorResil.VideoPacket.Enable [%d] = %d\n", iObj, !rgbErrorResilientDisable [BASE_LAYER] [iObj]);
		/*************/
	}
	if (bAnyScalability)	{
		for (iObj = 0; iObj < nVO; iObj++)	{
			if (fscanf (pfPara, "%d", &rgbErrorResilientDisable [ENHN_LAYER] [iObj]) != 1)	{
				fprintf(stderr, "wrong parameter file format on line %d\n", *pnLine);
				fatal_error("Conversion aborted");
			}
			my_assert (rgbErrorResilientDisable [ENHN_LAYER] [iObj] == TRUE); //error resilient coding not supported as for now
			rgbErrorResilientDisable [ENHN_LAYER] [iObj] = TRUE;
			rgbDataPartitioning [ENHN_LAYER] [iObj] = FALSE;
			rgbReversibleVlc [ENHN_LAYER] [iObj] = FALSE;
			/*************/
			fprintf(pfOut, "ErrorResil.RVLC.Enable [%d] = %d\n", iObj + nVO, rgbReversibleVlc [ENHN_LAYER] [iObj]);
			fprintf(pfOut, "ErrorResil.DataPartition.Enable [%d] = %d\n", iObj + nVO, rgbDataPartitioning [ENHN_LAYER] [iObj]);
			fprintf(pfOut, "ErrorResil.VideoPacket.Enable [%d] = %d\n", iObj + nVO, !rgbErrorResilientDisable [ENHN_LAYER] [iObj]);
			/*************/
		}
	}

	// Bit threshold for video packet spacing control
	nextValidLine (pfPara, pnLine);
	for (iObj = 0; iObj < nVO; iObj++)	{
		if (fscanf (pfPara, "%d", &rgiVPBitTh [BASE_LAYER] [iObj]) != 1)	{
			fprintf(stderr, "wrong parameter file format on line %d\n", *pnLine);
			fatal_error("Conversion aborted");
		}
		if (rgbErrorResilientDisable [BASE_LAYER] [iObj])
			rgiVPBitTh [BASE_LAYER] [iObj] = -1;	// set VPBitTh to negative value
		/*************/
		fprintf(pfOut, "ErrorResil.VideoPacket.Length [%d] = %d\n", iObj, rgiVPBitTh [BASE_LAYER] [iObj]);
		/*************/
	}
	if (bAnyScalability)	{
		for (iObj = 0; iObj < nVO; iObj++)	{
			if (fscanf (pfPara, "%d", &rgiVPBitTh [ENHN_LAYER] [iObj]) != 1)	{
				fprintf(stderr, "wrong parameter file format on line %d\n", *pnLine);
				fatal_error("Conversion aborted");
			}
			rgiVPBitTh [ENHN_LAYER] [iObj] = -1;
			/*************/
			fprintf(pfOut, "ErrorResil.VideoPacket.Length [%d] = %d\n", iObj + nVO, rgiVPBitTh [ENHN_LAYER] [iObj]);
			/*************/
		}
	}

	// Interlaced coding
	nextValidLine (pfPara, pnLine);
	for (iObj = 0; iObj < nVO; iObj++)	{
		if (fscanf (pfPara, "%d", &rgbInterlacedCoding [BASE_LAYER] [iObj]) != 1)	{
			fprintf(stderr, "wrong parameter file format on line %d\n", *pnLine);
			fatal_error("Conversion aborted");
		}
		my_assert (rgbInterlacedCoding [BASE_LAYER] [iObj] == 0 || rgbInterlacedCoding [BASE_LAYER] [iObj] == 1);
		/*************/
		fprintf(pfOut, "Motion.Interlaced.Enable [%d] = %d\n", iObj, rgbInterlacedCoding [BASE_LAYER] [iObj]);
		/*************/
	}
	if (bAnyScalability)	{
		for (iObj = 0; iObj < nVO; iObj++)	{
			if (fscanf (pfPara, "%d", &rgbInterlacedCoding [ENHN_LAYER] [iObj]) != 1)	{
				fprintf(stderr, "wrong parameter file format on line %d\n", *pnLine);
				fatal_error("Conversion aborted");
			}
			my_assert (rgbInterlacedCoding [ENHN_LAYER] [iObj] == 0 || rgbInterlacedCoding [ENHN_LAYER] [iObj] == 1);
			/*************/
			fprintf(pfOut, "Motion.Interlaced.Enable [%d] = %d\n", iObj + nVO, rgbInterlacedCoding [ENHN_LAYER] [iObj]);
			/*************/
		}
	}
	

	// sadct disable_flag

	if (iVersion>=817)
	{
		nextValidLine (pfPara, pnLine);
		
		for (iObj = 0; iObj < nVO; iObj++)	{
			if (fscanf (pfPara, "%d", &rgbSadctDisable [BASE_LAYER] [iObj]) != 1)	{
				fprintf(stderr, "wrong parameter file format on line %d\n", *pnLine);
				exit (1);
			}
			my_assert (rgbSadctDisable [BASE_LAYER] [iObj] == 0 || rgbSadctDisable [BASE_LAYER] [iObj] == 1);
		}
		if (bAnyScalability)	{
			for (iObj = 0; iObj < nVO; iObj++)	{
				if (fscanf (pfPara, "%d", &rgbSadctDisable [ENHN_LAYER] [iObj]) != 1)	{
					fprintf(stderr, "wrong parameter file format on line %d\n", *pnLine);
					exit (1);
				}
				my_assert (rgbSadctDisable [ENHN_LAYER] [iObj] == 0 || rgbSadctDisable [ENHN_LAYER] [iObj] == 1);
			}
		}
	}
	else {
		for (iObj = 0; iObj < nVO; iObj++)
		{
			rgbSadctDisable [BASE_LAYER] [iObj] = 1;
			rgbSadctDisable [ENHN_LAYER] [iObj] = 1;
		}
	}
	
	for (iObj = 0; iObj < nVO; iObj++)
	{
		/*************/
		fprintf(pfOut, "Texture.SADCT.Enable [%d] = %d\n", iObj, !rgbSadctDisable [BASE_LAYER] [iObj]);
		/*************/
		if(bAnyScalability)
			/*************/
			fprintf(pfOut, "Texture.SADCT.Enable [%d] = %d\n", iObj + nVO, !rgbSadctDisable [ENHN_LAYER] [iObj]);
			/*************/
	}

	// end sadct

	// quantizer selection: 0 -- H.263, 1 -- MPEG
	nextValidLine (pfPara, pnLine);
	for (iObj = 0; iObj < nVO; iObj++)	{
		if (fscanf (pfPara, "%d", (int *)&(rgfQuant [BASE_LAYER] [iObj])) != 1)	{
			fprintf(stderr, "wrong parameter file format on line %d\n", *pnLine);
			fatal_error("Conversion aborted");
		}
		my_assert (rgfQuant [BASE_LAYER] [iObj] ==0 || rgfQuant [BASE_LAYER] [iObj] == 1);
		/*************/
		fprintf(pfOut, "Quant.Type [%d] = \"%s\"\n", iObj, rgfQuant [BASE_LAYER] [iObj]==0 ? "H263" : "MPEG");
		/*************/
	}
	if (bAnyScalability)	{
		for (iObj = 0; iObj < nVO; iObj++)	{
			if (fscanf (pfPara, "%d", (int *)&(rgfQuant [ENHN_LAYER] [iObj])) != 1)	{
				fprintf(stderr, "wrong parameter file format on line %d\n", *pnLine);
				fatal_error("Conversion aborted");
			}
			my_assert (rgfQuant [ENHN_LAYER] [iObj] ==0 || rgfQuant [ENHN_LAYER] [iObj] == 1);
			/*************/
			fprintf(pfOut, "Quant.Type [%d] = \"%s\"\n", iObj + nVO, rgfQuant [ENHN_LAYER] [iObj]==0 ? "H263" : "MPEG");
			/*************/
		}
	}

	// load non-default intra Q-Matrix, 0 -- FALSE, 1 -- TRUE
	nextValidLine (pfPara, pnLine);
	for (iObj = 0; iObj < nVO; iObj++)	{
		if (fscanf (pfPara, "%d", &rgbLoadIntraMatrix [BASE_LAYER] [iObj]) != 1)	{
			fprintf(stderr, "wrong parameter file format on line %d\n", *pnLine);
			fatal_error("Conversion aborted");
		}
		my_assert (rgbLoadIntraMatrix [BASE_LAYER] [iObj] ==0 || 
				rgbLoadIntraMatrix [BASE_LAYER] [iObj] == 1);
		/*************/
		fprintf(pfOut, "Texture.QuantMatrix.Intra.Enable [%d] = %d\n", iObj, rgbLoadIntraMatrix [BASE_LAYER] [iObj]);
		/*************/
		if (rgbLoadIntraMatrix [BASE_LAYER] [iObj]) {
			/*************/
			fprintf(pfOut, "Texture.QuantMatrix.Intra [%d] = {", iObj);
			/*************/
			UInt i = 0;
			do {
				if(i>0)
					/*************/
					fprintf(pfOut, ", ");
					/*************/
				if (fscanf (pfPara, "%d", &rgppiIntraQuantizerMatrix [BASE_LAYER] [iObj] [i]) != 1)	{
					fprintf(stderr, "wrong parameter file format on line %d\n", *pnLine);
					fatal_error("Conversion aborted");
				}
				/*************/
				fprintf(pfOut, "%d", rgppiIntraQuantizerMatrix [BASE_LAYER] [iObj] [i]);
				/*************/
			} while (rgppiIntraQuantizerMatrix [BASE_LAYER] [iObj] [i] != 0 && ++i < BLOCK_SQUARE_SIZE);
			/*************/
			fprintf(pfOut, "}\n");
			/*************/
		}
	}
	if (bAnyScalability)	{
		for (iObj = 0; iObj < nVO; iObj++)	{
			if (fscanf (pfPara, "%d", &rgbLoadIntraMatrix [ENHN_LAYER] [iObj]) != 1)	{
				fprintf(stderr, "wrong parameter file format on line %d\n", *pnLine);
				fatal_error("Conversion aborted");
			}
			my_assert (rgbLoadIntraMatrix [ENHN_LAYER] [iObj] ==0 || 
					rgbLoadIntraMatrix [ENHN_LAYER] [iObj] == 1);
			/*************/
			fprintf(pfOut, "Texture.QuantMatrix.Intra.Enable [%d] = %d\n", iObj + nVO, rgbLoadIntraMatrix [ENHN_LAYER] [iObj]);
			/*************/
			if (rgbLoadIntraMatrix [ENHN_LAYER] [iObj]) {
				/*************/
				fprintf(pfOut, "Texture.QuantMatrix.Intra [%d] = {", iObj + nVO);
				/*************/
				UInt i = 0;
				do {
					if(i>0)
						/*************/
						fprintf(pfOut, ", ");
						/*************/
					if (fscanf (pfPara, "%d", &rgppiIntraQuantizerMatrix [ENHN_LAYER] [iObj] [i]) != 1)	{
						fprintf(stderr, "wrong parameter file format on line %d\n", *pnLine);
						fatal_error("Conversion aborted");
					}
					/*************/
					fprintf(pfOut, "%d", rgppiIntraQuantizerMatrix [ENHN_LAYER] [iObj] [i]);
					/*************/
				} while (rgppiIntraQuantizerMatrix [ENHN_LAYER] [iObj] [i] != 0 && ++i < BLOCK_SQUARE_SIZE);
				/*************/
				fprintf(pfOut, "}\n");
				/*************/
			}
		}
	}

	// load non-default inter Q-Matrix, 0 -- FALSE, 1 -- TRUE
	nextValidLine (pfPara, pnLine);
	for (iObj = 0; iObj < nVO; iObj++)	{
		if (fscanf (pfPara, "%d", &rgbLoadInterMatrix [BASE_LAYER] [iObj]) != 1)	{
			fprintf(stderr, "wrong parameter file format on line %d\n", *pnLine);
			fatal_error("Conversion aborted");
		}
		my_assert (rgbLoadInterMatrix [BASE_LAYER] [iObj] ==0 || 
				rgbLoadInterMatrix [BASE_LAYER] [iObj] == 1);
		/*************/
		fprintf(pfOut, "Texture.QuantMatrix.Inter.Enable [%d] = %d\n", iObj, rgbLoadInterMatrix [BASE_LAYER] [iObj]);
		/*************/
		if (rgbLoadInterMatrix [BASE_LAYER] [iObj]) {
			/*************/
			fprintf(pfOut, "Texture.QuantMatrix.Inter [%d] = {", iObj);
			/*************/
			UInt i = 0;
			do {
				if(i>0)
					/*************/
					fprintf(pfOut, ", ");
					/*************/
				if (fscanf (pfPara, "%d", &rgppiInterQuantizerMatrix [BASE_LAYER] [iObj] [i]) != 1)	{
					fprintf(stderr, "wrong parameter file format on line %d\n", *pnLine);
					fatal_error("Conversion aborted");
				}
				/*************/
				fprintf(pfOut, "%d", rgppiInterQuantizerMatrix [BASE_LAYER] [iObj] [i]);
				/*************/
			} while (rgppiInterQuantizerMatrix [BASE_LAYER] [iObj] [i] != 0 && ++i < BLOCK_SQUARE_SIZE);
			/*************/
			fprintf(pfOut, "}\n");
			/*************/
		}
	}
	if (bAnyScalability)	{
		for (iObj = 0; iObj < nVO; iObj++)	{
			if (fscanf (pfPara, "%d", &rgbLoadInterMatrix [ENHN_LAYER] [iObj]) != 1)	{
				fprintf(stderr, "wrong parameter file format on line %d\n", *pnLine);
				fatal_error("Conversion aborted");
			}
			my_assert (rgbLoadInterMatrix [ENHN_LAYER] [iObj] ==0 || 
					rgbLoadInterMatrix [ENHN_LAYER] [iObj] == 1);
			/*************/
			fprintf(pfOut, "Texture.QuantMatrix.Inter.Enable [%d] = %d\n", iObj + nVO, rgbLoadInterMatrix [ENHN_LAYER] [iObj]);
			/*************/
			if (rgbLoadInterMatrix [ENHN_LAYER] [iObj]) {
				/*************/
				fprintf(pfOut, "Texture.QuantMatrix.Inter [%d] = {", iObj + nVO);
				/*************/
				UInt i = 0;
				do {
					if(i>0)
						/*************/
						fprintf(pfOut, ", ");
						/*************/
					if (fscanf (pfPara, "%d", &rgppiInterQuantizerMatrix [ENHN_LAYER] [iObj] [i]) != 1)	{
						fprintf(stderr, "wrong parameter file format on line %d\n", *pnLine);
						fatal_error("Conversion aborted");
					}
					/*************/
					fprintf(pfOut, "%d", rgppiInterQuantizerMatrix [ENHN_LAYER] [iObj] [i]);
					/*************/
				} while (rgppiInterQuantizerMatrix [ENHN_LAYER] [iObj] [i] != 0 && ++i < BLOCK_SQUARE_SIZE);
				/*************/
				fprintf(pfOut, "}\n");
				/*************/
			}
		}
	}

	// threhold to code Intra-DC as AC
	nextValidLine (pfPara, pnLine);
	for (iObj = 0; iObj < nVO; iObj++)	{
		if (fscanf (pfPara, "%d", &(rgiIntraDCSwitchingThr [BASE_LAYER] [iObj])) != 1)	{
			fprintf(stderr, "wrong parameter file format on line %d\n", *pnLine);
			fatal_error("Conversion aborted");
		}
		my_assert (rgiIntraDCSwitchingThr [BASE_LAYER] [iObj] >= 0 && 
				rgiIntraDCSwitchingThr [BASE_LAYER] [iObj] <= 7);
		/*************/
		fprintf(pfOut, "Texture.IntraDCThreshold [%d] = %d\n", iObj, rgiIntraDCSwitchingThr [BASE_LAYER] [iObj]);
		/*************/
	}
	if (bAnyScalability)	{
		for (iObj = 0; iObj < nVO; iObj++)	{
			if (fscanf (pfPara, "%d", &(rgiIntraDCSwitchingThr [ENHN_LAYER] [iObj])) != 1)	{
				fprintf(stderr, "wrong parameter file format on line %d\n", *pnLine);
				fatal_error("Conversion aborted");
			}
			my_assert (rgiIntraDCSwitchingThr [ENHN_LAYER] [iObj] >= 0 && 
					rgiIntraDCSwitchingThr [ENHN_LAYER] [iObj] <= 7);
			/*************/
			fprintf(pfOut, "Texture.IntraDCThreshold [%d] = %d\n", iObj + nVO, rgiIntraDCSwitchingThr [ENHN_LAYER] [iObj]);
			/*************/
		}
	}

	// I-VO quantization stepsize
	nextValidLine (pfPara, pnLine);
	for (iObj = 0; iObj < nVO; iObj++)	{
		if (fscanf (pfPara, "%d", &(rgiIStep [BASE_LAYER] [iObj])) != 1)	{
			fprintf(stderr, "wrong parameter file format on line %d\n", *pnLine);
			fatal_error("Conversion aborted");
		}
		my_assert (rgiIStep [BASE_LAYER] [iObj] > 0 && rgiIStep [BASE_LAYER] [iObj] < (1<<uiQuantPrecision));
		/*************/
		fprintf(pfOut, "Texture.QuantStep.IVOP [%d] = %d\n", iObj, rgiIStep [BASE_LAYER] [iObj]);
		/*************/
	}
	if (bAnyScalability)	{
		for (iObj = 0; iObj < nVO; iObj++)	{
			if (fscanf (pfPara, "%d", &(rgiIStep [ENHN_LAYER] [iObj])) != 1)	{
				fprintf(stderr, "wrong parameter file format on line %d\n", *pnLine);
				fatal_error("Conversion aborted");
			}
			my_assert (rgiIStep [ENHN_LAYER] [iObj] > 0 && rgiIStep [ENHN_LAYER] [iObj] < (1<<uiQuantPrecision));
			/*************/
			fprintf(pfOut, "Texture.QuantStep.IVOP [%d] = %d\n", iObj + nVO, rgiIStep [ENHN_LAYER] [iObj]);
			/*************/
		}
	}

	// P-VO quantization stepsize
	nextValidLine (pfPara, pnLine);
	for (iObj = 0; iObj < nVO; iObj++)	{
		if (fscanf (pfPara, "%d", &(rgiPStep [BASE_LAYER] [iObj])) != 1)	{
			fprintf(stderr, "wrong parameter file format on line %d\n", *pnLine);
			fatal_error("Conversion aborted");
		}
		my_assert (rgiPStep [BASE_LAYER] [iObj] > 0 && rgiPStep [BASE_LAYER] [iObj] < (1<<uiQuantPrecision));
		/*************/
		fprintf(pfOut, "Texture.QuantStep.PVOP [%d] = %d\n", iObj, rgiPStep [BASE_LAYER] [iObj]);
		/*************/
	}
	if (bAnyScalability)	{
		for (iObj = 0; iObj < nVO; iObj++)	{
			if (fscanf (pfPara, "%d", &(rgiPStep [ENHN_LAYER] [iObj])) != 1)	{
				fprintf(stderr, "wrong parameter file format on line %d\n", *pnLine);
				fatal_error("Conversion aborted");
			}
			my_assert (rgiPStep [ENHN_LAYER] [iObj] > 0 && rgiPStep [ENHN_LAYER] [iObj] < (1<<uiQuantPrecision));
			/*************/
			fprintf(pfOut, "Texture.QuantStep.PVOP [%d] = %d\n", iObj + nVO, rgiPStep [ENHN_LAYER] [iObj]);
			/*************/
		}
	}

	// quantization stepsize for B-VOP
	nextValidLine (pfPara, pnLine);
	for (iObj = 0; iObj < nVO; iObj++)	{
		if (fscanf (pfPara, "%d", &(rgiStepBCode [BASE_LAYER] [iObj])) != 1)	{
			fprintf(stderr, "wrong parameter file format on line %d\n", *pnLine);
			fatal_error("Conversion aborted");
		}
		my_assert (rgiStepBCode [BASE_LAYER] [iObj] > 0 && rgiStepBCode [BASE_LAYER] [iObj] < (1<<uiQuantPrecision));
		/*************/
		fprintf(pfOut, "Texture.QuantStep.BVOP [%d] = %d\n", iObj, rgiStepBCode [BASE_LAYER] [iObj]);
		/*************/
	}
	if (bAnyScalability)	{
		for (iObj = 0; iObj < nVO; iObj++)	{
			if (fscanf (pfPara, "%d", &(rgiStepBCode [ENHN_LAYER] [iObj])) != 1)	{
				fprintf(stderr, "wrong parameter file format on line %d\n", *pnLine);
				fatal_error("Conversion aborted");
			}
			my_assert (rgiStepBCode [ENHN_LAYER] [iObj] > 0 && rgiStepBCode [ENHN_LAYER] [iObj] < (1<<uiQuantPrecision));
			/*************/
			fprintf(pfOut, "Texture.QuantStep.BVOP [%d] = %d\n", iObj + nVO, rgiStepBCode [ENHN_LAYER] [iObj]);
			/*************/
		}
	}

	// load non-default gray alpha intra Q-Matrix, 0 -- FALSE, 1 -- TRUE
	nextValidLine (pfPara, pnLine);
	for (iObj = 0; iObj < nVO; iObj++)	{
		if (fscanf (pfPara, "%d", &rgbLoadIntraMatrixAlpha [BASE_LAYER] [iObj]) != 1)	{
			fprintf(stderr, "wrong parameter file format on line %d\n", *pnLine);
			fatal_error("Conversion aborted");
		}
		my_assert (rgbLoadIntraMatrixAlpha [BASE_LAYER] [iObj] ==0 || 
				rgbLoadIntraMatrixAlpha [BASE_LAYER] [iObj] == 1);
		/*************/
		fprintf(pfOut, "Alpha.QuantMatrix.Intra.Enable [%d] = %d\n", iObj, rgbLoadIntraMatrixAlpha [BASE_LAYER] [iObj]);
		/*************/
		if (rgbLoadIntraMatrixAlpha [BASE_LAYER] [iObj]) {
			/*************/
			fprintf(pfOut, "Alpha.QuantMatrix.Intra [%d] = {", iObj);
			/*************/
			for (UInt i = 0; i < BLOCK_SQUARE_SIZE; i++) {
				if(i>0)
					/*************/
					fprintf(pfOut, ", ");
					/*************/
				if (fscanf (pfPara, "%d", &rgppiIntraQuantizerMatrixAlpha [BASE_LAYER] [iObj] [i]) != 1)	{
					fprintf(stderr, "wrong parameter file format on line %d\n", *pnLine);
					fatal_error("Conversion aborted");
				}
				/*************/
				fprintf(pfOut, "%d", rgppiIntraQuantizerMatrixAlpha [BASE_LAYER] [iObj] [i]);
				/*************/
			}
			/*************/
			fprintf(pfOut, "}\n");
			/*************/
		}
	}
	if (bAnyScalability)	{
		for (iObj = 0; iObj < nVO; iObj++)	{
			if (fscanf (pfPara, "%d", &rgbLoadIntraMatrixAlpha [ENHN_LAYER] [iObj]) != 1)	{
				fprintf(stderr, "wrong parameter file format on line %d\n", *pnLine);
				fatal_error("Conversion aborted");
			}
			my_assert (rgbLoadIntraMatrixAlpha [ENHN_LAYER] [iObj] ==0 || 
					rgbLoadIntraMatrixAlpha [ENHN_LAYER] [iObj] == 1);
			/*************/
			fprintf(pfOut, "Alpha.QuantMatrix.Intra.Enable [%d] = %d\n", iObj + nVO, rgbLoadIntraMatrixAlpha [ENHN_LAYER] [iObj]);
			/*************/
			if (rgbLoadIntraMatrixAlpha [ENHN_LAYER] [iObj]) {
				/*************/
				fprintf(pfOut, "Alpha.QuantMatrix.Intra [%d] = {", iObj + nVO);
				/*************/
				for (UInt i = 0; i < BLOCK_SQUARE_SIZE; i++) {
					if(i>0)
						/*************/
						fprintf(pfOut, ", ");
						/*************/
					if (fscanf (pfPara, "%d", &rgppiIntraQuantizerMatrixAlpha [ENHN_LAYER] [iObj] [i]) != 1)	{
						fprintf(stderr, "wrong parameter file format on line %d\n", *pnLine);
						fatal_error("Conversion aborted");
					}
					/*************/
					fprintf(pfOut, "%d", rgppiIntraQuantizerMatrixAlpha [ENHN_LAYER] [iObj] [i]);
					/*************/
				}
				/*************/
				fprintf(pfOut, "}\n");
				/*************/
			}
		}
	}

	// load non-default  gray alpha inter Q-Matrix, 0 -- FALSE, 1 -- TRUE
	nextValidLine (pfPara, pnLine);
	for (iObj = 0; iObj < nVO; iObj++)	{
		if (fscanf (pfPara, "%d", &rgbLoadInterMatrixAlpha [BASE_LAYER] [iObj]) != 1)	{
			fprintf(stderr, "wrong parameter file format on line %d\n", *pnLine);
			fatal_error("Conversion aborted");
		}
		my_assert (rgbLoadInterMatrixAlpha [BASE_LAYER] [iObj] ==0 || 
				rgbLoadInterMatrixAlpha [BASE_LAYER] [iObj] == 1);
		/*************/
		fprintf(pfOut, "Alpha.QuantMatrix.Inter.Enable [%d] = %d\n", iObj, rgbLoadInterMatrixAlpha [BASE_LAYER] [iObj]);
		/*************/
		if (rgbLoadInterMatrixAlpha [BASE_LAYER] [iObj]) {
			/*************/
			fprintf(pfOut, "Alpha.QuantMatrix.Inter [%d] = {", iObj);
			/*************/
			for (UInt i = 0; i < BLOCK_SQUARE_SIZE; i++) {
				if(i>0)
					/*************/
					fprintf(pfOut, ", ");
					/*************/
				if (fscanf (pfPara, "%d", &rgppiInterQuantizerMatrixAlpha [BASE_LAYER] [iObj] [i]) != 1)	{
					fprintf(stderr, "wrong parameter file format on line %d\n", *pnLine);
					fatal_error("Conversion aborted");
				}
				/*************/
				fprintf(pfOut, "%d", rgppiInterQuantizerMatrixAlpha [BASE_LAYER] [iObj] [i]);
				/*************/
			}
			/*************/
			fprintf(pfOut, "}\n");
			/*************/
		}
	}
	if (bAnyScalability)	{
		for (iObj = 0; iObj < nVO; iObj++)	{
			if (fscanf (pfPara, "%d", &rgbLoadInterMatrixAlpha [ENHN_LAYER] [iObj]) != 1)	{
				fprintf(stderr, "wrong parameter file format on line %d\n", *pnLine);
				fatal_error("Conversion aborted");
			}
			my_assert (rgbLoadInterMatrixAlpha [ENHN_LAYER] [iObj] ==0 || 
					rgbLoadInterMatrixAlpha [ENHN_LAYER] [iObj] == 1);
			/*************/
			fprintf(pfOut, "Alpha.QuantMatrix.Inter.Enable [%d] = %d\n", iObj + nVO, rgbLoadInterMatrixAlpha [ENHN_LAYER] [iObj]);
			/*************/
			if (rgbLoadInterMatrixAlpha [ENHN_LAYER] [iObj]) {
				/*************/
				fprintf(pfOut, "Alpha.QuantMatrix.Inter [%d] = {", iObj + nVO);
				/*************/
				for (UInt i = 0; i < BLOCK_SQUARE_SIZE; i++) {
					if(i>0)
						/*************/
						fprintf(pfOut, ", ");
						/*************/
					if (fscanf (pfPara, "%d", &rgppiInterQuantizerMatrixAlpha [ENHN_LAYER] [iObj] [i]) != 1)	{
						fprintf(stderr, "wrong parameter file format on line %d\n", *pnLine);
						fatal_error("Conversion aborted");
					}
					/*************/
					fprintf(pfOut, "%d", rgppiInterQuantizerMatrixAlpha [ENHN_LAYER] [iObj] [i]);
					/*************/
				}
				/*************/
				fprintf(pfOut, "}\n");
				/*************/
			}
		}
	}

	// I-VO quantization stepsize for Alpha
	nextValidLine (pfPara, pnLine);
	for (iObj = 0; iObj < nVO; iObj++)	{
		if (fscanf (pfPara, "%d", &(rgiIStepAlpha [BASE_LAYER] [iObj])) != 1)	{
			fprintf(stderr, "wrong parameter file format on line %d\n", *pnLine);
			fatal_error("Conversion aborted");
		}
		my_assert (rgiIStepAlpha [BASE_LAYER] [iObj] > 0 && rgiIStepAlpha [BASE_LAYER] [iObj] < 32);
		/*************/
		fprintf(pfOut, "Alpha.QuantStep.IVOP [%d] = %d\n", iObj, rgiIStepAlpha [BASE_LAYER] [iObj]);
		/*************/
	}
	if (bAnyScalability)	{
		for (iObj = 0; iObj < nVO; iObj++)	{
			if (fscanf (pfPara, "%d", &(rgiIStepAlpha [ENHN_LAYER] [iObj])) != 1)	{
				fprintf(stderr, "wrong parameter file format on line %d\n", *pnLine);
				fatal_error("Conversion aborted");
			}
			my_assert (rgiIStepAlpha [ENHN_LAYER] [iObj] > 0 && rgiIStepAlpha [ENHN_LAYER] [iObj] < 32);
			/*************/
			fprintf(pfOut, "Alpha.QuantStep.IVOP [%d] = %d\n", iObj + nVO, rgiIStepAlpha [ENHN_LAYER] [iObj]);
			/*************/
		}
	}

	// P-VO quantization stepsize for Alpha
	nextValidLine (pfPara, pnLine);
	for (iObj = 0; iObj < nVO; iObj++)	{
		if (fscanf (pfPara, "%d", &(rgiPStepAlpha [BASE_LAYER] [iObj])) != 1)	{
			fprintf(stderr, "wrong parameter file format on line %d\n", *pnLine);
			fatal_error("Conversion aborted");
		}
		my_assert (rgiPStepAlpha [BASE_LAYER] [iObj] > 0 && rgiPStepAlpha [BASE_LAYER] [iObj] < 64);
		/*************/
		fprintf(pfOut, "Alpha.QuantStep.PVOP [%d] = %d\n", iObj, rgiPStepAlpha [BASE_LAYER] [iObj]);
		/*************/
	}
	if (bAnyScalability)	{
		for (iObj = 0; iObj < nVO; iObj++)	{
			if (fscanf (pfPara, "%d", &(rgiPStepAlpha [ENHN_LAYER] [iObj])) != 1)	{
				fprintf(stderr, "wrong parameter file format on line %d\n", *pnLine);
				fatal_error("Conversion aborted");
			}
			my_assert (rgiPStepAlpha [ENHN_LAYER] [iObj] > 0 && rgiPStepAlpha [ENHN_LAYER] [iObj] < 64);
			/*************/
			fprintf(pfOut, "Alpha.QuantStep.PVOP [%d] = %d\n", iObj + nVO, rgiPStepAlpha [ENHN_LAYER] [iObj]);
			/*************/
		}
	}

	// B-VO quantization stepsize for Alpha
	nextValidLine (pfPara, pnLine);
	for (iObj = 0; iObj < nVO; iObj++)	{
		if (fscanf (pfPara, "%d", &(rgiBStepAlpha [BASE_LAYER] [iObj])) != 1)	{
			fprintf(stderr, "wrong parameter file format on line %d\n", *pnLine);
			fatal_error("Conversion aborted");
		}
		my_assert (rgiBStepAlpha [BASE_LAYER] [iObj] > 0 && rgiBStepAlpha [BASE_LAYER] [iObj] < 64);
		/*************/
		fprintf(pfOut, "Alpha.QuantStep.BVOP [%d] = %d\n", iObj, rgiBStepAlpha [BASE_LAYER] [iObj]);
		/*************/
	}
	if (bAnyScalability)	{
		for (iObj = 0; iObj < nVO; iObj++)	{
			if (fscanf (pfPara, "%d", &(rgiBStepAlpha [ENHN_LAYER] [iObj])) != 1)	{
				fprintf(stderr, "wrong parameter file format on line %d\n", *pnLine);
				fatal_error("Conversion aborted");
			}
			my_assert (rgiBStepAlpha [ENHN_LAYER] [iObj] > 0 && rgiBStepAlpha [ENHN_LAYER] [iObj] < 64);
			/*************/
			fprintf(pfOut, "Alpha.QuantStep.BVOP [%d] = %d\n", iObj + nVO, rgiBStepAlpha [ENHN_LAYER] [iObj]);
			/*************/
		}
	}

	// disable_gray_quant_update
	nextValidLine (pfPara, pnLine);
	for (iObj = 0; iObj < nVO; iObj++)	{
		if (fscanf (pfPara, "%d", &(rgbNoGrayQuantUpdate [BASE_LAYER] [iObj])) != 1)	{
			fprintf(stderr, "wrong parameter file format on line %d\n", *pnLine);
			fatal_error("Conversion aborted");
		}
		/*************/
		fprintf(pfOut, "Alpha.QuantDecouple.Enable [%d] = %d\n", iObj, rgbNoGrayQuantUpdate [BASE_LAYER] [iObj]);
		/*************/
	}
	if (bAnyScalability)	{
		for (iObj = 0; iObj < nVO; iObj++)	{
			if (fscanf (pfPara, "%d", &(rgbNoGrayQuantUpdate [ENHN_LAYER] [iObj])) != 1)	{
				fprintf(stderr, "wrong parameter file format on line %d\n", *pnLine);
				fatal_error("Conversion aborted");
			}
			/*************/
			fprintf(pfOut, "Alpha.QuantDecouple.Enable [%d] = %d\n", iObj + nVO, rgbNoGrayQuantUpdate [ENHN_LAYER] [iObj]);
			/*************/
		}
	}

	// number of P-VOP's between 2 I-VOP's; if there are B-VOPs, the no. of encode frames will multiply
	nextValidLine (pfPara, pnLine);
	for (iObj = 0; iObj < nVO; iObj++)	{
		if (fscanf (pfPara, "%d", &rgiNumPbetweenIVOP [iObj]) != 1)	{
			fprintf(stderr, "wrong parameter file format on line %d\n", *pnLine);
			fatal_error("Conversion aborted");
		}
		/*************/
		fprintf(pfOut, "Motion.PBetweenICount [%d] = %d\n", iObj, rgiNumPbetweenIVOP [iObj]);
		/*************/
	}

	// number of B-VOP's between 2 P-VOP's
	nextValidLine (pfPara, pnLine);
	for (iObj = 0; iObj < nVO; iObj++)	{
		if (fscanf (pfPara, "%d", &rgiNumBbetweenPVOP [iObj]) != 1)	{
			fprintf(stderr, "wrong parameter file format on line %d\n", *pnLine);
			fatal_error("Conversion aborted");
		}
		my_assert (rgiNumBbetweenPVOP [iObj] >= 0);
		/*************/
		fprintf(pfOut, "Motion.BBetweenPCount [%d] = %d\n", iObj, rgiNumBbetweenPVOP [iObj]);
		/*************/
	}

	if(iVersion==815)
	{
		// rgbAllowSkippedPMBs
		nextValidLine (pfPara, pnLine);
		for (iObj = 0; iObj < nVO; iObj++)	{
			if (fscanf (pfPara, "%d", &rgbAllowSkippedPMBs [iObj]) != 1)	{
				fprintf(stderr, "wrong parameter file format on line %d\n", *pnLine);
				fatal_error("Conversion aborted");
			}
			my_assert (rgbAllowSkippedPMBs [iObj] == 0 || rgbAllowSkippedPMBs[iObj]==1);
		}
	}
	else
		for (iObj = 0; iObj < nVO; iObj++)
			rgbAllowSkippedPMBs [iObj] = 1;

	for (iObj = 0; iObj < nVO; iObj++)
		/*************/
		fprintf(pfOut, "Motion.SkippedMB.Enable [%d] = %d\n", iObj, rgbAllowSkippedPMBs [iObj]);
		/*************/

	//added to encode GOV header by SONY 980212
	//number of VOP between GOV header
	//CAUTION:GOV period is not used in spatial scalable coding
	nextValidLine (pfPara, pnLine);
	for (iObj = 0; iObj < nVO; iObj++)	{
		if (fscanf (pfPara, "%d", &rgiGOVperiod [iObj]) != 1)	{
			fprintf(stderr, "wrong parameter file format on line %d\n", *pnLine);
			fatal_error("Conversion aborted");
		}
		my_assert (rgiGOVperiod [iObj] >= 0);
		/*************/
		fprintf(pfOut, "GOV.Enable [%d] = %d\n", iObj, rgiGOVperiod [iObj] > 0);
		fprintf(pfOut, "GOV.Period [%d] = %d\n", iObj, rgiGOVperiod [iObj]);
		/*************/
	}
	//980212

	// deblocking filter disable
	nextValidLine (pfPara, pnLine);
	for (iObj = 0; iObj < nVO; iObj++)	{
		if (fscanf (pfPara, "%d", &rgbDeblockFilterDisable [iObj]) != 1)	{
			fprintf(stderr, "wrong parameter file format on line %d\n", *pnLine);
			fatal_error("Conversion aborted");
		}
		/*************/
		fprintf(pfOut, "Motion.DeblockingFilter.Enable [%d] = %d\n", iObj, !rgbDeblockFilterDisable [iObj]);
		/*************/
	}

	// Temporal sampling rate
	nextValidLine (pfPara, pnLine);
	for (iObj = 0; iObj < nVO; iObj++)	{
		if (fscanf (pfPara, "%d", &rgiTSRate [iObj]) != 1)	{
			fprintf(stderr, "wrong parameter file format on line %d\n", *pnLine);
			fatal_error("Conversion aborted");
		}
		my_assert (rgiTSRate [iObj] >= 1);
		/*************/
		fprintf(pfOut, "Source.SamplingRate [%d] = %d\n", iObj, rgiTSRate [iObj]);
		/*************/
	}
	if ( bAnyScalability ){ // This part is added by Norio Ito
		for (iObj = 0; iObj < nVO; iObj++)	{
			if (fscanf (pfPara, "%d", &rgiEnhcTSRate [iObj]) != 1)	{
				fprintf(stderr, "wrong parameter file format on line %d\n", *pnLine);
				fatal_error("Conversion aborted");
			}
			my_assert (rgiEnhcTSRate [iObj] >= 1);
			/*************/
			fprintf(pfOut, "Source.SamplingRate [%d] = %d\n", iObj + nVO, rgiEnhcTSRate [iObj]);
			/*************/
		}
	}

	// maximum displacement
	nextValidLine (pfPara, pnLine);
	for (iObj = 0; iObj < nVO; iObj++) {
		if (fscanf (pfPara, "%d", &rguiSearchRange [BASE_LAYER] [iObj]) != 1)	{
			fprintf(stderr, "wrong parameter file format on line %d\n", *pnLine);
			fatal_error("Conversion aborted");
		}
		my_assert (rguiSearchRange [BASE_LAYER] [iObj] >= 1);
		/*************/
		fprintf(pfOut, "Motion.SearchRange [%d] = %d\n", iObj, rguiSearchRange [BASE_LAYER] [iObj]);
		/*************/
	}
	if (bAnyScalability)	{
		for (iObj = 0; iObj < nVO; iObj++) {
			if (fscanf (pfPara, "%d", &rguiSearchRange [ENHN_LAYER] [iObj]) != 1)	{
				fprintf(stderr, "wrong parameter file format on line %d\n", *pnLine);
				fatal_error("Conversion aborted");
			}
			my_assert (rguiSearchRange [ENHN_LAYER] [iObj] >= 1);
			/*************/
			fprintf(pfOut, "Motion.SearchRange [%d] = %d\n", iObj + nVO, rguiSearchRange [ENHN_LAYER] [iObj]);
			/*************/
		}
	}

	// use original or reconstructed previous VO for ME
	nextValidLine (pfPara, pnLine);
	for (iObj = 0; iObj < nVO; iObj++)	{
		if (fscanf (pfPara, "%d", &rgbOriginalME [BASE_LAYER] [iObj]) != 1)	{
			fprintf(stderr, "wrong parameter file format on line %d\n", *pnLine);
			fatal_error("Conversion aborted");
		}
		my_assert (rgbOriginalME [BASE_LAYER] [iObj] == 0 || rgbOriginalME [BASE_LAYER] [iObj] == 1);
		/*************/
		fprintf(pfOut, "Motion.UseSourceForME.Enable [%d] = %d\n", iObj, rgbOriginalME [BASE_LAYER] [iObj]);
		/*************/
	}
	if (bAnyScalability)	{
		for (iObj = 0; iObj < nVO; iObj++)	{
			if (fscanf (pfPara, "%d", &rgbOriginalME [ENHN_LAYER] [iObj]) != 1)	{
				fprintf(stderr, "wrong parameter file format on line %d\n", *pnLine);
				fatal_error("Conversion aborted");
			}
			my_assert (rgbOriginalME [ENHN_LAYER] [iObj] == 0 || rgbOriginalME [ENHN_LAYER] [iObj] == 1);
			/*************/
			fprintf(pfOut, "Motion.UseSourceForME.Enable [%d] = %d\n", iObj + nVO, rgbOriginalME [ENHN_LAYER] [iObj]);
			/*************/
		}
	}

	// disable advance prediction (obmc only)?
	nextValidLine (pfPara, pnLine);
	for (iObj = 0; iObj < nVO; iObj++)	{
		if (fscanf (pfPara, "%d", &rgbAdvPredDisable [BASE_LAYER] [iObj]) != 1)	{
			fprintf(stderr, "wrong parameter file format on line %d\n", *pnLine);
			fatal_error("Conversion aborted");
		}
		my_assert (rgbAdvPredDisable [BASE_LAYER] [iObj] == 0 || rgbAdvPredDisable [BASE_LAYER] [iObj] == 1);
		/*************/
		fprintf(pfOut, "Motion.AdvancedPrediction.Enable [%d] = %d\n", iObj, !rgbAdvPredDisable [BASE_LAYER] [iObj]);
		/*************/
	}
	if (bAnyScalability)	{
		for (iObj = 0; iObj < nVO; iObj++)	{
			if (fscanf (pfPara, "%d", &rgbAdvPredDisable [ENHN_LAYER] [iObj]) != 1)	{
				fprintf(stderr, "wrong parameter file format on line %d\n", *pnLine);
				fatal_error("Conversion aborted");
			}
			my_assert (rgbAdvPredDisable [ENHN_LAYER] [iObj] == 0 || rgbAdvPredDisable [ENHN_LAYER] [iObj] == 1);
			/*************/
			fprintf(pfOut, "Motion.AdvancedPrediction.Enable [%d] = %d\n", iObj + nVO, !rgbAdvPredDisable [ENHN_LAYER] [iObj]);
			/*************/
		}
	}

	if(iVersion>815)
	{
		// Quarter Sample
		nextValidLine (pfPara, pnLine);

		
		for (iObj = 0; iObj < nVO; iObj++)	{
			if (fscanf (pfPara, "%d", &rgbQuarterSample [BASE_LAYER] [iObj]) != 1)	{
				fprintf(stderr, "wrong parameter file format on line %d\n", *pnLine);
				exit (1);
			}
			my_assert (rgbQuarterSample [BASE_LAYER] [iObj] == 0 || 
					rgbQuarterSample [BASE_LAYER] [iObj] == 1);
		}
		if (bAnyScalability)	{
			
			for (iObj = 0; iObj < nVO; iObj++)	{
				if (fscanf (pfPara, "%d", &rgbQuarterSample [ENHN_LAYER] [iObj]) != 1)	{
					fprintf(stderr, "wrong parameter file format on line %d\n", *pnLine);
					exit (1);
				}
				my_assert (rgbQuarterSample [ENHN_LAYER] [iObj] == 0 || rgbQuarterSample [ENHN_LAYER] [iObj] == 1);
			}
		}
	}
	else
	{
		for (iObj = 0; iObj < nVO; iObj++)	{
			rgbQuarterSample [BASE_LAYER] [iObj] = 0;
			rgbQuarterSample [ENHN_LAYER] [iObj] = 0;
		}
	}

	for (iObj = 0; iObj < nVO; iObj++)	{
		/*************/
		fprintf(pfOut, "Motion.QuarterSample.Enable [%d] = %d\n", iObj, rgbQuarterSample [ENHN_LAYER] [iObj]);
		/*************/
		if(bAnyScalability)
			/*************/
			fprintf(pfOut, "Motion.QuarterSample.Enable [%d] = %d\n", iObj + nVO, rgbQuarterSample [ENHN_LAYER] [iObj]);
			/*************/
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

	/*************/
	for(iObj = 0; iObj<nVO; iObj++)
	{
		fprintf(pfOut, "Complexity.Enable [%d] = %d\n", iObj, !rgbComplexityEstimationDisable [BASE_LAYER] [iObj]);
		if(!rgbComplexityEstimationDisable [BASE_LAYER] [iObj])
		{
			fprintf(pfOut, "Complexity.Opaque.Enable [%d] = %d\n", iObj, rgbOpaque [BASE_LAYER] [iObj]);
			fprintf(pfOut, "Complexity.Transparent.Enable [%d] = %d\n", iObj, rgbTransparent [BASE_LAYER] [iObj]);
			fprintf(pfOut, "Complexity.IntraCAE.Enable [%d] = %d\n", iObj, rgbIntraCAE [BASE_LAYER] [iObj]);
			fprintf(pfOut, "Complexity.InterCAE.Enable [%d] = %d\n", iObj, rgbInterCAE [BASE_LAYER] [iObj]);
			fprintf(pfOut, "Complexity.NoUpdate.Enable [%d] = %d\n", iObj, rgbNoUpdate [BASE_LAYER] [iObj]);
			fprintf(pfOut, "Complexity.UpSampling.Enable [%d] = %d\n", iObj, rgbUpsampling [BASE_LAYER] [iObj]);
			fprintf(pfOut, "Complexity.IntraBlocks.Enable [%d] = %d\n", iObj, rgbIntraBlocks [BASE_LAYER] [iObj]);
			fprintf(pfOut, "Complexity.InterBlocks.Enable [%d] = %d\n", iObj, rgbInterBlocks [BASE_LAYER] [iObj]);
			fprintf(pfOut, "Complexity.Inter4VBlocks.Enable [%d] = %d\n", iObj, rgbInter4vBlocks [BASE_LAYER] [iObj]);
			fprintf(pfOut, "Complexity.NotCodedBlocks.Enable [%d] = %d\n", iObj, rgbNotCodedBlocks [BASE_LAYER] [iObj]);
			fprintf(pfOut, "Complexity.DCTCoefs.Enable [%d] = %d\n", iObj, rgbDCTCoefs [BASE_LAYER] [iObj]);
			fprintf(pfOut, "Complexity.DCTLines.Enable [%d] = %d\n", iObj, rgbDCTLines [BASE_LAYER] [iObj]);
			fprintf(pfOut, "Complexity.VLCSymbols.Enable [%d] = %d\n", iObj, rgbVLCSymbols [BASE_LAYER] [iObj]);
			fprintf(pfOut, "Complexity.VLCBits.Enable [%d] = %d\n", iObj, rgbVLCBits [BASE_LAYER] [iObj]);
			fprintf(pfOut, "Complexity.APM.Enable [%d] = %d\n", iObj, rgbAPM [BASE_LAYER] [iObj]);
			fprintf(pfOut, "Complexity.NPM.Enable [%d] = %d\n", iObj, rgbNPM [BASE_LAYER] [iObj]);
			fprintf(pfOut, "Complexity.InterpMCQ.Enable [%d] = %d\n", iObj, rgbInterpolateMCQ [BASE_LAYER] [iObj]);
			fprintf(pfOut, "Complexity.ForwBackMCQ.Enable [%d] = %d\n", iObj, rgbForwBackMCQ [BASE_LAYER] [iObj]);
			fprintf(pfOut, "Complexity.HalfPel2.Enable [%d] = %d\n", iObj, rgbHalfpel2 [BASE_LAYER] [iObj]);
			fprintf(pfOut, "Complexity.HalfPel4.Enable [%d] = %d\n", iObj, rgbHalfpel4 [BASE_LAYER] [iObj]);

			fprintf(pfOut, "Complexity.EstimationMethod [%d] = 0\n", iObj);
			fprintf(pfOut, "Complexity.SADCT.Enable [%d] = 0\n", iObj);
			fprintf(pfOut, "Complexity.QuarterPel.Enable [%d] = 0\n", iObj);
		}
		if(bAnyScalability)
		{
			fprintf(pfOut, "Complexity.Enable [%d] = %d\n", iObj + nVO, !rgbComplexityEstimationDisable [ENHN_LAYER] [iObj]);
			if(!rgbComplexityEstimationDisable [BASE_LAYER] [iObj])
			{
				fprintf(pfOut, "Complexity.Opaque.Enable [%d] = %d\n", iObj + nVO, rgbOpaque [ENHN_LAYER] [iObj]);
				fprintf(pfOut, "Complexity.Transparent.Enable [%d] = %d\n", iObj + nVO, rgbTransparent [ENHN_LAYER] [iObj]);
				fprintf(pfOut, "Complexity.IntraCAE.Enable [%d] = %d\n", iObj + nVO, rgbIntraCAE [ENHN_LAYER] [iObj]);
				fprintf(pfOut, "Complexity.InterCAE.Enable [%d] = %d\n", iObj + nVO, rgbInterCAE [ENHN_LAYER] [iObj]);
				fprintf(pfOut, "Complexity.NoUpdate.Enable [%d] = %d\n", iObj + nVO, rgbNoUpdate [ENHN_LAYER] [iObj]);
				fprintf(pfOut, "Complexity.UpSampling.Enable [%d] = %d\n", iObj + nVO, rgbUpsampling [ENHN_LAYER] [iObj]);
				fprintf(pfOut, "Complexity.IntraBlocks.Enable [%d] = %d\n", iObj + nVO, rgbIntraBlocks [ENHN_LAYER] [iObj]);
				fprintf(pfOut, "Complexity.InterBlocks.Enable [%d] = %d\n", iObj + nVO, rgbInterBlocks [ENHN_LAYER] [iObj]);
				fprintf(pfOut, "Complexity.Inter4VBlocks.Enable [%d] = %d\n", iObj + nVO, rgbInter4vBlocks [ENHN_LAYER] [iObj]);
				fprintf(pfOut, "Complexity.NotCodedBlocks.Enable [%d] = %d\n", iObj + nVO, rgbNotCodedBlocks [ENHN_LAYER] [iObj]);
				fprintf(pfOut, "Complexity.DCTCoefs.Enable [%d] = %d\n", iObj + nVO, rgbDCTCoefs [ENHN_LAYER] [iObj]);
				fprintf(pfOut, "Complexity.DCTLines.Enable [%d] = %d\n", iObj + nVO, rgbDCTLines [ENHN_LAYER] [iObj]);
				fprintf(pfOut, "Complexity.VLCSymbols.Enable [%d] = %d\n", iObj + nVO, rgbVLCSymbols [ENHN_LAYER] [iObj]);
				fprintf(pfOut, "Complexity.VLCBits.Enable [%d] = %d\n", iObj + nVO, rgbVLCBits [ENHN_LAYER] [iObj]);
				fprintf(pfOut, "Complexity.APM.Enable [%d] = %d\n", iObj + nVO, rgbAPM [ENHN_LAYER] [iObj]);
				fprintf(pfOut, "Complexity.NPM.Enable [%d] = %d\n", iObj + nVO, rgbNPM [ENHN_LAYER] [iObj]);
				fprintf(pfOut, "Complexity.InterpMCQ.Enable [%d] = %d\n", iObj + nVO, rgbInterpolateMCQ [ENHN_LAYER] [iObj]);
				fprintf(pfOut, "Complexity.ForwBackMCQ.Enable [%d] = %d\n", iObj + nVO, rgbForwBackMCQ [ENHN_LAYER] [iObj]);
				fprintf(pfOut, "Complexity.HalfPel2.Enable [%d] = %d\n", iObj + nVO, rgbHalfpel2 [ENHN_LAYER] [iObj]);
				fprintf(pfOut, "Complexity.HalfPel4.Enable [%d] = %d\n", iObj + nVO, rgbHalfpel4 [ENHN_LAYER] [iObj]);

				fprintf(pfOut, "Complexity.EstimationMethod [%d] = 0\n", iObj + nVO);
				fprintf(pfOut, "Complexity.SADCT.Enable [%d] = 0\n", iObj + nVO);
				fprintf(pfOut, "Complexity.QuarterPel.Enable [%d] = 0\n", iObj + nVO);
			}
		}
	}
	/*************/

	// START: VOL Control Parameters
	if(iVersion>813 && iVersion<816)
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
			rguiChromaFormat[BASE_LAYER][iObj] = 0;
			rguiChromaFormat[ENHN_LAYER][iObj] = 0;
			rguiLowDelay[BASE_LAYER][iObj] = 0;
			rguiLowDelay[ENHN_LAYER][iObj] = 0;
			rguiVBVParams[BASE_LAYER][iObj] = 0;
			rguiVBVParams[ENHN_LAYER][iObj] = 0;
			rguiBitRate[BASE_LAYER][iObj] = 0;
			rguiBitRate[ENHN_LAYER][iObj] = 0;
			rguiVbvBufferSize[BASE_LAYER][iObj] = 0;
			rguiVbvBufferSize[ENHN_LAYER][iObj] = 0;
			rguiVbvBufferOccupany[BASE_LAYER][iObj] = 0;
			rguiVbvBufferOccupany[ENHN_LAYER][iObj] = 0;
		}

	}

	for(iObj = 0; iObj<nVO; iObj++)
	{
		/*************/
		fprintf(pfOut, "VOLControl.Enable [%d] = %d\n", iObj, rguiVolControlParameters [BASE_LAYER] [iObj]);
		fprintf(pfOut, "VOLControl.ChromaFormat [%d] = %d\n", iObj, rguiChromaFormat [BASE_LAYER] [iObj]);
		fprintf(pfOut, "VOLControl.LowDelay [%d] = %d\n", iObj, rguiLowDelay [BASE_LAYER] [iObj]);
		fprintf(pfOut, "VOLControl.VBVParams.Enable [%d] = %d\n", iObj, rguiVBVParams [BASE_LAYER] [iObj]);
		fprintf(pfOut, "VOLControl.Bitrate [%d] = %d\n", iObj, rguiBitRate [BASE_LAYER] [iObj]);
		fprintf(pfOut, "VOLControl.VBVBuffer.Size [%d] = %d\n", iObj, rguiVbvBufferSize [BASE_LAYER] [iObj]);
		fprintf(pfOut, "VOLControl.VBVBuffer.Occupancy [%d] = %d\n", iObj, rguiVbvBufferOccupany [BASE_LAYER] [iObj]);
		/*************/
		if(bAnyScalability)
		{
			/*************/
			fprintf(pfOut, "VOLControl.Enable [%d] = %d\n", iObj + nVO, rguiVolControlParameters [ENHN_LAYER] [iObj]);
			fprintf(pfOut, "VOLControl.ChromaFormat [%d] = %d\n", iObj + nVO, rguiChromaFormat [ENHN_LAYER] [iObj]);
			fprintf(pfOut, "VOLControl.LowDelay [%d] = %d\n", iObj + nVO, rguiLowDelay [ENHN_LAYER] [iObj]);
			fprintf(pfOut, "VOLControl.VBVParams.Enable [%d] = %d\n", iObj + nVO, rguiVBVParams [ENHN_LAYER] [iObj]);
			fprintf(pfOut, "VOLControl.Bitrate [%d] = %d\n", iObj + nVO, rguiBitRate [ENHN_LAYER] [iObj]);
			fprintf(pfOut, "VOLControl.VBVBuffer.Size [%d] = %d\n", iObj + nVO, rguiVbvBufferSize [ENHN_LAYER] [iObj]);
			fprintf(pfOut, "VOLControl.VBVBuffer.Occupancy [%d] = %d\n", iObj + nVO, rguiVbvBufferOccupany [ENHN_LAYER] [iObj]);
			/*************/
		}
	}

	// END: VOL Control Parameters

	// Interlaced CHANGE
	// Sequence frame frequency (Hz).  Use double type because of 29.97Hz (etc)
	nextValidLine (pfPara, pnLine);
	for (iObj = 0; iObj < nVO; iObj++)	{
		if (fscanf (pfPara, "%lf", &(rgdFrameFrequency [BASE_LAYER] [iObj])) != 1)	{
			fprintf(stderr, "wrong parameter file format on line %d\n", *pnLine);
			fatal_error("Conversion aborted");
		}
		my_assert(rgdFrameFrequency [BASE_LAYER] [iObj] > 0.0);
		/*************/
		fprintf(pfOut, "Source.FrameRate [%d] = %g\n", iObj, rgdFrameFrequency [BASE_LAYER] [iObj]);
		/*************/
	}
	if (bAnyScalability)	{
		for (iObj = 0; iObj < nVO; iObj++)	{
			if (fscanf (pfPara, "%lf", &(rgdFrameFrequency [ENHN_LAYER] [iObj])) != 1)	{
				fprintf(stderr, "wrong parameter file format on line %d\n", *pnLine);
				fatal_error("Conversion aborted");
			}
			my_assert(rgdFrameFrequency [ENHN_LAYER] [iObj] > 0.0);
			/*************/
			fprintf(pfOut, "Source.FrameRate [%d] = %g\n", iObj + nVO, rgdFrameFrequency [ENHN_LAYER] [iObj]);
			/*************/
		}
	}

	// top field first flag
	nextValidLine (pfPara, pnLine);
	for (iObj = 0; iObj < nVO; iObj++)	{
		if (fscanf (pfPara, "%d", &(rgbTopFieldFirst [BASE_LAYER] [iObj])) != 1)	{
			fprintf(stderr, "wrong parameter file format on line %d\n", *pnLine);
			fatal_error("Conversion aborted");
		}
		my_assert (rgbTopFieldFirst [BASE_LAYER] [iObj] == 0 || 
				rgbTopFieldFirst [BASE_LAYER] [iObj] == 1);
		/*************/
		fprintf(pfOut, "Motion.Interlaced.TopFieldFirst.Enable [%d] = %d\n", iObj, rgbTopFieldFirst [BASE_LAYER] [iObj]);
		/*************/
	}
	if (bAnyScalability)	{
		for (iObj = 0; iObj < nVO; iObj++)	{
			if (fscanf (pfPara, "%d", &(rgbTopFieldFirst [ENHN_LAYER] [iObj])) != 1)	{
				fprintf(stderr, "wrong parameter file format on line %d\n", *pnLine);
				fatal_error("Conversion aborted");
			}
			my_assert (rgbTopFieldFirst [ENHN_LAYER] [iObj] == 0 || 
					rgbTopFieldFirst [ENHN_LAYER] [iObj] == 1);
		/*************/
		fprintf(pfOut, "Motion.Interlaced.TopFieldFirst.Enable [%d] = %d\n", iObj + nVO, rgbTopFieldFirst [ENHN_LAYER] [iObj]);
		/*************/
		}
	}

	// alternate scan flag
	nextValidLine (pfPara, pnLine);
	for (iObj = 0; iObj < nVO; iObj++)	{
		if (fscanf (pfPara, "%d", &(rgbAlternateScan [BASE_LAYER] [iObj])) != 1)	{
			fprintf(stderr, "wrong parameter file format on line %d\n", *pnLine);
			fatal_error("Conversion aborted");
		}
		my_assert (rgbAlternateScan [BASE_LAYER] [iObj] == 0 || 
				rgbAlternateScan [BASE_LAYER] [iObj] == 1);
		/*************/
		fprintf(pfOut, "Motion.Interlaced.AlternativeScan.Enable [%d] = %d\n", iObj, rgbAlternateScan [BASE_LAYER] [iObj]);
		/*************/
	}
	if (bAnyScalability)	{
		for (iObj = 0; iObj < nVO; iObj++)	{
			if (fscanf (pfPara, "%d", &(rgbAlternateScan [ENHN_LAYER] [iObj])) != 1)	{
				fprintf(stderr, "wrong parameter file format on line %d\n", *pnLine);
				fatal_error("Conversion aborted");
			}
			my_assert (rgbAlternateScan [ENHN_LAYER] [iObj] == 0 || 
					rgbAlternateScan [ENHN_LAYER] [iObj] == 1);
			/*************/
			fprintf(pfOut, "Motion.Interlaced.AlternativeScan.Enable [%d] = %d\n", iObj + nVO, rgbAlternateScan [ENHN_LAYER] [iObj]);
			/*************/
		}
	}

	// Direct Mode search radius (half luma pixel units)
	nextValidLine (pfPara, pnLine);
	for (iObj = 0; iObj < nVO; iObj++)	{
		if (fscanf (pfPara, "%d", &(rgiDirectModeRadius [BASE_LAYER] [iObj])) != 1)	{
			fprintf(stderr, "wrong parameter file format on line %d\n", *pnLine);
			fatal_error("Conversion aborted");
		}
		/*************/
		fprintf(pfOut, "Motion.SearchRange.DirectMode [%d] = %d\n", iObj, rgiDirectModeRadius [BASE_LAYER] [iObj]);
		/*************/
	}
	if (bAnyScalability)	{
		for (iObj = 0; iObj < nVO; iObj++)	{
			if (fscanf (pfPara, "%d", &(rgiDirectModeRadius [ENHN_LAYER] [iObj])) != 1)	{
				fprintf(stderr, "wrong parameter file format on line %d\n", *pnLine);
				fatal_error("Conversion aborted");
			}
			/*************/
			fprintf(pfOut, "Motion.SearchRange.DirectMode [%d] = %d\n", iObj + nVO, rgiDirectModeRadius [ENHN_LAYER] [iObj]);
			/*************/
		}
	}

	// motion vector file
	nextValidLine(pfPara, pnLine);
	for (iObj = 0; iObj < nVO; iObj++)	{
		if (fscanf (pfPara, "%d", &(rgiMVFileUsage [BASE_LAYER] [iObj])) != 1)	{
			fprintf(stderr, "wrong parameter file format on line %d\n", *pnLine);
			fatal_error("Conversion aborted");
		}
		my_assert (rgiMVFileUsage [BASE_LAYER] [iObj] == 0 ||	// 0= not used
				rgiMVFileUsage [BASE_LAYER] [iObj] == 1 ||	// 1= read motion vectors from file
				rgiMVFileUsage [BASE_LAYER] [iObj] == 2 );	// 2= write motion vectors to file
		/*************/
		fprintf(pfOut, "Motion.ReadWriteMVs [%d] = \"%s\"\n", iObj,
			rgiMVFileUsage [BASE_LAYER] [iObj]==0 ? "Off" : (rgiMVFileUsage [BASE_LAYER] [iObj]==1 ? "Read" : "Write"));
		/*************/

		pchMVFileName [BASE_LAYER] [iObj] = NULL;
		pchMVFileName [ENHN_LAYER] [iObj] = NULL;
		
		char cFileName[256];
		cFileName[0] = 0;
		if ((rgiMVFileUsage [BASE_LAYER] [iObj] != 0) &&
			(fscanf (pfPara, "%200s", cFileName) != 1))	{
			fprintf(stderr, "wrong parameter file format on line %d\n", *pnLine);
			fatal_error("Conversion aborted");
		}
		Int iLength = strlen(cFileName) + 1;
		pchMVFileName [BASE_LAYER] [iObj] = new char [iLength];
		memcpy(pchMVFileName [BASE_LAYER] [iObj], cFileName, iLength);
		/*************/
		fprintf(pfOut, "Motion.ReadWriteMVs.Filename [%d] = \"%s\"\n", iObj, pchMVFileName [BASE_LAYER] [iObj]);
		/*************/
	}
	if (bAnyScalability) {
		for (iObj = 0; iObj < nVO; iObj++)	{
			if (fscanf (pfPara, "%d", &(rgiMVFileUsage [ENHN_LAYER] [iObj])) != 1)	{
				fprintf(stderr, "wrong parameter file format on line %d\n", *pnLine);
				fatal_error("Conversion aborted");
			}
			my_assert (rgiMVFileUsage [ENHN_LAYER] [iObj] == 0 || 
					rgiMVFileUsage [ENHN_LAYER] [iObj] == 1 ||
					rgiMVFileUsage [ENHN_LAYER] [iObj] == 2 );
			/*************/
			fprintf(pfOut, "Motion.ReadWriteMVs [%d] = \"%s\"\n", iObj + nVO,
				rgiMVFileUsage [ENHN_LAYER] [iObj]==0 ? "Off" : (rgiMVFileUsage [ENHN_LAYER] [iObj]==1 ? "Read" : "Write"));
			/*************/

			char cFileName[256];
			cFileName[0] = 0;
			if ((rgiMVFileUsage [ENHN_LAYER] [iObj] != 0) &&
				(fscanf (pfPara, "%s", cFileName) != 1))	{
				fprintf(stderr, "wrong parameter file format on line %d\n", *pnLine);
				fatal_error("Conversion aborted");
			}
			Int iLength = strlen(cFileName) + 1;
			pchMVFileName [ENHN_LAYER] [iObj] = new char [iLength];
			memcpy(pchMVFileName [ENHN_LAYER] [iObj], cFileName, iLength);
			/*************/
			fprintf(pfOut, "Motion.ReadWriteMVs.Filename [%d] = \"%s\"\n", iObj + nVO, pchMVFileName [ENHN_LAYER] [iObj]);
			/*************/
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

	/*************/
	fprintf(pfOut, "Source.FilePrefix = \"%s\"\n", pchPrefix);
	fprintf(pfOut, "Source.Directory = \"%s\"\n", pchBmpDir);
	/*************/

	// chrominance format
	nextValidLine (pfPara, pnLine);
	for (iObj = 0; iObj < nVO; iObj++)	{
		UInt uiChrType;
		if (fscanf (pfPara, "%d", &uiChrType) != 1)	{
			fprintf(stderr, "wrong parameter file format on line %d\n", *pnLine);
			fatal_error("Conversion aborted");
		}
		else {
			my_assert (uiChrType == 0 || uiChrType == 1 || uiChrType == 2);
			rgfChrType [iObj] = (ChromType) uiChrType;
		}
		/*************/
		fprintf(pfOut, "Source.Format [%d] = \"%s\"\n", iObj,
			uiChrType==0 ? "444" : (uiChrType==1 ? "422" : "420"));
		/*************/
	}

	// output file directory
	pchOutBmpDir = new char [256];
	nextValidLine (pfPara, pnLine);
	fscanf (pfPara, "%100s", pchOutBmpDir);

	// output bitstream file
	pchOutStrFile = new char [256];
	nextValidLine (pfPara, pnLine);
	fscanf (pfPara, "%100s", pchOutStrFile);

	/*************/
	fprintf(pfOut, "Output.Directory.Bitstream = \"%s\"\n", pchOutStrFile);
	fprintf(pfOut, "Output.Directory.DecodedFrames = \"%s\"\n", pchOutBmpDir);
	/*************/

	// statistics dumping options
	nextValidLine (pfPara, pnLine);
	for (iObj = 0; iObj < nVO; iObj++)	{
		if (fscanf (pfPara, "%d", &rgbDumpMB [iObj]) != 1)	{
			fprintf(stderr, "wrong parameter file format on line %d\n", *pnLine);
			fatal_error("Conversion aborted");
		}
		my_assert (rgbDumpMB [iObj] == 0 || rgbDumpMB [iObj] == 1);
		/*************/
		fprintf(pfOut, "Trace.DetailedDump.Enable [%d] = %d\n", iObj, rgbDumpMB [iObj]);
		/*************/
	}

	// trace file options
	nextValidLine (pfPara, pnLine);
	for (iObj = 0; iObj < nVO; iObj++)	{
		if (fscanf (pfPara, "%d", &rgbTrace [iObj]) != 1)	{
			fprintf(stderr, "wrong parameter file format on line %d\n", *pnLine);
			fatal_error("Conversion aborted");
		}
		my_assert (rgbTrace [iObj] == 0 || rgbTrace [iObj] == 1);
		/*************/
		fprintf(pfOut, "Trace.CreateFile.Enable [%d] = %d\n", iObj, rgbTrace [iObj]);
		/*************/
	}

	// sprite info
	// sprite usage
	nextValidLine (pfPara, pnLine);
	for (iObj = 0; iObj < nVO; iObj++)	{
		if (fscanf (pfPara, "%d", &rguiSpriteUsage [iObj]) != 1)	{
			fprintf(stderr, "wrong parameter file format on line %d\n", *pnLine);
			fatal_error("Conversion aborted");
		}
		if (rgbScalability [iObj] == TRUE)
			my_assert (rguiSpriteUsage [iObj] == 0);
		/*************/
		fprintf(pfOut, "Sprite.Type [%d] = \"%s\"\n", iObj, rguiSpriteUsage [iObj]==0 ? "None" : (rguiSpriteUsage [iObj]==1 ? "Static" : "GMC"));
		/*************/
	}

	// warping accuracy
	nextValidLine (pfPara, pnLine);
	for (iObj = 0; iObj < nVO; iObj++)	{
		if (fscanf (pfPara, "%d", &rguiWarpingAccuracy [iObj]) != 1)	{
			fprintf(stderr, "wrong parameter file format on line %d\n", *pnLine);
			fatal_error("Conversion aborted");
		}
		if (rguiSpriteUsage [iObj] != 0)
			my_assert (rguiWarpingAccuracy [iObj] == 0 || rguiWarpingAccuracy [iObj] == 1 || rguiWarpingAccuracy [iObj] == 2 || rguiWarpingAccuracy [iObj] == 3);
		/*************/
		fprintf(pfOut, "Sprite.WarpAccuracy [%d] = \"1/%d\"\n", iObj, 1<<(rguiWarpingAccuracy [iObj]+1));
		/*************/
	}

	// number of points
	nextValidLine (pfPara, pnLine);
	for (iObj = 0; iObj < nVO; iObj++)	{
		if (fscanf (pfPara, "%d", &rgiNumPnts [iObj]) != 1)	{
			fprintf(stderr, "wrong parameter file format on line %d\n", *pnLine);
			fatal_error("Conversion aborted");
		}
		if (rgbScalability [iObj] == TRUE)
			my_assert (rgiNumPnts [iObj] == -1);
		if (rguiSpriteUsage [iObj] == 1) 
			my_assert (rgiNumPnts [iObj] == 0 ||
					rgiNumPnts [iObj] == 1 ||
					rgiNumPnts [iObj] == 2 ||
					rgiNumPnts [iObj] == 3 ||
					rgiNumPnts [iObj] == 4);
		/*************/
		fprintf(pfOut, "Sprite.Points [%d] = %d\n", iObj, rgiNumPnts [iObj]);
		/*************/
	}

	// sprite directory
	pchSptDir = new char [256];
	nextValidLine (pfPara, pnLine);
	fscanf (pfPara, "%100s", pchSptDir);

	// point directory
	pchSptPntDir = new char [256];
	nextValidLine (pfPara, pnLine);
	fscanf (pfPara, "%100s", pchSptPntDir);

	/*************/
	fprintf(pfOut, "Sprite.Directory = \"%s\"\n", pchSptDir);
	fprintf(pfOut, "Sprite.Points.Directory = \"%s\"\n", pchSptPntDir);
	/*************/

	//  sprite reconstruction mode, i.e. Low-latency-sprite-enable
	nextValidLine (pfPara, pnLine);
	for (iObj = 0; iObj < nVO; iObj++)
	{
		UInt uiSptMode;
		if (fscanf (pfPara, "%d", &uiSptMode) != 1)	{
			fprintf(stderr, "wrong parameter file format on line %d\n", *pnLine);
			fatal_error("Conversion aborted");
		}
		else {
			my_assert (uiSptMode == 0 || uiSptMode == 1 );
			rgSpriteMode[iObj] = (SptMode) uiSptMode;
		}
		/*************/
		fprintf(pfOut, "Sprite.Mode [%d] = \"%s\"\n", iObj,
			uiSptMode==0 ? "Basic" : (uiSptMode==1 ? "LowLatency" : (uiSptMode==2 ? "PieceObject" : "PieceUpdate")));
		/*************/
	}

	if(iVersion>815)
	{
		// RRV insertion
		nextValidLine (pfPara, pnLine);
		for (iObj = 0; iObj < nVO; iObj++)
		{
			if(fscanf(pfPara, "%d", &RRVmode[BASE_LAYER][iObj].iOnOff) != 1)
			{
				fprintf(stderr, "wrong parameter file format on line %d\n", *pnLine);
				exit (1);
			}
			if(fscanf(pfPara, "%d", &RRVmode[BASE_LAYER][iObj].iCycle) != 1)
			{
				fprintf(stderr, "wrong parameter file format on line %d\n", *pnLine);
				exit (1);
			}
			
			RRVmode[ENHN_LAYER][iObj] = RRVmode[BASE_LAYER][iObj];
		}
		// ~RRV
	}
	else
	{
		for (iObj = 0; iObj < nVO; iObj++)
		{
			RRVmode[BASE_LAYER][iObj].iOnOff = 0;
			RRVmode[BASE_LAYER][iObj].iCycle = 0;
			RRVmode[ENHN_LAYER][iObj] = RRVmode[BASE_LAYER][iObj];
		}
	}

	for (iObj = 0; iObj < nVO; iObj++)
	{
		/*************/
		fprintf(pfOut, "RRVMode.Enable [%d] = %d\n", iObj, RRVmode[BASE_LAYER][iObj].iOnOff);
		fprintf(pfOut, "RRVMode.Cycle [%d] = %d\n", iObj, RRVmode[BASE_LAYER][iObj].iCycle);
		/*************/
		if(bAnyScalability)
		{
			/*************/
			fprintf(pfOut, "RRVMode.Enable [%d] = %d\n", iObj + nVO, RRVmode[ENHN_LAYER][iObj].iOnOff);
			fprintf(pfOut, "RRVMode.Cycle [%d] = %d\n", iObj + nVO, RRVmode[ENHN_LAYER][iObj].iCycle);
			/*************/
		}
	}

	fclose(pfPara);	
	if(argc==3)
		fclose(pfOut);

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

		// version 2
		for (iObj = 0; iObj < nVO; iObj++) {
			if(rgcNewpredSlicePoint[iLayer][iObj]!=NULL)
				delete rgcNewpredSlicePoint[iLayer][iObj];
			if(rgcNewpredRefName[iLayer][iObj]!=NULL)
				delete rgcNewpredRefName[iLayer][iObj];
		}

		delete rgcNewpredSlicePoint [iLayer];
		delete rgcNewpredRefName [iLayer];
		delete rgbNewpredSegmentType [iLayer];
		delete rgbNewpredEnable [iLayer];
		
		delete rgbSadctDisable [iLayer];
		delete rgbQuarterSample [iLayer];
		delete RRVmode [iLayer];
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

	delete rguiVerID;

	return 0;
}


Void nextValidLine (FILE *pfPara, UInt* pnLine)	{

	Int chBoL = 0;
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


Void readBoolVOLFlag (Bool * rgbTable [2], UInt nVO, FILE * pfCfg, UInt * pnLine, Bool bAnyScalability)
{
	nextValidLine (pfCfg, pnLine);
	for (UInt i = 0; i < nVO; ++i) {
		if (fscanf (pfCfg, "%d", & rgbTable [BASE_LAYER] [i]) != 1) {
			fprintf(stderr, "wrong parameter file format on line %d\n", *pnLine);
			fatal_error("Conversion aborted");
		}
		my_assert (rgbTable [BASE_LAYER] [i] == 0 || rgbTable [BASE_LAYER] [i] == 1);
	}
	if (bAnyScalability) {
		for (UInt i = 0; i < nVO; ++i)	{
			if (fscanf (pfCfg, "%d", & rgbTable [ENHN_LAYER] [i]) != 1)	{
				fprintf(stderr, "wrong parameter file format on line %d\n", *pnLine);
				fatal_error("Conversion aborted");
			}
			my_assert (rgbTable [ENHN_LAYER] [i] == 0 || rgbTable [ENHN_LAYER] [i] == 1);
		}
	}
}

Void readItem(UInt *rguiTable [2], UInt nVO, FILE * pfCfg, UInt * pnLine, Bool bAnyScalability)
{
	nextValidLine (pfCfg, pnLine);
	for (UInt i = 0; i < nVO; ++i) {
		if (fscanf (pfCfg, "%d", & rguiTable [BASE_LAYER] [i]) != 1) {
			fprintf(stderr, "wrong parameter file format on line %d\n", *pnLine);
			fatal_error("Conversion aborted");
		}
	}
	if (bAnyScalability) 
	{
		for (UInt i = 0; i < nVO; ++i)	{
			if (fscanf (pfCfg, "%d", & rguiTable [ENHN_LAYER] [i]) != 1)	{
				fprintf(stderr, "wrong parameter file format on line %d\n", *pnLine);
				fatal_error("Conversion aborted");
			}
		}
	}
}

Void fatal_error(char *pchError, Bool bFlag)
{
	if(bFlag)
		return;

	fprintf(stderr, "Error: %s\n", pchError);
	exit(1);
}

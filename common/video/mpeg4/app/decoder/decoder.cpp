/*************************************************************************

This software module was originally developed by 

	Ming-Chieh Lee (mingcl@microsoft.com), Microsoft Corporation
	Wei-ge Chen (wchen@microsoft.com), Microsoft Corporation
	Bruce Lin (blin@microsoft.com), Microsoft Corporation
	Chuang Gu (chuanggu@microsoft.com), Microsoft Corporation
	(date: March, 1996)

and edited by

	Hiroyuki Katata (katata@imgsl.mkhar.sharp.co.jp), Sharp Corporation
	Norio Ito (norio@imgsl.mkhar.sharp.co.jp), Sharp Corporation
	Shuichi Watanabe (watanabe@imgsl.mkhar.sharp.co.jp), Sharp Corporation
	(date: October, 1997)

and also edited by
	Dick van Smirren (D.vanSmirren@research.kpn.com), KPN Research
	Cor Quist (C.P.Quist@research.kpn.com), KPN Research
	(date: July, 1998)

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

	tps_decoder.cpp

Abstract:

	caller for decoder for temporal scalability

Revision History:

*************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <iostream.h>
#include <fstream.h>

#ifdef __PC_COMPILER_
#include <windows.h>
#include <mmsystem.h>
#endif // __PC_COMPILER_

#include "typeapi.h"
#include "mode.hpp"
#include "vopses.hpp"
#include "entropy/bitstrm.hpp"
#include "tps_enhcbuf.hpp"
#include "decoder/enhcbufdec.hpp"
#include "decoder/vopsedec.hpp"

#include "dataStruct.hpp"

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

Void dumpFrame (const CVOPU8YUVBA* pvopcQuant, FILE* pfYUV, FILE* pfSeg, AlphaUsage, CRct& rct, UInt nBits, Int iAlphaScale, Int DumpSkip);


#define MAX_TIME	99999			// for bitstreams stopped at BVOP
#define USAGE "Usage: \tDECODER bitstream_file [enhn_layer_bitstream_file] output_file width height\n\
or:    \tDECODER -vtc bitstream_file output_file target_spatial_layer target_snr_layer\n"

// Main Routine
int main(int argc, char* argv[])
{
	Bool bScalability = 0;
	Bool bWavelet = 0;

	Bool main_short_video_header; // Added by KPN for short headers [FDS]
	
	main_short_video_header = FALSE; 

	if(argc>1 && strcmp(argv[1],"-vtc")==0)
	{
		bWavelet = 1;
		if(argc != 6)
		{
			fprintf(stderr,USAGE);
			exit(1);
		}
	}
	else
	{
		if (argc < 5 || argc > 6) {
			fprintf (stderr,USAGE);
			exit(1);
		}
		else if (argc == 6)
			bScalability = 1;
	}


	FILE *pfBits;
	if ((pfBits = fopen (argv [bWavelet + 1], "rb")) == NULL )
		fatal_error("Bitstream File Not Found");
	fclose (pfBits);
	if (bScalability) {
		if ((pfBits = fopen (argv [2], "rb")) == NULL )
			fatal_error("Bitstream File for Enhancement Layer Not Found\n");
		fclose (pfBits);
	}


	if(bWavelet)
	{
	/////  WAVELET VTC: begin   /////////////////////////
		CVTCDecoder vtcdec;

		// start VTC decoding 
		// argv[1]: "-vtc"
		// argv[2]: bitstream file
		// argv[3]: decoded image file
		// argv[4]: target spatial layer
		// argv[5]: target SNR layer
		vtcdec.decode(argv[2], argv[3], atoi(argv[4]), atoi(argv[5]) );

		return 0;
	/////// WAVELET VTC: end   /////////////////////////
	}

	CRct rctDisplay       (0, 0, atoi (argv [3 + bScalability]), atoi (argv [4 + bScalability]));
/*Added */
	CRct rctDisplay_SSenh (0, 0, atoi (argv [3 + bScalability]), atoi (argv [4 + bScalability]));

	Bool bTemporalScalability = 0;

	if (bScalability){
		CVideoObjectDecoder* pvodec_tps[2];
		cout << "Checking scalability type...\n";
		pvodec_tps[BASE_LAYER] = new CVideoObjectDecoder (argv[1], rctDisplay.width, rctDisplay.height (), NULL, &main_short_video_header);
		pvodec_tps[ENHN_LAYER] = new CVideoObjectDecoder (argv[2], rctDisplay.width, rctDisplay.height (), NULL, &main_short_video_header);
		pvodec_tps[BASE_LAYER]->setClockRateScale( pvodec_tps[ENHN_LAYER] ); // added by Sharp (98/6/26)
		Time FirstBase = pvodec_tps[BASE_LAYER] -> senseTime ();
		Time FirstEnhn = pvodec_tps[ENHN_LAYER] -> senseTime ();

		if ( FirstBase != FirstEnhn ){
			bTemporalScalability = 1;
			cout << "Starting temporal scalability decoding...\n\n";
		}
		else{
			bTemporalScalability = 0;
			cout << "Starting spatial scalability decoding...\n\n";
		}
		
		delete pvodec_tps[BASE_LAYER];
		delete pvodec_tps[ENHN_LAYER];
	}

	if ( !bScalability || !bTemporalScalability ){
		CVideoObjectDecoder* pvodec [2];
		Bool bSpatialScalable = FALSE;
		//CRct rctDisplayBackup = rctDisplay; // for recovery from sprite decoding

		pvodec [BASE_LAYER] = new CVideoObjectDecoder (argv [1], rctDisplay.width, rctDisplay.height(), &bSpatialScalable, &main_short_video_header);
	
		// NBIT: get nBits information
		UInt nBits = pvodec [BASE_LAYER]->volmd ().nBits;

		/* Added*/
		if (bScalability){
			pvodec [ENHN_LAYER] = new CVideoObjectDecoder (argv [2], rctDisplay.width, rctDisplay.height(), &bSpatialScalable, &main_short_video_header);
			/*Added*/
			if(bSpatialScalable)
				pvodec[ENHN_LAYER]->set_enh_display_size(rctDisplay, &rctDisplay_SSenh);
		}
		if (pvodec [BASE_LAYER] -> fSptUsage () == 1) {
				// decode the initial sprite: //wchen: instead of calling decode ()
			pvodec [BASE_LAYER] -> decodeInitSprite ();
		}

		FILE* pfReconYUV [2];
		Char pchTmp [100];
		sprintf (pchTmp, "%s.yuv", argv [2 + bScalability]);
		pfReconYUV [BASE_LAYER] = fopen (pchTmp, "wb");
		fatal_error("cant open output yuv file",pfReconYUV [BASE_LAYER] != NULL);
		if (bScalability)	{
			sprintf (pchTmp, "%s_e.yuv", argv [2 + bScalability]);
			pfReconYUV [ENHN_LAYER]= fopen (pchTmp, "wb");
			fatal_error("cant open enhancement layer output yuv file",pfReconYUV [ENHN_LAYER] != NULL);
		}
		sprintf (pchTmp, "%s.seg", argv [2 + bScalability]);
		FILE* pfReconSeg = NULL;
		if (pvodec [BASE_LAYER]-> volmd ().fAUsage != RECTANGLE)	{
			pfReconSeg = fopen (pchTmp, "wb");
			fatal_error("cant open output seg file",pfReconSeg != NULL);
		}

		Int iEof = 1;
		Int nFrames = 0;
		const CVOPU8YUVBA* pvopcQuant;
#ifdef __PC_COMPILER_
		Int tickBegin = ::GetTickCount ();
#endif // __PC_COMPILER_
		Bool bCachedRefFrame = FALSE;
		Bool bCachedRefFrameCoded = FALSE;
		Bool didntreadheader = FALSE;
		while (iEof != EOF)
		{
			if (main_short_video_header) // Added by KPN for short headers
			{
				fprintf(stderr,"Frame number: %d\n", nFrames);
				iEof = pvodec [BASE_LAYER] -> h263_decode (didntreadheader);
				didntreadheader = TRUE;
			}
			else
				iEof = pvodec [BASE_LAYER] -> decode ();  
			if (iEof != EOF)
				nFrames++;
			if(pvodec [BASE_LAYER] -> fSptUsage () == 1)
			{
				// sprite
				if(iEof != EOF)
				{
					pvopcQuant = pvodec [BASE_LAYER]->pvopcReconCurr();
					dumpFrame (pvopcQuant, pfReconYUV [BASE_LAYER], pfReconSeg, pvodec[BASE_LAYER]->volmd().fAUsage, rctDisplay,
						nBits, pvodec [BASE_LAYER]->vopmd().iVopConstantAlphaValue, 0);
				}
			}
			else
			{
				if(iEof == EOF)
				{
					// dump final cached frame if present
					if(bCachedRefFrame)
					{
						bCachedRefFrame = FALSE;
						if(bCachedRefFrameCoded)
						{
							pvopcQuant = pvodec [BASE_LAYER]->pvopcRefQLater(); // future reference
							dumpFrame (pvopcQuant, pfReconYUV [BASE_LAYER], pfReconSeg, pvodec[BASE_LAYER]->volmd().fAUsage, rctDisplay,
								nBits, pvodec [BASE_LAYER]->vopmd().iVopConstantAlphaValue, 0);
						}
						else // non coded
							dumpNonCodedFrame(pfReconYUV [BASE_LAYER], pfReconSeg, rctDisplay, nBits);
					}
				}
				else
				{
					// dump if bvop
					if(pvodec[BASE_LAYER]->vopmd().vopPredType == BVOP)
					{
						// BVOP
						if(iEof != FALSE)
						{
							pvopcQuant = pvodec [BASE_LAYER]->pvopcReconCurr(); // current vop
							dumpFrame (pvopcQuant, pfReconYUV [BASE_LAYER], pfReconSeg, pvodec[BASE_LAYER]->volmd().fAUsage, rctDisplay,
								nBits, pvodec [BASE_LAYER]->vopmd().iVopConstantAlphaValue, 0);
						}
						else // non coded BVOP
							dumpNonCodedFrame(pfReconYUV [BASE_LAYER], pfReconSeg, rctDisplay, nBits);
					}
					else
					{
						// not a BVOP, so dump any previous cached frame
						if(bCachedRefFrame)
						{
							bCachedRefFrame = FALSE;
							if(bCachedRefFrameCoded)
							{
								pvopcQuant = pvodec [BASE_LAYER]->pvopcRefQPrev(); // past reference
								dumpFrame (pvopcQuant, pfReconYUV [BASE_LAYER], pfReconSeg, pvodec[BASE_LAYER]->volmd().fAUsage, rctDisplay,
									nBits, pvodec [BASE_LAYER]->vopmd().iVopConstantAlphaValue, 0);
							}
							else // non coded
								dumpNonCodedFrame(pfReconYUV [BASE_LAYER], pfReconSeg, rctDisplay, nBits);
						}

						// cache current reference
						bCachedRefFrame = TRUE;
						bCachedRefFrameCoded = (iEof != FALSE);
					}
				}
			}

			if (bSpatialScalable == TRUE) {
				// if base layer is non coded, pvopcQuant=NULL, but enh layer should also be non coded.
				if(iEof == FALSE)
					pvopcQuant = NULL;
				else
					pvopcQuant = pvodec [BASE_LAYER]->pvopcReconCurr();
				iEof = pvodec [ENHN_LAYER] -> decode (pvopcQuant);
				if(iEof!=EOF)
					nFrames++; // include enhancement layer
				if(iEof==FALSE)
					dumpNonCodedFrame(pfReconYUV [ENHN_LAYER], NULL, rctDisplay_SSenh, nBits);
/*					dumpNonCodedFrame(pfReconYUV [ENHN_LAYER], NULL, rctDisplay * 2, nBits);*/

				if (iEof != EOF && iEof!=FALSE)
/*Added*/
					dumpFrame (pvodec [ENHN_LAYER]->pvopcReconCurr (), pfReconYUV [ENHN_LAYER], NULL, RECTANGLE, rctDisplay_SSenh,
						nBits, pvodec [BASE_LAYER]->vopmd().iVopConstantAlphaValue, 0); // still base layer
/*
					dumpFrame (pvodec [ENHN_LAYER]->pvopcReconCurr (), pfReconYUV [ENHN_LAYER], NULL, RECTANGLE, rctDisplay * 2,
						nBits, pvodec [BASE_LAYER]->vopmd().iVopConstantAlphaValue, 0); // still base layer
*/
			}
		}

#ifdef __PC_COMPILER_
		Int tickAfter = ::GetTickCount ();
		printf ("Total time: %d\n", tickAfter - tickBegin);
		Double dAverage = (Double) (tickAfter - tickBegin) / (Double) (nFrames);
		printf ("Total frames: %d\tAverage time: %.6lf\n", nFrames, dAverage);
		printf ("FPS %.6lf\n", 1000.0 / dAverage);
#endif // __PC_COMPILER_

		fclose (pfReconYUV [BASE_LAYER]);
		if (pvodec [BASE_LAYER]->volmd ().fAUsage != RECTANGLE)
			fclose (pfReconSeg);
		delete pvodec [BASE_LAYER];
		if (bScalability)	{
			fclose (pfReconYUV [ENHN_LAYER]);
			delete pvodec [ENHN_LAYER];
		}

	}
	else { // loop for temporal scalability
		CVideoObjectDecoder* pvodec[2];
		pvodec[BASE_LAYER] = new CVideoObjectDecoder (argv[1], rctDisplay.width, rctDisplay.height (), NULL, &main_short_video_header); // modified by Sharp (98/7/16)

		UInt nBits = pvodec [BASE_LAYER]->volmd ().nBits; // added by Sharp (98/11/11)
		if(bScalability){
			pvodec[ENHN_LAYER] = new CVideoObjectDecoder (argv[2], rctDisplay.width, rctDisplay.height (), NULL, &main_short_video_header); // modified by Sharp (98/7/16)
			// for back/forward shape
			pvodec[ENHN_LAYER]->rgpbfShape[0] = new CVideoObjectDecoder (rctDisplay.width, rctDisplay.height ());
			pvodec[ENHN_LAYER]->rgpbfShape[1] = new CVideoObjectDecoder (rctDisplay.width, rctDisplay.height ());
			pvodec[BASE_LAYER]->setClockRateScale( pvodec[ENHN_LAYER] );
			// copy pointers
			pvodec[ENHN_LAYER]->copyTobfShape ();
		}

		Char pchTmp [100];
		FILE* pfReconYUV [2]; // following two lines are swapped by Sharp (98/10/26)
#ifndef __OUT_ONE_FRAME_
		sprintf (pchTmp, "%s.yuv", argv [2 + bScalability]);
		pfReconYUV [BASE_LAYER] = fopen (pchTmp, "wb");
		fatal_error("cant open output yuv file",pfReconYUV [BASE_LAYER] != NULL);
		if (bScalability) {
//			sprintf (pchTmp, "%s_e.yuv", argv [2 + bScalability]); // deleted by Sharp (98/10/26)
//			pfReconYUV [ENHN_LAYER]= fopen (pchTmp, "wb"); // deleted by Sharp (98/10/26)
			pfReconYUV [ENHN_LAYER] = pfReconYUV [BASE_LAYER]; // added by Sharp (98/10/26)
			fatal_error("cant open enhancement layer output yuv file",pfReconYUV [ENHN_LAYER] != NULL);
		}
		sprintf (pchTmp, "%s.seg", argv [2 + bScalability]);
		FILE* pfReconSeg [2];
		if (pvodec [BASE_LAYER]-> volmd ().fAUsage != RECTANGLE) {
			pfReconSeg [BASE_LAYER] = fopen (pchTmp, "wb");
			fatal_error("cant open output seg file",pfReconSeg [BASE_LAYER] != NULL);
		}
		if (bScalability) {
			if (pvodec [ENHN_LAYER]-> volmd ().fAUsage != RECTANGLE) {
				if (pvodec [ENHN_LAYER]-> volmd ().iEnhnType == 1) { // added by Sharp (98/10/26)
					sprintf (pchTmp, "%s_e.seg", argv [2 + bScalability]);
					pfReconSeg [ENHN_LAYER]= fopen (pchTmp, "wb");
				} else // added by Sharp (98/10/26)
					pfReconSeg [ENHN_LAYER]= pfReconSeg [BASE_LAYER]; // added by Sharp (98/10/26)
				fatal_error("cant open output enhancement layer seg file",pfReconSeg [ENHN_LAYER] != NULL);
			}
		}

// begin: deleted by Sharp (98/10/26)
//		if (bScalability) 
//			if (pvodec [ENHN_LAYER]-> volmd ().iEnhnType == 1) {
//				FILE *pfTemp;
//				sprintf (pchTmp, "%s_bgc.yuv", argv [2 + bScalability]);
//				pfTemp = fopen (pchTmp, "wb");			 // clear file pointer for background composition
//				fclose(pfTemp);
//			}
// end: delted by Sharp (98/10/26)
#endif
		// for back/forward shape output 
		FILE* pfTestSeg = NULL;
		if (bScalability)
			if (pvodec [ENHN_LAYER]-> volmd ().iEnhnType == 1) {
				sprintf (pchTmp, "%s.bfseg", argv [2 + bScalability]);
				pfTestSeg = fopen (pchTmp, "wb"); //reconstructed Seg file(bfShape)
				fatal_error("cant open output bfseg file",pfTestSeg!=NULL);
			}

		int iEof = 1;
#ifdef __PC_COMPILER_
		Int tickBegin = ::GetTickCount ();
#endif // __PC_COMPILER_
		Time tPvopBase, tNextBase, tNextEnhc = 0;
		int nBaseFrames = 0;
		int nEnhcFrames = 0;
		Bool bCacheRefFrame = FALSE; // added by Sharp (98/11/18)
		Bool bCachedRefFrameCoded = FALSE; // added by Sharp (99/1/28)

		tNextBase = pvodec[BASE_LAYER] -> senseTime();
		while (iEof != EOF) {

			if (pvodec[BASE_LAYER] -> getPredType () != BVOP) {			// for bitstreams stopped at BVOP
				iEof = pvodec[BASE_LAYER] -> decode ();
				if(iEof == EOF)
					break;
				if(bScalability)
					pvodec[BASE_LAYER] -> updateBuffVOPsBase (pvodec[ENHN_LAYER]);
				nBaseFrames ++;

				// Output BASE_LAYER
#ifndef __OUT_ONE_FRAME_
// begin: added by Sharp (98/11/11)
				if ( bCacheRefFrame ) {  // modified by by Sharp (98/11/18)
					if ( bCachedRefFrameCoded ) // added by Sharp (99/1/28)
						dumpFrame (pvodec[BASE_LAYER]->pvopcRefQPrev() , pfReconYUV [BASE_LAYER], pfReconSeg[BASE_LAYER], pvodec[BASE_LAYER]->volmd().fAUsage, rctDisplay,
							nBits, pvodec [BASE_LAYER]->vopmd().iVopConstantAlphaValue,  0 );
					else // added by Sharp (99/1/28)
						dumpNonCodedFrame( pfReconYUV[BASE_LAYER], pfReconSeg[BASE_LAYER], rctDisplay, nBits); // added by Sharp (99/1/28)
				}
				// begin: added by Sharp (98/11/18)
				else {
					bCacheRefFrame = TRUE;
				}
				// end: added by Sharp (98/11/18)
// end: added by Sharp (98/11/11)
//				pvodec [BASE_LAYER]->dumpDataAllFrame (pfReconYUV [BASE_LAYER], pfReconSeg [BASE_LAYER], rctDisplay); // deleted by Sharp (98/11/11)
#else
				pvodec [BASE_LAYER]->dumpDataOneFrame(argv, bScalability, rctDisplay);
#endif

				bCachedRefFrameCoded = (iEof != FALSE); // added by Sharp(99/1/28)
				tPvopBase = pvodec[BASE_LAYER] -> getTime ();
			}
			else {	// for bitstreams stopped at BVOP
				tPvopBase = MAX_TIME;
				pvodec[BASE_LAYER] -> copyRefQ1ToQ0 ();
			}

			tNextBase = pvodec[BASE_LAYER] -> senseTime ();
			while((tPvopBase > tNextBase) && (tNextBase != EOF)) {
					iEof = pvodec[BASE_LAYER] -> decode ();
					if(bScalability)
						pvodec[BASE_LAYER] -> updateBuffVOPsBase (pvodec[ENHN_LAYER]);

					// Output BASE_LAYER
					if (iEof != EOF) {
						nBaseFrames ++; 
// begin: deleted by Sharp (98/11/11)
// #ifndef __OUT_ONE_FRAME_
// 						pvodec [BASE_LAYER]->dumpDataAllFrame (pfReconYUV [BASE_LAYER], pfReconSeg [BASE_LAYER], rctDisplay); // deleted by Sharp (98/11/11)
// #else
// end: deleted by Sharp (98/11/11)

#ifdef __OUT_ONE_FRAME_ // added by Sharp (98/11/11)
						pvodec [BASE_LAYER]->dumpDataOneFrame(argv, bScalability, rctDisplay);
#endif
					}

				if(bScalability && tNextEnhc != EOF) {
					tNextEnhc = pvodec[ENHN_LAYER] -> senseTime ();
					while((tNextBase > tNextEnhc) && (tNextEnhc != EOF)) {
						iEof = pvodec[ENHN_LAYER] -> decode ();
						nEnhcFrames ++;
						if (pvodec [ENHN_LAYER] -> volmd ().fAUsage != RECTANGLE) {
							// for background composition
							if(pvodec [ENHN_LAYER] -> volmd ().iEnhnType == 1) { 
								// should be changed to background_composition flag later.
								printf("============== background composition (1)\n");
								pvodec [ENHN_LAYER] -> BackgroundComposition(argv, bScalability, rctDisplay.width, rctDisplay.height (), pfReconYUV[ENHN_LAYER]); // modified by Sharp (98/10/26)
							}
							// for back/forward shape
							if(pvodec [ENHN_LAYER] -> vopmd ().iLoadBakShape) {
								printf("---------- output backward shape\n");
								const CVOPU8YUVBA* pvopcQuant = pvodec [ENHN_LAYER]->rgpbfShape[0]->pvopcReconCurr ();
								pvopcQuant->getPlane (BY_PLANE)->dump (pfTestSeg, rctDisplay);
							}
							if(pvodec [ENHN_LAYER] -> vopmd ().iLoadForShape) {
								printf("---------- output forward shape\n");
								const CVOPU8YUVBA* pvopcQuant = pvodec [ENHN_LAYER]->rgpbfShape[1]->pvopcReconCurr ();
								pvopcQuant->getPlane (BY_PLANE)->dump (pfTestSeg, rctDisplay);
							}
						}

					// Output ENHN_LAYER
						if (iEof != EOF) {
//							if ( pvodec [ENHN_LAYER] -> volmd ().iEnhnType != 1 ){ // added by Sharp (98/10/26) //deleted by Sharp (99/1/25)
#ifndef __OUT_ONE_FRAME_
// begin: added by Sharp (99/1/28)
							if ( iEof != FALSE ){
								dumpFrame (pvodec[ENHN_LAYER]->pvopcReconCurr(), pfReconYUV [ENHN_LAYER], pfReconSeg[ENHN_LAYER], pvodec[ENHN_LAYER]->volmd().fAUsage, rctDisplay,
									nBits, pvodec [ENHN_LAYER]->vopmd().iVopConstantAlphaValue,  pvodec[ENHN_LAYER]->volmd().iEnhnType );
							}
							else
								dumpNonCodedFrame(pfReconYUV[ENHN_LAYER], pfReconSeg[ENHN_LAYER], rctDisplay, nBits);
// end: added by Sharp (99/1/28)
//							pvodec [ENHN_LAYER]->dumpDataAllFrame (pfReconYUV [ENHN_LAYER], pfReconSeg [ENHN_LAYER], rctDisplay); // deleted by Sharp (99/1/28)
#else
							pvodec [ENHN_LAYER]->dumpDataOneFrame(argv, bScalability, rctDisplay);
#endif
//							} // added by Sharp (98/10/26) // deleted by Sharp (99/1/25)
						}

						tNextEnhc = pvodec[ENHN_LAYER] -> senseTime ();
					}
				}
// begin: added by Sharp (98/11/11)
#ifndef __OUT_ONE_FRAME_
				if ( iEof != FALSE )
				dumpFrame (pvodec[BASE_LAYER]->pvopcReconCurr() , pfReconYUV [BASE_LAYER], pfReconSeg[BASE_LAYER], pvodec[BASE_LAYER]->volmd().fAUsage, rctDisplay,
					nBits, pvodec [BASE_LAYER]->vopmd().iVopConstantAlphaValue,  0 );
				else
					dumpNonCodedFrame(pfReconYUV[BASE_LAYER], pfReconSeg[BASE_LAYER], rctDisplay, nBits);
#endif
// end: added by Sharp (98/11/11)
				tNextBase = pvodec[BASE_LAYER] -> senseTime ();
			}


			if(bScalability && (tNextEnhc = pvodec[ENHN_LAYER] -> senseTime ()) != EOF) { // modified by Sharp (99/1/25)
				pvodec[ENHN_LAYER] -> bufferB2flush ();

//				tNextEnhc = pvodec[ENHN_LAYER] -> senseTime (); // deleted by Sharp (99/1/25)
				while((tPvopBase > tNextEnhc) && (tNextEnhc != EOF)) {
					iEof = pvodec[ENHN_LAYER] -> decode ();
					nEnhcFrames ++;
					if (pvodec [ENHN_LAYER] -> volmd ().fAUsage != RECTANGLE) {
						// for background composition
						if(pvodec [ENHN_LAYER] -> volmd ().iEnhnType == 1) {
							// should be changed to background_composition flag later.
							printf("============== background composition (2)\n");
							pvodec [ENHN_LAYER] -> BackgroundComposition(argv, bScalability, rctDisplay.width, rctDisplay.height (), pfReconYUV [ENHN_LAYER]); // modified by Sharp (98/10/26)
						}
						// for back/forward shape
						if(pvodec [ENHN_LAYER] -> vopmd ().iLoadBakShape) {
							printf("---------- output backward shape\n");
							const CVOPU8YUVBA* pvopcQuant = pvodec [ENHN_LAYER]->rgpbfShape[0]->pvopcReconCurr ();
							pvopcQuant->getPlane (BY_PLANE)->dump (pfTestSeg, rctDisplay);
						}
						if(pvodec [ENHN_LAYER] -> vopmd ().iLoadForShape) {
							printf("---------- output forward shape\n");
							const CVOPU8YUVBA* pvopcQuant = pvodec [ENHN_LAYER]->rgpbfShape[1]->pvopcReconCurr ();
							pvopcQuant->getPlane (BY_PLANE)->dump (pfTestSeg, rctDisplay);
						}
					}

				// Output ENHN_LAYER
					if (iEof != EOF) {
//						if ( pvodec [ENHN_LAYER] -> volmd ().iEnhnType != 1 ){ // added by Sharp (98/11/18) // deleted by Sharp (99/1/25)
#ifndef __OUT_ONE_FRAME_
// begin: added by Sharp (99/1/28)
							if ( iEof != FALSE )
								dumpFrame (pvodec[ENHN_LAYER]->pvopcReconCurr(), pfReconYUV [ENHN_LAYER], pfReconSeg[ENHN_LAYER], pvodec[ENHN_LAYER]->volmd().fAUsage, rctDisplay,
									nBits, pvodec [ENHN_LAYER]->vopmd().iVopConstantAlphaValue,  pvodec[ENHN_LAYER]->volmd().iEnhnType );
							else
								dumpNonCodedFrame(pfReconYUV[ENHN_LAYER], pfReconSeg[ENHN_LAYER], rctDisplay, nBits);
// end: added by Sharp (99/1/28)
//							pvodec [ENHN_LAYER]->dumpDataAllFrame (pfReconYUV [ENHN_LAYER], pfReconSeg [ENHN_LAYER], rctDisplay); // deleted by Sharp (99/1/28)
#else
							pvodec [ENHN_LAYER]->dumpDataOneFrame(argv, bScalability, rctDisplay);
#endif
//						}// added by Sharp (98/11/18) // deleted by Sharp (99/1/25)
					}
					tNextEnhc = pvodec[ENHN_LAYER] -> senseTime ();
				}

// begin: deleted by Sharp (99/1/25)
/*
				if((tPvopBase == tNextEnhc) && (tNextEnhc != EOF)) {						// for Spatial Scalability
					pvodec[ENHN_LAYER] -> copyBufP2ToB1 ();
					iEof = pvodec[ENHN_LAYER] -> decode ();
					nEnhcFrames ++;
				}
*/
// end: deleted by Sharp (99/1/25)

// begin: added by Sharp (99/1/20)
      iEof = tNextBase;
#ifndef __OUT_ONE_FRAME_
// begin: added by Sharp (99/1/28)
      if ( iEof == EOF )
				if ( bCachedRefFrameCoded )
// end: added by Sharp (99/1/28)
        dumpFrame (pvodec[BASE_LAYER]->pvopcRefQLater() , pfReconYUV [BASE_LAYER], pfReconSeg[BASE_LAYER], pvodec[BASE_LAYER]->volmd().fAUsage, rctDisplay,
          nBits, pvodec [BASE_LAYER]->vopmd().iVopConstantAlphaValue,  0 );
// begin: added by Sharp (99/1/28)
				else
					dumpNonCodedFrame(pfReconYUV[BASE_LAYER], pfReconSeg[BASE_LAYER], rctDisplay, nBits);
// end: added by Sharp (99/1/28)
#endif
// end: added by Sharp (99/1/20)


				pvodec[ENHN_LAYER] -> bufferB1flush ();

				// Enhancement Layer after Base BVOP
				if (tNextBase == EOF && tNextEnhc != EOF) {
					pvodec[ENHN_LAYER] -> copyBufP2ToB1 ();
					while(tNextEnhc != EOF) {
						iEof = pvodec[ENHN_LAYER] -> decode ();
						nEnhcFrames ++;

						if (pvodec [ENHN_LAYER] -> volmd ().fAUsage != RECTANGLE) {
							// for background composition
							if(pvodec [ENHN_LAYER] -> volmd ().iEnhnType == 1) {
								// should be changed to background_composition flag later.
								printf("============== background composition (2)\n");
								pvodec [ENHN_LAYER] -> BackgroundComposition(argv, bScalability, rctDisplay.width, rctDisplay.height (), pfReconYUV [ENHN_LAYER]); // Modified after Sharp (98/10/26)
							}
							// for back/forward shape
							if(pvodec [ENHN_LAYER] -> vopmd ().iLoadBakShape) {
								printf("---------- output backward shape\n");
								const CVOPU8YUVBA* pvopcQuant = pvodec [ENHN_LAYER]->rgpbfShape[0]->pvopcReconCurr ();
								pvopcQuant->getPlane (BY_PLANE)->dump (pfTestSeg, rctDisplay);
							}
							if(pvodec [ENHN_LAYER] -> vopmd ().iLoadForShape) {
								printf("---------- output forward shape\n");
								const CVOPU8YUVBA* pvopcQuant = pvodec [ENHN_LAYER]->rgpbfShape[1]->pvopcReconCurr ();
								pvopcQuant->getPlane (BY_PLANE)->dump (pfTestSeg, rctDisplay);
							}
						}

						// Output ENHN_LAYER
						if (iEof != EOF) {
//							if ( pvodec [ENHN_LAYER] -> volmd ().iEnhnType != 1 ){ // added by Sharp (98/11/18) // deleted by Sharp (99/1/25)
#ifndef __OUT_ONE_FRAME_
							if ( iEof != FALSE )
								dumpFrame (pvodec[ENHN_LAYER]->pvopcReconCurr(), pfReconYUV [ENHN_LAYER], pfReconSeg[ENHN_LAYER], pvodec[ENHN_LAYER]->volmd().fAUsage, rctDisplay,
								nBits, pvodec [ENHN_LAYER]->vopmd().iVopConstantAlphaValue,  pvodec[ENHN_LAYER]->volmd().iEnhnType );
							else
								dumpNonCodedFrame(pfReconYUV[ENHN_LAYER], pfReconSeg[ENHN_LAYER], rctDisplay, nBits);
//								pvodec [ENHN_LAYER]->dumpDataAllFrame (pfReconYUV [ENHN_LAYER], pfReconSeg [ENHN_LAYER], rctDisplay);
#else
								pvodec [ENHN_LAYER]->dumpDataOneFrame(argv, bScalability, rctDisplay);
#endif
//							}// added by Sharp (98/11/18) // deleted by Sharp (99/1/25)
						}
						tNextEnhc = pvodec[ENHN_LAYER] -> senseTime ();
					}
					pvodec[ENHN_LAYER] -> bufferB1flush ();
				}
			}

// begin: deleted by Sharp (99/1/28)
//			if (bScalability) 
//				pvodec[ENHN_LAYER] -> bufferP1flush ();
// end: deleted by Sharp (99/1/28)

			iEof = tNextBase;
// begin: deleted by Sharp (99/1/25)
/*
// begin: added by Sharp (98/11/11)
#ifndef __OUT_ONE_FRAME_
			if ( iEof == EOF )
				if ( iEof != FALSE )
				dumpFrame (pvodec[BASE_LAYER]->pvopcRefQLater() , pfReconYUV [BASE_LAYER], pfReconSeg[BASE_LAYER], pvodec[BASE_LAYER]->volmd().fAUsage, rctDisplay,
					nBits, pvodec [BASE_LAYER]->vopmd().iVopConstantAlphaValue,  0 );
				else
					dumpNonCodedFrame(pfReconYUV[ENHN_LAYER], pfReconSeg[ENHN_LAYER], rctDisplay, nBits);
#endif
// end: added by Sharp (98/11/11)
*/
// end: deleted by Sharp (99/1/25)
		}

#ifdef __PC_COMPILER_
		Int tickAfter = ::GetTickCount ();
		printf ("Total time: %d\n", tickAfter - tickBegin);
		Double dAverage = (Double) (tickAfter - tickBegin) / (Double) (nBaseFrames + nEnhcFrames);
		printf ("Total frames: %d\tAverage time: %.6lf\n", nBaseFrames + nEnhcFrames, dAverage);
		printf ("FPS %.6lf\n", 1000.0 / dAverage);
#endif // __PC_COMPILER_

#ifndef __OUT_ONE_FRAME_
		fclose (pfReconYUV[BASE_LAYER]);
		if (pvodec[BASE_LAYER]->volmd ().fAUsage != RECTANGLE)
			fclose (pfReconSeg [BASE_LAYER]);
#endif
		delete pvodec[BASE_LAYER];
		if (bScalability) {
#ifndef __OUT_ONE_FRAME_
			fclose (pfReconYUV[ENHN_LAYER]);
			if (pvodec[ENHN_LAYER]->volmd ().fAUsage != RECTANGLE)
				fclose (pfReconSeg [ENHN_LAYER]);
#endif
			delete pvodec[ENHN_LAYER];
		}

	}

	return 0;
}

Void dumpFrame (const CVOPU8YUVBA* pvopcQuant, FILE* pfYUV, FILE* pfSeg, AlphaUsage fAUsage, CRct& rct, UInt nBits, Int iAlphaScale, Int DumpSkip)
{
	if ( DumpSkip == 0 ){
	pvopcQuant->getPlane (Y_PLANE)->dump (pfYUV, rct);
	pvopcQuant->getPlane (U_PLANE)->dump (pfYUV, rct / 2);
	pvopcQuant->getPlane (V_PLANE)->dump (pfYUV, rct / 2);
	}
	if(pfSeg!=NULL)
	{
		if (fAUsage == ONE_BIT)
			pvopcQuant->getPlane (BY_PLANE)->dump (pfSeg, rct, iAlphaScale);
		else if (fAUsage == EIGHT_BIT)
			pvopcQuant->getPlane (A_PLANE)->dumpWithMask (pfSeg, pvopcQuant->getPlane (BY_PLANE),
				rct, iAlphaScale);
	}
	return;
}

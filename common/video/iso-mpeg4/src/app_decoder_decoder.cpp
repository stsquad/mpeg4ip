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

and also edited by 
	Takefumi Nagumo (nagumo@av.crl.sony.co.jp), SONY corporation
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
Sharp retains full right to use the code for his/her own purpose, 
assign or donate the code to a third party and to inhibit third parties from using the code for non <MPEG standard> conforming products. 
This copyright notice must be included in all copies or derivative works. 

Copyright (c) 1997.

Module Name:

	tps_decoder.cpp

Abstract:

	caller for decoder for temporal scalability

Revision History:
	Feb. 12. 2000: Bug fix of OBSS by Takefumi Nagumo(SONY)


*************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <strstream>
#ifdef __PC_COMPILER_
#include <windows.h>
#include <mmsystem.h>
#endif // __PC_COMPILER_

#include "typeapi.h"
#include "mode.hpp"
#include "vopses.hpp"
#include "bitstrm.hpp"
#include "tps_enhcbuf.hpp"
#include "enhcbufdec.hpp"
#include "vopsedec.hpp"

#include "dataStruct.hpp"

#ifndef __GLOBAL_VAR_
//#define __GLOBAL_VAR_
#endif

//#define DEFINE_GLOBALS


#include "global.hpp"
#include "globals.hpp"
using namespace std;
#ifdef __MFC_
#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif
#define new DEBUG_NEW
#endif // __MFC_

Void dumpFrame (const CVOPU8YUVBA* pvopcQuant, const CVOPU8YUVBA** ppvopcPrevDisp, FILE* pfYUV, FILE* pfSeg, FILE** ppfAux, Int iAuxCompCount,
                AlphaUsage, CRct& rct, UInt nBits, Int iAlphaScale,
                Int DumpSkip, Bool bInterlace);
Void dumpFrameNonCoded(FILE* pfYUV, FILE* pfSeg, FILE **ppfAux, Int iAuxCompCount, 
				const CVOPU8YUVBA* pvopcPrevDisp, AlphaUsage, CRct& rctDisplay, UInt nBits);

#define MAX_TIME	99999			// for bitstreams stopped at BVOP
#define USAGE "Usage: \tDECODER bitstream_file [enhn_layer_bitstream_file] output_file width height\n\
or:    \tDECODER -vtc bitstream_file output_file display_width display_height \
target_spatial_layer target_snr_layer \
[target_shape_layer fullsize_out [target_tile_id(from) target_tile_id(to)]]\n" 
// modified by Sharp (99/2/16) further modified by SL@Sarnoff (03/02/99)

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
 		if(argc!=12 && argc != 10 && argc != 8) //modified by SL@Sarnoff (03/02/99)
		{
			fprintf(stderr,USAGE);
			exit(1);
		}
	}
	else	
	{
		if (argc < 5 || argc > 8) {		//OBSS_SAIT_991015
			fprintf (stderr,USAGE);
			exit(1);
		}
		else if (argc == 6 || argc == 8)		//OBSS_SAIT_991015
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
		// argv[4]: display width
		// argv[5]: display height
		// argv[6]: target spatial layer
		// argv[7]: target SNR layer
		// argv[8]: target Shape layer
		// argv[9]: fullsize_out
		// argv[10]: start tile id
		// argv[11]: end tile id
// begin : added by Sharp (99/2/16) modified by SL@Sarnoff (03/02/99)
		if ( argc == 8 )
			vtcdec.decode(argv[2], argv[3], atoi(argv[4]), atoi(argv[5]) , atoi(argv[6]), atoi(argv[7]), -1, 0, 0, 0 );
		else if(argc==10)
			vtcdec.decode(argv[2], argv[3], atoi(argv[4]), atoi(argv[5]) , atoi(argv[6]), atoi(argv[7]), atoi(argv[8]), atoi(argv[9]), 0, 0 );
		else 
			vtcdec.decode(argv[2], argv[3], atoi(argv[4]), atoi(argv[5]) , atoi(argv[6]), atoi(argv[7]), atoi(argv[8]), atoi(argv[9]), atoi(argv[10]), atoi(argv[11]));
// end : added by Sharp (99/2/16)
//			vtcdec.decode(argv[2], argv[3], atoi(argv[4]), atoi(argv[5]) ); // deleted by Sharp (99/2/16)

		return 0;
	/////// WAVELET VTC: end   /////////////////////////
	}

	CRct rctDisplay       (0, 0, atoi (argv [3 + bScalability]), atoi (argv [4 + bScalability]));
/*Added */
	CRct rctDisplay_SSenh (0, 0, atoi (argv [3 + bScalability]), atoi (argv [4 + bScalability]));

	//OBSS_SAIT_991015
	Int iFrmWidth_SS=0;
	Int iFrmHeight_SS=0;
	if (bScalability && argc == 8){
		iFrmWidth_SS = atoi (argv [6]) ;
		iFrmHeight_SS = atoi (argv [7]) ;
		rctDisplay_SSenh.right = iFrmWidth_SS;
		rctDisplay_SSenh.bottom = iFrmHeight_SS;
		rctDisplay_SSenh.width = rctDisplay_SSenh.right - rctDisplay_SSenh.left;
	}
	//~OBSS_SAIT_991015

	Bool bTemporalScalability = 0;
	const CVOPU8YUVBA* rgpvopcPrevDisp[2];
	rgpvopcPrevDisp[BASE_LAYER] = NULL;
	rgpvopcPrevDisp[ENHN_LAYER] = NULL;
	FILE** ppfReconAux[2];
	ppfReconAux[0] = NULL;
	ppfReconAux[1] = NULL;

	if (bScalability){
		CVideoObjectDecoder* pvodec_tps[2];
		cout << "Checking scalability type...\n";
		pvodec_tps[BASE_LAYER] = new CVideoObjectDecoder (argv[1], rctDisplay.width, rctDisplay.height (), NULL, &main_short_video_header);
//OBSS_SAIT_991015
		if (argc == 8)
			pvodec_tps[ENHN_LAYER] = new CVideoObjectDecoder (argv[2], iFrmWidth_SS, iFrmHeight_SS, NULL, &main_short_video_header);					
		else
			pvodec_tps[ENHN_LAYER] = new CVideoObjectDecoder (argv[2], rctDisplay.width, rctDisplay.height (), NULL, &main_short_video_header);
//~OBSS_SAIT_991015
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
		pvodec[0] = NULL;
		pvodec[1] = NULL;
		Bool bSpatialScalable = FALSE;
		//		CRct rctDisplayBackup = rctDisplay; // for recovery from sprite decoding

		pvodec [BASE_LAYER] = new CVideoObjectDecoder (argv [1], rctDisplay.width, rctDisplay.height(), &bSpatialScalable, &main_short_video_header);
	
		// NBIT: get nBits information
		UInt nBits = pvodec [BASE_LAYER]->volmd ().nBits;

		/* Added*/
		if (bScalability){
			//OBSS_SAIT_991015
			if (argc == 8) {
				pvodec [ENHN_LAYER] = new CVideoObjectDecoder (argv [2], iFrmWidth_SS, iFrmHeight_SS, &bSpatialScalable, &main_short_video_header);		
				bSpatialScalable = TRUE;											//OBSSFIX_1:1
				(pvodec [ENHN_LAYER]->getvolmd ()).bSpatialScalability = TRUE;		//OBSSFIX_1:1
				(pvodec [ENHN_LAYER]->getvolmd ()).iFrmWidth_SS = iFrmWidth_SS;		//OBSSFIX_1:1
				(pvodec [ENHN_LAYER]->getvolmd ()).iFrmHeight_SS = iFrmHeight_SS;	//OBSSFIX_1:1
			}
			else {
				pvodec [ENHN_LAYER] = new CVideoObjectDecoder (argv [2], rctDisplay.width, rctDisplay.height(), &bSpatialScalable, &main_short_video_header);
				//OBSSFIX_Usage
				if ((pvodec [ENHN_LAYER]->volmd ()).fAUsage == ONE_BIT) {	
					cout << "\nDecode aborted!\n8 arguments are required for decoding enh-layer of OBSS.\nUsage : decoder base.cmp enh.cmp out_file base_W base_H enh_W enh_H\n";
					exit(1);
				}
				bSpatialScalable = TRUE;																	//OBSSFIX_1:1
				(pvodec [ENHN_LAYER]->getvolmd ()).bSpatialScalability = TRUE;								//OBSSFIX_1:1
				(pvodec [ENHN_LAYER]->getvolmd ()).iFrmWidth_SS = pvodec [ENHN_LAYER]->getvolWidth();		//OBSSFIX_1:1
				(pvodec [ENHN_LAYER]->getvolmd ()).iFrmHeight_SS = pvodec [ENHN_LAYER]->getvolHeight();		//OBSSFIX_1:1
				/*Added*/
				if(bSpatialScalable)
					pvodec[ENHN_LAYER]->set_enh_display_size(rctDisplay, &rctDisplay_SSenh);
			}
			//~OBSS_SAIT_991015
		}
		if (pvodec [BASE_LAYER] -> fSptUsage () == 1) {
				// decode the initial sprite: //wchen: instead of calling decode ()
			pvodec [BASE_LAYER] -> decodeInitSprite ();
		}

		FILE* pfReconYUV [2] = {NULL, NULL};
		Char pchTmp [100];
		sprintf (pchTmp, "%s.yuv", argv [2 + bScalability]);
		pfReconYUV [BASE_LAYER] = fopen (pchTmp, "wb");
		fatal_error("cant open output yuv file",pfReconYUV [BASE_LAYER] != NULL);
		if (bScalability)	{
			sprintf (pchTmp, "%s_e.yuv", argv [2 + bScalability]);
			pfReconYUV [ENHN_LAYER]= fopen (pchTmp, "wb");
			fatal_error("cant open enhancement layer output yuv file",pfReconYUV [ENHN_LAYER] != NULL);
		}
		//OBSS_SAIT_991015
		FILE* pfReconSeg[2];
		pfReconSeg[0] = NULL;
		pfReconSeg[1] = NULL;
		sprintf (pchTmp, "%s.seg", argv [2 + bScalability]);
		if (pvodec [BASE_LAYER]-> volmd ().fAUsage != RECTANGLE)	{
			pfReconSeg[BASE_LAYER] = fopen (pchTmp, "wb");
			fatal_error("cant open output seg file",pfReconSeg[BASE_LAYER] != NULL);
		}
		// MAC (SB) 1-Dec-99

		if (pvodec [BASE_LAYER]-> volmd ().fAUsage == EIGHT_BIT)	{
			Int iAuxCompCount = pvodec [BASE_LAYER]-> volmd ().iAuxCompCount;
			ppfReconAux[0] = new FILE* [iAuxCompCount];
			ppfReconAux[1] = new FILE* [iAuxCompCount];
			for(Int iAuxComp=0; iAuxComp<iAuxCompCount; iAuxComp++ ) {
				sprintf (pchTmp, "%s.%d.aux", argv [2 + bScalability], iAuxComp);
				ppfReconAux[BASE_LAYER][iAuxComp] = fopen (pchTmp, "wb");
				fatal_error("cant open output seg file",ppfReconAux[BASE_LAYER][iAuxComp] != NULL);
			}
		}
		//~MAC
		if (bScalability){
			sprintf (pchTmp, "%s_e.seg", argv [2 + bScalability]);
			if (pvodec [ENHN_LAYER]-> volmd ().fAUsage != RECTANGLE)	{
				pfReconSeg[ENHN_LAYER] = fopen (pchTmp, "wb");
				fatal_error("cant open output seg file",pfReconSeg[ENHN_LAYER] != NULL);
			}
		}
		//~OBSS_SAIT_991015

		Int iEof = 1;
		Int nFrames = 0;
		const CVOPU8YUVBA* pvopcQuant;

#ifdef __PC_COMPILER_
		Int tickBegin = ::GetTickCount ();
#endif // __PC_COMPILER_
		Bool bCachedRefFrame = FALSE;
		Bool bCachedRefFrameCoded = FALSE;
		//OBSS_SAIT_991015 //_SS_BASE_BVOP_
		Int iBASEVOP_time = 0;
		Int iBASEVOP_PredType = 0;
		//~OBSS_SAIT_991015 //_SS_BASE_BVOP_
		while (iEof != EOF)
		{
			if (main_short_video_header) // Added by KPN for short headers
			{
				fprintf(stderr,"Frame number: %d\n", nFrames);
				iEof = pvodec [BASE_LAYER] -> h263_decode (false);
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
					dumpFrame (pvopcQuant, &(rgpvopcPrevDisp[BASE_LAYER]), pfReconYUV [BASE_LAYER], pfReconSeg[BASE_LAYER],
                     ppfReconAux[BASE_LAYER], pvodec [BASE_LAYER]->volmd().iAuxCompCount,
                     pvodec[BASE_LAYER]->volmd().fAUsage, rctDisplay,
						         nBits, pvodec [BASE_LAYER]->vopmd().iVopConstantAlphaValue, 0, pvodec [BASE_LAYER]->vopmd().bInterlace);		//OBSS_SAIT_991015
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
							dumpFrame (pvopcQuant, &(rgpvopcPrevDisp[BASE_LAYER]), pfReconYUV [BASE_LAYER], pfReconSeg[BASE_LAYER],
                         ppfReconAux[BASE_LAYER], pvodec [BASE_LAYER]->volmd().iAuxCompCount,
                         pvodec[BASE_LAYER]->volmd().fAUsage, rctDisplay,
								         nBits, pvodec [BASE_LAYER]->vopmd().iVopConstantAlphaValue, 0, pvodec [BASE_LAYER]->vopmd().bInterlace);		//OBSS_SAIT_991015
						}
						else // non coded
							dumpFrameNonCoded(pfReconYUV [BASE_LAYER], pfReconSeg[BASE_LAYER],
								ppfReconAux[BASE_LAYER], pvodec [BASE_LAYER]->volmd().iAuxCompCount,
								rgpvopcPrevDisp[BASE_LAYER],
								pvodec[BASE_LAYER]->volmd().fAUsage, rctDisplay, nBits);		//OBSS_SAIT_991015
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
							dumpFrame (pvopcQuant, &(rgpvopcPrevDisp[BASE_LAYER]), pfReconYUV [BASE_LAYER], pfReconSeg[BASE_LAYER],
                         ppfReconAux[BASE_LAYER], pvodec [BASE_LAYER]->volmd().iAuxCompCount,
                         pvodec[BASE_LAYER]->volmd().fAUsage, rctDisplay,
								         nBits, pvodec [BASE_LAYER]->vopmd().iVopConstantAlphaValue, 0, pvodec [BASE_LAYER]->vopmd().bInterlace);		//OBSS_SAIT_991015
						}
						else // non coded BVOP
							dumpFrameNonCoded(pfReconYUV [BASE_LAYER], pfReconSeg[BASE_LAYER],
								ppfReconAux[BASE_LAYER], pvodec [BASE_LAYER]->volmd().iAuxCompCount,
								rgpvopcPrevDisp[BASE_LAYER],
								pvodec[BASE_LAYER]->volmd().fAUsage, rctDisplay, nBits);		//OBSS_SAIT_991015
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
								dumpFrame (pvopcQuant, &(rgpvopcPrevDisp[BASE_LAYER]), pfReconYUV [BASE_LAYER], pfReconSeg[BASE_LAYER],
							        ppfReconAux[BASE_LAYER], pvodec [BASE_LAYER]->volmd().iAuxCompCount,
								    pvodec[BASE_LAYER]->volmd().fAUsage, rctDisplay,
									nBits, pvodec [BASE_LAYER]->vopmd().iVopConstantAlphaValue, 0, pvodec [BASE_LAYER]->vopmd().bInterlace);		//OBSS_SAIT_991015
							}
							else // non coded
								dumpFrameNonCoded(pfReconYUV [BASE_LAYER], pfReconSeg[BASE_LAYER],
									ppfReconAux[BASE_LAYER], pvodec [BASE_LAYER]->volmd().iAuxCompCount,
									rgpvopcPrevDisp[BASE_LAYER],
									pvodec[BASE_LAYER]->volmd().fAUsage, rctDisplay, nBits);		//OBSS_SAIT_991015
						}

						// cache current reference
						bCachedRefFrame = TRUE;
						bCachedRefFrameCoded = (iEof != FALSE);
					}
				}
			}

			if (bSpatialScalable == TRUE) {
//OBSS_SAIT_991015  //_SS_BASE_BVOP_
				if(iEof == FALSE){			
					pvopcQuant = NULL;		
					if(pvodec[BASE_LAYER]->volmd().fAUsage == ONE_BIT){					
						iBASEVOP_time		= pvodec[BASE_LAYER]->show_current_time();	
						iBASEVOP_PredType	= pvodec[BASE_LAYER]->vopmd().vopPredType;	
					}																	
				}							
				else {
					pvopcQuant			= pvodec [BASE_LAYER]->pvopcReconCurr();	//Added by SONY 98/11/25
					iBASEVOP_time		= pvodec[BASE_LAYER]->show_current_time();	
					iBASEVOP_PredType	= pvodec[BASE_LAYER]->vopmd().vopPredType;
				}
				//These static parameters will be used in the case BVOP is available in the BASE_LAYER (for spatial scalability)
				static CVOPU8YUVBA* pBASE_stackVOP=NULL; //Temporal buffer of BASELAYER reconstructed image
				static Int iBase_stackTime = -1;
				static CRct pBase_stack_rctBase;

				Int iENHNVOP_time			= 	pvodec[ENHN_LAYER] -> senseTime ();

				static ShapeMode* pBase_stack_Baseshpmd = NULL;
				static ShapeMode* pBase_tmp_Baseshpmd = NULL;
				static CMotionVector* pBase_stack_mvBaseBY = NULL;
				static CMotionVector* pBase_tmp_mvBaseBY = NULL;
				static Int iBase_stack_x, iBase_stack_y;
				Bool	BGComposition = FALSE;	//for OBSS partial enhancement mode

				if (iBASEVOP_time		!= iENHNVOP_time	&& iBASEVOP_PredType	!=BVOP ){
					//stack YUV images and paramters
					pBase_stack_rctBase = pvodec [BASE_LAYER] -> getBaseRct();
					iBase_stackTime		= iBASEVOP_time;
		//for OBSS BVOP_BASE : stack
//OBSSFIX_MODE3
					if(pvodec[ENHN_LAYER]->volmd().fAUsage == ONE_BIT &&
					   !(pvodec[ENHN_LAYER]->volmd().iHierarchyType == 0 &&
					     pvodec[ENHN_LAYER]->volmd().iuseRefShape   == 1 &&
						 pvodec[ENHN_LAYER]->volmd().iEnhnType      != 0 )) {
//					if(pvodec[ENHN_LAYER]->volmd().fAUsage == ONE_BIT) {
//~OBSSFIX_MODE3
						if(iEof) pBASE_stackVOP	= new CVOPU8YUVBA (*pvopcQuant, pvodec[BASE_LAYER]->volmd().fAUsage, (pvodec[BASE_LAYER]->pvopcQuantCurr())->whereY());
						iBase_stack_x = pvodec [BASE_LAYER] -> getMBXRef();
						iBase_stack_y = pvodec [BASE_LAYER] -> getMBYRef();
						pBase_stack_Baseshpmd = new ShapeMode[iBase_stack_x*iBase_stack_y];
						pBase_stack_mvBaseBY = new CMotionVector[iBase_stack_x*iBase_stack_y];
						for(Int j=0;j<iBase_stack_y;j++){
							for(Int i=0;i<iBase_stack_x;i++){
								pBase_stack_Baseshpmd[j*iBase_stack_x+i] = (pvodec[BASE_LAYER] ->shapemd())[j*iBase_stack_x+i];
								pBase_stack_mvBaseBY[j*iBase_stack_x+i] = (pvodec [BASE_LAYER] ->getmvBaseBY())[j*iBase_stack_x+i];	
							}
						}
					} 
//OBSSFIX_MODE3
					else if(pvodec[ENHN_LAYER]->volmd().fAUsage == RECTANGLE ||
						(pvodec[ENHN_LAYER]->volmd().iHierarchyType  == 0 &&
						 pvodec[ENHN_LAYER]->volmd().iuseRefShape    == 1 &&
						 pvodec[ENHN_LAYER]->volmd().iEnhnType       != 0 )){
						if(pvodec[BASE_LAYER]->volmd().fAUsage == RECTANGLE)
							pBASE_stackVOP= new CVOPU8YUVBA (*pvopcQuant, pvodec[BASE_LAYER]->volmd().fAUsage, pBase_stack_rctBase);
						else if(iEof) pBASE_stackVOP = new CVOPU8YUVBA (*pvopcQuant, pvodec[BASE_LAYER]->volmd ().fAUsage, (pvodec[BASE_LAYER]->pvopcQuantCurr())->whereY());
//					else if(pvodec[ENHN_LAYER]->volmd().fAUsage == RECTANGLE){
//						pBASE_stackVOP	= new CVOPU8YUVBA (*pvopcQuant, pvodec[BASE_LAYER]->volmd().fAUsage, pBase_stack_rctBase);
//~OBSSFIX_MODE3

					}
		//~for OBSS BVOP_BASE : stack

				} else if(	iBASEVOP_time != -1 &&iBASEVOP_time == iENHNVOP_time){
					//decode VOP using currentry decoded Base layer image
					if(pvodec[ENHN_LAYER]->volmd().fAUsage == ONE_BIT) {
						pvodec[ENHN_LAYER] -> setShapeMode(pvodec[BASE_LAYER] ->shapemd());
						CMotionVector* pmvBaseBY;							
						pmvBaseBY = pvodec [BASE_LAYER] ->getmvBaseBY();	
						pvodec [ENHN_LAYER] -> setmvBaseBY(pmvBaseBY);		
						Int x, y;
						x = pvodec [BASE_LAYER] -> getMBBaseXRef();		 
						y = pvodec [BASE_LAYER] -> getMBBaseYRef();		 
						pvodec [ENHN_LAYER] -> setMBXYRef(x, y);
						CRct prctBase;
						prctBase = pvodec [BASE_LAYER] -> getBaseRct();
						pvodec [ENHN_LAYER] -> setBaseRct(prctBase);
					}

				// if base layer is non coded, pvopcQuant=NULL, but enh layer should also be non coded.
					iEof = pvodec [ENHN_LAYER] -> decode (pvopcQuant);
					// for background composition (base layer(background) + partial enhancement layer)
					BGComposition = FALSE;
//OBSSFIX_MODE3
					if(	pvodec [ENHN_LAYER] -> volmd().iHierarchyType == 0 && 
						pvodec [ENHN_LAYER] -> volmd().iEnhnType      == 1 && 
						pvodec [ENHN_LAYER] -> vopmd().bBGComposition == 1 ) 
						BGComposition = pvodec [ENHN_LAYER] -> BackgroundCompositionSS(rctDisplay_SSenh.width, rctDisplay_SSenh.height (), pfReconYUV[ENHN_LAYER], pfReconSeg[ENHN_LAYER], pvopcQuant); 
//					if(	pvodec [ENHN_LAYER] -> volmd().iHierarchyType == 0 && 
//						pvodec [ENHN_LAYER] -> volmd().iEnhnType == 1 && 
//						pvodec [ENHN_LAYER] -> volmd().iuseRefShape == 0 ) 
//						BGComposition = pvodec [ENHN_LAYER] -> BackgroundComposition(rctDisplay_SSenh.width, rctDisplay_SSenh.height (), pfReconYUV[ENHN_LAYER], pfReconSeg[ENHN_LAYER]); 
//~OBSSFIX_MODE3
					if(iEof!=EOF)
						nFrames++; // include enhancement layer

					if(iEof==FALSE)
						dumpFrameNonCoded(pfReconYUV [ENHN_LAYER], pfReconSeg[ENHN_LAYER],
							ppfReconAux[ENHN_LAYER], pvodec [ENHN_LAYER]->volmd().iAuxCompCount,
							rgpvopcPrevDisp[ENHN_LAYER],
							pvodec[ENHN_LAYER]->volmd().fAUsage, rctDisplay_SSenh, nBits);

					if (iEof != EOF && iEof!=FALSE )
						//for OBSS partial enhancement mode
//OBSSFIX_MODE3
						if(!(pvodec [ENHN_LAYER]->volmd().iHierarchyType == 0 && pvodec [ENHN_LAYER] -> volmd().iEnhnType == 1 && BGComposition)) 
//						if(!(pvodec [ENHN_LAYER]->volmd().iHierarchyType == 0 && pvodec [ENHN_LAYER] -> volmd().iEnhnType == 1 && pvodec [ENHN_LAYER] -> volmd().iuseRefShape == 0 && BGComposition)) 
//~OBSSFIX_MODE3
							dumpFrame (pvodec [ENHN_LAYER]->pvopcReconCurr (), &(rgpvopcPrevDisp[ENHN_LAYER]), pfReconYUV [ENHN_LAYER], pfReconSeg[ENHN_LAYER], 
										NULL, 0,
										pvodec[ENHN_LAYER]->volmd().fAUsage, rctDisplay_SSenh,
										nBits, pvodec [BASE_LAYER]->vopmd().iVopConstantAlphaValue, 0, 0); // still base layer

					if(	iBase_stackTime != -1 &&
						iBase_stackTime == pvodec[ENHN_LAYER] -> senseTime ()){
						//decode VOP using stacked VOP.
		//for OBSS BVOP_BASE : stack out
//OBSSFIX_MODE3
						if(pvodec[ENHN_LAYER]->volmd().fAUsage == ONE_BIT &&
						  !(pvodec[ENHN_LAYER]->volmd().iHierarchyType == 0 &&
						    pvodec[ENHN_LAYER]->volmd().iEnhnType      != 0 &&
							pvodec[ENHN_LAYER]->volmd().iuseRefShape   == 1)) {
//						if(pvodec[ENHN_LAYER]->volmd().fAUsage == ONE_BIT) {
//~OBSSFIX_MODE3
							pBase_tmp_Baseshpmd = pvodec[BASE_LAYER] ->shapemd();
							pvodec[ENHN_LAYER] -> setShapeMode(pBase_stack_Baseshpmd);
							pBase_tmp_mvBaseBY = pvodec [BASE_LAYER] ->getmvBaseBY();
							pvodec [ENHN_LAYER] -> setmvBaseBY(pBase_stack_mvBaseBY);		
							pvodec [ENHN_LAYER] -> setMBXYRef(iBase_stack_x, iBase_stack_y);
							pvodec [ENHN_LAYER] -> setBaseRct(pBase_stack_rctBase);
						}
		//~for OBSS BVOP_BASE : stack out

						iEof = pvodec [ENHN_LAYER] -> decode (pBASE_stackVOP);
						// for background composition with base layer (OBSS partial enhancement mode)
						BGComposition = FALSE;
//OBSSFIX_MODE3
						if(	pvodec [ENHN_LAYER] -> volmd().iHierarchyType == 0 && 
							pvodec [ENHN_LAYER] -> volmd().iEnhnType      == 1 && 
							pvodec [ENHN_LAYER] -> vopmd().bBGComposition == 1) 
							BGComposition = pvodec [ENHN_LAYER] -> BackgroundCompositionSS(rctDisplay_SSenh.width, rctDisplay_SSenh.height (), pfReconYUV[ENHN_LAYER], pfReconSeg[ENHN_LAYER],pBASE_stackVOP); 
//						if(	pvodec [ENHN_LAYER] -> volmd().iHierarchyType == 0 && 
//							pvodec [ENHN_LAYER] -> volmd().iEnhnType == 1 && 
//							pvodec [ENHN_LAYER] -> volmd().iuseRefShape == 0 ) 
//							BGComposition = pvodec [ENHN_LAYER] -> BackgroundComposition(rctDisplay_SSenh.width, rctDisplay_SSenh.height (), pfReconYUV[ENHN_LAYER], pfReconSeg[ENHN_LAYER]); 
//~OBSSFIX_MODE3
		//for OBSS BVOP_BASE : free stack
//OBSSFIX_MODE3
						if(pvodec[ENHN_LAYER]->volmd().fAUsage == ONE_BIT &&
						  !(pvodec[ENHN_LAYER]->volmd().iHierarchyType == 0 &&
						    pvodec[ENHN_LAYER]->volmd().iuseRefShape   == 1 &&
							pvodec[ENHN_LAYER]->volmd().iEnhnType      != 0 )) {
//						if(pvodec[ENHN_LAYER]->volmd().fAUsage == ONE_BIT) {
//~OBSSFIX_MODE3
							pvodec[ENHN_LAYER] -> setShapeMode(pBase_tmp_Baseshpmd);
							delete [] pBase_stack_Baseshpmd;
							pBase_stack_Baseshpmd = NULL;
							pBase_tmp_Baseshpmd = NULL;
							pvodec [ENHN_LAYER] -> setmvBaseBY(pBase_tmp_mvBaseBY);	
							delete [] pBase_stack_mvBaseBY;
							pBase_stack_mvBaseBY = NULL;
							pBase_tmp_mvBaseBY = NULL;
						}
		//~for OBSS BVOP_BASE : free stack

						delete pBASE_stackVOP;
						pBASE_stackVOP = NULL;

						iBase_stackTime = -1;
						if(iEof!=EOF)
							nFrames++; // include enhancement layer

						if(iEof==FALSE)
							dumpFrameNonCoded(pfReconYUV [ENHN_LAYER], pfReconSeg[ENHN_LAYER],
								ppfReconAux[ENHN_LAYER], pvodec [ENHN_LAYER]->volmd().iAuxCompCount,
								rgpvopcPrevDisp[ENHN_LAYER],
								pvodec[ENHN_LAYER]->volmd().fAUsage, rctDisplay_SSenh, nBits);

						if (iEof != EOF && iEof!=FALSE)
							//for OBSS partial enhancement mode
//OBSSFIX_MODE3
							if(!(pvodec [ENHN_LAYER]->volmd().iHierarchyType == 0 && pvodec [ENHN_LAYER] -> volmd().iEnhnType == 1 && BGComposition)) 
//							if(!(pvodec [ENHN_LAYER]->volmd().iHierarchyType == 0 && pvodec [ENHN_LAYER] -> volmd().iEnhnType == 1 && pvodec [ENHN_LAYER] -> volmd().iuseRefShape == 0 && BGComposition)) 
//~OBSSFIX_MODE3
								dumpFrame (pvodec [ENHN_LAYER]->pvopcReconCurr (), &(rgpvopcPrevDisp[ENHN_LAYER]), pfReconYUV [ENHN_LAYER], pfReconSeg[ENHN_LAYER], 
                NULL, 0,
                pvodec[ENHN_LAYER]->volmd().fAUsage, rctDisplay_SSenh,
								nBits, pvodec [BASE_LAYER]->vopmd().iVopConstantAlphaValue, 0, 0); // still base layer
					} //					if(	iBase_stackTime != -1 && iBase_stackTime == pvodec[ENHN_LAYER] -> senseTime ()){
				} //else if(iBASEVOP_time != -1 &&iBASEVOP_time == iENHNVOP_time){
//~OBSS_SAIT_991015	//_SS_BASE_BVOP_
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
			fclose (pfReconSeg[BASE_LAYER]);		//OBSS_SAIT_991015		
		delete pvodec [BASE_LAYER];
		if (bScalability)	{
		//OBSS_SAIT_991015
			if (pvodec [ENHN_LAYER]->volmd ().fAUsage != RECTANGLE)		
				fclose (pfReconSeg[ENHN_LAYER]);						
		//~OBSS_SAIT_991015
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

		// begin changes from Norio Ito 8/16/99
		if (pvodec [ENHN_LAYER]-> volmd ().fAUsage != RECTANGLE && pvodec
			[BASE_LAYER] -> volmd ().fAUsage != RECTANGLE && pvodec[ENHN_LAYER] ->
			volmd().iEnhnType == 1)
		{
			pvodec[ENHN_LAYER] -> setEnhnType ( 2 );
 			printf("EnhnType = %d\n", pvodec [ENHN_LAYER]-> volmd ().iEnhnType);
		}
		// end changes from Norio Ito

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
		FILE* pfReconSeg [2] = { NULL, NULL};
		if (pvodec [BASE_LAYER]-> volmd ().fAUsage != RECTANGLE) {
			pfReconSeg [BASE_LAYER] = fopen (pchTmp, "wb");
			fatal_error("cant open output seg file",pfReconSeg [BASE_LAYER] != NULL);
		}
		if (bScalability) {
			if (pvodec [ENHN_LAYER]-> volmd ().fAUsage != RECTANGLE && pvodec
				[BASE_LAYER] -> volmd ().fAUsage == RECTANGLE) { // updated from Norio Ito
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
			if (pvodec [ENHN_LAYER]-> volmd ().iEnhnType != 0) { // change for Norio Ito
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
						dumpFrame (pvodec[BASE_LAYER]->pvopcRefQPrev() , &(rgpvopcPrevDisp[BASE_LAYER]), pfReconYUV [BASE_LAYER], pfReconSeg[BASE_LAYER],
              NULL, 0,
              pvodec[BASE_LAYER]->volmd().fAUsage, rctDisplay,
							nBits, pvodec [BASE_LAYER]->vopmd().iVopConstantAlphaValue,  0 , pvodec [BASE_LAYER]->vopmd().bInterlace);
					else // added by Sharp (99/1/28)
						dumpFrameNonCoded( pfReconYUV[BASE_LAYER], pfReconSeg[BASE_LAYER],
							ppfReconAux[BASE_LAYER], pvodec [BASE_LAYER]->volmd().iAuxCompCount,
							rgpvopcPrevDisp[BASE_LAYER],
							pvodec[BASE_LAYER]->volmd().fAUsage, rctDisplay, nBits); // added by Sharp (99/1/28)
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
							if(pvodec [ENHN_LAYER] -> volmd ().iEnhnType != 0) { // change for Norio Ito
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
								dumpFrame (pvodec[ENHN_LAYER]->pvopcReconCurr(), &(rgpvopcPrevDisp[ENHN_LAYER]), pfReconYUV [ENHN_LAYER], pfReconSeg[ENHN_LAYER], 
									NULL, 0,
							        pvodec[ENHN_LAYER]->volmd().fAUsage, rctDisplay,
									nBits, pvodec [ENHN_LAYER]->vopmd().iVopConstantAlphaValue,  pvodec[ENHN_LAYER]->volmd().iEnhnType, 0);
							}
							else
								dumpFrameNonCoded(pfReconYUV[ENHN_LAYER], pfReconSeg[ENHN_LAYER],
									ppfReconAux[ENHN_LAYER], pvodec [ENHN_LAYER]->volmd().iAuxCompCount,
									rgpvopcPrevDisp[ENHN_LAYER],
									pvodec[ENHN_LAYER]->volmd().fAUsage, rctDisplay, nBits);
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
				dumpFrame (pvodec[BASE_LAYER]->pvopcReconCurr() , &(rgpvopcPrevDisp[BASE_LAYER]), pfReconYUV [BASE_LAYER], pfReconSeg[BASE_LAYER], 
				    NULL, 0,
					pvodec[BASE_LAYER]->volmd().fAUsage, rctDisplay,
					nBits, pvodec [BASE_LAYER]->vopmd().iVopConstantAlphaValue,  0, pvodec [BASE_LAYER]->vopmd().bInterlace);
				else
					dumpFrameNonCoded(pfReconYUV[BASE_LAYER], pfReconSeg[BASE_LAYER],
						ppfReconAux[BASE_LAYER], pvodec [BASE_LAYER]->volmd().iAuxCompCount,
						rgpvopcPrevDisp[BASE_LAYER],
						pvodec[BASE_LAYER]->volmd().fAUsage, rctDisplay, nBits);
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
						if(pvodec [ENHN_LAYER] -> volmd ().iEnhnType != 0) { // change for Norio Ito
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
								dumpFrame (pvodec[ENHN_LAYER]->pvopcReconCurr(), &(rgpvopcPrevDisp[ENHN_LAYER]), pfReconYUV [ENHN_LAYER], pfReconSeg[ENHN_LAYER], 
								    NULL, 0,
									pvodec[ENHN_LAYER]->volmd().fAUsage, rctDisplay,
									nBits, pvodec [ENHN_LAYER]->vopmd().iVopConstantAlphaValue,  pvodec[ENHN_LAYER]->volmd().iEnhnType, 0);
							else
								dumpFrameNonCoded(pfReconYUV[ENHN_LAYER], pfReconSeg[ENHN_LAYER],
									ppfReconAux[ENHN_LAYER], pvodec [ENHN_LAYER]->volmd().iAuxCompCount,
									rgpvopcPrevDisp[ENHN_LAYER],
									pvodec[ENHN_LAYER]->volmd().fAUsage, rctDisplay, nBits);
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
        dumpFrame (pvodec[BASE_LAYER]->pvopcRefQLater() , &(rgpvopcPrevDisp[BASE_LAYER]), pfReconYUV [BASE_LAYER], pfReconSeg[BASE_LAYER], 
			NULL, 0,
			pvodec[BASE_LAYER]->volmd().fAUsage, rctDisplay,
			nBits, pvodec [BASE_LAYER]->vopmd().iVopConstantAlphaValue,  0, pvodec [BASE_LAYER]->vopmd().bInterlace );
// begin: added by Sharp (99/1/28)
				else
					dumpFrameNonCoded(pfReconYUV[BASE_LAYER], pfReconSeg[BASE_LAYER],
						ppfReconAux[BASE_LAYER], pvodec [BASE_LAYER]->volmd().iAuxCompCount,
						rgpvopcPrevDisp[BASE_LAYER],
						pvodec[BASE_LAYER]->volmd().fAUsage, rctDisplay, nBits);
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
							if(pvodec [ENHN_LAYER] -> volmd ().iEnhnType != 0) { // change for Norio Ito
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
								dumpFrame (pvodec[ENHN_LAYER]->pvopcReconCurr(), &(rgpvopcPrevDisp[ENHN_LAYER]), pfReconYUV [ENHN_LAYER], pfReconSeg[ENHN_LAYER], 
									NULL, 0,
									pvodec[ENHN_LAYER]->volmd().fAUsage, rctDisplay,
									nBits, pvodec [ENHN_LAYER]->vopmd().iVopConstantAlphaValue,  pvodec[ENHN_LAYER]->volmd().iEnhnType, 0);
							else
								dumpFrameNonCoded(pfReconYUV[ENHN_LAYER], pfReconSeg[ENHN_LAYER],
									ppfReconAux[ENHN_LAYER], pvodec [ENHN_LAYER]->volmd().iAuxCompCount,
									rgpvopcPrevDisp[ENHN_LAYER],
									pvodec[ENHN_LAYER]->volmd().fAUsage, rctDisplay, nBits);
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


Void dumpFrame (const CVOPU8YUVBA* pvopcQuant,
				const CVOPU8YUVBA** ppvopcPrevDisp,
                FILE*              pfYUV, 
                FILE*              pfSeg,
                FILE**             ppfAux, 
                Int                iAuxCompCount,
                AlphaUsage         fAUsage, 
                CRct&              rct,
                UInt               nBits, 
                Int                iAlphaScale,
                Int                DumpSkip,
                Bool               bInterlace)
{
	if ( DumpSkip == 0 ){
		if(fAUsage == RECTANGLE)
		{
			*ppvopcPrevDisp = pvopcQuant; // save this output frame, in case later one is skipped
			pvopcQuant->getPlane (Y_PLANE)->dump (pfYUV, rct);
			pvopcQuant->getPlane (U_PLANE)->dump (pfYUV, rct / 2);
			pvopcQuant->getPlane (V_PLANE)->dump (pfYUV, rct / 2);
		}
		else
		{
			PixelC pxlUVZero = 1 << (nBits-1);
			// binary - need to window the output frame
			((CU8Image*) pvopcQuant->getPlane (BUV_PLANE))->decimateBinaryShapeFrom (*(pvopcQuant->getPlane (BY_PLANE)), bInterlace);
			pvopcQuant->getPlane (Y_PLANE)->dumpWithMask (pfYUV, pvopcQuant->getPlane (BY_PLANE), rct, 256, (PixelC)0);
			pvopcQuant->getPlane (U_PLANE)->dumpWithMask (pfYUV, pvopcQuant->getPlane (BUV_PLANE), rct / 2, 256, pxlUVZero);
			pvopcQuant->getPlane (V_PLANE)->dumpWithMask (pfYUV, pvopcQuant->getPlane (BUV_PLANE), rct / 2, 256, pxlUVZero);
		}
	}
	
	if (pfSeg!= NULL && fAUsage != RECTANGLE)
		pvopcQuant->getPlane (BY_PLANE)->dump (pfSeg, rct, iAlphaScale);
	
	if (fAUsage == EIGHT_BIT) {
		if (ppfAux!=NULL) {
			for(Int iAuxComp=0; iAuxComp<iAuxCompCount; iAuxComp++ ) { // MAC (SB) 1-Dec-99
				if (ppfAux[iAuxComp]!=NULL)
					pvopcQuant->getPlaneA (iAuxComp)->dumpWithMask (ppfAux[iAuxComp], pvopcQuant->getPlane (BY_PLANE),
					rct, iAlphaScale, (PixelC)0);
			}
		}
	}
	
}

Void dumpFrameNonCoded(FILE* pfYUV, FILE* pfSeg, FILE **ppfAux, Int iAuxCompCount, const CVOPU8YUVBA* pvopcPrevDisp, AlphaUsage fAUsage, CRct& rctDisplay, UInt nBits)
{
	if(fAUsage != RECTANGLE)
	{
		if (fAUsage == EIGHT_BIT)
			dumpNonCodedFrame(pfYUV, pfSeg, ppfAux, iAuxCompCount, rctDisplay, nBits);
		else
			dumpNonCodedFrame(pfYUV, pfSeg, NULL, 0, rctDisplay, nBits);
	}
	else if(pvopcPrevDisp==NULL)
		dumpNonCodedFrame(pfYUV, pfSeg, NULL, 0, rctDisplay, nBits);
	else
	{
		// dump previous frame again
		pvopcPrevDisp->getPlane (Y_PLANE)->dump (pfYUV, rctDisplay);
		pvopcPrevDisp->getPlane (U_PLANE)->dump (pfYUV, rctDisplay / 2);
		pvopcPrevDisp->getPlane (V_PLANE)->dump (pfYUV, rctDisplay / 2);
	}
}


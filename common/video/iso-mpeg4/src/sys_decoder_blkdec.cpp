/*************************************************************************

This software module was originally developed by 

	Ming-Chieh Lee (mingcl@microsoft.com), Microsoft Corporation
	Wei-ge Chen (wchen@microsoft.com), Microsoft Corporation
	Bruce Lin (blin@microsoft.com), Microsoft Corporation
	Chuang Gu (chuanggu@microsoft.com), Microsoft Corporation
	(date: July, 1997)
and edited by
        Wei Wu (weiwu@stallion.risc.rockwell.com) Rockwell Science Center

and also edited by
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

	blkdec.cpp

Abstract:

	Block decoding functions

Revision History
    Dec 20, 1977    Interlaced tools added by NextLevel Systems
	Sep.06	1999 : RRV added by Eishi Morimatsu (Fujitsu Laboratories Ltd.) 

*************************************************************************/

#include <stdlib.h>
#include <math.h>
#include "typeapi.h"
#include "codehead.h"
#include "mode.hpp"
#include "global.hpp"
#include "bitstrm.hpp"
#include "entropy.hpp"
#include "huffman.hpp"
#include "grayf.hpp" // wmay
#include "dct.hpp"
#include "idct.hpp" // yrchen
#include "vopses.hpp"
#include "vopsedec.hpp"
// RRV insertion
#include "rrv.hpp"
// ~RRV

#ifdef __MFC_
#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

#define new DEBUG_NEW				   
#endif // __MFC_

// HHI  Schueuer
CInvScanSelectorForSADCT::CInvScanSelectorForSADCT(Int **rgiCurrMBCoeffWidth) :
	m_rgiCurrMBCoeffWidth(rgiCurrMBCoeffWidth)
{
	m_adaptedScan = new Int[BLOCK_SQUARE_SIZE];
}

CInvScanSelectorForSADCT::~CInvScanSelectorForSADCT()
{
	delete [] m_adaptedScan;
}         

Int *CInvScanSelectorForSADCT::select(Int *scan, Bool bIsBoundary, Int iBlk)
{

	/*	Int grgiStandardZigzag [BLOCK_SQUARE_SIZE] = {
	 *	0, 1, 8, 16, 9, 2, 3, 10, 
	 *	17, 24, 32, 25, 18, 11, 4, 5, 
	 *	12, 19, 26, 33, 40, 48, 41, 34, 
	 *	27, 20, 13, 6, 7, 14, 21, 28, 
	 *	35, 42, 49, 56, 57, 50, 43, 36, 
	 *	29, 22, 15, 23, 30, 37, 44, 51, 
	 *	58, 59, 52, 45, 38, 31, 39, 46, 
	 *	53, 60, 61, 54, 47, 55, 62, 63
	 *
		Int grgiHorizontalZigzag [BLOCK_SQUARE_SIZE] = {
		0, 1, 2, 3, 8, 9, 16, 17, 
		10, 11, 4, 5, 6, 7, 15, 14, 
		13, 12, 19, 18, 24, 25, 32, 33, 
		26, 27, 20, 21, 22, 23, 28, 29, 
		30, 31, 34, 35, 40, 41, 48, 49, 
		42, 43, 36, 37, 38, 39, 44, 45, 
		46, 47, 50, 51, 56, 57, 58, 59, 
		52, 53, 54, 55, 60, 61, 62, 63

		Int grgiVerticalZigzag [BLOCK_SQUARE_SIZE] = {
		0, 8, 16, 24, 1, 9, 2, 10, 
		17, 25, 32, 40, 48, 56, 57, 49, 
		41, 33, 26, 18, 3, 11, 4, 12, 
		19, 27, 34, 42, 50, 58, 35, 43, 
		51, 59, 20, 28, 5, 13, 6, 14, 
		21, 29, 36, 44, 52, 60, 37, 45, 
		53, 61, 22, 30, 7, 15, 23, 31, 
		38, 46, 54, 62, 39, 47, 55, 63
    */

	if (bIsBoundary) {
    	// the incoming scan is the zigzag scan for an 8x8 block which must be modified for
        // sadct coded boundary blocks. 
    	const Int *pCoeffWidth = m_rgiCurrMBCoeffWidth[iBlk]; 	
		int n, iy, ix;
        int coeff_count = 0;
        int coeff_count_save = 0;
        Int scan_save[BLOCK_SQUARE_SIZE]; 
        for (n=0; n<BLOCK_SQUARE_SIZE; n++) {
        	iy = scan[n]/BLOCK_SIZE;
            ix = scan[n] % BLOCK_SIZE;
            if (pCoeffWidth[iy] > ix) 
            	m_adaptedScan[coeff_count++] = scan[n];  
            else
            	scan_save[coeff_count_save++] = scan[n];          
        }
        coeff_count_save = 0;
        for (n=coeff_count; n<BLOCK_SQUARE_SIZE; n++)
        	m_adaptedScan[coeff_count++] = scan_save[coeff_count_save++];
        
    	return(m_adaptedScan);
	}
    else
    	return(scan);	
}

// end HHI

// HHI Schueuer : const PixelC *rgpxlcBlkShape, Int iBlkShapeWidth added
Void CVideoObjectDecoder::decodeIntraBlockTexture (PixelC* rgpxlcBlkDst,
								 Int iWidthDst,
								 Int iQP, 
								 Int iDcScaler,
								 Int iBlk,
								 MacroBlockMemory* pmbmCurr,
								 CMBMode* pmbmd,
 								 const BlockMemory blkmPred,
								 Int iQpPred,
								 const PixelC *rgpxlcBlkShape,
                 Int iBlkShapeWidth, 
                 Int iAuxComp )
{
	Int* rgiCoefQ = m_rgpiCoefQ [iBlk - 1];
	Int iCoefStart = 0;	// intra-DC 
	//	UInt nBits = m_volmd.nBits; // NBIT


// Added for short headers by KPN (1998-02-07, DS)


 
	if (short_video_header==1) // short_header
	{
		decodeShortHeaderIntraMBDC(rgiCoefQ); // 8 bits FLC & fixed quantizer op 8.
		iCoefStart++;
	}
	else 
	{  
	
		if (iBlk<=V_BLOCK && pmbmd->m_bCodeDcAsAc==FALSE
		|| iBlk>=A_BLOCK1 && pmbmd->m_bCodeDcAsAcAlpha==FALSE) {
		rgiCoefQ [0] = decodeIntraDCmpeg (iBlk <= Y_BLOCK4 || iBlk >=A_BLOCK1);
		iCoefStart++;
		}

	
	} 

// Added for short headers by KPN - END
	
	if (pmbmd->getCodedBlockPattern (iBlk))	{
		Int* rgiZigzag = grgiStandardZigzag;
	    if (m_vopmd.bAlternateScan && iBlk<=V_BLOCK) // 12.22.98 Changes
            rgiZigzag = grgiVerticalZigzag;
        else if (iBlk<=V_BLOCK && pmbmd->m_bACPrediction
			|| iBlk >= A_BLOCK1 && pmbmd->m_pbACPredictionAlpha[(iBlk-7)/4])
			rgiZigzag = (pmbmd->m_preddir [iBlk - 1] == HORIZONTAL) ? grgiVerticalZigzag : grgiHorizontalZigzag;
 		//	Added for data partitioning mode By Toshiba(1998-1-16:DP+RVLC)
		// HHI Schueuer
		if (!m_volmd.bSadctDisable)
				rgiZigzag = m_pscanSelector->select(rgiZigzag, (pmbmd->m_rgTranspStatus[0] == PARTIAL), iBlk); 
		// end HHI
 		if(m_volmd.bDataPartitioning && m_volmd.bReversibleVlc && m_vopmd.vopPredType != BVOP)
 			decodeIntraRVLCTCOEF (rgiCoefQ, iCoefStart, rgiZigzag);
 		else
		//	End Toshiba(1998-1-16:DP+RVLC)
		decodeIntraTCOEF (rgiCoefQ, iCoefStart, rgiZigzag);
	}
	else
		memset (rgiCoefQ + iCoefStart, 0, sizeof (Int) * (BLOCK_SQUARE_SIZE - iCoefStart));
	inverseDCACPred (pmbmd, iBlk - 1, rgiCoefQ, iQP, iDcScaler, blkmPred, iQpPred);

// Added for short headers by KPN (1998-02-07, DS)

	if (short_video_header) inverseQuantizeIntraDc(rgiCoefQ,8); 
	else inverseQuantizeIntraDc (rgiCoefQ, iDcScaler); 

	// HHI Schueuer: sadct 
	if (rgpxlcBlkShape) {
		// HHI Schueuer: for greyscale coding (iBlk-6)
		// assert(pmbmd->m_rgTranspStatus [iBlk] == PARTIAL);
		// end HHI 02/04/99

       	// brute force method to clean out mispredictions outside the active region
           Int *lx = m_rgiCurrMBCoeffWidth[iBlk];
           Int iy, ix;
           
           for (ix=lx[0]; ix<BLOCK_SIZE; ix++)
           	rgiCoefQ[ix] = 0;
			for (iy=1; iy<BLOCK_SIZE; iy++) {
            	if (!lx[iy])
                	rgiCoefQ[iy*BLOCK_SIZE] = 0;
            }
	}
	// end HHI

// Added for short headers by KPN - END
	
	if (m_volmd.fQuantizer == Q_H263)
		inverseQuantizeDCTcoefH263 (rgiCoefQ, 1, iQP);
	else
		inverseQuantizeIntraDCTcoefMPEG (rgiCoefQ, 1, iQP, iBlk>=A_BLOCK1, iAuxComp);

	Int i, j;							//save coefQ (ac) for intra pred
	pmbmCurr->rgblkm [iBlk - 1] [0] = m_rgiDCTcoef [0];	//save recon value of DC for intra pred								//save Qcoef in memory
	for (i = 1, j = 8; i < BLOCK_SIZE; i++, j += BLOCK_SIZE)	{
		pmbmCurr->rgblkm [iBlk - 1] [i] = rgiCoefQ [i];
		pmbmCurr->rgblkm [iBlk - 1] [i + BLOCK_SIZE - 1] = rgiCoefQ [j];
	}
	// this idct includes output clipping 0 -> (2**nbits-1)
// RRV modification
	PixelC *pc_block8x8, *pc_block16x16;
	if(m_vopmd.RRVmode.iRRVOnOff == 1)
	{
		pc_block8x8		= new PixelC [64];
		pc_block16x16	= new PixelC [256];
		// HHI Schueuer:  rgpxlcBlkShape, iBlkShapeWidth added
		//m_pidct->apply (m_rgiDCTcoef, BLOCK_SIZE, pc_block8x8, BLOCK_SIZE, rgpxlcBlkShape, iBlkShapeWidth);
		// yrchen integer idct 10.21.2003
 		// extra care needed if adaptive shape decoding or non-8 bit idct is used
 		m_pinvdct->IDCTClip(m_rgiDCTcoef, BLOCK_SIZE, pc_block8x8, BLOCK_SIZE);		
		MeanUpSampling(pc_block8x8, pc_block16x16, 8, 8);
		writeCubicRct(16, iWidthDst, pc_block16x16, rgpxlcBlkDst);
		delete pc_block8x8;
		delete pc_block16x16;
	}
	else
	{
	// HHI Schueuer:  rgpxlcBlkShape, iBlkShapeWidth added
	// m_pidct->apply (m_rgiDCTcoef, BLOCK_SIZE, rgpxlcBlkDst, iWidthDst, rgpxlcBlkShape, iBlkShapeWidth);
 	  // yrchen integer idct 10.21.2003
 	  // extra care needed if adaptive shape decoding or non-8 bit idct is used
 	  m_pinvdct->IDCTClip(m_rgiDCTcoef, BLOCK_SIZE, rgpxlcBlkDst, iWidthDst);
	//	m_pidct->apply (m_rgiDCTcoef, BLOCK_SIZE, rgpxlcBlkDst, iWidthDst);
	}
//	m_pidct->apply (m_rgiDCTcoef, BLOCK_SIZE, rgpxlcBlkDst, iWidthDst);
// ~RRV
}

Void CVideoObjectDecoder::decideIntraPred (const BlockMemory& blkmRet, 
										   CMBMode* pmbmdCurr,
										   Int&	iQPpred,
										   Int blkn,									   
										   const MacroBlockMemory* pmbmLeft, 
  										   const MacroBlockMemory* pmbmTop, 
										   const MacroBlockMemory* pmbmLeftTop,
										   const MacroBlockMemory* pmbmCurr,
										   const CMBMode* pmbmdLeft,
										   const CMBMode* pmbmdTop,
										   const CMBMode* pmbmdLeftTop)
{
	Int iQPpredTop, iQPpredLeftTop, iQPpredLeft;
	const BlockMemory blkmTop		= findPredictorBlock (blkn, VERTICAL,   pmbmLeft, pmbmTop, pmbmLeftTop, pmbmCurr, pmbmdLeft, pmbmdTop, pmbmdLeftTop, pmbmdCurr, iQPpredTop);
	const BlockMemory blkmLeftTop = findPredictorBlock (blkn, DIAGONAL,   pmbmLeft, pmbmTop, pmbmLeftTop, pmbmCurr, pmbmdLeft, pmbmdTop, pmbmdLeftTop, pmbmdCurr, iQPpredLeftTop);
	const BlockMemory blkmLeft	= findPredictorBlock (blkn, HORIZONTAL, pmbmLeft, pmbmTop, pmbmLeftTop, pmbmCurr, pmbmdLeft, pmbmdTop, pmbmdLeftTop, pmbmdCurr, iQPpredLeft);
	Int iDefVal = 1<<(m_volmd.nBits+2); // NBIT

	Int iPredLeftTop = (blkmLeftTop == NULL) ? iDefVal : blkmLeftTop [0];
	Int iHorizontalGrad				= ((blkmTop  == NULL) ? iDefVal : blkmTop  [0]) - iPredLeftTop;
	Int iVerticalGrad				= ((blkmLeft == NULL) ? iDefVal : blkmLeft [0]) - iPredLeftTop;

	blkmRet = NULL;
	if (abs(iVerticalGrad)  < abs (iHorizontalGrad))	{
		pmbmdCurr->m_preddir [blkn - 1] = VERTICAL;
		if (blkmTop != NULL)	{
			blkmRet = blkmTop;
			iQPpred = iQPpredTop;
		}
	}
	else	{
		pmbmdCurr->m_preddir [blkn - 1] = HORIZONTAL;
		if (blkmLeft != NULL)	{
			blkmRet = blkmLeft;
			iQPpred = iQPpredLeft;
		}
	}
}

// HHI Schueuer:  const CMBMode *pmbmd, Int iBlk, const PixelC *rgpxlcBlkShape, Int iBlkShapeWidth added
Void CVideoObjectDecoder::decodeTextureInterBlock (Int* rgiBlkCurrQ, Int iWidthDst, Int iQP, Bool bAlphaBlock,  const CMBMode *pmbmd, Int iBlk, const PixelC *rgpxlcBlkShape, Int iBlkShapeWidth, Int iAuxComp)
{
	Int* rgiCoefQ = m_rgpiCoefQ [0];							//doesn't matter which one to use
	// HHI Schueuer
	Int* scan = grgiStandardZigzag;
	// end HHI
	//	Added for data partitioning mode By Toshiba(1998-1-16:DP+RVLC)
	if(m_volmd.bDataPartitioning && m_volmd.bReversibleVlc && m_vopmd.vopPredType != BVOP) {
		// HHI Schueuer
		if (!m_volmd.bSadctDisable)
			scan = m_pscanSelector->select(scan, (pmbmd->m_rgTranspStatus[0] == PARTIAL), iBlk);
		// decodeInterRVLCTCOEF (rgiCoefQ, 0, grgiStandardZigzag);
		decodeInterRVLCTCOEF (rgiCoefQ, 0, scan);
		// end HHI
	}
	else {
	//	End Toshiba(1998-1-16:DP+RVLC)
	// 12.22.98 begin of changes
		// HHI Schueuer: modified scan for sadct
		scan = (m_vopmd.bAlternateScan&&!bAlphaBlock) ? grgiVerticalZigzag : grgiStandardZigzag;
		if (!m_volmd.bSadctDisable) 
			scan = m_pscanSelector->select(scan, (pmbmd->m_rgTranspStatus[0] == PARTIAL), iBlk);
		decodeInterTCOEF (rgiCoefQ, 0, scan);//else don't add error signal
		// decodeInterTCOEF (rgiCoefQ, 0, //else don't add error signal
		//    (m_vopmd.bAlternateScan&&!bAlphaBlock) ? grgiVerticalZigzag : grgiStandardZigzag);
		// end HHI			
	// 12.22.98 end of changes
	}
	if (m_volmd.fQuantizer == Q_H263)
		inverseQuantizeDCTcoefH263 (rgiCoefQ, 0, iQP);
	else
		inverseQuantizeInterDCTcoefMPEG (rgiCoefQ, 0, iQP, bAlphaBlock, iAuxComp );
	// this idct does not include clipping, but clipping is performed
	// in addErrorAndPredToCurrQ to range 0 -> (2**nbits-1)
// RRV modification
	PixelI *pi_block8x8, *pi_block16x16;
	if(m_vopmd.RRVmode.iRRVOnOff == 1)
	{
		pi_block8x8 = new PixelI [64];
		pi_block16x16 = new PixelI [256];
		// HHI Schueuer
		// m_pidct->apply (m_rgiDCTcoef, BLOCK_SIZE, rgiBlkCurrQ, iWidthDst);
		// m_pidct->apply (m_rgiDCTcoef, BLOCK_SIZE, pi_block8x8, BLOCK_SIZE, rgpxlcBlkShape, iBlkShapeWidth, NULL);
 		// yrchen integer idct 10.21.2003
 		// extra care needed if adaptive shape decoding or non-8 bit idct is used
 		m_pinvdct->IDCT (m_rgiDCTcoef, BLOCK_SIZE, rgiBlkCurrQ, iWidthDst);
		MeanUpSampling(pi_block8x8, pi_block16x16, 8, 8); 
		writeCubicRct(16, iWidthDst, pi_block16x16, rgiBlkCurrQ);
		delete pi_block8x8;
		delete pi_block16x16;
	}
	else
	{
		// HHI Schueuer
		// m_pidct->apply (m_rgiDCTcoef, BLOCK_SIZE, rgiBlkCurrQ, iWidthDst);
	        //m_pidct->apply (m_rgiDCTcoef, BLOCK_SIZE, rgiBlkCurrQ, iWidthDst, rgpxlcBlkShape, iBlkShapeWidth, NULL);
 		// yrchen integer idct 10.21.2003
 		// extra care needed if adaptive shape decoding or non-8 bit idct is used
 		m_pinvdct->IDCT(m_rgiDCTcoef, BLOCK_SIZE, rgiBlkCurrQ, iWidthDst);	
	}
// ~RRV
}

Void CVideoObjectDecoder::decodeIntraTCOEF (Int* rgiCoefQ, Int iCoefStart, Int* rgiZigzag)
{
	Bool bIsLastRun = FALSE;
	Int  iRun = 0;
	Int	 iLevel = 0;
	Int  iCoef = iCoefStart;
	Long lIndex;
	while (!bIsLastRun) {
		
	// Added for short headers by KPN (1998-02-07, DS)
		if (short_video_header) 
		{ // H.263
			lIndex = m_pentrdecSet->m_pentrdecDCT->decodeSymbol();															
		}
		else
		{	// MPEG-4
			lIndex = m_pentrdecSet->m_pentrdecDCTIntra->decodeSymbol();		
		}
	// Added for short headers by KPN - END
				
		if (lIndex != TCOEF_ESCAPE)	{	
			
			if (short_video_header) // Added by KPN [FDS]
			{ // short header
				Bool tempBool = (Bool) bIsLastRun;
				decodeInterVLCtableIndex (lIndex, iLevel, iRun, tempBool); 
			
				bIsLastRun = (Int) tempBool;
			}
			else 
			{ // MPEG-4
					decodeIntraVLCtableIndex (lIndex, iLevel, iRun, bIsLastRun);
					
			}
				
		}
		else	{
			decodeEscape (iLevel, iRun, bIsLastRun, g_rgiLMAXintra, g_rgiRMAXintra, 
						  m_pentrdecSet->m_pentrdecDCTIntra, 
				      /* (Void(CVideoObjectDecoder::*)(Int, Bool &, Int &, Int &)) */ &CVideoObjectDecoder::decodeIntraVLCtableIndex);
		}
		//fprintf(stderr,"%d %d %d %d\n", lIndex,iLevel,iRun,bIsLastRun);
		for (Int i = 0; i < iRun; i++)	{
			rgiCoefQ [rgiZigzag [iCoef]] = 0;
			iCoef++;
		}
		rgiCoefQ [rgiZigzag [iCoef]] = iLevel;			
		iCoef++;
	}															
	for (Int i = iCoef; i < BLOCK_SQUARE_SIZE; i++)			// fill the rest w/ zero
		rgiCoefQ [rgiZigzag [i]]  = 0;
}

Void CVideoObjectDecoder::decodeEscape (Int& iLevel, Int& iRun, Int& bIsLastRun, const Int* rgiLMAX, const Int* rgiRMAX, 
				   CEntropyDecoder* pentrdec, DECODE_TABLE_INDEX decodeVLCtableIndex)
{
	//	Modified by Toshiba(1997-11-14)
	if (!short_video_header) { // Added bij KPN [FDS]
	if (m_pbitstrmIn->getBits (1) == 0)	{				//vlc; Level+
	//if (m_pbitstrmIn->getBits (1) == 1)	{				//vlc; Level+
	//	End Toshiba(1997-11-14)
		Int iIndex = pentrdec->decodeSymbol();
		(this->*decodeVLCtableIndex) (iIndex, iLevel, iRun, bIsLastRun);
		//get level back
		Int iLevelPlusAbs = abs (iLevel);
		Int iLevelAbs = iLevelPlusAbs + rgiLMAX [(iRun & 0x0000003F) + (bIsLastRun << 6)];		//hashing the table
		iLevel = sign(iLevel) * iLevelAbs;
	}
	//	Modified Toshiba(1997-11-14)
	else if (m_pbitstrmIn->getBits (1) == 0) {			//vlc; Run+
	//else if (m_pbitstrmIn->getBits (1) == 1) {			//vlc; Run+
	//	End Toshiba(1997-11-14)
		Int iIndex = pentrdec->decodeSymbol();
		(this->*decodeVLCtableIndex) (iIndex, iLevel, iRun, bIsLastRun);		
		iRun = iRun + rgiRMAX [(abs(iLevel) & 0x0000001F) + (bIsLastRun << 5)];	//get run back; RMAX tabl incl. + 1 already
	}
	else {												//flc
		bIsLastRun = (Bool) m_pbitstrmIn->getBits (1);		
		iRun =	(Int) m_pbitstrmIn->getBits (NUMBITS_ESC_RUN);			
		assert (iRun < BLOCK_SQUARE_SIZE);\
		Int iLevelBits = 12; // = m_volmd.nBits;
		Int iMarker = m_pbitstrmIn->getBits (1);
		assert(iMarker == 1);
		iLevel = (Int) m_pbitstrmIn->getBits (iLevelBits);
		iMarker = m_pbitstrmIn->getBits (1);
		assert(iMarker == 1);
		Int iMaxAC = (1<<(iLevelBits-1)) - 1;
		assert(iLevel!=iMaxAC+1);
		if (iLevel > iMaxAC)
			iLevel -= (1<<iLevelBits);
		assert(iLevel != 0);
	}
	} // Escape coding short headers. Added by KPN
	else
	{
		bIsLastRun = (Bool) m_pbitstrmIn->getBits (1);
		iRun = (Int) m_pbitstrmIn->getBits (6);
		int iLevelIndex = (Int) m_pbitstrmIn->getBits(8);
		if (iLevelIndex==0||iLevelIndex==128) 
		{
			fprintf(stderr,"Short header mode. Levels 0 and 128 are not allowed\n");
			exit(2);
		}
		if (iLevelIndex >=0 && iLevelIndex <128) 
		{
			iLevel=iLevelIndex;
		} else 
		{
			iLevel=iLevelIndex-256;
		}
	}
}

Void CVideoObjectDecoder::decodeInterTCOEF (Int* rgiCoefQ, Int iCoefStart, Int* rgiZigzag)
{
	Bool bIsLastRun = FALSE;
	Int  iRun = 0;
	Int	 iLevel = 0;
	Int  iCoef = iCoefStart;
	Long lIndex;
	while (!bIsLastRun) {
		lIndex = m_pentrdecSet->m_pentrdecDCT->decodeSymbol();
		if (lIndex != TCOEF_ESCAPE)	{							// if Huffman
			decodeInterVLCtableIndex (lIndex, iLevel, iRun, bIsLastRun);
			assert (iRun < BLOCK_SQUARE_SIZE);
		}
		else
			decodeEscape (iLevel, iRun, bIsLastRun, g_rgiLMAXinter, g_rgiRMAXinter, 
				      m_pentrdecSet->m_pentrdecDCT, /*(Void(CVideoObjectDecoder::*)(Int, Bool &, Int &, Int &))*/ &CVideoObjectDecoder::decodeInterVLCtableIndex);
		for (Int i = 0; i < iRun; i++)	{
			rgiCoefQ [rgiZigzag [iCoef]] = 0;
			iCoef++;
		}
		rgiCoefQ [rgiZigzag [iCoef]] = iLevel;			
		iCoef++;
	}															
	for (Int i = iCoef; i < BLOCK_SQUARE_SIZE; i++)					// fill the rest w/ zero
		rgiCoefQ [rgiZigzag [i]]  = 0;
}


Void CVideoObjectDecoder::decodeIntraVLCtableIndex  (Int iIndex, Int& iLevel, Int& iRun, Int& bIsLastRun)
{
	static Int iLevelMask = 0x0000001F;
	static Int iRunMask = 0x000003E0;
	static Int iLastRunMask = 0x00000400;
	iLevel = iLevelMask & grgiIntraYAVCLHashingTable [iIndex];
	iRun = (iRunMask & grgiIntraYAVCLHashingTable [iIndex]) >> 5;
	bIsLastRun = (iLastRunMask  & grgiIntraYAVCLHashingTable [iIndex]) >> 10;
	if (m_pentrdecSet->m_pentrdecDCT->bitstream()->getBits (1) == TRUE)	// get signbit
		iLevel = -iLevel;
	assert (iRun < BLOCK_SQUARE_SIZE);
}

															
Void CVideoObjectDecoder::decodeInterVLCtableIndex (Int   iIndex, Int& iLevel,		// return islastrun, run and level													 
													Int&  iRun, Bool& bIsLastRun)
{	
	assert (iIndex >= 0 && iIndex < 102);
	bIsLastRun = FALSE;
	Int iIndexLeft = (Int) iIndex;
	if (iIndex >= 58)	{
		iIndexLeft -= 58;
		bIsLastRun = TRUE;
	}
	iRun = 0;
	while (iIndexLeft >= 0)	{
		if (!bIsLastRun)
			iIndexLeft -= grgIfNotLastNumOfLevelAtRun [iRun];
		else
			iIndexLeft -= grgIfLastNumOfLevelAtRun [iRun];
		iRun++;
	}
	assert (iRun > 0);
	iRun--;
	if (!bIsLastRun)
		iLevel = iIndexLeft + grgIfNotLastNumOfLevelAtRun [iRun] + 1;
	else
		iLevel = iIndexLeft + grgIfLastNumOfLevelAtRun [iRun] + 1;	
	assert (iRun >= 0);
	if (m_pentrdecSet->m_pentrdecDCT->bitstream()->getBits (1) == TRUE)	// get signbit
		iLevel = -iLevel;
}

Int CVideoObjectDecoder::decodeIntraDCmpeg (Bool bIsYBlk)
{
	Long lSzDiffIntraDC;
	if (bIsYBlk)
		lSzDiffIntraDC = m_pentrdecSet->m_pentrdecIntraDCy->decodeSymbol();
	else 
		lSzDiffIntraDC = m_pentrdecSet->m_pentrdecIntraDCc->decodeSymbol();
	Int iDiffIntraDC = 0;
	if (lSzDiffIntraDC !=0 )	{
	    if (lSzDiffIntraDC<=8) { // NBIT
		U8 chDiffIntraDC = 
			(U8) m_pentrdecSet->m_pentrdecIntraDCy->bitstream()->getBits (lSzDiffIntraDC);	
		if (!((1 << (lSzDiffIntraDC - 1)) & chDiffIntraDC))
			iDiffIntraDC = -1 * ((0x00FF >> (8 - lSzDiffIntraDC)) & (~chDiffIntraDC));
		else
			iDiffIntraDC = (Int) chDiffIntraDC;
	    } else { // NBIT - marker bit inserted after 8 bits
/*
		UInt uiDiffIntraDC =
			(UInt) m_pentrdecSet->m_pentrdecIntraDCy->bitstream()->getBits (lSzDiffIntraDC+1);
			Int iOffset = lSzDiffIntraDC-8;
			uiDiffIntraDC = ( uiDiffIntraDC>>(iOffset+1)<<iOffset )
				      + ( uiDiffIntraDC & ((1<<iOffset)-1) );
*/
			UInt uiDiffIntraDC =
				(UInt) m_pentrdecSet->m_pentrdecIntraDCy->bitstream()->getBits (lSzDiffIntraDC);
			if (!((1 << (lSzDiffIntraDC - 1)) & uiDiffIntraDC))
				iDiffIntraDC = -1 * ((0xFFFF >> (16 - lSzDiffIntraDC)) & (~uiDiffIntraDC));
			else
				iDiffIntraDC = (Int) uiDiffIntraDC;
			m_pentrdecSet->m_pentrdecIntraDCy->bitstream()->getBits (1);
	    }
	}
	return iDiffIntraDC;
}

Void CVideoObjectDecoder::inverseDCACPred (const CMBMode* pmbmd, Int iBlkIdx,
										   Int* rgiCoefQ, Int iQP,
										   Int iDcScaler,
										   const BlockMemory blkmPred,
										   Int iQpPred)
{
	UInt nBits = m_volmd.nBits; // NBIT
	Int iDefVal = 1<<(nBits+2); // NBIT

	//do DC prediction
	if (!short_video_header)
	{		// Added by KPN for short video headers
		if (blkmPred == NULL)
			rgiCoefQ [0] += divroundnearest(iDefVal, iDcScaler);
		else {
			rgiCoefQ [0] += divroundnearest(blkmPred [0], iDcScaler);
			// clip range after inverse pred
			rgiCoefQ [0] = rgiCoefQ[0] < -2048 ? -2048 : (rgiCoefQ[0] > 2047 ? 2047 : rgiCoefQ[0]);
			if (iBlkIdx<(A_BLOCK1 - 1) && pmbmd->m_bACPrediction
				|| iBlkIdx>=(A_BLOCK1 - 1) && pmbmd->m_pbACPredictionAlpha[(iBlkIdx-7)/4])	{
				Int i, j;
				//do AC prediction
				if (pmbmd->m_preddir [iBlkIdx] == HORIZONTAL)	{
					for (i = 8, j = 8; j < 2 * BLOCK_SIZE - 1; i += 8, j++)
					{
						rgiCoefQ [i] += (blkmPred == NULL) ? 0 : (iQP == iQpPred) ?
										 blkmPred [j] : divroundnearest(blkmPred [j] * iQpPred, iQP);
						// clip range after inverse pred
						rgiCoefQ [i] = rgiCoefQ[i] < -2048 ? -2048 : (rgiCoefQ[i] > 2047 ? 2047 : rgiCoefQ[i]);
					}

				}
				else if  (pmbmd->m_preddir [iBlkIdx] == VERTICAL)	{
					//horizontal zigzag scan
					for (i = 1; i < BLOCK_SIZE; i++)
					{
						rgiCoefQ [i] += (blkmPred == NULL) ? 0 : (iQP == iQpPred) ?
										 blkmPred [i] : divroundnearest(blkmPred [i] * iQpPred, iQP);
						// clip range after inverse pred
						rgiCoefQ [i] = rgiCoefQ[i] < -2048 ? -2048 : (rgiCoefQ[i] > 2047 ? 2047 : rgiCoefQ[i]);
					}
				}
				else
					assert (FALSE);
			}
		}
	}// End short video headers
}

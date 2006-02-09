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
	
	blkenc.cpp

Abstract:

	functions for block-level encoding.

Revision History:
	Sep.13	1999 : RRV added by Eishi Morimatsu (Fujitsu Laboratories Ltd.) 

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
#include "dct.hpp"
#include "vopses.hpp"
#include "vopseenc.hpp"

#ifdef __MFC_
#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

#define new DEBUG_NEW				   
#endif // __MFC_

// HHI Schueuer: sadct
CScanSelectorForSADCT::CScanSelectorForSADCT(Int **rgiCurrMBCoeffWidth) :
	m_rgiCurrMBCoeffWidth(rgiCurrMBCoeffWidth)
{
	m_adaptedScan = new Int[BLOCK_SQUARE_SIZE];
}

CScanSelectorForSADCT::~CScanSelectorForSADCT()
{
	delete [] m_adaptedScan;
}         

Int *CScanSelectorForSADCT::select(Int *scan, Bool bIsBoundary, Int iBlk)
{
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
// end

// 09/19/99 HHI Schueuer
Int *CScanSelectorForSADCT::select_DP(Int *scan, Bool bIsBoundary, Int iBlk, Int** rgiCurrMBCoeffWidth)
{
	if (bIsBoundary) {
    	// the incoming scan is the zigzag scan for an 8x8 block which must be modified for
        // sadct coded boundary blocks. 
    	const Int *pCoeffWidth = rgiCurrMBCoeffWidth[iBlk]; 
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
// end 09/19/99

// HHI Schueuer: added const PixelC *rgpxlcBlkShape, Int iBlkShapeWidth for sadct 
Int CVideoObjectEncoder::quantizeIntraBlockTexture (PixelC* ppxlcBlkSrc, 
													 Int iWidthSrc,													 
													 PixelC* ppxlcCurrQBlock, 
													 Int iWidthCurrQ,
													 Int* rgiCoefQ, 
													 Int iQP,
													 Int iDcScaler,
													 Int iBlk, 
													 MacroBlockMemory* pmbmLeft, 
													 MacroBlockMemory* pmbmTop, 
													 MacroBlockMemory* pmbmLeftTop,
													 MacroBlockMemory* pmbmCurr,
													 CMBMode* pmbmdLeft,
													 CMBMode* pmbmdTop,
													 CMBMode* pmbmdLeftTop,
													 CMBMode* pmbmdCurr,
													 const PixelC *rgpxlcBlkShape, 
													 Int iBlkShapeWidth,
                           Int iAuxComp )	
{
//	m_pentrencSet->m_pentrencDCT->bitstream()->trace (ppxlcCurrQBlock, "BLK_TEXTURE");
	// HHI Schueuer: added for sadct
	Int *lx = new Int [iBlkShapeWidth];

	m_pfdct->apply  (ppxlcBlkSrc, iWidthSrc, m_rgiDCTcoef, BLOCK_SIZE, rgpxlcBlkShape, iBlkShapeWidth, lx);

	// RRV insertion
	if(m_vopmd.RRVmode.iOnOff == 1)
	{
		cutoffDCTcoef();
	}
	// ~RRV
	
	Bool bClip = TRUE;
	if(iBlk>=A_BLOCK1 && pmbmdCurr->m_pCODAlpha[iAuxComp] == ALPHA_ALL255)
		bClip = FALSE; // for correct dc value for intra dc prediction

	quantizeIntraDCcoef (rgiCoefQ, (Float) iDcScaler, bClip);
	inverseQuantizeIntraDc (rgiCoefQ, iDcScaler);			//get the quantized block
	if (m_volmd.fQuantizer == Q_H263)	{
		quantizeIntraDCTcoefH263 (rgiCoefQ, 1, iQP);	
		// HHI Schueuer: sadct
		if (rgpxlcBlkShape && !m_volmd.bSadctDisable) {
			// assert(pmbmd->m_rgTranspStatus [iBlk] == PARTIAL);
			// brute force method to clean out mispredictions outside the active region
			// Int *lx = m_rgiCurrMBCoeffWidth[iBlk];
			Int iy, ix;
         
			for (iy = 0; iy < BLOCK_SIZE; iy++) {
				for (ix=lx[iy]; ix<BLOCK_SIZE; ix++)
			  		rgiCoefQ[ix + iy * BLOCK_SIZE] = 0;
			}
		} 		
		//end HHI		
		inverseQuantizeDCTcoefH263 (rgiCoefQ, 1, iQP);		//get the quantized block
	}
	else	{
		quantizeIntraDCTcoefMPEG (rgiCoefQ, 1, iQP, (iBlk > V_BLOCK));	
		// HHI Schueuer: sadct
		if (rgpxlcBlkShape && !m_volmd.bSadctDisable) {
			// assert(pmbmd->m_rgTranspStatus [iBlk] == PARTIAL);
			// brute force method to clean out mispredictions outside the active region
			Int *lx = m_rgiCurrMBCoeffWidth[iBlk];
			Int iy, ix;
         
			for (iy = 0; iy < BLOCK_SIZE; iy++) {
				for (ix=lx[iy]; ix<BLOCK_SIZE; ix++)
			  		rgiCoefQ[ix + iy * BLOCK_SIZE] = 0;
			}
		} 		
		//end
		inverseQuantizeIntraDCTcoefMPEG (rgiCoefQ, 1, iQP, (iBlk > V_BLOCK), 0);											//get the quantized block
	}

	Int i, j;
//	pmbmCurr->rgblkm [iBlk - 1] [0] = rgiCoefQ [0];		//save Qcoef in memory
	pmbmCurr->rgblkm [iBlk - 1] [0] = m_rgiDCTcoef [0];		//save reconstructed DC but quantized AC in memory
	for (i = 1, j = 8; i < BLOCK_SIZE; i++, j += BLOCK_SIZE)	{
		pmbmCurr->rgblkm [iBlk - 1] [i] = rgiCoefQ [i];
		pmbmCurr->rgblkm [iBlk - 1] [i + BLOCK_SIZE - 1] = rgiCoefQ [j];
	}

	Int iSumErr = 0;
	m_rgiQPpred [iBlk - 1] = iQP; //default to current in case no predictor
	iSumErr += decideIntraPredDir (	rgiCoefQ,
								  iBlk,
									&(m_rgblkmCurrMB [iBlk - 1]),
									pmbmLeft, 
  								pmbmTop, 
									pmbmLeftTop,									   
									pmbmCurr,
									pmbmdLeft,
									pmbmdTop,
									pmbmdLeftTop,
									pmbmdCurr,
									m_rgiQPpred [iBlk - 1],
									iQP);
	if(iBlk < A_BLOCK1 || pmbmdCurr->m_pCODAlpha[iAuxComp] == ALPHA_CODED)
		// m_pidct->apply (m_rgiDCTcoef, BLOCK_SIZE, ppxlcCurrQBlock, iWidthCurrQ);
		// HHI Schueuer: rgpxlcBlkShape, iBlkShapeWidth added for sadct
	{
		//if(iBlk==A_BLOCK1 || iBlk==A_BLOCK2)
		//{
		//	printf("[%d,%d]",iQP,rgiCoefQ[0]);
		//}
		m_pidct->apply (m_rgiDCTcoef, BLOCK_SIZE, ppxlcCurrQBlock, iWidthCurrQ, rgpxlcBlkShape, iBlkShapeWidth);
	}
	// end HHI

	// fix memory leak (10000 more to go ;-) - swinder
	delete lx;

	return iSumErr;
}

Void CVideoObjectEncoder::quantizeIntraDCcoef (Int* rgiCoefQ, Float fltDcScaler, Bool bClip)
{
	if (m_volmd.nBits<=8) { // NBIT: may not valid when nBits>8
		assert (fltDcScaler > 0 && fltDcScaler < 128);
	}
#ifdef NO_APPENDIXF
	Float fltDCquantized = (Float) m_rgiDCTcoef [0] / 8.0F;
#else
	Float fltDCquantized = (Float) m_rgiDCTcoef [0] / fltDcScaler;
#endif
	assert (fltDCquantized >= 0);
	fltDCquantized = fltDCquantized + 0.5F;
	if(bClip)
	{
		Int iMaxDC = (1<<m_volmd.nBits) - 2; // NBIT 1..254
		rgiCoefQ [0] = (Int) max (1, min (iMaxDC, fltDCquantized));
	}
	else
		rgiCoefQ [0] = (Int) fltDCquantized;
}

Void CVideoObjectEncoder::quantizeIntraDCTcoefH263 (Int* rgiCoefQ, Int iStart, Int iQP)
{
	Int iLevelBits = 12; // 12 bit FLC (= m_volmd.nBits?)
	Int iMaxAC = (1<<(iLevelBits - 1)) - 1; // NBIT 127
	for (Int i = iStart; i < BLOCK_SQUARE_SIZE; i++) {
		Int ilevel = sign(m_rgiDCTcoef [i]) * (abs (m_rgiDCTcoef [i]) / (2 * iQP));
		rgiCoefQ [i] =  min(iMaxAC, max(-iMaxAC, ilevel));
	}
}

Void CVideoObjectEncoder::quantizeInterDCTcoefH263 (Int* rgiCoefQ, Int iStart, Int iQP)
{
	Int iLevelBits = 12; // 12 bit FLC (= m_volmd.nBits?)
	Int iMaxAC = (1<<(iLevelBits - 1)) - 1; // NBIT 127
	for (Int i = iStart; i < BLOCK_SQUARE_SIZE; i++) {
		Int ilevel = sign(m_rgiDCTcoef [i]) * ((abs (m_rgiDCTcoef [i]) - iQP / 2) / (2 * iQP));
		rgiCoefQ [i] =  min(iMaxAC, max(-iMaxAC, ilevel));
	}
}

Void CVideoObjectEncoder::quantizeIntraDCTcoefMPEG (Int* rgiCoefQ, Int iStart, Int iQP,
													Bool bUseAlphaMatrix)
{
	Float fltScaledCoef;
	Int i, iScaledQP, iScaledCoef;
	Int *piQuantizerMatrix;
	if(bUseAlphaMatrix)
		piQuantizerMatrix = m_volmd.rgiIntraQuantizerMatrixAlpha[0];
	else
		piQuantizerMatrix = m_volmd.rgiIntraQuantizerMatrix;
	Int iMaxVal = 1<<(m_volmd.nBits+3); // NBIT 2048
	Int iLevelBits = 12; // 12 bit FLC (= m_volmd.nBits?)
	Int iMaxAC = (1<<(iLevelBits - 1)) - 1; // NBIT 127

	for (i = iStart; i < BLOCK_SQUARE_SIZE; i++) {
		fltScaledCoef = 16.0F * m_rgiDCTcoef [i] / piQuantizerMatrix [i]; 
		iScaledCoef = (Int) rounded (fltScaledCoef);
		iScaledCoef = checkrange (iScaledCoef, -iMaxVal, iMaxVal - 1);
		iScaledQP = (Int) (3.0F * (Float) iQP / 4.0F + 0.5);
		assert (iScaledQP >= 0);
		iScaledCoef = (iScaledCoef + sign(iScaledCoef) * iScaledQP) / (2 * iQP);
		rgiCoefQ [i]  = min(iMaxAC, max(-iMaxAC, iScaledCoef ));
	}
}

Void CVideoObjectEncoder::quantizeInterDCTcoefMPEG (Int* rgiCoefQ, Int iStart, Int iQP,
													Bool bUseAlphaMatrix)
{
	Float fltScaledCoef;
	Int i, iScaledCoef;
	Int *piQuantizerMatrix;
	if(bUseAlphaMatrix)
		piQuantizerMatrix = m_volmd.rgiInterQuantizerMatrixAlpha[0];
	else
		piQuantizerMatrix = m_volmd.rgiInterQuantizerMatrix;
	Int iLevelBits = 12; // 12 bit FLC (= m_volmd.nBits?)
	Int iMaxAC = (1<<(iLevelBits - 1)) - 1; // NBIT 127

	for (i = iStart; i < BLOCK_SQUARE_SIZE; i++) {
		fltScaledCoef = 16.0F * m_rgiDCTcoef [i] / piQuantizerMatrix [i]; 
		iScaledCoef = (Int) rounded (fltScaledCoef);
		iScaledCoef  = iScaledCoef / (2 * iQP);
		rgiCoefQ [i] = min(iMaxAC, max(-iMaxAC, iScaledCoef ));
	}
}

Int CVideoObject::decideIntraPredDir (Int* rgiCoefQ,
									   Int blkn,
									   const BlockMemory * pblkmRet,
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
									   Bool bDecideDCOnly)
{
	UInt nBits = m_volmd.nBits;
	Int iDefVal = 1<<(nBits+2);// NBIT: start to calculate default value
	Int iRange = (1<<(nBits-1))-1; // 127 for 8-bit

	Int iQPpredTop, iQPpredLeftTop, iQPpredLeft;
	const BlockMemory blkmTop = findPredictorBlock (blkn, VERTICAL,
		pmbmLeft, pmbmTop, pmbmLeftTop, pmbmCurr,
		pmbmdLeft, pmbmdTop, pmbmdLeftTop, pmbmdCurr, iQPpredTop);
	const BlockMemory blkmLeftTop = findPredictorBlock (blkn, DIAGONAL,
		pmbmLeft, pmbmTop, pmbmLeftTop, pmbmCurr,
		pmbmdLeft, pmbmdTop, pmbmdLeftTop, pmbmdCurr, iQPpredLeftTop);
	const BlockMemory blkmLeft = findPredictorBlock (blkn, HORIZONTAL,
		pmbmLeft, pmbmTop, pmbmLeftTop, pmbmCurr,
		pmbmdLeft, pmbmdTop, pmbmdLeftTop, pmbmdCurr, iQPpredLeft);
	//wchen: changed to 1024 per CD (based on recon instead of Q value now)
/* NBIT: change 1024 to iDefVal
	Int iPredLeftTop = (blkmLeftTop == NULL) ? 1024 : blkmLeftTop [0];		
	Int iHorizontalGrad				= ((blkmTop  == NULL) ? 1024 : blkmTop  [0]) - iPredLeftTop;
	Int iVerticalGrad				= ((blkmLeft == NULL) ? 1024 : blkmLeft [0]) - iPredLeftTop;
*/
	Int iPredLeftTop = (blkmLeftTop == NULL) ? iDefVal : blkmLeftTop [0];		
	Int iHorizontalGrad				= ((blkmTop  == NULL) ? iDefVal : blkmTop  [0]) - iPredLeftTop;
	Int iVerticalGrad				= ((blkmLeft == NULL) ? iDefVal : blkmLeft [0]) - iPredLeftTop;

	*pblkmRet = NULL;
	UInt i, j;
	Int iSumErr = 0; //per vm4.0, p53
	if (abs(iVerticalGrad)  < abs (iHorizontalGrad))	{
		pmbmdCurr->m_preddir [blkn - 1] = VERTICAL;
		if (blkmTop != NULL)	{
			*pblkmRet = blkmTop;
			iQPpred = iQPpredTop;
			if (bDecideDCOnly != TRUE)	{
				for (i = 1; i < BLOCK_SIZE; i++)	{
					Int iDiff = rgiCoefQ [i] - divroundnearest(blkmTop [i] * iQPpred, iQPcurr);
					if (iDiff >= -iRange && iDiff <= iRange)	//hack to deal with vm deficiency: ac pred out of range; dc pred is not dealt with
						iSumErr += abs (rgiCoefQ [i]) - abs (iDiff);
					else
						return -100000;	//arbitrary  negative number to turn off ac pred.
				}
			}
		}
	}
	else	{
		pmbmdCurr->m_preddir [blkn - 1] = HORIZONTAL;
		if (blkmLeft != NULL)	{
			*pblkmRet = blkmLeft;
			iQPpred = iQPpredLeft;
			if (bDecideDCOnly != TRUE)	{
				for (i = 8, j = 8; i < BLOCK_SQUARE_SIZE; i += 8, j++)	{
					Int iDiff = rgiCoefQ [i] - divroundnearest(blkmLeft [j] * iQPpred, iQPcurr);
					if (iDiff >= -iRange && iDiff <= iRange)	//hack to deal with vm deficiency: ac pred out of range; dc pred is not dealt with
						iSumErr += abs (rgiCoefQ [i]) - abs (iDiff);
					else
						return -100000;	//arbitrary negative number to turn off ac pred.
				}
			}
		}
	}
	return iSumErr;
}


Void CVideoObjectEncoder::intraPred (Int blkn, const CMBMode* pmbmd, 
									 Int* rgiCoefQ, Int iQPcurr, Int iDcScaler, 
									 const BlockMemory blkmPred, Int iQPpred)
{
	Int iDefVal; // NBIT: start to calculate default value
	UInt nBits = m_volmd.nBits;
	Int iMaxDC = (1<<nBits) - 1;
	Int iLevelBits = 12; // 12 bit FLC (= m_volmd.nBits?)
	Int iMaxAC = (1<<(iLevelBits - 1)) - 1; // NBIT 127
	iDefVal = 1<<(nBits + 2);


	//do DC prediction
	if (blkmPred == NULL)
/* NBIT: change 1024 to iDefVal
		rgiCoefQ [0] -= (1024 + (iDcScaler >> 1)) / iDcScaler;
*/
		rgiCoefQ [0] -= divroundnearest(iDefVal, iDcScaler);
	else {
		rgiCoefQ [0] -= divroundnearest(blkmPred [0], iDcScaler);
/* NBIT: change 255 to iMaxVal
		assert (rgiCoefQ [0] >= -255 && rgiCoefQ [0] <= 255);
*/
		assert (rgiCoefQ [0] >= -iMaxDC && rgiCoefQ [0] <= iMaxDC);

		if ((blkn < A_BLOCK1 && pmbmd->m_bACPrediction)
			|| (blkn >=A_BLOCK1 && pmbmd->m_pbACPredictionAlpha[(blkn-7)/4]))
		{
			Int i, j;
			//do AC prediction
			if (pmbmd->m_preddir [blkn - 1] == HORIZONTAL)	{
				for (i = 8, j = 8; j < 2 * BLOCK_SIZE - 1; i += 8, j++)	{
					rgiCoefQ [i] -= (blkmPred == NULL) ? 0 : (iQPcurr == iQPpred) ?
									 blkmPred [j] : divroundnearest(blkmPred [j] * iQPpred, iQPcurr);
					assert (rgiCoefQ [i] >= -iMaxAC && rgiCoefQ [i]  <= iMaxAC);
				}

			}
			else if  (pmbmd->m_preddir [blkn - 1] == VERTICAL)	{
				//horizontal zigzag scan
				for (i = 1; i < BLOCK_SIZE; i++)	{
					rgiCoefQ [i] -= (blkmPred == NULL) ? 0 : (iQPcurr == iQPpred) ?
									 blkmPred [i] : divroundnearest(blkmPred [i] * iQPpred, iQPcurr);
					assert (rgiCoefQ [i] >= -iMaxAC && rgiCoefQ [i] <= iMaxAC);
				}
			}
			else
				assert (FALSE);
		}
	}
}

// HHI Schueuer: added const PixelC *rgpxlcBlkShape, Int iBlkShapeWidth
Void CVideoObjectEncoder::quantizeTextureInterBlock (PixelI* ppxliCurrQBlock, 
													 Int iWidthCurrQ,
													 Int* rgiCoefQ, 
													 Int iQP,
													 Bool bAlphaBlock,
													 const PixelC *rgpxlcBlkShape, 
													 Int iBlkShapeWidth,
													 Int iBlk)	
{
//	m_pentrencSet->m_pentrencDCT->bitstream()->trace (rgiCoefQ, "BLK_TEXTURE");

// HHI Schueuer : rgpxlcBlkShape, iBlkShapeWidth, lx added
	Int *lx = new Int [iBlkShapeWidth];
	m_pfdct->apply  (ppxliCurrQBlock, iWidthCurrQ, m_rgiDCTcoef, BLOCK_SIZE, rgpxlcBlkShape, iBlkShapeWidth, lx);
	// end HHI
// RRV insertion
	if(m_vopmd.RRVmode.iOnOff == 1)
	{
		cutoffDCTcoef();
	}
// ~RRV

	if (m_volmd.fQuantizer == Q_H263)	{
		quantizeInterDCTcoefH263 (rgiCoefQ, 0, iQP);	
		// HHI Schueuer
		if (rgpxlcBlkShape && !m_volmd.bSadctDisable) {
			// assert(pmbmd->m_rgTranspStatus [iBlk] == PARTIAL);
			// brute force method to clean out mispredictions outside the active region
			// Int *lx = m_rgiCurrMBCoeffWidth[iBlk];
			Int iy, ix;
	         
			for (iy = 0; iy < BLOCK_SIZE; iy++) {
				for (ix=lx[iy]; ix<BLOCK_SIZE; ix++)
          			rgiCoefQ[ix + iy * BLOCK_SIZE] = 0;
			}
		}
		// end HHI		
		inverseQuantizeDCTcoefH263 (rgiCoefQ, 0, iQP);
	}
	else	{
		quantizeInterDCTcoefMPEG (rgiCoefQ, 0, iQP, bAlphaBlock);	
		// HHI Schueuer
		if (rgpxlcBlkShape && !m_volmd.bSadctDisable) {
			// assert(pmbmd->m_rgTranspStatus [iBlk] == PARTIAL);
			// brute force method to clean out mispredictions outside the active region
			// Int *lx = m_rgiCurrMBCoeffWidth[iBlk];
			Int iy, ix;
	         
			for (iy = 0; iy < BLOCK_SIZE; iy++) {
				for (ix=lx[iy]; ix<BLOCK_SIZE; ix++)
          			rgiCoefQ[ix + iy * BLOCK_SIZE] = 0;
			}
		}
		// end HHI		
		inverseQuantizeInterDCTcoefMPEG (rgiCoefQ, 0, iQP, bAlphaBlock, 0);
	}

	// m_pidct->apply (m_rgiDCTcoef, BLOCK_SIZE, ppxliCurrQBlock, iWidthCurrQ);
	// HHI Schueuer : added   rgpxlcBlkShape, iBlkShapeWidth, lx
	m_pidct->apply (m_rgiDCTcoef, BLOCK_SIZE, ppxliCurrQBlock, iWidthCurrQ, rgpxlcBlkShape, iBlkShapeWidth, lx);
	delete lx;
	// end HHI
//	m_pentrencSet->m_pentrencDCT->bitstream()->trace (ppxliCurrQBlock, "BLK_TEXTURE_Q");
}

UInt CVideoObjectEncoder::sendIntraDC (const Int* rgiCoefQ, Int blkn)
{
	UInt nBits = 0;
	UInt nBitsPerPixel = m_volmd.nBits; // NBIT
	Int iMaxVal = 1<<nBitsPerPixel; // NBIT

	assert (rgiCoefQ [0] >= -iMaxVal && rgiCoefQ [0] < iMaxVal);
	Int iAbsDiffIntraDC = abs (rgiCoefQ [0]);
	Long lSzDiffIntraDC = 0;
	for (Int i = nBitsPerPixel; i > 0; i--)
		if (iAbsDiffIntraDC & (1 << (i - 1)))	{
			lSzDiffIntraDC	= i;
			break;
		}

	COutBitStream* pobstrmIntraDC;
	if (blkn == U_BLOCK || blkn == V_BLOCK)	{
		nBits = m_pentrencSet->m_pentrencIntraDCc->encodeSymbol(lSzDiffIntraDC, "IntraDClen");		// huffman encode 
		pobstrmIntraDC = m_pentrencSet->m_pentrencIntraDCc -> bitstream ();
	}
	else	{
		nBits = m_pentrencSet->m_pentrencIntraDCy->encodeSymbol(lSzDiffIntraDC, "IntraDClen");		// huffman encode 
		pobstrmIntraDC = m_pentrencSet->m_pentrencIntraDCy -> bitstream ();
	}
	if (lSzDiffIntraDC<=8) { // NBIT
		    if (rgiCoefQ [0] > 0)
				pobstrmIntraDC->putBits ((Char) rgiCoefQ [0], lSzDiffIntraDC, "IntraDC");	//fix length code
		    else if (rgiCoefQ [0] < 0)
				pobstrmIntraDC->putBits (~iAbsDiffIntraDC, lSzDiffIntraDC, "IntraDC");		//fix length code
		    nBits += lSzDiffIntraDC;
	} else { // NBIT: MARKER bit inserted after first 8 bits
		 //   UInt uiOffset = lSzDiffIntraDC-8;
		    UInt uiValue = iAbsDiffIntraDC;
		    if (rgiCoefQ [0] < 0) {
				uiValue = ~iAbsDiffIntraDC;
		    }
/*
		    uiValue = ( (uiValue>>uiOffset)<<(uiOffset+1) )
				+ ( 1<<uiOffset )
			   + ( uiValue&((1<<uiOffset)-1) );
		    pobstrmIntraDC->putBits (uiValue, lSzDiffIntraDC+1, "IntraDC");
*/
		    pobstrmIntraDC->putBits (uiValue, lSzDiffIntraDC, "IntraDC");
		    pobstrmIntraDC->putBits (1, 1, "Marker");
		    nBits += lSzDiffIntraDC+1;
	}
	return nBits;
}

UInt CVideoObjectEncoder::sendTCOEFIntra (const Int* rgiCoefQ, Int iStart, Int* rgiZigzag)
{
	Bool bIsFirstRun = TRUE;
	Bool bIsLastRun = FALSE;
	UInt uiCurrRun = 0;
	UInt uiPrevRun = 0;
	//	Int	 iCurrLevel = 0;
	Int  iPrevLevel = 0;
	//	UInt uiCoefToStart = 0;
	UInt numBits = 0;

	for (Int j = iStart; j < BLOCK_SQUARE_SIZE; j++) {
		if (rgiCoefQ [rgiZigzag [j]] == 0)								// zigzag here
			uiCurrRun++;											// counting zeros
		else {
			if (!bIsFirstRun)
				numBits += putBitsOfTCOEFIntra (uiPrevRun, iPrevLevel, bIsLastRun);
			uiPrevRun = uiCurrRun; 									// reset for next run
			iPrevLevel = rgiCoefQ [rgiZigzag [j]]; 
			uiCurrRun = 0;											
			bIsFirstRun = FALSE;										
		}
	}
	assert (uiPrevRun <= (BLOCK_SQUARE_SIZE - 1) - 1);				// Some AC must be non-zero; at least for inter
	bIsLastRun = TRUE;
	numBits += putBitsOfTCOEFIntra (uiPrevRun, iPrevLevel, bIsLastRun);
	return numBits;
}


UInt CVideoObjectEncoder::sendTCOEFInter (const Int* rgiCoefQ, Int iStart, Int* rgiZigzag)
{
	Bool bIsFirstRun = TRUE;
	Bool bIsLastRun = FALSE;
	UInt uiCurrRun = 0;
	UInt uiPrevRun = 0;
	//	Int	 iCurrLevel = 0;
	Int  iPrevLevel = 0;
	UInt numBits = 0;

	for (Int j = iStart; j < BLOCK_SQUARE_SIZE; j++) {
		if (rgiCoefQ [rgiZigzag [j]] == 0)								// zigzag here
			uiCurrRun++;											// counting zeros
		else {
			if (!bIsFirstRun)
				numBits += putBitsOfTCOEFInter (uiPrevRun, iPrevLevel, bIsLastRun);
			uiPrevRun = uiCurrRun; 									// reset for next run
			iPrevLevel = rgiCoefQ [rgiZigzag [j]]; 
			uiCurrRun = 0;											
			bIsFirstRun = FALSE;										
		}
	}
	assert (uiPrevRun <= (BLOCK_SQUARE_SIZE - 1));				// Some AC must be non-zero; at least for inter
	bIsLastRun = TRUE;
	numBits += putBitsOfTCOEFInter (uiPrevRun, iPrevLevel, bIsLastRun);
	return numBits;
}

UInt CVideoObjectEncoder::putBitsOfTCOEFInter (UInt uiRun, Int iLevel, Bool bIsLastRun)
{
	UInt nBits = 0;
	Long lVLCtableIndex;
	if (bIsLastRun == FALSE) {
		lVLCtableIndex = findVLCtableIndexOfNonLastEvent (bIsLastRun, uiRun, abs(iLevel));
		if (lVLCtableIndex != NOT_IN_TABLE)  {
			nBits += m_pentrencSet->m_pentrencDCT->encodeSymbol(lVLCtableIndex, "Vlc_TCOEF");	// huffman encode 
			m_pentrencSet->m_pentrencDCT->bitstream()->putBits ((Char) invSignOf (iLevel), 1, "Sign_TCOEF");			
			nBits++;
		}
		else
			nBits += escapeEncode (uiRun, iLevel, bIsLastRun, g_rgiLMAXinter, g_rgiRMAXinter, 
			USE_NON_LAST_EVENT_INDEX);	
			//findVLCtableIndexOfNonLastEvent);
	}
	else	{
		lVLCtableIndex = findVLCtableIndexOfLastEvent (bIsLastRun, uiRun, abs(iLevel));
		if (lVLCtableIndex != NOT_IN_TABLE)  {
			nBits += m_pentrencSet->m_pentrencDCT->encodeSymbol(lVLCtableIndex, "Vlc_TCOEF");	// huffman encode 
			m_pentrencSet->m_pentrencDCT->bitstream()->putBits ((Char) invSignOf (iLevel), 1, "Sign_TCOEF");			
			nBits++;
		}
		else
			nBits += escapeEncode (uiRun, iLevel, bIsLastRun, g_rgiLMAXinter, g_rgiRMAXinter, USE_LAST_EVENT_INDEX); //&CVideoObjectEncoder::findVLCtableIndexOfLastEvent);
	}
	return nBits;
}



UInt CVideoObjectEncoder::putBitsOfTCOEFIntra (UInt uiRun, Int iLevel, Bool bIsLastRun)
{
	//fprintf(stderr,"%d %d %d\n", iLevel,uiRun,bIsLastRun);

	Int nBits = 0;
	Long lVLCtableIndex;
	lVLCtableIndex = findVLCtableIndexOfIntra (bIsLastRun, uiRun, abs(iLevel));

	if (lVLCtableIndex != NOT_IN_TABLE)  {
		nBits += m_pentrencSet->m_pentrencDCTIntra->encodeSymbol(lVLCtableIndex, "Vlc_TCOEF");	// huffman encode 
		m_pentrencSet->m_pentrencDCTIntra->bitstream()->putBits ((Char) invSignOf (iLevel), 1, "Sign_TCOEF");			
		nBits++;
	}
	else
		nBits += escapeEncode (uiRun, iLevel, bIsLastRun, g_rgiLMAXintra, g_rgiRMAXintra, USE_INFRA_INDEX); //&CVideoObjectEncoder::findVLCtableIndexOfIntra);

	return nBits;
}

UInt CVideoObjectEncoder::escapeEncode (UInt uiRun, Int iLevel, Bool bIsLastRun, Int* rgiLMAX, Int* rgiRMAX, 
										int findVLCtableIndex)
{
	UInt nBits = 0;
	nBits += m_pentrencSet->m_pentrencDCT->encodeSymbol(TCOEF_ESCAPE, "Esc_TCOEF");
	Int iLevelAbs = abs (iLevel);
	Int iLevelPlus = iLevelAbs - rgiLMAX [(uiRun & 0x0000003F) + (bIsLastRun << 6)];		//hashing the table
	Int iVLCtableIndex;
	if (findVLCtableIndex == USE_NON_LAST_EVENT_INDEX)
		iVLCtableIndex = findVLCtableIndexOfNonLastEvent(bIsLastRun, uiRun, abs (iLevelPlus));
	else if (findVLCtableIndex == USE_LAST_EVENT_INDEX)
		iVLCtableIndex = findVLCtableIndexOfLastEvent(bIsLastRun, uiRun, abs (iLevelPlus));
	else
		iVLCtableIndex = findVLCtableIndexOfIntra(bIsLastRun, uiRun, abs (iLevelPlus));
	if (iVLCtableIndex != NOT_IN_TABLE)  {
		m_pbitstrmOut->putBits (0, 1, "Esc_0");			
		nBits++;
		nBits += m_pentrencSet->m_pentrencDCT->encodeSymbol(iVLCtableIndex, "Esc_1_Vlc_TCOEF");	// huffman encode 
		m_pbitstrmOut->putBits ((Char) invSignOf (iLevel), 1, "Sign_TCOEF");
		nBits++;
	}
	else	{
		Int iRunPlus = uiRun - rgiRMAX [(iLevelAbs & 0x0000001F) + (bIsLastRun << 5)];  //RMAX table includes + 1 already
		if (findVLCtableIndex == USE_NON_LAST_EVENT_INDEX)
			iVLCtableIndex = findVLCtableIndexOfNonLastEvent(bIsLastRun, (UInt) iRunPlus, iLevelAbs);
		else if (findVLCtableIndex == USE_LAST_EVENT_INDEX)
			iVLCtableIndex = findVLCtableIndexOfLastEvent(bIsLastRun, (UInt) iRunPlus, iLevelAbs);
		else
			iVLCtableIndex = findVLCtableIndexOfIntra(bIsLastRun, (UInt) iRunPlus, iLevelAbs);
//		iVLCtableIndex = (this->*findVLCtableIndex) (bIsLastRun, (UInt) iRunPlus, iLevelAbs);
		if (iVLCtableIndex != NOT_IN_TABLE)  {
			m_pbitstrmOut->putBits (2, 2, "Esc_10");			
			nBits += 2;
			nBits += m_pentrencSet->m_pentrencDCT->encodeSymbol(iVLCtableIndex, "Esc_01_Vlc_TCOEF");	// huffman encode 
			m_pbitstrmOut->putBits ((Char) invSignOf (iLevel), 1, "Sign_TCOEF");			
			nBits++;
		}
		else	{
			m_pbitstrmOut->putBits (3, 2, "Esc_11");
			nBits += 2;
			nBits += fixLengthCode (uiRun, iLevel, bIsLastRun);
		}
	}
	return nBits;
}

UInt CVideoObjectEncoder::fixLengthCode (UInt uiRun, Int iLevel, Bool bIsLastRun)
{
	UInt nBits = 0;
	m_pentrencSet->m_pentrencDCT->bitstream()->putBits ((Char) bIsLastRun, 1, "Last_Run_TCOEF");
	nBits++;

	assert (uiRun < BLOCK_SQUARE_SIZE);
	m_pentrencSet->m_pentrencDCT->bitstream()->putBits (uiRun, NUMBITS_ESC_RUN, "Run_Esc_TCOEF");
	nBits+=NUMBITS_ESC_RUN;

	Int iLevelBits = 12; // 12 bit FLC (= m_volmd.nBits?)
	Int iMaxAC = (1<<(iLevelBits - 1)) - 1; // NBIT
	assert (iLevel >= -iMaxAC && iLevel <= iMaxAC && iLevel != 0);
	if (iLevel < 0)
		iLevel = (1<<iLevelBits) - abs(iLevel);
	m_pentrencSet->m_pentrencDCT->bitstream()->putBits (1, 1, "Marker");
	m_pentrencSet->m_pentrencDCT->bitstream()->putBits (iLevel, iLevelBits, "Level_Esc_TCOEF");	
	m_pentrencSet->m_pentrencDCT->bitstream()->putBits (1, 1, "Marker");
	nBits += iLevelBits + 2;
	
	return nBits;
}

Int CVideoObjectEncoder::findVLCtableIndexOfNonLastEvent (Bool bIsLastRun, UInt uiRun, UInt uiLevel)
{

	assert (uiRun >= 0);
	if (uiRun > 26 || (uiLevel > grgIfNotLastNumOfLevelAtRun [uiRun]))
		return NOT_IN_TABLE;
	else	{
		UInt uiTableIndex = 0;	
		for (UInt i = 0; i < uiRun; i++)
			uiTableIndex += grgIfNotLastNumOfLevelAtRun [i];
		uiTableIndex += uiLevel;
		uiTableIndex--;												// make it zero-based; see Table H13/H.263
		return uiTableIndex;
	}
}

Int CVideoObjectEncoder::findVLCtableIndexOfLastEvent (Bool bIsLastRun, UInt uiRun, UInt uiLevel)
{
	assert (uiRun >= 0);
	if (uiRun > 40 || (uiLevel > grgIfLastNumOfLevelAtRun [uiRun]))
		return NOT_IN_TABLE;
	else	{
		UInt uiTableIndex = 0;	
		for (UInt i = 0; i < uiRun; i++)
			uiTableIndex += grgIfLastNumOfLevelAtRun [i];
		uiTableIndex += uiLevel;		
		uiTableIndex += 57;									// make it zero-based and 
		return uiTableIndex;								//correction of offset; see Table H13/H.263
	}
}

Int CVideoObjectEncoder::findVLCtableIndexOfIntra (Bool bIsLastRun, UInt uiRun, UInt uiLevel)
{
	UInt i;
	assert (uiRun >= 0);
	if (uiRun > 20 || uiLevel > 27)
		return NOT_IN_TABLE;
	else	{
	  //		UInt uiTableIndex = 0;	
		for (i = 0; i < TCOEF_ESCAPE; i++) {
			UInt iHashing = (bIsLastRun << 10) | (uiRun << 5) | uiLevel;
			if (iHashing == grgiIntraYAVCLHashingTable [i])	
				return i;
		}
		return NOT_IN_TABLE;
	}
}

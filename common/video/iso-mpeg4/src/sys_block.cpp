/*************************************************************************

This software module was originally developed by 

	Ming-Chieh Lee (mingcl@microsoft.com), Microsoft Corporation
	Wei-ge Chen (wchen@microsoft.com), Microsoft Corporation
	Bruce Lin (blin@microsoft.com), Microsoft Corporation
	Chuang Gu (chuanggu@microsoft.com), Microsoft Corporation
	Simon Winder (swinder@microsoft.com), Microsoft Corporation
	(date: March, 1996)
and edited by
	Wei Wu (weiwu@stallion.risc.rockwell.com), Rockwell Science Center

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

	block.cpp

Abstract:

	Block base class

Revision History:
        May. 9   1998:  add boundary by Hyundai Electronics 
                                  Cheol-Soo Park (cspark@super5.hyundai.co.kr) 
*************************************************************************/

#include <stdlib.h>
#include <math.h>
#include "typeapi.h"
#include "codehead.h"
#include "mode.hpp"
#ifndef __GLOBAL_VAR_
#define __GLOBAL_VAR_
#endif
#include "global.hpp"
#include "bitstrm.hpp"
#include "entropy.hpp"
#include "huffman.hpp"
#include "dct.hpp"
#include "vopses.hpp"

#ifdef __MFC_
#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

#define new DEBUG_NEW				   
#endif // __MFC_

Int divroundnearest(Int i, Int iDenom)
{
	assert(iDenom>0);
	if(i>=0)
		return (i+(iDenom>>1))/iDenom;
	else
		return (i-(iDenom>>1))/iDenom;
}

Void CVideoObject::inverseQuantizeIntraDc (Int* rgiCoefQ, Int iDcScaler)
{
#ifdef NO_APPENDIXF
		m_rgiDCTcoef [0] = (Float) rgiCoefQ [0] * 8;
#else
		m_rgiDCTcoef [0] = rgiCoefQ [0] * iDcScaler;
/*
		if (iQP <= 4)
			m_rgiDCTcoef [0] = rgiCoefQ [0] * 8;
		else if (iQP >= 5 && iQP <= 8)
			m_rgiDCTcoef [0] = rgiCoefQ [0] * 2 * iQP;
		else if (iQP >= 9 && iQP <= 24)
			m_rgiDCTcoef [0] = rgiCoefQ [0] * (iQP + 8);
		else
			m_rgiDCTcoef [0] = rgiCoefQ [0] * (iQP * 2 - 16);
*/
#endif
}

Void CVideoObject::inverseQuantizeDCTcoefH263 (Int* rgiCoefQ, Int iStart, Int iQP)
{
	Int i;
	for (i = iStart; i < BLOCK_SQUARE_SIZE; i++) {
		if (rgiCoefQ[i]) {
			if (iQP % 2 == 1)			//?
				m_rgiDCTcoef [i] = iQP * (2 * abs (rgiCoefQ[i]) + 1);
			else
				m_rgiDCTcoef [i] = iQP * (2 * abs (rgiCoefQ[i]) + 1) - 1;
			m_rgiDCTcoef [i] = sign (rgiCoefQ[i]) * m_rgiDCTcoef [i] ;
		}
		else
			m_rgiDCTcoef [i] = 0;
	}
}

Void CVideoObject::inverseQuantizeIntraDCTcoefMPEG (Int* rgiCoefQ, Int iStart, Int iQP,
													Bool bUseAlphaMatrix, Int iAuxComp)
{
	assert (iQP != 0);
	Int iSum = m_rgiDCTcoef [0];
	Bool bCoefQAllZero = (m_rgiDCTcoef [0] == 0)? TRUE : FALSE;
	Int *piQuantizerMatrix;
	if(bUseAlphaMatrix)
		piQuantizerMatrix = m_volmd.rgiIntraQuantizerMatrixAlpha[iAuxComp];
	else
		piQuantizerMatrix = m_volmd.rgiIntraQuantizerMatrix;

	Int iMaxVal = 1<<(m_volmd.nBits+3); // NBIT
	UInt i;
	for (i = iStart; i < BLOCK_SQUARE_SIZE; i++) {
		if (rgiCoefQ [i] == 0)
			m_rgiDCTcoef [i] = 0;
		else {
			m_rgiDCTcoef [i] = iQP * rgiCoefQ [i] * piQuantizerMatrix [i] / 8;
/* NBIT: change
			m_rgiDCTcoef [i] = checkrange (m_rgiDCTcoef [i], -2048, 2047);
*/
			m_rgiDCTcoef [i] = checkrange (m_rgiDCTcoef [i], -iMaxVal, iMaxVal-1);
			bCoefQAllZero = FALSE;
		}
		iSum ^= m_rgiDCTcoef [i];
	}
	if (!bCoefQAllZero) {
		if ((iSum & 0x00000001) == 0)
			m_rgiDCTcoef [i - 1] ^= 0x00000001;
	}
}

Void CVideoObject::inverseQuantizeInterDCTcoefMPEG (Int* rgiCoefQ, Int iStart, Int iQP,
													Bool bUseAlphaMatrix, Int iAuxComp)
{
	assert (iQP != 0);
	Int iSum = 0;
	Bool bCoefQAllZero = TRUE;
	Int *piQuantizerMatrix;
	if(bUseAlphaMatrix)
		piQuantizerMatrix = m_volmd.rgiInterQuantizerMatrixAlpha[iAuxComp];
	else
		piQuantizerMatrix = m_volmd.rgiInterQuantizerMatrix;

	Int iMaxVal = 1<<(m_volmd.nBits+3); // NBIT
	UInt i;
	for (i = iStart; i < BLOCK_SQUARE_SIZE; i++)	{
		if (rgiCoefQ [i] == 0)
			m_rgiDCTcoef [i] = 0;
		else {
			m_rgiDCTcoef [i] = (iQP * (rgiCoefQ [i] * 2 + sign(rgiCoefQ [i])) * piQuantizerMatrix [i]) / 16;
/* NBIT: change
			m_rgiDCTcoef [i] = checkrange (m_rgiDCTcoef [i], -2048, 2047);
*/
			m_rgiDCTcoef [i] = checkrange (m_rgiDCTcoef [i], -iMaxVal, iMaxVal-1);
			bCoefQAllZero = FALSE;
		}
		iSum ^= m_rgiDCTcoef [i];
	}
	if (!bCoefQAllZero) {
		if ((iSum & 0x00000001) == 0)
			m_rgiDCTcoef [i - 1] ^= 0x00000001;
	}
}

const BlockMemory CVideoObject::findPredictorBlock (
	Int iBlkOrig, 
	IntraPredDirection predDir,
	const MacroBlockMemory* pmbmLeft, 
	const MacroBlockMemory* pmbmTop,
	const MacroBlockMemory* pmbmLeftTop,
	const MacroBlockMemory* pmbmCurr,
	const CMBMode* pmbmdLeft, 
	const CMBMode* pmbmdTop,
	const CMBMode* pmbmdLeftTop,
	const CMBMode* pmbmdCurr,
	Int& iQPpred
)
{
	const BlockMemory blkmRet = NULL;
// MAC
  Int iBlk, iAuxComp = 0;
  if (iBlkOrig<7)
    iBlk = iBlkOrig;
  else {
    iAuxComp = (iBlkOrig-7)/4;
    iBlk = ((iBlkOrig-7)&3)+7;
  }
//~MAC
  
	/*BBM// Added for Boundary by Hyundai(1998-5-9)
	if (m_vopmd.bInterlace && pmbmdCurr->m_bMerged [0])
                swapTransparentModes ((CMBMode*)pmbmdCurr, BBM);
	// End of Hyundai(1998-5-9)*/
	if (predDir == HORIZONTAL)	{
		switch (iBlk)	{
		case Y_BLOCK1:
			if (pmbmLeft != NULL && 
				(pmbmdLeft->m_dctMd == INTRA || pmbmdLeft->m_dctMd == INTRAQ) &&
				pmbmdLeft->m_rgTranspStatus [Y_BLOCK2] != ALL)	{
				blkmRet = pmbmLeft->rgblkm [Y_BLOCK2 - 1];
				iQPpred = pmbmdLeft->m_stepSize;
			}
			break;
		case Y_BLOCK2:
			if (pmbmdCurr->m_rgTranspStatus [Y_BLOCK1] != ALL)	{
				blkmRet = pmbmCurr->rgblkm [Y_BLOCK1 - 1];
				iQPpred = pmbmdCurr->m_stepSize;
			}
			break;
		case Y_BLOCK3:
			if (pmbmLeft != NULL && 
				(pmbmdLeft->m_dctMd == INTRA || pmbmdLeft->m_dctMd == INTRAQ) &&
				pmbmdLeft->m_rgTranspStatus [Y_BLOCK4] != ALL)	{
				blkmRet = pmbmLeft->rgblkm [Y_BLOCK4 - 1];
				iQPpred = pmbmdLeft->m_stepSize;
			}
			break;
		case Y_BLOCK4:
			if (pmbmdCurr->m_rgTranspStatus [Y_BLOCK3] != ALL)	{
				blkmRet = pmbmCurr->rgblkm [Y_BLOCK3 - 1];
				iQPpred = pmbmdCurr->m_stepSize;	
			}
			break;
		case A_BLOCK1:
			if (pmbmLeft != NULL && 
				(pmbmdLeft->m_dctMd == INTRA || pmbmdLeft->m_dctMd == INTRAQ) &&
				pmbmdLeft->m_rgTranspStatus [Y_BLOCK2] != ALL)	{
				blkmRet = pmbmLeft->rgblkm [A_BLOCK2 + iAuxComp*4 - 1]; 
				iQPpred = pmbmdLeft->m_stepSizeAlpha;	
			}
			break;
		case A_BLOCK2:
			if (pmbmdCurr->m_rgTranspStatus [Y_BLOCK1] != ALL)	{
				blkmRet = pmbmCurr->rgblkm [A_BLOCK1 + iAuxComp*4 - 1]; 
				iQPpred = pmbmdCurr->m_stepSizeAlpha;	
			}
			break;
		case A_BLOCK3:
			if (pmbmLeft != NULL && 
				(pmbmdLeft->m_dctMd == INTRA || pmbmdLeft->m_dctMd == INTRAQ) &&
				pmbmdLeft->m_rgTranspStatus [Y_BLOCK4] != ALL)	{
				blkmRet = pmbmLeft->rgblkm [A_BLOCK4 + iAuxComp*4 - 1];
				iQPpred = pmbmdLeft->m_stepSizeAlpha;	
			}
			break;
		case A_BLOCK4:
			if (pmbmdCurr->m_rgTranspStatus [Y_BLOCK3] != ALL)	{
				blkmRet = pmbmCurr->rgblkm [A_BLOCK3 + iAuxComp*4 - 1];        
				iQPpred = pmbmdCurr->m_stepSizeAlpha;	
			}
			break;
		default:								//U, V block
			if (pmbmLeft != NULL && 
				(pmbmdLeft->m_dctMd == INTRA || pmbmdLeft->m_dctMd == INTRAQ) &&
				pmbmdLeft->m_rgTranspStatus [ALL_Y_BLOCKS] != ALL)	{
				blkmRet = pmbmLeft->rgblkm [iBlk - 1];
				iQPpred = pmbmdLeft->m_stepSize;	
			}
		}
	}
	else if  (predDir == VERTICAL)	{
		switch (iBlk)	{
		case Y_BLOCK1:
			if (pmbmTop != NULL && 
				(pmbmdTop->m_dctMd == INTRA || pmbmdTop->m_dctMd == INTRAQ) &&
				pmbmdTop->m_rgTranspStatus [Y_BLOCK3] != ALL)	{
				blkmRet = pmbmTop->rgblkm [Y_BLOCK3 - 1];
				iQPpred = pmbmdTop->m_stepSize;	
			}
			break;
		case Y_BLOCK2:
			if (pmbmTop != NULL && 
				(pmbmdTop->m_dctMd == INTRA || pmbmdTop->m_dctMd == INTRAQ) &&
				pmbmdTop->m_rgTranspStatus [Y_BLOCK4] != ALL)	{
				blkmRet = pmbmTop->rgblkm [Y_BLOCK4 - 1];
				iQPpred = pmbmdTop->m_stepSize;	
			}
			break;
		case Y_BLOCK3:
			if (pmbmdCurr->m_rgTranspStatus [Y_BLOCK1] != ALL)	{
				blkmRet = pmbmCurr->rgblkm [Y_BLOCK1 - 1];
				iQPpred = pmbmdCurr->m_stepSize;	
			}
			break;
		case Y_BLOCK4:
			if (pmbmdCurr->m_rgTranspStatus [Y_BLOCK2] != ALL)	{
				blkmRet = pmbmCurr->rgblkm [Y_BLOCK2 - 1];
				iQPpred = pmbmdCurr->m_stepSize;	
			}
			break;
		case A_BLOCK1:
			if (pmbmTop != NULL && 
				(pmbmdTop->m_dctMd == INTRA || pmbmdTop->m_dctMd == INTRAQ) &&
				pmbmdTop->m_rgTranspStatus [Y_BLOCK3] != ALL)	{
				blkmRet = pmbmTop->rgblkm [A_BLOCK3 + iAuxComp*4 - 1];
				iQPpred = pmbmdTop->m_stepSizeAlpha;	
			}
			break;
		case A_BLOCK2:
			if (pmbmTop != NULL && 
				(pmbmdTop->m_dctMd == INTRA || pmbmdTop->m_dctMd == INTRAQ) &&
				pmbmdTop->m_rgTranspStatus [Y_BLOCK4] != ALL)	{
				blkmRet = pmbmTop->rgblkm [A_BLOCK4 + iAuxComp*4 - 1];
				iQPpred = pmbmdTop->m_stepSizeAlpha;	
			}
			break;
		case A_BLOCK3:
			if (pmbmdCurr->m_rgTranspStatus [Y_BLOCK1] != ALL)	{
				blkmRet = pmbmCurr->rgblkm [A_BLOCK1 + iAuxComp*4 - 1];
				iQPpred = pmbmdCurr->m_stepSizeAlpha;	
			}
			break;
		case A_BLOCK4:
			if (pmbmdCurr->m_rgTranspStatus [Y_BLOCK2] != ALL)	{
				blkmRet = pmbmCurr->rgblkm [A_BLOCK2 + iAuxComp*4 - 1];
				iQPpred = pmbmdCurr->m_stepSizeAlpha;	
			}
			break;
		default:								//U, V block
			if (pmbmTop != NULL && 
				(pmbmdTop->m_dctMd == INTRA || pmbmdTop->m_dctMd == INTRAQ) &&
				pmbmdTop->m_rgTranspStatus [ALL_Y_BLOCKS] != ALL)	{
				blkmRet = pmbmTop->rgblkm [iBlk - 1];
				iQPpred = pmbmdTop->m_stepSize;	
			}
		}
	}
	else if (predDir == DIAGONAL)	{
		switch (iBlk)	{
		case Y_BLOCK1:
			if (pmbmLeftTop != NULL && 
				(pmbmdLeftTop->m_dctMd == INTRA || pmbmdLeftTop->m_dctMd == INTRAQ) &&
				pmbmdLeftTop->m_rgTranspStatus [Y_BLOCK4] != ALL)	{
				blkmRet = pmbmLeftTop->rgblkm [Y_BLOCK4 - 1];
				iQPpred = pmbmdLeftTop->m_stepSize;	
			}
			break;
		case Y_BLOCK2:
			if (pmbmTop != NULL && 
				(pmbmdTop->m_dctMd == INTRA || pmbmdTop->m_dctMd == INTRAQ) &&
				pmbmdTop->m_rgTranspStatus [Y_BLOCK3] != ALL)	{
				blkmRet = pmbmTop->rgblkm [Y_BLOCK3 - 1];
				iQPpred = pmbmdTop->m_stepSize;	
			}
			break;
		case Y_BLOCK3:
			if (pmbmLeft != NULL && 
				(pmbmdLeft->m_dctMd == INTRA || pmbmdLeft->m_dctMd == INTRAQ) &&
				pmbmdLeft->m_rgTranspStatus [Y_BLOCK2] != ALL)	{
				blkmRet = pmbmLeft->rgblkm [Y_BLOCK2 - 1];
				iQPpred = pmbmdLeft->m_stepSize;	
			}
			break;
		case Y_BLOCK4:
			if (pmbmdCurr->m_rgTranspStatus [Y_BLOCK1] != ALL)	{
				blkmRet = pmbmCurr->rgblkm [Y_BLOCK1 - 1];
				iQPpred = pmbmdCurr->m_stepSize;	
			}
			break;
		case A_BLOCK1:
			if (pmbmLeftTop != NULL && 
				(pmbmdLeftTop->m_dctMd == INTRA || pmbmdLeftTop->m_dctMd == INTRAQ) &&
				pmbmdLeftTop->m_rgTranspStatus [Y_BLOCK4] != ALL)	{
				blkmRet = pmbmLeftTop->rgblkm [A_BLOCK4 + iAuxComp*4 - 1];
				iQPpred = pmbmdLeftTop->m_stepSizeAlpha;	
			}
			break;
		case A_BLOCK2:
			if (pmbmTop != NULL && 
				(pmbmdTop->m_dctMd == INTRA || pmbmdTop->m_dctMd == INTRAQ) &&
				pmbmdTop->m_rgTranspStatus [Y_BLOCK3] != ALL)	{
				blkmRet = pmbmTop->rgblkm [A_BLOCK3 + iAuxComp*4 - 1];
				iQPpred = pmbmdTop->m_stepSizeAlpha;	
			}
			break;
		case A_BLOCK3:
			if (pmbmLeft != NULL && 
				(pmbmdLeft->m_dctMd == INTRA || pmbmdLeft->m_dctMd == INTRAQ) &&
				pmbmdLeft->m_rgTranspStatus [Y_BLOCK2] != ALL)	{
				blkmRet = pmbmLeft->rgblkm [A_BLOCK2 + iAuxComp*4 - 1];
				iQPpred = pmbmdLeft->m_stepSizeAlpha;	
			}
			break;
		case A_BLOCK4:
			if (pmbmdCurr->m_rgTranspStatus [Y_BLOCK1] != ALL)	{
				blkmRet = pmbmCurr->rgblkm [A_BLOCK1 + iAuxComp*4 - 1];
				iQPpred = pmbmdCurr->m_stepSizeAlpha;	
			}
			break;
		default:								//U, V block
			if (pmbmLeftTop != NULL && 
				(pmbmdLeftTop->m_dctMd == INTRA || pmbmdLeftTop->m_dctMd == INTRAQ) &&
				pmbmdLeftTop->m_rgTranspStatus [ALL_Y_BLOCKS] != ALL)	{
				blkmRet = pmbmLeftTop->rgblkm [iBlk - 1];
				iQPpred = pmbmdLeftTop->m_stepSize;	
			}
		}
	}
	else 
		assert (FALSE);
	/*BBM// Added for Boundary by Hyundai(1998-5-9)
	if (m_vopmd.bInterlace && pmbmdCurr->m_bMerged [0])
                swapTransparentModes ((CMBMode*)pmbmdCurr, BBS);
	// End of Hyundai(1998-5-9)*/
	return (blkmRet);
}

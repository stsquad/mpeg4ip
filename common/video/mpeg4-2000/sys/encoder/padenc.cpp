/*************************************************************************

This software module was originally developed by 

	Ming-Chieh Lee (mingcl@microsoft.com), Microsoft Corporation
	(date: June, 1997)
and edited by
        Wei Wu (weiwu@stallion.risc.rockwell.com) Rockwell Science Center

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
assign or donate the code to a third party and to inhibit third parties from using the code for non MPEG-4 Video conforming products. 
This copyright notice must be included in all copies or derivative works. 

Copyright (c) 1996, 1997.


Module Name:

	padEnc.cpp

Abstract:

	MB Padding for Intra-texture coding.

Revision History:

*************************************************************************/

// NOTE: 
//		LPE padding is working on the current MB whose size is always a MB
//		MC padding is  working on the (reference) quantized VOP

#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <iostream.h>

#include "typeapi.h"
#include "codehead.h"
#include "global.hpp"
#include "entropy/bitstrm.hpp"
#include "entropy/entropy.hpp"
#include "entropy/huffman.hpp"
#include "mode.hpp"
#include "vopses.hpp"
#include "vopseenc.hpp"

Void CVideoObjectEncoder::LPEPadding (const CMBMode* pmbmd)
{
	const PixelC* ppxlcBlkB;
	PixelC* ppxlcBlk;
	if (pmbmd -> m_rgTranspStatus [1] == PARTIAL) {
		ppxlcBlkB = m_ppxlcCurrMBBY;
		ppxlcBlk = m_ppxlcCurrMBY;
		LPEPaddingBlk (ppxlcBlk, ppxlcBlkB, MB_SIZE);
	}
	if (pmbmd -> m_rgTranspStatus [2] == PARTIAL) {
		ppxlcBlkB = m_ppxlcCurrMBBY + BLOCK_SIZE;
		ppxlcBlk = m_ppxlcCurrMBY + BLOCK_SIZE;
		LPEPaddingBlk (ppxlcBlk, ppxlcBlkB, MB_SIZE);
	}
	if (pmbmd -> m_rgTranspStatus [3] == PARTIAL) {
		ppxlcBlkB = m_ppxlcCurrMBBY + MB_SIZE * BLOCK_SIZE;
		ppxlcBlk = m_ppxlcCurrMBY + MB_SIZE * BLOCK_SIZE;
		LPEPaddingBlk (ppxlcBlk, ppxlcBlkB, MB_SIZE);
	}
	if (pmbmd -> m_rgTranspStatus [4] == PARTIAL) {
		ppxlcBlkB = m_ppxlcCurrMBBY + MB_SIZE * BLOCK_SIZE + BLOCK_SIZE;
		ppxlcBlk = m_ppxlcCurrMBY + MB_SIZE * BLOCK_SIZE + BLOCK_SIZE;
		LPEPaddingBlk (ppxlcBlk, ppxlcBlkB, MB_SIZE);
	}
	if (pmbmd -> m_rgTranspStatus [5] == PARTIAL) {
		LPEPaddingBlk (m_ppxlcCurrMBU, m_ppxlcCurrMBBUV, BLOCK_SIZE);
		LPEPaddingBlk (m_ppxlcCurrMBV, m_ppxlcCurrMBBUV, BLOCK_SIZE);
	}
	if (m_volmd.fAUsage == EIGHT_BIT) {
    for(Int iAuxComp=0;iAuxComp<m_volmd.iAuxCompCount;iAuxComp++) { // MAC (SB) 29-Nov-99
      if (pmbmd -> m_rgTranspStatus [1] == PARTIAL) {
        ppxlcBlkB = m_ppxlcCurrMBBY;
        ppxlcBlk = m_ppxlcCurrMBA[iAuxComp];
        LPEPaddingBlk (ppxlcBlk, ppxlcBlkB, MB_SIZE);
      }
      if (pmbmd -> m_rgTranspStatus [2] == PARTIAL) {
        ppxlcBlkB = m_ppxlcCurrMBBY + BLOCK_SIZE;
        ppxlcBlk = m_ppxlcCurrMBA[iAuxComp] + BLOCK_SIZE;
        LPEPaddingBlk (ppxlcBlk, ppxlcBlkB, MB_SIZE);
      }
      if (pmbmd -> m_rgTranspStatus [3] == PARTIAL) {
        ppxlcBlkB = m_ppxlcCurrMBBY + MB_SIZE * BLOCK_SIZE;
        ppxlcBlk = m_ppxlcCurrMBA[iAuxComp] + MB_SIZE * BLOCK_SIZE;
        LPEPaddingBlk (ppxlcBlk, ppxlcBlkB, MB_SIZE);
      }
      if (pmbmd -> m_rgTranspStatus [4] == PARTIAL) {
        ppxlcBlkB = m_ppxlcCurrMBBY + MB_SIZE * BLOCK_SIZE + BLOCK_SIZE;
        ppxlcBlk = m_ppxlcCurrMBA[iAuxComp] + MB_SIZE * BLOCK_SIZE + BLOCK_SIZE;
        LPEPaddingBlk (ppxlcBlk, ppxlcBlkB, MB_SIZE);
      }
    }
  }
}

#define OUTSIDE_BLOCK_VALUE		1
CU8Image uciBuff (CRct (0, 0, BLOCK_SIZE + 2, BLOCK_SIZE + 2), OUTSIDE_BLOCK_VALUE);
PixelC* ppxlcBuff = (PixelC*) uciBuff.pixels () + BLOCK_SIZE + 3;

#define BLOCK_SIZE_PLUS2		10

Void CVideoObjectEncoder::LPEPaddingBlk (
	PixelC* ppxlcBlk, const PixelC* ppxlcBlkB,
	UInt uiSize)
{
	Int iUnit = sizeof(PixelC); // NBIT: for memcpy
	UInt uiNumNonTranspPixels = 0;
	UInt ix, iy;
	UInt uiSum = 0;
	PixelC* ppxlcBuffTmp = ppxlcBuff;
	PixelC* ppxlcBlkTmp = ppxlcBlk;
	const PixelC* ppxlcBlkBTmp = ppxlcBlkB;
	for (iy = 0; iy < BLOCK_SIZE; iy++) {
		memcpy (ppxlcBuffTmp, ppxlcBlkBTmp, BLOCK_SIZE*iUnit);
		for (ix = 0; ix < BLOCK_SIZE; ix++) {
			if (ppxlcBlkBTmp [ix] != transpValue)	{
				uiSum += ppxlcBlkTmp [ix];
				uiNumNonTranspPixels++;
			}
		}
		ppxlcBlkBTmp += uiSize;
		ppxlcBlkTmp += uiSize;
		ppxlcBuffTmp += BLOCK_SIZE_PLUS2;
	}
	assert (uiNumNonTranspPixels != 0);
	uiSum /= uiNumNonTranspPixels;

	UInt uiSumNeighbor, uiNumNonTranpNeighbor;
	ppxlcBuffTmp = ppxlcBuff;
	for (iy = 0; iy < BLOCK_SIZE; iy++) {
		for (ix = 0; ix < BLOCK_SIZE; ix++) {
			if (ppxlcBlkB [ix] == transpValue) {
				uiSumNeighbor = uiNumNonTranpNeighbor = 0;
				if (ppxlcBuffTmp [ix - 1] != OUTSIDE_BLOCK_VALUE) {
					uiNumNonTranpNeighbor++;
					uiSumNeighbor += (ppxlcBuffTmp [ix - 1] == transpValue) ? uiSum : ppxlcBlk [ix - 1];
				}
				if (ppxlcBuffTmp [ix + 1] != OUTSIDE_BLOCK_VALUE) {
					uiNumNonTranpNeighbor++;
					uiSumNeighbor += (ppxlcBuffTmp [ix + 1] == transpValue) ? uiSum : ppxlcBlk [ix + 1];
				}
				if (ppxlcBuffTmp [ix - BLOCK_SIZE_PLUS2] != OUTSIDE_BLOCK_VALUE) {
					uiNumNonTranpNeighbor++;
					uiSumNeighbor += (ppxlcBuffTmp [ix - BLOCK_SIZE_PLUS2] == transpValue) ? uiSum : ppxlcBlk [ix - uiSize];
				}
				if (ppxlcBuffTmp [ix + BLOCK_SIZE_PLUS2] != OUTSIDE_BLOCK_VALUE) {
					uiNumNonTranpNeighbor++;
					uiSumNeighbor += (ppxlcBuffTmp [ix + BLOCK_SIZE_PLUS2] == transpValue) ? uiSum : ppxlcBlk [ix + uiSize];
				}
				ppxlcBlk [ix] = (PixelC) (uiSumNeighbor / uiNumNonTranpNeighbor);
			}
		}
		ppxlcBlkB += uiSize;
		ppxlcBlk += uiSize;
		ppxlcBuffTmp += BLOCK_SIZE_PLUS2;
	}
}


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
    Fujitsu Laboratories Ltd. (contact: Eishi Morimatsu)

in the course of development of the MPEG-4 Video (ISO/IEC 14496-2). 
This software module is an implementation of a part of one or more MPEG-4 Video tools 
as specified by the MPEG-4 Video. 
ISO/IEC gives users of the MPEG-4 Video free license to this software module or modifications 
thereof for use in hardware or software products claiming conformance to the MPEG-4 Video. 
Those intending to use this software module in hardware or software products are advised that its use may infringe existing patents. 
The original developer of this software module and his/her cany, 
the subsequent editors and their companies, 
and ISO/IEC have no liability for use of this software module or modifications thereof in an implementation. 
Copyright is not released for non MPEG-4 Video conforming products. 
Microsoft retains full right to use the code for his/her own purpose, 
assign or donate the code to a third party and to inhibit third parties from using the code for non <MPEG standard> conforming products. 
This copyright notice must be included in all copies or derivative works. 

Copyright (c) 1996, 1997.

Module Name:

	MB.hpp

Abstract:

	MacroBlock base class 

Revision History:
	        May. 9   1998:  add chroma subsampling and field based MC padding by Hyundai Electronics
                                  Cheol-Soo Park (cspark@super5.hyundai.co.kr)
	Sep.06	1999 : RRV added by Eishi Morimatsu (Fujitsu Laboratories Ltd.) 

*************************************************************************/

#include <stdio.h>

#include "typeapi.h"
#include "codehead.h"
#include "mode.hpp"
#include "dct.hpp"
#include "vopses.hpp"

#ifdef __MFC_
#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

#define new DEBUG_NEW				   
#endif // __MFC_

// must be called with following conditions:
// transp status must be set
// dct mode must be set
// skip must be set if not intra mb
// stepsize must be the new stepsize
// delta qp must be set
Void CVideoObject::setDCVLCMode(CMBMode* pmbmd, Bool *pbBoundary)
{
	if(pmbmd->m_rgTranspStatus[0]!=ALL)
	{
		if(pmbmd->m_dctMd == INTRA || pmbmd->m_dctMd == INTRAQ || !pmbmd->m_bSkip)
		{
			Int iQP;
			if(*pbBoundary)
			{
				*pbBoundary = FALSE;
				iQP = pmbmd->m_stepSize; // current qp
			}
			else
				iQP = pmbmd->m_stepSize - pmbmd->m_intStepDelta; // previous qp

			if (m_vopmd.iIntraDcSwitchThr != 0) {
				if (m_vopmd.iIntraDcSwitchThr == 7)
					pmbmd->m_bCodeDcAsAc = TRUE;
				else if (iQP >= (2 * m_vopmd.iIntraDcSwitchThr + 11))
					pmbmd->m_bCodeDcAsAc = TRUE;
				else
					pmbmd->m_bCodeDcAsAc = FALSE;
			}
			else
				pmbmd->m_bCodeDcAsAc = FALSE;

			pmbmd->m_bCodeDcAsAcAlpha = FALSE;
		}
	}
}


Void CVideoObject::decideTransparencyStatus (CMBMode* pmbmd, const PixelC* ppxlcMBBY)
{
	const PixelC* ppxlcBlkBY1 = ppxlcMBBY;
	const PixelC* ppxlcBlkBY2 = ppxlcMBBY + BLOCK_SIZE;
	const PixelC* ppxlcBlkBY3 = ppxlcMBBY + MB_SIZE * BLOCK_SIZE;
	const PixelC* ppxlcBlkBY4 = ppxlcMBBY + MB_SIZE * BLOCK_SIZE + BLOCK_SIZE;
//	const PixelC* ppxlcBlkBUV = m_ppxlcCurrMBBUV;
	UInt uiNonTransPixels1 = 0, uiNonTransPixels2 = 0;
	UInt uiNonTransPixels3 = 0, uiNonTransPixels4 = 0;
//	UInt uiNonTransPixelsUV = 0;
	CoordI ix, iy;
	for (iy = 0; iy < BLOCK_SIZE; iy++) {
		for (ix = 0; ix < BLOCK_SIZE; ix++) {
			uiNonTransPixels1 += ppxlcBlkBY1 [ix];
			uiNonTransPixels2 += ppxlcBlkBY2 [ix];
			uiNonTransPixels3 += ppxlcBlkBY3 [ix];
			uiNonTransPixels4 += ppxlcBlkBY4 [ix];
//			uiNonTransPixelsUV += ppxlcBlkBUV [ix];
		}
		ppxlcBlkBY1 += MB_SIZE;
		ppxlcBlkBY2 += MB_SIZE;
		ppxlcBlkBY3 += MB_SIZE;
		ppxlcBlkBY4 += MB_SIZE;
//		ppxlcBlkBUV += BLOCK_SIZE;
	}
	uiNonTransPixels1 /= opaqueValue;
	uiNonTransPixels2 /= opaqueValue;
	uiNonTransPixels3 /= opaqueValue;
	uiNonTransPixels4 /= opaqueValue;
//	uiNonTransPixelsUV /= opaqueValue;

	pmbmd->m_rgNumNonTranspPixels [0] = 
		uiNonTransPixels1 + uiNonTransPixels2 + uiNonTransPixels3 + uiNonTransPixels4;
	pmbmd->m_rgNumNonTranspPixels [1] = uiNonTransPixels1;
	pmbmd->m_rgNumNonTranspPixels [2] = uiNonTransPixels2;
	pmbmd->m_rgNumNonTranspPixels [3] = uiNonTransPixels3;
	pmbmd->m_rgNumNonTranspPixels [4] = uiNonTransPixels4;
//	pmbmd->m_rgNumNonTranspPixels [5] = uiNonTransPixelsUV;
	if (pmbmd->m_rgNumNonTranspPixels [0] == 0) {
		pmbmd->m_rgTranspStatus [0] = ALL;
		pmbmd->m_dctMd = INTER;
	}
	else if (pmbmd->m_rgNumNonTranspPixels [0] == MB_SQUARE_SIZE)
		pmbmd -> m_rgTranspStatus [0] = NONE;
	else
		pmbmd -> m_rgTranspStatus [0] = PARTIAL;

//	for (ix = (UInt) Y_BLOCK1; ix <= (UInt) U_BLOCK; ix++) {
	for (ix = (UInt) Y_BLOCK1; ix < U_BLOCK; ix++) {
		if (pmbmd -> m_rgNumNonTranspPixels [ix] == 0)
			pmbmd -> m_rgTranspStatus [ix] = ALL;
		else if (pmbmd -> m_rgNumNonTranspPixels [ix] == BLOCK_SQUARE_SIZE)
			pmbmd -> m_rgTranspStatus [ix] = NONE;
		else
			pmbmd -> m_rgTranspStatus [ix] = PARTIAL;
	}
//pmbmd->m_rgTranspStatus [V_BLOCK] = pmbmd->m_rgTranspStatus [U_BLOCK];
pmbmd->m_rgTranspStatus [V_BLOCK] = pmbmd->m_rgTranspStatus [U_BLOCK] 
= pmbmd -> m_rgTranspStatus [0];
}

Void CVideoObject::decideMBTransparencyStatus (CMBMode* pmbmd)
{
	UInt uiNonTransPixels0 = 0;
	UInt ix;
	for (ix = 0; ix < MB_SQUARE_SIZE; ix++)
		uiNonTransPixels0 += m_ppxlcCurrMBBY [ix];
	uiNonTransPixels0 /= opaqueValue;

	pmbmd -> m_rgNumNonTranspPixels [0] = uiNonTransPixels0;
	if (uiNonTransPixels0 == 0) {
		pmbmd -> m_rgTranspStatus [0] = ALL;
		pmbmd -> m_dctMd = INTER;
	}
	else if (uiNonTransPixels0 == MB_SQUARE_SIZE)
		pmbmd -> m_rgTranspStatus [0] = NONE;
	else
		pmbmd -> m_rgTranspStatus [0] = PARTIAL;
}

// Added for Field Based MC Padding by Hyundai(1998-5-9)
Void CVideoObject::decideFieldTransparencyStatus (
        CMBMode* pmbmd,
        const PixelC* ppxlcMBBY,
        const PixelC* ppxlcMBBUV
)
{
        Int *piNonTransPixels = new Int [5];
        Int iYFieldSkip = 2*m_iFrameWidthY;
        Int iCFieldSkip = 2*m_iFrameWidthUV;
        CoordI ix, iy;
 
        // for field based mc-padding
        memset (piNonTransPixels, 0, sizeof(Int)*5);

        const PixelC* ppxlcTopFieldBY = ppxlcMBBY;
        const PixelC* ppxlcBotFieldBY = ppxlcMBBY + m_iFrameWidthY;
        for (iy = 0; iy < BLOCK_SIZE; iy++) {
                for (ix = 0; ix < MB_SIZE; ix++) {
                        piNonTransPixels[1] += ppxlcTopFieldBY [ix];
                        piNonTransPixels[2] += ppxlcBotFieldBY [ix];
                }
                ppxlcTopFieldBY += iYFieldSkip;
                ppxlcBotFieldBY += iYFieldSkip;
        }
        piNonTransPixels[1] /= opaqueValue;
        piNonTransPixels[2] /= opaqueValue;
        for (ix=1; ix<=2; ix++) {
                if (piNonTransPixels [ix] == 0)
                        pmbmd-> m_rgFieldTranspStatus [ix] = ALL;
                else if (piNonTransPixels [ix] == MB_SQUARE_SIZE/2)
                        pmbmd -> m_rgFieldTranspStatus [ix] = NONE;
                else    pmbmd -> m_rgFieldTranspStatus [ix] = PARTIAL;
        }
        ppxlcTopFieldBY = ppxlcMBBUV;
        ppxlcBotFieldBY = ppxlcMBBUV + m_iFrameWidthUV;
        for (iy = 0; iy < BLOCK_SIZE/2; iy++) {
                for (ix = 0; ix < BLOCK_SIZE; ix++) {
                        piNonTransPixels[3] += ppxlcTopFieldBY [ix];
                        piNonTransPixels[4] += ppxlcBotFieldBY [ix];
                }
                ppxlcTopFieldBY += iCFieldSkip;
                ppxlcBotFieldBY += iCFieldSkip;
        }
        piNonTransPixels[3] /= opaqueValue;
        piNonTransPixels[4] /= opaqueValue;
        for (ix=3; ix<=4; ix++) {
                if (piNonTransPixels [ix] == 0)
                        pmbmd-> m_rgFieldTranspStatus [ix] = ALL;
                else if (piNonTransPixels [ix] == BLOCK_SQUARE_SIZE/2)
                        pmbmd -> m_rgFieldTranspStatus [ix] = NONE;
                else    pmbmd -> m_rgFieldTranspStatus [ix] = PARTIAL;
        }
        
        delete piNonTransPixels;
}

Void CVideoObject::fieldBasedDownSampleBY (
        const PixelC* ppxlcMBBY,
        PixelC* ppxlcMBBUV
)
{
        UInt  ix, iy, x, iFieldSkip = 4*m_iFrameWidthY;
        const PixelC* ppxlcTopFieldBY = ppxlcMBBY;
        const PixelC* ppxlcBotFieldBY = ppxlcMBBY + m_iFrameWidthY;
        const PixelC* ppxlcTopFieldBYNext = ppxlcTopFieldBY + 2*m_iFrameWidthY;
        const PixelC* ppxlcBotFieldBYNext = ppxlcBotFieldBY + 2*m_iFrameWidthY;
 
        for (iy = 0; iy < BLOCK_SIZE/2; iy++) {
                for (ix = 0, x = 0; ix < BLOCK_SIZE; ix++, x = 2*ix)
                        ppxlcMBBUV [ix] = (ppxlcTopFieldBY [x] | ppxlcTopFieldBY [x+1] |
                                                ppxlcTopFieldBYNext [x] | ppxlcTopFieldBYNext [x+1]);
                ppxlcMBBUV += m_iFrameWidthUV;
                for (ix = 0, x = 0; ix < BLOCK_SIZE; ix++, x = 2*ix)
                        ppxlcMBBUV [ix] = (ppxlcBotFieldBY [x] | ppxlcBotFieldBY [x+1] |
                                                ppxlcBotFieldBYNext [x] | ppxlcBotFieldBYNext [x+1]);
                ppxlcMBBUV += m_iFrameWidthUV;
                ppxlcTopFieldBY += iFieldSkip;
                ppxlcBotFieldBY += iFieldSkip;
                ppxlcTopFieldBYNext += iFieldSkip;
                ppxlcBotFieldBYNext += iFieldSkip;
        }
}

// End of Hyundai(1998-5-9)

//Changed HHI 2000-04-11
Void CVideoObject::downSampleBY (const PixelC* ppxlcMBBY, PixelC* ppxlcMBBUV, CMBMode* pmbmd)
{
	const PixelC* ppxlcMBBYNextRow = ppxlcMBBY + MB_SIZE;
	UInt ix, iy;
	PixelC* ppxlcMBBUV_t = ppxlcMBBUV; // Changed HHI 2000-04-11
	// Added for Chroma Subsampling by Hyundai(1998-5-9)
        if (m_vopmd.bInterlace) {
                UInt  x, iFieldSkip = 4*MB_SIZE;
                const PixelC* ppxlcTopFieldBY = ppxlcMBBY;
                const PixelC* ppxlcBotFieldBY = ppxlcMBBY + MB_SIZE;
                const PixelC* ppxlcTopFieldBYNext = ppxlcTopFieldBY + 2*MB_SIZE;
                const PixelC* ppxlcBotFieldBYNext = ppxlcBotFieldBY + 2*MB_SIZE;
                for (iy = 0; iy < BLOCK_SIZE/2; iy++) {
                        for (ix = 0, x = 0; ix < BLOCK_SIZE; ix++, x = 2*ix)
                                ppxlcMBBUV [ix] = (ppxlcTopFieldBY [x] | ppxlcTopFieldBY [x+1] |
                                                        ppxlcTopFieldBYNext [x] | ppxlcTopFieldBYNext [x+1]);
                        ppxlcMBBUV += BLOCK_SIZE;
                        for (ix = 0, x = 0; ix < BLOCK_SIZE; ix++, x = 2*ix)
                                ppxlcMBBUV [ix] = (ppxlcBotFieldBY [x] | ppxlcBotFieldBY [x+1] |
                                                        ppxlcBotFieldBYNext [x] | ppxlcBotFieldBYNext [x+1]);
                        ppxlcMBBUV += BLOCK_SIZE;
                        ppxlcTopFieldBY += iFieldSkip;
                        ppxlcBotFieldBY += iFieldSkip;
                        ppxlcTopFieldBYNext += iFieldSkip;
                        ppxlcBotFieldBYNext += iFieldSkip;
                }
                // return;
        } else
		{
	// End of Hyundai(1998-5-9)
			for (iy = 0; iy < BLOCK_SIZE; iy++) {
				for (ix = 0; ix < BLOCK_SIZE; ix++) {
					ppxlcMBBUV [ix] = (
						(ppxlcMBBY [2 * ix] | ppxlcMBBY [2 * ix + 1]) |
						(ppxlcMBBYNextRow [2 * ix] | ppxlcMBBYNextRow [2 * ix + 1])
						);
				}
				ppxlcMBBUV += BLOCK_SIZE;
				ppxlcMBBY += 2 * MB_SIZE;
				ppxlcMBBYNextRow += 2 * MB_SIZE;
			}
		}
	// Begin Changed HHI 2000-04-11
	if (pmbmd!=NULL) {
		UInt uiNonTransPixelsUV = 0;
		for (iy = 0; iy < BLOCK_SIZE; iy++) {
			for (ix = 0; ix < BLOCK_SIZE; ix++) {
				uiNonTransPixelsUV += ppxlcMBBUV_t[ix];
			}
			ppxlcMBBUV_t+=BLOCK_SIZE;
		}
		uiNonTransPixelsUV /= opaqueValue;
		pmbmd->m_rgNumNonTranspPixels [5] = uiNonTransPixelsUV;
		if (pmbmd -> m_rgNumNonTranspPixels [5] == 0)
			pmbmd -> m_rgTranspStatus [U_BLOCK] = ALL;
		else if (pmbmd -> m_rgNumNonTranspPixels [5] == BLOCK_SQUARE_SIZE)
			pmbmd -> m_rgTranspStatus [U_BLOCK] = NONE;
		else
			pmbmd -> m_rgTranspStatus [U_BLOCK] = PARTIAL;
		pmbmd->m_rgTranspStatus [V_BLOCK] = pmbmd->m_rgTranspStatus [U_BLOCK];
	}
	// End Changed HHI 2000-04-11
}

Void CVideoObject::addErrorAndPredToCurrQ (
	PixelC* ppxlcQMBY, 
	PixelC* ppxlcQMBU, 
	PixelC* ppxlcQMBV
)
{
	CoordI ix, iy, ic = 0;
// RRV modification
	for (iy = 0; iy < (MB_SIZE *m_iRRVScale); iy++) {
		for (ix = 0; ix < (MB_SIZE *m_iRRVScale); ix++, ic++) {
// ~RRV
			ppxlcQMBY [ix] = m_rgiClipTab [m_ppxlcPredMBY [ic] + m_ppxliErrorMBY [ic]];

		}
		ppxlcQMBY += m_iFrameWidthY;
	}

	ic = 0;
// RRV modification
	for (iy = 0; iy < (BLOCK_SIZE *m_iRRVScale); iy++) {
		for (ix = 0; ix < (BLOCK_SIZE *m_iRRVScale); ix++, ic++) {
// ~RRV
			ppxlcQMBU [ix] = m_rgiClipTab [m_ppxlcPredMBU [ic] + m_ppxliErrorMBU [ic]];
			ppxlcQMBV [ix] = m_rgiClipTab [m_ppxlcPredMBV [ic] + m_ppxliErrorMBV [ic]];
		}
		ppxlcQMBU += m_iFrameWidthUV;
		ppxlcQMBV += m_iFrameWidthUV;
	}
}

Void CVideoObject::addAlphaErrorAndPredToCurrQ (PixelC* ppxlcQMBA, Int iAuxComp)
{
	CoordI ix, iy, ic = 0;
  
  for (iy = 0; iy < MB_SIZE; iy++) {
	 	for (ix = 0; ix < MB_SIZE; ix++, ic++)
      ppxlcQMBA[ix] = m_rgiClipTab [m_ppxlcPredMBA[iAuxComp][ic] + m_ppxliErrorMBA[iAuxComp] [ic]];
    ppxlcQMBA += m_iFrameWidthY;     
	}
}

Void CVideoObject::assignPredToCurrQ (
	PixelC* ppxlcQMBY, 
	PixelC* ppxlcQMBU, 
	PixelC* ppxlcQMBV
)
{
	CoordI ix;
	const PixelC* ppxlcPredMBY = m_ppxlcPredMBY;
	const PixelC* ppxlcPredMBU = m_ppxlcPredMBU;
	const PixelC* ppxlcPredMBV = m_ppxlcPredMBV;
// RRV modification
	for (ix = 0; ix < (BLOCK_SIZE *m_iRRVScale); ix++) {
		memcpy(ppxlcQMBY, ppxlcPredMBY,
			   (MB_SIZE *m_iRRVScale) *sizeof(PixelC));
		memcpy(ppxlcQMBU, ppxlcPredMBU,
			   (BLOCK_SIZE *m_iRRVScale) *sizeof(PixelC));
		memcpy(ppxlcQMBV, ppxlcPredMBV, 
			   (BLOCK_SIZE *m_iRRVScale) *sizeof(PixelC));
		ppxlcPredMBY += (MB_SIZE *m_iRRVScale);
		ppxlcPredMBU += (BLOCK_SIZE *m_iRRVScale);
		ppxlcPredMBV += (BLOCK_SIZE *m_iRRVScale);
// ~RRV
		ppxlcQMBY += m_iFrameWidthY;
		ppxlcQMBU += m_iFrameWidthUV;
		ppxlcQMBV += m_iFrameWidthUV;

// RRV modification
		memcpy(ppxlcQMBY, ppxlcPredMBY, 
			   (MB_SIZE *m_iRRVScale) *sizeof(PixelC));
		ppxlcPredMBY += (MB_SIZE *m_iRRVScale);
// ~RRV
		ppxlcQMBY += m_iFrameWidthY;
	}
}

Void CVideoObject::assignAlphaPredToCurrQ (PixelC* ppxlcQMBA, Int iAuxComp)
{
	CoordI ix;
	const PixelC* ppxlcPredMBA = m_ppxlcPredMBA[iAuxComp];
	for (ix = 0; ix < MB_SIZE; ix++) {
		memcpy (ppxlcQMBA, ppxlcPredMBA, MB_SIZE*sizeof(PixelC));
		ppxlcPredMBA += MB_SIZE;
		ppxlcQMBA += m_iFrameWidthY;
	}
}

Void CVideoObject::findColocatedMB (Int iMBX, Int iMBY,
									const CMBMode*& pmbmdRef,
									const CMotionVector*& pmvRef)
{
	Bool bColocatedMBExist = FALSE;

	if(m_bCodedFutureRef!=FALSE)
		if(iMBX<m_iNumMBXRef && iMBX>=0 && iMBY<m_iNumMBYRef && iMBY>=0)
		{
			pmbmdRef = &m_rgmbmdRef [iMBX + iMBY * m_iNumMBXRef];
			if(pmbmdRef -> m_rgTranspStatus [0] != ALL)
			{
				Int iOffset = min (max (0, iMBX), m_iNumMBXRef - 1) + 
 					min (max (0, iMBY), m_iNumMBYRef - 1) * m_iNumMBXRef;
				pmvRef = m_rgmvRef + iOffset * PVOP_MV_PER_REF_PER_MB;
				bColocatedMBExist = TRUE;
			}
		}

	if(bColocatedMBExist==FALSE)
	{
		pmbmdRef = NULL;
		pmvRef = NULL;
	}
}

/*************************************************************************

This software module was originally developed by 

	Ming-Chieh Lee (mingcl@microsoft.com), Microsoft Corporation
	Bruce Lin (blin@microsoft.com), Microsoft Corporation
	Simon Winder (swinder@microsoft.com), Microsoft Corporation
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
assign or donate the code to a third party and to inhibit third parties from using the code for non <MPEG standard> conforming products. 
This copyright notice must be included in all copies or derivative works. 

Copyright (c) 1996, 1997.

Module Name:

	mcenc.cpp

Abstract:

	Motion compensation routines for encoder.

Revision History:
	Dec 20, 1997:	Interlaced tools added by NextLevel Systems

*************************************************************************/

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

#ifdef __MFC_
#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

#define new DEBUG_NEW				   
#endif // __MFC_

#define OFFSET_BLK1 8
#define OFFSET_BLK2 128
#define OFFSET_BLK3 136

Void CVideoObjectEncoder::motionCompMBYEnc (
	const CMotionVector* pmv, const CMBMode* pmbmd, 
	Int imbX, Int imbY,
	CoordI x, CoordI y,
	CRct *prctMVLimit
)
{
	if (!m_volmd.bAdvPredDisable /* && pmbmd -> m_bhas4MVForward */ && !pmbmd->m_bFieldMV) {
		motionCompOverLapEncY (
			pmv, pmbmd,
			(imbX == 0), (imbX == m_iNumMBX - 1), (imbY == 0),
			x, y,
			prctMVLimit
		);
	}
	else {
		if (!pmbmd -> m_bhas4MVForward && !pmbmd -> m_bFieldMV)
			motionCompEncY (
				m_pvopcRefQ0->pixelsY (), 
				m_puciRefQZoom0->pixels (),
				m_ppxlcPredMBY, MB_SIZE, pmv, x, y,
				prctMVLimit
			);
// INTERLACE
		else if (pmbmd -> m_bFieldMV) {
			const CMotionVector* pmvTop = pmv + 5 + pmbmd->m_bForwardTop;
			motionCompYField(m_ppxlcPredMBY,
				m_pvopcRefQ0->pixelsY () + pmbmd->m_bForwardTop * m_iFrameWidthY,
				2*x + pmvTop->m_vctTrueHalfPel.x, 2*y + pmvTop->m_vctTrueHalfPel.y);

			const CMotionVector* pmvBottom = pmv + 7 + pmbmd->m_bForwardBottom;
			motionCompYField(m_ppxlcPredMBY + MB_SIZE,
				m_pvopcRefQ0->pixelsY () + pmbmd->m_bForwardBottom * m_iFrameWidthY,
				2*x + pmvBottom->m_vctTrueHalfPel.x, 2*y + pmvBottom->m_vctTrueHalfPel.y);
		}
// ~INTERLACE
		else {
			const CMotionVector* pmv8 = pmv;
			pmv8++;
			CoordI blkX = x + BLOCK_SIZE;
			CoordI blkY = y + BLOCK_SIZE;
			motionCompEncY (
				m_pvopcRefQ0->pixelsY (), 
				m_puciRefQZoom0->pixels (),
				m_ppxlcPredMBY, 
				BLOCK_SIZE, pmv8, x, y,
				prctMVLimit
			);
			pmv8++;
			motionCompEncY (
				m_pvopcRefQ0->pixelsY (), 
				m_puciRefQZoom0->pixels (),
				m_ppxlcPredMBY + OFFSET_BLK1, 
				BLOCK_SIZE, pmv8, blkX, y,
				prctMVLimit
			);
			pmv8++;
			motionCompEncY (
				m_pvopcRefQ0->pixelsY (), 
				m_puciRefQZoom0->pixels (),
				m_ppxlcPredMBY + OFFSET_BLK2, 
				BLOCK_SIZE, pmv8, x, blkY,
				prctMVLimit
			);
			pmv8++;
			motionCompEncY (
				m_pvopcRefQ0->pixelsY (), 
				m_puciRefQZoom0->pixels (),
				m_ppxlcPredMBY + OFFSET_BLK3, 
				BLOCK_SIZE, pmv8, blkX, blkY,
				prctMVLimit
			);
		}
	}
	if(m_volmd.fAUsage == EIGHT_BIT)
		motionCompMBAEnc (
			pmv, pmbmd,
			m_ppxlcPredMBA,
			m_pvopcRefQ0,
			x, y,
			m_vopmd.iRoundingControl,
			prctMVLimit,
			0
		);
}

Void CVideoObjectEncoder::motionCompMBAEnc (
	const CMotionVector* pmv, const CMBMode* pmbmd, 
	PixelC *ppxlcPredMBA,
	CVOPU8YUVBA* pvopcRefQ,
	CoordI x, CoordI y,
	Int iRoundingControl,
	CRct *prctMVLimit,
	Int direction //12.22.98
)
{
	if (!pmbmd -> m_bhas4MVForward && !pmbmd -> m_bFieldMV) //12.22.98
		motionComp (
			ppxlcPredMBA,
			pvopcRefQ->pixelsA (), 
			MB_SIZE,
			x * 2 + pmv->trueMVHalfPel ().x, 
			y * 2 + pmv->trueMVHalfPel ().y ,
			iRoundingControl,
			prctMVLimit
		);
// INTERLACE  12.22.98
	else if (pmbmd -> m_bFieldMV) {
		Int itmpref1,itmpref2;

		itmpref1=pmbmd->m_bForwardTop;
		itmpref2=pmbmd->m_bForwardBottom;

		if(m_vopmd.vopPredType==BVOP&&direction)
		{
			itmpref1=pmbmd->m_bBackwardTop;
			itmpref2=pmbmd->m_bBackwardBottom;
		}

		const CMotionVector* pmvTop,*pmvBottom;
		if(m_vopmd.vopPredType==BVOP)
			pmvTop = pmv + 1 + itmpref1;
		else
			pmvTop = pmv + 5 + itmpref1;
		motionCompYField(ppxlcPredMBA,
			pvopcRefQ->pixelsA () + itmpref1 * m_iFrameWidthY,
			2*x + pmvTop->trueMVHalfPel ().x, 2*y + pmvTop->trueMVHalfPel ().y);
		if(m_vopmd.vopPredType==BVOP)
			pmvBottom = pmv + 3 + itmpref2;
		else
			pmvBottom = pmv + 7 + itmpref2;
		motionCompYField(ppxlcPredMBA + MB_SIZE,
			pvopcRefQ->pixelsA () + itmpref2 * m_iFrameWidthY,
			2*x + pmvBottom->trueMVHalfPel ().x, 2*y + pmvBottom->trueMVHalfPel ().y);
		}
// ~INTERLACE 12.22.98
	else {
		const CMotionVector* pmv8 = pmv;
		pmv8++;
		CoordI blkX = x + BLOCK_SIZE;
		CoordI blkY = y + BLOCK_SIZE;
		motionComp (
			ppxlcPredMBA,
			pvopcRefQ->pixelsA (),
			BLOCK_SIZE,
			x * 2 + pmv8->trueMVHalfPel ().x, 
			y * 2 + pmv8->trueMVHalfPel ().y,
			iRoundingControl,
			prctMVLimit
		);
		pmv8++;
		motionComp (
			ppxlcPredMBA + OFFSET_BLK1,
			pvopcRefQ->pixelsA (), 				 
			BLOCK_SIZE,
			blkX * 2 + pmv8->trueMVHalfPel ().x, 
			y * 2 + pmv8->trueMVHalfPel ().y,
			iRoundingControl,
			prctMVLimit
		);
		pmv8++;
		motionComp (
			ppxlcPredMBA + OFFSET_BLK2, 
			pvopcRefQ->pixelsA (), 
			BLOCK_SIZE,
			x * 2 + pmv8->trueMVHalfPel ().x, 
			blkY * 2 + pmv8->trueMVHalfPel ().y,
			iRoundingControl,
			prctMVLimit
		);
		pmv8++;
		motionComp (
			ppxlcPredMBA + OFFSET_BLK3,
			pvopcRefQ->pixelsA (),
			BLOCK_SIZE,
			blkX * 2 + pmv8->trueMVHalfPel ().x, 
			blkY * 2 + pmv8->trueMVHalfPel ().y,
			iRoundingControl,
			prctMVLimit
		);
	}
}

Void CVideoObjectEncoder::motionCompEncY (
	const PixelC* ppxlcRef, const PixelC* ppxlcRefZoom,
	PixelC* ppxlcPred,
	Int iSize, // either MB or BLOCK size
	const CMotionVector* pmv, // motion vector
	CoordI x, CoordI y, // current coordinate system
	CRct *prctMVLimit
)
{
	Int iUnit = sizeof(PixelC); // NBIT: for memcpy
	CoordI ix, iy;
 	Bool bXSubPxl, bYSubPxl;
    CoordI xHalf = 2*x + pmv->m_vctTrueHalfPel.x;
    CoordI yHalf = 2*y + pmv->m_vctTrueHalfPel.y;
//	CoordI xHalf = 2*(x + pmv->iMVX) + pmv->iHalfX;
//	CoordI yHalf = 2*(y + pmv->iMVY) + pmv->iHalfY;
	limitMVRangeToExtendedBBHalfPel(xHalf,yHalf,prctMVLimit,iSize);

	bXSubPxl = (xHalf&1);
	bYSubPxl = (yHalf&1);
	if (!bYSubPxl && !bXSubPxl) {
		const PixelC* ppxlcRefMB = ppxlcRef + m_rctRefFrameY.offset (xHalf>>1, yHalf>>1);
		for (iy = 0; iy < iSize; iy++) {
			memcpy (ppxlcPred, ppxlcRefMB, iSize*iUnit);
			ppxlcRefMB += m_iFrameWidthY;
			ppxlcPred += MB_SIZE;
		}
	}
	else {
		const PixelC* ppxlcPrevZoomY = ppxlcRefZoom
			+ m_puciRefQZoom0->where ().offset (xHalf, yHalf);
		for (iy = 0; iy < iSize; iy++) {
			for (ix = 0; ix < iSize; ix++)
				ppxlcPred [ix] = ppxlcPrevZoomY [2 * ix]; 
			ppxlcPrevZoomY += m_iFrameWidthZoomY * 2;
			ppxlcPred += MB_SIZE;
		}
	}
}

Void CVideoObjectEncoder::motionCompOverLapEncY (
	const CMotionVector* pmv, // motion vector
	const CMBMode* pmbmd, // macroblk mode	
	Bool bLeftBndry, Bool bRightBndry, Bool bTopBndry,
	CoordI x, // current coordinate system
	CoordI y, // current coordinate system
	CRct *prctMVLimit
)
{
	// Overlap Motion Comp use motion vector of current blk and motion vectors of neighboring blks.
	const CMotionVector *pmvC, *pmvT = NULL, *pmvB = NULL, *pmvR = NULL, *pmvL = NULL; // MVs of Cur, Top, Bot, Right and Left Blocks. 
	const CMotionVector *pmvCurrMb, *pmvTopMb, *pmvRightMb, *pmvLeftMb; // MVs of Cur, Top, Right and Left MacroBlocks.
	const CMBMode *pmbmdTopMb, *pmbmdRightMb, *pmbmdLeftMb; // MVs of Cur, Top, Right and Left MacroBlocks.
	Bool bIntraT, bIntraR, bIntraL; // flags of 4MV for Cur, Top, Right and Left MacroBlocks.
	pmvTopMb = NULL; // compile warning
	pmvRightMb = NULL;
	pmvLeftMb = NULL;
	pmbmdTopMb = NULL;
	pmbmdRightMb = NULL;
	pmbmdLeftMb = NULL;
	bIntraT = FALSE;
	bIntraR = FALSE;
	bIntraL = FALSE;
	pmvCurrMb = pmv;
	// assign values to bIntra[TRL] and pmv{TRL}Mb, when they are valid. 
	if (!bTopBndry) {
		pmbmdTopMb = pmbmd - m_iNumMBX; 
		bIntraT = (pmbmdTopMb->m_dctMd == INTRA || pmbmdTopMb->m_dctMd == INTRAQ);
		pmvTopMb = pmv - m_iNumOfTotalMVPerRow;
	}
	if (!bLeftBndry) {
		pmbmdLeftMb = pmbmd - 1;
		bIntraL = (pmbmdLeftMb->m_dctMd == INTRA || pmbmdLeftMb->m_dctMd == INTRAQ);
		pmvLeftMb = pmv - PVOP_MV_PER_REF_PER_MB;
	}
	if (!bRightBndry) {
		pmbmdRightMb = pmbmd + 1;
		bIntraR = (pmbmdRightMb->m_dctMd == INTRA || pmbmdRightMb->m_dctMd == INTRAQ);
		pmvRightMb = pmv + PVOP_MV_PER_REF_PER_MB;
	}
	UInt i;
	// assign the neighboring blk's MVs to pmv[TBRLC] 
	for (i = 1; i < 5; i++) {
		if (pmbmd->m_rgTranspStatus [i] == ALL)
			continue;
		pmvC = pmvCurrMb + i;
		switch (i) {
			case 1:
				if (pmbmd->m_rgTranspStatus [3] == ALL)
					pmvB = pmvCurrMb + 1;
				else
					pmvB = pmvCurrMb + 3;
				
				if (pmbmd->m_rgTranspStatus [2] == ALL)
					pmvR = pmvCurrMb + 1; 	
				else
					pmvR = pmvCurrMb + 2; 	

				if (bTopBndry || bIntraT || pmbmdTopMb->m_rgTranspStatus [3] == ALL)
					pmvT = pmvCurrMb + 1;
				else
					pmvT = pmvTopMb + 3;

				if (bLeftBndry || bIntraL || pmbmdLeftMb->m_rgTranspStatus [2] == ALL)
					pmvL = pmvCurrMb + 1;
				else
					pmvL = pmvLeftMb + 2;
				break;
			case 2:
				if (pmbmd->m_rgTranspStatus [4] == ALL)
					pmvB = pmvCurrMb + 2;
				else
					pmvB = pmvCurrMb + 4;

				if (pmbmd->m_rgTranspStatus [1] == ALL)
					pmvL = pmvCurrMb + 2;
				else
					pmvL = pmvCurrMb + 1;

				if (bTopBndry || bIntraT || pmbmdTopMb->m_rgTranspStatus [4] == ALL)
					pmvT = pmvCurrMb + 2;
				else
					pmvT = pmvTopMb + 4;      	

				if (bRightBndry || bIntraR || pmbmdRightMb->m_rgTranspStatus [1] == ALL)
					pmvR = pmvCurrMb + 2;
				else
					pmvR = pmvRightMb + 1;
				break;
			case 3:
				if (pmbmd->m_rgTranspStatus [1] == ALL)
					pmvT = pmvCurrMb + 3;  
				else
					pmvT = pmvCurrMb + 1;
				
				pmvB = pmvCurrMb + 3; // use the current mv

				if (pmbmd->m_rgTranspStatus [4] == ALL)
					pmvR = pmvCurrMb + 3;
				else
					pmvR = pmvCurrMb + 4;

				if (bLeftBndry || bIntraL || pmbmdLeftMb->m_rgTranspStatus [4] == ALL)
					pmvL = pmvCurrMb + 3;
				else
					pmvL = pmvLeftMb + 4;
				break;
			case 4:
				if (pmbmd->m_rgTranspStatus [2] == ALL)
					pmvT = pmvCurrMb + 4;    	
				else
					pmvT = pmvCurrMb + 2;
				
				pmvB = pmvCurrMb + 4;

				if (pmbmd->m_rgTranspStatus [3] == ALL)
					pmvL = pmvCurrMb + 4;  	
				else
					pmvL = pmvCurrMb + 3;
				
				if (bRightBndry || bIntraR || pmbmdRightMb->m_rgTranspStatus [3] == ALL)
					pmvR = pmvCurrMb + 4;
				else
					pmvR = pmvRightMb + 3;
				break;
			default:
				assert (FALSE);
			}
		// Compute the top left corner's x,y coordinates of current block.
		UInt dxc = (((i - 1) & 1) << 3); 
		UInt dyc = (((i - 1) & 2) << 2);
		UInt nxcY = (x + dxc) << 1; 
		UInt nycY = (y + dyc) << 1; 
		// Compute the corresponding positions on Ref frm, using 5 MVs.
		CoordI xRefC = nxcY + pmvC->m_vctTrueHalfPel.x, yRefC = nycY + pmvC->m_vctTrueHalfPel.y;
		CoordI xRefT = nxcY + pmvT->m_vctTrueHalfPel.x, yRefT = nycY + pmvT->m_vctTrueHalfPel.y;
		CoordI xRefB = nxcY + pmvB->m_vctTrueHalfPel.x, yRefB = nycY + pmvB->m_vctTrueHalfPel.y;
		CoordI xRefR = nxcY + pmvR->m_vctTrueHalfPel.x, yRefR = nycY + pmvR->m_vctTrueHalfPel.y;
		CoordI xRefL = nxcY + pmvL->m_vctTrueHalfPel.x, yRefL = nycY + pmvL->m_vctTrueHalfPel.y;
//		UInt nxcY = x + dxc; 
//  	UInt nycY = y + dyc; 
//		// Compute the corresponding positions on Ref frm, using 5 MVs.
//		CoordI xRefC = ((nxcY + pmvC->iMVX) << 1) + pmvC->iHalfX, yRefC = ((nycY + pmvC->iMVY) << 1) + pmvC->iHalfY;
//		CoordI xRefT = ((nxcY + pmvT->iMVX) << 1) + pmvT->iHalfX, yRefT = ((nycY + pmvT->iMVY) << 1) + pmvT->iHalfY;
//		CoordI xRefB = ((nxcY + pmvB->iMVX) << 1) + pmvB->iHalfX, yRefB = ((nycY + pmvB->iMVY) << 1) + pmvB->iHalfY;
//		CoordI xRefR = ((nxcY + pmvR->iMVX) << 1) + pmvR->iHalfX, yRefR = ((nycY + pmvR->iMVY) << 1) + pmvR->iHalfY;
//		CoordI xRefL = ((nxcY + pmvL->iMVX) << 1) + pmvL->iHalfX, yRefL = ((nycY + pmvL->iMVY) << 1) + pmvL->iHalfY;

		limitMVRangeToExtendedBBHalfPel (xRefC,yRefC,prctMVLimit,BLOCK_SIZE);
		limitMVRangeToExtendedBBHalfPel (xRefT,yRefT,prctMVLimit,BLOCK_SIZE);
		limitMVRangeToExtendedBBHalfPel (xRefB,yRefB,prctMVLimit,BLOCK_SIZE);
		limitMVRangeToExtendedBBHalfPel (xRefR,yRefR,prctMVLimit,BLOCK_SIZE);
		limitMVRangeToExtendedBBHalfPel (xRefL,yRefL,prctMVLimit,BLOCK_SIZE);

		//PixelC* ppxlcPredY = ppxlcYMB + dxc + dyc * m_iWidthY;  // Starting of Pred. Frame
		PixelC* ppxlcPred = m_ppxlcPredMBY + dxc + dyc * MB_SIZE;  // Starting of Pred. Frame
		// 5 starting pos. in Zoomed Ref. Frames
		// m_ppxlcRefZoom0Y +  (xRefC, yRefC);
		const PixelC* ppxlcPrevZoomYC;
		const PixelC* ppxlcPrevZoomYT;
		const PixelC* ppxlcPrevZoomYB;
		const PixelC* ppxlcPrevZoomYR;
		const PixelC* ppxlcPrevZoomYL;
		ppxlcPrevZoomYC = m_puciRefQZoom0->pixels () + 
			(yRefC + EXPANDY_REF_FRAMEx2) * m_iFrameWidthZoomY +
			xRefC + EXPANDY_REF_FRAMEx2;
		ppxlcPrevZoomYT = m_puciRefQZoom0->pixels () + 
			(yRefT + EXPANDY_REF_FRAMEx2) * m_iFrameWidthZoomY +
			xRefT + EXPANDY_REF_FRAMEx2;
		ppxlcPrevZoomYB = m_puciRefQZoom0->pixels () + 
			(yRefB + EXPANDY_REF_FRAMEx2) * m_iFrameWidthZoomY +
			xRefB + EXPANDY_REF_FRAMEx2;
		ppxlcPrevZoomYR = m_puciRefQZoom0->pixels () + 
			(yRefR + EXPANDY_REF_FRAMEx2) * m_iFrameWidthZoomY +
			xRefR + EXPANDY_REF_FRAMEx2;
		ppxlcPrevZoomYL = m_puciRefQZoom0->pixels () + 
			(yRefL + EXPANDY_REF_FRAMEx2) * m_iFrameWidthZoomY +
			xRefL + EXPANDY_REF_FRAMEx2;
		UInt *pWghtC, *pWghtT, *pWghtB, *pWghtR, *pWghtL;
		pWghtC = (UInt*) gWghtC;
		pWghtT = (UInt*) gWghtT;
		pWghtB = (UInt*) gWghtB;
		pWghtR = (UInt*) gWghtR;
		pWghtL = (UInt*) gWghtL;
		for (UInt iy = 0; iy < BLOCK_SIZE; iy++) {
			for (UInt ix = 0; ix < BLOCK_SIZE; ix++) {
				*ppxlcPred++ = (
					*ppxlcPrevZoomYC * *pWghtC++ + 
					*ppxlcPrevZoomYT * *pWghtT++ + 
					*ppxlcPrevZoomYB * *pWghtB++ + 
					*ppxlcPrevZoomYR * *pWghtR++ + 
					*ppxlcPrevZoomYL * *pWghtL++ + 4
				) >> 3;
				ppxlcPrevZoomYC += 2;
				ppxlcPrevZoomYT += 2;
				ppxlcPrevZoomYB += 2;
				ppxlcPrevZoomYR += 2;
				ppxlcPrevZoomYL += 2;
			}
			ppxlcPrevZoomYC += m_iFrameWidthZoomYx2Minus2Blk;
			ppxlcPrevZoomYT += m_iFrameWidthZoomYx2Minus2Blk;
			ppxlcPrevZoomYB += m_iFrameWidthZoomYx2Minus2Blk;
			ppxlcPrevZoomYR += m_iFrameWidthZoomYx2Minus2Blk;
			ppxlcPrevZoomYL += m_iFrameWidthZoomYx2Minus2Blk;
			ppxlcPred += BLOCK_SIZE;
		}
	}
}

// for B-VOP

Void CVideoObjectEncoder::averagePredAndComputeErrorY()
{
	Int ic;
	for (ic = 0; ic < MB_SQUARE_SIZE; ic++) {
		m_ppxlcPredMBY [ic] = (m_ppxlcPredMBY [ic] + m_ppxlcPredMBBackY [ic] + 1) >> 1;
		m_ppxliErrorMBY [ic] = m_ppxlcCurrMBY [ic] - m_ppxlcPredMBY [ic];
	}
}

//INTERLACE
//new changes
Void CVideoObjectEncoder::averagePredAndComputeErrorY_WithShape()
{
	Int ic;
	for (ic = 0; ic < MB_SQUARE_SIZE; ic++) {
		if (m_ppxlcCurrMBBY [ic] == transpValue)
			m_ppxliErrorMBY [ic] = 0;
		else {
		m_ppxlcPredMBY [ic] = (m_ppxlcPredMBY [ic] + m_ppxlcPredMBBackY [ic] + 1) >> 1;
		m_ppxliErrorMBY [ic] = m_ppxlcCurrMBY [ic] - m_ppxlcPredMBY [ic];
		}
	}
}
// new changes
//INTERLACE

Void CVideoObjectEncoder::motionCompAndDiff_BVOP_MB (
	const CMotionVector* pmvForward, const CMotionVector* pmvBackward, 
	 CMBMode* pmbmd, 
	CoordI x, CoordI y,
	CRct *prctMVLimitForward,CRct *prctMVLimitBackward
)
{
    if (m_vopmd.bInterlace) {           // Should work for both progressive and interlaced, but keep
                                        // original code for now due to differences in direct mode.   Bob Eifrig
	    switch (pmbmd->m_mbType) {

	    case FORWARD:
		    motionCompOneBVOPReference(m_pvopcPredMB, FORWARD,  x, y, pmbmd, pmvForward, prctMVLimitForward);
		    computeTextureError();
		    break;

	    case BACKWARD:
		    motionCompOneBVOPReference(m_pvopcPredMB, BACKWARD, x, y, pmbmd, pmvBackward, prctMVLimitBackward);
		    computeTextureError();
		    break;

	    case DIRECT:
		    motionCompDirectMode(x, y, pmbmd,
                &m_rgmvRef[(PVOP_MV_PER_REF_PER_MB*(x + m_iNumMBX * y)) / MB_SIZE],
                prctMVLimitForward, prctMVLimitBackward,0);  // new change 02-19-99
		    averagePredAndComputeErrorY();
		    averagePredAndComputeErrorUV();
		    break;

	    case INTERPOLATE:
		    motionCompOneBVOPReference(m_pvopcPredMB,     FORWARD,  x, y, pmbmd, pmvForward, prctMVLimitForward);
		    motionCompOneBVOPReference(m_pvopcPredMBBack, BACKWARD, x, y, pmbmd, pmvBackward, prctMVLimitBackward);
		    averagePredAndComputeErrorY();
		    averagePredAndComputeErrorUV();
		    break;
	    }
        return;
    }
	if (pmbmd->m_mbType == DIRECT || pmbmd->m_mbType == INTERPOLATE) { // Y is done when doing motion estimation
		motionCompInterpAndError (pmvForward, pmvBackward, x, y,prctMVLimitForward,prctMVLimitBackward);
		CoordI xRefUVForward, yRefUVForward;
		mvLookupUVWithShape (pmbmd, pmvForward, xRefUVForward, yRefUVForward);
		motionCompUV (m_ppxlcPredMBU, m_ppxlcPredMBV, m_pvopcRefQ0, x, y, xRefUVForward, yRefUVForward, 0, prctMVLimitForward);
		
		CoordI xRefUVBackward, yRefUVBackward;
		mvLookupUVWithShape (pmbmd, pmvBackward, xRefUVBackward, yRefUVBackward);
		motionCompUV (m_ppxlcPredMBBackU, m_ppxlcPredMBBackV, m_pvopcRefQ1, x, y, xRefUVBackward, yRefUVBackward, 0, prctMVLimitBackward);
		averagePredAndComputeErrorUV ();
	}
	else { 
		const CMotionVector* pmv;
		const PixelC* ppxlcRef; // point to left-top of the reference VOP
		const PixelC* ppxlcRefZoom; // point to left-top of the reference VOP
		const CVOPU8YUVBA* pvopcRef;
		CRct *prctMVLimit;
		if (pmbmd->m_mbType == FORWARD) { // Y is done when doing motion estimation
			pmv = pmvForward;
			pvopcRef = m_pvopcRefQ0;
			ppxlcRef = m_pvopcRefQ0->pixelsY (); // point to left-top of the reference VOP
			ppxlcRefZoom = m_puciRefQZoom0->pixels (); // point to left-top of the reference VOP
			prctMVLimit=prctMVLimitForward;
		}
		else {
			pmv = pmvBackward;
			pvopcRef = m_pvopcRefQ1;
			ppxlcRef = m_pvopcRefQ1->pixelsY (); // point to left-top of the reference VOP
			ppxlcRefZoom = m_puciRefQZoom1->pixels (); // point to left-top of the reference VOP
			prctMVLimit=prctMVLimitBackward;
		}
		CoordI xRefUV, yRefUV;
		mvLookupUVWithShape (pmbmd, pmv, xRefUV, yRefUV);
		motionCompEncY (ppxlcRef, ppxlcRefZoom, m_ppxlcPredMBY, MB_SIZE, pmv, x, y,prctMVLimit);
		motionCompUV (m_ppxlcPredMBU, m_ppxlcPredMBV, pvopcRef, x, y, xRefUV, yRefUV, 0, prctMVLimit);
		computeTextureError ();
	}
}

Void CVideoObjectEncoder::motionCompAndDiff_BVOP_MB_WithShape (
	const CMotionVector* pmvForward, const CMotionVector* pmvBackward, 
	 CMBMode* pmbmd, 
	CoordI x, CoordI y,
	CRct *prctMVLimitForward,
	CRct *prctMVLimitBackward
)
{
// INTERLACED
// new changes
    if (m_vopmd.bInterlace&&pmbmd->m_bFieldMV) {           // Should work for both progressive and interlaced, but keep
                                        // original code for now due to differences in direct mode.   Bob Eifrig
	    switch (pmbmd->m_mbType) {

	    case FORWARD:
		    motionCompOneBVOPReference(m_pvopcPredMB, FORWARD,  x, y, pmbmd, pmvForward, prctMVLimitForward);
		    computeTextureErrorWithShape();
		    break;

	    case BACKWARD:
		    motionCompOneBVOPReference(m_pvopcPredMB, BACKWARD, x, y, pmbmd, pmvBackward, prctMVLimitBackward);
		    computeTextureErrorWithShape();
		    break;

	    case DIRECT:
		    motionCompDirectMode(x, y, pmbmd,
                &m_rgmvRef[(PVOP_MV_PER_REF_PER_MB*(x + m_iNumMBX * y)) / MB_SIZE],
                prctMVLimitForward, prctMVLimitBackward,0);   // new change 02-19-99
		    averagePredAndComputeErrorY_WithShape();
		    averagePredAndComputeErrorUV_WithShape();
		    break;

	    case INTERPOLATE:
		    motionCompOneBVOPReference(m_pvopcPredMB,     FORWARD,  x, y, pmbmd, pmvForward, prctMVLimitForward);
		    motionCompOneBVOPReference(m_pvopcPredMBBack, BACKWARD, x, y, pmbmd, pmvBackward, prctMVLimitBackward);
		    averagePredAndComputeErrorY_WithShape();
		    averagePredAndComputeErrorUV_WithShape();
		    break;
	    }
        return;
    }
// INTERLACED
// end of new changes

	if (pmbmd->m_mbType == DIRECT || pmbmd->m_mbType == INTERPOLATE) { // Y is done when doing motion estimation
		motionCompInterpAndError_WithShape (pmvForward, pmvBackward, x, y,prctMVLimitForward,prctMVLimitBackward);
		CoordI xRefUVForward, yRefUVForward;
		mvLookupUVWithShape (pmbmd, pmvForward, xRefUVForward, yRefUVForward);
		motionCompUV (m_ppxlcPredMBU, m_ppxlcPredMBV, m_pvopcRefQ0, x, y, xRefUVForward, yRefUVForward, 0, prctMVLimitForward);
		
		CoordI xRefUVBackward, yRefUVBackward;
		mvLookupUVWithShape (pmbmd, pmvBackward, xRefUVBackward, yRefUVBackward);
		motionCompUV (m_ppxlcPredMBBackU, m_ppxlcPredMBBackV, m_pvopcRefQ1, x, y, xRefUVBackward, yRefUVBackward, 0, prctMVLimitBackward);
		averagePredAndComputeErrorUV_WithShape ();
	}
	else { 
		const CMotionVector* pmv;
		const PixelC* ppxlcRef; // point to left-top of the reference VOP
		const PixelC* ppxlcRefZoom; // point to left-top of the reference VOP
		const CVOPU8YUVBA* pvopcRef;
		CRct *prctMVLimit;
		if (pmbmd->m_mbType == FORWARD) { // Y is done when doing motion estimation
			pmv = pmvForward;
			pvopcRef = m_pvopcRefQ0;
			ppxlcRef = m_pvopcRefQ0->pixelsY (); // point to left-top of the reference VOP
			ppxlcRefZoom = m_puciRefQZoom0->pixels (); // point to left-top of the reference VOP
			prctMVLimit = prctMVLimitForward;
		}
		else {
			pmv = pmvBackward;
			pvopcRef = m_pvopcRefQ1;
			ppxlcRef = m_pvopcRefQ1->pixelsY (); // point to left-top of the reference VOP
			ppxlcRefZoom = m_puciRefQZoom1->pixels (); // point to left-top of the reference VOP
			prctMVLimit = prctMVLimitBackward;
		}
		CoordI xRefUV, yRefUV;
		mvLookupUVWithShape (pmbmd, pmv, xRefUV, yRefUV);
		motionCompEncY (ppxlcRef, ppxlcRefZoom, m_ppxlcPredMBY, MB_SIZE, pmv, x, y,prctMVLimit);
		motionCompUV (m_ppxlcPredMBU, m_ppxlcPredMBV, pvopcRef, x, y, xRefUV, yRefUV, 0, prctMVLimit);
		computeTextureErrorWithShape ();
	}
}

Void CVideoObjectEncoder::motionCompAndDiffAlpha_BVOP_MB (
	const CMotionVector* pmvForward, const CMotionVector* pmvBackward, 
	const CMBMode* pmbmd, 
	CoordI x, CoordI y,
	CRct *prctMVLimitForward,CRct *prctMVLimitBackward
)
{
	if (pmbmd->m_mbType == DIRECT || pmbmd->m_mbType == INTERPOLATE) {
		motionCompMBAEnc (pmvForward, pmbmd, m_ppxlcPredMBA, m_pvopcRefQ0,
			x, y, 0, prctMVLimitForward,0);
		motionCompMBAEnc (pmvBackward, pmbmd, m_ppxlcPredMBBackA, m_pvopcRefQ1,
			x, y, 0, prctMVLimitBackward,1);

		// average predictions
		Int i;
		for(i = 0; i<MB_SQUARE_SIZE; i++)
			m_ppxlcPredMBA[i] = (m_ppxlcPredMBA[i] + m_ppxlcPredMBBackA[i] + 1)>>1;
	}
	else { 
		const CMotionVector* pmv;
		const PixelC* ppxlcRef; // point to left-top of the reference VOP
		CRct *prctMVLimit;
		if (pmbmd->m_mbType == FORWARD) {
			pmv = pmvForward;
			ppxlcRef = m_pvopcRefQ0->pixelsA (); // point to left-top of the reference VOP
			prctMVLimit = prctMVLimitForward;
// 12.22.98 begin of changes
			if(m_vopmd.bInterlace&&pmbmd->m_bFieldMV)
				motionCompMBAEnc (pmvForward, pmbmd, m_ppxlcPredMBA, m_pvopcRefQ0,
					x, y, 0, prctMVLimitForward,0);
// end of changes
		}
		else {
			pmv = pmvBackward;
			ppxlcRef = m_pvopcRefQ1->pixelsA (); // point to left-top of the reference VOP
			prctMVLimit = prctMVLimitBackward;
// 12.22.98 begin of changes
			if(m_vopmd.bInterlace&&pmbmd->m_bFieldMV)
				motionCompMBAEnc (pmvBackward, pmbmd, m_ppxlcPredMBA, m_pvopcRefQ1,
					x, y, 0, prctMVLimitBackward,1);
// end of changes
		}
		if((!m_vopmd.bInterlace||(m_vopmd.bInterlace&&!pmbmd->m_bFieldMV))) // 12.22.98 changes
			motionComp (
				m_ppxlcPredMBA,
				ppxlcRef,
				MB_SIZE, // MB size
				x * 2 + pmv->trueMVHalfPel ().x, 
				y * 2 + pmv->trueMVHalfPel ().y,
				0,
				prctMVLimit
			);
	}
	computeAlphaError();
}

Void CVideoObjectEncoder::averagePredAndComputeErrorUV ()
{
	Int i = 0;
	for (i = 0; i < BLOCK_SQUARE_SIZE; i++) {
		m_ppxlcPredMBU [i] = (m_ppxlcPredMBU [i] + m_ppxlcPredMBBackU [i] + 1) >> 1;
		m_ppxlcPredMBV [i] = (m_ppxlcPredMBV [i] + m_ppxlcPredMBBackV [i] + 1) >> 1;
		m_ppxliErrorMBU [i] = m_ppxlcCurrMBU [i] - m_ppxlcPredMBU [i];
		m_ppxliErrorMBV [i] = m_ppxlcCurrMBV [i] - m_ppxlcPredMBV [i];
	}
}

Void CVideoObjectEncoder::averagePredAndComputeErrorUV_WithShape ()
{
	Int i = 0;
	for (i = 0; i < BLOCK_SQUARE_SIZE; i++) {
		if (m_ppxlcCurrMBBUV [i] == transpValue)
			m_ppxliErrorMBU [i] = m_ppxliErrorMBV [i] = 0;
		else {
			m_ppxlcPredMBU [i] = (m_ppxlcPredMBU [i] + m_ppxlcPredMBBackU [i] + 1) >> 1;
			m_ppxlcPredMBV [i] = (m_ppxlcPredMBV [i] + m_ppxlcPredMBBackV [i] + 1) >> 1;
			m_ppxliErrorMBU [i] = m_ppxlcCurrMBU [i] - m_ppxlcPredMBU [i];
			m_ppxliErrorMBV [i] = m_ppxlcCurrMBV [i] - m_ppxlcPredMBV [i];
		}
	}
}

// B-VOP ME/MC stuff
Int CVideoObjectEncoder::interpolateAndDiffY (
	const CMotionVector* pmvForward, const CMotionVector* pmvBackward, 
	CoordI x, CoordI y,
	CRct *prctMVLimitForward,CRct *prctMVLimitBackward
)
{
	motionCompEncY (
		m_pvopcRefQ0->pixelsY (), 
		m_puciRefQZoom0->pixels (),
		m_ppxlcPredMBY, MB_SIZE, pmvForward, x, y,
		prctMVLimitForward
	);
	/*Int iLeft  = (x << 1) + pmvBackward->m_vctTrueHalfPel.x;
	Int iRight = (y << 1) + pmvBackward->m_vctTrueHalfPel.y;	

	//make sure don't end up with a mv that points out of a frame; object-based not done
	if (!m_puciRefQZoom1->where ().includes (CRct (iLeft, iRight, iLeft + (MB_SIZE << 1), iRight + (MB_SIZE << 1))))
		return 1000000000;
	else*/
	motionCompEncY (
		m_pvopcRefQ1->pixelsY (), 
		m_puciRefQZoom1->pixels (),
		m_ppxlcPredMBBackY, MB_SIZE, pmvBackward, x, y,
		prctMVLimitBackward
	);
	Int ic;
	Int iSAD = 0;
	for (ic = 0; ic < MB_SQUARE_SIZE; ic++)
		iSAD += abs (m_ppxlcCurrMBY [ic] - ((m_ppxlcPredMBY [ic] + m_ppxlcPredMBBackY [ic] + 1) >> 1));
	return iSAD;
}

Int CVideoObjectEncoder::interpolateAndDiffY_WithShape (
	const CMotionVector* pmvForward, const CMotionVector* pmvBackward, 
	CoordI x, CoordI y,
	CRct *prctMVLimitForward,CRct *prctMVLimitBackward
)
{
	motionCompEncY (
		m_pvopcRefQ0->pixelsY (), 
		m_puciRefQZoom0->pixels (),
		m_ppxlcPredMBY, MB_SIZE, pmvForward, x, y,
		prctMVLimitForward
	);
	motionCompEncY (
		m_pvopcRefQ1->pixelsY (), 
		m_puciRefQZoom1->pixels (),
		m_ppxlcPredMBBackY, MB_SIZE, pmvBackward, x, y,
		prctMVLimitBackward
	);
	Int ic;
	Int iSAD = 0;
	for (ic = 0; ic < MB_SQUARE_SIZE; ic++) {
		if (m_ppxlcCurrMBBY [ic] != transpValue)
			iSAD += abs (m_ppxlcCurrMBY [ic] - ((m_ppxlcPredMBY [ic] + m_ppxlcPredMBBackY [ic] + 1) >> 1));
	}
	return iSAD;
}

Int CVideoObjectEncoder::interpolateAndDiffYField(
	const CMotionVector* pmvFwdTop,
	const CMotionVector* pmvFwdBot,
	const CMotionVector* pmvBakTop,
	const CMotionVector* pmvBakBot,
	CoordI x, CoordI y,
	CMBMode *pmbmd
)
{
	x <<= 1;
	y <<= 1;
	motionCompYField(m_ppxlcPredMBY,
		m_pvopcRefQ0->pixelsY () + pmbmd->m_bForwardTop * m_iFrameWidthY,
		x + pmvFwdTop->m_vctTrueHalfPel.x, y + pmvFwdTop->m_vctTrueHalfPel.y);
	motionCompYField(m_ppxlcPredMBY + MB_SIZE,
		m_pvopcRefQ0->pixelsY () + pmbmd->m_bForwardBottom * m_iFrameWidthY,
		x + pmvFwdBot->m_vctTrueHalfPel.x, y + pmvFwdBot->m_vctTrueHalfPel.y);

	motionCompYField(m_ppxlcPredMBBackY,
		m_pvopcRefQ1->pixelsY () + pmbmd->m_bBackwardTop * m_iFrameWidthY,
		x + pmvBakTop->m_vctTrueHalfPel.x, y + pmvBakTop->m_vctTrueHalfPel.y);
	motionCompYField(m_ppxlcPredMBBackY + MB_SIZE,
		m_pvopcRefQ1->pixelsY () + pmbmd->m_bBackwardBottom * m_iFrameWidthY,
		x + pmvBakBot->m_vctTrueHalfPel.x, y + pmvBakBot->m_vctTrueHalfPel.y);
	
	Int ic;
	Int iSAD = 0;
// new changes by X. Chen
	if (pmbmd->m_rgTranspStatus[0]==NONE) {
		for (ic = 0; ic < MB_SQUARE_SIZE; ic++)
			iSAD += abs (m_ppxlcCurrMBY [ic] - ((m_ppxlcPredMBY [ic] + m_ppxlcPredMBBackY [ic] + 1) >> 1));
	} else if (pmbmd->m_rgTranspStatus[0]==PARTIAL) {
		for (ic = 0; ic < MB_SQUARE_SIZE; ic++) {
			if (m_ppxlcCurrMBBY [ic] != transpValue)
				iSAD += abs (m_ppxlcCurrMBY [ic] - ((m_ppxlcPredMBY [ic] + m_ppxlcPredMBBackY [ic] + 1) >> 1));
		}
	}
// end of new changes
	return iSAD;
}


Void CVideoObjectEncoder::motionCompInterpAndError (
	const CMotionVector* pmvForward, const CMotionVector* pmvBackward, 
	CoordI x, CoordI y,
	CRct *prctMVLimitForward,CRct *prctMVLimitBackward
)
{
	motionCompEncY (
		m_pvopcRefQ0->pixelsY (), 
		m_puciRefQZoom0->pixels (),
		m_ppxlcPredMBY, MB_SIZE, pmvForward, x, y,
		prctMVLimitForward
	);
	motionCompEncY (
		m_pvopcRefQ1->pixelsY (), 
		m_puciRefQZoom1->pixels (),
		m_ppxlcPredMBBackY, MB_SIZE, pmvBackward, x, y,
		prctMVLimitBackward
	);
	Int ic;
	for (ic = 0; ic < MB_SQUARE_SIZE; ic++) {
		m_ppxlcPredMBY [ic] = (m_ppxlcPredMBY [ic] + m_ppxlcPredMBBackY [ic] + 1) >> 1;
		m_ppxliErrorMBY [ic] = m_ppxlcCurrMBY [ic] - m_ppxlcPredMBY [ic];
	}
}

Void CVideoObjectEncoder::motionCompInterpAndError_WithShape (
	const CMotionVector* pmvForward, const CMotionVector* pmvBackward, 
	CoordI x, CoordI y,
	CRct *prctMVLimitForward,CRct *prctMVLimitBackward
)
{
	motionCompEncY (
		m_pvopcRefQ0->pixelsY (), 
		m_puciRefQZoom0->pixels (),
		m_ppxlcPredMBY, MB_SIZE, pmvForward, x, y,
		prctMVLimitForward
	);
	motionCompEncY (
		m_pvopcRefQ1->pixelsY (), 
		m_puciRefQZoom1->pixels (),
		m_ppxlcPredMBBackY, MB_SIZE, pmvBackward, x, y,
		prctMVLimitBackward
	);
	Int ic;
	for (ic = 0; ic < MB_SQUARE_SIZE; ic++) {
		if (m_ppxlcCurrMBBY [ic] == transpValue){
		m_ppxlcPredMBY [ic] = (m_ppxlcPredMBY [ic] + m_ppxlcPredMBBackY [ic] + 1) >> 1;
		/*m_ppxlcPredMBY [ic] = */m_ppxliErrorMBY [ic] = 0;
		}
		else {
			m_ppxlcPredMBY [ic] = (m_ppxlcPredMBY [ic] + m_ppxlcPredMBBackY [ic] + 1) >> 1;
			m_ppxliErrorMBY [ic] = m_ppxlcCurrMBY [ic] - m_ppxlcPredMBY [ic];
		}
	}
}

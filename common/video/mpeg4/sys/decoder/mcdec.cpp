/*************************************************************************

This software module was originally developed by 

	Ming-Chieh Lee (mingcl@microsoft.com), Microsoft Corporation
	Wei-ge Chen (wchen@microsoft.com), Microsoft Corporation
	(date: July, 1997)

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

	mcdec.cpp

Abstract:

	motion compensation for decoder

Revision History:
	Dec 20, 1997:	Interlaced tools added by NextLevel Systems (GI)
                    B. Eifrig (beifrig@nlvl.com) X. Chen (xchen@nlvl.com)
*************************************************************************/
#include <stdio.h>
#include <math.h>
#include <stdlib.h>

#include "typeapi.h"
#include "mode.hpp"
#include "codehead.h"
#include "entropy/bitstrm.hpp"
#include "entropy/entropy.hpp"
#include "entropy/huffman.hpp"
#include "global.hpp"
#include "vopses.hpp"
#include "vopsedec.hpp"

#ifdef __MFC_
#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif
#define new DEBUG_NEW				   
#endif // __MFC_

const Int rgiBlkOffsetX [] = {0, BLOCK_SIZE, 0, BLOCK_SIZE};
const Int rgiBlkOffsetY [] = {0, 0, BLOCK_SIZE, BLOCK_SIZE};
const Int rgiBlkOffsetPixel [] = {0, OFFSET_BLK1, OFFSET_BLK2, OFFSET_BLK3};

Void CVideoObjectDecoder::motionCompAndAddErrorMB_BVOP (
	const CMotionVector* pmvForward, const CMotionVector* pmvBackward,
	 CMBMode* pmbmd, 
	Int iMBX, Int iMBY, 
	CoordI x, CoordI y,
	PixelC* ppxlcCurrQMBY, PixelC* ppxlcCurrQMBU, PixelC* ppxlcCurrQMBV,
	CRct *prctMVLimitForward,CRct *prctMVLimitBackward
)
{
    Int iBlk;
// INTERLACE
	// new chnages
    if (m_vopmd.bInterlace) {         // Should not depend on .bInterlace; this code should work the progressive
 
		switch (pmbmd->m_mbType) {

	    case FORWARD:
		    motionCompOneBVOPReference(m_pvopcPredMB, FORWARD, x, y, pmbmd, pmvForward, prctMVLimitForward);
		    addErrorAndPredToCurrQ (ppxlcCurrQMBY, ppxlcCurrQMBU, ppxlcCurrQMBV);
		    break;

	    case BACKWARD:
		    motionCompOneBVOPReference(m_pvopcPredMB, BACKWARD, x, y, pmbmd, pmvBackward, prctMVLimitBackward);
		    addErrorAndPredToCurrQ (ppxlcCurrQMBY, ppxlcCurrQMBU, ppxlcCurrQMBV);
		    break;

	    case DIRECT:
			Int iOffset;
			if(m_volmd.fAUsage != RECTANGLE)
				iOffset = (MIN (MAX (0, iMBX), m_iNumMBXRef - 1) + 
				MIN (MAX (0, iMBY), m_iNumMBYRef - 1) * m_iNumMBXRef) * PVOP_MV_PER_REF_PER_MB;
			else
				iOffset=PVOP_MV_PER_REF_PER_MB*(iMBX + iMBY*m_iNumMBX);
			motionCompDirectMode(x, y, pmbmd, &m_rgmvRef[iOffset],
  			prctMVLimitForward, prctMVLimitBackward, 0);
			averagePredAndAddErrorToCurrQ (ppxlcCurrQMBY, ppxlcCurrQMBU, ppxlcCurrQMBV);
		    break;

	    case INTERPOLATE:
		    motionCompOneBVOPReference(m_pvopcPredMB, FORWARD, x, y, pmbmd, pmvForward, prctMVLimitForward);
		    motionCompOneBVOPReference(m_pvopcPredMBBack, BACKWARD, x, y, pmbmd, pmvBackward, prctMVLimitBackward);
		    averagePredAndAddErrorToCurrQ (ppxlcCurrQMBY, ppxlcCurrQMBU, ppxlcCurrQMBV);
		    break;
	    }
        return;
    }
// ~INTERLACE

	if (pmbmd->m_mbType == DIRECT || pmbmd->m_mbType == INTERPOLATE) {
		if (pmbmd->m_bhas4MVForward != TRUE)
			motionComp (
				m_ppxlcPredMBY,
				m_pvopcRefQ0->pixelsY (),
				MB_SIZE, // either MB or BLOCK size
				x * 2 + pmvForward->trueMVHalfPel ().x, 
				y * 2 + pmvForward->trueMVHalfPel ().y,
				m_vopmd.iRoundingControl,
				prctMVLimitForward
			);
		else {
			const CMotionVector* pmv8 = pmvForward;
			for (iBlk = 0; iBlk < 4; iBlk++)	{
				pmv8++;
				if (pmbmd->m_rgTranspStatus [iBlk + 1] != ALL)
					motionComp (
						m_ppxlcPredMBY + rgiBlkOffsetPixel [iBlk], 
						m_pvopcRefQ0->pixelsY (),
						BLOCK_SIZE, 
						(x + rgiBlkOffsetX [iBlk]) * 2 + pmv8->trueMVHalfPel ().x, 
						(y + rgiBlkOffsetY [iBlk]) * 2 + pmv8->trueMVHalfPel ().y,
						m_vopmd.iRoundingControl,
						prctMVLimitForward
					);
			}
		}
		CoordI xRefUVForward, yRefUVForward;
		mvLookupUVWithShape (pmbmd, pmvForward, xRefUVForward, yRefUVForward);
		motionCompUV (m_ppxlcPredMBU, m_ppxlcPredMBV, m_pvopcRefQ0, x, y, xRefUVForward, yRefUVForward, 0, prctMVLimitForward);

		if (pmbmd->m_bhas4MVBackward != TRUE)		
			motionComp (
				m_ppxlcPredMBBackY,
				m_pvopcRefQ1->pixelsY (),
				MB_SIZE, // either MB or BLOCK size
				x * 2 + pmvBackward->trueMVHalfPel ().x, 
				y * 2 + pmvBackward->trueMVHalfPel ().y,
				m_vopmd.iRoundingControl,
				prctMVLimitBackward
			);
		else {
			const CMotionVector* pmv8 = pmvBackward;
			for (iBlk = 0; iBlk < 4; iBlk++)	{
				pmv8++;
				if (pmbmd->m_rgTranspStatus [iBlk + 1] != ALL)
					motionComp (
						m_ppxlcPredMBBackY + rgiBlkOffsetPixel [iBlk], 
						m_pvopcRefQ1->pixelsY (),
						BLOCK_SIZE, 
						(x + rgiBlkOffsetX [iBlk]) * 2 + pmv8->trueMVHalfPel ().x, 
						(y + rgiBlkOffsetY [iBlk]) * 2 + pmv8->trueMVHalfPel ().y,
						m_vopmd.iRoundingControl,
						prctMVLimitBackward
					);
			}
		}
		CoordI xRefUVBackward, yRefUVBackward;
		mvLookupUVWithShape (pmbmd, pmvBackward, xRefUVBackward, yRefUVBackward);
		motionCompUV (m_ppxlcPredMBBackU, m_ppxlcPredMBBackV, m_pvopcRefQ1, x, y, xRefUVBackward, yRefUVBackward, 0, prctMVLimitBackward);
		averagePredAndAddErrorToCurrQ (ppxlcCurrQMBY, ppxlcCurrQMBU, ppxlcCurrQMBV);
	}
	else { 
		const CMotionVector* pmv;
		const PixelC* ppxlcRef; // point to left-top of the reference VOP
		const CVOPU8YUVBA* pvopcRef;
		CRct *prctMVLimit;
		if (pmbmd->m_mbType == FORWARD) { // Y is done when doing motion estimation
			pmv = pmvForward;
			ppxlcRef = m_pvopcRefQ0->pixelsY (); // point to left-top of the reference VOP
			pvopcRef = m_pvopcRefQ0;
			prctMVLimit = prctMVLimitForward;
		}
		else {
			pmv = pmvBackward;
			ppxlcRef = m_pvopcRefQ1->pixelsY (); // point to left-top of the reference VOP
			pvopcRef = m_pvopcRefQ1;
			prctMVLimit = prctMVLimitBackward;
		}
		motionComp (
			m_ppxlcPredMBY,
			ppxlcRef,
			MB_SIZE, // either MB or BLOCK size
			x * 2 + pmv->trueMVHalfPel ().x, 
			y * 2 + pmv->trueMVHalfPel ().y,
			m_vopmd.iRoundingControl,
			prctMVLimit
		);
		CoordI xRefUV, yRefUV;
		mvLookupUVWithShape (pmbmd, pmv, xRefUV, yRefUV);
		motionCompUV (m_ppxlcPredMBU, m_ppxlcPredMBV, pvopcRef, x, y, xRefUV, yRefUV, 0, prctMVLimit);
		addErrorAndPredToCurrQ (ppxlcCurrQMBY, ppxlcCurrQMBU, ppxlcCurrQMBV);
	}
}

Void CVideoObjectDecoder::motionCompAlphaMB_BVOP(
	const CMotionVector* pmvForward, const CMotionVector* pmvBackward,
	 CMBMode* pmbmd, 
	Int iMBX, Int iMBY, 
	CoordI x, CoordI y,
	PixelC* ppxlcCurrQMBA,
	CRct *prctMVLimitForward,CRct *prctMVLimitBackward)
{
    Int iBlk;
	if (pmbmd->m_mbType == DIRECT || pmbmd->m_mbType == INTERPOLATE) {
		if (!pmbmd->m_bhas4MVForward && !pmbmd -> m_bFieldMV) //12.22.98
			motionComp (	
				m_ppxlcPredMBA,
				m_pvopcRefQ0->pixelsA (),
				MB_SIZE, // either MB or BLOCK size
				x * 2 + pmvForward->trueMVHalfPel ().x, 
				y * 2 + pmvForward->trueMVHalfPel ().y,
				0,
				prctMVLimitForward
			);
// INTERLACE  12.22.98 with new change 02-19-99
		else if ((pmbmd -> m_bFieldMV) && (pmbmd->m_mbType == INTERPOLATE)) { 
			const CMotionVector* pmvTop = pmvForward + 1 + pmbmd->m_bForwardTop;
			motionCompYField(m_ppxlcPredMBA,
				m_pvopcRefQ0->pixelsA () + pmbmd->m_bForwardTop * m_iFrameWidthY,
				2*x + pmvTop->trueMVHalfPel ().x, 2*y + pmvTop->trueMVHalfPel ().y);

			const CMotionVector* pmvBottom = pmvForward + 3 + pmbmd->m_bForwardBottom;
			motionCompYField(m_ppxlcPredMBA + MB_SIZE,
				m_pvopcRefQ0->pixelsA () + pmbmd->m_bForwardBottom * m_iFrameWidthY,
				2*x + pmvBottom->trueMVHalfPel ().x, 2*y + pmvBottom->trueMVHalfPel ().y);
			}

 // ~INTERLACE 12.22.98 with new change 02-19-99

// INTERLACE 02-19-99
		else if ((pmbmd -> m_bFieldMV) && (pmbmd->m_mbType == DIRECT)) {
			Int iOffset = (MIN (MAX (0,iMBX), m_iNumMBXRef - 1) +
				MIN (MAX (0, iMBY), m_iNumMBYRef - 1) * m_iNumMBXRef) * PVOP_MV_PER_REF_PER_MB;
			motionCompDirectMode(x, y, pmbmd, &m_rgmvRef[iOffset],
				prctMVLimitForward, prctMVLimitBackward, 1);
		}
// ~INTERLACE 02-19-99
		else {
			const CMotionVector* pmv8 = pmvForward;
			for (iBlk = 0; iBlk < 4; iBlk++)	{
				pmv8++;
				if (pmbmd->m_rgTranspStatus [iBlk + 1] != ALL)
					motionComp (
						m_ppxlcPredMBA + rgiBlkOffsetPixel [iBlk], 
						m_pvopcRefQ0->pixelsA (),
						BLOCK_SIZE, 
						(x + rgiBlkOffsetX [iBlk]) * 2 + pmv8->trueMVHalfPel ().x, 
						(y + rgiBlkOffsetY [iBlk]) * 2 + pmv8->trueMVHalfPel ().y,
						0,
						prctMVLimitForward
					);
			}
		}
		if (!pmbmd->m_bhas4MVBackward && !pmbmd -> m_bFieldMV)	//12.22.98	
			motionComp (
				m_ppxlcPredMBBackA,
				m_pvopcRefQ1->pixelsA (),
				MB_SIZE, // either MB or BLOCK size
				x * 2 + pmvBackward->trueMVHalfPel ().x, 
				y * 2 + pmvBackward->trueMVHalfPel ().y,
				0,
				prctMVLimitBackward
			);
// INTERLACE  12.22.98 with new change 02-19-99

		else if (pmbmd -> m_bFieldMV && (pmbmd->m_mbType == INTERPOLATE)) {
			const CMotionVector* pmvTop = pmvBackward + 1 + pmbmd->m_bBackwardTop;
			motionCompYField(m_ppxlcPredMBBackA,
				m_pvopcRefQ1->pixelsA () + pmbmd->m_bBackwardTop * m_iFrameWidthY,
				2*x + pmvTop->trueMVHalfPel ().x, 2*y + pmvTop->trueMVHalfPel ().y);

			const CMotionVector* pmvBottom = pmvBackward + 3 + pmbmd->m_bBackwardBottom;
			motionCompYField(m_ppxlcPredMBBackA + MB_SIZE,
				m_pvopcRefQ1->pixelsA () + pmbmd->m_bBackwardBottom * m_iFrameWidthY,
				2*x + pmvBottom->trueMVHalfPel ().x, 2*y + pmvBottom->trueMVHalfPel ().y);
			}
		else if (((pmbmd -> m_bFieldMV)==0) || (pmbmd->m_mbType != DIRECT)) {
//		else {
// ~INTERLACE 12.22.98 with new change 02-19-99
			const CMotionVector* pmv8 = pmvBackward;
			for (iBlk = 0; iBlk < 4; iBlk++)	{
				pmv8++;
				if (pmbmd->m_rgTranspStatus [iBlk + 1] != ALL)
					motionComp (
						m_ppxlcPredMBBackA + rgiBlkOffsetPixel [iBlk], 
						m_pvopcRefQ1->pixelsA (),
						BLOCK_SIZE, 
						(x + rgiBlkOffsetX [iBlk]) * 2 + pmv8->trueMVHalfPel ().x, 
						(y + rgiBlkOffsetY [iBlk]) * 2 + pmv8->trueMVHalfPel ().y,
						0,
						prctMVLimitBackward
					);
			}
		}

		// average predictions
		Int i;
		for(i = 0; i<MB_SQUARE_SIZE; i++)
			m_ppxlcPredMBA[i] = (m_ppxlcPredMBA[i] + m_ppxlcPredMBBackA[i] + 1)>>1;
	}
	else { 
		const CMotionVector* pmv;
		const PixelC* ppxlcRef; // point to left-top of the reference VOP
		CRct *prctMVLimit;
		Int itmpref1,itmpref2; // 12.22.98
		if (pmbmd->m_mbType == FORWARD) {
			pmv = pmvForward;
			ppxlcRef = m_pvopcRefQ0->pixelsA (); // point to left-top of the reference VOP
			prctMVLimit = prctMVLimitForward;
			itmpref1=pmbmd->m_bForwardTop; // 12.22.98
			itmpref2=pmbmd->m_bForwardBottom; // 12.22.98
		}
		else {
			pmv = pmvBackward;
			ppxlcRef = m_pvopcRefQ1->pixelsA (); // point to left-top of the reference VOP
			prctMVLimit = prctMVLimitBackward;
			itmpref1=pmbmd->m_bBackwardTop;  // 12.22.98
			itmpref2=pmbmd->m_bBackwardBottom;  // 12.22.98
		}
// INTERLACE  12.22.98
		if (pmbmd -> m_bFieldMV) {
			const CMotionVector* pmvTop = pmv + 1 + itmpref1;
			motionCompYField(m_ppxlcPredMBA,
				ppxlcRef + itmpref1 * m_iFrameWidthY,
				2*x + pmvTop->trueMVHalfPel ().x, 2*y + pmvTop->trueMVHalfPel ().y);

			const CMotionVector* pmvBottom = pmv + 3 + itmpref2;
			motionCompYField(m_ppxlcPredMBA + MB_SIZE,
				ppxlcRef + itmpref2 * m_iFrameWidthY,
				2*x + pmvBottom->trueMVHalfPel ().x, 2*y + pmvBottom->trueMVHalfPel ().y);
			}
		else
// ~INTERLACE 12.22.98			
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
}

Void CVideoObjectDecoder::copyFromRefToCurrQ_BVOP (
	const CMBMode* pmbmd,
	CoordI x, CoordI y, 
	PixelC* ppxlcCurrQMBY, PixelC* ppxlcCurrQMBU, PixelC* ppxlcCurrQMBV,
	CRct *prctMVLimitForward,CRct *prctMVLimitBackward
)
{
	if (pmbmd->m_mbType == DIRECT || pmbmd->m_mbType == INTERPOLATE) { // Y is done when doing motion estimation
		motionComp (
			m_ppxlcPredMBY,
			m_pvopcRefQ0->pixelsY (),
			MB_SIZE, // either MB or BLOCK size
			x * 2, 
			y * 2,
			m_vopmd.iRoundingControl,
			prctMVLimitForward
		);
		motionCompUV (m_ppxlcPredMBU, m_ppxlcPredMBV, m_pvopcRefQ0,
			x, y, 0, 0, 0,prctMVLimitForward);
		
		motionComp (
			m_ppxlcPredMBBackY,
			m_pvopcRefQ1->pixelsY (),
			MB_SIZE, // either MB or BLOCK size
			x * 2, 
			y * 2,
			m_vopmd.iRoundingControl,
			prctMVLimitBackward
		);
		motionCompUV (m_ppxlcPredMBBackU, m_ppxlcPredMBBackV, m_pvopcRefQ1,
			x, y, 0, 0, 0, prctMVLimitBackward);
		averagePredAndAssignToCurrQ (ppxlcCurrQMBY, ppxlcCurrQMBU, ppxlcCurrQMBV);
	}
	else { 
		const CVOPU8YUVBA* pvopcRef; // point to left-top of the reference VOP
		CRct *prctMVLimit;
		if (pmbmd->m_mbType == FORWARD){
			pvopcRef = m_pvopcRefQ0;
			prctMVLimit = prctMVLimitForward;
		}
		else {
			pvopcRef = m_pvopcRefQ1;
			prctMVLimit = prctMVLimitBackward;
		}
		copyFromRefToCurrQ (pvopcRef, x, y, ppxlcCurrQMBY, ppxlcCurrQMBU, ppxlcCurrQMBV, prctMVLimit);
	}
}

Void CVideoObjectDecoder::motionCompSkipMB_BVOP (
	const CMBMode* pmbmd, const CMotionVector* pmvForward, const CMotionVector* pmvBackward,
	CoordI x, CoordI y, 
	PixelC* ppxlcCurrQMBY, PixelC* ppxlcCurrQMBU, PixelC* ppxlcCurrQMBV,
	CRct *prctMVLimitForward,CRct *prctMVLimitBackward
)
{
	Int iBlk;
	if (pmbmd->m_mbType == DIRECT || pmbmd->m_mbType == INTERPOLATE) {
		if (pmbmd->m_bhas4MVForward != TRUE)
			motionComp (
				m_ppxlcPredMBY,
				m_pvopcRefQ0->pixelsY (),
				MB_SIZE, // either MB or BLOCK size
				x * 2 + pmvForward->trueMVHalfPel ().x, 
				y * 2 + pmvForward->trueMVHalfPel ().y,
				m_vopmd.iRoundingControl,
				prctMVLimitForward
			);
		else {
			const CMotionVector* pmv8 = pmvForward;
			for (iBlk = 0; iBlk < 4; iBlk++)	{
				pmv8++;
				if (pmbmd->m_rgTranspStatus [iBlk + 1] != ALL)
					motionComp (
						m_ppxlcPredMBY + rgiBlkOffsetPixel [iBlk], 
						m_pvopcRefQ0->pixelsY (),
						BLOCK_SIZE, 
						(x + rgiBlkOffsetX [iBlk]) * 2 + pmv8->trueMVHalfPel ().x, 
						(y + rgiBlkOffsetY [iBlk]) * 2 + pmv8->trueMVHalfPel ().y,
						m_vopmd.iRoundingControl,
						prctMVLimitForward
					);
			}
		}
		CoordI xRefUVForward, yRefUVForward;
		mvLookupUVWithShape (pmbmd, pmvForward, xRefUVForward, yRefUVForward);
		motionCompUV (m_ppxlcPredMBU, m_ppxlcPredMBV, m_pvopcRefQ0, x, y, xRefUVForward, yRefUVForward, 0, prctMVLimitForward);

		if (pmbmd->m_bhas4MVBackward != TRUE)		
			motionComp (
				m_ppxlcPredMBBackY,
				m_pvopcRefQ1->pixelsY (),
				MB_SIZE, // either MB or BLOCK size
				x * 2 + pmvBackward->trueMVHalfPel ().x, 
				y * 2 + pmvBackward->trueMVHalfPel ().y,
				m_vopmd.iRoundingControl,
				prctMVLimitBackward
			);
		else {
			const CMotionVector* pmv8 = pmvBackward;
			for (iBlk = 0; iBlk < 4; iBlk++)	{
				pmv8++;
				if (pmbmd->m_rgTranspStatus [iBlk + 1] != ALL)
					motionComp (
						m_ppxlcPredMBBackY + rgiBlkOffsetPixel [iBlk], 
						m_pvopcRefQ1->pixelsY (),
						BLOCK_SIZE, 
						(x + rgiBlkOffsetX [iBlk]) * 2 + pmv8->trueMVHalfPel ().x, 
						(y + rgiBlkOffsetY [iBlk]) * 2 + pmv8->trueMVHalfPel ().y,
						m_vopmd.iRoundingControl,
						prctMVLimitBackward
					);
			}
		}
		CoordI xRefUVBackward, yRefUVBackward;
		mvLookupUVWithShape (pmbmd, pmvBackward, xRefUVBackward, yRefUVBackward);
		motionCompUV (m_ppxlcPredMBBackU, m_ppxlcPredMBBackV, m_pvopcRefQ1, x, y, xRefUVBackward, yRefUVBackward, 0, prctMVLimitBackward);
		averagePredAndAssignToCurrQ (ppxlcCurrQMBY, ppxlcCurrQMBU, ppxlcCurrQMBV);
	}
	else { 
		const CVOPU8YUVBA* pvopcRef; // point to left-top of the reference VOP
		CRct *prctMVLimit;
		if (pmbmd->m_mbType == FORWARD){
			pvopcRef = m_pvopcRefQ0;
			prctMVLimit = prctMVLimitForward;
		}
		else {
			pvopcRef = m_pvopcRefQ1;
			prctMVLimit = prctMVLimitBackward;
		}
		copyFromRefToCurrQ (pvopcRef, x, y, ppxlcCurrQMBY, ppxlcCurrQMBU, ppxlcCurrQMBV, prctMVLimit);
	}
}


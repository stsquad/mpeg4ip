/**************************************************************************

No header comments ????
 
Revision History:
	Dec 11, 1997:	X. Chen and B. Eifrig
	Interlaced tools added by NextLevel Systems

**************************************************************************/
#include <stdio.h>
#include <fstream.h>
#include <math.h>
#include <stdlib.h>

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

UInt CVideoObjectEncoder::encodeMV (const CMotionVector* pmv, const CMBMode* pmbmd,
									Bool bLeftBndry, Bool bRightBndry, Bool bTopBndry, Int iMBX, Int iMBY)
{
	UInt nBits = 0;
	CVector vctDiff;
	CVector vctPred;
	if (pmbmd->m_bhas4MVForward) {
		if (bLeftBndry || bRightBndry || bTopBndry)	{
			for (UInt iBlk = Y_BLOCK1; iBlk <= Y_BLOCK4; iBlk++)	{
				findMVpredGeneric (vctPred, pmv, pmbmd, iBlk, iMBX, iMBY);
				vctDiff = (pmv + iBlk)->trueMVHalfPel () - vctPred;
#ifdef __TRACE_AND_STATS_
				m_pbitstrmOut->trace ((pmv + iBlk)->trueMVHalfPel (), "MB_MV_Curr");
				m_pbitstrmOut->trace (vctPred, "MB_MV_Pred");
				m_pbitstrmOut->trace (vctDiff, "MB_MV_Diff");
#endif // __TRACE_AND_STATS_
				nBits += sendDiffMV (vctDiff, &m_vopmd.mvInfoForward);
			}
		}
		else {
			for (UInt iBlk = Y_BLOCK1; iBlk <= Y_BLOCK4; iBlk++)	{
				find8x8MVpredInterior (vctPred, pmv, (BlockNum) iBlk);
				vctDiff = (pmv + iBlk) -> trueMVHalfPel () - vctPred;
#ifdef __TRACE_AND_STATS_
				m_pbitstrmOut->trace ((pmv + iBlk)->trueMVHalfPel (), "MB_MV_Curr");
				m_pbitstrmOut->trace (vctPred, "MB_MV_Pred");
				m_pbitstrmOut->trace (vctDiff, "MB_MV_Diff");
#endif // __TRACE_AND_STATS_
				nBits += sendDiffMV (vctDiff, &m_vopmd.mvInfoForward);
			}
		}
	} else if (pmbmd->m_bFieldMV) {
			find16x16MVpred(vctPred, pmv, bLeftBndry, bRightBndry, bTopBndry);
			if(pmbmd->m_bForwardTop) {
				vctDiff.x = pmv[6].m_vctTrueHalfPel.x   - vctPred.x;
				vctDiff.y = pmv[6].m_vctTrueHalfPel.y/2 - vctPred.y/2;
			} 
			else
			{
				vctDiff.x = pmv[5].m_vctTrueHalfPel.x   - vctPred.x;
				vctDiff.y = pmv[5].m_vctTrueHalfPel.y/2 - vctPred.y/2;
			}
			nBits += sendDiffMV (vctDiff, &m_vopmd.mvInfoForward);
			if(pmbmd->m_bForwardBottom) {
				vctDiff.x = pmv[8].m_vctTrueHalfPel.x   - vctPred.x;
				vctDiff.y = pmv[8].m_vctTrueHalfPel.y/2 - vctPred.y/2;
			}
			else
			{
				vctDiff.x = pmv[7].m_vctTrueHalfPel.x   - vctPred.x;
				vctDiff.y = pmv[7].m_vctTrueHalfPel.y/2 - vctPred.y/2;
			}
			nBits += sendDiffMV (vctDiff, &m_vopmd.mvInfoForward);
	} else {		//1mv
		findMVpredGeneric (vctPred, pmv, pmbmd, ALL_Y_BLOCKS, iMBX, iMBY);
		vctDiff = pmv->trueMVHalfPel () - vctPred;
#ifdef __TRACE_AND_STATS_
		m_pbitstrmOut->trace (pmv->trueMVHalfPel (), "MB_MV_Curr");
		m_pbitstrmOut->trace (vctPred, "MB_MV_Pred");
		m_pbitstrmOut->trace (vctDiff, "MB_MV_Diff");
#endif // __TRACE_AND_STATS_
		nBits += sendDiffMV (vctDiff, &m_vopmd.mvInfoForward);
	}
	return nBits;
}


UInt CVideoObjectEncoder::encodeMVWithShape (const CMotionVector* pmv, const CMBMode* pmbmd, Int iXMB, Int iYMB)
{
	UInt nBits = 0;
	CVector vctDiff;
	CVector vctPred;

	if (pmbmd->m_bhas4MVForward)	{	//4mv
		for (UInt iBlk = Y_BLOCK1; iBlk <= Y_BLOCK4; iBlk++)	{
			if (pmbmd->m_rgTranspStatus[iBlk] != ALL)	{
				findMVpredGeneric (vctPred, pmv, pmbmd, iBlk, iXMB, iYMB);
				vctDiff = (pmv + iBlk)->trueMVHalfPel () - vctPred;
				nBits += sendDiffMV (vctDiff, &m_vopmd.mvInfoForward);
			}
		}
	}
// INTERLACED
		// new changes
	else if (pmbmd->m_bFieldMV) {
			findMVpredGeneric (vctPred, pmv, pmbmd, ALL_Y_BLOCKS, iXMB, iYMB);
			if(pmbmd->m_bForwardTop) {
				vctDiff.x = pmv[6].m_vctTrueHalfPel.x   - vctPred.x;
				vctDiff.y = pmv[6].m_vctTrueHalfPel.y/2 - vctPred.y/2;
			} 
			else
			{
				vctDiff.x = pmv[5].m_vctTrueHalfPel.x   - vctPred.x;
				vctDiff.y = pmv[5].m_vctTrueHalfPel.y/2 - vctPred.y/2;
			}
			nBits += sendDiffMV (vctDiff, &m_vopmd.mvInfoForward);
			if(pmbmd->m_bForwardBottom) {
				vctDiff.x = pmv[8].m_vctTrueHalfPel.x   - vctPred.x;
				vctDiff.y = pmv[8].m_vctTrueHalfPel.y/2 - vctPred.y/2;
			}
			else
			{
				vctDiff.x = pmv[7].m_vctTrueHalfPel.x   - vctPred.x;
				vctDiff.y = pmv[7].m_vctTrueHalfPel.y/2 - vctPred.y/2;
			}
			nBits += sendDiffMV (vctDiff, &m_vopmd.mvInfoForward);
	}
			// end of new changes
// INTERLACED
	else {		//1mv
		findMVpredGeneric (vctPred, pmv, pmbmd, ALL_Y_BLOCKS, iXMB, iYMB);
		vctDiff = pmv->trueMVHalfPel () - vctPred;
		nBits += sendDiffMV (vctDiff, &m_vopmd.mvInfoForward);
	}
	return nBits;
}

Void CVideoObjectEncoder::scaleMV (Int& iVLC, UInt& uiResidual, Int iDiffMVcomponent, const MVInfo *pmviDir)
{
	if (iDiffMVcomponent < (-1 * (Int) pmviDir->uiRange))
		iDiffMVcomponent += 2 * pmviDir->uiRange;
	else if (iDiffMVcomponent > (Int) (pmviDir->uiRange - 1))
		iDiffMVcomponent -= 2 * pmviDir->uiRange;

	if (iDiffMVcomponent == 0)	{									//nothing to do
		iVLC = 0;
		uiResidual = 0;
	}
	else	{
		if (pmviDir->uiScaleFactor == 1)	{									//simple no-scale case
			iVLC = iDiffMVcomponent;
			uiResidual = 0;
		}
		else {														//stupid scaling
			UInt uiAbsDiffMVcomponent = abs (iDiffMVcomponent);
			uiResidual = (uiAbsDiffMVcomponent - 1) % pmviDir->uiScaleFactor;																	
			UInt uiVLCmagnitude = (uiAbsDiffMVcomponent 
								- uiResidual + (pmviDir->uiScaleFactor - 1)) 
								/ pmviDir->uiScaleFactor;	// absorb ++ into here (m_volmd.mvInfo.uiScaleFactor - 1)
			iVLC = uiVLCmagnitude * sign (iDiffMVcomponent);
		}
	}
}

UInt CVideoObjectEncoder::sendDiffMV (const CVector& vctDiffMVHalfPel, const MVInfo *pmviDir)
{
	Int iVLC;
	UInt uiResidual;
	UInt nBits = 0;

	assert(vctDiffMVHalfPel.x < (Int)(pmviDir->uiRange<<1) && vctDiffMVHalfPel.x > -(Int)(pmviDir->uiRange<<1));
	assert(vctDiffMVHalfPel.y < (Int)(pmviDir->uiRange<<1) && vctDiffMVHalfPel.y > -(Int)(pmviDir->uiRange<<1));

	// send the bits
	scaleMV (iVLC, uiResidual, vctDiffMVHalfPel.x, pmviDir);

	Long lsymbol = iVLC + 32;
	nBits += m_pentrencSet -> m_pentrencMV->encodeSymbol (lsymbol, "MB_MV_Magnitude");
	if (iVLC != 0) {
		m_pbitstrmOut->putBits ((Char) uiResidual, pmviDir->uiFCode - 1, "MB_MV_Residual");
		nBits += pmviDir->uiFCode - 1;
	}
	else
		assert (uiResidual == 0);

	scaleMV (iVLC, uiResidual, vctDiffMVHalfPel.y, pmviDir);
	lsymbol = iVLC + 32;
	nBits += m_pentrencSet -> m_pentrencMV->encodeSymbol (lsymbol, "MB_MV_Magnitude");
	if (iVLC != 0) {
		m_pbitstrmOut->putBits ((Char) uiResidual, pmviDir->uiFCode - 1, "MB_MV_Residual");
		nBits += pmviDir->uiFCode - 1;
	}
	else
		assert (uiResidual == 0);
	return nBits;
}


UInt CVideoObjectEncoder::encodeMVofBVOP (
	  const CMotionVector* pmvForward, 
	  const CMotionVector* pmvBackward, 
	  const CMBMode*	   pmbmd, 
	  Int	iMBX,	Int iMBY,
	  const CMotionVector* pmvRef, // not used
	  const CMBMode*	   pmbmdRef // not used
)  // encode motion vectors for b-vop
{
	assert (pmbmd->m_rgTranspStatus [0] != ALL);
	assert (pmbmd->m_dctMd == INTER || pmbmd->m_dctMd == INTERQ);
	assert (pmbmd->m_bSkip == FALSE);

	CVector vctDiff;
	UInt nBits = 0;
	const CVector *pvctMV;
	if (pmbmd->m_mbType == FORWARD || pmbmd->m_mbType == INTERPOLATE)	{
		assert (pmbmd->m_bhas4MVForward != TRUE);
		// TPS FIX
		if (pmbmd->m_bFieldMV && m_volmd.volType != ENHN_LAYER) {
			pvctMV = &pmvForward[1 + (Int)pmbmd->m_bForwardTop].m_vctTrueHalfPel;
			assert((pvctMV->y & 1) == 0);
			vctDiff.x = pvctMV->x   - m_vctForwardMvPredBVOP[0].x;
			vctDiff.y = pvctMV->y/2 - m_vctForwardMvPredBVOP[0].y/2;
#ifdef __TRACE_AND_STATS_
			m_pbitstrmOut->trace (*pvctMV,					"MB_F_MV_Curr_Top");
			m_pbitstrmOut->trace (m_vctForwardMvPredBVOP[0],"MB_F_MV_Pred_Top");
			m_pbitstrmOut->trace (vctDiff,					"MB_F_MV_Diff_Top");
#endif // __TRACE_AND_STATS_
			m_vctForwardMvPredBVOP[0] = *pvctMV;
			nBits += sendDiffMV (vctDiff, &m_vopmd.mvInfoForward);

			pvctMV = &pmvForward[3 + (Int)pmbmd->m_bForwardBottom].m_vctTrueHalfPel;
			assert((pvctMV->y & 1) == 0);
			vctDiff.x = pvctMV->x   - m_vctForwardMvPredBVOP[1].x;
			vctDiff.y = pvctMV->y/2 - m_vctForwardMvPredBVOP[1].y/2;
#ifdef __TRACE_AND_STATS_
			m_pbitstrmOut->trace (*pvctMV,					"MB_F_MV_Curr_Bot");
			m_pbitstrmOut->trace (m_vctForwardMvPredBVOP[1],"MB_F_MV_Pred_Bot");
			m_pbitstrmOut->trace (vctDiff,					"MB_F_MV_Diff_Bot");
#endif // __TRACE_AND_STATS_
			m_vctForwardMvPredBVOP[1] = *pvctMV;
			nBits += sendDiffMV (vctDiff, &m_vopmd.mvInfoForward);
		} else {
			vctDiff = pmvForward[0].m_vctTrueHalfPel - m_vctForwardMvPredBVOP[0];
#ifdef __TRACE_AND_STATS_
			m_pbitstrmOut->trace (pmvForward[0].m_vctTrueHalfPel, "MB_F_MV_Curr");
			m_pbitstrmOut->trace (m_vctForwardMvPredBVOP[0],	  "MB_F_MV_Pred");
			m_pbitstrmOut->trace (vctDiff,						  "MB_F_MV_Diff");
#endif // __TRACE_AND_STATS_
			m_vctForwardMvPredBVOP[0] = pmvForward[0].m_vctTrueHalfPel;
			m_vctForwardMvPredBVOP[1] = pmvForward[0].m_vctTrueHalfPel;
			nBits += sendDiffMV (vctDiff, &m_vopmd.mvInfoForward);
		}
	}
	
	// TPS FIX
	if ((pmbmd->m_mbType == BACKWARD || pmbmd->m_mbType == INTERPOLATE) 
		&& (m_volmd.volType != ENHN_LAYER || m_vopmd.iRefSelectCode != 0))	{	// modified by Sharp (98/3/11)
		assert (pmbmd->m_bhas4MVBackward != TRUE);
		// TPS FIX
		if (pmbmd->m_bFieldMV && m_volmd.volType != ENHN_LAYER) {
			pvctMV = &pmvBackward[1 + (Int)pmbmd->m_bBackwardTop].m_vctTrueHalfPel;
			assert((pvctMV->y & 1) == 0);
			vctDiff.x = pvctMV->x   - m_vctBackwardMvPredBVOP[0].x;
			vctDiff.y = pvctMV->y/2 - m_vctBackwardMvPredBVOP[0].y/2;
#ifdef __TRACE_AND_STATS_
			m_pbitstrmOut->trace (*pvctMV,					  "MB_B_MV_Curr_Top");
			m_pbitstrmOut->trace (m_vctBackwardMvPredBVOP[0], "MB_B_MV_Pred_Top");
			m_pbitstrmOut->trace (vctDiff,					  "MB_B_MV_Diff_Top");
#endif // __TRACE_AND_STATS_
			m_vctBackwardMvPredBVOP[0] = *pvctMV;
			nBits += sendDiffMV (vctDiff, &m_vopmd.mvInfoBackward);

			pvctMV = &pmvBackward[3 + (Int)pmbmd->m_bBackwardBottom].m_vctTrueHalfPel;
			assert((pvctMV->y & 1) == 0);
			vctDiff.x = pvctMV->x   - m_vctBackwardMvPredBVOP[1].x;
			vctDiff.y = pvctMV->y/2 - m_vctBackwardMvPredBVOP[1].y/2;
#ifdef __TRACE_AND_STATS_
			m_pbitstrmOut->trace (*pvctMV,					  "MB_B_MV_Curr_Bot");
			m_pbitstrmOut->trace (m_vctBackwardMvPredBVOP[1], "MB_B_MV_Pred_Bot");
			m_pbitstrmOut->trace (vctDiff,					  "MB_B_MV_Diff_Bot");
#endif // __TRACE_AND_STATS_
			m_vctBackwardMvPredBVOP[1] = *pvctMV;
			nBits += sendDiffMV (vctDiff, &m_vopmd.mvInfoBackward);
		} else {
			vctDiff = pmvBackward[0].m_vctTrueHalfPel - m_vctBackwardMvPredBVOP[0];
#ifdef __TRACE_AND_STATS_
			m_pbitstrmOut->trace (pmvBackward[0].m_vctTrueHalfPel, "MB_B_MV_Curr");
			m_pbitstrmOut->trace (m_vctBackwardMvPredBVOP[0],	   "MB_B_MV_Pred");
			m_pbitstrmOut->trace (vctDiff,						   "MB_B_MV_Diff");
#endif // __TRACE_AND_STATS_
			m_vctBackwardMvPredBVOP[0] = pmvBackward[0].m_vctTrueHalfPel;
			m_vctBackwardMvPredBVOP[1] = pmvBackward[0].m_vctTrueHalfPel;
			nBits += sendDiffMV (vctDiff, &m_vopmd.mvInfoBackward);
		}
	}

	if (pmbmd->m_mbType == DIRECT) {
		static MVInfo mviDirect = { 32, 1, 1 };

		assert (pmbmd->m_bhas4MVForward != TRUE);
#ifdef __TRACE_AND_STATS_
		m_pbitstrmOut->trace (pmbmd->m_vctDirectDeltaMV, "MB_D_MV_Diff");
#endif // __TRACE_AND_STATS_
		nBits += sendDiffMV (pmbmd->m_vctDirectDeltaMV, &mviDirect);
	}
	return nBits;
}


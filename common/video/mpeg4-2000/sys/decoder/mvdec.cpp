/*************************************************************************

This software module was originally developed by 

	Wei-ge Chen (wchen@microsoft.com), Microsoft Corporation
	Ming-Chieh Lee (mingcl@microsoft.com), Microsoft Corporation
	(date: July, 1997)

and edited by Xuemin Chen (xchen@gi.com), General Instrument Corp.
and also edited by
    Mathias Wien (wien@ient.rwth-aachen.de) RWTH Aachen / Robert BOSCH GmbH

and also edited by
    Fujitsu Laboratories Ltd. (contact: Eishi Morimatsu)
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
Microsoft retains full right to use the code for his/her own purpose, 
assign or donate the code to a third party and to inhibit third parties from using the code for non <MPEG standard> conforming products. 
This copyright notice must be included in all copies or derivative works. 

Copyright (c) 1996, 1997.

Module Name:

	mvDec.cpp

Abstract:

	motion vector decoder

Revision History:
	Dec 20, 1997:	Interlaced tools added by General Instrument Corp.
    Feb.16, 1999:   add Quarter Sample 
                    Mathias Wien (wien@ient.rwth-aachen.de) 
	Sep.06	1999 : RRV added by Eishi Morimatsu (Fujitsu Laboratories Ltd.) 

*************************************************************************/

#include "typeapi.h"
#include "codehead.h"
#include "mode.hpp"
#include "global.hpp"
#include "entropy/bitstrm.hpp"
#include "entropy/entropy.hpp"
#include "entropy/huffman.hpp"
#include "vopses.hpp"
#include "vopsedec.hpp"

#ifdef __MFC_
#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

#define new DEBUG_NEW				   
#endif // __MFC_
#define ASSERT(a) if (!(a)) { printf("iso mvdec throw %d\n", __LINE__);throw((int)__LINE__);}


Void CVideoObjectDecoder::decodeMV (const CMBMode* pmbmd, CMotionVector* pmv, 
									Bool bLeftBndry, Bool bRightBndry, Bool bTopBndry, 
									Bool bZeroMV, Int iMBX, Int iMBY)
{
	if (pmbmd->m_bSkip || pmbmd->m_dctMd == INTRA || pmbmd->m_dctMd == INTRAQ || bZeroMV)	{
		memset (pmv, 0, PVOP_MV_PER_REF_PER_MB * sizeof (CMotionVector));
		return;
	}
// RRV insertion
	CMotionVector *pmv_copy = pmv;
// ~RRV
	CVector vctDiff, vctDecode, vctPred;
	if (pmbmd->m_bhas4MVForward)	{
		for (Int iBlk = Y_BLOCK1; iBlk <= Y_BLOCK4; iBlk++)	{
			if (bLeftBndry || bRightBndry || bTopBndry)
				if(short_video_header)
          find8x8MVpredAtBoundary(vctPred, pmv, bLeftBndry, bRightBndry, bTopBndry, iBlk);
				else
					findMVpredGeneric (vctPred, pmv, pmbmd, iBlk, iMBX, iMBY);
			else 
				find8x8MVpredInterior (vctPred, pmv, iBlk);
			getDiffMV (vctDiff, m_vopmd.mvInfoForward);
			vctDecode = vctDiff + vctPred;																//fit in range
			fitMvInRange (vctDecode, m_vopmd.mvInfoForward);
			pmv [iBlk] = CMotionVector (vctDecode);		//convert to full pel now
		}
	}
// INTERLACE
	else if ((m_vopmd.bInterlace)&&(pmbmd->m_bFieldMV))	{
		Int iTempX1, iTempY1, iTempX2, iTempY2;
		find16x16MVpred (vctPred, pmv, bLeftBndry, bRightBndry, bTopBndry);
		getDiffMV (vctDiff, m_vopmd.mvInfoForward);
		vctDiff.y *= 2;
		vctPred.y = 2*(vctPred.y / 2);
		vctDecode = vctDiff + vctPred;																//fit in range
		fitMvInRange (vctDecode, m_vopmd.mvInfoForward);
		CMotionVector* pmv16x8 = pmv +5;
		if(pmbmd->m_bForwardTop) {
			pmv16x8++;
			*pmv16x8 = CMotionVector (vctDecode);		//convert to full pel now
			iTempX1 = pmv16x8->m_vctTrueHalfPel.x;
			iTempY1 = pmv16x8->m_vctTrueHalfPel.y;
			pmv16x8++;
		} 
		else
		{
			*pmv16x8 = CMotionVector (vctDecode);		//convert to full pel now
			iTempX1 = pmv16x8->m_vctTrueHalfPel.x;
			iTempY1 = pmv16x8->m_vctTrueHalfPel.y;
			pmv16x8++;
			pmv16x8++;
		}
		getDiffMV (vctDiff, m_vopmd.mvInfoForward);
		vctDiff.y *= 2;
		vctPred.y = 2*(vctPred.y / 2);
		vctDecode = vctDiff + vctPred;																//fit in range
		fitMvInRange (vctDecode, m_vopmd.mvInfoForward);
		if(pmbmd->m_bForwardBottom) {
			pmv16x8++;
			*pmv16x8 = CMotionVector (vctDecode);		//convert to full pel now
			iTempX2 = pmv16x8->m_vctTrueHalfPel.x;
			iTempY2 = pmv16x8->m_vctTrueHalfPel.y;
		}
		else
		{
			*pmv16x8 = CMotionVector (vctDecode);		//convert to full pel now
			iTempX2 = pmv16x8->m_vctTrueHalfPel.x;
			iTempY2 = pmv16x8->m_vctTrueHalfPel.y;
		}

		Int iTemp;
		for (UInt i = 1; i < 5; i++) {
			iTemp = iTempX1 + iTempX2;
			pmv [i].m_vctTrueHalfPel.x = (iTemp & 3) ? ((iTemp>>1) | 1) : (iTemp>>1);
			iTemp = iTempY1 + iTempY2;
			pmv [i].m_vctTrueHalfPel.y = (iTemp & 3) ? ((iTemp>>1) | 1) : (iTemp>>1);
			pmv [i] = pmv [i].m_vctTrueHalfPel;        //convert to full pel now //added mwi
            pmv[i].computeMV (); 
		}
	}
// ~INTERLACE
	else {			//1 mv
		if(short_video_header)
			find16x16MVpred(vctPred, pmv, bLeftBndry, bRightBndry, bTopBndry);
		else
			findMVpredGeneric (vctPred, pmv, pmbmd, ALL_Y_BLOCKS, iMBX, iMBY);
		getDiffMV (vctDiff, m_vopmd.mvInfoForward);
		vctDecode = vctDiff + vctPred;																//fit in range
		fitMvInRange (vctDecode, m_vopmd.mvInfoForward);
		*pmv++ = CMotionVector (vctDecode);			//convert to full pel now
		for (Int i = 0; i < 4; i++) {
			*pmv = *(pmv - 1);
			pmv++;
		}
	}
// RRV insertion
	if(m_vopmd.RRVmode.iRRVOnOff == 1){
		for(int i = 0; i < PVOP_MV_PER_REF_PER_MB; i ++)
		{
			pmv_copy[i].scaleup();
		}
	}
// ~RRV
}

Void CVideoObjectDecoder::decodeMVofBVOP (CMotionVector* pmvForward, CMotionVector* pmvBackward, CMBMode* pmbmd, 
										  Int iMBX, Int iMBY, const CMotionVector* pmvRef, const CMBMode* pmbmdRef)
{
	CVector vctDiff;
	//	CVector vctPred;
	CVector vctDecode;
	//	UInt nBits = 0;
	if(pmbmd->m_bSkip == TRUE && m_volmd.volType == ENHN_LAYER && m_vopmd.iRefSelectCode == 0)
		return;
	if (pmbmd->m_mbType == FORWARD || pmbmd->m_mbType == INTERPOLATE)	{
		assert (pmbmd->m_bSkip != TRUE);
		assert (pmbmd->m_bhas4MVForward != TRUE);
		getDiffMV (vctDiff, m_vopmd.mvInfoForward);
		// TPS FIX
		if (pmbmd->m_bFieldMV && m_volmd.volType != ENHN_LAYER) {
			vctDecode.x = vctDiff.x + m_vctForwardMvPredBVOP[0].x;
			vctDecode.y = (vctDiff.y + m_vctForwardMvPredBVOP[0].y / 2);
			fitMvInRange (vctDecode, m_vopmd.mvInfoForward);
            vctDecode.y *= 2;
			m_vctForwardMvPredBVOP[0] = vctDecode;
			pmvForward[0] = CMotionVector(vctDecode);
			pmvForward[1] = pmvForward[0];
			pmvForward[2] = pmvForward[0];
			getDiffMV(vctDiff, m_vopmd.mvInfoForward);
			vctDecode.x = vctDiff.x + m_vctForwardMvPredBVOP[1].x;
			vctDecode.y = (vctDiff.y + m_vctForwardMvPredBVOP[1].y / 2);
			fitMvInRange (vctDecode, m_vopmd.mvInfoForward);
            vctDecode.y *= 2;
			m_vctForwardMvPredBVOP[1] = vctDecode;
			pmvForward[3] = CMotionVector(vctDecode);
			pmvForward[4] = pmvForward[3];
		} else {
			vctDecode = vctDiff + m_vctForwardMvPredBVOP[0];
			fitMvInRange (vctDecode, m_vopmd.mvInfoForward);
			m_vctForwardMvPredBVOP[0] = vctDecode;
			m_vctForwardMvPredBVOP[1] = vctDecode;
			*pmvForward++ = CMotionVector(vctDecode);
			for (Int i = 0; i < 4; i++) {
				*pmvForward = *(pmvForward - 1);
				pmvForward++;
			}
		}
	}
	// TPS FIX
	if ( (pmbmd->m_mbType == BACKWARD || pmbmd->m_mbType == INTERPOLATE)	
		&& (m_volmd.volType != ENHN_LAYER || m_vopmd.iRefSelectCode != 0) ){	// modified by Sharp (98/3/11)
		assert (pmbmd->m_bSkip != TRUE);
		assert (pmbmd->m_bhas4MVBackward != TRUE);
		getDiffMV (vctDiff, m_vopmd.mvInfoBackward);
		if (pmbmd->m_bFieldMV && m_volmd.volType != ENHN_LAYER) {
			vctDecode.x = vctDiff.x + m_vctBackwardMvPredBVOP[0].x;
			vctDecode.y = (vctDiff.y + m_vctBackwardMvPredBVOP[0].y / 2);
			fitMvInRange (vctDecode, m_vopmd.mvInfoBackward);
            vctDecode.y *= 2;
			m_vctBackwardMvPredBVOP[0] = vctDecode;
			pmvBackward[0] = CMotionVector(vctDecode);
			pmvBackward[1] = pmvBackward[0];
			pmvBackward[2] = pmvBackward[0];
			getDiffMV(vctDiff, m_vopmd.mvInfoBackward);
			vctDecode.x = vctDiff.x + m_vctBackwardMvPredBVOP[1].x;
			vctDecode.y = (vctDiff.y + m_vctBackwardMvPredBVOP[1].y / 2);
			fitMvInRange (vctDecode, m_vopmd.mvInfoBackward);
            vctDecode.y *= 2;
			m_vctBackwardMvPredBVOP[1] = vctDecode;
			pmvBackward[3] = CMotionVector(vctDecode);
			pmvBackward[4] = pmvBackward[3];
		} else {
			vctDecode = vctDiff + m_vctBackwardMvPredBVOP[0];
			fitMvInRange (vctDecode, m_vopmd.mvInfoBackward);
			m_vctBackwardMvPredBVOP[0] = vctDecode;
			m_vctBackwardMvPredBVOP[1] = vctDecode;
			*pmvBackward++ = CMotionVector(vctDecode);
			for (Int i = 0; i < 4; i++) {
				*pmvBackward = *(pmvBackward - 1);
				pmvBackward++;
			}
		}
	}
	if (pmbmd->m_mbType == DIRECT)	{	
      static MVInfo directInfo = { 32, 1, 1}; 
      assert (pmbmd->m_bhas4MVForward != TRUE);
      /* begin of new changes 02-16-99 
         if (m_vopmd.bInterlace) {
         memset (pmvForward, 0, BVOP_MV_PER_REF_PER_MB * sizeof (CMotionVector));	//set forward mv to zero
         if (!pmbmd->m_bSkip)
         getDiffMV (pmbmd->m_vctDirectDeltaMV, directInfo);
         } else {
         end of new changes 02-16-99	*/
      
      // begin of new changes 02-17-99
      if (m_vopmd.bInterlace) {
        if (!pmbmd->m_bSkip) {
          if (!m_volmd.bQuarterSample) {// Quarterpel, added by mwi          
            getDiffMV (pmbmd->m_vctDirectDeltaMV, directInfo);
          }
          else {
            Long lSymbol = m_pentrdecSet->m_pentrdecMV->decodeSymbol ();
            pmbmd->m_vctDirectDeltaMV.x  = deScaleMV (lSymbol - 32, 0, 1);
            lSymbol = m_pentrdecSet->m_pentrdecMV->decodeSymbol ();
            pmbmd->m_vctDirectDeltaMV.y  = deScaleMV (lSymbol - 32, 0, 1);
          }
          vctDiff.x = pmbmd->m_vctDirectDeltaMV.x;
          vctDiff.y = pmbmd->m_vctDirectDeltaMV.y;
        }
        else
          vctDiff.x = vctDiff.y = 0;
      }
      else {  // this if-else is added by Krit 
        //end of new changes 02-17-99
        
        if (pmbmd->m_bSkip)
          vctDiff.x = vctDiff.y = 0;
        else  {
          Long lSymbol = m_pentrdecSet->m_pentrdecMV->decodeSymbol ();
          vctDiff.x  = deScaleMV (lSymbol - 32, 0, 1);
          lSymbol = m_pentrdecSet->m_pentrdecMV->decodeSymbol ();
          vctDiff.y  = deScaleMV (lSymbol - 32, 0, 1);
        }
      }  // new change 02-17-99

      
      computeDirectForwardMV (vctDiff, pmvForward, pmvRef, pmbmdRef);	//compute forward mv from diff
      if(pmbmdRef==NULL)
        {
          // transparent reference macroblock
          if (!m_volmd.bQuarterSample) // added by mwi                    
            pmbmd->m_bhas4MVBackward = pmbmd->m_bhas4MVForward = FALSE;
          else
            pmbmd->m_bhas4MVBackward = pmbmd->m_bhas4MVForward = TRUE;
          CMotionVector mvRef; // zero motion vector
          backwardMVFromForwardMV (*pmvBackward, *pmvForward, mvRef, vctDiff);
          for (Int i = 0; i < 4; i++) {
            pmvForward++;
            pmvBackward++;
            backwardMVFromForwardMV( *pmvBackward, *pmvForward, mvRef, vctDiff);				  
          }
        }
      else
        {
          pmbmd->m_bhas4MVBackward = pmbmd->m_bhas4MVForward = pmbmdRef->m_bhas4MVForward;			//reset 4mv mode
          //if (pmbmd->m_bhas4MVBackward)	{ //  for Quarter Sample Direct Mode, changed by mwi 990614
          if (pmbmd->m_bhas4MVBackward || pmbmd->m_mbType == DIRECT)	{
            for (Int i = 0; i < 4; i++) {
              pmvForward++;
              pmvBackward++;
              pmvRef++;
              backwardMVFromForwardMV (*pmvBackward, *pmvForward, *pmvRef,
                                       vctDiff); //compute back mv from forward mv and ref mv for direct mode
            }
          }
          else
            backwardMVFromForwardMV (*pmvBackward, *pmvForward, *pmvRef,
                                     vctDiff); //compute back mv from forward mv and ref mv for direct mode
        } // 	}  new change 02-19-99
    }
    
}

Void CVideoObjectDecoder::computeDirectForwardMV (CVector vctDiff, CMotionVector* pmv,
												  const CMotionVector* pmvRef,
												  const CMBMode* pmbmdRef)
{
	if(pmvRef==NULL)
	{
		// colocated macroblock is transparent, use zero reference MV
		*pmv++ = CMotionVector (vctDiff); //convert to full pel now
		for (Int i = 0; i < 4; i++) {
			*pmv = *(pmv - 1);
			pmv++;
		}
	}
	else
	{
		Int iPartInterval = m_t - m_tPastRef;
		Int iFullInterval = m_tFutureRef - m_tPastRef;
		if (pmbmdRef->m_bhas4MVForward == FALSE)	{
			CVector vctRefScaled = pmvRef->trueMVHalfPel () * iPartInterval;
 			ASSERT(iFullInterval);//yrchen to throw severe exception caused by packet loss 10.21.2003 
			vctRefScaled.x /= iFullInterval;			//truncation as per vm
			vctRefScaled.y /= iFullInterval;			//truncation as per vm
			CVector vctDecode = vctDiff + vctRefScaled;
			*pmv++ = CMotionVector (vctDecode);			//convert to full pel now
			for (Int i = 0; i < 4; i++) {
				*pmv = *(pmv - 1);
				pmv++;
			}
		}
		else {
			for (Int i = 0; i < 4; i++) {
				pmv++;
				pmvRef++;
				CVector vctRefScaled = pmvRef->trueMVHalfPel () * iPartInterval;
 				ASSERT(iFullInterval);//yrchen to throw severe exception caused by packet loss 10.21.2003
				vctRefScaled.x /= iFullInterval;			//truncation as per vm
				vctRefScaled.y /= iFullInterval;			//truncation as per vm
				CVector vctDecode = vctDiff + vctRefScaled;
				*pmv = CMotionVector (vctDecode);			//convert to full pel now
			}
		}
	}
}

Void CVideoObjectDecoder::decodeMVWithShape (const CMBMode* pmbmd, CoordI iMBX, CoordI iMBY, CMotionVector* pmv)
{
	if (pmbmd->m_bSkip || pmbmd->m_dctMd == INTRA || pmbmd->m_dctMd == INTRAQ || (m_volmd.volType == ENHN_LAYER && m_vopmd.iRefSelectCode == 3)) {		//OBSS_SAIT_991015
		memset (pmv, 0, PVOP_MV_PER_REF_PER_MB * sizeof (CMotionVector));
		return;
	}
	CVector vctDiff, vctDecode, vctPred;
	if (pmbmd->m_bhas4MVForward)	{
		for (Int iBlk = Y_BLOCK1; iBlk <= Y_BLOCK4; iBlk++)	{
			if (pmbmd->m_rgTranspStatus [iBlk] == ALL)	
				pmv [iBlk] = CMotionVector (NOT_MV, NOT_MV);		
			else	{
				findMVpredGeneric (vctPred, pmv, pmbmd, iBlk, iMBX, iMBY);
				getDiffMV (vctDiff, m_vopmd.mvInfoForward);
				vctDecode = vctDiff + vctPred;																//fit in range
				fitMvInRange (vctDecode, m_vopmd.mvInfoForward);
				pmv [iBlk] = CMotionVector (vctDecode);
			}
		}
	}
// INTERLACE
	//new changes
	else if ((m_vopmd.bInterlace)&&(pmbmd->m_bFieldMV))	{
		Int iTempX1, iTempY1, iTempX2, iTempY2;
		assert (pmbmd->m_rgTranspStatus [0] != ALL);
		findMVpredGeneric (vctPred, pmv, pmbmd, ALL_Y_BLOCKS, iMBX, iMBY);
		getDiffMV (vctDiff, m_vopmd.mvInfoForward);
		vctDiff.y *= 2;
		vctPred.y = 2*(vctPred.y / 2);
		vctDecode = vctDiff + vctPred;																//fit in range
		fitMvInRange (vctDecode, m_vopmd.mvInfoForward);
		CMotionVector* pmv16x8 = pmv +5;
		if(pmbmd->m_bForwardTop) {
			pmv16x8++;
			*pmv16x8 = CMotionVector (vctDecode);		//convert to full pel now
			iTempX1 = pmv16x8->m_vctTrueHalfPel.x;
			iTempY1 = pmv16x8->m_vctTrueHalfPel.y;
			pmv16x8++;
		} 
		else
		{
			*pmv16x8 = CMotionVector (vctDecode);		//convert to full pel now
			iTempX1 = pmv16x8->m_vctTrueHalfPel.x;
			iTempY1 = pmv16x8->m_vctTrueHalfPel.y;
			pmv16x8++;
			pmv16x8++;
		}
		getDiffMV (vctDiff, m_vopmd.mvInfoForward);
		vctDiff.y *= 2;
		vctPred.y = 2*(vctPred.y / 2);
		vctDecode = vctDiff + vctPred;																//fit in range
		fitMvInRange (vctDecode, m_vopmd.mvInfoForward);
		if(pmbmd->m_bForwardBottom) {
			pmv16x8++;
			*pmv16x8 = CMotionVector (vctDecode);		//convert to full pel now
			iTempX2 = pmv16x8->m_vctTrueHalfPel.x;
			iTempY2 = pmv16x8->m_vctTrueHalfPel.y;
		}
		else
		{
			*pmv16x8 = CMotionVector (vctDecode);		//convert to full pel now
			iTempX2 = pmv16x8->m_vctTrueHalfPel.x;
			iTempY2 = pmv16x8->m_vctTrueHalfPel.y;
		}

		Int iTemp;
		for (UInt i = 1; i < 5; i++) {
			iTemp = iTempX1 + iTempX2;
			pmv [i].m_vctTrueHalfPel.x = (iTemp & 3) ? ((iTemp>>1) | 1) : (iTemp>>1);
			iTemp = iTempY1 + iTempY2;
			pmv [i].m_vctTrueHalfPel.y = (iTemp & 3) ? ((iTemp>>1) | 1) : (iTemp>>1);
            pmv[i].computeMV (); 
		}
	}
	//end of new changes
// ~INTERLACE
	else {			//1 mv
		assert (pmbmd->m_rgTranspStatus [0] != ALL);
		findMVpredGeneric (vctPred, pmv, pmbmd, ALL_Y_BLOCKS, iMBX, iMBY);
		getDiffMV (vctDiff, m_vopmd.mvInfoForward);
		vctDecode = vctDiff + vctPred;																//fit in range
		fitMvInRange (vctDecode, m_vopmd.mvInfoForward);
		*pmv++ = CMotionVector (vctDecode);
		for (Int i = 0; i < 4; i++) {
			*pmv = *(pmv - 1);
			pmv++;
		}
	}
}

Void CVideoObjectDecoder::getDiffMV (CVector& vctDiffMV, MVInfo mvinfo)		//get half pel
{
	Int iResidual;
	Long lSymbol = m_pentrdecSet->m_pentrdecMV->decodeSymbol ();
	Int iVLC = lSymbol - 32;
	if (iVLC != 0)	
		iResidual = m_pbitstrmIn->getBits (mvinfo.uiFCode - 1);
	else
		iResidual = 0;
	vctDiffMV.x  = deScaleMV (iVLC, iResidual, mvinfo.uiScaleFactor);

	lSymbol = m_pentrdecSet->m_pentrdecMV->decodeSymbol ();
	iVLC = lSymbol - 32;
	if (iVLC != 0)
		iResidual = m_pbitstrmIn->getBits (mvinfo.uiFCode - 1);
	else
		iResidual = 0;
	vctDiffMV.y  = deScaleMV (iVLC, iResidual, mvinfo.uiScaleFactor);
}

Int CVideoObjectDecoder::deScaleMV (Int iVLC, Int iResidual, Int iScaleFactor)
{
	if (iVLC == 0 && iResidual == 0)
		return 0;
	else if (iScaleFactor == 1)
		return (iVLC);
	else {
		Int iAbsDiffMVcomponent = abs (iVLC) * iScaleFactor + iResidual - iScaleFactor + 1;	//changed a'c to enc
		return (sign (iVLC) * iAbsDiffMVcomponent);
	}
}

Void CVideoObjectDecoder::fitMvInRange (CVector& vctSrc, MVInfo mvinfo)
{
    Int iMvRange = mvinfo.uiRange;	//in half pel unit
	if (vctSrc.x < -1 * iMvRange)					//* 2 to get length of [-range, range]
		vctSrc.x += 2 * iMvRange;				
	else if (vctSrc.x >= iMvRange)
		vctSrc.x  -= 2 * iMvRange;

	if (vctSrc.y < -1 * iMvRange)
		vctSrc.y += 2 * iMvRange;
	else if (vctSrc.y >= iMvRange)
		vctSrc.y  -= 2 * iMvRange;
}


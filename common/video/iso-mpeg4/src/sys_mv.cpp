/**************************************************************************

This software module was originally developed by 

	Ming-Chieh Lee (mingcl@microsoft.com), Microsoft Corporation
	Wei-ge Chen (wchen@microsoft.com), Microsoft Corporation
	Simon Winder (swinder@microsoft.com), Microsoft Corporation
	(date: June, 1997)
and edited by
        Wei Wu (weiwu@stallion.risc.rockwell.com) Rockwell Science Center

and also edited by
	Yoshihiro Kikuchi (TOSHIBA CORPORATION)
	Takeshi Nagai (TOSHIBA CORPORATION)
	Toshiaki Watanabe (TOSHIBA CORPORATION)
	Noboru Yamaguchi (TOSHIBA CORPORATION)

and also edited by
	Yoshinori Suzuki (Hitachi, Ltd.)

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
 
Revision History:
	Nov. 14, 1997:	Added/Modified for error resilient mode by Toshiba

	December 20, 1997:	Interlaced tools added by NextLevel Systems (GI)
                        X. Chen (xchen@nlvl.com), B. Eifrig (beifrig@nlvl.com)
    Feb.16 1999      :  add Quarter Sample 
                        Mathias Wien (wien@ient.rwth-aachen.de) 
	Sep.06	1999 : RRV added by Eishi Morimatsu (Fujitsu Laboratories Ltd.) 

**************************************************************************/

#include <stdio.h>
#include "typeapi.h"
#include "global.hpp"
#include "codehead.h"
#include "mode.hpp"
#include "vopses.hpp"

#ifdef __MFC_
#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

#define new DEBUG_NEW				   
#endif // __MFC_

inline Int medianof3 (Int a0, Int a1, Int a2)
{
	if (a0 > a1) {
		if (a1 > a2)
			return a1;
		else if (a0 > a2)
			return a2;
		else
			return a0;
	}
	else if (a0 > a2)
		return a0;
	else if (a1 > a2)
		return a2;
	else
		return a1;
}

Void CVideoObject::find16x16MVpred (CVector& vecPred, const CMotionVector* pmv, Bool bLeftBndry, Bool bRightBndry, Bool bTopBndry) const
{
	CVector vctCandMV0, vctCandMV1, vctCandMV2;

	if (bLeftBndry)
		vctCandMV0.set (0, 0);
	else
		vctCandMV0 = (pmv - PVOP_MV_PER_REF_PER_MB + gIndexOfCandBlk [1] [0]) -> trueMVHalfPel ();

	if (bTopBndry) {
		vecPred = vctCandMV0;
		return;
	}
	else {
		vctCandMV1 = (pmv - m_iNumOfTotalMVPerRow + gIndexOfCandBlk [1] [1]) -> trueMVHalfPel ();
		if (bRightBndry)
			vctCandMV2.set (0, 0);
		else
			vctCandMV2 = (pmv - m_iNumOfTotalMVPerRow + PVOP_MV_PER_REF_PER_MB + gIndexOfCandBlk [1] [2]) -> trueMVHalfPel ();
	}
	vecPred.x = medianof3 (vctCandMV0.x, vctCandMV1.x, vctCandMV2.x);
	vecPred.y = medianof3 (vctCandMV0.y, vctCandMV1.y, vctCandMV2.y);
}

Void CVideoObject::find8x8MVpredAtBoundary (CVector& vecPred, const CMotionVector* pmv, Bool bLeftBndry, Bool bRightBndry, Bool bTopBndry, Int blknCurr) const
{
	CVector vctCandMV0, vctCandMV1, vctCandMV2;

	switch (blknCurr){
	case Y_BLOCK1:
		if (bLeftBndry)
			vctCandMV0.set (0, 0);
		else
			vctCandMV0 = (pmv - PVOP_MV_PER_REF_PER_MB + gIndexOfCandBlk [blknCurr] [0]) -> trueMVHalfPel ();
		if (bTopBndry){
			vecPred = vctCandMV0;
			return;
		}
		else{
			vctCandMV1 = (pmv - m_iNumOfTotalMVPerRow + gIndexOfCandBlk [blknCurr] [1])->trueMVHalfPel ();
			if (bRightBndry)
				vctCandMV2.set (0, 0);
			else
				vctCandMV2 = (pmv - m_iNumOfTotalMVPerRow + PVOP_MV_PER_REF_PER_MB + gIndexOfCandBlk [blknCurr] [2])->trueMVHalfPel ();
		}
		break;
	case Y_BLOCK2:
		vctCandMV0 = (pmv +  gIndexOfCandBlk [blknCurr] [0]) -> trueMVHalfPel ();
		if (bTopBndry) {
			vecPred = vctCandMV0;
			return;
		}
		else{
			vctCandMV1 = (pmv - m_iNumOfTotalMVPerRow + gIndexOfCandBlk [blknCurr] [1]) -> trueMVHalfPel ();
			if (bRightBndry)
				vctCandMV2.set (0, 0);
			else
				vctCandMV2 = (pmv - m_iNumOfTotalMVPerRow + PVOP_MV_PER_REF_PER_MB + gIndexOfCandBlk [blknCurr] [2]) -> trueMVHalfPel ();
		}
		break;
	case Y_BLOCK3:
		if (bLeftBndry)
			vctCandMV0.set (0,0);
		else
			vctCandMV0 = (pmv - PVOP_MV_PER_REF_PER_MB +  gIndexOfCandBlk [blknCurr] [0]) -> trueMVHalfPel ();
		vctCandMV1 = (pmv + gIndexOfCandBlk [blknCurr] [1]) -> trueMVHalfPel ();
		vctCandMV2 = (pmv + gIndexOfCandBlk [blknCurr] [2]) -> trueMVHalfPel ();
		break;
	case Y_BLOCK4:
		vctCandMV0 = (pmv + gIndexOfCandBlk [blknCurr] [0]) -> trueMVHalfPel ();
		vctCandMV1 = (pmv + gIndexOfCandBlk [blknCurr] [1]) -> trueMVHalfPel ();
		vctCandMV2 = (pmv + gIndexOfCandBlk [blknCurr] [2]) -> trueMVHalfPel ();
		break;
	}
	vecPred.x = medianof3 (vctCandMV0.x, vctCandMV1.x, vctCandMV2.x);
	vecPred.y = medianof3 (vctCandMV0.y, vctCandMV1.y, vctCandMV2.y);
}

Void CVideoObject::find8x8MVpredInterior (CVector& vecPred, const CMotionVector* pmv, Int blknCurr) const
{
	CVector vctCandMV0, vctCandMV1, vctCandMV2;

	switch (blknCurr){
	case Y_BLOCK1:	
		vctCandMV0 = (pmv - PVOP_MV_PER_REF_PER_MB +  gIndexOfCandBlk [blknCurr] [0]) -> trueMVHalfPel ();
		vctCandMV1 = (pmv - m_iNumOfTotalMVPerRow + gIndexOfCandBlk [blknCurr] [1]) -> trueMVHalfPel ();
		vctCandMV2 = (pmv - m_iNumOfTotalMVPerRow + PVOP_MV_PER_REF_PER_MB + gIndexOfCandBlk [blknCurr] [2]) -> trueMVHalfPel ();
		break;
	case Y_BLOCK2:
		vctCandMV0 = (pmv +  gIndexOfCandBlk [blknCurr] [0]) -> trueMVHalfPel ();
		vctCandMV1 = (pmv - m_iNumOfTotalMVPerRow + gIndexOfCandBlk [blknCurr] [1]) -> trueMVHalfPel ();
		vctCandMV2 = (pmv - m_iNumOfTotalMVPerRow + PVOP_MV_PER_REF_PER_MB + gIndexOfCandBlk [blknCurr] [2]) -> trueMVHalfPel ();
		break;
	case Y_BLOCK3:
		vctCandMV0 = (pmv - PVOP_MV_PER_REF_PER_MB +  gIndexOfCandBlk [blknCurr] [0]) -> trueMVHalfPel ();
		vctCandMV1 = (pmv +  gIndexOfCandBlk [blknCurr] [1]) -> trueMVHalfPel ();
		vctCandMV2 = (pmv +  gIndexOfCandBlk [blknCurr] [2]) -> trueMVHalfPel ();
		break;
	case Y_BLOCK4:
		vctCandMV0 = (pmv +  gIndexOfCandBlk [blknCurr] [0]) -> trueMVHalfPel ();
		vctCandMV1 = (pmv +  gIndexOfCandBlk [blknCurr] [1]) -> trueMVHalfPel ();
		vctCandMV2 = (pmv +  gIndexOfCandBlk [blknCurr] [2]) -> trueMVHalfPel ();
		break;
	}
	vecPred.x = medianof3 (vctCandMV0.x, vctCandMV1.x, vctCandMV2.x);
	vecPred.y = medianof3 (vctCandMV0.y, vctCandMV1.y, vctCandMV2.y);
}

Void CVideoObject::mvLookupUV (const CMBMode* pmbmd, const CMotionVector* pmv, 
							   CoordI& xRefUV, CoordI& yRefUV, CoordI& xRefUV1,CoordI& yRefUV1)
{
  Int dx = 0, dy = 0;
  if (m_volmd.bQuarterSample) {

	if (pmbmd -> m_bhas4MVForward)	{
      pmv++;
      for (UInt k = 1; k <= 4; k++){
        dx += pmv->iMVX;
        dy += pmv->iMVY;
        pmv++;
      }
      xRefUV = sign (dx) * (grgiMvRound16 [abs (dx) % 16] + (abs (dx) / 16) * 2);
      yRefUV = sign (dy) * (grgiMvRound16 [abs (dy) % 16] + (abs (dy) / 16) * 2);
	}
    // INTERLACE
	else if (pmbmd -> m_bFieldMV)	{
      if(pmbmd->m_bForwardTop) {
        dx = pmv [6].iMVX;
        dy = pmv [6].iMVY;
      }
      else {
        dx = pmv [5].iMVX;
        dy = pmv [5].iMVY;
      }
      xRefUV = ((dx & 3) ? ((dx >> 1) | 1) : (dx>>1));
      yRefUV = ((dy & 6) ? ((dy >> 1) | 2) : (dy>>1));
      if(pmbmd->m_bForwardBottom) {
        dx = pmv [8].iMVX;
        dy = pmv [8].iMVY;
      }
      else {
        dx = pmv [7].iMVX;
        dy = pmv [7].iMVY;
      }
      xRefUV1 = ((dx & 3) ? ((dx >> 1) | 1) : (dx>>1));
      yRefUV1 = ((dy & 6) ? ((dy >> 1) | 2) : (dy>>1));
	}
    // ~INTERLACE
	else {
      dx = pmv->iMVX;
      dy = pmv->iMVY;
      xRefUV = sign (dx) * (grgiMvRound4  [abs (dx) % 4] + (abs (dx) / 4) * 2);
      yRefUV = sign (dy) * (grgiMvRound4  [abs (dy) % 4] + (abs (dy) / 4) * 2);
	}
	  
  }

  else {
	  
	if (pmbmd -> m_bhas4MVForward)	{
      pmv++;
// RRV modification
	  if(m_vopmd.RRVmode.iRRVOnOff == 1)
	  {
		  for (UInt k = 1; k <= 4; k++)
		  {
			  dx += pmv->m_vctTrueHalfPel_x2.x;
			  dy += pmv->m_vctTrueHalfPel_x2.y;
			  pmv++;
		  }
	  }
	  else{
		  for (UInt k = 1; k <= 4; k++)
		  {
			  dx += pmv->m_vctTrueHalfPel.x;
			  dy += pmv->m_vctTrueHalfPel.y;
			  pmv++;
		  }
//      for (UInt k = 1; k <= 4; k++){
//        dx += pmv->m_vctTrueHalfPel.x;
//        dy += pmv->m_vctTrueHalfPel.y;
//        pmv++;
// ~RRV
      }
      xRefUV = sign (dx) * (grgiMvRound16 [abs (dx) % 16] + (abs (dx) / 16) * 2);
      yRefUV = sign (dy) * (grgiMvRound16 [abs (dy) % 16] + (abs (dy) / 16) * 2);
	}
    // INTERLACE
	else if (pmbmd -> m_bFieldMV)	{
      if(pmbmd->m_bForwardTop) {
        dx = pmv [6].m_vctTrueHalfPel.x;
        dy = pmv [6].m_vctTrueHalfPel.y;
      }
      else {
        dx = pmv [5].m_vctTrueHalfPel.x;
        dy = pmv [5].m_vctTrueHalfPel.y;
      }
      xRefUV = ((dx & 3) ? ((dx >> 1) | 1) : (dx>>1));
      yRefUV = ((dy & 6) ? ((dy >> 1) | 2) : (dy>>1));
      if(pmbmd->m_bForwardBottom) {
        dx = pmv [8].m_vctTrueHalfPel.x;
        dy = pmv [8].m_vctTrueHalfPel.y;
      }
      else {
        dx = pmv [7].m_vctTrueHalfPel.x;
        dy = pmv [7].m_vctTrueHalfPel.y;
      }
      xRefUV1 = ((dx & 3) ? ((dx >> 1) | 1) : (dx>>1));
      yRefUV1 = ((dy & 6) ? ((dy >> 1) | 2) : (dy>>1));
	}
    // ~INTERLACE
	else {
// RRV modification
		if(m_vopmd.RRVmode.iRRVOnOff == 1){
			dx = pmv->m_vctTrueHalfPel_x2.x;
			dy = pmv->m_vctTrueHalfPel_x2.y;
		}
		else{
			dx = pmv->m_vctTrueHalfPel.x;
			dy = pmv->m_vctTrueHalfPel.y;
		}
//		dx = pmv->m_vctTrueHalfPel.x;
//		dy = pmv->m_vctTrueHalfPel.y;
// ~RRV
      xRefUV = sign (dx) * (grgiMvRound4  [abs (dx) % 4] + (abs (dx) / 4) * 2);
      yRefUV = sign (dy) * (grgiMvRound4  [abs (dy) % 4] + (abs (dy) / 4) * 2);
	}
  }
  
}

Void CVideoObject::mvLookupUVWithShape (const CMBMode* pmbmd, const CMotionVector* pmv,
                                        CoordI& xRefUV, CoordI& yRefUV)
{
  Int* rgiMvRound = NULL;
  Int dx = 0, dy = 0;
  UInt uiDivisor = 0; //2 = Y->UV resolution change; another -> full pel
  if (pmbmd -> m_bhas4MVForward || pmbmd -> m_mbType == DIRECT)	{ //for QuarterPel. mwi
    if (m_volmd.bQuarterSample) {
      for (UInt i = Y_BLOCK1; i <= Y_BLOCK4; i++)	{
        pmv++;
        if (pmbmd->m_rgTranspStatus [i] != ALL)	{
          dx += pmv->iMVX;
          dy += pmv->iMVY;
          uiDivisor += 4; }
      }
    }
    else {
      for (UInt i = Y_BLOCK1; i <= Y_BLOCK4; i++)	{
        pmv++;
        if (pmbmd->m_rgTranspStatus [i] != ALL)	{
          dx += pmv->m_vctTrueHalfPel.x;
          dy += pmv->m_vctTrueHalfPel.y;
          uiDivisor += 4;
        }
      }
    }
      
    switch (uiDivisor)	{
    case 4:
      rgiMvRound = grgiMvRound4;
      break;
    case 8:
      rgiMvRound = grgiMvRound8;
      break;
    case 12:
      rgiMvRound = grgiMvRound12;
      break;
    case 16:
      rgiMvRound = grgiMvRound16;
      break;
    }
    xRefUV = sign (dx) * (rgiMvRound [abs (dx) % uiDivisor] + (abs (dx) / uiDivisor) * 2);
    yRefUV = sign (dy) * (rgiMvRound [abs (dy) % uiDivisor] + (abs (dy) / uiDivisor) * 2);
  }
  else {
    if (m_volmd.bQuarterSample) {
      dx = pmv->iMVX;
      dy = pmv->iMVY;
    }
    else {
      dx = pmv->m_vctTrueHalfPel.x;
      dy = pmv->m_vctTrueHalfPel.y;
    }
    xRefUV = sign (dx) * (grgiMvRound4  [abs (dx) % 4] + (abs (dx) / 4) * 2);
    yRefUV = sign (dy) * (grgiMvRound4  [abs (dy) % 4] + (abs (dy) / 4) * 2);
  }
}

Void CVideoObject::findMVpredGeneric (CVector& vecPred, 
									  const CMotionVector* pmv, 
									  const CMBMode* pmbmd, 
									  Int iBlk, 
									  Int iXMB, Int iYMB) const
{
	static Bool rgbInBound [3];
	rgbInBound [0] = FALSE;
	rgbInBound [1] = FALSE;
	rgbInBound [2] = FALSE;
	Int nInBound = 0;
	CVector vctCandMV [3];
	UInt i;
	for (i = 0; i < 3; i++)	{
		vctCandMV [i].x = 0;
		vctCandMV [i].y = 0;
	}

	Bool bLeftBndry, bRightBndry, bTopBndry;
	Int iMBnum = VPMBnum(iXMB, iYMB);
	bLeftBndry = bVPNoLeft(iMBnum, iXMB);
	bTopBndry = bVPNoTop(iMBnum);
	bRightBndry = bVPNoRightTop(iMBnum, iXMB);

	if (pmbmd->m_bhas4MVForward == TRUE)	{		//how about backward??
		switch (iBlk){
		case Y_BLOCK1:
//	Modified for error resilient mode by Toshiba(1997-11-14)
//			if (iXMB != 0 && validBlock (pmbmd - 1, gIndexOfCandBlk [iBlk] [0]))	{
			if (!bLeftBndry && validBlock (pmbmd, pmbmd - 1, gIndexOfCandBlk [iBlk] [0]))	{
				vctCandMV [0] = (pmv - PVOP_MV_PER_REF_PER_MB + gIndexOfCandBlk [iBlk] [0]) -> trueMVHalfPel ();
				rgbInBound [0] = TRUE;
				nInBound++;
			}
			if (iYMB != 0)	{
//	Modified for error resilient mode by Toshiba(1997-11-14)
//				if (validBlock (pmbmd - m_iNumMBX, gIndexOfCandBlk [iBlk] [1]))	{
				if (!bTopBndry && validBlock (pmbmd, pmbmd - m_iNumMBX, gIndexOfCandBlk [iBlk] [1]))	{
					vctCandMV [1] = (pmv - m_iNumOfTotalMVPerRow + gIndexOfCandBlk [iBlk] [1])->trueMVHalfPel ();
					rgbInBound [1] = TRUE;
					nInBound++;
				}
//	Modified for error resilient mode by Toshiba(1997-11-14)
//				if (iXMB < m_iNumMBX - 1 && validBlock (pmbmd - m_iNumMBX + 1, gIndexOfCandBlk [iBlk] [2]))	{
				if (!bRightBndry && validBlock (pmbmd, pmbmd - m_iNumMBX + 1, gIndexOfCandBlk [iBlk] [2]))	{
					vctCandMV [2] = (pmv - m_iNumOfTotalMVPerRow + PVOP_MV_PER_REF_PER_MB + gIndexOfCandBlk [iBlk] [2])->trueMVHalfPel ();
					rgbInBound [2] = TRUE;
					nInBound++;
				}
			}
			break;
		case Y_BLOCK2:
			if (validBlock (pmbmd, pmbmd, gIndexOfCandBlk [iBlk] [0]))	{
				vctCandMV [0] = (pmv +  gIndexOfCandBlk [iBlk] [0]) -> trueMVHalfPel ();
				rgbInBound [0] = TRUE;
				nInBound++;
			}
			if (iYMB != 0)	{
//	Modified for error resilient mode by Toshiba(1997-11-14)
//				if (validBlock (pmbmd - m_iNumMBX, gIndexOfCandBlk [iBlk] [1]))	{
				if (!bTopBndry && validBlock (pmbmd, pmbmd - m_iNumMBX, gIndexOfCandBlk [iBlk] [1]))	{
					vctCandMV [1] = (pmv - m_iNumOfTotalMVPerRow + gIndexOfCandBlk [iBlk] [1]) -> trueMVHalfPel ();
					rgbInBound [1] = TRUE;
					nInBound++;
				}
//	Modified for error resilient mode by Toshiba(1997-11-14)
//				if (iXMB < m_iNumMBX - 1 && validBlock (pmbmd - m_iNumMBX + 1, gIndexOfCandBlk [iBlk] [2]))	{
				if (!bRightBndry && validBlock (pmbmd, pmbmd - m_iNumMBX + 1, gIndexOfCandBlk [iBlk] [2]))	{
					vctCandMV [2] = (pmv - m_iNumOfTotalMVPerRow + PVOP_MV_PER_REF_PER_MB + gIndexOfCandBlk [iBlk] [2]) -> trueMVHalfPel ();
					rgbInBound [2] = TRUE;
					nInBound++;
				}
			}
			break;
		case Y_BLOCK3:
//	Modified for error resilient mode by Toshiba(1997-11-14)
//			if (iXMB != 0 && validBlock (pmbmd - 1, gIndexOfCandBlk [iBlk] [0]))	{
			if (!bLeftBndry && validBlock (pmbmd, pmbmd - 1, gIndexOfCandBlk [iBlk] [0]))	{
				vctCandMV [0] = (pmv - PVOP_MV_PER_REF_PER_MB +  gIndexOfCandBlk [iBlk] [0]) -> trueMVHalfPel ();
				rgbInBound [0] = TRUE;
				nInBound++;
			}
			if (validBlock (pmbmd, pmbmd, gIndexOfCandBlk [iBlk] [1]))	{
				vctCandMV [1] = (pmv + gIndexOfCandBlk [iBlk] [1]) -> trueMVHalfPel ();
				rgbInBound [1] = TRUE;
				nInBound++;
			}
			if (validBlock (pmbmd, pmbmd, gIndexOfCandBlk [iBlk] [2]))	{
				vctCandMV [2] = (pmv + gIndexOfCandBlk [iBlk] [2]) -> trueMVHalfPel ();
				rgbInBound [2] = TRUE;
				nInBound++;
			}
			break;
		case Y_BLOCK4:
			if (validBlock (pmbmd, pmbmd, gIndexOfCandBlk [iBlk] [0]))	{
				vctCandMV [0] = (pmv + gIndexOfCandBlk [iBlk] [0]) -> trueMVHalfPel ();
				rgbInBound [0] = TRUE;
				nInBound++;
			}
			if (validBlock (pmbmd, pmbmd, gIndexOfCandBlk [iBlk] [1]))	{
				vctCandMV [1] = (pmv + gIndexOfCandBlk [iBlk] [1]) -> trueMVHalfPel ();
				rgbInBound [1] = TRUE;
				nInBound++;
			}
			if (validBlock (pmbmd, pmbmd, gIndexOfCandBlk [iBlk] [2]))	{
				vctCandMV [2] = (pmv + gIndexOfCandBlk [iBlk] [2]) -> trueMVHalfPel ();
				rgbInBound [2] = TRUE;
				nInBound++;
			}
			break;
		}
	}
	else {
//	Modified for error resilient mode by Toshiba(1997-11-14)
//		if (iXMB != 0 && validBlock (pmbmd - 1, gIndexOfCandBlk [1] [0]))	{
		if (!bLeftBndry && validBlock (pmbmd, pmbmd - 1, gIndexOfCandBlk [1] [0]))	{
			vctCandMV [0] = (pmv - PVOP_MV_PER_REF_PER_MB + gIndexOfCandBlk [1] [0]) -> trueMVHalfPel ();
			rgbInBound [0] = TRUE;
			nInBound++;
		}
		if (iYMB != 0)   {
//	Modified for error resilient mode by Toshiba(1997-11-14)
//			if (validBlock (pmbmd - m_iNumMBX, gIndexOfCandBlk [1] [1]))	{
			if (!bTopBndry && validBlock (pmbmd, pmbmd - m_iNumMBX, gIndexOfCandBlk [1] [1]))	{
				vctCandMV [1] = (pmv - m_iNumOfTotalMVPerRow + gIndexOfCandBlk [1] [1]) -> trueMVHalfPel ();
				rgbInBound [1] = TRUE;
				nInBound++;
			}
//	Modified for error resilient mode by Toshiba(1997-11-14)
//			if (iXMB < m_iNumMBX - 1 && validBlock (pmbmd - m_iNumMBX + 1, gIndexOfCandBlk [1] [2]))	{
			if (!bRightBndry && validBlock (pmbmd, pmbmd - m_iNumMBX + 1, gIndexOfCandBlk [1] [2]))	{
				vctCandMV [2] = (pmv - m_iNumOfTotalMVPerRow + PVOP_MV_PER_REF_PER_MB + gIndexOfCandBlk [1] [2]) -> trueMVHalfPel ();
				rgbInBound [2] = TRUE;
				nInBound++;
			}
		}
	}

	if (nInBound == 1)	{
		for (UInt i = 0; i < 3; i++)	{
			if (rgbInBound [i] == TRUE)	{
				vecPred = vctCandMV [i];
				return;
			}
		}
	}

	vecPred.x = medianof3 (vctCandMV [0].x, vctCandMV [1].x, vctCandMV [2].x);
	vecPred.y = medianof3 (vctCandMV [0].y, vctCandMV [1].y, vctCandMV [2].y);
}

Bool CVideoObject::validBlock (const CMBMode* pmbmdCurr, const CMBMode* pmbmd, Int blkn) const
{
	// block tests with mv padding
	if (pmbmd->m_rgTranspStatus [0] == ALL)		
		return FALSE;
	else if(pmbmd != pmbmdCurr)
		return TRUE;
	else {
		if(pmbmd->m_rgTranspStatus [blkn] == ALL)
			return FALSE;
		else
			return TRUE;
	}
}

Void CVideoObject::findMVpredictorOfBVOP (CVector& vctPred, const CMotionVector* pmv, const CMBMode* pmbmd, Int iMBX) const  		    //for B-VOP only
{
	vctPred.x = vctPred.y = 0;		//intialize to 0
	MBType mbtypCurr = pmbmd->m_mbType;
	for (Int iMBXCandidate = iMBX - 1; iMBXCandidate >= 0; iMBXCandidate--)	{		//scan backward MBs
		pmbmd--;
		pmv -= 5;
		if (pmbmd->m_bSkip)
			return;
		else if (pmbmd->m_mbType == mbtypCurr && pmbmd->m_rgTranspStatus [0] != ALL)	{
			vctPred = pmv->m_vctTrueHalfPel;
			return;
		}
	}
	return;							//reach the start of the row; reset
}

Void CVideoObject::backwardMVFromForwardMV (								//compute back mv from forward mv and ref mv for direct mode
					CMotionVector& mvBackward, const CMotionVector& mvForward,
					const CMotionVector& mvRef,	CVector vctDirectDeltaMV)
{
	assert (mvForward.iMVX != NOT_MV && mvForward.iMVY != NOT_MV);	//mv is valid
	CVector vctBackward;
	Int iFullInterval = m_tFutureRef - m_tPastRef;
	if (vctDirectDeltaMV.x == 0)
		vctBackward.x = (m_t - m_tFutureRef) * mvRef.m_vctTrueHalfPel.x / iFullInterval;
	else 
		vctBackward.x = mvForward.m_vctTrueHalfPel.x - mvRef.m_vctTrueHalfPel.x;

	if (vctDirectDeltaMV.y == 0)
		vctBackward.y = (m_t - m_tFutureRef) * mvRef.m_vctTrueHalfPel.y / iFullInterval;
	else 
		vctBackward.y = mvForward.m_vctTrueHalfPel.y - mvRef.m_vctTrueHalfPel.y;
	mvBackward = CMotionVector (vctBackward);
}


CVector CVideoObject::averageOfRefMV (const CMotionVector* pmvRef, const CMBMode* pmbmdRef)	
{
	assert(pmvRef!=NULL);
	CVector vctRef (0, 0);
	Int i;
	if (pmbmdRef -> m_bhas4MVForward)	{						//average
		Int iDivisor = 0;
		for (i = Y_BLOCK1; i <= Y_BLOCK4; i++)	{
			pmvRef++;
			if (pmbmdRef->m_rgTranspStatus [i] != ALL)	{
				assert (pmvRef->iMVX != NOT_MV);
				vctRef.x += pmvRef->m_vctTrueHalfPel.x;
				vctRef.y += pmvRef->m_vctTrueHalfPel.y;
				iDivisor += 1;
			}
		}
		vctRef.x = (Int) rounded((Float) vctRef.x / iDivisor);
		vctRef.y = (Int) rounded((Float) vctRef.y / iDivisor);
	}
	else {
		vctRef = pmvRef->m_vctTrueHalfPel;
		assert (pmvRef->iMVX != NOT_MV);
	}
	return vctRef;
}

Void CVideoObject::padMotionVectors (const CMBMode* pmbmd,CMotionVector *pmv)
{
	if(pmbmd->m_rgTranspStatus[0]==ALL)
		return;

	if(pmbmd->m_rgTranspStatus[1]==ALL)
	{
		pmv[1]=(pmbmd->m_rgTranspStatus[2]!=ALL)?pmv[2]:
			((pmbmd->m_rgTranspStatus[3]!=ALL)?pmv[3]:pmv[4]);
	}
	if(pmbmd->m_rgTranspStatus[2]==ALL)
	{
		pmv[2]=(pmbmd->m_rgTranspStatus[1]!=ALL)?pmv[1]:
			((pmbmd->m_rgTranspStatus[4]!=ALL)?pmv[4]:pmv[3]);
	}
	if(pmbmd->m_rgTranspStatus[3]==ALL)
	{
		pmv[3]=(pmbmd->m_rgTranspStatus[4]!=ALL)?pmv[4]:
			((pmbmd->m_rgTranspStatus[1]!=ALL)?pmv[1]:pmv[2]);
	}
	if(pmbmd->m_rgTranspStatus[4]==ALL)
	{
		pmv[4]=(pmbmd->m_rgTranspStatus[3]!=ALL)?pmv[3]:
			((pmbmd->m_rgTranspStatus[2]!=ALL)?pmv[2]:pmv[1]);
	}
}

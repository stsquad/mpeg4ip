/*************************************************************************

This software module was originally developed by 

	Ming-Chieh Lee (mingcl@microsoft.com), Microsoft Corporation
	Bruce Lin (blin@microsoft.com), Microsoft Corporation
	Simon Winder (swinder@microsoft.com), Microsoft Corporation
	(date: June, 1997)
and edited by
        Wei Wu (weiwu@stallion.risc.rockwell.com) Rockwell Science Center

and also edited by
    Mathias Wien (wien@ient.rwth-aachen.de) RWTH Aachen / Robert BOSCH GmbH

and also edited by
	Yoshinori Suzuki (Hitachi, Ltd.)

and also edited by
	Hideaki Kimata (NTT)

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

	mc.cpp

Abstract:

	Motion compensation routines (common for encoder and decoder).

Revision History:
    December 20, 1997   Interlaced tools added by NextLevel Systems (GI)
                        X. Chen (xchen@nlvl.com) B. Eifrig (beifrig@nlvl.com)
    Feb.16 1999         add Quarter Sample 
                        Mathias Wien (wien@ient.rwth-aachen.de) 
    Feb.23 1999         GMC added by Yoshinori Suzuki (Hitachi, Ltd.)
	Aug.24, 1999 : NEWPRED added by Hideaki Kimata (NTT) 
	Sep.06	1999 : RRV added by Eishi Morimatsu (Fujitsu Laboratories Ltd.) 

*************************************************************************/

#include <stdio.h>

#include "typeapi.h"
#include "codehead.h"
#include "mode.hpp"
#include "vopses.hpp"
#include "global.hpp"

// NEWPRED
//#include "entropy/bitstrm.hpp"
#include "newpred.hpp"
// ~NEWPRED

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

Void CVideoObject::limitMVRangeToExtendedBBFullPel (CoordI &x,CoordI &y,CRct *prct,Int iBlkSize)
{
	if(prct==NULL)
		return;

	if(x < prct->left)
		x=prct->left;
	else if(x > (prct->right-iBlkSize))
		x=(prct->right-iBlkSize);
	if(y < prct->top)
		y=prct->top;
	else if(y > (prct->bottom-iBlkSize))
		y=(prct->bottom-iBlkSize);
}

Void CVideoObject::limitMVRangeToExtendedBBHalfPel (CoordI &x,CoordI &y,CRct *prct,Int iBlkSize)
{
	if(prct==NULL)
		return;

	if(x < prct->left*2)
		x=prct->left*2;
	else if(x > (prct->right-iBlkSize)*2)
		x=(prct->right-iBlkSize)*2;
	if(y < prct->top*2)
		y=prct->top*2;
	else if(y > (prct->bottom-iBlkSize)*2)
		y=(prct->bottom-iBlkSize)*2;
}

 // Quarter sample
Void CVideoObject::limitMVRangeToExtendedBBQuarterPel (CoordI &x,CoordI &y,CRct *prct,Int iBlkSize)
{
	if(prct==NULL)
		return;

    Int iBlkSizeX, iBlkSizeY;
    
    if (iBlkSize == 0) {
      iBlkSizeX = MB_SIZE;
      iBlkSizeY = MB_SIZE; // Field vectors are given in frame coordinates
    }
    else {
      iBlkSizeX = iBlkSize; 
      iBlkSizeY = iBlkSize; 
    }
    
	if( x < (prct->left + EXPANDY_REFVOP - iBlkSizeX)*4)
		x = (prct->left + EXPANDY_REFVOP - iBlkSizeX)*4;
	else if (x > (prct->right - EXPANDY_REFVOP)*4)
		x = (prct->right - EXPANDY_REFVOP)*4;
	if( y < (prct->top + EXPANDY_REFVOP - iBlkSizeY)*4)
		y = (prct->top + EXPANDY_REFVOP - iBlkSizeY)*4;
	else if (y > (prct->bottom - EXPANDY_REFVOP)*4)
		y = (prct->bottom - EXPANDY_REFVOP)*4;
}
// ~Quarter sample

Void CVideoObject::motionCompMB (
	PixelC* ppxlcPredMB,
	const PixelC* ppxlcRefLeftTop,
	const CMotionVector* pmv, const CMBMode* pmbmd, 
	Int imbX, Int imbY,
	CoordI x, CoordI y,
	Bool bSkipNonOBMC,
	Bool bAlphaMB,
	CRct *prctMVLimit
)
{
// RRV insertion
	PixelC *pc_block16x16 = NULL;
	if(m_vopmd.RRVmode.iRRVOnOff == 1)
	{
    	pc_block16x16= new PixelC[256];
	}
// ~RRV

  if (!bAlphaMB && !m_volmd.bAdvPredDisable && !pmbmd->m_bFieldMV && !pmbmd->m_bMCSEL) { // GMC
    motionCompOverLap (
                       ppxlcPredMB, ppxlcRefLeftTop,
                       pmv, pmbmd,
                       imbX, imbY,
                       x, y,
                       prctMVLimit
                       );
  }
  else {
    if (bSkipNonOBMC && !pmbmd -> m_bMCSEL) // GMC
      return;
    if (!pmbmd -> m_bhas4MVForward && !pmbmd -> m_bFieldMV && !pmbmd -> m_bMCSEL) // GMC
      if (m_volmd.bQuarterSample) // Quarter sample
        motionCompQuarterSample (ppxlcPredMB, ppxlcRefLeftTop, MB_SIZE,
                                 x * 4 + pmv->trueMVHalfPel ().x,
                                 y * 4 + pmv->trueMVHalfPel ().y,
                                 m_vopmd.iRoundingControl, prctMVLimit);
      else
// RRV modification		  
		  if(m_vopmd.RRVmode.iRRVOnOff == 1)
		  {
			  motionComp(ppxlcPredMB, ppxlcRefLeftTop,
						 (MB_SIZE *2), 
						 x * 2 + pmv->trueMVHalfPel_x2 ().x, 
						 y * 2 + pmv->trueMVHalfPel_x2 ().y,
						 m_vopmd.iRoundingControl,
						 prctMVLimit);
		  }
		  else
		  {
			  motionComp(ppxlcPredMB, ppxlcRefLeftTop,
						 MB_SIZE, 
						 x * 2 + pmv->trueMVHalfPel ().x, 
						 y * 2 + pmv->trueMVHalfPel ().y ,
						 m_vopmd.iRoundingControl,
						 prctMVLimit);
		  }
//        motionComp (
//                    ppxlcPredMB, ppxlcRefLeftTop,
//                    MB_SIZE, 
//                    x * 2 + pmv->trueMVHalfPel ().x, 
//                    y * 2 + pmv->trueMVHalfPel ().y ,
//                    m_vopmd.iRoundingControl,
//                    prctMVLimit
//                    );
// ~RRV
    else if (pmbmd -> m_bFieldMV) {
      const CMotionVector* pmv16x8 = pmv+5;
      if(pmbmd->m_bForwardTop) {
        pmv16x8++;
        if (m_volmd.bQuarterSample) // Quarter sample
          motionCompQuarterSample (
                                   ppxlcPredMB, 
                                   ppxlcRefLeftTop+m_iFrameWidthY,
                                   0,
                                   x * 4 + pmv16x8->trueMVHalfPel ().x,
                                   y * 4 + pmv16x8->trueMVHalfPel ().y,
                                   m_vopmd.iRoundingControl, prctMVLimit
                                   );
        else
          motionCompYField (
                            ppxlcPredMB,
                            ppxlcRefLeftTop+m_iFrameWidthY,
                            x * 2 + pmv16x8->trueMVHalfPel ().x, 
                            y * 2 + pmv16x8->trueMVHalfPel ().y, 
                            prctMVLimit); // added by Y.Suzuki for the extended bounding box support
        pmv16x8++;
      }
      else {
        if (m_volmd.bQuarterSample) // Quarter sample
          motionCompQuarterSample (
                                   ppxlcPredMB, 
                                   ppxlcRefLeftTop,
                                   0,
                                   x * 4 + pmv16x8->trueMVHalfPel ().x,
                                   y * 4 + pmv16x8->trueMVHalfPel ().y,
                                   m_vopmd.iRoundingControl, prctMVLimit
                                   );
        else
          motionCompYField (
                            ppxlcPredMB,
                            ppxlcRefLeftTop,
                            x * 2 + pmv16x8->trueMVHalfPel ().x, 
                            y * 2 + pmv16x8->trueMVHalfPel ().y, 
                            prctMVLimit); // added by Y.Suzuki for the extended bounding box support
        pmv16x8++;
        pmv16x8++;
      }
      if(pmbmd->m_bForwardBottom) {
        pmv16x8++;
        if (m_volmd.bQuarterSample) // Quarter sample
          motionCompQuarterSample (
                                   ppxlcPredMB+MB_SIZE, 
                                   ppxlcRefLeftTop+m_iFrameWidthY,
                                   0,
                                   x * 4 + pmv16x8->trueMVHalfPel ().x,
                                   y * 4 + pmv16x8->trueMVHalfPel ().y,
                                   m_vopmd.iRoundingControl, prctMVLimit
                                   );
        else
          motionCompYField (
                            ppxlcPredMB+MB_SIZE,
                            ppxlcRefLeftTop+m_iFrameWidthY,
                            x * 2 + pmv16x8->trueMVHalfPel ().x, 
                            y * 2 + pmv16x8->trueMVHalfPel ().y, 
                            prctMVLimit); // added by Y.Suzuki for the extended bounding box support
      }
      else {
        if (m_volmd.bQuarterSample) // Quarter sample
          motionCompQuarterSample (
                                   ppxlcPredMB+MB_SIZE, 
                                   ppxlcRefLeftTop,
                                   0,
                                   x * 4 + pmv16x8->trueMVHalfPel ().x,
                                   y * 4 + pmv16x8->trueMVHalfPel ().y,
                                   m_vopmd.iRoundingControl, prctMVLimit
                                   );
        else
          motionCompYField (
                            ppxlcPredMB+MB_SIZE,
                            ppxlcRefLeftTop,
                            x * 2 + pmv16x8->trueMVHalfPel ().x, 
                            y * 2 + pmv16x8->trueMVHalfPel ().y, 
                            prctMVLimit); // added by Y.Suzuki for the extended bounding box support
      }
    }
// GMC
    else if(pmbmd -> m_bMCSEL){
	FindGlobalPredForGMC (x,y,ppxlcPredMB,ppxlcRefLeftTop);

    }
// ~GMC
    else {
      const CMotionVector* pmv8 = pmv;
      pmv8++;
// RRV modification
	  CoordI blkX, blkY;
	  if(m_vopmd.RRVmode.iRRVOnOff == 1)
	  {
		  blkX = x + BLOCK_SIZE *2;
		  blkY = y + BLOCK_SIZE *2;
		  motionComp(pc_block16x16, ppxlcRefLeftTop,
					 (BLOCK_SIZE *2),
					 x * 2 + pmv8->trueMVHalfPel_x2 ().x, 
					 y * 2 + pmv8->trueMVHalfPel_x2 ().y,
					 m_vopmd.iRoundingControl,
					 prctMVLimit);
		  writeCubicRct(BLOCK_SIZE *2, MB_SIZE *2,
						pc_block16x16, ppxlcPredMB);
		  
		  pmv8++;
		  motionComp(pc_block16x16, ppxlcRefLeftTop,
					 (BLOCK_SIZE *2), 
					 blkX * 2 + pmv8->trueMVHalfPel_x2 ().x, 
					 y * 2 + pmv8->trueMVHalfPel_x2 ().y,
					 m_vopmd.iRoundingControl,
					 prctMVLimit);
		  writeCubicRct(BLOCK_SIZE *2, MB_SIZE *2,
						pc_block16x16, ppxlcPredMB +(BLOCK_SIZE *2));

		  pmv8++;
		  motionComp(pc_block16x16, ppxlcRefLeftTop,
					 (BLOCK_SIZE *2), 
					 x * 2 + pmv8->trueMVHalfPel_x2 ().x, 
					 blkY * 2 + pmv8->trueMVHalfPel_x2 ().y,
					 m_vopmd.iRoundingControl,
					 prctMVLimit);
		  writeCubicRct(BLOCK_SIZE *2, MB_SIZE *2,
						pc_block16x16, ppxlcPredMB +(MB_SIZE * MB_SIZE *2));

		  pmv8++;
		  motionComp(pc_block16x16, ppxlcRefLeftTop,
					 (BLOCK_SIZE *2), 
					 blkX * 2 + pmv8->trueMVHalfPel_x2 ().x, 
					 blkY * 2 + pmv8->trueMVHalfPel_x2 ().y,
					 m_vopmd.iRoundingControl,
					 prctMVLimit);
		  writeCubicRct(BLOCK_SIZE *2, MB_SIZE *2,
						pc_block16x16, ppxlcPredMB +(MB_SIZE * MB_SIZE *2) +(BLOCK_SIZE *2));
	  }
	  else // !RRV
	  {
		  blkX = x + BLOCK_SIZE;
		  blkY = y + BLOCK_SIZE;
		  if (pmbmd->m_rgTranspStatus [Y_BLOCK1] != ALL)
			  if (m_volmd.bQuarterSample) // Quarter sample
				  motionCompQuarterSample (ppxlcPredMB, ppxlcRefLeftTop, BLOCK_SIZE,
										   x * 4 + 2 * pmv8->iMVX + pmv8->iHalfX, 
										   y * 4 + 2 * pmv8->iMVY + pmv8->iHalfY,
										   m_vopmd.iRoundingControl, prctMVLimit);
			  else
				  motionComp (
							  ppxlcPredMB, ppxlcRefLeftTop,
							  BLOCK_SIZE, 
							  x * 2 + pmv8->trueMVHalfPel ().x, 
							  y * 2 + pmv8->trueMVHalfPel ().y,
							  m_vopmd.iRoundingControl,
							  prctMVLimit
							  );
		  pmv8++;
		  if (pmbmd->m_rgTranspStatus [Y_BLOCK2] != ALL)
			  if (m_volmd.bQuarterSample) // Quarter sample
				  motionCompQuarterSample (ppxlcPredMB + OFFSET_BLK1, 
										   ppxlcRefLeftTop, BLOCK_SIZE,
										   blkX * 4 + 2 * pmv8->iMVX + pmv8->iHalfX, 
										   y * 4 + 2 * pmv8->iMVY + pmv8->iHalfY,
										   m_vopmd.iRoundingControl, prctMVLimit);
			  else
				  motionComp (
							  ppxlcPredMB + OFFSET_BLK1, ppxlcRefLeftTop,
							  BLOCK_SIZE, 
							  blkX * 2 + pmv8->trueMVHalfPel ().x, 
							  y * 2 + pmv8->trueMVHalfPel ().y,
							  m_vopmd.iRoundingControl,
							  prctMVLimit
							  );
		  pmv8++;
		  if (pmbmd->m_rgTranspStatus [Y_BLOCK3] != ALL)
			  if (m_volmd.bQuarterSample) // Quarter sample
				  motionCompQuarterSample (ppxlcPredMB + OFFSET_BLK2, 
										   ppxlcRefLeftTop, BLOCK_SIZE,
										   x * 4 + 2 * pmv8->iMVX + pmv8->iHalfX, 
										   blkY * 4 + 2 * pmv8->iMVY + pmv8->iHalfY,
										   m_vopmd.iRoundingControl, prctMVLimit);
			  else
				  motionComp (
							  ppxlcPredMB + OFFSET_BLK2, ppxlcRefLeftTop,
							  BLOCK_SIZE, 
							  x * 2 + pmv8->trueMVHalfPel ().x, 
							  blkY * 2 + pmv8->trueMVHalfPel ().y,
							  m_vopmd.iRoundingControl,
							  prctMVLimit
							  );
		  pmv8++;
		  if (pmbmd->m_rgTranspStatus [Y_BLOCK4] != ALL)
			  if (m_volmd.bQuarterSample) // Quarter sample
				  motionCompQuarterSample (ppxlcPredMB + OFFSET_BLK3, 
										   ppxlcRefLeftTop, BLOCK_SIZE,
										   blkX * 4 + 2 * pmv8->iMVX + pmv8->iHalfX, 
										   blkY * 4 + 2 * pmv8->iMVY + pmv8->iHalfY,
										   m_vopmd.iRoundingControl, prctMVLimit);
			  else
				  motionComp (
							  ppxlcPredMB + OFFSET_BLK3, ppxlcRefLeftTop,
							  BLOCK_SIZE, 
							  blkX * 2 + pmv8->trueMVHalfPel ().x, 
							  blkY * 2 + pmv8->trueMVHalfPel ().y,
							  m_vopmd.iRoundingControl,
							  prctMVLimit
							  );
	  }
//      CoordI blkX = x + BLOCK_SIZE;
//	    CoordI blkY = y + BLOCK_SIZE;
//      if (pmbmd->m_rgTranspStatus [Y_BLOCK1] != ALL)
//        if (m_volmd.bQuarterSample) // Quarter sample
//          motionCompQuarterSample (ppxlcPredMB, ppxlcRefLeftTop, BLOCK_SIZE,
//                                   x * 4 + 2 * pmv8->iMVX + pmv8->iHalfX, 
//                                   y * 4 + 2 * pmv8->iMVY + pmv8->iHalfY,
//                                   m_vopmd.iRoundingControl, prctMVLimit);
//        else
//          motionComp (
//                      ppxlcPredMB, ppxlcRefLeftTop,
//                      BLOCK_SIZE, 
//                      x * 2 + pmv8->trueMVHalfPel ().x, 
//                      y * 2 + pmv8->trueMVHalfPel ().y,
//                      m_vopmd.iRoundingControl,
//                      prctMVLimit
//                      );
//      pmv8++;
//      if (pmbmd->m_rgTranspStatus [Y_BLOCK2] != ALL)
//        if (m_volmd.bQuarterSample) // Quarter sample
//          motionCompQuarterSample (ppxlcPredMB + OFFSET_BLK1, 
//                                   ppxlcRefLeftTop, BLOCK_SIZE,
//                                   blkX * 4 + 2 * pmv8->iMVX + pmv8->iHalfX, 
//                                   y * 4 + 2 * pmv8->iMVY + pmv8->iHalfY,
//                                   m_vopmd.iRoundingControl, prctMVLimit);
//        else
//          motionComp (
//                      ppxlcPredMB + OFFSET_BLK1, ppxlcRefLeftTop,
//                      BLOCK_SIZE, 
//                      blkX * 2 + pmv8->trueMVHalfPel ().x, 
//                      y * 2 + pmv8->trueMVHalfPel ().y,
//                      m_vopmd.iRoundingControl,
//                      prctMVLimit
//                      );
//      pmv8++;
//      if (pmbmd->m_rgTranspStatus [Y_BLOCK3] != ALL)
//        if (m_volmd.bQuarterSample) // Quarter sample
//          motionCompQuarterSample (ppxlcPredMB + OFFSET_BLK2, 
//                                   ppxlcRefLeftTop, BLOCK_SIZE,
//                                   x * 4 + 2 * pmv8->iMVX + pmv8->iHalfX, 
//                                   blkY * 4 + 2 * pmv8->iMVY + pmv8->iHalfY,
//                                   m_vopmd.iRoundingControl, prctMVLimit);
//        else
//          motionComp (
//                      ppxlcPredMB + OFFSET_BLK2, ppxlcRefLeftTop,
//                      BLOCK_SIZE, 
//                      x * 2 + pmv8->trueMVHalfPel ().x, 
//                      blkY * 2 + pmv8->trueMVHalfPel ().y,
//                      m_vopmd.iRoundingControl,
//                      prctMVLimit
//                      );
//      pmv8++;
//      if (pmbmd->m_rgTranspStatus [Y_BLOCK4] != ALL)
//        if (m_volmd.bQuarterSample) // Quarter sample
//          motionCompQuarterSample (ppxlcPredMB + OFFSET_BLK3, 
//                                   ppxlcRefLeftTop, BLOCK_SIZE,
//                                   blkX * 4 + 2 * pmv8->iMVX + pmv8->iHalfX, 
//                                   blkY * 4 + 2 * pmv8->iMVY + pmv8->iHalfY,
//                                   m_vopmd.iRoundingControl, prctMVLimit);
//        else
//          motionComp (
//                      ppxlcPredMB + OFFSET_BLK3, ppxlcRefLeftTop,
//                      BLOCK_SIZE, 
//                      blkX * 2 + pmv8->trueMVHalfPel ().x, 
//                      blkY * 2 + pmv8->trueMVHalfPel ().y,
//                      m_vopmd.iRoundingControl,
//                      prctMVLimit
//                      );
// ~RRV
    }
  }
// RRV insertion
	if(m_vopmd.RRVmode.iRRVOnOff == 1)
	{
		delete	pc_block16x16;
	}
// ~RRV
}

Void CVideoObject::motionComp (
	PixelC* ppxlcPred, // can be either Y or A
	const PixelC* ppxlcRefLeftTop, // point to left-top of the frame
	Int iSize, // either MB or BLOCK size
	CoordI xRef, CoordI yRef, // x + mvX, in half pel unit
	Int iRoundingControl,
	CRct *prctMVLimit // extended bounding box
)
{
	CoordI ix, iy;

	limitMVRangeToExtendedBBHalfPel(xRef,yRef,prctMVLimit,iSize);

	const PixelC* ppxlcRef = 
		ppxlcRefLeftTop + 
		((yRef >> 1) + EXPANDY_REF_FRAME) * m_iFrameWidthY + (xRef >> 1) + EXPANDY_REF_FRAME;

	if(iSize==8 || iSize==16)
	{
		// optimisation
		if (!(yRef & 1)) {
			if (!(xRef & 1)) {
				Int iSz = iSize * sizeof(PixelC);
				for(iy = 0; iy < iSize; iy+=8)
				{
					memcpy (ppxlcPred, ppxlcRef, iSz);
					ppxlcRef += m_iFrameWidthY; ppxlcPred += MB_SIZE;
					memcpy (ppxlcPred, ppxlcRef, iSz);
					ppxlcRef += m_iFrameWidthY; ppxlcPred += MB_SIZE;
					memcpy (ppxlcPred, ppxlcRef, iSz);
					ppxlcRef += m_iFrameWidthY; ppxlcPred += MB_SIZE;
					memcpy (ppxlcPred, ppxlcRef, iSz);
					ppxlcRef += m_iFrameWidthY; ppxlcPred += MB_SIZE;
					memcpy (ppxlcPred, ppxlcRef, iSz);
					ppxlcRef += m_iFrameWidthY; ppxlcPred += MB_SIZE;
					memcpy (ppxlcPred, ppxlcRef, iSz);
					ppxlcRef += m_iFrameWidthY; ppxlcPred += MB_SIZE;
					memcpy (ppxlcPred, ppxlcRef, iSz);
					ppxlcRef += m_iFrameWidthY; ppxlcPred += MB_SIZE;
					memcpy (ppxlcPred, ppxlcRef, iSz);
					ppxlcRef += m_iFrameWidthY; ppxlcPred += MB_SIZE;
				}
			}
			else {
				PixelC pxlcT1,pxlcT2, *ppxlcDst;
				const PixelC *ppxlcSrc;
				Int iRndCtrl = 1 - iRoundingControl;
				for (iy = 0; iy < iSize; iy++){
					ppxlcDst = ppxlcPred;
					ppxlcSrc = ppxlcRef;
					for (ix = 0; ix < iSize; ix+=8) {
						ppxlcDst [0] = (ppxlcSrc[0] + (pxlcT1=ppxlcSrc[1]) + iRndCtrl) >> 1;
						ppxlcDst [1] = (pxlcT1 + (pxlcT2=ppxlcSrc[2]) + iRndCtrl) >> 1;
						ppxlcDst [2] = (pxlcT2 + (pxlcT1=ppxlcSrc[3]) + iRndCtrl) >> 1;
						ppxlcDst [3] = (pxlcT1 + (pxlcT2=ppxlcSrc[4]) + iRndCtrl) >> 1;
						ppxlcDst [4] = (pxlcT2 + (pxlcT1=ppxlcSrc[5]) + iRndCtrl) >> 1;
						ppxlcDst [5] = (pxlcT1 + (pxlcT2=ppxlcSrc[6]) + iRndCtrl) >> 1;
						ppxlcDst [6] = (pxlcT2 + (pxlcT1=ppxlcSrc[7]) + iRndCtrl) >> 1;
						ppxlcDst [7] = (pxlcT1 + ppxlcSrc[8] + iRndCtrl) >> 1;
						ppxlcDst += 8;
						ppxlcSrc += 8;
					}
					ppxlcRef += m_iFrameWidthY;
					ppxlcPred += MB_SIZE;
				}
			}
		}
		else {
			if (!(xRef & 1)) {
				Int iRndCtrl = 1 - iRoundingControl;
				const PixelC *ppxlcSrc, *ppxlcSrc2;
				PixelC *ppxlcDst;
				for (iy = 0; iy < iSize; iy++) {
					ppxlcDst = ppxlcPred;
					ppxlcSrc = ppxlcRef;
					ppxlcSrc2 = ppxlcRef + m_iFrameWidthY;
					for (ix = 0; ix < iSize; ix+=8) {
						ppxlcDst [0] = (ppxlcSrc [0] + ppxlcSrc2 [0] + iRndCtrl) >> 1;
						ppxlcDst [1] = (ppxlcSrc [1] + ppxlcSrc2 [1] + iRndCtrl) >> 1;
						ppxlcDst [2] = (ppxlcSrc [2] + ppxlcSrc2 [2] + iRndCtrl) >> 1;
						ppxlcDst [3] = (ppxlcSrc [3] + ppxlcSrc2 [3] + iRndCtrl) >> 1;
						ppxlcDst [4] = (ppxlcSrc [4] + ppxlcSrc2 [4] + iRndCtrl) >> 1;
						ppxlcDst [5] = (ppxlcSrc [5] + ppxlcSrc2 [5] + iRndCtrl) >> 1;
						ppxlcDst [6] = (ppxlcSrc [6] + ppxlcSrc2 [6] + iRndCtrl) >> 1;
						ppxlcDst [7] = (ppxlcSrc [7] + ppxlcSrc2 [7] + iRndCtrl) >> 1;
						ppxlcDst += 8;
						ppxlcSrc += 8;
						ppxlcSrc2 += 8;
					}
					ppxlcRef += m_iFrameWidthY;
					ppxlcPred += MB_SIZE;
				}
			}
			else {
				Int iRndCtrl = 2 - iRoundingControl;
				PixelC pxlcT1, pxlcT2, pxlcT3, pxlcT4, *ppxlcDst;
				const PixelC *ppxlcSrc, *ppxlcSrc2;
				for (iy = 0; iy < iSize; iy++) {
					ppxlcDst = ppxlcPred;
					ppxlcSrc = ppxlcRef;
					ppxlcSrc2 = ppxlcRef + m_iFrameWidthY;
					for (ix = 0; ix < iSize; ix+=8) {
						ppxlcDst [0] = (ppxlcSrc[0] + (pxlcT1=ppxlcSrc[1]) + ppxlcSrc2[0] + (pxlcT3=ppxlcSrc2[1]) + iRndCtrl) >> 2;
						ppxlcDst [1] = (pxlcT1 + (pxlcT2=ppxlcSrc[2]) + pxlcT3 + (pxlcT4=ppxlcSrc2[2]) + iRndCtrl) >> 2;
						ppxlcDst [2] = (pxlcT2 + (pxlcT1=ppxlcSrc[3]) + pxlcT4 + (pxlcT3=ppxlcSrc2[3]) + iRndCtrl) >> 2;
						ppxlcDst [3] = (pxlcT1 + (pxlcT2=ppxlcSrc[4]) + pxlcT3 + (pxlcT4=ppxlcSrc2[4]) + iRndCtrl) >> 2;
						ppxlcDst [4] = (pxlcT2 + (pxlcT1=ppxlcSrc[5]) + pxlcT4 + (pxlcT3=ppxlcSrc2[5]) + iRndCtrl) >> 2;
						ppxlcDst [5] = (pxlcT1 + (pxlcT2=ppxlcSrc[6]) + pxlcT3 + (pxlcT4=ppxlcSrc2[6]) + iRndCtrl) >> 2;
						ppxlcDst [6] = (pxlcT2 + (pxlcT1=ppxlcSrc[7]) + pxlcT4 + (pxlcT3=ppxlcSrc2[7]) + iRndCtrl) >> 2;
						ppxlcDst [7] = (pxlcT1 + ppxlcSrc[8] + pxlcT3 + ppxlcSrc2[8] + iRndCtrl) >> 2;
						ppxlcDst += 8;
						ppxlcSrc += 8;
						ppxlcSrc2 += 8;
					}
					ppxlcRef += m_iFrameWidthY;
					ppxlcPred += MB_SIZE;
				}
			}
		}
	}
	else {
		if (!(yRef & 1)) {
			if (!(xRef & 1)) {  //!bXSubPxl && !bYSubPxl
				for (iy = 0; iy < iSize; iy++) {
					memcpy (ppxlcPred, ppxlcRef, iSize*sizeof(PixelC));
					ppxlcRef += m_iFrameWidthY;
// RRV modification
					ppxlcPred += (m_vopmd.RRVmode.iRRVOnOff == 1) ? (iSize) : (MB_SIZE);
//					ppxlcPred += MB_SIZE;
// ~RRV
				}
			}
			else {  //bXSubPxl && !bYSubPxl
				for (iy = 0; iy < iSize; iy++){
					for (ix = 0; ix < iSize; ix++)
						ppxlcPred [ix] = (ppxlcRef [ix] + ppxlcRef [ix + 1] + 1 - iRoundingControl) >> 1;
					ppxlcRef += m_iFrameWidthY;
// RRV modification
					ppxlcPred += (m_vopmd.RRVmode.iRRVOnOff == 1) ? (iSize) : (MB_SIZE);
//					ppxlcPred += MB_SIZE;
// ~RRV
				}
			}
		}
		else {
			const PixelC* ppxlcRefBot;
			if (!(xRef & 1)) {  //!bXSubPxl&& bYSubPxl
				for (iy = 0; iy < iSize; iy++) {
					ppxlcRefBot = ppxlcRef + m_iFrameWidthY;		//UPln -> pixels (xInt,yInt+1);
					for (ix = 0; ix < iSize; ix++) 
						ppxlcPred [ix] = (ppxlcRef [ix] + ppxlcRefBot [ix] + 1 - iRoundingControl) >> 1;
					ppxlcRef = ppxlcRefBot;
// RRV modification
					ppxlcPred += (m_vopmd.RRVmode.iRRVOnOff == 1) ? (iSize) : (MB_SIZE);
//					ppxlcPred += MB_SIZE;
// ~RRV
				}
			}
			else { // bXSubPxl && bYSubPxl
				for (iy = 0; iy < iSize; iy++) {
					ppxlcRefBot = ppxlcRef + m_iFrameWidthY;		//UPln -> pixels (xInt,yInt+1);
					for (ix = 0; ix < iSize; ix++){
						ppxlcPred [ix] = (
							ppxlcRef [ix + 1] + ppxlcRef [ix] +
							ppxlcRefBot [ix + 1] + ppxlcRefBot [ix] + 2 - iRoundingControl
						) >> 2;

					}
					ppxlcRef = ppxlcRefBot;
// RRV modification
					ppxlcPred += (m_vopmd.RRVmode.iRRVOnOff == 1) ? (iSize) : (MB_SIZE);
//					ppxlcPred += MB_SIZE;
// ~RRV
				}
			}
		}
	}
}

Void CVideoObject::motionCompQuarterSample (
	PixelC* ppxlcPred, // can be either Y or A
	const PixelC* ppxlcRefLeftTop, // point to left-top of the frame
	Int iSize, // either MB or BLOCK size
	CoordI xRef, CoordI yRef, // x + mvX in quarter pel unit
	Int iRoundingControl, // rounding control
	CRct *prctMVLimit // extended bounding box
    )
{
	CoordI ix, iy;
    U8 *ppxlcblk;
    
	limitMVRangeToExtendedBBQuarterPel(xRef,yRef,prctMVLimit,iSize);
    
    Int blkSizeX, blkSizeY;
    if (iSize == 0) { // INTERLACED
      blkSizeX = 16;
      blkSizeY = 8;
      
      ppxlcblk = (U8*) calloc(blkSizeX*blkSizeX,sizeof(U8));
      blkInterpolateY (ppxlcRefLeftTop,iSize,xRef,yRef,ppxlcblk,iRoundingControl);
      
      for (iy = 0; iy < blkSizeX; iy++) { 
        if (!(iy&1)) 
          for (ix = 0; ix < blkSizeX; ix++) { 
            ppxlcPred [ix] = *(ppxlcblk+ix+iy*blkSizeX);
          }
        ppxlcPred += blkSizeX;
      }
    } //~INTERLACED
    else { 
      blkSizeX = iSize;
      blkSizeY = iSize;
      
      ppxlcblk = (U8*) calloc(blkSizeX*blkSizeX,sizeof(U8));
      blkInterpolateY (ppxlcRefLeftTop,iSize,xRef,yRef,ppxlcblk,iRoundingControl);
      
      for (iy = 0; iy < blkSizeX; iy++) { 
        for (ix = 0; ix < blkSizeX; ix++) { 
          ppxlcPred [ix] = *(ppxlcblk+ix+iy*blkSizeX);
        }
        ppxlcPred += MB_SIZE;
      }
    }

    free(ppxlcblk);
}

Void CVideoObject::motionCompDirectMode(                // Interlaced direct mode
    CoordI x, CoordI y,
    CMBMode *pmbmd,
    const CMotionVector *pmvRef,
    CRct *prctMVLimitFwd, CRct *prctMVLimitBak,
	Int plane	// plane=1 for grey scale, plane=0 for texture, 02-17-99

                                        )
{
	Int* rgiMvRound = NULL;
	UInt uiDivisor = 0;
	Int xRefUVF,yRefUVF,xRefUVB,yRefUVB;

// begin of new changes 10/21/98
	Int iMBX,iMBY;
	const CMBMode *pmbmdRef; 

	if(m_volmd.fAUsage != RECTANGLE)
	{
		iMBX=(x-m_rctCurrVOPY.left)/MB_SIZE;
		iMBY=(y-m_rctCurrVOPY.top)/MB_SIZE;
		pmbmdRef= m_rgmbmdRef +
				(iMBX+iMBY*m_iNumMBXRef);
	}
	else
	{
		iMBX=x/MB_SIZE;
		iMBY=y/MB_SIZE;
		pmbmdRef= m_rgmbmdRef +
				iMBX+iMBY*m_iNumMBXRef;
	}
// end of new changes 10/21/98

  if (pmbmdRef->m_rgTranspStatus[0]==ALL) {
    static CMotionVector mvZero[5];
    pmvRef = mvZero;
  }

	if ((iMBX<m_iNumMBXRef && iMBX>=0 && iMBY<m_iNumMBYRef && iMBY>=0)&& // new change 10/21/98
		(pmbmdRef->m_bFieldMV&&pmbmdRef->m_rgTranspStatus[0]!=ALL)) {
      static I8 iTROffsetTop[] = {  0, 0,  1, 1, 0, 0, -1, -1 };
      static I8 iTROffsetBot[] = { -1, 0, -1, 0, 1, 0,  1,  0 };
      CoordI iXFwdTop, iXFwdBot, iXBakTop, iXBakBot;
      CoordI iYFwdTop, iYFwdBot, iYBakTop, iYBakBot;
      CoordI iXFTQSUV = 0, iXFBQSUV = 0, iXBTQSUV = 0, iXBBQSUV = 0;
      CoordI iYFTQSUV = 0, iYFBQSUV = 0, iYBTQSUV = 0, iYBBQSUV = 0;
      const CMotionVector *pmvRefTop, *pmvRefBot;
      Int iTopRefFldOffset = 0, iBotRefFldOffset = 0;
      Int iCode = (Int)(vopmd().bTopFieldFirst) << 2;
      
      pmbmd->m_bFieldMV = 1;  // set field direct mode for grey scale, Krit 02-17-99

      assert((pmbmdRef->m_dctMd != INTRA) && (pmbmdRef->m_dctMd != INTRAQ));
      if (pmbmdRef->m_bForwardTop) {
        iCode |= 2;
        iTopRefFldOffset = 1;
        pmvRefTop = pmvRef + 6;
      } else
        pmvRefTop = pmvRef + 5;
      if (pmbmdRef->m_bForwardBottom) {
        iCode |= 1;
        iBotRefFldOffset = 1;
        pmvRefBot = pmvRef + 8;
      } else
        pmvRefBot = pmvRef + 7;
      Int iTempRefDTop = 2*(m_tFutureRef - m_tPastRef) + iTROffsetTop[iCode];
      Int iTempRefDBot = 2*(m_tFutureRef - m_tPastRef) + iTROffsetBot[iCode];
      Int iTempRefBTop = 2*(m_t          - m_tPastRef) + iTROffsetTop[iCode];
      Int iTempRefBBot = 2*(m_t          - m_tPastRef) + iTROffsetBot[iCode];
      
      assert(iTempRefDTop > 0); assert(iTempRefDBot > 0); assert(iTempRefBTop > 0); assert(iTempRefBBot > 0);
      
      // Find MVs for the top field
      iXFwdTop = (pmvRefTop->m_vctTrueHalfPel.x * iTempRefBTop) / iTempRefDTop + pmbmd->m_vctDirectDeltaMV.x;
      iYFwdTop = (pmvRefTop->m_vctTrueHalfPel.y * iTempRefBTop) / iTempRefDTop + pmbmd->m_vctDirectDeltaMV.y;
      iXBakTop = pmbmd->m_vctDirectDeltaMV.x ? (iXFwdTop - pmvRefTop->m_vctTrueHalfPel.x) :
        ((pmvRefTop->m_vctTrueHalfPel.x * (iTempRefBTop - iTempRefDTop)) / iTempRefDTop);
      iYBakTop = pmbmd->m_vctDirectDeltaMV.y ? (iYFwdTop - pmvRefTop->m_vctTrueHalfPel.y) :
        ((pmvRefTop->m_vctTrueHalfPel.y * (iTempRefBTop - iTempRefDTop)) / iTempRefDTop);
      
      // Find MVs for the bottom field
      iXFwdBot = (pmvRefBot->m_vctTrueHalfPel.x * iTempRefBBot) / iTempRefDBot + pmbmd->m_vctDirectDeltaMV.x;
      iYFwdBot = (pmvRefBot->m_vctTrueHalfPel.y * iTempRefBBot) / iTempRefDBot + pmbmd->m_vctDirectDeltaMV.y;
      iXBakBot = pmbmd->m_vctDirectDeltaMV.x ? (iXFwdBot - pmvRefBot->m_vctTrueHalfPel.x) :
        ((pmvRefBot->m_vctTrueHalfPel.x * (iTempRefBBot - iTempRefDBot)) / iTempRefDBot);
      iYBakBot = pmbmd->m_vctDirectDeltaMV.y ? (iYFwdBot - pmvRefBot->m_vctTrueHalfPel.y) :
        ((pmvRefBot->m_vctTrueHalfPel.y * (iTempRefBBot - iTempRefDBot)) / iTempRefDBot);
    
      // Motion compensate the top field forward
      if (m_volmd.bQuarterSample) {
        iXFTQSUV = iXFwdTop/2; 
        iYFTQSUV = iYFwdTop/2;
        iXBTQSUV = iXBakTop/2; 
        iYBTQSUV = iYBakTop/2;

        iXFBQSUV = iXFwdBot/2;
        iYFBQSUV = iYFwdBot/2;
        iXBBQSUV = iXBakBot/2; 
        iYBBQSUV = iYBakBot/2;

        iYFwdTop = iYFwdTop & ~1;
        iYBakTop = iYBakTop & ~1;
        iYFwdBot = iYFwdBot & ~1;
        iYBakBot = iYBakBot & ~1;
    
        iXFwdTop += 4*x; iYFwdTop += 4*y;
//        limitMVRangeToExtendedBBQuarterPel(iXFwdTop, iYFwdTop, prctMVLimitFwd, MB_SIZE); // deleted by Y.Suzuki for the extended bounding box support
		if (plane==0) {  // texture MC
          motionCompQuarterSample(m_ppxlcPredMBY, 
                                  m_pvopcRefQ0->pixelsY() + iTopRefFldOffset * m_iFrameWidthY, 0, 
                                  iXFwdTop, iYFwdTop, m_vopmd.iRoundingControl, prctMVLimitFwd);
          
//          iXFTQSUV += 2*x; iYFTQSUV += 2*y; // deleted by Y.Suzuki for the extended bounding box support
//          limitMVRangeToExtendedBBHalfPel(iXFTQSUV, iYFTQSUV, prctMVLimitFwd, MB_SIZE); // deleted by Y.Suzuki for the extended bounding box support
//          iXFTQSUV -= 2*x; iYFTQSUV -= 2*y; // deleted by Y.Suzuki for the extended bounding box support
          motionCompFieldUV(m_ppxlcPredMBU, m_ppxlcPredMBV, m_pvopcRefQ0, x, y,
                            (iXFTQSUV & 3) ? ((iXFTQSUV >> 1) | 1) : (iXFTQSUV >> 1),
                            (iYFTQSUV & 6) ? ((iYFTQSUV >> 1) | 2) : (iYFTQSUV >> 1), 
                            iTopRefFldOffset,
                            prctMVLimitFwd); // added by Y.Suzuki for the extended bounding box support
		}
		else {  // plane=1, grey scale MC
          for(Int iAuxComp=0;iAuxComp<m_volmd.iAuxCompCount;iAuxComp++) { // MAC (SB) 26-Nov-99
            motionCompQuarterSample(m_ppxlcPredMBA[iAuxComp], 
              m_pvopcRefQ0->pixelsA(iAuxComp) + iTopRefFldOffset * m_iFrameWidthY, 0, 
              iXFwdTop, iYFwdTop, m_vopmd.iRoundingControl, prctMVLimitFwd);
          }
		}
      }
      else {
          iXFwdTop += 2*x; iYFwdTop += 2*y;
//        limitMVRangeToExtendedBBHalfPel(iXFwdTop, iYFwdTop, prctMVLimitFwd, MB_SIZE); // deleted by Y.Suzuki for the extended bounding box support
		if (plane==0) {  // texture MC
          motionCompYField(m_ppxlcPredMBY,
                           m_pvopcRefQ0->pixelsY() + iTopRefFldOffset * m_iFrameWidthY, iXFwdTop, iYFwdTop,
                           prctMVLimitFwd); // added by Y.Suzuki for the extended bounding box support
          iXFwdTop -= 2*x; iYFwdTop -= 2*y;
          motionCompFieldUV(m_ppxlcPredMBU, m_ppxlcPredMBV, m_pvopcRefQ0, x, y,
                            (iXFwdTop & 3) ? ((iXFwdTop >> 1) | 1) : (iXFwdTop >> 1),
                            (iYFwdTop & 6) ? ((iYFwdTop >> 1) | 2) : (iYFwdTop >> 1), iTopRefFldOffset,
                            prctMVLimitFwd); // added by Y.Suzuki for the extended bounding box support
		}
		else {  // plane=1, grey scale MC
          for(Int iAuxComp=0;iAuxComp<m_volmd.iAuxCompCount;iAuxComp++) { // MAC (SB) 26-Nov-99
            motionCompYField(m_ppxlcPredMBA[iAuxComp],
                           m_pvopcRefQ0->pixelsA(iAuxComp) + iTopRefFldOffset * m_iFrameWidthY, iXFwdTop, iYFwdTop,
						   prctMVLimitFwd); // added by Y.Suzuki for the extended bounding box support
          }

          iXFwdTop -= 2*x; iYFwdTop -= 2*y;
		}
      }
    

      // Motion compensate the top field backward
    if (m_volmd.bQuarterSample) {
      iXBakTop += 4*x; iYBakTop += 4*y;
//      limitMVRangeToExtendedBBQuarterPel(iXBakTop, iYBakTop, prctMVLimitFwd, MB_SIZE); // deleted by Y.Suzuki for the extended bounding box support
      if (plane==0) {  // texture MC
        motionCompQuarterSample(m_ppxlcPredMBBackY, 
                                m_pvopcRefQ1->pixelsY(), 0, 
                                iXBakTop, iYBakTop, m_vopmd.iRoundingControl, prctMVLimitBak);
        
//        iXBTQSUV += 2*x; iYBTQSUV += 2*y; // deleted by Y.Suzuki for the extended bounding box support
//        limitMVRangeToExtendedBBHalfPel(iXBTQSUV, iYBTQSUV, prctMVLimitBak, MB_SIZE); // deleted by Y.Suzuki for the extended bounding box support
//        iXBTQSUV -= 2*x; iYBTQSUV -= 2*y; // deleted by Y.Suzuki for the extended bounding box support
        motionCompFieldUV(m_ppxlcPredMBBackU, m_ppxlcPredMBBackV, m_pvopcRefQ1, x, y,
                          (iXBTQSUV & 3) ? ((iXBTQSUV >> 1) | 1) : (iXBTQSUV >> 1),
                          (iYBTQSUV & 6) ? ((iYBTQSUV >> 1) | 2) : (iYBTQSUV >> 1), 
                          0,
                          prctMVLimitBak); // added by Y.Suzuki for the extended bounding box support
      }
      else {  // plane=1, grey scale MC
        for(Int iAuxComp=0;iAuxComp<m_volmd.iAuxCompCount;iAuxComp++) { // MAC (SB) 26-Nov-99
          motionCompQuarterSample(m_ppxlcPredMBBackA[iAuxComp], 
                                  m_pvopcRefQ1->pixelsA(iAuxComp), 0, 
                                  iXBakTop, iYBakTop, m_vopmd.iRoundingControl, prctMVLimitBak);
        }
	  }
    }
    else {
      iXBakTop += 2*x; iYBakTop += 2*y;
//      limitMVRangeToExtendedBBHalfPel(iXBakTop, iYBakTop, prctMVLimitBak, MB_SIZE); // deleted by Y.Suzuki for the extended bounding box support
      if (plane==0) {  // texture MC
        motionCompYField(m_ppxlcPredMBBackY, m_pvopcRefQ1->pixelsY(), iXBakTop,  iYBakTop,
			prctMVLimitBak); // added by Y.Suzuki for the extended bounding box support
        iXBakTop -= 2*x; iYBakTop -= 2*y;
        motionCompFieldUV(m_ppxlcPredMBBackU, m_ppxlcPredMBBackV, m_pvopcRefQ1, x, y,
                          (iXBakTop & 3) ? ((iXBakTop >> 1) | 1) : (iXBakTop >> 1),
                          (iYBakTop & 6) ? ((iYBakTop >> 1) | 2) : (iYBakTop >> 1), 0,
			   prctMVLimitBak); // added by Y.Suzuki for the extended bounding box support
      }
      else {  // plane=1, grey scale MC
        for(Int iAuxComp=0;iAuxComp<m_volmd.iAuxCompCount;iAuxComp++) { // MAC (SB) 26-Nov-99
          motionCompYField(m_ppxlcPredMBBackA[iAuxComp], m_pvopcRefQ1->pixelsA(iAuxComp), iXBakTop,  iYBakTop,
			  prctMVLimitBak); // added by Y.Suzuki for the extended bounding box support
        }
        iXBakTop -= 2*x; iYBakTop -= 2*y;
      }
    }
    

    // Motion compensate the bottom field forward
    if (m_volmd.bQuarterSample) {
      iXFwdBot += 4*x; iYFwdBot += 4*y;
//      limitMVRangeToExtendedBBQuarterPel(iXFwdBot, iYFwdBot, prctMVLimitFwd, MB_SIZE); // deleted by Y.Suzuki for the extended bounding box support
      if (plane==0) {  // texture MC
        motionCompQuarterSample(m_ppxlcPredMBY + MB_SIZE, 
                                m_pvopcRefQ0->pixelsY() + iBotRefFldOffset * m_iFrameWidthY, 0, 
                                iXFwdBot, iYFwdBot, m_vopmd.iRoundingControl, prctMVLimitFwd);
        
//        iXFBQSUV += 2*x; iYFBQSUV += 2*y; // deleted by Y.Suzuki for the extended bounding box support
//        limitMVRangeToExtendedBBHalfPel(iXFBQSUV, iYFBQSUV, prctMVLimitFwd, MB_SIZE); // delted by Y.Suzuki for the extended bounding box support
//        iXFBQSUV -= 2*x; iYFBQSUV -= 2*y; // deleted by Y.Suzuki for the extended bounding box support
        motionCompFieldUV(
                          m_ppxlcPredMBU + BLOCK_SIZE, m_ppxlcPredMBV + BLOCK_SIZE, 
                          m_pvopcRefQ0, x, y,
                          (iXFBQSUV & 3) ? ((iXFBQSUV >> 1) | 1) : (iXFBQSUV >> 1),
                          (iYFBQSUV & 6) ? ((iYFBQSUV >> 1) | 2) : (iYFBQSUV >> 1), 
                          iBotRefFldOffset,
                          prctMVLimitFwd); // added by Y.Suzuki for the extended bounding box support
      }
      else {  // plane=1, grey scale MC
        for(Int iAuxComp=0;iAuxComp<m_volmd.iAuxCompCount;iAuxComp++) { // MAC (SB) 26-Nov-99
          motionCompQuarterSample(
                                  m_ppxlcPredMBA[iAuxComp] + MB_SIZE, 
                                  m_pvopcRefQ0->pixelsA(iAuxComp) + iBotRefFldOffset * m_iFrameWidthY, 0, 
                                  iXFwdBot, iYFwdBot, m_vopmd.iRoundingControl, prctMVLimitFwd);
        }
	  }
    }
    else {
      iXFwdBot += 2*x; iYFwdBot += 2*y;
//      limitMVRangeToExtendedBBHalfPel(iXFwdBot, iYFwdBot, prctMVLimitFwd, MB_SIZE); // deleted by Y.Suzuki for the extended bounding box support
      if (plane==0) {  // texture MC
        motionCompYField(m_ppxlcPredMBY + MB_SIZE,
                         m_pvopcRefQ0->pixelsY() + iBotRefFldOffset * m_iFrameWidthY, iXFwdBot, iYFwdBot,
                         prctMVLimitFwd); // added by Y.Suzuki for the extended bounding box support
        iXFwdBot -= 2*x; iYFwdBot -= 2*y;
        motionCompFieldUV(m_ppxlcPredMBU + BLOCK_SIZE, m_ppxlcPredMBV + BLOCK_SIZE, m_pvopcRefQ0, x, y,
                          (iXFwdBot & 3) ? ((iXFwdBot >> 1) | 1) : (iXFwdBot >> 1),
                          (iYFwdBot & 6) ? ((iYFwdBot >> 1) | 2) : (iYFwdBot >> 1), iBotRefFldOffset,
                           prctMVLimitFwd); // added by Y.Suzuki for the extended bounding box support
      }
      else {  // plane=1, grey scale MC
        for(Int iAuxComp=0;iAuxComp<m_volmd.iAuxCompCount;iAuxComp++) { // MAC (SB) 26-Nov-99
          motionCompYField(m_ppxlcPredMBA[iAuxComp] + MB_SIZE,
                           m_pvopcRefQ0->pixelsA(iAuxComp) + iBotRefFldOffset * m_iFrameWidthY, iXFwdBot, iYFwdBot,
						   prctMVLimitFwd); // added by Y.Suzuki for the extended bounding box support
        }
	    iXFwdBot -= 2*x; iYFwdBot -= 2*y;
      }
    }
    

    // Motion compensate the bottom field backward
    if (m_volmd.bQuarterSample) {
      iXBakBot += 4*x; iYBakBot += 4*y;
//      limitMVRangeToExtendedBBQuarterPel(iXBakBot, iYBakBot, prctMVLimitFwd, MB_SIZE); // deleted by Y.Suzuki for the extended bounding box support
      if (plane==0) {  // texture MC
        motionCompQuarterSample(m_ppxlcPredMBBackY + MB_SIZE, 
                                m_pvopcRefQ1->pixelsY() + m_iFrameWidthY, 0, 
                                iXBakBot, iYBakBot, m_vopmd.iRoundingControl, prctMVLimitBak); // 991201 mwi, chg Fwd->Bak

//        iXBBQSUV += 2*x; iYBBQSUV += 2*y; // deleted by Y.Suzuki for the extended bounding box support
//        limitMVRangeToExtendedBBHalfPel(iXBBQSUV, iYBBQSUV, prctMVLimitBak, MB_SIZE); // deleted by Y.Suzuki for the extended bounding box support
//        iXBBQSUV -= 2*x; iYBBQSUV -= 2*y; // deleted by Y.Suzuki for the extended bounding box support
        motionCompFieldUV(m_ppxlcPredMBBackU + BLOCK_SIZE, 
                          m_ppxlcPredMBBackV + BLOCK_SIZE, m_pvopcRefQ1, x, y,
                          (iXBBQSUV & 3) ? ((iXBBQSUV >> 1) | 1) : (iXBBQSUV >> 1),
                          (iYBBQSUV & 6) ? ((iYBBQSUV >> 1) | 2) : (iYBBQSUV >> 1), 
                          1,
                          prctMVLimitBak); // added by Y.Suzuki for the extended bounding box support // 991201 mwi, chg Fwd->Bak
      }
      else {  // plane=1, grey scale MC
        for(Int iAuxComp=0;iAuxComp<m_volmd.iAuxCompCount;iAuxComp++) { // MAC (SB) 26-Nov-99
          motionCompQuarterSample(m_ppxlcPredMBBackA[iAuxComp] + MB_SIZE, 
                                  m_pvopcRefQ1->pixelsA(iAuxComp) + m_iFrameWidthY, 0, 
                                  iXBakBot, iYBakBot, m_vopmd.iRoundingControl, prctMVLimitBak); // 991201 mwi, chg Fwd->Bak
        }
      }
    }
    else {
      iXBakBot += 2*x; iYBakBot += 2*y;
//      limitMVRangeToExtendedBBHalfPel(iXBakBot, iYBakBot, prctMVLimitBak, MB_SIZE); // deleted by Y.Suzuki for the extended bounding box support
      if (plane==0) {  // texture MC
        motionCompYField(m_ppxlcPredMBBackY + MB_SIZE, m_pvopcRefQ1->pixelsY() + m_iFrameWidthY,
                         iXBakBot, iYBakBot,
                         prctMVLimitBak); // added by Y.Suzuki for the extended bounding box support
        iXBakBot -= 2*x; iYBakBot -= 2*y;
        motionCompFieldUV(m_ppxlcPredMBBackU + BLOCK_SIZE, m_ppxlcPredMBBackV + BLOCK_SIZE, m_pvopcRefQ1, x, y,
                          (iXBakBot & 3) ? ((iXBakBot >> 1) | 1) : (iXBakBot >> 1),
                          (iYBakBot & 6) ? ((iYBakBot >> 1) | 2) : (iYBakBot >> 1), 1,
                          prctMVLimitBak); // added by Y.Suzuki for the extended bounding box support
      }
      else {  // plane=1, grey scale MC
        for(Int iAuxComp=0;iAuxComp<m_volmd.iAuxCompCount;iAuxComp++) { // MAC (SB) 26-Nov-99
          motionCompYField(m_ppxlcPredMBBackA[iAuxComp] + MB_SIZE, m_pvopcRefQ1->pixelsA(iAuxComp) + m_iFrameWidthY,
                           iXBakBot, iYBakBot,
						   prctMVLimitBak); // added by Y.Suzuki for the extended bounding box support
        }
		iXBakBot -= 2*x; iYBakBot -= 2*y;
      }
    }
    
  } else {

    Int iTempRefD = m_tFutureRef - m_tPastRef;
    Int iTempRefB = m_t          - m_tPastRef;
    assert(iTempRefD > 0); assert(iTempRefB > 0);
    Int iChromaFwdX = 0, iChromaFwdY = 0, iChromaBakX = 0, iChromaBakY = 0;
    CVector vctFwd, vctBak;
    static I8 iBlkXOffset[] = { 0, 2*BLOCK_SIZE, 0, 2*BLOCK_SIZE };
    static I8 iBlkYOffset[] = { 0, 0, 2*BLOCK_SIZE, 2*BLOCK_SIZE };
    static Int iMBOffset[] = { 0, BLOCK_SIZE, MB_SIZE*BLOCK_SIZE, MB_SIZE*BLOCK_SIZE + BLOCK_SIZE };
    if ((pmbmdRef->m_dctMd == INTRA) || (pmbmdRef->m_dctMd == INTRAQ)) {
      static CMotionVector mvZero[5];
      pmvRef = mvZero;
    }
    if(iMBX<m_iNumMBXRef && iMBX>=0 && iMBY<m_iNumMBYRef && iMBY>=0) // new changes 10/21/98
      {
		if (pmbmdRef -> m_bhas4MVForward || m_volmd.bQuarterSample)	{
          for (Int iBlk = 0; iBlk < 4; iBlk++) {
            if(pmbmd->m_rgTranspStatus[iBlk+1]!=ALL) {
              vctFwd = (pmvRef[iBlk + 1].m_vctTrueHalfPel * iTempRefB) / iTempRefD + pmbmd->m_vctDirectDeltaMV;
              vctBak.x = pmbmd->m_vctDirectDeltaMV.x ? (vctFwd.x - pmvRef[iBlk + 1].m_vctTrueHalfPel.x) :
                ((pmvRef[iBlk + 1].m_vctTrueHalfPel.x * (iTempRefB - iTempRefD)) / iTempRefD);
              vctBak.y = pmbmd->m_vctDirectDeltaMV.y ? (vctFwd.y - pmvRef[iBlk + 1].m_vctTrueHalfPel.y) :
                ((pmvRef[iBlk + 1].m_vctTrueHalfPel.y * (iTempRefB - iTempRefD)) / iTempRefD);
              if (m_volmd.bQuarterSample) {
                motionCompQuarterSample(m_ppxlcPredMBY + iMBOffset[iBlk], 
                                        m_pvopcRefQ0->pixelsY(), BLOCK_SIZE, 
                                        x * 4 + iBlkXOffset[iBlk] * 2 + vctFwd.x, 
                                        y * 4 + iBlkYOffset[iBlk] * 2 + vctFwd.y, m_vopmd.iRoundingControl, prctMVLimitFwd);
                motionCompQuarterSample(m_ppxlcPredMBBackY + iMBOffset[iBlk], 
                                        m_pvopcRefQ1->pixelsY(), BLOCK_SIZE,
                                        x * 4 + iBlkXOffset[iBlk] * 2 + vctBak.x,
                                        y * 4 + iBlkYOffset[iBlk] * 2 + vctBak.y, m_vopmd.iRoundingControl, prctMVLimitBak);
              }
              else {
                motionComp(m_ppxlcPredMBY + iMBOffset[iBlk], m_pvopcRefQ0->pixelsY(), BLOCK_SIZE,
                           x * 2 + iBlkXOffset[iBlk] + vctFwd.x, y * 2 + iBlkYOffset[iBlk] + vctFwd.y, m_vopmd.iRoundingControl, prctMVLimitFwd);
                motionComp(m_ppxlcPredMBBackY + iMBOffset[iBlk], m_pvopcRefQ1->pixelsY(), BLOCK_SIZE,
                           x * 2 + iBlkXOffset[iBlk] + vctBak.x, y * 2 + iBlkYOffset[iBlk] + vctBak.y, m_vopmd.iRoundingControl, prctMVLimitBak);
              }
              
              if (m_volmd.bQuarterSample) {
                iChromaFwdX += vctFwd.x/2;
                iChromaFwdY += vctFwd.y/2;
                iChromaBakX += vctBak.x/2;
                iChromaBakY += vctBak.y/2;
              }
              else {	
                iChromaFwdX += vctFwd.x;
                iChromaFwdY += vctFwd.y;
                iChromaBakX += vctBak.x;
                iChromaBakY += vctBak.y;
              }
              uiDivisor += 4;
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
          xRefUVF = sign (iChromaFwdX) * (rgiMvRound [abs (iChromaFwdX) % uiDivisor] + (abs (iChromaFwdX) / uiDivisor) * 2);
          yRefUVF = sign (iChromaFwdY) * (rgiMvRound [abs (iChromaFwdY) % uiDivisor] + (abs (iChromaFwdY) / uiDivisor) * 2);
          xRefUVB = sign (iChromaBakX) * (rgiMvRound [abs (iChromaBakX) % uiDivisor] + (abs (iChromaBakX) / uiDivisor) * 2);
          yRefUVB = sign (iChromaBakY) * (rgiMvRound [abs (iChromaBakY) % uiDivisor] + (abs (iChromaBakY) / uiDivisor) * 2);
        }
        else {
          vctFwd = (pmvRef[0].m_vctTrueHalfPel * iTempRefB) / iTempRefD + pmbmd->m_vctDirectDeltaMV;
          vctBak.x = pmbmd->m_vctDirectDeltaMV.x ? (vctFwd.x - pmvRef[0].m_vctTrueHalfPel.x) :
            ((pmvRef[0].m_vctTrueHalfPel.x * (iTempRefB - iTempRefD)) / iTempRefD);
          vctBak.y = pmbmd->m_vctDirectDeltaMV.y ? (vctFwd.y - pmvRef[0].m_vctTrueHalfPel.y) :
            ((pmvRef[0].m_vctTrueHalfPel.y * (iTempRefB - iTempRefD)) / iTempRefD);
          if (m_volmd.bQuarterSample) {
            // mod 991201 mwi
            for (Int iBlk = 0; iBlk < 4; iBlk++) {
              motionCompQuarterSample(m_ppxlcPredMBY + iMBOffset[iBlk], 
                                      m_pvopcRefQ0->pixelsY(), BLOCK_SIZE,
                                      x * 4 + iBlkXOffset[iBlk] * 2 + vctFwd.x, 
                                      y * 4 + iBlkYOffset[iBlk] * 2 + vctFwd.y, 
                                      m_vopmd.iRoundingControl, prctMVLimitFwd);
              motionCompQuarterSample(m_ppxlcPredMBBackY + iMBOffset[iBlk] , 
                                      m_pvopcRefQ1->pixelsY(), BLOCK_SIZE,
                                      x * 4 + iBlkXOffset[iBlk] * 2 + vctBak.x, 
                                      y * 4 + iBlkYOffset[iBlk] * 2 + vctBak.y, 
                                      m_vopmd.iRoundingControl, prctMVLimitBak);
            }
            // ~mod 991201 mwi
          }
          
          else {
            motionComp(m_ppxlcPredMBY, m_pvopcRefQ0->pixelsY(), MB_SIZE,
                       x * 2 +  vctFwd.x, y * 2 +  vctFwd.y, m_vopmd.iRoundingControl, prctMVLimitFwd);
            motionComp(m_ppxlcPredMBBackY , m_pvopcRefQ1->pixelsY(), MB_SIZE,
                       x * 2 +  vctBak.x, y * 2 +  vctBak.y, m_vopmd.iRoundingControl, prctMVLimitBak);
          }
          
          if (m_volmd.bQuarterSample) {
            iChromaFwdX = vctFwd.x/2;
            iChromaFwdY = vctFwd.y/2;
            iChromaBakX = vctBak.x/2;
            iChromaBakY = vctBak.y/2;
          }
          else {	
            iChromaFwdX = vctFwd.x;
            iChromaFwdY = vctFwd.y;
            iChromaBakX = vctBak.x;
            iChromaBakY = vctBak.y;
          }
          xRefUVF = sign (iChromaFwdX) * (grgiMvRound4  [abs (iChromaFwdX) % 4] + (abs (iChromaFwdX) / 4) * 2);
          yRefUVF = sign (iChromaFwdY) * (grgiMvRound4  [abs (iChromaFwdY) % 4] + (abs (iChromaFwdY) / 4) * 2);
          xRefUVB = sign (iChromaBakX) * (grgiMvRound4  [abs (iChromaBakX) % 4] + (abs (iChromaBakX) / 4) * 2);
          yRefUVB = sign (iChromaBakY) * (grgiMvRound4  [abs (iChromaBakY) % 4] + (abs (iChromaBakY) / 4) * 2);
        }
      }
    // begin of new changes 10/21/98
    else
      {
        vctFwd = pmbmd->m_vctDirectDeltaMV;
        vctBak.x = pmbmd->m_vctDirectDeltaMV.x ? vctFwd.x :0;
        vctBak.y = pmbmd->m_vctDirectDeltaMV.y ? vctFwd.y :0;
        if (m_volmd.bQuarterSample) { // Quarter Sample, mwi
          // mod 991201 mwi
          for (Int iBlk = 0; iBlk < 4; iBlk++) {
            motionCompQuarterSample(m_ppxlcPredMBY + iMBOffset[iBlk], 
                                    m_pvopcRefQ0->pixelsY(), BLOCK_SIZE,
                                    x * 4 + iBlkXOffset[iBlk] * 2 + vctFwd.x, 
                                    y * 4 + iBlkYOffset[iBlk] * 2 + vctFwd.y, 
                                    m_vopmd.iRoundingControl, prctMVLimitFwd);
            motionCompQuarterSample(m_ppxlcPredMBBackY + iMBOffset[iBlk], 
                                    m_pvopcRefQ1->pixelsY(), BLOCK_SIZE,
                                    x * 4 + iBlkXOffset[iBlk] * 2 + vctBak.x, 
                                    y * 4 + iBlkYOffset[iBlk] * 2 + vctBak.y, 
                                    m_vopmd.iRoundingControl, prctMVLimitBak);
          }
          // ~mod 991201 mwi
        }
        else {
          motionComp(m_ppxlcPredMBY, m_pvopcRefQ0->pixelsY(), MB_SIZE,
                     x * 2 +  vctFwd.x, y * 2 +  vctFwd.y, m_vopmd.iRoundingControl, prctMVLimitFwd);
          motionComp(m_ppxlcPredMBBackY , m_pvopcRefQ1->pixelsY(), MB_SIZE,
                     x * 2 +  vctBak.x, y * 2 +  vctBak.y, m_vopmd.iRoundingControl, prctMVLimitBak);
        }
        
        if (m_volmd.bQuarterSample) { // Quarter Sample, mwi
          iChromaFwdX = vctFwd.x/2;
          iChromaFwdY = vctFwd.y/2;
          iChromaBakX = vctBak.x/2;
          iChromaBakY = vctBak.y/2;
        }
        else {	
          iChromaFwdX = vctFwd.x;
          iChromaFwdY = vctFwd.y;
          iChromaBakX = vctBak.x;
          iChromaBakY = vctBak.y;
        }
        xRefUVF = sign (iChromaFwdX) * (grgiMvRound4  [abs (iChromaFwdX) % 4] + (abs (iChromaFwdX) / 4) * 2);
        yRefUVF = sign (iChromaFwdY) * (grgiMvRound4  [abs (iChromaFwdY) % 4] + (abs (iChromaFwdY) / 4) * 2);
        xRefUVB = sign (iChromaBakX) * (grgiMvRound4  [abs (iChromaBakX) % 4] + (abs (iChromaBakX) / 4) * 2);
        yRefUVB = sign (iChromaBakY) * (grgiMvRound4  [abs (iChromaBakY) % 4] + (abs (iChromaBakY) / 4) * 2);       
      }
    // end of new changes 10/21/98
        
    motionCompUV(m_ppxlcPredMBU, m_ppxlcPredMBV, m_pvopcRefQ0, x, y,
                 xRefUVF,
                 yRefUVF,m_vopmd.iRoundingControl, prctMVLimitFwd);
    motionCompUV(m_ppxlcPredMBBackU, m_ppxlcPredMBBackV, m_pvopcRefQ1, x, y,
                 xRefUVB,
                 yRefUVB,m_vopmd.iRoundingControl, prctMVLimitBak);
  }
}

Void CVideoObject::motionCompOneBVOPReference(
	CVOPU8YUVBA *pvopcPred,
	MBType type,
	CoordI x, CoordI y,
	const CMBMode *pmbmd,
	const CMotionVector *pmv,
	CRct *prctMVLimit
)
{ 
  CVOPU8YUVBA *pvopcRef;
  Int topRef, botRef;

  if (type == BACKWARD) {
    pvopcRef = m_pvopcRefQ1;
    topRef = (Int)pmbmd->m_bBackwardTop;
    botRef = (Int)pmbmd->m_bBackwardBottom;
  } else {
    pvopcRef = m_pvopcRefQ0;
    topRef = (Int)pmbmd->m_bForwardTop;
    botRef = (Int)pmbmd->m_bForwardBottom;
  }
  if (pmbmd->m_bFieldMV) {
    const CMotionVector *pmvTop = pmv + 1 + topRef;
    const CMotionVector *pmvBot = pmv + 3 + botRef;
    assert((topRef & ~1) == 0); assert((botRef & ~1) == 0);
    CoordI iMVX, iMVY;

    if (m_volmd.bQuarterSample) {
      iMVX = 4*x + pmvTop->m_vctTrueHalfPel.x;
      iMVY = 4*y + pmvTop->m_vctTrueHalfPel.y;
//      limitMVRangeToExtendedBBQuarterPel(iMVX, iMVY, prctMVLimit, MB_SIZE);
      // Luma top field
      motionCompQuarterSample((PixelC *)pvopcPred->pixelsY(),
                              pvopcRef->pixelsY() + topRef * m_iFrameWidthY, 0, 
                              iMVX, iMVY, m_vopmd.iRoundingControl, prctMVLimit);
      iMVX = 2*x + pmvTop->iMVX;
      iMVY = 2*y + pmvTop->iMVY;
//      limitMVRangeToExtendedBBHalfPel(iMVX, iMVY, prctMVLimit, MB_SIZE);
    }
    else {
      iMVX = 2*x + pmvTop->m_vctTrueHalfPel.x;
      iMVY = 2*y + pmvTop->m_vctTrueHalfPel.y;
//      limitMVRangeToExtendedBBHalfPel(iMVX, iMVY, prctMVLimit, MB_SIZE);
      motionCompYField((PixelC *)pvopcPred->pixelsY(),					// Luma top field
                       pvopcRef->pixelsY() + topRef * m_iFrameWidthY, iMVX, iMVY, prctMVLimit);
    }
      
 
    iMVX -= 2*x; iMVY -= 2*y;
    motionCompFieldUV((PixelC *)pvopcPred->pixelsU(),					// Chroma top field
                      (PixelC *)pvopcPred->pixelsV(), pvopcRef, x, y,
                      (iMVX & 3) ? ((iMVX >> 1) | 1) : (iMVX >> 1),
                      (iMVY & 6) ? ((iMVY >> 1) | 2) : (iMVY >> 1), topRef,
                      prctMVLimit);

    if (m_volmd.bQuarterSample) {
      iMVX = 4*x + pmvBot->m_vctTrueHalfPel.x;
      iMVY = 4*y + pmvBot->m_vctTrueHalfPel.y;
//      limitMVRangeToExtendedBBQuarterPel(iMVX, iMVY, prctMVLimit, MB_SIZE);
      // Luma bottom field
      motionCompQuarterSample((PixelC *)(pvopcPred->pixelsY()) + MB_SIZE,
                              pvopcRef->pixelsY() + botRef * m_iFrameWidthY, 0, 
                              iMVX, iMVY, m_vopmd.iRoundingControl, prctMVLimit);
      iMVX = 2*x + pmvBot->iMVX;
      iMVY = 2*y + pmvBot->iMVY;
//      limitMVRangeToExtendedBBHalfPel(iMVX, iMVY, prctMVLimit, MB_SIZE);
    }
    else {
      iMVX = 2*x + pmvBot->m_vctTrueHalfPel.x;
      iMVY = 2*y + pmvBot->m_vctTrueHalfPel.y;
//      limitMVRangeToExtendedBBHalfPel(iMVX, iMVY, prctMVLimit, MB_SIZE);
      motionCompYField((PixelC *)(pvopcPred->pixelsY()) + MB_SIZE,		// Luma bottom field
                       pvopcRef->pixelsY() + botRef * m_iFrameWidthY, iMVX, iMVY,
		       prctMVLimit);
    }
      

    iMVX -= 2*x; iMVY -= 2*y;
    motionCompFieldUV((PixelC *)(pvopcPred->pixelsU()) + BLOCK_SIZE,	// Chroma bottom field
                      (PixelC *)(pvopcPred->pixelsV()) + BLOCK_SIZE, pvopcRef, x, y,
                      (iMVX & 3) ? ((iMVX >> 1) | 1) : (iMVX >> 1),
                      (iMVY & 6) ? ((iMVY >> 1) | 2) : (iMVY >> 1), botRef, prctMVLimit);
	} else { // no longer -> // rounding control is messed up here
      if (m_volmd.bQuarterSample)
        motionCompQuarterSample((PixelC *)pvopcPred->pixelsY(), pvopcRef->pixelsY(), MB_SIZE,
                                x * 4 + pmv->trueMVHalfPel().x, y * 4 + pmv->trueMVHalfPel().y, 
                                m_vopmd.iRoundingControl, prctMVLimit);
      else
		motionComp((PixelC *)pvopcPred->pixelsY(), pvopcRef->pixelsY(), MB_SIZE,
                   x * 2 + pmv->trueMVHalfPel().x, y * 2 + pmv->trueMVHalfPel().y, m_vopmd.iRoundingControl, prctMVLimit);
      // changed by mwi 980806
      //       motionCompUV((PixelC *)pvopcPred->pixelsU(), (PixelC *)pvopcPred->pixelsV(), pvopcRef, x, y,
      //                    (pmv->m_vctTrueHalfPel.x & 3) ? ((pmv->m_vctTrueHalfPel.x >> 1) | 1) :
      //                    (pmv->m_vctTrueHalfPel.x >> 1),
      //                    (pmv->m_vctTrueHalfPel.y & 3) ? ((pmv->m_vctTrueHalfPel.y >> 1) | 1) :
      //                    (pmv->m_vctTrueHalfPel.y >> 1), m_vopmd.iRoundingControl, prctMVLimit);
// GMC
      if (m_volmd.bQuarterSample)
// ~GMC
      motionCompUV((PixelC *)pvopcPred->pixelsU(), (PixelC *)pvopcPred->pixelsV(), pvopcRef, x, y,
                   (pmv->iMVX & 3) ? ((pmv->iMVX >> 1) | 1) :
                   (pmv->iMVX >> 1),
                   (pmv->iMVY & 3) ? ((pmv->iMVY >> 1) | 1) :
                   (pmv->iMVY >> 1), m_vopmd.iRoundingControl, prctMVLimit);
// GMC
      else
             motionCompUV((PixelC *)pvopcPred->pixelsU(), (PixelC *)pvopcPred->pixelsV(), pvopcRef, x, y,
                          (pmv->m_vctTrueHalfPel.x & 3) ? ((pmv->m_vctTrueHalfPel.x >> 1) | 1) :
                          (pmv->m_vctTrueHalfPel.x >> 1),
                          (pmv->m_vctTrueHalfPel.y & 3) ? ((pmv->m_vctTrueHalfPel.y >> 1) | 1) :
                          (pmv->m_vctTrueHalfPel.y >> 1), m_vopmd.iRoundingControl, prctMVLimit);
// changed by Y.Suzuki 99/02/03 
// ~GMC
      // ~changed by mwi 980806

	}
}

Void CVideoObject::motionCompYField (
	PixelC* ppxlcPred, // can be either Y or A
	const PixelC* ppxlcRefLeftTop, // point to left-top of the frame
	CoordI xRef, CoordI yRef, // current coordinate system
	CRct *prctMVLimit // added by Y.Suzuki for the extended bounding box support
)
{
	CoordI ix, iy;

	limitMVRangeToExtendedBBHalfPel(xRef, yRef, prctMVLimit, MB_SIZE); // added by Y.Suzuki for the extended bounding box support

	const PixelC* ppxlcRef = ppxlcRefLeftTop + 
		(((yRef >> 1) & ~1) + EXPANDY_REF_FRAME) * m_iFrameWidthY + (xRef >> 1) + EXPANDY_REF_FRAME;
	Int iRound = 1 - m_vopmd.iRoundingControl;
	Int iFieldStep = 2 * m_iFrameWidthY;

	if (!(yRef & 2)) {
		if (!(xRef & 1)) {  //!bXSubPxl && !bYSubPxl
			for (iy = 0; iy < MB_SIZE; iy+=2) {
				memcpy (ppxlcPred, ppxlcRef, MB_SIZE*sizeof(PixelC));
				ppxlcRef += iFieldStep;
				ppxlcPred += MB_SIZE*2;
			}
		}
		else {  //bXSubPxl && !bYSubPxl
			for (iy = 0; iy < MB_SIZE; iy+=2){
				for (ix = 0; ix < MB_SIZE; ix++)
					ppxlcPred [ix] = (ppxlcRef [ix] + ppxlcRef [ix + 1] + iRound) >> 1;
				ppxlcRef += iFieldStep;
				ppxlcPred += MB_SIZE*2;
			}
		}
	}
	else {
		const PixelC* ppxlcRefBot;
		if (!(xRef & 1)) {  //!bXSubPxl&& bYSubPxl
			for (iy = 0; iy < MB_SIZE; iy+=2) {
				ppxlcRefBot = ppxlcRef + iFieldStep;		//UPln -> pixels (xInt,yInt+1);
				for (ix = 0; ix < MB_SIZE; ix++) 
					ppxlcPred [ix] = (ppxlcRef [ix] + ppxlcRefBot [ix] + iRound) >> 1;
				ppxlcRef = ppxlcRefBot;
				ppxlcPred += MB_SIZE*2;
			}
		}
		else { // bXSubPxl && bYSubPxl
			iRound++;
			for (iy = 0; iy < MB_SIZE; iy+=2) {
				ppxlcRefBot = ppxlcRef + iFieldStep;		//UPln -> pixels (xInt,yInt+1);
				for (ix = 0; ix < MB_SIZE; ix++){
					ppxlcPred [ix] = (ppxlcRef [ix + 1] + ppxlcRef [ix] +
						ppxlcRefBot [ix + 1] + ppxlcRefBot [ix] + iRound) >> 2;

				}
				ppxlcRef = ppxlcRefBot;
				ppxlcPred += MB_SIZE*2;
			}
		}
	}
}

Void CVideoObject::motionCompFieldUV (	PixelC* ppxlcPredMBU, PixelC* ppxlcPredMBV,
								 const CVOPU8YUVBA* pvopcRef,
								 CoordI x, CoordI y, 
								 CoordI xRefUV, CoordI yRefUV,Int iRefFieldSelect,
								 CRct* prctMVLimit // added by Y.Suzuki for the extended bounding box support
								 )
{
	UInt ix, iy;
// added by Y.Suzuki for the extended bounding box support
	CoordI iTmpX = x + xRefUV;
	CoordI iTmpY = y + yRefUV;
	limitMVRangeToExtendedBBFullPel(iTmpX, iTmpY, prctMVLimit, MB_SIZE);
	xRefUV = iTmpX - x;
	yRefUV = iTmpY - y;
// ~extended bounding box support

	// delete by Hyundai for Microsoft and MoMusys alignment
	//Int iPxLoc = ((((y + yRefUV) >> 1) & ~1) + EXPANDUV_REF_FRAME) * m_iFrameWidthUV + ((x + xRefUV) >> 1) + EXPANDUV_REF_FRAME;
	// insert by Hyundai for Microsoft and MoMusys alignment
	Int iPxLoc = (y/2 + ((yRefUV >> 1) & ~1) + EXPANDUV_REF_FRAME) * m_iFrameWidthUV + ((x + xRefUV) >> 1) + EXPANDUV_REF_FRAME;

	const PixelC* ppxlcPrevU = pvopcRef->pixelsU () + iPxLoc + iRefFieldSelect*m_iFrameWidthUV;
	const PixelC* ppxlcPrevV = pvopcRef->pixelsV () + iPxLoc + iRefFieldSelect*m_iFrameWidthUV;
	Int iRound = 1 - m_vopmd.iRoundingControl;
	Int iFieldStep = 2 * m_iFrameWidthUV;

	if (!(yRefUV & 2)) {
		if (!(xRefUV & 1)) {  //!bXSubPxl && !bYSubPxl
			for (iy = 0; iy < BLOCK_SIZE; iy+=2) {
				memcpy (ppxlcPredMBU, ppxlcPrevU, BLOCK_SIZE*sizeof(PixelC));
				memcpy (ppxlcPredMBV, ppxlcPrevV, BLOCK_SIZE*sizeof(PixelC));
				ppxlcPrevU += iFieldStep;
				ppxlcPrevV += iFieldStep;
				ppxlcPredMBU += 2*BLOCK_SIZE;
				ppxlcPredMBV += 2*BLOCK_SIZE;
			}
		}
		else {  //bXSubPxl && !bYSubPxl
			for (iy = 0; iy < BLOCK_SIZE; iy+=2) {
				for (ix = 0; ix < BLOCK_SIZE; ix++) {
					ppxlcPredMBU [ix] = (ppxlcPrevU [ix + 1] + ppxlcPrevU [ix] + iRound) >> 1;
					ppxlcPredMBV [ix] = (ppxlcPrevV [ix + 1] + ppxlcPrevV [ix] + iRound) >> 1;
				}
				ppxlcPrevU += iFieldStep;
				ppxlcPrevV += iFieldStep;
				ppxlcPredMBU += 2*BLOCK_SIZE;
				ppxlcPredMBV += 2*BLOCK_SIZE;
			}
		}
	}
	else {
		const PixelC* ppxlcPrevUBot; 
		const PixelC* ppxlcPrevVBot; 
		if (!(xRefUV & 1)) {  //!bXSubPxl&& bYSubPxl
			for (iy = 0; iy < BLOCK_SIZE; iy+=2) {
				ppxlcPrevUBot = ppxlcPrevU + iFieldStep;            //UPln -> pixels (xInt,yInt+1);
				ppxlcPrevVBot = ppxlcPrevV + iFieldStep;            //VPln -> pixels (xInt,yInt+1);
				for (ix = 0; ix < BLOCK_SIZE; ix++) {
					ppxlcPredMBU [ix] = (ppxlcPrevU [ix] + ppxlcPrevUBot [ix] + iRound) >> 1;
					ppxlcPredMBV [ix] = (ppxlcPrevV [ix] + ppxlcPrevVBot [ix] + iRound) >> 1;
				}
				ppxlcPredMBU += 2*BLOCK_SIZE;
				ppxlcPredMBV += 2*BLOCK_SIZE;
				ppxlcPrevU = ppxlcPrevUBot; 
				ppxlcPrevV = ppxlcPrevVBot; 
			}
		}
		else { // bXSubPxl && bYSubPxl
			iRound++;
			for (iy = 0; iy < BLOCK_SIZE; iy+=2){
				ppxlcPrevUBot = ppxlcPrevU + iFieldStep; //UPln -> pixels (xInt,yInt+1);
				ppxlcPrevVBot = ppxlcPrevV + iFieldStep; //VPln -> pixels (xInt,yInt+1);
				for (ix = 0; ix < BLOCK_SIZE; ix++){
					ppxlcPredMBU [ix] = (ppxlcPrevU [ix + 1] + ppxlcPrevU [ix] + 
						ppxlcPrevUBot [ix + 1] + ppxlcPrevUBot [ix] + iRound) >> 2;
					ppxlcPredMBV [ix] = (ppxlcPrevV [ix + 1] + ppxlcPrevV [ix] + 
						ppxlcPrevVBot [ix + 1] + ppxlcPrevVBot [ix] + iRound) >> 2;
				}
				ppxlcPredMBU += 2*BLOCK_SIZE;
				ppxlcPredMBV += 2*BLOCK_SIZE;
				ppxlcPrevU = ppxlcPrevUBot; 
				ppxlcPrevV = ppxlcPrevVBot; 
			}
		}
	}
}

// #endif // INTERLACE

Void CVideoObject::motionCompUV (
	PixelC* ppxlcPredMBU, PixelC* ppxlcPredMBV,
	const CVOPU8YUVBA* pvopcRef,
	CoordI x, CoordI y, 
	CoordI xRefUV, CoordI yRefUV,
	Int iRoundingControl,
	CRct *prctMVLimit
)
{
	Int ix, iy;
	CoordI iTmpX = x + xRefUV;
	CoordI iTmpY = y + yRefUV;
// RRV modification
	limitMVRangeToExtendedBBFullPel(iTmpX, iTmpY, prctMVLimit,
									(MB_SIZE *m_iRRVScale));
//	limitMVRangeToExtendedBBFullPel (iTmpX,iTmpY,prctMVLimit,MB_SIZE);
// ~RRV
	xRefUV = iTmpX - x;
	yRefUV = iTmpY - y;

	Int iPxLoc = (((y + yRefUV) >> 1) + EXPANDUV_REF_FRAME) * m_iFrameWidthUV + ((x + xRefUV) >> 1) + EXPANDUV_REF_FRAME;
	const PixelC* ppxlcPrevU = pvopcRef->pixelsU () + iPxLoc;
	const PixelC* ppxlcPrevV = pvopcRef->pixelsV () + iPxLoc;
	if (!(yRefUV & 1)) {
		if (!(xRefUV & 1)) {  //!bXSubPxl && !bYSubPxl
// RRV modification
			for (iy = 0; iy < (BLOCK_SIZE *m_iRRVScale); iy++) {
				memcpy(ppxlcPredMBU, ppxlcPrevU, 
						(BLOCK_SIZE *m_iRRVScale) *sizeof(PixelC));
				memcpy(ppxlcPredMBV, ppxlcPrevV, 
						(BLOCK_SIZE *m_iRRVScale) *sizeof(PixelC));
				ppxlcPrevU += m_iFrameWidthUV;
				ppxlcPrevV += m_iFrameWidthUV;
				ppxlcPredMBU += (BLOCK_SIZE *m_iRRVScale);
				ppxlcPredMBV += (BLOCK_SIZE *m_iRRVScale);
			}
//			for (iy = 0; iy < BLOCK_SIZE; iy++) {
//				memcpy (ppxlcPredMBU, ppxlcPrevU, BLOCK_SIZE*sizeof(PixelC));
//				memcpy (ppxlcPredMBV, ppxlcPrevV, BLOCK_SIZE*sizeof(PixelC));
//				ppxlcPrevU += m_iFrameWidthUV;
//				ppxlcPrevV += m_iFrameWidthUV;
//				ppxlcPredMBU += BLOCK_SIZE;
//				ppxlcPredMBV += BLOCK_SIZE;
//			}
// ~RRV
		}
		else {  //bXSubPxl && !bYSubPxl
// RRV modification
			for (iy = 0; iy < (BLOCK_SIZE *m_iRRVScale); iy++) {
				for (ix = 0; ix < (BLOCK_SIZE *m_iRRVScale); ix++) {
//			for (iy = 0; iy < BLOCK_SIZE; iy++) {
//				for (ix = 0; ix < BLOCK_SIZE; ix++) {
// ~RRV
					ppxlcPredMBU [ix] = (ppxlcPrevU [ix + 1] + ppxlcPrevU [ix] + 1 - iRoundingControl) >> 1;
					ppxlcPredMBV [ix] = (ppxlcPrevV [ix + 1] + ppxlcPrevV [ix] + 1 - iRoundingControl) >> 1;
				}
				ppxlcPrevU += m_iFrameWidthUV;
				ppxlcPrevV += m_iFrameWidthUV;
// RRV modification
				ppxlcPredMBU += (BLOCK_SIZE *m_iRRVScale);
				ppxlcPredMBV += (BLOCK_SIZE *m_iRRVScale);
//				ppxlcPredMBU += BLOCK_SIZE;
//				ppxlcPredMBV += BLOCK_SIZE;
// ~RRV
			}
		}
	}
	else {
		const PixelC* ppxlcPrevUBot; 
		const PixelC* ppxlcPrevVBot; 
		if (!(xRefUV & 1)) {  //!bXSubPxl&& bYSubPxl
// RRV modification
			for (iy = 0; iy < (BLOCK_SIZE *m_iRRVScale); iy++) {
//			for (iy = 0; iy < BLOCK_SIZE; iy++) {
// ~RRV
				ppxlcPrevUBot = ppxlcPrevU + m_iFrameWidthUV;            //UPln -> pixels (xInt,yInt+1);
				ppxlcPrevVBot = ppxlcPrevV + m_iFrameWidthUV;            //VPln -> pixels (xInt,yInt+1);
// RRV modification
				for (ix = 0; ix < (BLOCK_SIZE *m_iRRVScale); ix++) {
//				for (ix = 0; ix < BLOCK_SIZE; ix++) {
// ~RRV
//					ppxlcPredMBU [ix] = (ppxlcPrevU [ix] + ppxlcPrevUBot [ix] + 1) >> 1;
//					ppxlcPredMBV [ix] = (ppxlcPrevV [ix] + ppxlcPrevVBot [ix] + 1) >> 1;
					ppxlcPredMBU [ix] = (ppxlcPrevU [ix] + ppxlcPrevUBot [ix] + 1 - iRoundingControl) >> 1;
					ppxlcPredMBV [ix] = (ppxlcPrevV [ix] + ppxlcPrevVBot [ix] + 1 - iRoundingControl) >> 1;
				}
// RRV modification
				ppxlcPredMBU += (BLOCK_SIZE *m_iRRVScale);
				ppxlcPredMBV += (BLOCK_SIZE *m_iRRVScale);
//				ppxlcPredMBU += BLOCK_SIZE;
//				ppxlcPredMBV += BLOCK_SIZE;
// ~RRV
				ppxlcPrevU = ppxlcPrevUBot; 
				ppxlcPrevV = ppxlcPrevVBot; 
			}
		}
		else { // bXSubPxl && bYSubPxl
// RRV modification
			for (iy = 0; iy < (BLOCK_SIZE *m_iRRVScale); iy++){
//			for (iy = 0; iy < BLOCK_SIZE; iy++){
// ~RRV
				ppxlcPrevUBot = ppxlcPrevU + m_iFrameWidthUV; //UPln -> pixels (xInt,yInt+1);
				ppxlcPrevVBot = ppxlcPrevV + m_iFrameWidthUV; //VPln -> pixels (xInt,yInt+1);
// RRV modification
				for (ix = 0; ix < (BLOCK_SIZE *m_iRRVScale); ix++){
//				for (ix = 0; ix < BLOCK_SIZE; ix++){
// ~RRV
/*
					ppxlcPredMBU [ix] = (
						ppxlcPrevU [ix + 1] + ppxlcPrevU [ix] + 
						ppxlcPrevUBot [ix + 1] + ppxlcPrevUBot [ix] + 2
					) >> 2;
					ppxlcPredMBV [ix] = (
						ppxlcPrevV [ix + 1] + ppxlcPrevV [ix] + 
						ppxlcPrevVBot [ix + 1] + ppxlcPrevVBot [ix] + 2
					) >> 2;
*/
					ppxlcPredMBU [ix] = (
						ppxlcPrevU [ix + 1] + ppxlcPrevU [ix] + 
						ppxlcPrevUBot [ix + 1] + ppxlcPrevUBot [ix] + 2 - iRoundingControl
					) >> 2;
					ppxlcPredMBV [ix] = (
						ppxlcPrevV [ix + 1] + ppxlcPrevV [ix] + 
						ppxlcPrevVBot [ix + 1] + ppxlcPrevVBot [ix] + 2 - iRoundingControl
					) >> 2;
				}
// RRV modification
				ppxlcPredMBU += (BLOCK_SIZE *m_iRRVScale);
				ppxlcPredMBV += (BLOCK_SIZE *m_iRRVScale);
//				ppxlcPredMBU += BLOCK_SIZE;
//				ppxlcPredMBV += BLOCK_SIZE;
// ~RRV
				ppxlcPrevU = ppxlcPrevUBot; 
				ppxlcPrevV = ppxlcPrevVBot; 
			}
		}
	}
}

UInt gOvrlpPredY [64];
// RRV insertion
UInt gOvrlpPredY_x2 [256];
// ~RRV

Void CVideoObject::motionCompOverLap (
	PixelC* ppxlcPredMB, 
	const PixelC* ppxlcRefLeftTop,
	const CMotionVector* pmv, // motion vector
	const CMBMode* pmbmd, // macroblk mode	
	Int imbx, // current macroblk index
	Int imby, // current macroblk index
	CoordI x, // current coordinate system
	CoordI y, // current coordinate system
	CRct *prctMVLimit
)
{
  // Overlap Motion Comp use motion vector of current blk and motion vectors of neighboring blks.
  const CMotionVector *pmvC,*pmvT=NULL,*pmvB=NULL,*pmvR=NULL,*pmvL=NULL; // MVs of Cur, Top, Bot, Right and Left Blocks.
  const CMotionVector *pmvCurrMb,*pmvTopMb = NULL,*pmvRightMb = NULL,*pmvLeftMb = NULL; // MVs of Cur, Top, Right and Left MacroBlocks.
  const CMBMode *pmbmdTopMb = NULL, *pmbmdRightMb = NULL, *pmbmdLeftMb = NULL; // MVs of Cur, Top, Right and Left MacroBlocks.
  Bool bIntraT = false, bIntraR = false, bIntraL = false; // flags of 4MV for Cur, Top, Right and Left MacroBlocks.
  Bool bLeftBndry, bRightBndry, bTopBndry;

// NEWPRED
	if ((m_volmd.bNewpredEnable) && (m_volmd.bNewpredSegmentType == 0)) {
//		if (imbx >= m_iNumMBX) {
		if (imby >= 1) {
			bTopBndry = (pmbmd->m_iNPSegmentNumber != (pmbmd-m_iNumMBX)->m_iNPSegmentNumber);
		} else {
			bTopBndry = (imby == 0); 
		}
		if (imbx >= 1) {
			bLeftBndry = (pmbmd->m_iNPSegmentNumber != (pmbmd-1)->m_iNPSegmentNumber);
		} else {
			bLeftBndry = (imbx == 0);
		}
		if (imbx < m_iNumMBX-1) {
			bRightBndry = (pmbmd->m_iNPSegmentNumber != (pmbmd+1)->m_iNPSegmentNumber);
		} else {
			bRightBndry = (imbx == m_iNumMBX - 1);
		}
	} else {
		bTopBndry = (imby == 0);
		bLeftBndry = (imbx == 0);
		bRightBndry = (imbx == m_iNumMBX - 1);
	}
// ~NEWPRED

  pmvCurrMb = pmv;
  // assign the neighboring blk's MVs to pmv[TBRLC]
  if (!bTopBndry) {
    pmbmdTopMb = pmbmd - m_iNumMBX; 
    bIntraT = (pmbmdTopMb->m_dctMd == INTRA || pmbmdTopMb->m_dctMd == INTRAQ || pmbmdTopMb->m_bMCSEL == TRUE); // GMC
    pmvTopMb = pmv - m_iNumOfTotalMVPerRow;
  }
  if (!bLeftBndry) {
    pmbmdLeftMb = pmbmd - 1;
    bIntraL = (pmbmdLeftMb->m_dctMd == INTRA || pmbmdLeftMb->m_dctMd == INTRAQ || pmbmdLeftMb->m_bMCSEL == TRUE); // GMC
    pmvLeftMb = pmv - PVOP_MV_PER_REF_PER_MB;
  }
  if (!bRightBndry) {
    pmbmdRightMb = pmbmd + 1;
    bIntraR = (pmbmdRightMb->m_dctMd == INTRA || pmbmdRightMb->m_dctMd == INTRAQ || pmbmdRightMb->m_bMCSEL == TRUE); // GMC
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
// RRV modification
    UInt dxc = (((i - 1) & 1) << 3) *m_iRRVScale;
    UInt dyc = (((i - 1) & 2) << 2) *m_iRRVScale;
//    UInt dxc = (((i - 1) & 1) << 3);
//    UInt dyc = (((i - 1) & 2) << 2);
// ~RRV
    UInt nxcY = (x + dxc) << 1; 
    UInt nycY = (y + dyc) << 1; 
    if (m_volmd.bQuarterSample) { // Quarter sample
      CoordI xRefC = ((nxcY + pmvC -> iMVX) << 1) + pmvC -> iHalfX, 
        yRefC = ((nycY + pmvC -> iMVY) << 1) + pmvC -> iHalfY;
      CoordI xRefT = ((nxcY + pmvT -> iMVX) << 1) + pmvT -> iHalfX, 
        yRefT = ((nycY + pmvT -> iMVY) << 1) + pmvT -> iHalfY;
      CoordI xRefB = ((nxcY + pmvB -> iMVX) << 1) + pmvB -> iHalfX, 
        yRefB = ((nycY + pmvB -> iMVY) << 1) + pmvB -> iHalfY;
      CoordI xRefR = ((nxcY + pmvR -> iMVX) << 1) + pmvR -> iHalfX, 
        yRefR = ((nycY + pmvR -> iMVY) << 1) + pmvR -> iHalfY;
      CoordI xRefL = ((nxcY + pmvL -> iMVX) << 1) + pmvL -> iHalfX, 
        yRefL = ((nycY + pmvL -> iMVY) << 1) + pmvL -> iHalfY;

      limitMVRangeToExtendedBBQuarterPel (xRefC,yRefC,prctMVLimit,BLOCK_SIZE);
      limitMVRangeToExtendedBBQuarterPel (xRefT,yRefT,prctMVLimit,BLOCK_SIZE);
      limitMVRangeToExtendedBBQuarterPel (xRefB,yRefB,prctMVLimit,BLOCK_SIZE);
      limitMVRangeToExtendedBBQuarterPel (xRefR,yRefR,prctMVLimit,BLOCK_SIZE);
      limitMVRangeToExtendedBBQuarterPel (xRefL,yRefL,prctMVLimit,BLOCK_SIZE);

      U8 *ppxlcblkC; ppxlcblkC = (U8*) calloc(BLOCK_SIZE*BLOCK_SIZE,sizeof(U8));
      U8 *ppxlcblkT; ppxlcblkT = (U8*) calloc(BLOCK_SIZE*BLOCK_SIZE,sizeof(U8));
      U8 *ppxlcblkB; ppxlcblkB = (U8*) calloc(BLOCK_SIZE*BLOCK_SIZE,sizeof(U8));
      U8 *ppxlcblkR; ppxlcblkR = (U8*) calloc(BLOCK_SIZE*BLOCK_SIZE,sizeof(U8));
      U8 *ppxlcblkL; ppxlcblkL = (U8*) calloc(BLOCK_SIZE*BLOCK_SIZE,sizeof(U8));

      blkInterpolateY (ppxlcRefLeftTop, BLOCK_SIZE, xRefC, yRefC, ppxlcblkC, m_vopmd.iRoundingControl);
      blkInterpolateY (ppxlcRefLeftTop, BLOCK_SIZE, xRefT, yRefT, ppxlcblkT, m_vopmd.iRoundingControl);
      blkInterpolateY (ppxlcRefLeftTop, BLOCK_SIZE, xRefB, yRefB, ppxlcblkB, m_vopmd.iRoundingControl);
      blkInterpolateY (ppxlcRefLeftTop, BLOCK_SIZE, xRefR, yRefR, ppxlcblkR, m_vopmd.iRoundingControl);
      blkInterpolateY (ppxlcRefLeftTop, BLOCK_SIZE, xRefL, yRefL, ppxlcblkL, m_vopmd.iRoundingControl);

      UInt *pWghtC, *pWghtT, *pWghtB, *pWghtR, *pWghtL;
      pWghtC = (UInt*) gWghtC;
      pWghtT = (UInt*) gWghtT;
      pWghtB = (UInt*) gWghtB;
      pWghtR = (UInt*) gWghtR;
      pWghtL = (UInt*) gWghtL;
      PixelC* ppxliPredY = ppxlcPredMB + dxc + dyc * MB_SIZE;  
      // Starting of Pred. Frame
      for (UInt iy = 0; iy < BLOCK_SIZE; iy++) {
        for (UInt ix = 0; ix < BLOCK_SIZE; ix++) {
          *ppxliPredY++ = (
                           *(ppxlcblkC+ix+iy*BLOCK_SIZE) * *pWghtC++ + 
                           *(ppxlcblkT+ix+iy*BLOCK_SIZE) * *pWghtT++ + 
                           *(ppxlcblkB+ix+iy*BLOCK_SIZE) * *pWghtB++ + 
                           *(ppxlcblkR+ix+iy*BLOCK_SIZE) * *pWghtR++ + 
                           *(ppxlcblkL+ix+iy*BLOCK_SIZE) * *pWghtL++ + 4
                           ) >> 3;
        }
        ppxliPredY += BLOCK_SIZE;
      }
      free(ppxlcblkC);
      free(ppxlcblkT);
      free(ppxlcblkB);
      free(ppxlcblkR);
      free(ppxlcblkL);
    }
    else {
      // Compute the corresponding positions on Ref frm, using 5 MVs.
// RRV modification
		CoordI xRefC, yRefC, xRefT, yRefT, xRefB, yRefB, xRefR, yRefR, xRefL, yRefL;
		if(m_vopmd.RRVmode.iRRVOnOff)
		{
			xRefC = nxcY + pmvC->m_vctTrueHalfPel_x2.x, yRefC = nycY + pmvC->m_vctTrueHalfPel_x2.y;
			xRefT = nxcY + pmvT->m_vctTrueHalfPel_x2.x, yRefT = nycY + pmvT->m_vctTrueHalfPel_x2.y;
			xRefB = nxcY + pmvB->m_vctTrueHalfPel_x2.x, yRefB = nycY + pmvB->m_vctTrueHalfPel_x2.y;
			xRefR = nxcY + pmvR->m_vctTrueHalfPel_x2.x, yRefR = nycY + pmvR->m_vctTrueHalfPel_x2.y;
			xRefL = nxcY + pmvL->m_vctTrueHalfPel_x2.x, yRefL = nycY + pmvL->m_vctTrueHalfPel_x2.y;
		}
		else
		{
			xRefC = nxcY + pmvC->m_vctTrueHalfPel.x, yRefC = nycY + pmvC->m_vctTrueHalfPel.y;
			xRefT = nxcY + pmvT->m_vctTrueHalfPel.x, yRefT = nycY + pmvT->m_vctTrueHalfPel.y;
			xRefB = nxcY + pmvB->m_vctTrueHalfPel.x, yRefB = nycY + pmvB->m_vctTrueHalfPel.y;
			xRefR = nxcY + pmvR->m_vctTrueHalfPel.x, yRefR = nycY + pmvR->m_vctTrueHalfPel.y;
			xRefL = nxcY + pmvL->m_vctTrueHalfPel.x, yRefL = nycY + pmvL->m_vctTrueHalfPel.y;
		}
//      CoordI xRefC = nxcY + pmvC->m_vctTrueHalfPel.x, yRefC = nycY + pmvC->m_vctTrueHalfPel.y;
//      CoordI xRefT = nxcY + pmvT->m_vctTrueHalfPel.x, yRefT = nycY + pmvT->m_vctTrueHalfPel.y;
//      CoordI xRefB = nxcY + pmvB->m_vctTrueHalfPel.x, yRefB = nycY + pmvB->m_vctTrueHalfPel.y;
//      CoordI xRefR = nxcY + pmvR->m_vctTrueHalfPel.x, yRefR = nycY + pmvR->m_vctTrueHalfPel.y;
//      CoordI xRefL = nxcY + pmvL->m_vctTrueHalfPel.x, yRefL = nycY + pmvL->m_vctTrueHalfPel.y;
// ~RRV
      //		UInt nxcY = x + dxc;
      //		UInt nycY = y + dyc;
      //		// Compute the corresponding positions on Ref frm, using 5 MVs.
      //		CoordI xRefC = ((nxcY + pmvC -> iMVX) << 1) + pmvC -> iHalfX, yRefC = ((nycY + pmvC -> iMVY) << 1) + pmvC -> iHalfY;
      //		CoordI xRefT = ((nxcY + pmvT -> iMVX) << 1) + pmvT -> iHalfX, yRefT = ((nycY + pmvT -> iMVY) << 1) + pmvT -> iHalfY;
      //  	CoordI xRefB = ((nxcY + pmvB -> iMVX) << 1) + pmvB -> iHalfX, yRefB = ((nycY + pmvB -> iMVY) << 1) + pmvB -> iHalfY;
      //		CoordI xRefR = ((nxcY + pmvR -> iMVX) << 1) + pmvR -> iHalfX, yRefR = ((nycY + pmvR -> iMVY) << 1) + pmvR -> iHalfY;
      //		CoordI xRefL = ((nxcY + pmvL -> iMVX) << 1) + pmvL -> iHalfX, yRefL = ((nycY + pmvL -> iMVY) << 1) + pmvL -> iHalfY;	
// RRV modification
		limitMVRangeToExtendedBBHalfPel (xRefC,yRefC,prctMVLimit,(BLOCK_SIZE *m_iRRVScale));
		limitMVRangeToExtendedBBHalfPel (xRefT,yRefT,prctMVLimit,(BLOCK_SIZE *m_iRRVScale));
		limitMVRangeToExtendedBBHalfPel (xRefB,yRefB,prctMVLimit,(BLOCK_SIZE *m_iRRVScale));
		limitMVRangeToExtendedBBHalfPel (xRefR,yRefR,prctMVLimit,(BLOCK_SIZE *m_iRRVScale));
		limitMVRangeToExtendedBBHalfPel (xRefL,yRefL,prctMVLimit,(BLOCK_SIZE *m_iRRVScale));
//      limitMVRangeToExtendedBBHalfPel (xRefC,yRefC,prctMVLimit,BLOCK_SIZE);
//      limitMVRangeToExtendedBBHalfPel (xRefT,yRefT,prctMVLimit,BLOCK_SIZE);
//      limitMVRangeToExtendedBBHalfPel (xRefB,yRefB,prctMVLimit,BLOCK_SIZE);
//      limitMVRangeToExtendedBBHalfPel (xRefR,yRefR,prctMVLimit,BLOCK_SIZE);
//      limitMVRangeToExtendedBBHalfPel (xRefL,yRefL,prctMVLimit,BLOCK_SIZE);
// ~RRV
      Bool bXSubPxlC = (xRefC & 1), bYSubPxlC = (yRefC & 1),
        bXSubPxlT = (xRefT & 1), bYSubPxlT = (yRefT & 1),
        bXSubPxlB = (xRefB & 1), bYSubPxlB = (yRefB & 1),
        bXSubPxlR = (xRefR & 1), bYSubPxlR = (yRefR & 1),
        bXSubPxlL = (xRefL & 1), bYSubPxlL = (yRefL & 1);
		
      // 5 starting pos. in Zoomed Ref. Frames
      const PixelC* ppxliPrevYC = ppxlcRefLeftTop + ((yRefC >> 1) + EXPANDY_REF_FRAME) * m_iFrameWidthY + (xRefC >> 1) + EXPANDY_REF_FRAME;
      const PixelC* ppxliPrevYT = ppxlcRefLeftTop + ((yRefT >> 1) + EXPANDY_REF_FRAME) * m_iFrameWidthY + (xRefT >> 1) + EXPANDY_REF_FRAME;
      const PixelC* ppxliPrevYB = ppxlcRefLeftTop + ((yRefB >> 1) + EXPANDY_REF_FRAME) * m_iFrameWidthY + (xRefB >> 1) + EXPANDY_REF_FRAME;
      const PixelC* ppxliPrevYR = ppxlcRefLeftTop + ((yRefR >> 1) + EXPANDY_REF_FRAME) * m_iFrameWidthY + (xRefR >> 1) + EXPANDY_REF_FRAME;
      const PixelC* ppxliPrevYL = ppxlcRefLeftTop + ((yRefL >> 1) + EXPANDY_REF_FRAME) * m_iFrameWidthY + (xRefL >> 1) + EXPANDY_REF_FRAME;
      UInt *pWghtC, *pWghtT, *pWghtB, *pWghtR, *pWghtL;
// RRV modification
		UInt* uiOvrlpPredY;
		UInt iOffset = (m_vopmd.RRVmode.iRRVOnOff) ? (128) : (32); 
	    if(m_vopmd.RRVmode.iRRVOnOff)
		{
			pWghtC = (UInt*) gWghtC_RRV;
			pWghtT = (UInt*) gWghtT_RRV;
			pWghtB = (UInt*) gWghtB_RRV + iOffset;
			pWghtR = (UInt*) gWghtR_RRV;
			pWghtL = (UInt*) gWghtL_RRV;
			uiOvrlpPredY = gOvrlpPredY_x2;
		}
		else
		{
			pWghtC = (UInt*) gWghtC;
			pWghtT = (UInt*) gWghtT;
			pWghtB = (UInt*) gWghtB + iOffset;
			pWghtR = (UInt*) gWghtR;
			pWghtL = (UInt*) gWghtL;
			uiOvrlpPredY = gOvrlpPredY;
		}
//      pWghtC = (UInt*) gWghtC;
//      pWghtT = (UInt*) gWghtT;
//      pWghtB = (UInt*) gWghtB + 32;
//      pWghtR = (UInt*) gWghtR;
//      pWghtL = (UInt*) gWghtL;
//      UInt* uiOvrlpPredY = gOvrlpPredY;
// ~RRV

// RRV modification
		UInt	iBSize, iHBSize, iShiftSize;
		iBSize = (m_vopmd.RRVmode.iRRVOnOff) ? (16) : (8); 
		iHBSize = (m_vopmd.RRVmode.iRRVOnOff) ? (8) : (4); 
		iShiftSize = (m_vopmd.RRVmode.iRRVOnOff) ? (3) : (2); 

		if (bXSubPxlC && bYSubPxlC)
			bilnrMCVH (uiOvrlpPredY, ppxliPrevYC, pWghtC, 0, iBSize, 0, iBSize, FALSE);
		else if (bXSubPxlC && !bYSubPxlC)
			bilnrMCH (uiOvrlpPredY, ppxliPrevYC, pWghtC, 0, iBSize, 0, iBSize, FALSE);
		else if (!bXSubPxlC && bYSubPxlC)
			bilnrMCV (uiOvrlpPredY, ppxliPrevYC, pWghtC, 0, iBSize, 0, iBSize, FALSE);
		else // (!bXSubPxlC && !bYSubPxlC)
			bilnrMC (uiOvrlpPredY, ppxliPrevYC, pWghtC, 0, iBSize, 0, iBSize, FALSE);

		if (bXSubPxlT && bYSubPxlT)
			bilnrMCVH (uiOvrlpPredY, ppxliPrevYT, pWghtT, 0, iBSize, 0, iHBSize, TRUE);
		else if (bXSubPxlT && !bYSubPxlT)
			bilnrMCH (uiOvrlpPredY, ppxliPrevYT, pWghtT, 0, iBSize, 0, iHBSize, TRUE);
		else if (!bXSubPxlT && bYSubPxlT)
			bilnrMCV (uiOvrlpPredY, ppxliPrevYT, pWghtT, 0, iBSize, 0, iHBSize, TRUE);
		else // (!bXSubPxlT && !bYSubPxlT)
			bilnrMC (uiOvrlpPredY, ppxliPrevYT, pWghtT, 0, iBSize, 0, iHBSize, TRUE);

		if (bXSubPxlB && bYSubPxlB)
			bilnrMCVH (&uiOvrlpPredY [iOffset], ppxliPrevYB + (m_iFrameWidthY << iShiftSize), pWghtB, 0, iBSize, iHBSize, iBSize, TRUE);
		else if (bXSubPxlB && !bYSubPxlB)
			bilnrMCH (&uiOvrlpPredY [iOffset], ppxliPrevYB + (m_iFrameWidthY << iShiftSize), pWghtB, 0, iBSize, iHBSize, iBSize, TRUE);
		else if (!bXSubPxlB && bYSubPxlB)
			bilnrMCV (&uiOvrlpPredY [iOffset], ppxliPrevYB + (m_iFrameWidthY << iShiftSize), pWghtB, 0, iBSize, iHBSize, iBSize, TRUE);
		else // (!bXSubPxlB && !bYSubPxlB)
			bilnrMC (&uiOvrlpPredY [iOffset], ppxliPrevYB + (m_iFrameWidthY << iShiftSize), pWghtB, 0, iBSize, iHBSize, iBSize, TRUE);

		if (bXSubPxlR && bYSubPxlR)
			bilnrMCVH (uiOvrlpPredY, ppxliPrevYR, pWghtR, iHBSize, iBSize, 0, iBSize, TRUE);
		else if (bXSubPxlR && !bYSubPxlR)
			bilnrMCH (uiOvrlpPredY, ppxliPrevYR, pWghtR, iHBSize, iBSize, 0, iBSize, TRUE);
		else if (!bXSubPxlR && bYSubPxlR)
			bilnrMCV (uiOvrlpPredY, ppxliPrevYR, pWghtR, iHBSize, iBSize, 0, iBSize, TRUE);
		else // (!bXSubPxlR && !bYSubPxlR)
			bilnrMC (uiOvrlpPredY, ppxliPrevYR, pWghtR, iHBSize, iBSize, 0, iBSize, TRUE);

		if (bXSubPxlL && bYSubPxlL)
			bilnrMCVH (uiOvrlpPredY, ppxliPrevYL, pWghtL, 0, iHBSize, 0, iBSize, TRUE);
		else if (bXSubPxlL && !bYSubPxlL)
			bilnrMCH (uiOvrlpPredY, ppxliPrevYL, pWghtL, 0, iHBSize, 0, iBSize, TRUE);
		else if (!bXSubPxlL && bYSubPxlL)
			bilnrMCV (uiOvrlpPredY, ppxliPrevYL, pWghtL, 0, iHBSize, 0, iBSize, TRUE);
		else // (!bXSubPxlL && !bYSubPxlL)
			bilnrMC (uiOvrlpPredY, ppxliPrevYL, pWghtL, 0, iHBSize, 0, iBSize, TRUE);

/*
      if (bXSubPxlC && bYSubPxlC)
        bilnrMCVH (uiOvrlpPredY, ppxliPrevYC, pWghtC, 0, 8, 0, 8, FALSE);
      else if (bXSubPxlC && !bYSubPxlC)
        bilnrMCH (uiOvrlpPredY, ppxliPrevYC, pWghtC, 0, 8, 0, 8, FALSE);
      else if (!bXSubPxlC && bYSubPxlC)
        bilnrMCV (uiOvrlpPredY, ppxliPrevYC, pWghtC, 0, 8, 0, 8, FALSE);
      else // (!bXSubPxlC && !bYSubPxlC)
        bilnrMC (uiOvrlpPredY, ppxliPrevYC, pWghtC, 0, 8, 0, 8, FALSE);

      if (bXSubPxlT && bYSubPxlT)
        bilnrMCVH (uiOvrlpPredY, ppxliPrevYT, pWghtT, 0, 8, 0, 4, TRUE);
      else if (bXSubPxlT && !bYSubPxlT)
        bilnrMCH (uiOvrlpPredY, ppxliPrevYT, pWghtT, 0, 8, 0, 4, TRUE);
      else if (!bXSubPxlT && bYSubPxlT)
        bilnrMCV (uiOvrlpPredY, ppxliPrevYT, pWghtT, 0, 8, 0, 4, TRUE);
      else // (!bXSubPxlT && !bYSubPxlT)
        bilnrMC (uiOvrlpPredY, ppxliPrevYT, pWghtT, 0, 8, 0, 4, TRUE);

      if (bXSubPxlB && bYSubPxlB)
        bilnrMCVH (&uiOvrlpPredY [32], ppxliPrevYB + (m_iFrameWidthY << 2), pWghtB, 0, 8, 4, 8, TRUE);
      else if (bXSubPxlB && !bYSubPxlB)
        bilnrMCH (&uiOvrlpPredY [32], ppxliPrevYB + (m_iFrameWidthY << 2), pWghtB, 0, 8, 4, 8, TRUE);
      else if (!bXSubPxlB && bYSubPxlB)
        bilnrMCV (&uiOvrlpPredY [32], ppxliPrevYB + (m_iFrameWidthY << 2), pWghtB, 0, 8, 4, 8, TRUE);
      else // (!bXSubPxlB && !bYSubPxlB)
        bilnrMC (&uiOvrlpPredY [32], ppxliPrevYB + (m_iFrameWidthY << 2), pWghtB, 0, 8, 4, 8, TRUE);

      if (bXSubPxlR && bYSubPxlR)
        bilnrMCVH (uiOvrlpPredY, ppxliPrevYR, pWghtR, 4, 8, 0, 8, TRUE);
      else if (bXSubPxlR && !bYSubPxlR)
        bilnrMCH (uiOvrlpPredY, ppxliPrevYR, pWghtR, 4, 8, 0, 8, TRUE);
      else if (!bXSubPxlR && bYSubPxlR)
        bilnrMCV (uiOvrlpPredY, ppxliPrevYR, pWghtR, 4, 8, 0, 8, TRUE);
      else // (!bXSubPxlR && !bYSubPxlR)
        bilnrMC (uiOvrlpPredY, ppxliPrevYR, pWghtR, 4, 8, 0, 8, TRUE);

      if (bXSubPxlL && bYSubPxlL)
        bilnrMCVH (uiOvrlpPredY, ppxliPrevYL, pWghtL, 0, 4, 0, 8, TRUE);
      else if (bXSubPxlL && !bYSubPxlL)
        bilnrMCH (uiOvrlpPredY, ppxliPrevYL, pWghtL, 0, 4, 0, 8, TRUE);
      else if (!bXSubPxlL && bYSubPxlL)
        bilnrMCV (uiOvrlpPredY, ppxliPrevYL, pWghtL, 0, 4, 0, 8, TRUE);
      else // (!bXSubPxlL && !bYSubPxlL)
        bilnrMC (uiOvrlpPredY, ppxliPrevYL, pWghtL, 0, 4, 0, 8, TRUE);
*/
// ~RRV
// RRV modification
      PixelC* ppxliPredY = ppxlcPredMB + dxc + dyc * (MB_SIZE *m_iRRVScale);  // Starting of Pred. Frame
	  Int ix, iy;
      for (iy = 0; iy < (BLOCK_SIZE *m_iRRVScale); iy++){
        for (ix = 0; ix < (BLOCK_SIZE *m_iRRVScale); ix++)
          *ppxliPredY++= (*uiOvrlpPredY++ + 4) >> 3;
        ppxliPredY += (BLOCK_SIZE *m_iRRVScale);
      }
//      PixelC* ppxliPredY = ppxlcPredMB + dxc + dyc * MB_SIZE;  // Starting of Pred. Frame
//      for (UInt iy = 0; iy < BLOCK_SIZE; iy++){
//        for (UInt ix = 0; ix < BLOCK_SIZE; ix++)
//          *ppxliPredY++= (*uiOvrlpPredY++ + 4) >> 3;
//        ppxliPredY += BLOCK_SIZE;
//      }
// ~RRV
    }
  }
}

Void CVideoObject::bilnrMCVH (UInt* PredY, const PixelC* ppxliPrevYC, UInt* pMWght, UInt xlow, UInt xhigh, UInt ylow, UInt yhigh, Bool bAdd)
{
	const PixelC* ppxliPrevYCBot = ppxliPrevYC + m_iFrameWidthY;
	if (bAdd)
		for (UInt iy = ylow; iy < yhigh; iy++) {
			for (UInt ix = xlow; ix < xhigh; ix++)
//				PredY [ix] += ((UInt) (ppxliPrevYC [ix] + ppxliPrevYC [ix + 1] + ppxliPrevYCBot [ix] + ppxliPrevYCBot[ix+1] + 2) >> 2) * pMWght [ix];
				PredY [ix] += ((UInt) (ppxliPrevYC [ix] + ppxliPrevYC [ix + 1] + ppxliPrevYCBot [ix] + ppxliPrevYCBot[ix+1] + 2 - m_vopmd.iRoundingControl) >> 2) * pMWght [ix];
			ppxliPrevYC += m_iFrameWidthY;
			ppxliPrevYCBot += m_iFrameWidthY;
// RRV modification
			PredY += (BLOCK_SIZE *m_iRRVScale);
			pMWght += (BLOCK_SIZE *m_iRRVScale);
//			PredY += BLOCK_SIZE;
//			pMWght += BLOCK_SIZE;
// ~RRV
		}
	else
		for (UInt iy = ylow; iy < yhigh; iy++) {
			for (UInt ix = xlow; ix < xhigh; ix++)
//				PredY [ix] = ((UInt) (ppxliPrevYC [ix] + ppxliPrevYC [ix + 1] + ppxliPrevYCBot [ix] + ppxliPrevYCBot[ix+1] + 2) >> 2) * pMWght [ix];
				PredY [ix] = ((UInt) (ppxliPrevYC [ix] + ppxliPrevYC [ix + 1] + ppxliPrevYCBot [ix] + ppxliPrevYCBot[ix+1] + 2 - m_vopmd.iRoundingControl) >> 2) * pMWght [ix];
			ppxliPrevYC += m_iFrameWidthY;
			ppxliPrevYCBot += m_iFrameWidthY;
// RRV modification
			PredY += (BLOCK_SIZE *m_iRRVScale);
			pMWght += (BLOCK_SIZE *m_iRRVScale);
//			PredY += BLOCK_SIZE;
//			pMWght += BLOCK_SIZE;
// ~RRV
		}
}

Void CVideoObject::bilnrMCV (UInt* PredY, const PixelC* ppxliPrevYC, UInt* pMWght, UInt xlow, UInt xhigh, UInt ylow, UInt yhigh, Bool bAdd)
{
	const PixelC* ppxliPrevYCBot = ppxliPrevYC + m_iFrameWidthY;
	if (bAdd)
		for (UInt iy = ylow; iy < yhigh; iy++) {
			for (UInt ix = xlow; ix < xhigh; ix++)
//				PredY [ix] += ((UInt) (ppxliPrevYC [ix] + ppxliPrevYCBot [ix] + 1) >> 1) * pMWght [ix];
 				PredY [ix] += ((UInt) (ppxliPrevYC [ix] + ppxliPrevYCBot [ix] + 1 - m_vopmd.iRoundingControl) >> 1) * pMWght [ix];
			ppxliPrevYC += m_iFrameWidthY;
			ppxliPrevYCBot += m_iFrameWidthY;
// RRV modification
			PredY += (BLOCK_SIZE *m_iRRVScale);
			pMWght += (BLOCK_SIZE *m_iRRVScale);
//			PredY += BLOCK_SIZE;
//			pMWght += BLOCK_SIZE;
// ~RRV
		}
	else
		for (UInt iy = ylow; iy < yhigh; iy++) {
			for (UInt ix = xlow; ix < xhigh; ix++)
//				PredY [ix] = ((UInt) (ppxliPrevYC [ix] + ppxliPrevYCBot [ix] + 1) >> 1) * pMWght [ix];
				PredY [ix] = ((UInt) (ppxliPrevYC [ix] + ppxliPrevYCBot [ix] + 1 - m_vopmd.iRoundingControl) >> 1) * pMWght [ix];
			ppxliPrevYC += m_iFrameWidthY;
			ppxliPrevYCBot += m_iFrameWidthY;
// RRV modification
			PredY += (BLOCK_SIZE *m_iRRVScale);
			pMWght += (BLOCK_SIZE *m_iRRVScale);
//			PredY += BLOCK_SIZE;
//			pMWght += BLOCK_SIZE;
// ~RRV
		}
}

Void CVideoObject::bilnrMCH (UInt* PredY, const PixelC* ppxliPrevYC, UInt* pMWght, UInt xlow, UInt xhigh, UInt ylow, UInt yhigh, Bool bAdd)
{
	//PixelC* ppxliPrevYCBot = ppxliPrevYC + m_iFrameWidthY;
	if (bAdd)
		for (UInt iy = ylow; iy < yhigh; iy++) {
			for (UInt ix = xlow; ix < xhigh; ix++)
//				PredY [ix] += ((UInt) (ppxliPrevYC [ix] + ppxliPrevYC [ix + 1] + 1) >> 1) * pMWght [ix];
				PredY [ix] += ((UInt) (ppxliPrevYC [ix] + ppxliPrevYC [ix + 1] + 1 - m_vopmd.iRoundingControl) >> 1) * pMWght [ix];
			ppxliPrevYC += m_iFrameWidthY;
// RRV modification
			PredY += (BLOCK_SIZE *m_iRRVScale);
			pMWght += (BLOCK_SIZE *m_iRRVScale);
//			PredY += BLOCK_SIZE;
//			pMWght += BLOCK_SIZE;
// ~RRV
		}
	else
		for (UInt iy = ylow; iy < yhigh; iy++) {
			for (UInt ix = xlow; ix < xhigh; ix++)
//				PredY [ix] = ((UInt) (ppxliPrevYC [ix] + ppxliPrevYC [ix + 1] + 1) >> 1) * pMWght [ix];
				PredY [ix] = ((UInt) (ppxliPrevYC [ix] + ppxliPrevYC [ix + 1] + 1 - m_vopmd.iRoundingControl) >> 1) * pMWght [ix];
			ppxliPrevYC += m_iFrameWidthY;
// RRV modification
			PredY += (BLOCK_SIZE *m_iRRVScale);
			pMWght += (BLOCK_SIZE *m_iRRVScale);
//			PredY += BLOCK_SIZE;
//			pMWght += BLOCK_SIZE;
// ~RRV
		}
}

Void CVideoObject::bilnrMC (UInt* PredY, const PixelC* ppxliPrevYC, UInt* pMWght, UInt xlow, UInt xhigh, UInt ylow, UInt yhigh, Bool bAdd)
{
	//PixelC* ppxliPrevYCBot = ppxliPrevYC + m_iFrameWidthY;
	if (bAdd)
		for (UInt iy = ylow; iy < yhigh; iy++) {
			for (UInt ix = xlow; ix < xhigh; ix++)
				PredY [ix] += (UInt) ppxliPrevYC [ix] * pMWght[ix];
			ppxliPrevYC += m_iFrameWidthY;
// RRV modification
			PredY += (BLOCK_SIZE *m_iRRVScale);
			pMWght += (BLOCK_SIZE *m_iRRVScale);
//			PredY += BLOCK_SIZE;
//			pMWght += BLOCK_SIZE;
// ~RRV
		}
	else
		for (UInt iy = ylow; iy < yhigh; iy++) {
			for (UInt ix = xlow; ix < xhigh; ix++)
				PredY [ix] = (UInt) ppxliPrevYC [ix] * pMWght [ix];
			ppxliPrevYC += m_iFrameWidthY;
// RRV modification
			PredY += (BLOCK_SIZE *m_iRRVScale);
			pMWght += (BLOCK_SIZE *m_iRRVScale);
//			PredY += BLOCK_SIZE;
//			pMWght += BLOCK_SIZE;
// ~RRV
		}
}

Void CVideoObject::motionCompBY (
	PixelC* ppxlcPred, // can be either Y or A
	const PixelC* ppxlcRefLeftTop,
	CoordI iXRef, CoordI iYRef // x + mvX in full pel unit
)
{
	CoordI iY;
	const PixelC* ppxlcRef = ppxlcRefLeftTop + 
							(iYRef + EXPANDY_REF_FRAME) * m_iFrameWidthY + iXRef + EXPANDY_REF_FRAME;
    // bugfix: use the proper bounding rect (!), 981028 mwi
    Int iLeftBound;
    Int iRightBound;
    Int iTopBound;
    Int iBottomBound;
    if (m_vopmd.vopPredType == BVOP && m_vopmd.fShapeBPredDir == B_BACKWARD) {
      iLeftBound = max(0,m_rctRefVOPY1.left - iXRef);
      iRightBound = max(0,m_rctRefVOPY1.right - iXRef);
      iTopBound = max(0,m_rctRefVOPY1.top - iYRef);
      iBottomBound = max(0,m_rctRefVOPY1.bottom - iYRef);
    }
    else {
      iLeftBound = max(0,m_rctRefVOPY0.left - iXRef);
      iRightBound = max(0,m_rctRefVOPY0.right - iXRef);
      iTopBound = max(0,m_rctRefVOPY0.top - iYRef);
      iBottomBound = max(0,m_rctRefVOPY0.bottom - iYRef);
    }
    // ~bugfix: use the proper bounding rect

	iLeftBound = min(MC_BAB_SIZE,iLeftBound);
	iRightBound = min(MC_BAB_SIZE,iRightBound);
	iTopBound = min(MC_BAB_SIZE,iTopBound);
	iBottomBound = min(MC_BAB_SIZE,iBottomBound);

	Int iHeightMax = iBottomBound-iTopBound;
	Int iWidthMax  = iRightBound-iLeftBound;

	if (iHeightMax == MC_BAB_SIZE && iWidthMax == MC_BAB_SIZE)	{
		for (iY = 0; iY < MC_BAB_SIZE; iY++) {
			memcpy (ppxlcPred, ppxlcRef, MC_BAB_SIZE*sizeof(PixelC));
			ppxlcRef  += m_iFrameWidthY;
			ppxlcPred += MC_BAB_SIZE;
		}
	} else if(iWidthMax == 0 || iHeightMax==0)
		for (iY = 0; iY < MC_BAB_SIZE; iY++) {
			memset (ppxlcPred, 0, MC_BAB_SIZE*sizeof(PixelC));
			ppxlcPred += MC_BAB_SIZE;
		}
	else {
		for (iY = 0; iY < MC_BAB_SIZE; iY++) {
			if(iY<iTopBound || iY>=iBottomBound)
				memset (ppxlcPred, 0, MC_BAB_SIZE*sizeof(PixelC));  // clear row
			else
			{
				if(iLeftBound>0)
					memset (ppxlcPred, 0, iLeftBound*sizeof(PixelC)); // clear left hand span
				if(iRightBound<MC_BAB_SIZE)
					memset (ppxlcPred+iRightBound, 0, (MC_BAB_SIZE-iRightBound)*sizeof(PixelC)); // right span
				memcpy (ppxlcPred+iLeftBound, ppxlcRef+iLeftBound, iWidthMax*sizeof(PixelC)); // copy middle region
			}

			ppxlcRef += m_iFrameWidthY;
			ppxlcPred += MC_BAB_SIZE;
		}
	}
}

//OBSS_SAIT_991015
Void CVideoObject::motionCompLowerBY (
	PixelC* ppxlcPred, // can be either Y or A
	const PixelC* ppxlcRefLeftTop,
	CoordI iXRef, CoordI iYRef // x + mvX in full pel unit
)
{
	CoordI iY;
	const PixelC* ppxlcRef = ppxlcRefLeftTop + 
							(iYRef + EXPANDY_REF_FRAME) * m_iFrameWidthY + iXRef + EXPANDY_REF_FRAME;
	Int iLeftBound = max(0,m_rctRefVOPY1.left - iXRef);
	Int iRightBound = max(0,m_rctRefVOPY1.right - iXRef);
	Int iTopBound = max(0,m_rctRefVOPY1.top - iYRef);
	Int iBottomBound = max(0,m_rctRefVOPY1.bottom - iYRef);
	iLeftBound = min(MC_BAB_SIZE,iLeftBound);
	iRightBound = min(MC_BAB_SIZE,iRightBound);
	iTopBound = min(MC_BAB_SIZE,iTopBound);
	iBottomBound = min(MC_BAB_SIZE,iBottomBound);

	Int iHeightMax = iBottomBound-iTopBound;
	Int iWidthMax  = iRightBound-iLeftBound;

	if (iHeightMax == MC_BAB_SIZE && iWidthMax == MC_BAB_SIZE)	{
		for (iY = 0; iY < MC_BAB_SIZE; iY++) {
			memcpy (ppxlcPred, ppxlcRef, MC_BAB_SIZE*sizeof(PixelC));
			ppxlcRef  += m_iFrameWidthY;
			ppxlcPred += MC_BAB_SIZE;
		}
	} else if(iWidthMax == 0 || iHeightMax==0)
		for (iY = 0; iY < MC_BAB_SIZE; iY++) {
			memset (ppxlcPred, 0, MC_BAB_SIZE*sizeof(PixelC));
			ppxlcPred += MC_BAB_SIZE;
		}
	else {
		for (iY = 0; iY < MC_BAB_SIZE; iY++) {
			if(iY<iTopBound || iY>=iBottomBound)
				memset (ppxlcPred, 0, MC_BAB_SIZE*sizeof(PixelC));  // clear row
			else
			{
				if(iLeftBound>0)
					memset (ppxlcPred, 0, iLeftBound*sizeof(PixelC)); // clear left hand span
				if(iRightBound<MC_BAB_SIZE)
					memset (ppxlcPred+iRightBound, 0, (MC_BAB_SIZE-iRightBound)*sizeof(PixelC)); // right span
				memcpy (ppxlcPred+iLeftBound, ppxlcRef+iLeftBound, iWidthMax*sizeof(PixelC)); // copy middle region
			}

			ppxlcRef += m_iFrameWidthY;
			ppxlcPred += MC_BAB_SIZE;
		}
	}
}
//~OBSS_SAIT_991015

Void CVideoObject::copyFromRefToCurrQ (
	const CVOPU8YUVBA* pvopcRef, // reference VOP
	CoordI x, CoordI y, 
	PixelC* ppxlcCurrQMBY, PixelC* ppxlcCurrQMBU, PixelC* ppxlcCurrQMBV,
	CRct *prctMVLimit
)
{
	// needs limiting to reference area bounding box
// RRV modification
	limitMVRangeToExtendedBBFullPel(x,y,prctMVLimit,(MB_SIZE *m_iRRVScale));
//	limitMVRangeToExtendedBBFullPel(x,y,prctMVLimit,MB_SIZE);
// ~RRV	
	
	Int iOffsetY = (y + EXPANDY_REF_FRAME) * m_iFrameWidthY + x + EXPANDY_REF_FRAME;
	Int iOffsetUV = (y / 2 + EXPANDUV_REF_FRAME) * m_iFrameWidthUV + x / 2 + EXPANDUV_REF_FRAME;
	const PixelC* ppxlcRefMBY = pvopcRef->pixelsY () + iOffsetY;
	const PixelC* ppxlcRefMBU = pvopcRef->pixelsU () + iOffsetUV;
	const PixelC* ppxlcRefMBV = pvopcRef->pixelsV () + iOffsetUV;

	CoordI iY;
// RRV modification
	for (iY = 0; iY < (BLOCK_SIZE *m_iRRVScale); iY++) {
		memcpy (ppxlcCurrQMBY, ppxlcRefMBY, 
				(MB_SIZE *m_iRRVScale)*sizeof(PixelC));
		memcpy (ppxlcCurrQMBU, ppxlcRefMBU, 
				(BLOCK_SIZE *m_iRRVScale)*sizeof(PixelC));
		memcpy (ppxlcCurrQMBV, ppxlcRefMBV,
				(BLOCK_SIZE *m_iRRVScale)*sizeof(PixelC));
//	for (iY = 0; iY < BLOCK_SIZE; iY++) {
//		memcpy (ppxlcCurrQMBY, ppxlcRefMBY, MB_SIZE*sizeof(PixelC));
//		memcpy (ppxlcCurrQMBU, ppxlcRefMBU, BLOCK_SIZE*sizeof(PixelC));
//		memcpy (ppxlcCurrQMBV, ppxlcRefMBV, BLOCK_SIZE*sizeof(PixelC));
// ~RRV

		ppxlcCurrQMBY += m_iFrameWidthY; ppxlcRefMBY += m_iFrameWidthY;
		ppxlcCurrQMBU += m_iFrameWidthUV; ppxlcRefMBU += m_iFrameWidthUV;
		ppxlcCurrQMBV += m_iFrameWidthUV; ppxlcRefMBV += m_iFrameWidthUV;

// RRV modifcation
		memcpy (ppxlcCurrQMBY, ppxlcRefMBY, 
				(MB_SIZE *m_iRRVScale)*sizeof(PixelC));
//		memcpy (ppxlcCurrQMBY, ppxlcRefMBY, MB_SIZE*sizeof(PixelC));
// ~RRV
		ppxlcCurrQMBY += m_iFrameWidthY; ppxlcRefMBY += m_iFrameWidthY;
	}
}

Void CVideoObject::copyFromRefToCurrQ_WithShape (
	const CVOPU8YUVBA* pvopcRef, // reference VOP
	CoordI x, CoordI y, 
	PixelC* ppxlcCurrQMBY, PixelC* ppxlcCurrQMBU, PixelC* ppxlcCurrQMBV, PixelC* ppxlcCurrQMBBY
)
{
	Int iOffsetY = (y + EXPANDY_REF_FRAME) * m_iFrameWidthY + x + EXPANDY_REF_FRAME;
	Int iOffsetUV = (y / 2 + EXPANDUV_REF_FRAME) * m_iFrameWidthUV + x / 2 + EXPANDUV_REF_FRAME;
	const PixelC* ppxlcRefMBY = pvopcRef->pixelsY () + iOffsetY;
	const PixelC* ppxlcRefMBBY = pvopcRef->pixelsBY () + iOffsetY;
	const PixelC* ppxlcRefMBU = pvopcRef->pixelsU () + iOffsetUV;
	const PixelC* ppxlcRefMBV = pvopcRef->pixelsV () + iOffsetUV;

	CoordI iY;
	for (iY = 0; iY < BLOCK_SIZE; iY++) {
		memcpy (ppxlcCurrQMBY, ppxlcRefMBY, MB_SIZE*sizeof(PixelC));
		memcpy (ppxlcCurrQMBBY, ppxlcRefMBBY, MB_SIZE*sizeof(PixelC));
		memcpy (ppxlcCurrQMBU, ppxlcRefMBU, BLOCK_SIZE*sizeof(PixelC));
		memcpy (ppxlcCurrQMBV, ppxlcRefMBV, BLOCK_SIZE*sizeof(PixelC));

		ppxlcCurrQMBY += m_iFrameWidthY; ppxlcRefMBY += m_iFrameWidthY;
		ppxlcCurrQMBBY += m_iFrameWidthY; ppxlcRefMBBY += m_iFrameWidthY;
		ppxlcCurrQMBU += m_iFrameWidthUV; ppxlcRefMBU += m_iFrameWidthUV;
		ppxlcCurrQMBV += m_iFrameWidthUV; ppxlcRefMBV += m_iFrameWidthUV;

		memcpy (ppxlcCurrQMBY, ppxlcRefMBY, MB_SIZE*sizeof(PixelC));
		ppxlcCurrQMBY += m_iFrameWidthY; ppxlcRefMBY += m_iFrameWidthY;
		memcpy (ppxlcCurrQMBBY, ppxlcRefMBBY, MB_SIZE*sizeof(PixelC));
		ppxlcCurrQMBBY += m_iFrameWidthY; ppxlcRefMBBY += m_iFrameWidthY;
	}
}

Void CVideoObject::copyAlphaFromRefToCurrQ (
	const CVOPU8YUVBA* pvopcRef, // reference VOP
	CoordI x, CoordI y, 
	PixelC** pppxlcCurrQMBA,
	CRct *prctMVLimit
)
{
	// needs limiting to reference area bounding box
	limitMVRangeToExtendedBBFullPel(x,y,prctMVLimit,MB_SIZE);

	Int iOffsetY = (y + EXPANDY_REF_FRAME) * m_iFrameWidthY + x + EXPANDY_REF_FRAME;

	CoordI iY;

	for(Int iAuxComp=0;iAuxComp<m_volmd.iAuxCompCount;iAuxComp++) { // MAC (SB) 26-Nov-99
		const PixelC* ppxlcRefMBA = pvopcRef->pixelsA (iAuxComp) + iOffsetY; 
		PixelC* ppxlcCurrQMBA = pppxlcCurrQMBA[iAuxComp];
		for (iY = 0; iY < MB_SIZE; iY++) {
			memcpy (ppxlcCurrQMBA, ppxlcRefMBA, MB_SIZE*sizeof(PixelC));
			ppxlcCurrQMBA += m_iFrameWidthY; 
			ppxlcRefMBA += m_iFrameWidthY;
		}
	}
}

/*************************************************************************

This software module was originally developed by 

	Ming-Chieh Lee (mingcl@microsoft.com), Microsoft Corporation
	Wei-ge Chen (wchen@microsoft.com), Microsoft Corporation
	Bruce Lin (blin@microsoft.com), Microsoft Corporation
	(date: March, 1996)
and edited by
        Wei Wu (weiwu@stallion.risc.rockwell.com) Rockwell Science Center
and edited by Xuemin Chen (xchen@gi.com) General Instrument Corp.)
and also edited by
    Mathias Wien (wien@ient.rwth-aachen.de) RWTH Aachen / Robert BOSCH GmbH

and also edited by
	Yoshinori Suzuki (Hitachi, Ltd.)

and also edited by
	Prabhudev Irappa Hosur (pn188359@ntu.edu.sg) NTU singapore

and also edited by
	Hideaki Kimata (NTT)

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

	motEst.cpp

Abstract:

	Motion estimation routines.

Revision History:
	Dec 20, 1997:	Interlaced tools added by NextLevel Systems
	Feb.16, 1999:     add Quarter Sample 
                            Mathias Wien (wien@ient.rwth-aachen.de) 
	Feb.24, 1999:	GMC added by Yoshinori Suzuki (Hitachi, Ltd.)
	May 9, 1999:	tm5 rate control by DemoGraFX, duhoff@mediaone.net (added by mwi)
	August 9, 1999:   Fast Motion Estimation added by
					Prabhudev Irappa Hosur (pn188359@ntu.edu.sg), NTU, S'pore
	Aug.24, 1999:   NEWPRED added by Hideaki Kimata (NTT) 
	Aug.31, 1999:   QP pred. for GMC/LMC decision in rate control cases
			added by Yoshinori Suzuki (Hitachi, Ltd.)
	Sep.06	1999 : RRV added by Eishi Morimatsu (Fujitsu Laboratories Ltd.) 
    March 30 , 2000 : Diamond Search removed by Prabhudev Irappa Hosur (NTU)
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

// NEWPRED
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

#define FAVORZERO		129
#define FAVOR_DIRECT	129
#define FAVOR_INTER		512
#define FAVOR_16x16		129
#define FAVOR_FIELD		 65			// P-VOP favor field over 8x8 due to fewer MVs
// RRV insertion
#define FAVORZERO_RRV		513
#define FAVOR_DIRECT_RRV	513
#define FAVOR_INTER_RRV		2048
#define FAVOR_32x32			513
#define FAVOR_FIELD_RRV		65	
// ~RRV 

inline Int minimum (Int a, Int b, Int c, Int d)
{
	if (a <= b && a <= c && a <= d)
		return a;
	else if (b <= a && b <= c && b <= d)
		return b;
	else if (c <= a && c <= b && c <= d)
		return c;
	else
		return d;
}

Void CVideoObjectEncoder::motionEstPVOP ()
{
	m_iMAD = 0;
	CoordI y = 0; 
	CMBMode* pmbmd = m_rgmbmd;
	CMotionVector* pmv = m_rgmv;
	const PixelC* ppxlcOrigY = m_pvopcOrig->pixelsBoundY ();
	const PixelC* ppxlcRefY = (m_volmd.bOriginalForME ? m_pvopcRefOrig0 : m_pvopcRefQ0)->pixelsY () + m_iStartInRefToCurrRctY;
	Int iMBX, iMBY;

// GMC_V2
        if ((m_uiSprite == 2) && (m_vopmd.vopPredType == SPRITE)) {
                CMBMode* pmbmdRef = m_rgmbmdRef;
                Int iqp_count=0, iqp_total=0;
                for (iMBY = 0; iMBY < m_iNumMBYRef; iMBY++) {
                        for (iMBX = 0; iMBX < m_iNumMBXRef; iMBX++)        {
                                iqp_total += pmbmdRef -> m_stepSize;
                                iqp_count++;
                             pmbmdRef++;
                        }
                }
                m_uiGMCQP = (iqp_total + iqp_count/2)/iqp_count;
		printf("QP for GMC/LMC selection: %d\n",m_uiGMCQP);
        }
// ~GMC_V2

	if (m_iMVFileUsage == 1)
		readPVOPMVs();

// NEWPRED
	const PixelC* RefbufY = (m_volmd.bOriginalForME ? m_pvopcRefOrig0 : m_pvopcRefQ0)->pixelsY ();
	if(m_volmd.bNewpredEnable) {
		g_pNewPredEnc->CopyRefYtoBufY(RefbufY, m_rctRefFrameY);
	}
// ~NEWPRED

// RRV modification
	for(iMBY = 0; iMBY < m_iNumMBY;	iMBY++, y += (MB_SIZE *m_iRRVScale))
	{
//	for (iMBY = 0; iMBY < m_iNumMBY; iMBY++, y += MB_SIZE) {
// ~RRV
		const PixelC* ppxlcOrigMBY = ppxlcOrigY;
		const PixelC* ppxlcRefMBY = ppxlcRefY;
		CoordI x = 0;
// RRV modification
		for(iMBX = 0; iMBX < m_iNumMBX;	iMBX++, x += (MB_SIZE *m_iRRVScale))
		{
//		for (iMBX = 0; iMBX < m_iNumMBX; iMBX++, x += MB_SIZE)	{
// ~RRV

// NEWPRED
			if(m_volmd.bNewpredEnable) {
// RRV modification
				if(g_pNewPredEnc->CheckSlice((iMBX *m_iRRVScale), (iMBY *m_iRRVScale))){
//				if(g_pNewPredEnc->CheckSlice(iMBX, iMBY)){
// ~RRV

// RRV modification
					PixelC* RefpointY = (PixelC*) m_pvopcRefQ0->pixelsY () + m_iStartInRefToCurrRctY + iMBY * (MB_SIZE *m_iRRVScale) * m_rctRefFrameY.width;
					g_pNewPredEnc->ChangeRefOfSliceYUV((const PixelC* )RefpointY, RefbufY, (iMBX *m_iRRVScale),(iMBY *m_iRRVScale),m_rctRefFrameY,'Y');   
//					PixelC* RefpointY = (PixelC*) m_pvopcRefQ0->pixelsY () + m_iStartInRefToCurrRctY + iMBY * MB_SIZE * m_rctRefFrameY.width;
//					g_pNewPredEnc->ChangeRefOfSliceYUV((const PixelC* )RefpointY, RefbufY, iMBX,iMBY,m_rctRefFrameY,'Y');   
// ~RRV
					m_rctRefVOPZoom0 = m_rctRefVOPY0.upSampleBy2 ();
					biInterpolateY (m_pvopcRefQ0, m_rctRefVOPY0, m_puciRefQZoom0, m_rctRefVOPZoom0, m_vopmd.iRoundingControl);
				}
			}		
// ~NEWPRED

#ifdef __TRACE_AND_STATS_
			m_pbitstrmOut->trace (CSite (iMBX, iMBY), "MB_X_Y");
#endif // __TRACE_AND_STATS_
			if (m_volmd.volType == ENHN_LAYER && m_vopmd.iRefSelectCode == 3)	{
				motionEstMB_PVOP (x, y, pmv, pmbmd);
			}
			else {
				copyToCurrBuffY (ppxlcOrigMBY);
				pmbmd->m_bFieldDCT=0;
				m_iMAD += motionEstMB_PVOP (x, y, pmv, pmbmd, ppxlcRefMBY);
			}

#ifdef __TRACE_AND_STATS_
			m_pbitstrmOut->trace (CSite (iMBX, iMBY), "MB_X_Y");
			m_pbitstrmOut->trace (pmv [0], "MV16");
			m_pbitstrmOut->trace (pmv [1], "MV8");
			m_pbitstrmOut->trace (pmv [2], "MV8");
			m_pbitstrmOut->trace (pmv [3], "MV8");
			m_pbitstrmOut->trace (pmv [4], "MV8");
// INTERLACE
			if(pmbmd->m_bForwardTop)
				m_pbitstrmOut->trace (pmv [6], "MV16x8");
			else
				m_pbitstrmOut->trace (pmv [5], "MV16x8");
			if(pmbmd->m_bForwardBottom)
				m_pbitstrmOut->trace (pmv [8], "MV16x8");
			else
				m_pbitstrmOut->trace (pmv [7], "MV16x8");
// ~INTERLACE
#endif // __TRACE_AND_STATS_
			pmbmd++;
			pmv += PVOP_MV_PER_REF_PER_MB;
// RRV modification
			ppxlcOrigMBY += (MB_SIZE *m_iRRVScale);
			ppxlcRefMBY += (MB_SIZE *m_iRRVScale);
//			ppxlcOrigMBY += MB_SIZE;
//			ppxlcRefMBY += MB_SIZE;
// ~RRV
		}
		ppxlcOrigY += m_iFrameWidthYxMBSize;
		ppxlcRefY += m_iFrameWidthYxMBSize;
	}
// RRV insertion
	pmv	= m_rgmv;
	if(m_vopmd.RRVmode.iRRVOnOff == 1)
	{
		MotionVectorScalingDown (pmv, m_iNumMBX *m_iNumMBY,
								 PVOP_MV_PER_REF_PER_MB);
	}
// ~RRV

	if (m_iMVFileUsage == 2)
		writePVOPMVs();

// NEWPRED
	if(m_volmd.bNewpredEnable)
		g_pNewPredEnc->CopyBufYtoRefY(RefbufY, m_rctRefFrameY);
// ~NEWPRED

}

Void CVideoObjectEncoder::motionEstPVOP_WithShape ()
{
	m_iMAD = 0;
	CoordI y = m_rctCurrVOPY.top; 
	CMBMode* pmbmd = m_rgmbmd;
	CMotionVector* pmv = m_rgmv;
	const PixelC* ppxlcOrigY = m_pvopcOrig->pixelsBoundY ();
	const PixelC* ppxlcOrigBY = m_pvopcOrig->pixelsBoundBY ();
	const PixelC* ppxlcRefY = (m_volmd.bOriginalForME ? m_pvopcRefOrig0 : m_pvopcRefQ0)->pixelsY () + m_iStartInRefToCurrRctY;
	Int iMBX, iMBY;

// GMC_V2
        if ((m_uiSprite == 2) && (m_vopmd.vopPredType == SPRITE)) {
                CMBMode* pmbmdRef = m_rgmbmdRef;
                Int iqp_count=0, iqp_total=0;
                for (iMBY = 0; iMBY < m_iNumMBYRef; iMBY++) {
                        for (iMBX = 0; iMBX < m_iNumMBXRef; iMBX++)     {
                          if (pmbmdRef->m_rgTranspStatus [0] != ALL){
                                iqp_total += pmbmdRef -> m_stepSize;
                                iqp_count++;
                          }
                             pmbmdRef++;
                        }
                }
                m_uiGMCQP = (iqp_total + iqp_count/2)/iqp_count;
		printf("QP for GMC/LMC selection: %d\n",m_uiGMCQP);
        }
// ~GMC_V2

	if (m_iMVFileUsage == 1)
		readPVOPMVs();

	for (iMBY = 0; iMBY < m_iNumMBY; iMBY++, y += MB_SIZE) {
		const PixelC* ppxlcOrigMBY = ppxlcOrigY;
		const PixelC* ppxlcOrigMBBY = ppxlcOrigBY;
		const PixelC* ppxlcRefMBY = ppxlcRefY;
		CoordI x = m_rctCurrVOPY.left;
		for (iMBX = 0; iMBX < m_iNumMBX; iMBX++, x += MB_SIZE)	{
#ifdef __TRACE_AND_STATS_
			if(m_volmd.bShapeOnly==FALSE)
				m_pbitstrmOut->trace (CSite (iMBX, iMBY), "MB_X_Y");
#endif // __TRACE_AND_STATS_
			copyToCurrBuffWithShapeY (ppxlcOrigMBY, ppxlcOrigMBBY);
			decideTransparencyStatus (pmbmd, m_ppxlcCurrMBBY);
			if(m_volmd.bShapeOnly==FALSE)
			{
//OBSS_SAIT_991015
				if (m_volmd.volType == ENHN_LAYER && m_vopmd.iRefSelectCode == 3)
					motionEstMB_PVOP (x, y, pmv, pmbmd);
				else {
					if (pmbmd->m_rgTranspStatus [0] == NONE)
					{
	// new changes X. Chen
						pmbmd->m_bFieldDCT=0;
						m_iMAD += motionEstMB_PVOP (x, y, pmv, pmbmd, ppxlcRefMBY);
					}
					else if (pmbmd->m_rgTranspStatus [0] == PARTIAL)
					{
	// new changes X. Chen
						pmbmd->m_bFieldDCT=0;
						m_iMAD += motionEstMB_PVOP_WithShape (x, y, pmv, pmbmd, ppxlcRefMBY);
					}
				}
//~OBSS_SAIT_991015

#ifdef __TRACE_AND_STATS_
				m_pbitstrmOut->trace (CSite (iMBX, iMBY), "MB_X_Y");
				m_pbitstrmOut->trace (pmv [0], "MV16");
				m_pbitstrmOut->trace (pmv [1], "MV8");
				m_pbitstrmOut->trace (pmv [2], "MV8");
				m_pbitstrmOut->trace (pmv [3], "MV8");
				m_pbitstrmOut->trace (pmv [4], "MV8");
// INTERLACE
	// new changes
				if(pmbmd->m_bFieldDCT) {
					if(pmbmd->m_bForwardTop)
						m_pbitstrmOut->trace (pmv [6], "MV16x8");
					else
						m_pbitstrmOut->trace (pmv [5], "MV16x8");
					if(pmbmd->m_bForwardBottom)
						m_pbitstrmOut->trace (pmv [8], "MV16x8");
					else
						m_pbitstrmOut->trace (pmv [7], "MV16x8");
				}
	// end of new changes
// ~INTERLACE
#endif // __TRACE_AND_STATS_
			}
			pmbmd++;
			pmv += PVOP_MV_PER_REF_PER_MB;
			ppxlcOrigMBY += MB_SIZE;
			ppxlcOrigMBBY += MB_SIZE;
			ppxlcRefMBY += MB_SIZE;
		}
		ppxlcOrigY += m_iFrameWidthYxMBSize;
		ppxlcOrigBY += m_iFrameWidthYxMBSize;
		ppxlcRefY += m_iFrameWidthYxMBSize;
	}
	if (m_iMVFileUsage == 2)
		writePVOPMVs();
}

Void CVideoObjectEncoder::motionEstBVOP ()
{
	if (m_iMVFileUsage == 1)
		readBVOPMVs();
	m_iMAD = 0;
	CoordI y = 0; // frame-based, always start from 0
	CMBMode* pmbmd = m_rgmbmd;
	const CMBMode* pmbmdRef = NULL;
	const CMotionVector* pmvRef = NULL;		//mv in ref frame (for direct mode)

	if(m_bCodedFutureRef!=FALSE)
	{	
		pmbmdRef = m_rgmbmdRef;
		pmvRef = m_rgmvRef;
	}

	CMotionVector* pmv = m_rgmv;
	CMotionVector* pmvBackward = m_rgmvBackward;
	const PixelC* ppxlcOrigY = m_pvopcOrig->pixelsBoundY ();
	const PixelC* ppxlcRef0Y = m_pvopcRefQ0->pixelsY () + m_iStartInRefToCurrRctY;
	const PixelC* ppxlcRef1Y = m_pvopcRefQ1->pixelsY () + m_iStartInRefToCurrRctY;
	Int iMBX, iMBY;
	for (iMBY = 0; iMBY < m_iNumMBY; iMBY++, y += MB_SIZE) {
		const PixelC* ppxlcOrigMBY = ppxlcOrigY;
		const PixelC* ppxlcRef0MBY = ppxlcRef0Y;
		const PixelC* ppxlcRef1MBY = ppxlcRef1Y;
		CoordI x = 0; // frame-based, always start from 0
		for (iMBX = 0; iMBX < m_iNumMBX; iMBX++, x += MB_SIZE) {
			pmbmd->m_dctMd = INTER; // B-VOP is always INTER
			pmbmd->m_bhas4MVForward = pmbmd->m_bhas4MVBackward = FALSE;  //out encoder doesn't support 4 mv in bvop (not even direct mode)
// GMC
			pmbmd->m_bMCSEL = FALSE;
// ~GMC
			// TPS FIX
			if(m_volmd.volType == ENHN_LAYER && (m_vopmd.iRefSelectCode != 3 || m_volmd.iEnhnType == 1)){
				pmbmd->m_bColocatedMBSkip= FALSE;
				copyToCurrBuffY(ppxlcOrigMBY); 
				// begin: modified by Sharp(98/3/10)
				if ( m_vopmd.iRefSelectCode == 0 )
					motionEstMB_BVOP (x, y, pmv, pmvBackward, pmbmd, ppxlcRef0MBY, ppxlcRef1MBY);
				else
					motionEstMB_BVOP (x, y, pmv, pmvBackward, pmbmd, pmbmdRef, pmvRef, ppxlcRef0MBY, ppxlcRef1MBY, TRUE);
				// end: modified by Sharp(98/3/10)
			}
			else {
// GMC
				pmbmd->m_bColocatedMBMCSEL = (pmbmdRef!=NULL) ? pmbmdRef->m_bMCSEL : FALSE;
				if(pmbmd->m_bColocatedMBMCSEL)
					pmbmd->m_bColocatedMBSkip = FALSE;
				else
// ~GMC
					pmbmd->m_bColocatedMBSkip = (pmbmdRef!=NULL) ? pmbmdRef->m_bSkip : FALSE;
				if (pmbmd->m_bColocatedMBSkip) {
					pmbmd->m_bSkip = TRUE;
					memset (pmv, 0, BVOP_MV_PER_REF_PER_MB * sizeof (CMotionVector));
					pmbmd->m_mbType = FORWARD; // can be set to FORWARD mode since the result is the same
				}
				else {
// GMC
					pmbmd->m_bSkip = FALSE;
// ~GMC
					copyToCurrBuffY (ppxlcOrigMBY);
					m_iMAD += m_vopmd.bInterlace ?
                        motionEstMB_BVOP_Interlaced (x, y, pmv, pmvBackward, pmbmd,
                                                     pmbmdRef, pmvRef, ppxlcRef0MBY, ppxlcRef1MBY, pmbmdRef!=NULL) :
                        motionEstMB_BVOP            (x, y, pmv, pmvBackward, pmbmd,
                                                     pmbmdRef, pmvRef, ppxlcRef0MBY, ppxlcRef1MBY, pmbmdRef!=NULL);
				}
			}
			pmbmd++;
			pmv         += BVOP_MV_PER_REF_PER_MB;
			pmvBackward += BVOP_MV_PER_REF_PER_MB;

			if(m_bCodedFutureRef!=FALSE)
			{	
				pmbmdRef++;
				pmvRef += PVOP_MV_PER_REF_PER_MB;
			}

			ppxlcOrigMBY += MB_SIZE;
			ppxlcRef0MBY += MB_SIZE;
			ppxlcRef1MBY += MB_SIZE;
		}
		ppxlcOrigY += m_iFrameWidthYxMBSize;
		ppxlcRef0Y += m_iFrameWidthYxMBSize;
		ppxlcRef1Y += m_iFrameWidthYxMBSize;
	}
	if (m_iMVFileUsage == 2)
		writeBVOPMVs();
}

Void CVideoObjectEncoder::motionEstBVOP_WithShape ()
{
	m_iMAD = 0;
	CoordI y = m_rctCurrVOPY.top; 
	CMBMode* pmbmd = m_rgmbmd;
	CMotionVector* pmv = m_rgmv;
	CMotionVector* pmvBackward = m_rgmvBackward;

	Bool bColocatedMBExist;
	const PixelC* ppxlcOrigY = m_pvopcOrig->pixelsBoundY ();
	const PixelC* ppxlcOrigBY = m_pvopcOrig->pixelsBoundBY ();
//OBSS_SAIT_991015 //_SONY_SS_ 
	//Here, shuld be better to use reconstructed image insted of original image.
	// This is same as rectanguler case. See motionEstBVOP .
	// Maybe, Motion Estimation using Original Picture cannot be used for all BVOP (not only scalable) in microsoft. 
	const PixelC* ppxlcRef0Y = m_pvopcRefQ0->pixelsY () + m_iStartInRefToCurrRctY; 
	const PixelC* ppxlcRef1Y = m_pvopcRefQ1->pixelsY () + m_iStartInRefToCurrRctY; 
//~OBSS_SAIT_991015

	Int iMBX, iMBY;

	if (m_iMVFileUsage == 1)
		readBVOPMVs();

	for (iMBY = 0; iMBY < m_iNumMBY; iMBY++, y += MB_SIZE) {
		const PixelC* ppxlcOrigMBY = ppxlcOrigY;
		const PixelC* ppxlcOrigMBBY = ppxlcOrigBY;
		const PixelC* ppxlcRef0MBY = ppxlcRef0Y;
		const PixelC* ppxlcRef1MBY = ppxlcRef1Y;
		CoordI x = m_rctCurrVOPY.left;
		for (iMBX = 0; iMBX < m_iNumMBX; iMBX++, x += MB_SIZE) {
			pmbmd->m_dctMd = INTER; // B-VOP is always INTER
			pmbmd->m_bhas4MVForward = pmbmd->m_bhas4MVBackward = FALSE;
// GMC
			pmbmd->m_bMCSEL = FALSE;
// ~GMC
			copyToCurrBuffWithShapeY (ppxlcOrigMBY, ppxlcOrigMBBY);
			decideTransparencyStatus (pmbmd, m_ppxlcCurrMBBY);
			if (pmbmd->m_rgTranspStatus [0] != ALL) {
				const CMBMode* pmbmdRef;
				const CMotionVector* pmvRef;
				findColocatedMB (iMBX, iMBY, pmbmdRef, pmvRef);
// GMC
				pmbmd->m_bColocatedMBMCSEL = (pmbmdRef!=NULL && pmbmdRef->m_bMCSEL);
				if(pmbmd->m_bColocatedMBMCSEL)
					pmbmd->m_bColocatedMBSkip = FALSE;
				else
// ~GMC
					pmbmd->m_bColocatedMBSkip = (pmbmdRef!=NULL && pmbmdRef->m_bSkip);
				// TPS FIX
				if(m_volmd.volType == ENHN_LAYER && (m_vopmd.iRefSelectCode != 3 || m_volmd.iEnhnType == 1)) {
					pmbmd->m_bColocatedMBSkip= FALSE;
					bColocatedMBExist = (pmbmdRef!=NULL);
//OBSS_SAIT_991015
					if (m_vopmd.iRefSelectCode == 0) {
						if (pmbmd->m_rgTranspStatus [0] == NONE) 
							m_iMAD += motionEstMB_BVOP (
								x, y, pmv, pmvBackward,	pmbmd, ppxlcRef0MBY, ppxlcRef1MBY);
						else if (pmbmd->m_rgTranspStatus [0] == PARTIAL) {
							m_iMAD += motionEstMB_BVOP_WithShape (
								x, y, pmv, pmvBackward,	pmbmd, ppxlcRef0MBY, ppxlcRef1MBY);
						}
					}
					else {
						if (pmbmd->m_rgTranspStatus [0] == NONE) 
							m_iMAD += motionEstMB_BVOP (
								x, y, pmv, pmvBackward, pmbmd, pmbmdRef, pmvRef, ppxlcRef0MBY, ppxlcRef1MBY, bColocatedMBExist );
						else if (pmbmd->m_rgTranspStatus [0] == PARTIAL) {
							m_iMAD += motionEstMB_BVOP_WithShape (
								x, y, pmv, pmvBackward,	pmbmd, pmbmdRef, pmvRef, ppxlcRef0MBY, ppxlcRef1MBY, bColocatedMBExist );
						}
					}
//~OBSS_SAIT_991015
				} 
				else {
					if (pmbmd->m_bColocatedMBSkip) {
						pmbmd->m_bSkip = TRUE;
						memset (pmv, 0, BVOP_MV_PER_REF_PER_MB * sizeof (CMotionVector));
						m_statsMB.nDirectMB++;
						pmbmd->m_mbType = FORWARD;
					}
					else {
						bColocatedMBExist = (pmbmdRef!=NULL);
						if (pmbmd->m_rgTranspStatus [0] == NONE) {
// new changes
						m_iMAD += m_vopmd.bInterlace ?
							motionEstMB_BVOP_Interlaced (x, y, pmv, pmvBackward, pmbmd,
                                                     pmbmdRef, pmvRef, ppxlcRef0MBY, 
													 ppxlcRef1MBY
													 ,bColocatedMBExist) :
// end of new changes
						motionEstMB_BVOP (
								x, y, 
								pmv, pmvBackward,
								pmbmd, 
								pmbmdRef, pmvRef,
								ppxlcRef0MBY, ppxlcRef1MBY,
								bColocatedMBExist
							);
						}
						else if (pmbmd->m_rgTranspStatus [0] == PARTIAL) {
// new changes
					m_iMAD += m_vopmd.bInterlace ?
                        motionEstMB_BVOP_InterlacedWithShape (x, y, pmv, pmvBackward, pmbmd,
                                                     pmbmdRef, pmvRef, ppxlcRef0MBY, 
													 ppxlcRef1MBY
													 ,bColocatedMBExist) :
// end of new changes
					motionEstMB_BVOP_WithShape (
								x, y, 
								pmv, pmvBackward,
								pmbmd, 
								pmbmdRef, pmvRef,
								ppxlcRef0MBY, ppxlcRef1MBY,
								bColocatedMBExist
							);
						}
					}
				}
			}
			ppxlcOrigMBBY += MB_SIZE;
			pmbmd++;
			pmv         += BVOP_MV_PER_REF_PER_MB;
			pmvBackward += BVOP_MV_PER_REF_PER_MB;
			ppxlcOrigMBY += MB_SIZE;
			ppxlcRef0MBY += MB_SIZE;
			ppxlcRef1MBY += MB_SIZE;
		}
		ppxlcOrigY += m_iFrameWidthYxMBSize;
		ppxlcOrigBY += m_iFrameWidthYxMBSize;
		ppxlcRef0Y += m_iFrameWidthYxMBSize;
		ppxlcRef1Y += m_iFrameWidthYxMBSize;
	}
	if (m_iMVFileUsage == 2)
		writeBVOPMVs();
}

Void CVideoObjectEncoder::copyToCurrBuffWithShapeY (const PixelC* ppxlcCurrY, const PixelC* ppxlcCurrBY)
{
	Int iUnit = sizeof(PixelC); // NBIT: for memcpy
	PixelC* ppxlcCurrMBY = m_ppxlcCurrMBY;
	PixelC* ppxlcCurrMBBY = m_ppxlcCurrMBBY;
	Int ic;
	for (ic = 0; ic < MB_SIZE; ic++) {
		memcpy (ppxlcCurrMBY, ppxlcCurrY, MB_SIZE*iUnit);
		memcpy (ppxlcCurrMBBY, ppxlcCurrBY, MB_SIZE*iUnit);
		ppxlcCurrMBY += MB_SIZE; ppxlcCurrY += m_iFrameWidthY;
		ppxlcCurrMBBY += MB_SIZE; ppxlcCurrBY += m_iFrameWidthY;
	}
}

Void CVideoObjectEncoder::copyToCurrBuffY (const PixelC* ppxlcCurrY)
{
	Int iUnit = sizeof(PixelC); // NBIT: for memcpy
	PixelC* ppxlcCurrMBY = m_ppxlcCurrMBY;
	Int ic;
// RRV modification
	for(ic = 0; ic < (MB_SIZE *m_iRRVScale); ic++)
	{
		memcpy(ppxlcCurrMBY, ppxlcCurrY, (MB_SIZE *m_iRRVScale *iUnit));
		ppxlcCurrMBY += (MB_SIZE *m_iRRVScale); ppxlcCurrY += m_iFrameWidthY;
	}
//	for (ic = 0; ic < MB_SIZE; ic++) {
//		memcpy (ppxlcCurrMBY, ppxlcCurrY, MB_SIZE*iUnit);
//		ppxlcCurrMBY += MB_SIZE; ppxlcCurrY += m_iFrameWidthY;
//	}
// ~RRV
}

//for spatial sclability: basically set every mv to zero
Void CVideoObjectEncoder::motionEstMB_PVOP (CoordI x, CoordI y, 
											CMotionVector* pmv, CMBMode* pmbmd)
{
	pmbmd -> m_bhas4MVForward = FALSE;
	pmbmd-> m_dctMd = INTER;
	memset (pmv, 0, sizeof (CMotionVector) * PVOP_MV_PER_REF_PER_MB);
	for (UInt i = 0; i < PVOP_MV_PER_REF_PER_MB; i++)  {
		pmv -> computeTrueMV (); 
		pmv++;
	}
}

Int CVideoObjectEncoder::motionEstMB_PVOP (
	CoordI x, CoordI y, 
	CMotionVector* pmv, CMBMode* pmbmd, 
	const PixelC* ppxlcRefMBY
)
{
// RRV modification/insertion
	Int iFavorZero, iFavorInter;

	Int nBits = m_volmd.nBits;	// NBIT
	if(m_vopmd.RRVmode.iRRVOnOff == 1)
	{
		iFavorZero	= FAVORZERO_RRV;	// NBIT
		iFavorInter	= FAVOR_INTER_RRV;	// NBIT
	}
	else
	{
		iFavorZero	= FAVORZERO;	// NBIT
		iFavorInter	= FAVOR_INTER;	// NBIT
	}
	Int iFavor16x16 = FAVOR_16x16; // NBIT
	Int iFavor32x32 = FAVOR_32x32; // NBIT
//	Int nBits = m_volmd.nBits; // NBIT
//	Int iFavorZero = FAVORZERO; // NBIT
//	Int iFavorInter = FAVOR_INTER; // NBIT
//	Int iFavor16x16 = FAVOR_16x16; // NBIT
// ~RRV
	// NBIT: addjust mode selection thresholds
	if (nBits > 8) {
		iFavorZero <<= (nBits-8);
		iFavorInter <<= (nBits-8);
		iFavor16x16 <<= (nBits-8);
	} else if (nBits < 8) {
		iFavorZero >>= (8-nBits);
		iFavorInter >>= (8-nBits);
		iFavor16x16 >>= (8-nBits);
	}

	Int iInitSAD = sad16x16At0 (ppxlcRefMBY);
	Int iSADInter = blkmatch16 (pmv, x, y, x, y, iInitSAD, ppxlcRefMBY, m_puciRefQZoom0, m_vopmd.iSearchRangeForward, m_volmd.bQuarterSample);
	Int iSumDev = sumDev ();
#ifdef __TRACE_AND_STATS_
	m_pbitstrmOut->trace (iSADInter, "MB_SAD16");
#endif // __TRACE_AND_STATS_

// INTERLACE
/* NBIT: change to a bigger number
	Int iSAD16x8 = 256*256;
*/
	Int iSAD16x8 = 4096*256;
	pmbmd -> m_bFieldMV = FALSE;
	if(m_vopmd.bInterlace) {
		// Field-Based Estimation
		CMotionVector* pmv16x8 = pmv + 5;
		const PixelC *ppxlcHalfPelRef = m_pvopcRefQ0->pixelsY()
			+EXPANDY_REF_FRAME*m_iFrameWidthY + EXPANDY_REF_FRAME+ x + y * m_iFrameWidthY; // 1.31.99 changes

		// top to top
		Int iSAD16x8top = blkmatch16x8 (pmv16x8, x, y, 0, ppxlcRefMBY,
			ppxlcHalfPelRef, m_vopmd.iSearchRangeForward, m_volmd.bQuarterSample);
		pmv16x8++;
		// bot to top
		Int iSAD16x8bot = blkmatch16x8 (pmv16x8, x, y, 0, ppxlcRefMBY + m_iFrameWidthY,
			ppxlcHalfPelRef + m_iFrameWidthY, m_vopmd.iSearchRangeForward, m_volmd.bQuarterSample);

		iSAD16x8=(iSAD16x8top<iSAD16x8bot) ? iSAD16x8top : iSAD16x8bot;
		pmbmd->m_bForwardTop = (iSAD16x8top<iSAD16x8bot) ? 0 : 1;
		pmv16x8++;
		// top to bot
		iSAD16x8top = blkmatch16x8 (pmv16x8, x, y, MB_SIZE, ppxlcRefMBY,
			ppxlcHalfPelRef, m_vopmd.iSearchRangeForward, m_volmd.bQuarterSample);
		pmv16x8++;
		// bot to bot
		iSAD16x8bot = blkmatch16x8 (pmv16x8, x, y, MB_SIZE, ppxlcRefMBY + m_iFrameWidthY,
			ppxlcHalfPelRef + m_iFrameWidthY, m_vopmd.iSearchRangeForward, m_volmd.bQuarterSample);
		iSAD16x8 += (iSAD16x8top<iSAD16x8bot) ? iSAD16x8top : iSAD16x8bot;
		pmbmd->m_bForwardBottom = (iSAD16x8top<iSAD16x8bot) ? 0 : 1;
	} else {
/* NBIT: change to a bigger number
		iSAD16x8 = 256*256;
*/
		iSAD16x8 = 4096*256;

		for (Int iBlk = 5; iBlk <= 8; iBlk++) {  // 04/28/99 david ruhoff
			pmv [iBlk] = pmv [0];   // fill in field info to make mv file deterministic
		}
	}
#ifdef __TRACE_AND_STATS_
	m_pbitstrmOut->trace (iSAD16x8, "MB_SAD16x8");
#endif // __TRACE_AND_STATS_
// ~INTERLACE

	CoordI blkX, blkY;
	Int iSAD8 = 0;
	CMotionVector* pmv8 = pmv + 1;
// RRV modification
	blkX = x + (BLOCK_SIZE *m_iRRVScale);
	blkY = y + (BLOCK_SIZE *m_iRRVScale);
//	blkX = x + BLOCK_SIZE;
//	blkY = y + BLOCK_SIZE;
// ~RRV
	const PixelC* ppxlcCodedBlkY = m_ppxlcCurrMBY;
	
#ifdef __DISABLE_4MV_FOR_PVOP_
	iSAD8 = 4096*4096; // big number to disable 8x8 mode
#else

	// 8x8
	iSAD8 += blockmatch8 (ppxlcCodedBlkY, pmv8, x, y, pmv, m_vopmd.iSearchRangeForward, m_volmd.bQuarterSample);
	pmv8++;
// RRV modification
	ppxlcCodedBlkY += (BLOCK_SIZE *m_iRRVScale);
//	ppxlcCodedBlkY += BLOCK_SIZE;
// ~RRV
#ifdef __TRACE_AND_STATS_
	m_pbitstrmOut->trace (iSAD8, "MB_SAD8");
#endif // __TRACE_AND_STATS_

	iSAD8 += blockmatch8 (ppxlcCodedBlkY, pmv8, blkX, y, pmv, m_vopmd.iSearchRangeForward, m_volmd.bQuarterSample);
	pmv8++;
// RRV modification
	ppxlcCodedBlkY += (BLOCK_SIZE *m_iRRVScale) *(MB_SIZE *m_iRRVScale) 
		-(BLOCK_SIZE *m_iRRVScale);
//	ppxlcCodedBlkY += BLOCK_SIZE * MB_SIZE - BLOCK_SIZE;
// ~RRV
#ifdef __TRACE_AND_STATS_
	m_pbitstrmOut->trace (iSAD8, "MB_SAD8");
#endif // __TRACE_AND_STATS_

	iSAD8 += blockmatch8 (ppxlcCodedBlkY, pmv8, x, blkY, pmv, m_vopmd.iSearchRangeForward, m_volmd.bQuarterSample);
	pmv8++;
// RRV modification
	ppxlcCodedBlkY += (BLOCK_SIZE *m_iRRVScale);
//	ppxlcCodedBlkY += BLOCK_SIZE;
// ~RRV
#ifdef __TRACE_AND_STATS_
	m_pbitstrmOut->trace (iSAD8, "MB_SAD8");
#endif // __TRACE_AND_STATS_

	iSAD8 += blockmatch8 (ppxlcCodedBlkY, pmv8, blkX, blkY, pmv, m_vopmd.iSearchRangeForward, m_volmd.bQuarterSample);
#ifdef __TRACE_AND_STATS_
	m_pbitstrmOut->trace (iSAD8, "MB_SAD8");
#endif // __TRACE_AND_STATS_
	
#endif // DISABLE_4MV_FOR_PVOP

/* NBIT: change
	iSADInter -= FAVOR_16x16;
*/
// RRV modification
	if(m_vopmd.RRVmode.iRRVOnOff == 1)
	{
		iSADInter	-= iFavor32x32;
		iSAD16x8	-= FAVOR_FIELD_RRV;
	}
	else
	{
		iSADInter	-= iFavor16x16;
		iSAD16x8	-= FAVOR_FIELD;
	}
//	iSADInter -= iFavor16x16;
//	iSAD16x8 -= FAVOR_FIELD;
// ~RRV
	if ((iSADInter <= iSAD8) && (iSADInter <= iSAD16x8)) {
/* NBIT: change
	    iSADInter += FAVOR_16x16;
*/
// RRV modification
		if(m_vopmd.RRVmode.iRRVOnOff == 1)
		{
			iSADInter += iFavor32x32;
		}
		else
		{
			iSADInter += iFavor16x16;
		}
//	    iSADInter += iFavor16x16;
// ~RRV
		pmbmd -> m_bhas4MVForward = FALSE;
		pmv -> computeTrueMV (); // compute here instead of blkmatch to save computation
		pmv -> computeMV ();     // to make pmv unique (iMVX=3+iHalfX=-1 == iMVX=2+iHalfX=+1)
		for (UInt i = 1; i < PVOP_MV_PER_REF_PER_MB; i++) {// didn't increment the last pmv
          pmv[i].m_vctTrueHalfPel = pmv->m_vctTrueHalfPel;	// Save (iMVX, iMVY) for MV file
          pmv[i].computeMV();
        }
        
        
	}
// INTERLACE
	else if (iSAD16x8 <= iSAD8) { // Field-based
		pmbmd -> m_bhas4MVForward = FALSE;
		pmbmd -> m_bFieldMV = TRUE;
		Int iTempX1, iTempY1, iTempX2, iTempY2;
		if(pmbmd->m_bForwardTop) {
			pmv [6].computeTrueMV ();
            pmv [6].computeMV ();     // to make pmv unique (iMVX=3+iHalfX=-1 == iMVX=2+iHalfX=+1)
			iTempX1 = pmv[6].m_vctTrueHalfPel.x;
			iTempY1 = pmv[6].m_vctTrueHalfPel.y;
		}
		else {
			pmv [5].computeTrueMV ();
            pmv [5].computeMV ();     // to make pmv unique (iMVX=3+iHalfX=-1 == iMVX=2+iHalfX=+1)
			iTempX1 = pmv[5].m_vctTrueHalfPel.x;
			iTempY1 = pmv[5].m_vctTrueHalfPel.y;
		}
		if(pmbmd->m_bForwardBottom) {
			pmv [8].computeTrueMV ();
            pmv [8].computeMV ();     // to make pmv unique (iMVX=3+iHalfX=-1 == iMVX=2+iHalfX=+1)
			iTempX2 = pmv[8].m_vctTrueHalfPel.x;
			iTempY2 = pmv[8].m_vctTrueHalfPel.y;
		}
		else {
			pmv [7].computeTrueMV ();
            pmv [7].computeMV ();     // to make pmv unique (iMVX=3+iHalfX=-1 == iMVX=2+iHalfX=+1)
			iTempX2 = pmv[7].m_vctTrueHalfPel.x;
			iTempY2 = pmv[7].m_vctTrueHalfPel.y;
		}
		iSADInter = iSAD16x8 + FAVOR_FIELD;
		Int iTemp;
		for (UInt i = 1; i < 5; i++) {
			iTemp = iTempX1 + iTempX2;
			pmv [i].m_vctTrueHalfPel.x = (iTemp & 3) ? ((iTemp>>1) | 1) : (iTemp>>1);
			iTemp = iTempY1 + iTempY2;
			pmv [i].m_vctTrueHalfPel.y = (iTemp & 3) ? ((iTemp>>1) | 1) : (iTemp>>1);
			pmv[i].computeMV (); 
		}
	}
// ~INTERLACE
	else {
		pmv [1].computeTrueMV ();
		pmv [2].computeTrueMV ();
		pmv [3].computeTrueMV ();
		pmv [4].computeTrueMV ();
		pmv [1].computeMV ();
		pmv [2].computeMV ();
		pmv [3].computeMV ();
		pmv [4].computeMV ();
		pmbmd -> m_bhas4MVForward = TRUE;
		iSADInter = iSAD8;
	}
// GMC
	pmbmd -> m_bMCSEL = FALSE;
	if(m_uiSprite == 2 && m_vopmd.vopPredType == SPRITE){
		Int iMSADG = 4096*256;
		PixelC* ppxlcRefforgme = (PixelC*) m_pvopcRefQ0->pixelsY () ;
		iMSADG = globalme (x,y,ppxlcRefforgme);
#ifdef __TRACE_AND_STATS_
		m_pbitstrmOut->trace (iMSADG, "MB_GMC");
#endif // __TRACE_AND_STATS_
		Int ioffset, iQP=m_uiGMCQP; // GMC_V2 *m_rgiQPpred;
		if(pmbmd -> m_bhas4MVForward == TRUE)
			//ioffset = iQP*256/24;
			ioffset = iQP*256/16;
		else
		{
			if(pmbmd -> m_bFieldMV == TRUE)
			{
				ioffset = iQP*256/32;
			}else{
				if(pmv->m_vctTrueHalfPel.x == 0 && pmv->m_vctTrueHalfPel.y == 0)
					ioffset = iFavorZero*2;
				else
					ioffset = iQP*256/64;
			}
		}
		if((iMSADG - ioffset) <= iSADInter)
		{
			iSADInter = iMSADG;
			pmbmd -> m_dctMd = INTER;
			pmbmd -> m_bFieldMV = FALSE;
			pmbmd -> m_bhas4MVForward = FALSE;
			pmbmd -> m_bMCSEL = TRUE;
			Int iPmvx, iPmvy, iHalfx, iHalfy;
			globalmv (iPmvx, iPmvy, iHalfx, iHalfy,
				x,y,m_vopmd.mvInfoForward.uiRange,m_volmd.bQuarterSample);
			*pmv = CMotionVector (iPmvx, iPmvy);
			pmv -> iHalfX = iHalfx;
			pmv -> iHalfY = iHalfy;
			pmv -> computeTrueMV ();
			pmv -> computeMV ();
			for (UInt i = 1; i < PVOP_MV_PER_REF_PER_MB; i++)
				pmv[i] = *pmv;
		}
		else
		{
			pmbmd -> m_bMCSEL = FALSE;
		}
	}
// ~GMC
/* NBIT: change
	if (iSumDev < (iSADInter - FAVOR_INTER)) {
*/
// GMC
	Int iIntraOffset;
	if(m_uiSprite == 2 && m_vopmd.vopPredType == SPRITE)
		iIntraOffset = iFavorInter;
		//iIntraOffset = iFavorInter/2;
	else
		iIntraOffset = iFavorInter;
// ~GMC
	if (iSumDev < (iSADInter - iIntraOffset)) { // GMC
		pmbmd -> m_bSkip = FALSE;
		pmbmd -> m_dctMd = INTRA;
		pmbmd -> m_bFieldMV = FALSE;
// GMC
		pmbmd -> m_bMCSEL = FALSE;
// ~GMC
		memset (pmv, 0, PVOP_MV_PER_REF_PER_MB * sizeof (CMotionVector));
		return ((m_uiRateControl==RC_MPEG4) ? sumAbsCurrMB () : 0);
	}
	else {
		pmbmd -> m_dctMd = INTER;
		if (pmbmd->m_bhas4MVForward == FALSE
			&& pmbmd -> m_bFieldMV == FALSE
// GMC
			&& pmbmd -> m_bMCSEL == FALSE
// ~GMC
			&& pmv->m_vctTrueHalfPel.x == 0 && pmv->m_vctTrueHalfPel.y == 0)
/* NBIT: change
			return (iSADInter + FAVORZERO);
*/
			return (iSADInter + iFavorZero);
		else 
			return iSADInter;
	}
}

Int CVideoObjectEncoder::motionEstMB_PVOP_WithShape (
	CoordI x, CoordI y, 
	CMotionVector* pmv, CMBMode* pmbmd, 
	const PixelC* ppxlcRefMBY
)
{
	assert (pmbmd->m_rgTranspStatus [0] == PARTIAL);
	UInt nBits = m_volmd.nBits; // NBIT
	Int iFavorZero = FAVORZERO;
	Int iFavor16x16 = (pmbmd->m_rgNumNonTranspPixels [0] >> 1) + 1; // NBIT
	Int iFavorInter = pmbmd->m_rgNumNonTranspPixels [0] << 1; // NBIT
	// NBIT: addjust mode selection thresholds
	if (nBits > 8) {
		iFavor16x16 <<= (nBits-8);
		iFavorInter <<= (nBits-8);
		iFavorZero <<= (nBits-8);
	} else if (nBits < 8) {
		iFavor16x16 >>= (8-nBits);
		iFavorInter >>= (8-nBits);
		iFavorZero >>= (8-nBits);
	}

	Int iInitSAD = sad16x16At0WithShape (ppxlcRefMBY, pmbmd);
	Int iSADInter = blkmatch16WithShape (pmv, x, y, x, y, iInitSAD, ppxlcRefMBY, m_puciRefQZoom0, pmbmd, m_vopmd.iSearchRangeForward, m_volmd.bQuarterSample,0);
	Int iSumDev = sumDevWithShape (pmbmd -> m_rgNumNonTranspPixels [0]);
#ifdef __TRACE_AND_STATS_
	m_pbitstrmOut->trace (iSADInter, "MB_SAD16");
#endif // __TRACE_AND_STATS_
// INTERLACE
// New Changes
	Int iSAD16x8 = 4096*256;
	pmbmd -> m_bFieldMV = FALSE;
	if(m_vopmd.bInterlace) {
		// Field-Based Estimation
		CMotionVector* pmv16x8 = pmv + 5;
		const PixelC *ppxlcHalfPelRef = m_pvopcRefQ0->pixelsY()
			+EXPANDY_REF_FRAME*m_iFrameWidthY + EXPANDY_REF_FRAME + x + y * m_iFrameWidthY; // 1.31.99 changes

		// top to top
		Int iSAD16x8top = blkmatch16x8WithShape (pmv16x8, x, y, 0, ppxlcRefMBY,
			ppxlcHalfPelRef, m_vopmd.iSearchRangeForward, m_volmd.bQuarterSample,0);
		pmv16x8++;
		// bot to top
		Int iSAD16x8bot = blkmatch16x8WithShape (pmv16x8, x, y, 0, ppxlcRefMBY + m_iFrameWidthY,
			ppxlcHalfPelRef + m_iFrameWidthY, m_vopmd.iSearchRangeForward, m_volmd.bQuarterSample,0);

		iSAD16x8=(iSAD16x8top<iSAD16x8bot) ? iSAD16x8top : iSAD16x8bot;
		pmbmd->m_bForwardTop = (iSAD16x8top<iSAD16x8bot) ? 0 : 1;
		pmv16x8++;
		// top to bot
		iSAD16x8top = blkmatch16x8WithShape (pmv16x8, x, y, MB_SIZE, ppxlcRefMBY,
			ppxlcHalfPelRef, m_vopmd.iSearchRangeForward, m_volmd.bQuarterSample,0);
		pmv16x8++;
		// bot to bot
		iSAD16x8bot = blkmatch16x8WithShape (pmv16x8, x, y, MB_SIZE, ppxlcRefMBY + m_iFrameWidthY,
			ppxlcHalfPelRef + m_iFrameWidthY, m_vopmd.iSearchRangeForward, m_volmd.bQuarterSample,0);
		iSAD16x8 += (iSAD16x8top<iSAD16x8bot) ? iSAD16x8top : iSAD16x8bot;
		pmbmd->m_bForwardBottom = (iSAD16x8top<iSAD16x8bot) ? 0 : 1;
	} else {
		iSAD16x8 = 4096*256;

		for (Int iBlk = 5; iBlk <= 8; iBlk++) {  // 04/28/99 david ruhoff
			pmv [iBlk] = pmv [0];   // fill in field info to make mv file deterministic
		}
	}
#ifdef __TRACE_AND_STATS_
	m_pbitstrmOut->trace (iSAD16x8, "MB_SAD16x8");
#endif // __TRACE_AND_STATS_
// end of New changes
// ~INTERLACE

	CoordI blkX, blkY;
	Int iSAD8 = 0;
	CMotionVector* pmv8 = pmv + 1;
	blkX = x + BLOCK_SIZE;
	blkY = y + BLOCK_SIZE;
	const PixelC* ppxlcCodedBlkY = m_ppxlcCurrMBY;
	const PixelC* ppxlcCodedBlkBY = m_ppxlcCurrMBBY;

	// 8 x 8
	iSAD8 += 
		(pmbmd->m_rgTranspStatus [1] == PARTIAL) ? blockmatch8WithShape (ppxlcCodedBlkY, ppxlcCodedBlkBY, pmv8, x, y, pmv, m_vopmd.iSearchRangeForward, m_volmd.bQuarterSample,0) :
		(pmbmd->m_rgTranspStatus [1] == NONE) ? blockmatch8 (ppxlcCodedBlkY, pmv8, x, y, pmv, m_vopmd.iSearchRangeForward, m_volmd.bQuarterSample) : 0;
	pmv8++;
	ppxlcCodedBlkY += BLOCK_SIZE;
	ppxlcCodedBlkBY += BLOCK_SIZE;
#ifdef __TRACE_AND_STATS_
	m_pbitstrmOut->trace (iSAD8, "MB_SAD8");
#endif // __TRACE_AND_STATS_

	iSAD8 += 
		(pmbmd->m_rgTranspStatus [2] == PARTIAL) ? blockmatch8WithShape (ppxlcCodedBlkY, ppxlcCodedBlkBY, pmv8, blkX, y, pmv, m_vopmd.iSearchRangeForward, m_volmd.bQuarterSample,0) :
		(pmbmd->m_rgTranspStatus [2] == NONE)	? blockmatch8 (ppxlcCodedBlkY, pmv8, blkX, y, pmv, m_vopmd.iSearchRangeForward, m_volmd.bQuarterSample) : 0;
	pmv8++;
	ppxlcCodedBlkY += BLOCK_SIZE * MB_SIZE - BLOCK_SIZE;
	ppxlcCodedBlkBY += BLOCK_SIZE * MB_SIZE - BLOCK_SIZE;
#ifdef __TRACE_AND_STATS_
	m_pbitstrmOut->trace (iSAD8, "MB_SAD8");
#endif // __TRACE_AND_STATS_
	
	iSAD8 += 
		(pmbmd->m_rgTranspStatus [3] == PARTIAL) ? blockmatch8WithShape (ppxlcCodedBlkY, ppxlcCodedBlkBY, pmv8, x, blkY, pmv, m_vopmd.iSearchRangeForward, m_volmd.bQuarterSample,0) :
		(pmbmd->m_rgTranspStatus [3] == NONE)	? blockmatch8 (ppxlcCodedBlkY, pmv8, x, blkY, pmv, m_vopmd.iSearchRangeForward, m_volmd.bQuarterSample) : 0;
	pmv8++;
	ppxlcCodedBlkY += BLOCK_SIZE;
	ppxlcCodedBlkBY += BLOCK_SIZE;
#ifdef __TRACE_AND_STATS_
	m_pbitstrmOut->trace (iSAD8, "MB_SAD8");
#endif // __TRACE_AND_STATS_
	
	iSAD8 += 
		(pmbmd->m_rgTranspStatus [4] == PARTIAL) ? blockmatch8WithShape (ppxlcCodedBlkY, ppxlcCodedBlkBY, pmv8, blkX, blkY, pmv, m_vopmd.iSearchRangeForward, m_volmd.bQuarterSample,0) :
		(pmbmd->m_rgTranspStatus [4] == NONE)	? blockmatch8 (ppxlcCodedBlkY, pmv8, blkX, blkY, pmv, m_vopmd.iSearchRangeForward, m_volmd.bQuarterSample) : 0;
#ifdef __TRACE_AND_STATS_
	m_pbitstrmOut->trace (iSAD8, "MB_SAD8");
#endif // __TRACE_AND_STATS_

	// begin added by Sharp (98/9/10) prevents inter4v with one transp block
	if (( pmbmd->m_rgTranspStatus [1] != ALL && pmbmd->m_rgTranspStatus [2] == ALL
		&& pmbmd->m_rgTranspStatus [3] == ALL && pmbmd->m_rgTranspStatus [4] == ALL	)||
		( pmbmd->m_rgTranspStatus [1] == ALL && pmbmd->m_rgTranspStatus [2] != ALL
		&& pmbmd->m_rgTranspStatus [3] == ALL && pmbmd->m_rgTranspStatus [4] == ALL	)||
		( pmbmd->m_rgTranspStatus [1] == ALL && pmbmd->m_rgTranspStatus [2] == ALL
		&& pmbmd->m_rgTranspStatus [3] != ALL && pmbmd->m_rgTranspStatus [4] == ALL	)||
		( pmbmd->m_rgTranspStatus [1] == ALL && pmbmd->m_rgTranspStatus [2] == ALL
		&& pmbmd->m_rgTranspStatus [3] == ALL && pmbmd->m_rgTranspStatus [4] != ALL ))
		iSAD8 = 256 * 4096;
	// end added by Sharp (98/9/10)

#ifdef __TRACE_AND_STATS_
	m_pbitstrmOut->trace (iSAD8, "MB_SAD8");
#endif // __TRACE_AND_STATS_

/* NBIT: change
	if ((iSADInter - ((pmbmd->m_rgNumNonTranspPixels [0] >> 1) + 1)) <= iSAD8) {
*/
// new changes
	iSADInter -= iFavor16x16;
	iSAD16x8 -= FAVOR_FIELD;
	if ((iSADInter <= iSAD8)&&(iSADInter <= iSAD16x8)) {
		iSADInter += iFavor16x16;
// end of new changes
		pmbmd -> m_bhas4MVForward = FALSE;
		pmv -> computeTrueMV (); // compute here instead of blkmatch to save computation
		pmv -> computeMV ();     // to make pmv unique (iMVX=3+iHalfX=-1 == iMVX=2+iHalfX=+1)
		for (UInt i = 1; i < PVOP_MV_PER_REF_PER_MB; i++) // didn't increment the last pmv
			pmv [i] = *pmv;
	}
// INTERLACE
	// new changes
	else if (iSAD16x8 <= iSAD8) { // Field-based
		pmbmd -> m_bhas4MVForward = FALSE;
		pmbmd -> m_bFieldMV = TRUE;
		Int iTempX1, iTempY1, iTempX2, iTempY2;
		if(pmbmd->m_bForwardTop) {
			pmv [6].computeTrueMV ();
			pmv [6].computeMV ();     // to make pmv unique (iMVX=3+iHalfX=-1 == iMVX=2+iHalfX=+1)
			iTempX1 = pmv[6].m_vctTrueHalfPel.x;
			iTempY1 = pmv[6].m_vctTrueHalfPel.y;
		}
		else {
			pmv [5].computeTrueMV ();
			pmv [5].computeMV ();     // to make pmv unique (iMVX=3+iHalfX=-1 == iMVX=2+iHalfX=+1)
			iTempX1 = pmv[5].m_vctTrueHalfPel.x;
			iTempY1 = pmv[5].m_vctTrueHalfPel.y;
		}
		if(pmbmd->m_bForwardBottom) {
			pmv [8].computeTrueMV ();
			pmv [8].computeMV ();     // to make pmv unique (iMVX=3+iHalfX=-1 == iMVX=2+iHalfX=+1)
			iTempX2 = pmv[8].m_vctTrueHalfPel.x;
			iTempY2 = pmv[8].m_vctTrueHalfPel.y;
		}
		else {
			pmv [7].computeTrueMV ();
			pmv [7].computeMV ();     // to make pmv unique (iMVX=3+iHalfX=-1 == iMVX=2+iHalfX=+1)
			iTempX2 = pmv[7].m_vctTrueHalfPel.x;
			iTempY2 = pmv[7].m_vctTrueHalfPel.y;
		}
		iSADInter = iSAD16x8 + FAVOR_FIELD;
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
	else {
		pmv [1].computeTrueMV ();
		pmv [2].computeTrueMV ();
		pmv [3].computeTrueMV ();
		pmv [4].computeTrueMV ();
		pmv [1].computeMV ();
		pmv [2].computeMV ();
		pmv [3].computeMV ();
		pmv [4].computeMV ();
		pmbmd -> m_bhas4MVForward = TRUE;
		iSADInter = iSAD8;
	}
// GMC
	pmbmd -> m_bMCSEL = FALSE;
	if(m_uiSprite == 2 && m_vopmd.vopPredType == SPRITE){
		Int iNb = pmbmd->m_rgNumNonTranspPixels [0] ;
		Int iMSADG = 4096*256;
		PixelC* ppxlcRefforgme = (PixelC*) m_pvopcRefQ0->pixelsY () ;
		iMSADG = globalme (x,y,ppxlcRefforgme);
#ifdef __TRACE_AND_STATS_
		m_pbitstrmOut->trace (iMSADG, "MB_GMC");
#endif // __TRACE_AND_STATS_
		Int ioffset, iQP=m_uiGMCQP; // GMC_V2 *m_rgiQPpred;
		if(pmbmd -> m_bhas4MVForward == TRUE)
			ioffset = iQP*iNb/16;
			//ioffset = iQP*iNb/24;
		else
		{
			if(pmbmd -> m_bFieldMV == TRUE)
			{
				ioffset = iQP*iNb/32;
			}else{
				if(pmv->m_vctTrueHalfPel.x == 0 && pmv->m_vctTrueHalfPel.y == 0)
					ioffset = iFavor16x16*2;
				else
					ioffset = iQP*iNb/64;
			}
		}
		if((iMSADG - ioffset) <= iSADInter)
		{
			iSADInter = iMSADG;
			pmbmd -> m_dctMd = INTER;
			pmbmd -> m_bFieldMV = FALSE;
			pmbmd -> m_bhas4MVForward = FALSE;
			pmbmd -> m_bMCSEL = TRUE;
			Int iPmvx, iPmvy, iHalfx, iHalfy;
			globalmv (iPmvx, iPmvy, iHalfx, iHalfy,
				x,y,m_vopmd.mvInfoForward.uiRange,m_volmd.bQuarterSample);
			*pmv = CMotionVector (iPmvx, iPmvy);
			pmv -> iHalfX = iHalfx;
			pmv -> iHalfY = iHalfy;
			pmv -> computeTrueMV ();
			pmv -> computeMV ();
			for (UInt i = 1; i < PVOP_MV_PER_REF_PER_MB; i++)
				pmv[i] = *pmv;
		}
		else
		{
			pmbmd -> m_bMCSEL = FALSE;
		}
	}
// ~GMC
/* NBIT: change
	if (iSumDev < (iSADInter - (pmbmd->m_rgNumNonTranspPixels [0] << 1))) {
*/
// GMC
	Int iIntraOffset;
	if(m_uiSprite == 2 && m_vopmd.vopPredType == SPRITE)
		iIntraOffset = iFavorInter;
		//iIntraOffset = iFavorInter/2;
	else
		iIntraOffset = iFavorInter;
// ~GMC
	if (iSumDev < (iSADInter - iIntraOffset)) { // GMC
		pmbmd -> m_bSkip = FALSE;
		pmbmd -> m_dctMd = INTRA;
// new changes
		pmbmd -> m_bFieldMV = FALSE;
// GMC
		pmbmd -> m_bMCSEL = FALSE;
// ~GMC
		memset (pmv, 0, PVOP_MV_PER_REF_PER_MB * sizeof (CMotionVector));
		return ((m_uiRateControl==RC_MPEG4) ? sumAbsCurrMB () : 0);
	}
	else {
		pmbmd -> m_dctMd = INTER;
// new changes
		if (pmbmd->m_bhas4MVForward == FALSE
			&& pmbmd -> m_bFieldMV == FALSE
// GMC
			&& pmbmd -> m_bMCSEL == FALSE
// ~GMC
			&& pmv->m_vctTrueHalfPel.x == 0 && pmv->m_vctTrueHalfPel.y == 0)
			return (iSADInter + iFavorZero);
		else 
			return iSADInter;
	}
}

CMotionVector rgmvDirectBack [5];
CMotionVector rgmvDirectForward [5];

Int CVideoObjectEncoder::motionEstMB_BVOP (
	CoordI x, CoordI y, 
	CMotionVector* pmvForward, CMotionVector* pmvBackward,
	CMBMode* pmbmd,
	const CMBMode* pmbmdRef, const CMotionVector* pmvRef,
	const PixelC* ppxlcRef0MBY, const PixelC* ppxlcRef1MBY,
	Bool bColocatedMBExist
)
{
  //for QuarterPel mwi 21JUL98
    const CMotionVector* pmvRefBlk = pmvRef;
    CMotionVector* pmvDirectForward = rgmvDirectForward;
    CMotionVector* pmvDirectBack = rgmvDirectBack;
    //~for QuarterPel mwi 21JUL98

 	Int iInitSAD = sad16x16At0 (ppxlcRef0MBY) + FAVORZERO;
	Int iSADForward = blkmatch16 (pmvForward, x, y, x, y, iInitSAD, ppxlcRef0MBY, m_puciRefQZoom0, m_vopmd.iSearchRangeForward, m_volmd.bQuarterSample);
	pmvForward->computeTrueMV ();
	pmvForward->computeMV ();

	Int iSADDirect = 1000000000;
	CVector vctRefScaled;
	// TPS FIX

	// QuarterPel mwi 21JUL98
	// 	if (bColocatedMBExist && pmbmdRef->m_bhas4MVForward == FALSE &&
	// 		(m_volmd.volType != ENHN_LAYER || ( m_vopmd.iRefSelectCode != 1 && m_vopmd.iRefSelectCode != 2))) {
	if (bColocatedMBExist &&
      (m_volmd.volType != ENHN_LAYER || ( m_vopmd.iRefSelectCode != 1 && m_vopmd.iRefSelectCode != 2))) {
	  iSADDirect = directSAD(x, y, pmbmd, pmbmdRef, pmvRef);
	  Int iPartInterval = m_t - m_tPastRef;	
	  Int iFullInterval = m_tFutureRef - m_tPastRef;
	  for (Int iBlkNo=1; iBlkNo<=4; iBlkNo++) {
	    pmvDirectForward++;
	    pmvDirectBack++;
	    pmvRefBlk++;
	        
	    vctRefScaled = pmvRefBlk->trueMVHalfPel () * iPartInterval;        
	    vctRefScaled.x /= iFullInterval;
	    vctRefScaled.y /= iFullInterval;
	    //set up forward vector; 
	    pmvDirectForward->m_vctTrueHalfPel.x = vctRefScaled.x + pmbmd->m_vctDirectDeltaMV.x;
	    pmvDirectForward->m_vctTrueHalfPel.y = vctRefScaled.y + pmbmd->m_vctDirectDeltaMV.y;
	    pmvDirectForward->computeMV ();
	    //set up backward vector
	    backwardMVFromForwardMV (rgmvDirectBack [iBlkNo], rgmvDirectForward [iBlkNo], *pmvRefBlk, pmbmd->m_vctDirectDeltaMV);
	  }

// GMC
		if(pmbmdRef->m_bMCSEL)
			iSADDirect -= FAVORZERO;
		else
		if (pmbmd->m_vctDirectDeltaMV.x == 0 && pmbmd->m_vctDirectDeltaMV.y == 0)
        		iSADDirect -= FAVORZERO;
	}
// ~GMC

	  //		Int iPartInterval = m_t - m_tPastRef;		//initialize at MVDB = 0
	  //vctRefScaled = pmvRef->trueMVHalfPel () * iPartInterval;
	  //Int iFullInterval = m_tFutureRef - m_tPastRef;

		//        //for QuarterPel mwi 21JUL98
		//        iSADDirect = 0;      
		//        Int iBlkNo;
		//        for (iBlkNo=1; iBlkNo<=4; iBlkNo++) {
		//          pmvDirectForward++;
		//          pmvDirectBack++;
		//          pmvRefBlk++;
		//        
		//          vctRefScaled = pmvRefBlk->trueMVHalfPel () * iPartInterval;        
		//          vctRefScaled.x /= iFullInterval;
		//          vctRefScaled.y /= iFullInterval;
		//          //set up initial forward vector; 
		//          pmvDirectForward->m_vctTrueHalfPel.x = vctRefScaled.x; //trueHalfPel!!
		//          pmvDirectForward->m_vctTrueHalfPel.y = vctRefScaled.y;
		//          pmvDirectForward->computeMV ();
		//          //set up initial backward vector
		//          pmbmd->m_vctDirectDeltaMV = pmvDirectForward->m_vctTrueHalfPel - vctRefScaled; //
		//          backwardMVFromForwardMV (rgmvDirectBack [iBlkNo], rgmvDirectForward [iBlkNo], *pmvRefBlk, pmbmd->m_vctDirectDeltaMV);
		//          //compute initial sad
		//          iSADDirect += interpolateAndDiffY8 (rgmvDirectForward, rgmvDirectBack, x, y, iBlkNo,
		//                                              &m_rctRefVOPY0, &m_rctRefVOPY1) - FAVORZERO;
		//        }
        // 		vctRefScaled.x /= iFullInterval;				//truncation as per vm
        // 		vctRefScaled.y /= iFullInterval;				//truncation as per vm
        // 		//set up initial forward vector; 
        // 		rgmvDirectForward->iMVX = vctRefScaled.x / 2;
        // 		rgmvDirectForward->iMVY = vctRefScaled.y / 2;
        // 		rgmvDirectForward->iHalfX = 0;
        // 		rgmvDirectForward->iHalfY = 0;
        // 		rgmvDirectForward->computeTrueMV ();
        // 		//set up initial backward vector
        // 		pmbmd->m_vctDirectDeltaMV = rgmvDirectForward->m_vctTrueHalfPel - vctRefScaled;	//mvdb not necessaryly 0 due to truncation of half pel 
        // 		backwardMVFromForwardMV (rgmvDirectBack [0], rgmvDirectForward [0], *pmvRef, pmbmd->m_vctDirectDeltaMV);
        // 		//compute initial sad
        // 		iSADDirect = interpolateAndDiffY (rgmvDirectForward, rgmvDirectBack, x, y,
        // 			&m_rctRefVOPY0, &m_rctRefVOPY1) - FAVORZERO;
        // 		//set up inital position in ref frame
        // 		Int iXInit = x + rgmvDirectForward->iMVX;
        // 		Int iYInit = y + rgmvDirectForward->iMVY;
        // 		const PixelC* ppxlcInitRefMBY = m_pvopcRefQ0->pixelsY () + m_rctRefFrameY.offset (iXInit, iYInit);
        // 		//compute forward mv and sad; to be continue in iSADMin==iSADDirect
        // 		iSADDirect = blkmatch16 (rgmvDirectForward, iXInit, iYInit, x, y, iSADDirect, ppxlcInitRefMBY,
        // 	       	m_puciRefQZoom0, m_vopmd.iDirectModeRadius, m_volmd.bQuarterSample) - FAVOR_DIRECT;
        // 		rgmvDirectForward->computeTrueMV ();
        // 		pmbmd->m_vctDirectDeltaMV = rgmvDirectForward->m_vctTrueHalfPel - vctRefScaled;
        // 		backwardMVFromForwardMV (rgmvDirectBack [0], rgmvDirectForward [0], *pmvRef, 
        // 								 pmbmd->m_vctDirectDeltaMV);
        // 		iSADDirect = interpolateAndDiffY (rgmvDirectForward, rgmvDirectBack, x, y,
        // 										  &m_rctRefVOPY0, &m_rctRefVOPY1);
        // // GMC
        // 		if(pmbmdRef->m_bMCSEL)
        // 			iSADDirect -= FAVORZERO;
        // 		else
        // // ~GMC
        // 		if (pmbmd->m_vctDirectDeltaMV.x == 0 && pmbmd->m_vctDirectDeltaMV.y == 0)
        // 			iSADDirect -= FAVORZERO;

        // ~for QuarterPel mwi 21JUL98
	
	
	iInitSAD = sad16x16At0 (ppxlcRef1MBY) + FAVORZERO;
	Int iSADBackward = blkmatch16 (pmvBackward, x, y, x, y, iInitSAD, ppxlcRef1MBY, m_puciRefQZoom1, m_vopmd.iSearchRangeBackward, m_volmd.bQuarterSample);
	pmvBackward->computeTrueMV ();
	pmvBackward->computeMV ();
	Int iSADInterpolate = interpolateAndDiffY (
		pmvForward, pmvBackward,
		x, y, &m_rctRefVOPY0, &m_rctRefVOPY1
	);

	Int iSADMin = minimum (iSADDirect, iSADForward, iSADBackward, iSADInterpolate);
	
	if(m_bCodedFutureRef==FALSE)
		iSADMin = iSADForward; // force forward mode
	
	Int iBlk;
	if (iSADMin == iSADDirect) {
		pmbmd->m_mbType = DIRECT;
		//compute backward mv
		//pmvForward [0]  = rgmvDirectForward  [0];
		//pmvBackward [0] = rgmvDirectBack [0];
		for (iBlk = 0; iBlk <= 4; iBlk++) {
		  pmvForward [iBlk] = rgmvDirectForward [iBlk];
		  pmvBackward [iBlk] = rgmvDirectBack [iBlk];
		}
	}
	else if (iSADMin == iSADForward) {
		pmbmd->m_mbType = FORWARD;
		for (iBlk = 1; iBlk <= 4; iBlk++)
			pmvForward [iBlk] = pmvForward [0];
	}
	else if (iSADMin == iSADBackward) {
		pmbmd->m_mbType = BACKWARD;
		for (iBlk = 1; iBlk <= 4; iBlk++)
			pmvBackward [iBlk] = pmvBackward [0];
	}
	else {
		pmbmd->m_mbType = INTERPOLATE;
		for (iBlk = 1; iBlk <= 4; iBlk++) {
			pmvForward [iBlk] = pmvForward [0];
			pmvBackward [iBlk] = pmvBackward [0];
		}
	}
	return iSADMin;
}

// for spatial scalability only
Int CVideoObjectEncoder::motionEstMB_BVOP (
	CoordI x, CoordI y, 
	CMotionVector* pmvForward, CMotionVector* pmvBackward,
	CMBMode* pmbmd,
	const PixelC* ppxlcRef0MBY, const PixelC* ppxlcRef1MBY
)
{
	Int iInitSAD = sad16x16At0 (ppxlcRef0MBY);
	Int iSADForward = blkmatch16 (pmvForward, x, y,x,y, iInitSAD, ppxlcRef0MBY, m_puciRefQZoom0, m_vopmd.iSearchRangeForward, m_volmd.bQuarterSample);

	pmvForward->computeTrueMV ();
	pmvForward->computeMV ();
	//	Int iSADDirect = 1000000000;

	iInitSAD = sad16x16At0 (ppxlcRef1MBY);
	Int iSADBackward = iInitSAD;
	*pmvBackward = CMotionVector (0,0);
	pmvBackward->computeTrueMV ();
	pmvBackward->computeMV ();
	Int iSADInterpolate = interpolateAndDiffY (pmvForward, pmvBackward, x, y,
			&m_rctRefVOPY0, &m_rctRefVOPY1);

	Int iSADMin=0;
	if(iSADForward <= iSADBackward && iSADForward <= iSADInterpolate)
		iSADMin = iSADForward;
	else if (iSADBackward <= iSADInterpolate)
		iSADMin = iSADBackward;
	else
		iSADMin = iSADInterpolate;
	Int iBlk;
	if (iSADMin == iSADForward) {
		pmbmd->m_mbType = FORWARD;
		for (iBlk = 1; iBlk <= 4; iBlk++)
			pmvForward [iBlk] = pmvForward [0];
	}
	else if (iSADMin == iSADBackward) {
		pmbmd->m_mbType = BACKWARD;
		for (iBlk = 1; iBlk <= 4; iBlk++)
			pmvBackward [iBlk] = pmvBackward [0];
	}
	else {
		pmbmd->m_mbType = INTERPOLATE;
		for (iBlk = 1; iBlk <= 4; iBlk++) {
			pmvForward [iBlk] = pmvForward [0];
			pmvBackward [iBlk] = pmvBackward [0];
		}
	}
	return iSADMin;
}

Int CVideoObjectEncoder::motionEstMB_BVOP_WithShape (
	CoordI x, CoordI y, 
	CMotionVector* pmvForward, CMotionVector* pmvBackward,
	CMBMode* pmbmd, 
	const CMBMode* pmbmdRef, const CMotionVector* pmvRef,
	const PixelC* ppxlcRef0MBY, const PixelC* ppxlcRef1MBY,
	Bool bColocatedMBExist
)
{
	assert (pmbmd->m_rgTranspStatus [0] == PARTIAL);

	Int iInitSAD = sad16x16At0WithShape (ppxlcRef0MBY, pmbmd);	//shouldn't favor 0 mv inside this function
	Int iSADForward = blkmatch16WithShape (pmvForward, x, y, x, y, iInitSAD, ppxlcRef0MBY,
        m_puciRefQZoom0, pmbmd, m_vopmd.iSearchRangeForward, m_volmd.bQuarterSample,0);
	pmvForward->computeTrueMV ();
	pmvForward->computeMV ();
	Int iSADDirect = 1000000000;
	CVector vctRefScaled;
	if (bColocatedMBExist && pmbmdRef->m_bhas4MVForward == FALSE && 
		(m_volmd.volType != ENHN_LAYER || ( m_vopmd.iRefSelectCode != 1 && m_vopmd.iRefSelectCode != 2))) {
		Int iPartInterval = m_t - m_tPastRef;		//initialize at MVDB = 0
		vctRefScaled = pmvRef->trueMVHalfPel () * iPartInterval;
		Int iFullInterval = m_tFutureRef - m_tPastRef;
		vctRefScaled.x /= iFullInterval;						//truncation as per vm
		vctRefScaled.y /= iFullInterval;						//truncation as per vm
		//set up initial forward vector; 
		rgmvDirectForward->iMVX = vctRefScaled.x / 2;
		rgmvDirectForward->iMVY = vctRefScaled.y / 2;
		rgmvDirectForward->iHalfX = 0;
		rgmvDirectForward->iHalfY = 0;
		rgmvDirectForward->computeTrueMV ();
		rgmvDirectForward->computeMV ();
		//set up initial backward vector
		pmbmd->m_vctDirectDeltaMV = rgmvDirectForward->m_vctTrueHalfPel - vctRefScaled;	//mvdb not necessaryly 0 due to truncation of half pel 
		backwardMVFromForwardMV (rgmvDirectBack [0], rgmvDirectForward [0], *pmvRef, pmbmd->m_vctDirectDeltaMV);
		//compute initial sad
		iSADDirect = interpolateAndDiffY_WithShape (rgmvDirectForward, rgmvDirectBack, x, y,
			&m_rctRefVOPY0, &m_rctRefVOPY1) - FAVORZERO;;
		//set up inital position in ref frame
		Int iXInit = x + rgmvDirectForward->iMVX;
		Int iYInit = y + rgmvDirectForward->iMVY;
		const PixelC* ppxlcInitRefMBY = m_pvopcRefQ0->pixelsY () + m_rctRefFrameY.offset (iXInit, iYInit);
		//compute forward mv and sad; to be continue in iSADMin==iSADDirect
		Int isave = m_iMVFileUsage;                       // 04/28/99 david ruhoff
		if (!m_volmd.bOriginalForME) m_iMVFileUsage = 0;  // 04/28/99  must not use mv file based on reconstructed vop
		iSADDirect = blkmatch16WithShape  (rgmvDirectForward, iXInit, iYInit, x, y, iSADDirect, ppxlcInitRefMBY,
	        m_puciRefQZoom0, pmbmd, m_vopmd.iDirectModeRadius, m_volmd.bQuarterSample,0) - FAVOR_DIRECT;
		m_iMVFileUsage = isave;                           // 04/28/99 
		rgmvDirectForward->computeTrueMV ();
		rgmvDirectForward->computeMV ();
		pmbmd->m_vctDirectDeltaMV = rgmvDirectForward->m_vctTrueHalfPel - vctRefScaled;
		backwardMVFromForwardMV (rgmvDirectBack [0], rgmvDirectForward [0], *pmvRef, 
								 pmbmd->m_vctDirectDeltaMV);
		iSADDirect = interpolateAndDiffY (rgmvDirectForward, rgmvDirectBack, x, y,
										  &m_rctRefVOPY0, &m_rctRefVOPY1);
// GMC
		if(pmbmdRef->m_bMCSEL)
			iSADDirect -= FAVORZERO;
		else
// ~GMC
		if (pmbmd->m_vctDirectDeltaMV.x == 0 && pmbmd->m_vctDirectDeltaMV.y == 0)
			iSADDirect -= FAVORZERO;
	}

	iInitSAD = sad16x16At0WithShape (ppxlcRef1MBY, pmbmd);//shouldn't favor 0 mv inside this function
    Int iSADBackward = blkmatch16WithShape (pmvBackward, x, y, x, y, iInitSAD, ppxlcRef1MBY,
	    m_puciRefQZoom1, pmbmd, m_vopmd.iSearchRangeBackward, m_volmd.bQuarterSample,1);
	pmvBackward->computeTrueMV ();
	pmvBackward->computeMV ();
	Int iSADInterpolate = interpolateAndDiffY_WithShape (
		pmvForward, pmvBackward,
		x, y, &m_rctRefVOPY0, &m_rctRefVOPY1
	);

	Int iSADMin = minimum (iSADDirect, iSADForward, iSADBackward, iSADInterpolate);
	Int iBlk;
	
	if(m_bCodedFutureRef==FALSE)
		iSADMin = iSADForward; // force forward mode
	
	if (iSADMin == iSADDirect) {
		pmbmd->m_mbType = DIRECT;
		//compute backward mv
		pmvForward [0] = rgmvDirectForward [0];
		pmvBackward [0] = rgmvDirectBack [0];
		for (iBlk = 1; iBlk <= 4; iBlk++) {
			pmvForward [iBlk] = pmvForward [0];
			pmvBackward [iBlk] = pmvBackward [0];
		}
	}
	else if (iSADMin == iSADForward) {
		pmbmd->m_mbType = FORWARD;
		for (iBlk = 1; iBlk <= 4; iBlk++)
			pmvForward [iBlk] = pmvForward [0];
	}
	else if (iSADMin == iSADBackward) {
		pmbmd->m_mbType = BACKWARD;
		for (iBlk = 1; iBlk <= 4; iBlk++)
			pmvBackward [iBlk] = pmvBackward [0];
	}
	else {
		pmbmd->m_mbType = INTERPOLATE;
		for (iBlk = 1; iBlk <= 4; iBlk++) {
			pmvForward [iBlk] = pmvForward [0];
			pmvBackward [iBlk] = pmvBackward [0];
		}
	}
	return iSADMin;
}

//OBSS_SAIT_991015
Int CVideoObjectEncoder::motionEstMB_BVOP_WithShape (
	CoordI x, CoordI y, CMotionVector* pmvForward, CMotionVector* pmvBackward,
	CMBMode* pmbmd, const PixelC* ppxlcRef0MBY, const PixelC* ppxlcRef1MBY
)
{
	Int numForePixel=0;Bool BackwardSkipped = FALSE;		
	assert (pmbmd->m_rgTranspStatus [0] == PARTIAL);

	Int iInitSAD = sad16x16At0WithShape (ppxlcRef0MBY, pmbmd);	//shouldn't favor 0 mv inside this function
	Int iSADForward = blkmatch16WithShape (pmvForward, x, y, x, y, iInitSAD, ppxlcRef0MBY,
        m_puciRefQZoom0, pmbmd, m_vopmd.iSearchRangeForward,m_volmd.bQuarterSample,0);

	pmvForward->computeTrueMV ();
	//	Int iSADDirect = 1000000000;

	iInitSAD = sad16x16At0WithShape (ppxlcRef1MBY, pmbmd, &numForePixel);
	Int iSADBackward = iInitSAD;
	*pmvBackward = CMotionVector( 0, 0 );
	pmvBackward->computeTrueMV ();
	BackwardSkipped = quantizeTextureMBBackwardCheck (pmbmd, m_ppxlcCurrMBY, m_ppxlcCurrMBBY, ppxlcRef1MBY);

	Int iSADInterpolate = interpolateAndDiffY_WithShape(pmvForward, pmvBackward, x, y,
							&m_rctRefVOPY0, &m_rctRefVOPY1);

	
	Int iSADMin = 0;
	if(BackwardSkipped)						
			iSADMin = iSADBackward;			
	else {
		if(iSADForward <= iSADBackward && iSADForward <= iSADInterpolate)
			iSADMin = iSADForward;
		else if (iSADBackward <= iSADInterpolate)
			iSADMin = iSADBackward;
		else
			iSADMin = iSADInterpolate;
	}
	Int iBlk;

	if (iSADMin == iSADForward) {
		pmbmd->m_mbType = FORWARD;
		for (iBlk = 1; iBlk <= 4; iBlk++)
			pmvForward [iBlk] = pmvForward [0];
	}
	else if (iSADMin == iSADBackward) {
		pmbmd->m_mbType = BACKWARD;
		for (iBlk = 1; iBlk <= 4; iBlk++)
			pmvBackward [iBlk] = pmvBackward [0];
	}
	else {
		pmbmd->m_mbType = INTERPOLATE;
		for (iBlk = 1; iBlk <= 4; iBlk++) {
			pmvForward [iBlk] = pmvForward [0];
			pmvBackward [iBlk] = pmvBackward [0];
		}
	}
	return iSADMin;
}
//~OBSS_SAIT_991015

// INTERLACE
#define	BbiasAgainstFrameAve	((MB_SIZE*MB_SIZE)/2 + 1)		// Biases favor modes with fewer MVs
#define BbiasAgainstFieldAve	((MB_SIZE*MB_SIZE)/1 + 1)
#define BbiasAgainstField		((MB_SIZE*MB_SIZE)/2 + 1)
#define	BbiasFavorDirect		((MB_SIZE*MB_SIZE)/2 + 1)

Int CVideoObjectEncoder::motionEstMB_BVOP_Interlaced (
	CoordI x, CoordI y, 
	CMotionVector* pmvForward, CMotionVector* pmvBackward,
	CMBMode* pmbmd,
	const CMBMode* pmbmdRef, const CMotionVector* pmvRef,
	const PixelC* ppxlcRef0MBY, const PixelC* ppxlcRef1MBY,
	Bool bColocatedMBExist
)
{
	Int iSAD, iSADnobias, iFieldSAD, iSADFrmMB[3];
	MBType mbtype;

	iSADFrmMB[0] = blkmatch16(pmvForward, x, y,x,y, sad16x16At0 (ppxlcRef0MBY),
		ppxlcRef0MBY, m_puciRefQZoom0, m_vopmd.iSearchRangeForward, m_volmd.bQuarterSample);
	pmvForward->computeTrueMV ();
	pmvForward->computeMV ();
    
	iSADFrmMB[1] = blkmatch16(pmvBackward, x, y,x,y, sad16x16At0 (ppxlcRef1MBY),
		ppxlcRef1MBY, m_puciRefQZoom1, m_vopmd.iSearchRangeBackward, m_volmd.bQuarterSample);
	pmvBackward->computeTrueMV ();
	pmvBackward->computeMV ();
	iSADFrmMB[2] = interpolateAndDiffY(pmvForward, pmvBackward, x, y
		, &m_rctRefVOPY0, &m_rctRefVOPY1);

#ifdef __TRACE_AND_STATS_
	m_pbitstrmOut->trace (iSADFrmMB, 3, "SADFrmMB");
#endif // __TRACE_AND_STATS_

	// Choose best 16x16 mode
	pmbmd->m_bFieldMV = FALSE;
	if (iSADFrmMB[0] < iSADFrmMB[1]) {
		iSAD = iSADFrmMB[0];
		pmbmd->m_mbType = FORWARD;
	} else {
		iSAD = iSADFrmMB[1];
		pmbmd->m_mbType = BACKWARD;
	}
	iSADnobias = iSAD;
	if ((iSADFrmMB[2] + BbiasAgainstFrameAve) < iSAD) {
		iSAD = iSADFrmMB[2] + BbiasAgainstFrameAve;
		iSADnobias = iSADFrmMB[2];
		pmbmd->m_mbType = INTERPOLATE;
	}

		
	if(m_bCodedFutureRef==FALSE)
	{
		iSAD = iSADFrmMB[0];
		pmbmd->m_mbType = FORWARD; // force forward mode
	}
	
	if (m_vopmd.bInterlace) {
		const PixelC *ppxlcFwdHalfPelRef = m_pvopcRefQ0->pixelsY()
			+EXPANDY_REF_FRAME * m_iFrameWidthY + EXPANDY_REF_FRAME+ x + y * m_iFrameWidthY; // 1.31.99 changes
		const PixelC *ppxlcBakHalfPelRef = m_pvopcRefQ1->pixelsY()
			+EXPANDY_REF_FRAME * m_iFrameWidthY + EXPANDY_REF_FRAME+ x + y * m_iFrameWidthY; // 1.31.99 changes
		Int iSADFldMB[3], iSADFld[8];

		// Choose best fwd top field predictor    // ppxlc???HalfPelRef is not zoomed [interpolation within blkmatch??()]
		iSADFld[0] = blkmatch16x8 (pmvForward+1, x, y, 0,
			ppxlcRef0MBY, ppxlcFwdHalfPelRef, m_vopmd.iSearchRangeForward, m_volmd.bQuarterSample);
		pmvForward[1].computeTrueMV();
		pmvForward[1].computeMV();
		iSADFld[1] = blkmatch16x8 (pmvForward+2, x, y, 0,
			ppxlcRef0MBY + m_iFrameWidthY, ppxlcFwdHalfPelRef + m_iFrameWidthY,
			m_vopmd.iSearchRangeForward, m_volmd.bQuarterSample);
		pmvForward[2].computeTrueMV();
		pmvForward[2].computeMV();
		if (iSADFld[0] <= iSADFld[1]) {
			pmbmd->m_bForwardTop = 0;
			iSADFldMB[0] = iSADFld[0];
		} else {
			pmbmd->m_bForwardTop = 1;
			iSADFldMB[0] = iSADFld[1];
		}

		// Choose best fwd botton field predictor
		iSADFld[2] = blkmatch16x8 (pmvForward+3, x, y, MB_SIZE,
			ppxlcRef0MBY, ppxlcFwdHalfPelRef, m_vopmd.iSearchRangeForward, m_volmd.bQuarterSample);
		pmvForward[3].computeTrueMV();
		pmvForward[3].computeMV();
		iSADFld[3] = blkmatch16x8 (pmvForward+4, x, y, MB_SIZE,
			ppxlcRef0MBY + m_iFrameWidthY, ppxlcFwdHalfPelRef + m_iFrameWidthY,
			m_vopmd.iSearchRangeForward, m_volmd.bQuarterSample);
		pmvForward[4].computeTrueMV();
		pmvForward[4].computeMV();
		if (iSADFld[2] < iSADFld[3]) {
			pmbmd->m_bForwardBottom = 0;
			iSADFldMB[0] += iSADFld[2];
		} else {
			pmbmd->m_bForwardBottom = 1;
			iSADFldMB[0] += iSADFld[3];
		}

		
		// Choose best bak top field predictor
		iSADFld[4] = blkmatch16x8 (pmvBackward+1, x, y, 0,
			ppxlcRef1MBY, ppxlcBakHalfPelRef, m_vopmd.iSearchRangeBackward, m_volmd.bQuarterSample);
		pmvBackward[1].computeTrueMV();
		pmvBackward[1].computeMV();
		iSADFld[5] = blkmatch16x8 (pmvBackward+2, x, y, 0,
			ppxlcRef1MBY + m_iFrameWidthY, ppxlcBakHalfPelRef + m_iFrameWidthY,
			m_vopmd.iSearchRangeBackward, m_volmd.bQuarterSample);
		pmvBackward[2].computeTrueMV();
		pmvBackward[2].computeMV();
		if (iSADFld[4] <= iSADFld[5]) {
			pmbmd->m_bBackwardTop = 0;
			iSADFldMB[1] = iSADFld[4];
		} else {
			pmbmd->m_bBackwardTop = 1;
			iSADFldMB[1] = iSADFld[5];
		}

		// Choose best bak bottom field predictor
		iSADFld[6] = blkmatch16x8 (pmvBackward+3, x, y, MB_SIZE,
			ppxlcRef1MBY, ppxlcBakHalfPelRef, m_vopmd.iSearchRangeBackward, m_volmd.bQuarterSample);
		pmvBackward[3].computeTrueMV();
		pmvBackward[3].computeMV();
		iSADFld[7] = blkmatch16x8 (pmvBackward+4, x, y, MB_SIZE,
			ppxlcRef1MBY + m_iFrameWidthY, ppxlcBakHalfPelRef + m_iFrameWidthY,
			m_vopmd.iSearchRangeBackward, m_volmd.bQuarterSample);
		pmvBackward[4].computeTrueMV();
		pmvBackward[4].computeMV();
		if (iSADFld[6] < iSADFld[7]) {
			pmbmd->m_bBackwardBottom = 0;
			iSADFldMB[1] += iSADFld[6];
		} else {
			pmbmd->m_bBackwardBottom = 1;
			iSADFldMB[1] += iSADFld[7];
		}

		// Choose best of forward & backward field prediction
		if (iSADFldMB[0] <= iSADFldMB[1]) {
			iFieldSAD = iSADFldMB[0];
			mbtype = FORWARD;
		} else {
			iFieldSAD = iSADFldMB[1];
			mbtype = BACKWARD;
		}
		iFieldSAD += BbiasAgainstField;
		iSADFldMB[2] = interpolateAndDiffYField(
			pmvForward  + 1 + (int)pmbmd->m_bForwardTop,
			pmvForward  + 3 + (int)pmbmd->m_bForwardBottom,
			pmvBackward + 1 + (int)pmbmd->m_bBackwardTop,
			pmvBackward + 3 + (int)pmbmd->m_bBackwardBottom,
			x, y, pmbmd);
		if ((iSADFldMB[2] + BbiasAgainstFieldAve) < iFieldSAD) {
			iFieldSAD = iSADFldMB[2] + BbiasAgainstFieldAve;
			mbtype = INTERPOLATE;
		}

		if(m_bCodedFutureRef==FALSE) // FORCE FORWARD
		{
			iFieldSAD = iSADFldMB[0];
			mbtype = FORWARD;
		}

		// Field versus Frame prediction
		if (iFieldSAD < iSAD) {
			pmbmd->m_bFieldMV = TRUE;
			pmbmd->m_mbType = mbtype;
			iSAD = iFieldSAD;
			iSADnobias = iFieldSAD - ((mbtype == INTERPOLATE) ? BbiasAgainstFieldAve : BbiasAgainstField);
		}
#ifdef __TRACE_AND_STATS_
		m_pbitstrmOut->trace (iSADFld,   8, "SADFld");
		m_pbitstrmOut->trace (iSADFldMB, 3, "SADFldMB");
#endif // __TRACE_AND_STATS_

	}

#if 1				// #if 0 to temporarily disable direct mode (debug only)
	// Check direct mode
	Int iSADDirect = 0;
	if (m_volmd.fAUsage == RECTANGLE) {	
		iSADDirect= (pmbmdRef->m_bFieldMV && !pmbmdRef->m_bMCSEL) ? // GMC
		directSADField(x, y, pmbmd, pmbmdRef, pmvRef, ppxlcRef0MBY, ppxlcRef1MBY) :
		directSAD     (x, y, pmbmd, pmbmdRef, pmvRef);
		if ((iSADDirect - BbiasFavorDirect) < iSAD) {
			pmbmd->m_mbType = DIRECT;
			pmbmd->m_bFieldMV = pmbmdRef->m_bFieldMV;
			iSADnobias = iSADDirect;
		}
	}
#endif
#ifdef __TRACE_AND_STATS_
	static char *cDirectType[] = {	"FrmDirectSAD", "FldDirectSAD" };
	static char *cWinner[] = { "FrmDirect\n", "FrmAve\n", "FrmBak\n", "FrmFwd\n", "FldDirect\n", "FldAve\n", "FldBak\n", "FldFwd\n" };
	if(bColocatedMBExist)
      m_pbitstrmOut->trace (iSADDirect, cDirectType[pmbmdRef->m_bFieldMV]);
	m_pbitstrmOut->trace (cWinner[(Int)pmbmd->m_mbType + 4*pmbmd->m_bFieldMV]);
#if 0				// Put all MV candidates in trace file
	Int iMV[10];
	iMV[0] = pmvForward[0].m_vctTrueHalfPel.x; iMV[1] = pmvForward[0].m_vctTrueHalfPel.y;
	iMV[2] = pmvForward[1].m_vctTrueHalfPel.x; iMV[3] = pmvForward[1].m_vctTrueHalfPel.y;
	iMV[4] = pmvForward[2].m_vctTrueHalfPel.x; iMV[5] = pmvForward[2].m_vctTrueHalfPel.y;
	iMV[6] = pmvForward[3].m_vctTrueHalfPel.x; iMV[7] = pmvForward[3].m_vctTrueHalfPel.y;
	iMV[8] = pmvForward[4].m_vctTrueHalfPel.x; iMV[9] = pmvForward[4].m_vctTrueHalfPel.y;
	m_pbitstrmOut->trace (iMV, 10, "FwdMV");
	iMV[0] = pmvBackward[0].m_vctTrueHalfPel.x; iMV[1] = pmvBackward[0].m_vctTrueHalfPel.y;
	iMV[2] = pmvBackward[1].m_vctTrueHalfPel.x; iMV[3] = pmvBackward[1].m_vctTrueHalfPel.y;
	iMV[4] = pmvBackward[2].m_vctTrueHalfPel.x; iMV[5] = pmvBackward[2].m_vctTrueHalfPel.y;
	iMV[6] = pmvBackward[3].m_vctTrueHalfPel.x; iMV[7] = pmvBackward[3].m_vctTrueHalfPel.y;
	iMV[8] = pmvBackward[4].m_vctTrueHalfPel.x; iMV[9] = pmvBackward[4].m_vctTrueHalfPel.y;
	m_pbitstrmOut->trace (iMV, 10, "BakMV");
#endif
#endif // __TRACE_AND_STATS_

	pmbmd->m_bhas4MVForward  = FALSE;
	pmbmd->m_bhas4MVBackward = FALSE;
	return iSADnobias;
}

//end of INTERLACED
//INTERLACED
// new changes
Int CVideoObjectEncoder::motionEstMB_BVOP_InterlacedWithShape (
	CoordI x, CoordI y, 
	CMotionVector* pmvForward, CMotionVector* pmvBackward,
	CMBMode* pmbmd,
	const CMBMode* pmbmdRef, const CMotionVector* pmvRef,
	const PixelC* ppxlcRef0MBY, const PixelC* ppxlcRef1MBY,
	Bool bColocatedMBExist
)
{
	assert (pmbmd->m_rgTranspStatus [0] == PARTIAL);

	Int iSAD, iSADnobias, iFieldSAD, iSADFrmMB[3];
	MBType mbtype;

	iSADFrmMB[0] = blkmatch16WithShape(pmvForward, x, y,x,y, sad16x16At0WithShape (ppxlcRef0MBY,pmbmd),
		ppxlcRef0MBY, m_puciRefQZoom0, pmbmd, m_vopmd.iSearchRangeForward, m_volmd.bQuarterSample,0);
	pmvForward->computeTrueMV ();
	pmvForward->computeMV (); // to make pmv unique
	iSADFrmMB[1] = blkmatch16WithShape(pmvBackward, x, y,x,y, sad16x16At0WithShape (ppxlcRef1MBY,pmbmd),
		ppxlcRef1MBY, m_puciRefQZoom1, pmbmd, m_vopmd.iSearchRangeBackward, m_volmd.bQuarterSample,1);
	pmvBackward->computeTrueMV ();
	pmvBackward->computeMV (); // to make pmv unique
	iSADFrmMB[2] = interpolateAndDiffY_WithShape(pmvForward, pmvBackward, x, y
		, &m_rctRefVOPY0, &m_rctRefVOPY1);

#ifdef __TRACE_AND_STATS_
	m_pbitstrmOut->trace (iSADFrmMB, 3, "SADFrmMB");
#endif // __TRACE_AND_STATS_

	// Choose best 16x16 mode
	pmbmd->m_bFieldMV = FALSE;
	if (iSADFrmMB[0] < iSADFrmMB[1]) {
		iSAD = iSADFrmMB[0];
		pmbmd->m_mbType = FORWARD;
	} else {
		iSAD = iSADFrmMB[1];
		pmbmd->m_mbType = BACKWARD;
	}
	iSADnobias = iSAD;
	if ((iSADFrmMB[2] + BbiasAgainstFrameAve) < iSAD) {
		iSAD = iSADFrmMB[2] + BbiasAgainstFrameAve;
		iSADnobias = iSADFrmMB[2];
		pmbmd->m_mbType = INTERPOLATE;
	}

	if(m_bCodedFutureRef==FALSE) // force forward
	{
		iSAD = iSADFrmMB[0];
		pmbmd->m_mbType = FORWARD;
	}
	
	if (m_vopmd.bInterlace) {
		const PixelC *ppxlcFwdHalfPelRef = m_pvopcRefQ0->pixelsY()
			+EXPANDY_REF_FRAME * m_iFrameWidthY + EXPANDY_REF_FRAME+ x + y * m_iFrameWidthY; // 1.31.99 changes
		const PixelC *ppxlcBakHalfPelRef = m_pvopcRefQ1->pixelsY()
			+EXPANDY_REF_FRAME * m_iFrameWidthY + EXPANDY_REF_FRAME+ x + y * m_iFrameWidthY; // 1.31.99 changes
		Int iSADFldMB[3], iSADFld[8];

		// Choose best fwd top field predictor
		iSADFld[0] = blkmatch16x8WithShape (pmvForward+1, x, y, 0,
			ppxlcRef0MBY, ppxlcFwdHalfPelRef, m_vopmd.iSearchRangeForward, m_volmd.bQuarterSample,0);
		pmvForward[1].computeTrueMV();
		pmvForward[1].computeMV();
		iSADFld[1] = blkmatch16x8WithShape (pmvForward+2, x, y, 0,
			ppxlcRef0MBY + m_iFrameWidthY, ppxlcFwdHalfPelRef + m_iFrameWidthY,
			m_vopmd.iSearchRangeForward, m_volmd.bQuarterSample,0);
		pmvForward[2].computeTrueMV();
		pmvForward[2].computeMV();
		if (iSADFld[0] <= iSADFld[1]) {
			pmbmd->m_bForwardTop = 0;
			iSADFldMB[0] = iSADFld[0];
		} else {
			pmbmd->m_bForwardTop = 1;
			iSADFldMB[0] = iSADFld[1];
		}

		// Choose best fwd botton field predictor
		iSADFld[2] = blkmatch16x8WithShape (pmvForward+3, x, y, MB_SIZE,
			ppxlcRef0MBY, ppxlcFwdHalfPelRef, m_vopmd.iSearchRangeForward, m_volmd.bQuarterSample,0);
		pmvForward[3].computeTrueMV();
		pmvForward[3].computeMV();
		iSADFld[3] = blkmatch16x8WithShape (pmvForward+4, x, y, MB_SIZE,
			ppxlcRef0MBY + m_iFrameWidthY, ppxlcFwdHalfPelRef + m_iFrameWidthY,
			m_vopmd.iSearchRangeForward, m_volmd.bQuarterSample,0);
		pmvForward[4].computeTrueMV();
		pmvForward[4].computeMV();
		if (iSADFld[2] < iSADFld[3]) {
			pmbmd->m_bForwardBottom = 0;
			iSADFldMB[0] += iSADFld[2];
		} else {
			pmbmd->m_bForwardBottom = 1;
			iSADFldMB[0] += iSADFld[3];
		}

		
		// Choose best bak top field predictor
		iSADFld[4] = blkmatch16x8WithShape (pmvBackward+1, x, y, 0,
			ppxlcRef1MBY, ppxlcBakHalfPelRef, m_vopmd.iSearchRangeBackward, m_volmd.bQuarterSample,1);
		pmvBackward[1].computeTrueMV();
		pmvBackward[1].computeMV();
		iSADFld[5] = blkmatch16x8WithShape (pmvBackward+2, x, y, 0,
			ppxlcRef1MBY + m_iFrameWidthY, ppxlcBakHalfPelRef + m_iFrameWidthY,
			m_vopmd.iSearchRangeBackward, m_volmd.bQuarterSample,1);
		pmvBackward[2].computeTrueMV();
		pmvBackward[2].computeMV();
		if (iSADFld[4] <= iSADFld[5]) {
			pmbmd->m_bBackwardTop = 0;
			iSADFldMB[1] = iSADFld[4];
		} else {
			pmbmd->m_bBackwardTop = 1;
			iSADFldMB[1] = iSADFld[5];
		}

		// Choose best bak bottom field predictor
		iSADFld[6] = blkmatch16x8WithShape (pmvBackward+3, x, y, MB_SIZE,
			ppxlcRef1MBY, ppxlcBakHalfPelRef, m_vopmd.iSearchRangeBackward, m_volmd.bQuarterSample,1);
		pmvBackward[3].computeTrueMV();
		pmvBackward[3].computeMV();
		iSADFld[7] = blkmatch16x8WithShape (pmvBackward+4, x, y, MB_SIZE,
			ppxlcRef1MBY + m_iFrameWidthY, ppxlcBakHalfPelRef + m_iFrameWidthY,
			m_vopmd.iSearchRangeBackward, m_volmd.bQuarterSample,1);
		pmvBackward[4].computeTrueMV();
		pmvBackward[4].computeMV();
		if (iSADFld[6] < iSADFld[7]) {
			pmbmd->m_bBackwardBottom = 0;
			iSADFldMB[1] += iSADFld[6];
		} else {
			pmbmd->m_bBackwardBottom = 1;
			iSADFldMB[1] += iSADFld[7];
		}

		// Choose best of forward & backward field prediction
		if (iSADFldMB[0] <= iSADFldMB[1]) {
			iFieldSAD = iSADFldMB[0];
			mbtype = FORWARD;
		} else {
			iFieldSAD = iSADFldMB[1];
			mbtype = BACKWARD;
		}
		iFieldSAD += BbiasAgainstField;
		iSADFldMB[2] = interpolateAndDiffYField(
			pmvForward  + 1 + (int)pmbmd->m_bForwardTop,
			pmvForward  + 3 + (int)pmbmd->m_bForwardBottom,
			pmvBackward + 1 + (int)pmbmd->m_bBackwardTop,
			pmvBackward + 3 + (int)pmbmd->m_bBackwardBottom,
			x, y, pmbmd);
		if ((iSADFldMB[2] + BbiasAgainstFieldAve) < iFieldSAD) {
			iFieldSAD = iSADFldMB[2] + BbiasAgainstFieldAve;
			mbtype = INTERPOLATE;
		}

		if(m_bCodedFutureRef==FALSE) // force forward
		{
			iFieldSAD = iSADFldMB[0];
			mbtype = FORWARD;
		}

		// Field versus Frame prediction
		if (iFieldSAD < iSAD) {
			pmbmd->m_bFieldMV = TRUE;
			pmbmd->m_mbType = mbtype;
			iSAD = iFieldSAD;
			iSADnobias = iFieldSAD - ((mbtype == INTERPOLATE) ? BbiasAgainstFieldAve : BbiasAgainstField);
		}
#ifdef __TRACE_AND_STATS_
		m_pbitstrmOut->trace (iSADFld,   8, "SADFld");
		m_pbitstrmOut->trace (iSADFldMB, 3, "SADFldMB");
#endif // __TRACE_AND_STATS_

	}

#if 1				// #if 0 to temporarily disable direct mode (debug only)
	// Check direct mode
	// new changes
/*	Int iSADDirect;
	if (bColocatedMBExist) {
		iSADDirect= (pmbmdRef->m_bFieldMV && !pmbmdRef->m_bMCSEL) ? // GMC
			directSADField(x, y, pmbmd, pmbmdRef, pmvRef, ppxlcRef0MBY, ppxlcRef1MBY) :
			directSAD     (x, y, pmbmd, pmbmdRef, pmvRef);
		if ((iSADDirect - BbiasFavorDirect) < iSAD) {
			pmbmd->m_mbType = DIRECT;
			pmbmd->m_bFieldMV = pmbmdRef->m_bFieldMV;
			iSADnobias = iSADDirect;
		}
	}  */
#endif
#ifdef __TRACE_AND_STATS_
/*	static char *cDirectType[] = {	"FrmDirectSAD", "FldDirectSAD" };
	static char *cWinner[] = { "FrmDirect\n", "FrmAve\n", "FrmBak\n", "FrmFwd\n", "FldDirect\n", "FldAve\n", "FldBak\n", "FldFwd\n" };
	if (bColocatedMBExist)
		m_pbitstrmOut->trace (iSADDirect, cDirectType[pmbmdRef->m_bFieldMV]);
	m_pbitstrmOut->trace (cWinner[(Int)pmbmd->m_mbType + 4*pmbmd->m_bFieldMV]); */
#if 0				// Put all MV candidates in trace file
	Int iMV[10];
	iMV[0] = pmvForward[0].m_vctTrueHalfPel.x; iMV[1] = pmvForward[0].m_vctTrueHalfPel.y;
	iMV[2] = pmvForward[1].m_vctTrueHalfPel.x; iMV[3] = pmvForward[1].m_vctTrueHalfPel.y;
	iMV[4] = pmvForward[2].m_vctTrueHalfPel.x; iMV[5] = pmvForward[2].m_vctTrueHalfPel.y;
	iMV[6] = pmvForward[3].m_vctTrueHalfPel.x; iMV[7] = pmvForward[3].m_vctTrueHalfPel.y;
	iMV[8] = pmvForward[4].m_vctTrueHalfPel.x; iMV[9] = pmvForward[4].m_vctTrueHalfPel.y;
	m_pbitstrmOut->trace (iMV, 10, "FwdMV");
	iMV[0] = pmvBackward[0].m_vctTrueHalfPel.x; iMV[1] = pmvBackward[0].m_vctTrueHalfPel.y;
	iMV[2] = pmvBackward[1].m_vctTrueHalfPel.x; iMV[3] = pmvBackward[1].m_vctTrueHalfPel.y;
	iMV[4] = pmvBackward[2].m_vctTrueHalfPel.x; iMV[5] = pmvBackward[2].m_vctTrueHalfPel.y;
	iMV[6] = pmvBackward[3].m_vctTrueHalfPel.x; iMV[7] = pmvBackward[3].m_vctTrueHalfPel.y;
	iMV[8] = pmvBackward[4].m_vctTrueHalfPel.x; iMV[9] = pmvBackward[4].m_vctTrueHalfPel.y;
	m_pbitstrmOut->trace (iMV, 10, "BakMV");
#endif
#endif // __TRACE_AND_STATS_

	pmbmd->m_bhas4MVForward  = FALSE;
	pmbmd->m_bhas4MVBackward = FALSE;
	return iSADnobias;
}

//end of new changes
//end of INTERLACED

Int CVideoObjectEncoder::blkmatch16 (
	CMotionVector* pmv, 
	CoordI iXRef, CoordI iYRef,
	CoordI iXCurr, CoordI iYCurr,
	Int iMinSAD,
	const PixelC* ppxlcRefMBY,
	const CU8Image* puciRefQZoomY,
	Int iSearchRange,
    Bool bQuarterSample
)
{
  Int mbDiff; // start with big
  Int iXDest = iXRef, iYDest = iYRef;
  CoordI x = iXRef, y = iYRef, ix, iy;
   
	Int uiPosInLoop, iLoop;
  

  // for QuarterSample
  //Left Top of Ref-Frame:
  
  const PixelC* ppxlcRefLeftTop;
  ppxlcRefLeftTop = (m_volmd.bOriginalForME ? m_pvopcRefOrig0 : m_pvopcRefQ0)->pixelsY (); 

  U8* ppxlcInterpBlk = NULL;
  U8* ppxlcInterpAct;;
    
  if (bQuarterSample) 
    ppxlcInterpBlk = (U8*) calloc(MB_SIZE*MB_SIZE,sizeof(U8)); 
  // ~for QuarterSample
  
if (m_iMVFileUsage != 1) {
  
    // Spiral Search for the rest
// RRV modification
    for (iLoop = 1; iLoop <= (iSearchRange *m_iRRVScale); iLoop++) {
//    for (iLoop = 1; iLoop <= iSearchRange; iLoop++) {
// ~RRV
      x++;
      y++;
      ppxlcRefMBY += m_iFrameWidthY + 1;
      for (uiPosInLoop = 0; uiPosInLoop < (iLoop << 3); uiPosInLoop++) { // inside each spiral loop 
        if (
            x >= m_rctRefVOPY0.left && y >= m_rctRefVOPY0.top && 
            // x <= m_rctRefVOPY0.right - MB_SIZE && y <= m_rctRefVOPY0.bottom - MB_SIZE //changed by mwi 
// RRV modification
            x < m_rctRefVOPY0.right - (MB_SIZE *m_iRRVScale) && y < m_rctRefVOPY0.bottom - (MB_SIZE *m_iRRVScale)
//            x < m_rctRefVOPY0.right - MB_SIZE && y < m_rctRefVOPY0.bottom - MB_SIZE
// ~RRV
            ) {
          const PixelC* ppxlcTmpC = m_ppxlcCurrMBY;
          const PixelC* ppxlcRefMB = ppxlcRefMBY;
          mbDiff = 0;
// RRV modification
          for (iy = 0; iy < (MB_SIZE *m_iRRVScale); iy++) {
            for (ix = 0; ix < (MB_SIZE *m_iRRVScale); ix++)
//          for (iy = 0; iy < MB_SIZE; iy++) {
//            for (ix = 0; ix < MB_SIZE; ix++)
// ~RRV
              mbDiff += abs (ppxlcTmpC [ix] - ppxlcRefMB [ix]);
            if (mbDiff >= iMinSAD)
              goto NEXT_POSITION; // skip the current position
            ppxlcRefMB += m_iFrameWidthY;
// RRV modification
            ppxlcTmpC += (MB_SIZE *m_iRRVScale);
//            ppxlcTmpC += MB_SIZE;
// ~RRV
          }
          iMinSAD = mbDiff;
          iXDest = x;
          iYDest = y;
        }
      NEXT_POSITION:	
        if (uiPosInLoop < (iLoop << 1)) {
          ppxlcRefMBY--;
          x--;
        }
        else if (uiPosInLoop < (iLoop << 2)) {
          ppxlcRefMBY -= m_iFrameWidthY;
          y--;
        }
        else if (uiPosInLoop < (iLoop * 6)) {
          ppxlcRefMBY++;					  
          x++;
        }
        else {
          ppxlcRefMBY += m_iFrameWidthY;
          y++;
        }
      }
    }
    

	*pmv = CMotionVector (iXDest - iXCurr, iYDest - iYCurr);
  } else {
    iXDest = iXRef + pmv->iMVX;
    iYDest = iYRef + pmv->iMVY;
  }
  /* NBIT: change to a bigger number
     iMinSAD = 256*256;		// Reset iMinSAD because reference VOP changed from original to reconstructed (Bob Eifrig)
     */
  iMinSAD = 4096*256;
  /* NBIT: change
     Int iFavorZero = pmv->isZero() ? -FAVORZERO : 0;
     */
// RRV modification
  Int iFavorZero;
  if(m_vopmd.RRVmode.iRRVOnOff == 1)
  {
	  iFavorZero = pmv->isZero() ? FAVORZERO_RRV : 0;
  }
  else
  {
	  iFavorZero = pmv->isZero() ? FAVORZERO : 0;
  }
//  Int iFavorZero = pmv->isZero() ? FAVORZERO : 0;
// ~RRV
  Int nBits = m_volmd.nBits;
  if (nBits > 8) {
    iFavorZero <<= (nBits-8);
  } else if (nBits < 8) {
    iFavorZero >>= (8-nBits);
  }
  iFavorZero = -iFavorZero;

  I8 destHalfX = 0, destHalfY = 0;
  Int lowX = (iXDest == m_rctRefVOPY0.left) ? 0 : -1;
  Int lowY = (iYDest == m_rctRefVOPY0.top) ? 0 : -1;
// RRV modification
  Int highX = (iXDest == m_rctRefVOPY0.right - (MB_SIZE *m_iRRVScale) + 1) ? 0 : 1;
  Int highY = (iYDest == m_rctRefVOPY0.bottom - (MB_SIZE *m_iRRVScale) + 1) ? 0 : 1;
//  Int highX = (iXDest == m_rctRefVOPY0.right - MB_SIZE + 1) ? 0 : 1;
//  Int highY = (iYDest == m_rctRefVOPY0.bottom - MB_SIZE + 1) ? 0 : 1;
// ~RRV
  const PixelC* ppxlcRefZoom = puciRefQZoomY->pixels ((iXDest << 1) + lowX, (iYDest << 1) + lowY);

  //for quarter pel search
  I8 destQuarterX = 0, destQuarterY = 0;
    
  if (!bQuarterSample) {
	// half pel search
	for (Int incY = lowY; incY <= highY; incY++) {
      for (Int incX = lowX; incX <= highX; incX++) {
        const PixelC* ppxlcRefZoomMB = ppxlcRefZoom;
        const PixelC* ppxlcTmpC = m_ppxlcCurrMBY;
        mbDiff = (incX || incY) ? 0 : iFavorZero;
// RRV modification
        for (iy = 0; iy < (MB_SIZE *m_iRRVScale); iy++) {
          for (ix = 0; ix < (MB_SIZE *m_iRRVScale); ix++)
//        for (iy = 0; iy < MB_SIZE; iy++) {
//          for (ix = 0; ix < MB_SIZE; ix++)
// ~RRV
            mbDiff += abs (ppxlcTmpC [ix] - ppxlcRefZoomMB [2 * ix]);
          if (mbDiff > iMinSAD)			// Try to find shortest MV in case of equal SADs
            goto NEXT_HALF_POSITION;
          ppxlcRefZoomMB += m_iFrameWidthZoomY * 2;
// RRV modification
          ppxlcTmpC += (MB_SIZE *m_iRRVScale);
//          ppxlcTmpC += MB_SIZE;
// ~RRV
        }
        if ((iMinSAD > mbDiff) ||
            ((abs(pmv->m_vctTrueHalfPel.x + destHalfX) + abs(pmv->m_vctTrueHalfPel.y + destHalfY)) >
             (abs(pmv->m_vctTrueHalfPel.x + incX)      + abs(pmv->m_vctTrueHalfPel.y + incY)))) {
          iMinSAD = mbDiff;
          destHalfX = incX;
          destHalfY = incY;
        }
      NEXT_HALF_POSITION:
        ppxlcRefZoom++;
      }
      ppxlcRefZoom += m_iFrameWidthZoomY - (highX - lowX + 1);
	}
	pmv -> iHalfX = destHalfX;
	pmv -> iHalfY = destHalfY;
	return iMinSAD;
  }
  else { // QUARTER SAMPLE 
    // half pel search
    Int incY;
    for (incY = lowY; incY <= highY; incY++) {
      for (Int incX = lowX; incX <= highX; incX++) {
        ppxlcInterpAct = ppxlcInterpBlk;
        const PixelC* ppxlcTmpC = m_ppxlcCurrMBY;
          
        blkInterpolateY(ppxlcRefLeftTop,MB_SIZE,4*iXDest+2*incX,4*iYDest+2*incY,ppxlcInterpAct,m_vopmd.iRoundingControl);
        mbDiff = (incX || incY) ? 0 : iFavorZero;

        for (iy = 0; iy < MB_SIZE; iy++) {
          for (ix = 0; ix < MB_SIZE; ix++)
            mbDiff += abs (ppxlcTmpC [ix] - ppxlcInterpAct[ix]);
          if (mbDiff > iMinSAD)			// Try to find shortest MV in case of equal SADs
            goto NEXT_HALF_POSITION_QS;
          ppxlcInterpAct += MB_SIZE;
          ppxlcTmpC += MB_SIZE;
        }
        if ((iMinSAD > mbDiff) ||
            ((abs(pmv->m_vctTrueHalfPel.x + destHalfX) + abs(pmv->m_vctTrueHalfPel.y + destHalfY)) >
             (abs(pmv->m_vctTrueHalfPel.x + incX)      + abs(pmv->m_vctTrueHalfPel.y + incY)))) {
          iMinSAD = mbDiff;
          destHalfX = incX;
          destHalfY = incY;
        }
      NEXT_HALF_POSITION_QS:
        ;
      }
    }
            
    pmv->iMVX = 2*pmv->iMVX + destHalfX;
    pmv->iMVY = 2*pmv->iMVY + destHalfY;

    // Half Pel Ref Position
    iXDest = 4*iXDest + 2*destHalfX;
    iYDest = 4*iYDest + 2*destHalfY;
      
    iMinSAD = 4096*256; // NBit. mwi

    // quarter pel search
    for (/*Int*/incY = lowY; incY <= highY; incY++) {
      for (Int incX = lowX; incX <= highX; incX++) {
        ppxlcInterpAct = ppxlcInterpBlk;
        const PixelC* ppxlcTmpC = m_ppxlcCurrMBY;
        blkInterpolateY(ppxlcRefLeftTop,MB_SIZE,iXDest+incX,iYDest+incY,ppxlcInterpAct,m_vopmd.iRoundingControl);

        mbDiff = (incX || incY) ? 0 : iFavorZero;

        for (iy = 0; iy < MB_SIZE; iy++) {
          for (ix = 0; ix < MB_SIZE; ix++)
            mbDiff += abs (ppxlcTmpC [ix] - ppxlcInterpAct[ix]);
          if (mbDiff > iMinSAD)			// Try to find shortest MV in case of equal SADs
            goto NEXT_QUARTER_POSITION_QS;
          ppxlcInterpAct += MB_SIZE;
          ppxlcTmpC += MB_SIZE;
        }
        if ((iMinSAD > mbDiff) ||
            ((abs(2*pmv->m_vctTrueHalfPel.x + destQuarterX) + abs(2*pmv->m_vctTrueHalfPel.y + destQuarterY)) >
             (abs(2*pmv->m_vctTrueHalfPel.x + incX)      + abs(2*pmv->m_vctTrueHalfPel.y + incY)))) {
          iMinSAD = mbDiff;
          destQuarterX = incX;
          destQuarterY = incY;
        }
      NEXT_QUARTER_POSITION_QS:
        ;
      }
    }
    pmv->iHalfX = destQuarterX;
    pmv->iHalfY = destQuarterY;
     
    free(ppxlcInterpBlk);
    return iMinSAD;
  }
}



Int CVideoObjectEncoder::blkmatch16WithShape (
                                              CMotionVector* pmv, 
                                              CoordI iXRef, CoordI iYRef,
                                              CoordI iXCurr, CoordI iYCurr,
                                              Int iMinSAD,
                                              const PixelC* ppxlcRefMBY,
                                              const CU8Image* puciRefQZoom,
                                              const CMBMode *pmbmd,
                                              Int iSearchRange,
                                              Bool bQuarterSample,
                                              Int iDirection
                                              )
{
  Int mbDiff; // start with big
  Int iXDest = iXRef, iYDest = iYRef;
  CoordI x = iXRef, y = iYRef, ix, iy;
  
	Int uiPosInLoop, iLoop;
  
  CRct rctRefVOPY = (iDirection) ? m_rctRefVOPY1:m_rctRefVOPY0;

  // for QuarterSample
  //Left Top of Ref-Frame:
  
  const PixelC* ppxlcRefLeftTop;
  //select Reference VOP
  ppxlcRefLeftTop = (iDirection ? m_pvopcRefQ1 : (m_volmd.bOriginalForME ? m_pvopcRefOrig0 : m_pvopcRefQ0))->pixelsY (); 

  U8* ppxlcInterpBlk = NULL;
  U8* ppxlcInterpAct;;
    
  if (bQuarterSample) 
    ppxlcInterpBlk = (U8*) calloc(MB_SIZE*MB_SIZE,sizeof(U8)); 
  // ~for QuarterSample

  // Spiral Search for the rest
  if (m_iMVFileUsage != 1) {
  
  for (iLoop = 1; iLoop <= iSearchRange; iLoop++) {
      x++;
      y++;
      ppxlcRefMBY += m_iFrameWidthY + 1;
      for (uiPosInLoop = 0; uiPosInLoop < (iLoop << 3); uiPosInLoop++) { // inside each spiral loop 
        if (
            x >= rctRefVOPY.left && y >= rctRefVOPY.top && 
            //x <= m_rctRefVOPY0.right - MB_SIZE && y <= m_rctRefVOPY0.bottom - MB_SIZE¤//changed by mwi
            x < rctRefVOPY.right - MB_SIZE && y < rctRefVOPY.bottom - MB_SIZE
            ) {
          const PixelC* ppxlcTmpC = m_ppxlcCurrMBY;
          const PixelC* ppxlcTmpCBY = m_ppxlcCurrMBBY;
          const PixelC* ppxlcRefMB = ppxlcRefMBY;
          mbDiff = 0;
          for (iy = 0; iy < MB_SIZE; iy++) {
            for (ix = 0; ix < MB_SIZE; ix++) {
              if (ppxlcTmpCBY [ix] != transpValue)
                mbDiff += abs (ppxlcTmpC [ix] - ppxlcRefMB [ix]);
            }
            if (mbDiff >= iMinSAD)
              goto NEXT_POSITION; // skip the current position
            ppxlcRefMB += m_iFrameWidthY;
            ppxlcTmpC += MB_SIZE;
            ppxlcTmpCBY += MB_SIZE;
          }
          iMinSAD = mbDiff;
          iXDest = x;
          iYDest = y;
        }
      NEXT_POSITION:	
        if (uiPosInLoop < (iLoop << 1)) {
          ppxlcRefMBY--;
          x--;
        }
        else if (uiPosInLoop < (iLoop << 2)) {
          ppxlcRefMBY -= m_iFrameWidthY;
          y--;
        }
        else if (uiPosInLoop < (iLoop * 6)) {
          ppxlcRefMBY++;					  
          x++;
        }
        else {
          ppxlcRefMBY += m_iFrameWidthY;
          y++;
        }
      }
    }
  
    *pmv = CMotionVector (iXDest - iXCurr, iYDest - iYCurr);
  } 
  else {
    iXDest = iXRef + pmv->iMVX;
    iYDest = iYRef + pmv->iMVY;
  }
  // check for case when zero MV due to MB outside the extended BB.
  if(iXDest==iXCurr && iYDest==iYCurr
     && (iXDest < rctRefVOPY.left || iYDest < rctRefVOPY.top
         || iXDest > rctRefVOPY.right - MB_SIZE
         || iYDest > rctRefVOPY.bottom - MB_SIZE))
	{
      pmv -> iHalfX = 0;
      pmv -> iHalfY = 0;
      return iMinSAD;  
	}

  /* NBIT: change to a bigger number
     iMinSAD = 256*256;		// Reset iMinSAD because reference VOP may change from original to reconstructed (Bob Eifrig)
     */
  iMinSAD = 4096*256;
  Int iFavorZero = pmv->isZero() ? -(pmbmd -> m_rgNumNonTranspPixels [0] >> 1) - 1 : 0;	
  // for half pel search
  I8 destHalfX = 0, destHalfY = 0;
  Int lowY, highY, lowX, highX;
  lowX = (iXDest == rctRefVOPY.left) ? 0 : -1;
  lowY = (iYDest == rctRefVOPY.top) ? 0 : -1;
  highX = (iXDest == rctRefVOPY.right - MB_SIZE + 1) ? 0 : 1;
  highY = (iYDest == rctRefVOPY.bottom - MB_SIZE + 1) ? 0 : 1;
  const PixelC* ppxlcRefZoom = puciRefQZoom->pixels ((iXDest << 1) + lowX, (iYDest << 1) + lowY);
    
  //for quarter pel search
  I8 destQuarterX = 0, destQuarterY = 0;
    
  if (!bQuarterSample) {
    // half pel search
	for (Int incY = lowY; incY <= highY; incY++) {
      for (Int incX = lowX; incX <= highX; incX++) {
        const PixelC* ppxlcRefZoomMB = ppxlcRefZoom;
        const PixelC* ppxlcTmpC = m_ppxlcCurrMBY;
        const PixelC* ppxlcTmpCBY = m_ppxlcCurrMBBY;
        mbDiff = (incX || incY) ? 0 : iFavorZero;
        for (iy = 0; iy < MB_SIZE; iy++) {
          for (ix = 0; ix < MB_SIZE; ix++) {
            if (ppxlcTmpCBY [ix] != transpValue) 
              mbDiff += abs (ppxlcTmpC [ix] - ppxlcRefZoomMB [2 * ix]);
          }
          if (mbDiff > iMinSAD)			// Try to find shortest MV in case of equal SADs
            goto NEXT_HALF_POSITION;
          ppxlcRefZoomMB += m_iFrameWidthZoomY * 2;
          ppxlcTmpC += MB_SIZE;
          ppxlcTmpCBY += MB_SIZE;
        }
        if ((iMinSAD > mbDiff) ||
            ((abs(pmv->m_vctTrueHalfPel.x + destHalfX) + abs(pmv->m_vctTrueHalfPel.y + destHalfY)) >
             (abs(pmv->m_vctTrueHalfPel.x + incX)      + abs(pmv->m_vctTrueHalfPel.y + incY)))) {
          iMinSAD = mbDiff;
          destHalfX = incX;
          destHalfY = incY;
        }
      NEXT_HALF_POSITION:
        ppxlcRefZoom++;
      }
      ppxlcRefZoom += m_iFrameWidthZoomY - (highX - lowX + 1);
	}
	pmv -> iHalfX = destHalfX;
	pmv -> iHalfY = destHalfY;
	return iMinSAD;
  }
  else { // QUARTER SAMPLE 
    // half pel search
    Int incY;
    for (incY = lowY; incY <= highY; incY++) {
      for (Int incX = lowX; incX <= highX; incX++) {
        ppxlcInterpAct = ppxlcInterpBlk;
        const PixelC* ppxlcTmpC = m_ppxlcCurrMBY;
        const PixelC* ppxlcTmpCBY = m_ppxlcCurrMBBY;

        blkInterpolateY(ppxlcRefLeftTop,MB_SIZE,4*iXDest+2*incX,4*iYDest+2*incY,ppxlcInterpAct,m_vopmd.iRoundingControl);

        mbDiff = (incX || incY) ? 0 : iFavorZero;
        for (iy = 0; iy < MB_SIZE; iy++) {
          for (ix = 0; ix < MB_SIZE; ix++) {
            if (ppxlcTmpCBY [ix] != transpValue) 
              mbDiff += abs (ppxlcTmpC [ix] - ppxlcInterpAct[ix]);
          }
          if (mbDiff > iMinSAD)			// Try to find shortest MV in case of equal SADs
            goto NEXT_HALF_POSITION_QS;
          ppxlcInterpAct += MB_SIZE;
          ppxlcTmpC += MB_SIZE;
          ppxlcTmpCBY += MB_SIZE;
        }
        if ((iMinSAD > mbDiff) ||
            ((abs(pmv->m_vctTrueHalfPel.x + destHalfX) + abs(pmv->m_vctTrueHalfPel.y + destHalfY)) >
             (abs(pmv->m_vctTrueHalfPel.x + incX)      + abs(pmv->m_vctTrueHalfPel.y + incY)))) {
          iMinSAD = mbDiff;
          destHalfX = incX;
          destHalfY = incY;
        }
      NEXT_HALF_POSITION_QS:
        ;
      }
    }

    pmv->iMVX = 2*pmv->iMVX + destHalfX;
    pmv->iMVY = 2*pmv->iMVY + destHalfY;

    // Half Pel Ref Position
    iXDest = 4*iXDest + 2*destHalfX;
    iYDest = 4*iYDest + 2*destHalfY;

    iMinSAD = 4096*256; // NBit. mwi
      

    // quarter pel search
    for (/*Int*/incY = lowY; incY <= highY; incY++) {
      for (Int incX = lowX; incX <= highX; incX++) {
        ppxlcInterpAct = ppxlcInterpBlk;
        const PixelC* ppxlcTmpC = m_ppxlcCurrMBY;
        const PixelC* ppxlcTmpCBY = m_ppxlcCurrMBBY;

        blkInterpolateY(ppxlcRefLeftTop,MB_SIZE,iXDest+incX,iYDest+incY,ppxlcInterpAct,m_vopmd.iRoundingControl);

        mbDiff = (incX || incY) ? 0 : iFavorZero;
        for (iy = 0; iy < MB_SIZE; iy++) {
          for (ix = 0; ix < MB_SIZE; ix++) {
            if (ppxlcTmpCBY [ix] != transpValue) 
              mbDiff += abs (ppxlcTmpC [ix] - ppxlcInterpAct[ix]);
          }
          if (mbDiff > iMinSAD)			// Try to find shortest MV in case of equal SADs
            goto NEXT_QUARTER_POSITION_QS;
          ppxlcInterpAct += MB_SIZE;
          ppxlcTmpC += MB_SIZE;
          ppxlcTmpCBY += MB_SIZE;
        }
        if ((iMinSAD > mbDiff) ||
            ((abs(2*pmv->m_vctTrueHalfPel.x + destQuarterX) + abs(2*pmv->m_vctTrueHalfPel.y + destQuarterY)) >
             (abs(2*pmv->m_vctTrueHalfPel.x + incX)      + abs(2*pmv->m_vctTrueHalfPel.y + incY)))) {
          iMinSAD = mbDiff;
          destQuarterX = incX;
          destQuarterY = incY;
        }
      NEXT_QUARTER_POSITION_QS:
        ;
      }
    }
    pmv->iHalfX = destQuarterX;
    pmv->iHalfY = destQuarterY;

    free(ppxlcInterpBlk);
    return iMinSAD;
  }
}



Int CVideoObjectEncoder::blockmatch8 (
	const PixelC* ppxlcCodedBlkY, 
	CMotionVector* pmv8, 
	CoordI iXBlk, CoordI iYBlk,
	const CMotionVector* pmvPred,
	Int iSearchRange,
    Bool bQuarterSample
)
{
  Int mbDiff,iMinSAD; // start with big
  CoordI ix,iy,iXDest,iYDest;
  // for QuarterSample
  //Left Top of Ref-Frame:
  
  const PixelC* ppxlcRefLeftTop;
  ppxlcRefLeftTop = (m_volmd.bOriginalForME ? m_pvopcRefOrig0 : m_pvopcRefQ0)->pixelsY (); 
  U8* ppxlcInterpBlk = NULL;
  U8* ppxlcInterpAct;;
    
  if (bQuarterSample) 
    ppxlcInterpBlk = (U8*) calloc(MB_SIZE*MB_SIZE,sizeof(U8)); 
  // ~for QuarterSample

  if (m_iMVFileUsage != 1) {
    CoordI x, y, left, right, top, bottom;
    if (!bQuarterSample) {
      x = iXBlk + pmvPred->iMVX, y = iYBlk + pmvPred->iMVY; // , ix, iy;
// RRV modification
      left = -1 * min (abs (pmvPred->iMVX + (iSearchRange *m_iRRVScale)), 
					   ADD_DISP);
      top = -1 * min (abs (pmvPred->iMVY + (iSearchRange *m_iRRVScale)), 
					  ADD_DISP);
      right = min (abs (pmvPred->iMVX - (iSearchRange *m_iRRVScale)), 
				   ADD_DISP);
      bottom = min (abs (pmvPred->iMVY - (iSearchRange *m_iRRVScale)), 
					ADD_DISP);
//      left = -1 * min (abs (pmvPred->iMVX + iSearchRange), ADD_DISP);
//      top = -1 * min (abs (pmvPred->iMVY + iSearchRange), ADD_DISP);
//      right = min (abs (pmvPred->iMVX - iSearchRange), ADD_DISP);
//      bottom = min (abs (pmvPred->iMVY - iSearchRange), ADD_DISP);
// ~RRV
    }
    else {
      x = iXBlk + pmvPred->iMVX/2, y = iYBlk + pmvPred->iMVY/2; // , ix, iy;
      left = -1 * min (abs (pmvPred->iMVX/2 + iSearchRange), ADD_DISP);
      top = -1 * min (abs (pmvPred->iMVY/2 + iSearchRange), ADD_DISP);
      right = min (abs (pmvPred->iMVX/2 - iSearchRange), ADD_DISP);
      bottom = min (abs (pmvPred->iMVY/2 - iSearchRange), ADD_DISP);
    }

    //    CoordI iOrigX = x, iOrigY = y;
    // CoordI 
    iXDest = x, iYDest = y;
    Int uiPosInLoop, iLoop;
    const PixelC* ppxlcRefBlk = (m_volmd.bOriginalForME ? m_pvopcRefOrig0 : m_pvopcRefQ0)->getPlane (Y_PLANE)->pixels (x, y);
    iMinSAD = sad8x8At0 (ppxlcCodedBlkY, ppxlcRefBlk);
    // Spiral Search for the rest
    Int iix = 0, iiy = 0;
    for (iLoop = 1; iLoop <= ADD_DISP; iLoop++) {
      x++;
      y++;
      iix++; iiy++;
      ppxlcRefBlk += m_iFrameWidthY + 1;
      for (uiPosInLoop = 0; uiPosInLoop < (iLoop << 3); uiPosInLoop++) { // inside each spiral loop 
        if (
            iix >= left && iix <= right &&
            iiy >= top && iiy <= bottom &&
            x >= m_rctRefVOPY0.left && y >= m_rctRefVOPY0.top && 
// RRV modification
            x < m_rctRefVOPY0.right - (BLOCK_SIZE *m_iRRVScale) && y < m_rctRefVOPY0.bottom - (BLOCK_SIZE *m_iRRVScale) //mod. mwi
//            x < m_rctRefVOPY0.right - BLOCK_SIZE && y < m_rctRefVOPY0.bottom - BLOCK_SIZE //mod. mwi
// ~RRV
            ) {
          const PixelC* ppxlcTmpC = ppxlcCodedBlkY;
          const PixelC* ppxlcRefMB = ppxlcRefBlk;
          mbDiff = 0;
// RRV modification
          for (iy = 0; iy < (BLOCK_SIZE *m_iRRVScale); iy++) {
            for (ix = 0; ix < (BLOCK_SIZE *m_iRRVScale); ix++)
//          for (iy = 0; iy < BLOCK_SIZE; iy++) {
//            for (ix = 0; ix < BLOCK_SIZE; ix++)
// ~RRV
              mbDiff += abs (ppxlcTmpC [ix] - ppxlcRefMB [ix]);
            if (mbDiff > iMinSAD)
              goto NEXT_POSITION; // skip the current position
            ppxlcRefMB += m_iFrameWidthY;
// RRV modification
            ppxlcTmpC += (MB_SIZE *m_iRRVScale);
//            ppxlcTmpC += MB_SIZE;
// ~RRV
          }
          iMinSAD = mbDiff;
          iXDest = x;
          iYDest = y;
        }
      NEXT_POSITION:	
        if (uiPosInLoop < (iLoop << 1)) {
          ppxlcRefBlk--;
          x--;
          iix--;
        }
        else if (uiPosInLoop < (iLoop << 2)) {
          ppxlcRefBlk -= m_iFrameWidthY;
          y--;
          iiy--;
        }
        else if (uiPosInLoop < (iLoop * 6)) {
          ppxlcRefBlk++;					  
          x++;
          iix++;
        }
        else {
          ppxlcRefBlk += m_iFrameWidthY;
          y++;
          iiy++;
        }
      }
    }
    *pmv8 = CMotionVector (iXDest - iXBlk, iYDest - iYBlk);
  } 
  else {
    iXDest = iXBlk + pmv8->iMVX;
    iYDest = iYBlk + pmv8->iMVY;
  }
  /* NBIT: change to a bigger number
     iMinSAD = 256*256;		// Reset iMinSAD because reference VOP changed from original to reconstructed (Bob Eifrig)
     */
  iMinSAD = 4096*256;
		
  // for half pel search
  I8 destHalfX = 0, destHalfY = 0;
  Int lowY, highY, lowX, highX;
// RRV modification
  lowX = (iXDest == m_rctRefVOPY0.left + (BLOCK_SIZE *m_iRRVScale)) ? 0 : -1; //changed by mwi
  lowY = (iYDest == m_rctRefVOPY0.top + (BLOCK_SIZE *m_iRRVScale)) ? 0 : -1;
  highX = (iXDest == m_rctRefVOPY0.right - (BLOCK_SIZE *m_iRRVScale) + 1) ? 0 : 1;
  highY = (iYDest == m_rctRefVOPY0.bottom - (BLOCK_SIZE *m_iRRVScale) + 1) ? 0 : 1; //~changed by mwi
//  lowX = (iXDest == m_rctRefVOPY0.left + BLOCK_SIZE) ? 0 : -1; //changed by mwi
//  lowY = (iYDest == m_rctRefVOPY0.top + BLOCK_SIZE) ? 0 : -1;
//  highX = (iXDest == m_rctRefVOPY0.right - BLOCK_SIZE + 1) ? 0 : 1;
//  highY = (iYDest == m_rctRefVOPY0.bottom - BLOCK_SIZE + 1) ? 0 : 1; //~changed by mwi
// ~RRV
  const PixelC* ppxlcRefZoom = m_puciRefQZoom0->pixels ((iXDest << 1) + lowX, (iYDest << 1) + lowY);

  //for quarter pel search
  I8 destQuarterX = 0, destQuarterY = 0;
    
  if (!bQuarterSample) {
	// half pel search
	for (Int incY = lowY; incY <= highY; incY++) {
      for (Int incX = lowX; incX <= highX; incX++) {
        // no longer skipp (0,0) due to using original as interger pel reference
        //if (!(incX == 0 && incY == 0)) {
        const PixelC* ppxlcRefZoomBlk = ppxlcRefZoom;
        const PixelC* ppxlcTmpC = ppxlcCodedBlkY;
        mbDiff = 0;
// RRV modification
        for (iy = 0; iy < (BLOCK_SIZE *m_iRRVScale); iy++) {
          for (ix = 0; ix < (BLOCK_SIZE *m_iRRVScale); ix++)
//        for (iy = 0; iy < BLOCK_SIZE; iy++) {
//          for (ix = 0; ix < BLOCK_SIZE; ix++)
// ~RRV
            mbDiff += abs (ppxlcTmpC [ix] - ppxlcRefZoomBlk [2 * ix]);
          if (mbDiff > iMinSAD)
            goto NEXT_HALF_POSITION;
          ppxlcRefZoomBlk += m_iFrameWidthZoomY * 2;
// RRV modification
          ppxlcTmpC += (MB_SIZE *m_iRRVScale);
//          ppxlcTmpC += MB_SIZE;
// ~RRV
        }
        if ((iMinSAD > mbDiff) ||
            ((abs(pmv8->m_vctTrueHalfPel.x + destHalfX) + abs(pmv8->m_vctTrueHalfPel.y + destHalfY)) >
             (abs(pmv8->m_vctTrueHalfPel.x + incX)      + abs(pmv8->m_vctTrueHalfPel.y + incY)))) {
          iMinSAD = mbDiff;
          destHalfX = incX;
          destHalfY = incY;
        }
      NEXT_HALF_POSITION:
        ppxlcRefZoom++;
      }
      ppxlcRefZoom += m_iFrameWidthZoomY - (highX - lowX + 1);
	}
	pmv8 -> iHalfX = destHalfX;
	pmv8 -> iHalfY = destHalfY;
	return iMinSAD;
  }
  else { // QUARTER SAMPLE
    // half pel search
    Int incY;
    for (incY = lowY; incY <= highY; incY++) {
      for (Int incX = lowX; incX <= highX; incX++) {
        const PixelC* ppxlcTmpC = ppxlcCodedBlkY;
        ppxlcInterpAct = ppxlcInterpBlk;
        blkInterpolateY(ppxlcRefLeftTop,MB_SIZE,4*iXDest+2*incX,4*iYDest+2*incY,ppxlcInterpAct,m_vopmd.iRoundingControl);
        mbDiff = 0;

        for (iy = 0; iy < BLOCK_SIZE; iy++) {
          for (ix = 0; ix < BLOCK_SIZE; ix++)
            mbDiff += abs (ppxlcTmpC [ix] - ppxlcInterpAct[ix]);
          if (mbDiff >= iMinSAD)
            goto NEXT_HALF_POSITION_QS;
          ppxlcInterpAct += MB_SIZE;
          ppxlcTmpC += MB_SIZE;
        }
        if ((iMinSAD > mbDiff) ||
            ((abs(pmv8->m_vctTrueHalfPel.x + destHalfX) + abs(pmv8->m_vctTrueHalfPel.y + destHalfY)) >
             (abs(pmv8->m_vctTrueHalfPel.x + incX)      + abs(pmv8->m_vctTrueHalfPel.y + incY)))) {
          iMinSAD = mbDiff;
          destHalfX = incX;
          destHalfY = incY;
        }
      NEXT_HALF_POSITION_QS:
        ;
      }
    }

    pmv8->iMVX = 2*pmv8->iMVX + destHalfX;
    pmv8->iMVY = 2*pmv8->iMVY + destHalfY;

    // Half Pel Ref Position
    iXDest = 4*iXDest + 2*destHalfX;
    iYDest = 4*iYDest + 2*destHalfY;

    iMinSAD = 4096*256; // NBit. mwi

    // quarter pel search
    for (/*Int*/incY = lowY; incY <= highY; incY++) {
      for (Int incX = lowX; incX <= highX; incX++) {
        // no longer skipp (0,0) due to using original as interger pel reference
        //if (!(incX == 0 && incY == 0)) {
        //const PixelC* ppxlcRefZoomBlk = ppxlcRefZoom;
        const PixelC* ppxlcTmpC = ppxlcCodedBlkY;
        ppxlcInterpAct = ppxlcInterpBlk;
        blkInterpolateY(ppxlcRefLeftTop,MB_SIZE,iXDest+incX,iYDest+incY,ppxlcInterpAct,m_vopmd.iRoundingControl);
        mbDiff = 0;

        for (iy = 0; iy < BLOCK_SIZE; iy++) {
          for (ix = 0; ix < BLOCK_SIZE; ix++)
            mbDiff += abs (ppxlcTmpC [ix] - ppxlcInterpAct[ix]);
          if (mbDiff >= iMinSAD)
            goto NEXT_QUARTER_POSITION_QS;
          ppxlcInterpAct += MB_SIZE;
          ppxlcTmpC += MB_SIZE;
        }
        if ((iMinSAD > mbDiff) ||
            ((abs(2*pmv8->m_vctTrueHalfPel.x + destQuarterX) + abs(2*pmv8->m_vctTrueHalfPel.y + destQuarterY)) >
             (abs(2*pmv8->m_vctTrueHalfPel.x + incX)      + abs(2*pmv8->m_vctTrueHalfPel.y + incY)))) {
          iMinSAD = mbDiff;
          destQuarterX = incX;
          destQuarterY = incY;
        }
      NEXT_QUARTER_POSITION_QS:
        ;
      }
    }
      
    pmv8->iHalfX = destQuarterX;
    pmv8->iHalfY = destQuarterY;

    free(ppxlcInterpBlk);
    return iMinSAD;
  }
}

Int CVideoObjectEncoder::blockmatch8WithShape (
	const PixelC* ppxlcCodedBlkY, 
	const PixelC* ppxlcCodedBlkBY, 
	CMotionVector* pmv8, 
	CoordI iXBlk, CoordI iYBlk,
	const CMotionVector* pmvPred,
	Int iSearchRange,
    Bool bQuarterSample,
	Int iDirection

    )
{
  Int mbDiff; // start with big
  CoordI ix, iy, x, y, left, right, top, bottom;
  if (!bQuarterSample) {
    x = iXBlk + pmvPred->iMVX, y = iYBlk + pmvPred->iMVY; 
    left = -1 * min (abs (pmvPred->iMVX + iSearchRange), ADD_DISP);
    top = -1 * min (abs (pmvPred->iMVY + iSearchRange), ADD_DISP);
    right = min (abs (pmvPred->iMVX - iSearchRange), ADD_DISP);
    bottom = min (abs (pmvPred->iMVY - iSearchRange), ADD_DISP);
  }
  else {
    x = iXBlk + pmvPred->iMVX/2, y = iYBlk + pmvPred->iMVY/2;
    left = -1 * min (abs (pmvPred->iMVX/2 + iSearchRange), ADD_DISP);
    top = -1 * min (abs (pmvPred->iMVY/2 + iSearchRange), ADD_DISP);
    right = min (abs (pmvPred->iMVX/2 - iSearchRange), ADD_DISP);
    bottom = min (abs (pmvPred->iMVY/2 - iSearchRange), ADD_DISP);
  }

  //  CoordI iOrigX = x, iOrigY = y;
  CoordI iXDest = x, iYDest = y;
  Int uiPosInLoop, iLoop, iMinSAD = 0;
  
  const PixelC* ppxlcRefBlk;
  ppxlcRefBlk = (iDirection ? m_pvopcRefQ1 : (m_volmd.bOriginalForME ? m_pvopcRefOrig0 : m_pvopcRefQ0))->getPlane (Y_PLANE)->pixels (x, y);

  CRct rctRefVOPY = (iDirection) ? m_rctRefVOPY1:m_rctRefVOPY0;

  // for QuarterSample
  U8* ppxlcInterpBlk = NULL;
  U8* ppxlcInterpAct;
  if (bQuarterSample) 
    ppxlcInterpBlk = (U8*) calloc(MB_SIZE*MB_SIZE,sizeof(U8)); 
  // ~for QuarterSample

  if (m_iMVFileUsage != 1) {
    iMinSAD = sad8x8At0WithShape (ppxlcCodedBlkY, ppxlcCodedBlkBY, ppxlcRefBlk);
    // Spiral Search for the rest
    Int iix = 0, iiy = 0;	
    for (iLoop = 1; iLoop <= ADD_DISP; iLoop++) {
      x++;
      y++;
      iix++;
      iiy++;
      ppxlcRefBlk += m_iFrameWidthY + 1;
      for (uiPosInLoop = 0; uiPosInLoop < (iLoop << 3); uiPosInLoop++) { // inside each spiral loop 
        if (
            iix >= left && iix <= right &&
            iiy >= top && iiy <= bottom &&
            x >= rctRefVOPY.left && y >= rctRefVOPY.top && 
            //x <= m_rctRefVOPY0.right - MB_SIZE && y <= m_rctRefVOPY0.bottom - MB_SIZE
            x < rctRefVOPY.right - BLOCK_SIZE && y < rctRefVOPY.bottom - BLOCK_SIZE
            ) {
          const PixelC* ppxlcTmpC = ppxlcCodedBlkY;
          const PixelC* ppxlcTmpCBY = ppxlcCodedBlkBY;
          const PixelC* ppxlcRefMB = ppxlcRefBlk;
          mbDiff = 0;
          for (iy = 0; iy < BLOCK_SIZE; iy++) {
            for (ix = 0; ix < BLOCK_SIZE; ix++) {
              if (ppxlcTmpCBY [ix] != transpValue)
                mbDiff += abs (ppxlcTmpC [ix] - ppxlcRefMB [ix]);
            }
            if (mbDiff > iMinSAD)
              goto NEXT_POSITION; // skip the current position
            ppxlcRefMB += m_iFrameWidthY;
            ppxlcTmpC += MB_SIZE;
            ppxlcTmpCBY += MB_SIZE;
          }
          iMinSAD = mbDiff;
          iXDest = x;
          iYDest = y;
        }
      NEXT_POSITION:	
        if (uiPosInLoop < (iLoop << 1)) {
          ppxlcRefBlk--;
          x--;
          iix--;
        }
        else if (uiPosInLoop < (iLoop << 2)) {
          ppxlcRefBlk -= m_iFrameWidthY;
          y--;
          iiy--;
        }
        else if (uiPosInLoop < (iLoop * 6)) {
          ppxlcRefBlk++;					  
          x++;
          iix++;
        }
        else {
          ppxlcRefBlk += m_iFrameWidthY;
          y++;
          iiy++;
        }
      }
    }
    *pmv8 = CMotionVector (iXDest - iXBlk, iYDest - iYBlk);
  } 
  else {
    iXDest = iXBlk + pmv8->iMVX;
    iYDest = iYBlk + pmv8->iMVY;
  }
	
  // check for case when zero MV due to MB outside the extended BB.
  if(iXDest==iXBlk && iYDest==iYBlk
     && (iXDest < rctRefVOPY.left || iYDest < rctRefVOPY.top
         || iXDest > rctRefVOPY.right - MB_SIZE
         || iYDest > rctRefVOPY.bottom - MB_SIZE))

	{
      pmv8 -> iHalfX = 0;
      pmv8 -> iHalfY = 0;
      return iMinSAD;  
	}

  /* NBIT: change to a bigger number
     iMinSAD = 256*256;		// Reset iMinSAD because reference VOP changed from original to reconstructed (Bob Eifrig)
     */
  iMinSAD = 4096*256;
		
  // for half pel search
  I8 destHalfX = 0, destHalfY = 0;
  Int lowY, highY, lowX, highX;
  //lowX = (iXDest == m_rctRefVOPY0.left) ? 0 : -1;
  //lowY = (iYDest == m_rctRefVOPY0.top) ? 0 : -1;
  //highX = (iXDest == m_rctRefVOPY0.right - MB_SIZE + 1) ? 0 : 1;
  //highY = (iYDest == m_rctRefVOPY0.bottom - MB_SIZE + 1) ? 0 : 1;
  lowX = (iXDest == rctRefVOPY.left + BLOCK_SIZE) ? 0 : -1; //changed by mwi
  lowY = (iYDest == rctRefVOPY.top + BLOCK_SIZE) ? 0 : -1;
  highX = (iXDest == rctRefVOPY.right - BLOCK_SIZE + 1) ? 0 : 1;
  highY = (iYDest == rctRefVOPY.bottom - BLOCK_SIZE + 1) ? 0 : 1; //~changed by mwi
  const PixelC* ppxlcRefZoom = m_puciRefQZoom0->pixels ((iXDest << 1) + lowX, (iYDest << 1) + lowY);
  //for quarter pel search
  I8 destQuarterX = 0, destQuarterY = 0;
    
  if (!bQuarterSample) {
    // half pel search
    for (Int incY = lowY; incY <= highY; incY++) {
      const PixelC* ppxlcRefZoomRow = ppxlcRefZoom;
      for (Int incX = lowX; incX <= highX; incX++) {
        // no longer skipp (0,0) due to using original as interger pel reference
        //if (!(incX == 0 && incY == 0)) {
        const PixelC* ppxlcRefZoomBlk = ppxlcRefZoomRow;
        const PixelC* ppxlcTmpC = ppxlcCodedBlkY;
        const PixelC* ppxlcTmpCBY = ppxlcCodedBlkBY;
        mbDiff = 0;
        for (iy = 0; iy < BLOCK_SIZE; iy++) {
          for (ix = 0; ix < BLOCK_SIZE; ix++) {
            if (ppxlcTmpCBY [ix] != transpValue)
              mbDiff += abs (ppxlcTmpC [ix] - ppxlcRefZoomBlk [2 * ix]);
          }
          if (mbDiff > iMinSAD)			// Try to find shortest MV in case of equal SADs
            goto NEXT_HALF_POSITION;
          ppxlcRefZoomBlk += m_iFrameWidthZoomY * 2;
          ppxlcTmpC += MB_SIZE;
          ppxlcTmpCBY += MB_SIZE;
        }
        if ((iMinSAD > mbDiff) ||
            ((abs(pmv8->m_vctTrueHalfPel.x + destHalfX) + abs(pmv8->m_vctTrueHalfPel.y + destHalfY)) >
             (abs(pmv8->m_vctTrueHalfPel.x + incX)      + abs(pmv8->m_vctTrueHalfPel.y + incY)))) {
          iMinSAD = mbDiff;
          destHalfX = incX;
          destHalfY = incY;
        }
      NEXT_HALF_POSITION:
        ppxlcRefZoomRow++;
      }
      ppxlcRefZoom += m_iFrameWidthZoomY;
    }
    pmv8 -> iHalfX = destHalfX;
    pmv8 -> iHalfY = destHalfY;
    return iMinSAD;
  }
  else {// QUARTER SAMPLE 
    // half pel search
    Int incY;
    for (incY = lowY; incY <= highY; incY++) {
      for (Int incX = lowX; incX <= highX; incX++) {
        // no longer skipp (0,0) due to using original as interger pel reference
        //if (!(incX == 0 && incY == 0)) {
        //const PixelC* ppxlcRefZoomBlk = ppxlcRefZoomRow;
        const PixelC* ppxlcTmpC = ppxlcCodedBlkY;
        const PixelC* ppxlcTmpCBY = ppxlcCodedBlkBY;
        ppxlcInterpAct = ppxlcInterpBlk;
        blkInterpolateY(ppxlcRefBlk,MB_SIZE,4*iXDest+2*incX,4*iYDest+2*incY,ppxlcInterpAct,m_vopmd.iRoundingControl);
        mbDiff = 0;
        for (iy = 0; iy < BLOCK_SIZE; iy++) {
          for (ix = 0; ix < BLOCK_SIZE; ix++) {
            if (ppxlcTmpCBY [ix] != transpValue)
              mbDiff += abs (ppxlcTmpC [ix] - ppxlcInterpAct[ix]);
          }
          if (mbDiff > iMinSAD)			// Try to find shortest MV in case of equal SADs
            goto NEXT_HALF_POSITION_QS;
          ppxlcInterpAct += MB_SIZE;
          ppxlcTmpC += MB_SIZE;
          ppxlcTmpCBY += MB_SIZE;
        }
        if ((iMinSAD > mbDiff) ||
            ((abs(pmv8->m_vctTrueHalfPel.x + destHalfX) + abs(pmv8->m_vctTrueHalfPel.y + destHalfY)) >
             (abs(pmv8->m_vctTrueHalfPel.x + incX)      + abs(pmv8->m_vctTrueHalfPel.y + incY)))) {
          iMinSAD = mbDiff;
          destHalfX = incX;
          destHalfY = incY;
        }
      NEXT_HALF_POSITION_QS:
        ;
      }
    }
      
    pmv8->iMVX = 2*pmv8->iMVX + destHalfX;
    pmv8->iMVY = 2*pmv8->iMVY + destHalfY;

    // Half Pel Ref Position
    iXDest = 4*iXDest + 2*destHalfX;
    iYDest = 4*iYDest + 2*destHalfY;

    iMinSAD = 4096*256; // NBit. mwi

    // quarter pel search
    for (/*Int*/incY = lowY; incY <= highY; incY++) {
      for (Int incX = lowX; incX <= highX; incX++) {
        // no longer skipp (0,0) due to using original as interger pel reference
        //if (!(incX == 0 && incY == 0)) {
        //const PixelC* ppxlcRefZoomBlk = ppxlcRefZoomRow;
        const PixelC* ppxlcTmpC = ppxlcCodedBlkY;
        const PixelC* ppxlcTmpCBY = ppxlcCodedBlkBY;
        ppxlcInterpAct = ppxlcInterpBlk;
        blkInterpolateY(ppxlcRefBlk,MB_SIZE,iXDest+incX,iYDest+incY,ppxlcInterpAct,m_vopmd.iRoundingControl);
        mbDiff = 0;
        for (iy = 0; iy < BLOCK_SIZE; iy++) {
          for (ix = 0; ix < BLOCK_SIZE; ix++) {
            if (ppxlcTmpCBY [ix] != transpValue)
              mbDiff += abs (ppxlcTmpC [ix] - ppxlcInterpAct[ix]);
          }
          if (mbDiff > iMinSAD)			// Try to find shortest MV in case of equal SADs
            goto NEXT_QUARTER_POSITION_QS;
          ppxlcInterpAct += MB_SIZE;
          ppxlcTmpC += MB_SIZE;
          ppxlcTmpCBY += MB_SIZE;
        }
        if ((iMinSAD > mbDiff) ||
            ((abs(2*pmv8->m_vctTrueHalfPel.x + destQuarterX) + abs(2*pmv8->m_vctTrueHalfPel.y + destQuarterY)) >
             (abs(2*pmv8->m_vctTrueHalfPel.x + incX)      + abs(2*pmv8->m_vctTrueHalfPel.y + incY)))) {
          iMinSAD = mbDiff;
          destQuarterX = incX;
          destQuarterY = incY;
        }
      NEXT_QUARTER_POSITION_QS:
        ;
      }
    }      
    pmv8 -> iHalfX = destQuarterX;
    pmv8 -> iHalfY = destQuarterY;

    free(ppxlcInterpBlk);
    return iMinSAD;
  }
}

Int CVideoObjectEncoder::sumDev () const // compute sum of deviation of an MB
{
	const PixelC* ppxlcY = m_ppxlcCurrMBY;
	Int iMean = 0;
// RRV insertion
    Int	iTmp	= (m_iRRVScale *m_iRRVScale);
// ~RRV
	Int iy;
// RRV modification
	for (iy = 0; iy < (MB_SQUARE_SIZE *iTmp); iy++)
		iMean += ppxlcY [iy];
	iMean /= (MB_SQUARE_SIZE *iTmp);
	Int iDevRet = 0;
	for (iy = 0; iy < (MB_SQUARE_SIZE *iTmp); iy++)
		iDevRet += abs (m_ppxlcCurrMBY [iy] - iMean);
//	for (iy = 0; iy < MB_SQUARE_SIZE; iy++)
//		iMean += ppxlcY [iy];
//	iMean /= MB_SQUARE_SIZE;
//	Int iDevRet = 0;
//	for (iy = 0; iy < MB_SQUARE_SIZE; iy++)
//		iDevRet += abs (m_ppxlcCurrMBY [iy] - iMean);
// ~RRV
	return iDevRet;
}


Int CVideoObjectEncoder::sumDevWithShape (UInt uiNumTranspPels) const // compute sum of deviation of an MB
{
	const PixelC* ppxlcY = m_ppxlcCurrMBY;
	const PixelC* ppxlcBY = m_ppxlcCurrMBBY;
	Int iMean = 0;
	CoordI iy;
	for (iy = 0; iy < MB_SQUARE_SIZE; iy++) {
		if (ppxlcBY [iy] != transpValue)
			iMean += ppxlcY [iy];
	}
	iMean /= uiNumTranspPels;
	Int iDevRet = 0;
	for (iy = 0; iy < MB_SQUARE_SIZE; iy++) {
		if (m_ppxlcCurrMBBY [iy] != transpValue)
			iDevRet += abs (m_ppxlcCurrMBY [iy] - iMean);
	}
	return iDevRet;
}

Int CVideoObjectEncoder::sad16x16At0 (const PixelC* ppxlcRefY) const
{
	if (m_iMVFileUsage == 1)
		return 0;
	Int iInitSAD = 0;
	UInt nBits = m_volmd.nBits; // NBIT
// RRV modification
	Int	iFavorZero;
	if(m_vopmd.RRVmode.iRRVOnOff == 1)
	{
		iFavorZero = FAVORZERO_RRV;	// NBIT
	}
	else
	{
		iFavorZero = FAVORZERO;	// NBIT
	}
//	Int iFavorZero = FAVORZERO; // NBIT
// ~RRV
	// NBIT: addjust iFavorZero based on nBits
	if (nBits > 8) {
		iFavorZero <<= (nBits-8);
	} else if (nBits < 8) {
		iFavorZero >>= (8-nBits);
	}

	CoordI ix, iy;
	const PixelC* ppxlcCurrY = m_ppxlcCurrMBY;
// RRV modification
	for (iy = 0; iy < (MB_SIZE *m_iRRVScale); iy++) {
		for (ix = 0; ix < (MB_SIZE *m_iRRVScale); ix++)
			iInitSAD += abs (ppxlcCurrY [ix] - ppxlcRefY [ix]);
		ppxlcCurrY += (MB_SIZE *m_iRRVScale);
		ppxlcRefY += m_iFrameWidthY;
	}
//	for (iy = 0; iy < MB_SIZE; iy++) {
//		for (ix = 0; ix < MB_SIZE; ix++)
//			iInitSAD += abs (ppxlcCurrY [ix] - ppxlcRefY [ix]);
//		ppxlcCurrY += MB_SIZE;
//		ppxlcRefY += m_iFrameWidthY;
//	}
// ~RRV
/* NBIT: change
	iInitSAD -= FAVORZERO;
*/
	iInitSAD -= iFavorZero;
	return iInitSAD;
}

Int CVideoObjectEncoder::sad16x16At0WithShape (const PixelC* ppxlcRefY, const CMBMode* pmbmd) const
{
	if (m_iMVFileUsage == 1)
		return 0;
	Int iInitSAD = 0;
	UInt nBits = m_volmd.nBits; // NBIT
	Int iFavorZero = (pmbmd->m_rgNumNonTranspPixels [0] >> 1) + 1; // NBIT
	// NBIT: addjust iFavorZero based on nBits 
        if (nBits > 8) { 
                iFavorZero <<= (nBits-8); 
        } else if (nBits < 8) { 
                iFavorZero >>= (8-nBits); 
        } 

	CoordI ix, iy;
	const PixelC* ppxlcCurrY = m_ppxlcCurrMBY;
	const PixelC* ppxlcCurrBY = m_ppxlcCurrMBBY;
	for (iy = 0; iy < MB_SIZE; iy++) {
		for (ix = 0; ix < MB_SIZE; ix++) {
			if (ppxlcCurrBY [ix] != transpValue)
				iInitSAD += abs (ppxlcCurrY [ix] - ppxlcRefY [ix]);
		}
		ppxlcCurrBY += MB_SIZE;
		ppxlcCurrY += MB_SIZE;
		ppxlcRefY += m_iFrameWidthY;
	}
/* NBIT: change
	iInitSAD -= (pmbmd->m_rgNumNonTranspPixels [0] >> 1) + 1;
*/
	iInitSAD -= iFavorZero;
	return iInitSAD;
}

//OBSS_SAIT_991015
Int CVideoObjectEncoder::sad16x16At0WithShape (const PixelC* ppxlcRefY, const CMBMode* pmbmd, Int *numForePixel) const
{
	if (m_iMVFileUsage == 1)
		return 0;
	Int iInitSAD = 0;
	UInt nBits = m_volmd.nBits; // NBIT
	Int iFavorZero = (pmbmd->m_rgNumNonTranspPixels [0] >> 1) + 1; // NBIT
	// NBIT: addjust iFavorZero based on nBits 
        if (nBits > 8) { 
                iFavorZero <<= (nBits-8); 
        } else if (nBits < 8) { 
                iFavorZero >>= (8-nBits); 
        } 

	CoordI ix, iy;
	const PixelC* ppxlcCurrY = m_ppxlcCurrMBY;
	const PixelC* ppxlcCurrBY = m_ppxlcCurrMBBY;
	for (iy = 0; iy < MB_SIZE; iy++) {
		for (ix = 0; ix < MB_SIZE; ix++) {
			if (ppxlcCurrBY [ix] != transpValue){
				iInitSAD += abs (ppxlcCurrY [ix] - ppxlcRefY [ix]);
				(*numForePixel)++;
			}
		}
		ppxlcCurrBY += MB_SIZE;
		ppxlcCurrY += MB_SIZE;
		ppxlcRefY += m_iFrameWidthY;
	}
/* NBIT: change
	iInitSAD -= (pmbmd->m_rgNumNonTranspPixels [0] >> 1) + 1;
*/
	iInitSAD -= iFavorZero;
	return iInitSAD;
}
//~OBSS_SAIT_991015

Int CVideoObjectEncoder::sad8x8At0 (const PixelC* ppxlcCurrY, const PixelC* ppxlcRefY) const
{
	Int iInitSAD = 0;
	CoordI ix, iy;
// RRV modification
	for (iy = 0; iy < (BLOCK_SIZE *m_iRRVScale); iy++) {
		for (ix = 0; ix < (BLOCK_SIZE *m_iRRVScale); ix++)
			iInitSAD += abs (ppxlcCurrY [ix] - ppxlcRefY [ix]);
		ppxlcCurrY += (MB_SIZE *m_iRRVScale);
		ppxlcRefY += m_iFrameWidthY;
	}
//	for (iy = 0; iy < BLOCK_SIZE; iy++) {
//		for (ix = 0; ix < BLOCK_SIZE; ix++)
//			iInitSAD += abs (ppxlcCurrY [ix] - ppxlcRefY [ix]);
//		ppxlcCurrY += MB_SIZE;
//		ppxlcRefY += m_iFrameWidthY;
//	}
// ~RRV
	return iInitSAD;
}

Int CVideoObjectEncoder::sad8x8At0WithShape (const PixelC* ppxlcCurrY, const PixelC* ppxlcCurrBY, const PixelC* ppxlcRefY) const
{
	Int iInitSAD = 0;
	CoordI ix, iy;
	for (iy = 0; iy < BLOCK_SIZE; iy++) {
		for (ix = 0; ix < BLOCK_SIZE; ix++) {
			if (ppxlcCurrBY [ix] != transpValue)
				iInitSAD += abs (ppxlcCurrY [ix] - ppxlcRefY [ix]);
		}
		ppxlcCurrBY += MB_SIZE;
		ppxlcCurrY += MB_SIZE;
		ppxlcRefY += m_iFrameWidthY;
	}
	return iInitSAD;
}

//Interlaced
Int CVideoObjectEncoder::directSAD(
	CoordI x, CoordI y,
	CMBMode *pmbmd,
	const CMBMode *pmbmdRef,
	const CMotionVector *pmvRef
)
{
	static CRct rctBounds[4];
	static CRct rctBoundsBackward[4]; // added by mwi
	static Int iTempRefD, iTempRefB, iZeroOffset, iLastNumber = -1;
	PixelC fwd[MB_SQUARE_SIZE], bak[MB_SQUARE_SIZE];
	static Int iXOffset[] = { 0, BLOCK_SIZE, 0, BLOCK_SIZE };
	static Int iYOffset[] = { 0, 0, BLOCK_SIZE, BLOCK_SIZE };
	static Int iBlkOffset[] = { 0, BLOCK_SIZE, BLOCK_SIZE*MB_SIZE, BLOCK_SIZE*(MB_SIZE + 1) };
	CMotionVector mvFwd[4], mvBak[4];
	// Int directSAD = 256*256; // change for nBit, mwi
	Int directSAD = 4096*256;

	if (iLastNumber != m_t) {
		iTempRefD = m_tFutureRef - m_tPastRef;
		iTempRefB = m_t          - m_tPastRef;
		//assert(m_rctRefVOPY0 == m_rctRefVOPY1);
		rctBounds[0] = m_rctRefVOPY0.upSampleBy2();
		rctBounds[1] = m_rctRefVOPY0.upSampleBy2();
		rctBounds[2] = m_rctRefVOPY0.upSampleBy2();
		rctBounds[3] = m_rctRefVOPY0.upSampleBy2();
		 // added by mwi
		rctBoundsBackward[0] = m_rctRefVOPY1.upSampleBy2();
		rctBoundsBackward[1] = m_rctRefVOPY1.upSampleBy2();
		rctBoundsBackward[2] = m_rctRefVOPY1.upSampleBy2();
		rctBoundsBackward[3] = m_rctRefVOPY1.upSampleBy2();
		 // ~added by mwi
		
		if (m_volmd.bQuarterSample) { // added by mwi
		  rctBounds[0] = rctBounds[0].upSampleBy2();
		  rctBounds[1] = rctBounds[1].upSampleBy2();
		  rctBounds[2] = rctBounds[2].upSampleBy2();
		  rctBounds[3] = rctBounds[3].upSampleBy2();
		  rctBounds[0].expand(           0,            0, - 4*BLOCK_SIZE, - 4*BLOCK_SIZE);
		  rctBounds[1].expand(4*BLOCK_SIZE,            0, - 8*BLOCK_SIZE, - 4*BLOCK_SIZE);
		  rctBounds[2].expand(           0, 4*BLOCK_SIZE, - 4*BLOCK_SIZE, - 8*BLOCK_SIZE);
		  rctBounds[3].expand(4*BLOCK_SIZE, 4*BLOCK_SIZE, - 8*BLOCK_SIZE, - 8*BLOCK_SIZE);
		  //added by mwi
		  rctBoundsBackward[0] = rctBoundsBackward[0].upSampleBy2();
		  rctBoundsBackward[1] = rctBoundsBackward[1].upSampleBy2();
		  rctBoundsBackward[2] = rctBoundsBackward[2].upSampleBy2();
		  rctBoundsBackward[3] = rctBoundsBackward[3].upSampleBy2();
		  rctBoundsBackward[0].expand(           0,            0, - 4*BLOCK_SIZE, - 4*BLOCK_SIZE);
		  rctBoundsBackward[1].expand(4*BLOCK_SIZE,            0, - 8*BLOCK_SIZE, - 4*BLOCK_SIZE);
		  rctBoundsBackward[2].expand(           0, 4*BLOCK_SIZE, - 4*BLOCK_SIZE, - 8*BLOCK_SIZE);
		  rctBoundsBackward[3].expand(4*BLOCK_SIZE, 4*BLOCK_SIZE, - 8*BLOCK_SIZE, - 8*BLOCK_SIZE);
		  //~added by mwi
		}
		else {
		  rctBounds[0].expand(           0,            0, - 2*BLOCK_SIZE, - 2*BLOCK_SIZE);
		  rctBounds[1].expand(2*BLOCK_SIZE,            0, - 4*BLOCK_SIZE, - 2*BLOCK_SIZE);
		  rctBounds[2].expand(           0, 2*BLOCK_SIZE, - 2*BLOCK_SIZE, - 4*BLOCK_SIZE);
		  rctBounds[3].expand(2*BLOCK_SIZE, 2*BLOCK_SIZE, - 4*BLOCK_SIZE, - 4*BLOCK_SIZE);
		  //added by mwi
		  rctBoundsBackward[0].expand(           0,            0, - 2*BLOCK_SIZE, - 2*BLOCK_SIZE);
		  rctBoundsBackward[1].expand(2*BLOCK_SIZE,            0, - 4*BLOCK_SIZE, - 2*BLOCK_SIZE);
		  rctBoundsBackward[2].expand(           0, 2*BLOCK_SIZE, - 2*BLOCK_SIZE, - 4*BLOCK_SIZE);
		  rctBoundsBackward[3].expand(2*BLOCK_SIZE, 2*BLOCK_SIZE, - 4*BLOCK_SIZE, - 4*BLOCK_SIZE);
		  //~added by mwi
		}
		iZeroOffset = m_puciRefQZoom0->where().offset(0,0);
		iLastNumber = m_t;
	}
	Int iRadius = m_vopmd.iDirectModeRadius;
	CVector vctDMV, vctCurrZoomMB(x << 1, y << 1);
	if (m_volmd.bQuarterSample)  // added by mwi
	  vctCurrZoomMB = vctCurrZoomMB*2; // vctCurrZoomMB is not static

	if ((pmbmdRef->m_dctMd == INTRA) || (pmbmdRef->m_dctMd == INTRAQ)) {
		static CMotionVector mvZero[5];
		pmvRef = mvZero;
	}
	for (vctDMV.y = -iRadius; vctDMV.y <= iRadius; vctDMV.y++) {
	  for (vctDMV.x = -iRadius; vctDMV.x <= iRadius; vctDMV.x++) {
	    Int iBlk;
	    for (iBlk = 0; iBlk < 4; iBlk++) {
	      mvFwd[iBlk] = (pmvRef[iBlk + 1].m_vctTrueHalfPel * iTempRefB) / iTempRefD + vctDMV;
	      if (!rctBounds[iBlk].includes(vctCurrZoomMB + mvFwd[iBlk].m_vctTrueHalfPel))
		break;		            	// out-of-range
	      mvBak[iBlk] = CVector(vctDMV.x ?
				    (mvFwd[iBlk].m_vctTrueHalfPel.x - pmvRef[iBlk + 1].m_vctTrueHalfPel.x) :
				    ((pmvRef[iBlk + 1].m_vctTrueHalfPel.x * (iTempRefB - iTempRefD)) / iTempRefD),
				    vctDMV.y ? (mvFwd[iBlk].m_vctTrueHalfPel.y - pmvRef[iBlk + 1].m_vctTrueHalfPel.y) :
				    ((pmvRef[iBlk + 1].m_vctTrueHalfPel.y * (iTempRefB - iTempRefD)) / iTempRefD));
	      //if (!rctBounds[iBlk].includes(vctCurrZoomMB + mvBak[iBlk].m_vctTrueHalfPel))
	      if (!rctBoundsBackward[iBlk].includes(vctCurrZoomMB + mvBak[iBlk].m_vctTrueHalfPel))
		break;						// out-of-range
	    }
	    if (iBlk < 4)		// All mvFwd[] and mvBak[] vectors must be inbounds to continue
	      continue;

	    for (iBlk = 0; iBlk < 4; iBlk++) {
              if (!m_volmd.bQuarterSample) { //added by mwi
		motionCompEncY(m_pvopcRefQ0->pixelsY (), m_puciRefQZoom0->pixels(),
                               fwd + iBlkOffset[iBlk], BLOCK_SIZE, &mvFwd[iBlk], x + iXOffset[iBlk], y + iYOffset[iBlk],0);
		motionCompEncY(m_pvopcRefQ1->pixelsY (), m_puciRefQZoom1->pixels(),
                               bak + iBlkOffset[iBlk], BLOCK_SIZE, &mvBak[iBlk], x + iXOffset[iBlk], y + iYOffset[iBlk],0);
              }
              else {
                motionCompQuarterSample (fwd + iBlkOffset[iBlk],
                                         m_pvopcRefQ0->pixelsY (), 
                                         BLOCK_SIZE,
                                         4*(x+iXOffset[iBlk]) + mvFwd[iBlk].m_vctTrueHalfPel.x,
                                         4*(y+iYOffset[iBlk]) + mvFwd[iBlk].m_vctTrueHalfPel.y,
                                         m_vopmd.iRoundingControl,
                                         &m_rctRefVOPY0
                                         );

                motionCompQuarterSample (bak + iBlkOffset[iBlk],
                                         m_pvopcRefQ1->pixelsY (), 
                                         BLOCK_SIZE,
                                         4*(x+iXOffset[iBlk]) + mvBak[iBlk].m_vctTrueHalfPel.x,
                                         4*(y+iYOffset[iBlk]) + mvBak[iBlk].m_vctTrueHalfPel.y,
                                         m_vopmd.iRoundingControl,
                                         &m_rctRefVOPY1
                                         );
              }
	    } //~added by mwi
	    Int iSAD, i;
	    for (iSAD = 0, i = 0; i < MB_SQUARE_SIZE; i++)
	      iSAD += abs(m_ppxlcCurrMBY[i] - ((fwd[i] + bak[i] + 1) >> 1));

	    if (iSAD < directSAD) {
	      directSAD = iSAD;
				//pmbmd->m_vctDMV = vctDMV;
				//wchen-Jan06-97: to be consistent with non-interlaced case
	      pmbmd->m_vctDirectDeltaMV = vctDMV;
	    }
	  }
	}
#ifdef __TRACE_AND_STATS_ //1 // #ifdef __TRACE_AND_STATS_
	Int iDirInfo[10];
	iDirInfo[0] = pmvRef[1].m_vctTrueHalfPel.x; iDirInfo[1] = pmvRef[1].m_vctTrueHalfPel.y;
	iDirInfo[2] = pmvRef[2].m_vctTrueHalfPel.x; iDirInfo[3] = pmvRef[2].m_vctTrueHalfPel.y;
	iDirInfo[4] = pmvRef[3].m_vctTrueHalfPel.x; iDirInfo[5] = pmvRef[3].m_vctTrueHalfPel.y;
	iDirInfo[6] = pmvRef[4].m_vctTrueHalfPel.x; iDirInfo[7] = pmvRef[4].m_vctTrueHalfPel.y;
	//iDirInfo[8] = pmbmd->m_vctDMV.x;            iDirInfo[9] = pmbmd->m_vctDMV.y;
	iDirInfo[8] = pmbmd->m_vctDirectDeltaMV.x;
	iDirInfo[9] = pmbmd->m_vctDirectDeltaMV.y;
	m_pbitstrmOut->trace (iDirInfo, 10, "FrameDirect");
#endif // __TRACE_AND_STATS_
	return directSAD;
}

Int CVideoObjectEncoder::directSADField(
	CoordI x, CoordI y,
	CMBMode *pmbmd,
	const CMBMode *pmbmdRef,
	const CMotionVector *pmvRef,
	const PixelC* ppxlcRef0MBY,
	const PixelC* ppxlcRef1MBY
)
{
  static I8 iTROffsetTop[] = {  0, 0,  1, 1, 0, 0, -1, -1 };
  static I8 iTROffsetBot[] = { -1, 0, -1, 0, 1, 0,  1,  0 };
  static CRct rctBounds;
  static CRct rctBoundsBackward; // added by mwi
  static Int iLastNumber = -1;
  const CMotionVector *pmvRefTop, *pmvRefBot;
  PixelC fwd[MB_SQUARE_SIZE], bak[MB_SQUARE_SIZE];
  CVector vctFwdTop, vctFwdBot, vctBakTop, vctBakBot, vctDMV, vctCurrZoomMB(x << 1, y << 1);
  //Int iDirectSAD = 256*256;
  Int iDirectSAD = 4096*256; // nbit! (mwi)
	
  if (m_t != iLastNumber) {
    // assert(m_rctRefVOPY0 == m_rctRefVOPY1);		// Probably only true for rectangular VOPs //mwi
    rctBounds = m_rctRefVOPY0.upSampleBy2();
    rctBoundsBackward = m_rctRefVOPY1.upSampleBy2(); // added by mwi
    if (m_volmd.bQuarterSample) { // added by mwi bounding Rct in Quarter Pels
      rctBounds = rctBounds.upSampleBy2();
      rctBounds.expand(0, 0, -4*MB_SIZE, -4*MB_SIZE);
      rctBoundsBackward = rctBoundsBackward.upSampleBy2();    // added by mwi
      rctBoundsBackward.expand(0, 0, -4*MB_SIZE, -4*MB_SIZE); // added by mwi
    }
    else {
      rctBounds.expand(0, 0, -2*MB_SIZE, -2*MB_SIZE);
      rctBoundsBackward.expand(0, 0, -2*MB_SIZE, -2*MB_SIZE); // added by mwi
    }
    iLastNumber = m_t;
  }
  if (m_volmd.bQuarterSample)  // added by mwi
    vctCurrZoomMB = vctCurrZoomMB*2; // vctCurrZoomMB is not static

  assert((pmbmdRef->m_dctMd != INTRA) && (pmbmdRef->m_dctMd != INTRAQ));
  Int iTopRefFldOffset = 0, iBotRefFldOffset = 0;
  Int iCode = ((Int)vopmd().bTopFieldFirst) << 2;
  if (pmbmdRef->m_bForwardTop) {
    iCode |= 2;
    iTopRefFldOffset = m_iFrameWidthY;
    pmvRefTop = pmvRef + 6;
  } else
    pmvRefTop = pmvRef + 5;
  if (pmbmdRef->m_bForwardBottom) {
    iCode |= 1;
    iBotRefFldOffset = m_iFrameWidthY;
    pmvRefBot = pmvRef + 8;
  } else
    pmvRefBot = pmvRef + 7;
  Int iTempRefDTop = 2*(m_tFutureRef - m_tPastRef) + iTROffsetTop[iCode];
  Int iTempRefDBot = 2*(m_tFutureRef - m_tPastRef) + iTROffsetBot[iCode];
  Int iTempRefBTop = 2*(m_t          - m_tPastRef) + iTROffsetTop[iCode];
  Int iTempRefBBot = 2*(m_t          - m_tPastRef) + iTROffsetBot[iCode];
  Int iRadius = m_vopmd.iDirectModeRadius;
  ppxlcRef0MBY -= EXPANDY_REF_FRAME * (m_iFrameWidthY + 1);
  ppxlcRef1MBY -= EXPANDY_REF_FRAME * (m_iFrameWidthY + 1);

  const PixelC* ppxlcRefLeftTopQ0 = m_pvopcRefQ0->pixelsY ();
  const PixelC* ppxlcRefLeftTopQ1 = m_pvopcRefQ1->pixelsY ();

  for (vctDMV.y = -iRadius; vctDMV.y <= iRadius; vctDMV.y++) {
    for (vctDMV.x = -iRadius; vctDMV.x <= iRadius; vctDMV.x++) {

      // Find MVs for the top field
      vctFwdTop = (pmvRefTop->m_vctTrueHalfPel * iTempRefBTop) / iTempRefDTop + vctDMV;
      if (!rctBounds.includes(vctCurrZoomMB + vctFwdTop))
	continue;
      vctBakTop.x = vctDMV.x ? (vctFwdTop.x - pmvRefTop->m_vctTrueHalfPel.x) :
	((pmvRefTop->m_vctTrueHalfPel.x * (iTempRefBTop - iTempRefDTop)) / iTempRefDTop);
      vctBakTop.y = vctDMV.y ? (vctFwdTop.y - pmvRefTop->m_vctTrueHalfPel.y) :
	((pmvRefTop->m_vctTrueHalfPel.y * (iTempRefBTop - iTempRefDTop)) / iTempRefDTop);
      //if (!rctBounds.includes(vctCurrZoomMB + vctBakTop))
      if (!rctBoundsBackward.includes(vctCurrZoomMB + vctBakTop)) //mwi
	continue;

      // Find MVs for the bottom field
      vctFwdBot = (pmvRefBot->m_vctTrueHalfPel * iTempRefBBot) / iTempRefDBot + vctDMV;
      if (!rctBounds.includes(vctCurrZoomMB + vctFwdBot))
	continue;
      vctBakBot.x = vctDMV.x ? (vctFwdBot.x - pmvRefBot->m_vctTrueHalfPel.x) :
	((pmvRefBot->m_vctTrueHalfPel.x * (iTempRefBBot - iTempRefDBot)) / iTempRefDBot);
      vctBakBot.y = vctDMV.y ? (vctFwdBot.y - pmvRefBot->m_vctTrueHalfPel.y) :
	((pmvRefBot->m_vctTrueHalfPel.y * (iTempRefBBot - iTempRefDBot)) / iTempRefDBot);
      //if (!rctBounds.includes(vctCurrZoomMB + vctBakBot))
      if (!rctBoundsBackward.includes(vctCurrZoomMB + vctBakBot)) //mwi
	continue;

      // Motion compensate the luma top field
      if (m_volmd.bQuarterSample) {// Quarter sample, added by mwi
	motionCompQuarterSample (fwd, 
				 ppxlcRefLeftTopQ0 + iTopRefFldOffset,
				 0,
				 4*x + vctFwdTop.x,
				 4*y + vctFwdTop.y,
				 m_vopmd.iRoundingControl, &m_rctRefVOPY0
				 );
	motionCompQuarterSample (bak, 
				 ppxlcRefLeftTopQ1,
				 0,
				 4*x + vctBakTop.x,
				 4*y + vctBakTop.y,
				 m_vopmd.iRoundingControl, &m_rctRefVOPY1
				 );
      }
      else {
	motionCompYField(fwd,           ppxlcRef0MBY + iTopRefFldOffset, vctFwdTop.x, vctFwdTop.y,
			&m_rctRefVOPY0); //added by Y.Suzuki for the extended bounding box support
	motionCompYField(bak,           ppxlcRef1MBY,                    vctBakTop.x, vctBakTop.y,
			&m_rctRefVOPY1); // added by Y.Suzuki for the extended bounding box support
      }
            

      // Motion compensate the luma bottom field
      if (m_volmd.bQuarterSample) {// Quarter sample, added by mwi
	motionCompQuarterSample (
				 fwd + MB_SIZE, 
				 ppxlcRefLeftTopQ0 + iBotRefFldOffset,
				 0,
				 4*x + vctFwdTop.x,
				 4*y + vctFwdTop.y,
				 m_vopmd.iRoundingControl, &m_rctRefVOPY0
				 );
	motionCompQuarterSample (
				 bak + MB_SIZE, 
				 ppxlcRefLeftTopQ1 + m_iFrameWidthY,
				 0,
				 4*x + vctBakTop.x,
				 4*y + vctBakTop.y,
				 m_vopmd.iRoundingControl, &m_rctRefVOPY1
				 );
      }
      else {
	motionCompYField(fwd + MB_SIZE, ppxlcRef0MBY + iBotRefFldOffset, vctFwdBot.x, vctFwdBot.y,
			&m_rctRefVOPY0); // added by Y.Suzuki for the extended bounding box support
	motionCompYField(bak + MB_SIZE, ppxlcRef1MBY + m_iFrameWidthY,   vctBakBot.x, vctBakBot.y,
			&m_rctRefVOPY1); // added by Y.Suzuki for the extended bounding box support
      }
            
			
      // Coumpute SAD
      Int iSAD = 0;
      for (Int i = 0; i < MB_SQUARE_SIZE; i++)
	iSAD += abs(m_ppxlcCurrMBY[i] - ((fwd[i] + bak[i] + 1) >> 1));

      if (iSAD < iDirectSAD) {
	iDirectSAD = iSAD;
				//pmbmd->m_vctDMV = vctDMV;
	pmbmd->m_vctDirectDeltaMV = vctDMV;
      }
    }
  }
#ifdef __TRACE_AND_STATS_ //#if 0 // #ifdef __TRACE_AND_STATS_
  Int iDirInfo[8];
  iDirInfo[0] = iCode;
  iDirInfo[1] = pmvRefTop->m_vctTrueHalfPel.x; iDirInfo[2] = pmvRefTop->m_vctTrueHalfPel.y;
  iDirInfo[3] = pmvRefBot->m_vctTrueHalfPel.x; iDirInfo[4] = pmvRefBot->m_vctTrueHalfPel.y;
  //iDirInfo[5] = pmbmd->m_vctDMV.x;             iDirInfo[6] = pmbmd->m_vctDMV.y;
  iDirInfo[5] = pmbmd->m_vctDirectDeltaMV.x;
  iDirInfo[6] = pmbmd->m_vctDirectDeltaMV.y;
  m_pbitstrmOut->trace (iDirInfo, 7, "FieldDirect");
#endif // __TRACE_AND_STATS_
  return iDirectSAD;
}
// ~INTERLACE

// INTERLACE
Int CVideoObjectEncoder::blkmatch16x8 (
	CMotionVector* pmv, 
	CoordI iXMB, CoordI iYMB,
	Int iFieldSelect,
	const PixelC* ppxlcRefMBY,
	const PixelC* ppxlcRefHalfPel,
	Int iSearchRange,
    Bool bQuarterSample
)
{
	Int mbDiff; // start with big
	Int iWidth2 = 2*m_iFrameWidthY;
	Int iXDest = iXMB, iYDest = iYMB;
	CoordI x = iXMB, y = iYMB, ix, iy;
	Int uiPosInLoop, iLoop;
	Int iMinSAD = 0;

    // for QuarterSample
    //Left Top of Ref-Frame:
    const PixelC* ppxlcRefLeftTop = ppxlcRefHalfPel - m_iStartInRefToCurrRctY - iXMB - iYMB*m_iFrameWidthY; // Top or Bottom Field

    U8* ppxlcInterpBlk = NULL;
    U8* ppxlcInterpAct;
    
    Int i2MB_SIZE = 2*MB_SIZE;
    if (bQuarterSample) {
      ppxlcInterpBlk = (U8*) calloc(MB_SIZE*MB_SIZE,sizeof(U8)); 
    }
    // ~for QuarterSample

	if (m_iMVFileUsage != 1) {
		
      // Calculate the center point SAD
		const PixelC* ppxlcCurrY = m_ppxlcCurrMBY + iFieldSelect;
		const PixelC* ppxlcRefMB = ppxlcRefMBY;
		for (iy = 0; iy < MB_SIZE/2; iy++) {
			for (ix = 0; ix < MB_SIZE; ix++)
				iMinSAD += abs (ppxlcCurrY [ix] - ppxlcRefMB [ix]);
			ppxlcCurrY += 2*MB_SIZE;
			ppxlcRefMB += iWidth2;
		}
		// iMinSAD -= FAVORZERO/2;		// Apply center favoring bias?

		// Spiral Search for the rest
		for (iLoop = 1; iLoop <= iSearchRange; iLoop++) {
			x++;
			y++;
			ppxlcRefMBY += m_iFrameWidthY + 1;
			for (uiPosInLoop = 0; uiPosInLoop < (iLoop << 3); uiPosInLoop++) { // inside each spiral loop 
				if (
					x >= m_rctRefVOPY0.left && y >= m_rctRefVOPY0.top && 
					//x <= m_rctRefVOPY0.right - MB_SIZE && y <= m_rctRefVOPY0.bottom - MB_SIZE //changed by mwi
					x < m_rctRefVOPY0.right - MB_SIZE && y < m_rctRefVOPY0.bottom - MB_SIZE
					&& ((y-iYMB)%2==0)
				) {
					ppxlcCurrY = m_ppxlcCurrMBY+iFieldSelect;
					ppxlcRefMB = ppxlcRefMBY;
					mbDiff = 0;
					// full pel field prediction
					for (iy = 0; iy < MB_SIZE; iy+=2) {
						for (ix = 0; ix < MB_SIZE; ix++)
							mbDiff += abs (ppxlcCurrY [ix] - ppxlcRefMB [ix]);

						if (mbDiff > iMinSAD)
							goto NEXT_POSITION; // skip the current position
						ppxlcRefMB += iWidth2;
						ppxlcCurrY += MB_SIZE*2;
					}
					iMinSAD = mbDiff;
					iXDest = x;
					iYDest = y;
				}
NEXT_POSITION:	
				if (uiPosInLoop < (iLoop << 1)) {
					ppxlcRefMBY--;
					x--;
				}
				else if (uiPosInLoop < (iLoop << 2)) {
					ppxlcRefMBY -= m_iFrameWidthY;
					y--;
				}
				else if (uiPosInLoop < (iLoop * 6)) {
					ppxlcRefMBY++;					  
					x++;
				}
				else {
					ppxlcRefMBY += m_iFrameWidthY;
					y++;
				}
			}
		}
		*pmv = CMotionVector (iXDest - iXMB, iYDest - iYMB);
	} else {
		iXDest = iXMB + pmv->iMVX;
		iYDest = iYMB + pmv->iMVY;
	}
    
	// half pel search
	I8 destHalfX = 0, destHalfY = 0;
	Int lowX = (iXDest == m_rctRefVOPY0.left) ? 0 : -1;
	Int lowY = (iYDest == m_rctRefVOPY0.top) ? 0 : -2;
	Int highX = (iXDest == m_rctRefVOPY0.right - MB_SIZE) ? 0 : 1;
	Int highY = (iYDest == m_rctRefVOPY0.bottom - MB_SIZE) ? 0 : 2;
	Int iRound1 = 1 - m_vopmd.iRoundingControl;
	Int iRound2 = 2 - m_vopmd.iRoundingControl;
	assert(!(pmv->iMVY & 1));
	ppxlcRefHalfPel += pmv->iMVX + m_iFrameWidthY * pmv->iMVY; // 12.22.98 changes
    // nbit: change to a bigger number! //mwi
    iMinSAD = 4096*256;
    
    //for quarter pel search
    I8 destQuarterX = 0, destQuarterY = 0;

    if (!bQuarterSample) {
      // half pel search
      for (Int incY = lowY; incY <= highY; incY += 2) {
		for (Int incX = lowX; incX <= highX; incX++) {
          const PixelC *ppxlcRef = ppxlcRefHalfPel;
          const PixelC *ppxlcCur = m_ppxlcCurrMBY + iFieldSelect;
          if (incY < 0)
            ppxlcRef -= iWidth2;
          if (incX < 0)
            ppxlcRef--;
          mbDiff = 0;
          if (incY == 0) {
            if (incX == 0) {
              for (iy = 0; iy < MB_SIZE; iy += 2) {
                for (ix = 0; ix < MB_SIZE; ix++)
                  mbDiff += abs(ppxlcRef[ix] - ppxlcCur[ix]);
                if (mbDiff > iMinSAD)
                  goto NEXT_HALF_POSITION;
                ppxlcRef += iWidth2;
                ppxlcCur += MB_SIZE*2;
              }
            } else {
              for (iy = 0; iy < MB_SIZE; iy += 2) {
                for (ix = 0; ix < MB_SIZE; ix++)
                  mbDiff += abs(((ppxlcRef[ix] + ppxlcRef[ix+1] + iRound1) >> 1) - ppxlcCur[ix]);
                if (mbDiff > iMinSAD)
                  goto NEXT_HALF_POSITION;
                ppxlcRef += iWidth2;
                ppxlcCur += MB_SIZE*2;
              }
            }
          } else {
            if (incX == 0) {
              for (iy = 0; iy < MB_SIZE; iy += 2) {
                for (ix = 0; ix < MB_SIZE; ix++)
                  mbDiff += abs(((ppxlcRef[ix] + ppxlcRef[ix+iWidth2] + iRound1) >> 1) - ppxlcCur[ix]);
                if (mbDiff > iMinSAD)
                  goto NEXT_HALF_POSITION;
                ppxlcRef += iWidth2;
                ppxlcCur += MB_SIZE*2;
              }
            } else {
              for (iy = 0; iy < MB_SIZE; iy += 2) {
                for (ix = 0; ix < MB_SIZE; ix++)
                  mbDiff += abs(((ppxlcRef[ix] + ppxlcRef[ix+1] + ppxlcRef[ix+iWidth2] +
                                  ppxlcRef[ix+iWidth2+1] + iRound2) >> 2) - ppxlcCur[ix]);
                if (mbDiff > iMinSAD)
                  goto NEXT_HALF_POSITION;
                ppxlcRef += iWidth2;
                ppxlcCur += MB_SIZE*2;
              }
            }
          }
          if ((iMinSAD > mbDiff) ||
              ((abs(pmv->m_vctTrueHalfPel.x + destHalfX) + abs(pmv->m_vctTrueHalfPel.y + destHalfY)) >
               (abs(pmv->m_vctTrueHalfPel.x + incX)      + abs(pmv->m_vctTrueHalfPel.y + incY)))) {
            iMinSAD = mbDiff;
            destHalfX = incX;
            destHalfY = incY;
          }
        NEXT_HALF_POSITION:
          ;
		}
      }
      pmv->iHalfX = destHalfX;
      pmv->iHalfY = destHalfY;	// Violates range of -1,0,1 but need to keep (iMVX,iMVY) pure for MV file write
      return iMinSAD;
    }
    else { // QUARTER SAMPLE 
      // half pel search
      Int incY;
      for (incY = lowY; incY <= highY; incY += 2) {
		for (Int incX = lowX; incX <= highX; incX++) {
          ppxlcInterpAct = ppxlcInterpBlk;
          const PixelC *ppxlcCur = m_ppxlcCurrMBY + iFieldSelect;
          blkInterpolateY(ppxlcRefLeftTop, 0, 4*iXDest+2*incX, 4*iYDest+2*incY, ppxlcInterpAct, m_vopmd.iRoundingControl);

          mbDiff = 0;
          for (iy = 0; iy < MB_SIZE; iy += 2) {
            for (ix = 0; ix < MB_SIZE; ix++)
              mbDiff += abs(ppxlcInterpAct[ix] - ppxlcCur[ix]);
            if (mbDiff > iMinSAD)
              goto NEXT_HALF_POSITION_QS;
            ppxlcInterpAct += i2MB_SIZE;
            ppxlcCur += i2MB_SIZE;
          }
          if ((iMinSAD > mbDiff) ||
              ((abs(pmv->m_vctTrueHalfPel.x + destHalfX) + abs(pmv->m_vctTrueHalfPel.y + destHalfY)) >
               (abs(pmv->m_vctTrueHalfPel.x + incX)      + abs(pmv->m_vctTrueHalfPel.y + incY)))) {
            iMinSAD = mbDiff;
            destHalfX = incX;
            destHalfY = incY;
          }
        NEXT_HALF_POSITION_QS:
          ;
		}
      }
      pmv->iMVX = 2*pmv->iMVX + destHalfX;
      pmv->iMVY = 2*pmv->iMVY + destHalfY;

      // Half Pel Ref Position
      iXDest = 4*iXDest + 2*destHalfX;
      iYDest = 4*iYDest + 2*destHalfY;
      
      iMinSAD = 4096*256;

      // quarter pel search
      for (/*Int*/incY = lowY; incY <= highY; incY += 2) {
		for (Int incX = lowX; incX <= highX; incX++) {
          ppxlcInterpAct = ppxlcInterpBlk;
          const PixelC *ppxlcCur = m_ppxlcCurrMBY + iFieldSelect;
          blkInterpolateY(ppxlcRefLeftTop, 0, iXDest+incX, iYDest+incY, ppxlcInterpAct, m_vopmd.iRoundingControl);

          mbDiff = 0;
          for (iy = 0; iy < MB_SIZE; iy += 2) {
            for (ix = 0; ix < MB_SIZE; ix++)
              mbDiff += abs(ppxlcInterpAct[ix] - ppxlcCur[ix]);
            if (mbDiff > iMinSAD)
              goto NEXT_QUARTER_POSITION_QS;
            ppxlcInterpAct += i2MB_SIZE;
            ppxlcCur += i2MB_SIZE;
          }
          if ((iMinSAD > mbDiff) ||
              ((abs(pmv->m_vctTrueHalfPel.x + destQuarterX) + abs(pmv->m_vctTrueHalfPel.y + destQuarterY)) >
               (abs(pmv->m_vctTrueHalfPel.x + incX)      + abs(pmv->m_vctTrueHalfPel.y + incY)))) {
            iMinSAD = mbDiff;
            destQuarterX = incX;
            destQuarterY = incY;
          }
        NEXT_QUARTER_POSITION_QS:
          ;
		}
      }
      pmv->iHalfX = destQuarterX;
      pmv->iHalfY = destQuarterY;
     
      free(ppxlcInterpBlk);
      return iMinSAD;
    }
}


// new changes
Int CVideoObjectEncoder::blkmatch16x8WithShape (
	CMotionVector* pmv, 
	CoordI iXMB, CoordI iYMB,
	Int iFieldSelect,
	const PixelC* ppxlcRefMBY,
	const PixelC* ppxlcRefHalfPel,
	Int iSearchRange,
    Bool bQuarterSample,
	Int iDirection
)
{
	Int mbDiff; // start with big
	Int iWidth2 = 2*m_iFrameWidthY;
	Int iXDest = iXMB, iYDest = iYMB;
	CoordI x = iXMB, y = iYMB, ix, iy;
	Int uiPosInLoop, iLoop;
	Int iMinSAD = 0;

    CRct rctRefVOPY = (iDirection) ? m_rctRefVOPY1:m_rctRefVOPY0;

    // for QuarterSample
    //Left Top of Ref-Frame:
    const PixelC* ppxlcRefLeftTop = ppxlcRefHalfPel - m_iStartInRefToCurrRctY - iXMB - iYMB*m_iFrameWidthY; // Top or Bottom Field

    U8* ppxlcInterpBlk = NULL;
    U8* ppxlcInterpAct;
    
    Int i2MB_SIZE = 2*MB_SIZE;
    if (bQuarterSample) {
      ppxlcInterpBlk = (U8*) calloc(MB_SIZE*MB_SIZE,sizeof(U8)); 
    }
    // ~for QuarterSample


	if (m_iMVFileUsage != 1) {
		
		// Calculate the center point SAD
		const PixelC* ppxlcCurrY = m_ppxlcCurrMBY + iFieldSelect;
		const PixelC* ppxlcCurrBY = m_ppxlcCurrMBBY + iFieldSelect;
		const PixelC* ppxlcRefMB = ppxlcRefMBY;
		for (iy = 0; iy < MB_SIZE/2; iy++) {
			for (ix = 0; ix < MB_SIZE; ix++) {
				if(ppxlcCurrBY[ix] != transpValue)
					iMinSAD += abs (ppxlcCurrY [ix] - ppxlcRefMB [ix]);
			}
			ppxlcCurrY += 2*MB_SIZE;
			ppxlcCurrBY += 2*MB_SIZE;
			ppxlcRefMB += iWidth2;
		}
		// iMinSAD -= FAVORZERO/2;		// Apply center favoring bias?

		// Spiral Search for the rest
		for (iLoop = 1; iLoop <= iSearchRange; iLoop++) {
			x++;
			y++;
			ppxlcRefMBY += m_iFrameWidthY + 1;
			for (uiPosInLoop = 0; uiPosInLoop < (iLoop << 3); uiPosInLoop++) { // inside each spiral loop 
				if (
					x >= rctRefVOPY.left && y >= rctRefVOPY.top && 
					//x <= m_rctRefVOPY0.right - MB_SIZE && y <= m_rctRefVOPY0.bottom - MB_SIZE //changed by mwi
					x < rctRefVOPY.right - MB_SIZE && y < rctRefVOPY.bottom - MB_SIZE
					&& ((y-iYMB)%2==0)
				) {
					ppxlcCurrY = m_ppxlcCurrMBY+iFieldSelect;
					ppxlcCurrBY = m_ppxlcCurrMBBY + iFieldSelect;
					ppxlcRefMB = ppxlcRefMBY;
					mbDiff = 0;
					// full pel field prediction
					for (iy = 0; iy < MB_SIZE; iy+=2) {
						for (ix = 0; ix < MB_SIZE; ix++) {
							if(ppxlcCurrBY[ix] != transpValue)
								mbDiff += abs (ppxlcCurrY [ix] - ppxlcRefMB [ix]);
						}

						if (mbDiff > iMinSAD)
							goto NEXT_POSITION; // skip the current position
						ppxlcRefMB += iWidth2;
						ppxlcCurrY += MB_SIZE*2;
						ppxlcCurrBY += MB_SIZE*2;
					}
					iMinSAD = mbDiff;
					iXDest = x;
					iYDest = y;
				}
NEXT_POSITION:	
				if (uiPosInLoop < (iLoop << 1)) {
					ppxlcRefMBY--;
					x--;
				}
				else if (uiPosInLoop < (iLoop << 2)) {
					ppxlcRefMBY -= m_iFrameWidthY;
					y--;
				}
				else if (uiPosInLoop < (iLoop * 6)) {
					ppxlcRefMBY++;					  
					x++;
				}
				else {
					ppxlcRefMBY += m_iFrameWidthY;
					y++;
				}
			}
		}
		*pmv = CMotionVector (iXDest - iXMB, iYDest - iYMB);
	} else {
		iXDest = iXMB + pmv->iMVX;
		iYDest = iYMB + pmv->iMVY;
	}

// 12.22.98 begin of changes, X. Chen
// check for case when zero MV due to MB outside the extended BB. (CTmp->rctRefVOPY: modified by mwi)
    /*	if(iXDest==iXMB && iYDest==iYMB
		&& (iXDest < rctRefVOPY.left || iYDest < rctRefVOPY.top	
			|| iXDest > rctRefVOPY.right - MB_SIZE		
			|| iYDest > rctRefVOPY.bottom - MB_SIZE))		
	{
		pmv -> iHalfX = 0;
		pmv -> iHalfY = 0;
		return iMinSAD;  
	} */
// 12.22.98 end of changes X.Chen

	// half pel search
	I8 destHalfX = 0, destHalfY = 0;
	Int lowX = (iXDest == rctRefVOPY.left) ? 0 : -1;
	Int lowY = (iYDest == rctRefVOPY.top) ? 0 : -2;
	Int highX = (iXDest == rctRefVOPY.right - MB_SIZE) ? 0 : 1;
	Int highY = (iYDest == rctRefVOPY.bottom - MB_SIZE) ? 0 : 2;
	Int iRound1 = 1 - m_vopmd.iRoundingControl;
	Int iRound2 = 2 - m_vopmd.iRoundingControl;
	assert(!(pmv->iMVY & 1));
	ppxlcRefHalfPel += pmv->iMVX + m_iFrameWidthY * pmv->iMVY; // 12.22.98
    iMinSAD = 4096*256; // nbit

    //for quarter pel search
    I8 destQuarterX = 0, destQuarterY = 0;

    if (!bQuarterSample) {
      // half pel search
      for (Int incY = lowY; incY <= highY; incY += 2) {
		for (Int incX = lowX; incX <= highX; incX++) {
          const PixelC *ppxlcRef = ppxlcRefHalfPel;
          const PixelC *ppxlcCurB = m_ppxlcCurrMBBY + iFieldSelect;
          const PixelC *ppxlcCur = m_ppxlcCurrMBY + iFieldSelect;
          if (incY < 0)
            ppxlcRef -= iWidth2;
          if (incX < 0)
            ppxlcRef--;
          mbDiff = 0;
          if (incY == 0) {
            if (incX == 0) {
              for (iy = 0; iy < MB_SIZE; iy += 2) {
                for (ix = 0; ix < MB_SIZE; ix++) {
                  if(ppxlcCurB[ix] != transpValue)
                    mbDiff += abs(ppxlcRef[ix] - ppxlcCur[ix]);
                }
                if (mbDiff > iMinSAD)
                  goto NEXT_HALF_POSITION;
                ppxlcRef += iWidth2;
                ppxlcCur += MB_SIZE*2;
                ppxlcCurB += MB_SIZE*2;
              }
            } else {
              for (iy = 0; iy < MB_SIZE; iy += 2) {
                for (ix = 0; ix < MB_SIZE; ix++) {
                  if(ppxlcCurB[ix] != transpValue)
                    mbDiff += abs(((ppxlcRef[ix] + ppxlcRef[ix+1] + iRound1) >> 1) - ppxlcCur[ix]);
                }
                if (mbDiff > iMinSAD)
                  goto NEXT_HALF_POSITION;
                ppxlcRef += iWidth2;
                ppxlcCur += MB_SIZE*2;
                ppxlcCurB += MB_SIZE*2;
              }
            }
          } else {
            if (incX == 0) {
              for (iy = 0; iy < MB_SIZE; iy += 2) {
                for (ix = 0; ix < MB_SIZE; ix++) {
                  if(ppxlcCurB[ix] != transpValue)
                    mbDiff += abs(((ppxlcRef[ix] + ppxlcRef[ix+iWidth2] + iRound1) >> 1) - ppxlcCur[ix]);
                }
                if (mbDiff > iMinSAD)
                  goto NEXT_HALF_POSITION;
                ppxlcRef += iWidth2;
                ppxlcCur += MB_SIZE*2;
                ppxlcCurB += MB_SIZE*2;
              }
            } else {
              for (iy = 0; iy < MB_SIZE; iy += 2) {
                for (ix = 0; ix < MB_SIZE; ix++) {
                  if(ppxlcCurB[ix] != transpValue)
                    mbDiff += abs(((ppxlcRef[ix] + ppxlcRef[ix+1] + ppxlcRef[ix+iWidth2] +
									ppxlcRef[ix+iWidth2+1] + iRound2) >> 2) - ppxlcCur[ix]);
                }
                if (mbDiff > iMinSAD)
                  goto NEXT_HALF_POSITION;
                ppxlcRef += iWidth2;
                ppxlcCur += MB_SIZE*2;
                ppxlcCurB += MB_SIZE*2;
              }
            }
          }
          if ((iMinSAD > mbDiff) ||
              ((abs(pmv->m_vctTrueHalfPel.x + destHalfX) + abs(pmv->m_vctTrueHalfPel.y + destHalfY)) >
               (abs(pmv->m_vctTrueHalfPel.x + incX)      + abs(pmv->m_vctTrueHalfPel.y + incY)))) {
            iMinSAD = mbDiff;
            destHalfX = incX;
            destHalfY = incY;
          }
NEXT_HALF_POSITION:
          ;
		}
      }
      pmv->iHalfX = destHalfX;
      pmv->iHalfY = destHalfY;	// Violates range of -1,0,1 but need to keep (iMVX,iMVY) pure for MV file write
      return iMinSAD;
    }
    else { // QUARTER SAMPLE 
      // half pel search
      Int incY;
      for (incY = lowY; incY <= highY; incY += 2) {
		for (Int incX = lowX; incX <= highX; incX++) {
          ppxlcInterpAct = ppxlcInterpBlk;
          const PixelC *ppxlcCur = m_ppxlcCurrMBY + iFieldSelect;
          const PixelC *ppxlcCurB = m_ppxlcCurrMBBY + iFieldSelect;
          blkInterpolateY(ppxlcRefLeftTop, 0, 4*iXDest+2*incX, 4*iYDest+2*incY, ppxlcInterpAct, m_vopmd.iRoundingControl);

          mbDiff = 0;
          for (iy = 0; iy < MB_SIZE; iy += 2) {
            for (ix = 0; ix < MB_SIZE; ix++)
              if(ppxlcCurB[ix] != transpValue)
                mbDiff += abs(ppxlcInterpAct[ix] - ppxlcCur[ix]);
            if (mbDiff > iMinSAD)
              goto NEXT_HALF_POSITION_QS;
            ppxlcInterpAct += i2MB_SIZE;
            ppxlcCur  += i2MB_SIZE;
            ppxlcCurB += i2MB_SIZE;
          }
          if ((iMinSAD > mbDiff) ||
              ((abs(pmv->m_vctTrueHalfPel.x + destHalfX) + abs(pmv->m_vctTrueHalfPel.y + destHalfY)) >
               (abs(pmv->m_vctTrueHalfPel.x + incX)      + abs(pmv->m_vctTrueHalfPel.y + incY)))) {
            iMinSAD = mbDiff;
            destHalfX = incX;
            destHalfY = incY;
          }
        NEXT_HALF_POSITION_QS:
          ;
		}
      }
      pmv->iMVX = 2*pmv->iMVX + destHalfX;
      pmv->iMVY = 2*pmv->iMVY + destHalfY;

      // Half Pel Ref Position
      iXDest = 4*iXDest + 2*destHalfX;
      iYDest = 4*iYDest + 2*destHalfY;
      
      iMinSAD = 4096*256;

      // quarter pel search
      for (/*Int*/incY = lowY; incY <= highY; incY += 2) {
		for (Int incX = lowX; incX <= highX; incX++) {
          ppxlcInterpAct = ppxlcInterpBlk;
          const PixelC *ppxlcCur = m_ppxlcCurrMBY + iFieldSelect;
          const PixelC *ppxlcCurB = m_ppxlcCurrMBBY + iFieldSelect;
          blkInterpolateY(ppxlcRefLeftTop, 0, iXDest+incX, iYDest+incY, ppxlcInterpAct, m_vopmd.iRoundingControl);

          mbDiff = 0;
          for (iy = 0; iy < MB_SIZE; iy += 2) {
            for (ix = 0; ix < MB_SIZE; ix++)
              if(ppxlcCurB[ix] != transpValue)
                mbDiff += abs(ppxlcInterpAct[ix] - ppxlcCur[ix]);
            if (mbDiff > iMinSAD)
              goto NEXT_QUARTER_POSITION_QS;
            ppxlcInterpAct += i2MB_SIZE;
            ppxlcCur  += i2MB_SIZE;
            ppxlcCurB += i2MB_SIZE;
          }
          if ((iMinSAD > mbDiff) ||
              ((abs(pmv->m_vctTrueHalfPel.x + destQuarterX) + abs(pmv->m_vctTrueHalfPel.y + destQuarterY)) >
               (abs(pmv->m_vctTrueHalfPel.x + incX)      + abs(pmv->m_vctTrueHalfPel.y + incY)))) {
            iMinSAD = mbDiff;
            destQuarterX = incX;
            destQuarterY = incY;
          }
        NEXT_QUARTER_POSITION_QS:
          ;
		}
      }
      pmv->iHalfX = destQuarterX;
      pmv->iHalfY = destQuarterY;
     
      free(ppxlcInterpBlk);
      return iMinSAD;
    }
}


// end of new changes
// ~INTERLACE

Void CVideoObjectEncoder::readPVOPMVs()
{
	Char line[150];
	int pic, type, ref, rad;
	CMotionVector* pmv = m_rgmv;
    CRct clip[9];

	m_iMVLineNo++;
	if (fgets(line, sizeof line, m_fMVFile) == NULL) {
		fprintf(stderr, "EOF on %s at %d\n", m_pchMVFileName, m_iMVLineNo);
		exit(1);
	}
	if ((sscanf(line + 4, "%d %d %d %d", &pic, &type, &ref, &rad) != 4) ||
		(pic != m_t) ||
		(type != m_vopmd.vopPredType) ||
		(ref != m_tPastRef)) {
		fprintf(stderr, "%s:%d MV read error: expected %c%d(%d), got %c%d(%d)\n",
			m_pchMVFileName, m_iMVLineNo, "IPBS"[m_vopmd.vopPredType],
			m_t, m_tPastRef, "IPBS"[type], pic, ref);
		exit(1);
	}
	if (rad > m_vopmd.iSearchRangeForward) {
		fprintf(stderr, "ME radius from file (%d) > current ME radius (%d)\n",
			  rad, m_vopmd.iSearchRangeForward);
		exit(1);
	}
	
    clip[0] = m_rctRefVOPY0; clip[0].expand(0,          0,          -MB_SIZE,    -MB_SIZE);    // 04/28/99
    clip[1] = m_rctRefVOPY0; clip[1].expand(0,          0,          -BLOCK_SIZE, -BLOCK_SIZE); // 04/28/99
    clip[2] = m_rctRefVOPY0; clip[2].expand(BLOCK_SIZE, 0,          -BLOCK_SIZE, -BLOCK_SIZE); // 04/28/99
    clip[3] = m_rctRefVOPY0; clip[3].expand(0,          BLOCK_SIZE, -BLOCK_SIZE, -BLOCK_SIZE); // 04/28/99
    clip[4] = m_rctRefVOPY0; clip[4].expand(BLOCK_SIZE, BLOCK_SIZE, -BLOCK_SIZE, -BLOCK_SIZE); // 04/28/99
    clip[5] = clip[0];
    clip[6] = clip[0];
    clip[7] = clip[0];
    clip[8] = clip[0];
    CRct ball(-rad, -rad, rad, rad);
	for (Int iMBY = 0; iMBY < m_iNumMBY; iMBY++) {
		for (Int iMBX = 0; iMBX < m_iNumMBX; iMBX++) {
			int x, y, inter;
			
			m_iMVLineNo++;
			if (fgets(line, sizeof line, m_fMVFile) == NULL) {
				fprintf(stderr, "EOF on %s at %d\n", m_pchMVFileName, m_iMVLineNo);
				exit(1);
			}
			if (sscanf(line,
"%d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d",
				&x, &y, &inter,  // MB coordinates and predicted-flag
				&pmv[0].iMVX, &pmv[0].iMVY, // 16x16 MV
				&pmv[5].iMVX, &pmv[5].iMVY, // field top-top MV
				&pmv[6].iMVX, &pmv[6].iMVY, // field top-bot MV
				&pmv[7].iMVX, &pmv[7].iMVY, // field bot-top MV
				&pmv[8].iMVX, &pmv[8].iMVY, // field bot-bot MV
				&pmv[1].iMVX, &pmv[1].iMVY, // Upper left  8x8 block MV
				&pmv[2].iMVX, &pmv[2].iMVY, // Upper right 8x8 block MV
				&pmv[3].iMVX, &pmv[3].iMVY, // Lower left  8x8 block MV
				&pmv[4].iMVX, &pmv[4].iMVY) // Lower right 8x8 block MV
			!= 21) {
				fprintf(stderr, "%s: Read error on line %d: %s\n",
					m_pchMVFileName, m_iMVLineNo, line);
				exit(1);
			}
			assert(x==iMBX && y==iMBY);
            x *= MB_SIZE;
            y *= MB_SIZE;
			for (Int i = 0; i < PVOP_MV_PER_REF_PER_MB; i++) {
				pmv[i].iMVX >>= 1;
				pmv[i].iMVY >>= 1;
                CRct bounds = clip[i];
                bounds.shift(-x, -y);
                bounds.clip(ball);
                if (bounds.left   > pmv[i].iMVX) pmv[i].iMVX = bounds.left;
                if (bounds.right  < pmv[i].iMVX) pmv[i].iMVX = bounds.right;
                if (bounds.top    > pmv[i].iMVY) pmv[i].iMVY = bounds.top;
                if (bounds.bottom < pmv[i].iMVY) pmv[i].iMVY = bounds.bottom;
				pmv[i].iHalfX = 0;
				pmv[i].iHalfY = 0;
				pmv[i].computeTrueMV();
			}
			pmv += PVOP_MV_PER_REF_PER_MB;
		}
	}
}

Void CVideoObjectEncoder::writePVOPMVs()
{
	CMotionVector* pmv = m_rgmv;
	CMBMode* pmbmd = m_rgmbmd;

	m_iMVLineNo++;
	fprintf(m_fMVFile, "Pic %d %d %d %d\n", m_t,
		(int)m_vopmd.vopPredType, m_tPastRef, m_vopmd.iSearchRangeForward);

	for (Int iMBY = 0; iMBY < m_iNumMBY; iMBY++) {
		for (Int iMBX = 0; iMBX < m_iNumMBX; iMBX++) {
			fprintf(m_fMVFile,
"%3d %3d %3d %3d %3d %3d %3d %3d %3d %3d %3d %3d %3d %3d %3d %3d %3d %3d %3d %3d %3d\n",
				iMBX, iMBY, pmbmd->m_dctMd != INTRA,
				2*pmv[0].iMVX, 2*pmv[0].iMVY,
				2*pmv[5].iMVX, 2*pmv[5].iMVY,
				2*pmv[6].iMVX, 2*pmv[6].iMVY,
				2*pmv[7].iMVX, 2*pmv[7].iMVY,
				2*pmv[8].iMVX, 2*pmv[8].iMVY,
				2*pmv[1].iMVX, 2*pmv[1].iMVY,
				2*pmv[2].iMVX, 2*pmv[2].iMVY,
				2*pmv[3].iMVX, 2*pmv[3].iMVY,
				2*pmv[4].iMVX, 2*pmv[4].iMVY);
			m_iMVLineNo++;
			pmv += PVOP_MV_PER_REF_PER_MB;
			pmbmd++;  // 04/28/99
		}
	}
}

Void CVideoObjectEncoder::readBVOPMVs()
{
	Char line[150];
	int pic, type, fref, bref, frad, brad;
	CMotionVector* pmvf = m_rgmv;
	CMotionVector* pmvb = m_rgmvBackward;

	m_iMVLineNo++;
	if (fgets(line, sizeof line, m_fMVFile) == NULL) {
		fprintf(stderr, "EOF on %s at %d\n", m_pchMVFileName, m_iMVLineNo);
		exit(1);
	}
	if ((sscanf(line + 4, "%d %d %d %d %d %d",
		&pic, &type, &fref, &bref, &frad, &brad) != 6) ||
		(pic != m_t) ||
		(type != m_vopmd.vopPredType) ||
		(fref != m_tPastRef) ||
		(bref != m_tFutureRef)) {
		fprintf(stderr, "%s:%d MV read error: expected %c%d(%d,%d), got %c%d(%d.%d)\n",
			m_pchMVFileName, m_iMVLineNo,
			"IPBS"[m_vopmd.vopPredType], m_t,
			m_tPastRef, m_tFutureRef,
			"IPBS"[type], pic, fref, bref);
		exit(1);
	}
	if ((frad > m_vopmd.iSearchRangeForward) ||
		(brad > m_vopmd.iSearchRangeBackward)) {
		fprintf(stderr, "%s:%d(%c%d): MV file ME radii (%d,%d) exceed current range (%d,%d)\n",
			m_pchMVFileName, m_iMVLineNo, "IPBS"[type], pic, frad, brad,
			m_vopmd.iSearchRangeForward, m_vopmd.iSearchRangeBackward);
		exit(1);
	}
    CRct clip = m_rctRefVOPY0; clip.expand(0, 0, -MB_SIZE, -MB_SIZE);
    CRct fball(-frad, -frad, frad, frad);
    CRct bball(-brad, -brad, brad, brad);
	Int height = m_iNumMBY * MB_SIZE;

	for (Int y = 0; y < height; y += MB_SIZE) {
		for (Int x = 0; x < m_iVOPWidthY; x += MB_SIZE) {
			m_iMVLineNo++;
			if (fgets(line, sizeof line, m_fMVFile) == NULL) {
				fprintf(stderr, "EOF on %s at %d\n", m_pchMVFileName, m_iMVLineNo);
				exit(1);
			}
			if (sscanf(line,
"%d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d",
				&pmvf[0].iMVX, &pmvf[0].iMVY,
				&pmvf[1].iMVX, &pmvf[1].iMVY,
				&pmvf[2].iMVX, &pmvf[2].iMVY,
				&pmvf[3].iMVX, &pmvf[3].iMVY,
				&pmvf[4].iMVX, &pmvf[4].iMVY,
				&pmvb[0].iMVX, &pmvb[0].iMVY,
				&pmvb[1].iMVX, &pmvb[1].iMVY,
				&pmvb[2].iMVX, &pmvb[2].iMVY,
				&pmvb[3].iMVX, &pmvb[3].iMVY,
				&pmvb[4].iMVX, &pmvb[4].iMVY) != 20) {
				fprintf(stderr, "%s: Read error on line %d: %s\n",
					m_pchMVFileName, m_iMVLineNo, line);
				exit(1);
			}
			for (Int i = 0; i < BVOP_MV_PER_REF_PER_MB; i++) {
				pmvf[i].iMVX >>= 1;
				pmvf[i].iMVY >>= 1;
                CRct bounds(clip);
                bounds.shift(-x, -y);
                bounds.clip(fball);
                if (bounds.left   > pmvf[i].iMVX) pmvf[i].iMVX = bounds.left;
                if (bounds.right  < pmvf[i].iMVX) pmvf[i].iMVX = bounds.right;
                if (bounds.top    > pmvf[i].iMVY) pmvf[i].iMVY = bounds.top;
                if (bounds.bottom < pmvf[i].iMVY) pmvf[i].iMVY = bounds.bottom;
				pmvf[i].iHalfX = 0;
				pmvf[i].iHalfY = 0;
				pmvf[i].computeTrueMV();
				pmvb[i].iMVX >>= 1;
				pmvb[i].iMVY >>= 1;
                bounds = clip;
                bounds.shift(-x, -y);
                bounds.clip(bball);
                if (bounds.left   > pmvb[i].iMVX) pmvb[i].iMVX = bounds.left;
                if (bounds.right  < pmvb[i].iMVX) pmvb[i].iMVX = bounds.right;
                if (bounds.top    > pmvb[i].iMVY) pmvb[i].iMVY = bounds.top;
                if (bounds.bottom < pmvb[i].iMVY) pmvb[i].iMVY = bounds.bottom;
				pmvb[i].iHalfX = 0;
				pmvb[i].iHalfY = 0;
				pmvb[i].computeTrueMV();
			}
			pmvf += BVOP_MV_PER_REF_PER_MB;
			pmvb += BVOP_MV_PER_REF_PER_MB;
		}
	}
}

Void CVideoObjectEncoder::writeBVOPMVs()
{
	CMotionVector* pmvf = m_rgmv;
	CMotionVector* pmvb = m_rgmvBackward;
	//	CMBMode* pmbmd = m_rgmbmd;

	m_iMVLineNo++;
	fprintf(m_fMVFile, "Pic %d %d %d %d %d %d\n",
		m_t, (int)m_vopmd.vopPredType, m_tPastRef, m_tFutureRef,
		m_vopmd.iSearchRangeForward, m_vopmd.iSearchRangeBackward);

	for (Int iMBY = 0; iMBY < m_iNumMBY; iMBY++) {
		for (Int iMBX = 0; iMBX < m_iNumMBX; iMBX++) {
			fprintf(m_fMVFile,
"%3d %3d %3d %3d %3d %3d %3d %3d %3d %3d %3d %3d %3d %3d %3d %3d %3d %3d %3d %3d\n",
				2*pmvf[0].iMVX, 2*pmvf[0].iMVY,
				2*pmvf[1].iMVX, 2*pmvf[1].iMVY,
				2*pmvf[2].iMVX, 2*pmvf[2].iMVY,
				2*pmvf[3].iMVX, 2*pmvf[3].iMVY,
				2*pmvf[4].iMVX, 2*pmvf[4].iMVY,
				2*pmvb[0].iMVX, 2*pmvb[0].iMVY,
				2*pmvb[1].iMVX, 2*pmvb[1].iMVY,
				2*pmvb[2].iMVX, 2*pmvb[2].iMVY,
				2*pmvb[3].iMVX, 2*pmvb[3].iMVY,
				2*pmvb[4].iMVX, 2*pmvb[4].iMVY);
			pmvf += BVOP_MV_PER_REF_PER_MB;
			pmvb += BVOP_MV_PER_REF_PER_MB;
		}
	}
}

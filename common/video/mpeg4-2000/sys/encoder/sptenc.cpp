/*************************************************************************

This software module was originally developed by 

	Chuang Gu (chuanggu@microsoft.com), Microsoft Corporation
	(date: Augest, 1997)

and also edited by
	David B. Shu (dbshu@hrl.com), Hughes Electronics/HRL Laboratories

and also edited by
	Yoshinori Suzuki (Hitachi, Ltd.)

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

	sptenc.cpp

Abstract:

	Sprite Encoder.

Revision History:

*************************************************************************/

#include <stdio.h>
#include <iostream.h>
#include <math.h>
#include <stdlib.h>

#include "typeapi.h"
#include "codehead.h"
#include "global.hpp"
#include "entropy/bitstrm.hpp"
#include "entropy/entropy.hpp"
#include "entropy/huffman.hpp"
#include "mode.hpp"
#include "dct.hpp"
#include "vopses.hpp"
#include "vopseenc.hpp"

#ifdef __MFC_
#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

#define new DEBUG_NEW				   
#endif // __MFC_

Void CVideoObjectEncoder::encodeSptTrajectory (Time t, const CSiteD* rgstDest, const CRct& rctWarp)
// code sprite trajactory and make reconstructed frame
{
	m_t = t;
	m_statsVOP.reset ();
	m_pbitstrmOut->reset ();
	//	cout << "\nVOP " << m_uiVOId << "\n";
	m_vopmd.vopPredType = SPRITE;
	m_rctCurrVOPY = rctWarp;
	codeVOPHead ();
#ifdef DEBUG_OUTPUT
	cout << "\t" << "Time..." << m_t << " (" << m_t/m_volmd.dFrameHz << " sec)\n";
	cout.flush ();
#endif
	if (m_iNumOfPnts > 0) {
		quantizeSptTrajectory (rgstDest, rctWarp);
		codeWarpPoints ();		
	}

	if (m_sptMode != BASIC_SPRITE)
		encodeSpritePiece (t);
	if (m_iNumOfPnts > 0) {
        if(m_iNumOfPnts==2 || m_iNumOfPnts==3)	{
			FastAffineWarp (rctWarp, rctWarp / 2, m_uiWarpingAccuracy, m_iNumOfPnts);
        }
        else	{
			CPerspective2D perspYA (m_iNumOfPnts, m_rgstSrcQ, m_rgstDstQ, m_uiWarpingAccuracy);
			warpYA (perspYA, rctWarp, m_uiWarpingAccuracy);
			CSiteD rgstSrcQUV [4], rgstDstQUV [4];
			for (Int i = 0; i < m_iNumOfPnts; i++) {
				rgstSrcQUV [i] = (m_rgstSrcQ [i] - CSiteD (0.5, 0.5)) / 2;
				rgstDstQUV [i] = (m_rgstDstQ [i] - CSiteD (0.5, 0.5)) / 2;
			}
			CPerspective2D perspUV (m_iNumOfPnts, rgstSrcQUV, rgstDstQUV, m_uiWarpingAccuracy);
			warpUV (perspUV, rctWarp / 2, m_uiWarpingAccuracy);
		}
	}

// Begin: modified by Hughes	  4/9/98
//	if((!tentativeFirstSpriteVop) &&  (m_sptMode == BASIC_SPRITE)) {
		// Transmit sprite_trasmit_mode="stop" at the first Sprite VOP.
		// This is a tentative solution for bitstream exchange of
		// normal sprite bitstreams. This part needs to be changed
		// when low latency sprites is integrated.
//		m_pbitstrmOut->putBits (0, 2, "sprite_transmit_mode");
//		tentativeFirstSpriteVop = 1;
//	}
// End: modified by Hughes	  4/9/98

	m_statsVOP.nBitsStuffing += m_pbitstrmOut->flush ();
	m_statsVOL += m_statsVOP;
}

Void CVideoObjectEncoder::quantizeSptTrajectory (const CSiteD* rgstDest, CRct rctWarp)
// construction of m_rgstSrcQ and m_rgstDstQ
{
	CSiteD rgstSrcQ [4];
	rgstSrcQ [0] = CSiteD (rctWarp.left, rctWarp.top);
	rgstSrcQ [1] = CSiteD (rctWarp.right, rctWarp.top);
	rgstSrcQ [2] = CSiteD (rctWarp.left, rctWarp.bottom);
	rgstSrcQ [3] = CSiteD (rctWarp.right, rctWarp.bottom);

	for (Int i = 0; i < m_iNumOfPnts; i++) {
		// quantization	rgstSrc
		CoordD x = rgstSrcQ [i].x, y = rgstSrcQ [i].y;
		Int rnd = (x > 0) ? (Int) (2.0 * x + .5) : (Int) (2.0 * x - .5);
		m_rgstSrcQ [i].x = (CoordD) rnd / (CoordD) 2.;
		rnd = (y > 0) ? (Int) (2.0 * y + .5) : (Int) (2.0 * y - .5);
		m_rgstSrcQ [i].y = (CoordD) rnd / (CoordD) 2.;
		// quantization	rgstDest
		x = rgstDest [i].x;
		y = rgstDest [i].y;
		rnd = (x > 0) ? (Int) (2.0 * x + .5) : (Int) (2.0 * x - .5);
		m_rgstDstQ [i].x = (CoordD) rnd / (CoordD) 2.;
		rnd = (y > 0) ? (Int) (2.0 * y + .5) : (Int) (2.0 * y - .5);
		m_rgstDstQ [i].y = (CoordD) rnd / (CoordD) 2.;
	}
}

UInt CVideoObjectEncoder::codeWarpPoints ()
{
	UInt nBits = 0;
	assert (m_iNumOfPnts > 0);

	// Dest corner points MV encoding
	// construct u, v, du, dv
	Int rgiU [4], rgiV [4], rgiDU [4], rgiDV [4];
	Int j;
	for (j = 0; j < m_iNumOfPnts; j++) { 	//here both m_rgstDstQ and m_rgstSrcQ are in fulpel accuracy
		rgiU [j] = (Int) (2 * (m_rgstDstQ [j].x -  m_rgstSrcQ [j].x));
		rgiV [j] = (Int) (2 * (m_rgstDstQ [j].y -  m_rgstSrcQ [j].y));
	}
	rgiDU [0] = rgiU [0];									rgiDV [0] = rgiV [0];
	rgiDU [1] = rgiU [1] - rgiU [0];						rgiDV [1] = rgiV [1] - rgiV [0];
	rgiDU [2] = rgiU [2] - rgiU [0];						rgiDV [2] = rgiV [2] - rgiV [0];
	rgiDU [3] = rgiU [3] - rgiU [2] - rgiU [1] + rgiU [0];	rgiDV [3] = rgiV [3] - rgiV [2] - rgiV [1] + rgiV [0];
	// du, dv encoding
	Int rgiWrpPnt0Del [2];
	COutBitStream* pobstrmWrpPt0 = m_pentrencSet->m_pentrencWrpPnt -> bitstream ();
	pobstrmWrpPt0->trace (m_rgstDstQ [0], "SPRT_Warp_Point_Q");
	for (j = 0; j < m_iNumOfPnts; j++) { 
		rgiWrpPnt0Del [0] = rgiDU [j];		//make them half pel units
		rgiWrpPnt0Del [1] = rgiDV [j];		//make them half pel units
		for (UInt iXorY = 0; iXorY < 2; iXorY++)	{
			assert (rgiWrpPnt0Del [iXorY] >= -16383 && rgiWrpPnt0Del [iXorY] <= 16383);
			UInt uiAbsWrpPnt0Del = (UInt) abs (rgiWrpPnt0Del [iXorY]);
			Long lSz = 0;
			for (Int i = 14; i > 0; i--)	{
				if (uiAbsWrpPnt0Del & (1 << (i - 1)))	{
					lSz = i;
					break;
				}
			}
			nBits += m_pentrencSet->m_pentrencWrpPnt->encodeSymbol(lSz, "SPRT_Warp_Point");		// huffman encode  // GMC
 
			if (rgiWrpPnt0Del [iXorY] > 0)
				pobstrmWrpPt0->putBits (rgiWrpPnt0Del [iXorY], lSz, "SPRT_Warp_Point_Sgn");	//fix length code
			else if (rgiWrpPnt0Del[iXorY] < 0)
				pobstrmWrpPt0->putBits (~abs(rgiWrpPnt0Del [iXorY]), lSz, "SPRT_Warp_Point_Sgn");		//fix length code
			pobstrmWrpPt0->putBits (MARKER_BIT, 1, "Marker_Bit");
			nBits += lSz + 1; // GMC
		}
	}
	return nBits;
}

//low latency stuff from now on
static	CRct InitPieceRect ;
static	CRct InitPartialRect ;

// code initial sprite piece 
Void CVideoObjectEncoder::encodeInitSprite (const CRct& rctOrg)
{

	m_rctOrg = 	rctOrg;

	if (( m_rctSpt.width < rctOrg.width)  && 		  // handle the bream caption case
		( m_rctSpt.height() < rctOrg.height() ) )	{			
		m_vopmd.SpriteXmitMode = NEXT;
		encode (TRUE, -1, IVOP);
		return;
	}
	m_bSptZoom = FALSE;  // Assuming sprite is not zooming
	m_bSptHvPan = TRUE;  // Assuming sprite is panning horizontally
	m_bSptRightPiece = TRUE;  // Assuming sent right side of the sprite piece
	//	Int OldCount = m_pbitstrmOut -> getCounter (); // used to compute average macroblock bits
	//	 save sprite object information
	m_pvopcSpt =  m_pvopcOrig;
	m_rctSptQ =  m_pvopcSpt -> whereY ();

	if (m_volmd.fAUsage != RECTANGLE) {
		m_iPieceWidth = m_rctSptQ.width - 2 * EXPANDY_REF_FRAME;
		m_iPieceHeight = m_rctSptQ.height() - 2 * EXPANDY_REF_FRAME;
		m_rctCurrVOPY = CRct(0, 0, m_iPieceWidth, m_iPieceHeight);
	}

// Begin: modified by Hughes	  4/9/98
//	m_rctSptExp = m_rctCurrVOPY;  // see initialSpritePiece()
// end:  modified by Hughes	  4/9/98

	m_iNumMBX = m_rctSptExp.width / MB_SIZE; 
	m_iNumMBY = m_rctSptExp.height () / MB_SIZE;
	m_ppPieceMBstatus = new SptMBstatus* [m_iNumMBY]; 	
	m_ppUpdateMBstatus = new SptMBstatus* [m_iNumMBY];
	m_rgmbmdSpt = new CMBMode* [m_iNumMBY];
	m_rgpmbmCurr_Spt = new MacroBlockMemory** [m_iNumMBY];

// Begin: modified by Hughes	  4/9/98
	Int iNumMB = m_iNumMBX * m_iNumMBY;
	m_rgmbmdSprite = new CMBMode [iNumMB];
	Int iMBY;
//	Int iMBX, iMBY;
// end:  modified by Hughes	  4/9/98

	Int iMB, iBlk;
	Int nBlk = (m_volmd.fAUsage == EIGHT_BIT) ? 10 : 6;
	for (iMBY = 0; iMBY < m_iNumMBY; iMBY++) {
		m_ppPieceMBstatus[iMBY] = new SptMBstatus [m_iNumMBX];
		m_ppUpdateMBstatus[iMBY] = new SptMBstatus [m_iNumMBX];
		m_rgmbmdSpt[iMBY] = new CMBMode [m_iNumMBX];		
		m_rgpmbmCurr_Spt[iMBY] = new MacroBlockMemory* [m_iNumMBX];
		for (iMB = 0; iMB < m_iNumMBX; iMB++)	{
			m_rgpmbmCurr_Spt[iMBY][iMB] = new MacroBlockMemory;
			m_rgpmbmCurr_Spt[iMBY][iMB]->rgblkm = new BlockMemory [nBlk];
			for (iBlk = 0; iBlk < nBlk; iBlk++)	{
				(m_rgpmbmCurr_Spt[iMBY][iMB]->rgblkm) [iBlk] = new Int [(BLOCK_SIZE << 1) - 1];
			}
// Begin: modified by Hughes	  4/9/98
			m_ppPieceMBstatus[iMBY][iMB] = NOT_DONE;
			m_ppUpdateMBstatus[iMBY][iMB] = NOT_DONE;
// end:  modified by Hughes	  4/9/98
		}
	}

// Begin: modified by Hughes	  4/9/98
/*
	CRct rctRefFrameY ;
	if (m_volmd.fAUsage != RECTANGLE) 
	{
		m_pvopcSptP = new CVOPU8YUVBA (*m_pvopcSpt, m_volmd.fAUsage, m_rctSptPieceY);
		m_rctSptPieceY = findTightBoundingBox (m_pvopcSptP);
		delete m_pvopcSptP;
	}
	rctRefFrameY = m_rctSptPieceY ;
	m_rctCurrVOPY = rctRefFrameY;

	m_rctSptPieceY.shift(m_rctSpt.left, m_rctSpt.top);	  // absolute coordinates for VOP
	if ( m_sptMode == PIECE_UPDATE)
		m_vopmd.SpriteXmitMode  = UPDATE;

 	// encode initial sprite piece
	m_pvopcSptP = new CVOPU8YUVBA (*m_pvopcSpt, m_volmd.fAUsage, rctRefFrameY);
	m_rctRefFrameY = CRct (
		-EXPANDY_REF_FRAME, -EXPANDY_REF_FRAME, 
		EXPANDY_REF_FRAME + rctRefFrameY.width, 
		EXPANDY_REF_FRAME + rctRefFrameY.height()
	);
	m_rctRefFrameY.shift (rctRefFrameY.left, rctRefFrameY.top);
	m_rctRefFrameUV = m_rctRefFrameY.downSampleBy2 ();

	m_pvopcOrig = new CVOPU8YUVBA (m_volmd.fAUsage, m_rctRefFrameY); 
	VOPOverlay (*m_pvopcSptP, *m_pvopcOrig);
	
	m_pvopcOrig->setBoundRct (rctRefFrameY);

	// dshu: begin of modification  
	delete m_pvopcRefQ0;	
	delete m_pvopcRefQ1;
	// dshu: end of modification

	m_pvopcRefQ1 = new CVOPU8YUVBA (m_volmd.fAUsage, m_rctRefFrameY);
 	m_pvopcRefQ0 = new CVOPU8YUVBA (m_volmd.fAUsage, m_rctRefFrameY);
	m_iFrameWidthY = m_pvopcRefQ0->whereY ().width;
	m_iFrameWidthUV = m_pvopcRefQ0->whereUV ().width;
	m_iFrameWidthYxMBSize = MB_SIZE * m_pvopcRefQ0->whereY ().width; 
	m_iFrameWidthYxBlkSize = BLOCK_SIZE * m_pvopcRefQ0->whereY ().width; 
	m_iFrameWidthUVxBlkSize = BLOCK_SIZE * m_pvopcRefQ0->whereUV ().width;

	m_iPieceHeight = rctRefFrameY.height () / MB_SIZE;
	m_iPieceWidth= rctRefFrameY.width  / MB_SIZE;
	m_iPieceXoffset= (rctRefFrameY.left - m_rctSptExp.left) / MB_SIZE;
	m_iPieceYoffset= (rctRefFrameY.top - m_rctSptExp.top) / MB_SIZE;
	if (m_volmd.fAUsage == RECTANGLE) {
//		m_rctCurrVOPY = CRct (0, 0, iSessionWidthRound, iSessionHeightRound);
		m_pvopcOrig->setBoundRct (m_rctCurrVOPY);
		m_rctCurrVOPUV = m_rctCurrVOPY.downSampleBy2 ();
		
		m_rctRefVOPY0 = m_rctCurrVOPY;
		m_rctRefVOPY0.expand (EXPANDY_REFVOP);
		m_rctRefVOPUV0 = m_rctRefVOPY0.downSampleBy2 ();
		
		m_rctRefVOPY1 = m_rctRefVOPY0;
		m_rctRefVOPUV1 = m_rctRefVOPUV0;
		
		m_rctRefVOPZoom0 = m_rctRefVOPY0.upSampleBy2 ();
		m_rctRefVOPZoom1 = m_rctRefVOPY1.upSampleBy2 ();
		m_pvopcRefOrig0->setBoundRct (m_rctRefVOPY0);
		m_pvopcRefOrig1->setBoundRct (m_rctRefVOPY1);
		computeVOLConstMembers (); // these VOP members are the same for all frames
	}

	m_tPiece = -1;
	encode (TRUE, -1, IVOP);

	m_pvopcSptQ = new CVOPU8YUVBA (m_volmd.fAUsage, m_rctSptQ);  
	VOPOverlay(*m_pvopcRefQ1, *m_pvopcSptQ, 1);
 	delete m_pvopcSptP;	  m_pvopcSptP = NULL;  
	delete m_pvopcOrig;	  m_pvopcOrig  = NULL;
	delete m_pvopcRefQ0;	m_pvopcRefQ0 = NULL;
	delete m_pvopcRefQ1;

	m_pvopcRefQ1 = 	m_pvopcSptQ ; // to support	"rgpvoenc [BASE_LAYER] -> swapRefQ1toSpt (); "

	m_iNumMBX = m_rctSptExp.width / MB_SIZE; 
	m_iNumMBY = m_rctSptExp.height () / MB_SIZE;

	CMBMode* pmbmd = m_rgmbmdRef;
	IntraPredDirection* Spreddir;
	IntraPredDirection* preddir;
	// initialize  MB hole status array	
	Int iMB_left = (rctRefFrameY.left - m_rctSptExp.left) / MB_SIZE;	
	Int iMB_right = (rctRefFrameY.right - m_rctSptExp.left) / MB_SIZE;  	
	Int iMB_top = (rctRefFrameY.top - m_rctSptExp.top) / MB_SIZE;	
	Int iMB_bottom = (rctRefFrameY.bottom - m_rctSptExp.top) / MB_SIZE;
	for (iMBY = 0; iMBY < m_iNumMBY; iMBY++) 
		for (iMBX = 0; iMBX < m_iNumMBX; iMBX++) {
			if ((iMBX >= iMB_left && iMBX < iMB_right) && (iMBY >= iMB_top && iMBY < iMB_bottom)) 
			{
				m_ppPieceMBstatus[iMBY][iMBX] = PIECE_DONE;
				m_rgmbmdSpt[iMBY][iMBX] = CMBMode (*pmbmd);
				Spreddir = m_rgmbmdSpt[iMBY][iMBX].m_preddir;
				preddir = (*pmbmd).m_preddir;
				memcpy (Spreddir, preddir, 10  * sizeof (IntraPredDirection));
				pmbmd++;
			}
			else
				m_ppPieceMBstatus[iMBY][iMBX] = NOT_DONE;
			m_ppUpdateMBstatus[iMBY][iMBX] = NOT_DONE;
		}
	Int Curr = (m_pbitstrmOut -> getCounter ()) - OldCount;
	Int Num_MB = (iMB_right-iMB_left) * (iMB_bottom-iMB_top);
	m_pSptmbBits[AVGPIECEMB] = Curr / Num_MB; // average macroblock bits used by a sprite piece
	
	InitPieceRect = rctRefFrameY; 
	InitPartialRect = InitPieceRect; 
	m_rctPieceQ = InitPieceRect;
*/
#ifdef __TRACE_AND_STATS_
	m_pbitstrmOut -> trace (-1, "VOP_Time");
#endif // __TRACE_AND_STATS_
	m_rctPieceQ = CRct ( m_rctSptPieceY.left, m_rctSptPieceY.top, m_rctSptPieceY.left, m_rctSptPieceY.top);
	m_pvopcSptQ = new CVOPU8YUVBA (m_volmd.fAUsage, m_rctSptQ, m_volmd.iAuxCompCount);  
	m_pvopcRefQ1 = 	m_pvopcSptQ ; // to support	"rgpvoenc [BASE_LAYER] -> swapRefQ1toSpt (); "
	m_pbitstrmOut->reset ();	// inserted as a result of half day debug
// end:  modified by Hughes	  4/9/98

	m_tPiece = m_nFirstFrame;
	m_tUpdate = m_nFirstFrame;
	m_vopmd.SpriteXmitMode = PIECE;
	if ( m_sptMode == BASIC_SPRITE)
		m_vopmd.SpriteXmitMode  = NEXT;
}

Void CVideoObjectEncoder::initialSpritePiece (Int iSessionWidth, Int iSessionHeight)
{
	// identify initial sprite piece
	CRct rctRefFrameY, rct ;
	Bool SpriteObject =  (m_sptMode == PIECE_UPDATE) || ( m_sptMode == BASIC_SPRITE);

// Begin: modified by Hughes	  4/9/98
	m_rctSptExp = CRct (0, 0, iSessionWidth, iSessionHeight);  // m_rctSptExp is specified in relative coordinate
// end:  modified by Hughes	  4/9/98

	if (!SpriteObject) 
	{
		if (m_iNumOfPnts == 0) 
			rct =  CRct (0, 0, iSessionWidth, iSessionHeight);
		else
			rct = InitialPieceRect (0);
		rctRefFrameY = CRct (rct.left, rct.top, rct.right, (rct.bottom + 16 - rct.height()/2));	   //dshu: 12/22/97
//		rctRefFrameY = CRct (rct.left, rct.top, rct.right, (rct.bottom - rct.width/2));	 
	}
	else
		rctRefFrameY = CRct (0, 0, iSessionWidth, iSessionHeight);
	if (!SpriteObject)
		rctRefFrameY = 	PieceExpand (rctRefFrameY);
	m_rctSptPieceY =  rctRefFrameY;
}

CRct CVideoObjectEncoder::InitialPieceRect (Time ts)
{
	CRct rctSpt, rctSptQ;
	CSiteD rgstSrcQ [4];
	rgstSrcQ [0] = CSiteD (m_rctOrg.left, m_rctOrg.top);
	rgstSrcQ [1] = CSiteD (m_rctOrg.right, m_rctOrg.top);
	rgstSrcQ [2] = CSiteD (m_rctOrg.left, m_rctOrg.bottom);
	rgstSrcQ [3] = CSiteD (m_rctOrg.right, m_rctOrg.bottom);

	for (Int i = 0; i < m_iNumOfPnts; i++) {
		// quantization	rgstSrc
		CoordD x = rgstSrcQ [i].x, y = rgstSrcQ [i].y;
		Int rnd = (x > 0) ? (Int) (2.0 * x + .5) : (Int) (2.0 * x - .5);
		m_rgstSrcQ [i].x = (CoordD) rnd / (CoordD) 2.;
		rnd = (y > 0) ? (Int) (2.0 * y + .5) : (Int) (2.0 * y - .5);
		m_rgstSrcQ [i].y = (CoordD) rnd / (CoordD) 2.;
	}
									   
	rctSptQ = CornerWarp (m_pprgstDest[ts], m_rgstSrcQ); 
	rctSpt = CRct (rctSptQ.left, 0, rctSptQ.right, m_rctSpt.height());	 
	return rctSpt;		
}

CRct CVideoObjectEncoder::CornerWarp (const CSiteD* rgstDst, 
									  const CSiteD* rgstSrcQ)
{
	CSiteD stdLeftTopWarp, stdRightTopWarp, stdLeftBottomWarp, stdRightBottomWarp;
	CPerspective2D persp (m_iNumOfPnts, rgstSrcQ, rgstDst, m_uiWarpingAccuracy);
	stdLeftTopWarp = (persp * CSiteD (0, 0)).s;
	stdRightTopWarp = (persp * CSiteD (m_rctOrg.width, 0)).s;
	stdLeftBottomWarp = (persp * CSiteD (0, m_rctOrg.height())).s;
	stdRightBottomWarp = (persp * CSiteD (m_rctOrg.width, m_rctOrg.height())).s;

	CRct rctWarp (stdLeftTopWarp, stdRightTopWarp, stdLeftBottomWarp, stdRightBottomWarp);
	UInt accuracy1 = 1 << (m_uiWarpingAccuracy + 1);
	rctWarp =  rctWarp	/ accuracy1 ; 
	rctWarp.shift(-m_rctSpt.left, -m_rctSpt.top);  // rectangle is w.r.t. UL corner of sprite object

	return rctWarp;
}

Void CVideoObjectEncoder::encodeSpritePiece (Time t)
// code sprite pieces 
{
	if (m_vopmd.SpriteXmitMode  == STOP)
		return ;
	if ((m_vopmd.SpriteXmitMode  == PIECE) || (m_vopmd.SpriteXmitMode  == UPDATE)){
		m_pvopcSptQ -> shift(-m_rctSpt.left, -m_rctSpt.top);	// prepare m_pvopcSptQ to merge new piece
		UInt uiF = t + m_volmd.iTemporalRate;
		uiF = (	uiF >  m_nLastFrame) ?  m_nLastFrame :  uiF;
		UInt uiSptPieceSize = m_iBufferSize/(300/m_volmd.iTemporalRate);
		CRct rct = m_rctPieceQ;
		if (( m_sptMode != PIECE_UPDATE) && (m_tPiece == (Time) m_nFirstFrame))
		{	
// Begin: modified by Hughes	  4/9/98
			InitPieceRect = encPiece (m_rctSptPieceY);
			rct = InitPieceRect;
// End: modified by Hughes	  4/9/98
			rct = CRct (rct.left, (rct.top - 3*MB_SIZE + m_rctSpt.height()/2), rct.right, m_rctSpt.height()); // dshu: 1/4/98	 
			InitPartialRect = encPiece (rct);

			m_vopmd.SpriteXmitMode = UPDATE;
			m_rctUpdateQ = CRct ( rct.left, rct.top, rct.left, rct.bottom);
			CRct tmp;// = encPiece (rct);
			tmp = encPiece (CRct ( rct.left, m_rctPieceQ.top, rct.right, rct.top));
			m_vopmd.SpriteXmitMode = PIECE;
			m_tPiece = 	uiF;

			if (m_iNumOfPnts == 0)
				m_vopmd.SpriteXmitMode  = NEXT; 
		}
		else  {
			if (( InitPartialRect.right >= InitPieceRect.right) &&
				( InitPartialRect.left <= InitPieceRect.left))  {
				CRct rctSptQ = ZoomOrPan ();
				encSptPiece (rctSptQ, uiSptPieceSize);
				while ((m_tPiece < (Time) uiF) &&
					(m_vopmd.SpriteXmitMode  == PIECE) )
					encSptPiece (ZoomOrPan (), uiSptPieceSize);
			}
			else {
				m_bSptRightPiece = TRUE;
				encSptPiece (m_rctUpdateQ, uiSptPieceSize);
				InitPartialRect = m_rctUpdateQ;
			}
			Bool StartUpdate = (m_tPiece >= (Time) m_nLastFrame) 
				&& (m_tUpdate == (Time) m_nFirstFrame);
			if ( StartUpdate )	{
				m_vopmd.SpriteXmitMode  = UPDATE;
				if(( t < (Time) m_nLastFrame) && (m_sptMode != PIECE_OBJECT)) {
					// identify initial update piece
					rct = InitialPieceRect (t-m_nFirstFrame);
					Int left = (rct.left < m_rctSptExp.left) ? 	m_rctSptExp.left : rct.left;
					Int right = (rct.right > m_rctSptExp.right) ? 	m_rctSptExp.right : rct.right;
					InitPieceRect = CRct (left, rct.top, right, rct.bottom);
					rct = InitPieceRect ;
					if (m_sptMode == PIECE_UPDATE) {					
#if 0
						cout << "\t" << "\n start sending updates for sprite..."  << "\n";
						cout.flush ();
#endif
						m_tUpdate = -1;
						m_rctUpdateQ = CRct ( left, rct.top, left, rct.bottom);
						m_pSptmbBits[AVGUPDATEMB] = m_pSptmbBits[AVGUPDATEMB] ;
						rct = PieceSize (TRUE, uiSptPieceSize);
					}
					rct = encPiece (rct);
					InitPartialRect = m_rctUpdateQ;				
					m_tUpdate = t;
				}
				else 
					m_vopmd.SpriteXmitMode = NEXT;
			}
		}
		m_pvopcSptQ -> shift(m_rctSpt.left, m_rctSpt.top);	 // prepare m_pvopcSptQ for warping	
		m_vopmd.vopPredType = SPRITE;
		// --- begin of bug fix (David Shu)
		if (m_volmd.fAUsage != RECTANGLE)
		{
			CRct r1 =  m_rctSptQ;
			m_rctSptQ = m_rctSptExp;
			m_rctCurrVOPY  = CRct(0,0, m_rctSptQ.width, m_rctSptQ.height());
			m_rctCurrVOPUV = m_rctCurrVOPY.downSampleBy2 ();
			m_rctRefFrameY = CRct (-EXPANDY_REF_FRAME, -EXPANDY_REF_FRAME,
				EXPANDY_REF_FRAME + m_rctSptQ.width, EXPANDY_REF_FRAME + m_rctSptQ.height());
			m_rctRefFrameUV = m_rctRefFrameY.downSampleBy2 ();
			setRefStartingPointers ();
			m_iFrameWidthY = m_rctRefFrameY.width;
			m_iFrameWidthUV = m_rctRefFrameUV.width;
			m_iFrameWidthYxMBSize = MB_SIZE * m_iFrameWidthY;
			m_iFrameWidthYxBlkSize = BLOCK_SIZE * m_iFrameWidthY;
			m_iFrameWidthUVxBlkSize = BLOCK_SIZE * m_iFrameWidthUV;
			m_iOffsetForPadY = m_rctRefFrameY.offset (m_rctCurrVOPY.left, m_rctCurrVOPY.top);
			m_iOffsetForPadUV = m_rctRefFrameUV.offset (m_rctCurrVOPUV.left,m_rctCurrVOPUV.top);
			padSprite() ;  //    final padding just before warping

			m_rctSptQ = r1;
		}
		// --- end of bug fix
	}
	
	if (m_vopmd.SpriteXmitMode  != STOP){
		SptXmitMode  PieceXmitMode = m_vopmd.SpriteXmitMode;
		if (m_vopmd.SpriteXmitMode  == NEXT )
		{
			PieceXmitMode = STOP ;
			m_vopmd.SpriteXmitMode  = STOP;
			m_iNumMBX = m_rctSptExp.width / MB_SIZE; 
			m_iNumMBY = m_rctSptExp.height () / MB_SIZE;
			Int nBlk = (m_volmd.fAUsage == EIGHT_BIT) ? 10 : 6;
			for (Int iMBY = 0; iMBY < m_iNumMBY; iMBY++){
				for (Int iMB = 0; iMB < m_iNumMBX; iMB++)	{
					for (Int iBlk = 0; iBlk < nBlk; iBlk++)	
						delete [] (m_rgpmbmCurr_Spt[iMBY][iMB]->rgblkm) [iBlk];

					delete [] m_rgpmbmCurr_Spt [iMBY][iMB]->rgblkm;
					delete [] m_rgpmbmCurr_Spt [iMBY][iMB];
				}
				delete [] m_ppPieceMBstatus[iMBY] ; 
				delete [] m_ppUpdateMBstatus[iMBY] ;  
				delete [] m_rgmbmdSpt[iMBY] ;
				delete [] m_rgpmbmCurr_Spt[iMBY];
			}
			delete [] m_ppPieceMBstatus ; 
			delete [] m_ppUpdateMBstatus ;  
			delete [] m_rgmbmdSpt ;
			delete [] m_rgpmbmCurr_Spt;
// Begin: modified by Hughes	  4/9/98
			delete [] m_rgmbmdSprite ;	   
// end:  modified by Hughes	  4/9/98
		}
		else
			m_vopmd.SpriteXmitMode  = PAUSE;
		codeVOSHead ();
		if (m_vopmd.SpriteXmitMode  != STOP)
			m_vopmd.SpriteXmitMode  = PieceXmitMode;

	}
}

Void CVideoObjectEncoder::encSptPiece (CRct rctSptQ, UInt uiSptPieceSize)
{
	UInt uiTR = m_volmd.iTemporalRate;
	CRct rctSpt = (m_vopmd.SpriteXmitMode == PIECE ) ? m_rctPieceQ : m_rctUpdateQ ; 
	CRct rctSptQNew = rctSpt;
	CRct rctPiece = PieceSize (m_bSptRightPiece, uiSptPieceSize);
	rctSpt.include(rctPiece);

	UInt uiF = (m_vopmd.SpriteXmitMode == PIECE ) ? m_tPiece : m_tUpdate;
	UInt boundary = m_nLastFrame ;	
	if ( rctSptQ.valid() && (rctSptQNew != m_rctSptExp)) {
		rctSptQ = rctSpt;
		uiF += uiTR ;  
		while ((rctSptQNew.right <= rctSptQ.right) && (rctSptQNew.left >= rctSptQ.left) 
			&& (uiF <= boundary)) {
			CRct rctWarpNew = CornerWarp (m_pprgstDest[uiF- m_nFirstFrame], m_rgstSrcQ);
			if (rctWarpNew.left < m_rctSptExp.left) rctWarpNew.left = m_rctSptExp.left;
			if (rctWarpNew.right > m_rctSptExp.right) rctWarpNew.right = m_rctSptExp.right;
			rctSptQNew.include (rctWarpNew);
			uiF = ((rctSptQNew.right <= rctSptQ.right) && (rctSptQNew.left >= rctSptQ.left))
				? (uiF + uiTR) : (uiF - uiTR);
		}
		Int left = (rctPiece.left < rctSptQNew.left) ? 	rctSptQNew.left :  rctPiece.left;
		Int right = (rctPiece.right > rctSptQNew.right) ? 	rctSptQNew.right :  rctPiece.right;
		if ((right-MB_SIZE) > left)
			rctSpt = encPiece (CRct (left, m_rctSptExp.top, right, m_rctSptExp.bottom));
	}
	else {
		uiF = boundary;
	}
	if (uiF  > boundary)		
		uiF = boundary;

	if (m_vopmd.SpriteXmitMode == PIECE )
		m_tPiece = uiF;
	if (m_vopmd.SpriteXmitMode == UPDATE ) 			
		m_tUpdate = uiF;

	if (uiF  >= boundary) {
		if (( m_sptMode == PIECE_OBJECT) || m_vopmd.SpriteXmitMode == UPDATE )
			m_vopmd.SpriteXmitMode = NEXT;
	}
}

CRct CVideoObjectEncoder::PieceSize (Bool rightpiece, UInt uiSptPieceSize)
{	

	Int right, left;

	CRct rctSptQ = (m_vopmd.SpriteXmitMode == PIECE ) ? m_rctPieceQ : m_rctUpdateQ ; 
	Int AvgBitsMb = (m_vopmd.SpriteXmitMode == PIECE ) ? m_pSptmbBits [AVGUPDATEMB] : m_pSptmbBits[AVGUPDATEMB];
	assert (AvgBitsMb != 0);	

	m_iPieceHeight = (MB_SIZE-1 + m_rctPieceQ.height ()) / MB_SIZE;
	m_iPieceWidth= (Int) (0.5 + uiSptPieceSize/(AvgBitsMb * m_iPieceHeight));

	m_iPieceWidth= (m_iPieceWidth<2)? 2 : m_iPieceWidth;

	if (rightpiece) {
		right = rctSptQ.right + MB_SIZE * m_iPieceWidth;
		if (right > m_rctSptExp.right) {
			right = m_rctSptExp.right;
		}
		if ((m_vopmd.SpriteXmitMode == UPDATE ) && (right > m_rctPieceQ.right)) {
			right = m_rctPieceQ.right;
		}
		left = rctSptQ.right;
	}
	else  {
		left = rctSptQ.left - MB_SIZE* m_iPieceWidth;
		if (left < m_rctSptExp.left) {
			left = m_rctSptExp.left ;
		}
		if ((m_vopmd.SpriteXmitMode == UPDATE ) && (left < m_rctPieceQ.left)) {
			left = m_rctPieceQ.left ;
		}
		right = rctSptQ.left;
	}
	CRct rctSptPieceY = CRct (left, m_rctSptExp.top, right, m_rctSptExp.bottom);
	if ((m_tUpdate > (Time) m_nFirstFrame) || (m_vopmd.SpriteXmitMode == PIECE )) 
		if (rightpiece) {
			if (left > (m_rctSptExp.left+MB_SIZE))
			{
				left -=  MB_SIZE;
				rctSptPieceY = CRct (left, rctSptPieceY.top, rctSptPieceY.right, rctSptPieceY.bottom);
			}
		}
		else{
			if (right < (m_rctSptExp.right - MB_SIZE))
			{
				right +=  MB_SIZE;
				rctSptPieceY = CRct (rctSptPieceY.left, rctSptPieceY.top, right, rctSptPieceY.bottom);
			}
		}
	return	rctSptPieceY;
}

CRct CVideoObjectEncoder::encPiece (CRct rctpiece)
{	
	Time ts;
	//	Int right = rctpiece.right; 
	//	Int left = rctpiece.left;
	CRct rctSpt = (m_vopmd.SpriteXmitMode == PIECE ) ? m_rctPieceQ : m_rctUpdateQ ; 

	CRct rctSptPieceY = PieceExpand (rctpiece);
//	m_rctSptPieceY = rctSptPieceY;
//	m_rctSptPieceY.shift(m_rctSpt.left, m_rctSpt.top);

	if (m_volmd.fAUsage == RECTANGLE) {
		m_rctCurrVOPY = rctSptPieceY;		
		m_rctCurrVOPUV = m_rctCurrVOPY.downSampleBy2 ();
	}

	m_iPieceWidth= rctSptPieceY.width / MB_SIZE;
	m_iPieceHeight= rctSptPieceY.height () / MB_SIZE;
		
	m_iPieceXoffset= (rctSptPieceY.left - m_rctSptExp.left) / MB_SIZE;
	m_iPieceYoffset= (rctSptPieceY.top - m_rctSptExp.top) / MB_SIZE;
	rctSpt.include(rctSptPieceY);
	codeVOSHead ();

	Int OldCount = m_pbitstrmOut -> getCounter ();

 	// encode sprite piece	
	m_pvopcSptP = new CVOPU8YUVBA (*m_pvopcSpt, m_volmd.fAUsage, rctSptPieceY);
	m_pvopcSptP -> shift (-rctSptPieceY.left, -rctSptPieceY.top);	
	CRct rctRefFrameY = m_pvopcSptP -> whereY() ;
	m_rctCurrVOPY = CRct (0, 0, rctRefFrameY.width, rctRefFrameY.height());		
	m_rctCurrVOPUV = m_rctCurrVOPY.downSampleBy2 ();

	m_rctRefFrameY = CRct (
		-EXPANDY_REF_FRAME, -EXPANDY_REF_FRAME, 
		EXPANDY_REF_FRAME + rctRefFrameY.width, EXPANDY_REF_FRAME + rctRefFrameY.height()
	);
	m_rctRefFrameUV = m_rctRefFrameY.downSampleBy2 ();
	m_pvopcOrig = new CVOPU8YUVBA (m_volmd.fAUsage, m_rctRefFrameY, m_volmd.iAuxCompCount); 
	VOPOverlay (*m_pvopcSptP, *m_pvopcOrig);

	m_pvopcOrig->setBoundRct (m_rctCurrVOPY);

	m_pvopcRefQ1 = new CVOPU8YUVBA (m_volmd.fAUsage, m_rctRefFrameY, m_volmd.iAuxCompCount);
	delete m_pvopcSptP;
	m_pvopcSptP = new CVOPU8YUVBA (*m_pvopcSptQ, m_volmd.fAUsage, rctSptPieceY);	 // extract reference
	m_pvopcSptP -> shift (-rctSptPieceY.left, -rctSptPieceY.top);
	VOPOverlay (*m_pvopcSptP, *m_pvopcRefQ1);

	if (m_vopmd.SpriteXmitMode == PIECE ) {		
		m_rctPieceQ = rctSpt;
		ts = m_tPiece;
		m_vopmd.vopPredType = IVOP;
	}
	else {
		m_rctUpdateQ = rctSpt;
		ts = m_tUpdate;
		m_vopmd.vopPredType = PVOP;
	}
		
 	m_pvopcRefQ0 = new CVOPU8YUVBA (m_volmd.fAUsage, m_rctRefFrameY, m_volmd.iAuxCompCount);
	m_iFrameWidthY = m_pvopcRefQ0->whereY ().width;
	m_iFrameWidthUV = m_pvopcRefQ0->whereUV ().width;
	m_iFrameWidthYxMBSize = MB_SIZE * m_pvopcRefQ0->whereY ().width; 
	m_iFrameWidthYxBlkSize = BLOCK_SIZE * m_pvopcRefQ0->whereY ().width; 
	m_iFrameWidthUVxBlkSize = BLOCK_SIZE * m_pvopcRefQ0->whereUV ().width;

	encode (TRUE, ts, m_vopmd.vopPredType);
//	encode (TRUE, ts, m_vopmd.SpriteXmitMode);
	
	m_pvopcRefQ1 -> shift (rctSptPieceY.left, rctSptPieceY.top);  
	VOPOverlay (*m_pvopcRefQ1, *m_pvopcSptQ, 1);
	delete m_pvopcSptP;	
	delete m_pvopcOrig;	 m_pvopcOrig = NULL ;
	delete m_pvopcRefQ0; m_pvopcRefQ0 = NULL ;	
	delete m_pvopcRefQ1; m_pvopcRefQ1 = NULL ;
 	if (m_iNumOfPnts == 0) {
		m_pvopcRefQ1 = m_pvopcSptQ;	  // to avoid	"rgpvoenc [BASE_LAYER] -> swapRefQ1toSpt (); "
	}

	Int Curr = (m_pbitstrmOut -> getCounter ()) - OldCount;	
	if (m_iNumSptMB > 0) {
		if (m_vopmd.SpriteXmitMode == PIECE ) 
			m_pSptmbBits[AVGPIECEMB] = Curr / m_iNumSptMB; // average macroblock bits used by a sprite piece	
		if (m_vopmd.SpriteXmitMode == UPDATE ) 
			m_pSptmbBits[AVGUPDATEMB] = Curr / m_iNumSptMB; // average macroblock bits used by a sprite piece 
	}
	return rctSpt;
}
CRct CVideoObjectEncoder::ZoomOrPan ()
{
	CRct rctWarpNew ;
	CRct rctWarpOld ;
	UInt uiTR = m_volmd.iTemporalRate;
	UInt uiF = (m_vopmd.SpriteXmitMode == PIECE ) ? m_tPiece : m_tUpdate;
	UInt boundary = m_nLastFrame ;

	if (uiF >= boundary)
		return rctWarpNew;		

	rctWarpNew = CornerWarp (m_pprgstDest[uiF- m_nFirstFrame], m_rgstSrcQ);
	rctWarpOld = rctWarpNew;
	// determine if sprite is zooming or panning?
	while (rctWarpNew == rctWarpOld) {
		uiF += uiTR ;
		rctWarpNew = CornerWarp (m_pprgstDest[uiF- m_nFirstFrame], m_rgstSrcQ);
	}
	if ((rctWarpNew.right > rctWarpOld.right) && (rctWarpNew.left < rctWarpOld.left))
		m_bSptZoom = TRUE;  // sprite is zooming
	else {
		m_bSptZoom = FALSE ;  // sprite is panning		 
		if (rctWarpNew.left < rctWarpOld.left)
			m_bSptRightPiece = FALSE;  // sprite is panning to the left
		else
			m_bSptRightPiece = TRUE;  // sprite is panning to the right
	}
	rctWarpNew.include (rctWarpOld);
	return rctWarpNew;		
}

Void CVideoObjectEncoder::codeVOSHead ()   
{
	m_pbitstrmOut -> putBits (m_vopmd.SpriteXmitMode, NUMBITS_SPRITE_XMIT_MODE, "Sprite_TRANSMIT_MODE");
	if (( m_vopmd.SpriteXmitMode != STOP) &&( m_vopmd.SpriteXmitMode != PAUSE)) {
		// Sprite piece overhead code
	  //		static UInt uiNumBitsStart = 4* NUMBITS_SPRITE_MB_OFFSET + NUMBITS_SPRITE_XMIT_MODE + NUMBITS_VOP_QUANTIZER;
		Int stepToBeCoded = (m_vopmd.SpriteXmitMode == UPDATE) ? m_vopmd.intStep : m_vopmd.intStepI;
		m_pbitstrmOut -> putBits (stepToBeCoded, NUMBITS_VOP_QUANTIZER, "sprite_Piece_Quantier");
		m_pbitstrmOut -> putBits (m_iPieceWidth, (UInt) NUMBITS_SPRITE_MB_OFFSET, "sprite_Piece_width");		//plus 9 bits
		m_pbitstrmOut -> putBits (m_iPieceHeight, (UInt) NUMBITS_SPRITE_MB_OFFSET, "sprite_Piece_height");		//plus 9 bits
// Begin: modified by Hughes	  4/9/98
		m_pbitstrmOut -> putBits (MARKER_BIT, MARKER_BIT, "Marker_Bit");
// End: modified by Hughes	  4/9/98
		m_pbitstrmOut -> putBits (m_iPieceXoffset, (UInt) NUMBITS_SPRITE_MB_OFFSET, "sprite_Piece_Xoffset");		//plus 9 bits
		m_pbitstrmOut -> putBits (m_iPieceYoffset, (UInt) NUMBITS_SPRITE_MB_OFFSET, "sprite_Piece_Yoffset");		//plus 9 bits
	}
}

CRct CVideoObjectEncoder::findTightBoundingBox (CVOPU8YUVBA* vopuc)
{
	const CRct& rctOrig =  vopuc -> whereY ();
	CoordI left = rctOrig.right - 1;
	CoordI top = rctOrig.bottom - 1;
	CoordI right = rctOrig.left;
	CoordI bottom = rctOrig.top;
	const PixelC* ppxlcBY = vopuc->pixelsBY ();
	for (CoordI y = rctOrig.top; y < rctOrig.bottom; y++) {
		for (CoordI x = rctOrig.left; x < rctOrig.right; x++) {
			if (*ppxlcBY != transpValue) {
				left = min (left, x);
				top = min (top, y);
				right = max (right, x);
				bottom = max (bottom, y);
			}								
			ppxlcBY++;
		}
	}
	right++;
	bottom++;
	if (left % 2 != 0)
		left--;
	if (top % 2 != 0)
		top--;

	return PieceExpand (CRct (left, top, right, bottom));
}

//  extend the rctOrg size to multiples of MBSize w.r.t. m_rctSptExp 
CRct CVideoObjectEncoder::PieceExpand (const CRct& rctOrg)
{
// Begin: modified by Hughes	  4/9/98
//	Int left = rctOrg.left ;	
//	Int right = rctOrg.right ;	
//	Int top = rctOrg.top ;	
//	Int bottom = rctOrg.bottom ;

	Int left = (rctOrg.left < m_rctSptExp.left) ? m_rctSptExp.left : rctOrg.left;	
	Int right = (rctOrg.right > m_rctSptExp.right) ? m_rctSptExp.right : rctOrg.right;
	Int top = (rctOrg.top < m_rctSptExp.top) ? m_rctSptExp.top : rctOrg.top;	
	Int bottom = (rctOrg.bottom > m_rctSptExp.bottom) ? m_rctSptExp.bottom : rctOrg.bottom;
// End: modified by Hughes	  4/9/98

	Int iMod = (left - m_rctSptExp.left) % MB_SIZE;
	if (iMod > 0)
		left -= iMod;
	iMod = (right - left) % MB_SIZE;
	if (iMod > 0)
		right += MB_SIZE - iMod;
	iMod = (top - m_rctSptExp.top) % MB_SIZE;
	if (iMod > 0)
		top -= iMod;
	iMod = (bottom - top) % MB_SIZE;
	if (iMod > 0)
		bottom += MB_SIZE - iMod;
	// for update pieces, skip the entire row of MBs if the cooresponding object piece if missing
	if (m_vopmd.SpriteXmitMode == UPDATE) {
		Int iMBX, iMBX_left, iMBX_right ;
		Int iMBY, iMBY_top, iMBY_bottom ;
		iMBX_left =  (left - m_rctSptExp.left) / MB_SIZE;
		iMBX_right =  (right - m_rctSptExp.left) / MB_SIZE;	
		iMBY_top =  (top - m_rctSptExp.top) / MB_SIZE;
		iMBY_bottom =  (bottom - m_rctSptExp.top) / MB_SIZE;
		for (iMBY = iMBY_top; iMBY < iMBY_bottom; iMBY++)	{
			for (iMBX = iMBX_left; iMBX < iMBX_right; iMBX++)	{
				if ( m_ppPieceMBstatus[iMBY][iMBX] != PIECE_DONE) {
					top = top  +  MB_SIZE;
					break;				
				}
			}
		}
		for (iMBY = iMBY_bottom-1; iMBY > iMBY_top; iMBY--)	{
			for (iMBX = iMBX_left; iMBX < iMBX_right; iMBX++)	{
				if ( m_ppPieceMBstatus[iMBY][iMBX] != PIECE_DONE) {
					bottom = bottom  -  MB_SIZE;
					break;				
				}
			}
		}
	}

	return CRct(left, top, right, bottom);
}


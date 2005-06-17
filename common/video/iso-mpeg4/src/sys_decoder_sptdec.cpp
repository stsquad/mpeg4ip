/*************************************************************************

This software module was originally developed by 

	Chuang Gu (chuanggu@microsoft.com), Microsoft Corporation
	(date: Augest, 1997)

and also edited by
	David B. Shu (dbshu@hrl.com), Hughes Electronics/HRL Laboratories

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

	sptdec.cpp

Abstract:

	Sprite decoder.

Revision History:

*************************************************************************/
#include <stdio.h>
//#include <fstream.h>
#ifndef _WIN32
//include <strstream.h>
#endif
#include <math.h>

#include "typeapi.h"
#include "codehead.h"
#include "bitstrm.hpp"
#include "entropy.hpp"
#include "huffman.hpp"
#include "global.hpp"
#include "mode.hpp"
#include "vopses.hpp"
#include "vopsedec.hpp"

#ifdef __MFC_
#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

#define new DEBUG_NEW				   
#endif // __MFC_

#ifdef __PC_COMPILER_
#define IOSBINARY ios::binary
#else
#define IOSBINARY ios::bin
#endif // __PC_COMPILER_

Int CVideoObjectDecoder::decodeSpt ()
{
	//David Shu: please insert your stuff in this function
	assert (m_vopmd.vopPredType == SPRITE);
	if (m_iNumOfPnts > 0)
		decodeWarpPoints ();

// Begin: modified by Hughes	  4/9/98
	if (m_sptMode != BASIC_SPRITE)
		decodeSpritePieces  ();
// End: modified by Hughes	  4/9/98

	if (m_iNumOfPnts > 0) {
		CRct rctWarp = (m_volmd.fAUsage != RECTANGLE)? m_rctCurrVOPY : CRct (0, 0, m_ivolWidth, m_ivolHeight);
        if(m_iNumOfPnts==2 || m_iNumOfPnts==3) {
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
	return TRUE;
}

Void CVideoObjectDecoder::decodeWarpPoints ()
{
	assert (m_iNumOfPnts > 0);

	// Dest corner points MV decoding
	Int rgiU [4], rgiV [4], rgiDU [4], rgiDV [4];
	Int rgiWrpPnt0Del [2];
	CInBitStream* pibstrmWrpPt0 = m_pentrdecSet->m_pentrdecWrpPnt -> bitstream ();
	Int j;
	UInt iXorY;
	Long lSz;
	Int iSign;
	for (j = 0; j < m_iNumOfPnts; j++) { 
		for (iXorY = 0; iXorY < 2; iXorY++)	{
			lSz = m_pentrdecSet->m_pentrdecWrpPnt->decodeSymbol();
			//			if (lSz > 0) { // wmay - no size, no check
			  iSign = pibstrmWrpPt0->peekBits (1);	//get the sign
			  if (iSign == 1)	
			    rgiWrpPnt0Del[iXorY] = pibstrmWrpPt0->getBits (lSz);
			  else	{
			    Int iAbsWrpPnt0Del = ~(pibstrmWrpPt0->getBits (lSz));	//get the magnitude
			    Int iMask = ~(0xFFFFFFFF << lSz);	//mask out the irrelavent bits
			    rgiWrpPnt0Del[iXorY] = -1 * (iAbsWrpPnt0Del & iMask); //masking and signing
			  }
			  assert (rgiWrpPnt0Del[iXorY] >= -16383 && rgiWrpPnt0Del[iXorY] <= 16383);
			  //} 
			  
			Int iMarker = pibstrmWrpPt0->getBits(1);
			assert(iMarker==1);
		}
		rgiDU [j] = rgiWrpPnt0Del[0];
		rgiDV [j] = rgiWrpPnt0Del[1];
	}

	// create Src VOP bounding box
	switch (m_iNumOfPnts) {
	case 1: 
		m_rgstSrcQ [0] = CSiteD (m_rctCurrVOPY.left, m_rctCurrVOPY.top);
		break;
	case 2: 
		m_rgstSrcQ [0] = CSiteD (m_rctCurrVOPY.left, m_rctCurrVOPY.top);
		m_rgstSrcQ [1] = CSiteD (m_rctCurrVOPY.right, m_rctCurrVOPY.top);
		break;
	case 3:
		m_rgstSrcQ [0] = CSiteD (m_rctCurrVOPY.left, m_rctCurrVOPY.top);
		m_rgstSrcQ [1] = CSiteD (m_rctCurrVOPY.right, m_rctCurrVOPY.top);
		m_rgstSrcQ [2] = CSiteD (m_rctCurrVOPY.left, m_rctCurrVOPY.bottom);
		break;
	case 4:
		m_rgstSrcQ [0] = CSiteD (m_rctCurrVOPY.left, m_rctCurrVOPY.top);
		m_rgstSrcQ [1] = CSiteD (m_rctCurrVOPY.right, m_rctCurrVOPY.top);
		m_rgstSrcQ [2] = CSiteD (m_rctCurrVOPY.left, m_rctCurrVOPY.bottom);
		m_rgstSrcQ [3] = CSiteD (m_rctCurrVOPY.right, m_rctCurrVOPY.bottom);
		break;
	}

	// Dest corner points reconstruction 
	rgiU [0] = rgiDU [0];									rgiV [0] = rgiDV [0];
	rgiU [1] = rgiDU [1] + rgiU [0];						rgiV [1] = rgiDV [1] + rgiV [0];
	rgiU [2] = rgiDU [2] + rgiU [0];						rgiV [2] = rgiDV [2] + rgiV [0];
	rgiU [3] = rgiDU [3] + rgiU [2] + rgiU [1] - rgiU [0];	rgiV [3] = rgiDV [3] + rgiV [2] + rgiV [1] - rgiV [0];
	for (j = 0; j < m_iNumOfPnts; j++) {
		m_rgstDstQ [j].x = (rgiU [j] + 2 * m_rgstSrcQ [j].x) / 2.0;
		m_rgstDstQ [j].y = (rgiV [j] + 2 * m_rgstSrcQ [j].y) / 2.0;
	}
}

Void CVideoObjectDecoder::decodeInitSprite ()
// decode initial sprite piece and initialize MB status array
{
	Int iMod = m_rctSpt.width % MB_SIZE;
	Int iSptWidth = (iMod > 0) ? m_rctSpt.width + MB_SIZE - iMod : m_rctSpt.width;
	iMod = m_rctSpt.height () % MB_SIZE;
	Int iSptHeight = (iMod > 0) ? m_rctSpt.height () + MB_SIZE - iMod : m_rctSpt.height ();

// Begin: modified by Hughes	  4/9/98
//	m_rctSptExp = m_pvopcRefQ1->whereY();
	if (m_sptMode == BASIC_SPRITE)
	{
        m_rctCurrVOPY = CRct (0, 0, iSptWidth, iSptHeight);
        m_rctCurrVOPUV = m_rctCurrVOPY.downSampleBy2 ();
        decode ();
		if (m_iNumOfPnts > 0) {
			swapRefQ1toSpt ();
			changeSizeofCurrQ (m_rctDisplayWindow);
		}
		m_pbitstrmIn->flush (8);
		return ;
	}
// end:  modified by Hughes	  4/9/98

	m_rctSptQ =  CRct (0, 0, iSptWidth, iSptHeight) ;  //	 save sprite object information
	Int iNumMBX = iSptWidth / MB_SIZE;	//for the whole sprite
	Int iNumMBY = iSptHeight / MB_SIZE;//for the whole sprite
	m_ppPieceMBstatus = new SptMBstatus* [iNumMBY]; 	
	m_ppUpdateMBstatus = new SptMBstatus* [iNumMBY];

// dshu: [v071]  Begin of modification
	Int iNumMB = iNumMBX * iNumMBY;
	m_rgmbmdSprite = new CMBMode [iNumMB];
// dshu: [v071] end of modification

	m_rgmbmdSpt = new CMBMode* [iNumMBY]; 
	m_rgpmbmCurr_Spt = new MacroBlockMemory** [iNumMBY];
		
// Begin: modified by Hughes	  4/9/98
	Int iMBY;
//	Int iMBX, iMBY;
// end:  modified by Hughes	  4/9/98

	Int iMB, iBlk;
	Int nBlk = (m_volmd.fAUsage == EIGHT_BIT) ? 10 : 6;	
	// Allocate MB hole status array and reconstructed MB  array
	for (iMBY = 0; iMBY < iNumMBY; iMBY++) {
		m_ppPieceMBstatus[iMBY] = new SptMBstatus [iNumMBX];
		m_ppUpdateMBstatus[iMBY] = new SptMBstatus [iNumMBX];
		m_rgmbmdSpt[iMBY] = new CMBMode [iNumMBX];		
		m_rgpmbmCurr_Spt[iMBY] = new MacroBlockMemory* [iNumMBX];
		for (iMB = 0; iMB < iNumMBX; iMB++)	{
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
	CRct rctSptQ48 = m_pvopcRefQ1->whereY();
	m_pvopcSptQ = new CVOPU8YUVBA (m_volmd.fAUsage, rctSptQ48);
	m_pvopcSptQ -> shift (m_rctSpt.left, m_rctSpt.top);	// change to absolute value in sprite coordinates
	m_pbitstrmIn->flush ();
	m_tPiece = 0;
	m_rctCurrVOPY = m_rctSptQ;
	m_rctCurrVOPUV = m_rctCurrVOPY.downSampleBy2 ();
	if (m_volmd.fAUsage != RECTANGLE)	{
		m_iNumMBRef = iNumMB;
		m_iNumMBXRef = iNumMBX;
		m_iNumMBYRef = iNumMBY;
		m_iOffsetForPadY = m_rctRefFrameY.offset (m_rctCurrVOPY.left, m_rctCurrVOPY.top);
		m_iOffsetForPadUV = m_rctRefFrameUV.offset (m_rctCurrVOPUV.left, m_rctCurrVOPUV.top);
		m_rctRefVOPY1 = m_rctCurrVOPY;
		m_rctRefVOPY1.expand (EXPANDY_REFVOP);
		m_rctRefVOPUV1 = m_rctCurrVOPUV;
		m_rctRefVOPUV1.expand (EXPANDUV_REFVOP);
		m_pvopcRefQ1->setBoundRct (m_rctRefVOPY1);
	}
/*
	m_tPiece = -1;
	decode ();

// dshu: begin of modification
	// delete the following statement
//	m_rctSptPieceY = m_rctCurrVOPY;	//for non-rectangular only becomes known now
// dshu: end of modification
	m_tPiece = 0;
	m_iStepI = m_vopmd.intStepI ;
	// ensure byte alignment; 9/16/97
	m_pbitstrmIn->flush (8);
	  		 
// dshu: begin of modification
//	m_pvopcSptQ = new CVOPU8YUVBA (*m_pvopcRefQ1);
//	m_pvopcSptQ -> shift (m_rctSptPieceY.left, m_rctSptPieceY.top); 
	m_pvopcSptQ = new CVOPU8YUVBA (m_volmd.fAUsage, m_rctSptExp);
	m_pvopcSptQ -> shift (m_rctSpt.left, m_rctSpt.top);	  // change to absolute value in sprite coordinates
	PiecePut(*m_pvopcRefQ1, *m_pvopcSptQ, m_rctSptPieceY);
// dshu: end of modification


//m_pvopcSptQ->vdlDump("c:\\SptQ.vdl");

//wchen: temporally out due to memory problem

	CMBMode* pmbmd = m_rgmbmdRef;
	IntraPredDirection* Spreddir;
	IntraPredDirection* preddir;
	// initialize  MB hole status array

// dshu: begin of modification
//	Int iMB_left = m_rctSptPieceY.left / MB_SIZE;	
//	Int iMB_right = m_rctSptPieceY.right / MB_SIZE;	
//	Int iMB_top = m_rctSptPieceY.top / MB_SIZE;	
//	Int iMB_bottom = m_rctSptPieceY.bottom / MB_SIZE;

	// MBs are based on vector, not on absolute value in sprite coordinates
	Int iMB_left = m_iPieceXoffset;	
	Int iMB_right = m_iPieceXoffset + m_iPieceWidth;	
	Int iMB_top = m_iPieceYoffset;	
	Int iMB_bottom = m_iPieceYoffset + m_iPieceHeight;
// dshu: end of modification

	for (iMBY = 0; iMBY < iNumMBY; iMBY++) 
		for (iMBX = 0; iMBX < iNumMBX; iMBX++) {
			if ((iMBX >= iMB_left && iMBX < iMB_right) && (iMBY >= iMB_top && iMBY < iMB_bottom)) 
			{
				m_ppPieceMBstatus[iMBY][iMBX] = PIECE_DONE;
				m_rgmbmdSpt[iMBY][iMBX] = CMBMode (*pmbmd);
				m_rgmbmdSprite[iMBX + iNumMBX * iMBY] = CMBMode (*pmbmd);  // dshu: [v071]  added to store mbmd array for sprite
				Spreddir = m_rgmbmdSpt[iMBY][iMBX].m_preddir;
				preddir = (*pmbmd).m_preddir;
				memcpy (Spreddir, preddir, 10  * sizeof (IntraPredDirection));
				pmbmd++;
			}
			else
				m_ppPieceMBstatus[iMBY][iMBX] = NOT_DONE;
			m_ppUpdateMBstatus[iMBY][iMBX] = NOT_DONE;
		}
*/
// End: modified by Hughes	  4/9/98

	m_oldSptXmitMode = PIECE;
	m_vopmd.SpriteXmitMode = PAUSE;
}

Void CVideoObjectDecoder::decodeSpritePieces  ()
// decode sprite pieces
{
	if ( m_vopmd.SpriteXmitMode == STOP)
		return;

	m_vopmd.SpriteXmitMode = m_oldSptXmitMode;
	CRct rctCurrVOPY = m_rctCurrVOPY;	//caching for warping
	do	{
		decodeOneSpritePiece ();
	} while (( m_vopmd.SpriteXmitMode != STOP) && ( m_vopmd.SpriteXmitMode != PAUSE));

// dshu: [v071]  Begin of modification
	m_rctCurrVOPY = rctCurrVOPY;  //    restore for warping
	if (m_volmd.fAUsage != RECTANGLE)
		padSprite()	;  //    final padding just before warping 
// dshu: [v071] end of modification

	if ( m_vopmd.SpriteXmitMode == STOP)	{

// dshu: begin of modification
		Int iNumMBX = m_rctSptQ.width / MB_SIZE; 
		Int iNumMBY = m_rctSptQ.height () / MB_SIZE;
		Int nBlk = (m_volmd.fAUsage == EIGHT_BIT) ? 10 : 6;
		for (Int iMBY = 0; iMBY < iNumMBY; iMBY++){
			for (Int iMB = 0; iMB < iNumMBX; iMB++)	{
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
		delete [] m_rgmbmdSprite ;	   // dshu: [v071] 
// dshu: end of modification
//wchen: can you just delete double points like this?
//temporarilly removed 

//		delete [] m_ppPieceMBstatus ;
//		delete [] m_ppUpdateMBstatus ;
	}
	
	//restore since it was changed during decoding to I/P
	m_vopmd.vopPredType = SPRITE;
	//restore for warping
//	m_rctCurrVOPY = rctCurrVOPY;   // dshu: [v071];   move to location of one statement before padSprite()
	// ensure byte alignment
	m_pbitstrmIn->flush (8);
}

//wchen: moved decode piece part from vopsedec.cpp -- decode ()
Int CVideoObjectDecoder::decodeOneSpritePiece ()
{
	//wchen: the part deals with stop and pause stay in decode ()
	assert (m_vopmd.SpriteXmitMode != STOP && m_vopmd.SpriteXmitMode != PAUSE);
	m_rctSptPieceY = decodeVOSHead ();
	if (m_vopmd.SpriteXmitMode == STOP || m_vopmd.SpriteXmitMode == PAUSE)
		return TRUE;
	else if (m_vopmd.SpriteXmitMode == PIECE )	//faking the paramters to fool vopmbdec
		m_vopmd.vopPredType = IVOP;
	else 
		m_vopmd.vopPredType = PVOP;

// dshu: begin of modification
//	m_pvopcRefQ1 -> shift (m_rctSptPieceY.left, m_rctSptPieceY.top);
//	PieceOverlay(*m_pvopcSptQ, *m_pvopcRefQ1,  m_rctSptPieceY);	  // extract reference
//	m_pvopcRefQ1 -> shift (-m_rctSptPieceY.left, -m_rctSptPieceY.top);	// restore the m_pvopcRefQ1 to zero offset
// dshu: end of modification


// dshu: begin of modification
	//copy to Q0 via SptP
	//	CVOPU8YUVBA* pvopcSptP = new CVOPU8YUVBA (*m_pvopcSptQ, m_volmd.fAUsage, m_rctSptPieceY);	 // extract reference
	//	m_pvopcRefQ0->overlay (*pvopcSptP);
	//	delete pvopcSptP;

	PieceGet(*m_pvopcRefQ1, *m_pvopcSptQ, m_rctSptPieceY);
// dshu: end of modification


	//always to that since the rect keeps changing
	//internally, every piece starts from (0, 0)
	m_rctCurrVOPY  = CRct(0,0, m_rctSptPieceY.width, m_rctSptPieceY.height());
	m_rctCurrVOPUV = m_rctCurrVOPY.downSampleBy2 ();
	setRefStartingPointers ();
	computeVOPMembers ();
	
	decodeVOP ();
	
	//delete ac/dc pred stuff
	Int nBlk = (m_volmd.fAUsage == EIGHT_BIT) ? 10 : 6;
	delete [] m_rgblkmCurrMB;
	m_rgblkmCurrMB = NULL;
	for (Int iMB = 0; iMB < m_iNumMBX; iMB++)	{
		for (Int iBlk = 0; iBlk < nBlk; iBlk++)	{
			delete [] (m_rgpmbmAbove [iMB]->rgblkm) [iBlk];
			delete [] (m_rgpmbmCurr  [iMB]->rgblkm) [iBlk];
		}
		delete [] m_rgpmbmAbove [iMB]->rgblkm;
		delete m_rgpmbmAbove [iMB];
		delete [] m_rgpmbmCurr [iMB]->rgblkm;
		delete m_rgpmbmCurr [iMB];
	}
	delete [] m_rgpmbmAbove;
	m_rgpmbmAbove = NULL;
	delete [] m_rgpmbmCurr;
	m_rgpmbmCurr = NULL;

	// Place the decoded sprite piece into sprite buffer.
	// Please notice that initial piece is offset by a vector like any VOP;
	// the rest of sprite pieces have zero offset, thus require shifting to be placed into buffer.
	// shift back to absolutes in sprite coordinates for overlaying	

// dshu: begin of modification
//	m_pvopcRefQ1 -> shift (m_rctSptPieceY.left, m_rctSptPieceY.top);
//	m_pvopcSptQ -> overlay (*m_pvopcRefQ1, m_rctSptPieceY);
	
	PiecePut(*m_pvopcRefQ1, *m_pvopcSptQ, m_rctSptPieceY);
//	PieceOverlay(*m_pvopcRefQ1, *m_pvopcSptQ, m_rctSptPieceY);
	// restore the m_pvopcRefQ1 to zero offset
//	m_pvopcRefQ1 -> shift (-m_rctSptPieceY.left, -m_rctSptPieceY.top);
// dshu: end of modification

	return TRUE;
}

CRct CVideoObjectDecoder::decodeVOSHead ()
{
	m_oldSptXmitMode = m_vopmd.SpriteXmitMode;
	m_vopmd.SpriteXmitMode = (SptXmitMode) m_pbitstrmIn -> getBits (NUMBITS_SPRITE_XMIT_MODE);
	if (( m_vopmd.SpriteXmitMode != STOP) &&( m_vopmd.SpriteXmitMode != PAUSE)) {
		Int stepToBeDeCoded = m_pbitstrmIn -> getBits (NUMBITS_VOP_QUANTIZER);
		if (m_vopmd.SpriteXmitMode == UPDATE) 
			m_vopmd.intStep = stepToBeDeCoded;
		else
			m_vopmd.intStepI = stepToBeDeCoded;
		m_iPieceWidth = m_pbitstrmIn -> getBits (NUMBITS_SPRITE_MB_OFFSET);
		m_iPieceHeight = m_pbitstrmIn -> getBits (NUMBITS_SPRITE_MB_OFFSET);
// Begin: modified by Hughes	  4/9/98
		/* Int iMarker = */ m_pbitstrmIn -> getBits (MARKER_BIT);
// End: modified by Hughes	  4/9/98

		m_iPieceXoffset = m_pbitstrmIn -> getBits (NUMBITS_SPRITE_MB_OFFSET);
		m_iPieceYoffset = m_pbitstrmIn -> getBits (NUMBITS_SPRITE_MB_OFFSET);
		//wchen: changed to absolute value in sprite coordinates
		Int left  = (m_iPieceXoffset * MB_SIZE) + m_rctSpt.left;
		Int top = (m_iPieceYoffset * MB_SIZE)  + m_rctSpt.top;
		Int right = left + m_iPieceWidth * MB_SIZE ;
		Int bottom = top + m_iPieceHeight * MB_SIZE ;
		return CRct (left, top, right, bottom);
	}
	else
		return CRct();
}

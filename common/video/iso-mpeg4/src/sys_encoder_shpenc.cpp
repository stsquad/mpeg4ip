/*************************************************************************

This software module was originally developed by 

	Wei-ge Chen (wchen@microsoft.com), Microsoft Corporation
	(April, 1997)
and edited by
        Wei Wu (weiwu@stallion.risc.rockwell.com) Rockwell Science Center

and also edited by
	Yoshihiro Kikuchi (TOSHIBA CORPORATION)
	Takeshi Nagai (TOSHIBA CORPORATION)
	Toshiaki Watanabe (TOSHIBA CORPORATION)
	Noboru Yamaguchi (TOSHIBA CORPORATION)

and also edited by 
	Takefumi Nagumo  (nagumo@av.crl.sony.co.jp) Sony Corporation
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

	shpenc.hpp

Abstract:

	binary shape encoder with context-based arithmatic coder

Revision History:
	Feb.01	2000: Bug fixed OBSS by Takefumi Nagumo (SONY)

*************************************************************************/

#include "typeapi.h"
#include "entropy.hpp"
#include "huffman.hpp"
#include "bitstrm.hpp"
#include "global.hpp"
#include "mode.hpp"
#include "codehead.h"
#include "cae.h"
#include "vopses.hpp"
#include "vopseenc.hpp"
#include "dct.hpp"  // HHI Schueuer: inserted for deriveSADCTRowLengths

//OBSS_SAIT_991015
#include <math.h>				
#define SIGN(n)	(n>0)?1:((n<0)?-1:0)		
//~OBSS_SAIT_991015

#ifdef __MFC_
#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

#define new DEBUG_NEW				   
#endif // __MFC_

// HHI Schueuer: inserted for SADCT 
Void CVideoObjectEncoder::deriveSADCTRowLengths(Int **piCoeffWidths, const PixelC* ppxlcMBBY, const PixelC* ppxlcCurrMBBUV, 
												const TransparentStatus *pTransparentStatus)
{
	CFwdSADCT *pfdct = (CFwdSADCT *)m_pfdct;
	const PixelC *pSrc = NULL;

	int i;
	for (i=Y_BLOCK1; i<U_BLOCK; i++) {
		if (pTransparentStatus[i] != PARTIAL) {
        	memset(piCoeffWidths[i], 0, BLOCK_SIZE * sizeof(piCoeffWidths[i][0]));
			continue;
		}
		switch (i) {
		case Y_BLOCK1: 
			pSrc = ppxlcMBBY;
			break;
		case Y_BLOCK2:	
			pSrc = ppxlcMBBY + BLOCK_SIZE;
			break;
		case Y_BLOCK3:
			pSrc = ppxlcMBBY + BLOCK_SIZE*MB_SIZE;
			break;
		case Y_BLOCK4:
			pSrc = ppxlcMBBY + BLOCK_SIZE*MB_SIZE + BLOCK_SIZE;
			break;
		}

		pfdct->getRowLength(piCoeffWidths[i], pSrc, MB_SIZE);
#ifdef _DEBUG
		for (int iy=0; iy<BLOCK_SIZE; iy++) {
        	int l = *(piCoeffWidths[i] + iy);
        	assert(l>=0 && l<=BLOCK_SIZE); 
		}
#endif		
	}
    if (pTransparentStatus[U_BLOCK] == PARTIAL) {
 		pfdct->getRowLength(piCoeffWidths[U_BLOCK], ppxlcCurrMBBUV, BLOCK_SIZE);    
#ifdef _DEBUG
		for (int iy=0; iy<BLOCK_SIZE; iy++) {
        	int l = *(piCoeffWidths[i] + iy);
        	assert(l>=0 && l<=BLOCK_SIZE); 
		}
#endif	 
    }
	else
    	memset(piCoeffWidths[U_BLOCK], 0, BLOCK_SIZE * sizeof(piCoeffWidths[i][0]));
      
#ifdef __TRACE_DECODING_
	m_ptrcer->dumpSADCTRowLengths(piCoeffWidths);
#endif
}
// end 

Int CVideoObjectEncoder::codeIntraShape (PixelC* ppxlcSrcFrm, CMBMode* pmbmd, Int iMBX, Int iMBY)
{
	m_iInverseCR = 1;
	m_iWidthCurrBAB = BAB_SIZE;

#ifdef __TRACE_AND_STATS_

	if(pmbmd->m_rgTranspStatus [0] == NONE)
		m_pbitstrmOut->trace ("ORIGINAL BAB IS ALL OPAQUE");
	else if(pmbmd->m_rgTranspStatus [0] == ALL)
		m_pbitstrmOut->trace ("ORIGINAL BAB IS ALL TRANSP");
	else
		m_pbitstrmOut->trace (m_ppxlcCurrMBBY, MB_SIZE, MB_SIZE, "ORIGINAL_BAB");
#endif // __TRACE_AND_STATS_

		Int iMBnum = VPMBnum(iMBX, iMBY);
		m_bVPNoLeft = bVPNoLeft(iMBnum, iMBX);
		m_bVPNoTop = bVPNoTop(iMBnum);
		m_bVPNoRightTop = bVPNoRightTop(iMBnum, iMBX);
		m_bVPNoLeftTop = bVPNoLeftTop(iMBnum, iMBX);

	if (pmbmd->m_rgTranspStatus [0] == NONE)
		pmbmd->m_shpmd = ALL_OPAQUE;
	else if (pmbmd->m_rgTranspStatus [0] == ALL)
		pmbmd->m_shpmd = ALL_TRANSP;
	else if (!isErrorLarge (m_ppxlcCurrMBBY, m_rgiSubBlkIndx16x16, 16, MPEG4_TRANSPARENT, pmbmd))
	{
		pmbmd->m_shpmd = ALL_TRANSP;
		if ( !isErrorLarge (m_ppxlcCurrMBBY, m_rgiSubBlkIndx16x16, 16, MPEG4_OPAQUE, pmbmd))
		{
			// both opaque and transparent are good, so count pixels to decide
			Int i,iSum=0;
			for(i=0;i<MB_SQUARE_SIZE;i++)
				iSum+=(m_ppxlcCurrMBBY[i]>0);
			if(iSum>=(MB_SQUARE_SIZE>>1))
				pmbmd->m_shpmd = ALL_OPAQUE;
		}
	}
	else if (!isErrorLarge (m_ppxlcCurrMBBY, m_rgiSubBlkIndx16x16, 16, MPEG4_OPAQUE, pmbmd))
		pmbmd->m_shpmd = ALL_OPAQUE;
	else
		pmbmd->m_shpmd = round (ppxlcSrcFrm, pmbmd);

	if(pmbmd->m_shpmd==ALL_TRANSP)
	{
#ifdef __TRACE_AND_STATS_
		m_pbitstrmOut->trace ("MB_ALL_TRANSP");
#endif // __TRACE_AND_STATS_
		copyReconShapeToMbAndRef (m_ppxlcCurrMBBY, ppxlcSrcFrm, MPEG4_TRANSPARENT);					
		return codeShapeModeIntra (ALL_TRANSP, pmbmd, iMBX, iMBY);
	}
 	else if(pmbmd->m_shpmd==ALL_OPAQUE)
	{
#ifdef __TRACE_AND_STATS_
		m_pbitstrmOut->trace ("MB_ALL_OPAQUE");
#endif // __TRACE_AND_STATS_
		copyReconShapeToMbAndRef (m_ppxlcCurrMBBY, ppxlcSrcFrm, MPEG4_OPAQUE);					
		return codeShapeModeIntra (ALL_OPAQUE, pmbmd, iMBX, iMBY);
	}
	else
	{
		// intra CAE
		assert(pmbmd->m_shpmd == INTRA_CAE);
#ifdef __TRACE_AND_STATS_
		m_pbitstrmOut->trace (m_ppxlcReconCurrBAB, TOTAL_BAB_SIZE, TOTAL_BAB_SIZE, "MB_RECON_BAB (Ignore border)");
		m_pbitstrmOut->trace (m_rgpxlcCaeSymbol, m_iWidthCurrBAB , m_iWidthCurrBAB , "MB_ENCODED_BAB");
#endif // __TRACE_AND_STATS_

		if (m_bNoShapeChg)				//must be partial
			copyReconShapeToRef (ppxlcSrcFrm, m_ppxlcReconCurrBAB, TOTAL_BAB_SIZE, BAB_BORDER);
		else
			copyReconShapeToMbAndRef (m_ppxlcCurrMBBY, ppxlcSrcFrm, m_ppxlcReconCurrBAB, TOTAL_BAB_SIZE, BAB_BORDER);
		UInt nBitsMode = codeShapeModeIntra (INTRA_CAE, pmbmd, iMBX, iMBY);			
		m_pbitstrmShapeMBOut->setBookmark ();
		UInt nBitsV = encodeCAEIntra (INTRA_CAE, VERTICAL);
		m_pbitstrmShapeMBOut->gotoBookmark ();
		UInt nBitsH = encodeCAEIntra (INTRA_CAE, HORIZONTAL);

		if (nBitsV < nBitsH)
		{
			m_pbitstrmShapeMBOut->gotoBookmark ();
			nBitsV = encodeCAEIntra (INTRA_CAE, VERTICAL);
			return nBitsV+nBitsMode;
		}
		else
			return nBitsH+nBitsMode;
	}
}

Int CVideoObjectEncoder::codeInterShape (PixelC* ppxlcSrcFrm,
										 CVOPU8YUVBA* pvopcRefQ,
										 CMBMode* pmbmd, 
										 const ShapeMode& shpmdColocatedMB,
										 const CMotionVector* pmv, 
										 CMotionVector* pmvBY, 
										 CoordI iX, CoordI iY, 
										 Int iMBX, Int iMBY)
{
	m_iInverseCR = 1;
	m_iWidthCurrBAB = BAB_SIZE;
	pmbmd->m_shpmd = UNKNOWN;
	CMotionVector mvBYD (0, 0);
	CMotionVector mvShapeMVP;

#ifdef __TRACE_AND_STATS_
	if(pmbmd->m_rgTranspStatus [0] == NONE)
		m_pbitstrmOut->trace ("ORIGINAL BAB IS ALL MPEG4_OPAQUE");
	else if(pmbmd->m_rgTranspStatus [0] == ALL)
		m_pbitstrmOut->trace ("ORIGINAL BAB IS ALL TRANSP");
	else
		m_pbitstrmOut->trace (m_ppxlcCurrMBBY, MB_SIZE, MB_SIZE, "ORIGINAL_BAB");
#endif // __TRACE_AND_STATS_

	Int iMBnum = VPMBnum(iMBX, iMBY);
	m_bVPNoLeft = bVPNoLeft(iMBnum, iMBX);
	m_bVPNoTop = bVPNoTop(iMBnum);
	m_bVPNoRightTop = bVPNoRightTop(iMBnum, iMBX);
	m_bVPNoLeftTop = bVPNoLeftTop(iMBnum, iMBX);

	if (pmbmd->m_rgTranspStatus [0] == ALL)
		pmbmd->m_shpmd = ALL_TRANSP;
	else if (!isErrorLarge (m_ppxlcCurrMBBY, m_rgiSubBlkIndx16x16, 16, MPEG4_TRANSPARENT, pmbmd))
	{
		pmbmd->m_shpmd = ALL_TRANSP;
		if ( !isErrorLarge (m_ppxlcCurrMBBY, m_rgiSubBlkIndx16x16, 16, MPEG4_OPAQUE, pmbmd))
		{
			// both opaque and transparent are good, so count pixels to decide
			Int i,iSum=0;
			for(i=0;i<MB_SQUARE_SIZE;i++)
				iSum+=(m_ppxlcCurrMBBY[i]>0);
			if(iSum>=(MB_SQUARE_SIZE>>1))
				pmbmd->m_shpmd = ALL_OPAQUE;
		}
	}
	else if(pmbmd->m_rgTranspStatus [0] == NONE)
		pmbmd->m_shpmd = ALL_OPAQUE;
	else if(!isErrorLarge (m_ppxlcCurrMBBY, m_rgiSubBlkIndx16x16, 16, MPEG4_OPAQUE, pmbmd))
		pmbmd->m_shpmd = ALL_OPAQUE;

	if(pmbmd->m_shpmd!=ALL_TRANSP )
	{
		// find motion vectors
//OBSSFIX_MODE3 
		if( m_volmd.volType == ENHN_LAYER && m_volmd.bSpatialScalability == 1 && m_volmd.iHierarchyType == 0 &&
			m_volmd.iEnhnType != 0 && m_volmd.iuseRefShape == 1 && m_vopmd.vopPredType == PVOP){
			//Set predictor of Shape coding of OBSS (simulcast shape case)
            mvShapeMVP.setToZero();
			memset((void *)m_puciPredBAB->pixels(),255, MC_BAB_SIZE*MC_BAB_SIZE);
		}else{
			mvShapeMVP = findShapeMVP (pmv, pmvBY, pmbmd, iMBX, iMBY);
			assert (mvShapeMVP.iMVX != NOT_MV && mvShapeMVP.iMVY != NOT_MV);
			//	mvShapeMVP = CMotionVector ((mvShapeMVP.iMVX * 2 + mvShapeMVP.iHalfX) / 2, 	//rounding
			//								(mvShapeMVP.iMVY * 2 + mvShapeMVP.iHalfY) / 2);	
			motionCompBY ((PixelC*) m_puciPredBAB->pixels (),
						  (PixelC*) pvopcRefQ->getPlane (BY_PLANE)->pixels (),
						  mvShapeMVP.iMVX + iX - 1,		
						  mvShapeMVP.iMVY + iY - 1);	//-1 due to 18x18 motion comp
		}
//		mvShapeMVP = findShapeMVP (pmv, pmvBY, pmbmd, iMBX, iMBY);
//		assert (mvShapeMVP.iMVX != NOT_MV && mvShapeMVP.iMVY != NOT_MV);
//		//	mvShapeMVP = CMotionVector ((mvShapeMVP.iMVX * 2 + mvShapeMVP.iHalfX) / 2, 	//rounding
//		//								(mvShapeMVP.iMVY * 2 + mvShapeMVP.iHalfY) / 2);	
//		motionCompBY ((PixelC*) m_puciPredBAB->pixels (),
//					  (PixelC*) pvopcRefQ->getPlane (BY_PLANE)->pixels (),
//					  mvShapeMVP.iMVX + iX - 1,		
//					  mvShapeMVP.iMVY + iY - 1);	//-1 due to 18x18 motion comp
//~OBSSFIX_MODE3
		if (!isErrorLarge (m_ppxlcCurrMBBY, m_rgiSubBlkIndx16x16, 16,
						  m_puciPredBAB->pixels (), m_rgiSubBlkIndx18x18, 18, pmbmd))
			// zero mvd is ok
			*pmvBY = mvShapeMVP;
		else
		{
			// do block matching
			CMotionVector mvShape;
//OBSSFIX_MODE3
			if(m_volmd.volType == ENHN_LAYER && m_volmd.bSpatialScalability == 1 && m_volmd.iHierarchyType == 0 &&
			   m_volmd.iEnhnType != 0 && m_volmd.iuseRefShape == 1 && m_vopmd.vopPredType == PVOP){
               mvShapeMVP.setToZero();
               memset((void *)m_puciPredBAB->pixels(),255, MC_BAB_SIZE*MC_BAB_SIZE);
            }else{  
				blkmatchForShape (pvopcRefQ,&mvShape, mvShapeMVP.trueMVHalfPel (), iX, iY);
				assert (mvShape.iMVX != NOT_MV && mvShape.iMVY != NOT_MV);
				*pmvBY = mvShape;
				mvBYD = mvShape - mvShapeMVP;
				motionCompBY ((PixelC*) m_puciPredBAB->pixels (),
							  (PixelC*) pvopcRefQ->getPlane (BY_PLANE)->pixels (),
							  mvShape.iMVX + iX - 1,
							  mvShape.iMVY + iY - 1);	//-1 due to 18x18 motion comp
			}

//			blkmatchForShape (pvopcRefQ,&mvShape, mvShapeMVP.trueMVHalfPel (), iX, iY);
//			assert (mvShape.iMVX != NOT_MV && mvShape.iMVY != NOT_MV);
//			*pmvBY = mvShape;
//			mvBYD = mvShape - mvShapeMVP;
//			motionCompBY ((PixelC*) m_puciPredBAB->pixels (),
//						  (PixelC*) pvopcRefQ->getPlane (BY_PLANE)->pixels (),
//						  mvShape.iMVX + iX - 1,
//						  mvShape.iMVY + iY - 1);	//-1 due to 18x18 motion comp
//~OBSSFIX_MODE3
		}
		
		// check status of motion compensated prediction BAB
		PixelC *ppxlcPredBAB = (PixelC *)m_puciPredBAB->pixels()+MC_BAB_SIZE*MC_BAB_BORDER;
		Int iXX,iYY,iOpaqueCount = 0;
		for(iYY=MC_BAB_BORDER;iYY<MC_BAB_SIZE-MC_BAB_BORDER;iYY++,ppxlcPredBAB+=MC_BAB_SIZE)
			for(iXX=MC_BAB_BORDER;iXX<MC_BAB_SIZE-MC_BAB_BORDER;iXX++)
				if(ppxlcPredBAB[iXX] == opaqueValue)
					iOpaqueCount++;

		if(iOpaqueCount==0 || isErrorLarge (m_ppxlcCurrMBBY, m_rgiSubBlkIndx16x16, 16,
			m_puciPredBAB->pixels (), m_rgiSubBlkIndx18x18, 18, pmbmd))
		{
			// MC BAB is all transparent or not acceptable, so code.
			round(ppxlcSrcFrm, pmbmd);
			if(mvBYD.isZero())
				pmbmd->m_shpmd = INTER_CAE_MVDZ;
			else
				pmbmd->m_shpmd = INTER_CAE_MVDNZ;
		}
		else if(pmbmd->m_shpmd==ALL_OPAQUE && !mvBYD.isZero())
			;
		else if(pmbmd->m_rgTranspStatus[0] == NONE && iOpaqueCount!=MB_SQUARE_SIZE)
			pmbmd->m_shpmd=ALL_OPAQUE;
		else if(mvBYD.isZero())
			pmbmd->m_shpmd = MVDZ_NOUPDT;
		else
			pmbmd->m_shpmd = MVDNZ_NOUPDT;
	}

	switch(pmbmd->m_shpmd)
	{
	case ALL_OPAQUE:
#ifdef __TRACE_AND_STATS_
		m_pbitstrmOut->trace ("MB_ALL_OPAQUE");
#endif // __TRACE_AND_STATS_
		pmvBY->iMVX = NOT_MV;
		pmvBY->iMVY = NOT_MV;
		copyReconShapeToMbAndRef (m_ppxlcCurrMBBY, ppxlcSrcFrm, MPEG4_OPAQUE);					
		return (codeShapeModeInter (ALL_OPAQUE, shpmdColocatedMB));

	case ALL_TRANSP:
#ifdef __TRACE_AND_STATS_
		m_pbitstrmOut->trace ("MB_ALL_TRANSP");
#endif // __TRACE_AND_STATS_
		pmvBY->iMVX = NOT_MV;
		pmvBY->iMVY = NOT_MV;
		copyReconShapeToMbAndRef (m_ppxlcCurrMBBY, ppxlcSrcFrm, MPEG4_TRANSPARENT);					
		return (codeShapeModeInter (ALL_TRANSP, shpmdColocatedMB));
		
	case INTER_CAE_MVDZ:
	case INTER_CAE_MVDNZ:
		{
			if (m_bNoShapeChg)
				copyReconShapeToRef (ppxlcSrcFrm, m_ppxlcReconCurrBAB, TOTAL_BAB_SIZE, BAB_BORDER);
			else
				copyReconShapeToMbAndRef (m_ppxlcCurrMBBY, ppxlcSrcFrm, m_ppxlcReconCurrBAB, TOTAL_BAB_SIZE, BAB_BORDER);
#ifdef __TRACE_AND_STATS_
			m_pbitstrmOut->trace ((PixelC *)m_puciPredBAB->pixels(), MC_BAB_SIZE, MC_BAB_SIZE, "MB_MC_PRED_BAB");
			m_pbitstrmOut->trace (m_ppxlcReconCurrBAB, TOTAL_BAB_SIZE, TOTAL_BAB_SIZE, "MB_RECON_BAB (Ignore Borders)");
			m_pbitstrmOut->trace (m_rgpxlcCaeSymbol, m_iWidthCurrBAB , m_iWidthCurrBAB , "MB_ENCODED_BAB");
#endif // __TRACE_AND_STATS_

			m_pbitstrmShapeMBOut->setBookmark ();

			// try intra
			UInt nBitsModeIntra = codeShapeModeInter (INTRA_CAE, shpmdColocatedMB);
#ifdef __TRACE_AND_STATS_
			m_pbitstrmOut->trace ("MB_CAE_INTRA_HORIZONTAL_CODING (TRIAL)");
#endif // __TRACE_AND_STATS_
			UInt nBitsHIntra = encodeCAEIntra (INTRA_CAE, HORIZONTAL);
#ifdef __TRACE_AND_STATS_
			m_pbitstrmOut->trace ("MB_CAE_INTRA_VERTICAL_CODING (TRIAL)");
#endif // __TRACE_AND_STATS_
			UInt nBitsVIntra = encodeCAEIntra (INTRA_CAE, VERTICAL);
			UInt nBitsIntra = nBitsModeIntra + min (nBitsVIntra, nBitsHIntra);

			// try inter
			UInt nBitsMV=0;
			UInt nBitsModeInter = codeShapeModeInter (pmbmd->m_shpmd, shpmdColocatedMB);
			if(pmbmd->m_shpmd==INTER_CAE_MVDNZ)
				nBitsMV = encodeMVDS (mvBYD);
#ifdef __TRACE_AND_STATS_
			m_pbitstrmOut->trace ("MB_CAE_INTER_VERTICAL_CODING (TRIAL)");
#endif // __TRACE_AND_STATS_
			UInt nBitsVInter = encodeCAEInter (pmbmd->m_shpmd, VERTICAL);
#ifdef __TRACE_AND_STATS_
			m_pbitstrmOut->trace ("MB_CAE_INTER_HORIZONTAL_CODING (TRIAL)");
#endif // __TRACE_AND_STATS_
			UInt nBitsHInter = encodeCAEInter (pmbmd->m_shpmd, HORIZONTAL);
			UInt nBitsInter = nBitsModeInter + nBitsMV + min (nBitsVInter, nBitsHInter);

			m_pbitstrmShapeMBOut->gotoBookmark ();
			
			if (nBitsInter < nBitsIntra)
			{
				// choose inter coding
#ifdef __TRACE_AND_STATS_
				m_pbitstrmOut->trace ("MB_CAE_INTER_CODING (REAL)");
				m_pbitstrmOut->trace (mvShapeMVP, "MB_SHP_MV_PRED (half pel)");
				m_pbitstrmOut->trace (*pmvBY, "MB_SHP_MV (half pel)");
				m_pbitstrmOut->trace (mvBYD, "MB_SHP_MVD (half pel)");
#endif // __TRACE_AND_STATS_
				codeShapeModeInter (pmbmd->m_shpmd, shpmdColocatedMB);
				if(pmbmd->m_shpmd==INTER_CAE_MVDNZ)
					encodeMVDS (mvBYD);
				if(nBitsVInter<nBitsHInter)
					encodeCAEInter (pmbmd->m_shpmd, VERTICAL);
				else
					encodeCAEInter (pmbmd->m_shpmd, HORIZONTAL);
				return nBitsInter;
			}
			else
			{
				// choose intra coding
#ifdef __TRACE_AND_STATS_
				m_pbitstrmOut->trace ("MB_CAE_INTRA_CODING (REAL)");
#endif // __TRACE_AND_STATS_
				pmvBY->iMVX = NOT_MV;
				pmvBY->iMVY = NOT_MV;
				pmbmd->m_shpmd = INTRA_CAE;
				codeShapeModeInter (pmbmd->m_shpmd, shpmdColocatedMB);
				if(nBitsVIntra<nBitsHIntra)
					encodeCAEIntra (pmbmd->m_shpmd, VERTICAL);
				else
					encodeCAEIntra (pmbmd->m_shpmd, HORIZONTAL);
				return nBitsIntra;
			}
		}
	case MVDZ_NOUPDT:
	case MVDNZ_NOUPDT:
		{
#ifdef __TRACE_AND_STATS_
			m_pbitstrmOut->trace ("MB_CAE_INTER_CODING_NO_UPDATE");
			m_pbitstrmOut->trace (mvShapeMVP, "MB_SHP_MV_PRED (half pel)");
			m_pbitstrmOut->trace (*pmvBY, "MB_SHP_MV (half pel)");
			m_pbitstrmOut->trace (mvBYD, "MB_SHP_MVD (half pel)");
#endif // __TRACE_AND_STATS_
			copyReconShapeToMbAndRef (m_ppxlcCurrMBBY, ppxlcSrcFrm, m_puciPredBAB->pixels (), MC_BAB_SIZE, MC_BAB_BORDER);
#ifdef __TRACE_AND_STATS_
			m_pbitstrmOut->trace ((PixelC *)m_puciPredBAB->pixels(), MC_BAB_SIZE, MC_BAB_SIZE, "MB_MC_PRED_BAB");
#endif // __TRACE_AND_STATS_
			Int nBits = codeShapeModeInter (pmbmd->m_shpmd, shpmdColocatedMB);
			if(pmbmd->m_shpmd==MVDNZ_NOUPDT)
				nBits += encodeMVDS (mvBYD);	
			return nBits;
		}
	default:
		assert(FALSE);
		return 0;
	}
}

//OBSS_SAIT_991015
//for SI coding -PVOP
Int CVideoObjectEncoder::codeSIShapePVOP (PixelC* ppxlcSrcFrm,
										 CVOPU8YUVBA* pvopcRefQ,
										 CMBMode* pmbmd, 
										 const ShapeMode& shpmdColocatedMB,
										 const CMotionVector* pmv, 
										 CMotionVector* pmvBY, 
										 CoordI iX, CoordI iY, 
										 Int iMBX, Int iMBY)
{
	m_iInverseCR = 1;
	m_iWidthCurrBAB = BAB_SIZE;
	pmbmd->m_shpssmd = UNDEFINED;
	//	CMotionVector mvBYD (0, 0);

#ifdef __TRACE_AND_STATS_
	if(pmbmd->m_rgTranspStatus [0] == NONE)
		m_pbitstrmOut->trace ("ORIGINAL BAB IS ALL MPEG4_OPAQUE");
	else if(pmbmd->m_rgTranspStatus [0] == ALL)
		m_pbitstrmOut->trace ("ORIGINAL BAB IS ALL TRANSP");
	else
		m_pbitstrmOut->trace (m_ppxlcCurrMBBY, MB_SIZE, MB_SIZE, "ORIGINAL_BAB");
#endif // __TRACE_AND_STATS_

	Int iMBnum = VPMBnum(iMBX, iMBY);
	m_bVPNoLeft = bVPNoLeft(iMBnum, iMBX);
	m_bVPNoTop = bVPNoTop(iMBnum);
	m_bVPNoRightTop = bVPNoRightTop(iMBnum, iMBX);
	m_bVPNoLeftTop = bVPNoLeftTop(iMBnum, iMBX);
	m_bVPNoRight = bVPNoRight(iMBX);
	m_bVPNoBottom = bVPNoBottom(iMBY);

	motionCompBY ((PixelC*) m_puciPredBAB->pixels (),
				  (PixelC*) pvopcRefQ->getPlane (BY_PLANE)->pixels (),
				  iX-1, 		
				  iY-1);	//-1 due to 18x18 motion comp
	pmvBY->iMVX = 0;
	pmvBY->iMVY = 0;
	if (!isErrorLarge (m_ppxlcCurrMBBY, m_rgiSubBlkIndx16x16, 16,
		m_puciPredBAB->pixels (), m_rgiSubBlkIndx18x18, 18, pmbmd))
		pmbmd->m_shpssmd = INTRA_NOT_CODED; //NO_UPDATE
	else
		pmbmd->m_shpssmd = INTRA_CODED; //CODED

	if((m_volmd.iuseRefShape ==1) && !m_volmd.bShapeOnly) {
		copyReconShapeToMbAndRef (m_ppxlcCurrMBBY, ppxlcSrcFrm, m_puciPredBAB->pixels (), MC_BAB_SIZE, MC_BAB_BORDER);
		Int nBits =0;
		return nBits;
	} 

	switch(pmbmd->m_shpssmd){
	case INTRA_CODED:
	{
	m_bNoShapeChg = TRUE;
	m_iInverseCR = 1;
	m_iWidthCurrBAB = BAB_SIZE;

	// copy data to ReconCurrBAB
	PixelC* ppxlcDst = m_ppxlcReconCurrBAB + BAB_BORDER + BAB_BORDER * TOTAL_BAB_SIZE;
	PixelC* ppxlcSrc = m_ppxlcCurrMBBY;
	Int iUnit = sizeof(PixelC); // NBIT: for memcpy
	Int i;
	for (i = 0; i < MB_SIZE; i++) {
		memcpy (ppxlcDst, ppxlcSrc, MB_SIZE*iUnit);
		ppxlcSrc += MB_SIZE;
		ppxlcDst += BAB_SIZE;
	}

	// make borders
	copyLeftTopBorderFromVOP (ppxlcSrcFrm, m_ppxlcReconCurrBAB);
	makeRightBottomBorder (	m_ppxlcReconCurrBAB, 
							TOTAL_BAB_SIZE,
							(PixelC*)((pvopcRefQ->getPlane (BY_PLANE)->pixels ())+(iY + EXPANDY_REF_FRAME) * m_iFrameWidthY + iX + EXPANDY_REF_FRAME),
							m_iFrameWidthY);

	// assign encoding buffer
	m_rgpxlcCaeSymbol = m_ppxlcReconCurrBAB;
	
	if (m_bNoShapeChg)  copyReconShapeToRef (ppxlcSrcFrm, m_ppxlcReconCurrBAB, TOTAL_BAB_SIZE, BAB_BORDER);
	else				copyReconShapeToMbAndRef (m_ppxlcCurrMBBY, ppxlcSrcFrm, m_ppxlcReconCurrBAB, TOTAL_BAB_SIZE, BAB_BORDER);

#ifdef __TRACE_AND_STATS_
	m_pbitstrmOut->trace ((PixelC *)m_puciPredBAB->pixels(), MC_BAB_SIZE, MC_BAB_SIZE, "MB_MC_PRED_BAB");
	m_pbitstrmOut->trace (m_ppxlcReconCurrBAB, TOTAL_BAB_SIZE, TOTAL_BAB_SIZE, "MB_RECON_BAB (Ignore Borders)");
	m_pbitstrmOut->trace (m_rgpxlcCaeSymbol, m_iWidthCurrBAB , m_iWidthCurrBAB , "MB_ENCODED_BAB");
#endif // __TRACE_AND_STATS_

//OBSS shape mode coding
	UInt nBitsModeIntra=0;
	nBitsModeIntra = codeShapeModeSSIntra (INTRA_CODED, shpmdColocatedMB);

#ifdef __TRACE_AND_STATS_
			m_pbitstrmOut->trace ("INTRA_CODED_CODING");
#endif // __TRACE_AND_STATS_
			UInt nBitsSIIntra = encodeSIIntra (INTRA_CODED, (pvopcRefQ->getPlane (BY_PLANE)->m_pbHorSamplingChk)+iX, (pvopcRefQ->getPlane (BY_PLANE)->m_pbVerSamplingChk)+iY);
#ifdef __TRACE_AND_STATS_
			m_pbitstrmOut->trace ("SI_INTRA_CODING");
#endif // __TRACE_AND_STATS_
			UInt nBitsIntra = nBitsModeIntra + nBitsSIIntra;

	return nBitsIntra;
	}
	case INTRA_NOT_CODED: 
	{
#ifdef __TRACE_AND_STATS_
			m_pbitstrmOut->trace ("SPATIAL_SCALABLE_SHAPE_NO_UPDATE");
			m_pbitstrmOut->trace (*pmvBY, "MB_SHP_MV (half pel)");
#endif // __TRACE_AND_STATS_

		copyReconShapeToMbAndRef (m_ppxlcCurrMBBY, ppxlcSrcFrm, m_puciPredBAB->pixels (), /*BAB_SIZE-BAB_BORDER*2*/MC_BAB_SIZE, /*0*/MC_BAB_BORDER);

#ifdef __TRACE_AND_STATS_
		m_pbitstrmOut->trace ((PixelC *)m_puciPredBAB->pixels(), MC_BAB_SIZE, MC_BAB_SIZE, "MB_MC_PRED_BAB_SS");
#endif // __TRACE_AND_STATS_
//OBSS shape mode coding
		Int nBits=0;
		nBits = codeShapeModeSSIntra (INTRA_NOT_CODED, shpmdColocatedMB);
		return nBits;
	} 
	default:
		assert(FALSE);
		return 0;
	}
}

//for BVOP SI shape coding
Int CVideoObjectEncoder::codeSIShapeBVOP (PixelC* ppxlcSrcFrm,
										 CVOPU8YUVBA* pvopcRefQ0,	//previous VOP
										 CVOPU8YUVBA* pvopcRefQ1,	//lower reference VOP
										 CMBMode* pmbmd, 
										 const ShapeMode& shpmdColocatedMB,
										 const CMotionVector* pmv, 
										 CMotionVector* pmvBY, 
										 CMotionVector* pmvBaseBY, 
										 CoordI iX, CoordI iY, 
										 Int iMBX, Int iMBY)
{

	UInt nBitsIntra=0;
	m_iInverseCR = 1;
	m_iWidthCurrBAB = BAB_SIZE;
	pmbmd->m_shpssmd = UNDEFINED;
	//	CMotionVector mvBYD (0, 0);

#ifdef __TRACE_AND_STATS_
	if(pmbmd->m_rgTranspStatus [0] == NONE)
		m_pbitstrmOut->trace ("ORIGINAL BAB IS ALL MPEG4_OPAQUE");
	else if(pmbmd->m_rgTranspStatus [0] == ALL)
		m_pbitstrmOut->trace ("ORIGINAL BAB IS ALL TRANSP");
	else
		m_pbitstrmOut->trace (m_ppxlcCurrMBBY, MB_SIZE, MB_SIZE, "ORIGINAL_BAB");
#endif // __TRACE_AND_STATS_

	Int iMBnum = VPMBnum(iMBX, iMBY);
	m_bVPNoLeft = bVPNoLeft(iMBnum, iMBX);
	m_bVPNoTop = bVPNoTop(iMBnum);
	m_bVPNoRightTop = bVPNoRightTop(iMBnum, iMBX);
	m_bVPNoLeftTop = bVPNoLeftTop(iMBnum, iMBX);
	m_bVPNoRight = bVPNoRight(iMBX);
	m_bVPNoBottom = bVPNoBottom(iMBY);

	// copy data to ReconCurrBAB
	PixelC* ppxlcDst = m_ppxlcReconCurrBAB + BAB_BORDER + BAB_BORDER * TOTAL_BAB_SIZE;
	PixelC* ppxlcSrc = m_ppxlcCurrMBBY;
	Int iUnit = sizeof(PixelC); // NBIT: for memcpy
	Int i;
	for (i = 0; i < MB_SIZE; i++) {
		memcpy (ppxlcDst, ppxlcSrc, MB_SIZE*iUnit);
		ppxlcSrc += MB_SIZE;
		ppxlcDst += BAB_SIZE;
	}
	motionCompLowerBY ((PixelC*) m_puciPredBAB->pixels (),
				  (PixelC*) pvopcRefQ1->getPlane (BY_PLANE)->pixels (),
				  iX-1, 		
				  iY-1);	//-1 due to 18x18 motion comp
	
	// make borders
	copyLeftTopBorderFromVOP (ppxlcSrcFrm, m_ppxlcReconCurrBAB);
	makeRightBottomBorder (	m_ppxlcReconCurrBAB, TOTAL_BAB_SIZE,
							(PixelC*)((pvopcRefQ1->getPlane (BY_PLANE)->pixels ())+(iY + EXPANDY_REF_FRAME) * m_iFrameWidthY + iX + EXPANDY_REF_FRAME),
							m_iFrameWidthY);	

	// assign encoding buffer
	m_rgpxlcCaeSymbol = m_ppxlcReconCurrBAB;

	if((m_volmd.iuseRefShape ==1) && !m_volmd.bShapeOnly) {
		copyReconShapeToMbAndRef (m_ppxlcCurrMBBY, ppxlcSrcFrm, m_puciPredBAB->pixels (), MC_BAB_SIZE, MC_BAB_BORDER);
		Int nBits =0;
		return nBits;
	} 

	if (!isErrorLarge (m_ppxlcCurrMBBY, m_rgiSubBlkIndx16x16, 16,
		m_puciPredBAB->pixels (), m_rgiSubBlkIndx18x18, 18, pmbmd)){
		if(!(shpmdColocatedMB == 0 || shpmdColocatedMB == 1)){
			if (pmvBaseBY->iMVX == NOT_MV && pmvBaseBY->iMVY == NOT_MV ){
				pmvBaseBY->iMVX = 0;
				pmvBaseBY->iMVY = 0;
			}

			motionCompBY ((PixelC*) m_puciPredBAB->pixels (),
						  (PixelC*) pvopcRefQ0->getPlane (BY_PLANE)->pixels (),
						  (((pmvBaseBY->iMVX)*m_volmd.ihor_sampling_factor_n_shape+(SIGN(pmvBaseBY->iMVX))*m_volmd.ihor_sampling_factor_m_shape/2)/m_volmd.ihor_sampling_factor_m_shape) + iX - 1,		
						  (((pmvBaseBY->iMVY)*m_volmd.iver_sampling_factor_n_shape+(SIGN(pmvBaseBY->iMVY))*m_volmd.iver_sampling_factor_m_shape/2)/m_volmd.iver_sampling_factor_m_shape) + iY - 1);	//-1 due to 18x18 motion comp

			if (!isErrorLarge (m_ppxlcCurrMBBY, m_rgiSubBlkIndx16x16, 16,
					m_puciPredBAB->pixels (), m_rgiSubBlkIndx18x18, 18, pmbmd)){
					pmvBY->iMVX = (((pmvBaseBY->iMVX)*m_volmd.ihor_sampling_factor_n_shape+(SIGN(pmvBaseBY->iMVX))*m_volmd.ihor_sampling_factor_m_shape/2)/m_volmd.ihor_sampling_factor_m_shape);
					pmvBY->iMVY = (((pmvBaseBY->iMVY)*m_volmd.iver_sampling_factor_n_shape+(SIGN(pmvBaseBY->iMVY))*m_volmd.iver_sampling_factor_m_shape/2)/m_volmd.iver_sampling_factor_m_shape);
					pmbmd->m_shpssmd = INTER_NOT_CODED; //INTER_NO_UPDATE

//INTER_NOT_CODED mode Coding
				#ifdef __TRACE_AND_STATS_
					m_pbitstrmOut->trace ("SCALABLE_SHAPE_INTER_NO_UPDATE");
					m_pbitstrmOut->trace (*pmvBY, "MB_SHP_MV (half pel)");
				#endif // __TRACE_AND_STATS_

				copyReconShapeToMbAndRef (m_ppxlcCurrMBBY, ppxlcSrcFrm, m_puciPredBAB->pixels (), MC_BAB_SIZE, MC_BAB_BORDER);

				#ifdef __TRACE_AND_STATS_
				m_pbitstrmOut->trace ((PixelC *)m_puciPredBAB->pixels(), MC_BAB_SIZE, MC_BAB_SIZE, "MB_MC_PRED_BAB_SS");
				#endif // __TRACE_AND_STATS_
				//OBSS shape mode coding
				Int nBits=0;
				nBits = codeShapeModeSSInter (INTER_NOT_CODED, shpmdColocatedMB);
				return nBits;			
			}
			else {
				motionCompLowerBY ((PixelC*) m_puciPredBAB->pixels (),
							  (PixelC*) pvopcRefQ1->getPlane (BY_PLANE)->pixels (),
							  iX-1, 		
							  iY-1);	//-1 due to 18x18 motion comp
			}
		}
		pmvBY->iMVX = 0;
		pmvBY->iMVY = 0;
		pmbmd->m_shpssmd = INTRA_NOT_CODED; //INTRA_NO_UPDATE

//INTRA_NOT_CODED mode Coding
		#ifdef __TRACE_AND_STATS_
		m_pbitstrmOut->trace ("SCALABLE_SHAPE_INTRA_NO_UPDATE");
		m_pbitstrmOut->trace (*pmvBY, "MB_SHP_MV (half pel)");
		#endif // __TRACE_AND_STATS_

		copyReconShapeToMbAndRef (m_ppxlcCurrMBBY, ppxlcSrcFrm, m_puciPredBAB->pixels (), MC_BAB_SIZE, MC_BAB_BORDER);

		#ifdef __TRACE_AND_STATS_
		m_pbitstrmOut->trace ((PixelC *)m_puciPredBAB->pixels(), MC_BAB_SIZE, MC_BAB_SIZE, "MB_MC_PRED_BAB_SS");
		#endif // __TRACE_AND_STATS_
		//SS shape mode coding
		Int nBits=0;
		nBits = codeShapeModeSSInter (INTRA_NOT_CODED, shpmdColocatedMB);
		return nBits;	
	}
	else {
	m_pbitstrmShapeMBOut->setBookmark ();

//try INTRA_CODED
	UInt nBitsModeIntra = codeShapeModeSSInter (INTRA_CODED, shpmdColocatedMB);
#ifdef __TRACE_AND_STATS_
	m_pbitstrmOut->trace ("SCALABLE_SHAPE_INTRA_CODED_CODING (TRIAL)");
#endif // __TRACE_AND_STATS_
	UInt nBitsSIIntra = encodeSIIntra (INTRA_CODED, (pvopcRefQ1->getPlane (BY_PLANE)->m_pbHorSamplingChk)+iX, (pvopcRefQ1->getPlane (BY_PLANE)->m_pbVerSamplingChk)+iY);
	nBitsIntra = nBitsModeIntra + nBitsSIIntra;

	m_pbitstrmShapeMBOut->gotoBookmark ();		

	
//if lower layer has no motion vector then the current MV is set to (0,0)
	if (pmvBaseBY->iMVX == NOT_MV && pmvBaseBY->iMVY == NOT_MV ){
		pmvBaseBY->iMVX = 0;
		pmvBaseBY->iMVY = 0;
	}

	motionCompBY (	(PixelC*) m_puciPredBAB->pixels (),
					(PixelC*) pvopcRefQ0->getPlane (BY_PLANE)->pixels (),
					(((pmvBaseBY->iMVX)*m_volmd.ihor_sampling_factor_n_shape+(SIGN(pmvBaseBY->iMVX))*m_volmd.ihor_sampling_factor_m_shape/2)/m_volmd.ihor_sampling_factor_m_shape) + iX - 1,		
					(((pmvBaseBY->iMVY)*m_volmd.iver_sampling_factor_n_shape+(SIGN(pmvBaseBY->iMVY))*m_volmd.iver_sampling_factor_m_shape/2)/m_volmd.iver_sampling_factor_m_shape) + iY - 1);	//-1 due to 18x18 motion comp

	if (!isErrorLarge (m_ppxlcCurrMBBY, m_rgiSubBlkIndx16x16, 16,
			m_puciPredBAB->pixels (), m_rgiSubBlkIndx18x18, 18, pmbmd)){
			pmvBY->iMVX = (((pmvBaseBY->iMVX)*m_volmd.ihor_sampling_factor_n_shape+(SIGN(pmvBaseBY->iMVX))*m_volmd.ihor_sampling_factor_m_shape/2)/m_volmd.ihor_sampling_factor_m_shape);
			pmvBY->iMVY = (((pmvBaseBY->iMVY)*m_volmd.iver_sampling_factor_n_shape+(SIGN(pmvBaseBY->iMVY))*m_volmd.iver_sampling_factor_m_shape/2)/m_volmd.iver_sampling_factor_m_shape);
			pmbmd->m_shpssmd = INTER_NOT_CODED; //INTER_NO_UPDATE

//INTER_NOT_CODED mode Coding
			#ifdef __TRACE_AND_STATS_
				m_pbitstrmOut->trace ("SCALABLE_SHAPE_INTER_NO_UPDATE");
				m_pbitstrmOut->trace (*pmvBY, "MB_SHP_MV (half pel)");
			#endif // __TRACE_AND_STATS_

			copyReconShapeToMbAndRef (m_ppxlcCurrMBBY, ppxlcSrcFrm, m_puciPredBAB->pixels (), MC_BAB_SIZE, MC_BAB_BORDER);

			#ifdef __TRACE_AND_STATS_
			m_pbitstrmOut->trace ((PixelC *)m_puciPredBAB->pixels(), MC_BAB_SIZE, MC_BAB_SIZE, "MB_MC_PRED_BAB_SS");
			#endif // __TRACE_AND_STATS_
			//OBSS shape mode coding
			Int nBits=0;
			nBits = codeShapeModeSSInter (INTER_NOT_CODED, shpmdColocatedMB);
			return nBits;			
		}
		else{
			pmvBY->iMVX = (((pmvBaseBY->iMVX)*m_volmd.ihor_sampling_factor_n_shape+(SIGN(pmvBaseBY->iMVX))*m_volmd.ihor_sampling_factor_m_shape/2)/m_volmd.ihor_sampling_factor_m_shape);
			pmvBY->iMVY = (((pmvBaseBY->iMVY)*m_volmd.iver_sampling_factor_n_shape+(SIGN(pmvBaseBY->iMVY))*m_volmd.iver_sampling_factor_m_shape/2)/m_volmd.iver_sampling_factor_m_shape);
			pmbmd->m_shpssmd = INTER_CODED; //INTER_NO_UPDATE
		}
	}

	assert(pmbmd->m_shpssmd == INTER_CODED);

	if (m_bNoShapeChg)
		copyReconShapeToRef (ppxlcSrcFrm, m_ppxlcReconCurrBAB, TOTAL_BAB_SIZE, BAB_BORDER);
	else
		copyReconShapeToMbAndRef (m_ppxlcCurrMBBY, ppxlcSrcFrm, m_ppxlcReconCurrBAB, TOTAL_BAB_SIZE, BAB_BORDER);
#ifdef __TRACE_AND_STATS_
			m_pbitstrmOut->trace ((PixelC *)m_puciPredBAB->pixels(), MC_BAB_SIZE, MC_BAB_SIZE, "MB_MC_PRED_BAB");
			m_pbitstrmOut->trace (m_ppxlcReconCurrBAB, TOTAL_BAB_SIZE, TOTAL_BAB_SIZE, "MB_RECON_BAB (Ignore Borders)");
			m_pbitstrmOut->trace (m_rgpxlcCaeSymbol, m_iWidthCurrBAB , m_iWidthCurrBAB , "MB_ENCODED_BAB");
#endif // __TRACE_AND_STATS_

	// make borders
	makeRightBottomBorder (m_ppxlcReconCurrBAB, TOTAL_BAB_SIZE);
	m_pbitstrmShapeMBOut->setBookmark ();

	// try inter
	UInt nBitsModeInter = codeShapeModeSSInter (pmbmd->m_shpssmd, shpmdColocatedMB);

#ifdef __TRACE_AND_STATS_
			m_pbitstrmOut->trace ("SCALABLE_SHAPE_INTER_CODED_CODING_VERTICAL (TRIAL)");
#endif // __TRACE_AND_STATS_
			UInt nBitsVInter = encodeCAEInter (INTER_CAE_MVDZ, VERTICAL);
#ifdef __TRACE_AND_STATS_
			m_pbitstrmOut->trace ("SCALABLE_SHAPE_INTER_CODED_CODING_HORIZONTAL (TRIAL)");
#endif // __TRACE_AND_STATS_
			UInt nBitsHInter = encodeCAEInter (INTER_CAE_MVDZ, HORIZONTAL);
			UInt nBitsInter = nBitsModeInter + min (nBitsVInter, nBitsHInter);

	m_pbitstrmShapeMBOut->gotoBookmark ();
			
	if (nBitsInter < nBitsIntra){
	// choose inter coding
#ifdef __TRACE_AND_STATS_
		m_pbitstrmOut->trace ("MB_CAE_INTER_CODING (REAL)");
		m_pbitstrmOut->trace (*pmvBY, "MB_SHP_MV (half pel)");
#endif // __TRACE_AND_STATS_
		codeShapeModeSSInter (pmbmd->m_shpssmd, shpmdColocatedMB);
		if(nBitsVInter<nBitsHInter)	encodeCAEInter (INTER_CAE_MVDZ, VERTICAL);
		else						encodeCAEInter (INTER_CAE_MVDZ, HORIZONTAL);
		return nBitsInter;
	}
	else{
		motionCompLowerBY ((PixelC*) m_puciPredBAB->pixels (),
					  (PixelC*) pvopcRefQ1->getPlane (BY_PLANE)->pixels (),
					  iX-1, 		
					  iY-1);	//-1 due to 18x18 motion comp

		// make borders
		makeRightBottomBorder (	m_ppxlcReconCurrBAB, 
								TOTAL_BAB_SIZE,
								(PixelC*)((pvopcRefQ1->getPlane (BY_PLANE)->pixels ())+(iY + EXPANDY_REF_FRAME) * m_iFrameWidthY + iX + EXPANDY_REF_FRAME),
								m_iFrameWidthY);
	// choose intra coding
#ifdef __TRACE_AND_STATS_
		m_pbitstrmOut->trace ("MB_SI_INTRA_CODING (REAL)");
#endif // __TRACE_AND_STATS_
		pmvBY->iMVX = NOT_MV;
		pmvBY->iMVY = NOT_MV;

		//for st_order
		motionCompLowerBY ((PixelC*) m_puciPredBAB->pixels (),
				  (PixelC*) pvopcRefQ1->getPlane (BY_PLANE)->pixels (),
				  iX-1, 		
				  iY-1);	//-1 due to 18x18 motion comp

		pmbmd->m_shpssmd = INTRA_CODED;
		codeShapeModeSSInter (pmbmd->m_shpssmd, shpmdColocatedMB);
		encodeSIIntra (INTRA_CODED, (pvopcRefQ1->getPlane (BY_PLANE)->m_pbHorSamplingChk)+iX, (pvopcRefQ1->getPlane (BY_PLANE)->m_pbVerSamplingChk)+iY);
		return nBitsIntra;
	}
	return FALSE;
}

//shape mode coding for OBSS 
UInt CVideoObjectEncoder::codeShapeModeSSIntra (const ShapeSSMode& shpmd, const ShapeMode& shpmdColocatedMB)
{		
	UInt nBits = 0;
	CEntropyEncoder* pentrenc = m_pentrencSet->m_ppentrencShapeSSModeIntra [0];

#ifdef __TRACE_AND_STATS_
	m_pbitstrmOut->trace ((Int) shpmd, "MB_Curr_ShpSSMd");
#endif // __TRACE_AND_STATS_
	pentrenc->attachStream(*m_pbitstrmShapeMBOut);
	nBits += pentrenc->encodeSymbol (shpmd, "MB_Shape_SS_Mode");
	pentrenc->attachStream(*m_pbitstrmOut);
	return nBits;
}

UInt CVideoObjectEncoder::codeShapeModeSSInter (const ShapeSSMode& shpmd, const ShapeMode& shpmdColocatedMB)
{		
	assert (shpmdColocatedMB != UNKNOWN && shpmd != UNDEFINED);
	UInt nBits = 0;
	Int colocatedIndex[7]={0,0,1,3,3,2,2};
	CEntropyEncoder* pentrenc = m_pentrencSet->m_ppentrencShapeSSModeInter [colocatedIndex[shpmdColocatedMB]];
#ifdef __TRACE_AND_STATS_
	m_pbitstrmOut->trace ((Int) shpmdColocatedMB, "MB_Colocated_ShpMd");
	m_pbitstrmOut->trace ((Int) shpmd, "MB_Curr_ShpSSMd");
#endif // __TRACE_AND_STATS_
	pentrenc->attachStream(*m_pbitstrmShapeMBOut);
	nBits += pentrenc->encodeSymbol (shpmd, "MB_Shape_SS_Mode");
	pentrenc->attachStream(*m_pbitstrmOut);
	return nBits;
}
//for SI shape encoding
UInt CVideoObjectEncoder::encodeSIIntra (ShapeSSMode shpmd, Bool* HorSamplingChk, Bool* VerSamplingChk)
{
	assert (shpmd == INTRA_CODED);	
	UInt nBits = 0;
	/* const PixelC *ppxlcPredBAB = */ m_puciPredBAB->pixels();
	//	Int iSizeMB = m_iWidthCurrBAB-BAB_BORDER*2;
	//	Int iSizePredBAB = iSizeMB+MC_BAB_BORDER*2;
	PixelC* ppxlcReconCurrBABTr = NULL;	//for st_order
	Int scan_order =0;					

	double h_factor = log((double)m_volmd.ihor_sampling_factor_n_shape/m_volmd.ihor_sampling_factor_m_shape)/log((double)2);
	int h_factor_int = (int)floor(h_factor+0.000001);
	double v_factor = log((double)m_volmd.iver_sampling_factor_n_shape/m_volmd.iver_sampling_factor_m_shape)/log((double)2);
	int v_factor_int = (int)floor(v_factor+0.000001);

	Int NumTwoPowerLoopX = h_factor_int;
	Bool ResidualLoopX = 0;
	if(h_factor - h_factor_int > 0.000001 ) ResidualLoopX = 1;
	Int NumTwoPowerLoopY = v_factor_int ; 
	Bool ResidualLoopY = 0 ;
	if(v_factor - v_factor_int > 0.000001 )	ResidualLoopY = 1;


	if ( m_volmd.ihor_sampling_factor_n_shape  == 2  && m_volmd.ihor_sampling_factor_m_shape == 1 && 
		m_volmd.iver_sampling_factor_n_shape  == 2  && m_volmd.iver_sampling_factor_m_shape == 1) {		
		scan_order = decideScanOrder((PixelC*) m_puciPredBAB->pixels ()); //for st_order
		/*--  Curr BAB transposing --*/
		if (scan_order){
			ppxlcReconCurrBABTr = new PixelC [BAB_SIZE * BAB_SIZE];
			for(Int j=0; j<BAB_SIZE; j++)
			   for(Int i=0; i<BAB_SIZE; i++)
					ppxlcReconCurrBABTr[j*BAB_SIZE+i]=m_ppxlcReconCurrBAB[i*BAB_SIZE+j];
			m_rgpxlcCaeSymbol = ppxlcReconCurrBABTr;

			Bool *tmp;
			tmp = HorSamplingChk;
			HorSamplingChk = VerSamplingChk;
			VerSamplingChk = tmp;
		}
	}		

	//the real arithmatic encoding
	StartArCoder (m_parcodec);

	Int	no_mismatch=0, q=0;
	Int	no_mismatch_v=0, no_match_v=0, no_xor_v=0, type_id_mis_v[256][4];
	Int	no_mismatch_h=0, no_match_h=0, no_xor_h=0, type_id_mis_h[256][4];
	
	if ( h_factor > 0.000001 )
		// Vertical Scan 
		VerticalScanning(&no_mismatch_v,&no_match_v,&no_xor_v,type_id_mis_v,NumTwoPowerLoopX,NumTwoPowerLoopY,ResidualLoopX,ResidualLoopY,HorSamplingChk,VerSamplingChk);
	
	if ( v_factor > 0.000001 )
		// Horizontal Scan 
		HorizontalScanning(&no_mismatch_h,&no_match_h,&no_xor_h,type_id_mis_h,NumTwoPowerLoopX,NumTwoPowerLoopY,ResidualLoopX,ResidualLoopY,HorSamplingChk,VerSamplingChk);

	no_mismatch = no_mismatch_v + no_mismatch_h;

	if(no_mismatch==0){
			q=SI_bab_type_prob[0];
			ArCodeSymbol (0,q,m_parcodec,m_pbitstrmShapeMBOut);
			if(ResidualLoopX == 1 || h_factor_int>0)	ExclusiveORcoding(no_xor_v, type_id_mis_v, V_SCANNING);
			if(ResidualLoopY == 1 || v_factor_int>0)	ExclusiveORcoding(no_xor_h, type_id_mis_h, H_SCANNING);
	}
	else{
			q=SI_bab_type_prob[0];
			ArCodeSymbol(1,q,m_parcodec,m_pbitstrmShapeMBOut);
			if(ResidualLoopX == 1 || h_factor_int>0)	FullCoding(no_match_v,type_id_mis_v, V_SCANNING);
			if(ResidualLoopY == 1 || v_factor_int>0)	FullCoding(no_match_h,type_id_mis_h, H_SCANNING);
	}
	StopArCoder (m_parcodec, m_pbitstrmShapeMBOut);

	if ( m_volmd.ihor_sampling_factor_n_shape  == 2  && m_volmd.ihor_sampling_factor_m_shape == 1 && 
		m_volmd.iver_sampling_factor_n_shape  == 2  && m_volmd.iver_sampling_factor_m_shape == 1) {		
		if (scan_order){
			m_rgpxlcCaeSymbol = m_ppxlcReconCurrBAB;
			delete [] ppxlcReconCurrBABTr;

			Bool *tmp;
			tmp = HorSamplingChk;
			HorSamplingChk = VerSamplingChk;
			VerSamplingChk = tmp;
		}
	}			
	nBits += m_parcodec->nBits;
	return nBits;
}

//XOR data coding
Void CVideoObjectEncoder::ExclusiveORcoding(Int no_xor, Int type_id_mis[256][4], SIDirection md_scan)
{
    int i=0,j=0, index=0;
    int bit=0, q=0;

    if(no_xor!=0){
		do{
			index=type_id_mis[j][1];
			if((index==2) || (index==3)){
				bit=index-2;
				if(md_scan==V_SCANNING)	q=enh_intra_v_prob[type_id_mis[j][0]];
				else					q=enh_intra_h_prob[type_id_mis[j][0]];
				ArCodeSymbol(bit,q,m_parcodec,m_pbitstrmShapeMBOut);
				i++;
			}
			j++;
		}while(i<no_xor);
	}
}

//Full coding for the mismatch data existed case
Void CVideoObjectEncoder::FullCoding(Int no_match, Int type_id_mis[256][4], SIDirection md_scan)
{
	int j=0, index=0;
	int bit=0, q=0;

	do{
		index=type_id_mis[j][1];
		bit=(int)index%2;
		if(md_scan==V_SCANNING) q=enh_intra_v_prob[type_id_mis[j][0]];
		else					q=enh_intra_h_prob[type_id_mis[j][0]];
		ArCodeSymbol(bit,q,m_parcodec,m_pbitstrmShapeMBOut);
		j++;
	}while(j<no_match);
}
//~OBSS_SAIT_991015

UInt CVideoObjectEncoder::codeShapeModeIntra (ShapeMode shpmd, const CMBMode* pmbmd, Int iMBX, Int iMBY)
{	
	UInt nBits = 0;
	assert (shpmd == ALL_TRANSP || shpmd == ALL_OPAQUE || shpmd == INTRA_CAE);

	//	static Int iLeftTopMostMB = 0;
	//	Int iRightMostMB = m_iNumMBX - 1;

	//get previous shape codes; 
	Bool bLeftBndry, bRightTopBndry, bTopBndry, bLeftTopBndry;
	Int iMBnum = VPMBnum(iMBX, iMBY);
	bLeftBndry = bVPNoLeft(iMBnum, iMBX);
	bTopBndry = bVPNoTop(iMBnum);
	bRightTopBndry = bVPNoRightTop(iMBnum, iMBX);
	bLeftTopBndry = bVPNoLeftTop(iMBnum, iMBX);

	ShapeMode shpmdTop		= ALL_TRANSP;
	ShapeMode shpmdTopRight = ALL_TRANSP;
	ShapeMode shpmdTopLeft	= ALL_TRANSP;
	ShapeMode shpmdLeft		= ALL_TRANSP;
	// Modified for error resilient mode by Toshiba(1997-11-14)
	if (!bTopBndry)		
		shpmdTop = (pmbmd - m_iNumMBX)->m_shpmd;
	if (!bRightTopBndry)	
		shpmdTopRight = (pmbmd - m_iNumMBX + 1)->m_shpmd;
	if (!bLeftBndry)	
		shpmdLeft = (pmbmd - 1)->m_shpmd;
	if (!bLeftTopBndry)	
		shpmdTopLeft = (pmbmd - m_iNumMBX - 1)->m_shpmd;
	//if (iMBY > iLeftTopMostMB)	{
	//	shpmdTop = (pmbmd - m_iNumMBX)->m_shpmd;
	//	if (iMBX < iRightMostMB)
	//		shpmdTopRight = (pmbmd - m_iNumMBX + 1)->m_shpmd;
	//}
	//if (iMBX > iLeftTopMostMB)	{
	//	shpmdLeft = (pmbmd - 1)->m_shpmd;
	//	if (iMBY > iLeftTopMostMB)
	//		shpmdTopLeft = (pmbmd - m_iNumMBX - 1)->m_shpmd;
	//}
	//	End Toshiba(1997-11-14)
	assert (shpmdTop != UNKNOWN && shpmdTopRight != UNKNOWN && 
			shpmdTopLeft != UNKNOWN && shpmdLeft != UNKNOWN);
	//get code and its size
	UInt uiTblIndex			= shpmdTopLeft * 81 +  shpmdTop * 27 +					
							  shpmdTopRight * 9 + shpmdLeft * 3 + shpmd;
	assert (uiTblIndex >= 0 && uiTblIndex < 243);
	assert (grgchFirstShpCd [uiTblIndex] == 0 || grgchFirstShpCd [uiTblIndex] == 2 || 
			grgchFirstShpCd [uiTblIndex] == 3);
	// Modified for error resilient mode by Toshiba(1997-11-14)
	UInt nCodeSize	= (grgchFirstShpCd [uiTblIndex] == 0) ? 1
			 : grgchFirstShpCd [uiTblIndex];
#ifdef __TRACE_AND_STATS_
	m_pbitstrmOut->trace ((Int) shpmd, "MB_Curr_ShpMd");
#endif // __TRACE_AND_STATS_
	m_pbitstrmShapeMBOut->putBits ((Int) 1, nCodeSize, "MB_Shape_Mode");
	//	End Toshiba(1997-11-14)
	nBits += nCodeSize;
	return nBits;
}

UInt CVideoObjectEncoder::codeShapeModeInter (const ShapeMode& shpmd, const ShapeMode& shpmdColocatedMB)
{		
	assert (shpmdColocatedMB != UNKNOWN && shpmd != UNKNOWN);
	UInt nBits = 0;
	CEntropyEncoder* pentrenc = m_pentrencSet->m_ppentrencShapeMode [shpmdColocatedMB];
#ifdef __TRACE_AND_STATS_
	m_pbitstrmOut->trace ((Int) shpmdColocatedMB, "MB_Colocated_ShpMd");
	m_pbitstrmOut->trace ((Int) shpmd, "MB_Curr_ShpMd");
#endif // __TRACE_AND_STATS_
	pentrenc->attachStream(*m_pbitstrmShapeMBOut);
	nBits += pentrenc->encodeSymbol (shpmd, "MB_Shape_Mode");
	pentrenc->attachStream(*m_pbitstrmOut);
	return nBits;
}

// find appropriate size for coded babs
ShapeMode CVideoObjectEncoder::round (PixelC* ppxlcSrcFrm, const CMBMode* pmbmd)					//only to partial BABs
{
	ShapeMode shpmd;
	Bool bIsErrorLarge;

	if (m_volmd.bNoCrChange == FALSE)
	{
		// attempt to reduce bab size
		m_bNoShapeChg = FALSE;
		static int iThreshD4 = 7 * MPEG4_OPAQUE;
		
		// reduce by factor 4
		downSampleShape (m_ppxlcCurrMBBY, 
						   m_rgiSubBlkIndx16x16,
						   m_ppxlcCurrMBBYDown4, 
						   m_rgiPxlIndx8x8,
						   4, iThreshD4, 16);

		// see if this size is acceptable
		shpmd = INTRA_CAE;
		m_iInverseCR = 4;
		m_iWidthCurrBAB = 8;
		
		// subsample border before upsample to original size
		subsampleLeftTopBorderFromVOP (ppxlcSrcFrm, m_ppxlcCurrMBBYDown4);
		makeRightBottomBorder (m_ppxlcCurrMBBYDown4, 8);
		upSampleShape(ppxlcSrcFrm,m_ppxlcCurrMBBYDown4,m_ppxlcReconCurrBAB);
		
		// check if approximation is acceptable
		bIsErrorLarge = isErrorLarge (m_ppxlcCurrMBBY, m_rgiSubBlkIndx16x16, 16,
			m_ppxlcReconCurrBAB, m_rgiSubBlkIndx20x20, 20, pmbmd);
		if(!bIsErrorLarge)
		{
			// ok, so assign encoding buffer
			m_rgpxlcCaeSymbol = m_ppxlcCurrMBBYDown4; 
			return shpmd;
		}

		static int iThreshD2 = MPEG4_OPAQUE;

		// factor 4 failed, so try to reduce by factor 2
		downSampleShape (m_ppxlcCurrMBBY, 
						   m_rgiSSubBlkIndx16x16,
						   m_ppxlcCurrMBBYDown2, 
						   m_rgiPxlIndx12x12,
						   2, iThreshD2, 64);

		// mixed, so see if this size is acceptable
		shpmd = INTRA_CAE;
		m_iInverseCR = 2;
		m_iWidthCurrBAB = 12;

		// subsample border before upsample to original size
		subsampleLeftTopBorderFromVOP (ppxlcSrcFrm, m_ppxlcCurrMBBYDown2);
		makeRightBottomBorder (m_ppxlcCurrMBBYDown2, 12);
		upSampleShape(ppxlcSrcFrm,m_ppxlcCurrMBBYDown2, m_ppxlcReconCurrBAB);

		// check if approximation is acceptable
		bIsErrorLarge = isErrorLarge (m_ppxlcCurrMBBY, m_rgiSubBlkIndx16x16, 16,
			m_ppxlcReconCurrBAB, m_rgiSubBlkIndx20x20, 20, pmbmd);
		if(!bIsErrorLarge)
		{
			// ok, so assign encoding buffer
			m_rgpxlcCaeSymbol = m_ppxlcCurrMBBYDown2; 
			return shpmd;
		}
	}

	// has to be original size
	m_bNoShapeChg = TRUE;
	shpmd = INTRA_CAE;
	m_iInverseCR = 1;
	m_iWidthCurrBAB = BAB_SIZE;

	// copy data to ReconCurrBAB
	PixelC* ppxlcDst = m_ppxlcReconCurrBAB + BAB_BORDER + BAB_BORDER * TOTAL_BAB_SIZE;
	PixelC* ppxlcSrc = m_ppxlcCurrMBBY;
	Int iUnit = sizeof(PixelC); // NBIT: for memcpy
	Int i;
	for (i = 0; i < MB_SIZE; i++) {
		memcpy (ppxlcDst, ppxlcSrc, MB_SIZE*iUnit);
		ppxlcSrc += MB_SIZE;
		ppxlcDst += BAB_SIZE;
	}

	// make borders
	copyLeftTopBorderFromVOP (ppxlcSrcFrm, m_ppxlcReconCurrBAB);
	makeRightBottomBorder (m_ppxlcReconCurrBAB, TOTAL_BAB_SIZE);	

	// assign encoding buffer
	m_rgpxlcCaeSymbol = m_ppxlcReconCurrBAB;
	return shpmd;
}


Int CVideoObjectEncoder::downSampleShape (const PixelC* ppxlcSrc, 
								   Int* rgiSrcSubBlk,
								   PixelC* ppxlcDst, 
								   Int* piDstPxl, 
								   Int iRate,
								   Int iThreshold,
								   Int nSubBlk)
{
	Int nOpaquePixel = 0, iSum = 0;
	Int iSubBlk;
	for (iSubBlk = 0; iSubBlk < nSubBlk; iSubBlk++)	{
		Int i = rgiSrcSubBlk [iSubBlk];
		iSum = 0;
		for (CoordI iY = 0; iY < iRate; iY++)	{
			for (CoordI iX = 0; iX < iRate; iX++)  {
				iSum += abs (ppxlcSrc [i++]);  //abs???
			}
			i += MB_SIZE - iRate;
		}
		ppxlcDst [*piDstPxl] = (iSum > iThreshold) ? MPEG4_OPAQUE : MPEG4_TRANSPARENT;
		nOpaquePixel += ppxlcDst [*piDstPxl];
		piDstPxl++;
	}		
	return (nOpaquePixel /= MPEG4_OPAQUE);
}

Bool CVideoObjectEncoder::isErrorLarge (const PixelC* rgppxlcSrc, const Int* rgiSubBlkIndx, Int iWidthSrc, PixelC pxlcRecon, const CMBMode* pmbmd)
{
	if (pmbmd->m_bhas4MVForward == TRUE)	{
		if (!sameBlockTranspStatus (pmbmd, pxlcRecon))	
			return TRUE;
	} 

	//check error in each 4x4 subblock
	Int iSubBlk;
	Int iError = 0;
	for (iSubBlk = 0; iSubBlk < 16; iSubBlk++)	{
		Int i = rgiSubBlkIndx [iSubBlk];
		for (CoordI iY = 0; iY < 4; iY++)	{
			for (CoordI iX = 0; iX < 4; iX++)  {
				iError += abs (rgppxlcSrc [i++] - pxlcRecon);
			}
			if (iError > m_volmd.iBinaryAlphaTH) 
				return TRUE;
			i += iWidthSrc - 4;
		}
	}		
	return FALSE;
}

Bool CVideoObjectEncoder::isErrorLarge (const PixelC* rgppxlcSrc, const Int* rgiSubBlkIndxSrc, const Int iSizeSrc,
								const PixelC* rgppxlcDst, const Int* rgiSubBlkIndxDst, const Int iSizeDst, const CMBMode* pmbmd)
{
	if (pmbmd->m_bhas4MVForward == TRUE)	{
		if (!sameBlockTranspStatus (pmbmd, rgppxlcDst, iSizeDst))	
			return TRUE;
	} 

	//check error in each 4x4 subblock
	Int iSubBlk;
	Int iError = 0;
	for (iSubBlk = 0; iSubBlk < 16; iSubBlk++)	{
		Int iSrc = rgiSubBlkIndxSrc [iSubBlk];
		Int iDst = rgiSubBlkIndxDst [iSubBlk];
		for (CoordI iY = 0; iY < 4; iY++)	{
			for (CoordI iX = 0; iX < 4; iX++)  {
				iError += abs (rgppxlcSrc [iSrc] - rgppxlcDst [iDst]);
				iSrc++;
				iDst++;
			}
			if (iError > m_volmd.iBinaryAlphaTH)
				return TRUE;
			iSrc += iSizeSrc - 4;				
			iDst += iSizeDst - 4;
		}
	}		
	return FALSE;
}

UInt CVideoObjectEncoder::encodeCAEIntra (ShapeMode shpmd, CAEScanDirection shpdir)
{
	assert (shpmd == INTRA_CAE);
//	m_pbitstrmOut->trace (m_rgiCurrShp, TOTAL_BAB_SQUARE_SIZE, "MB_BAB");
	UInt nBits = 0;
	nBits += codeCrAndSt (shpdir, m_iInverseCR);
	//the real arithmatic encoding
	StartArCoder (m_parcodec);
	if (shpdir == HORIZONTAL)	{
		const PixelC* ppxlcSrcRow = m_rgpxlcCaeSymbol + m_iWidthCurrBAB * BAB_BORDER + BAB_BORDER;
		for (Int iRow = BAB_BORDER; iRow < m_iWidthCurrBAB - BAB_BORDER; iRow++)	{
			const PixelC* ppxlcSrc = ppxlcSrcRow;
			for (Int iCol = BAB_BORDER; iCol < m_iWidthCurrBAB - BAB_BORDER; iCol++)	{
				Int iContext = contextIntra (ppxlcSrc);
#ifdef __TRACE_AND_STATS_
			//	m_pbitstrmOut->trace (iContext, "MB_CAE_Context");
#endif // __TRACE_AND_STATS_
				ArCodeSymbol ((*ppxlcSrc == MPEG4_OPAQUE), gCAEintraProb [iContext], m_parcodec, m_pbitstrmShapeMBOut);	
				ppxlcSrc++;
			}
			ppxlcSrcRow += m_iWidthCurrBAB;
		}
	}
	else {
		const PixelC* ppxlcSrcCol = m_rgpxlcCaeSymbol + m_iWidthCurrBAB * BAB_BORDER + BAB_BORDER;
		for (Int iCol = BAB_BORDER; iCol < m_iWidthCurrBAB - BAB_BORDER; iCol++)	{
			const PixelC* ppxlcSrc = ppxlcSrcCol;
			for (Int iRow = BAB_BORDER; iRow < m_iWidthCurrBAB - BAB_BORDER; iRow++)	{
				Int iContext = contextIntraTranspose (ppxlcSrc);
#ifdef __TRACE_AND_STATS_
				//m_pbitstrmOut->trace (iContext, "MB_CAE_Context");
#endif // __TRACE_AND_STATS_
				ArCodeSymbol ((*ppxlcSrc == MPEG4_OPAQUE), gCAEintraProb [iContext], m_parcodec, m_pbitstrmShapeMBOut);	
				ppxlcSrc += m_iWidthCurrBAB ;
			}
			ppxlcSrcCol++;
		}
	}
	StopArCoder (m_parcodec, m_pbitstrmShapeMBOut);
	nBits += m_parcodec->nBits;
	return nBits;
}

UInt CVideoObjectEncoder::encodeCAEInter (ShapeMode shpmd, CAEScanDirection shpdir)
{
	assert (shpmd == INTER_CAE_MVDNZ || shpmd == INTER_CAE_MVDZ);	
//	m_pbitstrmOut->trace (m_rgiCurrShp, TOTAL_BAB_SQUARE_SIZE, "MB_BAB");

	UInt nBits = 0;
	nBits += codeCrAndSt (shpdir, m_iInverseCR);
	const PixelC *ppxlcPredBAB = m_puciPredBAB->pixels();
	Int iSizeMB = m_iWidthCurrBAB-BAB_BORDER*2;
	Int iSizePredBAB = iSizeMB+MC_BAB_BORDER*2;
	if(m_iInverseCR==2)
	{
		downSampleShapeMCPred(ppxlcPredBAB,m_ppxlcPredBABDown2,2);
		ppxlcPredBAB = m_ppxlcPredBABDown2;
	}
	else if(m_iInverseCR==4)
	{
		downSampleShapeMCPred(ppxlcPredBAB,m_ppxlcPredBABDown4,4);
		ppxlcPredBAB = m_ppxlcPredBABDown4;
	}

	//the real arithmatic encoding
	StartArCoder (m_parcodec);
	if (shpdir == HORIZONTAL)	{
		const PixelC* ppxlcSrcRow  = m_rgpxlcCaeSymbol + m_iWidthCurrBAB * BAB_BORDER + BAB_BORDER;
		const PixelC* ppxlcPredRow = ppxlcPredBAB + iSizePredBAB * MC_BAB_BORDER + MC_BAB_BORDER;
		for (Int iRow = 0; iRow < iSizeMB; iRow++)	{
			const PixelC* ppxlcSrc = ppxlcSrcRow;
			const PixelC* ppxlcPred = ppxlcPredRow;
			for (Int iCol = 0; iCol < iSizeMB; iCol++)	{
				Int iContext = contextInter (ppxlcSrc, ppxlcPred);
#ifdef __TRACE_AND_STATS_
				//m_pbitstrmOut->trace (iContext, "MB_CAE_Context");
#endif // __TRACE_AND_STATS_
				ArCodeSymbol ((*ppxlcSrc == MPEG4_OPAQUE), gCAEinterProb [iContext], m_parcodec, m_pbitstrmShapeMBOut);	
				ppxlcSrc++;
				ppxlcPred++;
			}
			ppxlcSrcRow += m_iWidthCurrBAB;
			ppxlcPredRow += iSizePredBAB;
		}
	}
	else {
		const PixelC* ppxlcSrcCol  = m_rgpxlcCaeSymbol + m_iWidthCurrBAB * BAB_BORDER + BAB_BORDER;
		const PixelC* ppxlcPredCol = ppxlcPredBAB + iSizePredBAB * MC_BAB_BORDER + MC_BAB_BORDER;		
		for (Int iCol = 0; iCol < iSizeMB; iCol++)	{
			const PixelC* ppxlcSrc = ppxlcSrcCol;
			const PixelC* ppxlcPred = ppxlcPredCol;
			for (Int iRow = 0; iRow < iSizeMB; iRow++)	{
				Int iContext = contextInterTranspose (ppxlcSrc, ppxlcPred);
#ifdef __TRACE_AND_STATS_
				//m_pbitstrmOut->trace (iContext, "MB_CAE_Context");
#endif // __TRACE_AND_STATS_
				ArCodeSymbol ((*ppxlcSrc == MPEG4_OPAQUE), gCAEinterProb [iContext], m_parcodec, m_pbitstrmShapeMBOut);	
				ppxlcSrc += m_iWidthCurrBAB;
				ppxlcPred += iSizePredBAB;
			}
			ppxlcSrcCol++;
			ppxlcPredCol++;
		}
	}
	StopArCoder (m_parcodec, m_pbitstrmShapeMBOut);
	nBits += m_parcodec->nBits;
	return nBits;
}


UInt CVideoObjectEncoder::encodeMVDS (CMotionVector mvBYD)
{
	assert (!mvBYD.isZero());

	m_pentrencSet->m_pentrencShapeMV1->attachStream(*m_pbitstrmShapeMBOut);
	m_pentrencSet->m_pentrencShapeMV2->attachStream(*m_pbitstrmShapeMBOut);
	UInt nBits = 0;
	nBits += m_pentrencSet->m_pentrencShapeMV1->encodeSymbol (abs (mvBYD.iMVX), "MB_SHP_MVDX");			
	if (mvBYD.iMVX != 0)	{
		m_pbitstrmShapeMBOut->putBits (signOf(mvBYD.iMVX), (UInt) 1, "MB_SHP_MVDX_Sign");
		nBits++;
		nBits += m_pentrencSet->m_pentrencShapeMV1->encodeSymbol (abs (mvBYD.iMVY), "MB_SHP_MVDY");
	}
	else 
		nBits += m_pentrencSet->m_pentrencShapeMV2->encodeSymbol (abs (mvBYD.iMVY) - 1, "MB_SHP_MVDY");
	if (mvBYD.iMVY != 0)	{		
		m_pbitstrmShapeMBOut->putBits (signOf(mvBYD.iMVY), (UInt) 1, "MB_SHP_MVDY_Sign");
		nBits++;
	}
	
	m_pentrencSet->m_pentrencShapeMV1->attachStream(*m_pbitstrmOut);
	m_pentrencSet->m_pentrencShapeMV2->attachStream(*m_pbitstrmOut);
	return nBits;
}

Bool CVideoObjectEncoder::sameBlockTranspStatus (const CMBMode* pmbmd, PixelC pxlcRecon)	//assume src is 16 x 16 buffer
{
	if (pxlcRecon == opaqueValue)
		return ((pmbmd->m_rgTranspStatus [Y_BLOCK1] != ALL) &&		//if recon is all opaque
				(pmbmd->m_rgTranspStatus [Y_BLOCK2] != ALL) &&		//then each orig. blk has to be not all transp
				(pmbmd->m_rgTranspStatus [Y_BLOCK3] != ALL) &&
				(pmbmd->m_rgTranspStatus [Y_BLOCK4] != ALL));
	return TRUE;
}

Bool CVideoObjectEncoder::sameBlockTranspStatus (const CMBMode* pmbmd, const PixelC* pxlcRecon, Int iSizeRecon) //src: 16x16 Recon: 18 x 18 || 20 x 20
{
	Int iBorder = (iSizeRecon == 18) ? 1 : 2;
	const PixelC* ppxlcBlk1 = pxlcRecon + iBorder + iBorder * iSizeRecon;
	const PixelC* ppxlcBlk2 = ppxlcBlk1 + BLOCK_SIZE;
	const PixelC* ppxlcBlk3 = pxlcRecon + iSizeRecon * iSizeRecon / 2 + iBorder;
	const PixelC* ppxlcBlk4 = ppxlcBlk3 + BLOCK_SIZE;
	static PixelC rgnOpaquePixels [4];
	rgnOpaquePixels [0] = rgnOpaquePixels [1] = rgnOpaquePixels [2] = rgnOpaquePixels [3] = 0;
	CoordI ix, iy;
	for (iy = 0; iy < BLOCK_SIZE; iy++) {
		for (ix = 0; ix < BLOCK_SIZE; ix++) {
			rgnOpaquePixels [0] += ppxlcBlk1 [ix];
			rgnOpaquePixels [1] += ppxlcBlk2 [ix];
			rgnOpaquePixels [2] += ppxlcBlk3 [ix];
			rgnOpaquePixels [3] += ppxlcBlk4 [ix];
		}
		ppxlcBlk1 += iSizeRecon;
		ppxlcBlk2 += iSizeRecon;
		ppxlcBlk3 += iSizeRecon;
		ppxlcBlk4 += iSizeRecon;
	}

	Int iBlk;
	for (iBlk = Y_BLOCK1; iBlk <= Y_BLOCK4; iBlk++)
		if (pmbmd->m_rgTranspStatus [iBlk] == ALL)			//if org. blk is all transp, the recon can be otherwise
			if (rgnOpaquePixels [iBlk - 1] != 0)
				return FALSE;
	return TRUE;
}


UInt CVideoObjectEncoder::codeCrAndSt (CAEScanDirection shpdir, Int iInverseCR)
{
	UInt nBits = 0;
	if (m_volmd.bNoCrChange == FALSE)	{
		UInt nBitsCR = (iInverseCR == 1) ? 1 : 2;
		Int iCode = (iInverseCR == 1) ? 0 : (iInverseCR == 2) ? 2 : 3;
		m_pbitstrmShapeMBOut->putBits (iCode, nBitsCR, "MB_CR");
		nBits += nBitsCR;
	}
	//Scan direction
	m_pbitstrmShapeMBOut->putBits ((shpdir == HORIZONTAL) ? 1 : 0, (UInt) 1, "MB_ST");
	nBits++;
	return nBits;
}

Void CVideoObjectEncoder::copyReconShapeToRef (PixelC* ppxlcRefFrm, PixelC pxlcSrc)		//no need to reset MB buffer
{
	for (Int i = 0; i < MB_SIZE; i++)	{
		pxlcmemset (ppxlcRefFrm, pxlcSrc, MB_SIZE);
		ppxlcRefFrm+= m_iFrameWidthY;
	}
}

Void CVideoObjectEncoder::copyReconShapeToRef (PixelC* ppxlcRefFrm, 
											   const PixelC* ppxlcSrc, Int iSrcWidth, Int iBorder)
{
	ppxlcSrc += iSrcWidth * iBorder + iBorder;
	Int iUnit = sizeof(PixelC); // NBIT: for memcpy
	for (Int i = 0; i < MB_SIZE; i++)	{
		memcpy (ppxlcRefFrm, ppxlcSrc, MB_SIZE*iUnit);
		ppxlcRefFrm += m_iFrameWidthY;
		ppxlcSrc += iSrcWidth;
	}
}

Int CVideoObjectEncoder::sadForShape (const PixelC* ppxlcRefBY) const
{
	Int iInitSAD = 0;
	CoordI ix, iy;
	const PixelC* ppxlcCurrBY = m_ppxlcCurrMBBY;
	for (iy = 0; iy < MB_SIZE; iy++) {
		for (ix = 0; ix < MB_SIZE; ix++)
			iInitSAD += abs (ppxlcCurrBY [ix] - ppxlcRefBY [ix]);
		ppxlcCurrBY += MB_SIZE;
		ppxlcRefBY += m_iFrameWidthY;
	}
	return iInitSAD;
}

Void CVideoObjectEncoder::blkmatchForShape (
	CVOPU8YUVBA* pvopcRefQ,
	CMotionVector* pmv,
	const CVector& mvPredHalfPel,
	CoordI iX, CoordI iY
)
{
	Int mbDiff;
	CoordI x = iX + (mvPredHalfPel.x >> 1), y = iY + (mvPredHalfPel.y >> 1); 
	Int iXDest = x, iYDest = y;
	CoordI ix, iy;
	const PixelC* ppxlcRefMBY = pvopcRefQ->pixelsBY () + m_rctRefFrameY.offset (x, y);
	Int iMinSAD = sadForShape (ppxlcRefMBY);
	pmv->iMVX = pmv->iMVY = 0;
	Int uiPosInLoop, iLoop;
	// Spiral Search for the rest
	for (iLoop = 1; iLoop <= 16; iLoop++) {
		x++;
		y++;
		ppxlcRefMBY += m_iFrameWidthY + 1;
		for (uiPosInLoop = 0; uiPosInLoop < (iLoop << 3); uiPosInLoop++) {
			// inside each spiral loop 
			if (
				x >= m_rctRefVOPY0.left && y >= m_rctRefVOPY0.top && 
				x <= m_rctRefVOPY0.right - MB_SIZE && y <= m_rctRefVOPY0.bottom - MB_SIZE
			) {
				const PixelC* ppxlcTmpC = m_ppxlcCurrMBBY;
				const PixelC* ppxlcRefMB = ppxlcRefMBY;
				mbDiff = 0;
				for (iy = 0; iy < MB_SIZE; iy++) {
					for (ix = 0; ix < MB_SIZE; ix++)
						mbDiff += abs (ppxlcTmpC [ix] - ppxlcRefMB [ix]);
					if (mbDiff >= iMinSAD)
						goto NEXT_POSITION; // skip the current position
					ppxlcRefMB += m_iFrameWidthY;
					ppxlcTmpC += MB_SIZE;
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
	*pmv = CMotionVector (iXDest - iX, iYDest - iY);
}

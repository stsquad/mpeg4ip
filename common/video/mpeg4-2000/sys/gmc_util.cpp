/**************************************************************************

This software module was originally developed by 

	Yoshinori Suzuki (Hitachi, Ltd.)
	Yuichiro Nakaya (Hitachi, Ltd.)


in the course of development of the MPEG-4 Video (ISO/IEC 14496-2) standard.
This software module is an implementation of a part of one or more MPEG-4 
Video (ISO/IEC 14496-2) tools as specified by the MPEG-4 Video (ISO/IEC 
14496-2) standard. 

ISO/IEC gives users of the MPEG-4 Video (ISO/IEC 14496-2) standard free 
license to this software module or modifications thereof for use in hardware
or software products claiming conformance to the MPEG-4 Video (ISO/IEC 
14496-2) standard. 

Those intending to use this software module in hardware or software products
are advised that its use may infringe existing patents. The original 
developer of this software module and his/her company, the subsequent 
editors and their companies, and ISO/IEC have no liability for use of this 
software module or modifications thereof in an implementation. Copyright is 
not released for non MPEG-4 Video (ISO/IEC 14496-2) standard conforming 
products. 

Hitachi, Ltd. retains full right to use the code for his/her own 
purpose, assign or donate the code to a third party and to inhibit third 
parties from using the code for non MPEG-4 Video (ISO/IEC 14496-2) standard
conforming products. This copyright notice must be included in all copies or
derivative works. 

Copyright (c) 1998.

Module Name:

	gmc_util.cpp

Abstract:

	Reference points decoding for GMC.

**************************************************************************/

#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <iostream.h>
#include <assert.h>

#include "typeapi.h"
#include "codehead.h"
#include "global.hpp"
#include "entropy/bitstrm.hpp"
#include "entropy/entropy.hpp"
#include "entropy/huffman.hpp"
#include "mode.hpp"
#include "vopses.hpp"


Void CVideoObject::FindGlobalPredForGMC (Int cx_curr, Int cy_curr,
										 PixelC* ppxlcRef, const PixelC* puciRef) 
{
	
	switch(m_iNumOfPnts) {
	case 0:
		StationalWarpForGMC(cx_curr, cy_curr,
			ppxlcRef, puciRef);
		break;
	case 1:
		TranslationalWarpForGMC(cx_curr, cy_curr,
			ppxlcRef, puciRef);
		break;
	case 2:
	case 3:
		FastAffineWarpForGMC(cx_curr, cy_curr,
			ppxlcRef, puciRef);
		break;
	default:
		assert(m_iNumOfPnts<=3);
	}
}

Void CVideoObject::FindGlobalChromPredForGMC (Int cx_curr, Int cy_curr,
											  PixelC* ppxlcRefU, PixelC* ppxlcRefV) 
{
	
	switch(m_iNumOfPnts) {
	case 0:
		StationalWarpChromForGMC(cx_curr, cy_curr,
			ppxlcRefU, ppxlcRefV);
		break;
	case 1:
		TranslationalWarpChromForGMC(cx_curr, cy_curr,
			ppxlcRefU, ppxlcRefV);
		break;
	case 2:
	case 3:
		FastAffineWarpChromForGMC(cx_curr, cy_curr,
			ppxlcRefU, ppxlcRefV);
		break;
	default:
		assert(m_iNumOfPnts<=3);
	}
}

Void CVideoObject::StationalWarpForGMC(Int cx_curr, Int cy_curr,
									   PixelC *ppxlcRef, const PixelC *puciRef)
{
	Int iref_width = m_rctRefFrameY.right - m_rctRefFrameY.left;
	//	Int iref_height = m_rctRefFrameY.bottom - m_rctRefFrameY.top;
	Int iref_left = (m_rctRefVOPY0.left + EXPANDY_REFVOP);
	Int iref_top = (m_rctRefVOPY0.top + EXPANDY_REFVOP);
	Int iref_right = (m_rctRefVOPY0.right - EXPANDY_REFVOP + 31);
	Int iref_bottom = (m_rctRefVOPY0.bottom - EXPANDY_REFVOP+ 31);
	PixelC* pprev =  (PixelC*) puciRef +
		(EXPANDY_REF_FRAME-16)*m_iFrameWidthY +EXPANDY_REF_FRAME-16 ;
	Int xi,yj, ipel_in;
	Int xTI,yTJ;
	Int xTI_w,yTJ_w;
	
	for (yj = 0, yTJ = cy_curr; yj < MB_SIZE; yj++, yTJ++) {
		for (xi = 0, xTI = cx_curr; xi < MB_SIZE; xi++, xTI++) {
			
			xTI_w = xTI; yTJ_w = yTJ;
			
			if (xTI_w<iref_left||yTJ_w<iref_top||xTI_w>iref_right||yTJ_w>iref_bottom)
			{
				if(xTI_w<iref_left)xTI_w=iref_left;
				if(xTI_w>iref_right)xTI_w=iref_right;
				if(yTJ_w<iref_top)yTJ_w=iref_top;
				if(yTJ_w>iref_bottom)yTJ_w=iref_bottom;
				
			}
			
			ipel_in = yTJ_w * iref_width + xTI_w;
			ppxlcRef[xi+yj*MB_SIZE] = pprev[ipel_in];
			
		}
	}
	
}


Void CVideoObject::StationalWarpChromForGMC(Int cx_curry, Int cy_curry,
											PixelC *ppxlcRefU, PixelC *ppxlcRefV)
{
	Int iWarpingAccuracyp1 = m_uiWarpingAccuracy+1;
	Int iref_width = m_rctRefFrameY.right - m_rctRefFrameY.left;
	Int iref_height = m_rctRefFrameY.bottom - m_rctRefFrameY.top;
	Int iref_left = ((m_rctRefVOPY0.left/2) + EXPANDUV_REFVOP)<<iWarpingAccuracyp1;
	Int iref_top = ((m_rctRefVOPY0.top/2) + EXPANDUV_REFVOP)<<iWarpingAccuracyp1;
	Int iref_right = ((m_rctRefVOPY0.right/2) - EXPANDUV_REFVOP + 15)<<iWarpingAccuracyp1;
	Int iref_bottom = ((m_rctRefVOPY0.bottom/2) - EXPANDUV_REFVOP + 15)<<iWarpingAccuracyp1;
	CU8Image *puciRefU = (CU8Image*) m_pvopcRefQ0 -> getPlane (U_PLANE);
	CU8Image *puciRefV = (CU8Image*) m_pvopcRefQ0 -> getPlane (V_PLANE);
	PixelC* pprevu = (PixelC*) puciRefU -> pixels () +
		(EXPANDUV_REF_FRAME - 8)*m_iFrameWidthUV +EXPANDUV_REF_FRAME - 8;
	PixelC* pprevv = (PixelC*) puciRefV -> pixels () +
		(EXPANDUV_REF_FRAME - 8)*m_iFrameWidthUV +EXPANDUV_REF_FRAME - 8;
	
	Int ipel_in;
	Int xi, yj, xTI, yTJ;
	Int xTI_w, yTJ_w;
	
	Int cx_curr = cx_curry>> 1;
	Int cy_curr = cy_curry>> 1;
	iref_width = iref_width >>1;
	iref_height = iref_height >>1;
	
	
	for (yj = 0, yTJ = cy_curr; yj < MB_SIZE/2; yj++, yTJ++) {
		for (xi = 0, xTI = cx_curr; xi < MB_SIZE/2; xi++, xTI++) {
			
			xTI_w = xTI; yTJ_w = yTJ;
			
			if (xTI_w<iref_left||yTJ_w<iref_top||xTI_w>iref_right||yTJ_w>iref_bottom)
			{
				if(xTI_w<iref_left)xTI_w=iref_left;
				if(xTI_w>iref_right)xTI_w=iref_right;
				if(yTJ_w<iref_top)yTJ_w=iref_top;
				if(yTJ_w>iref_bottom)yTJ_w=iref_bottom;
				
			}
			
			ipel_in = yTJ_w * iref_width + xTI_w;
			
			ppxlcRefU[xi+yj*BLOCK_SIZE] = pprevu[ipel_in];
			ppxlcRefV[xi+yj*BLOCK_SIZE] = pprevv[ipel_in];
			
		}
	}
}


Void CVideoObject::TranslationalWarpForGMC(Int cx_curr, Int cy_curr,
										   PixelC *ppxlcRef, const PixelC *puciRef)
{
	Int iWarpingAccuracyp1 = m_uiWarpingAccuracy+1;
	Int iref_width = m_rctRefFrameY.right - m_rctRefFrameY.left;
	//Int iref_height = m_rctRefFrameY.bottom - m_rctRefFrameY.top;
	Int iref_left = (m_rctRefVOPY0.left + EXPANDY_REFVOP)<<iWarpingAccuracyp1;
	Int iref_top = (m_rctRefVOPY0.top + EXPANDY_REFVOP)<<iWarpingAccuracyp1;
	Int iref_right = (m_rctRefVOPY0.right - EXPANDY_REFVOP + 31)<<iWarpingAccuracyp1;
	Int iref_bottom = (m_rctRefVOPY0.bottom - EXPANDY_REFVOP + 31)<<iWarpingAccuracyp1;
	PixelC* pprev = (PixelC*) puciRef +
		(EXPANDY_REF_FRAME-16)*m_iFrameWidthY +EXPANDY_REF_FRAME-16;
	Int xi0, yj0;
	Int xi0prime, yj0prime;
	Int xi,yj;
	Int cxx, cyy, ipa, ipe, ioffset;
	Int ibias, isqr_wac;
	Int irx, iry;
	Int cxx_w, cyy_w;
	
	Int iPowerWA, iwarpmask;
	
	iPowerWA = 1 << iWarpingAccuracyp1;
	iwarpmask = iPowerWA - 1;
	isqr_wac = iWarpingAccuracyp1 << 1;
	ibias = 1 << (isqr_wac - 1);
	
	xi0 = m_rctCurrVOPY.left ;
	yj0 = m_rctCurrVOPY.top ;
	
	xi0prime = (Int)((m_rgstDstQ[0].x + 16) * 2.0) ;
	yj0prime = (Int)((m_rgstDstQ[0].y + 16) * 2.0) ;
	
	xi0prime = xi0prime << m_uiWarpingAccuracy;
	yj0prime = yj0prime << m_uiWarpingAccuracy;
	
	cxx = xi0prime;
	cyy = yj0prime;
	ipa = iPowerWA;
	ipe = iPowerWA;
	
	xi0prime += ipa*(cx_curr-xi0);
	cyy += ipe*(cy_curr-yj0);
	
	for (yj = 0; yj < MB_SIZE; cyy+=ipe, yj++) {
		for (xi = 0, cxx=xi0prime; xi < MB_SIZE; cxx+=ipa, xi++) {
			
			cxx_w = cxx; cyy_w = cyy;
			
			if (cxx_w<iref_left||cyy_w<iref_top||cxx_w>iref_right||cyy_w>iref_bottom)
			{
				if(cxx_w<iref_left)cxx_w=iref_left;
				if(cxx_w>iref_right)cxx_w=iref_right;
				if(cyy_w<iref_top)cyy_w=iref_top;
				if(cyy_w>iref_bottom)cyy_w=iref_bottom;
				
			}
			
			ioffset = (cyy_w >> iWarpingAccuracyp1) * iref_width + 
				(cxx_w >> iWarpingAccuracyp1);
			irx = cxx_w & iwarpmask;
			iry = cyy_w & iwarpmask;
			
			ppxlcRef[xi+yj*MB_SIZE] = CInterpolatePixelValue(pprev, ioffset,
				iref_width, irx, iry, iPowerWA,
				ibias, isqr_wac);
		}
	}
	
}

Void CVideoObject::TranslationalWarpChromForGMC(Int cx_curry, Int cy_curry,
												PixelC *ppxlcRefU, PixelC *ppxlcRefV)
{
	Int iWarpingAccuracyp1 = m_uiWarpingAccuracy+1;
	Int iref_width = m_rctRefFrameY.right - m_rctRefFrameY.left;
	Int iref_height = m_rctRefFrameY.bottom - m_rctRefFrameY.top;
	Int iref_left = ((m_rctRefVOPY0.left/2) + EXPANDUV_REFVOP)<<iWarpingAccuracyp1;
	Int iref_top = ((m_rctRefVOPY0.top/2) + EXPANDUV_REFVOP)<<iWarpingAccuracyp1;
	Int iref_right = ((m_rctRefVOPY0.right/2) - EXPANDUV_REFVOP + 15)<<iWarpingAccuracyp1;
	Int iref_bottom = ((m_rctRefVOPY0.bottom/2) - EXPANDUV_REFVOP + 15)<<iWarpingAccuracyp1;
	CU8Image *puciRefU = (CU8Image*) m_pvopcRefQ0 -> getPlane (U_PLANE);
	CU8Image *puciRefV = (CU8Image*) m_pvopcRefQ0 -> getPlane (V_PLANE);
	PixelC* pprevu = (PixelC*) puciRefU -> pixels () +
		(EXPANDUV_REF_FRAME - 8)*m_iFrameWidthUV +EXPANDUV_REF_FRAME - 8;
	PixelC* pprevv = (PixelC*) puciRefV -> pixels () +
		(EXPANDUV_REF_FRAME - 8)*m_iFrameWidthUV +EXPANDUV_REF_FRAME - 8;
	Int xi0, yj0;
	Int xi0prime, yj0prime;
	Int xi,yj;
	Int cxx, cyy, cxy, ipa, ipe, ioffset;
	Int ibias, isqr_wac;
	Int irx, iry;
	Int cxx_w, cyy_w;
	
	Int iPowerWA, iwarpmask;
	
	iref_width = iref_width >>1;
	iref_height = iref_height >>1;
	
	iPowerWA = 1 << iWarpingAccuracyp1;
	iwarpmask = iPowerWA - 1;
	isqr_wac = iWarpingAccuracyp1 << 1;
	ibias = 1 << (isqr_wac - 1);
	
	xi0 = m_rctCurrVOPY.left << m_uiWarpingAccuracy;
	yj0 = m_rctCurrVOPY.top << m_uiWarpingAccuracy;
	Int cx_curr = cx_curry>> 1;
	Int cy_curr = cy_curry>> 1;
	
	xi0prime = (Int)(m_rgstDstQ[0].x * 2.0) ;
	yj0prime = (Int)(m_rgstDstQ[0].y * 2.0) ;
	
	xi0prime = xi0prime << m_uiWarpingAccuracy;
	yj0prime = yj0prime << m_uiWarpingAccuracy;
	
	cxx = cxy = ((xi0prime >> 1) | (xi0prime & 1)) + (8<<iWarpingAccuracyp1) - xi0;
	cyy = ((yj0prime >> 1) | (yj0prime & 1)) + (8<<iWarpingAccuracyp1) - yj0;
	
	ipa = iPowerWA;
	ipe = iPowerWA;
	
	cxy += ipa*cx_curr;
	cyy += ipe*cy_curr;
	
	for (yj = 0; yj < MB_SIZE/2; cyy+=ipe, yj++) {
		for  (xi = 0, cxx=cxy; xi < MB_SIZE/2; cxx+=ipa, xi++) {
			
			cxx_w = cxx; cyy_w = cyy;
			
			if (cxx_w<iref_left||cyy_w<iref_top||cxx_w>iref_right||cyy_w>iref_bottom)
			{
				if(cxx_w<iref_left)cxx_w=iref_left;
				if(cxx_w>iref_right)cxx_w=iref_right;
				if(cyy_w<iref_top)cyy_w=iref_top;
				if(cyy_w>iref_bottom)cyy_w=iref_bottom;
				
			}
			
			ioffset = (cyy_w >> iWarpingAccuracyp1) * iref_width + 
				(cxx_w >> iWarpingAccuracyp1);
			irx = cxx_w & iwarpmask;
			iry = cyy_w & iwarpmask;
			
			ppxlcRefU[xi+yj*BLOCK_SIZE] = CInterpolatePixelValue(pprevu, ioffset,
				iref_width, irx, iry, iPowerWA,
				ibias, isqr_wac);
			ppxlcRefV[xi+yj*BLOCK_SIZE] = CInterpolatePixelValue(pprevv, ioffset,
				iref_width, irx, iry, iPowerWA,
				ibias, isqr_wac);
		}
	}
	
}


Void CVideoObject::FastAffineWarpForGMC(Int cx_curr, Int cy_curr,
										PixelC *ppxlcRef, const PixelC *puciRef)
										
{
	Int iWarpingAccuracyp1 = m_uiWarpingAccuracy+1;
	Int iref_width = m_rctRefFrameY.right - m_rctRefFrameY.left;
	//	Int iref_height = m_rctRefFrameY.bottom - m_rctRefFrameY.top;
	Int iref_left = (m_rctRefVOPY0.left + EXPANDY_REFVOP)<<iWarpingAccuracyp1;
	Int iref_top = (m_rctRefVOPY0.top + EXPANDY_REFVOP)<<iWarpingAccuracyp1;
	Int iref_right = (m_rctRefVOPY0.right - EXPANDY_REFVOP + 31)<<iWarpingAccuracyp1;
	Int iref_bottom = (m_rctRefVOPY0.bottom - EXPANDY_REFVOP + 31)<<iWarpingAccuracyp1;
	PixelC* pprev = (PixelC*) puciRef +
		(EXPANDY_REF_FRAME-16)*m_iFrameWidthY +EXPANDY_REF_FRAME-16;
	Int xi0, yj0, xi1, yj1, xi2 = 0, yj2 = 0;
	Int xi0prime, yj0prime, xi1prime = 0, yj1prime, xi2prime = 0, yj2prime  =0;
	Int ipa, ipb, ipc, ipd, ipe, ipf;
	Int ipa_i, ipa_f, ipb_i, ipb_f, ipd_i, ipd_f, ipe_i, ipe_f;
	Int iWidth, iHight = 0, iWidthHight;
	Int xi, yj;
	Int xi1dprime, yj1dprime, xi2dprime = 0, yj2dprime = 0;
	Int iVW, iVH = 0, iVWH = 0;
	Int ivw_pwr, ivh_pwr = 0, ivwh_pwr = 0;
	Int ir_pwr, irat;
	Int cxy, cyy, idnm_pwr, ifracmask;
	Int cxx_i, cxx_f, cyx_i, cyx_f, cxy_i, cxy_f, cyy_i, cyy_f, ioffset;
	Int cxx_w, cyx_w;
	
	Int irx, iry;
	Int ibias, isqr_wac;
	
	Int iPowerWA, iwarpmask;
	
	iPowerWA = 1 << iWarpingAccuracyp1;
	iwarpmask = iPowerWA - 1;
	ir_pwr = 4 - iWarpingAccuracyp1;
	irat = 1<<(4 - iWarpingAccuracyp1);
	isqr_wac = iWarpingAccuracyp1 << 1;
	ibias = 1 << (isqr_wac - 1);
	
	xi0 = m_rctCurrVOPY.left ;
	yj0 = m_rctCurrVOPY.top ;
	xi1 = m_rctCurrVOPY.right ;
	yj1 = m_rctCurrVOPY.top ;
	xi0prime = (Int)((m_rgstDstQ[0].x) * 2.0) ;
	yj0prime = (Int)((m_rgstDstQ[0].y) * 2.0) ;
	xi1prime = (Int)((m_rgstDstQ[1].x) * 2.0) ;
	yj1prime = (Int)((m_rgstDstQ[1].y) * 2.0) ;
	
	xi0prime = xi0prime << 3;
	yj0prime = yj0prime << 3;
	xi1prime = xi1prime << 3;
	yj1prime = yj1prime << 3;
	
	if(m_iNumOfPnts == 3) {
		xi2 = m_rctCurrVOPY.left ;
		yj2 = m_rctCurrVOPY.bottom ;
		
		xi2prime = (Int)((m_rgstDstQ[2].x) * 2.0) ;
		yj2prime = (Int)((m_rgstDstQ[2].y) * 2.0) ;
		
		xi2prime = xi2prime << 3;
		yj2prime = yj2prime << 3;
	}
	
	iWidth = xi1 - xi0;
	
	iVW = 1; ivw_pwr = 0;
	while(iVW < iWidth) {
		iVW <<= 1;
		ivw_pwr++;
	}
	
	if(m_iNumOfPnts == 3) {
		iHight = yj2 - yj0;
		iWidthHight = iWidth*iHight;
		
		iVH = 1; ivh_pwr = 0;
		while(iVH < iHight) {
			iVH <<= 1;
			ivh_pwr++;
		}
		iVWH = iVW * iVH;
		ivwh_pwr = ivw_pwr + ivh_pwr;
	}
	
	xi1dprime = LinearExtrapolation(xi0, xi1, xi0prime, xi1prime, iWidth, iVW) + ((xi0+iVW)<<4);
	yj1dprime = LinearExtrapolation(yj0, yj1, yj0prime, yj1prime, iWidth, iVW) + (yj0<<4);
	if(m_iNumOfPnts == 3) {
		xi2dprime = LinearExtrapolation(xi0, xi2, xi0prime, xi2prime, iHight, iVH) + (xi0<<4);
		yj2dprime = LinearExtrapolation(yj0, yj2, yj0prime, yj2prime, iHight, iVH) + ((yj0+iVH)<<4);
	}
	
	xi0prime += 256;
	yj0prime += 256;
	xi1prime = xi1dprime + 256;
	yj1prime = yj1dprime + 256;
	if(m_iNumOfPnts == 3) {
		xi2prime = xi2dprime + 256;
		yj2prime = yj2dprime + 256;
	}
	
	
	if(m_iNumOfPnts == 3) {
		if(ivw_pwr<=ivh_pwr)
		{
			iVH /= iVW;
			iVWH /= iVW;
			iVW=1;
			ivh_pwr -= ivw_pwr;
			ivwh_pwr -= ivw_pwr;
			ivw_pwr=0;
		}
		else
		{
			iVW /= iVH;
			iVWH /= iVH;
			iVH=1;
			ivw_pwr -= ivh_pwr;
			ivwh_pwr -= ivh_pwr;
			ivh_pwr=0;
		}
	}
	
	if(m_iNumOfPnts == 2) {
		ipa = xi1prime - xi0prime;
		ipb = -(yj1prime - yj0prime);
		ipc = xi0prime * iVW;
		ipd = -ipb;
		ipe = ipa;
		ipf = yj0prime * iVW;
		cxy = ipc + irat*iVW/2;
		cyy = ipf + irat*iVW/2;
		idnm_pwr = ir_pwr+ivw_pwr;
	} else {
		ipa = (xi1prime - xi0prime) * iVH;
		ipb = (xi2prime - xi0prime) * iVW;
		ipc = xi0prime * iVWH;
		ipd = (yj1prime - yj0prime) * iVH;
		ipe = (yj2prime - yj0prime) * iVW;
		ipf = yj0prime * iVWH;
		cxy = ipc + irat*iVWH/2;
		cyy = ipf + irat*iVWH/2;
		idnm_pwr = ir_pwr+ivwh_pwr;
	}
	
	cxy += ipb * (cy_curr - yj0) + ipa * (cx_curr -xi0);
	cyy += ipe * (cy_curr - yj0) + ipd * (cx_curr -xi0);
	
	FourSlashesShift(cxy, idnm_pwr, &cxy_i, &cxy_f);
	FourSlashesShift(cyy, idnm_pwr, &cyy_i, &cyy_f);
	FourSlashesShift(ipa, idnm_pwr, &ipa_i, &ipa_f);
	FourSlashesShift(ipb, idnm_pwr, &ipb_i, &ipb_f);
	FourSlashesShift(ipd, idnm_pwr, &ipd_i, &ipd_f);
	FourSlashesShift(ipe, idnm_pwr, &ipe_i, &ipe_f);
	ifracmask = (1 << idnm_pwr) - 1;
	
	for (yj = 0; yj < MB_SIZE;
	cxy_i+=ipb_i, cxy_f+=ipb_f, cyy_i+=ipe_i, cyy_f+=ipe_f, yj++) {
		cxy_i += cxy_f >> idnm_pwr;
		cyy_i += cyy_f >> idnm_pwr;
		cxy_f &= ifracmask;
		cyy_f &= ifracmask;
		for (xi = 0, cxx_i = cxy_i, cxx_f = cxy_f, cyx_i= cyy_i, cyx_f= cyy_f;
		xi < MB_SIZE;
		cxx_i+=ipa_i, cxx_f+=ipa_f, cyx_i+=ipd_i, cyx_f+=ipd_f, xi++) {
			cxx_i += cxx_f >> idnm_pwr;
			cyx_i += cyx_f >> idnm_pwr;
			cxx_f &= ifracmask;
			cyx_f &= ifracmask;
			cxx_w = cxx_i;
			cyx_w = cyx_i;
			
			if (cxx_w<iref_left||cyx_w<iref_top||cxx_w>iref_right||cyx_w>iref_bottom)
			{
				if(cxx_w<=iref_left)cxx_w=iref_left;
				if(cxx_w>iref_right)cxx_w=iref_right;
				if(cyx_w<=iref_top)cyx_w=iref_top;
				if(cyx_w>iref_bottom)cyx_w=iref_bottom;
			}
			ioffset=(cyx_w>>iWarpingAccuracyp1)*iref_width + (cxx_w>>iWarpingAccuracyp1);
			
			irx = cxx_w & iwarpmask;
			iry = cyx_w & iwarpmask;
			
			ppxlcRef[xi+yj*MB_SIZE] = CInterpolatePixelValue(pprev, ioffset,
				iref_width, irx, iry, iPowerWA,
				ibias, isqr_wac);
			
		}
	}
	
}

Void CVideoObject::FastAffineWarpChromForGMC(Int cx_curry, Int cy_curry,
											 PixelC *ppxlcRefU, PixelC *ppxlcRefV)
{
	Int iWarpingAccuracyp1 = m_uiWarpingAccuracy+1;
	Int iref_width = m_rctRefFrameY.right - m_rctRefFrameY.left;
	Int iref_height = m_rctRefFrameY.bottom - m_rctRefFrameY.top;
	
	Int iref_left = ((m_rctRefVOPY0.left/2) + EXPANDUV_REFVOP)<<iWarpingAccuracyp1;
	Int iref_top = ((m_rctRefVOPY0.top/2) + EXPANDUV_REFVOP)<<iWarpingAccuracyp1;
	Int iref_right = ((m_rctRefVOPY0.right/2) - EXPANDUV_REFVOP + 15)<<iWarpingAccuracyp1;
	Int iref_bottom = ((m_rctRefVOPY0.bottom/2) - EXPANDUV_REFVOP + 15)<<iWarpingAccuracyp1;
	CU8Image *puciRefU = (CU8Image*) m_pvopcRefQ0 -> getPlane (U_PLANE);
	CU8Image *puciRefV = (CU8Image*) m_pvopcRefQ0 -> getPlane (V_PLANE);
	PixelC* pprevu = (PixelC*) puciRefU -> pixels () +
		(EXPANDUV_REF_FRAME - 8)*m_iFrameWidthUV +EXPANDUV_REF_FRAME - 8;
	PixelC* pprevv = (PixelC*) puciRefV -> pixels () +
		(EXPANDUV_REF_FRAME - 8)*m_iFrameWidthUV +EXPANDUV_REF_FRAME - 8;
	Int xi0, yj0, xi1, yj1, xi2 = 0, yj2 = 0;
	Int xi0prime, yj0prime, xi1prime, yj1prime, xi2prime= 0, yj2prime = 0;
	Int ipa, ipb, ipc, ipd, ipe, ipf;
	Int ipa_i, ipa_f, ipb_i, ipb_f, ipd_i, ipd_f, ipe_i, ipe_f;
	Int iWidth, iHight = 0, iWidthHight;
	Int xi, yj;
	Int xi1dprime, yj1dprime, xi2dprime = 0, yj2dprime = 0;
	Int iVW, iVH = 0, iVWH = 0;
	Int ivw_pwr, ivh_pwr = 0, ivwh_pwr = 0;
	Int ir_pwr, irat;
	Int cxy, cyy, idnm_pwr, ifracmask;
	Int cxx_i, cxx_f, cyx_i, cyx_f, cxy_i, cxy_f, cyy_i, cyy_f, ioffset;
	Int cxx_w, cyx_w;
	
	
	Int irx, iry;
	Int ibias, isqr_wac;
	
	Int iPowerWA, iwarpmask;
	
	iref_width = iref_width >>1;
	iref_height = iref_height >>1;
	
	iPowerWA = 1 << iWarpingAccuracyp1;
	iwarpmask = iPowerWA - 1;
	ir_pwr = 4 - iWarpingAccuracyp1;
	irat = 1<<(4 - iWarpingAccuracyp1);
	isqr_wac = iWarpingAccuracyp1 << 1;
	ibias = 1 << (isqr_wac - 1);
	
	
	xi0 = m_rctCurrVOPY.left ;
	yj0 = m_rctCurrVOPY.top ;
	xi1 = m_rctCurrVOPY.right ;
	yj1 = m_rctCurrVOPY.top ;
	
	Int cx_curr = cx_curry-xi0;
	Int cy_curr = cy_curry-yj0;
	
	
	xi0prime = (Int)((m_rgstDstQ[0].x) * 2.0) ;
	yj0prime = (Int)((m_rgstDstQ[0].y) * 2.0) ;
	xi1prime = (Int)((m_rgstDstQ[1].x) * 2.0) ;
	yj1prime = (Int)((m_rgstDstQ[1].y) * 2.0) ;
	
	xi0prime = xi0prime << 3;
	yj0prime = yj0prime << 3;
	xi1prime = xi1prime << 3;
	yj1prime = yj1prime << 3;
	
	if(m_iNumOfPnts == 3) {
		xi2 = m_rctCurrVOPY.left ;
		yj2 = m_rctCurrVOPY.bottom ;
		
		xi2prime = (Int)((m_rgstDstQ[2].x) * 2.0) ;
		yj2prime = (Int)((m_rgstDstQ[2].y) * 2.0) ;
		
		xi2prime = xi2prime << 3;
		yj2prime = yj2prime << 3;
	}
	
	iWidth = xi1 - xi0;
	
	iVW = 1; ivw_pwr = 0;
	while(iVW < iWidth) {
		iVW <<= 1;
		ivw_pwr++;
	}
	
	if(m_iNumOfPnts == 3) {
		iHight = yj2 - yj0;
		iWidthHight = iWidth*iHight;
		
		iVH = 1; ivh_pwr = 0;
		while(iVH < iHight) {
			iVH <<= 1;
			ivh_pwr++;
		}
		iVWH = iVW * iVH;
		ivwh_pwr = ivw_pwr + ivh_pwr;
	}
	
	xi1dprime = LinearExtrapolation(xi0, xi1, xi0prime, xi1prime, iWidth, iVW) + ((xi0+iVW)<<4);
	yj1dprime = LinearExtrapolation(yj0, yj1, yj0prime, yj1prime, iWidth, iVW) + (yj0<<4);
	if(m_iNumOfPnts == 3) {
		xi2dprime = LinearExtrapolation(xi0, xi2, xi0prime, xi2prime, iHight, iVH) + (xi0<<4);
		yj2dprime = LinearExtrapolation(yj0, yj2, yj0prime, yj2prime, iHight, iVH) + ((yj0+iVH)<<4);
	}
	
	xi0prime += 256;
	yj0prime += 256;
	xi1prime = xi1dprime + 256;
	yj1prime = yj1dprime + 256;
	if(m_iNumOfPnts == 3) {
		xi2prime = xi2dprime + 256;
		yj2prime = yj2dprime + 256;
	}
	
	if(m_iNumOfPnts == 3) {
		if(ivw_pwr<=ivh_pwr)
		{
			iVH /= iVW;
			iVWH /= iVW;
			iVW=1;
			ivh_pwr -= ivw_pwr;
			ivwh_pwr -= ivw_pwr;
			ivw_pwr=0;
		}
		else
		{
			iVW /= iVH;
			iVWH /= iVH;
			iVH=1;
			ivw_pwr -= ivh_pwr;
			ivwh_pwr -= ivh_pwr;
			ivh_pwr=0;
		}
	}
	
	if(m_iNumOfPnts == 2) {
		ipa = xi1prime - xi0prime;
		ipb = -(yj1prime - yj0prime);
		ipc = 2 * iVW * xi0prime - 16*iVW;
		ipd = -ipb;
		ipe = ipa;
		ipf = 2 * iVW * yj0prime - 16*iVW;
		cxy = ipa + ipb + ipc + 2*irat*iVW;
		cyy = ipd + ipe + ipf + 2*irat*iVW;
		idnm_pwr = ir_pwr+ivw_pwr+2;
	} else {
		ipa = (xi1prime - xi0prime) * iVH;
		ipb = (xi2prime - xi0prime) * iVW;
		ipc = 2 * iVWH * xi0prime - 16*iVWH;
		ipd = (yj1prime - yj0prime) * iVH;
		ipe = (yj2prime - yj0prime) * iVW;
		ipf = 2 * iVWH * yj0prime - 16*iVWH;
		cxy = ipa + ipb + ipc + 2*irat*iVWH;
		cyy = ipd + ipe + ipf + 2*irat*iVWH;
		idnm_pwr = ir_pwr+ivwh_pwr+2;
	}
	
	cxy += (ipb * cy_curr + ipa * cx_curr)<<1; 
	cyy += (ipe * cy_curr + ipd * cx_curr)<<1; 
	
	FourSlashesShift(ipa, idnm_pwr-2, &ipa_i, &ipa_f);
	FourSlashesShift(ipb, idnm_pwr-2, &ipb_i, &ipb_f);
	FourSlashesShift(ipd, idnm_pwr-2, &ipd_i, &ipd_f);
	FourSlashesShift(ipe, idnm_pwr-2, &ipe_i, &ipe_f);
	ipa_f *= 4;
	ipb_f *= 4;
	ipd_f *= 4;
	ipe_f *= 4;
	FourSlashesShift(cxy, idnm_pwr, &cxy_i, &cxy_f);
	FourSlashesShift(cyy, idnm_pwr, &cyy_i, &cyy_f);
	ifracmask = (1 << idnm_pwr) - 1;
	
	for (yj = 0; yj < MB_SIZE/2;
	cxy_i+=ipb_i, cxy_f+=ipb_f, cyy_i+=ipe_i, cyy_f+=ipe_f, yj++) {
		cxy_i += cxy_f >> idnm_pwr;
		cyy_i += cyy_f >> idnm_pwr;
		cxy_f &= ifracmask;
		cyy_f &= ifracmask;
		for (xi = 0, cxx_i = cxy_i, cxx_f = cxy_f, cyx_i= cyy_i, cyx_f= cyy_f;
		xi < MB_SIZE/2;
		cxx_i+=ipa_i, cxx_f+=ipa_f, cyx_i+=ipd_i, cyx_f+=ipd_f, xi++) {
			cxx_i += (cxx_f >> idnm_pwr);
			cyx_i += (cyx_f >> idnm_pwr);
			cxx_f &= ifracmask;
			cyx_f &= ifracmask;
			cxx_w = cxx_i;
			cyx_w = cyx_i;
			
			if (cxx_w<iref_left||cyx_w<iref_top||cxx_w>iref_right||cyx_w>iref_bottom)
			{
				if(cxx_w<=iref_left)cxx_w=iref_left;
				if(cxx_w>iref_right)cxx_w=iref_right;
				if(cyx_w<=iref_top)cyx_w=iref_top;
				if(cyx_w>iref_bottom)cyx_w=iref_bottom;
			}
			
			ioffset = (cyx_w>>iWarpingAccuracyp1)*iref_width+(cxx_w>>iWarpingAccuracyp1);
			
			irx = cxx_w & iwarpmask;
			iry = cyx_w & iwarpmask;
			
			ppxlcRefU[xi+yj*BLOCK_SIZE] = CInterpolatePixelValue(pprevu, ioffset,
				iref_width, irx, iry, iPowerWA,
				ibias, isqr_wac);
			ppxlcRefV[xi+yj*BLOCK_SIZE] = CInterpolatePixelValue(pprevv, ioffset,
				iref_width, irx, iry, iPowerWA,
				ibias, isqr_wac);
		}
	}
	
}


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

	gmc_enc_util.cpp

Abstract:

	Calculates Sum of Absolute Difference of GMC macroblock.

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
#include "vopseenc.hpp"


Int CVideoObjectEncoder::globalme(CoordI iXCurr, CoordI iYCurr,
                                  PixelC *pref) 
{
	Int iSumofAD = 0;
	Int iref_width = m_rctRefFrameY.right - m_rctRefFrameY.left;
	
	
	pref += (EXPANDY_REF_FRAME - 16) * iref_width + (EXPANDY_REF_FRAME - 16);
	
	switch(m_iNumOfPnts) {
	case 0:
		StationalWarpME(iXCurr, iYCurr, pref,
			iSumofAD);
		break;
	case 1:
		TranslationalWarpME(iXCurr, iYCurr, pref,
			iSumofAD);
		break;
	case 2:
	case 3:
		FastAffineWarpME(iXCurr, iYCurr, pref, 
			iSumofAD);
		break;
	default:
		assert(m_iNumOfPnts<=3);
	}
	return(iSumofAD);
}


Void CVideoObjectEncoder::StationalWarpME(Int iXCurr, Int iYCurr, 
										  PixelC* pref, 
										  Int& iSumofAD)
{
	Int iref_width = m_rctRefFrameY.right - m_rctRefFrameY.left;
	//Int iref_height = m_rctRefFrameY.bottom - m_rctRefFrameY.top;
	Int iref_left = (m_rctRefVOPY0.left + EXPANDY_REFVOP);
	Int iref_top = (m_rctRefVOPY0.top + EXPANDY_REFVOP);
	Int iref_right = (m_rctRefVOPY0.right - EXPANDY_REFVOP + 31);
	Int iref_bottom = (m_rctRefVOPY0.bottom - EXPANDY_REFVOP + 31);
	
	Int    idest;
	Int    xi0, yj0;
	
	Int    cx_curr = iXCurr,
		cy_curr = iYCurr;
	
	Int xi,yj, xTI, yTJ;
	
	iSumofAD = 0;
	
	xi0 = m_rctCurrVOPY.left ;
	yj0 = m_rctCurrVOPY.top ;
	
	
	const PixelC* ppxlcTmpC = m_ppxlcCurrMBY;
	const PixelC* ppxlcTmpA = (m_volmd.fAUsage == RECTANGLE ? (PixelC*)NULL : m_ppxlcCurrMBBY);
	
	if(m_volmd.fAUsage == RECTANGLE){
		for (yj = 0, yTJ = cy_curr; yj < MB_SIZE; yj++, yTJ++) {
			for (xi = 0, xTI = cx_curr; xi < MB_SIZE; xi++, xTI++) {
				
				
				if (xTI<iref_left||yTJ<iref_top||xTI>iref_right||
					yTJ>iref_bottom)
				{
					iSumofAD = 100000;
					return;
				}
				else
				{
					idest = pref[yTJ * iref_width + xTI];
					iSumofAD += abs(idest - ppxlcTmpC [xi]);
				}
				
			}
			ppxlcTmpC += MB_SIZE;
		}
		
	}else{
		
		for (yj = 0, yTJ = yj0 + cy_curr; yj < MB_SIZE; yj++, yTJ++) {
			for (xi = 0, xTI = xi0 + cx_curr; xi < MB_SIZE; xi++, xTI++) {
				
				if (ppxlcTmpA[xi])
				{
					
					if (xTI<iref_left||yTJ<iref_top||xTI>iref_right||
						yTJ>iref_bottom)
					{
						iSumofAD = 100000;
						return;
					}
					else
					{
						idest = pref[yTJ * iref_width + xTI];
						iSumofAD += abs(idest - ppxlcTmpC [xi]);
					}
					
				}
			}
			ppxlcTmpC += MB_SIZE;
			ppxlcTmpA += MB_SIZE;
		}
		
	}
	
	return;
}


Void CVideoObjectEncoder::TranslationalWarpME(Int iXCurr, Int iYCurr, 
											  PixelC* pref, 
											  Int& iSumofAD)
{
	Int iWarpingAccuracyp1 = m_uiWarpingAccuracy+1;
	Int iref_width = m_rctRefFrameY.right - m_rctRefFrameY.left;
	//	Int iref_height = m_rctRefFrameY.bottom - m_rctRefFrameY.top;
	Int iref_left = (m_rctRefVOPY0.left + EXPANDY_REFVOP)<<iWarpingAccuracyp1;
	Int iref_top = (m_rctRefVOPY0.top + EXPANDY_REFVOP)<<iWarpingAccuracyp1;
	Int iref_right = (m_rctRefVOPY0.right - EXPANDY_REFVOP + 31)<<iWarpingAccuracyp1;
	Int iref_bottom = (m_rctRefVOPY0.bottom - EXPANDY_REFVOP + 31)<<iWarpingAccuracyp1;
	
	Int    idest;
	
	Int    cx_curr = iXCurr,
		cy_curr = iYCurr;
	
	Int xi0, yj0;
	Int xi0prime, yj0prime;
	Int xi,yj;
	Int cxx, cyy, ipa, ipe, ioffset;
	Int ibias, isqr_wac;
	Int irx, iry;
	
	Int iPowerWA, iwarpmask;
	
	iSumofAD = 0;
	
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
	
	const PixelC* ppxlcTmpC = m_ppxlcCurrMBY;
	const PixelC* ppxlcTmpA = (m_volmd.fAUsage == RECTANGLE ? (PixelC*)NULL : m_ppxlcCurrMBBY);
	
	if(m_volmd.fAUsage == RECTANGLE){
		
		for (yj = 0; yj < MB_SIZE; cyy+=ipe, yj++) {
			for (xi = 0, cxx=xi0prime; xi < MB_SIZE; cxx+=ipa, xi++) {
				
				if (cxx<iref_left||cyy<iref_top||cxx>iref_right||
					cyy>iref_bottom)
				{
					iSumofAD = 100000;
					return;
				}
				else
				{
					
					ioffset = (cyy >> iWarpingAccuracyp1) * iref_width 
						+ (cxx >> iWarpingAccuracyp1);
					irx = cxx & iwarpmask;
					iry = cyy & iwarpmask;
					
					idest = CInterpolatePixelValue(pref, ioffset,
						iref_width, irx, iry, iPowerWA,
						ibias, isqr_wac);
					iSumofAD += abs(idest - ppxlcTmpC [xi]);
				}
			}
			ppxlcTmpC += MB_SIZE;
		}
		
	}else{
		
		for (yj = 0; yj < MB_SIZE; cyy+=ipe, yj++) {
			for (xi = 0, cxx=xi0prime; xi < MB_SIZE; cxx+=ipa, xi++) {
				
				if (ppxlcTmpA[xi])
				{
					
					if (cxx<iref_left||cyy<iref_top||cxx>iref_right||
						cyy>iref_bottom)
					{
						iSumofAD = 100000;
						return;
					}
					else
					{
						
						ioffset = (cyy >> iWarpingAccuracyp1) * iref_width 
							+ (cxx >> iWarpingAccuracyp1);
						irx = cxx & iwarpmask;
						iry = cyy & iwarpmask;
						
						idest = CInterpolatePixelValue(pref, ioffset,
							iref_width, irx, iry, iPowerWA,
							ibias, isqr_wac);
						iSumofAD += abs(idest - ppxlcTmpC [xi]);
					}
				}
			}
			ppxlcTmpC += MB_SIZE;
			ppxlcTmpA += MB_SIZE;
		}
	}
	return;
}


Void CVideoObjectEncoder::FastAffineWarpME(Int iXCurr, Int iYCurr, 
										   PixelC* pref, 
										   Int& iSumofAD)
{
	Int iWarpingAccuracyp1 = m_uiWarpingAccuracy+1;
	Int iref_width = m_rctRefFrameY.right - m_rctRefFrameY.left;
	//	Int iref_height = m_rctRefFrameY.bottom - m_rctRefFrameY.top;
	Int iref_left = (m_rctRefVOPY0.left + EXPANDY_REFVOP)<<iWarpingAccuracyp1;
	Int iref_top = (m_rctRefVOPY0.top + EXPANDY_REFVOP)<<iWarpingAccuracyp1;
	Int iref_right = (m_rctRefVOPY0.right - EXPANDY_REFVOP + 31)<<iWarpingAccuracyp1;
	Int iref_bottom = (m_rctRefVOPY0.bottom - EXPANDY_REFVOP + 31)<<iWarpingAccuracyp1;
	
	Int    idest;
	
	Int    cx_curr = iXCurr,
		cy_curr = iYCurr;
	
	Int xi0, yj0, xi1, yj1, xi2 = 0, yj2 = 0;
	Int xi0prime, yj0prime, xi1prime, yj1prime, xi2prime = 0, yj2prime = 0;
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
	
	Int irx, iry;
	Int ibias, isqr_wac;
	
	Int iPowerWA, iwarpmask;
	
	iSumofAD = 0;
	
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
	
	cxy += ipb * (cy_curr - yj0) + ipa * (cx_curr - xi0);
	cyy += ipe * (cy_curr - yj0) + ipd * (cx_curr - xi0);
	
	FourSlashesShift(cxy, idnm_pwr, &cxy_i, &cxy_f);
	FourSlashesShift(cyy, idnm_pwr, &cyy_i, &cyy_f);
	FourSlashesShift(ipa, idnm_pwr, &ipa_i, &ipa_f);
	FourSlashesShift(ipb, idnm_pwr, &ipb_i, &ipb_f);
	FourSlashesShift(ipd, idnm_pwr, &ipd_i, &ipd_f);
	FourSlashesShift(ipe, idnm_pwr, &ipe_i, &ipe_f);
	ifracmask = (1 << idnm_pwr) - 1;
	
	const PixelC* ppxlcTmpC = m_ppxlcCurrMBY;
	const PixelC* ppxlcTmpA = (m_volmd.fAUsage == RECTANGLE ? (PixelC*)NULL : m_ppxlcCurrMBBY);
	
	if(m_volmd.fAUsage == RECTANGLE){
		
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
				
				if (cxx_i<iref_left || cyx_i<iref_top || 
					cxx_i>iref_right||
					cyx_i>iref_bottom)
				{
					iSumofAD = 100000;
					return;
				}
				else
				{
					ioffset=(cyx_i>>iWarpingAccuracyp1)*iref_width + (cxx_i>>iWarpingAccuracyp1);
					
					irx = cxx_i & iwarpmask;
					iry = cyx_i & iwarpmask;
					
					idest = CInterpolatePixelValue(pref, ioffset,
						iref_width, irx, iry, iPowerWA,
						ibias, isqr_wac);
					iSumofAD += abs(idest - ppxlcTmpC[xi]);
				}
			}
			ppxlcTmpC += MB_SIZE;
		}
		
	}else{
		
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
				
				if (ppxlcTmpA[xi])
				{
					
					if (cxx_i<iref_left || cyx_i<iref_top || 
						cxx_i>iref_right||
						cyx_i>iref_bottom)
					{
						iSumofAD = 100000;
						return;
					}
					else
					{
						ioffset=(cyx_i>>iWarpingAccuracyp1)*iref_width + (cxx_i>>iWarpingAccuracyp1);
						
						irx = cxx_i & iwarpmask;
						iry = cyx_i & iwarpmask;
						
						idest = CInterpolatePixelValue(pref, ioffset,
							iref_width, irx, iry, iPowerWA,
							ibias, isqr_wac);
						iSumofAD += abs(idest - ppxlcTmpC[xi]);
					}
				}
			}
			ppxlcTmpC += MB_SIZE;
			ppxlcTmpA += MB_SIZE;
		}
		
	}
	return;
}


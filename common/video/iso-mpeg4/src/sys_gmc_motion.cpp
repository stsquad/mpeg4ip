/***************************************************************************

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

	gmc_motion.cpp

Abstract:

	Motion vector decoding for GMC blocks.

**************************************************************************/

#include <stdio.h>
#include <math.h>
#include <stdlib.h>
//#include <iostream.h>
#include <assert.h>

#include "typeapi.h"
#include "codehead.h"
#include "global.hpp"
#include "bitstrm.hpp"
#include "entropy.hpp"
#include "huffman.hpp"
#include "mode.hpp"
#include "vopses.hpp"


Void CVideoObject::FourSlashesShift(Int inum, Int idenom_pwr, Int *ipquot, Int *ipres)
{
	*ipquot =  abs(inum) >> idenom_pwr;
	if(inum < 0)
		*ipquot = -*ipquot;
	if(*ipquot << idenom_pwr == inum)
		*ipres = 0;
	else {
		if(inum < 0)
			*ipquot -= 1;
		*ipres = inum - (*ipquot << idenom_pwr);
	}
}


Void CVideoObject::globalmv(Int& iXpel, Int& iYpel, 
                            Int& iXhalf, Int& iYhalf,
                            CoordI iXCurr, CoordI iYCurr,
                            Int iSearchRenge, Bool bQuarterSample)
{
	Int ifcode_area= iSearchRenge;
	
	switch(m_iNumOfPnts) {
	case 0:
		iXpel = iYpel = iXhalf = iYhalf = 0;
		break;
	case 1:
		TranslationalWarpMotion(iXpel, iYpel, iXhalf, iYhalf,iSearchRenge,bQuarterSample);
		break;
	case 2:
	case 3:
		FastAffineWarpMotion(iXpel, iYpel, iXhalf, iYhalf,
			iXCurr, iYCurr,
			ifcode_area, bQuarterSample);
		break;
	default:
		assert(m_iNumOfPnts<=3);
	}
}


Void CVideoObject::TranslationalWarpMotion(Int& iXpel, Int& iYpel, 
										   Int& iXhalf, Int& iYhalf, Int iSearchRenge, Bool bQuarterSample)
{
	Int xi0, yj0;
	Int xi0prime, yj0prime;
	Int ifcode_area;
	Int ix_vec, iy_vec;
	
	xi0 = m_rctCurrVOPY.left ;
	yj0 = m_rctCurrVOPY.top ;
	
	if(bQuarterSample){
        ifcode_area= iSearchRenge/2;
		xi0prime = (Int)((m_rgstDstQ[0].x) * 4.0) ;
		yj0prime = (Int)((m_rgstDstQ[0].y) * 4.0) ;
		ix_vec = xi0prime - xi0*4;
		iy_vec = yj0prime - yj0*4;
	}else{
        ifcode_area= iSearchRenge;
		xi0prime = (Int)((m_rgstDstQ[0].x) * 2.0) ;
		yj0prime = (Int)((m_rgstDstQ[0].y) * 2.0) ;
		ix_vec = xi0prime - xi0*2;
		iy_vec = yj0prime - yj0*2;
	}
	
	if(ix_vec<-ifcode_area)ix_vec=-ifcode_area;
	if(ix_vec>=ifcode_area)ix_vec=ifcode_area-1;
	if(iy_vec<-ifcode_area)iy_vec=-ifcode_area;
	if(iy_vec>=ifcode_area)iy_vec=ifcode_area-1;
	
	iXpel = ix_vec/2;
	iYpel = iy_vec/2;
	iXhalf = ix_vec - iXpel*2;
	iYhalf = iy_vec - iYpel*2;
	
}


Void CVideoObject::FastAffineWarpMotion(Int& iXpel, Int& iYpel, 
										Int& iXhalf, Int& iYhalf,
										Int iXCurr, Int iYCurr,
										Int iSearchRenge, Bool bQuarterSample)
{
	Int xi0, yj0, xi1, yj1, xi2 = 0, yj2 = 0;
	Int xi0prime, yj0prime, xi1prime, yj1prime, xi2prime = 0, yj2prime = 0;
	Int ipa, ipb, ipc, ipd, ipe, ipf;
	Int ipa_i, ipa_f, ipb_i, ipb_f, ipd_i, ipd_f, ipe_i, ipe_f;
	Int iWidth, iHight = 0, iWidthHight;
	Int xi1dprime, yj1dprime, xi2dprime = 0, yj2dprime = 0;
	Int iVW, iVH = 0, iVWH = 0;
	Int ivw_pwr, ivh_pwr = 0, ivwh_pwr = 0;
	Int ir_pwr, irat;
	Int cxy, cyy, idnm_pwr, ifracmask;
	Int cxx_i, cxx_f, cyx_i, cyx_f, cxy_i, cxy_f, cyy_i, cyy_f;
	Int crx, cry;
	Int xi,yj, xTI, yTJ;
	
	Int irx, iry;
	
	
	Int cx_curr, cy_curr;
	Int ix_vec, iy_vec;
	Int ifcode_area;
	Int iavg_x, iavg_y, ifavg, ires;
	Int ipp;
	
	Int iWarpingAccuracyp1 = m_uiWarpingAccuracy+1;
	ir_pwr = 4 - iWarpingAccuracyp1;
	irat = 1<<(4 - iWarpingAccuracyp1);
	if(bQuarterSample){
        ifcode_area= iSearchRenge/2;
		ires = 64 << m_uiWarpingAccuracy;
	}else{
        ifcode_area= iSearchRenge;
		ires = 128 << m_uiWarpingAccuracy;
	}
	
	xi0 = m_rctCurrVOPY.left ;
	yj0 = m_rctCurrVOPY.top ;
	xi1 = m_rctCurrVOPY.right ;
	yj1 = m_rctCurrVOPY.top ;
	
	xi0prime = (Int)((m_rgstDstQ[0].x) * 2.0) ;
	yj0prime = (Int)((m_rgstDstQ[0].y) * 2.0) ;
	xi1prime = (Int)((m_rgstDstQ[1].x) * 2.0) ;
	yj1prime = (Int)((m_rgstDstQ[1].y) * 2.0) ;
	
	xi0prime = xi0prime<< 3;
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
	
	xi1prime = xi1dprime ;
	yj1prime = yj1dprime ;
	
	if(m_iNumOfPnts == 3) {
		xi2prime = xi2dprime ;
		yj2prime = yj2dprime ;
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
		cxy = ipc;
		cyy = ipf;
		ipp = irat*iVW/2 ;
		idnm_pwr = ir_pwr+ivw_pwr;
	} else {
		ipa = (xi1prime - xi0prime) * iVH;
		ipb = (xi2prime - xi0prime) * iVW;
		ipc = xi0prime * iVWH;
		ipd = (yj1prime - yj0prime) * iVH;
		ipe = (yj2prime - yj0prime) * iVW;
		ipf = yj0prime * iVWH;
		cxy = ipc;
		cyy = ipf;
		ipp = irat*iVWH/2 ;
		idnm_pwr = ir_pwr+ivwh_pwr;
	}
	
	cx_curr = iXCurr - xi0;
	cy_curr = iYCurr - yj0;
	
	iavg_x = iavg_y = 0 ;
	ifavg = 0;
	
	cxy += ipb * cy_curr + ipa * cx_curr;
	cyy += ipe * cy_curr + ipd * cx_curr;
	
	FourSlashesShift(cxy, idnm_pwr, &cxy_i, &cxy_f);
	FourSlashesShift(cyy, idnm_pwr, &cyy_i, &cyy_f);
	FourSlashesShift(ipa, idnm_pwr, &ipa_i, &ipa_f);
	FourSlashesShift(ipb, idnm_pwr, &ipb_i, &ipb_f);
	FourSlashesShift(ipd, idnm_pwr, &ipd_i, &ipd_f);
	FourSlashesShift(ipe, idnm_pwr, &ipe_i, &ipe_f);
	ifracmask = (1 << idnm_pwr) - 1;
	
	for (yj = 0, yTJ = iYCurr; yj < MB_SIZE;
	cxy_i+=ipb_i, cxy_f+=ipb_f, cyy_i+=ipe_i, cyy_f+=ipe_f, yj++, yTJ++) {
		cxy_i += cxy_f >> idnm_pwr;
		cyy_i += cyy_f >> idnm_pwr;
		cxy_f &= ifracmask;
		cyy_f &= ifracmask;
		for (xi = 0, xTI=iXCurr, cxx_i = cxy_i, cxx_f = cxy_f, cyx_i= cyy_i, 
			cyx_f= cyy_f;xi < MB_SIZE;
		cxx_i+=ipa_i, cxx_f+=ipa_f, cyx_i+=ipd_i, cyx_f+=ipd_f, xi++, xTI++) {
			crx = (cxx_i<<idnm_pwr)+cxx_f;
			cry = (cyx_i<<idnm_pwr)+cyx_f;
			crx = (crx + ipp) >> idnm_pwr;
			cry = (cry + ipp) >> idnm_pwr;
			cxx_i += cxx_f >> idnm_pwr;
			cyx_i += cyx_f >> idnm_pwr;
			cxx_f &= ifracmask;
			cyx_f &= ifracmask;
			
			irx = crx - (xTI<<iWarpingAccuracyp1);
			iry = cry - (yTJ<<iWarpingAccuracyp1);
			
			iavg_x += irx;
			iavg_y += iry;
			ifavg++;
			
		}
	}
	
	if(ifavg!=256){
		fprintf(stderr, "Error : MV PREDICTION (ifavg!=256) \n");
		exit(0);
	}
	
	if(bQuarterSample){
		ix_vec = (iavg_x + (iavg_x<0 ? ires-1 : ires))>>(6+iWarpingAccuracyp1);
		iy_vec = (iavg_y + (iavg_y<0 ? ires-1 : ires))>>(6+iWarpingAccuracyp1);
	}else{
		ix_vec = (iavg_x + (iavg_x<0 ? ires-1 : ires))>>(7+iWarpingAccuracyp1);
		iy_vec = (iavg_y + (iavg_y<0 ? ires-1 : ires))>>(7+iWarpingAccuracyp1);
	}
	
	if(ix_vec<-ifcode_area)ix_vec=-ifcode_area;
	if(ix_vec>=ifcode_area)ix_vec=ifcode_area-1;
	if(iy_vec<-ifcode_area)iy_vec=-ifcode_area;
	if(iy_vec>=ifcode_area)iy_vec=ifcode_area-1;
	
	iXpel = ix_vec/2;
	iYpel = iy_vec/2;
	iXhalf = ix_vec - iXpel*2;
	iYhalf = iy_vec - iYpel*2;
	
}


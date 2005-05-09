/***************************************************************************

This software module was originally developed by

	Yoshinori Suzuki (Hitachi, Ltd.)

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

	gme_for_iso.cpp

Abstruct:

	Global Motion Estimation.

Specification of Parameters:

    dp1 x + dp2 y + dp3    dp4 x + dp5 y + dp6
    -------------------    -------------------
    dp7 x + dp8 y + 1      dp7 x + dp8 y + 1

   d=2  X'i = X + M[0], Y'i = Y + M[1]
   d=4  X'i = M[0]X + M[1]Y + M[2], Y'i = -M[1]X + M[0]Y + M[3]
   d=6  X'i = M[0]X + M[1]Y + M[2], Y'i = M[3]X + M[4]Y + M[5]

 *************************************************************************/


#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include "typeapi.h"
#include "codehead.h"
#include "global.hpp"
#include "bitstrm.hpp"
#include "entropy.hpp"
#include "huffman.hpp"
#include "mode.hpp"
#include "vopses.hpp"
#include "vopseenc.hpp"


Void CVideoObjectEncoder::GlobalMotionEstimation(CSiteD* m_rgstDestQ,
												 const CVOPU8YUVBA* pvopref, const CVOPU8YUVBA* pvopcurr,
												 const CRct rctframe, const CRct rctref,const CRct rctcurr,
												 Int iNumOfPnts)
{
	GME_para *pparameter = new GME_para;
	Double d_num_h, d_num_v;
	Int xi;
	
	CSiteD *m_rgstSrcQ = new CSiteD [4];
	m_rgstSrcQ [0] = CSiteD ((double)rctcurr.left, (double)rctcurr.top);
	m_rgstSrcQ [1] = CSiteD ((double)rctcurr.right, (double)rctcurr.top);
	m_rgstSrcQ [2] = CSiteD ((double)rctcurr.left, (double)rctcurr.bottom);
	m_rgstSrcQ [3] = CSiteD ((double)rctcurr.right, (double)rctcurr.bottom);
	
	switch(iNumOfPnts) {
	case 0:
		pparameter -> dp1 = 1.0;
		pparameter -> dp2 = 0.0;
		pparameter -> dp3 = 0.0;
		pparameter -> dp4 = 0.0;
		pparameter -> dp5 = 1.0;
		pparameter -> dp6 = 0.0;
		pparameter -> dp7 = 0.0;
		pparameter -> dp8 = 0.0;
		break;
	case 1:
		TranslationalGME(pvopcurr, pvopref, rctframe, rctcurr, rctref, pparameter);
		break;
	case 2:
		IsotropicGME(pvopcurr, pvopref, rctframe, rctcurr, rctref, pparameter);
		break;
	case 3:
		AffineGME(pvopcurr, pvopref, rctframe, rctcurr, rctref, pparameter);
		break;
	default:
		assert(iNumOfPnts<=3);
	}
	
	printf("parameterx : %f %f %f\n",pparameter->dp1,pparameter->dp2,pparameter->dp3);
	printf("parametery : %f %f %f\n",pparameter->dp4,pparameter->dp5,pparameter->dp6);
	printf("parameterd : %f %f 1\n",pparameter->dp7,pparameter->dp8);
	
	for (xi=0; xi<iNumOfPnts; xi++)
	{
        d_num_h = pparameter->dp1 * m_rgstSrcQ[xi].x +
			pparameter->dp2 * m_rgstSrcQ[xi].y +
			pparameter->dp3;
        d_num_v = pparameter->dp4 * m_rgstSrcQ[xi].x +
			pparameter->dp5 * m_rgstSrcQ[xi].y +
			pparameter->dp6;
		
        m_rgstDestQ[xi].x = d_num_h;
        m_rgstDestQ[xi].y = d_num_v;
		printf("trajectory[%d],%f,%f to %f %f\n",xi,m_rgstSrcQ[xi].x, m_rgstSrcQ[xi].y, m_rgstDestQ[xi].x,m_rgstDestQ[xi].y);
	}
	delete [] m_rgstSrcQ; m_rgstSrcQ = NULL;
	delete pparameter; pparameter = NULL;
	
}


Void CVideoObjectEncoder::TranslationalGME(const CVOPU8YUVBA* pvopcurr,
										   const CVOPU8YUVBA* pvopref, const CRct rctframe, const CRct rctcurr,
										   const CRct rctref, GME_para *pparameter)
{
	const CU8Image* puciRefY = pvopref -> getPlane (Y_PLANE);
	const CU8Image* puciCurrY = pvopcurr -> getPlane (Y_PLANE);
	PixelC* pref = (PixelC*) puciRefY -> pixels ();
	PixelC* pcurr = (PixelC*) puciCurrY -> pixels ();
	const CU8Image* puciRefA = (pvopref -> fAUsage () == EIGHT_BIT)?
        pvopref -> getPlaneA (0) : 
	((m_volmd.fAUsage == RECTANGLE) ? (CU8Image*)NULL:pvopref -> getPlane (BY_PLANE));
	const CU8Image* puciCurrA = (pvopcurr -> fAUsage () == EIGHT_BIT)?
        pvopcurr -> getPlaneA (0) : 
	((m_volmd.fAUsage == RECTANGLE) ? (CU8Image*)NULL:pvopcurr -> getPlane (BY_PLANE));
	PixelC* pref_alpha = NULL;
	PixelC* pcurr_alpha = NULL;
	
	if(m_volmd.fAUsage != RECTANGLE){
		pref_alpha = (PixelC*) puciRefA -> pixels ();
		pcurr_alpha = (PixelC*) puciCurrA -> pixels ();
	}
	
	PixelC *pref_P0=NULL, *pref_P1=NULL, *pref_P2=NULL;
	PixelC *pcurr_P0=NULL, *pcurr_P1=NULL, *pcurr_P2=NULL;
	PixelC *pref_a_P0=NULL, *pref_a_P1=NULL, *pref_a_P2=NULL;
	PixelC *pcurr_a_P0=NULL, *pcurr_a_P1=NULL, *pcurr_a_P2=NULL;
	PixelC *pref_work, *pcurr_work, *pcurr_alpha_work = NULL, *pref_alpha_work = NULL;
	Int icurr_width, icurr_height, icurr_offset;
	Int icurr_left, icurr_top, icurr_right, icurr_bottom;
	Int iref_width, iref_height;
	Int iref_left, iref_top, iref_right, iref_bottom;
	
	Int xi, yj, itmp, ibest_locationx, ibest_locationy;
	Int icurr_pel, iref_pel;
	Int iref_pel1, iref_pel2, iref_pel3;
	Double *dm = new Double [2];
	Double *db = new Double [2];
	Double *dA = new Double [4];
	Double *dTM = new Double [2];
	Double dE2 = 0;
	Int istop = 0, ilevel = 2, iite = 0;
	Int x, y, x1, y1;
	Double dx1, dy1, dx, dy;
	Double dt, du, dk, d1mk, dl, d1ml,
		dI1, de, dI1dx, dI1dy;
	Double *ddedm = new Double [2];
	Double *dI1x1y1 = new Double [4];
	Int *iperror_histgram = new Int [256];
	Int ithreshold_T=0, icheck=1;
	
	iref_left = ((rctref.left+EXPANDY_REFVOP)) >> 2;
	iref_top = ((rctref.top+EXPANDY_REFVOP)) >> 2;
	iref_right = ((rctref.right-EXPANDY_REFVOP)) >> 2;
	iref_bottom = ((rctref.bottom-EXPANDY_REFVOP)) >> 2;
	iref_width = ((rctframe.right - rctframe.left)) >> 2;
	iref_height = ((rctframe.bottom - rctframe.top)) >> 2;
	
	icurr_left = (rctcurr.left) >> 2;
	icurr_top = (rctcurr.top) >> 2;
	icurr_right = (rctcurr.right) >> 2;
	icurr_bottom = (rctcurr.bottom) >> 2;
	icurr_width = iref_width ;
	icurr_height = iref_height ;
	
	pref_P0 = pref;
	pcurr_P0 = pcurr;
	pref_P1 = new PixelC[iref_width*iref_height*4];
	pcurr_P1 = new PixelC[icurr_width*icurr_height*4];
	pref_P2 = new PixelC[iref_width*iref_height];
	pcurr_P2 = new PixelC[icurr_width*icurr_height];
	ThreeTapFilter(pref_P1, pref_P0, iref_width*4, iref_height*4);
	ThreeTapFilter(pcurr_P1, pcurr_P0, icurr_width*4, icurr_height*4);
	ThreeTapFilter(pref_P2, pref_P1, iref_width*2, iref_height*2);
	ThreeTapFilter(pcurr_P2, pcurr_P1, icurr_width*2, icurr_height*2);
	
	if(m_volmd.fAUsage != RECTANGLE){
		pref_a_P0 = pref_alpha;
		pcurr_a_P0 = pcurr_alpha;
		pref_a_P1 = new PixelC[iref_width*iref_height*4];
		pcurr_a_P1 = new PixelC[icurr_width*icurr_height*4];
		pref_a_P2 = new PixelC[iref_width*iref_height];
		pcurr_a_P2 = new PixelC[icurr_width*icurr_height];
		ThreeTapFilter(pref_a_P1, pref_a_P0, iref_width*4, iref_height*4);
		ThreeTapFilter(pcurr_a_P1, pcurr_a_P0, icurr_width*4, icurr_height*4);
		ThreeTapFilter(pref_a_P2, pref_a_P1, iref_width*2, iref_height*2);
		ThreeTapFilter(pcurr_a_P2, pcurr_a_P1, icurr_width*2, icurr_height*2);
	}
	
	Int ifilter_offset = 
		icurr_width * (EXPANDY_REF_FRAME>>2) + (EXPANDY_REF_FRAME>>2);
	pcurr_P2 += ifilter_offset;
	pref_P2 += ifilter_offset;
	if(m_volmd.fAUsage != RECTANGLE){
		pcurr_a_P2 += ifilter_offset;
		pref_a_P2 += ifilter_offset;
	}
	ifilter_offset = 
		(icurr_width<<1) * (EXPANDY_REF_FRAME>>1) + (EXPANDY_REF_FRAME>>1);
	pcurr_P1 += ifilter_offset;
	pref_P1 += ifilter_offset;
	if(m_volmd.fAUsage != RECTANGLE){
		pcurr_a_P1 += ifilter_offset;
		pref_a_P1 += ifilter_offset;
	}
	ifilter_offset = 
		(icurr_width<<2) * EXPANDY_REF_FRAME + EXPANDY_REF_FRAME;
	pcurr_P0 += ifilter_offset;
	pref_P0 += ifilter_offset;
	if(m_volmd.fAUsage != RECTANGLE){
		pcurr_a_P0 += ifilter_offset;
		pref_a_P0 += ifilter_offset;
	}
	
	for(xi = 0; xi < 2; xi++)
		dm[xi] = 0;
	for(xi = 0; xi < 2; xi++)
		dTM[xi] = 0;
	
	pref_work = pref_P2;
	pcurr_work = pcurr_P2;
	if(m_volmd.fAUsage != RECTANGLE){
		pref_alpha_work = pref_a_P2;
		pcurr_alpha_work = pcurr_a_P2;
	}
	
	ibest_locationx = 0;
	ibest_locationy = 0;
	
	if(m_volmd.fAUsage == RECTANGLE){
		ModifiedThreeStepSearch(pref_work, pcurr_work, 
			icurr_width, icurr_height, iref_width, iref_height,
			icurr_left, icurr_top, icurr_right, icurr_bottom,
			iref_left, iref_top, iref_right, iref_bottom,
			&ibest_locationx, &ibest_locationy); 
	}else{
		ModifiedThreeStepSearch_WithShape(pref_work, pcurr_work, 
			pref_alpha_work, pcurr_alpha_work,
			icurr_width, icurr_height, iref_width, iref_height,
			icurr_left, icurr_top, icurr_right, icurr_bottom,
			iref_left, iref_top, iref_right, iref_bottom,
			&ibest_locationx, &ibest_locationy); 
	}
	
	dTM[0] = (Double)ibest_locationx;
	dTM[1] = (Double)ibest_locationy;
	ithreshold_T = 1000000;
	
	for(ilevel = 2; ilevel >= 0; ilevel--){
		
		if(ilevel == 0){
			pref_work = pref_P0;
			pcurr_work = pcurr_P0;
		}else if(ilevel ==1){
			pref_work = pref_P1;
			pcurr_work = pcurr_P1;
		}
		if(m_volmd.fAUsage != RECTANGLE){
			if(ilevel == 0){
				pref_alpha_work = pref_a_P0;
				pcurr_alpha_work = pcurr_a_P0;
			}else if(ilevel ==1){
				pref_alpha_work = pref_a_P1;
				pcurr_alpha_work = pcurr_a_P1;
			}
		}
		icurr_offset = icurr_width - icurr_right + icurr_left;
		
		for(xi=0;xi<256;xi++)
			iperror_histgram[xi]=0;
		
		for(iite = 0; iite < 32; iite++){
			
			dE2 = 0.;
			istop = 0;
			icurr_pel = 0;
			
			for(xi = 0; xi < 4; xi++)
				dA[xi] = 0;
			for(xi = 0; xi < 2; xi++)
				db[xi] = 0;
			
			icurr_pel = icurr_width*icurr_top + icurr_left;
			if(m_volmd.fAUsage == RECTANGLE) {
				
				for(y=icurr_top; y<icurr_bottom; y++) {
					dy = (Double)y;
					for(x=icurr_left; x<icurr_right; x++, icurr_pel++) {
						
						dx = (Double)x; 
						
						dt = dx + dTM[0] ;
						du = dy + dTM[1] ;
						dx1 = dt;
						dy1 = du;
						x1 = (Int) dx1;
						y1 = (Int) dy1;
						if(x1>iref_left && (x1+1)<iref_right && y1>iref_top && (y1+1)<iref_bottom) {
							iref_pel = x1 + iref_width * y1;
							iref_pel1 = x1+1 + iref_width * y1;
							iref_pel2 = x1 + iref_width * (y1+1);
							iref_pel3 = x1+1 + iref_width * (y1+1);
							istop++;
							dk = dx1 - x1;
							d1mk = 1. - dk;
							dl = dy1 - y1;
							d1ml = 1. -dl;
							dI1x1y1[0] = pref_work[iref_pel];
							dI1x1y1[1] = pref_work[iref_pel1];
							dI1x1y1[2] = pref_work[iref_pel2];
							dI1x1y1[3] = pref_work[iref_pel3];
							dI1 = d1mk*d1ml*dI1x1y1[0] + dk*d1ml*dI1x1y1[1] +
								d1mk*dl*dI1x1y1[2] + dk*dl*dI1x1y1[3];
							de = dI1 - pcurr_work[icurr_pel];
							if(iite==0)
								iperror_histgram[(Int)(fabs(de))]++;
							if(fabs(de) < ithreshold_T) {
								dI1dx = (dI1x1y1[1]-dI1x1y1[0])*d1ml + (dI1x1y1[3]-dI1x1y1[2])*dl;
								dI1dy = (dI1x1y1[2]-dI1x1y1[0])*d1mk + (dI1x1y1[3]-dI1x1y1[1])*dk;
								ddedm[0] = dI1dx;
								ddedm[1] = dI1dy;
								db[0] += -de*ddedm[0];
								db[1] += -de*ddedm[1];
								for(yj=0; yj<2; yj++)
									for(xi=0; xi<=yj; xi++)
										dA[yj*2+xi] += ddedm[yj] * ddedm[xi];
									dE2 += de*de;
									
							}// threshold
						} // limit of ref_luma area
						
					} // x
					icurr_pel += icurr_offset;
				} // y
				
			}else{
				
				for(y=icurr_top; y<icurr_bottom; y++) {
					dy = (Double)y;
					for(x=icurr_left; x<icurr_right; x++, icurr_pel++) {
						
						if(pcurr_alpha_work[icurr_pel]) {
							dx = (Double)x; 
							
							dt = dx + dTM[0] ;
							du = dy + dTM[1] ;
							dx1 = dt;
							dy1 = du;
							x1 = (Int) dx1;
							y1 = (Int) dy1;
							if(x1>iref_left && (x1+1)<iref_right && y1>iref_top && (y1+1)<iref_bottom) {
								iref_pel = x1 + iref_width * y1;
								iref_pel1 = x1+1 + iref_width * y1;
								iref_pel2 = x1 + iref_width * (y1+1);
								iref_pel3 = x1+1 + iref_width * (y1+1);
								if((pref_alpha_work[iref_pel] && pref_alpha_work[iref_pel1] &&
									pref_alpha_work[iref_pel2] && pref_alpha_work[iref_pel3])){
									istop++;
									dk = dx1 - x1;
									d1mk = 1. - dk;
									dl = dy1 - y1;
									d1ml = 1. -dl;
									dI1x1y1[0] = pref_work[iref_pel];
									dI1x1y1[1] = pref_work[iref_pel1];
									dI1x1y1[2] = pref_work[iref_pel2];
									dI1x1y1[3] = pref_work[iref_pel3];
									dI1 = d1mk*d1ml*dI1x1y1[0] + dk*d1ml*dI1x1y1[1] +
										d1mk*dl*dI1x1y1[2] + dk*dl*dI1x1y1[3];
									de = dI1 - pcurr_work[icurr_pel];
									if(iite==0)
										iperror_histgram[(Int)(fabs(de))]++;
									if(fabs(de) < ithreshold_T) {
										dI1dx = (dI1x1y1[1]-dI1x1y1[0])*d1ml + (dI1x1y1[3]-dI1x1y1[2])*dl;
										dI1dy = (dI1x1y1[2]-dI1x1y1[0])*d1mk + (dI1x1y1[3]-dI1x1y1[1])*dk;
										ddedm[0] = dI1dx;
										ddedm[1] = dI1dy;
										db[0] += -de*ddedm[0];
										db[1] += -de*ddedm[1];
										for(yj=0; yj<2; yj++)
											for(xi=0; xi<=yj; xi++)
												dA[yj*2+xi] += ddedm[yj] * ddedm[xi];
											dE2 += de*de;
									} // threshold
								} // limit of ref_luma area
							} // limit of curr_luma area
							
						} // limit of curr_alpha value
						
					} // x
					icurr_pel += icurr_offset;
				} // y
				
			}
			
			if(iite==0){
				istop = istop*90/100;
				itmp=0;
				for(xi=0;xi<256;xi++){
					itmp+=iperror_histgram[xi];
					if(itmp > istop){
						ithreshold_T = xi-1;
						break; 
					}
				}
			}
			
			dA[1] = dA[2];
			
			icheck = DeltaMP(dA, 2, db, dm);
			
			if(icheck){
				for(xi=0; xi<2; xi++)
					dTM[xi] += dm[xi];
				if(fabs(dm[0]) < 0.001 && fabs(dm[1]) < 0.001 ) break; 
			}else{
				// printf("not up to matrix\n");
				break;
			}
			
		} // iteration/

		if(ilevel){
			dTM[0] *= 2;
			dTM[1] *= 2;
			ifilter_offset = 
				icurr_width * (EXPANDY_REF_FRAME>>ilevel) + (EXPANDY_REF_FRAME>>ilevel);
			pref_work -= ifilter_offset ;
			delete [] pref_work; pref_work = NULL;
			pcurr_work -= ifilter_offset ;
			delete [] pcurr_work; pcurr_work = NULL;
			if(m_volmd.fAUsage != RECTANGLE) {
				pref_alpha_work -= ifilter_offset ;
				delete [] pref_alpha_work; pref_alpha_work = NULL;
				pcurr_alpha_work -= ifilter_offset ;
				delete [] pcurr_alpha_work; pcurr_alpha_work = NULL;
			}
		}

		icurr_width *= 2;
		icurr_height *= 2;
		icurr_left *= 2;
		icurr_top *= 2;
		icurr_right *= 2;
		icurr_bottom *= 2;
		iref_width *= 2;
		iref_height *= 2;
		iref_left *= 2;
		iref_top *= 2;
		iref_right *= 2;
		iref_bottom *= 2;


	} // level

	pparameter -> dp1 = 1.0;
	pparameter -> dp2 = 0.0;
	pparameter -> dp3 = dTM[0];
	pparameter -> dp4 = 0.0;
	pparameter -> dp5 = 1.0;
	pparameter -> dp6 = dTM[1];
	pparameter -> dp7 = 0.0;
	pparameter -> dp8 = 0.0;

	delete [] dm; dm=NULL;
	delete [] db; db= NULL;
	delete [] dA; dA= NULL;
	delete [] dTM; dTM= NULL;
	delete [] ddedm; ddedm = NULL;
	delete [] dI1x1y1; dI1x1y1 = NULL;
	delete [] iperror_histgram; iperror_histgram = NULL;

}


Void CVideoObjectEncoder::IsotropicGME(const CVOPU8YUVBA* pvopcurr,
									   const CVOPU8YUVBA* pvopref, const CRct rctframe, const CRct rctcurr,
									   const CRct rctref, GME_para *pparameter)
{
	const CU8Image* puciRefY = pvopref -> getPlane (Y_PLANE);
	const CU8Image* puciCurrY = pvopcurr -> getPlane (Y_PLANE);
	PixelC* pref = (PixelC*) puciRefY -> pixels ();
	PixelC* pcurr = (PixelC*) puciCurrY -> pixels ();
	const CU8Image* puciRefA = (pvopref -> fAUsage () == EIGHT_BIT)?
        pvopref -> getPlaneA (0) :
	((m_volmd.fAUsage == RECTANGLE) ? (CU8Image*)NULL:pvopref -> getPlane (BY_PLANE));
	const CU8Image* puciCurrA = (pvopcurr -> fAUsage () == EIGHT_BIT)?
        pvopcurr -> getPlaneA (0) :
	((m_volmd.fAUsage == RECTANGLE) ? (CU8Image*)NULL:pvopcurr -> getPlane (BY_PLANE));
	PixelC* pref_alpha = NULL;
	PixelC* pcurr_alpha = NULL;
	
	if(m_volmd.fAUsage != RECTANGLE){
		pref_alpha = (PixelC*) puciRefA -> pixels ();
		pcurr_alpha = (PixelC*) puciCurrA -> pixels ();
	}
	
	PixelC *pref_P0=NULL, *pref_P1=NULL, *pref_P2=NULL;
	PixelC *pcurr_P0=NULL, *pcurr_P1=NULL, *pcurr_P2=NULL;
	PixelC *pref_a_P0=NULL, *pref_a_P1=NULL, *pref_a_P2=NULL;
	PixelC *pcurr_a_P0=NULL, *pcurr_a_P1=NULL, *pcurr_a_P2=NULL;
	PixelC *pref_work, *pcurr_work, *pcurr_alpha_work = NULL, *pref_alpha_work = NULL;
	Int icurr_width, icurr_height, icurr_offset;
	Int icurr_left, icurr_top, icurr_right, icurr_bottom;
	Int iref_width, iref_height;
	Int iref_left, iref_top, iref_right, iref_bottom;
	
	Int xi, yj, itmp, ibest_locationx, ibest_locationy;
	Int icurr_pel, iref_pel;
	Int iref_pel1, iref_pel2, iref_pel3;
	Double *dm = new Double [4];
	Double *db = new Double [4];
	Double *dA = new Double [16];
	Double *dTM = new Double [4];
	Double dE2 = 0;
	Int istop = 0, ilevel = 2, iite = 0;
	Int x, y, x1, y1;
	Double dx1, dy1, dx, dy;
	Double dt, du, dk, d1mk, dl, d1ml,
		dI1, de, dI1dx, dI1dy;
	Double *ddedm = new Double [4];
	Double *dI1x1y1 = new Double [4];
	Int *iperror_histgram = new Int [256];
	Int ithreshold_T=0, icheck = 1;
	
	iref_left = (rctref.left+EXPANDY_REFVOP) >> 2;
	iref_top = (rctref.top+EXPANDY_REFVOP) >> 2;
	iref_right = (rctref.right-EXPANDY_REFVOP) >> 2;
	iref_bottom = (rctref.bottom-EXPANDY_REFVOP) >> 2;
	iref_width = (rctframe.right - rctframe.left) >> 2;
	iref_height = (rctframe.bottom - rctframe.top) >> 2;
	
	icurr_left = (rctcurr.left) >> 2;
	icurr_top = (rctcurr.top) >> 2;
	icurr_right = (rctcurr.right) >> 2;
	icurr_bottom = (rctcurr.bottom) >> 2;
	icurr_width = iref_width ;
	icurr_height = iref_height ;
	
	pref_P0 = pref;
	pcurr_P0 = pcurr;
	pref_P1 = new PixelC[iref_width*iref_height*4];
	pcurr_P1 = new PixelC[icurr_width*icurr_height*4];
	pref_P2 = new PixelC[iref_width*iref_height];
	pcurr_P2 = new PixelC[icurr_width*icurr_height];
	ThreeTapFilter(pref_P1, pref_P0, iref_width*4, iref_height*4);
	ThreeTapFilter(pcurr_P1, pcurr_P0, icurr_width*4, icurr_height*4);
	ThreeTapFilter(pref_P2, pref_P1, iref_width*2, iref_height*2);
	ThreeTapFilter(pcurr_P2, pcurr_P1, icurr_width*2, icurr_height*2);
	
	if(m_volmd.fAUsage != RECTANGLE){
		pref_a_P0 = pref_alpha;
		pcurr_a_P0 = pcurr_alpha;
		pref_a_P1 = new PixelC[iref_width*iref_height*4];
		pcurr_a_P1 = new PixelC[icurr_width*icurr_height*4];
		pref_a_P2 = new PixelC[iref_width*iref_height];
		pcurr_a_P2 = new PixelC[icurr_width*icurr_height];
		ThreeTapFilter(pref_a_P1, pref_a_P0, iref_width*4, iref_height*4);
		ThreeTapFilter(pcurr_a_P1, pcurr_a_P0, icurr_width*4, icurr_height*4);
		ThreeTapFilter(pref_a_P2, pref_a_P1, iref_width*2, iref_height*2);
		ThreeTapFilter(pcurr_a_P2, pcurr_a_P1, icurr_width*2, icurr_height*2);
	}
	
	Int ifilter_offset = 
		icurr_width * (EXPANDY_REF_FRAME>>2) + (EXPANDY_REF_FRAME>>2);
	pcurr_P2 += ifilter_offset;
	pref_P2 += ifilter_offset;
	if(m_volmd.fAUsage != RECTANGLE){
		pcurr_a_P2 += ifilter_offset;
		pref_a_P2 += ifilter_offset;
	}
	ifilter_offset = 
		(icurr_width<<1) * (EXPANDY_REF_FRAME>>1) + (EXPANDY_REF_FRAME>>1);
	pcurr_P1 += ifilter_offset;
	pref_P1 += ifilter_offset;
	if(m_volmd.fAUsage != RECTANGLE){
		pcurr_a_P1 += ifilter_offset;
		pref_a_P1 += ifilter_offset;
	}
	ifilter_offset = 
		(icurr_width<<2) * EXPANDY_REF_FRAME + EXPANDY_REF_FRAME;
	pcurr_P0 += ifilter_offset;
	pref_P0 += ifilter_offset;
	if(m_volmd.fAUsage != RECTANGLE){
		pcurr_a_P0 += ifilter_offset;
		pref_a_P0 += ifilter_offset;
	}
	
	for(xi = 0; xi < 4; xi++)
		dm[xi] = 0;
	for(xi = 0; xi < 4; xi++)
		dTM[xi] = 0;
	dTM[0]=1.0;
	
	pref_work = pref_P2;
	pcurr_work = pcurr_P2;
	if(m_volmd.fAUsage != RECTANGLE){
		pref_alpha_work = pref_a_P2;
		pcurr_alpha_work = pcurr_a_P2;
	}
	
	ibest_locationx = 0;
	ibest_locationy = 0;
	
	if(pvopref -> fAUsage () == RECTANGLE){
		ModifiedThreeStepSearch(pref_work, pcurr_work, 
			icurr_width, icurr_height, iref_width, iref_height,
			icurr_left, icurr_top, icurr_right, icurr_bottom,
			iref_left, iref_top, iref_right, iref_bottom,
			&ibest_locationx, &ibest_locationy); 
	}else{
		ModifiedThreeStepSearch_WithShape(pref_work, pcurr_work, 
			pref_alpha_work, pcurr_alpha_work,
			icurr_width, icurr_height, iref_width, iref_height,
			icurr_left, icurr_top, icurr_right, icurr_bottom,
			iref_left, iref_top, iref_right, iref_bottom,
			&ibest_locationx, &ibest_locationy); 
	}
	
	dTM[2] = (Double)ibest_locationx;
	dTM[3] = (Double)ibest_locationy;
	ithreshold_T = 1000000;
	
	for(ilevel = 2; ilevel >= 0; ilevel--){
		
		if(ilevel == 0){
			pref_work = pref_P0;
			pcurr_work = pcurr_P0;
		}else if(ilevel ==1){
			pref_work = pref_P1;
			pcurr_work = pcurr_P1;
		}
		if(m_volmd.fAUsage != RECTANGLE){
			if(ilevel == 0){
				pref_alpha_work = pref_a_P0;
				pcurr_alpha_work = pcurr_a_P0;
			}else if(ilevel ==1){
				pref_alpha_work = pref_a_P1;
				pcurr_alpha_work = pcurr_a_P1;
			}
		}
		icurr_offset = icurr_width - icurr_right + icurr_left;
		
		for(xi=0;xi<256;xi++)
			iperror_histgram[xi]=0;
		
		for(iite = 0; iite < 32; iite++){
			
			dE2 = 0.;
			istop = 0;
			icurr_pel = 0;
			
			for(xi = 0; xi < 16; xi++)
				dA[xi] = 0;
			for(xi = 0; xi < 4; xi++)
				db[xi] = 0;
			
			icurr_pel = icurr_width*icurr_top + icurr_left;
			if(pvopref -> fAUsage () == RECTANGLE){
				
				for(y=icurr_top; y<icurr_bottom; y++) {
					dy = (Double)y;
					for(x=icurr_left; x<icurr_right; x++, icurr_pel++) {
						
						dx = (Double)x;
						
						dt = dTM[0] * dx + dTM[1] * dy + dTM[2];
						du = -dTM[1] * dx + dTM[0] * dy + dTM[3];
						dx1 = dt;
						dy1 = du;
						x1 = (Int) dx1;
						y1 = (Int) dy1;
						if(x1>iref_left && (x1+1)<iref_right && y1>iref_top && (y1+1)<iref_bottom) {
							iref_pel = x1 + iref_width * y1;
							iref_pel1 = x1+1 + iref_width * y1;
							iref_pel2 = x1 + iref_width * (y1+1);
							iref_pel3 = x1+1 + iref_width * (y1+1);
							istop++;
							dk = dx1 - x1;
							d1mk = 1. - dk;
							dl = dy1 - y1;
							d1ml = 1. -dl;
							dI1x1y1[0] = pref_work[iref_pel];
							dI1x1y1[1] = pref_work[iref_pel1];
							dI1x1y1[2] = pref_work[iref_pel2];
							dI1x1y1[3] = pref_work[iref_pel3];
							dI1 = d1mk*d1ml*dI1x1y1[0] + dk*d1ml*dI1x1y1[1] +
								d1mk*dl*dI1x1y1[2] + dk*dl*dI1x1y1[3];
							de = dI1 - pcurr_work[icurr_pel];
							if(iite==0)
								iperror_histgram[(Int)(fabs(de))]++;
							if(fabs(de) < ithreshold_T) {
								dI1dx = (dI1x1y1[1]-dI1x1y1[0])*d1ml + (dI1x1y1[3]-dI1x1y1[2])*dl;
								dI1dy = (dI1x1y1[2]-dI1x1y1[0])*d1mk + (dI1x1y1[3]-dI1x1y1[1])*dk;
								ddedm[0] = dx * dI1dx + dy * dI1dy;
								ddedm[1] = dy * dI1dx - dx * dI1dy;
								ddedm[2] = dI1dx;
								ddedm[3] = dI1dy;
								db[0] += -de*ddedm[0];
								db[1] += -de*ddedm[1];
								db[2] += -de*ddedm[2];
								db[3] += -de*ddedm[3];
								for(yj=0; yj<4; yj++)
									for(xi=0; xi<=yj; xi++)
										dA[yj*4+xi] += ddedm[yj] * ddedm[xi];
									dE2 += de*de;
							} // threshold
						} // limit of ref_luma area
						
					} // x
					icurr_pel += icurr_offset;
				} // y
				
			}else{
				
				for(y=icurr_top; y<icurr_bottom; y++) {
					dy = (Double)y;
					for(x=icurr_left; x<icurr_right; x++, icurr_pel++) {
						
						if(pcurr_alpha_work[icurr_pel]) {
							dx = (Double)x;
							
							dt = dTM[0] * dx + dTM[1] * dy + dTM[2];
							du = -dTM[1] * dx + dTM[0] * dy + dTM[3];
							dx1 = dt;
							dy1 = du;
							x1 = (Int) dx1;
							y1 = (Int) dy1;
							if(x1>iref_left && (x1+1)<iref_right && y1>iref_top && (y1+1)<iref_bottom) {
								iref_pel = x1 + iref_width * y1;
								iref_pel1 = x1+1 + iref_width * y1;
								iref_pel2 = x1 + iref_width * (y1+1);
								iref_pel3 = x1+1 + iref_width * (y1+1);
								if(pref_alpha_work[iref_pel] && pref_alpha_work[iref_pel1] &&
									pref_alpha_work[iref_pel2] && pref_alpha_work[iref_pel3]) {
									istop++;
									dk = dx1 - x1;
									d1mk = 1. - dk;
									dl = dy1 - y1;
									d1ml = 1. -dl;
									dI1x1y1[0] = pref_work[iref_pel];
									dI1x1y1[1] = pref_work[iref_pel1];
									dI1x1y1[2] = pref_work[iref_pel2];
									dI1x1y1[3] = pref_work[iref_pel3];
									dI1 = d1mk*d1ml*dI1x1y1[0] + dk*d1ml*dI1x1y1[1] +
										d1mk*dl*dI1x1y1[2] + dk*dl*dI1x1y1[3];
									de = dI1 - pcurr_work[icurr_pel];
									if(iite==0)
										iperror_histgram[(Int)(fabs(de))]++;
									if(fabs(de) < ithreshold_T) {
										dI1dx = (dI1x1y1[1]-dI1x1y1[0])*d1ml + (dI1x1y1[3]-dI1x1y1[2])*dl;
										dI1dy = (dI1x1y1[2]-dI1x1y1[0])*d1mk + (dI1x1y1[3]-dI1x1y1[1])*dk;
										ddedm[0] = dx * dI1dx + dy * dI1dy;
										ddedm[1] = dy * dI1dx - dx * dI1dy;
										ddedm[2] = dI1dx;
										ddedm[3] = dI1dy;
										db[0] += -de*ddedm[0];
										db[1] += -de*ddedm[1];
										db[2] += -de*ddedm[2];
										db[3] += -de*ddedm[3];
										for(yj=0; yj<4; yj++)
											for(xi=0; xi<=yj; xi++)
												dA[yj*4+xi] += ddedm[yj] * ddedm[xi];
											dE2 += de*de;
									} // threshold
								} // limit of ref_luma area
							} // limit of curr_luma area
							
						} // limit of curr_alpha value
						
					} // x
					icurr_pel += icurr_offset;
				} // y
				
			}
			
			if(iite==0){
				istop = istop*90/100;
				itmp=0;
				for(xi=0;xi<256;xi++){
					itmp+=iperror_histgram[xi];
					if(itmp > istop){
						ithreshold_T = xi-1;
						break; 
					}
				}
			}
			
			for(yj=0; yj<3; yj++)
				for(xi=yj+1; xi<4; xi++)
					dA[yj*4+xi] = dA[xi*4+yj];
			
			icheck = DeltaMP(dA, 4, db, dm);
			
			if(icheck){
				for(xi=0; xi<4; xi++)
					dTM[xi] += dm[xi];
				
				if(fabs(dm[2]) < 0.001 && fabs(dm[3]) < 0.001 && fabs(dm[0]) < 0.00001 &&
					fabs(dm[1]) < 0.00001 ) break;
			}else {
				// printf("not up to matrix\n");
				break;
			}
				
		} // iteration

		if(ilevel){
			dTM[2] *= 2;
			dTM[3] *= 2;
			ifilter_offset =
				icurr_width * (EXPANDY_REF_FRAME>>ilevel) + (EXPANDY_REF_FRAME>>ilevel);
			pref_work -= ifilter_offset ;
			delete [] pref_work; pref_work = NULL;
			pcurr_work -= ifilter_offset ;
			delete [] pcurr_work; pcurr_work = NULL;
			if(m_volmd.fAUsage != RECTANGLE) {
				pref_alpha_work -= ifilter_offset ;
				delete [] pref_alpha_work; pref_alpha_work = NULL;
				pcurr_alpha_work -= ifilter_offset ;
				delete [] pcurr_alpha_work; pcurr_alpha_work = NULL;
			}
		}

		icurr_width *= 2;
		icurr_height *= 2;
		icurr_left *= 2;
		icurr_top *= 2;
		icurr_right *= 2;
		icurr_bottom *= 2;
		iref_width *= 2;
		iref_height *= 2;
		iref_left *= 2;
		iref_top *= 2;
		iref_right *= 2;
		iref_bottom *= 2;

	} // level

	pparameter -> dp1 = dTM[0];
	pparameter -> dp2 = dTM[1];
	pparameter -> dp3 = dTM[2];
	pparameter -> dp4 = -dTM[1];
	pparameter -> dp5 = dTM[0];
	pparameter -> dp6 = dTM[3];
	pparameter -> dp7 = 0.0;
	pparameter -> dp8 = 0.0;

	delete [] dm; dm=NULL;
	delete [] db; db= NULL;
	delete [] dA; dA= NULL;
	delete [] dTM; dTM= NULL;
	delete [] ddedm; ddedm = NULL;
	delete [] dI1x1y1; dI1x1y1 = NULL;
	delete [] iperror_histgram; iperror_histgram = NULL;
}


Void CVideoObjectEncoder::AffineGME(const CVOPU8YUVBA* pvopcurr,
									const CVOPU8YUVBA* pvopref, const CRct rctframe, const CRct rctcurr,
									const CRct rctref, GME_para *pparameter)
{
	const CU8Image* puciRefY = pvopref -> getPlane (Y_PLANE);
	const CU8Image* puciCurrY = pvopcurr -> getPlane (Y_PLANE);
	PixelC* pref = (PixelC*) puciRefY -> pixels ();
	PixelC* pcurr = (PixelC*) puciCurrY -> pixels ();
	const CU8Image* puciRefA = (pvopref -> fAUsage () == EIGHT_BIT)?
        pvopref -> getPlaneA (0) :
	((m_volmd.fAUsage == RECTANGLE) ? (CU8Image*)NULL:pvopref -> getPlane (BY_PLANE));
	const CU8Image* puciCurrA = (pvopcurr -> fAUsage () == EIGHT_BIT)?
        pvopcurr -> getPlaneA (0) :
	((m_volmd.fAUsage == RECTANGLE) ? (CU8Image*)NULL:pvopcurr -> getPlane (BY_PLANE));
	PixelC* pref_alpha = NULL;
	PixelC* pcurr_alpha = NULL;
	
	if(m_volmd.fAUsage != RECTANGLE){
		pref_alpha = (PixelC*) puciRefA -> pixels ();
		pcurr_alpha = (PixelC*) puciCurrA -> pixels ();
	}
	
	PixelC *pref_P0=NULL, *pref_P1=NULL, *pref_P2=NULL;
	PixelC *pcurr_P0=NULL, *pcurr_P1=NULL, *pcurr_P2=NULL;
	PixelC *pref_a_P0=NULL, *pref_a_P1=NULL, *pref_a_P2=NULL;
	PixelC *pcurr_a_P0=NULL, *pcurr_a_P1=NULL, *pcurr_a_P2=NULL;
	PixelC *pref_work, *pcurr_work, *pcurr_alpha_work = NULL, *pref_alpha_work = NULL;
	Int icurr_width, icurr_height, icurr_offset;
	Int icurr_left, icurr_top, icurr_right, icurr_bottom;
	Int iref_width, iref_height;
	Int iref_left, iref_top, iref_right, iref_bottom;
	
	Int xi, yj, itmp, ibest_locationx, ibest_locationy;
	Int icurr_pel, iref_pel;
	Int iref_pel1, iref_pel2, iref_pel3;
	Double *dm = new Double [6];
	Double *db = new Double [6];
	Double *dA = new Double [36];
	Double *dTM = new Double [6];
	Double dE2 = 0;
	Int istop = 0, ilevel = 2, iite = 0;
	Int x, y, x1, y1;
	Double dx1, dy1, dx, dy;
	Double dt, du, dk, d1mk, dl, d1ml,
		dI1, de, dI1dx, dI1dy;
	Double *ddedm = new Double [6];
	Double *dI1x1y1 = new Double [4];
	Int *iperror_histgram = new Int [256];
	Int ithreshold_T=0, icheck = 1;
	
	iref_left = (rctref.left+EXPANDY_REFVOP) >> 2;
	iref_top = (rctref.top+EXPANDY_REFVOP) >> 2;
	iref_right = (rctref.right-EXPANDY_REFVOP) >> 2;
	iref_bottom = (rctref.bottom-EXPANDY_REFVOP) >> 2;
	iref_width = (rctframe.right - rctframe.left) >> 2;
	iref_height = (rctframe.bottom - rctframe.top) >> 2;
	
	icurr_left = (rctcurr.left) >> 2;
	icurr_top = (rctcurr.top) >> 2;
	icurr_right = (rctcurr.right) >> 2;
	icurr_bottom = (rctcurr.bottom) >> 2;
	icurr_width = iref_width ;
	icurr_height = iref_height ;
	
	pref_P0 = pref;
	pcurr_P0 = pcurr;
	pref_P1 = new PixelC[iref_width*iref_height*4];
	pcurr_P1 = new PixelC[icurr_width*icurr_height*4];
	pref_P2 = new PixelC[iref_width*iref_height];
	pcurr_P2 = new PixelC[icurr_width*icurr_height];
	ThreeTapFilter(pref_P1, pref_P0, iref_width*4, iref_height*4);
	ThreeTapFilter(pcurr_P1, pcurr_P0, icurr_width*4, icurr_height*4);
	ThreeTapFilter(pref_P2, pref_P1, iref_width*2, iref_height*2);
	ThreeTapFilter(pcurr_P2, pcurr_P1, icurr_width*2, icurr_height*2);
	
	if(m_volmd.fAUsage != RECTANGLE){
		pref_a_P0 = pref_alpha;
		pcurr_a_P0 = pcurr_alpha;
		pref_a_P1 = new PixelC[iref_width*iref_height*4];
		pcurr_a_P1 = new PixelC[icurr_width*icurr_height*4];
		pref_a_P2 = new PixelC[iref_width*iref_height];
		pcurr_a_P2 = new PixelC[icurr_width*icurr_height];
		ThreeTapFilter(pref_a_P1, pref_a_P0, iref_width*4, iref_height*4);
		ThreeTapFilter(pcurr_a_P1, pcurr_a_P0, icurr_width*4, icurr_height*4);
		ThreeTapFilter(pref_a_P2, pref_a_P1, iref_width*2, iref_height*2);
		ThreeTapFilter(pcurr_a_P2, pcurr_a_P1, icurr_width*2, icurr_height*2);
	}
	
	
	
	Int ifilter_offset = 
		icurr_width * (EXPANDY_REF_FRAME>>2) + (EXPANDY_REF_FRAME>>2);
	pcurr_P2 += ifilter_offset;
	pref_P2 += ifilter_offset;
	if(m_volmd.fAUsage != RECTANGLE){
		pcurr_a_P2 += ifilter_offset;
		pref_a_P2 += ifilter_offset;
	}
	ifilter_offset = 
		(icurr_width<<1) * (EXPANDY_REF_FRAME>>1) + (EXPANDY_REF_FRAME>>1);
	pcurr_P1 += ifilter_offset;
	pref_P1 += ifilter_offset;
	
	if(m_volmd.fAUsage != RECTANGLE){
		pcurr_a_P1 += ifilter_offset;
		pref_a_P1 += ifilter_offset;
	}
	ifilter_offset = 
		(icurr_width<<2) * EXPANDY_REF_FRAME + EXPANDY_REF_FRAME;
	pcurr_P0 += ifilter_offset;
	pref_P0 += ifilter_offset;
	
	if(m_volmd.fAUsage != RECTANGLE){
		pcurr_a_P0 += ifilter_offset;
		pref_a_P0 += ifilter_offset;
	}
	
	for(xi = 0; xi < 6; xi++)
		dm[xi] = 0;
	for(xi = 0; xi < 6; xi++)
		dTM[xi] = 0;
	dTM[0]=1.0;
	dTM[4]=1.0;
	
	pref_work = pref_P2;
	pcurr_work = pcurr_P2;
	if(m_volmd.fAUsage != RECTANGLE){
		pref_alpha_work = pref_a_P2;
		pcurr_alpha_work = pcurr_a_P2;
	}
	
	ibest_locationx = 0;
	ibest_locationy = 0;
	/* for the absolute coordinate system */
	
	if(pvopref -> fAUsage () == RECTANGLE){
		ModifiedThreeStepSearch(pref_work, pcurr_work, 
			icurr_width, icurr_height, iref_width, iref_height,
			icurr_left, icurr_top, icurr_right, icurr_bottom,
			iref_left, iref_top, iref_right, iref_bottom,
			&ibest_locationx, &ibest_locationy); 
	}else{
		ModifiedThreeStepSearch_WithShape(pref_work, pcurr_work, 
			pref_alpha_work, pcurr_alpha_work,
			icurr_width, icurr_height, iref_width, iref_height,
			icurr_left, icurr_top, icurr_right, icurr_bottom,
			iref_left, iref_top, iref_right, iref_bottom,
			&ibest_locationx, &ibest_locationy); 
	}
	
	dTM[2] = (Double)ibest_locationx;
	dTM[5] = (Double)ibest_locationy;
	ithreshold_T = 1000000;
	
	for(ilevel = 2; ilevel >= 0; ilevel--){
		
		if(ilevel == 0){
			pref_work = pref_P0;
			pcurr_work = pcurr_P0;
		}else if(ilevel ==1){
			pref_work = pref_P1;
			pcurr_work = pcurr_P1;
		}
		if(m_volmd.fAUsage != RECTANGLE){
			if(ilevel == 0){
				pref_alpha_work = pref_a_P0;
				pcurr_alpha_work = pcurr_a_P0;
			}else if(ilevel ==1){
				pref_alpha_work = pref_a_P1;
				pcurr_alpha_work = pcurr_a_P1;
			}
		}
		icurr_offset = icurr_width - icurr_right + icurr_left;
		
		for(xi=0;xi<256;xi++)
			iperror_histgram[xi]=0;
		
		for(iite = 0; iite < 32; iite++){
			
			dE2 = 0.;
			istop = 0;
			icurr_pel = 0;
			
			for(xi = 0; xi < 36; xi++)
				dA[xi] = 0;
			for(xi = 0; xi < 6; xi++)
				db[xi] = 0;
			
			icurr_pel = icurr_width*icurr_top + icurr_left;
			if(pvopref -> fAUsage () == RECTANGLE){
				
				for(y=icurr_top; y<icurr_bottom; y++) {
					dy = (Double)y;
					for(x=icurr_left; x<icurr_right; x++, icurr_pel++) {
						
						dx = (Double)x;
						
						dt = dTM[0] * dx + dTM[1] * dy + dTM[2];
						du = dTM[3] * dx + dTM[4] * dy + dTM[5];
						dx1 = dt;
						dy1 = du;
						x1 = (Int) dx1;
						y1 = (Int) dy1;
						if(x1>iref_left && (x1+1)<iref_right && y1>iref_top && (y1+1)<iref_bottom) {
							iref_pel = x1 + iref_width * y1;
							iref_pel1 = x1+1 + iref_width * y1;
							iref_pel2 = x1 + iref_width * (y1+1);
							iref_pel3 = x1+1 + iref_width * (y1+1);
							istop++;
							dk = dx1 - x1;
							d1mk = 1. - dk;
							dl = dy1 - y1;
							d1ml = 1. -dl;
							dI1x1y1[0] = pref_work[iref_pel];
							dI1x1y1[1] = pref_work[iref_pel1];
							dI1x1y1[2] = pref_work[iref_pel2];
							dI1x1y1[3] = pref_work[iref_pel3];
							dI1 = d1mk*d1ml*dI1x1y1[0] + dk*d1ml*dI1x1y1[1] +
								d1mk*dl*dI1x1y1[2] + dk*dl*dI1x1y1[3];
							de = dI1 - pcurr_work[icurr_pel];
							if(iite==0)
								iperror_histgram[(Int)(fabs(de))]++;
							if(fabs(de) < ithreshold_T) {
								dI1dx = (dI1x1y1[1]-dI1x1y1[0])*d1ml + (dI1x1y1[3]-dI1x1y1[2])*dl;
								dI1dy = (dI1x1y1[2]-dI1x1y1[0])*d1mk + (dI1x1y1[3]-dI1x1y1[1])*dk;
								ddedm[0] = dx * dI1dx;
								ddedm[1] = dy * dI1dx;
								ddedm[2] = dI1dx;
								ddedm[3] = dx * dI1dy;
								ddedm[4] = dy * dI1dy;
								ddedm[5] = dI1dy;
								db[0] += -de*ddedm[0];
								db[1] += -de*ddedm[1];
								db[2] += -de*ddedm[2];
								db[3] += -de*ddedm[3];
								db[4] += -de*ddedm[4];
								db[5] += -de*ddedm[5];
								for(yj=0; yj<6; yj++)
									for(xi=0; xi<=yj; xi++)
										dA[yj*6+xi] += ddedm[yj] * ddedm[xi];
									dE2 += de*de;
							} // threshold
						} // limit of ref_luma area
						
					} // x
					icurr_pel += icurr_offset;
				} // y
				
			}else{
				
				for(y=icurr_top; y<icurr_bottom; y++) {
					dy = (Double)y;
					for(x=icurr_left; x<icurr_right; x++, icurr_pel++) {
						
						if(pcurr_alpha_work[icurr_pel]) {
							dx = (Double)x;
							
							dt = dTM[0] * dx + dTM[1] * dy + dTM[2];
							du = dTM[3] * dx + dTM[4] * dy + dTM[5];
							dx1 = dt;
							dy1 = du;
							x1 = (Int) dx1;
							y1 = (Int) dy1;
							if(x1>iref_left && (x1+1)<iref_right && y1>iref_top && (y1+1)<iref_bottom) {
								iref_pel = x1 + iref_width * y1;
								iref_pel1 = x1+1 + iref_width * y1;
								iref_pel2 = x1 + iref_width * (y1+1);
								iref_pel3 = x1+1 + iref_width * (y1+1);
								if(pref_alpha_work[iref_pel] && pref_alpha_work[iref_pel1] &&
									pref_alpha_work[iref_pel2] && pref_alpha_work[iref_pel3]) {
									istop++;
									dk = dx1 - x1;
									d1mk = 1. - dk;
									dl = dy1 - y1;
									d1ml = 1. -dl;
									dI1x1y1[0] = pref_work[iref_pel];
									dI1x1y1[1] = pref_work[iref_pel1];
									dI1x1y1[2] = pref_work[iref_pel2];
									dI1x1y1[3] = pref_work[iref_pel3];
									dI1 = d1mk*d1ml*dI1x1y1[0] + dk*d1ml*dI1x1y1[1] +
										d1mk*dl*dI1x1y1[2] + dk*dl*dI1x1y1[3];
									de = dI1 - pcurr_work[icurr_pel];
									if(iite==0)
										iperror_histgram[(Int)(fabs(de))]++;
									if(fabs(de) < ithreshold_T) {
										dI1dx = (dI1x1y1[1]-dI1x1y1[0])*d1ml + (dI1x1y1[3]-dI1x1y1[2])*dl;
										dI1dy = (dI1x1y1[2]-dI1x1y1[0])*d1mk + (dI1x1y1[3]-dI1x1y1[1])*dk;
										ddedm[0] = dx * dI1dx;
										ddedm[1] = dy * dI1dx;
										ddedm[2] = dI1dx;
										ddedm[3] = dx * dI1dy;
										ddedm[4] = dy * dI1dy;
										ddedm[5] = dI1dy;
										db[0] += -de*ddedm[0];
										db[1] += -de*ddedm[1];
										db[2] += -de*ddedm[2];
										db[3] += -de*ddedm[3];
										db[4] += -de*ddedm[4];
										db[5] += -de*ddedm[5];
										for(yj=0; yj<6; yj++)
											for(xi=0; xi<=yj; xi++)
												dA[yj*6+xi] += ddedm[yj] * ddedm[xi];
											dE2 += de*de;
									} // threshold
								} // limit of ref_luma area
							} // limit of curr_luma area
							
						} // limit of curr_alpha value
						
					} // x
					icurr_pel += icurr_offset;
				} // y
				
			}
			
			if(iite==0){
				istop = istop*90/100;
				itmp=0;
				for(xi=0;xi<256;xi++){
					itmp+=iperror_histgram[xi];
					if(itmp > istop){
						ithreshold_T = xi-1;
						break; 
					}
				}
			}
			
			for(yj=0; yj<5; yj++)
				for(xi=yj+1; xi<6; xi++)
					dA[yj*6+xi] = dA[xi*6+yj];
				
			icheck = DeltaMP(dA, 6, db, dm);
			
			if(icheck){ 
				for(xi=0; xi<6; xi++)
					dTM[xi] += dm[xi];
				
				if(fabs(dm[2]) < 0.001 && fabs(dm[5]) < 0.001 && fabs(dm[0]) < 0.00001 &&
					fabs(dm[1]) < 0.00001 && fabs(dm[3]) < 0.00001 && 
					fabs(dm[4]) < 0.00001 ) break;
			}else {
				// printf("not up to matrix\n");
				break;
			}
				
		} // iteration

		if(ilevel){
			dTM[2] *= 2;
			dTM[5] *= 2;
			
			ifilter_offset =
				icurr_width * (EXPANDY_REF_FRAME>>ilevel) + (EXPANDY_REF_FRAME>>ilevel);
			pref_work -= ifilter_offset ;
			delete [] pref_work; pref_work = NULL;
			pcurr_work -= ifilter_offset ;
			delete [] pcurr_work; pcurr_work = NULL;
			if(m_volmd.fAUsage != RECTANGLE) {
				pref_alpha_work -= ifilter_offset ;
				delete [] pref_alpha_work; pref_alpha_work = NULL;
				pcurr_alpha_work -= ifilter_offset ;
				delete [] pcurr_alpha_work; pcurr_alpha_work = NULL;
			}
			
			
		}

		icurr_width *= 2;
		icurr_height *= 2;
		icurr_left *= 2;
		icurr_top *= 2;
		icurr_right *= 2;
		icurr_bottom *= 2;
		iref_width *= 2;
		iref_height *= 2;
		iref_left *= 2;
		iref_top *= 2;
		iref_right *= 2;
		iref_bottom *= 2;


	} // level

	pparameter -> dp1 = dTM[0];
	pparameter -> dp2 = dTM[1];
	pparameter -> dp3 = dTM[2];
	pparameter -> dp4 = dTM[3];
	pparameter -> dp5 = dTM[4];
	pparameter -> dp6 = dTM[5];
	pparameter -> dp7 = 0.0;
	pparameter -> dp8 = 0.0;
	delete [] dm; dm=NULL;
	delete [] db; db= NULL;
	delete [] dA; dA= NULL;
	delete [] dTM; dTM= NULL;
	delete [] ddedm; ddedm = NULL;
	delete [] dI1x1y1; dI1x1y1 = NULL;
	delete [] iperror_histgram; iperror_histgram = NULL;


}

Void CVideoObjectEncoder::ModifiedThreeStepSearch(PixelC *pref_work, 
												  PixelC *pcurr_work, 
												  Int icurr_width, Int icurr_height, Int iref_width, Int iref_height,
												  Int icurr_left, Int icurr_top, Int icurr_right, Int icurr_bottom,
												  Int iref_left, Int iref_top, Int iref_right, Int iref_bottom,
												  Int *ibest_locationx, Int *ibest_locationy) 
{
	
	Int xi,yj, irange, ilocationx, ilocationy, ino_of_pel=0;
	Int icurr_pel, iref_pel, iref_x, iref_y;
	Int irange_locationx=0, irange_locationy=0;
	Double dE1 = 0, dmin_error=300;
	Int icurr_offsety;
	
	icurr_offsety = icurr_width - icurr_right + icurr_left;
	
	for (irange = 4; irange >=1 ; irange/=2) {
		irange_locationx=*ibest_locationx;
		irange_locationy=*ibest_locationy;
		for (ilocationy = irange_locationy-irange; ilocationy <= irange_locationy+irange; 
		ilocationy += irange) {
			for (ilocationx = irange_locationx-irange; ilocationx <= irange_locationx+irange; 
			ilocationx += irange) {
				dE1 = 0.0;
				ino_of_pel = 0;
				icurr_pel = icurr_left + icurr_top*icurr_width;
				for (yj = icurr_top; yj< icurr_bottom ; yj++) {
					for (xi = icurr_left; xi< icurr_right ; xi++, icurr_pel++) {
						
						iref_x = ilocationx + xi;
						iref_y = ilocationy + yj;
						iref_pel = iref_y * iref_width + iref_x;
						
						if ((iref_x >=  iref_left) && (iref_x < iref_right) &&
							(iref_y >=  iref_top) && (iref_y < iref_bottom)){ 
							
							dE1 += Double(abs((Int)pcurr_work[icurr_pel] - (Int)pref_work[iref_pel]));
							ino_of_pel ++;
							
						} // limit of ref value
					} // i
					icurr_pel += icurr_offsety;
				} // j
				
				if (ino_of_pel > 0)
					dE1 = dE1 / ino_of_pel;
				else
					dE1 = dmin_error;
				if ((dE1 < dmin_error) || (dE1 == dmin_error && (abs(*ibest_locationx) + 
					abs(*ibest_locationy)) > (abs(ilocationx) + abs(ilocationy)))) {
					dmin_error = dE1;
					*ibest_locationx = ilocationx;
					*ibest_locationy = ilocationy;
				}
				
			} // locationx
		} // locationy
	} // range
	
}

Void CVideoObjectEncoder::ModifiedThreeStepSearch_WithShape(PixelC *pref_work, 
															PixelC *pcurr_work, 
															PixelC *pref_alpha_work, PixelC *pcurr_alpha_work,
															Int icurr_width, Int icurr_height, Int iref_width, Int iref_height,
															Int icurr_left, Int icurr_top, Int icurr_right, Int icurr_bottom,
															Int iref_left, Int iref_top, Int iref_right, Int iref_bottom,
															Int *ibest_locationx, Int *ibest_locationy) 
{
	
	Int xi,yj, irange, ilocationx, ilocationy, ino_of_pel=0;
	Int icurr_pel, iref_pel, iref_x, iref_y;
	Int irange_locationx=0, irange_locationy=0;
	Double dE1 = 0, dmin_error=300;
	Int icurr_offsety;
	
	icurr_offsety = icurr_width - icurr_right + icurr_left;
	
	for (irange = 4; irange >=1 ; irange/=2) {
		irange_locationx=*ibest_locationx;
		irange_locationy=*ibest_locationy;
		for (ilocationy = irange_locationy-irange; ilocationy <= irange_locationy+irange; 
		ilocationy += irange) {
			for (ilocationx = irange_locationx-irange; ilocationx <= irange_locationx+irange; 
			ilocationx += irange) {
				dE1 = 0.0;
				ino_of_pel = 0;
				icurr_pel = icurr_left + icurr_top*icurr_width;
				for (yj = icurr_top; yj< icurr_bottom ; yj++) {
					for (xi = icurr_left; xi< icurr_right ; xi++, icurr_pel++) {
						
						if (pcurr_alpha_work[icurr_pel]) {  
							
							iref_x = ilocationx + xi;
							iref_y = ilocationy + yj;
							iref_pel = iref_y * iref_width + iref_x;
							
							if ((iref_x >=  iref_left) && (iref_x < iref_right) &&
								(iref_y >=  iref_top) && (iref_y < iref_bottom) &&
								pref_alpha_work[iref_pel]) {
								
								dE1 += Double(abs((Int)pcurr_work[icurr_pel] - (Int)pref_work[iref_pel]));
								ino_of_pel ++;
								
							} // limit of ref area
						} // limit of curr_alpha value
					} // i
					icurr_pel += icurr_offsety;
				} // j
				
				if (ino_of_pel > 0)
					dE1 = dE1 / ino_of_pel;
				else
					dE1 = dmin_error;
				if ((dE1 < dmin_error) || (dE1 == dmin_error && (abs(*ibest_locationx) + 
					abs(*ibest_locationy)) > (abs(ilocationx) + abs(ilocationy)))) {
					dmin_error = dE1;
					*ibest_locationx = ilocationx;
					*ibest_locationy = ilocationy;
				}
				
			} // locationx
		} // locationy/
	} // range
	
}


Void CVideoObjectEncoder::ThreeTapFilter(PixelC *pLow, PixelC *pHight, Int iwidth,
										 Int iheight)
{
	Int xi, yj, ihwidth = iwidth/2, iwwidth = iwidth*2, iwidth2 = iwidth+2;
	PixelC *plt, *pct, *prt,
		*plc, *pcc, *prc,
		*plb, *pcb, *prb, *pp;
	
	pp=pLow; plt=pHight; pct=pHight; prt=pHight+1;
	plc=pHight; pcc=pHight; prc=pHight+1;
	plb=plc+iwidth; pcb=pcc+iwidth; prb=prc+iwidth;
	*pp = (PixelC)(((UInt)(*plt) + (((UInt)(*pct))<<1) + (UInt)(*prt) +
		(((UInt)(*plc))<<1) + (((UInt)(*pcc))<<2) + (((UInt)(*prc))<<1) +
		(UInt)(*plb) + (((UInt)(*pcb))<<1) + (UInt)(*prb) + 8) >> 4);
	
	pp=pLow+1; plt=pHight+1; pct=pHight+2; prt=pHight+3;
	plc=pHight+1; pcc=pHight+2; prc=pHight+3;
	plb=plc+iwidth; pcb=pcc+iwidth; prb=prc+iwidth;
	for(xi=1; xi<iwidth/2-1; xi++, pp++, plt+=2, pct+=2, prt+=2,
		plc+=2, pcc+=2, prc+=2,
		plb+=2, pcb+=2, prb+=2)
		*pp = (PixelC)(((UInt)(*plt) + (((UInt)(*pct))<<1) + (UInt)(*prt) +
		(((UInt)(*plc))<<1) + (((UInt)(*pcc))<<2) + (((UInt)(*prc))<<1) +
		(UInt)(*plb) + (((UInt)(*pcb))<<1) + (UInt)(*prb) + 8) >> 4);
	
	pp=pLow+ihwidth; plt=pHight+iwidth; pct=pHight+iwidth; prt=pHight+iwidth+1;
	plc=plt+iwidth; pcc=pct+iwidth; prc=prt+iwidth;
	plb=plc+iwidth; pcb=pcc+iwidth; prb=prc+iwidth;
	for(xi=1; xi<iheight/2-1; xi++, pp+=ihwidth, plt+=iwwidth, pct+=iwwidth, prt+=iwwidth,
		plc+=iwwidth, pcc+=iwwidth, prc+=iwwidth,
		plb+=iwwidth, pcb+=iwwidth, prb+=iwwidth)
		*pp = (PixelC)(((UInt)(*plt) + (((UInt)(*pct))<<1) + (UInt)(*prt) +
		(((UInt)(*plc))<<1) + (((UInt)(*pcc))<<2) + (((UInt)(*prc))<<1) +
		(UInt)(*plb) + (((UInt)(*pcb))<<1) + (UInt)(*prb) + 8) >> 4);
	
	pp=pLow+ihwidth+1; plt=pHight+1; pct=pHight+2; prt=pHight+3;
	plc=plt+iwidth; pcc=pct+iwidth; prc=prt+iwidth;
	plb=plc+iwidth; pcb=pcc+iwidth; prb=prc+iwidth;
	for(yj=1; yj<iheight/2-2; yj++, pp++, plt+=iwidth2, pct+=iwidth2, prt+=iwidth2,
		plc+=iwidth2, pcc+=iwidth2, prc+=iwidth2,
		plb+=iwidth2, pcb+=iwidth2, prb+=iwidth2)
		for(xi=1; xi<ihwidth; xi++, pp++, plt+=2, pct+=2, prt+=2,
			plc+=2, pcc+=2, prc+=2,
			plb+=2, pcb+=2, prb+=2)
		{
			*pp = (PixelC)(((UInt)(*plt) + (((UInt)(*pct))<<1) + (UInt)(*prt) +
				(((UInt)(*plc))<<1) + (((UInt)(*pcc))<<2) + (((UInt)(*prc))<<1) +
				(UInt)(*plb) + (((UInt)(*pcb))<<1) + (UInt)(*prb) + 8) >> 4);
		}
		
}


Int CVideoObjectEncoder::DeltaMP(Double *dA, Int in, Double *db, Double *dm)
{
	int xi, yj, xi2, in2=in*2, ik, i_pivot;
	Double *dAi = new Double [in * in * 2];
	Double dpivot, dtmp;
	
	dpivot = *dA;
	for(yj = 0; yj < in; yj++)
		for(xi = 0; xi < in; xi++)
			if(fabs(*(dA + yj * in + xi)) > fabs(dpivot))
				dpivot = *(dA + yj * in + xi);
			
	if(fabs(dpivot) < 0.000000001) {
		delete [] dAi; dAi= NULL;
		return(0);
	}
	dpivot = 1.0 / dpivot; 
	
	for(yj = 0; yj < in; yj++)
		for(xi = 0, xi2 = in; xi < in; xi++, xi2++){
			*(dAi + yj * in2 + xi) = (*(dA + yj * in + xi))*dpivot;
			if(xi == yj)
				*(dAi + yj * in2 + xi2) = dpivot;
			else
				*(dAi + yj * in2 + xi2) = 0.0;
		}
		
	for(xi = 0; xi < in; xi++) {
		
		i_pivot = xi;
		dpivot = *(dAi + xi * in2 + xi);
		for(yj = xi; yj < in; yj++)
			if(fabs(*(dAi + yj * in2 + xi)) > fabs(dpivot))
			{
				i_pivot = yj;
				dpivot = *(dAi + yj * in2 + xi);
			}
			if(fabs(dpivot) < 0.000000001) {
				delete [] dAi; dAi = NULL;
				return(0);
			}
			if(i_pivot!=xi)
				for(ik = 0; ik < in2; ik++) {
					dtmp = *(dAi + xi * in2 + ik);
					*(dAi + xi * in2 + ik) = *(dAi + i_pivot * in2 + ik);
					*(dAi + i_pivot * in2 + ik) = dtmp;
				}
				
				for(yj = 0; yj < in; yj++) {
					if(yj != xi)
					{
						dpivot = *(dAi + yj * in2 + xi) / *(dAi + xi * in2 + xi);
						for(ik = xi+1; ik < in2; ik++)
							*(dAi + yj * in2 + ik) -=
							dpivot*(*(dAi + xi * in2 + ik));
					}
				}
				
	}
	
	for(yj = 0; yj < in; yj++)
		for(xi = 0, xi2 = in; xi < in; xi++, xi2++)
			*(dAi + yj * in2 + xi2) /= *(dAi + yj * in2 + yj);
		
	for(xi = 0; xi < in; xi++)
		*(dm + xi) = 0.0;
	
	for(yj = 0; yj < in; yj++)
		for(xi = 0, xi2 = in; xi < in; xi++, xi2++)
			*(dm + yj) += (*(dAi + yj * in2 +xi2))*(*(db + xi));
		
	delete [] dAi; dAi = NULL;
	return(1);
}


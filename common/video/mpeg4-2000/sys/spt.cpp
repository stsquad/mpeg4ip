/*************************************************************************

This software module was originally developed by 

	Chuang Gu (chuanggu@microsoft.com), Microsoft Corporation
	(date: Auguest, 1997)

and also edited by

    Yoshinori Suzuki (Hitachi, Ltd.)
    Yuichiro Nakaya (Hitachi, Ltd.)

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

	spt.cpp

Abstract:

	Functions for sprite warping 

Revision History:
	Nov. 14, 1997: Fast Affine Warping functions added by Hitachi, Ltd.
	Jan. 13, 1999: Code for disallowing zero demoninators in perspective
                       warping added by Hitachi, Ltd.

*************************************************************************/

#include <stdio.h>
#include <math.h>
#include "typeapi.h"
#include "mode.hpp"
#include "vopses.hpp"
#include "codehead.h"

#ifdef __MFC_
#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

#define new DEBUG_NEW				   
#endif // __MFC_

#define _FOR_GSSP_

Void CVideoObject::warpYA (const CPerspective2D& persp, const CRct& rctWarpedBound, UInt accuracy) 
// warp m_pvopcSptQ's Y and A components to m_pvopcCurrQs
{
	assert (m_pvopcCurrQ -> whereY ().includes (rctWarpedBound));
	const CU8Image* puciCurrY = m_pvopcCurrQ -> getPlane (Y_PLANE);
#ifdef _FOR_GSSP_
	const CU8Image* puciCurrBY = m_pvopcCurrQ -> getPlane (BY_PLANE);
#endif
	const CU8Image* puciCurrA = (m_pvopcSptQ -> fAUsage () == EIGHT_BIT)? m_pvopcCurrQ -> getPlaneA (0) : m_pvopcCurrQ -> getPlane (BY_PLANE);
	const CU8Image* puciSptY = m_pvopcSptQ -> getPlane (Y_PLANE);
#ifdef _FOR_GSSP_
	const CU8Image* puciSptBY = m_pvopcSptQ -> getPlane (BY_PLANE);
#endif
	const CU8Image* puciSptA = (m_pvopcSptQ -> fAUsage () == EIGHT_BIT)? m_pvopcSptQ -> getPlaneA (0) : m_pvopcSptQ -> getPlane (BY_PLANE);
	const CRct rctSptY = m_pvopcSptQ -> whereY ();
	const UInt offsetSlice = m_pvopcCurrQ -> whereY ().width * MB_SIZE;
	UInt accuracy1 = accuracy + 1;
	PixelC* ppxlcCurrQYSlice = (PixelC*) puciCurrY -> pixels ();
#ifdef _FOR_GSSP_
	PixelC* ppxlcCurrQBYSlice = (PixelC*) puciCurrBY -> pixels ();
#endif
	PixelC* ppxlcCurrQASlice = (PixelC*) puciCurrA -> pixels ();
	memset (ppxlcCurrQYSlice, 0, puciCurrY -> where ().area () * sizeof(PixelC));
#ifdef _FOR_GSSP_
	memset (ppxlcCurrQBYSlice, 0, puciCurrBY -> where ().area () * sizeof(PixelC));
#endif
	memset (ppxlcCurrQASlice, 0, puciCurrA -> where ().area () * sizeof(PixelC));
	ppxlcCurrQYSlice = (PixelC*) puciCurrY -> pixels (rctWarpedBound.left, rctWarpedBound.top);
#ifdef _FOR_GSSP_
	ppxlcCurrQBYSlice = (PixelC*) puciCurrBY -> pixels (rctWarpedBound.left, rctWarpedBound.top);
#endif
	ppxlcCurrQASlice = (PixelC*) puciCurrA -> pixels (rctWarpedBound.left, rctWarpedBound.top);
	for (Int topMB = rctWarpedBound.top; topMB < rctWarpedBound.bottom; topMB += MB_SIZE) {
		PixelC * ppxlcCurrQYBlock = ppxlcCurrQYSlice;
		PixelC * ppxlcCurrQABlock = ppxlcCurrQASlice;
		PixelC * ppxlcCurrQBYBlock = ppxlcCurrQBYSlice;
		for (Int leftMB = rctWarpedBound.left; leftMB < rctWarpedBound.right; leftMB += MB_SIZE) {
			UInt offsetLine = m_pvopcCurrQ -> whereY ().width - min(MB_SIZE, rctWarpedBound.right - leftMB);
			Bool existOpaguePixelMB = (m_pvopcSptQ -> fAUsage () == RECTANGLE);
			Bool existZeroDenomMB = FALSE; 
			PixelC * ppxlcCurrQY = ppxlcCurrQYBlock;
			PixelC * ppxlcCurrQBY = ppxlcCurrQBYBlock;
			PixelC * ppxlcCurrQA = ppxlcCurrQABlock;
			for (CoordI y = topMB; y < min(topMB + MB_SIZE, rctWarpedBound.bottom); y++) {
				for (CoordI x = leftMB; x < min(leftMB + MB_SIZE, rctWarpedBound.right); x++) {
					CSiteWFlag src = persp * (CSite (x, y)); 
					if (src.f) {
						existZeroDenomMB = TRUE;
						continue;
					}
					CoordD dx = (CoordD)src.s.x / (1 << accuracy1);
					CoordD dy = (CoordD)src.s.y / (1 << accuracy1);
					CoordI fx = (CoordI) floor (dx);
					CoordI fy = (CoordI) floor (dy); 
					CoordI cx = (CoordI) ceil (dx);
					CoordI cy = (CoordI) ceil (dy);
					if (rctSptY.includes (fx, fy) && rctSptY.includes (fx, cy) && rctSptY.includes (cx, fy) && rctSptY.includes (cx, cy)) {
#ifdef _FOR_GSSP_
						PixelC pxlcWarpedBY = puciSptBY -> pixel (src.s, accuracy);
						if (pxlcWarpedBY >= 128) {
							*ppxlcCurrQBY = MPEG4_OPAQUE;
#else
						PixelC pxlcWarpedA = puciSptA -> pixel (src.s, accuracy);
						if (pxlcWarpedA >= 128) {
							*ppxlcCurrQA = MPEG4_OPAQUE;
#endif
							existOpaguePixelMB = TRUE;
							*ppxlcCurrQY = puciSptY -> pixel (src.s, accuracy);
#ifdef _FOR_GSSP_
							if(m_pvopcSptQ -> fAUsage () == EIGHT_BIT)
								*ppxlcCurrQA = puciSptA -> pixel (src.s, accuracy);
#endif
						}
					}
					ppxlcCurrQY++;	
#ifdef _FOR_GSSP_
					ppxlcCurrQBY++;
#endif
					ppxlcCurrQA++;
				}
				ppxlcCurrQY += offsetLine;
#ifdef _FOR_GSSP_
				ppxlcCurrQBY += offsetLine;
#endif
				ppxlcCurrQA += offsetLine;
			}
			assert (!(existOpaguePixelMB && existZeroDenomMB));

			ppxlcCurrQYBlock += MB_SIZE;
#ifdef _FOR_GSSP_
			ppxlcCurrQBYBlock += MB_SIZE;
#endif
			ppxlcCurrQABlock += MB_SIZE;
		}
		ppxlcCurrQYSlice += offsetSlice;
#ifdef _FOR_GSSP_
		ppxlcCurrQBYSlice += offsetSlice;
#endif
		ppxlcCurrQASlice += offsetSlice;
	}
} 

Void CVideoObject::warpUV (const CPerspective2D& persp, const CRct& rctWarpedBound, UInt accuracy) 
// warp m_pvopcSptQ's U and V components to m_pvopcCurrQs
{
	assert (m_pvopcCurrQ -> whereUV ().includes (rctWarpedBound));
	const CU8Image* puciCurrU = m_pvopcCurrQ -> getPlane (U_PLANE);
	const CU8Image* puciCurrV = m_pvopcCurrQ -> getPlane (V_PLANE);
	const CU8Image* puciCurrBY = m_pvopcCurrQ -> getPlane (BY_PLANE);
	const CU8Image* puciSptU = m_pvopcSptQ -> getPlane (U_PLANE);
	const CU8Image* puciSptV = m_pvopcSptQ -> getPlane (V_PLANE);
	const CRct rctSptUV = m_pvopcSptQ -> whereUV ();
	const UInt offsetSliceUV = m_pvopcCurrQ -> whereUV ().width * BLOCK_SIZE;
	const UInt offsetSliceBY = m_pvopcCurrQ -> whereY ().width * MB_SIZE;
	const UInt nextRowBY = m_pvopcCurrQ -> whereY ().width;
	UInt accuracy1 = accuracy + 1;
	PixelC* ppxlcCurrQUSlice = (PixelC*) puciCurrU -> pixels ();
	PixelC* ppxlcCurrQVSlice = (PixelC*) puciCurrV -> pixels ();
	PixelC pxlcVal = 128;
	// not sure if this is needed swinder
	if(m_volmd.nBits>8)
		pxlcVal = 1<<(m_volmd.nBits - 1);
	pxlcmemset (ppxlcCurrQUSlice, pxlcVal, puciCurrU -> where ().area ());
	pxlcmemset (ppxlcCurrQVSlice, pxlcVal, puciCurrV -> where ().area ());
	ppxlcCurrQUSlice = (PixelC*) puciCurrU -> pixels (rctWarpedBound.left, rctWarpedBound.top);
	ppxlcCurrQVSlice = (PixelC*) puciCurrV -> pixels (rctWarpedBound.left, rctWarpedBound.top);
	PixelC* ppxlcCurrQBYSlice = (PixelC*) puciCurrBY -> pixels (2 * rctWarpedBound.left, 2 * rctWarpedBound.top);
	
	for (Int topMB = rctWarpedBound.top; topMB < rctWarpedBound.bottom; topMB += BLOCK_SIZE) {
		PixelC * ppxlcCurrQUBlock = ppxlcCurrQUSlice;
		PixelC * ppxlcCurrQVBlock = ppxlcCurrQVSlice;
		PixelC * ppxlcCurrQBYBlock = ppxlcCurrQBYSlice;

		for (Int leftMB = rctWarpedBound.left; leftMB < rctWarpedBound.right; leftMB += BLOCK_SIZE) {
			PixelC *ppxlcCurrQU = ppxlcCurrQUBlock;
			PixelC *ppxlcCurrQV = ppxlcCurrQVBlock;
			PixelC *ppxlcCurrQBY = ppxlcCurrQBYBlock;
			PixelC *ppxlcCurrQBYNextRow = ppxlcCurrQBYBlock + nextRowBY;
			UInt offsetLineUV = m_pvopcCurrQ -> whereUV ().width - min(BLOCK_SIZE, rctWarpedBound.right - leftMB);
			UInt offsetLineBY = 2 * (m_pvopcCurrQ -> whereY ().width - min(BLOCK_SIZE, rctWarpedBound.right - leftMB));
			Bool existOpaguePixelMBUV = (m_pvopcSptQ -> fAUsage () == RECTANGLE);
			Bool existZeroDenomMBUV = FALSE; 
			for (CoordI y = topMB; y < min(topMB + BLOCK_SIZE, rctWarpedBound.bottom); y++) {
				for (CoordI x = leftMB; x < min(leftMB + BLOCK_SIZE, rctWarpedBound.right); x++) {
					CSiteWFlag src = persp * (CSite (x, y)); 
					if (src.f) {
						existZeroDenomMBUV = TRUE;
						continue;
					}
					CoordD dx = (CoordD)src.s.x / (1 << accuracy1);
					CoordD dy = (CoordD)src.s.y / (1 << accuracy1);
					CoordI fx = (CoordI) floor (dx);
					CoordI fy = (CoordI) floor (dy); 
					CoordI cx = (CoordI) ceil (dx);
					CoordI cy = (CoordI) ceil (dy);
					if (rctSptUV.includes (fx, fy) && rctSptUV.includes (fx, cy) && rctSptUV.includes (cx, fy) && rctSptUV.includes (cx, cy)) {
						if (*ppxlcCurrQBY | *(ppxlcCurrQBY + 1) | *ppxlcCurrQBYNextRow | *(ppxlcCurrQBYNextRow+1)) {
							existOpaguePixelMBUV = TRUE;
							*ppxlcCurrQU = puciSptU -> pixel (src.s, accuracy);
							*ppxlcCurrQV = puciSptV -> pixel (src.s, accuracy);
						}
					}
					ppxlcCurrQBY += 2;
					ppxlcCurrQBYNextRow += 2;
					ppxlcCurrQU++;	
					ppxlcCurrQV++;
				}
				ppxlcCurrQBY += offsetLineBY;
				ppxlcCurrQBYNextRow += offsetLineBY;
				ppxlcCurrQU += offsetLineUV;
				ppxlcCurrQV += offsetLineUV;
			}
			assert (!(existOpaguePixelMBUV && existZeroDenomMBUV));

			ppxlcCurrQBYBlock += MB_SIZE;
			ppxlcCurrQUBlock += BLOCK_SIZE;
			ppxlcCurrQVBlock += BLOCK_SIZE;
		}
		ppxlcCurrQBYSlice += offsetSliceBY;
		ppxlcCurrQUSlice += offsetSliceUV;
		ppxlcCurrQVSlice += offsetSliceUV;
	}
}		 

Void CVideoObject::FastAffineWarp (const CRct& rctWarpedBound, 
                   const CRct& rctWarpedBoundUV, UInt accuracy, UInt pntNum) 
{
	assert (m_pvopcCurrQ -> whereY ().includes (rctWarpedBound));
	const CU8Image* puciCurrY = m_pvopcCurrQ -> getPlane (Y_PLANE);
#ifdef _FOR_GSSP_
	const CU8Image* puciCurrBY = m_pvopcCurrQ -> getPlane (BY_PLANE);
#endif
	const CU8Image* puciCurrA = (m_pvopcSptQ -> fAUsage () == EIGHT_BIT)? m_pvopcCurrQ -> getPlaneA (0) : m_pvopcCurrQ -> getPlane (BY_PLANE);
	const CU8Image* puciSptY = m_pvopcSptQ -> getPlane (Y_PLANE);
#ifdef _FOR_GSSP_
	const CU8Image* puciSptBY = m_pvopcSptQ -> getPlane (BY_PLANE);
#endif
	const CU8Image* puciSptA = (m_pvopcSptQ -> fAUsage () == EIGHT_BIT)? m_pvopcSptQ -> getPlaneA (0) : m_pvopcSptQ -> getPlane (BY_PLANE);
	const UInt offset = m_pvopcCurrQ -> whereY ().width - rctWarpedBound.width;
	PixelC* ppxlcCurrQY = (PixelC*) puciCurrY -> pixels ();
#ifdef _FOR_GSSP_
	PixelC* ppxlcCurrQBY = (PixelC*) puciCurrBY -> pixels ();
#endif
	PixelC* ppxlcCurrQA = (PixelC*) puciCurrA -> pixels ();
	memset (ppxlcCurrQY, 0, puciCurrY -> where ().area () * sizeof(PixelC));
#ifdef _FOR_GSSP_
	memset (ppxlcCurrQBY, 0, puciCurrBY -> where ().area () * sizeof(PixelC));
#endif
	memset (ppxlcCurrQA, 0, puciCurrA -> where ().area () * sizeof(PixelC));
	ppxlcCurrQY = (PixelC*) puciCurrY -> pixels (rctWarpedBound.left, rctWarpedBound.top);
#ifdef _FOR_GSSP_
	ppxlcCurrQBY = (PixelC*) puciCurrBY -> pixels (rctWarpedBound.left, rctWarpedBound.top);
#endif
	ppxlcCurrQA = (PixelC*) puciCurrA -> pixels (rctWarpedBound.left, rctWarpedBound.top);
	PixelC* ppxlcSptQY = (PixelC*) puciSptY -> pixels ();
#ifdef _FOR_GSSP_
	PixelC* ppxlcSptQBY = (PixelC*) puciSptBY -> pixels ();
#endif
	PixelC* ppxlcSptQA = (PixelC*) puciSptA -> pixels ();
    Int sprite_left_edge = m_pvopcSptQ -> whereY ().left;
    Int sprite_top_edge = m_pvopcSptQ -> whereY ().top;
	ppxlcSptQY = (PixelC*) puciSptY -> pixels (sprite_left_edge, sprite_top_edge);
#ifdef _FOR_GSSP_
	ppxlcSptQBY = (PixelC*) puciSptBY -> pixels (sprite_left_edge, sprite_top_edge);
#endif
	ppxlcSptQA = (PixelC*) puciSptA -> pixels (sprite_left_edge, sprite_top_edge);

	assert (m_pvopcCurrQ -> whereUV ().includes (rctWarpedBoundUV));
	const CU8Image* puciCurrU = m_pvopcCurrQ -> getPlane (U_PLANE);
	const CU8Image* puciCurrV = m_pvopcCurrQ -> getPlane (V_PLANE);
#ifndef _FOR_GSSP_
	const CU8Image* puciCurrBY = m_pvopcCurrQ -> getPlane (BY_PLANE);
#endif
	const CU8Image* puciSptU = m_pvopcSptQ -> getPlane (U_PLANE);
	const CU8Image* puciSptV = m_pvopcSptQ -> getPlane (V_PLANE);
	const UInt offsetUV = m_pvopcCurrQ -> whereUV ().width - rctWarpedBoundUV.width;
	const UInt offsetBY = 2 * (m_pvopcCurrQ -> whereY ().width -  rctWarpedBoundUV.width);
	PixelC* ppxlcCurrQU = (PixelC*) puciCurrU -> pixels ();
	PixelC* ppxlcCurrQV = (PixelC*) puciCurrV -> pixels ();
	PixelC pxlcVal = 128;
	// not sure if this is needed swinder
	if(m_volmd.nBits>8)
		pxlcVal = 1<<(m_volmd.nBits - 1);
	memset (ppxlcCurrQU, pxlcVal, puciCurrU -> where ().area () * sizeof(PixelC));
	memset (ppxlcCurrQV, pxlcVal, puciCurrV -> where ().area () * sizeof(PixelC));
	ppxlcCurrQU = (PixelC*) puciCurrU -> pixels (rctWarpedBoundUV.left, rctWarpedBoundUV.top);
	ppxlcCurrQV = (PixelC*) puciCurrV -> pixels (rctWarpedBoundUV.left, rctWarpedBoundUV.top);
	PixelC* ppxlcSptQU = (PixelC*) puciSptU -> pixels ();
	PixelC* ppxlcSptQV = (PixelC*) puciSptV -> pixels ();
	ppxlcSptQU = (PixelC*) puciSptU -> pixels (sprite_left_edge/2, sprite_top_edge/2);
	ppxlcSptQV = (PixelC*) puciSptV -> pixels (sprite_left_edge/2, sprite_top_edge/2);

	UInt accuracy1 = accuracy + 1;
    UInt uiScale = 1 << accuracy1;
    Int a =0, b = 0, c, d = 0, e = 0, f;
    Int cyy =0 , cxy = 0, dnm_pwr = 0;	//wchen: get rid of unreferenced local variable cxx and cyx

    Int width = rctWarpedBound.right - rctWarpedBound.left;
    Int height = rctWarpedBound.bottom - rctWarpedBound.top;
    Int W, H, VW, VH = 0, VWH = 0, vh_pwr, vw_pwr, vwh_pwr = 0;

    Int r_pwr = 3 - accuracy;
    Int r = 1 << r_pwr;

    Int x0p =  (Int)(m_rgstDstQ [0].x * uiScale * r);
    Int y0p =  (Int)(m_rgstDstQ [0].y * uiScale * r);
    Int x1p =  (Int)(m_rgstDstQ [1].x * uiScale * r);
    Int y1p =  (Int)(m_rgstDstQ [1].y * uiScale * r);

    W = width;
    VW = 1;
    vw_pwr = 0;
    while (VW < W) {
        VW <<= 1;
        vw_pwr++;
    }
    Int ex2p = 0, ey2p = 0;
    Int x0 = 0;
    Int y0 = 0;
    Int x1 = W;
    Int y1 = 0;
    Int ex1p = LinearExtrapolation(x0, x1, x0p, x1p, W, VW) + ((x0+VW)<<4);
    Int ey1p = LinearExtrapolation(y0, y1, y0p, y1p, W, VW) + (y0<<4);

    if (pntNum==3) {
         Int x2p =  (Int)(m_rgstDstQ [2].x * uiScale * r);
         Int y2p =  (Int)(m_rgstDstQ [2].y * uiScale * r);

         H = height;
         VH = 1;
         vh_pwr = 0;
         while (VH < H) {
             VH <<= 1;
             vh_pwr++;
         }
         VWH = VW * VH;
         vwh_pwr = vw_pwr + vh_pwr;
         Int x2 = 0;
         Int y2 = H;
         ex2p = LinearExtrapolation(x0, x2, x0p, x2p, H, VH) + (x0<<4);
         ey2p = LinearExtrapolation(y0, y2, y0p, y2p, H, VH) + ((y0+VH)<<4);
         if (vw_pwr<=vh_pwr) {
            VH /= VW;
            VWH /= VW;
            VW=1;
            vh_pwr -= vw_pwr;
            vwh_pwr -= vw_pwr;
            vw_pwr=0;
         }
         else {
            VW /= VH;
            VWH /= VH;
            VH=1;
            vw_pwr -= vh_pwr;
            vwh_pwr -= vh_pwr;
            vh_pwr=0;
         }
         ex2p -= (sprite_left_edge * 16);
         ey2p -= (sprite_top_edge * 16);
     }

     x0p -= (sprite_left_edge * 16);
     y0p -= (sprite_top_edge * 16);
     ex1p -= (sprite_left_edge * 16);
     ey1p -= (sprite_top_edge * 16);

     if (pntNum == 2) {
        a = ex1p - x0p ;
        b = y0p - ey1p ;
        c = x0p * VW;
        d = -b ;
        e = a ;
        f = y0p * VW;
        dnm_pwr = r_pwr + vw_pwr;
        cxy = c + r*VW/2;
        cyy = f + r*VW/2;
     } 
	 else if (pntNum == 3) {
		a = (ex1p - x0p) * VH;
		b = (ex2p - x0p) * VW;
		c = x0p * VWH;
		d = (ey1p - y0p) * VH;
		e = (ey2p - y0p) * VW;
		f = y0p * VWH;
		dnm_pwr = r_pwr + vwh_pwr;
		cxy = c + r*VWH/2;
		cyy = f + r*VWH/2;
     }

     Int sprite_width = m_pvopcSptQ -> whereY ().width;
     Int sprite_height = m_pvopcSptQ -> whereY ().bottom
                       - m_pvopcSptQ -> whereY ().top;

     sprite_left_edge *= (1 << accuracy1);
     sprite_top_edge *= (1 << accuracy1);

     Int cxx_i, cxx_f, cyx_i, cyx_f, cxy_i, cxy_f, cyy_i, cyy_f;
     Int a_i, a_f, b_i, b_f, d_i, d_f, e_i, e_f;
     FourSlashes(cxy, 1<<dnm_pwr, &cxy_i, &cxy_f);
     FourSlashes(cyy, 1<<dnm_pwr, &cyy_i, &cyy_f);
     FourSlashes(a, 1<<dnm_pwr, &a_i, &a_f);
     FourSlashes(b, 1<<dnm_pwr, &b_i, &b_f);
     FourSlashes(d, 1<<dnm_pwr, &d_i, &d_f);
     FourSlashes(e, 1<<dnm_pwr, &e_i, &e_f);
     Int fracmask = (1 << dnm_pwr) - 1;

     Int y,x;
     UInt accuracy2 = accuracy1 << 1;
#ifdef _FOR_GSSP_
     PixelC pxlcWarpedBY;
#else
     PixelC pxlcWarpedA;
#endif
     Int rx,ry;
     Int addr_offset;
     Int warpmask = uiScale -1;
     Int bias = 1<<(accuracy2-1);
	 for (y = 0; y < height; 
          cxy_i += b_i, cxy_f += b_f, cyy_i += e_i, cyy_f += e_f, y++) {
		cxy_i += cxy_f >> dnm_pwr;
		cyy_i += cyy_f >> dnm_pwr;
		cxy_f &= fracmask;
		cyy_f &= fracmask;
		for (x = 0, cxx_i = cxy_i, cxx_f = cxy_f, 
				cyx_i = cyy_i, cyx_f = cyy_f;
				x < width; cxx_i += a_i, cxx_f += a_f, 
				cyx_i += d_i, cyx_f += d_f, x++) {
			cxx_i += cxx_f >> dnm_pwr;
			cyx_i += cyx_f >> dnm_pwr;
			cxx_f &= fracmask;
			cyx_f &= fracmask;
            if (cxx_i >= EXPANDY_REFVOP && cyx_i >= EXPANDY_REFVOP 
					  && cxx_i <= ((sprite_width-1-EXPANDY_REFVOP)<<accuracy1)
					  && cyx_i <= ((sprite_height-1-EXPANDY_REFVOP)<<accuracy1)) {
				addr_offset = (cyx_i>>accuracy1) * sprite_width + (cxx_i>>accuracy1);
				rx = cxx_i & warpmask;
				ry = cyx_i & warpmask;
#ifdef _FOR_GSSP_
				pxlcWarpedBY = CInterpolatePixelValue (	ppxlcSptQBY, 
#else
				pxlcWarpedA = CInterpolatePixelValue (	ppxlcSptQA, 
#endif
														addr_offset,sprite_width, rx, ry,
														uiScale, bias, accuracy2);
#ifdef _FOR_GSSP_
	  	        if (pxlcWarpedBY >= 128) {
		 			*ppxlcCurrQBY = MPEG4_OPAQUE;
#else
	  	        if (pxlcWarpedA >= 128) {
		 			*ppxlcCurrQA = MPEG4_OPAQUE;
#endif
                    *ppxlcCurrQY = CInterpolatePixelValue (	ppxlcSptQY, 
															addr_offset, sprite_width, rx, ry,
															uiScale, bias, accuracy2);
#ifdef _FOR_GSSP_
	                        if(m_pvopcSptQ -> fAUsage () == EIGHT_BIT)
                    			*ppxlcCurrQA = CInterpolatePixelValue (	ppxlcSptQA, 
																addr_offset, sprite_width, rx, ry,
																uiScale, bias, accuracy2);
#endif
				}
		    }
			ppxlcCurrQY++;	
#ifdef _FOR_GSSP_
			ppxlcCurrQBY++;	
#endif
			ppxlcCurrQA++;
		}
		ppxlcCurrQY += offset;
#ifdef _FOR_GSSP_
		ppxlcCurrQBY += offset;
#endif
		ppxlcCurrQA += offset;
	 }

#ifdef _FOR_GSSP_
	ppxlcCurrQBY = (PixelC*) puciCurrBY -> pixels (rctWarpedBound.left, rctWarpedBound.top);
	PixelC* ppxlcCurrQBYNextRow = ppxlcCurrQBY + puciCurrBY -> where ().width;
#else
	ppxlcCurrQA = (PixelC*) puciCurrA -> pixels (rctWarpedBound.left, rctWarpedBound.top);
	PixelC* ppxlcCurrQANextRow = ppxlcCurrQA + puciCurrA -> where ().width;
#endif
	sprite_left_edge /= 2;
	sprite_top_edge /= 2;
	width /= 2;
	height /= 2;
	sprite_width /= 2;
	sprite_height /= 2;
	if (pntNum == 2) {
		c = 2 * x0p * VW - 16*VW ;
		f = 2 * y0p * VW - 16*VW ;
		cxy = a + b + c + 2*r*VW;
		cyy = d + e + f + 2*r*VW;
	} 
	else if (pntNum == 3) {
		c = 2 * x0p * VWH - 16*VWH ;
		f = 2 * y0p * VWH - 16*VWH ;
		cxy = a + b + c + 2*VWH*r;
		cyy = d + e + f + 2*VWH*r;
	}
	a_f *= 4;
	b_f *= 4;
	d_f *= 4;
	e_f *= 4;
	dnm_pwr += 2;

	FourSlashes(cxy, 1<<dnm_pwr, &cxy_i, &cxy_f);
	FourSlashes(cyy, 1<<dnm_pwr, &cyy_i, &cyy_f);
	fracmask = (1 << dnm_pwr) - 1;

	for (y = 0; y < height; 
			cxy_i+=b_i, cxy_f+=b_f, cyy_i+=e_i, cyy_f+=e_f, y++) {
		cxy_i += cxy_f >> dnm_pwr;
		cyy_i += cyy_f >> dnm_pwr;
		cxy_f &= fracmask;
		cyy_f &= fracmask;
		for (x = 0, cxx_i = cxy_i, cxx_f = cxy_f, 
                    cyx_i = cyy_i, cyx_f = cyy_f;
                    x < width; cxx_i += a_i, cxx_f += a_f, 
                    cyx_i += d_i, cyx_f += d_f, x++) {
			cxx_i += cxx_f >> dnm_pwr;
			cyx_i += cyx_f >> dnm_pwr;
			cxx_f &= fracmask;
			cyx_f &= fracmask;
			if ( cxx_i >= EXPANDUV_REFVOP && cyx_i >= EXPANDUV_REFVOP 
					  && cxx_i <= ((sprite_width-1-EXPANDUV_REFVOP)<<accuracy1)
					  && cyx_i <= ((sprite_height-1-EXPANDUV_REFVOP)<<accuracy1)) {
				addr_offset = (cyx_i>>accuracy1) * sprite_width + (cxx_i>>accuracy1);
				rx = cxx_i & warpmask;
				ry = cyx_i & warpmask;
#ifdef _FOR_GSSP_
				if (*ppxlcCurrQBY | *(ppxlcCurrQBY + 1) | *ppxlcCurrQBYNextRow | *(ppxlcCurrQBYNextRow+1)) {
#else
				if (*ppxlcCurrQA | *(ppxlcCurrQA + 1) | *ppxlcCurrQANextRow | *(ppxlcCurrQANextRow+1)) {
#endif
					*ppxlcCurrQU = CInterpolatePixelValue (	ppxlcSptQU, addr_offset,
															sprite_width, rx, ry, uiScale, 
															bias, accuracy2); 
					*ppxlcCurrQV = CInterpolatePixelValue (	ppxlcSptQV, addr_offset,
															sprite_width, rx, ry, uiScale, 
															bias, accuracy2); 
				}
		    }
#ifdef _FOR_GSSP_
			ppxlcCurrQBY += 2;
			ppxlcCurrQBYNextRow += 2;
#else
			ppxlcCurrQA += 2;
			ppxlcCurrQANextRow += 2;
#endif
			ppxlcCurrQU++;	
			ppxlcCurrQV++;
		}
#ifdef _FOR_GSSP_
		ppxlcCurrQBY += offsetBY;
		ppxlcCurrQBYNextRow += offsetBY;
#else
		ppxlcCurrQA += offsetBY;
		ppxlcCurrQANextRow += offsetBY;
#endif
		ppxlcCurrQU += offsetUV;
		ppxlcCurrQV += offsetUV;
	}
}

PixelC CVideoObject:: CInterpolatePixelValue(PixelC* F, Int pos, Int width,
                           Int rx, Int ry, Int wpc, Int bias, Int pow_denom)
{
	PixelC* Fp;
	Int rxc, ryc;
	Int hstep, vstep;
	Int irtype = m_vopmd.iRoundingControl;

	Fp = F + pos;
	rxc = wpc - rx;
	ryc = wpc - ry;
	hstep = rx ? 1 : 0;
	vstep = ry ? width : 0;

	return ((PixelC) (((ryc * (rxc * *Fp + rx * *(Fp+hstep))
            + ry * (rxc * *(Fp+vstep) + rx * *(Fp+vstep+hstep))) + bias - irtype)
            >> pow_denom));
}

Int CVideoObject:: LinearExtrapolation(Int x0, Int x1, Int x0p, Int x1p, Int W, Int VW)
{
	Int quot, res, ressum, extrapolated;

	FourSlashes(x0p - (x0 << 4), W, &quot, &res);
	extrapolated = quot * (W - VW);

	FourSlashes(res * (W - VW), W, &quot, &res);
	extrapolated += quot;
	ressum = res;

	FourSlashes(x1p - (x1 << 4), W, &quot, &res);
	extrapolated += quot * VW + res;

	FourSlashes(res * (VW - W), W, &quot, &res);
	extrapolated += quot;
	ressum += res;

	FourSlashes(ressum, W, &quot, &res);
	extrapolated += quot;

	if(extrapolated >= 0) {
		if(res >= (W+1) / 2)
			extrapolated++;
	} 
	else
		if(res > W / 2)
			extrapolated++;

	return(extrapolated);
}

Void CVideoObject::FourSlashes(Int num, Int denom, Int *quot, Int *res)
{
	*quot =  num / denom;
	if (*quot * denom == num)
		*res = 0;
	else 
	{
		if (num < 0) /* denom is larger than zero */
			*quot -= 1;
		*res = num - denom * *quot;
	}
}

Void CVideoObject::swapRefQ1toSpt ()
{
	m_pvopcSptQ = m_pvopcRefQ1;
	m_pvopcSptQ -> shift (m_rctSpt.left, m_rctSpt.top); 
#ifdef _FOR_GSSP_
	if(m_pvopcSptQ -> fAUsage () == EIGHT_BIT){
		CU8Image* puciSptBY = (CU8Image *) m_pvopcSptQ -> getPlane (BY_PLANE);
		puciSptBY -> shift (m_rctSpt.left, m_rctSpt.top);
	}
#endif
	m_pvopcRefQ1 = NULL;
}

Void CVideoObject::changeSizeofCurrQ (CRct rctOrg)
{
	delete m_pvopcCurrQ;
	rctOrg.expand (EXPANDY_REF_FRAME);
	m_pvopcCurrQ = new CVOPU8YUVBA (m_pvopcSptQ -> fAUsage (), rctOrg, m_volmd.iAuxCompCount);
}
 
Bool CVideoObject::SptPieceMB_NOT_HOLE(Int iMBXoffset, Int iMBYoffset, CMBMode* pmbmd) 
//	  In a given Sprite object piece, Check whether current macroblock is not a hole and should be coded ?
{
	if (m_tPiece < 0)
		return TRUE;
	else 
	{
		CMBMode* pmbmdLeft = pmbmd - 1;
		Int iMBX = 	iMBXoffset + m_iPieceXoffset;
		Int iMBY = 	iMBYoffset + m_iPieceYoffset;		
		Int iMBX1 = iMBX -1;

// dshu: [v071] begin of modification
		Int iMod = m_rctSpt.width % MB_SIZE;
		Int iSptWidth = (iMod > 0) ? m_rctSpt.width + MB_SIZE - iMod : m_rctSpt.width;
		Int iNumMBX = iSptWidth / MB_SIZE;   
// dshu: [v071] end of modification

		IntraPredDirection* Spreddir = m_rgmbmdSpt[iMBY][iMBX1].m_preddir;
		IntraPredDirection* preddir = (*pmbmdLeft).m_preddir;	
		if (iMBXoffset >0 ) 
			if( m_ppPieceMBstatus[iMBY][iMBX1] == NOT_DONE)
			{
				m_ppPieceMBstatus[iMBY][iMBX1] = PIECE_DONE;
				m_rgmbmdSpt[iMBY][iMBX1] = CMBMode (*pmbmdLeft);
				m_rgmbmdSprite[iMBX1 + iNumMBX * iMBY] = CMBMode (*pmbmdLeft);  // dshu: [v071]  added to store mbmd array for sprite
				memcpy (Spreddir, preddir, 10  * sizeof (IntraPredDirection));
			}
			else {
				*pmbmdLeft = CMBMode (m_rgmbmdSpt[iMBY][iMBX1]);
				memcpy (preddir, Spreddir, 10  * sizeof (IntraPredDirection));
			}
// dshu: begin of modification
		if ( iMBX < (m_rctSptQ.width / MB_SIZE))	  
// dshu: end of modification

//		if ( iMBX < (m_rctSptExp.width / MB_SIZE))
			return ( m_ppPieceMBstatus[iMBY][iMBX] == NOT_DONE)	;
	}
	return FALSE;
}
 
Bool CVideoObject::SptUpdateMB_NOT_HOLE(Int iMBXoffset, Int iMBYoffset, CMBMode* pmbmd)
//	  In a given Sprite update piece, Check whether current macroblock is not a hole and should be coded ?
{

	Int iMBX = 	iMBXoffset + m_iPieceXoffset;
	Int iMBY = 	iMBYoffset + m_iPieceYoffset;

	Int iMBX1 = iMBX -1;
	if ( (iMBXoffset >0 ) && ( m_ppUpdateMBstatus[iMBY][iMBX1] == NOT_DONE))	
			m_ppUpdateMBstatus[iMBY][iMBX1] = UPDATE_DONE;		
	*pmbmd = CMBMode (m_rgmbmdSpt[iMBY][iMBX]);
	return ( m_ppUpdateMBstatus[iMBY][iMBX] == NOT_DONE)	;
} 

Void CVideoObject::SaveMBmCurrRow (Int iMBYoffset, MacroBlockMemory** rgpmbmCurr)
{
	Int iMBX;
	Int iMBY = 	iMBYoffset + m_iPieceYoffset;
	Int iMB, iBlk;
	Int nBlk = (m_volmd.fAUsage == EIGHT_BIT) ? 10 : 6;
	Int *Dest;
	Int *Src;

 	for (iMB = 0; iMB < m_iNumMBX; iMB++)	{
		iMBX = iMB + m_iPieceXoffset;
		for (iBlk = 0; iBlk < nBlk; iBlk++)	{
			Dest = (m_rgpmbmCurr_Spt[iMBY][iMBX]->rgblkm) [iBlk];
			Src = (rgpmbmCurr[iMB]->rgblkm) [iBlk];
			memcpy (Dest, Src, ((BLOCK_SIZE << 1) - 1)  * sizeof (Int));
		}
	}
}

 Void CVideoObject::RestoreMBmCurrRow (Int iMBYoffset, MacroBlockMemory** rgpmbmCurr)
{
	Int iMBX;
	Int iMBY = 	iMBYoffset + m_iPieceYoffset;
	Int iMB, iBlk;
	Int nBlk = (m_volmd.fAUsage == EIGHT_BIT) ? 10 : 6;
	Int *Dest, *Src;

 	for (iMB = 0; iMB < m_iNumMBX; iMB++)	{
		iMBX = iMB + m_iPieceXoffset;
		for (iBlk = 0; iBlk < nBlk; iBlk++)	{
			Src = (m_rgpmbmCurr_Spt[iMBY][iMBX]->rgblkm) [iBlk];
			Dest = (rgpmbmCurr[iMB]->rgblkm) [iBlk];
			memcpy (Dest, Src, ((BLOCK_SIZE << 1) - 1)  * sizeof (Int));
		}
	}
}

Void CVideoObject::CopyCurrQToPred (
	PixelC* ppxlcQMBY, 
	PixelC* ppxlcQMBU, 
	PixelC* ppxlcQMBV
)
{
	CoordI ix, iy, ic = 0;
	for (iy = 0; iy < MB_SIZE; iy++) {
		for (ix = 0; ix < MB_SIZE; ix++, ic++) {
			m_ppxlcPredMBY [ic] = ppxlcQMBY [ix] ;				
		}																				  
		ppxlcQMBY += m_iFrameWidthY;
	}

	ic = 0;
	for (iy = 0; iy < BLOCK_SIZE; iy++) {
		for (ix = 0; ix < BLOCK_SIZE; ix++, ic++) {
			m_ppxlcPredMBU [ic] =  ppxlcQMBU [ix];
			m_ppxlcPredMBV [ic] =  ppxlcQMBV [ix];
		}
		ppxlcQMBU += m_iFrameWidthUV;
		ppxlcQMBV += m_iFrameWidthUV;
	}
}

//low latency sprite stuff
Void CVideoObject::VOPOverlay (CVOPU8YUVBA& pvopc1, CVOPU8YUVBA& pvopc2, Int iscale)
{
	float fscaleY = (float) ((iscale == 0) ? 0.0 : 1.0);	  //  check whether there is a EXPANDY_REF_FRAME
	float fscaleUV = (float) ((iscale == 0) ? 0.0 : 0.5);
	CU8Image* uci1 = const_cast<CU8Image*> (pvopc1.getPlane (Y_PLANE)); 
	CU8Image* uci2 = const_cast<CU8Image*> (pvopc2.getPlane (Y_PLANE));
	Overlay (*uci1, *uci2, fscaleY) ; 
	uci1 = const_cast<CU8Image*> (pvopc1.getPlane (U_PLANE)); 
	uci2 = const_cast<CU8Image*> (pvopc2.getPlane (U_PLANE));
	Overlay (*uci1, *uci2, fscaleUV) ; 
	uci1 = const_cast<CU8Image*> (pvopc1.getPlane (V_PLANE)); 
	uci2 = const_cast<CU8Image*> (pvopc2.getPlane (V_PLANE));
	Overlay (*uci1, *uci2, fscaleUV) ; 

	if (m_volmd.fAUsage != RECTANGLE) {
		uci1 = const_cast<CU8Image*> (pvopc1.getPlane (BY_PLANE)); 
		uci2 = const_cast<CU8Image*> (pvopc2.getPlane (BY_PLANE));
		Overlay (*uci1, *uci2, fscaleY) ; 

		uci1 = const_cast<CU8Image*> (pvopc1.getPlane (BUV_PLANE)); 
		uci2 = const_cast<CU8Image*> (pvopc2.getPlane (BUV_PLANE));
		Overlay (*uci1, *uci2, fscaleUV) ;

		if (m_volmd.fAUsage == EIGHT_BIT) { 
			uci1 = const_cast<CU8Image*> (pvopc1.getPlaneA (0)); 
			uci2 = const_cast<CU8Image*> (pvopc2.getPlaneA (0));
			Overlay (*uci1, *uci2, fscaleY) ; 
		}

	}
}

Void CVideoObject::Overlay (CU8Image& uci1, CU8Image& uci2, float fscale)
{
	CRct r1 = uci1.where ();	
	CRct r2 = uci2.where (); 
	CoordI x = r1.left + (CoordI) (EXPANDY_REF_FRAME * fscale); // copy pixels, skip the guard band
	CoordI y = r1.top + (CoordI) (EXPANDY_REF_FRAME * fscale);
	Int cbLine = (r1.width- 2 * (Int) (EXPANDY_REF_FRAME * fscale)) * sizeof (PixelC);
	PixelC* ppxl = (PixelC*) uci2.pixels (x, y);
	const PixelC* ppxlFi = uci1.pixels (x, y);
	Int widthCurr = r2.width;
	Int widthFi = r1.width;
	for (CoordI yi = y; yi < (r1.bottom - EXPANDY_REF_FRAME * fscale); yi++) {
		memcpy (ppxl, ppxlFi, cbLine);
		ppxl += widthCurr;
		ppxlFi += widthFi;
	}
}

// dshu: begin of modification			
// get from  pvopc2 at (rctPieceY.left,rctPieceY.right)
Void CVideoObject::PieceGet (CVOPU8YUVBA& pvopc1, CVOPU8YUVBA& pvopc2, CRct rctPieceY)
{
	CRct rctPieceUV = rctPieceY.downSampleBy2 ();
	CU8Image* uci1 = const_cast<CU8Image*> (pvopc1.getPlane (Y_PLANE)); 
	CU8Image* uci2 = const_cast<CU8Image*> (pvopc2.getPlane (Y_PLANE));
	U8iGet(*uci1, *uci2, rctPieceY) ; 
	uci1 = const_cast<CU8Image*> (pvopc1.getPlane (U_PLANE)); 
	uci2 = const_cast<CU8Image*> (pvopc2.getPlane (U_PLANE));
	U8iGet(*uci1, *uci2, rctPieceUV) ; 
	uci1 = const_cast<CU8Image*> (pvopc1.getPlane (V_PLANE)); 
	uci2 = const_cast<CU8Image*> (pvopc2.getPlane (V_PLANE));
	U8iGet(*uci1, *uci2, rctPieceUV) ; 

	if (m_volmd.fAUsage != RECTANGLE) {
		uci1 = const_cast<CU8Image*> (pvopc1.getPlane (BY_PLANE)); 
		uci2 = const_cast<CU8Image*> (pvopc2.getPlane (BY_PLANE));
		U8iGet(*uci1, *uci2, rctPieceY) ; 

		uci1 = const_cast<CU8Image*> (pvopc1.getPlane (BUV_PLANE)); 
		uci2 = const_cast<CU8Image*> (pvopc2.getPlane (BUV_PLANE));
		U8iGet(*uci1, *uci2, rctPieceUV) ;

		if (m_volmd.fAUsage == EIGHT_BIT) { 
			uci1 = const_cast<CU8Image*> (pvopc1.getPlaneA (0)); 
			uci2 = const_cast<CU8Image*> (pvopc2.getPlaneA (0));
			U8iGet(*uci1, *uci2, rctPieceY) ; 
		}
	}
}
 
Void CVideoObject::U8iGet(CU8Image& uci1, CU8Image& uci2, CRct rctPiece)
{
	CRct r1 = uci1.where ();	
	CRct r2 = uci2.where (); 
	CoordI x = rctPiece.left ; 
	CoordI y = rctPiece.top ;
	Int cbLine = (rctPiece.width) * sizeof (PixelC);
	PixelC* ppxl = (PixelC*) uci1.pixels (0, 0);
	const PixelC* ppxlFi = uci2.pixels (x, y);
	Int widthCurr = r2.width;
	Int widthFi = r1.width;
	for (CoordI yi = y; yi < rctPiece.bottom; yi++) {
		memcpy (ppxl, ppxlFi, cbLine);
		ppxl += widthCurr;
		ppxlFi += widthFi;
	}
}

//  put into pvopc2 at (rctPieceY.left,rctPieceY.right)
Void CVideoObject::PiecePut (CVOPU8YUVBA& pvopc1, CVOPU8YUVBA& pvopc2, CRct rctPieceY)
{
	CRct rctPieceUV = rctPieceY.downSampleBy2 ();
	CU8Image* uci1 = const_cast<CU8Image*> (pvopc1.getPlane (Y_PLANE)); 
	CU8Image* uci2 = const_cast<CU8Image*> (pvopc2.getPlane (Y_PLANE));
	U8iPut(*uci1, *uci2, rctPieceY) ; 
	uci1 = const_cast<CU8Image*> (pvopc1.getPlane (U_PLANE)); 
	uci2 = const_cast<CU8Image*> (pvopc2.getPlane (U_PLANE));
	U8iPut(*uci1, *uci2, rctPieceUV) ; 
	uci1 = const_cast<CU8Image*> (pvopc1.getPlane (V_PLANE)); 
	uci2 = const_cast<CU8Image*> (pvopc2.getPlane (V_PLANE));
	U8iPut(*uci1, *uci2, rctPieceUV) ; 

	if (m_volmd.fAUsage != RECTANGLE) {
		uci1 = const_cast<CU8Image*> (pvopc1.getPlane (BY_PLANE)); 
		uci2 = const_cast<CU8Image*> (pvopc2.getPlane (BY_PLANE));
		U8iPut(*uci1, *uci2, rctPieceY) ; 

		uci1 = const_cast<CU8Image*> (pvopc1.getPlane (BUV_PLANE)); 
		uci2 = const_cast<CU8Image*> (pvopc2.getPlane (BUV_PLANE));
		U8iPut(*uci1, *uci2, rctPieceUV) ;

		if (m_volmd.fAUsage == EIGHT_BIT) { 
			uci1 = const_cast<CU8Image*> (pvopc1.getPlaneA (0)); 
			uci2 = const_cast<CU8Image*> (pvopc2.getPlaneA (0));
			U8iPut(*uci1, *uci2, rctPieceY) ; 
		}
	}
}
 
Void CVideoObject::U8iPut(CU8Image& uci1, CU8Image& uci2, CRct rctPiece)
{
	CRct r1 = uci1.where ();	
	CRct r2 = uci2.where (); 
	CoordI x = rctPiece.left ; 
	CoordI y = rctPiece.top ;
	Int cbLine = (rctPiece.width) * sizeof (PixelC);
	PixelC* ppxl = (PixelC*) uci2.pixels (x, y);
	const PixelC* ppxlFi = uci1.pixels (0, 0);
	Int widthCurr = r2.width;
	Int widthFi = r1.width;
	for (CoordI yi = y; yi < rctPiece.bottom; yi++) {
		memcpy (ppxl, ppxlFi, cbLine);
		ppxl += widthCurr;
		ppxlFi += widthFi;
	}
}
 
// dshu: end of modification

// dshu: [v071]  Begin of modification
 
Void CVideoObject::padSprite()
{
	m_iNumMBX = m_rctSptQ.width / MB_SIZE; 
	m_iNumMBY = m_rctSptQ.height () / MB_SIZE;	
	Int iMBXleft, iMBXright; 
	Int iMBYup, iMBYdown;
	CMBMode* pmbmd = m_rgmbmdSprite;
	PixelC* ppxlcRefY  = (PixelC*) m_pvopcSptQ->pixelsY () + m_iStartInRefToCurrRctY;
	PixelC* ppxlcRefU  = (PixelC*) m_pvopcSptQ->pixelsU () + m_iStartInRefToCurrRctUV;
	PixelC* ppxlcRefV  = (PixelC*) m_pvopcSptQ->pixelsV () + m_iStartInRefToCurrRctUV;
	PixelC* ppxlcRefBY = (PixelC*) m_pvopcSptQ->pixelsBY () + m_iStartInRefToCurrRctY;
	PixelC* ppxlcRefA  = (PixelC*) m_pvopcSptQ->pixelsA (0) + m_iStartInRefToCurrRctY;
	PixelC* ppxlcRefBUV = (PixelC*) m_pvopcSptQ->pixelsBUV () + m_iStartInRefToCurrRctUV;
  


	for (Int iMBY = 0; iMBY < m_iNumMBY; iMBY++){
		PixelC* ppxlcRefMBY  = ppxlcRefY;
		PixelC* ppxlcRefMBU  = ppxlcRefU;
		PixelC* ppxlcRefMBV  = ppxlcRefV;
		PixelC* ppxlcRefMBBY = ppxlcRefBY;
		PixelC* ppxlcRefMBBUV = ppxlcRefBUV;
		PixelC* ppxlcRefMBA   = ppxlcRefA;
    PixelC** pppxlcRefMBA = &ppxlcRefMBA;

		iMBYup = iMBY -1;
		iMBYdown = iMBY +1;
		for (Int iMBX = 0; iMBX < m_iNumMBX; iMBX++)
		{
			iMBXleft = iMBX -1 ; 
			iMBXright = iMBX +1 ;
			if((m_volmd.bShapeOnly==FALSE) && (m_ppPieceMBstatus[iMBY][iMBX] == PIECE_DONE))
			{
				pmbmd->m_bPadded=FALSE;
				copySptQShapeYToMb (m_ppxlcCurrMBBY, ppxlcRefMBBY);				
				downSampleBY (m_ppxlcCurrMBBY, m_ppxlcCurrMBBUV, NULL); // downsample original BY now for LPE padding (using original shape)

				if (pmbmd->m_rgTranspStatus [0] != ALL)
				{
					// MC padding
					if (pmbmd -> m_rgTranspStatus [0] == PARTIAL)
						mcPadCurrMB (ppxlcRefMBY, ppxlcRefMBU, ppxlcRefMBV,
							pppxlcRefMBA);
					padNeighborTranspMBs (
						iMBX, iMBY,
						pmbmd,
						ppxlcRefMBY, ppxlcRefMBU, ppxlcRefMBV, pppxlcRefMBA
					);
				}
				else
				{
					if (iMBX > 0)
					{
						if ((pmbmd - 1)->m_rgTranspStatus [0] != ALL &&
							m_ppPieceMBstatus[iMBY][iMBX-1] == PIECE_DONE)
						{
						    mcPadCurrMBFromLeft (ppxlcRefMBY, ppxlcRefMBU,
						                         ppxlcRefMBV, pppxlcRefMBA);
							pmbmd->m_bPadded = TRUE;
						}
					}
					if (iMBY > 0)
					{
						if ((pmbmd - m_iNumMBX)->m_rgTranspStatus [0] != ALL &&
							m_ppPieceMBstatus[iMBY-1][iMBX] == PIECE_DONE)
						{
							if (!(pmbmd->m_bPadded))
							{
								mcPadCurrMBFromTop (ppxlcRefMBY, ppxlcRefMBU,
												   ppxlcRefMBV, pppxlcRefMBA);
								pmbmd->m_bPadded = TRUE;
							}
						}
						else if (!(pmbmd - m_iNumMBX)->m_bPadded)
							mcSetTopMBGray (ppxlcRefMBY, ppxlcRefMBU,
											   ppxlcRefMBV, pppxlcRefMBA);
					}
					if(iMBY == m_iNumMBY-1)
					{
						if(iMBX > 0 && (pmbmd-1)->m_rgTranspStatus [0] == ALL &&
							!((pmbmd-1)->m_bPadded))
							mcSetLeftMBGray(ppxlcRefMBY, ppxlcRefMBU,
											ppxlcRefMBV, pppxlcRefMBA);
						if(iMBX == m_iNumMBX-1 && !(pmbmd->m_bPadded))
							mcSetCurrMBGray(ppxlcRefMBY, ppxlcRefMBU,
											ppxlcRefMBV, pppxlcRefMBA);
					}
				}
			}
			ppxlcRefMBA += MB_SIZE;
			ppxlcRefMBBY += MB_SIZE;
			ppxlcRefMBBUV += BLOCK_SIZE;
			pmbmd++;
			ppxlcRefMBY += MB_SIZE;
			ppxlcRefMBU += BLOCK_SIZE;
			ppxlcRefMBV += BLOCK_SIZE;
		}
		ppxlcRefY += m_iFrameWidthYxMBSize;
		ppxlcRefU += m_iFrameWidthUVxBlkSize;
		ppxlcRefV += m_iFrameWidthUVxBlkSize;
		ppxlcRefBY += m_iFrameWidthYxMBSize;
		ppxlcRefA += m_iFrameWidthYxMBSize;
		ppxlcRefBUV += m_iFrameWidthUVxBlkSize;
	}
	m_rctPrevNoExpandY = m_rctSptQ;
	m_rctPrevNoExpandUV = m_rctPrevNoExpandY.downSampleBy2 ();
	repeatPadYOrA ((PixelC*) m_pvopcSptQ->pixelsY () + m_iOffsetForPadY, m_pvopcSptQ);
	repeatPadUV (m_pvopcSptQ);

	//reset by in RefQ1 so that no left-over from last frame
	if (m_volmd.fAUsage != RECTANGLE)       {
		if (m_volmd.fAUsage == EIGHT_BIT)
				repeatPadYOrA ((PixelC*) m_pvopcSptQ->pixelsA (0) + m_iOffsetForPadY, m_pvopcSptQ);
	}
}

Void CVideoObject::copySptQShapeYToMb (
	PixelC* ppxlcDstMB, const PixelC* ppxlcSrc)
{
	for (Int i = 0; i < MB_SIZE; i++)	{
		memcpy (ppxlcDstMB, ppxlcSrc, MB_SIZE*sizeof(PixelC));
		ppxlcSrc += m_iFrameWidthY;
		ppxlcDstMB  += MB_SIZE;
	}
}

// dshu: [v071] end of modification

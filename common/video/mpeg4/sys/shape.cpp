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

*************************************************************************/

#include "typeapi.h"
#include "entropy/entropy.hpp"
#include "entropy/huffman.hpp"
#include "entropy/bitstrm.hpp"
#include "global.hpp"
#include "mode.hpp"
#include "codehead.h"
#include "cae.h"
#include "vopses.hpp"


#ifdef __MFC_
#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

#define new DEBUG_NEW				   
#endif // __MFC_

own Int* CVideoObject::computeShapeSubBlkIndex (Int iSubBlkSize, Int iSrcSize)
{
	Int* rgiShapeSubBlkIndex = new Int [MB_SIZE * MB_SIZE / iSubBlkSize / iSubBlkSize];
	Int nBorderLine = (iSrcSize - MB_SIZE) / 2;
	CoordI iX, iY, i = 0;
	for (iY = nBorderLine; iY < (MB_SIZE + nBorderLine); iY += iSubBlkSize)	{
		for (iX = nBorderLine; iX < (MB_SIZE + nBorderLine); iX += iSubBlkSize)
			rgiShapeSubBlkIndex [i++] = iY * iSrcSize + iX;
	}
	return rgiShapeSubBlkIndex;
}

Void CVideoObject::downSampleShapeMCPred(const PixelC*ppxlcSrc,
												PixelC* ppxlcDst,Int iRate)
{
	assert(iRate==1 || iRate==2 || iRate==4);
	static Int rgiScan[16] = {0,1,MC_BAB_SIZE,MC_BAB_SIZE+1,
		2,3,MC_BAB_SIZE+2,MC_BAB_SIZE+3,
		2*MC_BAB_SIZE,2*MC_BAB_SIZE+1,2*MC_BAB_SIZE+2,2*MC_BAB_SIZE+3,
		3*MC_BAB_SIZE,3*MC_BAB_SIZE+1,3*MC_BAB_SIZE+2,3*MC_BAB_SIZE+3};
	static Int rgiThresh[5] = {0,0,OPAQUE,0,7*OPAQUE};
	Int iBdrThresh=0;
	if(iRate>2)
		iBdrThresh=OPAQUE;
	Int iThreshold = rgiThresh[iRate];

#ifdef __TRACE_AND_STATS_
	//m_pbitstrmOut->trace ((PixelC*)ppxlcSrc, MC_BAB_SIZE, MC_BAB_SIZE, "MB_MC_BAB_ORIG");
#endif //__TRACE_AND_STATS_

	const PixelC *ppxlcSrcScan = ppxlcSrc+(MC_BAB_SIZE+1)*MC_BAB_BORDER;
	Int iDstSize = 2*MC_BAB_BORDER+(MC_BAB_SIZE-2*MC_BAB_BORDER)/iRate;
	PixelC *ppxlcDstScan = ppxlcDst+(iDstSize+1)*MC_BAB_BORDER;
	Int iX,iY,i,iSum,iSum1,iSum2,iSum3,iSum4;
	Int iRateSqd=iRate*iRate;
	const PixelC *ppxlcSrcBdr1 = ppxlcSrc+MC_BAB_SIZE*MC_BAB_BORDER;
	const PixelC *ppxlcSrcBdr2 = ppxlcSrc+2*MC_BAB_SIZE-1;
	const PixelC *ppxlcSrcBdr3 = ppxlcSrc+MC_BAB_BORDER;
	const PixelC *ppxlcSrcBdr4 = ppxlcSrc+MC_BAB_SIZE*(MC_BAB_SIZE-1)+MC_BAB_BORDER;
	PixelC *ppxlcDstBdr1 = ppxlcDst+iDstSize*MC_BAB_BORDER;
	PixelC *ppxlcDstBdr2 = ppxlcDst+2*iDstSize-1;
	PixelC *ppxlcDstBdr3 = ppxlcDst+MC_BAB_BORDER;
	PixelC *ppxlcDstBdr4 = ppxlcDst+iDstSize*(iDstSize-1)+MC_BAB_BORDER;

	for(iY=MC_BAB_BORDER;iY<iDstSize-MC_BAB_BORDER;iY++)
	{
		for(iX=MC_BAB_BORDER;iX<iDstSize-MC_BAB_BORDER;iX++,ppxlcDstScan++)
		{
			iSum=0;
			for(i=0;i<iRateSqd;i++)
				iSum+=ppxlcSrcScan[rgiScan[i]];
			*ppxlcDstScan=(iSum>iThreshold)?OPAQUE:TRANSPARENT;
			ppxlcSrcScan+=iRate;
		}
		ppxlcDstScan+=2*MC_BAB_BORDER;
		ppxlcSrcScan+=MC_BAB_SIZE*iRate-MC_BAB_SIZE+2*MC_BAB_BORDER;

		// border
		iSum1=iSum2=iSum3=iSum4=0;
		for(i=0;i<iRate;i++)
		{
			iSum1+=ppxlcSrcBdr1[i*MC_BAB_SIZE];
			iSum2+=ppxlcSrcBdr2[i*MC_BAB_SIZE];
			iSum3+=ppxlcSrcBdr3[i];
			iSum4+=ppxlcSrcBdr4[i];
		}
		*ppxlcDstBdr1=(iSum1>iBdrThresh)?OPAQUE:TRANSPARENT;
		*ppxlcDstBdr2=(iSum2>iBdrThresh)?OPAQUE:TRANSPARENT;
		*ppxlcDstBdr3++=(iSum3>iBdrThresh)?OPAQUE:TRANSPARENT;
		*ppxlcDstBdr4++=(iSum4>iBdrThresh)?OPAQUE:TRANSPARENT;
		ppxlcDstBdr1+=iDstSize;
		ppxlcDstBdr2+=iDstSize;
		ppxlcSrcBdr1+=MC_BAB_SIZE*iRate;
		ppxlcSrcBdr2+=MC_BAB_SIZE*iRate;
		ppxlcSrcBdr3+=iRate;
		ppxlcSrcBdr4+=iRate;
	}

	// corners
	ppxlcDst[0]=ppxlcSrc[0];
	ppxlcDst[iDstSize-1]=ppxlcSrc[MC_BAB_SIZE-1];
	ppxlcDst[(iDstSize-1)*iDstSize]=ppxlcSrc[(MC_BAB_SIZE-1)*MC_BAB_SIZE];
	ppxlcDst[iDstSize*iDstSize-1]=ppxlcSrc[MC_BAB_SIZE*MC_BAB_SIZE-1];

#ifdef __TRACE_AND_STATS_
	//m_pbitstrmOut->trace ((PixelC*)ppxlcDst, iDstSize, iDstSize, "MB_MC_BAB_DOWN");
#endif // __TRACE_AND_STATS_
}

Void CVideoObject::upSampleShape(PixelC*ppxlcBYFrm,const PixelC* rgpxlcSrc, PixelC* rgpxlcDst)
{
	if(m_iInverseCR==2)
		adaptiveUpSampleShape(rgpxlcSrc, rgpxlcDst,8,8);
	else
	{
		static PixelC rgpxlcTmp[12*12];
		
		assert(m_iInverseCR==4);
		adaptiveUpSampleShape(rgpxlcSrc, rgpxlcTmp,4,4);

		Int iSizeTmp=12;
		Int iSizeSrc=8;
		Int i,j;

		// top left corner 
		rgpxlcTmp[0]=rgpxlcSrc[0];
		rgpxlcTmp[1]=rgpxlcSrc[1];
		rgpxlcTmp[iSizeTmp]=rgpxlcSrc[iSizeSrc];
		rgpxlcTmp[iSizeTmp+1]=rgpxlcSrc[iSizeSrc+1];

		// top right corner
		rgpxlcTmp[iSizeTmp-2]=rgpxlcSrc[iSizeSrc-2];
		rgpxlcTmp[iSizeTmp-1]=rgpxlcSrc[iSizeSrc-1];
		rgpxlcTmp[(iSizeTmp<<1)-2]=rgpxlcSrc[(iSizeSrc<<1)-2];
		rgpxlcTmp[(iSizeTmp<<1)-1]=rgpxlcSrc[(iSizeSrc<<1)-1];

		// top border
		for(j=0;j<2;j++)
			for(i=BAB_BORDER;i<iSizeTmp-BAB_BORDER;i++)
				rgpxlcTmp[j*iSizeTmp+i]=rgpxlcSrc[j*iSizeSrc+i/2+1];

		// left border 
		for(i=0;i<2;i++)
			for(j=BAB_BORDER;j<iSizeTmp-BAB_BORDER;j++)
				rgpxlcTmp[j*iSizeTmp+i]=rgpxlcSrc[(j/2+1)*iSizeSrc+i];

		/*Int iTmp = m_iWidthCurrBAB;
		m_iInverseCR = 2;
		m_iWidthCurrBAB = 12;
		subsampleLeftTopBorderFromVOP (ppxlcBYFrm, rgpxlcTmp);
		m_iInverseCR = 4;
		m_iWidthCurrBAB = iTmp;*/
		
		adaptiveUpSampleShape(rgpxlcTmp, rgpxlcDst,8,8);
	}
}

Int CVideoObject::getContextUS( PixelC a,
                                PixelC b,
                                PixelC c,
                                PixelC d,
                                PixelC e,
                                PixelC f,
                                PixelC g,
                                PixelC h)
{
        Int     ret =(Int)(a+(b<<1)+(c<<2)+(d<<3)+(e<<4)+(f<<5)+(g<<6)+(h<<7));
 
        return  ret;
}

PixelC CVideoObject::getRefValue(const PixelC* ppxlcRow,
                                Int x_adr, Int y_adr,
                                Int h_size, Int v_size)
{
        assert ((x_adr>=-2) && (x_adr<=h_size+1) && (y_adr>=-2) && (y_adr<=v_size+1));
 
        Int     x_lim, y_lim;
        PixelC  ret;
 
        if(x_adr>=0 && x_adr<h_size && y_adr>=0 && y_adr<v_size) {
                ret=(ppxlcRow[y_adr*(h_size+BAB_BORDER_BOTH)+x_adr]>0) ? 1 : 0;
        } else {
                if(y_adr<0 || (x_adr<0 && y_adr<v_size)) {
			ret=(ppxlcRow[y_adr*(h_size+BAB_BORDER_BOTH)+x_adr]>0) ? 1 : 0;
                } else {
                        x_lim=(x_adr<0)? 0: (x_adr>=h_size)? h_size-1 :x_adr;
                        y_lim=(y_adr<0)? 0: (y_adr>=v_size)? v_size-1 :y_adr;
                        ret=(ppxlcRow[y_lim*(h_size+BAB_BORDER_BOTH)+x_lim]>0) ? 1 : 0;
                }
        }
 
        return ret;
}

Void CVideoObject::adaptiveUpSampleShape (const PixelC* rgpxlcSrc, 
						PixelC* rgpxlcDst,
						Int h_size, Int v_size)
{
	Int	x[12], y[12], other_total, context, rvalue, th, m;
	Int width_up = h_size << 1;
	Int width_up_border = width_up + BAB_BORDER_BOTH;
	Int j, i, py, px, py_st, py_en, px_st, px_en, py_p;
	PixelC	val[12];
	const PixelC* ppxlcRow = rgpxlcSrc + BAB_BORDER+BAB_BORDER * (h_size + BAB_BORDER_BOTH);

	/* --  4  5 -- */
	/*  6  0  1  7 */
	/*  8  2  3  9 */
	/* -- 10 11 -- */
	for (j = -1; j < v_size; j++) {

		y[0] = y[1] = y[6] =y[7] = j;
		y[2] = y[3] = y[8] =y[9] = j + 1;
		y[4] = y[5] = j - 1;
		y[10]= y[11]= j + 2;
		if (j > -1) 
			py_st = 0;
		else	 
			py_st = 1;
		if (j < v_size - 1) 
			py_en = 2;
		else	       
			py_en = 1;

		for(i = -1; i < h_size; i++) {
			x[0] = x[2] = x[4] = x[10] = i;
			x[1] = x[3] = x[5] = x[11] = i+1;
			x[6] = x[8] = i - 1;
			x[7] = x[9] = i + 2;

			for(m = 0; m < 12; m++)
				val[m] = getRefValue (ppxlcRow, x[m], y[m], h_size, v_size);

			if (i > -1) 
				px_st = 0;
			else	    
				px_st = 1;

			if (i < h_size-1) 
				px_en = 2;
			else		  
				px_en=1;		

			other_total = val[4] + val[5] + val[6] + val[7] + val[8] + val[9] + val[10] + val[11];
			for (py = py_st; py < py_en; py++)	{
				py_p = ((j << 1) + 1 + py + BAB_BORDER) * width_up_border + (i << 1) + 1;
				for (px = px_st; px < px_en; px++)	{
					if(px < 1 && py < 1)	{			
						context = getContextUS (val[5], val[4], val[6], val[8], val[10], val[11], val[9], val[7]);
						th = grgchInterpolationFilterTh[context];
						rvalue = (val[0] << 2) + ((val[1] + val[2] + val[3]) << 1) + other_total;
					} 
					else if	(py < 1) {
						context = getContextUS(val[9], val[7], val[5], val[4], val[6], val[8], val[10], val[11]);
						th = grgchInterpolationFilterTh [context];
						rvalue = (val[1] << 2) + ((val[0] + val[2] + val[3]) << 1) + other_total;
					} 
					else if (px < 1) {
						context = getContextUS(val[6], val[8], val[10], val[11], val[9], val[7], val[5], val[4]);
						th = grgchInterpolationFilterTh [context];
						rvalue = (val[2] << 2) + ((val[1] + val[0] + val[3]) << 1) + other_total;
					} 
					else {
						context = getContextUS(val[10], val[11], val[9], val[7], val[5], val[4], val[6], val[8]);
						th = grgchInterpolationFilterTh [context];
						rvalue = (val[3] << 2) + ((val[1] + val[2] + val[0]) << 1) + other_total;
					}
					rgpxlcDst [py_p+(px+BAB_BORDER)] = (rvalue > th) ? opaqueValue: transpValue;
				}
			}
		}
	}
}

Void CVideoObject::copyLeftTopBorderFromVOP (PixelC* ppxlcSrc, PixelC* ppxlcDst)
{
	PixelC* ppxlcSrcTop1  = ppxlcSrc - BAB_BORDER  * m_iFrameWidthY - BAB_BORDER;
	PixelC* ppxlcSrcTop2  = ppxlcSrcTop1 + m_iFrameWidthY;
	PixelC* ppxlcSrcLeft1 = ppxlcSrcTop1;
	PixelC* ppxlcSrcLeft2 = ppxlcSrcLeft1 + 1;
	PixelC* ppxlcDstTop1  = ppxlcDst;
	PixelC* ppxlcDstTop2  = ppxlcDst + TOTAL_BAB_SIZE;
	PixelC* ppxlcDstLeft  = ppxlcDst;
	CoordI iPixel;
//	Modified for error resilient mode by Toshiba(1997-11-14)
	for (iPixel = 0; iPixel < BAB_BORDER; iPixel++)	{
		if (!m_bVPNoLeftTop) {
			ppxlcDstTop1 [iPixel]  = *ppxlcSrcTop1;
			//src pixel never out of bound due to padding of Ref1
			ppxlcDstTop2 [iPixel]  = *ppxlcSrcTop2;
			//BY should be intialized as zero outside of VOP
		}
		else {
			ppxlcDstTop1 [iPixel]  = (PixelC) 0;
			ppxlcDstTop2 [iPixel]  = (PixelC) 0;
		}
		ppxlcSrcTop1++; 
		ppxlcSrcTop2++; 
	}
	for (iPixel = BAB_BORDER; iPixel < TOTAL_BAB_SIZE-BAB_BORDER; iPixel++)	{
		if (!m_bVPNoTop) {
			ppxlcDstTop1 [iPixel]  = *ppxlcSrcTop1;
			//src pixel never out of bound due to padding of Ref1
			ppxlcDstTop2 [iPixel]  = *ppxlcSrcTop2;
			//BY should be intialized as zero outside of VOP
		}
		else {
			ppxlcDstTop1 [iPixel]  = (PixelC) 0;
			ppxlcDstTop2 [iPixel]  = (PixelC) 0;
		}
		ppxlcSrcTop1++; 
		ppxlcSrcTop2++; 
	}
	for (iPixel = TOTAL_BAB_SIZE-BAB_BORDER; iPixel < TOTAL_BAB_SIZE; iPixel++)	{
		if (!m_bVPNoRightTop) {
			ppxlcDstTop1 [iPixel]  = *ppxlcSrcTop1;
			//src pixel never out of bound due to padding of Ref1
			ppxlcDstTop2 [iPixel]  = *ppxlcSrcTop2;
			//BY should be intialized as zero outside of VOP
		}
		else {
			ppxlcDstTop1 [iPixel]  = (PixelC) 0;
			ppxlcDstTop2 [iPixel]  = (PixelC) 0;
		}
		ppxlcSrcTop1++; 
		ppxlcSrcTop2++; 
	}
	ppxlcSrcLeft1 += BAB_BORDER*m_iFrameWidthY; 
	ppxlcSrcLeft2 += BAB_BORDER*m_iFrameWidthY; 
	ppxlcDstLeft += BAB_BORDER*TOTAL_BAB_SIZE;
	for (iPixel = BAB_BORDER; iPixel < TOTAL_BAB_SIZE; iPixel++)	{
		if (!m_bVPNoLeft) {
			*ppxlcDstLeft		= *ppxlcSrcLeft1;
			*(ppxlcDstLeft + 1)	= *ppxlcSrcLeft2;
		}
		else {
			*ppxlcDstLeft		= (PixelC) 0;
			*(ppxlcDstLeft + 1)	= (PixelC) 0;
		}
		ppxlcSrcLeft1 += m_iFrameWidthY; 
		ppxlcSrcLeft2 += m_iFrameWidthY; 
		ppxlcDstLeft += TOTAL_BAB_SIZE;				//last two values of left border will be over written after the loop
	}
	//repeat pad the left border (this will be done again in right bottom border; improve in the future)
	if (!m_bVPNoLeft) {
		Int iLastValidSrcLeft1 = *(ppxlcSrcLeft1 - (BAB_BORDER+1) * m_iFrameWidthY);
		Int iLastValidSrcLeft2 = *(ppxlcSrcLeft2 - (BAB_BORDER+1) * m_iFrameWidthY);
		for (iPixel = 0; iPixel < BAB_BORDER; iPixel++)	{
			ppxlcDstLeft -= TOTAL_BAB_SIZE;
			*ppxlcDstLeft       = iLastValidSrcLeft1;
			*(ppxlcDstLeft + 1) = iLastValidSrcLeft2;
		}
	}
// End Toshiba(1997-11-14)
}

Void CVideoObject::subsampleLeftTopBorderFromVOP (PixelC* ppxlcSrc, PixelC* ppxlcDst)
{
	PixelC* ppxlcSrcTop1 = ppxlcSrc - BAB_BORDER  * m_iFrameWidthY - BAB_BORDER;
	PixelC* ppxlcSrcTop2 = ppxlcSrcTop1 + m_iFrameWidthY;
	PixelC* ppxlcSrcLft1 = ppxlcSrcTop1;
	PixelC* ppxlcSrcLft2 = ppxlcSrcLft1 + 1;
	PixelC* ppxlcDstTop1 = ppxlcDst;
	PixelC* ppxlcDstTop2 = ppxlcDst + m_iWidthCurrBAB;
	PixelC* ppxlcDstLft1 = ppxlcDst;
	PixelC* ppxlcDstLft2 = ppxlcDst + 1;
	Int iThresh = (m_iInverseCR == 2) ? 0 : opaqueValue;
	CoordI iPixel, iPixelSub, iSampleInterval;
	//	Modified for error resilient mode by Toshiba(1997-11-14)
	for (iPixelSub = BAB_BORDER, iSampleInterval = BAB_BORDER; 
		iPixelSub < m_iWidthCurrBAB - BAB_BORDER; 
		iPixelSub++, iSampleInterval += m_iInverseCR) {					//get each subsample
		Int iTempSumTop1 = 0, iTempSumTop2 = 0; 
		Int iTempSumLft1 = 0, iTempSumLft2 = 0;
		for (iPixel = 0; iPixel < m_iInverseCR; iPixel++)	{
			iTempSumTop1 += ppxlcSrcTop1 [iSampleInterval + iPixel];
			iTempSumTop2 += ppxlcSrcTop2 [iSampleInterval + iPixel];
			iTempSumLft1 += ppxlcSrcLft1 [(iSampleInterval + iPixel) * m_iFrameWidthY];
			iTempSumLft2 += ppxlcSrcLft2 [(iSampleInterval + iPixel) * m_iFrameWidthY];
		}
		if (!m_bVPNoTop) {
			ppxlcDstTop1 [iPixelSub] = (iTempSumTop1 > iThresh) ? opaqueValue : transpValue;
			ppxlcDstTop2 [iPixelSub] = (iTempSumTop2 > iThresh) ? opaqueValue : transpValue;
		}
		else {
			ppxlcDstTop1 [iPixelSub] = 0;
			ppxlcDstTop2 [iPixelSub] = 0;
		}
		if (!m_bVPNoLeft) {
			ppxlcDstLft1 [iPixelSub * m_iWidthCurrBAB] = (iTempSumLft1 > iThresh) ? opaqueValue : transpValue;
			ppxlcDstLft2 [iPixelSub * m_iWidthCurrBAB] = (iTempSumLft2 > iThresh) ? opaqueValue : transpValue;
		}
		else {
			ppxlcDstLft1 [iPixelSub * m_iWidthCurrBAB] = 0;
			ppxlcDstLft2 [iPixelSub * m_iWidthCurrBAB] = 0;
		}
	}
	//boundary conditions 0, 1, 18, 19 
	CoordI iBorder;
	for (iBorder = 0; iBorder < BAB_BORDER; iBorder++)	{
		if (!m_bVPNoLeftTop) {
			ppxlcDstTop1 [iBorder] = ppxlcSrcTop1 [iBorder];
			ppxlcDstTop2 [iBorder] = ppxlcSrcTop2 [iBorder];
		}
		else {
			ppxlcDstTop1 [iBorder] = 0;
			ppxlcDstTop2 [iBorder] = 0;
		}
		if (!m_bVPNoRightTop) {
			ppxlcDstTop1 [m_iWidthCurrBAB - 1 - iBorder] = ppxlcSrcTop1 [TOTAL_BAB_SIZE - 1 - iBorder];
			ppxlcDstTop2 [m_iWidthCurrBAB - 1 - iBorder] = ppxlcSrcTop2 [TOTAL_BAB_SIZE - 1 - iBorder];
		}
		else {
			ppxlcDstTop1 [m_iWidthCurrBAB - 1 - iBorder] = 0;
			ppxlcDstTop2 [m_iWidthCurrBAB - 1 - iBorder] = 0;
		}
	}
	// End Toshiba(1997-11-14)
	//repeat pad left-bottom corner
	ppxlcDstLft1 [(m_iWidthCurrBAB - 1) * m_iWidthCurrBAB] = 
		ppxlcDstLft1 [(m_iWidthCurrBAB - 2) * m_iWidthCurrBAB] =
		ppxlcDstLft1 [(m_iWidthCurrBAB - 3) * m_iWidthCurrBAB];	
	ppxlcDstLft2 [(m_iWidthCurrBAB - 1) * m_iWidthCurrBAB] =
		ppxlcDstLft2 [(m_iWidthCurrBAB - 2) * m_iWidthCurrBAB] =
		ppxlcDstLft2 [(m_iWidthCurrBAB - 3) * m_iWidthCurrBAB];
}

Void CVideoObject::makeRightBottomBorder (PixelC* ppxlcSrc, Int iWidth)
{
	Int i;
	PixelC* ppxlcDst = ppxlcSrc + BAB_BORDER * iWidth + iWidth - BAB_BORDER;		//make right border
	for (i = 0; i < iWidth - 2*BAB_BORDER; i++) {
		*ppxlcDst = *(ppxlcDst - 1);
		*(ppxlcDst + 1) = *ppxlcDst;
		ppxlcDst += iWidth;
	}
	ppxlcDst = ppxlcSrc + (iWidth - BAB_BORDER) * iWidth;							//make bottom border
	for (i = 0; i < iWidth; i++) {
		*ppxlcDst = *(ppxlcDst - iWidth);
		*(ppxlcDst + iWidth) = *ppxlcDst;
		ppxlcDst++;
	}
}

Int CVideoObject::contextIntra (const PixelC* ppxlcSrc)
{
	Int iContext = 0, i;
	static Int rgiNeighbourIndx [10];
	rgiNeighbourIndx [0] = -1;
	rgiNeighbourIndx [1] = -2;
	rgiNeighbourIndx [2] = -m_iWidthCurrBAB + 2;
	rgiNeighbourIndx [3] = -m_iWidthCurrBAB + 1;
	rgiNeighbourIndx [4] = -m_iWidthCurrBAB;
	rgiNeighbourIndx [5] = -m_iWidthCurrBAB - 1;
	rgiNeighbourIndx [6] = -m_iWidthCurrBAB - 2;
	rgiNeighbourIndx [7] = -2 * m_iWidthCurrBAB + 1;
	rgiNeighbourIndx [8] = -2 * m_iWidthCurrBAB;
	rgiNeighbourIndx [9] = -2 * m_iWidthCurrBAB - 1;
	for (i = 0; i < 10; i++)
		iContext += (ppxlcSrc [rgiNeighbourIndx [i]] == OPAQUE) << i;
	assert (iContext >= 0 && iContext < 1024);
	return iContext;
}

Int CVideoObject::contextIntraTranspose (const PixelC* ppxlcSrc)
{
	Int iContext = 0, i;
	static Int rgiNeighbourIndx [10];
	rgiNeighbourIndx [0] = -m_iWidthCurrBAB;
	rgiNeighbourIndx [1] = -2 * m_iWidthCurrBAB;
	rgiNeighbourIndx [2] = -1 + 2 * m_iWidthCurrBAB;
	rgiNeighbourIndx [3] = -1 + m_iWidthCurrBAB;
	rgiNeighbourIndx [4] = -1;
	rgiNeighbourIndx [5] = -m_iWidthCurrBAB - 1;
	rgiNeighbourIndx [6] = -1 - 2 * m_iWidthCurrBAB;
	rgiNeighbourIndx [7] = -2 + m_iWidthCurrBAB;
	rgiNeighbourIndx [8] = -2;
	rgiNeighbourIndx [9] = -2 - m_iWidthCurrBAB;
	for (i = 0; i < 10; i++)
		iContext += (ppxlcSrc [rgiNeighbourIndx [i]] == OPAQUE) << i;
	assert (iContext >= 0 && iContext < 1024);
	return iContext;
}

Int CVideoObject::contextInter (const PixelC* ppxlcSrc, const PixelC* ppxlcPred)
{
	Int i, iContext = 0;
	static Int rgiNeighbourIndx [9];
	Int iSizePredBAB = m_iWidthCurrBAB - 2*BAB_BORDER + 2*MC_BAB_BORDER;

	rgiNeighbourIndx [0] = -1;
	rgiNeighbourIndx [1] = -m_iWidthCurrBAB + 1;
	rgiNeighbourIndx [2] = -m_iWidthCurrBAB;
	rgiNeighbourIndx [3] = -m_iWidthCurrBAB - 1;
	rgiNeighbourIndx [4] = iSizePredBAB;
	rgiNeighbourIndx [5] = 1;
	rgiNeighbourIndx [6] = 0;
	rgiNeighbourIndx [7] = -1;
	rgiNeighbourIndx [8] = -iSizePredBAB;

	for (i = 0; i < 4; i++)	{
		iContext += (ppxlcSrc [rgiNeighbourIndx [i]] == OPAQUE) << i;
	}

	for (i = 4; i < 9; i++)	{
		iContext += (ppxlcPred [rgiNeighbourIndx [i]] == OPAQUE) << i;
	}
	assert (iContext >= 0 && iContext < 1024);
	return iContext;
}

Int CVideoObject::contextInterTranspose (const PixelC* ppxlcSrc, const PixelC* ppxlcPred)
{
	Int i, iContext = 0;
	static Int rgiNeighbourIndx [9];
	Int iSizePredBAB = m_iWidthCurrBAB - 2*BAB_BORDER + 2*MC_BAB_BORDER;

	rgiNeighbourIndx [0] = -m_iWidthCurrBAB;
	rgiNeighbourIndx [1] = m_iWidthCurrBAB - 1;
	rgiNeighbourIndx [2] = -1;
	rgiNeighbourIndx [3] = -m_iWidthCurrBAB - 1;
	rgiNeighbourIndx [4] = 1;
	rgiNeighbourIndx [5] = iSizePredBAB;
	rgiNeighbourIndx [6] = 0;
	rgiNeighbourIndx [7] = -iSizePredBAB;
	rgiNeighbourIndx [8] = -1;	

	for (i = 0; i < 4; i++)
		iContext += (ppxlcSrc [rgiNeighbourIndx [i]] == OPAQUE) << i;

	for (i = 4; i < 9; i++)
		iContext += (ppxlcPred [rgiNeighbourIndx [i]] == OPAQUE) << i;
	assert (iContext >= 0 && iContext < 1024);
	return iContext;
}

Void CVideoObject::copyReconShapeToMbAndRef (
	PixelC* ppxlcDstMB, 
	PixelC* ppxlcRefFrm, PixelC pxlcSrc
)
{
	pxlcmemset (ppxlcDstMB, pxlcSrc, MB_SQUARE_SIZE); // Modified by swinder
	for (Int i = 0; i < MB_SIZE; i++)	{
		pxlcmemset (ppxlcRefFrm, pxlcSrc, MB_SIZE);
		ppxlcRefFrm += m_iFrameWidthY;
	}
}

Void CVideoObject::copyReconShapeUVToRef (
	PixelC* ppxlcRefFrm, const PixelC* ppxlcSrc)
{
	for (Int i = 0; i < BLOCK_SIZE; i++)	{
		memcpy (ppxlcRefFrm, ppxlcSrc, BLOCK_SIZE*sizeof(PixelC));
		ppxlcRefFrm += m_iFrameWidthUV;
		ppxlcSrc += BLOCK_SIZE;
	}
}

Void CVideoObject::copyReconShapeToMbAndRef (
	PixelC* ppxlcDstMB, 
	PixelC* ppxlcRefFrm, 
	const PixelC* ppxlcSrc, Int iSrcWidth, Int iBorder
)
{
	Int iUnit = sizeof(PixelC); // NBIT: for memcpy
	ppxlcSrc += iSrcWidth * iBorder + iBorder;
	for (Int i = 0; i < MB_SIZE; i++)	{
		memcpy (ppxlcDstMB, ppxlcSrc, MB_SIZE*iUnit);
		memcpy (ppxlcRefFrm, ppxlcSrc, MB_SIZE*iUnit);
		ppxlcRefFrm += m_iFrameWidthY;
		ppxlcDstMB  += MB_SIZE;
		ppxlcSrc += iSrcWidth;
	}
}

CMotionVector CVideoObject::findShapeMVP (
	const CMotionVector* pmv, const CMotionVector* pmvBY, 
	const CMBMode* pmbmd,
	Int iMBX, Int iMBY
) const
{
	CMotionVector mvRet;
	// try shape vector first

	Bool bLeftBndry, bRightBndry, bTopBndry;
	Int iMBnum = VPMBnum(iMBX, iMBY);
	bLeftBndry = bVPNoLeft(iMBnum, iMBX);
	bTopBndry = bVPNoTop(iMBnum);
	bRightBndry = bVPNoRightTop(iMBnum, iMBX);

	mvRet = *(pmvBY - 1);	
//	Modified for error resilient mode by Toshiba(1997-11-14)
//	if ((iMBX > 0) && ((pmbmd - 1)->m_shpmd == INTER_CAE_MVDZ ||
	if (!bLeftBndry && ((pmbmd - 1)->m_shpmd == INTER_CAE_MVDZ ||
		(pmbmd - 1)->m_shpmd == INTER_CAE_MVDNZ ||
		(pmbmd - 1)->m_shpmd == MVDZ_NOUPDT ||
		(pmbmd - 1)->m_shpmd == MVDNZ_NOUPDT))
		return mvRet;
	if (iMBY > 0)   {
		mvRet = *(pmvBY - m_iNumMBX);
//	Modified for error resilient mode by Toshiba(1997-11-14)
//		if ((pmbmd - m_iNumMBX)->m_shpmd == INTER_CAE_MVDZ ||
		if (!bTopBndry && ((pmbmd - m_iNumMBX)->m_shpmd == INTER_CAE_MVDZ ||
			(pmbmd - m_iNumMBX)->m_shpmd == INTER_CAE_MVDNZ ||
			(pmbmd - m_iNumMBX)->m_shpmd == MVDZ_NOUPDT ||
			(pmbmd - m_iNumMBX)->m_shpmd == MVDNZ_NOUPDT
//	Modified for error resilient mode by Toshiba(1997-11-14)
//		)
		))
			return mvRet;			
		mvRet = *(pmvBY - m_iNumMBX + 1);
//	Modified for error resilient mode by Toshiba(1997-11-14)
//		if ((iMBX < m_iNumMBX - 1) && ((pmbmd - m_iNumMBX + 1)->m_shpmd == INTER_CAE_MVDZ ||
		if (!bRightBndry && ((pmbmd - m_iNumMBX + 1)->m_shpmd == INTER_CAE_MVDZ ||
			(pmbmd - m_iNumMBX + 1)->m_shpmd == INTER_CAE_MVDNZ ||
			(pmbmd - m_iNumMBX + 1)->m_shpmd == MVDZ_NOUPDT ||
			(pmbmd - m_iNumMBX + 1)->m_shpmd == MVDNZ_NOUPDT)
		)
			return mvRet;
	}

	// try texture vector then; truncate half pel
	if(m_volmd.bShapeOnly==FALSE && m_vopmd.vopPredType==PVOP)
	{
//	Modified for error resilient mode by Toshiba(1997-11-14)
//		if (iMBX != 0 && validBlock (pmbmd - 1, gIndexOfCandBlk [1] [0]))
		if (!bLeftBndry && validBlock (pmbmd, pmbmd - 1, gIndexOfCandBlk [1] [0]))
			return CMotionVector ((pmv - PVOP_MV_PER_REF_PER_MB + gIndexOfCandBlk [1] [0])->m_vctTrueHalfPel.x / 2,
								  (pmv - PVOP_MV_PER_REF_PER_MB + gIndexOfCandBlk [1] [0])->m_vctTrueHalfPel.y / 2);
		if (iMBY != 0)   {
//	Modified for error resilient mode by Toshiba(1997-11-14)
//			if (validBlock (pmbmd - m_iNumMBX, gIndexOfCandBlk [1] [1]))
			if (!bTopBndry && validBlock (pmbmd, pmbmd - m_iNumMBX, gIndexOfCandBlk [1] [1]))
				return CMotionVector ((pmv - m_iNumOfTotalMVPerRow + gIndexOfCandBlk [1] [1])->m_vctTrueHalfPel.x / 2,
									  (pmv - m_iNumOfTotalMVPerRow + gIndexOfCandBlk [1] [1])->m_vctTrueHalfPel.y / 2);
//	Modified for error resilient mode by Toshiba(1997-11-14)
//			if (iMBX < m_iNumMBX - 1 && validBlock (pmbmd - m_iNumMBX + 1, gIndexOfCandBlk [1] [2]))
			if (!bRightBndry && validBlock (pmbmd, pmbmd - m_iNumMBX + 1, gIndexOfCandBlk [1] [2]))
				return CMotionVector ((pmv - m_iNumOfTotalMVPerRow + PVOP_MV_PER_REF_PER_MB + gIndexOfCandBlk [1] [2])->m_vctTrueHalfPel.x / 2,
									  (pmv - m_iNumOfTotalMVPerRow + PVOP_MV_PER_REF_PER_MB + gIndexOfCandBlk [1] [2])->m_vctTrueHalfPel.y / 2);
		}
	}
	return CMotionVector ();																		
}

/******************************************************************************
 *                                                                          
 * This software module was originally developed by 
 *
 *   Fujitsu Laboratories Ltd. (contact: Eishi Morimatsu)
 *
 * in the course of development of the MPEG-4 Video (ISO/IEC 14496-2) standard.
 * This software module is an implementation of a part of one or more MPEG-4 
 * Video (ISO/IEC 14496-2) tools as specified by the MPEG-4 Video (ISO/IEC 
 * 14496-2) standard. 
 *
 * ISO/IEC gives users of the MPEG-4 Video (ISO/IEC 14496-2) standard free 
 * license to this software module or modifications thereof for use in hardware
 * or software products claiming conformance to the MPEG-4 Video (ISO/IEC 
 * 14496-2) standard. 
 *
 * Those intending to use this software module in hardware or software products
 * are advised that its use may infringe existing patents. THE ORIGINAL 
 * DEVELOPER OF THIS SOFTWARE MODULE AND HIS/HER COMPANY, THE SUBSEQUENT 
 * EDITORS AND THEIR COMPANIES, AND ISO/IEC HAVE NO LIABILITY FOR USE OF THIS 
 * SOFTWARE MODULE OR MODIFICATIONS THEREOF. No license to this software
 * module is granted for non MPEG-4 Video (ISO/IEC 14496-2) standard 
 * conforming products.
 *
 * Fujitsu Laboratories Ltd. retains full right to use the software module for 
 * his/her own purpose, assign or donate the software module to a third party 
 * and to inhibit third parties from using the code for non MPEG-4 Video 
 * (ISO/IEC 14496-2) standard conforming products. This copyright notice must 
 * be included in all copies or derivative works of the software module. 
 *
 * Copyright (c) 1999.  
 *

Module Name:

	rrv.cpp

Abstract:

	Utility functions for Reduced Resolution Vop (RRV) mode

Revision History:

 *****************************************************************************/
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
//#include <fstream.h>
//#include <iostream.h>
//#include "mpeg4ip.h"
#include "typeapi.h"
#include "codehead.h"
#include "global.hpp"
#include "mode.hpp"
#include "bitstrm.hpp"
class ofstream;
#include "sesenc.hpp"
#include "vopses.hpp"
#include "vopseenc.hpp"
#include "vopsedec.hpp"


#define RRV_MAX(x,y) (((x)>(y))?(x):(y))
#define RRV_MIN(x,y) (((x)<(y))?(x):(y))
#define RRV_BLOCK_SIZE 16

#define	RRV_C1	2.5
#define	RRV_C2	2.5
#define	RRV_FR1	6.0
#define	RRV_FR2	8.0
#define	RRV_QP1	14
#define	RRV_QP2	6

//
// functions refered from other files
//
Void	MotionVectorScalingDown(CMotionVector*, Int, Int);
Void	MotionVectorScalingUp(CMotionVector*,Int, Int);
Void	DownSamplingTextureForRRV(PixelC*, PixelC*,	Int, Int);
Void	DownSamplingTextureForRRV(PixelI*, PixelI*, Int, Int);
Void	UpSamplingTextureForRRV(PixelC*, PixelC*, Int, Int, Int);
Void	UpSamplingTextureForRRV(PixelI*, PixelI*, Int, Int, Int);
Void	writeCubicRct(Int, Int, PixelC*, PixelC*);
Void	writeCubicRct(Int, Int, PixelI*, PixelI*);
Void	MeanUpSampling(PixelC*, PixelC*, Int, Int);
Void	MeanUpSampling(PixelI*, PixelI*, Int, Int);

//
// functions refered only in this file
//
Void	MotionVectorScalingDownMB(CMotionVector*);
Void	calculateMVdown(Float*);
Void	MotionVectorScalingUpMB(CMotionVector*);
Void	calculateMVup(Float *pfvec);
Void	MotionVectorChange(Float, Int*, Int*);
Float	MotionVectorChange(Int, Int);
Void	filterMBVarBorder(PixelC*, Int, Int, Int);
Void	filterMBHorBorder(PixelC*, Int, Int, Int);

/*
Void	CVideoObjectEncoder::redefineVOLMembersRRV();
Void	CVideoObjectEncoder::cutoffDCTcoef();
Void	CVideoObjectEncoder::resetAndCalcRRV();
Void	CVideoObjectDecoder::redefineVOLMembersRRV();
Void	CVideoObject::filterCodedPictureForRRV(PixelC*, PixelC*, PixelC*, 
											   Int isizex, Int isizey,
											   Int inumMBx,Int inumMBy, 
											   Int iwidthY, Int iwidthUV);
*/

Void	CVideoObjectEncoder::cutoffDCTcoef()
{
	if((m_vopmd.RRVmode.iCutoffThr == 8)||(m_vopmd.RRVmode.iOnOff != 1))
	{
		return;
	}

	assert((m_vopmd.RRVmode.iCutoffThr >= 4)&&(m_vopmd.RRVmode.iCutoffThr <= 7));
	Int		x, y, pos;

	for(y = 0 ; y < BLOCK_SIZE; y ++)
	{
		for(x = 0 ; x < BLOCK_SIZE; x ++)
		{
			if((x >= m_vopmd.RRVmode.iCutoffThr)||(y >= m_vopmd.RRVmode.iCutoffThr))
			{
				pos	= x +y *BLOCK_SIZE;
				m_rgiDCTcoef[pos]	= 0;
			}
		}
	}
}

Void	CVideoObjectEncoder::redefineVOLMembersRRV()
{
	m_iNumMBX	= m_iVOPWidthY / MB_SIZE;
	m_iNumMBY	= m_rctCurrVOPY.height () / MB_SIZE;
	m_iNumMB	= m_iNumMBX * m_iNumMBY;
	m_iNumOfTotalMVPerRow	= PVOP_MV_PER_REF_PER_MB * m_iNumMBX;
	if(m_vopmd.RRVmode.iRRVOnOff == 1)
	{
		m_iRRVScale	= 2;
		m_iNumMBX	= m_iNumMBX /2;
		m_iNumMBY	= m_iNumMBY /2;
		m_iNumMB	= m_iNumMB /4;
		m_iNumOfTotalMVPerRow	= m_iNumOfTotalMVPerRow /2;
	}
	else
	{
		m_iRRVScale	= 1;
	}
	m_iFrameWidthYxMBSize = (MB_SIZE *m_iRRVScale) 
		*m_pvopcRefQ0->whereY ().width; 
	m_iFrameWidthYxBlkSize = (BLOCK_SIZE *m_iRRVScale) 
		*m_pvopcRefQ0->whereY ().width; 
	m_iFrameWidthUVxBlkSize = (BLOCK_SIZE *m_iRRVScale) *
		m_pvopcRefQ0->whereUV ().width;
}


Void	CVideoObjectEncoder::resetAndCalcRRV()
{
	assert(m_vopmd.RRVmode.iOnOff == 1);
	assert(m_vopmd.RRVmode.iCycle != 0);
	
	static Int iCount	= 0;
	static Int iFirst	= 0;
	
    /* fixed interval mode */
	if(m_vopmd.RRVmode.iCycle == 0)
	{
		m_vopmd.RRVmode.iRRVOnOff	= 0;
	}
	else if(m_vopmd.RRVmode.iCycle < 0)
	{
//		m_vopmd.RRVmode.iCutoffThr	= 8;
		switch(iFirst)
		{
		  case 0:
			m_vopmd.RRVmode.iRRVOnOff	= 0;
			m_vopmd.RRVmode.iCutoffThr	= 8;
			iCount++;
			iFirst	= 1;
			break;
		  case 1:
			iCount	= iCount % m_vopmd.RRVmode.iCycle;
			if(iCount == 0)
			{
				m_vopmd.RRVmode.iRRVOnOff	^= 0x01;
				if(m_vopmd.RRVmode.iRRVOnOff == 0)
				{
				    m_vopmd.RRVmode.iCutoffThr	= 4;
				}
				else
				{
				    m_vopmd.RRVmode.iCutoffThr	= 8;
				}
			}
			else
			{
			    if(m_vopmd.RRVmode.iCutoffThr < 8)
			    {
				m_vopmd.RRVmode.iCutoffThr ++;
			    }
			}
			iCount++;	      
			break;
		  default:
			fprintf(stderr, "Error in resetAndCalcRRV\n");
			exit(1);
			break;
		}
	}
	/* adaptive mode */
	else
	{
		Float	f_alpha, f_TH1, f_TH2;

		if(m_vopmd.RRVmode.iQave == 0)
		{
			/* perhaps first I-VOP */
			m_vopmd.RRVmode.iRRVOnOff	= 0;
			m_vopmd.RRVmode.iCutoffThr	= 8;
			return;
		}
		f_alpha	= (Float)(m_vopmd.RRVmode.iQave) 
			*(Float)(m_vopmd.RRVmode.iNumBits);

		m_vopmd.RRVmode.iQP1	= RRV_QP1;
		m_vopmd.RRVmode.iQP2	= RRV_QP2;
		m_vopmd.RRVmode.fFR1	= RRV_FR1;
		m_vopmd.RRVmode.fFR2	= RRV_FR2;
		
//		printf("target bitrate %d\n", m_iBufferSize);
		
		f_TH1	= m_vopmd.RRVmode.iQP1 *(Float)(m_iBufferSize) 
			/m_vopmd.RRVmode.fFR1;
		
		f_TH2	= m_vopmd.RRVmode.iQP2 *(Float)(m_iBufferSize) 
			/m_vopmd.RRVmode.fFR2;

		if((m_vopmd.RRVmode.iRRVOnOff == 0)&&(f_alpha > f_TH1))
		{
			m_vopmd.RRVmode.iRRVOnOff	= 1;
			m_vopmd.RRVmode.iCutoffThr	= 8;
		}
		else if((m_vopmd.RRVmode.iRRVOnOff == 1)&&(f_alpha < f_TH2))
		{
			m_vopmd.RRVmode.iRRVOnOff	= 0;
			m_vopmd.RRVmode.iCutoffThr	= 4;
		}
		else if((m_vopmd.RRVmode.iRRVOnOff == 0)&&(m_vopmd.RRVmode.iCutoffThr < 8))
		{
			m_vopmd.RRVmode.iCutoffThr ++;
		}

//		printf("RRV_debug: QxB = %f, TH1 = %f, TH2 = %f, Mode = %d, Thr = %d\n",
//			   f_alpha, f_TH1, f_TH2, m_vopmd.RRVmode.iRRVOnOff, m_vopmd.RRVmode.iCutoffThr);
	}
}


Void CVideoObjectDecoder::redefineVOLMembersRRV()
{
	m_iRRVScale	= (m_vopmd.RRVmode.iRRVOnOff == 1) ? (2) : (1);
	
	m_iNumMBX = m_iVOPWidthY / MB_SIZE /m_iRRVScale;
	m_iNumMBY = m_rctCurrVOPY.height () / MB_SIZE /m_iRRVScale;
	
	m_iFrameWidthYxMBSize = (MB_SIZE *m_iRRVScale) 
		* m_pvopcRefQ0->whereY ().width;
	m_iFrameWidthYxBlkSize = (BLOCK_SIZE *m_iRRVScale) 
		* m_pvopcRefQ0->whereY ().width;
	m_iFrameWidthUVxBlkSize = (BLOCK_SIZE *m_iRRVScale)
		* m_pvopcRefQ0->whereUV ().width;

	m_iNumMB = m_iNumMBX * m_iNumMBY;
	m_iNumOfTotalMVPerRow = PVOP_MV_PER_REF_PER_MB * m_iNumMBX;
}

Void MotionVectorScalingDown(CMotionVector* pmv, Int inumMB, Int iMVperMB)
{
	Int	i;

	for(i = 0; i < inumMB *iMVperMB; i++)
	{
		MotionVectorScalingDownMB(&pmv[i]);
	}
}

Void MotionVectorScalingDownMB(CMotionVector* pmv)
{
//see basic.hpp
    pmv->computeMV();
    Float vx	= MotionVectorChange(pmv->iMVX,pmv->iHalfX);
    Float vy	= MotionVectorChange(pmv->iMVY,pmv->iHalfY);

    calculateMVdown(&vx);
    calculateMVdown(&vy);

    MotionVectorChange(vx, &(pmv->iMVX), &(pmv->iHalfX));
    MotionVectorChange(vy, &(pmv->iMVY), &(pmv->iHalfY));

    pmv->computeTrueMV();
}

Void calculateMVdown(Float *pfvec)
{
	Float fvec	= (Float) *pfvec;

	if(fvec == 0.0)
	{
		*pfvec	= 0.0;
	}
	else if(fvec > 0.0)
	{
		*pfvec	= (fvec + 0.5) /2.0;
	}
	else if(fvec < 0.0)
	{
		*pfvec	= (fvec - 0.5) /2.0;
	}
}

Void MotionVectorScalingUp(CMotionVector* pmv,Int inumMB,Int iMVperMB)
{
  Int i;
  CMotionVector* plocalpmv = pmv;

  for (i=0;i<inumMB*iMVperMB;i++)
    MotionVectorScalingUpMB(plocalpmv++);
}

Void MotionVectorScalingUpMB(CMotionVector* pmv)
{
//see basic.hpp
    pmv->computeMV();
    Float fvx	= MotionVectorChange(pmv->iMVX,pmv->iHalfX);
    Float fvy	= MotionVectorChange(pmv->iMVY,pmv->iHalfY);

    calculateMVup(&fvx);
    calculateMVup(&fvy);

    MotionVectorChange(fvx, &(pmv->iMVX), &(pmv->iHalfX));
    MotionVectorChange(fvy, &(pmv->iMVY), &(pmv->iHalfY));

    pmv->computeTrueMV();
}

Void calculateMVup(Float *pfvec)
{
	Float fvec	= (Float) *pfvec;

	if(fvec == 0.0)
	{
		*pfvec	= 0.0;
	}
	else if(fvec > 0.0)
	{
		*pfvec	= 2.0 * fvec - 0.5;
	}
	else if(fvec < 0.0)
	{
		*pfvec	= 2.0 * fvec + 0.5;
	}
}

Void MotionVectorChange(Float fvec, Int *iMV, Int *iHalfV)
{
//Float --> Int
	Int	ilMV,ilHV;

	ilMV	= nint(fvec);
	ilHV	= nint((fvec - (Float) ilMV) *2.0);

	*iMV = ilMV;
	*iHalfV = ilHV;
}

Float MotionVectorChange(Int iMV, Int iHalfV)
{
//Int --> Float
  return((Float) iMV + ((Float) iHalfV) /2.0);
}

Void DownSamplingTextureForRRV(PixelC *pPxlc, PixelC *pretPxlc,
								 Int iwidth, Int ihight)
{
	Int	i, j, ival;
	
	PixelC *pnewblock	= new PixelC[iwidth *ihight /4];
	
	for(j = 0; j < ihight /2; j++)
	{
		for(i = 0; i < iwidth /2 ;i++)
		{
			ival	= pPxlc[i * 2 + j * 2 * iwidth];
			ival	+= pPxlc[i * 2 + 1 + j * 2 * iwidth];
			ival	+= pPxlc[i * 2 + (j * 2 + 1)* iwidth];
			ival	+= pPxlc[i * 2 + 1 + (j * 2 + 1)* iwidth];
			pnewblock[i+j*iwidth/2] = ((ival + 2) / 4);
		}
	}
	for(i = 0; i < iwidth *ihight /4; i++)
	{
		pretPxlc[i]	= pnewblock[i];
	}
	delete [] pnewblock;
}

Void DownSamplingTextureForRRV(PixelI *pPxli, PixelI *pretPxli,
								 Int iwidth, Int ihight)
{
	Int i,j,ival;
	PixelI *pnewblock	= new PixelI[iwidth *ihight /4];

	for(j = 0; j < ihight /2; j++)
	{
		for(i = 0; i < iwidth /2; i++)
		{
			ival	= pPxli[i * 2 + j * 2 * iwidth];
			ival	+= pPxli[i * 2 + 1 + j * 2 * iwidth];
			ival	+= pPxli[i * 2 + (j * 2 + 1)* iwidth];
			ival	+= pPxli[i * 2 + 1 + (j * 2 + 1)* iwidth];
			pnewblock[i+j*iwidth/2]= ((ival + 2) / 4);
		}
	}
	for(i = 0; i < iwidth *ihight /4; i++)
	{
		pretPxli[i]	= pnewblock[i];
	}
	delete [] pnewblock;
}

//
//
//UpSamplingTextureforRRV
//
//
Void UpSamplingTextureForRRV(PixelC *pPixlc, PixelC *pretPxlc,
							 Int iwidth, Int ihight,Int iwidthofpPix)
{
	Int	ibx, iby, ix, iy, ipos, icnt;
	PixelC* pblock_low	=  new PixelC [64];
	PixelC* pblock_norm	= new PixelC [256];
	PixelC* pnormal_data	= new PixelC [iwidth *ihight *4];
	Int inor_sizex 	= iwidth *2;
	Int inor_sizey	= ihight *2;

	for(iby = 0; iby < (inor_sizey /2); iby += 8)
	{
		for(ibx = 0; ibx < (inor_sizex/2); ibx += 8)
		{
			for(iy = iby, icnt = 0; iy < iby + 8; iy ++)
			{
				ipos = ibx + (iy) * iwidthofpPix;
				for(ix = ibx; ix < ibx + 8; ix++)
				{
					pblock_low[icnt]	= pPixlc[ipos];
					icnt++;
					ipos++;
				}
			}
			MeanUpSampling(pblock_low, pblock_norm, 8, 8);
			
			for(iy = 2 * iby, icnt = 0; iy < 2 * iby + 16; iy ++)
			{
				ipos	= 2 * ibx + (iy) * inor_sizex;
				for(ix = 2 * ibx; ix < 2 * ibx + 16; ix++)
				{
					pnormal_data[ipos]	= pblock_norm[icnt];
					icnt++;
					ipos++;
				}
			}
		}
	}

	for(iy = 0; iy < inor_sizey; iy++)
	{
		for(ix = 0; ix < inor_sizex; ix++)
		{
			pretPxlc[ix +(iy *iwidthofpPix)]
				= pnormal_data[ix +(iy *inor_sizex)];
		}
	}

	delete [] pblock_norm;
	delete [] pblock_low;
	delete [] pnormal_data;
}

// for error(m_ppxliErrorMBY)
void UpSamplingTextureForRRV(PixelI *pPixlc, PixelI *pretPxlc, 
							 Int iwidth, Int ihight,Int iwidthofpPix)
{
	Int	ibx, iby, ix, iy, ipos, icnt;
	PixelI* pblock_low	=  new PixelI [64];
	PixelI* pblock_norm	= new PixelI [256];
	PixelI* pnormal_data	= new PixelI [iwidth *ihight *4];
	Int inor_sizex	= iwidth *2;
	Int inor_sizey	= ihight *2;
  
	for(iby = 0; iby < (inor_sizey/2); iby+=8)
	{
		for(ibx = 0; ibx < (inor_sizex/2); ibx+=8)
		{
			for(iy = iby, icnt = 0; iy < iby + 8; iy ++)
			{
				ipos = ibx + (iy) * iwidth;
				for(ix = ibx; ix < ibx + 8; ix++)
				{
					pblock_low[icnt]	= pPixlc[ipos];
					icnt++;
					ipos++;
				}
			}

			MeanUpSampling(pblock_low, pblock_norm, 8, 8);

			for(iy = 2 * iby, icnt = 0; iy < 2 * iby + 16; iy ++)
			{
				ipos = 2 * ibx + (iy) * inor_sizex;
				for(ix = 2 * ibx; ix < 2 * ibx + 16; ix++)
				{
					pnormal_data[ipos]	= pblock_norm[icnt];
					icnt++;
					ipos++;
				}
			}
		}
	}

	for(iy = 0; iy < inor_sizey; iy++)
	{
		for(ix = 0; ix < inor_sizex; ix++)
		{
			pretPxlc[ix +(iy *iwidthofpPix)]
				= pnormal_data[ix +(iy *inor_sizex)];
		}
	}

	delete [] pblock_norm;
	delete [] pblock_low;
	delete [] pnormal_data;
}

//static void
Void MeanUpSampling(PixelI *pSrcPic, PixelI *pDestPic,
					Int iwidth, Int iheight )
{
    Int	i, j, ix0, iy0, ix1, iy1, ix2, iy2;
    Int	ival;
    for(j = 0; j < iheight; j++)
	{
        for(i = 0; i < iwidth; i++)
		{
            ix0	= i;
            iy0 = j;
            ix1	= RRV_MAX(0, (i - 1));
            iy1	= RRV_MAX(0, (j - 1));
            ix2	= RRV_MIN((iwidth - 1), (i + 1));
            iy2	= RRV_MIN((iheight - 1), (j + 1));

// RRV_3
            if(i==0 && j==0) {
              pDestPic[i * 2 + j * 2 * iwidth * 2] = pSrcPic[ix0 + iy0 * iwidth];
            } else {
// ~RRV_3
            ival = 9 * pSrcPic[ix0 + iy0 * iwidth] +
                   3 * pSrcPic[ix1 + iy0 * iwidth] +
                   3 * pSrcPic[ix0 + iy1 * iwidth] +
                   1 * pSrcPic[ix1 + iy1 * iwidth];
            pDestPic[i * 2 + j * 2 * iwidth * 2] = (ival + 8) / 16;
// RRV_3
	        }
// ~RRV_3

// RRV_3
            if(i==(iwidth-1) && j==0) {
              pDestPic[i * 2 + 1 + j * 2 * iwidth * 2] = pSrcPic[ix0 + iy0 * iwidth];
            } else {
// ~RRV_3
            ival = 9 * pSrcPic[ix0 + iy0 * iwidth] +
                   3 * pSrcPic[ix2 + iy0 * iwidth] +
                   3 * pSrcPic[ix0 + iy1 * iwidth] +
                   1 * pSrcPic[ix2 + iy1 * iwidth];
            pDestPic[i * 2 + 1 + j * 2 * iwidth * 2] = (ival + 8) / 16;
// RRV_3
	        }
// ~RRV_3

// RRV_3
            if(i==0 && j==(iheight-1)) {
              pDestPic[i * 2  + (j * 2 + 1) * iwidth * 2] = pSrcPic[ix0 + iy0 * iwidth];
            } else {
// ~RRV_3
            ival = 9 * pSrcPic[ix0 + iy0 * iwidth] +
                   3 * pSrcPic[ix1 + iy0 * iwidth] +
                   3 * pSrcPic[ix0 + iy2 * iwidth] +
                   1 * pSrcPic[ix1 + iy2 * iwidth];
            pDestPic[i * 2  + (j * 2 + 1) * iwidth * 2] = (ival + 8) / 16;
// RRV_3
	        }
// ~RRV_3

// RRV_3
            if(i==(iwidth-1) && j==(iheight-1)) {
              pDestPic[i * 2 + 1 + (j * 2 + 1)* iwidth * 2] = pSrcPic[ix0 + iy0 * iwidth];
            } else {
// ~RRV_3
            ival = 9 * pSrcPic[ix0 + iy0 * iwidth] +
                   3 * pSrcPic[ix2 + iy0 * iwidth] +
                   3 * pSrcPic[ix0 + iy2 * iwidth] +
                   1 * pSrcPic[ix2 + iy2 * iwidth];
            pDestPic[i * 2 + 1 + (j * 2 + 1)* iwidth * 2] = (ival + 8) / 16;
// RRV_3
	        }
// ~RRV_3
        }
    }
}

//static void
void MeanUpSampling(PixelC *pSrcPic, PixelC *pDestPic,
					Int iwidth, Int iheight )
{
    Int	i, j, ix0, iy0, ix1, iy1, ix2, iy2;
    Int	ival;
    for(j = 0; j < iheight; j++)
	{
        for(i = 0; i < iwidth; i++)
		{
            ix0 = i;
            iy0 = j;
            ix1 = RRV_MAX(0, (i - 1));
            iy1 = RRV_MAX(0, (j - 1));
            ix2 = RRV_MIN((iwidth - 1), (i + 1));
            iy2 = RRV_MIN((iheight - 1), (j + 1));
// RRV_3
            if(i==0 && j==0) {
              pDestPic[i * 2 + j * 2 * iwidth * 2] = pSrcPic[ix0 + iy0 * iwidth];
            } else {
// ~RRV_3
            ival = 9 * pSrcPic[ix0 + iy0 * iwidth] +
                   3 * pSrcPic[ix1 + iy0 * iwidth] +
                   3 * pSrcPic[ix0 + iy1 * iwidth] +
                   1 * pSrcPic[ix1 + iy1 * iwidth];
            pDestPic[i * 2 + j * 2 * iwidth * 2] = (ival + 8) / 16;
// RRV_3
	        }
// ~RRV_3

// RRV_3
            if(i==(iwidth-1) && j==0) {
              pDestPic[i * 2 + 1 + j * 2 * iwidth * 2] = pSrcPic[ix0 + iy0 * iwidth];
            } else {
// ~RRV_3
            ival = 9 * pSrcPic[ix0 + iy0 * iwidth] +
                   3 * pSrcPic[ix2 + iy0 * iwidth] +
                   3 * pSrcPic[ix0 + iy1 * iwidth] +
                   1 * pSrcPic[ix2 + iy1 * iwidth];
            pDestPic[i * 2 + 1 + j * 2 * iwidth * 2] = (ival + 8) / 16;
// RRV_3
	        }
// ~RRV_3

// RRV_3
            if(i==0 && j==(iheight-1)) {
              pDestPic[i * 2  + (j * 2 + 1) * iwidth * 2] = pSrcPic[ix0 + iy0 * iwidth];
            } else {
// ~RRV_3
            ival = 9 * pSrcPic[ix0 + iy0 * iwidth] +
                   3 * pSrcPic[ix1 + iy0 * iwidth] +
                   3 * pSrcPic[ix0 + iy2 * iwidth] +
                   1 * pSrcPic[ix1 + iy2 * iwidth];
            pDestPic[i * 2  + (j * 2 + 1) * iwidth * 2] = (ival + 8) / 16;
// RRV_3
	        }
// ~RRV_3

// RRV_3
            if(i==(iwidth-1) && j==(iheight-1)) {
              pDestPic[i * 2 + 1 + (j * 2 + 1)* iwidth * 2] = pSrcPic[ix0 + iy0 * iwidth];
            } else {
// ~RRV_3
            ival = 9 * pSrcPic[ix0 + iy0 * iwidth] +
                   3 * pSrcPic[ix2 + iy0 * iwidth] +
                   3 * pSrcPic[ix0 + iy2 * iwidth] +
                   1 * pSrcPic[ix2 + iy2 * iwidth];
            pDestPic[i * 2 + 1 + (j * 2 + 1)* iwidth * 2] = (ival + 8) / 16;
// RRV_3
	        }
// ~RRV_3
        }
    }
}

//
//
//FilterCodedPictureforRRV
//
//
Void CVideoObject::filterCodedPictureForRRV(PixelC *pPxlcY,
											 PixelC *pPxlcU, PixelC *pPxlcV, 
											 Int isizex, Int isizey,
											 Int inumMBx,Int inumMBy, 
											 Int iwidthY, Int iwidthUV)
{
//	PixelC	pdataY[isizex *isizey];
//	PixelC	pdataU[isizex *isizey /4];
//	PixelC	pdataV[isizex *isizey /4];
	PixelC	*pdataY	= new PixelC[isizex *isizey];
	PixelC	*pdataU	= new PixelC[isizex *isizey /4];
	PixelC	*pdataV	= new PixelC[isizex *isizey /4];
	Int ipMB,ipPreMB;
	Int ixcount,iycount; 
	Int icurMB	= 0;
	Int i, j;
// RRV_2 insertion
	Int	iVpEdgeEnable;
	iVpEdgeEnable	= (!m_volmd.bNewpredEnable)||(m_volmd.bNewpredSegmentType);

// ~RRV_2
// copy data, data -> pPxlc
	for(j = 0; j < isizey; j++)
	{
		for(i = 0; i < isizex; i++)
		{
			pdataY[i+ j* isizex] = pPxlcY[i+ j* iwidthY];
		}
	}
	for(j = 0; j < isizey /2; j++)
	{
		for(i = 0; i < isizex /2; i++){
			pdataU[i+ j *isizex /2]	= pPxlcU[i +j *iwidthUV];
			pdataV[i+ j* isizex /2]	= pPxlcV[i +j *iwidthUV];
		}
	}

// check cod-frag
/*
	printf("Checking cod flag numMBy = %d numMBx = %d\n", inumMBy, inumMBx);
	for(i = 0; i < inumMBy; i++)
	{
		for(j = 0; j < inumMBx; j++)
		{
			if(m_rgmbmd[i *inumMBx +j].m_bSkip == TRUE)
			{
				printf("T ");
			}
			else if(m_rgmbmd[i *inumMBx +j].m_bSkip == FALSE)
			{
				printf("F ");
			}
			else
			{
				printf("E ");
			}
		}
		printf("\n");
	}
*/
// --- Filtering Macro-Block Borderline (Lum part) ---
//    printf("Filtering Macro-Block Horizontal Borderline (Lum)\n");
    for(iycount = 1; iycount < (inumMBy * 2); iycount++)
	{
        icurMB	= (iycount / 2) * inumMBx;
        for(ixcount=0; ixcount<(inumMBx * 2); ixcount+=2, icurMB++)
		{
            ipMB	= icurMB;
            if(iycount % 2)
			{
                ipPreMB	= ipMB;
            }
            else
			{
                ipPreMB	= (ipMB - inumMBx);
            }
// RRV_2 modification
            if(((m_rgmbmd[ipMB].m_bSkip == FALSE)||
			   (m_rgmbmd[ipPreMB].m_bSkip == FALSE))&&
	       ((m_rgmbmd[ipMB].m_iVideoPacketNumber == m_rgmbmd[ipPreMB].m_iVideoPacketNumber)||(iVpEdgeEnable == 1)))
			{
//            if((m_rgmbmd[ipMB].m_bSkip == FALSE)||
//			   (m_rgmbmd[ipPreMB].m_bSkip == FALSE))
//			{
// ~RRV_2
                filterMBHorBorder(pdataY, isizex, ixcount, iycount);
                filterMBHorBorder(pdataY, isizex, (ixcount+1), iycount);
            }
        }
    }
//    printf("Done\n");

//    printf("Filtering Macro-Block Vartial Borderline (Lum)\n");
    for(iycount=0; iycount<(inumMBy * 2); iycount+=2)
	{
        icurMB	= (iycount / 2) * inumMBx;
        for(ixcount = 1; ixcount < (inumMBx * 2); ixcount++)
		{
            ipMB	= icurMB + (ixcount / 2);
            if(ixcount % 2)
			{
                ipPreMB	= ipMB;
            }
            else
			{
                ipPreMB	= ipMB - 1;
            }
// RRV_2 modification
            if(((m_rgmbmd[ipMB].m_bSkip == FALSE)||
			   (m_rgmbmd[ipPreMB].m_bSkip == FALSE))&&
	       ((m_rgmbmd[ipMB].m_iVideoPacketNumber == m_rgmbmd[ipPreMB].m_iVideoPacketNumber)||(iVpEdgeEnable == 1)))
			{
//            if((m_rgmbmd[ipMB].m_bSkip == FALSE)||
//			   (m_rgmbmd[ipPreMB].m_bSkip == FALSE))
//			{
// ~RRV_2
                filterMBVarBorder(pdataY, isizex, ixcount, iycount);
                filterMBVarBorder(pdataY, isizex, ixcount, (iycount+1));
            }
        }
    }
//    printf("Done\n");

//--- Filtering Macro-Block Borderline (Cb & Cr part) ---
//    printf("Filtering Macro-Block Horizontal Borderline (Cb & Cr)\n");
    for(iycount = 1; iycount < inumMBy; iycount++)
	{
        icurMB	= iycount * inumMBx;
        for(ixcount = 0; ixcount < inumMBx; ixcount++, icurMB++)
		{
            ipMB	= icurMB;
            ipPreMB	= ipMB - inumMBx;
// RRV_2 modification
            if(((m_rgmbmd[ipMB].m_bSkip == FALSE)||
			   (m_rgmbmd[ipPreMB].m_bSkip == FALSE))&&
	       ((m_rgmbmd[ipMB].m_iVideoPacketNumber == m_rgmbmd[ipPreMB].m_iVideoPacketNumber)||(iVpEdgeEnable == 1)))
			{
//            if((m_rgmbmd[ipMB].m_bSkip == FALSE)||
//			   (m_rgmbmd[ipPreMB].m_bSkip == FALSE))
//			{
// ~RRV_2
                filterMBHorBorder(pdataU, isizex/2, ixcount, iycount);
                filterMBHorBorder(pdataV, isizex/2, ixcount, iycount);
            }
        }
    }
//    printf("Done\n");

//    printf("Filtering Macro-Block Vartial Borderline (Cb & Cr)\n");
    for(iycount = 0; iycount < inumMBy; iycount++)
	{
        icurMB	= iycount * inumMBx;
        for(ixcount = 1; ixcount < inumMBx; ixcount++)
		{
            icurMB++;
            ipMB	= icurMB;
            ipPreMB	= ipMB - 1;
// RRV_2 modification
            if(((m_rgmbmd[ipMB].m_bSkip == FALSE)||
			   (m_rgmbmd[ipPreMB].m_bSkip == FALSE))&&
	       ((m_rgmbmd[ipMB].m_iVideoPacketNumber == m_rgmbmd[ipPreMB].m_iVideoPacketNumber)||(iVpEdgeEnable == 1)))
			{
//            if((m_rgmbmd[ipMB].m_bSkip == FALSE)||
//			   (m_rgmbmd[ipPreMB].m_bSkip == FALSE))
//			{
// ~RRV_2
                filterMBVarBorder(pdataU, isizex/2, ixcount, iycount);
                filterMBVarBorder(pdataV, isizex/2, ixcount, iycount);
            }
        }
    }
//    printf("Done\n");

// copy data, pPxlc -> data
	for(j = 0; j < isizey; j++)
	{
		for(i = 0; i < isizex; i++)
		{
			pPxlcY[i +j *iwidthY]	= pdataY[i +j *isizex];
		}
	}
	for(j = 0; j < isizey /2; j++)
	{
		for(i = 0; i < isizex /2; i++)
		{
			pPxlcU[i +j *iwidthUV]	= pdataU[i +j* isizex /2];
			pPxlcV[i +j *iwidthUV]	= pdataV[i +j* isizex /2];
		}
	}
}

Void filterMBVarBorder(PixelC *pPicTop, Int isizex, Int icurX, Int icurY)
{
    PixelC	*pPixel1, *pPixel2;
    Int	ival1, ival2;
    Int	i;

    icurX	*= RRV_BLOCK_SIZE;
    icurY	*= RRV_BLOCK_SIZE;

//--- Read Cb Data ---
    pPixel1	= pPicTop + ((icurX - 1) + icurY * isizex);
    pPixel2	= pPicTop + ( icurX      + icurY * isizex);

    for(i = 0; i < RRV_BLOCK_SIZE; i++)
	{
        ival1	= *pPixel1;
        ival2	= *pPixel2;

        *pPixel1	= (3 * ival1 + ival2 + 2) / 4;
        *pPixel2	= (ival1 + 3 * ival2 + 2) / 4;

        pPixel1	+= isizex;
        pPixel2	+= isizex;
    }
}

Void filterMBHorBorder(PixelC *pPicTop, Int isizex, Int icurX, Int icurY)
{
    PixelC	*pPixel1, *pPixel2;
    Int	ival1, ival2;
    Int	i;

    icurX	*= RRV_BLOCK_SIZE;
    icurY	*= RRV_BLOCK_SIZE;

//--- Read Data ---
    pPixel1	= pPicTop + (icurX + (icurY - 1) * isizex);
    pPixel2	= pPicTop + (icurX +  icurY      * isizex);

    for(i = 0; i < RRV_BLOCK_SIZE; i++)
	{
        ival1	= *pPixel1;
        ival2	= *pPixel2;

        *pPixel1	= (3 * ival1 + ival2 + 2) / 4;
        *pPixel2	= (ival1 + 3 * ival2 + 2) / 4;

        pPixel1++;
        pPixel2++;
    }
}

Void writeCubicRct(Int xsize, Int line_interval, PixelC* src, PixelC* dst)
{
	PixelC	*p0, *p1;
	p0	= src;
	p1	= dst;
	for(int y = 0; y < xsize; y ++)
	{
		for(int x = 0; x < xsize; x ++)
		{
			(*p1 ++)	= (*p0 ++);
		}
		p1	+= (line_interval -xsize);
	}
}

Void writeCubicRct(Int xsize, Int line_interval, PixelI* src, PixelI* dst)
{
	PixelI	*p0, *p1;
	p0	= src;
	p1	= dst;
	for(int y = 0; y < xsize; y ++)
	{
		for(int x = 0; x < xsize; x ++)
		{
			(*p1 ++)	= (*p0 ++);
		}
		p1	+= (line_interval -xsize);
	}
}


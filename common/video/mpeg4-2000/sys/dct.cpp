/*************************************************************************

This software module was originally developed by 

	Ming-Chieh Lee (mingcl@microsoft.com), Microsoft Corporation
	Wei-ge Chen (wchen@microsoft.com), Microsoft Corporation
	Bruce Lin (blin@microsoft.com), Microsoft Corporation
	Chuang Gu (chuanggu@microsoft.com), Microsoft Corporation
	(date: March, 1996)
and edited by
	Wei Wu (weiwu@stallion.risc.rockwell.com) Rockwell Science Center
and also edited by
    Mathias Wien (wien@ient.rwth-aachen.de) RWTH Aachen / Robert BOSCH GmbH

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

	vopSes.cpp

Abstract:

	Base class for the encoder for one VOP session.

Revision History:
    

*************************************************************************/

#include <stdio.h>

#include "typeapi.h"
#include "dct.hpp"

#ifdef __MFC_
#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

#define new DEBUG_NEW				   
#endif // __MFC_

CBlockDCT::~CBlockDCT ()
{
/* NBIT: change
	m_rgchClipTbl -= 512;
*/
	m_rgchClipTbl -= (1<<(m_nBits+1));
	delete [] m_rgchClipTbl;
}

/* NBIT
CBlockDCT::CBlockDCT ()
*/
CBlockDCT::CBlockDCT (UInt nBits) : m_nBits(nBits)
{
	Int ClipTblSize = 1<<(m_nBits+2); // NBIT
	Int offset = ClipTblSize/2; // NBIT
	Int maxVal = (1<<m_nBits)-1; // NBIT

/* NBIT: change
	m_rgchClipTbl = new PixelC [1024];
	m_rgchClipTbl += 512;
	Int i;
	for (i = -512; i < 512; i++)	{
		if (i < 0)
			m_rgchClipTbl [i] = 0;
		else if (i >= 0 && i < 256)
			m_rgchClipTbl [i] = i;
		else 
			m_rgchClipTbl [i] = 255;
	}
*/
	m_rgchClipTbl = new PixelC [ClipTblSize];
	m_rgchClipTbl += offset;
	Int i;
	for (i = -offset; i < offset; i++)	{
		if (i < 0)
			m_rgchClipTbl [i] = 0;
		else if (i >= 0 && i <= maxVal)
			m_rgchClipTbl [i] = i;
		else 
			m_rgchClipTbl [i] = maxVal;
	}

	/*	m_c0 = 0.7071068F; //mwi
	m_c1 = 0.4903926F;
	m_c2 = 0.4619398F;
	m_c3 = 0.4157348F;
	m_c4 = 0.3535534F;
	m_c5 = 0.2777851F;
	m_c6 = 0.1913417F;
	m_c7 = 0.0975452F; */
	m_c0 = 0.7071068;
	m_c1 = 0.4903926;
	m_c2 = 0.4619398;
	m_c3 = 0.4157348;
	m_c4 = 0.3535534;
	m_c5 = 0.2777851;
	m_c6 = 0.1913417;
	m_c7 = 0.0975452; 
}

Void CBlockDCT::apply (const PixelC* rgchSrc, Int nColSrc, Int* rgiDst, Int nColDst)
{
	// transform here
	const PixelC* pchRowSrc = rgchSrc;
	for (CoordI iRow = 0; iRow < BLOCK_SIZE; iRow++)	{
		xformRow (pchRowSrc, iRow);
		pchRowSrc += nColSrc;
	}

	Int* piColDst = rgiDst;
	for (CoordI iCol = 0; iCol < BLOCK_SIZE; iCol++)	{
		xformColumn (piColDst, iCol, nColDst);
		piColDst++;
	}
}

Void CBlockDCT::apply (const Int* rgiSrc, Int nColSrc, PixelC* rgchDst, Int nColDst)
{
	// transform here
	const PixelI* piRowSrc = rgiSrc;
	for (CoordI iRow = 0; iRow < BLOCK_SIZE; iRow++)	{
		xformRow (piRowSrc, iRow);
		piRowSrc += nColSrc;
	}

	PixelC* pchColDst = rgchDst;
	for (CoordI iCol = 0; iCol < BLOCK_SIZE; iCol++)	{
		xformColumn (pchColDst, iCol, nColDst);
		pchColDst++;
	}
}

Void CBlockDCT::apply (const Int* rgiSrc, Int nColSrc, Int* rgiDst, Int nColDst)
{
	// transform here
	const PixelI* piRowSrc = rgiSrc;
	for (CoordI iRow = 0; iRow < BLOCK_SIZE; iRow++)	{
		xformRow (piRowSrc, iRow);
		piRowSrc += nColSrc;
	}

	Int* piColDst = rgiDst;
	for (CoordI iCol = 0; iCol < BLOCK_SIZE; iCol++)	{
		xformColumn (piColDst, iCol, nColDst);
		piColDst++;
	}
}

Void CBlockDCT::xformRow (const PixelC* ppxlcRowSrc, CoordI i)
{
	UInt j;
	for (j = 0; j < BLOCK_SIZE; j++) 
		m_rgfltBuf1 [j] = (Float) *ppxlcRowSrc++;

	oneDimensionalDCT ();

	for (j = 0; j < BLOCK_SIZE; j++) 
		m_rgfltAfterRowXform [i] [j] = m_rgfltAfter1dXform [j];
}

Void CBlockDCT::xformRow (const PixelI* ppxlfRowSrc, CoordI i)
{
	UInt j;
	for (j = 0; j < BLOCK_SIZE; j++) 
		m_rgfltBuf1 [j] = (Float) *ppxlfRowSrc++;

	oneDimensionalDCT ();

	for (j = 0; j < BLOCK_SIZE; j++) 
		m_rgfltAfterRowXform [i] [j] = m_rgfltAfter1dXform [j];
}

Void CBlockDCT::xformColumn (PixelC* ppxlcColDst, CoordI i, Int nColDst)
{
	UInt j;
	for (j = 0; j < BLOCK_SIZE; j++)	
		m_rgfltBuf1[j] = m_rgfltAfterRowXform[j][i];
	
	oneDimensionalDCT ();

	for (j = 0; j < BLOCK_SIZE; j++)	{ 
		Int iValue = 
				(m_rgfltAfter1dXform[j] >= 0) ? (PixelI) (m_rgfltAfter1dXform[j] + .5) :
											(PixelI) (m_rgfltAfter1dXform[j] - .5);
		*ppxlcColDst = m_rgchClipTbl [iValue];
		ppxlcColDst += nColDst;
	}
}

Void CBlockDCT::xformColumn (PixelI* ppxliColDst, CoordI i, Int nColDst)
{
	UInt j;
	for (j = 0; j < BLOCK_SIZE; j++)	
		m_rgfltBuf1[j] = m_rgfltAfterRowXform[j][i];

	oneDimensionalDCT ();

	for (j = 0; j < BLOCK_SIZE; j++)	{ 
		PixelI vl = 
			(m_rgfltAfter1dXform[j] >= 0) ? (PixelI) (m_rgfltAfter1dXform[j] + .5) :
											(PixelI) (m_rgfltAfter1dXform[j] - .5);
		*ppxliColDst = vl;
		ppxliColDst += nColDst;
	}
}

/* NBIT: change
CFwdBlockDCT::CFwdBlockDCT () : CBlockDCT ()
*/
CFwdBlockDCT::CFwdBlockDCT (UInt nBits) : CBlockDCT (nBits)
{
}

Void CFwdBlockDCT::oneDimensionalDCT ()
{
	Int j, j1;
	for (j = 0; j < 4; j++) {
		j1 = 7 - j;
		m_rgfltBuf2[j] = m_rgfltBuf1[j] + m_rgfltBuf1[j1];
		m_rgfltBuf2[j1] = m_rgfltBuf1[j] - m_rgfltBuf1[j1];
	}

	m_rgfltBuf1[0] = m_rgfltBuf2[0] + m_rgfltBuf2[3];
	m_rgfltBuf1[1] = m_rgfltBuf2[1] + m_rgfltBuf2[2];
	m_rgfltBuf1[2] = m_rgfltBuf2[1] - m_rgfltBuf2[2];
	m_rgfltBuf1[3] = m_rgfltBuf2[0] - m_rgfltBuf2[3];
	m_rgfltBuf1[4] = m_rgfltBuf2[4];
	m_rgfltBuf1[5] = (m_rgfltBuf2[6] - m_rgfltBuf2[5]) * m_c0;
	m_rgfltBuf1[6] = (m_rgfltBuf2[6] + m_rgfltBuf2[5]) * m_c0;
	m_rgfltBuf1[7] = m_rgfltBuf2[7];
	m_rgfltAfter1dXform[0] = (m_rgfltBuf1[0] + m_rgfltBuf1[1]) * m_c4;
	m_rgfltAfter1dXform[4] = (m_rgfltBuf1[0] - m_rgfltBuf1[1]) * m_c4;
	m_rgfltAfter1dXform[2] = m_rgfltBuf1[2] * m_c6 + m_rgfltBuf1[3] * m_c2;
	m_rgfltAfter1dXform[6] = m_rgfltBuf1[3] * m_c6 - m_rgfltBuf1[2] * m_c2;

	m_rgfltBuf2[4] = m_rgfltBuf1[4] + m_rgfltBuf1[5];
	m_rgfltBuf2[7] = m_rgfltBuf1[7] + m_rgfltBuf1[6];
		m_rgfltBuf2[5] = m_rgfltBuf1[4] - m_rgfltBuf1[5];
	m_rgfltBuf2[6] = m_rgfltBuf1[7] - m_rgfltBuf1[6];
	m_rgfltAfter1dXform[1] = m_rgfltBuf2[4] * m_c7 + m_rgfltBuf2[7] * m_c1;
	m_rgfltAfter1dXform[5] = m_rgfltBuf2[5] * m_c3 + m_rgfltBuf2[6] * m_c5;
	m_rgfltAfter1dXform[7] = m_rgfltBuf2[7] * m_c7 - m_rgfltBuf2[4] * m_c1;
	m_rgfltAfter1dXform[3] = m_rgfltBuf2[6] * m_c3 - m_rgfltBuf2[5] * m_c5;
}

/* NBIT: change
CInvBlockDCT::CInvBlockDCT () : CBlockDCT ()
*/
CInvBlockDCT::CInvBlockDCT (UInt nBits) : CBlockDCT (nBits)
{
}

Void CInvBlockDCT::oneDimensionalDCT ()
{
  /*	Float flt1 = m_rgfltBuf1[1] * m_c7 - m_rgfltBuf1[7] * m_c1; // Double, because m_rgfltBuf1,2 are doubles, mwi
	Float flt2 = m_rgfltBuf1[7] * m_c7 + m_rgfltBuf1[1] * m_c1;
	Float flt3 = m_rgfltBuf1[5] * m_c3 - m_rgfltBuf1[3] * m_c5;
	Float flt4 = m_rgfltBuf1[3] * m_c3 + m_rgfltBuf1[5] * m_c5; */
	Double flt1 = m_rgfltBuf1[1] * m_c7 - m_rgfltBuf1[7] * m_c1;
	Double flt2 = m_rgfltBuf1[7] * m_c7 + m_rgfltBuf1[1] * m_c1;
	Double flt3 = m_rgfltBuf1[5] * m_c3 - m_rgfltBuf1[3] * m_c5;
	Double flt4 = m_rgfltBuf1[3] * m_c3 + m_rgfltBuf1[5] * m_c5;

	m_rgfltBuf2[0] = (m_rgfltBuf1[0] + m_rgfltBuf1[4]) * m_c4;
	m_rgfltBuf2[1] = (m_rgfltBuf1[0] - m_rgfltBuf1[4]) * m_c4;
	m_rgfltBuf2[2] = m_rgfltBuf1[2] * m_c6 - m_rgfltBuf1[6] * m_c2;
	m_rgfltBuf2[3] = m_rgfltBuf1[6] * m_c6 + m_rgfltBuf1[2] * m_c2;
	m_rgfltBuf1[4] = flt1 + flt3;
	m_rgfltBuf2[5] = flt1 - flt3;
	m_rgfltBuf2[6] = flt2 - flt4;
	m_rgfltBuf1[7] = flt2 + flt4;
    
	m_rgfltBuf1[5] = (m_rgfltBuf2[6] - m_rgfltBuf2[5]) * m_c0;
	m_rgfltBuf1[6] = (m_rgfltBuf2[6] + m_rgfltBuf2[5]) * m_c0;
	m_rgfltBuf1[0] = m_rgfltBuf2[0] + m_rgfltBuf2[3];
	m_rgfltBuf1[1] = m_rgfltBuf2[1] + m_rgfltBuf2[2];
	m_rgfltBuf1[2] = m_rgfltBuf2[1] - m_rgfltBuf2[2];
	m_rgfltBuf1[3] = m_rgfltBuf2[0] - m_rgfltBuf2[3];

	Int j, j1;
	for (j = 0; j < 4; j++) {
		j1 = 7 - j;
		m_rgfltAfter1dXform[j] = m_rgfltBuf1[j] + m_rgfltBuf1[j1];
		m_rgfltAfter1dXform[j1] = m_rgfltBuf1[j] - m_rgfltBuf1[j1];
	}
}

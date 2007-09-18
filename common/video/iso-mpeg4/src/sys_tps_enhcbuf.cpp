/*************************************************************************

This software module was originally developed by 

        Norio Ito (norio@imgsl.mkhar.sharp.co.jp), Sharp Corporation
        Hiroyuki Katata (katata@imgsl.mkhar.sharp.co.jp), Sharp Corporation
        Shuichi Watanabe (watanabe@imgsl.mkhar.sharp.co.jp), Sharp Corporation
        (date: October, 1997)

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
Sharp retains full right to use the code for his/her own purpose, 
assign or donate the code to a third party and to inhibit third parties from using the code for non <MPEG standard> conforming products. 
This copyright notice must be included in all copies or derivative works. 

Copyright (c) 1997.

Module Name:

	tps_enhcbuf.cpp

Abstract:

	buffers for enhancement layer for temporal scalability

Revision History:

*************************************************************************/

#include <stdio.h>
#include <fstream>
#include <stdlib.h>

#include "typeapi.h"
#include "codehead.h"
#include "global.hpp"
#include "mode.hpp"
#include "vopses.hpp"
#include "tps_enhcbuf.hpp"

CEnhcBuffer::CEnhcBuffer(Int iSessionWidth, Int iSessionHeight)
{
	Int iNumMBX = iSessionWidth / MB_SIZE;
	Int iNumMBY = iSessionHeight / MB_SIZE;
	if ( iSessionWidth % MB_SIZE != 0 )
		iNumMBX++;
	if ( iSessionHeight % MB_SIZE != 0 )
		iNumMBY++;
	Int iNumMB = iNumMBX*iNumMBY;

//	m_rgmbmd = new CMBMode [iNumMB];
	m_rgmbmdRef = new CMBMode [iNumMB];
//	m_rgmv = new CMotionVector [iNumMB * 5];
	// TPS FIX
	m_rgmvRef = new CMotionVector [iNumMB * max(PVOP_MV_PER_REF_PER_MB, 2*BVOP_MV_PER_REF_PER_MB)];

//	m_iNumMB = 0;
//	m_iNumMBX = 0;
//	m_iNumMBY = 0;
	m_iNumMBRef = 0;
	m_iNumMBXRef = 0;
	m_iNumMBYRef = 0;

	m_iOffsetForPadY = 0;
	m_iOffsetForPadUV = 0;
	m_rctPrevNoExpandY = CRct(0,0,0,0);
	m_rctPrevNoExpandUV = CRct(0,0,0,0);

	m_rctRefVOPY0 = CRct(0,0,0,0);
	m_rctRefVOPUV0 = CRct(0,0,0,0);
	m_rctRefVOPY1 = CRct(0,0,0,0);
	m_rctRefVOPUV1 = CRct(0,0,0,0);
	m_rctRefVOPZoom = CRct(0,0,0,0); // 9/26 by Norio 

	m_puciZoomBuf = NULL; // 9/26 by Norio 
	m_pvopcBuf = NULL;
}

CEnhcBuffer::~CEnhcBuffer()
{
//	delete [] m_rgmbmd;
	delete [] m_rgmbmdRef;
//	delete [] m_rgmv;
	delete [] m_rgmvRef;

	delete m_pvopcBuf;
	delete m_puciZoomBuf; // 9/26 by Norio
}

Void CEnhcBuffer::dispose()
{
	delete m_pvopcBuf;
	delete m_puciZoomBuf; // 9/26 by Norio
	m_pvopcBuf = NULL;
	m_puciZoomBuf = NULL; // 9/26 by Norio
}

Bool CEnhcBuffer::empty()
{
	return m_pvopcBuf == NULL;
}

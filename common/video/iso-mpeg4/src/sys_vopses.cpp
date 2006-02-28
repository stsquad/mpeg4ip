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
and also edited by
	Yoshinori Suzuki (Hitachi, Ltd.)
and also edited by
	Hideaki Kimata (NTT)

and also edited by
    Fujitsu Laboratories Ltd. (contact: Eishi Morimatsu)

and also edited by 
	Takefumi Nagumo (nagumo@av.crl.sony.co.jp) Sony Corporation
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

	vopSes.cpp

Abstract:

	Base class for the encoder for one VOP session.

Revision History:
	December 20, 1997:	Interlaced tools added by NextLevel Systems (GI)
                        X.Chen (xchen@nlvl.com), B. Eifrig (beifrig@nlvl.com)
        May. 9   1998:  add boundary by Hyundai Electronics 
                                  Cheol-Soo Park (cspark@super5.hyundai.co.kr) 
        May. 9   1998:  add field based unrestricted MC padding by Hyundai Electronics 
                                  Cheol-Soo Park (cspark@super5.hyundai.co.kr) 
        Feb. 16  1999:  add Quarter Sample 
                        Mathias Wien (wien@ient.rwth-aachen.de) 
        Feb. 23  1999:  GMC added by Yoshinori Suzuki (Hitachi, Ltd.)
		Aug.24, 1999 : NEWPRED added by Hideaki Kimata (NTT) 
		Sep.06	1999 : RRV added by Eishi Morimatsu (Fujitsu Laboratories Ltd.) 
		Feb.12	2000 : Bug fix of OBSS by Takefumi Nagumo (SONY)

*************************************************************************/

#include <stdio.h>

#include "typeapi.h"
#include "basic.hpp"
#include "header.h"
#include "codehead.h"
#include "mode.hpp"
#include "dct.hpp"
#include "cae.h"
#include "vopses.hpp"

// NEWPRED
#include "global.hpp"
#include "newpred.hpp"
// ~NEWPRED

#ifdef __MFC_
#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

#define new DEBUG_NEW				   
#endif // __MFC_

CVideoObject::~CVideoObject ()
{
	delete m_pvopcCurrQ;
	delete m_pvopcRefQ0;
	delete m_pvopcRefQ1;       // modified by Shu 12/12/97
							   // wchen: unmodified 12/16/97

	delete m_pvopcCurrMB;
	delete m_pvopcPredMB;
	delete m_pvopiErrorMB;
	delete [] m_rgmbmd;
	delete [] m_rgmv;
	delete [] m_rgmvBY;
    delete [] m_rgmvRef;
	delete m_pidct;

	Int nBlk = (m_volmd.fAUsage == EIGHT_BIT) ? 6+m_volmd.iAuxCompCount*4 : 6;
	Int iBlk;
	for (iBlk = 0; iBlk < nBlk; iBlk++)
		delete [] m_rgpiCoefQ [iBlk];
	delete [] m_rgpiCoefQ;
//	delete m_pvopfCurrFilteredQ;
//	delete m_pdrtmdRef1;

	delete [] m_rgiQPpred;

	// HHI Schueuer: sadct
	if (m_rgiCurrMBCoeffWidth) {
		for (int i=Y_BLOCK1; i<=U_BLOCK; i++) 
			delete [] m_rgiCurrMBCoeffWidth[i];
		delete [] m_rgiCurrMBCoeffWidth;
	}	
	// end HHI


	if (m_volmd.fAUsage == RECTANGLE) {
		Int nBlk = (m_volmd.fAUsage == EIGHT_BIT) ? 6+m_volmd.iAuxCompCount*4 : 6;
		delete [] m_rgblkmCurrMB;
        if (m_rgpmbmAbove != NULL)
		    for (Int iMB = 0; iMB < m_iNumMBX; iMB++)	{
			    for (iBlk = 0; iBlk < nBlk; iBlk++)	{
				    delete [] (m_rgpmbmAbove [iMB]->rgblkm) [iBlk];
				    delete [] (m_rgpmbmCurr  [iMB]->rgblkm) [iBlk];
			    }
			    delete [] m_rgpmbmAbove [iMB]->rgblkm;
			    delete m_rgpmbmAbove [iMB];
			    delete [] m_rgpmbmCurr [iMB]->rgblkm;
			    delete m_rgpmbmCurr [iMB];
		    }
		delete [] m_rgpmbmAbove;
		delete [] m_rgpmbmCurr;
	}

	// sprite
	if (m_uiSprite == 1 || m_uiSprite == 2) { // GMC
		delete [] m_rgstSrcQ;
		delete [] m_rgstDstQ;
		if (m_sptMode == BASIC_SPRITE)
			delete m_pvopcSptQ;
	}
	delete m_pbEmptyRowArray;

/* NBIT: change
	m_rgiClipTab -= 400;
*/
	m_rgiClipTab -= m_iOffset;
	delete [] m_rgiClipTab;
	m_rgiClipTab = NULL;

	// shape
//OBSS_SAIT_991015
	if(m_rgshpmd!=NULL)
		delete m_rgshpmd;
	if(m_rgBaseshpmd!=NULL && m_volmd.volType == BASE_LAYER )
		delete m_rgBaseshpmd;								
//~OBSS_SAIT_991015

	delete m_puciPredBAB;
	delete m_parcodec;
	delete [] m_ppxlcCurrMBBYDown4;
	delete [] m_ppxlcCurrMBBYDown2;
	delete [] m_ppxlcReconCurrBAB;
	delete [] m_ppxlcPredBABDown2;
	delete [] m_ppxlcPredBABDown4;

// MAC
  delete [] m_ppxlcPredMBA;
  delete [] m_ppxliErrorMBA;
  delete [] m_ppxlcPredMBBackA;
  delete [] m_ppxlcCurrMBA;
 
  //! Added by Danijel Kopcinovic, as a fix for memory leak.
  if (m_pvopcPredMBBack != NULL) delete m_pvopcPredMBBack;
   if (m_rgmvBaseBY != NULL) delete m_rgmvBaseBY;
   if (m_rgmbmdRef != NULL) delete[] m_rgmbmdRef;
   //! Added by Danijel Kopcinovic, as a fix for memory leak.
}

CVideoObject::CVideoObject ()
{
	m_t =0; m_iBCount =0; m_tPastRef=0; m_tFutureRef=0;
	m_bCodedFutureRef=TRUE;
	m_rgmbmd =NULL; m_rgmv =NULL; m_rgmvBackward =NULL; m_rgmvBY =NULL;
	m_rgmbmdRef =NULL; m_rgmvRef =NULL;
	m_tModuloBaseDecd =0; m_tModuloBaseDisp =0;
	m_pvopcCurrQ =NULL; 
	m_pvopcRefQ0 =NULL; m_pvopcRefQ1 =NULL; 
	m_puciPredBAB =NULL; m_ppxlcReconCurrBAB =NULL; m_parcodec =NULL;
	m_ppxlcCurrMBBYDown4 =NULL;
	m_ppxlcCurrMBBYDown2 =NULL;
	m_ppxlcPredBABDown4 =NULL;
	m_ppxlcPredBABDown2 =NULL;
	m_pvopcSptQ =NULL; m_uiSprite =0; m_rgstSrcQ =NULL; m_rgstDstQ =NULL;
	m_iOffsetForPadY =0; m_iOffsetForPadUV =0;
	m_rgshpmd=NULL;
	// HHI Schueuer: sadct
	m_rgiCurrMBCoeffWidth=NULL;
	// end HHI
//OBSS_SAIT_991015
	 m_rgmvBaseBY=NULL;
	m_rgBaseshpmd=NULL;
//~OBSS_SAIT_991015

	m_pbEmptyRowArray = new Bool [MB_SIZE];

/* NBIT: replaced by setClipTab() function
	m_rgiClipTab = new U8 [1024];
	m_rgiClipTab += 400;
	Int i;
	for (i = -400; i < 624; i++)
		m_rgiClipTab [i] = (i < 0) ? 0 : (i > 255) ? 255 : i;
*/

// RRV insertion
	m_iRRVScale	= 1;	// default 
	
	//! Added by Danijel Kopcinovic, as a fix for memory leak.
	m_pvopcPredMBBack = NULL;

// ~RRV
}

// NBIT: added function
Void CVideoObject::setClipTab()
{
	Int TabSize = 1<<(m_volmd.nBits+2);
	Int maxVal = (1<<m_volmd.nBits)-1;
	m_iOffset = TabSize/2;
	m_rgiClipTab = new PixelC [TabSize];
	m_rgiClipTab += m_iOffset;
	Int i;
	for (i = -m_iOffset; i < m_iOffset; i++)
		m_rgiClipTab [i] = (i < 0) ? 0 : (i > maxVal) ? maxVal : i;
}

Void CVideoObject::allocateVOLMembers (Int iSessionWidth, Int iSessionHeight)
{
// RRV insertion
  Int iAuxComp;
  Int	iScaleMB	= (m_volmd.breduced_resolution_vop_enable) ? (2) : (1);
// ~RRV

// RRV modification
	m_pvopcCurrMB = new CVOPU8YUVBA (m_volmd.fAUsage, CRct (0, 0, (MB_SIZE *iScaleMB), (MB_SIZE *iScaleMB)),
                                   m_volmd.iAuxCompCount );
//	m_pvopcCurrMB = new CVOPU8YUVBA (m_volmd.fAUsage, CRct (0, 0, MB_SIZE, MB_SIZE));
// ~RRV
	m_ppxlcCurrMBY = (PixelC*) m_pvopcCurrMB->pixelsY ();
	m_ppxlcCurrMBU = (PixelC*) m_pvopcCurrMB->pixelsU ();
	m_ppxlcCurrMBV = (PixelC*) m_pvopcCurrMB->pixelsV ();
	m_ppxlcCurrMBBY = (PixelC*) m_pvopcCurrMB->pixelsBY ();
	m_ppxlcCurrMBBUV = (PixelC*) m_pvopcCurrMB->pixelsBUV ();
  m_ppxlcCurrMBA = new PixelC* [m_volmd.iAuxCompCount];
  for(iAuxComp=0;iAuxComp<m_volmd.iAuxCompCount;iAuxComp++) { // MAC (SB) 26-Nov-99
    m_ppxlcCurrMBA[iAuxComp] = (PixelC*) m_pvopcCurrMB->pixelsA (iAuxComp);
  }
// RRV modification
	m_pvopcPredMB = new CVOPU8YUVBA (m_volmd.fAUsage, CRct (0, 0, (MB_SIZE *iScaleMB), (MB_SIZE *iScaleMB)),
                                   m_volmd.iAuxCompCount );
//	m_pvopcPredMB = new CVOPU8YUVBA (m_volmd.fAUsage, CRct (0, 0, MB_SIZE, MB_SIZE));
// ~RRV

	m_ppxlcPredMBY = (PixelC*) m_pvopcPredMB->pixelsY ();
	m_ppxlcPredMBU = (PixelC*) m_pvopcPredMB->pixelsU ();
	m_ppxlcPredMBV = (PixelC*) m_pvopcPredMB->pixelsV ();
  m_ppxlcPredMBA = new PixelC* [m_volmd.iAuxCompCount];
  for(iAuxComp=0;iAuxComp<m_volmd.iAuxCompCount;iAuxComp++) { // MAC (SB) 26-Nov-99
  	m_ppxlcPredMBA[iAuxComp] = (PixelC*) m_pvopcPredMB->pixelsA (iAuxComp);
  }
	
	// B-VOP MB buffer data
// RRV modification
	m_pvopcPredMBBack = new CVOPU8YUVBA (m_volmd.fAUsage, CRct (0, 0, (MB_SIZE *iScaleMB), (MB_SIZE *iScaleMB)),
                                       m_volmd.iAuxCompCount);
//	m_pvopcPredMBBack = new CVOPU8YUVBA (m_volmd.fAUsage, CRct (0, 0, MB_SIZE, MB_SIZE));
// ~RRV
	m_ppxlcPredMBBackY = (PixelC*) m_pvopcPredMBBack->pixelsY ();
	m_ppxlcPredMBBackU = (PixelC*) m_pvopcPredMBBack->pixelsU ();
	m_ppxlcPredMBBackV = (PixelC*) m_pvopcPredMBBack->pixelsV ();
  m_ppxlcPredMBBackA = new PixelC* [m_volmd.iAuxCompCount];
  for(iAuxComp=0;iAuxComp<m_volmd.iAuxCompCount;iAuxComp++) { // MAC (SB) 26-Nov-99
    m_ppxlcPredMBBackA[iAuxComp] = (PixelC*) m_pvopcPredMBBack->pixelsA (iAuxComp);
  }

// RRV modification
	m_pvopiErrorMB = new CVOPIntYUVBA (m_volmd.fAUsage, m_volmd.iAuxCompCount, CRct (0, 0, (MB_SIZE *iScaleMB), (MB_SIZE *iScaleMB)));
//	m_pvopiErrorMB = new CVOPIntYUVBA (m_volmd.fAUsage, CRct (0, 0, MB_SIZE, MB_SIZE));
// ~RRV
	m_ppxliErrorMBY = (PixelI*) m_pvopiErrorMB->getPlane (Y_PLANE)->pixels ();
	m_ppxliErrorMBU = (PixelI*) m_pvopiErrorMB->getPlane (U_PLANE)->pixels ();
	m_ppxliErrorMBV = (PixelI*) m_pvopiErrorMB->getPlane (V_PLANE)->pixels ();
  m_ppxliErrorMBA = new PixelI* [m_volmd.iAuxCompCount];
  for(iAuxComp=0;iAuxComp<m_volmd.iAuxCompCount;iAuxComp++) { // MAC (SB) 26-Nov-99
	  m_ppxliErrorMBA[iAuxComp] = (PixelI*) m_pvopiErrorMB->getPlaneA ( iAuxComp)->pixels ();
  }

	if (m_uiSprite == 0 || m_uiSprite == 2) // GMC
		m_pvopcCurrQ = new CVOPU8YUVBA (m_volmd.fAUsage, m_rctRefFrameY, m_volmd.iAuxCompCount);
	else //for sprite m_rctRefFrameY is for whole sprite, m_rctOrg is the display window 
// dshu: begin of modification
	//	m_pvopcCurrQ = new CVOPU8YUVBA (m_volmd.fAUsage, m_rctDisplayWindow);
	{
		CRct rctCurrQ = CRct (
			-EXPANDY_REF_FRAME, -EXPANDY_REF_FRAME, 
			EXPANDY_REF_FRAME + m_rctDisplayWindow.width, EXPANDY_REF_FRAME + m_rctDisplayWindow.height()
		);

		m_pvopcCurrQ = new CVOPU8YUVBA (m_volmd.fAUsage, rctCurrQ, m_volmd.iAuxCompCount);
	}
// dshu: end of modification

    assert (m_pvopcCurrQ != NULL);
	m_pvopcRefQ0 = new CVOPU8YUVBA (m_volmd.fAUsage, m_rctRefFrameY, m_volmd.iAuxCompCount);
	assert (m_pvopcRefQ0 != NULL);
	m_pvopcRefQ1 = new CVOPU8YUVBA (m_volmd.fAUsage, m_rctRefFrameY, m_volmd.iAuxCompCount);
	assert (m_pvopcRefQ1 != NULL);

	m_iFrameWidthY = m_pvopcRefQ0->whereY ().width;
	m_iFrameWidthUV = m_pvopcRefQ0->whereUV ().width;
	m_iFrameWidthYxMBSize = MB_SIZE * m_pvopcRefQ0->whereY ().width; 
	m_iFrameWidthYxBlkSize = BLOCK_SIZE * m_pvopcRefQ0->whereY ().width; 
	m_iFrameWidthUVxBlkSize = BLOCK_SIZE * m_pvopcRefQ0->whereUV ().width;

	// MB data	
	Int nBlk = (m_volmd.fAUsage == EIGHT_BIT) ? 6+m_volmd.iAuxCompCount*4 : 6;
	m_rgpiCoefQ = new Int* [nBlk];
	m_rgiQPpred = new Int [nBlk];
	Int iBlk;
	for (iBlk = 0; iBlk < nBlk; iBlk++)
		m_rgpiCoefQ [iBlk] = new Int [BLOCK_SQUARE_SIZE];
/* NBIT: change
	m_pidct = new CInvBlockDCT;
*/
	// m_pidct = new CInvBlockDCT(m_volmd.nBits);
	// HHI Schueuer: sadct
	if (m_volmd.fAUsage != RECTANGLE && (!m_volmd.bSadctDisable)) {
    	int i, j;
		m_pidct = new CInvSADCT(m_volmd.nBits);
		// m_pidct = new CInvBlockDCT(m_volmd.nBits); // 20-11
        m_rgiCurrMBCoeffWidth = new Int*[11];
        for (i=0; i<11; i++) 
        	m_rgiCurrMBCoeffWidth[i] = 0;
        
		for (i=Y_BLOCK1; i<=U_BLOCK; i++) 
			m_rgiCurrMBCoeffWidth[i] = new Int[BLOCK_SIZE];
        // shape of U and V is the same, no need to duplicate the data    
        m_rgiCurrMBCoeffWidth[V_BLOCK] = m_rgiCurrMBCoeffWidth[U_BLOCK];
        // the 8bit alpha blocks use the same shape information as the luminance blocks
        for (j=Y_BLOCK1,i=A_BLOCK1; i<=A_BLOCK4; i++,j++) {
        	m_rgiCurrMBCoeffWidth[i] = m_rgiCurrMBCoeffWidth[j];
        }    
	}
	else
		m_pidct = new CInvBlockDCT(m_volmd.nBits);
	// end HHI

	// motion vectors and MBMode
	Int iNumMBX = iSessionWidth / MB_SIZE; 
	if (iSessionWidth  % MB_SIZE != 0)				//round up
		iNumMBX++;
	Int iNumMBY = iSessionHeight / MB_SIZE;
	if (iSessionHeight % MB_SIZE != 0)				//deal with frational MB
		iNumMBY++;
	Int iNumMB = m_iSessNumMB = iNumMBX * iNumMBY;

	m_rgmbmd = new CMBMode [iNumMB];
  if (m_volmd.iAuxCompCount>0) {
    for(Int k=0; k<iNumMB; k++)
      m_rgmbmd[k]=CMBMode(m_volmd.iAuxCompCount);
  }
	m_rgmv = new CMotionVector [max(PVOP_MV_PER_REF_PER_MB, 2*BVOP_MV_PER_REF_PER_MB) * iNumMB];
	m_rgmvBackward = m_rgmv + BVOP_MV_PER_REF_PER_MB * m_iSessNumMB;
	m_rgmvRef = new CMotionVector [max(PVOP_MV_PER_REF_PER_MB, 2*BVOP_MV_PER_REF_PER_MB) * iNumMB];
	m_rgmvBY = new CMotionVector [iNumMB]; // for shape
	m_rgmbmdRef = new CMBMode [iNumMB];
  if (m_volmd.iAuxCompCount>0) {
    for(Int k=0; k<iNumMB; k++)
      m_rgmbmdRef[k]=CMBMode(m_volmd.iAuxCompCount);
  }

//OBSS_SAIT_991015
	m_rgmvBaseBY = new CMotionVector [iNumMB]; // for shape  
	if (m_volmd.volType == ENHN_LAYER && !(m_volmd.bSpatialScalability && m_volmd.iHierarchyType==0))
		m_rgshpmd = new ShapeMode [iNumMB];
//~OBSS_SAIT_991015
	
	// shape data
	if (m_volmd.fAUsage != RECTANGLE) {
		m_puciPredBAB = new CU8Image (CRct (0, 0, MC_BAB_SIZE, MC_BAB_SIZE));
		m_ppxlcReconCurrBAB = new PixelC [BAB_SIZE * BAB_SIZE];
		m_parcodec = new ArCodec;
		m_ppxlcCurrMBBYDown4 = new PixelC [8 * 8];
		m_ppxlcCurrMBBYDown2 = new PixelC [12 * 12];
		m_ppxlcPredBABDown4 = new PixelC [6*6];
		m_ppxlcPredBABDown2 = new PixelC [10*10];
	}


//  m_ppxlcPredMBA = new 
//  m_ppxliErrorMBA = new 
//  m_ppxlcPredMBBackA = new 

}

Void CVideoObject::computeVOLConstMembers ()
{
	m_iOffsetForPadY = m_rctRefFrameY.offset (m_rctCurrVOPY.left, m_rctCurrVOPY.top);
	m_iOffsetForPadUV = m_rctRefFrameUV.offset (m_rctCurrVOPUV.left, m_rctCurrVOPUV.top);
	m_rctPrevNoExpandY = m_rctCurrVOPY;
	m_rctPrevNoExpandUV = m_rctCurrVOPUV;
	m_iVOPWidthY = m_rctCurrVOPY.width;
	m_iVOPWidthUV = m_rctCurrVOPUV.width;
	m_iNumMBX = m_iNumMBXRef = m_iVOPWidthY / MB_SIZE; 
	m_iNumMBY = m_iNumMBYRef = m_rctCurrVOPY.height () / MB_SIZE;
	m_iNumMB = m_iNumMBRef = m_iNumMBX * m_iNumMBY;
	m_iNumOfTotalMVPerRow = PVOP_MV_PER_REF_PER_MB * m_iNumMBX;
	setRefStartingPointers ();
	m_pvopcCurrQ->setBoundRct (m_rctCurrVOPY);
	m_pvopcRefQ0->setBoundRct (m_rctRefVOPY0);
	m_pvopcRefQ1->setBoundRct (m_rctRefVOPY1);

	Int iMB, iBlk;
	Int nBlk = (m_volmd.fAUsage == EIGHT_BIT) ? 6+m_volmd.iAuxCompCount*4 : 6;
	m_rgblkmCurrMB = (const BlockMemory *)new BlockMemory [nBlk];

	m_rgpmbmAbove = new MacroBlockMemory* [m_iNumMBX];
	m_rgpmbmCurr  = new MacroBlockMemory* [m_iNumMBX];
	for (iMB = 0; iMB < m_iNumMBX; iMB++)	{
		m_rgpmbmAbove [iMB] = new MacroBlockMemory;
		m_rgpmbmAbove [iMB]->rgblkm = new BlockMemory [nBlk];
		m_rgpmbmCurr  [iMB] = new MacroBlockMemory;
		m_rgpmbmCurr  [iMB]->rgblkm = new BlockMemory [nBlk];
		for (iBlk = 0; iBlk < nBlk; iBlk++)	{
			(m_rgpmbmAbove [iMB]->rgblkm) [iBlk] = new Int [(BLOCK_SIZE << 1) - 1];
			(m_rgpmbmCurr  [iMB]->rgblkm) [iBlk] = new Int [(BLOCK_SIZE << 1) - 1];
		}
	}
}

Void CVideoObject::computeVOPMembers ()
{
	m_iVOPWidthY = m_rctCurrVOPY.width;
	m_iVOPWidthUV = m_rctCurrVOPUV.width;
	m_iNumMBX = m_iVOPWidthY / MB_SIZE; 
	m_iNumMBY = m_rctCurrVOPY.height () / MB_SIZE;
	m_iNumMB = m_iNumMBX * m_iNumMBY;
	m_iNumOfTotalMVPerRow = PVOP_MV_PER_REF_PER_MB * m_iNumMBX;

	Int iMB, iBlk;
	Int nBlk = (m_volmd.fAUsage == EIGHT_BIT) ? 6+m_volmd.iAuxCompCount*4 : 6;
	m_rgblkmCurrMB = (const BlockMemory *)new BlockMemory [nBlk];

	m_rgpmbmAbove = new MacroBlockMemory* [m_iNumMBX];
	m_rgpmbmCurr  = new MacroBlockMemory* [m_iNumMBX];
	for (iMB = 0; iMB < m_iNumMBX; iMB++)	{
		m_rgpmbmAbove [iMB] = new MacroBlockMemory;
		m_rgpmbmAbove [iMB]->rgblkm = new BlockMemory [nBlk];
		m_rgpmbmCurr  [iMB] = new MacroBlockMemory;
		m_rgpmbmCurr  [iMB]->rgblkm = new BlockMemory [nBlk];
		for (iBlk = 0; iBlk < nBlk; iBlk++)	{
			// BLOCK_SIZE*2-1 is 15 Ints for dc/ac prediction of coefficients
			(m_rgpmbmAbove [iMB]->rgblkm) [iBlk] = new Int [(BLOCK_SIZE << 1) - 1];
			(m_rgpmbmCurr  [iMB]->rgblkm) [iBlk] = new Int [(BLOCK_SIZE << 1) - 1];
		}
	}
}

Void CVideoObject::repeatPadYOrA (PixelC* ppxlcOldLeft, CVOPU8YUVBA* pvopcRef)
{
	Int iScale	= (m_vopmd.RRVmode.iOnOff == 1) ? (2) : (1);
	Int iExpand = EXPANDY_REFVOP * iScale;

	Int iStride = pvopcRef->whereY ().width;
	Int iOldWidth = (m_volmd.fAUsage == RECTANGLE) ? m_ivolWidth : m_rctPrevNoExpandY.width;
	Int iOldHeight = (m_volmd.fAUsage == RECTANGLE) ? m_ivolHeight : m_rctPrevNoExpandY.height();

	PixelC* ppxlcLeftTop = ppxlcOldLeft - iExpand * iStride - iExpand;
	Int iExpandR = iExpand + MB_SIZE * ((iOldWidth + MB_SIZE - 1)/MB_SIZE) - iOldWidth;
	Int iExpandB = iExpand + MB_SIZE * ((iOldHeight + MB_SIZE - 1)/MB_SIZE) - iOldHeight; // extra is added for non mb-multiple sizes
	Int iNewWidth = iExpand + iOldWidth + iExpandR;

	const PixelC* ppxlcOldRight = ppxlcOldLeft + iOldWidth - 1;
	const PixelC* ppxlcOldTopLn = ppxlcOldLeft - iExpand;

	PixelC* ppxlcNewLeft = (PixelC*) ppxlcOldTopLn;
	PixelC* ppxlcNewRight = (PixelC*) ppxlcOldRight + 1;
	CoordI y;
	Int i;

	for (y = 0; y < iOldHeight; y++) {
		for (i=0; i<iExpand; i++)
			ppxlcNewLeft[i] = *ppxlcOldLeft;
		for (i=0; i<iExpandR; i++)
			ppxlcNewRight[i] = *ppxlcOldRight;

		ppxlcNewLeft += iStride;		
		ppxlcNewRight += iStride;
		ppxlcOldLeft += iStride;
		ppxlcOldRight += iStride;
	}

	const PixelC* ppxlcOldBotLn = ppxlcNewLeft - iStride;
	for (y = 0; y < iExpand; y++) {
		memcpy (ppxlcLeftTop, ppxlcOldTopLn, iNewWidth * sizeof(PixelC));
		ppxlcLeftTop += iStride;
	}
	for (y = 0; y < iExpandB; y++) {	
		memcpy (ppxlcNewLeft, ppxlcOldBotLn, iNewWidth * sizeof(PixelC));
		ppxlcNewLeft += iStride;
	}
}

Void CVideoObject::repeatPadUV (CVOPU8YUVBA* pvopcRef)
{
	Int iScale	= (m_vopmd.RRVmode.iOnOff == 1) ? (2) : (1);
	Int iExpand = EXPANDUV_REFVOP * iScale;
	Int iStride = pvopcRef->whereUV ().width;
	Int iOldWidth = (m_volmd.fAUsage == RECTANGLE) ? m_ivolWidth/2 : m_rctPrevNoExpandUV.width;
	Int iOldHeight = (m_volmd.fAUsage == RECTANGLE) ? m_ivolHeight/2 : m_rctPrevNoExpandUV.height();
	Int iMBSize = MB_SIZE>>1;
	Int iExpandR = iExpand + iMBSize * ((iOldWidth + iMBSize - 1)/iMBSize) - iOldWidth;
	Int iExpandB = iExpand + iMBSize * ((iOldHeight + iMBSize - 1)/iMBSize) - iOldHeight; // extra is added for non mb-multiple sizes
	Int iNewWidth = iExpand + iOldWidth + iExpandR;

	const PixelC* ppxlcOldLeftU = pvopcRef->pixelsU () + m_iOffsetForPadUV; // point to the left-top of bounding box
	const PixelC* ppxlcOldLeftV = pvopcRef->pixelsV () + m_iOffsetForPadUV; // point to the left-top of bounding box

	PixelC* ppxlcLeftTopU = (PixelC*) ppxlcOldLeftU - iExpand * iStride - iExpand;
	PixelC* ppxlcLeftTopV = (PixelC*) ppxlcOldLeftV - iExpand * iStride - iExpand;

		// point to the left top of the expanded bounding box (+- EXPANDY_REFVOP)
	const PixelC* ppxlcOldRightU = ppxlcOldLeftU + iOldWidth - 1;
	const PixelC* ppxlcOldRightV = ppxlcOldLeftV + iOldWidth - 1;
		// point to the right-top of the bounding box

	const PixelC* ppxlcOldTopLnU = ppxlcOldLeftU - iExpand;
	const PixelC* ppxlcOldTopLnV = ppxlcOldLeftV - iExpand;

	PixelC* ppxlcNewLeftU = (PixelC*) ppxlcOldTopLnU;
	PixelC* ppxlcNewLeftV = (PixelC*) ppxlcOldTopLnV;
	PixelC* ppxlcNewRightU = (PixelC*) ppxlcOldRightU + 1;
	PixelC* ppxlcNewRightV = (PixelC*) ppxlcOldRightV + 1;
	
	CoordI y;
	for (y = 0; y < iOldHeight; y++) {
		Int i;
		for (i=0; i<iExpand; i++) {
			ppxlcNewLeftU[i] = *ppxlcOldLeftU;
			ppxlcNewLeftV[i] = *ppxlcOldLeftV;
		}
		for (i=0; i<iExpandR; i++) {
			ppxlcNewRightU[i] = *ppxlcOldRightU;
			ppxlcNewRightV[i] = *ppxlcOldRightV;
		}
		
		ppxlcNewLeftU += iStride;		
		ppxlcNewLeftV += iStride;		
		ppxlcNewRightU += iStride;
		ppxlcNewRightV += iStride;
		ppxlcOldLeftU += iStride;
		ppxlcOldLeftV += iStride;
		ppxlcOldRightU += iStride;
		ppxlcOldRightV += iStride;
	}

	const PixelC* ppxlcOldBotLnU = ppxlcNewLeftU - iStride;
	const PixelC* ppxlcOldBotLnV = ppxlcNewLeftV - iStride;
	for (y = 0; y < iExpand; y++) {
		memcpy (ppxlcLeftTopU, ppxlcOldTopLnU, iNewWidth * sizeof(PixelC));
		memcpy (ppxlcLeftTopV, ppxlcOldTopLnV, iNewWidth * sizeof(PixelC));
		ppxlcLeftTopU += iStride;
		ppxlcLeftTopV += iStride;
	}
	for (y = 0; y < iExpandB; y++) {
		memcpy (ppxlcNewLeftU, ppxlcOldBotLnU, iNewWidth * sizeof(PixelC));
		memcpy (ppxlcNewLeftV, ppxlcOldBotLnV, iNewWidth * sizeof(PixelC));
		ppxlcNewLeftU += iStride;
		ppxlcNewLeftV += iStride;
	}
//	pvopcRef->dump("bar.yuv");
}


Void CVideoObject::saveShapeMode()
{
	// called after reference frame encode/decode
	if(m_rgshpmd == NULL)
	{
		// first time
		m_iRefShpNumMBX = m_iNumMBX;
		m_iRefShpNumMBY = m_iNumMBY;
		m_rgshpmd = new ShapeMode [m_iNumMB];
	}
	else
	{
		// update if changed
		if(m_iRefShpNumMBX!=m_iNumMBXRef || m_iRefShpNumMBY != m_iNumMBYRef)
		{
			delete [] m_rgshpmd;
			m_rgshpmd = new ShapeMode [m_iNumMBRef];
			m_iRefShpNumMBX = m_iNumMBXRef;
			m_iRefShpNumMBY = m_iNumMBYRef;
		}
		// copy shape mode from previous ref to save
		Int i;
		for (i=0; i<m_iNumMBRef; i++ )
			m_rgshpmd[i] = m_rgmbmdRef[i].m_shpmd;
	}
}

//OBSS_SAIT_991015
Void CVideoObject::saveBaseShapeMode()
{
	// called after reference frame encode/decode
	if(m_rgBaseshpmd == NULL){
		// first time
		m_iNumMBBaseXRef = m_iNumMBX;					
		m_iNumMBBaseYRef = m_iNumMBY;					

		// save current shape mode for enhancement layer shape mode coding
		if(m_volmd.volType == BASE_LAYER) {										
			m_rgBaseshpmd = new ShapeMode [m_iNumMB];

			// first time
			m_iRefShpNumMBX = m_iNumMBX;
			m_iRefShpNumMBY = m_iNumMBY;
			
			Int i;
			Int iMBX, iMBY;
			for (iMBY=0, i=0; iMBY<m_iNumMBY; iMBY++ ) {
				for (iMBX=0; iMBX<m_iNumMBX; iMBX++ ) {
					m_rgBaseshpmd[i] = m_rgmbmd[i].m_shpmd;
					i++;
				}
			}
		}
	} else {
		// update if changed
		if(m_volmd.volType == BASE_LAYER) {				
			//for save lower layer shape mode
			if(m_iNumMBBaseXRef!=m_iNumMBX || m_iNumMBBaseYRef != m_iNumMBY){
				delete [] m_rgBaseshpmd;
				m_rgBaseshpmd = new ShapeMode [m_iNumMB];
				m_iNumMBBaseXRef = m_iNumMBX;		
				m_iNumMBBaseYRef = m_iNumMBY;		
			}
			Int i=0;
			Int iMBX, iMBY;
			for (iMBY=0, i=0; iMBY<m_iNumMBY; iMBY++ ){ 
				for (iMBX=0; iMBX<m_iNumMBX; iMBX++ ){
						m_rgBaseshpmd[i] = m_rgmbmd[i].m_shpmd;
						i++;
				}
			}
			m_iNumMBBaseXRef = m_iNumMBX;					
			m_iNumMBBaseYRef = m_iNumMBY;					
			//for save lawer layer shape mode
		}	
	}
}
//~OBSS_SAIT_991015

Void CVideoObject::resetBYPlane ()
{
	if (m_vopmd.vopPredType == PVOP || m_vopmd.vopPredType == IVOP || (m_uiSprite == 2 && m_vopmd.vopPredType == SPRITE)) { // GMC
		PixelC* ppxlcBY = (PixelC*) m_pvopcRefQ1->pixelsBY ();
		memset (ppxlcBY, 0, m_pvopcRefQ1->whereY().area() * sizeof(PixelC));
	}
	else {
		PixelC* ppxlcBY = (PixelC*) m_pvopcCurrQ->pixelsBY ();
		memset (ppxlcBY, 0, m_pvopcCurrQ->whereY().area() * sizeof(PixelC));
	}
}

Void CVideoObject::updateAllRefVOPs ()
// perform this after VOP prediction type decided and before encoding
{
	if (m_vopmd.vopPredType != BVOP) {
		m_rctRefVOPY0 = m_rctRefVOPY1;
		swapVOPU8Pointers (m_pvopcRefQ0, m_pvopcRefQ1);
	}
}

// for spatial scalability
Void CVideoObject::updateAllRefVOPs (const CVOPU8YUVBA* pvopcRefBaseLayer)
{
	CVOPU8YUVBA *pvopcUpSampled = NULL;
	//	Int fEnhancementLayer = 0;

	assert (m_volmd.volType == ENHN_LAYER);
//OBSS_SAIT_991015
	pvopcUpSampled = pvopcRefBaseLayer->upsampleForSpatialScalability (
													m_volmd.iver_sampling_factor_m,
													m_volmd.iver_sampling_factor_n,
													m_volmd.ihor_sampling_factor_m,
													m_volmd.ihor_sampling_factor_n,
													m_volmd.iver_sampling_factor_m_shape,
													m_volmd.iver_sampling_factor_n_shape,
													m_volmd.ihor_sampling_factor_m_shape,
													m_volmd.ihor_sampling_factor_n_shape,
													m_volmd.iFrmWidth_SS,				 
													m_volmd.iFrmHeight_SS,				 
													m_volmd.bShapeOnly,					 
													EXPANDY_REF_FRAME,
													EXPANDUV_REF_FRAME);
//~OBSS_SAIT_991015


	if(m_vopmd.vopPredType == PVOP || (m_uiSprite == 2 && m_vopmd.vopPredType == SPRITE)) { // GMC
//OBSS_SAIT_991015
		if(m_volmd.fAUsage == RECTANGLE) {
			m_rctRefVOPY0 = m_rctRefVOPY1;	
		}
//OBSSFIX__000323
		else if(m_volmd.bSpatialScalability && m_volmd.iHierarchyType == 0 && m_volmd.iEnhnType != 0 && m_volmd.iuseRefShape == 1){
//			m_rctRefVOPY0 = m_rctRefVOPY1;
			if(pvopcUpSampled->fAUsage() == RECTANGLE)
				m_rctRefVOPY0 = pvopcUpSampled->whereY();
			else{
				CRct tmp;
				tmp = m_rctBase;
				tmp.left = (int)((tmp.left) * (m_volmd.ihor_sampling_factor_n_shape)/(m_volmd.ihor_sampling_factor_m_shape));
				tmp.right = (int)((tmp.right) * (m_volmd.ihor_sampling_factor_n_shape)/(m_volmd.ihor_sampling_factor_m_shape));
				tmp.top = (int)((tmp.top) * (m_volmd.iver_sampling_factor_n_shape)/(m_volmd.iver_sampling_factor_m_shape));
				tmp.bottom = (int)((tmp.bottom) * (m_volmd.iver_sampling_factor_n_shape)/(m_volmd.iver_sampling_factor_m_shape));
				tmp.width = tmp.right - tmp.left ;
				tmp.left -= 32; tmp.right += 32;
				tmp.top -= 32; tmp.bottom += 32;
				tmp.width += 64;
//OBSS_SAIT_FIX000524
				if(!( tmp <= pvopcUpSampled->whereY () )) {
					if(!(tmp.left >= (pvopcUpSampled->whereY ()).left))
						tmp.left = (pvopcUpSampled->whereY ()).left;
					if(!(tmp.top >= (pvopcUpSampled->whereY ()).top))
						tmp.top = (pvopcUpSampled->whereY ()).top;
					if(!(tmp.right <= (pvopcUpSampled->whereY ()).right))
						tmp.right = (pvopcUpSampled->whereY ()).right;
					if(!(tmp.bottom <= (pvopcUpSampled->whereY ()).bottom))
						tmp.bottom = (pvopcUpSampled->whereY ()).bottom;
				}
//~OBSS_SAIT_FIX000524
				m_rctRefVOPY0 = tmp;	// m_rctBase : BASE_LAYER RECT SIZE
			}
		}
//~OBSSFIX__000323
		else if(m_volmd.fAUsage == ONE_BIT) {		
			CRct tmp;
			tmp = m_rctBase;
			tmp.left = (int)((tmp.left) * (m_volmd.ihor_sampling_factor_n_shape)/(m_volmd.ihor_sampling_factor_m_shape));
			tmp.right = (int)((tmp.right) * (m_volmd.ihor_sampling_factor_n_shape)/(m_volmd.ihor_sampling_factor_m_shape));
			tmp.top = (int)((tmp.top) * (m_volmd.iver_sampling_factor_n_shape)/(m_volmd.iver_sampling_factor_m_shape));
			tmp.bottom = (int)((tmp.bottom) * (m_volmd.iver_sampling_factor_n_shape)/(m_volmd.iver_sampling_factor_m_shape));
			tmp.width = tmp.right - tmp.left ;

			tmp.left -= 32; tmp.right += 32;
			tmp.top -= 32; tmp.bottom += 32;
			tmp.width += 64;
//OBSS_SAIT_FIX000524
			if(!( tmp <= pvopcUpSampled->whereY () )) {
				if(!(tmp.left >= (pvopcUpSampled->whereY ()).left))
					tmp.left = (pvopcUpSampled->whereY ()).left;
				if(!(tmp.top >= (pvopcUpSampled->whereY ()).top))
					tmp.top = (pvopcUpSampled->whereY ()).top;
				if(!(tmp.right <= (pvopcUpSampled->whereY ()).right))
					tmp.right = (pvopcUpSampled->whereY ()).right;
				if(!(tmp.bottom <= (pvopcUpSampled->whereY ()).bottom))
					tmp.bottom = (pvopcUpSampled->whereY ()).bottom;
			}
//~OBSS_SAIT_FIX000524
			m_rctRefVOPY0 = tmp;	// m_rctBase : BASE_LAYER RECT SIZE
		}
//~OBSS_SAIT_991015

		swapVOPU8Pointers (m_pvopcRefQ0,pvopcUpSampled);
		m_pvopcRefQ0->setBoundRct(m_rctRefVOPY0);
		delete pvopcUpSampled;
	}
	else if(m_vopmd.vopPredType == BVOP){
		CRct tmp;
//OBSS_SAIT_991015
		if(m_volmd.fAUsage == RECTANGLE) {
			tmp = m_rctRefVOPY0; 
		}
//OBSSFIX_MODE3_02
		else if(m_volmd.bSpatialScalability && m_volmd.iHierarchyType == 0 && m_volmd.iEnhnType != 0 && m_volmd.iuseRefShape == 1){
//			tmp = pvopcUpSampled->whereY();
			if(pvopcUpSampled->fAUsage()==RECTANGLE)
				tmp = pvopcUpSampled->whereY();
			else{
				tmp = m_rctBase;
				tmp.left = (int)((tmp.left) * (m_volmd.ihor_sampling_factor_n_shape)/(m_volmd.ihor_sampling_factor_m_shape));
				tmp.right = (int)((tmp.right) * (m_volmd.ihor_sampling_factor_n_shape)/(m_volmd.ihor_sampling_factor_m_shape));
				tmp.top = (int)((tmp.top) * (m_volmd.iver_sampling_factor_n_shape)/(m_volmd.iver_sampling_factor_m_shape));
				tmp.bottom = (int)((tmp.bottom) * (m_volmd.iver_sampling_factor_n_shape)/(m_volmd.iver_sampling_factor_m_shape));
				tmp.width = tmp.right - tmp.left ;
				tmp.left -= 32; tmp.right += 32;
				tmp.top -= 32; tmp.bottom += 32;
				tmp.width += 64;
//OBSS_SAIT_FIX000524
				if(!( tmp <= pvopcUpSampled->whereY () )) {
					if(!(tmp.left >= (pvopcUpSampled->whereY ()).left))
						tmp.left = (pvopcUpSampled->whereY ()).left;
					if(!(tmp.top >= (pvopcUpSampled->whereY ()).top))
						tmp.top = (pvopcUpSampled->whereY ()).top;
					if(!(tmp.right <= (pvopcUpSampled->whereY ()).right))
						tmp.right = (pvopcUpSampled->whereY ()).right;
					if(!(tmp.bottom <= (pvopcUpSampled->whereY ()).bottom))
						tmp.bottom = (pvopcUpSampled->whereY ()).bottom;
				}
//~OBSS_SAIT_FIX000524
			}
		}
//~OBSSFIX_MODE3_02
		else if(m_volmd.fAUsage == ONE_BIT) {
			tmp = m_rctBase;
			tmp.left = (int)((tmp.left) * (m_volmd.ihor_sampling_factor_n_shape)/(m_volmd.ihor_sampling_factor_m_shape));
			tmp.right = (int)((tmp.right) * (m_volmd.ihor_sampling_factor_n_shape)/(m_volmd.ihor_sampling_factor_m_shape));
			tmp.top = (int)((tmp.top) * (m_volmd.iver_sampling_factor_n_shape)/(m_volmd.iver_sampling_factor_m_shape));
			tmp.bottom = (int)((tmp.bottom) * (m_volmd.iver_sampling_factor_n_shape)/(m_volmd.iver_sampling_factor_m_shape));
			tmp.width = tmp.right - tmp.left ;
			tmp.left -= 32; tmp.right += 32;
			tmp.top -= 32; tmp.bottom += 32;
			tmp.width += 64;
//OBSS_SAIT_FIX000524
			if(!( tmp <= pvopcUpSampled->whereY () )) {
				if(!(tmp.left >= (pvopcUpSampled->whereY ()).left))
					tmp.left = (pvopcUpSampled->whereY ()).left;
				if(!(tmp.top >= (pvopcUpSampled->whereY ()).top))
					tmp.top = (pvopcUpSampled->whereY ()).top;
				if(!(tmp.right <= (pvopcUpSampled->whereY ()).right))
					tmp.right = (pvopcUpSampled->whereY ()).right;
				if(!(tmp.bottom <= (pvopcUpSampled->whereY ()).bottom))
					tmp.bottom = (pvopcUpSampled->whereY ()).bottom;
			}
//~OBSS_SAIT_FIX000524
		}
//~OBSS_SAIT_991015

		m_rctRefVOPY0 = m_rctRefVOPY1;
		m_rctRefVOPY1 = tmp;
		swapVOPU8Pointers (m_pvopcRefQ0,m_pvopcRefQ1);
		swapVOPU8Pointers (m_pvopcRefQ1,pvopcUpSampled);
		m_pvopcRefQ0->setBoundRct(m_rctRefVOPY0);
		m_pvopcRefQ1->setBoundRct(m_rctRefVOPY1);
		delete pvopcUpSampled;
	}
}

Void CVideoObject::swapVOPU8Pointers (CVOPU8YUVBA*& pvopc0, CVOPU8YUVBA*& pvopc1)
{
	CVOPU8YUVBA* pvopcTmp = pvopc0;
	pvopc0 = pvopc1;
	pvopc1 = pvopcTmp;
}

Void CVideoObject::swapVOPIntPointers (CVOPIntYUVBA*& pvopi0, CVOPIntYUVBA*& pvopi1)
{
	CVOPIntYUVBA* pvopiTmp = pvopi0;
	pvopi0 = pvopi1;
	pvopi1 = pvopiTmp;
}

Void CVideoObject::setRefStartingPointers ()
{
	m_iStartInRefToCurrRctY = m_rctRefFrameY.offset (m_rctCurrVOPY.left, m_rctCurrVOPY.top);
	m_iStartInRefToCurrRctUV = m_rctRefFrameUV.offset (m_rctCurrVOPUV.left, m_rctCurrVOPUV.top);
}

const CVOPU8YUVBA* CVideoObject::pvopcReconCurr () const
{
	if ((m_uiSprite == 1 && m_vopmd.vopPredType == SPRITE) && m_iNumOfPnts > 0) // GMC
		return m_pvopcCurrQ;
	else if ((m_uiSprite == 1 && m_vopmd.vopPredType == SPRITE) && m_iNumOfPnts == 0) { // GMC
		if (m_sptMode != BASIC_SPRITE) 
			return m_pvopcSptQ;
		else 
			return m_pvopcRefQ1;
	}
	else if ((m_vopmd.vopPredType == BVOP && m_volmd.volType == BASE_LAYER) 
			 ||(m_vopmd.vopPredType == BVOP && m_vopmd.iRefSelectCode != 0))
		return m_pvopcCurrQ;
	else
		return m_pvopcRefQ1;
}
////////// 97/12/22 start
Void CVideoObject::compute_bfShapeMembers ()
{
	m_iVOPWidthY = m_rctCurrVOPY.width;
	m_iVOPWidthUV = m_rctCurrVOPUV.width;
	m_iNumMBX = m_iVOPWidthY / MB_SIZE; 
	m_iNumMBY = m_rctCurrVOPY.height () / MB_SIZE;
	m_iNumMB = m_iNumMBX * m_iNumMBY;
//	m_iNumOfTotalMVPerRow = 5 * m_iNumMBX;
//  wchen: changed to const as discussed with Bob.
	m_iNumOfTotalMVPerRow = PVOP_MV_PER_REF_PER_MB * m_iNumMBX;
}

Void CVideoObject::copyVOPU8YUVBA(CVOPU8YUVBA*& pvopc0, CVOPU8YUVBA*& pvopc1)
{
	delete pvopc0;  pvopc0 = NULL;
	pvopc0 = new CVOPU8YUVBA (*pvopc1);	/* i.e. pvopc0 = pvopc1; */
}

Void CVideoObject::copyVOPU8YUVBA(CVOPU8YUVBA*& pvopc0, CVOPU8YUVBA*& pvopc1, CVOPU8YUVBA*& pvopc2)
{
	delete pvopc0;  pvopc0 = NULL;
	pvopc0 = (pvopc1 != NULL) ?
		new CVOPU8YUVBA (*pvopc1) : new CVOPU8YUVBA (*pvopc2);
}
///// 97/12/22 end

// QuarterSample
Void CVideoObject::blkInterpolateY (
	                 const PixelC* ppxlcRefLeftTop, 
                     // point to left-top of the frame
					 Int blkSize, // Blocksize (0=16x8, 8=8x8, 16=16x16)
					 Int xRef, Int yRef, // x + mvX in quarter pel unit
					 U8* ppxlcblk, // Pointer to quarter sample interpolated block
                     Int iRoundingControl
					 ) 
{
  Int maxVal = (1<<m_volmd.nBits)-1; // mwi
  Int c[]={160, -48, 24, -8}; // Filter Coefficients

  U8 refblk[MB_SIZE+7][MB_SIZE+7];
  U8 tmpblk0[MB_SIZE+7][MB_SIZE+7];  
  U8 tmpblk1[MB_SIZE+1][MB_SIZE+1];

  Int x_int, y_int, x_frac, y_frac;
  Int blkSizeX, blkSizeY, iWidthY;  
  Int ix, iy, k;

  Long u;
  Int yTmp = yRef>>1;  // Field MV!

  if (blkSize == 0) {
    blkSizeX = 16;
    blkSizeY = 8;
    iWidthY  = 2*m_iFrameWidthY;
    
    y_int  = yTmp/4;
    if (yTmp < 0)
      if (yTmp%4) y_int--;
    y_frac = yTmp - (4*y_int);
  }
  else {
    blkSizeX = blkSize;
    blkSizeY = blkSize;
    iWidthY  = m_iFrameWidthY;
    y_int  = yRef/4;
    if (yRef < 0)
      if (yRef%4) y_int--;
    y_frac = yRef - (4*y_int);
  }

  x_int  = xRef/4;
  if (xRef < 0)
    if (xRef%4) x_int--;
  x_frac = xRef - (4*x_int);
  
  
  // Initialization
  const PixelC* ppxlcRef = ppxlcRefLeftTop + y_int * iWidthY + EXPANDY_REF_FRAME
    * m_iFrameWidthY + x_int + EXPANDY_REF_FRAME;
  for (iy=0; iy <= blkSizeY; iy++) {
    for (ix=0; ix <= blkSizeX; ix++) {
      refblk[iy+3][ix+3] = ppxlcRef[ix+iy*iWidthY];
    }
  }
   
  // Horizontal Mirroring

   for (iy=0; iy <= blkSizeY; iy++) {
	 for (ix=0; ix <= 2; ix++) {
	   refblk[iy+3][ix] = refblk[iy+3][5-ix];
	   refblk[iy+3][blkSizeX+ix+4] = refblk[iy+3][blkSizeX-ix+3];
	 }
   }
   
   switch (x_frac) {
	 
     case 0 : // Full Pel Position
       for (iy=0; iy <= blkSizeY; iy++) {
	     for (ix=0; ix <= blkSizeX-1; ix++) {
		   tmpblk0[iy+3][ix] = refblk[iy+3][ix+3];
	     }
	   }
	 break;
	 
     case 1 : // Quarter Pel Position
       for (iy=0; iy <= blkSizeY; iy++) {
	     for (ix=0; ix <= blkSizeX-1; ix++) {
           u = 0;
		   for (k=0; k<=3; k++) {
			 u += c[k] * ( refblk[iy+3][ix+3-k] + refblk[iy+3][ix+4+k] );
		   }
		   u = (u+128-iRoundingControl)/256;
		   u = max(0,min(maxVal,u));
		   tmpblk1[iy][ix] = (U8)u; 
	     }
	   }
       for (iy=0; iy <= blkSizeY; iy++) {
		 for (ix=0; ix <= blkSizeX-1; ix++) {
		   tmpblk0[iy+3][ix] = ( refblk[iy+3][ix+3] + tmpblk1[iy][ix] + 1 - iRoundingControl)/2;
		 }
	   }
     break;
	   
     case 2 : // Half Pel Position
       for (iy=0; iy <= blkSizeY; iy++) {
	     for (ix=0; ix <= blkSizeX-1; ix++) {
           u = 0;
		   for (k=0; k<=3; k++) {
			 u += c[k] * ( refblk[iy+3][ix+3-k] + refblk[iy+3][ix+4+k] );
		   }
		   u = (u+128-iRoundingControl)/256;
		   u = max(0,min(maxVal,u));
// GMC_V3
		   tmpblk0[iy+3][ix] = (U8)u; 
//		   tmpblk1[iy][ix] = (U8)u; 
// ~GMC_V3
	     }
	   }
	 break;
	   
     case 3 : // Quarter Pel Position
       for (iy=0; iy <= blkSizeY; iy++) {
	     for (ix=0; ix <= blkSizeX-1; ix++) {
           u = 0;
		   for (k=0; k<=3; k++) {
			 u += c[k] * ( refblk[iy+3][ix+3-k] + refblk[iy+3][ix+4+k] );
		   }
		   u = (u+128-iRoundingControl)/256;
		   u = max(0,min(maxVal,u));
		   tmpblk1[iy][ix] = (U8)u; 
	     }
	   }
       for (iy=0; iy <= blkSizeY; iy++) {
		 for (ix=0; ix <= blkSizeX-1; ix++) {
		   tmpblk0[iy+3][ix] = ( refblk[iy+3][ix+4] + tmpblk1[iy][ix] + 1 - iRoundingControl)/2;
		 }
	   }
     break;
	 
   }

   // Copy tmpblk0 -> refblk

   for (iy=0; iy <= blkSizeY; iy++) {
	 for (ix=0; ix <= blkSizeX-1; ix++) {
	   refblk[iy+3][ix] = tmpblk0[iy+3][ix];
	 }
   }
   
   // Vertical Mirroring

   for (iy=0; iy <= 2; iy++) {
	 for (ix=0; ix <= blkSizeX-1; ix++) {
	   refblk[iy][ix] = refblk[5-iy][ix];
	   refblk[blkSizeY+iy+4][ix] = refblk[blkSizeY-iy+3][ix];
	 }
   }
   
   switch (y_frac) {
	 
     case 0 : // Full Pel Position
       for (iy=0; iy <= blkSizeY-1; iy++) {
	     for (ix=0; ix <= blkSizeX-1; ix++) {
		   tmpblk0[iy][ix] = refblk[iy+3][ix];
	     }
	   }
	 break;
	 
     case 1 : // Quarter Pel Position
       for (iy=0; iy <= blkSizeY-1; iy++) {
	     for (ix=0; ix <= blkSizeX-1; ix++) {		   
           u = 0;
		   for (k=0; k<=3; k++) {
			 u += c[k] * ( refblk[iy+3-k][ix] + refblk[iy+4+k][ix] );
		   }
		   u = (u+128-iRoundingControl)/256;
		   u = max(0,min(maxVal,u));
		   tmpblk1[iy][ix] = (U8)u; 
	     }
	   }
       for (iy=0; iy <= blkSizeY-1; iy++) {
		 for (ix=0; ix <= blkSizeX-1; ix++) {
		   tmpblk0[iy][ix] = ( refblk[iy+3][ix] + tmpblk1[iy][ix] + 1 - iRoundingControl)/2;
		 }
	   }
     break;
	   
     case 2 : // Half Pel Position
       for (iy=0; iy <= blkSizeY-1; iy++) {
	     for (ix=0; ix <= blkSizeX-1; ix++) {
           u = 0;
		   for (k=0; k<=3; k++) {
			 u += c[k] * ( refblk[iy+3-k][ix] + refblk[iy+4+k][ix] );
		   }
		   u = (u+128-iRoundingControl)/256;
		   u = max(0,min(maxVal,u));
// GMC_V3
		   tmpblk0[iy][ix] = (U8)u; 
//		   tmpblk1[iy][ix] = (U8)u; 
// ~GMC_V3
	     }
	   }
	 break;
	   
     case 3 : // Quarter Pel Position
       for (iy=0; iy <= blkSizeY-1; iy++) {
	     for (ix=0; ix <= blkSizeX-1; ix++) {
           u = 0;
		   for (k=0; k<=3; k++) {
			 u += c[k] * ( refblk[iy+3-k][ix] + refblk[iy+4+k][ix] );
		   }
		   u = (u+128-iRoundingControl)/256;
		   u = max(0,min(maxVal,u));
		   tmpblk1[iy][ix] = (U8)u; 
	     }
	   }
       for (iy=0; iy <= blkSizeY-1; iy++) {
		 for (ix=0; ix <= blkSizeX-1; ix++) {
		   tmpblk0[iy][ix] = ( refblk[iy+4][ix] + tmpblk1[iy][ix] + 1 - iRoundingControl)/2;
		 }
	   }
     break;
	 
   }

   // Copy tmpblk0 -> ppxlcblk

   if (blkSize == 0) {
     for (iy=0; iy <= blkSizeY-1; iy++) {
       for (ix=0; ix <= blkSizeX-1; ix++) {
	     *(ppxlcblk+ix+iy*2*blkSizeX) = tmpblk0[iy][ix];
	   }
     }
   }
   else {
     for (iy=0; iy <= blkSizeY-1; iy++) {
       for (ix=0; ix <= blkSizeX-1; ix++) {
	     *(ppxlcblk+ix+iy*blkSizeX) = tmpblk0[iy][ix];
	   }
     }
   }
}
// ~QuarterSample

Void dumpNonCodedFrame(FILE* pfYUV, FILE* pfSeg, FILE **ppfAux, Int iAuxCompCount, CRct& rct, UInt nBits)
{
	// black frame
	Int iW = rct.width;
	Int iH = rct.height();
	Int i;

	PixelC *ppxlcPix = new PixelC [iW];

	pxlcmemset(ppxlcPix, 0, iW);
	for(i=0; i<iH; i++) // Y
		fwrite(ppxlcPix, sizeof(PixelC), iW, pfYUV);

	if(pfSeg!=NULL)
	{
		for(i=0; i<iH; i++) // A
			fwrite(ppxlcPix, sizeof(PixelC), iW, pfSeg);
	}
	if(ppfAux!=NULL)
	{
		Int iAux;
		for(iAux=0; iAux<iAuxCompCount; iAux++)
			if(ppfAux[iAux]!=NULL)
			{
				for(i=0; i<iH; i++)
					fwrite(ppxlcPix, sizeof(PixelC), iW, ppfAux[iAux]);
			}
	}

	PixelC pxlcValUV = 1<<(nBits-1);
	pxlcmemset(ppxlcPix, pxlcValUV, iW>>1);
	for(i=0; i<iH; i++) // UV
		fwrite(ppxlcPix, sizeof(PixelC), iW>>1, pfYUV);

	delete ppxlcPix;
}

Int CVideoObject::getAuxCompCount(Int vol_shape_extension)
{
  switch (vol_shape_extension) {  /* see N2687, pg. 178, tab. V2-1 */
    case -1:	return 1;
    case 0:  return 1;
    case 1:  return 1;
    case 2:  return 2;
    case 3:  return 2;      
    case 4:  return 3;
    case 5:  return 1;
    case 6:  return 2;
    case 7:  return 1;
    case 8:  return 1;
    case 9:  return 2;
    case 10: return 3;
    case 11: return 2;
    case 12: return 3;
    default: return 1;
  }  
}


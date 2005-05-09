/*************************************************************************

This software module was originally developed by 

	Ming-Chieh Lee (mingcl@microsoft.com), Microsoft Corporation
	Simon Winder (swinder@microsoft.com), Microsoft Corporation
	(date: June, 1997)
and edited by
        Wei Wu (weiwu@stallion.risc.rockwell.com) Rockwell Science Center

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
assign or donate the code to a third party and to inhibit third parties from using the code for non MPEG-4 Video conforming products. 
This copyright notice must be included in all copies or derivative works. 

Copyright (c) 1996, 1997.


Module Name:

	mcPad.cpp

Abstract:

	MB Padding (for motion estimation and compensation).

Revision History:
        May. 9   1998:  add field based MC padding  by Hyundai Electronics
                                  Cheol-Soo Park (cspark@super5.hyundai.co.kr)

*************************************************************************/

#include "typeapi.h"
#include "codehead.h"
#include "mode.hpp"
#include "vopses.hpp"

// size of image for ppxlcAlphaBase is (uiBlkSize X uiBlkSize)


#define invalidColour -1

// Added for field based MC padding by Hyundai(1998-5-9)
#define MB_FIELDY       1
#define MB_FIELDC       3
// End of Hyundai(1998-5-9)

Void CVideoObject::mcPadCurrMB (
	PixelC* ppxlcRefMBY, 
	PixelC* ppxlcRefMBU, PixelC* ppxlcRefMBV,
	PixelC** pppxlcRefMBA 
)
{
	mcPadCurr (ppxlcRefMBY, m_ppxlcCurrMBBY, MB_SIZE, m_iFrameWidthY);
	mcPadCurr (ppxlcRefMBU, m_ppxlcCurrMBBUV, BLOCK_SIZE, m_iFrameWidthUV);
	mcPadCurr (ppxlcRefMBV, m_ppxlcCurrMBBUV, BLOCK_SIZE, m_iFrameWidthUV);
  if (m_volmd.fAUsage == EIGHT_BIT) {
    for(Int iAuxComp=0;iAuxComp<m_volmd.iAuxCompCount;iAuxComp++) // MAC (SB) 29-Nov-99
		  mcPadCurr (pppxlcRefMBA[iAuxComp], m_ppxlcCurrMBBY, MB_SIZE, m_iFrameWidthY);
  }
}

Void CVideoObject::mcPadCurr (
	PixelC *ppxlcTextureBase, // (uiStride X ???)
	const PixelC *ppxlcAlphaBase, // uiBlkSize X uiBlkSize
	UInt uiBlkSize, UInt uiStride
)
{
	Int iUnit = sizeof(PixelC); // NBIT: memcpy
	CoordI iX,iY,iJ,iLeftX = 0;
	Bool bEmptyRow = FALSE;
	Bool bInVop;
	Int iLeftColour;

	PixelC *ppxlcTexture = ppxlcTextureBase;
	const PixelC *ppxlcAlpha = ppxlcAlphaBase;

	for (iY=0; iY < (CoordI) uiBlkSize; iY++, ppxlcTexture+=uiStride)
    {
		bInVop = TRUE;
		iLeftColour = invalidColour;
		m_pbEmptyRowArray[iY]=0;

		for(iX=0;iX<(CoordI)uiBlkSize;iX++,ppxlcAlpha++)
		{
			if(bInVop==TRUE && *ppxlcAlpha==transpValue)
		    {
				// start of stripe or left border 
				bInVop=FALSE;
				iLeftX=iX;
				if(iX>0)
					iLeftColour=ppxlcTexture[iLeftX-1];
		    }
			else if(bInVop==FALSE && *ppxlcAlpha!=transpValue)
		    {
				// end of stripe not right border 
				bInVop=TRUE;
				if(iLeftColour==invalidColour)
					iLeftColour=ppxlcTexture[iX];
				else
					iLeftColour=(iLeftColour+ppxlcTexture[iX]+1)>>1;
				
				// fill stripe 
				for(iJ=iLeftX;iJ<iX;iJ++)
					ppxlcTexture[iJ]=(PixelC)iLeftColour;
		    }
		}

        if(bInVop==FALSE)
		{
			// end of stripe at right border 
			if(iLeftX==0)
			{
				// blank stripe so mark
				m_pbEmptyRowArray[iY]=TRUE;
				bEmptyRow=TRUE;
			}
			else
	        {
				// fill trailing stripe 
	            for(iJ=iLeftX;iJ<iX;iJ++)
					ppxlcTexture[iJ]=(PixelC)iLeftColour;	      
	        }
 	    }
    }

	// fill remaining holes 
	if(bEmptyRow)
	{
		ppxlcTexture=ppxlcTextureBase;
		PixelC *ppxlcUpperRow = NULL;
		for(iY=0;iY<(CoordI)uiBlkSize;iY++,ppxlcTexture+=uiStride)
			if(!m_pbEmptyRowArray[iY])
				ppxlcUpperRow = ppxlcTexture;
			else
			{
				// empty row, find lower row
				PixelC *ppxlcLowerRow = ppxlcTexture+uiStride;
				CoordI iYY;
				for(iYY=iY+1;iYY<(CoordI)uiBlkSize;iYY++,ppxlcLowerRow+=uiStride)
					if(!m_pbEmptyRowArray[iYY])
						break;
				if(iYY<(CoordI)uiBlkSize)
				{
					if(ppxlcUpperRow==NULL)
					{
						// just lower row
						for(;ppxlcTexture<ppxlcLowerRow;ppxlcTexture+=uiStride)
							memcpy(ppxlcTexture,ppxlcLowerRow,uiBlkSize*iUnit);
					}
					else
					{
						// lower row, upper row
						for(;ppxlcTexture<ppxlcLowerRow;ppxlcTexture+=uiStride)
							for(iX=0;iX<(CoordI)uiBlkSize;iX++)
								ppxlcTexture[iX]=
									(ppxlcUpperRow[iX]+ppxlcLowerRow[iX]+1)>>1;
					}
				}
				else
				{
					// just upper row
					assert(ppxlcUpperRow!=NULL);
					for(iYY=iY;iYY<(CoordI)uiBlkSize;iYY++,ppxlcTexture+=uiStride)
						memcpy(ppxlcTexture,ppxlcUpperRow,uiBlkSize*iUnit);
				}
				iY=iYY-1;
				ppxlcTexture -= uiStride;
			}	
	}
}

Void CVideoObject::padNeighborTranspMBs (
	CoordI xb, CoordI yb,
	CMBMode* pmbmd,
	PixelC* ppxlcY, PixelC* ppxlcU, PixelC* ppxlcV, PixelC **pppxlcA
)
{
	if (xb > 0) {
		if ((pmbmd - 1)->m_rgTranspStatus [0] == ALL) {
			if (!((pmbmd - 1)->m_bPadded)) {
				mcPadLeftMB (ppxlcY, ppxlcU, ppxlcV, pppxlcA);
				(pmbmd - 1)->m_bPadded = TRUE;
			}
		}
	}
	if (yb > 0) {
		if ((pmbmd - m_iNumMBX)->m_rgTranspStatus [0] == ALL) {
			if (!((pmbmd - m_iNumMBX)->m_bPadded)) {
				mcPadTopMB (ppxlcY, ppxlcU, ppxlcV, pppxlcA);
				(pmbmd - m_iNumMBX)->m_bPadded = TRUE;
			}
		}
	}
}

Void CVideoObject::padCurrAndTopTranspMBFromNeighbor (
	CoordI xb, CoordI yb,
	CMBMode* pmbmd,
	PixelC* ppxlcY, PixelC* ppxlcU, PixelC* ppxlcV, PixelC** pppxlcA
)
{
	if (xb > 0) {
		if ((pmbmd - 1)->m_rgTranspStatus [0] != ALL) {
			mcPadCurrMBFromLeft (ppxlcY, ppxlcU, ppxlcV, pppxlcA);
			pmbmd->m_bPadded = TRUE;
		}
	}
	if (yb > 0) {
		if ((pmbmd - m_iNumMBX)->m_rgTranspStatus [0] != ALL) {
			if (!(pmbmd->m_bPadded)) {
				mcPadCurrMBFromTop (ppxlcY, ppxlcU, ppxlcV, pppxlcA);
				pmbmd->m_bPadded = TRUE;
			}
		}
		else
			if (!(pmbmd - m_iNumMBX)->m_bPadded)
				mcSetTopMBGray (ppxlcY, ppxlcU, ppxlcV, pppxlcA);
	}
	if(yb == m_iNumMBY-1)
	{
		if(xb > 0 && (pmbmd-1)->m_rgTranspStatus [0] == ALL && !((pmbmd-1)->m_bPadded))
			mcSetLeftMBGray(ppxlcY, ppxlcU, ppxlcV, pppxlcA);
		if(xb == m_iNumMBX-1 && !(pmbmd->m_bPadded))
			mcSetCurrMBGray(ppxlcY, ppxlcU, ppxlcV, pppxlcA);
	}
}


Void CVideoObject::mcPadLeftMB (PixelC* ppxlcMBY, PixelC* ppxlcMBU, PixelC* ppxlcMBV, PixelC** pppxlcMBA)
{
	UInt iy;
	for (iy = 0; iy < BLOCK_SIZE; iy++) {
		pxlcmemset (ppxlcMBY - MB_SIZE, *ppxlcMBY, MB_SIZE);
		pxlcmemset (ppxlcMBU - BLOCK_SIZE, *ppxlcMBU, BLOCK_SIZE);
		pxlcmemset (ppxlcMBV - BLOCK_SIZE, *ppxlcMBV, BLOCK_SIZE);
		ppxlcMBY += m_iFrameWidthY;
		ppxlcMBU += m_iFrameWidthUV;
		ppxlcMBV += m_iFrameWidthUV;
	
		pxlcmemset (ppxlcMBY - MB_SIZE, *ppxlcMBY, MB_SIZE);
		ppxlcMBY += m_iFrameWidthY;
	}
	if(m_volmd.fAUsage == EIGHT_BIT)
    for(Int iAuxComp=0;iAuxComp<m_volmd.iAuxCompCount;iAuxComp++) { // MAC (SB) 29-Nov-99
      PixelC* ppxlcMBA = pppxlcMBA[iAuxComp];
      for (iy = 0; iy < BLOCK_SIZE; iy++) {
        pxlcmemset (ppxlcMBA - MB_SIZE, *ppxlcMBA, MB_SIZE);
        ppxlcMBA += m_iFrameWidthY;
        pxlcmemset (ppxlcMBA - MB_SIZE, *ppxlcMBA, MB_SIZE);
        ppxlcMBA += m_iFrameWidthY;
      }
    }
}

Void CVideoObject::mcPadTopMB (PixelC* ppxlcMBY, PixelC* ppxlcMBU, PixelC* ppxlcMBV, PixelC** pppxlcMBA)
{
	UInt ix, iy;
	for (ix = 0; ix < MB_SIZE; ix++) {
		PixelC* ppxlcYCol = ppxlcMBY;
		for (iy = 0; iy < MB_SIZE; iy++) {
			ppxlcYCol -= m_iFrameWidthY;
			*ppxlcYCol = *ppxlcMBY;
		}
		ppxlcMBY++;
	}

	for (ix = 0; ix < BLOCK_SIZE; ix++) {
		PixelC* ppxlcUCol = ppxlcMBU;
		PixelC* ppxlcVCol = ppxlcMBV;
		for (iy = 0; iy < BLOCK_SIZE; iy++) {
			ppxlcUCol -= m_iFrameWidthUV;
			ppxlcVCol -= m_iFrameWidthUV;
			*ppxlcUCol = *ppxlcMBU;
			*ppxlcVCol = *ppxlcMBV;
		}
		ppxlcMBU++;
		ppxlcMBV++;
	}
	if(m_volmd.fAUsage == EIGHT_BIT)
    for(Int iAuxComp=0;iAuxComp<m_volmd.iAuxCompCount;iAuxComp++) { // MAC (SB) 29-Nov-99
      PixelC* ppxlcMBA = pppxlcMBA[iAuxComp];
      for (ix = 0; ix < MB_SIZE; ix++) {
        PixelC* ppxlcACol = ppxlcMBA;
        for (iy = 0; iy < MB_SIZE; iy++) {
          ppxlcACol -= m_iFrameWidthY;
          *ppxlcACol = *ppxlcMBA;
        }
        ppxlcMBA++;
      }
    }
}

Void CVideoObject::mcPadCurrMBFromLeft (PixelC* ppxlcMBY,
                                        PixelC* ppxlcMBU,
                                        PixelC* ppxlcMBV,
                                        PixelC** pppxlcMBA)
{
	UInt iy;
	for (iy = 0; iy < BLOCK_SIZE; iy++) {
		pxlcmemset (ppxlcMBY, *(ppxlcMBY - 1), MB_SIZE);
		pxlcmemset (ppxlcMBU, *(ppxlcMBU - 1), BLOCK_SIZE);
		pxlcmemset (ppxlcMBV, *(ppxlcMBV - 1), BLOCK_SIZE);
		ppxlcMBY += m_iFrameWidthY;
		ppxlcMBU += m_iFrameWidthUV;
		ppxlcMBV += m_iFrameWidthUV;
	
		pxlcmemset (ppxlcMBY, *(ppxlcMBY - 1), MB_SIZE);
		ppxlcMBY += m_iFrameWidthY;
	}
  if(m_volmd.fAUsage == EIGHT_BIT)
    for(Int iAuxComp=0;iAuxComp<m_volmd.iAuxCompCount;iAuxComp++) { // MAC (SB) 29-Nov-99
      PixelC* ppxlcMBA = pppxlcMBA[iAuxComp];
      for (iy = 0; iy < BLOCK_SIZE; iy++) {
        pxlcmemset (ppxlcMBA, *(ppxlcMBA - 1), MB_SIZE);
        ppxlcMBA += m_iFrameWidthY;
        pxlcmemset (ppxlcMBA, *(ppxlcMBA - 1), MB_SIZE);
        ppxlcMBA += m_iFrameWidthY;
      }
    }
}

Void CVideoObject::mcPadCurrMBFromTop (PixelC* ppxlcMBY, PixelC* ppxlcMBU, PixelC* ppxlcMBV, PixelC** pppxlcMBA)
{
	UInt ix, iy;
	for (ix = 0; ix < MB_SIZE; ix++) {
		PixelC* ppxlcYCol = ppxlcMBY;
		PixelC pxlcY = *(ppxlcMBY - m_iFrameWidthY);
		for (iy = 0; iy < MB_SIZE; iy++) {
			*ppxlcYCol = pxlcY;
			ppxlcYCol += m_iFrameWidthY;
		}
		ppxlcMBY++;
	}

	for (ix = 0; ix < BLOCK_SIZE; ix++) {
		PixelC* ppxlcUCol = ppxlcMBU;
		PixelC* ppxlcVCol = ppxlcMBV;
		PixelC pxlcU = *(ppxlcMBU - m_iFrameWidthUV);
		PixelC pxlcV = *(ppxlcMBV - m_iFrameWidthUV);
		for (iy = 0; iy < BLOCK_SIZE; iy++) {
			*ppxlcUCol = pxlcU;
			*ppxlcVCol = pxlcV;
			ppxlcUCol += m_iFrameWidthUV;
			ppxlcVCol += m_iFrameWidthUV;
		}
		ppxlcMBU++;
		ppxlcMBV++;
	}
	if(m_volmd.fAUsage == EIGHT_BIT)
    for(Int iAuxComp=0;iAuxComp<m_volmd.iAuxCompCount;iAuxComp++) { // MAC (SB) 29-Nov-99
      PixelC* ppxlcMBA = pppxlcMBA[iAuxComp];
      for (ix = 0; ix < MB_SIZE; ix++) {
        PixelC* ppxlcACol = ppxlcMBA;
        PixelC pxlcA = *(ppxlcMBA - m_iFrameWidthY);
        for (iy = 0; iy < MB_SIZE; iy++) {
          *ppxlcACol = pxlcA;
          ppxlcACol += m_iFrameWidthY;
        }
        ppxlcMBA++;
      }
    }
}

Void CVideoObject::mcSetTopMBGray (PixelC* ppxlcMBY, PixelC* ppxlcMBU, PixelC* ppxlcMBV, PixelC** pppxlcMBA)
{
	PixelC pxlcGrayVal = 128;
	if(m_volmd.bNot8Bit)
		pxlcGrayVal = 1<<(m_volmd.nBits-1);

	UInt iy;
	for (iy = 0; iy < BLOCK_SIZE; iy++) {
		ppxlcMBY -= m_iFrameWidthY;
		ppxlcMBU -= m_iFrameWidthUV;
		ppxlcMBV -= m_iFrameWidthUV;
		pxlcmemset (ppxlcMBY, pxlcGrayVal, MB_SIZE);
		pxlcmemset (ppxlcMBU, pxlcGrayVal, BLOCK_SIZE);
		pxlcmemset (ppxlcMBV, pxlcGrayVal, BLOCK_SIZE);

		ppxlcMBY -= m_iFrameWidthY;
		pxlcmemset (ppxlcMBY, pxlcGrayVal, MB_SIZE);
	}
	if(m_volmd.fAUsage == EIGHT_BIT)
    for(Int iAuxComp=0;iAuxComp<m_volmd.iAuxCompCount;iAuxComp++) { // MAC (SB) 29-Nov-99
      PixelC* ppxlcMBA = pppxlcMBA[iAuxComp];
      for (iy = 0; iy < BLOCK_SIZE; iy++) {
        ppxlcMBA -= m_iFrameWidthY;
        pxlcmemset (ppxlcMBA, pxlcGrayVal, MB_SIZE);
        ppxlcMBA -= m_iFrameWidthY;
        pxlcmemset (ppxlcMBA, pxlcGrayVal, MB_SIZE);
      }
    }
}

Void CVideoObject::mcSetLeftMBGray (PixelC* ppxlcMBY, PixelC* ppxlcMBU, PixelC* ppxlcMBV, PixelC** pppxlcMBA)
{
  PixelC** pppxlcMBA_tmp = new PixelC* [m_volmd.iAuxCompCount];
  for(Int iAuxComp=0;iAuxComp<m_volmd.iAuxCompCount;iAuxComp++) // MAC (SB) 29-Nov-99
    pppxlcMBA_tmp[iAuxComp] = pppxlcMBA[iAuxComp]-MB_SIZE;

	mcSetCurrMBGray(
		ppxlcMBY-MB_SIZE,
		ppxlcMBU-BLOCK_SIZE,
		ppxlcMBV-BLOCK_SIZE,
		(m_volmd.fAUsage == EIGHT_BIT) ? pppxlcMBA_tmp : (PixelC **)NULL
	);
  delete [] pppxlcMBA_tmp;
}

Void CVideoObject::mcSetCurrMBGray (PixelC* ppxlcMBY, PixelC* ppxlcMBU, PixelC* ppxlcMBV, PixelC** pppxlcMBA)
{
	PixelC pxlcGrayVal = 128;
	if(m_volmd.bNot8Bit)
		pxlcGrayVal = 1<<(m_volmd.nBits-1);
	UInt iy;
	for (iy = 0; iy < BLOCK_SIZE; iy++) {
		pxlcmemset (ppxlcMBY, pxlcGrayVal, MB_SIZE);
		pxlcmemset (ppxlcMBU, pxlcGrayVal, BLOCK_SIZE);
		pxlcmemset (ppxlcMBV, pxlcGrayVal, BLOCK_SIZE);
		ppxlcMBY += m_iFrameWidthY;
		ppxlcMBU += m_iFrameWidthUV;
		ppxlcMBV += m_iFrameWidthUV;

		pxlcmemset (ppxlcMBY, pxlcGrayVal, MB_SIZE);
		ppxlcMBY += m_iFrameWidthY;
	}
	if(m_volmd.fAUsage == EIGHT_BIT)
    for(Int iAuxComp=0;iAuxComp<m_volmd.iAuxCompCount;iAuxComp++) { // MAC (SB) 29-Nov-99
      PixelC* ppxlcMBA = pppxlcMBA[iAuxComp];
      for (iy = 0; iy < BLOCK_SIZE; iy++) {
        pxlcmemset (ppxlcMBA, pxlcGrayVal, MB_SIZE);
        ppxlcMBA += m_iFrameWidthY;
        pxlcmemset (ppxlcMBA, pxlcGrayVal, MB_SIZE);
        ppxlcMBA += m_iFrameWidthY;
      }
    }
}

// Added for field based MC padding by Hyundai(1998-5-9)

Void CVideoObject::fieldBasedMCPadding (CMBMode* pmbmd, CVOPU8YUVBA* pvopcCurrQ)
{
  Int     iMBX, iMBY;
  
  PixelC* ppxlcCurrY  = (PixelC*) pvopcCurrQ->pixelsY () + m_iStartInRefToCurrRctY;
  PixelC* ppxlcCurrU  = (PixelC*) pvopcCurrQ->pixelsU () + m_iStartInRefToCurrRctUV;
  PixelC* ppxlcCurrV  = (PixelC*) pvopcCurrQ->pixelsV () + m_iStartInRefToCurrRctUV;
  PixelC* ppxlcCurrBY = (PixelC*) pvopcCurrQ->pixelsBY () + m_iStartInRefToCurrRctY;
  PixelC* ppxlcCurrBUV = (PixelC*) pvopcCurrQ->pixelsBUV () + m_iStartInRefToCurrRctUV;
  // 12.22.98 begin of changes
	PixelC** pppxlcCurrMBA = NULL;
  if (m_volmd.fAUsage == EIGHT_BIT) {
    pppxlcCurrMBA = new PixelC* [m_volmd.iAuxCompCount];
  } 
  // 12.22.98 end of changes
  for (iMBY = 0; iMBY < m_iNumMBY; iMBY++) {
    PixelC* ppxlcCurrMBY  = ppxlcCurrY;
    PixelC* ppxlcCurrMBU  = ppxlcCurrU;
    PixelC* ppxlcCurrMBV  = ppxlcCurrV;
    PixelC* ppxlcCurrMBBY = ppxlcCurrBY;
    PixelC* ppxlcCurrMBBUV = ppxlcCurrBUV;
//    PixelC* ppxlcCurrMBA;
    // 12.22.98 begin of changes
//    if (m_volmd.fAUsage == EIGHT_BIT)
//      ppxlcCurrMBA  = ppxlcCurrA; 
    // 12.22.98 end of changes
    for (iMBX = 0; iMBX < m_iNumMBX; iMBX++) {
      memset(pmbmd->m_rgbFieldPadded, 0, 5*sizeof(Bool));
      fieldBasedDownSampleBY (ppxlcCurrMBBY, ppxlcCurrMBBUV);
      decideFieldTransparencyStatus (pmbmd, ppxlcCurrMBBY, ppxlcCurrMBBUV);
      for(Int iAuxComp=0;iAuxComp<m_volmd.iAuxCompCount;iAuxComp++) { // MAC (SB) 29-Nov-99
        pppxlcCurrMBA[iAuxComp] = (PixelC*) pvopcCurrQ->pixelsA (iAuxComp) + m_iStartInRefToCurrRctY;
        pppxlcCurrMBA[iAuxComp] += MB_SIZE * iMBX + m_iFrameWidthYxMBSize * iMBY;
      }
      mcPadCurrAndNeighborsMBFields (iMBX, iMBY, pmbmd, ppxlcCurrMBY, ppxlcCurrMBU, ppxlcCurrMBV, ppxlcCurrMBBY, ppxlcCurrMBBUV,pppxlcCurrMBA);
      ppxlcCurrMBY += MB_SIZE;
      ppxlcCurrMBU += BLOCK_SIZE;
      ppxlcCurrMBV += BLOCK_SIZE;
      ppxlcCurrMBBY += MB_SIZE;
      ppxlcCurrMBBUV += BLOCK_SIZE;
      // 12.22.98 begin of changes
//      if (m_volmd.fAUsage == EIGHT_BIT)
//        ppxlcCurrMBA  += MB_SIZE; 
      // 12.22.98 end of changes
      pmbmd++;
    }
    ppxlcCurrY += m_iFrameWidthYxMBSize;
    ppxlcCurrU += m_iFrameWidthUVxBlkSize;
    ppxlcCurrV += m_iFrameWidthUVxBlkSize;
    ppxlcCurrBY += m_iFrameWidthYxMBSize;
    ppxlcCurrBUV += m_iFrameWidthUVxBlkSize;
    // 12.22.98 begin of changes
//    if (m_volmd.fAUsage == EIGHT_BIT)
//      ppxlcCurrA  += m_iFrameWidthYxMBSize; 
    // 12.22.98 end of changes
  }
  if (m_volmd.fAUsage == EIGHT_BIT) {
    delete [] pppxlcCurrMBA;
  } 
}

Void CVideoObject::mcPadCurrAndNeighborsMBFields (
        Int     iMBX,
        Int     iMBY,
        CMBMode *pmbmd,
        PixelC* ppxlcRefMBY,
        PixelC* ppxlcRefMBU,
        PixelC* ppxlcRefMBV,
        PixelC* ppxlcRefMBBY,
        PixelC* ppxlcRefMBBUV,
		    PixelC** pppxlcRefMBA
)
{
  // 2.9.99 begin of changes.  X. Chen, 
		if (m_volmd.fAUsage == EIGHT_BIT) {
      for(Int iAuxComp=0;iAuxComp<m_volmd.iAuxCompCount;iAuxComp++)  // MAC (SB) 29-Nov-99
        mcPadFieldsCurr (iMBX, iMBY, pmbmd, MB_FIELDY, ppxlcRefMBY, pppxlcRefMBA[iAuxComp], ppxlcRefMBBY, MB_SIZE, m_iFrameWidthY);
    } else
      // 2.9.99 end of changes
      mcPadFieldsCurr (iMBX, iMBY, pmbmd, MB_FIELDY, ppxlcRefMBY, NULL, ppxlcRefMBBY, MB_SIZE, m_iFrameWidthY);
    mcPadFieldsCurr (iMBX, iMBY, pmbmd, MB_FIELDC, ppxlcRefMBU, ppxlcRefMBV, ppxlcRefMBBUV, BLOCK_SIZE, m_iFrameWidthUV);
}

Void CVideoObject::mcPadFieldsCurr (
        Int     iMBX,
        Int     iMBY,
        CMBMode *pmbmd,
        Int     mode,
        PixelC  *ppxlcCurrMB1,
        PixelC  *ppxlcCurrMB2,
        PixelC  *ppxlcCurrMBB,
        Int     uiBlkXSize,
        Int     uiStride
)
{
        PixelC  *ppxlcCurrTopField1 = ppxlcCurrMB1,
                *ppxlcCurrBotField1 = ppxlcCurrMB1 + uiStride,
                *ppxlcCurrTopField2 = ((ppxlcCurrMB2) ? ppxlcCurrMB2 : NULL),
                *ppxlcCurrBotField2 = ((ppxlcCurrMB2) ? ppxlcCurrMB2 + uiStride : NULL),
                *ppxlcCurrTopFieldB = ppxlcCurrMBB,
                *ppxlcCurrBotFieldB = ppxlcCurrMBB + uiStride;
 
        if (pmbmd->m_rgFieldTranspStatus [mode] != ALL) {       // Top Field MC Padding
                if (pmbmd->m_rgFieldTranspStatus [mode] == PARTIAL) {
                        mcPadOneField (ppxlcCurrTopField1, ppxlcCurrTopFieldB, uiBlkXSize, uiStride);
                        if (ppxlcCurrTopField2)
                                mcPadOneField (ppxlcCurrTopField2, ppxlcCurrTopFieldB, uiBlkXSize, uiStride);
                }
                padNeighborTranspMBFields (iMBX, iMBY, pmbmd, mode, ppxlcCurrTopField1, ppxlcCurrTopField2, uiBlkXSize, uiStride);
        } else  padCurrAndTopTranspMBFieldsFromNeighbor (iMBX, iMBY, pmbmd, mode, ppxlcCurrTopField1, ppxlcCurrTopField2, uiBlkXSize, uiStride);
        if (pmbmd->m_rgFieldTranspStatus [mode+1] != ALL) {     // Bottom Field MC Padding
                if (pmbmd->m_rgFieldTranspStatus [mode+1] == PARTIAL) {
                        mcPadOneField (ppxlcCurrBotField1, ppxlcCurrBotFieldB, uiBlkXSize, uiStride);
                        if (ppxlcCurrBotField2)
                                mcPadOneField (ppxlcCurrBotField2, ppxlcCurrBotFieldB, uiBlkXSize, uiStride);
                }
                padNeighborTranspMBFields (iMBX, iMBY, pmbmd, mode+1, ppxlcCurrBotField1, ppxlcCurrBotField2, uiBlkXSize, uiStride);
        } else  padCurrAndTopTranspMBFieldsFromNeighbor (iMBX, iMBY, pmbmd, mode+1, ppxlcCurrBotField1, ppxlcCurrBotField2, uiBlkXSize, uiStride);
}

Void CVideoObject::mcPadOneField (
        PixelC          *ppxlcTextureBase,
        const PixelC    *ppxlcAlphaBase,
        Int             uiBlkXSize,
        Int             uiStride
)
{
        Int     iUnit = sizeof(PixelC);
        Int     uiBlkYSize = uiBlkXSize/2;
        Int     iFieldSkip = 2*uiStride;
        CoordI  iX,iY,iJ,iLeftX = 0;
        Bool    bEmptyRow = FALSE;
        Bool    bInVop;
        Int     iLeftColour;
         
 
        PixelC  *ppxlcTexture = ppxlcTextureBase;
        for (iY=0;iY<(CoordI)uiBlkYSize;iY++, ppxlcTexture+=iFieldSkip) {
                bInVop = TRUE;
                iLeftColour = invalidColour;
                m_pbEmptyRowArray[iY]=0;
                const PixelC *ppxlcAlpha = ppxlcAlphaBase;
                for(iX=0;iX<(CoordI)uiBlkXSize;iX++,ppxlcAlpha++) {
                        if(bInVop==TRUE && *ppxlcAlpha==transpValue) {
                                bInVop=FALSE;
                                iLeftX=iX;
                                if(iX>0)
                                        iLeftColour=ppxlcTexture[iLeftX-1];
                        }
                        else if(bInVop==FALSE && *ppxlcAlpha!=transpValue) {
                                bInVop=TRUE;
                                if(iLeftColour==invalidColour)
                                        iLeftColour=ppxlcTexture[iX];
                                else    iLeftColour=(iLeftColour+ppxlcTexture[iX]+1)>>1;
                                for(iJ=iLeftX;iJ<iX;iJ++)
                                        ppxlcTexture[iJ]=(PixelC)iLeftColour;
                        }                 
                }
                ppxlcAlphaBase += iFieldSkip;
                if(bInVop==FALSE) {
                        if(iLeftX==0) {
                                m_pbEmptyRowArray[iY]=TRUE;
                                bEmptyRow=TRUE;
                        }
                        else {
                                for(iJ=iLeftX;iJ<iX;iJ++)
                                        ppxlcTexture[iJ]=(PixelC)iLeftColour;
                        }                 
                }
        }
        if(bEmptyRow) {
                ppxlcTexture=ppxlcTextureBase;
                PixelC  *ppxlcUpperRow = NULL;
                for(iY=0;iY<(CoordI)uiBlkYSize;iY++,ppxlcTexture+=iFieldSkip)
                        if(!m_pbEmptyRowArray[iY])
                                ppxlcUpperRow = ppxlcTexture;
                        else {
                                PixelC *ppxlcLowerRow = ppxlcTexture+iFieldSkip;
                                CoordI iYY;
                                for(iYY=iY+1;iYY<(CoordI)uiBlkYSize;iYY++,ppxlcLowerRow+=iFieldSkip)
                                        if(!m_pbEmptyRowArray[iYY])
                                                break;
                                if(iYY<(CoordI)uiBlkYSize) {
                                        if(ppxlcUpperRow==NULL) {
                                                for(;ppxlcTexture<ppxlcLowerRow;ppxlcTexture+=iFieldSkip)
                                                        memcpy(ppxlcTexture,ppxlcLowerRow,uiBlkXSize*iUnit);
                                        }
                                        else {
                                                for(;ppxlcTexture<ppxlcLowerRow;ppxlcTexture+=iFieldSkip)
                                                        for(iX=0;iX<(CoordI)uiBlkXSize;iX++)
                                                                ppxlcTexture[iX]= (ppxlcUpperRow[iX]+ppxlcLowerRow[iX]+1)>>1;
                                        }
                                }         
                                else {
                                        assert(ppxlcUpperRow!=NULL);
                                        for(iYY=iY;iYY<(CoordI)uiBlkYSize;iYY++,ppxlcTexture+=iFieldSkip)
                                                memcpy(ppxlcTexture,ppxlcUpperRow,uiBlkXSize*iUnit);
                                }
                                iY=iYY-1;
                                ppxlcTexture -= iFieldSkip;
                        }
        }
}

Void CVideoObject::padNeighborTranspMBFields (
        CoordI  xb,
        CoordI  yb,
        CMBMode *pmbmd,
        Int     mode,
        PixelC  *ppxlcMBField1,
        PixelC  *ppxlcMBField2,
        Int     uiBlkXSize,
        Int     uiStride
)
{
        CMBMode* pmbmdLeft = pmbmd - 1;
        CMBMode* pmbmdAbov = pmbmd - m_iNumMBX;
 
        if (xb > 0) {
                if (pmbmdLeft->m_rgFieldTranspStatus [mode] == ALL)
                        if (!(pmbmdLeft->m_rgbFieldPadded [mode])) {
                                mcPadLeftMBFields (ppxlcMBField1, ppxlcMBField2, uiBlkXSize, uiStride);
                                pmbmdLeft->m_rgbFieldPadded[mode] = TRUE;
                        }
        }
        if (yb > 0) {
                if (pmbmdAbov->m_rgFieldTranspStatus [mode] == ALL)
                        if (!(pmbmdAbov->m_rgbFieldPadded [mode])) {
                                mcPadTopMBFields (ppxlcMBField1, ppxlcMBField2, uiBlkXSize, uiStride);
                                pmbmdAbov->m_rgbFieldPadded[mode] = TRUE;
                        }
        }        
}

Void CVideoObject::padCurrAndTopTranspMBFieldsFromNeighbor (
        CoordI  xb,
        CoordI  yb,
        CMBMode *pmbmd,
        Int     mode,
        PixelC* ppxlcMBField1,
        PixelC* ppxlcMBField2,
        Int     uiBlkXSize,
        Int     uiStride
)
{
        CMBMode* pmbmdLeft = pmbmd - 1;
        CMBMode* pmbmdAbov = pmbmd - m_iNumMBX;
 
        if (xb > 0) {
                if (pmbmdLeft->m_rgFieldTranspStatus [mode] != ALL) {
                        mcPadCurrMBFieldsFromLeft (ppxlcMBField1, ppxlcMBField2, uiBlkXSize, uiStride);
                        pmbmd->m_rgbFieldPadded [mode] = TRUE;
                }
        }
        if (yb > 0) {
                if (pmbmdAbov->m_rgFieldTranspStatus [mode] != ALL) {
                        if (!(pmbmd->m_rgbFieldPadded [mode])) {
                                mcPadCurrMBFieldsFromTop (ppxlcMBField1, ppxlcMBField2, uiBlkXSize, uiStride);
                                pmbmd->m_rgbFieldPadded [mode] = TRUE;
                        }
                } else {
                        if (!(pmbmdAbov->m_rgbFieldPadded [mode]))
                                mcSetTopMBFieldsGray (ppxlcMBField1, ppxlcMBField2, uiBlkXSize, uiStride);
                }
        }
        if(yb == m_iNumMBY-1) {
                if(xb > 0 && pmbmdLeft->m_rgFieldTranspStatus [mode] == ALL && !(pmbmdLeft->m_rgbFieldPadded [mode]))
                        mcSetLeftMBFieldsGray (ppxlcMBField1, ppxlcMBField2, uiBlkXSize, uiStride);
                if(xb == m_iNumMBX-1 && !(pmbmd->m_rgbFieldPadded [mode]))
                        mcSetCurrMBFieldsGray (ppxlcMBField1, ppxlcMBField2, uiBlkXSize, uiStride);
        }
}

Void CVideoObject::mcPadLeftMBFields (
        PixelC* ppxlcMBField1,
        PixelC* ppxlcMBField2,
        Int     uiBlkXSize,
        Int     uiStride
)
{
        UInt    iy, uiBlkYSize = uiBlkXSize/2;
 
        for (iy = 0; iy < uiBlkYSize; iy++) {
                pxlcmemset (ppxlcMBField1 - uiBlkXSize, *ppxlcMBField1, uiBlkXSize);
                ppxlcMBField1 += 2*uiStride;
        }
        if (!ppxlcMBField2) return;
        for (iy = 0; iy < uiBlkYSize; iy++) {
                pxlcmemset (ppxlcMBField2 - uiBlkXSize, *ppxlcMBField2, uiBlkXSize);
                ppxlcMBField2 += 2*uiStride;
        }
}
 
Void CVideoObject::mcPadTopMBFields (
        PixelC* ppxlcMBField1,
        PixelC* ppxlcMBField2,
        Int     iBlkXSize,
        Int     iStride
)
{
        Int    ix, iy, iBlkYSize = iBlkXSize/2;
 
        for (ix = 0; ix < iBlkXSize; ix++) {
                PixelC* ppxlcYCol = ppxlcMBField1;
                for (iy = 0; iy < iBlkYSize; iy++) {
                        ppxlcYCol -= 2*iStride;
                        *ppxlcYCol = *ppxlcMBField1;
                }
                ppxlcMBField1++;
        }
        if (!ppxlcMBField2) return;
        for (ix = 0; ix < iBlkXSize; ix++) {
                PixelC* ppxlcACol = ppxlcMBField2;
                for (iy = 0; iy < iBlkYSize; iy++) {
                        ppxlcACol -= 2*iStride;
                        *ppxlcACol = *ppxlcMBField2;
                }
                ppxlcMBField2++;
        }
}

Void CVideoObject::mcPadCurrMBFieldsFromLeft (
        PixelC* ppxlcMBField1,
        PixelC* ppxlcMBField2,
        Int     uiBlkXSize,
        Int     uiStride)
{
        UInt iy, uiBlkYSize = uiBlkXSize/2;

        for (iy = 0; iy < uiBlkYSize; iy++) {
                pxlcmemset (ppxlcMBField1, *(ppxlcMBField1 - 1), uiBlkXSize);
                ppxlcMBField1 += 2*uiStride;
        }
        if (!ppxlcMBField2) return;
        for (iy = 0; iy < uiBlkYSize; iy++) {
                pxlcmemset (ppxlcMBField2, *(ppxlcMBField2 - 1), uiBlkXSize);
                ppxlcMBField2 += 2*uiStride;
        }
}
 
Void CVideoObject::mcPadCurrMBFieldsFromTop (
        PixelC* ppxlcMBField1,
        PixelC* ppxlcMBField2,
        Int     iBlkXSize,
        Int     iStride)
{
        Int    ix, iy, iBlkYSize = iBlkXSize/2;

        PixelC  *ppxlcSrcMBField = ppxlcMBField1-2*iStride;
        for (ix = 0; ix < iBlkXSize; ix++) {
                PixelC* ppxlcDstMBField = ppxlcSrcMBField;
                for (iy = 0; iy < iBlkYSize; iy++) {
                        ppxlcDstMBField += 2*iStride;
                        *ppxlcDstMBField = *ppxlcSrcMBField;
                }
                ppxlcSrcMBField++;
        }
        if(!ppxlcMBField2) return;
        ppxlcSrcMBField = ppxlcMBField2-2*iStride;
        for (ix = 0; ix < iBlkXSize; ix++) {
                PixelC* ppxlcDstMBField = ppxlcSrcMBField;
                for (iy = 0; iy < iBlkYSize; iy++) {
                        ppxlcDstMBField += 2*iStride;
                        *ppxlcDstMBField = *ppxlcSrcMBField;
                }
                ppxlcSrcMBField++;
        }
}

Void CVideoObject::mcSetTopMBFieldsGray (
        PixelC* ppxlcMBField1,
        PixelC* ppxlcMBField2,
        Int     uiBlkXSize,
        Int     uiStride
)
{
        UInt    iy, uiBlkYSize = uiBlkXSize/2;
		PixelC pxlcGrayVal = 128;
		if(m_volmd.bNot8Bit)
			pxlcGrayVal = 1<<(m_volmd.nBits-1);

        ppxlcMBField1 -= 2*uiStride;
        for (iy = 0; iy < uiBlkYSize; iy++) {
                pxlcmemset (ppxlcMBField1, pxlcGrayVal, uiBlkXSize);
                ppxlcMBField1 -= 2*uiStride;
        }
        if (!ppxlcMBField2) return;
        ppxlcMBField2 -= 2*uiStride;
        for (iy = 0; iy < uiBlkYSize; iy++) {
                pxlcmemset (ppxlcMBField2, pxlcGrayVal, uiBlkXSize);
                ppxlcMBField2 -= 2*uiStride;
        }
}
 
Void CVideoObject::mcSetLeftMBFieldsGray (
        PixelC* ppxlcMBField1,
        PixelC* ppxlcMBField2,
        Int     uiBlkXSize,
        Int     uiStride)
{
        UInt    iy, uiBlkYSize = uiBlkXSize/2;
		PixelC pxlcGrayVal = 128;
		if(m_volmd.bNot8Bit)
			pxlcGrayVal = 1<<(m_volmd.nBits-1);

	ppxlcMBField1 = ppxlcMBField1-uiBlkXSize;
        for (iy = 0; iy < uiBlkYSize; iy++) {
                pxlcmemset (ppxlcMBField1, pxlcGrayVal, uiBlkXSize);
                ppxlcMBField1 += 2*uiStride;
        }
        if (!ppxlcMBField2) return;
	ppxlcMBField2 = ppxlcMBField2-uiBlkXSize;
        for (iy = 0; iy < uiBlkYSize; iy++) {
                pxlcmemset (ppxlcMBField2, pxlcGrayVal, uiBlkXSize);
                ppxlcMBField2 += 2*uiStride;
        }
}
 
Void CVideoObject::mcSetCurrMBFieldsGray (
        PixelC* ppxlcMBField1,
        PixelC* ppxlcMBField2,
        Int     uiBlkXSize,
        Int     uiStride)
{
        UInt    iy, uiBlkYSize = uiBlkXSize/2;
		PixelC pxlcGrayVal = 128;
		if(m_volmd.bNot8Bit)
			pxlcGrayVal = 1<<(m_volmd.nBits-1);

        for (iy = 0; iy < uiBlkYSize; iy++) {
                pxlcmemset (ppxlcMBField1, pxlcGrayVal, uiBlkXSize);
                ppxlcMBField1 += 2*uiStride;
        }
        if (!ppxlcMBField2) return;
        for (iy = 0; iy < uiBlkYSize; iy++) {
                pxlcmemset (ppxlcMBField2, pxlcGrayVal, uiBlkXSize);
                ppxlcMBField2 += 2*uiStride;
        }
}
 
// End of HYUNDAI(1998-5-9)

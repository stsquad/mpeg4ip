/*************************************************************************

This software module was originally developed by 

	Yoshihiro Kikuchi (TOSHIBA CORPORATION)
	Takeshi Nagai (TOSHIBA CORPORATION)

    and edited by:

	Toshiaki Watanabe (TOSHIBA CORPORATION)

  in the course of development of the <MPEG-4 Video(ISO/IEC 14496-2)>. This
  software module is an implementation of a part of one or more <MPEG-4 Video
  (ISO/IEC 14496-2)> tools as specified by the <MPEG-4 Video(ISO/IEC 14496-2)
  >. ISO/IEC gives users of the <MPEG-4 Video(ISO/IEC 14496-2)> free license
  to this software module or modifications thereof for use in hardware or
  software products claiming conformance to the <MPEG-4 Video(ISO/IEC 14496-2
  )>. Those intending to use this software module in hardware or software
  products are advised that its use may infringe existing patents. The
  original developer of this software module and his/her company, the
  subsequent editors and their companies, and ISO/IEC have no liability for
  use of this software module or modifications thereof in an implementation.
  Copyright is not released for non <MPEG-4 Video(ISO/IEC 14496-2)>
  conforming products. TOSHIBA CORPORATION retains full right to use the code
  for his/her own purpose, assign or donate the code to a third party and to
  inhibit third parties from using the code for non <MPEG-4 Video(ISO/IEC
  14496-2)> conforming products. This copyright notice must be included in
  all copies or derivative works.
  Copyright (c)1997.

*************************************************************************/


//#include <stdlib.h>
#include <math.h>
#include "typeapi.h"
#include "codehead.h"
#include "mode.hpp"
#include "global.hpp"
#include "bitstrm.hpp"
#include "entropy.hpp"
#include "huffman.hpp"
#include "dct.hpp"
#include "vopses.hpp"
#include "vopsedec.hpp"

#ifdef __MFC_
#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

#define new DEBUG_NEW				   
#endif // __MFC_

Void CVideoObjectDecoder::decodeIntraRVLCTCOEF (Int* rgiCoefQ, Int iCoefStart, Int* rgiZigzag)
{
	Bool bIsLastRun = FALSE;
	Int  iRun = 0;
	Int	 iLevel = 0;
	Int  iCoef = iCoefStart;
	Long lIndex;
	while (!bIsLastRun) {
		lIndex = m_pentrdecSet->m_pentrdecDCTIntraRVLC->decodeSymbol();															
		if (lIndex != TCOEF_RVLC_ESCAPE)	{							// if Huffman
			decodeIntraRVLCtableIndex (lIndex, iLevel, iRun, bIsLastRun);
		}
		else	{
			decodeRVLCEscape (iLevel, iRun, bIsLastRun, g_rgiLMAXintra, g_rgiRMAXintra, 
					  m_pentrdecSet->m_pentrdecDCTIntraRVLC,/* (Void (CVideoObjectDecoder::*)(Int, Bool &, Int &, Int &)) */& CVideoObjectDecoder::decodeIntraRVLCtableIndex);
		}
		for (Int i = 0; i < iRun; i++)	{
			rgiCoefQ [rgiZigzag [iCoef]] = 0;
			iCoef++;
		}
		rgiCoefQ [rgiZigzag [iCoef]] = iLevel;			
		iCoef++;
	}															
	for (Int i = iCoef; i < BLOCK_SQUARE_SIZE; i++)			// fill the rest w/ zero
		rgiCoefQ [rgiZigzag [i]]  = 0;
}

Void CVideoObjectDecoder::decodeRVLCEscape (Int& iLevel, Int& iRun, Int& bIsLastRun, const Int* rgiLMAX, const Int* rgiRMAX, 
				   CEntropyDecoder* pentrdec, DECODE_TABLE_INDEX decodeVLCtableIndex)
{
	// for error resilience the code should check to see if escape coded tcoef can be
	// encoded using non-escape mode. if so, then it is an error case.

	Bool bFlagEscape = (Bool) m_pbitstrmIn->getBits (1);
	assert(bFlagEscape == TRUE);

	bIsLastRun = (Bool) m_pbitstrmIn->getBits (1);		
	iRun =	(Int) m_pbitstrmIn->getBits (NUMBITS_RVLC_ESC_RUN);			
	assert (iRun < BLOCK_SQUARE_SIZE);
	Int iLevelBits = 12; // = m_volmd.nBits;

	Int iMarker = m_pbitstrmIn->getBits (1);
	assert(iMarker == 1);
	iLevel = (Int) m_pbitstrmIn->getBits (iLevelBits - 1);
	iMarker = m_pbitstrmIn->getBits (1);
	assert(iMarker == 1);

	Long lIndex = m_pentrdecSet->m_pentrdecDCTIntraRVLC->decodeSymbol();															
	assert (lIndex == TCOEF_RVLC_ESCAPE);
	if((Bool) m_pbitstrmIn->getBits (1) == TRUE){
		iLevel = - iLevel;
	}
	assert (iLevel != 0);
}

Void CVideoObjectDecoder::decodeInterRVLCTCOEF (Int* rgiCoefQ, Int iCoefStart, Int* rgiZigzag)
{
	Bool bIsLastRun = FALSE;
	Int  iRun = 0;
	Int	 iLevel = 0;
	Int  iCoef = iCoefStart;
	Long lIndex;
	while (!bIsLastRun) {
		lIndex = m_pentrdecSet->m_pentrdecDCTRVLC->decodeSymbol();
		if (lIndex != TCOEF_RVLC_ESCAPE)	{							// if Huffman
			decodeInterRVLCtableIndex (lIndex, iLevel, iRun, bIsLastRun);
			assert (iRun < BLOCK_SQUARE_SIZE);
		}
		else
			decodeRVLCEscape (iLevel, iRun, bIsLastRun, g_rgiLMAXinter, g_rgiRMAXinter, 
					  m_pentrdecSet->m_pentrdecDCTRVLC, /* (Void (CVideoObjectDecoder::*)(Int, Bool &, Int &, Int &)) */& CVideoObjectDecoder::decodeInterRVLCtableIndex);
		for (Int i = 0; i < iRun; i++)	{
			rgiCoefQ [rgiZigzag [iCoef]] = 0;
			iCoef++;
		}
		rgiCoefQ [rgiZigzag [iCoef]] = iLevel;			
		iCoef++;
	}															
	for (Int i = iCoef; i < BLOCK_SQUARE_SIZE; i++)					// fill the rest w/ zero
		rgiCoefQ [rgiZigzag [i]]  = 0;
}

Void CVideoObjectDecoder::decodeIntraRVLCtableIndex  (Int iIndex, Int& iLevel, Int& iRun, Int& bIsLastRun)
{
	static Int iLevelMask = 0x0000001F;
	static Int iRunMask = 0x000007E0;
	static Int iLastRunMask = 0x00000800;
	iLevel = iLevelMask & grgiIntraRVLCYAVCLHashingTable [iIndex];
	iRun = (iRunMask & grgiIntraRVLCYAVCLHashingTable [iIndex]) >> 5;
	bIsLastRun = (iLastRunMask  & grgiIntraRVLCYAVCLHashingTable [iIndex]) >> 11;
	if (m_pentrdecSet->m_pentrdecDCTIntraRVLC->bitstream()->getBits (1) == TRUE)	// get signbit
		iLevel = -iLevel;
	assert (iRun < BLOCK_SQUARE_SIZE);
}

															
Void CVideoObjectDecoder::decodeInterRVLCtableIndex (Int   iIndex, Int& iLevel,		// return islastrun, run and level													 
													Int&  iRun, Bool& bIsLastRun)
{	
	static Int iLevelMask = 0x0000001F;
	static Int iRunMask = 0x000007E0;
	static Int iLastRunMask = 0x00000800;
	iLevel = iLevelMask & grgiInterRVLCYAVCLHashingTable [iIndex];
	iRun = (iRunMask & grgiInterRVLCYAVCLHashingTable [iIndex]) >> 5;
	bIsLastRun = (iLastRunMask  & grgiInterRVLCYAVCLHashingTable [iIndex]) >> 11;
	if (m_pentrdecSet->m_pentrdecDCTRVLC->bitstream()->getBits (1) == TRUE)	// get signbit
		iLevel = -iLevel;
	assert (iRun < BLOCK_SQUARE_SIZE);
}

/*************************************************************************

This software module was originally developed by 

	Ming-Chieh Lee (mingcl@microsoft.com), Microsoft Corporation
	Wei-ge Chen (wchen@microsoft.com), Microsoft Corporation
	Bruce Lin (blin@microsoft.com), Microsoft Corporation
	Chuang Gu (chuanggu@microsoft.com), Microsoft Corporation
	(date: March, 1996)

and edited by
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
assign or donate the code to a third party and to inhibit third parties from using the code for non MPEG-4 Video conforming products. 
This copyright notice must be included in all copies or derivative works. 

Copyright (c) 1996, 1997.

Module Name:
	
	bitstrm.cpp

Abstract:

	Classes for bitstream I/O.

Revision History:

*************************************************************************/

#include "typeapi.h"
#include "bitstrm.hpp"
#include <istream.h>
#include <ostream.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef __MFC_
#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

#define new DEBUG_NEW				   
#endif // __MFC_

UInt getbit (UInt data, UInt position, UInt num) // get the num-bit field of x starting from position p
{
	return ((data >> (position + 1 - num)) & ~(~0 << num));
}

Void print_bit (UInt x, UInt num, UInt startPos) // print num bits starting from startPos
{
	for (Int k = 0; k <= (Int) (num - startPos); k++) {
		UInt y = getbit (x, num, 1);
		printf ("%u ", y);
		x = x << 1;
	}
	printf ("\n");
}

Char CInBitStream::getBitsC (Int iNOfBits)
{
    assert (iNOfBits <= 8);
    assert (iNOfBits >= 0);
	return (Char)  getBits ((UInt) iNOfBits);
}

static unsigned int msk[33] =
{
  0x00000000, 0x00000001, 0x00000003, 0x00000007,
  0x0000000f, 0x0000001f, 0x0000003f, 0x0000007f,
  0x000000ff, 0x000001ff, 0x000003ff, 0x000007ff,
  0x00000fff, 0x00001fff, 0x00003fff, 0x00007fff,
  0x0000ffff, 0x0001ffff, 0x0003ffff, 0x0007ffff,
  0x000fffff, 0x001fffff, 0x003fffff, 0x007fffff,
  0x00ffffff, 0x01ffffff, 0x03ffffff, 0x07ffffff,
  0x0fffffff, 0x1fffffff, 0x3fffffff, 0x7fffffff,
  0xffffffff
};

UInt CInBitStream::getBits (UInt numBits)
{
  assert (numBits <= 32);
  if (numBits == 0) return 0;
	
  UInt retData;
  if (m_uNumOfBitsInBuffer >= numBits) {	// don't need to read from FILE
    m_uNumOfBitsInBuffer -= numBits;
    retData = m_chDecBuffer >> m_uNumOfBitsInBuffer;
    retData &= msk[numBits];
  } else {
    int nbits;
    nbits = numBits - m_uNumOfBitsInBuffer;
    retData = m_chDecBuffer << nbits;
    switch ((nbits - 1) / 8) {
    case 3:
      nbits -= 8;
      retData |= m_pInStream->get() << nbits;
      // fall through
    case 2:
      nbits -= 8;
      retData |= m_pInStream->get() << nbits;
    case 1:
      nbits -= 8;
      retData |= m_pInStream->get() << nbits;
    case 0:
      break;
    }
    m_chDecBuffer = m_pInStream->get();
    m_uNumOfBitsInBuffer = 8 - nbits;
    retData |= (m_chDecBuffer >> m_uNumOfBitsInBuffer) & msk[nbits];
  }
  return retData & msk[numBits];

}

Void CInBitStream::getBits (Char *pBits, Int lNOfBits)
{
    assert (lNOfBits <= 8);
    assert (lNOfBits >= 0);

    while(lNOfBits>0)
    {
        if(lNOfBits>8)
        {
            *pBits=(Char) getBitsC(8);
            lNOfBits-=8;
            pBits++;
        }
        else
        {
            *pBits=(Char) getBitsC ((UInt) lNOfBits);
            break;
        }
    }
}

Void CInBitStream::attach (CInByteStreamBase *inStream, Int iBitPosition)
{
    assert(iBitPosition<8);
    assert(iBitPosition>=0);
    m_iBitPosition=iBitPosition;
    m_pInStream=inStream;
    m_iBuffer=0x00ff&m_pInStream->peek();
}

Void CInBitStream::flush (Int nExtraBits)
{
//	Modified for error resilient mode by Toshiba(1997-11-14)
	if(m_uNumOfBitsInBuffer==0) getBits (nExtraBits);		//first get some bits then flush
	//getBits (nExtraBits);		//first get some bits then flush
//	End Toshiba(1997-11-14)
	// commented out for error recovery assert (m_uNumOfBitsInBuffer != 8);
	m_lCounter += m_uNumOfBitsInBuffer;
	m_uNumOfBitsInBuffer = 0;
}

Void CInBitStream::setBookmark ()
{
	bookmark (1);
}

Void CInBitStream::gotoBookmark ()
{
	bookmark (0);
}

Void CInBitStream::bookmark (Bool bSet)
{

	if(bSet) {
	  m_pInStream->bookmark(bSet);
		m_uNumOfBitsInBuffer_bookmark = m_uNumOfBitsInBuffer;
		m_chDecBuffer_bookmark = m_chDecBuffer;
		m_lCounter_bookmark = m_lCounter;
		m_bBookmarkOn = TRUE;
	}
	else {
	  m_pInStream->bookmark(bSet);
		m_uNumOfBitsInBuffer = m_uNumOfBitsInBuffer_bookmark; 
		m_chDecBuffer = m_chDecBuffer_bookmark; 
		m_lCounter = m_lCounter_bookmark; 
		m_bBookmarkOn = FALSE;
	}
}

COutBitStream::COutBitStream (Char* pchBuffer, Int iBitPosition, ostream* pstrmTrace) : 
	m_pstrmTrace (pstrmTrace),
	m_chEncBuffer (0),
	m_uEncNumEmptyBits (8)
{
	assert (iBitPosition < 8);
	assert (iBitPosition >= 0);
	m_iBitPosition = iBitPosition;
	m_pchBuffer = pchBuffer;
	m_lCounter = 0;
	m_pchBufferRun = m_pchBuffer;
	m_iBuffer = 0;
// Added for Data Partitioning mode by Toshiba(1998-1-16)
	m_bDontSendBits = FALSE;
// End Toshiba(1998-1-16)
}
 

Void COutBitStream::reset ()		//but keep the byte buffer
{
//	m_uEncNumEmptyBits = 8;
//	m_chEncBuffer = 0;
	m_pchBufferRun = m_pchBuffer;
	m_iBuffer = 0;
}

Void COutBitStream::resetAll ()
{
	m_iBitPosition = 0;
	m_lCounter = 0;
	m_uEncNumEmptyBits = 8;
	m_chEncBuffer = 0;
	m_pchBufferRun = m_pchBuffer;
	m_iBuffer = 0;
}

Void COutBitStream::setBookmark ()
{
	bookmark (1);
}

Void COutBitStream::gotoBookmark ()
{
	bookmark (0);
}

Void COutBitStream::bookmark (Bool bSet)
{
	static Bool bBookmarkOn = FALSE;
	static Int iBitPosition;
	static Int lCounter;
	static UInt uEncNumEmptyBits;
	static U8 chEncBuffer;
	static Char* pchBufferRun;
	static Int iBuffer;

	if (bSet) {
		iBitPosition = m_iBitPosition;
		lCounter = m_lCounter;
		uEncNumEmptyBits = m_uEncNumEmptyBits;
		chEncBuffer = m_chEncBuffer;
		pchBufferRun = m_pchBufferRun;
		iBuffer = m_iBuffer;
		bBookmarkOn = TRUE;
	}
	else {
		m_iBitPosition = iBitPosition;
		m_lCounter = lCounter;
		m_uEncNumEmptyBits = uEncNumEmptyBits;
		m_chEncBuffer = chEncBuffer;
		m_pchBufferRun = pchBufferRun;
		m_iBuffer = iBuffer;
		bBookmarkOn = FALSE;
	}
}

Void COutBitStream::putBitsC (Char cBits,Int iNOfBits)
{
	putBits ((Int) cBits, (UInt) iNOfBits); 
}


Void COutBitStream::putBits (Int data, UInt numBits, const Char* rgchSymbolName)
{	
	assert (numBits < 100);				//usually not that large
	if (numBits == 0) return;

// Added for Data Partitioning mode by Toshiba(1998-1-16)
	if(m_bDontSendBits) return;
// End Toshiba(1998-1-16)

#ifdef __TRACE_AND_STATS_
	if (m_pstrmTrace != NULL && rgchSymbolName != NULL)	{
		Char* rgchBinaryForm = new Char [numBits + 1];
		assert (rgchBinaryForm != NULL);
		m_pstrmTrace->width (20);
		(*m_pstrmTrace) << rgchSymbolName << ": ";
		Int iMask = 0xFFFFFFFF;
		iMask = iMask << numBits;
		iMask = ~iMask;
		Int iCleanData = data & iMask;
		//_itoa (iCleanData, rgchBinaryForm, 2);	// win32 only
		Int iBit;
		iMask = 0x00000001;
		for (iBit = (Int) numBits - 1; iBit >= 0; iBit--)	{
			rgchBinaryForm [iBit] = ((iCleanData & iMask) == 0) ? '0' : '1';
			iMask = iMask << 1;
		}
		rgchBinaryForm [numBits] = '\0';
		m_pstrmTrace->width (numBits);
		m_pstrmTrace->fill ('0');
		(*m_pstrmTrace) << rgchBinaryForm;
		m_pstrmTrace->fill (' ');
		(*m_pstrmTrace) << " @" << m_lCounter << '\n';
		m_pstrmTrace->flush ();
		delete rgchBinaryForm;
	}
#endif // __TRACE_AND_STATS_

	if (m_uEncNumEmptyBits > numBits) {	// not ready to put the data to buffer since it's not full
		m_uEncNumEmptyBits -= numBits;
		Char mskData = (0xFF >> (8 - numBits)) & data; 
		mskData = mskData << m_uEncNumEmptyBits;
		m_chEncBuffer = m_chEncBuffer ^ mskData;
        m_lCounter += numBits;
	}
	else if (m_uEncNumEmptyBits == numBits) { // ready
		Char mskData = (0xFF >> (8 - numBits)) & data; 
		m_chEncBuffer = m_chEncBuffer ^ mskData;
		*m_pchBufferRun++ = m_chEncBuffer;
		m_iBuffer++;
		m_chEncBuffer = 0;
		m_uEncNumEmptyBits = 8;
        m_lCounter += numBits;
	}
	else { // ready, but need to handle the leftover part
		UInt temp = getbit (data, numBits - 1, m_uEncNumEmptyBits);
		numBits -= m_uEncNumEmptyBits; // length of unhandled part
		m_chEncBuffer = m_chEncBuffer ^ temp;
        m_lCounter += m_uEncNumEmptyBits;
		*m_pchBufferRun++ = m_chEncBuffer;
		m_iBuffer++;
		m_chEncBuffer = 0;
		m_uEncNumEmptyBits = 8;
		data = data ^ (temp << numBits);
		putBits (data, numBits);
	}
}

Int CInBitStream::eof () const
{
	if(m_uNumOfBitsInBuffer==0)
		return (m_pInStream->eof())?(EOF):0;
	else
		return 0;
}

Int CInBitStream::peekBits (const UInt numBits)
{
#if 0
  // wmay - removed this to simply set the bookmark and restore it.
	assert (numBits <= 32);
	UInt iBitsToRet;
	Int nBitsToPeek = numBits - m_uNumOfBitsInBuffer;

	if (nBitsToPeek <= 0)
		iBitsToRet = getbit (m_chDecBuffer, 7, numBits);
	else
	{
	  m_pInStream->bookmark(TRUE);
		iBitsToRet = getbit (m_chDecBuffer, 7, m_uNumOfBitsInBuffer);
		for (; nBitsToPeek > 0; nBitsToPeek -= 8) {
			int chNext = m_pInStream->get();			//get the next ch
			Int iNext = chNext & 0x000000FF;			//clean the upper bits 
			if (nBitsToPeek < 8){						//fractional char
				iNext = iNext >> (8 - nBitsToPeek);
				iBitsToRet = iNext | (iBitsToRet << nBitsToPeek);
			}
			else	{
				iBitsToRet = iBitsToRet << 8;			//full char
				iBitsToRet |= iNext;
			}
		}
		m_pInStream->bookmark(FALSE);
	}			
	
	return iBitsToRet;		
#endif
	UInt iBitsToRet;
	setBookmark();
	iBitsToRet = getBits(numBits);
	gotoBookmark();
	return iBitsToRet;

}

Int CInBitStream::peekOneBit (const UInt numBits) const
{
	Int iBit,chNext;
	Int nBitsToPeek = numBits - m_uNumOfBitsInBuffer;
	
	if(nBitsToPeek<=0)
		iBit=m_chDecBuffer&(256>>(m_uNumOfBitsInBuffer - 1 - numBits));
	else
	{
	  m_pInStream->bookmark(TRUE);
		for (; nBitsToPeek > 0; nBitsToPeek -= 8)
			chNext = m_pInStream->get();
		nBitsToPeek += 8;
		iBit=chNext&(256>>nBitsToPeek);
		m_pInStream->bookmark(FALSE);
	}

	return iBit!=0;
}

//wchen-9-30-97: peekflush () function name changed
//wchen-9-30-97: matching of stuffing bits done outside bitstrm.cpp because bitstrm.cpp should be generic 
//	Added for error resilience mode By Toshiba
Int CInBitStream::peekBitsTillByteAlign (Int& nBitsToPeek)
{
	assert (m_uNumOfBitsInBuffer != 8);
//	if(m_bFlashEnable){
	nBitsToPeek = (m_uNumOfBitsInBuffer == 0) ? 8 : m_uNumOfBitsInBuffer;
	return peekBits(nBitsToPeek);
//	}
//	return 0;
}

//wchen-9-30-97: function name changed
//added by toshiba for error resilience
//	Modified for error resilient mode by Toshiba(1997-11-14)
Int CInBitStream::peekBitsFromByteAlign (Int nBitsToPeek) const
//Int CInBitStream::peekBitsFromByteAlign (UInt nBitsToPeek) const
//	End Toshiba(1997-11-14)
{
	assert (nBitsToPeek <= 32);
	UInt iBitsToRet = 0;

//	if (nBitsToPeek <= 0)
//		iBitsToRet = getbit (m_chDecBuffer, 7, numBits);
	//wchen-9-30-97
	if (nBitsToPeek == 0)		// UInt can never to negative
		return 0;				// no need to call get bits
	else
	{
	  m_pInStream->bookmark(TRUE);
		if (m_uNumOfBitsInBuffer==0)	
			m_pInStream->get();	//length of stuffing bit = 8bits
		for (; nBitsToPeek > 0; nBitsToPeek -= 8) {
			Int chNext = m_pInStream->get();			//get the next ch
			Int iNext = chNext & 0x000000FF;			//clean the upper bits 
			if (nBitsToPeek < 8){						//fractional char
				iNext = iNext >> (8 - nBitsToPeek);
				iBitsToRet = iNext | (iBitsToRet << nBitsToPeek);
			}
			else	{
				iBitsToRet = iBitsToRet << 8;			//full char
				iBitsToRet |= iNext;
			}
		}
		m_pInStream->bookmark(FALSE);
	}
	return iBitsToRet;		
}

Void COutBitStream::putBits (Char *pBits, Int lNOfBits)
{
    assert(lNOfBits>=0);
    while(lNOfBits>0)
    {
        if(lNOfBits>8)
        {
            putBitsC((Int)(*pBits),8);
            lNOfBits-=8;
            pBits++;
        }
        else
        {
            putBitsC((Int)(*pBits),(UInt)lNOfBits);
            break;
        }
    }
}

Void COutBitStream::putBitStream(COutBitStream &cStrm)
{
	cStrm.m_pchBufferRun[0]=cStrm.m_chEncBuffer >> cStrm.m_uEncNumEmptyBits;
	putBits(cStrm.m_pchBuffer,cStrm.m_lCounter);
}

Int COutBitStream::flush ()
{
	Int nBits = 0;
	if (m_uEncNumEmptyBits != 8) {
		nBits = m_uEncNumEmptyBits;
		putBits ((Int) 0, (UInt) 1);
		putBits ((Int) 0xFFFF, nBits - 1);

/*
		*m_pchBufferRun++ = m_chEncBuffer;
		m_iBuffer++;
		m_lCounter += m_uEncNumEmptyBits;
		nBits = m_uEncNumEmptyBits;
		m_uEncNumEmptyBits = 8;
		m_chEncBuffer = 0;
*/
	}
	else	{
		nBits = 8;
		putBits ((Int) 0x007F, nBits);
	}
	return nBits;
}

Void COutBitStream::trace (const Char* rgchSymbolName)
{
	if (m_pstrmTrace != NULL)	{
		m_pstrmTrace->width (20);
		(*m_pstrmTrace) << rgchSymbolName;
		m_pstrmTrace->flush ();
	}
}

Void COutBitStream::trace (Int iValue, const Char* rgchSymbolName)
{
	if (m_pstrmTrace != NULL)	{
		m_pstrmTrace->width (20);
		(*m_pstrmTrace) << rgchSymbolName << "= ";
		(*m_pstrmTrace) << iValue << "\n";
		m_pstrmTrace->flush ();
	}
}

Void COutBitStream::trace (UInt uiValue, const Char* rgchSymbolName)
{
	if (m_pstrmTrace != NULL)	{
		m_pstrmTrace->width (20);
		(*m_pstrmTrace) << rgchSymbolName << "= ";
		(*m_pstrmTrace) << uiValue << "\n";
		m_pstrmTrace->flush ();
	}
}

Void COutBitStream::trace (Float fltValue, const Char* rgchSymbolName)
{
	if (m_pstrmTrace != NULL)	{
		m_pstrmTrace->width (20);
		(*m_pstrmTrace) << rgchSymbolName << "= ";
		(*m_pstrmTrace) << fltValue << "\n";
		m_pstrmTrace->flush ();
	}
}

#ifndef __DOUBLE_PRECISION_
Void COutBitStream::trace (Double dblValue, const Char* rgchSymbolName)
{
	if (m_pstrmTrace != NULL)	{
		m_pstrmTrace->width (20);
		(*m_pstrmTrace) << rgchSymbolName << "= ";
		(*m_pstrmTrace) << dblValue << "\n";
		m_pstrmTrace->flush ();
	}
}
#endif

Void COutBitStream::trace (const CMotionVector& mvValue, const Char* rgchSymbolName)
{
	if (m_pstrmTrace != NULL)	{
		m_pstrmTrace->width (20);
		(*m_pstrmTrace) << rgchSymbolName << "= ";
		(*m_pstrmTrace) << mvValue.iMVX + mvValue.iMVX + mvValue.iHalfX << ", ";
		(*m_pstrmTrace) << mvValue.iMVY + mvValue.iMVY + mvValue.iHalfY << "\n ";
		m_pstrmTrace->flush ();
	}
}

Void COutBitStream::trace (const CVector2D& vctValue, const Char* rgchSymbolName)
{
	if (m_pstrmTrace != NULL)	{
		m_pstrmTrace->width (20);
		(*m_pstrmTrace) << rgchSymbolName << "= ";
		(*m_pstrmTrace) << vctValue.x << ", ";
		(*m_pstrmTrace) << vctValue.y << "\n ";
		m_pstrmTrace->flush ();
	}
}

Void COutBitStream::trace (const CSite& stValue, const Char* rgchSymbolName)
{
	if (m_pstrmTrace != NULL)	{
		m_pstrmTrace->width (20);
		(*m_pstrmTrace) << rgchSymbolName << "= ";
		(*m_pstrmTrace) << stValue.x << ", ";
		(*m_pstrmTrace) << stValue.y << "\n ";
		m_pstrmTrace->flush ();
	}
}

Void COutBitStream::trace (const CFloatImage* pfi, const Char* rgchSymbolName, CRct rct)
{
	if (m_pstrmTrace == NULL)
		return;
	Int iValue;
	(*m_pstrmTrace) << rgchSymbolName << "= \n";
	if (rct.valid ())	{
		for (CoordI iY = rct.top; iY < rct.bottom; iY++)	{
			const PixelF* ppxlf = pfi->pixels (rct.left, iY);
			for (CoordI iX = rct.left; iX < rct.right; iX++)	{	
				iValue = (Int) *ppxlf;
				(*m_pstrmTrace) << iValue << " ";
				ppxlf++;
			}
			(*m_pstrmTrace) << "\n";
		}

	}
	else	{
		const PixelF* ppxlf = pfi->pixels ();
		for (CoordI iY = pfi->where ().top; iY < pfi->where ().bottom; iY++)	{
			for (CoordI iX = pfi->where ().left; iX < pfi->where ().right; iX++)	{	
				iValue = (Int) *ppxlf;
				(*m_pstrmTrace) << iValue << " ";
				ppxlf++;
			}
			(*m_pstrmTrace) << "\n";
		}
	}
	m_pstrmTrace->flush ();
}

Void COutBitStream::trace (const Float* ppxlf, UInt cElem, const Char* rgchSymbolName)
{
	if (m_pstrmTrace == NULL)
			return;	
	Int iValue;	
	(*m_pstrmTrace) << rgchSymbolName << ": \n";
	for (UInt iElem = 0; iElem < cElem; iElem++)	{
			iValue = (Int) *ppxlf;
			(*m_pstrmTrace) << iValue << " ";
			ppxlf++;
	}
	(*m_pstrmTrace) << "\n";
	m_pstrmTrace->flush ();
}

Void COutBitStream::trace (const Int* rgi, UInt cElem, const Char* rgchSymbolName)
{
	if (m_pstrmTrace == NULL)
		return;	
	(*m_pstrmTrace) << rgchSymbolName << ": \n";
	for (UInt iElem = 0; iElem < cElem; iElem++)	{
		(*m_pstrmTrace) << *rgi++ << " ";
	}
	(*m_pstrmTrace) << "\n";
	m_pstrmTrace->flush ();
}

Void COutBitStream::trace (const PixelC* rgpxlc, Int cCol, Int cRow, const Char* rgchSymbolName)	//this is for tracing shape buffers
{
	if (m_pstrmTrace == NULL)
		return;	
	(*m_pstrmTrace) << rgchSymbolName << ": \n";
	for (Int iRow = -1; iRow < cRow; iRow++)	{
		for (Int iCol = -1; iCol < cCol; iCol++)	{
			if (iRow == -1)
				(*m_pstrmTrace) << "-";
			else if (iCol == -1)	{
				m_pstrmTrace->width (2);
				(*m_pstrmTrace) << iRow << "|"; 
			}
			else
				(*m_pstrmTrace) << (*rgpxlc++ == OPAQUE); 
		}
		(*m_pstrmTrace) << "\n";		
	}
	m_pstrmTrace->flush ();
}

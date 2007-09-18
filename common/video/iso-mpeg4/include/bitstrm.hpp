/*************************************************************************

This software module was originally developed by 

	Ming-Chieh Lee (mingcl@microsoft.com), Microsoft Corporation
	Wei-ge Chen (wchen@microsoft.com), Microsoft Corporation
	Bruce Lin (blin@microsoft.com), Microsoft Corporation
	Chuang Gu (chuanggu@microsoft.com), Microsoft Corporation
	(date: March, 1996)

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
	
	bitstrm.hpp

Abstract:

	Classes for bitstream I/O.

Revision History:

*************************************************************************/

#ifndef _BITSTREAM_HPP_
#define _BITSTREAM_HPP_
#include "inbits.h"

#include <istream>
#include <ostream>

class CIOBitStream
{
public:
	// Operations
    Int getPosition () {return m_iBitPosition;}//get current bit position within current byte
    Int getCounter () {return m_lCounter;}//get counter of processed bits
    Void getCounter (Int lCounter) {m_lCounter = lCounter;}//set counter of processed bits

protected:
    Int m_lCounter;
    Int m_iBitPosition;
    UInt m_iBuffer;
    CIOBitStream () {m_iBitPosition=0; m_iBuffer=0; m_lCounter=0;}
};

#if 0
Class CInBitStream : public CIOBitStream
{
public:
	// Constructors
    CInBitStream () {m_bBookmarkOn = FALSE; m_chDecBuffer = 0; m_uNumOfBitsInBuffer = 0;}//create unattached bitstream    
    CInBitStream (CInByteStreamBase *inStream, Int iBitPosition = 0)//create bitstream and attach to inStream
            {m_bBookmarkOn = FALSE; attach (inStream, iBitPosition); m_chDecBuffer = 0; m_uNumOfBitsInBuffer = 0;}

	// Attributes
	Int eof () const;//is EOF

	// Operations
    UInt getBits (UInt numBits);//return bits from the stream
    Int peekBits (const UInt numBits);			//peek bits from the stream
	Int peekOneBit(const UInt numBits) const;			//peek one bit as far ahead as needed
	Int peekBitsTillByteAlign (Int& nBitsToPeek); //peek 1-8 bits until the next byte alignment
//	Modified for Error Resilient Mode By Toshiba(1997-11-14)
    Int peekBitsFromByteAlign (Int numBits) const;		//peek from the stream at byte boundary
    //Int peekBitsFromByteAlign (UInt numBits) const;		//peek from the stream at byte boundary
//	End Toshiba(1997-11-14)
    Void getBits (Char *pBits, Int lNOfBits);//get bits and write to the buffer
    Void attach (CInByteStreamBase *inStream, Int iBitPosition = 0);//attach to the inStream
	Void flush (Int nExtraBits = 0);
	Void setBookmark ();
	Void gotoBookmark ();
	Void bookmark (Bool bSet);

protected:
    CInByteStreamBase *m_pInStream;
    Char getBitsC (Int iNOfBits);
	UInt m_uNumOfBitsInBuffer;
	Char m_chDecBuffer;
	Bool m_bBookmarkOn;
	UInt m_uNumOfBitsInBuffer_bookmark;
	Char m_chDecBuffer_bookmark;
    Int m_lCounter_bookmark;
    Int m_iBitPosition_bookmark;
    UInt m_iBuffer_bookmark;
    Int m_bookmark_eofstate;
};
#endif

class COutBitStream : public CIOBitStream
{
public:
	// Constructors
	COutBitStream (){m_chEncBuffer = 0; m_uEncNumEmptyBits = 8;}//create unattached bitstream
  COutBitStream (Char* pchBuffer, Int iBitPosition, std::ostream * pstrmTrace); //create bitstream and attach to outStream
  ~COutBitStream() {};
	// attributes
	Char* str () const {return m_pchBuffer;}
	UInt pcount () const {return m_iBuffer;}

	// Operations
	Void putBits (Int data, UInt numBits, const Char* rgchSymbolName = NULL);//put bits into the stream
	Void putBits (Char *pBits, Int lNOfBits);//but bits from the array into the stream
	Void putBitStream(COutBitStream &cStrm); //put another bitstream into this one
	Void reset ();
	Void resetAll ();
	Void setBookmark ();
	Void gotoBookmark ();
	Int  flush ();//flush remaing bits in the COutBitStream into the attached ostream
	Void trace (const Char* rgchSymbolName);//dump a string into the trace stream
	Void trace (Int iValue, const Char* rgchSymbolName);//dump a variable into the trace stream
	Void trace (UInt iValue, const Char* rgchSymbolName);//dump a variable into the trace stream
	Void trace (Float fltValue, const Char* rgchSymbolName);
#ifndef __DOUBLE_PRECISION_
	Void trace (Double dblValue, const Char* rgchSymbolName);
#endif // __DOUBLE_PRECISION_
	Void trace (const CMotionVector& mvValue, const Char* rgchSymbolName);
	Void trace (const CVector2D& vctValue, const Char* rgchSymbolName);
	Void trace (const CSite& stValue, const Char* rgchSymbolName);
	Void trace (const CFloatImage* pfi, const Char* rgchSymbolName, CRct rct = CRct ());//dump a variable into the trace stream
	Void trace (const Float* rgflt, UInt cElem, const Char* rgchSymbolName);//dump a variable into the trace stream
	Void trace (const Int* rgi, UInt cElem, const Char* rgchSymbolName);//dump a variable into the trace stream
	Void trace (const PixelC* rgpxlc, Int cCol, Int cRow, const Char* rgchSymbolName);	//this is for tracing shape buffers
// Added for Data Partitioning mode by Toshiba(1998-1-16)
	Void SetDontSendBits(const Bool bDontSendBits) {m_bDontSendBits = bDontSendBits;}
	Bool GetDontSendBits() {return m_bDontSendBits;}
// End Toshiba(1998-1-16)

protected:
	Char* m_pchBuffer;
	Char* m_pchBufferRun;
	std::ostream* m_pstrmTrace;		//stream for trace file

    Void putBitsC (Char cBits, Int iNOfBits);
	Char m_chEncBuffer;
	UInt m_uEncNumEmptyBits;
	Void bookmark (Bool bSet);
// Added for Data Partitioning mode by Toshiba(1998-1-16)
	Bool m_bDontSendBits;
// End Toshiba(1998-1-16)
};

#endif

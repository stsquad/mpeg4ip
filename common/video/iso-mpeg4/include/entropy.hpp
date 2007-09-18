/*************************************************************************

This software module was originally developed by 

	Ming-Chieh Lee (mingcl@microsoft.com), Microsoft Corporation
	Wei-ge Chen (wchen@microsoft.com), Microsoft Corporation
	Bruce Lin (blin@microsoft.com), Microsoft Corporation
	Chuang Gu (chuanggu@microsoft.com), Microsoft Corporation
	(date: March, 1996)

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

	entropy.hpp

Abstract:

	base classes for entropy encoder/decoder

Revision History:

*************************************************************************/

#ifndef __ENTROPY_HPP_
#define __ENTROPY_HPP_
#include <istream>
#include <ostream>

class CInBitStream;
class COutBitStream;

typedef struct VlcTableTag {
	Int lSymbol;
	char *pchBits;
} VlcTable;

class CEntropyEncoder
{
public:
	virtual ~CEntropyEncoder () {};
	virtual Void loadTable (std::istream &Table) = 0;
	virtual Void loadTable (VlcTable *pVlc) = 0;
    virtual Void attachStream (COutBitStream &BitStream) = 0;
    virtual UInt encodeSymbol (Int lSymbol, Char* rgchSymbolName = NULL, Bool bDontSendBits = FALSE) = 0;
	virtual COutBitStream* bitstream () = 0;
};


class CEntropyDecoder
{
public:
	virtual ~CEntropyDecoder () {};
	virtual Void loadTable (std::istream &Table, Bool bIncompleteTree=TRUE) = 0;
	virtual Void loadTable (VlcTable *pVlc, Bool bIncompleteTree=TRUE) = 0;
    virtual Int decodeSymbol () = 0;
    virtual Void attachStream (CInBitStream *bitStream) = 0;
	virtual CInBitStream* bitstream () = 0;
};
#endif // __ENTROPY_HPP_

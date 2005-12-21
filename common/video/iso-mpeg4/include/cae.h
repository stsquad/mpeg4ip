/*************************************************************************

This software module was adopted from MPEG4 VM7.0 text, modified by 

	Wei-ge Chen (wchen@microsoft.com), Microsoft Corporation
	(date: April, 1997)

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
assign or donate the code to a third party and to inhibit third parties from using the code for non <MPEG standard> conforming products. 
This copyright notice must be included in all copies or derivative works. 

Copyright (c) 1996, 1997.

Module Name:

	cae.h

Abstract:

	Context-based arithmatic coding

Revision History:

*************************************************************************/

typedef unsigned short int USInt;
#define		CODE_BITS	32
#define 	HALF		((unsigned) 1 << (CODE_BITS-1))
#define 	QUARTER		(1 << (CODE_BITS-2))

#define ArCodec arcodec
#define ArCoder arcodec
#define ArDecoder arcodec

class COutBitStream;
class CInBitStream;
class arcodec {
public:
	~arcodec () {};
	arcodec () {};
	UInt L; /* lower bound */
	UInt R; /* code range */
	UInt V; /* current code value */
	UInt arpipe;
	Int bits_to_follow; /* follow bit count */
	Int first_bit;
	Int nzeros;
	Int nonzero;
	Int nzerosf;
	Int extrabits;
	Int nBits;
};


#define MAXHEADING 8
#define MAXMIDDLE 16
#define MAXTRAILING 8
//	Added for error resilient mode by Toshiba(1997-11-14)
#define MAXHEADING_ERR 3
#define MAXMIDDLE_ERR 10
#define MAXTRAILING_ERR 2
// End Toshiba(1997-11-14)

API Void StartArCoder(ArCoder *coder);
API Int  StopArCoder(ArCoder *coder, COutBitStream *bitstream);
API Void BitByItself(Int bit, ArCoder *coder, COutBitStream *bitstream);
API Void BitPlusFollow(Int bit, ArCoder *coder, COutBitStream *bitstream);
API Void ArCodeSymbol(Int bit, USInt c0, ArCoder *coder, COutBitStream *bitstream);
API Void ENCODE_RENORMALISE(ArCoder *coder, COutBitStream *bitstream);
API Void StartArDecoder(ArDecoder *decoder, CInBitStream *bitstream);
API Void StopArDecoder(ArDecoder *decoder, CInBitStream *bitstream);
API Void AddNextInputBit(CInBitStream *bitstream, ArDecoder *decoder);
API Int  ArDecodeSymbol(USInt c0, ArDecoder *decoder, CInBitStream *bitstream);
API Void DECODE_RENORMALISE(ArDecoder *decoder,  CInBitStream *bitstream);
API Void  BitstreamFlushBits(CInBitStream *bitstream, Int nBits);

/*************************************************************************

This software module was originally developed by 

	Wei-ge Chen (wchen@microsoft.com), Microsoft Corporation
	(date: April, 1996)

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

	shpdec.cpp

Abstract:

	binary shape decoding. 

Revision History:

*************************************************************************/

class CShapeDecoder : public CShapeCodec
{
public:
	// Constructors
	~CShapeDecoder ();
	CShapeDecoder ();
	CShapeDecoder (CVOPofMBsDecoder* pvmdec);

	// Atributes

	// Operations
	Void decodeShapeMode (Int iMB, const ShapeMode& shpmdColocatedMB = UNKNOWN);
	Void decode			 (Int iMB, /*const ShapeMode& shpmdColocatedMB,*/ 
								const CIntImage* pfiPred = NULL);

private:

	CEntropyDecoderSet* m_pentrdec;				// decoder for shape mv part 2
	CInBitStream* m_pibstrm;


	Void decodeCrAndSt ();
	Void decodeMVDS ();
};

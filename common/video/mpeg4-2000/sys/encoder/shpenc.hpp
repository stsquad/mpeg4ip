/*************************************************************************

This software module was originally developed by 

	Wei-ge Chen (wchen@microsoft.com), Microsoft Corporation
	(April, 1997)

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

	caenc.hpp

Abstract:

	binary shape encoder with context-based arithmatic coder

Revision History:

*************************************************************************/

#ifndef __SHPENC_HPP_
#define __SHPENC_HPP_

class CShapeEncoder : public CShapeCodec
{
public:
	// Constructors
	~CShapeEncoder ();
	CShapeEncoder(CVOPofMBsEncoder* pvmenc);

	UInt encode (Int iMB, const ShapeMode& shpmdColocatedMB);
	Bool isErrorLarge (const CIntImage* pfiCurrRecon, Int iAlphaTH);

	// Operations
	ShapeMode quantizeIntra (Int iMB, const ShapeMode& shpmdColocatedMB, Int iAlphaTH); //to be called before quantizeInter
	ShapeMode quantizeInter (Int iMB, const ShapeMode& shpmdColocatedMB, Int iAlphaTH,  //assume quantizeIntra has been done
							 const CIntImage* pfiPred, const CVector& vctShapeMVD, Bool bHas4MV);
																				
private:

	own CIntImage* m_pfiShape;
	own CIntImage* m_pfiShapeDown2;
	own CIntImage* m_pfiShapeDown4;
	UInt m_nBitsIfIntra, m_nBitsIfInter;
	Bool m_bHas4MV; //whether this MB has 4 mv (for lossy shape hack)
	CEntropyEncoderSet* m_pentenc;
	COutBitStream* m_pobstrm;

	UInt codeShapeMode (Int iMB, const ShapeMode& shpmdColocatedMB, ShapeMode shpmd = UNKNOWN, Bool bDontSendBits = FALSE);
	Void round (Int iAlphaTH);		//rounding of each MB
	UInt encodeCAE (CAEScanDirection msdir = UNKNOWN_DIR, ShapeMode shpmd = UNKNOWN,
					Bool bDontSendBits = FALSE);
	UInt codeCrAndSt (Bool bDontSendBits = FALSE);
	UInt encodeMVDS (Bool bDontSendBits = FALSE);
	Bool sameBlockTranspStatus (const CIntImage* pfiSrc, const CIntImage* pfiCurrRecon);				//for inter lossy shape
	Void readyCurrData ();
}; //
#endif


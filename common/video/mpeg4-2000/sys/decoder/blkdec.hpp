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

	blkDec.hpp

Abstract:

	Block decoder (texture decoding).  Derived from CBlock

Revision History:

*************************************************************************/

#ifndef __BLKDEC_HPP_
#define __BLKDEC_HPP_

class CBlockDecoder : public CBlock
{
public:
	// Constructors
	~CBlockDecoder () {};
	CBlockDecoder::CBlockDecoder (
		const VOLMode& volmd,
		CMBMode* pmbmdFromMB,							// coding mode for the current block
		const CIntImage* pfiAlpha,
		const Int thisBlockNum,
		CEntropyDecoderSet* pdecset						// entropy decoder set
   	);

	// Operations
	void decode (const CBlock* pblkPredictor);												// return the quantized block and sends the bits

///////////////// implementation /////////////////
private:

	CEntropyDecoderSet* m_pdecset;								// entropy decoders

	// Operations
	Void decodeTCOEF ();
	Int decodeIntraDCmpeg ();					//decode the intra-dc: mpeg2 method
	Void findEventOfVLCtableIndex (
								  Bool& bIsLastRun, 
								  UInt& uiRun, 
								  Int& iLevel, 
								  const Long lIndex);
	Void inverseDCACPredictionAndUnZigzag (const CBlock* pblkPredictor);

};

#endif // __BLKDEC_HPP_ 

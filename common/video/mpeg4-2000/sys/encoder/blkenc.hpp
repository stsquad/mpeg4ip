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

	blkEnc.hpp

Abstract:

	Block encoder, derived from CBlock

Revision History:

	Add mpeg mode (wchen) 8-April-1996

*************************************************************************/


#ifndef __BLKENC_HPP_
#define __BLKENC_HPP_

class CBlockEncoder : public CBlock
{
public:
	// Constructors
	~CBlockEncoder ();
	CBlockEncoder (
		const CIntImage* pfiSrc,			// the source data
		const CIntImage* pfiAlpha,
		const Int thisBlockNum,
		const VOLMode& volmd,
		CMBMode* pmbmd,				// coding mode for the current block
		CEntropyEncoderSet* pencset
	);
   	
	// Attributes
	const Float* dctCoeff () const {return m_rgfltDCTcoef;}// unquantized DCT coefficients
	
	// Operations
	Void quantize ();						// return the quantized block 
	Void DCACPredictionAndZigzag (const CBlock* pblkencPredictor);	//reset CBPY as well
	UInt encode ();

///////////////// implementation /////////////////
private:

	own CIntImage* m_pfiTexture;			// texture data (padded)
	CEntropyEncoderSet* m_pencset;
	own Float* m_rgfltDCTcoef;				// unquantized DCT coefficients
	own CFDCT* m_pfdct;

	// Operations
	UInt encodeMPEG ();// mpeg-style encoding
	Void dct ();
	Void quantizeDCTcoef ();				// quantization of DCT coeffs
	Void quantizeDCTcoefMPEG ();			// mpeg2 method
	UInt sendDCTcoef ();
	UInt sendIntraDCmpeg ();				//send intra-dc: mpeg2 method
	Int findVLCtableIndexOfNonLastEvent (UInt uiRun, UInt uiLevel);
	Int findVLCtableIndexOfLastEvent (UInt uiRun, UInt uiLevel);
	Int findVLCtableIndexOfIntra (Bool bIsLastRun, UInt uiRun, UInt uiLevel);
	UInt sendTCOEF ();
	UInt putBitsOfTCOEF (UInt uiRun, Int iLevel, Bool bIsLastRun);

};

#endif // __BLKENC_HPP_ 

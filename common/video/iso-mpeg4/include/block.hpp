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

	block.hpp

Abstract:

	Block base class

Revision History:

*************************************************************************/


#ifndef __BLOCK_HPP_
#define __BLOCK_HPP_

class CBlock
{
public:
	// Constructors
	virtual ~CBlock ();	
	CBlock (												
		const VOLMode& volmd,
		CMBMode* pmbmdFromMB,				// (de)coding mode for the current block
		const CIntImage* pfiAlpha,
		const Int thisBlockNum);	
   	
	// Attributes
	const CIntImage* textureQ () const {return m_pfiTextureQ;}// the texture (padded) data
	const Int* dctCoeffQ () const {return m_rgiDCTcoefQ;}// reconstructed DCT coefficients
	const Float* dctCoefRecon () const {return m_rgfltDCTcoefRecon;}// reconstructed DCT coefficients
	const CRct& where () const {return m_pfiAlpha -> where();}
	const CMBMode* pmbmd () const {return m_pmbmdFromMB;}	//mode of curr mb

	// Operations

///////////////// implementation /////////////////
protected:
	const VOLMode& m_volmd;				// coding mode of vop
	CMBMode* m_pmbmdFromMB;					// the coding mode information
	own CIntImage* m_pfiAlpha;			// the alpha data (for padding)
	Int m_blockNum;
	own CIntImage* m_pfiTextureQ;			// quantized texture data (will be de-padded)
	own Int* m_rgiDCTcoefQ;					// quantized DCT coefficients
own Int* m_rgiDCTcoefQD;				// quantized DCT coefficients, differentiated
	own Int* m_rgiDCTcoefQDZ;				// quantized DCT coefficients, differentiated and zigzag scanned
	own Float* m_rgfltDCTcoefRecon;			// reconstructed (de-quantized) DCT coefficients
	CIDCT* m_pidct;

	Void idct ();
	Void inverseQuantizeDCTcoef ();
	Void inverseQuantizeDCTcoefMPEG ();		// mpeg2 method
};

#endif // __BLOCK_HPP_

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

	MBDec.hpp

Abstract:

	MacroBlock Decoder

Revision History:

*************************************************************************/

#ifndef __MBDEC_HPP_
#define __MBDEC_HPP_

class CMacroBlockDecoder : public CMacroBlock
{
friend class CVOPofMBsDecoder;
public:
	// Constructors
	~CMacroBlockDecoder ();
	CMacroBlockDecoder ( // decoder
		const CRct& rctVOP,
		const CIntImage* pfiRecA, 
		UInt indexX, // indexX is the x indexTH macroblock of vopF
		UInt indexY, // indexY is the y indexTH macroblock of vopF
		const VOLMode& volmd,
		const VOPMode& vopmd,
		const CVOPIntYUVBA* pvopfRef0, // reference VOP in a previous time
		const CVOPIntYUVBA* pvopfRef1, // reference VOP in a later time
		CInBitStream* pbitstrmIn, // input bitstream
		CEntropyDecoderSet* pentrdecSet // huffman decoder set
	); 

	// attributes
	const CVOPIntYUVBA* pvopfError () {return m_pvopfError;}
	// Operations
	Void decodeOverhead (Int iPart = 0, const CMBMode* pmbmdColocatedRef1 = NULL);
	Void decodeMotionOverhead (const CMBMode* pmbmdColocatedRef1 = NULL);	//for sepr SMT
	Void decodeTextureOverhead (const CMBMode* pmbmdColocatedRef1 = NULL);	//for sepr SMT
	Void decodeAlphaOverhead ();
	Void textureDecode (const CMacroBlock* pmbPredLeft = NULL,
						const CMacroBlock* pmbPredTop = NULL,
						const CMacroBlock* pmbPredLeftTop = NULL);
	Void textureDecodeAlpha (const CMacroBlock* pmbPredLeft = NULL,
							 const CMacroBlock* pmbPredTop = NULL,
							 const CMacroBlock* pmbPredLeftTop = NULL);

	Void addPredMBToError (const CVOPIntYUVBA* pvopfPred); // set the predicted MB data

///////////////// implementation /////////////////

private:
	own CVOPIntYUVBA* m_pvopfError;		// recodntructed error signals
	CInBitStream* m_pbitstrmIn;				// input bitstream
	CEntropyDecoderSet* m_pentrdecSet;		// huffman decoder set
	CEntropyDecoder* m_pentrdecDCT;			// huffman decoder for DCT
	CEntropyDecoder* m_pentrdecMCBPCintra;	// encoder for MCBPC = intra 
	CEntropyDecoder* m_pentrdecMCBPCinter;	// encoder for MCBPC = inter 
	CEntropyDecoder* m_pentrdecCBPY;		// decoder for CBPY 
	CEntropyDecoder* m_pentrdecMbTypeBVOP;	// decoder for MBTYPE (BVOP)
	Void setBlkToErrorMB (const CIntImage* pfiBlkQ, Int blkNum); // overlay a quantized block to currQ 
	Void setErrorAlphaZero ();
};

#endif // __MBDEC_HPP_ 

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

	vopmbDec.hpp

Abstract:

	Decoder for VOP composed of MB's

Revision History:

*************************************************************************/


#ifndef __VOPMBDEC_HPP_
#define __VOPMBDEC_HPP_

class CVOPofMBsDecoder : public CVOPofMBs
{
friend class CShapeDecoder;
public:
	// Constructors
	~CVOPofMBsDecoder ();
	CVOPofMBsDecoder (
		Time tBCounter, // B-VOP counter
		VOLMode& volmd, 
		VOPMode& vopmd, 
		const CRct& rctVOP,
		const CIntImage* pfiRecA, 
		const CVOPIntYUVBA* pvopfRefer0, // reference VOP in a previous time
		const CVOPIntYUVBA* pvopfRefer1, // reference VOP in a later time
		const CDirectModeData* pdrtmdRef1, // for B-VOP direct mode 
		CInBitStream* pbitstrmIn, // bitstream
		CEntropyDecoderSet* pentrdecSet // huffman decoder set
	);

	// Operations
	Void decode ();


///////////////// implementation /////////////////

private:
	CInBitStream* m_pbitstrmIn; // bitstream
	CEntropyDecoder* m_pentrdecMV; // huffman decoder for MV
	CEntropyDecoderSet* m_pentrdecSet; // huffman decoder set
	CShapeDecoder** m_ppshpdec;			// shape decoders

	//motion dec funcs
	Void decodeMV (const UInt ix, const UInt iy);
	Int deScaleMV ( const Int iVLC, const UInt uiResidual, 
					const UInt uiScaleFactor);
	CVector2D getDiffMV ();
	Void setForwardMVofDirect (
		CMacroBlock* pmbCurr,
		const CVector2D& vctMVdelta, 
		const CVector2D& vctMVRef, 
		const Double dblTimeScale,
		const Int blkn
	);
	Void decodeMVforward (UInt ix, UInt iy);
	Void decodeMVbackward (UInt ix, UInt iy);
	Void decodeMVdirect (UInt ix, UInt iy);
	//motion dec funcs for the d*n sepr mode
	Void decodeMVD (const UInt ix, const UInt iy);
	Void forwardMVfromMVD (UInt ix, UInt iy);
	Void backwardMVfromMVD (UInt ix, UInt iy);
	Void mvFromMVD (const UInt ix, const UInt iy);
	Void decodeMVDforward (UInt ix, UInt iy);
	Void decodeMVDbackward (UInt ix, UInt iy);

	Void decodeNS ();
	Void decodeSI ();
	Void decodeSPB ();
};

#endif // __VOPMBDEC_HPP_


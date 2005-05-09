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

	vopmbEnc.hpp

Abstract:

	Encoder for VOP composed of MB's

Revision History:

*************************************************************************/


#ifndef __VOPMBENC_HPP_
#define __VOPMBENC_HPP_
	 

class CMacroBlock;
class CVOPIntYUVBA;

class CVOPofMBsEncoder : public CVOPofMBs
{
friend class CShapeEncoder;
public:
	// Constructors
	~CVOPofMBsEncoder ();
	CVOPofMBsEncoder (
		Time tBCounter, // B-VOP counter
		const CVOPIntYUVBA* pvopfCurr,
		const VOLMode& volmd, // VOL mode
		VOPMode& vopmd,	// VOP mode
		const CVOPIntYUVBA* pvopfMRefer0, // reference VOP in a previous time for motion estimation
		const CVOPIntYUVBA* pvopfMRefer1, // reference VOP in a later time for motion estimation
		const CVOPIntYUVBA* pvopfRefer0,	// reference VOP in a previous time
		const CVOPIntYUVBA* pvopfRefer1,	// reference VOP in a later time
		const CDirectModeData* pdrtmdRef1,	// for B-VOP direct mode 
		COutBitStream* pbitstrmOut,			// output bitstream
		CEntropyEncoderSet*  pEntrencSet,
		Bool bRateControl,
		CRCMode* pstatRC						// rate control status
	);

	// Attributes		   	
	CStatistics statistics () {return m_stat;} // return statistics


	// Operations
	Void encode ();

///////////////// implementation /////////////////

private:
	const CVOPIntYUVBA* m_pvopfCurr;
	const CVOPIntYUVBA* m_pvopfMRef0;
	const CVOPIntYUVBA* m_pvopfMRef1;
	COutBitStream* m_pbitstrmOut; // output bitstream
	CEntropyEncoderSet* m_pEntrencSet;
	CStatistics m_stat;
	Bool m_bRateControl;
	CRCMode* m_pstatRC;				// rate control
	CShapeEncoder** m_ppshpenc;

	//encoder the MV of MB (x, iy)
	UInt encodeMV (const UInt ix, const UInt iy);
	Void scaleMV (Int& iVLC, UInt& uiResidual, Int iDiffMVcomponent);
	UInt sendDiffMV (const CVector2D& vctDiffMV);

	Void encodeNS ();		//non-separate shape motion texture
	Void encodeSI ();	//separate shape motion texture I vop
	Void encodeSPB ();	//ditto, PB vop
};

#endif 

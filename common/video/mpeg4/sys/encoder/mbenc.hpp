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

	MBenc.hpp

Abstract:

	MacroBlock encoder

Revision History:

*************************************************************************/

#ifndef __MBENC_HPP_
#define __MBENC_HPP_

Class CMacroBlockEncoder : public CMacroBlock
{
friend class CVOPofMBsEncoder;
public:
	// Constructors
	~CMacroBlockEncoder ();
	CMacroBlockEncoder (
		Time tBCounter,						// B-VOP counter
		const CVOPIntYUVBA* pvopfCurr,	// original VOP 
		UInt indexX,						// index is the indexTH macroblock of vopF
		UInt indexY,						// index is the indexTH macroblock of vopF
		const VOLMode& volmd,				// VOL mode
		const VOPMode& vopmd,				// VOP mode
		const CVOPIntYUVBA* pvopfMRef0,	// reference VOP in a previous time for motion estimation
		const CVOPIntYUVBA* pvopfMRef1,	// reference VOP in a later time for motion estimation
		const CVOPIntYUVBA* pvopfRef0,	// reference VOP in a previous time
		const CVOPIntYUVBA* pvopfRef1,	// reference VOP in a later time
		COutBitStream* pbitstrmOut,			// output bitstream
		CEntropyEncoderSet* pentrencSet		// collection of entropy encoders
	); 

	// Attributes
	const CVOPIntYUVBA* pvopfPred () const {return m_pvopfPred;}
	const CVOPIntYUVBA* pvopfText () const {return m_pvopfTexture;}
	const CStatistics& statistics () const {return m_stat;} // return statistics

	// Operations
	Void setPredMB (const CVOPIntYUVBA* pvopfPred); // set the predicted MB data
	Double motionEsti (
		Bool bInBoundRef1,					// these data are for direct mode 
		const CMBMode* pmbmdRef1 = NULL,
		const CMotionVector* rgmvRef1 = NULL,
		Time tBCounter = 0
	);										// find MV and set some of the modes
	CMotionVector motionEstiForShape (const CMotionVector& mvPredictor);
											// find MV for binary shape MB
	Void textureQuantize ();				// DCT and quantize DCT coefficients
	Void zigZagScanAndIntraDCACPrediction (const CMacroBlock* pmbPredLeft, 
							  const CMacroBlock* pmbPredTop,
							  const CMacroBlock* pmbPredLeftTop);
	Void dctTextureCoeffCompress ();		// compress DCT coefficients
	Void dctAlphaCoeffCompress ();			// compress DCT coefficients for gray alpha
	Void codeOverhead (Int iPart = 0, const CMBMode* pmbmdColocatedRef1 = NULL); //can be split into two parts by iPart
	Void codeMotionOverhead (const CMBMode* pmbmdColocatedRef1 = NULL);
	Void codeTextureOverhead (const CMBMode* pmbmdColocatedRef1 = NULL);
	Void codeAlphaOverhead ();
	Void decideSkipMB ();					// decide whether to skip the MB
	Void setMVStat (UInt nbitsForMV)		// set number of bits for MV from vopMBs 
		{m_stat.nBitsMV = nbitsForMV;}	
	Void addMVStat (UInt nbitsForMV)		// add number of bits for MV from vopMBs 
		{m_stat.nBitsMV += nbitsForMV;}	


///////////////// implementation /////////////////

private:
	const CVOPIntYUVBA* m_pvopfMRef0;
	const CVOPIntYUVBA* m_pvopfMRef1;
	own CVOPIntYUVBA* m_pvopfPred; // predicted MB (only for inter MB)
	own CVOPIntYUVBA* m_pvopfTexture; // texture data to be coded
	own CIntImage* m_pfiBYOrig;
	own CIntImage* m_pfiBUVOrig;
	COutBitStream* m_pbitstrmOut; // output bitstream
	CEntropyEncoderSet* m_pentrencSet;
	CEntropyEncoder* m_pentrencDCT;			// collection huffman encoder
	CEntropyEncoder* m_pentrencMCBPCinter;	// entropy coder for MCBPC
	CEntropyEncoder* m_pentrencMCBPCintra;	// entropy coder for MCBPC
	CEntropyEncoder* m_pentrencMbTypeBVOP;	// entropy coder for MBtype (BVOP)

	CStatistics m_stat;

	Double blockMatch (Bool bForward, Bool bApply8x8); // perform forward or backward block match
	Double interpolateMC () const; // compute the SAD for the interpolate mode
	Double directMC () const; // compute the SAD for the interpolate mode
	Void decideDCTMode (Double sadInter);
	Void setBlkToQuanMB (const CIntImage* pfiBlkQ, BlockNum blkNum); // overlay a quantized block to currQ 
	Bool zeroMotionV (MBType mbType) const; // check whether the MV's are all zero
	Void computeDirectDeltaMV (const CMBMode* pmbmdRef1, const CMotionVector* rgmvRef1, Time tBCounter);
	Void decideCODA (); // decide the skip mode of alpha plane
	Void storeOriginalBianry (); // original shape is for texture coding
};

#endif // __MBENC_HPP_ 

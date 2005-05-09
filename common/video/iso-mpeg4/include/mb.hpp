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

	MB.hpp

Abstract:

	MacroBlock base class 

Revision History:

NOTE:
	
	For encoder:
		
		m_pvopfCurrQ holds the original data until it is texture quantized

*************************************************************************/

#ifndef __MB_HPP_
#define __MB_HPP_

class CBlock;

class CMacroBlock
{
friend class CVOPofMBs;
public:
	// Constructors
	virtual ~CMacroBlock ();
	CMacroBlock (
		const VOLMode& volmd, // VOP mode
		const VOPMode& vopmd, // VOP mode
		const CVOPIntYUVBA* pvopfRef0, // reference VOP in a previous time
		const CVOPIntYUVBA* pvopfRef1 // reference VOP in a later time
	);

	// Attributes
	virtual const CMotionVector* rgMVForward () const {return m_rgmvForward;} // motion vector(s), 4 MV's for advanced mode
	virtual const CMotionVector* rgMVBackward () const {return m_rgmvBackward;} // motion vector(s), 4 MV's for advanced mode
	virtual CMotionVector mvForwardOfBlock (UInt iblk) const;  // motion vector(s), 4 MV's for advanced mode
	virtual CMotionVector mvBackwardOfBlock (UInt iblk) const;  // motion vector(s), 4 MV's for advanced mode
	virtual CMotionVector mvDirectDelta (UInt iblk) const;  // motion vector(s), 4 MV's for advanced mode
	virtual CMotionVector mvBY () const {return m_mvBY;}	//shape motion vector
	virtual const CVOPIntYUVBA* pvopfQOfMB () const {return m_pvopfCurrQ;}
	virtual const CMBMode& mode () const {return *m_pmbmd;}
	virtual const CRct& whereY () const {return m_pvopfCurrQ -> whereY ();}
	virtual const CRct& whereUV () const {return m_pvopfCurrQ -> whereUV ();}
	virtual CBlock** ppBlock () const {return m_ppblk;}
	virtual Bool bTranspAveraged () const {return m_bTranspAveraged;} 

	// Operations
	virtual own CVOPIntYUVBA* motionComp (MBType mbType = FORWARD) const; // non-overlapped motion compensation, overlapped MC is done at the vopmb level
	Void setMVForward (const CMotionVector& mvSrc, const Int blkn) 
		{m_rgmvForward [(UInt) blkn] = mvSrc;} // set motion vectors
	Void setMVBackward (const CMotionVector& mvSrc, const Int blkn) 
		{m_rgmvBackward [(UInt) blkn] = mvSrc;} // set motion vectors
	Void setMVBY (const CMotionVector& mvSrc)	{m_mvBY = mvSrc;} //set shape mv
	Void zeroPadCurrQ ();
	Void repeatPadCurrQ ();
	Void averagePadCurrQ ();
	Void repeatPadCurrQFromRight (const CMacroBlock& pmb);
	Void repeatPadCurrQFromBottom (const CMacroBlock& pmb);
	Void repeatPadCurrQFromLeft (const CMacroBlock& pmb);
	Void repeatPadCurrQFromTop (const CMacroBlock& pmb);

	// Resultant
	Bool isBlockAllTransparent (const Int blkn) const;
	Bool isAllBlocksAllValue (PixelI pxlf, Int blkn)	const;

///////////////// implementation /////////////////

protected:
	const CVOPIntYUVBA* m_pvopfRef0; // reference VOP in a previous time
	const CVOPIntYUVBA* m_pvopfRef1; // reference VOP in a later time
	own CVOPIntYUVBA* m_pvopfCurrQ; // macroblock
	own CMotionVector* m_rgmvForward; // forward motion vectors
	own CMotionVector* m_rgmvBackward; // backward motion vectors
	own CMotionVector* m_rgmvDirectDelta; // delta motion vectors for direct mode
	CMotionVector m_mvBY;					//mv for binary shape
	const VOLMode& m_volmd; // VOL mode
	const VOPMode& m_vopmd; // VOP mode
	CMBMode* m_pmbmd;
	CBlock** m_ppblk; // block objects
	UInt m_uiNumBlks; // number of blocks for texture coding.  10 for gray-scale alpha and 6 otherwise

	// for padding
	Bool m_bTranspAveraged;
	
	Void clapQuant (); // clap the pixel value to be within 0 and 255
	own CVOPIntYUVBA* motionCompFB (MBType mbType) const;	// motion comp, Forward and Backward
	own CIntImage* motionCompYBA (MBType mbType, PlaneType plnType) const; // non-oevrlapped MC for Y, B, and A plane
	own CIntImage* motionCompForCAE () const;		//only one MV, 18x18 motion comp, forward only
	Void motionCompUV (MBType mbType, CVOPIntYUVBA* pvopf, const CVector2D& mv, const CVector2D& mvBW = CVector2D ()) const; // non-overlapped motion compensation, overlapped MC is done at the vopmb level
	CVector2D mvDivideY (const CVector2D& mv, UInt divisor) const;
	CVector2D mvLookupUV (const CVector2D& mv, UInt uiNumNonTranspBlocks) const;
	Void backwardMVFromForwardMV (Bool bInBoundRef1, Bool bhas4MVRef1, const CMotionVector* rgmvRef1);
	Void decideTransparencyStatus (); //also change mvs of trasnparent blocks to NOT_MV to be consistent
	Void setQuantizedBinary (const CIntImage* pfiB);
	const CBlock* findPredictorBlock (Int iBlk, 
									  IntraPredDirection predDir,
									  const CMacroBlock* pmbPredLeft, 
									  const CMacroBlock* pmbPredTop, 
									  const CMacroBlock* pmbPredLeftTop = NULL);
	Void decideIntraPrediction (const CMacroBlock* pmbPredLeft, 
								const CMacroBlock* pmbPredTop, 
								const CMacroBlock* pmbPredLeftTop,
								Bool bDecideDCOnly = FALSE,
								Int blkn = ALL_Y_BLOCKS);


};

#endif // __MENCB_HPP_ 

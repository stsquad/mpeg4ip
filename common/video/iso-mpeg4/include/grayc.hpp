/*************************************************************************

This software module was originally developed by 

	Ming-Chieh Lee (mingcl@microsoft.com), Microsoft Corporation
	Wei-ge Chen (wchen@microsoft.com), Microsoft Corporation
	Bruce Lin (blin@microsoft.com), Microsoft Corporation
	Chuang Gu (chuanggu@microsoft.com), Microsoft Corporation
	(date: March, 1996)

  and edited by
    Sehoon Son (shson@unitel.co.kr) Samsung AIT

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

	grayf.hpp

Abstract:

	Float image class for gray (one-plane) pictures 

Revision History:
	Nov. 27, 1997 :added Spatial tool 
						by Takefumi Nagumo(nagumo@av.crl.sony.co.jp),SONY corporation

*************************************************************************/

#ifndef __GRAYC_HPP_ 
#define __GRAYC_HPP_

//#define __NBIT_ // NBIT: remove this line and recompile when code 8-bit data
#ifdef __NBIT_ // NBIT: only set when bits per pixel bigger than 8
typedef unsigned short PixelC;
#else
typedef unsigned char PixelC;
#endif

#ifndef MAX_NUM_CONTOUR
#define MAX_NUM_CONTOUR 200
#endif

class CVideoObjectPlane;

class CU8Image
{
public:
	// Constructors
	~CU8Image ();
	CU8Image (const CU8Image& uci, const CRct& r = CRct ()); // crop from a floatimage
	CU8Image (const CVideoObjectPlane& vop, RGBA comp, const CRct& r = CRct ());
	CU8Image (const CRct& r, PixelC px); // floatimage with same pixel value everywhere
	CU8Image (const CRct& r); // floatimage with same pixel value everywhere
	CU8Image (const CPolygonI& plg, const CRct& rct);		// fill in the polygon (interior boundary)
	CU8Image (
		const Char* pchFileName, // MPEG4 file format
		UInt ifr, // frame number
		const CRct& rct, // final rect 
		UInt nszHeader = 0 // number of header to be skipped
	);
	CU8Image (const Char* vdlFileName); // read from a VM file.

	// Attributes
	Bool valid () const {return this != 0;}
	const CRct& where () const {return m_rc;}
	CRct boundingBox (const PixelC pxlcOutsideColor = (PixelC) transpValue) const;
	PixelC pixel (CoordI x, CoordI y) const {return m_ppxlc [m_rc.offset (x, y)];}
	PixelC pixel (const CSite& st) const {return pixel (st.x, st.y);}
	const PixelC* pixels () const {return m_ppxlc;} // return pointer
	const PixelC* pixels (CoordI x, CoordI y) const {return m_ppxlc + m_rc.offset (x, y);}
	const PixelC* pixels (const CSite& st) const {return pixels (st.x, st.y);}

	// Resultants
	own CU8Image* decimate (UInt rateX, UInt rateY) const; // decimate the vframe by rateX and rateY.
	own CU8Image* decimateBinaryShape (UInt rateX, UInt rateY) const; // decimate by rateX and rateY in a conservative way
	own CU8Image* zoomup (UInt rateX, UInt rateY) const;
	own CU8Image* expand (UInt rateX, UInt rateY) const;  // expand by putting zeros in between
	own CU8Image* biInterpolate () const; // bilinearly interpolate 
	own CU8Image* downsampleForSpatialScalability () const;
//OBSS_SAIT_991015
	own CU8Image* upsampleForSpatialScalability (	Int iVerticalSamplingFactorM,
															Int iVerticalSamplingFactorN,
															Int iHorizontalSamplingFactorM,
															Int iHorizontalSamplingFactorN,
															Int iFrmWidth_SS,		
															Int iFrmHeight_SS,		
															Int iType,
															Int iExpandYRefFrame,
															Bool bShapeOnly		
															) const;
	own CU8Image* upsampleSegForSpatialScalability (	Int iVerticalSamplingFactorM,
															Int iVerticalSamplingFactorN,
															Int iHorizontalSamplingFactorM,
															Int iHorizontalSamplingFactorN,
															Int iFrmWidth_SS,			
															Int iFrmHeight_SS,		
															Int iType,
															Int iExpandYRefFrame
															) const;
//~OBSS_SAIT_991015
	own CU8Image* biInterpolate (UInt accuracy) const; // bilinearly interpolate using 2/4/8/16 quantization
	own CU8Image* transpose () const; // transpose the floatimge
	own CU8Image* complement () const; // complement a binarized uci
	own CU8Image* warp (const CAffine2D& aff) const; // affine warp
	own CU8Image* warp (const CPerspective2D& persp) const; // perspective warp
	own CU8Image* warp (const CPerspective2D& persp, const CRct& rctWarp) const; // perspective warp to rctWarp
	own CU8Image* warp (const CPerspective2D& persp, const CRct& rctWarp, const UInt accuracy) const; // perspective warp to rctWarp
	own CU8Image* smooth (UInt window) const; // smooth a binarized uci of window
	PixelC pixel (CoordD x, CoordD y) const; // bi-linear interpolation
	PixelC pixel (CoordI x, CoordI y, UInt accuracy) const; // bi-linear interpolation using 2/4/8/16 quantization
	PixelC pixel (const CSiteD& std) const {return pixel (std.x, std.y);} // bi-linear interpolation
	PixelC pixel (const CSite& std, UInt accuracy) const {return pixel (std.x, std.y, accuracy);} // bi-linear interpolation
	U8 mean () const; // mean
	U8 mean (const CU8Image* pfiMsk) const; // mean
	Int sumDeviation () const; // sum of first-order deviation
	Int sumDeviation (const CU8Image* puciMsk) const; // sum of first-order deviation
	UInt sumAbs (const CRct& rct = CRct ()) const; // sum of absolute value
	Bool allValue (PixelC ucVl, const CRct& rct = CRct ()) const; // whether all pixels have value of vl
	Bool biLevel (const CRct& rct = CRct ()) const; // whether it is a binary image
	Bool atLeastOneValue (PixelC ucVl, const CRct& rct = CRct ()) const; // whether at least one pixel has value of vl
	CRct whereVisible () const; // the tightest bounding box of non-transparent pixels
	UInt numPixelsNotValued (PixelC ucVl, const CRct& rct = CRct ()) const; // number of pixels not valued vl in region rct
	Double mse (const CU8Image& uciCompare) const;
	Double mse (const CU8Image& uciCompare, const CU8Image& uciMsk) const;
	Double snr (const CU8Image& uciCompare) const;
	Double snr (const CU8Image& uciCompare, const CU8Image& uciMsk) const;
	Void vdlDump (const Char* fileName = "tmp.vdl", const CRct& rct = CRct ()) const;
	Void dump (FILE* pf, const CRct& rct = CRct (), Int iScale = 256) const;
	Void dumpWithMask (FILE* pf, const CU8Image *puciMask, const CRct& rct = CRct (), Int iScale = 256, PixelC pxlZero = 0) const;
	Void txtDump (const Char* fileName = "tmp.txt") const;
	Void txtDump (FILE *) const;
	Void txtDumpMask (FILE *) const;
	own CU8Image* extendPadLeft ();	 // for block padding
	own CU8Image* extendPadRight ();	 // for block padding
	own CU8Image* extendPadTop ();	 // for block padding
	own CU8Image* extendPadDown ();	 // for block padding

	// Overloaded operators
	own CU8Image* operator * (const CTransform& tf) const;
	own CU8Image* operator * (const CAffine2D& aff) const {return warp (aff);} // affine warp
	own CU8Image* operator * (const CPerspective2D& persp) const {return warp (persp);} // perspective warp	
	Bool operator == (const CU8Image& uci) const;

	// Operations
	Void where (const CRct& r);	// crop based on r
	Void pixel (CoordI x, CoordI y, PixelC p) {m_ppxlc [m_rc.offset (x, y)] = p;} // set a pixel
	Void pixel (const CSite& st, PixelC p) {pixel (st.x, st.y, p);} //  set a pixel
	Void setRect (const CRct& rct); // set the rect
	Void shift (Int nX, Int nY) {m_rc.shift (nX, nY);} // shift CRct by nX and nY
	Void zeroPad (const CU8Image* pucimsk); // zero padding
	Void LPEPad (const CU8Image* pucimsk); // Low Pass Extrapolation Padding

	Void simplifiedRepeatPad (const CU8Image*); // simplified repetitive padding
	Void modifiedSimplifiedRepeatPad (const CU8Image*);
	Void originalRepeatPad (const CU8Image*);
	Void modifiedVMRepeatPad(const CU8Image *puciMask);
	
	Void repeatPad (const CU8Image* pucimsk); // repetitive padding
	Void sHorRepeatPad (const CU8Image* pucimsk); // simplified horizontal repetitive padding
	Void sVerRepeatPad (const CU8Image* pucimsk); // simplified vertical repetitive padding
	Void mrepeatPad (const CU8Image* pucimsk); // modified repetitive padding
	Void blockPad (const CU8Image* puciOrigMask, const UInt uintBlockSize); // block padding
	Void averagePad (const CU8Image* puciOrigMask); // averaging padding
	Void repeatPadNew (const CU8Image* pucimsk); // repetitive padding
	Void threshold (PixelC ucThresh);				   // cut < thresh to be zero
	Void binarize (PixelC ucThresh); // binarize it
/* NBIT: change U8 to PixelC
	Void checkRange (U8 ucMin, U8 ucMax); 
*/
	Void checkRange (PixelC ucMin, PixelC ucMax); 
	Void overlay (const CU8Image& uci); // overlay uci on top of *this
	Void overlay (const CU8Image& uci, const CRct& rctSrc); // overlay part (according to rctSrc) of uci on top of this
	Void CU8Image_xor (const CU8Image& uci); // xor two binarized uci
	Void CU8Image_or (const CU8Image& uci); // or of two binarized uci
	Void CU8Image_and (const CU8Image& uci); // or of two binarized uci
	Void maskOut (const CU8Image& uci); // mask out uci
	Void mutiplyAlpha (const CU8Image& uci); //multiply by alpha channel
	Void cropOnAlpha (); //  crop the image based on non-transparent value
	Void operator = (const CU8Image& uci);
	Void decimateBinaryShapeFrom (const CU8Image& uciSrc, const Bool bInterlace); // decimate by rateX and rateY in a conservative way

//OBSS_SAIT_991015
	Bool* m_pbHorSamplingChk;
	Bool* m_pbVerSamplingChk;
//~OBSS_SAIT_991015
	
///////////////// implementation /////////////////
protected:
	PixelC* m_ppxlc;
	CRct m_rc;

	Void allocate (const CRct& r, PixelC pxlc);
	Void allocate (const CRct& r);
	Void copyConstruct (const CU8Image& uci, const CRct& rct);
	Void swap (CU8Image& uci);

	Void fillColumnDownward (
		const PixelC*& pxlOrigMask, 
		const PixelC*& ppxlOrigData, 
		PixelC*& ppxlFillMask, 
		PixelC*& ppxlFillData
	) const;
	Void fillColumnUpward (
		const PixelC*& ppxlOrigMask, 
		const PixelC*& ppxlOrigData, 
		PixelC*& ppxlFillMask, 
		PixelC*& ppxlMaskData
	)	const;
	Void fillRowRightward (
		const PixelC*& ppxlOrigMask, 
		const PixelC*& ppxlOrigData, 
		PixelC*& ppxlFillMask, 
		PixelC*& ppxlFillData
	) const;
	Void fillRowLeftward (
		const PixelC*& ppxlOrigMask, 
		const PixelC*& ppxlOrigData, 
		PixelC*& ppxlFillMask, 
		PixelC*& ppxlFillData
	) const;
	Void fillCorners (CRct& nBlkRct);
	Void fillHoles (const CU8Image *puciFillMask);
	own CU8Image* smooth_ (UInt window) const; // smooth a binarized uci of window

	// for block padding
	Void dilation4 (CU8Image* pMask);
	Void dilation8 (CU8Image* pMask);
};

#endif //_GRAYF_HPP_	 

	 

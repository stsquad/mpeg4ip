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

	grayf.hpp

Abstract:

	Float image class for gray (one-plane) pictures 

Revision History:

*************************************************************************/

#ifndef __GRAYI_HPP_ 
#define __GRAYI_HPP_

typedef int PixelI;

#ifndef MAX_NUM_CONTOUR
#define MAX_NUM_CONTOUR 200
#endif

class CVideoObjectPlane;

class CIntImage
{
public:
	// Constructors
	~CIntImage ();
	CIntImage (const CIntImage& ii, const CRct& r = CRct ()); // crop from a floatimage
	CIntImage (const CVideoObjectPlane& vop, RGBA comp);
	CIntImage (const CRct& r = CRct (), PixelI px = 0); // floatimage with same pixel value everywhere
	CIntImage (const CPolygonI& plg, const CRct& rct);		// fill in the polygon (interior boundary)
	CIntImage (
		const Char* pchFileName, // MPEG4 file format
		UInt ifr, // frame number
		const CRct& rct, // final rect 
		UInt nszHeader = 0 // number of header to be skipped
	);
	CIntImage (const Char* vdlFileName); // read from a VM file.

	// Attributes
	Bool valid () const {return this != 0;}
	const CRct& where () const {return m_rc;}
	CRct boundingBox (const PixelI pxliOutsideColor = (PixelI) transpValue) const;
	PixelI pixel (CoordI x, CoordI y) const {return m_ppxli [m_rc.offset (x, y)];}
	PixelI pixel (const CSite& st) const {return pixel (st.x, st.y);}
	const PixelI* pixels () const {return (this == NULL) ? NULL : m_ppxli;} // return pointer
	const PixelI* pixels (CoordI x, CoordI y) const {return m_ppxli + m_rc.offset (x, y);}
	const PixelI* pixels (const CSite& st) const {return pixels (st.x, st.y);}

	// Resultants
	own CIntImage* decimate (UInt rateX, UInt rateY) const; // decimate the vframe by rateX and rateY.
	own CIntImage* decimateBinaryShape (UInt rateX, UInt rateY) const; // decimate by rateX and rateY in a conservative way
	own CIntImage* zoomup (UInt rateX, UInt rateY) const;
	own CIntImage* expand (UInt rateX, UInt rateY) const;  // expand by putting zeros in between
	own CIntImage* biInterpolate () const; // bilinearly interpolate 
	own CIntImage* downsampleForSpatialScalability () const;
	own CIntImage* upsampleForSpatialScalability () const;
	own CIntImage* biInterpolate (UInt accuracy) const; // bilinearly interpolate using 2/4/8/16 quantization
	own CIntImage* transpose () const; // transpose the floatimge
	own CIntImage* complement () const; // complement a binarized ii
	own CIntImage* warp (const CAffine2D& aff) const; // affine warp
	own CIntImage* warp (const CPerspective2D& persp) const; // perspective warp
	own CIntImage* warp (const CPerspective2D& persp, const CRct& rctWarp) const; // perspective warp to rctWarp
	own CIntImage* warp (const CPerspective2D& persp, const CRct& rctWarp, const UInt accuracy) const; // perspective warp to rctWarp
	own CIntImage* smooth (UInt window) const; // smooth a binarized ii of window
	PixelI pixel (CoordD x, CoordD y) const; // bi-linear interpolation
	PixelI pixel (CoordI x, CoordI y, UInt accuracy) const; // bi-linear interpolation using 2/4/8/16 quantization
	PixelI pixel (const CSiteD& std) const {return pixel (std.x, std.y);} // bi-linear interpolation
	PixelI pixel (const CSite& std, UInt accuracy) const {return pixel (std.x, std.y, accuracy);} // bi-linear interpolation
	Int mean () const; // mean
	Int mean (const CIntImage* pfiMsk) const; // mean
	Int sumAbs (const CRct& rct = CRct ()) const;
	Int sumDeviation () const; // sum of first-order deviation
	Int sumDeviation (const CIntImage* piiMsk) const; // sum of first-order deviation
	Bool allValue (Int ucVl, const CRct& rct = CRct ()) const; // whether all pixels have value of vl
	Bool biLevel (const CRct& rct = CRct ()) const; // whether it is a binary image
	Bool atLeastOneValue (Int ucVl, const CRct& rct = CRct ()) const; // whether at least one pixel has value of vl
	CRct whereVisible () const; // the tightest bounding box of non-transparent pixels
	UInt numPixelsNotValued (Int ucVl, const CRct& rct = CRct ()) const; // number of pixels not valued vl in region rct
	Double mse (const CIntImage& iiCompare) const;
	Double mse (const CIntImage& iiCompare, const CIntImage& iiMsk) const;
	Double snr (const CIntImage& iiCompare) const;
	Double snr (const CIntImage& iiCompare, const CIntImage& iiMsk) const;
	Void vdlDump (const Char* fileName = "tmp.vdl") const;
	Void dump (FILE *) const;
	Void txtDump (const Char* fileName = "tmp.txt") const;
	Void txtDump (FILE *) const;
	Void txtDumpMask (FILE *) const;
	own CIntImage* extendPadLeft ();	 // for block padding
	own CIntImage* extendPadRight ();	 // for block padding
	own CIntImage* extendPadTop ();	 // for block padding
	own CIntImage* extendPadDown ();	 // for block padding
	own CIntImage* average (const CIntImage& ii) const;

	// Overloaded operators
	own CIntImage* operator + (const CIntImage& ii) const;
	own CIntImage* operator - (const CIntImage& ii) const;
	own CIntImage* operator * (Int scale) const; // multiply every pixel by scale
	own CIntImage* operator / (Int scale) const; // divide every pixel by scale
	own CIntImage* operator * (const CTransform& tf) const;
	own CIntImage* operator * (const CAffine2D& aff) const {return warp (aff);} // affine warp
	own CIntImage* operator * (const CPerspective2D& persp) const {return warp (persp);} // perspective warp	
	Bool operator == (const CIntImage& ii) const;

	// Operations
	Void where (const CRct& r);	// crop based on r
	Void pixel (CoordI x, CoordI y, PixelI p) {m_ppxli [m_rc.offset (x, y)] = p;} // set a pixel
	Void pixel (const CSite& st, PixelI p) {pixel (st.x, st.y, p);} //  set a pixel
	Void setRect (const CRct& rct); // set the rect
	Void shift (Int nX, Int nY) {m_rc.shift (nX, nY);} // shift CRct by nX and nY
	Void zeroPad (const CIntImage* piimsk); // zero padding
	Void LPEPad (const CIntImage* piimsk); // Low Pass Extrapolation Padding

	Void simplifiedRepeatPad (const CIntImage*); // simplified repetitive padding
	Void modifiedSimplifiedRepeatPad (const CIntImage*);
	Void originalRepeatPad (const CIntImage*);
	Void modifiedVMRepeatPad(const CIntImage *piiMask);
	
	Void repeatPad (const CIntImage* piimsk); // repetitive padding
	Void sHorRepeatPad (const CIntImage* piimsk); // simplified horizontal repetitive padding
	Void sVerRepeatPad (const CIntImage* piimsk); // simplified vertical repetitive padding
	Void mrepeatPad (const CIntImage* piimsk); // modified repetitive padding
	Void blockPad (const CIntImage* piiOrigMask, const UInt uintBlockSize); // block padding
	Void averagePad (const CIntImage* piiOrigMask); // averaging padding
	Void repeatPadNew (const CIntImage* piimsk); // repetitive padding
	Void threshold (Int ucThresh);				   // cut < thresh to be zero
	Void binarize (Int ucThresh); // binarize it
	Void checkRange (Int ucMin, Int ucMax); 
	Void overlay (const CIntImage& ii); // overlay ii on top of *this
	Void overlay (const CFloatImage& fi);
	Void xorIi (const CIntImage& ii); // xor two binarized ii
	Void orIi (const CIntImage& ii); // or of two binarized ii
	Void andIi (const CIntImage& ii); // or of two binarized ii
	Void maskOut (const CIntImage& ii); // mask out ii
	Void mutiplyAlpha (const CIntImage& ii); //multiply by alpha channel
	Void cropOnAlpha (); //  crop the image based on non-transparent value
	Void operator = (const CIntImage& ii);


	
///////////////// implementation /////////////////
protected:
	PixelI* m_ppxli;
	CRct m_rc;

	Void allocate (const CRct& r, PixelI pxli);
	Void allocate (const CRct& r);
	Void copyConstruct (const CIntImage& ii, const CRct& rct);
	Void swap (CIntImage& ii);

	Void fillColumnDownward (
		const PixelI*& pxlOrigMask, 
		const PixelI*& ppxlOrigData, 
		PixelI*& ppxlFillMask, 
		PixelI*& ppxlFillData
	) const;
	Void fillColumnUpward (
		const PixelI*& ppxlOrigMask, 
		const PixelI*& ppxlOrigData, 
		PixelI*& ppxlFillMask, 
		PixelI*& ppxlMaskData
	)	const;
	Void fillRowRightward (
		const PixelI*& ppxlOrigMask, 
		const PixelI*& ppxlOrigData, 
		PixelI*& ppxlFillMask, 
		PixelI*& ppxlFillData
	) const;
	Void fillRowLeftward (
		const PixelI*& ppxlOrigMask, 
		const PixelI*& ppxlOrigData, 
		PixelI*& ppxlFillMask, 
		PixelI*& ppxlFillData
	) const;
	Void fillCorners (CRct& nBlkRct);
	Void fillHoles (const CIntImage *piiFillMask);
	own CIntImage* smooth_ (UInt window) const; // smooth a binarized ii of window

	// for block padding
	Void dilation4 (CIntImage* pMask);
	Void dilation8 (CIntImage* pMask);
};

#endif //_GRAYF_HPP_	 

	 

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

#ifndef __GRAYF_HPP_ 
#define __GRAYF_HPP_

typedef Float PixelF;

#define MAX_NUM_CONTOUR 200

class CVideoObjectPlane;

class CFloatImage
{
public:
	// Constructors
	~CFloatImage ();
	CFloatImage (const CFloatImage& fi, const CRct& r = CRct ()); // crop from a floatimage
	CFloatImage (const CIntImage& fi, const CRct& r = CRct ()); // crop from an int image
	CFloatImage (const CVideoObjectPlane& vop, RGBA comp, const CRct& r = CRct ());
	CFloatImage (const CRct& r = CRct (), PixelF px = 0.); // floatimage with same pixel value everywhere
	CFloatImage (const CPolygonI& plg, const CRct& rct);		// fill in the polygon (interior boundary)
	CFloatImage (
		const Char* pchFileName, // MPEG4 file format
		UInt ifr, // frame number
		const CRct& rct, // final rect 
		UInt nszHeader = 0 // number of header to be skipped
	);
	CFloatImage (const Char* vdlFileName); // read from a VM file.

	// Attributes
	Bool valid () const {return this != 0;}
	const CRct& where () const {return m_rc;}
	CRct boundingBox (const PixelF pxlfOutsideColor = (PixelF) transpValue) const;
	PixelF pixel (CoordI x, CoordI y) const {return m_ppxlf [m_rc.offset (x, y)];}
	PixelF pixel (const CSite& st) const {return pixel (st.x, st.y);}
	const PixelF* pixels () const {return (this == NULL) ? NULL : m_ppxlf;} // return pointer
	const PixelF* pixels (CoordI x, CoordI y) const {return m_ppxlf + m_rc.offset (x, y);}
	const PixelF* pixels (const CSite& st) const {return pixels (st.x, st.y);}

	// Resultants
	own CFloatImage* decimate (UInt rateX, UInt rateY) const; // decimate the vframe by rateX and rateY.
	own CFloatImage* decimateBinaryShape (UInt rateX, UInt rateY) const; // decimate by rateX and rateY in a conservative way
	own CFloatImage* zoomup (UInt rateX, UInt rateY) const;
	own CFloatImage* expand (UInt rateX, UInt rateY) const;  // expand by putting zeros in between
	own CFloatImage* biInterpolate () const; // bilinearly interpolate 
	own CFloatImage* downsampleForSpatialScalability () const;
	own CFloatImage* upsampleForSpatialScalability () const;
	own CFloatImage* biInterpolate (UInt accuracy) const; // bilinearly interpolate using 2/4/8/16 quantization
	own CFloatImage* transpose () const; // transpose the floatimge
	own CFloatImage* complement () const; // complement a binarized fi
	own CFloatImage* warp (const CAffine2D& aff) const; // affine warp
	own CFloatImage* warp (const CPerspective2D& persp) const; // perspective warp
	own CFloatImage* warp (const CPerspective2D& persp, const CRct& rctWarp) const; // perspective warp to rctWarp
	own CFloatImage* warp (const CPerspective2D& persp, const CRct& rctWarp, const UInt accuracy) const; // perspective warp to rctWarp
	own CFloatImage* smooth (UInt window) const; // smooth a binarized fi of window
	PixelF pixel (CoordD x, CoordD y) const; // bi-linear interpolation
	PixelF pixel (CoordI x, CoordI y, UInt accuracy) const; // bi-linear interpolation using 2/4/8/16 quantization
	PixelF pixel (const CSiteD& std) const {return pixel (std.x, std.y);} // bi-linear interpolation
	PixelF pixel (const CSite& std, UInt accuracy) const {return pixel (std.x, std.y, accuracy);} // bi-linear interpolation
	Double mean () const; // mean
	Double mean (const CFloatImage* pfiMsk) const; // mean
	Double sumAbs (const CRct& rct = CRct ()) const; // sum of absolute value
	Double sumDeviation () const; // sum of first-order deviation
	Double sumDeviation (const CFloatImage* pfiMsk) const; // sum of first-order deviation
	Bool allValue (Float vl, const CRct& rct = CRct ()) const; // whether all pixels have value of vl
	Bool biLevel (const CRct& rct = CRct ()) const; // whether it is a binary image
	Bool atLeastOneValue (Float vl, const CRct& rct = CRct ()) const; // whether at least one pixel has value of vl
	CRct whereVisible () const; // the tightest bounding box of non-transparent pixels
	UInt numPixelsNotValued (Float vl, const CRct& rct = CRct ()) const; // number of pixels not valued vl in region rct
	Double mse (const CFloatImage& fiCompare) const;
	Double mse (const CFloatImage& fiCompare, const CFloatImage& fiMsk) const;
	Double snr (const CFloatImage& fiCompare) const;
	Double snr (const CFloatImage& fiCompare, const CFloatImage& fiMsk) const;
	Void vdlDump (const Char* fileName = "tmp.vdl") const;
	Void dump (FILE *) const;
	Void txtDump (const Char* fileName = "tmp.txt") const;
	Void txtDump (FILE *) const;
	Void txtDumpMask (FILE *) const;
	own CFloatImage* extendPadLeft ();	 // for block padding
	own CFloatImage* extendPadRight ();	 // for block padding
	own CFloatImage* extendPadTop ();	 // for block padding
	own CFloatImage* extendPadDown ();	 // for block padding

	// Overloaded operators
	own CFloatImage& operator += (const CFloatImage &);
	own CFloatImage* operator + (const CFloatImage& fi) const;
	own CFloatImage* operator - (const CFloatImage& fi) const;
	own CFloatImage* operator * (Float scale) const; // multiply every pixel by scale
	own CFloatImage* operator / (Float scale) const; // divide every pixel by scale
	own CFloatImage* operator * (const CTransform& tf) const;
	own CFloatImage* operator * (const CAffine2D& aff) const {return warp (aff);} // affine warp
	own CFloatImage* operator * (const CPerspective2D& persp) const {return warp (persp);} // perspective warp	
	Bool operator == (const CFloatImage& fi) const;

	// Operations
	Void where (const CRct& r);	// crop based on r
	Void pixel (CoordI x, CoordI y, PixelF p) {m_ppxlf [m_rc.offset (x, y)] = p;} // set a pixel
	Void pixel (const CSite& st, PixelF p) {pixel (st.x, st.y, p);} //  set a pixel
	Void setRect (const CRct& rct); // set the rect
	Void shift (Int nX, Int nY) {m_rc.shift (nX, nY);} // shift CRct by nX and nY
	Void zeroPad (const CFloatImage* pfimsk); // zero padding
	Void LPEPad (const CFloatImage* pfimsk); // Low Pass Extrapolation Padding

	Void simplifiedRepeatPad(const CFloatImage *); // simplified repetitive padding
	Void modSimplifiedRepeatPad(const CFloatImage *);
	Void originalRepeatPad(const CFloatImage *);
	Void modOriginalRepeatPad(const CFloatImage *);
	
	Void repeatPad (const CFloatImage* pfimsk); // repetitive padding
	Void sHorRepeatPad (const CFloatImage* pfimsk); // simplified horizontal repetitive padding
	Void sVerRepeatPad (const CFloatImage* pfimsk); // simplified vertical repetitive padding
	Void mrepeatPad (const CFloatImage* pfimsk); // modified repetitive padding
	Void blockPad (const CFloatImage* pfiOrigMask, const UInt uintBlockSize); // block padding
	Void dilatePad (const CFloatImage* pfiOrigMask); // morphological dilating padding
	Void averagePad (const CFloatImage* pfiOrigMask); // averaging padding
	Void repeatPadNew (const CFloatImage* pfimsk); // repetitive padding
	Void threshold (Float thresh);				   // cut < thresh to be zero
	Void binarize (Float fltThresh); // binarize it
	Void checkRange (Float fltMin, Float fltMax); 
	Void overlay (const CFloatImage& fi); // overlay fi on top of *this
	Void xorFi (const CFloatImage& fi); // xor two binarized fi
	Void orFi (const CFloatImage& fi); // or of two binarized fi
	Void andFi (const CFloatImage& fi); // or of two binarized fi
	Void maskOut (const CFloatImage& fi); // mask out fi
	Void mutiplyAlpha (const CFloatImage& fi); //multiply by alpha channel
	Void cropOnAlpha (); //  crop the image based on non-transparent value
	Void quantize (int stepsize, Bool dpcm = FALSE);
		// quantize the signal using the stepsize, dpcm if required
	Void deQuantize (int stepsize, Bool dpcm = FALSE);
		// deQuantize
	Void operator = (const CFloatImage& fi);
	Void roundNearestInt();


	
///////////////// implementation /////////////////
protected:
	PixelF* m_ppxlf;
	CRct m_rc;

	Void allocate (const CRct& r, PixelF pxlf);
	Void copyConstruct (const CFloatImage& fi, const CRct& rct);
	Void swap (CFloatImage& fi);

	Void fillColumnDownward (
		const PixelF*& pxlOrigMask, 
		const PixelF*& ppxlOrigData, 
		PixelF*& ppxlFillMask, 
		PixelF*& ppxlFillData
	) const;
	Void fillColumnUpward (
		const PixelF*& ppxlOrigMask, 
		const PixelF*& ppxlOrigData, 
		PixelF*& ppxlFillMask, 
		PixelF*& ppxlMaskData
	)	const;
	Void fillRowRightward (
		const PixelF*& ppxlOrigMask, 
		const PixelF*& ppxlOrigData, 
		PixelF*& ppxlFillMask, 
		PixelF*& ppxlFillData
	) const;
	Void fillRowLeftward (
		const PixelF*& ppxlOrigMask, 
		const PixelF*& ppxlOrigData, 
		PixelF*& ppxlFillMask, 
		PixelF*& ppxlFillData
	) const;
	Void fillCorners (CRct& nBlkRct);
	Void fillHoles (const CFloatImage *pfiFillMask);
	own CFloatImage* smooth_ (UInt window) const; // smooth a binarized fi of window

	// for block padding
	Void dilation4 (CFloatImage* pMask);
	Void dilation8 (CFloatImage* pMask);
};

#endif //_GRAYF_HPP_	 

	 

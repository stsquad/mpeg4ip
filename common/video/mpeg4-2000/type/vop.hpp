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

	vop.hpp

Abstract:

	class for Video Object Plane 

Revision History:

*************************************************************************/

#ifndef __VOP_HPP_ 
#define __VOP_HPP_


#include <stdio.h>

class CU8Image;
class CFloatImage;
class CIntImage;
class CTransform;

class CVideoObjectPlane
{
public:
	// Constructors
	~CVideoObjectPlane ();
	CVideoObjectPlane (const CVideoObjectPlane& vop, CRct r = CRct ()); // copy constructor
	CVideoObjectPlane (CRct r = CRct (), CPixel pxl = transpPixel); 
	CVideoObjectPlane ( // load a VOP from MPEG4 file
		const Char* pchFileName, // file name 
		UInt ifr, // frame number, 0 is always the base number
		const CRct& rct, // rect for the VOP
		ChromType chrType, // chrominance type
		Int nszHeader = 0 // header to be skipped	
	); 
	CVideoObjectPlane (const Char* vdlFileName); // load from a vdl stream

	// Attributes
	Bool valid () const {return this != 0;}
	CRct where () const {return m_rc;}
	CPixel pixel (CoordI x, CoordI y) const {return m_ppxl [m_rc.offset (x, y)];}
	CPixel pixel (CoordI x, CoordI y, UInt accuracy) const; 
	CPixel pixel (const CSite& st, UInt accuracy) const {return pixel (st.x, st.y, accuracy);} // bi-linear interpolation
	CPixel pixel (const CSite& st) const {return pixel (st.x, st.y);}
	CPixel pixel (CoordD x, CoordD y) const; // bi-linear interpolation
	CPixel pixel (const CSiteD& std) const {return pixel (std.x, std.y);} // bi-linear interpolation	
	const CPixel* pixels () const {return m_ppxl;} // return pointer
	const CPixel* pixels (CoordI x, CoordI y) const {return m_ppxl + m_rc.offset (x, y);}
	const CPixel* pixels (const CSite& st) const {return pixels (st.x, st.y);}
	CRct whereVisible() const;  // smallest rect containing all non transparent pixels

	// Resultants	
	own CVideoObjectPlane* warp (const CAffine2D& aff) const; // affine warp
	own CVideoObjectPlane* warp (const CAffine2D& aff, const CRct& rctWarp) const; // affine warp to rctWarp
	own CVideoObjectPlane* warp (const CPerspective2D& persp) const; // perspective warp
	own CVideoObjectPlane* warp (const CPerspective2D& persp, const CRct& rctWarp) const; // perspective warp to rctWarp
	own CVideoObjectPlane* warp (const CPerspective2D& persp, const CRct& rctWarp, UInt accuracy) const;
	own CVideoObjectPlane* decimate (UInt rateX, UInt rateY) const; // decimate the vframe by rateX and rateY.
	own CVideoObjectPlane* zoomup (UInt rateX, UInt rateY) const;
	own CVideoObjectPlane* expand (UInt rateX, UInt rateY) const;
	own CVideoObjectPlane* biInterpolate () const;  // bilinearly interpolate the vframe
	own CVideoObjectPlane* biInterpolate (UInt accuracy) const;  // bilinearly interpolate the vframe
	own CVideoObjectPlane* biInterpolate (const CRct& r) const;  // bilinearly interpolate the vframe
	own CFloatImage* plane (RGBA pxlCom) const; // get a plane
	Void vdlDump (const Char* fileName = "tmp.vdl", CPixel ppxlFalse = CPixel (0, 0, 0, 0)) const;
	Void vdlByteDump (const Char* fileName = "tmp.vdl", CPixel ppxlFalse = CPixel (0, 0, 0, 0)) const;
	Void dump (FILE* pfFile, ChromType = FOUR_TWO_TWO) const;
	Void dumpAlpha (FILE* pfFile) const;
	Void dumpAbekas (FILE* pfFile) const; //dump abekas file

	// Operations
	Void where (const CRct& r);	// crop based on r
	Void pixel (CoordI x, CoordI y, CPixel p) {m_ppxl [m_rc.offset (x, y)] = p;} // set a pixel
	Void pixel (const CSite& st, CPixel p) {pixel (st.x, st.y, p);} //  set a pixel
	Void lightChange (Int rShift, Int gShift, Int bShift); // lighting change
	Void falseColor (CPixel pxl); // replace the rgb values of the transparent pixels by pxl
	Void falseColor (U8 r, U8 g, U8 b); // replace the rgb values of the transparent pixels by r, g, b
	Void shift (Int nX, Int nY) {m_rc.shift (nX,nY);} // shift CRct by nX and nY
	Void mirrorPad (); // mirror padding
	Void repeatPad (); // repetitive padding
	Void multiplyAlpha (); // multiply alpha
	Void multiplyBiAlpha (); // multiply alpha
	Void unmultiplyAlpha ();
	Void thresholdAlpha (U8 uThresh); // convert 8-bit alpha to binary alpha
	Void thresholdRGB (U8 uThresh); // threshold RGB components
	Void cropOnAlpha (); // Crop based on the alpha plane 
	Void overlay (const CVideoObjectPlane& vop); // overlay VOP on top of *this
	Void setPlane (const CFloatImage& fi, RGBA pxlCom); // set a plane
	Void setPlane (const CIntImage& ii, RGBA pxlCom); // set a plane  
	Void setPlane (const CU8Image& ci, RGBA pxlCom); // set a plane	 
	Void getDownSampledPlane(CFloatImage &fi,Int plane,Int sx,Int sy); // downsample plane into float image
	Void setUpSampledPlane(const CFloatImage &fi,Int plane,Int sx,Int sy); // upsample float image into plane
	Void getDownSampledPlane(CIntImage &fi,Int plane,Int sx,Int sy); // downsample plane into int image
	Void setUpSampledPlane(const CIntImage &fi,Int plane,Int sx,Int sy); // upsample int image into plane
	Void rgbToYUV ();
	Void yuvToRGB ();

	// Overloaded operators
	own CVideoObjectPlane* operator * (const CTransform& tf) const;
	own CVideoObjectPlane* operator * (const CAffine2D& aff) const {return warp (aff);} // affine warp
	own CVideoObjectPlane* operator * (const CPerspective2D& persp) const {return warp (persp);} // perspective warp	
	own CVideoObjectPlane* operator - (const CVideoObjectPlane& vop) const;

///////////////// implementation /////////////////

private:
	CPixel* m_ppxl;
	CRct m_rc;

	Void allocate (CRct r, CPixel pxl = transpPixel); 
	Void copyConstruct (const CVideoObjectPlane& vop, CRct r = CRct());
	Void swap (CVideoObjectPlane& vop); 
};

#endif // __VOP_HPP_

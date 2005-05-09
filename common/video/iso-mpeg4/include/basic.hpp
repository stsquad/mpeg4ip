/*************************************************************************

This software module was originally developed by 

	Ming-Chieh Lee (mingcl@microsoft.com), Microsoft Corporation
	Wei-ge Chen (wchen@microsoft.com), Microsoft Corporation
	Bruce Lin (blin@microsoft.com), Microsoft Corporation
	Chuang Gu (chuanggu@microsoft.com), Microsoft Corporation
	(date: March, 1996)

and edited by
    Fujitsu Laboratories Ltd. (contact: Eishi Morimatsu)

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
assign or donate the code to a third party and to inhibit third parties from using the code for non MPEG-4 Video conforming products. 
This copyright notice must be included in all copies or derivative works. 

Copyright (c) 1996, 1997.

Module Name:

	basic.hpp

Abstract:

    Basic types:
		Data, CSite, CVector2D, CVector3D, CRct, CPixel, CMotionVector, CMatrix3x3D

Revision History:
	Sep.06	1999 : RRV added by Eishi Morimatsu (Fujitsu Laboratories Ltd.) 

*************************************************************************/


#ifndef __BASIC_HPP_ 
#define __BASIC_HPP_
#include "assert.h"
#include "string.h"

#ifdef __MAKE_DLL_
#define API __declspec (dllexport)
//#define Class        class API
#else
#define API
//#define Class		class
#endif // __MAKE_DLL_

#ifdef __MFC_
#include <afx.h>
#include <afxtempl.h>
#include <windowsx.h>
#endif // __MFC_

#define own // used as "reserved word" to indicate ownership or transfer to distinguish from const
#define TRUE 1
#define FALSE 0
#ifndef NULL
#define NULL 0
#endif
#define transpValue 0
#define opaqueValue 255
#define transpValueF 0.0F
#define opaqueValueF 255.0F

/*BBM// Added for Boundary by Hyundai(1998-5-9)
#define BBM             TRUE
#define BBS             FALSE
// End of Hyundai(1998-5-9)*/

#ifdef MPEG4_TRANSPARENT
#undef MPEG4_TRANSPARENT
#endif
#ifdef MPEG4_OPAQUE
#undef MPEG4_OPAQUE
#endif
#define MPEG4_TRANSPARENT 0
#define MPEG4_OPAQUE 255

#define transpPixel CPixel(0,0,0,0)
#define opaquePixel CPixel(255,255,255,255)

#ifndef max
#define max(a,b) (((a) > (b)) ? (a) : (b))
#endif
#ifndef min
#define min(a, b)  (((a) < (b)) ? (a) : (b))
#endif
#define signOf(x) (((x) > 0) ? 1 : 0)
#define invSignOf(x) ((x) > 0 ? 0 : 1)					// see p.22/H.263
#define sign(x) ((x) > 0 ? 1 : -1)					// see p.22/H.263
#define rounded(x) ((x) > 0 ? x + 0.5 : x - 0.5)

/////////////////////////////////////////////
// 
//      Forward declarations for classes
// 
/////////////////////////////////////////////
class CSite;
class CVector2D;
class CRct;
class CPixel; // 32 bit pixel, various interpretations depending on PixelType
class CMotionVector;


/////////////////////////////////////////////
// 
//  Typedefs for basic types
// 
/////////////////////////////////////////////

#define Sizeof(x) ((unsigned long) sizeof (x))

#if 1 // in case future compilers redefine longs and shorts
typedef unsigned long U32; 
typedef int I32; 
typedef unsigned short U16; 
typedef short I16; 

///// WAVELET VTC: begin ///////////////////////////////
typedef unsigned short UShort;  // hjlee
typedef short Short;   // hjlee
///// WAVELET VTC: end ///////////////////////////////


#endif // in case future compilers redefine longs and shorts 

typedef unsigned UInt; 
typedef double Double; 

#ifdef __DOUBLE_PRECISION_
typedef double Float; 
#else
typedef float Float; 
#endif // __DOUBLE_PRECISION_

typedef float Float32; 
typedef float F32; 
typedef unsigned char U8; 
typedef signed char I8; 
///// WAVELET VTC: begin ///////////////////////////////
typedef unsigned char UChar;  // hjlee
///// WAVELET VTC: end ///////////////////////////////
typedef long Long; 
typedef unsigned long ULong; 
typedef int Int; 
typedef void Void; 
typedef int Bool; 
typedef long CoordI; 
typedef double CoordD; 
typedef char Byte; 
typedef char Char; 
typedef enum {red, green, blue, alpha} RGBA; // define pixel component
typedef enum {zero, repeat} PadMethod; // define padding technique
typedef enum {Q_H263, Q_MPEG} Quantizer; 
typedef enum {IVOP, PVOP, BVOP, SPRITE, UNKNOWNVOPTYPE} VOPpredType;
typedef enum {B_FORWARD, B_BACKWARD} ShapeBPredDir;
typedef enum BlockNum {
	ALL_Y_BLOCKS	= 0,
	Y_BLOCK1		= 1, 
	Y_BLOCK2		= 2, 
	Y_BLOCK3		= 3, 
	Y_BLOCK4		= 4, 
	U_BLOCK			= 5, 
	V_BLOCK			= 6,
	A_BLOCK1		= 7, 
	A_BLOCK2		= 8, 
	A_BLOCK3		= 9, 
	A_BLOCK4		= 10, 
	ALL_A_BLOCKS	= 11
} BlockNum;
typedef enum PlaneType {Y_PLANE, U_PLANE, V_PLANE, A_PLANE, BY_PLANE, BUV_PLANE} PlaneType;
typedef enum AlphaUsage {RECTANGLE, ONE_BIT, EIGHT_BIT} AlphaUsage;
typedef enum ChromType {FOUR_FOUR_FOUR, FOUR_TWO_TWO, FOUR_TWO_ZERO} ChromType;
typedef enum EntropyCodeType {huffman, arithmetic} EntropyCodeType;
typedef enum TransparentStatus {ALL, PARTIAL, NONE} TransparentStatus;
typedef enum {STOP, PIECE, UPDATE, PAUSE, NEXT} SptXmitMode;
typedef enum {BASIC_SPRITE, LOW_LATENCY, PIECE_OBJECT, 
		PIECE_UPDATE} SptMode;	// basic sprite, and low-latency (object only, update only, intermingled) 

typedef Int Time;

/////////////////////////////////////////////
// 
//  Space
// 
/////////////////////////////////////////////

class CSite
{
public:  
	CoordI x; 
	CoordI y; 

	// Constructors
	CSite () {}
	CSite (const CSite& s) {x = s.x; y = s.y;}
	CSite (CoordI xx, CoordI yy) {x = xx; y = yy;}

	// Properties
	CoordI xCoord () const {return x;}
	CoordI yCoord () const {return y;}

	// Operators
	Void set (CoordI xx, CoordI yy) {x = xx; y = yy;}
	CSite operator + (const CSite& st) const; // Coornidate-wise +
	CSite operator - (const CSite& st) const; // Coornidate-wise -
	CSite operator * (const CSite& st) const; // Coornidate-wise *
	CSite operator * (Int scale) const; // Coornidate-wise scaling
	CSite operator / (const CSite& st) const; // Coornidate-wise /
	CSite operator / (Int scale) const; // Coornidate-wise scaling
	CSite operator % (const CSite& st) const; // Coornidate-wise %
	Void operator = (const CSite& st);

	// Synonyms
	Bool operator == (const CSite& s) const {return x == s.x && y == s.y;}
	Bool operator != (const CSite& s) const {return x != s.x || y != s.y;}
}; 

typedef CSite CVector;

class CVector2D
{
public:  
	CoordD x; 
	CoordD y; 

	// Constructors
	CVector2D () {}
	CVector2D (CoordD xx, CoordD yy) {x = xx; y = yy;}

	// Operators
	Void set (CoordD xx, CoordD yy) {x = xx; y = yy;}
	Void operator = (const CVector2D& v) {x = v.x; y = v.y;}

	// Resultants
	CoordD squared () const {return x * x + y * y;}
	CVector2D operator + (const CVector2D& v) const 
		{return CVector2D (x + v.x, y + v.y);} 
	CVector2D operator - (const CVector2D& v) const 
		{return CVector2D (x - v.x, y - v.y);} 
	Double operator * (const CVector2D& v) const // inner product
		{return x * v.x + y * v.y;} 
	CVector2D operator * (Double alpha) const // times scalar
		{return CVector2D (x * alpha, y * alpha);} 
	CVector2D operator / (Double alpha) const // times scalar
		{return (alpha == 0) ? *this : CVector2D (x / alpha, y / alpha);} 
	Bool operator == (const CVector2D& v) const
		{return (x == v.x && y == v.y);}
	Bool operator != (const CVector2D& v) const
		{return (x != v.x || y != v.y);}
	CVector2D rot90 () const // rotation 90 degrees
		{return CVector2D (-y, x);} 
	CVector2D rot270 () const // rotation 270 degrees
		{return CVector2D (y, -x);} 

///////////////// implementation /////////////////
protected:  
};

typedef CVector2D CSiteD;

class CRct
{
public:  
	CoordI left, top, right, bottom;
	Int width; // width is needed almost for every Rect.  So have a member to avoid extra computations.

	// Constructors
	CRct () 
		{invalidate ();}
	CRct (const CRct& r) 
		{left = r.left; top = r.top; right = r.right; bottom = r.bottom; width = r.width;}
	CRct (const CSite& s) // one-point CRct
		{left = s.x; top = s.y; right = s.x + 1; bottom = s.y + 1; width = 1;}
	CRct (CoordI l, CoordI t, CoordI r, CoordI b)
		{left = l; top = t; right = r; bottom = b; width = right - left;}
	CRct (CoordI x, CoordI y)
		{left = x; top = y; right = x + 1; bottom= y + 1; width = 1;}
	CRct (const CSite& st1, const CSite& st2);
	CRct (Int radius)
		{left = -radius; top = -radius; right = radius + 1; bottom = radius + 1; width = 2 * radius + 1;}
	CRct (CoordI x, CoordI y, Int radius)
		{left = x - radius; top = y - radius; right= x + radius + 1; bottom = y + radius + 1; width = 2 * radius + 1;}
	CRct (const CSite& st, Int radiusX, Int radiusY); // centered at st, expanded by radiusX and radiusY in each direction
	CRct ( // a Rect that includes all four sites
		const CSiteD& stdLeftTop,
		const CSiteD& stdRightTop,
		const CSiteD& stdLeftBottom,
		const CSiteD& stdRightBottom
	);

	// Attributes 
	Bool valid () const // check whether the CRct is valid
		{return left < right && top < bottom;} 
	Bool empty () const 
		{return left >= right || top >= bottom;} 
//	CoordI width () const 
//		{return !valid () ? 0 : (right - left);}
	CoordI height () const 
		{return !valid () ? 0 : (bottom - top);} 
	UInt area () const 
		{return (UInt) width * height ();} 
	CSite center () const 
		{return CSite ((left + right) >> 1, (top + bottom) >> 1);}
	UInt offset (CoordI x, CoordI y) const // index into linear array
		{return !valid () ? 0 : (UInt) width * (y - top) + (x - left);} 
	Bool includes (const CSite& s) const
		{return includes (s.x, s.y);}
	Bool includes (CoordI x, CoordI y) const 
		{return x >= left && x < right && y >= top && y < bottom;}
	Bool includes (const CRct& rc) const
	{
		return 
			includes(rc.left, rc.top) && 
			includes(rc.left, rc.bottom - 1) &&
			includes(rc.right - 1, rc.top) && 
			includes(rc.right - 1, rc.bottom - 1);
	}

	// Operations
	Void invalidate () 
		{left = top = 0; right = bottom = -1;} 
	Void expand (CoordI dl, CoordI dt, CoordI dr, CoordI db)
		{left -= dl; top -= dt; right += dr; bottom += db; width += dr + dl;}
	Void expand (Int expandSize) // same expansion size for all four directions
		{expand (expandSize, expandSize, expandSize, expandSize);}
	Void shift (CoordI dx, CoordI dy)               
		{left += dx; top += dy; right += dx; bottom += dy;}
	Void transpose (); // transpose the width and height, left-top corner remains the same
	Void rightRotate (); // Rotate the CRct by 90 degrees, center remains the same
	Void clip (const CRct& rc); // intersect
	Void include (const CRct& rc); // union
	Void include (const CSite& s);
	Void include (CoordI x, CoordI y) {include (CSite (x, y));}
	CRct downSampleBy2 () const;
	CRct upSampleBy2 () const;

	// Operators
	Void operator = (const CRct& rc);
	Bool operator == (const CRct& rc) const; 
	Bool operator != (const CRct& rc) const {return !(operator == (rc));}
	Bool operator <= (const CRct& rc) const;
	Bool operator >= (const CRct& rc) const;

	// Resultants
	CRct operator / (Int scale) const; 
		// divide the width and height by scale
		// round if the original size cannot be divided by scale
	CRct operator * (Int scale) const; 
		// scale the width and height by scale
}; 



/////////////////////////////////////////////
// 
//  Pixels
// 
/////////////////////////////////////////////

class CPixel // 32 bit pixel, various interpretations depending on PixelType
{
public:  
	union 
	{
		U32 bits; 
		struct rgb 
		{
			U8 r:8, g:8, b:8, a:8; 
		} rgb; 
		struct yuv
		{
			U8 y:8, u:8, v:8, a:8; 
		} yuv; 
		struct xy 
		{
			I32 x :16, y :16; 
		} xy;
		U8 color[4];
	} pxlU; 

	CPixel () {} // uninitialized
	CPixel (const U32 u) {pxlU.bits = u;} // copy a long
	CPixel (U8 r, U8 g, U8 b, U8 a)
		{pxlU.bits = (U32) ((((((U32) r) | (((U32) g) << 8)) | (((U32) b) << 16)) | (((U32) a) << 24)));}
	operator U32 () {return pxlU.bits;}
	CPixel operator & (U32 u) {return CPixel (pxlU.bits & u);}
	CPixel operator & (Int u) {return CPixel (pxlU.bits & u);}
	CPixel operator & (CPixel p) {return CPixel (pxlU.bits & p.pxlU.bits);}
	CPixel operator | (CPixel p) {return CPixel (pxlU.bits | p.pxlU.bits);}
	CPixel operator ~ () {return CPixel (~pxlU.bits);}
}; 

typedef CPixel* PixelPtr; 


/////////////////////////////////////////////
// 
//  Motion Vectors
// 
/////////////////////////////////////////////


class CMotionVector
{
public:
	CVector m_vctTrueHalfPel;
// RRV insertion
	CVector m_vctTrueHalfPel_x2;
// ~RRV
	Int iMVX; // x direction motion
	Int iMVY; // y direction motion
	Int iHalfX; // x direction half pixel. 3 values: -1, 0, 1
	Int iHalfY; // x direction half pixel. 3 values: -1, 0, 1

	// Constructor
	CMotionVector (const CVector& vctHalfPel);
	CMotionVector (Int ix, Int iy, Int iHx = 0, Int iHy = 0) {iMVX = ix; iMVY = iy; iHalfX = iHx; iHalfY = iHy; computeTrueMV ();}
	CMotionVector () {setToZero ();}

	// Attributes
	const CVector& trueMVHalfPel () const {return m_vctTrueHalfPel;}
// RRV insertion
	const CVector& trueMVHalfPel_x2 () const {return m_vctTrueHalfPel_x2;}
// ~RRV
	Bool isZero () const;

	// Operations
	Void operator = (const CMotionVector& mv);
	Void operator = (const CVector& vctHalfPel);
	Void setToZero ();
	Void computeTrueMV (); // compute trueMV (CVector) from MV (CMotionVector)
	Void computeMV (); // compute MV (CMotionVector) from trueMV (CVector)

	// Resultants
	CMotionVector operator + (const CMotionVector& mv) const;
	CMotionVector operator - (const CMotionVector& mv) const;

// RRV insertion
	Void scaleup ();
// ~RRV
};


#endif // __BASIC_HPP_


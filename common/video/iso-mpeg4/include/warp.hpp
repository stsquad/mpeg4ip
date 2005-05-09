/*************************************************************************

This software module was originally developed by 

	Ming-Chieh Lee (mingcl@microsoft.com), Microsoft Corporation
	Wei-ge Chen (wchen@microsoft.com), Microsoft Corporation
	Bruce Lin (blin@microsoft.com), Microsoft Corporation
	Chuang Gu (chuanggu@microsoft.com), Microsoft Corporation
	(date: March, 1996)

and also edited by

    Yuichiro Nakaya (Hitachi, Ltd.)
    Yoshinori Suzuki (Hitachi, Ltd.)

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

	warp.hpp

Abstract:

	Geometry transformation:
		2D Affine
		2D Perspective

Revision History:
	Jan. 13, 1999: Classes CSiteWFlag and CSiteDWFlag added and 
		definition of the "*" operator for the "CPerspective2D"
		class revised by Hitachi, Ltd. for disallowing zero 
		demoninators in perspective warping. 

*************************************************************************/

#ifndef __WARP_HPP_
#define __WARP_HPP_


class CMatrix2x2D;
class CAffine2D;
class CPerspective2D;

class CMatrix2x2D 
{
public:  
	// Constructors 
	CMatrix2x2D () {} 
	CMatrix2x2D (Double d); // diagonal matrix
	CMatrix2x2D (Double d00, Double d01, Double d10, Double d11);
	CMatrix2x2D (const CMatrix2x2D& m) {memcpy (this, &m, sizeof (*this));} 
	CMatrix2x2D (const CVector2D v [2]); // from column vectors
	CMatrix2x2D (const CVector2D& v0, const CVector2D& v1, Bool fAsColumns = TRUE); // from column vectors
	CMatrix2x2D (const CVector2D source [2], const CVector2D dest [2]); // maps source to dest
	CMatrix2x2D (const CVector2D& s0, const CVector2D& s1, const CVector2D& d0, const CVector2D& d1); // maps source to dest

	// Properties
	Void expose (); // debug
	Double determinant () const 
		{return m_value [0] [0] * m_value [1] [1] - m_value [0] [1] * m_value [1] [0];} 
	CoordD element (UInt row, UInt col) const {return m_value [row] [col];}

	// Operators
	CMatrix2x2D inverse () const; 
	Void transpose (); 
	CVector2D apply (const CVector2D& v) const; 
	CVector2D operator * (const CVector2D& v) const {return apply (v);}
	CMatrix2x2D operator * (const CMatrix2x2D& x) const; 

///////////////// implementation /////////////////
protected:  
	CoordD m_value [2] [2]; 
};


class CAffine2D 
{
public:  
	// Constructors
	CAffine2D () {} 
	CAffine2D (const CSiteD& source, const CSiteD& dest); // translation
	CAffine2D (const CSiteD& s0, const CSiteD& s1, const CSiteD& d0, const CSiteD& d1); // rotate and stretch
	CAffine2D (
		const CSiteD& s, const CVector2D& sv0, const CVector2D& sv1, 
		const CSiteD& d, const CVector2D& dv0, const CVector2D& dv1
	); 
	CAffine2D (const CSiteD source [3], const CSiteD dest [3]); 
	CAffine2D (const CoordD params [6]);
	
	// Properties
	Double determinant () const {return m_mtx.determinant ();}
	Void getParams (CoordD params [6]) const;

	// Operations
	CSiteD apply (const CSiteD& s) const {return m_stdDst + m_mtx * (s - m_stdSrc);}
	CSiteD operator * (const CSiteD& s) const {return apply (s);}
	CAffine2D inverse () const; 
	CAffine2D setOrigin (const CSiteD& std) const;

///////////////// implementation /////////////////
protected:  
	CMatrix2x2D m_mtx; 
	CSiteD m_stdSrc; 
	CSiteD m_stdDst; 
}; 
 
class CSiteWFlag
{
public: 
	CSite s; 
//	CoordI x; 
//	CoordI y; 
	Bool f;

	// Constructors
	CSiteWFlag () {}
	CSiteWFlag (const CSite& s0) {s.x = s0.x; s.y = s0.y; f = FALSE;}
	CSiteWFlag (const CSiteWFlag& s0) {s.x = s0.s.x; s.y = s0.s.y; f = s0.f;}
	CSiteWFlag (CoordI xx, CoordI yy, Bool ff) {s.x = xx; s.y = yy; f = ff;}

	// Properties
	CoordI xCoord () const {return s.x;}
	CoordI yCoord () const {return s.y;}
	Bool flag () const {return f;}
}; 

class CSiteDWFlag
{
public:  
	CSiteD s;
//	CoordD x; 
//	CoordD y; 
	Bool f;

	// Constructors
	CSiteDWFlag () {}
	CSiteDWFlag (const CSiteD& s0) {s.x = s0.x; s.y = s0.y; f = FALSE;}
	CSiteDWFlag (const CSiteDWFlag& s0) {s.x = s0.s.x; s.y = s0.s.y; f = s0.f;}
	CSiteDWFlag (CoordD xx, CoordD yy, Bool ff) {s.x = xx; s.y = yy; f = ff;}

	// Properties
	CoordD xCoord () const {return s.x;}
	CoordD yCoord () const {return s.y;}
	Bool flag () const {return f;}
}; 

class CPerspective2D 
{
public:  
	// Constructors
	~CPerspective2D ();
	CPerspective2D (const CSiteD source [4], const CSiteD dest [4]);
	CPerspective2D (const UInt pntNum, const CSiteD source [4], const CSiteD dest [4], const UInt accuracy);
	CPerspective2D (Double* rgCoeff);
	CPerspective2D ();

	// Operators
	CSiteDWFlag apply (const CSiteD& s0) const;
	CSiteWFlag apply (const CSite& s0) const;
	CSiteDWFlag operator * (const CSiteD& s0) const {return apply (s0);} 
	CSiteWFlag operator * (const CSite& s0) const {return apply (s0);}

	// Properties
	Double* getParams () {return m_rgCoeff;}

	// Resultants
	CPerspective2D inverse () const;

///////////////// implementation /////////////////
private:  
	Double* m_rgCoeff;		// perspective coefficients 
	CSiteD m_rgstdSrc [4];	// Source sites 
	CSiteD m_rgstdDst [4];	// Source sites 
	CoordD m_x0, m_y0;		// used in 2/4/8/16 quantization for warping
}; 
#endif //  __WARP_HPP_


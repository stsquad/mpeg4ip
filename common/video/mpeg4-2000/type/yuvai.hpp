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

	yuva.hpp

Abstract:

	YUVBA VOP: 
		There are 6 planes: Y, U, V, Binary for Y, Binary for UV, and Alpha.
		A is dynamically allocated depending on alpha usage.

Revision History:

*************************************************************************/

#ifndef __YUVAI_HPP_
#define __YUVAI_HPP_

class CVideoObjectPlane;
class CIntImage;
class CAffine2D;
class CPerspective2D;

class CVOPIntYUVBA
{
public:
	// Constructors
	~CVOPIntYUVBA ();
	CVOPIntYUVBA (const CVOPIntYUVBA& vopi, AlphaUsage fAUsage, const CRct& rc = CRct ());
	CVOPIntYUVBA (const CVOPIntYUVBA& vopi, const CRct& rc = CRct ());
	CVOPIntYUVBA (const CVideoObjectPlane& vop, AlphaUsage fAUsage, Int iAuxCompCount, const CRct& rc = CRct ());
	CVOPIntYUVBA (AlphaUsage fAUsage, Int iAuxCompCount, const CRct& rc);
	CVOPIntYUVBA (AlphaUsage fAUsage, Int iAuxCompCount, const CRct& rcY, const CRct& rcUV); // rcY is the rect for Y-plane. rcUV is the rect for UV plane
	CVOPIntYUVBA (AlphaUsage fAUsage = RECTANGLE);

	// Attributes
	Bool valid () const {return this != 0;}
	const CRct& whereY () const {return m_piiY -> where ();}
	const CRct& whereUV () const {return m_piiU -> where ();}
	const CIntImage* getPlane (PlaneType plnType) const;
  const CIntImage* getPlaneA (Int iAuxComp) const; // MAC (SB) 26-Nov-1999
	AlphaUsage fAUsage () const {return m_fAUsage;}
  Int auxCompCount() const {return m_iAuxCompCount;}

	// Operations
	Void where (const CRct& rct);
	Void whereY (const CRct& rct); // crop Y
	Void whereUV (const CRct& rct); // crop UV
	Void setPlane (const CIntImage* pii, PlaneType plnType, Bool bBUV = FALSE);
  Void setPlaneA (const CIntImage* pii, Int iAuxComp );
	Void cropOnAlpha ();
	Void overlay (const CVOPIntYUVBA& vopi); // defined only for YUV planes
	Void overlayBY (const CIntImage& fiBY); // defined only for YUV planes

	// Resultants
/*
	own CVideoObjectPlane* pvopYUV () const;
	own CVideoObjectPlane* pvopRGB () const;
	own CVOPIntYUVBA* warp (const CAffine2D& aff) const; // affine warp
	own CVOPIntYUVBA* warp (const CAffine2D& aff, const CRct& rctWarp) const; // affine warp	to rctWarp
	own CVOPIntYUVBA* warp (const CPerspective2D& persp) const; // perspective warp
	own CVOPIntYUVBA* warp (const CPerspective2D& persp, const CRct& rctWarp) const; // perspective warp	to rctWarp
	own CVOPIntYUVBA* warp (const CPerspective2D& persp, const CRct& rctWarp, UInt accuracy) const; 
*/
	own CVOPIntYUVBA* warpYUV (const CPerspective2D& persp, const CRct& rctWarp) const; // perspective warp only YUV	to rctWarp
	own CVOPIntYUVBA* warpYUV (const CPerspective2D& persp, const CRct& rctWarp, UInt accuracy) const; // perspective warp only YUV	to rctWarp
	own CVOPIntYUVBA* operator + (const CVOPIntYUVBA& vopi) const; // only for YUV, A uses the original
	own CVOPIntYUVBA* operator - (const CVOPIntYUVBA& vopi) const; // only for YUV, A uses the original
	own CVOPIntYUVBA* operator * (Int vl) const; // pixel-wise scaling, only for YUV, A uses the original
	own CVOPIntYUVBA* operator / (Int vl) const; // pixel-wise dividing, only for YUV, A uses the original
//	own CVOPIntYUVBA* operator * (const CAffine2D& aff) const {return warp (aff);} // affine warp
//	own CVOPIntYUVBA* operator * (const CPerspective2D& persp) const {return warp (persp);} // perspective warp	
	own CVOPIntYUVBA* average (const CVOPIntYUVBA& vopi) const; // only for YUV, A uses the original
	own CVOPIntYUVBA* downsampleForSpatialScalability () const;
	own CVOPIntYUVBA* upsampleForSpatialScalability () const;
	Double* mse (const CVOPIntYUVBA& vopi) const;
	Double* snr (const CVOPIntYUVBA& vopi) const;
//	Void vdlDump (const Char* fileName = "tmp.vdl", CPixel ppxlFalse = CPixel (0, 0, 0, 0)) const;

///////////////// implementation /////////////////

private:
	AlphaUsage m_fAUsage; // alpha usage of this plane
  Int        m_iAuxCompCount; // number of alpha planes
	own CIntImage* m_piiY; // Y plane
	own CIntImage* m_piiU; // U plane
	own CIntImage* m_piiV; // V plane
	own CIntImage* m_piiBY; // binary support for Y
	own CIntImage* m_piiBUV; // binary support for UV
	own CIntImage** m_ppiiA; // (gray-scale) alpha planes

	Void constructFromVOPF (const CVOPIntYUVBA& vopi, const CRct& rc);
};


#endif // __YUVA_HPP

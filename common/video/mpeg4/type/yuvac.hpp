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

#ifndef __YUVAC_HPP_
#define __YUVAC_HPP_

Class CVideoObjectPlane;
Class CU8Image;
Class CAffine2D;
Class CPerspective2D;

Class CVOPU8YUVBA
{
public:
	// Constructors
	~CVOPU8YUVBA ();
	CVOPU8YUVBA (const CVOPU8YUVBA& vopuc, AlphaUsage fAUsage, const CRct& rc = CRct ());
	CVOPU8YUVBA (const CVOPU8YUVBA& vopuc, const CRct& rc = CRct ());
	CVOPU8YUVBA (const Char* sptFilename);
	CVOPU8YUVBA (const CVideoObjectPlane& vop, AlphaUsage fAUsage, const CRct& rc = CRct ());
	CVOPU8YUVBA (AlphaUsage fAUsage, const CRct& rc);
	CVOPU8YUVBA (AlphaUsage fAUsage = RECTANGLE);

	// Attributes
	const PixelC* pixelsY () const {return m_ppxlcY;}
	const PixelC* pixelsU () const {return m_ppxlcU;}
	const PixelC* pixelsV () const {return m_ppxlcV;}
	const PixelC* pixelsBY () const {return m_ppxlcBY;}
	const PixelC* pixelsBUV () const {return m_ppxlcBUV;}
	const PixelC* pixelsA () const {return m_ppxlcA;}
	const PixelC* pixelsBoundY () const {return m_ppxlcBoundY;}
	const PixelC* pixelsBoundU () const {return m_ppxlcBoundU;}
	const PixelC* pixelsBoundV () const {return m_ppxlcBoundV;}
	const PixelC* pixelsBoundBY () const {return m_ppxlcBoundBY;}
	const PixelC* pixelsBoundBUV () const {return m_ppxlcBoundBUV;}
	const PixelC* pixelsBoundA () const {return m_ppxlcBoundA;}
	const CRct& whereY () const {return m_rctY;}
	const CRct& whereUV () const {return m_rctUV;}
	const CRct& whereBoundY () const {return m_rctBoundY;}
	const CRct& whereBoundUV () const {return m_rctBoundUV;}

	Bool valid () const {return this != 0;}
	const CU8Image* getPlane (PlaneType plnType) const;
	AlphaUsage fAUsage () const {return m_fAUsage;}

	// Operations
	Void shift (CoordI left, CoordI top);
	Void setBoundRct (const CRct& rctBoundY);
	Void setAndExpandBoundRctOnly (const CRct& rctBoundY, Int iExpand);
	Void cropOnAlpha ();
	Void overlay (const CVOPU8YUVBA& vopuc); // 
	Void overlay (const CVOPU8YUVBA& vopc, const CRct& rctY); //overlay part of the source
	Void setPlane (const CU8Image* pfiSrc, PlaneType plnType);

	// Resultants
	own CVOPU8YUVBA* downsampleForSpatialScalability () const;
	own CVOPU8YUVBA* upsampleForSpatialScalability (Int iVerticalSamplingFactorM,
													Int iVerticalSamplingFactorN,
													Int iHorizontalSamplingFactorM,
													Int iHorizontalSamplingFactorN,
													Int iExpandYRefFrame,
													Int iExpandUVRefFrame) const;
	Double* mse (const CVOPU8YUVBA& vopuc) const;
	Double* snr (const CVOPU8YUVBA& vopuc) const;
	Void vdlDump (const Char* pchFileName, const CRct& rct = CRct ()) const;
	Void dump (FILE* pfFile) const;
	Void dump (const Char* pchFileName) const;
	Void addBYPlain();
	Void addBYPlain (const CRct& rct, const CRct& rctUV);	// new definition


///////////////// implementation /////////////////

private:

	// key members
	AlphaUsage m_fAUsage; // alpha usage of this plane
	own CU8Image* m_puciY; // Y plane
	own CU8Image* m_puciU; // U plane
	own CU8Image* m_puciV; // V plane
	own CU8Image* m_puciBY; // binary support for Y
	own CU8Image* m_puciBUV; // binary support for UV
	own CU8Image* m_puciA; // (gray-scale) alpha plane
	CRct m_rctY, m_rctUV;
	PixelC *m_ppxlcY, *m_ppxlcU, *m_ppxlcV, *m_ppxlcBY, *m_ppxlcBUV, *m_ppxlcA;

	// non-key members (only needed for some cases)
	CRct m_rctBoundY, m_rctBoundUV;
	PixelC *m_ppxlcBoundY, *m_ppxlcBoundU, *m_ppxlcBoundV; 
	PixelC *m_ppxlcBoundBY, *m_ppxlcBoundBUV, *m_ppxlcBoundA;


	Void constructFromVOPU8 (const CVOPU8YUVBA& vopuc, const CRct& rc);
};


#endif // __YUVA_HPP

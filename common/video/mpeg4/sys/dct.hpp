/*************************************************************************

This software module was originally developed by 

	Ming-Chieh Lee (mingcl@microsoft.com), Microsoft Corporation
	Wei-ge Chen (wchen@microsoft.com), Microsoft Corporation
	Bruce Lin (blin@microsoft.com), Microsoft Corporation
	Chuang Gu (chuanggu@microsoft.com), Microsoft Corporation
	(date: March, 1996)
and edited by
        Wei Wu (weiwu@stallion.risc.rockwell.com) Rockwell Science Center

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

	dct.hpp

Abstract:

	DCT and inverse DCT

Revision History:

*************************************************************************/


#ifndef __DCT_HPP_
#define __DCT_HPP_

Class CTransform;

Class CBlockDCT : public CTransform
{
public:
	// Constructors
	virtual ~CBlockDCT ();
/* NBIT: change
	CBlockDCT ();	
*/
	CBlockDCT (UInt nBits); // NBIT

	// Operations
	Void apply (const PixelC* rgiSrc, Int nColSrc, Int* rgiDst, Int nColDst);
	Void apply (const Int* rgiSrc, Int nColSrc, PixelC* rgchDst, Int nColDst);
	Void apply (const Int* rgiSrc, Int nColSrc, Int* rgchDst, Int nColDst);
   	
///////////////// implementation /////////////////
protected:

	UInt m_nBits; // NBIT
	// only used for 8x8 case
	Float m_c0;											//xformation costants
	Float m_c1;
	Float m_c2;
	Float m_c3;
	Float m_c4;
	Float m_c5;
	Float m_c6;
	Float m_c7;
	PixelC* m_rgchClipTbl;
	Float	m_rgfltBuf1[8];								//buffers
	Float 	m_rgfltBuf2[8];
	Float 	m_rgfltAfter1dXform[8];						//results of 1dDCT
	Float 	m_rgfltAfterRowXform[8][8];					//intermediate results

	Void xformRow (const PixelC* ppxlcRowSrc, CoordI i);
	Void xformRow (const PixelI* ppxlfRowSrc, CoordI i);//transform a row
	Void xformColumn (PixelC* ppxlcColDst, CoordI i, Int nColDst);
	Void xformColumn (PixelI* ppxlfColDst, CoordI i, Int nColDst);	//transfrom a col
	virtual Void oneDimensionalDCT () = 0;					//1d DCT

};

Class CFwdBlockDCT: public CBlockDCT								//forward DCT
{
public:
	// Constructors
	~CFwdBlockDCT () {}	
/* NBIT: change
	CFwdBlockDCT ();	
*/
	CFwdBlockDCT (UInt nBits=8);	
   	
///////////////// implementation /////////////////
protected:
	Void oneDimensionalDCT ();
};

Class CInvBlockDCT: public CBlockDCT								//inverse DCT
{
public:
	// Constructors
	~CInvBlockDCT () {}		
/* NBIT: change
	CInvBlockDCT ();	
*/
	CInvBlockDCT (UInt nBits=8);	
   	

///////////////// implementation /////////////////
protected:
	Void oneDimensionalDCT ();
};

#endif 
// __DCT_HPP_

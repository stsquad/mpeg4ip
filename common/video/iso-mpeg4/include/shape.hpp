/*************************************************************************

This software module was originally developed by 

	Wei-ge Chen (wchen@microsoft.com), Microsoft Corporation
	(date: April, 1997)

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

	shape.hpp

Abstract:

	binary shape codec

Revision History:

*************************************************************************/

#ifndef __SHAPE_HPP_
#define __SHAPE_HPP_


#define BAB_TOTAL_SIZE			20
#define BAB_BORDER_SIZE			2
#define BAB_BORDER_SIZE_DBL		4
#define MCBAB_BORDER_SIZE		1
#define MCBAB_BORDER_SIZE_DBL	2
#define MCBAB_TOTAL_SIZE		18

class CShapeCodec
{
public:
	// Constructors
	virtual ~CShapeCodec ();
	CShapeCodec (CVOPofMBs* pvopmbs);

	// Atributes
	const CIntImage* reconShape () {return m_pfiShapeRecon;}
	TransparentStatus trnspstts () const {return m_trnspstts;}
	ShapeMode encodingMode () const {return m_shpmd;}
	Bool interShape () const {assert (FALSE); return 0;}
	CVector mvDiff () const {return m_vctMVDS;}

	// operations
	Void setFirstMmr (TransparentStatus trnspstts, Bool bInterShapeCoding) {;}  //useless: only for sepr mode

protected:

	CVOPofMBs* m_pvopmbs;		//place to copy the source data from
	Int m_rgiTopBorder  [BAB_BORDER_SIZE] [BAB_TOTAL_SIZE];
	Int m_rgiLeftBorder [BAB_TOTAL_SIZE]  [BAB_BORDER_SIZE];
	Int m_rgiCurrShp	[BAB_TOTAL_SIZE]  [BAB_TOTAL_SIZE];
	Int m_rgiPredShp	[MCBAB_TOTAL_SIZE][MCBAB_TOTAL_SIZE];
	Int m_iContextNumber;

	ArCodec* m_parcodec;
	own CIntImage* m_pfiPred;
	Int m_iWidthOfAlphaMB;			//after downsampling
	CRct m_rct;
	TransparentStatus m_trnspstts;
	CAEScanDirection m_msdir;	//scanning direction
	ShapeMode m_shpmd;
	CIntImage* m_pfiShapeRecon;
	Int m_iInverseCR;
	CVector m_vctMVDS;

	// pre-processing functions
	own CIntImage* downSample (const CIntImage* pfiSrc, const CRct& rctSrc = CRct (), Int iRate = 1);
	own CIntImage* upSample (const CIntImage* pfiCurrDown, Int iRate = 1);
	Void readyReference (ShapeMode shpmd = UNKNOWN, Bool bDownSample = TRUE);
	Void transposeData (ShapeMode shpmd = UNKNOWN);
	Void computeContext (CoordI iX, CoordI iY, ShapeMode shpmd = UNKNOWN);
};

#endif // __SHAPE_HPP

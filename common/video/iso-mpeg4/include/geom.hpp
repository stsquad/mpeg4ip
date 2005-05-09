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

    geom.hpp

Abstract:

    2D geometry for polygons

Revision History:

*************************************************************************/

#ifndef __GEOM_HPP_ 
#define __GEOM_HPP_


class CPolygonI;

class CPolygonI {
public:
	// Constructors
	CPolygonI () : m_csit (0), m_rgsit (NULL)
		{}
	CPolygonI (const CPolygonI& poly);
	CPolygonI (UInt csit, const CSite* rgsit, Bool bCheckCorner = FALSE, const CRct& rc = CRct ());
	CPolygonI (const CRct& r);
	~CPolygonI () {delete [] m_rgsit, m_rgsit=NULL;}

	// operations
	Void crop (const CRct& rct);		//crop to stay in rct
	Void close ();		//close possible open polygons

	// Properties
	UInt nSites () const {return m_csit;}
	const CSite& operator [] (UInt i) const {return at (i);}
	Void unpack (UInt& csit, CSite*& rgsit) const;
	CPolygonI* sample (int rate=1, const CRct& rc = CRct());
	Void dump (const Char* pchFileName) const;

protected:
	UInt m_csit;
	CSite* m_rgsit;

	const CSite& at (Int isit) const 
		{assert (isit < (Int) m_csit && isit >= - (Int) m_csit);
		 return m_rgsit[isit >= 0 ? isit : (isit + (Int) m_csit)];}
	void allocate (UInt n);
	Void checkCorner (const CRct& rc);
};

#endif // __GEOM_HPP



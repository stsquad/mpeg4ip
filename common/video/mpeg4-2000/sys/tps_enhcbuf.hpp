/*************************************************************************

This software module was originally developed by 

        Norio Ito (norio@imgsl.mkhar.sharp.co.jp), Sharp Corporation
        Hiroyuki Katata (katata@imgsl.mkhar.sharp.co.jp), Sharp Corporation
        Shuichi Watanabe (watanabe@imgsl.mkhar.sharp.co.jp), Sharp Corporation
        (date: October, 1997)

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
Sharp retains full right to use the code for his/her own purpose, 
assign or donate the code to a third party and to inhibit third parties from using the code for non <MPEG standard> conforming products. 
This copyright notice must be included in all copies or derivative works. 

Copyright (c) 1997.

Module Name:

	tps_enhcbuf.hpp

Abstract:

	buffers for enhancement layer for temporal scalability

Revision History:

*************************************************************************/

class CVOPU8YUVBA;

class CEnhcBuffer
{
public:
	~CEnhcBuffer ();
	CEnhcBuffer (Int iSessionWidth, Int iSessionHeight);

	Void dispose ();	// dispose buffer pvopcBuf
	Bool empty ();		// whether pvopcBuf is NULL or not

protected:
//	CMBMode* m_rgmbmd;
	CMBMode* m_rgmbmdRef;
//	CMotionVector *m_rgmv;
	CMotionVector *m_rgmvRef;

//	Int m_iNumMB;
//	Int m_iNumMBX;
//	Int m_iNumMBY;
	Int m_iNumMBRef;
	Int m_iNumMBXRef;
	Int m_iNumMBYRef;

	Int m_iOffsetForPadY;
	Int m_iOffsetForPadUV;
	CRct m_rctPrevNoExpandY;
	CRct m_rctPrevNoExpandUV;
	Bool m_bCodedFutureRef; // added by Sharp (99/1/26)

	CRct m_rctRefVOPY0;
	CRct m_rctRefVOPUV0;
	CRct m_rctRefVOPY1;
	CRct m_rctRefVOPUV1;
	CRct m_rctRefVOPZoom; // 9/26

	CU8Image* m_puciZoomBuf; // 9/26
	CVOPU8YUVBA* m_pvopcBuf;
	Int m_t;
};

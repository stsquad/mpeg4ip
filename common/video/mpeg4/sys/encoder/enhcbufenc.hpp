/*************************************************************************

This software module was originally developed by 

	Ming-Chieh Lee (mingcl@microsoft.com), Microsoft Corporation
	Wei-ge Chen (wchen@microsoft.com), Microsoft Corporation
	Bruce Lin (blin@microsoft.com), Microsoft Corporation
	Chuang Gu (chuanggu@microsoft.com), Microsoft Corporation
	(date: March, 1996)

and edited by

	Hiroyuki Katata (katata@imgsl.mkhar.sharp.co.jp), Sharp Corporation
	Norio Ito (norio@imgsl.mkhar.sharp.co.jp), Sharp Corporation
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

	vopSeEncTPS.hpp

Abstract:

	Encoder for one VO for object based temporal scalability

Revision History:

*************************************************************************/

#ifndef __VOPSEENCTPS_HPP_ 
#define __VOPSEENCTPS_HPP_

class CEnhcBufferEncoder : public CEnhcBuffer
{
	friend class CSessionEncoder;

public:
	CEnhcBufferEncoder (Int iSessionWidth, Int iSessionHeight);

//	void dispose ();
//	bool empty ();
	Void copyBuf (const CEnhcBufferEncoder& srcBuf);
	Void getBuf (const CVideoObjectEncoder* pvopc);  // get params from Base layer
	Void putBufToQ0 (CVideoObjectEncoder* pvopc);  // store params to Enhancement layer
	Void putBufToQ1 (CVideoObjectEncoder* pvopc);  // store params to Enhancement layer
};
#endif


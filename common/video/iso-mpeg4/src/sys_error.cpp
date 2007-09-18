/*************************************************************************

This software module was originally developed by 

	Yoshihiro Kikuchi (TOSHIBA CORPORATION)
	Takeshi Nagai (TOSHIBA CORPORATION)

    and edited by:

	Toshiaki Watanabe (TOSHIBA CORPORATION)
	Noboru Yamaguchi (TOSHIBA CORPORATION)
     Sehoon Son (shson@unitel.co.kr) Samsung AIT

  in the course of development of the <MPEG-4 Video(ISO/IEC 14496-2)>. This
  software module is an implementation of a part of one or more <MPEG-4 Video
  (ISO/IEC 14496-2)> tools as specified by the <MPEG-4 Video(ISO/IEC 14496-2)
  >. ISO/IEC gives users of the <MPEG-4 Video(ISO/IEC 14496-2)> free license
  to this software module or modifications thereof for use in hardware or
  software products claiming conformance to the <MPEG-4 Video(ISO/IEC 14496-2
  )>. Those intending to use this software module in hardware or software
  products are advised that its use may infringe existing patents. The
  original developer of this software module and his/her company, the
  subsequent editors and their companies, and ISO/IEC have no liability for
  use of this software module or modifications thereof in an implementation.
  Copyright is not released for non <MPEG-4 Video(ISO/IEC 14496-2)>
  conforming products. TOSHIBA CORPORATION retains full right to use the code
  for his/her own purpose, assign or donate the code to a third party and to
  inhibit third parties from using the code for non <MPEG-4 Video(ISO/IEC
  14496-2)> conforming products. This copyright notice must be included in
  all copies or derivative works.
  Copyright (c)1997.

*************************************************************************/

#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <iostream>

#include "typeapi.h"
#include "codehead.h"
#include "mode.hpp"
#include "global.hpp"
#include "vopses.hpp"

#ifdef __MFC_
#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

#define new DEBUG_NEW				   
#endif // __MFC_

// Video Packet MB number
Int CVideoObject::VPMBnum(Int iMBX, Int iMBY)	const {return (iMBY * m_iNumMBX + iMBX);}

// Boundary decision
Bool CVideoObject::bVPNoLeft(Int iMBnum, Int iMBX)	const {
	return (iMBnum <= m_iVPMBnum || iMBX == 0);
}
Bool CVideoObject::bVPNoRightTop(Int iMBnum, Int iMBX)	const {return (iMBnum - m_iNumMBX + 1 < m_iVPMBnum || iMBX == m_iNumMBX - 1);}
Bool CVideoObject::bVPNoTop(Int iMBnum)	const {return(iMBnum - m_iNumMBX < m_iVPMBnum);}
Bool CVideoObject::bVPNoLeftTop(Int iMBnum, Int iMBX)	const {return(iMBnum - m_iNumMBX - 1 < m_iVPMBnum || iMBX == 0);}
//OBSS_SAIT_991015
Bool CVideoObject::bVPNoRight(Int iMBX)	const {return(iMBX == m_iNumMBX - 1);}
Bool CVideoObject::bVPNoBottom(Int iMBY)const {return(iMBY == m_iNumMBY - 1);}
//~OBSS_SAIT_991015

//	Added for data partitioning mode By Toshiba(1998-1-16:DP+RVLC)
Void CVideoObject::copyRefShapeToMb (
	PixelC* ppxlcDstMB, 
	const PixelC* ppxlcSrc
)
{
	for (Int i = 0; i < MB_SIZE; i++)	{
		memcpy (ppxlcDstMB, ppxlcSrc, MB_SIZE * sizeof(PixelC));
		ppxlcSrc += m_iFrameWidthY;
		ppxlcDstMB  += MB_SIZE;
	}
}
//	End Toshiba(1998-1-16:DP+RVLC)

Void fatal_error(char *pchMessage, Int iCond)
{
	if(iCond)
		return;

	fprintf(stderr,"******** ERROR ********\n");
	fprintf(stderr,"%s\n", pchMessage);
	fprintf(stderr,"***********************\n\n");
	exit(1);
}

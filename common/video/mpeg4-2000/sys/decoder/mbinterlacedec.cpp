/* 
 * Copyright (c) 1997 NextLevel Systems of Delaware, Inc.  All rights reserved.
 * 
 * This software module  was developed by  Bob Eifrig (at NextLevel
 * Systems of Delaware, Inc.), Xuemin Chen (at NextLevel Systems of
 * Delaware, Inc.), and Ajay Luthra (at NextLevel Systems of Delaware,
 * Inc.), in the course of development of the MPEG-4 Video Standard
 * (ISO/IEC 14496-2).   This software module is an implementation of a
 * part of one or more tools as specified by the MPEG-4 Video Standard.
 * 
 * NextLevel Systems of Delaware, Inc. grants the right, under its
 * copyright in this software module, to use this software module and to
 * make modifications to it for use in products which conform to the
 * MPEG-4 Video Standard.  No license is granted for any use in
 * connection with products which do not conform to the MPEG-4 Video
 * Standard.
 * 
 * Those intending to use this software module are advised that such use
 * may infringe existing and unissued patents.  Please note that in
 * order to practice the MPEG-4 Video Standard, a license may be
 * required to certain patents held by NextLevel Systems of Delaware,
 * Inc., its parent or affiliates ("NextLevel").   The provision of this
 * software module conveys no license, express or implied, under any
 * patent rights of NextLevel or of any third party.  This software
 * module is subject to change without notice.  NextLevel assumes no
 * responsibility for any errors that may appear in this software
 * module.  NEXTLEVEL DISCLAIMS ALL WARRANTIES, EXPRESS AND IMPLIED,
 * INCLUDING, BUT NOT LIMITED TO ANY WARRANTY THAT COMPLIANCE WITH OR
 * PRACTICE OF THE SPECIFICATIONS OR USE OF THIS SOFTWARE MODULE WILL
 * NOT INFRINGE THE INTELLECTUAL PROPERTY RIGHTS OF NEXTLEVEL OR ANY
 * THIRD PARTY, AND ANY IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS FOR A PARTICULAR PURPOSE.
 * 
 * NextLevel retains the full right to use this software module for its
 * own purposes, to assign or transfer this software module to others,
 * to prevent others from using this software module in connection with
 * products which do not conform to the MPEG-4 Video Standard, and to
 * prevent others from infringing NextLevel's patents.
 * 
 * As an express condition of the above license grant, users are
 * required to include this copyright notice in all copies or derivative
 * works of this software module.
 */
/***********************************************************CommentBegin******
 *
 * -- FrameFieldDCTDecide -- Determine if field DCT is better & swizzle 
 *
 * Purpose :  
 *			Determine if the macroblock should be field DCT code and
 *			reorder luminancemacroblock lines if the macroblock is
 *			field-coded.
 *
 * Return values : 
 *			Int FieldDCT : 0 if frame DCT, 1 if field DCT 
 *
 * Written by X. Chen, NextLevel System, Inc.
 *
 ***********************************************************CommentEnd********/

#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <iostream.h>

#include "typeapi.h"
#include "codehead.h"
#include "global.hpp"
#include "entropy/bitstrm.hpp"
#include "entropy/entropy.hpp"
#include "entropy/huffman.hpp"
#include "mode.hpp"
#include "vopses.hpp"
#include "vopsedec.hpp"

#ifdef __MFC_
#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

#define new DEBUG_NEW				   
#endif // __MFC_


/***********************************************************CommentBegin******
 *
 * -- fieldDCTtoFrame -- permute the intra- field-DCT block back to frame layout 
 *
 *
 ***********************************************************CommentEnd********/

Void
CVideoObjectDecoder::fieldDCTtoFrameC(PixelC* ppxlcRefMBY)
{
	static unsigned char inv_shuffle[] = {
        1, 0, 8, 1, 4, 8, 2, 4, 0, 2, 3, 0, 9, 3,
        12, 9, 6, 12, 0, 6, 5, 0, 10, 5, 0, 10, 7, 0,
        11, 7, 13, 11, 14, 13, 0, 14, };

    PixelC tmp[MB_SIZE];
    unsigned int i;

	for (i = 0; i < sizeof(inv_shuffle); i += 2)
                memcpy(inv_shuffle[i+1] ? ppxlcRefMBY+inv_shuffle[i+1]*m_iFrameWidthY : tmp,
                           inv_shuffle[i+0] ? ppxlcRefMBY+inv_shuffle[i+0]*m_iFrameWidthY : tmp,
                           MB_SIZE*sizeof(PixelC));
}


/***********************************************************CommentBegin******
 *
 * -- fieldDCTtoFrame -- permute the inter field DCT block back to frame layout 
 *
 *
 ***********************************************************CommentEnd********/

Void
CVideoObjectDecoder::fieldDCTtoFrameI(PixelI* m_ppxliErrorMBY)
{
	static unsigned char inv_shuffle[] = {
        16, 0, 128, 16, 64, 128, 32, 64, 0, 32, 48, 0, 144, 48,
        192, 144, 96, 192, 0, 96, 80, 0, 160, 80, 0, 160, 112, 0,
        176, 112, 208, 176, 224, 208, 0, 224,
                };
    Int tmp[MB_SIZE];
    unsigned int i;

	for (i = 0; i < sizeof(inv_shuffle); i += 2)
                memcpy(inv_shuffle[i+1] ? m_ppxliErrorMBY+inv_shuffle[i+1] : tmp,
                           inv_shuffle[i+0] ? m_ppxliErrorMBY+inv_shuffle[i+0] : tmp,
                           MB_SIZE*sizeof(PixelI));
}

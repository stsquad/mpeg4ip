/*
 * The contents of this file are subject to the Mozilla Public
 * License Version 1.1 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.mozilla.org/MPL/
 * 
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 * 
 * The Original Code is MPEG4IP.
 * 
 * The Initial Developer of the Original Code is Cisco Systems Inc.
 * Portions created by Cisco Systems Inc. are
 * Copyright (C) Cisco Systems Inc. 2000, 2001.  All Rights Reserved.
 * 
 * Contributor(s): 
 *		Dave Mackie			dmackie@cisco.com
 */

#include <mpeg4ip.h>

void FrameDoubler(u_int8_t* pSrcPlane, u_int8_t* pDstPlane, 
	u_int32_t srcWidth, u_int32_t srcHeight, u_int32_t destWidth)
{
	register u_int8_t p00, p01, p10, p11;
	register u_int32_t si0, si1, di0, di1;
	/* initialize these to keep compiler happy at -Werror */
	p00 = p01 = p10 = p11 = 0;

	for (u_int16_t row = 0; row < srcHeight - 1; row++) {
		si0 = 0;
		si1 = srcWidth;
		di0 = 0;
		di1 = destWidth;		/* setup for first column */
		p01 = pSrcPlane[si0++];
		p11 = pSrcPlane[si1++];

		for (u_int16_t col = 0; col < srcWidth-1; col++) {
			p00 = p01;
			p10 = p11; 
			p01 = pSrcPlane[si0++];
			p11 = pSrcPlane[si1++];

			pDstPlane[di0++] = p00;
			pDstPlane[di0++] = (p00 + p01) >> 1;
			pDstPlane[di1++] = (p00 + p10) >> 1;
			pDstPlane[di1++] = (p00 + p11) >> 1;
		}
		
		/* last column */
		pDstPlane[di0++] = p00;
		pDstPlane[di0++] = p00;
		p01 = (p00 + p10) >> 1;
		pDstPlane[di1++] = p01;
		pDstPlane[di1++] = p01;

		pSrcPlane += srcWidth;
		pDstPlane += destWidth * 2;
	}

	/* last row */
	di0 = 0;
	di1 = destWidth;
	for (u_int16_t col = 0; col < srcWidth-1; col++) {
		pDstPlane[di0++] = p00;
		pDstPlane[di1++] = p00;
		p10 = (p00 + p01) >> 1;
		pDstPlane[di0++] = p10;
		pDstPlane[di1++] = p10;
	}

	/* last pixel */
	pDstPlane[di0++] = p00;
	pDstPlane[di0] = p00;
	pDstPlane[di1++] = p00;
	pDstPlane[di1] = p00;
}

/* end file frame_doubler.cpp */


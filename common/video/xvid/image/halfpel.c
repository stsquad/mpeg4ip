/**************************************************************************
 *
 *	XVID MPEG-4 VIDEO CODEC
 *	mmx quantization/dequantization
 *
 *	This program is an implementation of a part of one or more MPEG-4
 *	Video tools as specified in ISO/IEC 14496-2 standard.  Those intending
 *	to use this software module in hardware or software products are
 *	advised that its use may infringe existing patents or copyrights, and
 *	any such use would be at such party's own risk.  The original
 *	developer of this software module and his/her company, and subsequent
 *	editors and their companies, will have no liability for use of this
 *	software or modifications or derivatives thereof.
 *
 *	This program is free software; you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation; either version 2 of the License, or
 *	(at your option) any later version.
 *
 *	This program is distributed in the hope that it will be useful,
 *	but WITHOUT ANY WARRANTY; without even the implied warranty of
 *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *	GNU General Public License for more details.
 *
 *	You should have received a copy of the GNU General Public License
 *	along with this program; if not, write to the Free Software
 *	Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *************************************************************************/

/**************************************************************************
 *
 *	History:
 *
 *	05.11.2001	initial version; (c)2001 peter ross <pross@cs.rmit.edu.au>
 *
 *************************************************************************/


#include "halfpel.h"

// halfpel function pointers
halfpelFuncPtr interpolate_halfpel_h;
halfpelFuncPtr interpolate_halfpel_v;
halfpelFuncPtr interpolate_halfpel_hv;


void interpolate_halfpel_h_c(uint8_t * const dst,
						const uint8_t * const src,
						const uint32_t width, 
						const uint32_t height,
						const uint32_t rounding)
{
	const uint32_t n = width * height - 1;
	uint32_t i;
	
	for (i = 0; i < n; i++) 
	{
		uint16_t tot = src[i] + src[i + 1];
		dst[i] = (tot + 1 - rounding) >> 1;
	}
}



void interpolate_halfpel_v_c(uint8_t * const dst,
						const uint8_t * const src,
						const uint32_t width, 
						const uint32_t height, 
						const uint32_t rounding)
{
	const uint32_t n = width * (height - 1);
	uint32_t i;
	
	for (i = 0; i < n; i++) 
	{
		uint16_t tot = src[i] + src[i + width];
		dst[i] = (tot + 1 - rounding) >> 1;
	}
}



void interpolate_halfpel_hv_c(uint8_t * const dst,
						const uint8_t * const src,
						const uint32_t width, 
						const uint32_t height, 
						const uint32_t rounding)
{
	const uint32_t n = width * (height - 1) - 1;
	uint32_t i;
	
	for (i = 0; i < n; i++) 
	{
		uint16_t tot = src[i] + src[i + 1] + src[i + width] + src[i + width + 1];
		dst[i] = (tot + 2 - rounding) >> 2;
	}
}

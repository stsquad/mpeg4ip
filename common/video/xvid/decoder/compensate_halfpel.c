/**************************************************************************
 *
 *	XVID MPEG-4 VIDEO CODEC
 *	block-based halfpel
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


#include "../portab.h"
#include "compensate_halfpel.h"

comphalfpelFuncPtr compensate_halfpel_h;
comphalfpelFuncPtr compensate_halfpel_v;
comphalfpelFuncPtr compensate_halfpel_hv;

// dst = interpolate(src) + data

void compensate_halfpel_h_c(uint8_t * const dst,
					const uint8_t * const src,
					const int16_t * const data,
					const uint32_t stride,
					const uint32_t rounding)
{
	uint32_t i, j;

	for (j = 0; j < 8; j++) {
		for (i = 0; i < 8; i++) {

			int16_t tot =	(int32_t)src[j * stride + i] +
							(int32_t)src[j * stride + i + 1];

			tot = (int32_t)((tot + 1 - rounding) >> 1) + (int32_t)data[j * 8 + i];

			if (tot < 0) 
			{
				tot = 0;
			}
			else if (tot > 255) 
			{
				tot = 255;
			}			
			dst[j * stride + i] = (uint8_t)tot;
		}
	}
}



void compensate_halfpel_v_c(uint8_t * const dst,
					const uint8_t * const src,
					const int16_t * const data,
					const uint32_t stride,
					const uint32_t rounding)
{
	uint32_t i, j;

	for (j = 0; j < 8; j++) {
		for (i = 0; i < 8; i++) {
			int16_t tot =	src[j * stride + i] + 
							src[j * stride + i + stride];
			tot = ((tot + 1 - rounding) >> 1) + data[j * 8 + i];
			if (tot < 0) 
			{
				tot = 0;
			}
			else if (tot > 255) 
			{
				tot = 255;
			}
			dst[j * stride + i] = (uint8_t)tot;
		}
	}
}

void compensate_halfpel_hv_c(uint8_t * const dst,
					const uint8_t * const src,
					const int16_t * const data,
					const uint32_t stride,
					const uint32_t rounding)
{
	uint32_t i, j;

	for (j = 0; j < 8; j++) {
		for (i = 0; i < 8; i++) {
			int16_t tot =	src[j * stride + i] + 
							src[j * stride + i + 1] + 
							src[j * stride + i + stride] +
							src[j * stride + i + stride + 1];
			tot = ((tot + 2 - rounding) >> 2) + data[j * 8 + i];
			
			if (tot < 0) 
			{
				tot = 0;
			}
			else if (tot > 255) 
			{
				tot = 255;
			}
	
			dst[j * stride + i] = (uint8_t)tot;
		}
	}
}



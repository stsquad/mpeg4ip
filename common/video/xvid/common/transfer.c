/**************************************************************************
 *
 *	XVID MPEG-4 VIDEO CODEC
 *	8bit<->16bit transfer
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
 *	22.12.2001	transfer_8to8add16 limit fix
 *	07.11.2001	initial version; (c)2001 peter ross <pross@cs.rmit.edu.au>
 *
 *************************************************************************/


#include "common.h"


// function pointers
transfer_8to8copyFuncPtr transfer_8to8copy;
transfer_8to8add16FuncPtr transfer_8to8add16;

transfer_16to8FuncPtr transfer_16to8copy;
transfer_16to8FuncPtr transfer_16to8add;

transfer_8to16copyFuncPtr transfer_8to16copy;



void transfer_8to16copy_c(int16_t * const dst,
					const uint8_t * const src,
					uint32_t stride)
{
	uint32_t i, j;

	for (j = 0; j < 8; j++) {
		for (i = 0; i < 8; i++) {
			dst[j * 8 + i] = (int16_t)src[j * stride + i];
		}
	}
}


void transfer_16to8copy_c(uint8_t * const dst,
					const int16_t * const src,
					uint32_t stride)
{
	uint32_t i, j;

	for (j = 0; j < 8; j++) {
		for (i = 0; i < 8; i++) {
			int16_t pixel = src[j * 8 + i];
			if (pixel < 0) {
				pixel = 0;
			} else if (pixel > 255) {
				pixel = 255;
			} 
			dst[j * stride + i] = (uint8_t)pixel;
		}
	}
}


void transfer_16to8add_c(uint8_t * const dst,
					const int16_t * const src,
					uint32_t stride)
{
	uint32_t i, j;

	for (j = 0; j < 8; j++) {
		for (i = 0; i < 8; i++) {
			int16_t pixel = (int16_t)dst[j * stride + i] + src[j * 8 + i];
			if (pixel < 0) {
				pixel = 0;
			} else if (pixel > 255) {
				pixel = 255;
			} 
			dst[j * stride + i] = (uint8_t)pixel;
		}
	}
}






void transfer_8to8copy_c(uint8_t * const dst,
					const uint8_t * const src,
					const uint32_t stride)
{
	uint32_t i, j;

	for (j = 0; j < 8; j++) {
		for (i = 0; i < 8; i++) {
			dst[j * stride + i] = src[j * stride + i];
		}
	}
}



void transfer_8to8add16_c(uint8_t * const dst,
					const uint8_t * const src,
					const int16_t * const data,
					const uint32_t stride)
{
	uint32_t i, j;

	for (j = 0; j < 8; j++) {
		for (i = 0; i < 8; i++) {
			int16_t pixel = src[j * stride + i] + data[j * 8 + i];
			if (pixel < 0) {
				pixel = 0;
			} else if (pixel > 255) {
				pixel = 255;
			} 
			dst[j * stride + i] = (uint8_t)pixel;
		}
	}
}


/**************************************************************************
 *
 *	XVID MPEG-4 VIDEO CODEC
 *	bitstream writer
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
 *  17.11.2001  moved bswap -> portab.h (Isibaar)
 *	16.11.2001	inital version; (c)2001 peter ross <pross@cs.rmit.edu.au>
 *
 *************************************************************************/


#ifndef _BITSTREAM_H_
#define _BITSTREAM_H_

#include "encoder.h"
#include <assert.h>


void BitstreamVolHeader(Bitstream * const bs,
			  const int width,
			  const int height,
			  const int quant_type);


void BitstreamVopHeader(Bitstream * const bs,
			  VOP_TYPE prediction_type,
			  const int rounding_type,
			  const uint32_t quant,
			  const uint32_t fcode);


/* initialise bitstream structure */

static void __inline BitstreamInit(Bitstream * const bs, 
								   void * const pointer)
{
    bs->buf = 0;
	bs->pos = 0;
	bs->start = bs->tail = (uint32_t *)pointer;
}


/* reset bitstream state */

static void __inline BitstreamReset(Bitstream * const bs)
{
    bs->pos = 0;
	bs->buf = 0;
	bs->tail = bs->start;
}


/* bitstream length (unit bits) */

static uint32_t __inline BsPos(const Bitstream * const bs)
{
    return 8 * ((uint32_t)bs->tail - (uint32_t)bs->start) + bs->pos;
}



/*	flush the bitsream & return length (unit bytes)
	NOTE: assumes no futher bitsream functions will be called.
 */

static uint32_t __inline BitstreamLength(Bitstream * const bs)
{
	uint32_t len = (uint32_t) bs->tail - (uint32_t) bs->start;

    if (bs->pos)
    {
		uint32_t b = bs->buf;
#ifndef ARCH_IS_BIG_ENDIAN
		BSWAP(b);
#endif
		*bs->tail = b;

		len += (bs->pos + 7) / 8;
    }

	return len;
}


/* move bitstream position forward by n bits 
*/

static void __inline BitstreamSkip(Bitstream * const bs, const uint32_t bits)
{
    bs->pos += bits;

    if (bs->pos >= 32)
    {
		uint32_t b = bs->buf;
#ifndef ARCH_IS_BIG_ENDIAN
		BSWAP(b);
#endif
		*bs->tail++ = b;
		bs->buf = 0;
		bs->pos -= 32;
    }
}


/* pad bitstream to the next byte boundary */

static void __inline BitstreamPad(Bitstream * const bs)
{
	uint32_t remainder = bs->pos % 8;

    if (remainder)
    {
		BitstreamSkip(bs, 8 - remainder);
    }
}


/* write single bit to bitstream */

static void __inline BitstreamPutBit(Bitstream * const bs, 
									const uint32_t bit)
{
    if (bit)
		bs->buf |= (0x80000000 >> bs->pos);

    BitstreamSkip(bs, 1);
}


/* write n bits to bitstream */

static void __inline BitstreamPutBits(Bitstream * const bs, 
									const uint32_t value,
									const uint32_t size)
{
	uint32_t shift = 32 - bs->pos - size;

	if (shift <= 32) {
		bs->buf |= value << shift;
		BitstreamSkip(bs, size);
	} else {
		uint32_t i = size;
		while (i--) {
			BitstreamPutBit(bs, (value >> i) & 1);
		}
	}
}


#endif /* _BITSTREAM_H_ */

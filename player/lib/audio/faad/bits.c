/*
 * FAAD - Freeware Advanced Audio Decoder
 * Copyright (C) 2001 Menno Bakker
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.

 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * $Id: bits.c,v 1.1 2001/06/28 23:54:22 wmaycisco Exp $
 */

#include <assert.h>
#include "bits.h"

/* to mask the n least significant bits of an integer */
static unsigned int msk[33] =
{
	0x00000000, 0x00000001, 0x00000003, 0x00000007,
	0x0000000f, 0x0000001f, 0x0000003f, 0x0000007f,
	0x000000ff, 0x000001ff, 0x000003ff, 0x000007ff,
	0x00000fff, 0x00001fff, 0x00003fff, 0x00007fff,
	0x0000ffff, 0x0001ffff, 0x0003ffff, 0x0007ffff,
	0x000fffff, 0x001fffff, 0x003fffff, 0x007fffff,
	0x00ffffff, 0x01ffffff, 0x03ffffff, 0x07ffffff,
	0x0fffffff, 0x1fffffff, 0x3fffffff, 0x7fffffff,
	0xffffffff
};

/* initialize buffer, call once before first getbits or showbits */
void faad_initbits(bitfile *ld, char *buffer)
{
	ld->incnt = 0;
	ld->framebits = 0;
	ld->bitcnt = 0;
	ld->rdptr = buffer;
	ld->bookmark = 0;
}

#define _SWAP(a) ((a[0] << 24) | (a[1] << 16) | (a[2] << 8) | a[3])

__inline unsigned int showbits(bitfile *ld, int n)
{
	unsigned char *v = ld->rdptr;
	int rbit = 32 - ld->bitcnt;
	unsigned int b;

	b = _SWAP(v);
	return ((b & msk[rbit]) >> (rbit-n));
}

__inline void flushbits(bitfile *ld, int n)
{
	ld->bitcnt += n;

	if (ld->bitcnt >= 8) {
		ld->rdptr += (ld->bitcnt>>3);
		ld->bitcnt &= 7;
	}

	ld->framebits += n;
}

/* return next n bits (right adjusted) */
unsigned int faad_getbits(bitfile *ld, int n)
{
	long l;

	l = showbits(ld, n);
	flushbits(ld, n);

	return l;
}

unsigned int faad_getbits_fast(bitfile *ld, int n)
{
	unsigned int l;

	l =  (unsigned char) (ld->rdptr[0] << ld->bitcnt);
	l |= ((unsigned int) ld->rdptr[1] << ld->bitcnt)>>8;
	l <<= n;
	l >>= 8;

	ld->bitcnt += n;
	ld->framebits += n;

	ld->rdptr += (ld->bitcnt>>3);
	ld->bitcnt &= 7;

	return l;
}

unsigned int faad_get1bit(bitfile *ld)
{
	unsigned char l;

	l = *ld->rdptr << ld->bitcnt;

	ld->bitcnt++;
	ld->framebits++;
	ld->rdptr += (ld->bitcnt>>3);
	ld->bitcnt &= 7;

	return l>>7;
}

void faad_bookmark(bitfile *ld, int state)
{
	if (state != 0) {
		assert(ld->bookmark == 0);
		ld->book_rdptr = ld->rdptr;
		ld->book_incnt = ld->incnt;
		ld->book_bitcnt = ld->bitcnt;
		ld->book_framebits = ld->framebits;
		ld->bookmark = 1;
	} else {
		assert(ld->bookmark == 1);
		ld->rdptr = ld->book_rdptr;
		ld->incnt = ld->book_incnt;
		ld->bitcnt = ld->book_bitcnt;
		ld->framebits = ld->book_framebits;
		ld->bookmark = 0;
	}
}

int faad_get_processed_bits(bitfile *ld)
{
	return (ld->framebits);
}

unsigned int faad_byte_align(bitfile *ld)
{
    int i=0;
	
	while(ld->bitcnt!=ld->m_alignment_offset)
	{
		faad_get1bit(ld);
		i += 1;
	}
	
    return(i);
}

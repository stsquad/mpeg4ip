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
 * $Id: bits.c,v 1.6 2005/05/09 21:29:56 wmaycisco Exp $
 */

#include <assert.h>
#include "all.h"
/* to mask the n least significant bits of an integer */
unsigned int faad_bit_msk[33] =
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
void faad_initbits(bitfile *ld, unsigned char *buffer, uint32_t buflen)
{
	ld->incnt = 0;
	ld->framebits = 0;
	ld->bitcnt = 0;
	ld->buffer = buffer;
	ld->rdptr = buffer;
	ld->maxbits = buflen * 8;
}


/* return next n bits (right adjusted) */
uint32_t faad_getbits(bitfile *ld, int n)
{
	long l;

	l = faad_showbits(ld, n);
	faad_flushbits(ld, n);

	return l;
}

uint32_t faad_getbits_fast(bitfile *ld, int n)
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

uint32_t faad_get1bit(bitfile *ld)
{
	unsigned char l;
	l = *ld->rdptr << ld->bitcnt;

	ld->bitcnt++;
	ld->framebits++;
	ld->rdptr += (ld->bitcnt>>3);
	ld->bitcnt &= 7;

	return l>>7;
}


int faad_get_processed_bits(bitfile *ld)
{
	return (ld->framebits);
}

uint32_t faad_byte_align(bitfile *ld)
{
  int i;

  if (ld->bitcnt == 0) return 0;
  i = 8 - ld->bitcnt;

  faad_flushbits(ld, i);
  return i;
}

void faad_bitdump (bitfile *ld)
{
  #if 0
  printf("processed %d %d bits left - %d\n",
	 ld->m_total / 8,
	 ld->m_total % 8,
	 ld->m_uNumOfBitsInBuffer);
  #endif
}

int faad_bits_done (bitfile *ld)
{
  if (ld->maxbits > ld->framebits) return 0;
  return 1;
}

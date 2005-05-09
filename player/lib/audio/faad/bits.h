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
 * $Id: bits.h,v 1.5 2005/05/09 21:29:56 wmaycisco Exp $
 */

#ifndef __BITS_H__
#define __BITS_H__ 1
#include "all.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef struct _bitfile2
{
	/* bit input */
  unsigned char *buffer;
  unsigned char *rdptr;
    int m_alignment_offset;
  int incnt;
  int bitcnt;
  int framebits;
  int maxbits;
} bitfile;

void faad_initbits(bitfile *ld, unsigned char *buffer, uint32_t buflen);
uint32_t faad_getbits(bitfile *ld, int n);
uint32_t faad_getbits_fast(bitfile *ld, int n);
uint32_t faad_get1bit(bitfile *ld);
uint32_t faad_byte_align(bitfile *ld);
int faad_get_processed_bits(bitfile *ld);
  int faad_bits_done(bitfile *ld);

  void faad_bitdump(bitfile *ld);

extern unsigned int faad_bit_msk[33];

#define _SWAP(a) ((a[0] << 24) | (a[1] << 16) | (a[2] << 8) | a[3])

static __inline uint32_t faad_showbits(bitfile *ld, int n)
{
        unsigned char *v;
	int rbit = 32 - ld->bitcnt;
	uint32_t b;

	v = ld->rdptr;
	b = _SWAP(v);
	return ((b & faad_bit_msk[rbit]) >> (rbit-n));
}

static __inline void faad_flushbits(bitfile *ld, int n)
{
	ld->bitcnt += n;

	if (ld->bitcnt >= 8) {
		ld->rdptr += (ld->bitcnt>>3);
		ld->bitcnt &= 7;
	}

	ld->framebits += n;
}

#ifdef __cplusplus
}
#endif
#endif

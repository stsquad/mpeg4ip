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
 * $Id: bits.h,v 1.2 2001/12/11 18:12:04 wmaycisco Exp $
 */

#ifndef __BITS_H__
#define __BITS_H__ 1
#include "all.h"
#ifdef __cplusplus
extern "C" {
#endif

  typedef void (*get_more_bytes_t)(void *, unsigned char **, uint32_t *, uint32_t);
typedef struct _bitfile2
{
  get_more_bytes_t get_more_bytes;
  void *ud;
	/* bit input */
  unsigned char *buffer;
  unsigned char *rdptr;
    int m_alignment_offset;
  int incnt;
  int bitcnt;
  int framebits;
  int framebits_max;
  uint32_t orig_buflen;
  uint32_t buflen;

} bitfile;

void faad_initbits(bitfile *ld, char *buffer, uint32_t buflen);
uint32_t faad_getbits(bitfile *ld, int n);
uint32_t faad_getbits_fast(bitfile *ld, int n);
uint32_t faad_get1bit(bitfile *ld);
uint32_t faad_byte_align(bitfile *ld);
int faad_get_processed_bits(bitfile *ld);

  void faad_bitdump(bitfile *ld);

extern unsigned int faad_bit_msk[33];

#define _SWAP(a) ((a[0] << 24) | (a[1] << 16) | (a[2] << 8) | a[3])

static __inline void check_buffer (bitfile *ld, int n)
{
  int cmp;

  cmp = ld->framebits + n;
  if (cmp > ld->framebits_max) {
    
    (ld->get_more_bytes)(ld->ud, &ld->buffer, &ld->orig_buflen,
			 (ld->framebits / 8));
    ld->framebits = ld->bitcnt;
    ld->rdptr = ld->buffer;
    ld->buflen = ld->orig_buflen;
    ld->framebits_max = ld->orig_buflen * 8;
  }
}

static __inline uint32_t faad_showbits(bitfile *ld, int n)
{
        unsigned char *v;
	int rbit = 32 - ld->bitcnt;
	uint32_t b;

	check_buffer(ld, n);
	
	v = ld->rdptr;
	b = _SWAP(v);
	return ((b & faad_bit_msk[rbit]) >> (rbit-n));
}

static __inline void faad_flushbits(bitfile *ld, int n)
{
	ld->bitcnt += n;

	if (ld->bitcnt >= 8) {
		ld->rdptr += (ld->bitcnt>>3);
		ld->buflen -= (ld->bitcnt>>3);
		ld->bitcnt &= 7;
	}

	ld->framebits += n;
}

#ifdef __cplusplus
}
#endif
#endif

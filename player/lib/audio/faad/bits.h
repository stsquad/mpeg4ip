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
 * $Id: bits.h,v 1.1 2001/06/28 23:54:22 wmaycisco Exp $
 */

#ifndef __BITS_H__
#define __BITS_H__ 1

#ifdef __cplusplus
extern "C" {
#endif

#if 0
typedef struct _bitfile
{
	/* bit input */
	unsigned char *rdptr;
    int m_alignment_offset;
	int incnt;
	int bitcnt;
	int framebits;

	/* bookmarks */
	unsigned char *book_rdptr;
	int book_incnt;
	int book_bitcnt;
	int book_framebits;
	int bookmark;
} bitfile;

void faad_initbits(bitfile *ld, char *buffer);
__inline unsigned int showbits(bitfile *ld, int n);
__inline void flushbits(bitfile *ld, int n);
unsigned int faad_getbits(bitfile *ld, int n);
unsigned int faad_getbits_fast(bitfile *ld, int n);
unsigned int faad_get1bit(bitfile *ld);
unsigned int faad_byte_align(bitfile *ld);
int faad_get_processed_bits(bitfile *ld);
void faad_bookmark(bitfile *ld, int state);

#else
#include "mpeg4ip_bits.h"
#endif

#ifdef __cplusplus
}
#endif
#endif

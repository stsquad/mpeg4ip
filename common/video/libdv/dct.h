/* 
 *  dct.h
 *
 *     Copyright (C) Charles 'Buck' Krasic - April 2000
 *     Copyright (C) Erik Walthinsen - April 2000
 *
 *  This file is part of libdv, a free DV (IEC 61834/SMPTE 314M)
 *  codec.
 *
 *  libdv is free software; you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your
 *  option) any later version.
 *   
 *  libdv is distributed in the hope that it will be useful, but
 *  WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  General Public License for more details.
 *   
 *  You should have received a copy of the GNU General Public License
 *  along with GNU Make; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA. 
 *
 *  The libdv homepage is http://libdv.sourceforge.net/.  
 */
 
#ifndef DV_DCT_H
#define DV_DCT_H

#include <libdv/dv_types.h>

#define DCT_YUV_PRECISION 1        /* means fixpoint with YUV_PRECISION bits 
				      after the point (if you change this,
				      change rgbtoyuv.S and dct_block_mmx.S
				      accordingly) */

#ifdef __cplusplus
extern "C" {
#endif

void dct_init(void);
/* Input is transposed ! */
void dct_88(dv_coeff_t *block);
/* Input is transposed ! */
void dct_248(dv_coeff_t *block);
void idct_88(dv_coeff_t *block);
#if BRUTE_FORCE_248
void idct_248(double *block);
#endif

#ifdef __cplusplus
}
#endif

#endif // DV_DCT_H

/* 
 *  YV12.c
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

#include <dv_types.h>

#include <stdlib.h>

#include "YV12.h"

#if ARCH_X86
#include "mmx.h"
#endif // ARCH_X86

/* Lookup tables for mapping signed to unsigned, and clamping */
static unsigned char	real_uvlut[256], *uvlut;
static unsigned char	real_ylut[768],  *ylut;

#if ARCH_X86
/* Define some constants used in MMX range mapping and clamping logic */
static mmx_t		mmx_0x10s   = (mmx_t) 0x1010101010101010LL,
			mmx_0x0080s = (mmx_t) 0x0080008000800080LL,
			mmx_0x7f24s = (mmx_t) 0x7f247f247f247f24LL,
                        mmx_0x7f94s = (mmx_t) 0x7f947f947f947f94LL;
#endif // ARCH_X86

void 
dv_YV12_init(void) {
  gint i;
  gint value;

  uvlut = real_uvlut + 128; // index from -128 .. 127
  for(i=-128;
      i<128;
      ++i) {
    if(i < (16-128)) value = 16;
    else if(i > (240-128)) value = 240;
    else value = i+128;
    uvlut[i] = value;
  } /* for */
  
  ylut = real_ylut + 256; // index from -256 .. 511
  for(i=-256;
      i<512;
      ++i) {
    if(i < (16-128)) value = 16;
    else if(i > (235-128)) value = 240;
    else value = i+128;
    ylut[i] = value;
  } /* for */
} /* dv_YV12_init */

void 
dv_mb420_YV12(dv_macroblock_t *mb, guchar **pixels, guint16 *pitches) {
  dv_coeff_t		*Y[4], *UV[2], *Ytmp, *UVtmp;
  unsigned char	        *py, *pwy, *puv, *pwuv;
  int			i, j, row, col;

  Y [0] = mb->b[0].coeffs;
  Y [1] = mb->b[1].coeffs;
  Y [2] = mb->b[2].coeffs;
  Y [3] = mb->b[3].coeffs;
  UV[0] = mb->b[4].coeffs;
  UV[1] = mb->b[5].coeffs;

  
  py  = pixels[0] + mb->x + (mb->y * pitches[0]);

  for(i=0; i<4; i+=2) {  // two rows of Y blocks

    for(row=0; 
	row < 8; 
	row++) {

      pwy = py;

      for(j=0; j<2; j++) { // two columns of Y blocks
	Ytmp = Y[i+j];
	for(col = 0; col < 8; col++) {
	  *pwy++ = ylut[*Ytmp++];
	}
	Y[i+j] = Ytmp;
      } /* for j */

      py += pitches[0];

    } /* for row */
  } /* for i */

  for(i=1; 
      i<3;
      i++) { // two Chroma blocks 

    puv = pixels[i] + (mb->x/2) + ((mb->y/2) * pitches[i]);
    UVtmp = UV[i-1];

    for(row=0; 
	row < 8; 
	row++, puv += pitches[i]) {

      pwuv = puv;
      for(col=0; 
	  col<8; 
	  col++) {
	*pwuv++ = uvlut[*UVtmp++];
      } /* for col */

    } /* for row */

  } // for i 

} /* dv_mb420_YV12 */

#if ARCH_X86

void 
dv_mb420_YV12_mmx(dv_macroblock_t *mb, guchar **pixels, guint16 *pitches) {
  dv_coeff_t		*Y[4], *UV[2], *Ytmp, *UVtmp;
  unsigned char	        *py, *pwy, *puv;
  int			i, j, row;

  Y [0] = mb->b[0].coeffs;
  Y [1] = mb->b[1].coeffs;
  Y [2] = mb->b[2].coeffs;
  Y [3] = mb->b[3].coeffs;
  UV[0] = mb->b[4].coeffs;
  UV[1] = mb->b[5].coeffs;
  
  py  = pixels[0] + mb->x + (mb->y * pitches[0]);

  movq_m2r(mmx_0x0080s,mm0);
  movq_m2r(mmx_0x10s,mm1);

  movq_m2r(mmx_0x7f94s,mm2);
  movq_m2r(mmx_0x7f24s,mm3);

  for(i=0; i<4; i+=2) {  // two rows of Y blocks

    for(row=0; 
	row < 8; 
	row++) {

      pwy = py;

      for(j=0; j<2; j++) { // two columns of Y blocks
	Ytmp = Y[i+j];

	movq_m2r(*(Ytmp  ),mm4);
	movq_m2r(*(Ytmp+4),mm5);

	paddw_r2r(mm0,mm4); // +128
	paddw_r2r(mm0,mm5); // +128

	packuswb_r2r(mm5,mm4); // 16 -> 8 bit
	movq_r2m(mm4, *pwy);

	Y[i+j] = Ytmp + 8;
	pwy += 8;
      } /* for j */

      py += pitches[0];

    } /* for row */
  } /* for i */

  for(i=1; 
      i<3;
      i++) { // two Chroma blocks 

    puv = pixels[i] + (mb->x/2) + ((mb->y/2) * pitches[i]);
    UVtmp = UV[i-1];

    for(row=0; 
	row < 8; 
	row++) {

	movq_m2r(*(UVtmp  ),mm4);
	movq_m2r(*(UVtmp+4),mm5);

	paddw_r2r(mm0,mm4); // +128
	paddw_r2r(mm0,mm5); // +128

	packuswb_r2r(mm5,mm4); // 16 -> 8 bit
	movq_r2m(mm4, *puv);

	UVtmp += 8;
	puv += pitches[i];
    } /* for row */

  } // for i 
  emms();

} /* dv_mb420_YV12_mmx */


#endif // ARCH_X86


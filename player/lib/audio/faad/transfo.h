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
 * $Id: transfo.h,v 1.4 2002/01/11 00:55:17 wmaycisco Exp $
 */

#ifndef TRANSFORM_H
#define TRANSFORM_H

/* Use this for decoder - single precision */
typedef float fftw_real;

/* Use this for encoder - double precision */
/* typedef double fftw_real; */

typedef struct {
     fftw_real re, im;
} fftw_complex;

#include "all.h"

#define c_re(c)  ((c).re)
#define c_im(c)  ((c).im)

#define DEFINE_PFFTW(size)          \
 void pfftwi_##size(fftw_complex *input);   \
 void pfftw_##size(fftw_complex *input);    \
 int  pfftw_permutation_##size(int i);

DEFINE_PFFTW(16)
DEFINE_PFFTW(32)
DEFINE_PFFTW(64)
DEFINE_PFFTW(128)
DEFINE_PFFTW(512)

void MakeFFTOrder(faacDecHandle hDecoder);
void IMDCT_Long(faacDecHandle hDecoder, fftw_real *data);
void IMDCT_Short(faacDecHandle hDecoder, fftw_real *data);

void MDCT_Long(faacDecHandle hDecoder, fftw_real *data);
void MDCT_Short(faacDecHandle hDecoder, fftw_real *data);

#endif    /*    TRANSFORM_H     */

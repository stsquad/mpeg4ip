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
 * $Id: fastfft.h,v 1.3 2001/06/28 23:54:22 wmaycisco Exp $
 */

#include "transfo.h"

#define PFFTW(name)  CONCAT(pfftw_, name)
#define PFFTWI(name)  CONCAT(pfftwi_, name)
#define CONCAT_AUX(a, b) a ## b
#define CONCAT(a, b) CONCAT_AUX(a,b)
#define FFTW_KONST(x) ((fftw_real) x)

void PFFTW(twiddle_4)(fftw_complex *A, const fftw_complex *W, int iostride);
void PFFTWI(twiddle_4)(fftw_complex *A, const fftw_complex *W, int iostride);


/* 
 *  weighting.h
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

#ifndef DV_WEIGHTING_H
#define DV_WEIGHTING_H

#include <libdv/dv_types.h>

#ifdef __cplusplus
extern "C" {
#endif

void weight_init(void);
void weight_88(dv_coeff_t *block);
void weight_248(dv_coeff_t *block);
void weight_88_inverse(dv_coeff_t *block);
void weight_248_inverse(dv_coeff_t *block);

#ifdef __cplusplus
}
#endif

#endif // DV_WEIGHTING_H

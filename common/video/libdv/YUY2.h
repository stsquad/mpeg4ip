/* 
 *  YUY2.h
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

#ifndef DV_YUY2_H
#define DV_YUY2_H

#include <libdv/dv_types.h>

/* Convert output of decoder to YUY2 conforming layout.  YUY2 is a
 * format supported directly by many display adaptors.  See
 * the following website for details of YUY2:
 *
 *    http://www.webartz.com/fourcc/fccyuv.htm#YUY2
 *
 *
 * The convertion entails going from 16bit to 8bit, properly clamping
 * YUV values, and upsampling to 422 by duplicating chroma values.
 * 
 * These conversions make sense to use if the HW supports YUY2 and the
 * DV is NTSC or SMPTE PAL.  Older IEC 61834 PAL DV is completely 420
 * sampled, so it is better to convert that to YV12 format.  */

#ifdef __cplusplus
extern "C" {
#endif

extern void dv_YUY2_init(void);

/* scalar versions */
extern void dv_mb411_YUY2(dv_macroblock_t *mb, guchar **pixels, gint *pitches);
extern void dv_mb411_right_YUY2(dv_macroblock_t *mb, guchar **pixels, gint *pitches);
extern void dv_mb420_YUY2(dv_macroblock_t *mb, guchar **pixels, gint *pitches);

#if ARCH_X86
/* pentium architecture mmx versions */
extern void dv_mb411_YUY2_mmx(dv_macroblock_t *mb, guchar **pixels, gint *pitches);
extern void dv_mb411_right_YUY2_mmx(dv_macroblock_t *mb, guchar **pixels, gint *pitches);
extern void dv_mb420_YUY2_mmx(dv_macroblock_t *mb, guchar **pixels, gint *pitches);
#endif // ARCH_X86

#ifdef __cplusplus
}
#endif

#endif // DV_YUY2_H

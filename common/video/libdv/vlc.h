/* 
 *  vlc.h
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

#ifndef DV_VLC_H
#define DV_VLC_H

#include <libdv/dv_types.h>

#include <libdv/bitstream.h>

#define VLC_NOBITS (-1)
#define VLC_ERROR (-2)

typedef struct dv_vlc_s {
  gint8 run;
  gint8 len;
  gint16 amp;
} dv_vlc_t;

#if 1
typedef dv_vlc_t dv_vlc_tab_t;
#else
typedef struct dv_vlc_tab_s {
  gint8 run;
  gint8 len;
  gint16 amp;
} dv_vlc_tab_t;
#endif

#ifdef __cplusplus
extern "C" {
#endif

extern const gint8 *dv_vlc_classes[17];
extern const gint dv_vlc_class_index_mask[17];
extern const gint dv_vlc_class_index_rshift[17];
extern const dv_vlc_tab_t dv_vlc_broken[1];
extern const dv_vlc_tab_t *dv_vlc_lookups[6];
extern const gint dv_vlc_index_mask[6];
extern const gint dv_vlc_index_rshift[6];
extern const gint sign_lookup[2];
extern const gint sign_mask[17];
extern const gint sign_rshift[17];
extern void dv_construct_vlc_table();

// Note we assume bits is right (lsb) aligned, 0 < maxbits < 17
// This may look crazy, but there are no branches here.
extern void dv_decode_vlc(gint bits,gint maxbits, dv_vlc_t *result);
extern void __dv_decode_vlc(gint bits, dv_vlc_t *result);

extern __inline__ void dv_peek_vlc(bitstream_t *bs,gint maxbits, dv_vlc_t *result) {
  if(maxbits < 16)
    dv_decode_vlc(bitstream_show(bs,16),maxbits,result);
  else
    __dv_decode_vlc(bitstream_show(bs,16),result);
} // dv_peek_vlc

#ifdef __cplusplus
}
#endif

#endif // DV_VLC_H

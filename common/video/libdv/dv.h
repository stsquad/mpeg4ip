/* 
 *  dv.h
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

#ifndef DV_H
#define DV_H


#include <libdv/dv_types.h>

#include <stdlib.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Main API */
extern dv_decoder_t *dv_decoder_new      (void);
extern void          dv_init             (void); 
extern gint          dv_parse_header     (dv_decoder_t *dv, guchar *buffer);
extern void          dv_decode_full_frame(dv_decoder_t *dv, 
					  guchar *buffer, dv_color_space_t color_space, 
					  guchar **pixels, gint *pitches);
extern gint          dv_decode_full_audio(dv_decoder_t *dv, 
					  guchar *buffer, gint16 **outbufs);

/* Low level API */
extern gint dv_parse_video_segment(dv_videosegment_t *seg, guint quality);
extern void dv_decode_video_segment(dv_decoder_t *dv, dv_videosegment_t *seg, guint quality);

extern void dv_render_video_segment_rgb(dv_decoder_t *dv, dv_videosegment_t *seg, 
					guchar **pixels, gint *pitches);

extern void dv_render_video_segment_yuv(dv_decoder_t *dv, dv_videosegment_t *seg, 
					guchar **pixels, gint *pitches);

/* ---------------------------------------------------------------------------
 * functions based on vaux data
 * return value: <0 unknown, 0 no, >0 yes
 */
extern int dv_frame_changed (dv_decoder_t *dv),
	   dv_frame_is_color (dv_decoder_t *dv),
	   dv_system_50_fields (dv_decoder_t *dv),
	   dv_format_normal (dv_decoder_t *dv),
	   dv_format_wide (dv_decoder_t *dv),
	   dv_get_vaux_pack (dv_decoder_t *dv, guint8 pack_id, guint8 *pack_data);

#ifdef __cplusplus
}
#endif

#endif // DV_H 

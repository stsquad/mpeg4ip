/* 
 *  audio.h
 *
 *     Copyright (C) Charles 'Buck' Krasic - January 2001
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

#ifndef DV_AUDIO_H
#define DV_AUDIO_H

#include <dv_types.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Low-level routines */
extern dv_audio_t *dv_audio_new(void);
extern gboolean    dv_parse_audio_header(dv_decoder_t *decoder, guchar *inbuf);
extern gboolean    dv_update_num_samples(dv_audio_t *dv_audio, guint8 *inbuf);
extern gint        dv_decode_audio_block(dv_audio_t *dv_audio, guint8 *buffer, gint ds, gint audio_dif, gint16 **outbufs);
extern void        dv_audio_deemphasis(dv_audio_t *dv_audio, gint16 *outbuf);
extern void        dv_dump_aaux_as(void *buffer, int ds, int audio_dif);

#ifdef __cplusplus
}
#endif

#endif // DV_AUDIO_H

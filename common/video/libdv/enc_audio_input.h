/* 
 *  enc_audio_input.h
 *
 *     Copyright (C) Peter Schlaile - Feb 2001
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
 
#ifndef DV_ENC_AUDIO_INPUT_H
#define DV_ENC_AUDIO_INPUT_H

#include "dv_types.h"

#ifdef __cplusplus
extern "C" {
#endif
	#define DV_ENC_MAX_AUDIO_INPUT_FILTERS     32

	typedef struct dv_enc_audio_info_s {
		/* stored by init (but could be used for on the fly sample
		   rate changes) */
		int channels;
		int frequency;
		int bitspersample;
		int bytespersecond;
		int bytealignment;
		/* stored by load */
		int bytesperframe;
		/* big endian 12/16 bit is assumed */
		unsigned char data[1920 * 2 * 2]; /* max 48000.0 Hz PAL */
	} dv_enc_audio_info_t;

	typedef struct dv_audio_enc_input_filter_s {
		int (*init)(const char* filename, 
			    dv_enc_audio_info_t * audio_info);
		void (*finish)();
		int (*load)(dv_enc_audio_info_t * audio_info, int isPAL);

		const char* filter_name;
	} dv_enc_audio_input_filter_t;

	extern void dv_enc_register_audio_input_filter(
		dv_enc_audio_input_filter_t filter);
	extern int get_dv_enc_audio_input_filters(
		dv_enc_audio_input_filter_t ** filters, int * count);

#ifdef __cplusplus
}
#endif

#endif // DV_ENC_AUDIO_INPUT_H

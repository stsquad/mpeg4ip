/* 
 *  enc_input.h
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
 
#ifndef DV_ENC_INPUT_H
#define DV_ENC_INPUT_H

#include "dv_types.h"

#ifdef __cplusplus
extern "C" {
#endif
	#define DV_ENC_MAX_INPUT_FILTERS     32

	typedef struct dv_enc_input_filter_s {
		int (*init)(int wrong_interlace, int force_dct);
		void (*finish)();
		int (*load)(const char* filename, int * isPAL);
		int (*skip)(const char* filename, int * isPAL);
		/* fills macroblock, determines dct_mode and
		   transposes dv_blocks */
		void (*fill_macroblock)(dv_macroblock_t *mb, int isPAL);

		const char* filter_name;
	} dv_enc_input_filter_t;

	extern void dv_enc_register_input_filter(dv_enc_input_filter_t filter);
	extern int get_dv_enc_input_filters(dv_enc_input_filter_t ** filters,
					    int * count);

#ifdef __cplusplus
}
#endif

#endif // DV_ENC_INPUT_H

/*
 * The contents of this file are subject to the Mozilla Public
 * License Version 1.1 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.mozilla.org/MPL/
 * 
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 * 
 * The Original Code is MPEG4IP.
 * 
 * The Initial Developer of the Original Code is Cisco Systems Inc.
 * Portions created by Cisco Systems Inc. are
 * Copyright (C) Cisco Systems Inc. 2000, 2001.  All Rights Reserved.
 * 
 * Contributor(s): 
 *              Bill May        wmay@cisco.com
 */
/*
 * aa.h - class definition for AAC codec.
 */

#ifndef __OURWAV_H__
#define __OURWAV_H__ 1
#include "mpeg4ip_sdl_includes.h"
#include "codec_plugin.h"

#define m_vft c.v.audio_vft
#define m_ifptr c.ifptr

typedef struct wav_codec_t {
  codec_data_t c;
  SDL_AudioSpec *m_sdl_config;
  uint32_t m_bytes_per_channel;
  int m_configured;
  Uint8 *m_wav_buffer;
  Uint32 m_wav_len;
  Uint32 m_wav_buffer_on;
} wav_codec_t;

codec_data_t *wav_file_check(lib_message_func_t message,
			     const char *name,
			     double *max,
			     char *desc[4],
			     CConfigSet *pConfig);

int wav_file_next_frame(codec_data_t *ifptr,
			uint8_t **buffer,
			frame_timestamp_t  *ts);
int wav_file_eof(codec_data_t *ifptr);

void wav_file_used_for_frame(codec_data_t *ifptr,
			     uint32_t bytes);

int wav_raw_file_seek_to(codec_data_t *ifptr,
			 uint64_t ts);

#endif

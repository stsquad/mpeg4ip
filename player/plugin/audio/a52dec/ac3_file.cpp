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
 * Copyright (C) Cisco Systems Inc. 2004.  All Rights Reserved.
 * 
 * Contributor(s): 
 *              Bill May        wmay@cisco.com
 */
/*
 * ac3_file.cpp - create media structure for ac3 files
 */

#include "a52dec.h"
codec_data_t *ac3_file_check (lib_message_func_t message,
			      const char *name, 
			      double *max, 
			      char *desc[4],
			      CConfigSet *pConfig)
{
  a52dec_codec_t *ac3;
  int len = strlen(name);
  if (strcasecmp(name + len - 4, ".ac3") != 0) {
    return (NULL);
  }

  ac3 = MALLOC_STRUCTURE(a52dec_codec_t);
  memset(ac3, 0, sizeof(*ac3));
  *max = 0;

  ac3->m_buffer = (uint8_t *)malloc(MAX_READ_BUFFER);
  ac3->m_buffer_size_max = MAX_READ_BUFFER;
  ac3->m_ifile = fopen(name, FOPEN_READ_BINARY);
  if (ac3->m_ifile == NULL) {
    free(ac3);
    return NULL;
  }

  ac3->m_initialized = 0;
  ac3->m_state = a52_init(0);
  ac3->m_buffer_size = fread(ac3->m_buffer, 
			     1, 
			     ac3->m_buffer_size_max, 
			     ac3->m_ifile);
  int sample_rate, bit_rate, flags;
  a52_syncinfo(ac3->m_buffer, &flags, &sample_rate, &bit_rate);
  ac3->m_freq = sample_rate;
  return ((codec_data_t *)ac3);
}


int ac3_file_next_frame (codec_data_t *your,
			 uint8_t **buffer, 
			 frame_timestamp_t *pts)
{
  a52dec_codec_t *ac3 = (a52dec_codec_t *)your;

  if (ac3->m_buffer_on > 0) {
    memmove(ac3->m_buffer, 
	    &ac3->m_buffer[ac3->m_buffer_on],
	    ac3->m_buffer_size - ac3->m_buffer_on);
  }
  ac3->m_buffer_size -= ac3->m_buffer_on;
  ac3->m_buffer_size += fread(ac3->m_buffer + ac3->m_buffer_size, 
			      1, 
			      ac3->m_buffer_size_max - ac3->m_buffer_size,
			      ac3->m_ifile);
  ac3->m_buffer_on = 0;
  if (ac3->m_buffer_size == 0) return 0;


  uint64_t calc;
  pts->audio_freq = ac3->m_freq;
  pts->audio_freq_timestamp = ac3->m_framecount * 256 * 6;
  pts->timestamp_is_pts = false;
  calc = ac3->m_framecount * 256 * 6 * TO_U64(1000);
  if (ac3->m_freq == 0) {
    calc = 0;
  } else {
    calc /= ac3->m_freq;
  }
  pts->msec_timestamp = calc;
  *buffer = ac3->m_buffer;
  ac3->m_framecount++;
  return (ac3->m_buffer_size);
}
	
void ac3_file_used_for_frame (codec_data_t *ifptr, 
			     uint32_t bytes)
{
  a52dec_codec_t *ac3 = (a52dec_codec_t *)ifptr;
  ac3->m_buffer_on += bytes;
  if (ac3->m_buffer_on > ac3->m_buffer_size) ac3->m_buffer_on = ac3->m_buffer_size;
}

int ac3_file_eof (codec_data_t *ifptr)
{
  a52dec_codec_t *ac3 = (a52dec_codec_t *)ifptr;
  return ac3->m_buffer_on == ac3->m_buffer_size && feof(ac3->m_ifile);
}

int ac3_raw_file_seek_to (codec_data_t *ifptr, uint64_t ts)
{
  if (ts != 0) return -1;

  a52dec_codec_t *ac3 = (a52dec_codec_t *)ifptr;
  rewind(ac3->m_ifile);
  ac3->m_buffer_size = ac3->m_buffer_on = 0;
  ac3->m_framecount = 0;
  return 0;
}

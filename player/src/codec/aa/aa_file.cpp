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
 * aa_file.cpp - create media structure for aac files
 */

#include "aac.h"
codec_data_t *aac_file_check (lib_message_func_t message,
			      const char *name, 
			      double *max, 
			      char *desc[4],
			      CConfigSet *pConfig)
{
  aac_codec_t *aac;
  int len = strlen(name);
  if (strcasecmp(name + len - 4, ".aac") != 0) {
    return (NULL);
  }

  aac = MALLOC_STRUCTURE(aac_codec_t);
  memset(aac, 0, sizeof(*aac));
  *max = 0;

  aac->m_buffer = (uint8_t *)malloc(MAX_READ_BUFFER);
  aac->m_buffer_size_max = MAX_READ_BUFFER;
  aac->m_ifile = fopen(name, FOPEN_READ_BINARY);
  if (aac->m_ifile == NULL) {
    free(aac);
    return NULL;
  }
  aac->m_output_frame_size = 1024;
  aac->m_info = faacDecOpen(); // use defaults here...
  aac->m_buffer_size = fread(aac->m_buffer, 
			     1, 
			     aac->m_buffer_size_max, 
			     aac->m_ifile);

  unsigned long freq, chans;

  faacDecInit(aac->m_info, (unsigned char *)aac->m_buffer, &freq, &chans);
  // may want to actually decode the first frame...
  if (freq == 0) {
    message(LOG_ERR, aaclib, "Couldn't determine AAC frame rate");
    aac_close((codec_data_t *)aac);
    return (NULL);
  } 
  aac->m_freq = freq;
  aac->m_chans = chans;
  aac->m_faad_inited = 1;
  aac->m_framecount = 0;
  return ((codec_data_t *)aac);
}


int aac_file_next_frame (codec_data_t *your,
			 uint8_t **buffer, 
			 frame_timestamp_t *ts)
{
  aac_codec_t *aac = (aac_codec_t *)your;

  if (aac->m_buffer_on > 0) {
    memmove(aac->m_buffer, 
	    &aac->m_buffer[aac->m_buffer_on],
	    aac->m_buffer_size - aac->m_buffer_on);
  }
  aac->m_buffer_size -= aac->m_buffer_on;
  aac->m_buffer_size += fread(aac->m_buffer + aac->m_buffer_size, 
			      1, 
			      aac->m_buffer_size_max - aac->m_buffer_size,
			      aac->m_ifile);
  aac->m_buffer_on = 0;
  if (aac->m_buffer_size == 0) return 0;


  uint64_t calc;
  calc = aac->m_framecount * TO_U64(1024 * 1000);
  calc /= aac->m_freq;
  ts->msec_timestamp = calc;
  ts->audio_freq_timestamp = aac->m_framecount * 1024;
  ts->audio_freq = aac->m_freq;
  ts->timestamp_is_pts = false;
  *buffer = aac->m_buffer;
  aac->m_framecount++;
  return (aac->m_buffer_size);
}
	
void aac_file_used_for_frame (codec_data_t *ifptr, 
			     uint32_t bytes)
{
  aac_codec_t *aac = (aac_codec_t *)ifptr;
  aac->m_buffer_on += bytes;
  if (aac->m_buffer_on > aac->m_buffer_size) aac->m_buffer_on = aac->m_buffer_size;
}

int aac_file_eof (codec_data_t *ifptr)
{
  aac_codec_t *aac = (aac_codec_t *)ifptr;
  return aac->m_buffer_on == aac->m_buffer_size && feof(aac->m_ifile);
}

int aac_raw_file_seek_to (codec_data_t *ifptr, uint64_t ts)
{
  if (ts != 0) return -1;

  aac_codec_t *aac = (aac_codec_t *)ifptr;
  rewind(aac->m_ifile);
  aac->m_buffer_size = aac->m_buffer_on = 0;
  aac->m_framecount = 0;
  return 0;
}

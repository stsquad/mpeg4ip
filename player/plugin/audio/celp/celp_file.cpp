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
 *              Massimo Villari     mvillari@cisco.com
 */
/*
 * celp_file.cpp - create media structure for celp files
 */

#include "celp.h"
codec_data_t *celp_file_check (lib_message_func_t message,
			      const char *name, 
			      double *max, 
			      char *desc[4])
{
  celp_codec_t *celp;
  int len = strlen(name);
  if (strcasecmp(name + len - 5, ".celp") != 0) {
    return (NULL);
  }

  celp = MALLOC_STRUCTURE(celp_codec_t);
  memset(celp, 0, sizeof(*celp));
  *max = 0;

  celp->m_buffer = (uint8_t *)malloc(MAX_READ_BUFFER);
  celp->m_buffer_size_max = MAX_READ_BUFFER;
  celp->m_ifile = fopen(name, FOPEN_READ_BINARY);
  if (celp->m_ifile == NULL) {
    free(celp);
    return NULL;
  }
  //celp->m_output_frame_size = 1024;
  
  celp->m_buffer_size = fread(celp->m_buffer, 
			     1, 
			     celp->m_buffer_size_max, 
			     celp->m_ifile);

  unsigned long freq, chans;


  // may want to actually decode the first frame...
  if (freq == 0) {
    message(LOG_ERR, celplib, "Couldn't determine CELP frame rate");
    celp_close((codec_data_t *)celp);
    return (NULL);
  } 
  celp->m_freq = freq;
  celp->m_chans = chans;
  celp->m_celp_inited = 1;
  celp->m_framecount = 0;
  return ((codec_data_t *)celp);
}


int celp_file_next_frame (codec_data_t *your,
			 uint8_t **buffer, 
			 uint64_t *ts)
{
  celp_codec_t *celp = (celp_codec_t *)your;

  if (celp->m_buffer_on > 0) {
    memmove(celp->m_buffer, 
	    &celp->m_buffer[celp->m_buffer_on],
	    celp->m_buffer_size - celp->m_buffer_on);
  }
  celp->m_buffer_size -= celp->m_buffer_on;
  celp->m_buffer_size += fread(celp->m_buffer + celp->m_buffer_size, 
			      1, 
			      celp->m_buffer_size_max - celp->m_buffer_size,
			      celp->m_ifile);
  celp->m_buffer_on = 0;
  if (celp->m_buffer_size == 0) return 0;


  uint64_t calc;
  calc = celp->m_framecount * 1024 * M_64;
  calc /= celp->m_freq;
  *ts = calc;
  *buffer = celp->m_buffer;
  celp->m_framecount++;
  return (celp->m_buffer_size);
}
	
void celp_file_used_for_frame (codec_data_t *ifptr, 
			     uint32_t bytes)
{
  celp_codec_t *celp = (celp_codec_t *)ifptr;
  celp->m_buffer_on += bytes;
  if (celp->m_buffer_on > celp->m_buffer_size) celp->m_buffer_on = celp->m_buffer_size;
}

int celp_file_eof (codec_data_t *ifptr)
{
  celp_codec_t *celp = (celp_codec_t *)ifptr;
  return celp->m_buffer_on == celp->m_buffer_size && feof(celp->m_ifile);
}

int celp_raw_file_seek_to (codec_data_t *ifptr, uint64_t ts)
{
  if (ts != 0) return -1;

  celp_codec_t *celp = (celp_codec_t *)ifptr;
  rewind(celp->m_ifile);
  celp->m_buffer_size = celp->m_buffer_on = 0;
  celp->m_framecount = 0;
  return 0;
}

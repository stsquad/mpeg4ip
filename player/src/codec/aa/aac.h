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

#ifndef __AA_H__
#define __AA_H__ 1
#include "faad/all.h"
#include <faad/bits.h>
#include "codec_plugin.h"

typedef struct aac_codec_t {
  codec_data_t c;
  audio_vft_t *m_vft;
  void *m_ifptr;
  faacDecHandle m_info;
  int m_object_type;
  int m_resync_with_header;
  int m_record_sync_time;
  uint64_t m_current_time;
  uint64_t m_last_rtp_ts;
  uint64_t m_msec_per_frame;
  uint32_t m_current_frame;
  int m_audio_inited;
  int m_faad_inited;
  uint32_t m_freq;  // frequency
  int m_chans; // channels
  int m_output_frame_size;
  uint8_t *m_temp_buff;
#if DUMP_OUTPUT_TO_FILE
  FILE *m_outfile;
#endif
  FILE *m_ifile;
  uint8_t *m_buffer;
  uint32_t m_buffer_size_max;
  uint32_t m_buffer_size;
  uint32_t m_buffer_on;
  uint64_t m_framecount;
} aac_codec_t;

#define m_vft c.v.audio_vft
#define m_ifptr c.ifptr
#define MAX_READ_BUFFER (768 * 8)

#define aa_message aac->m_vft->log_msg
void aac_close(codec_data_t *ptr);
extern const char *aaclib;

codec_data_t *aac_file_check(lib_message_func_t message,
			     const char *name,
			     double *max,
			     char *desc[4],
			     CConfigSet *pConfig);

int aac_file_next_frame(codec_data_t *ifptr,
			uint8_t **buffer,
			frame_timestamp_t *ts);
int aac_file_eof(codec_data_t *ifptr);

void aac_file_used_for_frame(codec_data_t *ifptr,
			     uint32_t bytes);

int aac_raw_file_seek_to(codec_data_t *ifptr,
			 uint64_t ts);
#endif

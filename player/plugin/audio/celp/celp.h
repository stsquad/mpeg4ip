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
 * Copyright (C) Cisco Systems Inc. 2000-2004  All Rights Reserved.
 * 
 * Contributor(s): 
 *              Bill May        wmay@cisco.com
 */
/*
 * celp.h - class definition for CELP codec.
 */

#ifndef __CELP_H__
#define __CELP_H__ 1


#include "include/audio.h"
#include "include/austream.h"

#include "bitstream.h"
#include "codec_plugin.h"

typedef struct celp_codec_t {
  codec_data_t c;
  audio_vft_t *m_vft;
  void *m_ifptr;
  float **m_sampleBuf;
  uint16_t *m_bufs;
  //AudioFileStruct  *audiFile;
  int m_object_type;
  int m_record_sync_time;
  uint64_t m_current_time;
  uint64_t m_last_rtp_ts;
  uint64_t m_msec_per_frame;
  uint32_t m_current_freq_time;
  uint32_t m_samples_per_frame;
  uint32_t m_current_frame;
  int m_audio_inited;
  int m_celp_inited;
  uint32_t m_freq;  // frequency
  int m_chans; // channels
  int m_output_frame_size;
  //unsigned char *m_temp_buff;
  int m_last;
  #if DUMP_OUTPUT_TO_FILE
  FILE *m_outfile;
#endif
  FILE *m_ifile;
  uint8_t *m_buffer;
  uint32_t m_buffer_size_max;
  uint32_t m_buffer_size;
  uint32_t m_buffer_on;
  uint64_t m_framecount;
} celp_codec_t;

#define m_vft c.v.audio_vft
#define m_ifptr c.ifptr
#define MAX_READ_BUFFER (768 * 8)

#define celp_message celp->m_vft->log_msg
void celp_close(codec_data_t *ptr);
extern const char *celplib;

codec_data_t *celp_file_check(lib_message_func_t message,
			     const char *name,
			     double *max,
			     char *desc[4]);

int celp_file_next_frame(codec_data_t *ifptr,
			uint8_t **buffer,
			uint64_t *ts);
int celp_file_eof(codec_data_t *ifptr);

void celp_file_used_for_frame(codec_data_t *ifptr,
			     uint32_t bytes);

int celp_raw_file_seek_to(codec_data_t *ifptr,
			 uint64_t ts);
#endif

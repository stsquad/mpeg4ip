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
 * a52dec.h - class definition for raw audio codec
 */

#ifndef __A52DEC_H__
#define __A52DEC_H__ 1
#include "codec_plugin.h"
#ifdef __cplusplus
extern "C" {
#endif
#include <a52dec/a52.h>
#ifdef __cplusplus
}
#endif

#define m_vft c.v.audio_vft
#define m_ifptr c.ifptr

typedef struct a52dec_codec_t {
  codec_data_t c;
  int m_initialized;
  int m_chans;
  int m_freq;
  uint64_t m_last_ts;
  uint32_t m_frames_at_ts;
  a52_state_t *m_state;
  uint8_t *m_buffer;
  uint32_t m_framecount;
  uint32_t m_buffer_on;
  uint32_t m_buffer_size;
  uint32_t m_buffer_size_max;
  FILE *m_ifile;
  //#define OUTPUT_TO_FILE 1
#ifdef OUTPUT_TO_FILE
  FILE *m_outfile;
#endif
} a52dec_codec_t;

#define MAX_READ_BUFFER (768 * 8 * 2)

codec_data_t *ac3_file_check(lib_message_func_t message,
			     const char *name, 
			     double *max, 
			     char *desc[4],
			     CConfigSet *pConfig);
int ac3_file_next_frame(codec_data_t *your,
			 uint8_t **buffer, 
			 frame_timestamp_t *ts);
void ac3_file_used_for_frame(codec_data_t *ifptr, 
			     uint32_t bytes);
int ac3_file_eof (codec_data_t *ifptr);
int ac3_raw_file_seek_to(codec_data_t *ifptr, uint64_t ts);



#endif

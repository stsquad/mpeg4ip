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
 * Copyright (C) Cisco Systems Inc. 2002.  All Rights Reserved.
 * 
 * Contributor(s): 
 *              Bill May        wmay@cisco.com
 */
/*
 * ourxvid.h - plugin interface to XVID decoder
 */
#ifndef __OURXVID_H__
#define __OURXVID_H__ 1
#include "codec_plugin.h"
#include <fposrec/fposrec.h>
#include <mp4av.h>

#define XVID_STATE_VO_SEARCH 0
#define XVID_STATE_NORMAL 1
#define XVID_STATE_WAIT_I 2

DECLARE_CONFIG(CONFIG_USE_XVID);

typedef struct xvid_codec_t {
  codec_data_t c;
  int m_nFrames;
  uint32_t m_num_wait_i;
  int m_decodeState;
  uint32_t m_num_wait_i_frames;
  uint32_t m_total_frames;
  // raw file support
  FILE *m_ifile;
  uint8_t *m_buffer;
  uint32_t m_buffer_size_max;
  uint32_t m_buffer_size;
  uint32_t m_buffer_on;
  uint32_t m_framecount;
  uint32_t m_frame_on;
  CFilePosRecorder *m_fpos;
  video_info_t *m_vinfo;
  int m_short_headers;
  void *m_xvid_handle;
  uint32_t m_width, m_height;
  mp4av_pts_to_dts_t pts_to_dts;
} xvid_codec_t;

#define m_vft c.v.video_vft
#define m_ifptr c.ifptr

void xvid_clean_up(xvid_codec_t *divx);

codec_data_t *xvid_file_check(lib_message_func_t message,
			      const char *name, 
			      double *max,
			      char *desc[4],
			      CConfigSet *pConfig);
int xvid_file_next_frame(codec_data_t *your_data,
			  uint8_t **buffer, 
			 frame_timestamp_t *ts);
void xvid_file_used_for_frame(codec_data_t *your,uint32_t bytes);
int xvid_file_seek_to(codec_data_t *you, uint64_t ts);
int xvid_file_eof (codec_data_t *ifptr);
#endif
/* end file ourxvid.h */

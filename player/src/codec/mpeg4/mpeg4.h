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
 * mpeg4.h - interface to iso reference codec
 */
#ifndef __MPEG4_H__
#define __MPEG4_H__ 1

#include <codec_plugin.h>
#include <fposrec/fposrec.h>
#define DECODE_STATE_VOL_SEARCH 0
#define DECODE_STATE_NORMAL 1
#define DECODE_STATE_WAIT_I 2

DECLARE_CONFIG(CONFIG_USE_MPEG4_ISO_ONLY);

class CVideoObjectDecoder;

#define m_vft c.v.video_vft
#define m_ifptr c.ifptr

typedef struct iso_decode_t {
  codec_data_t c;
  CVideoObjectDecoder *m_pvodec;
  int m_main_short_video_header;
  int m_nFrames;
  int m_decodeState;
  int m_bSpatialScalability;
  int m_bCachedRefFrame;
  int m_bCachedRefFrameCoded;
  int m_dropFrame;
  int m_cached_valid;
  uint64_t m_cached_time;
  uint64_t m_last_time;
  uint32_t m_dropped_b_frames;
  uint32_t m_num_wait_i;
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
  int m_framerate;
  video_info_t *m_vinfo;
  int m_short_header;
} iso_decode_t;

void iso_clean_up(iso_decode_t *iso);

codec_data_t *mpeg4_iso_file_check(lib_message_func_t message,
				   const char *name, 
				   double *max,
				   char *desc[4],
				   CConfigSet *pConfig);

int divx_file_next_frame(codec_data_t *your_data,
			 uint8_t **buffer, 
			 frame_timestamp_t *ts);

void divx_file_used_for_frame(codec_data_t *your,
			      uint32_t bytes);

int divx_file_seek_to(codec_data_t *your, uint64_t ts);
int divx_file_eof(codec_data_t *your);
#endif
/* end file mpeg4.h */

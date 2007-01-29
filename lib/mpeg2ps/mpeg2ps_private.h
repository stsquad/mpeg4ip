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
 *		Bill May wmay@cisco.com
 */

#ifndef __MPEG2PS_PRIVATE_H__
#define __MPEG2PS_PRIVATE_H__ 1
#include "mpeg4ip.h"

static __inline uint16_t convert16 (uint8_t *p)
{
#ifdef WORDS_BIGENDIAN
  return *(uint16_t *)p;
#else
  return ntohs(*(uint16_t *)p);
#endif
}

static __inline uint32_t convert32 (uint8_t *p)
{
#ifdef WORDS_BIGENDIAN
  return *(uint32_t *)p;
#else
  return ntohl(*(uint32_t *)p);
#endif
}

// these are defined in case I want to change to FILE * later.
#define FDTYPE int
#define FDNULL 0

/*
 * structure for passing timestamps around
 */
typedef struct mpeg2ps_ts_t
{
  bool have_pts;
  bool have_dts;
  uint64_t pts;
  uint64_t dts;
} mpeg2ps_ts_t;

typedef struct mpeg2ps_record_pes_t
{
  struct mpeg2ps_record_pes_t *next_rec;
  uint64_t dts;
  off_t location;
} mpeg2ps_record_pes_t;

/*
 * information about reading a stream
 */
typedef struct mpeg2ps_stream_t 
{
  mpeg2ps_record_pes_t *record_first, *record_last;
  FDTYPE m_fd;
  bool is_video;
  uint8_t m_stream_id;    // program stream id
  uint8_t m_substream_id; // substream, for program stream id == 0xbd

  mpeg2ps_ts_t next_pes_ts, frame_ts, next_next_pes_ts;
  uint frames_since_last_ts;
  uint64_t last_ts;

  bool have_frame_loaded;
  /*
   * pes_buffer processing.  this contains the raw elementary stream data
   */
  uint8_t *pes_buffer;
  uint32_t pes_buffer_size;
  uint32_t pes_buffer_size_max;
  uint32_t pes_buffer_on;
  uint32_t frame_len;
  uint32_t pict_header_offset; // for mpeg video

  // timing information and locations.
  off_t first_pes_loc;
  uint64_t start_dts;
  bool first_pes_has_dts;
  off_t    end_dts_loc;
  uint64_t end_dts;
  // audio stuff
  uint32_t freq;
  uint32_t channels;
  uint32_t bitrate;
  uint32_t samples_per_frame;
  uint8_t layer;
  uint8_t version;
  // video stuff
  uint32_t h, w;
  double frame_rate;
  bool determined_type;
  bool have_h264;
  int have_mpeg2;
  double bit_rate;
  uint64_t ticks_per_frame;
  uint8_t video_profile;
  uint8_t video_level;
  uint8_t audio_private_stream_info[6];
  bool lpcm_read_offset;
} mpeg2ps_stream_t;

#define LPCM_FRAME_COUNT 0
#define LPCM_PES_OFFSET_MSB 1
#define LPCM_PES_OFFSET_LSB 2
#define LPCM_INFO 4

/*
 * main interface structure - contains stream pointers and other
 * information
 */
struct mpeg2ps_ {
  mpeg2ps_stream_t *video_streams[16];
  mpeg2ps_stream_t *audio_streams[32];
  const char *filename;
  FDTYPE fd;
  uint64_t first_dts;
  uint audio_cnt, video_cnt;
  off_t end_loc;
  uint64_t max_dts;
  uint64_t max_time;  // time is in msec.
};

void mpeg2ps_message(int loglevel, const char *fmt, ...);

void mpeg2ps_record_pts(mpeg2ps_stream_t *sptr, off_t location,
			mpeg2ps_ts_t *pTs);

mpeg2ps_record_pes_t *search_for_ts(mpeg2ps_stream_t *sptr, 
				    uint64_t dts);
#endif

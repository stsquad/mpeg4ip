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
#ifndef __MEDIA_UTILS_H__
#define __MEDIA_UTILS_H__ 1

#include "player_session.h"
#include "our_bytestream.h"
#include "mp4.h"

typedef struct video_query_t {
  uint32_t track_id;
  const char *stream_type;
  const char *compressor;
  const char *original_fmt;
  int type;
  int profile;
  format_list_t *fptr;
  int h;
  int w;
  double frame_rate;
  const uint8_t *config;
  uint32_t config_len;
  // user supplied
  int enabled;
  void *reference;
} video_query_t;

typedef struct audio_query_t {
  uint32_t track_id;
  const char *stream_type;
  const char *compressor;
  int type;
  int profile;
  format_list_t *fptr;
  int sampling_freq;
  int chans;
  const uint8_t *config;
  uint32_t config_len;
  // user supplied
  int enabled;
  void *reference;
} audio_query_t;

typedef struct text_query_t {
  uint32_t track_id;
  const char *stream_type;
  const char *compressor;
  format_list_t *fptr;
  const uint8_t *config;
  uint32_t config_len;
  // user supplied
  int enabled;
  void *reference;
} text_query_t;

typedef void (*media_list_query_f)(CPlayerSession *psptr,
				   uint num_video, 
				   video_query_t *,
				   uint num_audio,
				   audio_query_t *,
				   uint num_text,
				   text_query_t *);

typedef struct control_callback_vft_t {
  media_list_query_f media_list_query;
} control_callback_vft_t;

class CMsgQueue;

CPlayerSession *start_session(CMsgQueue *master_queue,
			      SDL_sem *master_sem,
			      void *persist,
			      const char *name,
			      control_callback_vft_t *cc_vft,
			      int audio_volume,
			      int screen_loc_x,
			      int screen_loc_y,
			      int screen_size,
			      double start_time = 0,
			      struct rtp *video_rtp = NULL,
			      struct rtp *audio_rtp = NULL );
// internal api only
int parse_name_for_session(CPlayerSession *psptr,
			    const char *name,
			    control_callback_vft_t *cc_vft);

int check_name_for_network(const char *name, int &isOnDemand, int &isRtpOverRtsp);
int lookup_audio_codec_by_name(const char *name);
int lookup_video_codec_by_name(const char *name);

enum {
  VIDEO_MPEG4_ISO,
  VIDEO_DIVX,
  VIDEO_MPEG4_ISO_OR_DIVX,
  VIDEO_MPEG12,
};

enum {
  MPEG4IP_AUDIO_MP3,
  MPEG4IP_AUDIO_WAV,
  MPEG4IP_AUDIO_MP3_ROBUST,
  MPEG4IP_AUDIO_GENERIC,
  MPEG4IP_AUDIO_NONE,
};
#endif

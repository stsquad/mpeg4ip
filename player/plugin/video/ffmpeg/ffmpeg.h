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
 * ffmpeg.h - interface for ffmpeg video codecs
 */
#ifndef __MPEG4IP_FFMPEG_H__
#define __MPEG4IP_FFMPEG_H__ 1
#define always_inline inline
#include "codec_plugin.h"
extern "C" {
#ifdef HAVE_FFMPEG_INSTALLED
#include <ffmpeg/avcodec.h>
#else
#include <avcodec.h>
#endif
}
#include <mp4av.h>

DECLARE_CONFIG(CONFIG_USE_FFMPEG);

#define m_vft c.v.video_vft
#define m_ifptr c.ifptr

typedef struct ffmpeg_codec_t {
  codec_data_t c;
  enum CodecID m_codecId;
  AVCodec *m_codec;
  AVCodecContext *m_c;
  AVFrame *m_picture;
  bool m_codec_opened;
  bool m_video_initialized;
  int m_did_pause;
  bool m_got_i;
  bool have_cached_ts;
  uint64_t cached_ts;
  mpeg3_pts_to_dts_t pts_convert;
  mp4av_pts_to_dts_t pts_to_dts;
} ffmpeg_codec_t;
  
#endif
/* end file ffmpeg.h */

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
 * codec.h - base classes for audio/video codecs.
 */
#ifndef __CODEC_H__
#define __CODEC_H__ 1

#include <stdio.h>
#include <stdlib.h>
#include <fstream.h>
#include <sdp/sdp.h>
#include "player_media.h"
#include "player_rtp_bytestream.h"
#include "video.h"

class CCodecBase {
 public:
  CCodecBase() {};
  virtual ~CCodecBase() {};
  virtual int decode(uint64_t rtptime, int from_rtp) = 0;
  virtual void do_pause(void) = 0;
};

class CAudioCodecBase : public CCodecBase {
 public:
  CAudioCodecBase (CAudioSync *audio_sync,
		   CInByteStreamBase *pbytestream,
		   format_list_t *media_fmt,
		   audio_info_t *audio,
		   const char *userdata = NULL,
		   size_t userdata_size = 0) : CCodecBase() {
    m_audio_sync = audio_sync;
    m_bytestream = pbytestream;
    m_media_fmt = media_fmt;
    m_audio_info = audio;
    m_userdata = userdata;
    m_userdata_size = userdata_size;
  };
 protected:
  CAudioSync *m_audio_sync;
  CInByteStreamBase *m_bytestream;
  format_list_t *m_media_fmt;
  audio_info_t *m_audio_info;
  const char *m_userdata;
  size_t m_userdata_size;
};

class CVideoCodecBase : public CCodecBase {
 public:
  CVideoCodecBase (CVideoSync *video_sync,
		   CInByteStreamBase *pbytestream,
		   format_list_t *media_fmt,
		   video_info_t *vid,
		   const char *userdata = NULL,
		   size_t userdata_size = 0) : CCodecBase() {
    m_video_sync = video_sync;
    m_bytestream = pbytestream;
    m_media_fmt = media_fmt;
    m_video_info = vid;
    m_userdata = userdata;
    m_userdata_size = userdata_size;
  };
 protected:
  CVideoSync *m_video_sync;
  CInByteStreamBase *m_bytestream;
  format_list_t *m_media_fmt;
  video_info_t *m_video_info;
  const char *m_userdata;
  size_t m_userdata_size;
};

#endif

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
 * mp3.h - class definition for mp3 codec.
 */

#ifndef __MP3_H__
#define __MP3_H__ 1

#include "systems.h"
#include <mp3/MPEGaudio.h>
#include "codec.h"
#include "audio.h"
#include "player_util.h"

class COurMp3Loader;

class CMP3Codec : public CAudioCodecBase {
 public:
  CMP3Codec(CAudioSync *a,
	    CInByteStreamBase *pbytestrm,
	    format_list_t *media_desc,
	    audio_info_t *audio,
	    const unsigned char *userdata = NULL,
	    uint32_t userdata_size = 0);
  ~CMP3Codec();
  int decode(uint64_t rtptime, int fromrtp);
  int skip_frame(uint64_t rtptime);
  void do_pause(void);
  //  int read_byte(FILE_STREAM *fs, int *err);
  //void reset(void);
  //int peek(void *data, int len);
  CInByteStreamBase *m_bytestream;
 private:
  MPEGaudio *m_mp3_info;
  int m_resync_with_header;
  int m_record_sync_time;
  uint64_t m_first_time_offset;
  uint64_t m_current_time;
  uint64_t m_last_rtp_ts;
  uint32_t m_current_frame;
  int m_audio_inited;
  int m_freq;  // frequency
  int m_chans, m_samplesperframe;
#ifdef OUTPUT_TO_FILE
  FILE *m_output_file;
#endif
};

#ifdef _WIN32
DEFINE_MESSAGE_MACRO(mp3_message, "mp3")
#else
#define mp3_message(loglevel, fmt...) message(loglevel, "mp3", fmt)
#endif

#endif

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
 * audio.h - provides a class that interfaces between the codec and
 * the SDL audio application.  Will provide for volume, buffering,
 * syncronization
 */

#ifndef __AUDIO_DUMMY_H__
#define __AUDIO_DUMMY_H__ 1

#include "systems.h"
#include "mpeg4ip_sdl_includes.h"
#include "codec_plugin.h"
#include "audio.h"

#define DECODE_BUFFERS_MAX 32

class CPlayerSession;
// states
class CDummyAudioSync : public CAudioSync {
 public:
  CDummyAudioSync(CPlayerSession *psptr) : CAudioSync(psptr) { 
    m_configed = 0;
  } ;

  uint8_t *get_audio_buffer(void);
  void filled_audio_buffer(uint64_t ts, int resync);
  void set_config(int freq, int channels, int format, uint32_t max_buffer_size);
  uint32_t load_audio_buffer(uint8_t *from, 
			     uint32_t bytes, 
			     uint64_t ts, 
			     int resync);

 protected:
  int m_freq;
  int m_chans;
  int m_decode_format;
  uint32_t m_max_sample_size;
  uint8_t *m_buffer;
  int m_configed;
};

CAudioSync *create_audio_sync(CPlayerSession *);

audio_vft_t *get_audio_vft(void);
#endif



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
 * audio.cpp provides an interface (CAudioSync) between the codec and
 * the SDL audio APIs.
 */
#include <stdlib.h>
#include <string.h>
#include "player_session.h"
#include "audio_dummy.h"
#include "player_util.h"
//#define DEBUG_SYNC 1
//#define DEBUG_AUDIO_FILL 1
//#define DEBUG_DELAY 1
#ifdef _WIN32
DEFINE_MESSAGE_MACRO(audio_message, "audiosync")
#else
#define audio_message(loglevel, fmt...) message(loglevel, "audiosync", fmt)
#endif

void CDummyAudioSync::set_config (int freq, int channels, int format, 
			     uint32_t max_buffer_size)
{
  uint32_t max_bytes;
  m_freq = freq;
  m_chans = channels;
  m_decode_format = format;
  m_max_sample_size = max_buffer_size;
  if (m_max_sample_size != 0) {
    max_bytes = m_chans * m_max_sample_size;
    if (m_decode_format == AUDIO_U8 || m_decode_format == AUDIO_S8) {
    } else {
      max_bytes *= 2;
    }
    m_buffer = (uint8_t *)malloc(max_bytes);
  } else {
    m_buffer = NULL;
  }
}
  
uint8_t *CDummyAudioSync::get_audio_buffer (void)
{
  return m_buffer;
}

void CDummyAudioSync::filled_audio_buffer (uint64_t ts, int resync)
{
  audio_message(LOG_DEBUG, "Filled audio buffer "U64" resync %d", 
		ts, resync);
}

uint32_t CDummyAudioSync::load_audio_buffer (uint8_t *from,
					     uint32_t bytes, 
					     uint64_t ts, 
					     int resync)
{
  audio_message(LOG_DEBUG, "Load audio buffer %d bytes at "U64" resync %d", 
		bytes, ts, resync);
  return bytes;
}

static void c_audio_config (void *ifptr, int freq, 
			    int chans, int format, uint32_t max_buffer_size)
{
  ((CDummyAudioSync *)ifptr)->set_config(freq,
				    chans,
				    format,
				    max_buffer_size);
}

static uint8_t *c_get_audio_buffer (void *ifptr)
{
  return ((CDummyAudioSync *)ifptr)->get_audio_buffer();
}

static void c_filled_audio_buffer (void *ifptr,
				   uint64_t ts,
				   int resync_req)
{
  ((CDummyAudioSync *)ifptr)->filled_audio_buffer(ts, 
					     resync_req);
}

static uint32_t c_load_audio_buffer (void *ifptr, 
				     uint8_t *from, 
				     uint32_t bytes, 
				     uint64_t ts, 
				     int resync)
{
  return ((CDummyAudioSync *)ifptr)->load_audio_buffer(from,
						  bytes,
						  ts, 
						  resync);
}
  
static audio_vft_t audio_vft = {
  message,
  c_audio_config,
  c_get_audio_buffer,
  c_filled_audio_buffer,
  c_load_audio_buffer
};

CAudioSync *create_audio_sync (CPlayerSession *psptr)
{
  return new CDummyAudioSync(psptr, psptr->get_audio_volume());
}

audio_vft_t *get_audio_vft (void)
{
  return &audio_vft;
}

int do_we_have_audio (void) 
{
  return (1);
}
/* end audio.cpp */


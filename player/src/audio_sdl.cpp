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
 * Copyright (C) Cisco Systems Inc. 2000-2005.  All Rights Reserved.
 * 
 * Contributor(s): 
 *              Bill May        wmay@cisco.com
 */
/*
 * audio_sdl.cpp provides the interface between the CBufferAudioSync
 * class and our version of the SDL audio APIs.
 */
#include <stdlib.h>
#include <string.h>
#include "player_session.h"
#include "audio_sdl.h"
#include "player_util.h"
#include "our_config_file.h"

//#define TEST_MONO_TO_STEREO 1

#ifdef _WIN32
DEFINE_MESSAGE_MACRO(audio_message, "audiosdl")
#else
#define audio_message(loglevel, fmt...) message(loglevel, "audiosdl", fmt)
#endif

/*
 * c routine to call into the AudioSync class callback
 */
static void c_audio_callback (void *userdata, Uint8 *stream, int len)
{
  CSDLAudioSync *a = (CSDLAudioSync *)userdata;
  a->audio_callback(stream, len);
}

void CSDLAudioSync::audio_callback(Uint8 *stream, int len)
{
  uint32_t latency;
  uint64_t our_time;
    our_time = get_time_of_day_usec();
  if (m_use_SDL_delay != 0) {
    latency = SDL_AUDIODELAY();
	if (latency < 0) latency = 0;
  } else {
    latency = 0;
  }

  audio_buffer_callback((uint8_t *)stream, len, latency, our_time);
}

/*
 * Create an CSDLAudioSync for a session.  Don't alloc any buffers until
 * config is called by codec
 */
CSDLAudioSync::CSDLAudioSync (CPlayerSession *psptr, int volume) :
  CBufferAudioSync(psptr, volume)
{
  SDL_AUDIOINIT(getenv("SDL_AUDIODRIVER"));
  m_volume = (volume * SDL_MIX_MAXVOLUME)/100;
}

/*
 * Close out audio sync - stop and disconnect from SDL
 */
CSDLAudioSync::~CSDLAudioSync (void)
{
  SDL_PAUSEAUDIO(1);
  SDL_CLOSEAUDIO();
}


// Sync task api - initialize the sucker.
// May need to check out non-standard frequencies, see about conversion.
// returns 1 for initialized, -1 for error
int CSDLAudioSync::InitializeHardware (void) 
{
  uint32_t bytes_per_sample;
  SDL_AudioSpec wanted;
  
  memset(&wanted, 0, sizeof(wanted));
  wanted.freq = m_freq;
  wanted.channels = m_channels;
  // bytes per sample is the # of bytes for the output buffer
  bytes_per_sample = 2;
  switch (m_decode_format) {
  case AUDIO_FMT_U8: wanted.format = AUDIO_U8; bytes_per_sample = 1; break;
  case AUDIO_FMT_S8: wanted.format = AUDIO_S8; bytes_per_sample = 1; break;
  case AUDIO_FMT_U16LSB: wanted.format = AUDIO_U16LSB; break;
  case AUDIO_FMT_U16MSB: wanted.format = AUDIO_U16MSB; break;
  case AUDIO_FMT_S16LSB: wanted.format = AUDIO_S16LSB; break;
  case AUDIO_FMT_S16MSB: wanted.format = AUDIO_S16MSB; break;
  case AUDIO_FMT_U16: wanted.format = AUDIO_U16SYS; break;
  case AUDIO_FMT_S16: wanted.format = AUDIO_S16SYS; break;
  case AUDIO_FMT_FLOAT: wanted.format = AUDIO_S16SYS; break;
  case AUDIO_FMT_HW_AC3: 
    wanted.format = AUDIO_FORMAT_HW_AC3; 
    bytes_per_sample = 1;
    break;
  }

  uint32_t sample_size;
  sample_size = m_samples_per_frame * m_channels;
#ifndef _WIN32
  uint32_t ix;
  for (ix = 2; ix <= 0x8000; ix <<= 1) {
    if ((sample_size & ~(ix - 1)) == 0) {
      break;
    }
  }
  ix >>= 1;
  audio_message(LOG_DEBUG, "Sample size is %d", ix);
  sample_size = ix;
#else
  sample_size = 4096;
#endif

  if (config.get_config_value(CONFIG_LIMIT_AUDIO_SDL_BUFFER) > 1) {
    sample_size = MAX(config.get_config_value(CONFIG_LIMIT_AUDIO_SDL_BUFFER),
		      sample_size);
  } else {
    if (config.get_config_value(CONFIG_LIMIT_AUDIO_SDL_BUFFER) > 0 &&
	sample_size > 4096) 
      sample_size = 4096;
    if (sample_size < m_freq / 10) 
      sample_size = 4096;
    
    while (sample_size * m_bytes_per_sample_input > m_sample_buffer_size / 4) {
      sample_size /= 2;
    }
  }
  wanted.samples = sample_size;
  wanted.callback = c_audio_callback;
  wanted.userdata = this;
#if 1
  audio_message(LOG_INFO, 
		"requested f %d chan %d format %x samples %d",
		wanted.freq,
		wanted.channels,
		wanted.format,
		wanted.samples);
#endif
  int ret = SDL_OPENAUDIO(&wanted, &m_obtained);
  if (ret < 0) {
    audio_message(LOG_ERR, "Couldn't open audio %d channels - %s",
		  wanted.channels, SDL_GetError());
    if (wanted.channels > 2) {
      wanted.channels = 2;
      ret = SDL_OPENAUDIO(&wanted, &m_obtained);
    }
    if (ret < 0) {
      audio_message(LOG_CRIT, "Couldn't open audio, %s", SDL_GetError());
      return (-1);
    }
  }

  char buffer[128];
  if (SDL_AUDIODRIVERNAME(buffer, sizeof(buffer)) == NULL) {
    strcpy(buffer, "no audio driver");
  }
  audio_message(LOG_INFO, "got f %d chan %d format %x samples %d size %u %s",
		m_obtained.freq,
		m_obtained.channels,
		m_obtained.format,
		m_obtained.samples,
		m_obtained.size,
		buffer);

#ifdef TEST_MONO_TO_STEREO
#define CHECK_SDL_CHANS_RETURNED  TRUE
#define OBTAINED_CHANS  2
#else
#define CHECK_SDL_CHANS_RETURNED m_obtained.channels != m_channels
#define OBTAINED_CHANS  m_obtained.channels
#endif
  bool need_convert = false;
  if (CHECK_SDL_CHANS_RETURNED) {
    SDL_CLOSEAUDIO();
    wanted.channels = OBTAINED_CHANS;
    wanted.format = AUDIO_S16SYS; // we're converting, so always choose
    m_bytes_per_sample_output = sizeof(int16_t) * wanted.channels;
    
    ret = SDL_OPENAUDIO(&wanted, &m_obtained);
    audio_message(LOG_INFO, 
		  "requested f %d chan %d format %x samples %d",
		  wanted.freq,
		  wanted.channels,
		  wanted.format,
		  wanted.samples);
    if (ret < 0) {
      audio_message(LOG_CRIT, "Couldn't reopen audio, %s", SDL_GetError());
      return (-1);
    }
    audio_message(LOG_INFO, "got f %d chan %d format %x samples %d size %u %s",
		  m_obtained.freq,
		  m_obtained.channels,
		  m_obtained.format,
		  m_obtained.samples,
		  m_obtained.size,
		  buffer);
    need_convert = true;
  } else {
    m_bytes_per_sample_output = m_obtained.channels * bytes_per_sample;
  }

  m_got_channels = m_obtained.channels;
  m_output_buffer_size_bytes = m_obtained.size;

  if (m_decode_format == AUDIO_FMT_FLOAT || need_convert) {
    audio_message(LOG_DEBUG, "convert buffer size is %d", m_obtained.size);
    audio_convert_init(m_obtained.size, m_obtained.samples);
  }

  m_use_SDL_delay = SDL_HAS_AUDIO_DELAY();
  if (m_use_SDL_delay)
    audio_message(LOG_NOTICE, "Using delay measurement from SDL");
  return 1; // check again pretty soon...
}

void CSDLAudioSync::StartHardware (void) 
{
  SDL_PAUSEAUDIO(0);
}

void CSDLAudioSync::StopHardware (void) 
{
  SDL_PAUSEAUDIO(1);
}

void CSDLAudioSync::CopyBytesToHardware (uint8_t *from,
					 uint8_t *to, 
					 uint32_t bytes)
{
  SDL_MIXAUDIO(/*(unsigned char *)*/to, 
		   /*(const unsigned char *)*/from,
		   bytes, 
		   m_volume);
}



void CSDLAudioSync::set_volume (int volume)
{
  m_volume = (volume * SDL_MIX_MAXVOLUME)/100;
}


/*
 * These are the function interfaces from the plugins to the 
 * CSDLAudioSync class functions
 */
static void c_audio_config (void *ifptr, int freq, 
			    int chans, audio_format_t format, 
			    uint32_t max_samples)
{
  ((CSDLAudioSync *)ifptr)->set_config(freq,
				       chans,
				       format,
				       max_samples);
}

static uint8_t *c_get_audio_buffer (void *ifptr,
				    uint32_t freq_ts,
				    uint64_t ts)
{
  return ((CSDLAudioSync *)ifptr)->get_audio_buffer(freq_ts, ts);
}

static void c_filled_audio_buffer (void *ifptr)
{
  ((CSDLAudioSync *)ifptr)->filled_audio_buffer();
}

static void c_load_audio_buffer (void *ifptr, 
				 const uint8_t *from, 
				 uint32_t bytes, 
				 uint32_t freq_ts,
				 uint64_t ts)
{
  ((CSDLAudioSync *)ifptr)->load_audio_buffer(from,
					      bytes,
					      freq_ts, 
					      ts);
}
  
static audio_vft_t audio_vft = {
  message,
  c_audio_config,
  c_get_audio_buffer,
  c_filled_audio_buffer,
  c_load_audio_buffer,
  NULL,
};

CAudioSync *create_audio_sync (CPlayerSession *psptr)
{
  return new CSDLAudioSync(psptr, psptr->get_audio_volume());
}

audio_vft_t *get_audio_vft (void)
{
  audio_vft.pConfig = &config;
  return &audio_vft;
}

int do_we_have_audio (void) 
{
  char buffer[80];
  if (SDL_AUDIOINIT(getenv("SDL_AUDIODRIVER")) < 0) {
    player_debug_message("Can't initialize SDL audio");
    return (0);
  } 
  if (SDL_AUDIODRIVERNAME(buffer, sizeof(buffer)) == NULL) {
    player_debug_message("No buffer name");
    return (0);
  }
  //  Our_SDL_CloseAudio(); - wmay - don't know why commented out - 
  // causes memleak if PlayAudio is 0.
  return (1);
}

/* end audio_sdl.cpp */


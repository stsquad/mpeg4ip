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
 * Copyright (C) Cisco Systems Inc. 2000-2004.  All Rights Reserved.
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
#include "audio_macosx.h"
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
static int c_audio_callback (void *inputBuffer,
			     void *outputBuffer,
			     unsigned long framesPerBuffer,
			     PaTimestamp outTime,
			     void *userdata)
{
  CPortAudioSync *a = (CPortAudioSync *)userdata;
  a->audio_callback(outputBuffer, framesPerBuffer, outTime);
  return 0;
}

void CPortAudioSync::audio_callback(void *stream, unsigned long len,
				    PaTimestamp outtime)
{
  uint64_t our_time;
  uint32_t outBufferTotalBytes = len * m_bytes_per_sample_output;
  our_time = get_time_of_day_usec();
  PaTimestamp realtime = Pa_StreamTime(m_pa_stream);
  uint64_t delay = (uint64_t)(outtime - realtime);
  audio_buffer_callback((uint8_t *)stream, outBufferTotalBytes, delay, our_time);
}

/*
 * Create an CPortAudioSync for a session.  Don't alloc any buffers until
 * config is called by codec
 */
CPortAudioSync::CPortAudioSync (CPlayerSession *psptr, int volume) :
  CBufferAudioSync(psptr, volume)
{
  PaError err;
  err = Pa_Initialize();
  if (err != paNoError) {
    audio_message(LOG_CRIT, "resp from Pa_Initialize is %d %s", err,
		 Pa_GetErrorText(err));
  }
    
  m_volume = volume;
  m_pa_stream = NULL;
}

/*
 * Close out audio sync - stop and disconnect from SDL
 */
CPortAudioSync::~CPortAudioSync (void)
{
  Pa_StopStream(m_pa_stream);
  Pa_CloseStream(m_pa_stream);
  Pa_Terminate();
}

static int fmt_to_pa_format[] = {
  paUInt8,
  paInt8,
  0,
  0,
  0,
  0,
  0,
  paInt16,
  0,
};

// Sync task api - initialize the sucker.
// May need to check out non-standard frequencies, see about conversion.
// returns 1 for initialized, -1 for error
int CPortAudioSync::InitializeHardware (void) 
{
  uint32_t bytes_per_sample;
  uint32_t sample_size;
  bool need_convert = false;
  
  // bytes per sample is the # of bytes for the output buffer
  bytes_per_sample = 2;
  if (m_decode_format == AUDIO_FMT_U8 || m_decode_format == AUDIO_FMT_S8)
    bytes_per_sample = 1; 

  sample_size = m_samples_per_frame;
  uint32_t ix;
  for (ix = 2; ix <= 0x8000; ix <<= 1) {
    if ((sample_size & ~(ix - 1)) == 0) {
      break;
    }
  }

  audio_message(LOG_DEBUG, "Sample size is %d %d", ix, m_samples_per_frame);
  sample_size = ix;
#if 0
  if (m_output_buffer_size_bytes < 4096)
    m_output_buffer_size_bytes = 4096;
#endif
  if (config.get_config_value(CONFIG_LIMIT_AUDIO_SDL_BUFFER) > 0 &&
      sample_size > 4096) 
    sample_size = 4096;

  while (sample_size * m_bytes_per_sample_input > m_sample_buffer_size / 4) {
    sample_size /= 2;
  }

  PaSampleFormat format;
  double freq = (double) m_freq;
  format = fmt_to_pa_format[m_decode_format];
  if (format == 0) {
    format = paInt16;
    need_convert = true;
  }
  PaError err;
  err = Pa_OpenDefaultStream(&m_pa_stream,
			     0,
			     m_channels,
			     format,
			     freq,
			     sample_size,
			     0,
			     c_audio_callback,
			     this);

  m_got_channels = m_channels;
  if (err != paNoError) {
    if (err == paInvalidChannelCount) {
      format = paInt16;
      err = Pa_OpenDefaultStream(&m_pa_stream,
				 0,
				 2,
				 format,
				 freq,
				 sample_size,
				 0,
				 c_audio_callback,
				 this);
      m_got_channels = 2;
      need_convert = true;
    }
    if (err != paNoError) {
      audio_message(LOG_CRIT, "Couldn't open audio, %s", 
		    Pa_GetErrorText(err));
      return (-1);
    }
  }
    
  audio_message(LOG_DEBUG, "freq %u chans %u samples %u", 
		m_freq, m_got_channels, sample_size);
  m_output_buffer_size_bytes = sample_size * m_bytes_per_sample_input;
  if (m_decode_format == AUDIO_FMT_FLOAT || need_convert) {
    m_bytes_per_sample_output = sizeof(int16_t) * m_got_channels;
    audio_message(LOG_DEBUG, "convert buffer size is %d", m_output_buffer_size_bytes);
    audio_convert_init(m_output_buffer_size_bytes, sample_size);
  } else {
    m_bytes_per_sample_output = m_got_channels * bytes_per_sample;
  }

  return 1; 
}

void CPortAudioSync::StartHardware (void) 
{
  Pa_StartStream(m_pa_stream);
}

void CPortAudioSync::StopHardware (void) 
{
  Pa_StopStream(m_pa_stream);
}

void CPortAudioSync::CopyBytesToHardware (uint8_t *from,
					 uint8_t *to, 
					 uint32_t bytes)
{
  memcpy(to, from, bytes);
}



void CPortAudioSync::set_volume (int volume)
{
  m_volume = (volume * SDL_MIX_MAXVOLUME)/100;
}


/*
 * These are the function interfaces from the plugins to the 
 * CPortAudioSync class functions
 */
static void c_audio_config (void *ifptr, int freq, 
			    int chans, audio_format_t format, 
			    uint32_t max_samples)
{
  ((CPortAudioSync *)ifptr)->set_config(freq,
				       chans,
				       format,
				       max_samples);
}

static uint8_t *c_get_audio_buffer (void *ifptr,
				    uint32_t freq_ts,
				    uint64_t ts)
{
  return ((CPortAudioSync *)ifptr)->get_audio_buffer(freq_ts, ts);
}

static void c_filled_audio_buffer (void *ifptr)
{
  ((CPortAudioSync *)ifptr)->filled_audio_buffer();
}

static void c_load_audio_buffer (void *ifptr, 
				 const uint8_t *from, 
				 uint32_t bytes, 
				 uint32_t freq_ts,
				 uint64_t ts)
{
  ((CPortAudioSync *)ifptr)->load_audio_buffer(from,
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
  return new CPortAudioSync(psptr, psptr->get_audio_volume());
}

audio_vft_t *get_audio_vft (void)
{
  audio_vft.pConfig = &config;
  return &audio_vft;
}

int do_we_have_audio (void) 
{
  PaError err;
  err = Pa_Initialize();

  if (err != paNoError) {
    audio_message(LOG_ERR, "No audio - %d %s", err, 
		  Pa_GetErrorText(err));
    return 0;
  }
 
  Pa_Terminate();
  return (1);
}

/* end audio_sdl.cpp */


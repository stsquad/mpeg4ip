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
#include "ourwav.h"

#define DEBUG_SYNC
/*
 * Create CAACodec class
 */
static codec_data_t *wav_codec_create (const char *stream_type, 
				       const char *compressor,
				       int type, 
				       int profile, 
				       format_list_t *media_fmt,
				       audio_info_t *audio,
				       const uint8_t *userdata,
				       uint32_t userdata_size,
				       audio_vft_t *vft,
				       void *ifptr)
{ 
  wav_codec_t *wav = (wav_codec_t *)malloc(sizeof(wav_codec_t));
  memset(wav, 0, sizeof(*wav));
  wav->m_vft = vft;
  wav->m_ifptr = ifptr;
  wav->m_sdl_config = (SDL_AudioSpec *)userdata;
  return ((codec_data_t *)wav);
}

static void wav_close (codec_data_t *ptr)
{
  wav_codec_t *wav;
  wav = (wav_codec_t *)ptr;
  if (wav->m_wav_buffer != NULL) {
    SDL_FreeWAV(wav->m_wav_buffer);
    wav->m_wav_buffer = NULL;
  }
  if (wav->m_sdl_config != NULL) {
    free(wav->m_sdl_config);
    wav->m_sdl_config = NULL;
  }
  free(ptr);
}

/*
 * Handle pause - basically re-init the codec
 */
static void wav_do_pause (codec_data_t *ifptr)
{

}

/*
 * Decode task call for FAAC
 */
static int wav_decode (codec_data_t *ifptr, 
		       frame_timestamp_t *ts, 
		       int from_rtp,
		       int *sync_frame,
		       uint8_t *buffer, 
		       uint32_t buflen,
		       void *userdata)
{
  wav_codec_t *wav = (wav_codec_t *)ifptr;
  uint32_t freq_timestamp;
  if ((uint32_t)wav->m_sdl_config->freq != ts->audio_freq) {
    abort();
  }
  freq_timestamp = ts->audio_freq_timestamp;
  if (wav->m_configured == 0) {
    wav->m_configured = 1;
    audio_format_t fmt;

    switch (wav->m_sdl_config->format) {
    case AUDIO_U8: fmt = AUDIO_FMT_U8; break;
    case AUDIO_S16LSB: fmt = AUDIO_FMT_S16LSB; break;
    case AUDIO_U16LSB: fmt = AUDIO_FMT_U16LSB; break;
    case AUDIO_S16MSB: fmt = AUDIO_FMT_S16MSB; break;
    default:
    case AUDIO_U16MSB: fmt = AUDIO_FMT_U16MSB; break;
    }
    wav->m_vft->audio_configure(wav->m_ifptr,
				wav->m_sdl_config->freq, 
				wav->m_sdl_config->channels, 
				fmt,
				wav->m_sdl_config->samples);
    if (wav->m_sdl_config->format == AUDIO_U8 || 
	wav->m_sdl_config->format == AUDIO_S8)
      wav->m_bytes_per_channel = 1;
    else
      wav->m_bytes_per_channel = 2;
  }
  /* 
   * Get an audio buffer
   */
  uint32_t bytes;
  bytes = 1024 * wav->m_bytes_per_channel * wav->m_sdl_config->channels;
  bytes = MIN(bytes, buflen);
  wav->m_vft->audio_load_buffer(wav->m_ifptr, 
				buffer,
				bytes,
				freq_timestamp,
				ts->msec_timestamp);
  return (bytes);
}

static int wav_codec_check (lib_message_func_t message,
			    const char *stream_type,
			    const char *compressor,
			    int audio_format,
			    int profile, 
			    format_list_t *fptr,
			    const uint8_t *userdata,
			    uint32_t userdata_size,
			    CConfigSet *pConfig)
{
  return -1;
}

AUDIO_CODEC_WITH_RAW_FILE_PLUGIN("wav", 
				 wav_codec_create,
				 wav_do_pause,
				 wav_decode,
				 NULL,
				 wav_close,
				 wav_codec_check,
				 wav_file_check,
				 wav_file_next_frame,
				 wav_file_used_for_frame,
				 NULL,
				 wav_file_eof,
				 NULL,
				 0);

/* end file ourwav.cpp */



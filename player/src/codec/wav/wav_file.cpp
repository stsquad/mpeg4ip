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
 * wav_file.cpp - create media structure for aac files
 */

#include "ourwav.h"

codec_data_t *wav_file_check (lib_message_func_t message,
			      const char *name, 
			      double *max, 
			      char *desc[4],
			      CConfigSet *pConfig)
{

  int len = strlen(name);
  if (strcasecmp(name + len - 4, ".wav") != 0) {
    return (NULL);
  }

  SDL_AudioSpec *temp, *read;
  Uint8 *wav_buffer;
  Uint32 wav_len;

  temp = (SDL_AudioSpec *)malloc(sizeof(SDL_AudioSpec));
  read = SDL_LoadWAV(name, temp, &wav_buffer, &wav_len);
  if (read == NULL) {
    message(LOG_DEBUG, "libwav", "Can't decode wav file");
    return (NULL);
  }
  message(LOG_DEBUG, "libwav", 
	  "Wav got f %d chan %d format %x samples %d size %u",
	  temp->freq,
	  temp->channels,
	  temp->format,
	  temp->samples,
	  wav_len);

  wav_codec_t *wav;
  wav = MALLOC_STRUCTURE(wav_codec_t);
  memset(wav, 0, sizeof(*wav));
  
  wav->m_sdl_config = temp;
  wav->m_wav_buffer = wav_buffer;
  wav->m_wav_len = wav_len;

  if (wav->m_sdl_config->format == AUDIO_U8 || 
      wav->m_sdl_config->format == AUDIO_S8)
    wav->m_bytes_per_channel = 1;
  else
    wav->m_bytes_per_channel = 2;

  int divs;

  divs = wav->m_bytes_per_channel * 
    wav->m_sdl_config->channels *
    wav->m_sdl_config->freq;

  *max = (double)wav->m_wav_len / (double) divs;
  message(LOG_DEBUG, "libwav", "wav length is %g", *max);
  return ((codec_data_t *)wav);
}


int wav_file_next_frame (codec_data_t *your,
			 uint8_t **buffer,
			 frame_timestamp_t *ts)
{
  wav_codec_t *wav = (wav_codec_t *)your;
  uint64_t calc;

  *buffer = &wav->m_wav_buffer[wav->m_wav_buffer_on];
  calc = wav->m_wav_buffer_on * TO_U64(1000);
  calc /= wav->m_bytes_per_channel;
  calc /= wav->m_sdl_config->channels;
  calc /= wav->m_sdl_config->freq;
  ts->msec_timestamp = calc;
  ts->audio_freq = wav->m_sdl_config->freq;
  ts->audio_freq_timestamp = wav->m_wav_buffer_on / 
    (wav->m_bytes_per_channel * wav->m_sdl_config->channels);
  ts->timestamp_is_pts = false;
  return (wav->m_wav_len - wav->m_wav_buffer_on);
}

void wav_file_used_for_frame (codec_data_t *ifptr,
			      uint32_t bytes)
{
  wav_codec_t *wav = (wav_codec_t *)ifptr;

  wav->m_wav_buffer_on += bytes;
  if (wav->m_wav_buffer_on > wav->m_wav_len) wav->m_wav_buffer_on = wav->m_wav_len;
}

int wav_file_eof (codec_data_t *ifptr)
{
  wav_codec_t *wav = (wav_codec_t *)ifptr;
  return (wav->m_wav_buffer_on == wav->m_wav_len);
}

int wav_raw_file_seek_to (codec_data_t *ifptr, uint64_t ts)
{
  wav_codec_t *wav = (wav_codec_t *)ifptr;
  uint64_t calc;
  
  calc = ts * wav->m_bytes_per_channel * 
    wav->m_sdl_config->channels *
    wav->m_sdl_config->freq;
  calc /= TO_U64(1000);
  wav->m_wav_buffer_on = calc;
  if (wav->m_bytes_per_channel != 1)
    wav->m_wav_buffer_on &= ~1;

  wav->m_vft->log_msg(LOG_DEBUG, "libwav", "skip " U64 " bytes %d max %d", 
		      ts, wav->m_wav_buffer_on, wav->m_wav_len);

  return 0;
}


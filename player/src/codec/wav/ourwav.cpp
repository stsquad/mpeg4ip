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
#include "our_bytestream.h"
#define DEBUG_SYNC
/*
 * Create CAACodec class
 */
CWavCodec::CWavCodec (CAudioSync *a, 
		      COurInByteStream *pbytestrm,
		      format_list_t *media_fmt,
		      audio_info_t *audio,
		      const unsigned char *userdata,
		      uint32_t userdata_size) : 
  CAudioCodecBase(a, pbytestrm, media_fmt, audio, userdata, userdata_size)
{
  
  m_sdl_config = (SDL_AudioSpec *)userdata;
  m_audio_sync->set_config(m_sdl_config->freq, 
			   m_sdl_config->channels, 
			   m_sdl_config->format, 
			   m_sdl_config->samples);
  if (m_sdl_config->format == AUDIO_U8 || m_sdl_config->format == AUDIO_S8)
    m_bytes_per_sample = 1;
  else
    m_bytes_per_sample = 2;
}

CWavCodec::~CWavCodec()
{

}

/*
 * Handle pause - basically re-init the codec
 */
void CWavCodec::do_pause (void)
{

}

/*
 * Decode task call for FAAC
 */
int CWavCodec::decode (uint64_t ts, 
		       int from_rtp,
		       unsigned char *buffer, 
		       uint32_t buflen)
{
  unsigned char *buff;
	
  /* 
   * Get an audio buffer
   */
  buff = m_audio_sync->get_audio_buffer();
  if (buff == NULL) {
    return (-1);
  }
	
  uint32_t bytes_to_copy;
  bytes_to_copy = m_sdl_config->samples * 
    m_sdl_config->channels * 
    m_bytes_per_sample;
  
  uint32_t bytes;
    
  bytes = MIN(bytes_to_copy, buflen);
  memcpy(buff, buffer, bytes);
  if (bytes < bytes_to_copy) {
    memset(&buff[bytes], 0, bytes_to_copy - bytes);
  }
  m_bytestream->used_bytes_for_frame(bytes);

  m_audio_sync->filled_audio_buffer(ts, 0);
  return (0);
}

int CWavCodec::skip_frame (uint64_t ts, unsigned char *buffer, uint32_t buflen)
{
  return (decode(ts, 0, buffer, buflen));
}
/* end file ourwav.cpp */



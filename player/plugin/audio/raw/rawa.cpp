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
#include "rawa.h"
#include <mp4v2/mp4.h>
#include <SDL.h>
/*
 * Create CAACodec class
 */
static codec_data_t *rawa_codec_create (format_list_t *media_fmt,
					audio_info_t *audio,
					const unsigned char *userdata,
					uint32_t userdata_size,
					audio_vft_t *vft,
					void *ifptr)

{
  rawa_codec_t *rawa;

  if (audio == NULL) return NULL;

  rawa = (rawa_codec_t *)malloc(sizeof(rawa_codec_t));
  memset(rawa, 0, sizeof(rawa_codec_t));

  rawa->m_vft = vft;
  rawa->m_ifptr = ifptr;
  rawa->m_initialized = 0;
  rawa->m_temp_buff = NULL;
  rawa->m_freq = audio->freq;
  rawa->m_chans = audio->chans;
  rawa->m_bitsperchan = audio->bitspersample;

  return (codec_data_t *)rawa;
}

static void rawa_close (codec_data_t *ptr)
{
  rawa_codec_t *rawa = (rawa_codec_t *)ptr;
  if (rawa->m_temp_buff != NULL) free(rawa->m_temp_buff);
  free(rawa);
}

/*
 * Handle pause - basically re-init the codec
 */
static void rawa_do_pause (codec_data_t *ifptr)
{
  rawa_codec_t *rawa = (rawa_codec_t *)ifptr;
  rawa->m_resync = 1;
}

/*
 * Decode task call for FAAC
 */
static int rawa_decode (codec_data_t *ptr,
		       uint64_t ts,
		       int from_rtp,
		       int *sync_frame,
		       unsigned char *buffer,
		       uint32_t buflen)
{
  rawa_codec_t *rawa = (rawa_codec_t *)ptr;
  int samples;
  
  

  if (rawa->m_initialized == 0) {
    if (rawa->m_temp_buff == NULL) {
      rawa->m_temp_buff = (unsigned char *)malloc(buflen);
      memcpy(rawa->m_temp_buff, buffer, buflen);
      rawa->m_temp_buffsize = buflen;
      rawa->m_vft->log_msg(LOG_DEBUG, "rawaudio", "setting %d bufsize", 
			   rawa->m_temp_buffsize);
      return (buflen);
    } else {
      //
      if (buflen != rawa->m_temp_buffsize) {
	rawa->m_vft->log_msg(LOG_ERR, "rawaudio", "Inconsistent raw audio buffer size %d should be %d", buflen, rawa->m_temp_buffsize);
	return buflen;
      }

      uint64_t calc;

      if (rawa->m_chans == 0) {
	rawa->m_vft->log_msg(LOG_DEBUG, "rawaudio",
			     "freq %d ts "LLU" buffsize %d",
			     rawa->m_freq, ts, rawa->m_temp_buffsize);

	calc = ts * rawa->m_temp_buffsize;
	calc /= 2000;
	calc /= rawa->m_freq;
	rawa->m_vft->log_msg(LOG_DEBUG, "rawaudio", "Channels is %d", calc);
	rawa->m_chans = calc;
	rawa->m_bitsperchan = 16;
      } 

      samples = rawa->m_temp_buffsize;
      samples /= rawa->m_chans;
      if (rawa->m_bitsperchan == 16) samples /= 2;
      rawa->m_vft->audio_configure(rawa->m_ifptr,
				   rawa->m_freq, 
				   rawa->m_chans, 
				   rawa->m_bitsperchan == 16 ? AUDIO_S16SYS :
				   AUDIO_U8, 
				   samples);
      unsigned char *temp = rawa->m_vft->audio_get_buffer(rawa->m_ifptr);
      memcpy(temp, rawa->m_temp_buff, rawa->m_temp_buffsize);
      rawa->m_vft->audio_filled_buffer(rawa->m_ifptr,
				       0,
				       1);
      if (ts == 0) rawa->m_frames = 1;
      free(rawa->m_temp_buff);
      rawa->m_temp_buff = NULL;
      rawa->m_initialized = 1;
    }
  } else {
    samples = rawa->m_temp_buffsize;
    samples /= rawa->m_chans;
    if (rawa->m_bitsperchan == 16) samples /= 2;
  }

  if (ts == rawa->m_ts) {
    uint64_t calc;
    calc = rawa->m_frames * samples * M_LLU;
    calc /= rawa->m_freq;
    ts += calc;
    rawa->m_frames++;
  } else {
    rawa->m_frames = 0;
    rawa->m_ts = ts;
  }
  unsigned char *now = rawa->m_vft->audio_get_buffer(rawa->m_ifptr);
  if (now != NULL) {
    memcpy(now, buffer, buflen);
    rawa->m_vft->audio_filled_buffer(rawa->m_ifptr,
				     ts,
				     rawa->m_resync);
    rawa->m_resync = 0;
  }

  return (buflen);
}


static int rawa_codec_check (lib_message_func_t message,
			    const char *compressor,
			    int type,
			    int profile,
			    format_list_t *fptr, 
			    const unsigned char *userdata,
			    uint32_t userdata_size)
{
  if (compressor != NULL && 
      strcasecmp(compressor, "MP4 FILE") == 0 &&
      type != -1) {
    if (type == MP4_PCM16_AUDIO_TYPE)
      return 1;
  }
  if (compressor != NULL &&
      strcasecmp(compressor, "AVI FILE") == 0 &&
      type == 1) {
    return 1;
  }
  return -1;
}

AUDIO_CODEC_PLUGIN("rawa",
		   rawa_codec_create,
		   rawa_do_pause,
		   rawa_decode,
		   rawa_close,
		   rawa_codec_check,
		   NULL,
		   NULL,
		   NULL,
		   NULL,
		   NULL,
		   NULL);
/* end file rawa.cpp */



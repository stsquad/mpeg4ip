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
#define LOGIT rawa->m_vft->log_msg
/*
 * Create raw audio structure
 */
static codec_data_t *rawa_codec_create (const char *compressor, 
					int type, 
					int profile, 
					format_list_t *media_fmt,
					audio_info_t *audio,
					const uint8_t *userdata,
					uint32_t userdata_size,
					audio_vft_t *vft,
					void *ifptr)

{
  rawa_codec_t *rawa;

  rawa = (rawa_codec_t *)malloc(sizeof(rawa_codec_t));
  memset(rawa, 0, sizeof(rawa_codec_t));

  rawa->m_vft = vft;
  rawa->m_ifptr = ifptr;
  rawa->m_initialized = 0;
  rawa->m_temp_buff = NULL;
  if (media_fmt != NULL) {
    /*
     * Raw pcm - L8 or L16
     */
    rawa->m_freq = media_fmt->rtpmap->clock_rate;
    rawa->m_chans = media_fmt->rtpmap->encode_param != 0 ?
      media_fmt->rtpmap->encode_param : 1;
    if (strcasecmp(media_fmt->rtpmap->encode_name, "L16") == 0) {
      rawa->m_bitsperchan = 16;
    } else if ((*media_fmt->rtpmap->encode_name == '1') &&
	       (media_fmt->rtpmap->encode_name[1] == '0') ||
	       (media_fmt->rtpmap->encode_name[1] == '1')) {
      rawa->m_bitsperchan = 16;
      rawa->m_convert_bytes = 1;
      rawa->m_freq = 44100;
      rawa->m_chans = media_fmt->rtpmap->encode_name[1] == '0' ? 2 : 1;
    } else
      rawa->m_bitsperchan = 8;
#ifndef WORDS_BIGENDIAN
    rawa->m_convert_bytes = 1;
#endif
  } else {
#ifndef WORDS_BIGENDIAN
    if (type == MP4_PCM16_BIG_ENDIAN_AUDIO_TYPE)
      rawa->m_convert_bytes = 1;
#else
    if (type == MP4_PCM16_LITTLE_ENDIAN_AUDIO_TYPE)
      rawa->m_convert_bytes = 1;
#endif
    rawa->m_freq = audio->freq;
    rawa->m_chans = audio->chans;
    rawa->m_bitsperchan = audio->bitspersample;
  }

  LOGIT(LOG_DEBUG, "rawa", 
	"setting freq %d chans %d bitsper %d", 
	rawa->m_freq,
	rawa->m_chans, 
	rawa->m_bitsperchan);

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
  //LOGIT(LOG_DEBUG, "rawa", "do pause");
}

/*
 * Decode task call for FAAC
 */
static int rawa_decode (codec_data_t *ptr,
		       uint64_t ts,
		       int from_rtp,
		       int *sync_frame,
		       uint8_t *buffer,
		       uint32_t buflen,
			void *ud)
{
  rawa_codec_t *rawa = (rawa_codec_t *)ptr;
  uint32_t ix;
  unsigned short *b;

  //LOGIT(LOG_DEBUG, "rawa", "ts %llu buffer len %d", ts, buflen);
  if (rawa->m_initialized == 0) {
    if (rawa->m_chans == 0) {
      // Special mp4 case - we don't know how many channels, but we
      // do know that we've got 1 seconds worth of data...
      if (rawa->m_temp_buff == NULL) {
	rawa->m_temp_buff = (uint8_t *)malloc(buflen);
	memcpy(rawa->m_temp_buff, buffer, buflen);
	rawa->m_temp_buffsize = buflen;
	rawa->m_ts = ts;
	LOGIT(LOG_DEBUG, "rawaudio", "setting %d bufsize", 
	      rawa->m_temp_buffsize);
	return (buflen);
      } else {
	//
	if (buflen != rawa->m_temp_buffsize) {
	  LOGIT(LOG_ERR, "rawaudio", "Inconsistent raw audio buffer size %d should be %d", buflen, rawa->m_temp_buffsize);
	  return buflen;
	}

	uint64_t calc;

	LOGIT(LOG_DEBUG, "rawaudio",
	      "freq %d ts "LLU" buffsize %d",
	      rawa->m_freq, ts, rawa->m_temp_buffsize);
	
	calc = 1000 *  rawa->m_temp_buffsize;
	calc /= rawa->m_freq;
	calc /= (ts - rawa->m_ts);
	calc /= 2;
	if (calc == 0) calc = 1;
	LOGIT(LOG_DEBUG, "rawaudio", "Channels is %d", calc);
	rawa->m_chans = calc;
	rawa->m_bitsperchan = 16;
      } 
    }
    int audio_format;

    if (rawa->m_bitsperchan == 16) {
      audio_format = AUDIO_S16MSB;
    } else {
      audio_format = AUDIO_U8;
    }
    rawa->m_vft->audio_configure(rawa->m_ifptr,
				 rawa->m_freq, 
				 rawa->m_chans, 
				 audio_format,
				 0);
    if (rawa->m_temp_buff != NULL) {
      rawa->m_vft->audio_load_buffer(rawa->m_ifptr,
				     rawa->m_temp_buff, 
				     rawa->m_temp_buffsize,
				     0, 
				     1);
      rawa->m_resync = 0;
      if (ts == 0) rawa->m_bytes = rawa->m_temp_buffsize;
      free(rawa->m_temp_buff);
      rawa->m_temp_buff = NULL;
    }
    rawa->m_initialized = 1;
  } 

  if (ts == rawa->m_ts) {
    uint64_t calc;
    calc = rawa->m_bytes * M_LLU;
    calc /= rawa->m_chans;
    if (rawa->m_bitsperchan == 16) calc /= 2;
    calc /= rawa->m_freq;
    ts += calc;
    rawa->m_bytes += buflen;
  } else {
    rawa->m_bytes = buflen;
    rawa->m_ts = ts;
  }

  /*
   * if we're over RTP, we've got the buffer reversed...
   */
  if (rawa->m_convert_bytes) {
    for (ix = 0, b = (unsigned short *)buffer; 
	 ix < buflen; 
	 ix += 2, b++) {
      *b = ntohs(*b);
    }
  }

  rawa->m_vft->audio_load_buffer(rawa->m_ifptr, 
				 buffer, 
				 buflen,
				 ts, 
				 rawa->m_resync);
  rawa->m_resync = 0;

  return (buflen);
}


static int rawa_codec_check (lib_message_func_t message,
			    const char *compressor,
			    int type,
			    int profile,
			    format_list_t *fptr, 
			    const uint8_t *userdata,
			    uint32_t userdata_size)
{
  if (compressor != NULL && 
      strcasecmp(compressor, "MP4 FILE") == 0 &&
      type != -1) {
    if ((type == MP4_PCM16_LITTLE_ENDIAN_AUDIO_TYPE) ||
	(type == MP4_PCM16_BIG_ENDIAN_AUDIO_TYPE))
      return 1;
  }
  if (compressor != NULL &&
      strcasecmp(compressor, "AVI FILE") == 0 &&
      type == 1) {
    return 1;
  }
  if (fptr != NULL) {
    if (fptr->rtpmap != NULL && fptr->rtpmap->encode_name != NULL) {
      if (strcasecmp(fptr->rtpmap->encode_name, "L16") == 0) {
	return 1;
      }
      if (strcasecmp(fptr->rtpmap->encode_name, "L8") == 0) {
	return 1;
      }
    }
  }
  return -1;
}

AUDIO_CODEC_PLUGIN("rawa",
		   rawa_codec_create,
		   rawa_do_pause,
		   rawa_decode,
		   NULL,
		   rawa_close,
		   rawa_codec_check);
/* end file rawa.cpp */



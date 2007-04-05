
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
 * Copyright (C) Cisco Systems Inc. 2004.  All Rights Reserved.
 * 
 * Contributor(s): 
 *              Bill May        wmay@cisco.com
 */
/*
 * mpeg2 video codec with libmpeg2 library
 */
#define DECLARE_CONFIG_VARIABLES
#include "ffmpeg.h"
#include "mp4av.h"
#include "mp4av_h264.h"
#include <mpeg2t/mpeg2t_defines.h>
#include "ffmpeg_if.h"

static SConfigVariable MyConfigVariables[] = {
  CONFIG_BOOL(CONFIG_USE_FFMPEG_AUDIO, "UseFFmpegAudio", false),
};

#define ffmpeg_message (ffmpeg->m_vft->log_msg)

//#define DEBUG_FFMPEG
//#define DEBUG_FFMPEG_FRAME 1
#ifdef HAVE_AVRATIONAL
// cheap way to tell difference between ffmpeg versions
#ifndef HAVE_AMR_CODEC
#define HAVE_AMR_CODEC 1
#endif
#endif

static enum CodecID ffmpeg_find_codec (const char *stream_type,
				       const char *compressor, 
				       int type, 
				       int profile, 
				       format_list_t *fptr,
				       const uint8_t *userdata,
				       uint32_t ud_size)
{
  if (strcasecmp(stream_type, STREAM_TYPE_MP4_FILE) == 0) {
#ifdef HAVE_AMR_CODEC
    if (strcmp(compressor, "sawb") == 0) {
      return CODEC_ID_AMR_WB;
    }
    if (strcmp(compressor, "samr") == 0) {
      return CODEC_ID_AMR_NB;
    }
#endif
    if (strcmp(compressor, "ulaw") == 0) {
      return CODEC_ID_PCM_MULAW;
    }
    if (strcmp(compressor, "alaw") == 0) {
      return CODEC_ID_PCM_ALAW;
    }
    if (strcmp(compressor, "mp4a") == 0) {
      if (type == MP4_ALAW_AUDIO_TYPE) {
	return CODEC_ID_PCM_ALAW;
      } 
      if (type == MP4_ULAW_AUDIO_TYPE) {
	return CODEC_ID_PCM_MULAW;
      }
    }
    return CODEC_ID_NONE;
  }
  if (strcasecmp(stream_type, STREAM_TYPE_MPEG_FILE) == 0) {
    return CODEC_ID_NONE;
  }

  if (strcasecmp(stream_type, STREAM_TYPE_MPEG2_TRANSPORT_STREAM) == 0) {
    return CODEC_ID_NONE;
  }

  if (strcasecmp(stream_type, STREAM_TYPE_AVI_FILE) == 0) {
    return CODEC_ID_NONE;
  }
  if (strcasecmp(stream_type, "QT FILE") == 0) {
    if (strcmp(compressor, "ulaw") == 0) {
      return CODEC_ID_PCM_MULAW;
    }
    if (strcmp(compressor, "alaw") == 0) {
      return CODEC_ID_PCM_ALAW;
    }
    return CODEC_ID_NONE;
  }
  if ((strcasecmp(stream_type, STREAM_TYPE_RTP) == 0) && fptr != NULL) {
    if (strcmp(fptr->fmt, "8") == 0) {
      return CODEC_ID_PCM_ALAW;
    }
    if (strcmp(fptr->fmt, "0") == 0) {
      return CODEC_ID_PCM_MULAW;
    }
    if (fptr->rtpmap_name != NULL) {
#ifdef HAVE_AMR_CODEC
      if (strcasecmp(fptr->rtpmap_name, "AMR-WB") == 0)
	return CODEC_ID_AMR_WB;
      if (strcasecmp(fptr->rtpmap_name, "AMR") == 0) 
	return CODEC_ID_AMR_NB;
#endif
    }
    return CODEC_ID_NONE;
  }
  return CODEC_ID_NONE;
}

static codec_data_t *ffmpeg_create (const char *stream_type,
				    const char *compressor,
				    int type, 
				    int profile, 
				    format_list_t *media_fmt,
				    audio_info_t *ainfo,
				    const uint8_t *userdata,
				    uint32_t ud_size,
				    audio_vft_t *vft,
				    void *ifptr)
{
  ffmpeg_codec_t *ffmpeg;

  ffmpeg = MALLOC_STRUCTURE(ffmpeg_codec_t);
  memset(ffmpeg, 0, sizeof(*ffmpeg));

  ffmpeg->m_vft = vft;
  ffmpeg->m_ifptr = ifptr;
  avcodec_init();
  avcodec_register_all();

  ffmpeg->m_codecId = ffmpeg_find_codec(stream_type, compressor, type, 
					profile, media_fmt, userdata, ud_size);

  // must have a codecID - we checked it earlier
  ffmpeg->m_codec = avcodec_find_decoder(ffmpeg->m_codecId);
  ffmpeg->m_c = avcodec_alloc_context();

  if (ainfo != NULL) {
    ffmpeg->m_c->channels = ainfo->chans;
    ffmpeg->m_c->sample_rate = ainfo->freq;
  }
  switch (ffmpeg->m_codecId) {
#ifdef HAVE_AMR_CODEC
  case CODEC_ID_AMR_WB:
  case CODEC_ID_AMR_NB:
    ffmpeg->m_c->channels = 1;
    ffmpeg->m_c->sample_rate = ffmpeg->m_codecId == CODEC_ID_AMR_WB ? 
      16000 : 8000;
    break;
#endif
  case CODEC_ID_PCM_ALAW:
  case CODEC_ID_PCM_MULAW:
    ffmpeg->m_c->channels = 1;
    ffmpeg->m_c->sample_rate = 8000;
    break;
  default:
    break;
  }
  if (userdata) {
    ffmpeg->m_c->extradata = (typeof(ffmpeg->m_c->extradata))userdata;
    ffmpeg->m_c->extradata_size = ud_size;
  }
  ffmpeg_interface_lock();
  if (avcodec_open(ffmpeg->m_c, ffmpeg->m_codec) < 0) {
    ffmpeg_interface_unlock();
    ffmpeg_message(LOG_CRIT, "ffmpeg", "failed to open codec");
    return NULL;
  }
  ffmpeg_interface_unlock();
  ffmpeg->m_outbuf = (uint8_t *)malloc(AVCODEC_MAX_AUDIO_FRAME_SIZE);

  return ((codec_data_t *)ffmpeg);
}


static void ffmpeg_close (codec_data_t *ifptr)
{
  ffmpeg_codec_t *ffmpeg;

  ffmpeg = (ffmpeg_codec_t *)ifptr;
  if (ffmpeg->m_c != NULL) {
    ffmpeg_interface_lock();
    avcodec_close(ffmpeg->m_c);
    ffmpeg_interface_unlock();
    free(ffmpeg->m_c);
  }
  CHECK_AND_FREE(ffmpeg->m_outbuf);
  free(ffmpeg);
}


static void ffmpeg_do_pause (codec_data_t *ifptr)
{
}

static int ffmpeg_decode (codec_data_t *ptr,
			  frame_timestamp_t *pts, 
			  int from_rtp,
			  int *sync_frame,
			  uint8_t *buffer, 
			  uint32_t buflen,
			  void *ud)
{
  ffmpeg_codec_t *ffmpeg = (ffmpeg_codec_t *)ptr;
  uint32_t left = buflen;
  uint32_t used;
  int outsize;

  uint64_t ts = pts->msec_timestamp;
  uint32_t freq_ts = pts->audio_freq_timestamp;

  do {
#if HAVE_DECL_AVCODEC_DECODE_AUDIO2 != 1
    used = avcodec_decode_audio(ffmpeg->m_c, (short *)ffmpeg->m_outbuf,
				&outsize, buffer, left);
#else
    outsize = AVCODEC_MAX_AUDIO_FRAME_SIZE;
    used = avcodec_decode_audio2(ffmpeg->m_c, (int16_t *)ffmpeg->m_outbuf,
				 &outsize, buffer, left);
#endif
    if (used < 0) {
      ffmpeg_message(LOG_DEBUG, "ffmpeg", "failed to decode at "U64, 
		     ts);
      return buflen;
    }
    if (outsize > 0) {
      if (ffmpeg->m_audio_initialized == 0) {
	ffmpeg->m_vft->audio_configure(ffmpeg->m_ifptr, 
				       ffmpeg->m_c->sample_rate,
				       ffmpeg->m_c->channels,
				       AUDIO_FMT_S16,
				       0);
	ffmpeg->m_channels = ffmpeg->m_c->channels;
	ffmpeg->m_freq = ffmpeg->m_c->sample_rate;
	ffmpeg->m_audio_initialized = 1;
      }
#ifdef DEBUG_FFMPEG
      ffmpeg_message(LOG_DEBUG, "ffmpeg", "decoded %u bytes %llu", outsize, 
		     ts);
#endif
      if (pts->audio_freq != ffmpeg->m_freq) {
	freq_ts = convert_timescale(freq_ts, pts->audio_freq, ffmpeg->m_freq);
      }
      if (freq_ts == ffmpeg->m_freq_ts && ts == ffmpeg->m_ts) {
	uint64_t calc;
	freq_ts += ffmpeg->m_samples;
	calc = ffmpeg->m_samples * TO_U64(1000);
	calc /= ffmpeg->m_channels;
	calc /= 2;
	calc /= ffmpeg->m_freq;
	ts += calc;
	ffmpeg->m_samples += outsize;
      } else {
	ffmpeg->m_samples = outsize;
	ffmpeg->m_ts = ts;
	ffmpeg->m_freq_ts = freq_ts;
      }

      ffmpeg->m_vft->audio_load_buffer(ffmpeg->m_ifptr, 
				       ffmpeg->m_outbuf, 
				       outsize,
				       freq_ts,
				       ts);
    }
    left -= used;
  } while (left > 0 && used != 0);

  return (buflen);
}

static int ffmpeg_codec_check (lib_message_func_t message,
				 const char *stream_type,
			     const char *compressor,
			     int type,
			     int profile,
			     format_list_t *fptr,
			     const uint8_t *userdata,
			     uint32_t userdata_size,
			      CConfigSet *pConfig)
{
  enum CodecID fcodec;
  AVCodec *c;
  avcodec_init();
  avcodec_register_all();

  fcodec = ffmpeg_find_codec(stream_type, compressor, type, profile, 
			     fptr, userdata, userdata_size);

  if (fcodec == CODEC_ID_NONE)
    return -1;

  c = avcodec_find_decoder(fcodec);
  if (c == NULL) {
    message(LOG_ERR, "ffmpeg_audio", "codec %d found, not in library", 
	    fcodec);
    return -1;
  }
  return pConfig->GetBoolValue(CONFIG_USE_FFMPEG_AUDIO) ? 10 : 1;
}

AUDIO_CODEC_PLUGIN("ffmpeg audio", 
		   ffmpeg_create,
		   ffmpeg_do_pause,
		   ffmpeg_decode,
		   NULL,
		   ffmpeg_close,
		   ffmpeg_codec_check,
		   MyConfigVariables,
		   sizeof(MyConfigVariables) /
		   sizeof(*MyConfigVariables));

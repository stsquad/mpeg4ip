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
#define DECLARE_CONFIG_VARIABLES
#include "mp3if.h"
#include <mp4av/mp4av.h>
#include <mp4v2/mp4.h>
#include <mpeg2t/mpeg2_transport.h>
#include <mpeg2ps/mpeg2_ps.h>

static SConfigVariable MyConfigVariables[] = {
  CONFIG_BOOL(CONFIG_USE_MAD, "UseMAD", false),
};

#define mp3_message mp3->m_vft->log_msg

#define DEBUG_MAD 1
/*
 * Create CMP3Codec class
 */
static codec_data_t *mp3_codec_create (const char *stream_type,
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
  mp3_codec_t *mp3;

  mp3 = (mp3_codec_t *)malloc(sizeof(mp3_codec_t));
  if (mp3 == NULL) return NULL;
  memset(mp3, 0, sizeof(mp3_codec_t));

  mp3->m_vft = vft;
  mp3->m_ifptr = ifptr;
#ifdef OUTPUT_TO_FILE
  mp3->m_output_file = fopen("smpeg.raw", "w");
#endif

  mp3->m_record_sync_time = 1;

  mp3->m_audio_inited = 0;
  // Use media_fmt to indicate that we're streaming.
  // create a CInByteStreamMem that will be used to copy from the
  // streaming packet for use locally.  This will allow us, if we need
  // to skip, to get the next frame.
  // we really shouldn't need to set this here...
  if (media_fmt && media_fmt->rtpmap_name) 
    mp3->m_freq = media_fmt->rtpmap_clock_rate;
  else if (audio)
    mp3->m_freq = audio->freq;
  else 
    mp3->m_freq = 44100;
  return ((codec_data_t *)mp3);
}

static void mp3_close (codec_data_t *ptr)
{
  mp3_codec_t *mp3 = (mp3_codec_t *)ptr;

  mad_synth_finish(mp3->m_mad_synth);
  mad_frame_finish(mp3->m_mad_frame);
  mad_stream_finish(mp3->m_mad_stream);
  mad_decoder_finish(&mp3->m_mad_decoder);

#ifdef OUTPUT_TO_FILE
  fclose(mp3->m_output_file);
#endif
  if (mp3->m_ifile != NULL) {
    fclose(mp3->m_ifile);
    mp3->m_ifile = NULL;
  }
  if (mp3->m_buffer != NULL) {
    free(mp3->m_buffer);
    mp3->m_buffer = NULL;
  }
  if (mp3->m_fpos != NULL) {
    delete mp3->m_fpos;
    mp3->m_fpos = NULL;
  }
  free(mp3);
}

/*
 * Handle pause - basically re-init the codec
 */
static void mp3_do_pause (codec_data_t *ifptr)
{
  mp3_codec_t *mp3 = (mp3_codec_t *)ifptr;

  mp3->m_record_sync_time = 1;
  mp3->m_audio_inited = 0;
}

static enum mad_flow mad_input (void *data, struct mad_stream *stream)
{
  return MAD_FLOW_STOP;
}

static enum mad_flow mad_output (void *data, const struct mad_header *header, 
				 struct mad_pcm *pcm)
{
  return MAD_FLOW_CONTINUE;
}

static
enum mad_flow mad_error(void *data,
			struct mad_stream *stream,
			struct mad_frame *frame)
{
  return MAD_FLOW_STOP;
}

static inline
int scale(mad_fixed_t sample)
{
  /* round */
  sample += (1L << (MAD_F_FRACBITS - 16));

  /* clip */
  if (sample >= MAD_F_ONE)
    sample = MAD_F_ONE - 1;
  else if (sample < -MAD_F_ONE)
    sample = -MAD_F_ONE;

  /* quantize */
  return sample >> (MAD_F_FRACBITS + 1 - 16);
}
/*
 * Decode task call for MP3
 */
static int mp3_decode (codec_data_t *ptr,
		       frame_timestamp_t *ts, 
		       int from_rtp, 
		       int *sync_frame,
		       uint8_t *buffer,
		       uint32_t buflen,
		       void *userdata)
{
  mp3_codec_t *mp3 = (mp3_codec_t *)ptr;
  if (mp3->m_audio_inited == 0) {
    // handle initialization here...
    // Make sure that we read the header to make sure that
    // the frequency/number of channels goes through...
    const uint8_t *frame;
    uint32_t frame_size;
    if (MP4AV_Mp3GetNextFrame(buffer, 
			      buflen,
			      &frame,
			      &frame_size) == false) {
      return buflen;
    }
    
    MP4AV_Mp3Header hdr = MP4AV_Mp3HeaderFromBytes(frame);
    mp3->m_chans = MP4AV_Mp3GetChannels(hdr);
    mp3->m_freq = MP4AV_Mp3GetHdrSamplingRate(hdr);
    mp3->m_samplesperframe = 
      MP4AV_Mp3GetHdrSamplingWindow(hdr);
    mp3_message(LOG_DEBUG, "mad", "chans %d layer %d freq %d samples %d bitrate %u", 
		mp3->m_chans, 
		MP4AV_Mp3GetHdrLayer(hdr),
		mp3->m_freq, mp3->m_samplesperframe,
		MP4AV_Mp3GetBitRate(hdr));
    mp3->m_vft->audio_configure(mp3->m_ifptr,
				mp3->m_freq, 
				mp3->m_chans, 
				AUDIO_FMT_S16, 
				mp3->m_samplesperframe);
    mp3->m_audio_inited = 1;
    mp3->m_last_rtp_ts = ts->msec_timestamp - 1; // so we meet the critera below
    mad_decoder_init(&mp3->m_mad_decoder,
		     mp3,
		     mad_input,
		     NULL,
		     NULL,
		     mad_output,
		     mad_error,
		     NULL);
#ifdef DEBUG_MAD
    mp3_message(LOG_DEBUG, "mad", "init decoder");
#endif
    void **foo = (void **)&mp3->m_mad_decoder.sync;

    *foo = malloc(sizeof(*mp3->m_mad_decoder.sync));
    memset(mp3->m_mad_decoder.sync, 0, sizeof(*mp3->m_mad_decoder.sync));

    mp3->m_mad_stream = &mp3->m_mad_decoder.sync->stream;
    mp3->m_mad_frame = &mp3->m_mad_decoder.sync->frame;
    mp3->m_mad_synth = &mp3->m_mad_decoder.sync->synth;
    mad_stream_init(mp3->m_mad_stream);
    mad_frame_init(mp3->m_mad_frame);
    mad_synth_init(mp3->m_mad_synth);
    mad_stream_options(mp3->m_mad_stream, mp3->m_mad_decoder.options);
  }
      

  uint8_t *buff;
  uint32_t freq_timestamp;
  freq_timestamp = ts->audio_freq_timestamp;
  if (ts->audio_freq != mp3->m_freq) {
    freq_timestamp = convert_timescale(freq_timestamp,
				       ts->audio_freq,
				       mp3->m_freq);
  }
  if (mp3->m_last_rtp_ts == ts->msec_timestamp) {
#if 0
    mp3_message(LOG_DEBUG, "mp3",
		"ts %llu current time "U64" spf %d freq %d", 
		ts, mp3->m_current_time, mp3->m_samplesperframe, 
		mp3->m_freq);
#endif
    mp3->m_current_frame++;
    mp3->m_current_time = mp3->m_last_rtp_ts + 
      ((mp3->m_samplesperframe * mp3->m_current_frame * 1000) / mp3->m_freq);
    freq_timestamp += (mp3->m_current_frame * mp3->m_samplesperframe);
  } else {
    mp3->m_last_rtp_ts = ts->msec_timestamp;
    mp3->m_current_time = ts->msec_timestamp;
    mp3->m_current_frame = 0;
  }
  mp3->m_current_buffer = buffer;
  mp3->m_current_buflen = buflen;
  mp3->m_freq_timestamp = freq_timestamp;

  mp3->m_mad_stream->skiplen = 0;
  mad_stream_buffer(mp3->m_mad_stream, 
		    buffer, 
		    buflen + 9);
#ifdef DEBUG_MAD
  mp3_message(LOG_DEBUG, "mad", "loaded stream %p %u",buffer, buflen);
#endif

  if (mad_frame_decode(mp3->m_mad_frame, mp3->m_mad_stream) == -1) {
    mp3_message(LOG_ERR, "mp3", "mad returned -1 %d",
		mp3->m_mad_stream->error);
    return buflen;
  }
  
  mad_synth_frame(mp3->m_mad_synth, mp3->m_mad_frame);

  

  /* 
   * Get an audio buffer
   */
  buff = mp3->m_vft->audio_get_buffer(mp3->m_ifptr,
				      freq_timestamp,
				      mp3->m_current_time);
  if (buff == NULL) {
    //mp3_message(LOG_DEBUG, "mp3", "Can't get buffer in mp3 ts" U64, ts);
    return (-1);
  }

  int16_t *pcm = (int16_t *)buff;
  uint32_t samples = mp3->m_mad_synth->pcm.length;

  if (mp3->m_samplesperframe != samples) {
    mp3_message(LOG_ERR, "mp3", "mismatch in samples per frame %u vs %u %u",
		mp3->m_samplesperframe, samples, mp3->m_chans);
    return buflen;
  }

  const mad_fixed_t *left, *right;
  left = mp3->m_mad_synth->pcm.samples[0];
  right = mp3->m_mad_synth->pcm.samples[1];
  for (uint32_t ix = 0; ix < samples; ix++) {
    *pcm++ = scale(*left++);
    if (mp3->m_chans > 1) {
      *pcm++ = scale(*right++);
    }
  }
  
  mp3->m_vft->audio_filled_buffer(mp3->m_ifptr);

  return (buflen);
}

static const char *mp3_compressors[] = {
  "mp3 ",
  "mp3", 
  "ms",
  NULL
};

static int mp3_codec_check (lib_message_func_t message,
			    const char *stream_type,
			    const char *compressor,
			    int audio_type,
			    int profile,
			    format_list_t *fptr,
			    const uint8_t *userdata,
			    uint32_t userdata_size,
			    CConfigSet *pConfig)
{
#if 0
  // wmay - 1/17/05 - commented out using mad in pretty much any file
  // format - mad seems to require 1 extra frame to decode - our system
  // doesn't work like that.
  if (pConfig->GetBoolValue(CONFIG_USE_MAD) == false) {
    return -1;
  }
  if ((strcasecmp(stream_type, STREAM_TYPE_MP4_FILE) == 0) &&
      (audio_type != -1)) {
    switch (audio_type) {
    case MP4_MPEG1_AUDIO_TYPE:
    case MP4_MPEG2_AUDIO_TYPE:
      return 2;
    default:
      return -1;
    }
  }
  if ((strcasecmp(stream_type, STREAM_TYPE_AVI_FILE) == 0) &&
      (audio_type == 85)) {
    return 2;
  }
  if ((strcasecmp(stream_type, STREAM_TYPE_MPEG_FILE) == 0) &&
	(audio_type == MPEG_AUDIO_MPEG)) { // AUDIO_MPEG def from libmpeg3
      return 2;
  }

  if ((strcasecmp(stream_type, STREAM_TYPE_MPEG2_TRANSPORT_STREAM) == 0) &&
      ((audio_type == MPEG2T_ST_MPEG_AUDIO) ||
       (audio_type == MPEG2T_ST_11172_AUDIO))) {
    return 2;
  }

  if (strcasecmp(stream_type, STREAM_TYPE_RTP) == 0 &&
      fptr != NULL) {
    if (strcmp(fptr->fmt, "14") == 0) {
      return 2;
    }
    if (fptr->rtpmap_name != NULL) {
      if (strcasecmp(fptr->rtpmap_name, "MPA") == 0) {
	return 2;
      }
      if (strcasecmp(fptr->rtpmap_name, "mpa-robust") == 0) {
	return 2;
      }
    }
    return -1;
  }
  if (compressor != NULL) {
    const char **lptr = mp3_compressors;
    while (*lptr != NULL) {
      if (strcasecmp(*lptr, compressor) == 0) {
	return 2;
      }
      lptr++;
    }
  }
#endif
  return -1;
}

static int mp3_file_eof (codec_data_t *ifptr)
{
  mp3_codec_t *mp3 = (mp3_codec_t *)ifptr;

  return mp3->m_buffer_on == mp3->m_buffer_size && feof(mp3->m_ifile);
}

AUDIO_CODEC_WITH_RAW_FILE_PLUGIN("mad", 
				 mp3_codec_create,
				 mp3_do_pause,
				 mp3_decode,
				 NULL,
				 mp3_close,
				 mp3_codec_check,
				 mp3_file_check,
				 mp3_file_next_frame,
				 NULL,
				 mp3_raw_file_seek_to,
				 mp3_file_eof,
				 MyConfigVariables,
				 sizeof(MyConfigVariables) /
				 sizeof(*MyConfigVariables));

/* end file mp3.cpp */


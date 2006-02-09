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
#include "a52dec.h"
#include <mp4v2/mp4.h>
#include <mp4av/mp4av.h>
#include <mpeg2ps/mpeg2_ps.h>
#define LOGIT a52dec->m_vft->log_msg


// We now support float data passed to the audio driver
// if there is a problem, define FLOAT_TO_16, and we'll do the
// float conversion here.
//#define FLOAT_TO_16 1

#ifndef FLOAT_TO_16
#define FLOAT float
#define CONVERT(a) (a)
#define FLOAT_0 (0.0)
#define FLOAT_FORMAT AUDIO_FMT_FLOAT
#else
#define FLOAT int16_t
#define CONVERT(a) convert(*(int32_t *)&(a))
#define FLOAT_0  (0)
#define FLOAT_FORMAT AUDIO_FMT_S16
#endif

#if defined(FLOAT_TO_16) 
/*
 * lifted from a52dec output routines - license for this function is
 * GPL
 */
static inline int16_t convert (int32_t i)
{
    if (i > 0x43c07fff)
	return 32767;
    else if (i < 0x43bf8000)
	return -32768;
    else
	return i - 0x43c00000;
}
#endif

/*
 * Create ac3 audio structure
 */
static codec_data_t *a52dec_codec_create (const char *stream_type,
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
  a52dec_codec_t *a52dec;

  a52dec = (a52dec_codec_t *)malloc(sizeof(a52dec_codec_t));
  memset(a52dec, 0, sizeof(a52dec_codec_t));

  a52dec->m_vft = vft;
  a52dec->m_ifptr = ifptr;
  a52dec->m_initialized = 0;
  a52dec->m_state = a52_init(0);
#ifdef OUTPUT_TO_FILE
  a52dec->m_outfile = fopen("raw.ac3", FOPEN_WRITE_BINARY);
#endif
  return (codec_data_t *)a52dec;
}

static void a52dec_close (codec_data_t *ptr)
{
  a52dec_codec_t *a52dec = (a52dec_codec_t *)ptr;
  a52_free(a52dec->m_state);
#ifdef OUTPUT_TO_FILE
  fclose(a52dec->m_outfile);
#endif
  free(a52dec);
}

/*
 * Handle pause - basically re-init the codec
 */
static void a52dec_do_pause (codec_data_t *ifptr)
{
}

/*
 * These routines convert to the interleaved format that we want
 * to use
 */
static inline FLOAT *a52_mono (FLOAT *o, sample_t *s, bool have_lfe)
{
  sample_t *first;
  int ix;
  if (have_lfe) first = &s[256];
  else first = s;

  for (ix = 0; ix < 256; ix++) {
    FLOAT value = CONVERT(first[ix]);
    if (have_lfe) {
      *o++ = value;
      *o++ = value;
      *o++ = value;
      *o++ = value;
    }
    *o++ = value;
    if (have_lfe) {
      *o++ = CONVERT(s[ix]);
    }
  }
  return o;
}

static inline FLOAT *a52_stereo (FLOAT *o, sample_t *sample, bool have_lfe)
{
  sample_t *first;
  int ix;
  if (have_lfe) first = &sample[256];
  else first = sample;

  for (ix = 0; ix < 256; ix++) {
    FLOAT l = CONVERT(first[ix]);
    FLOAT r = CONVERT(first[ix + 256]);
    
    *o++ = l;
    *o++ = r;
    if (have_lfe) {
      *o++ = l;
      *o++ = r;
      *o++ = (l + r) / 2;
      *o++ = CONVERT(sample[ix]);
    }
  }
  return o;
}


static inline FLOAT *a52_3f (FLOAT *outptr, sample_t *sample, bool have_lfe)
{
  int ix;
  sample_t *first;
  if (have_lfe) {
    first = &sample[256];
  } else {
    first = sample;
  }
  for (ix = 0; ix < 256; ix++) {
    FLOAT l = CONVERT(first[ix]);
    FLOAT r = CONVERT(first[ix + 512]);
    *outptr++ = l;
    *outptr++ = r;
    *outptr++ = l;
    *outptr++ = r;
    *outptr++ = CONVERT(first[ix+256]);
    if (have_lfe) *outptr++ = CONVERT(sample[ix]);
  }
  return outptr;
}

static inline FLOAT *a52_2f2r (FLOAT *o, sample_t *sample, bool have_lfe)
{
  int ix;
  sample_t *first;
  if (have_lfe) first = &sample[256];
  else first = sample;

  for (ix = 0; ix < 256; ix++) {
    *o++ = CONVERT(first[ix]);
    *o++ = CONVERT(first[ix + 256]);
    *o++ = CONVERT(first[ix + 512]);
    *o++ = CONVERT(first[ix + 768]);
    if (have_lfe) {
      *o++ = FLOAT_0;
      *o++ = CONVERT(sample[ix]);
    }
  }
  return o;
}
static inline FLOAT *a52_3f2r (FLOAT *o, sample_t *sample, bool have_lfe)
{
  int ix;
  sample_t *first;
  if (have_lfe) first = &sample[256];
  else first = sample;

  for (ix = 0; ix < 256; ix++) {
    *o++ = CONVERT(first[ix]);
    if (have_lfe == false) {
      *o++ = CONVERT(first[ix + 256]);
    }
    *o++ = CONVERT(first[ix + 512]);
    *o++ = CONVERT(first[ix + 768]);
    *o++ = CONVERT(first[ix + 1024]);
    if (have_lfe) {
      *o++ = CONVERT(first[ix + 256]);
      *o++ = CONVERT(sample[ix]);
    }
  }
  return o;
}

/*
 * Decode task call for ac3
 */
static int a52dec_decode (codec_data_t *ptr,
			  frame_timestamp_t *pts,
			  int from_rtp,
			  int *sync_frame,
			  uint8_t *buffer,
			  uint32_t buflen,
			  void *ud)
{
  a52dec_codec_t *a52dec = (a52dec_codec_t *)ptr;
  sample_t level, bias;
  int flags;
  uint32_t len;
  int sample_rate;
  int bit_rate;
  uint64_t ts;
  uint32_t freq_ts;
  // we probably don't need this each time; it won't hurt to
  // check everything out
  len = a52_syncinfo(buffer, &flags, &sample_rate, &bit_rate);
  if (len > buflen || len == 0) {
    LOGIT(LOG_ERR, "a52dec", "buffer len too small %d", len);
    return buflen;
  }
#ifdef OUTPUT_TO_FILE
  fwrite(buffer, buflen, 1, a52dec->m_outfile);
#endif
  
  // these variables are needed - I'm not sure why
  level = 1;
  bias = 384;
  flags |= A52_ADJUST_LEVEL;
  // This reads the header and gets ready for the decode
  if (a52_frame(a52dec->m_state, 
		buffer, 
		&flags, 
		&level, 
		bias)) {
    LOGIT(LOG_DEBUG, "a52dec", "a52 frame did not return 0");
    return buflen;
  }

  ts = pts->msec_timestamp;
  freq_ts = pts->audio_freq_timestamp;
  if (pts->audio_freq != (uint32_t)sample_rate) {
    freq_ts = convert_timescale(freq_ts, pts->audio_freq, sample_rate);
  }

  if (a52dec->m_initialized == 0) {
    if (flags & A52_LFE) {
      a52dec->m_chans = 6;
      LOGIT(LOG_DEBUG, "a52dec", "has lfe - 6 channel");
    } else {
      switch (flags & A52_CHANNEL_MASK) {
      case A52_MONO:
	a52dec->m_chans = 1;
	break;
      case A52_CHANNEL:
      case A52_STEREO:
      case A52_DOLBY:
	a52dec->m_chans = 2;
	break;
      case A52_2F2R:
	a52dec->m_chans = 4;
	break;
      default:
	a52dec->m_chans = 5;
	break;
      }
      LOGIT(LOG_DEBUG, "a52dec", "channels are %u", a52dec->m_chans);
    }
    a52dec->m_freq = sample_rate;
    // we could probably deal with more channels here
    a52dec->m_vft->audio_configure(a52dec->m_ifptr,
				   sample_rate, 
				   a52dec->m_chans, 
				   FLOAT_FORMAT,
				   256 * 6);
    a52dec->m_initialized = 1;
    a52dec->m_last_ts = ts;
  } else {
    // make sure that we're not repeating timestamps here
    if (ts == a52dec->m_last_ts) {
      a52dec->m_frames_at_ts++;
      // there are 256 * 6 samples in each ac3 decode.
      ts += a52dec->m_frames_at_ts * 256 * 6 * 1000 / sample_rate;
      freq_ts += a52dec->m_frames_at_ts * 256 * 6;
    } else {
      a52dec->m_frames_at_ts = 0;
      a52dec->m_last_ts = ts;
    }
  }
  uint8_t *outbuf;
  // get an output buffer
  outbuf = a52dec->m_vft->audio_get_buffer(a52dec->m_ifptr,
					   freq_ts,
					   ts);
  if (outbuf == NULL) {
    return len;
  }

  FLOAT *outptr;

  outptr = (FLOAT *)outbuf;

  // Main guts of program - decode the 6 blocks.  Each block will decode
  // 256 samples.
  for (int i = 0; i < 6; i++) {
    if (a52_block(a52dec->m_state)) {
      return len;
    }
    // get the sample pointer, and convert it from float to int16_t
    sample_t *sample = a52_samples(a52dec->m_state);
    if (sample == NULL) {
      LOGIT(LOG_CRIT, "a52dec", "nullpointer from samples");
      return len;

    }
    /*
     * write the sample data to the audio ring buffer in interleaved
     * format
     */
    switch (flags & (A52_CHANNEL_MASK | A52_LFE)) {
    case A52_MONO:
      outptr = a52_mono(outptr, sample, false);
      break;
    case A52_MONO | A52_LFE:
      outptr = a52_mono(outptr, sample, true);
      break;
    case A52_STEREO:
    case A52_CHANNEL:
    case A52_DOLBY:
      outptr = a52_stereo(outptr, sample, false);
      break;
    case A52_STEREO | A52_LFE:
    case A52_CHANNEL | A52_LFE:
    case A52_DOLBY | A52_LFE:
      outptr = a52_stereo(outptr, sample, true);
      break;
    case A52_3F:
      outptr = a52_3f(outptr, sample, false);
      break;
    case A52_3F | A52_LFE:
      outptr = a52_3f(outptr, sample, true);
      break;
    case A52_2F2R:
      outptr = a52_2f2r(outptr, sample, false);
      break;
    case A52_2F2R | A52_LFE:
      outptr = a52_2f2r(outptr, sample, true);
      break;
    case A52_3F2R:
      outptr = a52_3f2r(outptr, sample, false);
      break;
    case A52_3F2R | A52_LFE:
      outptr = a52_3f2r(outptr, sample, true);
      break;
    }
  }
  
  //  LOGIT(LOG_DEBUG, "a52dec", "wrote into frame "U64" %x", ts, flags);
  a52dec->m_vft->audio_filled_buffer(a52dec->m_ifptr);

  return (len);
}


static int a52dec_codec_check (lib_message_func_t message,
			       const char *stream_type,
			       const char *compressor,
			       int type,
			       int profile,
			       format_list_t *fptr, 
			       const uint8_t *userdata,
			       uint32_t userdata_size,
			       CConfigSet *pConfig)
{
  if (strcasecmp(stream_type, STREAM_TYPE_MP4_FILE) == 0 &&
      type != -1) {
    // we don't handle this yet...
    return -1; 
  }
  if ((strcasecmp(stream_type, STREAM_TYPE_MPEG_FILE) == 0) &&
      (type == MPEG_AUDIO_AC3)) { // AUDIO_AC3 from mpeg file
    return 1;
  }
  if ((strcasecmp(stream_type, STREAM_TYPE_MPEG2_TRANSPORT_STREAM) == 0) &&
	(type == 129)) {
    return 1;
  }
						    
  return -1;
}

AUDIO_CODEC_WITH_RAW_FILE_PLUGIN("a52dec",
				 a52dec_codec_create,
				 a52dec_do_pause,
				 a52dec_decode,
				 NULL,
				 a52dec_close,
				 a52dec_codec_check,
				 ac3_file_check,
				 ac3_file_next_frame,
				 ac3_file_used_for_frame,
				 ac3_raw_file_seek_to,
				 ac3_file_eof,
				 NULL, 
				 0);
/* end file a52dec.cpp */



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
#include <SDL.h>
#define LOGIT a52dec->m_vft->log_msg

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

/*
 * lifted from a52dec output routines - license for this function is
 * GPL
 */
static void float2s16_2 (float * _f, int16_t * s16)
{
    int i;
    int32_t * f = (int32_t *) _f;

    for (i = 0; i < 256; i++) {
	s16[2*i] = convert (f[i]);
	s16[2*i+1] = convert (f[i+256]);
    }
}

static void float2s16_1 (float * _f, int16_t * s16)
{
    int i;
    int32_t * f = (int32_t *) _f;

    for (i = 0; i < 256; i++) {
	s16[2*i] = convert (f[i]);
    }
}

/*
 * Create ac3 audio structure
 */
static codec_data_t *a52dec_codec_create (const char *compressor, 
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
  a52dec_codec_t *a52dec = (a52dec_codec_t *)ifptr;
  a52dec->m_resync = 1;
  //LOGIT(LOG_DEBUG, "a52dec", "do pause");
}

/*
 * Decode task call for ac3
 */
static int a52dec_decode (codec_data_t *ptr,
			  uint64_t ts,
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
  flags = A52_STEREO | A52_ADJUST_LEVEL;
  // This reads the header and gets ready for the decode
  if (a52_frame(a52dec->m_state, 
		buffer, 
		&flags, 
		&level, 
		bias)) {
    LOGIT(LOG_DEBUG, "a52dec", "a52 frame did not return 0");
    return buflen;
  }

  if (a52dec->m_initialized == 0) {
    if (flags & (A52_CHANNEL_MASK | A52_LFE) == A52_MONO) {
      a52dec->m_chans = 1;
    } else {
      a52dec->m_chans = 2;
    }
    
    // we could probably deal with more channels here
    a52dec->m_vft->audio_configure(a52dec->m_ifptr,
				   sample_rate, 
				   a52dec->m_chans, 
				   AUDIO_S16LSB,
				   256 * 6);
    a52dec->m_initialized = 1;
    a52dec->m_last_ts = ts;
  } else {
    // make sure that we're not repeating timestamps here
    if (ts == a52dec->m_last_ts) {
      a52dec->m_frames_at_ts++;
      // there are 256 * 6 samples in each ac3 decode.
      ts += a52dec->m_frames_at_ts * 256 * 6 * 1000 / sample_rate;
    } else {
      a52dec->m_frames_at_ts = 0;
      a52dec->m_last_ts = ts;
    }
  }
  uint8_t *outbuf;
  // get an output buffer
  outbuf = a52dec->m_vft->audio_get_buffer(a52dec->m_ifptr);
  if (outbuf == NULL) {
    return buflen;
  }

  int16_t *outptr;

  outptr = (int16_t *)outbuf;

  // Main guts of program - decode the 6 blocks.  Each block will decode
  // 256 samples.
  for (int i = 0; i < 6; i++) {
    if (a52_block(a52dec->m_state)) {
      return buflen;
    }
    // get the sample pointer, and convert it from float to int16_t
    sample_t *sample = a52_samples(a52dec->m_state);
    if (sample == NULL) {
      LOGIT(LOG_CRIT, "a52dec", "nullpointer from samples");
      return buflen;

    }
    if (flags & (A52_CHANNEL_MASK | A52_LFE) == A52_MONO) {
      float2s16_1(sample, outptr);
      outptr += 256;
    } else {
      float2s16_2(sample, outptr);
      outptr += 2 * 256;
    }
  }
  //  LOGIT(LOG_DEBUG, "a52dec", "wrote into frame "U64" %x", ts, flags);
  a52dec->m_vft->audio_filled_buffer(a52dec->m_ifptr,
				     ts, 
				     a52dec->m_resync);
  a52dec->m_resync = 0;

  return (buflen);
}


static int a52dec_codec_check (lib_message_func_t message,
			       const char *compressor,
			       int type,
			       int profile,
			       format_list_t *fptr, 
			       const uint8_t *userdata,
			       uint32_t userdata_size,
			       CConfigSet *pConfig)
{
  if (compressor != NULL && 
      strcasecmp(compressor, "MP4 FILE") == 0 &&
      type != -1) {
    // we don't handle this yet...
    return -1; 
  }
  if (compressor != NULL) {
    if ((strcasecmp(compressor, "MPEG FILE") == 0) &&
	(type == 2)) { // AUDIO_AC3 from mpeg file
      return 1;
    }
    if ((strcasecmp(compressor, "MPEG2 TRANSPORT") == 0) &&
	(type == 129)) {
      return 1;
    }
						    
  }
  return -1;
}

AUDIO_CODEC_PLUGIN("a52dec",
		   a52dec_codec_create,
		   a52dec_do_pause,
		   a52dec_decode,
		   NULL,
		   a52dec_close,
		   a52dec_codec_check,
		   NULL, 
		   0);
/* end file a52dec.cpp */



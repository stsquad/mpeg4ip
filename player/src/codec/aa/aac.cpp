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
#include "aac.h"
#include <mp4util/mpeg4_audio_config.h>
#include <mp4util/mpeg4_sdp.h>
#include <mp4.h>
#include <mp4av.h>

#define DEBUG_SYNC 2

const char *aaclib="aac";

/*
 * Create CAACodec class
 */
static codec_data_t *aac_codec_create (const char *stream_type,
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
  aac_codec_t *aac;
  bool parse_streammux = false;

  aac = (aac_codec_t *)malloc(sizeof(aac_codec_t));
  memset(aac, 0, sizeof(aac_codec_t));

  aac->m_vft = vft;
  aac->m_ifptr = ifptr;
  fmtp_parse_t *fmtp = NULL;
  // Start setting up FAAC stuff...

  aac->m_resync_with_header = 1;
  aac->m_record_sync_time = 1;
  
  aac->m_faad_inited = 0;
  aac->m_audio_inited = 0;
  aac->m_temp_buff = (uint8_t *)malloc(4096);

  // Use media_fmt to indicate that we're streaming.
  if (media_fmt != NULL) {
    // haven't checked for null buffer
    // This is not necessarilly right - it is, for the most part, but
    // we should be reading the fmtp statement, and looking at the config.
    // (like we do below in the userdata section...
    aac->m_freq = media_fmt->rtpmap_clock_rate;
    if (strcasecmp(media_fmt->rtpmap_name, "mp4a-latm") == 0) {
      fmtp = parse_fmtp_for_rfc3016(media_fmt->fmt_param, vft->log_msg);
      parse_streammux = true;
    } else {
      fmtp = parse_fmtp_for_mpeg4(media_fmt->fmt_param, vft->log_msg);
    }
    if (fmtp != NULL) {
      userdata = fmtp->config_binary;
      userdata_size = fmtp->config_binary_len;
    }
  } else {
    if (audio != NULL) {
      aac->m_freq = audio->freq;
    } else {
      aac->m_freq = 44100;
    }
  }
  aac->m_chans = 2; // this may be wrong - the isma spec, Appendix A.1.1 of
  // Appendix H says the default is 1 channel...
  aac->m_output_frame_size = 1024;
  aac->m_object_type = AACMAIN;
  if (userdata != NULL || fmtp != NULL) {
    mpeg4_audio_config_t audio_config;
    decode_mpeg4_audio_config(userdata, userdata_size, &audio_config, parse_streammux);
    aac->m_object_type = audio_config.audio_object_type;
    aac->m_freq = audio_config.frequency;
    aac->m_chans = audio_config.channels;
    if (audio_config.codec.aac.frame_len_1024 == 0) {
      aac->m_output_frame_size = 960;
    }
  }

  aa_message(LOG_INFO, aaclib,"AAC object type is %d %u", aac->m_object_type,
	     aac->m_output_frame_size);
  aac->m_info = faacDecOpen();
  faacDecConfiguration config;
  config.defObjectType = aac->m_object_type;
  config.defSampleRate = aac->m_freq;
  faacDecSetConfiguration(aac->m_info, &config);
  aac->m_msec_per_frame = aac->m_output_frame_size;
  aac->m_msec_per_frame *= TO_U64(1000);
  aac->m_msec_per_frame /= aac->m_freq;

  //  faad_init_bytestream(&m_info->ld, c_read_byte, c_bookmark, m_bytestream);

  aa_message(LOG_INFO, aaclib, "Setting freq to %d", aac->m_freq);
#if DUMP_OUTPUT_TO_FILE
  aac->m_outfile = fopen("temp.raw", "w");
#endif
  if (fmtp != NULL) {
    free_fmtp_parse(fmtp);
  }
  return (codec_data_t *)aac;
}

void aac_close (codec_data_t *ptr)
{
  if (ptr == NULL) {
    return;
  }
  aac_codec_t *aac = (aac_codec_t *)ptr;
  faacDecClose(aac->m_info);
  aac->m_info = NULL;

  CHECK_AND_FREE(aac->m_temp_buff);
  CHECK_AND_FREE(aac->m_buffer);
  if (aac->m_ifile != NULL) {
    fclose(aac->m_ifile);
    aac->m_ifile = NULL;
  }
#if DUMP_OUTPUT_TO_FILE
  fclose(aac->m_outfile);
#endif
  free(aac);
}

/*
 * Handle pause - basically re-init the codec
 */
static void aac_do_pause (codec_data_t *ifptr)
{
  aac_codec_t *aac = (aac_codec_t *)ifptr;
  aac->m_resync_with_header = 1;
  aac->m_record_sync_time = 1;
  aac->m_audio_inited = 0;
  aac->m_faad_inited = 0;
  if (aac->m_temp_buff == NULL) 
    aac->m_temp_buff = (uint8_t *)malloc(4096);
}

/*
 * Decode task call for FAAC
 */
static int aac_decode (codec_data_t *ptr,
		       frame_timestamp_t *ts,
		       int from_rtp,
		       int *sync_frame,
		       uint8_t *buffer,
		       uint32_t buflen, 
		       void *userdata)
{
  aac_codec_t *aac = (aac_codec_t *)ptr;
  unsigned long bytes_consummed;
  int bits = -1;
  //  struct timezone tz;
  uint32_t freq_timestamp;

  freq_timestamp = ts->audio_freq_timestamp;
  if (ts->audio_freq != aac->m_freq) {
    freq_timestamp = convert_timescale(freq_timestamp,
				       ts->audio_freq,
				       aac->m_freq);
  }
  if (aac->m_record_sync_time) {
    aac->m_current_frame = 0;
    aac->m_record_sync_time = 0;
    aac->m_current_time = ts->msec_timestamp;
    aac->m_last_rtp_ts = ts->msec_timestamp;
  } else {
    if (aac->m_last_rtp_ts == ts->msec_timestamp) {
      aac->m_current_frame++;
      aac->m_current_time = aac->m_last_rtp_ts;
      aac->m_current_time += 
	aac->m_output_frame_size * aac->m_current_frame * 
	TO_U64(1000) / aac->m_freq;
      freq_timestamp += aac->m_output_frame_size * aac->m_current_frame;
    } else {
      aac->m_last_rtp_ts = ts->msec_timestamp;
      aac->m_current_time = ts->msec_timestamp;
      aac->m_current_frame = 0;
    }

    // Note - here m_current_time should pretty much always be >= rtpts.  
    // If we're not, we most likely want to stop and resync.  We don't
    // need to keep decoding - just decode this frame and indicate we
    // need a resync... That should handle fast forwards...  We need
    // someway to handle reverses - perhaps if we're more than .5 seconds
    // later...
  }

    if (aac->m_faad_inited == 0) {
      /*
       * If not initialized, do so.  
     */
      unsigned long freq, chans;

      faacDecInit(aac->m_info,
		  (unsigned char *)buffer,
		  &freq,
		  &chans);
      aac->m_freq = freq;
      aac->m_chans = chans;
      aac->m_faad_inited = 1;
    }

    uint8_t *buff;

    /* 
     * Get an audio buffer
     */
    if (aac->m_audio_inited == 0) {
      buff = aac->m_temp_buff;
    } else {
      buff = aac->m_vft->audio_get_buffer(aac->m_ifptr,
					  freq_timestamp,
					  aac->m_current_time);
    }
    if (buff == NULL) {
      //player_debug_message("Can't get buffer in aa");
      return (0);
    }

    unsigned long samples;
    bytes_consummed = buflen;
    //aa_message(LOG_DEBUG, aaclib, "decoding %d bits", buflen * 8);
    bits = faacDecDecode(aac->m_info,
			 (unsigned char *)buffer, 
			 &bytes_consummed,
			 (short *)buff, 
			 &samples);
    switch (bits) {
    case FAAD_OK_CHUPDATE:
      if (aac->m_audio_inited != 0) {
	int tempchans = faacDecGetProgConfig(aac->m_info, NULL);
	if (tempchans != aac->m_chans) {
	  aa_message(LOG_NOTICE, aaclib, "chupdate - chans from data is %d", 
			       tempchans);
	}
      }
      // fall through...
    case FAAD_OK:
      if (aac->m_audio_inited == 0) {
	int tempchans = faacDecGetProgConfig(aac->m_info, NULL);
	if (tempchans == 0) {
	  aac->m_resync_with_header = 1;
	  aac->m_record_sync_time = 1;
	  return bytes_consummed;
	}
	if (tempchans != aac->m_chans) {
	  aa_message(LOG_NOTICE, aaclib, "chans from data is %d conf %d", 
		     tempchans, aac->m_chans);
	  aac->m_chans = tempchans;
	}
	aac->m_vft->audio_configure(aac->m_ifptr,
				     aac->m_freq, 
				     aac->m_chans, 
				     AUDIO_FMT_S16, 
				     aac->m_output_frame_size);
	uint8_t *now = aac->m_vft->audio_get_buffer(aac->m_ifptr,
						    freq_timestamp,
						    aac->m_current_time);

	if (now != NULL) {
	  memcpy(now, buff, tempchans * aac->m_output_frame_size * sizeof(int16_t));
	}
	aac->m_audio_inited = 1;
      }
      /*
       * good result - give it to audio sync class
       */
#if DUMP_OUTPUT_TO_FILE
      fwrite(buff, aac->m_output_frame_size * 4, 1, aac->m_outfile);
#endif
      aac->m_vft->audio_filled_buffer(aac->m_ifptr);
      if (aac->m_resync_with_header == 1) {
	aac->m_resync_with_header = 0;
#ifdef DEBUG_SYNC
	aa_message(LOG_DEBUG, aaclib, "Back to good at "U64, aac->m_current_time);
#endif
      }
      break;
    default:
      aa_message(LOG_ERR, aaclib, "Bits return is %d", bits);
      aac->m_resync_with_header = 1;
#ifdef DEBUG_SYNC
      aa_message(LOG_ERR, aaclib, "Audio decode problem - at "U64, 
		 aac->m_current_time);
#endif
      break;
    }
  return (bytes_consummed);
}

static const char *aac_compressors[] = {
  "aac ",
  "mp4a",
  "enca",
  NULL
};

static int aac_codec_check (lib_message_func_t message,
			    const char *stream_type,
			    const char *compressor,
			    int type,
			    int profile,
			    format_list_t *fptr, 
			    const uint8_t *userdata,
			    uint32_t userdata_size,
			    CConfigSet *pConfig)
{
  fmtp_parse_t *fmtp = NULL;
  bool use_streammux = false;
  if (compressor != NULL && 
      strcasecmp(stream_type, "MP4 FILE") == 0 &&
      type != -1) {
    switch (type) {
    case MP4_MPEG2_AAC_MAIN_AUDIO_TYPE:
    case MP4_MPEG2_AAC_LC_AUDIO_TYPE:
    case MP4_MPEG2_AAC_SSR_AUDIO_TYPE:
    case MP4_MPEG4_AUDIO_TYPE:
    case MP4_INVALID_AUDIO_TYPE:
      break;
    default:
      return -1;
    }
  }
  if (strcasecmp(stream_type, STREAM_TYPE_RTP) == 0 &&
      fptr != NULL && 
      fptr->rtpmap_name != NULL) {
    if (strcasecmp(fptr->rtpmap_name, "mp4a-latm") == 0) {
      fmtp = parse_fmtp_for_rfc3016(fptr->fmt_param, message);
      if (fmtp == NULL) {
	return -1;
      }
      if (fmtp->cpresent != 0) {
	return -1;
      }
      userdata = fmtp->config_binary;
      userdata_size = fmtp->config_binary_len;
      use_streammux = true;
    } else {
      if ((strcasecmp(fptr->rtpmap_name, "mpeg4-generic") != 0) &&
	  (strcasecmp(fptr->rtpmap_name, "enc-mpeg4-generic") != 0)) {
	return -1;
      }
      if (userdata == NULL) {
	fmtp = parse_fmtp_for_mpeg4(fptr->fmt_param, message);
	if (fmtp != NULL) {
	  userdata = fmtp->config_binary;
	  userdata_size = fmtp->config_binary_len;
	}
      }
    }
  }
  if (userdata != NULL) {
    mpeg4_audio_config_t audio_config;
    decode_mpeg4_audio_config(userdata, userdata_size, &audio_config, use_streammux);
    //    message(LOG_DEBUG, "aac", "audio type is %d", audio_config.audio_object_type);
    if (fmtp != NULL) free_fmtp_parse(fmtp);
    
    if (audio_object_type_is_aac(&audio_config) == 0) {
      return -1;
    }
    if (audio_config.audio_object_type == 17) {
      message(LOG_INFO, "aac", "audio type is legal ISMA, but not supported");
      return -1;
    }
    return 1;
  }
  if (compressor != NULL) {
    const char **lptr = aac_compressors;
    while (*lptr != NULL) {
      if (strcasecmp(*lptr, compressor) == 0) {
	return 1;
      }
      lptr++;
    }
  }
  return -1;
}

AUDIO_CODEC_WITH_RAW_FILE_PLUGIN("aac",
				 aac_codec_create,
				 aac_do_pause,
				 aac_decode,
				 NULL, 
				 aac_close,
				 aac_codec_check,
				 aac_file_check,
				 aac_file_next_frame,
				 aac_file_used_for_frame,
				 aac_raw_file_seek_to,
				 aac_file_eof,
				 NULL,
				 0);
/* end file aa.cpp */



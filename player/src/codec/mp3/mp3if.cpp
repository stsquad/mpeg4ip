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
#include "mp3if.h"
#include <mp4av/mp4av.h>
#include <mp4v2/mp4.h>
#include <mpeg2t/mpeg2_transport.h>

#define mp3_message mp3->m_vft->log_msg
#define DEBUG_SYNC 1

/*
 * Create CMP3Codec class
 */
static codec_data_t *mp3_codec_create (const char *compressor, 
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
  mp3->m_mp3_info = new MPEGaudio();

  mp3->m_resync_with_header = 1;
  mp3->m_record_sync_time = 1;

  mp3->m_audio_inited = 0;
  // Use media_fmt to indicate that we're streaming.
  // create a CInByteStreamMem that will be used to copy from the
  // streaming packet for use locally.  This will allow us, if we need
  // to skip, to get the next frame.
  // we really shouldn't need to set this here...
  if (media_fmt && media_fmt->rtpmap) 
    mp3->m_freq = media_fmt->rtpmap->clock_rate;
  else if (audio)
    mp3->m_freq = audio->freq;
  else 
    mp3->m_freq = 44100;
  return ((codec_data_t *)mp3);
}

static void mp3_close (codec_data_t *ptr)
{
  mp3_codec_t *mp3 = (mp3_codec_t *)ptr;

  if (mp3->m_mp3_info) {
    delete mp3->m_mp3_info;
    mp3->m_mp3_info = NULL;
  }
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

  mp3->m_resync_with_header = 1;
  mp3->m_record_sync_time = 1;
  mp3->m_audio_inited = 0;
}

/*
 * Decode task call for MP3
 */
static int mp3_decode (codec_data_t *ptr,
		       uint64_t ts, 
		       int from_rtp, 
		       int *sync_frame,
		       uint8_t *buffer,
		       uint32_t buflen,
		       void *userdata)
{
  int bits = -1;
  mp3_codec_t *mp3 = (mp3_codec_t *)ptr;
  if (mp3->m_audio_inited == 0) {
    // handle initialization here...
    // Make sure that we read the header to make sure that
    // the frequency/number of channels goes through...
    bits = mp3->m_mp3_info->findheader(buffer, buflen);
    if (bits < 0) {
      mp3_message(LOG_DEBUG, "libmp3", "Couldn't load mp3 header");
      return (-1);
    }
      
    buffer += bits;

    mp3->m_chans = mp3->m_mp3_info->isstereo() ? 2 : 1;
    mp3->m_freq = mp3->m_mp3_info->getfrequency();
    
    MP4AV_Mp3Header hdr = MP4AV_Mp3HeaderFromBytes(buffer);
    mp3->m_samplesperframe = 
      MP4AV_Mp3GetHdrSamplingWindow(hdr);
    mp3_message(LOG_DEBUG, "libmp3", "chans %d layer %d freq %d samples %d bitrate %u", 
		mp3->m_chans, 
		MP4AV_Mp3GetHdrLayer(hdr),
		mp3->m_freq, mp3->m_samplesperframe,
		MP4AV_Mp3GetBitRate(hdr));
    mp3->m_vft->audio_configure(mp3->m_ifptr,
				mp3->m_freq, 
				mp3->m_chans, 
				AUDIO_S16SYS, 
				mp3->m_samplesperframe);
    mp3->m_audio_inited = 1;
    mp3->m_last_rtp_ts = ts - 1; // so we meet the critera below
  }
      

  uint8_t *buff;

    /* 
     * Get an audio buffer
     */
  buff = mp3->m_vft->audio_get_buffer(mp3->m_ifptr);
  if (buff == NULL) {
    //mp3_message(LOG_DEBUG, "mp3", "Can't get buffer in mp3 ts" U64, ts);
    return (-1);
  }
  bits = mp3->m_mp3_info->decodeFrame(buff, buffer, buflen);

  if (bits > 4) {
    if (mp3->m_last_rtp_ts == ts) {
#if 0
      mp3_message(LOG_DEBUG, "mp3",
		  "ts %llu current time %llu spf %d freq %d", 
		  ts, mp3->m_current_time, mp3->m_samplesperframe, 
		  mp3->m_freq);
#endif
      mp3->m_current_time += ((mp3->m_samplesperframe * 1000) / mp3->m_freq);
    } else {
      mp3->m_last_rtp_ts = ts;
      mp3->m_current_time = ts;
    }
#ifdef OUTPUT_TO_FILE
    fwrite(buff, mp3->m_chans * mp3->m_samplesperframe * sizeof(ushort), 1, mp3->m_output_file);
#endif
    /*
     * good result - give it to audio sync class
     * May want to check frequency, number of channels here...
     */
    mp3->m_vft->audio_filled_buffer(mp3->m_ifptr, 
				    mp3->m_current_time, 
				    mp3->m_resync_with_header);
    if (mp3->m_resync_with_header == 1) {
      mp3->m_resync_with_header = 0;
#ifdef DEBUG_SYNC
      mp3_message(LOG_DEBUG, "libmp3", 
		  "Back to good at %llu", mp3->m_current_time);
#endif
    }
      
  } else {
    mp3->m_resync_with_header = 1;
    mp3_message(LOG_DEBUG, "libmp3", "decode problem %d - at "U64, 
		bits, mp3->m_current_time);
    bits = -1;
  }
  return (bits);
}

static const char *mp3_compressors[] = {
  "mp3 ",
  "mp3", 
  "ms",
  NULL
};

static int mp3_codec_check (lib_message_func_t message,
			    const char *compressor,
			    int audio_type,
			    int profile,
			    format_list_t *fptr,
			    const uint8_t *userdata,
			    uint32_t userdata_size,
			    CConfigSet *pConfig)
{
  if (compressor != NULL && 
      (strcasecmp(compressor, "MP4 FILE") == 0) &&
      (audio_type != -1)) {
    switch (audio_type) {
    case MP4_MPEG1_AUDIO_TYPE:
    case MP4_MPEG2_AUDIO_TYPE:
      return 1;
    default:
      return -1;
    }
  }
  if (compressor != NULL) {
    if ((strcasecmp(compressor, "AVI FILE") == 0) &&
	(audio_type == 85)) {
      return 1;
    }
    if ((strcasecmp(compressor, "MPEG FILE") == 0) &&
	(audio_type == 1)) { // AUDIO_MPEG def from libmpeg3
      return 1;
    }
    if ((strcasecmp(compressor, "MPEG2 TRANSPORT") == 0) &&
	((audio_type == MPEG2T_ST_MPEG_AUDIO) ||
	 (audio_type == MPEG2T_ST_11172_AUDIO))) {
      return 1;
    }
  }

  if (fptr != NULL) {
    if (strcmp(fptr->fmt, "14") == 0) {
      return 1;
    }
    if (fptr->rtpmap != NULL && fptr->rtpmap->encode_name != NULL) {
      if (strcasecmp(fptr->rtpmap->encode_name, "MPA") == 0) {
	return 1;
      }
      if (strcasecmp(fptr->rtpmap->encode_name, "mpa-robust") == 0) {
	return 1;
      }
    }
    return -1;
  }
  if (compressor != NULL) {
    const char **lptr = mp3_compressors;
    while (*lptr != NULL) {
      if (strcasecmp(*lptr, compressor) == 0) {
	return 1;
      }
      lptr++;
    }
  }
  return -1;
}

static int mp3_file_eof (codec_data_t *ifptr)
{
  mp3_codec_t *mp3 = (mp3_codec_t *)ifptr;

  return mp3->m_buffer_on == mp3->m_buffer_size && feof(mp3->m_ifile);
}

AUDIO_CODEC_WITH_RAW_FILE_PLUGIN("mp3", 
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
				 NULL,
				 0);

/* end file mp3.cpp */


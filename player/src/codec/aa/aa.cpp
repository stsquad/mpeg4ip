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
#include "aa.h"
#include "mpeg4_audio_config.h"
#include "player_sdp.h"
#define DEBUG_SYNC 2
/*
 * C interfaces for faac callbacks
 */
static unsigned int c_read_byte (void *ud)
{
  CInByteStreamBase *byte_stream;

  byte_stream = (CInByteStreamBase *)ud;
  return (byte_stream->get());
}

static void c_bookmark (void *ud, int state)
{
  CInByteStreamBase *byte_stream;

  byte_stream = (CInByteStreamBase *)ud;
  byte_stream->bookmark(state);
}
/*
 * Create CAACodec class
 */
CAACodec::CAACodec (CAudioSync *a, 
		    CInByteStreamBase *pbytestrm,
		    format_list_t *media_fmt,
		    audio_info_t *audio,
		    const unsigned char *userdata,
		    uint32_t userdata_size) : 
  CAudioCodecBase(a, pbytestrm, media_fmt, audio, userdata, userdata_size)
{
  fmtp_parse_t *fmtp = NULL;
  // Start setting up FAAC stuff...
  m_bytestream = pbytestrm;

  m_resync_with_header = 1;
  m_record_sync_time = 1;
  
  m_faad_inited = 0;
  m_audio_inited = 0;
  m_temp_buff = (unsigned char *)malloc(4096);
  // Use media_fmt to indicate that we're streaming.
  if (media_fmt != NULL) {
    // haven't checked for null buffer
    // This is not necessarilly right - it is, for the most part, but
    // we should be reading the fmtp statement, and looking at the config.
    // (like we do below in the userdata section...
    m_freq = media_fmt->rtpmap->clock_rate;
    fmtp = parse_fmtp_for_mpeg4(media_fmt->fmt_param);
    if (fmtp != NULL) {
      userdata = fmtp->config_binary;
      userdata_size = fmtp->config_binary_len;
    }
  } else {
    if (audio != NULL) {
      m_freq = audio->freq;
    } else {
      m_freq = 44100;
    }
  }
  m_chans = 2; // this may be wrong - the isma spec, Appendix A.1.1 of
  m_output_frame_size = 1024;
  m_object_type = AACMAIN;
  // Appendix H says the default is 1 channel...
  if (userdata != NULL || fmtp != NULL) {
    mpeg4_audio_config_t audio_config;
    decode_mpeg4_audio_config(userdata, userdata_size, &audio_config);
    m_object_type = audio_config.audio_object_type;
    m_freq = audio_config.frequency;
    m_chans = audio_config.channels;
    if (audio_config.codec.aac.frame_len_1024 == 0) {
      m_output_frame_size = 960;
    }
  }

  player_debug_message("AAC object type is %d", m_object_type);
  m_info = faacDecOpen(m_object_type, m_freq);
  m_msec_per_frame = m_output_frame_size;
  m_msec_per_frame *= M_LLU;
  m_msec_per_frame /= m_freq;

  faad_init_bytestream(&m_info->ld, c_read_byte, c_bookmark, m_bytestream);

  player_debug_message("Setting freq to %d", m_freq);
#if DUMP_OUTPUT_TO_FILE
  m_outfile = fopen("temp.raw", "w");
#endif
  if (fmtp != NULL) {
    free_fmtp_parse(fmtp);
  }
}

CAACodec::~CAACodec()
{
  faacDecClose(m_info);
  m_info = NULL;

  if (m_temp_buff) {
    free(m_temp_buff);
    m_temp_buff = NULL;
  }
#if DUMP_OUTPUT_TO_FILE
  fclose(m_outfile);
#endif
}

/*
 * Handle pause - basically re-init the codec
 */
void CAACodec::do_pause (void)
{
  m_resync_with_header = 1;
  m_record_sync_time = 1;
  m_audio_inited = 0;
  m_faad_inited = 0;
  if (m_temp_buff == NULL) 
    m_temp_buff = (unsigned char *)malloc(4096);
  //    player_debug_message("AA got do pause");
}

/*
 * Decode task call for FAAC
 */
int CAACodec::decode (uint64_t rtpts, int from_rtp)
{
  int bits = -1;
  //  struct timezone tz;

  if (m_record_sync_time) {
    m_current_frame = 0;
    m_record_sync_time = 0;
    m_current_time = rtpts;
    m_last_rtp_ts = rtpts;
  } else {
    if (m_last_rtp_ts == rtpts) {
      m_current_time += m_msec_per_frame;
      m_current_frame++;
    } else {
      m_last_rtp_ts = rtpts;
      m_current_time = rtpts;
      m_current_frame = 0;
    }

    // Note - here m_current_time should pretty much always be >= rtpts.  
    // If we're not, we most likely want to stop and resync.  We don't
    // need to keep decoding - just decode this frame and indicate we
    // need a resync... That should handle fast forwards...  We need
    // someway to handle reverses - perhaps if we're more than .5 seconds
    // later...
  }
  // player_debug_message("AA at " LLD, m_current_time);

  if (m_faad_inited == 0) {
    /*
     * If not initialized, do so.  
     */
    unsigned long freq, chans;

    faacDecInit(m_info,
		NULL,
		&freq,
		&chans);
    m_freq = freq;
    m_chans = chans;
    m_faad_inited = 1;
  }

  try {
    unsigned char *buff;

    /* 
     * Get an audio buffer
     */
    if (m_audio_inited == 0) {
      buff = m_temp_buff;
    } else {
      buff = m_audio_sync->get_audio_buffer();
    }
    if (buff == NULL) {
      //player_debug_message("Can't get buffer in aa");
      return (-1);
    }

    unsigned long bytes_consummed;
    bits = faacDecDecode(m_info,
			 NULL,
			 &bytes_consummed,
			 (short *)buff);
    switch (bits) {
    case FAAD_OK_CHUPDATE:
      if (m_audio_inited != 0) {
	int tempchans = faacDecGetProgConfig(m_info, NULL);
	if (tempchans != m_chans) {
	  player_debug_message("AA-chupdate - chans from data is %d", 
			       tempchans);
	}
      }
      // fall through...
    case FAAD_OK:
      if (m_audio_inited == 0) {
	int tempchans = faacDecGetProgConfig(m_info, NULL);
	if (tempchans == 0) {
	  m_resync_with_header = 1;
	  m_record_sync_time = 1;
	  return -1;
	}
	if (tempchans != m_chans) {
	  player_debug_message("AA - chans from data is %d conf %d", 
			       tempchans, m_chans);
	  m_chans = tempchans;
	}
	m_audio_sync->set_config(m_freq, m_chans, AUDIO_S16SYS, m_output_frame_size);
	unsigned char *now = m_audio_sync->get_audio_buffer();
	if (now != NULL) {
	  memcpy(now, buff, tempchans * m_output_frame_size * sizeof(int16_t));
	}
	free(m_temp_buff);
	m_temp_buff = NULL;
	m_audio_inited = 1;
      }
      /*
       * good result - give it to audio sync class
       */
#if DUMP_OUTPUT_TO_FILE
      fwrite(buff, m_output_frame_size * 4, 1, m_outfile);
#endif
      m_audio_sync->filled_audio_buffer(m_current_time, 
					m_resync_with_header);
      if (m_resync_with_header == 1) {
	m_resync_with_header = 0;
#ifdef DEBUG_SYNC
	player_debug_message("AA - Back to good at "LLU, m_current_time);
#endif
      }
      break;
    default:
      player_debug_message("Bits return is %d", bits);
      m_resync_with_header = 1;
#ifdef DEBUG_SYNC
      player_debug_message("Audio decode problem - at "LLU, 
			   m_current_time);
#endif
      break;
    }
  } catch (int err) {
#ifdef DEBUG_SYNC
    player_error_message("aa Got exception %s at "LLU, 
			 m_bytestream->get_throw_error(err), 
			 m_current_time);
#endif
    m_resync_with_header = 1;
    m_record_sync_time = 1;
  } catch (...) {
    m_resync_with_header = 1;
    m_record_sync_time = 1;
    return (bits);
  }
  return (bits);
}

int CAACodec::skip_frame (uint64_t rtpts)
{
  // will want to do a bit more - especially if we can get the
  // header.
  return (decode(rtpts, 0));
}

/* end file aa.cpp */


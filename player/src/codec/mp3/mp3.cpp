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
#include "mp3.h"

static unsigned char c_byte_get (void *ud)
{
  CInByteStreamBase *bs = (CInByteStreamBase *)ud;
  return bs->get();
}

static uint32_t c_byte_read (unsigned char *buffer, uint32_t readbytes, void *ud)
{
  CInByteStreamBase *bs = (CInByteStreamBase *)ud;
  return bs->read(buffer, readbytes);
}
/*
 * Create CMP3Codec class
 */
CMP3Codec::CMP3Codec (CAudioSync *a, 
		      CInByteStreamBase *pbytestrm,
		      format_list_t *media_fmt,
		      audio_info_t *audio,
		      const unsigned char *userdata,
		      uint32_t userdata_size) : 
  CAudioCodecBase(a, pbytestrm, media_fmt, audio, userdata, userdata_size)
{
  m_bytestream = pbytestrm;
#ifdef OUTPUT_TO_FILE
  m_output_file = fopen("smpeg.raw", "w");
#endif
  m_mp3_info = new MPEGaudio(c_byte_get, c_byte_read, m_bytestream);

  m_resync_with_header = 1;
  m_record_sync_time = 1;

  m_audio_inited = 0;
  // Use media_fmt to indicate that we're streaming.
  // create a CInByteStreamMem that will be used to copy from the
  // streaming packet for use locally.  This will allow us, if we need
  // to skip, to get the next frame.
  if (media_fmt && media_fmt->rtpmap) 
    m_freq = media_fmt->rtpmap->clock_rate;
  else if (audio)
    m_freq = audio->freq;
  else 
    m_freq = 44100;

  player_debug_message("Setting freq to %d", m_freq);
}

CMP3Codec::~CMP3Codec()
{
  delete m_mp3_info;
  m_mp3_info = NULL;
#ifdef OUTPUT_TO_FILE
  fclose(m_output_file);
#endif
}

/*
 * Handle pause - basically re-init the codec
 */
void CMP3Codec::do_pause (void)
{
  m_resync_with_header = 1;
  m_record_sync_time = 1;
  m_audio_inited = 0;
}

/*
 * Decode task call for MP3
 */
int CMP3Codec::decode (uint64_t rtpts, int from_rtp)
{
  int bits = -1;
  //  struct timezone tz;

  //player_debug_message("MP3 at %lld", m_current_time);


  try {
    int have_head = 0;

    if (m_audio_inited == 0) {
      // handle initialization here...
      // Make sure that we read the header to make sure that
      // the frequency/number of channels goes through...
      if (from_rtp) {
	CRtpByteStreamBase *rtp = (CRtpByteStreamBase *)m_bytestream;
	rtp->set_skip_on_advance(4);
	rtp->get();
	rtp->get();
	rtp->get();
	rtp->get();  // get the 4 bytes...
      }

      bits = m_mp3_info->loadheader();
      if (bits == false) {
	return (-1);
      }

      m_chans = m_mp3_info->isstereo() ? 2 : 1;
      m_freq = m_mp3_info->getfrequency();
      int samplesperframe;
      samplesperframe = 32;
      if (m_mp3_info->getlayer() == 3) {
	samplesperframe *= 18;
	if (m_mp3_info->getversion() == 0) {
	  samplesperframe *= 2;
	}
      } else {
	samplesperframe *= SCALEBLOCK;
	if (m_mp3_info->getlayer() == 2) {
	  samplesperframe *= 3;
	}
      }
      m_samplesperframe = samplesperframe;
      player_debug_message("chans %d freq %d samples %d", 
			   m_chans, m_freq, samplesperframe);
      m_audio_sync->set_config(m_freq, m_chans, AUDIO_S16SYS, samplesperframe);
      have_head = 1;
      m_audio_inited = 1;
      m_last_rtp_ts = rtpts - 1; // so we meet the critera below
    }
      

    unsigned char *buff;

    /* 
     * Get an audio buffer
     */
    buff = m_audio_sync->get_audio_buffer();
    if (buff == NULL) {
      //player_debug_message("Can't get buffer in aa");
      return (-1);
    }
    bits = m_mp3_info->decodeFrame(buff, have_head);
    
    if (bits) {
      if (m_last_rtp_ts == rtpts) {
	m_current_time += ((m_samplesperframe * 1000) / m_freq);
      } else {
	m_last_rtp_ts = rtpts;
	m_current_time = rtpts;
      }
#ifdef OUTPUT_TO_FILE
      fwrite(buff, m_chans * m_samplesperframe * sizeof(ushort), 1, m_output_file);
#endif
      /*
       * good result - give it to audio sync class
       * May want to check frequency, number of channels here...
       */
      m_audio_sync->filled_audio_buffer(m_current_time, 
					m_resync_with_header);
      if (m_resync_with_header == 1) {
	m_resync_with_header = 0;
#ifdef DEBUG_SYNC
	player_debug_message("Back to good at %llu", m_current_time);
#endif
      }
      
    } else {
      player_debug_message("Bits return is %d", bits);
      m_resync_with_header = 1;
#ifdef DEBUG_SYNC
      player_debug_message("Audio decode problem - at %llu", 
			   m_current_time);
#endif
    }
  } catch (const char *err) {
#ifdef DEBUG_SYNC
    player_error_message("Got exception %s at %llu", err, m_current_time);
#endif
    m_resync_with_header = 1;
    m_record_sync_time = 1;
  }
  return (bits);
}

int CMP3Codec::skip_frame (uint64_t ts)
{
  m_mp3_info->loadheader();
  if (m_last_rtp_ts == ts) {
    m_current_time += ((m_samplesperframe * 1000) / m_freq);
  } else {
    m_last_rtp_ts = ts;
    m_current_time = ts;
  }
  return (0);
}
/* end file mp3.cpp */


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
 * Copyright (C) Cisco Systems Inc. 2001.  All Rights Reserved.
 * 
 * Contributor(s): 
 *              Bill May        wmay@cisco.com
 */

#include "systems.h"
#include <rtp/rtp.h>
#include <rtp/memory.h>
#include <sdp/sdp.h> // for NTP_TO_UNIX_TIME
#include "aac_rtp_bytestream.h"
#include "isma_rtp_bytestream.h"
#include "our_config_file.h"

/*
 * I'm not sure that I'm happy with this.  For aac, we now use
 * the frame length in the packet.  We don't move to the next
 * packet until we get the start of frame. This causes have_no_data
 * to pass, but start_next_frame to fail.  I've gimmicked it up to 
 * work, but there might be a more elegant solution.
 */

/*
 * aac rtp bytestream basically works like a RTP byte stream; however, 
 * it uses local pointers for the frame and data, and doesn't adjust
 * the normal rtp byte stream values (m_offset_in_pak, m_pak) until the
 * start of the next frame.
 */
CAacRtpByteStream::CAacRtpByteStream(unsigned int rtp_proto,
				     int ondemand,
				     uint64_t tps,
				     rtp_packet **head, 
				     rtp_packet **tail,
				     int rtpinfo_received,
				     uint32_t rtp_rtptime,
				     int rtcp_received,
				     uint32_t ntp_frac,
				     uint32_t ntp_sec,
				     uint32_t rtp_ts) :
  CRtpByteStreamBase(rtp_proto, ondemand, tps, head, tail, 
		     rtpinfo_received, rtp_rtptime, rtcp_received,
		     ntp_frac, ntp_sec, rtp_ts)
{
  m_frame_ptr = NULL;
  m_offset_in_frame = 0;
  m_frame_len = 0;
}

unsigned char CAacRtpByteStream::get (void)
{
  unsigned char ret;

  if (m_pak == NULL || m_frame_ptr == NULL) {
    if (m_bookmark_set == 1) {
      return (0);
    }
    init();
    throw THROW_RTP_NULL_WHEN_START;
  }

  if (m_offset_in_frame >= m_frame_len) {
    if (m_bookmark_set == 1) {
      return (0);
    }
    throw THROW_RTP_DECODE_PAST_EOF;
  }
  ret = m_frame_ptr[m_offset_in_frame];
  m_offset_in_frame++;
  m_total++;
  //check_for_end_of_pak();
  return (ret);
}

unsigned char CAacRtpByteStream::peek (void) 
{
  return (m_frame_ptr ? m_frame_ptr[m_offset_in_frame] : 0);
}

void CAacRtpByteStream::bookmark (int bSet)
{
  if (bSet == TRUE) {
    m_bookmark_set = 1;
    m_bookmark_offset_in_frame = m_offset_in_frame;
    m_total_book = m_total;
    //player_debug_message("bookmark on");
  } else {
    m_bookmark_set = 0;
    m_offset_in_frame = m_bookmark_offset_in_frame;
    m_total = m_total_book;
    //player_debug_message("book restore %d", m_offset_in_pak);
  }
}

uint64_t CAacRtpByteStream::start_next_frame (void)
{
  uint64_t timetick;
  m_offset_in_frame = 0;
  if (m_frame_ptr != NULL) {
    m_offset_in_pak += m_frame_len;
    check_for_end_of_pak(1); // don't throw an exception...
  }
  if (m_pak != NULL) {
    m_frame_ptr = &m_pak->rtp_data[m_offset_in_pak];
    m_frame_len = ntohs(*(unsigned short *)m_frame_ptr);
    m_offset_in_pak += 2;
    m_frame_ptr += 2;
  }
  // We're going to have to handle wrap better...
  if (m_stream_ondemand) {
    int neg = 0;
    if (m_pak != NULL)
      m_ts = m_pak->rtp_pak_ts;
    
    if (m_ts >= m_rtp_rtptime) {
      timetick = m_ts;
      timetick -= m_rtp_rtptime;
    } else {
      timetick = m_rtp_rtptime - m_ts;
      neg = 1;
    }
    timetick *= 1000;
    timetick /= m_rtptime_tickpersec;
    if (neg == 1) {
      if (timetick > m_play_start_time) return (0);
      return (m_play_start_time - timetick);
    }
    timetick += m_play_start_time;
  } else {
    if (m_pak != NULL) {
      if (((m_ts & 0x80000000) == 0x80000000) &&
	  ((m_pak->rtp_pak_ts & 0x80000000) == 0)) {
	m_wrap_offset += (I_LLU << 32);
      }
      m_ts = m_pak->rtp_pak_ts;
    }
    timetick = m_ts + m_wrap_offset;
    timetick *= 1000;
    timetick /= m_rtptime_tickpersec;
    timetick += m_wallclock_offset;
#if 0
	player_debug_message("time %x "LLX" "LLX" "LLX" "LLX,
			m_ts, m_wrap_offset, m_rtptime_tickpersec, m_wallclock_offset,
			timetick);
#endif
  }
  return (timetick);
					   
}

ssize_t CAacRtpByteStream::read (unsigned char *buffer, size_t bytes_to_read)
{
  size_t inbuffer;

  if (m_pak == NULL || m_frame_ptr == NULL) {
    if (m_bookmark_set == 1) {
      return (0);
    }
    init();
    throw THROW_RTP_NULL_WHEN_START;
  }

  inbuffer = m_frame_len - m_offset_in_frame;
  if (inbuffer < bytes_to_read) {
    bytes_to_read = inbuffer;
  }
  memcpy(buffer, &m_frame_ptr[m_offset_in_frame], bytes_to_read);
  m_offset_in_frame += bytes_to_read;
  m_total += bytes_to_read;
  return (bytes_to_read);
}

void CAacRtpByteStream::reset (void)
{
  m_frame_ptr = NULL;
  m_offset_in_frame = 0;
  m_frame_len = 0;
  CRtpByteStreamBase::reset();
}

const char *CAacRtpByteStream::get_throw_error (int error)
{
  if (error <= THROW_RTP_BASE_MAX) {
    return (CRtpByteStreamBase::get_throw_error(error));
  }
  if (error == THROW_RTP_DECODE_PAST_EOF)
    return "Read past end of frame";
  player_debug_message("AAC rtp bytestream - unknown throw error %d", error);
  return "Unknown error";
}

int CAacRtpByteStream::throw_error_minor (int error)
{
  if (error <= THROW_RTP_BASE_MAX) {
    return (CRtpByteStreamBase::throw_error_minor(error));
  }
  return 0;
}

CRtpByteStreamBase *create_aac_rtp_bytestream (format_list_t *media_fmt,
					       unsigned int rtp_proto,
					       int ondemand,
					       uint64_t tickpersec,
					       rtp_packet **head, 
					       rtp_packet **tail,
					       int rtpinfo_received,
					       uint32_t rtp_rtptime,
					       int rtcp_received,
					       uint32_t ntp_frac,
					       uint32_t ntp_sec,
					       uint32_t rtp_ts)
{
  CRtpByteStreamBase *rtp_byte_stream = NULL;

  if ((strncasecmp(media_fmt->rtpmap->encode_name,
		  "mpeg-simple-a",
		  strlen("mpeg-simple-a")) == 0) ||
      (strncasecmp(media_fmt->rtpmap->encode_name, 
		   "mpeg4-simple-a",
		   strlen("mpeg4-simple-a")) == 0) ||
      (strncasecmp(media_fmt->rtpmap->encode_name,
		   "mpeg4-generic",
		   strlen("mpeg4-generic")) == 0)) {
    fmtp_parse_t *fmtp;

    fmtp = parse_fmtp_for_mpeg4(media_fmt->fmt_param);
    if (fmtp->size_length == 0) {
      // No headers in RTP packet -create normal bytestream
      player_debug_message("Size of AAC RTP header is 0 - normal bytestream");
      return (NULL);
    }
    rtp_byte_stream = new CIsmaAudioRtpByteStream(media_fmt,
						  fmtp,
						  rtp_proto,
						  ondemand,
						  tickpersec,
						  head,
						  tail,
						  rtpinfo_received,
						  rtp_rtptime,
						  rtcp_received,
						  ntp_frac,
						  ntp_sec,
						  rtp_ts);
  } else {
    player_debug_message("starting normal bytestream");
    rtp_byte_stream = new CAacRtpByteStream(rtp_proto,
					    ondemand,
					    tickpersec,
					    head,
					    tail,
					    rtpinfo_received,
					    rtp_rtptime,
					    rtcp_received,
					    ntp_frac,
					    ntp_sec,
					    rtp_ts);
  }
  return (rtp_byte_stream);

}


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

#include "rtp_bytestream_plugin.h"
#include "rtp/memory.h"
#include "player_util.h"
#include "our_config_file.h"
#ifdef _WIN32
DEFINE_MESSAGE_MACRO(rtp_message, "rtpbyst")
#else
#define rtp_message(loglevel, fmt...) message(loglevel, "rtpbyst", fmt)
#endif

static uint64_t rtp_ts_to_msec (void *ifp, 
				uint32_t rtp_ts, 
				uint64_t uts, 
				int just_checking)
{
  CPluginRtpByteStream *cptr = (CPluginRtpByteStream *)ifp;

  return cptr->get_rtp_ts_to_msec(rtp_ts, uts, just_checking);
}

static rtp_packet *get_next_pak (void *ifp, rtp_packet *current, int remove)
{
  CPluginRtpByteStream *cptr = (CPluginRtpByteStream *)ifp;

  return cptr->get_next_pak(current, remove);
}

static void remove_from_list (void *ifp, rtp_packet *pak)
{
  CPluginRtpByteStream *cptr = (CPluginRtpByteStream *)ifp;
  cptr->remove_from_list(pak);
}

static void free_pak (rtp_packet *pak) 
{
  xfree(pak);
}

static bool find_mbit (void *ifp) 
{
  CPluginRtpByteStream *cptr = (CPluginRtpByteStream *)ifp;
  return cptr->find_mbit();
}


static rtp_packet *get_head_and_check(void *ifp, 
				      bool fail_if_not, 
				      uint32_t rtp_ts)
{
  CPluginRtpByteStream *cptr = (CPluginRtpByteStream *)ifp;
  return cptr->get_head_and_check(fail_if_not, rtp_ts);
}

static rtp_vft_t rtp_vft = {
  message,
  rtp_ts_to_msec,
  get_next_pak,
  remove_from_list,
  free_pak,
  NULL,
  find_mbit,
  get_head_and_check,
};

CPluginRtpByteStream::CPluginRtpByteStream(rtp_plugin_t *rtp_plugin,
					   format_list_t *fmt,
					   unsigned int rtp_pt,
					   int ondemand,
					   uint64_t tickpersec,
					   rtp_packet **head, 
					   rtp_packet **tail,
					   int rtp_seq_set,
					   uint16_t rtp_base_seq,
					   int rtp_ts_set,
					   uint32_t rtp_base_ts,
					   int rtcp_received,
					   uint32_t ntp_frac,
					   uint32_t ntp_sec,
					   uint32_t rtp_ts) :
  CRtpByteStreamBase(rtp_plugin->name, fmt, rtp_pt, ondemand, tickpersec, 
		     head, tail,rtp_seq_set, rtp_base_seq, rtp_ts_set, 
		     rtp_base_ts, rtcp_received, ntp_frac, ntp_sec, rtp_ts)
{
  m_rtp_plugin = rtp_plugin;

  rtp_vft.pConfig = &config;
  m_rtp_plugin_data = (m_rtp_plugin->rtp_plugin_create)(fmt, 
							rtp_pt, 
							&rtp_vft, 
							this);
					   
}

CPluginRtpByteStream::~CPluginRtpByteStream (void)
{
  (m_rtp_plugin->rtp_plugin_destroy)(m_rtp_plugin_data);

}

bool CPluginRtpByteStream::start_next_frame (uint8_t **buffer, 
					     uint32_t *buflen,
					     frame_timestamp_t *ts,
					     void **userdata)
{
  ts->audio_freq = m_timescale;
  return (m_rtp_plugin->rtp_plugin_start_next_frame)(m_rtp_plugin_data,
						     buffer, 
						     buflen, 
						     ts, 
						     userdata);
}

bool CPluginRtpByteStream::skip_next_frame (frame_timestamp_t *pts, 
					   int *hasSyncFrame, 
					   uint8_t **buffer, 
					   uint32_t *buflen,
					   void **ud)
{
  uint32_t ts;
  *hasSyncFrame = -1;

  
  if (m_head == NULL) return false;

  ts = m_head->rtp_pak_ts;
  do {
    check_seq(m_head->rtp_pak_seq);
    set_last_seq(m_head->rtp_pak_seq);
    remove_packet_rtp_queue(m_head, 1);
  } while (m_head != NULL && m_head->rtp_pak_ts == ts);

  if (m_head == NULL) return false;
  (m_rtp_plugin->rtp_plugin_reset)(m_rtp_plugin_data);

  return start_next_frame(buffer, buflen, pts, ud);
}
void CPluginRtpByteStream::used_bytes_for_frame (uint32_t bytes)
{
  (m_rtp_plugin->rtp_plugin_used_bytes_for_frame)(m_rtp_plugin_data, bytes);
}

bool CPluginRtpByteStream::have_frame (void)
{
  return (m_rtp_plugin->rtp_plugin_have_frame)(m_rtp_plugin_data);
}

void CPluginRtpByteStream::flush_rtp_packets (void)
{
  (m_rtp_plugin->rtp_plugin_flush)(m_rtp_plugin_data);
  CRtpByteStreamBase::flush_rtp_packets();
}

void CPluginRtpByteStream::reset (void)
{
  (m_rtp_plugin->rtp_plugin_reset)(m_rtp_plugin_data);
  CRtpByteStreamBase::reset();
}

//void CPluginRtpByteStream::
rtp_packet *CPluginRtpByteStream::get_next_pak (rtp_packet *current, 
						int remove)
{
  if (current == NULL) current = m_head;
  else current = current->rtp_next;

  if (remove && current) {
    remove_packet_rtp_queue(current, 0);
  }
  return (current);
}

void CPluginRtpByteStream::remove_from_list (rtp_packet *pak)
{
  remove_packet_rtp_queue(pak, 0);
}


rtp_packet *CPluginRtpByteStream::get_head_and_check (bool fail_if_not,
						      uint32_t ts)
{
  rtp_packet *ret;
  ret = m_head;
  if (ret == NULL) return NULL;
  if (check_seq(ret->rtp_pak_seq) == false && fail_if_not == true)
    return NULL;
  if (fail_if_not && ts != ret->rtp_pak_ts) {
    rtp_message(LOG_INFO, "%s - rtp timestamp doesn't match is %u should be %u", 
		m_name, ret->rtp_pak_ts, ts);

    return NULL;
  }
  remove_packet_rtp_queue(ret, 0);
  set_last_seq(ret->rtp_pak_seq);
  return ret;
}

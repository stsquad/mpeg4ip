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

static uint64_t rtp_ts_to_msec (void *ifp, uint32_t rtp_ts, int just_checking)
{
  CPluginRtpByteStream *cptr = (CPluginRtpByteStream *)ifp;

  return cptr->get_rtp_ts_to_msec(rtp_ts, just_checking);
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

static rtp_vft_t rtp_vft = {
  message,
  rtp_ts_to_msec,
  get_next_pak,
  remove_from_list,
  free_pak,
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

  m_rtp_plugin_data = (m_rtp_plugin->rtp_plugin_create)(fmt, 
							rtp_pt, 
							&rtp_vft, 
							this);
					   
}

CPluginRtpByteStream::~CPluginRtpByteStream (void)
{
  (m_rtp_plugin->rtp_plugin_destroy)(m_rtp_plugin_data);

}

uint64_t CPluginRtpByteStream::start_next_frame (uint8_t **buffer, 
						 uint32_t *buflen,
						 void **userdata)
{
  return (m_rtp_plugin->rtp_plugin_start_next_frame)(m_rtp_plugin_data,
						     buffer, 
						     buflen, 
						     userdata);
}

void CPluginRtpByteStream::used_bytes_for_frame (uint32_t bytes)
{
  (m_rtp_plugin->rtp_plugin_used_bytes_for_frame)(m_rtp_plugin_data, bytes);
}

int CPluginRtpByteStream::have_no_data (void)
{
  return (m_rtp_plugin->rtp_plugin_have_no_data)(m_rtp_plugin_data);
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

  if (remove) {
    remove_packet_rtp_queue(current, 0);
  }
  return (current);
}

void CPluginRtpByteStream::remove_from_list (rtp_packet *pak)
{
  remove_packet_rtp_queue(pak, 0);
}


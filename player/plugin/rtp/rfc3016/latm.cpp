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

#include "latm.h"
#include "mp4util/mpeg4_audio_config.h"
#include "rtp/rtp.h"
//#define DEBUG_LATM_AAC

#define latm_message iptr->m_vft->log_msg
static const char *latmrtp="latmrtp";

static rtp_check_return_t check (lib_message_func_t msg, 
				 format_list_t *fmt, 
				 uint8_t rtp_payload_type,
				 CConfigSet *pConfig)
{

  if (fmt == NULL || fmt->rtpmap_name == NULL) 
    return RTP_PLUGIN_NO_MATCH;

  if (strcasecmp(fmt->rtpmap_name, "mp4a-latm") != 0) {
    return RTP_PLUGIN_NO_MATCH;
  }

  fmtp_parse_t *fmtp;
  int len, cpresent;
  fmtp = parse_fmtp_for_rfc3016(fmt->fmt_param, msg);
  if (fmtp == NULL) {
    (msg)(LOG_ERR, latmrtp, "Couldn't parse fmtp");
    return RTP_PLUGIN_NO_MATCH;
  }

  len = fmtp->config_binary_len;
  cpresent = fmtp->cpresent;
  free_fmtp_parse(fmtp);
  if (len == 0 || cpresent != 0) {
    (msg)(LOG_ERR, latmrtp, "%s len %u cpresent %u", fmt->rtpmap_name,
	  len, cpresent);
    return RTP_PLUGIN_NO_MATCH;
  }
  return RTP_PLUGIN_MATCH;
}
/*
 * latm rtp bytestream has a potential set of headers at the beginning
 * of each rtp frame.  This can interleave frames in different packets
 */
rtp_plugin_data_t *latm_rtp_plugin_create (format_list_t *media_fmt,
					   uint8_t rtp_payload_type, 
					   rtp_vft_t *vft,
					   void *ifptr)
{
  latm_rtp_data_t *iptr;
  
  iptr = MALLOC_STRUCTURE(latm_rtp_data_t);
  memset(iptr, 0, sizeof(latm_rtp_data_t));
  iptr->m_vft = vft;
  iptr->m_ifptr = ifptr;

  return (&iptr->plug);
}


static void latm_rtp_destroy (rtp_plugin_data_t *pifptr)
{
  latm_rtp_data_t *iptr = (latm_rtp_data_t *)pifptr;

  if (iptr->m_rpak != NULL) {
    (iptr->m_vft->free_pak)(iptr->m_rpak);
    iptr->m_rpak = NULL;
  }
  CHECK_AND_FREE(iptr->m_frag_buffer);
#ifdef LATM_RTP_DUMP_OUTPUT_TO_FILE
  fclose(iptr->m_outfile);
#endif
  free(iptr);
}


static bool start_next_frame (rtp_plugin_data_t *pifptr, 
			      uint8_t **buffer, 
			      uint32_t *buflen,
			      frame_timestamp_t *ts,
			      void **userdata)
{
  latm_rtp_data_t *iptr = (latm_rtp_data_t *)pifptr;
  uint64_t timetick;
  rtp_packet *rpak;
  uint32_t rtp_ts;
  uint8_t *dptr;
  uint32_t dlen;
  uint32_t calc_len;
  uint64_t ntp_ts;

  if (iptr->m_rpak != NULL) {
    (iptr->m_vft->free_pak)(iptr->m_rpak);
    iptr->m_rpak = NULL;
  }
  rpak = (iptr->m_vft->get_head_and_check)(iptr->m_ifptr, false, 0);
  if (rpak == NULL) return false;

  dlen = rpak->rtp_data_len;
  dptr = rpak->rtp_data;
  rtp_ts = rpak->rtp_pak_ts;
  ntp_ts = rpak->pd.rtp_pd_timestamp;
  calc_len = 0;
  uint8_t value;
  do {
    value = *dptr++;
    calc_len += value;
    dlen--;
  } while (value == 0xff);
#if 0
  latm_message(LOG_DEBUG, latmrtp, "len %u header %u", 
	       calc_len, rpak->rtp_data_len - dlen);
#endif
  if (rpak->rtp_pak_m != 0) {
    // just one packet - we should have frame length
    if (calc_len != dlen) {
      latm_message(LOG_ERR, latmrtp, "header length not correct %u %u", 
		   calc_len, dlen);
      return false;
    }    
    iptr->m_rpak = rpak;

    *buffer = dptr;
    *buflen = dlen;
  } else {
    if (calc_len > iptr->m_frag_buffer_len) {
      iptr->m_frag_buffer = (uint8_t *)realloc(iptr->m_frag_buffer, calc_len);
      iptr->m_frag_buffer_len = calc_len;
    }
    memcpy(iptr->m_frag_buffer, dptr, dlen);
    calc_len -= dlen;
    uint32_t offset = dlen;
    (iptr->m_vft->free_pak)(rpak);
    do {
      rpak = iptr->m_vft->get_head_and_check(iptr->m_ifptr,
					     true, 
					     rtp_ts);
      if (rpak == NULL) return false;
      if (calc_len < rpak->rtp_data_len) {
	latm_message(LOG_ERR, latmrtp, "Illegal frag len - remaining %u pak len %u", calc_len, rpak->rtp_data_len);
	return false;
      }
      memcpy(iptr->m_frag_buffer + offset,
	     rpak->rtp_data,
	     rpak->rtp_data_len);
      calc_len -= rpak->rtp_data_len;
      offset += rpak->rtp_data_len;
    } while (calc_len > 0);

  }
    
  timetick = iptr->m_vft->rtp_ts_to_msec(iptr->m_ifptr, 
					 rtp_ts, 
					 ntp_ts, 
					 0);
  ts->audio_freq_timestamp = rtp_ts;
  ts->msec_timestamp = timetick;
  ts->timestamp_is_pts = true;
  return (true);
}

static void used_bytes_for_frame (rtp_plugin_data_t *pifptr, uint32_t bytes)
{
}

static void reset (rtp_plugin_data_t *pifptr)
{
}

static void flush_rtp_packets (rtp_plugin_data_t *pifptr)
{
}

static bool have_frame (rtp_plugin_data_t *pifptr)
{
  latm_rtp_data_t *iptr = (latm_rtp_data_t *)pifptr;
  return (iptr->m_vft->find_mbit)(iptr->m_ifptr);
}

RTP_PLUGIN("mpeg4-latm", 
	   check,
	   latm_rtp_plugin_create,
	   latm_rtp_destroy,
	   start_next_frame, 
	   used_bytes_for_frame,
	   reset, 
	   flush_rtp_packets,
	   have_frame,
	   NULL,
	   0);

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
 * Copyright (C) Cisco Systems Inc. 2005.  All Rights Reserved.
 * 
 * Contributor(s): 
 *              Bill May        wmay@cisco.com
 */

#include "href_plugin.h"
#include "rtp.h"
//#define DEBUG_HREF 1

#define href_message iptr->m_vft->log_msg
//static const char *hrefrtp="hrefrtp";

static rtp_check_return_t check (lib_message_func_t msg, 
				 format_list_t *fmt, 
				 uint8_t rtp_payload_type,
				 CConfigSet *pConfig)
{

  if (fmt == NULL)
    return RTP_PLUGIN_NO_MATCH;

  if (fmt->rtpmap_name != NULL) {
    if (strcasecmp(fmt->rtpmap_name, "X-HREF") == 0) {
      return RTP_PLUGIN_MATCH;
    }
    // handle just a plain text url, as well...
    if (strcasecmp(fmt->rtpmap_name, "x-plain-text") == 0) {
      return RTP_PLUGIN_MATCH_USE_VIDEO_DEFAULT;
    }
  }

  return RTP_PLUGIN_NO_MATCH;
}

/*
 * Isma rtp bytestream has a potential set of headers at the beginning
 * of each rtp frame.  This can interleave frames in different packets
 */
rtp_plugin_data_t *href_rtp_plugin_create (format_list_t *media_fmt,
					   uint8_t rtp_payload_type, 
					   rtp_vft_t *vft,
					   void *ifptr)
{
  href_rtp_data_t *iptr;

  iptr = MALLOC_STRUCTURE(href_rtp_data_t);
  memset(iptr, 0, sizeof(*iptr));
  iptr->m_vft = vft;
  iptr->m_ifptr = ifptr;

#ifdef HREF_RTP_DUMP_OUTPUT_TO_FILE
  iptr->m_outfile = fopen("rtp.h263", "w");
#endif
  iptr->m_buffer = NULL;
  iptr->m_buffer_len = 0;
  iptr->m_buffer_len_max = 0;
  iptr->pak = NULL;
  return (&iptr->plug);
}


static void href_rtp_destroy (rtp_plugin_data_t *pifptr)
{
  href_rtp_data_t *iptr = (href_rtp_data_t *)pifptr;

#ifdef ISMA_RTP_DUMP_OUTPUT_TO_FILE
  fclose(iptr->m_outfile);
#endif
  CHECK_AND_FREE(iptr->m_buffer);
  if (iptr->pak != NULL) {
    iptr->m_vft->free_pak(iptr->pak);
    iptr->pak = NULL;
  }
  free(iptr);
}

  
static bool start_next_frame (rtp_plugin_data_t *pifptr, 
			      uint8_t **buffer, 
			      uint32_t *buflen,
			      frame_timestamp_t *pts,
			      void **userdata)
{
  href_rtp_data_t *iptr = (href_rtp_data_t *)pifptr;
  // we should have a start code here
 
  while (1) {
    if (iptr->pak != NULL) {
      // still have something left in the packet
      //
      uint16_t ts_offset;
      ts_offset = ntohs(*(uint16_t *)(iptr->pak->rtp_data + 
				     iptr->m_next_au_offset));
      iptr->m_next_au_offset += sizeof(uint16_t);
      iptr->m_au_ts += ts_offset;
    } else {
      iptr->pak = (iptr->m_vft->get_head_and_check)(iptr->m_ifptr, 
						    false, 
						    0);
      if (iptr->pak == NULL) return false;
      if (iptr->pak->rtp_pak_m == 0) {
	href_message(LOG_ERR, "hrefb", "pak seq %u M bit 0", 
		     iptr->pak->rtp_pak_seq);
	iptr->m_vft->free_pak(iptr->pak);
	iptr->pak = NULL;
	continue;
      }
	
      iptr->m_au_ts = iptr->pak->rtp_pak_ts;
      iptr->m_aus_in_pak = (iptr->pak->rtp_data[1]);
      iptr->m_next_au_offset = 2;
      iptr->m_next_au = 1;
      iptr->m_au_ts = iptr->pak->rtp_pak_ts;
#ifdef DEBUG_HREF
      href_message(LOG_DEBUG, "hrefb", "href ts %u aus %u", 
		   iptr->m_au_ts, iptr->m_aus_in_pak);
#endif
    }

    uint16_t nextlen;
    nextlen = ntohs(*(uint16_t *)(iptr->pak->rtp_data + 
				  iptr->m_next_au_offset));
    iptr->m_next_au_offset += sizeof(uint16_t);

    if (iptr->m_next_au_offset + nextlen < iptr->pak->rtp_data_len) {
      href_message(LOG_ERR, "hrefb", "illegal size - off %u next %u len %u", 
		   iptr->m_next_au_offset, nextlen, iptr->pak->rtp_data_len);
      iptr->m_vft->free_pak(iptr->pak);
      iptr->pak = NULL;
      continue;
    }

    if (nextlen > iptr->m_buffer_len_max) {
      iptr->m_buffer_len_max = nextlen;
      iptr->m_buffer = (uint8_t *)realloc(iptr->m_buffer, nextlen);
    }
    memcpy(iptr->m_buffer, 
	   iptr->pak->rtp_data + iptr->m_next_au_offset, 
	   nextlen);
    iptr->m_buffer_len = nextlen;

    pts->msec_timestamp = 
      iptr->m_vft->rtp_ts_to_msec(iptr->m_ifptr, 
				  iptr->m_au_ts,
				  iptr->pak->pd.rtp_pd_timestamp,
				  0);
    pts->timestamp_is_pts = false;
    *buffer = iptr->m_buffer;
    *buflen = iptr->m_buffer_len;
    *userdata = NULL;

    iptr->m_next_au_offset += nextlen;
    iptr->m_next_au++;
    if (iptr->m_next_au >= iptr->m_aus_in_pak) {
      iptr->m_vft->free_pak(iptr->pak);
      iptr->pak = NULL;
    }
    return true;
  }
  return false;

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
  href_rtp_data_t *iptr = (href_rtp_data_t *)pifptr;

  return (iptr->m_vft->find_mbit)(iptr->m_ifptr);
}

RTP_PLUGIN("isma-href", 
	   check,
	   href_rtp_plugin_create,
	   href_rtp_destroy,
	   start_next_frame, 
	   used_bytes_for_frame,
	   reset, 
	   flush_rtp_packets,
	   have_frame,
	   NULL,
	   0);

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

#include "h261_rtp_plugin.h"
#include "h261_userdata.h"
#include "rtp/rtp.h"
//#define DEBUG_H261

#define h261_message iptr->m_vft->log_msg
static const char *h261rtp="h261rtp";

static rtp_check_return_t check (lib_message_func_t msg, 
				 format_list_t *fmt, 
				 uint8_t rtp_payload_type,
				 CConfigSet *pConfig)
{

  if (fmt == NULL)
    return RTP_PLUGIN_NO_MATCH;

  if (strcmp(fmt->fmt, "31") == 0) {
    return RTP_PLUGIN_MATCH;
  }

  if (fmt->rtpmap_name && strcasecmp(fmt->rtpmap_name, "h261") == 0) {
    return RTP_PLUGIN_MATCH;
  }

  return RTP_PLUGIN_NO_MATCH;
}

/*
 * Isma rtp bytestream has a potential set of headers at the beginning
 * of each rtp frame.  This can interleave frames in different packets
 */
rtp_plugin_data_t *h261_rtp_plugin_create (format_list_t *media_fmt,
					   uint8_t rtp_payload_type, 
					   rtp_vft_t *vft,
					   void *ifptr)
{
  h261_rtp_data_t *iptr;

  iptr = MALLOC_STRUCTURE(h261_rtp_data_t);
  memset(iptr, 0, sizeof(*iptr));
  iptr->m_vft = vft;
  iptr->m_ifptr = ifptr;

#ifdef H261_RTP_DUMP_OUTPUT_TO_FILE
  iptr->m_outfile = fopen("rtp.h261", "w");
#endif

  iptr->m_first_pak = 0;
  iptr->m_current_pak = NULL;
  return (&iptr->plug);
}


static void h261_rtp_destroy (rtp_plugin_data_t *pifptr)
{
  h261_rtp_data_t *iptr = (h261_rtp_data_t *)pifptr;

#ifdef ISMA_RTP_DUMP_OUTPUT_TO_FILE
  fclose(iptr->m_outfile);
#endif
  if (iptr->m_current_pak != NULL) {
    (iptr->m_vft->free_pak)(iptr->m_current_pak);
    iptr->m_current_pak = NULL;
  }
  free(iptr);
}

static bool start_next_frame (rtp_plugin_data_t *pifptr, 
			      uint8_t **buffer, 
			      uint32_t *buflen,
			      frame_timestamp_t *ts,
			      void **userdata)
{
  h261_rtp_data_t *iptr = (h261_rtp_data_t *)pifptr;
  uint64_t timetick;
  h261_rtp_userdata_t *udata;

  udata = MALLOC_STRUCTURE(h261_rtp_userdata_t);

  if (iptr->m_current_pak != NULL) {
    (iptr->m_vft->free_pak)(iptr->m_current_pak);
    iptr->m_current_pak = NULL;
  }

  iptr->m_current_pak = (iptr->m_vft->get_next_pak)(iptr->m_ifptr, 
						    NULL, 
						    1);
  if (iptr->m_current_pak == NULL) return false;

  udata->detected_loss = 0;
  if (iptr->m_first_pak != 0) {
    if (iptr->m_last_seq + 1 != iptr->m_current_pak->rtp_pak_seq) {
      udata->detected_loss = 1;
      h261_message(LOG_ERR, h261rtp, "RTP sequence should be %d is %d", 
		   iptr->m_last_seq + 1, iptr->m_current_pak->rtp_pak_seq);
    }
  }
  udata->m_bit_value = iptr->m_current_pak->rtp_pak_m;
  iptr->m_first_pak = 1;
  iptr->m_last_seq = iptr->m_current_pak->rtp_pak_seq;

  *buffer = iptr->m_current_pak->rtp_data;
  *buflen = iptr->m_current_pak->rtp_data_len;
  *userdata = udata;

#ifdef H261_RTP_DUMP_OUTPUT_TO_FILE
  if (*buffer != NULL) {
    fwrite(*buffer, *buflen,  1, iptr->m_outfile);
  }
#endif
  timetick = 
    iptr->m_vft->rtp_ts_to_msec(iptr->m_ifptr, 
				iptr->m_current_pak->rtp_pak_ts,
				iptr->m_current_pak->pd.rtp_pd_timestamp,
				0);
  // We're going to have to handle wrap better...
#ifdef DEBUG_H261
  h261_message(LOG_DEBUG, h261rtp, "start next frame %p %d ts %x "U64, 
	       *buffer, *buflen, iptr->m_current_pak->rtp_pak_ts, timetick);
#endif
  ts->msec_timestamp = timetick;
  ts->timestamp_is_pts = false;
  return (timetick);
}

static void used_bytes_for_frame (rtp_plugin_data_t *pifptr, uint32_t bytes)
{
}

static void reset (rtp_plugin_data_t *pifptr)
{
  h261_rtp_data_t *iptr = (h261_rtp_data_t *)pifptr;
  iptr->m_first_pak = 0;
}

static void flush_rtp_packets (rtp_plugin_data_t *pifptr)
{

}

static bool have_frame (rtp_plugin_data_t *pifptr)
{
  h261_rtp_data_t *iptr = (h261_rtp_data_t *)pifptr;
  return (iptr->m_vft->get_next_pak(iptr->m_ifptr, NULL, 0) != NULL);
}

RTP_PLUGIN("h261", 
	   check,
	   h261_rtp_plugin_create,
	   h261_rtp_destroy,
	   start_next_frame, 
	   used_bytes_for_frame,
	   reset, 
	   flush_rtp_packets,
	   have_frame,
	   NULL,
	   0);

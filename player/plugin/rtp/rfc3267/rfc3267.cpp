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

#include "rfc3267.h"
#include "mp4av.h"
#include "rtp.h"

#define rfc3267_message iptr->m_vft->log_msg
static const char *rfc3267rtp="rfc3267";
//#define DEBUG_RFC3267 1
//#define DEBUG_RFC3267_FRAME 1
static rtp_check_return_t check (lib_message_func_t msg, 
				 format_list_t *fmt, 
				 uint8_t rtp_payload_type,
				 CConfigSet *pConfig)
{

  if (fmt == NULL || fmt->rtpmap_name == NULL) 
    return RTP_PLUGIN_NO_MATCH;

  if ((strcasecmp(fmt->rtpmap_name, "AMR") != 0) &&
      (strcasecmp(fmt->rtpmap_name, "AMR-WB") != 0)) {
    return RTP_PLUGIN_NO_MATCH;
  }
  const char *interleave = strcasestr(fmt->fmt_param, "interleaving");
  if (interleave != NULL) return RTP_PLUGIN_NO_MATCH;

  const char *octet_align = strcasestr(fmt->fmt_param, "octet-align");
  if (octet_align == NULL) 
    return RTP_PLUGIN_NO_MATCH;

  octet_align += strlen("octet-align");
  ADV_SPACE(octet_align);
  if ((*octet_align == '\0') || *octet_align == ';')
    return RTP_PLUGIN_MATCH;

  if (*octet_align++ != '=') return RTP_PLUGIN_NO_MATCH;
  ADV_SPACE(octet_align);
  if (*octet_align != '1') return RTP_PLUGIN_NO_MATCH;
  return RTP_PLUGIN_MATCH;
}

/*
 * Isma rtp bytestream has a potential set of headers at the beginning
 * of each rtp frame.  This can interleave frames in different packets
 */
rtp_plugin_data_t *rfc3267_plugin_create (format_list_t *media_fmt,
					   uint8_t rtp_payload_type, 
					   rtp_vft_t *vft,
					   void *ifptr)
{
  rfc3267_data_t *iptr;

  iptr = MALLOC_STRUCTURE(rfc3267_data_t);
  memset(iptr, 0, sizeof(rfc3267_data_t));
  iptr->m_vft = vft;
  iptr->m_ifptr = ifptr;

  iptr->m_amr_is_wb = 
    strcasecmp(media_fmt->rtpmap_name, "AMR-WB") == 0; 
#ifdef RFC3267_DUMP_OUTPUT_TO_FILE
  iptr->m_outfile = fopen("raw.amr", "w");
#endif
  if (iptr->m_amr_is_wb) {
    iptr->m_rtp_ts_add = 320;
  } else {
    iptr->m_rtp_ts_add = 160;
  }
  rfc3267_message(LOG_DEBUG, rfc3267rtp, "type %s ts add %u",
		  iptr->m_amr_is_wb ? "AMR-WB" : "AMR", iptr->m_rtp_ts_add);
  return (&iptr->plug);
}


static void rfc3267_destroy (rtp_plugin_data_t *pifptr)
{
  rfc3267_data_t *iptr = (rfc3267_data_t *)pifptr;

#ifdef RFC3267_DUMP_OUTPUT_TO_FILE
  fclose(iptr->m_outfile);
#endif
  if (iptr->m_pak_on != NULL) {
    iptr->m_vft->free_pak(iptr->m_pak_on);
  }
  free(iptr);
}

static void flush_rtp_packets (rtp_plugin_data_t *pifptr)
{
  rfc3267_data_t *iptr = (rfc3267_data_t *)pifptr;
  if (iptr->m_pak_on != NULL) {
    iptr->m_vft->free_pak(iptr->m_pak_on);
    iptr->m_pak_on = NULL;
    iptr->m_pak_frame_on = 0;
    iptr->m_pak_frame_offset = 0;
  }
}

static bool start_next_frame (rtp_plugin_data_t *pifptr, 
			      uint8_t **buffer, 
			      uint32_t *buflen,
			      frame_timestamp_t *ts,
			      void **userdata)
{
  rfc3267_data_t *iptr = (rfc3267_data_t *)pifptr;
  uint64_t uts = 0;
  uint64_t timetick;

#ifdef DEBUG_RFC3267
  rfc3267_message(LOG_DEBUG, rfc3267rtp, "start %p frame %u offset %u", 
		  iptr->m_pak_on, iptr->m_pak_frame_on, 
		  iptr->m_pak_frame_offset);
#endif

  // first check if we're done with this packet

  // see if we need to read the packet
  if (iptr->m_pak_on == NULL) {
    do {
      iptr->m_pak_on = iptr->m_vft->get_head_and_check(iptr->m_ifptr, 
						       false,
						       0);
      if (iptr->m_pak_on == NULL) return false;
      iptr->m_pak_frame_offset = 1;
      while (((iptr->m_pak_on->rtp_data[iptr->m_pak_frame_offset] & 0x80) != 0) &&
	     (iptr->m_pak_frame_offset < iptr->m_pak_on->rtp_data_len)) {
	iptr->m_pak_frame_offset++;
      }
      if (iptr->m_pak_frame_offset >= iptr->m_pak_on->rtp_data_len) {
	rfc3267_message(LOG_ERR, rfc3267rtp, "frame seq number %x has incorrect rfc3267 TOC - no last frame indication", iptr->m_pak_on->rtp_pak_seq);
	iptr->m_vft->free_pak(iptr->m_pak_on);
	iptr->m_pak_on = NULL;
      }
    } while (iptr->m_pak_on == NULL);
    iptr->m_pak_frame_on = 0;
    iptr->m_pak_frame_offset++;  
    uts = iptr->m_pak_on->pd.rtp_pd_timestamp;
#ifdef DEBUG_RFC3267
    rfc3267_message(LOG_DEBUG, rfc3267rtp, "start %p frame %u offset %u", 
		    iptr->m_pak_on, iptr->m_pak_frame_on, 
		    iptr->m_pak_frame_offset);
#endif
  }
  // now, use the index we have and the offset we have to
  // determine the buffer size and length
  uint8_t mode;
  int16_t frame_len;
  mode = iptr->m_pak_on->rtp_data[iptr->m_pak_frame_on + 1];
  iptr->m_frame[0] = mode;
  frame_len = MP4AV_AmrFrameSize(mode, iptr->m_amr_is_wb);
  // copy the frame into our buffer, so we'll have the header and the
  // frame.
  memcpy(iptr->m_frame + 1,
	 iptr->m_pak_on->rtp_data + iptr->m_pak_frame_offset,
	 frame_len);
  iptr->m_pak_frame_offset += frame_len;
  // calculate the ts.
  iptr->m_ts = iptr->m_pak_on->rtp_pak_ts;
  iptr->m_ts += iptr->m_pak_frame_on * iptr->m_rtp_ts_add;
#ifdef DEBUG_RFC3267
  rfc3267_message(LOG_DEBUG, rfc3267rtp, "mode 0x%x len %d ts %llu", 
		  mode, frame_len, iptr->m_ts);
#endif
  // Are we done with this packet ?
  uint8_t toc = iptr->m_pak_on->rtp_data[1 + iptr->m_pak_frame_on];
  if ((toc & 0x80) == 0) {
    // toc bit indicates last frame in packet
    // we've already copied the data, so we don't need it anymore...
    // flush_rtp_packets clears out all the data we want it to
    flush_rtp_packets(pifptr);
  } else {
    // still have more frames in the packet.
    iptr->m_pak_frame_on++;
  }

  *buffer = iptr->m_frame;
  *buflen = frame_len + 1; // include the header byte
  // lastly, establish the timestamp
  timetick = 
    iptr->m_vft->rtp_ts_to_msec(iptr->m_ifptr, 
				iptr->m_ts,
				iptr->m_pak_on ?
				iptr->m_pak_on->pd.rtp_pd_timestamp : uts,
				0);
  ts->audio_freq_timestamp = iptr->m_ts;
  ts->msec_timestamp = timetick;
  ts->timestamp_is_pts = false;
  // We're going to have to handle wrap better...
#ifdef DEBUG_RFC3267_FRAME
  rfc3267_message(LOG_DEBUG, rfc3267rtp, "start next frame %p %d ts "X64" "U64, 
	       *buffer, *buflen, iptr->m_ts, timetick);
#endif
  return true;
}

static void used_bytes_for_frame (rtp_plugin_data_t *pifptr, uint32_t bytes)
{
  // we really don't care here - we're just going to move to the next
  // frame, anyway...
}

static void reset (rtp_plugin_data_t *pifptr)
{
  flush_rtp_packets(pifptr);
}


static bool have_frame (rtp_plugin_data_t *pifptr)
{
  rfc3267_data_t *iptr = (rfc3267_data_t *)pifptr;
  if (iptr->m_vft->get_next_pak(iptr->m_ifptr, NULL, 0) != NULL) 
    return true; // we have data

  if (iptr->m_pak_on == NULL) return false; // no pak, none on queue

  uint8_t toc = iptr->m_pak_on->rtp_data[1 + iptr->m_pak_frame_on];
  return (toc & 0x80) != 0; // if toc is not done, we are...
}

RTP_PLUGIN("rfc3267", 
	   check,
	   rfc3267_plugin_create,
	   rfc3267_destroy,
	   start_next_frame, 
	   used_bytes_for_frame,
	   reset, 
	   flush_rtp_packets,
	   have_frame,
	   NULL,
	   0);

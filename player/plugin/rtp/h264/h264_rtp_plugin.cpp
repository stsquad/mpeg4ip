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
 * Copyright (C) Cisco Systems Inc. 2004.  All Rights Reserved.
 * 
 * Contributor(s): 
 *              Bill May        wmay@cisco.com
 */

#include "h264_rtp_plugin.h"
#include "rtp/rtp.h"
#include "mp4av/mp4av_h264.h"
//#define DEBUG_H264

#define h264_message iptr->m_vft->log_msg
static const char *h264rtp="h264rtp";

static rtp_check_return_t check (lib_message_func_t msg, 
				 format_list_t *fmt, 
				 uint8_t rtp_payload_type,
				 CConfigSet *pConfig)
{

  if (fmt == NULL || fmt->rtpmap_name == NULL || fmt->fmt_param == NULL)
    return RTP_PLUGIN_NO_MATCH;

  if (strcasecmp(fmt->rtpmap_name, "h264") == 0) {
    // see if the fmtp has a packetization-mode parameter
    const char *temp;
    temp = strcasestr(fmt->fmt_param, "packetization-mode");
    if (temp == NULL) {
      return RTP_PLUGIN_MATCH;
    }
    temp += strlen("packetization-mode");
    ADV_SPACE(temp);
    if (*temp != '=') return RTP_PLUGIN_NO_MATCH;
    temp++;
    ADV_SPACE(temp);
    if (*temp == '0' || *temp == '1')
      return RTP_PLUGIN_MATCH;
    // not a packetization-mode that we understand
    msg(LOG_DEBUG, h264rtp, "incorrect packetization mode %c for this version", 
	*temp);
  }

  return RTP_PLUGIN_NO_MATCH;
}

rtp_plugin_data_t *h264_rtp_plugin_create (format_list_t *media_fmt,
					   uint8_t rtp_payload_type, 
					   rtp_vft_t *vft,
					   void *ifptr)
{
  h264_rtp_data_t *iptr;

  iptr = MALLOC_STRUCTURE(h264_rtp_data_t);
  memset(iptr, 0, sizeof(*iptr));
  iptr->m_vft = vft;
  iptr->m_ifptr = ifptr;

#ifdef H264_RTP_DUMP_OUTPUT_TO_FILE
  iptr->m_outfile = fopen("rtp.h264", "w");
#endif

  return (&iptr->plug);
}


static void h264_rtp_destroy (rtp_plugin_data_t *pifptr)
{
  h264_rtp_data_t *iptr = (h264_rtp_data_t *)pifptr;

#ifdef ISMA_RTP_DUMP_OUTPUT_TO_FILE
  fclose(iptr->m_outfile);
#endif
  CHECK_AND_FREE(iptr->m_buffer);
  free(iptr);
}
static void add_nal_to_buffer (h264_rtp_data_t *iptr,
			       uint8_t *buffer,
			       uint32_t buflen,
			       uint8_t header,
			       bool add_header = true)
{
  uint32_t headersize;
  if (add_header) {
    uint8_t nal_type = header & 0x1f;
    if (nal_type == H264_NAL_TYPE_SEQ_PARAM ||
	nal_type == H264_NAL_TYPE_PIC_PARAM) {
      headersize = 5;
    } else if (iptr->m_have_first_nal == false) {
      iptr->m_have_first_nal = true;
      headersize = 5;
    } else 
      headersize = 4;
  } else headersize = 0;

  if (iptr->m_buffer_size + buflen + headersize > iptr->m_buffersize_max) {
    iptr->m_buffersize_max += (buflen + headersize) * 2;
    iptr->m_buffer = (uint8_t *)realloc(iptr->m_buffer,iptr->m_buffersize_max);
  }
  // add 00 00 01 [headerbyte] header
  uint8_t *bptr = iptr->m_buffer + iptr->m_buffer_size;
  if (add_header) {
    *bptr++ = 0;
    *bptr++ = 0;
    if (headersize == 5) {
      *bptr++ = 0;
    }
    *bptr++ = 1;
    *bptr++ = header;
  }

  memcpy(iptr->m_buffer + iptr->m_buffer_size + headersize, 
	 buffer, 
	 buflen);
  iptr->m_buffer_size += buflen + headersize;
}

/*
 * process_fus - process fragmentation units.
 */
static bool process_fus (h264_rtp_data_t *iptr, rtp_packet **pRpak,
			 uint32_t rtp_ts)
{
  rtp_packet *rpak = *pRpak;
  uint8_t *dptr;
  uint8_t header;
  dptr = rpak->rtp_data;
  // Read the FU header. d7 - start bit, d6 - end bit, d5 - 0 d43210 - nal type
  if ((dptr[1] & 0x80) != 0x80) {
    h264_message(LOG_ERR, h264rtp, "FUs - first packet no start bit %x seq %u",
		 dptr[1], rpak->rtp_pak_seq);
    return false;
  }
  header = (dptr[0] & 0xe0) | (dptr[1] & 0x1f);
  add_nal_to_buffer(iptr, 
		    rpak->rtp_data + 2, 
		    rpak->rtp_data_len - 2, 
		    header);
  // continue with the rest of the fragmentation - packets should be here, 
  // because we wouldn't be here if we didn't find an m bit
  do {
    (iptr->m_vft->free_pak)(rpak);
    rpak = (iptr->m_vft->get_head_and_check)(iptr->m_ifptr, true, rtp_ts);
    *pRpak = rpak;
    if (rpak == NULL) {
      return false;
    }

    add_nal_to_buffer(iptr, 
		      rpak->rtp_data + 2,
		      rpak->rtp_data_len - 2,
		      0, 
		      false);
  } while ((rpak->rtp_data[1] & 0x40) == 0);
  return true;
}
  
  
static bool start_next_frame (rtp_plugin_data_t *pifptr, 
			      uint8_t **buffer, 
			      uint32_t *buflen,
			      frame_timestamp_t *ts,
			      void **userdata)
{
  h264_rtp_data_t *iptr = (h264_rtp_data_t *)pifptr;
  uint64_t timetick;
  rtp_packet *rpak;
  bool have_m = false;
  uint32_t rtp_ts = 0;
  uint64_t ntp_ts = 0;
  bool have_first = false;

  while (1) {
    rpak = (iptr->m_vft->get_head_and_check)(iptr->m_ifptr, 
					     have_first,
					     rtp_ts);

    if (rpak == NULL) {
      if ((iptr->m_vft->find_mbit)(iptr->m_ifptr)) {
	// we had a problem, and we still have a valid frame
	have_first = false;
	continue;
      } else {
	// problem with no valid frame in the buffer
	return false;
      }
    }

    if (have_first == false) {
      iptr->m_buffer_size = 0;
      rtp_ts = rpak->rtp_pak_ts;
      ntp_ts = rpak->pd.rtp_pd_timestamp;
      have_first = true;
      iptr->m_have_first_nal = false;
    } 
    // process the various NAL types.
    uint8_t *dptr;
    dptr = rpak->rtp_data;
    uint8_t nal_type;
    nal_type = *dptr & 0x1f;
#ifdef H264_DEBUG
    h264_message(LOG_ERR, h264rtp, "nal %d", nal_type);
#endif
    if (nal_type >= H264_NAL_TYPE_NON_IDR_SLICE &&
	nal_type <= H264_NAL_TYPE_FILLER_DATA) {
      // regular NAL - put in buffer, adding the header.
      add_nal_to_buffer(iptr, rpak->rtp_data + 1, 
			rpak->rtp_data_len - 1,
			*rpak->rtp_data);
    } else if (nal_type == 24) {
      // stap-A (single time aggregation packet - copy each access 
      uint32_t data_len = rpak->rtp_data_len - 1; // remove stap header
      dptr++;
      while (data_len > 0) {
	// first, theres a 2 byte length field
	uint32_t len = (dptr[0] << 8) | dptr[1];
	dptr += 2;
	// then the header, followed by the body.  We'll add the header
	// in the add_nal_to_buffer - that's why the nal body is dptr + 1
	add_nal_to_buffer(iptr, dptr + 1,
			  len - 1,
			  *dptr);
	dptr += len;
	if ((len + 2) > data_len) {
	  data_len = 0;
#ifdef H264_DEBUG
	  h264_message(LOG_ERR, h264rtp, 
		       "Stap error - requested %d - %d in buffer", 
		       len + 2, data_len);
#endif
	} else {
	  data_len -= (len + 2);
	}
      }
    } else if (nal_type == 28) {
      // FUs
      if (process_fus(iptr, &rpak, rtp_ts) == false) {
	// had an error
	if (rpak != NULL) {
	  (iptr->m_vft->free_pak)(rpak);
	}
	have_first = false;
	continue;
      }
    } else {
      h264_message(LOG_ERR, h264rtp, "illegal NAL type %d in header seq %u",
		   nal_type, rpak->rtp_pak_seq);
      // just fall through - if it was an m-bit, let the buffer pass
    }
    // save off the last sequence number
    have_m = rpak->rtp_pak_m != 0;
    (iptr->m_vft->free_pak)(rpak);

    if (have_m) {
      *buffer = iptr->m_buffer;
      *buflen = iptr->m_buffer_size;

#ifdef H264_RTP_DUMP_OUTPUT_TO_FILE
      if (*buffer != NULL) {
	fwrite(*buffer, *buflen,  1, iptr->m_outfile);
      }
#endif
      timetick = 
	iptr->m_vft->rtp_ts_to_msec(iptr->m_ifptr, 
				    rtp_ts,
				    ntp_ts,
				    0);

#ifdef DEBUG_H264
      h264_message(LOG_DEBUG, h264rtp, "start next frame %p %d ts %x "U64, 
		   *buffer, *buflen, rtp_ts, timetick);
#endif
      ts->msec_timestamp = timetick;
      ts->timestamp_is_pts = true;
      return true;
    }
    rpak = NULL;
  }

  // We're going to have to handle wrap better...
  return (false);
}

static void used_bytes_for_frame (rtp_plugin_data_t *pifptr, uint32_t bytes)
{
}

static void reset (rtp_plugin_data_t *pifptr)
{
  //  h264_rtp_data_t *iptr = (h264_rtp_data_t *)pifptr;
}

static void flush_rtp_packets (rtp_plugin_data_t *pifptr)
{

}

static bool have_frame (rtp_plugin_data_t *pifptr)
{
  h264_rtp_data_t *iptr = (h264_rtp_data_t *)pifptr;
  return (iptr->m_vft->find_mbit)(iptr->m_ifptr);
}

RTP_PLUGIN("h264", 
	   check,
	   h264_rtp_plugin_create,
	   h264_rtp_destroy,
	   start_next_frame, 
	   used_bytes_for_frame,
	   reset, 
	   flush_rtp_packets,
	   have_frame,
	   NULL,
	   0);

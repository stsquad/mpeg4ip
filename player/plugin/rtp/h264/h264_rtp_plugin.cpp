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

  if (fmt == NULL || fmt->rtpmap == NULL || fmt->fmt_param == NULL)
    return RTP_PLUGIN_NO_MATCH;

  if (strcasecmp(fmt->rtpmap->encode_name, "h264") == 0) {
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
  if (add_header) headersize = 4;
  else headersize = 0;

  if (iptr->m_buffer_size + buflen + headersize > iptr->m_buffersize_max) {
    iptr->m_buffersize_max += (buflen + headersize) * 2;
    iptr->m_buffer = (uint8_t *)realloc(iptr->m_buffer,iptr->m_buffersize_max);
  }
  // add 00 00 01 [headerbyte] header
  if (add_header) {
    iptr->m_buffer[iptr->m_buffer_size] = 0;
    iptr->m_buffer[iptr->m_buffer_size + 1] = 0;
    iptr->m_buffer[iptr->m_buffer_size + 2] = 1;
    iptr->m_buffer[iptr->m_buffer_size + 3] = header;
  }

  memcpy(iptr->m_buffer + iptr->m_buffer_size + headersize, 
	 buffer, 
	 buflen);
  iptr->m_buffer_size += buflen + headersize;
}

static bool process_fus (h264_rtp_data_t *iptr, rtp_packet **pRpak)
{
  rtp_packet *rpak = *pRpak;
  uint8_t *dptr;
  uint8_t header;
  dptr = rpak->rtp_data;
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
  do {
    uint16_t seq;
    seq = rpak->rtp_pak_seq;
    (iptr->m_vft->free_pak)(rpak);
    rpak = (iptr->m_vft->get_next_pak)(iptr->m_ifptr, NULL, 1);
    *pRpak = rpak;
    if (rpak == NULL) {
      return false;
    }
    if (rpak->rtp_pak_seq != seq + 1) {
      h264_message(LOG_ERR, h264rtp, "RTP sequence should be %u is %u", 
		   seq + 1, rpak->rtp_pak_seq);
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
  
  
static uint64_t start_next_frame (rtp_plugin_data_t *pifptr, 
				  uint8_t **buffer, 
				  uint32_t *buflen,
				  void **userdata)
{
  h264_rtp_data_t *iptr = (h264_rtp_data_t *)pifptr;
  uint64_t timetick;
  rtp_packet *rpak;
  bool have_m = false;
  uint32_t rtp_ts;
  uint64_t ntp_ts;
  bool have_first = true;
  
#ifdef H264_DEBUG
  h264_message(LOG_ERR, h264rtp, "start_next_frame");
#endif
  iptr->m_buffer_size = 0;
  rpak = (iptr->m_vft->get_next_pak)(iptr->m_ifptr, NULL, 1);
  rtp_ts = rpak->rtp_pak_ts;
  ntp_ts = rpak->pd.rtp_pd_timestamp;
  while (rpak != NULL && have_m == false) {
#if 0
    if (rpak->rtp_pak_ts != rtp_ts) {
      // start again
      rtp_pak_ts = rpak->rtp_ts;
      iptr->m_buffer_size = 0;
    }
#endif
    // process the various NAL types.
    uint8_t *dptr;
    dptr = rpak->rtp_data;
    uint8_t nal_type;
    nal_type = *dptr & 0x1f;
    uint16_t seq = rpak->rtp_pak_seq;
#ifdef H264_DEBUG
    h264_message(LOG_ERR, h264rtp, "nal %d", nal_type);
#endif
    if (nal_type >= H264_NAL_TYPE_NON_IDR_SLICE &&
	nal_type <= H264_NAL_TYPE_FILLER_DATA) {
      add_nal_to_buffer(iptr, rpak->rtp_data + 1, 
			rpak->rtp_data_len - 1,
			*rpak->rtp_data);
    } else if (nal_type == 24) {
      // stap - copy each access 
      uint32_t data_len = rpak->rtp_data_len - 1; // remove stap header
      dptr++;
      while (data_len > 0) {
	uint32_t len = (dptr[0] << 8) | dptr[1];
	dptr += 2;
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
	} else 
	  data_len -= (len + 2);
      }
    } else if (nal_type == 28) {
      // FUs
      if (process_fus(iptr, &rpak) == false) {
	// had an error
	if (rpak == NULL) {
	  return 0;
	}
	iptr->m_buffer_size = 0;
	seq = rpak->rtp_pak_seq - 1; // clear through the error
	have_first = false;
      }
      // don't forget to check for rpak == NULL here
    }
    // save off the last sequence number
    have_m = rpak->rtp_pak_m != 0;
    (iptr->m_vft->free_pak)(rpak);
    rpak = NULL;
    if (have_m == false) {
      rpak = (iptr->m_vft->get_next_pak)(iptr->m_ifptr, NULL, 1);
      if (rpak == NULL) {
	// forget about it
	return 0;
      }
      if (((seq + 1) & 0xffff) != rpak->rtp_pak_seq) {
	h264_message(LOG_ERR, h264rtp, "RTP sequence should be %u is %u", 
		   seq + 1, rpak->rtp_pak_seq);
	have_first = false;
      }
	
      if (have_first == false ||
	  rtp_ts != rpak->rtp_pak_ts) {
	if (have_first) {
	  h264_message(LOG_ERR, h264rtp, "RTP timestamp should be %u is %u (seq %u",
		       rtp_ts, rpak->rtp_pak_ts, rpak->rtp_pak_seq);
	}
	iptr->m_buffer_size = 0;
	rtp_ts = rpak->rtp_pak_ts;
	ntp_ts = rpak->pd.rtp_pd_timestamp;
	have_first = true;
      } 
    }
  }

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
  } else {
    timetick = 0;
  }
  // We're going to have to handle wrap better...
#ifdef DEBUG_H264
  h264_message(LOG_DEBUG, h264rtp, "start next frame %p %d ts %x "U64, 
	       *buffer, *buflen, rtp_ts, timetick);
#endif
  return (timetick);
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

static int have_no_data (rtp_plugin_data_t *pifptr)
{
  h264_rtp_data_t *iptr = (h264_rtp_data_t *)pifptr;
  rtp_packet *rpak, *firstpak;

  firstpak = rpak = (iptr->m_vft->get_next_pak)(iptr->m_ifptr, NULL, 0);
  //h264_message(LOG_DEBUG, h264rtp, "in have no data");

  if (firstpak == NULL) {
    //h264_message(LOG_DEBUG, h264rtp, "have no data1");
    return TRUE;
  }

  if (firstpak->rtp_pak_m != 0) {
    //h264_message(LOG_DEBUG, h264rtp, "have no data2");
    return FALSE;
  }

  do {
    rpak = (iptr->m_vft->get_next_pak)(iptr->m_ifptr, rpak, 0);
    if (rpak == NULL) {
      //h264_message(LOG_DEBUG, h264rtp, "have no data3");
      return TRUE;
    }

    if (rpak && rpak->rtp_pak_m != 0) {
      //h264_message(LOG_DEBUG, h264rtp, "have no data4 %d", FALSE);
      return FALSE;
    }
  } while (rpak != firstpak);
  return TRUE;
}

RTP_PLUGIN("h264", 
	   check,
	   h264_rtp_plugin_create,
	   h264_rtp_destroy,
	   start_next_frame, 
	   used_bytes_for_frame,
	   reset, 
	   flush_rtp_packets,
	   have_no_data,
	   NULL,
	   0);

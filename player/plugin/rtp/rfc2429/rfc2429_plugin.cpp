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

#include "rfc2429_plugin.h"
#include "rtp.h"
//#define DEBUG_R2429

#define r2429_message iptr->m_vft->log_msg
//static const char *r2429rtp="r2429rtp";

static rtp_check_return_t check (lib_message_func_t msg, 
				 format_list_t *fmt, 
				 uint8_t rtp_payload_type,
				 CConfigSet *pConfig)
{

  if (fmt == NULL)
    return RTP_PLUGIN_NO_MATCH;

  if (fmt->rtpmap != NULL) {
    if (strcasecmp(fmt->rtpmap->encode_name, "h263-1998") == 0 ||
	strcasecmp(fmt->rtpmap->encode_name, "h263-2000") == 0) {
      return RTP_PLUGIN_MATCH;
    }
  }

  return RTP_PLUGIN_NO_MATCH;
}

/*
 * Isma rtp bytestream has a potential set of headers at the beginning
 * of each rtp frame.  This can interleave frames in different packets
 */
rtp_plugin_data_t *r2429_rtp_plugin_create (format_list_t *media_fmt,
					   uint8_t rtp_payload_type, 
					   rtp_vft_t *vft,
					   void *ifptr)
{
  r2429_rtp_data_t *iptr;

  iptr = MALLOC_STRUCTURE(r2429_rtp_data_t);
  memset(iptr, 0, sizeof(*iptr));
  iptr->m_vft = vft;
  iptr->m_ifptr = ifptr;

#ifdef R2429_RTP_DUMP_OUTPUT_TO_FILE
  iptr->m_outfile = fopen("rtp.h263", "w");
#endif
  iptr->m_buffer = NULL;
  iptr->m_buffer_len = 0;
  iptr->m_buffer_len_max = 0;
  return (&iptr->plug);
}


static void r2429_rtp_destroy (rtp_plugin_data_t *pifptr)
{
  r2429_rtp_data_t *iptr = (r2429_rtp_data_t *)pifptr;

#ifdef ISMA_RTP_DUMP_OUTPUT_TO_FILE
  fclose(iptr->m_outfile);
#endif
  CHECK_AND_FREE(iptr->m_buffer);

  free(iptr);
}

static uint64_t start_next_frame (rtp_plugin_data_t *pifptr, 
				  uint8_t **buffer, 
				  uint32_t *buflen,
				  void **userdata)
{
  r2429_rtp_data_t *iptr = (r2429_rtp_data_t *)pifptr;
  uint64_t timetick;
  rtp_packet *rpak;
  uint32_t rtp_ts;
  uint64_t ntp_ts;
  uint8_t *dptr;
  uint32_t dlen;
  int pbit;
  int vbit;
  uint32_t plen;
  // we should have a start code here

  rpak = (iptr->m_vft->get_next_pak)(iptr->m_ifptr, 
				     NULL,
				     1);
  while (rpak != NULL) {
    iptr->m_buffer_len = 0;
    rtp_ts = rpak->rtp_pak_ts;
    ntp_ts = rpak->pd.rtp_pd_timestamp;
    do {
      dptr = rpak->rtp_data;
      dlen = rpak->rtp_data_len;
      pbit = (*dptr >> 2) & 1;
      vbit = (*dptr >> 1) & 1;
      plen = ((*dptr & 0x1) << 5) | ((dptr[1] >> 3) & 0x1f);

      dptr += 2;
      dlen -= 2; // remove the header

      dptr += plen;
      dlen -= plen;

      if (vbit) {
	dptr++;
	dlen--;
      }
      
      uint32_t toadd;
      // always make sure there are 3 bytes at end...
      toadd = dlen + 3;
      if (pbit) {
	toadd += 2;
      }

      if (toadd + iptr->m_buffer_len > iptr->m_buffer_len_max) {
	iptr->m_buffer_len_max += toadd + 1024;
	iptr->m_buffer = (uint8_t *)realloc(iptr->m_buffer, 
					    iptr->m_buffer_len_max);
      }

      if (pbit) {
	iptr->m_buffer[iptr->m_buffer_len] = 0;
	iptr->m_buffer[iptr->m_buffer_len + 1] = 0;
	iptr->m_buffer_len += 2;
      }
      
      memcpy(iptr->m_buffer + iptr->m_buffer_len, dptr, dlen);
      iptr->m_buffer_len += dlen;

      if (rpak->rtp_pak_m != 0) {
#if 0
	iptr->m_buffer[iptr->m_buffer_len] = 0;
	iptr->m_buffer[iptr->m_buffer_len + 1] = 0;
	iptr->m_buffer[iptr->m_buffer_len + 2] = 0x80;
	iptr->m_buffer_len += 3;
#endif
	timetick = 
	  iptr->m_vft->rtp_ts_to_msec(iptr->m_ifptr, 
				      rtp_ts,
				      ntp_ts,
				      0);
	*buffer = iptr->m_buffer;
	*buflen = iptr->m_buffer_len;
	return timetick;
      }
      // free and get the next one
      (iptr->m_vft->free_pak)(rpak);
      rpak = (iptr->m_vft->get_next_pak)(iptr->m_ifptr, 
					 NULL,
					 1);
    } while (rpak && rpak->rtp_pak_ts == rtp_ts);
  
  }
  return 0;

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

static int have_no_data (rtp_plugin_data_t *pifptr)
{
  r2429_rtp_data_t *iptr = (r2429_rtp_data_t *)pifptr;
  rtp_packet *rpak, *firstpak;

  firstpak = rpak = (iptr->m_vft->get_next_pak)(iptr->m_ifptr, NULL, 0);

  if (firstpak == NULL) return true;

  if (firstpak->rtp_pak_m != 0) return false;

  do {
    rpak = (iptr->m_vft->get_next_pak)(iptr->m_ifptr, rpak, 0);
    if (rpak == NULL) return true;

    if (rpak && rpak->rtp_pak_m != 0) return false;
  } while (rpak != firstpak);
  return true;
}

RTP_PLUGIN("rfc-2429", 
	   check,
	   r2429_rtp_plugin_create,
	   r2429_rtp_destroy,
	   start_next_frame, 
	   used_bytes_for_frame,
	   reset, 
	   flush_rtp_packets,
	   have_no_data,
	   NULL,
	   0);

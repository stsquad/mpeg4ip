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
 * Copyright (C) Cisco Systems Inc. 2001-2004  All Rights Reserved.
 * 
 * Contributor(s): 
 *              Bill May        wmay@cisco.com
 *              Alex Vanzella   alexv@cisco.com
 */

#include "isma_enc_video_plugin.h"
#include "rtp/rtp.h"

//#define DEBUG_ISMA_AAC

#define isma_message iptr->m_vft->log_msg

static rtp_check_return_t check (lib_message_func_t msg, 
				 format_list_t *fmt, 
				 uint8_t rtp_payload_type,
				 CConfigSet *pConfig)
{

  if (fmt == NULL || fmt->rtpmap_name == NULL)
    return RTP_PLUGIN_NO_MATCH;

  if ( (strcasecmp(fmt->rtpmap_name, "enc-mpeg4-generic") != 0) ) {
    return RTP_PLUGIN_NO_MATCH;
  }

  if ( (strcasecmp(fmt->media->media, "video") != 0 ) ) {
    return RTP_PLUGIN_NO_MATCH;
  }

  fmtp_parse_t *fmtp;
  fmtp = parse_fmtp_for_mpeg4(fmt->fmt_param, msg);
  if (fmtp == NULL) {
    return RTP_PLUGIN_NO_MATCH;
  }

  return RTP_PLUGIN_MATCH;

}

rtp_plugin_data_t *isma_enc_video_rtp_plugin_create (format_list_t *media_fmt,
					   uint8_t rtp_payload_type, 
					   rtp_vft_t *vft,
					   void *ifptr)
{
  isma_enc_video_rtp_data_t *iptr;

  iptr = MALLOC_STRUCTURE(isma_enc_video_rtp_data_t);
  if ( iptr == NULL )
     return NULL;
  memset(iptr, 0, sizeof(*iptr));
  iptr->m_vft = vft;
  iptr->m_ifptr = ifptr;

#ifdef R2429_RTP_DUMP_OUTPUT_TO_FILE
  iptr->m_outfile = fopen("rtp.h263", "w");
#endif
  iptr->m_buffer = NULL;
  iptr->m_buffer_len = 0;
  iptr->m_buffer_len_max = 0;

  if (strcasecmp(media_fmt->media->media, "video") == 0) {
    ismacrypInitSession(&(iptr->myEncSID), KeyTypeVideo);
  } 

  iptr->frameCount = 0;
  iptr->mp4SDP = parse_fmtp_for_mpeg4(media_fmt->fmt_param, iptr->m_vft->log_msg);

  return (&iptr->plug);
}


static void r2429_rtp_destroy (rtp_plugin_data_t *pifptr)
{
  isma_enc_video_rtp_data_t *iptr = (isma_enc_video_rtp_data_t *)pifptr;

#ifdef ISMA_RTP_DUMP_OUTPUT_TO_FILE
  fclose(iptr->m_outfile);
#endif
  CHECK_AND_FREE(iptr->m_buffer);

  free(iptr);
}

static bool start_next_frame (rtp_plugin_data_t *pifptr, 
			      uint8_t **buffer, 
			      uint32_t *buflen,
			      frame_timestamp_t *ts,
			      void **userdata)
{
  isma_enc_video_rtp_data_t *iptr = (isma_enc_video_rtp_data_t *)pifptr;
  uint64_t timetick;
  rtp_packet *rpak;
  uint32_t rtp_ts;
  uint64_t ntp_ts;
  uint8_t *dptr;
  uint32_t dlen;
  uint32_t IV;
  uint16_t seq;

  rpak = (iptr->m_vft->get_next_pak)(iptr->m_ifptr, 
				     NULL,
				     1);

  seq  =  rpak->rtp_pak_seq;  
  iptr->frameCount++;

  // awv notes: 1. video frames can be fragmented across several rtp packets.
  //            2. each fragment has the appropriate ismacryp streaming header  
  //               (i.e. IV in our case since that is all that is supported).
  //            3. important: the complete frame once all ismacryp streaming
  //               headers are removed still has the header from when it was
  //               encrypted. (i.e. IV again in our case)

  while (rpak != NULL) {

    iptr->m_buffer_len = 0;
    rtp_ts = rpak->rtp_pak_ts;
    ntp_ts = rpak->pd.rtp_pd_timestamp;
    do {
      dptr = rpak->rtp_data;
      dlen = rpak->rtp_data_len;

      if ( rpak->rtp_pak_seq - seq > 1 ) {
          return false;
      }
      else
          seq = rpak->rtp_pak_seq;
        

      // awv notes. 1. the first frame has stream config info prepended to the
      //               video frame. we need to remove this.
      //            2. each fragment of video frame has the mysterious 00 20
      //               header which looks like rfc2429. the very first packet
      //               with the stream config info does not have this 00 20.
      //            3. each fragment of video frame has the appropriate ismacryp
      //               header (= IV in our case since that is all that is supported)
      //            4. important: the complete frame once all ismacryp streaming
      //               headers are removed still has the header from when it was
      //               encrypted. (i.e IV again in our case). 

      // awv notes. remove the mysterious 00 20. 
      //            this is also done to the first fragment of 1st frame which is 
      //            the stream config info which does not have 00 20. this will be 
      //            accounted for later.
      dptr += 2;
      dlen -= 2; 

      // awv notes. remove the IV and save it. 
      //            this is also done to the first fragment of 1st frame which is 
      //            the stream config info which does not have IV. this will be 
      //            accounted for later.
      //            note that IV is saved for each fragment. in the case of the
      //            the first fragment of the first frame, it is gibberish but
      //            it will be set properly when the next fragment arrives. 
      IV = ntohl(*((uint32_t *)(dptr)));
      dptr += 4;
      dlen -= 4;

      uint32_t toadd;
      // always make sure there are 3 bytes at end... 
      // awv note: I have no idea why this is
      toadd = dlen + 3;
      if (toadd + iptr->m_buffer_len > iptr->m_buffer_len_max) {
	iptr->m_buffer_len_max += toadd + 1024;
	iptr->m_buffer = (uint8_t *)realloc(iptr->m_buffer, 
					    iptr->m_buffer_len_max);
      }

      memcpy(iptr->m_buffer + iptr->m_buffer_len, dptr, dlen);
      iptr->m_buffer_len += dlen;

      if (rpak->rtp_pak_m != 0) {
	timetick = 
	  iptr->m_vft->rtp_ts_to_msec(iptr->m_ifptr, 
				      rtp_ts,
				      ntp_ts,
				      0);
	*buffer = iptr->m_buffer;
	*buflen = iptr->m_buffer_len;
        if (iptr->frameCount == 1 ) { 
            // this is the first frame so it has the stream config info prepended.
            // we took off the two mystery bytes and the IV which the stream config
            // info does not have so we have to account for those. see notes above.
            // then we have to get rid of the IV which is still on the complete frame.
            *buffer = &iptr->m_buffer[iptr->mp4SDP->config_binary_len-2-4+4];
            *buflen -= (iptr->mp4SDP->config_binary_len-2-4+4);
        }
        else {
            *buffer = &iptr->m_buffer[4];
            *buflen -= 4;
        }

        ismacrypDecryptSampleRandomAccess(iptr->myEncSID, IV, *buflen, *buffer);
        //ismacrypDecryptSample(iptr->myEncSID, *buflen, *buffer);
	ts->msec_timestamp = timetick;
	ts->timestamp_is_pts = true;
	return true;
      }
      // free and get the next one
      (iptr->m_vft->free_pak)(rpak);
      rpak = (iptr->m_vft->get_next_pak)(iptr->m_ifptr, 
					 NULL,
					 1);
    } while (rpak && rpak->rtp_pak_ts == rtp_ts);
  
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
  isma_enc_video_rtp_data_t *iptr = (isma_enc_video_rtp_data_t *)pifptr;
  rtp_packet *rpak, *firstpak;

  firstpak = rpak = (iptr->m_vft->get_next_pak)(iptr->m_ifptr, NULL, 0);

  if (firstpak == NULL) return false;

  if (firstpak->rtp_pak_m != 0) return true;

  do {
    rpak = (iptr->m_vft->get_next_pak)(iptr->m_ifptr, rpak, 0);
    if (rpak == NULL) return false;

    if (rpak && rpak->rtp_pak_m != 0) return true;
  } while (rpak != firstpak);
  return false;
}

RTP_PLUGIN("enc-mpeg4-generic:video", 
	   check,
	   isma_enc_video_rtp_plugin_create,
	   r2429_rtp_destroy,
	   start_next_frame, 
	   used_bytes_for_frame,
	   reset, 
	   flush_rtp_packets,
	   have_frame,
	   NULL,
	   0);

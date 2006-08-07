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
 *		Bill May wmay@cisco.com
 */

#include "mp4live.h"
#include "text_encoder.h"
#include "mp4av.h"

static config_index_t CFG_TEXT_HREF_BASE_URL;

static SConfigVariable hrefEncoderVariables[] = {
  CONFIG_STRING(CFG_TEXT_HREF_BASE_URL, "hrefBaseUrl", NULL),
};

GUI_STRING(gui_href_base, CFG_TEXT_HREF_BASE_URL, "Base Url");
DECLARE_TABLE(href_gui_options) = {
  TABLE_GUI(gui_href_base),
};

DECLARE_TABLE_FUNC(href_gui_options);

void AddTextProfileEncoderVariables (CTextProfile *pConfig)
{
  pConfig->AddConfigVariables(hrefEncoderVariables, 
			      NUM_ELEMENTS_IN_ARRAY(hrefEncoderVariables));
}

//#define DEBUG_TEXT
text_encoder_table_t text_encoder_table[] = {
  { "Plain Text",
    TEXT_ENCODING_PLAIN,
    NULL,
  },
  { "ISMA Href",
    TEXT_ENCODING_HREF,
    TABLE_FUNC(href_gui_options),
  }
};

uint32_t text_encoder_table_size = NUM_ELEMENTS_IN_ARRAY(text_encoder_table);

class CPlainTextEncoder : public CTextEncoder 
{
 public:
  CPlainTextEncoder(CTextProfile *profile, uint16_t mtu, CTextEncoder *next, 
		    bool realTime = true) :
    CTextEncoder(profile, mtu, next, realTime) { };
  bool Init(void) { 
    m_encodedFrame = NULL;
    return true; 
  };
  MediaType GetFrameType(void) { return PLAINTEXTFRAME; };
 protected:
  void StopEncoder(void) {
    CHECK_AND_FREE(m_encodedFrame);
  };
  bool EncodeFrame (const char *fptr) {
    CHECK_AND_FREE(m_encodedFrame);
    m_encodedFrame = strdup(fptr);
    return true;
  };
  bool GetEncodedFrame(void **ppBuffer, 
		       uint32_t *pBufferLength) {
    if (m_encodedFrame == NULL) return false;
    *pBufferLength = strlen(m_encodedFrame) + 1;
#ifdef DEBUG_TEXT
    debug_message("encode len %u", *pBufferLength);
#endif
    *ppBuffer = strdup(m_encodedFrame);
    return true;
  };
  char *m_encodedFrame;
};

class CHrefTextEncoder : public CTextEncoder
{
 public:
  CHrefTextEncoder(CTextProfile *profile, uint16_t mtu, CTextEncoder *next, 
		    bool realTime = true) :
    CTextEncoder(profile, mtu, next, realTime) { 
  };
  bool Init(void) { 
    m_encodedFrame = NULL;
    m_base_url = Profile()->GetStringValue(CFG_TEXT_HREF_BASE_URL);
    m_base_url_len = strlen(m_base_url);
    return true; 
  };
  MediaType GetFrameType(void) { return HREFTEXTFRAME; };
 protected:
  void StopEncoder(void) {
    CHECK_AND_FREE(m_encodedFrame);
  };
  void chomp (void) {
    char *end = m_encodedFrame + strlen(m_encodedFrame);
    end--;
    while (isspace(*end)) {
      *end = '\0';
      end--;
    }
  }
  bool EncodeFrame (const char *fptr) {
    ADV_SPACE(fptr);
    CHECK_AND_FREE(m_encodedFrame);
    if (*fptr == 'A' || *fptr == '<') {
      // we have an already formatted href
      const char *check = fptr + 1;
      if (*check == '<') {
	check++;
      }
      if (strncmp(check, m_base_url, m_base_url_len) == 0) {
	check += m_base_url_len;
	m_encodedFrame = (char *)malloc(strlen(check) + 
					*fptr == 'A' ? 2 : 1);
	char *copyto = m_encodedFrame;
	if (*fptr == 'A') {
	  *copyto++ = 'A';
	}
	*copyto++ = '<';
	strcpy(copyto, check);
	debug_message("%s", m_encodedFrame);
      } else {
	m_encodedFrame = strdup(fptr);
      }
      chomp();
    } else {
      // we need to add <> and maybe an A
      uint32_t size = strlen(fptr) + 1; // add \0 at end
      if (strncmp(fptr, m_base_url, m_base_url_len) == 0) {
	fptr += m_base_url_len;
      }
      debug_message("string \"%s\"", fptr);
      size += 2; // add <>
      if (Profile()->GetBoolValue(CFG_TEXT_HREF_MAKE_AUTOMATIC)) size++;
      m_encodedFrame = (char *)malloc(size);
      char *write = m_encodedFrame;
      if (Profile()->GetBoolValue(CFG_TEXT_HREF_MAKE_AUTOMATIC)) {
	*write++ = 'A';
      }
      *write++ = '<';
      *write = '\0';
      strcat(write, fptr);
      debug_message("before chomp \"%s\"", m_encodedFrame);
      chomp();
      strcat(write, ">");
      debug_message("\"%s\"", m_encodedFrame);
    }
    m_encodedFrameLen = strlen(m_encodedFrame) + 1;
    return true;
  };
  bool GetEncodedFrame(void **ppBuffer, 
		       uint32_t *pBufferLength) {
    if (m_encodedFrame == NULL) return false;
    *pBufferLength = m_encodedFrameLen;
#ifdef DEBUG_TEXT
    debug_message("encode len %u", *pBufferLength);
#endif
    *ppBuffer = strdup(m_encodedFrame);
    return true;
  };
  char *m_encodedFrame;
  uint32_t m_encodedFrameLen;
  const char *m_base_url;
  uint32_t m_base_url_len;
};

CTextEncoder* TextEncoderCreate(CTextProfile *tp, 
				uint16_t mtu,
				CTextEncoder *next, 
				bool realTime)
{
  const char *encoding = tp->GetStringValue(CFG_TEXT_ENCODING);
  if (strcmp(encoding, TEXT_ENCODING_PLAIN) == 0) {
    return new CPlainTextEncoder(tp, mtu, next, realTime);
  } else if (strcmp(encoding, TEXT_ENCODING_HREF) == 0) {
    return new CHrefTextEncoder(tp, mtu, next, realTime);
  }

  return NULL;
}

MediaType get_text_mp4_fileinfo (CTextProfile *pConfig,
				 const char **url)
{
  const char *encoding = pConfig->GetStringValue(CFG_TEXT_ENCODING);
  *url = NULL;
  if (strcmp(encoding, TEXT_ENCODING_HREF) == 0) {
    *url = pConfig->GetStringValue(CFG_TEXT_HREF_BASE_URL);
    return HREFTEXTFRAME;
  }

  return UNDEFINEDFRAME;
}

media_desc_t *create_text_sdp (CTextProfile *pConfig)
{
  media_desc_t *sdpMedia;
  format_list_t *sdpMediaFormat;
  sdpMedia = MALLOC_STRUCTURE(media_desc_t);
  memset(sdpMedia, 0, sizeof(*sdpMedia));

  sdpMediaFormat = MALLOC_STRUCTURE(format_list_t);
  memset(sdpMediaFormat, 0, sizeof(*sdpMediaFormat));

  sdpMediaFormat->media = sdpMedia;
  sdpMediaFormat->fmt = strdup("98");

  sdpMediaFormat->rtpmap_clock_rate = 90000;
  sdpMedia->fmt_list = sdpMediaFormat;

  if (strcmp(pConfig->GetStringValue(CFG_TEXT_ENCODING), 
	     TEXT_ENCODING_PLAIN) == 0) {
    // text
    sdpMediaFormat->rtpmap_name = strdup("x-plain-text");
    sdpMedia->media = strdup("application");
  } else {
    sdpMediaFormat->rtpmap_name = strdup("X-HREF");
    sdpMedia->media = strdup("control");
    const char *base_url = pConfig->GetStringValue(CFG_TEXT_HREF_BASE_URL);
    if (base_url != NULL) {
      char *temp = (char *)malloc(strlen("base_url=") + strlen(base_url) + 1);
      sprintf(temp,"base_url=%s", base_url);
      sdpMediaFormat->fmt_param = temp;
    }
  }
  
  return sdpMedia;
}

void create_mp4_text_hint_track (CTextProfile *pConfig, 
				 MP4FileHandle mp4file,
				 MP4TrackId trackId,
				 uint16_t mtu)
{
  if (strcasecmp(pConfig->GetStringValue(CFG_TEXT_ENCODING), TEXT_ENCODING_HREF) == 0) {
    HrefHinter(mp4file, trackId, mtu);
  }
}

static void SendPlainText (CMediaFrame *pFrame,
			   CRtpDestination *list,
			   uint32_t rtpTimestamp,
			   uint16_t mtu)
{
  CRtpDestination *rdptr;
  uint32_t bytesToSend = pFrame->GetDataLength();
  struct iovec iov;
  uint8_t *pData = (uint8_t *)pFrame->GetData();
  
  while (bytesToSend) {
    uint32_t payloadLength;
    bool lastPacket;
    if (bytesToSend < mtu) {
      payloadLength = bytesToSend;
      lastPacket = true;
    } else {
      payloadLength = mtu;
      lastPacket = false;
    }

    iov.iov_base = pData;
    iov.iov_len = payloadLength;

    rdptr = list;
    while (rdptr != NULL) {
      int rc = rdptr->send_iov(&iov, 1, rtpTimestamp, lastPacket);
      if (rc != (int)payloadLength) {
	error_message("text send send_iov error - returned %d %d", 
		      rc, payloadLength);
      }
      rdptr = rdptr->get_next();
    }

    pData += payloadLength;
    bytesToSend -= payloadLength;
  }
  if (pFrame->RemoveReference()) 
    delete pFrame;
}

static void SendHrefText (CMediaFrame *pFrame,
			  CRtpDestination *list,
			  uint32_t rtpTimestamp,
			  uint16_t mtu)
{
  CRtpDestination *rdptr;
  uint32_t bytesToSend = pFrame->GetDataLength();
  struct iovec iov[2];
  uint8_t *pData = (uint8_t *)pFrame->GetData();
  
  if (pFrame->GetDataLength() + 4 > mtu) {
    error_message("Href url is too long - not transmitting \"%s\"", 
		  (char *)pData);
    return;
  }
  
  uint8_t header[4];
  header[0] = 0;
  header[1] = 1;
  header[2] = bytesToSend >> 8;
  header[3] = bytesToSend & 0xff;

  iov[0].iov_base = header;
  iov[0].iov_len = 4;
  iov[1].iov_base = pData;
  iov[1].iov_len = bytesToSend;

  rdptr = list;
  while (rdptr != NULL) {
    int rc = rdptr->send_iov(iov, 2, rtpTimestamp, true);
    rc -= 4; 
    if (rc != (int)bytesToSend) {
      error_message("text send send_iov error - returned %d %d", 
		    rc, bytesToSend);
    }
    rdptr = rdptr->get_next();
  }

  if (pFrame->RemoveReference()) 
    delete pFrame;
}

rtp_transmitter_f GetTextRtpTransmitRoutine (CTextProfile *pConfig, 
					     MediaType *pType, 
					     uint8_t *pPayload)
{
  *pPayload = 98;
  if (strcmp(pConfig->GetStringValue(CFG_TEXT_ENCODING),
	     TEXT_ENCODING_PLAIN)  == 0) {
    *pType = PLAINTEXTFRAME;
    return SendPlainText;
  } else if (strcmp(pConfig->GetStringValue(CFG_TEXT_ENCODING),
		    TEXT_ENCODING_HREF) == 0) {
    *pType = HREFTEXTFRAME;
    return SendHrefText;
  }
  return NULL;
}

CTextEncoder::CTextEncoder (CTextProfile *tp,
			    uint16_t mtu,
			    CTextEncoder *next,
			    bool realTime) :
  CMediaCodec(tp, mtu, next, realTime)
{
  
}

int CTextEncoder::ThreadMain (void)
{
  CMsg* pMsg;
  bool stop = false;
  
  Init();

  m_textDstType = GetFrameType();
  double temp = Profile()->GetFloatValue(CFG_TEXT_REPEAT_TIME_SECS);
  temp *= 1000.0;
  uint32_t wait_time = (uint32_t)temp;

  while (stop == false) {
    int rc = SDL_SemWaitTimeout(m_myMsgQueueSemaphore, wait_time);
    if (rc == 0) {
      pMsg = m_myMsgQueue.get_message();
      if (pMsg != NULL) {
	switch (pMsg->get_value()) {
	case MSG_NODE_STOP_THREAD:
	  debug_message("text %s stop received", 
			Profile()->GetName());
	  DoStopText();
	  stop = true;
	  break;
	case MSG_NODE_START:
	  // DoStartTransmit();  Anything ?
	  break;
	case MSG_NODE_STOP:
	  DoStopText();
	  break;
	case MSG_SINK_FRAME: {
	  uint32_t dontcare;
	  CMediaFrame *mf = (CMediaFrame*)pMsg->get_message(dontcare);
	  if (m_stop_thread == false)
	    ProcessTextFrame(mf);
	  if (mf->RemoveReference()) {
	    delete mf;
	  }
	  break;
	}
	}
      
	delete pMsg;
      } 
    } else if (rc == SDL_MUTEX_TIMEDOUT) {
      SendFrame(GetTimestamp());
    }
  }
  while ((pMsg = m_myMsgQueue.get_message()) != NULL) {
    if (pMsg->get_value() == MSG_SINK_FRAME) {
      uint32_t dontcare;
      CMediaFrame *mf = (CMediaFrame*)pMsg->get_message(dontcare);
      if (mf->RemoveReference()) {
	delete mf;
      }
    }
    delete pMsg;
  }
  debug_message("text encoder %s exit", Profile()->GetName());
  return 0;
}

void CTextEncoder::ProcessTextFrame (CMediaFrame *pFrame)
{
  const char *fptr = (const char *)pFrame->GetData();

  EncodeFrame(fptr);
  
  SendFrame(pFrame->GetTimestamp());
}

void CTextEncoder::SendFrame (Timestamp t)
{
  CMediaFrame *mf;
  void *buf;
  uint32_t buflen;

  if (GetEncodedFrame(&buf, &buflen) == false) {
    return;
  }

  //debug_message("encode %p", buf);
  mf = new CMediaFrame(m_textDstType,
		       buf, 
		       buflen,
		       t);
#ifdef DEBUG_TEXT
  debug_message("frame len %u", buflen);
#endif
  ForwardFrame(mf);
}
void CTextEncoder::DoStopText (void)
{

}

void CTextEncoder::AddRtpDestination (CMediaStream *stream,
				       bool disable_ts_offset, 
				       uint16_t max_ttl,
				       in_port_t srcPort)
{
  mp4live_rtp_params_t *mrtp;

  mrtp = MALLOC_STRUCTURE(mp4live_rtp_params_t);
  rtp_default_params(&mrtp->rtp_params);
  mrtp->rtp_params.rtp_addr = stream->GetStringValue(STREAM_TEXT_DEST_ADDR);
  mrtp->rtp_params.rtp_rx_port = srcPort;
  mrtp->rtp_params.rtp_tx_port = stream->GetIntegerValue(STREAM_TEXT_DEST_PORT);
  mrtp->rtp_params.rtp_ttl = max_ttl;
  mrtp->rtp_params.transmit_initial_rtcp = 1;
  mrtp->rtp_params.rtcp_addr = stream->GetStringValue(STREAM_TEXT_RTCP_DEST_ADDR);
  mrtp->rtp_params.rtcp_tx_port = stream->GetIntegerValue(STREAM_TEXT_RTCP_DEST_PORT);

  mrtp->use_srtp = stream->GetBoolValue(STREAM_TEXT_USE_SRTP);
  mrtp->srtp_params.enc_algo = 
    (srtp_enc_algos_t)stream->GetIntegerValue(STREAM_TEXT_SRTP_ENC_ALGO);
  mrtp->srtp_params.auth_algo = 
    (srtp_auth_algos_t)stream->GetIntegerValue(STREAM_TEXT_SRTP_AUTH_ALGO);
  mrtp->srtp_params.tx_key = stream->m_text_key;
  mrtp->srtp_params.tx_salt = stream->m_text_salt;
  mrtp->srtp_params.rx_key = stream->m_text_key;
  mrtp->srtp_params.rx_salt = stream->m_text_salt;
  mrtp->srtp_params.rtp_enc = stream->GetBoolValue(STREAM_TEXT_SRTP_RTP_ENC);
  mrtp->srtp_params.rtp_auth = stream->GetBoolValue(STREAM_TEXT_SRTP_RTP_AUTH);
  mrtp->srtp_params.rtcp_enc = stream->GetBoolValue(STREAM_TEXT_SRTP_RTCP_ENC);

  AddRtpDestInt(disable_ts_offset, mrtp);
}

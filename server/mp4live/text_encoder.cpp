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

text_encoder_table_t text_encoder_table[] = {
  { "Plain Text",
    TEXT_ENCODING_PLAIN,
  },
  { "ISMA Href",
    TEXT_ENCODING_HREF,
  }
};

uint32_t text_encoder_table_size = NUM_ELEMENTS_IN_ARRAY(text_encoder_table);

class CPlainTextEncoder : public CTextEncoder {
 public:
  CPlainTextEncoder(CTextProfile *profile, CTextEncoder *next, 
		    bool realTime = true) :
    CTextEncoder(profile, next, realTime) { };
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
    debug_message("encode len %u", *pBufferLength);
    *ppBuffer = strdup(m_encodedFrame);
    return true;
  };
  char *m_encodedFrame;
};

CTextEncoder* TextEncoderCreate(CTextProfile *tp, 
				CTextEncoder *next, 
				bool realTime)
{
  const char *encoding = tp->GetStringValue(CFG_TEXT_ENCODING);
  if (strcmp(encoding, TEXT_ENCODING_PLAIN) == 0) {
    return new CPlainTextEncoder(tp, next, realTime);
  }

  return NULL;
}

MediaType get_text_mp4_fileinfo (CTextProfile *pConfig,
				 uint8_t *textProfile,
				 uint8_t **textConfig,
				 uint32_t *textConfigLen)
{
  return UNDEFINEDFRAME;
}

media_desc_t *create_text_sdp (CTextProfile *pConfig)
{
  return NULL;
}

void create_mp4_text_hint_track (CTextProfile *pConfig, 
				 MP4FileHandle mp4file,
				 MP4TrackId trackId,
				 uint16_t mtu)
{

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
      rc -= sizeof(rtp_packet_header);
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

rtp_transmitter_f GetTextRtpTransmitRoutine (CTextProfile *pConfig, 
					     MediaType *pType, 
					     uint8_t *pPayload)
{
  *pPayload = 98;
  if (strcmp(pConfig->GetStringValue(CFG_TEXT_ENCODING),
	     TEXT_ENCODING_PLAIN)  == 0) {
    *pType = PLAINTEXTFRAME;
    return SendPlainText;
  }
  return NULL;
}

CTextEncoder::CTextEncoder (CTextProfile *tp,
			    CTextEncoder *next,
			    bool realTime) :
  CMediaCodec(tp, next, realTime)
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

  mf = new CMediaFrame(m_textDstType,
		       buf, 
		       buflen,
		       t);
  debug_message("frame len %u", buflen);
  ForwardFrame(mf);
}
void CTextEncoder::DoStopText (void)
{

}

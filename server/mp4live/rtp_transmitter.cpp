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
 * Copyright (C) Cisco Systems Inc. 2000, 2001.  All Rights Reserved.
 * 
 * Contributor(s): 
 *		Dave Mackie		dmackie@cisco.com
 *		Bill May 		wmay@cisco.com
 *		Peter Maersk-Moller	peter@maersk-moller.net
 */

#include "mp4live.h"
#include "rtp_transmitter.h"
#include "encoder-h261.h"
#include "audio_encoder.h"
#include "video_encoder.h"
#include "text_encoder.h"
#include "liblibsrtp.h"

//#define RTP_DEBUG 1
//#define DEBUG_WRAP_TS 1
//#define DEBUG_SEND 1
CRtpTransmitter::CRtpTransmitter (uint16_t mtu, bool disable_ts_offset) : CMediaSink()
{
  //SetConfig(pConfig);

  m_destListMutex = SDL_CreateMutex();
  m_rtpDestination = NULL;
  m_haveStartTimestamp = false;

  m_mtu = m_original_mtu = mtu;
#ifdef DEBUG_WRAP_TS
  m_rtpTimestampOffset = 0xffff0000;
#else 
  if (disable_ts_offset) {
    m_rtpTimestampOffset = 0;
  } else {
    m_rtpTimestampOffset = random();
  }
#endif
  
}

CRtpTransmitter::~CRtpTransmitter (void)
{
  DoStopTransmit();
  SDL_DestroyMutex(m_destListMutex);
}

void CRtpTransmitter::DoAddRtpDestinationToQueue (CRtpDestination *dest)
{
  CRtpDestination *p;
  SDL_LockMutex(m_destListMutex);
  if (m_rtpDestination == NULL) {
    m_rtpDestination = dest;
  } else {
    p = m_rtpDestination;
    while (p->get_next() != NULL) {
      p = p->get_next();
    }
    p->set_next(dest);
  }
  SDL_UnlockMutex(m_destListMutex);
}

void CRtpTransmitter::AddRtpDestination (struct rtp *rtp)
{
  CRtpDestination *dest;

  dest = new CRtpDestination(rtp, m_payloadNumber);
  
  m_mtu = m_original_mtu - rtp_get_mtu_adjustment(rtp);
  DoAddRtpDestinationToQueue(dest);
}

  
void CRtpTransmitter::AddRtpDestination (mp4live_rtp_params_t *rtp_params)
{
  CRtpDestination *dest;
  const char *destAddress = rtp_params->rtp_params.rtp_addr;
  in_port_t destPort = rtp_params->rtp_params.rtp_tx_port;
  in_port_t srcPort = rtp_params->rtp_params.rtp_rx_port;
  uint8_t max_ttl = rtp_params->rtp_params.rtp_ttl;

  rtp_params->rtp_params.rtcp_bandwidth = DEFAULT_RTCP_BW;
  debug_message("Creating rtp destination %s %u %u: ttl %u", 
		destAddress, destPort, srcPort, max_ttl);
  /* wmay - comment out for now - might have to revisit for SSM
     if (srcPort == 0) srcPort = destPort;
   */

  dest = m_rtpDestination;
  SDL_LockMutex(m_destListMutex);
  while (dest != NULL) {
    if (dest->Matches(destAddress, destPort)) {
      if (srcPort != 0 && srcPort != dest->get_source_port()) {
	error_message("Try to create RTP destination to %s:%u with different source ports %u %u",
		      destAddress, destPort, srcPort, dest->get_source_port());
      } else {
	dest->add_reference();
	debug_message("adding reference");
      }
      SDL_UnlockMutex(m_destListMutex);
      return;
    }
    dest = dest->get_next();
  }
  SDL_UnlockMutex(m_destListMutex);

  dest = new CRtpDestination(rtp_params,
			     m_payloadNumber);

  if (rtp_params->use_srtp && rtp_params->srtp_params.rtp_auth) {
    uint16_t diff = m_original_mtu - m_mtu;
    if (diff > rtp_params->auth_len) {
      m_mtu = m_original_mtu - rtp_params->auth_len;
    }
  }

  DoAddRtpDestinationToQueue(dest);
}


int CRtpTransmitter::ThreadMain(void) 
{
  CMsg* pMsg;
  bool stop = false;
  uint32_t len;
  while (stop == false && SDL_SemWait(m_myMsgQueueSemaphore) == 0) {
    pMsg = m_myMsgQueue.get_message();
    if (pMsg != NULL) {
      switch (pMsg->get_value()) {
      case MSG_NODE_STOP_THREAD:
	DoStopTransmit();
	stop = true;
	break;
      case MSG_NODE_START:
	DoStartTransmit();
	break;
      case MSG_NODE_STOP:
	DoStopTransmit();
	break;
      case MSG_SINK_FRAME:
	uint32_t dontcare;
	DoSendFrame((CMediaFrame*)pMsg->get_message(dontcare));
	break;
      case MSG_RTP_DEST_START:
	DoStartRtpDestination((const char *)pMsg->get_message(len), 
			      pMsg->get_param());
	break;
      case MSG_RTP_DEST_STOP:
	DoStopRtpDestination((const char *)pMsg->get_message(len), 
			     pMsg->get_param());
	break;
      }
      
      delete pMsg;
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
  
  return 0;
}

void CRtpTransmitter::DoStartTransmit()
{
  if (m_sink) {
    return;
  }

  CRtpDestination *dest = m_rtpDestination;
  while (dest != NULL) {
    dest->start();
    dest = dest->get_next();
    m_sink = true;
  }
}

void CRtpTransmitter::DoStopTransmit()
{
	if (!m_sink) {
		return;
	}

	while (m_rtpDestination != NULL) {
	  CRtpDestination *dest = m_rtpDestination;
	  m_rtpDestination = dest->get_next();
	  delete dest;
	}

	m_sink = false;
}
void CAudioRtpTransmitter::DoStopTransmit(void)
{
	// send any pending frames
	SendQueuedAudioFrames();
	CRtpTransmitter::DoStopTransmit();

}

void CAudioRtpTransmitter::DoSendFrame(CMediaFrame* pFrame)
{
	if (pFrame == NULL) {
		return;
	}
	if (!m_sink || m_rtpDestination == NULL) {
	  if (pFrame->RemoveReference())
		delete pFrame;
	  return;
	}

  if (pFrame->GetType() == m_frameType) {
    SendAudioFrame(pFrame);
  } else {
    if (pFrame->RemoveReference())
      delete pFrame;
  }
}

void CVideoRtpTransmitter::DoSendFrame (CMediaFrame *pFrame)
{
  if (pFrame == NULL) {
    return;
  }
  if (!m_sink || m_rtpDestination == NULL) {
    debug_message("video frame, sink %d dest %p", m_sink, 
		  m_rtpDestination);
    if (pFrame->RemoveReference())
      delete pFrame;
    return;
  }

  if (pFrame->GetType() == m_frameType) {
	  // Note - the below changed from the DTS to the PTS - this
	  // is required for b-frames, or mpeg2
    //debug_message("video rtp has frame");
    u_int32_t rtpTimestamp =
      TimestampToRtp(pFrame->GetPtsTimestamp());
    u_int64_t ntpTimestamp = 
      TimestampToNtp(pFrame->GetTimestamp());
	
#ifdef DEBUG_SEND  
    debug_message("V ts "U64" rtp %u ntp %u.%u",
		pFrame->GetTimestamp(),
		rtpTimestamp,
		(u_int32_t)(ntpTimestamp >> 32),
		(u_int32_t)ntpTimestamp);
#endif
    CRtpDestination *rdptr;
	  
    rdptr = m_rtpDestination;
    while (rdptr != NULL) {
      rdptr->send_rtcp(rtpTimestamp, ntpTimestamp);
      rdptr = rdptr->get_next();
    }
    (m_videoSendFunc)(pFrame, m_rtpDestination, rtpTimestamp, 
		      m_mtu);
  } else {
    // not the fame we want - okay for previews...
    if (pFrame->RemoveReference())
      delete pFrame;
  }
}

void CTextRtpTransmitter::DoSendFrame (CMediaFrame *pFrame)
{
  if (pFrame == NULL) {
    return;
  }
  if (!m_sink || m_rtpDestination == NULL) {
    debug_message("text frame, sink %d dest %p", m_sink, 
		  m_rtpDestination);
    if (pFrame->RemoveReference())
      delete pFrame;
    return;
  }
  if (pFrame->GetType() == m_frameType) {
	  // Note - the below changed from the DTS to the PTS - this
	  // is required for b-frames, or mpeg2
    //debug_message("video rtp has frame");
    u_int32_t rtpTimestamp =
      TimestampToRtp(pFrame->GetTimestamp());
    u_int64_t ntpTimestamp = 
      TimestampToNtp(pFrame->GetTimestamp());
	  
    CRtpDestination *rdptr;
	  
    rdptr = m_rtpDestination;
    while (rdptr != NULL) {
      rdptr->send_rtcp(rtpTimestamp, ntpTimestamp);
      rdptr = rdptr->get_next();
    }
    (m_textSendFunc)(pFrame, m_rtpDestination, rtpTimestamp, 
		      m_mtu);
  } else {
    // not the fame we want - okay for previews...
    if (pFrame->RemoveReference())
      delete pFrame;
  }
}

// void CRtpTransmitter::SendAudioFrame(CMediaFrame* pFrame)
void CAudioRtpTransmitter::OldSendAudioFrame(CMediaFrame* pFrame)
{
  //  debug_message("RTP Timestamp %u\tFrame Duration "U64,
  //                AudioTimestampToRtp(pFrame->GetTimestamp())
  //                , pFrame->GetDuration());

	// first compute how much data we'll have 
	// after adding this audio frame
	
  	//u_int16_t newAudioQueueSize = m_audioQueueSize;

        //	if (m_audioQueueCount == 0) {
        //		newAudioQueueSize += 
        //			m_audioPayloadBytesPerPacket;
        //	} else {
  //uint32_t ourTs = AudioTimestampToRtp(pFrame->GetTimestamp());
  //int32_t diff = ourTs - m_nextAudioRtpTimestamp;

  //if (diff > 1) {
  //  debug_message("Timestamp not consecutive error - timestamp "U64" should be %u is %u", 
  //                pFrame->GetTimestamp(),
  //                m_nextAudioRtpTimestamp,
  //                ourTs);
	    //***   SendQueuedAudioFrames();
            //	    newAudioQueueSize = m_audioQueueSize +=
            //	      m_audioPayloadBytesPerPacket;
  //}

          //	}

  /*
	// save the next timestamp.
	if (m_audioTimeScale == pFrame->GetDurationScale()) {
	  m_nextAudioRtpTimestamp = 
	    AudioTimestampToRtp(pFrame->GetTimestamp()) + 
	    pFrame->GetDuration();
	} else {
	  m_nextAudioRtpTimestamp = 
	    AudioTimestampToRtp(pFrame->GetTimestamp()) + 
	    pFrame->ConvertDuration(m_audioTimeScale);
	}
  */
        uint16_t newAudioQueueSize = m_audioQueueSize
          + m_audioPayloadBytesPerPacket
          + m_audioPayloadBytesPerFrame
          + pFrame->GetDataLength();

	// if new queue size exceeds the RTP payload
	if (newAudioQueueSize > m_mtu) {

		// send anything that's pending
		SendQueuedAudioFrames();

		// adjust new queue size
		newAudioQueueSize =
			m_audioPayloadBytesPerPacket
			+ m_audioPayloadBytesPerFrame
			+ pFrame->GetDataLength();
	}

	// if new data will fit into RTP payload
	if (newAudioQueueSize <= m_mtu) {

		// add it to the queue
		m_audioQueue[m_audioQueueCount++] = pFrame;
		m_audioQueueSize = newAudioQueueSize;
		// if we fill the queue, (latency bound)
		if (m_audioQueueCount == m_audioQueueMaxCount) {
			// send the pending data
			SendQueuedAudioFrames();
		}
	} else {
		// highly unusual case 
		// we need to fragment audio frame
		// over multiple packets
		SendAudioJumboFrame(pFrame);
	}
}

/*
 * #define DROP_IT	1
 * #define SEND_FIRST	2
 * #define IS_JUMBO	4
 * #define SEND_NOW	8
 */
void CAudioRtpTransmitter::SendAudioFrame(CMediaFrame* pFrame)
{
#ifdef RTP_DEBUG
  debug_message("sendaudioframe "U64, pFrame->GetTimestamp());
#endif
	if (m_audio_queue_frame != NULL) {

		// Get status for what we should do with the next frame
		int check_frame = m_audio_queue_frame(&m_frameno,
			pFrame->GetDataLength(),
			m_audioQueueCount, m_audioQueueSize,
						      (u_int32_t) m_mtu);

#ifdef RTP_DEBUG
		fprintf(stderr,"Check Frame :");
		if (check_frame & DROP_IT) fprintf(stderr," DROP");
		if (check_frame & SEND_FIRST) fprintf(stderr," FIRST");
		if (check_frame & IS_JUMBO) fprintf(stderr," JUMBO");
		if (check_frame == 0 || !((check_frame & DROP_IT) ||
		  (check_frame & IS_JUMBO))) fprintf(stderr," QUEUE");
		if (check_frame & SEND_NOW) fprintf(stderr," NOW");
		fprintf(stderr,"\n");
#endif	// RTP_DEBUG

		// Check and see if we should send it or drop it
		if (check_frame & DROP_IT)
			return;

		// Check and see if we should send the queue first
		if (m_audioQueueCount > 0 && (check_frame & SEND_FIRST)) {

			// send anything that's pending
			SendQueuedAudioFrames();
		}

		// See if we have a jumbo frame
		if (check_frame & IS_JUMBO) {
			SendAudioJumboFrame(pFrame);
			return;
		}

		// Add frame to queue
		m_audioQueue[m_audioQueueCount++] = pFrame;
		m_audioQueueSize += pFrame->GetDataLength();

		// and check to see if we want ot send it now
		if (check_frame & SEND_NOW) {

			// send anything that's pending
			SendQueuedAudioFrames();
		}

	} else {

		// B & D 's original CRtpTransmitter::SendAudioFrame
		OldSendAudioFrame(pFrame);
	}
}

void CAudioRtpTransmitter::SendQueuedAudioFrames(void)
{
	struct iovec iov[m_audioiovMaxCount + 1];
	int ix;
	int i = 0;
	CRtpDestination *rdptr;

	if (m_audioQueueCount == 0) {
		return;
	}

	u_int32_t rtpTimestamp =
		TimestampToRtp(m_audioQueue[0]->GetTimestamp());
	u_int64_t ntpTimestamp =
		TimestampToNtp(m_audioQueue[0]->GetTimestamp());

#ifdef DEBUG_SEND
	debug_message("A ts "U64" rtp %u ntp %u.%u",
		m_audioQueue[0]->GetTimestamp(),
		rtpTimestamp,
		(u_int32_t)(ntpTimestamp >> 32),
		(u_int32_t)ntpTimestamp);
#endif
	rdptr = m_rtpDestination;
	while (rdptr != NULL) {
	  rdptr->send_rtcp(rtpTimestamp, ntpTimestamp);
	  rdptr = rdptr->get_next();
	}

	// moved this from below
	// See if we need to add the header.
	int iov_start = 1;
	uint iov_add = 0;
	bool mbit = 1;
	if (m_audio_set_rtp_payload != NULL) {
		iov_start = 0;
		iov_add = m_audio_set_rtp_payload(m_audioQueue,
						  m_audioQueueCount,
						  iov,
						  m_audio_rtp_userdata,
						  &mbit);
	} else {
		// audio payload data
		for (i = 0; i < m_audioQueueCount; i++) {
	  	iov[i + 1].iov_base = m_audioQueue[i]->GetData();
	  	iov[i + 1].iov_len = m_audioQueue[i]->GetDataLength();
		}

		if (m_audio_set_rtp_header != NULL) {
	  		if ((m_audio_set_rtp_header)(iov, 
						     m_audioQueueCount, 
						     m_audio_rtp_userdata, 
						     &mbit)) {
	    			iov_start = 0;
	    			iov_add = 1;
	  		} 
		}
	}
		
	// send packet
	rdptr = m_rtpDestination;
	while (rdptr != NULL) {
	  rdptr->send_iov(&iov[iov_start],
			  iov_add + m_audioQueueCount,
			  rtpTimestamp,
			  mbit ? 1 : 0);
	  rdptr = rdptr->get_next();
	}

	// delete all the pending media frames
	for (ix = 0; ix < m_audioQueueCount; ix++) {
	  if (m_audioQueue[ix]->RemoveReference())
	    delete m_audioQueue[ix];
	  m_audioQueue[ix] = NULL;
	}

	m_audioQueueCount = 0;
	m_audioQueueSize = 0;
}

void CAudioRtpTransmitter::SendAudioJumboFrame(CMediaFrame* pFrame)
{
  u_int32_t rtpTimestamp = 
    TimestampToRtp(pFrame->GetTimestamp());
  u_int64_t ntpTimestamp =
    TimestampToNtp(pFrame->GetTimestamp());
  CRtpDestination *rdptr;

#ifdef DEBUG_SEND
  debug_message("A ts "U64" rtp %u ntp %u.%u",
		pFrame->GetTimestamp(),
		rtpTimestamp,
		(u_int32_t)(ntpTimestamp >> 32),
		(u_int32_t)ntpTimestamp);
#endif
  rdptr = m_rtpDestination;
  while (rdptr != NULL) {
    rdptr->send_rtcp(rtpTimestamp, ntpTimestamp);
    rdptr = rdptr->get_next();
  }

  u_int32_t dataOffset = 0;
  u_int32_t spaceAvailable = m_mtu;


  struct iovec iov[2];
  do {
    iov[1].iov_base = (u_int8_t*)pFrame->GetData() + dataOffset;
    uint iov_start = 1, iov_hdr = 0;
    bool mbit;
    if (m_audio_set_rtp_jumbo_frame(iov,
				    dataOffset, 
				    pFrame->GetDataLength(),
				    spaceAvailable,
				    mbit,
				    m_audio_rtp_userdata)) {
      iov_start = 0;
      iov_hdr = 1;
    }
    // send packet
    rdptr = m_rtpDestination;
    while (rdptr != NULL) {
      rdptr->send_iov(&iov[iov_start],
		      1 + iov_hdr,
		      rtpTimestamp,
		      mbit);
      rdptr = rdptr->get_next();
    }
    error_message("data offset %d len %d max %d", dataOffset, 
		  (int)iov[1].iov_len,
		  pFrame->GetDataLength());
    dataOffset += iov[1].iov_len;
  } while (dataOffset < pFrame->GetDataLength());

  if (pFrame->RemoveReference())
    delete pFrame;
}




void CRtpTransmitter::DoStartRtpDestination (const char *destAddr,
					     in_port_t destPort)
{
  CRtpDestination *ptr;

  ptr = m_rtpDestination;
  while (ptr != NULL) {
    if (ptr->Matches(destAddr, destPort)) {
      ptr->start();
      return;
    }
    ptr = ptr->get_next();
  }
}

void CRtpTransmitter::DoStopRtpDestination (const char *destAddr, 
					    in_port_t destPort)
{
  CRtpDestination *ptr, *q;

  q = NULL;
  ptr = m_rtpDestination;
  while (ptr != NULL) {
    if (ptr->Matches(destAddr, destPort)) {
      if (ptr->remove_reference()) {
	if (q == NULL) {
	  m_rtpDestination = ptr->get_next();
	} else {
	  q->set_next(ptr->get_next());
	}
	delete ptr;
      }
      return;
    }
    q = ptr;
    ptr = ptr->get_next();
  }
}

CAudioRtpTransmitter::CAudioRtpTransmitter (CAudioProfile *ap,
					    uint16_t mtu,
					    bool disable_ts_offset) :
  CRtpTransmitter(mtu)
{
  m_audio_profile = ap;
  m_payloadNumber = 97;
  m_audioQueue = NULL;
  m_audio_rtp_userdata = NULL;
  m_audio_set_rtp_payload = NULL;	// plugin function to determind how to build RTP payload
  m_audio_queue_frame = NULL;		// plugin function to determind how to queue frames for RTP
  m_frameno = NULL;			// plugin will allocate counter space
  m_audioiovMaxCount = 0;		// Max number of iov segments for send_iov
	  
  /*  if (m_audioSrcPort == 0) {
    m_audioSrcPort = GetRandomPortBlock();
    }
    if (m_pConfig->GetIntegerValue(CONFIG_RTP_AUDIO_DEST_PORT) == 0) {
      m_pConfig->SetIntegerValue(CONFIG_RTP_AUDIO_DEST_PORT,
				 m_audioSrcPort + 2);
    }
  */
  if (get_audio_rtp_info(ap,
			 &m_frameType,
			 &m_timeScale,
			 &m_payloadNumber, 
			 &m_audioPayloadBytesPerPacket,
			 &m_audioPayloadBytesPerFrame,
			 &m_audioQueueMaxCount,
			 &m_audioiovMaxCount,
			 &m_audio_queue_frame,
			 &m_audio_set_rtp_payload,
			 &m_audio_set_rtp_header,
			 &m_audio_set_rtp_jumbo_frame,
			 &m_audio_rtp_userdata) == false) {
      error_message("rtp transmitter: unknown audio encoding %s",
		    ap->GetStringValue(CFG_AUDIO_ENCODING));
    }

  uint8_t max_queue_from_config = 
    ap->GetIntegerValue(CFG_RTP_MAX_FRAMES_PER_PACKET);
  if (max_queue_from_config != 0 &&
      max_queue_from_config < m_audioQueueMaxCount) {
    m_audioQueueMaxCount = max_queue_from_config;
  }
    if (m_audioiovMaxCount == 0)			// This is purely for backwards compability
      m_audioiovMaxCount = m_audioQueueMaxCount;	// Can go away when lame and faac plugin sets this

#ifdef DEBUG_WRAP_TS
  m_audioRtpTimestampOffset = 0xffff0000;
#else 
    if (disable_ts_offset) {
      m_audioRtpTimestampOffset = 0;
    } else {
      m_audioRtpTimestampOffset = random();
    }
#endif

    m_audioQueueCount = 0;
    m_audioQueueSize = 0;
    
    m_audioQueue = (CMediaFrame**)Malloc(m_audioQueueMaxCount * sizeof(CMediaFrame*));
}

CAudioRtpTransmitter::~CAudioRtpTransmitter (void) 
{
  DoStopTransmit();
  CHECK_AND_FREE(m_audio_rtp_userdata);
  CHECK_AND_FREE(m_frameno);
  CHECK_AND_FREE(m_audioQueue);
}

CVideoRtpTransmitter::CVideoRtpTransmitter (CVideoProfile *vp, 
					    uint16_t mtu,
					    bool disable_ts_offset) :
  CRtpTransmitter(mtu, disable_ts_offset)
{
  m_videoSendFunc = GetVideoRtpTransmitRoutine(vp,
					       &m_frameType, 
					       &m_payloadNumber);
  m_timeScale = 90000;

}

CTextRtpTransmitter::CTextRtpTransmitter (CTextProfile *tp,
					    uint16_t mtu,
					    bool disable_ts_offset) :
  CRtpTransmitter(mtu, disable_ts_offset)
{
  m_textSendFunc = GetTextRtpTransmitRoutine(tp,
					     &m_frameType, 
					     &m_payloadNumber);
  m_timeScale = 90000;
}

static void RtpCallback (struct rtp *session, rtp_event *e) 
{
  // Currently we ignore RTCP packets
  // Just do our required housekeeping
  if (e && (e->type == RX_RTP || e->type == RX_APP)) {
    free(e->data);
  }
}

CRtpDestination::CRtpDestination (mp4live_rtp_params_t *rtp_params,
				  uint8_t payloadNumber)
{
  m_rtp_params = rtp_params;
  m_payloadNumber = payloadNumber;
  m_rtpSession = NULL;
  m_srtpSession = NULL;
  m_next = NULL;
  m_ref_mutex = SDL_CreateMutex();
  m_reference = 1;
  m_destroy_rtp_session = true;
}

CRtpDestination::CRtpDestination (struct rtp *session, 
				  uint8_t payloadNumber)
{
  m_rtp_params = NULL;
  m_payloadNumber = payloadNumber;
  m_rtpSession = session;
  m_srtpSession = NULL;
  m_next = NULL;
  m_ref_mutex = SDL_CreateMutex();
  m_reference = 1;
  m_destroy_rtp_session = false;
}
  

CRtpDestination::~CRtpDestination (void)
{
  if (m_rtpSession != NULL && m_destroy_rtp_session) {
    rtp_done(m_rtpSession);
  }
  m_rtpSession = NULL;
  if (m_srtpSession != NULL) {
    destroy_srtp(m_srtpSession);
    m_srtpSession = NULL;
  }
  CHECK_AND_FREE(m_rtp_params);
  SDL_DestroyMutex(m_ref_mutex);
}


void CRtpDestination::start (void) 
{

  while (m_rtpSession == NULL) {
    debug_message("Starting rtp dest %s %d %d", 
		  m_rtp_params->rtp_params.rtp_addr,
		  m_rtp_params->rtp_params.rtp_tx_port,
		  m_rtp_params->rtp_params.rtp_rx_port);
    if (m_rtp_params->rtp_params.rtcp_addr != NULL) {
      debug_message("Rtcp %s %u", 
		    m_rtp_params->rtp_params.rtcp_addr,
		    m_rtp_params->rtp_params.rtcp_tx_port);
    }
    /* wmay - comment out for now - might have to revisit for ssm
       if (m_srcPort == 0) m_srcPort = m_destPort;
     */
    m_rtp_params->rtp_params.rtp_callback = RtpCallback;
    m_rtp_params->rtp_params.recv_userdata = this;

    m_rtpSession = rtp_init_stream(&m_rtp_params->rtp_params);

    if (m_rtpSession == NULL) {
      error_message("Couldn't start rtp session");
      SDL_Delay(10);
    } else {
      //srtp init
      //what format is the key in?
      if (m_rtp_params->use_srtp) {
	m_srtpSession = srtp_setup(m_rtpSession, &m_rtp_params->srtp_params);
      }
      
    }
  }
}
			  
			  
void CRtpDestination::send_rtcp (u_int32_t rtpTimestamp,
				 u_int64_t ntpTimestamp)
{
  if (m_rtpSession != NULL) {
    rtp_send_ctrl_2(m_rtpSession, rtpTimestamp, ntpTimestamp, NULL);
    rtp_update(m_rtpSession);
  }
}

int CRtpDestination::send_iov (struct iovec *piov,
			       uint iovCount,
			       u_int32_t rtpTimestamp,
			       int mbit)
{
  if (m_rtpSession != NULL) {

    int ret = rtp_send_data_iov(m_rtpSession,
			     rtpTimestamp,
			     m_payloadNumber,
			     mbit, 
			     0, 
			     NULL, 
			     piov, 
			     iovCount,
			     NULL, 
			     0, 
			     0,
			     0);
    //debug_message("sending iov %d", ret);
    return ret;
    
  }
  return -1;
}

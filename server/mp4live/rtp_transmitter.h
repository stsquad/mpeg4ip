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
 */

#ifndef __RTP_TRANSMITTER_H__
#define __RTP_TRANSMITTER_H__

#include <rtp/rtp.h>
#include "media_sink.h"
#include "audio_encoder.h"

class CRtpDestination
{
 public:
  CRtpDestination(uint32_t refNum,
		  char *destAddr, 
		  in_port_t destPort,
		  in_port_t srcPort,
		  uint8_t payloadNumber,
		  int mcast_ttl, 
		  float rtcp_bandwidth);
  ~CRtpDestination();
  void start(void);
  void send_rtcp(u_int32_t rtpTimestamp,
		 u_int64_t ntpTimestamp);
  int send_iov(struct iovec *piov,
	       int iovCount,
	       u_int32_t rtpTimestamp,
	       int mbit);
  CRtpDestination *get_next(void) { return m_next;};
  void set_next (CRtpDestination *p) { m_next = p; };
  uint32_t get_ref_num (void) { return m_refNum; } ;
 protected:
  uint32_t m_refNum;
  CRtpDestination *m_next;
  char *m_destAddr;
  in_port_t m_destPort;
  in_port_t m_srcPort;
  uint32_t m_timeScale;
  uint8_t m_payloadNumber;
  int m_mcast_ttl;
  float m_rtcp_bandwidth;
  struct rtp *m_rtpSession;
};

#define DEFAULT_RTCP_BW (100.0)
class CRtpTransmitter : public CMediaSink {
public:
        CRtpTransmitter(CLiveConfig *pConfig);
	~CRtpTransmitter();
	static void SeedRandom(void) {
		static bool once = false;
		if (!once) {
			srandom(time(NULL));
			once = true;
		}
	}

	static u_int32_t GetRandomMcastAddress(void) {
		SeedRandom();

		// pick a random number in the multicast range
		u_int32_t mcast = ((random() & 0x0FFFFFFF) | 0xE0000000);

		// screen out undesirable values
		// introduces small biases in the results

		// stay away from 224.0.0.x
		if ((mcast & 0x0FFFFF00) == 0) {
			mcast |= 0x00000100;	// move to 224.0.1
		} 

		// stay out of SSM range 232.x.x.x
		// user should explictly select this if they want SSM
		if ((mcast & 0xFF000000) == 232) {
			mcast |= 0x01000000;	// move to 233
		}

		// stay away from .0 .1 and .255
		if ((mcast & 0xFF) == 0 || (mcast & 0xFF) == 1 
		  || (mcast & 0xFF) == 255) {
			mcast = (mcast & 0xFFFFFFF0) | 0x4;	// move to .4 or .244
		}

		return htonl(mcast);
	}

	static u_int16_t GetRandomPortBlock(void) {
		SeedRandom();

		// Get random block of 4 port numbers above 20000
		return (u_int16_t)(20000 + ((random() >> 18) << 2));

	}
	void CreateVideoRtpDestination (uint32_t ref,
					char *destAddress,
					in_port_t destPort,
					in_port_t srcPort);
	void CreateAudioRtpDestination (uint32_t ref,
					char *destAddress,
					in_port_t destPort,
					in_port_t srcPort);
	
	static const uint32_t MSG_RTP_DEST_START = 8192;
	static const uint32_t MSG_RTP_DEST_STOP  = MSG_RTP_DEST_START + 1;

	void StartRtpDestination (uint32_t RtpDestinationHandle) {
	  m_myMsgQueue.send_message(MSG_RTP_DEST_START, 
				    RtpDestinationHandle,
				    m_myMsgQueueSemaphore);
	};
	void StopRtpDestination (uint32_t RtpDestinationHandle) {
	  m_myMsgQueue.send_message(MSG_RTP_DEST_STOP, 
				    RtpDestinationHandle,
				    m_myMsgQueueSemaphore);
	};
protected:
	int ThreadMain(void);

	void DoStartTransmit(void);
	void DoStopTransmit(void);
	void DoSendFrame(CMediaFrame* pFrame);

	void SendAudioFrame(CMediaFrame* pFrame);
	void SendAudioJumboFrame(CMediaFrame* pFrame);
	void SendQueuedAudioFrames(void);

	void SendMpeg4VideoWith3016(CMediaFrame* pFrame);
	void DoStartRtpDestination(uint32_t handle);
	void DoStopRtpDestination(uint32_t handle);

	u_int32_t AudioTimestampToRtp(Timestamp t) {
	  if (m_haveAudioStartTimestamp) {
	    return (u_int32_t)(((t - m_audioStartTimestamp) 
				* m_audioTimeScale) / TimestampTicks)
	      + m_audioRtpTimestampOffset;
	  } 
	  m_audioStartTimestamp = t;
	  m_haveAudioStartTimestamp = true;
	  return m_audioRtpTimestampOffset;  
	}

	u_int32_t VideoTimestampToRtp(Timestamp t) {
	  if (m_haveVideoStartTimestamp) {
	    return (u_int32_t)(((t - m_videoStartTimestamp) 
				* m_videoTimeScale) / TimestampTicks)
	      + m_videoRtpTimestampOffset;
	  }
	  m_videoStartTimestamp = t;
	  m_haveVideoStartTimestamp = true;
	  return m_videoRtpTimestampOffset;
	}

	static const u_int32_t SECS_BETWEEN_1900_1970 = 2208988800U;

	u_int64_t TimestampToNtp(Timestamp t) {
		// low order ntp 32 bits is 2 ^ 32 -1 ticks per sec
		register u_int32_t usecs = t % TimestampTicks;
		return (((t / TimestampTicks) + SECS_BETWEEN_1900_1970) << 32)
			| ((usecs << 12) + (usecs << 8) - ((usecs * 3650) >> 6));
	}

	static void RtpCallback(struct rtp *session, rtp_event *e) {
		// Currently we ignore RTCP packets
		// Just do our required housekeeping
		if (e && e->type == RX_RTP) {
			free(e->data);
		}
	}

protected:
	bool m_haveAudioStartTimestamp;
	bool m_haveVideoStartTimestamp;
	Timestamp		m_audioStartTimestamp;
	Timestamp		m_videoStartTimestamp;

	SDL_mutex *m_destListMutex;
	CRtpDestination  *m_videoRtpDestination;
	CRtpDestination  *m_audioRtpDestination;

	u_int8_t		m_videoPayloadNumber;
	u_int32_t		m_videoTimeScale;
	u_int32_t		m_videoRtpTimestampOffset;
	u_int16_t		m_videoSrcPort;

	MediaType		m_audioFrameType;
	u_int8_t		m_audioPayloadNumber;
	u_int32_t		m_audioTimeScale;
	u_int32_t		m_audioRtpTimestampOffset;
	u_int16_t		m_audioSrcPort;
	u_int8_t		m_audioPayloadBytesPerPacket;
	u_int8_t		m_audioPayloadBytesPerFrame;
	audio_set_rtp_header_f  m_audio_set_rtp_header;
	audio_set_rtp_jumbo_frame_f m_audio_set_rtp_jumbo_frame;
	void *m_audio_rtp_userdata;
	CMediaFrame**	m_audioQueue;
	u_int8_t		m_audioQueueCount;	// number of frames
	u_int8_t		m_audioQueueMaxCount;	// max number of frames
	u_int16_t		m_audioQueueSize;	// bytes for RTP packet payload
};

  
#endif /* __RTP_TRANSMITTER_H__ */

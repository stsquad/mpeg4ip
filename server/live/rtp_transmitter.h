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

#include "media_node.h"

class CRtpTransmitter : public CMediaSink {
public:
	CRtpTransmitter() {
		m_transmit = false;
		m_rtcpBandwidth = 100.0;

		m_audioDestAddress = NULL;
		m_audioRtpSession = NULL;
		m_audioPayloadNumber = 14;

		m_videoDestAddress = NULL;
		m_videoRtpSession = NULL;
		m_videoPayloadNumber = 96;
		m_videoTimeScale = 90000;
	}

	void StartTransmit(void) {
		m_myMsgQueue.send_message(MSG_START_TRANSMIT,
			 NULL, 0, m_myMsgQueueSemaphore);
	}

	void StopTransmit(void) {
		m_myMsgQueue.send_message(MSG_STOP_TRANSMIT,
			NULL, 0, m_myMsgQueueSemaphore);
	}

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

		// stay away from 224.0.0
		if ((mcast & 0x0FFFFF00) == 0) {
			mcast |= 0x00000100;	// move to 224.0.1
		} 
		// stay away from .0 .1 and .255
		if ((mcast & 0xFF) == 0 || (mcast & 0xFF) == 1 
		  || (mcast & 0xFF) == 255) {
			mcast = (mcast & 0xFFFFFFF0) | 0x4;	// move to .4 or .244
		}

		return htonl(mcast);
	}

	static u_int16_t GetRandomPortBlock(void) {
		// Get random block of 4 port numbers between 32768 and 65532
		// Conventional wisdom is that higher order bits of
		// pseudo-random number generators are more random so we use those 
		SeedRandom();

		return (u_int16_t)(((random() >> 16) & 0x0000FFFC) | 0x00008000);
	}

protected:
	static const int MSG_START_TRANSMIT	= 1;
	static const int MSG_STOP_TRANSMIT	= 2;

	int ThreadMain(void);

	void DoStartTransmit(void);
	void DoStopTransmit(void);
	void DoSendFrame(CMediaFrame* pFrame);

	void SendMp3AudioWith2250(CMediaFrame* pFrame);
	void SendMp3QueuedFrames(void);
	void SendMp3JumboFrame(CMediaFrame* pFrame);

	void SendMpeg4VideoWith3016(CMediaFrame* pFrame);

	u_int32_t AudioTimestampToRtp(Timestamp t) {
		return (u_int32_t)(((t - m_startTimestamp) 
			* m_audioTimeScale) / TimestampTicks)
			+ m_audioRtpTimestampOffset;
	}

	u_int32_t VideoTimestampToRtp(Timestamp t) {
		return (u_int32_t)(((t - m_startTimestamp) 
			* m_videoTimeScale) / TimestampTicks)
			+ m_videoRtpTimestampOffset;
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
	bool			m_transmit;
	Timestamp		m_startTimestamp;
	float			m_rtcpBandwidth;

	char*			m_audioDestAddress;
	struct rtp*		m_audioRtpSession;
	u_int8_t		m_audioPayloadNumber;
	u_int32_t		m_audioTimeScale;
	u_int32_t		m_audioRtpTimestampOffset;
	u_int16_t		m_audioSrcPort;

	char*			m_videoDestAddress;
	struct rtp*		m_videoRtpSession;
	u_int8_t		m_videoPayloadNumber;
	u_int32_t		m_videoTimeScale;
	u_int32_t		m_videoRtpTimestampOffset;
	u_int16_t		m_videoSrcPort;

	// this value chosen to keep queuing latency reasonable
	// i.e. on the order of 100's of ms
	static const u_int8_t mp3QueueMaxCount = 8;
	static const u_int16_t mp3PayloadHeaderSize = 4;

	CMediaFrame*	m_mp3Queue[mp3QueueMaxCount];
	u_int8_t		m_mp3QueueCount;
	u_int16_t		m_mp3QueueSize;
};

#endif /* __RTP_TRANSMITTER_H__ */

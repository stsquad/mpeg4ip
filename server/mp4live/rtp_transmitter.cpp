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

#include "mp4live.h"
#include "rtp_transmitter.h"

int CRtpTransmitter::ThreadMain(void) 
{
	while (SDL_SemWait(m_myMsgQueueSemaphore) == 0) {
		CMsg* pMsg = m_myMsgQueue.get_message();
		
		if (pMsg != NULL) {
			switch (pMsg->get_value()) {
			case MSG_NODE_STOP_THREAD:
				DoStopTransmit();
				delete pMsg;
				return 0;
			case MSG_NODE_START:
				DoStartTransmit();
				break;
			case MSG_NODE_STOP:
				DoStopTransmit();
				break;
			case MSG_SINK_FRAME:
				size_t dontcare;
				DoSendFrame((CMediaFrame*)pMsg->get_message(dontcare));
				break;
			}

			delete pMsg;
		}
	}

	return -1;
}

void CRtpTransmitter::DoStartTransmit()
{
	if (m_transmit) {
		return;
	}

	m_startTimestamp = GetTimestamp();

	if (m_pConfig->GetBoolValue(CONFIG_AUDIO_ENABLE)) {
		m_audioTimeScale = 
			m_pConfig->GetIntegerValue(CONFIG_AUDIO_SAMPLE_RATE); 

		m_audioDestAddress = 
			m_pConfig->GetStringValue(CONFIG_RTP_DEST_ADDRESS); 

		if (m_pConfig->GetBoolValue(CONFIG_RTP_DISABLE_TS_OFFSET)) {
			m_audioRtpTimestampOffset = 0;
		} else {
			m_audioRtpTimestampOffset = random();
		}

		m_audioSrcPort = GetRandomPortBlock();
		if (m_pConfig->GetIntegerValue(CONFIG_RTP_AUDIO_DEST_PORT) == 0) {
			m_pConfig->SetIntegerValue(CONFIG_RTP_AUDIO_DEST_PORT,
				m_audioSrcPort + 2);
		}

		m_audioRtpSession = rtp_init(m_audioDestAddress, 
			m_audioSrcPort, 
			m_pConfig->GetIntegerValue(CONFIG_RTP_AUDIO_DEST_PORT),
			m_pConfig->GetIntegerValue(CONFIG_RTP_MCAST_TTL), 
			m_rtcpBandwidth, 
			RtpCallback, (uint8_t *)this);

		m_audioQueueCount = 0;
		m_audioQueueSize = 0;

		if (!strcasecmp(m_pConfig->GetStringValue(CONFIG_AUDIO_ENCODING),
		  AUDIO_ENCODING_MP3)) {
			m_audioFrameType = CMediaFrame::Mp3AudioFrame;
			m_audioPayloadBytesPerPacket = 4;
			m_audioPayloadBytesPerFrame = 0;
		} else { // AAC
			m_audioFrameType = CMediaFrame::AacAudioFrame;
			m_audioPayloadBytesPerPacket = 2;
			m_audioPayloadBytesPerFrame = 2;
		}
	}

	if (m_pConfig->GetBoolValue(CONFIG_VIDEO_ENABLE)) {
		m_videoDestAddress = 
			m_pConfig->GetStringValue(CONFIG_RTP_DEST_ADDRESS); 

		if (m_pConfig->GetBoolValue(CONFIG_RTP_DISABLE_TS_OFFSET)) {
			m_videoRtpTimestampOffset = 0;
		} else {
			m_videoRtpTimestampOffset = random();
		}

		m_videoSrcPort = GetRandomPortBlock();
		if (m_pConfig->GetIntegerValue(CONFIG_RTP_VIDEO_DEST_PORT) == 0) {
			m_pConfig->SetIntegerValue(CONFIG_RTP_VIDEO_DEST_PORT,
				m_videoSrcPort + 2);
		}

		m_videoRtpSession = rtp_init(m_videoDestAddress, 
			m_videoSrcPort, 
			m_pConfig->GetIntegerValue(CONFIG_RTP_VIDEO_DEST_PORT),
			m_pConfig->GetIntegerValue(CONFIG_RTP_MCAST_TTL), 
			m_rtcpBandwidth, 
			RtpCallback, (uint8_t *)this);
	}

	if (m_audioRtpSession || m_videoRtpSession) {
		// automatic sdp file generation
		GenerateSdpFile(m_pConfig);

		m_transmit = true;
	}
}

void CRtpTransmitter::DoStopTransmit()
{
	if (!m_transmit) {
		return;
	}

	if (m_audioRtpSession) {
		// send any pending frames
		SendQueuedAudioFrames();

		rtp_done(m_audioRtpSession);
		m_audioRtpSession = NULL;
	}
	if (m_videoRtpSession) {
		rtp_done(m_videoRtpSession);
		m_videoRtpSession = NULL;
	}

	m_transmit = false;
}

void CRtpTransmitter::DoSendFrame(CMediaFrame* pFrame)
{
	if (pFrame == NULL) {
		return;
	}
	if (!m_transmit) {
		delete pFrame;
		return;
	}

	if (pFrame->GetType() == m_audioFrameType && m_audioRtpSession) {
		SendAudioFrame(pFrame);

	} else if (pFrame->GetType() == CMediaFrame::Mpeg4VideoFrame 
	  && m_videoRtpSession) {
		SendMpeg4VideoWith3016(pFrame);

	} else {
		delete pFrame;
	}
}

void CRtpTransmitter::SendAudioFrame(CMediaFrame* pFrame)
{
	u_int16_t newAudioQueueSize;

	if (m_audioQueueCount > 0) {
		newAudioQueueSize =
			m_audioQueueSize 
			+ m_audioPayloadBytesPerFrame
			+ pFrame->GetDataLength();

		if (newAudioQueueSize <= 
		  m_pConfig->GetIntegerValue(CONFIG_RTP_PAYLOAD_SIZE)) {

			m_audioQueue[m_audioQueueCount++] = pFrame;
			m_audioQueueSize = newAudioQueueSize;

			// if we fill the last slot in the queue
			// send the packet
			if (m_audioQueueCount == audioQueueMaxCount) {
				SendQueuedAudioFrames();
			}

			return;

		} else {
			// send queued frames
			SendQueuedAudioFrames();

			// fall thru
		}
	}

	// by here m_audioQueueCount == 0

	newAudioQueueSize =
		m_audioPayloadBytesPerPacket
		+ m_audioPayloadBytesPerFrame
		+ pFrame->GetDataLength();

	if (newAudioQueueSize 
	  <= m_pConfig->GetIntegerValue(CONFIG_RTP_PAYLOAD_SIZE)) {

		m_audioQueue[m_audioQueueCount++] = pFrame;
		m_audioQueueSize = newAudioQueueSize;

	} else {
		// highly unusual case 
		// we need to fragment audio frame
		// over multiple packets
		SendAudioJumboFrame(pFrame);
	}
}

void CRtpTransmitter::SendQueuedAudioFrames(void)
{
	static u_int32_t zero32 = 0;
	u_int8_t aacHeader[2 + (audioQueueMaxCount * 2)];
	struct iovec iov[audioQueueMaxCount + 1];

	if (m_audioQueueCount == 0) {
		return;
	}

	u_int32_t rtpTimestamp =
		AudioTimestampToRtp(m_audioQueue[0]->GetTimestamp());
	u_int64_t ntpTimestamp =
		TimestampToNtp(m_audioQueue[0]->GetTimestamp());

	// check if RTCP SR needs to be sent
	rtp_send_ctrl_2(m_audioRtpSession, rtpTimestamp, ntpTimestamp, NULL);
	rtp_update(m_audioRtpSession);

	// create the iovec list
	u_int8_t i;

	// audio payload headers
	if (m_audioQueue[0]->GetType() == CMediaFrame::Mp3AudioFrame) {
		iov[0].iov_base = &zero32;
		iov[0].iov_len = 4;
	} else { // AAC
		u_int16_t numHdrBits = 16 * m_audioQueueCount;
		aacHeader[0] = numHdrBits >> 8;
		aacHeader[1] = numHdrBits & 0xFF;

		for (i = 0; i < m_audioQueueCount; i++) {
			aacHeader[2 + (i * 2)] = 
				m_audioQueue[i]->GetDataLength() >> 5;
			aacHeader[2 + (i * 2) + 1] = 
				(m_audioQueue[i]->GetDataLength() & 0x1F) << 3;
		}

		iov[0].iov_base = aacHeader;
		iov[0].iov_len = 2 + (m_audioQueueCount * 2);
	}

	// audio payload data
	for (i = 0; i < m_audioQueueCount; i++) {
		iov[i+1].iov_base = m_audioQueue[i]->GetData();
		iov[i+1].iov_len = m_audioQueue[i]->GetDataLength();
	}

	// send packet
	rtp_send_data_iov(
		m_audioRtpSession,
		rtpTimestamp,
		m_audioPayloadNumber, 1, 0, NULL,
		iov, m_audioQueueCount + 1,
		NULL, 0, 0);

	// delete all the pending media frames
	for (int i = 0; i < m_audioQueueCount; i++) {
		delete m_audioQueue[i];
		m_audioQueue[i] = NULL;
	}

	m_audioQueueCount = 0;
	m_audioQueueSize = 0;
}

void CRtpTransmitter::SendAudioJumboFrame(CMediaFrame* pFrame)
{
	u_int32_t rtpTimestamp = 
		AudioTimestampToRtp(pFrame->GetTimestamp());
	u_int64_t ntpTimestamp =
		TimestampToNtp(pFrame->GetTimestamp());

	// check if RTCP SR needs to be sent
	rtp_send_ctrl_2(m_audioRtpSession, rtpTimestamp, ntpTimestamp, NULL);
	rtp_update(m_audioRtpSession);

	u_int32_t dataOffset = 0;
	u_int32_t spaceAvailable = 
		m_pConfig->GetIntegerValue(CONFIG_RTP_PAYLOAD_SIZE)
		- (m_audioPayloadBytesPerPacket + m_audioPayloadBytesPerFrame);

	bool firstPacket = true;
	u_int8_t payloadHeader[4];

	struct iovec iov[2];
	iov[0].iov_base = &payloadHeader;
	iov[0].iov_len = 4;

	if (pFrame->GetType() == CMediaFrame::Mp3AudioFrame) {
		payloadHeader[0] = 0;
		payloadHeader[1] = 0;

		do {
			// update MP3 payload header
			payloadHeader[2] = (dataOffset >> 8);
			payloadHeader[3] = (dataOffset & 0xFF);

			iov[1].iov_base = (u_int8_t*)pFrame->GetData() + dataOffset;

			// figure out how much data we're sending
			u_int32_t dataPending = pFrame->GetDataLength() - dataOffset;
			iov[1].iov_len = MIN(dataPending, spaceAvailable); 

			// send packet
			rtp_send_data_iov(
				m_audioRtpSession,
				rtpTimestamp,
				m_audioPayloadNumber, firstPacket, 0, NULL,
				iov, 2,
				NULL, 0, 0);

			dataOffset += iov[1].iov_len;
			firstPacket = false;

		} while (dataOffset < pFrame->GetDataLength());

	} else { // AAC
		bool lastPacket = false;

		payloadHeader[0] = 0;
		payloadHeader[1] = 16;
		payloadHeader[2] = pFrame->GetDataLength() >> 5;
		payloadHeader[3] = (pFrame->GetDataLength() & 0x1F) << 3;

		do {
			iov[1].iov_base = (u_int8_t*)pFrame->GetData() + dataOffset;

			// figure out how much data we're sending
			u_int32_t dataPending = pFrame->GetDataLength() - dataOffset;
			if (dataPending <= spaceAvailable) {
				iov[1].iov_len = dataPending;
				lastPacket = true;
			} else {
				iov[1].iov_len = spaceAvailable;
			}

			// send packet
			if (firstPacket) {
				rtp_send_data_iov(
					m_audioRtpSession,
					rtpTimestamp,
					m_audioPayloadNumber, false, 0, NULL,
					&iov[0], 2,
					NULL, 0, 0);

				firstPacket = false;
				spaceAvailable += 
					m_audioPayloadBytesPerPacket 
					+ m_audioPayloadBytesPerFrame;

			} else {
				rtp_send_data_iov(
					m_audioRtpSession,
					rtpTimestamp,
					m_audioPayloadNumber, lastPacket, 0, NULL,
					&iov[1], 1,
					NULL, 0, 0);
			}

			dataOffset += iov[1].iov_len;

		} while (dataOffset < pFrame->GetDataLength());
	}

	delete pFrame;
}

void CRtpTransmitter::SendMpeg4VideoWith3016(CMediaFrame* pFrame)
{
	u_int32_t rtpTimestamp =
		VideoTimestampToRtp(pFrame->GetTimestamp());
	u_int64_t ntpTimestamp =
		TimestampToNtp(pFrame->GetTimestamp());

	// check if RTCP SR needs to be sent
	rtp_send_ctrl_2(m_videoRtpSession, rtpTimestamp, ntpTimestamp, NULL);
	rtp_update(m_videoRtpSession);

	u_int8_t* pData = (u_int8_t*)pFrame->GetData();
	u_int32_t bytesToSend = pFrame->GetDataLength();
	struct iovec iov;

	while (bytesToSend) {
		u_int32_t payloadLength;
		bool lastPacket;

		if (bytesToSend 
		  <= m_pConfig->GetIntegerValue(CONFIG_RTP_PAYLOAD_SIZE)) {
			payloadLength = bytesToSend;
			lastPacket = true;
		} else {
			payloadLength = 
				m_pConfig->GetIntegerValue(CONFIG_RTP_PAYLOAD_SIZE);
			lastPacket = false;
		}

		iov.iov_base = pData;
		iov.iov_len = payloadLength;

		rtp_send_data_iov(
			m_videoRtpSession,
			rtpTimestamp,
			m_videoPayloadNumber, lastPacket, 0, NULL,
			&iov, 1,
		 	NULL, 0, 0);

		pData += payloadLength;
		bytesToSend -= payloadLength;
	}

	delete pFrame;
}


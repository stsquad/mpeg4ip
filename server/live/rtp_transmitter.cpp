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
			case MSG_START_TRANSMIT:
				DoStartTransmit();
				break;
			case MSG_STOP_TRANSMIT:
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

	if (m_pConfig->m_audioEnable) {
		m_audioDestAddress = m_pConfig->m_rtpDestAddress; 

		if (m_pConfig->m_rtpDisableTimestampOffset) {
			m_audioRtpTimestampOffset = 0;
		} else {
			m_audioRtpTimestampOffset = random();
		}

		m_audioSrcPort = GetRandomPortBlock();
		if (m_pConfig->m_rtpAudioDestPort == 0) {
			m_pConfig->m_rtpAudioDestPort = m_audioSrcPort + 2;
		}

		m_audioRtpSession = rtp_init(m_audioDestAddress, 
			m_audioSrcPort, m_pConfig->m_rtpAudioDestPort, 
			m_pConfig->m_rtpMulticastTtl, m_rtcpBandwidth, 
			RtpCallback, this);

		m_mp3QueueCount = 0;
		m_mp3QueueSize = 0;
	}

	if (m_pConfig->m_videoEnable) {
		m_videoDestAddress = m_pConfig->m_rtpDestAddress; 

		if (m_pConfig->m_rtpDisableTimestampOffset) {
			m_videoRtpTimestampOffset = 0;
		} else {
			m_videoRtpTimestampOffset = random();
		}

		m_videoSrcPort = GetRandomPortBlock();
		if (m_pConfig->m_rtpVideoDestPort == 0) {
			m_pConfig->m_rtpVideoDestPort = m_videoSrcPort + 2;
		}

		m_videoRtpSession = rtp_init(m_videoDestAddress, 
			m_videoSrcPort, m_pConfig->m_rtpVideoDestPort, 
			m_pConfig->m_rtpMulticastTtl, m_rtcpBandwidth, 
			RtpCallback, this);
	}

	if (m_audioRtpSession || m_videoRtpSession) {
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
		SendMp3QueuedFrames();

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
	if (pFrame->GetType() == CMediaFrame::Mp3AudioFrame) {
		SendMp3AudioWith2250(pFrame);
	}

	if (pFrame->GetType() == CMediaFrame::Mpeg4VideoFrame) {
		SendMpeg4VideoWith3016(pFrame);
	}
}

void CRtpTransmitter::SendMp3AudioWith2250(CMediaFrame* pFrame)
{
	// if this frame would overflow current rtp packet
	if ((u_int16_t)(m_pConfig->m_rtpPayloadSize - m_mp3QueueSize)
	  < pFrame->GetDataLength() + mp3PayloadHeaderSize) {

		// send queued frames
		SendMp3QueuedFrames();
	}

	// TBD might want to treat greater than 1/2 payload size as jumbo

	if (pFrame->GetDataLength() + mp3PayloadHeaderSize 
	  < m_pConfig->m_rtpPayloadSize) {

		// by here we are guaranteed mp3 frame 
		// will fit in packet so queue it up
		m_mp3Queue[m_mp3QueueCount++] = pFrame;
		m_mp3QueueSize += pFrame->GetDataLength() + mp3PayloadHeaderSize;

		// if we fill the last slot in the queue
		// send the packet
		if (m_mp3QueueCount == mp3QueueMaxCount) {
			SendMp3QueuedFrames();
		}
	} else {
		// highly unusual case 
		// we need to fragment MP3 frame
		// over multiple packets
		SendMp3JumboFrame(pFrame);
	}
}

void CRtpTransmitter::SendMp3QueuedFrames(void)
{
	struct iovec iov[2 * mp3QueueMaxCount];
	static u_int32_t zero32 = 0;

	if (m_mp3QueueCount == 0) {
		return;
	}

	// create the iovec list
	for (int i = 0; i < 2 * m_mp3QueueCount; ) {
		iov[i].iov_base = &zero32;
		iov[i++].iov_len = 4;
		iov[i].iov_base = m_mp3Queue[i/2]->GetData();
		iov[i++].iov_len = m_mp3Queue[i/2]->GetDataLength();
	}

	// send packet
	rtp_send_data_iov(m_videoRtpSession,
		ConvertAudioTimestamp(m_mp3Queue[0]->GetTimestamp()),
		m_audioPayloadNumber, 1, 0, NULL,
		iov, m_mp3QueueCount,
		NULL, 0, 0);

	// delete all the pending media frames
	for (int i = 0; i < m_mp3QueueCount; i++) {
		delete m_mp3Queue[i];
		m_mp3Queue[i] = NULL;
	}

	m_mp3QueueCount = 0;
	m_mp3QueueSize = 0;
}

void CRtpTransmitter::SendMp3JumboFrame(CMediaFrame* pFrame)
{
	u_int32_t dataOffset = 0;
	u_int32_t spaceAvailable = 
		m_pConfig->m_rtpPayloadSize - mp3PayloadHeaderSize;
	bool lastPacket = false;

	Timestamp timestamp = ConvertAudioTimestamp(pFrame->GetTimestamp());

	u_int8_t payloadHeader[4];
	payloadHeader[0] = payloadHeader[1] = 0;

	struct iovec iov[2];
	iov[0].iov_base = &payloadHeader;
	iov[0].iov_len = 4;

	do {
		// update MP3 payload header
		payloadHeader[2] = (dataOffset >> 8);
		payloadHeader[3] = (dataOffset & 0xFF);

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
		rtp_send_data_iov(m_videoRtpSession,
			timestamp,
			m_audioPayloadNumber, lastPacket, 0, NULL,
			iov, 2,
		 	NULL, 0, 0);

		dataOffset += iov[1].iov_len;

	} while (dataOffset < pFrame->GetDataLength());

	delete pFrame;
}

void CRtpTransmitter::SendMpeg4VideoWith3016(CMediaFrame* pFrame)
{
	u_int8_t* pData = (u_int8_t*)pFrame->GetData();
	u_int32_t bytesToSend = pFrame->GetDataLength();

	while (bytesToSend) {
		u_int32_t payloadLength;
		bool lastPacket;

		if (bytesToSend <= m_pConfig->m_rtpPayloadSize) {
			payloadLength = bytesToSend;
			lastPacket = true;
		} else {
			payloadLength = m_pConfig->m_rtpPayloadSize;
			lastPacket = false;
		}

		rtp_send_data(m_videoRtpSession,
			ConvertVideoTimestamp(pFrame->GetTimestamp()),
			m_videoPayloadNumber, lastPacket, 0, NULL,
			(char*)pData, payloadLength,
			NULL, 0, 0);

		pData += payloadLength;
		bytesToSend -= payloadLength;
	}

	delete pFrame;
}


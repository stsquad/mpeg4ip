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

#ifndef __MP4_RECORDER_H__
#define __MP4_RECORDER_H__

#include <quicktime.h>
#include "media_node.h"

#define RTP_HEADER_STD_SIZE 12

class CMp4Recorder : public CMediaSink {
public:
	CMp4Recorder() {
		m_record = false;
		m_mp4File = NULL;

		m_audioTrack = 0;
		m_audioHintTrack = 0;
		m_videoTrack = 0;
		m_videoHintTrack = 0;

		m_videoTimeScale = 90000;
		m_movieTimeScale = m_videoTimeScale;
	}

	void StartRecord(void) {
		m_myMsgQueue.send_message(MSG_START_RECORD,
			 NULL, 0, m_myMsgQueueSemaphore);
	}

	void StopRecord(void) {
		m_myMsgQueue.send_message(MSG_STOP_RECORD,
			NULL, 0, m_myMsgQueueSemaphore);
	}

protected:
	static const int MSG_START_RECORD	= 1;
	static const int MSG_STOP_RECORD	= 2;

	int ThreadMain(void);

	void DoStartRecord(void);
	void DoStopRecord(void);
	void DoWriteFrame(CMediaFrame* pFrame);

	void Write2250Hints(CMediaFrame* pFrame);

	void Write3016Hints(CMediaFrame* pFrame);
	void Write3016Hints(u_int32_t frameLength, bool isIFrame);

protected:
	bool			m_record;
	char*			m_mp4FileName;
	quicktime_t*	m_mp4File;

	int				m_audioTrack;
	u_int32_t		m_audioFrameNum;
	u_int16_t		m_audioFrameRate;
	int				m_audioHintTrack;
	u_int8_t		m_audioHintBuf[4*1024];
	u_int32_t 		m_audioHintBufLength;
	u_int32_t		m_audioRtpPktNum;
	u_int32_t		m_audioFramesThisHint;
	u_int32_t		m_audioBytesThisHint;
	u_int32_t		m_audioBytesThisSec;
	u_int32_t		m_audioFirstRtpPktThisSec;
	u_int32_t		m_audioMaxRtpBytesPerSec;

	int				m_videoTrack;
	u_int32_t		m_videoFrameNum;
	int				m_videoHintTrack;
	u_int8_t		m_videoHintBuf[4*1024];
	u_int32_t 		m_videoHintBufLength;
	u_int32_t		m_videoRtpPktNum;
	u_int32_t		m_videoBytesThisSec;
	u_int32_t		m_videoFirstRtpPktThisSec;
	u_int32_t		m_videoMaxRtpBytesPerSec;

	u_int32_t		m_movieTimeScale;
	u_int32_t		m_audioTimeScale;
	u_int32_t		m_audioFrameDuration;
	u_int32_t		m_videoTimeScale;

	u_int32_t		m_audioPayloadNumber;
	u_int32_t		m_videoPayloadNumber;
};

#endif /* __MP4_RECORDER_H__ */

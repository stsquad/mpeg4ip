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

#ifndef __FILE_MP4_RECORDER_H__
#define __FILE_MP4_RECORDER_H__

#include <mp4.h>
#include <mp4av.h>
#include "media_sink.h"

class CMp4Recorder : public CMediaSink {
public:
	CMp4Recorder() {
		m_mp4File = NULL;
		m_rawAudioTrackId = MP4_INVALID_TRACK_ID;
		m_encodedAudioTrackId = MP4_INVALID_TRACK_ID;
		m_rawVideoTrackId = MP4_INVALID_TRACK_ID;
		m_encodedVideoTrackId = MP4_INVALID_TRACK_ID;

		m_videoTimeScale = 90000;
		m_movieTimeScale = m_videoTimeScale;
		m_audioGapThresholdInSamples = 128;
	}

protected:
	int ThreadMain(void);

	void DoStartRecord(void);
	void DoStopRecord(void);
	void DoWriteFrame(CMediaFrame* pFrame);

protected:
	bool			m_recordVideo;
	bool			m_canRecordAudio;	// used for sync'ed start of A/V

	char*			m_mp4FileName;
	MP4FileHandle	m_mp4File;

	u_int32_t		m_movieTimeScale;
	u_int32_t		m_videoTimeScale;
	u_int32_t		m_audioTimeScale;
	Timestamp		m_movieStartTimestamp;
	u_int32_t		m_audioGapThresholdInSamples;

	MP4TrackId		m_rawVideoTrackId;
	u_int32_t		m_rawVideoFrameNumber;
	Timestamp		m_rawVideoStartTimestamp;

	MP4TrackId		m_encodedVideoTrackId;
	u_int32_t		m_encodedVideoFrameNumber;
	Timestamp		m_encodedVideoStartTimestamp;

	MP4TrackId		m_rawAudioTrackId;
	u_int32_t		m_rawAudioFrameNumber;
	Timestamp		m_rawAudioStartTimestamp;
	Duration		m_rawAudioDuration;

	MP4TrackId		m_encodedAudioTrackId;
	u_int32_t		m_encodedAudioFrameNumber;
	Timestamp		m_encodedAudioStartTimestamp;
	Duration		m_encodedAudioDuration;
};

#endif /* __FILE_MP4_RECORDER_H__ */

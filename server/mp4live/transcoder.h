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
 */

#ifndef __MP4_TRANSCODER_H__
#define __MP4_TRANSCODER_H__

#include <mp4.h>
#include <mp4av.h>
#include "media_node.h"
#include "video_encoder.h"
#include "video_decoder.h"
#include "audio_encoder.h"

class CTranscoder : public CMediaSource {
public:
	CTranscoder() {
		m_transcode = false;

		m_srcMp4File = NULL;
		m_dstMp4File = NULL;

		m_srcAudioTrackId = MP4_INVALID_TRACK_ID;
		m_dstAudioTrackId = MP4_INVALID_TRACK_ID;
		m_srcAudioSampleId = 1;

		m_srcVideoTrackId = MP4_INVALID_TRACK_ID;
		m_dstVideoTrackId = MP4_INVALID_TRACK_ID;
		m_srcVideoSampleId = 1;

		m_videoEncoder = NULL;
		m_videoDecoder = NULL;

		m_audioEncoder = NULL;
	}

	void StartTranscode(void) {
		m_myMsgQueue.send_message(MSG_START_TRANSCODE,
			 NULL, 0, m_myMsgQueueSemaphore);
	}

	void StopTranscode(void) {
		m_myMsgQueue.send_message(MSG_STOP_TRANSCODE,
			NULL, 0, m_myMsgQueueSemaphore);
	}

	bool GetRunning() {
		return m_transcode;
	} 

	u_int32_t GetNumEncodedVideoFrames() {
		return m_srcVideoSampleId;
	}

	float GetProgress();

	u_int64_t GetEstSize();

protected:
	static const int MSG_START_TRANSCODE	= 1;
	static const int MSG_STOP_TRANSCODE		= 2;

	int ThreadMain(void);

	void DoStartTranscode(void);
	void DoTranscode(void);
	void DoStopTranscode(void);

	bool DoVideoTrack(MP4SampleId startSampleId, u_int32_t numSamples);
	bool DoAudioTrack(MP4SampleId startSampleId, u_int32_t numSamples);

	bool InitVideoTranscode(void);
	bool InitVideoDecoder(void);
	bool InitVideoEncoder(void);

	bool InitAudioTranscode(void);
	bool InitAudioEncoder(void);

protected:
	bool			m_transcode;

	const char*		m_srcMp4FileName;
	const char*		m_dstMp4FileName;
	MP4FileHandle	m_srcMp4File;
	MP4FileHandle	m_dstMp4File;
	MP4TrackId		m_srcVideoTrackId;
	MP4TrackId		m_dstVideoTrackId;
	MP4TrackId		m_srcAudioTrackId;
	MP4TrackId		m_dstAudioTrackId;

	CVideoEncoder*	m_videoEncoder;
	CVideoDecoder*	m_videoDecoder;
	u_int32_t		m_srcVideoNumSamples;
	MP4SampleId		m_srcVideoSampleId;
	u_int16_t		m_srcVideoWidth;
	u_int16_t		m_srcVideoHeight;
	u_int32_t		m_videoYSize;
	u_int32_t		m_videoUVSize;
	u_int32_t		m_videoYUVSize;
	u_int8_t		m_videoDstType;

	CAudioEncoder*	m_audioEncoder;
	u_int32_t		m_srcAudioNumSamples;
	MP4SampleId		m_srcAudioSampleId;
	u_int8_t		m_audioDstType;
};

#endif /* __MP4_TRANSCODER_H__ */

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
 * Copyright (C) Cisco Systems Inc. 2000-2002.  All Rights Reserved.
 * 
 * Contributor(s): 
 *		Dave Mackie		dmackie@cisco.com
 */

#ifndef __FILE_MPEG2_SOURCE_H__
#define __FILE_MPEG2_SOURCE_H__

#include "media_node.h"
#include "video_encoder.h"
#include "audio_encoder.h"

#include <libmpeg3.h>
#include "video_util_resize.h"

class CMpeg2FileSource : public CMediaSource {
public:
	CMpeg2FileSource() : CMediaSource() {
		m_source = false;
		m_mpeg2File = NULL;

		m_sourceVideo = false;
		m_videoStream = 0;
		m_videoSrcYImage = NULL;
		m_videoDstYImage = NULL;
		m_videoYResizer = NULL;
		m_videoSrcUVImage = NULL;
		m_videoDstUVImage = NULL;
		m_videoUVResizer = NULL;
		m_videoEncoder = NULL;
		m_wantKeyFrame = false;
		m_videoFrameNumber = 0;

		m_sourceAudio = false;
		m_audioStream = 0;
		m_audioEncode = false;
		m_audioEncoder = NULL;
		m_audioEncodedForwardedFrames = 0;
	}

	u_int32_t GetNumEncodedVideoFrames() {
		return m_videoFrameNumber;
	}

	u_int32_t GetNumEncodedAudioFrames() {
		return m_audioEncodedForwardedFrames;
	}

protected:
	int ThreadMain(void);

	void DoStartVideo(void);
	void DoStartAudio(void);
	void DoStopSource(void);

	void DoGenerateKeyFrame(void);

	bool InitVideo(void);
	bool InitAudio(void);

	void ProcessMedia(void);
	void ProcessVideo(void);
	void ProcessAudio(void);

	Duration GetElapsedDuration(void);

protected:
	bool				m_source;
	mpeg3_t*			m_mpeg2File;

	bool				m_sourceVideo;

	int					m_videoStream;
	image_t*			m_videoSrcYImage;
	image_t*			m_videoDstYImage;
	scaler_t*			m_videoYResizer;
	image_t*			m_videoSrcUVImage;
	image_t*			m_videoDstUVImage;
	scaler_t*			m_videoUVResizer;

	u_int16_t			m_videoSrcWidth;
	u_int16_t			m_videoSrcHeight;
	u_int32_t			m_videoSrcYUVSize;
	u_int32_t			m_videoSrcYSize;
	u_int32_t			m_videoSrcUVSize;

	CVideoEncoder*		m_videoEncoder;
	u_int32_t			m_videoFrameNumber;
	Duration			m_videoFrameDuration;
	bool				m_wantKeyFrame;

	Timestamp			m_videoStartTimestamp;
	Duration			m_videoElapsedDuration;

	bool				m_sourceAudio;
	int					m_audioStream;

	char*				m_audioSrcEncoding;
	u_int8_t			m_audioSrcChannels;
	u_int32_t			m_audioSrcSampleRate;
	u_int16_t			m_audioSrcSamplesPerFrame;

	bool				m_audioEncode;
	CAudioEncoder*		m_audioEncoder;
	u_int16_t			m_audioDstFrameType;
	u_int32_t			m_audioMaxFrameLength;

	u_int64_t			m_audioSrcSamples;
	u_int32_t			m_audioSrcFrames;
	u_int64_t			m_audioRawForwardedSamples;
	u_int32_t			m_audioRawForwardedFrames;
	u_int64_t			m_audioEncodedForwardedSamples;
	u_int32_t			m_audioEncodedForwardedFrames;

	Timestamp			m_audioStartTimestamp;
	Duration			m_audioElapsedDuration;
};

#endif /* __FILE_MPEG2_SOURCE_H__ */

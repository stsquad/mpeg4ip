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

#include "media_source.h"
#include <libmpeg3.h>

class CMpeg2FileSource : public CMediaSource {
public:
	CMpeg2FileSource() : CMediaSource() {
		m_mpeg2File = NULL;

		m_videoStream = 0;
#ifdef MPEG2_RAW_ONLY
		m_videoYUVImage = NULL;
		m_videoYImage = NULL;
		m_videoUImage = NULL;
		m_videoVImage = NULL;
#else
		m_videoMpeg2Frame = NULL;
#endif

		m_audioStream = 0;
#ifdef MPEG2_RAW_ONLY
		m_audioPcmLeftSamples = NULL;
		m_audioPcmRightSamples = NULL;
		m_audioPcmSamples = NULL;
#else
		m_audioFrameData = NULL;
#endif
	}

	bool IsDone() {
		return !m_source;
	}

	float GetProgress() {
		if (m_mpeg2File) {
			return mpeg3_tell_percentage(m_mpeg2File);
		} else {
			return 0.0;
		}
	}

protected:
	int ThreadMain();

	void DoStartVideo();
	void DoStartAudio();
	void DoStopSource();

	bool InitVideo();
	bool InitAudio();

	void ProcessVideo();
	void ProcessAudio();

	bool IsEndOfVideo() {
		return mpeg3_end_of_video(m_mpeg2File, m_videoStream);
	}
	bool IsEndOfAudio() {
		return mpeg3_end_of_audio(m_mpeg2File, m_audioStream);
	}

protected:
	mpeg3_t*			m_mpeg2File;

	int					m_videoStream;
	Duration			m_videoSrcFrameDuration;

#ifdef MPEG2_RAW_ONLY
	u_int8_t*			m_videoYUVImage;
	u_int8_t*			m_videoYImage;
	u_int8_t*			m_videoUImage;
	u_int8_t*			m_videoVImage;
#else
	u_int8_t*			m_videoMpeg2Frame;
	u_int32_t			m_videoSrcMaxFrameLength;
#endif

	int					m_audioStream;

#ifdef MPEG2_RAW_ONLY
	u_int16_t*			m_audioPcmLeftSamples;
	u_int16_t*			m_audioPcmRightSamples;
	u_int16_t*			m_audioPcmSamples;
#else
	u_int8_t*			m_audioFrameData;
	u_int32_t			m_audioSrcMaxFrameLength;
#endif
};

#endif /* __FILE_MPEG2_SOURCE_H__ */

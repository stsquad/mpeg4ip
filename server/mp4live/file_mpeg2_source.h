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

class CMpeg2FileSource : public CMediaSource {
public:
	CMpeg2FileSource() : CMediaSource() {
		m_source = false;
		m_mpeg2File = NULL;
		m_sourceVideo = false;
		m_videoStream = 0;
		m_videoEncoder = NULL;
		m_wantKeyFrame = false;
		m_sourceAudio = false;
	}

	u_int32_t GetNumEncodedFrames() {
		return m_videoFrameNumber;
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

protected:
	bool				m_source;
	mpeg3_t*			m_mpeg2File;

	bool				m_sourceVideo;
	int					m_videoStream;
	CVideoEncoder*		m_videoEncoder;
	u_int32_t			m_videoFrameNumber;
	Duration			m_videoFrameDuration;
	bool				m_wantKeyFrame;
	u_int8_t			m_maxPasses;

	// LATER audio
	bool				m_sourceAudio;
	int					m_audioStream;
	CAudioEncoder*		m_audioEncoder;
};

#endif /* __FILE_MPEG2_SOURCE_H__ */

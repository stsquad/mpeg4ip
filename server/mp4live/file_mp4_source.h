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

#ifndef __FILE_MP4_SOURCE_H__
#define __FILE_MP4_SOURCE_H__

#include "media_source.h"
#include <mp4.h>

class CMp4FileSource : public CMediaSource {
public:
	CMp4FileSource() : CMediaSource() {
		m_mp4File = MP4_INVALID_FILE_HANDLE;
		m_videoTrackId = MP4_INVALID_TRACK_ID;
		m_audioTrackId = MP4_INVALID_TRACK_ID;
	}

	bool IsDone() {
		return !m_source;
	}

	float GetProgress();

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
		return m_videoSrcFrameNumber >= m_videoSrcTotalFrames;
	}
	bool IsEndOfAudio() {
		return m_audioSrcFrameNumber >= m_audioSrcTotalFrames;
	}

protected:
	MP4FileHandle		m_mp4File;

	MP4TrackId			m_videoTrackId;
	u_int32_t			m_videoSrcTotalFrames;

	MP4TrackId			m_audioTrackId;
	u_int32_t			m_audioSrcTotalFrames;
};

#endif /* __FILE_MP4_SOURCE_H__ */

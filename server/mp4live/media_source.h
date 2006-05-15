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
 * Copyright (C) Cisco Systems Inc. 2000-2005.  All Rights Reserved.
 * 
 * Contributor(s): 
 *		Dave Mackie		dmackie@cisco.com
 *		Bill May 		wmay@cisco.com
 */

#ifndef __MEDIA_SOURCE_H__
#define __MEDIA_SOURCE_H__

#include "media_node.h"
#include "media_sink.h"

#include "video_util_resize.h"
#include "resampl.h"
#include "media_feeder.h"
// DEBUG #define DEBUG_VCODEC_SHADOW

// forward declarations
class CAudioEncoder;
class CVideoEncoder;

// virtual parent class for audio and/or video sources
// contains all the common media processing code
class CMediaSource : public CMediaNode, public CMediaFeeder {
public:
	CMediaSource();

	~CMediaSource();
	
	virtual bool IsDone() = 0;

	virtual float GetProgress() = 0;

	void SetVideoSource(CMediaSource* source) {
		m_videoSource = source;
	}

	virtual bool SetPictureControls() {
		// overridden by sources that can manipulate brightness, hue, etc.
		// i.e. video capture cards
		return false;
	}

	virtual bool CanShareSource(void) { return true; };
	void SetAudioSrcSamplesPerFrame(uint32_t samples) {
	  m_audioSrcSamplesPerFrame = samples;
	};
	void RequestKeyFrame (Timestamp t) {
	  m_videoWantKeyFrame = true;
	  m_audioStartTimestamp = t;
	};
	virtual bool PushTextFrame (const char *frame) {
	  return false;
	}

	virtual const char* name() {
	  return "CMediaSource";
	}

protected:
	// Video & Audio support
	virtual bool IsEndOfVideo() { 
		return true;
	}
	virtual bool IsEndOfAudio() {
		return true;
	}

	// Video

	bool InitVideo(
		bool realTime = true);

	void SetVideoSrcSize(
		u_int16_t srcWidth,
		u_int16_t srcHeight,
		u_int16_t srcStride);

	// Audio

	bool InitAudio(
		bool realTime);

	bool SetAudioSrc(
		MediaType srcType,
		u_int8_t srcChannels,
		u_int32_t srcSampleRate);

	// audio utility routines

	inline Duration SrcSamplesToTicks(u_int64_t numSamples) {
		return (numSamples * TimestampTicks) / m_audioSrcSampleRate;
	}

	inline u_int32_t SrcSamplesToBytes(u_int64_t numSamples) {
		return (numSamples * m_audioSrcChannels * sizeof(u_int16_t));
	}

	inline u_int64_t SrcBytesToSamples(u_int32_t numBytes) {
		return (numBytes / (m_audioSrcChannels * sizeof(u_int16_t)));
	}

protected:
	static const uint32_t MSG_SOURCE	= 2048;
	static const uint32_t MSG_SOURCE_KEY_FRAME	= MSG_SOURCE + 3;

	// generic media info
	bool			m_source;
	bool			m_sourceRealTime;

	// video source info
	CMediaSource*	m_videoSource; 
	u_int16_t		m_videoSrcWidth;
	u_int16_t		m_videoSrcHeight;
	u_int16_t		m_videoSrcAdjustedHeight;
	float			m_videoSrcAspectRatio;
	u_int32_t		m_videoSrcYUVSize;
	u_int32_t		m_videoSrcYSize;
	u_int16_t		m_videoSrcYStride;
	u_int32_t		m_videoSrcUVSize;
	u_int16_t		m_videoSrcUVStride;

	// video resizing info
	bool			m_videoWantKeyFrame;
	Timestamp               m_audioStartTimestamp;

	// audio source info 
	u_int8_t		m_audioSrcChannels;
	u_int32_t		m_audioSrcSampleRate;
	u_int16_t		m_audioSrcSamplesPerFrame;
	u_int64_t		m_audioSrcSampleNumber;
	u_int32_t		m_audioSrcFrameNumber;
};

#endif /* __MEDIA_SOURCE_H__ */

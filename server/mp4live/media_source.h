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
 *		Bill May 		wmay@cisco.com
 */

#ifndef __MEDIA_SOURCE_H__
#define __MEDIA_SOURCE_H__

#include "media_node.h"
#include "media_sink.h"

#include "video_util_resize.h"

// forward declarations
class CAudioEncoder;
class CVideoEncoder;

class CMediaSource : public CMediaNode {
public:
	CMediaSource() : CMediaNode() {
		m_pSinksMutex = SDL_CreateMutex();
		if (m_pSinksMutex == NULL) {
			debug_message("CreateMutex error");
		}
		for (int i = 0; i < MAX_SINKS; i++) {
			m_sinks[i] = NULL;
		}
	}

	~CMediaSource() {
		SDL_DestroyMutex(m_pSinksMutex);
		m_pSinksMutex = NULL;
	}
	
	bool AddSink(CMediaSink* pSink);

	void RemoveSink(CMediaSink* pSink);

	void RemoveAllSinks(void);

	void StartVideo(void) {
		m_myMsgQueue.send_message(MSG_SOURCE_START_VIDEO,
			NULL, 0, m_myMsgQueueSemaphore);
	}

	void StartAudio(void) {
		m_myMsgQueue.send_message(MSG_SOURCE_START_AUDIO,
			NULL, 0, m_myMsgQueueSemaphore);
	}

	void GenerateKeyFrame(void) {
		m_myMsgQueue.send_message(MSG_SOURCE_KEY_FRAME,
			NULL, 0, m_myMsgQueueSemaphore);
	}

	virtual bool IsDone() = NULL;

	virtual Duration GetElapsedDuration() = NULL;

	virtual float GetProgress() = NULL;

	virtual u_int32_t GetNumEncodedVideoFrames() {
		return m_videoDstFrameNumber;
	}

	virtual u_int32_t GetNumEncodedAudioFrames() {
		return 0;
	}

protected:
	void ForwardFrame(CMediaFrame* pFrame) {
		if (SDL_LockMutex(m_pSinksMutex) == -1) {
			debug_message("ForwardFrame LockMutex error");
			return;
		}
		for (int i = 0; i < MAX_SINKS; i++) {
			if (m_sinks[i] == NULL) {
				break;
			}
			m_sinks[i]->EnqueueFrame(pFrame);
		}
		if (SDL_UnlockMutex(m_pSinksMutex) == -1) {
			debug_message("UnlockMutex error");
		}
	}

	void ForwardEncodedAudioFrames(
		CAudioEncoder* encoder,
		Timestamp baseTimestamp,
		u_int32_t* pNumSamples,
		u_int32_t* pNumFrames);

	Duration SamplesToTicks(u_int32_t numSamples) {
		return (numSamples * TimestampTicks)
			/ m_pConfig->GetIntegerValue(CONFIG_AUDIO_SAMPLE_RATE);
	}

	bool InitVideo(
		float srcFrameRate, 
		u_int16_t srcWidth,
		u_int16_t srcHeight,
		bool matchAspectRatios = true,
		bool realTime = true);

	void ProcessVideoFrame(
		u_int8_t* image,
		Timestamp frameTimestamp);

	bool WillUseVideoFrame();

	void DoGenerateKeyFrame() {
		m_videoWantKeyFrame = true;
	}

	void ShutdownVideo();

protected:
	static const int MSG_SOURCE_START_VIDEO	= 1;
	static const int MSG_SOURCE_START_AUDIO	= 2;
	static const int MSG_SOURCE_KEY_FRAME	= 3;

	// sink info
	static const u_int16_t MAX_SINKS = 8;
	CMediaSink* m_sinks[MAX_SINKS];
	SDL_mutex*	m_pSinksMutex;

	bool			m_realTime;

	// video source info
	float			m_videoSrcFrameRate;
	Duration		m_videoSrcFrameDuration;
	u_int32_t		m_videoSrcFrameNumber;
	u_int16_t		m_videoSrcWidth;
	u_int16_t		m_videoSrcHeight;
	u_int16_t		m_videoSrcAdjustedHeight;
	float			m_videoSrcAspectRatio;
	u_int32_t		m_videoSrcYUVSize;
	u_int32_t		m_videoSrcYSize;
	u_int32_t		m_videoSrcUVSize;
	u_int32_t		m_videoSrcYCrop;
	u_int32_t		m_videoSrcUVCrop;

	// video destination info
	float			m_videoDstFrameRate;
	Duration		m_videoDstFrameDuration;
	u_int32_t		m_videoDstFrameNumber;
	u_int16_t		m_videoDstWidth;
	u_int16_t		m_videoDstHeight;
	float			m_videoDstAspectRatio;
	u_int32_t		m_videoDstYUVSize;
	u_int32_t		m_videoDstYSize;
	u_int32_t		m_videoDstUVSize;

	// video resizing info
	image_t*		m_videoSrcYImage;
	image_t*		m_videoDstYImage;
	scaler_t*		m_videoYResizer;
	image_t*		m_videoSrcUVImage;
	image_t*		m_videoDstUVImage;
	scaler_t*		m_videoUVResizer;

	// video encoding info
	CVideoEncoder*	m_videoEncoder;
	bool			m_videoWantKeyFrame;

	// video timing info
	Timestamp		m_videoStartTimestamp;
	u_int32_t		m_videoSkippedFrames;
	Duration		m_videoEncodingDrift;
	Duration		m_videoEncodingMaxDrift;
	Duration		m_videoDstElapsedDuration;

	// video previous frame info
	u_int8_t*		m_videoDstPrevImage;
	u_int8_t*		m_videoDstPrevReconstructImage;
	u_int8_t*		m_videoDstPrevFrame;
	u_int32_t		m_videoDstPrevFrameLength;
};

#endif /* __MEDIA_SOURCE_H__ */


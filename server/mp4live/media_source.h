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
#include "resampl.h"
// DEBUG #define DEBUG_VCODEC_SHADOW

// forward declarations
class CAudioEncoder;
class CVideoEncoder;

// virtual parent class for audio and/or video sources
// contains all the common media processing code

class CMediaSource : public CMediaNode {
public:
	CMediaSource();

	~CMediaSource();
	
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

	virtual Duration GetElapsedDuration();

	virtual float GetProgress() = NULL;

	virtual u_int32_t GetNumEncodedVideoFrames() {
		return m_videoEncodedFrames;
	}

	virtual u_int32_t GetNumEncodedAudioFrames() {
		return m_audioDstFrameNumber;
	}

	void SetVideoSource(CMediaSource* source) {
		m_videoSource = source;
	}

	void AddEncodingDrift(Duration drift) {
		// currently there is no thread contention
		// for this function, so we omit a mutex
		m_otherTotalDrift += drift;
	}
	void SubtractEncodingDrift(Duration drift) {
	  if (m_otherTotalDrift > 0) {
	    if (m_otherTotalDrift <= drift) {
	      m_otherTotalDrift = 0;
	    } else {
	      m_otherTotalDrift -= drift;
	    }
	  }
	}

	virtual bool SetPictureControls() {
		// overridden by sources that can manipulate brightness, hue, etc.
		// i.e. video capture cards
		return false;
	}

protected:
	// Video & Audio support
	virtual bool IsEndOfVideo() { 
		return true;
	}
	virtual bool IsEndOfAudio() {
		return true;
	}

	void ForwardFrame(CMediaFrame* pFrame);

	void DoStopSource();


	// Video

	bool InitVideo(
		MediaType srcType,
		bool realTime = true);

	void SetVideoSrcSize(
		u_int16_t srcWidth,
		u_int16_t srcHeight,
		u_int16_t srcStride,
		bool matchAspectRatios = false);

	void SetVideoSrcStride(
		u_int16_t srcStride);

	void ProcessVideoYUVFrame(
		u_int8_t* pY,
		u_int8_t* pU,
		u_int8_t* pV,
		u_int16_t yStride,
		u_int16_t uvStride,
		Timestamp frameTimestamp);

	void DoGenerateKeyFrame() {
		m_videoWantKeyFrame = true;
	}

	void DestroyVideoResizer();

	void DoStopVideo();

	// Audio

	bool InitAudio(
		bool realTime);

	bool SetAudioSrc(
		MediaType srcType,
		u_int8_t srcChannels,
		u_int32_t srcSampleRate);

	void ProcessAudioFrame(
		u_int8_t* frameData,
		u_int32_t frameDataLength,
		Timestamp frameTimestamp,
		bool resync);

	void ResampleAudio(
		u_int8_t* frameData,
		u_int32_t frameDataLength);

	void ForwardEncodedAudioFrames(void);

	void DoStopAudio();

	// audio utility routines

	Duration SrcSamplesToTicks(u_int64_t numSamples) {
		return (numSamples * TimestampTicks) / m_audioSrcSampleRate;
	}

	Duration DstSamplesToTicks(u_int64_t numSamples) {
		return (numSamples * TimestampTicks) / m_audioDstSampleRate;
	}

	u_int32_t SrcTicksToSamples(Duration duration) {
		return (duration * m_audioSrcSampleRate) / TimestampTicks;
	}

	u_int32_t DstTicksToSamples(Duration duration) {
		return (duration * m_audioDstSampleRate) / TimestampTicks;
	}

	u_int32_t SrcSamplesToBytes(u_int64_t numSamples) {
		return (numSamples * m_audioSrcChannels * sizeof(u_int16_t));
	}

	u_int32_t DstSamplesToBytes(u_int64_t numSamples) {
		return (numSamples * m_audioDstChannels * sizeof(u_int16_t));
	}

	u_int64_t SrcBytesToSamples(u_int32_t numBytes) {
		return (numBytes / (m_audioSrcChannels * sizeof(u_int16_t)));
	}

	u_int64_t DstBytesToSamples(u_int32_t numBytes) {
		return (numBytes / (m_audioDstChannels * sizeof(u_int16_t)));
	}

	Duration VideoDstFramesToDuration(void) {
	  double tempd;
	  tempd = m_videoDstFrameNumber;
	  tempd *= TimestampTicks;
	  tempd /= m_videoDstFrameRate;
	  return (Duration) tempd;
	};

	void AddGapToAudio(Timestamp startTimestamp, Duration silenceDuration);
	  
protected:
	static const int MSG_SOURCE	= 2048;
	static const int MSG_SOURCE_START_VIDEO	= MSG_SOURCE + 1;
	static const int MSG_SOURCE_START_AUDIO	= MSG_SOURCE + 2;
	static const int MSG_SOURCE_KEY_FRAME	= MSG_SOURCE + 3;

	// sink info
	static const u_int16_t MAX_SINKS = 8;
	CMediaSink* m_sinks[MAX_SINKS];
	SDL_mutex*	m_pSinksMutex;

	// generic media info
	bool			m_source;
	bool			m_sourceVideo;
	bool			m_sourceAudio;
	bool			m_sourceRealTime;
	bool			m_sinkRealTime;
	Timestamp		m_encodingStartTimestamp;
	Duration		m_maxAheadDuration;

	// video source info
	CMediaSource*	m_videoSource; 
	MediaType		m_videoSrcType;
	u_int32_t		m_videoSrcFrameNumber;
	u_int16_t		m_videoSrcWidth;
	u_int16_t		m_videoSrcHeight;
	u_int16_t		m_videoSrcAdjustedHeight;
	float			m_videoSrcAspectRatio;
	u_int32_t		m_videoSrcYUVSize;
	u_int32_t		m_videoSrcYSize;
	u_int16_t		m_videoSrcYStride;
	u_int32_t		m_videoSrcUVSize;
	u_int16_t		m_videoSrcUVStride;
	u_int32_t		m_videoSrcYCrop;
	u_int32_t		m_videoSrcUVCrop;

	// video destination info
	MediaType		m_videoDstType;
	float			m_videoDstFrameRate;
	Duration		m_videoDstFrameDuration;
	u_int32_t		m_videoDstFrameNumber;
	u_int32_t               m_videoEncodedFrames;
	u_int16_t		m_videoDstWidth;
	u_int16_t		m_videoDstHeight;
	float			m_videoDstAspectRatio;
	u_int32_t		m_videoDstYUVSize;
	u_int32_t		m_videoDstYSize;
	u_int32_t		m_videoDstUVSize;

	// video resizing info
	bool			m_videoMatchAspectRatios;
	image_t*		m_videoSrcYImage;
	image_t*		m_videoDstYImage;
	scaler_t*		m_videoYResizer;
	image_t*		m_videoSrcUVImage;
	image_t*		m_videoDstUVImage;
	scaler_t*		m_videoUVResizer;

	// video encoding info
	CVideoEncoder*	m_videoEncoder;
#ifdef DEBUG_VCODEC_SHADOW
	CVideoEncoder*	m_videoEncoderShadow;
#endif
	bool			m_videoWantKeyFrame;

	// video timing info
	Timestamp		m_videoStartTimestamp;
	Duration		m_videoEncodingDrift;
	Duration		m_videoEncodingMaxDrift;
	Duration                m_videoSrcPrevElapsedDuration;
	Duration		m_videoSrcElapsedDuration;
	Duration		m_videoDstElapsedDuration;
	Duration		m_otherTotalDrift;
	Duration		m_otherLastTotalDrift;

	// video previous frame info
	u_int8_t*		m_videoDstPrevImage;
	u_int8_t*		m_videoDstPrevReconstructImage;
	u_int8_t*		m_videoDstPrevFrame;
	u_int32_t		m_videoDstPrevFrameLength;
	Timestamp               m_videoDstPrevFrameTimestamp;
	Duration                m_videoDstPrevFrameElapsedDuration;


	// audio source info 
	MediaType		m_audioSrcType;
	u_int8_t		m_audioSrcChannels;
	u_int32_t		m_audioSrcSampleRate;
	u_int16_t		m_audioSrcSamplesPerFrame;
	u_int64_t		m_audioSrcSampleNumber;
	u_int32_t		m_audioSrcFrameNumber;

	// audio resampling info
	resample_t              *m_audioResample;

	// audio destination info
	MediaType		m_audioDstType;
	u_int8_t		m_audioDstChannels;
	u_int32_t		m_audioDstSampleRate;
	u_int16_t		m_audioDstSamplesPerFrame;
	u_int64_t		m_audioDstSampleNumber;
	u_int32_t		m_audioDstFrameNumber;
	u_int64_t		m_audioDstRawSampleNumber;
	u_int32_t		m_audioDstRawFrameNumber;

	// audio encoding info
	u_int8_t*		m_audioPreEncodingBuffer;
	u_int32_t		m_audioPreEncodingBufferLength;
	u_int32_t		m_audioPreEncodingBufferMaxLength;
	CAudioEncoder*	m_audioEncoder;

	// audio timing info
	Timestamp		m_audioStartTimestamp;
	Timestamp               m_audioEncodingStartTimestamp;
	Duration		m_audioSrcElapsedDuration;
	Duration		m_audioDstElapsedDuration;
};

#endif /* __MEDIA_SOURCE_H__ */


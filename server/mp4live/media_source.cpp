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

#include "mp4live.h"
#include "media_source.h"
#include "audio_encoder.h"
#include "video_encoder.h"
#include "video_util_rgb.h"


bool CMediaSource::AddSink(CMediaSink* pSink) 
{
	bool rc = false;

	if (SDL_LockMutex(m_pSinksMutex) == -1) {
		debug_message("AddSink LockMutex error");
		return rc;
	}
	for (int i = 0; i < MAX_SINKS; i++) {
		if (m_sinks[i] == NULL) {
			m_sinks[i] = pSink;
			rc = true;
			break;
		}
	}
	if (SDL_UnlockMutex(m_pSinksMutex) == -1) {
		debug_message("UnlockMutex error");
	}
	return rc;
}

void CMediaSource::RemoveSink(CMediaSink* pSink) 
{
	if (SDL_LockMutex(m_pSinksMutex) == -1) {
		debug_message("RemoveSink LockMutex error");
		return;
	}
	for (int i = 0; i < MAX_SINKS; i++) {
		if (m_sinks[i] == pSink) {
			int j;
			for (j = i; j < MAX_SINKS - 1; j++) {
				m_sinks[j] = m_sinks[j+1];
			}
			m_sinks[j] = NULL;
			break;
		}
	}
	if (SDL_UnlockMutex(m_pSinksMutex) == -1) {
		debug_message("UnlockMutex error");
	}
}

void CMediaSource::RemoveAllSinks(void) 
{
	if (SDL_LockMutex(m_pSinksMutex) == -1) {
		debug_message("RemoveAllSinks LockMutex error");
		return;
	}
	for (int i = 0; i < MAX_SINKS; i++) {
		if (m_sinks[i] == NULL) {
			break;
		}
		m_sinks[i] = NULL;
	}
	if (SDL_UnlockMutex(m_pSinksMutex) == -1) {
		debug_message("UnlockMutex error");
	}
}

void CMediaSource::ForwardEncodedAudioFrames(
	CAudioEncoder* encoder,
	Timestamp baseTimestamp,
	u_int32_t* pNumSamples,
	u_int32_t* pNumFrames)
{
	u_int8_t* pFrame;
	u_int32_t frameLength;
	u_int32_t frameNumSamples;

	(*pNumSamples) = 0;
	(*pNumFrames) = 0;

	while (encoder->GetEncodedSamples(
	  &pFrame, &frameLength, &frameNumSamples)) {

		// sanity check
		if (pFrame == NULL || frameLength == 0) {
			break;
		}

		// forward the encoded frame to sinks
		CMediaFrame* pMediaFrame =
			new CMediaFrame(
				encoder->GetFrameType(),
				pFrame, 
				frameLength,
				baseTimestamp + SamplesToTicks(frameNumSamples),
				frameNumSamples,
				m_pConfig->GetIntegerValue(CONFIG_AUDIO_SAMPLE_RATE));
		ForwardFrame(pMediaFrame);
		delete pMediaFrame;

		(*pNumSamples) += frameNumSamples;
		(*pNumFrames)++;
	}
}

bool CMediaSource::InitVideo(
	float srcFrameRate, 
	u_int16_t srcWidth,
	u_int16_t srcHeight,
	bool matchAspectRatios,
	bool realTime)
{
	m_videoSrcFrameRate = srcFrameRate;
	m_videoSrcFrameDuration = 
		(Duration)(((float)TimestampTicks / m_videoSrcFrameRate) + 0.5);
	m_videoSrcFrameNumber = 0;
	m_videoSrcWidth = srcWidth;
	m_videoSrcHeight = srcHeight;
	m_videoSrcAspectRatio = (float)srcWidth / (float)srcHeight;
	// these next three may change below
	m_videoSrcAdjustedHeight = srcHeight;
	m_videoSrcYCrop = 0;
	m_videoSrcUVCrop = 0;

	m_videoDstFrameRate =
		m_pConfig->GetFloatValue(CONFIG_VIDEO_FRAME_RATE);
	m_videoDstFrameDuration = 
		(Duration)(((float)TimestampTicks / m_videoDstFrameRate) + 0.5);
	m_videoDstFrameNumber = 0;
	m_videoDstWidth =
		m_pConfig->m_videoWidth;
	m_videoDstHeight =
		m_pConfig->m_videoHeight;
	m_videoDstAspectRatio = 
		(float)m_pConfig->m_videoWidth / (float)m_pConfig->m_videoHeight;
	m_videoDstYSize = m_videoDstWidth * m_videoDstHeight;
	m_videoDstUVSize = m_videoDstYSize / 4;
	m_videoDstYUVSize = (m_videoDstYSize * 3) / 2;

	// match aspect ratios
	if (matchAspectRatios 
	  && fabs(m_videoSrcAspectRatio - m_videoDstAspectRatio) > 0.01) {

		m_videoSrcAdjustedHeight =
			(u_int16_t)(m_videoSrcWidth / m_videoDstAspectRatio);
		if ((m_videoSrcAdjustedHeight % 16) != 0) {
			m_videoSrcAdjustedHeight += 16 - (m_videoSrcAdjustedHeight % 16);
		}

		if (m_videoSrcAspectRatio < m_videoDstAspectRatio) {
			// crop src
			m_videoSrcYCrop = m_videoSrcWidth * 
				((m_videoSrcHeight - m_videoSrcAdjustedHeight) / 2);
			m_videoSrcUVCrop = m_videoSrcYCrop / 4;
		}
	}

	m_videoSrcYSize = m_videoSrcWidth 
		* MAX(m_videoSrcHeight, m_videoSrcAdjustedHeight);
	m_videoSrcUVSize = m_videoSrcYSize / 4;
	m_videoSrcYUVSize = (m_videoSrcYSize * 3) / 2;

	// intialize encoder
	m_videoEncoder = VideoEncoderCreate(
		m_pConfig->GetStringValue(CONFIG_VIDEO_ENCODER));

	if (!m_videoEncoder) {
		return false;
	}
	if (!m_videoEncoder->Init(m_pConfig)) {
		return false;
	}

	// initial resizing
	if (m_videoSrcWidth != m_videoDstWidth 
	  || m_videoSrcAdjustedHeight != m_videoDstHeight) {

		m_videoSrcYImage = 
			scale_new_image(m_videoSrcWidth, 
				m_videoSrcAdjustedHeight, 1);
		m_videoDstYImage = 
			scale_new_image(m_videoDstWidth, 
				m_videoDstHeight, 1);
		m_videoYResizer = 
			scale_image_init(m_videoDstYImage, m_videoSrcYImage, 
				Bell_filter, Bell_support);

		m_videoSrcUVImage = 
			scale_new_image(m_videoSrcWidth / 2, 
				m_videoSrcAdjustedHeight / 2, 1);
		m_videoDstUVImage = 
			scale_new_image(m_videoDstWidth / 2, 
				m_videoDstHeight / 2, 1);
		m_videoUVResizer = 
			scale_image_init(m_videoDstUVImage, m_videoSrcUVImage, 
				Bell_filter, Bell_support);
	}

	m_videoWantKeyFrame = true;
	m_realTime = realTime;
	m_videoStartTimestamp = GetTimestamp();
	m_videoSkippedFrames = 0;
	m_videoEncodingDrift = 0;
	m_videoEncodingMaxDrift = m_videoDstFrameDuration;
	m_videoDstElapsedDuration = 0;

	m_videoDstPrevImage = NULL;
	m_videoDstPrevReconstructImage = NULL;
	m_videoDstPrevFrame = NULL;
	m_videoDstPrevFrameLength = 0;

	return true;
}

bool CMediaSource::WillUseVideoFrame()
{
	return m_videoDstElapsedDuration
		< (m_videoSrcFrameNumber + 1) * m_videoSrcFrameDuration;
}

void CMediaSource::ProcessVideoFrame(
	u_int8_t* image,
	Timestamp frameTimestamp)
{
	m_videoSrcFrameNumber++;

	Duration elapsedTime = 
		frameTimestamp - m_videoStartTimestamp;

	// drop src frames as needed to match target frame rate
	if (m_videoDstElapsedDuration >= elapsedTime + m_videoSrcFrameDuration) {
		return;
	}

	// if we're running in real-time mode
	// check if we are falling behind due to encoding speed
	if (m_realTime && m_videoEncodingDrift >= m_videoEncodingMaxDrift) {
		if (m_videoEncodingDrift <= m_videoDstFrameDuration) {
			m_videoEncodingDrift = 0;
		} else {
			m_videoEncodingDrift -= m_videoDstFrameDuration;
		}

		// skip this frame			
		m_videoSkippedFrames++;
		return;
	}

	u_int8_t* yuvImage;
	bool mallocedYuvImage;

	Duration encodingStartTimestamp = GetTimestamp();

	// perform colorspace conversion if necessary
	if (m_pConfig->m_videoNeedRgbToYuv) {
		yuvImage = (u_int8_t*)Malloc(m_videoSrcYUVSize);
		mallocedYuvImage = true;

		RGB2YUV(
			m_videoSrcWidth,
			m_videoSrcHeight,
			image,
			yuvImage,
			yuvImage + m_videoSrcYSize,
			yuvImage + m_videoSrcYSize + m_videoSrcUVSize,
			1);
	} else {
		yuvImage = image;
		mallocedYuvImage = false;
	}

	// crop to desired aspect ratio (may be a no-op)
	u_int8_t* yImage = yuvImage + m_videoSrcYCrop;
	u_int8_t* uImage = yuvImage + m_videoSrcYSize + m_videoSrcUVCrop;
	u_int8_t* vImage = uImage + m_videoSrcUVSize;

	// Note: caller is responsible for adding any padding that is needed

	// resize image if necessary
	if (m_videoYResizer) {
		u_int8_t* resizedYUV = (u_int8_t*)Malloc(m_videoDstYUVSize);
		
		u_int8_t* resizedY = 
			resizedYUV;
		u_int8_t* resizedU = 
			resizedYUV + m_videoDstYSize;
		u_int8_t* resizedV = 
			resizedYUV + m_videoDstYSize + m_videoDstUVSize;

		m_videoSrcYImage->data = yImage;
		m_videoDstYImage->data = resizedY;
		scale_image_process(m_videoYResizer);

		m_videoSrcUVImage->data = uImage;
		m_videoDstUVImage->data = resizedU;
		scale_image_process(m_videoUVResizer);

		m_videoSrcUVImage->data = vImage;
		m_videoDstUVImage->data = resizedV;
		scale_image_process(m_videoUVResizer);

		// done with the original source image
		if (mallocedYuvImage) {
			free(yuvImage);
		}

		// switch over to resized version
		yuvImage = resizedYUV;
		yImage = resizedY;
		uImage = resizedU;
		vImage = resizedV;
		mallocedYuvImage = true;
	}

	// if we want encoded video frames
	if (m_pConfig->m_videoEncode) {

		// call video encoder
		bool rc = m_videoEncoder->EncodeImage(
			yImage, uImage, vImage, m_videoWantKeyFrame);

		if (!rc) {
			debug_message("Can't encode image!");
			if (mallocedYuvImage) {
				free(yuvImage);
			}
			return;
		}

		// clear want key frame flag
		m_videoWantKeyFrame = false;
	}

	Timestamp dstPrevFrameTimestamp =
		m_videoStartTimestamp + m_videoDstElapsedDuration;

	// calculate previous frame duration
	Duration dstPrevFrameDuration = m_videoDstFrameDuration;
	m_videoDstElapsedDuration += m_videoDstFrameDuration;

	if (m_realTime && m_videoSrcFrameNumber > 0) {

		// first adjust due to skipped frames
		Duration dstPrevFrameAdjustment = 
			m_videoSkippedFrames * m_videoDstFrameDuration;

		dstPrevFrameDuration += dstPrevFrameAdjustment;
		m_videoDstElapsedDuration += dstPrevFrameAdjustment;

		// check our duration against real elasped time
		Duration lag = elapsedTime - m_videoDstElapsedDuration;

		if (lag > 0) {
			// adjust by integral number of target duration units
			dstPrevFrameAdjustment = 
				(lag / m_videoDstFrameDuration) * m_videoDstFrameDuration;

			dstPrevFrameDuration += dstPrevFrameAdjustment;
			m_videoDstElapsedDuration += dstPrevFrameAdjustment;
		}
	}

	// forward encoded video to sinks
	if (m_pConfig->m_videoEncode) {
		if (m_videoDstPrevFrame) {
			CMediaFrame* pFrame = new CMediaFrame(
				CMediaFrame::Mpeg4VideoFrame, 
				m_videoDstPrevFrame, 
				m_videoDstPrevFrameLength,
				dstPrevFrameTimestamp, 
				dstPrevFrameDuration);
			ForwardFrame(pFrame);
			delete pFrame;
		}

		// hold onto this encoded vop until next one is ready
		m_videoEncoder->GetEncodedImage(
			&m_videoDstPrevFrame, &m_videoDstPrevFrameLength);
	}

	// forward raw video to sinks
	if (m_pConfig->GetBoolValue(CONFIG_VIDEO_RAW_PREVIEW)
	  || m_pConfig->GetBoolValue(CONFIG_RECORD_RAW_VIDEO)) {

		if (m_videoDstPrevImage) {
			CMediaFrame* pFrame =
				new CMediaFrame(
					CMediaFrame::RawYuvVideoFrame, 
					m_videoDstPrevImage, 
					m_videoDstYUVSize,
					dstPrevFrameTimestamp, 
					dstPrevFrameDuration);
			ForwardFrame(pFrame);
			delete pFrame;
		}

		m_videoDstPrevImage = (u_int8_t*)Malloc(m_videoDstYUVSize);

		memcpy(m_videoDstPrevImage, 
			yImage, 
			m_videoDstYSize);
		memcpy(m_videoDstPrevImage + m_videoDstYSize,
			uImage, 
			m_videoDstUVSize);
		memcpy(m_videoDstPrevImage + m_videoDstYSize + m_videoDstUVSize,
			vImage, 
			m_videoDstUVSize);
	}

	// forward reconstructed video to sinks
	if (m_pConfig->m_videoEncode
	  && m_pConfig->GetBoolValue(CONFIG_VIDEO_ENCODED_PREVIEW)) {

		if (m_videoDstPrevReconstructImage) {
			CMediaFrame* pFrame =
				new CMediaFrame(CMediaFrame::ReconstructYuvVideoFrame, 
					m_videoDstPrevReconstructImage, 
					m_videoDstYUVSize,
					dstPrevFrameTimestamp, 
					dstPrevFrameDuration);
			ForwardFrame(pFrame);
			delete pFrame;
		}

		m_videoDstPrevReconstructImage = 
			(u_int8_t*)Malloc(m_videoDstYUVSize);

		m_videoEncoder->GetReconstructedImage(
			m_videoDstPrevReconstructImage,
			m_videoDstPrevReconstructImage 
				+ m_videoDstYSize,
			m_videoDstPrevReconstructImage
				+ m_videoDstYSize + m_videoDstUVSize);
	}

	m_videoDstFrameNumber++;

	// calculate how we're doing versus target frame rate
	// this is used to decide if we need to drop frames
	if (m_realTime) {
		// reset skipped frames
		m_videoSkippedFrames = 0;

		// TBD 
		Duration drift =
			(GetTimestamp() - encodingStartTimestamp) 
			- m_videoDstFrameDuration;

		m_videoEncodingDrift += drift;
		if (m_videoEncodingDrift < 0) {
			m_videoEncodingDrift = 0;
		}
	}

	if (mallocedYuvImage) {
		free(yuvImage);
	}
	return;
}

void CMediaSource::ShutdownVideo()
{
	if (m_videoSrcYImage) {
		scale_free_image(m_videoSrcYImage);
		m_videoSrcYImage = NULL;
	}
	if (m_videoDstYImage) {
		scale_free_image(m_videoDstYImage);
		m_videoDstYImage = NULL;
	}
	if (m_videoYResizer) {
		scale_image_done(m_videoYResizer);
		m_videoYResizer = NULL;
	}
	if (m_videoSrcUVImage) {
		scale_free_image(m_videoSrcUVImage);
		m_videoSrcUVImage = NULL;
	}
	if (m_videoDstUVImage) {
		scale_free_image(m_videoDstUVImage);
		m_videoDstUVImage = NULL;
	}
	if (m_videoUVResizer) {
		scale_image_done(m_videoUVResizer);
		m_videoUVResizer = NULL;
	}

	if (m_videoEncoder) {
		m_videoEncoder->Stop();
		delete m_videoEncoder;
		m_videoEncoder = NULL;
	}
}


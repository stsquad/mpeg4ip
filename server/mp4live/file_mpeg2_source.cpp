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
#include "file_mpeg2_source.h"


int CMpeg2FileSource::ThreadMain(void) 
{
	while (true) {
		int rc;

		if (m_source) {
			rc = SDL_SemTryWait(m_myMsgQueueSemaphore);
		} else {
			rc = SDL_SemWait(m_myMsgQueueSemaphore);
		}

		// semaphore error
		if (rc == -1) {
			break;
		} 

		// message pending
		if (rc == 0) {
			CMsg* pMsg = m_myMsgQueue.get_message();
		
			if (pMsg != NULL) {
				switch (pMsg->get_value()) {
				case MSG_NODE_STOP_THREAD:
					DoStopSource();	// ensure things get cleaned up
					delete pMsg;
					return 0;
				case MSG_NODE_START:
					DoStartVideo();
					DoStartAudio();
					break;
				case MSG_SOURCE_START_VIDEO:
					DoStartVideo();
					break;
				case MSG_SOURCE_START_AUDIO:
					DoStartAudio();
					break;
				case MSG_NODE_STOP:
					DoStopSource();
					break;
				case MSG_SOURCE_KEY_FRAME:
					DoGenerateKeyFrame();
					break;
				}

				delete pMsg;
			}
		}

		if (m_source) {
			ProcessMedia();
		}
	}

	return -1;
}

void CMpeg2FileSource::DoStartVideo(void)
{
	if (m_sourceVideo) {
		return;
	}

	if (!m_source) {
		m_mpeg2File = mpeg3_open(
			m_pConfig->GetStringValue(CONFIG_VIDEO_SOURCE_NAME));

		if (!m_mpeg2File) {
			return;
		}

#ifdef USE_MMX
		mpeg3_set_mmx(m_mpeg2File, true);
#endif
	}

	if (!InitVideo()) {
		return;
	}

	m_sourceVideo = true;
	m_source = true;
}

void CMpeg2FileSource::DoStartAudio(void)
{
	if (m_sourceAudio) {
		return;
	}

	if (!m_source) {
		m_mpeg2File = mpeg3_open(
			m_pConfig->GetStringValue(CONFIG_AUDIO_SOURCE_NAME));

		if (!m_mpeg2File) {
			return;
		}

#ifdef USE_MMX
		mpeg3_set_mmx(m_mpeg2File, true);
#endif
	}

	if (!InitAudio()) {
		return;
	}

	m_sourceAudio = true;
	m_source = true;
}

void CMpeg2FileSource::DoStopSource(void)
{
	if (!m_source) {
		return;
	}

	if (m_videoEncoder) {
		m_videoEncoder->Stop();
		delete m_videoEncoder;
		m_videoEncoder = NULL;
	}

	if (m_audioEncoder) {
		m_audioEncoder->Stop();
		delete m_audioEncoder;
		m_audioEncoder = NULL;
	}

	mpeg3_close(m_mpeg2File);
	m_mpeg2File = NULL;

	m_source = false;
}

bool CMpeg2FileSource::InitVideo(void)
{
	if (!mpeg3_has_video(m_mpeg2File)) {
		debug_message("mpeg2 file doesn't contain video\n");
		return false;
	}

	m_videoStream = 
		m_pConfig->GetIntegerValue(CONFIG_VIDEO_SOURCE_TRACK);

	// Force our config to use these for now
	// LATER will resample if input format != output format
	m_pConfig->m_videoWidth =
		mpeg3_video_width(m_mpeg2File, m_videoStream);
	m_pConfig->m_videoHeight =
		mpeg3_video_height(m_mpeg2File, m_videoStream);

	m_pConfig->m_ySize = m_pConfig->m_videoWidth * m_pConfig->m_videoHeight;
	m_pConfig->m_uvSize = m_pConfig->m_ySize / 4;
	m_pConfig->m_yuvSize = (m_pConfig->m_ySize * 3) / 2;

	float fps =
		mpeg3_frame_rate(m_mpeg2File, m_videoStream);

	m_maxPasses = (u_int8_t)(fps + 0.5);

	m_pConfig->SetIntegerValue(CONFIG_VIDEO_FRAME_RATE, m_maxPasses);

	m_videoFrameDuration = (Duration)(((float)TimestampTicks / fps) + 0.5);

	GenerateMpeg4VideoConfig(m_pConfig);

	// END LATER block

	m_videoEncoder = VideoEncoderCreate(
		m_pConfig->GetStringValue(CONFIG_VIDEO_ENCODER));

	if (!m_videoEncoder) {
		return false;
	}
	if (!m_videoEncoder->Init(m_pConfig)) {
		return false;
	}

	m_videoFrameNumber = 0;

	return true;
}

bool CMpeg2FileSource::InitAudio(void)
{
	if (!mpeg3_has_audio(m_mpeg2File)) {
		debug_message("mpeg2 file doesn't contain audio\n");
		return false;
	}

	m_audioStream = 
		m_pConfig->GetIntegerValue(CONFIG_AUDIO_SOURCE_TRACK);

	// LATER

	return true;
}

void CMpeg2FileSource::DoGenerateKeyFrame(void)
{
	m_wantKeyFrame = true;
}

void CMpeg2FileSource::ProcessMedia(void)
{
	// TBD audio

	int rc = 0;

	// for efficiency, process 1 second before returning to check for commands
	for (u_int8_t pass = 0; pass < m_maxPasses; pass++) {

		if (mpeg3_end_of_video(m_mpeg2File, m_videoStream)) {
			break;
		}

		u_int8_t* yuvImage = (u_int8_t*)malloc(m_pConfig->m_yuvSize);
		
		if (yuvImage == NULL) {
			debug_message("Can't malloc YUV buffer!");
			break;
		}

		u_int8_t* yImage = yuvImage;
		u_int8_t* uImage = yuvImage + m_pConfig->m_ySize;
		u_int8_t* vImage = yuvImage + m_pConfig->m_ySize + m_pConfig->m_uvSize;

		// read mpeg2 frame
		rc = mpeg3_read_yuvframe(
			m_mpeg2File, 
			(char*)yImage, 
			(char*)uImage, 
			(char*)vImage, 
			0, 
			0,
			m_pConfig->m_videoWidth, 
			m_pConfig->m_videoHeight,
			m_videoStream);

		if (rc != 0) {
			debug_message("error reading mpeg2 video");
			break;
		}

		// encode video frame to MPEG-4
		if (m_pConfig->m_videoEncode) {

			// call encoder
			rc = m_videoEncoder->EncodeImage(
				yImage, 
				uImage, 
				vImage, 
				m_wantKeyFrame);

			if (!rc) {
				debug_message("Can't encode image!");
				break;
			}

			// clear this flag
			m_wantKeyFrame = false;

			u_int8_t* vopBuffer;
			u_int32_t vopBufferLength;

			m_videoEncoder->GetEncodedImage(&vopBuffer, &vopBufferLength);

			// forward encoded video to sinks
			CMediaFrame* pFrame = new CMediaFrame(
				CMediaFrame::Mpeg4VideoFrame, 
				vopBuffer, 
				vopBufferLength,
				m_videoFrameNumber * m_videoFrameDuration,
				m_videoFrameDuration);
			ForwardFrame(pFrame);
			delete pFrame;
		}

		// forward raw video to sinks
		if (m_pConfig->GetBoolValue(CONFIG_VIDEO_RAW_PREVIEW)
		  || m_pConfig->GetBoolValue(CONFIG_RECORD_RAW_VIDEO)) {

			CMediaFrame* pFrame =
				new CMediaFrame(CMediaFrame::RawYuvVideoFrame, 
					yuvImage, 
					m_pConfig->m_yuvSize,
					m_videoFrameNumber * m_videoFrameDuration,
					m_videoFrameDuration);
			ForwardFrame(pFrame);
			delete pFrame;
		}

		// forward reconstructed video to sinks
		if (m_pConfig->m_videoEncode
		  && m_pConfig->GetBoolValue(CONFIG_VIDEO_ENCODED_PREVIEW)) {

			u_int8_t* reconstructImage = 
				(u_int8_t*)malloc(m_pConfig->m_yuvSize);
			
			if (reconstructImage == NULL) {
				debug_message("Can't malloc YUV buffer!");
				break;
			}

			m_videoEncoder->GetReconstructedImage(
				reconstructImage,
				reconstructImage + m_pConfig->m_ySize,
				reconstructImage + m_pConfig->m_ySize 
					+ m_pConfig->m_uvSize);

			CMediaFrame* pFrame =
				new CMediaFrame(CMediaFrame::ReconstructYuvVideoFrame, 
					reconstructImage, 
					m_pConfig->m_yuvSize,
					m_videoFrameNumber * m_videoFrameDuration,
					m_videoFrameDuration);
			ForwardFrame(pFrame);
			delete pFrame;
		}

		m_videoFrameNumber++;
	}
}


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

#include "mp4live.h"
#include "transcoder.h"

#ifdef ADD_XVID_ENCODER
#include "video_xvid.h"
#endif


int CTranscoder::ThreadMain(void) 
{
	while (true) {
		int rc;

		if (m_transcode) {
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
					DoStopTranscode();	// ensure things get cleaned up
					delete pMsg;
					return 0;
				case MSG_START_TRANSCODE:
					DoStartTranscode();
					break;
				case MSG_STOP_TRANSCODE:
					DoStopTranscode();
					break;
				}

				delete pMsg;
			}
		}

		if (m_transcode) {
			DoTranscode();
		}
	}

	return -1;
}

void CTranscoder::DoStartTranscode()
{
	u_int32_t numVideoTracks;
	u_int32_t numAudioTracks;

	if (m_transcode) {
		return;
	}

	m_srcVideoSampleId = 1;
	m_srcAudioSampleId = 1;
	m_dstMp4File = NULL;

	char* srcMp4FileName =
		m_pConfig->GetStringValue(CONFIG_TRANSCODE_SRC_FILE_NAME);

	char* dstMp4FileName =
		m_pConfig->GetStringValue(CONFIG_TRANSCODE_DST_FILE_NAME);

	bool twoFiles = !(dstMp4FileName == NULL || dstMp4FileName[0] == '\0');

	if (twoFiles) {
		m_srcMp4File = MP4Read(srcMp4FileName);
	} else {
		m_srcMp4File = MP4Modify(srcMp4FileName);
	}
	if (m_srcMp4File == MP4_INVALID_FILE_HANDLE) {
		error_message("can't open %s", srcMp4FileName);
		return;
	}

	if (twoFiles) {
		m_dstMp4File = MP4Create(dstMp4FileName);

		if (m_dstMp4File == MP4_INVALID_FILE_HANDLE) {
			error_message("can't create %s", dstMp4FileName);
			goto start_failure;
		}
	} else {
		m_dstMp4File = m_srcMp4File;
	}

	// find raw video track
	numVideoTracks =
		MP4GetNumberOfTracks(m_srcMp4File, MP4_VIDEO_TRACK_TYPE);

	m_srcVideoTrackId = MP4_INVALID_TRACK_ID;

	u_int32_t i;
	for (i = 0; i < numVideoTracks; i++) {
		MP4TrackId videoTrackId =
			MP4FindTrackId(m_srcMp4File, i, MP4_VIDEO_TRACK_TYPE);

		u_int8_t videoType =
			MP4GetTrackVideoType(m_srcMp4File, videoTrackId);

		if (videoType == MP4_PRIVATE_VIDEO_TYPE) {
			m_srcVideoTrackId = videoTrackId;
			break;
		}
	}

	if (m_srcVideoTrackId == MP4_INVALID_TRACK_ID) {
		error_message("can't find raw video track");
		goto start_failure;
	}

	m_srcVideoNumSamples =
		MP4GetTrackNumberOfSamples(m_srcMp4File, m_srcVideoTrackId);

	m_srcVideoWidth =
		MP4GetTrackIntegerProperty(m_srcMp4File, m_srcVideoTrackId, 
			"mdia.minf.stbl.stsd.mp4v.width");
	m_srcVideoHeight =
		MP4GetTrackIntegerProperty(m_srcMp4File, m_srcVideoTrackId, 
			"mdia.minf.stbl.stsd.mp4v.height");

	// TBD preview sink needs to know the video size

	m_videoYSize = m_srcVideoWidth * m_srcVideoHeight;
	m_videoUVSize = m_videoYSize / 4;
	m_videoYUVSize = m_videoYSize + 2 * m_videoUVSize;

	m_dstVideoTrackId = MP4AddVideoTrack(m_dstMp4File, 
		MP4GetTrackTimeScale(m_srcMp4File, m_srcVideoTrackId),
		MP4GetTrackFixedSampleDuration(m_srcMp4File, m_srcVideoTrackId),
		m_srcVideoWidth,
		m_srcVideoHeight,
		MP4_MPEG4_VIDEO_TYPE);

	// TBD ES Configuration
	MP4SetTrackESConfiguration(m_dstMp4File, m_dstVideoTrackId,
		m_pConfig->m_videoMpeg4Config, 
		m_pConfig->m_videoMpeg4ConfigLength); 

	if (!InitVideoEncoder()) {
		goto start_failure;
	}

	// TBD if audio

	m_transcode = true;
	return;

start_failure:
	if (m_dstMp4File != m_srcMp4File) {
		MP4Close(m_dstMp4File);
		m_dstMp4File = NULL;
	}
	MP4Close(m_srcMp4File);
	m_srcMp4File = NULL;
	return;
}

void CTranscoder::DoTranscode()
{
	// do next N samples of video, audio
	u_int32_t numSamples = 30; // TBD base it on src frame rate

	if (m_srcVideoSampleId + numSamples > m_srcVideoNumSamples) {
		numSamples = m_srcVideoNumSamples - m_srcVideoSampleId + 1;
	}
 
	if (DoVideoTrack(m_srcVideoSampleId, numSamples) == false) {
		DoStopTranscode();
	}

	m_srcVideoSampleId += numSamples;

	if (m_srcVideoSampleId >= m_srcVideoNumSamples) {
		DoStopTranscode();
	}
}

void CTranscoder::DoStopTranscode()
{
	if (!m_transcode) {
		return;
	}

	if (m_dstMp4File != m_srcMp4File) {
		MP4Close(m_dstMp4File);
		m_dstMp4File = NULL;
	}
	MP4Close(m_srcMp4File);
	m_srcMp4File = NULL;

	// TBD ISMA compliance?

	if (m_videoEncoder) {
		m_videoEncoder->Stop();
	}

	m_transcode = false;
}

bool CTranscoder::InitVideoEncoder()
{
	char* encoderName = "xvid";	// TBD TEMP

	if (!strcasecmp(encoderName, VIDEO_ENCODER_XVID)) {
#ifdef ADD_XVID_ENCODER
		m_videoEncoder = new CXvidVideoEncoder();
#else
		error_message("xvid encoder not available in this build");
		return false;
#endif
	} else {
		error_message("unknown encoder specified");
		return false;
	}

	return m_videoEncoder->Init(m_pConfig);
}

bool CTranscoder::DoVideoTrack(
	MP4SampleId startSampleId, 
	u_int32_t numSamples)
{
	u_int8_t* pSample;
	u_int32_t sampleSize;
	MP4Duration sampleDuration;

	for (MP4SampleId sampleId = startSampleId; 
	  sampleId < startSampleId + numSamples; sampleId++) {
		bool rc;

		// signals to ReadSample() that it should malloc a buffer for us
		pSample = NULL;
		sampleSize = 0;

		rc = MP4ReadSample(
			m_srcMp4File, 
			m_srcVideoTrackId, 
			sampleId, 
			&pSample, 
			&sampleSize,
			NULL,
			&sampleDuration);

		if (rc == false) {
			error_message("failed to read sample %u\n", sampleId);
			return false;
		}

		// call encoder
		rc = m_videoEncoder->EncodeImage(
			pSample, 
			pSample + m_videoYSize,
			pSample + m_videoYSize + m_videoUVSize);

		if (rc == false) {
			debug_message("Can't encode image!");
			return false;
		}

		u_int8_t* vopBuf;
		u_int32_t vopBufLength;

		m_videoEncoder->GetEncodedFrame(&vopBuf, &vopBufLength);

		// TBD is IFrame
		bool isIFrame = ((vopBuf[4] >> 6) == 0);

		// write out encoded frame
		rc = MP4WriteSample(
			m_dstMp4File, 
			m_dstVideoTrackId,
			vopBuf,
			vopBufLength,
			sampleDuration,
			0,
			isIFrame);

		if (rc == false) {
			error_message("failed to write sample %u\n", sampleId);
			return false;
		}

		// forward for encoded preview
		if (m_pConfig->GetBoolValue(CONFIG_VIDEO_ENCODED_PREVIEW)) {
			u_int8_t* reconstructed = 
				(u_int8_t*)malloc(m_videoYUVSize);
			// TBD error

			m_videoEncoder->GetReconstructedImage(
				reconstructed,
				reconstructed + m_videoYSize,
				reconstructed + m_videoYSize + m_videoUVSize);

			CMediaFrame* pFrame =
				new CMediaFrame(CMediaFrame::ReconstructYuvVideoFrame, 
					reconstructed, 
					m_videoYUVSize,
					0, 
					sampleDuration);
			ForwardFrame(pFrame);
			delete pFrame;
		}

		// forward for raw preview
		if (m_pConfig->GetBoolValue(CONFIG_VIDEO_RAW_PREVIEW)) {
			CMediaFrame* pFrame =
				new CMediaFrame(CMediaFrame::RawYuvVideoFrame, 
					pSample,
					sampleSize,
					0, 
					sampleDuration);
			ForwardFrame(pFrame);
			delete pFrame;
		} else {
			free(pSample);
		}
	}

	return true;
}

bool CTranscoder::DoAudioTrack(
	MP4SampleId startSampleId, 
	u_int32_t numSamples)
{
	return true;
}

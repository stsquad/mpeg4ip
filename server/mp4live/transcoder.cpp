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

#ifdef ADD_FFMPEG_ENCODER
#include "video_ffmpeg.h"
#endif
#ifdef ADD_XVID_ENCODER
#include "video_xvid.h"
#endif
#ifdef ADD_H26L_ENCODER
#include "video_h26l.h"
#endif

#ifdef ADD_LAME_ENCODER
#include "audio_lame.h"
#endif
#ifdef ADD_FAAC_ENCODER
#include "audio_faac.h"
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
	if (m_transcode) {
		return;
	}

	m_srcVideoTrackId = MP4_INVALID_TRACK_ID;
	m_srcVideoSampleId = 1;

	m_srcAudioTrackId = MP4_INVALID_TRACK_ID;
	m_srcAudioSampleId = 1;

	m_dstMp4File = NULL;

	const char* srcMp4FileName =
		m_pConfig->GetStringValue(CONFIG_TRANSCODE_SRC_FILE_NAME);

	const char* dstMp4FileName =
		m_pConfig->GetStringValue(CONFIG_TRANSCODE_DST_FILE_NAME);

	bool twoFiles = !(dstMp4FileName == NULL || dstMp4FileName[0] == '\0');

	if (twoFiles) {
		m_srcMp4File = MP4Read(srcMp4FileName);
		m_srcMp4FileName = srcMp4FileName;
		m_dstMp4FileName = dstMp4FileName;
	} else {
		m_srcMp4File = MP4Modify(srcMp4FileName);
		m_srcMp4FileName = m_dstMp4FileName = srcMp4FileName;
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

	// check if user wants video transcoding
	if (strcasecmp(m_pConfig->GetStringValue(
	  CONFIG_TRANSCODE_DST_VIDEO_ENCODING), VIDEO_ENCODING_NONE)) {
		if (!InitVideoTranscode()) {
			goto start_failure;
		}
	}

	// check if user wants audio transcoding
	if (strcasecmp(m_pConfig->GetStringValue(
	  CONFIG_TRANSCODE_DST_AUDIO_ENCODING), AUDIO_ENCODING_NONE)) {
		if (!InitAudioTranscode()) {
			goto start_failure;
		}
	}

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

bool CTranscoder::InitVideoTranscode()
{
	// TBD CONFIG_TRANSCODE_SRC_VIDEO_ENCODING != VIDEO_ENCODING_YUV12
	// TEMP find raw video track
	m_srcVideoTrackId = MP4FindTrackId(
		m_srcMp4File, 0, MP4_VIDEO_TRACK_TYPE, MP4_YUV12_VIDEO_TYPE);

	// initialize decoder if needed
	if (strcasecmp(m_pConfig->GetStringValue(
	  CONFIG_TRANSCODE_SRC_VIDEO_ENCODING), VIDEO_ENCODING_YUV12)) {
		if (!InitVideoDecoder()) {
			return false;
		}
	}

	if (m_srcVideoTrackId == MP4_INVALID_TRACK_ID) {
		error_message("can't find raw video track");
		return false;
	}

	m_srcVideoNumSamples =
		MP4GetTrackNumberOfSamples(m_srcMp4File, m_srcVideoTrackId);

	m_srcVideoWidth =
		MP4GetTrackVideoWidth(m_srcMp4File, m_srcVideoTrackId);
	m_srcVideoHeight =
		MP4GetTrackVideoHeight(m_srcMp4File, m_srcVideoTrackId);

	m_videoYSize = m_srcVideoWidth * m_srcVideoHeight;
	m_videoUVSize = m_videoYSize / 4;
	m_videoYUVSize = m_videoYSize + 2 * m_videoUVSize;

	// TBD preview sink needs to be informed of the video size

	const char* encoding =
		m_pConfig->GetStringValue(CONFIG_TRANSCODE_DST_VIDEO_ENCODING);
		
	if (!strcasecmp(encoding, VIDEO_ENCODING_MPEG4)) {
		m_videoDstType = MP4_MPEG4_VIDEO_TYPE;
	} else if (!strcasecmp(encoding, VIDEO_ENCODING_H26L)) {
		m_videoDstType = MP4_H26L_VIDEO_TYPE;
	} else {
		error_message("invalid transcoder dest encoding %s", encoding);
		return false;
	}

	m_dstVideoTrackId = MP4AddVideoTrack(
		m_dstMp4File, 
		MP4GetTrackTimeScale(m_srcMp4File, m_srcVideoTrackId),
		MP4GetTrackFixedSampleDuration(m_srcMp4File, m_srcVideoTrackId),
		m_srcVideoWidth,
		m_srcVideoHeight,
		m_videoDstType);

	// TBD ES Configuration
	MP4SetTrackESConfiguration(
		m_dstMp4File, 
		m_dstVideoTrackId,
		m_pConfig->m_videoMpeg4Config, 
		m_pConfig->m_videoMpeg4ConfigLength); 

	if (!InitVideoEncoder()) {
		return false;
	}

	return true;
}

bool CTranscoder::InitVideoDecoder()
{
	// TBD
	return false;
}

bool CTranscoder::InitVideoEncoder()
{
	const char* encoding = 
		m_pConfig->GetStringValue(CONFIG_TRANSCODE_DST_VIDEO_ENCODING);
	const char* encoderName = 
		m_pConfig->GetStringValue(CONFIG_VIDEO_ENCODER);

	if (!strcasecmp(encoding, VIDEO_ENCODING_MPEG4)) {
		if (!strcasecmp(encoderName, VIDEO_ENCODER_FFMPEG)) {
#ifdef ADD_FFMPEG_ENCODER
			m_videoEncoder = new CFfmpegVideoEncoder();
#else
			error_message("ffmpeg encoder not available in this build");
			return false;
#endif
		} else if (!strcasecmp(encoderName, VIDEO_ENCODER_XVID)) {
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
	} else if (!strcasecmp(encoding, VIDEO_ENCODING_H26L)) {
#ifdef ADD_H26L_ENCODER
			m_videoEncoder = new CH26LVideoEncoder();
#else
			error_message("H.26L encoder not available in this build");
			return false;
#endif
	} else {
		error_message("unknown encoding specified");
		return false;
	}

	return m_videoEncoder->Init(m_pConfig);
}

bool CTranscoder::InitAudioTranscode()
{
	// TBD CONFIG_TRANSCODE_SRC_AUDIO_ENCODING != AUDIO_ENCODING_PCM16
	// TEMP find raw audio track
	m_srcAudioTrackId = MP4FindTrackId(
		m_srcMp4File, 0, MP4_AUDIO_TRACK_TYPE, MP4_PCM16_AUDIO_TYPE);

	if (m_srcAudioTrackId == MP4_INVALID_TRACK_ID) {
		error_message("can't find raw audio track");
		return false;
	}

	m_srcAudioNumSamples =
		MP4GetTrackNumberOfSamples(m_srcMp4File, m_srcAudioTrackId);

#ifdef TBD
	m_dstAudioTrackId = MP4AddAudioTrack(
		m_dstMp4File, 

	MP4SetTrackESConfiguration(
		m_dstMp4File, 
		m_dstAudioTrackId,
#endif

	if (!InitAudioEncoder()) {
		return false;
	}

	return true;
}

bool CTranscoder::InitAudioEncoder()
{
	const char* encoding = 
		m_pConfig->GetStringValue(CONFIG_TRANSCODE_DST_AUDIO_ENCODING);

	if (!strcasecmp(encoding, AUDIO_ENCODING_AAC)) {
#ifdef ADD_FAAC_ENCODER
		m_audioEncoder = new CFaacAudioEncoder();
#else
		error_message("faac encoder not available in this build");
		return false;
#endif
	} else if (!strcasecmp(encoding, AUDIO_ENCODING_MP3)) {
#ifdef ADD_LAME_ENCODER
		m_audioEncoder = new CLameAudioEncoder();
#else
		error_message("lame encoder not available in this build");
		return false;
#endif
	} else {
		error_message("unknown encoder specified");
		return false;
	}

	return m_audioEncoder->Init(m_pConfig);
}

void CTranscoder::DoTranscode()
{
	if (m_videoEncoder) {
		u_int32_t numSamples = 30; // TBD base it on src frame rate

		if (m_videoDstType == MP4_H26L_VIDEO_TYPE) {
			numSamples = 1;
		}

		if (m_srcVideoSampleId + numSamples > m_srcVideoNumSamples) {
			numSamples = m_srcVideoNumSamples - m_srcVideoSampleId + 1;
		}
	 
		if (DoVideoTrack(m_srcVideoSampleId, numSamples) == false) {
			DoStopTranscode();
		}

		m_srcVideoSampleId += numSamples;

		// all done
		if (m_srcVideoSampleId >= m_srcVideoNumSamples) {
			DoStopTranscode();

			// add hint track
			if (m_videoDstType == MP4_MPEG4_VIDEO_TYPE) {
				MP4AV_Rfc3016Hinter(
					m_dstMp4File, 
					m_dstVideoTrackId,
					m_pConfig->GetIntegerValue(CONFIG_RTP_PAYLOAD_SIZE));
			}
		}
	}

	if (m_audioEncoder) {
		u_int32_t numSamples = 30; // TBD base it on src sampling rate

		if (m_srcAudioSampleId + numSamples > m_srcAudioNumSamples) {
			numSamples = m_srcAudioNumSamples - m_srcAudioSampleId + 1;
		}
	 
		if (DoAudioTrack(m_srcAudioSampleId, numSamples) == false) {
			DoStopTranscode();
		}

		m_srcAudioSampleId += numSamples;

		// all done
		if (m_srcAudioSampleId >= m_srcAudioNumSamples) {
			DoStopTranscode();

			if (m_audioDstType == MP4_MP3_AUDIO_TYPE) {
				MP4AV_Rfc2250Hinter(
					m_dstMp4File, 
					m_dstAudioTrackId, 
					false, 
					m_pConfig->GetIntegerValue(CONFIG_RTP_PAYLOAD_SIZE));

			} else if (m_audioDstType == MP4_MPEG4_AUDIO_TYPE) {
				MP4AV_RfcIsmaHinter(
					m_dstMp4File, 
					m_dstAudioTrackId, 
					false, 
					m_pConfig->GetIntegerValue(CONFIG_RTP_PAYLOAD_SIZE));
			}
		}
	}
}

bool CTranscoder::DoVideoTrack(
	MP4SampleId startSampleId, 
	u_int32_t numSamples)
{
	u_int8_t* pSample;
	u_int32_t sampleSize;
	MP4Duration sampleDuration;
	CMediaFrame* pRawMediaFrame = NULL;

	for (MP4SampleId sampleId = startSampleId; 
	  sampleId < startSampleId + numSamples; sampleId++) {
		bool rc;

		// signals to ReadSample() 
		// that it should malloc a buffer for us
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
			error_message("failed to read video sample %u\n", sampleId);
			return false;
		}

		if (m_videoDecoder) {
			// call decoder
			if (!m_videoDecoder->DecodeImage(pSample, sampleSize)) {
				error_message("failed to decode video sample %u\n", sampleId);
				return false;
			}

			u_int8_t* pY;
			u_int8_t* pU;
			u_int8_t* pV;
		
			m_videoDecoder->GetDecodedImage(&pY, &pU, &pV);

			// cleanup pSample, no further use for it
			free(pSample);
			pSample = NULL;

			// call encoder
			rc = m_videoEncoder->EncodeImage(pY, pU, pV);

			// TBD handle case where YUV planes are not sequential
			pRawMediaFrame = new CMediaFrame(
					CMediaFrame::RawYuvVideoFrame, 
					pY,
					m_videoYUVSize,
					0, 
					sampleDuration);
		} else {
			// call encoder
			rc = m_videoEncoder->EncodeImage(
				pSample, 
				pSample + m_videoYSize,
				pSample + m_videoYSize + m_videoUVSize);

			pRawMediaFrame = new CMediaFrame(
					CMediaFrame::RawYuvVideoFrame, 
					pSample,
					sampleSize,
					0, 
					sampleDuration);

			// don't free pSample, delete pRawMediaFrame will now do that
			pSample = NULL;
		}

		if (rc == false) {
			error_message("failed to encode video sample %u\n", sampleId);
			return false;
		}

		// get the resulting encoded image
		u_int8_t* vopBuf;
		u_int32_t vopBufLength;

		m_videoEncoder->GetEncodedImage(&vopBuf, &vopBufLength);

		if (vopBufLength != 0) {
			// determine if image was encoded as an I(ntra) frame
			bool isIFrame = false;

			if (m_videoDstType == MP4_MPEG4_VIDEO_TYPE) {
				isIFrame = 
					(MP4AV_Mpeg4GetVopType(vopBuf, vopBufLength) == 'I');
			} else {
				// TBD H.26L I frame
			}

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
				error_message("failed to write video sample %u\n", sampleId);
				return false;
			}
		}

		// forward for encoded preview
		if (m_pConfig->GetBoolValue(CONFIG_VIDEO_ENCODED_PREVIEW)) {
			u_int8_t* reconstructed = 
				(u_int8_t*)malloc(m_videoYUVSize);
			// TBD malloc error

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
			ForwardFrame(pRawMediaFrame);
		}
		delete pRawMediaFrame;
		pRawMediaFrame = NULL;
	}

	return true;
}

bool CTranscoder::DoAudioTrack(
	MP4SampleId startSampleId, 
	u_int32_t numSamples)
{
	u_int8_t* pSample;
	u_int32_t sampleSize;
	MP4Duration sampleDuration;

	for (MP4SampleId sampleId = startSampleId; 
	  sampleId < startSampleId + numSamples; sampleId++) {
		bool rc;

		// signals to ReadSample() 
		// that it should malloc a buffer for us
		pSample = NULL;
		sampleSize = 0;

		rc = MP4ReadSample(
			m_srcMp4File, 
			m_srcAudioTrackId, 
			sampleId, 
			&pSample, 
			&sampleSize,
			NULL,
			&sampleDuration);

		if (rc == false) {
			error_message("failed to read audio sample %u\n", sampleId);
			return false;
		}

#ifdef TBD
		// need to deal with mismatch between raw samples 
		// and number needed by audio encoder

		// call encoder
		rc = m_audioEncoder->EncodeSamples(
			pSample, xxx);

		if (rc == false) {
			debug_message("Can't encode audio samples!");
			return false;
		}

		u_int8_t* frameBuf;
		u_int32_t frameBufLength;

		m_audioEncoder->GetEncodedSamples(&frameBuf, &frameBufLength);

		// write out encoded frame
		rc = MP4WriteSample(
			m_dstMp4File, 
			m_dstAudioTrackId,
			frameBuf,
			frameBufLength,
			xxxDuration);
#endif /* TBD */

		if (rc == false) {
			error_message("failed to write audio sample %u\n", sampleId);
			return false;
		}
	}

	return true;
}

float CTranscoder::GetProgress()
{
	if (!m_transcode) {
		return 0.0;
	}

	if (m_srcVideoTrackId != MP4_INVALID_TRACK_ID) {
		return (float)(m_srcVideoSampleId - 1) / (float)m_srcVideoNumSamples;
	} else { // m_srcAudioTrackId != MP4_INVALID_TRACK_ID
		return (float)(m_srcAudioSampleId - 1) / (float)m_srcAudioNumSamples;
	}
}

u_int64_t CTranscoder::GetEstSize()
{
	if (!m_transcode) {
		return 0;
	}

	MP4Duration duration = MP4GetDuration(m_srcMp4File);
	u_int64_t seconds = MP4ConvertFromMovieDuration(m_srcMp4File, 
		duration, MP4_SECONDS_TIME_SCALE);

	u_int32_t videoBytesPerSec = 0;
	if (m_srcVideoTrackId != MP4_INVALID_TRACK_ID) {
		videoBytesPerSec +=
			(m_pConfig->GetIntegerValue(CONFIG_VIDEO_BIT_RATE) * 1000) / 8;
	}

	u_int32_t audioBytesPerSec = 0;
	if (m_srcAudioTrackId != MP4_INVALID_TRACK_ID) {
		audioBytesPerSec +=
			(m_pConfig->GetIntegerValue(CONFIG_AUDIO_BIT_RATE) * 1000) / 8;
	}

	return (u_int64_t)
		((double)((videoBytesPerSec + audioBytesPerSec) * seconds) * 1.025);
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
	if (m_audioEncoder) {
		m_audioEncoder->Stop();
	}

	m_transcode = false;
}


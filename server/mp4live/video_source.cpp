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
 *		Bill May 		wmay@cisco.com
 */

#include "mp4live.h"

#include <sys/mman.h>

#include "video_source.h"
#include "rgb2yuv.h"

#ifdef ADD_FFMPEG_ENCODER
#include "video_ffmpeg.h"
#endif

#ifdef ADD_XVID_ENCODER
#include "video_xvid.h"
#endif


int CVideoSource::ThreadMain(void) 
{
	while (true) {
		int rc;

		if (m_capture) {
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
					DoStopCapture();	// ensure things get cleaned up
					delete pMsg;
					return 0;
				case MSG_START_CAPTURE:
					DoStartCapture();
					break;
				case MSG_STOP_CAPTURE:
					DoStopCapture();
					break;
				case MSG_GENERATE_KEY_FRAME:
					DoGenerateKeyFrame();
					break;
				}

				delete pMsg;
			}
		}

		if (m_capture) {
			ProcessVideo();
		}
	}

	return -1;
}

void CVideoSource::DoStartCapture(void)
{
	if (m_capture) {
		return;
	}
	if (!Init()) {
		return;
	}

	m_capture = true;
}

void CVideoSource::DoStopCapture(void)
{
	if (!m_capture) {
		return;
	}

	m_encoder->Stop();

	// release device resources
	munmap(m_videoMap, m_videoMbuf.size);
	m_videoMap = NULL;

	close(m_videoDevice);
	m_videoDevice = -1;

	m_capture = false;
}

void CVideoSource::DoGenerateKeyFrame(void)
{
	m_wantKeyFrame = true;
}

bool CVideoSource::Init(void)
{
	if (!InitDevice()) {
		return false;
	}

	if (m_pConfig->GetIntegerValue(CONFIG_VIDEO_SIGNAL) == VIDEO_MODE_NTSC) {
		m_rawFrameRate = NTSC_INT_FPS;
	} else {
		m_rawFrameRate = PAL_INT_FPS;
	}
	InitSampleFrames(
		m_pConfig->GetIntegerValue(CONFIG_VIDEO_FRAME_RATE), 
		m_rawFrameRate);

	InitSizes();

	if (!InitEncoder()) {
		close(m_videoDevice);
		m_videoDevice = -1;
		return false;
	}

	m_rawFrameNumber = 0xFFFFFFFF;
	m_rawFrameDuration = TimestampTicks / m_rawFrameRate;
	m_encodedFrameNumber = 0;
	m_targetFrameDuration = TimestampTicks 
		/ m_pConfig->GetIntegerValue(CONFIG_VIDEO_FRAME_RATE);

	m_prevYuvImage = NULL;
	m_prevReconstructImage = NULL;
	m_prevVopBuf = NULL;
	m_prevVopBufLength = 0;

	m_skippedFrames = 0;
	m_accumDrift = 0;
	m_maxDrift = m_targetFrameDuration;

	return true;
}

bool CVideoSource::InitDevice(void)
{
	int rc;
	char* deviceName = m_pConfig->GetStringValue(CONFIG_VIDEO_DEVICE_NAME);
	u_int16_t format;

	// open the video device
	m_videoDevice = open(deviceName, O_RDWR);
	if (m_videoDevice < 0) {
		error_message("Failed to open %s: %s", 
			deviceName, strerror(errno));
		return false;
	}

	// get device capabilities
	struct video_capability videoCapability;
	rc = ioctl(m_videoDevice, VIDIOCGCAP, &videoCapability);
	if (rc < 0) {
		error_message("Failed to get video capabilities for %s",
			deviceName);
		goto failure;
	}

	if (!(videoCapability.type & VID_TYPE_CAPTURE)) {
		error_message("Device %s is not capable of video capture!",
			deviceName);
		goto failure;
	}

	// N.B. "channel" here is really an input source
	struct video_channel videoChannel;
	videoChannel.channel = m_pConfig->GetIntegerValue(CONFIG_VIDEO_INPUT);
	rc = ioctl(m_videoDevice, VIDIOCGCHAN, &videoChannel);
	if (rc < 0) {
		error_message("Failed to get video channel info for %s",
			deviceName);
		goto failure;
	}

	// select video input and signal type
	videoChannel.norm = m_pConfig->GetIntegerValue(CONFIG_VIDEO_SIGNAL);
	rc = ioctl(m_videoDevice, VIDIOCSCHAN, &videoChannel);
	if (rc < 0) {
		error_message("Failed to set video channel info for %s",
			deviceName);
		goto failure;
	}

	// input source has a TV tuner
	if (videoChannel.flags & VIDEO_VC_TUNER) {
		struct video_tuner videoTuner;

		// get tuner info
		if ((int32_t)m_pConfig->GetIntegerValue(CONFIG_VIDEO_TUNER) == -1) {
			m_pConfig->SetIntegerValue(CONFIG_VIDEO_TUNER, 0);
		}
		videoTuner.tuner = m_pConfig->GetIntegerValue(CONFIG_VIDEO_TUNER);
		rc = ioctl(m_videoDevice, VIDIOCGTUNER, &videoTuner);
		if (rc < 0) {
			error_message("Failed to get video tuner info for %s",
				deviceName);
		}
		
		// set tuner and signal type
		videoTuner.mode = m_pConfig->GetIntegerValue(CONFIG_VIDEO_SIGNAL);
		rc = ioctl(m_videoDevice, VIDIOCSTUNER, &videoTuner);
		if (rc < 0) {
			error_message("Failed to set video tuner info for %s",
				deviceName);
		}

		// tune in the desired frequency (channel)
		struct CHANNEL_LIST* pChannelList = ListOfChannelLists[
			m_pConfig->GetIntegerValue(CONFIG_VIDEO_SIGNAL)];
		struct CHANNEL* pChannel = pChannelList[
			m_pConfig->GetIntegerValue(CONFIG_VIDEO_CHANNEL_LIST_INDEX)].list;
		unsigned long videoFrequencyKHz = pChannel[
			m_pConfig->GetIntegerValue(CONFIG_VIDEO_CHANNEL_INDEX)].freq;
		unsigned long videoFrequencyTuner =
			((videoFrequencyKHz / 1000) << 4) 
			| ((videoFrequencyKHz % 1000) >> 4);

		rc = ioctl(m_videoDevice, VIDIOCSFREQ, &videoFrequencyTuner);
		if (rc < 0) {
			error_message("Failed to set video tuner frequency for %s",
				deviceName);
		}
	}

	// get info on video capture buffers 
	rc = ioctl(m_videoDevice, VIDIOCGMBUF, &m_videoMbuf);
	if (rc < 0) {
		error_message("Failed to get video capture info for %s", 
			deviceName);
		goto failure;
	}

	// map the video capture buffers
	m_videoMap = mmap(0, m_videoMbuf.size, 
		PROT_READ, MAP_SHARED, m_videoDevice, 0);
	if (m_videoMap == MAP_FAILED) {
		error_message("Failed to map video capture memory for %s", 
			deviceName);
		goto failure;
	}

	// allocate enough frame maps
	m_videoFrameMap = (struct video_mmap*)
		malloc(m_videoMbuf.frames * sizeof(struct video_mmap));
	if (m_videoFrameMap == NULL) {
		error_message("Failed to allocate enough memory"); 
		goto failure;
	}

	m_captureHead = 0;
	m_encodeHead = -1;
	format = VIDEO_PALETTE_YUV420P;

	for (int i = 0; i < m_videoMbuf.frames; i++) {
		// initialize frame map
		m_videoFrameMap[i].frame = i;
		m_videoFrameMap[i].width = 
			m_pConfig->GetIntegerValue(CONFIG_VIDEO_RAW_WIDTH);
		m_videoFrameMap[i].height = 
			m_pConfig->GetIntegerValue(CONFIG_VIDEO_RAW_HEIGHT);
		m_videoFrameMap[i].format = format;

		// give frame to the video capture device
		rc = ioctl(m_videoDevice, VIDIOCMCAPTURE, &m_videoFrameMap[i]);
		if (rc < 0) {
			// try RGB24 palette instead
			if (i == 0 && format == VIDEO_PALETTE_YUV420P) {
				format = VIDEO_PALETTE_RGB24;
				i--;
				continue;
			} 

			error_message("Failed to allocate video capture buffer for %s", 
				deviceName);
			goto failure;
		}
	}

	if (format == VIDEO_PALETTE_RGB24) {
		m_pConfig->m_videoNeedRgbToYuv = true;
	}

	return true;

failure:
	free(m_videoFrameMap);
	if (m_videoMap) {
		munmap(m_videoMap, m_videoMbuf.size);
		m_videoMap = NULL;
	}
	close(m_videoDevice);
	m_videoDevice = -1;
	return false;
}

void CVideoSource::InitSampleFrames(u_int16_t targetFps, u_int16_t rawFps)
{
	float faccum = 0.0;
	float fout = 1.0;
	float epsilon = 0.01;
	float ratio = (float)targetFps / (float)rawFps;
	u_int16_t f;

	for (f = 0; f < rawFps; f++) {
		faccum += ratio;
		if (faccum + epsilon >= fout && fout <= (float)targetFps) {
			fout += 1.0;
			m_sampleFrames[f] = true;
		} else {
			m_sampleFrames[f] = false;
		}
	}

	// use to get an estimate 
	// of how many raw frames until
	// the next desired frame
	m_frameRateRatio = rawFps / targetFps;
}

void CalculateVideoFrameSize(CLiveConfig* pConfig)
{
	u_int16_t frameHeight;
	float aspectRatio = pConfig->GetFloatValue(CONFIG_VIDEO_ASPECT_RATIO);

	// crop video to appropriate aspect ratio modulo 16 pixels
	if ((aspectRatio - VIDEO_STD_ASPECT_RATIO) < 0.1) {
		frameHeight = pConfig->GetIntegerValue(CONFIG_VIDEO_RAW_HEIGHT);
	} else {
		frameHeight = (u_int16_t)(
			(float)pConfig->GetIntegerValue(CONFIG_VIDEO_RAW_WIDTH) 
			/ aspectRatio);

		if ((frameHeight % 16) != 0) {
			frameHeight += 16 - (frameHeight % 16);
		}

		if (frameHeight > pConfig->GetIntegerValue(CONFIG_VIDEO_RAW_HEIGHT)) {
			// OPTION might be better to insert black lines 
			// to pad image but for now we crop down
			frameHeight = pConfig->GetIntegerValue(CONFIG_VIDEO_RAW_HEIGHT);
			if ((frameHeight % 16) != 0) {
				frameHeight -= (frameHeight % 16);
			}
		}
	}

	pConfig->m_videoWidth = pConfig->GetIntegerValue(CONFIG_VIDEO_RAW_WIDTH);
	pConfig->m_videoHeight = frameHeight;

	pConfig->m_ySize = pConfig->m_videoWidth * pConfig->m_videoHeight;
	pConfig->m_uvSize = pConfig->m_ySize / 4;
}

void CVideoSource::InitSizes()
{
	CalculateVideoFrameSize(m_pConfig);

	m_yRawSize = 
		m_pConfig->GetIntegerValue(CONFIG_VIDEO_RAW_WIDTH)
		* m_pConfig->GetIntegerValue(CONFIG_VIDEO_RAW_HEIGHT);
	m_uvRawSize = m_yRawSize / 4;
	m_yuvRawSize = m_yRawSize + (2 * m_uvRawSize);

	m_yOffset = m_pConfig->GetIntegerValue(CONFIG_VIDEO_RAW_WIDTH)
		* ((m_pConfig->GetIntegerValue(CONFIG_VIDEO_RAW_HEIGHT)
			- m_pConfig->m_videoHeight) / 2);
	m_uvOffset = m_yOffset / 4;
		
	m_yuvSize = m_pConfig->m_ySize + 2 * m_pConfig->m_uvSize;
}

bool CVideoSource::InitEncoder()
{
	char* encoderName = 
		m_pConfig->GetStringValue(CONFIG_VIDEO_ENCODER);

	if (!strcasecmp(encoderName, VIDEO_ENCODER_FFMPEG)) {
#ifdef ADD_FFMPEG_ENCODER
		m_encoder = new CFfmpegVideoEncoder();
#else
		error_message("ffmpeg encoder not available in this build");
		return false;
#endif
	} else if (!strcasecmp(encoderName, VIDEO_ENCODER_XVID)) {
#ifdef ADD_XVID_ENCODER
		m_encoder = new CXvidVideoEncoder();
#else
		error_message("xvid encoder not available in this build");
		return false;
#endif
	} else {
		error_message("unknown encoder specified");
		return false;
	}

	return m_encoder->Init(m_pConfig);
}

int8_t CVideoSource::AcquireFrame(void)
{
	int rc;

	rc = ioctl(m_videoDevice, VIDIOCSYNC, &m_videoFrameMap[m_captureHead]);
	if (rc != 0) {
		return -1;
	}

	int8_t capturedFrame = m_captureHead;
	m_captureHead = (m_captureHead + 1) % m_videoMbuf.frames;

	if (m_captureHead == m_encodeHead) {
		debug_message("Video capture buffer overflow");
		return -1;
	}
	return capturedFrame;
}

void CVideoSource::ProcessVideo(void)
{
	u_int8_t* yuvImage = NULL;
	u_int8_t* yImage;
	u_int8_t* uImage;
	u_int8_t* vImage;
	Duration prevFrameDuration = 0;

	// for efficiency, process 1 second before returning to check for commands
	for (int pass = 0; pass < m_rawFrameRate; pass++) {

		// get next frame from video capture device
		m_encodeHead = AcquireFrame();
		if (m_encodeHead == -1) {
			continue;
		}
		Timestamp frameTimestamp = GetTimestamp();
		m_rawFrameNumber++;

		if (m_rawFrameNumber == 0) {
			m_startTimestamp = frameTimestamp;
			m_elapsedDuration = 0;
		}

		// check if we want this frame (to match target fps)
		if (!m_sampleFrames[m_rawFrameNumber % m_rawFrameRate]) {
			goto release;
		}

		// check if we are falling behind
		if (m_accumDrift >= m_maxDrift) {
			if (m_accumDrift <= m_targetFrameDuration) {
				m_accumDrift = 0;
			} else {
				m_accumDrift -= m_targetFrameDuration;
			}

			// skip this frame			
			m_skippedFrames++;
			goto release;
		}

		// perform colorspace conversion if necessary
		if (m_pConfig->m_videoNeedRgbToYuv) {
			yuvImage = (u_int8_t*)malloc(m_yuvRawSize);
			if (yuvImage == NULL) {
				debug_message("Can't malloc YUV buffer!");
				goto release;
			}
			RGB2YUV(
				m_pConfig->GetIntegerValue(CONFIG_VIDEO_RAW_WIDTH), 
				m_pConfig->GetIntegerValue(CONFIG_VIDEO_RAW_HEIGHT), 
				(u_int8_t*)m_videoMap + m_videoMbuf.offsets[m_encodeHead],
				yuvImage,
				yuvImage + m_yRawSize,
				yuvImage + m_yRawSize + m_uvRawSize,
				1);
		} else {
			yuvImage = (u_int8_t*)m_videoMap 
				+ m_videoMbuf.offsets[m_encodeHead];
		}

		yImage = yuvImage + m_yOffset;
		uImage = yuvImage + m_yRawSize + m_uvOffset;
		vImage = uImage + m_uvRawSize;

		// encode video frame to MPEG-4
		if (m_pConfig->m_videoEncode) {

			// call encoder
			if (!m_encoder->EncodeImage(
			  yImage, uImage, vImage, m_wantKeyFrame)) {
				debug_message("Can't encode image!");
				goto release;
			}

			// clear this flag
			m_wantKeyFrame = false;
		}

		// calculate frame duration
		// making an adjustment to account 
		// for any raw frames dropped by the driver
		if (m_rawFrameNumber > 0) {
			Duration elapsedTime = 
				frameTimestamp - m_startTimestamp;

			prevFrameDuration = 
				(m_skippedFrames + 1) * m_targetFrameDuration;

			Duration elapsedDuration = 
				m_elapsedDuration + prevFrameDuration;

			Duration skew = elapsedTime - elapsedDuration;

			if (skew > 0) {
				prevFrameDuration += 
					(skew / m_targetFrameDuration) * m_targetFrameDuration;
			}

			m_elapsedDuration += prevFrameDuration;
		}

		// forward encoded video to sinks
		if (m_pConfig->m_videoEncode) {
			if (m_prevVopBuf) {
				CMediaFrame* pFrame = new CMediaFrame(
					CMediaFrame::Mpeg4VideoFrame, 
					m_prevVopBuf, 
					m_prevVopBufLength,
					m_prevVopTimestamp, 
					prevFrameDuration);
				ForwardFrame(pFrame);
				delete pFrame;
			}

			// hold onto this encoded vop until next one is ready
			m_encoder->GetEncodedFrame(&m_prevVopBuf, &m_prevVopBufLength);
			m_prevVopTimestamp = frameTimestamp;

			m_encodedFrameNumber++;
		}

		// forward raw video to sinks
		if (m_pConfig->GetBoolValue(CONFIG_VIDEO_RAW_PREVIEW)
		  || m_pConfig->GetBoolValue(CONFIG_RECORD_RAW_VIDEO)) {

			if (m_prevYuvImage) {
				CMediaFrame* pFrame =
					new CMediaFrame(CMediaFrame::RawYuvVideoFrame, 
						m_prevYuvImage, 
						m_yuvSize,
						m_prevYuvImageTimestamp, 
						prevFrameDuration);
				ForwardFrame(pFrame);
				delete pFrame;
			}

			m_prevYuvImage = (u_int8_t*)malloc(m_yuvSize);
			if (m_prevYuvImage == NULL) {
				debug_message("Can't malloc YUV buffer!");
			} else {
				memcpy(m_prevYuvImage, 
					yImage, 
					m_pConfig->m_ySize);
				memcpy(m_prevYuvImage + m_pConfig->m_ySize, 
					uImage, 
					m_pConfig->m_uvSize);
				memcpy(m_prevYuvImage + m_pConfig->m_ySize 
						+ m_pConfig->m_uvSize, 
					vImage, 
					m_pConfig->m_uvSize);

				m_prevYuvImageTimestamp = frameTimestamp;
			}
		}

		// forward reconstructed video to sinks
		if (m_pConfig->m_videoEncode
		  && m_pConfig->GetBoolValue(CONFIG_VIDEO_ENCODED_PREVIEW)) {

			if (m_prevReconstructImage) {
				CMediaFrame* pFrame =
					new CMediaFrame(CMediaFrame::ReconstructYuvVideoFrame, 
						m_prevReconstructImage, 
						m_yuvSize,
						m_prevVopTimestamp, 
						prevFrameDuration);
				ForwardFrame(pFrame);
				delete pFrame;
			}

			m_prevReconstructImage = (u_int8_t*)malloc(m_yuvSize);
			if (m_prevReconstructImage == NULL) {
				debug_message("Can't malloc YUV buffer!");
			} else {
				m_encoder->GetReconstructedImage(
					m_prevReconstructImage,
					m_prevReconstructImage + m_pConfig->m_ySize,
					m_prevReconstructImage + m_pConfig->m_ySize 
						+ m_pConfig->m_uvSize);
			}
		}

		// reset skipped frames
		m_skippedFrames = 0;

		// calculate how we're doing versus target frame rate
		// this is used to decide if we need to drop frames
		Duration encodingTime;
		encodingTime = GetTimestamp() - frameTimestamp;
		if (encodingTime >= m_targetFrameDuration) {
			m_accumDrift += encodingTime - m_targetFrameDuration;
		} else {
			m_accumDrift -= m_targetFrameDuration - encodingTime;
			if (m_accumDrift < 0) {
				m_accumDrift = 0;
			}
		}

release:
		// release video frame buffer back to video capture device
		if (ReleaseFrame(m_encodeHead)) {
			m_encodeHead = (m_encodeHead + 1) % m_videoMbuf.frames;
		} else {
			debug_message("Couldn't release capture buffer!");
		}
		if (m_pConfig->m_videoNeedRgbToYuv) {
			free(yuvImage);
			yuvImage = NULL;
		}
	}
}

char CVideoSource::GetMpeg4VideoFrameType(CMediaFrame* pFrame)
{
	if (pFrame == NULL) {
		return 0;
	}
	if (pFrame->GetDataLength() < 5) {
		return 0;
	}

	u_int8_t* pData = (u_int8_t*)pFrame->GetData();
	if (pData[0] != 0x00 || pData[1] != 0x00 
	  || pData[2] != 0x01 || pData[3] != 0xB6) {
		return 0;
	}

	switch (pData[4] >> 6) {
	case 0:
		return 'I';
	case 1:
		return 'P';
	case 2:
		return 'B';
	case 3:
		return 'S';
	}

	return 0;
}

bool CVideoSource::InitialVideoProbe(CLiveConfig* pConfig)
{
	static char* devices[] = {
		"/dev/video", 
		"/dev/video0", 
		"/dev/video1", 
		"/dev/video2", 
		"/dev/video3"
	};
	char* deviceName = pConfig->GetStringValue(CONFIG_VIDEO_DEVICE_NAME);
	CVideoCapabilities* pVideoCaps;

	// first try the device we're configured with
	pVideoCaps = new CVideoCapabilities(deviceName);

	if (pVideoCaps->IsValid()) {
		pConfig->m_videoCapabilities = pVideoCaps;
		return true;
	}

	delete pVideoCaps;

	// no luck, go searching
	for (u_int32_t i = 0; i < sizeof(devices) / sizeof(char*); i++) {

		// don't waste time trying something that's already failed
		if (!strcmp(devices[i], deviceName)) {
			continue;
		} 

		pVideoCaps = new CVideoCapabilities(devices[i]);

		if (pVideoCaps->IsValid()) {
			pConfig->SetStringValue(CONFIG_VIDEO_DEVICE_NAME, devices[i]);
			pConfig->m_videoCapabilities = pVideoCaps;
			return true;
		}
		
		delete pVideoCaps;
	}

	return false;
}

bool CVideoCapabilities::ProbeDevice()
{
	int rc;

	int videoDevice = open(m_deviceName, O_RDWR);
	if (videoDevice < 0) {
		m_canOpen = false;
		return false;
	}
	m_canOpen = true;

	// get device capabilities
	struct video_capability videoCapability;
	rc = ioctl(videoDevice, VIDIOCGCAP, &videoCapability);
	if (rc < 0) {
		debug_message("Failed to get video capabilities for %s", m_deviceName);
		m_canCapture = false;
		close(videoDevice);
		return false;
	}

	if (!(videoCapability.type & VID_TYPE_CAPTURE)) {
		debug_message("Device %s is not capable of video capture!", 
			m_deviceName);
		m_canCapture = false;
		close(videoDevice);
		return false;
	}
	m_canCapture = true;

	m_driverName = stralloc(videoCapability.name);
	m_numInputs = videoCapability.channels;

	m_minWidth = videoCapability.minwidth;
	m_minHeight = videoCapability.minheight;
	m_maxWidth = videoCapability.maxwidth;
	m_maxHeight = videoCapability.maxheight;

	m_inputNames = (char**)malloc(m_numInputs * sizeof(char*));
	memset(m_inputNames, 0, m_numInputs * sizeof(char*));

	m_inputSignalTypes = (u_int8_t*)malloc(m_numInputs * sizeof(u_int8_t));
	memset(m_inputSignalTypes, 0, m_numInputs * sizeof(u_int8_t));

	m_inputHasTuners = (bool*)malloc(m_numInputs * sizeof(bool));
	memset(m_inputHasTuners, 0, m_numInputs * sizeof(bool));

	m_inputTunerSignalTypes = (u_int8_t*)malloc(m_numInputs * sizeof(u_int8_t));
	memset(m_inputTunerSignalTypes, 0, m_numInputs * sizeof(u_int8_t));


	for (int i = 0; i < m_numInputs; i++) {
		// N.B. "channel" here is really an input source
		struct video_channel videoChannel;
		videoChannel.channel = i;
		rc = ioctl(videoDevice, VIDIOCGCHAN, &videoChannel);
		if (rc < 0) {
			debug_message("Failed to get video channel info for %s:%u",
				m_deviceName, i);
			continue;
		}
		m_inputNames[i] = stralloc(videoChannel.name);
		m_inputSignalTypes[i] = videoChannel.norm;

		if (videoChannel.flags & VIDEO_VC_TUNER) {
			// ignore videoChannel.tuners for now
			// current bt drivers only support 1 tuner per input port

			struct video_tuner videoTuner;
			videoTuner.tuner = 0;
			rc = ioctl(videoDevice, VIDIOCGTUNER, &videoTuner);
			if (rc < 0) {
				debug_message("Failed to get video tuner info for %s:%u",
					m_deviceName, i);
				continue;
			}
				
			m_inputHasTuners[i] = true;
			m_inputTunerSignalTypes[i] = videoTuner.flags & 0x7;
		}
	}

	close(videoDevice);
	return true;
}


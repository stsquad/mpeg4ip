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

#include "video_v4l_source.h"
#include "video_util_rgb.h"


int CV4LVideoSource::ThreadMain(void) 
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
					DoStopCapture();	// ensure things get cleaned up
					delete pMsg;
					return 0;
				case MSG_NODE_START:
				case MSG_SOURCE_START_VIDEO:
					DoStartCapture();
					break;
				case MSG_NODE_STOP:
					DoStopCapture();
					break;
				case MSG_SOURCE_KEY_FRAME:
					DoGenerateKeyFrame();
					break;
				}

				delete pMsg;
			}
		}

		if (m_source) {
			try {
				ProcessVideo();
			}
			catch (...) {
				DoStopCapture();
				break;
			}
		}
	}

	return -1;
}

void CV4LVideoSource::DoStartCapture(void)
{
	if (m_source) {
		return;
	}
	if (!Init()) {
		return;
	}

	m_source = true;
}

void CV4LVideoSource::DoStopCapture(void)
{
	if (!m_source) {
		return;
	}

	DoStopVideo();

	ReleaseDevice();

	m_source = false;
}

bool CV4LVideoSource::Init(void)
{
	if (!InitDevice()) {
		return false;
	}

	m_pConfig->CalculateVideoFrameSize();

	InitVideo(
		(m_pConfig->m_videoNeedRgbToYuv ?
			CMediaFrame::RgbVideoFrame :
			CMediaFrame::YuvVideoFrame),
		true);

	SetVideoSrcSize(
		m_pConfig->GetIntegerValue(CONFIG_VIDEO_RAW_WIDTH),
		m_pConfig->GetIntegerValue(CONFIG_VIDEO_RAW_HEIGHT),
		m_pConfig->GetIntegerValue(CONFIG_VIDEO_RAW_WIDTH),
		true);

	m_maxPasses = (u_int8_t)(m_videoSrcFrameRate + 0.5);

	return true;
}

bool CV4LVideoSource::InitDevice(void)
{
	int rc;
	char* deviceName = m_pConfig->GetStringValue(CONFIG_VIDEO_SOURCE_NAME);
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
	if (m_pConfig->GetIntegerValue(CONFIG_VIDEO_SIGNAL) == VIDEO_MODE_NTSC) {
		m_videoSrcFrameRate = VIDEO_NTSC_FRAME_RATE;
	} else {
		m_videoSrcFrameRate = VIDEO_PAL_FRAME_RATE;
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
			| ((videoFrequencyKHz % 1000) >> 6);

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
		PROT_READ | PROT_WRITE, MAP_SHARED, m_videoDevice, 0);
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

	SetPictureControls();

	if (videoCapability.audios) {
		SetVideoAudioMute(false);
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

void CV4LVideoSource::ReleaseDevice()
{
	SetVideoAudioMute(true);

	// release device resources
	munmap(m_videoMap, m_videoMbuf.size);
	m_videoMap = NULL;

	close(m_videoDevice);
	m_videoDevice = -1;
}
	
void CV4LVideoSource::SetVideoAudioMute(bool mute)
{
	if (!m_pConfig->m_videoCapabilities
	  || !m_pConfig->m_videoCapabilities->m_hasAudio) {
		return;
	}

	int rc;
	struct video_audio videoAudio;

	rc = ioctl(m_videoDevice, VIDIOCGAUDIO, &videoAudio);

	if (rc == 0 && (videoAudio.flags & VIDEO_AUDIO_MUTABLE)) {
		if (mute) {
			videoAudio.flags |= VIDEO_AUDIO_MUTE;
		} else {
			videoAudio.flags &= ~VIDEO_AUDIO_MUTE;
		}

		rc = ioctl(m_videoDevice, VIDIOCSAUDIO, &videoAudio);

		if (rc < 0) {
			debug_message("Can't set video audio for %s",
				m_pConfig->m_videoCapabilities->m_deviceName);
		}
	}
}

bool CV4LVideoSource::SetPictureControls()
{
	if (m_videoDevice == -1) {
		return false;
	}

	struct video_picture videoPicture;
	int rc;

	rc = ioctl(m_videoDevice, VIDIOCGPICT, &videoPicture);

	if (rc < 0) {
		return false;
	}

	videoPicture.brightness = (u_int16_t)
		((m_pConfig->GetIntegerValue(CONFIG_VIDEO_BRIGHTNESS) * 0xFFFF) / 100);
	videoPicture.hue = (u_int16_t)
		((m_pConfig->GetIntegerValue(CONFIG_VIDEO_HUE) * 0xFFFF) / 100);
	videoPicture.colour = (u_int16_t)
		((m_pConfig->GetIntegerValue(CONFIG_VIDEO_COLOR) * 0xFFFF) / 100);
	videoPicture.contrast = (u_int16_t)
		((m_pConfig->GetIntegerValue(CONFIG_VIDEO_CONTRAST) * 0xFFFF) / 100);

	rc = ioctl(m_videoDevice, VIDIOCSPICT, &videoPicture);

	if (rc < 0) {
		return false;
	}

	return true;
}

int8_t CV4LVideoSource::AcquireFrame(void)
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

void CV4LVideoSource::ProcessVideo(void)
{
	// for efficiency, process ~1 second before returning to check for commands
	for (int pass = 0; pass < m_maxPasses; pass++) {

		// get next frame from video capture device
		m_encodeHead = AcquireFrame();
		if (m_encodeHead == -1) {
			continue;
		}

		Timestamp frameTimestamp = GetTimestamp();

		u_int8_t* mallocedYuvImage = NULL;
		u_int8_t* pY;
		u_int8_t* pU;
		u_int8_t* pV;

		// perform colorspace conversion if necessary
		if (m_videoSrcType == CMediaFrame::RgbVideoFrame) {
			mallocedYuvImage = (u_int8_t*)Malloc(m_videoSrcYUVSize);

			pY = mallocedYuvImage;
			pU = pY + m_videoSrcYSize;
			pV = pU + m_videoSrcUVSize,

			RGB2YUV(
				m_videoSrcWidth,
				m_videoSrcHeight,
				(u_int8_t*)m_videoMap + m_videoMbuf.offsets[m_encodeHead],
				pY,
				pU,
				pV,
				1);
		} else {
			pY = (u_int8_t*)m_videoMap + m_videoMbuf.offsets[m_encodeHead];
			pU = pY + m_videoSrcYSize;
			pV = pU + m_videoSrcUVSize;
		}

		ProcessVideoYUVFrame(
			pY, 
			pU, 
			pV,
			m_videoSrcWidth,
			m_videoSrcWidth >> 1,
			frameTimestamp);

		// release video frame buffer back to video capture device
		if (ReleaseFrame(m_encodeHead)) {
			m_encodeHead = (m_encodeHead + 1) % m_videoMbuf.frames;
		} else {
			debug_message("Couldn't release capture buffer!");
		}

		free(mallocedYuvImage);
	}
}

bool CV4LVideoSource::InitialVideoProbe(CLiveConfig* pConfig)
{
	static char* devices[] = {
		"/dev/video", 
		"/dev/video0", 
		"/dev/video1", 
		"/dev/video2", 
		"/dev/video3"
	};
	char* deviceName = pConfig->GetStringValue(CONFIG_VIDEO_SOURCE_NAME);
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
			pConfig->SetStringValue(CONFIG_VIDEO_SOURCE_NAME, devices[i]);
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

	m_hasAudio = videoCapability.audios;

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


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
#ifndef HAVE_LINUX_VIDEODEV2_H
#include <sys/mman.h>

#include "video_v4l_source.h"
#include "video_util_rgb.h"
#include "video_util_filter.h"

const char *get_linux_video_type (void)
{
  return "V4L";
}

//#define DEBUG_TIMESTAMPS 1
int CV4LVideoSource::ThreadMain(void) 
{
  m_v4l_mutex = SDL_CreateMutex();
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
					DoStartCapture();
					break;
				case MSG_NODE_STOP:
					DoStopCapture();
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

	ReleaseDevice();

	m_source = false;
	SDL_DestroyMutex(m_v4l_mutex);
}

bool CV4LVideoSource::Init(void)
{
	m_pConfig->CalculateVideoFrameSize();

	InitVideo(true);

	SetVideoSrcSize(
		m_pConfig->GetIntegerValue(CONFIG_VIDEO_RAW_WIDTH),
		m_pConfig->GetIntegerValue(CONFIG_VIDEO_RAW_HEIGHT),
		m_pConfig->GetIntegerValue(CONFIG_VIDEO_RAW_WIDTH));
	if (!InitDevice()) {
		return false;
	}


	m_maxPasses = (u_int8_t)(m_videoSrcFrameRate + 0.5);

	return true;
}

bool CV4LVideoSource::InitDevice(void)
{
	int rc;
	const char* deviceName = m_pConfig->GetStringValue(CONFIG_VIDEO_SOURCE_NAME);
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
        switch(m_pConfig->GetIntegerValue(CONFIG_VIDEO_SIGNAL)) {
        case VIDEO_SIGNAL_PAL:
          videoChannel.norm = VIDEO_MODE_PAL;
          break;
        case VIDEO_SIGNAL_NTSC:
          videoChannel.norm = VIDEO_MODE_NTSC;
          break;
        case VIDEO_SIGNAL_SECAM:
          videoChannel.norm = VIDEO_MODE_SECAM;
          break;
        }
        rc = ioctl(m_videoDevice, VIDIOCSCHAN, &videoChannel);
	if (rc < 0) {
		error_message("Failed to set video channel info for %s",
			deviceName);
		goto failure;
	}
	if (m_pConfig->GetIntegerValue(CONFIG_VIDEO_SIGNAL) == VIDEO_SIGNAL_NTSC) {
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
		struct CHANLISTS* pChannelList = chanlists;
		struct CHANLIST* pChannel = pChannelList[
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

	m_videoFrameMapFrame = (uint64_t *)malloc(m_videoMbuf.frames * sizeof(uint64_t));
	m_videoFrameMapTimestamp = (uint64_t *)malloc(m_videoMbuf.frames * sizeof(Timestamp));
	m_captureHead = 0;
	m_encodeHead = -1;
	format = VIDEO_PALETTE_YUV420P;

	m_videoFrames = 0;
       m_videoSrcFrameDuration = 
		(Duration)(((float)TimestampTicks / m_videoSrcFrameRate) + 0.5);
       m_cacheTimestamp = true;
       if (m_pConfig->GetBoolValue(CONFIG_V4L_CACHE_TIMESTAMP) == false) {
	 m_videoMbuf.frames = 2;
	 m_cacheTimestamp = false;
       }
       uint32_t width, height;
       width = m_pConfig->GetIntegerValue(CONFIG_VIDEO_RAW_WIDTH);
       height = m_pConfig->GetIntegerValue(CONFIG_VIDEO_RAW_HEIGHT);
       
       if (strncasecmp(m_pConfig->GetStringValue(CONFIG_VIDEO_FILTER),
		       VIDEO_FILTER_DECIMATE,
		       strlen(VIDEO_FILTER_DECIMATE)) == 0) {
	 uint32_t max_width, max_height;
	 switch (m_pConfig->GetIntegerValue(CONFIG_VIDEO_SIGNAL)) {
	 case VIDEO_SIGNAL_NTSC:
	   max_width = 720;
	   max_height = 480;
	   break;
	 case VIDEO_SIGNAL_PAL:
	 case VIDEO_SIGNAL_SECAM:
	 default:
	   max_width = 768;
	   max_height = 576;
	   break;
	 }
	 if (max_width < width * 2 || max_height < height * 2) {
	   error_message("Decimate filter choosen with too large video size - max %ux%u",
			 max_width / 2, max_height / 2);
	 } else {
	   m_decimate_filter = true;
	   width *= 2;
	   height *= 2;
	 }
       }

	for (int i = 0; i < m_videoMbuf.frames; i++) {
		// initialize frame map
		m_videoFrameMap[i].frame = i;
		m_videoFrameMap[i].width = width;
		m_videoFrameMap[i].height = height;
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

			error_message("Failed to allocate video capture buffer for %s - frame %d max %d rc %d errno %d format %d", 
				deviceName,
				      i, m_videoMbuf.frames, 
				      rc, errno, format);
			goto failure;
		}
		if (i == 0) {
		  m_videoCaptureStartTimestamp = GetTimestamp();
		}
		m_lastVideoFrameMapFrameLoaded = m_videoFrameMapFrame[i] = i;
		m_lastVideoFrameMapTimestampLoaded = 
		  m_videoFrameMapTimestamp[i] = 
		  CalculateVideoTimestampFromFrames(i);
	}

	m_videoNeedRgbToYuv = (format == VIDEO_PALETTE_RGB24);

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
  if (!m_pConfig->m_videoCapabilities) return;

  CVideoCapabilities *vc = (CVideoCapabilities *)m_pConfig->m_videoCapabilities;
  if ( !vc->m_hasAudio) {
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

int8_t CV4LVideoSource::AcquireFrame(Timestamp &frameTimestamp)
{
	int rc;
	ReleaseFrames();
	rc = ioctl(m_videoDevice, VIDIOCSYNC, &m_videoFrameMap[m_captureHead]);
	if (rc != 0) {
		return -1;
	}

	if (m_cacheTimestamp)
	  frameTimestamp = m_videoFrameMapTimestamp[m_captureHead];
	else
	  frameTimestamp = GetTimestamp();

	int8_t capturedFrame = m_captureHead;
	m_captureHead = (m_captureHead + 1) % m_videoMbuf.frames;

	return capturedFrame;
}

void c_ReleaseFrame (void *f)
{
  yuv_media_frame_t *yuv = (yuv_media_frame_t *)f;
  if (yuv->free_y) {
    CHECK_AND_FREE(yuv->y);
  } else {
    CV4LVideoSource *s = (CV4LVideoSource *)yuv->hardware;
    s->IndicateReleaseFrame(yuv->hardware_index);
  }
  free(yuv);
}

void CV4LVideoSource::ReleaseFrames (void)
{
  uint32_t index_mask = 1;
  uint32_t released_mask;
  uint32_t back_on_queue_mask = 0;

  SDL_LockMutex(m_v4l_mutex);
  released_mask = m_release_index_mask;
  SDL_UnlockMutex(m_v4l_mutex);

  m_release_index_mask = 0;

  while (released_mask != 0) {
    index_mask = 1 << m_encodeHead;
    if ((index_mask & released_mask) != 0) {
      back_on_queue_mask |= index_mask;
      Timestamp calc = GetTimestamp();

      if (calc > m_videoSrcFrameDuration + m_lastVideoFrameMapTimestampLoaded) {
#ifdef DEBUG_TIMESTAMPS
	debug_message("video frame delay past end of buffer - time is "U64" should be "U64,
		      calc,
		      m_videoSrcFrameDuration + m_lastVideoFrameMapTimestampLoaded);
#endif
	m_videoCaptureStartTimestamp = calc;
	m_videoFrameMapFrame[m_encodeHead] = 0;
	m_videoFrameMapTimestamp[m_encodeHead] = calc;
      } else {
	m_videoFrameMapFrame[m_encodeHead] = m_lastVideoFrameMapFrameLoaded + 1;
	m_videoFrameMapTimestamp[m_encodeHead] =
	  CalculateVideoTimestampFromFrames(m_videoFrameMapFrame[m_encodeHead]);
      }

      m_lastVideoFrameMapFrameLoaded = m_videoFrameMapFrame[m_encodeHead];
      m_lastVideoFrameMapTimestampLoaded = m_videoFrameMapTimestamp[m_encodeHead];
      ioctl(m_videoDevice, VIDIOCMCAPTURE, 
	    &m_videoFrameMap[m_encodeHead]);
      m_encodeHead = (m_encodeHead + 1) % m_videoMbuf.frames;
    } else {
      released_mask = 0;
    }
  }
  if (back_on_queue_mask != 0) {
    SDL_LockMutex(m_v4l_mutex);
    m_release_index_mask &= ~back_on_queue_mask;
    SDL_UnlockMutex(m_v4l_mutex);
  }
  
}

void CV4LVideoSource::ProcessVideo(void)
{
	// for efficiency, process ~1 second before returning to check for commands
  Timestamp frameTimestamp;
	for (int pass = 0; pass < m_maxPasses; pass++) {

		// get next frame from video capture device
	  int8_t index;
	  index = AcquireFrame(frameTimestamp);
	  if (index == -1) {
	    continue;
	  }


	  u_int8_t* mallocedYuvImage = NULL;
	  u_int8_t* pY;
	  u_int8_t* pU;
	  u_int8_t* pV;
	  
	  // perform colorspace conversion if necessary
	  if (m_videoNeedRgbToYuv) {
	    mallocedYuvImage = (u_int8_t*)Malloc(m_videoSrcYUVSize);
	    
	    pY = mallocedYuvImage;
	    pU = pY + m_videoSrcYSize;
	    pV = pU + m_videoSrcUVSize,
	      
	      RGB2YUV(
		      m_videoSrcWidth,
		      m_videoSrcHeight,
		      (u_int8_t*)m_videoMap + m_videoMbuf.offsets[index],
		      pY,
		      pU,
		      pV,
		      1, false);
	  } else {
	    pY = (u_int8_t*)m_videoMap + m_videoMbuf.offsets[index];
	    pU = pY + m_videoSrcYSize;
	    pV = pU + m_videoSrcUVSize;
	  }

	  if (m_decimate_filter) {
	    video_filter_decimate(pY,
				  m_videoSrcWidth,
				  m_videoSrcHeight);
	  }
	  yuv_media_frame_t *yuv = MALLOC_STRUCTURE(yuv_media_frame_t);
	  yuv->y = pY;
	  yuv->u = pU;
	  yuv->v = pV;
	  yuv->y_stride = m_videoSrcWidth;
	  yuv->uv_stride = m_videoSrcWidth >> 1;
	  yuv->w = m_videoSrcWidth;
	  yuv->h = m_videoSrcHeight;
	  yuv->hardware = this;
	  yuv->hardware_index = index;
	  if (m_videoWantKeyFrame && frameTimestamp >= m_audioStartTimestamp) {
	    yuv->force_iframe = true;
	    m_videoWantKeyFrame = false;
	    debug_message("Frame "U64" request key frame", frameTimestamp);
	  } else 
	    yuv->force_iframe = false;
	  yuv->free_y = (mallocedYuvImage != NULL);

	  CMediaFrame *frame = new CMediaFrame(YUVVIDEOFRAME,
					       yuv,
					       0,
					       frameTimestamp);
	  frame->SetMediaFreeFunction(c_ReleaseFrame);
	  ForwardFrame(frame);
    //debug_message("video source forward");
    // enqueue the frame to video capture buffer
	  if (mallocedYuvImage != NULL) {
	    IndicateReleaseFrame(index);
	  }
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
	const char* deviceName = pConfig->GetStringValue(CONFIG_VIDEO_SOURCE_NAME);
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

	m_driverName = strdup(videoCapability.name);
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
		m_inputNames[i] = strdup(videoChannel.name);
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

static const char *signals[] = {
  "PAL", "NTSC", "SECAM",
};
void CVideoCapabilities::Display (CLiveConfig *pConfig,
				  char *msg, 
				  uint32_t max_len)
{
  uint32_t port = pConfig->GetIntegerValue(CONFIG_VIDEO_INPUT);

  if (port >= m_numInputs) {
    snprintf(msg, max_len, "Video port has illegal value");
    return;
  }
  if (m_inputHasTuners[port] == false) {
    snprintf(msg, max_len, 
	     "%s, %ux%u, %s, %s",
	     pConfig->GetStringValue(CONFIG_VIDEO_SOURCE_NAME),
	     pConfig->m_videoWidth,
	     pConfig->m_videoHeight,
	     signals[pConfig->GetIntegerValue(CONFIG_VIDEO_SIGNAL)],
	     m_inputNames[port]);
  } else {
    snprintf(msg, max_len, 
	     "%s, %ux%u, %s, %s, %s, channel %s",
	     pConfig->GetStringValue(CONFIG_VIDEO_SOURCE_NAME),
	     pConfig->m_videoWidth,
	     pConfig->m_videoHeight,
	     signals[pConfig->GetIntegerValue(CONFIG_VIDEO_SIGNAL)],
	     m_inputNames[port],
	     chanlists[pConfig->GetIntegerValue(CONFIG_VIDEO_CHANNEL_LIST_INDEX)].name,
	     chanlists[pConfig->GetIntegerValue(CONFIG_VIDEO_CHANNEL_LIST_INDEX)].list[pConfig->GetIntegerValue(CONFIG_VIDEO_CHANNEL_INDEX)].name
	     );
  }
}

#endif

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
 * Copyright (C) Cisco Systems Inc. 2003.  All Rights Reserved.
 * 
 * Contributor(s): 
 *              Waqar Mohsin            wmohsin@cisco.com
 */

#include "mp4live.h"

#include <sys/mman.h>

#include "video_v4l2_source.h"
#include "video_util_rgb.h"


int CV4L2VideoSource::ThreadMain(void) 
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

void CV4L2VideoSource::DoStartCapture(void)
{
  if (m_source) return;
  if (!Init()) return;
  m_source = true;
}

void CV4L2VideoSource::DoStopCapture(void)
{
  if (!m_source) return;
  DoStopVideo();
  ReleaseDevice();
  m_source = false;
}

bool CV4L2VideoSource::Init(void)
{
  if (!InitDevice()) return false;

  m_pConfig->CalculateVideoFrameSize();

  InitVideo((m_pConfig->m_videoNeedRgbToYuv ?
             RGBVIDEOFRAME : YUVVIDEOFRAME), true);

  SetVideoSrcSize(
                  m_pConfig->GetIntegerValue(CONFIG_VIDEO_RAW_WIDTH),
                  m_pConfig->GetIntegerValue(CONFIG_VIDEO_RAW_HEIGHT),
                  m_pConfig->GetIntegerValue(CONFIG_VIDEO_RAW_WIDTH),
                  true);

  m_maxPasses = (u_int8_t)(m_videoSrcFrameRate + 0.5);

  return true;
}

bool CV4L2VideoSource::InitDevice(void)
{
  int rc;
  char* deviceName = m_pConfig->GetStringValue(CONFIG_VIDEO_SOURCE_NAME);
  int buftype = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  v4l2_std_id std;

  if (m_videoDevice != -1)
    debug_message("video device already open !!!");

  // open the video device
  m_videoDevice = open(deviceName, O_RDWR);
  if (m_videoDevice < 0) {
    error_message("Failed to open %s: %s", deviceName, strerror(errno));
    return false;
  }

  // query device capabilities
  struct v4l2_capability capability;
  rc = ioctl(m_videoDevice, VIDIOC_QUERYCAP, &capability);
  if (rc < 0) {
    error_message("Failed to query video capabilities for %s", deviceName);
    goto failure;
  }

  // make sure device supports video capture
  if (!(capability.capabilities & V4L2_CAP_VIDEO_CAPTURE)) {
    error_message("Device %s is not capable of video capture!", deviceName);
    goto failure;
  }

  // query attributes of video input
  struct v4l2_input input;
  input.index = m_pConfig->GetIntegerValue(CONFIG_VIDEO_INPUT);
  rc = ioctl(m_videoDevice, VIDIOC_ENUMINPUT, &input);
  if (rc < 0) {
    error_message("Failed to enumerate video input %d for %s",
                  input.index, deviceName);
    goto failure;
  }

  // select video input
  rc = ioctl(m_videoDevice, VIDIOC_S_INPUT, &input.index);
  if (rc < 0) {
    error_message("Failed to select video input %d for %s",
                  input.index, deviceName);
    goto failure;
  }
  
  // select video input standard type
  switch(m_pConfig->GetIntegerValue(CONFIG_VIDEO_SIGNAL)) {
  case VIDEO_SIGNAL_PAL:
    std = V4L2_STD_PAL;
    break;
  case VIDEO_SIGNAL_NTSC:
    std = V4L2_STD_NTSC;
    break;
  case VIDEO_SIGNAL_SECAM:
    std = V4L2_STD_SECAM;
    break;
  }
  rc = ioctl(m_videoDevice, VIDIOC_S_STD, &std);
  if (rc < 0) {
    error_message("Failed to select video standard for %s", deviceName);
    goto failure;
  }

  if (m_pConfig->GetIntegerValue(CONFIG_VIDEO_SIGNAL) == VIDEO_SIGNAL_NTSC) {
    m_videoSrcFrameRate = VIDEO_NTSC_FRAME_RATE;
  } else {
    m_videoSrcFrameRate = VIDEO_PAL_FRAME_RATE;
  }

  // input source has a tuner
  if (input.type == V4L2_INPUT_TYPE_TUNER) {
    // get tuner info
    if ((int32_t)m_pConfig->GetIntegerValue(CONFIG_VIDEO_TUNER) == -1) {
      m_pConfig->SetIntegerValue(CONFIG_VIDEO_TUNER, 0);
    }

    // query tuner attributes
    struct v4l2_tuner tuner;
    memset(&tuner, 0, sizeof(tuner));
    tuner.index = m_pConfig->GetIntegerValue(CONFIG_VIDEO_TUNER);
    //tuner.index = input.tuner;
    rc = ioctl(m_videoDevice, VIDIOC_G_TUNER, &tuner);
    if (rc < 0) {
      error_message("Failed to query tuner attributes for %s", deviceName);
    }

    // tuner is an analog TV tuner
    if (tuner.type == V4L2_TUNER_ANALOG_TV) {

      // tune in the desired frequency (channel)
      struct CHANLISTS* pChannelList = chanlists;
      struct CHANLIST* pChannel =
        pChannelList[m_pConfig->GetIntegerValue(CONFIG_VIDEO_CHANNEL_LIST_INDEX)].list;
      unsigned long videoFrequencyKHz =
        pChannel[m_pConfig->GetIntegerValue(CONFIG_VIDEO_CHANNEL_INDEX)].freq;
      unsigned long videoFrequencyTuner =
        ((videoFrequencyKHz / 1000) << 4) | ((videoFrequencyKHz % 1000) >> 6);

      struct v4l2_frequency frequency;
      frequency.tuner = tuner.index;
      frequency.type = tuner.type;
      frequency.frequency = videoFrequencyTuner;

      rc = ioctl(m_videoDevice, VIDIOC_S_FREQUENCY, &frequency);
      if (rc < 0) {
        error_message("Failed to set video tuner frequency for %s", deviceName);
      }
    }
  }

  // query image format
  struct v4l2_format format;
  format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  rc = ioctl(m_videoDevice, VIDIOC_G_FMT, &format);
  if (rc < 0) {
    error_message("Failed to query video image format for %s", deviceName);
    goto failure;
  }
  
  // select image format
  format.fmt.pix.width = m_pConfig->GetIntegerValue(CONFIG_VIDEO_RAW_WIDTH);
  format.fmt.pix.height = m_pConfig->GetIntegerValue(CONFIG_VIDEO_RAW_HEIGHT);
  format.fmt.pix.pixelformat = V4L2_PIX_FMT_YVU420;
  rc = ioctl(m_videoDevice, VIDIOC_S_FMT, &format);
  if (rc < 0) {
    // try RGB24 palette instead
    format.fmt.pix.pixelformat = V4L2_PIX_FMT_RGB24;
    rc = ioctl(m_videoDevice, VIDIOC_S_FMT, &format);
    if (rc < 0) {
      error_message("Failed to select video image format for %s", deviceName);
      goto failure;
    }
  }
  
  // allocate the desired number of buffers
  struct v4l2_requestbuffers reqbuf;
  reqbuf.count = m_pConfig->GetIntegerValue(CONFIG_VIDEO_CAP_BUFF_COUNT);
  reqbuf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  reqbuf.memory = V4L2_MEMORY_MMAP;
  rc = ioctl(m_videoDevice, VIDIOC_REQBUFS, &reqbuf);
  if (rc < 0 || reqbuf.count < 1) {
    error_message("Failed to allocate buffers for %s", deviceName);
    goto failure;
  }

  // map the video capture buffers
  m_buffers_count = reqbuf.count;
  m_buffers = (capture_buffer_t*)calloc(reqbuf.count, sizeof(capture_buffer_t));
  if (m_buffers == NULL) {
    error_message("Failed to allocate memory for m_buffers");
    return false;
  }
  
  for(uint32_t ix=0; ix<reqbuf.count; ix++) {
    struct v4l2_buffer buffer;
    buffer.index = ix;
    buffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    rc = ioctl(m_videoDevice, VIDIOC_QUERYBUF, &buffer);
    if (rc < 0) {
      error_message("Failed to query video capture buffer status for %s",
                    deviceName);
      goto failure;
    }

    m_buffers[ix].length = buffer.length;
    m_buffers[ix].start = mmap(NULL, buffer.length,
                               PROT_READ | PROT_WRITE,
                               MAP_SHARED,
                               m_videoDevice, buffer.m.offset);

    if (m_buffers[ix].start == MAP_FAILED) {
      error_message("Failed to map video capture buffer for %s", deviceName);
      goto failure;
    }

    //enqueue the mapped buffer
    rc = ioctl(m_videoDevice, VIDIOC_QBUF, &buffer);
    if (rc < 0) {
      error_message("Failed to enqueue video capture buffer status for %s",
                    deviceName);
      goto failure;
    }
  }

  SetPictureControls();

  if (capability.capabilities & V4L2_CAP_AUDIO) {
    SetVideoAudioMute(false);
  }

  // start video capture
  rc = ioctl(m_videoDevice, VIDIOC_STREAMON, &buftype);
  if (rc < 0) {
    error_message("Failed to start video capture for %s", deviceName);
    perror("Reason");
    goto failure;
  }

  // start video capture again
  rc = ioctl(m_videoDevice, VIDIOC_STREAMON, &buftype);
  if (rc < 0) {
    error_message("Failed to start video capture again for %s", deviceName);
    perror("Again Reason");
    goto failure;
  }

  return true;

 failure:
  for (uint32_t i = 0; i < m_buffers_count; i++) {
    if (m_buffers[i].start)
      munmap(m_buffers[i].start, m_buffers[i].length);
  }
  if (m_buffers) free(m_buffers);
  
  close(m_videoDevice);
  m_videoDevice = -1;
  return false;
}

void CV4L2VideoSource::ReleaseDevice()
{
  SetVideoAudioMute(true);

  // stop video capture
  int buftype = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  int rc = ioctl(m_videoDevice, VIDIOC_STREAMOFF, &buftype);
  if (rc < 0) {
    error_message("Failed to stop video capture");
  }

  // release device resources
  for (uint32_t i = 0; i < m_buffers_count; i++) {
    if (m_buffers[i].start)
      munmap(m_buffers[i].start, m_buffers[i].length);
  }
  free(m_buffers);

  close(m_videoDevice);
  m_videoDevice = -1;
}
	
void CV4L2VideoSource::SetVideoAudioMute(bool mute)
{
  if (!m_pConfig->m_videoCapabilities
      || !m_pConfig->m_videoCapabilities->m_hasAudio) {
    return;
  }

  int rc;
  struct v4l2_control control;

  control.id = V4L2_CID_AUDIO_MUTE;
  rc = ioctl(m_videoDevice, VIDIOC_G_CTRL, &control);
  if (rc < 0) {
    error_message("V4L2_CID_AUDIO_MUTE not supported");
  } else {
    control.value = mute ? true : false;
    rc = ioctl(m_videoDevice, VIDIOC_S_CTRL, &control);
    if (rc < 0) {
      error_message("Couldn't set audio mute for %s ",
                    m_pConfig->m_videoCapabilities->m_deviceName);
    }
  }
}

bool CV4L2VideoSource::SetPictureControls()
{
  if (m_videoDevice == -1) return false;

  int rc;
  struct v4l2_control control;

  // brightness
  control.id = V4L2_CID_BRIGHTNESS;
  rc = ioctl(m_videoDevice, VIDIOC_G_CTRL, &control);
  if (rc < 0) {
     error_message("V4L2_CID_BRIGHTNESS not supported");
  } else {
    control.value = (int)
      ((m_pConfig->GetIntegerValue(CONFIG_VIDEO_BRIGHTNESS) * 0xFFFF) / 100);
    rc = ioctl(m_videoDevice, VIDIOC_S_CTRL, &control);
    if (rc < 0) {
      error_message("Couldn't set brightness level");
      return false;
    }
  }

  // hue
  control.id = V4L2_CID_HUE;
  rc = ioctl(m_videoDevice, VIDIOC_G_CTRL, &control);
  if (rc < 0) {
    error_message("V4L2_CID_HUE not supported");
  } else {
    control.value = (int)
      ((m_pConfig->GetIntegerValue(CONFIG_VIDEO_HUE) * 0xFFFF) / 100);
    rc = ioctl(m_videoDevice, VIDIOC_S_CTRL, &control);
    if (rc < 0) {
      error_message("Couldn't set hue level");
      return false;
    }
  }

  // color
  control.id = V4L2_CID_SATURATION;
  rc = ioctl(m_videoDevice, VIDIOC_G_CTRL, &control);
  if (rc < 0) {
    error_message("V4L2_CID_SATURATION not supported");
  } else {
    control.value = (int)
      ((m_pConfig->GetIntegerValue(CONFIG_VIDEO_COLOR) * 0xFFFF) / 100);
    rc = ioctl(m_videoDevice, VIDIOC_S_CTRL, &control);
    if (rc < 0) {
      error_message("Couldn't set saturation level");
      return false;
    }
  }

  // contrast
  control.id = V4L2_CID_CONTRAST;
  rc = ioctl(m_videoDevice, VIDIOC_G_CTRL, &control);
  if (rc < 0) {
    error_message("V4L2_CID_CONTRAST not supported");
  } else {
    control.value = (int)
      ((m_pConfig->GetIntegerValue(CONFIG_VIDEO_CONTRAST) * 0xFFFF) / 100);
    rc = ioctl(m_videoDevice, VIDIOC_S_CTRL, &control);
    if (rc < 0) {
      error_message("Couldn't set contrast level");
      return false;
    }
  }

  return true;
}

int8_t CV4L2VideoSource::AcquireFrame(Timestamp &frameTimestamp)
{
  struct v4l2_buffer buffer;
  buffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

  int rc = ioctl(m_videoDevice, VIDIOC_DQBUF, &buffer);
  if (rc != 0) {
    return -1;
  }

  frameTimestamp = GetTimestampFromTimeval(&(buffer.timestamp));
  return buffer.index;
}

void CV4L2VideoSource::ReleaseFrame(uint8_t index)
{
  struct v4l2_buffer buffer;
  buffer.index = index;
  buffer.memory = V4L2_MEMORY_MMAP;
  buffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

  int rc = ioctl(m_videoDevice, VIDIOC_QBUF, &buffer);
  if (rc < 0) {
    error_message("Could not enqueue buffer to video capture queue");
  }
}

void CV4L2VideoSource::ProcessVideo(void)
{
  // for efficiency, process ~1 second before returning to check for commands
  Timestamp frameTimestamp;
  for (int pass = 0; pass < m_maxPasses; pass++) {

    // dequeue next frame from video capture buffer
    int index = AcquireFrame(frameTimestamp);
    if (index == -1) {
      continue;
    }

    u_int8_t* mallocedYuvImage = NULL;
    u_int8_t* pY;
    u_int8_t* pU;
    u_int8_t* pV;

    // perform colorspace conversion if necessary
    if (m_videoSrcType == RGBVIDEOFRAME) {
      mallocedYuvImage = (u_int8_t*)Malloc(m_videoSrcYUVSize);

      pY = mallocedYuvImage;
      pV = pY + m_videoSrcYSize;
      pU = pV + m_videoSrcUVSize,

        RGB2YUV(
                m_videoSrcWidth,
                m_videoSrcHeight,
                (u_int8_t*)m_buffers[index].start,
                pY,
                pU,
                pV,
                1);
    } else {
      pY = (u_int8_t*)m_buffers[index].start;
      pV = pY + m_videoSrcYSize;
      pU = pV + m_videoSrcUVSize;
    }

    ProcessVideoYUVFrame(
                         pY, 
                         pU, 
                         pV,
                         m_videoSrcWidth,
                         m_videoSrcWidth >> 1,
                         frameTimestamp);

    // enqueue the frame to video capture buffer
    ReleaseFrame(index);

    if (mallocedYuvImage != NULL) {
      free(mallocedYuvImage);
    }
  }
}

bool CV4L2VideoSource::InitialVideoProbe(CLiveConfig* pConfig)
{
  static const char* devices[] = {
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
  for (unsigned int i = 0; i < sizeof(devices) / sizeof(*devices); i++) {

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
  int rc, i;

  // open the video device
  int videoDevice = open(m_deviceName, O_RDWR);
  if (videoDevice < 0) return false;
  m_canOpen = true;

  // query device capabilities
  struct v4l2_capability capability;
  rc = ioctl(videoDevice, VIDIOC_QUERYCAP, &capability);
  if (rc < 0) {
    error_message("Failed to query video capabilities for %s", m_deviceName);
    close(videoDevice);
    return false;
  }

  // make sure device supports video capture
  if (!(capability.capabilities & V4L2_CAP_VIDEO_CAPTURE)) {
    error_message("Device %s is not capable of video capture!", m_deviceName);
    close(videoDevice);
    return false;
  }

  m_canCapture = true;
  m_driverName = stralloc((char*)capability.driver);
  m_hasAudio = capability.capabilities & V4L2_CAP_AUDIO;

  struct v4l2_input input;

  // get the number of inputs
  for(i=0; ; i++) {
    input.index = i;
    rc = ioctl(videoDevice, VIDIOC_ENUMINPUT, &input);
    if (rc < 0) {
      if (errno == EINVAL) break;
    }
  }

  m_numInputs = i;

  m_inputNames = (char**)malloc(m_numInputs * sizeof(char*));
  memset(m_inputNames, 0, m_numInputs * sizeof(char*));

  m_inputHasTuners = (bool*)malloc(m_numInputs * sizeof(bool));
  memset(m_inputHasTuners, 0, m_numInputs * sizeof(bool));

  m_inputTunerSignalTypes = (u_int8_t*)malloc(m_numInputs * sizeof(u_int8_t));
  memset(m_inputTunerSignalTypes, 0, m_numInputs * sizeof(u_int8_t));
  
  // enumerate all inputs
  for(i=0; i<m_numInputs; i++) {
    input.index = i;
    rc = ioctl(videoDevice, VIDIOC_ENUMINPUT, &input);
    if (rc < 0) {
      error_message("Failed to enumerate video input %d for %s",
                    i, m_deviceName);
      continue;
    }
    m_inputNames[i] = stralloc((char*)input.name);

    error_message("type %d %s type %x", i, m_inputNames[i], input.type);
    if (input.type == V4L2_INPUT_TYPE_TUNER) {
      error_message("Has tuner");
      m_inputHasTuners[i] = true;

      if (input.std & V4L2_STD_PAL) m_inputTunerSignalTypes[i] |= 0x1;
      if (input.std & V4L2_STD_NTSC) m_inputTunerSignalTypes[i] |= 0x2;
      if (input.std & V4L2_STD_SECAM) m_inputTunerSignalTypes[i] |= 0x4;
    }
  }

  close(videoDevice);
  return true;
}

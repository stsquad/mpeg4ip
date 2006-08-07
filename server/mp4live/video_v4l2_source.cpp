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
 * Copyright (C) Cisco Systems Inc. 2003-2005.  All Rights Reserved.
 * 
 * Contributor(s): 
 *              Waqar Mohsin            wmohsin@cisco.com
 *              Bill May     wmay@cisco.com
 *              Charlie Normand  charlienormand@cantab.net alias cpn24
 */

#include "mp4live.h"
#include <sys/mman.h>

#include "video_v4l_source.h"
#include "video_util_rgb.h"
#include "video_util_filter.h"
#include "video_util_convert.h"

const char *get_linux_video_type (void)
{
  return "V4L2";
}

int CV4LVideoSource::ThreadMain(void) 
{
  debug_message("v4l2 thread start");
  m_v4l_mutex = NULL;
  m_waiting_frames_return = false;
  while (true) {
    int rc;

   if (m_source && m_waiting_frames_return == false) {
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
          while (DoStopCapture() == false) {
	    SDL_Delay(10);	// ensure things get cleaned up
	    debug_message("waiting for stop thread");
	  }
          delete pMsg;
	  debug_message("v4l2 thread exit");
          return 0;
        case MSG_NODE_START:
          DoStartCapture();
	  debug_message("v4l2 thread start capture");
          break;
        case MSG_NODE_STOP:
	  debug_message("v4l2 thread stop capture");
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
    } else {
      DoStopCapture();
    }
  }

  debug_message("v4l2 thread exit");
  return -1;
}

void CV4LVideoSource::DoStartCapture(void)
{
  if (m_source) {
    return;
  }
  if (m_v4l_mutex == NULL) 
  m_v4l_mutex = SDL_CreateMutex();

  if (!Init()) return;
  m_source = true;
}

bool CV4LVideoSource::DoStopCapture(void)
{
  if (m_source) {
    debug_message("dostopcapture - releasing device");
    ReleaseDevice();
#ifdef CAPTURE_RAW
    if (m_rawfile) {
      fclose(m_rawfile);
      m_rawfile = NULL;
    }
#endif
  }
  m_source = false;
  if (ReleaseBuffers() == false) {
    debug_message("releasebuffers false");
    return false;
  }

  SDL_DestroyMutex(m_v4l_mutex);
  m_v4l_mutex = NULL;
  return true;
}
bool CV4LVideoSource::Init(void)
{
  m_pConfig->CalculateVideoFrameSize();

  InitVideo(true);

  SetVideoSrcSize(
                  m_pConfig->GetIntegerValue(CONFIG_VIDEO_RAW_WIDTH),
                  m_pConfig->GetIntegerValue(CONFIG_VIDEO_RAW_HEIGHT),
                  m_pConfig->GetIntegerValue(CONFIG_VIDEO_RAW_WIDTH));

  if (!InitDevice()) return false;

  m_maxPasses = (u_int8_t)(m_videoSrcFrameRate + 0.5);

  return true;
}

static const uint32_t formats[] = {
  V4L2_PIX_FMT_YUV420,
  V4L2_PIX_FMT_YVU420,
  V4L2_PIX_FMT_YUYV,
  V4L2_PIX_FMT_UYVY,
  V4L2_PIX_FMT_YYUV,
  V4L2_PIX_FMT_NV12,
  V4L2_PIX_FMT_RGB24,
  V4L2_PIX_FMT_BGR24,
};

bool CV4LVideoSource::InitDevice(void)
{
  int rc;
  int sinput_index;
#ifdef CAPTURE_RAW
  m_rawfile = fopen("raw.yuv", FOPEN_WRITE_BINARY);
#endif
  SDL_LockMutex(m_v4l_mutex);
  if (m_buffers != NULL) {
    unreleased_capture_buffers_t *p = 
      MALLOC_STRUCTURE(unreleased_capture_buffers_t);
    debug_message("creating unreleased capture buffer, version %u", 
		  m_hardware_version);
    p->buffer_list = m_buffers;
    m_buffers = NULL;
    p->hardware_version = m_hardware_version;
    p->next = m_unreleased_buffers_list;
    m_unreleased_buffers_list = p;
  }
  m_hardware_version++;
  SDL_UnlockMutex(m_v4l_mutex);

  const char* deviceName = m_pConfig->GetStringValue(CONFIG_VIDEO_SOURCE_NAME);
  int buftype = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  bool pass = false;
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
  memset(&capability, 0, sizeof(capability));
  rc = ioctl(m_videoDevice, VIDIOC_QUERYCAP, &capability);
  if (rc < 0) {
    error_message("Failed to query video capabilities for %s - %s", deviceName,
		  strerror(errno));
    goto failure;
  }
  // make sure device supports video capture
  if (!(capability.capabilities & V4L2_CAP_VIDEO_CAPTURE)) {
    error_message("Device %s is not capable of video capture!", deviceName);
    goto failure;
  }

  // query attributes of video input
  struct v4l2_input input;
  memset(&input, 0, sizeof(input));

  uint input_value;
  input_value = m_pConfig->GetIntegerValue(CONFIG_VIDEO_INPUT);
  if (m_pConfig->m_videoCapabilities != NULL) {
    uint compare_value = m_pConfig->m_videoCapabilities->m_numInputs;
    if (compare_value > 0) compare_value--;
    if (input_value > compare_value) {
      input_value = 0;
      m_pConfig->SetIntegerValue(CONFIG_VIDEO_INPUT, input_value);
      error_message("Video input value exceed capabilities; replacing with 0");
    }
  }

  input.index = input_value;
  rc = ioctl(m_videoDevice, VIDIOC_ENUMINPUT, &input);
  if (rc < 0) {
    error_message("Failed to enumerate video input %d for %s, %s",
                  input.index, deviceName, strerror(errno));
    //goto failure;
  }

  // select video input
  sinput_index = input.index;
  rc = ioctl(m_videoDevice, VIDIOC_S_INPUT, &sinput_index);
  if (rc < 0) {
    error_message("Failed to select video input %d for %s - %s",
                  input.index, deviceName, strerror(errno));
    //goto failure;
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
    error_message("Failed to select video standard for %s - %s", deviceName,
		  strerror(errno));
    //goto failure;
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
      error_message("Failed to query tuner attributes for %s, %s", deviceName,
		    strerror(errno));
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
      memset(&frequency, 0, sizeof(frequency));
      frequency.tuner = tuner.index;
      frequency.type = tuner.type;
      frequency.frequency = videoFrequencyTuner;

      rc = ioctl(m_videoDevice, VIDIOC_S_FREQUENCY, &frequency);
      if (rc < 0) {
        error_message("Failed to set video tuner frequency for %s - %s", deviceName,
		      strerror(errno));
      }
    }
  }

  // query image format
  
  // select image format
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
      error_message("Decimate filter choosen with too large video size - %ux%u max %ux%u",
		    width, height, max_width / 2, max_height / 2);
    } else {
      m_decimate_filter = true;
      width *= 2;
      height *= 2;
    }
  }

  for (uint ix = 0; 
       pass == false && ix < NUM_ELEMENTS_IN_ARRAY(formats); 
       ix++) {
    struct v4l2_format format;
    memset(&format, 0, sizeof(format));
    format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    rc = ioctl(m_videoDevice, VIDIOC_G_FMT, &format);
    if (rc < 0) {
      error_message("Failed to query video image format for %s, %s", deviceName,
		    strerror(errno));
      goto failure;
    }
    format.fmt.pix.width = width;
    format.fmt.pix.height = height;
    format.fmt.pix.pixelformat = formats[ix];

    rc = ioctl(m_videoDevice, VIDIOC_S_FMT, &format);
    if (rc == 0 && format.fmt.pix.pixelformat == formats[ix]) {
      m_format = formats[ix];
      pass = true;
    } else {
      debug_message("format %c%c%c%c return code %d", 
		    formats[ix] & 0xff,
		    formats[ix] >> 8,
		    formats[ix] >> 16,
		    formats[ix] >> 24,
		    rc);
    }
    if (format.fmt.pix.width != width) {
      error_message("format %u - returned width %u not selected %u", 
		    formats[ix], format.fmt.pix.width, width);
    }
    if (format.fmt.pix.height != height) {
      error_message("format %u - returned height %u not selected %u", 
		    formats[ix], format.fmt.pix.height, height);
    }
  }
  if (pass == false) {
      error_message("Failed to select any video formats for %s", deviceName);
      goto failure;
  }
 
 //turn auto exposure off for ovfx2 camera
 // int exposure=0;
 // ioctl(m_videoDevice, VIDIOCCAPTURE, &exposure);



  switch (m_format) {
  case V4L2_PIX_FMT_YVU420:
    m_v_offset = m_videoSrcYSize;
    m_u_offset = m_videoSrcYSize + m_videoSrcUVSize;
    debug_message("format is YVU 4:2:0 %ux%u", width, height);
    break;
  case V4L2_PIX_FMT_YUV420:
    m_u_offset = m_videoSrcYSize;
    m_v_offset = m_videoSrcYSize + m_videoSrcUVSize;
    debug_message("format is YUV 4:2:0 %ux%u", width, height);
    break;
  case V4L2_PIX_FMT_RGB24:
    debug_message("format is RGB24 %ux%u", width, height);
    break;
  case V4L2_PIX_FMT_BGR24:
    debug_message("format is BGR24 %ux%u", width, height);
    break;
  case V4L2_PIX_FMT_YUYV:
    debug_message("format is YUYV %ux%u", width, height);
    break;
  case V4L2_PIX_FMT_UYVY:
    debug_message("format is UYUV %ux%u", width, height);
    break;
  case V4L2_PIX_FMT_YYUV:
    debug_message("format is YYUV %ux%u", width, height);
    break;
  case V4L2_PIX_FMT_NV12:
    debug_message("format is NV12 %ux%u", width, height);
    break;
  }
  
  // allocate the desired number of buffers
  m_release_index_mask = 0;
  struct v4l2_requestbuffers reqbuf;
  memset(&reqbuf, 0, sizeof(reqbuf));
  reqbuf.count = MIN(m_pConfig->GetIntegerValue(CONFIG_VIDEO_CAP_BUFF_COUNT),
		     32);
  reqbuf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  reqbuf.memory = V4L2_MEMORY_MMAP;
  rc = ioctl(m_videoDevice, VIDIOC_REQBUFS, &reqbuf);
  if (rc < 0 || reqbuf.count < 1) {
    error_message("Failed to allocate buffers for %s %s", deviceName, 
		  strerror(errno));
    return false;
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
    memset(&buffer, 0, sizeof(buffer)); // cpn24
    buffer.index = ix;
    buffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buffer.memory = V4L2_MEMORY_MMAP; // cpn24

    rc = ioctl(m_videoDevice, VIDIOC_QUERYBUF, &buffer);
    if (rc < 0) {
      error_message("Failed to query video capture buffer status for %s, %s",
                    deviceName, strerror(errno));
      goto failure;
    }

    m_buffers[ix].length = buffer.length;
    m_buffers[ix].start = mmap(NULL, buffer.length,
                               PROT_READ | PROT_WRITE,
                               MAP_SHARED,
                               m_videoDevice, buffer.m.offset);

    if (m_buffers[ix].start == MAP_FAILED) {
      error_message("Failed to map video capture buffer for %s, %s", deviceName,
		    strerror(errno));
      goto failure;
    }

    //enqueue the mapped buffer
    rc = ioctl(m_videoDevice, VIDIOC_QBUF, &buffer);
    if (rc < 0) {
      error_message("Failed to enqueue video capture buffer status for %s, %s",
                    deviceName, strerror(errno));
      goto failure;
    }
  }

  SetPictureControls();

  if (capability.capabilities & (V4L2_CAP_AUDIO | V4L2_CAP_TUNER)) {
    SetVideoAudioMute(false);
  }

  // start video capture
  rc = ioctl(m_videoDevice, VIDIOC_STREAMON, &buftype);
  if (rc < 0) {
    error_message("Failed to start video capture for %s, %s", deviceName,
		  strerror(errno));
    perror("Reason");
    goto failure;
  }

#if 0
  // start video capture again
  rc = ioctl(m_videoDevice, VIDIOC_STREAMON, &buftype);
  if (rc < 0) {
    error_message("Failed to start video capture again for %s", deviceName);
    perror("Again Reason");
    goto failure;
  }
#endif

  return true;

 failure:
  if (m_buffers) {
    for (uint32_t i = 0; i < m_buffers_count; i++) {
      if (m_buffers[i].start)
	munmap(m_buffers[i].start, m_buffers[i].length);
    }
    free(m_buffers);
  }
  
  close(m_videoDevice);
  m_videoDevice = -1;
  return false;
}

void CV4LVideoSource::ReleaseDevice()
{
  SetVideoAudioMute(true);

  // stop video capture
  int buftype = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  int rc = ioctl(m_videoDevice, VIDIOC_STREAMOFF, &buftype);
  if (rc < 0) {
    error_message("Failed to stop video capture %s", strerror(errno));
  }


  close(m_videoDevice);
  m_videoDevice = -1;
}
bool CV4LVideoSource::ReleaseBuffers (void)
{
  if (m_buffers == NULL) return true;
  ReleaseFrames();
  // release device resources
  bool have_buffers = false;
  for (uint32_t i = 0; i < m_buffers_count; i++) {
    if (m_buffers[i].start) {
      if (m_buffers[i].in_use) {
	have_buffers = true;
	debug_message("buffer %u still in use", i);
      } else {
      munmap(m_buffers[i].start, m_buffers[i].length);
	m_buffers[i].start = NULL;
      }
    }
  }
  if (have_buffers) return false;
  debug_message("All buffers released");
  free(m_buffers);
  m_buffers = NULL;
  return true;
}
	
void CV4LVideoSource::SetVideoAudioMute(bool mute)
{
  if (!m_pConfig->m_videoCapabilities) return;

  CVideoCapabilities *vc = (CVideoCapabilities *)m_pConfig->m_videoCapabilities;
  if ( !vc->m_hasAudio) {
    return;
  }

  int rc;
  struct v4l2_queryctrl query;
  struct v4l2_control control;
  memset(&query, 0, sizeof(query));

  query.id = V4L2_CID_AUDIO_MUTE;
  rc = ioctl(m_videoDevice, VIDIOC_QUERYCTRL, &query);
  if (rc < 0) {
    if (errno != EINVAL) {
      error_message("Can't queryctrl control \"audio mute\" %s", 
		    strerror(errno));
    } else {
      error_message("Control \"audio mute\" not supported");
    }
    return;
  }

  control.id = V4L2_CID_AUDIO_MUTE;
  control.value = mute ? true : false;
  rc = ioctl(m_videoDevice, VIDIOC_S_CTRL, &control);
  if (rc < 0) {
    error_message("Couldn't set audio mute for %s, %s",
		  m_pConfig->m_videoCapabilities->m_deviceName, 
		  strerror(errno));
  }
}

void CV4LVideoSource::SetIndividualPictureControl (const char *type, 
						   uint32_t type_value,
						   uint32_t value)
{
  struct v4l2_queryctrl query;
  struct v4l2_control control;
  int rc;

  if (m_videoDevice == -1) return;

  memset(&query, 0, sizeof(query));

  query.id = type_value;
  rc = ioctl(m_videoDevice, VIDIOC_QUERYCTRL, &query);
  if (rc < 0) {
    if (errno != EINVAL) {
      error_message("Can't queryctrl control \"%s\" %s", type, 
		    strerror(errno));
    } else {
      error_message("Control \"%s\" not supported", type);
    }
    return;
  }
  memset(&control, 0, sizeof(control));
  control.id = type_value;

  int64_t range = query.maximum - query.minimum;
  value = MIN(value, 100);
  range *= value;
  range /= 100;
  control.value = query.minimum + (int32_t)range;
  debug_message("control %s min %d max %d val %u convert %d",
		type, query.minimum,
		query.maximum,
		value, 
		control.value);
		
  rc = ioctl(m_videoDevice, VIDIOC_S_CTRL, &control);
  if (rc < 0) {
    error_message("Can't set control \"%s\" %s", type, 
		  strerror(errno));
  }
}
  

bool CV4LVideoSource::SetPictureControls()
{
  if (m_videoDevice == -1) return false;

 /*
  //autoexposure
  control.id = V4L2_CID_AUTOGAIN;
  control.value = 0; 
      ((m_pConfig->GetIntegerValue(CONFIG_VIDEO_AUTOGAIN) * 0xFFFF) /100);// Changes to 1 to turn it on
  rc = ioctl(m_videoDevice, VIDIOC_S_CTRL, &control);

  //autobrightness
  control.id = V4L2_CID_AUTO_WHITE_BALANCE;
  control.value = 0;
      ((m_pConfig->GetIntegerValue(CONFIG_VIDEO_AUTO_WHITE_BALANCE) * 0xFFFF) /100);// Changes to 1 to turn it on
  rc = ioctl(m_videoDevice, VIDIOC_S_CTRL, &control);
  
  //Turn on flipvert
  control.id = V4L2_CID_VFLIP;
  control.value = 0;
      ((m_pConfig->GetIntegerValue(CONFIG_VIDEO_VFLIP) * 0xFFFF) /100);// Changes to 1 to turn it on
  rc = ioctl(m_videoDevice, VIDIOC_S_CTRL, &control);

*/
  // brightness
  SetIndividualPictureControl("brightness", 
			      V4L2_CID_BRIGHTNESS,
			      m_pConfig->GetIntegerValue(CONFIG_VIDEO_BRIGHTNESS));

  // hue
  SetIndividualPictureControl("hue", 
			      V4L2_CID_HUE,
			      m_pConfig->GetIntegerValue(CONFIG_VIDEO_HUE));
  SetIndividualPictureControl("saturation", 
			      V4L2_CID_SATURATION,
			      m_pConfig->GetIntegerValue(CONFIG_VIDEO_COLOR));
  SetIndividualPictureControl("contrast", 
			      V4L2_CID_CONTRAST,
			      m_pConfig->GetIntegerValue(CONFIG_VIDEO_CONTRAST));
  return true;
}

int8_t CV4LVideoSource::AcquireFrame(Timestamp &frameTimestamp)
{
  struct v4l2_buffer buffer;

  ReleaseFrames();
  // if we're still waiting to release frames, there's no need
  // to get another error

  buffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  buffer.memory = V4L2_MEMORY_MMAP;

  int rc = ioctl(m_videoDevice, VIDIOC_DQBUF, &buffer);
  if (rc != 0) {
    error_message("error %d errno %d %s", rc, errno, strerror(errno));
    m_waiting_frames_return = true;
    return -1;
  }
  //  debug_message("acq %d", buffer.index);
  frameTimestamp = GetTimestampFromTimeval(&(buffer.timestamp));
  return buffer.index;
}

void c_ReleaseFrame (void *f)
{
  yuv_media_frame_t *yuv = (yuv_media_frame_t *)f;
  if (yuv->free_y) {
    CHECK_AND_FREE(yuv->y);
  } else {
    CV4LVideoSource *s = (CV4LVideoSource *)yuv->hardware;
    if (s->IsHardwareVersion(yuv->hardware_version)) {
      s->IndicateReleaseFrame(yuv->hardware_index);
    } else {
      // need to free the buffer
      //munmap(yuv->y, )
      // hit the semaphore...
      s->ReleaseOldFrame(yuv->hardware_version, yuv->hardware_index);
    }
  }
  free(yuv);
}

void CV4LVideoSource::ReleaseOldFrame (uint hardware_version, 
 				       uint8_t index)
{
  SDL_LockMutex(m_v4l_mutex);
  unreleased_capture_buffers_t *p = m_unreleased_buffers_list, *q = NULL;
  while (p != NULL && p->hardware_version != hardware_version) {
    q = p; 
    p = p->next;
  }
  if (p != NULL) {
    debug_message("release version %u index %u", 
 		  hardware_version, index);
    p->buffer_list[index].in_use = false;
    munmap(p->buffer_list[index].start, p->buffer_list[index].length);
    p->buffer_list[index].start = NULL;
    bool have_one = false;
    for (uint32_t ix = 0; ix < m_buffers_count && have_one == false; ix++) {
      have_one = p->buffer_list[index].start != NULL;
    }
    if (have_one) {
      q->next = p->next;
      free(p->buffer_list);
      free(p);
    }
  } else
    debug_message("couldn't release version %u index %u", 
 		  hardware_version, index);
  SDL_UnlockMutex(m_v4l_mutex);
}
 

/*
 * Use the m_release_index_mask to indicate which indexes need to
 * be released - note - there may be a problem if we don't release
 * in order given.
 */
void CV4LVideoSource::ReleaseFrames (void)
{
  uint8_t index = 0;
  uint32_t index_mask = 1;
  uint32_t released_mask;
  int rc;

  SDL_LockMutex(m_v4l_mutex);
  released_mask = m_release_index_mask;
  m_release_index_mask = 0;
  SDL_UnlockMutex(m_v4l_mutex);

  while (released_mask != 0 && index < 32) {
    if ((index_mask & released_mask) != 0) {
      if (m_waiting_frames_return) {
	m_waiting_frames_return = false;
	debug_message("frame return");
      }
      struct v4l2_buffer buffer;
      m_buffers[index].in_use = false;
      if (m_source) {
	buffer.index = index;
	buffer.memory = V4L2_MEMORY_MMAP;
	buffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	buffer.flags = 0;

	// it appears that some cards, some drivers require a QUERYBUF
	// before the QBUF.  This code is designed to do so, but only if
	// we need to
	if (m_use_alternate_release) {
	  rc = ioctl(m_videoDevice, VIDIOC_QUERYBUF, &buffer);
	  if (rc < 0) {
	    error_message("Failed to query video capture buffer status %s", strerror(errno));
	  }
	}
	rc = ioctl(m_videoDevice, VIDIOC_QBUF, &buffer);
	if (rc < 0) {
	  if (m_use_alternate_release) {
	    error_message("Could not enqueue buffer to video capture queue");
	  } else {
	    rc = ioctl(m_videoDevice, VIDIOC_QUERYBUF, &buffer);
	    if (rc < 0) {
	      error_message("Failed to query video capture buffer status %s", 
			    strerror(errno));
	    }
	    rc = ioctl(m_videoDevice, VIDIOC_QBUF, &buffer);
	    if (rc < 0) {
	      error_message("Failed to query video capture buffer status %s", 
			    strerror(errno));
	    } else {
	      m_use_alternate_release = true;
	    }
	  }
	}
	//      debug_message("rel %d", index);
      } 
    }
    index_mask <<= 1;
    index++;
  }
}

void CV4LVideoSource::ProcessVideo(void)
{
  // for efficiency, process ~1 second before returning to check for commands
  Timestamp frameTimestamp;
  for (int pass = 0; pass < m_maxPasses && m_stop_thread == false; pass++) {

    // dequeue next frame from video capture buffer
    int index = AcquireFrame(frameTimestamp);
    if (index == -1) {
      return;
    }
    m_buffers[index].in_use = true;
    //debug_message("buffer %u in use", index);

#ifdef CAPTURE_RAW
    fwrite(m_buffers[index].start, m_videoSrcYUVSize, 1, m_rawfile);
#endif

    u_int8_t* mallocedYuvImage = NULL;
    u_int8_t* pY;
    u_int8_t* pU;
    u_int8_t* pV;

    // perform colorspace conversion if necessary
    switch (m_format) {
    case V4L2_PIX_FMT_RGB24:
    case V4L2_PIX_FMT_BGR24:
      mallocedYuvImage = (u_int8_t*)Malloc(m_videoSrcYUVSize);
      debug_message("converting to YUV420P from RGB");
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
                1, 
		m_format == V4L2_PIX_FMT_RGB24);
      break;
    case V4L2_PIX_FMT_YUYV: 
      mallocedYuvImage = (u_int8_t*)Malloc(m_videoSrcYUVSize);
      //debug_message("converting to YUV420P from YUYV");
      pY = mallocedYuvImage;
      pU = pY + m_videoSrcYSize;
      pV = pU + m_videoSrcUVSize;
      convert_yuyv_to_yuv420p(pY, 
			      (const uint8_t *)m_buffers[index].start,
			      m_videoSrcWidth,
			      m_videoSrcHeight);
      break;
    case V4L2_PIX_FMT_UYVY:
      mallocedYuvImage = (u_int8_t*)Malloc(m_videoSrcYUVSize);
      //debug_message("converting to YUV420P from YUYV");
      pY = mallocedYuvImage;
      pU = pY + m_videoSrcYSize;
      pV = pU + m_videoSrcUVSize;
      convert_uyvy_to_yuv420p(pY,
			      (const uint8_t *)m_buffers[index].start,
			      m_videoSrcWidth,
			      m_videoSrcHeight);
      break;
    case V4L2_PIX_FMT_YYUV:
      mallocedYuvImage = (u_int8_t*)Malloc(m_videoSrcYUVSize);
      //debug_message("converting to YUV420P from YUYV");
      pY = mallocedYuvImage;
      pU = pY + m_videoSrcYSize;
      pV = pU + m_videoSrcUVSize;
      convert_yyuv_to_yuv420p(pY,
			      (const uint8_t *)m_buffers[index].start,
			      m_videoSrcWidth,
			      m_videoSrcHeight);
      break;
    case V4L2_PIX_FMT_NV12:
      mallocedYuvImage = (u_int8_t*)Malloc(m_videoSrcYUVSize);
      //debug_message("converting to YUV420P from YUYV");
      pY = mallocedYuvImage;
      pU = pY + m_videoSrcYSize;
      pV = pU + m_videoSrcUVSize;
      convert_nv12_to_yuv420p(pY,
			      (const uint8_t *)m_buffers[index].start,
			      m_videoSrcYSize,
			      m_videoSrcWidth,
			      m_videoSrcHeight);
      break;
    default:
#if 0
      // we would need the below if we were going to switch 
      // video - this is a problem with the 
      mallocedYuvImage = (u_int8_t*)Malloc(m_videoSrcYUVSize);
      pY = (u_int8_t*)mallocedYuvImage;
      memcpy(pY, m_buffers[index].start, m_videoSrcYUVSize);
#else
      pY = (u_int8_t*)m_buffers[index].start;
#endif
      pU = pY + m_u_offset;
      pV = pY + m_v_offset;
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
    yuv->hardware_version = m_hardware_version;
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
  static const char* devices[] = {
    "/dev/video", 
    "/dev/video0", 
    "/dev/video1", 
    "/dev/video2", 
    "/dev/video3"
  };
  const char* deviceName = pConfig->GetStringValue(CONFIG_VIDEO_SOURCE_NAME);
  CVideoCapabilities* pVideoCaps;

  pVideoCaps = FindVideoCapabilitiesByDevice(deviceName);
  if (pVideoCaps != NULL && pVideoCaps->IsValid()) {
    pConfig->m_videoCapabilities = pVideoCaps;
    return true;
  }
  // first try the device we're configured with
  pVideoCaps = new CVideoCapabilities(deviceName);

  if (pVideoCaps->IsValid()) {
    pConfig->m_videoCapabilities = pVideoCaps;
    pVideoCaps->SetNext(VideoCapabilities);
    VideoCapabilities = pVideoCaps;
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
      pVideoCaps->SetNext(VideoCapabilities);
      VideoCapabilities = pVideoCaps;
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
  memset(&capability, 0, sizeof(capability));
  rc = ioctl(videoDevice, VIDIOC_QUERYCAP, &capability);
  if (rc < 0) {
    error_message("Failed to query video capabilities for %s %s", m_deviceName, 
		  strerror(errno));
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
  m_driverName = strdup((char*)capability.driver);
  m_hasAudio = capability.capabilities & (V4L2_CAP_AUDIO | V4L2_CAP_TUNER);

  struct v4l2_input input;
  memset(&input, 0, sizeof(input));

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
    if (rc >= 0) {
      m_inputNames[i] = strdup((char*)input.name);
    } else {
      m_inputNames[i] = strdup("Unknown input");
      error_message("Failed to enumerate video input %d for %s, %s",
                    i, m_deviceName, strerror(errno));
      continue;
    }

    debug_message("type %d %s type %x", i, m_inputNames[i], input.type);
    if (input.type == V4L2_INPUT_TYPE_TUNER) {
      debug_message("Has tuner");
      m_inputHasTuners[i] = true;

      if (input.std & V4L2_STD_PAL) m_inputTunerSignalTypes[i] |= 0x1;
      if (input.std & V4L2_STD_NTSC) m_inputTunerSignalTypes[i] |= 0x2;
      if (input.std & V4L2_STD_SECAM) m_inputTunerSignalTypes[i] |= 0x4;
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

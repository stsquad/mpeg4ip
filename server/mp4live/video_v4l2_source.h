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

#ifndef __VIDEO_V4L2_SOURCE_H__
#define __VIDEO_V4L2_SOURCE_H__

#include <sys/types.h>
#include <sys/ioctl.h>
#include <linux/videodev.h>
#include "media_source.h"
#include "video_encoder.h"

class CV4L2VideoSource : public CMediaSource {
 public:
  CV4L2VideoSource() : CMediaSource() {
    m_videoDevice = -1;
    m_buffers = NULL;
    m_decimate_filter = false;
    m_use_alternate_release = false;
  }

  static bool InitialVideoProbe(CLiveConfig* pConfig);

  bool IsDone() {
    return false;	// live capture is inexhaustible
  }

  float GetProgress() {
    return 0.0;		// live capture device is inexhaustible
  }

  bool SetPictureControls();

  void IndicateReleaseFrame(uint8_t index) {
    // indicate which index needs to be released before next acquire
    SDL_LockMutex(m_v4l_mutex);
    m_release_index_mask |= (1 << index);
    SDL_UnlockMutex(m_v4l_mutex);
    if (m_waiting_frames_return)
      SDL_SemPost(m_myMsgQueueSemaphore);
  };

 protected:
  int ThreadMain(void);
  void DoStartCapture(void);
  void DoStopCapture(void);
  bool Init(void);
  bool InitDevice(void);
  void ReleaseDevice(void);
  void ProcessVideo(void);
  int8_t AcquireFrame(Timestamp &frameTimestamp);
  void ReleaseFrames(void);
  void SetVideoAudioMute(bool mute);

  u_int8_t m_maxPasses;
  int m_videoDevice;

  typedef struct {
    void* start;
    __u32 length;
  } capture_buffer_t;

  capture_buffer_t* m_buffers;
  uint32_t m_buffers_count;
  
  Timestamp m_videoCaptureStartTimestamp;
  float m_videoSrcFrameRate;
  bool m_decimate_filter;
  SDL_mutex *m_v4l_mutex;
  uint32_t m_release_index_mask;
  bool m_use_alternate_release;
  bool m_waiting_frames_return;
};



class CVideoCapabilities : public CCapabilities {
 public:
  CVideoCapabilities(const char* deviceName) :
    CCapabilities(deviceName) {
    m_canCapture = false;
    m_driverName = NULL;
    m_numInputs = 0;
    m_inputNames = NULL;
    //m_inputSignalTypes = NULL;
    m_inputHasTuners = NULL;
    m_inputTunerSignalTypes = NULL;
    m_hasAudio = false;

    ProbeDevice();
  }

  ~CVideoCapabilities() {
    free(m_driverName);
    for (int i = 0; i < m_numInputs; i++) {
      free(m_inputNames[i]);
    }
    free(m_inputNames);
    //free(m_inputSignalTypes);
    free(m_inputHasTuners);
    free(m_inputTunerSignalTypes);
  }

  bool IsValid() {
    return m_canOpen && m_canCapture;
  }
  void Display(CLiveConfig *pConfig, 
	       char *msg, 
	       uint32_t max_len);
 public:
  bool		m_canOpen;
  bool		m_canCapture;

  // N.B. the rest of the fields are only valid 
  // if m_canOpen and m_canCapture are both true

  char*		m_driverName; 
  u_int16_t	m_numInputs;

  //TODO: why seg fault if I comment these ?
  u_int16_t	m_minWidth;
  u_int16_t	m_minHeight;
  u_int16_t	m_maxWidth;
  u_int16_t	m_maxHeight;

  // each of these points at m_numInputs values
  char**	m_inputNames;
  bool*		m_inputHasTuners;
  u_int8_t*	m_inputTunerSignalTypes;	// possible signal types from tuner

  bool		m_hasAudio;
 protected:
  bool ProbeDevice(void);
};

#endif /* __VIDEO_V4L2_SOURCE_H__ */

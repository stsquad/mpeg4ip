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

#ifndef __AUDIO_OSS_SOURCE_H__
#define __AUDIO_OSS_SOURCE_H__

#include <sys/types.h>
#include <sys/ioctl.h>
#include <linux/soundcard.h>

#include "media_source.h"
//#include "audio_encoder.h"

class COSSAudioSource : public CMediaSource {
 public:
  COSSAudioSource(CLiveConfig *pConfig);

  ~COSSAudioSource() {
  }

  bool IsDone() {
    return false;	// live capture device is inexhaustible
  }

  float GetProgress() {
    return 0.0;		// live capture device is inexhaustible
  }

  virtual const char* name() {
    return "COSSAudioSource";
  }

 protected:
  int ThreadMain();

  void DoStartCapture();
  void DoStopCapture();

  bool Init();
  bool InitDevice();

  void ProcessAudio();

 protected:
  int           m_audioDevice;
  int           m_maxPasses;
  Timestamp     m_prevTimestamp;
  int           m_audioOssMaxBufferSize;
  int           m_audioOssMaxBufferFrames;
  Timestamp     *m_timestampOverflowArray;
  size_t        m_timestampOverflowArrayIndex;
  u_int32_t     m_pcmFrameSize;
  uint32_t      m_channelsConfigured;
};


class CAudioCapabilities : public CCapabilities {
 public:
  CAudioCapabilities(const char* deviceName) : 
    CCapabilities(deviceName) {
    m_numSamplingRates = 0;

    ProbeDevice();
  }

  ~CAudioCapabilities() {
  }

  void Display(CLiveConfig *pConfig, 
	       char *msg, 
	       uint32_t max_len);
 public:
  // N.B. the rest of the fields are only valid 
  // if m_canOpen is true

  char*		m_driverName; 

  u_int16_t	m_numSamplingRates;
  u_int32_t	m_samplingRates[16];
  u_int32_t CheckSampleRate(u_int32_t rate) {
    int32_t diff = 0x7fffffff;
    u_int32_t ret_rate = m_samplingRates[0];
    for (u_int16_t ix = 0; ix < m_numSamplingRates; ix++) {
      if (rate == m_samplingRates[ix]) {
	return rate;
      }
      int32_t calc;
      calc = abs(m_samplingRates[ix] - rate);
      if (calc < diff) {
	diff = calc;
	ret_rate = m_samplingRates[ix];
      }
    }
    return ret_rate;
  };
      
 protected:
  bool ProbeDevice(void);
};

#endif /* __AUDIO_OSS_SOURCE_H__ */

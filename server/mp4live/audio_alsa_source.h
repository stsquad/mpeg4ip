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
 *		Sverker Abrahamsson	sverker@abrahamsson.com
 */

#ifndef __AUDIO_ALSA_SOURCE_H__
#define __AUDIO_ALSA_SOURCE_H__

#ifdef HAVE_ALSA
#include <alsa/asoundlib.h>

#include "media_source.h"
#include "audio_oss_source.h"
//#include "audio_encoder.h"

class CALSAAudioSource : public CMediaSource {
 public:
  CALSAAudioSource(CLiveConfig *pConfig);

  ~CALSAAudioSource() {
  }

  bool IsDone() {
    return false;	// live capture device is inexhaustible
  }

  float GetProgress() {
    return 0.0;		// live capture device is inexhaustible
  }

  virtual const char* name() {
    return "CALSAAudioSource";
  }


 protected:
  int ThreadMain();

  void DoStartCapture();
  void DoStopCapture();

  bool Init();
  bool InitDevice();

  void ProcessAudio();

 protected:
  /* Handle for the PCM device */ 
  snd_pcm_t     *m_pcmHandle;
  int           m_maxPasses;
  Timestamp     m_prevTimestamp;
  int           m_audioMaxBufferSize;
  int           m_audioMaxBufferFrames;
  Timestamp     *m_timestampOverflowArray;
  size_t        m_timestampOverflowArrayIndex;
  u_int32_t     m_pcmFrameSize;
  uint32_t      m_channelsConfigured;
};


class CALSAAudioCapabilities : public CAudioCapabilities {
 public:
  CALSAAudioCapabilities(const char* deviceName) : 
    CAudioCapabilities(deviceName) {
    m_numSamplingRates = 0;

    ProbeDevice();
  }

  ~CALSAAudioCapabilities() {
  }

  void Display(CLiveConfig *pConfig, 
	       char *msg, 
	       uint32_t max_len);

 protected:
  bool ProbeDevice(void);
};

#endif
#endif /* __AUDIO_ALSA_SOURCE_H__ */

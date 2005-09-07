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
 * Copyright (C) Cisco Systems Inc. 2000-2002.  All Rights Reserved.
 * 
 * Contributor(s): 
 *		Dave Mackie		dmackie@cisco.com
 *		Bill May 		wmay@cisco.com
 */

#include "mp4live.h"
#include "audio_oss_source.h"
#include "audio_encoder.h"
//#define DEBUG_TIMESTAMPS 1

COSSAudioSource::COSSAudioSource(CLiveConfig *pConfig) : CMediaSource() 
{
  SetConfig(pConfig);

  m_audioDevice = -1;
  m_prevTimestamp = 0;
  m_timestampOverflowArray = NULL;
  m_timestampOverflowArrayIndex = 0;
  m_audioOssMaxBufferSize = 0;

  // NOTE: This used to be CAVMediaFlow::SetAudioInput();
  // if mixer is specified, then user takes responsibility for
  // configuring mixer to set the appropriate input sources
  // this allows multiple inputs to be used, for example

  if (!strcasecmp(m_pConfig->GetStringValue(CONFIG_AUDIO_INPUT_NAME),
		  "mix")) {
    return;
  }

  // else set the mixer input source to the one specified

  static char* inputNames[] = SOUND_DEVICE_NAMES;

  const char* mixerName = 
    m_pConfig->GetStringValue(CONFIG_AUDIO_MIXER_NAME);

  int mixer = open(mixerName, O_RDONLY);

  if (mixer < 0) {
    error_message("Couldn't open mixer %s", mixerName);
    return;
  }

  u_int8_t i;
  int recmask = 0;

  for (i = 0; i < sizeof(inputNames) / sizeof(char*); i++) {
    if (!strcasecmp(m_pConfig->GetStringValue(CONFIG_AUDIO_INPUT_NAME),
		    inputNames[i])) {
      recmask |= (1 << i);
      ioctl(mixer, SOUND_MIXER_WRITE_RECSRC, &recmask);
      break;
    }
  }

  close(mixer);
}

int COSSAudioSource::ThreadMain(void) 
{
  debug_message("oss start");
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
	  debug_message("oss stop thread");
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
	//debug_message("processaudio");
        ProcessAudio();
      }
      catch (...) {
	error_message("oss stop capture");
        DoStopCapture();	
        break;
      }
    }
  }

  debug_message("oss thread exit");
  return -1;
}

void COSSAudioSource::DoStartCapture()
{
  if (m_source) {
    return;
  }

  if (!Init()) {
    return;
  }

  m_source = true;
}

void COSSAudioSource::DoStopCapture()
{
  if (!m_source) {
    return;
  }

  //  CMediaSource::DoStopAudio();

  close(m_audioDevice);
  m_audioDevice = -1;

  CHECK_AND_FREE(m_timestampOverflowArray);

  m_source = false;
}

bool COSSAudioSource::Init(void)
{
  bool rc = InitAudio(true);

  if (!rc) {
    return false;
  }

  if (!InitDevice()) {
    return false;
  }

  //#error we will have to remove this below - the sample rate will be
  rc = SetAudioSrc(
                   PCMAUDIOFRAME,
                   m_channelsConfigured,
                   m_pConfig->GetIntegerValue(CONFIG_AUDIO_SAMPLE_RATE));

  if (!rc) {
    return false;
  }

  // for live capture we can match the source to the destination
  //  m_audioSrcSamplesPerFrame = m_audioDstSamplesPerFrame;
  // gets set 
  m_pcmFrameSize = 
    m_audioSrcSamplesPerFrame * m_audioSrcChannels * sizeof(u_int16_t);
  debug_message("Audio input size is %u", m_pcmFrameSize);

  if (m_audioOssMaxBufferSize > 0) {
    size_t array_size;
    m_audioOssMaxBufferFrames = m_audioOssMaxBufferSize / m_pcmFrameSize;
    if (m_audioOssMaxBufferFrames == 0) {
      m_audioOssMaxBufferFrames = 1;
    }
    array_size = m_audioOssMaxBufferFrames * sizeof(*m_timestampOverflowArray);
    m_timestampOverflowArray = (Timestamp *)Malloc(array_size);
    memset(m_timestampOverflowArray, 0, array_size);
  }
    
  // maximum number of passes in ProcessAudio, approx 1 sec.
  m_maxPasses = m_audioSrcSampleRate / m_audioSrcSamplesPerFrame;

  return true;

#if 0
 init_failure:
  debug_message("audio initialization failed");

  close(m_audioDevice);
  m_audioDevice = -1;
  return false;
#endif
}

bool COSSAudioSource::InitDevice(void)
{
  int rc;
  const char* deviceName = m_pConfig->GetStringValue(CONFIG_AUDIO_SOURCE_NAME);

  // open the audio device
  m_audioDevice = open(deviceName, O_RDONLY);
  if (m_audioDevice < 0) {
    error_message("Failed to open %s", deviceName);
    return false;
  }

  int enablebits;
  // Disable the audio input until we can start it below
  ioctl(m_audioDevice, SNDCTL_DSP_GETTRIGGER, &enablebits);
  enablebits &= ~PCM_ENABLE_INPUT;
  ioctl(m_audioDevice, SNDCTL_DSP_SETTRIGGER, &enablebits);
#ifdef WORDS_BIGENDIAN
#define OUR_FORMAT AFMT_S16_BE
#else
#define OUR_FORMAT AFMT_S16_LE
#endif
  int format = OUR_FORMAT;

  rc = ioctl(m_audioDevice, SNDCTL_DSP_SETFMT, &format);
  if (rc < 0 || format != OUR_FORMAT) {
    error_message("Couldn't set format for %s", deviceName);
    close(m_audioDevice);
    return false;
  }

  m_channelsConfigured = m_pConfig->GetIntegerValue(CONFIG_AUDIO_CHANNELS);
  rc = ioctl(m_audioDevice, SNDCTL_DSP_CHANNELS, &m_channelsConfigured);
  if (rc < 0) {
    error_message("Couldn't set audio channels for %s", deviceName);
    close(m_audioDevice);
    return false;
  }
  if (m_channelsConfigured != m_pConfig->GetIntegerValue(CONFIG_AUDIO_CHANNELS)) {
    error_message("Channels not set to configured driver says %d - configured %d", 
		  m_channelsConfigured, 
		  m_pConfig->GetIntegerValue(CONFIG_AUDIO_CHANNELS));
  }

  u_int32_t samplingRate = 
    m_pConfig->GetIntegerValue(CONFIG_AUDIO_SAMPLE_RATE);
  u_int32_t targetSamplingRate = samplingRate;

  rc = ioctl(m_audioDevice, SNDCTL_DSP_SPEED, &samplingRate);

  if (rc < 0 || abs(samplingRate - targetSamplingRate) > 1) {
    error_message("Couldn't set sampling rate for %s", deviceName);
    close(m_audioDevice);
    return false;
  }

  if (m_pConfig->GetBoolValue(CONFIG_AUDIO_OSS_USE_SMALL_FRAGS)) {
    // from rca.  set OSS audio fragment size to a small value.
    // value is 0xMMMMSSSS where MMMM is number of buffers 
    // (configuration value CONFIG_AUDIO_OSS_FRAGMENTS) and SSSS is
    // power of 2 sized fragment value
    // 
    // Reason for this is OSS will block on read until a complete
    // fragment is read - even if correct number of bytes is in buffers
    debug_message("Using small frags");
    int fragSize = m_pConfig->GetIntegerValue(CONFIG_AUDIO_OSS_FRAG_SIZE);
    int bufcfg = m_pConfig->GetIntegerValue(CONFIG_AUDIO_OSS_FRAGMENTS);

    bufcfg <<= 16; // shift over
    bufcfg |= fragSize;

    rc = ioctl(m_audioDevice, SNDCTL_DSP_SETFRAGMENT, &bufcfg);
    if (rc) {
      error_message("Error - could not set OSS Input fragment size %d", rc);
      close(m_audioDevice);
      return false;
    }
  }

  audio_buf_info info;
  rc = ioctl(m_audioDevice, SNDCTL_DSP_GETISPACE, &info);
  if (rc < 0) {
    error_message("Failed to query OSS GETISPACE");
    error_message("This means you will not get accurate audio timestamps");
    error_message("Please think about updating your audio card or driver");
    m_audioOssMaxBufferSize = 0;
    return true;
  }
  
#ifdef DEBUG_TIMESTAMPS
  debug_message("fragstotal = %d", info.fragstotal);
  debug_message("fragsize = %d", info.fragsize);
#endif

  m_audioOssMaxBufferSize = info.fragstotal * info.fragsize;
  debug_message("oss init done");
  return true;
}

void COSSAudioSource::ProcessAudio(void)
{
#ifdef SNDCTL_DSP_GETERROR
  audio_errinfo errinfo;
  if (m_audioSrcFrameNumber == 0) {
    ioctl(m_audioDevice, SNDCTL_DSP_GETERROR, &errinfo);
  } else {
    ioctl(m_audioDevice, SNDCTL_DSP_GETERROR, &errinfo);
    if (errinfo.rec_overruns > 0) {
      debug_message("overrun error found in audio - adding "U64" samples",
		    SrcBytesToSamples(errinfo.rec_ptradjust));
      close(m_audioDevice);
      InitDevice();
      m_audioSrcSampleNumber = 0;
    }
  }
#endif

  if (m_audioSrcFrameNumber == 0) {
    // Pull the trigger and start the audio input
    int enablebits;
    ioctl(m_audioDevice, SNDCTL_DSP_GETTRIGGER, &enablebits);
    enablebits |= PCM_ENABLE_INPUT;
    ioctl(m_audioDevice, SNDCTL_DSP_SETTRIGGER, &enablebits);
    //debug_message("oss input enable");
  }

  // for efficiency, process 1 second before returning to check for commands
  for (int pass = 0; pass < m_maxPasses && m_stop_thread == false; pass++) {

    audio_buf_info info;
    int rc = ioctl(m_audioDevice, SNDCTL_DSP_GETISPACE, &info);
    Timestamp currentTime = GetTimestamp();

    if (rc<0) {
      error_message("Failed to query OSS GETISPACE");
      info.bytes = 0;
    }
    //debug_message("reading %u", m_pcmFrameSize);
    u_int8_t*     pcmFrameBuffer;
    pcmFrameBuffer = (u_int8_t*)malloc(m_pcmFrameSize);

    uint32_t bytesRead = read(m_audioDevice, pcmFrameBuffer, m_pcmFrameSize);
    if (bytesRead < m_pcmFrameSize) {
      debug_message("bad audio read");
      free(pcmFrameBuffer);
      continue;
    }
    //debug_message("oss read");
    Timestamp timestamp;

    if (info.bytes == m_audioOssMaxBufferSize) {
      // means the audio buffer is full, and not capturing
      // we want to make the timestamp based on the previous one
      // When we hit this case, we start using the m_timestampOverflowArray
      // This will give us a timestamp for when the array is full.
      // 
      // In other words, if we have a full audio buffer (ie: it's not loading
      // any more), we start storing the current timestamp into the array.
      // This will let us "catch up", and have a somewhat accurate timestamp
      // when we loop around
      // 
      // wmay - I'm not convinced that this actually works - if the buffer
      // cleans up, we'll ignore m_timestampOverflowArray
      if (m_timestampOverflowArray != NULL && 
	  m_timestampOverflowArray[m_timestampOverflowArrayIndex] != 0) {
	timestamp = m_timestampOverflowArray[m_timestampOverflowArrayIndex];
      } else {
	timestamp = m_prevTimestamp + SrcSamplesToTicks(m_audioSrcSamplesPerFrame);
      }

      if (m_timestampOverflowArray != NULL)
	m_timestampOverflowArray[m_timestampOverflowArrayIndex] = currentTime;

      debug_message("audio buffer full !");

    } else {
      // buffer is not full - so, we make the timestamp based on the number
      // of bytes in the buffer that we read.
      timestamp = currentTime - SrcSamplesToTicks(SrcBytesToSamples(info.bytes));
      if (m_timestampOverflowArray != NULL)
	m_timestampOverflowArray[m_timestampOverflowArrayIndex] = 0;
    }

#ifdef DEBUG_TIMESTAMPS
    debug_message("info.bytes=%d t="U64" timestamp="U64" delta="U64,
                  info.bytes, currentTime, timestamp, timestamp - m_prevTimestamp);
#endif

    m_prevTimestamp = timestamp;
    if (m_timestampOverflowArray != NULL) {
      m_timestampOverflowArrayIndex = (m_timestampOverflowArrayIndex + 1) % 
	m_audioOssMaxBufferFrames;
    }
#ifdef DEBUG_TIMESTAMPS
    debug_message("pcm forward "U64" %u", timestamp, m_pcmFrameSize);
#endif
    if (m_audioSrcFrameNumber == 0 && m_videoSource != NULL) {
      m_videoSource->RequestKeyFrame(timestamp);
    }
    m_audioSrcFrameNumber++;
    CMediaFrame *frame = new CMediaFrame(PCMAUDIOFRAME,
					 pcmFrameBuffer, 
					 m_pcmFrameSize, 
					 timestamp);
    ForwardFrame(frame);
  }
}

bool CAudioCapabilities::ProbeDevice()
{
  int rc;

  if (allSampleRateTableSize > NUM_ELEMENTS_IN_ARRAY(m_samplingRates)) {
    error_message("Number of sample rates exceeds audio cap array");
    return false;
  }
  // open the audio device
  int audioDevice = open(m_deviceName, O_RDONLY);
  if (audioDevice < 0) {
    return false;
  }
  m_canOpen = true;

  // union of valid sampling rates for MP3 and AAC

  // for all possible sampling rates
  u_int8_t i;
  for (i = 0; i < allSampleRateTableSize; i++) {
    u_int32_t targetRate = allSampleRateTable[i];
    u_int32_t samplingRate = targetRate;

    // attempt to set sound card to this sampling rate
    rc = ioctl(audioDevice, SNDCTL_DSP_SPEED, &samplingRate);

    // invalid sampling rate, allow deviation of 1 sample/sec
    if (rc < 0 || abs(samplingRate - targetRate) > 1) {
      debug_message("audio device %s doesn't support sampling rate %u",
                    m_deviceName, targetRate);
      continue;
    }

    // valid sampling rate
    m_samplingRates[m_numSamplingRates++] = targetRate;
  }

  // zero out remaining sampling rate entries
  for (i = m_numSamplingRates; i < allSampleRateTableSize; i++) {
    m_samplingRates[i] = 0;
  }

  close(audioDevice);

  return true;
}

void CAudioCapabilities::Display (CLiveConfig *pConfig, 
				  char *msg, 
				  uint32_t max_len)
{
  // might want to parse out CONFIG_AUDIO_INPUT_NAME a bit to make it
  // more English like...
  snprintf(msg, max_len,
	   "%s, %s, %s, %u Hz, %s", 
	   pConfig->GetStringValue(CONFIG_AUDIO_SOURCE_TYPE),
	   pConfig->GetStringValue(CONFIG_AUDIO_SOURCE_NAME),
	   pConfig->GetStringValue(CONFIG_AUDIO_INPUT_NAME),
	   pConfig->GetIntegerValue(CONFIG_AUDIO_SAMPLE_RATE),
	   pConfig->GetIntegerValue(CONFIG_AUDIO_CHANNELS) == 1 ? 
	   "mono" : "stereo");
}

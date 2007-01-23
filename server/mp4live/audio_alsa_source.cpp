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
 *		Sverker Abrahamsson	sverker@abrahamsson.com
 */

#include "mp4live.h"
#include "audio_alsa_source.h"
#include "audio_encoder.h"
//#define DEBUG_TIMESTAMPS 1

#ifdef HAVE_ALSA

CALSAAudioSource::CALSAAudioSource(CLiveConfig *pConfig) : CMediaSource() 
{
  SetConfig(pConfig);

  m_prevTimestamp = 0;
  m_timestampOverflowArray = NULL;
  m_timestampOverflowArrayIndex = 0;
  m_audioMaxBufferSize = 0;
}

int CALSAAudioSource::ThreadMain(void) 
{
  debug_message("alsa start");
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
          debug_message("alsa stop thread");
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
        error_message("alsa stop capture");
        DoStopCapture();	
        break;
      }
    }
  }

  debug_message("alsa thread exit");
  return -1;
}

void CALSAAudioSource::DoStartCapture()
{
  if (m_source) {
    return;
  }

  if (!Init()) {
    return;
  }

  m_source = true;
}

void CALSAAudioSource::DoStopCapture()
{
  if (!m_source) {
    return;
  }

  //  CMediaSource::DoStopAudio();

  snd_pcm_drop(m_pcmHandle);
  snd_pcm_close(m_pcmHandle);
  CHECK_AND_FREE(m_timestampOverflowArray);
  m_source = false;
}

bool CALSAAudioSource::Init(void)
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
  m_pcmFrameSize = m_audioSrcSamplesPerFrame * m_audioSrcChannels * sizeof(u_int16_t);

  if (m_audioMaxBufferSize > 0) {
    size_t array_size;
    m_audioMaxBufferFrames = m_audioMaxBufferSize / m_pcmFrameSize;
    if (m_audioMaxBufferFrames == 0) {
      m_audioMaxBufferFrames = 1;
    }
    array_size = m_audioMaxBufferFrames * sizeof(*m_timestampOverflowArray);
    m_timestampOverflowArray = (Timestamp *)Malloc(array_size);
    memset(m_timestampOverflowArray, 0, array_size);
  }
    
  // maximum number of passes in ProcessAudio, approx 1 sec.
  m_maxPasses = m_audioSrcSampleRate / m_audioSrcSamplesPerFrame;

  return true;

#if 0
 init_failure:
  debug_message("audio initialization failed");

  snd_pcm_close(m_pcmHandle);
  m_pcmHandle = -1;
  return false;
#endif
}

bool CALSAAudioSource::InitDevice(void)
{
	int err;
  // Capture stream
  snd_pcm_stream_t stream = SND_PCM_STREAM_CAPTURE;

  // This structure contains information about
  // the hardware and can be used to specify the
  // configuration to be used for the PCM stream.
  snd_pcm_hw_params_t *hwparams;

  // Name of the PCM device, like plughw:0,0
  const char *pcm_name = m_pConfig->GetStringValue(CONFIG_AUDIO_SOURCE_NAME);

  // Allocate the snd_pcm_hw_params_t structure on the stack.
  snd_pcm_hw_params_alloca(&hwparams);

  // Open PCM
  if ((err = snd_pcm_open(&m_pcmHandle, pcm_name, stream, 0)) < 0) {
    error_message("Failed to open %s: %s", pcm_name, snd_strerror(err));
    return false;
  }

  // Init hwparams with full configuration space
  if ((err = snd_pcm_hw_params_any(m_pcmHandle, hwparams)) < 0) {
    error_message("Can not configure the PCM device %s: %s", pcm_name, snd_strerror(err));
    return false;
  }

	// Enable resample
#ifdef HAVE_SND_PCM_HW_PARAMS_SET_RATE_RESAMPLE
  if ((err = snd_pcm_hw_params_set_rate_resample(m_pcmHandle, hwparams, 1)) < 0) {
    error_message("Error enabling resample for PCM device %s: %s", pcm_name, snd_strerror(err));
    return false;
  }
#endif

  // Set access type to SND_PCM_ACCESS_RW_INTERLEAVED
  if ((err = snd_pcm_hw_params_set_access(m_pcmHandle, hwparams, SND_PCM_ACCESS_RW_INTERLEAVED)) < 0) {
    error_message("Error setting access type for PCM device %s: %s", pcm_name, snd_strerror(err));
    return false;
  }

  // Set sample format
#ifdef WORDS_BIGENDIAN
#define OUR_FORMAT SND_PCM_FORMAT_S16_BE
#else
#define OUR_FORMAT SND_PCM_FORMAT_S16_LE
#endif
  if ((err = snd_pcm_hw_params_set_format(m_pcmHandle, hwparams, OUR_FORMAT)) < 0) {
    error_message("Couldn't set format for PCM device %s: %s", pcm_name, snd_strerror(err));
    snd_pcm_close(m_pcmHandle);
    return false;
  }

  // Set number of channels
  m_channelsConfigured = m_pConfig->GetIntegerValue(CONFIG_AUDIO_CHANNELS);
  if ((err = snd_pcm_hw_params_set_channels(m_pcmHandle, hwparams, m_channelsConfigured)) < 0) {
    error_message("Couldn't set audio channels for PCM device %s: %s", pcm_name, snd_strerror(err));
    snd_pcm_close(m_pcmHandle);
    return false;
  }

  // Set sample rate
  u_int32_t samplingRate = m_pConfig->GetIntegerValue(CONFIG_AUDIO_SAMPLE_RATE);
  u_int32_t targetSamplingRate = samplingRate;
  // TODO maybe use snd_pcm_hw_params_set_rate instead
  if ((err = snd_pcm_hw_params_set_rate_near(m_pcmHandle, hwparams, &targetSamplingRate, 0)) < 0) {
    error_message("Couldn't set sample rate for PCM device %s: %s", pcm_name, snd_strerror(err));
    snd_pcm_close(m_pcmHandle);
    return false;
  }

  if (samplingRate != targetSamplingRate) {
    error_message("Couldn't set sample rate for PCM device %s", pcm_name);
    snd_pcm_close(m_pcmHandle);
    return false;
  }

  // Apply HW parameter settings to PCM device. This will also put it into PREPARED state
  if ((err = snd_pcm_hw_params(m_pcmHandle, hwparams)) < 0) {
    error_message("Couldn't set hw parameters PCM device %s: %s", pcm_name, snd_strerror(err));
    snd_pcm_close(m_pcmHandle);
    return false;
  }

  debug_message("alsa init done");
  return true;
}

void CALSAAudioSource::ProcessAudio(void)
{
	int err;
	
  if (m_audioSrcFrameNumber == 0) {
  	// Start the device
    if ((err = snd_pcm_start(m_pcmHandle)) < 0) {
      error_message("Couldn't start the PCM device: %s", snd_strerror(err));
    }
  }
	
  snd_pcm_status_t *status;
  snd_pcm_status_alloca(&status);

  // for efficiency, process 1 second before returning to check for commands
  for (int pass = 0; pass < m_maxPasses && m_stop_thread == false; pass++) {

    u_int8_t*     pcmFrameBuffer;
    pcmFrameBuffer = (u_int8_t*)malloc(m_pcmFrameSize);

    // The alsa frames is not the same as the pcm frames used to feed the encoder
    // Calculate how many alsa frames is neccesary to read to fill one pcm frame
	  snd_pcm_uframes_t num_frames = m_pcmFrameSize / (m_audioSrcChannels * sizeof(u_int16_t));
	  
		// Check how many bytes there is to read in the buffer, it will be used to calculate timestamp
    snd_pcm_status(m_pcmHandle, status);
		unsigned long avail_bytes = snd_pcm_status_get_avail(status) * (m_audioSrcChannels * sizeof(u_int16_t));
    Timestamp currentTime = GetTimestamp();
    Timestamp timestamp;

    // Read num_frames frames from the PCM device
    // pointed to by pcm_handle to buffer capdata.
    // Returns the number of frames actually read.
    // TODO On certain alsa configurations, e.g. when using dsnoop with low sample rate, the period gets too small. What to do about that?
    snd_pcm_sframes_t framesRead;
    if((framesRead = snd_pcm_readi(m_pcmHandle, pcmFrameBuffer, num_frames)) == -EPIPE) {
      snd_pcm_prepare(m_pcmHandle);
      // Buffer Overrun. This means the audio buffer is full, and not capturing
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
      if (m_timestampOverflowArray != NULL && m_timestampOverflowArray[m_timestampOverflowArrayIndex] != 0) {
        timestamp = m_timestampOverflowArray[m_timestampOverflowArrayIndex];
      } else {
        timestamp = m_prevTimestamp + SrcSamplesToTicks(avail_bytes);
      }

      if (m_timestampOverflowArray != NULL)
        m_timestampOverflowArray[m_timestampOverflowArrayIndex] = currentTime;

      debug_message("audio buffer full !");
    } else {
      if (framesRead < (snd_pcm_sframes_t) num_frames) {
        error_message("Bad audio read. Expected %li frames, got %li", num_frames, framesRead);
        free(pcmFrameBuffer);
        continue;
      }

      // buffer is not full - so, we make the timestamp based on the number
      // of bytes in the buffer that we read.
      timestamp = currentTime - SrcSamplesToTicks(SrcBytesToSamples(avail_bytes));
      if (m_timestampOverflowArray != NULL)
        m_timestampOverflowArray[m_timestampOverflowArrayIndex] = 0;
    }
    //debug_message("alsa read");
#ifdef DEBUG_TIMESTAMPS
    debug_message("avail_bytes=%lu t="U64" timestamp="U64" delta="U64,
                  avail_bytes, currentTime, timestamp, timestamp - m_prevTimestamp);
#endif

    m_prevTimestamp = timestamp;
    if (m_timestampOverflowArray != NULL) {
      m_timestampOverflowArrayIndex = (m_timestampOverflowArrayIndex + 1) % m_audioMaxBufferFrames;
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

bool CALSAAudioCapabilities::ProbeDevice()
{
  if (allSampleRateTableSize > NUM_ELEMENTS_IN_ARRAY(m_samplingRates)) {
    error_message("Number of sample rates exceeds audio cap array");
    return false;
  }

  // Open PCM
  snd_pcm_t *pcm_handle;
  snd_pcm_stream_t stream = SND_PCM_STREAM_CAPTURE;
  snd_pcm_hw_params_t *hwparams;
  snd_pcm_hw_params_alloca(&hwparams);
	int err;
  if ((err= snd_pcm_open(&pcm_handle, m_deviceName, stream, SND_PCM_NONBLOCK)) < 0) {
    error_message("Failed to open %s: %s", m_deviceName, snd_strerror(err));
    return false;
  }
  m_canOpen = true;

  if ((err = snd_pcm_hw_params_any(pcm_handle, hwparams)) < 0) {
    error_message("Broken configuration for playback: no configurations available: %s\n", snd_strerror(err));
    return false;
  }

  // union of valid sampling rates for MP3 and AAC
  // for all possible sampling rates
  u_int8_t i;
  for (i = 0; i < allSampleRateTableSize; i++) {
    u_int32_t targetRate = allSampleRateTable[i];
    if (snd_pcm_hw_params_test_rate(pcm_handle, hwparams, targetRate, 0) != 0) {
      debug_message("audio device %s doesn't support sampling rate %u",
                    m_deviceName, targetRate);
      continue;
    }
    debug_message("sampling rate %u supported", targetRate);

    // valid sampling rate
    m_samplingRates[m_numSamplingRates++] = targetRate;
  }

  // zero out remaining sampling rate entries
  for (i = m_numSamplingRates; i < allSampleRateTableSize; i++) {
    m_samplingRates[i] = 0;
  }

  snd_pcm_close(pcm_handle);
  return true;
}

void CALSAAudioCapabilities::Display (CLiveConfig *pConfig, 
				  char *msg, 
				  uint32_t max_len)
{
  // might want to parse out CONFIG_AUDIO_INPUT_NAME a bit to make it
  // more English like...
  snprintf(msg, max_len,
	   "%s, %s, %u Hz, %s", 
	   pConfig->GetStringValue(CONFIG_AUDIO_SOURCE_TYPE),
	   pConfig->GetStringValue(CONFIG_AUDIO_SOURCE_NAME),
	   pConfig->GetIntegerValue(CONFIG_AUDIO_SAMPLE_RATE),
	   pConfig->GetIntegerValue(CONFIG_AUDIO_CHANNELS) == 1 ? 
	   "mono" : "stereo");
}

#endif

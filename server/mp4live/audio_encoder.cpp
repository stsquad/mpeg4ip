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
 * Copyright (C) Cisco Systems Inc. 2002.  All Rights Reserved.
 * 
 * Contributor(s): 
 *	Bill May		wmay@cisco.com	
 *	Peter Maersk-Moller	peter@maersk-moller.net
 */

#include "mp4live.h"
#include "audio_encoder.h"
#include "mp4av.h"

/*
 * This looks like a fairly bogus set of routines; however, the
 * code makes it really easy to add your own codecs here, include
 * the mp4live library, write your own main, and go.
 * Just add the codecs you want before the call to the base routines
 */
const uint32_t allSampleRateTable[] = {
  7350, 8000, 11025, 12000, 16000, 22050, 
  24000, 32000, 44100, 48000, 64000, 88200, 96000
};
const uint32_t allSampleRateTableSize = NUM_ELEMENTS_IN_ARRAY(allSampleRateTable);

CAudioEncoder* AudioEncoderCreate(CAudioProfile *ap, 
				  CAudioEncoder *next, 
				      u_int8_t srcChannels,
				      u_int32_t srcSampleRate,
				      bool realTime)
{
  // add codecs here
  return AudioEncoderBaseCreate(ap, next, 
				srcChannels, srcSampleRate, realTime);
}

MediaType get_audio_mp4_fileinfo (CAudioProfile *pConfig,
				  bool *mpeg4,
				  bool *isma_compliant,
				  uint8_t *audioProfile,
				  uint8_t **audioConfig,
				  uint32_t *audioConfigLen,
				  uint8_t *mp4_audio_type)
{
  return get_base_audio_mp4_fileinfo(pConfig, mpeg4, isma_compliant, 
				     audioProfile, audioConfig, 
				     audioConfigLen, mp4_audio_type);
}

media_desc_t *create_audio_sdp (CAudioProfile *pConfig,
				bool *mpeg4,
				bool *isma_compliant,
				uint8_t *audioProfile,
				uint8_t **audioConfig,
				uint32_t *audioConfigLen)
{
  return create_base_audio_sdp(pConfig, mpeg4, isma_compliant,
			       audioProfile, audioConfig, audioConfigLen);
}
				
void create_mp4_audio_hint_track (CAudioProfile *pConfig, 
				  MP4FileHandle mp4file,
				  MP4TrackId trackId,
				  uint16_t mtu)
{
  create_base_mp4_audio_hint_track(pConfig, mp4file, trackId, mtu);
}

bool get_audio_rtp_info (CAudioProfile *pConfig,
			 MediaType *audioFrameType,
			 uint32_t *audioTimeScale,
			 uint8_t *audioPayloadNumber,
			 uint8_t *audioPayloadBytesPerPacket,
			 uint8_t *audioPayloadBytesPerFrame,
			 uint8_t *audioQueueMaxCount,
			 uint8_t *audioiovMaxCount,
			 audio_queue_frame_f *audio_queue_frame,
			 audio_set_rtp_payload_f *audio_set_rtp_payload,
			 audio_set_rtp_header_f *audio_set_header,
			 audio_set_rtp_jumbo_frame_f *audio_set_jumbo,
			 void **ud)
{
  return get_base_audio_rtp_info(pConfig, 
				 audioFrameType, 
				 audioTimeScale,
				 audioPayloadNumber, 
				 audioPayloadBytesPerPacket,
				 audioPayloadBytesPerFrame, 
				 audioQueueMaxCount,
				 audioiovMaxCount,
			 	 audio_queue_frame,
			         audio_set_rtp_payload,
				 audio_set_header, 
				 audio_set_jumbo, 
				 ud);
}

CAudioEncoder::CAudioEncoder(CAudioProfile *profile,
			     CAudioEncoder *next, 
			     u_int8_t srcChannels,
			     u_int32_t srcSampleRate,
			     bool realTime) :
  CMediaCodec(profile, next, realTime) 
{
    m_audioSrcChannels = srcChannels;
    m_audioSrcSampleRate = srcSampleRate;
    m_audioPreEncodingBuffer = NULL;
    m_audioPreEncodingBufferLength = 0;
    m_audioPreEncodingBufferMaxLength = 0;
    m_audioResample = NULL;
    m_audioSrcFrameNumber = 0;
}

void CAudioEncoder::Initialize (void)
{
  // called from derived classes init function from the start function
  // in the media flow
  m_audioSrcFrameNumber = 0;
  m_audioDstFrameNumber = 0;
  m_audioDstSampleNumber = 0;
  m_audioSrcElapsedDuration = 0;
  m_audioDstElapsedDuration = 0;

  // destination parameters are from the audio profile
  m_audioDstType = GetFrameType();
  m_audioDstSampleRate = m_pConfig->GetIntegerValue(CFG_AUDIO_SAMPLE_RATE);
  m_audioDstChannels = m_pConfig->GetIntegerValue(CFG_AUDIO_CHANNELS);
  m_audioDstSamplesPerFrame = GetSamplesPerFrame();

  // if we need to resample
  if (m_audioDstSampleRate != m_audioSrcSampleRate) {
    // create a resampler for each audio destination channel - 
    // we will combine the channels before resampling
    m_audioResample = (resample_t *)malloc(sizeof(resample_t) *
					   m_audioDstChannels);
    for (int ix = 0; ix <= m_audioDstChannels; ix++) {
      m_audioResample[ix] = st_resample_start(m_audioSrcSampleRate, 
					      m_audioDstSampleRate);
    }
  }

  // this calculation doesn't take into consideration the resampling
  // size of the src.  4 times might not be enough - we need most likely
  // 2 times the max of the src samples and the dest samples

  m_audioPreEncodingBufferLength = 0;
  m_audioPreEncodingBufferMaxLength =
    4 * DstSamplesToBytes(m_audioDstSamplesPerFrame);

  m_audioPreEncodingBuffer = (u_int8_t*)realloc(
						m_audioPreEncodingBuffer,
						m_audioPreEncodingBufferMaxLength);
}

// Audio encoding main process
int CAudioEncoder::ThreadMain(void) 
{
  CMsg* pMsg;
  bool stop = false;

  //  debug_message("audio encoder thread %s start", Profile()->GetName());

  while (stop == false && SDL_SemWait(m_myMsgQueueSemaphore) == 0) {
    pMsg = m_myMsgQueue.get_message();
    if (pMsg != NULL) {
      switch (pMsg->get_value()) {
      case MSG_NODE_STOP_THREAD:
	DoStopAudio();
	stop = true;
	break;
      case MSG_NODE_START:
	// DoStartTransmit();  Anything ?
	break;
      case MSG_NODE_STOP:
	DoStopAudio();
	break;
      case MSG_SINK_FRAME: {
	uint32_t dontcare;
	CMediaFrame *mf = (CMediaFrame*)pMsg->get_message(dontcare);
	if (m_stop_thread == false)
	  ProcessAudioFrame(mf);
	if (mf->RemoveReference()) {
	  delete mf;
	}
	break;
      }
      }
      
      delete pMsg;
    }
  }
  while ((pMsg = m_myMsgQueue.get_message()) != NULL) {
    if (pMsg->get_value() == MSG_SINK_FRAME) {
      uint32_t dontcare;
      CMediaFrame *mf = (CMediaFrame*)pMsg->get_message(dontcare);
      if (mf->RemoveReference()) {
	delete mf;
      }
    }
    delete pMsg;
  }
  debug_message("audio encoder thread %s exit", Profile()->GetName());
  
  return 0;
}

void CAudioEncoder::AddSilenceFrame(void)
{
  int bytes = DstSamplesToBytes(m_audioDstSamplesPerFrame);
  uint8_t *pSilenceData = (uint8_t *)Malloc(bytes);
  memset(pSilenceData, 0, bytes);

  bool rc = EncodeSamples(
			  (int16_t*)pSilenceData,
			  m_audioDstSamplesPerFrame,
			  m_audioDstChannels);
  if (!rc) {
    debug_message("failed to encode audio");
    return;
  }

  ForwardEncodedAudioFrames();
  free(pSilenceData);
}

void CAudioEncoder::ProcessAudioFrame(CMediaFrame *pFrame)
{
  const u_int8_t* frameData = (const uint8_t *)pFrame->GetData();
  u_int32_t frameDataLength = pFrame->GetDataLength();
  Timestamp srcFrameTimestamp = pFrame->GetTimestamp();;
  if (m_audioSrcFrameNumber == 0) {
    m_audioStartTimestamp = srcFrameTimestamp;
#ifdef DEBUG_AUDIO_SYNC
    debug_message("m_audioStartTimestamp = "U64, m_audioStartTimestamp);
#endif
  }

  if (m_audioDstFrameNumber == 0) {
    // we wait until we see the first encoded frame.
    // this is because encoders usually buffer the first few
    // raw audio frames fed to them, and this number varies
    // from one encoder to another
    m_audioEncodingStartTimestamp = srcFrameTimestamp;
  }

  // we calculate audioSrcElapsedDuration by taking the current frame's
  // timestamp and subtracting the audioEncodingStartTimestamp (and NOT
  // the audioStartTimestamp).
  // this way, we just need to compare audioSrcElapsedDuration with 
  // audioDstElapsedDuration (which should match in the ideal case),
  // and we don't have to compensate for the lag introduced by the initial
  // buffering of source frames in the encoder, which may vary from
  // one encoder to another
  m_audioSrcElapsedDuration = srcFrameTimestamp - m_audioEncodingStartTimestamp;
  m_audioSrcFrameNumber++;

  bool pcmMalloced = false;
  bool pcmBuffered;
  const u_int8_t* pcmData = frameData;
  u_int32_t pcmDataLength = frameDataLength;
  uint32_t audioSrcSamplesPerFrame = SrcBytesToSamples(frameDataLength);

  if (m_audioSrcChannels != m_audioDstChannels) {
    // Convert the channels if they don't match
    // we either double the channel info, or combine
    // the left and right
    uint32_t samples = SrcBytesToSamples(frameDataLength);
    uint32_t dstLength = DstSamplesToBytes(samples);
    pcmData = (u_int8_t *)Malloc(dstLength);
    pcmDataLength = dstLength;
    pcmMalloced = true;

    int16_t *src = (int16_t *)frameData;
    int16_t *dst = (int16_t *)pcmData;
    if (m_audioSrcChannels == 1) {
      // 1 channel to 2
      for (uint32_t ix = 0; ix < samples; ix++) {
	*dst++ = *src;
	*dst++ = *src++;
      }
    } else {
      // 2 channels to 1
      for (uint32_t ix = 0; ix < samples; ix++) {
	int32_t sum = *src++;
	sum += *src++;
	sum /= 2;
	if (sum < -32768) sum = -32768;
	else if (sum > 32767) sum = 32767;
	*dst++ = sum & 0xffff;
      }
    }
  }

  // resample audio, if necessary
  if (m_audioSrcSampleRate != m_audioDstSampleRate) {
    ResampleAudio(pcmData, pcmDataLength);

    // resampled data is now available in m_audioPreEncodingBuffer
    pcmBuffered = true;

  } else if (audioSrcSamplesPerFrame != m_audioDstSamplesPerFrame) {
    // reframe audio, if necessary
    // e.g. MP3 is 1152 samples/frame, AAC is 1024 samples/frame

    // add samples to end of m_audioBuffer
    // InitAudio() ensures that buffer is large enough
    if (m_audioPreEncodingBuffer == NULL) {
      m_audioPreEncodingBuffer = 
	(u_int8_t*)realloc(m_audioPreEncodingBuffer,
			   m_audioPreEncodingBufferMaxLength);
    }
    memcpy(
	   &m_audioPreEncodingBuffer[m_audioPreEncodingBufferLength],
	   pcmData,
	   pcmDataLength);

    m_audioPreEncodingBufferLength += pcmDataLength;

    pcmBuffered = true;

  } else {
    pcmBuffered = false;
  }

  // LATER restructure so as get rid of this label, and goto below
 pcmBufferCheck:

  if (pcmBuffered) {
    u_int32_t samplesAvailable =
      DstBytesToSamples(m_audioPreEncodingBufferLength);

    // not enough samples collected yet to call encode or forward
    if (samplesAvailable < m_audioDstSamplesPerFrame) {
      return;
    }
    if (pcmMalloced) {
      free((void *)pcmData);
      pcmMalloced = false;
    }

    // setup for encode/forward
    pcmData = &m_audioPreEncodingBuffer[0];
    pcmDataLength = DstSamplesToBytes(m_audioDstSamplesPerFrame);
  }


  // encode audio frame
  Duration frametime = DstSamplesToTicks(DstBytesToSamples(frameDataLength));

#ifdef DEBUG_AUDIO_SYNC
  debug_message("asrc# %d srcDuration="U64" dst# %d dstDuration "U64,
		m_audioSrcFrameNumber, m_audioSrcElapsedDuration,
		m_audioDstFrameNumber, m_audioDstElapsedDuration);
#endif

  // destination gets ahead of source
  // This has been observed as a result of clock frequency drift between
  // the sound card oscillator and the system mainbord oscillator
  // Example: If the sound card oscillator has a 'real' frequency that
  // is slightly larger than the 'rated' frequency, and we are sampling
  // at 32kHz, then the 32000 samples acquired from the sound card
  // 'actually' occupy a duration of slightly less than a second.
  // 
  // The clock drift is usually fraction of a Hz and takes a long
  // time (~ 20-30 minutes) before we are off by one frame duration
  
  if (m_audioSrcElapsedDuration + frametime < m_audioDstElapsedDuration) {
    debug_message("audio: dropping frame, SrcElapsedDuration="U64" DstElapsedDuration="U64,
		  m_audioSrcElapsedDuration, m_audioDstElapsedDuration);
    return;
  }

  // source gets ahead of destination
  // We tolerate a difference of 3 frames since A/V sync is usually
  // noticeable after that. This way we give the encoder a chance to pick up
  if (m_audioSrcElapsedDuration > (3 * frametime) + m_audioDstElapsedDuration) {
    int j = (int) (DstTicksToSamples(m_audioSrcElapsedDuration
				     + (2 * frametime)
				     - m_audioDstElapsedDuration)
		   / m_audioDstSamplesPerFrame);
    debug_message("audio: Adding %d silence frames", j);
    for (int k=0; k<j; k++)
      AddSilenceFrame();
  }
  
  //Timestamp encodingStartTimestamp = GetTimestamp();
  bool rc = EncodeSamples(
			  (int16_t*)pcmData,
			  m_audioDstSamplesPerFrame,
			  m_audioDstChannels);
  
  if (!rc) {
    debug_message("failed to encode audio");
    return;
  }
  
  // Disabled since we are not taking into account audio drift anymore
  /*
    Duration encodingTime =  (GetTimestamp() - encodingStartTimestamp);
    if (m_sourceRealTime && m_videoSource) {
    Duration drift;
    if (frametime <= encodingTime) {
    drift = encodingTime - frametime;
    m_videoSource->AddEncodingDrift(drift);
    }
    }
  */
  
  ForwardEncodedAudioFrames();
  if (pcmMalloced) {
    free((void *)pcmData);
  }

  if (pcmBuffered) {
    m_audioPreEncodingBufferLength -= pcmDataLength;
    memcpy(
	   &m_audioPreEncodingBuffer[0],
	   &m_audioPreEncodingBuffer[pcmDataLength],
	   m_audioPreEncodingBufferLength);

    goto pcmBufferCheck;
  }

}

void CAudioEncoder::ResampleAudio(
				 const u_int8_t* frameData,
				 u_int32_t frameDataLength)
{
  uint32_t samplesIn;
  uint32_t samplesInConsumed;
  uint32_t outBufferSamplesLeft;
  uint32_t outBufferSamplesWritten;
  uint32_t chan_offset;

  samplesIn = DstBytesToSamples(frameDataLength);

  // so far, record the pre length
  while (samplesIn > 0) {
    outBufferSamplesLeft = 
      DstBytesToSamples(m_audioPreEncodingBufferMaxLength - 
			m_audioPreEncodingBufferLength);
    for (uint8_t chan_ix = 0; chan_ix < m_audioDstChannels; chan_ix++) {
      samplesInConsumed = samplesIn;
      outBufferSamplesWritten = outBufferSamplesLeft;

      chan_offset = chan_ix * (DstSamplesToBytes(1));
#ifdef DEBUG_AUDIO_RESAMPLER
      error_message("resample - chans %d %d, samples %d left %d", 
		    m_audioDstChannels, chan_ix,
		    samplesIn, outBufferSamplesLeft);
#endif

      if (st_resample_flow(m_audioResample[chan_ix],
			   (int16_t *)(frameData + chan_offset),
			   (int16_t *)(&m_audioPreEncodingBuffer[m_audioPreEncodingBufferLength + chan_offset]),
			   &samplesInConsumed, 
			   &outBufferSamplesWritten,
			   m_audioDstChannels) < 0) {
	error_message("resample failed");
      }
#ifdef DEBUG_AUDIO_RESAMPLER
      debug_message("Chan %d consumed %d wrote %d", 
		    chan_ix, samplesInConsumed, outBufferSamplesWritten);
#endif
    }
    if (outBufferSamplesLeft < outBufferSamplesWritten) {
      error_message("Written past end of buffer");
    }
    samplesIn -= samplesInConsumed;
    outBufferSamplesLeft -= outBufferSamplesWritten;
    m_audioPreEncodingBufferLength += DstSamplesToBytes(outBufferSamplesWritten);
    // If we have no room for new output data, and more to process,
    // give us a bunch more room...
    if (outBufferSamplesLeft == 0 && samplesIn > 0) {
      m_audioPreEncodingBufferMaxLength *= 2;
      m_audioPreEncodingBuffer = 
	(u_int8_t*)realloc(m_audioPreEncodingBuffer,
			   m_audioPreEncodingBufferMaxLength);
    }
  } // end while we still have input samples
}

void CAudioEncoder::ForwardEncodedAudioFrames(void)
{
  u_int8_t* pFrame;
  u_int32_t frameLength;
  u_int32_t frameNumSamples;

  while (GetEncodedFrame(&pFrame, 
			 &frameLength, 
			 &frameNumSamples)) {

    // sanity check
    if (pFrame == NULL || frameLength == 0) {
      break;
    }

    //debug_message("Got encoded frame");

    // output has frame start timestamp
    Timestamp output = DstSamplesToTicks(m_audioDstSampleNumber);

    m_audioDstFrameNumber++;
    m_audioDstSampleNumber += frameNumSamples;
    m_audioDstElapsedDuration = DstSamplesToTicks(m_audioDstSampleNumber);

    //debug_message("m_audioDstSampleNumber = %llu", m_audioDstSampleNumber);

    // forward the encoded frame to sinks

#ifdef DEBUG_SYNC
    debug_message("audio forwarding "U64, output);
#endif
    CMediaFrame* pMediaFrame =
      new CMediaFrame(
		      GetFrameType(),
		      pFrame, 
		      frameLength,
		      m_audioStartTimestamp + output,
		      frameNumSamples,
		      m_audioDstSampleRate);

    ForwardFrame(pMediaFrame);
  }
}

void CAudioEncoder::DoStopAudio()
{
  // flush remaining output from audio encoder
  // and forward it to sinks
  
  EncodeSamples(NULL, 0, m_audioSrcChannels);

  ForwardEncodedAudioFrames();

  StopEncoder();

  CHECK_AND_FREE(m_audioPreEncodingBuffer);
  debug_message("Audio profile %s stats", GetProfileName());
  debug_message(" encoded samples: "U64, m_audioDstSampleNumber);
  debug_message(" encoded frames: %u", m_audioDstFrameNumber);
}

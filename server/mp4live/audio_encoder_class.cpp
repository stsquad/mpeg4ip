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
//#define DEBUG_SYNC 1
//#define DEBUG_AUDIO_RESAMPLER 1
//#define DEBUG_AUDIO_SYNC 1

CAudioEncoder::CAudioEncoder(CAudioProfile *profile,
			     CAudioEncoder *next, 
			     u_int8_t srcChannels,
			     u_int32_t srcSampleRate,
			     uint16_t mtu,
			     bool realTime) :
  CMediaCodec(profile, mtu, next, realTime) 
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
    for (int ix = 0; ix < m_audioDstChannels; ix++) {
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

  debug_message("audio encoder thread %s %s %s start", Profile()->GetName(),
		Profile()->GetStringValue(CFG_AUDIO_ENCODER), 
		Profile()->GetStringValue(CFG_AUDIO_ENCODING));

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

  if (m_audioResample != NULL) {
    for (uint ix = 0; ix < m_audioDstChannels; ix++) {
      st_resample_stop(m_audioResample[ix]);
      m_audioResample[ix] = NULL;
    }
    free(m_audioResample);
  }
  CHECK_AND_FREE(m_audioPreEncodingBuffer);
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

  bool pcmMalloced = false;
  bool pcmBuffered;
  const u_int8_t* pcmData = frameData;
  u_int32_t pcmDataLength = frameDataLength;
  uint32_t audioSrcSamplesPerFrame = SrcBytesToSamples(frameDataLength);
  Duration subtractDuration = 0;

  /*************************************************************************
   * First convert input samples to format we need them to be in
   *************************************************************************/
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
    subtractDuration = 
      DstSamplesToTicks(DstBytesToSamples(m_audioPreEncodingBufferLength));
 
     ResampleAudio(pcmData, pcmDataLength);

    // resampled data is now available in m_audioPreEncodingBuffer
    pcmBuffered = true;

  } else if (audioSrcSamplesPerFrame != m_audioDstSamplesPerFrame) {
    // reframe audio, if necessary
    // e.g. MP3 is 1152 samples/frame, AAC is 1024 samples/frame

    // add samples to end of m_audioPreEncodingBuffer
    // InitAudio() ensures that buffer is large enough
    if (m_audioPreEncodingBuffer == NULL) {
      m_audioPreEncodingBuffer = 
	(u_int8_t*)realloc(m_audioPreEncodingBuffer,
			   m_audioPreEncodingBufferMaxLength);
    }
    subtractDuration = 
      DstSamplesToTicks(DstBytesToSamples(m_audioPreEncodingBufferLength));
    memcpy(
	   &m_audioPreEncodingBuffer[m_audioPreEncodingBufferLength],
	   pcmData,
	   pcmDataLength);

    m_audioPreEncodingBufferLength += pcmDataLength;

    pcmBuffered = true;

  } else {
    // default case - just use what we're passed
    pcmBuffered = false;
  }
 
  srcFrameTimestamp -= subtractDuration;

  /************************************************************************
   * Loop while we have enough samples
   ************************************************************************/
  Duration frametime = DstSamplesToTicks(m_audioDstSamplesPerFrame);
  if (m_audioDstFrameNumber == 0)
    debug_message("%s:frametime "U64, Profile()->GetName(), frametime);
  while (1) {

    /*
     * Record starting timestamps
     */
    if (m_audioSrcFrameNumber == 0) {
      /*
       * we use m_audioStartTimestamp to determine audio output start time
       */
      m_audioStartTimestamp = srcFrameTimestamp;
#ifdef DEBUG_AUDIO_SYNC
      if (Profile()->GetBoolValue(CFG_AUDIO_DEBUG))
	debug_message("%s:m_audioStartTimestamp = "U64, 
		      Profile()->GetName(), m_audioStartTimestamp);
#endif
    }
    
    if (m_audioDstFrameNumber == 0) {
      // we wait until we see the first encoded frame.
      // this is because encoders usually buffer the first few
      // raw audio frames fed to them, and this number varies
      // from one encoder to another
      // We use this value to determine if we need to drop due to
      // a bad input frequency
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
    m_audioSrcElapsedDuration = 
      srcFrameTimestamp - m_audioEncodingStartTimestamp;
    m_audioSrcFrameNumber++;


    if (pcmBuffered) {
      u_int32_t samplesAvailable =
	DstBytesToSamples(m_audioPreEncodingBufferLength);
      
      if (pcmMalloced) {
	free((void *)pcmData);
	pcmMalloced = false;
      }
#ifdef DEBUG_AUDIO_SYNC
      if (Profile()->GetBoolValue(CFG_AUDIO_DEBUG))
      debug_message("%s: samples %u need %u", 
		    Profile()->GetName(), 
		    samplesAvailable, m_audioDstSamplesPerFrame);
#endif
      // not enough samples collected yet to call encode or forward
      // we moved the data above.
      if (samplesAvailable < m_audioDstSamplesPerFrame) {
	return;
      }
      // setup for encode/forward
      pcmData = &m_audioPreEncodingBuffer[0];
      pcmDataLength = DstSamplesToBytes(m_audioDstSamplesPerFrame);
    }


#ifdef DEBUG_AUDIO_SYNC
      if (Profile()->GetBoolValue(CFG_AUDIO_DEBUG))
	debug_message("%s:srcDuration="U64" dstDuration "U64" "D64,
		  Profile()->GetName(),
		  m_audioSrcElapsedDuration,
		  m_audioDstElapsedDuration,
		      m_audioDstElapsedDuration - m_audioSrcElapsedDuration);
#endif

    /*
     * Check if we can encode, or if we have to add/drop frames
     * First check is to see if the source frequency is greater than the
     * theory frequency.
     */
    if (m_audioSrcElapsedDuration + frametime >= m_audioDstElapsedDuration) {
      
      // source gets ahead of destination
      // We tolerate a difference of 3 frames since A/V sync is usually
      // noticeable after that. This way we give the encoder a chance to pick 
      // up
      if (m_audioSrcElapsedDuration > 
	  (3 * frametime) + m_audioDstElapsedDuration) {
	int j = (int) (DstTicksToSamples(m_audioSrcElapsedDuration
					 + (2 * frametime)
					 - m_audioDstElapsedDuration)
		       / m_audioDstSamplesPerFrame);
	debug_message("%s: Adding %d silence frames", 
		      Profile()->GetName(), j);
	for (int k=0; k<j; k++)
	  AddSilenceFrame();
      }
      
#ifdef DEBUG_SYNC
      debug_message("%s:encoding", Profile()->GetName());
#endif
      /*
       * Actually encode and forward the frames
       */
      bool rc = EncodeSamples(
			      (int16_t*)pcmData,
			      m_audioDstSamplesPerFrame,
			      m_audioDstChannels);
      
      if (!rc) {
	debug_message("failed to encode audio");
      }
      
      ForwardEncodedAudioFrames();
    } else {
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
      
      debug_message("%s:audio: dropping frame, SrcElapsedDuration="U64" DstElapsedDuration="U64" "U64,
		    Profile()->GetName(), 
		    m_audioSrcElapsedDuration, m_audioDstElapsedDuration,
		    frametime);
      // don't return - drop through to remove frame
    }
    
    if (pcmMalloced) {
      free((void *)pcmData);
    }
    if (pcmBuffered) {
      /*
       * This means we're storing data, either from resampling, or if the
       * sample numbers do not match.  We will remove the encoded samples, 
       * and increment the srcFrameTimestamp
       */
      m_audioPreEncodingBufferLength -= pcmDataLength;
      memmove(
	     &m_audioPreEncodingBuffer[0],
	     &m_audioPreEncodingBuffer[pcmDataLength],
	     m_audioPreEncodingBufferLength);
      subtractDuration = 0;
      srcFrameTimestamp += frametime;
    } else {
      // no data in buffer (default case).
      return;
    }
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
    if (outBufferSamplesLeft * 2 <= samplesIn && samplesIn > 0) {
      m_audioPreEncodingBufferMaxLength *= 2;
      m_audioPreEncodingBuffer = 
	(u_int8_t*)realloc(m_audioPreEncodingBuffer,
			   m_audioPreEncodingBufferMaxLength);
    }
    for (uint8_t chan_ix = 0; chan_ix < m_audioDstChannels; chan_ix++) {
      samplesInConsumed = samplesIn;
      outBufferSamplesWritten = outBufferSamplesLeft;

      chan_offset = chan_ix * (DstSamplesToBytes(1));
#ifdef DEBUG_AUDIO_RESAMPLER
      error_message("%s:resample - chans %d %d, samples %d left %d", 
		    Profile()->GetName(),
		    m_audioDstChannels, chan_ix,
		    samplesIn, outBufferSamplesLeft);
#endif

      if (st_resample_flow(m_audioResample[chan_ix],
			   (int16_t *)(frameData + chan_offset),
			   (int16_t *)(&m_audioPreEncodingBuffer[m_audioPreEncodingBufferLength + chan_offset]),
			   &samplesInConsumed, 
			   &outBufferSamplesWritten,
			   m_audioDstChannels) < 0) {
	error_message("%s:resample failed", Profile()->GetName());
      }
#ifdef DEBUG_AUDIO_RESAMPLER
      debug_message("%s:Chan %d consumed %d wrote %d", 
		    Profile()->GetName(),
		    chan_ix, samplesInConsumed, outBufferSamplesWritten);
#endif
    }
    if (outBufferSamplesLeft < outBufferSamplesWritten) {
      error_message("%s:Written past end of buffer",
		    Profile()->GetName());
    }
    samplesIn -= samplesInConsumed;
    outBufferSamplesLeft -= outBufferSamplesWritten;
    m_audioPreEncodingBufferLength += DstSamplesToBytes(outBufferSamplesWritten);
    // If we have no room for new output data, and more to process,
    // give us a bunch more room...
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
#ifdef DEBUG_SYNC
      debug_message("%s:No frame", Profile()->GetName());
#endif
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
    debug_message("%s:audio forwarding "U64, 
		  Profile()->GetName(), output);
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

void CAudioEncoder::AddRtpDestination (CMediaStream *stream,
				       bool disable_ts_offset, 
				       uint16_t max_ttl,
				       in_port_t srcPort)
{
  mp4live_rtp_params_t *mrtp;

  if (stream->m_audio_rtp_session != NULL) {
    AddRtpDestInt(disable_ts_offset, stream->m_audio_rtp_session);
    return;
  } 
  mrtp = MALLOC_STRUCTURE(mp4live_rtp_params_t);
  rtp_default_params(&mrtp->rtp_params);
  mrtp->rtp_params.rtp_addr = stream->GetStringValue(STREAM_AUDIO_DEST_ADDR);
  mrtp->rtp_params.rtp_rx_port = srcPort;
  mrtp->rtp_params.rtp_tx_port = stream->GetIntegerValue(STREAM_AUDIO_DEST_PORT);
  mrtp->rtp_params.rtp_ttl = max_ttl;
  mrtp->rtp_params.transmit_initial_rtcp = 1;
  mrtp->rtp_params.rtcp_addr = stream->GetStringValue(STREAM_AUDIO_RTCP_DEST_ADDR);
  mrtp->rtp_params.rtcp_tx_port = stream->GetIntegerValue(STREAM_AUDIO_RTCP_DEST_PORT);

  mrtp->use_srtp = stream->GetBoolValue(STREAM_AUDIO_USE_SRTP);
  mrtp->srtp_params.enc_algo = 
    (srtp_enc_algos_t)stream->GetIntegerValue(STREAM_AUDIO_SRTP_ENC_ALGO);
  mrtp->srtp_params.auth_algo = 
    (srtp_auth_algos_t)stream->GetIntegerValue(STREAM_AUDIO_SRTP_AUTH_ALGO);
  mrtp->srtp_params.tx_key = stream->m_audio_key;
  mrtp->srtp_params.tx_salt = stream->m_audio_salt;
  mrtp->srtp_params.rx_key = stream->m_audio_key;
  mrtp->srtp_params.rx_salt = stream->m_audio_salt;
  mrtp->srtp_params.rtp_enc = stream->GetBoolValue(STREAM_AUDIO_SRTP_RTP_ENC);
  mrtp->srtp_params.rtp_auth = stream->GetBoolValue(STREAM_AUDIO_SRTP_RTP_AUTH);
  mrtp->srtp_params.rtcp_enc = stream->GetBoolValue(STREAM_AUDIO_SRTP_RTCP_ENC);
  AddRtpDestInt(disable_ts_offset, mrtp);
}

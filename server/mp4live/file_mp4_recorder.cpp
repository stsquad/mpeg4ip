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
#include "file_mp4_recorder.h"

#ifdef HAVE_LINUX_VIDEODEV2_H
#include "video_v4l2_source.h"
#else
#include "video_v4l_source.h"
#endif

#include "audio_encoder.h"

int CMp4Recorder::ThreadMain(void) 
{
  CMsg *pMsg;
  bool stop = false;

  while (stop == false && SDL_SemWait(m_myMsgQueueSemaphore) == 0) {
    pMsg = m_myMsgQueue.get_message();
		
    if (pMsg != NULL) {
      switch (pMsg->get_value()) {
      case MSG_NODE_STOP_THREAD:
        DoStopRecord();
	stop = true;
	break;
      case MSG_NODE_START:
        DoStartRecord();
        break;
      case MSG_NODE_STOP:
        DoStopRecord();
        break;
      case MSG_SINK_FRAME:
        size_t dontcare;
        DoWriteFrame((CMediaFrame*)pMsg->get_message(dontcare));
        break;
      }

      delete pMsg;
    }
  }

  while ((pMsg = m_myMsgQueue.get_message()) != NULL) {
    error_message("recorder - had msg after stop");
    if ((int)pMsg->get_value() == MSG_SINK_FRAME) {
      size_t dontcare;
      CMediaFrame *mf = (CMediaFrame*)pMsg->get_message(dontcare);
      if (mf->RemoveReference()) {
	delete mf;
      }
    }
    delete pMsg;
  }
  return 0;
}

void CMp4Recorder::DoStartRecord()
{
  // already recording
  if (m_sink) {
    return;
  }

  m_makeIod = true;
  m_makeIsmaCompliant = true;

  m_canRecordRawVideo = false;

  m_prevRawVideoFrame = NULL;
  m_prevEncodedVideoFrame = NULL;
  m_prevRawAudioFrame = NULL;
  m_prevEncodedAudioFrame = NULL;

  m_rawVideoTrackId = MP4_INVALID_TRACK_ID;
  m_encodedVideoTrackId = MP4_INVALID_TRACK_ID;
  m_rawAudioTrackId = MP4_INVALID_TRACK_ID;
  m_encodedAudioTrackId = MP4_INVALID_TRACK_ID;
  m_audioTimeScale =
    m_pConfig->GetIntegerValue(CONFIG_AUDIO_SAMPLE_RATE);

  // are we recording any video?
  if (m_pConfig->GetBoolValue(CONFIG_VIDEO_ENABLE)
      && (m_pConfig->GetBoolValue(CONFIG_RECORD_RAW_VIDEO)
          || m_pConfig->GetBoolValue(CONFIG_RECORD_ENCODED_VIDEO))) {
    m_recordVideo = true;
    m_movieTimeScale = m_videoTimeScale;

  } else { // just audio
    m_recordVideo = false;
    m_movieTimeScale = m_audioTimeScale;
  }

  // get the mp4 file setup
 
  // enable huge file mode in mp4 
  // if duration is very long or if estimated size goes over 1 GB
  u_int64_t duration = m_pConfig->GetIntegerValue(CONFIG_APP_DURATION) 
    * m_pConfig->GetIntegerValue(CONFIG_APP_DURATION_UNITS)
    * m_movieTimeScale;
  bool hugeFile = 
    (duration > 0xFFFFFFFF) 
    || (m_pConfig->m_recordEstFileSize > 1000000000LLU);
 
  debug_message("Creating huge file - %s", hugeFile ? "yes" : "no");
  u_int32_t verbosity =
    MP4_DETAILS_ERROR /* DEBUG | MP4_DETAILS_WRITE_ALL */;
 
  if (m_pConfig->GetBoolValue(CONFIG_RECORD_MP4_OVERWRITE)) {
    m_mp4File = MP4Create(
                          m_pConfig->GetStringValue(CONFIG_RECORD_MP4_FILE_NAME),
                          verbosity, hugeFile);
  } else {
    m_mp4File = MP4Modify(
                          m_pConfig->GetStringValue(CONFIG_RECORD_MP4_FILE_NAME),
                          verbosity);
  }
 
  if (!m_mp4File) {
    return;
  }
  MP4SetTimeScale(m_mp4File, m_movieTimeScale);

  if (m_pConfig->GetBoolValue(CONFIG_VIDEO_ENABLE)) {

    if (m_pConfig->GetBoolValue(CONFIG_RECORD_RAW_VIDEO)) {
      m_rawVideoFrameNumber = 1;

      m_rawVideoTrackId = MP4AddVideoTrack(
                                           m_mp4File,
                                           m_videoTimeScale,
                                           MP4_INVALID_DURATION,
                                           m_pConfig->m_videoWidth, 
                                           m_pConfig->m_videoHeight,
                                           MP4_YUV12_VIDEO_TYPE);

      if (m_rawVideoTrackId == MP4_INVALID_TRACK_ID) {
        error_message("can't create raw video track");
        goto start_failure;
      }

      MP4SetVideoProfileLevel(m_mp4File, 0xFF);
    }

    if (m_pConfig->GetBoolValue(CONFIG_RECORD_ENCODED_VIDEO)) {
      m_encodedVideoFrameNumber = 1;
      bool vIod, vIsma;
      uint8_t videoProfile;
      uint8_t *videoConfig;
      uint32_t videoConfigLen;
      uint8_t videoType;

      m_encodedVideoFrameType = 
        get_video_mp4_fileinfo(m_pConfig,
                               &vIod,
                               &vIsma,
                               &videoProfile,
                               &videoConfig,
                               &videoConfigLen,
                               &videoType);
						 
      m_encodedVideoTrackId = MP4AddVideoTrack(
                                               m_mp4File,
                                               m_videoTimeScale,
                                               MP4_INVALID_DURATION,
                                               m_pConfig->m_videoWidth, 
                                               m_pConfig->m_videoHeight,
                                               videoType);

      if (vIod == false) m_makeIod = false;
      if (vIsma == false) m_makeIsmaCompliant = false;

      if (m_encodedVideoTrackId == MP4_INVALID_TRACK_ID) {
        error_message("can't create encoded video track");
        goto start_failure;
      }

      MP4SetVideoProfileLevel(
                              m_mp4File, 
                              videoProfile);

      MP4SetTrackESConfiguration(
                                 m_mp4File, 
                                 m_encodedVideoTrackId,
                                 videoConfig,
                                 videoConfigLen);
    }
  }

  m_rawAudioFrameNumber = 1;
  m_encodedAudioFrameNumber = 1;

  m_canRecordEncodedVideo = true;
  if (m_pConfig->GetBoolValue(CONFIG_AUDIO_ENABLE)) {

    if (m_pConfig->GetBoolValue(CONFIG_RECORD_RAW_AUDIO)) {

      m_canRecordEncodedVideo = false;
      m_rawAudioTrackId = MP4AddAudioTrack(
                                           m_mp4File, 
                                           m_audioTimeScale, 
                                           0,
                                           MP4_PCM16_BIG_ENDIAN_AUDIO_TYPE);

      if (m_rawAudioTrackId == MP4_INVALID_TRACK_ID) {
        error_message("can't create raw audio track");
        goto start_failure;
      }

      MP4SetAudioProfileLevel(m_mp4File, 0xFF);
    }

    if (m_pConfig->GetBoolValue(CONFIG_RECORD_ENCODED_AUDIO)) {

      u_int8_t audioType;
      bool createIod = false;
      bool isma_compliant = false;
      uint8_t audioProfile;
      uint8_t *pAudioConfig;
      uint32_t audioConfigLen;
      m_canRecordEncodedVideo = false;
      m_encodedAudioFrameType = 
        get_audio_mp4_fileinfo(m_pConfig,
                               &createIod,
                               &isma_compliant,
                               &audioProfile,
                               &pAudioConfig,
                               &audioConfigLen,
                               &audioType);
					       
      if (createIod == false) m_makeIod = false;
      if (isma_compliant == false) 
        m_makeIsmaCompliant = false;
      MP4SetAudioProfileLevel(m_mp4File, audioProfile);
      m_encodedAudioTrackId = MP4AddAudioTrack(
                                               m_mp4File, 
                                               m_audioTimeScale, 
                                               MP4_INVALID_DURATION,
                                               audioType);

      if (m_encodedAudioTrackId == MP4_INVALID_TRACK_ID) {
        error_message("can't create encoded audio track");
        goto start_failure;
      }

      if (pAudioConfig) {
        MP4SetTrackESConfiguration(
                                   m_mp4File, 
                                   m_encodedAudioTrackId,
                                   pAudioConfig, 
                                   audioConfigLen);
	free(pAudioConfig);
      }
    }
  }

  m_sink = true;
  return;

 start_failure:
  MP4Close(m_mp4File);
  m_mp4File = NULL;
  return;
}

void CMp4Recorder::ProcessEncodedAudioFrame (CMediaFrame *pFrame)
{
    if (m_encodedAudioFrameNumber == 1) {
      m_encodedAudioStartTimestamp = pFrame->GetTimestamp();
      m_canRecordEncodedVideo = true;
      m_prevEncodedAudioFrame = pFrame;
      m_encodedAudioSamples = 0;
      m_encodedAudioDiffTicks = 0;
      m_encodedAudioDiffTicksTotal = 0;
      m_encodedAudioFrameNumber++;
      return; // wait until the next audio frame
    }

    Duration thisFrameDurationInTicks = 
      pFrame->GetTimestamp() - m_prevEncodedAudioFrame->GetTimestamp();

    Duration thisFrameDurationInSamples =
      GetTimescaleFromTicks(thisFrameDurationInTicks, m_audioTimeScale);

    Duration elapsedTimeFromSamples;
    Duration elapsedTimeFromTimestamp;
    elapsedTimeFromTimestamp = pFrame->GetTimestamp() - m_encodedAudioStartTimestamp;

    if (thisFrameDurationInSamples > pFrame->GetDuration()) {
      // we have a gap frame
      // add the number of extra samples
      m_encodedAudioSamples += 
	(thisFrameDurationInSamples - pFrame->GetDuration());
      error_message("adding encoded %lld samples", 
		    thisFrameDurationInSamples - pFrame->GetDuration());
    }
    // we have a consecutive frame
    // Timestamp of pFrame should reflect encoded audio samples
    // take difference
    m_encodedAudioSamples += pFrame->GetDuration();
    elapsedTimeFromSamples = 
      GetTicksFromTimescale(m_encodedAudioSamples, 0, 0, m_audioTimeScale);

    Duration diffTimeTicks;

    diffTimeTicks = elapsedTimeFromSamples - elapsedTimeFromTimestamp;

    // we need to adjust this by the amount of time we've already changed
    diffTimeTicks += m_encodedAudioDiffTicksTotal;

    if (diffTimeTicks != 0) {
      error_message("elfts %lld samples %lld elfs %lld diff %lld",
		    elapsedTimeFromTimestamp,
		    m_encodedAudioSamples,
		    elapsedTimeFromSamples,
		    diffTimeTicks);
      
      error_message("adj %lld diff %lld",
		    m_encodedAudioDiffTicksTotal, diffTimeTicks);
    }

    // diffTimeTicks is now the amount we need to add to the video frame
    m_encodedAudioDiffTicks = diffTimeTicks;

    m_encodedAudioDiffTicksTotal += diffTimeTicks;

    /*
     * we don't convert the audio duration any more
     *  m_prevEncodedAudioFrame->SetDuration(audioDurationInSamples);
     * Instead, we'll convert the video duration
     */
    MP4WriteSample(
                   m_mp4File,
                   m_encodedAudioTrackId,
                   (u_int8_t*)m_prevEncodedAudioFrame->GetData(), 
                   m_prevEncodedAudioFrame->GetDataLength(),
                   m_prevEncodedAudioFrame->ConvertDuration(m_audioTimeScale));

    m_encodedAudioFrameNumber++;
    if (m_prevEncodedAudioFrame->RemoveReference()) {
      delete m_prevEncodedAudioFrame;
    }
    m_prevEncodedAudioFrame = pFrame;
}

/******************************************************************************
 * Process encoded video frame
 ******************************************************************************/
void CMp4Recorder::ProcessEncodedVideoFrame (CMediaFrame *pFrame)
{
    // we drop encoded video frames until we get the first encoded audio frame
    // after that we drop encoded video frames until we get the first I frame.
    // we then stretch this I frame to the start of the first encoded audio
    // frame and write it to the encoded video track

    if (m_encodedVideoFrameNumber == 1) {
      // wait until first audio frame before looking for the next I frame
      if (!m_canRecordEncodedVideo) {
        if (pFrame->RemoveReference()) delete pFrame;
        return;
      }

      // make sure this frame was captured after the first audio frame
      if (m_pConfig->GetBoolValue(CONFIG_RECORD_ENCODED_AUDIO) &&
	  pFrame->GetTimestamp() < m_encodedAudioStartTimestamp) {
        if (pFrame->RemoveReference()) delete pFrame;
        return;
      }

      if (! MP4AV_Mpeg4GetVopType((u_int8_t*)pFrame->GetData(),
                                  pFrame->GetDataLength()) == 'I') {
        if (pFrame->RemoveReference()) delete pFrame;
        return;
      }

      if (m_pConfig->GetBoolValue(CONFIG_AUDIO_ENABLE) &&
	  m_pConfig->GetBoolValue(CONFIG_RECORD_ENCODED_AUDIO)) {
	// reset this timestamp to video's beginning
	pFrame->SetTimestamp(m_encodedAudioStartTimestamp);
      } 
      m_encodedVideoStartTimestamp = pFrame->GetTimestamp();
      m_prevEncodedVideoFrame = pFrame;
      m_encodedVideoFrameNumber++;
      m_encodedVideoDurationTimescale = 0;
      return; // wait until the next video frame
    }

    Duration videoDurationInTicks;

    videoDurationInTicks = 
      pFrame->GetTimestamp() - m_encodedVideoStartTimestamp;

    // at this point, we'll probably want to add in the audio drift values, 
    // if there are any
    Duration videoDurationInTimescaleTotal;
    videoDurationInTimescaleTotal = 
      GetTimescaleFromTicks(videoDurationInTicks, m_videoTimeScale);

    Duration videoDurationInTimescaleFrame;
    videoDurationInTimescaleFrame = 
      videoDurationInTimescaleTotal - m_encodedVideoDurationTimescale;

    m_encodedVideoDurationTimescale += videoDurationInTimescaleFrame;
#if 0
    debug_message("vdit %lld vdits %lld frame %lld total %lld %lld",
		  videoDurationInTicks, videoDurationInTimescaleTotal,
		  videoDurationInTimescaleFrame, m_encodedVideoDurationTimescale,
		  GetTicksFromTimescale(m_encodedVideoDurationTimescale, 0, 0, m_videoTimeScale));
#endif
    bool isIFrame =
      (MP4AV_Mpeg4GetVopType(
                             (u_int8_t*) m_prevEncodedVideoFrame->GetData(),
                             m_prevEncodedVideoFrame->GetDataLength()) == 'I');

    MP4WriteSample(
                   m_mp4File,
                   m_encodedVideoTrackId,
                   (u_int8_t*) m_prevEncodedVideoFrame->GetData(),
                   m_prevEncodedVideoFrame->GetDataLength(),
		   videoDurationInTimescaleFrame,
                   0,
                   isIFrame);
		
    m_encodedVideoFrameNumber++;
    if (m_prevEncodedVideoFrame->RemoveReference()) {
      delete m_prevEncodedVideoFrame;
    }
    m_prevEncodedVideoFrame = pFrame;
}

void CMp4Recorder::DoWriteFrame(CMediaFrame* pFrame)
{
  // dispose of degenerate cases
  if (pFrame == NULL) return;

  if (!m_sink) {
    if (pFrame->RemoveReference()) {
      delete pFrame;
    }
    return;
  }

  // RAW AUDIO
  if (pFrame->GetType() == PCMAUDIOFRAME
      && m_pConfig->GetBoolValue(CONFIG_RECORD_RAW_AUDIO)) {

    if (m_rawAudioFrameNumber == 1) {

      debug_message("First raw audio frame at %llu", pFrame->GetTimestamp());

      m_rawAudioStartTimestamp = pFrame->GetTimestamp();
      m_canRecordRawVideo = true;
      m_prevRawAudioFrame = pFrame;
      m_rawAudioFrameNumber++;
      return; // wait until the next audio frame
    }

    Duration audioDurationInTicks = 
      pFrame->GetTimestamp() - m_prevRawAudioFrame->GetTimestamp();

    MP4Duration audioDurationInSamples =
      MP4ConvertToTrackDuration(m_mp4File, m_rawAudioTrackId,
                                audioDurationInTicks, TimestampTicks);

    m_prevRawAudioFrame->SetDuration(audioDurationInSamples);

    MP4WriteSample(
                   m_mp4File,
                   m_rawAudioTrackId,
                   (u_int8_t*)m_prevRawAudioFrame->GetData(), 
                   m_prevRawAudioFrame->GetDataLength(),
                   m_prevRawAudioFrame->ConvertDuration(m_audioTimeScale));

    m_rawAudioFrameNumber++;
    if (m_prevRawAudioFrame->RemoveReference()) {
      delete m_prevRawAudioFrame;
    }
    m_prevRawAudioFrame = pFrame;

    // ENCODED AUDIO
  } else if (pFrame->GetType() == m_encodedAudioFrameType
             && m_pConfig->GetBoolValue(CONFIG_RECORD_ENCODED_AUDIO)) {

    ProcessEncodedAudioFrame(pFrame);
    // RAW VIDEO
  } else if (pFrame->GetType() == YUVVIDEOFRAME
             && m_pConfig->GetBoolValue(CONFIG_RECORD_RAW_VIDEO)) {

    // we drop raw video frames until we get the first raw audio frame
    // after that:
    // if we are also recording encoded video, we wait until the first I frame
    // else
    // we wait until the next raw video frame
    // in both cases, the duration of the first raw video frame is stretched
    // to the start of the first raw audio frame
    
    if (m_rawVideoFrameNumber == 1) {
      // wait until the first raw audio frame is received
      if (!m_canRecordRawVideo) {
        if (pFrame->RemoveReference()) delete pFrame;
        return;
      }

      // make sure this frame was captured after the first audio frame
      if (pFrame->GetTimestamp() < m_rawAudioStartTimestamp) {
        if (pFrame->RemoveReference()) delete pFrame;
        return;
      }

      // if we're also recording encoded video
      if (m_pConfig->GetBoolValue(CONFIG_RECORD_ENCODED_VIDEO)) {
        // media source will send encoded video frame first
        // so if we haven't gotten an encoded I frame yet
        // don't accept this raw video frame
        if (m_encodedVideoFrameNumber == 1) {
          if (pFrame->RemoveReference()) delete pFrame;
          return;
        }
      }

      debug_message("First raw video frame at %llu", pFrame->GetTimestamp());

      m_rawVideoStartTimestamp = pFrame->GetTimestamp();
      m_prevRawVideoFrame = pFrame;
      m_rawVideoFrameNumber++;
      return; // wait until the next video frame
    }

    Duration videoDurationInTicks;
    
    // the first raw video frame is stretched to the begining
    // of the first raw audio frame
    if (m_rawVideoFrameNumber == 2) {
      videoDurationInTicks =
        pFrame->GetTimestamp() - m_rawAudioStartTimestamp;
    } else {
      videoDurationInTicks =
        pFrame->GetTimestamp() - m_prevRawVideoFrame->GetTimestamp();
    }

    m_prevRawVideoFrame->SetDuration(videoDurationInTicks);

    MP4WriteSample(
                   m_mp4File,
                   m_rawVideoTrackId,
                   (u_int8_t*)m_prevRawVideoFrame->GetData(), 
                   m_prevRawVideoFrame->GetDataLength(),
                   m_prevRawVideoFrame->ConvertDuration(m_videoTimeScale));

    m_rawVideoFrameNumber++;
    if (m_prevRawVideoFrame->RemoveReference()) {
      delete m_prevRawVideoFrame;
    }
    m_prevRawVideoFrame = pFrame;

    // ENCODED VIDEO
  } else if (pFrame->GetType() == MPEG4VIDEOFRAME
             && m_pConfig->GetBoolValue(CONFIG_RECORD_ENCODED_VIDEO)) {

    ProcessEncodedVideoFrame(pFrame);
  } else {
    // degenerate case
    if (pFrame->RemoveReference()) delete pFrame;
  }
}

void CMp4Recorder::DoStopRecord()
{
  if (!m_sink) return;


  // write last audio frame
  if (m_prevRawAudioFrame) {
    MP4WriteSample(
                   m_mp4File,
                   m_rawAudioTrackId,
                   (u_int8_t*)m_prevRawAudioFrame->GetData(), 
                   m_prevRawAudioFrame->GetDataLength(),
                   m_prevRawAudioFrame->ConvertDuration(m_audioTimeScale));
    if (m_prevRawAudioFrame->RemoveReference()) {
      delete m_prevRawAudioFrame;
    }
  }
  if (m_prevEncodedAudioFrame) {
    MP4WriteSample(
                   m_mp4File,
                   m_encodedAudioTrackId,
                   (u_int8_t*)m_prevEncodedAudioFrame->GetData(), 
                   m_prevEncodedAudioFrame->GetDataLength(),
                   m_prevEncodedAudioFrame->ConvertDuration(m_audioTimeScale));
    if (m_prevEncodedAudioFrame->RemoveReference()) {
      delete m_prevEncodedAudioFrame;
    }
  }
 

  // write last video frame
  if (m_prevRawVideoFrame) {
    MP4WriteSample(
                   m_mp4File,
                   m_rawVideoTrackId,
                   (u_int8_t*)m_prevRawVideoFrame->GetData(), 
                   m_prevRawVideoFrame->GetDataLength(),
                   m_prevRawVideoFrame->ConvertDuration(m_videoTimeScale));
    if (m_prevRawVideoFrame->RemoveReference()) {
      delete m_prevRawVideoFrame;
    }
  }
  if (m_prevEncodedVideoFrame) {
    bool isIFrame =
      (MP4AV_Mpeg4GetVopType(
                             (u_int8_t*) m_prevEncodedVideoFrame->GetData(),
                             m_prevEncodedVideoFrame->GetDataLength()) == 'I');

    MP4WriteSample(
                   m_mp4File,
                   m_encodedVideoTrackId,
                   (u_int8_t*) m_prevEncodedVideoFrame->GetData(),
                   m_prevEncodedVideoFrame->GetDataLength(),
                   m_prevEncodedVideoFrame->ConvertDuration(m_videoTimeScale),
                   0,
                   isIFrame);
    if (m_prevEncodedVideoFrame->RemoveReference()) {
      delete m_prevEncodedVideoFrame;
    }
  }

  bool optimize = false;

  // create hint tracks
  if (m_pConfig->GetBoolValue(CONFIG_RECORD_MP4_HINT_TRACKS)) {

    if (m_pConfig->GetBoolValue(CONFIG_RECORD_MP4_OPTIMIZE)) {
      optimize = true;
    }

    if (MP4_IS_VALID_TRACK_ID(m_encodedVideoTrackId)) {
      create_mp4_video_hint_track(m_pConfig,
                                  m_mp4File, 
                                  m_encodedVideoTrackId);
    }

    if (MP4_IS_VALID_TRACK_ID(m_encodedAudioTrackId)) {
      create_mp4_audio_hint_track(m_pConfig, 
                                  m_mp4File, 
                                  m_encodedAudioTrackId);
    }
#if 0
    if ((m_pConfig->GetBoolValue(CONFIG_RECORD_RAW_AUDIO)) &&
        (MP4_IS_VALID_TRACK_ID(m_rawAudioTrackId))) {
      L16Hinter(m_mp4File, 
                m_rawAudioTrackId,
                m_pConfig->GetIntegerValue(CONFIG_RTP_PAYLOAD_SIZE));
    }
#endif
  }

  // close the mp4 file
  MP4Close(m_mp4File);
  m_mp4File = NULL;

  // add ISMA style OD and Scene tracks
  if (m_pConfig->GetBoolValue(CONFIG_RECORD_ENCODED_VIDEO)
      || m_pConfig->GetBoolValue(CONFIG_RECORD_ENCODED_AUDIO)) {

    bool useIsmaTag = false;

    // if AAC track is present, can tag this as ISMA compliant content
    useIsmaTag = m_makeIsmaCompliant;
    if (m_makeIod) {
      MP4MakeIsmaCompliant(
                           m_pConfig->GetStringValue(CONFIG_RECORD_MP4_FILE_NAME),
                           0,
                           useIsmaTag);
    }
  }

  if (optimize) {
    MP4Optimize(m_pConfig->GetStringValue(CONFIG_RECORD_MP4_FILE_NAME));
  }

  m_sink = false;
}

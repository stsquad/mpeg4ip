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
#include "video_encoder.h"
#include "audio_encoder.h"
#include "text_encoder.h"
#include "mpeg4ip_byteswap.h"

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
        uint32_t dontcare;
        DoWriteFrame((CMediaFrame*)pMsg->get_message(dontcare));
        break;
      }

      delete pMsg;
    }
  }

  while ((pMsg = m_myMsgQueue.get_message()) != NULL) {
    error_message("recorder - had msg after stop");
    if (pMsg->get_value() == MSG_SINK_FRAME) {
      uint32_t dontcare;
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
  const char *filename;

  m_makeIod = true;
  m_makeIsmaCompliant = true;

  m_audioFrameType = UNDEFINEDFRAME;
  m_videoFrameType = UNDEFINEDFRAME;
  m_textFrameType = UNDEFINEDFRAME;

  if (m_stream != NULL) {
    // recording normal file
    m_video_profile = m_stream->GetVideoProfile();
    m_audio_profile = m_stream->GetAudioProfile();
    m_text_profile = m_stream->GetTextProfile();
    m_recordVideo = m_stream->GetBoolValue(STREAM_VIDEO_ENABLED);
    m_recordAudio = m_stream->GetBoolValue(STREAM_AUDIO_ENABLED);
    m_recordText = m_stream->GetBoolValue(STREAM_TEXT_ENABLED);
    if (m_recordAudio) {
      m_audioTimeScale = 
	m_audio_profile->GetIntegerValue(CFG_AUDIO_SAMPLE_RATE);
    }
    filename = m_stream->GetStringValue(STREAM_RECORD_MP4_FILE_NAME);
  } else {
    // recording raw file
    m_recordVideo = m_pConfig->GetBoolValue(CONFIG_VIDEO_ENABLE) ||
      m_pConfig->GetBoolValue(CONFIG_RECORD_RAW_IN_MP4_VIDEO);
    m_recordAudio = m_pConfig->GetBoolValue(CONFIG_AUDIO_ENABLE) ||
      m_pConfig->GetBoolValue(CONFIG_RECORD_RAW_IN_MP4_AUDIO);
    m_recordText = false;
    m_audioTimeScale = m_pConfig->GetIntegerValue(CONFIG_AUDIO_SAMPLE_RATE);
    filename = m_pConfig->GetStringValue(CONFIG_RECORD_RAW_MP4_FILE_NAME);
  }


  m_prevVideoFrame = NULL;
  m_prevAudioFrame = NULL;
  m_prevTextFrame = NULL;

  m_videoTrackId = MP4_INVALID_TRACK_ID;
  m_audioTrackId = MP4_INVALID_TRACK_ID;
  m_textTrackId = MP4_INVALID_TRACK_ID;

  // are we recording any video?
  if (m_recordVideo || m_recordText) {
    m_movieTimeScale = m_videoTimeScale;
  } else { // just audio
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
    || (m_pConfig->m_recordEstFileSize > (TO_U64(1000000000)));
  uint32_t createFlags = 0;
  if (hugeFile) {
    createFlags |= MP4_CREATE_64BIT_DATA;
  }
  debug_message("Creating huge file - %s", hugeFile ? "yes" : "no");
  u_int32_t verbosity =
    MP4_DETAILS_ERROR /* DEBUG | MP4_DETAILS_WRITE_ALL */;

  bool create = false;
  switch (m_pConfig->GetIntegerValue(CONFIG_RECORD_MP4_FILE_STATUS)) {
  case FILE_MP4_APPEND:
    m_mp4File = MP4Modify(filename,
                          verbosity);
    break;
  case FILE_MP4_CREATE_NEW: {
    struct stat stats;
    const char *fname = filename;
    if (stat(fname, &stats) == 0) {
      // file already exists - create new one
      size_t len = strlen(fname);
      if (strncasecmp(fname + len - 4, ".mp4", 4) == 0) {
	len -= 4;
      }
      struct tm timeval;
      int ret;
      char *buffer = (char *)malloc(len + 22);
      do {
	time_t val = time(NULL);
	localtime_r(&val, &timeval);
	memcpy(buffer, fname, len);
	sprintf(buffer + len, "_%04u%02u%02u_%02u%02u%02u.mp4",
		1900 + timeval.tm_year, timeval.tm_mon + 1, timeval.tm_mday,
		timeval.tm_hour, timeval.tm_min, timeval.tm_sec);
	error_message("trying file %s", buffer);
	ret = stat(buffer, &stats);
	if (ret == 0) {
	  SDL_Delay(100);
	}
      } while (ret == 0);
      m_mp4FileName = strdup(buffer);
      create = true;
      break;
    }
  }
    // else fall through
    
  case FILE_MP4_OVERWRITE:
    m_mp4FileName = strdup(filename);
    create = true;
    break;
  }
  if (create) {
    if (m_stream &&
	(m_recordAudio == false ||
	 strcasecmp(m_audio_profile->GetStringValue(CFG_AUDIO_ENCODING), 
		    AUDIO_ENCODING_AMR) == 0) &&
	(m_recordVideo == false ||
	 strcasecmp(m_video_profile->GetStringValue(CFG_VIDEO_ENCODING), 
		    VIDEO_ENCODING_H263) == 0)) {
      static char* p3gppSupportedBrands[2] = {"3gp5", "3gp4"};

      m_mp4File = MP4CreateEx(m_mp4FileName,
			      verbosity,
			      createFlags,
			      1,
			      0,
			      p3gppSupportedBrands[0],
			      0x0001,
			      p3gppSupportedBrands,
			      NUM_ELEMENTS_IN_ARRAY(p3gppSupportedBrands));
    } else {
      m_mp4File = MP4Create(m_mp4FileName,
			    verbosity, createFlags);
    }
  }
    
  if (!m_mp4File) {
    return;
  }
  MP4SetTimeScale(m_mp4File, m_movieTimeScale);

  char buffer[80];
  sprintf(buffer, "mp4live version %s %s", MPEG4IP_VERSION, 
#ifdef HAVE_LINUX_VIDEODEV2_H
		      "V4L2"
#else
		      "V4L"
#endif
	  );
  MP4SetMetadataTool(m_mp4File, buffer);

  if (m_recordVideo) {
    m_videoFrameNumber = 1;
    if (m_stream == NULL) {
      m_videoTrackId = MP4AddVideoTrack(m_mp4File,
					m_videoTimeScale,
					MP4_INVALID_DURATION,
					m_pConfig->m_videoWidth, 
					m_pConfig->m_videoHeight,
					MP4_YUV12_VIDEO_TYPE);

      if (m_videoTrackId == MP4_INVALID_TRACK_ID) {
        error_message("can't create raw video track");
        goto start_failure;
      }
      m_videoFrameType = YUVVIDEOFRAME;
      MP4SetVideoProfileLevel(m_mp4File, 0xFF);
    } else {
      bool vIod, vIsma;
      uint8_t videoProfile;
      uint8_t *videoConfig;
      uint32_t videoConfigLen;
      uint8_t videoType;

      m_videoFrameType = 
        get_video_mp4_fileinfo(m_video_profile,
                               &vIod,
                               &vIsma,
                               &videoProfile,
                               &videoConfig,
                               &videoConfigLen,
                               &videoType);

      if (m_videoFrameType == H263VIDEOFRAME) {
	m_videoTrackId = MP4AddH263VideoTrack(m_mp4File,
					      m_videoTimeScale,
					      0,
					      m_video_profile->m_videoWidth,
					      m_video_profile->m_videoHeight,
					      10,
					      0,
					      0, 
					      0);
	uint32_t bitrate = 
	  m_video_profile->GetIntegerValue(CFG_VIDEO_BIT_RATE) * 1000;
	// may need to do this at the end
	MP4SetH263Bitrates(m_mp4File,
			   m_videoTrackId,
			   bitrate,
			   bitrate);
      } else {
	m_videoTrackId = MP4AddVideoTrack(m_mp4File,
					  m_videoTimeScale,
					  MP4_INVALID_DURATION,
					  m_video_profile->m_videoWidth, 
					  m_video_profile->m_videoHeight,
					  videoType);
	
	if (vIod == false) m_makeIod = false;
	if (vIsma == false) m_makeIsmaCompliant = false;
	
	if (m_videoTrackId == MP4_INVALID_TRACK_ID) {
	  error_message("can't create encoded video track");
	  goto start_failure;
	}
	
	MP4SetVideoProfileLevel(m_mp4File, 
				videoProfile);
      }

      if (videoConfigLen > 0) {
	MP4SetTrackESConfiguration(
				   m_mp4File, 
				   m_videoTrackId,
				   videoConfig,
				   videoConfigLen);
      }
    }
  }

  m_audioFrameNumber = 1;
  m_canRecordVideo = true;
  if (m_recordAudio) {
    if (m_stream == NULL) {
      // raw audio
      m_canRecordVideo = false;
      m_audioTrackId = MP4AddAudioTrack(m_mp4File, 
					m_audioTimeScale, 
					0,
					MP4_PCM16_BIG_ENDIAN_AUDIO_TYPE);

      if (m_audioTrackId == MP4_INVALID_TRACK_ID) {
        error_message("can't create raw audio track");
        goto start_failure;
      }

      MP4SetAudioProfileLevel(m_mp4File, 0xFF);
      m_audioFrameType = PCMAUDIOFRAME;
    } else {
      u_int8_t audioType;
      bool createIod = false;
      bool isma_compliant = false;
      uint8_t audioProfile;
      uint8_t *pAudioConfig;
      uint32_t audioConfigLen;
      m_canRecordVideo = false;
      m_audioFrameType = 
        get_audio_mp4_fileinfo(m_audio_profile, 
                               &createIod,
                               &isma_compliant,
                               &audioProfile,
                               &pAudioConfig,
                               &audioConfigLen,
                               &audioType);
					       
      if (m_audioFrameType == AMRNBAUDIOFRAME ||
	  m_audioFrameType == AMRWBAUDIOFRAME) {
	m_audioTrackId = 
	  MP4AddAmrAudioTrack(m_mp4File,
			      m_audioFrameType == AMRNBAUDIOFRAME ? 
			      8000 : 16000,
			      0, 
			      0, 
			      1, 
			      m_audioFrameType == AMRWBAUDIOFRAME);
      } else {
	if (createIod == false) m_makeIod = false;
	if (isma_compliant == false) 
	  m_makeIsmaCompliant = false;
	MP4SetAudioProfileLevel(m_mp4File, audioProfile);
	m_audioTrackId = MP4AddAudioTrack(
						 m_mp4File, 
						 m_audioTimeScale, 
						 MP4_INVALID_DURATION,
						 audioType);
      }

      if (m_audioTrackId == MP4_INVALID_TRACK_ID) {
        error_message("can't create encoded audio track");
        goto start_failure;
      }

      if (pAudioConfig) {
        MP4SetTrackESConfiguration(
                                   m_mp4File, 
                                   m_audioTrackId,
                                   pAudioConfig, 
                                   audioConfigLen);
	free(pAudioConfig);
      }
    }
  }
  
  debug_message("recording text %u", m_recordText);
  if (m_recordText) {
    m_textFrameNumber = 1;
  if (m_stream == NULL) {
      m_recordText = false;
    } else {
      m_textFrameType = get_text_mp4_fileinfo(m_text_profile);
      debug_message("text type %u", m_textFrameType);
      if (m_textFrameType == HREFTEXTFRAME) {
	m_textTrackId = MP4AddHrefTrack(m_mp4File, m_textTimeScale, MP4_INVALID_DURATION);
	debug_message("Added text track %u", m_textTrackId);
      } else {
	m_recordText = false;
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
  if (m_audioFrameType == AMRNBAUDIOFRAME ||
      m_audioFrameType == AMRWBAUDIOFRAME) {
    uint8_t decMode = *(uint8_t *)pFrame->GetData();
    m_amrMode |= 1 << ((decMode >> 3) & 0xf);
  }

    if (m_audioFrameNumber == 1) {
      m_audioStartTimestamp = pFrame->GetTimestamp();
      debug_message("record audio start "U64, m_audioStartTimestamp);
      m_canRecordVideo = true;
      m_prevAudioFrame = pFrame;
      m_audioSamples = 0;
      m_audioDiffTicks = 0;
      m_audioDiffTicksTotal = 0;
      m_audioFrameNumber++;
      return; // wait until the next audio frame
    }

    Duration thisFrameDurationInTicks = 
      pFrame->GetTimestamp() - m_prevAudioFrame->GetTimestamp();

    Duration thisFrameDurationInSamples =
      GetTimescaleFromTicks(thisFrameDurationInTicks, m_audioTimeScale);

    Duration elapsedTimeFromSamples;
    Duration elapsedTimeFromTimestamp;
    elapsedTimeFromTimestamp = pFrame->GetTimestamp() - m_audioStartTimestamp;

    if (thisFrameDurationInSamples > pFrame->GetDuration()) {
      // we have a gap frame
      // add the number of extra samples
      m_audioSamples += 
	(thisFrameDurationInSamples - pFrame->GetDuration());
      error_message("adding encoded "D64" samples", 
		    thisFrameDurationInSamples - pFrame->GetDuration());
    }
    // we have a consecutive frame
    // Timestamp of pFrame should reflect encoded audio samples
    // take difference
    m_audioSamples += pFrame->GetDuration();
    elapsedTimeFromSamples = 
      GetTicksFromTimescale(m_audioSamples, 0, 0, m_audioTimeScale);

    Duration diffTimeTicks;

    diffTimeTicks = elapsedTimeFromSamples - elapsedTimeFromTimestamp;

    // we need to adjust this by the amount of time we've already changed
    diffTimeTicks += m_audioDiffTicksTotal;

    if (diffTimeTicks != 0) {
      error_message("elfts "D64" samples "D64" elfs "D64" diff "D64,
		    elapsedTimeFromTimestamp,
		    m_audioSamples,
		    elapsedTimeFromSamples,
		    diffTimeTicks);
      
      error_message("adj "D64" diff "D64,
		    m_audioDiffTicksTotal, diffTimeTicks);
    }

    // diffTimeTicks is now the amount we need to add to the video frame
    m_audioDiffTicks = diffTimeTicks;

    m_audioDiffTicksTotal += diffTimeTicks;

    /*
     * we don't convert the audio duration any more
     *  m_prevEncodedAudioFrame->SetDuration(audioDurationInSamples);
     * Instead, we'll convert the video duration
     */
    MP4WriteSample(
                   m_mp4File,
                   m_audioTrackId,
                   (u_int8_t*)m_prevAudioFrame->GetData(), 
                   m_prevAudioFrame->GetDataLength(),
                   m_prevAudioFrame->ConvertDuration(m_audioTimeScale));

    m_audioFrameNumber++;
    if (m_prevAudioFrame->RemoveReference()) {
      delete m_prevAudioFrame;
    }
    m_prevAudioFrame = pFrame;
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
    bool isIFrame = false;
    uint8_t *pData, *pDataStart;
    uint32_t dataLen;

    if (m_videoFrameNumber == 1) {
      // wait until first audio frame before looking for the next I frame
      if (!m_canRecordVideo) {
        if (pFrame->RemoveReference()) delete pFrame;
        return;
      }

      // make sure this frame was captured after the first audio frame
      if (m_recordAudio &&
	  pFrame->GetTimestamp() < m_audioStartTimestamp) {
        if (pFrame->RemoveReference()) delete pFrame;
        return;
      }

      dataLen = pFrame->GetDataLength();
      pDataStart = (uint8_t *)pFrame->GetData();
      if (pFrame->GetType() == MPEG4VIDEOFRAME) {
	pData = MP4AV_Mpeg4FindVop(pDataStart, dataLen);
	if (pData == NULL) {
	  error_message("Couldn't find vop header");
	  if (pFrame->RemoveReference()) delete pFrame;
	  return;
	}
	int voptype =
	    MP4AV_Mpeg4GetVopType(pData,
				  dataLen - (pData - pDataStart));
	if (voptype != VOP_TYPE_I) {
	  debug_message(U64" wrong vop type %d %02x %02x %02x %02x %02x", 
			pFrame->GetTimestamp(),
			voptype,
			pData[0],
			pData[1],
			pData[2],
			pData[3],
			pData[4]);
	  if (pFrame->RemoveReference()) delete pFrame;
	  return;
	}
      } else if (pFrame->GetType() == H263VIDEOFRAME) {
	// wait for an i frame
	if ((pDataStart[4] & 0x02) != 0) {
	  if (pFrame->RemoveReference()) delete pFrame;
	  return;
	}
      } else {
	// MPEG2 video
	int ret, ftype;
	ret = MP4AV_Mpeg3FindPictHdr(pDataStart, dataLen, &ftype);
	if (ret < 0 || ftype != 1) {
	  if (pFrame->RemoveReference()) delete pFrame;
	  return;
	}
      }

      debug_message("Video start ts "U64, pFrame->GetTimestamp());
	if (m_recordAudio) {
	// reset this timestamp to video's beginning
	pFrame->SetTimestamp(m_audioStartTimestamp);
      } 
      m_videoStartTimestamp = pFrame->GetTimestamp();
      m_prevVideoFrame = pFrame;
      m_videoFrameNumber++;
      m_videoDurationTimescale = 0;
      return; // wait until the next video frame
    }

    Duration videoDurationInTicks;

    videoDurationInTicks = 
      pFrame->GetTimestamp() - m_videoStartTimestamp;

    // at this point, we'll probably want to add in the audio drift values, 
    // if there are any
    Duration videoDurationInTimescaleTotal;
    videoDurationInTimescaleTotal = 
      GetTimescaleFromTicks(videoDurationInTicks, m_videoTimeScale);

    Duration videoDurationInTimescaleFrame;
    videoDurationInTimescaleFrame = 
      videoDurationInTimescaleTotal - m_videoDurationTimescale;

    m_videoDurationTimescale += videoDurationInTimescaleFrame;
#if 0
    debug_message("vdit "D64" vdits "D64" frame "D64" total "D64" "D64,
		  videoDurationInTicks, videoDurationInTimescaleTotal,
		  videoDurationInTimescaleFrame, m_videoDurationTimescale,
		  GetTicksFromTimescale(m_videoDurationTimescale, 0, 0, m_videoTimeScale));
#endif
    dataLen = m_prevVideoFrame->GetDataLength();

    
    Duration rend_offset = 0;
    if (pFrame->GetType() == MPEG4VIDEOFRAME) {
      pData = MP4AV_Mpeg4FindVop((uint8_t *)m_prevVideoFrame->GetData(),
			       dataLen);
      if (pData) {
	dataLen -= (pData - (uint8_t *)m_prevVideoFrame->GetData());
	isIFrame =
	  (MP4AV_Mpeg4GetVopType(pData,dataLen) == VOP_TYPE_I);
#if 0
	debug_message("record type %d %02x %02x %02x %02x",
		      MP4AV_Mpeg4GetVopType(pData, dataLen),
		      pData[0],
		      pData[1],
		      pData[2],
		      pData[3]);
#endif
      } else {
	pData = (uint8_t *)m_prevVideoFrame->GetData();
      }
    } else if (pFrame->GetType() == H263VIDEOFRAME) {
      pData = (uint8_t *)m_prevVideoFrame->GetData();
      isIFrame = ((pData[4] & 0x02) == 0);
#if 0
      isIFrame =
	(MP4AV_Mpeg4GetVopType(pData,dataLen) == VOP_TYPE_I);
      debug_message("frame %02x %02x %02x %d", pData[2], pData[3], pData[4],
		    isIFrame);
#endif
    } else {
      // mpeg2
      int ret, ftype;
      pData = (uint8_t *)m_prevVideoFrame->GetData();
      ret = MP4AV_Mpeg3FindPictHdr(pData, dataLen, &ftype);
      isIFrame = false;
      if (ret >= 0) {
	if (ftype == 1) isIFrame = true;
	if (ftype != 3) {
	  rend_offset = m_prevVideoFrame->GetPtsTimestamp() - 
	    m_prevVideoFrame->GetTimestamp();
	  rend_offset = GetTimescaleFromTicks(rend_offset, m_movieTimeScale);
	}
      }
    }

    MP4WriteSample(
		   m_mp4File,
		   m_videoTrackId,
		   pData,
		   dataLen,
		   videoDurationInTimescaleFrame,
		   rend_offset,
		   isIFrame);
		
    m_videoFrameNumber++;
    if (m_prevVideoFrame->RemoveReference()) {
      delete m_prevVideoFrame;
    }
    m_prevVideoFrame = pFrame;
}

void CMp4Recorder::ProcessEncodedTextFrame (CMediaFrame *pFrame)
{
  if (m_textFrameNumber == 1) {
    if (m_prevTextFrame != NULL) {
      if (m_prevTextFrame->RemoveReference()) {
	delete m_prevTextFrame;
      }
    }
    m_prevTextFrame = pFrame;

    if (m_canRecordVideo == false) return;

    if (m_recordAudio) {
    // we can record.  Set the timestamp of this frame to the audio start timestamp
      m_prevTextFrame->SetTimestamp(m_audioStartTimestamp);
      m_textStartTimestamp = m_audioStartTimestamp;
    } else {
      // need to work with video here, as well...
      m_textStartTimestamp = m_prevTextFrame->GetTimestamp();
    }
    m_textFrameNumber++;
    m_textDurationTimescale = 0;
    return;
  }

  Duration textDurationInTicks;
  textDurationInTicks = pFrame->GetTimestamp() - m_textStartTimestamp;
  
  Duration textDurationInTimescaleTotal;
  textDurationInTimescaleTotal = 
    GetTimescaleFromTicks(textDurationInTicks, m_textTimeScale);

  Duration textDurationInTimescaleFrame;
  textDurationInTimescaleFrame = 
    textDurationInTimescaleTotal - m_textDurationTimescale;

  m_textDurationTimescale += textDurationInTimescaleFrame;

  MP4WriteSample(m_mp4File, m_textTrackId,
		 (uint8_t *)m_prevTextFrame->GetData(),
		 m_prevTextFrame->GetDataLength(),
		 textDurationInTimescaleFrame);
  debug_message("wrote text frame %u", m_textFrameNumber);

  m_textFrameNumber++;
  if (m_prevTextFrame->RemoveReference()) {
    delete m_prevTextFrame;
  }
  m_prevTextFrame = pFrame;
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
  //debug_message("type %d ts "U64, pFrame->GetType(), pFrame->GetTimestamp());
  // RAW AUDIO
  if (pFrame->GetType() == PCMAUDIOFRAME && m_recordAudio) {
    if (m_audioFrameNumber == 1) {

      debug_message("First raw audio frame at "U64, pFrame->GetTimestamp());

      m_audioStartTimestamp = pFrame->GetTimestamp();
      m_canRecordVideo = true;
      m_prevAudioFrame = pFrame;
      m_audioFrameNumber++;
      m_audioSamples = 0;
      return; // wait until the next audio frame
    }

#if 0
    Duration audioFrameSamples = 
      m_prevAudioFrame->GetDatalength() /
      (m_pConfig->GetIntegerValue(CONFIG_AUDIO_CHANNELS) * sizeof(int16_t));

    m_audioSamples += ======
#endif



    Duration audioDurationInTicks = 
      pFrame->GetTimestamp() - m_prevAudioFrame->GetTimestamp();

    MP4Duration audioDurationInSamples =
      MP4ConvertToTrackDuration(m_mp4File, m_audioTrackId,
                                audioDurationInTicks, TimestampTicks);
#if 0
    debug_message("prev "U64" this "U64" diff samples"U64,
		  m_prevAudioFrame->GetTimestamp(),
		  pFrame->GetTimestamp(),
		  audioDurationInSamples);
#endif
		  
    m_prevAudioFrame->SetDuration(audioDurationInSamples);
    void *pcm;
#ifdef WORDS_BIGENDIAN
    pcm = m_prevAudioFrame->GetData();
#else
    uint32_t convert_size = m_prevAudioFrame->GetDataLength();
    uint16_t *pdata = (uint16_t *)m_prevAudioFrame->GetData();

    if (m_convert_pcm_size < convert_size) {
      m_convert_pcm = (uint16_t *)realloc(m_convert_pcm, convert_size);
      m_convert_pcm_size = convert_size;
    }
    convert_size /= sizeof(uint16_t);
    for (uint32_t ix = 0; ix < convert_size; ix++) {
      uint16_t swap = *pdata++;
      m_convert_pcm[ix] = B2N_16(swap);
    }
    pcm = m_convert_pcm;
#endif
    MP4WriteSample(
                   m_mp4File,
                   m_audioTrackId,
                   (u_int8_t*)pcm,
                   m_prevAudioFrame->GetDataLength(),
		   audioDurationInSamples);
    //                   m_prevAudioFrame->ConvertDuration(m_audioTimeScale));

    m_audioFrameNumber++;
    if (m_prevAudioFrame->RemoveReference()) {
      delete m_prevAudioFrame;
    }
    m_prevAudioFrame = pFrame;

    // ENCODED AUDIO
  } else if (pFrame->GetType() == m_audioFrameType && m_recordAudio) {

    ProcessEncodedAudioFrame(pFrame);
    // RAW VIDEO
  } else if (pFrame->GetType() == YUVVIDEOFRAME && m_recordVideo) {
    // we drop raw video frames until we get the first raw audio frame
    // after that:
    // if we are also recording encoded video, we wait until the first I frame
    // else
    // we wait until the next raw video frame
    // in both cases, the duration of the first raw video frame is stretched
    // to the start of the first raw audio frame
    
    if (m_videoFrameNumber == 1) {
      // wait until the first raw audio frame is received
      if (!m_canRecordVideo) {
        if (pFrame->RemoveReference()) delete pFrame;
        return;
      }

      // make sure this frame was captured after the first audio frame
      if (m_recordAudio &&
	  pFrame->GetTimestamp() < m_audioStartTimestamp) {
        if (pFrame->RemoveReference()) delete pFrame;
        return;
      }

      debug_message("First raw video frame at "U64, pFrame->GetTimestamp());

      m_videoStartTimestamp = pFrame->GetTimestamp();
      m_prevVideoFrame = pFrame;
      m_videoFrameNumber++;
      return; // wait until the next video frame
    }

    Duration videoDurationInTicks;
    
    // the first raw video frame is stretched to the begining
    // of the first raw audio frame
    if (m_videoFrameNumber == 2) {
      videoDurationInTicks =
        pFrame->GetTimestamp() - m_audioStartTimestamp;
    } else {
      videoDurationInTicks =
        pFrame->GetTimestamp() - m_prevVideoFrame->GetTimestamp();
    }

    m_prevVideoFrame->SetDuration(videoDurationInTicks);
    yuv_media_frame_t *pYUV = (yuv_media_frame_t *)m_prevVideoFrame->GetData();
    if (pYUV->y_stride != m_pConfig->m_videoWidth) {
      error_message("Need to handle different strides in record raw");
    } else {
      MP4WriteSample(
		     m_mp4File,
		     m_videoTrackId,
		     pYUV->y,
		     m_pConfig->m_yuvSize,
		     m_prevVideoFrame->ConvertDuration(m_videoTimeScale));
    }

    m_videoFrameNumber++;
    if (m_prevVideoFrame->RemoveReference()) {
      delete m_prevVideoFrame;
    }
    m_prevVideoFrame = pFrame;

    // ENCODED VIDEO
  } else if (pFrame->GetType() == m_videoFrameType && m_recordVideo) {

    ProcessEncodedVideoFrame(pFrame);
  } else if (pFrame->GetType() == m_textFrameType && m_recordText) {
    ProcessEncodedTextFrame(pFrame);
  } else {    // degenerate case
    if (pFrame->RemoveReference()) delete pFrame;
  }
}

void CMp4Recorder::DoStopRecord()
{
  if (!m_sink) return;

  Duration totalAudioDuration = 0;

  // write last audio frame
  if (m_prevAudioFrame) {
    MP4WriteSample(
                   m_mp4File,
                   m_audioTrackId,
                   (u_int8_t*)m_prevAudioFrame->GetData(), 
                   m_prevAudioFrame->GetDataLength(),
                   m_prevAudioFrame->ConvertDuration(m_audioTimeScale));
    m_audioSamples += m_prevAudioFrame->GetDuration();

    totalAudioDuration = m_audioSamples;
    totalAudioDuration *= TimestampTicks;
    totalAudioDuration /= m_audioTimeScale;

    if (m_prevAudioFrame->RemoveReference()) {
      delete m_prevAudioFrame;
    }
    if (m_audioFrameType == AMRNBAUDIOFRAME ||
	m_audioFrameType == AMRWBAUDIOFRAME) {
      MP4SetAmrModeSet(m_mp4File, m_audioTrackId, m_amrMode);
    }
  }
 
  // write last video frame
  if (m_prevVideoFrame) {
    if (m_stream == NULL) {
#if 1
      error_message("write last raw video frame");
#else
      MP4WriteSample(
		     m_mp4File,
		     m_videoTrackId,
		     (u_int8_t*)m_prevVideoFrame->GetData(), 
		     m_prevVideoFrame->GetDataLength(),
		     m_prevVideoFrame->ConvertDuration(m_videoTimeScale));
#endif
      if (m_prevVideoFrame->RemoveReference()) {
	delete m_prevVideoFrame;
      }
    } else {
      bool isIFrame;
      Duration rend_offset = 0;
      
      if (m_prevVideoFrame->GetType() == MPEG4VIDEOFRAME ||
	  m_prevVideoFrame->GetType() == H263VIDEOFRAME) {
	isIFrame = 
	  (MP4AV_Mpeg4GetVopType(
				 (u_int8_t*) m_prevVideoFrame->GetData(),
				 m_prevVideoFrame->GetDataLength()) == VOP_TYPE_I);
      } else {
	int ret, ftype;
	ret = 
	  MP4AV_Mpeg3FindPictHdr((u_int8_t *)m_prevVideoFrame->GetData(), 
				 m_prevVideoFrame->GetDataLength(),
				 &ftype);
	isIFrame = false;
	if (ret >= 0) {
	  if (ftype == 1) isIFrame = true;
	  if (ftype != 3) {
	    rend_offset = m_prevVideoFrame->GetPtsTimestamp() - 
	      m_prevVideoFrame->GetTimestamp();
	    rend_offset = GetTimescaleFromTicks(rend_offset, m_movieTimeScale);
	  }
	}
      }

      MP4WriteSample(
		     m_mp4File,
		     m_videoTrackId,
		     (u_int8_t*) m_prevVideoFrame->GetData(),
		     m_prevVideoFrame->GetDataLength(),
		     m_prevVideoFrame->ConvertDuration(m_videoTimeScale),
		     rend_offset,
		     isIFrame);
      if (m_prevVideoFrame->RemoveReference()) {
	delete m_prevVideoFrame;
      }
    }
  }

  if (m_prevTextFrame) {
    Duration lastDuration = TimestampTicks;

    if (totalAudioDuration != 0) {
      Duration totalSoFar = m_prevTextFrame->GetTimestamp() - m_textStartTimestamp;
      if (totalAudioDuration > totalSoFar) {
	lastDuration = totalAudioDuration - totalSoFar;
      }
    }
    debug_message("last duration "U64" timescale "U64,
		  lastDuration, GetTimescaleFromTicks(lastDuration, m_textTimeScale));
    MP4WriteSample(m_mp4File, 
		   m_textTrackId, 
		   (uint8_t *)m_prevTextFrame->GetData(),
		   m_prevTextFrame->GetDataLength(),
		   GetTimescaleFromTicks(lastDuration, m_textTimeScale));
    if (m_prevTextFrame->RemoveReference()) {
      delete m_prevTextFrame;
    }
    m_prevTextFrame = NULL;
  }
    
    
  bool optimize = false;

  // create hint tracks
  if (m_pConfig->GetBoolValue(CONFIG_RECORD_MP4_HINT_TRACKS)) {

    if (m_pConfig->GetBoolValue(CONFIG_RECORD_MP4_OPTIMIZE)) {
      optimize = true;
    }

    if (m_stream != NULL) {
      if (MP4_IS_VALID_TRACK_ID(m_videoTrackId) && m_stream) {
	create_mp4_video_hint_track(m_video_profile,
				    m_mp4File, 
				    m_videoTrackId,
				    m_pConfig->GetIntegerValue(CONFIG_RTP_PAYLOAD_SIZE));
      }

      if (MP4_IS_VALID_TRACK_ID(m_audioTrackId)) {
	create_mp4_audio_hint_track(m_audio_profile, 
				    m_mp4File, 
				    m_audioTrackId,
				    m_pConfig->GetIntegerValue(CONFIG_RTP_PAYLOAD_SIZE));
      }
      if (MP4_IS_VALID_TRACK_ID(m_textTrackId)) {
	create_mp4_text_hint_track(m_text_profile, 
				   m_mp4File, 
				   m_textTrackId,
				   m_pConfig->GetIntegerValue(CONFIG_RTP_PAYLOAD_SIZE));
      }
    } else {
      if (MP4_IS_VALID_TRACK_ID(m_audioTrackId)) {
	L16Hinter(m_mp4File, 
		  m_audioTrackId,
		  m_pConfig->GetIntegerValue(CONFIG_RTP_PAYLOAD_SIZE));
      }
    }
  }
  // close the mp4 file
  MP4Close(m_mp4File);
  m_mp4File = NULL;

  // add ISMA style OD and Scene tracks
  if (m_stream != NULL) {

    bool useIsmaTag = false;

    // if AAC track is present, can tag this as ISMA compliant content
    useIsmaTag = m_makeIsmaCompliant;
    if (m_makeIod) {
      MP4MakeIsmaCompliant(
			   m_mp4FileName,
                           0,
                           useIsmaTag);
    }
  }

  if (optimize) {
    MP4Optimize(m_mp4FileName);
  }

  m_sink = false;
}

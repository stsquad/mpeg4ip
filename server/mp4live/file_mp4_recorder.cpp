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
#include "mp4av_h264.h"
#include "video_v4l_source.h"
//#define DEBUG_H264 1

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
  CHECK_AND_FREE(m_videoTempBuffer);
  m_videoTempBufferSize = 0;
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
    m_recordVideo = m_pConfig->GetBoolValue(CONFIG_VIDEO_ENABLE) &&
      m_pConfig->GetBoolValue(CONFIG_RECORD_RAW_IN_MP4_VIDEO);
    m_recordAudio = m_pConfig->GetBoolValue(CONFIG_AUDIO_ENABLE) &&
      m_pConfig->GetBoolValue(CONFIG_RECORD_RAW_IN_MP4_AUDIO);
    m_recordText = false;
    m_audioTimeScale = m_pConfig->GetIntegerValue(CONFIG_AUDIO_SAMPLE_RATE);
    filename = m_pConfig->GetStringValue(CONFIG_RECORD_RAW_MP4_FILE_NAME);
  }

  if (m_recordAudio &&
      m_pConfig->GetBoolValue(CONFIG_RECORD_MP4_VIDEO_TIMESCALE_USES_AUDIO))
    m_videoTimeScale = m_audioTimeScale;

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
    MP4_DETAILS_ERROR  /*DEBUG  | MP4_DETAILS_WRITE_ALL */;

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
	  get_linux_video_type());
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
      } else if (m_videoFrameType == H264VIDEOFRAME) {
	uint8_t avcprofile, profile_compat, avclevel;
	avcprofile = (m_video_profile->m_videoMpeg4ProfileId >> 16) & 0xff;
	profile_compat = (m_video_profile->m_videoMpeg4ProfileId >> 8) & 0xff;
	avclevel = (m_video_profile->m_videoMpeg4ProfileId) & 0xff;
	debug_message("h264 track %x %x %x", 
		      avcprofile, profile_compat, avclevel);
	m_videoTrackId = MP4AddH264VideoTrack(m_mp4File, 
					      m_videoTimeScale,
					      MP4_INVALID_DURATION,
					      m_video_profile->m_videoWidth,
					      m_video_profile->m_videoHeight,
					      avcprofile, 
					      profile_compat, 
					      avclevel, 
					      3);

	MP4SetVideoProfileLevel(m_mp4File, 0x7f);
	m_makeIod = false;
	m_makeIsmaCompliant = false;
      } else if (m_videoFrameType == H261VIDEOFRAME) {
	error_message("H.261 recording is not supported");
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

	if (videoConfigLen > 0) {
	  MP4SetTrackESConfiguration(
				     m_mp4File, 
				     m_videoTrackId,
				     videoConfig,
				     videoConfigLen);
	}
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
      const char *url;
      m_textFrameType = get_text_mp4_fileinfo(m_text_profile, &url);
      debug_message("text type %u", m_textFrameType);
      if (m_textFrameType == HREFTEXTFRAME) {
	m_textTrackId = MP4AddHrefTrack(m_mp4File, m_textTimeScale, MP4_INVALID_DURATION, url);
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
static uint32_t write_sei (uint8_t *to, const uint8_t *from, uint32_t len)
{
  uint8_t *nal_write = to + 4;
  uint32_t written_bytes = 1;
#ifdef DEBUG_H264
  debug_message("sei start %u", len);
#endif
  *nal_write++ = *from++;
  len--;
  
  uint32_t payload_type, payload_size, typesize, sizesize;
  while (len >= 2 && *from != 0x80) {
    payload_type = h264_read_sei_value(from, &typesize);
    payload_size = h264_read_sei_value(from + typesize, &sizesize);
#ifdef DEBUG_H264
    debug_message("type %u sizes %u", payload_type, payload_size);
#endif
    uint32_t size = typesize + sizesize + payload_size;
    if (size <= len) {
      switch (payload_type) {
      case 3:
      case 10:
      case 11:
      case 12:
	// need to skip these
	break;
      default:
	memmove(nal_write, from, size);
	nal_write += size;
	written_bytes += size;
      }
      from += size;
      len -= size;
    } else {
      len = 0;
    }
  }
  if (written_bytes <= 1) {
    return 0;
  } 
  to[0] = (written_bytes >> 24) & 0xff;
  to[1] = (written_bytes >> 16) & 0xff;
  to[2] = (written_bytes >> 8) & 0xff;
  to[3] = written_bytes & 0xff;
#ifdef DEBUG_H264
  debug_message("sei - %u bytes", written_bytes + 4);
#endif
  return written_bytes + 4;
}

void CMp4Recorder::WriteH264Frame (CMediaFrame *pFrame,
				   Duration dur)
{
  bool isIFrame = false;
  Duration rend_offset = 0;
  rend_offset = pFrame->GetPtsTimestamp() - 
    pFrame->GetTimestamp();
  rend_offset = GetTimescaleFromTicks(rend_offset, m_movieTimeScale);

  h264_media_frame_t *mf = (h264_media_frame_t *)pFrame->GetData();
  
  uint32_t size = mf->buffer_len + (4 * mf->nal_number);
  if (size > m_videoTempBufferSize) {
    m_videoTempBuffer = (uint8_t *)realloc(m_videoTempBuffer, size);
    m_videoTempBufferSize = size;
  }
  uint32_t len_written = 0;
  for (uint32_t ix = 0; ix < mf->nal_number; ix++) {
    bool write_it = false;
    switch (mf->nal_bufs[ix].nal_type) {
    case H264_NAL_TYPE_SEI:
      len_written += write_sei(m_videoTempBuffer + len_written,
			       mf->buffer + mf->nal_bufs[ix].nal_offset,
			       mf->nal_bufs[ix].nal_length);
      break;
    case H264_NAL_TYPE_SEQ_PARAM: 
      if (mf->nal_bufs[ix].nal_length != m_videoH264SeqSize ||
	  (m_videoH264Seq != NULL &&
	   memcmp(m_videoH264Seq, 
		  mf->buffer + mf->nal_bufs[ix].nal_offset,
		  m_videoH264SeqSize) != 0)) {
	m_videoH264SeqSize = mf->nal_bufs[ix].nal_length;
	m_videoH264Seq = 
	  (uint8_t *)realloc(m_videoH264Seq, m_videoH264SeqSize);
	memcpy(m_videoH264Seq, 
	       mf->buffer + mf->nal_bufs[ix].nal_offset,
	       m_videoH264SeqSize);
	MP4AddH264SequenceParameterSet(m_mp4File,
				       m_videoTrackId,
				       m_videoH264Seq,
				       m_videoH264SeqSize);
	debug_message("writing seq parameter %u",mf->nal_bufs[ix].nal_length);
      }
      break;
    case H264_NAL_TYPE_PIC_PARAM:
      if (mf->nal_bufs[ix].nal_length != m_videoH264PicSize ||
	  (m_videoH264Pic != NULL &&
	   memcmp(m_videoH264Pic, 
		  mf->buffer + mf->nal_bufs[ix].nal_offset,
		  m_videoH264PicSize) != 0)) {
	m_videoH264PicSize = mf->nal_bufs[ix].nal_length;
	m_videoH264Pic = 
	  (uint8_t *)realloc(m_videoH264Pic, m_videoH264PicSize);
	memcpy(m_videoH264Pic, 
	       mf->buffer + mf->nal_bufs[ix].nal_offset,
	       m_videoH264PicSize);
	MP4AddH264PictureParameterSet(m_mp4File,
				      m_videoTrackId, 
				      m_videoH264Pic,
				      m_videoH264PicSize);
	debug_message("writing pic parameter %u", mf->nal_bufs[ix].nal_length);
      }
      break;
    case H264_NAL_TYPE_IDR_SLICE:
      isIFrame = true;
      // fall through
    case H264_NAL_TYPE_NON_IDR_SLICE:
    case H264_NAL_TYPE_DP_A_SLICE:
    case H264_NAL_TYPE_DP_B_SLICE:
    case H264_NAL_TYPE_DP_C_SLICE:
#if 0
      if (m_videoFrameNumber % 7 == 0) {
	debug_message("drop frame");
	return;
      }
#endif
      write_it = true;
      break;
    case H264_NAL_TYPE_FILLER_DATA:
      write_it = false;
    default:
      write_it = true;
      break;
    }
#ifdef DEBUG_H264
    debug_message("%u h264 nal %d type %d %u write %d", 
		  m_videoFrameNumber, ix, mf->nal_bufs[ix].nal_type, 
		  mf->nal_bufs[ix].nal_length,
		  write_it);
#endif
    if (write_it) {
      // write length.
      uint32_t to_write;
      to_write = mf->nal_bufs[ix].nal_length;
      m_videoTempBuffer[len_written] = (to_write >> 24) & 0xff;
      m_videoTempBuffer[len_written + 1] = (to_write >> 16) & 0xff;
      m_videoTempBuffer[len_written + 2] = (to_write >> 8) & 0xff;
      m_videoTempBuffer[len_written + 3] = to_write & 0xff;
      len_written += 4;
      memcpy(m_videoTempBuffer + len_written,
	     mf->buffer + mf->nal_bufs[ix].nal_offset,
	     to_write);
      len_written += to_write;
    }
  }
#ifdef DEBUG_H264
  debug_message("%u h264 write %u", m_videoFrameNumber, len_written);
#endif
  MP4WriteSample(m_mp4File, 
		 m_videoTrackId,
		 m_videoTempBuffer, 
		 len_written,
		 dur, 
		 rend_offset, 
		 isIFrame);
}

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
      } else if (pFrame->GetType() == H264VIDEOFRAME) {
	h264_media_frame_t *mf = (h264_media_frame_t *)pFrame->GetData();
	bool found_idr = false;
	for (uint32_t ix = 0;
	     found_idr == false && ix < mf->nal_number;
	     ix++) {
	  found_idr = mf->nal_bufs[ix].nal_type == H264_NAL_TYPE_IDR_SLICE;
	}
#ifdef DEBUG_H264
	debug_message("h264 nals %d found %d", 
		      mf->nal_number, found_idr);
#endif
	if (found_idr == false) {
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

    
    if (pFrame->GetType() == H264VIDEOFRAME) {
      WriteH264Frame(m_prevVideoFrame, videoDurationInTimescaleFrame);
    } else {
      Duration rend_offset = 0;
      if (pFrame->GetType() == MPEG4VIDEOFRAME) {
	pData = MP4AV_Mpeg4FindVop((uint8_t *)m_prevVideoFrame->GetData(),
				   dataLen);
	if (pData) {
	  dataLen -= (pData - (uint8_t *)m_prevVideoFrame->GetData());
	  int vop_type = MP4AV_Mpeg4GetVopType(pData,dataLen);
	  isIFrame = (vop_type == VOP_TYPE_I);
	  rend_offset = m_prevVideoFrame->GetPtsTimestamp() - 
	    m_prevVideoFrame->GetTimestamp();
	  if (rend_offset != 0 && vop_type != VOP_TYPE_B) {
	    rend_offset = GetTimescaleFromTicks(rend_offset, m_movieTimeScale);
	  }
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
#if 1
	    rend_offset = m_prevVideoFrame->GetPtsTimestamp() - 
	      m_prevVideoFrame->GetTimestamp();
	    rend_offset = GetTimescaleFromTicks(rend_offset, m_movieTimeScale);
#else
	    rend_offset = 
	      GetTimescaleFromTicks(m_prevVideoFrame->GetPtsTimestamp(),
				    m_movieTimeScale);
	    rend_offset -= 
	      GetTimescaleFromTicks(m_prevVideoFrame->GetTimestamp(),
				    m_movieTimeScale);
#endif
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
    }
		
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
  // RAW AUDIO
  if (m_recordAudio) {
    if ((m_stream == NULL && pFrame->GetType() == PCMAUDIOFRAME) ||
	(m_stream != NULL && pFrame->GetType() == NETPCMAUDIOFRAME)) {
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
      if (pFrame->GetType() != NETPCMAUDIOFRAME) {
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
      } else {
	pcm = m_prevAudioFrame->GetData();
      }
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
      return;
      // ENCODED AUDIO
    } else if (pFrame->GetType() == m_audioFrameType) {

      ProcessEncodedAudioFrame(pFrame);
      return;
      // RAW VIDEO
    } 
  }

  if (m_recordVideo) {
    if (m_stream == NULL && pFrame->GetType() == YUVVIDEOFRAME) {
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
      if (m_videoFrameNumber == 2 && m_recordAudio) {
	videoDurationInTicks =
	  pFrame->GetTimestamp() - m_audioStartTimestamp;
      } else {
	videoDurationInTicks =
	  pFrame->GetTimestamp() - m_prevVideoFrame->GetTimestamp();
      }

      m_prevVideoFrame->SetDuration(videoDurationInTicks);
      yuv_media_frame_t *pYUV = (yuv_media_frame_t *)m_prevVideoFrame->GetData();
      if (pYUV->y + m_pConfig->m_ySize == pYUV->u) {
	MP4WriteSample(
		       m_mp4File,
		       m_videoTrackId,
		       pYUV->y,
		       m_pConfig->m_yuvSize,
		       m_prevVideoFrame->ConvertDuration(m_videoTimeScale));
      } else {
	if (m_rawYUV == NULL) {
	  debug_message("Mallocing %u", m_pConfig->m_yuvSize);
	  m_rawYUV = (uint8_t *)malloc(m_pConfig->m_yuvSize);
	}
	CopyYuv(pYUV->y, pYUV->u, pYUV->v,
		pYUV->y_stride, pYUV->uv_stride, pYUV->uv_stride,
		m_rawYUV, 
		m_rawYUV + m_pConfig->m_ySize, 
		m_rawYUV + m_pConfig->m_ySize + m_pConfig->m_uvSize,
		m_pConfig->m_videoWidth, 
		m_pConfig->m_videoWidth / 2, 
		m_pConfig->m_videoWidth / 2,
		m_pConfig->m_videoWidth, m_pConfig->m_videoHeight);
	MP4WriteSample(m_mp4File, 
		       m_videoTrackId, 
		       m_rawYUV,
		       m_pConfig->m_yuvSize,
		       m_prevVideoFrame->ConvertDuration(m_videoTimeScale));
      }
	      

      m_videoFrameNumber++;
      if (m_prevVideoFrame->RemoveReference()) {
	delete m_prevVideoFrame;
      }
      m_prevVideoFrame = pFrame;
      return;
      // ENCODED VIDEO
    } else if (pFrame->GetType() == m_videoFrameType) {

      ProcessEncodedVideoFrame(pFrame);
      return;
    }
  }
  if (pFrame->GetType() == m_textFrameType && m_recordText) {
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
      CHECK_AND_FREE(m_rawYUV);
    } else {
      bool isIFrame;
      Duration rend_offset = 0;

      if (m_prevVideoFrame->GetType() == H264VIDEOFRAME) {
	WriteH264Frame(m_prevVideoFrame, 
		       m_prevVideoFrame->ConvertDuration(m_videoTimeScale));
      } else {
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
      }
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
    
  CHECK_AND_FREE(m_videoH264Seq);
  m_videoH264SeqSize = 0;
  CHECK_AND_FREE(m_videoH264Pic);
  m_videoH264PicSize = 0;

  // close the mp4 file
  MP4Close(m_mp4File);
  m_mp4File = NULL;
    
  debug_message("done with writing last frame");
  bool optimize = false;

  // create hint tracks
  if (m_pConfig->GetBoolValue(CONFIG_RECORD_MP4_HINT_TRACKS)) {

    m_mp4File = MP4Modify(m_mp4FileName, MP4_DETAILS_ERROR);

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
    MP4Close(m_mp4File);
    m_mp4File = NULL;
  }
  debug_message("done with hint");

  // add ISMA style OD and Scene tracks
  if (m_stream != NULL) {
    if (m_pConfig->GetBoolValue(CONFIG_RECORD_MP4_ISMA_COMPLIANT)) {
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
  }

  if (optimize) {
    MP4Optimize(m_mp4FileName);
  }

  m_sink = false;
}

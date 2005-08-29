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

#ifndef __FILE_MP4_RECORDER_H__
#define __FILE_MP4_RECORDER_H__

#include <mp4.h>
#include <mp4av.h>
#include "media_sink.h"
#include "media_stream.h"

class CMp4Recorder : public CMediaSink {
public:
  CMp4Recorder(CMediaStream *stream) {
    Init();
    m_stream = stream;
    m_videoTempBuffer = NULL;
    m_videoTempBufferSize = 0;
    m_rawYUV = NULL;
  };

  const char *GetRecordFileName(void) {
    if (m_mp4FileName == NULL) {
      return m_pConfig->GetStringValue(CONFIG_RECORD_RAW_MP4_FILE_NAME);
    }
    return m_mp4FileName;
  };
protected:
   void Init (void) {
    m_mp4File = NULL;

    m_prevVideoFrame = NULL;
    m_prevAudioFrame = NULL;

    m_audioTrackId = MP4_INVALID_TRACK_ID;
    m_videoTrackId = MP4_INVALID_TRACK_ID;
    m_textTrackId = MP4_INVALID_TRACK_ID;

    m_videoTimeScale = 90000;
    m_textTimeScale = 90000;
    m_movieTimeScale = m_videoTimeScale;

    m_canRecordVideo = false;
    m_mp4FileName = NULL;
    m_amrMode = 0;
    m_stream = NULL;
#ifndef WORDS_BIGENDIAN
    m_convert_pcm = NULL;
    m_convert_pcm_size = 0;
#endif
    m_videoH264Seq = NULL;
    m_videoH264SeqSize = 0;
    m_videoH264Pic = NULL;
    m_videoH264PicSize = 0;

   };

  int ThreadMain(void);

  void DoStartRecord(void);
  void DoStopRecord(void);
  void DoWriteFrame(CMediaFrame* pFrame);

protected:
  CMediaStream *m_stream;
  CVideoProfile *m_video_profile;
  CAudioProfile *m_audio_profile;
  CTextProfile *m_text_profile;
  bool			m_canRecordVideo;

  const char*		m_mp4FileName;
  MP4FileHandle	m_mp4File;

  CMediaFrame*          m_prevVideoFrame;
  CMediaFrame*          m_prevAudioFrame;
  CMediaFrame*          m_prevTextFrame;

  uint8_t              *m_rawYUV;
  u_int32_t		m_movieTimeScale;
  u_int32_t		m_videoTimeScale;
  u_int32_t		m_audioTimeScale;
  u_int32_t             m_textTimeScale;

  bool                  m_recordVideo;
  MP4TrackId		m_videoTrackId;
  u_int32_t		m_videoFrameNumber;
  Timestamp		m_videoStartTimestamp;
  Duration              m_videoDurationTimescale;
  MediaType             m_videoFrameType;
  uint8_t              *m_videoTempBuffer;
  uint32_t              m_videoTempBufferSize;
  
  bool                  m_recordAudio;
  MP4TrackId		m_audioTrackId;
  u_int32_t		m_audioFrameNumber;
  Timestamp		m_audioStartTimestamp;
  MediaType             m_audioFrameType;
  uint64_t              m_audioSamples;
  Duration              m_audioDiffTicks;
  Duration              m_audioDiffTicksTotal;

  bool                  m_recordText;
  MP4TrackId            m_textTrackId;
  u_int32_t             m_textFrameNumber;
  Timestamp             m_textStartTimestamp;
  Duration              m_textDurationTimescale;
  MediaType             m_textFrameType;

  bool                  m_makeIod;
  bool                  m_makeIsmaCompliant;

  uint16_t m_amrMode;
  void ProcessEncodedAudioFrame(CMediaFrame *pFrame);
  void ProcessEncodedVideoFrame(CMediaFrame *pFrame);
  void ProcessEncodedTextFrame(CMediaFrame *pFrame);
  void WriteH264Frame(CMediaFrame *pFrame, Duration dur);
  uint8_t              *m_videoH264Seq;
  uint32_t              m_videoH264SeqSize;
  uint8_t              *m_videoH264Pic;
  uint32_t              m_videoH264PicSize;
#ifndef WORDS_BIGENDIAN
  uint16_t *m_convert_pcm;
  uint32_t  m_convert_pcm_size;
#endif
};

#endif /* __FILE_MP4_RECORDER_H__ */

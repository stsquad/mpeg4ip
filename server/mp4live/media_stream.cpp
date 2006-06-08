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
 * Copyright (C) Cisco Systems Inc. 2005.  All Rights Reserved.
 * 
 * Contributor(s): 
 *		Bill May 		wmay@cisco.com
 */
#include "mp4live.h"
#include "profile_audio.h"
#include "profile_video.h"
#include "profile_text.h"
#define DECLARE_CONFIG_VARIABLES
#include "media_stream.h"
#undef DECLARE_CONFIG_VARIABLES
#include "media_sink.h"
#include "media_flow.h"
#include "file_mp4_recorder.h"
#include "video_encoder.h"

CMediaStream::CMediaStream (const char *filename,
			    CConfigEntry *next,
			    CVideoProfileList *vpl,
			    CAudioProfileList *apl,
			    CTextProfileList *tpl) :
  CConfigEntry(filename, "stream", next)
{
  m_pVideoProfile = NULL;
  m_pAudioProfile = NULL;
  m_pTextProfile = NULL;
  m_video_profile_list = vpl;
  m_audio_profile_list = apl;
  m_text_profile_list = tpl;
  m_mp4_recorder = NULL;
  m_video_encoder = NULL;
  m_audio_encoder = NULL;
  m_text_encoder = NULL;
  m_audio_key = NULL;
  m_audio_salt = NULL;
  m_video_key = NULL;
  m_video_salt = NULL;
  m_text_key = NULL;
  m_text_salt = NULL;
  m_video_rtp_session = m_audio_rtp_session = NULL;
}

CMediaStream::~CMediaStream (void)
{
  CHECK_AND_FREE(m_audio_key);
  CHECK_AND_FREE(m_audio_salt);
  CHECK_AND_FREE(m_video_key);
  CHECK_AND_FREE(m_video_salt);
  CHECK_AND_FREE(m_text_key);
  CHECK_AND_FREE(m_text_salt);
}

void CMediaStream::SetVideoProfile (const char *name)
{
  SetStringValue(STREAM_VIDEO_PROFILE, name);
  m_pVideoProfile = m_video_profile_list->FindProfile(name);
}

void CMediaStream::SetAudioProfile (const char *name) 
{
  SetStringValue(STREAM_AUDIO_PROFILE, name);
  m_pAudioProfile = m_audio_profile_list->FindProfile(name);
}

void CMediaStream::SetTextProfile (const char *name)
{
  SetStringValue(STREAM_TEXT_PROFILE, name);
  m_pTextProfile = m_text_profile_list->FindProfile(name);
}

void CMediaStream::Initialize (bool check_config_name)

{
  // reads the configuration file
  CConfigEntry::Initialize(check_config_name);

  if (m_valid == false) { 
    return;
  }

  // set variables based on information we read
  if (GetBoolValue(STREAM_VIDEO_ENABLED)) {
    if (m_video_profile_list == NULL) {
      error_message("No video profiles to read");
      m_valid = false;
      return;
    }

    SetVideoProfile(GetStringValue(STREAM_VIDEO_PROFILE));
    if (m_pVideoProfile == NULL) {
      SetVideoProfile("default");
      if (m_pVideoProfile == NULL) {
	error_message("Video profile \"%s\" could not be found in stream %s", 
		      GetStringValue(STREAM_VIDEO_PROFILE),
		      GetStringValue(STREAM_NAME));
	m_valid = false;
	return;
      } 
      SetStringValue(STREAM_VIDEO_PROFILE, "default");
    }
      
  }

  if (GetBoolValue(STREAM_AUDIO_ENABLED)) {
    if (m_audio_profile_list == NULL) {
      error_message("No audio profiles to read");
      m_valid = false;
      return;
    }
    SetAudioProfile(GetStringValue(STREAM_AUDIO_PROFILE));
    if (m_pAudioProfile == NULL) {
      SetAudioProfile("default");
      if (m_pAudioProfile == NULL) {
	error_message("Audio profile \"%s\" could not be found in stream %s", 
		      GetStringValue(STREAM_AUDIO_PROFILE),
		      GetStringValue(STREAM_NAME));
	m_valid = false;
	return;
      } 
      SetStringValue(STREAM_AUDIO_PROFILE, "default");
    }
  }

  if (GetBoolValue(STREAM_TEXT_ENABLED)) {
    if (m_text_profile_list == NULL) {
      error_message("No text profiles to read");
      m_valid = false;
      return;
    }
    SetTextProfile(GetStringValue(STREAM_TEXT_PROFILE));
    if (m_pTextProfile == NULL) {
      SetTextProfile("default");
      if (m_pTextProfile == NULL) {
	error_message("Text profile \"%s\" could not be found in stream %s", 
		      GetStringValue(STREAM_TEXT_PROFILE),
		      GetStringValue(STREAM_NAME));
	m_valid = false;
	return;
      } 
      SetStringValue(STREAM_TEXT_PROFILE, "default");
    }
  }
  // same profile for text here...
  char buffer[PATH_MAX];
  // Set up the file names, if they do not already exist
  const char *last_sep = strrchr(m_filename, '/');
  if (last_sep == NULL) last_sep = m_filename;
  else last_sep++;
  if (GetStringValue(STREAM_SDP_FILE_NAME) == NULL) {
    snprintf(buffer, PATH_MAX, "%s.sdp", last_sep);
    SetStringValue(STREAM_SDP_FILE_NAME, buffer);
    debug_message("Setting stream %s sdp file to \"%s\"", 
		  GetName(), buffer);
  }
  if (GetStringValue(STREAM_RECORD_MP4_FILE_NAME) == NULL) {
    // set file name
    snprintf(buffer, PATH_MAX, "%s.mp4", last_sep);
    SetStringValue(STREAM_RECORD_MP4_FILE_NAME, buffer);
    debug_message("Setting stream %s file to \"%s\"",
		  GetName(), buffer);
  }

  if (GetStringValue(STREAM_CAPTION) == NULL) {
    if (GetStringValue(STREAM_NAME) == NULL)
      SetStringValue(STREAM_CAPTION, "default");
    else
      SetStringValue(STREAM_CAPTION, GetStringValue(STREAM_NAME));
    debug_message("Setting stream %s caption to \"%s\"", 
		  GetName(), GetStringValue(STREAM_NAME));
  }
}

void CMediaStream::LoadConfigVariables (void)
{
  AddConfigVariables(StreamConfigVariables, 
		     NUM_ELEMENTS_IN_ARRAY(StreamConfigVariables));
}

CMediaSink *CMediaStream::CreateFileRecorder(CLiveConfig *pConfig)
{
  if (m_mp4_recorder == NULL) {
    m_mp4_recorder = new CMp4Recorder(this);
    m_mp4_recorder->SetConfig(pConfig);
    m_mp4_recorder->StartThread();
  }
  return m_mp4_recorder;
}
      
void CMediaStream::Stop (void)
{
  if (m_mp4_recorder != NULL) {
    m_mp4_recorder->StopThread();
    delete m_mp4_recorder;
    m_mp4_recorder = NULL;
  }
}

bool CMediaStream::GetStreamStatus (uint32_t valueName, void *pValue)
{

  switch (valueName) {
  case FLOW_STATUS_VIDEO_ENCODED_FRAMES:
    if (m_video_encoder == NULL) return false;
    *(uint32_t *)pValue = m_video_encoder->GetEncodedFrames();
    return true;
  case FLOW_STATUS_FILENAME:
    if (m_mp4_recorder == NULL) return false;
    *(const char **)pValue = m_mp4_recorder->GetRecordFileName();
    return true;
  }
  return false;
}

void CMediaStream::RestartFileRecording (void)
{
  if (m_mp4_recorder != NULL) {
    m_mp4_recorder->Stop();
    m_mp4_recorder->Start();
  }
}

static char randhex (void)
{
  char ret = random() & 0xf;
  if (ret >= 10) return ret - 10 + 'a';
  return '0' + ret;
}

const char *generate_random(uint size)
{
  char *ret = (char *)malloc((size * 2) + 1);
  uint ix;
  for (ix = 0; ix < size; ix++) {
    ret[ix * 2] = randhex();
    ret[(ix * 2) + 1] = randhex();
  }
  ret[(size * 2)] = '\0';
  return ret;
}

void CMediaStream::SetUpSRTPKeys (void) 
{
  if (GetBoolValue(STREAM_VIDEO_USE_SRTP)) {
    srandom(GetTimestamp());
    if (GetBoolValue(STREAM_VIDEO_SRTP_FIXED_KEYS)) {
      CHECK_AND_FREE(m_video_key);
      m_video_key = strdup(GetStringValue(STREAM_VIDEO_SRTP_KEY));
      CHECK_AND_FREE(m_video_salt);
      m_video_salt = strdup(GetStringValue(STREAM_VIDEO_SRTP_SALT));
    } else {
      if (m_video_key == NULL)
	m_video_key = generate_random(16);
      if(m_video_salt == NULL) 
	m_video_salt = generate_random(14);
    }
  }
  if (GetBoolValue(STREAM_AUDIO_USE_SRTP)) {
    if (GetBoolValue(STREAM_AUDIO_SRTP_FIXED_KEYS)) {
      CHECK_AND_FREE(m_audio_key);
      m_audio_key = strdup(GetStringValue(STREAM_AUDIO_SRTP_KEY));
      CHECK_AND_FREE(m_audio_salt);
      m_audio_salt = strdup(GetStringValue(STREAM_AUDIO_SRTP_SALT));
    } else {
      if (m_audio_key == NULL)
	m_audio_key = generate_random(16);
      if(m_audio_salt == NULL) 
	m_audio_salt = generate_random(14);
    }
  }
  if (GetBoolValue(STREAM_TEXT_USE_SRTP)) {
    if (GetBoolValue(STREAM_TEXT_SRTP_FIXED_KEYS)) {
      CHECK_AND_FREE(m_text_key);
      m_text_key = strdup(GetStringValue(STREAM_TEXT_SRTP_KEY));
      CHECK_AND_FREE(m_text_salt);
      m_text_salt = strdup(GetStringValue(STREAM_TEXT_SRTP_SALT));
    } else {
      if (m_text_key == NULL)
	m_text_key = generate_random(16);
      if(m_text_salt == NULL) 
	m_text_salt = generate_random(14);
    }
  }

}

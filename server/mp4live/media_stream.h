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

/*
 * Media stream.  Contains an audio/video/text stream.  puts it in a 
 * file, out to a destination address, or both.
 * 
 * This file defines CMediaStream, and CMediaStreamList
 */

#ifndef __MEDIA_STREAM_H__
#define __MEDIA_STREAM_H__ 1
#include "mpeg4ip_config_set.h"
#include "config_list.h"

DECLARE_CONFIG(STREAM_NAME);
DECLARE_CONFIG(STREAM_DESCRIPTION);
DECLARE_CONFIG(STREAM_VIDEO_ENABLED);
DECLARE_CONFIG(STREAM_AUDIO_ENABLED);
DECLARE_CONFIG(STREAM_TEXT_ENABLED);

DECLARE_CONFIG(STREAM_VIDEO_PROFILE);
DECLARE_CONFIG(STREAM_AUDIO_PROFILE);
DECLARE_CONFIG(STREAM_TEXT_PROFILE);

DECLARE_CONFIG(STREAM_TRANSMIT);
DECLARE_CONFIG(STREAM_SDP_FILE_NAME);
DECLARE_CONFIG(STREAM_VIDEO_ADDR_FIXED);
DECLARE_CONFIG(STREAM_VIDEO_DEST_ADDR);
DECLARE_CONFIG(STREAM_VIDEO_DEST_PORT);

DECLARE_CONFIG(STREAM_AUDIO_ADDR_FIXED);
DECLARE_CONFIG(STREAM_AUDIO_DEST_ADDR);
DECLARE_CONFIG(STREAM_AUDIO_DEST_PORT);

DECLARE_CONFIG(STREAM_TEXT_ADDR_FIXED);
DECLARE_CONFIG(STREAM_TEXT_DEST_ADDR);
DECLARE_CONFIG(STREAM_TEXT_DEST_PORT);

DECLARE_CONFIG(STREAM_RECORD);
DECLARE_CONFIG(STREAM_RECORD_MP4_FILE_NAME);

#ifdef DECLARE_CONFIG_VARIABLES
static SConfigVariable StreamConfigVariables[] = {
  CONFIG_STRING(STREAM_NAME, "name", NULL),
  CONFIG_STRING(STREAM_DESCRIPTION, "streamDescription", NULL),
  CONFIG_BOOL(STREAM_VIDEO_ENABLED, "videoEnabled", true),
  CONFIG_BOOL(STREAM_AUDIO_ENABLED, "audioEnabled", true),
  CONFIG_BOOL(STREAM_TEXT_ENABLED, "textEnabled", false),

  CONFIG_STRING(STREAM_VIDEO_PROFILE, "videoProfile", "default"),
  CONFIG_STRING(STREAM_AUDIO_PROFILE, "audioProfile", "default"),
  CONFIG_STRING(STREAM_TEXT_PROFILE, "textProfile", "default"),

  CONFIG_BOOL(STREAM_TRANSMIT, "transmitEnabled", true),
  CONFIG_STRING(STREAM_SDP_FILE_NAME, "sdpFile", NULL),
  CONFIG_BOOL(STREAM_VIDEO_ADDR_FIXED, "videoAddrFixed", false),
  CONFIG_STRING(STREAM_VIDEO_DEST_ADDR, "videoDestAddress", "224.1.2.3"),
  CONFIG_INT(STREAM_VIDEO_DEST_PORT, "videoDestPort", 20000),

  CONFIG_BOOL(STREAM_AUDIO_ADDR_FIXED, "audioAddrFixed", false),
  CONFIG_STRING(STREAM_AUDIO_DEST_ADDR, "audioDestAddress", "224.1.2.3"),
  CONFIG_INT(STREAM_AUDIO_DEST_PORT, "audioDestPort", 20002),

  CONFIG_BOOL(STREAM_TEXT_ADDR_FIXED, "textAddrFixed", false),
  CONFIG_STRING(STREAM_TEXT_DEST_ADDR, "textDestAddress", "224.1.2.3"),
  CONFIG_INT(STREAM_TEXT_DEST_PORT, "textDestPort", 20004),

  CONFIG_BOOL(STREAM_RECORD, "recordEnabled", false),
  CONFIG_STRING(STREAM_RECORD_MP4_FILE_NAME, "recordFile", NULL),
};
#endif

class CVideoProfile;
class CVideoProfileList;
class CAudioProfile;
class CAudioProfileList;
class CTextProfileList;
class CMp4Recorder;
class CLiveConfig;
class CMediaSink;
class CVideoEncoder;
class CAudioEncoder;

class CMediaStream : public CConfigEntry
{
 public:
  CMediaStream(const char *filename,
	       CConfigEntry *next,
	       CVideoProfileList *video_profile_list,
	       CAudioProfileList *audio_profile_list, 
	       CTextProfileList *text_profile_list);
  ~CMediaStream(void);

  CMediaStream *GetNext (void) { 
    return (CMediaStream *)CConfigEntry::GetNext(); 
  } ;

  void SetVideoProfile(const char *name); 
  void SetAudioProfile(const char *name);
  void LoadConfigVariables(void);
  void Initialize(bool check_config_name = true);
  CAudioProfile *GetAudioProfile(void) { return m_pAudioProfile; };
  CVideoProfile *GetVideoProfile(void) { return m_pVideoProfile; };
  CMediaSink *CreateFileRecorder(CLiveConfig *pConfig);
  void Stop(void);
  void SetVideoEncoder(CVideoEncoder *ve) { m_video_encoder = ve; };
  void SetAudioEncoder(CAudioEncoder *ae) { m_audio_encoder = ae; };
  bool GetStreamStatus(uint32_t valueName, void *pValue);
 protected:
  CVideoProfile *m_pVideoProfile;
  CAudioProfile *m_pAudioProfile;
  //CConfigSet *m_pTextProfile;
  CVideoProfileList *m_video_profile_list;
  CAudioProfileList *m_audio_profile_list;
  CTextProfileList *m_text_profile_list;
  CMp4Recorder *m_mp4_recorder;
  CVideoEncoder *m_video_encoder;
  CAudioEncoder *m_audio_encoder;
};

class CMediaStreamList : public CConfigList
{
 public:
  CMediaStreamList(const char *directory,
		   CVideoProfileList *vpl,
		   CAudioProfileList *apl, 
		   CTextProfileList *tpl = NULL) :
    CConfigList(directory, "stream") {
    m_video_profile_list = vpl;
    m_audio_profile_list = apl;
    m_text_profile_list = tpl;
  };
  ~CMediaStreamList(void) {
  };
  CMediaStream *FindStream(const char *name) {
    return (CMediaStream *)FindConfigInt(name);
  }
  CMediaStream *GetHead (void) { return (CMediaStream *)m_config_list; };
  void RemoveStream (CMediaStream *ms) {
    RemoveEntryFromList((CConfigEntry *)ms);
  };
 protected:
  CConfigEntry *CreateConfigInt(const char *fname, CConfigEntry *next) {
    CMediaStream *ret = new CMediaStream(fname, next,
					 m_video_profile_list, 
					 m_audio_profile_list, 
					 m_text_profile_list);
    ret->LoadConfigVariables();
    return ret;
  };
  CVideoProfileList *m_video_profile_list;
  CAudioProfileList *m_audio_profile_list;
  CTextProfileList *m_text_profile_list;
};

void createStreamSdp(CLiveConfig *pGlobal, CMediaStream *pStream);

#endif

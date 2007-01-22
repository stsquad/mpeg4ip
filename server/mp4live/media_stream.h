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
DECLARE_CONFIG(STREAM_CAPTION);
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
DECLARE_CONFIG(STREAM_VIDEO_RTCP_DEST_ADDR);
DECLARE_CONFIG(STREAM_VIDEO_RTCP_DEST_PORT);
DECLARE_CONFIG(STREAM_VIDEO_SRC_PORT);
DECLARE_CONFIG(STREAM_VIDEO_USE_SRTP);
DECLARE_CONFIG(STREAM_VIDEO_SRTP_ENC_ALGO);
DECLARE_CONFIG(STREAM_VIDEO_SRTP_AUTH_ALGO);
DECLARE_CONFIG(STREAM_VIDEO_SRTP_FIXED_KEYS);
DECLARE_CONFIG(STREAM_VIDEO_SRTP_KEY);
DECLARE_CONFIG(STREAM_VIDEO_SRTP_SALT);
DECLARE_CONFIG(STREAM_VIDEO_SRTP_RTP_ENC);
DECLARE_CONFIG(STREAM_VIDEO_SRTP_RTP_AUTH);
DECLARE_CONFIG(STREAM_VIDEO_SRTP_RTCP_ENC);

DECLARE_CONFIG(STREAM_AUDIO_ADDR_FIXED);
DECLARE_CONFIG(STREAM_AUDIO_DEST_ADDR);
DECLARE_CONFIG(STREAM_AUDIO_DEST_PORT);
DECLARE_CONFIG(STREAM_AUDIO_RTCP_DEST_ADDR);
DECLARE_CONFIG(STREAM_AUDIO_RTCP_DEST_PORT);
DECLARE_CONFIG(STREAM_AUDIO_SRC_PORT);
DECLARE_CONFIG(STREAM_AUDIO_USE_SRTP);
DECLARE_CONFIG(STREAM_AUDIO_SRTP_ENC_ALGO);
DECLARE_CONFIG(STREAM_AUDIO_SRTP_AUTH_ALGO);
DECLARE_CONFIG(STREAM_AUDIO_SRTP_FIXED_KEYS);
DECLARE_CONFIG(STREAM_AUDIO_SRTP_KEY);
DECLARE_CONFIG(STREAM_AUDIO_SRTP_SALT);
DECLARE_CONFIG(STREAM_AUDIO_SRTP_RTP_ENC);
DECLARE_CONFIG(STREAM_AUDIO_SRTP_RTP_AUTH);
DECLARE_CONFIG(STREAM_AUDIO_SRTP_RTCP_ENC);

DECLARE_CONFIG(STREAM_TEXT_ADDR_FIXED);
DECLARE_CONFIG(STREAM_TEXT_DEST_ADDR);
DECLARE_CONFIG(STREAM_TEXT_DEST_PORT);
DECLARE_CONFIG(STREAM_TEXT_RTCP_DEST_ADDR);
DECLARE_CONFIG(STREAM_TEXT_RTCP_DEST_PORT);
DECLARE_CONFIG(STREAM_TEXT_SRC_PORT);
DECLARE_CONFIG(STREAM_TEXT_USE_SRTP);
DECLARE_CONFIG(STREAM_TEXT_SRTP_ENC_ALGO);
DECLARE_CONFIG(STREAM_TEXT_SRTP_AUTH_ALGO);
DECLARE_CONFIG(STREAM_TEXT_SRTP_FIXED_KEYS);
DECLARE_CONFIG(STREAM_TEXT_SRTP_KEY);
DECLARE_CONFIG(STREAM_TEXT_SRTP_SALT);
DECLARE_CONFIG(STREAM_TEXT_SRTP_RTP_ENC);
DECLARE_CONFIG(STREAM_TEXT_SRTP_RTP_AUTH);
DECLARE_CONFIG(STREAM_TEXT_SRTP_RTCP_ENC);

DECLARE_CONFIG(STREAM_RECORD);
DECLARE_CONFIG(STREAM_RECORD_MP4_FILE_NAME);

#ifdef DECLARE_CONFIG_VARIABLES
static SConfigVariable StreamConfigVariables[] = {
  CONFIG_STRING(STREAM_NAME, "name", NULL),
  CONFIG_STRING(STREAM_CAPTION, "caption", NULL),
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
  CONFIG_STRING(STREAM_VIDEO_RTCP_DEST_ADDR, "videoRtcpDestAddress", NULL),
  CONFIG_INT(STREAM_VIDEO_RTCP_DEST_PORT, "videoRtcpDestPort", 0),
  CONFIG_INT(STREAM_VIDEO_SRC_PORT, "videoSrcPort", 0),

  CONFIG_BOOL(STREAM_VIDEO_USE_SRTP, "videoUseSRTP", false),
  CONFIG_INT(STREAM_VIDEO_SRTP_ENC_ALGO, "videoSRtpEncAlgorithm", 0),
  CONFIG_INT(STREAM_VIDEO_SRTP_AUTH_ALGO, "videoSRtpAuthAlgorithm", 0),
  CONFIG_BOOL(STREAM_VIDEO_SRTP_FIXED_KEYS, "videoSRTPFixedKeys", false),
  CONFIG_STRING(STREAM_VIDEO_SRTP_KEY, "videoSRTPKey", NULL),
  CONFIG_STRING(STREAM_VIDEO_SRTP_SALT, "videoSRTPSalt", NULL),
  CONFIG_BOOL(STREAM_VIDEO_SRTP_RTP_ENC, "videoSrtpRtpEnc", true),
  CONFIG_BOOL(STREAM_VIDEO_SRTP_RTP_AUTH, "videoSrtpRtpAuth", true),
  CONFIG_BOOL(STREAM_VIDEO_SRTP_RTCP_ENC, "videoSrtpRtcpEnc", true),

  CONFIG_BOOL(STREAM_AUDIO_ADDR_FIXED, "audioAddrFixed", false),
  CONFIG_STRING(STREAM_AUDIO_DEST_ADDR, "audioDestAddress", "224.1.2.3"),
  CONFIG_INT(STREAM_AUDIO_DEST_PORT, "audioDestPort", 20002),
  CONFIG_STRING(STREAM_AUDIO_RTCP_DEST_ADDR, "audioRtcpDestAddress", NULL),
  CONFIG_INT(STREAM_AUDIO_RTCP_DEST_PORT, "audioRtcpDestPort", 0),
  CONFIG_INT(STREAM_AUDIO_SRC_PORT, "audioSrcPort", 0),

  CONFIG_BOOL(STREAM_AUDIO_USE_SRTP, "audioUseSRTP", false),
  CONFIG_INT(STREAM_AUDIO_SRTP_ENC_ALGO, "audioSRtpEncAlgorithm", 0),
  CONFIG_INT(STREAM_AUDIO_SRTP_AUTH_ALGO, "audioSRtpAuthAlgorithm", 0),
  CONFIG_BOOL(STREAM_AUDIO_SRTP_FIXED_KEYS, "audioSRTPFixedKeys", false),
  CONFIG_STRING(STREAM_AUDIO_SRTP_KEY, "audioSRTPKey", NULL),
  CONFIG_STRING(STREAM_AUDIO_SRTP_SALT, "audioSRTPSalt", NULL),
  CONFIG_BOOL(STREAM_AUDIO_SRTP_RTP_ENC, "audioSrtpRtpEnc", true),
  CONFIG_BOOL(STREAM_AUDIO_SRTP_RTP_AUTH, "audioSrtpRtpAuth", true),
  CONFIG_BOOL(STREAM_AUDIO_SRTP_RTCP_ENC, "audioSrtpRtcpEnc", true),

  CONFIG_BOOL(STREAM_TEXT_ADDR_FIXED, "textAddrFixed", false),
  CONFIG_STRING(STREAM_TEXT_DEST_ADDR, "textDestAddress", "224.1.2.3"),
  CONFIG_INT(STREAM_TEXT_DEST_PORT, "textDestPort", 20004),
  CONFIG_STRING(STREAM_TEXT_RTCP_DEST_ADDR, "textRtcpDestAddress", NULL),
  CONFIG_INT(STREAM_TEXT_RTCP_DEST_PORT, "textRtcpDestPort", 0),
  CONFIG_INT(STREAM_TEXT_SRC_PORT, "textSrcPort", 0),

  CONFIG_BOOL(STREAM_TEXT_USE_SRTP, "textUseSRTP", false),
  CONFIG_INT(STREAM_TEXT_SRTP_ENC_ALGO, "textSRtpEncAlgorithm", 0),
  CONFIG_INT(STREAM_TEXT_SRTP_AUTH_ALGO, "textSRtpAuthAlgorithm", 0),
  CONFIG_BOOL(STREAM_TEXT_SRTP_FIXED_KEYS, "textSRTPFixedKeys", false),
  CONFIG_STRING(STREAM_TEXT_SRTP_KEY, "textSRTPKey", NULL),
  CONFIG_STRING(STREAM_TEXT_SRTP_SALT, "textSRTPSalt", NULL),
  CONFIG_BOOL(STREAM_TEXT_SRTP_RTP_ENC, "textSrtpRtpEnc", true),
  CONFIG_BOOL(STREAM_TEXT_SRTP_RTP_AUTH, "textSrtpRtpAuth", true),
  CONFIG_BOOL(STREAM_TEXT_SRTP_RTCP_ENC, "textSrtpRtcpEnc", true),

  CONFIG_BOOL(STREAM_RECORD, "recordEnabled", false),
  CONFIG_STRING(STREAM_RECORD_MP4_FILE_NAME, "recordFile", NULL),
};
#endif

class CVideoProfile;
class CVideoProfileList;
class CAudioProfile;
class CAudioProfileList;
class CTextProfile;
class CTextProfileList;
class CMp4Recorder;
class CLiveConfig;
class CMediaSink;
class CVideoEncoder;
class CAudioEncoder;
class CTextEncoder;

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
  void SetTextProfile(const char *name);
  void LoadConfigVariables(void);
  void Initialize(bool check_config_name = true);
  CAudioProfile *GetAudioProfile(void) { return m_pAudioProfile; };
  CVideoProfile *GetVideoProfile(void) { return m_pVideoProfile; };
  CTextProfile *GetTextProfile(void) { return m_pTextProfile; };
  CMediaSink *CreateFileRecorder(CLiveConfig *pConfig);
  void Stop(void);
  void SetVideoEncoder(CVideoEncoder *ve) { m_video_encoder = ve; };
  void SetAudioEncoder(CAudioEncoder *ae) { m_audio_encoder = ae; };
  void SetTextEncoder(CTextEncoder *te) { m_text_encoder = te; };
  bool GetStreamStatus(uint32_t valueName, void *pValue);
  void RestartFileRecording(void);
  void SetUpSRTPKeys(void);
 protected:
  CVideoProfile *m_pVideoProfile;
  CAudioProfile *m_pAudioProfile;
  CTextProfile *m_pTextProfile;
  CVideoProfileList *m_video_profile_list;
  CAudioProfileList *m_audio_profile_list;
  CTextProfileList *m_text_profile_list;
  CMp4Recorder *m_mp4_recorder;
  CVideoEncoder *m_video_encoder;
  CAudioEncoder *m_audio_encoder;
  CTextEncoder *m_text_encoder;
 public:
  const char *m_audio_key;
  const char *m_audio_salt;
  const char *m_video_key;
  const char *m_video_salt;
  const char *m_text_key;
  const char *m_text_salt;
  struct rtp *m_video_rtp_session, *m_audio_rtp_session;
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


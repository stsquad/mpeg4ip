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
#ifndef __PROFILE_AUDIO__
#define __PROFILE_AUDIO__ 1
#include "config_list.h"

#define AUDIO_ENCODER_FAAC		"faac"
#define AUDIO_ENCODER_LAME		"lame"
#define AUDIO_ENCODER_G711              "g711"
#define AUDIO_ENCODER_L16               "l16"
#define AUDIO_ENCODER_TWOLAME           "twolame"

#define AUDIO_ENCODING_NONE		"None"
#define AUDIO_ENCODING_PCM16	"PCM16"
#define AUDIO_ENCODING_MP3		"MP3"
#define AUDIO_ENCODING_AAC		"AAC"
#define AUDIO_ENCODING_AC3		"AC3"
#define AUDIO_ENCODING_VORBIS	"VORBIS"
#define AUDIO_ENCODING_AMR      "AMR"
#define AUDIO_ENCODING_ALAW     "ALAW"
#define AUDIO_ENCODING_ULAW     "ULAW"
#define AUDIO_ENCODING_L16      "L16"

DECLARE_CONFIG(CFG_AUDIO_PROFILE_NAME);
DECLARE_CONFIG(CFG_AUDIO_CHANNELS);
DECLARE_CONFIG(CFG_AUDIO_SAMPLE_RATE);
DECLARE_CONFIG(CFG_AUDIO_BIT_RATE);
DECLARE_CONFIG(CFG_AUDIO_ENCODING);
DECLARE_CONFIG(CFG_AUDIO_ENCODER);
DECLARE_CONFIG(CFG_RTP_USE_MP3_PAYLOAD_14);
DECLARE_CONFIG(CFG_RTP_MAX_FRAMES_PER_PACKET);
DECLARE_CONFIG(CFG_RTP_RFC3016);
DECLARE_CONFIG(CFG_AUDIO_DEBUG);

#ifdef DECLARE_CONFIG_VARIABLES
#ifdef HAVE_FAAC
#define AUDIO_DEF_CHAN 2
#define AUDIO_DEF_SRATE 44100
#define AUDIO_DEF_BRATE 128000
#define AUDIO_DEF_ENCODING AUDIO_ENCODING_AAC
#define AUDIO_DEF_ENCODER AUDIO_ENCODER_FAAC
#else
#define AUDIO_DEF_CHAN 1
#define AUDIO_DEF_SRATE 8000
#define AUDIO_DEF_BRATE 64000
#define AUDIO_DEF_ENCODING AUDIO_ENCODING_ULAW
#define AUDIO_DEF_ENCODER AUDIO_ENCODER_G711
#endif
static SConfigVariable AudioProfileConfigVariables[] = {
  CONFIG_STRING(CFG_AUDIO_PROFILE_NAME, "name", NULL),
  CONFIG_INT(CFG_AUDIO_CHANNELS, "audioChannels", AUDIO_DEF_CHAN),
  CONFIG_INT(CFG_AUDIO_SAMPLE_RATE, "audioSampleRate", AUDIO_DEF_SRATE),
  CONFIG_INT(CFG_AUDIO_BIT_RATE, "audioBitRateBps", AUDIO_DEF_BRATE),
  CONFIG_STRING(CFG_AUDIO_ENCODING, "audioEncoding", AUDIO_DEF_ENCODING),
  CONFIG_STRING(CFG_AUDIO_ENCODER, "audioEncoder", AUDIO_DEF_ENCODER),
  CONFIG_BOOL(CFG_RTP_USE_MP3_PAYLOAD_14, "rtpUseMp3RtpPayload14", false),
  CONFIG_INT(CFG_RTP_MAX_FRAMES_PER_PACKET, "rtpMaxFramesPerPacket", 0),
  CONFIG_BOOL(CFG_RTP_RFC3016, "rtpRFC3016", false),
  CONFIG_BOOL(CFG_AUDIO_DEBUG, "debug", false),
};
#endif

class CAudioProfile : public CConfigEntry
{
 public:
  CAudioProfile(const char *filename, CConfigEntry *next) :
    CConfigEntry(filename, "audio", next) {
  };
  ~CAudioProfile(void) { };
  void LoadConfigVariables(void);
  void Update(void);
};

class CAudioProfileList : public CConfigList
{
  public:
  CAudioProfileList(const char *directory) :
    CConfigList(directory, "audio") {
  };
  
  ~CAudioProfileList(void) {
  };
  CAudioProfile *FindProfile(const char *name) {
    return (CAudioProfile *)FindConfigInt(name);
  };
 protected:
  CConfigEntry *CreateConfigInt(const char *fname, CConfigEntry *next) {
    CAudioProfile *ret = new CAudioProfile(fname, next);
    ret->LoadConfigVariables();
    return ret;
  };
};

#endif

#ifndef __PROFILE_AUDIO__
#define __PROFILE_AUDIO__ 1
#include "profile_list.h"

#define AUDIO_ENCODER_LAME "lame"
#define AUDIO_ENCODING_MP3 "MP3"

DECLARE_CONFIG(CONFIG_AUDIO_PROFILE_NAME);
DECLARE_CONFIG(CONFIG_AUDIO_CHANNELS);
DECLARE_CONFIG(CONFIG_AUDIO_SAMPLE_RATE);
DECLARE_CONFIG(CONFIG_AUDIO_BIT_RATE_KBPS);
DECLARE_CONFIG(CONFIG_AUDIO_BIT_RATE);
DECLARE_CONFIG(CONFIG_AUDIO_ENCODING);
DECLARE_CONFIG(CONFIG_AUDIO_ENCODER);

#ifdef DECLARE_CONFIG_VARIABLES
static SConfigVariable AudioProfileConfigVariables[] = {
  CONFIG_STRING(CONFIG_AUDIO_PROFILE_NAME, "profileName", NULL),
  CONFIG_INT(CONFIG_AUDIO_CHANNELS, "audioChannels", 2),
  CONFIG_INT(CONFIG_AUDIO_SAMPLE_RATE, "audioSampleRate", 44100),
  CONFIG_INT(CONFIG_AUDIO_BIT_RATE_KBPS, "audioBitRate", 128),
  CONFIG_INT(CONFIG_AUDIO_BIT_RATE, "audioBitRateBps",128000),
  CONFIG_STRING(CONFIG_AUDIO_ENCODING, "audioEncoding", AUDIO_ENCODING_MP3),
  CONFIG_STRING(CONFIG_AUDIO_ENCODER, "audioEncoder", AUDIO_ENCODER_LAME),
};
#endif

class CAudioProfile : public CProfileList
{
  public:
  CAudioProfile(const char *directory) :
    CProfileList(directory, "audio") {
  };

  ~CAudioProfile(void) {
  };
  
 protected:
  void LoadConfigVariables(CProfileEntry *e);
};

#endif

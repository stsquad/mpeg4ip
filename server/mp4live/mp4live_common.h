

#ifndef __MP4LIVE_COMMON_H__
#define __MP4LIVE_COMMON_H__ 1

CLiveConfig *InitializeConfigVariables(void);

int ReadConfigFile(const char *configFileName, CLiveConfig *ppConfig);

void ProbeVideoAudioCapabilities(CLiveConfig *pConfig);

void SetupRealTimeFeatures(CLiveConfig *pConfig);

class CMediaSource;
class CTextSource;

CMediaSource *CreateVideoSource(CLiveConfig *pConfig);

CMediaSource *CreateAudioSource(CLiveConfig *pConfig, CMediaSource *videoSource);
CTextSource *CreateTextSource(CLiveConfig *pConfig);

void InitialVideoProbe(CLiveConfig *pConfig);

void InstallSignalHandler(const char *list,
			  void (*sighandler)(int), 
			  bool setsigint = false);

void GetHomeDirectory (char *base);

void CapabilitiesCleanUp(void);
#endif

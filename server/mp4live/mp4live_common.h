

#ifndef __MP4LIVE_COMMON_H__
#define __MP4LIVE_COMMON_H__ 1

CLiveConfig *InitializeConfigVariables(void);

int ReadConfigFile(const char *configFileName, CLiveConfig *ppConfig);

void ProbeVideoAudioCapabilities(CLiveConfig *pConfig);

void SetupRealTimeFeatures(CLiveConfig *pConfig);

class CMediaSource;

CMediaSource *CreateVideoSource(CLiveConfig *pConfig);

CMediaSource *CreateAudioSource(CLiveConfig *pConfig, CMediaSource *videoSource);

void InstallSignalHandler(CLiveConfig *pConfig, 
			  void (*sighandler)(int));

#endif

#include "mp4live.h"
#include "util.h"
#include "mp4live_common.h"
#include "audio_alsa_source.h"
#include "audio_oss_source.h"
#include "signal.h"
#include "net_udp.h"
#include "video_v4l_source.h"

int ReadConfigFile (const char *configFileName, 
		    CLiveConfig *pConfig)
{

  char *addr = get_host_ip_address();
  udp_set_multicast_src(addr);
  free(addr);

  try {
    pConfig->ReadFile(configFileName);

#ifndef HAVE_ALSA 
    if (strcmp(pConfig->GetStringValue(CONFIG_AUDIO_SOURCE_TYPE),
	       AUDIO_SOURCE_ALSA) == 0) {
      error_message("Audio source from config is alsa, and not configured");
      error_message("Switching to OSS");
      pConfig->SetStringValue(CONFIG_AUDIO_SOURCE_TYPE, AUDIO_SOURCE_OSS);
    }
#endif
    if (!pConfig->IsDefault(CONFIG_RECORD_MP4_OVERWRITE)) {
      pConfig->SetIntegerValue(CONFIG_RECORD_MP4_FILE_STATUS, FILE_MP4_APPEND);
      pConfig->SetToDefault(CONFIG_RECORD_MP4_OVERWRITE);
    }
    PrintDebugMessages =
      pConfig->GetBoolValue(CONFIG_APP_DEBUG);
 }
  catch (CConfigException* e) {
    delete e;
    return -1;
  }
  
  return 0;
}

void ProbeVideoAudioCapabilities (CLiveConfig *pConfig) 
{
  // probe for capture cards
  if (!strcasecmp(pConfig->GetStringValue(CONFIG_VIDEO_SOURCE_TYPE),
		  VIDEO_SOURCE_V4L)) {
    InitialVideoProbe(pConfig);
  }

  // probe for sound card capabilities
  const char *deviceName = pConfig->GetStringValue(CONFIG_AUDIO_SOURCE_NAME);
  pConfig->m_audioCapabilities = FindAudioCapabilitiesByDevice(deviceName);
  if (pConfig->m_audioCapabilities == NULL) {
    if (!strcasecmp(pConfig->GetStringValue(CONFIG_AUDIO_SOURCE_TYPE), 
		    AUDIO_SOURCE_OSS)) {
    
      pConfig->m_audioCapabilities = new CAudioCapabilities(deviceName);
#ifdef HAVE_ALSA
    } else if (!strcasecmp(pConfig->GetStringValue(CONFIG_AUDIO_SOURCE_TYPE), 
			   AUDIO_SOURCE_ALSA)) {
      pConfig->m_audioCapabilities = new CALSAAudioCapabilities(deviceName);
#endif
    }
    if (pConfig->m_audioCapabilities != NULL) {
      pConfig->m_audioCapabilities->SetNext(AudioCapabilities);
      AudioCapabilities = pConfig->m_audioCapabilities;
    }
  }
}

void SetupRealTimeFeatures (CLiveConfig *pConfig)
{
  int rc;
  // attempt to exploit any real time features of the OS
  // will probably only succeed if user has root privileges
  if (pConfig->GetBoolValue(CONFIG_APP_REAL_TIME_SCHEDULER)) {
#ifdef _POSIX_PRIORITY_SCHEDULING
    // put us into the lowest real-time scheduling queue
    struct sched_param sp;
    sp.sched_priority = 1;
    rc = sched_setscheduler(0, SCHED_RR, &sp);

#ifdef DEBUG
    if (rc < 0) {
      debug_message("Unable to set scheduling priority: %s",
		    strerror(errno));
    } else {
      debug_message("Set real time scheduling priority");
    }
#endif

#endif /* _POSIX_PRIORITY_SCHEDULING */
#ifdef _POSIX_MEMLOCK
    // recommendation is to reserve some stack pages
    u_int8_t huge[1024 * 1024];
    memset(huge, 1, sizeof(huge));

    // and then lock memory
    if (mlockall(MCL_CURRENT|MCL_FUTURE) < 0) {
# ifdef DEBUG
      debug_message("Unable to lock memory: %s", 
		    strerror(errno));
# endif
    }
#endif /* _POSIX_MEMLOCK */
  }
}

static struct sig_str_to_value_ {
  const char *str;
  int value; 
} sig_str_to_value[] = {
  { "hup", SIGHUP },
  { "abrt", SIGABRT },
  { "alrm",  SIGALRM },
  { "usr1", SIGUSR1},
  { "usr2", SIGUSR2},
  { "int", SIGINT},
  { "quit", SIGQUIT},
  { "term", SIGTERM},
  { "stop", SIGSTOP},
};

void InstallSignalHandler (const char *sig,
			   void (*sighandler)(int),
			   bool setsigint)
{
  struct sigaction act;
  bool sigintset = false;

  act.sa_handler = sighandler;
  sigemptyset(&act.sa_mask);
  act.sa_flags = 0;

  int maxsigs = sizeof(sig_str_to_value) / sizeof(sig_str_to_value[0]);
  if (sig != NULL && *sig != '\0') {
    debug_message("sigals are %s", sig);
    for (int ix = 0; ix < maxsigs; ix++) {
      if (strcasestr(sig, sig_str_to_value[ix].str) != NULL) {
	debug_message("installing sig %s", sig_str_to_value[ix].str);
	sigaction(sig_str_to_value[ix].value, &act, 0);
	if (sig_str_to_value[ix].value == SIGINT) {
	  sigintset = true;
	}
      }
    }
  }

  if (setsigint && sigintset == false) {
    sigaction(SIGINT, &act, 0);
  }
}

void GetHomeDirectory (char *base)
{
  const char *home;
  home = getenv("HOME");
  base[0] = '\0';
  if (home) {
    strcpy(base, home);
    strcat(base, "/");
  }
}

void CapabilitiesCleanUp (void)
{
  CCapabilities *aud;
  while (AudioCapabilities != NULL) {
    aud = AudioCapabilities->GetNext();
    delete AudioCapabilities;
    AudioCapabilities = aud;
  }
  CVideoCapabilities *vid;
  while (VideoCapabilities != NULL) {
    vid = (CVideoCapabilities *)VideoCapabilities->GetNext();
    delete VideoCapabilities;
    VideoCapabilities = vid;
  }
}

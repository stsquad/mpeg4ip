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
 * Copyright (C) Cisco Systems Inc. 2000-2004.  All Rights Reserved.
 * 
 * Contributor(s): 
 *		Dave Mackie		dmackie@cisco.com
 *		Bill May 		wmay@cisco.com
 */
#define DECLARE_CONFIG_VARIABLES 1
#include <mp4.h>
#include "mp4live.h"
#undef DECLARE_CONFIG_VARIABLES
#include "media_flow.h"

#include "video_v4l_source.h"
#include "audio_alsa_source.h"
#include "audio_oss_source.h"
#include "text_source.h"
#include "audio_encoder.h"
#include "mp4live_common.h"
#include <getopt.h>
#include <signal.h>
#include "preview_flow.h"

// InitializeConfigVariables - if you want to add configuration 
// variables, you most likely want to add them here, before you
// return the new CLiveConfig.  Use the AddConfigVariables function
// of the CLiveConfig class to do that.

CLiveConfig *InitializeConfigVariables (void)
{
  char userFileName[MAXPATHLEN];
  char* home;
  CLiveConfig *ret;

  home = getenv("HOME");
  userFileName[0] = '\0';
  if (home) {
    strcpy(userFileName, home);
    strcat(userFileName, "/");
  }
  strcat(userFileName, ".mp4live_rc");

  ret = new CLiveConfig(MyConfigVariables,
			 sizeof(MyConfigVariables) / sizeof(SConfigVariable),
			 home ? userFileName : NULL);
  ret->InitializeIndexes();
  return ret;
}

// CreateVideoSource - if you've got more sources to add, this is
// where you can do that.
CMediaSource *CreateVideoSource (CLiveConfig *pConfig)
{
  CMediaSource *vs;
  const char* sourceType = 
    pConfig->GetStringValue(CONFIG_VIDEO_SOURCE_TYPE);

  if (!strcasecmp(sourceType, VIDEO_SOURCE_V4L)) {
    vs = new CV4LVideoSource();
  } else {
    error_message("unknown video source type %s", sourceType);
    return NULL;
  }
  vs->SetConfig(pConfig);
  vs->StartThread();
  return vs;
}

// CreateAudioSource - if you've got more sources to add, this is
// where you can do that.
CMediaSource *CreateAudioSource (CLiveConfig *pConfig,
				 CMediaSource *videoSource)
{
  CMediaSource *audioSource;

  if (pConfig->IsOneSource() && videoSource) {
    audioSource = videoSource;
  } else {
    const char* sourceType = 
      pConfig->GetStringValue(CONFIG_AUDIO_SOURCE_TYPE);

    if (!strcasecmp(sourceType, AUDIO_SOURCE_OSS)) {
      audioSource = new COSSAudioSource(pConfig);
#ifdef HAVE_ALSA
    }else if (!strcasecmp(sourceType, AUDIO_SOURCE_ALSA)) {
      audioSource = new CALSAAudioSource(pConfig);
#endif
    } else {
      error_message("unknown audio source type %s", sourceType);
      return NULL;
    }
    audioSource->StartThread();
  }
  
  audioSource->SetVideoSource(videoSource);

  return audioSource;
}

CTextSource *CreateTextSource (CLiveConfig *pConfig) 
{
  CTextSource *textSource = NULL;
  if (strcmp(pConfig->GetStringValue(CONFIG_TEXT_SOURCE_TYPE), 
	     TEXT_SOURCE_TIMED_FILE) == 0)
    textSource = new CFileTextSource(pConfig);
  else 
    textSource = new CDialogTextSource(pConfig);

  textSource->StartThread();
  return textSource;
}

// Probe video input devices
void InitialVideoProbe(CLiveConfig *pConfig)
{
  CV4LVideoSource::InitialVideoProbe(pConfig);
}

int main(int argc, char** argv)
{
  int rc = 0;
	char* configFileName = NULL;
	bool automatic = false;
	bool headless = false;
	bool sdpOnly = false;
	bool detach = false;
	extern int nogui_main(CLiveConfig* pConfig);
	extern int gui_main(int argc, char**argv, CLiveConfig* pConfig);

	CLiveConfig* pConfig = InitializeConfigVariables();
	config_index_t ix;
	bool have_unknown_opts = false;
	/*
	 * First pass through getopt - read the below options, to see if
	 * a file had been specified for config options.
	 */
	static struct option orig_options[] = {
	  { "automatic", 0, 0, 'a' },
	  { "file", 1, 0, 'f' },
	  { "headless", 0, 0, 'h' },
	  { "detach", 0, 0, 'd' },
	  { "sdp", 0, 0, 's' },
	  { "version", 0, 0, 'v' },
	  { "help", 0, 0, 'H'},
	  { "config-vars", 0, 0, 'c'},
	  { NULL, 0, 0, 0 }
	};
	opterr = 0;
	while (true) {
	  int c = -1;
	  int option_index = 0;
	  
	  c = getopt_long_only(argc, argv, "af:hsvc",
			       orig_options, &option_index);

	  if (c == -1)
	    break;

	  switch (c) {
	  case 'a':
	    automatic = true;
	    break;
	  case 'f':	// -f <config-file>
	    configFileName = strdup(optarg);
	    break;
	  case 'H':
	    fprintf(stderr, 
		    "Usage: %s [-f config_file] [--automatic] [--headless] [--sdp] [--<config variable>=<value>]\n",
		    argv[0]);
	    fprintf(stderr, "Use [--config-vars] to dump configuration variables\n");
	    exit(-1);

	  case 'c':
	    pConfig->DisplayHelp();
	    exit(0);
	  case 'h':
	    headless = true;
	    break;
	  case 'd':
	    detach = true;
	    break;
	  case 's':
	    sdpOnly = true;
	    break;
	  case 'v':
	    fprintf(stderr, 
		    "%s version %s %s\n", argv[0], MPEG4IP_VERSION,
		    get_linux_video_type());
#ifdef HAVE_FFMPEG
	    fprintf(stderr, "  - with ffmpeg encoder\n");
#endif
#ifdef HAVE_XVID_1_0
	    fprintf(stderr, "  - with XVID 1.0 encoder\n");
#endif
#ifdef HAVE_XVID_H
	    fprintf(stderr, "  - with xvid 0.9.2 encoder\n");
#endif

	    exit(0);
	  case '?':
	  default:
	    have_unknown_opts = true;
	    break;
	  }
	}

  if (detach) {
    // This will detach from the starting tty
    if(fork() !=0) {
      exit(0);
    }
    setpgrp();
  }

	InitAudioEncoders();
	/*
	 * Read the config file so that any command line options can
	 * overwrite them
	 */
	if (ReadConfigFile(configFileName, pConfig) < 0) {
	}

	if (have_unknown_opts) {
	  /*
	   * Create an struct option that allows all the loaded configuration
	   * options
	   */
	  struct option *long_options;
	  uint32_t origo = sizeof(orig_options) / sizeof(*orig_options);
	  long_options = create_long_opts_from_config(pConfig,
						      orig_options,
						      origo,
						      128);
	  if (long_options == NULL) {
	    error_message("Couldn't create options");
	    exit(-1);
	  }
	  optind = 1;
	  // command line parsing
	  while (true) {
	    int c = -1;
	    int option_index = 0;

	    c = getopt_long_only(argc, argv, "af:hsv",
				 long_options, &option_index);

	    if (c == -1)
	      break;

	    /*
	     * We set a value of 128 to 128 + number of variables
	     * we added the original options to the block, but don't
	     * process them here.
	     */
	    if (c >= 128) {
	      // we have an option from the config file
	      ix = c - 128;
	      error_message("setting %s to %s",
			    pConfig->GetNameFromIndex(ix),
			    optarg);
	      if (pConfig->GetTypeFromIndex(ix) == CONFIG_TYPE_BOOL &&
		  optarg == NULL) {
		pConfig->SetBoolValue(ix, true);
	      } else
		pConfig->SetVariableFromAscii(ix, optarg);
	    } else if (c == '?') {
	      fprintf(stderr, 
		      "Usage: %s [-f config_file] [--automatic] [--headless] [--sdp]\n",
		      argv[0]);
	      exit(-1);
	    }
	  }
	  free(long_options);
	}
	
	pConfig->m_appAutomatic = automatic;

	ProbeVideoAudioCapabilities(pConfig);

	pConfig->Update();

	// initialization done, time to execute

	// first case: just want to generate the SDP file
	if (sdpOnly) {
	  // just creating the flow will create the sdp files for all
	  // sources
	  CAVMediaFlow* pFlow = new CAVMediaFlow(pConfig);
	  delete pFlow;
	  delete pConfig;
	  exit(rc);
	}

	// other cases:

	SetupRealTimeFeatures(pConfig);
	error_message("%s version %s %s", argv[0], MPEG4IP_VERSION,
		      get_linux_video_type());
#ifndef HAVE_GTK
	error_message("You may be expecting a GUI at this point, but you "
		      "have not installed GTK-2.0 development libraries");
	rc = nogui_main(pConfig);
#else
	if (headless) {
		rc = nogui_main(pConfig);
	} else {
		// hand over control to GUI
		try {
			rc = gui_main(argc, argv, pConfig);
		}
		catch (void* e) {
			rc = -1;
		}
	}
#endif

	// save any changes to user config file
	if (configFileName == NULL) {
		pConfig->WriteToFile(pConfig->GetFileName());
	} else {
		free(configFileName);
	}

	if (pConfig->GetBoolValue(CONFIG_APP_REAL_TIME_SCHEDULER)) {
#ifdef _POSIX_MEMLOCK
		munlockall();
#endif /* _POSIX_MEMLOCK */
	}

	// cleanup
	delete pConfig;
	CapabilitiesCleanUp();
	exit (rc);
}

static volatile bool stop_signal_received, restart_recording_signal_received;

static void signal_stop_handler (int sig)
{
  error_message("Received stop signal %d", sig);
  stop_signal_received = true;
}

static void signal_restart_recording_handler (int sig)
{
  error_message("Received restart recoding signal %d", sig);
  restart_recording_signal_received = true;
}

int nogui_main(CLiveConfig* pConfig)
{
  double maxduration, duration;
  time_t starttime, nowtime;

	if (!pConfig->GetBoolValue(CONFIG_AUDIO_ENABLE)
	  && !pConfig->GetBoolValue(CONFIG_VIDEO_ENABLE) 
	    && !pConfig->GetBoolValue(CONFIG_TEXT_ENABLE)) {
		return -1;
	}

	InstallSignalHandler(pConfig->GetStringValue(CONFIG_APP_SIGNAL_HALT), 
			     signal_stop_handler,
			     true);
	InstallSignalHandler(pConfig->GetStringValue(CONFIG_APP_SIGNAL_REC_RESTART),
			     signal_restart_recording_handler);

	// override any configured value of preview
	bool previewValue =
		pConfig->GetBoolValue(CONFIG_VIDEO_PREVIEW);
	//#define TEST_PREVIEW 1
#ifndef TEST_PREVIEW
	pConfig->SetBoolValue(CONFIG_VIDEO_PREVIEW, false);

	CAVMediaFlow* pFlow = new CAVMediaFlow(pConfig);
#else
	CPreviewAVMediaFlow *pFlow = new CPreviewAVMediaFlow(pConfig);
#endif
	

	pFlow->Start();
	error_message("Started - wait");
	stop_signal_received = false;
	restart_recording_signal_received = false;

	maxduration = pConfig->GetIntegerValue(CONFIG_APP_DURATION)
	  * pConfig->GetIntegerValue(CONFIG_APP_DURATION_UNITS);
	
	starttime = time(NULL);
	do {
	  sleep(1);
#ifdef TEST_PREVIEW
	  pFlow->ProcessSDLEvents();
#endif
	  nowtime = time(NULL);
	  duration = difftime(nowtime, starttime);
	  if (restart_recording_signal_received) {
	    restart_recording_signal_received = false;
	    pFlow->RestartFileRecording();
	  }
	} while (duration < maxduration && stop_signal_received == 0);

	pFlow->Stop();

	delete pFlow;

	pConfig->SetBoolValue(CONFIG_VIDEO_PREVIEW, previewValue);

	return 0;
}

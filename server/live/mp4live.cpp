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

#define DECLARE_CONFIG_VARIABLES 1
#include "mp4live.h"
#include "media_flow.h"
#include <getopt.h>

int main(int argc, char** argv)
{
	int rc;
	char* configFileName = NULL;
	bool automatic = false;
	bool headless = false;
	extern int nogui_main(CLiveConfig* pConfig);
	extern int gui_main(int argc, char**argv, CLiveConfig* pConfig);

	// command line parsing
	while (true) {
		int c = -1;
		int option_index = 0;
		static struct option long_options[] = {
			{ "automatic", 0, 0, 'a' },
			{ "file", 1, 0, 'f' },
			{ "headless", 0, 0, 'h' },
			{ "version", 0, 0, 'v' },
			{ NULL, 0, 0, 0 }
		};

		c = getopt_long_only(argc, argv, "af:hv",
			long_options, &option_index);

		if (c == -1)
			break;

		switch (c) {
		case 'a':
			automatic = true;
			break;
		case 'f':	// -f <config-file>
			configFileName = stralloc(optarg);
			break;
		case 'h':
			headless = true;
			break;
		case 'v':
			fprintf(stderr, "%s version %s\n", argv[0], VERSION);
			exit(0);
		case '?':
		default:
			fprintf(stderr, 
				"Usage: %s [-f config_file] [--automatic] [--headless]\n",
				argv[0]);
			exit(-1);
		}
	}

	char userFileName[MAXPATHLEN];
	char* home = getenv("HOME");
	if (home) {
		strcpy(userFileName, home);
		strcat(userFileName, "/.mp4live_rc");
	}

	CLiveConfig* pConfig = NULL;
	try {
		pConfig = new CLiveConfig(MyConfigVariables,
			sizeof(MyConfigVariables) / sizeof(SConfigVariable),
			home ? userFileName : NULL);

		if (configFileName) {
			pConfig->ReadFromFile(configFileName);
		} else {
			// read user config file if present
			pConfig->ReadDefaultFile();
		}

		pConfig->m_appAutomatic = automatic;

#ifndef USE_DIVX_ENCODER
		pConfig->SetBoolValue(CONFIG_VIDEO_USE_DIVX_ENCODER, false);
#endif
	} 
	catch (CConfigException* e) {
		delete e;
	}

	CVideoSource::InitialVideoProbe(pConfig);

	pConfig->Regenerate();


	// EXPERIMENTAL
	if (pConfig->GetBoolValue(CONFIG_APP_USE_REAL_TIME)) {
#ifdef _POSIX_PRIORITY_SCHEDULING
		// put us into the lowest real-time scheduling queue
		struct sched_param sp;
		sp.sched_priority = 1;
		if (sched_setscheduler(0, SCHED_RR, &sp) < 0) {
			debug_message("Unable to set scheduling priority: %s",
				strerror(errno));
		}
#endif /* _POSIX_PRIORITY_SCHEDULING */
#ifdef _POSIX_MEMLOCK
		// recommendation is to reserve some stack pages
		u_int8_t huge[1024 * 1024];
		memset(huge, 1, sizeof(huge));

		// and then lock memory
		if (mlockall(MCL_CURRENT|MCL_FUTURE) < 0) {
			debug_message("Unable to lock memory: %s",
				strerror(errno));
		}
#endif /* _POSIX_MEMLOCK */
	}

#ifdef NOGUI
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
		pConfig->WriteDefaultFile();
	} else {
		free(configFileName);
	}

	if (pConfig->GetBoolValue(CONFIG_APP_USE_REAL_TIME)) {
#ifdef _POSIX_MEMLOCK
		munlockall();
#endif /* _POSIX_MEMLOCK */
	}

	delete pConfig;
	exit (rc);
}

int nogui_main(CLiveConfig* pConfig)
{
	if (!pConfig->GetBoolValue(CONFIG_AUDIO_ENABLE)
	  && !pConfig->GetBoolValue(CONFIG_VIDEO_ENABLE)) {
		return -1;
	}

	// override any configured value of preview
	bool previewValue =
		pConfig->GetBoolValue(CONFIG_VIDEO_PREVIEW);
	pConfig->SetBoolValue(CONFIG_VIDEO_PREVIEW, false);

	CAVMediaFlow* pFlow = new CAVMediaFlow(pConfig);

	pFlow->Start();

	sleep(pConfig->GetIntegerValue(CONFIG_APP_DURATION)
		* pConfig->GetIntegerValue(CONFIG_APP_DURATION_UNITS));

	pFlow->Stop();

	delete pFlow;

	pConfig->SetBoolValue(CONFIG_VIDEO_PREVIEW, previewValue);

	return 0;
}

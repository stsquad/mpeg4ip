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

#include "mp4live.h"

int main(int argc, char** argv)
{
	int rc;
	extern int gui_main(int argc, char**argv, CLiveConfig* pConfig);

	// OPTION to add command line arg parsing here
	// -f <config-file>

	// construct default configuration
	CLiveConfig* pConfig = new CLiveConfig();

	// read user config file if present
	pConfig->ReadUser();

	// DJM TEMP
	pConfig->m_videoDeviceName = "/dev/video0";
	pConfig->m_videoInput = 1;

	// OPTION to add a headless mode for automatic operation here

	// hand over control to GUI
	rc = gui_main(argc, argv, pConfig);

	delete pConfig;
	exit (rc);
}


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
 * Copyright (C) Cisco Systems Inc. 2001.  All Rights Reserved.
 * 
 * Contributor(s): 
 *		Dave Mackie		dmackie@cisco.com
 */

#include "mp4.h"

int main(int argc, char** argv)
{
	char* fileName;
	u_int32_t verbosity = 0;

	// -v option to control verbosity
	if (!strcmp(argv[1], "-v")) {
		verbosity = MP4_DETAILS_ALL;
		fileName = argv[2];
	} else {
		verbosity = MP4_DETAILS_ERROR;
		fileName = argv[1];
	}

	MP4FileHandle mp4File = MP4Read(fileName, verbosity);

	if (!mp4File) {
		exit(1);
	}

	MP4Dump(mp4File);

	MP4Close(mp4File);

	exit(0);
}


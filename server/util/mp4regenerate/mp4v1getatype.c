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
 */

#include <mpeg4ip.h>
#include <quicktime.h>

/*
 * mp4v1getfps
 * required arg1 should be the target file
 */ 
int main(int argc, char** argv)
{
	char* progName = argv[0];
	char* qtFileName = argv[1];
	quicktime_t* qtFile = NULL;
	char* acomp = NULL;

	/* open the Quicktime file */
	qtFile = quicktime_open(qtFileName, 1, 0, 0);
	if (qtFile == NULL) {
		fprintf(stderr, 
			"%s: error %s: %s\n",
			progName, qtFileName, strerror(errno));
		exit(4);
	}

	/* print answer to stdout */
	acomp = quicktime_audio_compressor(qtFile, 0);
	if (!strcmp(acomp, "mp4a")) {
		printf("aac\n");
	} else {
		printf("%s\n", acomp);
	}

	/* cleanup */
	quicktime_close(qtFile);
	return (0);
}


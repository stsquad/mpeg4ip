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

#include <sys/types.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <getopt.h>
#include <fcntl.h>
#include <unistd.h>
#include <quicktime.h>
#include <netinet/in.h>
#include <mpeg4ip.h>


/* globals */
char* progName;

/*
 * mp4dump
 * required arg1 should be the target file
 */ 
int main(int argc, char** argv)
{
	/* internal variables */
	char* qtFileName = NULL;
	quicktime_t* qtFile = NULL;

	/* begin process command line */
	progName = argv[0];
	while (1) {
		int c = -1;
		int option_index = 0;
		static struct option long_options[] = {
			{ NULL, 0, 0, 0 }
		};

		c = getopt_long_only(argc, argv, "",
			long_options, &option_index);

		if (c == -1)
			break;

		switch (c) {
		case '?':
			break;
		default:
			fprintf(stderr, "%s: unknown option specified, ignoring: %c\n", 
				progName, c);
		}
	}

	/* check that we have at least one non-option arguments */
	if ((argc - optind) < 1) {
		fprintf(stderr, 
			"usage: %s <mp4-file>\n",
			progName);
		exit(1);
	}

	/* point to the specified file names */
	qtFileName = argv[optind++];

	/* warn about extraneous non-option arguments */
	if (optind < argc) {
		fprintf(stderr, "%s: unknown options specified, ignoring: ", progName);
		while (optind < argc) {
			fprintf(stderr, "%s ", argv[optind++]);
		}
		fprintf(stderr, "\n");
	}

	/* end processing of command line */

	/* check if Quicktime file already exists */
	if (access(qtFileName, X_OK) == 0) {
		/* exists, check signature */
		if (!quicktime_check_sig(qtFileName)) {
			/* error, not a Quicktime file? */
			fprintf(stderr, 
				"%s: error %s: not a valid Quicktime file\n",
				progName, qtFileName);
			exit(3);
		}
	}

	/* open the Quicktime file */
	qtFile = quicktime_open(qtFileName, 1, 0, 0);
	if (qtFile == NULL) {
		fprintf(stderr, 
			"%s: error %s: %s\n",
			progName, qtFileName, strerror(errno));
		exit(4);
	}

	/* dump it to stdout */
	quicktime_dump(qtFile);

	/* cleanup */
	quicktime_close(qtFile);
	exit(0);
}


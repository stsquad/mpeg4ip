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

/* 
 * Notes:
 *  - file formatted with tabstops == 4 spaces 
 *  - TBD == "To Be Done" 
 */

#include <sys/types.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <mpeg4ip_getopt.h>
#include <fcntl.h>
#include <unistd.h>
#include <quicktime.h>
#include <mpeg4ip.h>


/* globals */
char* progName;

/* extern declarations */

/*
 * mp4extract
 * required arg1 should be the MP4 input file
 * required arg2 should be the output file for the extracted track 
 */ 
int main(int argc, char** argv)
{
	/* variables controlable from command line */
	bool extractVideo = TRUE;

	/* internal variables */
	char* mp4FileName = NULL;
	char* outFileName = NULL;
	quicktime_t* mp4File = NULL;
	FILE* outFile = NULL;
	int track = 0;
	int frame, numFrames;
	u_int8_t buf[64*1024];	/* big enough? */
	int bufSize;

	/* begin process command line */
	progName = argv[0];
	while (1) {
		int c = -1;
		int option_index = 0;
		static struct option long_options[] = {
			{ "audio", 0, 0, 'a' },
			{ "video", 0, 0, 'v' },
			{ NULL, 0, 0, 0 }
		};

		c = getopt_long_only(argc, argv, "av",
			long_options, &option_index);

		if (c == -1)
			break;

		switch (c) {
		case 'a': {
			extractVideo = FALSE;
			break;
		}
		case 'v': {
			extractVideo = TRUE;
			break;
		}
		case '?':
			break;
		default:
			fprintf(stderr, "%s: unknown option specified, ignoring: %c\n", 
				progName, c);
		}
	}

	/* check that we have at least two non-option arguments */
	if ((argc - optind) < 2) {
		fprintf(stderr, 
			"usage: %s <mpeg-file> <mov-file>\n",
			progName);
		exit(1);
	}

	/* point to the specified file names */
	mp4FileName = argv[optind++];
	outFileName = argv[optind++];

	/* warn about extraneous non-option arguments */
	if (optind < argc) {
		fprintf(stderr, "%s: unknown options specified, ignoring: ", progName);
		while (optind < argc) {
			fprintf(stderr, "%s ", argv[optind++]);
		}
		fprintf(stderr, "\n");
	}

	/* end processing of command line */

	/* open the MP4 file */
	mp4File = quicktime_open(mp4FileName, 1, 0, 0);
	if (mp4File == NULL) {
		fprintf(stderr, 
			"%s: error %s: %s\n",
			progName, mp4FileName, strerror(errno));
		exit(2);
	}

	/* open output file for writing */
	outFile = fopen(outFileName, "w");
	if (outFile == NULL) {
		fprintf(stderr,
			"%s: error opening %s: %s\n",
			progName, outFileName, strerror(errno));
		exit(3);
	}

	/* add MPEG-4 video config info at beginning of output file */
	if (extractVideo) {
		if (!strcasecmp(quicktime_video_compressor(mp4File, track), "mp4v")) {
			u_char* pConfig;
			int configSize;

			quicktime_get_mp4_video_decoder_config(mp4File, track,
				&pConfig, &configSize);
			
			if (fwrite(pConfig, 1, configSize, outFile) != configSize) {
				fprintf(stderr,
					"%s: write error: %s\n",
					progName, strerror(errno));
				exit(4);
			}
			free(pConfig);
		}
	}

	if (extractVideo) {
		numFrames = quicktime_video_length(mp4File, track);
	} else {
		numFrames = quicktime_audio_length(mp4File, track);
	}

	for (frame = 0; frame < numFrames; frame++) {
		if (extractVideo) {
			bufSize = quicktime_read_frame(mp4File, 
				&buf[0], track);
		} else { // extract audio
			bufSize = quicktime_read_audio_frame(mp4File, 
				&buf[0], sizeof(buf), track);
		}
		if (bufSize <= 0) {
			fprintf(stderr,
				"%s: read error: %s\n",
				progName, strerror(errno));
			exit(5);
		}

		if (fwrite(buf, 1, bufSize, outFile) != bufSize) {
			fprintf(stderr,
				"%s: write error: %s\n",
				progName, strerror(errno));
			exit(4);
		}
	}
	
	/* cleanup */
	quicktime_close(mp4File);
	fclose(outFile);
	exit(0);
}


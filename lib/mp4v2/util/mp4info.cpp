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
#include "mpeg4ip_getopt.h"

char* ProgName;
char* Mp4FileName;

int main(int argc, char** argv)
{
	char* usageString = 
		"usage: %s <file-name>\n";
	u_int32_t verbosity = MP4_DETAILS_ERROR;

	/* begin processing command line */
	ProgName = argv[0];
	while (true) {
		int c = -1;
		int option_index = 0;
		static struct option long_options[] = {
			{ "verbose", 2, 0, 'v' },
			{ NULL, 0, 0, 0 }
		};

		c = getopt_long_only(argc, argv, "v::",
			long_options, &option_index);

		if (c == -1)
			break;

		switch (c) {
		case 'v':
			verbosity |= MP4_DETAILS_READ;
			if (optarg) {
				u_int32_t level;
				if (sscanf(optarg, "%u", &level) == 1) {
					if (level >= 2) {
						verbosity |= MP4_DETAILS_TABLE;
					} 
					if (level >= 3) {
						verbosity |= MP4_DETAILS_SAMPLE;
					} 
					if (level >= 4) {
						verbosity = MP4_DETAILS_ALL;
					}
				}
			}
			break;
		case '?':
			fprintf(stderr, usageString, ProgName);
			exit(0);
		default:
			fprintf(stderr, "%s: unknown option specified, ignoring: %c\n", 
				ProgName, c);
		}
	}

	/* check that we have at least one non-option argument */
	if ((argc - optind) < 1) {
		fprintf(stderr, usageString, ProgName);
		exit(1);
	}

	/* point to the specified file names */
	Mp4FileName = argv[optind++];

	/* warn about extraneous non-option arguments */
	if (optind < argc) {
		fprintf(stderr, "%s: unknown options specified, ignoring: ", ProgName);
		while (optind < argc) {
			fprintf(stderr, "%s ", argv[optind++]);
		}
		fprintf(stderr, "\n");
	}

	/* end processing of command line */


	MP4FileHandle mp4File = MP4Read(Mp4FileName, verbosity);

	if (!mp4File) {
		exit(1);
	}

	u_int32_t numTracks = MP4GetNumberOfTracks(mp4File);

	printf("Id\tType\tAvgBitRate\n");
	for (u_int32_t i = 0; i < numTracks; i++) {
		MP4TrackId trackId = MP4FindTrackId(mp4File, i);

		const char* trackType = 
			MP4GetTrackType(mp4File, trackId);

		if (!strcmp(trackType, MP4_AUDIO_TRACK_TYPE)
		  || !strcmp(trackType, MP4_VIDEO_TRACK_TYPE)) {

			u_int32_t avgBitRate =
				MP4GetTrackIntegerProperty(mp4File, trackId, 
					"mdia.minf.stbl.stsd.*.esds.decConfigDescr.avgBitrate");

			printf("%u\t%s\t%u Kbps\n", 
				trackId, trackType, avgBitRate / 1000);

		} else {
			printf("%u\t%s\n", 
				trackId, trackType);
		}
	}

	MP4Close(mp4File);

	return(0);
}


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

main(int argc, char** argv)
{
	// TBD option for which track
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

	u_int32_t numTracks = MP4GetNumberOfTracks(mp4File);

	for (u_int32_t i = 0; i < numTracks; i++) {
		char trackFileName[MAXPATHLEN];
		snprintf(trackFileName, MAXPATHLEN, "%s.t%u", fileName, i + 1);

		int trackFd = open(trackFileName, 
			O_WRONLY | O_CREAT | O_TRUNC, 0644);
		if (trackFd == -1) {
			fprintf(stderr, "%s: can't open %s: %s\n",
				argv[0], trackFileName, strerror(errno));
			continue;
		}

		MP4TrackId trackId = MP4FindTrackId(mp4File, i);
		MP4SampleId numSamples = MP4GetNumberOfTrackSamples(mp4File, trackId);

		u_int8_t* pSample;
		u_int32_t sampleSize;

		for (MP4SampleId sampleId = 1; sampleId <= numSamples; sampleId++) {
			MP4ReadSample(mp4File, trackId, sampleId, &pSample, &sampleSize);
			if (write(trackFd, pSample, sampleSize) != sampleSize) {
				fprintf(stderr, "%s: write to %s failed: %s\n",
					argv[0], trackFileName, strerror(errno));
				break;
			}
			free(pSample);
		}

		close(trackFd);
	}

	MP4Close(mp4File);

	exit(0);
}


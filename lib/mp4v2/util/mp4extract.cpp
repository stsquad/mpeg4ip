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
char* Mp4PathName;
char* Mp4FileName;

// forward declaration
void PrintTrackList(MP4FileHandle mp4File);
void ExtractTrack(MP4FileHandle mp4File, MP4TrackId trackId, 
	bool sampleMode, char* dstFileName = NULL);


int main(int argc, char** argv)
{
	char* usageString = 
		"usage: %s [-l] [-t <track-id>] [-v [<level>]] <file-name>\n";
	bool doList = false;
	bool doSamples = false;
	MP4TrackId trackId = 0;
	char* dstFileName = NULL;
	u_int32_t verbosity = MP4_DETAILS_ERROR;

	/* begin processing command line */
	ProgName = argv[0];
	while (true) {
		int c = -1;
		int option_index = 0;
		static struct option long_options[] = {
			{ "list", 0, 0, 'l' },
			{ "track", 1, 0, 't' },
			{ "samples", 0, 0, 's' },
			{ "verbose", 2, 0, 'v' },
			{ "version", 0, 0, 'V' },
			{ NULL, 0, 0, 0 }
		};

		c = getopt_long_only(argc, argv, "lt:sv::V",
			long_options, &option_index);

		if (c == -1)
			break;

		switch (c) {
		case 'l':
			doList = true;
			break;
		case 's':
			doSamples = true;
			break;
		case 't':
			if (sscanf(optarg, "%u", &trackId) != 1) {
				fprintf(stderr, 
					"%s: bad track-id specified: %s\n",
					 ProgName, optarg);
				exit(1);
			}
			break;
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
		case 'V':
		  fprintf(stderr, "%s - %s version %s\n", 
			  ProgName, PACKAGE, VERSION);
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
	
	if (verbosity) {
		fprintf(stderr, "%s version %s\n", ProgName, VERSION);
	}

	/* point to the specified file names */
	Mp4PathName = argv[optind++];

	/* get dest file name for a single track */
	if (trackId && (argc - optind) > 0) {
		dstFileName = argv[optind++];
	}

	char* lastSlash = strrchr(Mp4PathName, '/');
	if (lastSlash) {
		Mp4FileName = lastSlash + 1;
	} else {
		Mp4FileName = Mp4PathName; 
	}

	/* warn about extraneous non-option arguments */
	if (optind < argc) {
		fprintf(stderr, "%s: unknown options specified, ignoring: ", ProgName);
		while (optind < argc) {
			fprintf(stderr, "%s ", argv[optind++]);
		}
		fprintf(stderr, "\n");
	}

	/* end processing of command line */


	MP4FileHandle mp4File = MP4Read(Mp4PathName, verbosity);

	if (!mp4File) {
		exit(1);
	}

	if (doList) {
		PrintTrackList(mp4File);
		exit(0);
	}

	if (trackId == 0) {
		u_int32_t numTracks = MP4GetNumberOfTracks(mp4File);

		for (u_int32_t i = 0; i < numTracks; i++) {
			trackId = MP4FindTrackId(mp4File, i);
			ExtractTrack(mp4File, trackId, doSamples);
		}
	} else {
		ExtractTrack(mp4File, trackId, doSamples, dstFileName);
	}

	MP4Close(mp4File);

	return(0);
}

void PrintTrackList(MP4FileHandle mp4File)
{
	u_int32_t numTracks = MP4GetNumberOfTracks(mp4File);

	printf("Id\tType\n");
	for (u_int32_t i = 0; i < numTracks; i++) {
		MP4TrackId trackId = MP4FindTrackId(mp4File, i);
		const char* trackType = MP4GetTrackType(mp4File, trackId);
		printf("%u\t%s\n", trackId, trackType);
	}
}

void ExtractTrack(MP4FileHandle mp4File, MP4TrackId trackId, 
	bool sampleMode, char* dstFileName)
{
	char outFileName[PATH_MAX];
	int outFd = -1;

	if (!sampleMode) {
		if (dstFileName == NULL) {
			snprintf(outFileName, sizeof(outFileName), 
				"%s.t%u", Mp4FileName, trackId);
		} else {
			snprintf(outFileName, sizeof(outFileName), 
				"%s", dstFileName);
		}

		outFd = open(outFileName, 
			O_WRONLY | O_CREAT | O_TRUNC, 0644);
		if (outFd == -1) {
			fprintf(stderr, "%s: can't open %s: %s\n",
				ProgName, outFileName, strerror(errno));
			return;
		}
	}

	MP4SampleId numSamples = 
		MP4GetTrackNumberOfSamples(mp4File, trackId);

	u_int8_t* pSample;
	u_int32_t sampleSize;

	for (MP4SampleId sampleId = 1; sampleId <= numSamples; sampleId++) {
		int rc;

		// signals to ReadSample() that it should malloc a buffer for us
		pSample = NULL;
		sampleSize = 0;

		rc = MP4ReadSample(mp4File, trackId, sampleId, &pSample, &sampleSize);
		if (rc == 0) {
			fprintf(stderr, "%s: read sample %u for %s failed\n",
				ProgName, sampleId, outFileName);
			break;
		}

		if (sampleMode) {
			snprintf(outFileName, sizeof(outFileName), "%s.t%u.s%u",
				Mp4FileName, trackId, sampleId);

			outFd = open(outFileName, 
				O_WRONLY | O_CREAT | O_TRUNC, 0644);

			if (outFd == -1) {
				fprintf(stderr, "%s: can't open %s: %s\n",
					ProgName, outFileName, strerror(errno));
				break;
			}
		}

		// in order to reconstruct video elementary stream
		// need to prepend ES configuration info 
		if (sampleId == 1 && !sampleMode) {
			const char* trackType =
				MP4GetTrackType(mp4File, trackId);

			if (!strcmp(trackType, MP4_VIDEO_TRACK_TYPE)) {
				u_int8_t videoType =
					MP4GetTrackVideoType(mp4File, trackId);

				if (videoType != MP4_YUV12_VIDEO_TYPE) {
					u_int8_t* pConfig = NULL;
					u_int32_t configSize;

					MP4GetTrackESConfiguration(mp4File, trackId, 
						&pConfig, &configSize);
					write(outFd, pConfig, configSize);

					free(pConfig);
				}
			}
		}

		rc = write(outFd, pSample, sampleSize);
		if (rc == -1 || (u_int32_t)rc != sampleSize) {
			fprintf(stderr, "%s: write to %s failed: %s\n",
				ProgName, outFileName, strerror(errno));
			break;
		}

		free(pSample);

		if (sampleMode) {
			close(outFd);
			outFd = -1;
		}
	}

	if (outFd != -1) {
		close(outFd);
	}
}


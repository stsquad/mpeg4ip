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
 * Copyright (C) Cisco Systems Inc. 2001-2002.  All Rights Reserved.
 * 
 * Contributor(s): 
 *		Dave Mackie		dmackie@cisco.com
 */

#define MP4CREATOR_GLOBALS
#include "mp4creator.h"
#include "mpeg4ip_getopt.h"

// forward declarations

MP4TrackId* CreateMediaTracks(
	MP4FileHandle mp4File, 
	const char* inputFileName);

void CreateHintTrack(
	MP4FileHandle mp4File, 
	MP4TrackId mediaTrackId,
	const char* payloadName, 
	bool interleave, 
	u_int16_t maxPayloadSize);

void ExtractTrack(
	MP4FileHandle mp4File, 
	MP4TrackId trackId, 
	const char* outputFileName);

// external declarations

// track creators
MP4TrackId* AviCreator(
	MP4FileHandle mp4File, const char* aviFileName);

MP4TrackId AacCreator(
	MP4FileHandle mp4File, FILE* inFile);

MP4TrackId Mp3Creator(
	MP4FileHandle mp4File, FILE* inFile);

MP4TrackId Mp4vCreator(
	MP4FileHandle mp4File, FILE* inFile);


// main routine
int main(int argc, char** argv)
{
	char* usageString = 
		"usage: %s <options> <mp4-file>\n"
		"  Options:\n"
		"  -create=<input-file>    Create track from <input-file>\n"
		"    input files can be of type: .aac .mp3 .divx .mp4v .m4v .cmp .xvid\n"
		"  -extract=<track-id>     Extract a track\n"
		"  -delete=<track-id>      Delete a track\n"
		"  -hint[=<track-id>]      Create hint track, also -H\n"
		"  -interleave             Use interleaved audio payload format, also -I\n"
		"  -list                   List tracks in mp4 file\n"
		"  -mtu=<size>             MTU for hint track\n"
		"  -optimize               Optimize mp4 file layout\n"
	        "  -payload=<payload>      Rtp payload type \n"
                "                          (use 3119 or mpa-robust for mp3 rfc 3119 support)\n"
		"  -rate=<fps>             Video frame rate, e.g. 30 or 29.97\n"
		"  -timescale=<ticks>      Time scale (ticks per second)\n"
		"  -verbose[=[1-5]]        Enable debug messages\n"
        "  -version                Display version information\n"
		;

	bool doCreate = false;
	bool doExtract = false;
	bool doDelete = false;
	bool doHint = false;
	bool doList = false;
	bool doOptimize = false;
	bool doInterleave = false;
	char* mp4FileName = NULL;
	char* inputFileName = NULL;
	char* outputFileName = NULL;
	char* payloadName = NULL;
	MP4TrackId hintTrackId = MP4_INVALID_TRACK_ID;
	MP4TrackId extractTrackId = MP4_INVALID_TRACK_ID;
	MP4TrackId deleteTrackId = MP4_INVALID_TRACK_ID;
	u_int16_t maxPayloadSize = 1460;

	Verbosity = MP4_DETAILS_ERROR;
	VideoFrameRate = 0;		// determine from input file
	Mp4TimeScale = 90000;

	// begin processing command line
	ProgName = argv[0];

	while (true) {
		int c = -1;
		int option_index = 0;
		static struct option long_options[] = {
			{ "create", 1, 0, 'c' },
			{ "delete", 1, 0, 'd' },
			{ "extract", 1, 0, 'e' },
			{ "help", 0, 0, '?' },
			{ "hint", 2, 0, 'H' },
			{ "interleave", 0, 0, 'I' },
			{ "list", 0, 0, 'l' },
			{ "mtu", 1, 0, 'm' },
			{ "optimize", 0, 0, 'O' },
			{ "payload", 1, 0, 'p' },
			{ "rate", 1, 0, 'r' },
			{ "timescale", 1, 0, 't' },
			{ "verbose", 2, 0, 'v' },
			{ "version", 0, 0, 'V' },
			{ NULL, 0, 0, 0 }
		};

		c = getopt_long_only(argc, argv, "c:d:e:H::Ilm:Op:r:t:v::V",
			long_options, &option_index);

		if (c == -1)
			break;

		switch (c) {
		case 'c':
			doCreate = true;
			inputFileName = optarg;
			break;
		case 'd':
			if (sscanf(optarg, "%u", &deleteTrackId) != 1) {
				fprintf(stderr, 
					"%s: bad track-id specified: %s\n",
					 ProgName, optarg);
				exit(EXIT_COMMAND_LINE);
			}
			doDelete = true;
			break;
		case 'e':
			if (sscanf(optarg, "%u", &extractTrackId) != 1) {
				fprintf(stderr, 
					"%s: bad track-id specified: %s\n",
					 ProgName, optarg);
				exit(EXIT_COMMAND_LINE);
			}
			doExtract = true;
			break;
		case 'H':
			doHint = true;
			if (optarg) {
				if (sscanf(optarg, "%u", &hintTrackId) != 1) {
					fprintf(stderr, 
						"%s: bad track-id specified: %s\n",
						 ProgName, optarg);
					exit(EXIT_COMMAND_LINE);
				}
			}
			break;
		case 'I':
			doInterleave = true;
			break;
		case 'l':
			doList = true;
			break;
		case 'm':
			u_int32_t mtu;
			if (sscanf(optarg, "%u", &mtu) != 1 || mtu < 64) {
				fprintf(stderr, 
					"%s: bad mtu specified: %s\n",
					 ProgName, optarg);
				exit(EXIT_COMMAND_LINE);
			}
			maxPayloadSize = mtu - 40;	// subtract IP, UDP, and RTP hdrs
			break;
		case 'O':
			doOptimize = true;
			break;
		case 'p':
			payloadName = optarg;
			break;
		case 'r':
			if (sscanf(optarg, "%f", &VideoFrameRate) != 1) {
				fprintf(stderr, 
					"%s: bad rate specified: %s\n",
					 ProgName, optarg);
				exit(EXIT_COMMAND_LINE);
			}
			break;
		case 't':
			if (sscanf(optarg, "%u", &Mp4TimeScale) != 1) {
				fprintf(stderr, 
					"%s: bad timescale specified: %s\n",
					 ProgName, optarg);
				exit(EXIT_COMMAND_LINE);
			}
			break;
		case 'v':
			Verbosity |= (MP4_DETAILS_READ | MP4_DETAILS_WRITE);
			if (optarg) {
				u_int32_t level;
				if (sscanf(optarg, "%u", &level) == 1) {
					if (level >= 2) {
						Verbosity |= MP4_DETAILS_TABLE;
					} 
					if (level >= 3) {
						Verbosity |= MP4_DETAILS_SAMPLE;
					} 
					if (level >= 4) {
						Verbosity |= MP4_DETAILS_HINT;
					} 
					if (level >= 5) {
						Verbosity = MP4_DETAILS_ALL;
					} 
				}
			}
			break;
		case '?':
			fprintf(stderr, usageString, ProgName);
			exit(EXIT_SUCCESS);
		case 'V':
		  fprintf(stderr, "%s - %s version %s\n", 
			  ProgName, PACKAGE, VERSION);
		  exit(EXIT_SUCCESS);
		default:
			fprintf(stderr, "%s: unknown option specified, ignoring: %c\n", 
				ProgName, c);
		}
	}

	// check that we have at least one non-option argument
	if ((argc - optind) < 1) {
		fprintf(stderr, usageString, ProgName);
		exit(EXIT_COMMAND_LINE);
	}

	if ((argc - optind) == 1) {
		mp4FileName = argv[optind++];
	} else {
		// it appears we have two file names
		if (doExtract) {
			mp4FileName = argv[optind++];
			outputFileName = argv[optind++];
		} else {
			if (inputFileName == NULL) {
				// then assume -c for the first file name
				doCreate = true;
				inputFileName = argv[optind++];
			}
			mp4FileName = argv[optind++];
		}
	}

	// warn about extraneous non-option arguments
	if (optind < argc) {
		fprintf(stderr, "%s: unknown options specified, ignoring: ", ProgName);
		while (optind < argc) {
			fprintf(stderr, "%s ", argv[optind++]);
		}
		fprintf(stderr, "\n");
	}

	// operations consistency checks

	if (!doList && !doCreate && !doHint 
	  && !doOptimize && !doExtract && !doDelete) {
		fprintf(stderr, 
			"%s: no operation specified\n",
			 ProgName);
		exit(EXIT_COMMAND_LINE);
	}
	if ((doCreate || doHint) && doExtract) {
		fprintf(stderr, 
			"%s: extract operation must be done separately\n",
			 ProgName);
		exit(EXIT_COMMAND_LINE);
	}
	if ((doCreate || doHint) && doDelete) {
		fprintf(stderr, 
			"%s: delete operation must be done separately\n",
			 ProgName);
		exit(EXIT_COMMAND_LINE);
	}
	if (doExtract && doDelete) {
		fprintf(stderr, 
			"%s: extract and delete operations must be done separately\n",
			 ProgName);
		exit(EXIT_COMMAND_LINE);
	}

	// end processing of command line

	if (doList) {
		// just want the track listing
		char* info = MP4Info(mp4FileName);

		if (!info) {
			fprintf(stderr, 
				"%s: can't open %s\n", 
				ProgName, mp4FileName);
			exit(EXIT_INFO);
		}

		fputs(info, stdout);
		free(info);
		exit(EXIT_SUCCESS);
	}

	// test if mp4 file exists
	bool mp4FileExists = (access(mp4FileName, F_OK) == 0);

	MP4FileHandle mp4File;

	if (doCreate || doHint) {
		if (!mp4FileExists) {
			if (doCreate) {
				mp4File = MP4Create(mp4FileName, Verbosity);
				if (mp4File) {
					MP4SetTimeScale(mp4File, Mp4TimeScale);
				}
			} else {
				fprintf(stderr,
					"%s: can't hint track in file that doesn't exist\n", 
					ProgName);
				exit(EXIT_CREATE_FILE);
			}
		} else {
			mp4File = MP4Modify(mp4FileName, Verbosity);
		}

		if (!mp4File) {
			// mp4 library should have printed a message
			exit(EXIT_CREATE_FILE);
		}

		bool allMpeg4Streams = true;

		if (doCreate) {
			MP4TrackId* pCreatedTrackIds = 
				CreateMediaTracks(mp4File, inputFileName);

			if (pCreatedTrackIds) {
				// decide if we can raise the ISMA compliance tag in SDP
				// we do this if audio and/or video are MPEG-4
				MP4TrackId* pTrackId = pCreatedTrackIds;

				while (*pTrackId != MP4_INVALID_TRACK_ID) {				
					const char *type =
						MP4GetTrackType(mp4File, *pTrackId);

					if (!strcmp(type, MP4_AUDIO_TRACK_TYPE)) { 
						allMpeg4Streams &=
							(MP4GetTrackAudioType(mp4File, *pTrackId) 
							== MP4_MPEG4_AUDIO_TYPE);

					} else if (!strcmp(type, MP4_VIDEO_TRACK_TYPE)) { 
						allMpeg4Streams &=
							(MP4GetTrackVideoType(mp4File, *pTrackId)
							== MP4_MPEG4_VIDEO_TYPE);
					}
					pTrackId++;
				}
			}

			if (pCreatedTrackIds && doHint) {
				MP4TrackId* pTrackId = pCreatedTrackIds;

				while (*pTrackId != MP4_INVALID_TRACK_ID) {
					CreateHintTrack(mp4File, *pTrackId, 
						payloadName, doInterleave, maxPayloadSize);
					pTrackId++;
				}
			}
		} else {
			CreateHintTrack(mp4File, hintTrackId, 
				payloadName, doInterleave, maxPayloadSize);
		} 

		MP4Close(mp4File);

		if (doCreate) {
			// this creates simple MPEG-4 OD and BIFS streams
			MP4MakeIsmaCompliant(mp4FileName, Verbosity, allMpeg4Streams);
		}


	} else if (doExtract) {
		if (!mp4FileExists) {
			fprintf(stderr,
				"%s: can't extract track in file that doesn't exist\n", 
				ProgName);
			exit(EXIT_CREATE_FILE);
		}

		mp4File = MP4Read(mp4FileName, Verbosity);
		if (!mp4File) {
			// mp4 library should have printed a message
			exit(EXIT_CREATE_FILE);
		}

		char tempName[PATH_MAX];
		if (outputFileName == NULL) {
			snprintf(tempName, sizeof(tempName), 
				"%s.t%u", mp4FileName, extractTrackId);
			outputFileName = tempName;
		}

		ExtractTrack(mp4File, extractTrackId, outputFileName);

		MP4Close(mp4File);

	} else if (doDelete) {
		if (!mp4FileExists) {
			fprintf(stderr,
				"%s: can't delete track in file that doesn't exist\n", 
				ProgName);
			exit(EXIT_CREATE_FILE);
		}

		mp4File = MP4Modify(mp4FileName, Verbosity);
		if (!mp4File) {
			// mp4 library should have printed a message
			exit(EXIT_CREATE_FILE);
		}

		MP4DeleteTrack(mp4File, deleteTrackId);

		MP4Close(mp4File);

		doOptimize = true;	// to purge unreferenced track data
	}

	if (doOptimize) {
		if (!MP4Optimize(mp4FileName, NULL, Verbosity)) {
			// mp4 library should have printed a message
			exit(EXIT_OPTIMIZE_FILE);
		}
	}

	return(EXIT_SUCCESS);
}

MP4TrackId* CreateMediaTracks(MP4FileHandle mp4File, const char* inputFileName)
{
	FILE* inFile = fopen(inputFileName, "rb");

	if (inFile == NULL) {
		fprintf(stderr, 
			"%s: can't open file %s: %s\n",
			ProgName, inputFileName, strerror(errno));
		exit(EXIT_CREATE_MEDIA);
	}

	struct stat s;
	if (fstat(fileno(inFile), &s) < 0) {
		fprintf(stderr, 
			"%s: can't stat file %s: %s\n",
			ProgName, inputFileName, strerror(errno));
		exit(EXIT_CREATE_MEDIA);
	}

	if (s.st_size == 0) {
		fprintf(stderr, 
			"%s: file %s is empty\n",
			ProgName, inputFileName);
		exit(EXIT_CREATE_MEDIA);
	}

	const char* extension = strrchr(inputFileName, '.');
	if (extension == NULL) {
		fprintf(stderr, 
			"%s: no file type extension\n", ProgName);
		exit(EXIT_CREATE_MEDIA);
	}

	static MP4TrackId trackIds[2] = {
		MP4_INVALID_TRACK_ID, MP4_INVALID_TRACK_ID
	};
	MP4TrackId* pTrackIds = trackIds;

	if (!strcasecmp(extension, ".avi")) {
		fclose(inFile);
		inFile = NULL;

		pTrackIds = AviCreator(mp4File, inputFileName);

	} else if (!strcasecmp(extension, ".aac")) {
		trackIds[0] = AacCreator(mp4File, inFile);

	} else if (!strcasecmp(extension, ".mp3")
	  || !strcasecmp(extension, ".mp1")
	  || !strcasecmp(extension, ".mp2")) {
		trackIds[0] = Mp3Creator(mp4File, inFile);

	} else if (!strcasecmp(extension, ".divx")
	  || !strcasecmp(extension, ".mp4v")
	  || !strcasecmp(extension, ".m4v")
	  || !strcasecmp(extension, ".xvid")
	  || !strcasecmp(extension, ".cmp")) {
		trackIds[0] = Mp4vCreator(mp4File, inFile);

	} else {
		fprintf(stderr, 
			"%s: unknown file type\n", ProgName);
		exit(EXIT_CREATE_MEDIA);
	}

	if (inFile) {
		fclose(inFile);
	}

	return pTrackIds;
}

void CreateHintTrack(MP4FileHandle mp4File, MP4TrackId mediaTrackId,
	const char* payloadName, bool interleave, u_int16_t maxPayloadSize)
{
	bool rc;

	if (MP4GetTrackNumberOfSamples(mp4File, mediaTrackId) == 0) {
		fprintf(stderr, 
			"%s: couldn't create hint track, no media samples\n", ProgName);
		exit(EXIT_CREATE_HINT);
	}

	// vector out to specific hinters
	const char* trackType = MP4GetTrackType(mp4File, mediaTrackId);

	if (!strcmp(trackType, MP4_AUDIO_TRACK_TYPE)) {
		u_int8_t audioType = MP4GetTrackAudioType(mp4File, mediaTrackId);

		switch (audioType) {
		case MP4_MPEG4_AUDIO_TYPE:
		case MP4_MPEG2_AAC_MAIN_AUDIO_TYPE:
		case MP4_MPEG2_AAC_LC_AUDIO_TYPE:
		case MP4_MPEG2_AAC_SSR_AUDIO_TYPE:
			rc = MP4AV_RfcIsmaHinter(mp4File, mediaTrackId, 
				interleave, maxPayloadSize);
			break;
		case MP4_MPEG1_AUDIO_TYPE:
		case MP4_MPEG2_AUDIO_TYPE:
			if (payloadName && 
			  (!strcasecmp(payloadName, "3119") 
			  || !strcasecmp(payloadName, "mpa-robust"))) {
				rc = MP4AV_Rfc3119Hinter(mp4File, mediaTrackId, 
					interleave, maxPayloadSize);
			} else {
				rc = MP4AV_Rfc2250Hinter(mp4File, mediaTrackId, 
					false, maxPayloadSize);
			}
			break;
		default:
			fprintf(stderr, 
				"%s: can't hint non-MPEG4/non-MP3 audio type\n", ProgName);
			exit(EXIT_CREATE_HINT);
		}
	} else if (!strcmp(trackType, MP4_VIDEO_TRACK_TYPE)) {
		u_int8_t videoType = MP4GetTrackVideoType(mp4File, mediaTrackId);

		if (videoType == MP4_MPEG4_VIDEO_TYPE) {
			rc = MP4AV_Rfc3016Hinter(mp4File, mediaTrackId, maxPayloadSize);
		} else {
			fprintf(stderr, 
				"%s: can't hint non-MPEG4 video type\n", ProgName);
			exit(EXIT_CREATE_HINT);
		}
	} else {
		fprintf(stderr, 
			"%s: can't hint track type %s\n", ProgName, trackType);
		exit(EXIT_CREATE_HINT);
	}

	if (!rc) {
		fprintf(stderr, 
			"%s: error hinting track %u\n", ProgName, mediaTrackId);
		exit(EXIT_CREATE_HINT);
	}
}

void ExtractTrack(
	MP4FileHandle mp4File, 
	MP4TrackId trackId, 
	const char* outputFileName)
{
	int outFd = open(outputFileName, 
			O_WRONLY | O_CREAT | O_TRUNC, 0644);

	if (outFd == -1) {
		fprintf(stderr, "%s: can't open %s: %s\n",
			ProgName, outputFileName, strerror(errno));
		exit(EXIT_EXTRACT_TRACK);
	}

	// some track types have special needs
	// to properly recreate their raw ES file

	bool prependES = false;
	bool prependADTS = false;

	const char* trackType =
		MP4GetTrackType(mp4File, trackId);

	if (!strcmp(trackType, MP4_VIDEO_TRACK_TYPE)) {
		if (MP4_IS_MPEG4_VIDEO_TYPE(MP4GetTrackVideoType(mp4File, trackId))) {
			prependES = true;
		}
	} else if (!strcmp(trackType, MP4_AUDIO_TRACK_TYPE)) {
		if (MP4_IS_AAC_AUDIO_TYPE(MP4GetTrackAudioType(mp4File, trackId))) {
			prependADTS = true;
		}
	}

	MP4SampleId numSamples = 
		MP4GetTrackNumberOfSamples(mp4File, trackId);
	u_int8_t* pSample;
	u_int32_t sampleSize;

	// extraction loop
	for (MP4SampleId sampleId = 1 ; sampleId <= numSamples; sampleId++) {
		int rc;

		// signal to ReadSample() 
		// that it should malloc a buffer for us
		pSample = NULL;
		sampleSize = 0;

		if (prependADTS) {
			// need some very specialized work for these
			MP4AV_AdtsMakeFrameFromMp4Sample(
				mp4File,
				trackId,
				sampleId,
				&pSample,
				&sampleSize);
		} else {
			// read the sample
			rc = MP4ReadSample(
				mp4File, 
				trackId, 
				sampleId, 
				&pSample, 
				&sampleSize);

			if (rc == 0) {
				fprintf(stderr, "%s: read sample %u for %s failed\n",
					ProgName, sampleId, outputFileName);
				exit(EXIT_EXTRACT_TRACK);
			}

			if (prependES && sampleId == 1) {
				u_int8_t* pConfig = NULL;
				u_int32_t configSize = 0;

				MP4GetTrackESConfiguration(mp4File, trackId, 
					&pConfig, &configSize);

				write(outFd, pConfig, configSize);

				free(pConfig);
			}
		}

		rc = write(outFd, pSample, sampleSize);

		if (rc == -1 || (u_int32_t)rc != sampleSize) {
			fprintf(stderr, "%s: write to %s failed: %s\n",
				ProgName, outputFileName, strerror(errno));
			exit(EXIT_EXTRACT_TRACK);
		}

		free(pSample);
	}

	// close ES file
	close(outFd);
}


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

// forward declarations
static void PrintAudioInfo(MP4FileHandle mp4File, MP4TrackId trackId);
static void PrintVideoInfo(MP4FileHandle mp4File, MP4TrackId trackId);
static void PrintHintInfo(MP4FileHandle mp4File, MP4TrackId trackId);

// globals
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
			{ "version", 0, 0, 'V' },
			{ NULL, 0, 0, 0 }
		};

		c = getopt_long_only(argc, argv, "v::V",
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
		case 'V':
		  fprintf(stderr, "%s - %s version %s\n", ProgName, 
			  PACKAGE, VERSION);
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
	printf("%s version %s\n", ProgName, VERSION);

	MP4FileHandle mp4File = MP4Read(Mp4FileName, verbosity);

	if (!mp4File) {
		exit(1);
	}

	u_int32_t numTracks = MP4GetNumberOfTracks(mp4File);

	printf("Id\tType\tInfo\n");
	for (u_int32_t i = 0; i < numTracks; i++) {
		MP4TrackId trackId = MP4FindTrackId(mp4File, i);

		const char* trackType = 
			MP4GetTrackType(mp4File, trackId);

		if (!strcmp(trackType, MP4_AUDIO_TRACK_TYPE)) {
			PrintAudioInfo(mp4File, trackId);
		} else if (!strcmp(trackType, MP4_VIDEO_TRACK_TYPE)) {
			PrintVideoInfo(mp4File, trackId);
		} else if (!strcmp(trackType, MP4_HINT_TRACK_TYPE)) {
			PrintHintInfo(mp4File, trackId);
		} else {
			printf("%u\t%s\n", 
				trackId, trackType);
		}
	}

	MP4Close(mp4File);

	return(0);
}

static void PrintAudioInfo(MP4FileHandle mp4File, MP4TrackId trackId)
{
	static const char* mpeg4AudioNames[] = {
		"MPEG-4 Main @ L1",
		"MPEG-4 Main @ L2",
		"MPEG-4 Main @ L3",
		"MPEG-4 Main @ L4",
		"MPEG-4 Scalable @ L1",
		"MPEG-4 Scalable @ L2",
		"MPEG-4 Scalable @ L3",
		"MPEG-4 Scalable @ L4",
		"MPEG-4 Speech @ L1",
		"MPEG-4 Speech @ L2",
		"MPEG-4 Synthesis @ L1",
		"MPEG-4 Synthesis @ L2",
		"MPEG-4 Synthesis @ L3",
	};
	static u_int8_t numMpeg4AudioTypes = 
		sizeof(mpeg4AudioNames) / sizeof(char*);

	static u_int8_t mpegAudioTypes[] = {
		MP4_MPEG2_AAC_MAIN_AUDIO_TYPE,	// 0x66
		MP4_MPEG2_AAC_LC_AUDIO_TYPE,	// 0x67
		MP4_MPEG2_AAC_SSR_AUDIO_TYPE,	// 0x68
		MP4_MPEG2_AUDIO_TYPE,			// 0x69
		MP4_MPEG1_AUDIO_TYPE,			// 0x6B
	};
	static const char* mpegAudioNames[] = {
		"MPEG-2 AAC Main",
		"MPEG-2 AAC LC",
		"MPEG-2 AAC SSR",
		"MPEG-2 (MP3)",
		"MPEG-1 (MP3)"
	};
	static u_int8_t numMpegAudioTypes = 
		sizeof(mpegAudioTypes) / sizeof(u_int8_t);

	u_int8_t type =
		MP4GetTrackAudioType(mp4File, trackId);
	const char* typeName = "Unknown";

	if (type == MP4_MPEG4_AUDIO_TYPE) {
		type = MP4GetAudioProfileLevel(mp4File);
		if (type > 0 && type <= numMpeg4AudioTypes) {
			typeName = mpeg4AudioNames[type - 1];
		} else {
			typeName = "MPEG-4";
		}
	} else {
		for (u_int8_t i = 0; i < numMpegAudioTypes; i++) {
			if (type == mpegAudioTypes[i]) {
				typeName = mpegAudioNames[i];
				break;
			}
		}
	}

	u_int32_t timeScale =
		MP4GetTrackTimeScale(mp4File, trackId);

	MP4Duration trackDuration =
		MP4GetTrackDuration(mp4File, trackId);

	u_int64_t msDuration =
		MP4ConvertFromTrackDuration(mp4File, trackId, 
			trackDuration, MP4_MSECS_TIME_SCALE);

	u_int32_t avgBitRate =
		MP4GetTrackIntegerProperty(mp4File, trackId, 
			"mdia.minf.stbl.stsd.mp4a.esds.decConfigDescr.avgBitrate");

	// type duration avgBitrate samplingFrequency
	printf("%u\taudio\t%s, %.3f secs, %u kbps, %u Hz\n", 
		trackId, 
		typeName,
		(float)msDuration / 1000.0, 
		(avgBitRate + 500) / 1000, 
		timeScale);
}

static void PrintVideoInfo(MP4FileHandle mp4File, MP4TrackId trackId)
{
	static const char* mpeg4VideoNames[] = {
		"MPEG-4 Simple @ L3",
		"MPEG-4 Simple @ L2",
		"MPEG-4 Simple @ L1",
		"MPEG-4 Simple Scalable @ L2",
		"MPEG-4 Simple Scalable @ L1",
		"MPEG-4 Core @ L2",
		"MPEG-4 Core @ L1",
		"MPEG-4 Main @ L4",
		"MPEG-4 Main @ L3",
		"MPEG-4 Main @ L2",
		"MPEG-4 Main @ L1",
		"MPEG-4 N-Bit @ L2",
		"MPEG-4 Hybrid @ L2",
		"MPEG-4 Hybrid @ L1",
		"MPEG-4 Hybrid @ L1",
	};
	static u_int8_t numMpeg4VideoTypes = 
		sizeof(mpeg4VideoNames) / sizeof(char*);

	static u_int8_t mpegVideoTypes[] = {
		MP4_MPEG2_SIMPLE_VIDEO_TYPE,	// 0x60
		MP4_MPEG2_MAIN_VIDEO_TYPE,		// 0x61
		MP4_MPEG2_SNR_VIDEO_TYPE,		// 0x62
		MP4_MPEG2_SPATIAL_VIDEO_TYPE,	// 0x63
		MP4_MPEG2_HIGH_VIDEO_TYPE,		// 0x64
		MP4_MPEG2_442_VIDEO_TYPE,		// 0x65
		MP4_MPEG1_VIDEO_TYPE,			// 0x6A
		MP4_JPEG_VIDEO_TYPE,			// 0x6C
	};
	static const char* mpegVideoNames[] = {
		"MPEG-2 Simple",
		"MPEG-2 Main",
		"MPEG-2 SNR",
		"MPEG-2 Spatial",
		"MPEG-2 High",
		"MPEG-2 4:2:2",
		"MPEG-1",
		"JPEG"
	};
	static u_int8_t numMpegVideoTypes = 
		sizeof(mpegVideoTypes) / sizeof(u_int8_t);

	u_int8_t type =
		MP4GetTrackVideoType(mp4File, trackId);
	const char* typeName = "Unknown";

	if (type == MP4_MPEG4_VIDEO_TYPE) {
		type = MP4GetVideoProfileLevel(mp4File);
		if (type > 0 && type <= numMpeg4VideoTypes) {
			typeName = mpeg4VideoNames[type - 1];
		} else {
			typeName = "MPEG-4";
		}
	} else {
		for (u_int8_t i = 0; i < numMpegVideoTypes; i++) {
			if (type == mpegVideoTypes[i]) {
				typeName = mpegVideoNames[i];
				break;
			}
		}
	}

	MP4Duration trackDuration =
		MP4GetTrackDuration(mp4File, trackId);

	u_int64_t msDuration =
		MP4ConvertFromTrackDuration(mp4File, trackId, 
			trackDuration, MP4_MSECS_TIME_SCALE);

	u_int32_t avgBitRate =
		MP4GetTrackIntegerProperty(mp4File, trackId, 
			"mdia.minf.stbl.stsd.mp4v.esds.decConfigDescr.avgBitrate");

	// Note not all mp4 implementations set width and height correctly
	// The real answer can be buried inside the ES configuration info
	u_int16_t width = MP4GetTrackVideoWidth(mp4File, trackId); 

	u_int16_t height = MP4GetTrackVideoHeight(mp4File, trackId); 

	// Note if variable rate video is being used the fps is an average 
	u_int32_t numFrames = 
		MP4GetTrackNumberOfSamples(mp4File, trackId); 

	float fps = 
		((float)numFrames / (float)msDuration) * 1000;

	// type duration avgBitrate frameSize frameRate
	printf("%u\tvideo\t%s, %.3f secs, %u kbps, %ux%u @ %.2f fps\n", 
		trackId, 
		typeName,
		(float)msDuration / 1000.0, 
		(avgBitRate + 500) / 1000,
		width,	
		height,
		fps
	);
}

static void PrintHintInfo(MP4FileHandle mp4File, MP4TrackId trackId)
{
	MP4TrackId referenceTrackId =
		MP4GetHintTrackReferenceTrackId(mp4File, trackId);

	char* payloadName = NULL;
	MP4GetHintTrackRtpPayload(mp4File, trackId, &payloadName);		

	printf("%u\thint\tPayload %s for track %u\n", 
		trackId, 
		payloadName,
		referenceTrackId
	);
}


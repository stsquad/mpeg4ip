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

#include "mp4common.h"

static char* PrintAudioInfo(
	MP4FileHandle mp4File, 
	MP4TrackId trackId)
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
		MP4_PCM16_AUDIO_TYPE,			
		MP4_VORBIS_AUDIO_TYPE,			
	};
	static const char* mpegAudioNames[] = {
		"MPEG-2 AAC Main",
		"MPEG-2 AAC LC",
		"MPEG-2 AAC SSR",
		"MPEG-2 (MP3)",
		"MPEG-1 (MP3)",
		"PCM16",
		"OGG VORBIS",
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
		MP4GetTrackBitRate(mp4File, trackId);

	char *sInfo = (char*)MP4Malloc(256);
	double duration;
	duration = 
#ifdef _WIN32
		(int64_t)
#endif
		msDuration;

	// type duration avgBitrate samplingFrequency
	sprintf(sInfo,	
		"%u\taudio\t%s, %.3f secs, %u kbps, %u Hz\n", 
		trackId, 
		typeName,
		duration / 1000.0, 
		(avgBitRate + 500) / 1000, 
		timeScale);

	return sInfo;
}

static char* PrintVideoInfo(
	MP4FileHandle mp4File, 
	MP4TrackId trackId)
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
		MP4_YUV12_VIDEO_TYPE,			
		MP4_H26L_VIDEO_TYPE			
	};
	static const char* mpegVideoNames[] = {
		"MPEG-2 Simple",
		"MPEG-2 Main",
		"MPEG-2 SNR",
		"MPEG-2 Spatial",
		"MPEG-2 High",
		"MPEG-2 4:2:2",
		"MPEG-1",
		"JPEG",
		"YUV12",
		"H26L"
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
		MP4GetTrackBitRate(mp4File, trackId);

	// Note not all mp4 implementations set width and height correctly
	// The real answer can be buried inside the ES configuration info
	u_int16_t width = MP4GetTrackVideoWidth(mp4File, trackId); 

	u_int16_t height = MP4GetTrackVideoHeight(mp4File, trackId); 

	float fps = MP4GetTrackVideoFrameRate(mp4File, trackId);

	char *sInfo = (char*)MP4Malloc(256);
	double duration;
	duration = 
#ifdef _WIN32
		(int64_t)
#endif	
		msDuration;
	// type duration avgBitrate frameSize frameRate
	sprintf(sInfo, 
		"%u\tvideo\t%s, %.3f secs, %u kbps, %ux%u @ %.2f fps\n", 
		trackId, 
		typeName,
		duration / 1000.0, 
		(avgBitRate + 500) / 1000,
		width,	
		height,
		fps
	);

	return sInfo;
}

static char* PrintHintInfo(
	MP4FileHandle mp4File, 
	MP4TrackId trackId)
{
	MP4TrackId referenceTrackId =
		MP4GetHintTrackReferenceTrackId(mp4File, trackId);

	char* payloadName = NULL;
	MP4GetHintTrackRtpPayload(mp4File, trackId, &payloadName);		

	char *sInfo = (char*)MP4Malloc(256);

	sprintf(sInfo,
		"%u\thint\tPayload %s for track %u\n", 
		trackId, 
		payloadName,
		referenceTrackId);

	free(payloadName);

	return sInfo;
}

extern "C" char* MP4Info(
	const char* fileName)
{
	try {
		MP4FileHandle mp4File = 
			MP4Read(fileName);

		if (!mp4File) {
			return NULL;
		}

		char* fileInfo = (char*)MP4Calloc(4*1024);

		sprintf(fileInfo, "Track\tType\tInfo\n");

		u_int32_t numTracks = MP4GetNumberOfTracks(mp4File);

		for (u_int32_t i = 0; i < numTracks; i++) {
			MP4TrackId trackId = 
				MP4FindTrackId(mp4File, i);
			const char* trackType = 
				MP4GetTrackType(mp4File, trackId);
			char* trackInfo = NULL;

			if (!strcmp(trackType, MP4_AUDIO_TRACK_TYPE)) {
				trackInfo = PrintAudioInfo(mp4File, trackId);
			} else if (!strcmp(trackType, MP4_VIDEO_TRACK_TYPE)) {
				trackInfo = PrintVideoInfo(mp4File, trackId);
			} else if (!strcmp(trackType, MP4_HINT_TRACK_TYPE)) {
				trackInfo = PrintHintInfo(mp4File, trackId);
			} else {
				trackInfo = (char*)MP4Malloc(256);
				if (!strcmp(trackType, MP4_OD_TRACK_TYPE)) {
					sprintf(trackInfo, 
						"%u\tod\tObject Descriptors\n", 
						trackId);
				} else if (!strcmp(trackType, MP4_SCENE_TRACK_TYPE)) {
					sprintf(trackInfo,
						"%u\tscene\tBIFS\n", 
						trackId);
				} else {
					sprintf(trackInfo,
						"%u\t%s\n", 
						trackId, trackType);
				}
			}

			strcat(fileInfo, trackInfo);
			MP4Free(trackInfo);
		}

		MP4Close(mp4File);

		return fileInfo;	// caller should free this
	}
	catch (MP4Error* e) {
		delete e;
	}

	return NULL;
}


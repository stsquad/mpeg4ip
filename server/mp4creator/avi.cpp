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
 */

#include <mp4creator.h>
#include <avilib.h>

static MP4TrackId VideoCreator(MP4FileHandle mp4File, avi_t* aviFile)
{
	char* videoType = AVI_video_compressor(aviFile);

	if (strcasecmp(videoType, "divx")
	  && strcasecmp(videoType, "xvid")) {
		fprintf(stderr,	
			"%s: video compressor %s not recognized\n",
			 ProgName, videoType);
		exit(EXIT_AVI_CREATOR);
	}

	double frameRate = AVI_video_frame_rate(aviFile);

	if (frameRate == 0) {
		fprintf(stderr,	
			"%s: no video frame rate in avi file\n", ProgName);
		exit(EXIT_AVI_CREATOR);
	}

	MP4Duration mp4TimeScale = 90000;
#ifdef _WIN32
	MP4Duration mp4FrameDuration;
	mp4FrameDuration = (MP4Duration)((int64_t)(mp4TimeScale));
	mp4FrameDuration /= (int64_t)frameRate;
#else
	MP4Duration mp4FrameDuration = (MP4Duration)(mp4TimeScale / frameRate);
#endif

	MP4TrackId trackId = MP4AddVideoTrack(mp4File,
		mp4TimeScale, mp4FrameDuration, 
		AVI_video_width(aviFile), AVI_video_height(aviFile), 
		MP4_MPEG4_VIDEO_TYPE);

	if (trackId == MP4_INVALID_TRACK_ID) {
		fprintf(stderr,	
			"%s: can't create video track\n", ProgName);
		exit(EXIT_AVI_CREATOR);
	}

	int32_t i;
	int32_t numFrames = AVI_video_frames(aviFile);
	int32_t frameSize;
	int32_t maxFrameSize = 0;

	// determine maximum video frame size
	AVI_seek_start(aviFile);

	for (i = 0; i < numFrames; i++) {
		frameSize = AVI_frame_size(aviFile, i);
		if (frameSize > maxFrameSize) {
			maxFrameSize = frameSize;
		}
	}

	// allocate a large enough frame buffer
	u_int8_t* pFrameBuffer = (u_int8_t*)malloc(maxFrameSize);

	if (pFrameBuffer == NULL) {
		fprintf(stderr,	
			"%s: can't allocate memory\n", ProgName);
		exit(EXIT_AVI_CREATOR);
	}

	AVI_seek_start(aviFile);

	// read first frame, should contain VO/VOL and I-VOP
	frameSize = AVI_read_frame(aviFile, (char*)pFrameBuffer);

	if (frameSize < 0) {
		fprintf(stderr,	
			"%s: can't read video frame 1: %s\n",
			ProgName, AVI_strerror());
		exit(EXIT_AVI_CREATOR);
	}

	// find VOP start code in first sample
	static u_int8_t vopStartCode[4] = { 
		0x00, 0x00, 0x01, MP4AV_MPEG4_VOP_START 
	};

	for (i = 0; i < frameSize - 4; i++) {
		if (!memcmp(&pFrameBuffer[i], vopStartCode, 4)) {
			// everything before the VOP
			// should be configuration info
			MP4SetTrackESConfiguration(mp4File, trackId,
				pFrameBuffer, i);
		}
	}

	if (MP4GetNumberOfTracks(mp4File, MP4_VIDEO_TRACK_TYPE) == 1) {
		MP4SetVideoProfileLevel(mp4File, 0x01);
	}

	// write out the first frame, including the initial configuration info
	MP4WriteSample(mp4File, trackId, 
		pFrameBuffer, frameSize, mp4FrameDuration, 0, true);

	// process the rest of the frames
	for (i = 1; i < numFrames; i++) {
		// read the frame from the AVI file
		frameSize = AVI_read_frame(aviFile, (char*)pFrameBuffer);

		if (frameSize < 0) {
			fprintf(stderr,	
				"%s: can't read video frame %i: %s\n",
				ProgName, i + 1, AVI_strerror());
			exit(EXIT_AVI_CREATOR);
		}

		// we mark random access points in MP4 files
		bool isIFrame = 
			(MP4AV_Mpeg4GetVopType(pFrameBuffer, frameSize) == 'I');

		// write the frame to the MP4 file
		MP4WriteSample(mp4File, trackId, 
			pFrameBuffer, frameSize, mp4FrameDuration, 0, isIFrame);
	}

	return trackId;
}

static inline u_int32_t BytesToInt32(u_int8_t* pBytes) 
{
	return (pBytes[0] << 24) | (pBytes[1] << 16) 
		| (pBytes[2] << 8) | pBytes[3];
}

static MP4TrackId AudioCreator(MP4FileHandle mp4File, avi_t* aviFile)
{
	int32_t audioType = AVI_audio_format(aviFile);

	// Check for MP3 audio type
	if (audioType != 0x55) {
		fprintf(stderr,	
			"%s: audio compressor 0x%x not recognized\n",
			 ProgName, audioType);
		exit(EXIT_AVI_CREATOR);
	}

	u_int8_t temp[4];
	u_int32_t mp3header;

	AVI_seek_start(aviFile);

	if (AVI_read_audio(aviFile, (char*)&temp, 4) != 4) {
		fprintf(stderr,	
			"%s: can't read audio frame 0: %s\n",
			ProgName, AVI_strerror());
		exit(EXIT_AVI_CREATOR);
	}
	mp3header = BytesToInt32(temp);

	// check mp3header sanity
	if ((mp3header & 0xFFE00000) != 0xFFE00000) {
		fprintf(stderr,	
			"%s: data in file doesn't appear to be valid mp3 audio\n",
			ProgName);
		exit(EXIT_AVI_CREATOR);
	}

	u_int16_t samplesPerSecond = 
		MP4AV_Mp3GetHdrSamplingRate(mp3header);
	u_int16_t samplesPerFrame = 
		MP4AV_Mp3GetHdrSamplingWindow(mp3header);
	u_int8_t mp4AudioType = 
		MP4AV_Mp3ToMp4AudioType(MP4AV_Mp3GetHdrVersion(mp3header));

	if (audioType == MP4_INVALID_AUDIO_TYPE
	  || samplesPerSecond == 0) {
		fprintf(stderr,	
			"%s: data in file doesn't appear to be valid mp3 audio\n",
			 ProgName);
		exit(EXIT_AVI_CREATOR);
	}

	MP4TrackId trackId = MP4AddAudioTrack(mp4File, 
		samplesPerSecond, samplesPerFrame, mp4AudioType);

	if (trackId == MP4_INVALID_TRACK_ID) {
		fprintf(stderr,	
			"%s: can't create audio track\n", ProgName);
		exit(EXIT_AVI_CREATOR);
	}

	if (MP4GetNumberOfTracks(mp4File, MP4_AUDIO_TRACK_TYPE) == 1) {
		MP4SetAudioProfileLevel(mp4File, 0xFE);
	}

	int32_t i;
	int32_t aviFrameSize;
	int32_t maxAviFrameSize = 0;

	// determine maximum avi audio chunk size
	// should be at least as large as maximum mp3 frame size
	AVI_seek_start(aviFile);

	i = 0;
	while (AVI_set_audio_frame(aviFile, i, (long*)&aviFrameSize) == 0) {
		if (aviFrameSize > maxAviFrameSize) {
			maxAviFrameSize = aviFrameSize;
		}
		i++;
	}

	// allocate a large enough frame buffer
	u_int8_t* pFrameBuffer = (u_int8_t*)malloc(maxAviFrameSize);

	if (pFrameBuffer == NULL) {
		fprintf(stderr,	
			"%s: can't allocate memory\n", ProgName);
		exit(EXIT_AVI_CREATOR);
	}

	AVI_seek_start(aviFile);

	u_int32_t mp3FrameNumber = 1;
	while (true) {
		if (AVI_read_audio(aviFile, (char*)pFrameBuffer, 4) != 4) {
			// EOF presumably
			break;
		}

		mp3header = BytesToInt32(&pFrameBuffer[0]);

		u_int16_t mp3FrameSize = MP4AV_Mp3GetFrameSize(mp3header);

		if (AVI_read_audio(aviFile, (char*)&pFrameBuffer[4], mp3FrameSize - 4)
		  != mp3FrameSize - 4) {
			fprintf(stderr,	
				"%s: can't read audio frame %u: %s\n",
				ProgName, mp3FrameNumber, AVI_strerror());
			exit(EXIT_AVI_CREATOR);
		}

		if (!MP4WriteSample(mp4File, trackId, 
		  &pFrameBuffer[0], mp3FrameSize)) {
			fprintf(stderr,	
				"%s: can't write audio frame %u\n", ProgName, mp3FrameNumber);
			exit(EXIT_AVI_CREATOR);
		}
	
		mp3FrameNumber++;
	}

	return trackId;
}

MP4TrackId* AviCreator(MP4FileHandle mp4File, const char* aviFileName)
{
	static MP4TrackId trackIds[3];
	u_int8_t numTracks = 0;

	avi_t* aviFile = AVI_open_input_file(aviFileName, true);
	if (aviFile == NULL) {
		fprintf(stderr,	
			"%s: can't open %s: %s\n",
			ProgName, aviFileName, AVI_strerror());
		exit(EXIT_AVI_CREATOR);
	}

	if (AVI_video_frames(aviFile) > 0) {
		trackIds[numTracks++] = VideoCreator(mp4File, aviFile);
	}

	if (AVI_audio_bytes(aviFile) > 0) {
		trackIds[numTracks++] = AudioCreator(mp4File, aviFile);
	}

	trackIds[numTracks] = MP4_INVALID_TRACK_ID;

	AVI_close(aviFile);

	return trackIds;
}


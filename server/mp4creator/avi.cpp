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
	MP4TrackId trackId = 0; 

	// TBD avi video

	return trackId;
}

static MP4TrackId AudioCreator(MP4FileHandle mp4File, avi_t* aviFile)
{
	MP4TrackId trackId = 0; 
#ifdef NOTDEF
		MP4AddAudioTrack(mp4File, 
			samplesPerSecond, samplesPerFrame, audioType);

	if (trackId == MP4_INVALID_TRACK_ID) {
		fprintf(stderr,	
			"%s: can't create audio track\n", ProgName);
		exit(EXIT_AVI_CREATOR);
	}

	if (MP4GetNumberOfTracks(mp4File, MP4_AUDIO_TRACK_TYPE) == 1) {
		MP4SetAudioProfileLevel(mp4File, 0xFE);
	}

	u_int8_t sampleBuffer[8 * 1024];
	u_int32_t sampleSize = sizeof(sampleBuffer);
	MP4SampleId sampleId = 1;

	AVI_seek_start(aviFile);

	while (xxx(aviFile, sampleBuffer, &sampleSize)) {
		if (!MP4WriteSample(mp4File, trackId, sampleBuffer, sampleSize)) {
			fprintf(stderr,	
				"%s: can't write audio frame %u\n", ProgName, sampleId);
			exit(EXIT_AVI_CREATOR);
		}
		sampleId++;
		sampleSize = sizeof(sampleBuffer);
	}
#endif
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


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
	if (argc < 2) {
		fprintf(stderr, "Usage: %s <file>\n", argv[0]);
		exit(1);
	}

	u_int32_t verbosity = MP4_DETAILS_ALL;

	try {
		char* fileName = argv[1];

		// open the mp4 file
		MP4File* pFile = new MP4File(fileName, "r", verbosity);

		u_int8_t profileLevel = pFile->GetVideoProfileLevel();

		// get a handle on the first video track
		MP4TrackId trackId = pFile->FindTrackId(0, "video");

		// gather the crucial track information 

		u_int32_t timeScale = pFile->GetTrackTimeScale(trackId);

		// note all times and durations 
		// are in units of the track time scale

		MP4Duration trackDuration = pFile->GetTrackDuration(trackId);

		MP4SampleId numSamples = pFile->GetNumberofTrackSamples(trackId);

		u_int8_t* pConfig;
		u_int32_t configSize;

		pFile->GetTrackESConfiguration(trackId, &pConfig, &configSize);

		// initialize decoder with Elementary Stream (ES) configuration

		free(pConfig);

		// now consecutively read and display the track samples

		u_int8_t* pSample;
		u_int32_t sampleSize;
		MP4Timestamp sampleTime;
		MP4Duration sampleDuration;
		MP4Duration sampleRenderingOffset;
		bool isSyncSample;

		for (MP4SampleId sampleId = 1; sampleId <= numSamples; sampleId++) {

			// read next sample from video track
			pFile->ReadSample(trackId, sampleId, 
				&pSample, &sampleSize,
				&sampleTime, &sampleDuration, &sampleRenderingOffset, 
				&isSyncSample);

			// decode frame and display it

			// free sample buffer
			free(pSample);
		}

		// close mp4 file
		delete pFile;


		// Note to seek to time 'when' in the track
		// use GetSampleIdFromTime(MP4Timestamp when, bool wantSyncSample)
		// 'wantSyncSample' determines if a sync sample is desired or not
		// e.g.
		// MP4SampleId newSampleId = pFile->GetSampleIdFromTime(when, true); 
		// pFile->ReadSample(trackId, newSampleId, ...);
	}

	catch (MP4Error* e) {
		// if verbosity is on,
		// library will already have printed an error message, 
		// just print it ourselves if we were in quiet mode
		if (!verbosity) {
			e->Print();
		}
		exit(1);
	}
	exit(0);
}


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

void mp4dump(int argc, char** argv)
{
	MP4File* pFile = new MP4File(argv[1], "r", MP4_DETAILS_ALL);
	
	if (pFile->Read() == 0) {
		pFile->Dump();
	}

	delete pFile;
}

void mp4extract(int argc, char** argv)
{
	MP4File* pFile = new MP4File(argv[1], "r", MP4_DETAILS_FIND);

	if (pFile->Read() != 0) {
		return;
	}

	printf("movie duration is %u secs\n", 
		MP4TOSECS(pFile->GetDuration()));

	printf("video profileLevel id is %u\n",
		pFile->GetVideoProfileLevel());

	u_int8_t* pConfig;
	u_int32_t configSize;
	pFile->GetESConfiguration(2, &pConfig, &configSize);

	printf("first video config is %u bytes long\n", configSize);

#ifdef NOTDEF
	// foreach video track
	u_int32_t numVideoTracks = pFile->GetNumberOfTracks("video");

	for (u_int32_t i = 0; i < numVideoTracks; i++) {

		MP4TrackId videoTrack = pFile->FindTrack(i, "video");

		int rawFd = open("extract1.yuv", O_CREAT | O_TRUNC);

		u_int8_t* pFrame;
		u_int32_t frameSize;

		// get video decoder info
		pFile->GetESConfigInfo(videoTrack, &pFrame, &frameSize);
		write(rawFd, pFrame, frameSize);
		free(pFrame);

		// while there are video frames, read them, and write them raw
		while (pFile->GetNextVideoFrame(&pFrame, &frameSize) == 0)) {
			write(rawFd, pFrame, frameSize);
			free(pFrame);
		}
	}

	// foreach audio track
#endif

	delete pFile;
}

main(int argc, char** argv)
{
	if (argc < 2) {
		fprintf(stderr, "Usage: cpptest <file>\n");
		exit(1);
	}

	try {
		mp4dump(argc, argv);

		mp4extract(argc, argv);
	}
	catch (MP4Error* e) {
		e->Print();
		exit(1);
	}

	exit(0);
}


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

	MP4FileHandle mp4File = MP4Create(argv[1], verbosity);

	if (!mp4File) {
		exit(1);
	}

	printf("Created skeleton\n");
	MP4Dump(mp4File);

	MP4SetODProfileLevel(mp4File, 1);
	MP4SetSceneProfileLevel(mp4File, 1);
	MP4SetVideoProfileLevel(mp4File, 1);
	MP4SetAudioProfileLevel(mp4File, 1);
	MP4SetGraphicsProfileLevel(mp4File, 1);

	MP4TrackId odTrackId = 
		MP4AddODTrack(mp4File);

	MP4TrackId bifsTrackId = 
		MP4AddSceneTrack(mp4File);

	MP4TrackId videoTrackId = 
		MP4AddVideoTrack(mp4File, 90000, 3000, 320, 240);

	MP4TrackId audioTrackId = 
		MP4AddAudioTrack(mp4File, 44100, 1152);

	MP4MakeIsmaCompliant(mp4File);

	printf("Added tracks\n");
	MP4Dump(mp4File);

	MP4Close(mp4File);

	exit(0);
}


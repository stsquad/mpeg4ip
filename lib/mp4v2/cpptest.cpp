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

void mp4create(int argc, char** argv)
{
	MP4File* pFile = new MP4File(argv[1], "w", MP4_DETAILS_ALL);

	pFile->SetODProfileLevel(1);
	pFile->SetSceneProfileLevel(1);
	pFile->SetVideoProfileLevel(1);
	pFile->SetAudioProfileLevel(1);
	pFile->SetGraphicsProfileLevel(1);

	MP4TrackId odTrackId = 
		pFile->AddTrack("od");

#ifdef NOTDEF
	MP4TrackId bifsTrackId = 
		pFile->AddTrack("bifs");

	MP4TrackId videoTrackId = 
		pFile->AddVideoTrack(90000, 3000, 320, 240);

	MP4TrackId audioTrackId = 
		pFile->AddAudioTrack(44100, 1152);
#endif

	// pFile->SetTrackESConfiguration();
	// pFile->WriteSample();

	pFile->Write();

	delete pFile;
}

main(int argc, char** argv)
{
	if (argc < 2) {
		fprintf(stderr, "Usage: cpptest <file>\n");
		exit(1);
	}

	try {
		mp4create(argc, argv);
	}
	catch (MP4Error* e) {
		e->Print();
		exit(1);
	}

	exit(0);
}


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

	u_int32_t verbosity = MP4_DETAILS_ERROR;
	char* fileName = argv[1];

	// open the mp4 file
	MP4FileHandle mp4File = MP4Read(fileName, verbosity);

	if (MP4GetNumberOfTracks(mp4File, "hint") == 0) {
		exit(1);
	}

#ifdef NOTDEF
	// write out sdp somewhere
	MP4GetSdp(mp4File);

	// create UDP socket
	int udpSocket = socket();
	connect(udpSocket);


	// now consecutively read and send the RTP packets
	// TBD from all hint tracks

	u_int8_t* pPacket;
	u_int32_t packetSize;

	for (u_int32_t packetNumber = 1; packetNumber <= numPackets; 
	  packetNumber++) {

		// read next packet
		MP4ReadRtpPacket(mp4File, trackId, packetNumber, 
			&pPacket, &packetSize, &xmitTime);

		// TBD timing of send

		send(udpSocket, pPacket, packetSize);

		// free packet buffer
		free(pPacket);
	}

	// close the UDP socket
	close(udpSocket);
#endif

	// close mp4 file
	MP4Close(mp4File);

	exit(0);
}


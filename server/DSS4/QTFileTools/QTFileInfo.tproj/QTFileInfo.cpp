/*
 *
 * @APPLE_LICENSE_HEADER_START@
 *
 * Copyright (c) 1999-2001 Apple Computer, Inc.  All Rights Reserved. The
 * contents of this file constitute Original Code as defined in and are
 * subject to the Apple Public Source License Version 1.2 (the 'License').
 * You may not use this file except in compliance with the License.  Please
 * obtain a copy of the License at http://www.apple.com/publicsource and
 * read it before using this file.
 *
 * This Original Code and all software distributed under the License are
 * distributed on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, AND APPLE HEREBY DISCLAIMS ALL SUCH WARRANTIES,
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR NON-INFRINGEMENT.  Please
 * see the License for the specific language governing rights and
 * limitations under the License.
 *
 *
 * @APPLE_LICENSE_HEADER_END@
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "getopt.h"

#include <unistd.h>

#include "QTFile.h"

#include "QTTrack.h"
#include "QTHintTrack.h"

#if __MACOS__
	#include <SIOUX.h>
#endif


/* getopt() replacements */
#ifdef __MACOS__
	int optind = 0;
	
	int optcharcount = 1;
	int optchar[] = { 'd' };
	
	int getopt(int argc, char *argv[], char * optstring);
	int getopt(int /*argc*/, char ** /*argv*/, char * /*optstring*/)
	{
		if( optind >= optcharcount )
			return -1;
		
		return optchar[optind++];
	}
#endif


int main(int argc, char *argv[]) {
	// Temporary vars
	int			ch;

	// General vars
	QTTrack			*Track;
	bool			Debug = false, DeepDebug = false;
    extern int optind;


/* argc/argv preloading */
#if __MACOS__
	char			*argv1 = "-d", *argv2 = "mystery.mov";
	
	argc = 2;
	argv[0] = argv1; /* this is a placeholder; see 'optchar' above */
	argv[1] = argv2;
#endif

/* SIOUX setup */
#if __MACOS__
	SIOUXSettings.asktosaveonclose = 0;
#endif

	//
	// Read our command line options
	while( (ch = getopt(argc, argv, "dD")) != -1 ) {
		switch( ch ) {
			case 'd':
				Debug = true;
			break;

			case 'D':
				Debug = true;
				DeepDebug = true;
			break;
		}
	}

	argc -= optind;
	argv += optind;

	//
	// Validate our arguments.
	if( argc != 1 ) {
		printf("usage: QTFileInfo [-d] [-D] <filename>\n");
		exit(1);
	}


	//
	// Open the movie.
	QTFile file(Debug, DeepDebug);
	file.Open(*argv);
	if(Debug) file.DumpAtomTOC();

	//
	// Print out some information.
	printf("-- Movie %s \n", *argv);
	printf("   Duration        : %f\n", file.GetDurationInSeconds());
	
	Track = NULL;
	while( file.NextTrack(&Track, Track) ) {
		// Temporary vars
		QTHintTrack	*HintTrack;
		time_t		unixCreationTime = (time_t)Track->GetCreationTime() + (time_t)QT_TIME_TO_LOCAL_TIME;
		time_t		unixModificationTime = (time_t)Track->GetModificationTime() + (time_t)QT_TIME_TO_LOCAL_TIME;


		//
		// Initialize the track and dump it.
		if( Track->Initialize() != QTTrack::errNoError ) {
			printf("!!! Failed to initialize track !!!\n");
			continue;
		}
		if(DeepDebug) Track->DumpTrack();
		
		//
		// Dump some info.
		printf("-- Track #%02ld ---------------------------\n", Track->GetTrackID());
		printf("   Name               : %s\n", Track->GetTrackName());
		printf("   Created on         : %s", asctime(gmtime(&unixCreationTime)));
		printf("   Modified on        : %s", asctime(gmtime(&unixModificationTime)));

		//
		// Dump hint information is possible.
		if( file.IsHintTrack(Track) ) {
			HintTrack = (QTHintTrack *)Track;

			printf("   Total RTP bytes    : %"_64BITARG_"u\n", HintTrack->GetTotalRTPBytes());
			printf("   Total RTP packets  : %"_64BITARG_"u\n", HintTrack->GetTotalRTPPackets());
			printf("   Average bitrate    : %.2f Kbps\n", ((HintTrack->GetTotalRTPBytes() << 3) / file.GetDurationInSeconds()) / 1024);
			printf("   Average packet size: %"_64BITARG_"u\n", HintTrack->GetTotalRTPBytes() / HintTrack->GetTotalRTPPackets());

			UInt32 UDPIPHeaderSize = (56 * HintTrack->GetTotalRTPPackets());
			UInt32 RTPUDPIPHeaderSize = ((56+12) * HintTrack->GetTotalRTPPackets());
			printf("   Percentage of stream wasted on UDP/IP headers    : %.2f\n", (float)UDPIPHeaderSize / (float)(HintTrack->GetTotalRTPBytes() + UDPIPHeaderSize) * 100);
			printf("   Percentage of stream wasted on RTP/UDP/IP headers: %.2f\n", (float)RTPUDPIPHeaderSize / (float)(HintTrack->GetTotalRTPBytes() + RTPUDPIPHeaderSize) * 100);
		}
	}
	
	return 0;
}

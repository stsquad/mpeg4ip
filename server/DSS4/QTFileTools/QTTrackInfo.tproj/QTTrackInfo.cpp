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

/*
	

*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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
	char *optarg = NULL;
	
	int optcharcount = 1;
	int optchar[] = { 'T' };
	char * optchar_arg[] = { "stco" };
	
	int getopt(int argc, char *argv[], char * optstring);
	int getopt(int /*argc*/, char ** /*argv*/, char * /*optstring*/)
	{
		if( optind >= optcharcount )
			return -1;
		
		if( optchar_arg[optind] != NULL ) {
			char ch = optchar[optind];
			optarg = optchar_arg[optind];
			optind += 2;
			return ch;
		} else {
			return optchar[optind++];
		}
	}
	
	char * strdup(const char * str);
	char * strdup(const char * str)
	{
		if( str == NULL )
			return NULL;
		char * newstr = (char *)malloc(strlen(str) + 1);
		if( newstr != NULL )
			memcpy(newstr, str, strlen(str) + 1);
		return newstr;
	}
#endif


//extern char		*optarg;
//extern int		optind;

int main(int argc, char *argv[]) {
	// Temporary vars
	int			ch;

	// General vars
	const char		*MovieFilename;
	int				TrackNumber;

	QTTrack			*Track;
	QTHintTrack		*HintTrack;
	bool			Debug = false, DeepDebug = false;
	char			*Table = NULL;
    extern char* optarg;
    extern int optind;


/* argc/argv preloading */
#if __MACOS__
	char			*argv1 = "-T", *argv2 = "stco", *argv3 = "Hackers.mov", *argv4 = "6";
	
	argc = 4;
	argv[0] = argv1; /* this is a placeholder; see 'optchar' above */
	argv[1] = argv2;
	argv[2] = argv3;
	argv[3] = argv4;
#endif

/* SIOUX setup */
#if __MACOS__
	SIOUXSettings.asktosaveonclose = 0;
#endif

	//
	// Read our command line options
	while( (ch = getopt(argc, argv, "dDT:")) != -1 ) {
		switch( ch ) {
			case 'd':
				Debug = true;
			break;

			case 'D':
				Debug = true;
				DeepDebug = true;
			break;
			
			case 'T':
				Table = strdup(optarg);
			break;
		}
	}

	argc -= optind;
	argv += optind;

	//
	// Validate our arguments.
	if( argc != 2 ) {
		printf("usage: QTTrackInfo [-d] [-D] [-T <table name>]<filename> <track number>\n");
		exit(1);
	}
	
	MovieFilename = *argv++;
	TrackNumber = atoi(*argv++);


	//
	// Open the movie.
	QTFile file(Debug, DeepDebug);
	file.Open(MovieFilename);

	//
	// Find the specified track and dump out information about its' samples.
	if( !file.FindTrack(TrackNumber, &Track) ) {
		printf("Error!  Could not find track number %d in file \"%s\"!",
			   TrackNumber, MovieFilename);
		exit(1);
	}

	//
	// Initialize the track.
	if( Track->Initialize() != QTTrack::errNoError ) {
		printf("Error!  Failed to initialize track %d in file \"%s\"!\n",
			   TrackNumber, MovieFilename);
		exit(1);
	}
	
	//
	// Dump some information about this track.
	{
		time_t		unixCreationTime = (time_t)Track->GetCreationTime() + (time_t)QT_TIME_TO_LOCAL_TIME;
		time_t		unixModificationTime = (time_t)Track->GetModificationTime() + (time_t)QT_TIME_TO_LOCAL_TIME;

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

			UInt32 UDPIPHeaderSize = (UInt32) (56 * HintTrack->GetTotalRTPPackets());
			UInt32 RTPUDPIPHeaderSize = (UInt32) ((56+12) * HintTrack->GetTotalRTPPackets());
			printf("   Percentage of stream wasted on UDP/IP headers    : %.2f\n", (float)UDPIPHeaderSize / (float)(HintTrack->GetTotalRTPBytes() + UDPIPHeaderSize) * 100);
			printf("   Percentage of stream wasted on RTP/UDP/IP headers: %.2f\n", (float)RTPUDPIPHeaderSize / (float)(HintTrack->GetTotalRTPBytes() + RTPUDPIPHeaderSize) * 100);
		}

		printf("\n");
		printf("\n");
	}

	//
	// Dump all of the entries in the specified table (if we were given one).
	// Go through all of the samples in this track, printing out their offsets
	// and sizes.
	if( Table != NULL ) {
		if( (strcmp(Table, "stco") == 0) || (strcmp(Table, "co64") == 0) ) {
			Track->DumpChunkOffsetTable();
		} else if( strcmp(Table, "stsc") == 0 ) {
			Track->DumpSampleToChunkTable();
		} else if( strcmp(Table, "stsz") == 0 ) {
			Track->DumpSampleSizeTable();
		} else if( strcmp(Table, "stts") == 0 ) {
		}
	}

	return 0;
}

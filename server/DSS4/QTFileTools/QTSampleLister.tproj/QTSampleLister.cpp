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
#include <string.h>
#include <time.h>
#include "getopt.h"

#include <unistd.h>

#include "QTFile.h"
#include "QTTrack.h"

#if __MACOS__
	#include <SIOUX.h>
#endif


/* getopt() replacements */
#ifdef __MACOS__
	int optind = 0;
	
	int optcharcount = 0;
	int optchar[] = { 'd' };
	
	int getopt(int argc, char *argv[], char * optstring);
	int getopt(int /*argc*/, char ** /*argv*/, char * /*optstring*/)
	{
		if( optind >= optcharcount )
			return -1;
		
		return optchar[optind++];
	}
#endif


//extern char		*optarg;
//extern int		optind;
	
int main(int argc, char *argv[]) {
	// Temporary vars
	int				ch;

	// General vars
	const char		*MovieFilename;
	int				TrackNumber;

	QTTrack			*Track;
	bool			Debug = false, DeepDebug = false,
					DumpHTML = false;
	
	UInt64			Offset;
	UInt32			Size, MediaTime;
    extern char* optarg;
    extern int optind;


/* argc/argv preloading */
#if __MACOS__
	char			*argv1 = "Hackers.mov", *argv2 = "6";
	
	argc = 2;
	argv[0] = argv1;
	argv[1] = argv2;
#endif

/* SIOUX setup */
#if __MACOS__
	SIOUXSettings.asktosaveonclose = 0;
#endif

	//
	// Read our command line options
	while( (ch = getopt(argc, argv, "dDH")) != -1 ) {
		switch( ch ) {
			case 'd':
				Debug = true;
			break;

			case 'D':
				Debug = true;
				DeepDebug = true;
			break;
			
			case 'H':
				DumpHTML = true;
			break;
		}
	}

	argc -= optind;
	argv += optind;

	//
	// Validate our arguments.
	if( argc != 2 ) {
		printf("usage: QTSampleLister [-d] [-D] [-H] <filename> <track number>\n");
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
	if( Track->Initialize() != QTTrack::errNoError){
		printf("Error!  Failed to initialize track %d in file \"%s\"!\n",
			   TrackNumber, MovieFilename);
		exit(1);
	}
	
	//
	// Dump some information about this track.
	if( !DumpHTML ) {
		time_t		unixCreationTime = (time_t)Track->GetCreationTime() + (time_t)QT_TIME_TO_LOCAL_TIME;
		time_t		unixModificationTime = (time_t)Track->GetModificationTime() + (time_t)QT_TIME_TO_LOCAL_TIME;

		printf("-- Track #%02ld ---------------------------\n", Track->GetTrackID());
		printf("   Name               : %s\n", Track->GetTrackName());
		printf("   Created on         : %s", asctime(gmtime(&unixCreationTime)));
		printf("   Modified on        : %s", asctime(gmtime(&unixModificationTime)));
		printf("\n");
		printf("\n");
	}

	//
	// Go through all of the samples in this track, printing out their offsets
	// and sizes.
	if( !DumpHTML ) {
		printf("Sample #     Media Time  DataOffset  SampleSize\n");
		printf("--------     ----------  ----------  ----------\n");
	}
	
	UInt32 curSample = 1;

        QTAtom_stsc_SampleTableControlBlock stsc;
        QTAtom_stts_SampleTableControlBlock stts;
        while( Track->GetSampleInfo(curSample, &Size, &Offset, NULL, &stsc) ) {
		//
		// Get some additional information about this sample.
            Track->GetSampleMediaTime(curSample, &MediaTime, &stts);

		//
		// Dump out the sample.
		if( DumpHTML ) 
			printf("<FONT COLOR=black>%010"_64BITARG_"u: track=%02lu; size=%lu</FONT><BR>\n",Offset, Track->GetTrackID(), Size);
		else 
			printf("%8lu  -  %10lu  %10"_64BITARG_"u  %10lu\n", curSample, MediaTime, Offset, Size);

	
		// Next sample.
		curSample++;
	}

	return 0;
}

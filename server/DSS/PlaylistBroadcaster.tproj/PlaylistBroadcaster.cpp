/*
 * Copyright (c) 1999 Apple Computer, Inc. All rights reserved.
 *
 * @APPLE_LICENSE_HEADER_START@
 * 
 * Copyright (c) 1999 Apple Computer, Inc.  All Rights Reserved.
 * The contents of this file constitute Original Code as defined in and are 
 * subject to the Apple Public Source License Version 1.1 (the "License").  
 * You may not use this file except in compliance with the License.  Please 
 * obtain a copy of the License at http://www.apple.com/publicsource and 
 * read it before using this file.
 * 
 * This Original Code and all software distributed under the License are 
 * distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY KIND, EITHER 
 * EXPRESS OR IMPLIED, AND APPLE HEREBY DISCLAIMS ALL SUCH WARRANTIES, 
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY, FITNESS 
 * FOR A PARTICULAR PURPOSE OR NON-INFRINGEMENT.  Please see the License for 
 * the specific language governing rights and limitations under the 
 * License.
 * 
 * 
 * @APPLE_LICENSE_HEADER_END@
 */

/*
	8.11.99 rt 	- changed references to "PlaylistBroadcaster Setup" to Broadcast Description File

	8.4.99 rt	- changed references to "PlaylistBroadcaster Description" to Broadcast Setup File
				- addded error messages
				- prefilght config file access
				- require log file creation
				
				
	7.27.99 rt 	- removed license from about display
				- updated credit names
				- fixed mapping of --stop to 's' from 'l'
				- added about to help
				

	8.2.99 rt 	- changed reference to "channel setup" to "PlaylistBroadcaster Description"
				- changed &d's in printf's to %d
				
*/



#include <stdio.h>
#include <stdlib.h>
#include <signal.h>


#ifndef __Win32__
#ifndef __MACOS__
	#ifdef __solaris__
		#include "daemon.h"
	#else
		#ifndef __FreeBSD__
			#include <sys/sysctl.h>
		#endif
	#endif
#else
	#include <SIOUX.h>
#endif
#endif


#include <errno.h>
#include "QTRTPFile.h"

#include "OSHeaders.h"
#include "OS.h"
#include "SocketUtils.h"
#include "Socket.h"
#include "OSArrayObjectDeleter.h"
#include "PickerFromFile.h"
#include "PLBroadcastDef.h"
#include "BroadcastLog.h"
#ifndef __Win32__
#include "BCasterTracker.h"
#endif
// must now inlcude this from the project level using the -i switch in the compiler
#ifndef __MacOSXServer__
#ifndef __MacOSX__
	#include "revision.h"
#endif
#endif

#include "playlist_SDPGen.h"
#include "playlist_broadcaster.h"

#include "getopt.h" // uses glibc getopt_long for long form option support.
#include "MyAssert.h"

/*
	local functions
*/

static void usage();
static void version();
static void help();
static PlaylistPicker*	MakePickerFromConfig( PLBroadcastDef* broadcastParms );
static void SignalEventHandler( int signalID );
static int EvalBroadcasterErr(int err);
static const char* GetMovieFileErrString( int err );

static void RegisterEventHandlers();
static void ShowPlaylistStatus();
static void StopABroadcast( const char* arg );
static bool DoSDPGen( PLBroadcastDef *broadcastParms, bool preflight );

static void AddOurPIDToTracker( const char* bcastSetupFilePath );



/* -- preflighting --*/

static int MyAccess( const char* path, int mode );
static int PreflightSDPFileAccess( const char * sdpPath );
static int PreflightDestinationAddress( const char *destAddress );
static int PreflightBasePort( const char *basePort );
static int PreflightReferenceMovieAccess( const char *refMoviePath );
static bool PreflightTrackerFileAccess( int mode );
static void PreFlightSetupFile( const char * bcastSetupFilePath );
static void ShowSetupParams(PLBroadcastDef*	broadcastParms, const char *file);


static void PreFlightOrBroadcast( const char *bcastSetupFilePath, bool preflight, bool noDaemon, bool showMovieList,bool currentMovie );

/*
	local variables
*/
static char *			sStartMessage = "PLAY"; 
static char* 			sgProgramName; // the actual program name as input at commmand line
static char* 			sgTrackerDirPath = "/var/run";
static char* 			sgTrackerFilePath = "/var/run/broadcastlist";
static BroadcastLog*	sgLogger = NULL;
static bool				sgTrackingSucceeded = false;

#ifdef __MACOS__
	static char *kMacOptions = "pdv";
	static char *kMacFilePath = "Work:Movies:test";
#endif

/* long form options for "getopt_long" */

struct option longopts[] =
{
      {"version",    0, 0, 'v'} 	/* display version number */
    , {"list",    0, 0, 'l'} 		/* display running broadcasts */
    , {"stop",    1, 0, 's'} 		/* stop a broadcast */
    , {"debug",    0, 0, 'd'} 		/* debug a broadcast */
    , {"help",    0, 0, 'h'} 		/* print help info */
    , {"preflight",  0, 0, 'p'} 	/* preflight */
    , {"movies",  0, 0, 'm'} 		/* show movie list */
    , {"current",  0, 0, 'c'} 		/* show currently playing movie */
    , {0, 0, 0, 0 } 				// terminator for options.	

};


//+ main is getting too big.. need to clean up and separate the command actions 
// into a separate file.


int main(int argc, char *argv[]) {


	char	*bcastSetupFilePath = NULL;
	int 	noDaemon = false;
	char 	anOption;
	bool	preflight = false;
	bool	showMovieList = false;
	bool	current = false;
	int		displayedOptions = 0; // count number of command line options we displayed
	bool	needsTracker = false;	// set to true when PLB needs tracker access
	bool	needsLogfile = false;	// set to true when PLB needs tracker access

#ifdef __Win32__		
	//
	// Start Win32 DLLs
	WORD wsVersion = MAKEWORD(1, 1);
	WSADATA wsData;
	(void)::WSAStartup(wsVersion, &wsData);
#endif

	QTRTPFile::Initialize(); // do only once
	OS::Initialize();
	Socket::Initialize();
	SocketUtils::Initialize();
	if (SocketUtils::GetNumIPAddrs() == 0)
	{
		printf("IP must be enabled to run PlaylistBroadcaster\n");
		::exit(0);
	}


	RegisterEventHandlers();

	
#ifndef __MACOS__
	sgProgramName = argv[0];
	while ((anOption = _getopt_internal(argc, argv, "s:ahvdlpm", longopts, (int*)0, true )) != EOF)

//	while ((anOption = getopt_long_only(argc, argv, "s:ahvdlpm", longopts, (int*)0 )) != EOF)

#else
	sgProgramName = "PlaylistMac";
	char *theOption = kMacOptions;
 	int count = 0;
	while (anOption = theOption[count++])
#endif
	{
		
		switch(anOption)
		{
			case 'c':
				current = true;
				break;
				
			case 'v':
				::version();
				::usage();
				displayedOptions++;
				break;
				
			case 'm':
				
				showMovieList = true;
				noDaemon = true;
				break;
				
			case 'p':
				preflight = true;
				noDaemon = true;
				needsTracker = true;
				needsLogfile = true;
				break;
				

#ifndef __Win32__
			case 'd':
				noDaemon = true;
				break;

			case 'l':
				// show playlist broadcast status			
				if (!PreflightTrackerFileAccess( R_OK ))	// check for read access. 
					::exit(-1);
				ShowPlaylistStatus();				// <- exits() on failure
				displayedOptions++;
				break;
				
			case 's':				
				// stop a playlist broadcast			
				// is there one to kill?
				if (!PreflightTrackerFileAccess( R_OK | W_OK )) 	// check for read access. 
					::exit(-1);
				StopABroadcast( optarg );					// <- exits() on failure
				displayedOptions++;				
				break;
#endif
							
			case 'h':
				::help();
				displayedOptions++;
				break;
			
			default:
			    /* Error message already emitted by getopt_long. */
			    ::usage();
			    ::exit(-1);
		}
	}

#ifndef __MACOS__
	if (optind < argc)
    {
    	int 	numFiles = 0;
    			
		while (optind < argc )
		{	numFiles++;
			bcastSetupFilePath = argv[optind++];
		}
		
		if ( numFiles > 1 )
		{
			::printf("- PlaylistBroadcaster:  Error. Specify only one broadcast description file.\n" );
			::exit(-1);
		}
		
	}
#else
	bcastSetupFilePath = kMacFilePath;
#endif
	
	// preflight needs a description file	
	if ( preflight && !bcastSetupFilePath )
	{	::printf("- PlaylistBroadcaster: Error.  \"Preflight\" requires a broadcast description file.\n" );
		::usage();
		::exit(-1);
	}


	// don't complain about lack of description File if we were asked to display some options.	
	if ( displayedOptions > 0 && !bcastSetupFilePath )
		::exit(0);		// <-- we displayed option info, but have no description file to broadcast
		
		
	PreFlightOrBroadcast( bcastSetupFilePath, preflight, noDaemon, showMovieList, current ); // <- exits() on failure
	


	return 0;

}


static void PreFlightOrBroadcast( const char *bcastSetupFilePath, bool preflight, bool noDaemon, bool showMovieList, bool currentMovie )
{
	PLBroadcastDef*		broadcastParms = NULL;
	PlaylistPicker* 	picker = NULL;
	QTFileBroadcaster 	fileBroadcaster;
	int 				broadcastErr;
	long				moviePlayCount;
	char*				thePick;
	long				numMovieErrors;
	char				*bcastSetupDirPath;
	bool				sdpFileCreated = false;
	
#ifndef __MACOS__
	PreFlightSetupFile( bcastSetupFilePath );	// <- exits() on failure
	

	broadcastParms = new PLBroadcastDef( bcastSetupFilePath );
	
	
	if( !broadcastParms )
	{	::printf("- PlaylistBroadcaster: Error. Out of memory.\n" );
		goto bail;
	}

	
	if ( !broadcastParms->ParamsAreValid() )
	{	::printf("- PlaylistBroadcaster: Error reading the broadcast description file \"%s\". (bad format or missing file)\n", bcastSetupFilePath );
		goto bail;
	}
	
	
	if ( preflight )
		ShowSetupParams(broadcastParms, bcastSetupFilePath);
		

	bcastSetupDirPath = new char [strlen(bcastSetupFilePath) + 1 ];
	
	if ( bcastSetupDirPath )
	{
		strcpy( bcastSetupDirPath, bcastSetupFilePath );

		char* endOfDirPath = strrchr( bcastSetupDirPath, kPathDelimiterChar );
		
		if ( endOfDirPath )
		{
			endOfDirPath++;
		
			*endOfDirPath= 0;
			
			
			int 	chDirErr = ::chdir( bcastSetupDirPath );
			
			if ( chDirErr )
				chDirErr = errno;
				
			//::printf("- PLB DEBUG MSG: bcastSetupDirPath==%s\n  chdir err: %i\n", bcastSetupDirPath, chDirErr);
			
		}
		
		
		
	}
	else
	{	::printf("- PlaylistBroadcaster: Error. Out of memory.\n" );
		goto bail;
	}


	if (preflight)
		picker = new PlaylistPicker(false);				// make sequential picker, no looping
	else
		picker = MakePickerFromConfig( broadcastParms ); // make  picker according to parms
	
	
	if ( !picker )
	{	::printf("- PlaylistBroadcaster: Error. Out of memory.\n" );
		goto bail;
	}
	
	

	::printf("\n");
	
	Assert( broadcastParms->mPlayListFile );
	if ( broadcastParms->mPlayListFile )
	{	
		char*	fileName;
		
		fileName = broadcastParms->mPlayListFile;
		// initial call uses empty string for path, NULL for loop detection list
		(void)PopulatePickerFromFile( picker, fileName, "", NULL );
		
		// ignore errors, if we have movies in the list, play them
	}
	
	if ( preflight )
	{
		if ( picker->mNumToPickFrom == 1 )
			::printf( "\nThere is (%li) movie in the Playlist.\n\n", (long) picker->mNumToPickFrom );
		else
			::printf( "\nThere are (%li) movies in the Playlist.\n\n", (long) picker->mNumToPickFrom );
	}	
	
	if ( !picker->mNumToPickFrom )
	{	printf( "- PlaylistBroadcaster setup failed: There are no movies to play.\n" );
		goto bail;
	}
	

	// check that we have enough movies to cover the recent movies list.
	if ( preflight && broadcastParms->ParamsAreValid() )
	{
		if (  !strcmp( broadcastParms->mPlayMode, "weighted_random" ) ) // this implies "random" play
		{	if ( atoi( broadcastParms->mLimitPlayQueueLength ) >=  picker->mNumToPickFrom )
				printf( "- PlaylistBroadcaster Warning:\n  The recent_movies_list_size is greater than or equal\n  to the number of movies in the playlist.\n  Each movie will play once, then the broadcast will end.\n" );
		}
	}
	
	// create the log file
	sgLogger = new BroadcastLog( broadcastParms );
	
	
	Assert( sgLogger );
	
	if( !sgLogger )
	{	::printf("- PlaylistBroadcaster: Error. Out of memory.\n" );
		goto bail;
	}
	
	sdpFileCreated = DoSDPGen( broadcastParms, preflight );
	
	broadcastErr = fileBroadcaster.SetUp(broadcastParms);
	
	if (  broadcastErr )
	{	
		::EvalBroadcasterErr(broadcastErr);
		::printf( "- Broadcaster setup failed.\n" );
		goto bail;
	}
	
	if (preflight)
	{	 fileBroadcaster.fPlay = false;
	
	}
	
	if ( !PreflightTrackerFileAccess( R_OK | W_OK ) )
		goto bail;

	//Unless the Debug command line option is set, daemonize the process at this point
	if (!noDaemon)
	{	
#ifndef __Win32__
		::printf("- PlaylistBroadcaster: Started in background.\n");
		
		// keep the same working directory..
		if (::daemon( 1, 0 ) != 0)
		{
			::printf("- PlaylistBroadcaster:  System error (%i).\n", errno);
			goto bail;
		}
#endif		
	}

	// ^ daemon must be called before we Open the log and tracker since we'll
	// get a new pid, our files close,  ( does SIGTERM get sent? )
	
	if (( sgLogger ) && ( sgLogger->WantsLogging() ))
		sgLogger->EnableLog( false ); // don't append ".log" to name for PLB
	
	if ( sgLogger->WantsLogging() && !sgLogger->IsLogEnabled() )
	{
		if (  sgLogger->LogDirName() && *sgLogger->LogDirName() )
			::printf("- PlaylistBroadcaster: The log file failed to open.\n  ( path: %s/%s )\n  Exiting.\n", sgLogger->LogDirName(), sgLogger->LogFileName() );
		else
			::printf("- PlaylistBroadcaster: The log file failed to open.\n  ( path: %s )\n  Exiting.\n", sgLogger->LogFileName() );
		
		goto bail;
	}
	
	
	
	AddOurPIDToTracker( bcastSetupFilePath ); // <-- exits on failure
	
	

	if ( !preflight || showMovieList )
		sgLogger->LogInfo( "PlaylistBroadcaster started" );
	else
		sgLogger->LogInfo( "PlaylistBroadcaster preflight started" );

	//+ make the RTP movie broadcaster
	
	if ( !preflight )
	{
		printf( "\n" );
		printf( "[pick#] movie path\n" );
		printf( "----------------------------\n" );
	}
	else
	{
		printf( "\n" );
		printf( "Problems found\n" );
		printf( "--------------\n" );
	}
	
	moviePlayCount = 0;
	numMovieErrors = 0;
	
	for ( thePick = picker->PickOne(); thePick; thePick = picker->PickOne()  )
	{	++moviePlayCount;
		
		if ( !preflight || showMovieList )
		{
			// display the picks in debug mode, but not preflight
			printf( "[%li] ", moviePlayCount );
			printf( "%s picked\n", thePick );
		}
		
		
		if ( !showMovieList )
		{
			int playError;
			
			if (currentMovie && !preflight)
			{	
				sgLogger->LogMoviePlay( thePick, NULL, sStartMessage );
			}
			playError = fileBroadcaster.PlayMovie( thePick );

			if (playError)
			{	playError = ::EvalBroadcasterErr(playError);
				printf("  (file: %s err: %d %s)\n", thePick, playError,GetMovieFileErrString( playError ) );
				numMovieErrors++;
			}
			else
			{
				int tracks = fileBroadcaster.GetMovieTrackCount() ;
				int mtracks = fileBroadcaster.GetMappedMovieTrackCount();

				if (tracks != mtracks)
				{	numMovieErrors++;
					printf("- PlaylistBroadcaster: Warning, movie tracks do not match the SDP file.\n" );
					printf("  Movie: %s .\n", thePick );
					printf("  %i of %i hinted tracks will not broadcast.\n", tracks- mtracks, tracks );
				}
			}
			
			if ( !preflight )
				sgLogger->LogMoviePlay( thePick, GetMovieFileErrString( playError ),NULL );
		}
		
		delete [] thePick;
	
	} 
#else
		PLBroadcastDef params(bcastSetupFilePath);
		numMovieErrors = 0;
		params.ShowSettings();
		sdpFileCreated = DoSDPGen( &params, true );
		//fileBroadcaster.fPlay = false;
		thePick = params.mSDPReferenceMovie;
		int playError = fileBroadcaster.SetUp(&params);
		if (!playError)
			playError = fileBroadcaster.PlayMovie( thePick );
		if (playError) numMovieErrors =1;
		printf("%s \n",GetMovieFileErrString( playError ));
#endif

	if ( preflight )
	{
		char	str[256];
		
		printf( " - "  );
		sprintf( str, "PlaylistBroadcaster found (%li) movie problem(s)." , numMovieErrors );
		printf( "%s\n", str );
		if (sgLogger) sgLogger->LogInfo( str );
	}
	
	
	if ( !preflight )
		if (sgLogger) sgLogger->LogInfo( "PlaylistBroadcaster finished" );
	else
	{	if (sgLogger) sgLogger->LogInfo( "PlaylistBroadcaster preflight finished" );
		printf( "PlaylistBroadcaster preflight finished.\n" );
	}
	/*
	if (!preflight)
		ShowPickDistribution( picker );
	*/
		
bail:

	//if preflighting and we created tghe file, delete the file now
	if (preflight && sdpFileCreated)
		(void)unlink(broadcastParms->mSDPFile);

	delete picker;
	
	delete broadcastParms;

	delete sgLogger;
	sgLogger = NULL;

#ifndef __Win32__
	if ( sgTrackingSucceeded )
	{
		// remove ourselves from the tracker
		// this is the "normal" remove, also signal handlers
		// may remove us.
		
		BCasterTracker	tracker( sgTrackerFilePath );
		
		tracker.RemoveByProcessID( getpid() );
		tracker.Save();
	}
#endif
	::exit(-1);
}


/* ========================================================================
 * Signal and error handler.
 */
static void SignalEventHandler( int signalID )
{
  
 	//::printf( "signal %d caught(SignalEventHandler)\n", signalID );
 	
 	if ( sgLogger )
	 	sgLogger->LogInfo( "PlaylistBroadcaster finished" );
	 
	 delete sgLogger;
	 sgLogger = NULL;

#ifndef __Win32__
	if ( sgTrackingSucceeded )
	{	// give tracker scope so that it really does it's
		// thing before "exit" is called.
		BCasterTracker	tracker( sgTrackerFilePath );
	
		tracker.RemoveByProcessID( getpid() );
		tracker.Save();
	}
#endif	
 	::exit(-1);
}



static void version()
{
	/*
		print PlaylistBroadcaster version and build info
		
		see revision.h
	*/
	
	//RoadRunner/2.0.0-v24 Built on: Sep 17 1999 , 16:09:53
	//revision.h (project level file) -- include with -i option
	::printf("PlaylistBroadcaster/%s Built on: %s, %s\n",  kVersionString, __DATE__, __TIME__ );

}

static void usage()
{
	/*
		print PlaylistBroadcaster usage string

	*/
#ifndef __Win32__
	::printf("usage: %s [ -list | -stop # | -version | -help | -current] | [ -preflight | -debug  file ] \n", sgProgramName );
#else
	::printf("usage: %s [ -version | -help | -current] | [ -preflight ] \n", sgProgramName );
#endif

}

static void help()
{
	/*
		print PlaylistBroadcaster help info

	*/
	::version();
	
	::usage();
	
	::printf("-version: Display version information.\n");
	::printf("-preflight: Verify a broadcast description file and movie list.\n");
	::printf("-help: Display the help screen.\n");
#ifndef __Win32__
	::printf("-debug: Run attached to the terminal. (debug)\n");
	::printf("-list: List running broadcasts.\n");
	::printf("-stop n: Stop a running broadcast.\n");
#endif
	::printf("-current: Show the current movie in the log file. \n");
	::printf("file: Specify a broadcast description file.\n");
//	::printf("-o: Hold trackerfile open. (debug)\n");
//	::printf("-x n: Movie play limit. (debug)\n");
//	::printf("-g file: Parse and Display broadcast settings. (debug)\n");

}



static PlaylistPicker* MakePickerFromConfig( PLBroadcastDef* broadcastParms )
{
	// construct a PlaylistPicker object using options set from a PLBroadcastDef
	
	PlaylistPicker *picker = NULL;
	
	if ( broadcastParms && broadcastParms->ParamsAreValid() )
	{
		if ( broadcastParms->mPlayMode )
		{
			if ( !::strcmp( broadcastParms->mPlayMode, "weighted_random" ) )
			{
				int		noPlayQueueLen = 0;
				
				if ( broadcastParms->mLimitPlayQueueLength )
					noPlayQueueLen = atoi( broadcastParms->mLimitPlayQueueLength );
				
				picker = new PlaylistPicker( 10, noPlayQueueLen );
				
			}
			else if ( !::strcmp( broadcastParms->mPlayMode, "sequential_looped" ) )
			{			
				picker = new PlaylistPicker(true);
			}
			else if ( !::strcmp( broadcastParms->mPlayMode, "sequential" ) )
			{			
				picker = new PlaylistPicker(false);
			}
		
		}
	}
	
	return picker;
}

static const char* GetMovieFileErrString( int err )
{
	static 	char buff[80];
	
	switch ( err )
	{
		case 0:
			break;
			
		case QTFileBroadcaster::eMovieFileNotFound:
			return "Movie file not found."; 
		
		case QTFileBroadcaster::eMovieFileNoHintedTracks:
			return "Movie file has no hinted tracks."; 

		case QTFileBroadcaster::eMovieFileNoSDPMatches : 
			return "Movie file does not match SDP.";
			
		case QTFileBroadcaster::eMovieFileInvalid:
			return "Movie file is invalid.";
			
		case QTFileBroadcaster::eMovieFileInvalidName:
			return "Movie file name is missing or too long."; 

		default:
			sprintf( buff, "Movie set up error %d occured.",err );
			return buff;
	}
	
	return NULL;

}


static int EvalBroadcasterErr(int err)
{
	int returnErr = err;
	
	switch (err)
	{
		case QTFileBroadcaster::eNoAvailableSockets :	
		case QTFileBroadcaster::eSDPFileNotFound :
		case QTFileBroadcaster::eSDPDestAddrInvalid :
		case QTFileBroadcaster::eSDPFileInvalid 	 : 
		case QTFileBroadcaster::eSDPFileNoMedia 	 : 
		case QTFileBroadcaster::eSDPFileNoPorts 	 :
		case QTFileBroadcaster::eSDPFileInvalidPort  :
		case QTFileBroadcaster::eSDPFileInvalidName  :
		{	printf("- SDP set up failed: ");
			switch( err )
			{
				case QTFileBroadcaster::eNoAvailableSockets : printf("System error. No sockets are available to broadcast from."); 
				break;
				case QTFileBroadcaster::eSDPFileNotFound : printf("The SDP file is missing."); 
				break;
				case QTFileBroadcaster::eSDPDestAddrInvalid : printf("The SDP file server address is invalid."); 
				break;
				case QTFileBroadcaster::eSDPFileInvalid 	 : printf("The SDP file is invalid."); 
				break; 
				case QTFileBroadcaster::eSDPFileNoMedia 	 : printf("The SDP file missing media (m=) references."); 
				break; 
				case QTFileBroadcaster::eSDPFileNoPorts 	 : printf("The SDP file missing server port information."); 
				break;
				case QTFileBroadcaster::eSDPFileInvalidPort  : printf("The SDP file server port value is invalid. Valid range is 1001 - 65535."); 
				break;
				case QTFileBroadcaster::eSDPFileInvalidName  : printf("The specified SDP file name is missing or too long."); 
				break;
				
				default: printf("SDP set up error %d occured.",err);
				break;	
			
			};
			printf("\n");
		}
		break;
		
		case QTFileBroadcaster::eMovieFileNotFound	 		:
		case QTFileBroadcaster::eMovieFileNoHintedTracks 	:
		case QTFileBroadcaster::eMovieFileNoSDPMatches 	:
		case QTFileBroadcaster::eMovieFileInvalid 			:
		case QTFileBroadcaster::eMovieFileInvalidName 		:
		{	printf("- Movie set up failed: ");
			switch( err )
			{	
				case QTFileBroadcaster::eMovieFileNotFound	 		: printf("Movie file not found."); 
				break;
				case QTFileBroadcaster::eMovieFileNoHintedTracks 	: printf("Movie file has no hinted tracks."); 
				break;
				case QTFileBroadcaster::eMovieFileNoSDPMatches 	: printf("Movie file does not match SDP."); 
				break;
				case QTFileBroadcaster::eMovieFileInvalid 			: printf("Movie file is invalid."); 
				break;
				case QTFileBroadcaster::eMovieFileInvalidName 		: printf("Movie file name is missing or too long."); 
				break;
		
				default: printf("Movie set up error %d occured.",err);
				break;	
		
			};
			printf("\n");
		}
		break;
		
		case QTFileBroadcaster::eMem:		
		case QTFileBroadcaster::eInternalError:
			printf("- Internal Error: ");
			switch( err )
			{	
				case QTFileBroadcaster::eMem	 		: printf("Memory error."); 
				break;
				
				case QTFileBroadcaster::eInternalError	 : printf("Internal error."); 
				break;
			
				default: printf("internal error %d occured.",err);
				break;	
			}
			printf("\n");
		break;
			
		default:
			
		break;
	}
	
	return returnErr;
}


static int MyAccess( const char* path, int mode )
{
	int 	error = 0;
	
#ifndef __MACOS__
#ifndef __Win32__
	if ( access( path, mode ) )
		error = errno;
	else
		error = 0;
#endif
#endif
	
	
	return error;
}


static int PreflightSDPFileAccess( const char * sdpPath )
{
	int 	numPreflightErrors = 0;

	Assert( sdpPath );
	
	if ( !sdpPath )
	{	::printf("- PlaylistBroadcaster: The sdp_file parameter is missing from the Broadcaster description file.\n" );
		numPreflightErrors++;
	}
	else
	{	int		accessError;
	
		accessError = MyAccess( sdpPath, R_OK | W_OK );
		
		switch( accessError )
		{
		
			case 0:
			case ENOENT:	// if its not there, we'll create it
				break;
			
			case EACCES:
				::printf("- PlaylistBroadcaster: Permission to access the SDP File was denied.\n  Read/Write access is required.\n  (path: %s, errno: %i).\n", sdpPath, accessError);
				numPreflightErrors++;
				break;
				
			default:
				::printf("- PlaylistBroadcaster: Unable to access the SDP File.\n  (path: %s, errno: %i).\n", sdpPath, accessError);
				numPreflightErrors++;
				break;
				
		}
	
	}

	return numPreflightErrors;

}

static int PreflightDestinationAddress( const char *destAddress )
{
	int 	numPreflightErrors = 0;

	Assert( destAddress );

	if ( !destAddress )
	{	::printf("- PlaylistBroadcaster: Error, desitination_ip_address parameter missing from the Broadcaster description file.\n" );
		numPreflightErrors++;
	}
	
	return numPreflightErrors;

}

static int PreflightBasePort( const char *basePort )
{
	int 	numPreflightErrors = 0;

		
	Assert( basePort );
	if ( basePort == NULL )
	{	::printf("- PlaylistBroadcaster: Error, destination_base_port parameter missing from the Broadcaster description file.\n" );
		numPreflightErrors++;
	}
	else if ( (atoi(basePort) & 1)  != 0)
	{
		::printf("- PlaylistBroadcaster: Warning, the destination_base_port parameter(%s) is an odd port number.  It should be even.\n", basePort );		
	}
	
	return numPreflightErrors;

}


static int PreflightReferenceMovieAccess( const char *refMoviePath )
{
	int 	numPreflightErrors = 0;
	
	Assert( refMoviePath );
	if ( !refMoviePath )
	{	::printf("- PlaylistBroadcaster: Error, sdp_reference_movie parameter missing from the Broadcaster description file.\n" );
		numPreflightErrors++;
	}
	else
	{	int		accessError;
	
		accessError = MyAccess( refMoviePath, R_OK );
		
		switch( accessError )
		{			
			case 0:
				break;
				
			case ENOENT:
				::printf("- PlaylistBroadcaster: Error, SDP Reference Movie is missing.\n  (path: %s, errno: %i).\n", refMoviePath, accessError);
				numPreflightErrors++;
				break;
			
			case EACCES:
				::printf("- PlaylistBroadcaster: Permission to access the SDP reference movie was denied.\n  Read access required.\n  (path: %s, errno: %i).\n", refMoviePath, accessError);
				numPreflightErrors++;
				break;
				
			default:
				::printf("- PlaylistBroadcaster: Unable to access the SDP reference movie.\n  (path: %s, errno: %i).\n", refMoviePath, accessError);
				numPreflightErrors++;
				break;
				
		}
	
	}

	return numPreflightErrors;
}


static void RegisterEventHandlers()
{
#ifndef __MACOS__	

   	if ( ::signal(SIGTERM, SIG_IGN) != SIG_IGN) 
    {	// from kill...
		if ( ::signal(SIGTERM, SignalEventHandler ) == SIG_ERR )
     	{	::printf( "- PlaylistBroadcaster: System error (%i).\n", (int)SIG_ERR );
     	}
 	}

    if ( ::signal(SIGINT, SIG_IGN) != SIG_IGN) 
    {	// ^C signal
		if ( ::signal(SIGINT, SignalEventHandler )  == SIG_ERR  )
    	{	::printf( "- PlaylistBroadcaster: System error (%i).\n", (int)SIG_ERR );
     	}
     	
    }
    
#endif


}


static void ShowPlaylistStatus()
{	
#ifndef __Win32__
	BCasterTracker	tracker( sgTrackerFilePath );

	if ( tracker.IsOpen() )
	{
		tracker.Show();
	}
	else
	{	
		::printf("- PlaylistBroadcaster: Could not open %s.  Reason: access denied.\n", sgTrackerFilePath );
		::printf("- PlaylistBroadcaster: Change user or change the directory's privileges to access %s.\n",sgTrackerFilePath) ;
		::exit(-1);

	}
#else
	::printf("Showing Playlist status is currently not supported on this platform\n");
#endif
}

static void StopABroadcast( const char* arg )
{
#ifndef __Win32__
	if ( arg )
	{	
		
		BCasterTracker	tracker( sgTrackerFilePath );
		int				playlistIDToKill;
		
		playlistIDToKill = ::atoi( arg );						
		playlistIDToKill--; 	// convert from UI one based to to zero based ID for BCasterTracker

		
		if ( tracker.IsOpen() )
		{
			bool	broadcastIDIsValid;
			int		error;
			
			error = tracker.Remove( playlistIDToKill );
			
			if ( !error )	// remove the xth item from the list.
			{	tracker.Save();
				broadcastIDIsValid = true;	
			}
			else
				broadcastIDIsValid = false;
				
			if ( !broadcastIDIsValid )
			{
				if ( playlistIDToKill >= 0 )
				{
					if ( error == ESRCH || error == -1 )
						::printf( "- PlaylistBroadcaster Broadcast ID (%s) not running.\n", arg );
					else
						::printf( "- PlaylistBroadcaster Broadcast ID (%s), permission to stop denied (%i).\n", arg, error );
					
				}
				else
				{	::printf( "- Bad argument for stop: (%s).\n", arg );
					::exit( -1 );
				}
			}
			else
				::printf("PlaylistBroadcaster stopped Broadcast ID: %s.\n", arg);
		}
		else
		{	::printf("- PlaylistBroadcaster: Could not open %s.  Reason: access denied.\n", sgTrackerFilePath );
			::printf("- PlaylistBroadcaster: Change user or change the directory's privileges to access %s.\n",sgTrackerFilePath) ;
			::exit( -1 );

		}
	}
	else
	{	// getopt will catch this problem before we see it.
		::printf("- Stop requires a Broadcast ID.\n");
		::exit( -1 );
	}
#else
	::printf("Stopping a broadcast is currently not supported on this platform\n");
#endif
}


static bool DoSDPGen( PLBroadcastDef *broadcastParms, bool preflight )
{
	int numSDPSetupErrors = 0;
	bool sdpFileCreated = false;
	
	numSDPSetupErrors += PreflightSDPFileAccess( broadcastParms->mSDPFile );
	
	
	numSDPSetupErrors += PreflightReferenceMovieAccess( broadcastParms->mSDPReferenceMovie );
	
	
	numSDPSetupErrors += PreflightDestinationAddress( broadcastParms->mDestAddress );


	numSDPSetupErrors += PreflightBasePort( broadcastParms->mBasePort );

	
	if ( numSDPSetupErrors == 0 )
	{
		SDPGen*	sdpGen = new SDPGen;
		
		int 	sdpResult = -1;
		
		Assert( sdpGen );
		
		if ( sdpGen )
		{
			sdpResult = sdpGen->Run( broadcastParms->mSDPReferenceMovie, broadcastParms->mSDPFile, 
									  broadcastParms->mBasePort, broadcastParms->mDestAddress, 
									  NULL, 0, 
									  false );
		
			sdpFileCreated =  sdpGen->fSDPFileCreated;
			
			if ( sdpGen->fSDPFileCreated && !preflight)
				::printf( "- PlaylistBroadcaster: Created SDP file.\n  (path: %s)\n", broadcastParms->mSDPFile);
		}
		
		
		if ( sdpResult )
		{
			::printf( "- SDP generation failed (error: %li).\n", (long)sdpResult );
			::exit(-1);
		}
		
		if ( sdpGen )
			delete sdpGen;
		
		sdpGen = NULL;
	}
	else
	{	::printf( "- PlaylistBroadcaster: Too many SDP set up errors, exiting.\n" );
		::exit(-1);
	}

	return sdpFileCreated;

}

static bool PreflightTrackerFileAccess( int mode )
{
	int		trackerError;
	
#ifndef __Win32__	
	int dirlen = strlen(sgTrackerDirPath) + 1;
	OSCharArrayDeleter trackDir(new char[dirlen]);
	char *trackDirPtr = trackDir.GetObject();
	::memcpy(trackDirPtr, sgTrackerDirPath, dirlen);

	trackerError= OS::RecursiveMakeDir( trackDirPtr );
	
	if ( trackerError == 0 )
		trackerError = MyAccess( sgTrackerFilePath, mode );
	
	switch ( trackerError )
	{			
		case 0:
		case ENOENT:
			break;
			
		default:
			::printf("- PlaylistBroadcaster: Error opening %s.  (errno: %i).\n", sgTrackerFilePath, trackerError);
			::printf("- PlaylistBroadcaster: Change user or change the directory's privileges to access %s.\n",sgTrackerFilePath) ;
			return false;
			break;
			
	}
#endif
	return true;
}

static void ShowSetupParams(PLBroadcastDef*	broadcastParms, const char *bcastSetupFilePath)
{		
	::printf( "\n" );
	::printf( "PlaylistBroadcaster broadcast description File\n" );
	::printf( "----------------------------------------------\n" );
	::printf( "%s\n", bcastSetupFilePath );

	broadcastParms->ShowSettings();

}

static void PreFlightSetupFile( const char * bcastSetupFilePath )
{
	// now complain!
	if ( !bcastSetupFilePath )
	{	::printf("- PlaylistBroadcaster: A broadcast description file is required.\n" );
		::usage();
		::exit(-1);
	}

	{	int		accessError;
	
		accessError = MyAccess(bcastSetupFilePath, R_OK  );
		
		switch( accessError )
		{
		
			case 0:
				break;
				
			case ENOENT:	
				::printf("- PlaylistBroadcaster: The broadcast description file is missing.\n  (path: %s, errno: %i).\n", bcastSetupFilePath, accessError);
				::exit (-1);
				break;
			
			case EACCES:
				::printf("- PlaylistBroadcaster: Permission denied to access the broadcast description file.\n  Read access required.\n  (path: %s, errno: %i).\n", bcastSetupFilePath, accessError);
				::exit (-1);
				break;
				
			default:
				::printf("- PlaylistBroadcaster: Unable to access the broadcast description file.\n  (path: %s, errno: %i).\n", bcastSetupFilePath, accessError);
				::exit (-1);
				break;
				
		}
	
	}


}

static void AddOurPIDToTracker( const char* bcastSetupFilePath )
{
	// add our pid and Broadcast description file File to the tracker
#ifndef __Win32__	
	BCasterTracker	tracker( sgTrackerFilePath );
	
	if ( tracker.IsOpen() )
	{	sgTrackingSucceeded = 1;
		tracker.Add( getpid(), bcastSetupFilePath );
		tracker.Save();
	}
	else
	{				
		::printf("- PlaylistBroadcaster: Could not open %s.  Reason: access denied.\n", sgTrackerFilePath );
		::printf("- PlaylistBroadcaster: Change user or change the directory's privileges to access %s.\n",sgTrackerFilePath) ;
		::exit(-1);
	}
#endif
}

#if 0

static void ShowPickDistribution( PlaylistPicker *picker )
{		
	::printf( "\n" );
	::printf( "Pick Distribution by Bucket\n" );
	::printf( "---------------------------\n" );
	
	UInt32		bucketIndex;
	
	for ( bucketIndex = 0; bucketIndex < picker->GetNumBuckets(); bucketIndex++ )
	{	
		::printf( "bucket total for w: %li, (%li)\n", (bucketIndex + 1), (long)picker->mPickCounts[bucketIndex] );
	
	}
}

// archaic debug code

static void PrintContents( PLDoubleLinkedListNode<SimplePlayListElement> *node, void *  );
static void ShowPicker(PlaylistPicker *picker);


static void PrintContents( PLDoubleLinkedListNode<SimplePlayListElement> *node, void *  )
{
	::printf( "element name %s\n", node->mElement->mElementName );
}

static void ShowPicker(PlaylistPicker *picker)
{
	int	x;
	
	for ( x= 0; x < picker->GetNumBuckets(); x++ )
	{
		picker->GetBucket(x)->ForEach( PrintContents, NULL );
	}

}

#endif

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
#include "getopt.h"

#ifndef __Win32__
#ifndef __MACOS__
	#if defined (__solaris__) || defined (__osf__)
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
#include "OSMemory.h"
#include "SocketUtils.h"
#include "Socket.h"
#include "Task.h"
#include "TimeoutTask.h"
#include "BroadcasterSession.h"
#include "OSArrayObjectDeleter.h"
#include "PickerFromFile.h"
#include "PLBroadcastDef.h"
#include "BroadcastLog.h"
#include "StringTranslator.h"

#ifndef __Win32__
	#include "BCasterTracker.h"
#endif

#ifdef __Win32__
	#include "getopt.h"
#endif	

// must now inlcude this from the project level using the -i switch in the compiler
#ifndef __MacOSX__
	#include "revision.h"
#endif

#include "playlist_SDPGen.h"
#include "playlist_broadcaster.h"


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
static bool DoSDPGen( PLBroadcastDef *broadcastParms, bool preflight, bool overWriteSDP, bool generateNewSDP, int* numErrorsPtr, char* refMovie);

static bool AddOurPIDToTracker( const char* bcastSetupFilePath );

static void Cleanup();

/* -- preflighting --*/

static int MyAccess( const char* path, int mode );
static int PreflightSDPFileAccess( const char * sdpPath );
static int PreflightDestinationAddress( const char *destAddress );
static int PreflightBasePort( const char *basePort );
static int PreflightReferenceMovieAccess( const char *refMoviePath );
static bool PreflightTrackerFileAccess( int mode );
static bool PreFlightSetupFile( const char * bcastSetupFilePath );
static int PreflightClientBufferDelay( const char * delay, Float32 *ioDelay);
static void ShowSetupParams(PLBroadcastDef*	broadcastParms, const char *file);
static int PreflightDestSDPFileAccess( const char * sdpPath );
static void PreFlightOrBroadcast( const char *bcastSetupFilePath, bool preflight, bool daemonize, bool showMovieList, bool writeCurrentMovieFile, char *destinationIP,bool writeNewSDP, const char* errorFilePath);

/* changed by emil@popwire.com */
static bool FileCreateAndCheckAccess(char *theFileName);
static void PrintPlaylistElement(PLDoubleLinkedListNode<SimplePlayListElement> *node,void *file);
static void ShowPlaylistElements(PlaylistPicker *picker,FILE *file);
static void RemoveFiles(PLBroadcastDef*	broadcastParms);
/* ***************************************************** */

/*
	local variables
*/
static char*				sStartMessage = "PLAY"; 
static char* 				sgProgramName; // the actual program name as input at commmand line
static char* 				sgTrackerDirPath = "/var/run";
static char* 				sgTrackerFilePath = "/var/run/broadcastlist";
static BroadcastLog*		sgLogger = NULL;
static bool				sgTrackingSucceeded = false;
static BroadcasterSession* sBroadcasterSession = NULL;
static	StrPtrLen			sSetupDirPath;
static	PlaylistPicker* 	sTempPicker = NULL;
static SInt32				sElementCount = 0;
static SInt32				sMaxUpcomingListSize = 5;
static PLBroadcastDef*		sBroadcastParms = NULL;

enum {maxSDPbuffSize = 10000};
static char			sSDPBuffer[maxSDPbuffSize] = {0};
static bool			sQuitImmediate = false;
#ifdef __MACOS__
	static char *kMacOptions = "pdv";
	static char *kMacFilePath = "Work:Movies:test";
#endif

static	bool	sAnnounceBroadcast = false;
static	bool	sPushOverTCP = false;
static	int	sRTSPClientError = 0;
static	bool	sErrorEvaluted = false;
static	bool	sDeepDebug = false;

static int	sNumWarnings = 0;
static int	sNumErrors = 0;
static bool	sPreflight = false;
static bool	sCleanupDone = false;

//+ main is getting too big.. need to clean up and separate the command actions 
// into a separate file.


int main(int argc, char *argv[]) {

	char	*bcastSetupFilePath = NULL;
	bool 	daemonize = true;
	int	anOption = 0;
	bool	preflight = false;
	bool	showMovieList = false;
	bool	writeCurrentMovieFile = false;
	int	displayedOptions = 0; // count number of command line options we displayed
	bool	needsTracker = false;	// set to true when PLB needs tracker access
	bool	needsLogfile = false;	// set to true when PLB needs tracker access
	char* 	destinationIP = NULL;
	bool 	writeNewSDP = false;
	char*	errorlog = NULL;    
    extern char* optarg;
    extern int optind;

#ifdef __Win32__		
	//
	// Start Win32 DLLs
	WORD wsVersion = MAKEWORD(1, 1);
	WSADATA wsData;
	(void)::WSAStartup(wsVersion, &wsData);
#endif


	QTRTPFile::Initialize(); // do only once
	OS::Initialize();
	OSThread::Initialize();
	OSMemory::SetMemoryError(ENOMEM);
	Socket::Initialize();
	SocketUtils::Initialize(false);
	
	if (SocketUtils::GetNumIPAddrs() == 0)
	{
		printf("Network initialization failed. IP must be enabled to run PlaylistBroadcaster\n");
		::exit(0);
	}
#ifndef __MACOS__
	sgProgramName = argv[0];
#ifdef __Win32__
	while ((anOption = getopt(argc, argv, "vhdcpDtai:fe:" )) != EOF)
#else
	while ((anOption = getopt(argc, argv, "vhdcpDls:tai:fe:" )) != EOF)
#endif
#else
	sgProgramName = "PlaylistMac";
	char *theOption = kMacOptions;
 	int count = 0;
	while (anOption = theOption[count++])
#endif
	{
		
		switch(anOption)
		{
			case 'v':
				::version();
				::usage();
				return 0;

			case 'h':
				::help();
				displayedOptions++;
				break;
			
			case 'd':
				daemonize = false;
				break;
									
			case 'c':
				writeCurrentMovieFile = true;
				break;				
				
/*
			case 'm':
				showMovieList = true;
				daemonize = false;
				break;
*/
							
			case 'p':
				preflight = true;
				daemonize = false;
				needsTracker = true;
				needsLogfile = true;
				break;
				
			case 'D':
				sDeepDebug = true;
				daemonize = false;
				break;

#ifndef __Win32__

			
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
			
			case 't':
				sAnnounceBroadcast = true;
				sPushOverTCP = true;
				break;

			case 'a':
				sAnnounceBroadcast = true;
				break;				
			
			case 'i':
				destinationIP = (char*)malloc(strlen(optarg)+1);
				strcpy(destinationIP, optarg);
				printf("destinationIP =%s\n",destinationIP);
				break;
				
			case 'f':
				writeNewSDP = true;
				break;
				
			case 'e':
				errorlog = (char*)malloc(strlen(optarg)+1);
				strcpy(errorlog, optarg);
				break;
				
			default:
			    ::usage();
			    ::exit(-1);
		}
	}

#ifndef __MACOS__
	if (argv[optind] != NULL)
    {
        bcastSetupFilePath = (char*)malloc(strlen(argv[optind])+1);
        strcpy(bcastSetupFilePath, argv[optind]);
    }
#else
	bcastSetupFilePath = kMacFilePath;
#endif

        sPreflight = preflight;
         
	if (errorlog != NULL)
	{
	        if (preflight)
	                freopen(errorlog, "w", stdout);
		else
		        freopen(errorlog, "a", stdout);
		::setvbuf(stdout, (char *)NULL, _IONBF, 0);
	}
		
	// preflight needs a description file
	if ( preflight && !bcastSetupFilePath )
	{	::printf("- PlaylistBroadcaster: Error.  \"Preflight\" requires a broadcast description file.\n" );
		::usage();
		::exit(-1);
	}

	// don't complain about lack of description File if we were asked to display some options.	
	if ( displayedOptions > 0 && !bcastSetupFilePath )
		::exit(0);		// <-- we displayed option info, but have no description file to broadcast
		
	PreFlightOrBroadcast( bcastSetupFilePath, preflight, daemonize, showMovieList, writeCurrentMovieFile,destinationIP,writeNewSDP, errorlog ); // <- exits() on failure -- NOT ANYMORE
	
	return 0;

}

BroadcasterSession *StartBroadcastRTSPSession(PLBroadcastDef *broadcastParms)
{	
	TaskThreadPool::AddThreads(OS::GetNumProcessors());
	TimeoutTask::Initialize();
	Socket::StartThread();

	UInt32 inAddr = SocketUtils::ConvertStringToAddr(broadcastParms->mDestAddress);
	
	UInt16 inPort = atoi(broadcastParms->mRTSPPort);
	char * theURL = new char[512];
	StringTranslator::EncodeURL(broadcastParms->mDestSDPFile, strlen(broadcastParms->mDestSDPFile) + 1, theURL, 512);
	BroadcasterSession::BroadcasterType inBroadcasterType;
		
	if (sPushOverTCP)
		 inBroadcasterType = BroadcasterSession::kRTSPTCPBroadcasterType;
	else
		 inBroadcasterType = BroadcasterSession::kRTSPUDPBroadcasterType;
	
	UInt32 inDurationInSec = 0;
	UInt32 inStartPlayTimeInSec = 0;
	UInt32 inRTCPIntervalInSec = 5; 
	UInt32 inOptionsIntervalInSec = 0;
	UInt32 inHTTPCookie = 1; 
	Bool16 inAppendJunkData = false; 
	UInt32 inReadInterval = 50;
	UInt32 inSockRcvBufSize = 32768;
	StrPtrLen sdpSPL(sSDPBuffer,maxSDPbuffSize);
	sRTSPClientError = QTFileBroadcaster::eNetworkConnectionError;
	sBroadcasterSession = NEW BroadcasterSession(inAddr, 
							inPort,					
							theURL,
							inBroadcasterType,		// Client type
							inDurationInSec,		// Movie length
							inStartPlayTimeInSec,					// Movie start time
							inRTCPIntervalInSec,					// RTCP interval
							inOptionsIntervalInSec,					// Options interval
							inHTTPCookie,	// HTTP cookie
							inAppendJunkData,
							inReadInterval,	// Interval between data reads	
							inSockRcvBufSize,
							&sdpSPL,
							broadcastParms->mName,
							broadcastParms->mPassword,
							sDeepDebug);	

	return 	sBroadcasterSession;
}

Bool16 AnnounceBroadcast(PLBroadcastDef *broadcastParms,QTFileBroadcaster 	*fileBroadcasterPtr)
{	// return true if no Announce required or broadcast is ok.

	if (!sAnnounceBroadcast) return true;
	
#if !MACOSXEVENTQUEUE
	::select_startevents();//initialize the select() implementation of the event queue
#endif

	if (SocketUtils::GetNumIPAddrs() == 0)
	{
		printf("IP must be enabled to run PlaylistBroadcaster\n");
		//::exit(0); // why exit here? If we return false here, the calling function will take care of the error
                return false;
	}

	broadcastParms->mTheSession = sBroadcasterSession = StartBroadcastRTSPSession(broadcastParms);
	while (!sBroadcasterSession->IsDone() && BroadcasterSession::kBroadcasting != sBroadcasterSession->GetState()) 
	{	OSThread::Sleep(100);
	}
	sRTSPClientError = 0;
	int broadcastErr = sBroadcasterSession->GetRequestStatus();
	
	Bool16 isOK = false;	
	if (broadcastErr != 200) do 
	{
		if (412 == broadcastErr)
		{	
			::EvalBroadcasterErr(broadcastErr);
			break;	
		}
		
		if (200 != broadcastErr)
		{	//printf("broadcastErr = %ld sBroadcasterSession->GetDeathState()=%ld sBroadcasterSession->GetReasonForDying()=%ld\n",broadcastErr,sBroadcasterSession->GetDeathState(),sBroadcasterSession->GetReasonForDying());
			if (sBroadcasterSession->GetDeathState() == BroadcasterSession::kSendingAnnounce && sBroadcasterSession->GetReasonForDying() == BroadcasterSession::kConnectionFailed)
				::EvalBroadcasterErr(QTFileBroadcaster::eNetworkConnectionError);
			else if (401 == broadcastErr)
				::EvalBroadcasterErr(QTFileBroadcaster::eNetworkAuthorization);
			else if ((500 == broadcastErr) || (400 == broadcastErr) )
				::EvalBroadcasterErr(QTFileBroadcaster::eNetworkNotSupported);
			else 
				::EvalBroadcasterErr(QTFileBroadcaster::eNetworkRequestError);

			break;
		}	
		
		if (sBroadcasterSession != NULL && BroadcasterSession::kDiedNormally != sBroadcasterSession->GetReasonForDying())
		{	::EvalBroadcasterErr(QTFileBroadcaster::eNetworkRequestError);
			break;
		}	
	
		isOK = true;
		
	} while (false);
	else 
		isOK = true;
		
	if (isOK)
	{
		broadcastErr = fileBroadcasterPtr->SetUp(broadcastParms, &sQuitImmediate);
		if (  broadcastErr )
		{	::EvalBroadcasterErr(broadcastErr);
			::printf( "- Broadcaster setup failed.\n" );
			isOK = false;
			if (sBroadcasterSession != NULL && !sBroadcasterSession->IsDone())
				sBroadcasterSession->TearDownNow();
		}
	}
	
	return isOK;
}

char* GetBroadcastDirPath(const char * setupFilePath)
{
	int len = 2;
	
	if (setupFilePath != NULL)
		len = ::strlen(setupFilePath);

	char *setupDirPath = new char [ len + 1 ];
	
	if ( setupDirPath )
	{	
		strcpy( setupDirPath, setupFilePath );
		char* endOfDirPath = strrchr( setupDirPath, kPathDelimiterChar );
		
		if ( endOfDirPath )
		{
			endOfDirPath++;
			*endOfDirPath= 0;			
			
			int  chDirErr = ::chdir( setupDirPath );			
			if ( chDirErr )
				chDirErr = errno;
				
			//::printf("- PLB DEBUG MSG: setupDirPath==%s\n  chdir err: %i\n", setupDirPath, chDirErr);			
		}
		else
		{	
			setupDirPath[0] = '.';
			setupDirPath[1] = kPathDelimiterChar;
			setupDirPath[2] = 0;
		}
			
	}
	
	//printf("GetBroadcastDirPath setupDirPath = %s\n",setupDirPath);
	return setupDirPath;
}

void CreateCurrentAndUpcomingFiles(PLBroadcastDef* broadcastParms)
{
	if (!::strcmp(broadcastParms->mShowCurrent, "enabled")) 
	{	if(FileCreateAndCheckAccess(broadcastParms->mCurrentFile))
		{ 	/* error */
		  	sgLogger->LogInfo( "PlaylistBroadcaster Error: Failed to create current broadcast file" );
		}
	}


	if (!::strcmp(broadcastParms->mShowUpcoming, "enabled")) 
	{	if(FileCreateAndCheckAccess(broadcastParms->mUpcomingFile))
		{	/* error */
			sgLogger->LogInfo( "PlaylistBroadcaster Error:  Failed to create upcoming broadcast file" );
		}
	}

}

void UpdateCurrentAndUpcomingFiles(PLBroadcastDef* broadcastParms, char *thePick, PlaylistPicker *picker,PlaylistPicker *insertPicker)
{
	if ( 	(NULL == broadcastParms)
		||  (NULL == thePick)
		||  (NULL == picker)
		||  (NULL == insertPicker)
		) return;
		
	// save currently playing song to .current file 
	if (!::strcmp(broadcastParms->mShowCurrent, "enabled")) 
	{	FILE *currentFile = fopen(broadcastParms->mCurrentFile, "w");
		if(currentFile)
		{
			if (sSetupDirPath.EqualIgnoreCase(thePick, sSetupDirPath.Len) || '\\' == thePick[0] || '/' == thePick[0])
				::fprintf(currentFile,"u=%s\n",thePick);
			else
				::fprintf(currentFile,"u=%s%s\n", sSetupDirPath.Ptr,thePick);

			fclose(currentFile);
		}	
	}
						
	/* if .stoplist file exists - prepare to stop broadcast */
	if(!access(broadcastParms->mStopFile, R_OK))
	{
		picker->CleanList();
		PopulatePickerFromFile(picker, broadcastParms->mStopFile, "", NULL);

		sTempPicker->CleanList();

		remove(broadcastParms->mStopFile);
		picker->mStopFlag = true;
	}

	/* if .replacelist file exists - replace current playlist */
	if(!access(broadcastParms->mReplaceFile, R_OK))
	{
		picker->CleanList();	 
		PopulatePickerFromFile(picker, broadcastParms->mReplaceFile, "", NULL);
		
		sTempPicker->CleanList();
		
		remove(broadcastParms->mReplaceFile);
		picker->mStopFlag = false;
	}

	/* if .insertlist file exists - insert into current playlist */
	if(!access(broadcastParms->mInsertFile, R_OK))
	{
		insertPicker->CleanList();
		sTempPicker->CleanList();

		PopulatePickerFromFile(insertPicker, broadcastParms->mInsertFile, "", NULL);
		remove(broadcastParms->mInsertFile);
		picker->mStopFlag = false;
	}


				// write upcoming playlist to .upcoming file 
	if (!::strcmp(broadcastParms->mShowUpcoming, "enabled")) 
	{
		FILE *upcomingFile = fopen(broadcastParms->mUpcomingFile, "w");
		if(upcomingFile)
		{
			sElementCount = 0;
		
			if (!::strcmp(broadcastParms->mPlayMode, "weighted_random")) 
				fprintf(upcomingFile,"#random play - upcoming list not supported\n");
			else
			{	fprintf(upcomingFile,"*PLAY-LIST*\n");
				ShowPlaylistElements(insertPicker,upcomingFile);
				ShowPlaylistElements(picker,upcomingFile);
				if (	picker->GetNumMovies() == 0 
						&& !picker->mStopFlag 
						&& 0 != ::strcmp(broadcastParms->mPlayMode, "sequential") 
					)
				{	picker->CleanList();
					PopulatePickerFromFile(picker,broadcastParms->mPlayListFile,"",NULL);
					ShowPlaylistElements(picker,upcomingFile);
					sTempPicker->CleanList();
					PopulatePickerFromFile(sTempPicker,broadcastParms->mPlayListFile,"",NULL);
				}
				
				if 	(	sElementCount <= sMaxUpcomingListSize 
						&& 0 == ::strcmp(broadcastParms->mPlayMode, "sequential_looped")
					)
				{	if (sTempPicker->GetNumMovies() == 0)
					{	sTempPicker->CleanList();
						PopulatePickerFromFile(sTempPicker,broadcastParms->mPlayListFile,"",NULL);
					}
					while (sElementCount <= sMaxUpcomingListSize )
						ShowPlaylistElements(sTempPicker,upcomingFile);
				}
			}
			fclose(upcomingFile);
		}	
	}
	else
	{	
		if (	picker->GetNumMovies() == 0 
				&& !picker->mStopFlag 
				&& ::strcmp(broadcastParms->mPlayMode, "sequential") 
			)
		{	picker->CleanList();
			PopulatePickerFromFile(picker,broadcastParms->mPlayListFile,"",NULL);
		}		
	}
}


static void PreFlightOrBroadcast( const char *bcastSetupFilePath, bool preflight, bool daemonize, bool showMovieList, bool writeCurrentMovieFile, char *destinationIP,bool writeNewSDP, const char* errorFilePath)
{
	PLBroadcastDef*		broadcastParms = NULL;
	PlaylistPicker* 	picker = NULL;
	PlaylistPicker* 	insertPicker = NULL;
	
	QTFileBroadcaster 	fileBroadcaster;
	int		broadcastErr = 0;
	long	moviePlayCount;
	char*	thePick = NULL;
	int		numMovieErrors;
	bool	sdpFileCreated = false;
	char	*allocatedIPStr = NULL;
	bool	generateNewSDP = false;
    int		numErrorsBeforeSDPGen = 0;
    char	theUserAgentStr[128];
    char*	thePLBStr = "PlaylistBroadcaster/";
    char*	strEnd		= NULL;
        
	RegisterEventHandlers();

	if ( !PreFlightSetupFile( bcastSetupFilePath ) )	// returns true on success and false on failure
        {
            sNumErrors++;
            goto bail;
        }
        
	if (destinationIP != NULL)
	{ 
		allocatedIPStr = new char[strlen(destinationIP)+1];
		strcpy(allocatedIPStr,destinationIP);
		generateNewSDP = true;
	}
	broadcastParms = new PLBroadcastDef( bcastSetupFilePath, allocatedIPStr,  sDeepDebug);	
	sBroadcastParms = broadcastParms;
	
	if( !broadcastParms )
	{	
		::printf("- PlaylistBroadcaster: Error. Out of memory.\n" );
		sNumErrors++;
		goto bail;
	}

	
	if ( !broadcastParms->ParamsAreValid() )
	{	
		::printf("- PlaylistBroadcaster: Error reading the broadcast description file \"%s\". (bad format or missing file)\n", bcastSetupFilePath );
		broadcastParms->ShowErrorParams();
		sNumErrors++;
		goto bail;
	}
	
	
	if ( preflight || sDeepDebug)
		ShowSetupParams(broadcastParms, bcastSetupFilePath);

	if (NULL == GetBroadcastDirPath(bcastSetupFilePath))
	{	
		::printf("- PlaylistBroadcaster: Error. Out of memory.\n" );
		sNumErrors++;
		goto bail;
	}
	sSetupDirPath.Set(GetBroadcastDirPath(bcastSetupFilePath), strlen(GetBroadcastDirPath(bcastSetupFilePath)));

	if (preflight)
	{
		picker = new PlaylistPicker(false);				// make sequential picker, no looping
	}
	else
	{	
		picker = MakePickerFromConfig( broadcastParms ); // make  picker according to parms
		sTempPicker =  new PlaylistPicker(false);
		insertPicker = new PlaylistPicker(false);
		insertPicker->mRemoveFlag = true;
	}
	
	if ( !picker )
	{	
		::printf("- PlaylistBroadcaster: Error. Out of memory.\n" );
		sNumErrors++;
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
	{	
		printf( "- PlaylistBroadcaster setup failed: There are no movies to play.\n" );
		sNumErrors++;
		goto bail;
	}
	

	// check that we have enough movies to cover the recent movies list.
	if ( preflight && broadcastParms->ParamsAreValid() )
	{
		if (  !strcmp( broadcastParms->mPlayMode, "weighted_random" ) ) // this implies "random" play
		{	
			if ( broadcastParms->mLimitPlayQueueLength >=  picker->mNumToPickFrom )
			{
				broadcastParms->mLimitPlayQueueLength = picker->mNumToPickFrom-1;
				printf("- PlaylistBroadcaster Warning:\n  The recent_movies_list_size setting is greater than \n");
				printf("  or equal to the number of movies in the playlist.\n" );
			}
		}
	}
	
	// create the log file
	sgLogger = new BroadcastLog( broadcastParms, &sSetupDirPath );
	
	
	Assert( sgLogger != NULL );
	
	if( sgLogger == NULL )
	{	
		::printf("- PlaylistBroadcaster: Error. Out of memory.\n" );
		sNumErrors++;
		goto bail;
	}
	
        numErrorsBeforeSDPGen = sNumErrors;
	sdpFileCreated = DoSDPGen( broadcastParms, preflight, writeNewSDP,generateNewSDP, &sNumErrors, picker->GetFirstFile());
        if( sNumErrors > numErrorsBeforeSDPGen )
            goto bail;
            
	if (!sAnnounceBroadcast)
		broadcastErr = fileBroadcaster.SetUp(broadcastParms, &sQuitImmediate);
	
	if (  broadcastErr )
	{	
		::EvalBroadcasterErr(broadcastErr);
		::printf( "- Broadcaster setup failed.\n" );
		sNumErrors++;
		goto bail;
	}
	
	if (preflight)
	{	 
		fileBroadcaster.fPlay = false;
	}
	if ( !PreflightTrackerFileAccess( R_OK | W_OK ) )
	{
		sNumErrors++;
		goto bail;
	}
       
	//Unless the Debug command line option is set, daemonize the process at this point
	if (daemonize)
	{	
#ifndef __Win32__
		::printf("- PlaylistBroadcaster: Started in background.\n");
		
		// keep the same working directory..
		if (::daemon( 1, 0 ) != 0)
		{
			::printf("- PlaylistBroadcaster:  System error (%i).\n", errno);
			goto bail;
		}

#endif	//__Win32__
	}
        
        // If daemonize, then reopen stdout to the error file 
        if (daemonize && (errorFilePath != NULL))
        {
                freopen(errorFilePath, "a", stdout);
                ::setvbuf(stdout, (char *)NULL, _IONBF, 0);
        }
        
        // Set up our User Agent string for the RTSP client
        strEnd = strchr(kVersionString, ' ');
        Assert(strEnd != NULL);

#ifndef __Win32__
	snprintf(theUserAgentStr, ::strlen(thePLBStr) + (strEnd - kVersionString) + 1, "%s%s", thePLBStr, kVersionString);
#else
	_snprintf(theUserAgentStr, ::strlen(thePLBStr) + (strEnd - kVersionString) + 1, "%s%s", thePLBStr, kVersionString);
#endif
	RTSPClient::SetUserAgentStr(theUserAgentStr);
        
        if (!preflight && !AnnounceBroadcast(broadcastParms,&fileBroadcaster)) 
	{
		sNumErrors++;
		goto bail;
	}
        
        // ^ daemon must be called before we Open the log and tracker since we'll
	// get a new pid, our files close,  ( does SIGTERM get sent? )
	
	if (( sgLogger != NULL ) && ( sgLogger->WantsLogging() ))
		sgLogger->EnableLog( false ); // don't append ".log" to name for PLB
	
	if ( sgLogger->WantsLogging() && !sgLogger->IsLogEnabled() )
	{
		if (  sgLogger->LogDirName() && *sgLogger->LogDirName() )
			::printf("- PlaylistBroadcaster: The log file failed to open.\n  ( path: %s/%s )\n  Exiting.\n", sgLogger->LogDirName(), sgLogger->LogFileName() );
		else
			::printf("- PlaylistBroadcaster: The log file failed to open.\n  ( path: %s )\n  Exiting.\n", sgLogger->LogFileName() );
		
		sNumErrors++;
		goto bail;
	}
	
	
	if (broadcastParms->mPIDFile != NULL)
	{
		if(!FileCreateAndCheckAccess(broadcastParms->mPIDFile))
		{
			FILE *pidFile = fopen(broadcastParms->mPIDFile, "w");
			if(pidFile)
			{
				::fprintf(pidFile,"%d\n",getpid());
				fclose(pidFile);
			}	
		}
	}
	else if ( !AddOurPIDToTracker( bcastSetupFilePath ) ) // <-- doesn't exit on failure anymore - returns false if failed
        {
            // writes to the broadcast list file only if the pid_file config param doesn't exist
            sNumErrors++;
            goto bail;
        }
        
	if ( !preflight || showMovieList )
		sgLogger->LogInfo( "PlaylistBroadcaster started." );
	else
		sgLogger->LogInfo( "PlaylistBroadcaster preflight started." );

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

	if(!preflight)
		CreateCurrentAndUpcomingFiles(broadcastParms);
	
	moviePlayCount = 0;
	numMovieErrors = 0;
	sMaxUpcomingListSize = ::atoi( broadcastParms->mMaxUpcomingMovieListSize );
			
	while (true)
	{	
		if (NULL != insertPicker)
			thePick = insertPicker->PickOne(); 
			
		if (NULL == thePick && (NULL != picker))
			thePick = picker->PickOne();
		
		if ( (thePick != NULL) && (!preflight || showMovieList) )
		{
			// display the picks in debug mode, but not preflight
			printf( "[%li] ", moviePlayCount );
			{	
				if (sSetupDirPath.EqualIgnoreCase(thePick, sSetupDirPath.Len) || '\\' == thePick[0] || '/' == thePick[0])
					::printf("%s picked\n", thePick);
				else
					::printf("%s%s picked\n", sSetupDirPath.Ptr,thePick);
			}

		}
		
		
		if ( !showMovieList )
		{
			int playError;
				
			if(!preflight)
			{	UpdateCurrentAndUpcomingFiles(broadcastParms, thePick, 	picker, insertPicker);
				
				/* if playlist is about to run out repopulate it */
				if  (	!::strcmp(broadcastParms->mPlayMode, "sequential_looped") )
				{	
					if(NULL == thePick &&!picker->mStopFlag)
					{	
						if (picker->GetNumMovies() == 0) 
							break;
						else
							continue;
					}
						
				}
			}

			if (thePick == NULL)
				break;
			
			++moviePlayCount;
			
			if (writeCurrentMovieFile && !preflight)
				sgLogger->LogMoviePlay( thePick, NULL, sStartMessage );

			if(!preflight)
				playError = fileBroadcaster.PlayMovie( thePick, broadcastParms->mCurrentFile );
			else
				playError = fileBroadcaster.PlayMovie( thePick, NULL );
			
			if (sQuitImmediate) 
			{	
				//printf("QUIT now sBroadcasterSession->Signal(BroadcasterSession::kTeardownEvent)\n");
				//if (!sBroadcasterSession->IsDone())
				//	sBroadcasterSession->TearDownNow();
				//goto bail;
				break;
			}
			
			if (sBroadcasterSession != NULL && sBroadcasterSession->GetReasonForDying() != BroadcasterSession::kDiedNormally)
			{	
				playError = ::EvalBroadcasterErr(QTFileBroadcaster::eNetworkConnectionFailed);
				sNumErrors++;
				goto bail;
			}
				
			if ( playError == 0 && sAnnounceBroadcast)
			{	
				int theErr = sBroadcasterSession->GetRequestStatus();
				if (200 != theErr)
				{	
					if (401 == theErr)
						playError = QTFileBroadcaster::eNetworkAuthorization;
					else if (500 == theErr)
						playError = QTFileBroadcaster::eNetworkNotSupported;
					else 
						playError = QTFileBroadcaster::eNetworkConnectionError;
						
					sNumErrors++;
					goto bail;
				}
			}
			
			if (playError)
			{	
				playError = ::EvalBroadcasterErr(playError);
				
				if (playError == QTFileBroadcaster::eNetworkConnectionError)
					goto bail;

				printf("  (file: %s err: %d %s)\n", thePick, playError,GetMovieFileErrString( playError ) );
				sNumWarnings++;
				numMovieErrors++;
			}
			else
			{
				int tracks = fileBroadcaster.GetMovieTrackCount() ;
				int mtracks = fileBroadcaster.GetMappedMovieTrackCount();

				if (tracks != mtracks)
				{	
					sNumWarnings++;
					numMovieErrors++;
					printf("- PlaylistBroadcaster: Warning, movie tracks do not match the SDP file.\n" );
					printf("  Movie: %s .\n", thePick );
					printf("  %i of %i hinted tracks will not broadcast.\n", tracks- mtracks, tracks );
				}
			}
			

			if ( !preflight )
				sgLogger->LogMoviePlay( thePick, GetMovieFileErrString( playError ),NULL );
		}
		

		delete [] thePick;
		thePick = NULL;
	} 
	
	remove(broadcastParms->mCurrentFile);
	remove(broadcastParms->mUpcomingFile);	

	if ( preflight )
	{
		

		char	str[256];	
		printf( " - "  );
		if (numMovieErrors == 1)
			strcpy(str, "PlaylistBroadcaster found one problem movie file.");
		else
			sprintf( str, "PlaylistBroadcaster found %li problem movie files." , numMovieErrors );
		printf( "%s\n", str );
		if (sgLogger != NULL) 
			sgLogger->LogInfo( str );
		
		if (numMovieErrors == moviePlayCount)
		{
			printf("There are no valid movies to play\n");
			sNumErrors++;
		}
	}
	

	if (NULL != sBroadcasterSession && !sBroadcasterSession->IsDone() )
	{	sErrorEvaluted = true;
	}
		
bail:

	Cleanup();

	delete picker;
	
	sBroadcastParms = NULL;
	delete broadcastParms;

	if ( !preflight )
	{
		if (sgLogger != NULL) 
		{	if (!sQuitImmediate)
				sgLogger->LogInfo( "PlaylistBroadcaster finished." );
			else
				sgLogger->LogInfo( "PlaylistBroadcaster stopped." );			
		}
		
		if (!sQuitImmediate) // test sQuitImmediate again 
			printf( "\nPlaylistBroadcaster broadcast finished.\n" ); 
		else
			printf( "\nPlaylistBroadcaster broadcast stopped.\n" ); 
	}
	else
	{
		if (sgLogger != NULL) 
		{	if (!sQuitImmediate)
				sgLogger->LogInfo( "PlaylistBroadcaster preflight finished." );
			else
				sgLogger->LogInfo( "PlaylistBroadcaster preflight stopped." );
		}
		
		if (!sQuitImmediate)  // test sQuitImmediate again
			printf( "\nPlaylistBroadcaster preflight finished.\n" ); 
		else
			printf( "\nPlaylistBroadcaster preflight stopped.\n" ); 

	}
	sgLogger = NULL; // protect the interrupt handler and just let it die don't delete because it is a task thread

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
#endif //__Win32__
	if (sBroadcasterSession != NULL && !sBroadcasterSession->IsDone())
	{	
		//printf("QUIT now sBroadcasterSession->TearDownNow();\n");
		sBroadcasterSession->TearDownNow();
		int count=0;
		while (count++ < 30 && !sBroadcasterSession->IsDone() )
		{	sBroadcasterSession->Run();
			OSThread::Sleep(100);
		}
		sBroadcasterSession = NULL;
	}
   
}

static void Cleanup()
{
    if (sCleanupDone == true)
        return;
    
    sCleanupDone = true;
    
    if (sPreflight)
    {
            printf("Warnings: %ld\n", sNumWarnings);
            printf("Errors: %ld\n", sNumErrors);
    }
    else
    {
            printf("Broadcast Warnings: %ld\n", sNumWarnings);
            printf("Broadcast Errors: %ld\n", sNumErrors);
    }
    
    RemoveFiles(sBroadcastParms);
        
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
	::printf("usage: PlaylistBroadcaster [-v] [-h] [-p] [-c] [-a] [-t] [-i destAddress] [-e filename] [-f] [-d] [-l] [-s broadcastNum] filename\n" );
#else
	::printf("usage: PlaylistBroadcaster [-v] [-h] [-p] [-c] [-a] [-t] [-i destAddress] [-e filename] [-f] filename\n" );
#endif

	::printf("       -v: Display version\n" );
	::printf("       -h: Display help\n" );
	::printf("       -p: Verify a broadcast description file and movie list.\n" );
	::printf("       -c: Show the current movie in the log file.\n" );
	::printf("       -a: Announce the broadcast to the server.\n" );
	::printf("       -t: Send the broadcast over TCP to the server.\n" );
	::printf("       -i: Specify the destination ip address. Over-rides config file value.\n" );
	::printf("       -e: Log errors to filename.\n" );
	::printf("       -f: Force a new SDP file to be created even if one already exists.\n" );
#ifndef __Win32__
	::printf("       -d: Run attached to the terminal.\n" );
	::printf("       -l: List running currently broadcasts.\n" );
	::printf("       -s: Stop a running broadcast.\n" );
#endif
	::printf("        filename: Broadcast description filename.\n" );


}



static void help()
{
	/*
		print PlaylistBroadcaster help info

	*/
	::version();
	
	::usage();
	
	
	::printf("\n\nSample broadcast description file: ");
	PLBroadcastDef(NULL,NULL,false);

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

				noPlayQueueLen = broadcastParms->mLimitPlayQueueLength;
				
				picker = new PlaylistPicker( 10, noPlayQueueLen );
				
			}
			else if ( !::strcmp( broadcastParms->mPlayMode, "sequential_looped" ) )
			{			
				picker = new PlaylistPicker(true);
				picker->mRemoveFlag = true;
			}
			else if ( !::strcmp( broadcastParms->mPlayMode, "sequential" ) )
			{			
				picker = new PlaylistPicker(false);
				picker->mRemoveFlag = true;
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

static int sErr = 0;
static int EvalBroadcasterErr(int err)
{
	int returnErr = err;
	if (sErr != 0) 
		return sErr;
	
	switch (err)
	{	case 412:
		{	printf("- Server Session Failed: The request was denied.");
			printf("\n");
			break;
		}
		case QTFileBroadcaster::eNoAvailableSockets :	
		case QTFileBroadcaster::eSDPFileNotFound 	:
		case QTFileBroadcaster::eSDPDestAddrInvalid :
		case QTFileBroadcaster::eSDPFileInvalid 	: 
		case QTFileBroadcaster::eSDPFileNoMedia 	: 
		case QTFileBroadcaster::eSDPFileNoPorts 	:
		case QTFileBroadcaster::eSDPFileInvalidPort :	
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
				case QTFileBroadcaster::eSDPFileNoMedia 	 : printf("The SDP file is missing media (m=) references."); 
				break; 
				case QTFileBroadcaster::eSDPFileNoPorts 	 : printf("The SDP file is missing server port information."); 
				break;
				case QTFileBroadcaster::eSDPFileInvalidPort  : printf("The SDP file contains an invalid port. Valid range is 5004 - 65530."); 
				break;

				default: printf("SDP set up error %d occured.",err);
				break;	
			
			};
			printf("\n");
		}
		break;
		
		case QTFileBroadcaster::eSDPFileInvalidTTL:
		case QTFileBroadcaster::eDescriptionInvalidDestPort:
		case QTFileBroadcaster::eSDPFileInvalidName: 
		case QTFileBroadcaster::eNetworkSDPFileNameInvalidBadPath:
		case QTFileBroadcaster::eNetworkSDPFileNameInvalidMissing:
		{	printf("- Description set up failed: ");
			switch( err )
			{
			
				case QTFileBroadcaster::eSDPFileInvalidTTL 	 : printf("The multicast_ttl value is incorrect. Valid range is 1 to 255."); 
				break;
				case QTFileBroadcaster::eDescriptionInvalidDestPort  : printf("The destination_base_port value is incorrect. Valid range is 5004 - 65530."); 
				break;
				case QTFileBroadcaster::eSDPFileInvalidName  : printf("The sdp_file name is missing or too long."); 
				break;
				case QTFileBroadcaster::eNetworkSDPFileNameInvalidBadPath: printf("The specified destination_sdp_file must a be relative file path in the movies directory."); 
				break;
				case QTFileBroadcaster::eNetworkSDPFileNameInvalidMissing: printf("The specified destination_sdp_file name is missing."); 
				break;
							
				default: printf("Description set up error %d occured.",err);
				break;	
			}
			printf("\n");
		}
		break;
		
		
		
		case QTFileBroadcaster::eMovieFileNotFound	 		:
		case QTFileBroadcaster::eMovieFileNoHintedTracks 	:
		case QTFileBroadcaster::eMovieFileNoSDPMatches 		:
		case QTFileBroadcaster::eMovieFileInvalid 			:
		case QTFileBroadcaster::eMovieFileInvalidName 		:
		{	printf("- Movie set up failed: ");
			switch( err )
			{	
				case QTFileBroadcaster::eMovieFileNotFound	 		: printf("Movie file not found."); 
				break;
				case QTFileBroadcaster::eMovieFileNoHintedTracks 	: printf("Movie file has no hinted tracks."); 
				break;
				case QTFileBroadcaster::eMovieFileNoSDPMatches 		: printf("Movie file does not match SDP."); 
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
		{	printf("- Internal Error: ");
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
		}
		break;
		
		case QTFileBroadcaster::eFailedBind:
		case QTFileBroadcaster::eNetworkConnectionError:
		case QTFileBroadcaster::eNetworkRequestError:
		case QTFileBroadcaster::eNetworkConnectionStopped:
		case QTFileBroadcaster::eNetworkAuthorization:
		case QTFileBroadcaster::eNetworkNotSupported:
		case QTFileBroadcaster::eNetworkConnectionFailed:
		{	sErrorEvaluted = true;
			printf("- Network Connection: ");
			switch( err )
			{	
				case QTFileBroadcaster::eFailedBind 	 : printf("A Socket failed trying to open and bind to a local port.\n."); 
				break;

				case QTFileBroadcaster::eNetworkConnectionError	 : printf("Failed to connect."); 
				break;
				
				case QTFileBroadcaster::eNetworkRequestError	 : printf("Server returned error."); 
				break;
			
				case QTFileBroadcaster::eNetworkConnectionStopped: printf("Connection stopped."); 
				break;

				case QTFileBroadcaster::eNetworkAuthorization: printf("Authorization failed."); 
				break;

				case QTFileBroadcaster::eNetworkNotSupported: printf("Connection not supported by server."); 
				break;

				case QTFileBroadcaster::eNetworkConnectionFailed	 : printf("Disconnected."); 
				break;

				default: printf("internal error %d occured.",err);
				break;	
			}
			printf("\n");
			returnErr = QTFileBroadcaster::eNetworkConnectionError;
		}
		break;
			
		default:
			
		break;
	}
	
	sErr = returnErr;

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

static int PreflightClientBufferDelay( const char * delay, Float32 *ioDelay)
{
	
	int 	numPreflightErrors = 0;
	
	if ( NULL == delay|| 0 == delay[0] )
		numPreflightErrors++;
	else 
	{ 	Float32 delayValue = 0.0;
	  	::sscanf(delay, "%f", &delayValue);
	  	if (delayValue < 0.0)
			numPreflightErrors++;
		if (ioDelay != NULL)
		 	*ioDelay = delayValue;
	}		
	
	if (numPreflightErrors > 0)
		::printf("- PlaylistBroadcaster: The client_buffer_delay parameter is invalid.\n" );

	return numPreflightErrors;
}

static int PreflightDestSDPFileAccess( const char * sdpPath )
{
	int 	numPreflightErrors = 0;
	
	if ( NULL == sdpPath || 0==sdpPath[0] || 0 == ::strcmp(sdpPath, "no_name") )
	{	::printf("- PlaylistBroadcaster: The destination_sdp_file parameter is missing from the Broadcaster description file.\n" );
		numPreflightErrors++;
	}

	return numPreflightErrors;

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


	if ( !destAddress || (SocketUtils::ConvertStringToAddr(destAddress) == INADDR_NONE) )
	{	::printf("- PlaylistBroadcaster: Error, desitination_ip_address parameter missing or incorrect in the Broadcaster description file.\n" );
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


static bool DoSDPGen( PLBroadcastDef *broadcastParms, bool preflight, bool overWriteSDP, bool generateNewSDP, int* numErrorsPtr, char* refMovie)
{
	int numSDPSetupErrors = 0;
	bool sdpFileCreated = false;
	
	if (sAnnounceBroadcast)
	{	numSDPSetupErrors += PreflightDestSDPFileAccess( broadcastParms->mDestSDPFile );
		Assert(broadcastParms->mBasePort != NULL);
		Assert(broadcastParms->mBasePort[0]!=0);
		broadcastParms->mBasePort[0] = '0';//set to dynamic. ignore any defined.
		broadcastParms->mBasePort[1] = 0;
	}
	
	if (broadcastParms->mSDPReferenceMovie != NULL)
		refMovie = broadcastParms->mSDPReferenceMovie;
		
	printf("RefMovie = %s\n", refMovie);
	numSDPSetupErrors += PreflightSDPFileAccess( broadcastParms->mSDPFile );
	
	
	numSDPSetupErrors += PreflightReferenceMovieAccess( refMovie );
	
	
	numSDPSetupErrors += PreflightDestinationAddress( broadcastParms->mDestAddress );


	numSDPSetupErrors += PreflightBasePort( broadcastParms->mBasePort );

	Float32	bufferDelay = 0.0;	
	numSDPSetupErrors += PreflightClientBufferDelay(  broadcastParms->mClientBufferDelay,&bufferDelay );
	
	if ( numSDPSetupErrors == 0 )
	{
		SDPGen*	sdpGen = new SDPGen;
		
		int 	sdpResult = -1;
		
		Assert( sdpGen );
		
		if ( sdpGen )
		{	
			sdpGen->SetClientBufferDelay(bufferDelay); // sdp "a=x-bufferdelay: value" 
			sdpGen->KeepSDPTracks(false); // set this to keep a=control track ids if we are going to ANNOUNCE the sdp to the server.
			sdpGen->AddIndexTracks(true); // set this if KeepTracks is false and to ANNOUNCE the sdp to the server.
			sdpResult = sdpGen->Run( refMovie, broadcastParms->mSDPFile, 
						  broadcastParms->mBasePort, broadcastParms->mDestAddress, 
						  sSDPBuffer, maxSDPbuffSize, 
						  overWriteSDP,  generateNewSDP,
						  broadcastParms->mStartTime,
						  broadcastParms->mEndTime,
						  broadcastParms->mIsDynamic
						  );
		
			sdpFileCreated =  sdpGen->fSDPFileCreated;
			
			if ( sdpGen->fSDPFileCreated && !preflight)
				::printf( "- PlaylistBroadcaster: Created SDP file.\n  (path: %s)\n", broadcastParms->mSDPFile);
		}
		
		
		if ( sdpResult )
		{
			::printf( "- SDP generation failed (error: %li).\n", (long)sdpResult );
                        *numErrorsPtr++;
                        return sdpFileCreated;
		}
		
		if ( sdpGen )
			delete sdpGen;
		
		sdpGen = NULL;
	}
	else
	{	::printf( "- PlaylistBroadcaster: Too many SDP set up errors, exiting.\n" );
		*numErrorsPtr += numSDPSetupErrors;
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

static bool PreFlightSetupFile( const char * bcastSetupFilePath )
{
        bool success = true;
        
	// now complain!
	if ( !bcastSetupFilePath )
	{	::printf("- PlaylistBroadcaster: A broadcast description file is required.\n" );
		::usage();
		success = false;
	}
        else
	{	int		accessError;
	
		accessError = MyAccess(bcastSetupFilePath, R_OK  );
		
		switch( accessError )
		{
		
			case 0:
				break;
				
			case ENOENT:	
				::printf("- PlaylistBroadcaster: The broadcast description file is missing.\n  (path: %s, errno: %i).\n", bcastSetupFilePath, accessError);
				success = false;
				break;
			
			case EACCES:
				::printf("- PlaylistBroadcaster: Permission denied to access the broadcast description file.\n  Read access required.\n  (path: %s, errno: %i).\n", bcastSetupFilePath, accessError);
				success = false;
				break;
				
			default:
				::printf("- PlaylistBroadcaster: Unable to access the broadcast description file.\n  (path: %s, errno: %i).\n", bcastSetupFilePath, accessError);
				success = false;
				break;
				
		}
	
	}

        return success;
}

static bool AddOurPIDToTracker( const char* bcastSetupFilePath )
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
        return false;
	}
#endif
    return true; // return true if pid was successfully added to the tracker
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
/* changed by emil@popwire.com (see relaod.txt for info) */
bool FileCreateAndCheckAccess(char *theFileName){
	FILE *theFile;


#ifndef __Win32__
	if(access(theFileName, F_OK)){
		/* file does not exist  - create and set rights */
		theFile = ::fopen(theFileName, "w+");
		if(theFile){
			/* make sure everybody has r/w access to file */
			(void)::chmod(theFileName, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH);
			(void)::fclose(theFile);
		}else return 1;
	}else{
		/* file exists - check rights */
		if(access(theFileName, R_OK|W_OK))return 2;
	}

#else

	theFile = ::fopen(theFileName,"a");

	if (theFile) ::fclose(theFile);

#endif


	return 0;
}

static void PrintPlaylistElement(PLDoubleLinkedListNode<SimplePlayListElement> *node,void *file)
{	
	sElementCount ++;
	if (sElementCount <= sMaxUpcomingListSize) 	
	{	char* thePick = node->fElement->mElementName;
		if (sSetupDirPath.EqualIgnoreCase(thePick, sSetupDirPath.Len) || '\\' == thePick[0] || '/' == thePick[0])
			::fprintf((FILE*)file,"%s\n", thePick);
		else
			::fprintf((FILE*)file,"%s%s\n", sSetupDirPath.Ptr,thePick);
	}
}

static void ShowPlaylistElements(PlaylistPicker *picker,FILE *file)
{
	if (sElementCount > sMaxUpcomingListSize) 	
		return;
		
	UInt32	x;
	for (x= 0;x<picker->GetNumBuckets();x++)
	{	
		picker->GetBucket(x)->ForEach(PrintPlaylistElement,file);
	}
}
/* ***************************************************** */

static void RegisterEventHandlers()
{
#ifdef  __Win32__
	SetConsoleCtrlHandler( (PHANDLER_ROUTINE) SignalEventHandler, true);
	return;
#endif

#ifndef __Win32__	

struct sigaction act;
	
#if defined(sun) || defined(i386) || defined(__powerpc__)
	sigemptyset(&act.sa_mask);
	act.sa_flags = 0;
	act.sa_handler = (void(*)(int))&SignalEventHandler;
#else
	act.sa_mask = 0;
	act.sa_flags = 0;
	act.sa_handler = (void(*)(...))&SignalEventHandler;
#endif

  	if ( ::signal(SIGTERM, SIG_IGN) != SIG_IGN) 
    {	// from kill...
		if ( ::sigaction(SIGTERM, &act, NULL) != 0 )
     	{	::printf( "- PlaylistBroadcaster: System error (%i).\n", (int)SIG_ERR );
     	}
 	}

    if ( ::signal(SIGINT, SIG_IGN) != SIG_IGN) 
    {	// ^C signal
		if ( ::sigaction(SIGINT, &act, NULL)  != 0 )
    	{	::printf( "- PlaylistBroadcaster: System error (%i).\n", (int)SIG_ERR );
     	}
     	
    }
    
    if ( ::signal(SIGPIPE, SIG_IGN) != SIG_IGN) 
    {	// broken pipe probably from a failed RTSP session (the server went down?)
		if ( ::sigaction(SIGPIPE, &act, NULL)   != 0 )
     	{	::printf( "- PlaylistBroadcaster: System error (%i).\n", (int)SIG_ERR );
     	}
     	
    }
 
    if ( ::signal(SIGHUP, SIG_IGN) != SIG_IGN) 
    {	// broken pipe probably from a failed RTSP session (the server went down?)
		if ( ::sigaction(SIGHUP, &act, NULL)  != 0)
    	{	::printf( "- PlaylistBroadcaster: System error (%i).\n", (int)SIG_ERR );
     	}
     	
    }
    
    
#endif


}

/* ========================================================================
 * Signal and error handler.
 */
static void RemoveFiles(PLBroadcastDef*	broadcastParms)
{
	if (broadcastParms != NULL)
	{
		if (broadcastParms->mPIDFile != NULL)
            		remove(broadcastParms->mPIDFile);
		if (broadcastParms->mStopFile != NULL)
			remove(broadcastParms->mStopFile);
		if (broadcastParms->mReplaceFile != NULL)
			remove(broadcastParms->mReplaceFile);
		if (broadcastParms->mInsertFile != NULL)
			remove(broadcastParms->mInsertFile);
		if (broadcastParms->mCurrentFile != NULL)
			remove(broadcastParms->mCurrentFile);
		if (broadcastParms->mUpcomingFile != NULL)
			remove(broadcastParms->mUpcomingFile);	
	}

}
/* ========================================================================
 * Signal and error handler.
 */
static void SignalEventHandler( int signalID )
{	

#ifdef __Win32__
        if ( (signalID != SIGINT) && (signalID != SIGTERM) )
#else
        if (signalID == SIGPIPE)
#endif
            sNumErrors++;
        
        // evaluate the error
  	if (sRTSPClientError != 0 && !sErrorEvaluted)
  	{	
		EvalBroadcasterErr(sRTSPClientError);
  		sErrorEvaluted = true;
   	}
  	else if (sBroadcasterSession != NULL && !sErrorEvaluted)
	{
#ifndef __Win32__
		if ( (signalID == SIGINT) || (signalID == SIGTERM) )
		{	EvalBroadcasterErr(QTFileBroadcaster::eNetworkConnectionStopped);
		}
		else
		{	EvalBroadcasterErr(QTFileBroadcaster::eNetworkConnectionFailed);
		}
#else		
		EvalBroadcasterErr(QTFileBroadcaster::eNetworkConnectionStopped);
#endif
		sErrorEvaluted = true;
	}

        // do the cleanup - write warning and error messages and remove files
        Cleanup();
        
#ifndef __Win32__
	if (sBroadcasterSession != NULL)
	{	

		if ( (signalID == SIGINT) || (signalID == SIGTERM) )
			sQuitImmediate = true; // just in case we get called again
			
		return;	//we need to let the broadcaster session teardown and clean up
	}

	if ( sgTrackingSucceeded )
	{	
		// give tracker scope so that it really does it's
		// thing before "exit" is called.
		BCasterTracker	tracker( sgTrackerFilePath );
	
		tracker.RemoveByProcessID( getpid() );
		tracker.Save();
	}
	
#endif	

	if (!sQuitImmediate) // do once
 	{	
		sQuitImmediate = true; // just in case we get called again

#ifdef __Win32__

 		if (sBroadcasterSession && !sBroadcasterSession->IsDone())
		{	sBroadcasterSession->TearDownNow();
			OSThread::Sleep(1000);
		}
		if (NULL == sBroadcasterSession)
			printf("\n"); // make sure the message was printed before quitting.
		
#endif		
		return;
		
	}
 	::exit(-1);
}

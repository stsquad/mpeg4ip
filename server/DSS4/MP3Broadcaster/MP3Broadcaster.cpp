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

#include "MP3Broadcaster.h"
#include "MP3MetaInfoUpdater.h"
#include "StringTranslator.h"

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/stat.h>
#ifndef __Win32__
	#include <unistd.h>
#endif




#ifndef __Win32__
#ifndef __MACOS__
        #include <netdb.h>
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

#include "OSHeaders.h"
#include "OS.h"
#include "OSMemory.h"
#include "Task.h"
#include "TimeoutTask.h"
#include "OSArrayObjectDeleter.h"
#include "ConfParser.h"
#include "MP3FileBroadcaster.h"
// must now inlcude this from the project level using the -i switch in the compiler
#ifndef __MacOSXServer__
#ifndef __MacOSX__
	#include "revision.h"
#endif
#endif

MP3Broadcaster* MP3Broadcaster::sBroadcaster = NULL;

//#include "MyAssert.h"

MP3Broadcaster::MP3Broadcaster(char* ipaddr, int port, char* config, char* playList, char* workingDir) :
	mValid(true),
	mPort(8000),
	mBitRate(0),
	mFrequency(0),
	mUpcomingSongsListSize(7),
	mRecentSongsListSize(0),
	mLogging(0),
	mShowCurrent(true),
	mShowUpcoming(true),
        mNumErrors(0),
        mNumWarnings(0),
        mPreflight(false),
        mCleanupDone(false),
	mTempPicker(NULL),
	mSocket(NULL, 0),
	mLog(NULL)
{
	sBroadcaster = this;
	
	strcpy(mIPAddr, "128.0.0.1");
	strcpy(mPlayListPath, "/etc/streaming/mp3playlist.ply");
	strcpy(mWorkingDirPath, "/etc/streaming/");
	strcpy(mPlayMode, "sequential");
	strcpy(mMountPoint, "/");
	strcpy(mGenre, "Pop");
	strcpy(mURL, "");
	strcpy(mPIDFile, "");
	
	int err = ::ParseConfigFile( false, config, ConfigSetter, this );
	if (err)
	{
		mValid = false;
		return;
	}
	
	if (ipaddr != NULL)
	{
		// override config file
		// size limit for any IP addr is 255
		strncpy(mIPAddr, ipaddr, 255);
	}
	
	if (port != 0)
	{
		// override config file
		mPort = port;
	}
	
	if (playList)
	{
		// override config file
		// size limit for any playlist string is PATH_MAX - 1
		strncpy(mPlayListPath, playList, PATH_MAX-1);
	}
	
	if (workingDir)
	{
		// override config file
		// size limit for any working path is PATH_MAX - extension - 1
		strncpy(mWorkingDirPath, playList, PATH_MAX-12);
	}
	
	CreateWorkingFilePath(".current", mCurrentFile);
	CreateWorkingFilePath(".upcoming", mUpcomingFile);
	CreateWorkingFilePath(".replacelist", mReplaceFile);
	CreateWorkingFilePath(".stoplist", mStopFile);
	CreateWorkingFilePath(".insertlist", mInsertFile);
	CreateWorkingFilePath(".log", mLogFile);
}

Bool16 MP3Broadcaster::ConfigSetter( const char* paramName, const char* paramValue[], void* userData )
{
	// return true if set fails
	MP3Broadcaster* thisPtr = (MP3Broadcaster*)userData;

	if (!::strcmp( "destination_ip_address", paramName) )
	{	
		if (strlen(paramValue[0]) >= sizeof(thisPtr->mIPAddr))
			return true;
		
		strcpy(thisPtr->mIPAddr, paramValue[0]);
		return false;
	}
	else if (!::strcmp( "destination_base_port", paramName) )
	{
		thisPtr->mPort = atoi(paramValue[0]);

		return false;
	}
	else if (!::strcmp( "max_upcoming_list_size", paramName) )
	{
		if ( ::atoi( paramValue[0] )  < 0 )
			return true;
			
		thisPtr->mUpcomingSongsListSize = ::atoi( paramValue[0] );
		return false;
	}
	else if (!::strcmp( "play_mode", paramName) )
	{
		if (strlen(paramValue[0]) >= sizeof(thisPtr->mPlayMode))
			return true;
		
		strcpy(thisPtr->mPlayMode, paramValue[0]);
		return false;
	}	
	else if (!::strcmp( "recent_songs_list_size", paramName) )
	{
		if ( ::atoi( paramValue[0] )  < 0 )
			return true;
			
		thisPtr->mRecentSongsListSize = ::atoi( paramValue[0] );
		return false;
	}
	else if (!::strcmp( "playlist_file", paramName) )
	{
		if (strlen(paramValue[0]) >= sizeof(thisPtr->mPlayListPath))
			return true;
		
		strcpy(thisPtr->mPlayListPath, paramValue[0]);
		return false;
	}
	else if (!::strcmp( "working_dir", paramName) )
	{
		if (strlen(paramValue[0]) >= sizeof(thisPtr->mWorkingDirPath))
			return true;
		
		strcpy(thisPtr->mWorkingDirPath, paramValue[0]);
		return false;
	}
	else if (!::strcmp( "logging", paramName) )
	{
		return SetEnabled(paramValue[0], &thisPtr->mLogging);
	}
	else if (!::strcmp( "show_current", paramName) )
	{
		return SetEnabled(paramValue[0], &thisPtr->mShowCurrent);
	}
	else if (!::strcmp( "show_upcoming", paramName) )
	{
		return SetEnabled(paramValue[0], &thisPtr->mShowUpcoming);
	}
	else if (!::strcmp( "broadcast_name", paramName) )
	{
		if (strlen(paramValue[0]) >= sizeof(thisPtr->mName))
			return true;
		
		strcpy(thisPtr->mName, paramValue[0]);
		return false;
	}
	else if (!::strcmp( "broadcast_password", paramName) )
	{
		if (strlen(paramValue[0]) >= sizeof(thisPtr->mPassword))
			return true;
		
		strcpy(thisPtr->mPassword, paramValue[0]);
		return false;
	}
	else if (!::strcmp( "broadcast_genre", paramName) )
	{
		if (strlen(paramValue[0]) >= sizeof(thisPtr->mGenre))
			return true;
		
		strcpy(thisPtr->mGenre, paramValue[0]);
		return false;
	}
	else if (!::strcmp( "broadcast_url", paramName) )
	{
		if (strlen(paramValue[0]) >= sizeof(thisPtr->mURL))
			return true;
		
		strcpy(thisPtr->mURL, paramValue[0]);
		return false;
	}
	else if (!::strcmp( "broadcast_bitrate", paramName) )
	{
		if ( ::atoi( paramValue[0] )  < 0 )
			return true;
			
		thisPtr->mBitRate = ::atoi( paramValue[0] );
		return false;
	}
	else if (!::strcmp( "broadcast_sample_rate", paramName) )
	{
		if ( ::atoi( paramValue[0] )  < -1 )
			return true;
			
		thisPtr->mFrequency = ::atoi( paramValue[0] );
		return false;
	}
	else if (!::strcmp( "broadcast_mount_point", paramName) )
	{
		if (strlen(paramValue[0]) >= sizeof(thisPtr->mMountPoint))
			return true;
		
		strcpy(thisPtr->mMountPoint, paramValue[0]);
		return false;
	}
	else if (!::strcmp( "pid_file", paramName) )
	{
		if (strlen(paramValue[0]) >= sizeof(thisPtr->mPIDFile))
			return true;
		
		strcpy(thisPtr->mPIDFile, paramValue[0]);
		return false;
	}
	
	return true;
}

Bool16 MP3Broadcaster::SetEnabled( const char* value, Bool16* field)
{
	if ( ::strcmp( "enabled", value) && ::strcmp( "disabled", value) )
		return true;
		
	*field = !strcmp( "enabled", value );
		
	return false;
}

void MP3Broadcaster::CreateWorkingFilePath(char* extension, char* result)
{
	if (strlen(mWorkingDirPath) + strlen(extension) >= PATH_MAX)
		result[0] = 0;
	else
	{
		strcpy(result, mWorkingDirPath);
		strcat(result, extension);
	}
}

void MP3Broadcaster::CreateCurrentAndUpcomingFiles()
{
	if (mShowCurrent) 
	{	
		if(FileCreateAndCheckAccess(mCurrentFile))
		{ 	/* error */
		  	mLog->LogInfo( "MP3Broadcaster Error: Failed to create current broadcast file" );
		}
	}


	if (mShowUpcoming) 
	{		
		if(FileCreateAndCheckAccess(mUpcomingFile))
		{ 	/* error */
		  	mLog->LogInfo( "MP3Broadcaster Error: Failed to create upcoming broadcast file" );
		}
	}
}


void MP3Broadcaster::UpdateCurrentAndUpcomingFiles(char *thePick, PlaylistPicker *picker,PlaylistPicker *insertPicker)
{
	if ( 	(NULL == thePick)
		||  (NULL == picker)
		||  (NULL == insertPicker)
		) return;
		
	// save currently playing song to .current file 
	if (mShowCurrent) 
	{	FILE *currentFile = fopen(mCurrentFile, "w");
		if(currentFile)
		{
			::fprintf(currentFile,"u=%s\n",thePick);

			fclose(currentFile);
		}	
	}
						
	/* if .stoplist file exists - prepare to stop broadcast */
	if(!access(mStopFile, R_OK))
	{
		picker->CleanList();
		PopulatePickerFromFile(picker, mStopFile, "", NULL);

		mTempPicker->CleanList();

		remove(mStopFile);
		picker->mStopFlag = true;
	}

	/* if .replacelist file exists - replace current playlist */
	if(!access(mReplaceFile, R_OK))
	{
		picker->CleanList();	 
		PopulatePickerFromFile(picker, mReplaceFile, "", NULL);
		
		mTempPicker->CleanList();
		
		remove(mReplaceFile);
		picker->mStopFlag = false;
	}

	/* if .insertlist file exists - insert into current playlist */
	if(!access(mInsertFile, R_OK))
	{
		insertPicker->CleanList();
		mTempPicker->CleanList();

		PopulatePickerFromFile(insertPicker, mInsertFile, "", NULL);
		remove(mInsertFile);
		picker->mStopFlag = false;
	}


				// write upcoming playlist to .upcoming file 
	if (mShowUpcoming) 
	{
		FILE *upcomingFile = fopen(mUpcomingFile, "w");
		if(upcomingFile)
		{
			mElementCount = 0;
		
			if (!::strcmp(mPlayMode, "weighted_random")) 
				fprintf(upcomingFile,"#random play - upcoming list not supported\n");
			else
			{	fprintf(upcomingFile,"*PLAY-LIST*\n");
				ShowPlaylistElements(insertPicker,upcomingFile);
				ShowPlaylistElements(picker,upcomingFile);
				if (	picker->GetNumMovies() == 0 
						&& !picker->mStopFlag 
						&& 0 != ::strcmp(mPlayMode, "sequential") 
					)
				{	picker->CleanList();
					PopulatePickerFromFile(picker,mPlayListPath,"",NULL);
					ShowPlaylistElements(picker,upcomingFile);
					mTempPicker->CleanList();
					PopulatePickerFromFile(mTempPicker,mPlayListPath,"",NULL);
				}
				
				if 	(	mElementCount <= mUpcomingSongsListSize 
						&& 0 == ::strcmp(mPlayMode, "sequential_looped")
					)
				{	if (mTempPicker->GetNumMovies() == 0)
					{	mTempPicker->CleanList();
						PopulatePickerFromFile(mTempPicker,mPlayListPath,"",NULL);
					}
					while (mElementCount <= mUpcomingSongsListSize )
						ShowPlaylistElements(mTempPicker,upcomingFile);
				}
			}
			fclose(upcomingFile);
		}	
	}
	else
	{	
		if (	picker->GetNumMovies() == 0 
				&& !picker->mStopFlag 
				&& ::strcmp(mPlayMode, "sequential") 
			)
		{	picker->CleanList();
			PopulatePickerFromFile(picker,mPlayListPath,"",NULL);
		}		
	}
}

void MP3Broadcaster::PrintPlaylistElement(PLDoubleLinkedListNode<SimplePlayListElement> *node,void *file)
{	
	sBroadcaster->mElementCount ++;
	if (sBroadcaster->mElementCount <= sBroadcaster->mUpcomingSongsListSize) 	
	{	
		char* thePick = node->fElement->mElementName;
		::fprintf((FILE*)file,"%s\n", thePick);
	}
}

void MP3Broadcaster::ShowPlaylistElements(PlaylistPicker *picker,FILE *file)
{
	if (mElementCount > mUpcomingSongsListSize) 	
		return;
		
	UInt32	x;
	for (x= 0;x<picker->GetNumBuckets();x++)
	{	
		picker->GetBucket(x)->ForEach(PrintPlaylistElement,file);
	}
}

void MP3Broadcaster::PreFlightOrBroadcast( bool preflight, bool daemonize, bool showMovieList, bool currentMovie, bool checkMP3s, const char* errorlog)
{
	PlaylistPicker* 	picker = NULL;
	PlaylistPicker* 	insertPicker = NULL;
	MP3FileBroadcaster	fileBroadcaster(&mSocket, mBitRate, mFrequency);
	MP3MetaInfoUpdater* metaInfoUpdater = NULL;
	
	long				moviePlayCount;
	char*				thePick = NULL;
	long				numMovieErrors;
	int err;
        
        mPreflight = preflight;
        
	if ( preflight )
		ShowSetupParams();

	if (preflight)
		picker = new PlaylistPicker(false);				// make sequential picker, no looping
	else
	{	
		picker = MakePickerFromConfig(); // make  picker according to parms
		mTempPicker =  new PlaylistPicker(false);
		insertPicker = new PlaylistPicker(false);
		insertPicker->mRemoveFlag = true;
	}
	
	// initial call uses empty string for path, NULL for loop detection list
	(void)PopulatePickerFromFile( picker, mPlayListPath, "", NULL );
	
	if ( preflight )
	{
		if ( picker->mNumToPickFrom == 1 )
			::printf( "\nThere is one movie in the Playlist.\n\n" );
		else
			::printf( "\nThere are (%li) movies in the Playlist.\n\n", (long) picker->mNumToPickFrom );
	}	
	
	if ( picker->mNumToPickFrom == 0 )
	{	
		printf( "- MP3Broadcaster setup failed: There are no movies to play.\n" );
		mNumErrors++;
		goto bail;
	}
	

	// check that we have enough movies to cover the recent movies list.
	if ( preflight )
	{
		if (  !strcmp( mPlayMode, "weighted_random" ) ) // this implies "random" play
		{	
			if ( mRecentSongsListSize >=  picker->mNumToPickFrom )
			{
				mRecentSongsListSize = picker->mNumToPickFrom - 1;
				printf("- PlaylistBroadcaster Warning:\n  The recent_movies_list_size setting is greater than \n");
				printf("  or equal to the number of movies in the playlist.\n" );
                                mNumWarnings++;
			}
		}
	}
	
	// create the log file
	mLog = new MP3BroadcasterLog( mWorkingDirPath,  mLogFile, mLogging );
	
//	if ( !PreflightTrackerFileAccess( R_OK | W_OK ) )
//		goto bail;

	if (!preflight)
	{
		err = ConnectToServer();
		if (err)
		{       
                        if (err == EACCES)
                            ::printf("- MP3Broadcaster: Disconnected from Server. Bad password or mount point\n  Exiting.\n" );
                        else
                            ::printf("- MP3Broadcaster: Couldn't connect to server\n  Exiting.\n" );
			mNumErrors++;
			goto bail;
		}
	}
	
	//Unless the Debug command line option is set, daemonize the process at this point
	if (daemonize)
	{	
#ifndef __Win32__
		::printf("- MP3Broadcaster: Started in background.\n");
		
		// keep the same working directory..
		if (::daemon( 1, 0 ) != 0)
		{
			::printf("- MP3Broadcaster:  System error (%i).\n", errno);
			mNumErrors++;
			goto bail;
		}

#endif		
	}
	
        if (daemonize && (errorlog != NULL))
	{	
                freopen(errorlog, "a", stdout);
		::setvbuf(stdout, (char *)NULL, _IONBF, 0);
	}
        
        if (!preflight)
        {
                metaInfoUpdater = new MP3MetaInfoUpdater(mPassword, mMountPoint, mSocket.GetRemoteAddr(), mPort);
                metaInfoUpdater->Start();
                fileBroadcaster.SetInfoUpdater(metaInfoUpdater);
	}
        
		// ^ daemon must be called before we Open the log and tracker since we'll
	// get a new pid, our files close,  ( does SIGTERM get sent? )
	
	if (( mLog ) && ( mLogging ))
		mLog->EnableLog( false ); // don't append ".log" to name for PLB
	
	if ( mLogging && !mLog->IsLogEnabled() )
	{
		if (  mLog->LogDirName() && *mLog->LogDirName() )
			::printf("- MP3Broadcaster: The log file failed to open.\n  ( path: %s/%s )\n  Exiting.\n", mLog->LogDirName(), mLog->LogFileName() );
		else
			::printf("- MP3Broadcaster: The log file failed to open.\n  ( path: %s )\n  Exiting.\n", mLog->LogFileName() );
		
		mNumErrors++;
		goto bail;
	}
	
	
	if (mPIDFile[0] != 0)
	{
		if(!FileCreateAndCheckAccess(mPIDFile))
		{
			FILE *pidFile = fopen(mPIDFile, "w");
			if(pidFile)
			{
				::fprintf(pidFile,"%d\n",getpid());
				fclose(pidFile);
			}	
		}
	}
	
//	AddOurPIDToTracker( bcastSetupFilePath ); // <-- exits on failure

	if ( !preflight || showMovieList )
		mLog->LogInfo( "MP3Broadcaster started." );
	else
		mLog->LogInfo( "MP3Broadcaster preflight started." );

	if(!preflight)
	{	
		CreateCurrentAndUpcomingFiles();
		SendXAudioCastHeaders();
	}
        
	if (!preflight)
	{
		// check the frequency of the first song
		fileBroadcaster.PlaySong( picker->GetFirstFile(), NULL, true, true );
	}
	
	moviePlayCount = 0;
	numMovieErrors = 0;
			
	while (true)
	{	
		if (NULL != insertPicker)
			thePick = insertPicker->PickOne(); 
			
		if (NULL == thePick && (NULL != picker))
			thePick = picker->PickOne();
		
		if ( (thePick != NULL) && (!preflight || showMovieList) )
		{
			// display the picks in debug mode, but not preflight
			printf( "[%li] %s picked\n", moviePlayCount, thePick );
		}
		
		
		if ( !showMovieList )
		{
			int playError;
				
			if(!preflight)
			{	UpdateCurrentAndUpcomingFiles(thePick, 	picker, insertPicker);
				
				/* if playlist is about to run out repopulate it */
				if  (	!::strcmp(mPlayMode, "sequential_looped") )
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
			
			if (currentMovie && !preflight)
			{	
				mLog->LogMoviePlay( thePick, NULL, "PLAY" );
			}

			if(!preflight)
			{
				playError = fileBroadcaster.PlaySong( thePick, mCurrentFile );
			}
			else
			{
				playError = fileBroadcaster.PlaySong( thePick, NULL, preflight, !checkMP3s );
			}
			
			if (playError == MP3FileBroadcaster::kConnectionError)
			{
				// do something
				mNumErrors++;
				goto bail;
			}
			
			if ( !preflight )
				mLog->LogMoviePlay( thePick, MapErrorToString(playError), NULL );
			else if (playError != 0)
			{
				::printf("File %s : ", thePick);
				MapErrorToString(playError);
				numMovieErrors++;
				mNumWarnings++;
			}
		}
		

		delete [] thePick;
		thePick = NULL;
	} 
	
	remove(mCurrentFile);
	remove(mUpcomingFile);	

	if ( preflight )
	{
		char	str[256];	
		printf( " - "  );
		if (numMovieErrors == 1)
			strcpy(str, "MP3Broadcaster found one problem MP3 file.");
		else
			sprintf( str, "MP3Broadcaster found %li problem MP3 files." , numMovieErrors );
		printf( "%s\n", str );
		if (mLog) mLog->LogInfo( str );
		
		if (numMovieErrors == moviePlayCount)
		{
			printf("There are no valid MP3s to play\n");
			mNumErrors++;
		}
	}
	
bail:

	delete picker;

	if (metaInfoUpdater)
		delete metaInfoUpdater;
        
        Cleanup();

#ifndef __Win32__
/*
	if ( sgTrackingSucceeded )
	{
		// remove ourselves from the tracker
		// this is the "normal" remove, also signal handlers
		// may remove us.
		
		BCasterTracker	tracker( sgTrackerFilePath );
		
		tracker.RemoveByProcessID( getpid() );
		tracker.Save();
	}
*/
#endif
}

PlaylistPicker* MP3Broadcaster::MakePickerFromConfig()
{
	// construct a PlaylistPicker object using options set
	
	PlaylistPicker *picker = NULL;
	
	if ( !::strcmp( mPlayMode, "weighted_random" ) )
	{
		picker = new PlaylistPicker( 10, mRecentSongsListSize );
		
	}
	else if ( !::strcmp( mPlayMode, "sequential_looped" ) )
	{			
		picker = new PlaylistPicker(true);
		picker->mRemoveFlag = true;
	}
	else if ( !::strcmp( mPlayMode, "sequential" ) )
	{			
		picker = new PlaylistPicker(false);
		picker->mRemoveFlag = true;
	}
	
	return picker;
}

bool MP3Broadcaster::FileCreateAndCheckAccess(char *theFileName)
{
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

int MP3Broadcaster::ConnectToServer()
{
	UInt32 addr;
	addr = SocketUtils::ConvertStringToAddr(mIPAddr);
	if (addr == INADDR_NONE)
	{
		struct hostent* theHostent = ::gethostbyname(mIPAddr);		
		if (theHostent != NULL)
			addr = ntohl(*(UInt32*)(theHostent->h_addr_list[0]));
		else
			::printf("Couldn't resolve address %s\n", mIPAddr);
	}
		
	OS_Error err = mSocket.Open();
	err = mSocket.Connect(addr, mPort);
        
	if (err == 0)
	{
		char buffer1[512];
		char buffer2[512];
		UInt32 len;

		StringTranslator::EncodeURL(mMountPoint, strlen(mMountPoint) + 1, buffer1, sizeof(buffer1));
		if (strlen(buffer1) + strlen(mPassword) + 12 <= 512)
		{
			if (buffer1[0] == '/')
				::sprintf(buffer2, "SOURCE %s %s\n\n", mPassword, buffer1);
			else
				::sprintf(buffer2, "SOURCE %s /%s\n\n", mPassword, buffer1);
			mSocket.Send(buffer2, strlen(buffer2), &len);
		}
		else return -1;
                
                char buffer3[3];
                len = 0;
		mSocket.Read(buffer3, 2, &len);
                buffer3[2] = '\0';
                if (::strcmp(buffer3, "OK") != 0)
                   err = EACCES; 
        }
        
	return err;
}

int MP3Broadcaster::SendXAudioCastHeaders()
{
	char buffer[1024];
	char temp[256];
	UInt32 len;
	
	sprintf(buffer, "x-audiocast-name:%s\n", mName);
	sprintf(temp, "x-audiocast-genre:%s\n", mGenre);
	strcat(buffer, temp);
	sprintf(temp, "x-audiocast-public:%s\n", "0");
	strcat(buffer, temp);
	sprintf(temp, "x-audiocast-description:%s\n", "");
	strcat(buffer, temp);
        
	mSocket.Send(buffer, strlen(buffer), &len);
	
	return 0;
}

void MP3Broadcaster::ShowSetupParams()
{
	::printf( "\n" );
	::printf( "Configuration Settings\n" );
	::printf( "----------------------------\n" );
	::printf( "Destination address %s:%d\n", mIPAddr, mPort );
	::printf( "MP3 bitrate %d\n", mBitRate );
	::printf( "play_mode  %s\n", mPlayMode );
	::printf( "recent_movies_list_size  %d\n", mRecentSongsListSize );
	::printf( "playlist_file  %s\n", mPlayListPath );
	::printf( "working_dir  %s\n", mWorkingDirPath );
	::printf( "logging  %d\n", mLogging );
	::printf( "log_file  %s\n", mLogFile );
	::printf( "max_upcoming_list_size  %d\n", mUpcomingSongsListSize );
	::printf( "show_current  %d\n", mShowCurrent );
	::printf( "show_upcoming  %d\n", mShowUpcoming );
	::printf( "broadcast_name \"%s\"\n", mName);
	::printf( "broadcast_genre \"%s\"\n", mGenre);
	::printf( "broadcast_mount_point \"%s\"\n", mMountPoint);
	::printf( "broadcast_password \"XXXXX\"\n");
	::printf( "\n" );
}

void MP3Broadcaster::RemoveFiles()
{
	if (mPIDFile[0] != 0)
	{
		remove(mPIDFile);
	}

	remove(mStopFile);
	remove(mReplaceFile);
	remove(mInsertFile);
	remove(mCurrentFile);
	remove(mUpcomingFile);	
}

char* MP3Broadcaster::MapErrorToString(int error)
{
	char* result = NULL;
		
	if (error == MP3FileBroadcaster::kBadFileFormat)
		result = "Bad file format.";
	else if (error == MP3FileBroadcaster::kWrongFrequency)
		result = "Encoded at wrong frequency.";
	else if (error == MP3FileBroadcaster::kWrongBitRate)
		result = "Doesn't use desired bit rate.";
	else if (error == MP3FileBroadcaster::kCouldntOpenFile)
		result = "Couldn't open file.";
		
	if (result != NULL)
		::printf("%s\n", result);
	
	return result;
}

void MP3Broadcaster::Cleanup(bool signalHandler)
{
        if (mCleanupDone)
            return;
            
        mCleanupDone = true;
        
        if (signalHandler)
        {
            mNumErrors++;
            printf("- MP3Broadcaster: Disconnected from Server while playing. Exiting.\n");
        }
        
    	if (mPreflight)
	{
		printf("Warnings: %ld\n", mNumWarnings);
		printf("Errors: %ld\n", mNumErrors);
	}
        else
        {
            printf("Broadcast Warnings: %ld\n", mNumWarnings);
            printf("Broadcast Errors: %ld\n", mNumErrors);
        }
	
	RemoveFiles();
        
        if (mLog)
        {
            if ( mPreflight )
                mLog->LogInfo( "MP3Broadcaster preflight finished." );
            else
                mLog->LogInfo( "MP3Broadcaster finished." );
	}
        
        //	mLog = NULL; // protect the interrupt handler and just let it die don't delete because it is a task thread
}

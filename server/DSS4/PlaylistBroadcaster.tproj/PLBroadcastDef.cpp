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

	8.2.99 - rt updated ShowSettings() to display user names for fields instead of C++ member names.
*/

#include "PLBroadcastDef.h"
#include "MyAssert.h"
#include "SocketUtils.h"

#include "ConfParser.h"
#include <string.h>

#include <stdio.h>	
#include <stdlib.h>
#ifndef __Win32__
#include <netdb.h>
#endif

#include "BroadcasterSession.h"

Bool16 PLBroadcastDef::ConfigSetter( const char* paramName, const char* paramValue[], void* userData )
{
	// return true if set fails
	
	
	PLBroadcastDef*	broadcastParms = (PLBroadcastDef*)userData;
	
	if (!::strcmp( "destination_ip_address", paramName) )
	{	
		broadcastParms->SetValue( &broadcastParms->mOrigDestAddress, paramValue[0] );
		if (broadcastParms->mIgnoreFileIP)
			return false;
		else
			return broadcastParms->SetValue( &broadcastParms->mDestAddress, paramValue[0] );
	}
	else if (!::strcmp( "destination_base_port", paramName) )
	{
		return broadcastParms->SetValue( &broadcastParms->mBasePort, paramValue[0] );

	}
	else if (!::strcmp( "max_upcoming_list_size", paramName) )
	{
		return broadcastParms->SetValue( &broadcastParms->mMaxUpcomingMovieListSize, paramValue[0] );

	}
	else if (!::strcmp( "play_mode", paramName) )
	{
			
		if ( ::strcmp( "sequential", paramValue[0]) 
			&& ::strcmp( "sequential_looped", paramValue[0])
			&& ::strcmp( "weighted_random", paramValue[0])
		 	)
			return true;

		return broadcastParms->SetValue( &broadcastParms->mPlayMode, paramValue[0] );

	}	
	/*
	rt- rremoved for buld 12
	else if (!::strcmp( "limit_play", paramName) )
	{
		if ( ::strcmp( "enabled", paramValue[0]) && ::strcmp( "disabled", paramValue[0]) )
			return true;

		return broadcastParms->SetValue( &broadcastParms->mLimitPlay, paramValue[0] );

	}
	*/
	// changed at bulid 12 else if (!::strcmp( "repeats_queue_size", paramName) )
	else if (!::strcmp( "recent_movies_list_size", paramName) )
	{
		if ( ::atoi( paramValue[0] )  < 0 )
			return true;
			
		broadcastParms->mLimitPlayQueueLength = atoi(paramValue[0]);

		return false;
		
	}
	else if (!::strcmp( "playlist_file", paramName) )
	{
		return broadcastParms->SetValue( &broadcastParms->mPlayListFile, paramValue[0] );

	}
	else if (!::strcmp( "sdp_file", paramName) )
	{
		return broadcastParms->SetValue( &broadcastParms->mSDPFile, paramValue[0] );

	}
	else if (!::strcmp( "destination_sdp_file", paramName) )
	{
		return broadcastParms->SetValue( &broadcastParms->mDestSDPFile, paramValue[0] );

	}
	else if (!::strcmp( "logging", paramName) )
	{
		if ( ::strcmp( "enabled", paramValue[0]) && ::strcmp( "disabled", paramValue[0]) )
			return true;

		return broadcastParms->SetValue( &broadcastParms->mLogging, paramValue[0] );

	}
	else if (!::strcmp( "log_file", paramName) )
	{
		return broadcastParms->SetValue( &broadcastParms->mLogFile, paramValue[0] );

	}
	else if (!::strcmp( "sdp_reference_movie", paramName) )
	{
		return broadcastParms->SetValue( &broadcastParms->mSDPReferenceMovie, paramValue[0] );

	}
	else if (!::strcmp( "show_current", paramName) )
	{
		if ( ::strcmp( "enabled", paramValue[0]) && ::strcmp( "disabled", paramValue[0]) )
			return true;
			
		return broadcastParms->SetValue( &broadcastParms->mShowCurrent, paramValue[0] );

	}
	else if (!::strcmp( "show_upcoming", paramName) )
	{
		if ( ::strcmp( "enabled", paramValue[0]) && ::strcmp( "disabled", paramValue[0]) )
			return true;
			
		return broadcastParms->SetValue( &broadcastParms->mShowUpcoming, paramValue[0] );

	}
	else if (!::strcmp( "broadcast_start_time", paramName) )
	{
		time_t startTime = (time_t) ::strtoul(paramName, NULL, 10);
		if (startTime < (time_t) 2208988800LU)
			return true;

		return broadcastParms->SetValue( &broadcastParms->mStartTime, paramValue[0] );
	}
	else if (!::strcmp( "broadcast_end_time", paramName) )
	{			
		time_t endTime = (time_t) ::strtoul(paramName, NULL, 10);
		if (endTime < (time_t) 2208988800LU)
			return true;

		return broadcastParms->SetValue( &broadcastParms->mEndTime, paramValue[0] );
	}
	else if (!::strcmp( "broadcast_SDP_is_dynamic", paramName) )
	{
		if ( ::strcmp( "enabled", paramValue[0]) && ::strcmp( "disabled", paramValue[0]) )
			return true;
			
		return broadcastParms->SetValue( &broadcastParms->mIsDynamic, paramValue[0] );
	}
	else if (!::strcmp( "broadcaster_name", paramName) )
	{
		return broadcastParms->SetValue( &broadcastParms->mName, paramValue[0] );

	}
	else if (!::strcmp( "broadcaster_password", paramName) )
	{
		return broadcastParms->SetValue( &broadcastParms->mPassword, paramValue[0] );

	}
	else if (!::strcmp( "multicast_ttl", paramName) )
	{
		return broadcastParms->SetValue( &broadcastParms->mTTL, paramValue[0] );

	}
	else if (!::strcmp( "rtsp_port", paramName) )
	{
		return broadcastParms->SetValue( &broadcastParms->mRTSPPort, paramValue[0] );

	}
	else if (!::strcmp( "pid_file", paramName) )
	{
		return broadcastParms->SetValue( &broadcastParms->mPIDFile, paramValue[0] );

	}
	else if (!::strcmp( "client_buffer_delay", paramName) )
	{
		return broadcastParms->SetValue( &broadcastParms->mClientBufferDelay, paramValue[0] );

	}

	
	return true;
}

Bool16 PLBroadcastDef::SetValue( char** dest, const char* value)
{
	Bool16	didFail = false;
		
	// if same param occurs more than once in file, delete 
	// initial occurance and override with second.
	if ( *dest )
		delete [] *dest;
		
	*dest = new char[ strlen(value) + 1 ];
	Assert( *dest );
	
	if ( *dest )
		::strcpy( *dest, value );
	else 
		didFail = true;

	return didFail;
}

Bool16 PLBroadcastDef::SetDefaults( const char* setupFileName )
{
	Bool16	didFail = false;
	
	if (mDestAddress != NULL)
		mIgnoreFileIP = true;
		
	if ( !didFail && !mIgnoreFileIP)	
		didFail = this->SetValue( &mDestAddress, SocketUtils::GetIPAddrStr(0)->Ptr );

	if ( !didFail )	
		didFail = this->SetValue( &mBasePort, "5004" );

	if ( !didFail )	
		didFail = this->SetValue( &mPlayMode, "sequential_looped" );
	
	if ( !didFail )	
		didFail = this->SetValue( &mMaxUpcomingMovieListSize, "7" );
	
	if ( !didFail )	
		didFail = this->SetValue( &mLogging, "enabled" );

	if ( !didFail )	
		didFail = this->SetValue( &mRTSPPort, "554" );

	char	nameBuff[512];
	::strcpy( nameBuff, "broadcast" );
	if (setupFileName)
		::strcpy( nameBuff, setupFileName );
		
	int		baseLen = strlen(nameBuff);

/*
	
	if you want to add .log to the base name of the 
	description file with the .ext stripped, un comment 
	this code.
	rt 8.12.99
*/
	char	*ext = NULL;
	ext = ::strrchr( nameBuff, '.' );
	if ( ext )
	{	
		*ext  = 0;
		baseLen = ::strlen(nameBuff);
	}

		
	::strcat( nameBuff, ".log" );	
	if ( !didFail )
		didFail = this->SetValue( &mLogFile, nameBuff );

	nameBuff[baseLen] = 0;	
	::strcat( nameBuff, ".ply" );	
	if ( !didFail )	
		didFail = this->SetValue( &mPlayListFile, nameBuff );
	

//	nameBuff[baseLen] = 0;
//	::strcat( nameBuff, ".mov" );	
//	if ( !didFail )	
//		didFail = this->SetValue( &mSDPReferenceMovie, nameBuff );
	
	nameBuff[baseLen] = 0;
	::strcat( nameBuff, ".sdp" );
	if ( !didFail )	
		didFail = this->SetValue( &mSDPFile, nameBuff );

	if ( !didFail )	
		didFail = this->SetValue( &mDestSDPFile, "no_name" );
	

/* current, upcoming, and replacelist created by emil@popwire.com */
	nameBuff[baseLen] = 0;
	::strcat( nameBuff, ".current" );	
	if ( !didFail )
		didFail = this->SetValue( &mCurrentFile, nameBuff );

	nameBuff[baseLen] = 0;
	::strcat( nameBuff, ".upcoming" );	
	if ( !didFail )
		didFail = this->SetValue( &mUpcomingFile, nameBuff );

	nameBuff[baseLen] = 0;
	::strcat( nameBuff, ".replacelist" );	
	if ( !didFail )
		didFail = this->SetValue( &mReplaceFile, nameBuff );

	nameBuff[baseLen] = 0;
	::strcat( nameBuff, ".stoplist" );	
	if ( !didFail )
		didFail = this->SetValue( &mStopFile, nameBuff );
		
	nameBuff[baseLen] = 0;
	::strcat( nameBuff, ".insertlist" );	
	if ( !didFail )
		didFail = this->SetValue( &mInsertFile, nameBuff );
	
	if ( !didFail )	
		didFail = this->SetValue( &mShowCurrent, "enabled" );
		
	if ( !didFail )	
		didFail = this->SetValue( &mShowUpcoming, "enabled" );
		
	if ( !didFail )	
		didFail = this->SetValue( &mStartTime, "0" );

	if ( !didFail )	
		didFail = this->SetValue( &mEndTime, "0" );

	if ( !didFail )	
		didFail = this->SetValue( &mIsDynamic, "disabled" );
		
	if ( !didFail )	
		didFail = this->SetValue( &mName, "" );

	if ( !didFail )	
		didFail = this->SetValue( &mPassword, "" );

	if ( !didFail )	
		didFail = this->SetValue( &mTTL, "1" );

	if ( !didFail )	
		didFail = this->SetValue( &mClientBufferDelay, "0" );
	
/* ***************************************************** */
	return didFail;
}


PLBroadcastDef::PLBroadcastDef( const char* setupFileName,  char *destinationIP, Bool16 debug )
	: mDestAddress(destinationIP)
	, mOrigDestAddress(NULL)
	, mBasePort(NULL)
	, mPlayMode(NULL)
	// removed at build 12 , mLimitPlay(NULL)
	, mLimitPlayQueueLength(0)
	, mPlayListFile(NULL)
	, mSDPFile(NULL)
	, mLogging(NULL)
	, mLogFile(NULL)
	, mSDPReferenceMovie( NULL )
	, mCurrentFile( NULL )
	, mUpcomingFile( NULL )
	, mReplaceFile( NULL )
	, mStopFile( NULL )
	, mInsertFile( NULL )
	, mShowCurrent( NULL )
	, mShowUpcoming( NULL )
	, mTheSession( NULL )
	, mIgnoreFileIP(false)
	, mMaxUpcomingMovieListSize(NULL)
	, mDestSDPFile(NULL)
	, mStartTime(NULL)
	, mEndTime(NULL)
	, mIsDynamic(NULL)
	, mName( NULL)
	, mPassword( NULL)
	, mTTL(NULL)
	, mRTSPPort(NULL)
	, mPIDFile(NULL)
	, mClientBufferDelay(NULL)
	, mParamsAreValid(false)
	, mInvalidParamFlags(kInvalidNone)
	
{

	if (!setupFileName && !destinationIP)
	{	this->SetDefaults( NULL );
		::printf( "default settings\n" ); 
		this->ShowSettings();
		return;
	}

	fDebug = debug;
	if (destinationIP != NULL) //we were given an IP to use
		mIgnoreFileIP = true;
			
	Assert( setupFileName );
	
	if (setupFileName )
	{
		int	err = -1;
	
		if ( !this->SetDefaults( setupFileName ) )
		{	err = ::ParseConfigFile( false, setupFileName, ConfigSetter, this );
		}


		if ( !err )
		{	mParamsAreValid = true;		
		}
		
		ValidateSettings();
	}
}

void PLBroadcastDef::ValidateSettings()
{

	// For now it just validates the destination ip address
	UInt32 inAddr = 0;
	inAddr = SocketUtils::ConvertStringToAddr(mDestAddress);
	if(inAddr == INADDR_NONE)
	{
		struct hostent* theHostent = ::gethostbyname(mDestAddress);		
		if (theHostent != NULL)
		{
			inAddr = ntohl(*(UInt32*)(theHostent->h_addr_list[0]));
			
			struct in_addr inAddrStruct;
			char buffer[50];
			StrPtrLen temp(buffer);
			inAddrStruct.s_addr = *(UInt32*)(theHostent->h_addr_list[0]);
			SocketUtils::ConvertAddrToString(inAddrStruct, &temp);
			SetValue( &mDestAddress, buffer );
		}
	}
	if(inAddr == INADDR_NONE)
		mInvalidParamFlags |= kInvalidDestAddress;

	// If mInvalidParamFlags is set, set mParamsAreValid to false
	if ( mInvalidParamFlags | kInvalidNone )
		mParamsAreValid = false;
}

void PLBroadcastDef::ShowErrorParams()
{
	if( mInvalidParamFlags & kInvalidDestAddress )
		::printf( "destination_ip_address \"%s\" is Invalid\n", mOrigDestAddress );
}


void PLBroadcastDef::ShowSettings()
{
	
	
	::printf( "\n" );
	::printf( "Description File Settings\n" );
	::printf( "----------------------------\n" );
		
	::printf( "destination_ip_address  %s\n", mOrigDestAddress );
	::printf( "destination_sdp_file  %s\n", mDestSDPFile );
	::printf( "destination_base_port  %s\n", mBasePort );
	::printf( "play_mode  %s\n", mPlayMode );
	::printf( "recent_movies_list_size  %d\n", mLimitPlayQueueLength );
	::printf( "playlist_file  %s\n", mPlayListFile );
	::printf( "logging  %s\n", mLogging );
	::printf( "log_file  %s\n", mLogFile );
	if (mSDPReferenceMovie != NULL)
		::printf( "sdp_reference_movie  %s\n", mSDPReferenceMovie );
	::printf( "sdp_file  %s\n", mSDPFile );
	::printf( "max_upcoming_list_size  %s\n", mMaxUpcomingMovieListSize );
	::printf( "show_current  %s\n", mShowCurrent );
	::printf( "show_upcoming  %s\n", mShowUpcoming );
	::printf( "broadcaster_name \"%s\"\n", mName);
	::printf( "broadcaster_password \"XXXXX\"\n");
	::printf( "multicast_ttl %s\n",mTTL);
	::printf( "rtsp_port %s\n",mRTSPPort);

	Float32 bufferDelay = 0.0;
	::sscanf(mClientBufferDelay, "%f", &bufferDelay);
	if (bufferDelay != 0.0) 
		::printf( "client_buffer_delay %.2f\n",bufferDelay);
	else
		::printf( "client_buffer_delay default\n");
	
	if (mPIDFile != NULL)
		::printf( "pid_file %s\n",mPIDFile);
	
	time_t startTime = (time_t) ::strtoul(mEndTime, NULL, 10);
	if (startTime != 0)
	{	 startTime -= (time_t) 2208988800LU; //1970 - 1900 secs
		::printf( "broadcast_start_time  %s = %s", mStartTime,ctime(&startTime));
	}
	else
		::printf( "broadcast_start_time  %s\n", mStartTime);
	
	
	time_t endTime = (time_t) strtoul(mEndTime, NULL, 10);
	if (endTime != 0)
	{	endTime -= (time_t) 2208988800LU;//1970 - 1900 secs
		::printf( "broadcast_end_time  %s = %s", mEndTime,ctime(&endTime) );
	}
	else
		::printf( "broadcast_end_time  %s\n", mEndTime);
	
	::printf( "broadcast_SDP_is_dynamic  %s\n", mIsDynamic );
	::printf( "============================\n" );
	
}


PLBroadcastDef::~PLBroadcastDef()
{
	delete [] mDestAddress;
	delete [] mOrigDestAddress;
	delete [] mBasePort;
	delete [] mPlayMode;
	// removed at build 12 delete [] mLimitPlay;
	delete [] mPlayListFile;
	delete [] mSDPFile;
	delete [] mLogging;
	delete [] mLogFile;
	if (mSDPReferenceMovie != NULL)
		delete [] mSDPReferenceMovie;
	delete [] mMaxUpcomingMovieListSize;
	delete [] mTTL;
	delete [] mName;
	delete [] mShowUpcoming;
	delete [] mShowCurrent;
	delete [] mPassword;
	delete [] mIsDynamic;
	delete [] mStartTime;
	delete [] mEndTime;
	
}

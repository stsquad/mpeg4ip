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

	8.2.99 - rt updated ShowSettings() to display user names for fields instead of C++ member names.
*/

#include "PLBroadcastDef.h"
#include "MyAssert.h"
#include "SocketUtils.h"

#include "ConfParser.h"
#include <string.h>

#include <stdio.h>	
#include <stdlib.h>


Bool16 PLBroadcastDef::ConfigSetter( const char* paramName, const char* paramValue[], void* userData )
{
	// return true if set fails
	
	
	PLBroadcastDef*	broadcastParms = (PLBroadcastDef*)userData;
	
	if (!::strcmp( "destination_ip_address", paramName) )
	{
		return broadcastParms->SetValue( &broadcastParms->mDestAddress, paramValue[0] );

	}
	else if (!::strcmp( "destination_base_port", paramName) )
	{
		return broadcastParms->SetValue( &broadcastParms->mBasePort, paramValue[0] );

	}
	else if (!::strcmp( "ttl", paramName) )
	{
		return broadcastParms->SetValue( &broadcastParms->mTtl, paramValue[0] );

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
			
		return broadcastParms->SetValue( &broadcastParms->mLimitPlayQueueLength, paramValue[0] );

	}
	else if (!::strcmp( "playlist_file", paramName) )
	{
		return broadcastParms->SetValue( &broadcastParms->mPlayListFile, paramValue[0] );

	}
	else if (!::strcmp( "sdp_file", paramName) )
	{
		return broadcastParms->SetValue( &broadcastParms->mSDPFile, paramValue[0] );

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
	
	if ( !didFail )	
		didFail = this->SetValue( &mDestAddress, SocketUtils::GetIPAddrStr(0)->Ptr );

	if ( !didFail )	
		didFail = this->SetValue( &mBasePort, "5004" );

	if ( !didFail )	
		didFail = this->SetValue( &mTtl, "1" );

	if ( !didFail )	
		didFail = this->SetValue( &mPlayMode, "sequential_looped" );

	/*	removed at build 12 
		if ( !didFail )	
		didFail = this->SetValue( &mLimitPlay, "enabled" );
	*/
	if ( !didFail )	
		didFail = this->SetValue( &mLimitPlayQueueLength, "0" );

	if ( !didFail )	
		didFail = this->SetValue( &mLogging, "enabled" );


	char	nameBuff[512];
	::strcpy( nameBuff, setupFileName );
	int		baseLen = strlen(nameBuff);
	
	/*
	
	if you want to add .log to the base name of the 
	description file with the .ext stripped, un comment 
	this code.
	rt 8.12.99
	
	char	*ext;
	ext = ::strrchr( nameBuff, '.' );
	
	if ( ext )
	{
		*ext  = 0;
		
	}
	*/
	
	::strcat( nameBuff, ".log" );	
	if ( !didFail )
		didFail = this->SetValue( &mLogFile, nameBuff );

	nameBuff[baseLen] = 0;	
	::strcat( nameBuff, ".ply" );	
	if ( !didFail )	
		didFail = this->SetValue( &mPlayListFile, nameBuff );
	

	nameBuff[baseLen] = 0;
	::strcat( nameBuff, ".mov" );	
	if ( !didFail )	
		didFail = this->SetValue( &mSDPReferenceMovie, nameBuff );
	
	nameBuff[baseLen] = 0;
	::strcat( nameBuff, ".sdp" );
	if ( !didFail )	
		didFail = this->SetValue( &mSDPFile, nameBuff );
	
	return didFail;
}


PLBroadcastDef::PLBroadcastDef( const char* setupFileName )
	: mDestAddress(NULL)
	, mBasePort(NULL)
	, mTtl(NULL)
	, mPlayMode(NULL)
	// removed at build 12 , mLimitPlay(NULL)
	, mLimitPlayQueueLength(NULL)
	, mPlayListFile(NULL)
	, mSDPFile(NULL)
	, mLogging(NULL)
	, mLogFile(NULL)
	, mSDPReferenceMovie( NULL )
{
	mParamsAreValid = false;
	
	Assert( setupFileName );
	
	if (setupFileName )
	{
		int	err = -1;
	
		if ( !this->SetDefaults( setupFileName ) )
			err = ::ParseConfigFile( false, setupFileName, ConfigSetter, this );


		if ( !err )
			mParamsAreValid = true;
		
		
	}
}

void PLBroadcastDef::ShowSettings()
{
	::printf( "\n" );
	::printf( "PlaylistBroadcaster Settings\n" );
	::printf( "----------------------------\n" );
	::printf( "destination_ip_address: %s\n", mDestAddress );
	::printf( "destination_base_port: %s\n", mBasePort );
	::printf( "ttl: %s\n", mTtl );
	::printf( "play_mode: %s\n", mPlayMode );
	// ::printf( "limit_play: %s\n", mLimitPlay );
	::printf( "recent_movies_list_size: %s\n", mLimitPlayQueueLength );
	::printf( "playlist_file: %s\n", mPlayListFile );
	::printf( "logging: %s\n", mLogging );
	::printf( "log_file: %s\n", mLogFile );
	::printf( "sdp_reference_movie: %s\n", mSDPReferenceMovie ); 
	::printf( "sdp_file: %s\n", mSDPFile );
}


PLBroadcastDef::~PLBroadcastDef()
{
	delete [] mDestAddress;
	delete [] mBasePort;
	delete [] mTtl;
	delete [] mPlayMode;
	// removed at build 12 delete [] mLimitPlay;
	delete [] mLimitPlayQueueLength;
	delete [] mPlayListFile;
	delete [] mSDPFile;
	delete [] mLogging;
	delete [] mLogFile;
	delete [] mSDPReferenceMovie;

}

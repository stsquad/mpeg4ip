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

#ifndef __MP3BroadcasterLog_h__
#define __MP3BroadcasterLog_h__

#include "QTSSRollingLog.h"
#include "StrPtrLen.h"
#include <string.h>


class MP3BroadcasterLog : public QTSSRollingLog
{
	enum { eLogMaxBytes = 0, eLogMaxDays = 0 };
	
	public:
		MP3BroadcasterLog( char* defaultPath, char* logName, bool enabled );
		virtual ~MP3BroadcasterLog() {}
	
		virtual char* GetLogName() 
		{ 	// RTSPRollingLog wants to see a "new'd" copy of the file name
			char*	name = new char[strlen( mLogFileName ) + 1 ];
			
			if ( name )
				::strcpy( name, mLogFileName );

			return name;
		}		
		
		virtual char* GetLogDir() 
		{ 	// RTSPRollingLog wants to see a "new'd" copy of the file name
			char *name = new char[strlen( mDirPath ) + 1 ];
			
			if ( name )
				::strcpy( name, mDirPath );

			return name;
		}		
																				
		virtual UInt32 GetRollIntervalInDays() { return eLogMaxDays; /* we dont' roll*/ }
											
		virtual UInt32 GetMaxLogBytes() {  return eLogMaxBytes; /* we dont' roll*/ }
		
		void	LogInfo( const char* infoStr );
		void	LogMoviePlay( const char* path, const char* errStr, const char* messageStr);

		bool	WantsLogging() { return mWantsLogging; }
		const char*	LogFileName() { return mLogFileName; }
		const char*	LogDirName() { return mDirPath; }
	protected:
		char 	mDirPath[256];
		char 	mLogFileName[256];
		bool	mWantsLogging;
	
};

#endif


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



#include "BroadcastLog.h"


BroadcastLog::BroadcastLog( PLBroadcastDef*	broadcastParms, StrPtrLen* defaultPathPtr ) 
			: QTSSRollingLog() 
{
	*mDirPath = 0;
	*mLogFileName = 0;
	mWantsLogging = false;
	
	if (broadcastParms->mLogging)
	{
		if (!::strcmp( broadcastParms->mLogging, "enabled" ) )
		{
			mWantsLogging = true;
			
			::strcpy( mDirPath, broadcastParms->mLogFile );
			char*	nameBegins = ::strrchr( mDirPath, kPathDelimiterChar );
			if ( nameBegins )
			{
				*nameBegins = 0; // terminate mDirPath at the last PathDelimeter
				nameBegins++;
				::strcpy( mLogFileName, nameBegins );
			}
			else
			{	// it was just a file name, no dir spec'd
				memcpy(mDirPath,defaultPathPtr->Ptr,defaultPathPtr->Len);
				mDirPath[defaultPathPtr->Len] = 0;
				
				::strcpy( mLogFileName, broadcastParms->mLogFile );
			}
			
		}
	}
}


void	BroadcastLog::LogInfo( const char* infoStr )
{
	// log a generic comment 
	char	strBuff[1024] = "# ";
	char	dateBuff[80] = "";
	
	if ( this->FormatDate( dateBuff, false ) )
	{	
		if  (	(NULL != infoStr) 
			&& 	( ( strlen(infoStr) + strlen(strBuff) + strlen(dateBuff)  ) < 800)
			)
		{
			sprintf(strBuff,"# %s %s\n",dateBuff, infoStr);
			this->WriteToLog( strBuff, kAllowLogToRoll );
		}
		else
		{	
			::strcat(strBuff,dateBuff);
			::strcat(strBuff," internal error in LogInfo\n");
			this->WriteToLog( strBuff, kAllowLogToRoll );		
		}

	}
	
}


void BroadcastLog::LogMoviePlay( const char* path, const char* errStr , const char* messageStr)
{
	// log movie play info
	char	strBuff[1024] = "";
	char	dateBuff[80] = "";
	
	if ( this->FormatDate( dateBuff, false ) )
	{	
		if  (	(NULL != path) 
				&&	( (strlen(path) + strlen(dateBuff) ) < 800)
			)
		{

			sprintf(strBuff,"%s %s ",dateBuff, path);
					
			if ( errStr )
			{	if  ( (strlen(strBuff) + strlen(errStr) ) < 1000 )
				{
					::strcat(strBuff,"Error:");
					::strcat(strBuff,errStr);
				}
			}
			else
				if 	( 	(NULL != messageStr) 
						&& 
						( (strlen(strBuff) + strlen(messageStr) ) < 1000 )
					)
				{	::strcat(strBuff,messageStr);
				}
				else
					::strcat(strBuff,"OK");
				
			::strcat(strBuff,"\n");
			this->WriteToLog(strBuff, kAllowLogToRoll );
		}
		else
		{	
			::strcat(strBuff,dateBuff);
			::strcat(strBuff," internal error in LogMoviePlay\n");
			this->WriteToLog( strBuff, kAllowLogToRoll );		
		}

	}
	
}


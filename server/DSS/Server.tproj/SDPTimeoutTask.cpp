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
	File:		SDPTimeoutTask.cpp

	Contains:	Implementation of class defined in SDPTimeoutTask.h
					
	
	
	

*/

#include "QTSServerInterface.h"
#include "StringParser.h"
#include "StringFormatter.h"
#include "OSMemory.h"
#include "StrPtrLen.h"
#include "SDPTimeoutTask.h"
#include "OSArrayObjectDeleter.h"
#include "OS.h"
#include "SDPSourceInfo.h"
#include <stdio.h> 
#ifndef __Win32__
	#include <unistd.h>
	#include <sys/types.h>
	#include <dirent.h>

	#ifdef __MacOSX__
	    char *SDPTimeoutTask::sTempFile = "/Library/QuickTimeStreaming/broadcast_sdpfiles";
	#else
		char *SDPTimeoutTask::sTempFile = "/var/streaming/broadcast_sdpfiles";
	#endif
	
#endif

#if __Win32__
	char *SDPTimeoutTask::sTempFile = "c:\\Program Files\\Darwin Streaming Server\\broadcast_sdpfiles";
#endif


int SDPTimeoutTask::sState = SDPTimeoutTask::kSearchFiles;
UInt32 SDPTimeoutTask::sInterval = 10000; // 10 seconds
StrPtrLen SDPTimeoutTask::sFileList;

Bool16 SDPTimeoutTask::sFirstRun = true;


SInt16 SDPTimeoutTask::FindSDPFiles(char *thePath, FILE *outputFilePtr, SInt16 depth)
{
	if (NULL == thePath || 0 == thePath[0])
		return -1;
		
	outputFilePtr = ::fopen(sTempFile,"w");
	if (NULL == outputFilePtr)
	{	return -1;
	}
	::fclose(outputFilePtr);

#ifndef __Win32__

	char	findCommand[2048];
	if (strlen(thePath) > 1800) 
		return -1;
		
	// default find command
	::sprintf( findCommand, "find -L \"%s\" -name \"*.sdp\" > %s", thePath,sTempFile);	

	#ifdef __linux__ 
		static SInt16 sMaxDepth = 20;
		::sprintf( findCommand, "find \"%s\" -follow -maxdepth %d -name \"*.sdp\" -fprint %s", thePath,sMaxDepth, sTempFile);
	#endif

	#ifdef __linuxppc__
		static SInt16 sMaxDepth = 20;
		::sprintf( findCommand, "find \"%s\" -follow -maxdepth %d -name \"*.sdp\" -fprint %s", thePath,sMaxDepth, sTempFile);
	#endif

	#ifdef __solaris__
		::sprintf( findCommand, "find \"%s\" -name \"*.sdp\" > %s", thePath,sTempFile);
	#endif
		
	return system(findCommand);
#endif


#ifdef __Win32__

	static SInt16 sMaxDepth = 20;
	SInt16 result = 0;
	char searchPath[2048]; 
	searchPath[0] = 0;
	WIN32_FIND_DATA findData;
	HANDLE findResultHandle;
    Bool16 keepSearching = true;
	SInt16 pathLen = strlen(thePath);
	SInt16 pathEnd = pathLen;
	
 	if (0 == pathLen)
		return 0;

	if ( thePath[pathLen - 1] != '\\' )
	{	::sprintf(searchPath,"%s%s",thePath,"\\*");
		pathEnd ++; // add '\' to the len not '*'
 	}
	else
	{	::sprintf(searchPath,"%s%s",thePath,"*");
	}

	 //printf("FindFirstFile path=%s\n",searchPath);

     findResultHandle = ::FindFirstFile( searchPath, &findData);
     if (	NULL == findResultHandle 
		 || INVALID_HANDLE_VALUE == findResultHandle
		 )
     {
        //printf( "FindFirstFile( \"%s\" ): gle = %lu\n", searchPath, GetLastError() );
        return 0;
     }

 	depth ++;

	if (depth > sMaxDepth)
		return 0;

    while ( keepSearching )
    {
		if	(	(findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ) 
			&&	strcmp( findData.cFileName, "." ) != 0 
			&&	strcmp( findData.cFileName, ".." ) != 0 
			)

		{
            strcpy( searchPath + pathEnd, findData.cFileName );
            this->FindSDPFiles( searchPath, outputFilePtr, depth );
        }
		else
		{	SInt16 nameLen = ::strlen(findData.cFileName);
			if	(	nameLen > 4
				 && 0 == _stricmp( &findData.cFileName[nameLen - 4], ".sdp")
				 )
			{	searchPath[pathEnd] = 0; // get rid of "*" if it exists
				fprintf(outputFilePtr,"%s%s\n",searchPath,findData.cFileName);
				//printf("found sdp=%s%s\n",searchPath,findData.cAlternateFileName);
			}

		}
        keepSearching = FindNextFile( findResultHandle, &findData );
     }

    FindClose( findResultHandle );

	return result;
#endif

}


SInt64 SDPTimeoutTask::Run()
{
	//printf("SDPTimeoutTask::Run\n");
	sInterval = QTSServerInterface::GetServer()->GetPrefs()->DeleteSDPFilesInterval() * 1000;
	if (sInterval < 1000) // don't allow less than 1 second interval.
		sInterval = 1000;
		
	if (QTSServerInterface::GetServer()->GetPrefs()->AutoDeleteSDPFiles() != true)
		return sInterval;
		

	if (QTSServerInterface::GetServer()->GetServerState() != qtssRunningState)
		return sInterval;
		
	SInt64	interval = sInterval;
	if (sState == kSearchFiles || sFirstRun) do
	{	
		//printf("SDPTimeoutTask sState == kSearchFiles \n");
		fCurrentListFile = 0;
		
		UInt32 pathLen = 0;
		char *noPath = NULL;
		char *thePath = QTSServerInterface::GetServer()->GetPrefs()->GetMovieFolder(noPath, &pathLen);
		OSCharArrayDeleter pathDeleter(thePath);
		
		//printf("movies folder = %s \n",thePath);

		SInt16 result = 0;

#ifndef __Win32__

		DIR *testDir = opendir(thePath);
		if (NULL == testDir) 
			break;


		closedir(testDir);
		result = this->FindSDPFiles(thePath, NULL, 0);
		if (result != 0)
			return sInterval;
		
#else

		// WIN32 has to make its own output file.

		FILE* outputFilePtr = ::fopen(sTempFile,"w");
		if (NULL == outputFilePtr)
			return sInterval;

		result = this->FindSDPFiles(thePath, outputFilePtr, 0);
		::fflush(outputFilePtr);
		::fclose(outputFilePtr);
		if (result != 0)
			return sInterval;

#endif


		if (0 == result)
		{	
			sFileList.Delete(); //just to be sure.
	
			(void)QTSSModuleUtils::ReadEntireFile(sTempFile, &sFileList);
			if (sFileList.Len == 0) 
			{	sFileList.Delete(); //just to be sure.
			}
			else
			{	sState = kCheckingFiles;
				interval = 100;
				//printf("the SDPFile list = \n%s \n",sFileList.Ptr);
			}
		}
						
	} while (false);

	if (sState == kCheckingFiles)
	{
		//printf("SDPTimeoutTask sState == kCheckingFiles \n");
		char tempchar;
		time_t unixTimeNow = OS::UnixTime_Secs();
		StrPtrLen sdpFile;
		StringParser sdpList(&sFileList);
		UInt32 countLines = 0;
		while (sdpList.GetThruEOL(&sdpFile))
		{
			countLines ++;
			
			if (countLines <= fCurrentListFile)
				continue;
				
			if (sdpFile.Len == 0)
				continue;//skip over any blank lines
			
			StrPtrLen theFileData;
			tempchar = sdpFile.Ptr[sdpFile.Len]; sdpFile.Ptr[sdpFile.Len] = 0;
			//printf("Read sdpfile = %s countlines=%lu fCurrentListFile=%lu\n",sdpFile.Ptr,countLines,fCurrentListFile);
			(void)QTSSModuleUtils::ReadEntireFile(sdpFile.Ptr, &theFileData);
			sdpFile.Ptr[sdpFile.Len] = tempchar;
			if (theFileData.Len == 0) 
			{	theFileData.Delete(); 
				continue;
			}
			if (theFileData.Len > 1024 * 10) // maximum sdp size
			{	theFileData.Delete(); 
				continue;
			}

			SDPSourceInfo* theInfo = NEW SDPSourceInfo(theFileData.Ptr, theFileData.Len);
			if (theInfo != NULL)
			{
				time_t endTime = theInfo->GetEndTimeUnixSecs();
				if	( 	(sFirstRun && theInfo->IsRTSPControlled()) 
					||	( (0 != endTime) && (unixTimeNow > theInfo->GetEndTimeUnixSecs()) )
					)
				{
					tempchar = sdpFile.Ptr[sdpFile.Len]; sdpFile.Ptr[sdpFile.Len] = 0;
					::unlink(sdpFile.Ptr); //printf("SDP timed out - delete %s\n",sdpFile.Ptr);
					sdpFile.Ptr[sdpFile.Len] = tempchar;
				}
			}
			delete theInfo;
			theFileData.Delete(); 
			
			fCurrentListFile = countLines;

			if (!sFirstRun) // first run does all
				break;
		}
		

		if (sdpList.GetDataRemaining() == 0)
		{	sState = kSearchFiles;
			sFileList.Delete();
			interval = sInterval; //go back to being called at the sInterval of 10 seconds
			fCurrentListFile = 0;
		}
		else 
			interval = 100; //continue to process sdps at 1 per 1/10 second
	}
	if (sFirstRun)
	{	sFirstRun = false;
		//printf("First Run \n");

	}


	return interval;
}

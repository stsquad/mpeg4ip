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
	File:		QTSSRollingLog.cpp

	Contains:	Implements object defined in .h file


*/

#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <sys/types.h>   
#include <sys/stat.h>
#include <errno.h> 
#ifndef __Win32__
#include <sys/time.h>
#endif

#include "QTSSRollingLog.h"
#include "OS.h"
#include "OSMemory.h"
#include "OSArrayObjectDeleter.h"
 
 QTSSRollingLog::QTSSRollingLog() : 	
	fLog(NULL), 
	fLogCreateTime(-1),
	fLogFullPath(NULL)
{
	//this->EnableLog();
}

QTSSRollingLog::~QTSSRollingLog()
{
	CloseLog();
	if (fLogFullPath != NULL)
		delete[] fLogFullPath;
}

void QTSSRollingLog::WriteToLog(char* inLogData, Bool16 allowLogToRoll)
{
	if (allowLogToRoll)
		(void)this->CheckRollLog();
	if (fLog != NULL)
	{
		::fprintf(fLog, "%s", inLogData);
		::fflush(fLog);
	}
}

Bool16 QTSSRollingLog::RollLog()
{
	//returns false if an error occurred, true otherwise

	//close the old file.
	this->CloseLog();
	//rename the old file
	Bool16 result = this->RenameLogFile(fLogFullPath);
	if (result)
		this->EnableLog();//re-opens log file

	return result;
}

//returns false if some error has occurred
Bool16 QTSSRollingLog::FormatDate(char *ioDateBuffer)
{
	Assert(NULL != ioDateBuffer);
	
	//use ansi routines for getting the date.
	time_t calendarTime = ::time(NULL);
	Assert(-1 != calendarTime);
	if (-1 == calendarTime)
		return false;
		
	struct tm* theLocalTime = ::localtime(&calendarTime);
	Assert(NULL != theLocalTime);
	if (NULL == theLocalTime)
		return false;
		
	//date needs to look like this for common log format: 29/Sep/1998:11:34:54 -0700
	//this wonderful ANSI routine just does it for you.
	//::strftime(ioDateBuffer, kMaxDateBufferSize, "%d/%b/%Y:%H:%M:%S", theLocalTime);
	::strftime(ioDateBuffer, kMaxDateBufferSizeInBytes, "%Y-%m-%d %H:%M:%S", theLocalTime);	
	return true;
}


Bool16 QTSSRollingLog::CheckRollLog()
{
	//returns false if an error occurred, true otherwise
	if (fLog == NULL)
		return true;
	
	//first check to see if log rolling should happen because of a date interval.
	//This takes precedence over size based log rolling
	if ((-1 != fLogCreateTime) && (0 != this->GetRollIntervalInDays()))
	{
		time_t calendarTime = ::time(NULL);
		Assert(-1 != calendarTime);
		if (-1 != calendarTime)
		{
			double theExactInterval = ::difftime(calendarTime, fLogCreateTime);
			SInt32 theCurInterval = (SInt32)::floor(theExactInterval);
			
			//transfer_roll_interval is in days, theCurInterval is in seconds
			SInt32 theRollInterval = this->GetRollIntervalInDays() * 60 * 60 * 24;
			if (theCurInterval > theRollInterval)
				return this->RollLog();
		}
	}
	
	//now check size based log rolling
	UInt32 theCurrentPos = ::ftell(fLog);
	//max_transfer_log_size being 0 is a signal to ignore the setting.
	if ((this->GetMaxLogBytes() != 0) &&
		(theCurrentPos > this->GetMaxLogBytes()))
		return this->RollLog();
	return true;
}

void QTSSRollingLog::CloseLog()
{
	if (fLog != NULL)
	{
		::fclose(fLog);
		fLog = NULL;
	}
}

void QTSSRollingLog::EnableLog( Bool16 appendDotLog )
{
	//kill off the cached value
	if (fLogFullPath != NULL)
		delete[] fLogFullPath;
	
	//The log name passed into this function is a private copy of the path,
	//we are responsible for it.
	OSCharArrayDeleter logDirectory(this->GetLogDir());
	OSCharArrayDeleter logBaseName(this->GetLogName());
	
	//- don't append .log for playlist broadcaster 
	int pathLen = ::strlen(logDirectory) + ::strlen(logBaseName) + 2;
	
	if ( appendDotLog )
		pathLen +=+ ::strlen(".log");
		
	fLogFullPath = NEW char[pathLen];
	
	//copy over the directory - append a '/' if it's missing
	::strcpy(fLogFullPath, logDirectory);
	if (fLogFullPath[::strlen(fLogFullPath)-1] != kPathDelimiterChar)
	{
		::strcat(fLogFullPath, kPathDelimiterString);
	}
	
	//copy over the base filename & suffix
	::strcat(fLogFullPath, logBaseName);
	
	//- don't append .log for playlist broadcaster 
	if ( appendDotLog )
		::strcat(fLogFullPath, ".log");
	
	//we need to make sure that when we create a new log file, we write the
	//log header at the top
	Bool16 logExists = DoesFileExist(fLogFullPath);
	
	//create the log directory if it doesn't already exist
	if (!logExists)
		OS::RecursiveMakeDir(logDirectory);

	
	fLog = fopen(fLogFullPath, "a+");//open for "append"

	if (NULL != fLog)
	{
		//If the file is new, write a log header with the create time of the file.
		//If it's old, read the log header to find the create time of the file.
		if (!logExists)
			fLogCreateTime = this->WriteLogHeader(fLog);
		else
			fLogCreateTime = this->ReadLogHeader(fLog);
	}
}

Bool16 QTSSRollingLog::RenameLogFile(const char* inFileName)
{
	//returns false if an error occurred, true otherwise

	//this function takes care of renaming a log file from "myLogFile.log" to
	//"myLogFile.981217000.log" or if that is already taken, myLogFile.981217001.log", etc 
	time_t calendarTime = ::time(NULL);
	Assert(-1 != calendarTime);
	if (-1 == calendarTime)
		return false;
	
	//fix 2287086. Rolled log name can be different than original log name
	//GetLogDir returns a copy of the log dir
	OSCharArrayDeleter logDirectory(this->GetLogDir());

	//create the log directory if it doesn't already exist
	OS::RecursiveMakeDir(logDirectory.GetObject());
	
	//GetLogName returns a copy of the log name
	OSCharArrayDeleter logBaseName(this->GetLogName());
		
	//QTStreamingServer.981217003.log
	//format the new file name
	OSCharArrayDeleter theNewNameBuffer(NEW char[::strlen(logDirectory) + kMaxFilenameLengthInBytes + 3]);
	
	//copy over the directory - append a '/' if it's missing
	::strcpy(theNewNameBuffer, logDirectory);
	if (theNewNameBuffer[::strlen(theNewNameBuffer)-1] != kPathDelimiterChar)
	{
		::strcat(theNewNameBuffer, kPathDelimiterString);
	}
	
	//copy over the base filename
	::strcat(theNewNameBuffer, logBaseName.GetObject());

	//append today's date
	struct tm* theLocalTime = ::localtime(&calendarTime);
	char timeString[10];
	::strftime(timeString,  10, ".%y%m%d", theLocalTime);
	::strcat(theNewNameBuffer, timeString);
	
	SInt32 theBaseNameLength = ::strlen(theNewNameBuffer);


	//loop until we find a unique name to rename this file
	//and append the log number and suffix
	SInt32 theErr = 0;
	for (SInt32 x = 0; (theErr == 0) && (x<=1000); x++)
	{
		if (x  == 1000)	//we don't have any digits left, so just reuse the "---" until tomorrow...
		{
			//add a bogus log number and exit the loop
			sprintf(theNewNameBuffer + theBaseNameLength, "---.log");
			break;
		}

		//add the log number & suffix
		sprintf(theNewNameBuffer + theBaseNameLength, "%03ld.log", x);

		//assume that when ::stat returns an error, it is becase
		//the file doesnt exist. Once that happens, we have a unique name
		// csl - shouldn't you watch for a ENOENT result?
		struct stat theIdontCare;
     	theErr = ::stat(theNewNameBuffer, &theIdontCare);
		WarnV((theErr == 0 || OSThread::GetErrno() == ENOENT), "unexpected stat error in RenameLogFile");
		
	}
	
	//rename the file. Use posix rename function
	int result = ::rename(inFileName, theNewNameBuffer);
	if (result == -1)
		theErr = (SInt32)OSThread::GetErrno();
	else
		theErr = 0;
		
	WarnV(theErr == 0 , "unexpected rename error in RenameLogFile");

	
	if (theErr != 0)
		return false;
	else
		return true;	
	

}


Bool16 QTSSRollingLog::DoesFileExist(const char *inPath)
{
	struct stat theStat;
	int theErr = ::stat(inPath, &theStat);
	if (theErr != 0)
		return false;
	else
		return true;
}

time_t QTSSRollingLog::WriteLogHeader(FILE* /*inFile*/)
{
	//The point of this header is to record the exact time the log file was created,
	//in a format that is easy to parse through whenever we open the file again.
	//This is necessary to support log rolling based on a time interval, and POSIX doesn't
	//support a create date in files.
	time_t calendarTime = ::time(NULL);
	Assert(-1 != calendarTime);
	if (-1 == calendarTime)
		return -1;

	struct tm* theLocalTime = ::localtime(&calendarTime);
	Assert(NULL != theLocalTime);
	if (NULL == theLocalTime)
		return -1;

	char tempbuf[1024];
	::strftime(tempbuf, sizeof(tempbuf), "#Log File Created On: %m/%d/%Y %H:%M:%S\n", theLocalTime);
	//::sprintf(tempbuf, "#Log File Created On: %d/%d/%d %d:%d:%d %d:%d:%d GMT\n",
	//			theLocalTime->tm_mon, theLocalTime->tm_mday, theLocalTime->tm_year,
	//			theLocalTime->tm_hour, theLocalTime->tm_min, theLocalTime->tm_sec,
	//			theLocalTime->tm_yday, theLocalTime->tm_wday, theLocalTime->tm_isdst);
	this->WriteToLog(tempbuf, !kAllowLogToRoll);
		
	return calendarTime;
}

time_t QTSSRollingLog::ReadLogHeader(FILE* inFile)
{
	//This function reads the header in a log file, returning the time stored
	//at the beginning of this file. This value is used to determine when to
	//roll the log.
	//Returns -1 if the header is bogus. In that case, just ignore time based log rolling

	//first seek to the beginning of the file
	SInt32 theCurrentPos = ::ftell(inFile);
	if (theCurrentPos == -1)
		return -1;
	(void)::rewind(inFile);

	const UInt32 kMaxHeaderLength = 500;
	char theFirstLine[kMaxHeaderLength];
	
	if (NULL == ::fgets(theFirstLine, kMaxHeaderLength, inFile))
	{
		::fseek(inFile, 0, SEEK_END);
		return -1;
	}
	::fseek(inFile, 0, SEEK_END);
	
	struct tm theFileCreateTime;
	
	// Zero out fields we will not be using
	theFileCreateTime.tm_isdst = -1;
	theFileCreateTime.tm_wday = 0;
	theFileCreateTime.tm_yday = 0;
	
	if (EOF == ::sscanf(theFirstLine, "#Log File Created On: %d/%d/%d %d:%d:%d\n",
				&theFileCreateTime.tm_mon, &theFileCreateTime.tm_mday, &theFileCreateTime.tm_year,
				&theFileCreateTime.tm_hour, &theFileCreateTime.tm_min, &theFileCreateTime.tm_sec))
		return -1;
	
#ifdef __Win32__
	// Win32 has slightly different atime basis than UNIX.
	theFileCreateTime.tm_yday--;
	theFileCreateTime.tm_mon--;
	theFileCreateTime.tm_year -= 1900;
#endif

	//ok, we should have a filled in tm struct. Convert it to a time_t.
	return ::mktime(&theFileCreateTime);
}


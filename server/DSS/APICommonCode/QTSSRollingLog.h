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
	File:		QTSSRollingLog.h

	Contains:	A log toolkit, log can roll either by time or by size, clients
				must derive off of this object ot provide configuration information. 



*/

#ifndef __QTSS_ROLLINGLOG_H__
#define __QTSS_ROLLINGLOG_H__

#include <stdio.h>
#include "OSHeaders.h"

const Bool16 kAllowLogToRoll = true;

class QTSSRollingLog
{
	public:
	
		//pass in whether you'd like the log roller to log errors.
		QTSSRollingLog();
		virtual ~QTSSRollingLog();
		
		void	WriteToLog(char* inLogData, Bool16 allowLogToRoll);
		
		//log rolls automatically based on the configuration criteria,
		//but you may roll the log manually by calling this function.
		//Returns true if no error, false otherwise
		Bool16	RollLog();
		
		//mainly to check and see if errors occurred
		Bool16	IsLogEnabled() { return fLog != NULL; }

		//General purpose utility function
		//returns false if some error has occurred
		static Bool16 	FormatDate(char *ioDateBuffer);
		
		void EnableLog( Bool16 appendDotLog = true );

		enum
		{
			kMaxDateBufferSizeInBytes = 30, //UInt32
			kMaxFilenameLengthInBytes = 31	//UInt32
		};
	
	protected:
	
		//Derived class must provide a way to get the log & rolled log name
		virtual char* GetLogName() = 0;
		virtual char* GetLogDir() = 0;
		virtual UInt32 GetRollIntervalInDays() = 0;//0 means no interval
		virtual UInt32 GetMaxLogBytes() = 0;//0 means unlimited
			
		FILE*			fLog;
		time_t			fLogCreateTime;
		char*			fLogFullPath;

		Bool16		CheckRollLog();//rolls the log if necessary
		void 		CloseLog();
		Bool16 		RenameLogFile(const char* inFileName);
		Bool16 		DoesFileExist(const char *inPath);
		
		//to record the time the file was created (for time based rolling)
		virtual time_t WriteLogHeader(FILE *inFile);
		time_t 		ReadLogHeader(FILE* inFile);

};
#endif // __QTSS_ROLLINGLOG_H__


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
	File:		OS.h

	Contains:  	OS utility functions


		
	
*/

#include <stdlib.h>
#include <string.h>

#ifdef __MACOS__
#include <time.h>
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>

#ifndef __Win32__
#include <sys/time.h>
#endif

#include "OS.h"
#include "OSThread.h"
#include "MyAssert.h"

#if USE_ATOMICLIB
#include "timestamp.h"//for fast milliseconds routine
#endif

#if __MacOSX__
#include <CarbonCore/Timer.h> // Carbon Microseconds
#endif

double 	OS::sDivisor = 0;
double OS::sMicroDivisor = 0;
SInt64  OS::sMsecSince1970 = 0;
SInt64  OS::sMsecSince1900 = 0;
SInt64  OS::sInitialMsec = 0;

#if DEBUG
#include "OSMutex.h"
#include "OSMemory.h"
static OSMutex* sLastMillisMutex = NULL;
#endif

void OS::Initialize()
{
	Assert (sInitialMsec == 0);  // do only once
	if (sInitialMsec != 0) return;
#if USE_ATOMICLIB
	//Setup divisor for milliseconds
	double divisorNumerator = 0;
	double divisorDenominator = 0;
	struct timescale theTimescale;
	
	(void)utimescale(&theTimescale);
	
	divisorNumerator = (double)theTimescale.tsc_numerator;
	divisorDenominator = (double)theTimescale.tsc_denominator;//we want milliseconds, not micro
	sMicroDivisor = divisorNumerator / divisorDenominator;
	divisorDenominator *= 1000;//we want milliseconds, not micro
	sDivisor = divisorNumerator / divisorDenominator;
#endif

	//setup t0 value for msec since 1900

	//t.tv_sec is number of seconds since Jan 1, 1970. Convert to seconds since 1900	
	SInt64 the1900Msec = 24 * 60 * 60;
	the1900Msec *= (70 * 365) + 17;
	sMsecSince1900 = the1900Msec;
	
	sInitialMsec = OS::Milliseconds(); //Milliseconds uses sInitialMsec so this assignment is valid only once.

	sMsecSince1970 = ::time(NULL); 	// POSIX time always returns seconds since 1970
	sMsecSince1970 *= 1000;			// Convert to msec

#if DEBUG
	sLastMillisMutex = NEW OSMutex();
#endif
}

SInt64 OS::Milliseconds()
{
#if USE_ATOMICLIB
	//DMS - here is that superfast way of getting milliseconds. Note that this is time
	//relative to when the machine booted.
	UInt64 theCurTime = timestamp();
	Assert(sDivisor > 0);
	
	double theDoubleTime = (double)theCurTime;
	theDoubleTime *= sDivisor;
	return (((SInt64)theDoubleTime) - sInitialMsec) + sMsecSince1970;
#elif __MacOSX__

#if DEBUG
	OSMutexLocker locker(sLastMillisMutex);
#endif

   UnsignedWide theMicros;
    ::Microseconds(&theMicros);
    SInt64 scalarMicros = theMicros.hi;
    scalarMicros <<= 32;
    scalarMicros += theMicros.lo;
	scalarMicros = ((scalarMicros / 1000) - sInitialMsec) + sMsecSince1970; // convert to msec

#if DEBUG
	static SInt64 sLastMillis = 0;
	//Assert(scalarMicros >= sLastMillis); // currently this fails on dual processor machines
	sLastMillis = scalarMicros;
#endif
	return scalarMicros;

#elif __Win32__
	SInt64 curTimeMilli = (SInt64) ::timeGetTime(); // system time
	return (curTimeMilli - sInitialMsec) + sMsecSince1970; // convert to application time
#else
	struct timeval t;
	struct timezone tz;
	int theErr = ::gettimeofday(&t, &tz);
	Assert(theErr == 0);

	SInt64 curTime;
	curTime = t.tv_sec;
	curTime *= 1000;				// sec -> msec
	curTime += t.tv_usec / 1000;	// usec -> msec

	return (curTime - sInitialMsec) + sMsecSince1970;
#endif

}

SInt64 OS::Microseconds()
{
#if USE_ATOMICLIB
	UInt64 theCurTime = timestamp();	
	double theDoubleTime = (double)theCurTime;
	theDoubleTime *= sMicroDivisor;
	return (SInt64)theDoubleTime;
#elif __MacOSX__
    UnsignedWide theMicros;
    ::Microseconds(&theMicros);
    SInt64 theMillis = theMicros.hi;
    theMillis <<= 32;
    theMillis += theMicros.lo;
    return theMillis;
#elif __Win32__
	SInt64 curTime = (SInt64) ::timeGetTime(); // unsigned long system time in milliseconds
	curTime -= sInitialMsec; // convert to application time
	curTime *= 1000; // convert to microseconds                   
	return curTime;
#else
	struct timeval t;
	struct timezone tz;
	int theErr = ::gettimeofday(&t, &tz);
	Assert(theErr == 0);

	SInt64 curTime;
	curTime = t.tv_sec;
	curTime *= 1000000;		// sec -> usec
	curTime += t.tv_usec;

	return curTime - (sInitialMsec * 1000);
#endif
}

SInt32 OS::GetGMTOffset()
{
#ifdef __Win32__
	TIME_ZONE_INFORMATION tzInfo;
	DWORD theErr = ::GetTimeZoneInformation(&tzInfo);
	if (theErr == TIME_ZONE_ID_INVALID)
		return 0;
	
	return ((tzInfo.Bias / 60) * -1);
#else
	struct timeval	tv;
	struct timezone	tz;

	int err = ::gettimeofday(&tv, &tz);
	if (err != 0)
		return 0;
		
	return ((tz.tz_minuteswest / 60) * -1);//return hours before or after GMT
#endif
}


SInt64	OS::HostToNetworkSInt64(SInt64 hostOrdered)
{
#if BIGENDIAN
	return hostOrdered;
#else
	return (SInt64) (  (UInt64)  (hostOrdered << 56) | (UInt64)  (((UInt64) 0x00ff0000 << 32) & (hostOrdered << 40))
		| (UInt64)  ( ((UInt64)  0x0000ff00 << 32) & (hostOrdered << 24)) | (UInt64)  (((UInt64)  0x000000ff << 32) & (hostOrdered << 8))
		| (UInt64)  ( ((UInt64)  0x00ff0000 << 8) & (hostOrdered >> 8)) | (UInt64)     ((UInt64)  0x00ff0000 & (hostOrdered >> 24))
		| (UInt64)  (  (UInt64)  0x0000ff00 & (hostOrdered >> 40)) | (UInt64)  ((UInt64)  0x00ff & (hostOrdered >> 56)) );
#endif
}

SInt64	OS::NetworkToHostSInt64(SInt64 networkOrdered)
{
#if BIGENDIAN
	return networkOrdered;
#else
	return (SInt64) (  (UInt64)  (networkOrdered << 56) | (UInt64)  (((UInt64) 0x00ff0000 << 32) & (networkOrdered << 40))
		| (UInt64)  ( ((UInt64)  0x0000ff00 << 32) & (networkOrdered << 24)) | (UInt64)  (((UInt64)  0x000000ff << 32) & (networkOrdered << 8))
		| (UInt64)  ( ((UInt64)  0x00ff0000 << 8) & (networkOrdered >> 8)) | (UInt64)     ((UInt64)  0x00ff0000 & (networkOrdered >> 24))
		| (UInt64)  (  (UInt64)  0x0000ff00 & (networkOrdered >> 40)) | (UInt64)  ((UInt64)  0x00ff & (networkOrdered >> 56)) );
#endif
}


OS_Error OS::MakeDir(char *inPath)
{
	struct stat theStatBuffer;
	if (::stat(inPath, &theStatBuffer) == -1)
	{
		//this directory doesn't exist, so let's try to create it
#ifdef __Win32__
		if (::mkdir(inPath) == -1)
#else
		if (::mkdir(inPath, S_IRWXU) == -1)
#endif
			return (OS_Error)OSThread::GetErrno();
	}
#ifdef __Win32__
	else if (!(theStatBuffer.st_mode & _S_IFDIR)) // MSVC++ doesn't define the S_ISDIR macro
		return EEXIST; // there is a file at this point in the path!
#else
	else if (!S_ISDIR(theStatBuffer.st_mode))
		return EEXIST;//there is a file at this point in the path!
#endif

	//directory exists
	return OS_NoErr;
}

OS_Error OS::RecursiveMakeDir(char *inPath)
{
	Assert(inPath != NULL);
	
	//iterate through the path, replacing '/' with '\0' as we go
	char *thePathTraverser = inPath;
	
	//skip over the first / in the path.
	if (*thePathTraverser == kPathDelimiterChar)
		thePathTraverser++;
		
	while (*thePathTraverser != '\0')
	{
		if (*thePathTraverser == kPathDelimiterChar)
		{
			//we've found a filename divider. Now that we have a complete
			//filename, see if this partial path exists.
			
			//make the partial path into a C string
			*thePathTraverser = '\0';
			OS_Error theErr = MakeDir(inPath);
			//there is a directory here. Just continue in our traversal
			*thePathTraverser = kPathDelimiterChar;

			if (theErr != OS_NoErr)
				return theErr;
		}
		thePathTraverser++;
	}
	
	//need to create the last directory in the path
	return MakeDir(inPath);
}

UInt32	OS::GetNumProcessors()
{
#ifdef __Win32__
	SYSTEM_INFO theSystemInfo;
	::GetSystemInfo(&theSystemInfo);
	
	return (UInt32)theSystemInfo.dwNumberOfProcessors;
#else
	return 1;
#endif
}


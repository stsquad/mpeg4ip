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
	File:		osfile.cpp

	Contains:	simple file abstraction
					
	
	
	
*/

#include <stdio.h>
#include <string.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

#ifndef __Win32__
#include <unistd.h>
#endif

#include "OSFileSource.h"
#include "OSMemory.h"
#include "OSThread.h"

#if __MACOS__
#include "OSArrayObjectDeleter.h"
#endif


void OSFileSource::Set(const char *inPath)
{
	Close();

#ifdef __MacOSXServer__
	fFile = open(inPath, O_RDONLY | O_NO_MFS);
#elif __Win32__
	fFile = open(inPath, O_RDONLY | O_BINARY);
#else
	fFile = open(inPath, O_RDONLY);
#endif

	if (fFile != -1)
	{
		struct stat buf;
		if (::fstat(fFile, &buf) >= 0)
		{
			fLength = buf.st_size;
			fModDate = buf.st_mtime;
#ifdef __Win32__
			fIsDir = buf.st_mode & _S_IFDIR;
#else
			fIsDir = S_ISDIR(buf.st_mode);
#endif
		}
		else
			this->Close();
	}	
}

void OSFileSource::Advise(UInt64 advisePos, UInt32 adviseAmt)
{
#if __MacOSXServer__
	//if the caller wants us to, issue an advise. Advise for the file area
	//immediately following the current read area
	struct radvisory radv;
	memset(&radv, 0, sizeof(radv));
	radv.ra_offset = (off_t)advisePos;
	radv.ra_count = (int)adviseAmt;
	::fcntl(fFile, F_RDADVISE, &radv);
#endif
}

OS_Error	OSFileSource::Read(void* inBuffer, UInt32 inLength, UInt32* outRcvLen)
{
		if (lseek(fFile, fPosition, SEEK_SET) == -1)
			return OSThread::GetErrno();
		
	int rcvLen = ::read(fFile, (char*)inBuffer, inLength);
	if (rcvLen == -1)
		return OSThread::GetErrno();

	if (outRcvLen != NULL)
		*outRcvLen = rcvLen;

	fPosition += rcvLen;
	fReadPos = fPosition;
	return OS_NoErr;
}

OS_Error	OSFileSource::Read(UInt64 inPosition, void* inBuffer, UInt32 inLength, UInt32* outRcvLen)
{
    this->Seek(inPosition);
    return this->Read(inBuffer,inLength,outRcvLen);
}


void	OSFileSource::Close()
{
	if ((fFile != -1) && (fShouldClose))
		::close(fFile);
	fFile = -1;
	fModDate = 0;
	fLength = 0;
	fPosition = 0;
	fReadPos = 0;
}


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
	File:		StringFormatter.cpp

	Contains:	Implementation of StringFormatter class.  
					
	
	
	
*/

#include <string.h>
#include "StringFormatter.h"
#include "MyAssert.h"

char*	StringFormatter::sEOL = "\r\n";
UInt32	StringFormatter::sEOLLen = 2;

void StringFormatter::Put(const SInt32 num)
{
	char buff[32];
	sprintf(buff, "%ld", num);
	Put(buff);
}

void StringFormatter::Put(char* buffer, UInt32 bufferSize)
{
	if((bufferSize == 1) && (fCurrentPut != fEndPut)) {
		*(fCurrentPut++) = *buffer;
		fBytesWritten++;
		return;
	}		
		
	//loop until the input buffer size is smaller than the space in the output
	//buffer. Call BufferIsFull at each pass through the loop
	UInt32 spaceInBuffer = 0;
	while (((spaceInBuffer = (this->GetSpaceLeft() - 1)) < bufferSize) || (this->GetSpaceLeft() == 0))
	{
		if (this->GetSpaceLeft() > 0)
		{
			::memcpy(fCurrentPut, buffer, spaceInBuffer);
			fCurrentPut += spaceInBuffer;
			fBytesWritten += spaceInBuffer;
			buffer += spaceInBuffer;
			bufferSize -= spaceInBuffer;
		}
		this->BufferIsFull(fStartPut, this->GetCurrentOffset());
	}
	
	//copy the remaining chunk into the buffer
	::memcpy(fCurrentPut, buffer, bufferSize);
	fCurrentPut += bufferSize;
	fBytesWritten += bufferSize;
}


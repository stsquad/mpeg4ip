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
	File:		osfilesource.h

	Contains:	simple file abstraction. This file abstraction is ONLY to be
				used for files intended for serving 
					
	
*/

#ifndef __OSFILE_H_
#define __OSFILE_H_

#include <stdio.h>
#include <time.h>

#include "OSHeaders.h"
#include "StrPtrLen.h"


class OSFileSource
{
	public:
	
		OSFileSource() : fFile(-1), fLength(0), fPosition(0), fReadPos(0), fShouldClose(true), fIsDir(false) {}
				
		OSFileSource(const char *inPath) : 	fFile(-1), fShouldClose(true), fIsDir(false) { Set(inPath); }
			
		~OSFileSource() { Close(); }
		
		//Sets this object to reference this file
		void 			Set(const char *inPath);
		
		// Call this if you don't want Close or the destructor to close the fd
		void			DontCloseFD() { fShouldClose = false; }
		
		//Advise: this advises the OS that we are going to be reading soon from the
		//following position in the file
		void			Advise(UInt64 advisePos, UInt32 adviseAmt);

		OS_Error	Read(void* inBuffer, UInt32 inLength, UInt32* outRcvLen = NULL);
		OS_Error	Read(UInt64 inPosition, void* inBuffer, UInt32 inLength,
								UInt32* outRcvLen = NULL);
		
		void		Close();
		
		time_t			GetModDate()				{ return fModDate; }
		UInt64			GetLength() 				{ return fLength; }
		UInt64			GetCurOffset()				{ return fPosition; }
		void			Seek(SInt64 newPosition) 	{ fPosition = newPosition; 	}
		Bool16 IsValid() 								{ return fFile != -1; 		}
		Bool16 IsDir()								{ return fIsDir; }
		
		// For async I/O purposes
		int				GetFD()						{ return fFile; }
		
		// So that close won't do anything
		void ResetFD()	{ fFile=-1; }

	private:

		int		fFile;
		UInt64	fLength;
		UInt64	fPosition;
		UInt64	fReadPos;
		Bool16	fShouldClose;
		Bool16	fIsDir;
		time_t	fModDate;
};

#endif //__OSFILE_H_

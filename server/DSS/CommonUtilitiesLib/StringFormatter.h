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
	File:		StringFormatter.h

	Contains:	Utility class for formatting text to a buffer.
				Construct object with a buffer, then call one
				of many Put methods to write into that buffer.
					
	

*/

#ifndef __STRINGFORMATTER_H__
#define __STRINGFORMATTER_H__

#include <string.h>
#include "StrPtrLen.h"
#include "MyAssert.h"


class StringFormatter
{
	public:
		
		//pass in a buffer and length for writing
		StringFormatter(char *buffer, UInt32 length) : 	fCurrentPut(buffer), 
														fStartPut(buffer),
														fEndPut(buffer + length),
														fBytesWritten(0) {}

		StringFormatter(StrPtrLen &buffer) : 			fCurrentPut(buffer.Ptr),
														fStartPut(buffer.Ptr),
														fEndPut(buffer.Ptr + buffer.Len),
														fBytesWritten(0) {}
		virtual ~StringFormatter() {}
		
		//"erases" all data in the output stream save this number
		void		Reset(UInt32 inNumBytesToLeave = 0)
			{ fCurrentPut = fStartPut + inNumBytesToLeave; }

		//Object does no bounds checking on the buffer. That is your responsibility!
		void	 	Put(const SInt32 num);
		void		Put(char* buffer, UInt32 bufferSize);
		void		Put(char* str) 		{ Put(str, strlen(str)); }
		void		Put(const StrPtrLen &str) { Put(str.Ptr, str.Len); }
		void		PutSpace() 			{ PutChar(' '); }
		void		PutEOL() 			{  Put(sEOL, sEOLLen); }
		void		PutChar(char c)		{ Put(&c, 1); }
		void		PutTerminator()		{ PutChar('\0'); }
			
		inline UInt32		GetCurrentOffset();
		inline UInt32		GetSpaceLeft();
		inline UInt32		GetTotalBufferSize();
		char*				GetCurrentPtr()		{ return fCurrentPut; }
		char*				GetBufPtr()			{ return fStartPut; }
		
		// Counts total bytes that have been written to this buffer (increments
		// even when the buffer gets reset)
		void				ResetBytesWritten()	{ fBytesWritten = 0; }
		UInt32				GetBytesWritten()	{ return fBytesWritten; }
			
	protected:

		//If you fill up the StringFormatter buffer, this function will get called. By
		//default, no action is taken. But derived objects can clear out the data and reset the buffer
		virtual void	BufferIsFull(char* /*inBuffer*/, UInt32 /*inBufferLen*/) { Assert(0); }

		char*		fCurrentPut;
		char*		fStartPut;
		char*		fEndPut;
		
		// A way of keeping count of how many bytes have been written total
		UInt32 fBytesWritten;

		static char*	sEOL;
		static UInt32	sEOLLen;
};

inline UInt32 StringFormatter::GetCurrentOffset()
{
	Assert(fCurrentPut >= fStartPut);
	return (UInt32)(fCurrentPut - fStartPut);
}

inline UInt32 StringFormatter::GetSpaceLeft()
{
	Assert(fEndPut >= fCurrentPut);
	return (UInt32)(fEndPut - fCurrentPut);
}

inline UInt32 StringFormatter::GetTotalBufferSize()
{
	Assert(fEndPut >= fStartPut);
	return (UInt32)(fEndPut - fStartPut);
}
#endif // __STRINGFORMATTER_H__


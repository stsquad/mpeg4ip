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

// $Id: QTFile_FileControlBlock.cpp,v 1.3 2001/10/11 20:39:08 wmaycisco Exp $
//
// QTFile:
//   The central point of control for a file in the QuickTime File Format.


// -------------------------------------
// Includes
//
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "OSMutex.h"
#include "OSMemory.h"

#include "QTFile.h"

#include "QTAtom.h"
#include "QTAtom_mvhd.h"
#include "QTAtom_tkhd.h"

#include "QTTrack.h"
#include "QTHintTrack.h"

#include <fcntl.h>



// -------------------------------------
// Macros
//
//#define DEBUG_PRINT(s) if(fDebug) printf s
//#define DEEP_DEBUG_PRINT(s) if(fDeepDebug) printf s



// -------------------------------------
// Class state cookie
//
QTFile_FileControlBlock::QTFile_FileControlBlock(void)
	: fDataBufferPool(NULL),
	  fDataBufferSize(0), fDataBufferPosStart(0), fDataBufferPosEnd(0),
	  fCurrentDataBuffer(NULL), fPreviousDataBuffer(NULL),
	  fCurrentDataBufferLength(0), fPreviousDataBufferLength(0)
	  
{
#if DSS_SERVER_INTERNAL
	fDataFD = NULL;
#endif
	fDataBufferSize = QTFile::kDefaultDataBufferUnits * (1 << kDataBufferUnitSizeExp);
	fDataBufferPool = NEW char[2 * fDataBufferSize];
	if( fDataBufferPool == NULL )
		fDataBufferSize = 0;

	fCurrentDataBuffer = fDataBufferPool;
	fCurrentDataBufferLength = 0;
	fPreviousDataBuffer = (char *)fDataBufferPool + fDataBufferSize;
	fPreviousDataBuffer = 0;
}

QTFile_FileControlBlock::~QTFile_FileControlBlock(void)
{
	if( fDataBufferPool != NULL )
		delete[] fDataBufferPool;
#if DSS_SERVER_INTERNAL
	(void)QTSS_CloseFileObject(fDataFD);
#endif
}


void QTFile_FileControlBlock::Set(const char * DataPath)
{
#if DSS_SERVER_INTERNAL
	(void)QTSS_OpenFileObject((char*)DataPath, qtssOpenFileReadAhead, &fDataFD);
#else
    fDataFD.Set(DataPath);
#endif
}

#if DSS_SERVER_INTERNAL
Bool16 QTFile_FileControlBlock::Read(QTSS_Object *dflt, UInt64 inPosition, void* inBuffer, UInt32 inLength)
#else
Bool16 QTFile_FileControlBlock::Read(OSFileSource *dflt, UInt64 inPosition, void* inBuffer, UInt32 inLength)
#endif
{
    // Temporary vars
    UInt32 rcSize;

    // General vars
#if DSS_SERVER_INTERNAL
    QTSS_Object		*DataFD;
    QTSS_Error		theErr = QTSS_NoErr;
#else
    OSFileSource	*DataFD;
#endif

    //
    // Get the file descriptor.  If the FCB is NULL, or the descriptor in
    // the FCB is -1, then we need to use the class' descriptor.
    if (this->IsValid())
		DataFD = &fDataFD;
    else
		DataFD = dflt;

    //
    // Do we have a buffer?  Is this read larger than our buffer size?  If
    // either of these statements is true, then we need to do the read
    // directly, bypassing the buffer entirely.
    if( inLength > fDataBufferSize ) {
		//DEEP_DEBUG_PRINT(("QTFile::Read - Read request for Length > DataBufferSize.\n"));

		//
		// Seek, read and return.
#if DSS_SERVER_INTERNAL
		theErr = QTSS_Seek(*DataFD, inPosition);
		if (theErr == QTSS_NoErr)
			theErr = QTSS_Read(*DataFD, inBuffer, inLength, &rcSize);
		if (theErr != QTSS_NoErr)
			return false;
#else
		if( DataFD->Read(inPosition, inBuffer, inLength, &rcSize) != OS_NoErr )
		    return false;
#endif
		if( rcSize != inLength )
		    return false;
		return true;
    }

    //
    // Is the requested block of data in our data buffer?  If not, read in the
    // section of the file where this piece of data is.
    if( (inPosition < fDataBufferPosStart) || ((inPosition + inLength) > fDataBufferPosEnd) ) {
	
	//DEEP_DEBUG_PRINT(("QTFile::Read - Reading new buffer from disk (offset=%"_64BITARG_"u).\n", Offset));
#if 0
#if HAVE_64BIT_PRINTF
#if HAVE_64BIT_PRINTF_AS_LL	
		//DEEP_DEBUG_PRINT(("QTFile::Read - Reading new buffer from disk (offset=%llu).\n", Offset));
#else
		//DEEP_DEBUG_PRINT(("QTFile::Read - Reading new buffer from disk (offset=%qu).\n", Offset));
#endif
#else
		//DEEP_DEBUG_PRINT(("QTFile::Read - Reading new buffer from disk (offset=%lu).\n", (UInt32)Offset));
#endif
#endif
		//
		// If we're moving backwards, we have to do some extra work to make
		// sure that we don't grab the wrong data later on.
		if( inPosition < fDataBufferPosStart ) {
	#if HAVE_64BIT_PRINTF
	#if HAVE_64BIT_PRINTF_AS_LL
			//DEEP_DEBUG_PRINT(("QTFile::Read - ..File pointer moved backward (DataBufferPosStart=%llu).\n", fDataBufferPosStart));
	#else
			//DEEP_DEBUG_PRINT(("QTFile::Read - ..File pointer moved backward (DataBufferPosStart=%qu).\n", fDataBufferPosStart));
	#endif
	#else
			//DEEP_DEBUG_PRINT(("QTFile::Read - ..File pointer moved backward (DataBufferPosStart=%lu).\n", (UInt32)fDataBufferPosStart));
	#endif
		}
			
		//
		// If this is a forward-moving, contiguous read, then we can keep the
		// current buffer around.
		if(    (fCurrentDataBufferLength != 0)
		       && ((fDataBufferPosEnd - fCurrentDataBufferLength) <= inPosition)
		       && ((fDataBufferPosEnd + fDataBufferSize) >= (inPosition + inLength))
		    ) {
		    // Temporary vars
		    char		*TempDataBuffer;
				
				
		    //
		    // First, demote the current buffer.
		    fDataBufferPosStart += fPreviousDataBufferLength;

		    TempDataBuffer = fPreviousDataBuffer;
				
		    fPreviousDataBuffer = fCurrentDataBuffer;
		    fPreviousDataBufferLength = fCurrentDataBufferLength;
				
		    fCurrentDataBuffer = TempDataBuffer;
		    fCurrentDataBufferLength = 0;
				
		    //
		    // Then, fill the now-current buffer with data.
#if DSS_SERVER_INTERNAL
			theErr = QTSS_Seek(*DataFD, fDataBufferPosEnd);
			if (theErr == QTSS_NoErr)
				theErr = QTSS_Read(*DataFD, fCurrentDataBuffer, fDataBufferSize, &rcSize);
			if (theErr != QTSS_NoErr)
				return false;
#else
		    if( DataFD->Read(fDataBufferPosEnd,fCurrentDataBuffer,fDataBufferSize,&rcSize)!=OS_NoErr)
				return false;
#endif
		    if( rcSize <=0 )
				return false;
			//Assert(rcSize == fDataBufferSize);
		    fCurrentDataBufferLength = (UInt32)rcSize;
		    fDataBufferPosEnd += fCurrentDataBufferLength;
		} else {
		    //
		    // We need to play with our current and previous data buffers in
		    // order to skip around while reading.
		    fCurrentDataBuffer = fDataBufferPool;
		    fCurrentDataBufferLength = 0;

		    fPreviousDataBuffer = (char *)fDataBufferPool + fDataBufferSize;
		    fPreviousDataBufferLength = 0;

		    fDataBufferPosStart = inPosition;
#if DSS_SERVER_INTERNAL
			theErr = QTSS_Seek(*DataFD, fDataBufferPosStart);
			if (theErr == QTSS_NoErr)
				theErr = QTSS_Read(*DataFD, fCurrentDataBuffer, fDataBufferSize, &rcSize);
			if (theErr != QTSS_NoErr)
				return false;
#else
		    if( DataFD->Read( fDataBufferPosStart, fCurrentDataBuffer, fDataBufferSize, &rcSize ) != OS_NoErr )
				return false;
#endif
			//Assert(rcSize == fDataBufferSize);
		    fCurrentDataBufferLength = (UInt32)rcSize;
		    fDataBufferPosEnd = fDataBufferPosStart + fCurrentDataBufferLength;
		}

		//
		// Advise the I/O Subsystem that we will soon be reading the block after
		// the one that we just saw.
#if DSS_SERVER_INTERNAL
		(void)QTSS_Advise(*DataFD, fDataBufferPosEnd, fDataBufferSize);
#else
		DataFD->Advise(fDataBufferPosEnd,fDataBufferSize);
#endif
    }
	
    //
    // Copy the data out of our buffer(s).
    {
	// General vars
	UInt32		ReadLength = inLength;
	UInt32		ReadOffset = inPosition - fDataBufferPosStart;
		
		
	//
	// Figure out if doing a continuous copy would cause us to cross a
	// buffer boundary.
	if(    (inPosition < (fDataBufferPosStart + fPreviousDataBufferLength))
	       && (ReadOffset + ReadLength) > fPreviousDataBufferLength ) {
	    // Temporary vars
	    char		*pBuffer = (char *)inBuffer;
			
			
	    //
	    // Read the first part of the block.
	    ReadLength = fDataBufferSize - ReadOffset;
	    memcpy(pBuffer, fPreviousDataBuffer + ReadOffset, ReadLength);
	    pBuffer += ReadLength;
			
	    //
	    // Read the last part of the block.
	    ReadLength = inLength - ReadLength;
	    memcpy(pBuffer, fCurrentDataBuffer, ReadLength);
		
	    //
	    // Or maybe this is a continuous copy out of the old buffer.
	} else if( inPosition < (fDataBufferPosEnd - fCurrentDataBufferLength) ) {
	    memcpy(inBuffer, fPreviousDataBuffer + ReadOffset, ReadLength);

	    //
	    // Or perhaps the current one.
	} else {
	    ReadOffset -= fPreviousDataBufferLength;
	    memcpy(inBuffer, fCurrentDataBuffer + ReadOffset, ReadLength);
	}
    }

    //
    // We're done.
    return true;
}

//
// Buffer management functions
void QTFile_FileControlBlock::AdjustDataBuffer(UInt32 BufferSize)
{
	// General vars
	UInt32		BytesPerBuffer = BufferSize;
	
	char		*NewDataBufferPool;
	UInt32		NewDataBufferSizeInUnits, NewDataBufferSize;
	
	
	//
	// Limit the buffer size value to a reasonable maximum.
	if( BytesPerBuffer > 262144 )
		BytesPerBuffer = 262144;
	
	//
	// Convert this value to a buffer size.
	NewDataBufferSizeInUnits = (BytesPerBuffer & ~((1 << kDataBufferUnitSizeExp) - 1)) >> kDataBufferUnitSizeExp;
	if( NewDataBufferSizeInUnits == 0 )
		NewDataBufferSizeInUnits = 1;
	NewDataBufferSize = NewDataBufferSizeInUnits * (1 << kDataBufferUnitSizeExp);
	
	//
	// Adjust our data buffer if this new size is greater/less than the previous
	// size.
	if( NewDataBufferSize != fDataBufferSize ) {
		//
		// Create the new buffer.  We do this first just in case we are running
		// out of memory.
		NewDataBufferPool = NEW char[2 * NewDataBufferSize];
		if( NewDataBufferPool == NULL )
			return;

		//
		// Free the old buffer.
		if( fDataBufferPool != NULL ) {
			delete[] fDataBufferPool;
			fDataBufferPool = NULL;
		}
		
		//
		// Migrate the new buffer.
		fDataBufferPool = NewDataBufferPool;

		fDataBufferSize = NewDataBufferSize;
		fDataBufferPosStart = fDataBufferPosEnd = 0;

		fCurrentDataBuffer = fDataBufferPool;
		fCurrentDataBufferLength = 0;
		fPreviousDataBuffer = (char *)fDataBufferPool + fDataBufferSize;
		fPreviousDataBuffer = 0;
	}
}

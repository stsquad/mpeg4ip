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
	File:		RTPFileSession.h

	Contains:	
					
	Revision 1.2  2001/10/02 18:50:53  dmackie
	Newlines again
	Revert to original RTSPSession.cpp
	
	Revision 1.1  2001/10/01 22:08:35  dmackie
	DSS 3.0.1 import
	
	Revision 1.2  2001/03/20 02:19:19  mythili
	updated/added APSL 1.2
	
	Revision 1.1  2000/10/20 07:24:52  serenyi
	Created
	
	
*/

#ifndef __RTPFILESESSIONH__
#define __RTPFILESESSIONH__

#include "RTPFileDefs.h"
#include "OSHeaders.h"
#include "OSRef.h"
#include "QTSS.h" // This object uses QTSS API file I/O
#include "SDPSourceInfo.h"

class RTPFile;

class RTPFileSession
{
	public:

		// One per client
		
		RTPFileSession();
		~RTPFileSession();
		
		//
		// Class error codes
		enum ErrorCode
		{
			errNoError					= 0,
			errFileNotFound				= 1,
			errTrackAlreadyAdded		= 2,
			errTrackDoesntExist			= 3,
			errSeekToNonexistentTime	= 4
		};

		//
		// INITIALIZE
		ErrorCode	Initialize(const StrPtrLen& inFilePath, Float32 inBufferSeconds);
		
		//
		// ACCESSORS
		
		//
		// Global information
		inline Float64			GetMovieDuration();
		inline StrPtrLen*		GetSDPFile();
		inline SourceInfo*		GetSourceInfo();
		inline StrPtrLen*		GetMoviePath();
		UInt64 GetAddedTracksRTPBytes()	{ return fAddedTracksRTPBytes; }
		
		
		//
		// Track functions
		inline Float64		GetTrackDuration(UInt32 inTrackID);
		inline UInt32		GetTrackTimeScale(UInt32 inTrackID);

		UInt16		GetNextTrackSequenceNumber(UInt32 inTrackID);
		UInt32		GetSeekTimestamp(UInt32 TrackID);
		
		//
		// MODIFIERS
		
		//
		// Track modifiers
		ErrorCode	AddTrack(UInt32 inTrackID);
		void		SetTrackSSRC(UInt32 inTrackID, UInt32 inSSRC) 	{ fTrackInfo[inTrackID].fSSRC = inSSRC; }
		void		SetTrackCookie(UInt32 inTrackID, void *inCookie){ fTrackInfo[inTrackID].fCookie = inCookie; }
		
		// Seek to a time
		ErrorCode	Seek(Float64 inTime);

		// GetNextPacket. Returns the transmit time for this packet
		Float64		GetNextPacket(UInt8** outPacket, UInt32* outPacketLength, void** outCookie);

	private:
		
		// Utility functions
		void SkipToNextPacket(RTPFilePacket* inCurPacket);
		void ReadAndAdvise();
		UInt32 PowerOf2Floor(UInt32 inNumToFloor);
		
		enum
		{
			kMaxDataBufferSize = 262144
		};

		struct RTPFileSessionTrackInfo
		{
			Bool16	fEnabled;
			Bool16	fMarked; // is the seek timestamp, seq num recorded?
			UInt32	fSSRC;
			UInt32	fSeekTimestamp;
			UInt16	fSeekSeqNumber;
			void*	fCookie;
		};
		
		QTSS_Object					fFileSource;
		//OSFileSource 				fFileSource;
		UInt64						fFileLength;
		UInt64						fCurrentPosition;
		
		RTPFile*					fFile;
		RTPFileSessionTrackInfo*	fTrackInfo;
		UInt32						fNumTracksEnabled;
		
		UInt8*						fReadBuffer; 	// Buffer that file data gets read into.
													// Usually same as fDataBuffer
		UInt32						fReadBufferOffset;
		
		UInt8*						fDataBuffer; // Buffer for file data
		UInt32						fDataBufferSize;
		UInt32						fDataBufferLen;
		UInt8*						fCurrentPacket;
		UInt64						fAddedTracksRTPBytes;
		
		friend class RTPFile;
};



class RTPFile
{
	public:

		// One per file
		
		RTPFile();
		~RTPFile();
		
		RTPFileSession::ErrorCode	Initialize(const StrPtrLen& inFilePath);

		Float64		GetMovieDuration() 	{ return fHeader.fMovieDuration; }
		
		Float64		GetTrackDuration(UInt32 inTrackID);
		UInt32		GetTrackTimeScale(UInt32 inTrackID);
		UInt32		GetTrackBytes(UInt32 inTrackID);
		Bool16		TrackExists(UInt32 inTrackID);
		UInt32		GetBytesPerSecond() { return fBytesPerSecond; }
		UInt32		GetMaxTrackNumber() { return fMaxTrackNumber; }

		StrPtrLen*	GetSDPFile()	{ return &fSDPData; }
		SourceInfo*	GetSourceInfo()	{ return &fSourceInfo; }
		OSRef*		GetRef()		{ return &fRef; }
		
		// Returns the location in the file corresponding to this time,
		// rounded to the nearest start of block.
		SInt64		GetBlockLocation(Float64 inTimeInSecs);

	private:
		
		RTPFileTrackInfo*	fTrackInfo;
		RTPFileHeader		fHeader;
		UInt8*				fBlockMap;
		StrPtrLen			fSDPData;
		SDPSourceInfo		fSourceInfo;
		UInt32				fBytesPerSecond;
		UInt32				fMaxTrackNumber;
		
		OSRef 		fRef;
		StrPtrLen 	fFilePath;

		friend class RTPFileSession;
};


inline StrPtrLen*	RTPFileSession::GetSDPFile()
{
	return fFile->GetSDPFile();
}

inline SourceInfo* RTPFileSession::GetSourceInfo()
{
	return &fFile->fSourceInfo;
}

inline StrPtrLen* RTPFileSession::GetMoviePath()
{
	return &fFile->fFilePath;
}

inline Float64	RTPFileSession::GetMovieDuration() 
{
	return fFile->fHeader.fMovieDuration;
}

inline Float64	RTPFileSession::GetTrackDuration(UInt32 inTrackID)
{
	return fFile->GetTrackDuration(inTrackID);
}

inline UInt32	RTPFileSession::GetTrackTimeScale(UInt32 inTrackID)
{
	return fFile->GetTrackTimeScale(inTrackID);
}


#endif


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
// $Id: QTRTPFile.h,v 1.2 2001/05/09 21:04:37 cahighlander Exp $
//
// QTRTPFile:
//   An interface to QTFile for TimeShare.

#ifndef QTRTPFile_H
#define QTRTPFile_H


//
// Includes
#include "OSHeaders.h"
#include "MyAssert.h"
#include "RTPMetaInfoPacket.h"

#ifndef __Win32__
#include <sys/stat.h>
#endif

//
// Constants
#define QTRTPFILE_MAX_PACKET_LENGTH		2048


//
// QTRTPFile class
class OSMutex;

class QTFile;
class QTFile_FileControlBlock;
class QTHintTrack;
class QTHintTrack_HintTrackControlBlock;

class QTRTPFile {

public:
	//
	// Class error codes
	enum ErrorCode {
		errNoError					= 0,
		errFileNotFound				= 1,
		errInvalidQuickTimeFile		= 2,
		errNoHintTracks				= 3,
		errTrackIDNotFound			= 4,
		errInternalError			= 100
	};


	//
	// Class typedefs.
	struct RTPFileCacheEntry {
		//
		// Init mutex (do not use this entry until you have acquired and
		// released this.
		OSMutex		*InitMutex;
		
		//
		// File information
	    char*		fFilename;
		QTFile		*File;
		
		//
		// Reference count for this cache entry
		int			ReferenceCount; 
		
		//
		// List pointers
		RTPFileCacheEntry	*PrevEntry, *NextEntry;
	};
	
	struct RTPTrackListEntry {
		//
		// Track information
		UInt32			TrackID;
		QTHintTrack		*HintTrack;
		QTHintTrack_HintTrackControlBlock	*HTCB;
		Bool16			IsTrackActive, IsPacketAvailable;
		UInt32			QualityLevel;
		
		//
		// Server information
		void			*Cookie;
		UInt32			SSRC;
		UInt16			FileSequenceNumberRandomOffset, BaseSequenceNumberRandomOffset,
						LastSequenceNumber;
		SInt32			SequenceNumberAdditive;
		UInt32			FileTimestampRandomOffset, BaseTimestampRandomOffset;

		//
		// Sample/Packet information
		UInt32			CurSampleNumber;
		UInt16			NumPacketsInThisSample, CurPacketNumber;

		Float64			CurPacketTime;
		char			CurPacket[QTRTPFILE_MAX_PACKET_LENGTH];
		UInt32			CurPacketLength;

		//
		// List pointers
		RTPTrackListEntry	*NextTrack;
	};


public:
	//
	// Global initialize function; CALL THIS FIRST!
	static void			Initialize(void);
	
	//
	// Returns a static array of the RTP-Meta-Info fields supported by QTFileLib.
	// It also returns field IDs for the fields it recommends being compressed.
	static const RTPMetaInfoPacket::FieldID*		GetSupportedRTPMetaInfoFields() { return kMetaInfoFields; }
	
	//
	// Constructors and destructor.
						QTRTPFile(Bool16 Debug = false, Bool16 DeepDebug = false);
	virtual				~QTRTPFile(void);


	//
	// Initialization functions.
	virtual	ErrorCode	Initialize(const char * FilePath);
	
	//
	// Accessors
			Float64		GetMovieDuration(void);
			UInt64		GetAddedTracksRTPBytes(void);
	virtual	char *		GetSDPFile(int * SDPFileLength);
			UInt32 		GetBytesPerSecond(void);

			char*		GetMoviePath();
			QTFile*		GetQTFile() { return fFile; }
	
	//
	// Track functions
	
			//
			// AddTrack
			//
			// If you would like this track to be an RTP-Meta-Info stream, pass in
			// the field names you would like to see
			ErrorCode	AddTrack(UInt32 TrackID, Bool16 UseRandomOffset = true);


			Float64		GetTrackDuration(UInt32 TrackID);
			UInt32		GetTrackTimeScale(UInt32 TrackID);
			
			void		SetTrackSSRC(UInt32 TrackID, UInt32 SSRC);
			void		SetTrackCookie(UInt32 TrackID, void * Cookie);
			
			//
			// If you want QTRTPFile to output an RTP-Meta-Info packet instead
			// of a normal RTP packet for this track, call this function and
			// pass in a proper Field ID array (see RTPMetaInfoPacket.h) to
			// tell QTRTPFile which fields to include and which IDs to use with the fields.
			// You have to let this function know whether this is a video track or not.
			void		SetTrackRTPMetaInfo(UInt32 TrackID, RTPMetaInfoPacket::FieldID* inFieldArray, Bool16 isVideo );

			//
			// What sort of packets do you want?
			enum
			{
				kAllPackets = 0,
				kNoBFrames = 1,
				kKeyFramesOnly = 2,
				kEveryOtherKeyFrame = 3,
				kEveryFourthKeyFrame = 4
				//this continues until infinity...
			};
			
			void		SetTrackQualityLevel(UInt32 TrackID, UInt32 inNewQualityLevel);

	//
	// Packet functions
	virtual	ErrorCode	Seek(Float64 Time, Float64 MaxBackupTime = 3.0);
	virtual	UInt32		GetSeekTimestamp(UInt32 TrackID);

	virtual	UInt16		GetNextTrackSequenceNumber(UInt32 TrackID);
	virtual	Float64		GetNextPacket(char ** Packet, int * PacketLength, void ** Cookie);

	virtual Float64		GetFirstPacketTransmissionTime() { Assert( !fIsFirstPacket );  /*must read first packet before fFirstPacketTransmissionTime is valid*/	return	fFirstPacketTransmissionTime; }

			ErrorCode	Error() { return fErr; };
protected:
	//
	// Protected cache functions and variables.
	static	OSMutex				*gFileCacheMutex, *gFileCacheAddMutex;
	static	RTPFileCacheEntry	*gFirstFileCacheEntry;
	
	static	ErrorCode	new_QTFile(const char * FilePath, QTFile ** File, Bool16 Debug = false, Bool16 DeepDebug = false);
	static	void		delete_QTFile(QTFile * File);

	static	void		AddFileToCache(const char *inFilename, QTRTPFile::RTPFileCacheEntry ** NewListEntry);
	static	Bool16		FindAndRefcountFileCacheEntry(const char *inFilename, QTRTPFile::RTPFileCacheEntry **CacheEntry);

	//
	// Protected member functions.
			Bool16		FindTrackEntry(UInt32 TrackID, RTPTrackListEntry **TrackEntry);
			Bool16		PrefetchNextPacket(RTPTrackListEntry * TrackEntry, Bool16 DoSeek = false, Float64 Time = 0.0);

	//
	// Protected member variables.
	Bool16				fDebug, fDeepDebug;

	QTFile				*fFile;
	QTFile_FileControlBlock	*fFCB;
	
	UInt32				fNumHintTracks;
	RTPTrackListEntry	*fFirstTrack, *fLastTrack;
	
	char				*fSDPFile;
	UInt32				fSDPFileLength;
	
	Float64				fRequestedSeekTime, fSeekTime;

	RTPTrackListEntry	*fLastPacketTrack;
	
	UInt32				fBytesPerSecond;
	
	Float64				fFirstPacketTransmissionTime;
	Bool16				fIsFirstPacket;
	ErrorCode			fErr;
	
	
	static const RTPMetaInfoPacket::FieldID kMetaInfoFields[];
};

#endif // QTRTPFile

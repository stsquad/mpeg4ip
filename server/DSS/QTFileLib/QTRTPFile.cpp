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
// $Id: QTRTPFile.cpp,v 1.1 2001/02/27 00:56:49 cahighlander Exp $
//
// QTRTPFile:
//   An interface to QTFile for TimeShare.
//
//  htons and friends are macros and should not include the global specifier ::

// -------------------------------------
// Includes
//
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "OSMutex.h"

#include "QTFile.h"

#include "QTTrack.h"
#include "QTHintTrack.h"

#include "QTRTPFile.h"
#include "OSMemory.h"


#define QT_PROFILE 0


#if QT_PROFILE
#include "StopWatch.h"
#endif


// -------------------------------------
// Macros
//
#define DEBUG_PRINT(s) if(fDebug) printf s
#define DEEP_DEBUG_PRINT(s) if(fDeepDebug) printf s


//
// This is a declaration of what RTP-Meta-Info fields are supported by QTFileLib.
// The actual support goes into QTHintTrack.h / .cpp
//
// Current fields are: kPacketPosField, kTransTimeField, kFrameTypeField, kPacketNumField, kSeqNumField, kMediaDataField
// See RTPMetaInfoPacket.h for details.
const RTPMetaInfoPacket::FieldID QTRTPFile::kMetaInfoFields[] =
	{ RTPMetaInfoPacket::kUncompressed, RTPMetaInfoPacket::kUncompressed, RTPMetaInfoPacket::kUncompressed, RTPMetaInfoPacket::kUncompressed, RTPMetaInfoPacket::kUncompressed, RTPMetaInfoPacket::kUncompressed };


// -------------------------------------
// Protected cache functions and variables.
//
OSMutex							*QTRTPFile::gFileCacheMutex,
								*QTRTPFile::gFileCacheAddMutex;
QTRTPFile::RTPFileCacheEntry	*QTRTPFile::gFirstFileCacheEntry = NULL;

void QTRTPFile::Initialize(void)
{
	QTRTPFile::gFileCacheMutex = NEW OSMutex();
	QTRTPFile::gFileCacheAddMutex = NEW OSMutex();
}


QTRTPFile::ErrorCode QTRTPFile::new_QTFile(const char * filePath, QTFile ** theQTFile, Bool16 debugFlag, Bool16 deepDebugFlag)
{
	// Temporary vars
	QTFile::ErrorCode	rcFile;

	// General vars
	OSMutexLocker		fileCacheAddMutex(QTRTPFile::gFileCacheAddMutex);

	QTRTPFile::RTPFileCacheEntry	*fileCacheEntry;
	
		
	//
	// Find and return the QTFile object out of our cache, if it exists.
	if( QTRTPFile::FindAndRefcountFileCacheEntry(filePath, &fileCacheEntry) ) 
	{
		fileCacheAddMutex.Unlock();
	
		fileCacheEntry->InitMutex->Lock();	// Guaranteed to block as the mutex
											// is acquired before it is added to
											// the list.
		fileCacheEntry->InitMutex->Unlock();// Because we don't actually need it.
	
		*theQTFile = fileCacheEntry->File;
	
		return errNoError;
	}


	//
	// Construct our file object.
	*theQTFile = NEW QTFile(debugFlag, deepDebugFlag);
	if( *theQTFile == NULL )
		return errInternalError;
		
	//
	// Open the specified movie.
	if( (rcFile = (*theQTFile)->Open(filePath)) != QTFile::errNoError ) 
	{
		delete *theQTFile;
		
		switch( rcFile ) 
		{
			case errFileNotFound:
				return errFileNotFound;
				
			case errInvalidQuickTimeFile: 
				return errInvalidQuickTimeFile;
				
			default: 
				return errInternalError;
		}
	}
	

	//
	// Add this file to our cache and release the global add mutex.
	QTRTPFile::AddFileToCache(filePath, &fileCacheEntry); // Grabs InitMutex.
	fileCacheAddMutex.Unlock();


	//
	// Finish setting up the fileCacheEntry.
	if( fileCacheEntry != NULL )
	{	// it may not have been cached..
		fileCacheEntry->File = *theQTFile;
		fileCacheEntry->InitMutex->Unlock();
	}
	

	//
	// Return the file object.
	return errNoError;
}

void QTRTPFile::delete_QTFile(QTFile * theQTFile)
{

	// General vars
	OSMutexLocker					fileCacheMutex(QTRTPFile::gFileCacheMutex);
	QTRTPFile::RTPFileCacheEntry	*listEntry;


	//
	// Find the specified cache entry.
	for ( listEntry = QTRTPFile::gFirstFileCacheEntry; listEntry != NULL; listEntry = listEntry->NextEntry ) 
	{
		//
		// Check for matches.
		if( listEntry->File != theQTFile )
			continue;
			
		//
		// Delete the object if the reference count has dropped to zero.
		if ( --listEntry->ReferenceCount == 0 ) 
		{
			//
			// Delete the file.
			if( theQTFile != NULL )
				delete theQTFile;

			//
			// Free our other vars.
			if( listEntry->InitMutex != NULL )
				delete listEntry->InitMutex;

			if( listEntry->fFilename != NULL )
			    delete [] listEntry->fFilename;
			
			//
			// Remove this entry from the list.
			if( listEntry->PrevEntry != NULL )
				listEntry->PrevEntry->NextEntry = listEntry->NextEntry;
		
			if( listEntry->NextEntry != NULL )
				listEntry->NextEntry->PrevEntry = listEntry->PrevEntry;
		
			if( QTRTPFile::gFirstFileCacheEntry == listEntry )
				QTRTPFile::gFirstFileCacheEntry = listEntry->NextEntry;
				
			delete listEntry;
		}

		//
		// Return.
		return;
	}

	//
	// The object was not in the cache.  Delete it
	if( theQTFile != NULL )
		delete theQTFile;
}


void QTRTPFile::AddFileToCache(const char *inFilename, QTRTPFile::RTPFileCacheEntry ** newListEntry)
{
	// General vars
	OSMutexLocker					fileCacheMutex(QTRTPFile::gFileCacheMutex);
	QTRTPFile::RTPFileCacheEntry*	listEntry;
	QTRTPFile::RTPFileCacheEntry*	lastListEntry;
	
	
	//
	// Add this track object to our track list.
	(*newListEntry) = NEW QTRTPFile::RTPFileCacheEntry();
	if( (*newListEntry) == NULL )
		return;

	(*newListEntry)->InitMutex = NEW OSMutex();
	if( (*newListEntry)->InitMutex == NULL ) {
		delete (*newListEntry);
		*newListEntry = NULL;
		return;
	}
	(*newListEntry)->InitMutex->Lock();
	
	(*newListEntry)->fFilename = NEW char[(::strlen(inFilename) + 2)];
	::strcpy((*newListEntry)->fFilename, inFilename);
	(*newListEntry)->File = NULL;
	
	(*newListEntry)->ReferenceCount = 1;

	(*newListEntry)->PrevEntry = NULL;
	(*newListEntry)->NextEntry = NULL;

	//
	// Make this the first entry if there are no entries, otherwise we need to
	// find out where this file fits in the list and insert it there.
	if( QTRTPFile::gFirstFileCacheEntry == NULL ) 
	{
		QTRTPFile::gFirstFileCacheEntry = (*newListEntry);
	}
	else
	{
		//
		// Go through the cache list until we find an inode number greater than
		// the one that we have now.  Insert it in the list when we find this.
		for( listEntry = lastListEntry = QTRTPFile::gFirstFileCacheEntry; listEntry != NULL; listEntry = listEntry->NextEntry ) {
			//
			// This is the last list entry that we saw (useful for later).
			lastListEntry = listEntry;
			
			//
			// Skip this entry if this inode number is smaller than the one
			// for our new entry.
			if( strcmp(listEntry->fFilename,inFilename) < 0 )
				continue;
			
			//
			// We've found a larger inode; insert this one in the list.
			if( listEntry->PrevEntry == NULL )
				QTRTPFile::gFirstFileCacheEntry = (*newListEntry);
			else
				listEntry->PrevEntry->NextEntry = (*newListEntry);
				
			(*newListEntry)->PrevEntry = listEntry->PrevEntry;
			
			listEntry->PrevEntry = (*newListEntry);
			
			(*newListEntry)->NextEntry = listEntry;
			
			return;
		}
		
		//
		// We fell out of our loop; this means that we are the largest inode
		// in the list; add ourselves to the end of the list.
		if( lastListEntry == NULL ) { // this can't happen, but..
			QTRTPFile::gFirstFileCacheEntry = (*newListEntry);
		}
		else
		{
			lastListEntry->NextEntry = (*newListEntry);
			(*newListEntry)->PrevEntry = lastListEntry;
		}
	}
}

Bool16 QTRTPFile::FindAndRefcountFileCacheEntry(const char *inFilename, QTRTPFile::RTPFileCacheEntry **cacheEntry)
{
	// General vars
	OSMutexLocker					fileCacheMutex(QTRTPFile::gFileCacheMutex);
	QTRTPFile::RTPFileCacheEntry	*listEntry;


	//
	// Find the specified cache entry.
	for( listEntry = QTRTPFile::gFirstFileCacheEntry; listEntry != NULL; listEntry = listEntry->NextEntry )
	{
		//
		// Check for matches.
	    if( ::strcmp(listEntry->fFilename, inFilename) != 0 )
			continue;

		//
		// Update the reference count and set the return value.
		listEntry->ReferenceCount++;
		
		*cacheEntry = listEntry;
		
		//
		// Return.
		return true;
	}

	//
	// The search failed.
	return false;
}



// -------------------------------------
// Constructors and destructors
//
QTRTPFile::QTRTPFile(Bool16 debugFlag, Bool16 deepDebugFlag)
	: fDebug(debugFlag)
	, fDeepDebug(deepDebugFlag)
	, fFile(NULL)
	, fFCB(NULL)
	, fNumHintTracks(0)
	, fFirstTrack(NULL)
	, fLastTrack(NULL)
	, fSDPFile(NULL)
	, fSDPFileLength(0)
	, fRequestedSeekTime(0.0)
	, fSeekTime(0.0)
	, fLastPacketTrack(NULL)
	, fBytesPerSecond(0)
	, fFirstPacketTransmissionTime(0.0)
	, fIsFirstPacket(true)
	, fErr(errNoError)
{
	fFCB = NEW QTFile_FileControlBlock();
}


QTRTPFile::~QTRTPFile(void)
{
	//
	// Free our track list (and the associated tracks)
	RTPTrackListEntry*	trackEntry = fFirstTrack;
	RTPTrackListEntry*	nextTrackEntry;
	
	
	while( trackEntry != NULL ) 
	{
		nextTrackEntry = trackEntry->NextTrack;
		
		//
		// Delete this track entry and move to the next one.
		if( trackEntry->HTCB != NULL )
			delete trackEntry->HTCB;
	
		delete trackEntry;
		
		trackEntry = nextTrackEntry;
			
	}

	//
	// Free our variables
	if( fSDPFile != NULL )
		delete[] fSDPFile;
	
	this->delete_QTFile(fFile);

	if( fFCB != NULL )
		delete fFCB;
}



// -------------------------------------
// Initialization functions.
//
QTRTPFile::ErrorCode QTRTPFile::Initialize(const char * filePath)
{
	// Temporary vars
	QTRTPFile::ErrorCode	rc;

	// General vars
	QTTrack					*track;
	
	
	//
	// Create our file object.
	rc = this->new_QTFile(filePath, &fFile, fDebug, fDeepDebug);
	if ( rc != errNoError ) 
	{
		fFile = NULL;
		return rc;
	}


	//
	// Iterate through all of the tracks, adding hint tracks to our list.
	for ( track = NULL; fFile->NextTrack(&track, track); )
	{
		// General vars
		QTHintTrack			*hintTrack;
		RTPTrackListEntry	*listEntry;
		
		//
		// Skip over anything that's *not* a hint track.
		if( !fFile->IsHintTrack(track) )
			continue;
			
		hintTrack = (QTHintTrack *)track;
		
		//
		// Add this track object to our track list.
		listEntry = NEW RTPTrackListEntry();
		if( listEntry == NULL )
			return fErr = errNoHintTracks; 

		listEntry->TrackID = track->GetTrackID();
		listEntry->HintTrack = hintTrack;
		listEntry->HTCB = NEW QTHintTrack_HintTrackControlBlock(fFCB);
		listEntry->IsTrackActive = false;
		listEntry->IsPacketAvailable = false;
		listEntry->QualityLevel = kAllPackets;
		
		listEntry->Cookie = NULL;
		listEntry->SSRC = 0;

		listEntry->BaseSequenceNumberRandomOffset = 0;
		listEntry->FileSequenceNumberRandomOffset = 0;
		listEntry->LastSequenceNumber = 0;
		listEntry->SequenceNumberAdditive = 0;

		listEntry->BaseTimestampRandomOffset = 0;
		listEntry->FileTimestampRandomOffset = 0;
		
		listEntry->CurSampleNumber = 0;
		listEntry->NumPacketsInThisSample = 0;
		listEntry->CurPacketNumber = 0;
		
		listEntry->CurPacketTime = 0.0;
		listEntry->CurPacketLength = 0;

		listEntry->NextTrack = NULL;

		if( fFirstTrack == NULL ) {
			fFirstTrack = fLastTrack = listEntry;
		} else {
			fLastTrack->NextTrack = listEntry;
			fLastTrack = listEntry;
		}
		
		// One more track..
		fNumHintTracks++;
	}
	
	// If there aren't any hint tracks, there's no way we can stream this movie,
	// so notify the caller
	if (fNumHintTracks == 0)
		return fErr = errNoHintTracks; 
	
		
	// The RTP file has been initialized.
	return errNoError;
}



// -------------------------------------
// Accessors
//
Float64 QTRTPFile::GetMovieDuration(void)
{
	return fFile->GetDurationInSeconds();
}

UInt64 QTRTPFile::GetAddedTracksRTPBytes(void)
{
	// Temporary vars
	RTPTrackListEntry	*curEntry;
	
	// General vars
	UInt64				totalRTPBytes = 0;
	
	
	//
	// Go through all of the tracks, adding up the size of the RTP bytes
	// for all active tracks.
	for( curEntry = fFirstTrack;
		 curEntry != NULL;
		 curEntry = curEntry->NextTrack
	   ) 
	{
		//
		// We only want active tracks.
		if( !curEntry->IsTrackActive )
			continue;

		//
		// Add this length to our count.
		totalRTPBytes += curEntry->HintTrack->GetTotalRTPBytes();
	}
	
	//
	// Return the size.
	return totalRTPBytes;
}

char * QTRTPFile::GetSDPFile(int * sdpFileLength)
{
	// Temporary vars
	RTPTrackListEntry*	curEntry;
	UInt32				tempAtomType;

	// General vars
	QTFile::AtomTOCEntry*	globalSDPTOCEntry;
	Bool16					haveGlobalSDPAtom = false;	
	char					sdpRangeLine[256];
	char*					pSDPFile;
	
	
	//
	// Return our cached SDP file if we have one.
	if ( fSDPFile != NULL) 
	{
		*sdpFileLength = fSDPFileLength;
		return fSDPFile;
	}
	
	//
	// Build our range header.
	::sprintf(sdpRangeLine, "a=range:npt=0-%10.5f\r\n", this->GetMovieDuration());


	//
	// Figure out how long the SDP file is going to be.
	fSDPFileLength = ::strlen(sdpRangeLine);
	
	for ( curEntry = fFirstTrack;
		 curEntry != NULL;
		 curEntry = curEntry->NextTrack
		) 
	{
		// Temporary vars
		int			trackSDPLength;


		//
		// Get the length of this track's SDP file.
		if( curEntry->HintTrack->GetSDPFileLength(&trackSDPLength) != QTTrack::errNoError )
			continue;
		
		//
		// Add it to our count.
		fSDPFileLength += trackSDPLength;
	}

	//
	// See if this movie has a global SDP atom.
	if( fFile->FindTOCEntry("moov:udta:hnti:rtp ", &globalSDPTOCEntry, NULL) )
	{
		//
		// Verify that this is an SDP atom.
		fFile->Read(globalSDPTOCEntry->AtomDataPos, (char *)&tempAtomType, 4);
		
		if ( ntohl(tempAtomType) == FOUR_CHARS_TO_INT('s', 'd', 'p', ' ') ) 
		{
			haveGlobalSDPAtom = true;
			fSDPFileLength += globalSDPTOCEntry->AtomDataLength - 4;
		}
	}

	//
	// Build the SDP file.
	fSDPFile = pSDPFile = NEW char[fSDPFileLength];
	if( fSDPFile == NULL )
	{	fErr = errInternalError;
		return NULL;
	}
	
	if( haveGlobalSDPAtom ) 
	{
		fFile->Read(globalSDPTOCEntry->AtomDataPos + 4, pSDPFile, globalSDPTOCEntry->AtomDataLength - 4);
		pSDPFile += globalSDPTOCEntry->AtomDataLength - 4;
	}
	
	::memcpy(pSDPFile, sdpRangeLine, ::strlen(sdpRangeLine));
	
	pSDPFile += ::strlen(sdpRangeLine);
	
	for ( curEntry = fFirstTrack;
		  curEntry != NULL;
		  curEntry = curEntry->NextTrack
		) 
	{
		// Temporary vars
		char		*trackSDP;
		int			trackSDPLength;
		
		
		//
		// Get this track's SDP file and add it to our buffer.
		trackSDP = curEntry->HintTrack->GetSDPFile(&trackSDPLength);
		if( trackSDP == NULL )
			continue;
		
		::memcpy(pSDPFile, trackSDP, trackSDPLength);
		delete [] trackSDP;//ARGH! GetSDPFile allocates the pointer that is being returned.
		pSDPFile += trackSDPLength;
	}
	
	
	//
	// Return the (cached) SDP file.
	*sdpFileLength = fSDPFileLength;
	return fSDPFile;
}

char*	QTRTPFile::GetMoviePath() 
{ 
	Assert( fFile );
	if (!fFile) 
	{	fErr = errInternalError;
		return NULL;
	}
	else
	return fFile->GetMoviePath(); 
}


// -------------------------------------
// track functions
//
QTRTPFile::ErrorCode QTRTPFile::AddTrack(UInt32 trackID, Bool16 useRandomOffset)
{
	// General vars
	RTPTrackListEntry	*trackEntry;
	
	
	//
	// Find this track.
	if( !this->FindTrackEntry(trackID, &trackEntry) )
		return fErr =  errTrackIDNotFound;

	{
		OSMutexLocker locker(fFile->GetMutex());
		//
		// Initialize this track.
		if( trackEntry->HintTrack->Initialize() != QTTrack::errNoError )
			return fErr = errInternalError;
	}
	
	//
	// Set up the sequence number and timestamp offsets.
	if( useRandomOffset ) {
		trackEntry->BaseSequenceNumberRandomOffset = (UInt16)rand();
		trackEntry->BaseTimestampRandomOffset = (UInt32)rand();
	} else {
		trackEntry->BaseSequenceNumberRandomOffset = 0;
		trackEntry->BaseTimestampRandomOffset = 0;
	}

	trackEntry->FileSequenceNumberRandomOffset = trackEntry->HintTrack->GetRTPSequenceNumberRandomOffset();
	trackEntry->FileTimestampRandomOffset = trackEntry->HintTrack->GetRTPTimestampRandomOffset();
	
	trackEntry->LastSequenceNumber = 0;
	trackEntry->SequenceNumberAdditive = 0;

	//
	// Setup RTP-Meta-Info stuff for this track.
	
	//
	// This track is now active.
	trackEntry->IsTrackActive = true;
	
	//
	// The track has been added.
	return errNoError;
}

Float64 QTRTPFile::GetTrackDuration(UInt32 trackID)
{
	// General vars
	RTPTrackListEntry	*trackEntry;
	
	
	//
	// Find this track.
	if( !this->FindTrackEntry(trackID, &trackEntry) )
		return 0;
	
	//
	// Return the duration.
	return trackEntry->HintTrack->GetDurationInSeconds();
}

UInt32 QTRTPFile::GetTrackTimeScale(UInt32 trackID)
{
	// General vars
	RTPTrackListEntry	*trackEntry;
	
	
	//
	// Find this track.
	if( !this->FindTrackEntry(trackID, &trackEntry) )
		return 0;
	
	//
	// Return the duration.
	return (UInt32)trackEntry->HintTrack->GetTimeScale();
}

void QTRTPFile::SetTrackSSRC(UInt32 trackID, UInt32 SSRC)
{
	// General vars
	RTPTrackListEntry	*trackEntry;
	
	
	//
	// Find this track.
	if( !this->FindTrackEntry(trackID, &trackEntry) )
		return;
	
	//
	// Set the SSRC.
	trackEntry->SSRC = SSRC;
}

void QTRTPFile::SetTrackCookie(UInt32 trackID, void * cookie)
{
	// General vars
	RTPTrackListEntry	*trackEntry;
	
	
	//
	// Find this track.
	if( !this->FindTrackEntry(trackID, &trackEntry) )
		return;
	
	//
	// Set the cookie.
	trackEntry->Cookie = cookie;
}

void QTRTPFile::SetTrackRTPMetaInfo(UInt32 TrackID, RTPMetaInfoPacket::FieldID* inFieldArray, Bool16 isVideo)
{
	// General vars
	RTPTrackListEntry	*trackEntry;
	
	
	//
	// Find this track.
	if( !this->FindTrackEntry(TrackID, &trackEntry) )
		return;
	
	//
	// Set the cookie.
	trackEntry->HTCB->SetupRTPMetaInfo(inFieldArray, isVideo);
}

void QTRTPFile::SetTrackQualityLevel(UInt32 trackID, UInt32 inNewQuality)
{
	// General vars
	RTPTrackListEntry	*trackEntry;
	
	
	//
	// Find this track.
	if( !this->FindTrackEntry(trackID, &trackEntry) )
		return;
	
	//
	// Set the option.
	trackEntry->QualityLevel = inNewQuality;
}

// -------------------------------------
//	BytesPerSecond
UInt32 QTRTPFile::GetBytesPerSecond(void)
{
	// Must be a SInt64 bc Win32 doesn't implement UInt64 -> Float64
	SInt64 	totalBytes = (SInt64)QTRTPFile::GetAddedTracksRTPBytes();	
	Float64 duration = fFile->GetDurationInSeconds();
	
	UInt32	bytesPerSecond = 0;
	if (duration > 0)  
		bytesPerSecond = (UInt32) ( (Float64) totalBytes / (Float64) duration);

	return bytesPerSecond;
}


// -------------------------------------
// Packet functions
//
QTRTPFile::ErrorCode QTRTPFile::Seek(Float64 seekToTime, Float64 maxBackupTime)
{
	// General vars
	RTPTrackListEntry	*listEntry;
	UInt32				bytesPerSecond;
	Float64				syncToTime = seekToTime;


	//
	// Adjust the size of our QTFile buffer to reflect the current bitrate of
	// the movie.
	bytesPerSecond = this->GetBytesPerSecond();
	if ( bytesPerSecond != fBytesPerSecond )
		fFCB->AdjustDataBuffer((fBytesPerSecond = bytesPerSecond) * 8);

	//
	// Find the earliest sync sample and sync all of the tracks to that
	// sample.
	for ( listEntry = fFirstTrack; listEntry != NULL; listEntry = listEntry->NextTrack ) 
	{
		// General vars
		SInt32			mediaTime;
		UInt32			newSampleNumber;
		UInt32			newSyncSampleNumber;
		UInt32			newSampleMediaTime;
		Float64			newSampleTime;
	
	
		//
		// Only consider active tracks.
		if( !listEntry->IsTrackActive )
			continue;
		
		//
		// Compute the media time and get the sample at that time.
		mediaTime = (SInt32)(seekToTime * listEntry->HintTrack->GetTimeScale());
		mediaTime -= listEntry->HintTrack->GetFirstEditMediaTime();
		if ( mediaTime < 0 )
			mediaTime = 0;
			
		if ( !listEntry->HintTrack->GetSampleNumberFromMediaTime(mediaTime, &newSampleNumber, listEntry->HTCB->fsttsSTCB) )
			continue;	// This track is probably done playing.
		
		//
		// Find the nearest (moving backwards in time) keyframe.
		listEntry->HintTrack->GetPreviousSyncSample(newSampleNumber, &newSyncSampleNumber);
		if ( newSampleNumber == newSyncSampleNumber )
			continue;

		//
		// Figure out what time this sample is at.
		if( !listEntry->HintTrack->GetSampleMediaTime(newSyncSampleNumber, &newSampleMediaTime) )
			return errInvalidQuickTimeFile;
			
		newSampleMediaTime += listEntry->HintTrack->GetFirstEditMediaTime();
		newSampleTime = (Float64)newSampleMediaTime * listEntry->HintTrack->GetTimeScaleRecip();

		//
		// Figure out if this is the time that we need to sync to.
		if( newSampleTime < syncToTime )
			syncToTime = newSampleTime;
	}

	//
	// Evaluate/Store the seek time
	fRequestedSeekTime = seekToTime;
	if( (seekToTime - syncToTime) <= maxBackupTime )
		fSeekTime = syncToTime;
	else
		fSeekTime = seekToTime;

	//
	// Prefetch a packet for all active tracks.
	for( listEntry = fFirstTrack; listEntry != NULL; listEntry = listEntry->NextTrack ) 
	{
		//
		// Only fetch packets on active tracks.
		if( !listEntry->IsTrackActive )
			continue;
		
		//
		// Reset the sample table caches.
		listEntry->HTCB->fstscSTCB->Reset();
		listEntry->HTCB->Reset(fSeekTime);

		//
		// Prefetch a packet.
		listEntry->IsPacketAvailable = false;
		if( !this->PrefetchNextPacket(listEntry, true, fSeekTime) )
			continue;
		
		//
		// We've got a packet..
		listEntry->IsPacketAvailable = true;
	}

	//
	// 'Forget' that we had a previous packet.
	fLastPacketTrack = NULL;
	
	return errNoError;
}

UInt32 QTRTPFile::GetSeekTimestamp(UInt32 trackID)
{
	// General vars
	RTPTrackListEntry	*trackEntry;
	UInt32				mediaTime;
	UInt32				rtpTimestamp;
	
	
	DEEP_DEBUG_PRINT(("Calculating RTP timestamp for track #%lu at time %.2f.\n", trackID, fRequestedSeekTime));

	//
	// Find this track.
	if( !FindTrackEntry(trackID, &trackEntry) )
		return 0;
	
	//
	// Calculate the timestamp at this seek time.
	mediaTime = (UInt32)(fRequestedSeekTime * trackEntry->HintTrack->GetTimeScale());
	if( trackEntry->HintTrack->GetRTPTimescale() == trackEntry->HintTrack->GetTimeScale() )
		rtpTimestamp = mediaTime;
	else
		rtpTimestamp = (UInt32)(mediaTime * (trackEntry->HintTrack->GetRTPTimescale() * trackEntry->HintTrack->GetTimeScaleRecip()) );
	
	//
	// Add the appropriate offsets.
	rtpTimestamp += trackEntry->BaseTimestampRandomOffset + trackEntry->FileTimestampRandomOffset;

	//
	// Return the RTP timestamp.
	DEEP_DEBUG_PRINT(("..timestamp=%lu\n", rtpTimestamp));

	return rtpTimestamp;
}

UInt16 QTRTPFile::GetNextTrackSequenceNumber(UInt32 trackID)
{
	// General vars
	RTPTrackListEntry	*trackEntry;
	UInt16				rtpSequenceNumber;
	
	
	//
	// Find this track.
	if( !this->FindTrackEntry(trackID, &trackEntry) )
		return 0;
	
	//
	// Read the sequence number right out of the packet.
	::memcpy(&rtpSequenceNumber, (char *)trackEntry->CurPacket + 2, 2);

	return ntohs(rtpSequenceNumber);
}

Float64 QTRTPFile::GetNextPacket(char ** outPacket, int * outPacketLength, void ** outCookie)
{
	// General vars
	RTPTrackListEntry	*listEntry;

	Bool16				haveFirstPacketTime = false;
	Float64				firstPacketTime = 0.0;
	RTPTrackListEntry	*firstPacket = NULL;


	//
	// Clear the input.
	*outPacket = NULL;
	*outPacketLength = 0;
	*outCookie = NULL;
	
	//
	// Prefetch the next packet of the track that the *last* packet came from.
	if( fLastPacketTrack != NULL )
	{
		if ( !this->PrefetchNextPacket(fLastPacketTrack) )
			fLastPacketTrack->IsPacketAvailable = false;
	}
	
	//
	// Figure out which track is going to produce the next packet.
	for( listEntry = fFirstTrack; listEntry != NULL; listEntry = listEntry->NextTrack ) 
	{
		//
		// Only look at active tracks that have packets.
		if( !listEntry->IsTrackActive || !listEntry->IsPacketAvailable)
			continue;
		
		//
		// See if this track has a time earlier than our initial time.
		if( (listEntry->CurPacketTime <= firstPacketTime) || !haveFirstPacketTime ) 
		{
			haveFirstPacketTime 	= true;
			firstPacketTime 		= listEntry->CurPacketTime;
			firstPacket 			= listEntry;
		}
	}
	
	//
	// Abort if we didn't find a packet.  Either the movie is over, or there
	// weren't any packets to begin with.
	if( firstPacket == NULL )
		return 0.0;
	
	//
	// Remember the sequence number of this packet.
	firstPacket->LastSequenceNumber = ntohs(*(UInt16 *)((char *)firstPacket->CurPacket + 2));
	firstPacket->LastSequenceNumber -= firstPacket->BaseSequenceNumberRandomOffset + firstPacket->FileSequenceNumberRandomOffset + firstPacket->SequenceNumberAdditive;

	//
	// Return this packet.
	fLastPacketTrack = firstPacket;
	
	*outPacket = firstPacket->CurPacket;
	*outPacketLength = firstPacket->CurPacketLength;
	*outCookie = firstPacket->Cookie;

	// hinted qt movies impossibly tell you to play their data at a point in the 
	// past.  we note that here so that it can be corrected for
	if ( fIsFirstPacket )
	{
		fIsFirstPacket = false;
		fFirstPacketTransmissionTime =  firstPacket->CurPacketTime;
	}
	
	return firstPacket->CurPacketTime;
}



// -------------------------------------
// Protected member functions
//
Bool16 QTRTPFile::FindTrackEntry(UInt32 trackID, RTPTrackListEntry **trackEntry)
{
	// General vars
	RTPTrackListEntry	*listEntry;


	//
	// Find the specified track.
	for( listEntry = fFirstTrack; listEntry != NULL; listEntry = listEntry->NextTrack )
	{
		//
		// Check for matches.
		if( listEntry->TrackID == trackID )
		{
			*trackEntry = listEntry;
			return true;
		}
	}

	//
	// The search failed.
	return false;
}

Bool16 QTRTPFile::PrefetchNextPacket(RTPTrackListEntry * trackEntry, Bool16 doSeek, Float64 packetTime)
{
	// General vars
	UInt16			*pSequenceNumber;
	UInt32			*pTimestamp;

	//
	// Do a seek if requested.
	if( doSeek ) 
	{
		// General vars
		SInt32			mediaTime;
	
		//
		// Compute the media time and get the sample at that time.
		mediaTime = (SInt32)(packetTime * trackEntry->HintTrack->GetTimeScale());
		mediaTime -= trackEntry->HintTrack->GetFirstEditMediaTime();
		if( mediaTime < 0 )
			mediaTime = 0;
			
		if( !trackEntry->HintTrack->GetSampleNumberFromMediaTime(mediaTime, &trackEntry->CurSampleNumber, trackEntry->HTCB->fsttsSTCB) )
			return false;
		
		//
		// Clear our current packet information.
		trackEntry->NumPacketsInThisSample = 0;
		trackEntry->CurPacketNumber = 0;
	}
	
	// Temporary vars
	QTTrack::ErrorCode	rcTrack = QTTrack::errIsBFrame;
	
	// If we are dropping b-frames, QTHintTrack::GetPacket will return the errIsBFrame error to us.
	// If we get that error, we should fetch another packet. So, we have this loop here.
	while (rcTrack == QTTrack::errIsBFrame)
	{
		//
		// Next packet (or first packet, since CurPacketNumber starts at 0 above)
		trackEntry->CurPacketNumber++;

		//
		// Do we need to move to the next sample?
		if	( (trackEntry->CurPacketNumber > trackEntry->NumPacketsInThisSample)
			  && (trackEntry->NumPacketsInThisSample != 0)
			) 
		{
			//
			// If we're only reading sync samples, then we need to find the next
			// one to send out, otherwise just increment the sample number and
			// move on.
			if( trackEntry->QualityLevel >= kKeyFramesOnly )
			{
				//Use the quality level to determine how many key frames to skip over.
				for (UInt32 keyFramesSkipCount = (kKeyFramesOnly - 1); keyFramesSkipCount < trackEntry->QualityLevel; keyFramesSkipCount++)
					trackEntry->HintTrack->GetNextSyncSample(trackEntry->CurSampleNumber, &trackEntry->CurSampleNumber);
			}
			else
			{
				trackEntry->CurSampleNumber++;
			}
			
			//
			// We'll need to recompute the number of samples in this packet.
			trackEntry->NumPacketsInThisSample = 0;
		}

		//
		// Do we know how many packets are in this sample?  If not, figure it out.
		while ( trackEntry->NumPacketsInThisSample == 0 ) 
		{
			if ( trackEntry->HintTrack->GetNumPackets(trackEntry->CurSampleNumber, &trackEntry->NumPacketsInThisSample, trackEntry->HTCB) != QTTrack::errNoError )
				return false;
				
			if ( trackEntry->NumPacketsInThisSample == 0 )
				trackEntry->CurSampleNumber++;
		
			trackEntry->CurPacketNumber = 1;
		}
		
		//
		// Fetch this packet.
		trackEntry->CurPacketLength = QTRTPFILE_MAX_PACKET_LENGTH;

		
	#if QT_PROFILE
		MicroSecondStopWatch	packetTimer;
		packetTimer.Start();
	#endif
		rcTrack = trackEntry->HintTrack->GetPacket(trackEntry->CurSampleNumber, trackEntry->CurPacketNumber,
												   trackEntry->CurPacket, &trackEntry->CurPacketLength,
												   &trackEntry->CurPacketTime,
												   (trackEntry->QualityLevel == kNoBFrames),
												   trackEntry->SSRC,
												   trackEntry->HTCB);

	#if QT_PROFILE
		packetTimer.Stop();
		::printf( "GetPacket sample time %li NumPackets: %li Packet fetched %li. len: %li\n", 
					(long)packetTimer.Duration(), (long)trackEntry->NumPacketsInThisSample, (long)trackEntry->CurPacketNumber, (long)trackEntry->CurPacketLength  );
	#endif
	}

	if( rcTrack != QTTrack::errNoError )
	{	fErr = errInvalidQuickTimeFile; 
		return false;
	}
		
		
	//
	// Update our sequence number and timestamp.  If we seeked to get here,
	// then we need to adjust the additive to account for the shift in
	// sequence numbers.
	pSequenceNumber = (UInt16 *)((char *)trackEntry->CurPacket + 2);
	pTimestamp = (UInt32 *)((char *)trackEntry->CurPacket + 4);
	
	if( doSeek || (trackEntry->QualityLevel > kAllPackets) )
		trackEntry->SequenceNumberAdditive += (trackEntry->LastSequenceNumber + 1) - ntohs(*pSequenceNumber);
	
	*pSequenceNumber = htons(ntohs(*pSequenceNumber) + trackEntry->BaseSequenceNumberRandomOffset + trackEntry->FileSequenceNumberRandomOffset + trackEntry->SequenceNumberAdditive);
	*pTimestamp = htonl(ntohl(*pTimestamp) + trackEntry->BaseTimestampRandomOffset + trackEntry->FileTimestampRandomOffset);
	
	//
	// Return the packet.
	return true;
}

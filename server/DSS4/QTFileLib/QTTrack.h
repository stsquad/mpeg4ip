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
// $Id: QTTrack.h,v 1.1 2002/02/27 19:49:01 wmaycisco Exp $
//
// QTTrack:
//   The central point of control for a track in a QTFile.

#ifndef QTTrack_H
#define QTTrack_H


//
// Includes
#include "QTAtom_dref.h"
#include "QTAtom_elst.h"
#include "QTAtom_mdhd.h"
#include "QTAtom_tkhd.h"
#include "QTAtom_stco.h"
#include "QTAtom_stsc.h"
#include "QTAtom_stsd.h"
#include "QTAtom_stss.h"
#include "QTAtom_stsz.h"
#include "QTAtom_stts.h"


//
// External classes
class QTFile;
class QTFile_FileControlBlock;
class QTAtom_stsc_SampleTableControlBlock;
class QTAtom_stts_SampleTableControlBlock;


//
// QTTrack class
class QTTrack {

public:
	//
	// Class error codes
	enum ErrorCode {
		errNoError					= 0,
		errInvalidQuickTimeFile		= 1,
		errParamError				= 2,
		errIsSkippedPacket			= 3,
		errInternalError			= 100
	};

	
public:
	//
	// Constructors and destructor.
						QTTrack(QTFile * File, QTFile::AtomTOCEntry * trakAtom,
							   Bool16 Debug = false, Bool16 DeepDebug = false);
	virtual				~QTTrack(void);


	//
	// Initialization functions.
	virtual	ErrorCode	Initialize(void);

	//
	// Accessors.
	inline	Bool16		IsInitialized(void) { return fIsInitialized; }
	
	inline	const char *		GetTrackName(void) { return (fTrackName ? fTrackName : ""); }
	inline	UInt32		GetTrackID(void) { return fTrackHeaderAtom->GetTrackID(); }
	inline	UInt32		GetCreationTime(void) { return fTrackHeaderAtom->GetCreationTime(); }
	inline	UInt32		GetModificationTime(void) { return fTrackHeaderAtom->GetModificationTime(); }
	inline	UInt32		GetDuration(void) { return fTrackHeaderAtom->GetDuration(); }
	inline	Float64		GetTimeScale(void) { return fMediaHeaderAtom->GetTimeScale(); }
	inline	Float64		GetTimeScaleRecip(void) { return fMediaHeaderAtom->GetTimeScaleRecip(); }
	inline	Float64		GetDurationInSeconds(void) { return GetDuration() / (Float64)GetTimeScale(); }
	inline	UInt32		GetFirstEditMovieTime(void)
											  { if(fEditListAtom != NULL) return fEditListAtom->FirstEditMovieTime();
												else return 0; }
	inline	UInt32		GetFirstEditMediaTime(void) { return fFirstEditMediaTime; }
	
	//
	// Sample functions
	Bool16 				GetSizeOfSamplesInChunk(UInt32 chunkNumber, UInt32 * const sizePtr, UInt32 * const firstSampleNumPtr, UInt32 * const lastSampleNumPtr, QTAtom_stsc_SampleTableControlBlock * stcbPtr);

	inline 	Bool16		GetChunkFirstLastSample(UInt32 chunkNumber, UInt32 *firstSample, UInt32 *lastSample, 
												QTAtom_stsc_SampleTableControlBlock *STCB)
						{	return fSampleToChunkAtom->GetChunkFirstLastSample(chunkNumber,firstSample, lastSample, STCB); 
						}


	inline	Bool16		SampleToChunkInfo(UInt32 SampleNumber, UInt32 *samplesPerChunk, UInt32 *ChunkNumber, UInt32 *SampleDescriptionIndex, UInt32 *SampleOffsetInChunk,
												QTAtom_stsc_SampleTableControlBlock * STCB)
						{	return fSampleToChunkAtom->SampleToChunkInfo(SampleNumber,samplesPerChunk, ChunkNumber, SampleDescriptionIndex, SampleOffsetInChunk, STCB); 
						}


	inline	Bool16		SampleNumberToChunkNumber(UInt32 SampleNumber, UInt32 *ChunkNumber, UInt32 *SampleDescriptionIndex, UInt32 *SampleOffsetInChunk,
												 QTAtom_stsc_SampleTableControlBlock * STCB)
						{	return fSampleToChunkAtom->SampleNumberToChunkNumber(SampleNumber, ChunkNumber, SampleDescriptionIndex, SampleOffsetInChunk, STCB); 
						}

	
	inline	UInt32		GetChunkFirstSample(UInt32 chunkNumber) 
						{	return fSampleToChunkAtom->GetChunkFirstSample(chunkNumber); 
						}

	inline	Bool16		ChunkOffset(UInt32 ChunkNumber, UInt64 *Offset = NULL) 
						{	return fChunkOffsetAtom->ChunkOffset(ChunkNumber, Offset); 
						}

	inline	Bool16		SampleSize(UInt32 SampleNumber, UInt32 *Size = NULL) 
						{	return fSampleSizeAtom->SampleSize(SampleNumber, Size); 
						}

	inline	Bool16		SampleRangeSize(UInt32 firstSample, UInt32 lastSample, UInt32 *sizePtr = NULL) 
						{ 	return fSampleSizeAtom->SampleRangeSize(firstSample, lastSample, sizePtr); 
						}

	Bool16		GetSampleInfo(UInt32 SampleNumber, UInt32 * const Length, UInt64 * const Offset, UInt32 * const SampleDescriptionIndex,
									  			QTAtom_stsc_SampleTableControlBlock * STCB);

	Bool16		GetSample(UInt32 SampleNumber, char * Buffer, UInt32 * Length,QTFile_FileControlBlock * FCB, 
												QTAtom_stsc_SampleTableControlBlock * STCB);

	inline	Bool16		GetSampleMediaTime(UInt32 SampleNumber, UInt32 * const MediaTime, 
												QTAtom_stts_SampleTableControlBlock * STCB)
						{ 	return fTimeToSampleAtom->SampleNumberToMediaTime(SampleNumber, MediaTime, STCB); 
						}						

	inline	Bool16		GetSampleNumberFromMediaTime(UInt32 MediaTime, UInt32 * const SampleNumber, 
												QTAtom_stts_SampleTableControlBlock * STCB)
						{ 	return fTimeToSampleAtom->MediaTimeToSampleNumber(MediaTime, SampleNumber, STCB); 
						}


	inline	void		GetPreviousSyncSample(UInt32 SampleNumber, UInt32 * SyncSampleNumber)
						{ 	if(fSyncSampleAtom != NULL) fSyncSampleAtom->PreviousSyncSample(SampleNumber, SyncSampleNumber);
							else *SyncSampleNumber = SampleNumber; 
						}
						
	inline	void		GetNextSyncSample(UInt32 SampleNumber, UInt32 * SyncSampleNumber)
						{ 		if(fSyncSampleAtom != NULL) fSyncSampleAtom->NextSyncSample(SampleNumber, SyncSampleNumber);
								else *SyncSampleNumber = SampleNumber + 1; 
						}

	inline Bool16			IsSyncSample(UInt32 SampleNumber, UInt32 SyncSampleCursor)
						{ if (fSyncSampleAtom != NULL) return fSyncSampleAtom->IsSyncSample(SampleNumber, SyncSampleCursor);
							else return true;
						} 
	//
	// Read functions.
	inline	Bool16		Read(UInt32 SampleDescriptionID, UInt64 Offset, char * const Buffer, UInt32 Length,
							 					QTFile_FileControlBlock * FCB = NULL)
						{	return fDataReferenceAtom->Read(fSampleDescriptionAtom->SampleDescriptionToDataReference(SampleDescriptionID), Offset, Buffer, Length, FCB); 
						}

	//
	// Debugging functions.
	virtual	void		DumpTrack(void);
	inline	void		DumpSampleToChunkTable(void) { fSampleToChunkAtom->DumpTable(); }
	inline	void		DumpChunkOffsetTable(void) { fChunkOffsetAtom->DumpTable(); }
	inline	void		DumpSampleSizeTable(void) { fSampleSizeAtom->DumpTable(); }


protected:
	//
	// Protected member variables.
	Bool16				fDebug, fDeepDebug;
	QTFile				*fFile;
	QTFile::AtomTOCEntry fTOCEntry;
	
	Bool16				fIsInitialized;

	QTAtom_tkhd			*fTrackHeaderAtom;
	char				*fTrackName;

	QTAtom_mdhd			*fMediaHeaderAtom;

	QTAtom_elst			*fEditListAtom;
	QTAtom_dref			*fDataReferenceAtom;

	QTAtom_stts			*fTimeToSampleAtom;
	QTAtom_stsc			*fSampleToChunkAtom;
	QTAtom_stsd			*fSampleDescriptionAtom;
	QTAtom_stco			*fChunkOffsetAtom;
	QTAtom_stsz			*fSampleSizeAtom;
	QTAtom_stss			*fSyncSampleAtom;

	UInt32				fFirstEditMediaTime;
};

#endif // QTTrack_H

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
// $Id: QTFile.h,v 1.1 2002/02/27 19:49:01 wmaycisco Exp $
//
// QTFile:
//   The central point of control for a file in the QuickTime File Format.

#ifndef QTFile_H
#define QTFile_H


//
// Includes
#include "OSHeaders.h"
#include "OSFileSource.h"
#include "QTFile_FileControlBlock.h"
#include "DateTranslator.h"

//
// External classes
class OSMutex;

class QTAtom_mvhd;
class QTTrack;


//
// QTFile class
class QTFile {

public:
	//
	// Class constants
	
	
	//
	// Class error codes
	enum ErrorCode {
		errNoError					= 0,
		errFileNotFound				= 1,
		errInvalidQuickTimeFile		= 2,
		errInternalError			= 100
	};


	//
	// Class typedefs.
	struct AtomTOCEntry {
		// TOC id (used to compare TOCs)
		UInt32			TOCID;

		// Atom information
		OSType			AtomType, beAtomType; // be = Big Endian

		UInt64			AtomDataPos;
		UInt32			AtomDataLength, AtomHeaderSize;

		// TOC pointers
		AtomTOCEntry	*NextOrdAtom;

		AtomTOCEntry	*PrevAtom, *NextAtom;
		AtomTOCEntry	*Parent, *FirstChild;
	};

	struct TrackListEntry {
		// Track information
		UInt32			TrackID;
		QTTrack			*Track;
		Bool16			IsHintTrack;

		// List pointers
		TrackListEntry	*NextTrack;
	};


public:
	//
	// Constructors and destructor.
						QTFile(Bool16 Debug = false, Bool16 DeepDebug = false);
	virtual				~QTFile(void);


	//
	// Open a movie file and generate the atom table of contents.
			ErrorCode	Open(const char * MoviePath);
			
			OSMutex*	GetMutex() { return fReadMutex; }

	//
	// Table of Contents functions.
			Bool16		FindTOCEntry(const char * AtomPath,
									 AtomTOCEntry **TOCEntry,
									 AtomTOCEntry *LastFoundTOCEntry = NULL);
	
	//
	// Track List functions
	inline	UInt32		GetNumTracks(void) { return fNumTracks; }
			Bool16		NextTrack(QTTrack **Track, QTTrack *LastFoundTrack = NULL);
			Bool16		FindTrack(UInt32 TrackID, QTTrack **Track);
			Bool16		IsHintTrack(QTTrack *Track);
	
	//
	// Accessors
	inline	char *		GetMoviePath(void) { return fMoviePath; }
			Float64		GetTimeScale(void);
			Float64		GetDurationInSeconds(void);
			SInt64		GetModDate();
			// Returns the mod date as a RFC 1123 formatted string
			char*		GetModDateStr()	{ return fModDateBuffer.GetDateBuffer(); }
	//
	// Read functions.
			Bool16		Read(UInt64 Offset, char * const Buffer, UInt32 Length, QTFile_FileControlBlock * FCB = NULL);
	

			void 		AllocateBuffers(UInt32 inUnitSizeInK, UInt32 inBufferInc, UInt32 inBufferSize, UInt32 inMaxBitRateBuffSizeInBlocks, UInt32 inBitrate);
			void		IncBufferUserCount() {fMovieFD.IncMaxBuffers();}
			void		DecBufferUserCount() {fMovieFD.DecMaxBuffers();}
			
	//
	// Debugging functions.
			void		DumpAtomTOC(void);

protected:
	//
	// Protected member functions.
			Bool16		GenerateAtomTOC(void);
	
	//
	// Protected member variables.
	Bool16				fDebug, fDeepDebug;

	UInt32				fNextTOCID;
#if DSS_SERVER_INTERNAL
	QTSS_Object			fMovieFD;
#else
	OSFileSource		fMovieFD;
#endif
	Bool16				fCacheBuffersSet;
	
	DateBuffer			fModDateBuffer;
	char				*fMoviePath;

	AtomTOCEntry		*fTOC, *fTOCOrdHead, *fTOCOrdTail;
	
	UInt32				fNumTracks;
	TrackListEntry		*fFirstTrack, *fLastTrack;

	QTAtom_mvhd			*fMovieHeaderAtom;
	
	OSMutex				*fReadMutex;
};

#endif // QTFile_H

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
// $Id: QTAtom_stss.cpp,v 1.3 2001/10/11 20:39:08 wmaycisco Exp $
//
// QTAtom_stss:
//   The 'stss' QTAtom class.


// -------------------------------------
// Includes
//
#include <stdio.h>
#include <stdlib.h>
#ifndef __Win32__
#include <sys/types.h>
#include <netinet/in.h>
#endif

#include "QTFile.h"

#include "QTAtom.h"
#include "QTAtom_stss.h"
#include "OSMemory.h"


// -------------------------------------
// Constants
//
const int		stssPos_VersionFlags		=  0;
const int		stssPos_NumEntries			=  4;
const int		stssPos_SampleTable			=  8;



// -------------------------------------
// Macros
//
#define DEBUG_PRINT(s) if(fDebug) printf s
#define DEEP_DEBUG_PRINT(s) if(fDeepDebug) printf s



// -------------------------------------
// Constructors and destructors
//
QTAtom_stss::QTAtom_stss(QTFile * File, QTFile::AtomTOCEntry * TOCEntry, Bool16 Debug, Bool16 DeepDebug)
	: QTAtom(File, TOCEntry, Debug, DeepDebug),
	  fNumEntries(0), fSyncSampleTable(NULL), fTable(NULL)
{
}

QTAtom_stss::~QTAtom_stss(void)
{
	//
	// Free our variables.
	if( fSyncSampleTable != NULL )
		delete[] fSyncSampleTable;
}



// -------------------------------------
// Initialization functions
//
Bool16 QTAtom_stss::Initialize(void)
{
	Bool16		initSucceeds = false;
	UInt32		tempInt32;


	//
	// Parse this atom's fields.
	initSucceeds = ReadInt32(stssPos_VersionFlags, &tempInt32);
	fVersion = (UInt8)((tempInt32 >> 24) & 0x000000ff);
	fFlags = tempInt32 & 0x00ffffff;

	if ( initSucceeds )
	{	
		initSucceeds = ReadInt32(stssPos_NumEntries, &fNumEntries);

		//
		// Validate the size of the sample table.
		if( (unsigned long)(fNumEntries * 4) != (fTOCEntry.AtomDataLength - 8) )
			return false;

		//
		// Read in the sync sample table.
		fSyncSampleTable = NEW char[(fNumEntries * 4) + 1];
		if( fSyncSampleTable == NULL )
			return false;
		
		if( ((PointerSizedInt)fSyncSampleTable & (PointerSizedInt)0x3) == 0)
			fTable = (UInt32 *)fSyncSampleTable;
		else
			fTable = (UInt32 *)(((PointerSizedInt)fSyncSampleTable + 4) & ~((PointerSizedInt)0x3));
		
		initSucceeds = ReadBytes(stssPos_SampleTable, (char *)fTable, fNumEntries * 4);
		
		if ( initSucceeds )
		{
			// This atom has been successfully read in.
			// sample offsets are in network byte order on disk, convert them to host order
			UInt32		sampleIndex = 0;
			
			// convert each sample to host order
			// NOTE - most other Atoms handle byte order conversions in
			// the accessor function.  For efficiency reasons it's converted
			// to host order here for sync samples.
			
			for ( sampleIndex = 0; sampleIndex < fNumEntries; sampleIndex++ )
			{
				fTable[sampleIndex] = ntohl( fTable[sampleIndex] );
			
			}
		
		}
	}
	
	return initSucceeds;
}



// -------------------------------------
// Accessors
//
void QTAtom_stss::PreviousSyncSample(UInt32 SampleNumber, UInt32 *SyncSampleNumber)
{
	//
	// We assume that we won't find an answer
	*SyncSampleNumber = SampleNumber;
	
	//
	// Scan the table until we find a sample number greater than our current
	// sample number; then return that.
	for( UInt32 CurEntry = 0; CurEntry < fNumEntries; CurEntry++ ) {
		//
		// Take this entry if it is before (or equal to) our current entry.
		if( fTable[CurEntry] <= SampleNumber )
			*SyncSampleNumber = fTable[CurEntry];
	}
}

void QTAtom_stss::NextSyncSample(UInt32 SampleNumber, UInt32 *SyncSampleNumber)
{
	//
	// We assume that we won't find an answer
	*SyncSampleNumber = SampleNumber + 1;
	
	//
	// Scan the table until we find a sample number greater than our current
	// sample number; then return that.
	for( UInt32 CurEntry = 0; CurEntry < fNumEntries; CurEntry++ ) {
		//
		// Take this entry if it is greater than our current entry.
		if( fTable[CurEntry] > SampleNumber ) {
			*SyncSampleNumber = fTable[CurEntry];
			break;
		}
	}
}


// -------------------------------------
// Debugging functions
//
void QTAtom_stss::DumpAtom(void)
{
	DEBUG_PRINT(("QTAtom_stss::DumpAtom - Dumping atom.\n"));
	DEBUG_PRINT(("QTAtom_stss::DumpAtom - ..Number of sync sample entries: %ld\n", fNumEntries));
}

void QTAtom_stss::DumpTable(void)
{
	//
	// Print out a header.
	printf("-- Sync Sample table -----------------------------------------------------------\n");
	printf("\n");
	printf("  Entry Num.   Sample Num\n");
	printf("  ----------   ----------\n");
	
	//
	// Print the table.
	for( UInt32 CurEntry = 1; CurEntry <= fNumEntries; CurEntry++ ) {
		//
		// Print out a listing.
		printf("  %10lu : %10lu\n", CurEntry, fTable[CurEntry-1]);
	}
}

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
// $Id: QTAtom_stts.cpp,v 1.2 2001/05/09 21:04:37 cahighlander Exp $
//
// QTAtom_stts:
//   The 'stts' QTAtom class.


// -------------------------------------
// Includes
//
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifndef __Win32__
#include <sys/types.h>
#include <netinet/in.h>
#endif

#include "QTFile.h"

#include "QTAtom.h"
#include "QTAtom_stts.h"
#include "OSMemory.h"


// -------------------------------------
// Constants
//
const int		sttsPos_VersionFlags		=  0;
const int		sttsPos_NumEntries			=  4;
const int		sttsPos_SampleTable			=  8;



// -------------------------------------
// Macros
//
#define DEBUG_PRINT(s) if(fDebug) printf s
#define DEEP_DEBUG_PRINT(s) if(fDeepDebug) printf s



// -------------------------------------
// Class state cookie
//
QTAtom_stts_SampleTableControlBlock::QTAtom_stts_SampleTableControlBlock(void)
{
	Reset();
}

QTAtom_stts_SampleTableControlBlock::~QTAtom_stts_SampleTableControlBlock(void)
{
}

void QTAtom_stts_SampleTableControlBlock::Reset(void)
{
	fMTtSN_CurEntry = 0;
	fMTtSN_CurMediaTime = 0;
	fMTtSN_CurSample = 1;
	
	fSNtMT_CurEntry = 0;
	fSNtMT_CurMediaTime = 0;
	fSNtMT_CurSample = 1;
	
	fGetSampleMediaTime_SampleNumber = 0;
 	fGetSampleMediaTime_MediaTime = 0;

}



// -------------------------------------
// Constructors and destructors
//
QTAtom_stts::QTAtom_stts(QTFile * File, QTFile::AtomTOCEntry * TOCEntry, Bool16 Debug, Bool16 DeepDebug)
	: QTAtom(File, TOCEntry, Debug, DeepDebug),
	  fNumEntries(0), fTimeToSampleTable(NULL)
{
}

QTAtom_stts::~QTAtom_stts(void)
{
	//
	// Free our variables.
	if( fTimeToSampleTable != NULL )
		delete[] fTimeToSampleTable;
}



// -------------------------------------
// Initialization functions
//
Bool16 QTAtom_stts::Initialize(void)
{
	// Temporary vars
	UInt32		tempInt32;


	//
	// Parse this atom's fields.
	ReadInt32(sttsPos_VersionFlags, &tempInt32);
	fVersion = (UInt8)((tempInt32 >> 24) & 0x000000ff);
	fFlags = tempInt32 & 0x00ffffff;

	ReadInt32(sttsPos_NumEntries, &fNumEntries);

	//
	// Validate the size of the sample table.
	if( (unsigned long)(fNumEntries * 8) != (fTOCEntry.AtomDataLength - 8) )
		return false;

	//
	// Read in the time-to-sample table.
	fTimeToSampleTable = NEW char[fNumEntries * 8];
	if( fTimeToSampleTable == NULL )
		return false;
	
	ReadBytes(sttsPos_SampleTable, fTimeToSampleTable, fNumEntries * 8);

	//
	// This atom has been successfully read in.
	return true;
}



// -------------------------------------
// Accessors
//
Bool16 QTAtom_stts::MediaTimeToSampleNumber(UInt32 MediaTime, UInt32 * SampleNumber, QTAtom_stts_SampleTableControlBlock * STCB)
{
	// General vars
	UInt32		SampleCount, SampleDuration;
	QTAtom_stts_SampleTableControlBlock tempSTCB;

	//
	// Use the default STCB if one was not passed in to us.
	if( STCB == NULL )
	{
//		printf("QTAtom_stts::MediaTimeToSampleNumber  ( STCB == NULL ) \n");
		STCB = &tempSTCB;
	}
	//
	// Reconfigure the STCB if necessary.
	if( MediaTime < STCB->fMTtSN_CurMediaTime )
	{
//		printf(" QTAtom_stts::MediaTimeToSampleNumber RESET \n");
		STCB->Reset();
	}
	//
	// Linearly search through the sample table until we find the sample
	// which fits inside the given media time.
	for( ; STCB->fMTtSN_CurEntry < fNumEntries; STCB->fMTtSN_CurEntry++ ) {
		//
		// Copy this sample count and duration.
		memcpy(&SampleCount, fTimeToSampleTable + (STCB->fMTtSN_CurEntry * 8), 4);
		SampleCount = ntohl(SampleCount);
		memcpy(&SampleDuration, fTimeToSampleTable + (STCB->fMTtSN_CurEntry * 8) + 4, 4);
		SampleDuration = ntohl(SampleDuration);

		//
		// Can we skip over this entry?
		if( STCB->fMTtSN_CurMediaTime + (SampleCount * SampleDuration) < MediaTime ) {
			STCB->fMTtSN_CurMediaTime += SampleCount * SampleDuration;
			STCB->fMTtSN_CurSample += SampleCount;
			continue;
		}

		//
		// Locate and return the sample which is/begins right before the
		// given media time.
		if( SampleNumber != NULL )
			*SampleNumber = STCB->fMTtSN_CurSample + ((MediaTime - STCB->fMTtSN_CurMediaTime) / SampleDuration);
		return true;
	}

	//
	// No match; return false.
	return false;
}

Bool16 QTAtom_stts::SampleNumberToMediaTime(UInt32 SampleNumber, UInt32 * MediaTime, QTAtom_stts_SampleTableControlBlock * STCB)
{
	// General vars
	UInt32		SampleCount, SampleDuration;
	QTAtom_stts_SampleTableControlBlock tempSTCB;

	//
	// Use the default STCB if one was not passed in to us.
	if( STCB == NULL )
	{
//		printf("QTAtom_stts::SampleNumberToMediaTime ( STCB == NULL ) \n");
		STCB = &tempSTCB;
	}

	if ( STCB->fGetSampleMediaTime_SampleNumber == SampleNumber)
	{	
//		printf("QTTrack::GetSampleMediaTime cache hit SampleNumber %ld \n", SampleNumber);
		*MediaTime = STCB->fGetSampleMediaTime_MediaTime;
 		return true;
 	}


	//
	// Reconfigure the STCB if necessary.
	if( SampleNumber < STCB->fSNtMT_CurSample )
	{
//		printf(" QTAtom_stts::SampleNumberToMediaTime reset \n");
		STCB->Reset();
	}
	//
	// Linearly search through the sample table until we find the sample
	// which fits inside the given media time.
	for( ; STCB->fSNtMT_CurEntry < fNumEntries; STCB->fSNtMT_CurEntry++ ) {
		//
		// Copy this sample count and duration.
		memcpy(&SampleCount, fTimeToSampleTable + (STCB->fSNtMT_CurEntry * 8), 4);
		SampleCount = ntohl(SampleCount);
		memcpy(&SampleDuration, fTimeToSampleTable + (STCB->fSNtMT_CurEntry * 8) + 4, 4);
		SampleDuration = ntohl(SampleDuration);

		//
		// Can we skip over this entry?
		if( STCB->fSNtMT_CurSample + SampleCount < SampleNumber ) {
			STCB->fSNtMT_CurMediaTime += SampleCount * SampleDuration;
			STCB->fSNtMT_CurSample += SampleCount;
			continue;
		}

		//
		// Return the sample time at the beginning of this sample.
		if( MediaTime != NULL )
			*MediaTime = STCB->fSNtMT_CurMediaTime + ((SampleNumber - STCB->fSNtMT_CurSample) * SampleDuration);

		STCB->fGetSampleMediaTime_SampleNumber = SampleNumber;
 		STCB->fGetSampleMediaTime_MediaTime = *MediaTime;

		return true;
	}

	//
	// No match; return false.
	return false;
}



// -------------------------------------
// Debugging functions
//
void QTAtom_stts::DumpAtom(void)
{
	DEBUG_PRINT(("QTAtom_stts::DumpAtom - Dumping atom.\n"));
	DEBUG_PRINT(("QTAtom_stts::DumpAtom - ..Number of TTS entries: %ld\n", fNumEntries));
}

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
// $Id: QTAtom_stsd.cpp,v 1.1 2001/02/27 00:56:49 cahighlander Exp $
//
// QTAtom_stsd:
//   The 'stsd' QTAtom class.


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
#include "QTAtom_stsd.h"
#include "OSMemory.h"


// -------------------------------------
// Constants
//
const int		stsdPos_VersionFlags		=  0;
const int		stsdPos_NumEntries			=  4;
const int		stsdPos_SampleTable			=  8;

const int		stsdDescPos_Size			=  0;
const int		stsdDescPos_DataFormat		=  4;
const int		stsdDescPos_Index			= 14;



// -------------------------------------
// Macros
//
#define DEBUG_PRINT(s) if(fDebug) printf s
#define DEEP_DEBUG_PRINT(s) if(fDeepDebug) printf s



// -------------------------------------
// Constructors and destructors
//
QTAtom_stsd::QTAtom_stsd(QTFile * File, QTFile::AtomTOCEntry * TOCEntry, Bool16 Debug, Bool16 DeepDebug)
	: QTAtom(File, TOCEntry, Debug, DeepDebug),
	  fNumEntries(0), fSampleDescriptionTable(NULL), fTable(NULL)
{
}

QTAtom_stsd::~QTAtom_stsd(void)
{
	//
	// Free our variables.
	if( fSampleDescriptionTable != NULL )
		delete[] fSampleDescriptionTable;
	if( fTable != NULL )
		delete[] fTable;
}



// -------------------------------------
// Initialization functions
//
Bool16 QTAtom_stsd::Initialize(void)
{
	// Temporary vars
	UInt32		tempInt32;

	// General vars
	char		*pSampleDescriptionTable;


	//
	// Parse this atom's fields.
	ReadInt32(stsdPos_VersionFlags, &tempInt32);
	fVersion = (UInt8)((tempInt32 >> 24) & 0x000000ff);
	fFlags = tempInt32 & 0x00ffffff;

	ReadInt32(stsdPos_NumEntries, &fNumEntries);

	//
	// Read in all of the sample descriptions.
	if( fNumEntries > 0 ) {
		//
		// Allocate our description tables.
		SInt32 tableSize = fTOCEntry.AtomDataLength - 8;
		fSampleDescriptionTable = NEW char[tableSize];
		if( fSampleDescriptionTable == NULL )
			return false;
		
		fTable = NEW char *[fNumEntries];
		if( fTable == NULL )
			return false;
		
		//
		// Read in the sample description table.
		ReadBytes(stsdPos_SampleTable, fSampleDescriptionTable, tableSize);

		//
		// Read them all in..
		pSampleDescriptionTable = fSampleDescriptionTable;
		char		*maxSampleDescriptionPtr = pSampleDescriptionTable + tableSize;
		for( UInt32 CurDesc = 0; CurDesc < fNumEntries; CurDesc++ ) {
			//
			// Associate this entry in our Table with the actual location of
			// this sample description.
			fTable[CurDesc] = pSampleDescriptionTable;
			
			//
			// Skip over this mini-atom.
			memcpy(&tempInt32, pSampleDescriptionTable, 4);
			pSampleDescriptionTable += ntohl(tempInt32);
			if (pSampleDescriptionTable > maxSampleDescriptionPtr)
			{	return false;
			}
		}
	}

	//
	// This atom has been successfully read in.
	return true;
}



// -------------------------------------
// Accessors
//
Bool16 QTAtom_stsd::FindSampleDescription(OSType DataFormat, char ** Buffer, UInt32 * Length)
{
	// Temporary vars
	UInt32		tempInt32;
	
	
	//
	// Go through all of the sample descriptions, looking for the requested
	// entry.
	for( UInt32 CurDesc = 0; CurDesc < fNumEntries; CurDesc++ ) {
		//
		// Get this entry's data format.
		memcpy(&tempInt32, fTable[CurDesc] + stsdDescPos_DataFormat, 4);
		tempInt32 = ntohl(tempInt32);

		//
		// Skip this entry if it does not match.
		if( DataFormat != tempInt32 )
			continue;
		
		//
		// We found a match; return it.
		*Buffer = fTable[CurDesc];

		memcpy(&tempInt32, fTable[CurDesc] + stsdDescPos_Size, 4);
		*Length = ntohl(tempInt32);

		return true;
	}
	
	//
	// No match was found.
	return false;
}

UInt16 QTAtom_stsd::SampleDescriptionToDataReference(UInt32 SampleDescriptionID)
{
	// Temporary vars
	UInt16		tempInt16;
	
	
	//
	// Validate our arguments.
	if( SampleDescriptionID > fNumEntries )
		return 1;
	
	//
	// Find and return the given sample's data reference index.
	memcpy(&tempInt16, fTable[SampleDescriptionID - 1] + stsdDescPos_Index, 2);
	return ntohs(tempInt16);
}



// -------------------------------------
// Debugging functions
//
void QTAtom_stsd::DumpAtom(void)
{
	DEBUG_PRINT(("QTAtom_stsd::DumpAtom - Dumping atom.\n"));
	DEBUG_PRINT(("QTAtom_stsd::DumpAtom - ..Number of sample description entries: %ld\n", fNumEntries));
}

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
// $Id: QTAtom_stsz.cpp,v 1.1 2001/02/27 00:56:49 cahighlander Exp $
//
// QTAtom_stsz:
//   The 'stsz' QTAtom class.


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
#include "QTAtom_stsz.h"
#include "OSMemory.h"


// -------------------------------------
// Constants
//
const int		stszPos_VersionFlags		=  0;
const int		stszPos_SampleSize			=  4;
const int		stszPos_NumEntries			=  8;
const int		stszPos_SampleTable			= 12;



// -------------------------------------
// Macros
//
#define DEBUG_PRINT(s) if(fDebug) printf s
#define DEEP_DEBUG_PRINT(s) if(fDeepDebug) printf s



// -------------------------------------
// Constructors and destructors
//
QTAtom_stsz::QTAtom_stsz(QTFile * File, QTFile::AtomTOCEntry * TOCEntry, Bool16 Debug, Bool16 DeepDebug)
	: QTAtom(File, TOCEntry, Debug, DeepDebug),
	  fCommonSampleSize(0),
	  fNumEntries(0), fSampleSizeTable(NULL), fTable(NULL)
{
}

QTAtom_stsz::~QTAtom_stsz(void)
{
	//
	// Free our variables.
	if( fSampleSizeTable != NULL )
		delete[] fSampleSizeTable;
}



// -------------------------------------
// Initialization functions
//
Bool16 QTAtom_stsz::Initialize(void)
{
	// Temporary vars
	UInt32		tempInt32;


	//
	// Parse this atom's fields.
	ReadInt32(stszPos_VersionFlags, &tempInt32);
	fVersion = (UInt8)((tempInt32 >> 24) & 0x000000ff);
	fFlags = tempInt32 & 0x00ffffff;

	ReadInt32(stszPos_SampleSize, &fCommonSampleSize);
	
	//
	// We don't need to read in the table (it doesn't exist anyway) if the
	// SampleSize field is non-zero.
	if( fCommonSampleSize != 0 )
		return true;


	//
	// Build the table..
	ReadInt32(stszPos_NumEntries, &fNumEntries);

	//
	// Validate the size of the sample table.
	if( (unsigned long)(fNumEntries * 4) != (fTOCEntry.AtomDataLength - 12) )
		return false;

	//
	// Read in the sample size table.
	fSampleSizeTable = NEW char[(fNumEntries * 4) + 1];
	if( fSampleSizeTable == NULL )
		return false;
	
	if( ((UInt32)fSampleSizeTable & (UInt32)0x3) == 0)
		fTable = (UInt32 *)fSampleSizeTable;
	else
		fTable = (UInt32 *)(((UInt32)fSampleSizeTable + 4) & ~((UInt32)0x3));
	
	ReadBytes(stszPos_SampleTable, (char *)fTable, fNumEntries * 4);

	//
	// This atom has been successfully read in.
	return true;
}

Bool16 QTAtom_stsz::SampleRangeSize(UInt32 firstSampleNumber, UInt32 lastSampleNumber, UInt32 *sizePtr)
{	
	Bool16 result = false;
	

	do 
	{
		if (lastSampleNumber < firstSampleNumber) 
		{
//			printf("QTAtom_stsz::SampleRangeSize (lastSampleNumber %ld < firstSampleNumber %ld) \n",lastSampleNumber, firstSampleNumber);
			break;
		}
		
		if(fCommonSampleSize) 
		{ 
//			printf("QTAtom_stsz::SampleRangeSize fCommonSampleSize %ld firstSampleNumber %ld lastSampleNumber %ld *sizePtr %ld\n",fCommonSampleSize,firstSampleNumber,lastSampleNumber,*sizePtr);
			if( sizePtr != NULL ) 
				*sizePtr = fCommonSampleSize * (lastSampleNumber - firstSampleNumber + 1) ; 
				
			result =  true; 
			break;
		} 
									
		if(firstSampleNumber && lastSampleNumber && (lastSampleNumber<=fNumEntries) && (firstSampleNumber <= fNumEntries) ) 
		{
			if( sizePtr != NULL ) 
			{	*sizePtr = 0;
				
				for (UInt32 sampleNumber = firstSampleNumber; sampleNumber <= lastSampleNumber; sampleNumber++ ) 
					*sizePtr += ntohl(fTable[sampleNumber-1]);
				
			}
			result =  true; 
			break;
		}
	
	} 
	while (false);
							
	return result; 
}

// -------------------------------------
// Debugging functions
//
void QTAtom_stsz::DumpAtom(void)
{
	DEBUG_PRINT(("QTAtom_stsz::DumpAtom - Dumping atom.\n"));
	DEBUG_PRINT(("QTAtom_stsz::DumpAtom - ..Number of sample size entries: %ld\n", fNumEntries));
}

void QTAtom_stsz::DumpTable(void)
{
	//
	// Print out a header.
	printf("-- Sample Size table -----------------------------------------------------------\n");
	printf("\n");
	printf("  Sample Num   SampleSize\n");
	printf("  ----------   ----------\n");
	
	//
	// Print the table.
	for( UInt32 CurEntry = 1; CurEntry <= fNumEntries; CurEntry++ ) {
		//
		// Print out a listing.
		printf("  %10lu : %10lu\n", CurEntry, fTable[CurEntry-1]);
	}
}
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
// $Id: QTAtom_elst.cpp,v 1.2 2001/05/09 21:04:37 cahighlander Exp $
//
// QTAtom_elst:
//   The 'elst' QTAtom class.


// -------------------------------------
// Includes
//
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "QTFile.h"

#include "QTAtom.h"
#include "QTAtom_elst.h"
#include "OSMemory.h"



// -------------------------------------
// Macros
//
#define DEBUG_PRINT(s) if(fDebug) printf s
#define DEEP_DEBUG_PRINT(s) if(fDeepDebug) printf s



// -------------------------------------
// Constants
//
const int		elstPos_VersionFlags		=  0;
const int		elstPos_NumEdits			=  4;
const int		elstPos_EditList			=  8;

const int		elstEntryPos_Duration		=  0;
const int		elstEntryPos_MediaTime		=  4;
const int		elstEntryPos_MediaRate		=  8;



// -------------------------------------
// Constructors and destructors
//
QTAtom_elst::QTAtom_elst(QTFile * File, QTFile::AtomTOCEntry * TOCEntry, Bool16 Debug, Bool16 DeepDebug)
	: QTAtom(File, TOCEntry, Debug, DeepDebug),
	  fNumEdits(0), fEdits(NULL),
	  fFirstEditMovieTime(0)
{
}

QTAtom_elst::~QTAtom_elst(void)
{
	//
	// Free our variables.
	if( fEdits != NULL )
		delete[] fEdits;
}



// -------------------------------------
// Initialization functions
//
Bool16 QTAtom_elst::Initialize(void)
{
	// Temporary vars
	UInt32		tempInt32;


	DEEP_DEBUG_PRINT(("Processing 'elst' atom.\n"));

	//
	// Parse this atom's fields.
	ReadInt32(elstPos_VersionFlags, &tempInt32);
	fVersion = (UInt8)((tempInt32 >> 24) & 0x000000ff);
	fFlags = tempInt32 & 0x00ffffff;

	ReadInt32(elstPos_NumEdits, &fNumEdits);

	//
	// Read in all of the edits.
	if( fNumEdits > 0 ) {
		DEEP_DEBUG_PRINT(("..%lu edits found.\n", fNumEdits));

		//
		// Allocate our ref table.
		fEdits = NEW EditListEntry[fNumEdits];
		if( fEdits == NULL )
			return false;

		//
		// Read them all in..
		for( UInt32 CurEdit = 0; CurEdit < fNumEdits; CurEdit++ ) {
			//
			// Get all of the data in this edit list entry.
			ReadInt32(elstPos_EditList + (CurEdit * 12) + elstEntryPos_Duration, &tempInt32);
			fEdits[CurEdit].EditDuration = tempInt32;

			ReadInt32(elstPos_EditList + (CurEdit * 12) + elstEntryPos_MediaTime, &tempInt32);
			fEdits[CurEdit].StartingMediaTime = (SInt32)tempInt32;

			ReadInt32(elstPos_EditList + (CurEdit * 12) + elstEntryPos_MediaRate, &tempInt32);
			fEdits[CurEdit].EditMediaRate = tempInt32;
			
			DEEP_DEBUG_PRINT(("..Edit #%lu: Duration=%lu; MediaTime=%ld\n", CurEdit, fEdits[CurEdit].EditDuration, fEdits[CurEdit].StartingMediaTime));

			//
			// Adjust our starting media time.
			if( fEdits[CurEdit].StartingMediaTime == -1 )
				fFirstEditMovieTime = fEdits[CurEdit].EditDuration;
		}
	}

	//
	// This atom has been successfully read in.
	return true;
}



// -------------------------------------
// Debugging functions
//
void QTAtom_elst::DumpAtom(void)
{
	DEBUG_PRINT(("QTAtom_elst::DumpAtom - Dumping atom.\n"));
	DEBUG_PRINT(("QTAtom_elst::DumpAtom - ..Number of edits: %ld\n", fNumEdits));
}

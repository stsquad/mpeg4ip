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
// $Id: QTAtom_mdhd.cpp,v 1.2 2001/05/09 21:04:37 cahighlander Exp $
//
// QTAtom_mdhd:
//   The 'mdhd' QTAtom class.


// -------------------------------------
// Includes
//
#include <stdio.h>
#include <time.h>

#include "QTFile.h"

#include "QTAtom.h"
#include "QTAtom_mdhd.h"



// -------------------------------------
// Constants
//
const int		mdhdPos_VersionFlags		=  0;
const int		mdhdPos_CreationTime		=  4;
const int		mdhdPos_ModificationTime	=  8;
const int		mdhdPos_TimeScale			= 12;
const int		mdhdPos_Duration			= 16;
const int		mdhdPos_Language			= 20;
const int		mdhdPos_Quality				= 22;



// -------------------------------------
// Macros
//
#define DEBUG_PRINT(s) if(fDebug) printf s
#define DEEP_DEBUG_PRINT(s) if(f7DeepDebug) printf s



// -------------------------------------
// Constructors and destructors
//
QTAtom_mdhd::QTAtom_mdhd(QTFile * File, QTFile::AtomTOCEntry * TOCEntry, Bool16 Debug, Bool16 DeepDebug)
	: QTAtom(File, TOCEntry, Debug, DeepDebug)
{
}

QTAtom_mdhd::~QTAtom_mdhd(void)
{
}



// -------------------------------------
// Initialization functions
//
Bool16 QTAtom_mdhd::Initialize(void)
{
	// Temporary vars
	UInt32		tempInt32;


	//
	// Verify that this atom is the correct length.
	if( fTOCEntry.AtomDataLength != 24 )
		return false;

	//
	// Parse this atom's fields.
	ReadInt32(mdhdPos_VersionFlags, &tempInt32);
	fVersion = (UInt8)((tempInt32 >> 24) & 0x000000ff);
	fFlags = tempInt32 & 0x00ffffff;

	ReadInt32(mdhdPos_CreationTime, &fCreationTime);
	ReadInt32(mdhdPos_ModificationTime, &fModificationTime);
	ReadInt32(mdhdPos_TimeScale, &fTimeScale);
	ReadInt32(mdhdPos_Duration, &fDuration);
	ReadInt16(mdhdPos_Language, &fLanguage);
	ReadInt16(mdhdPos_Quality, &fQuality);

	//
	// Compute the reciprocal of the timescale.
	fTimeScaleRecip = 1 / (Float64)fTimeScale;
	
	//
	// This atom has been successfully read in.
	return true;
}



// -------------------------------------
// Debugging functions
//
void QTAtom_mdhd::DumpAtom(void)
{
	// Temporary vars
	time_t		unixCreationTime = fCreationTime + QT_TIME_TO_LOCAL_TIME;


	DEBUG_PRINT(("QTAtom_mdhd::DumpAtom - Dumping atom.\n"));
	DEBUG_PRINT(("QTAtom_mdhd::DumpAtom - ..Creation date: %s", asctime(gmtime(&unixCreationTime))));
}

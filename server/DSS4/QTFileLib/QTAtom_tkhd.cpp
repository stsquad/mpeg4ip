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
// $Id: QTAtom_tkhd.cpp,v 1.1 2002/02/27 19:49:01 wmaycisco Exp $
//
// QTAtom_tkhd:
//   The 'tkhd' QTAtom class.


// -------------------------------------
// Includes
//
#include <stdio.h>
#include <time.h>

#include "QTFile.h"

#include "QTAtom.h"
#include "QTAtom_tkhd.h"



// -------------------------------------
// Constants
//
const int		tkhdPos_VersionFlags		=  0;
const int		tkhdPos_CreationTime		=  4;
const int		tkhdPos_ModificationTime	=  8;
const int		tkhdPos_TrackID				= 12;
const int		tkhdPos_Duration			= 20;
const int		tkhdPos_Layer				= 32;
const int		tkhdPos_AlternateGroup		= 34;
const int		tkhdPos_Volume				= 36;
const int		tkhdPos_a					= 40;
const int		tkhdPos_b					= 44;
const int		tkhdPos_u					= 48;
const int		tkhdPos_c					= 52;
const int		tkhdPos_d					= 56;
const int		tkhdPos_v					= 60;
const int		tkhdPos_x					= 64;
const int		tkhdPos_y					= 68;
const int		tkhdPos_w					= 72;
const int		tkhdPos_TrackWidth			= 76;
const int		tkhdPos_TrackHeight			= 80;



// -------------------------------------
// Macros
//
#define DEBUG_PRINT(s) if(fDebug) printf s
#define DEEP_DEBUG_PRINT(s) if(f7DeepDebug) printf s



// -------------------------------------
// Constructors and destructors
//
QTAtom_tkhd::QTAtom_tkhd(QTFile * File, QTFile::AtomTOCEntry * TOCEntry, Bool16 Debug, Bool16 DeepDebug)
	: QTAtom(File, TOCEntry, Debug, DeepDebug)
{
}

QTAtom_tkhd::~QTAtom_tkhd(void)
{
}



// -------------------------------------
// Initialization functions
//
Bool16 QTAtom_tkhd::Initialize(void)
{
	// Temporary vars
	UInt32		tempInt32;


	//
	// Verify that this atom is the correct length.
	if( fTOCEntry.AtomDataLength != 84 )
		return false;

	//
	// Parse this atom's fields.
	ReadInt32(tkhdPos_VersionFlags, &tempInt32);
	fVersion = (UInt8)((tempInt32 >> 24) & 0x000000ff);
	fFlags = tempInt32 & 0x00ffffff;

	ReadInt32(tkhdPos_CreationTime, &fCreationTime);
	ReadInt32(tkhdPos_ModificationTime, &fModificationTime);
	ReadInt32(tkhdPos_TrackID, &fTrackID);
	ReadInt32(tkhdPos_Duration, &fDuration);
	ReadInt16(tkhdPos_AlternateGroup, &fAlternateGroup);
	ReadInt16(tkhdPos_Volume, &fVolume);

	ReadInt32(tkhdPos_a, &fa);
	ReadInt32(tkhdPos_b, &fb);
	ReadInt32(tkhdPos_u, &fu);
	ReadInt32(tkhdPos_c, &fc);
	ReadInt32(tkhdPos_d, &fd);
	ReadInt32(tkhdPos_v, &fv);
	ReadInt32(tkhdPos_x, &fx);
	ReadInt32(tkhdPos_y, &fy);
	ReadInt32(tkhdPos_w, &fw);

	ReadInt32(tkhdPos_TrackWidth, &fTrackWidth);
	ReadInt32(tkhdPos_TrackHeight, &fTrackHeight);

	//
	// This atom has been successfully read in.
	return true;
}



// -------------------------------------
// Debugging functions
//
void QTAtom_tkhd::DumpAtom(void)
{
	// Temporary vars
	time_t		unixCreationTime = (time_t)fCreationTime + (time_t)QT_TIME_TO_LOCAL_TIME;


	DEBUG_PRINT(("QTAtom_tkhd::DumpAtom - Dumping atom.\n"));
	DEBUG_PRINT(("QTAtom_tkhd::DumpAtom - ..Track ID: %ld\n", fTrackID));
	DEBUG_PRINT(("QTAtom_tkhd::DumpAtom - ..Flags:%s%s%s%s\n", (fFlags & flagEnabled) ? " Enabled" : "", (fFlags & flagInMovie) ? " InMovie" : "", (fFlags & flagInPreview) ? " InPreview" : "", (fFlags & flagInPoster) ? " InPoster" : ""));
	DEBUG_PRINT(("QTAtom_tkhd::DumpAtom - ..Creation date: %s", asctime(gmtime(&unixCreationTime))));
}

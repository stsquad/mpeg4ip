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
// $Id: QTAtom_mvhd.cpp,v 1.1 2002/02/27 19:49:01 wmaycisco Exp $
//
// QTAtom_mvhd:
//   The 'mvhd' QTAtom class.


// -------------------------------------
// Includes
//
#include <stdio.h>
#include <time.h>

#include "QTFile.h"

#include "QTAtom.h"
#include "QTAtom_mvhd.h"



// -------------------------------------
// Constants
//
const int		mvhdPos_VersionFlags		=  0;
const int		mvhdPos_CreationTime		=  4;
const int		mvhdPos_ModificationTime	=  8;
const int		mvhdPos_TimeScale			= 12;
const int		mvhdPos_Duration			= 16;
const int		mvhdPos_PreferredRate		= 20;
const int		mvhdPos_PreferredVolume		= 24;
const int		mvhdPos_a					= 36;
const int		mvhdPos_b					= 40;
const int		mvhdPos_u					= 44;
const int		mvhdPos_c					= 48;
const int		mvhdPos_d					= 52;
const int		mvhdPos_v					= 56;
const int		mvhdPos_x					= 60;
const int		mvhdPos_y					= 64;
const int		mvhdPos_w					= 68;
const int		mvhdPos_PreviewTime			= 72;
const int		mvhdPos_PreviewDuration		= 76;
const int		mvhdPos_PosterTime			= 80;
const int		mvhdPos_SelectionTime		= 84;
const int		mvhdPos_SelectionDuration	= 88;
const int		mvhdPos_CurrentTime			= 92;
const int		mvhdPos_NextTrackID			= 96;



// -------------------------------------
// Macros
//
#define DEBUG_PRINT(s) if(fDebug) printf s
#define DEEP_DEBUG_PRINT(s) if(fDeepDebug) printf s



// -------------------------------------
// Constructors and destructors
//
QTAtom_mvhd::QTAtom_mvhd(QTFile * File, QTFile::AtomTOCEntry * TOCEntry, Bool16 Debug, Bool16 DeepDebug)
	: QTAtom(File, TOCEntry, Debug, DeepDebug)
{
}

QTAtom_mvhd::~QTAtom_mvhd(void)
{
}



// -------------------------------------
// Initialization functions
//
Bool16 QTAtom_mvhd::Initialize(void)
{
	// Temporary vars
	UInt32		tempInt32;


	//
	// Verify that this atom is the correct length.
	if( fTOCEntry.AtomDataLength != 100 )
		return false;

	//
	// Parse this atom's fields.
	ReadInt32(mvhdPos_VersionFlags, &tempInt32);
	fVersion = (UInt8)((tempInt32 >> 24) & 0x000000ff);
	fFlags = tempInt32 & 0x00ffffff;

	ReadInt32(mvhdPos_CreationTime, &fCreationTime);
	ReadInt32(mvhdPos_ModificationTime, &fModificationTime);
	ReadInt32(mvhdPos_TimeScale, &fTimeScale);
	ReadInt32(mvhdPos_Duration, &fDuration);
	ReadInt32(mvhdPos_PreferredRate, &fPreferredRate);
	ReadInt16(mvhdPos_PreferredVolume, &fPreferredVolume);

	ReadInt32(mvhdPos_a, &fa);
	ReadInt32(mvhdPos_b, &fb);
	ReadInt32(mvhdPos_u, &fu);
	ReadInt32(mvhdPos_c, &fc);
	ReadInt32(mvhdPos_d, &fd);
	ReadInt32(mvhdPos_v, &fv);
	ReadInt32(mvhdPos_x, &fx);
	ReadInt32(mvhdPos_y, &fy);
	ReadInt32(mvhdPos_w, &fw);

	ReadInt32(mvhdPos_PreviewTime, &fPreviewTime);
	ReadInt32(mvhdPos_PreviewDuration, &fPreviewDuration);
	ReadInt32(mvhdPos_PosterTime, &fPosterTime);
	ReadInt32(mvhdPos_SelectionTime, &fSelectionTime);
	ReadInt32(mvhdPos_SelectionDuration, &fSelectionDuration);
	ReadInt32(mvhdPos_CurrentTime, &fCurrentTime);
	ReadInt32(mvhdPos_NextTrackID, &fNextTrackID);

	//
	// This atom has been successfully read in.
	return true;
}



// -------------------------------------
// Debugging functions
//
void QTAtom_mvhd::DumpAtom(void)
{
	// Temporary vars
	time_t		unixCreationTime = (time_t)fCreationTime + (time_t)QT_TIME_TO_LOCAL_TIME;


	DEBUG_PRINT(("QTAtom_mvhd::DumpAtom - Dumping atom.\n"));
	DEBUG_PRINT(("QTAtom_mvhd::DumpAtom - ..Creation date: %s", asctime(gmtime(&unixCreationTime))));
	DEBUG_PRINT(("QTAtom_mvhd::DumpAtom - ..Movie duration: %.2f seconds\n", GetDurationInSeconds()));
}

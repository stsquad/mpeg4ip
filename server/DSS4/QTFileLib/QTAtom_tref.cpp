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
// $Id: QTAtom_tref.cpp,v 1.1 2002/02/27 19:49:01 wmaycisco Exp $
//
// QTAtom_tref:
//   The 'tref' QTAtom class.


// -------------------------------------
// Includes
//
#include <stdio.h>
#include <stdlib.h>

#include "QTFile.h"

#include "QTAtom.h"
#include "QTAtom_tref.h"
#include "OSMemory.h"


// -------------------------------------
// Constants
//
const int		trefPos_SampleTable			=  0;



// -------------------------------------
// Macros
//
#define DEBUG_PRINT(s) if(fDebug) printf s
#define DEEP_DEBUG_PRINT(s) if(fDeepDebug) printf s



// -------------------------------------
// Constructors and destructors
//
QTAtom_tref::QTAtom_tref(QTFile * File, QTFile::AtomTOCEntry * TOCEntry, Bool16 Debug, Bool16 DeepDebug)
	: QTAtom(File, TOCEntry, Debug, DeepDebug),
	  fNumEntries(0), fTrackReferenceTable(NULL), fTable(NULL)
{
}

QTAtom_tref::~QTAtom_tref(void)
{
	//
	// Free our variables.
	if( fTrackReferenceTable != NULL )
		delete[] fTrackReferenceTable;
}



// -------------------------------------
// Initialization functions
//
Bool16 QTAtom_tref::Initialize(void)
{
	//
	// Compute the size of the sample table.
	fNumEntries = fTOCEntry.AtomDataLength / 4;

	//
	// Read in the track reference table.
	fTrackReferenceTable = NEW char[(fNumEntries * 4) + 1];
	if( fTrackReferenceTable == NULL )
		return false;
	
	if( ((PointerSizedInt)fTrackReferenceTable & (PointerSizedInt)0x3) == 0)
		fTable = (UInt32 *)fTrackReferenceTable;
	else
		fTable = (UInt32 *)(((PointerSizedInt)fTrackReferenceTable + 4) & ~((PointerSizedInt)0x3));
	
	ReadBytes(trefPos_SampleTable, (char *)fTable, fNumEntries * 4);

	//
	// This atom has been successfully read in.
	return true;
}



// -------------------------------------
// Debugging functions
//
void QTAtom_tref::DumpAtom(void)
{
	DEBUG_PRINT(("QTAtom_tref::DumpAtom - Dumping atom.\n"));
	DEBUG_PRINT(("QTAtom_tref::DumpAtom - ..Number of track reference entries: %ld\n", fNumEntries));
}

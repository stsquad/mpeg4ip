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
// $Id: QTAtom_tref.h,v 1.3 2001/10/11 20:39:08 wmaycisco Exp $
//
// QTAtom_tref:
//   The 'tref' QTAtom class.

#ifndef QTAtom_tref_H
#define QTAtom_tref_H

//
// Includes
#ifndef __Win32__
#include <netinet/in.h>
#endif

#include "QTFile.h"
#include "QTAtom.h"


//
// QTAtom class
class QTAtom_tref : public QTAtom {

public:
	//
	// Constructors and destructor.
						QTAtom_tref(QTFile * File, QTFile::AtomTOCEntry * Atom,
							   Bool16 Debug = false, Bool16 DeepDebug = false);
	virtual				~QTAtom_tref(void);


	//
	// Initialization functions.
	virtual	Bool16		Initialize(void);
	
	//
	// Accessors.
	inline	UInt32		GetNumReferences(void) { return fNumEntries; }
	inline	Bool16		TrackReferenceToTrackID(UInt32 TrackReference, UInt32 * TrackID = NULL) \
							{	if(TrackReference < fNumEntries) { \
									if( TrackID != NULL ) \
										*TrackID = ntohl(fTable[TrackReference]); \
									return true; \
								} else \
									return false; \
							}


	//
	// Debugging functions.
	virtual	void		DumpAtom(void);


protected:
	//
	// Protected member variables.
	UInt32		fNumEntries;
	char		*fTrackReferenceTable;
	UInt32		*fTable; // longword-aligned version of the above
};

#endif // QTAtom_tref_H

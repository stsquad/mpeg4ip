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
 * @APPLE_LICENSE_HEADER_END@
 */

// $Id: QTAtom_dref.h,v 1.1 2001/02/27 00:56:49 cahighlander Exp $
//
// QTAtom_dref:
//   The 'dref' QTAtom class.

#ifndef QTAtom_dref_H
#define QTAtom_dref_H


//
// Includes
#include "QTFile.h"
#include "QTAtom.h"


//
// External classes
class QTFile_FileControlBlock;


//
// QTAtom class
class QTAtom_dref : public QTAtom {
	//
	// Class constants
	enum {
		flagSelfRef		= 0x00000001
	};


	//
	// Class typedefs.
	struct DataRefEntry {
		// Data ref information
		UInt32			Flags;
		OSType			ReferenceType;
		UInt32			DataLength;
		char			*Data;
		
		// Tracking information
		Bool16			IsEntryInitialized, IsFileOpen;
		QTFile_FileControlBlock	*FCB;
	};


public:
	//
	// Constructors and destructor.
						QTAtom_dref(QTFile * File, QTFile::AtomTOCEntry * Atom,
							   Bool16 Debug = false, Bool16 DeepDebug = false);
	virtual				~QTAtom_dref(void);


	//
	// Initialization functions.
	virtual	Bool16		Initialize(void);

	//
	// Accessors.
	virtual	Bool16		IsRefInThisFile(UInt32 RefID) { if(RefID && (RefID<=fNumRefs)) \
															return (fRefs[RefID-1].Flags & flagSelfRef); \
														return false; }

	//
	// Read functions.
			Bool16		Read(UInt32 RefID, UInt64 Offset, char * const Buffer, UInt32 Length,
							 QTFile_FileControlBlock * FCB = NULL);


	//
	// Debugging functions.
	virtual	void		DumpAtom(void);


protected:
	//
	// Protected member functions.
			char *		ResolveAlias(char * const AliasData, UInt32 AliasDataLength);


	//
	// Protected member variables.
	UInt8		fVersion;
	UInt32		fFlags; // 24 bits in the low 3 bytes

	UInt32			fNumRefs;
	DataRefEntry	*fRefs;	
};

#endif // QTAtom_dref_H

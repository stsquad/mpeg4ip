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
// $Id: QTAtom_stsd.h,v 1.1 2001/02/27 00:56:49 cahighlander Exp $
//
// QTAtom_stsd:
//   The 'stsd' QTAtom class.

#ifndef QTAtom_stsd_H
#define QTAtom_stsd_H


//
// Includes
#include "QTFile.h"
#include "QTAtom.h"


//
// QTAtom class
class QTAtom_stsd : public QTAtom {

public:
	//
	// Constructors and destructor.
						QTAtom_stsd(QTFile * File, QTFile::AtomTOCEntry * Atom,
							   Bool16 Debug = false, Bool16 DeepDebug = false);
	virtual				~QTAtom_stsd(void);


	//
	// Initialization functions.
	virtual	Bool16		Initialize(void);
	
	//
	// Accessors.
	virtual	Bool16		FindSampleDescription(OSType DataFormat, char ** Buffer, UInt32 * Length);
	virtual	UInt16		SampleDescriptionToDataReference(UInt32 SampleDescriptionID);


	//
	// Debugging functions.
	virtual	void		DumpAtom(void);


protected:
	//
	// Protected member variables.
	UInt8		fVersion;
	UInt32		fFlags; // 24 bits in the low 3 bytes

	UInt32		fNumEntries;
	char		*fSampleDescriptionTable;
	char		**fTable; // each entry points to the start of a sample
						  // description entry in the above memory area
};

#endif // QTAtom_stsd_H

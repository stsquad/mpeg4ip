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
// $Id: QTAtom_stts.h,v 1.2 2001/05/09 21:04:37 cahighlander Exp $
//
// QTAtom_stts:
//   The 'stts' QTAtom class.

#ifndef QTAtom_stts_H
#define QTAtom_stts_H


//
// Includes
#include "QTFile.h"
#include "QTAtom.h"


//
// Class state cookie
class QTAtom_stts_SampleTableControlBlock {

public:
	//
	// Constructor and destructor.
						QTAtom_stts_SampleTableControlBlock(void);
	virtual				~QTAtom_stts_SampleTableControlBlock(void);
	
	//
	// Reset function
			void		Reset(void);

	//
	// MT->SN Sample table cache
	UInt32				fMTtSN_CurEntry;
	UInt32				fMTtSN_CurMediaTime, fMTtSN_CurSample;
	
	//
	/// SN->MT Sample table cache
	UInt32				fSNtMT_CurEntry;
	UInt32				fSNtMT_CurMediaTime, fSNtMT_CurSample;
	
	UInt32				fGetSampleMediaTime_SampleNumber;
 	UInt32				fGetSampleMediaTime_MediaTime;

};


//
// QTAtom class
class QTAtom_stts : public QTAtom {

public:
	//
	// Constructors and destructor.
						QTAtom_stts(QTFile * File, QTFile::AtomTOCEntry * Atom,
							   Bool16 Debug = false, Bool16 DeepDebug = false);
	virtual				~QTAtom_stts(void);


	//
	// Initialization functions.
	virtual	Bool16		Initialize(void);
	
	//
	// Accessors.
			Bool16		MediaTimeToSampleNumber(UInt32 MediaTime, UInt32 * SampleNumber = NULL,
												QTAtom_stts_SampleTableControlBlock * STCB = NULL);
			Bool16		SampleNumberToMediaTime(UInt32 SampleNumber, UInt32 * MediaTime = NULL,
												QTAtom_stts_SampleTableControlBlock * STCB = NULL);


	//
	// Debugging functions.
	virtual	void		DumpAtom(void);


protected:
	//
	// Protected member variables.
	UInt8		fVersion;
	UInt32		fFlags; // 24 bits in the low 3 bytes

	UInt32		fNumEntries;
	char		*fTimeToSampleTable;
	
};

#endif // QTAtom_stts_H

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
// $Id: QTAtom_mvhd.h,v 1.2 2001/05/09 21:04:37 cahighlander Exp $
//
// QTAtom_mvhd:
//   The 'mvhd' QTAtom class.

#ifndef QTAtom_mvhd_H
#define QTAtom_mvhd_H


//
// Includes
#include "QTFile.h"
#include "QTAtom.h"


//
// QTAtom class
class QTAtom_mvhd : public QTAtom {

public:
	//
	// Constructors and destructor.
						QTAtom_mvhd(QTFile * File, QTFile::AtomTOCEntry * Atom,
							   Bool16 Debug = false, Bool16 DeepDebug = false);
	virtual				~QTAtom_mvhd(void);


	//
	// Initialization functions.
	virtual	Bool16		Initialize(void);
	
	//
	// Accessors.
	inline	Float64		GetTimeScale(void) { return (Float64)fTimeScale; }
	inline	Float64		GetDurationInSeconds(void) { if (fTimeScale != 0){ return fDuration / (Float64)fTimeScale; } else {return (Float64) 0.0;} }


	//
	// Debugging functions.
	virtual	void		DumpAtom(void);


protected:
	//
	// Protected member variables.
	UInt8		fVersion;
	UInt32		fFlags; // 24 bits in the low 3 bytes
	UInt32		fCreationTime, fModificationTime;
	UInt32		fTimeScale, fDuration;
	UInt32		fPreferredRate;
	UInt16		fPreferredVolume;
	UInt32		fa, fb, fu, fc, fd, fv, fx, fy, fw;
	UInt32		fPreviewTime, fPreviewDuration, fPosterTime;
	UInt32		fSelectionTime, fSelectionDuration;
	UInt32		fCurrentTime;
	UInt32		fNextTrackID;
};

#endif // QTAtom_mvhd_H

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
// $Id: QTAtom_mdhd.h,v 1.1 2002/02/27 19:49:01 wmaycisco Exp $
//
// QTAtom_mdhd:
//   The 'mdhd' QTAtom class.

#ifndef QTAtom_mdhd_H
#define QTAtom_mdhd_H


//
// Includes
#include "OSHeaders.h"

#include "QTFile.h"
#include "QTAtom.h"


//
// QTAtom class
class QTAtom_mdhd : public QTAtom {

public:
	//
	// Constructors and destructor.
						QTAtom_mdhd(QTFile * File, QTFile::AtomTOCEntry * Atom,
							   Bool16 Debug = false, Bool16 DeepDebug = false);
	virtual				~QTAtom_mdhd(void);


	//
	// Initialization functions.
	virtual	Bool16		Initialize(void);

	//
	// Accessors.
	inline	Float64		GetTimeScale(void) { return (Float64)fTimeScale; }
	inline	Float64		GetTimeScaleRecip(void) { return fTimeScaleRecip; }


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
	UInt16		fLanguage;
	UInt16		fQuality;
	
	Float64		fTimeScaleRecip;
};

#endif // QTAtom_mdhd_H

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
// $Id: QTAtom.cpp,v 1.3 2001/10/11 20:39:08 wmaycisco Exp $
//
// QTAtom:
//   The base-class for atoms in a QuickTime file.


// -------------------------------------
// Includes
//
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef __Win32__
#include <sys/types.h>
#include <netinet/in.h>
#endif

#include "QTFile.h"
#include "QTAtom.h"



// -------------------------------------
// Macros
//
#define DEBUG_PRINT(s) if(fDebug) printf s
#define DEEP_DEBUG_PRINT(s) if(fDeepDebug) printf s



// -------------------------------------
// Constructors and destructors
//
QTAtom::QTAtom(QTFile * File, QTFile::AtomTOCEntry * Atom, Bool16 Debug, Bool16 DeepDebug)
	: fDebug(Debug), fDeepDebug(DeepDebug),
	  fFile(File)
{
	//
	// Make a copy of the TOC entry.
	memcpy(&fTOCEntry, Atom, sizeof(QTFile::AtomTOCEntry));
}

QTAtom::~QTAtom(void)
{
}



// -------------------------------------
// Read functions
//
Bool16 QTAtom::ReadBytes(UInt64 Offset, char * Buffer, UInt32 Length)
{
	//
	// Validate the arguments.
	if( (Offset + Length) > fTOCEntry.AtomDataLength )
		return false;

	//
	// Read and return this data.
	return fFile->Read(fTOCEntry.AtomDataPos + Offset, Buffer, Length);
}

Bool16 QTAtom::ReadInt8(UInt64 Offset, UInt8 * Datum)
{
	//
	// Read and return.
	return ReadBytes(Offset, (char *)Datum, 1);
}

Bool16 QTAtom::ReadInt16(UInt64 Offset, UInt16 * Datum)
{
	// General vars
	UInt16		tempDatum;


	//
	// Read and flip.
	if( !ReadBytes(Offset, (char *)&tempDatum, 2) )
		return false;
	
	*Datum = ntohs(tempDatum);
	return true;
}

Bool16 QTAtom::ReadInt32(UInt64 Offset, UInt32 * Datum)
{
	// General vars
	UInt32		tempDatum;


	//
	// Read and flip.
	if( !ReadBytes(Offset, (char *)&tempDatum, 4) )
		return false;
	
	*Datum = ntohl(tempDatum);
	return true;
}



Bool16 QTAtom::ReadSubAtomBytes(const char * AtomPath, char * Buffer, UInt32 Length)
{
	// General vars
	QTFile::AtomTOCEntry	*atomTOCEntry;
	

	//
	// Find the TOC entry for this sub-atom.
	if( !fFile->FindTOCEntry(AtomPath, &atomTOCEntry, &fTOCEntry) )
		return false;
	
	//
	// Validate the arguments.
	if( (atomTOCEntry->AtomDataPos <= fTOCEntry.AtomDataPos) || ((atomTOCEntry->AtomDataPos + Length) > (fTOCEntry.AtomDataPos + fTOCEntry.AtomDataLength)) )
		return false;

	//
	// Read and return this data.
	return fFile->Read(atomTOCEntry->AtomDataPos, Buffer, Length);
}

Bool16 QTAtom::ReadSubAtomInt8(const char * AtomPath, UInt8 * Datum)
{
	//
	// Read and return.
	return ReadSubAtomBytes(AtomPath, (char *)Datum, 1);
}

Bool16 QTAtom::ReadSubAtomInt16(const char * AtomPath, UInt16 * Datum)
{
	// General vars
	UInt16		tempDatum;


	//
	// Read and flip.
	if( !ReadSubAtomBytes(AtomPath, (char *)&tempDatum, 2) )
		return false;
	
	*Datum = ntohs(tempDatum);
	return true;
}

Bool16 QTAtom::ReadSubAtomInt32(const char * AtomPath, UInt32 * Datum)
{
	// General vars
	UInt32		tempDatum;


	//
	// Read and flip.
	if( !ReadSubAtomBytes(AtomPath, (char *)&tempDatum, 4) )
		return false;
	
	*Datum = ntohl(tempDatum);
	return true;
}

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
// $Id: QTAtom_hinf.cpp,v 1.1 2001/02/27 00:56:49 cahighlander Exp $
//
// QTAtom_hinf:
//   The 'hinf' QTAtom class.


// -------------------------------------
// Includes
//
#include <stdio.h>
#include <time.h>

#include "QTFile.h"

#include "QTAtom.h"
#include "QTAtom_hinf.h"



// -------------------------------------
// Macros
//
#define DEBUG_PRINT(s) if(fDebug) printf s
#define DEEP_DEBUG_PRINT(s) if(fDeepDebug) printf s



// -------------------------------------
// Constants
//
const char *	hinfAtom_TotalRTPBytes		= ":totl";
const char *	hinfAtom_TotalRTPPackets	= ":npck";



// -------------------------------------
// Constructors and destructors
//
QTAtom_hinf::QTAtom_hinf(QTFile * File, QTFile::AtomTOCEntry * TOCEntry, Bool16 Debug, Bool16 DeepDebug)
	: QTAtom(File, TOCEntry, Debug, DeepDebug),
	  fTotalRTPBytes(0), fTotalRTPPackets(0)
{
}

QTAtom_hinf::~QTAtom_hinf(void)
{
}



// -------------------------------------
// Initialization functions
//
Bool16 QTAtom_hinf::Initialize(void)
{
	//
	// Parse this atom's sub-atoms.
	ReadSubAtomInt32(hinfAtom_TotalRTPBytes, &fTotalRTPBytes);
	ReadSubAtomInt32(hinfAtom_TotalRTPPackets, &fTotalRTPPackets);

	//
	// This atom has been successfully read in.
	return true;
}



// -------------------------------------
// Debugging functions
//
void QTAtom_hinf::DumpAtom(void)
{
	DEBUG_PRINT(("QTAtom_hinf::DumpAtom - Dumping atom.\n"));
	DEBUG_PRINT(("QTAtom_hinf::DumpAtom - ..Total RTP bytes: %ld\n", fTotalRTPBytes));
	DEBUG_PRINT(("QTAtom_hinf::DumpAtom - ....Average bitrate: %.2f Kbps\n", ((fTotalRTPBytes << 3) / fFile->GetDurationInSeconds()) / 1024));
	DEBUG_PRINT(("QTAtom_hinf::DumpAtom - ..Total RTP packets: %ld\n", fTotalRTPPackets));
	DEBUG_PRINT(("QTAtom_hinf::DumpAtom - ....Average packet size: %ld\n", fTotalRTPBytes / fTotalRTPPackets));
}

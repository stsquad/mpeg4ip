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
// $Id: playlist_QTRTPBroadcastFile.cpp,v 1.2 2001/05/09 21:04:36 cahighlander Exp $
//
// QTRTPBroadcastFile:
//   An interface to QTFile for TimeShare.


// -------------------------------------
// Includes
//
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//#include "OSMutex.h"

//#include "QTFile.h"

//#include "QTTrack.h"
//#include "QTHintTrack.h"

#include "playlist_QTRTPBroadcastFile.h"



bool QTRTPBroadcastFile::FindTrackSSRC( UInt32 SSRC)
{
	// General vars
	RTPTrackListEntry	*ListEntry;


	//
	// Find the specified track.
	for( ListEntry = fFirstTrack; ListEntry != NULL; ListEntry = ListEntry->NextTrack ) 
	{
		// Check for matches.
		if( ListEntry->SSRC == SSRC ) 
		{	return true;
		}
	}

	// The search failed.
	return false;
}



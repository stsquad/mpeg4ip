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

#include "playlist_lists.h"
#include "OS.h"
#include "playlist_utils.h"

// ************************
//
// MEDIA STREAM LIST
//
// ************************

void MediaStreamList::SetUpStreamSSRCs()
{
	UInt32          ssrc;
	MediaStream*    setMediaStreamPtr;
	MediaStream*    aMediaStreamPtr;
	bool            found_duplicate;

	for (int i = 0; i < Size(); i++)
	{	
		setMediaStreamPtr = SetPos(i);
		if (setMediaStreamPtr != NULL) do 
		{  
			ssrc = PlayListUtils::Random() + OS::Milliseconds(); // get a new ssrc
			aMediaStreamPtr = Begin();		// start at the beginning of the stream list
			found_duplicate = false;		// default is don't loop
  
			while (aMediaStreamPtr != NULL) //check all the streams for a duplicate
			{
				if (aMediaStreamPtr->fData.fInitSSRC == ssrc) // it is a duplicate
				{	found_duplicate = true; // set to loop: try a new ssrc 
					break;
				}

				aMediaStreamPtr = Next(); // keep checking for a duplicate
			}
               
			if (!found_duplicate) // no duplicates found so keep this ssrc
				setMediaStreamPtr->fData.fInitSSRC = ssrc;
            	
		} while (found_duplicate); // we have a duplicate ssrc so find another one
	}
}


void MediaStreamList::StreamStarted(SInt64 startTime)
{
	for ( MediaStream *theStreamPtr = Begin(); (theStreamPtr != NULL) ; theStreamPtr = Next() ) 
	{				
		theStreamPtr->StreamStart(startTime);
	} 	
}

void MediaStreamList::MovieStarted(SInt64 startTime)
{
	for ( MediaStream *theStreamPtr = Begin(); (theStreamPtr != NULL) ; theStreamPtr = Next() ) 
	{				
		theStreamPtr->MovieStart(startTime);
	} 
}

void MediaStreamList::MovieEnded(SInt64 endTime)
{
	for ( MediaStream *theStreamPtr = Begin(); (theStreamPtr != NULL) ; theStreamPtr = Next() ) 
	{				
		theStreamPtr->MovieEnd(endTime);
	} 
}


SInt16 MediaStreamList::UpdateStreams()
{
	SInt16 err = 0;
	for ( MediaStream *theStreamPtr = Begin(); (theStreamPtr != NULL) ; theStreamPtr = Next() ) 
	{				
		theStreamPtr->ReceiveOnPorts();
	};
	
	return err;
}

void MediaStreamList::UpdateSenderReportsOnStreams()
{
	SInt64 theTime = PlayListUtils::Milliseconds();
	for ( MediaStream *theStreamPtr = Begin(); (theStreamPtr != NULL) ; theStreamPtr = Next() ) 
	{				
		(void) theStreamPtr->UpdateSenderReport(theTime);
	};
	
}


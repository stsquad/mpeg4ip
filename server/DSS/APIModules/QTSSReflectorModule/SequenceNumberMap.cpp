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
/*
	File:		SequenceNumberMap.cpp

	Contains:	Implements object defined in SequenceNumberMap.h.
					
	
	

*/

#include <string.h>
#include "MyAssert.h"
#include "OSMemory.h"

#include "SequenceNumberMap.h"

SequenceNumberMap::SequenceNumberMap(UInt32 inSlidingWindowSize)
:	fSlidingWindow(NULL),
	fWindowSize(inSlidingWindowSize),
	fNegativeWindowSize(inSlidingWindowSize - (2 * inSlidingWindowSize)),
 	fHighestSeqIndex(0),
	fHighestSeqNumber(0)
{
	Assert(fNegativeWindowSize < 0);
	Assert(fWindowSize < 32768);//AddSequenceNumber makes this assumption
	
}

Bool16 SequenceNumberMap::AddSequenceNumber(UInt16 inSeqNumber)
{
	// Returns whether sequence number has already been added.
	
	//Check to see if object has been initialized
	if (fSlidingWindow == NULL)
	{
		fSlidingWindow = NEW Bool16[fWindowSize + 1];
		::memset(fSlidingWindow, 0, fWindowSize * sizeof(Bool16));
		fHighestSeqIndex = 0;
		fHighestSeqNumber = inSeqNumber;
	}

	// First check to see if this sequence number is so far below the highest sequence number
	// we can't even put it in the sliding window.

	SInt16 theWindowOffset = inSeqNumber - fHighestSeqNumber;
	
	if (theWindowOffset < fNegativeWindowSize)
		return false;//We don't know, but for safety, assume we haven't seen it.
		
	// If this seq # is higher thn the highest previous, set the highest to be this
	// new sequence number, and zero out our sliding window as we go.
	
	while (theWindowOffset > 0)
	{
		fHighestSeqNumber++;

		fHighestSeqIndex++;
		if (fHighestSeqIndex == fWindowSize)
			fHighestSeqIndex = 0;
		fSlidingWindow[fHighestSeqIndex] = false;
			
		theWindowOffset--;
	}

	// Find the right entry in the sliding window for this sequence number, taking
	// into account that we may need to wrap.
	
	SInt16 theWindowIndex = fHighestSeqIndex + theWindowOffset;
	if (theWindowIndex < 0)
		theWindowIndex += fWindowSize;
		
	Assert(theWindowIndex >= 0);
	Assert(theWindowIndex < fWindowSize);
		
	// Turn this index on, return whether it was already turned on.
	Bool16 alreadyAdded = fSlidingWindow[theWindowIndex];
	fSlidingWindow[theWindowIndex] = true;
#if SEQUENCENUMBERMAPTESTING
	//if (alreadyAdded)
	//	printf("Found a duplicate seq num. Num = %d\n", inSeqNumber);
#endif
	return alreadyAdded;
}

#if SEQUENCENUMBERMAPTESTING
void SequenceNumberMap::Test()
{
	SequenceNumberMap theMap1;
	Bool16 retval = theMap1.AddSequenceNumber(64674);
	Assert(retval == false);
	
	retval = theMap1.AddSequenceNumber(64582);
	Assert(retval == false);
	
	retval = theMap1.AddSequenceNumber(64777);
	Assert(retval == false);
	
	retval = theMap1.AddSequenceNumber(64582);
	Assert(retval == true);
	
	retval = theMap1.AddSequenceNumber(64674);
	Assert(retval == true);
	
	retval = theMap1.AddSequenceNumber(1);
	Assert(retval == false);

	retval = theMap1.AddSequenceNumber(65500);
	Assert(retval == false);

	retval = theMap1.AddSequenceNumber(65500);
	Assert(retval == true);

	retval = theMap1.AddSequenceNumber(32768);
	Assert(retval == false);

	retval = theMap1.AddSequenceNumber(1024);
	Assert(retval == false);

	retval = theMap1.AddSequenceNumber(32757);
	Assert(retval == false);

	retval = theMap1.AddSequenceNumber(32799);
	Assert(retval == false);

	retval = theMap1.AddSequenceNumber(32768);
	Assert(retval == false);

}
#endif

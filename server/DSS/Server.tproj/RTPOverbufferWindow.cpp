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
/*
	File:		RTPOverbufferWindow.cpp

	Contains:	Implementation of the class
	
	Written By:	Denis Serenyi

*/

#include "RTPOverbufferWindow.h"
#include "OSMemory.h"
#include "MyAssert.h"

RTPOverbufferWindow::RTPOverbufferWindow(UInt32 inBucketInterval, UInt32 inInitialWindowSize, UInt32 inMaxSendAheadTimeInSec)
:	fWindowSize(inInitialWindowSize),
	fNumBytesInWindow(0),
	fBucketArray(NULL),
	fNumBuckets(inMaxSendAheadTimeInSec * 1000 ),
	fBucketInterval(inBucketInterval),
	fStartIndex(0),
	fStartTransmitTime(0),
	fWriteBurstBeginning(true),
	fOverbufferingEnabled(true),
	fStartBucketInThisWriteBurst(0)
{
	Assert(fBucketInterval > 0); // must be greater than 0;
	//
	// The number of buckets is a function of how much time each bucket represents,
	// times the amount of allowed send ahead time we have.
	fNumBuckets = ((inMaxSendAheadTimeInSec * 1000) / inBucketInterval) + 1;
	fBucketArray = NEW UInt32[fNumBuckets];
	::memset(fBucketArray, 0, sizeof(UInt32) * fNumBuckets);
}

SInt64 RTPOverbufferWindow::AddPacketToWindow(const SInt64& inTransmitTime, const SInt64& inCurrentTime, UInt32 inPacketSize)
{
	if (fNumBytesInWindow == 0)
	{
		fStartTransmitTime = inCurrentTime;
		fStartIndex = 0;
	}
	
	if (inTransmitTime <= fStartTransmitTime)
	{
		//
		// If this happens, this packet is definitely behind in time, so
		// we definitely don't need to put it in the window. It's a <= so
		// packets sent right on time don't get put in the window.
		return -1;
	}
	
	//
	// Check to see if this packet can fit in the window. If it can't, return the
	// time at which it will be able to fit.
	
	// If the packet is bigger than the window itself, no amount of finagling will
	// get it into the window, so this packet must be sent on time.
	if ((inPacketSize > fWindowSize) || (!fOverbufferingEnabled))
		return inTransmitTime + fBucketInterval;
		
	UInt32 bytesAvailable = fWindowSize - fNumBytesInWindow;
	SInt64 theTime = fStartTransmitTime;
	UInt32 theIndex = fStartIndex;
	
	while (bytesAvailable < inPacketSize)
	{
		bytesAvailable += fBucketArray[theIndex];
			
		theIndex++;
		if (theIndex == fNumBuckets)
			theIndex = 0;

		Assert(theIndex != fStartIndex); // we should never do a full loop			
		theTime += fBucketInterval;
	}
	
	//
	// If there isn't enough space right now, return a time in the future where
	// there will be space. Always exaggerate the times by 1 bucket.
	// This is simply so that we do more work at once.
	if (theTime > inCurrentTime)
		return theTime + fBucketInterval;
	
	//
	// The implementation of this window doesn't track individual packets that you
	// add. That would be too memory intensive. Rather, it throws packets into
	// "buckets" that are separated by a fixed time interval.

	//
	// Figure out which bucket this packet goes into. 
	UInt32 theBucket = ((inTransmitTime - fStartTransmitTime) / fBucketInterval) + fStartIndex;
	UInt32 theAdjustedBucket = theBucket;
	if (theAdjustedBucket >= fNumBuckets)
	{
		theAdjustedBucket -= fNumBuckets;
		
		//
		// Check to see if we are so far ahead we can't put this packet in a bucket.
		if (theAdjustedBucket >= fStartIndex)
			return inCurrentTime + fBucketInterval;
	}
	
	if (fWriteBurstBeginning)
	{
		fWriteBurstBeginning = false;
		fStartBucketInThisWriteBurst = theBucket;
	}
	
	//
	// Or that our play rate has exceeded the max allowed play rate.
	if ((theBucket - fStartBucketInThisWriteBurst) > kMaxPlayRateAllowed)
	{
		Assert(!fWriteBurstBeginning);
		return inCurrentTime + fBucketInterval;
	}

#if DEBUG
	if ((((inTransmitTime - fStartTransmitTime) / fBucketInterval) + fStartIndex) < fNumBuckets)
	{
		Assert((((inTransmitTime - fStartTransmitTime) / fBucketInterval) + fStartIndex) == theAdjustedBucket);
	}
	else
	{
		Assert(((((inTransmitTime - fStartTransmitTime) / fBucketInterval) + fStartIndex) - fNumBuckets) == theAdjustedBucket);		
	}
	Assert(theAdjustedBucket < fNumBuckets);
#endif
	
	fBucketArray[theAdjustedBucket] += inPacketSize;
	fNumBytesInWindow += inPacketSize;
	
	return -1;
}

void RTPOverbufferWindow::EmptyOutWindow(const SInt64& inCurrentTime)
{
#if DEBUG
	UInt32 debugCount = 0;
#endif

	//
	// As time passes, buckets get zeroed out, emptying out the window
		
	while (inCurrentTime > (fStartTransmitTime + fBucketInterval))
	{
		Assert(fStartIndex < fNumBuckets);
		Assert(fBucketArray[fStartIndex] <= fNumBytesInWindow);
		if (fBucketArray[fStartIndex] > fNumBytesInWindow) // For some reason there are more packets in the buckets than in the window. (bug 2654571)
			fNumBytesInWindow = 0;
		else
			fNumBytesInWindow -= fBucketArray[fStartIndex]; // this will wrap below 0 and later assert unless we test before subtracting.
		fBucketArray[fStartIndex] = 0;
		Assert(fNumBytesInWindow < 1000000000); // Make sure we didn't go below 0
		
		if (fNumBytesInWindow == 0)
			break;
			
		fStartTransmitTime += fBucketInterval;
		fStartIndex++;
		
		if (fStartIndex == fNumBuckets)
			fStartIndex = 0;
#if DEBUG			
		debugCount++;
		Assert(debugCount < fNumBuckets);
#endif
	}
}

		

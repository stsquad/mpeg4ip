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
	File:		RTPOverbufferWindow.h

	Contains:	Class that tracks packets that are part of the "overbuffer". That is,
				packets that are being sent ahead of time. This class can be used
				to make sure the server isn't overflowing the client's overbuffer size.
	
	Written By:	Denis Serenyi

*/

#ifndef __RTP_OVERBUFFER_WINDOW_H__
#define __RTP_OVERBUFFER_WINDOW_H__

#include "OSHeaders.h"

class RTPOverbufferWindow
{
	public:

		RTPOverbufferWindow(UInt32 inBucketInterval, UInt32 inInitialWindowSize, UInt32 inMaxSendAheadTimeInSec);
		~RTPOverbufferWindow() { delete [] fBucketArray; }
		
		//
		// ACCESSORS
		
		UInt32	NumBytesInWindow() { return fNumBytesInWindow; }
		UInt32	GetBucketInterval() { return fBucketInterval; }
		
		// This may be negative!
		SInt32	AvailableSpaceInWindow() { return fWindowSize - fNumBytesInWindow; }
		
		
		//
		// The window size may be changed at any time
		void	SetWindowSize(UInt32 inWindowSizeInBytes) { fWindowSize = inWindowSizeInBytes; }

		//
		// Without changing the window size, you can enable / disable all overbuffering
		// using these calls. Defaults to enabled
		void	TurnOffOverbuffering()	{ fOverbufferingEnabled = false; }
		void	TurnOnOverbuffering()	{ fOverbufferingEnabled = true; }
		
		//
		// Puts a packet in its proper place in the window. Make sure that
		// packets added here have transmit times that are in the future.
		// Only packets that are being sent ahead of time are part of the overbuffer window.
		//
		// If the overbuffer window is full, this returns a time in the future when
		// enough space will open up for this packet. Otherwise, returns a time in the
		// past or -1.
		//
		// The overbuffer window is full if the byte count is filled up, or if the
		// bitrate is above the max play rate.
		SInt64 AddPacketToWindow(const SInt64& inTransmitTime, const SInt64& inCurrentTime, UInt32 inPacketSize);
		
		//
		// As time passes, transmit times that were in the future become transmit
		// times that are in the past or present. Call this function to empty
		// those old packets out of the window, freeing up space in the window.
		void EmptyOutWindow(const SInt64& inCurrentTime);
		
		//
		// MarkBeginningOfWriteBurst
		// Call this on the first write of a write burst for a client. This
		// allows the overbuffer window to track whether the bitrate of the movie
		// is above the play rate.
		void MarkBeginningOfWriteBurst() { fWriteBurstBeginning = true; }
		
		enum
		{
			//
			// In addition to capping the overbuffer window at a certain number
			// of bytes, it is possible to cap the bitrate of the movie at a certain
			// multiple of the normal play rate, in order to prevent bursts of bandwidth.
			kMaxPlayRateAllowed = 2
		};
		

	private:
		
		UInt32 fWindowSize;
		UInt32 fNumBytesInWindow;
		
		UInt32* fBucketArray;
		UInt32 fNumBuckets;
		UInt32 fBucketInterval;
		UInt32 fStartIndex;
		SInt64 fStartTransmitTime;
		Bool16 fWriteBurstBeginning;
		Bool16 fOverbufferingEnabled;
		UInt32 fStartBucketInThisWriteBurst;
		
};


#endif // __RTP_OVERBUFFER_TRACKER_H__



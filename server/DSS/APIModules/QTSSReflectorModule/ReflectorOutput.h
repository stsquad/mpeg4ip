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
	File:		ReflectorOutput.h

	Contains:	VERY simple abstract base class that defines one virtual method, WritePacket.
				This is extremely useful to the reflector, which, using one of these objects,
				can transparently reflect a packet, not being aware of how it will actually be
				written to the network
					


*/

#ifndef __REFLECTOR_OUTPUT_H__
#define __REFLECTOR_OUTPUT_H__

#include "QTSS.h"
#include "StrPtrLen.h"
#include "OSHeaders.h"
#include "MyAssert.h"
#include "OS.h"
#include "OSQueue.h"

class ReflectorOutput
{
	public:
	
		ReflectorOutput() 
		{
			fBookmarkedPacketsElemsArray = NULL;
			fNumBookmarks = 0;
		}	

		virtual ~ReflectorOutput() 
		{
			if ( fBookmarkedPacketsElemsArray )
			{	::memset( fBookmarkedPacketsElemsArray, 0, sizeof ( OSQueueElem* ) * fNumBookmarks );
				delete [] fBookmarkedPacketsElemsArray;
			}
		}
		
		// an array of packet elements ( from fPacketQueue in ReflectorSender )
		// possibly one for each ReflectorSender that sends data to this ReflectorOutput		
		OSQueueElem			**fBookmarkedPacketsElemsArray;
		UInt32				fNumBookmarks;
		// WritePacket
		//
		// Pass in the packet contents, the cookie of the stream to which it will be written,
		// and the QTSS API write flags (this should either be qtssWriteFlagsIsRTP or IsRTCP
		// packetLateness is how many MSec's late this packet is in being delivered ( will be < 0 if its early )
		virtual QTSS_Error	WritePacket(StrPtrLen* inPacket, void* inStreamCookie, UInt32 inFlags, SInt64 packetLatenessInMSec ) = 0;
	
	protected:
		void	InititializeBookmarks( UInt32 numStreams ) 
		{ 	
			// need 2 bookmarks for each stream ( include RTCPs )
			UInt32	numBookmarks = numStreams * 2;

			fBookmarkedPacketsElemsArray = new OSQueueElem*[numBookmarks]; 
			::memset( fBookmarkedPacketsElemsArray, 0, sizeof ( OSQueueElem* ) * numBookmarks );
			
			fNumBookmarks = numBookmarks;
		}

};

#endif //__REFLECTOR_OUTPUT_H__

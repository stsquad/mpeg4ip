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
	File:		SDPSourceInfo.h

	Contains:	This object takes input SDP data, and uses it to support the SourceInfo
				API.



*/

#ifndef __SDP_SOURCE_INFO_H__
#define __SDP_SOURCE_INFO_H__

#include "StrPtrLen.h"
#include "SourceInfo.h"
#include "StringParser.h"

class SDPSourceInfo : public SourceInfo
{
	public:
	
		// Uses the SDP Data to build up the StreamInfo structures
		SDPSourceInfo(char* sdpData, UInt32 sdpLen) { Parse(sdpData, sdpLen); }
		SDPSourceInfo() {}
		virtual ~SDPSourceInfo();
		
		// Parses out the SDP file provided, sets up the StreamInfo structures
		void	Parse(char* sdpData, UInt32 sdpLen);

		// This function uses the Parsed SDP file, and strips out all the network information,
		// producing an SDP file that appears to be local.
		virtual char*	GetLocalSDP(UInt32* newSDPLen);

		// Returns the SDP data
		StrPtrLen*	GetSDPData()	{ return &fSDPData; }
		
		// Utility routines
		
		// Assuming the parser is currently pointing at the beginning of an dotted-
		// decimal IP address, this consumes it (stopping at inStopChar), and returns
		// the IP address (host ordered) as a UInt32
		static UInt32 GetIPAddr(StringParser* inParser, char inStopChar);
		
	private:

		enum
		{
			kDefaultTTL = 15	//UInt16
		};
		StrPtrLen	fSDPData;
};
#endif // __SDP_SOURCE_INFO_H__


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
	File:		RCFSourceInfo.cpp

	Contains:	Implementation of object defined in .h file

	Copyright:	© 1998 by Apple Computer, Inc., all rights reserved.

	

*/

#include "RCFSourceInfo.h"
#include "PrefsSource.h"
#include "StringParser.h"
#include "SDPSourceInfo.h"
#include "OSMemory.h"

// StrPtrLen's for various keywords on the relay_source & relay_destination lines
static StrPtrLen sInAddr("in_addr");
static StrPtrLen sOutAddr("out_addr");
static StrPtrLen sSrcAddr("src_addr");
static StrPtrLen sDestAddr("dest_addr");
static StrPtrLen sInPorts("in_ports");
static StrPtrLen sDestPorts("dest_ports");
static StrPtrLen sTtl("ttl");

RCFSourceInfo::~RCFSourceInfo()
{
	if (fOutputArray != NULL)
	{
		for (UInt32 x = 0; x < fNumOutputs; x++)
			delete [] fOutputArray[x].fPortArray;
			
		char* theOutputArray = (char*)fOutputArray;
		delete [] theOutputArray;
	}
	if (fStreamArray != NULL)
	{
		char* theStreamArray = (char*)fStreamArray;
		delete [] theStreamArray;
	}
}

void	RCFSourceInfo::Parse(RelayPrefsSource* inPrefsSource, SInt32 inStartIndex)
{
#if DEBUG
	// This should be checked by the calling function
	static StrPtrLen sKeyStr("relay_source");
	StrPtrLen theKeyStr(inPrefsSource->GetKeyAtIndex(inStartIndex));
	Assert(theKeyStr.EqualIgnoreCase(sKeyStr.Ptr, sKeyStr.Len));
#endif

	// First parse out the source information
	StrPtrLen theBodyStr(inPrefsSource->GetValueAtIndex(inStartIndex));
	StringParser theSourceParser(&theBodyStr);
	
	fNumStreams = 0;
	UInt32 destIPAddr = 0;
	UInt32 srcIPAddr = 0;
	UInt16 ttl = 0;
	StrPtrLen thePortsMarker;
	
	// FORMAT OF RELAY SOURCE LINE:
	// relay_source: in_addr=224.48.91.219 src_addr=17.221.93.215 in_ports=2250 2252 2254 ttl=15
	// relay_destination: dest_addr=17.221.89.45 out_addr=17.221.94.80 dest_ports=5400 5402 5404 ttl=15
	
		
	while (true)
	{
		// First count up the number of streams we have. While doing this,
		// store the source IP address, dest IP address, and ttl
		
		StrPtrLen theKeyWord;
		theSourceParser.ConsumeWord(&theKeyWord);
		if (theKeyWord.Len == 0)
			break;
		
		if (theKeyWord.Equal(sInAddr))
		{
			theSourceParser.ConsumeUntil(NULL, StringParser::sDigitMask);
			destIPAddr = SDPSourceInfo::GetIPAddr(&theSourceParser, ' ');
			theSourceParser.ConsumeUntil(NULL, StringParser::sWordMask);
		}
		else if (theKeyWord.Equal(sSrcAddr))
		{
			theSourceParser.ConsumeUntil(NULL, StringParser::sDigitMask);
			srcIPAddr = SDPSourceInfo::GetIPAddr(&theSourceParser, ' ');
			theSourceParser.ConsumeUntil(NULL, StringParser::sWordMask);
		}
		else if (theKeyWord.Equal(sInPorts))
		{
			theSourceParser.ConsumeUntil(NULL, StringParser::sDigitMask);
			
			// Mark where the ports are for future reference
			thePortsMarker.Ptr = theSourceParser.GetCurrentPosition();
			thePortsMarker.Len = theSourceParser.GetDataRemaining();
			
			while (true)
			{
				StrPtrLen thePort;
				(void)theSourceParser.ConsumeInteger(&thePort);
				if (thePort.Len == 0)
					break;
				theSourceParser.ConsumeWhitespace();
				fNumStreams++;
			}
			theSourceParser.ConsumeUntil(NULL, StringParser::sWordMask);
		}
		else if (theKeyWord.Equal(sTtl))
		{
			theSourceParser.ConsumeUntil(NULL, StringParser::sDigitMask);
			ttl = theSourceParser.ConsumeInteger(NULL);
			theSourceParser.ConsumeUntil(NULL, StringParser::sWordMask);			
		}
	}
	
	// Allocate a proper sized stream array
	fStreamArray = (StreamInfo*)NEW char[(fNumStreams + 1) * sizeof(StreamInfo)];
	::memset(fStreamArray, 0, (fNumStreams + 1) * sizeof(StreamInfo));
	
	StringParser thePortParser(&thePortsMarker);
	for (UInt32 x = 0; x < fNumStreams; x++)
	{
		// Setup all the StreamInfo structures
		fStreamArray[x].fSrcIPAddr = srcIPAddr;
		fStreamArray[x].fDestIPAddr = destIPAddr;
		fStreamArray[x].fPort = thePortParser.ConsumeInteger(NULL);
		fStreamArray[x].fTimeToLive = ttl;
		fStreamArray[x].fPayloadType = qtssUnknownPayloadType;
		fStreamArray[x].fTrackID = x+1;
		
		thePortParser.ConsumeUntil(NULL, StringParser::sDigitMask);
	}
	
	// Now go through all the relay_destination lines (starting from the next line after the
	// relay_source line.
	this->ParseRelayDestinations(inPrefsSource, --inStartIndex);
}

void RCFSourceInfo::ParseRelayDestinations(RelayPrefsSource* inPrefsSource, SInt32 inStartIndex)
{
	// Figure out how many relay outputs there are. We are going through the file
	// in top to bottom order, looking for all the consecutive "relay_destination" lines
	// after the "relay_source" line, but because RelayPrefsSource indexes the lines
	// in reverse order, we actually are decrementing inStartIndex.
	static StrPtrLen sDestKeyString("relay_destination");
	SInt32 theDestStart = inStartIndex;
	fNumOutputs = 0;
	while ((inStartIndex >= 0) && (sDestKeyString.EqualIgnoreCase(inPrefsSource->GetKeyAtIndex(inStartIndex),
					::strlen(inPrefsSource->GetKeyAtIndex(inStartIndex)))))
	{
		inStartIndex--;
		fNumOutputs++;
	}
		
	// Allocate the proper number of relay outputs
	fOutputArray = (OutputInfo*)NEW char[(fNumOutputs + 1) * sizeof(OutputInfo)];
	::memset(fOutputArray, 0, (fNumOutputs + 1) * sizeof(OutputInfo));
	
	for (UInt32 a = 0; a < fNumOutputs; a++)
	{
		// Each OutputInfo has a dynamically allocated array of output ports.
		// Allocate those arrays here.
		fOutputArray[a].fPortArray = NEW UInt16[fNumStreams + 1];
		::memset(fOutputArray[a].fPortArray, 0, fNumStreams * sizeof(UInt16));
	}
	
	// Now actually go through and figure out what to put into these OutputInfo structures,
	// based on what's on the relay_destination line
	for (UInt32 y = 0; y < fNumOutputs; y++)
	{
		StrPtrLen theDestBodyStr(inPrefsSource->GetValueAtIndex(theDestStart--));
		StringParser theDestParser(&theDestBodyStr);
	
		while (true)
		{
			// Go through each destination line, picking out the fields and placing
			// the values into the right OutputInfo structure.
						
			StrPtrLen theKeyWord;
			theDestParser.ConsumeWord(&theKeyWord);
			if (theKeyWord.Len == 0)
				break;
			
			if (theKeyWord.Equal(sOutAddr))
			{
				theDestParser.ConsumeUntil(NULL, StringParser::sDigitMask);
				fOutputArray[y].fLocalAddr = SDPSourceInfo::GetIPAddr(&theDestParser, ' ');
				theDestParser.ConsumeUntil(NULL, StringParser::sWordMask);
			}
			else if (theKeyWord.Equal(sDestAddr))
			{
				theDestParser.ConsumeUntil(NULL, StringParser::sDigitMask);
				fOutputArray[y].fDestAddr = SDPSourceInfo::GetIPAddr(&theDestParser, ' ');
				theDestParser.ConsumeUntil(NULL, StringParser::sWordMask);
			}
			else if (theKeyWord.Equal(sDestPorts))
			{
				theDestParser.ConsumeUntil(NULL, StringParser::sDigitMask);
				
				for (UInt32 z = 0; z < fNumStreams; z++)
				{
					fOutputArray[y].fPortArray[z] = theDestParser.ConsumeInteger(NULL);
					theDestParser.ConsumeWhitespace();
				}
				theDestParser.ConsumeUntil(NULL, StringParser::sWordMask);
			}
			else if (theKeyWord.Equal(sTtl))
			{
				theDestParser.ConsumeUntil(NULL, StringParser::sDigitMask);
				fOutputArray[y].fTimeToLive = theDestParser.ConsumeInteger(NULL);
				theDestParser.ConsumeUntil(NULL, StringParser::sWordMask);			
			}
		}
	}
}

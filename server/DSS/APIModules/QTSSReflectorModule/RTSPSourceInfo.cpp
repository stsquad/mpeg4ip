/*
 * Copyright (c) 1999 Apple Computer, Inc. All rights reserved.
 *
 * @APPLE_LICENSE_HEADER_START@
 * 
 * Portions Copyright (c) 1999 Apple Computer, Inc.  All Rights
 * Reserved.  This file contains Original Code and/or Modifications of
 * Original Code as defined in and that are subject to the Apple Public
 * Source License Version 1.1 (the "License").  You may not use this file
 * except in compliance with the License.  Please obtain a copy of the
 * License at http://www.apple.com/publicsource and read it before using
 * this file.
 * 
 * The Original Code and all software distributed under the License are
 * distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, AND APPLE HEREBY DISCLAIMS ALL SUCH WARRANTIES,
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE OR NON- INFRINGEMENT.  Please see the
 * License for the specific language governing rights and limitations
 * under the License.
 * 
 * @APPLE_LICENSE_HEADER_END@
 */
/*
	File:		RTSPSourceInfo.cpp

	Contains:	

	Copyright:	© 1998 by Apple Computer, Inc., all rights reserved.

	

*/

#include "RTSPSourceInfo.h"
#include "SDPSourceInfo.h"
#include "OSMemory.h"

StrPtrLen RTSPSourceInfo::sKeyString("rtsp_source");

QTSS_Error RTSPSourceInfo::ParsePrefs(RelayPrefsSource* inPrefsSource, UInt32 inStartIndex)
{
	// First parse the file, fetching the IP address, port, and URL on the server
	StrPtrLen theKeyStr(inPrefsSource->GetKeyAtIndex(inStartIndex));
	StrPtrLen theBodyStr(inPrefsSource->GetValueAtIndex(inStartIndex));
	
	// If this isn't an RTSP source line, we don't care!
	if (!theKeyStr.Equal(sKeyString))
		return QTSS_RequestFailed;
		
	// Parse the rtsp_source line. Format: rtsp_source: 17.221.98.45 554 rtsp://17.221.98.45/moof.mov
	StringParser theSrcParser(&theBodyStr);
	UInt32 theHostAddr = SDPSourceInfo::GetIPAddr(&theSrcParser, ' ');
	theSrcParser.ConsumeWhitespace();
	UInt32 theHostPort = theSrcParser.ConsumeInteger(NULL);
	theSrcParser.ConsumeWhitespace();
	
	// The remaining bit is the URL
	StrPtrLen theURL;
	theURL.Ptr = theSrcParser.GetCurrentPosition();
	theURL.Len = theSrcParser.GetDataRemaining();

	fClientSocket.Set(theHostAddr, (UInt16)theHostPort);
	fClient.Set(theURL);
	return QTSS_NoErr;
}

QTSS_Error	RTSPSourceInfo::Describe()
{
	QTSS_Error theErr = QTSS_NoErr;
	
	if (!fDescribeComplete)
	{
		// Work on the describe
		theErr = fClient.SendDescribe();
		if (theErr != QTSS_NoErr)
			return theErr;
		if (fClient.GetStatus() != 200)
			return QTSS_RequestFailed;
			
		// If the above function returns QTSS_NoErr, we've gotten the describe response,
		// so process it.
		SDPSourceInfo theSourceInfo(fClient.GetContentBody(), fClient.GetContentLength());
		
		// Copy the Source Info into our local SourceInfo.
		fNumStreams = theSourceInfo.GetNumStreams();
		fStreamArray = (StreamInfo*)NEW char[(fNumStreams + 1) * sizeof(StreamInfo)];
		::memset(fStreamArray, 0, (fNumStreams + 1) * sizeof(StreamInfo));
		
		for (UInt32 x = 0; x < fNumStreams; x++)
		{
			// Copy all stream info data. Also set fSrcIPAddr to be the host addr
			fStreamArray[x].fSrcIPAddr = fClientSocket.GetHostAddr();
			fStreamArray[x].fDestIPAddr = fClientSocket.GetLocalAddr();
			fStreamArray[x].fPort = 0;
			fStreamArray[x].fTimeToLive = 0;
			fStreamArray[x].fPayloadType = theSourceInfo.GetStreamInfo(x)->fPayloadType;
			fStreamArray[x].fPayloadName = theSourceInfo.GetStreamInfo(x)->fPayloadName;
			fStreamArray[x].fTrackID = theSourceInfo.GetStreamInfo(x)->fTrackID;
			fStreamArray[x].fBufferDelay = theSourceInfo.GetStreamInfo(x)->fBufferDelay;
		}
	}

	// Ok, describe is complete, copy out the SDP information.
	
	fLocalSDP.Ptr = NEW char[fClient.GetContentLength() + 1];
	
	// Look for an "a=range" line in the SDP. If there is one, remove it.
	
	static StrPtrLen sRangeStr("a=range:");
	StrPtrLen theSDPPtr(fClient.GetContentBody(), fClient.GetContentLength());
	StringParser theSDPParser(&theSDPPtr);
	
	do
	{
		// Loop until we reach the end of the SDP or hit a a=range line.
		StrPtrLen theSDPLine(theSDPParser.GetCurrentPosition(), theSDPParser.GetDataRemaining());
		if ((theSDPLine.Len > sRangeStr.Len) && (theSDPLine.NumEqualIgnoreCase(sRangeStr.Ptr, sRangeStr.Len)))
			break;
			
	} while (theSDPParser.GetThruEOL(NULL));
	
	// Copy what we have so far
	::memcpy(fLocalSDP.Ptr, fClient.GetContentBody(), theSDPParser.GetDataParsedLen());
	fLocalSDP.Len = theSDPParser.GetDataParsedLen();
	
	// Skip over the range (if it exists)
	(void)theSDPParser.GetThruEOL(NULL);
	
	// Copy the rest of the SDP
	::memcpy(fLocalSDP.Ptr + fLocalSDP.Len, theSDPParser.GetCurrentPosition(), theSDPParser.GetDataRemaining());
	fLocalSDP.Len += theSDPParser.GetDataRemaining();

#define _WRITE_SDP_ 0

#if _WRITE_SDP_
	FILE* outputFile = ::fopen("rtspclient.sdp", "w");
	if (outputFile != NULL)
	{
		fLocalSDP.Ptr[fLocalSDP.Len] = '\0';
		::fprintf(outputFile, "%s", fLocalSDP.Ptr);
		::fclose(outputFile);
		printf("Wrote sdp to rtspclient.sdp\n");
	}
	else
		printf("Failed to write sdp\n");
#endif
	fDescribeComplete = true;
	return QTSS_NoErr;
}

QTSS_Error RTSPSourceInfo::SetupAndPlay()
{
	QTSS_Error theErr = QTSS_NoErr;
	
	// Do all the setups. This is async, so when a setup doesn't complete
	// immediately, return an error, and we'll pick up where we left off.
	while (fNumSetupsComplete < fNumStreams)
	{
		theErr = fClient.SendUDPSetup(fStreamArray[fNumSetupsComplete].fTrackID, fStreamArray[fNumSetupsComplete].fPort);
		if (theErr != QTSS_NoErr)
			return theErr;
		else if (fClient.GetStatus() != 200)
			return QTSS_RequestFailed;
		else
			fNumSetupsComplete++;
	}
	
	// We've done all the setups. Now send a play.
	theErr = fClient.SendPlay(0);
	if (theErr != QTSS_NoErr)
		return theErr;
	if (fClient.GetStatus() != 200)
		return QTSS_RequestFailed;
		
	return QTSS_NoErr;
}

QTSS_Error RTSPSourceInfo::Teardown()
{
	return (QTSS_Error)fClient.SendTeardown();
}

char*	RTSPSourceInfo::GetLocalSDP(UInt32* newSDPLen)
{
	*newSDPLen = fLocalSDP.Len;
	return fLocalSDP.Ptr;
}

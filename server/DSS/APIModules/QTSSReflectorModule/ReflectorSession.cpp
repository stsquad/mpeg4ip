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
	File:		ReflectorSession.cpp

	Contains:	Implementation of object defined in ReflectorSession.h. 
					
	

*/


#include "ReflectorSession.h"
#include "RTCPPacket.h"
#include "SocketUtils.h"
#include "EventContext.h"

#include "OSMemory.h"
#include "OS.h"
#include "atomic.h"

#include "QTSSModuleUtils.h"

#include <errno.h>

#if DEBUG
#define REFLECTOR_SESSION_DEBUGGING 0
#else
#define REFLECTOR_SESSION_DEBUGGING 0
#endif

#pragma mark _REFLECTOR_SESSION_

static OSRefTable* 		sStreamMap = NULL;

void ReflectorSession::Initialize()
{
	if (sStreamMap == NULL)
		sStreamMap = NEW OSRefTable();
}

ReflectorSession::ReflectorSession(StrPtrLen* inSourceID, SourceInfo* inInfo)
: 	fIsSetup(false),
	fQueueElem(this),
	fNumOutputs(0),
	fStreamArray(NULL),
	fFormatter(fHTMLBuf, kMaxHTMLSize),
	fSourceInfo(inInfo),
	fSocketStream(NULL)
{
	if (inSourceID != NULL)
	{
		fSourceID.Ptr = NEW char[inSourceID->Len + 1];
		::memcpy(fSourceID.Ptr, inSourceID->Ptr, inSourceID->Len);
		fSourceID.Len = inSourceID->Len;
	fRef.Set(fSourceID, this);
	}
}


ReflectorSession::~ReflectorSession()
{
#if REFLECTOR_SESSION_DEBUGGING
	printf("Removing ReflectorSession: %s\n", fSourceInfoHTML.Ptr);
#endif

	// For each stream, check to see if the ReflectorStream should be deleted
	OSMutexLocker locker (sStreamMap->GetMutex());
	for (UInt32 x = 0; x < fSourceInfo->GetNumStreams(); x++)
	{
		if (fStreamArray[x] == NULL)
			continue;
			
		//decrement the ref count
		if (fStreamArray[x]->GetRef()->GetRefCount() > 0) 	// Refcount may be 0 if there was
															// some error setting up the stream
			sStreamMap->Release(fStreamArray[x]->GetRef());
			
		if (fStreamArray[x]->GetRef()->GetRefCount() == 0)
		{
			// Delete this stream if the refcount has dropped to 0
			sStreamMap->UnRegister(fStreamArray[x]->GetRef());
			delete fStreamArray[x];
		}	
	}
	
	// We own this object when it is given to us, so delete it now
	delete fSourceInfo;
}


QTSS_Error ReflectorSession::SetupReflectorSession(SourceInfo* inInfo, QTSS_RTSPRequestObject inRequest, UInt32 inFlags)
{
	// Store a reference to this sourceInfo permanently
	Assert((fSourceInfo == NULL) || (inInfo == fSourceInfo));
	fSourceInfo = inInfo;
	fLocalSDP.Ptr = inInfo->GetLocalSDP(&fLocalSDP.Len);

	// Allocate all our ReflectorStreams, using the SourceInfo
	fStreamArray = NEW ReflectorStream*[fSourceInfo->GetNumStreams()];
	::memset(fStreamArray, 0, fSourceInfo->GetNumStreams() * sizeof(ReflectorStream*));
	
	for (UInt32 x = 0; x < fSourceInfo->GetNumStreams(); x++)
	{
		// For each ReflectorStream, check and see if there is one that matches
		// this stream ID
		char theStreamID[ReflectorStream::kStreamIDSize];
		StrPtrLen theStreamIDPtr(theStreamID, ReflectorStream::kStreamIDSize);
		ReflectorStream::GenerateSourceID(fSourceInfo->GetStreamInfo(x), &theStreamID[0]);
		
		OSMutexLocker locker(sStreamMap->GetMutex());
		OSRef* theStreamRef = NULL;
		
		// If the port # of this stream is 0, that means "any port".
		// We don't know what the dest port of this stream is yet (this
		// can happen while acting as an RTSP client). Never share these streams.
		// This can also happen if the incoming data is interleaved in the TCP connection
		if (fSourceInfo->GetStreamInfo(x)->fPort > 0)
			theStreamRef = sStreamMap->Resolve(&theStreamIDPtr);

		if (theStreamRef == NULL)
		{
			fStreamArray[x] = NEW ReflectorStream(fSourceInfo->GetStreamInfo(x));

			// Obviously, we may encounter an error binding the reflector sockets.
			// If that happens, we'll just abort here, which will leave the ReflectorStream
			// array in an inconsistent state, so we need to make sure in our cleanup
			// code to check for NULL.
			QTSS_Error theError = fStreamArray[x]->BindSockets(inRequest);
			if (theError != QTSS_NoErr)
				return theError;
				
			// If the port was 0, update it to reflect what the actual RTP port is.
			fSourceInfo->GetStreamInfo(x)->fPort = fStreamArray[x]->GetStreamInfo()->fPort;
			ReflectorStream::GenerateSourceID(fSourceInfo->GetStreamInfo(x), &theStreamID[0]);

			theError = sStreamMap->Register(fStreamArray[x]->GetRef());
			Assert(theError == QTSS_NoErr);

			//unless we do this, the refcount won't increment (and we'll delete the session prematurely
			OSRef* debug = sStreamMap->Resolve(&theStreamIDPtr);
			Assert(debug == fStreamArray[x]->GetRef());
		}
		else
			fStreamArray[x] = (ReflectorStream*)theStreamRef->GetObject();
	}
	
#if REFLECTOR_SESSION_DEBUGGING > 3
	printf("Client spacing: %ld\n", inBucketDelayInMsec);
#endif

	if (inFlags & kMarkSetup)
		fIsSetup = true;

	return QTSS_NoErr;
}

void	ReflectorSession::FormatHTML(StrPtrLen* inURL)
{
	// Begin writing our source description HTML (used by the relay)
	// Line looks like: Relay Source: 17.221.98.239, Ports: 5430 5432 5434
	static StrPtrLen sHTMLStart("<H2>Relay Source: ");
	static StrPtrLen sPorts(", Ports: ");
	static StrPtrLen sHTMLEnd("</H2><BR>");
	
	// Begin writing the HTML
	fFormatter.Put(sHTMLStart);
	
	if (inURL == NULL)
	{	
		// If no URL is provided, format the dest IP addr as a string.
		char theIPAddrBuf[20];
		StrPtrLen theIPAddr(theIPAddrBuf, 20);
		struct in_addr theAddr;
		theAddr.s_addr = htonl(fSourceInfo->GetStreamInfo(0)->fDestIPAddr);
		SocketUtils::ConvertAddrToString(theAddr, &theIPAddr);
		fFormatter.Put(theIPAddr);
	}
	else
		fFormatter.Put(*inURL);
		
	fFormatter.Put(sPorts);

	for (UInt32 x = 0; x < fSourceInfo->GetNumStreams(); x++)
	{
		fFormatter.Put(fSourceInfo->GetStreamInfo(x)->fPort);
		fFormatter.PutSpace();
	}
	fFormatter.Put(sHTMLEnd);

	// Setup the StrPtrLen to point to the right stuff
	fSourceInfoHTML.Ptr = fFormatter.GetBufPtr();
	fSourceInfoHTML.Len = fFormatter.GetCurrentOffset();
	
	fFormatter.PutTerminator();
}


void	ReflectorSession::AddOutput(ReflectorOutput* inOutput)
{
	Assert(fSourceInfo->GetNumStreams() > 0);
	
	// We need to make sure that this output goes into the same bucket for each ReflectorStream.
	SInt32 bucket = -1;
	SInt32 lastBucket = -1;
	
	while (true)
	{
		UInt32 x = 0;
		for ( ; x < fSourceInfo->GetNumStreams(); x++)
		{
			bucket = fStreamArray[x]->AddOutput(inOutput, bucket);
			if (bucket == -1) 	// If this output couldn't be added to this bucket,
				break;			// break and try again
			else
				lastBucket = bucket; // Remember the last successful bucket placement.
		}
		
		if (bucket == -1)
		{
			// If there was some kind of conflict adding this output to this bucket,
			// we need to remove it from the streams to which it was added.
			for (UInt32 y = 0; y < x; y++)
				fStreamArray[y]->RemoveOutput(inOutput);
				
			// Because there was an error, we need to start the whole process over again,
			// this time starting from a higher bucket
			lastBucket = bucket = lastBucket + 1;
		}
		else
			break;
	}
	(void)atomic_add(&fNumOutputs, 1);
}

void 	ReflectorSession::RemoveOutput(ReflectorOutput* inOutput)
{
	(void)atomic_sub(&fNumOutputs, 1);
	for (UInt32 y = 0; y < fSourceInfo->GetNumStreams(); y++)
		fStreamArray[y]->RemoveOutput(inOutput);
}

UInt32	ReflectorSession::GetBitRate()
{
	UInt32 retval = 0;
	for (UInt32 x = 0; x < fSourceInfo->GetNumStreams(); x++)
		retval += fStreamArray[x]->GetBitRate();
	return retval;
}

Bool16 ReflectorSession::Equal(SourceInfo* inInfo)
{
	// Check to make sure the # of streams matches up
	if (fSourceInfo->GetNumStreams() != inInfo->GetNumStreams())
		return false;
	
	// Check the src & dest addr, and port of each stream. 
	for (UInt32 x = 0; x < fSourceInfo->GetNumStreams(); x++)
	{
		if (fStreamArray[x]->GetStreamInfo()->fDestIPAddr != inInfo->GetStreamInfo(x)->fDestIPAddr)
			return false;
		if (fStreamArray[x]->GetStreamInfo()->fSrcIPAddr != inInfo->GetStreamInfo(x)->fSrcIPAddr)
			return false;
		
		// If either one of the comparators is 0 (the "wildcard" port), then we know at this point
		// they are equivalent
		if ((fStreamArray[x]->GetStreamInfo()->fPort == 0) || (inInfo->GetStreamInfo(x)->fPort == 0))
			return true;
			
		// Neither one is the wildcard port, so they must be the same
		if (fStreamArray[x]->GetStreamInfo()->fPort != inInfo->GetStreamInfo(x)->fPort)
			return false;
	}
	return true;
}

void*	ReflectorSession::GetStreamCookie(UInt32 inStreamID)
{
	for (UInt32 x = 0; x < fSourceInfo->GetNumStreams(); x++)
	{
		if (fSourceInfo->GetStreamInfo(x)->fTrackID == inStreamID)
			return fStreamArray[x]->GetStreamCookie();
	}
	return NULL;
}


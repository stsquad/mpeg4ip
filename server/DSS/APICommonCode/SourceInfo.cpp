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
	File:		SourceInfo.cpp

	Contains:	Implements object defined in .h file.
					

*/

#include "SourceInfo.h"
#include "SocketUtils.h"

Bool16	SourceInfo::IsReflectable()
{
	if (fStreamArray == NULL)
		return false;
	if (fNumStreams == 0)
		return false;
		
	//each stream's info must meet certain criteria
	for (UInt32 x = 0; x < fNumStreams; x++)
	{
		if (fStreamArray[x].fIsTCP)
			continue;
			
		if ((!this->IsReflectableIPAddr(fStreamArray[x].fDestIPAddr)) ||
			(fStreamArray[x].fPort == 0) ||
			(fStreamArray[x].fTimeToLive == 0))
			return false;
	}
	return true;
}

Bool16	SourceInfo::IsReflectableIPAddr(UInt32 inIPAddr)
{
	if (SocketUtils::IsMulticastIPAddr(inIPAddr) || SocketUtils::IsLocalIPAddr(inIPAddr))
		return true;
	return false;
}

SourceInfo::StreamInfo*	SourceInfo::GetStreamInfo(UInt32 inIndex)
{
	Assert(inIndex < fNumStreams);
	if (fStreamArray == NULL)
		return NULL;
	if (inIndex < fNumStreams)
		return &fStreamArray[inIndex];
	else
		return NULL;
}

SourceInfo::StreamInfo*	SourceInfo::GetStreamInfoByTrackID(UInt32 inTrackID)
{
	if (fStreamArray == NULL)
		return NULL;
	for (UInt32 x = 0; x < fNumStreams; x++)
	{
		if (fStreamArray[x].fTrackID == inTrackID)
			return &fStreamArray[x];
	}
	return NULL;
}

SourceInfo::OutputInfo*	SourceInfo::GetOutputInfo(UInt32 inIndex)
{
	Assert(inIndex < fNumOutputs);
	if (fOutputArray == NULL)
		return NULL;
	if (inIndex < fNumOutputs)
		return &fOutputArray[inIndex];
	else
		return NULL;
}

UInt32 SourceInfo::GetNumNewOutputs()
{
	UInt32 theNumNewOutputs = 0;
	for (UInt32 x = 0; x < fNumOutputs; x++)
	{
		if (!fOutputArray[x].fAlreadySetup)
			theNumNewOutputs++;
	}
	return theNumNewOutputs;
}

SourceInfo::StreamInfo::StreamInfo(const StreamInfo& copy)
: 	fSrcIPAddr(copy.fSrcIPAddr),
	fDestIPAddr(copy.fDestIPAddr),
	fPort(copy.fPort),
	fTimeToLive(copy.fTimeToLive),
	fPayloadType(copy.fPayloadType),
	fPayloadName(copy.fPayloadName),// Note that we don't copy data here
	fTrackID(copy.fTrackID),
	fBufferDelay(copy.fBufferDelay),
	fIsTCP(copy.fIsTCP)
{}

SourceInfo::OutputInfo::OutputInfo(const OutputInfo& copy)
: 	fDestAddr(copy.fDestAddr),
	fLocalAddr(copy.fLocalAddr),
	fTimeToLive(copy.fTimeToLive),
	fPortArray(copy.fPortArray),// Note that we don't copy data here
	fAlreadySetup(copy.fAlreadySetup)
{}

Bool16 SourceInfo::OutputInfo::Equal(const OutputInfo& info)
{
	if ((fDestAddr == info.fDestAddr) && (fLocalAddr == info.fLocalAddr) &&
		(fTimeToLive == info.fTimeToLive) && (fPortArray[0] == info.fPortArray[0]))
		return true;
	return false;
}

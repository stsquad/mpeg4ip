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
	File:		UDPSocketPool.cpp

	Contains:	Object that creates & maintains UDP socket pairs in a pool.

	
*/

#include "UDPSocketPool.h"

UDPSocketPair* UDPSocketPool::GetUDPSocketPair(UInt32 inIPAddr, UInt16 inPort,
												UInt32 inSrcIPAddr, UInt16 inSrcPort)
{
	OSMutexLocker locker(&fMutex);
	if ((inSrcIPAddr != 0) || (inSrcPort != 0))
	{
		for (OSQueueIter qIter(&fUDPQueue); !qIter.IsDone(); qIter.Next())
		{
			//If we find a pair that is a) on the right IP address, and b) doesn't
			//have this source IP & port in the demuxer already, we can return this pair
			UDPSocketPair* theElem = (UDPSocketPair*)qIter.GetCurrent()->GetEnclosingObject();
			if ((theElem->fSocketA->GetLocalAddr() == inIPAddr) &&
				((inPort == 0) || (theElem->fSocketA->GetLocalPort() == inPort)))
			{
				//check to make sure this source IP & port is not already in the demuxer.
				//If not, we can return this socket pair.
				if ((theElem->fSocketB->GetDemuxer() == NULL) ||
					((!theElem->fSocketB->GetDemuxer()->AddrInMap(0, 0)) &&
					(!theElem->fSocketB->GetDemuxer()->AddrInMap(inSrcIPAddr, inSrcPort))))
				{
					theElem->fRefCount++;
					return theElem;
				}
				//If port is specified, there is NO WAY a socket pair can exist that matches
				//the criteria (because caller wants a specific ip & port combination)
				else if (inPort != 0)
					return NULL;
			}
		}
	}
	//if we get here, there is no pair already in the pool that matches the specified
	//criteria, so we have to create a new pair.
	return this->CreateUDPSocketPair(inIPAddr, inPort);
}

void UDPSocketPool::ReleaseUDPSocketPair(UDPSocketPair* inPair)
{
	OSMutexLocker locker(&fMutex);
	inPair->fRefCount--;
	if (inPair->fRefCount == 0)
	{
		fUDPQueue.Remove(&inPair->fElem);
		this->DestructUDPSocketPair(inPair);
	}
}

UDPSocketPair*	UDPSocketPool::CreateUDPSocketPair(UInt32 inAddr, UInt16 inPort)
{
	OSMutexLocker locker(&fMutex);
	UDPSocketPair* theElem = ConstructUDPSocketPair();
	Assert(theElem != NULL);
	if (theElem->fSocketA->Open() != OS_NoErr)
	{
		this->DestructUDPSocketPair(theElem);
		return NULL;
	}
	if (theElem->fSocketB->Open() != OS_NoErr)
	{
		this->DestructUDPSocketPair(theElem);
		return NULL;
	}
	
	// Set socket options on these new sockets
	this->SetUDPSocketOptions(theElem);
	
	//try to find an open pair of ports to bind these suckers tooo
	Bool16 foundPair = false;
	
	//If port is 0, then the caller doesn't care what port # we bind this socket to.
	//Otherwise, ONLY attempt to bind this socket to the specified port
	UInt16 startIndex = kLowestUDPPort;
	if (inPort != 0)
		startIndex = inPort;
	UInt16 stopIndex = kHighestUDPPort;
	if (inPort != 0)
		stopIndex = inPort;
		
	while ((!foundPair) && (startIndex <= kHighestUDPPort))
	{
		OS_Error theErr = theElem->fSocketA->Bind(inAddr, startIndex);
		if (theErr == OS_NoErr)
		{
			theErr = theElem->fSocketB->Bind(inAddr, startIndex+1);
			if (theErr == OS_NoErr)
			{
				foundPair = true;
				fUDPQueue.EnQueue(&theElem->fElem);
				theElem->fRefCount++;
				return theElem;
			}
		}
		//If we are looking to bind to a specific port set, and we couldn't then
		//just break it out here.
		if (inPort != 0)
			break;
		startIndex += 2;
	}
	//if we couldn't find a pair of sockets, make sure to clean up our mess
	this->DestructUDPSocketPair(theElem);
	return NULL;
}

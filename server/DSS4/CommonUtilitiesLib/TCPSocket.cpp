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
	File:		TCPSocket.cpp

	Contains:	implements TCPSocket class
					
	
	
*/

#ifndef __Win32__
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#endif

#include <errno.h>

#include "TCPSocket.h"
#include "SocketUtils.h"
#include "OS.h"


void TCPSocket::SnarfSocket( TCPSocket & fromSocket )
{
	// take the connection away from the other socket
	// and use it as our own.
	Assert(fFileDesc == EventContext::kInvalidFileDesc);
	this->Set( fromSocket.fFileDesc, &fromSocket.fRemoteAddr );
	
	// clear the old socket so he doesn't close and the like
	struct	sockaddr_in	 remoteaddr;
	
	::memset( &remoteaddr, 0, sizeof( remoteaddr ) );

	fromSocket.Set( EventContext::kInvalidFileDesc, &remoteaddr );

	// get the event context too
	this->SnarfEventContext( fromSocket );

}

void TCPSocket::Set(int inSocket, struct sockaddr_in* remoteaddr)
{
	fRemoteAddr = *remoteaddr;
	fFileDesc = inSocket;
	
	if ( inSocket != EventContext::kInvalidFileDesc ) 
	{
		//make sure to find out what IP address this connection is actually occuring on. That
		//way, we can report correct information to clients asking what the connection's IP is
#if __Win32__ || __MacOSX__ || __osf__
		int len = sizeof(fLocalAddr);
#else
		socklen_t len = sizeof(fLocalAddr);
#endif
		int err = ::getsockname(fFileDesc, (struct sockaddr*)&fLocalAddr, &len);
		AssertV(err == 0, OSThread::GetErrno());
		fState |= kBound;
		fState |= kConnected;
	}
	else
		fState = 0;
}

StrPtrLen*	TCPSocket::GetRemoteAddrStr()
{
	if (fRemoteStr.Len == kIPAddrBufSize)
		SocketUtils::ConvertAddrToString(fRemoteAddr.sin_addr, &fRemoteStr);
	return &fRemoteStr;
}

OS_Error  TCPSocket::Connect(UInt32 inRemoteAddr, UInt16 inRemotePort)
{
	::memset(&fRemoteAddr, 0, sizeof(fRemoteAddr));
    fRemoteAddr.sin_family = AF_INET;        /* host byte order */
    fRemoteAddr.sin_port = htons(inRemotePort); /* short, network byte order */
    fRemoteAddr.sin_addr.s_addr = htonl(inRemoteAddr);

    /* don't forget to error check the connect()! */
    int err = ::connect(fFileDesc, (sockaddr *)&fRemoteAddr, sizeof(fRemoteAddr));
	fState |= kConnected;
	
	if (err == -1)
	{
		fRemoteAddr.sin_port = 0;
		fRemoteAddr.sin_addr.s_addr = 0;
		return (OS_Error)OSThread::GetErrno();
	}
	
	return OS_NoErr;

}


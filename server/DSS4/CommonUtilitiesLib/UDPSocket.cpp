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
	File:		UDPSocket.cpp

	Contains:	Implementation of object defined in UDPSocket.h.

	
	
*/

#ifndef __Win32__
#include <sys/types.h>
#include <sys/socket.h>

#if __solaris__
#include "SocketUtils.h"
#endif

#if NEED_SOCKETBITS
#if __GLIBC__ >= 2
#include <bits/socket.h>
#else
#include <socketbits.h>
#endif
#endif
#endif

#include <errno.h>
#include "UDPSocket.h"
#include "OSMemory.h"


UDPSocket::UDPSocket(Task* inTask, UInt32 inSocketType)
: Socket(inTask, inSocketType), fDemuxer(NULL)
{
	if (inSocketType & kWantsDemuxer)
		fDemuxer = NEW UDPDemuxer();
		
	//setup msghdr
	::memset(&fMsgAddr, 0, sizeof(fMsgAddr));
}


OS_Error
UDPSocket::SendTo(UInt32 inRemoteAddr, UInt16 inRemotePort, void* inBuffer, UInt32 inLength)
{
	Assert(inBuffer != NULL);
	
	struct sockaddr_in 	theRemoteAddr;
	theRemoteAddr.sin_family = AF_INET;
	theRemoteAddr.sin_port = htons(inRemotePort);
	theRemoteAddr.sin_addr.s_addr = htonl(inRemoteAddr);

	// Win32 says that inBuffer is a char*
	int theErr = ::sendto(fFileDesc, (char*)inBuffer, inLength, 0,
							(sockaddr*)&theRemoteAddr, sizeof(theRemoteAddr));
	if (theErr == -1)
		return (OS_Error)OSThread::GetErrno();
	return OS_NoErr;
}

OS_Error UDPSocket::RecvFrom(UInt32* outRemoteAddr, UInt16* outRemotePort,
							void* ioBuffer, UInt32 inBufLen, UInt32* outRecvLen)
{
	Assert(outRecvLen != NULL);
	Assert(outRemoteAddr != NULL);
	Assert(outRemotePort != NULL);
	
#if __Win32__ || __MacOSX__ || __osf__
	int addrLen = sizeof(fMsgAddr);
#else
	socklen_t addrLen = sizeof(fMsgAddr);
#endif
	// Win32 says that ioBuffer is a char*
	SInt32 theRecvLen = ::recvfrom(fFileDesc, (char*)ioBuffer, inBufLen, 0, (sockaddr*)&fMsgAddr, &addrLen);
	if (theRecvLen == -1)
		return (OS_Error)OSThread::GetErrno();
	
	*outRemoteAddr = ntohl(fMsgAddr.sin_addr.s_addr);
	*outRemotePort = ntohs(fMsgAddr.sin_port);
	Assert(theRecvLen >= 0);
	*outRecvLen = (UInt32)theRecvLen;
	return OS_NoErr;		
}

OS_Error UDPSocket::JoinMulticast(UInt32 inRemoteAddr)
{
	struct ip_mreq	theMulti;
        UInt32 localAddr = fLocalAddr.sin_addr.s_addr; // Already in network byte order

#if __solaris__
	if( localAddr == htonl(INADDR_ANY) )
	     localAddr = htonl(SocketUtils::GetIPAddr(0));
#endif
	theMulti.imr_multiaddr.s_addr = htonl(inRemoteAddr);
	theMulti.imr_interface.s_addr = localAddr;
	int err = setsockopt(fFileDesc, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char*)&theMulti, sizeof(theMulti));
	//AssertV(err == 0, OSThread::GetErrno());
	if (err == -1)
	     return (OS_Error)OSThread::GetErrno();
	else
	     return OS_NoErr;
}

OS_Error UDPSocket::SetTtl(UInt16 timeToLive)
{
	// set the ttl
	u_char	nOptVal = (u_char)timeToLive;//dms - stevens pp. 496. bsd implementations barf
											//unless this is a u_char
	int err = setsockopt(fFileDesc, IPPROTO_IP, IP_MULTICAST_TTL, (char*)&nOptVal, sizeof(nOptVal));
	if (err == -1)
		return (OS_Error)OSThread::GetErrno();
	else
		return OS_NoErr;	
}

OS_Error UDPSocket::SetMulticastInterface(UInt32 inLocalAddr)
{
	// set the outgoing interface for multicast datagrams on this socket
	in_addr	theLocalAddr;
	theLocalAddr.s_addr = inLocalAddr;
	int err = setsockopt(fFileDesc, IPPROTO_IP, IP_MULTICAST_IF, (char*)&theLocalAddr, sizeof(theLocalAddr));
	AssertV(err == 0, OSThread::GetErrno());
	if (err == -1)
		return (OS_Error)OSThread::GetErrno();
	else
		return OS_NoErr;	
}

OS_Error UDPSocket::LeaveMulticast(UInt32 inRemoteAddr)
{
	struct ip_mreq	theMulti;
	theMulti.imr_multiaddr.s_addr = htonl(inRemoteAddr);
	theMulti.imr_interface.s_addr = htonl(fLocalAddr.sin_addr.s_addr);
	int err = setsockopt(fFileDesc, IPPROTO_IP, IP_DROP_MEMBERSHIP, (char*)&theMulti, sizeof(theMulti));
	if (err == -1)
		return (OS_Error)OSThread::GetErrno();
	else
		return OS_NoErr;	
}

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
	File:		TCPListenerSocket.cpp

	Contains:	implements TCPListenerSocket class
					
	
*/

#ifndef __Win32__
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <unistd.h>
#endif

#include <errno.h>

#include "TCPListenerSocket.h"
#include "Task.h"

OS_Error TCPListenerSocket::Listen(UInt32 queueLength)
{
	if (fFileDesc == EventContext::kInvalidFileDesc)
		return EBADF;
		
	int err = ::listen(fFileDesc, queueLength);
	if (err != 0)
		return (OS_Error)OSThread::GetErrno();
	return OS_NoErr;
}

OS_Error TCPListenerSocket::Initialize(UInt32 addr, UInt16 port)
{
	OS_Error err = this->TCPSocket::Open();
	if (0 == err) do
	{	
		// set SO_REUSEADDR socket option before calling bind.
#ifndef __Win32__
		// this causes problems on NT (multiple processes can bind simultaneously),
		// so don't do it on NT.
		this->ReuseAddr();
#endif
		err = this->Bind(addr, port);
		if (err != 0) break; // don't assert this is just a port already in use.

		//
		// Unfortunately we need to advertise a big buffer because our TCP sockets
		// can be used for incoming broadcast data. This could force the server
		// to run out of memory faster if it gets bogged down, but it is unavoidable.
		this->SetSocketRcvBufSize(32 * 1024);		
		err = this->Listen(kListenQueueLength);
		AssertV(err == 0, OSThread::GetErrno()); 
		if (err != 0) break;
		
	} while (false);
	
	return err;
}

void TCPListenerSocket::ProcessEvent(int /*eventBits*/)
{
	//we are executing on the same thread as every other
	//socket, so whatever you do here has to be fast.
	
	struct sockaddr_in addr;
#if __Win32__ || __MacOSX__ || __osf__
	int size = sizeof(addr);
#else
	socklen_t size = sizeof(addr);
#endif
	Task* theTask = NULL;
	TCPSocket* theSocket = NULL;
	
	//fSocket data member of TCPSocket.
	while (true)
	{
		int osSocket = accept(fFileDesc, (struct sockaddr*)&addr, &size);
		if (osSocket == -1)
		{
			//take a look at what this error is.
			int acceptError = OSThread::GetErrno();
			
			//If it's EAGAIN, there's nothing on the listen queue right now,
			//so modwatch and return
			if (acceptError == EAGAIN)
				break;

			//if this error gets returned, we're out of file desciptors, so let's
			//idle the listener until the condition clears up.
			if (acceptError == EMFILE)
			{
				fOutOfDescriptors = true;
				this->SetIdleTimer(kTimeBetweenAcceptsInMsec);
				return;//don't modwatch, because we're out of file descriptors anyway.
			}
			else
			{	
				WarnV( (acceptError == ENOENT) || (acceptError == EINTR), "Unusual accept error on socket -- ignoring");
				continue;
			}
		}
		if ((theTask = this->GetSessionTask(&theSocket)) == NULL)
			//this should be a disconnect. do an ioctl call?
			close(osSocket);
		else
		{	
			Assert(osSocket != EventContext::kInvalidFileDesc);
			
			//set options on the socket
			//we are a server, always disable nagle algorithm
			int one = 1;
			int err = ::setsockopt(osSocket, IPPROTO_TCP, TCP_NODELAY, (char*)&one, sizeof(int));
			AssertV(err == 0, OSThread::GetErrno());
			
			err = ::setsockopt(osSocket, SOL_SOCKET, SO_KEEPALIVE, (char*)&one, sizeof(int));
			AssertV(err == 0, OSThread::GetErrno());

			int	sndBufSize = 8L * 1024L;
			err = ::setsockopt(osSocket, SOL_SOCKET, SO_SNDBUF, (char*)&sndBufSize, sizeof(int));
			AssertV(err == 0, OSThread::GetErrno());

			//setup the socket. When there is data on the socket,
			//theTask will get an kReadEvent event
			theSocket->Set(osSocket, &addr);
			theSocket->InitNonBlocking(osSocket);
			theSocket->SetTask(theTask);
			theSocket->RequestEvent(EV_RE);
		}
	}
	
	fOutOfDescriptors = false;
	//after every accept we should modwatch to make sure we will continue
	//to get listen events
	this->RequestEvent(EV_RE);
}
SInt64 TCPListenerSocket::Run()
{
	EventFlags events = this->GetEvents();
	
	//
	// ProcessEvent cannot be going on when this object gets deleted, because
	// the resolve / release mechanism of EventContext will ensure this thread
	// will block before destructing stuff.
	if (events & Task::kKillEvent)
		return -1;
		
		
	//This function will get called when we have run out of file descriptors.
	//All we need to do is check the listen queue to see if the situation has
	//cleared up.
	(void)this->GetEvents();
	this->ProcessEvent(Task::kReadEvent);
	return 0;
}

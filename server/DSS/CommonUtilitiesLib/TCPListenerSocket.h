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
	File:		TCPListenerSocket.h

	Contains:	A TCP listener socket. When a new connection comes in, the listener
				attempts to assign the new connection to a socket object and a Task
				object. Derived classes must implement a method of getting new
				Task & socket objects
					
	
*/


#ifndef __TCPLISTENERSOCKET_H__
#define __TCPLISTENERSOCKET_H__

#include "TCPSocket.h"
#include "IdleTask.h"

class TCPListenerSocket : public TCPSocket, public IdleTask
{
	public:

		TCPListenerSocket() : 	TCPSocket(NULL, Socket::kNonBlockingSocketType), IdleTask(),
								fAddr(0), fPort(0), fOutOfDescriptors(false) {}
		virtual ~TCPListenerSocket() {}
		
		//
		// Send a TCPListenerObject a Kill event to delete it.
				
		//addr = listening address. port = listening port. Automatically
		//starts listening
		OS_Error		Initialize(UInt32 addr, UInt16 port);

		//You can query the listener to see if it is failing to accept
		//connections because the OS is out of descriptors.
		Bool16		IsOutOfDescriptors() { return fOutOfDescriptors; }

		//derived object must implement a way of getting tasks & sockets to this object 
		virtual Task*	GetSessionTask(TCPSocket** outSocket) = 0;
		
		virtual SInt64	Run();
			
	private:
	
		enum
		{
			kTimeBetweenAcceptsInMsec = 1000,	//UInt32
			kListenQueueLength = 128			//UInt32
		};

		virtual void ProcessEvent(int eventBits);
		OS_Error 	Listen(UInt32 queueLength);

		UInt32			fAddr;
		UInt16			fPort;
		
		Bool16			fOutOfDescriptors;
};
#endif // __TCPLISTENERSOCKET_H__


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
	File:		EventContext.h

	Contains:	An event context provides the intelligence to take an event
				generated from a UNIX file descriptor (usually EV_RE or EV_WR)
				and signal a Task. 
					

	
	
*/

#ifndef __EVENT_CONTEXT_H__
#define __EVENT_CONTEXT_H__

#include "OSThread.h"
#include "Task.h"
#include "OSRef.h"
#include "ev.h"

class EventThread;

class EventContext
{
	public:
	
		//
		// Constructor. Pass in the EventThread you would like to receive
		// events for this context, and the fd that this context applies to
		EventContext(int inFileDesc, EventThread* inThread);
		virtual ~EventContext() { if (fAutoCleanup) this->Cleanup(); }
		
		//
		// InitNonBlocking
		//
		// Sets inFileDesc to be non-blocking. Once this is called, the
		// EventContext object "owns" the file descriptor, and will close it
		// when Cleanup is called. This is necessary because of some weird
		// select() behavior. DON'T CALL CLOSE ON THE FD ONCE THIS IS CALLED!!!!
		void 			InitNonBlocking(int inFileDesc);

		//
		// Cleanup. Will be called by the destructor, but can be called earlier
		void			Cleanup();

		//
		// Arms this EventContext. Pass in the events you would like to receive
		void			RequestEvent(int theMask = EV_RE);

		
		//
		// Provide the task you would like to be notified
		void			SetTask(Task* inTask) { fTask = inTask; }
		
		// when the HTTP Proxy tunnels takes over a TCPSocket, we need to maintain this context too
		void 			SnarfEventContext( EventContext &fromContext );
		
		// Don't cleanup this socket automatically
		void			DontAutoCleanup() { fAutoCleanup = false; }
		
		// Direct access to the FD is not recommended, but is needed for modules
		// that want to use the Socket classes and need to request events on the fd.
		int				GetSocketFD()		{ return fFileDesc; }
		
		enum
		{
			kInvalidFileDesc = -1	//int
		};

	protected:

		//
		// ProcessEvent
		//
		// When an event occurs on this file descriptor, this function
		// will get called. Default behavior is to Signal the associated
		// task, but that behavior may be altered / overridden.
		//
		// Currently, we always generate a Task::kReadEvent
		virtual void ProcessEvent(int /*eventBits*/) { fTask->Signal(Task::kReadEvent); }

		int				fFileDesc;

	private:

		struct eventreq fEventReq;
		
		//+rt -- need to use a pointer so that one TCP Soket can steal the events from another
		OSRef			fRef;
		PointerSizedInt	fUniqueID;
		StrPtrLen 		fUniqueIDStr;
		EventThread*	fEventThread;
		Bool16			fWatchEventCalled;
		int				fEventBits;
		Bool16			fAutoCleanup;

		Task*			fTask;
#if DEBUG
		Bool16			fModwatched;
#endif
		
		static unsigned int	sUniqueID;
		
		friend class EventThread;
};

class EventThread : public OSThread
{
	public:
	
		EventThread() : OSThread() {}
		virtual ~EventThread() {}
	
	private:
	
		virtual void Entry();
		OSRefTable		fRefTable;
		
		friend class EventContext;
};

#endif //__EVENT_CONTEXT_H__

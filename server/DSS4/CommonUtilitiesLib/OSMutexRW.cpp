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
	File:		OSMutex.cpp

	Contains:	

	

*/

#include "OSMutexRW.h"
#include "OSMutex.h"
#include "OSCond.h"

#include <stdlib.h>
#include <string.h>

#if DEBUGMUTEXRW
	int OSMutexRW::fCount = 0;
	int OSMutexRW::fMaxCount =0;
#endif
	

#if DEBUGMUTEXRW
void OSMutexRW::CountConflict(int i)            
{
	fCount += i;
	if (i == -1) printf("Num Conflicts: %d\n", fMaxCount);
	if (fCount > fMaxCount)
	fMaxCount = fCount;

}
#endif

void OSMutexRW::LockRead()
{
	OSMutexLocker locker(&fInternalLock);
#if DEBUGMUTEXRW
	if (fState != 0) 
	{	printf("LockRead(conflict) fState = %d active readers = %d, waiting writers = %d, waiting readers=%d\n",fState,  fActiveReaders, fWriteWaiters, fReadWaiters);
		CountConflict(1);  
	}
 
#endif
	
 	AddReadWaiter();
  	while 	(	ActiveWriter() // active writer so wait
 			|| 	WaitingWriters() // reader must wait for write waiters
 			)
    {	
		fReadersCond.Wait(&fInternalLock,OSMutexRW::eMaxWait);
	}
		
	RemoveReadWaiter();
	AddActiveReader(); // add 1 to active readers
	fActiveReaders = fState;
	
#if DEBUGMUTEXRW
//	printf("LockRead(conflict) fState = %d active readers = %d, waiting writers = %d, waiting readers=%d\n",fState,  fActiveReaders, fWriteWaiters, fReadWaiters);

#endif
}

void OSMutexRW::LockWrite()
{
	OSMutexLocker locker(&fInternalLock);
	AddWriteWaiter();       //  1 writer queued 	       
#if DEBUGMUTEXRW

	if (Active()) 
	{	printf("LockWrite(conflict) state = %d active readers = %d, waiting writers = %d, waiting readers=%d\n", fState, fActiveReaders, fWriteWaiters, fReadWaiters);
		CountConflict(1);  
	}

	printf("LockWrite 'waiting' fState = %d locked active readers = %d, waiting writers = %d, waiting readers=%d\n",fState, fActiveReaders, fReadWaiters, fWriteWaiters);
#endif

 	while 	(ActiveReaders())  // active readers
 	{		
		fWritersCond.Wait(&fInternalLock,OSMutexRW::eMaxWait);
	}

 	RemoveWriteWaiter(); // remove from waiting writers
 	SetState(OSMutexRW::eActiveWriterState);    // this is the active writer    
 	fActiveReaders = fState; 
#if DEBUGMUTEXRW
//	printf("LockWrite 'locked' fState = %d locked active readers = %d, waiting writers = %d, waiting readers=%d\n",fState, fActiveReaders, fReadWaiters, fWriteWaiters);
#endif

}


void OSMutexRW::Unlock()
{			
	OSMutexLocker locker(&fInternalLock);
#if DEBUGMUTEXRW
//	printf("Unlock active readers = %d, waiting writers = %d, waiting readers=%d\n", fActiveReaders, fReadWaiters, fWriteWaiters);

#endif

	if (ActiveWriter()) 
	{			
		SetState(OSMutexRW::eNoWriterState); // this was the active writer 
		if (WaitingWriters()) // there are waiting writers
		{	fWritersCond.Signal();
		}
		else
		{	fReadersCond.Broadcast();
		}
#if DEBUGMUTEXRW
		printf("Unlock(writer) active readers = %d, waiting writers = %d, waiting readers=%d\n", fActiveReaders, fReadWaiters, fWriteWaiters);
#endif
	}
	else
	{
		RemoveActiveReader(); // this was a reader
		if (!ActiveReaders()) // no active readers
		{	SetState(OSMutexRW::eNoWriterState); // this was the active writer now no actives threads
			fWritersCond.Signal();
		} 
	}
	fActiveReaders = fState;

}



// Returns true on successful grab of the lock, false on failure
int OSMutexRW::TryLockWrite()
{
	int    status  = EBUSY;
	OSMutexLocker locker(&fInternalLock);

	if ( !Active() && !WaitingWriters()) // no writers, no readers, no waiting writers
	{
		this->LockWrite();
		status = 0;
	}

	return status;
}

int OSMutexRW::TryLockRead()
{
	int    status  = EBUSY;
	OSMutexLocker locker(&fInternalLock);

	if ( !ActiveWriter() && !WaitingWriters() ) // no current writers but other readers ok
	{
		this->LockRead(); 
		status = 0;
	}
	
	return status;
}




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
	File:		Task.cpp

	Contains:	implements Task class
					
	
*/

#include "Task.h"
#include "OS.h"
#include "OSMemory.h"
#include "atomic.h"
#include "OSMutexRW.h"

#define	TASK_DEBUG 0

unsigned int	Task::sThreadPicker = 0;
OSMutexRW 		TaskThreadPool::sMutexRW;

Task::Task()
: 	fEvents(0), fUseThisThread(NULL), fWriteLock(false), fTimerHeapElem(this), fTaskQueueElem(this)
{
#if DEBUG
	fInRunCount = 0;
#endif
	
}

Task::EventFlags Task::GetEvents()
{
	//Mask off every event currently in the mask except for the alive bit, of course,
	//which should remain unaffected and unreported by this call.
	EventFlags events = fEvents & kAliveOff;
	(void)atomic_sub(&fEvents, events);
	return events;
}

void Task::Signal(EventFlags events)
{
	//Fancy no mutex implementation. We atomically mask the new events into
	//the event mask. Because atomic_or returns the old state of the mask,
	//we only schedule this task once.
	events |= kAlive;
	EventFlags oldEvents = atomic_or(&fEvents, events);
	if ((!(oldEvents & kAlive)) && (TaskThreadPool::sNumTaskThreads > 0))
	{
		if (fUseThisThread != NULL)
			// Task needs to be placed on a particular thread.
			fUseThisThread->fTaskQueue.EnQueue(&fTaskQueueElem);
		else
		{
			//find a thread to put this task on
			unsigned int theThread = atomic_add(&sThreadPicker, 1);
			theThread %= TaskThreadPool::sNumTaskThreads;
			TaskThreadPool::sTaskThreadArray[theThread]->fTaskQueue.EnQueue(&fTaskQueueElem);
		}
	}
}


void	Task::GlobalUnlock()	
{	
	if (this->fWriteLock)
	{	this->fWriteLock = false;	
		TaskThreadPool::sMutexRW.Unlock();
	}												
}



void TaskThread::Entry()
{
	Task* theTask = NULL;
	
	while (true) 
	{
		theTask = this->WaitForTask();

		//
		// WaitForTask returns NULL when it is time to quit
		if (theTask == NULL)
			return;
					
		Bool16 doneProcessingEvent = false;
		
		while (!doneProcessingEvent)
		{
			//If a task holds locks when it returns from its Run function,
			//that would be catastrophic and certainly lead to a deadlock
#if DEBUG
			Assert(this->GetNumLocksHeld() == 0);
			Assert(theTask->fInRunCount == 0);
			theTask->fInRunCount++;
#endif
			theTask->fUseThisThread = NULL; // Each invocation of Run must independently
											// request a specific thread.
			SInt64 theTimeout = 0;
			
			if (theTask->fWriteLock)
			{	
				OSMutexWriteLocker mutexLocker(&TaskThreadPool::sMutexRW);
				theTimeout = theTask->Run();
				theTask->fWriteLock = false;
			}
			else
			{
				OSMutexReadLocker mutexLocker(&TaskThreadPool::sMutexRW);
				theTimeout = theTask->Run();
			
			}
#if DEBUG
			Assert(this->GetNumLocksHeld() == 0);
			theTask->fInRunCount--;
			Assert(theTask->fInRunCount == 0);
#endif			
			if (theTimeout < 0)
			{
				delete theTask;
				theTask = NULL;
				doneProcessingEvent = true;
			}
			else if (theTimeout == 0)
			{
				//We want to make sure that 100% definitely the task's Run function WILL
				//be invoked when another thread calls Signal. We also want to make sure
				//that if an event sneaks in right as the task is returning from Run()
				//(via Signal) that the Run function will be invoked again.
				doneProcessingEvent = compare_and_store(Task::kAlive, 0, &theTask->fEvents);
				if (doneProcessingEvent)
					theTask = NULL;	
			}
			else
			{
				//note that if we get here, we don't reset theTask, so it will get passed into
				//WaitForTask
				theTask->fTimerHeapElem.SetValue(OS::Milliseconds() + theTimeout);
				fHeap.Insert(&theTask->fTimerHeapElem);
				(void)atomic_or(&theTask->fEvents, Task::kIdleEvent);
				doneProcessingEvent = true;
			}
		
		
		#if	TASK_DEBUG
		SInt64	yieldStart = OS::Milliseconds();
		#endif
		
		this->ThreadYield();
		#if	TASK_DEBUG
		SInt64	yieldDur = OS::Milliseconds() - yieldStart;
		static SInt64	numZeroYields;
		
		if ( yieldDur > 1 )
		{
			printf( "TaskThread time in Yield %i, numZeroYields %i\n", (long)yieldDur, (long)numZeroYields );
			numZeroYields = 0;
		}
		else
			numZeroYields++;
		#endif
		
		}
	}
}

Task* TaskThread::WaitForTask()
{
	while (true)
	{
		SInt64 theCurrentTime = OS::Milliseconds();
		
		if ((fHeap.PeekMin() != NULL) && (fHeap.PeekMin()->GetValue() <= theCurrentTime))
			return (Task*)fHeap.ExtractMin()->GetEnclosingObject();
	
		//if there is an element waiting for a timeout, figure out how long we should wait.
		SInt32 theTimeout = 0;
		if (fHeap.PeekMin() != NULL)
			theTimeout = fHeap.PeekMin()->GetValue() - theCurrentTime;
		Assert(theTimeout >= 0);
		
		//
		// Make sure we can't go to sleep for some ridiculously short
		// period of time
		if (theTimeout < 50)
			theTimeout = 50;
			
		//wait...
		OSQueueElem* theElem = fTaskQueue.DeQueueBlocking(this, theTimeout);
		if (theElem != NULL)
			return (Task*)theElem->GetEnclosingObject();

		//
		// If we are supposed to stop, return NULL, which signals the caller to stop
		if (OSThread::GetCurrent()->IsStopRequested())
			return NULL;
	}	
}

TaskThread** TaskThreadPool::sTaskThreadArray = NULL;
UInt32		 TaskThreadPool::sNumTaskThreads = 0;

Bool16 TaskThreadPool::AddThreads(UInt32 numToAdd)
{
	Assert(sTaskThreadArray == NULL);
	sTaskThreadArray = new TaskThread*[numToAdd];
		
	for (UInt32 x = 0; x < numToAdd; x++)
	{
		sTaskThreadArray[x] = NEW TaskThread();
		sTaskThreadArray[x]->Start();
	}
	sNumTaskThreads = numToAdd;
	return true;
}

void TaskThreadPool::RemoveThreads()
{
	//Tell all the threads to stop
	for (UInt32 x = 0; x < sNumTaskThreads; x++)
		sTaskThreadArray[x]->SendStopRequest();

	//Because any (or all) threads may be blocked on the queue, cycle through
	//all the threads, signalling each one
	for (UInt32 y = 0; y < sNumTaskThreads; y++)
		sTaskThreadArray[y]->fTaskQueue.GetCond()->Signal();
	
	//Ok, now wait for the selected threads to terminate, deleting them and removing
	//them from the queue.
	for (UInt32 z = 0; z < sNumTaskThreads; z++)
		delete sTaskThreadArray[z];
	
	sNumTaskThreads = 0;
}

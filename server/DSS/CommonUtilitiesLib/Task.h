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
	File:		Task.h

	Contains:	Tasks are objects that can be scheduled. To schedule a task, you call its
				signal method, and pass in an event (events are bits and all events are defined
				below).
				
				Once Signal() is called, the task object will be scheduled. When it runs, its
				Run() function will get called. In order to clear the event, the derived task
				object must call GetEvents() (which returns the events that were sent).
				
				Calling GetEvents() implicitly "clears" the events returned. All events must
				be cleared before the Run() function returns, or Run() will be invoked again
				immediately.
					
	
	
	
*/

#ifndef __TASK_H__
#define __TASK_H__

#include "OSQueue.h"
#include "OSHeap.h"
#include "OSThread.h"
#include "OSMutexRW.h"

class TaskThread;

class Task
{
	public:
		
		typedef unsigned int EventFlags;

		//EVENTS
		//here are all the events that can be sent to a task
		enum
		{
			kKillEvent = 	0x00000001, //these are all of type "EventFlags"
			kIdleEvent = 	0x00000002,
			kStartEvent	= 	0x00000004,
			kTimeoutEvent = 0x00000008
		};

		//socket events
		enum
		{
			kReadEvent = 		0x00000010,	//All of type "EventFlags"
			kWriteEvent = 		0x00000020
		};
		
		//CONSTRUCTOR / DESTRUCTOR
		//You must assign priority at create time.
								Task();
		virtual					~Task() {}

		//return:
		// >0-> invoke me after this number of MilSecs with a kIdleEvent
		// 0 don't reinvoke me at all.
		//-1 delete me
		//Suggested practice is that any task should be deleted by returning true from the
		//Run function. That way, we know that the Task is not running at the time it is
		//deleted. This object provides no protection against calling a method, such as Signal,
		//at the same time the object is being deleted (because it can't really), so watch
		//those dangling references!
		virtual SInt64			Run() = 0;
		
		//Send an event to this task.
		void 					Signal(EventFlags eventFlags);
		void					GlobalUnlock();		
		
	protected:
	
		//Only the tasks themselves may find out what events they have received
		EventFlags 				GetEvents();
		
		// ForceSameThread
		//
		// A task, inside its run function, may want to ensure that the same task thread
		// is used for subsequent calls to Run(). This may be the case if the task is holding
		// a mutex between calls to run. By calling this function, the task ensures that the
		// same task thread will be used for the next call to Run(). It only applies to the
		// next call to run.
		void					ForceSameThread()	{ 	fUseThisThread = (TaskThread*)OSThread::GetCurrent();
														Assert(fUseThisThread != NULL);
													}
		SInt64					CallLocked()		{   ForceSameThread();
														fWriteLock = true;
														return (SInt64) 10; // minimum of 10 milliseconds between locks
													}

	private:

		enum
		{
			kAlive = 			0x80000000,	//EventFlags, again
			kAliveOff =			0x7fffffff
		};

		void			SetTaskThread(TaskThread *thread);
		
		EventFlags		fEvents;
		TaskThread*		fUseThisThread;
		Bool16			fWriteLock;

#if DEBUG
		//The whole premise of a task is that the Run function cannot be re-entered.
		//This debugging variable ensures that that is always the case
		volatile UInt32	fInRunCount;
#endif

		//This could later be optimized by using a timing wheel instead of a heap,
		//and that way we wouldn't need both a heap elem and a queue elem here (just queue elem)
		OSHeapElem		fTimerHeapElem;
		OSQueueElem		fTaskQueueElem;
		
		//Variable used for assigning tasks to threads in a round-robin fashion
		static unsigned int sThreadPicker;
		
		friend class	TaskThread;	
};

class TaskThread : public OSThread
{
	public:
	
		//Implementation detail: all tasks get run on TaskThreads.
		
						TaskThread() :	OSThread(), fTaskThreadPoolElem(this)
										{}
		virtual			~TaskThread() { this->StopAndWaitForThread(); }

	private:
	
		enum
		{
			kMinWaitTimeInMilSecs = 10	//UInt32
		};

		virtual void 	Entry();
		Task* 			WaitForTask();
		
		OSQueueElem		fTaskThreadPoolElem;
		
		OSHeap				fHeap;
		OSQueue_Blocking	fTaskQueue;
		
		
		friend class Task;
		friend class TaskThreadPool;
};

//Because task threads share a global queue of tasks to execute,
//there can only be one pool of task threads. That is why this object
//is static.
class TaskThreadPool {
public:

	//Adds some threads to the pool
	static Bool16				AddThreads(UInt32 numToAdd);
	//returns num actually removed (this call is non-blocking)
	static void RemoveThreads();
	
private:

	static TaskThread**		sTaskThreadArray;
	static UInt32			sNumTaskThreads;
	static OSMutexRW		sMutexRW;
	
	friend class Task;
	friend class TaskThread;
};

#endif

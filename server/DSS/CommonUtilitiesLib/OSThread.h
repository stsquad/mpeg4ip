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
	File:		OSThread.h

	Contains:	A thread abstraction

	

*/

// OSThread.h
#ifndef __OSTHREAD__
#define __OSTHREAD__

#ifndef __Win32__
#if __PTHREADS__
#if __solaris__
	#include <errno.h>
#else
	#include <sys/errno.h>
#endif
	#include <pthread.h>
#else
	#include <mach/mach.h>
	#include <mach/cthreads.h>
#endif
#endif

#include "OSHeaders.h"
#include "DateTranslator.h"

class OSThread
{

public:
				//
				// Call before calling any other OSThread function
				static void		Initialize();
				
								OSThread();
	virtual 					~OSThread();
	
	//
	// Derived classes must implement their own entry function
	virtual 	void 			Entry() = 0;
				void 			Start();
				
				static void		ThreadYield();
				static void		Sleep(UInt32 inMsec);
				
				void			Join();
				void			SendStopRequest() { fStopRequested = true; }
				Bool16			IsStopRequested() { return fStopRequested; }
				void			StopAndWaitForThread();

				void*			GetThreadData()			{ return fThreadData; }
				void			SetThreadData(void* inThreadData) { fThreadData = inThreadData; }
				
				// As a convienence to higher levels, each thread has its own date buffer
				DateBuffer*		GetDateBuffer()			{ return &fDateBuffer; }
				
				static void*	GetMainThreadData()		{ return sMainThreadData; }
				static void		SetMainThreadData(void* inData) { sMainThreadData = inData; }
#if DEBUG
				UInt32			GetNumLocksHeld() { return 0; }
				void			IncrementLocksHeld() {}
				void			DecrementLocksHeld() {}
#endif

#if __linux__
				static void		WrapSleep( Bool16 wrapSleep) {sWrapSleep = wrapSleep; }
#endif

#ifdef __Win32__
	static int			GetErrno();
	static DWORD		GetCurrentThreadID() { return ::GetCurrentThreadId(); }
#elif __PTHREADS__
	static  int			GetErrno() { return errno; }
	static  pthread_t	GetCurrentThreadID() { return ::pthread_self(); }
#else
	static	int			GetErrno() { return cthread_errno();	}	
	static  cthread_t	GetCurrentThreadID() { return cthread_self(); }
#endif

	static	OSThread*	GetCurrent();
	
private:

#ifdef __Win32__
	static DWORD	sThreadStorageIndex;
#elif __PTHREADS__
	static pthread_key_t	gMainKey;
#ifdef _POSIX_THREAD_PRIORITY_SCHEDULING
	static pthread_attr_t sThreadAttr;
#endif
#endif

	Bool16 fStopRequested;
	Bool16 fJoined;

#ifdef __Win32__
	HANDLE			fThreadID;
#elif __PTHREADS__
	pthread_t		fThreadID;
#else
	UInt32 			fThreadID;
#endif
	void*			fThreadData;
	DateBuffer		fDateBuffer;
	
	static void*	sMainThreadData;
#ifdef __Win32__
	static unsigned int WINAPI _Entry(LPVOID inThread);
#else
	static void* 	_Entry(void* inThread);
#endif

#if __linux__
	static Bool16 sWrapSleep;
#endif
};

class OSThreadDataSetter
{
	public:
	
		OSThreadDataSetter(void* inInitialValue, void* inFinalValue) : fFinalValue(inFinalValue)
			{ OSThread::GetCurrent()->SetThreadData(inInitialValue); }
			
		~OSThreadDataSetter() { OSThread::GetCurrent()->SetThreadData(fFinalValue); }
		
	private:
	
		void*	fFinalValue;
};


#endif


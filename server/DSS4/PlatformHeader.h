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

// Build flags. How do you want your server built?
#define DEBUG 0
#define ASSERT 1
#define MEMORY_DEBUGGING  0 //enable this to turn on really fancy debugging of memory leaks, etc...
#define QTFILE_MEMORY_DEBUGGING 0

// Platform-specific switches
#if __MacOSX__

#define USE_ATOMICLIB 0
#define MACOSXEVENTQUEUE 1
#define __PTHREADS__ 	1
#define __PTHREADS_MUTEXES__	1
#define BIGENDIAN		1
#define ALLOW_NON_WORD_ALIGN_ACCESS 1
#define USE_THREAD 		0 //Flag used in QTProxy
#define THREADING_IS_COOPERATIVE		0 
#define USE_THR_YIELD	0
#define kPlatformNameString 	"MacOSX"
#define EXPORT
#define MACOSX_PUBLICBETA 0

#elif __Win32__

#define USE_ATOMICLIB 0
#define MACOSXEVENTQUEUE 0
#define __PTHREADS__ 	0
#define __PTHREADS_MUTEXES__	0
//#define BIGENDIAN		0	// Defined equivalently inside windows
#define ALLOW_NON_WORD_ALIGN_ACCESS 1
#define USE_THREAD 		0 //Flag used in QTProxy
#define THREADING_IS_COOPERATIVE		0
#define USE_THR_YIELD	0
#define kPlatformNameString 	"Win32"
#define EXPORT	__declspec(dllexport)

#elif __MACOS__ 

#define USE_ATOMICLIB 0
#define MACOSXEVENTQUEUE 0
#define __PTHREADS__	1
#define __PTHREADS_MUTEXES__	1
#define BIGENDIAN		1
#define ALLOW_NON_WORD_ALIGN_ACCESS 1
#define USE_THREAD 		1 //Flag used in QTProxy
#define THREADING_IS_COOPERATIVE		0
#define USE_THR_YIELD	0
#define kPlatformNameString 	"MacOS"
#define EXPORT

#elif __linux__ 

#define USE_ATOMICLIB 0
#define MACOSXEVENTQUEUE 0
#define __PTHREADS__	1
#define __PTHREADS_MUTEXES__	1
#define BIGENDIAN		0
#define ALLOW_NON_WORD_ALIGN_ACCESS 1
#define USE_THREAD 		0 //Flag used in QTProxy
#define THREADING_IS_COOPERATIVE		0 
#define USE_THR_YIELD	0
#define kPlatformNameString 	"Linux"
#define EXPORT
#define _REENTRANT 1

#elif __linuxppc__ 

#define USE_ATOMICLIB 0
#define MACOSXEVENTQUEUE 0
#define __PTHREADS__	1
#define __PTHREADS_MUTEXES__	1
#define BIGENDIAN		1
#define ALLOW_NON_WORD_ALIGN_ACCESS 1
#define USE_THREAD 		0 //Flag used in QTProxy
#define THREADING_IS_COOPERATIVE		0 
#define USE_THR_YIELD	0
#define kPlatformNameString 	"LinuxPPC"
#define EXPORT
#define _REENTRANT 1

#elif __FreeBSD__ 

#define USE_ATOMICLIB 0
#define MACOSXEVENTQUEUE 0
#define __PTHREADS__	1
#define __PTHREADS_MUTEXES__	1
#define BIGENDIAN		0
#define ALLOW_NON_WORD_ALIGN_ACCESS 1
#define USE_THREAD 		1 //Flag used in QTProxy
#define THREADING_IS_COOPERATIVE		1 
#define USE_THR_YIELD	0
#define kPlatformNameString 	"FreeBSD"
#define EXPORT
#define _REENTRANT 1

#elif __solaris__ 

#define USE_ATOMICLIB 0
#define MACOSXEVENTQUEUE 0
#define __PTHREADS__	1
#define __PTHREADS_MUTEXES__	1
#define BIGENDIAN		1
#define ALLOW_NON_WORD_ALIGN_ACCESS 0
#define USE_THREAD 		1 //Flag used in QTProxy
#define THREADING_IS_COOPERATIVE		0
#define USE_THR_YIELD	0
#define kPlatformNameString 	"Solaris"
#define EXPORT
#define _REENTRANT 1

#elif defined(__osf__)

#define __osf__ 1
#define USE_ATOMICLIB 0
#define MACOSXEVENTQUEUE 0
#define __PTHREADS__	1
#define __PTHREADS_MUTEXES__	1
#define BIGENDIAN		0
#define ALLOW_NON_WORD_ALIGN_ACCESS 0
#define USE_THREAD 		1 //Flag used in QTProxy
#define THREADING_IS_COOPERATIVE		0
#define USE_THR_YIELD	0
#define kPlatformNameString 	"Tru64UNIX"
#define EXPORT

#endif

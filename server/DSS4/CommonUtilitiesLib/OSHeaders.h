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

#ifndef OSHeaders_H
#define OSHeaders_H

/* Platform-specific components */
#if __linux__ || __linuxppc__ || __FreeBSD__ || __MacOSX__
	
        /* Defines */
        #define _64BITARG_ "q"

	/* paths */
	#define	kEOLString "\n"
	#define	kPathDelimiterString "/"
	#define	kPathDelimiterChar '/'
	#define kPartialPathBeginsWithDelimiter 0

	/* Includes */
	#include <sys/types.h>
	
	/* Constants */
	#define QT_TIME_TO_LOCAL_TIME	(-2082844800)
	#define QT_PATH_SEPARATOR		'/'

	/* Typedefs */
	typedef unsigned int		PointerSizedInt;
	typedef unsigned char		UInt8;
	typedef signed char			SInt8;
	typedef unsigned short		UInt16;
	typedef signed short		SInt16;
	typedef unsigned long		UInt32;
	typedef signed long			SInt32;
	typedef signed long long	SInt64;
	typedef unsigned long long	UInt64;
	typedef float				Float32;
	typedef double				Float64;
	typedef UInt16				Bool16;
	typedef UInt8				Bool8;
	
	typedef unsigned long		FourCharCode;
	typedef FourCharCode		OSType;

	#ifdef	FOUR_CHARS_TO_INT
	#error Conflicting Macro "FOUR_CHARS_TO_INT"
	#endif

	#define	FOUR_CHARS_TO_INT( c1, c2, c3, c4 )  ( c1 << 24 | c2 << 16 | c3 << 8 | c4 )

	#ifdef	TW0_CHARS_TO_INT
	#error Conflicting Macro "TW0_CHARS_TO_INT"
	#endif
		
	#define	TW0_CHARS_TO_INT( c1, c2 )  ( c1 << 8 | c2 )

#elif __Win32__
	
	/* Defines */
	#define _64BITARG_ "I64"

	/* paths */
	#define	kEOLString "\r\n"
	#define	kPathDelimiterString "\\"
	#define	kPathDelimiterChar '\\'
	#define kPartialPathBeginsWithDelimiter 0
	
	#define crypt(buf, salt) ((char*)buf)
	
	/* Includes */
	#include <windows.h>
	#include <winsock2.h>
	#include <mswsock.h>
	#include <process.h>
	#include <ws2tcpip.h>
	#include <io.h>
	#include <direct.h>
	#include <errno.h>

	
	#define R_OK 0
	#define W_OK 1
		
	// POSIX errorcodes
	#define	ENOTCONN 1002
	#define	EADDRINUSE 1004
	#define EINPROGRESS 1007
	#define ENOBUFS 1008
	#define EADDRNOTAVAIL 1009

	// Winsock does not use iovecs
	struct iovec {
		u_long	iov_len; // this is not the POSIX definition, it is rather defined to be
		char FAR*	iov_base; // equivalent to a WSABUF for easy integration into Win32
	};
	
	/* Constants */
	#define QT_TIME_TO_LOCAL_TIME	(-2082844800)
	#define QT_PATH_SEPARATOR		'/'

	/* Typedefs */
	typedef unsigned int		PointerSizedInt;
	typedef unsigned char		UInt8;
	typedef signed char			SInt8;
	typedef unsigned short		UInt16;
	typedef signed short		SInt16;
	typedef unsigned long		UInt32;
	typedef signed long			SInt32;
	typedef LONGLONG			SInt64;
	typedef ULONGLONG			UInt64;
	typedef float				Float32;
	typedef double				Float64;
	typedef UInt16				Bool16;
	typedef UInt8				Bool8;
	
	typedef unsigned long		FourCharCode;
	typedef FourCharCode		OSType;

	#ifdef	FOUR_CHARS_TO_INT
	#error Conflicting Macro "FOUR_CHARS_TO_INT"
	#endif

	#define	FOUR_CHARS_TO_INT( c1, c2, c3, c4 )  ( c1 << 24 | c2 << 16 | c3 << 8 | c4 )

	#ifdef	TW0_CHARS_TO_INT
	#error Conflicting Macro "TW0_CHARS_TO_INT"
	#endif
		
	#define	TW0_CHARS_TO_INT( c1, c2 )  ( c1 << 8 | c2 )

#elif __MACOS__

	#define _64BITARG_ "ll"
	
	/* paths */
	#define	kEOLString "\r"
	#define	kPathDelimiterString ":"
	#define	kPathDelimiterChar ':'
	#define kPartialPathBeginsWithDelimiter 1

	/* Includes */
	#include <sys/types.h>

	/* Constants */
	#define QT_TIME_TO_LOCAL_TIME	(126115200)
	#define QT_PATH_SEPARATOR		':'
	
	/* Typedefs */
	typedef unsigned int		PointerSizedInt;
	typedef unsigned char		UInt8;
	typedef signed char			SInt8;
	typedef unsigned short		UInt16;
	typedef signed short		SInt16;
	typedef unsigned long		UInt32;
	typedef signed long			SInt32;
	typedef signed long long	SInt64;
	typedef unsigned long long	UInt64;
	typedef float				Float32;
	typedef short double		Float64;
	typedef UInt16				Bool16;
	typedef UInt8				Bool8;
	
	typedef unsigned long		FourCharCode;
	typedef FourCharCode		OSType;

	#ifdef	FOUR_CHARS_TO_INT
	#error Conflicting Macro "FOUR_CHARS_TO_INT"
	#endif

	#define	FOUR_CHARS_TO_INT( c1, c2, c3, c4 )  ( c1 << 24 | c2 << 16 | c3 << 8 | c4 )

	#ifdef	TW0_CHARS_TO_INT
	#error Conflicting Macro "TW0_CHARS_TO_INT"
	#endif
		
	#define	TW0_CHARS_TO_INT( c1, c2 )  ( c1 << 8 | c2 )

#elif __sgi
	/* Defines */
	#define _64BITARG_ "ll"

	/* paths */
	#define	kPathDelimiterString "/"
	#define	kPathDelimiterChar '/'
	#define kPartialPathBeginsWithDelimiter 0

	/* Includes */
	#include <sys/types.h>
	#include <netinet/in.h>
	#include <pthread.h>
	
	/* Constants */
	#define QT_TIME_TO_LOCAL_TIME	(-2082844800)
	#define QT_PATH_SEPARATOR		'/'

	/* Typedefs */
	typedef unsigned char		boolean;
	#define true				1
	#define false				0

	typedef unsigned int		PointerSizedInt;
	typedef unsigned char		UInt8;
	typedef signed char			SInt8;
	typedef unsigned short		UInt16;
	typedef signed short		SInt16;
	typedef unsigned long		UInt32;
	typedef signed long			SInt32;
	typedef signed long long	SInt64;
	typedef unsigned long long	UInt64;
	typedef float				Float32;
	typedef double				Float64;
	
	typedef unsigned long		FourCharCode;
	typedef FourCharCode		OSType;

	/* Nulled-out new() for use without memory debugging */
	#define NEW(t,c,v) new c v
	#define NEW_ARRAY(t,c,n) new c[n]

	#define thread_t	pthread_t
	#define cthread_errno() errno

	#ifdef	FOUR_CHARS_TO_INT
	#error Conflicting Macro "FOUR_CHARS_TO_INT"
	#endif

	#define	FOUR_CHARS_TO_INT( c1, c2, c3, c4 )  ( c1 << 24 | c2 << 16 | c3 << 8 | c4 )

	#ifdef	TW0_CHARS_TO_INT
	#error Conflicting Macro "TW0_CHARS_TO_INT"
	#endif
		
	#define	TW0_CHARS_TO_INT( c1, c2 )  ( c1 << 8 | c2 )

#elif defined(sun) // && defined(sparc)

	/* Defines */
	#define _64BITARG_ "ll"

	/* paths */
	#define	kPathDelimiterString "/"
	#define	kPathDelimiterChar '/'
	#define kPartialPathBeginsWithDelimiter 0
	#define	kEOLString "\n"

	/* Includes */
	#include <sys/types.h>
	#include <sys/byteorder.h>
	
	/* Constants */
	#define QT_TIME_TO_LOCAL_TIME	(-2082844800)
	#define QT_PATH_SEPARATOR		'/'

	/* Typedefs */
	//typedef unsigned char		Bool16;
	//#define true				1
	//#define false				0

	typedef unsigned int		PointerSizedInt;
	typedef unsigned char		UInt8;
	typedef signed char			SInt8;
	typedef unsigned short		UInt16;
	typedef signed short		SInt16;
	typedef unsigned long		UInt32;
	typedef signed long			SInt32;
	typedef signed long long	SInt64;
	typedef unsigned long long	UInt64;
	typedef float				Float32;
	typedef double				Float64;
	typedef UInt16				Bool16;
	typedef UInt8				Bool8;
	
	typedef unsigned long		FourCharCode;
	typedef FourCharCode		OSType;

	#ifdef	FOUR_CHARS_TO_INT
	#error Conflicting Macro "FOUR_CHARS_TO_INT"
	#endif

	#define	FOUR_CHARS_TO_INT( c1, c2, c3, c4 )  ( c1 << 24 | c2 << 16 | c3 << 8 | c4 )

	#ifdef	TW0_CHARS_TO_INT
	#error Conflicting Macro "TW0_CHARS_TO_INT"
	#endif
		
	#define	TW0_CHARS_TO_INT( c1, c2 )  ( c1 << 8 | c2 )

#elif defined(__osf__)
	
   /* Defines */
    #define _64BITARG_ "l"

	/* paths */
	#define	kEOLString "\n"
	#define	kPathDelimiterString "/"
	#define	kPathDelimiterChar '/'
	#define kPartialPathBeginsWithDelimiter 0

	/* Includes */
	#include <sys/types.h>
	#include <machine/endian.h>
	
	/* Constants */
	#define QT_TIME_TO_LOCAL_TIME	(-2082844800)
	#define QT_PATH_SEPARATOR		'/'

	/* Typedefs */
	typedef unsigned long		PointerSizedInt;
	typedef unsigned char		UInt8;
	typedef signed char			SInt8;
	typedef unsigned short		UInt16;
	typedef signed short		SInt16;
	typedef unsigned int		UInt32;
	typedef signed int			SInt32;
	typedef signed long			SInt64;
	typedef unsigned long		UInt64;
	typedef float				Float32;
	typedef double				Float64;
	typedef UInt16				Bool16;
	typedef UInt8				Bool8;
	
	typedef unsigned int		FourCharCode;
	typedef FourCharCode		OSType;

	#ifdef	FOUR_CHARS_TO_INT
	#error Conflicting Macro "FOUR_CHARS_TO_INT"
	#endif

	#define	FOUR_CHARS_TO_INT( c1, c2, c3, c4 )  ( c1 << 24 | c2 << 16 | c3 << 8 | c4 )

	#ifdef	TW0_CHARS_TO_INT
	#error Conflicting Macro "TW0_CHARS_TO_INT"
	#endif
		
	#define	TW0_CHARS_TO_INT( c1, c2 )  ( c1 << 8 | c2 )


#endif

enum
{
	OS_NoErr = 0,
	OS_BadURLFormat = -100,
	OS_NotEnoughSpace = -101
};
typedef UInt32 OS_Error;


#endif /* OSHeaders_H */

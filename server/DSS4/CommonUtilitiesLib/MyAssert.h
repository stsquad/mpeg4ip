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

#ifndef _MYASSERT_H_
#define _MYASSERT_H_

#include <stdio.h>

#ifdef __cplusplus
class AssertLogger
{
	public:
		// An interface so the MyAssert function can write a message
		virtual void LogAssert(char* inMessage) = 0;
};

// If a logger is provided, asserts will be logged. Otherwise, asserts will cause a bus error
void SetAssertLogger(AssertLogger* theLogger);
#endif

#if ASSERT	
	void MyAssert(char *s);

	#define Assert(condition)	 {								\
	 															\
		if (!(condition)) 										\
		{														\
			char s[65];											\
			sprintf (s, "_Assert: %s, %d",__FILE__, __LINE__ );	\
			MyAssert(s); 										\
		} 	}


	#define AssertV(condition,errNo) 	{									\
		if (!(condition)) 													\
		{ 																	\
			char s[65];														\
			sprintf( s, "_AssertV: %s, %d (%d)",__FILE__, __LINE__, errNo );	\
			MyAssert(s);													\
		}	}
									 
										 
	#define Warn(condition) {								        \
			if (!(condition)) 										\
				printf( "_Warn: %s, %d\n",__FILE__, __LINE__ ); 	}															
										 
	#define WarnV(condition,msg)		{								\
		if (!(condition)) 												\
			printf ("_WarnV: %s, %d (%s)\n",__FILE__, __LINE__, msg );	}													
										 
	#define WarnVE(condition,msg,err)		{								\
		if (!(condition)) 												\
			printf ("_WarnV: %s, %d (%s, %s [err=%d])\n",__FILE__, __LINE__, msg, strerror(err), err );	}

#else
			
	#define Assert(condition) ((void) 0)
	#define AssertV(condition,errNo) ((void) 0)
	#define Warn(condition) ((void) 0)
	#define WarnV(condition,msg) ((void) 0)

#endif
#endif //_MY_ASSERT_H_

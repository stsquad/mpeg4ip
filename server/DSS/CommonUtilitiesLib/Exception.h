/*
 * Copyright (c) 1999 Apple Computer, Inc. All rights reserved.
 *
 * @APPLE_LICENSE_HEADER_START@
 * 
 * Copyright (c) 1999 Apple Computer, Inc.  All Rights Reserved.
 * The contents of this file constitute Original Code as defined in and are 
 * subject to the Apple Public Source License Version 1.1 (the "License").  
 * You may not use this file except in compliance with the License.  Please 
 * obtain a copy of the License at http://www.apple.com/publicsource and 
 * read it before using this file.
 * 
 * This Original Code and all software distributed under the License are 
 * distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY KIND, EITHER 
 * EXPRESS OR IMPLIED, AND APPLE HEREBY DISCLAIMS ALL SUCH WARRANTIES, 
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY, FITNESS 
 * FOR A PARTICULAR PURPOSE OR NON-INFRINGEMENT.  Please see the License for 
 * the specific language governing rights and limitations under the 
 * License.
 * 
 * 
 * @APPLE_LICENSE_HEADER_END@
 */
/*
	File:		Exception.h

	Contains:	Stuff for exceptions.

	

*/

#ifndef __EXCEPTION__
#define __EXCEPTION__

#include <stdio.h>

class Exception {
public:
	Exception(char* name = NULL, int status = 0);

//
// modifiers...
//

	void SetStatus(int status);

	void SetBreakOnThrow();
	void ClearBreakOnThrow();

	void SetLogOnThrow();
	void ClearLogOnThrow();
	static void SetLogNextThrow(int logState);

	int GetStatus();
	
	void PreThrow();

	void BreakOnThrow();
	void LogOnThrow();

private:
	char *name_;
	int status_;
	int breakOnThrow_;
	static int globalBreakOnThrow_;
	int logOnThrow_;
	static int logNextThrow_;
};

inline void operator + (Exception& ex) {ex.PreThrow(); throw &ex;};

#define DeclareExceptionClass(EXCLASS) \
class t##EXCLASS; \
extern t##EXCLASS EXCLASS; \
class t##EXCLASS : public Exception \
{ \
	public: \
		t##EXCLASS(char* name = NULL, int status = 0) : Exception(name, status) {}; \
}
//}; 
//inline void operator + (t##EXCLASS& ex) {ex.PreThrow(); throw &ex;}


#define ImplementExceptionClass(EXCLASS, status) \
	t##EXCLASS EXCLASS(#EXCLASS, status)

#define	Try_ try {

#define	Catch_(EX) } catch(t##EX* exp) { t##EX& ex = *exp;

#define Catch__  } catch(Exception* exp) { Exception& ex = *exp;
			
#define	EndCatch_ }

#define Throw_ +

#define Throw__ throw

#if 0
Try_
	foo();
	Try_
		foo();
	Catch__
		bat();
	EndCatch_
Catch__
	bar();
EndCatch_
#endif


template<class T>
class NewHelperCustom {
public:
	NewHelperCustom(T* ptr)				{ p = ptr; };
	~NewHelperCustom()					{ if (p) p->operator delete(p); };
	void Success()						{ p = NULL; };
private:
	T *p;
};



//DeclareExceptionClass(OS_E);
//DeclareExceptionClass(Assert_E);
DeclareExceptionClass(Cancel_E);
//DeclareExceptionClass(Generic_E);

#define ThrowIfNULL_(v) if (v == NULL) Throw_ (OS_E)

#endif

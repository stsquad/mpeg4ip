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
	File:		Exception.cpp

	Contains:	Implementation of Exception class


	

*/

#include <stdio.h>

#include "Exception.h"

//TestClass TestClass::fClassObject("test",0);

//ImplementExceptionClass(OS_E, 0);
//ImplementExceptionClass(Assert_E, -5014);
//ImplementExceptionClass(Generic_E, 0);
ImplementExceptionClass(Cancel_E, 0);

int Exception::globalBreakOnThrow_ = true;
int Exception::logNextThrow_ = true;

Exception::Exception(char* name, int status)
{
	name_ = name;
	status_ = status;
	breakOnThrow_ = false;
	logOnThrow_ = true;
}
void Exception::PreThrow()
{
	BreakOnThrow();
	LogOnThrow();
}
int Exception::GetStatus()
{
	return status_;
}
void Exception::SetStatus(int status)
{
	status_ = status;
}
void Exception::BreakOnThrow()
{
	if (globalBreakOnThrow_ && breakOnThrow_)
		(*(long*)0) = 0;
}
void Exception::SetLogNextThrow(int logState)
{
	logNextThrow_ = logState;
}
void Exception::SetBreakOnThrow()
{
	breakOnThrow_ = true;
}
void Exception::ClearBreakOnThrow()
{
	breakOnThrow_ = false;
}
void Exception::LogOnThrow()
{
}
void Exception::SetLogOnThrow()
{
	logOnThrow_ = true;
}
void Exception::ClearLogOnThrow()
{
	logOnThrow_ = false;
}

#if __MacOSXServer__ || __FreeBSD__
// We are using exceptions, and X requires this
void terminate() {}
#endif


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
	File:		QTSS_Private.h

	Contains:	Implementation-specific structures and typedefs used by the
				implementation of QTSS API in the Darwin Streaming Server
					
	
	
*/


#ifndef QTSS_PRIVATE_H
#define QTSS_PRIVATE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "OSHeaders.h"
#include "QTSS.h"

class QTSSModule;
class Task;

typedef QTSS_Error 	(*QTSS_CallbackProcPtr)(...);
typedef void*			(*QTSS_CallbackPtrProcPtr)(...);

enum
{
	// Indexes for each callback routine. Addresses of the callback routines get
	// placed in an array.
	
	kNewCallback 					= 0,
	kDeleteCallback					= 1,
	kMillisecondsCallback			= 2,
	kConvertToUnixTimeCallback		= 3,
	kAddRoleCallback 				= 4,
	kAddAttributeCallback 			= 5,
	kIDForTagCallback 				= 6,
	kGetAttributePtrByIDCallback 	= 7,
	kGetAttributeByIDCallback 		= 8,
	kSetAttributeByIDCallback 		= 9,
	kWriteCallback					= 10,
	kWriteVCallback					= 11,
	kFlushCallback					= 12,
	kAddServiceCallback				= 13,
	kIDForServiceCallback			= 14,
	kDoServiceCallback				= 15,
	kSendRTSPHeadersCallback		= 16,
	kAppendRTSPHeadersCallback		= 17,
	kSendStandardRTSPCallback		= 18,
	kAddRTPStreamCallback			= 19,
	kPlayCallback					= 20,
	kPauseCallback					= 21,
	kTeardownCallback				= 22,
	kRequestEventCallback			= 23,
	kSetIdleTimerCallback			= 24,
	kOpenFileObjectCallback			= 25,
	kCloseFileObjectCallback		= 26,
	kReadCallback					= 27,
	kSeekCallback					= 28,
	kAdviseCallback					= 29,
	kGetNumValuesCallback			= 30,
	kGetNumAttributesCallback		= 31,
	kSignalStreamCallback			= 32,
	kCreateSocketStreamCallback		= 33,
	kDestroySocketStreamCallback	= 34,
	kAddStaticAttributeCallback		= 35,
	kAddInstanceAttributeCallback	= 36,
	kRemoveInstanceAttributeCallback= 37,
	kGetStaticAttrInfoByNameCallback= 38,
	kGetStaticAttrInfoByIDCallback	= 39,
	kGetStaticAttrInfoByIndexCallback=40,
	kGetAttrInfoByIndexCallback		= 41,
	kGetAttrInfoByNameCallback		= 42,
	kGetAttrInfoByIDCallback		= 43,
	
	kGetValueAsStringCallback		= 44,
	kGetTypeAsStringCallback		= 45,
	kConvertStringToTypeCallback	= 46,
	kGetDataTypeForTypeStringCallback = 47,		
	kRemoveValueCallback			= 48,

	kRequestGlobalLockCallback		= 49, 
	kIsGlobalLockedCallback			= 50, 
	kUnlockGlobalLock				= 51, 

	kLastCallback 					= 52
};

typedef struct {
	// Callback function pointer array
	QTSS_CallbackProcPtr addr [kLastCallback];
} QTSS_Callbacks, *QTSS_CallbacksPtr;

typedef struct
{
	UInt32					inServerAPIVersion;
	QTSS_CallbacksPtr 		inCallbacks;
	QTSS_StreamRef	 		inErrorLogStream;
	UInt32					outStubLibraryVersion;
	QTSS_DispatchFuncPtr	outDispatchFunction;
	
} QTSS_PrivateArgs, *QTSS_PrivateArgsPtr;

typedef struct
{
	QTSSModule*	curModule;	// this structure is setup in each thread
	QTSS_Role	curRole;	// before invoking a module in a role. Sometimes
	Task*		curTask;	// this info. helps callback implementation
	Bool16		eventRequested;
	Bool16		globalLockRequested;	// request event with global lock.
	Bool16		isGlobalLocked;
	SInt64		idleTime;	// If a module has requested idle time.
	
} QTSS_ModuleState, *QTSS_ModuleStatePtr;

QTSS_StreamRef	GetErrorLogStream();


#ifdef __cplusplus
}
#endif

#endif

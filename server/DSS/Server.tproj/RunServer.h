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
	File:		RunServer.h

	Contains:	Routines to run the Streaming Server


*/


#include "OSHeaders.h"
#include "XMLPrefsParser.h"
#include "PrefsSource.h"
#include "QTSS.h"


//
// This function starts the Streaming Server. Pass in a source
// for preferences, a source for text messages, and an optional
// port to override the default.
//
// Returns the server state upon completion of startup. If this
// is qtssFatalErrorState, something went horribly wrong and caller
// should just die.
QTSS_ServerState StartServer(	XMLPrefsParser* inPrefsSource,
								PrefsSource* inMessagesSource,
								UInt16 inPortOverride,
								int statsUpdateInterval);

//
// Call this after StartServer if it doesn't return qtssFatalError.
// This will not return until the server is going away
void RunServer();

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
	File:		QTSSPosixFileSysModule.h

	Contains:	
	
	Written By: Denis Serenyi

	$Log: QTSSPosixFileSysModule.h,v $
	Revision 1.3  2001/06/01 20:52:37  wmaycisco
	Sync to our latest code base.
	
	Revision 1.1.1.1  2001/02/26 23:27:27  dmackie
	Import of Apple Darwin Streaming Server 3 Public Preview
	
	Revision 1.1.1.1  2000/08/31 00:30:24  serenyi
	Mothra Repository
	
	Revision 1.1  2000/05/22 04:37:44  serenyi
	Created
	

*/

#ifndef __QTSSPOSIXFILESYSMODULE_H__
#define __QTSSPOSIXFILESYSMODULE_H__

#include "QTSS.h"
#include "QTSS_Private.h" 	// This module MUST be compiled directly into the server!
							// This is because it uses the server's private internal event
							// mechanism for doing async file I/O

extern "C"
{
	EXPORT QTSS_Error QTSSPosixFileSysModule_Main(void* inPrivateArgs);
}

#endif // __QTSSPOSIXFILESYSMODULE_H__


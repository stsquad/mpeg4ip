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
	File:		QTSSWebStatsModule.h

	Contains:	A module that uses the information available in the server
				to present a web page containing that information. Uses the Filter
				module feature of QTSS API.

	$Log: QTSSAdminModule.h,v $
	Revision 1.3  2001/06/01 20:52:37  wmaycisco
	Sync to our latest code base.
	
	Revision 1.1.1.1  2001/02/26 23:27:26  dmackie
	Import of Apple Darwin Streaming Server 3 Public Preview
	
	Revision 1.1.1.1  2000/08/31 00:30:22  serenyi
	Mothra Repository
	
	Revision 1.1  2000/07/24 23:50:25  murata
	tuff for Ghidra/Rodan
	
	Revision 1.1  2000/06/17 04:20:11  murata
	Add QTSSAdminModule and fix Project Builder,Jam and Posix Make builds.
	Bug #:
	Submitted by:
	Reviewed by:
		
	
*/

#ifndef __QTSSADMINMODULE_H__
#define __QTSSADMINMODULE_H__

#include "QTSS.h"

extern "C"
{
	EXPORT QTSS_Error QTSSAdminModule_Main(void* inPrivateArgs);
}

#endif // __QTSSADMINMODULE_H__


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
	File:		QTSSPosixFileSysModule.h

	Contains:	
	
	Written By: Denis Serenyi

	$Log: QTSSPosixFileSysModule.h,v $
	Revision 1.5  2001/10/11 20:39:05  wmaycisco
	Sync 10/11/2001. 1:40PM
	
	Revision 1.3  2001/10/01 22:08:35  dmackie
	DSS 3.0.1 import
	
	Revision 1.3  2001/03/13 22:24:06  murata
	Replace copyright notice for license 1.0 with license 1.2 and update the copyright year.
	Bug #:
	Submitted by:
	Reviewed by:
	
	Revision 1.2  2000/10/11 07:26:58  serenyi
	import
	
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


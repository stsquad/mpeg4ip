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
	File:		QTSSWebDebugModule.h

	Contains:	A module that uses the debugging information available in the server
				to present a web page containing that information. Uses the Filter
				module feature of the QTSS API.


	

*/

#ifndef __QTSSWEBDEBUGMODULE_H__
#define __QTSSWEBDEBUGMODULE_H__

#include "QTSS.h"

extern "C"
{
	EXPORT QTSS_Error QTSSWebDebugModule_Main(void* inPrivateArgs);
}

#endif //__QTSSWEBDEBUGMODULE_H__

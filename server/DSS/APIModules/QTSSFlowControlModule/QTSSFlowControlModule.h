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
	File:		QTSSFlowControlModule.h

	Contains:	Uses information in RTCP packers to throttle back the server
				when it's pumping out too much data to a given client
	
	
	
	
*/

#ifndef _QTSSFLOWCONTROLMODULE_H_
#define _QTSSFLOWCONTROLMODULE_H_

#include "QTSS.h"

extern "C"
{
	EXPORT QTSS_Error QTSSFlowControlModule_Main(void* inPrivateArgs);
}

#endif //_QTSSFLOWCONTROLMODULE_H_

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
        File:		QTSSHttpFileModule.h

        Contains:	A module for HTTP file transfer of files and for
                    on-the-fly ref movie creation.
                    Uses the Filter module feature of QTSS API.

*/

#ifndef __QTSSHTTPFILEMODULE_H__
#define __QTSSHTTPFILEMODULE_H__

#include "QTSS.h"

enum {
                transferHttpFile			= 1,
                transferRefMovieFolder		= 2,
                transferRefMovieFile		= 3
};
typedef UInt32 TransferType;

extern "C"
{
	QTSS_Error QTSSHttpFileModule_Main(void* inPrivateArgs);
}

#endif // __QTSSHTTPFILEMODULE_H__

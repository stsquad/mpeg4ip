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
#ifndef __tracking_element__
#define __tracking_element__

#if (! __MACOS__)
	#include <unistd.h>
#else
	#include "BogusDefs.h"
#endif

#include <string.h>
#include "OSHeaders.h"
#include "MyAssert.h"

class TrackingElement {

	public:
		TrackingElement( pid_t pid, const char * name )
		{
			mName = new char[ strlen(name) + 1 ];
			
			Assert( mName );
			if( mName )
				strcpy( mName, name );
			
			mPID = pid;
			
		}		
		
		virtual ~TrackingElement() 
		{ 
			if ( mName )  
				delete [] mName;
		}
		
		char 	*mName;
		pid_t	mPID;

};

#endif

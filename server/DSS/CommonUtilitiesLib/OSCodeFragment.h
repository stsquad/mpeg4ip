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
	File:		OSDynamicLoader.h

	Contains:  	OS abstraction for loading code fragments.

	

*/

#ifndef _OS_CODEFRAGMENT_H_
#define _OS_CODEFRAGMENT_H_

#include <stdlib.h>
#include "OSHeaders.h"

#ifdef __MacOSX__
#include <CoreFoundation/CFBundle.h>
#endif

class OSCodeFragment
{
	public:
	
		static void Initialize();
	
		OSCodeFragment(const char* inPath);
		~OSCodeFragment();
		
		Bool16	IsValid() { return (fFragmentP != NULL); }
		void*	GetSymbol(const char* inSymbolName);
		
	private:
	
#ifdef __Win32__
		HMODULE fFragmentP;
#elif __MacOSX__
		CFBundleRef fFragmentP;
#else
		void*	fFragmentP;
#endif
};

#endif//_OS_CODEFRAGMENT_H_

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
	File:		OSCodeFragment.cpp

	Contains:  	Implementation of object defined in OSCodeFragment.h

	
*/

#include <stdlib.h>
#include <stdio.h>
#include "MyAssert.h"

#if __MACOS__
	//CFM Includes here
#elif __Win32__
	// Win32 includes here
#elif __MacOSX__
	#include <CoreFoundation/CFString.h>
	#include <CoreFoundation/CFBundle.h>
#else
#include <dlfcn.h>
#endif

#include "OSCodeFragment.h"

void OSCodeFragment::Initialize()
{
// does nothing...should do any CFM initialization here
}

OSCodeFragment::OSCodeFragment(const char* inPath)
: fFragmentP(NULL)
{
#if defined(HPUX) || defined(HPUX10)
    shl_t handle;
    fFragmentP = shl_load(inPath, BIND_IMMEDIATE|BIND_VERBOSE|BIND_NOSTART, 0L);
#elif defined(OSF1) ||\
    (defined(__FreeBSD_version) && (__FreeBSD_version >= 220000))
    fFragmentP = dlopen((char *)inPath, RTLD_NOW | RTLD_GLOBAL);
#elif defined(__MACOS__)
	// Need to put CFM code here
#elif defined(__FreeBSD__)
    fFragmentP = dlopen(inPath, RTLD_NOW);
#elif defined(__Win32__)
	fFragmentP = ::LoadLibrary(inPath);
#elif defined(__MacOSX__)
	CFStringRef theString = CFStringCreateWithCString( kCFAllocatorDefault, inPath, kCFStringEncodingASCII);

        //
        // In MacOSX, our "fragments" are CF bundles, which are really
        // directories, so our paths are paths to a directory
        CFURLRef	bundleURL = CFURLCreateWithFileSystemPath(	kCFAllocatorDefault,
															theString,
															kCFURLPOSIXPathStyle,
															true);

    //
    // I figure CF is safe about having NULL passed
    // into its functions (if fBundle failed to get created).
    // So, I won't worry about error checking myself
	fFragmentP = CFBundleCreate( kCFAllocatorDefault, bundleURL );
	Boolean success = false;
	if (fFragmentP != NULL)
		success = CFBundleLoadExecutable( fFragmentP );
	if (!success && fFragmentP != NULL)
    {
		CFRelease( fFragmentP );
		fFragmentP = NULL;
    }
    
	CFRelease(bundleURL);
	CFRelease(theString);
    
#else
    fFragmentP = dlopen(inPath, RTLD_NOW | RTLD_GLOBAL);
#endif
}

OSCodeFragment::~OSCodeFragment()
{
	if (fFragmentP == NULL)
		return;
		
#if defined(HPUX) || defined(HPUX10)
    shl_unload((shl_t)fFragmentP);
#elif defined(__MACOS__)
	// Need to put CFM code here
#elif defined(__Win32__)
	BOOL theErr = ::FreeLibrary(fFragmentP);
	Assert(theErr);
#elif defined(__MacOSX__)
    CFBundleUnloadExecutable( fFragmentP );
    CFRelease( fFragmentP );
#else
    dlclose(fFragmentP);
#endif
}

void*	OSCodeFragment::GetSymbol(const char* inSymbolName)
{
	if (fFragmentP == NULL)
		return NULL;
		
#if defined(HPUX) || defined(HPUX10)
    void *symaddr = NULL;
    int status;

    errno = 0;
    status = shl_findsym((shl_t *)&fFragmentP, symname, TYPE_PROCEDURE, &symaddr);
    if (status == -1 && errno == 0) /* try TYPE_DATA instead */
        status = shl_findsym((shl_t *)&fFragmentP, inSymbolName, TYPE_DATA, &symaddr);
    return (status == -1 ? NULL : symaddr);
#elif defined(DLSYM_NEEDS_UNDERSCORE)
    char *symbol = (char*)malloc(sizeof(char)*(strlen(inSymbolName)+2));
    void *retval;
    sprintf(symbol, "_%s", inSymbolName);
    retval = dlsym(fFragmentP, symbol);
    free(symbol);
    return retval;
#elif defined(__MACOS__)
	// Need to put CFM code here
	return NULL;
#elif defined(__Win32__)
	return ::GetProcAddress(fFragmentP, inSymbolName);
#elif defined(__MacOSX__)
    CFStringRef theString = CFStringCreateWithCString( kCFAllocatorDefault, inSymbolName, kCFStringEncodingASCII);
    void* theSymbol = (void*)CFBundleGetFunctionPointerForName( fFragmentP, theString );
    CFRelease(theString);
    return theSymbol;
#else
    return dlsym(fFragmentP, inSymbolName);
#endif	
}

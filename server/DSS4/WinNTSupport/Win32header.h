#define __Win32__ 1
#define _MT 1

/* Defines needed to compile windows headers */

#ifndef _X86_
	#define _X86_ 1
#endif

/* Pro4 compilers should automatically define this to appropriate value */
#ifndef _M_IX86
	#define _M_IX86	500
#endif

#ifndef WIN32
	/* same as definition in OLE2.h where 100 implies WinNT version 1.0 */
	#define WIN32 100
#endif

#ifndef _WIN32
	#define _WIN32 1
#endif

#ifndef _MSC_VER
	/* we implement the feature set of CL 9.0 (MSVC++ 2) */
	#define _MSC_VER 900
#endif

#ifndef _CRTAPI1
	#define _CRTAPI1 __cdecl
#endif

#ifndef _WIN32_WINNT
	#define _WIN32_WINNT 0x0400
#endif

#ifndef _WIN32_IE
	#define _WIN32_IE 0x0400
#endif

#include "PlatformHeader.h"
#include "revision.h"
#include <fcntl.h>
/*
	File:		mymutex.h

	Contains:	xxx put contents here xxx

	Written by:	Greg Vaughan

	Copyright:	© 1999 by Apple Computer, Inc., all rights reserved.

	Change History (most recent first):

		 <2>	10/27/99	GBV		update for beaker

	To Do:
*/

#ifndef _MYMUTEX_H_
#define _MYMUTEX_H_


#ifdef __cplusplus
extern "C" {
#endif

#include <mach/mach.h>
#include <pthread.h>

typedef void* mymutex_t;
mymutex_t mymutex_alloc();
void mymutex_free(mymutex_t);

void mymutex_lock(mymutex_t);
int mymutex_try_lock(mymutex_t);
void mymutex_unlock(mymutex_t);

#ifdef __cplusplus
}
#endif

#endif

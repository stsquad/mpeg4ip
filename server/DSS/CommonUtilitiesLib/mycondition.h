/*
	File:		mycondition.h

	Contains:	xxx put contents here xxx

	Written by:	Greg Vaughan

	Copyright:	© 1999 by Apple Computer, Inc., all rights reserved.

	Change History (most recent first):

		 <2>	10/27/99	GBV		update for beaker

	To Do:
*/

#ifndef _MYCONDITION_H_
#define _MYCONDITION_H_


#include <mach/mach.h>
#include <pthread.h>

#include "mymutex.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void* mycondition_t;
mycondition_t mycondition_alloc();
void mycondition_free(mycondition_t);

void mycondition_broadcast(mycondition_t);
void mycondition_signal(mycondition_t);
void mycondition_wait(mycondition_t, mymutex_t, int); //timeout as a msec offset from now (0 means no timeout)

#ifdef __cplusplus
}
#endif

#endif
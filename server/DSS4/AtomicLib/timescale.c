/*
 * Copyright (c) 1998-1999, Apple Computer, Inc. All rights reserved.
 *
 *	File:	timescale.c
 *
 *	History:
 *		23-Oct-1998	Umesh Vaishampayan (umeshv@apple.com)
 *			Created.
 */

#include "timestamp.h"
//#include <mach_debug/mach_debug.h>
//#include <mach_debug/mach_debug_types.h>
//#include <mach/syscall_sw.h>
#include <stdlib.h>

void
utimescale(struct timescale *tscp)
{
	unsigned int theNanosecondNumerator = 0;
	unsigned int theNanosecondDenominator = 0;

        MKGetTimeBaseInfo(NULL, &theNanosecondNumerator, &theNanosecondDenominator, NULL, NULL);
        tscp->tsc_numerator = theNanosecondNumerator / 1000; /* PPC magic number */
        tscp->tsc_denominator = theNanosecondDenominator;
        return;
}


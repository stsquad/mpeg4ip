/*
 * Copyright (c) 1998-1999, Apple Computer, Inc. All rights reserved.
 *
 *	History:
 *      08-Feb-1999 Umesh Vaishampayan (umeshv@apple.com)
 *			Added scaledtimestamp().
 *
 *		26-Oct-1998 Umesh Vaishampayan (umeshv@apple.com)
 *			Made the header c++ friendly.
 *
 *		23-Oct-1998	Umesh Vaishampayan (umeshv@apple.com)
 *			Created.
 */

#ifndef _TIMESTAMP_H_
#define _TIMESTAMP_H_

#ifdef __cplusplus
extern "C" {
#endif

/* Get a 64 bit timestamp */
extern long long timestamp(void);

struct timescale {
    long long tsc_numerator;
    long long tsc_denominator;
};

/*
 * Get numerator and denominator to convert value returned 
 * by timestamp() to microseconds
 */
extern void utimescale(struct timescale *tscp);

extern long long scaledtimestamp(double scale);

#ifdef __cplusplus
}
#endif

#endif /* _TIMESTAMP_H_ */

/*
 * Copyright (c) 1998-1999, Apple Computer, Inc. All rights reserved.
 *
 *	History:
 *		11-Feb-1999 Umesh Vaishampayan (umeshv@apple.com)
 *			Added atomic_or().
 *
 *		26-Oct-1998 Umesh Vaishampayan (umeshv@apple.com)
 *			Made the header c++ friendly.
 *
 *		12-Oct-1998	Umesh Vaishampayan (umeshv@apple.com)
 *			Changed simple_ to spin_ so as to coexist with cthreads till
 *			the merge to the system framework.
 *
 *		8-Oct-1998	Umesh Vaishampayan (umeshv@apple.com)
 *			Created from the kernel code to be in a dynamic shared library.
 *			Kernel code created by: Bill Angell	(angell@apple.com)
 */

#ifndef _ATOMIC_H_
#define _ATOMIC_H_

#ifdef __cplusplus
extern "C" {
#endif

/* Locking routines */

struct spin_lock {			/* sizeof cache line */
	unsigned int lock_data;
	unsigned int pad[7];
};

typedef struct spin_lock *spin_lock_t;

extern void spin_lock_init(spin_lock_t);

extern void spin_lock_unlock(spin_lock_t);

extern unsigned int spin_lock_lock(spin_lock_t);

extern unsigned int spin_lock_bit(spin_lock_t, unsigned int bits);

extern unsigned int spin_unlock_bit(spin_lock_t, unsigned int bits);

extern unsigned int spin_lock_try(spin_lock_t);

extern unsigned int spin_lock_held(spin_lock_t);

/* Other atomic routines */

extern unsigned int compare_and_store(unsigned int oval,
								unsigned int nval, unsigned int *area);

extern unsigned int atomic_add(unsigned int *area, int val);

extern unsigned int atomic_or(unsigned int *area, unsigned int mask);

extern unsigned int atomic_sub(unsigned int *area, int val);

extern void queue_atomic(unsigned int *anchor,
					unsigned int *elem, unsigned int disp);

extern void queue_atomic_list(unsigned int *anchor,
						unsigned int *first, unsigned int *last,
						unsigned int disp);

extern unsigned int *dequeue_atomic(unsigned int *anchor, unsigned int disp);

#ifdef __cplusplus
}
#endif

#endif /* _ATOMIC_H_ */

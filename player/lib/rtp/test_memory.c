/*
 * FILE:    test_memory.c
 * AUTHORS: Orion Hodson
 * 
 * Copyright (c) 1999-2000 University College London
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, is permitted provided that the following conditions 
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by the Computer Science
 *      Department at University College London
 * 4. Neither the name of the University nor of the Department may be used
 *    to endorse or promote products derived from this software without
 *    specific prior written permission.
 * THIS SOFTWARE IS PROVIDED BY THE AUTHORS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESSED OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include "config_unix.h"
#include "config_win32.h"
#include "debug.h"
#include "drand48.h"
#include "memory.h"
#include "util.h"

#include "test_memory.h"

#ifdef DEBUG_MEM

static size_t sz[] = {17, 32, 64, 81, 1024, 4096};

static void fill_block(char *y, uint32_t ylen)
{
        uint32_t i, xlen;
        double *x;

        xlen = ylen / sizeof(double);
        x    = (double*)y;

        for(i = 0; i < xlen; i++) {
                x[i] = 0;
        }
}

static void 
do_test()
{
        static char     *b[37];
        static uint32_t  s[37];
        uint32_t         i, idx, nslots, nszs, r;
        uint32_t         allocations;

        memset(b, 0, sizeof(b));
        memset(s, 0, sizeof(s));

        nszs = sizeof(sz)/sizeof(sz[0]);
        nslots  = sizeof(b)/sizeof(b[0]);

        /* test xmalloc */
        for (allocations = 10; allocations < 1000000; allocations *= 10) {
                for (i = 0; i < allocations; i++) {
                        idx = drand48();
                        idx = idx % nslots;
                        if (b[idx] != NULL) {
                                xfree(b[idx]);
                        }
                        r = drand48();
                        b[idx] = (void*)xmalloc(sz[r % nszs]);
                        fill_block(b[idx], sz[r % nszs]);
                }

                for(idx = 0; idx < nslots; idx++) {
                        xmemchk();
                        if (b[idx] != NULL) {
                                xfree(b[idx]);
                                b[idx] = NULL;
                        }
                        xmemchk();
                }
        }

        /* test block alloc */
        for (allocations = 10; allocations < 1000000; allocations *= 10) {
                for (i = 0; i < allocations; i++) {
                        idx = drand48();
                        idx = idx % nslots;
                        if (b[idx] != NULL) {
                                block_free(b[idx], s[idx]);
                        }
                        r = drand48();
                        s[idx] = sz[r % nszs];
                        b[idx] = (void*)block_alloc(s[idx]);
                        fill_block(b[idx], s[idx]);
                }

                for(idx = 0; idx < nslots; idx++) {
                        xmemchk();
                        if (b[idx] != NULL) {
                                block_free(b[idx], s[idx]);
                                b[idx] = NULL;
                        }
                        xmemchk();
                }
        }
}


void test_memory(void)
{
        printf("Memory................................. "); fflush(stdout);
        do_test();
        printf("pass\n");
}
#else
void test_memory(void)
{
        return;
}
#endif /* DEBUG_MEM */

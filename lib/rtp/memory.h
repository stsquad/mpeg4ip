/*
 * FILE:    memory.h
 * PROGRAM: RAT
 * AUTHOR:  Isidor Kouvelas + Colin Perkins + Orion Hodson
 *
 * $Revision: 1.2 $
 * $Date: 2001/10/11 20:39:03 $
 *
 * Copyright (c) 1995-2000 University College London
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

#ifndef _RAT_MEMORY_H
#define _RAT_MEMORY_H

#define xmalloc(x)	_xmalloc(x,__FILE__,__LINE__)
#define xrealloc(p,x)	_xrealloc(p, x,__FILE__,__LINE__)
#define xstrdup(str)	_xstrdup(str,__FILE__,__LINE__)

#if defined(__cplusplus)
extern "C" {
#endif

/* Debug Functions */
void 	 xdoneinit(void);
void	 xmemchk(void);
void     xmemdmp(void);
void     xclaim(void *addr, const char *filen, int line);
void     xmemdist(FILE *fp);

/* Replacements for regular memory fn's */
void	 xfree(void *p);
void	*_xmalloc(unsigned size,const char *filen,int line);
void	*_xrealloc(void *p,unsigned size,const char *filen,int line);
char	*_xstrdup(const char *s1, const char *filen, int line);
void	*_block_alloc(unsigned size, const char *filen, int line);
void	 _block_free(void *p, int size, int line);

#if defined(__cplusplus)
}
#endif

#endif 

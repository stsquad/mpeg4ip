/*
 * FILE:    util.h
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

#ifndef _UTIL_H
#define _UTIL_H

#if defined(__cplusplus)
extern "C" {
#endif

#define block_alloc(x)	_block_alloc(x,__FILE__,__LINE__)
#define block_free(x,y) _block_free(x,y,__LINE__)

void	*_block_alloc(unsigned size, const char *filen, int line);
void	 _block_free(void *p, int size, int line);
void	 block_release_all(void);
void     block_trash_check(void);
void     block_check(char *p);

/* purge_chars: removes occurances of chars in to_go from src */
void purge_chars(char *src, char *to_go); 

/* overlapping_words: returns how many words match in two strings */
int overlapping_words(const char *s1, const char *s2, int max_words);

/* The strfind() function is mainly for internal use, but might be
   useful to others... */
int strfind(const char *haystack, 
	    const char *needle_start, 
	    const char *needle_end);

#if defined(__cplusplus)
}
#endif

#endif 

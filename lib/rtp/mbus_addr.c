/*
 * FILE:     mbus_addr.c
 * AUTHOR:   Colin Perkins
 * 
 * Copyright (c) 2000 University College London
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
#include "util.h"
#include "mbus_addr.h"

int mbus_addr_match(const char *a, const char *b)
{
	/* Compare the addresses "a" and "b". These may optionally be */
	/* surrounded by "(" and ")" and may have an arbitrary amount */
	/* of white space between components of the addresses. There  */
	/* is a match if every word of address b is in address a.     */
	const char	*y = NULL; 

	ASSERT(a != NULL);
	ASSERT(b != NULL);

	/* Skip leading whitespace and '('... */
	while (isspace((unsigned char)*a) || (*a == '(')) a++;
	while (isspace((unsigned char)*b) || (*b == '(')) b++;

	while ((*b != '\0') && (*b != ')')) {
		/* Move b along through the string to the start of the next */
		/* word. Move y along from that to the end of the word.     */
		while (isspace((unsigned char)*b)) b++;
		for (y = b; ((*y != ' ') && (*y != ')') && (*y != '\0')); y++) {
			/* do nothing */
		}
		y--;
		/* Check if the word between b and y is contained in the    */
		/* string pointed to be a.                                  */
		if (!strfind(a, b, y)) {
			return FALSE;
		}
		b = ++y;
	}		
	return TRUE;
}

int mbus_addr_identical(const char *a, const char *b)
{
	/* A more restrictive version of mbus_addr_match. Returns TRUE  */
	/* iff the addresses are identical (except, possibly, for order */
	/* of the elements.                                             */
	return mbus_addr_match(a, b) && mbus_addr_match(b, a);
}


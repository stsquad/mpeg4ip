/*
 * FILE:    test_mbus_addr.c
 * AUTHORS: Colin Perkins
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
#include "mbus_addr.h"
#include "test_mbus_addr.h"

int strfind(const char *haystack, const char *needle_start, const char *needle_end);

void test_mbus_addr(void)
{
	const char	haystack[] = "The quick brown fox jumped over the lazy dog";
	const char	needle_1[] = "fox";
	const char	needle_2[] = "fat fox jumping";
	const char	needle_3[] = "The";
	const char	needle_4[] = "dog";
	const char	a1[] = "()";
	const char	a2[] = "( one  two three)";
	const char	a3[] = " ( four five six)";
	const char	a4[] = "six four";
	const char	a5[] = " three one two";
	const char	a6[] = "";

	printf("Mbus addr (strfind).................... "); fflush(stdout);
	if (!strfind(haystack, needle_1, needle_1 + strlen(needle_1) - 1)) {
		printf("fail (1)\n");
		goto match;
	}
	if (strfind(haystack, needle_2, needle_2 + strlen(needle_2) - 1)) {
		printf("fail (2)\n");
		goto match;
	}
	if (!strfind(haystack, needle_3, needle_3 + strlen(needle_1) - 1)) {
		printf("fail (3)\n");
		goto match;
	}
	if (!strfind(haystack, needle_4, needle_4 + strlen(needle_1) - 1)) {
		printf("fail (4)\n");
		goto match;
	}
	if (!strfind(haystack, needle_2 + 4, needle_2 + 7)) {
		printf("fail (5)\n");
		goto match;
	}
	printf("pass\n");

match:
	printf("Mbus addr (match)...................... "); fflush(stdout);
	if (!mbus_addr_match(a2, a1)) {
		printf("fail (1)\n");
		goto identical;
	}
	if (mbus_addr_match(a2, a3)) {
		printf("fail (2)\n");
		goto identical;
	}
	if (!mbus_addr_match(a3, a4)) {
		printf("fail (3)\n");
		goto identical;
	}
	if (!mbus_addr_match(a3, a6)) {
		printf("fail (4)\n");
		goto identical;
	}
	if (mbus_addr_match(a6, a3)) {
		printf("fail (5)\n");
		goto identical;
	}
	printf("pass\n");

identical:
	printf("Mbus addr (identical).................. "); fflush(stdout);
	if (!mbus_addr_identical(a2, a5)) {
		printf("fail (1)\n");
		return;
	}
	if (mbus_addr_identical(a4, a3)) {
		printf("fail (2)\n");
		return;
	}
	printf("pass\n");
}


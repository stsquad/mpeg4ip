/*
 * FILE:    test_base64.c
 * AUTHORS: Colin Perkins
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
#include "base64.h"
#include "test_base64.h"

void test_base64(void)
{
	/* The string "Hello, world" should encode as "SGVsbG8sIHdvcmxk" */
	const char	*input = "Hello, world";
	char	 output[100];
	char	 decode[100];
	int	 i;

	for (i = 0; i < 100; i++) {
		output[i] = '\0';
	}

	printf("Base64 encode.......................... "); fflush(stdout);
	i = base64encode(input, strlen(input), output, 100);
	if ((i != 16) || (strncmp(output, "SGVsbG8sIHdvcmxk", i) != 0)) {
		printf("fail\n");
		return;
	}
	printf("pass\n");

	printf("Base64 decode.......................... "); fflush(stdout);
	i = base64decode(output, i, decode, 100);
	if ((i != 12) || (strncmp(decode, "Hello, world", i) != 0)) {
		printf("fail\n");
		return;
	}
	printf("pass\n");
}


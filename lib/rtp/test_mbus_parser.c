/*
 * FILE:    test_mbus_parser.c
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
#include "memory.h"
#include "mbus_parser.h"
#include "test_mbus_parser.h"

static void test_encdec(void)
{
	char		*res;
	const char 	*str1d = "Hello, world!";
	const char 	*str1e = "\"Hello,\\ world!\"";
	const char 	*str2d = "Test \"quoted\" text";
	const char 	*str2e = "\"Test\\ \\\"quoted\\\"\\ text\"";

	printf("Mbus parser (string encode/decode)..... "); fflush(stdout);
	res = mbus_encode_str(str1d);
	if (strcmp(res, str1e) != 0) {
		printf("fail (%s != %s)\n", res, str1e);
		goto fail;
	}
	xfree(res);
	
	res = mbus_encode_str(str2d);
	if (strcmp(res, str2e) != 0) {
		printf("fail (%s != %s)\n", res, str2e);
		goto fail;
	}

	res = xstrdup(str1e);
	mbus_decode_str(res);
	if (strcmp(res, str1d) != 0) {
		printf("fail (%s != %s)\n", res, str1d);
		goto fail;
	}
	xfree(res);

	res = xstrdup(str2e);
	mbus_decode_str(res);
	if (strcmp(res, str2d) != 0) {
		printf("fail (%s != %s)\n", res, str2d);
		goto fail;
	}

	printf("pass\n");
fail:
	xfree(res);
}

static void test_parsing(void)
{
	struct mbus_parser	*p;
	char			*str1 = xstrdup("1234 567.89  Symbol \"String\"  (a b c) ()");
	int			 i;
	double			 d;
	char			*s;

	printf("Mbus parser (parsing).................. "); fflush(stdout);
	p = mbus_parse_init(str1);
	if (!mbus_parse_int(p, &i)) {
		printf("fail (1)\n");
		goto done;
	}
	if (i != 1234) {
		printf("fail (2)\n");
		goto done;
	}

	if (!mbus_parse_flt(p, &d)) {
		printf("fail (3)\n");
		goto done;
	}
	if (d != 567.89) {
		printf("fail (4)\n");
		goto done;
	}

	if (!mbus_parse_sym(p, &s)) {
		printf("fail (5)\n");
		goto done;
	}
	if (strcmp(s, "Symbol") != 0) {
		printf("fail (6)\n");
		goto done;
	}

	if (!mbus_parse_str(p, &s)) {
		printf("fail (7)\n");
		goto done;
	}
	if (strcmp(s, "\"String\"") != 0) {
		printf("fail (8)\n");
		goto done;
	}

	if (!mbus_parse_lst(p, &s)) {
		printf("fail (9)\n");
		goto done;
	}
	if (strcmp(s, "a b c") != 0) {
		printf("fail (10)\n");
		goto done;
	}

	if (!mbus_parse_lst(p, &s)) {
		printf("fail (11)\n");
		goto done;
	}
	if (strcmp(s, "") != 0) {
		printf("fail (12)\n");
		goto done;
	}

	printf("pass\n");
done:
	mbus_parse_done(p);
}

void test_mbus_parser(void)
{
	test_encdec();
	test_parsing();
}


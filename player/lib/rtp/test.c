/*
 * FILE:    test.c
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
#include "version.h"
#include "test_base64.h"
#include "test_des.h"
#include "test_md5.h"
#include "test_memory.h"
#include "test_net_udp.h"
#include "test_mbus_parser.h"
#include "test_mbus_addr.h"

#ifdef WIN32
#define WS_VERSION_ONE MAKEWORD(1,1)
#define WS_VERSION_TWO MAKEWORD(2,2)
#endif

int main(int argc, char *argv[])
{
#ifdef WIN32
	WSADATA WSAdata;
	if (WSAStartup(WS_VERSION_TWO, &WSAdata) != 0 && WSAStartup(WS_VERSION_ONE, &WSAdata) != 0) {
    		printf("Windows Socket initialization failed.\n");
		return 1;
	}
	debug_msg("WSAStartup OK: %sz\nStatus:%s\n", WSAdata.szDescription, WSAdata.szSystemStatus);
#endif

	UNUSED(argc);
	UNUSED(argv);
	printf("Testing common multimedia library %s\n", CCL_VERSION);
	test_base64();
	test_des();
	test_md5();
        test_memory();
	test_net_udp();
	test_mbus_parser();
	test_mbus_addr();
#ifdef WIN32
	Sleep(2000);
	WSACleanup();
#endif
	return 0;
}


/*
 * FILE:    mbus_config.h
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

#ifndef _MBUS_CONFIG
#define _MBUS_CONFIG

struct mbus_key {
	char	*algorithm;
	char	*key;
	int	 key_len;
};

struct mbus_config;

#define MBUS_CONFIG_VERSION	 1

#define SCOPE_HOSTLOCAL       	 0
#define SCOPE_HOSTLOCAL_NAME 	"HOSTLOCAL"
#define SCOPE_LINKLOCAL       	 1
#define SCOPE_LINKLOCAL_NAME 	"LINKLOCAL"

#define MBUS_DEFAULT_NET_ADDR 	"224.255.222.239"
#define MBUS_DEFAULT_NET_PORT 	 47000
#define MBUS_DEFAULT_SCOPE    	 SCOPE_HOSTLOCAL
#define MBUS_DEFAULT_SCOPE_NAME	 SCOPE_HOSTLOCAL_NAME

#if defined(__cplusplus)
extern "C" {
#endif

char *mbus_new_encrkey(void);
char *mbus_new_hashkey(void);
void mbus_lock_config_file(struct mbus_config *m);
void mbus_unlock_config_file(struct mbus_config *m);
void mbus_get_encrkey(struct mbus_config *m, struct mbus_key *key);
void mbus_get_hashkey(struct mbus_config *m, struct mbus_key *key);
void mbus_get_net_addr(struct mbus_config *m, char *net_addr, uint16_t *net_port, int *net_scope);
int  mbus_get_version(struct mbus_config *m);
struct mbus_config *mbus_create_config(void);

#if defined(__cplusplus)
}
#endif

#endif

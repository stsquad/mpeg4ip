/*
 * FILE:     mbus_config.c
 * AUTHOR:   Colin Perkins
 * MODIFIED: Orion Hodson
 *           Markus Germeier
 * 
 * Copyright (c) 1999-2001 University College London
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
#include "crypt_random.h"
#include "base64.h"
#include "mbus.h"
#include "mbus_config.h"

#define MBUS_ENCRYPT_BY_DEFAULT
#define MBUS_ENCRKEY_LEN      8
#define MBUS_HASHKEY_LEN     12
#define MBUS_BUF_SIZE	   1500
#define MBUS_FILE_NAME   ".mbus"
#define MBUS_FILE_NAME_LEN    5

struct mbus_config {
#ifdef WIN32
	HKEY		  cfgKey;
#else
	fd_t		  cfgfd;
#endif
	int		  cfg_locked;
};

char *mbus_new_encrkey(void)
{
	char		*key;	/* The key we are going to return... */
#ifdef MBUS_ENCRYPT_BY_DEFAULT
	/* Create a new key, for use by the hashing routines. Returns */
	/* a key of the form (DES,MTIzMTU2MTg5MTEyMQ==)               */
	char		 random_string[MBUS_ENCRKEY_LEN];
	char		 encoded_string[(MBUS_ENCRKEY_LEN*4/3)+4];
	int		 encoded_length;
	int		 i, j, k;

	/* Step 1: generate a random string for the key... */
	for (i = 0; i < MBUS_ENCRKEY_LEN; i++) {
		random_string[i] = ((int32_t)lbl_random() | 0x000ff000) >> 24;
	}

	/* Step 2: fill in parity bits to make DES library happy */
	for (i = 0; i < MBUS_ENCRKEY_LEN; ++i) {
	        k = random_string[i] & 0xfe;
		j = k;
		j ^= j >> 4;
		j ^= j >> 2;
		j ^= j >> 1;
		j = (j & 1) ^ 1;
		random_string[i] = k | j;
	}

	/* Step 3: base64 encode that string... */
	memset(encoded_string, 0, (MBUS_ENCRKEY_LEN*4/3)+4);
	encoded_length = base64encode(random_string, MBUS_ENCRKEY_LEN, encoded_string, (MBUS_ENCRKEY_LEN*4/3)+4);

	/* Step 4: put it all together to produce the key... */
	key = (char *) xmalloc(encoded_length + 18);
	sprintf(key, "(DES,%s)", encoded_string);
#else
	key = (char *) xmalloc(9);
	sprintf(key, "(NOENCR)");
#endif
	return key;
}

char *mbus_new_hashkey(void)
{
	/* Create a new key, for use by the hashing routines. Returns  */
	/* a key of the form (HMAC-MD5-96,MTIzMTU2MTg5MTEyMQ==)           */
	char		 random_string[MBUS_HASHKEY_LEN];
	char		 encoded_string[(MBUS_HASHKEY_LEN*4/3)+4];
	int		 encoded_length;
	int		 i;
	char		*key;

	/* Step 1: generate a random string for the key... */
	for (i = 0; i < MBUS_HASHKEY_LEN; i++) {
		random_string[i] = ((int32_t)lbl_random() | 0x000ff000) >> 24;
	}
	/* Step 2: base64 encode that string... */
	memset(encoded_string, 0, (MBUS_HASHKEY_LEN*4/3)+4);
	encoded_length = base64encode(random_string, MBUS_HASHKEY_LEN, encoded_string, (MBUS_HASHKEY_LEN*4/3)+4);

	/* Step 3: put it all together to produce the key... */
	key = (char *) xmalloc(encoded_length + 26);
	sprintf(key, "(HMAC-MD5-96,%s)", encoded_string);

	return key;
}

static void rewrite_config(struct mbus_config *m)
{
#ifdef WIN32
	char		*hashkey = mbus_new_hashkey();
	char		*encrkey = mbus_new_encrkey();
	char		*scope   = MBUS_DEFAULT_SCOPE_NAME;
	const uint32_t	 cver    = MBUS_CONFIG_VERSION;
	const uint32_t	 port    = MBUS_DEFAULT_NET_PORT;
	char		*addr    = xstrdup(MBUS_DEFAULT_NET_ADDR);
	char		 buffer[MBUS_BUF_SIZE];
	LONG		 status;

	status = RegSetValueEx(m->cfgKey, "CONFIG_VERSION", 0, REG_DWORD, (const uint8_t *) &cver, sizeof(cver));
	if (status != ERROR_SUCCESS) {
		FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, status, 0, buffer, MBUS_BUF_SIZE, NULL);
		debug_msg("Unable to set version: %s\n", buffer);
		abort();
	}
	status = RegSetValueEx(m->cfgKey, "HASHKEY", 0, REG_SZ, hashkey, strlen(hashkey) + 1);
	if (status != ERROR_SUCCESS) {
		FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, status, 0, buffer, MBUS_BUF_SIZE, NULL);
		debug_msg("Unable to set hashkey: %s\n", buffer);
		abort();
	}	
	status = RegSetValueEx(m->cfgKey, "ENCRYPTIONKEY", 0, REG_SZ, encrkey, strlen(encrkey) + 1);
	if (status != ERROR_SUCCESS) {
		FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, status, 0, buffer, MBUS_BUF_SIZE, NULL);
		debug_msg("Unable to set encrkey: %s\n", buffer);
		abort();
	}	
	status = RegSetValueEx(m->cfgKey, "SCOPE", 0, REG_SZ, scope, strlen(scope) + 1);
	if (status != ERROR_SUCCESS) {
		FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, status, 0, buffer, MBUS_BUF_SIZE, NULL);
		debug_msg("Unable to set scope: %s\n", buffer);
		abort();
	}	
	status = RegSetValueEx(m->cfgKey, "ADDRESS", 0, REG_SZ, addr, strlen(addr) + 1);
	if (status != ERROR_SUCCESS) {
		FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, status, 0, buffer, MBUS_BUF_SIZE, NULL);
		debug_msg("Unable to set address: %s\n", buffer);
		abort();
	}	
	status = RegSetValueEx(m->cfgKey, "PORT", 0, REG_DWORD, (const uint8_t *) &port, sizeof(port));
	if (status != ERROR_SUCCESS) {
		FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, status, 0, buffer, MBUS_BUF_SIZE, NULL);
		debug_msg("Unable to set port: %s\n", buffer);
		abort();
	}	
	xfree(addr);
#else
	char	*hashkey = mbus_new_hashkey();
	char	*encrkey = mbus_new_encrkey();
	char	 buf[1024];

	if (lseek(m->cfgfd, 0, SEEK_SET) == -1) {
		perror("Can't seek to start of config file");
		abort();
	}
	sprintf(buf, "[MBUS]\nCONFIG_VERSION=%d\nHASHKEY=%s\nENCRYPTIONKEY=%s\nSCOPE=%s\nADDRESS=%s\nPORT=%d\n", 
		MBUS_CONFIG_VERSION, hashkey, encrkey, MBUS_DEFAULT_SCOPE_NAME, MBUS_DEFAULT_NET_ADDR, MBUS_DEFAULT_NET_PORT);
	write(m->cfgfd, buf, strlen(buf));
	free(hashkey);
	xfree(encrkey);
#endif
}

void mbus_lock_config_file(struct mbus_config *m)
{
#ifdef WIN32
	/* Open the registry and create the mbus entries if they don't exist   */
	/* already. The default contents of the registry are random encryption */
	/* and authentication keys, and node local scope.                      */
	HKEY			key    = HKEY_CURRENT_USER;
	LPCTSTR			subKey = "Software\\Mbone Applications\\mbus";
	DWORD			disp;
	char			buffer[MBUS_BUF_SIZE];
	LONG			status;

	status = RegCreateKeyEx(key, subKey, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &(m->cfgKey), &disp);
	if (status != ERROR_SUCCESS) {
		FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, status, 0, buffer, MBUS_BUF_SIZE, NULL);
		debug_msg("Unable to open registry: %s\n", buffer);
		abort();
	}
	if (disp == REG_CREATED_NEW_KEY) {
		rewrite_config(m);
		debug_msg("Created new registry entry...\n");
	} else {
		debug_msg("Opened existing registry entry...\n");
	}
#else
	/* Obtain a valid lock on the mbus configuration file. This function */
	/* creates the file, if one does not exist. The default contents of  */
	/* this file are random authentication and encryption keys, and node */
	/* local scope.                                                      */
	struct flock	 l;
	struct stat	 s;
	char		*buf;
	char		*cfg_file;
	char            *cfg_loc;
	int              cfg_loc_len;

	cfg_loc = getenv("MBUS");
	if (cfg_loc == NULL) {
                cfg_loc = getenv("HOME");
                if (cfg_loc == NULL) {
                        /* The getpwuid() stuff is to determine the users    */
                        /* home directory, into which we write a .mbus       */
                        /* config file. The struct returned by getpwuid() is */
                        /* statically allocated, so it's not necessary to    */
                        /* free it afterwards.                               */
                        struct passwd	*p;	
                        p = getpwuid(getuid());
                        if (p == NULL) {
                                perror("Unable to get passwd entry");
                                abort();	      
                        }
                        cfg_loc = p->pw_dir;
                }
	}

	/* Check if config_loc is terminated by mbus config file name. If    */
        /* it's not add it.  This is allows environment variable MBUS to     */
        /* point to config file of directory of config file.                 */
        cfg_loc_len = strlen(cfg_loc);
	if (cfg_loc_len < MBUS_FILE_NAME_LEN ||
	    strcmp(cfg_loc + cfg_loc_len - MBUS_FILE_NAME_LEN, MBUS_FILE_NAME)){
                /* File location does not include config file name.          */
                cfg_file = (char*)xmalloc(cfg_loc_len + MBUS_FILE_NAME_LEN + 2);
                sprintf(cfg_file, "%s/%s", cfg_loc, MBUS_FILE_NAME);	
	} else {
                cfg_file = xstrdup(cfg_loc);
	}
        
	m->cfgfd = open(cfg_file, O_RDWR | O_CREAT, 0600);
	if (m->cfgfd == -1) {
		perror("Unable to open mbus configuration file");
		abort();
	}

	/* We attempt to get a lock on the config file, blocking until  */
	/* the lock can be obtained. The only time this should block is */
	/* when another instance of this code has a write lock on the   */
	/* file, because the contents are being updated.                */
	l.l_type   = F_WRLCK;
	l.l_start  = 0;
	l.l_whence = SEEK_SET;
	l.l_len    = 0;
	if (fcntl(m->cfgfd, F_SETLKW, &l) == -1) {
		perror("Unable to lock mbus configuration file");
		printf("The most likely reason for this error is that %s\n", cfg_file);
		printf("is on an NFS filestore, and you have not correctly setup file locking. \n");
		printf("Ask your system administrator to ensure that rpc.lockd and/or rpc.statd\n");
		printf("are running. \n");
		abort();
	}

	xfree(cfg_file);

	if (fstat(m->cfgfd, &s) != 0) {
		perror("Unable to stat config file\n");
		abort();
	}
	if (s.st_size == 0) {
		/* Empty file, create with sensible defaults... */
		rewrite_config(m);
	} else {
		/* Read in the contents of the config file... */
		buf = (char *) xmalloc(s.st_size+1);
		memset(buf, '\0', s.st_size+1);
		if (read(m->cfgfd, buf, s.st_size) != s.st_size) {
			perror("Unable to read config file\n");
			abort();
		}
		/* Check that the file contains sensible information...   */
		/* This is rather a pathetic check, but it'll do for now! */
		if (strncmp(buf, "[MBUS]", 6) != 0) {
			debug_msg("Mbus config file has incorrect header\n");
			abort();
		}
		xfree(buf);
	}
#endif
	m->cfg_locked = TRUE;
	if (mbus_get_version(m) < MBUS_CONFIG_VERSION) {
		rewrite_config(m);
		debug_msg("Updated Mbus configuration database.\n");
	}
	if (mbus_get_version(m) > MBUS_CONFIG_VERSION) {
		debug_msg("Mbus configuration database has later version number than expected.\n");
		debug_msg("Continuing, in the hope that the extensions are backwards compatible.\n");
	}
}

void mbus_unlock_config_file(struct mbus_config *m)
{
#ifdef WIN32
	LONG status;
	char buffer[MBUS_BUF_SIZE];
	
	status = RegCloseKey(m->cfgKey);
	if (status != ERROR_SUCCESS) {
		FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, status, 0, buffer, MBUS_BUF_SIZE, NULL);
		debug_msg("Unable to close registry: %s\n", buffer);
		abort();
	}
	debug_msg("Closed registry entry...\n");
#else
	struct flock	l;

	l.l_type   = F_UNLCK;
	l.l_start  = 0;
	l.l_whence = SEEK_SET;
	l.l_len    = 0;
	if (fcntl(m->cfgfd, F_SETLKW, &l) == -1) {
		perror("Unable to unlock mbus configuration file");
		abort();
	}
	close(m->cfgfd);
	m->cfgfd = -1;
#endif
	m->cfg_locked = FALSE;
}

#ifndef WIN32
static void mbus_get_key(struct mbus_config *m, struct mbus_key *key, const char *id)
{
	struct stat	 s;
	char		*buf;
	char		*line;
	char		*tmp;
	int		 pos;
        int              linepos;

	assert(m->cfg_locked);

	if (lseek(m->cfgfd, 0, SEEK_SET) == -1) {
		perror("Can't seek to start of config file");
		abort();
	}
	if (fstat(m->cfgfd, &s) != 0) {
		perror("Unable to stat config file\n");
		abort();
	}
	/* Read in the contents of the config file... */
	buf = (char *) xmalloc(s.st_size+1);
	memset(buf, '\0', s.st_size+1);
	if (read(m->cfgfd, buf, s.st_size) != s.st_size) {
		perror("Unable to read config file\n");
		abort();
	}
	
	line = (char *) xmalloc(s.st_size+1);
	sscanf(buf, "%s", line);
	if (strcmp(line, "[MBUS]") != 0) {
		debug_msg("Invalid .mbus file\n");
		abort();
	}
	pos = strlen(line) + 1;
	while (pos < s.st_size) {
                /* get whole line and filter out spaces */
                /* this is good enough for getting keys */
                linepos=0;
                do {
                    while((buf[pos+linepos]==' ')||(buf[pos+linepos]=='\n')
                                                 ||(buf[pos+linepos]=='\t'))
                        pos++;
                    sscanf(buf+pos+linepos, "%s", line+linepos);                
                    linepos = strlen(line);
                } while((buf[pos+linepos]!='\n') && (s.st_size>(pos+linepos+1)));
                pos += linepos + 1;
		if (strncmp(line, id, strlen(id)) == 0) {
			key->algorithm   = (char *) strdup(strtok(line+strlen(id), ",)"));
			if (strcmp(key->algorithm, "NOENCR") != 0) {
				key->key     = (char *) strtok(NULL  , ")");
                                assert(key->key!=NULL);
				key->key_len = strlen(key->key);
				tmp = (char *) xmalloc(key->key_len);
				key->key_len = base64decode(key->key, key->key_len, tmp, key->key_len);
				key->key = tmp;
			} else {
				key->key     = NULL;
				key->key_len = 0;
			}
			xfree(buf);
			xfree(line);
			return;
		}
	}
	debug_msg("Unable to read hashkey from config file\n");
	xfree(buf);
	xfree(line);
}
#endif

void mbus_get_encrkey(struct mbus_config *m, struct mbus_key *key)
{
	/* This MUST be called while the config file is locked! */
	int		 i, j, k;
#ifdef WIN32
	long		 status;
	DWORD		 type;
	char		*buffer;
	int	 	 buflen = MBUS_BUF_SIZE;
	char		*tmp;

	assert(m->cfg_locked);
	
	/* Read the key from the registry... */
	buffer = (char *) xmalloc(MBUS_BUF_SIZE);
	status = RegQueryValueEx(m->cfgKey, "ENCRYPTIONKEY", 0, &type, buffer, &buflen);
	if (status != ERROR_SUCCESS) {
		FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, status, 0, buffer, MBUS_BUF_SIZE, NULL);
		debug_msg("Unable to get encrkey: %s\n", buffer);
		abort();
	}
	assert(type == REG_SZ);
	assert(buflen > 0);

	/* Parse the key... */
	key->algorithm   = strdup(strtok(buffer+1, ",)"));
	if (strcmp(key->algorithm, "NOENCR") != 0) {
		key->key     = (char *) strtok(NULL  , ")");
		key->key_len = strlen(key->key);
		tmp = (char *) xmalloc(key->key_len);
		key->key_len = base64decode(key->key, key->key_len, tmp, key->key_len);
		key->key = tmp;
	} else {
		key->key     = NULL;
		key->key_len = 0;
	}
	xfree(buffer);
#else
	mbus_get_key(m, key, "ENCRYPTIONKEY=(");
#endif
	if (strcmp(key->algorithm, "DES") == 0) {
		assert(key->key != NULL);
		assert(key->key_len == 8);

		/* check parity bits */
		for (i = 0; i < 8; ++i) 
		{
			k = key->key[i] & 0xfe;
			j = k;
			j ^= j >> 4;
			j ^= j >> 2;
			j ^= j >> 1;
			j = (j & 1) ^ 1;
			assert((key->key[i] & 0x01) == j);
		}
	}
}

void mbus_get_hashkey(struct mbus_config *m, struct mbus_key *key)
{
	/* This MUST be called while the config file is locked! */
#ifdef WIN32
	long	 status;
	DWORD	 type;
	char	*buffer;
	int	 buflen = MBUS_BUF_SIZE;
	char	*tmp;

	assert(m->cfg_locked);
	
	/* Read the key from the registry... */
	buffer = (char *) xmalloc(MBUS_BUF_SIZE);
	status = RegQueryValueEx(m->cfgKey, "HASHKEY", 0, &type, buffer, &buflen);
	if (status != ERROR_SUCCESS) {
		FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, status, 0, buffer, MBUS_BUF_SIZE, NULL);
		debug_msg("Unable to get encrkey: %s\n", buffer);
		abort();
	}
	assert(type == REG_SZ);
	assert(buflen > 0);

	/* Parse the key... */
	key->algorithm   = strdup(strtok(buffer+1, ","));
	key->key         = strtok(NULL  , ")");
	key->key_len     = strlen(key->key);

	/* Decode the key... */
	tmp = (char *) xmalloc(key->key_len);
	key->key_len = base64decode(key->key, key->key_len, tmp, key->key_len);
	key->key = tmp;

	xfree(buffer);
#else
	mbus_get_key(m, key, "HASHKEY=(");
#endif
}

void mbus_get_net_addr(struct mbus_config *m, char *net_addr, uint16_t *net_port, int *net_scope)
{
#ifdef WIN32
	long	 status;
	DWORD	 type;
	char	*buffer;
	int	 buflen = MBUS_BUF_SIZE;
	uint32_t port;

	assert(m->cfg_locked);
	
	/* Read the key from the registry... */
	buffer = (char *) xmalloc(MBUS_BUF_SIZE);
        buflen = MBUS_BUF_SIZE;
	status = RegQueryValueEx(m->cfgKey, "ADDRESS", 0, &type, buffer, &buflen);
	if (status != ERROR_SUCCESS) {
		FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, status, 0, buffer, MBUS_BUF_SIZE, NULL);
		debug_msg("Unable to get address: %s\n", buffer);
		strcpy(net_addr, MBUS_DEFAULT_NET_ADDR);
	} else {
		assert(type == REG_SZ);
		assert(buflen > 0);
		strncpy(net_addr, buffer, buflen);
	}

	buflen = sizeof(port);
	status = RegQueryValueEx(m->cfgKey, "PORT", 0, &type, (uint8_t *) &port, &buflen);
	if (status != ERROR_SUCCESS) {
		FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, status, 0, buffer, MBUS_BUF_SIZE, NULL);
		debug_msg("Unable to get port: %s\n", buffer);
		*net_port  = MBUS_DEFAULT_NET_PORT;
	} else {
		assert(type == REG_DWORD);
		assert(buflen == 4);
		*net_port = (uint16_t) port;
	}

	buflen = MBUS_BUF_SIZE;
	status = RegQueryValueEx(m->cfgKey, "SCOPE", 0, &type, buffer, &buflen);
	if (status != ERROR_SUCCESS) {
		FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, status, 0, buffer, MBUS_BUF_SIZE, NULL);
		debug_msg("Unable to get scope: %s\n", buffer);
		*net_scope = MBUS_DEFAULT_SCOPE;
	} else {
		assert(type == REG_SZ);
		assert(buflen > 0);
		if (strncmp(buffer, SCOPE_HOSTLOCAL_NAME, strlen(SCOPE_HOSTLOCAL_NAME)) == 0) {
			*net_scope = SCOPE_HOSTLOCAL;
		} else if (strncmp(buffer, SCOPE_LINKLOCAL_NAME, strlen(SCOPE_LINKLOCAL_NAME)) == 0) {
			*net_scope = SCOPE_LINKLOCAL;
		} else {
			debug_msg("Unrecognized scope: %s\n", buffer);
			*net_scope = MBUS_DEFAULT_SCOPE;
		}
	}
        xfree(buffer);
#else
	struct stat	 s;
	char		*buf;
	char		*line;
	int		 pos;
	int              pos2;
        int              linepos;

	int              scope;
	char            *addr;
	uint16_t        port;

	assert(m->cfg_locked);

	addr = (char *)xmalloc(20);
	memset(addr, 0, 20);
	port = 0;
	scope = -1;
	

	if (lseek(m->cfgfd, 0, SEEK_SET) == -1) {
		perror("Can't seek to start of config file");
		abort();
	}
	if (fstat(m->cfgfd, &s) != 0) {
		perror("Unable to stat config file\n");
		abort();
	}
	/* Read in the contents of the config file... */
	buf = (char *) xmalloc(s.st_size+1);
	memset(buf, '\0', s.st_size+1);
	if (read(m->cfgfd, buf, s.st_size) != s.st_size) {
		perror("Unable to read config file\n");
		abort();
	}
	
	line = (char *) xmalloc(s.st_size+1);
	sscanf(buf, "%s", line);
	if (strcmp(line, "[MBUS]") != 0) {
		debug_msg("Invalid .mbus file\n");
		abort();
	}
	pos = strlen(line) + 1;
	while (pos < s.st_size) {
                /* get whole line and filter out spaces */
                linepos=0;
                do {
                    while((buf[pos+linepos]==' ')||(buf[pos+linepos]=='\n')
                                                 ||(buf[pos+linepos]=='\t'))
                        pos++;
                    sscanf(buf+pos+linepos, "%s", line+linepos);                
                    linepos = strlen(line);
  
                } while((buf[pos+linepos]!='\n') && (s.st_size>(pos+linepos+1)));
                pos += linepos + 1;     
		if (strncmp(line, "SCOPE", strlen("SCOPE")) == 0) {
		    pos2 = strlen("SCOPE") + 1;
		    if (strncmp(line+pos2, SCOPE_HOSTLOCAL_NAME, 
				strlen(SCOPE_HOSTLOCAL_NAME)) == 0) {
			scope = SCOPE_HOSTLOCAL;
		    }
		    if (strncmp(line+pos2, SCOPE_LINKLOCAL_NAME, 
				strlen(SCOPE_LINKLOCAL_NAME)) == 0) {
			scope = SCOPE_LINKLOCAL ;
		    }
		}
		if (strncmp(line, "ADDRESS", strlen("ADDRESS")) == 0) {
		    pos2 = strlen("ADDRESS") + 1;
		    strncpy(addr, line+pos2, 16);
		}
		if (strncmp(line, "PORT", strlen("PORT")) == 0) {
		    pos2 = strlen("PORT") + 1;
		    port = atoi(line+pos2);
		}
		    
	}
	/* If we did not find all values, fill in the default ones */
	*net_port  = (port>0)?port:MBUS_DEFAULT_NET_PORT;
	*net_scope = (scope!=-1)?scope:MBUS_DEFAULT_SCOPE;
	if (strlen(addr)>0) {
		strncpy(net_addr, addr, 16);
	} else {
		strcpy(net_addr, MBUS_DEFAULT_NET_ADDR);
	}
	debug_msg("using Addr:%s Port:%d Scope:%s for MBUS\n", 
		  net_addr, *net_port, (*net_scope==0)?SCOPE_HOSTLOCAL_NAME:SCOPE_LINKLOCAL_NAME);
	xfree(buf);
	xfree(line);
	xfree(addr);
#endif
}

int mbus_get_version(struct mbus_config *m)
{
#ifdef WIN32
	long		status;
	DWORD		type;
	uint32_t	cver;
	int		verl;
	char		buffer[MBUS_BUF_SIZE];

	assert(m->cfg_locked);
	
	/* Read the key from the registry... */
        verl = sizeof(&cver);
	status = RegQueryValueEx(m->cfgKey, "CONFIG_VERSION", 0, &type, (uint8_t *) &cver, &verl);
	if (status != ERROR_SUCCESS) {
		FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, status, 0, buffer, MBUS_BUF_SIZE, NULL);
		debug_msg("Unable to get version: %s\n", buffer);
		return 0;
	}
	assert(type == REG_DWORD);
	assert(verl == 4);
	return cver;
#else
	struct stat	 s;
	char		*buf;
	char		*line;
	int		 pos;
	int              pos2;
        int              linepos;
	int		 version = 0;

	assert(m->cfg_locked);

	if (lseek(m->cfgfd, 0, SEEK_SET) == -1) {
		perror("Can't seek to start of config file");
		abort();
	}
	if (fstat(m->cfgfd, &s) != 0) {
		perror("Unable to stat config file\n");
		abort();
	}
	/* Read in the contents of the config file... */
	buf = (char *) xmalloc(s.st_size+1);
	memset(buf, '\0', s.st_size+1);
	if (read(m->cfgfd, buf, s.st_size) != s.st_size) {
		perror("Unable to read config file\n");
		abort();
	}
	
	line = (char *) xmalloc(s.st_size+1);
	sscanf(buf, "%s", line);
	if (strcmp(line, "[MBUS]") != 0) {
		debug_msg("Invalid .mbus file\n");
		abort();
	}
	pos = strlen(line) + 1;
	while (pos < s.st_size) {
                /* get whole line and filter out spaces */
                linepos=0;
                do {
			while((buf[pos+linepos]==' ')||(buf[pos+linepos]=='\n') ||(buf[pos+linepos]=='\t')) {
				pos++;
			}
			sscanf(buf+pos+linepos, "%s", line+linepos);                
			linepos = strlen(line);
                } while((buf[pos+linepos]!='\n') && (s.st_size>(pos+linepos+1)));
                pos += linepos + 1;     
		if (strncmp(line, "CONFIG_VERSION", strlen("CONFIG_VERSION")) == 0) {
		    pos2    = strlen("CONFIG_VERSION") + 1;
		    version = atoi(line+pos2);
		}
		    
	}
	xfree(buf);
	xfree(line);
	return version;
#endif
}

struct mbus_config *mbus_create_config(void)
{
	return (struct mbus_config *) xmalloc(sizeof(struct mbus_config));
}


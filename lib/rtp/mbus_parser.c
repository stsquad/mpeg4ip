/*
 * FILE:     mbus_parser.c
 * AUTHOR:   Colin Perkins
 * 
 * Copyright (c) 1997-2000 University College London
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

#define MBUS_PARSER_MAGIC 0xbadface

struct mbus_parser {
	char	*buffer;	/* Temporary storage for parsing mbus commands 			*/
	char	*bufend;	/* End of space allocated for parsing, to check for overflows 	*/
	uint32_t magic;		/* For debugging...                                             */
};

struct mbus_parser *mbus_parse_init(char *str)
{
	struct mbus_parser	*m;

	m = (struct mbus_parser *) xmalloc(sizeof(struct mbus_parser));
	m->buffer = str;
	m->bufend = str + strlen(str);
	m->magic  = MBUS_PARSER_MAGIC;
	return m;
}

void mbus_parse_done(struct mbus_parser *m)
{
	ASSERT(m->magic == MBUS_PARSER_MAGIC);
	xfree(m);
}

#define CHECK_OVERRUN if (m->buffer > m->bufend) {\
	debug_msg("parse m->buffer overflow\n");\
	return FALSE;\
}

int mbus_parse_lst(struct mbus_parser *m, char **l)
{
	int instr = FALSE;
	int inlst = FALSE;

	ASSERT(m->magic == MBUS_PARSER_MAGIC);

        while (isspace((unsigned char)*m->buffer)) {
                m->buffer++;
		CHECK_OVERRUN;
        }
	if (*m->buffer != '(') {
		return FALSE;
	}
	m->buffer++;
	*l = m->buffer;
	while (*m->buffer != '\0') {
		if ((*m->buffer == '"') && (*(m->buffer-1) != '\\')) {
			instr = !instr;
		}
		if ((*m->buffer == '(') && (*(m->buffer-1) != '\\') && !instr) {
			inlst = !inlst;
		}
		if ((*m->buffer == ')') && (*(m->buffer-1) != '\\') && !instr) {
			if (inlst) {
				inlst = !inlst;
			} else {
				*m->buffer = '\0';
				m->buffer++;
				CHECK_OVERRUN;
				return TRUE;
			}
		}
		m->buffer++;
		CHECK_OVERRUN;
	}
	return FALSE;
}

int mbus_parse_str(struct mbus_parser *m, char **s)
{
	ASSERT(m->magic == MBUS_PARSER_MAGIC);

        while (isspace((unsigned char)*m->buffer)) {
                m->buffer++;
		CHECK_OVERRUN;
        }
	if (*m->buffer != '"') {
		return FALSE;
	}
	*s = m->buffer++;
	while (*m->buffer != '\0') {
		if ((*m->buffer == '"') && (*(m->buffer-1) != '\\')) {
			m->buffer++;
			*m->buffer = '\0';
			m->buffer++;
			return TRUE;
		}
		m->buffer++;
		CHECK_OVERRUN;
	}
	return FALSE;
}

int mbus_parse_sym(struct mbus_parser *m, char **s)
{
	ASSERT(m->magic == MBUS_PARSER_MAGIC);

        while (isspace((unsigned char)*m->buffer)) {
                m->buffer++;
		CHECK_OVERRUN;
        }
	if (!isgraph((unsigned char)*m->buffer)) {
		return FALSE;
	}
	*s = m->buffer++;
	while (!isspace((unsigned char)*m->buffer) && (*m->buffer != '\0')) {
		m->buffer++;
		CHECK_OVERRUN;
	}
	*m->buffer = '\0';
	m->buffer++;
	CHECK_OVERRUN;
	return TRUE;
}

int mbus_parse_int(struct mbus_parser *m, int *i)
{
	char	*p;

	ASSERT(m->magic == MBUS_PARSER_MAGIC);

        while (isspace((unsigned char)*m->buffer)) {
                m->buffer++;
		CHECK_OVERRUN;
        }

	*i = strtol(m->buffer, &p, 10);
	if (((*i == INT_MAX) || (*i == INT_MIN)) && (errno == ERANGE)) {
		debug_msg("integer out of range\n");
		return FALSE;
	}

	if (p == m->buffer) {
		return FALSE;
	}
	if (!isspace((unsigned char)*p) && (*p != '\0')) {
		return FALSE;
	}
	m->buffer = p;
	CHECK_OVERRUN;
	return TRUE;
}

int mbus_parse_flt(struct mbus_parser *m, double *d)
{
	char	*p;

	ASSERT(m->magic == MBUS_PARSER_MAGIC);

        while (isspace((unsigned char)*m->buffer)) {
                m->buffer++;
		CHECK_OVERRUN;
        }

	*d = strtod(m->buffer, &p);
	if (errno == ERANGE) {
		debug_msg("float out of range\n");
		return FALSE;
	}

	if (p == m->buffer) {
		return FALSE;
	}
	if (!isspace((unsigned char)*p) && (*p != '\0')) {
		return FALSE;
	}
	m->buffer = p;
	CHECK_OVERRUN;
	return TRUE;
}

char *mbus_decode_str(char *s)
{
	int	l = strlen(s);
	int	i, j;

	/* Check that this an encoded string... */
	ASSERT(s[0]   == '\"');
	ASSERT(s[l-1] == '\"');

	for (i=1,j=0; i < l - 1; i++,j++) {
		if (s[i] == '\\') {
			i++;
		}
		s[j] = s[i];
	}
	s[j] = '\0';
	return s;
}

char *mbus_encode_str(const char *s)
{
	int 	 i, j;
	int	 len = strlen(s);
	char	*buf = (char *) xmalloc((len * 2) + 3);

	for (i = 0, j = 1; i < len; i++,j++) {
		if (s[i] == ' ') {
			buf[j] = '\\';
			buf[j+1] = ' ';
			j++;
		} else if (s[i] == '\"') {
			buf[j] = '\\';
			buf[j+1] = '\"';
			j++;
		} else {
			buf[j] = s[i];
		}
	}
	buf[0]   = '\"';
	buf[j]   = '\"';
	buf[j+1] = '\0';
	return buf;
}


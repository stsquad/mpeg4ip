/*
 * The contents of this file are subject to the Mozilla Public
 * License Version 1.1 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.mozilla.org/MPL/
 * 
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 * 
 * The Original Code is MPEG4IP.
 * 
 * The Initial Developer of the Original Code is Cisco Systems Inc.
 * Portions created by Cisco Systems Inc. are
 * Copyright (C) Cisco Systems Inc. 2001.  All Rights Reserved.
 * 
 * Contributor(s): 
 *		Dave Mackie		dmackie@cisco.com
 */

#ifndef __MP4_UTIL_INCLUDED__
#define __MP4_UTIL_INCLUDED__

#include <assert.h>

#ifdef NDEBUG
#define ASSERT(expr)
#define WARNING(expr)
#else
#define ASSERT(expr)	assert(expr)
#define WARNING(expr) \
	if (expr) { \
		fprintf(stderr, "Warning (%s) in %s at line %u\n", \
			__STRING(expr), __FILE__, __LINE__); \
	}
#endif

#define VERBOSE(exprverbosity, verbosity, expr)	\
	if ((exprverbosity) & (verbosity)) { expr; }

/* verbosity levels, inputs to SetVerbosity */
#define MP4_DETAILS_ALL				0xFFFF
#define MP4_DETAILS_ERROR			0x1
#define MP4_DETAILS_WARNING			0x2
#define MP4_DETAILS_READ			0x4
#define MP4_DETAILS_READ_TABLE		0x8
#define MP4_DETAILS_READ_ALL		\
	(MP4_DETAILS_READ | MP4_DETAILS_READ_TABLE)
#define MP4_DETAILS_FIND			0x10

#define VERBOSE_ERROR(verbosity, expr)		\
	VERBOSE(MP4_DETAILS_ERROR, verbosity, expr)

#define VERBOSE_WARNING(verbosity, expr)		\
	VERBOSE(MP4_DETAILS_WARNING, verbosity, expr)

#define VERBOSE_READ(verbosity, expr)		\
	VERBOSE(MP4_DETAILS_READ, verbosity, expr)

#define VERBOSE_READ_TABLE(verbosity, expr)	\
	VERBOSE(MP4_DETAILS_READ_ALL, verbosity, expr)

#define VERBOSE_FIND(verbosity, expr)		\
	VERBOSE(MP4_DETAILS_FIND, verbosity, expr)

inline void Indent(FILE* pFile, u_int8_t depth) {
	fprintf(pFile, "%*c", depth, ' ');
}

class MP4Error {
public:
	MP4Error() {
		m_errno = 0;
		m_errstring = NULL;
		m_where = NULL;
	}
	MP4Error(int err, char* where = NULL) {
		m_errno = err;
		m_errstring = NULL;
		m_where = where;
	}
	MP4Error(char* errstring, char* where = NULL) {
		m_errno = 0;
		m_errstring = errstring;
		m_where = where;
	}
	MP4Error(int err, char* errstring, char* where) {
		m_errno = err;
		m_errstring = errstring;
		m_where = where;
	}

	void Print(FILE* pFile = stderr) {
		if (m_where) {
			fprintf(pFile, "%s: ", m_where);
		}
		if (m_errstring) {
			fprintf(pFile, "%s: ", m_errstring);
		}
		if (m_errno) {
			fprintf(pFile, "%s ", strerror(m_errno));
		}
		fprintf(pFile, "\n");
	}

	int m_errno;
	char* m_errstring;
	char* m_where;
};

inline void* MP4Malloc(size_t size) {
	void* p = malloc(size);
	if (p == NULL && size > 0) {
		throw new MP4Error(errno);
	}
	return p;
}

inline char* MP4Stralloc(char* s1) {
	char* s2 = (char*)MP4Malloc(strlen(s1) + 1);
	strcpy(s2, s1);
	return s2;
}

inline void* MP4Realloc(void* p, u_int32_t newSize) {
	// workaround library bug
	if (p == NULL && newSize == 0) {
		return NULL;
	}
	p = realloc(p, newSize);
	if (p == NULL && newSize > 0) {
		throw new MP4Error(errno);
	}
	return p;
}

inline void MP4Free(void* p) {
	free(p);
}

bool MP4NameFirstMatches(const char* s1, const char* s2);

bool MP4NameFirstIndex(const char* s, u_int32_t* pIndex);

char* MP4NameAfterFirst(char *s);

#endif /* __MP4_UTIL_INCLUDED__ */

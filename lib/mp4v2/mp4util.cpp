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

#include "mp4common.h"

bool MP4NameFirstMatches(const char* s1, const char* s2) 
{
	if (s1 == NULL || *s1 == '\0' || s2 == NULL || *s2 == '\0') {
		return false;
	}

	if (*s2 == '*') {
		return true;
	}

	while (*s1 != '\0') {
		if (*s2 == '\0' || strchr("[.", *s2)) {
			break;
		}
		if (tolower(*s1) != tolower(*s2)) {
			return false;
		}
		s1++;
		s2++;
	}
	return true;
}

bool MP4NameFirstIndex(const char* s, u_int32_t* pIndex)
{
	if (s == NULL) {
		return false;
	}

	while (*s != '\0' && *s != '.') {
		if (*s == '[') {
			s++;
			ASSERT(pIndex);
			if (sscanf(s, "%u", pIndex) != 1) {
				return false;
			}
			return true;
		}
		s++;
	}
	return false;
}

char* MP4NameAfterFirst(char *s)
{
	if (s == NULL) {
		return NULL;
	}

	while (*s != '\0') {
		if (*s == '.') {
			s++;
			if (*s == '\0') {
				return NULL;
			}
			return s;
		}
		s++;
	}
	return NULL;
}


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
 * Copyright (C) Cisco Systems Inc. 2000, 2001.  All Rights Reserved.
 * 
 * Contributor(s): 
 *		Dave Mackie		dmackie@cisco.com
 */

#include "quicktime.h"


int quicktime_tref_init(quicktime_tref_t *tref)
{
	quicktime_hint_init(&(tref->hint));
}

int quicktime_tref_init_hint(quicktime_tref_t *tref, quicktime_trak_t *refTrak)
{
	quicktime_hint_set(&(tref->hint), refTrak);
}

int quicktime_tref_delete(quicktime_tref_t *tref)
{
	quicktime_hint_delete(&(tref->hint));
}

int quicktime_tref_dump(quicktime_tref_t *tref)
{
	printf("  tref\n");
	quicktime_hint_dump(&tref->hint);
}

int quicktime_read_tref(quicktime_t *file, quicktime_tref_t *tref, quicktime_atom_t *parent_atom)
{
	quicktime_atom_t leaf_atom;

	do {
		quicktime_atom_read_header(file, &leaf_atom);

		if (quicktime_atom_is(&leaf_atom, "hint")) {
			quicktime_read_hint(file, &(tref->hint), &leaf_atom);
		} else {
			quicktime_atom_skip(file, &leaf_atom);
		}
	} while (quicktime_position(file) < parent_atom->end);
}

int quicktime_write_tref(quicktime_t *file, quicktime_tref_t *tref)
{
	quicktime_atom_t atom;

	if (tref->hint.numTracks == 0) {
		return;
	}

	quicktime_atom_write_header(file, &atom, "tref");

	quicktime_write_hint(file, &(tref->hint));

	quicktime_atom_write_footer(file, &atom);
}


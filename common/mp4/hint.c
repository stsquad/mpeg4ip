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


int quicktime_hint_init(quicktime_hint_t *hint)
{
	hint->numTracks = 0;
	hint->trackIds[0] = -1;
	hint->traks[0] = NULL;
}

int quicktime_hint_set(quicktime_hint_t *hint, quicktime_trak_t *refTrak)
{
	hint->traks[hint->numTracks] = refTrak;
	hint->trackIds[hint->numTracks] = refTrak->tkhd.track_id;
	hint->numTracks++;
}

int quicktime_hint_delete(quicktime_hint_t *hint)
{
}

int quicktime_read_hint(quicktime_t *file, quicktime_hint_t *hint, quicktime_atom_t *parent_atom)
{
	quicktime_trak_t* refTrak = NULL;
	int i;

	while (quicktime_position(file) < parent_atom->end) {
		hint->trackIds[hint->numTracks] = quicktime_read_int32(file);
		hint->traks[hint->numTracks] = NULL; 
		hint->numTracks++;
	}
}

int quicktime_write_hint(quicktime_t *file, quicktime_hint_t *hint)
{
	quicktime_atom_t atom;
	int i;

	quicktime_atom_write_header(file, &atom, "hint");

	for (i = 0; i < hint->numTracks; i++) {
		quicktime_write_int32(file, hint->trackIds[i]);
	}

	quicktime_atom_write_footer(file, &atom);
}

int quicktime_hint_dump(quicktime_hint_t *hint)
{
	int i;

	printf("   hint\n");
	for (i = 0; i < hint->numTracks; i++) {
		printf("    track %d\n", hint->trackIds[i]);
	}
}


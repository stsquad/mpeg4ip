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


int quicktime_hint_udta_init(quicktime_hint_udta_t *hint_udta)
{
	quicktime_hinf_init(&(hint_udta->hinf));
	quicktime_hint_hnti_init(&(hint_udta->hnti));
}

int quicktime_hint_udta_delete(quicktime_hint_udta_t *hint_udta)
{
	quicktime_hinf_delete(&(hint_udta->hinf));
	quicktime_hint_hnti_delete(&(hint_udta->hnti));
}

int quicktime_hint_udta_dump(quicktime_hint_udta_t *hint_udta)
{
	printf("  udta\n");
	quicktime_hinf_dump(&hint_udta->hinf);
	quicktime_hint_hnti_dump(&hint_udta->hnti);
}

int quicktime_read_hint_udta(quicktime_t *file, quicktime_hint_udta_t *hint_udta, quicktime_atom_t *parent_atom)
{
	quicktime_atom_t leaf_atom;

	do {
		/* udta atoms can be terminated by a 4 byte zero */
		if (parent_atom->end - quicktime_position(file) < HEADER_LENGTH) {
			u_char trash[HEADER_LENGTH];
			int remainder = parent_atom->end - quicktime_position(file);
			quicktime_read_data(file, trash, remainder);
			break;
		}

		quicktime_atom_read_header(file, &leaf_atom);

		if (quicktime_atom_is(&leaf_atom, "hinf")) {
			quicktime_read_hinf(file, &(hint_udta->hinf), &leaf_atom);
		} else if (quicktime_atom_is(&leaf_atom, "hnti")) {
			quicktime_read_hint_hnti(file, &(hint_udta->hnti), &leaf_atom);
		} else {
			quicktime_atom_skip(file, &leaf_atom);
		}
	} while (quicktime_position(file) < parent_atom->end);
}

int quicktime_write_hint_udta(quicktime_t *file, quicktime_hint_udta_t *hint_udta)
{
	quicktime_atom_t atom;

	if (hint_udta->hnti.sdp.string == NULL) {
		return;
	}

	quicktime_atom_write_header(file, &atom, "udta");

	quicktime_write_hinf(file, &(hint_udta->hinf));
	quicktime_write_hint_hnti(file, &(hint_udta->hnti));

	quicktime_atom_write_footer(file, &atom);
}


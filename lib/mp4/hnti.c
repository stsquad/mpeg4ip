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


int quicktime_hnti_init(quicktime_hnti_t *hnti)
{
	quicktime_rtp_init(&(hnti->rtp));
}

int quicktime_hnti_delete(quicktime_hnti_t *hnti)
{
	quicktime_rtp_delete(&(hnti->rtp));
}

int quicktime_hnti_dump(quicktime_hnti_t *hnti)
{
	printf("   hnti\n");
	quicktime_rtp_dump(&hnti->rtp);
}

int quicktime_read_hnti(quicktime_t *file, quicktime_hnti_t *hnti, quicktime_atom_t *parent_atom)
{
	quicktime_atom_t leaf_atom;

	do {
		quicktime_atom_read_header(file, &leaf_atom);

		if (quicktime_atom_is(&leaf_atom, "rtp ")) {
			quicktime_read_rtp(file, &(hnti->rtp), &leaf_atom);
		} else {
			quicktime_atom_skip(file, &leaf_atom);
		}
	} while (quicktime_position(file) < parent_atom->end);
}

int quicktime_write_hnti(quicktime_t *file, quicktime_hnti_t *hnti)
{
	quicktime_atom_t atom;

	if (hnti->rtp.string == NULL) {
		return;
	}

	quicktime_atom_write_header(file, &atom, "hnti");

	quicktime_write_rtp(file, &(hnti->rtp));

	quicktime_atom_write_footer(file, &atom);
}


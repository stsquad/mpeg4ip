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


int quicktime_maxr_init(quicktime_maxr_t *maxr)
{
	maxr->granularity = 0;
	maxr->maxBitRate = 0;
}

int quicktime_maxr_delete(quicktime_maxr_t *maxr)
{
}

int quicktime_maxr_dump(quicktime_maxr_t *maxr)
{
	printf("    max data rate\n");
	printf("     granularity %lu\n", maxr->granularity);
	printf("     maxBitRate %lu\n", maxr->maxBitRate);
}

int quicktime_read_maxr(quicktime_t *file, quicktime_maxr_t *maxr)
{
	maxr->granularity = quicktime_read_int32(file);
	maxr->maxBitRate = quicktime_read_int32(file);
}

int quicktime_write_maxr(quicktime_t *file, quicktime_maxr_t *maxr)
{
	quicktime_atom_t atom;

	quicktime_atom_write_header(file, &atom, "maxr");

	quicktime_write_int32(file, maxr->granularity);
	quicktime_write_int32(file, maxr->maxBitRate);
	
	quicktime_atom_write_footer(file, &atom);
}


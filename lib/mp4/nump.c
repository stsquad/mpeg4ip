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


int quicktime_nump_init(quicktime_nump_t *nump)
{
	nump->numPackets = 0;
}

int quicktime_nump_delete(quicktime_nump_t *nump)
{
}

int quicktime_nump_dump(quicktime_nump_t *nump)
{
	printf("    total RTP packets\n");
	printf("     numBytes "U64"\n", nump->numPackets);
}

int quicktime_read_nump(quicktime_t *file, quicktime_nump_t *nump)
{
	nump->numPackets = quicktime_read_int64(file);
}

int quicktime_write_nump(quicktime_t *file, quicktime_nump_t *nump)
{
	quicktime_atom_t atom;

	quicktime_atom_write_header(file, &atom, "nump");

	quicktime_write_int64(file, nump->numPackets);
	
	quicktime_atom_write_footer(file, &atom);
}


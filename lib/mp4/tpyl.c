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


int quicktime_tpyl_init(quicktime_tpyl_t *tpyl)
{
	tpyl->numBytes = 0;
}

int quicktime_tpyl_delete(quicktime_tpyl_t *tpyl)
{
}

int quicktime_tpyl_dump(quicktime_tpyl_t *tpyl)
{
	printf("    total RTP payload bytes\n");
	printf("     numBytes "U64"\n", tpyl->numBytes);
}

int quicktime_read_tpyl(quicktime_t *file, quicktime_tpyl_t *tpyl)
{
	tpyl->numBytes = quicktime_read_int64(file);
}

int quicktime_write_tpyl(quicktime_t *file, quicktime_tpyl_t *tpyl)
{
	quicktime_atom_t atom;

	quicktime_atom_write_header(file, &atom, "tpyl");

	quicktime_write_int64(file, tpyl->numBytes);
	
	quicktime_atom_write_footer(file, &atom);
}


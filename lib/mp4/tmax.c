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


int quicktime_tmax_init(quicktime_tmax_t *tmax)
{
	tmax->milliSecs = 0;
}

int quicktime_tmax_delete(quicktime_tmax_t *tmax)
{
}

int quicktime_tmax_dump(quicktime_tmax_t *tmax)
{
	printf("    max relative xmit time\n");
	printf("     milliSecs %lu\n", tmax->milliSecs);
}

int quicktime_read_tmax(quicktime_t *file, quicktime_tmax_t *tmax)
{
	tmax->milliSecs = quicktime_read_int32(file);
}

int quicktime_write_tmax(quicktime_t *file, quicktime_tmax_t *tmax)
{
	quicktime_atom_t atom;

	quicktime_atom_write_header(file, &atom, "tmax");

	quicktime_write_int32(file, tmax->milliSecs);
	
	quicktime_atom_write_footer(file, &atom);
}


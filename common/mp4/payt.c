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


int quicktime_payt_init(quicktime_payt_t *payt)
{
	payt->payloadNumber = 0;
	payt->rtpMapString[0] = '\0';
}

int quicktime_payt_delete(quicktime_payt_t *payt)
{
}

int quicktime_payt_dump(quicktime_payt_t *payt)
{
	printf("    payload type\n");
	printf("     payload number %lu\n", payt->payloadNumber);
	printf("     rtp map string %s\n", payt->rtpMapString);
}

int quicktime_read_payt(quicktime_t *file, quicktime_payt_t *payt)
{
	payt->payloadNumber = quicktime_read_int32(file);
	quicktime_read_pascal(file, payt->rtpMapString);
}

int quicktime_write_payt(quicktime_t *file, quicktime_payt_t *payt)
{
	quicktime_atom_t atom;

	quicktime_atom_write_header(file, &atom, "payt");

	quicktime_write_int32(file, payt->payloadNumber);
	quicktime_write_pascal(file, payt->rtpMapString);
	
	quicktime_atom_write_footer(file, &atom);
}


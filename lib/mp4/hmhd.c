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


int quicktime_hmhd_init(quicktime_hmhd_t *hmhd)
{
	hmhd->version = 0;
	hmhd->flags = 0;
	hmhd->maxPDUsize = 0;
	hmhd->avgPDUsize = 0;
	hmhd->maxbitrate = 0;
	hmhd->avgbitrate = 0;
	hmhd->slidingavgbitrate = 0;
}

int quicktime_hmhd_delete(quicktime_hmhd_t *hmhd)
{
}

int quicktime_hmhd_dump(quicktime_hmhd_t *hmhd)
{
	printf("    hint media header\n");
	printf("     version %d\n", hmhd->version);
	printf("     flags %d\n", hmhd->flags);
	printf("     maxPDUsize %d\n", hmhd->maxPDUsize);
	printf("     avgPDUsize %d\n", hmhd->avgPDUsize);
	printf("     maxbitrate %d\n", hmhd->maxbitrate);
	printf("     avgbitrate %d\n", hmhd->avgbitrate);
	printf("     slidingavgbitrate %d\n", hmhd->slidingavgbitrate);
}

int quicktime_read_hmhd(quicktime_t *file, quicktime_hmhd_t *hmhd)
{
	int i;
	hmhd->version = quicktime_read_char(file);
	hmhd->flags = quicktime_read_int24(file);
	hmhd->maxPDUsize = quicktime_read_int16(file);
	hmhd->avgPDUsize = quicktime_read_int16(file);
	hmhd->maxbitrate = quicktime_read_int32(file);
	hmhd->avgbitrate = quicktime_read_int32(file);
	hmhd->slidingavgbitrate = quicktime_read_int32(file);
}

int quicktime_write_hmhd(quicktime_t *file, quicktime_hmhd_t *hmhd)
{
	quicktime_atom_t atom;
	int i;

	quicktime_atom_write_header(file, &atom, "hmhd");

	quicktime_write_char(file, hmhd->version);
	quicktime_write_int24(file, hmhd->flags);

	quicktime_write_int16(file, hmhd->maxPDUsize);
	quicktime_write_int16(file, hmhd->avgPDUsize);
	quicktime_write_int32(file, hmhd->maxbitrate);
	quicktime_write_int32(file, hmhd->avgbitrate);
	quicktime_write_int32(file, hmhd->slidingavgbitrate);

	quicktime_atom_write_footer(file, &atom);
}


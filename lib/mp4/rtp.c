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
 *              Dave Mackie             dmackie@cisco.com
 */

#include "quicktime.h"


int quicktime_rtp_init(quicktime_rtp_t *rtp)
{
        rtp->string = NULL;
}

int quicktime_rtp_set(quicktime_rtp_t *rtp, char *string)
{
        free(rtp->string);
        if (string != NULL) {
                rtp->string = malloc(strlen(string) + 1);
                strcpy(rtp->string, string);
        } else {
                rtp->string = NULL;
        }
}

int quicktime_rtp_append(quicktime_rtp_t *rtp, char *appendString)
{
        char* newString = malloc(strlen(rtp->string) + strlen(appendString) + 1);

        strcpy(newString, rtp->string);
        strcat(newString, appendString);
        free(rtp->string);
        rtp->string = newString;
}

int quicktime_rtp_delete(quicktime_rtp_t *rtp)
{
        free(rtp->string);
}

int quicktime_rtp_dump(quicktime_rtp_t *rtp)
{
        printf("    rtp\n");
        printf("     string %s\n", rtp->string);
}

int quicktime_read_rtp(quicktime_t *file, quicktime_rtp_t *rtp, 
        quicktime_atom_t* rtp_atom)
{
        int rtpLen = rtp_atom->size - 12;
        free(rtp->string);
        rtp->string = malloc(rtpLen + 1);
		quicktime_read_int32(file);	// skip the 'sdp '
        quicktime_read_data(file, rtp->string, rtpLen);
        rtp->string[rtpLen] = '\0';
}

int quicktime_write_rtp(quicktime_t *file, quicktime_rtp_t *rtp)
{
        int i;
        quicktime_atom_t atom;

        if (rtp->string == NULL) {
                return;
        }

        quicktime_atom_write_header(file, &atom, "rtp ");

        quicktime_write_data(file, "sdp ", 4);
        quicktime_write_data(file, rtp->string, strlen(rtp->string));

        quicktime_atom_write_footer(file, &atom);
}

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


int quicktime_sdp_init(quicktime_sdp_t *sdp)
{
        sdp->string = NULL;
}

int quicktime_sdp_set(quicktime_sdp_t *sdp, char *string)
{
        free(sdp->string);
        if (string != NULL) {
                sdp->string = malloc(strlen(string) + 1);
                strcpy(sdp->string, string);
        } else {
                sdp->string = NULL;
        }
}

int quicktime_sdp_append(quicktime_sdp_t *sdp, char *appendString)
{
        char* newString = malloc(strlen(sdp->string) + strlen(appendString) + 1);

        strcpy(newString, sdp->string);
        strcat(newString, appendString);
        free(sdp->string);
        sdp->string = newString;
}

int quicktime_sdp_delete(quicktime_sdp_t *sdp)
{
        free(sdp->string);
}

int quicktime_sdp_dump(quicktime_sdp_t *sdp)
{
        printf("    sdp\n");
        printf("     string %s\n", sdp->string);
}

int quicktime_read_sdp(quicktime_t *file, quicktime_sdp_t *sdp, 
        quicktime_atom_t* sdp_atom)
{
        int sdpLen = sdp_atom->size - 8;
        free(sdp->string);
        sdp->string = malloc(sdpLen + 1);
        quicktime_read_data(file, sdp->string, sdpLen);
        sdp->string[sdpLen] = '\0';
}

int quicktime_write_sdp(quicktime_t *file, quicktime_sdp_t *sdp)
{
        int i;
        quicktime_atom_t atom;

        if (sdp->string == NULL) {
                return;
        }

        quicktime_atom_write_header(file, &atom, "sdp ");

        quicktime_write_data(file, sdp->string, strlen(sdp->string));

        quicktime_atom_write_footer(file, &atom);
}

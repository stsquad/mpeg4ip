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


int quicktime_iods_init(quicktime_iods_t *iods)
{
	iods->version = 0;
	iods->flags = 0;
	iods->audioProfileId = 0xFF;
	iods->videoProfileId = 0xFF;
	return 0;
}

int quicktime_iods_set_audio_profile(quicktime_iods_t* iods, int id)
{
	iods->audioProfileId = id;
}

int quicktime_iods_set_video_profile(quicktime_iods_t* iods, int id)
{
	iods->videoProfileId = id;
}

int quicktime_iods_delete(quicktime_iods_t *iods)
{
	return 0;
}

int quicktime_iods_dump(quicktime_iods_t *iods)
{
	printf(" initial object descriptor\n");
	printf("  version %d\n", iods->version);
	printf("  flags %ld\n", iods->flags);
	printf("  audioProfileId %u\n", iods->audioProfileId);
	printf("  videoProfileId %u\n", iods->videoProfileId);
}

int quicktime_read_iods(quicktime_t *file, quicktime_iods_t *iods)
{
	iods->version = quicktime_read_char(file);
	iods->flags = quicktime_read_int24(file);
	quicktime_read_char(file); /* skip tag */
	quicktime_read_mp4_descr_length(file);	/* skip length */
	/* skip ODID, ODProfile, sceneProfile */
	quicktime_set_position(file, quicktime_position(file) + 4);
	iods->audioProfileId = quicktime_read_char(file);
	iods->videoProfileId = quicktime_read_char(file);
	/* will skip the remainder of the atom */
}

int quicktime_write_iods(quicktime_t *file, quicktime_iods_t *iods)
{
	quicktime_atom_t atom;
	int i;

	if (!file->use_mp4) {
		return 0;
	}

	quicktime_atom_write_header(file, &atom, "iods");

	quicktime_write_char(file, iods->version);
	quicktime_write_int24(file, iods->flags);

	quicktime_write_char(file, 0x10);	/* MP4_IOD_Tag */
	quicktime_write_char(file, 7 + (file->moov.total_tracks * (1+1+4)));	/* length */
	quicktime_write_int16(file, 0x004F); /* ObjectDescriptorID = 1 */
	quicktime_write_char(file, 0xFF);	/* ODProfileLevel */
	quicktime_write_char(file, 0xFF);	/* sceneProfileLevel */
	quicktime_write_char(file, iods->audioProfileId);	/* audioProfileLevel */
	quicktime_write_char(file, iods->videoProfileId);	/* videoProfileLevel */
	quicktime_write_char(file, 0xFF);	/* graphicsProfileLevel */

	for (i = 0; i < file->moov.total_tracks; i++) {
		quicktime_write_char(file, 0x0E);	/* ES_ID_IncTag */
		quicktime_write_char(file, 0x04);	/* length */
		quicktime_write_int32(file, file->moov.trak[i]->tkhd.track_id);	
	}

	/* no OCI_Descriptors */
	/* no IPMP_DescriptorPointers */
	/* no Extenstion_Descriptors */

	quicktime_atom_write_footer(file, &atom);
}


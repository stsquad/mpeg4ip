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


int quicktime_esds_init(quicktime_esds_t *esds)
{
	esds->version = 0;
	esds->flags = 0;
	esds->decoderConfigLen = 0;
	esds->decoderConfig = NULL;
	return 0;
}

int quicktime_esds_get_decoder_config(quicktime_esds_t* esds, u_char** ppBuf, int* pBufSize)
{
	if (esds->decoderConfig == NULL || esds->decoderConfigLen == 0) {
		*ppBuf = NULL;
		*pBufSize = 0;
	} else {
		*ppBuf = malloc(esds->decoderConfigLen);
		if (*ppBuf == NULL) {
			*pBufSize = 0;
			return 1;
		}
		memcpy(*ppBuf, esds->decoderConfig, esds->decoderConfigLen);
		*pBufSize = esds->decoderConfigLen;
	}
	return 0;
}

int quicktime_esds_set_decoder_config(quicktime_esds_t* esds, u_char* pBuf, int bufSize)
{
	free(esds->decoderConfig);
	esds->decoderConfig = malloc(bufSize);
	if (esds->decoderConfig) {
		memcpy(esds->decoderConfig, pBuf, bufSize);
		esds->decoderConfigLen = bufSize;
		return 0;
	}
	return 1;
}

int quicktime_esds_delete(quicktime_esds_t *esds)
{
	free(esds->decoderConfig);
	return 0;
}

int quicktime_esds_dump(quicktime_esds_t *esds)
{
	int i;

	printf("       elementary stream descriptor\n");
	printf("        version %d\n", esds->version);
	printf("        flags %ld\n", esds->flags);
	printf("        decoder config ");
	for (i = 0; i < esds->decoderConfigLen; i++) {	
		printf("%02x ", esds->decoderConfig[i]);
	}
	printf("\n");
}

int quicktime_read_esds(quicktime_t *file, quicktime_esds_t *esds)
{
	u_int8_t tag;
	u_int32_t length;

	esds->version = quicktime_read_char(file);
	esds->flags = quicktime_read_int24(file);

	/* get and verify ES_DescrTag */
	tag = quicktime_read_char(file);
	if (tag == 0x03) {
		/* read length */
		if (quicktime_read_mp4_descr_length(file) < 5 + 15) {
			return 1;
		}
		/* skip 3 bytes */
		quicktime_set_position(file, quicktime_position(file) + 3);
	} else {
		/* skip 2 bytes */
		quicktime_set_position(file, quicktime_position(file) + 2);
	}

	/* get and verify DecoderConfigDescrTab */
	if (quicktime_read_char(file) != 0x04) {
		return 1;
	}

	/* read length */
	if (quicktime_read_mp4_descr_length(file) < 15) {
		return 1;
	}

	/* skip 13 bytes */
	quicktime_set_position(file, quicktime_position(file) + 13);

	/* get and verify DecSpecificInfoTag */
	if (quicktime_read_char(file) != 0x05) {
		return 1;
	}

	/* read length */
	esds->decoderConfigLen = quicktime_read_mp4_descr_length(file); 

	free(esds->decoderConfig);
	esds->decoderConfig = malloc(esds->decoderConfigLen);
	if (esds->decoderConfig) {
		quicktime_read_data(file, esds->decoderConfig, esds->decoderConfigLen);
	} else {
		esds->decoderConfigLen = 0;
	}

	/* will skip the remainder of the atom */
	return 0;
}

int quicktime_write_esds_common(quicktime_t *file, quicktime_esds_t *esds, int esid, u_int objectType, u_int streamType)
{
	quicktime_atom_t atom;

	if (!file->use_mp4) {
		return 0;
	}

	quicktime_atom_write_header(file, &atom, "esds");

	quicktime_write_char(file, esds->version);
	quicktime_write_int24(file, esds->flags);

	quicktime_write_char(file, 0x03);	/* ES_DescrTag */
	quicktime_write_mp4_descr_length(file, 
		3 + (5 + (13 + (5 + esds->decoderConfigLen))) + 3, FALSE);

	quicktime_write_int16(file, esid);
	quicktime_write_char(file, 0x10);	/* streamPriorty = 16 (0-31) */

	/* DecoderConfigDescriptor */
	quicktime_write_char(file, 0x04);	/* DecoderConfigDescrTag */
	quicktime_write_mp4_descr_length(file, 
		13 + (5 + esds->decoderConfigLen), FALSE);

	quicktime_write_char(file, objectType); /* objectTypeIndication */
	quicktime_write_char(file, streamType); /* streamType */

	quicktime_write_int24(file, 0);		/* buffer size */
	quicktime_write_int32(file, 0);		/* max bitrate */
	quicktime_write_int32(file, 0);		/* average bitrate */

	quicktime_write_char(file, 0x05);	/* DecSpecificInfoTag */
	quicktime_write_mp4_descr_length(file, esds->decoderConfigLen, FALSE);
	quicktime_write_data(file, esds->decoderConfig, esds->decoderConfigLen);

	/* SLConfigDescriptor */
	quicktime_write_char(file, 0x06);	/* SLConfigDescrTag */
	quicktime_write_char(file, 0x01);	/* length */
	quicktime_write_char(file, 0x02);	/* constant in mp4 files */

	/* no IPI_DescrPointer */
	/* no IP_IdentificationDataSet */
	/* no IPMP_DescriptorPointer */
	/* no LanguageDescriptor */
	/* no QoS_Descriptor */
	/* no RegistrationDescriptor */
	/* no ExtensionDescriptor */

	quicktime_atom_write_footer(file, &atom);
}

int quicktime_write_esds_audio(quicktime_t *file, quicktime_esds_t *esds, int esid)
{
	return quicktime_write_esds_common(file, esds, esid, (u_int)0x40, (u_int)0x05);
}

int quicktime_write_esds_video(quicktime_t *file, quicktime_esds_t *esds, int esid)
{
	return quicktime_write_esds_common(file, esds, esid, (u_int)0x20, (u_int)0x04);
}


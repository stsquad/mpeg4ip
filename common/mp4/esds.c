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
		esds->decoderConfig = bufSize;
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
	esds->version = quicktime_read_char(file);
	esds->flags = quicktime_read_int24(file);

	/* skip 3 bytes */
	quicktime_set_position(file, quicktime_position(file) + 3);

	/* get and verify DecoderConfigDescrTab */
	if (quicktime_read_char(file) == 0x04) {
		/* get length of 1-4 bytes */
		u_int8_t b, bcount = 0;
		u_int32_t length = 0;

		/* TBD create a function to do this */
		do {
			b = quicktime_read_char(file);
			bcount++;
			length = (length << 7) | (b & 0x7F);
		} while ((b & 0x80) && bcount <= 4);

		/* read it */
		esds->decoderConfigLen = length; 
		free(esds->decoderConfig);
		esds->decoderConfig = malloc(esds->decoderConfigLen);
		if (esds->decoderConfig) {
			quicktime_read_data(file, esds->decoderConfig, 
				esds->decoderConfigLen);
		} else {
			esds->decoderConfigLen = 0;
		}
	}

	/* will skip the remainder of the atom */
}

int quicktime_write_esds(quicktime_t *file, quicktime_esds_t *esds, int esid)
{
	quicktime_atom_t atom;

	if (!file->use_mp4) {
		return 0;
	}

	quicktime_atom_write_header(file, &atom, "esds");

	quicktime_write_char(file, esds->version);
	quicktime_write_int24(file, esds->flags);

	quicktime_write_int16(file, esid);
	quicktime_write_char(file, 0x2F);	/* streamPriorty = 16 (0-31) */

	/* DecoderConfigDescriptor */
	quicktime_write_char(file, 0x04);	/* DecoderConfigDescrTag */
	/* TBD create a function to do this */
	{
	u_int8_t b;
	u_int32_t length = esds->decoderConfigLen;
	
	do {
		b = length & 0x7F;
		length >>= 7;
		if (length) {
			b |= 0x80;
		}
		quicktime_write_char(file, b);
	} while (length);
	}
	quicktime_write_data(file, esds->decoderConfig, esds->decoderConfigLen);

	/* SLConfigDescriptor */
	quicktime_write_char(file, 0x06);	/* SLConfigDescrTag */
	quicktime_write_char(file, 0x01);	/* length */
	quicktime_write_char(file, 0x7F);	/* no SL */

	/* no IPI_DescrPointer */
	/* no IP_IdentificationDataSet */
	/* no IPMP_DescriptorPointer */
	/* no LanguageDescriptor */
	/* no QoS_Descriptor */
	/* no RegistrationDescriptor */
	/* no ExtensionDescriptor */

	quicktime_atom_write_footer(file, &atom);
}

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

#include <sys/types.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <getopt.h>
#include <fcntl.h>
#include <unistd.h>
#include <mpeg4ip.h>

typedef struct {
	u_int8_t*	pData;
	u_int32_t	numbits;
	u_int32_t	numbytes;
} BitBuffer; 

/* globals */
char* progName;

/* forward declarations */
static bool init_putbits(BitBuffer* pBitBuf, u_int32_t maxNumBits);
static bool putbits(BitBuffer* pBitBuf, u_int32_t dataBits, u_int8_t numBits);
static void destroy_putbits(BitBuffer* pBitBuf);

static char* binaryToAscii(u_int8_t* pBuf, u_int32_t bufSize, u_int32_t spaces);

/*
 * mp4vhdr
 */ 
int main(int argc, char** argv)
{
	/* command line controllable variables */
	bool want_ascii = FALSE;		/* --ascii */
	u_int32_t asciiSpaces = 1;		/* --ascii-spaces=<uint> */
	bool want_binary = TRUE;		/* --binary */
	bool want_head = TRUE;			/* --head */
	bool want_tail = FALSE;			/* --tail */
	bool want_vosh = FALSE;			/* --vosh=[0|1] */
	bool want_vo = TRUE;			/* --vo=[0|1] */
	bool want_vol = TRUE;			/* --vol=[0|1] */
	bool want_short_time = FALSE;	/* --short-time */
	bool want_variable_rate = FALSE;/* --variable-rate */
	u_int16_t frameWidth = 320;		/* --width=<uint> */
	u_int16_t frameHeight = 240;	/* --height=<uint> */
	float frameRate = 30.0;			/* --rate=<float> */
	u_int8_t profileId = 1;			/* --profile=<uint> */
	u_int8_t profileLevelId = 3;	/* --profile-level=<uint> */

	/* internal variables */
	char* inFileName = NULL;
	FILE* fin = NULL;
	FILE* fout = stdout;

	/* begin process command line */
	progName = argv[0];
	while (1) {
		int c = -1;
		int option_index = 0;
		static struct option long_options[] = {
			{ "ascii", 0, 0, 0x81 },
			{ "ascii-spaces", 1, 0, 0x8B },
			{ "all", 0, 0, 0x82 },
			{ "binary", 0, 0, 0x83 },
			{ "height", 1, 0, 'h' },
			{ "head", 0, 0, 0x84 },
			{ "infile", 1, 0, 'i' },
			{ "profile", 1, 0, 'p' },
			{ "rate", 1, 0, 'r' },
			{ "short-time", 0, 0, 0x89 },
			{ "tail", 0, 0, 0x85  },
			{ "variable-rate", 0, 0, 0x8A },
			{ "vosh", 1, 0, 0x86 },
			{ "vo", 1, 0, 0x87  },
			{ "vol", 1, 0, 0x88 },
			{ "width", 1, 0, 'w' },
			{ NULL, 0, 0, 0 }
		};

		c = getopt_long_only(argc, argv, "h:i:p:r:w:",
			long_options, &option_index);

		if (c == -1)
			break;

		switch (c) {
		case 0x81: {
			/* --ascii */
			want_ascii = TRUE;
			want_binary = FALSE;
			break;
		}
		case 0x82: {
			/* --all */
			want_vosh = TRUE;
			want_vo = TRUE;
			want_vol = TRUE;
			break;
		}
		case 0x83: {
			/* --binary */
			want_binary = TRUE;
			want_ascii = FALSE;
			break;
		}
		case 0x84: {
			/* --head */
			want_head = TRUE;
			break;
		}
		case 0x85: {
			/* --tail */
			want_tail = TRUE;
			break;
		}
		case 0x86: {
			/* --vosh=[0|1] */
			u_int i;
			if (sscanf(optarg, "%u", &i) == 1) {
				want_vosh = i ? TRUE : FALSE;
			}
			break;
		}
		case 0x87: {
			/* --vo=[0|1] */
			u_int i;
			if (sscanf(optarg, "%u", &i) == 1) {
				want_vo = i ? TRUE : FALSE;
			}
			break;
		}
		case 0x88: {
			/* --vol=[0|1] */
			u_int i;
			if (sscanf(optarg, "%u", &i) == 1) {
				want_vol = i ? TRUE : FALSE;
			}
			break;
		}
		case 0x89: {
			/* --short-time */
			want_short_time = TRUE;
			break;
		}
		case 0x8A: {
			/* --variable-rate */
			want_variable_rate = TRUE;
			break;
		}
		case 0x8B: {
			/* --ascii-spaces=<uint> */
			u_int i;
			if (sscanf(optarg, "%u", &i) < 1) {
				fprintf(stderr, 
					"%s: bad ascii-spaces specified: %s\n",
					 progName, optarg);
			} else {
				/* currently no range checking */
				asciiSpaces = i;
			}
			break;
		}
		case 'h': {
			/* --height <pixels> */
			u_int i;
			if (sscanf(optarg, "%u", &i) < 1) {
				fprintf(stderr, 
					"%s: bad height specified: %s\n",
					 progName, optarg);
			} else {
				/* currently no range checking */
				frameHeight = i;
			}
			break;
		}
		case 'i': {
			inFileName = optarg;
			break;
		}
		case 'p': {
			/* -profile <id> */
			u_int i;
			if (sscanf(optarg, "%u", &i) < 1) {
				fprintf(stderr, 
					"%s: bad profile specified: %s\n",
					 progName, optarg);
			} else if (i > 255) {
				fprintf(stderr,
					"%s: profile must be less than 256\n",
					progName);
			} else {
				/*
				 * currently we do not validate 
				 * that the given profile id is defined by MPEG
				 * since this is constantly evolving
				 */ 
				profileId = i;
			}
			break;
		}
		case 'r': {
			/* -rate <fps> */
			float f;
			if (sscanf(optarg, "%f", &f) < 1) {
				fprintf(stderr, 
					"%s: bad rate specified: %s\n",
					 progName, optarg);
			} else if (f < 1.0 || f > 30.0) {
				fprintf(stderr,
					"%s: rate must be between 1 and 30\n",
					progName);
			} else {
				frameRate = f;
			}
			break;
		}
		case 'w': {
			/* -width <pixels> */
			u_int i;
			if (sscanf(optarg, "%u", &i) < 1) {
				fprintf(stderr, 
					"%s: bad width specified: %s\n",
					 progName, optarg);
			} else {
				/* currently no range checking */
				frameWidth = i;
			}
			break;
		}
		case '?':
			break;
		default:
			fprintf(stderr, "%s: unknown option specified, ignoring: %c\n", 
				progName, c);
		}
	}

	/* warn about extraneous non-option arguments */
	if (optind < argc) {
		fprintf(stderr, "%s: unknown options specified, ignoring: ", progName);
		while (optind < argc) {
			fprintf(stderr, "%s ", argv[optind++]);
		}
		fprintf(stderr, "\n");
	}

	/* end processing of command line */


	if (want_head) {
		if (want_vosh) {
			/* VOSH - Visual Object Sequence Header */
			u_int8_t voshStart[5] = { 
				0x00, 0x00, 0x01, 0xB0,		/* VOSH start code */
				/* profile_level_id, default is 3, Simple Profile @ Level 3 */
			};
			voshStart[4] = profileLevelId;

			if (want_binary) {
				fwrite(voshStart, 1, sizeof(voshStart), fout);
			} else { /* want_ascii */
				char *s = binaryToAscii(voshStart, sizeof(voshStart), asciiSpaces);
				fputs(s, fout);
				free(s);
			}
		}

		if (want_vo) {
			/* VO - Visual Object */
			u_int8_t vo[9] = { 
				0x00, 0x00, 0x01, 0xB5,	/* VO */ 
				0x08,					/* no verid, priority or signal type */
				0x00, 0x00, 0x01, 0x00	/* video object 1 */ 
			};

			if (want_binary) {
				fwrite(vo, 1, sizeof(vo), fout);
			} else { /* want_ascii */
				char *s = binaryToAscii(vo, sizeof(vo), asciiSpaces);
				fputs(s, fout);
				free(s);
			}
		}
	
		if (want_vol) {
			/* VOL - Video Object Layer */
			u_int8_t i = 4;
			u_int16_t ticks;
			BitBuffer vol;

			init_putbits(&vol, 16 * 8);

			/* VOL 1 */
			putbits(&vol, 0x00000120, 32);
			/* 1 bit - random access = 0 (1 only if every VOP is an I frame) */
			putbits(&vol, 0, 1);
			/*
			 * 8 bits - type indication 
			 * 		= 1 (simple profile)
			 * 		= 4 (main profile)
			 */
			putbits(&vol, profileId, 8);
			/* 1 bit - is object layer id = 0 */
			putbits(&vol, 0, 1);
			/* 4 bits - aspect ratio info = 1 (square pixels) */
			putbits(&vol, 1, 4);
			/* 1 bit - VOL control params = 0 */
			putbits(&vol, 0, 1);
			/* 2 bits - VOL shape = 0 (rectangular) */
			putbits(&vol, 0, 2);
			/* 1 bit - marker = 1 */
			putbits(&vol, 1, 1);
			if (want_short_time && frameRate == (float)((int)frameRate)) {
				ticks = (u_int16_t)frameRate;
			} else {
				ticks = 30000;
			}
			/* 16 bits - VOP time increment resolution */
			putbits(&vol, ticks, 16);
			/* 1 bit - marker = 1 */
			putbits(&vol, 1, 1);
			/* 1 bit - fixed vop rate = 0 or 1 */
			if (want_variable_rate) {
				putbits(&vol, 0, 1);
			} else {
				u_int16_t frameDuration = 
					(u_int16_t)((float)ticks / frameRate);
				u_int8_t rangeBits = 0;

				putbits(&vol, 1, 1);

				/* 1-16 bits - fixed vop time increment in ticks */
				while (ticks >= (1 << rangeBits)) {
					rangeBits++;
				}
				putbits(&vol, frameDuration, rangeBits);
			}
			/* 1 bit - marker = 1 */
			putbits(&vol, 1, 1);
			/* 13 bits - VOL width */
			putbits(&vol, frameWidth, 13);
			/* 1 bit - marker = 1 */
			putbits(&vol, 1, 1);
			/* 13 bits - VOL height */
			putbits(&vol, frameHeight, 13);
			/* 1 bit - marker = 1 */
			putbits(&vol, 1, 1);
			/* 1 bit - interlaced = 0 */
			putbits(&vol, 0, 1);
			/* 1 bit - overlapped block motion compensation disable = 1 */
			putbits(&vol, 1, 1);
			/* 1 bits - sprite usage = 0 */
			putbits(&vol, 0, 1);
			/* 1 bit - not 8 bit pixels = 0 */
			putbits(&vol, 0, 1);
			/* 1 bit - quant type = 0 */
			putbits(&vol, 0, 1);
			/* 1 bit - complexity estimation disable = 1 */
			putbits(&vol, 1, 1);
			/* 1 bit - resync marker disable = 0 */
			putbits(&vol, 0, 1);
			/* 1 bit - data partitioned = 0 */
			putbits(&vol, 0, 1);
			/* 1 bit - scalability = 0 */
			putbits(&vol, 0, 1);

			/* pad to byte boundary with 0 then 1's */
			if ((vol.numbits % 8) != 0) {
				putbits(&vol, 0, 1);
				putbits(&vol, 0xFF, 8 - (vol.numbits % 8));
			}

			if (want_binary) {
				fwrite(vol.pData, 1, vol.numbits / 8, fout);
			} else { /* want_ascii */
				char *s = binaryToAscii(vol.pData, vol.numbits / 8, asciiSpaces);
				fputs(s, fout);
				free(s);
			}

			destroy_putbits(&vol);
		}
	}

	if (inFileName) {
		u_int8_t buf[8*1024];
		int numBytes;

		if ((fin = fopen(inFileName, "r")) == NULL) {
			fprintf(stderr,
				"%s: can't open %s: %s\n",
				progName, inFileName, strerror(errno));
			exit(1);
		}
		while ((numBytes = fread(buf, 1, sizeof(buf), fin)) > 0) {
			if (want_binary) {
				fwrite(buf, 1, numBytes, fout);
			} else { /* want_ascii */
				char *s = binaryToAscii(buf, numBytes, asciiSpaces);
				fputs(s, fout);
				free(s);
			}
		}
	}
	
	if (want_tail) {
		if (want_vosh) {
			u_int8_t voshEnd[] = {
				0x00, 0x00, 0x01, 0xB1	 /* VOSH end code */
			};

			if (want_binary) {
				fwrite(voshEnd, 1, sizeof(voshEnd), fout);
			} else { /* want_ascii */
				char *s = binaryToAscii(voshEnd, sizeof(voshEnd), asciiSpaces);
				fputs(s, fout);
				free(s);
			}
		}
	}

	if (want_ascii) {
		fputs("\n", fout);
	}

	exit(0);
}

static bool init_putbits(BitBuffer* pBitBuf, u_int32_t maxNumBits)
{
	pBitBuf->pData = (u_int8_t*)malloc((maxNumBits / 8) + 1);
	pBitBuf->pData[0] = 0;
	pBitBuf->numbits = 0;
}

static bool putbits(BitBuffer* pBitBuf, u_int32_t dataBits, u_int8_t numBits)
{
	u_int32_t byteIndex;
	u_int8_t freebits;
	int i;

	if (numBits == 0) {
		return TRUE;
	}
	if (numBits > 32) {
		return FALSE;
	}
	
	byteIndex = (pBitBuf->numbits / 8);
	freebits = 8 - (pBitBuf->numbits % 8);
	for (i = numBits; i > 0; i--) {
		pBitBuf->pData[byteIndex] |= 
			(((dataBits >> (i - 1)) & 1) << (--freebits));
		if (freebits == 0) {
			pBitBuf->pData[++byteIndex] = 0;
			freebits = 8;
		}
	}

	pBitBuf->numbits += numBits;
	return TRUE;
}

static void destroy_putbits(BitBuffer* pBitBuf)
{
	free(pBitBuf->pData);
	pBitBuf->pData = NULL;
	pBitBuf->numbits = 0;	
}

static char* binaryToAscii(u_int8_t* pBuf, u_int32_t bufSize, u_int32_t spaces)
{
	char* s;
	u_int i, j;

	if (spaces) {
		s = (char*)malloc((2 * bufSize) + (bufSize / spaces) + 1);
	} else {
		s = (char*)malloc((2 * bufSize) + 1);
	}
	if (s == NULL) {
		return NULL;
	}

	for (i = 0, j = 0; i < bufSize; i++) {
		sprintf(&s[j], "%02x", pBuf[i]);
		j += 2;
		if (spaces > 0 && (i % spaces) == 0) {
			sprintf(&s[j], " ");
			j++;
		}
	}

	return s;	/* N.B. caller is responsible for free'ing s */
}

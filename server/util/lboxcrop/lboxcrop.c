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

/* 
 * Notes:
 *  - file formatted with tabstops == 4 spaces 
 *  - TBD == "To Be Done" 
 */

#include <sys/types.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <getopt.h>
#include <fcntl.h>
#include <unistd.h>
#include <math.h>
#include <mpeg4ip.h>

/* globals */
char* progName = NULL;
FILE* inFile = NULL;
FILE* outFile = NULL;
u_int8_t* planeBuf = NULL;

/* forward declarations */
static int cropPlane(u_int32_t inSize, u_int32_t outOffset, u_int32_t outSize);

/*
 * lboxcrop
 * required arg1 should be the raw input file to be cropped
 * required arg2 should be the raw output file
 */ 
int main(int argc, char** argv)
{
	/* variables controlable from command line */
	float aspectRatio = 2.35;		/* --aspect=<float> */
	u_int frameWidth = 320;			/* --width=<uint> */
	u_int frameHeight = 240;		/* --height=<uint> */

	/* internal variables */
	char* inFileName = NULL;
	char* outFileName = NULL;
	u_int32_t inYSize, inUVSize;
	u_int32_t outYStart, outYSize, outUVStart, outUVSize;
	u_int32_t cropHeight;
	u_int32_t frameNumber = 0;

	/* begin process command line */
	progName = argv[0];
	while (1) {
		int c = -1;
		int option_index = 0;
		static struct option long_options[] = {
			{ "aspect", 1, 0, 'a' },
			{ "height", 1, 0, 'h' },
			{ "width", 1, 0, 'w' },
			{ NULL, 0, 0, 0 }
		};

		c = getopt_long_only(argc, argv, "a:h:w:",
			long_options, &option_index);

		if (c == -1)
			break;

		/* TBD a help option would be nice */

		switch (c) {
		case 'a': {
			/* --aspect <ratio> */
			float f;
			if (sscanf(optarg, "%f", &f) < 1) {
				fprintf(stderr, 
					"%s: bad aspect ratio specified: %s\n",
					 progName, optarg);
			} else {
				aspectRatio = f;
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

	/* check that we have at least two non-option arguments */
	if ((argc - optind) < 2) {
		fprintf(stderr, 
			"usage: %s <infile> <outfile>\n",
			progName);
		exit(1);
	}

	/* point to the specified file names */
	inFileName = argv[optind++];
	outFileName = argv[optind++];

	/* warn about extraneous non-option arguments */
	if (optind < argc) {
		fprintf(stderr, "%s: unknown options specified, ignoring: ", progName);
		while (optind < argc) {
			fprintf(stderr, "%s ", argv[optind++]);
		}
		fprintf(stderr, "\n");
	}

	/* end processing of command line */

	/* open the raw input file for reading */
	inFile = fopen(inFileName, "r");
	if (inFile == NULL) {
		/* error, file doesn't exist or we can't read it */
		fprintf(stderr,
			"%s: error opening %s: %s\n",
			progName, inFileName, strerror(errno));
		exit(2);
	}
	
	/* open the raw output file for writing */
	outFile = fopen(outFileName, "w");
	if (outFile == NULL) {
		/* error, file doesn't exist or we can't read it */
		fprintf(stderr,
			"%s: error opening %s: %s\n",
			progName, outFileName, strerror(errno));
		exit(3);
	}

	/* calculate the necessary sizes */
	inYSize = frameWidth * frameHeight;
	inUVSize = inYSize >> 2;
	cropHeight = (float)frameWidth / aspectRatio;
	/* encoders need sizes to be a multiple of 16 */
	if ((cropHeight % 16) != 0) {
		cropHeight += 16 - (cropHeight % 16);
	}
	outYStart = frameWidth * ((frameHeight - cropHeight) >> 1);
	outYSize = frameWidth * cropHeight;
	outUVStart = outYStart >> 2;
	outUVSize = outYSize >> 2;

	printf("Input size %ux%u, output size %ux%u\n",
		frameWidth, frameHeight, frameWidth, cropHeight);

	planeBuf = (u_int8_t *)malloc(inYSize);

	while (!feof(inFile)) {
		int i, rc;
	
		frameNumber++;

		rc = cropPlane(inYSize, outYStart, outYSize);
		switch (rc) {
		case 0:
			goto done;
		case -1:
			goto read_error;
		case -2:
			goto write_error;
		}

		for (i = 0; i < 2; i++) {
			rc = cropPlane(inUVSize, outUVStart, outUVSize);
			switch (rc) {
			case 0:
			case -1:
				goto read_error;
			case -2:
				goto write_error;
			}
		}
	}
	goto done;

read_error:
	fprintf(stderr, 
		"%s: read error %s on frame %u: %s\n",
		progName, inFileName, frameNumber, strerror(errno));
	goto done;

write_error:
	fprintf(stderr, 
		"%s: write error %s on frame %u: %s\n",
		progName, outFileName, frameNumber, strerror(errno));
	goto done;

done:	
	/* cleanup */
	fclose(inFile);
	fclose(outFile);
	exit(0);
}

static int cropPlane(u_int32_t inSize, u_int32_t outOffset, u_int32_t outSize)
{
	int rc;

	/* read Y plane */
	rc = fread(&planeBuf[0], sizeof(u_int8_t), inSize, inFile);
	if (rc == 0) {
		return 0;
	}
	if (rc != inSize) {
		return -1;
	}

	/* write cropped frame to output file */
	rc = fwrite(&planeBuf[outOffset], 1, outSize, outFile);
	if (rc != outSize) {
		return -2;
	}

	return 1;
}


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

#include <mpeg4ip.h>
#include <mpeg4ip_getopt.h>
#include "rgb2yuv.h"


/* globals */
char* progName;

/*
 * rgb2yuv
 * required arg1 should be the input RAW RGB24 file
 * required arg2 should be the output RAW YUV12 file
 */ 
static const char *usage = 
"\t--flip           - flip image\n"
"\t--height <value> - specify height\n"
"\t--width <value>  - specify width\n"
"\t--version        - display version\n";

int main(int argc, char** argv)
{
	/* variables controlable from command line */
	u_int frameWidth = 320;			/* --width=<uint> */
	u_int frameHeight = 240;		/* --height=<uint> */
	bool flip = FALSE;				/* --flip */

	/* internal variables */
	char* rgbFileName = NULL;
	char* yuvFileName = NULL;
	FILE* rgbFile = NULL;
	FILE* yuvFile = NULL;
	u_int8_t* rgbBuf = NULL;
	u_int8_t* yBuf = NULL;
	u_int8_t* uBuf = NULL;
	u_int8_t* vBuf = NULL;
	u_int32_t videoFramesWritten = 0;
	bool gotheight, gotwidth;

	gotheight = gotwidth = FALSE;

	/* begin process command line */
	progName = argv[0];
	while (1) {
		int c = -1;
		int option_index = 0;
		static struct option long_options[] = {
			{ "flip", 0, 0, 'f' },
			{ "height", 1, 0, 'h' },
			{ "width", 1, 0, 'w' },
			{ "version", 0, 0, 'V' },
			{ NULL, 0, 0, 0 }
		};

		c = getopt_long_only(argc, argv, "fh:w:V",
			long_options, &option_index);

		if (c == -1)
			break;

		switch (c) {
		case 'f': {
			flip = TRUE;
			break;
		}
		case 'h': {
			/* --height <pixels> */
			u_int i;
			gotheight = TRUE;
			if (sscanf(optarg, "%u", &i) < 1) {
				fprintf(stderr, 
					"%s: bad height specified: %s\n",
					 progName, optarg);
			} else if (i & 1) {
				fprintf(stderr, 
					"%s: bad height specified, must be multiple of 2: %s\n",
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
			gotwidth = TRUE;
			if (sscanf(optarg, "%u", &i) < 1) {
				fprintf(stderr, 
					"%s: bad width specified: %s\n",
					 progName, optarg);
			} else if (i & 1) {
				fprintf(stderr, 
					"%s: bad width specified, must be multiple of 2: %s\n",
					 progName, optarg);
			} else {
				/* currently no range checking */
				frameWidth = i;
			}
			break;
		}
		case '?':
		  fprintf(stderr, 
			  "usage: %s <rgb-file> <yuv-file>\n%s",
			  progName, usage);
		  return (0);
		case 'V':
		  fprintf(stderr, "%s - %s version %s\n",
			  progName, MPEG4IP_PACKAGE, MPEG4IP_VERSION);
		  return (0);
		default:
			fprintf(stderr, "%s: unknown option specified, ignoring: %c\n", 
				progName, c);
		}
	}

	/* check that we have at least two non-option arguments */
	if ((argc - optind) < 2) {
		fprintf(stderr, 
			"usage: %s <rgb-file> <yuv-file>\n%s",
			progName, usage);
		exit(1);
	}

	if (gotheight == FALSE || gotwidth == FALSE) {
	  fprintf(stderr, "%s - you haven't specified height or width - going with %dx%d", 
		  progName, frameWidth, frameHeight);
	}
	/* point to the specified file names */
	rgbFileName = argv[optind++];
	yuvFileName = argv[optind++];

	/* warn about extraneous non-option arguments */
	if (optind < argc) {
		fprintf(stderr, "%s: unknown options specified, ignoring: ");
		while (optind < argc) {
			fprintf(stderr, "%s ", argv[optind++]);
		}
		fprintf(stderr, "\n");
	}

	/* end processing of command line */

	/* open the RGB file */
	rgbFile = fopen(rgbFileName, "rb");
	if (rgbFile == NULL) {
		fprintf(stderr, 
			"%s: error %s: %s\n",
			progName, rgbFileName, strerror(errno));
		exit(4);
	}

	/* open the RAW file */
	yuvFile = fopen(yuvFileName, "wb");
	if (yuvFile == NULL) {
		fprintf(stderr,
			"%s: error opening %s: %s\n",
			progName, yuvFileName, strerror(errno));
		exit(5);
	}

	/* get an input buffer for a frame */
	rgbBuf = (u_int8_t*)malloc(frameWidth * frameHeight * 3);

	/* get the output buffers for a frame */
	yBuf = (u_int8_t*)malloc(frameWidth * frameHeight);
	uBuf = (u_int8_t*)malloc((frameWidth * frameHeight) / 4);
	vBuf = (u_int8_t*)malloc((frameWidth * frameHeight) / 4);

	if (rgbBuf == NULL || yBuf == NULL || uBuf == NULL || vBuf == NULL) {
		fprintf(stderr,
			"%s: error allocating memory: %s\n",
			progName, strerror(errno));
		exit(6);
	}

	while (fread(rgbBuf, 1, frameWidth * frameHeight * 3, rgbFile)) {

		RGB2YUV(frameWidth, frameHeight, rgbBuf, yBuf, uBuf, vBuf, flip);

		fwrite(yBuf, 1, frameWidth * frameHeight, yuvFile);
		fwrite(uBuf, 1, (frameWidth * frameHeight) / 4, yuvFile);
		fwrite(vBuf, 1, (frameWidth * frameHeight) / 4, yuvFile);

		videoFramesWritten++;
	}

	printf("%u %ux%u video frames written\n", 
		videoFramesWritten, frameWidth, frameHeight);

	/* cleanup */
	fclose(rgbFile);
	fclose(yuvFile);

	return(0);
}


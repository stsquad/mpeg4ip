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
 * Copyright (C) Cisco Systems Inc. 2000-2002.  All Rights Reserved.
 * 
 * Contributor(s): 
 *		Dave Mackie		dmackie@cisco.com
 */

/* 
 * Notes:
 *  - file formatted with tabstops == 4 spaces 
 */

#include <mpeg4ip.h>
#include <sys/types.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <mpeg4ip_getopt.h>
#include <fcntl.h>
#include <math.h>
#include <time.h>
#include <divx4.h>

/* globals */
char* progName;

/*
 * xvidenc
 * required arg1 should be the raw file to be encoded
 * required arg2 should be the target encoded file (MPEG-4 ES)
 */ 
int main(int argc, char** argv)
{
	/* variables controlable from command line */
	u_int bitRate = 500000;			/* --bitrate=<uint> */
	u_int frameWidth = 320;			/* --width=<uint> */
	u_int frameHeight = 240;		/* --height=<uint> */
	float frameRate = 30.0;			/* --rate=<float> */
	u_int iFrameFrequency = 30;		/* --ifrequency=<uint> */
	int short_headers = 0;                  /* --shortheaders */

	/* internal variables */
	char* rawFileName = NULL;
	char* mpeg4FileName = NULL;
	FILE* rawFile = NULL;
	FILE* mpeg4File = NULL;
	void* myHandle = NULL;
	ENC_PARAM encParams;
	ENC_FRAME encFrame;
	ENC_RESULT encResult;
	u_int32_t yuvSize;
	u_int8_t encVopBuffer[128 * 1024];
	u_int32_t frameNumber = 0;
	time_t startTime;

	/* begin process command line */
	progName = argv[0];
	while (1) {
		int c = -1;
		int option_index = 0;
		static struct option long_options[] = {
			{ "bitrate", 1, 0, 'b' },
			{ "height", 1, 0, 'h' },
			{ "ifrequency", 1, 0, 'i' },
			{ "rate", 1, 0, 'r' },
			{ "width", 1, 0, 'w' },
			{ "shortheaders", 0, 0, 's' },
			{ NULL, 0, 0, 0 }
		};

		c = getopt_long_only(argc, argv, "b:h:i:r:w:s",
			long_options, &option_index);

		if (c == -1)
			break;

		switch (c) {
		case 'b': {
			/* --bitrate <Kbps> */
			u_int i;
			if (sscanf(optarg, "%u", &i) < 1) {
				fprintf(stderr, 
					"%s: bad bitrate specified: %s\n",
					 progName, optarg);
			} else {
				/* currently no range checking */
				bitRate = i * 1000;
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
			u_int i;
			if (sscanf(optarg, "%u", &i) < 1) {
				fprintf(stderr, 
					"%s: bad ifrequency specified: %s\n",
					 progName, optarg);
			} else {
				iFrameFrequency = i;
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
		case 's':
		  short_headers = 1;
		  break;
		case '?':
			break;
		default:
			fprintf(stderr, "%s: unknown option specified, ignoring: %c\n",
				progName, c);
		}
	}

	if (short_headers) {
	  switch (frameWidth) {
	  case 128:
	    if (frameHeight != 96) {
	      fprintf(stderr, "Illegal height %d with width of 128 - must be 96\n", frameHeight);
	      exit(-1);
	    }
	    break;
	  case 176:
	    if (frameHeight != 144) {
	      fprintf(stderr, "Illegal height %d with width of 176 - must be 144\n", frameHeight);
	      exit(-1);
	    }
	    break;
	  case 352:
	    if (frameHeight != 288) {
	      fprintf(stderr, "Illegal height %d with width of 352 - must be 288\n", frameHeight);
	      exit(-1);
	    }
	    break;
	  case 704:
	    if (frameHeight != 576) {
	      fprintf(stderr, "Illegal height %d with width of 704 - must be 576\n", frameHeight);
	      exit(-1);
	    }
	    break;
	  case 1408:
	    if (frameHeight != 1152) {
	      fprintf(stderr, "Illegal height %d with width of 1408 - must be 1152\n", frameHeight);
	      exit(-1);
	    }
	    break;
	  default:
	    fprintf(stderr, "Must have legal height/width for short headers\n");
	    exit(-1);
	  }
	}
	/* check that we have at least two non-option arguments */
	if ((argc - optind) < 2) {
		fprintf(stderr, 
			"usage: %s <raw-file> <mpeg4-file>\n",
			progName);
		exit(1);
	}

	/* point to the specified file names */
	rawFileName = argv[optind++];
	mpeg4FileName = argv[optind++];

	/* warn about extraneous non-option arguments */
	if (optind < argc) {
		fprintf(stderr, "%s: unknown options specified, ignoring: ", progName);
		while (optind < argc) {
			fprintf(stderr, "%s ", argv[optind++]);
		}
		fprintf(stderr, "\n");
	}

	/* end processing of command line */

	/* open raw file for reading */
	rawFile = fopen(rawFileName, "rb");
	if (rawFile == NULL) {
		/* error, file doesn't exist or we can't read it */
		fprintf(stderr,
			"%s: error opening %s: %s\n",
			progName, rawFileName, strerror(errno));
		exit(2);
	}
	
	/* open the MPEG-4 file for writing */
	mpeg4File = fopen(mpeg4FileName, "wb");
	if (mpeg4File == NULL) {
		fprintf(stderr, 
			"%s: error %s: %s\n",
			progName, mpeg4FileName, strerror(errno));
		fclose(rawFile);
		exit(4);
	}

	/* initialize encoder */
	encParams.x_dim = frameWidth;
	encParams.y_dim = frameHeight;
	encParams.framerate = frameRate;
	encParams.bitrate = bitRate;
	encParams.rc_period = 2000;
	encParams.rc_reaction_period = 10;
	encParams.rc_reaction_ratio = 20;
	encParams.max_key_interval = iFrameFrequency;
	encParams.use_bidirect = 0;
	encParams.deinterlace = 0;
	encParams.quality = 5;
	encParams.obmc = 0;
	encParams.handle = NULL;

	if (encore(myHandle, ENC_OPT_INIT, &encParams, NULL) != ENC_OK) {
		fprintf(stderr, 
			"%s: error: can't initialize encoder\n",
			progName);
		exit(5);
	}
	myHandle = encParams.handle;

	yuvSize = (frameWidth * frameHeight * 3) / 2;
	encFrame.image = (u_int8_t *)malloc(yuvSize);
	encFrame.bitstream = encVopBuffer;
	encFrame.colorspace = ENC_CSP_I420;
	encFrame.quant = 0;
	encFrame.intra = -1;
	encFrame.mvs = NULL;
	encFrame.general = 0;
	if (short_headers) {
	  encFrame.general |= DEC_SHORT_HEADERS;
	}
	startTime = time(0);

	while (!feof(rawFile)) {
		int rc;
	
		frameNumber++;

		/* read yuv data */
		rc = fread(encFrame.image, sizeof(u_int8_t), yuvSize, rawFile);
		if (rc == 0) {
			break;
		}
		if (rc != yuvSize) {
			fprintf(stderr, 
				"%s: read error %s on frame %d: %s\n",
				progName, rawFileName, frameNumber, strerror(errno));
			break;
		}

		encFrame.length = sizeof(encVopBuffer);

		/* call xvid encoder */
		rc = encore(myHandle, ENC_OPT_ENCODE, &encFrame, &encResult);

		if (rc != ENC_OK) {
			if (rc == ENC_MEMORY) {
				fprintf(stderr, 
					"%s: encode error %s: memory\n",
					progName, mpeg4FileName);
			} else if (rc == ENC_BAD_FORMAT) {
				fprintf(stderr, 
					"%s: encode error %s: bad format\n",
					progName, mpeg4FileName);
			} else {
				fprintf(stderr, 
					"%s: encode error %s: unknown error %d\n",
					progName, mpeg4FileName, rc);
			}
			break;
		}

		/* write results to output file */
		rc = fwrite(encFrame.bitstream, 1, encFrame.length, mpeg4File);

		if (rc != encFrame.length) {
			fprintf(stderr, 
				"%s: write error %s: %s\n",
				progName, mpeg4FileName, strerror(errno));
			break;
		}

		/* DEBUG
		printf("Encoding frame %u to %lu bytes", frameNumber, encFrame.length);
	    */

		if (frameNumber > 0 && (frameNumber % (10 * (int)frameRate)) == 0) {
			int elapsed = time(0) - startTime;
			if (elapsed > 0) {
				printf("Encoded %u seconds of video in %u seconds, %u fps\n", 
					(int)(frameNumber / frameRate),
					elapsed, 
					frameNumber / elapsed);
			}
		}
	}

	/* cleanup */
	encore(myHandle, ENC_OPT_RELEASE, NULL, NULL);
	fclose(rawFile);
	fclose(mpeg4File);
	return(0);
}


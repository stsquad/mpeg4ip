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
 * Copyright (C) Cisco Systems Inc. 2000-2003.  All Rights Reserved.
 * 
 * Contributor(s): 
 *		Dave Mackie		dmackie@cisco.com
 *              Bill May                wmay@cisco.com
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
#include <xvid.h>
#include <mp4av.h>
/* globals */
char* progName;

/*
 * xvidenc
 * required arg1 should be the raw file to be encoded
 * required arg2 should be the target encoded file (MPEG-4 ES)
 */ 
static const char *help_str = 
"usage: %s <yuv file> <output file>\n"
"--bitrate <brate>      target bit rate\n"
"--height <height>      video height\n"
"--width <height>       video width\n"
"--rate <fps>           frames per second\n"
"--ifrequency <ifreq>   iframe frequency in frames\n"
"--version              display version\n";

int main(int argc, char** argv)
{
  /* variables controlable from command line */
  u_int bitRate = 500000;			/* --bitrate=<uint> */
  u_int frameWidth = 320;			/* --width=<uint> */
  u_int frameHeight = 240;		/* --height=<uint> */
  float frameRate = 30.0;			/* --rate=<float> */
  u_int iFrameFrequency = 30;		/* --ifrequency=<uint> */

  /* internal variables */
  char* rawFileName = NULL;
  char* mpeg4FileName = NULL;
  FILE* rawFile = NULL;
  FILE* mpeg4File = NULL;
  XVID_INIT_PARAM xvidInitParams;
  XVID_ENC_PARAM xvidEncParams;
  XVID_ENC_FRAME xvidFrame;
  XVID_DEC_PICTURE decpict;
  XVID_ENC_STATS xvidResult;
  void *xvidHandle;
  u_int32_t yuvSize, ySize;
  u_int8_t encVopBuffer[128 * 1024];
  u_int32_t frameNumber = 0;
  time_t startTime;
  int rc;
  uint8_t *readbuffer;

  memset(&xvidInitParams, 0, sizeof(xvidInitParams));
  if (xvid_init(NULL, 0, &xvidInitParams, NULL) != XVID_ERR_OK) {
    fprintf(stderr, "%s: failed to initialize xvid\n", progName);
    exit(5);
  }

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
      { "version", 0, 0, 'v'},
      { NULL, 0, 0, 0 }
    };

    c = getopt_long_only(argc, argv, "b:h:i:r:w:v",
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
    case 'v':
      fprintf(stdout, "%s version %s\n", MPEG4IP_PACKAGE, MPEG4IP_VERSION);
      fprintf(stdout, "xvid api version %d\n", xvidInitParams.api_version);
      fprintf(stdout, "xvid core build %d\n", xvidInitParams.core_build);
      exit(0);

    default:
      fprintf(stderr, "%s: unknown option specified, ignoring: %c\n",
	      progName, c);
      break;
    case '?':
      fprintf(stderr, help_str, progName);
      exit(0);
    }
  }

  /* check that we have at least two non-option arguments */
  if ((argc - optind) < 2) {
    fprintf(stderr, help_str, progName);
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
  memset(&xvidEncParams, 0, sizeof(xvidEncParams));
  xvidEncParams.width = frameWidth;
  xvidEncParams.height = frameHeight;
  xvidEncParams.fincr = 1;
  xvidEncParams.fbase = (int)(frameRate + 0.5);
  xvidEncParams.rc_bitrate = bitRate;
  xvidEncParams.min_quantizer = 1;
  xvidEncParams.max_quantizer = 31;
  xvidEncParams.max_key_interval = iFrameFrequency;
  if (xvidEncParams.max_key_interval == 0) 
    xvidEncParams.max_key_interval = 1;
  if (xvid_encore(NULL, XVID_ENC_CREATE, &xvidEncParams, NULL) != XVID_ERR_OK) {
    fprintf(stderr, "%s: failed to initialize xvid encoder params\n",
	    progName);
    exit(6);
  }
  xvidHandle = xvidEncParams.handle;
  memset(&xvidFrame, 0, sizeof(xvidFrame));

  xvidFrame.general = XVID_HALFPEL | XVID_H263QUANT;
  xvidFrame.motion = 
    PMV_EARLYSTOP16 | PMV_HALFPELREFINE16 
    | PMV_EARLYSTOP8 | PMV_HALFPELDIAMOND8;

  xvidFrame.general |= XVID_INTER4V;
  xvidFrame.quant = 0;

  ySize = frameWidth * frameHeight;
  yuvSize = (ySize * 3) / 2;
  readbuffer = (uint8_t *)malloc(yuvSize);
  decpict.y = readbuffer;
  decpict.u = decpict.y + ySize;
  decpict.v = decpict.u + ySize / 4;
  decpict.stride_y = frameWidth;
  decpict.stride_u = frameWidth / 2;
  xvidFrame.image = &decpict;
  xvidFrame.colorspace = XVID_CSP_USER;

  
  startTime = time(0);

  while (!feof(rawFile)) {
	
    frameNumber++;

    /* read yuv data */
    rc = fread(readbuffer, sizeof(u_int8_t), yuvSize, rawFile);

    if (rc == 0) {
      break;
    }
    if (rc == -1) {
      fprintf(stderr, 
	      "%s: read error %s on frame %d: %s\n",
	      progName, rawFileName, frameNumber, strerror(errno));
      break;
    }
    if (rc != yuvSize) {
      fprintf(stderr, 
	      "%s: read error %s on frame %d: too few bytes, expected %d, got %d\n",
	      progName, rawFileName, frameNumber, yuvSize, rc);
      break;
    }

    xvidFrame.bitstream = encVopBuffer;
    xvidFrame.intra = -1;
    //		encFrame.length = sizeof(encVopBuffer);

    /* call xvid encoder */
    rc = xvid_encore(xvidHandle, XVID_ENC_ENCODE, &xvidFrame, &xvidResult);

    if (rc != XVID_ERR_OK) {
      fprintf(stderr, 
	      "%s: encode error %s: unknown error %d\n",
	      progName, mpeg4FileName, rc);
    }

    /* write results to output file */
    if (frameNumber == 1 &&
	encVopBuffer[3] != MP4AV_MPEG4_VOSH_START) {
      static uint8_t vosh[] = { 0, 0, 1, MP4AV_MPEG4_VOSH_START,MPEG4_SP_L3,
				0, 0, 1, MP4AV_MPEG4_VO_START, 0x08 };
      fwrite(vosh, 1, sizeof(vosh), mpeg4File);
    }
    rc = fwrite(encVopBuffer, 1, xvidFrame.length, mpeg4File);

    if (rc != xvidFrame.length) {
      fprintf(stderr, 
	      "%s: write error %s: %s\n",
	      progName, mpeg4FileName, strerror(errno));
			
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
  xvid_encore(xvidHandle, XVID_ENC_DESTROY, NULL, NULL);
  fclose(rawFile);
  fclose(mpeg4File);
  return(0);
}


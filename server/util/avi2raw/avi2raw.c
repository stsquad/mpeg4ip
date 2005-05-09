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

#include <math.h>
#include <mpeg4ip.h>
#include <mpeg4ip_getopt.h>
#include <avilib.h>
#include <mp4av_h264.h>
//#define DEBUG_H264 1
/* globals */
char* progName;

/*
 * avi2raw
 * required arg1 should be the input AVI file
 * required arg2 should be the output RAW file
 */ 
int main(int argc, char** argv)
{
	/* configurable variables from command line */
	bool extractVideo = TRUE;	/* FALSE implies extract audio */
	u_int32_t start = 0;		/* secs, start offset */
	u_int32_t duration = 0;		/* secs, 0 implies entire file */
	bool quiet = FALSE;		

	/* internal variables */
	char* aviFileName = NULL;
	char* rawFileName = NULL;
	avi_t* aviFile = NULL;
	FILE* rawFile = NULL;
	int verbose = FALSE;
	u_int32_t numBytes, totBytes = 0;
	bool eliminate_short_frames = FALSE;
	uint32_t short_frames_len;
	/* begin process command line */
	progName = argv[0];
	while (1) {
		int c = -1;
		int option_index = 0;
		static struct option long_options[] = {
			{ "audio", 0, 0, 'a' },
			{ "eliminate-short-frames", optional_argument, 0, 'e'},
			{ "length", 1, 0, 'l' },
			{ "quiet", 0, 0, 'q' },
			{ "start", 1, 0, 's' },
			{ "video", 0, 0, 'v' },
			{ "version", 0, 0, 'V'},
			{ "verbose", 0, 0, 'R'},
			{ "help", 0, 0, 'h'},
			{ NULL, 0, 0, 0 }
		};

		c = getopt_long_only(argc, argv, "ael:qs:vVh",
			long_options, &option_index);

		if (c == -1)
			break;

		switch (c) {
		case 'h':
		  fprintf(stderr, "%s - %s version %s\n", progName,
			  MPEG4IP_PACKAGE, MPEG4IP_VERSION);
		  fprintf(stderr, "options:\n");
		  fprintf(stderr, " --audio - extract audio track\n");
		  fprintf(stderr, " --length <length> - extract <length> secs\n");
		  fprintf(stderr, " --quiet - quiet mode\n");
		  fprintf(stderr, " --start <time> - extract from <start> time\n");
		  fprintf(stderr, " --video - extract video track\n");
		  fprintf(stderr, " --eliminate-short-frames <bytes> - eliminate short frames of bytes or less - default 4\n");
		  return 0;
		case 'a': {
			extractVideo = FALSE;
			break;
		}
		case 'e':
		  eliminate_short_frames = TRUE;
		  if (optarg) {
		    if (optarg[0] == '=') optarg[0] = ' ';
		    if (sscanf(optarg, "%u", &short_frames_len) != 1) {
		      fprintf(stderr, "%s:bad eliminate length %s\n",
			      progName, optarg);
		    }
		  } else {
		    short_frames_len = 4;
		  }
		  break;
		case 'l': {
			/* --length=<secs> */
			u_int i;
			if (sscanf(optarg, "%u", &i) < 1) {
				fprintf(stderr, 
					"%s: bad length specified: %s\n",
					 progName, optarg);
			} else {
				duration = i;
			}
			break;
		}
		case 'q': {
			quiet = TRUE;
			break;
		}
		case 's': {
			/* --start=<secs> */
			u_int i;
			if (sscanf(optarg, "%u", &i) < 1) {
				fprintf(stderr, 
					"%s: bad start specified: %s\n",
					 progName, optarg);
			} else {
				start = i;
			}
			break;
		}
		case 'v': {
			extractVideo = TRUE;
			break;
		}
		case '?':
			break;
		case 'R':
		  verbose = TRUE; quiet = FALSE;
		  break;
		case 'V':
		  fprintf(stderr, "%s - %s version %s\n", progName,
			  MPEG4IP_PACKAGE, MPEG4IP_VERSION);
		  return(0);
		default:
			fprintf(stderr, "%s: unknown option specified, ignoring: %c\n", 
				progName, c);
		}
	}

	/* check that we have at least two non-option arguments */
	if ((argc - optind) < 2) {
		fprintf(stderr, 
			"usage: %s <avi-file> <raw-file>\n",
			progName);
		exit(1);
	}

	/* point to the specified file names */
	aviFileName = argv[optind++];
	rawFileName = argv[optind++];

	/* warn about extraneous non-option arguments */
	if (optind < argc) {
		fprintf(stderr, "%s: unknown options specified, ignoring: ", progName);
		while (optind < argc) {
			fprintf(stderr, "%s ", argv[optind++]);
		}
		fprintf(stderr, "\n");
	}

	/* end processing of command line */

	/* open the AVI file */
	aviFile = AVI_open_input_file(aviFileName, TRUE);
	if (aviFile == NULL) {
		fprintf(stderr, 
			"%s: error %s: %s\n",
			progName, aviFileName, AVI_strerror());
		exit(4);
	}

	if (!quiet) {
	  fprintf(stderr, "%s - %s version %s\n",
		  progName, MPEG4IP_PACKAGE, MPEG4IP_VERSION);
	}
	/* open the RAW file */
	rawFile = fopen(rawFileName, "wb");
	if (rawFile == NULL) {
		fprintf(stderr,
			"%s: error opening %s: %s\n",
			progName, rawFileName, strerror(errno));
		exit(5);
	}

	if (extractVideo) {

		double videoFrameRate = AVI_video_frame_rate(aviFile);
		u_int32_t numVideoFrames = AVI_video_frames(aviFile);
		u_int32_t fileDuration = ceil(numVideoFrames / videoFrameRate);
		u_int32_t numDesiredVideoFrames;
		u_int32_t videoFramesRead = 0;
		u_int32_t emptyFramesRead = 0;
		/* get a buffer large enough to handle a frame of raw SDTV */
		u_char* buf = (u_char*)malloc(768 * 576 * 4);

		if (duration) {
			numDesiredVideoFrames = duration * videoFrameRate;
		} else {
			numDesiredVideoFrames = numVideoFrames;
		}

		if (buf == NULL) {
			fprintf(stderr,
				"%s: error allocating memory: %s\n",
				progName, strerror(errno));
			exit(6);
		}

		/* check that start offset is valid */
		if (start > fileDuration) {
			fprintf(stderr,
				"%s: specified start is past the end of the file\n",
				progName);
			exit(7);
		}

		if (AVI_seek_start(aviFile)) {
			fprintf(stderr,
				"%s: bad seek: %s\n",
				progName, AVI_strerror());
			exit(8);
		}
		if (AVI_set_video_position(aviFile, (long) ROUND(start * videoFrameRate), NULL)) {
			fprintf(stderr,
				"%s: bad seek: %s\n",
				progName, AVI_strerror());
			exit(9);
		}
#ifdef DEBUG_H264
		h264_decode_t dec;
#endif

		while (TRUE) {
		  numBytes = AVI_read_frame(aviFile, (char *)buf);

			/* read error */
			if (numBytes < 0) {
				break;
			}

			totBytes += numBytes;
			videoFramesRead++;
			if (verbose) {
			  printf("frame %d - len %u total %u\n", 
				 videoFramesRead, numBytes, totBytes);
			}
			/*
			 * note some capture programs 
			 * insert a zero length frame occasionally
			 * hence numBytes == 0, but we're not a EOF
			 */
			if ((eliminate_short_frames && numBytes > short_frames_len) ||
			    (eliminate_short_frames == FALSE && numBytes)) {
			  // test
#ifdef DEBUG_H264
			  uint32_t offset = 0, read;
			  do {
			    if (h264_is_start_code(buf + offset)) {
			      int ret = 
			      h264_detect_boundary(buf + offset, 
						   numBytes - offset, 
						   &dec);

			      printf(" frame offset %d nal type %d slice %d %d\n", 
				     offset,
				     dec.nal_unit_type,
				     dec.slice_type,
				     ret);
			    }
			    read = h264_find_next_start_code(buf + offset, 
							     numBytes - offset);
			    offset += read;
			  } while (read != 0 && offset < numBytes);
#endif
				if (fwrite(buf, 1, numBytes, rawFile) != numBytes) {
					fprintf(stderr,
						"%s: error writing %s: %s\n",
						progName, rawFileName, strerror(errno));
					break;
				}
			} else {
				emptyFramesRead++;
			}
			if (videoFramesRead >= numDesiredVideoFrames) {
				break;
			}
		}
		if (verbose) {
		  printf("read %u video bytes\n", totBytes);
		}

		if (numBytes < 0) {
			printf("%s: error reading %s, frame %d, %s\n",
				progName, aviFileName, videoFramesRead + 1, AVI_strerror());
		}
		if (videoFramesRead < numDesiredVideoFrames) {
			fprintf(stderr,
				"%s: warning: could only extract %u seconds of video (%u of %u frames)\n",
				progName, (unsigned int)ceil(videoFramesRead / videoFrameRate),
				videoFramesRead, numDesiredVideoFrames);
		}
		if (emptyFramesRead) {
			fprintf(stderr,
				"%s: warning: %u zero length frames ignored\n",
				progName, emptyFramesRead);
		}

		if (!quiet) {
			printf("%u video frames written\n", 
				videoFramesRead - emptyFramesRead);
		}

		/* cleanup */
		free(buf);

	} else {
		/* extract audio */
	  u_int32_t audioBytesRead = 0;
	  u_char *buf = (u_char*) malloc(8*1024);
	  u_int32_t numDesiredAudioBytes = AVI_audio_bytes(aviFile);
	  u_int32_t audioBytesPerSec = 0;
	  if (start != 0) {
		u_int32_t numAudioBytes = numDesiredAudioBytes;
		u_int32_t fileDuration;
		audioBytesPerSec = AVI_audio_rate(aviFile) * 	
		  ((AVI_audio_bits(aviFile) + 7) / 8) * AVI_audio_channels(aviFile);
		fileDuration = ceil(numAudioBytes / audioBytesPerSec);
		numDesiredAudioBytes = duration * audioBytesPerSec;

		/* check that start offset is valid */
		if (start > fileDuration) {
			fprintf(stderr,
				"%s: specified start is past the end of the file\n",
				progName);
			exit(7);
		}
		if (AVI_seek_start(aviFile)) {
			fprintf(stderr,
				"%s: bad seek: %s\n",
				progName, AVI_strerror());
			exit(8);
		}
		if (AVI_set_audio_position(aviFile, start * audioBytesPerSec)) {
			fprintf(stderr,
				"%s: bad seek: %s\n",
				progName, AVI_strerror());
			exit(9);
		}
	  } else {
		if (AVI_seek_start(aviFile)) {
			fprintf(stderr,
				"%s: bad seek: %s\n",
				progName, AVI_strerror());
			exit(8);
		}
		if (AVI_set_audio_position(aviFile, 0)) {
			fprintf(stderr,
				"%s: bad seek: %s\n",
				progName, AVI_strerror());
			exit(9);
		}
	  }

	  while ((numBytes = AVI_read_audio(aviFile, (char *)buf, sizeof(buf))) > 0) {
			if (fwrite(buf, 1, numBytes, rawFile) != numBytes) {
				fprintf(stderr,
					"%s: error writing %s: %s\n",
					progName, rawFileName, strerror(errno));
				break;
			}

			audioBytesRead += numBytes;
			if (numDesiredAudioBytes 
			  && audioBytesRead >= numDesiredAudioBytes) {
				break;
			}
		}

		if (verbose) {
		  printf("read %u audio bytes\n", audioBytesRead);
		}
		if (duration && audioBytesRead < numDesiredAudioBytes) {
			fprintf(stderr,
				"%s: warning: could only extract %u seconds of audio\n",
				progName, audioBytesPerSec == 0 ? audioBytesRead : audioBytesRead / audioBytesPerSec);
		}

		if (!quiet && AVI_audio_bits(aviFile) != 0) {
			printf("%u audio samples written\n", 
				audioBytesRead / ((AVI_audio_bits(aviFile) + 7) / 8)); 
		}

	}

	/* cleanup */
	AVI_close(aviFile);
	fclose(rawFile);

	return(0);
}


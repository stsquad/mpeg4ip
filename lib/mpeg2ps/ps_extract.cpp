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
 * Copyright (C) Cisco Systems Inc. 2004.  All Rights Reserved.
 * 
 * Contributor(s): 
 *		Bill May wmay@cisco.com
 */
/*
 * ps_extract.c - extract elementary stream from program stream
 */
#include "mpeg2_ps.h"
#include "mpeg4ip_getopt.h"

int main(int argc, char** argv)
{
  char* usageString = "[--video] [--audio] <file-name>\n";
  bool dump_audio = false, dump_video = false;
  uint audio_stream = 0, video_stream = 0;
  char *infilename;
  char outfilename[FILENAME_MAX], *suffix;
  size_t suffix_len;
  FILE *outfile;
  mpeg2ps_t *infile;
  /* begin processing command line */
  char* progName = argv[0];
  long cnt = 0;
  bool dump = false;

  while (1) {
    int c = -1;
    int option_index = 0;
    static struct option long_options[] = {
      { "audio", 2, 0, 'a' },
      { "video", 2, 0, 'v' },
      { "dump", 0, 0, 'd' },
      { "version", 0, 0, 'V' },
      { NULL, 0, 0, 0 }
    };

    c = getopt_long_only(argc, argv, "a:v:Vd",
			 long_options, &option_index);
    
    if (c == -1)
      break;

    switch (c) {
    case 'v':
      dump_video = true;
      if (optarg) {
	uint32_t readstream;
	if (sscanf(optarg, "%u", &readstream) == 1) {
	  video_stream = readstream;
	}
      }
      break;
    case 'a':
      dump_audio = true;
      if (optarg) {
	uint32_t readstream;
	if (sscanf(optarg, "%u", &readstream) == 1) {
	  audio_stream = readstream;
	}
      }
      break;
    case 'd':
      dump = true;
      break;
    case 'V':
      fprintf(stderr, "%s - %s version %s\n", 
	      progName, MPEG4IP_PACKAGE, MPEG4IP_VERSION);
      exit(0);
    default:
      fprintf(stderr, "%s: unknown option specified, ignoring: %c\n", 
	      progName, c);
    }
  }

  if ((argc - optind) < 1) {
    fprintf(stderr, "usage: %s %s\n", progName, usageString);
    exit(1);
  }

#if 0
  if (dump_audio == 0 && dump_video == 0) {
    dump_audio = dump_video = 1;
  }
#endif

  while (optind < argc) {
    infilename = argv[optind];
    optind++;
    suffix = strrchr(infilename, '.');
    suffix_len = suffix -  infilename;
    if (suffix == NULL) {
      fprintf(stderr, "can't find suffix in %s\n", infilename);
      continue;
    }
    infile = mpeg2ps_init(infilename);
    if (infile == NULL) {
      fprintf(stderr, "%s is not a valid mpeg file\n", 
	      infilename);
      continue;
    }
    
    uint8_t *buf = NULL;
    uint32_t len = 0;
    uint64_t ftime;
    uint32_t freq_ftime;
    uint32_t last_freq_time = 0;
    if (dump_audio) {
      if (mpeg2ps_get_audio_stream_count(infile) == 0) {
	fprintf(stderr, "no audio streams in %s\n", infilename);
      } else if (audio_stream >= mpeg2ps_get_audio_stream_count(infile)) {
	fprintf(stderr, "audio stream %d does not exist\n", audio_stream);
      } else {
	char *slash;
	slash = strrchr(infilename, '/');
	if (slash != NULL) {
	  slash++;
	  suffix_len = suffix - slash;
	  memcpy(outfilename, slash, suffix_len);
	} else {
	  memcpy(outfilename, infilename, suffix_len);
	}
	mpeg2ps_audio_type_t audio_type = 
	  mpeg2ps_get_audio_stream_type(infile, audio_stream);
	switch (audio_type) {
	case MPEG_AUDIO_MPEG:
	  strcpy(outfilename + suffix_len, ".mp3");
	  break;
	case MPEG_AUDIO_AC3:
	  strcpy(outfilename + suffix_len, ".ac3");
	  break;
	case MPEG_AUDIO_LPCM:
	  strcpy(outfilename + suffix_len, ".pcm");
	  break;
	default:
	  break;
	}
	outfile = fopen(outfilename, FOPEN_WRITE_BINARY);
	while (mpeg2ps_get_audio_frame(infile,
				       audio_stream,
				       &buf, 
				       &len,
				       TS_MSEC,
				       &freq_ftime,
				       &ftime)) {
	  printf("audio len %d time %u "U64"\n", 
		 len, freq_ftime, ftime);
	  if (audio_type == MPEG_AUDIO_LPCM) {
	    if (last_freq_time != 0) {
	      if (last_freq_time != freq_ftime) {
		printf("error in timestamp %u %u\n", freq_ftime, last_freq_time);
	      }
	    }
	    last_freq_time = freq_ftime + (len / 4);
#if 0
	    if (last_freq_time + 1152 != freq_ftime) {
	      printf("error in timestamp %d\n", freq_ftime - last_freq_time);
	    }
	    last_freq_time = freq_ftime;
#endif
	  }
	  fwrite(buf, len, 1, outfile);
	  cnt++;
	}
	fclose(outfile);
	printf("%ld audio frames\n", cnt);
      }
    }
	
    if (dump_video) {
      if (mpeg2ps_get_video_stream_count(infile) == 0) {
	fprintf(stderr, "no video streams in %s\n", infilename);
      } else if (video_stream >= mpeg2ps_get_video_stream_count(infile)) {
	fprintf(stderr, "video stream %d does not exist\n", video_stream);
      } else {
	char *slash;
	slash = strrchr(infilename, '/');
	if (slash != NULL) {
	  slash++;
	  suffix_len = suffix - slash;
	  memcpy(outfilename, slash, suffix_len);
	} else {
	  memcpy(outfilename, infilename, suffix_len);
	}
	mpeg2ps_video_type_t video_type = 
	  mpeg2ps_get_video_stream_type(infile, video_stream);
	switch (video_type) {
	case MPEG_VIDEO_H264:
	  strcpy(outfilename + suffix_len, ".264");
	  break;
	default:
	  strcpy(outfilename + suffix_len, ".m2v");
	}
      
	outfile = fopen(outfilename, FOPEN_WRITE_BINARY);
	cnt = 0;
	while (mpeg2ps_get_video_frame(infile, 
				       video_stream,
				       &buf,
				       &len,
				       NULL,
				       TS_MSEC,
				       &ftime)) {
	  printf("video len %d time "U64"\n", 
		 len, ftime);
	  if (buf[len - 2] == 1 &&
	      buf[len - 3] == 0 &&
	      buf[len - 4] == 0) len -= 4;
	  fwrite(buf, len, 1, outfile);
	  cnt++;
	}
	fclose(outfile);
	printf("%ld video frames\n", cnt);
      }
    }
    mpeg2ps_close(infile);

  }
  return 0;
}

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
 * ps_info.cpp - dump information about a program stream file
 */
#include "mpeg2_ps.h"
#include "mpeg4ip_getopt.h"


int main(int argc, char** argv)
{
  const char* usageString = 
    "<file-name>\n";
  /* begin processing command line */
  char* ProgName = argv[0];
  while (true) {
    int c = -1;
    int option_index = 0;
    static struct option long_options[] = {
      { "version", 0, 0, 'V' },
      { "debug", 0, 0, 'd' },
      { NULL, 0, 0, 0 }
    };

    c = getopt_long_only(argc, argv, "Vd",
			 long_options, &option_index);

    if (c == -1)
      break;

    switch (c) {
    case '?':
      fprintf(stderr, "usage: %s %s", ProgName, usageString);
      exit(0);
    case 'V':
      fprintf(stderr, "%s - %s version %s\n", ProgName, 
	      MPEG4IP_PACKAGE, MPEG4IP_VERSION);
      exit(0);
    case 'd':
      mpeg2ps_set_loglevel(LOG_DEBUG);
      break;
    default:
      fprintf(stderr, "%s: unknown option specified, ignoring: %c\n", 
	      ProgName, c);
    }
  }

  /* check that we have at least one non-option argument */
  if ((argc - optind) < 1) {
    fprintf(stderr, "usage: %s %s", ProgName, usageString);
    exit(1);
  }

  /* end processing of command line */
  printf("%s version %s\n", ProgName, MPEG4IP_VERSION);

  mpeg2ps_t *ps;
  while (optind < argc) {
    char *fileName = argv[optind++];


    ps = mpeg2ps_init(fileName);

    if (ps == NULL) {

      fprintf(stderr, "Can't read file %s as program stream\n", fileName);
      continue;
    }
    double dur = mpeg2ps_get_max_time_msec(ps);
    dur /= 1000.0;
    printf("%s: duration %.2f seconds\n", fileName, dur);
    uint ix;
    uint max;
    printf("  Video streams:\n");
    max = mpeg2ps_get_video_stream_count(ps);
    if (max == 0) {
      printf("   No streams\n");
    } else {
      for (ix = 0; ix < max; ix++) {
	char *name = 
	  mpeg2ps_get_video_stream_name(ps, ix);
	printf("   stream %d: %s %ux%u", 
	       ix,
	       name,
	       mpeg2ps_get_video_stream_width(ps, ix),
	       mpeg2ps_get_video_stream_height(ps, ix));
	free(name);
	double brate, framerate;
	brate = mpeg2ps_get_video_stream_bitrate(ps, ix);
	framerate = mpeg2ps_get_video_stream_framerate(ps, ix);
	if (brate != 0.0 && framerate != 0.0) {
	  printf(", %.0f @ %.2f frames per second", brate, framerate);
	} else if (brate != 0.0) {
	  printf(", %.0f", brate); 
	} else if (framerate != 0.0) {
	  printf("%.2f frames per second", framerate);
	}
	    
	printf("\n");
      }
    }
    printf(" Audio streams:\n");
    max = mpeg2ps_get_audio_stream_count(ps);
    if (max == 0) {
      printf("   No streams\n");
    } else {
      for (ix = 0; ix < mpeg2ps_get_audio_stream_count(ps); ix++) {
	printf("   stream %d: %s, %u channels %u sample rate %u bitrate\n",
	       ix,
	       mpeg2ps_get_audio_stream_name(ps, ix),
	       mpeg2ps_get_audio_stream_channels(ps, ix),
	       mpeg2ps_get_audio_stream_sample_freq(ps, ix),
	       mpeg2ps_get_audio_stream_bitrate(ps, ix));
      }
    }
	
    mpeg2ps_close(ps);
    ps = NULL;
  }

  return(0);
}


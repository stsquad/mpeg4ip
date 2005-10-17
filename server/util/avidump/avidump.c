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
 * Copyright (C) Cisco Systems Inc. 2001.  All Rights Reserved.
 * 
 * Contributor(s): 
 *		Dave Mackie		dmackie@cisco.com
 *		Bill May		wmay@cisco.com
 */

#include <math.h>
#include <mpeg4ip.h>
#include <mpeg4ip_getopt.h>
#include <avilib.h>


/* globals */
char* progName;
static struct {
  const char *name;
  int value;
} audio_names[] = {
  {"UNKNOWN", 0x0000},
  {"PCM", 0x0001},
  {"ADPCM", 0x0002},
  {"IBM_CVSD", 0x0005},
  {"ALAW", 0x0006},
  {"MULAW", 0x0007},
  {"OKI_ADPCM", 0x0010},
  {"DVI_ADPCM", 0x0011},
  {"DIGISTD", 0x0015},
  {"DIGIFIX", 0x0016},
  {"YAMAHA_ADPCM", 0x0020},
  {"DSP_TRUESPEECH", 0x0022},
  {"GSM610", 0x0031},
  {"MP3", 0x0055},
  {"IBMMULAW", 0x0101},
  {"IBMALAW", 0x0102},
  {"IBMADPCM", 0x0103},
  { NULL, 0}, 
};

static dump_video_sizes (avi_t* aviFile)
{
  long frames = AVI_video_frames(aviFile);
  long ix;

  for (ix = 0; ix < frames; ix++) {
    fprintf(stdout, "    frame %lu size %lu\n", ix + 1, AVI_frame_size(aviFile, ix));
  }
}

static const char *usage=":[--video-track-dump] <filename> ...\n";
/*
 *avidump
 * required arg1 should be file to check
 */ 
int main(int argc, char** argv)
{
  /* internal variables */
  char* aviFileName = NULL;
  avi_t* aviFile = NULL;
  uint32_t numVideoFrames;
  int ix, format;
  const char *aname;
  bool dump_video_track = false;
  
  /* begin process command line */
  progName = argv[0];
  while (1) {
    int c = -1;
    int option_index = 0;
    static struct option long_options[] = {
      { "version", 0, 0, 'V'},
      { "help", 0, 0, '?'},
      { "video-track-dump", 0, 0, 'v'},
      { NULL, 0, 0, 0 }
    };

    c = getopt_long_only(argc, argv, "Vh",
			 long_options, &option_index);
    
    if (c == -1)
      break;

    switch (c) {
    case '?':
      fprintf(stderr, "%s%s\n", progName, usage); 
      return (0);
    case 'V':
      fprintf(stderr, "%s - %s version %s\n", progName,
	      MPEG4IP_PACKAGE, MPEG4IP_VERSION);
      return (0);
    case 'v':
      dump_video_track = true;
      break;
    default:
      fprintf(stderr, "%s: unknown option specified, ignoring: %c\n", 
	      progName, c);
    }
  }

  /* check that we have at least two non-option arguments */
  if ((argc - optind) < 1) {
    fprintf(stderr, 
	    "usage: %s%s\n",
	    progName, usage);
    exit(1);
  }

  fprintf(stderr, "%s - %s version %s\n",
	  progName, MPEG4IP_PACKAGE, MPEG4IP_VERSION);

  /* point to the specified file names */
  while (optind < argc) {
    aviFileName = argv[optind++];
    if (aviFileName == NULL) return (0);
    /* open the AVI file */
    aviFile = AVI_open_input_file(aviFileName, TRUE);
    if (aviFile == NULL) {
      fprintf(stderr, 
	      "%s: error %s: %s\n",
	      progName, aviFileName, AVI_strerror());
      exit(4);
    }

    fprintf(stdout, "%s\n", aviFileName);
    numVideoFrames = AVI_video_frames(aviFile);

    if (numVideoFrames != 0) {
      fprintf(stdout, "Video Information\n");
      fprintf(stdout,
	      "   Video compressor: %s\n",
	      AVI_video_compressor(aviFile));
      fprintf(stdout,
	      "   Video frame rate: %g\n", 
	      AVI_video_frame_rate(aviFile));
      fprintf(stdout, "   Number of frames: %u\n", numVideoFrames);
      fprintf(stdout, "   Video height    : %d\n", AVI_video_height(aviFile));
      fprintf(stdout, "   Video width     : %d\n", AVI_video_width(aviFile));
      if (dump_video_track) 
	dump_video_sizes(aviFile);
    } else {
      fprintf(stdout, "No video frames\n");
    }

    if (AVI_audio_bytes(aviFile) != 0) {
      fprintf(stdout, "Audio information:\n");
      fprintf(stdout, "   Audio channels: %d\n", AVI_audio_channels(aviFile));
      fprintf(stdout, "   Audio bits/sample: %d\n", AVI_audio_bits(aviFile));
      format = AVI_audio_format(aviFile);
      for (ix = 0, aname = NULL;
	   aname == NULL && audio_names[ix].name != NULL;
	   ix++) {
	if (audio_names[ix].value == format) {
	  aname = audio_names[ix].name;
	}
      }
      if (aname == NULL) aname = "Format not in list";
      fprintf(stdout, "   Audio format: %d %s\n", format, aname);
      fprintf(stdout, "   Audio sample rate: %ld\n", AVI_audio_rate(aviFile));
      fprintf(stdout, "   Audio bytes: %ld\n", AVI_audio_bytes(aviFile));
    } else {
      fprintf(stdout, "No audio track\n");
    }
    /* cleanup */
    AVI_close(aviFile);
  }

  return(0);
}


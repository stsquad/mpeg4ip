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
 */

// N.B. mp4extract just extracts tracks/samples from an mp4 file
// For many track types this is insufficient to reconsruct a valid
// elementary stream (ES). Use "mp4creator -extract=<trackId>" if
// you need the ES reconstructed. 

#include "mp4.h"
#include "mpeg4ip_getopt.h"

char* ProgName;
char* Mp4PathName;
char* Mp4FileName;
static void DumpTrack (MP4FileHandle mp4file, MP4TrackId tid)
{
  uint32_t numSamples;
  MP4SampleId sid;
  MP4Duration time;
  uint32_t timescale;
  double sectime;

  numSamples = MP4GetTrackNumberOfSamples(mp4file, tid);
  timescale = MP4GetTrackTimeScale(mp4file, tid);
  printf("mp4file %s, track %d, samples %d, timescale %d\n", 
	 Mp4FileName, tid, numSamples, timescale);

  for (sid = 1; sid <= numSamples; sid++) {
    time = MP4GetSampleTime(mp4file, tid, sid);
    sectime = time;
    sectime /= timescale;

    printf("sampleId %6d, size %5u duration %8llu time %8llu %g\n",
	  sid,  MP4GetSampleSize(mp4file, tid, sid), 
	   MP4GetSampleDuration(mp4file, tid, sid),
	   time, sectime);
  }
}

int main(int argc, char** argv)
{
  char* usageString = 
    "usage: %s [-l] [-t <track-id>] [-s <sample-id>] [-v [<level>]] <file-name>\n";
  MP4TrackId trackId = MP4_INVALID_TRACK_ID;
  u_int32_t verbosity = MP4_DETAILS_ERROR;

  /* begin processing command line */
  ProgName = argv[0];
  while (true) {
    int c = -1;
    int option_index = 0;
    static struct option long_options[] = {
      { "track", 1, 0, 't' },
      { "verbose", 2, 0, 'v' },
      { "version", 0, 0, 'V' },
      { NULL, 0, 0, 0 }
    };

    c = getopt_long_only(argc, argv, "t:v::V",
			 long_options, &option_index);

    if (c == -1)
      break;

    switch (c) {
    case 't':
      if (sscanf(optarg, "%u", &trackId) != 1) {
	fprintf(stderr, 
		"%s: bad track-id specified: %s\n",
		ProgName, optarg);
	exit(1);
      }
      break;
    case 'v':
      verbosity |= MP4_DETAILS_READ;
      if (optarg) {
	u_int32_t level;
	if (sscanf(optarg, "%u", &level) == 1) {
	  if (level >= 2) {
	    verbosity |= MP4_DETAILS_TABLE;
	  } 
	  if (level >= 3) {
	    verbosity |= MP4_DETAILS_SAMPLE;
	  } 
	  if (level >= 4) {
	    verbosity = MP4_DETAILS_ALL;
	  }
	}
      }
      break;
    case '?':
      fprintf(stderr, usageString, ProgName);
      exit(0);
    case 'V':
      fprintf(stderr, "%s - %s version %s\n", 
	      ProgName, PACKAGE, VERSION);
      exit(0);
    default:
      fprintf(stderr, "%s: unknown option specified, ignoring: %c\n", 
	      ProgName, c);
    }
  }

  /* check that we have at least one non-option argument */
  if ((argc - optind) < 1) {
    fprintf(stderr, usageString, ProgName);
    exit(1);
  }
	
  if (verbosity) {
    fprintf(stderr, "%s version %s\n", ProgName, VERSION);
  }

  /* point to the specified file names */
  Mp4PathName = argv[optind++];

  char* lastSlash = strrchr(Mp4PathName, '/');
  if (lastSlash) {
    Mp4FileName = lastSlash + 1;
  } else {
    Mp4FileName = Mp4PathName; 
  }

  /* warn about extraneous non-option arguments */
  if (optind < argc) {
    fprintf(stderr, "%s: unknown options specified, ignoring: ", ProgName);
    while (optind < argc) {
      fprintf(stderr, "%s ", argv[optind++]);
    }
    fprintf(stderr, "\n");
  }

  /* end processing of command line */


  MP4FileHandle mp4File = MP4Read(Mp4PathName, verbosity);

  if (!mp4File) {
    exit(1);
  }

  if (trackId == 0) {
    u_int32_t numTracks = MP4GetNumberOfTracks(mp4File);

    for (u_int32_t i = 0; i < numTracks; i++) {
      trackId = MP4FindTrackId(mp4File, i);
      DumpTrack(mp4File, trackId);
    }
  } else {
    DumpTrack(mp4File, trackId);
  }

  MP4Close(mp4File);

  return(0);
}


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
 *              Bill May        wmay@cisco.com
 */
/* 
 * movplayer.cpp - test program for quicktime library
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <syslog.h>
#include <fstream.h>
#include <strstream.h>
#include <quicktime.h>

int main (int argc, char **argv)
{
  char *name;
  int ret;
  argv++;
  argc--;

  if (*argv == NULL) {
    name = "/home/wmay/content/batman_av.mov";
  } else {
    name = *argv;
  }
  
  ret = quicktime_check_sig(name);
  if (ret == 1) {
    printf("Looks like qt movie\n");
  } else {
    printf("return from check_sig is %d", ret);
    exit(0);
  }

  quicktime_t *file;
  file = quicktime_open(name, 1, 0, 0);
  if (file == NULL) {
    printf("Couldn't open file %s\n", name);
    exit(1);
  }
  int video = quicktime_video_tracks(file);
  int audio = quicktime_audio_tracks(file);
  int track_chan = quicktime_track_channels(file, 0);
  printf("Video tracks %d audio tracks %d trackchan %d\n", video, audio,
	 track_chan);
  printf("Video compressor :%s:\n", quicktime_video_compressor(file, 0));
  if (video > 0) {
    long vlen = quicktime_video_length(file, 0);
    int vw, vh;
    float fr;
    vw = quicktime_video_width(file, 0);
    vh = quicktime_video_height(file, 0);
    fr = quicktime_video_frame_rate(file, 0);
    printf("len %ld vw %d vh %d frame rate %g\n", vlen, vw, vh, fr);
#if 0
    FILE *ofile = fopen("temp.mp4v", "w");
    unsigned char *buffer = (unsigned char *)malloc(8 * 1024);
    long max_fsize = 8 * 1024;

    for (long frame_on = 0; frame_on < vlen; frame_on++) {
      long next_frame = quicktime_frame_size(file, frame_on, 0);
      if (next_frame > max_fsize) {
	max_fsize = next_frame;
	free(buffer);
	buffer = (unsigned char *)malloc(max_fsize);
      }
      next_frame = quicktime_read_frame(file, 
					buffer,
					0);
      printf("Framesize %ld\n", next_frame);
      fwrite(buffer, next_frame, sizeof(unsigned char), ofile);
    }
    fclose(ofile);
#endif
      
  }
  
  if (audio > 0) {
    long sample = quicktime_audio_sample_rate(file, 0);
    long audiolen = quicktime_audio_length(file, 0);
    int bits = quicktime_audio_bits(file, 0);
    printf("Audio sample %ld len %ld bits %d\n", sample, audiolen, bits);
    printf("Audio compressor :%s:\n", quicktime_audio_compressor(file, 0));
#if 0
    unsigned char buffer[8096];
    FILE *ofile = fopen("temp.aac", "w");
    uint32_t retsize;
    for (long ix = 0; ix < audiolen; ix++) {
      retsize = quicktime_read_audio_frame(file, buffer, 0);
      //printf("Frame %ld - len %d", ix, retsize);
      fwrite(buffer, retsize, sizeof(char), ofile);
    }
    fclose(ofile);
#endif
  }
  quicktime_close(file);

}


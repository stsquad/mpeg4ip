#include <string.h>
#include <stdio.h>
#include "live_apis.h"
#include "file_write.h"

static int record_video = 1, record_audio = 1;
static char *record_file = NULL;

int get_record_video (void)
{
  return record_video;
}

int get_record_audio (void)
{
  return record_audio;
}

const char *get_record_file (void)
{
  return (record_file);
}

int set_record_parameters (int record_v,
			   int record_a,
			   const char *record_f,
			   const char **ermsg)
{
  record_video = record_v;
  record_audio = record_a;
  if (record_file != NULL) 
    free(record_file);
  record_file = strdup(record_f);
  return (1);
}

CFileWriteBase *start_record (int audio_enabled, int video_enabled)
{
  if (record_audio && (audio_enabled != 0 && video_enabled == 0)) {
    // just start audio record
    return (audio_record_file(record_file));
  }
  return NULL;
}

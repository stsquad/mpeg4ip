#include "live_apis.h"
#include "audio.h"
#include "mp3.h"
#include "mp3_file.h"

CAudioRecorder audio_recorder;

#define MAX_FREQ 10
int try_frequencies[MAX_FREQ] = 
{ 8000, 11025, 12000, 16000, 22050, 24000, 32000, 44100, 48000, 96000 };

char *frequency_names[MAX_FREQ];
int frequency_values[MAX_FREQ];
size_t established_freq;

void init_audio (void)
{
  size_t ix;
  size_t freq_ix;
  int orig_freq = audio_recorder.get_frequency();
  const char *errmsg;
  // create audio frequency list - try to configure various frequencies, 
  // see what happens.
  for (ix = 0, freq_ix = 0; ix < MAX_FREQ; ix++) {
    if (audio_recorder.configure_audio(try_frequencies[ix], &errmsg) >= 0) {
      frequency_names[freq_ix] = (char *)malloc(8);
      sprintf(frequency_names[freq_ix], "%d", try_frequencies[ix]);
      frequency_values[freq_ix] = try_frequencies[ix];
      freq_ix++;
    }
  }
  established_freq = freq_ix;
  audio_recorder.configure_audio(orig_freq, &errmsg);
}

const char *get_audio_frequency (void) 
{
  int freq = audio_recorder.get_frequency();
  for (size_t ix = 0; ix < established_freq; ix++) {
    if (freq == frequency_values[ix]) {
      return (frequency_names[ix]);
    }
  }
  return (NULL);
}

const char **get_audio_frequencies (size_t &max, size_t &current_index)
{
  int freq = audio_recorder.get_frequency();
  max = established_freq;
  for (current_index = 0; current_index < established_freq; current_index++) {
    if (freq == frequency_values[current_index]) {
      return ((const char **)frequency_names);
    }
  }
  current_index = 0;
  return ((const char **)frequency_names);
}

#define CODEC_MP3 0
const char *audio_codecs[] = {
  "MP3",
};

static size_t audio_codec_index = CODEC_MP3;
size_t audio_kbps = 128;

const char *get_audio_codec (void)
{
  return audio_codecs[audio_codec_index];
}

const char **get_audio_codecs (size_t &max, size_t &current_index)
{
  max = 1;
  current_index = audio_codec_index;
  return audio_codecs;
}

size_t get_audio_kbitrate (void)
{
  return audio_kbps;
}

int set_audio_parameters (size_t audio_codec_ix,
			  size_t audio_freq_index,
			  size_t audio_bitrate,
			  const char **errmsg) 
{
  *errmsg = NULL;
  if (audio_freq_index >= established_freq) {
    *errmsg = "Frequency out of range";
    return (-1);
  }
  
  if (audio_recorder.configure_audio(frequency_values[audio_freq_index],
				     errmsg) < 0) {
    return (-1);
  }
  switch (audio_codec_ix) {
  case CODEC_MP3:
    if (mp3_check_params(frequency_values[audio_freq_index], 
			 audio_bitrate,
			 errmsg) < 0) {
      return (-1);
    }
    break;
  }

  audio_kbps = audio_bitrate;
  return (0);
}

int start_audio_recording (CFileWriteBase *fb)
{
  audio_recorder.set_file_write(fb);
  return (audio_recorder.start_record());
}

int stop_audio_recording (void)
{
  return (audio_recorder.stop_record());
}

CFileWriteBase *audio_record_file (const char *record_file)
{
  return new CMp3FileWrite(record_file);
}

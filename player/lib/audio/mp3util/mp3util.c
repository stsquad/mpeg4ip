#include "mp3util.h"

static const int frequencies[9]=
{
  44100,48000,32000, // MPEG 1
  22050,24000,16000,  // MPEG 2
  11025,12000, 8000, // mpeg 2.5
};

static const int bitrate[2][3][15]=
{
  // MPEG 1
  {{0,32,64,96,128,160,192,224,256,288,320,352,384,416,448},
   {0,32,48,56,64,80,96,112,128,160,192,224,256,320,384},
   {0,32,40,48,56,64,80,96,112,128,160,192,224,256,320}},

  // MPEG 2
  {{0,32,48,56,64,80,96,112,128,144,160,176,192,224,256},
   {0,8,16,24,32,40,48,56,64,80,96,112,128,144,160},
   {0,8,16,24,32,40,48,56,64,80,96,112,128,144,160}}
};
  
int decode_mp3_header (unsigned char *buffer,
		       mp3_header_t *mp3)
{
  unsigned char c;
  int sampling_freq;
  
  if ((buffer[0] != 0xff) ||
      ((buffer[1] & 0xe0) != 0xe0)) {
    return -1;
  }

  c = buffer[1];
  mp3->mpeg25 = ((c & 0x10) == 0) ? 1 : 0;

  c &= 0xf;
  mp3->protection = c & 1;
  mp3->layer = 4 - ((c >> 1) & 3);
  if (mp3->mpeg25 == 0) {
    mp3->version = ((c >> 3) ^ 1);
  } else
    mp3->version = 1;

  c = buffer[2] >> 1;
  mp3->padding = c & 1;
  c >>= 1;
  mp3->frequencyidx = c & 3;
  if (mp3->frequencyidx == 3) return -1;
  c >>= 2;
  mp3->bitrateindex = c;
  if (mp3->bitrateindex == 15) return -1;
  sampling_freq = mp3->frequencyidx + (mp3->version * 3);
  if (mp3->mpeg25) sampling_freq += 3;

  c = buffer[3];
  mp3->extendedmode = c & 3;
  mp3->mode = c >> 2;

  mp3->frequency = frequencies[mp3->frequencyidx];
  if (mp3->layer == 1) {
    mp3->framesize =
      (1200 * bitrate[mp3->version][0][mp3->bitrateindex]) /
      frequencies[sampling_freq];
    if (mp3->frequency == 0 && mp3->padding) mp3->framesize++;
    mp3->framesize <<= 2;
  } else {
    mp3->framesize=
      (144000 * bitrate[mp3->version][mp3->layer-1][mp3->bitrateindex])/
      (frequencies[sampling_freq]<<mp3->version);
    if (mp3->padding) mp3->framesize++;
  }

  return 0;
}

int mp3_sideinfo_size (mp3_header_t *mp3)
{
  int ret;
  if (mp3->version)
    // mpeg2
    if (mp3->mode == 3)
      ret = 9;
    else
      ret = 17;
  else 
    // mpeg1
    if (mp3->mode == 3)
      ret = 17;
    else
      ret = 32;
  if (!mp3->protection) ret += 2;
  return ret;
}

int mp3_get_backpointer (mp3_header_t *mp3, const unsigned char *buf)
{
  int ret;
  buf += 4;
  if (!mp3->protection) buf += 2;
  
  if (mp3->version) {
    // mpeg 2
    return buf[0];
  } 
  ret = buf[0];
  ret <<= 1;
  if (buf[1] & 0x80) ret |= 1;
  return ret;
}

int mp3_get_samples_per_frame (int layer, int version)
{
  int samplesperframe;

  samplesperframe = 32;

  if (layer == 3) {
    samplesperframe *= 18;
    if (version == 0) {
      samplesperframe *= 2;
    }
  } else {
    samplesperframe *= SCALEBLOCK;
    if (layer == 2) {
      samplesperframe *= 3;
    }
  }
  return samplesperframe;
}

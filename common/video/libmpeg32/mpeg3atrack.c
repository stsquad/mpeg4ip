#include "libmpeg3.h"
#include "mpeg3protos.h"
#include "mpeg3demux.h"
#include "mp4av.h"
#include <stdlib.h>

static int mpeg3_atrack_get_mp3_info (mpeg3_atrack_t *atrack)
{
  uint8_t hdr[4];
  mpeg3_demuxer_t *demux;
  MP4AV_Mp3Header temp;
  demux = atrack->demuxer;

  hdr[1] = mpeg3demux_read_char(demux);
  do {
    hdr[0] = hdr[1];
    hdr[1] = mpeg3demux_read_char(demux);
  } while (((hdr[0] != 0xff) || 
	   ((hdr[1] & 0xe0) != 0xe0)) && 
	   !mpeg3demux_eof(demux));
  
  if (mpeg3demux_eof(demux)) return -1;

  hdr[2] = mpeg3demux_read_char(demux);
  hdr[3] = mpeg3demux_read_char(demux);
  temp = MP4AV_Mp3HeaderFromBytes(hdr);
  atrack->sample_rate = MP4AV_Mp3GetHdrSamplingRate(temp);
  atrack->channels = MP4AV_Mp3GetChannels(temp);
  atrack->samples_per_frame = MP4AV_Mp3GetHdrSamplingWindow(temp);
  return 0;
}

static int mpeg3_atrack_get_aac_info (mpeg3_atrack_t *atrack)
{
  uint8_t hdr[16];
  mpeg3_demuxer_t *demux;
  uint16_t hdr_size;
  uint16_t ix;

  demux = atrack->demuxer;

  hdr[1] = mpeg3demux_read_char(demux);
  do {
    hdr[0] = hdr[1];
    hdr[1] = mpeg3demux_read_char(demux);
  } while (((hdr[0] != 0xff) || 
	   ((hdr[1] & 0xf0) != 0xf0)) && 
	   !mpeg3demux_eof(demux));

  if (mpeg3demux_eof(demux)) return -1;

  hdr_size = MP4AV_AacAdtsGetHeaderByteSize(hdr);
  if (hdr_size > sizeof(hdr)) return -1;
  for (ix = 2; ix < hdr_size; ix++) {
    hdr[ix] = mpeg3demux_read_char(demux);
  }

  if (mpeg3demux_eof(demux)) return -1;
  
  atrack->sample_rate = MP4AV_AacAdtsGetSamplingRate(hdr);
  atrack->channels = MP4AV_AacAdtsGetChannels(hdr);
  atrack->samples_per_frame = 1024; // always the same
  
  return 0;
}
static int mpeg3_ac3_samplerates[] = { 48000, 44100, 32000 };

struct mpeg3_framesize_s
{
	unsigned short bit_rate;
	unsigned short frm_size[3];
};

static struct mpeg3_framesize_s framesize_codes[] = 
{
      { 32  ,{64   ,69   ,96   } },
      { 32  ,{64   ,70   ,96   } },
      { 40  ,{80   ,87   ,120  } },
      { 40  ,{80   ,88   ,120  } },
      { 48  ,{96   ,104  ,144  } },
      { 48  ,{96   ,105  ,144  } },
      { 56  ,{112  ,121  ,168  } },
      { 56  ,{112  ,122  ,168  } },
      { 64  ,{128  ,139  ,192  } },
      { 64  ,{128  ,140  ,192  } },
      { 80  ,{160  ,174  ,240  } },
      { 80  ,{160  ,175  ,240  } },
      { 96  ,{192  ,208  ,288  } },
      { 96  ,{192  ,209  ,288  } },
      { 112 ,{224  ,243  ,336  } },
      { 112 ,{224  ,244  ,336  } },
      { 128 ,{256  ,278  ,384  } },
      { 128 ,{256  ,279  ,384  } },
      { 160 ,{320  ,348  ,480  } },
      { 160 ,{320  ,349  ,480  } },
      { 192 ,{384  ,417  ,576  } },
      { 192 ,{384  ,418  ,576  } },
      { 224 ,{448  ,487  ,672  } },
      { 224 ,{448  ,488  ,672  } },
      { 256 ,{512  ,557  ,768  } },
      { 256 ,{512  ,558  ,768  } },
      { 320 ,{640  ,696  ,960  } },
      { 320 ,{640  ,697  ,960  } },
      { 384 ,{768  ,835  ,1152 } },
      { 384 ,{768  ,836  ,1152 } },
      { 448 ,{896  ,975  ,1344 } },
      { 448 ,{896  ,976  ,1344 } },
      { 512 ,{1024 ,1114 ,1536 } },
      { 512 ,{1024 ,1115 ,1536 } },
      { 576 ,{1152 ,1253 ,1728 } },
      { 576 ,{1152 ,1254 ,1728 } },
      { 640 ,{1280 ,1393 ,1920 } },
      { 640 ,{1280 ,1394 ,1920 } }
};

/* Audio channel modes */
static short mpeg3_ac3_acmodes[] = {2, 1, 2, 3, 3, 4, 4, 5};

static int mpeg3_atrack_get_ac3_info (mpeg3_atrack_t *atrack)
{
  uint16_t code, mask;
  unsigned int sampling_freq_code, framesize_code;
  unsigned int acmod;
  unsigned int skipbits;
  mpeg3_demuxer_t *demux;

  demux = atrack->demuxer;

  code = mpeg3demux_read_char(demux);
  do {
    code &= 0xff;
    code <<= 8;
    code |= mpeg3demux_read_char(demux);
  } while (!mpeg3demux_eof(demux) && code != MPEG3_AC3_START_CODE);
  if (mpeg3demux_eof(demux)) return -1;
  
  // 2 bytes CRC
  mpeg3demux_read_char(demux);
  mpeg3demux_read_char(demux);
  
  code = mpeg3demux_read_char(demux);
  sampling_freq_code = (code >> 6) & 0x3;
  framesize_code = code & 0x3f;

  // bsid and bsmod
  mpeg3demux_read_char(demux);
  // acmod is 3 bits
  code = mpeg3demux_read_char(demux) << 8;
  code |= mpeg3demux_read_char(demux);
  if (mpeg3demux_eof(demux)) return -1;

  atrack->sample_rate = 
    mpeg3_ac3_samplerates[sampling_freq_code];
  atrack->framesize = 
    2 * framesize_codes[framesize_code].frm_size[sampling_freq_code];
  atrack->samples_per_frame = 1536; // fixed amount from rfc draft

  acmod = (code >> 13) & 0x7;
  atrack->channels = mpeg3_ac3_acmodes[acmod];

  skipbits = 3;
  if ((acmod & 0x1) && (acmod != 1)) {
    skipbits += 2;
  }
  if (acmod & 0x4) skipbits += 2;
  if (acmod == 0x2) skipbits += 2;

  mask = 1 << (15 - skipbits);
  if ((code & mask) != 0) atrack->channels++;
  return 0;
}

static int mpeg3_atrack_suck_frame_info (mpeg3_atrack_t *atrack)
{
  mpeg3_demuxer_t *demux;

  demux = atrack->demuxer;
  if (atrack->format == AUDIO_UNKNOWN) {
    uint16_t code;
    code = mpeg3demux_read_char(demux);
    code <<= 8;
    code |= mpeg3demux_read_char(demux);
    if (code == MPEG3_AC3_START_CODE) {
      atrack->format = AUDIO_AC3;
    } else if ((code & 0xfff8) == 0xfff8) {
      atrack->format = AUDIO_MPEG;
    } else {
      atrack->format = AUDIO_AAC;
    }
    code = mpeg3demux_read_prev_char(demux);
    code = mpeg3demux_read_prev_char(demux);
  }
  
  switch (atrack->format) {
  case AUDIO_PCM:
    atrack->sample_rate = 48000;
    atrack->channels = 2;
    atrack->framesize = 0x7db;
    break;
  case AUDIO_MPEG:
    if (mpeg3_atrack_get_mp3_info(atrack) < 0) return -1;
    break;
  case AUDIO_AC3:
    if (mpeg3_atrack_get_ac3_info(atrack) < 0) return -1;
    break;
  case AUDIO_AAC:
    if (mpeg3_atrack_get_aac_info(atrack) < 0) return -1;
    break;
  }
  mpeg3demux_seek_start(demux);
}

mpeg3_atrack_t* mpeg3_new_atrack(mpeg3_t *file, 
	int stream_id, 
	int format, 
	mpeg3_demuxer_t *demuxer,
	int number)
{
	mpeg3_atrack_t *new_atrack;

	new_atrack = calloc(1, sizeof(mpeg3_atrack_t));
	new_atrack->file = file;
	new_atrack->channels = 0;
	new_atrack->sample_rate = 0;
	new_atrack->total_samples = 0;
	new_atrack->percentage_seek = -1;
	new_atrack->demuxer = mpeg3_new_demuxer(file, 1, 0, stream_id);
	if(new_atrack->demuxer) mpeg3demux_copy_titles(new_atrack->demuxer, demuxer);
	new_atrack->current_position = 0;

	new_atrack->format = format;
	if (mpeg3_atrack_suck_frame_info(new_atrack) < 0 ||
	    new_atrack->format == AUDIO_UNKNOWN) {
/* Failed */
	  mpeg3_delete_atrack(file, new_atrack);
	  new_atrack = NULL;
	}

	//printf("mpeg3 demux length %g\n", mpeg3demux_length(new_atrack->demuxer));
// Copy pointers
	if(file->sample_offsets)
	{
		new_atrack->sample_offsets = file->sample_offsets[number];
		new_atrack->total_sample_offsets = file->total_sample_offsets[number];
		new_atrack->total_frames = new_atrack->total_sample_offsets / 
		  new_atrack->samples_per_frame;
	} else {
	  double time;
	  if (new_atrack->samples_per_frame != 0) {
	    time = mpeg3demux_length(new_atrack->demuxer);

	    time *= new_atrack->sample_rate;
	    time /= new_atrack->samples_per_frame;
	    new_atrack->total_frames = (uint32_t)time;
	    new_atrack->total_sample_offsets = new_atrack->total_frames * 
	      new_atrack->samples_per_frame;
	    //printf("total frames %d\n", new_atrack->total_frames);
	  } else {
	    new_atrack->total_frames = 0;
	    new_atrack->total_sample_offsets = 0;
	  }
	} 
	//printf("total offsets %ld\n", new_atrack->total_sample_offsets);
	return new_atrack;
}

int mpeg3_delete_atrack(mpeg3_t *file, mpeg3_atrack_t *atrack)
{
	if(atrack->demuxer) mpeg3_delete_demuxer(atrack->demuxer);
	free(atrack);
	return 0;
}

static int mpeg3_atrack_check_length (unsigned char **output, 
				      uint32_t cmplen,
				      uint32_t *maxlen)
{
  if (cmplen > *maxlen) {
    *output = (unsigned char *)realloc(*output, cmplen);
    if (*output == NULL) return -1;
    *maxlen = cmplen;
  }
  return 0;
}
   

static int mpeg3_atrack_read_pcm_frame (mpeg3_atrack_t *atrack,
					unsigned char **output,
					uint32_t *len,
					uint32_t *maxlen)
{
  uint16_t code;
  mpeg3_demuxer_t *demux;
  uint32_t frame_sample;
  int ret;

  demux = atrack->demuxer;

  code = mpeg3demux_read_char(demux);
  do {
    code <<= 8;
    code |= mpeg3demux_read_char(demux);
  } while (!mpeg3demux_eof(demux) && code != MPEG3_PCM_START_CODE);

  if (mpeg3demux_eof(demux)) return -1;

  frame_sample = (atrack->framesize - 3) / atrack->channels / 2;
  *len = frame_sample * atrack->channels * (sizeof(uint16_t));

  if (mpeg3_atrack_check_length(output, *len, maxlen) < 0) return -1;
 
  ret = mpeg3demux_read_data(demux, *output, *len);
  if (ret) return -1;
  return 0;
}

static int mpeg3_atrack_read_mp3_frame (mpeg3_atrack_t *atrack,
					unsigned char **output,
					uint32_t *len,
					uint32_t *maxlen)
{
  uint8_t code[4];
  mpeg3_demuxer_t *demux;
  uint32_t frame_samples;
  int ret;
  MP4AV_Mp3Header temp;
  int cnt = 1;

  demux = atrack->demuxer;

  code[1] = mpeg3demux_read_char(demux);
  do {
    code[0] = code[1];
    code[1] = mpeg3demux_read_char(demux);
    cnt++;
  } while (!mpeg3demux_eof(demux) && 
	   (code[0] != 0xff || ((code[1] & 0xe0) != 0xe0)));

  if (mpeg3demux_eof(demux)) return -1;

  code[2] = mpeg3demux_read_char(demux);
  code[3] = mpeg3demux_read_char(demux);

  temp = MP4AV_Mp3HeaderFromBytes(code);
  *len = MP4AV_Mp3GetFrameSize(temp);

  //printf("header is %08x framesize %d\n", temp, *len);
  if (mpeg3_atrack_check_length(output, *len, maxlen) < 0) return -1;

  memcpy(*output, code, 4);
  ret = mpeg3demux_read_data(demux, *output + 4, *len - 4);
  if (ret) return -1;
  return 0;
}

static int mpeg3_atrack_read_aac_frame (mpeg3_atrack_t *atrack,
					unsigned char **output, 
					uint32_t *len, 
					uint32_t *maxlen)
{
  mpeg3_demuxer_t *demux;
  uint16_t hdr_size, ix;
  uint32_t bytes;
  unsigned char hdr[16];
  int ret;

  demux = atrack->demuxer;
  hdr[1] = mpeg3demux_read_char(demux);
  do {
    hdr[0] = hdr[1];
    hdr[1] = mpeg3demux_read_char(demux);
  } while (((hdr[0] != 0xff) || 
	   ((hdr[1] & 0xf0) != 0xf0)) && 
	   !mpeg3demux_eof(demux));
  bytes = atrack->channels * sizeof(uint16_t) * atrack->samples_per_frame;

  if (mpeg3demux_eof(demux)) return -1;

  hdr_size = MP4AV_AacAdtsGetHeaderByteSize(hdr);
  if (hdr_size > sizeof(hdr)) return -1;
  for (ix = 2; ix < hdr_size; ix++) {
    hdr[ix] = mpeg3demux_read_char(demux);
  }
  if (mpeg3demux_eof(demux)) return -1;

  bytes = MP4AV_AacAdtsGetFrameSize(hdr);
  *len = bytes;
  if (mpeg3_atrack_check_length(output, *len, maxlen) < 0) return -1;

  memcpy(*output, hdr, hdr_size);
  ret = mpeg3demux_read_data(demux, *output + hdr_size, *len - hdr_size);
  if (ret) return -1;
  return 0;
}

static int mpeg3_atrack_read_ac3_frame (mpeg3_atrack_t *atrack,
					unsigned char **output, 
					uint32_t *len, 
					uint32_t *maxlen)
{
  uint16_t code;
  mpeg3_demuxer_t *demux;
  int ret;

  demux = atrack->demuxer;

  code = mpeg3demux_read_char(demux);
  do {
    code &= 0xff;
    code <<= 8;
    code |= mpeg3demux_read_char(demux);
  } while (!mpeg3demux_eof(demux) && code != MPEG3_AC3_START_CODE);
  if (mpeg3demux_eof(demux)) return -1;

  *len = atrack->framesize;
  if (mpeg3_atrack_check_length(output, *len, maxlen) < 0) return -1;
  *output[0] = code >> 8;
  *output[1] = code & 0xff;
  ret = mpeg3demux_read_data(demux, *output + 2, *len - 2);
  if (ret) return -1;

  return 0;
}

int mpeg3_atrack_read_frame (mpeg3_atrack_t *atrack, 
			     unsigned char **output,
			     uint32_t *len,
			     uint32_t *maxlen)
{
  if (atrack->percentage_seek >= 0) {
    mpeg3demux_seek_percentage(atrack->demuxer, atrack->percentage_seek);
    atrack->percentage_seek = -1;
  }
  switch (atrack->format) {
  case AUDIO_PCM:
    return (mpeg3_atrack_read_pcm_frame(atrack, output, len, maxlen));
  case AUDIO_MPEG:
    return (mpeg3_atrack_read_mp3_frame(atrack, output, len, maxlen));
  case AUDIO_AC3:
    return (mpeg3_atrack_read_ac3_frame(atrack, output, len, maxlen));
  case AUDIO_AAC:
    return (mpeg3_atrack_read_aac_frame(atrack, output, len, maxlen));
  }
  return -1;
}

int mpeg3atrack_seek_percentage(mpeg3_atrack_t *atrack, double percentage)
{
  atrack->percentage_seek = percentage;
  return 0;
}


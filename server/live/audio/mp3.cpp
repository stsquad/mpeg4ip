#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "mp3.h"
#include "util.h"
#include "../../audio/lame/util.h"

// needed to figure out framesize
static const int frequencies[2][3]=
{
  {44100,48000,32000}, // MPEG 1
  {22050,24000,16000}  // MPEG 2
};

// needed to figure out framesize
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

// Generic defines for ether payload.  Tune MP3_SAMPLE_SIZE so you don't
// have a lot of leftover while encoding.
#define ETHER_RTP_PAYLOAD 1460
#define MP3_SAMPLE_SIZE 1152

CMp3Encoder::CMp3Encoder (int sample_freq, int bitrate, CFileWriteBase *fb)
{
  // Initialize LAME subsystem
  lame_init(&m_gf);
  m_gf.num_channels = 2;
  m_gf.in_samplerate = sample_freq;
  m_gf.brate = bitrate;
  m_gf.mode = 0;
  m_gf.quality = 2;
  m_gf.silent = 1;
  m_gf.gtkflag = 0;
  lame_init_params(&m_gf);
  //
  // Output buffer storage will be enough to store a complete ethernet
  // frame.
  //
  m_encoded_data_size = ETHER_RTP_PAYLOAD + 7200 + ((5 * MP3_SAMPLE_SIZE) / 4);
  m_encoded_data = (unsigned char *) malloc(m_encoded_data_size);
  m_data_end = m_encoded_data;
  m_next_frame_start = m_encoded_data;
  m_prev_frame_start = m_encoded_data;
  m_data_in_buffer = 0;
  m_file_write = fb;
}

CMp3Encoder::~CMp3Encoder (void)
{
  int ret;
  ret = lame_encode_finish(&m_gf, 
			   (char *)m_data_end, 
			   m_encoded_data_size - m_data_in_buffer);
  if (ret > 0) {
    process_encoded_data(ret);
    check_data_end();
    // now, get rid of any more data
    if (m_data_in_buffer) {
      debug_message("At end - %d bytes", m_data_in_buffer);
    }
  }

  free(m_encoded_data);
}

size_t CMp3Encoder::get_requested_sample_size (void) 
{
  return (MP3_SAMPLE_SIZE);
}

// get_frame_size - peek into the frame, return the size.
size_t CMp3Encoder::get_frame_size (void)
{
  if ((*m_next_frame_start != 0xff) &&
      ((*(m_next_frame_start + 1) & 0xf0) != 0xf0)) {
    // frame header error
    return (0);
  } 
  // Good frame header...
  // Read out of it, and get the frame size - we need to be able
  // to handle pure frames...
  unsigned char ch = *(m_next_frame_start + 1);
  int layer, bitrateindex, padding;
  int version, frequency, framesize;
  ch &= 0xf;
  layer = 4 - ((ch >> 1) & 0x3);
  version = ((ch >> 3) ^ 1);
    
  ch = *(m_next_frame_start + 2) >> 1;
  padding = ch & 0x1;
  ch >>= 1;
  frequency = ch & 0x3;
  ch >>= 2;
  bitrateindex = ch;
#if 0
  debug_message("%02x %02x %02x", *m_next_frame_start,
		*(m_next_frame_start + 1),
		*(m_next_frame_start + 2)
		);
  debug_message("layer %d version %d bitrate %d freq %d",
		layer, version, bitrateindex, frequency);
  debug_message("bitrate %d freq %d", 
		bitrate[version][layer - 1][bitrateindex],
		frequencies[version][frequency]);
#endif
  if (layer == 1) {
    framesize = (12000 * bitrate[version][0][bitrateindex]) / 
      frequencies[version][frequency];
      
    // 0 is 44100
    if (frequency == 0 && padding) framesize++;
    framesize <<= 2;
  } else {
    framesize = (144000 * bitrate[version][layer - 1][bitrateindex]) /
      (frequencies[version][frequency] << version);
    if (padding) framesize++;
  }
  //debug_message("frame size is %d", framesize);
  return (framesize);
}

// check_data_end - see if we have enough data to send out the wire - 
// we probably also want to use this for writing to the disk, so we're
// not writing a bunch of small data packets.
void CMp3Encoder::check_data_end (void)
{

  if (m_next_frame_start < m_data_end) {
    // We have 1 frame, which is m_next_frame_start to next_frame_start,
    // size framesize
    // We have a couple of frames, m_encoded_data to next_frame_start
    if (m_file_write != NULL && m_prev_frame_start != m_next_frame_start) {
      size_t prev;
      prev = m_next_frame_start - m_prev_frame_start;
      if (m_file_write != NULL) {
	m_file_write->write_audio_frame(m_prev_frame_start, prev);
      }
      m_prev_frame_start = m_next_frame_start;
    }

    size_t complete_fr_in_buffer;
    complete_fr_in_buffer = m_next_frame_start - m_encoded_data;

    size_t lower_limit = (ETHER_RTP_PAYLOAD / m_framesize) * m_framesize;
    //debug_message("lowerlimit is %d, now have %d", 
    // lower_limit, complete_fr_in_buffer);
    if ((complete_fr_in_buffer >= lower_limit) ||
	(complete_fr_in_buffer + m_framesize > ETHER_RTP_PAYLOAD)) {
      // bingo... Output frame, adjust pointer, move rest of data back.
      size_t left_over;
      left_over = m_data_in_buffer - complete_fr_in_buffer;
      debug_message("Good set of frames - len %d, moving %d",
		    complete_fr_in_buffer, left_over);
      memcpy(m_encoded_data, m_next_frame_start, left_over);
      m_next_frame_start = m_encoded_data;
      m_prev_frame_start = m_encoded_data;
      m_data_end -= complete_fr_in_buffer;
      m_data_in_buffer = left_over;
    }
  }
}

// process_encoded_data - process the data returned from the encoder.
// this is where we'll enhance it to write to a file, etc.
void CMp3Encoder::process_encoded_data (int bytes_ret)
{
  m_data_end += bytes_ret; // adjust data end pointer.
  m_data_in_buffer += bytes_ret;

  if (m_data_end <= m_next_frame_start + 2)
    return;

  m_framesize = get_frame_size();
  if (m_framesize != 0) {
    check_data_end();

    do {
      m_framesize = get_frame_size();
      if (m_framesize != 0) {
	m_next_frame_start += m_framesize;
	check_data_end();
      } else {
	// dump buffer
	debug_message("Frame error - is %02x %02x", 
		      *m_next_frame_start, 
		      *(m_next_frame_start + 1));
	m_data_end = m_next_frame_start = m_prev_frame_start = m_encoded_data;
	m_data_in_buffer = 0;
      } 
    } while (m_next_frame_start < m_data_end);
  } else {
    // dump buffer
    debug_message("Frame error - is %02x %02x", 
		  *m_next_frame_start, 
		  *(m_next_frame_start + 1));
    m_data_end = m_next_frame_start = m_prev_frame_start = m_encoded_data;
    m_data_in_buffer = 0;
  }
}

void CMp3Encoder::encode (unsigned char *buffer, size_t size)
{
  short int left[MP3_SAMPLE_SIZE], right[MP3_SAMPLE_SIZE], *ptr;
  int ret;

  ptr = (short int *)buffer;
  for (int ix = 0; ix < MP3_SAMPLE_SIZE; ix++) {
    left[ix] = *ptr++;
    right[ix] = *ptr++;
  }

  ret = lame_encode_buffer(&m_gf,
			   left,
			   right,
			   MP3_SAMPLE_SIZE,
			   (char *)m_data_end, 
			   m_encoded_data_size - m_data_in_buffer);
  if (ret >= 0) {
    process_encoded_data(ret);
  } else {
    debug_message("bad frame - return %d", ret);
  }
}
			
int mp3_check_params (int freq, int bitrate, const char **errmsg)
{
  int version;

  if (SmpFrqIndex(freq, &version) < 0) {
    *errmsg = "Illegal Sampling frequency selected";
    return -1;
  }

  if (BitrateIndex(bitrate, version, freq) < 0) {
    *errmsg = "Illegal bit rate for sampling frequency";
    return -1;
  }
  return 0;
}

// end file mp3.cpp

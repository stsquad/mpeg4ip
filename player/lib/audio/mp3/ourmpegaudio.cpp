#include "MPEGaudio.h"

MPEGaudio::MPEGaudio (c_byte_get_t c_byte_get, 
		      c_byte_read_t c_byte_read,
		      void *ud)
{
  initialize();
  m_byte_get = c_byte_get;
  m_byte_read = c_byte_read;
  m_userdata = ud;
}

MPEGaudio::~MPEGaudio()
{
}

bool MPEGaudio::fillbuffer(int _size)
{
  size_t ret, size;
  size = _size;
  bitindex = 0;
  ret = m_byte_read(_buffer, size, m_userdata);
  return (size == ret);
}

bool MPEGaudio::decodeFrame (unsigned char *buffer, bool no_head)
{
  if (no_head == 0) {
    if (loadheader() == false) {
      return false;
    }
  }

  rawdata = (short *)buffer;
  rawdatawriteoffset = 0;
  if (layer == 3) extractlayer3();
  else if (layer == 2) extractlayer2();
  else if (layer == 1) extractlayer1();

  return (true);
}
  




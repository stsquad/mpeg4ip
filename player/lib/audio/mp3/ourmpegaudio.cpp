#include "MPEGaudio.h"

MPEGaudio::MPEGaudio (c_get_more_t c_get_more, 
		      void *ud)
{
  initialize();
  m_get_more = c_get_more;
  m_userdata = ud;
}

MPEGaudio::~MPEGaudio()
{
}

bool MPEGaudio::fillbuffer(uint32_t _size)
{
  if (_size > _buflen) {
    (m_get_more)(m_userdata, &_buffer, &_buflen, _orig_buflen - _buflen);
    if (_buflen < _size)
      return false;
    _orig_buflen = _buflen;
  }
  bitindex = 0;
  return true;
}

int MPEGaudio::findheader (unsigned char *frombuffer, 
			   uint32_t frombuffer_len,
			   uint32_t *frameptr)
{
  uint32_t skipped;
  _orig_buflen = frombuffer_len;
  while (frombuffer_len >= 4) {
    if ((ntohs(*(uint16_t*)frombuffer) & 0xffe0) == 0xffe0) {
      _buffer = frombuffer;
      _buflen = frombuffer_len;
      skipped = _orig_buflen - frombuffer_len;

      if (loadheader()) {
	/* Fill the buffer with new data */
	if (frombuffer_len < framesize) {
	  _buflen = 0;
	  skipped = 0;
	  // fillbuffer will say we've used all the frombuffer_len - 
	  // so we don't need to skip anything...
	  if (frameptr != NULL)
	    *frameptr = framesize - frombuffer_len;
	  if(!fillbuffer(framesize))
	    return -1;
	} else {
	  if (frameptr != NULL) {
	    *frameptr = framesize;
	  }
	}
	return skipped;
      }
    }
    frombuffer++;
    frombuffer_len--;
  }
  return -1;
}

int MPEGaudio::decodeFrame (unsigned char *tobuffer, 
			    unsigned char *frombuffer, 
			    uint32_t fromlen)
{
  _buffer = frombuffer;
  _buflen = fromlen;
  _orig_buflen = fromlen;
  if (loadheader() == false) {
    return _orig_buflen - _buflen;
  }

  _buffer += 4;
  _buflen -= 4;

  /* Fill the buffer with new data */
  if(!fillbuffer(framesize-4))
    return false;

  if(!protection)
  {
    getbyte();                      // CRC, Not check!!
    getbyte();
  }

  rawdata = (short *)tobuffer;
  rawdatawriteoffset = 0;
  if (layer == 3) extractlayer3();
  else if (layer == 2) extractlayer2();
  else if (layer == 1) extractlayer1();
  return ((_orig_buflen - _buflen) + (framesize - 4));
}
  




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
  //printf("fillbuffer %d\n", _size);
  //int buflen = _buflen;
  if (_size > _buflen) {
    (m_get_more)(m_userdata, &_buffer, &_buflen, _orig_buflen - _buflen);
    if (_buflen < _size) {
#if 0
      printf("Fill buffer failed - size %d buflen %d return %d\n", 
	     _size, buflen, _buflen);
#endif
      return false;
    }
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
	//printf("Find header\n");
	_orig_buflen = frombuffer_len;
	while (frombuffer_len >= 4) {
		if ((ntohs(*(uint16_t*)frombuffer) & 0xffe0) == 0xffe0) {
			_buffer = frombuffer;
			_buflen = frombuffer_len;
			skipped = _orig_buflen - frombuffer_len;
			//printf("skipped is %d\n", skipped);
			if (loadheader()) {
				/* Fill the buffer with new data */
				if (frameptr != NULL) {
					*frameptr = framesize;
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
  //printf("DecodeFrame\n");
  _buffer = frombuffer;
  _buflen = fromlen;
  _orig_buflen = fromlen;
  if (loadheader() == false) {
#if 1
    printf("Couldn't load mp3 header - orig %d buflen %d\n", 
	   _orig_buflen, _buflen);
#endif
    return _orig_buflen - _buflen;
  }

 
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
#if 0
  printf("orig %d buflen %d framesize %d\n", 
	 _orig_buflen, _buflen, framesize - 4);
#endif
  return ((_orig_buflen - _buflen) + (framesize - 4));
}
  




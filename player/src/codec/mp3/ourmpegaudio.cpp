#include "MPEGaudio.h"

MPEGaudio::MPEGaudio (void)
{
  initialize();
}

MPEGaudio::~MPEGaudio()
{
}

bool MPEGaudio::fillbuffer(uint32_t _size)
{
  //printf("fillbuffer %d\n", _size);
  //int buflen = _buflen;
  if (_size > _buflen) {
      return false;
  }
  bitindex = 0;
  return true;
}

int MPEGaudio::findheader (uint8_t *frombuffer, 
			   uint32_t frombuffer_len,
			   uint32_t *frameptr)
{

  for (uint32_t skipped = 0;        skipped <= frombuffer_len - 4;
       skipped++) {
    if (frombuffer[skipped] == 0xff &&
	((frombuffer[skipped + 1] & 0xe0) == 0xe0)) {
      _buffer = (unsigned char *)frombuffer + skipped;
      _buflen = frombuffer_len - skipped;

      if (loadheader()) {
	/* Fill the buffer with new data */
	if (frameptr != NULL) {
	  *frameptr = framesize;
	}
	return skipped;
      }
    }
	      
  }
  return -1;
}

int MPEGaudio::decodeFrame (uint8_t *tobuffer, 
			    uint8_t *frombuffer, 
			    uint32_t fromlen)
{
  //printf("DecodeFrame\n");
  _buffer = (unsigned char *)frombuffer;
  _buflen = fromlen;
  if (loadheader() == false) {
#if 1
    printf("Couldn't load mp3 header - orig %d buflen %d\n", 
	   fromlen, _buflen);
#endif
#if 0
    for (uint32_t ix = 0; ix < fromlen; ix += 8) {
      printf("%4d %02x %02x %02x %02x  %02x %02x %02x %02x\n",
	     ix, 
	     frombuffer[ix],
	     frombuffer[ix + 1],
	     frombuffer[ix + 2],
	     frombuffer[ix + 3],
	     frombuffer[ix + 4],
	     frombuffer[ix + 5],
	     frombuffer[ix + 6],
	     frombuffer[ix + 7]);
    }
#endif
    return fromlen - _buflen;
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
	 fromlen, _buflen, framesize - 4);
#endif
  return ((fromlen - _buflen) + (framesize - 4));
}
  




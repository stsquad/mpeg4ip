#ifndef __BYTESTREAM_HPP__
#define __BYTESTREAM_HPP__ 1

#include "systems.h"

class CInByteStreamBase
{
 public:
  CInByteStreamBase() {};
  virtual ~CInByteStreamBase() {};
  virtual int eof(void) = 0;
  virtual unsigned char get(void) = 0;
  virtual unsigned char peek(void) = 0;
  virtual void bookmark(int bSet) = 0;
  virtual void reset(void) = 0;
  virtual ssize_t read (char *buffer, size_t bytes) {
    for (size_t ix = 0; ix < bytes; ix++) buffer[ix] = get();
    return bytes;
  };
  virtual ssize_t read (unsigned char *buffer, size_t bytes) {
    return (read((char *)buffer, bytes));
  };
};


#endif

#ifndef __BYTESTREAM_HPP__
#define __BYTESTREAM_HPP__ 1


class CInByteStreamBase
{
 public:
  CInByteStreamBase() {};
  virtual ~CInByteStreamBase() {};
  virtual int eof(void) = 0;
  virtual char get(void) = 0;
  virtual char peek(void) = 0;
  virtual void bookmark(int bSet) = 0;
  virtual void reset(void) = 0;
};


#endif

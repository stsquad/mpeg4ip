#ifndef __BYTESTREAM_HPP__
#define __BYTESTREAM_HPP__ 1

#ifdef _WIN32
typedef int ssize_t;
#endif

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
  virtual const char *get_throw_error(int error) = 0;
  // A minor error is one where in video, you don't need to skip to the
  // next I frame.
  virtual int throw_error_minor(int error) = 0;
};


#endif

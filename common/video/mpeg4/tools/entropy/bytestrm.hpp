#ifndef __BYTESTREAM_HPP__
#define __BYTESTREAM_HPP__ 1

#include "systems.h"
#ifdef _WIN32
typedef int ssize_t;
#endif

typedef void (*get_more_t)(void *, unsigned char **buffer, 
			   uint32_t *buflen, uint32_t used, 
			   int nothrow);
class CInByteStreamBase
{
 public:
  CInByteStreamBase(get_more_t get_more, 
		    void *ud) 
  {
    m_get_more = get_more;
    m_ud = ud;
  };
  ~CInByteStreamBase() {};
  unsigned char get(void);
  unsigned char peek(void);
  void bookmark(int bSet);
  int eof(void) { return m_len == 0; };
  ssize_t read (char *buffer, size_t bytes) {
    for (size_t ix = 0; ix < bytes; ix++) buffer[ix] = get();
    return bytes;
  };
  ssize_t read (unsigned char *buffer, size_t bytes) {
    return (read((char *)buffer, bytes));
  };
  void set_buffer (unsigned char *mptr, uint32_t len)
  {
    m_memptr = mptr;
    m_len = len;
    m_offset = 0;
    m_bookmark_set = 0;
  };
  uint32_t used_bytes(void) { return m_offset;};
 protected:
  unsigned char *m_memptr;
 private:
  uint32_t m_offset, m_len, m_bookmark_offset;
  int m_bookmark_set;
  get_more_t m_get_more;
  void *m_ud;

};


#endif

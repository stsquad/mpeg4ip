
#ifndef __BYTESTRM_FILE__
#define __BYTESTRM_FILE__ 1
#include "bytestrm.hpp"
#include <istream.h>

class CInByteStreamFile : public CInByteStreamBase
{
 private:
  istream *m_pInStream;
  streampos m_bookmark_strmpos;
  int m_bookmark_eofstate;
 public:
  CInByteStreamFile(istream &inStream) { m_pInStream = &inStream; };
  ~CInByteStreamFile() {};
  int eof(void) { return (m_pInStream->eof()); };
  unsigned char get(void) { return (m_pInStream->get());};
  unsigned char peek(void) { return (m_pInStream->peek()); };
  void bookmark (int bSet) {
    if (bSet) {
      m_bookmark_strmpos = m_pInStream->tellg();
      m_bookmark_eofstate = m_pInStream->eof();
    } else {
      m_pInStream->seekg(m_bookmark_strmpos);
      if (m_bookmark_eofstate == 0)
	m_pInStream->clear();
    }
  };
  void reset(void) { m_pInStream->seekg(0);};
};

#endif

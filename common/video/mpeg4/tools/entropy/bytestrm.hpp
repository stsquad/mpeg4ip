#ifndef __BYTESTREAM_HPP__
#define __BYTESTREAM_HPP__ 1
#include <istream.h>

Class CInByteStreamBase
{
 public:
  CInByteStreamBase() {};
  virtual ~CInByteStreamBase() {};
  virtual int eof(void) = 0;
  virtual Char get(void) = 0;
  virtual Char peek(void) = 0;
  virtual void bookmark(Bool bSet) = 0;
  virtual void reset(void) = 0;
};

Class CInByteStreamFile : public CInByteStreamBase
{
 private:
  istream *m_pInStream;
  streampos m_bookmark_strmpos;
  int m_bookmark_eofstate;
 public:
  CInByteStreamFile(istream &inStream) { m_pInStream = &inStream; };
  ~CInByteStreamFile() {};
  int eof(void) { return (m_pInStream->eof()); };
  Char get(void) { return (m_pInStream->get());};
  Char peek(void) { return (m_pInStream->peek()); };
  void bookmark (Bool bSet) {
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

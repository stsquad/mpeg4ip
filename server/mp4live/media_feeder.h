#include "mp4live.h"
#include "media_sink.h"

class CMediaFeeder {
 public:
  CMediaFeeder(void);
  ~CMediaFeeder(void);

  bool AddSink(CMediaSink *pSink);
  void RemoveSink(CMediaSink *pSink);
  void RemoveAllSinks(void);
 protected:
  void ForwardFrame(CMediaFrame* pFrame);
  static const u_int16_t MAX_SINKS = 8;
  CMediaSink* m_sinks[MAX_SINKS];
  SDL_mutex*	m_pSinksMutex;
};

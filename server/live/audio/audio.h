
#ifndef __AUDIO_H__
#define __AUDIO_H__ 1
#include <time.h>
#include <sys/time.h>
#include "msg_queue.h"
#include "file_write.h"

class CAudioRecorder {
 public:
  CAudioRecorder(void);
  ~CAudioRecorder(void);

  int configure_audio(int frequency, const char **errmsg);
  int start_record(void);
  int stop_record(void);
  int audio_recorder_thread(void);
  int get_frequency (void) { return m_frequency;};
  void set_file_write (CFileWriteBase *fb) { m_file_write = fb; };
 private:
  int initialize_audio(void);
  void audio_recorder_task_process_msg(int &thread_stop, int &record);
  int m_format;
  int m_channels;
  int m_frequency;
  int m_selected_freq;
  int m_record_state;
  struct timeval m_start_time;
  SDL_Thread *m_record_thread;
  CMsgQueue m_record_thread_msg_queue;
  CFileWriteBase *m_file_write;
};

#endif

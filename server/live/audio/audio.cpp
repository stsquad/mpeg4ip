#include <unistd.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/soundcard.h>
#include "live_apis.h"
#include "audio.h"
#include "audio_msgs.h"
#include "mp3.h"
#include "util.h"

#define DEVICE "/dev/dsp"
static int c_record_thread (void *data)
{
  CAudioRecorder *a;
  a = (CAudioRecorder *)data;
  return (a->audio_recorder_thread());
}

CAudioRecorder::CAudioRecorder (void)
{
  m_record_thread = NULL;
  m_record_state = 0;
  m_channels = 2;
  m_format = AFMT_S16_LE;
  m_frequency = 44100;
  m_file_write = NULL;
}

CAudioRecorder::~CAudioRecorder (void) 
{
  if (m_record_thread != NULL) {
    m_record_thread_msg_queue.send_message(MSG_STOP_THREAD);
    SDL_WaitThread(m_record_thread, NULL);
    m_record_thread = NULL;
  }
}
    
int CAudioRecorder::configure_audio (int frequency,
				     const char **errmsg)
{
  int old_freq;
  if (m_record_state != 0) {
    *errmsg = "Cannot configure while recording";
    return (-1);
  }

  old_freq = m_frequency;
  m_frequency = frequency;
  int state = initialize_audio();
  if (state >= 0) {
    close(state);
    return (0);
  }
  m_frequency = old_freq; // restore...
  switch (state) {
  case -1:
    *errmsg = "Illegal Format Selected";
    break;
  case -2:
    *errmsg = "Format not returned as selected";
    break;
  case -3:
    *errmsg = "Illegal Number of channels selected";
    break;
  case -4:
    *errmsg = "Channel number not returned as selected";
    break;
  case -5:
    *errmsg = "Illegal sampling frequency selected";
    break;
  case -6:
    *errmsg = "Sample Frequency not in 10% range";
    break;
  }
  return -1;
}
				     
int CAudioRecorder::initialize_audio (void)
{
  int audio_fd;
  int ret;
  audio_fd = open(DEVICE, O_RDONLY, 0);

  ret = m_format;
  if (ioctl(audio_fd, SNDCTL_DSP_SETFMT, &ret) == -1) {
    close(audio_fd);
    return -1;
  }

  if (ret != m_format) {
    close(audio_fd);
    return -2;
  }
  
  ret = m_channels;
  if (ioctl(audio_fd, SNDCTL_DSP_CHANNELS, &ret) == -1) {
    close(audio_fd);
    return -3;
  }

  if (ret != m_channels) {
    close(audio_fd);
    return -4;
  }

  ret = m_frequency;
  if (ioctl(audio_fd, SNDCTL_DSP_SPEED, &ret) == -1) {
    close(audio_fd);
    return -5;
  }

  if (ret == m_frequency) {
    m_selected_freq = ret;
    return audio_fd;
  }

  int delta = m_frequency / 10;
  if (ret >= m_frequency - delta &&
      ret <= m_frequency + delta) {
    debug_message("selected freq not exact - %d is %d", m_frequency, ret);
    m_selected_freq = ret;
    return audio_fd;
  }
  close(audio_fd);
  return -6;
}

int CAudioRecorder::start_record (void)
{
  // start audio recording task
  if (m_record_state != 0) {
    return (0);
  }
  if (m_record_thread == NULL) {
    m_record_thread = SDL_CreateThread(c_record_thread, this);
    if (m_record_thread == NULL) {
      return (-1);
    }
  }
  m_record_thread_msg_queue.send_message(MSG_RECORD);
  m_record_state = 1;
  // start decoding task
  return (0);
}

int CAudioRecorder::stop_record (void)
{
  if (m_record_state == 0) {
    return (0);
  }

  if (m_record_thread == NULL) {
    return (-1);
  }
  debug_message("Sending stop");
  m_record_thread_msg_queue.send_message(MSG_STOP_RECORD);
  m_record_state = 0;

  m_record_thread_msg_queue.send_message(MSG_STOP_THREAD);
  SDL_WaitThread(m_record_thread, NULL);
  m_record_thread = NULL;
  return (0);
}

void CAudioRecorder::audio_recorder_task_process_msg (int &thread_stop,
						      int &record)
{
  CMsg *newmsg;

  if ((newmsg = m_record_thread_msg_queue.get_message()) != NULL) {
    switch (newmsg->get_value()) {
    case MSG_STOP_THREAD:
      thread_stop = 1;
      break;
    case MSG_RECORD:
      debug_message("Thread started");
      record = 1;
      break;
    case MSG_STOP_RECORD:
      debug_message("Thread Received stop");
      record = 0;
      break;
    }
    delete newmsg;
  }
}

int CAudioRecorder::audio_recorder_thread (void) 
{
  int thread_stop = 0;
  int record = 0;
  int audio_fd;

  while (thread_stop == 0) {
    // process message every 50 passes
    audio_recorder_task_process_msg(thread_stop, record);
    if (record == 0) {
      SDL_Delay(500);
      continue;
    }

    audio_fd = initialize_audio();
    if (audio_fd < 0) {
      record = 0;
      continue;
    }
    // Initialize file_write here...
    CMp3Encoder *encoder = new CMp3Encoder(m_frequency, 
					   get_audio_kbitrate(), 
					   m_file_write);

    // Here, we'd query the codec as to the requested input sample size
#ifdef DUMP_RAW_DATA_TO_FILE
    int ofile;
    ofile = creat("temp.raw",  S_IWRITE | S_IREAD);
#endif
    unsigned char *buffer;
    size_t size, req;
    req = encoder->get_requested_sample_size();
    size = m_channels * 
      sizeof(uint16_t) * 
      req;

    debug_message("buffer size is %d msec per frame %d", size,
		  req * 1000 / m_frequency);
    buffer = (unsigned char *)malloc(size);
    int first = 0;

    while (record != 0 && thread_stop == 0) {
      for (int ix = 0; ix < 10; ix++) {
	// do recording here...
	int ret;
	ret = read(audio_fd, buffer, size);

	if (ret < 0) {
	  error_message("Audio read failed - error %d", ret);
	  close(audio_fd);
	  audio_fd = initialize_audio();
	} else {
	  if (first == 0) {
	    gettimeofday(&m_start_time, NULL);
	    int64_t time;
#define USEC_PER_SEC 1000000LL
	    time = (req * USEC_PER_SEC);
	    time /= m_frequency;
	    debug_message("Subtracting off %llu usec", time);
	    if (time > USEC_PER_SEC) {
	      m_start_time.tv_sec -= time / USEC_PER_SEC;
	      time %= USEC_PER_SEC;
	    }
	    if (time > m_start_time.tv_usec) {
	      m_start_time.tv_sec--;
	      m_start_time.tv_usec += USEC_PER_SEC;
	    }
	    m_start_time.tv_usec -= time;

	    m_file_write->initialize_audio(m_channels,
					   m_frequency,
					   req,
					   0,
					   encoder->get_codec_name(),
					   &m_start_time);
	    first = 1;
	  }
#ifdef DUMP_RAW_DATA_TO_FILE
	  debug_message("Writing bytes to file");
	  write(ofile, buffer, size);
#endif
	  encoder->encode(buffer, size);
	}
	  
      }
      audio_recorder_task_process_msg(thread_stop, record);
    }
    delete encoder;
    free(buffer);
    close(audio_fd);
    audio_fd = 0;

  }
  return (0);
}

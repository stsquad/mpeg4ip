/*
 * The contents of this file are subject to the Mozilla Public
 * License Version 1.1 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.mozilla.org/MPL/
 * 
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 * 
 * The Original Code is MPEG4IP.
 * 
 * The Initial Developer of the Original Code is Cisco Systems Inc.
 * Portions created by Cisco Systems Inc. are
 * Copyright (C) Cisco Systems Inc. 2000, 2001.  All Rights Reserved.
 * 
 * Contributor(s): 
 *              Bill May        wmay@cisco.com
 */
/*
 * msg_queue.h - provides a basic, SDL based message passing routine
 */
#ifndef __MSG_QUEUE_H__
#define __MSG_QUEUE_H__ 1

#include <stdlib.h>
#include <SDL.h>
#include <SDL_thread.h>

class CMsg {
 public:
  CMsg(size_t value, unsigned char *msg = NULL, size_t msg_len = 0);
  ~CMsg(void);
  const unsigned char *get_message(size_t &len);
  CMsg *get_next(void) { return m_next; };
  void set_next (CMsg *next) { m_next = next; };
  size_t get_value(void) { return m_value;};
 private:
  CMsg *m_next;
  size_t m_value;
  size_t m_msg_len;
  unsigned char *m_msg;
};

class CMsgQueue {
 public:
  CMsgQueue(void);
  ~CMsgQueue(void);
  int send_message(size_t msgval,
		   unsigned char *msg = NULL,
		   size_t msg_len = 0,
		   SDL_sem *sem = NULL);
  CMsg *get_message(void);
 private:
  CMsg *m_msg_queue;
  SDL_mutex *m_msg_queue_mutex;
};

#define MSG_SESSION_FINISHED 1
#define MSG_PAUSE_SESSION 2
#define MSG_START_SESSION 3
#define MSG_STOP_THREAD 4
#define MSG_START_DECODING 5
#define MSG_SYNC_RESIZE_SCREEN 6  
#endif

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
 * msg_queue.cpp - generic class to send/receive messages.  Uses SDL mutexs
 * to protect queues
 */
#include "msg_queue.h"

/*****************************************************************************
 * CMsg class methods.  Defines information about a single message
 *****************************************************************************/
CMsg::CMsg (uint32_t value, const void *msg, uint32_t msg_len, uint32_t param)
{
  m_value = value;
  m_msg_len = 0;
  m_has_param = (param != 0);
  m_param = param;
  m_next = NULL;

  if (msg_len) {
    void *temp = malloc(msg_len);
    if (temp) {
      memcpy(temp, msg, msg_len);
      m_msg_len = msg_len;
    }
    m_msg = temp;
  } else {
    m_msg = msg;
  }
}

CMsg::CMsg (uint32_t value, uint32_t param)
{
  m_value = value;
  m_msg_len = 0;
  m_has_param = 1;
  m_param = param;
  m_next = NULL;
}

CMsg::~CMsg (void) 
{
  if (m_msg_len) {
    free((void *)m_msg);
    m_msg = NULL;
  }
}

const void *CMsg::get_message (uint32_t &len)
{
  len = m_msg_len;
  return (m_msg);
}

/*****************************************************************************
 * CMsgQueue class methods.  Defines information about a message queue
 *****************************************************************************/
CMsgQueue::CMsgQueue(void)
{
  m_msg_queue = NULL;
  m_msg_queue_mutex = SDL_CreateMutex();
}

CMsgQueue::~CMsgQueue (void) 
{
  CMsg *p;
  SDL_LockMutex(m_msg_queue_mutex);
  while (m_msg_queue != NULL) {
    p = m_msg_queue->get_next();
    delete m_msg_queue;
    m_msg_queue = p;
  }
  SDL_UnlockMutex(m_msg_queue_mutex);
  SDL_DestroyMutex(m_msg_queue_mutex);
  m_msg_queue_mutex = NULL;
}

int CMsgQueue::send_message (uint32_t msgval, 
			     const void *msg, 
			     uint32_t msg_len, 
			     SDL_sem *sem,
			     uint32_t param)
{
  CMsg *newmsg = new CMsg(msgval, msg, msg_len, param);

  if (newmsg == NULL) 
    return (-1);
  return (send_message(newmsg, sem));
}

int CMsgQueue::send_message (uint32_t msgval, uint32_t param, SDL_sem *sem)
{
	CMsg *newmsg = new CMsg(msgval, param);

	if (newmsg == NULL) return -1;

	return (send_message(newmsg, sem));
}

int CMsgQueue::send_message(CMsg *newmsg, SDL_sem *sem)
{

  SDL_LockMutex(m_msg_queue_mutex);
  if (m_msg_queue == NULL) {
    m_msg_queue = newmsg;
  } else {
    CMsg *p = m_msg_queue;
    while (p->get_next() != NULL) p = p->get_next();
    p->set_next(newmsg);
  }
  SDL_UnlockMutex(m_msg_queue_mutex);
  if (sem != NULL) {
    SDL_SemPost(sem);
  }
  return (0);
}

CMsg *CMsgQueue::get_message (void) 
{
  CMsg *ret;

  if (m_msg_queue == NULL) 
    return(NULL);

  SDL_LockMutex(m_msg_queue_mutex);
  if (m_msg_queue == NULL) 
    ret = NULL;
  else {
    ret = m_msg_queue;
    m_msg_queue = ret->get_next();
  }
  SDL_UnlockMutex(m_msg_queue_mutex);
  if (ret) {
    ret->set_next(NULL);
  }
  return (ret);
}
  
/* end file msg_queue.cpp */

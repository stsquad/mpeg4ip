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
 * Copyright (C) Cisco Systems Inc. 2000-2002.  All Rights Reserved.
 * 
 * Contributor(s): 
 *		Dave Mackie		dmackie@cisco.com
 *		Bill May 		wmay@cisco.com
 */

#include "mp4live.h"
#include "text_source.h"

//#define DEBUG_TIMESTAMPS 1
CFileTextSource::CFileTextSource(CLiveConfig *pConfig) : CTextSource(pConfig) 
{
  SetConfig(pConfig);
  m_started = false;
  m_infile = fopen(pConfig->GetStringValue(CONFIG_TEXT_SOURCE_FILE_NAME), 
					   "r");
  if (m_infile == NULL) {
    error_message("Couldn't open file");
  }
}

int CFileTextSource::ThreadMain(void) 
{
  debug_message("text start");
  m_source = false;
  int rc = 0;
  while (true) {
    if (m_started) {
      Timestamp now;
      Timestamp compare;
      bool cont;
      do {
	now = GetTimestamp();
	compare = m_start_timestamp + m_buffer_timestamp;
	if (now >= compare) {
	  SendFrame();
	  ReadLine();
	  cont = true && m_started;
	} else {
	  Duration diff = compare - now;
	  diff /= TO_U64(1000);
	  uint32_t wait = (uint32_t)diff;
	  if (wait == 0) wait = 1;
	  rc = SDL_SemWaitTimeout(m_myMsgQueueSemaphore, wait);
	  cont = false;
	} 
      } while (cont);

    } else {
      rc = SDL_SemWait(m_myMsgQueueSemaphore);
    }

    // semaphore error
    if (rc == -1) {
      break;
    } 

    // message pending
    if (rc == 0) {
      CMsg* pMsg = m_myMsgQueue.get_message();
		
      if (pMsg != NULL) {
        switch (pMsg->get_value()) {
        case MSG_NODE_STOP_THREAD:
          DoStopCapture();	// ensure things get cleaned up
          delete pMsg;
	  debug_message("text stop thread");
          return 0;
        case MSG_NODE_START:
          DoStartCapture();
          break;
        case MSG_NODE_STOP:
          DoStopCapture();
          break;
        }

        delete pMsg;
      }
    }
  }

  debug_message("text thread exit");
  return -1;
}

void CFileTextSource::DoStartCapture()
{
  if (m_source) {
    return;
  }

  if (!Init()) {
    return;
  }

  m_source = true;
}

void CFileTextSource::DoStopCapture()
{
  if (!m_source) {
    return;
  }

  m_source = false;
}

bool CFileTextSource::Init(void)
{
  m_started = true;
  ReadLine();
  m_start_timestamp = GetTimestamp();
  return true;
}

void CFileTextSource::ReadLine (void) 
{
  if (fgets(m_buffer, PATH_MAX, m_infile) == NULL) {
    m_started = false;
    error_message("Readline failed");
    return;
  }
  float readit;
  int num_chars;
  int num = sscanf(m_buffer, "%f %n", &readit, &num_chars);
  if (num != 1) {
    m_started = false;
    error_message("Can't sscanf %d", num);
    return;
  }
  readit *= TimestampTicks;
  m_buffer_timestamp = (Duration)readit;
  m_bufptr = m_buffer + num_chars;
  ADV_SPACE(m_bufptr);
  debug_message("read for "U64" \"%s\"",
		m_buffer_timestamp, 
		m_bufptr);
}

void CFileTextSource::SendFrame (void) 
{
  size_t buflen = strlen(m_bufptr);

  if (buflen == 0) 
    return;

  debug_message("sending "U64,
		m_buffer_timestamp);

  CMediaFrame *mf = new CMediaFrame(PLAINTEXTFRAME,
				    strdup(m_bufptr),
				    buflen + 1,
				    m_buffer_timestamp + m_start_timestamp);
  //debug_message("source fwd %p", mf);
  ForwardFrame(mf);
}

CDialogTextSource::CDialogTextSource(CLiveConfig *pConfig) : CTextSource(pConfig) 
{
  SetConfig(pConfig);
}

int CDialogTextSource::ThreadMain(void) 
{
  debug_message("text start");
  m_source = false;
  int rc = 0;
  while (true) {
    rc = SDL_SemWait(m_myMsgQueueSemaphore);
    // semaphore error
    if (rc == -1) {
      break;
    } 

    // message pending
    if (rc == 0) {
      CMsg* pMsg = m_myMsgQueue.get_message();
		
      if (pMsg != NULL) {
        switch (pMsg->get_value()) {
        case MSG_NODE_STOP_THREAD:
          DoStopCapture();	// ensure things get cleaned up
          delete pMsg;
	  debug_message("text stop thread");
          return 0;
        case MSG_NODE_START:
          DoStartCapture();
          break;
        case MSG_NODE_STOP:
          DoStopCapture();
          break;
        }

        delete pMsg;
      }
    }
  }

  debug_message("text thread exit");
  return -1;
}

void CDialogTextSource::DoStartCapture()
{
  if (m_source) {
    return;
  }

  if (!Init()) {
    return;
  }

  m_source = true;
}

void CDialogTextSource::DoStopCapture()
{
  if (!m_source) {
    return;
  }

  m_source = false;
}

bool CDialogTextSource::Init(void)
{
  m_started = true;

  return true;
}

bool CDialogTextSource::SourceString (const char *string)
{
  CMediaFrame *mf = new CMediaFrame(PLAINTEXTFRAME,
				    strdup(string),
				    strlen(string) + 1,
				    GetTimestamp());
  if (mf != NULL)
    ForwardFrame(mf);
  return mf != NULL;
}


void DisplayTextSource (CLiveConfig *pConfig, char *buffer, uint32_t buflen)
{
  const char *type = pConfig->GetStringValue(CONFIG_TEXT_SOURCE_TYPE);
  if (type == NULL) {
    snprintf(buffer, buflen, "text source type not set");
    return;
  }

  if (strcmp(type, TEXT_SOURCE_TIMED_FILE) == 0) {
    snprintf(buffer, buflen, "From file with timing info:%s", 
	     pConfig->GetStringValue(CONFIG_TEXT_SOURCE_FILE_NAME));
  } else if (strcmp(type, TEXT_SOURCE_DIALOG) == 0) {
    snprintf(buffer, buflen, "From GUI dialog");
  } else if (strcmp(type, TEXT_SOURCE_FILE_WITH_DIALOG) == 0) {
    snprintf(buffer, buflen, "GUI using file:%s", 
	     pConfig->GetStringValue(CONFIG_TEXT_SOURCE_FILE_NAME));
  } else {
    snprintf(buffer, buflen, "Unknown type %s", type);
  } 
}

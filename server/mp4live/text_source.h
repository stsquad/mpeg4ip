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
 * Copyright (C) Cisco Systems Inc. 2005.  All Rights Reserved.
 * 
 * Contributor(s): 
 *		Bill May 		wmay@cisco.com
 */

#ifndef __TEXT_SOURCE_H__
#define __TEXT_SOURCE_H__

#include "media_source.h"

class CTextSource : public CMediaSource
{
 public:
  CTextSource(CLiveConfig *pConfig) : CMediaSource() {
    SetConfig(pConfig);
  };

  virtual bool SourceString(const char *string) { return false; };
  bool IsDone() {
    return false;	// live capture device is inexhaustible
  };

  float GetProgress() {
    return 0.0;		// live capture device is inexhaustible
  }

  virtual const char* name() {
    return "CTextSource";
  }
};
  
class CFileTextSource : public CTextSource
{
 public:
  CFileTextSource(CLiveConfig *pConfig);

  ~CFileTextSource() {
  };

  virtual const char* name() {
    return "CFileTextSource";
  }


 protected:
  int ThreadMain();

  void DoStartCapture();
  void DoStopCapture();

  bool Init();

  void ReadLine(void);
  void SendFrame(void);
 protected:
  bool m_started;
  FILE *m_infile;
  char m_buffer[PATH_MAX];
  char *m_bufptr;
  Duration m_buffer_timestamp;
  Timestamp m_start_timestamp;
};

class CDialogTextSource : public CTextSource
{
 public:
  CDialogTextSource(CLiveConfig *pConfig);

  ~CDialogTextSource() {
  };

  bool SourceString(const char *string);

 protected:
  int ThreadMain();

  void DoStartCapture();
  void DoStopCapture();

  bool Init();

  void ReadLine(void);
  void SendFrame(void);
 protected:
  bool m_started;
};

void DisplayTextSource(CLiveConfig *pConfig, char *buffer, uint32_t buflen);

#endif /* __TEXT_SOURCE_SOURCE_H__ */

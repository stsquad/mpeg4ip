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
 *              Bill May        wmay@cisco.com
 */

#ifndef __TEXT_H__
#define __TEXT_H__ 1

#include "sync.h"

class CTextRenderer
{
 public:
  CTextRenderer(void) {};
  virtual ~CTextRenderer(void) {};
  virtual uint32_t initialize(void) = 0;
  virtual void load_frame(uint32_t fill_index, 
			  uint32_t disp_type, 
			  void *disp_struct) = 0;
  virtual void render(uint32_t play_index) = 0;
  virtual void flush(void) { };
};

class CTimedTextSync : public CTimedSync
{
 public:
  CTimedTextSync(CPlayerSession *psptr);

  ~CTimedTextSync(void);

  // base class required routines
  session_sync_types_t get_sync_type (void) { return TIMED_TEXT_SYNC; };
  int initialize(const char *name);
  bool is_ready(uint64_t &disptime);
  // apis from plugin
  void configure(uint32_t display_type, void *display_config);
  void have_frame(uint64_t ts, uint32_t display_type, void *disp_struct);
  void flush(void) { 
    if (m_render_device != NULL) 
      m_render_device->flush(); 
  };
  bool active_at_start(void) { return false; };
 protected:
  // base class routines
  void render(uint32_t play_index);
  uint32_t m_display_type;
  CTextRenderer *m_render_device;
};
  

class CPlainTextRender : public CTextRenderer
{
 public:
  CPlainTextRender(void *nothing);

  ~CPlainTextRender(void) {
    for (uint ix = 0; ix < 8; ix++) {
      CHECK_AND_FREE(m_buffer_list[ix]);
    }
    free(m_buffer_list);
    m_buffer_list = NULL;
  };

  uint32_t initialize (void) {
    m_buffer_list = (char **)malloc(8 * sizeof(char *));
    return 8;
  };

  void load_frame(uint32_t fill_index, uint32_t disp_type, void *disp_struct); 
  void render(uint32_t play_index);
 protected:
  char **m_buffer_list;
};

CTimedTextSync *create_text_sync(CPlayerSession *psptr);

text_vft_t *get_text_vft(void);
#endif

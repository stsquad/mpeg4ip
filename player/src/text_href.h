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

#ifndef __TEXT_HREF_H__
#define __TEXT_HREF__ 1

#include "text.h"
#include "text_plugin.h"

#define HREF_BUFFER_SIZE 8
#ifndef _WIN32
typedef struct pid_list_t {
  pid_list_t *next, *prev;
  pid_t pid_value;
} pid_list_t;
#endif

class CHrefTextRenderer : public CTextRenderer
{
 public:
  CHrefTextRenderer(CPlayerSession *psptr, void *disp_config);
  ~CHrefTextRenderer(void);
  uint32_t initialize(void);
  void load_frame(uint32_t fill_index, 
		  uint32_t disp_type, 
		  void *disp_struct);
  void render(uint32_t play_index);
  void flush(void);
  void process_click(uint16_t x, uint16_t y);
 protected:
  CPlayerSession *m_psptr;
  href_display_structure_t *m_buffer_list;
  bool m_clickable;
  href_display_structure_t m_current, m_click_info;
  void clear_href_structure(href_display_structure_t *hptr);
  void dispatch(const char *url);
#ifndef _WIN32
  pid_list_t *m_pid_list;
#endif
  bool clean_up(void);
  void finish(void);
};

#endif

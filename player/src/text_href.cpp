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

#include "text_href.h"
#include "mpeg4ip_utils.h"
#ifndef _WIN32
#include <sys/wait.h>
#include <signal.h>
#endif
#include "our_config_file.h"

static void c_mouse_click (void *ifptr, uint16_t x, uint16_t y)
{
  CHrefTextRenderer *h = (CHrefTextRenderer *)ifptr;

  h->process_click(x, y);
}

CHrefTextRenderer::CHrefTextRenderer (CPlayerSession *psptr, void *disp_config)
{
  m_psptr = psptr;
  m_buffer_list = NULL;
  m_clickable = false;
  memset(&m_current, 0, sizeof(m_current));
#ifndef _WIN32
  m_pid_list = NULL;
#endif
}

CHrefTextRenderer::~CHrefTextRenderer (void)
{
  flush();
  CHECK_AND_FREE(m_buffer_list);
  finish();
}

uint32_t CHrefTextRenderer::initialize (void)
{
  uint32_t size = HREF_BUFFER_SIZE * sizeof(href_display_structure_t);
  m_buffer_list = (href_display_structure_t *)malloc(size);
  m_psptr->register_mouse_click_callback(c_mouse_click, this);
  memset(m_buffer_list, 0, size);
  return HREF_BUFFER_SIZE;
}

void CHrefTextRenderer::load_frame (uint32_t fill_index, 
				uint32_t disp_type, 
				void *disp_struct)
{
  if (disp_type != TEXT_DISPLAY_TYPE_HREF) {
    message(LOG_ERR, "href", "Expecting type %u got type %u in text render", 
			 TEXT_DISPLAY_TYPE_HREF, disp_type);
    return;
  }
  href_display_structure_t *href = (href_display_structure_t *)disp_struct;
  //message(LOG_INFO, "href", "filled %s", href->url);
  m_buffer_list[fill_index].url = 
    href->url != NULL ? strdup(href->url) : NULL;
  m_buffer_list[fill_index].target_element = 
    href->target_element != NULL ? strdup(href->target_element) : NULL;
  m_buffer_list[fill_index].embed_element = 
    href->embed_element != NULL ? strdup(href->embed_element) : NULL;
  m_buffer_list[fill_index].send_click_location = href->send_click_location;
  m_buffer_list[fill_index].auto_dispatch = href->auto_dispatch;
}

void CHrefTextRenderer::clear_href_structure (href_display_structure_t *hptr)
{
  CHECK_AND_FREE(hptr->url);
  CHECK_AND_FREE(hptr->target_element);
  CHECK_AND_FREE(hptr->embed_element);
}

void CHrefTextRenderer::flush (void)
{
  for (uint ix = 0; ix < HREF_BUFFER_SIZE; ix++) {
    clear_href_structure(&m_buffer_list[ix]);
  }
  clear_href_structure(&m_current);
  if (m_clickable) {
    m_clickable = false;
    clear_href_structure(&m_click_info);
    m_psptr->set_cursor(false);
    // clear player session
  }
}

bool compare_href_displays (href_display_structure_t *s1,
			    href_display_structure_t *s2)
{
  if (s1->url == NULL || s2->url == NULL) return false;
  if (strcmp(s1->url, s2->url) != 0) return false;

  if (s1->target_element == NULL && s2->target_element != NULL) return false;
  if (s1->target_element != NULL && s2->target_element == NULL) return false;
  if (s1->embed_element == NULL && s2->embed_element != NULL) return false;
  if (s1->embed_element != NULL && s2->embed_element == NULL) return false;

  if (s1->target_element != NULL) 
    if (strcmp(s1->target_element, s2->target_element) != 0) return false;
  if (s1->embed_element != NULL) 
    if (strcmp(s1->embed_element, s2->embed_element) != 0) return false;
  return true;
}
	


void CHrefTextRenderer::render (uint32_t play_index)
{
  if (m_clickable) {
    m_clickable = false;
    m_psptr->set_cursor(false);
    clear_href_structure(&m_click_info);
    if (m_buffer_list[play_index].auto_dispatch) {
      // clear out psptr clickable
    }
  }

  if (m_buffer_list[play_index].auto_dispatch) {
    if (compare_href_displays(&m_current, 
			      &m_buffer_list[play_index]) == false) {
      // render url
      dispatch(m_buffer_list[play_index].url);
    }

    clear_href_structure(&m_current);
    m_current.url = m_buffer_list[play_index].url;
    m_current.target_element = m_buffer_list[play_index].target_element;    
    m_current.embed_element = m_buffer_list[play_index].embed_element;
  } else {
    m_clickable = true;
    m_click_info.url = m_buffer_list[play_index].url;
    m_click_info.target_element = m_buffer_list[play_index].target_element;    
    m_click_info.embed_element = m_buffer_list[play_index].embed_element;
    m_click_info.send_click_location = m_buffer_list[play_index].send_click_location;
    message(LOG_INFO, "href", "click url \"%s\" %u", m_click_info.url,
	    m_click_info.send_click_location);

    m_psptr->set_cursor(true);
  }
  m_buffer_list[play_index].url = NULL;
  m_buffer_list[play_index].target_element = NULL;    
  m_buffer_list[play_index].embed_element = NULL;
}

void CHrefTextRenderer::dispatch (const char *url)
{
  message(LOG_INFO, "href", "dispatch \"%s\"", url);
#ifdef _WIN32
  ::ShellExecute(NULL, "open", url, NULL, NULL, SW_SHOW);
#else
  pid_t pid = fork();
  if (pid > 0) {
    pid_list_t *pl = MALLOC_STRUCTURE(pid_list_t);
    pl->next = NULL;
    pl->prev = NULL;
    pl->pid_value = pid;
    if (m_pid_list == NULL) {
      m_pid_list = pl;
    } else {
      pl->next = m_pid_list;
      m_pid_list->prev = pl;
      m_pid_list = pl;
    }
    clean_up();
  } else if (pid == 0) {
    const char *temp = config.get_config_string(CONFIG_URL_EXEC);
    if (temp == NULL) {
#ifdef HAVE_MACOSX
      temp = "/usr/bin/open";
#else 
      temp = "/usr/bin/firefox/firefox";
#endif
    } 
    execl(temp, temp, url, (void *)NULL);
  }
#endif
}

bool CHrefTextRenderer::clean_up (void)
{
#ifndef _WIN32
  pid_list_t *pl;
  pid_t wpid = waitpid(0, NULL, WNOHANG);
  if (wpid > 0) {
    pl = m_pid_list;
    while (pl != NULL && pl->pid_value != wpid) {
      pl = pl->next;
    }
    if (pl != NULL) {
      if (pl == m_pid_list) {
	m_pid_list = m_pid_list->next;
	if (m_pid_list != NULL)
	  m_pid_list->prev = NULL;
      } else {
	if (pl->prev != NULL) 
	  pl->prev->next = pl->next;
	if (pl->next != NULL)
	  pl->next->prev = pl->prev;
      }
      free(pl);
    } else {
      message(LOG_ERR, "href", "pid not found in list %u", wpid);
    }
    return true;
  }
#endif
  return false;
}
void CHrefTextRenderer::finish (void)
{
  while (clean_up());

#ifndef _WIN32
  while (m_pid_list != NULL) {
    message(LOG_INFO, "href", "killing pid %u", m_pid_list->pid_value);
    kill(m_pid_list->pid_value, SIGKILL);
    pid_list_t *p = m_pid_list->next;
    free(m_pid_list);
    m_pid_list = p;
  }
#endif
}
      
void CHrefTextRenderer::process_click (uint16_t x, uint16_t y)
{
  message(LOG_DEBUG, "href", "got click %u x %u", x, y);
  if (m_clickable == false) {
    return;
  }
  clear_href_structure(&m_current);
  m_current.url = strdup(m_click_info.url);
  m_current.target_element = m_click_info.target_element == NULL ? NULL : 
    strdup(m_click_info.target_element);    
  m_current.embed_element = m_click_info.embed_element == NULL ? NULL : 
    strdup(m_click_info.embed_element);

  if (m_click_info.send_click_location) {
    char url[PATH_MAX];
    char add_char;
    if (strchr(url, '?') != NULL) {
      add_char = '&';
    } else {
      add_char = '?';
    }
    snprintf(url, PATH_MAX, "%s%cxclick=%u&yclick=%u", m_click_info.url, add_char, x, y);
    dispatch(url);
  } else {
    dispatch(m_click_info.url);
  }
}

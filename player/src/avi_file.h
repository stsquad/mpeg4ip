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
 * qtime_file.h - contains interfacing to quick time files for local playback
 */
#ifndef __AVI_FILE_H__
#define __AVI_FILE_H__ 1
#include "avilib.h"
#include <SDL.h>
#include <SDL_thread.h>

int create_media_for_avi_file (CPlayerSession *psptr,
				 const char *name,
				 const char **errmsg);

/*
 * CAviFile contains access information for the avi bytestream
 * classes.  It will provide mutual exclusion, so bytestreams in different
 * threads don't overlap
 */
class CAviFile {
 public:
  CAviFile(const char *name, avi_t *avi);
  ~CAviFile();
  void lock_file_mutex (void) { SDL_mutexP(m_file_mutex); };
  void unlock_file_mutex (void) { SDL_mutexV(m_file_mutex); };
  int create_audio (CPlayerSession *psptr);
  int create_video (CPlayerSession *psptr);
  avi_t *get_file(void) {return m_file; };
  int get_audio_tracks (void) { return m_audio_tracks; };
  int get_video_tracks (void) { return m_video_tracks; };
 private:
  char *m_name;
  avi_t *m_file;
  SDL_mutex *m_file_mutex;
  int m_video_tracks;
  int m_audio_tracks;
};


#endif

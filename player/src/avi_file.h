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
#include "avi/avilib.h"
#include "mpeg4ip_sdl_includes.h"

class CPlayerSession;

struct control_callback_vft_t;
int create_media_for_avi_file (CPlayerSession *psptr,
			       const char *name,
			       int have_audio_driver,
			       control_callback_vft_t *);

/*
 * CAviFile contains access information for the avi bytestream
 * classes.  It will provide mutual exclusion, so bytestreams in different
 * threads don't overlap
 */
class CAviFile {
 public:
  CAviFile(const char *name, avi_t *avi, int audio_tracks, int video_tracks);
  ~CAviFile();
  void lock_file_mutex (void) { SDL_mutexP(m_file_mutex); };
  void unlock_file_mutex (void) { SDL_mutexV(m_file_mutex); };
  avi_t *get_file(void) {return m_file; };
 private:
  char *m_name;
  avi_t *m_file;
  SDL_mutex *m_file_mutex;
  int m_video_tracks;
  int m_audio_tracks;
};


#endif

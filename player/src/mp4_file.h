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
 * mp4_file.h - contains interfacing to mp4 files for local playback
 */
#ifndef __MP4_FILE_H__
#define __MP4_FILE_H__ 1
#include <mp4.h>
#include "mpeg4ip_sdl_includes.h"

class CPlayerSession;

struct control_callback_vft_t;
struct video_query_t;
struct audio_query_t;
struct text_query_t;

int create_media_for_mp4_file(CPlayerSession *psptr,
			      const char *name,
			      int have_audio_driver,
			      control_callback_vft_t *);

/*
 * CMp4File contains access information for the mp4 bytestream
 * classes.  It will provide mutual exclusion, so bytestreams in different
 * threads don't overlap
 */
class CMp4File {
 public:
  CMp4File(MP4FileHandle fh);
  ~CMp4File();
  void lock_file_mutex (void) { SDL_mutexP(m_file_mutex); };
  void unlock_file_mutex (void) { SDL_mutexV(m_file_mutex); };
  int create_media (CPlayerSession *psptr,
		    int have_audio_driver, control_callback_vft_t *);
  MP4FileHandle get_file(void) {return m_mp4file; };
  int get_illegal_audio_codec (void) { return m_illegal_audio_codec;};
  int get_illegal_video_codec (void) { return m_illegal_video_codec;};
  bool have_audio (void) { return m_have_audio; };
  int create_video(CPlayerSession *psptr, video_query_t *vq, uint video_offset,
		   uint &start_desc);
  int create_audio(CPlayerSession *psptr, audio_query_t *vq, uint audio_offset,
		   uint &start_desc);
  int create_text(CPlayerSession *psptr, text_query_t *tq, uint text_offset, 
		  uint &start_desc);
 private:
  MP4FileHandle m_mp4file;
  SDL_mutex *m_file_mutex;
  uint m_illegal_audio_codec;
  uint m_illegal_video_codec;
  uint m_illegal_text_codec;
  bool m_have_audio;
};


#endif

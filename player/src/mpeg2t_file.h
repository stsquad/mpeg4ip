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
 * Copyright (C) Cisco Systems Inc. 2004.  All Rights Reserved.
 * 
 * Contributor(s): 
 *              Bill May        wmay@cisco.com
 */
#ifndef __MPEG2T_FILE_H__
#define __MPEG2T_FILE_H__
#include "mpeg4ip.h"
#include "mpeg2t/mpeg2_transport.h"
#include "mpeg2t_private.h"
#include "fposrec/fposrec.h"

class CMpeg2tFile {
 public:
  CMpeg2tFile(const char *name, FILE *file);

  ~CMpeg2tFile(void);
  void get_frame_for_pid(mpeg2t_es_t *es_pid);
  int create(CPlayerSession *psptr);
  int create_media(CPlayerSession *psptr,
		   control_callback_vft_t *cc_vft);
  int create_video(CPlayerSession *psptr,
		   mpeg2t_t *info,
		   video_query_t *vq,
		   uint video_count,
		   int &sdesc);
  int create_audio (CPlayerSession *psptr,
		    mpeg2t_t *decoder,
		    audio_query_t *aq,
		    uint audio_offset,
		    int &sdesc);
  int eof(void);
  uint64_t get_start_psts (void) { return m_start_psts; } ;
  double get_max_time (void) { return m_max_time; } ;
  void seek_to(uint64_t ts_in_msec);
 private:
  void lock_file_mutex(void) { SDL_LockMutex(m_file_mutex); };  
  void unlock_file_mutex(void) { SDL_UnlockMutex(m_file_mutex); };
  FILE *m_ifile;
  uint8_t *m_buffer;
  uint32_t m_buffer_size_max;
  uint32_t m_buffer_size;
  uint32_t m_buffer_on;
  SDL_mutex *m_file_mutex;
  mpeg2t_t *m_mpeg2t;

  uint64_t m_start_psts;
  uint64_t m_last_psts;
  double m_max_time;
  CFilePosRecorder m_file_record;
  uint64_t m_ts_seeked_in_msec;
};

int create_media_for_mpeg2t_file (CPlayerSession *psptr, 
				  const char *name,
				  int have_audio_driver, 
				  control_callback_vft_t *cc_vft);

#endif

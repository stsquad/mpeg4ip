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
 * audio.h - provides a class that interfaces between the codec and
 * the SDL audio application.  Will provide for volume, buffering,
 * syncronization
 */

#ifndef __AUDIO_SDL_H__
#define __AUDIO_SDL_H__ 1

#include "audio_buffer.h"
#include <portaudio.h>

class CPortAudioSync : public CBufferAudioSync {
 public:
  CPortAudioSync(CPlayerSession *psptr, int volume);
  ~CPortAudioSync(void);

  int InitializeHardware(void);
  void StartHardware(void);
  void StopHardware(void);
  void CopyBytesToHardware(uint8_t *from,
			   uint8_t *to, 
			   uint32_t bytes);
    
  void set_volume(int volume);
  void audio_callback(void *stream, unsigned long len,
		      PaTimestamp outtime);
 private:
  PortAudioStream *m_pa_stream;
  int m_use_SDL_delay;
};

#endif



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
 * Copyright (C) Cisco Systems Inc. 2000-2005.  All Rights Reserved.
 * 
 * Contributor(s): 
 *              Bill May        wmay@cisco.com
 */
/*
 * audio_sdl.h - provides the SDL rendering from the CBufferAudioSync
 * class
 */

#ifndef __AUDIO_SDL_H__
#define __AUDIO_SDL_H__ 1

#include "audio_buffer.h"
#include "Our_SDL_audio.h"

class CSDLAudioSync : public CBufferAudioSync {
 public:
  CSDLAudioSync(CPlayerSession *psptr, int volume);
  ~CSDLAudioSync(void);

  bool Lock (void) {
    if (m_hardware_initialized) {
      Our_SDL_LockAudio();
      return true;
    }
    return false;
  };

  void Unlock (void) {
    Our_SDL_UnlockAudio();
  };

  int InitializeHardware(void);
  void StartHardware(void);
  void StopHardware(void);
  void CopyBytesToHardware(uint8_t *from,
			   uint8_t *to, 
			   uint32_t bytes);
    
  void set_volume(int volume);
  void audio_callback(Uint8 *stream, int len);
 private:
  SDL_AudioSpec m_obtained;
  int m_use_SDL_delay;
};

#endif



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
 * wav_file.cpp - create media structure for aac files
 */

#include "player_session.h"
#include "player_media.h"
#include "our_bytestream_mem.h"
#include "ourwav.h"
#include "wav_file.h"
#include "player_util.h"
 
    
int create_media_for_wav_file (CPlayerSession *psptr, 
			       const char *name,
			       const char **errmsg)
{
  CPlayerMedia *mptr;
  COurInByteStreamWav *mbyte;
  psptr->session_set_seekable(0);
  mptr = new CPlayerMedia;
  if (mptr == NULL) {
    *errmsg = "Couldn't create media";
    return (-1);
  }

  SDL_AudioSpec *temp, *read;
  Uint8 *wav_buffer;
  Uint32 wav_len;

  temp = (SDL_AudioSpec *)malloc(sizeof(SDL_AudioSpec));
  read = SDL_LoadWAV(name, temp, &wav_buffer, &wav_len);
  if (read == NULL) {
    *errmsg = "Invalid wav file";
  }
  player_debug_message("Wav got f %d chan %d format %x samples %d size %u",
		       temp->freq,
		       temp->channels,
		       temp->format,
		       temp->samples,
		       temp->size);
  player_debug_message("Wav read %d bytes", wav_len);

  mbyte = new COurInByteStreamWav(wav_buffer, 
				  wav_len);
  if (mbyte == NULL) {
    *errmsg = "Couldn't create byte stream";
    return (-1);
  }
  /*
   * This is not necessarilly true - we may want to read the aac, then
   * read the adts header for it.
   */
  mbyte->config_frame_per_sec(temp->freq / temp->samples);
  //  fbyte->config_for_file(freq / 1024);
  *errmsg = "Can't create thread";
  int ret =  mptr->create_from_file(psptr, mbyte, FALSE);
  if (ret != 0) {
    return (-1);
  }

  mptr->set_user_data((const unsigned char *)temp, sizeof(temp));
  mptr->set_codec_type("wav ");
  return (0);
}



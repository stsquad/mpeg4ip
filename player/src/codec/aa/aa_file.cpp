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
 * aa_file.cpp - create media structure for aac files
 */

#include "player_session.h"
#include "player_media.h"
#include "our_bytestream_file.h"
#include "aa.h"
#include "aa_file.h"

int create_media_for_aac_file (CPlayerSession *psptr, 
			       const char *name,
			       const char **errmsg)
{
  CPlayerMedia *mptr;
  COurInByteStreamFile *fbyte;
  int freq = 0;
  psptr->session_set_seekable(0);
  mptr = new CPlayerMedia;
  if (mptr == NULL) {
    *errmsg = "Couldn't create media";
    return (1);
  }

  /*
   * Read faad file to get the frequency.  We need to read at
   * least 1 frame to get the adts header.
   */
  faadAACInfo fInfo;
  short buffer[1024 * 8]; // temp buffer
  aac_decode_init_filestream(name);
  aac_decode_init(&fInfo);
  int ret = aac_decode_frame(buffer);
  if (ret > 0) {
    // decoded a frame - get sample rate from aac
    freq = aac_get_sample_rate();
    if (freq == 0) {
      *errmsg = "Couldn't determine AAC frame rate";
      return (1);
    }
  } 
  aac_decode_free();

  fbyte = new COurInByteStreamFile(mptr, name);
  if (fbyte == NULL) {
    *errmsg = "Couldn't create file stream";
    return (1);
  }
  /*
   * This is not necessarilly true - we may want to read the aac, then
   * read the adts header for it.
   */
  fbyte->config_for_file(freq / 1024);
  *errmsg = "Can't create thread";
  ret =  mptr->create_from_file(psptr, fbyte, FALSE);
  if (ret != 0) {
    return (1);
  }

  audio_info_t *audio = (audio_info_t *)malloc(sizeof(audio_info_t));
  audio->freq = freq;
  audio->stream_has_length = 0;
  mptr->set_audio_info(audio);
  player_debug_message("Freq set to %d", freq);
  return (0);
}



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

static unsigned int c_get (void *ud)
{
  CInByteStreamBase *byte_stream;

  byte_stream = (CInByteStreamBase *)ud;
  return (byte_stream->get());
}

static void c_bookmark (void *ud, int state)
{
  CInByteStreamBase *byte_stream;

  byte_stream = (CInByteStreamBase *)ud;
  byte_stream->bookmark(state);
}

int create_media_for_aac_file (CPlayerSession *psptr, 
			       const char *name,
			       const char **errmsg)
{
  CPlayerMedia *mptr;
  COurInByteStreamFile *fbyte;

  psptr->session_set_seekable(0);
  mptr = new CPlayerMedia;
  if (mptr == NULL) {
    *errmsg = "Couldn't create media";
    return (-1);
  }

  /*
   * Read faad file to get the frequency.  We need to read at
   * least 1 frame to get the adts header.
   */
  faacDecHandle fInfo;
  fInfo = faacDecOpen(AACMAIN, -1); // use defaults here...

  fbyte = new COurInByteStreamFile(name);
  if (fbyte == NULL) {
    *errmsg = "Couldn't create file stream";
    return (-1);
  }
  
  faad_init_bytestream(&fInfo->ld, c_get, c_bookmark, fbyte);
  unsigned long freq, chans;
  freq = 0;
  faacDecInit(fInfo, NULL, &freq, &chans);
  // may want to actually decode the first frame...
  if (freq == 0) {
    *errmsg = "Couldn't determine AAC frame rate";
    return (-1);
  } 
#if 0
  if (fInfo->adts_header) {
    player_debug_message("detected adts header - making seekable");
  }
#endif
  faacDecClose(fInfo);

  fbyte->config_for_file(freq / 1024);
  *errmsg = "Can't create thread";
  int ret =  mptr->create_from_file(psptr, fbyte, FALSE);
  if (ret != 0) {
    return (-1);
  }

  audio_info_t *audio = (audio_info_t *)malloc(sizeof(audio_info_t));
  audio->freq = freq;
  mptr->set_audio_info(audio);
  mptr->set_codec_type("aac ");
  return (0);
}



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
 * mp3_file.cpp - create media structure for mp3 files
 */

#include "player_session.h"
#include "player_media.h"
#include "our_bytestream_file.h"
#include "mp3.h"
#include "mp3_file.h"

static unsigned char c_read_byte (void *userdata)
{
  COurInByteStreamFile *fd = (COurInByteStreamFile *)userdata;
  return (fd->get());
}

static size_t c_read_bytes (unsigned char *b, size_t bytes, void *userdata)
{
  COurInByteStreamFile *fd = (COurInByteStreamFile *)userdata;
  return (fd->read(b, bytes));
}

int create_media_for_mp3_file (CPlayerSession *psptr, 
			       const char *name,
			       const char **errmsg)
{
  CPlayerMedia *mptr;
  COurInByteStreamFile *fbyte;
  int freq = 0, samplesperframe;
  int ret;

  psptr->session_set_seekable(0);
  mptr = new CPlayerMedia;
  if (mptr == NULL) {
    *errmsg = "Couldn't create media";
    return (-1);
  }

  fbyte = new COurInByteStreamFile(mptr, name);

  MPEGaudio *mp3 = new MPEGaudio(c_read_byte, c_read_bytes, fbyte);

  if (mp3->loadheader() == FALSE) {
    *errmsg = "Couldn't read MP3 header";
    delete mp3;
    delete mptr;
    return (-1);
  }
			
  freq = mp3->getfrequency();
  samplesperframe = 32;
  if (mp3->getlayer() == 3) {
    samplesperframe *= 18;
    if (mp3->getversion() == 0) {
      samplesperframe *= 2;
    }
  } else {
    samplesperframe *= SCALEBLOCK;
    if (mp3->getlayer() == 2) {
      samplesperframe *= 3;
    }
  }
  delete mp3;

  player_debug_message("freq %d samples %d fps %d", freq, samplesperframe, 
		       freq / samplesperframe);
  fbyte->config_for_file(freq / samplesperframe);

  *errmsg = "Can't create thread";
  ret =  mptr->create_from_file(psptr, fbyte, FALSE);
  if (ret != 0) {
    return (-1);
  }

  audio_info_t *audio = (audio_info_t *)malloc(sizeof(audio_info_t));
  audio->freq = freq;
  audio->stream_has_length = 0;
  mptr->set_audio_info(audio);
  mptr->set_codec_type("mp3 ");
  return (0);
}

/* end file mp3_file.cpp */



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
 * mpeg4_file.cpp
 * Create media structures for session for an mp4v file (raw mpeg4)
 */
#include "player_session.h"
#include "player_media.h"
#include "our_bytestream_file.h"
#include "mpeg4.h"
#include "mpeg4_file.h"
#include "type/typeapi.h"
#include "sys/mode.hpp"
#include "sys/vopses.hpp"
//#include "sys/tps_enhcbuf.hpp"
//#include "sys/decoder/enhcbufdec.hpp"
#include "sys/decoder/vopsedec.hpp"
#include "player_util.h"

int create_media_for_mpeg4_file (CPlayerSession *psptr, 
				 const char *name,
				 const char **errmsg)
{
  CPlayerMedia *mptr;
  COurInByteStreamFile *fbyte;
  CVideoObjectDecoder *vdec;
  int dummy1, dummy2;
  vdec = new CVideoObjectDecoder(name, -1, -1, &dummy1, &dummy2);
  Int clockrate = vdec->getClockRate();
  delete vdec;

  psptr->session_set_seekable(0);
  mptr = new CPlayerMedia;
  if (mptr == NULL) {
    *errmsg = "Couldn't create media";
    return (-1);
  }

  fbyte = new COurInByteStreamFile(name);
  if (fbyte == NULL) {
    *errmsg = "Couldn't create file";
    return (-1);
  }

  fbyte->config_for_file(clockrate);
  player_debug_message("Configuring for frame rate %d", clockrate);
  *errmsg = "Couldn't create task";
  mptr->set_codec_type("mp4 ");
  return( mptr->create_from_file(psptr, fbyte, TRUE));
}


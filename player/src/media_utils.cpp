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
 * media_utils.cpp - various utilities, globals (ick).
 */
#include <stdlib.h>
#include "player_session.h"
#include "codec/aa/aa_file.h"
#include "codec/mpeg4/mpeg4_file.h"
#include "qtime_file.h"

/*
 * This needs to be global so we can store any ports that we don't
 * care about but need to reserve
 */
CIpPort *global_invalid_ports = NULL;

/*
 * create_media_for_streaming - create streaming media session
 */
static int create_media_for_streaming (CPlayerSession *psptr, 
				       const char *name,
				       const char **errmsg)
{
  int err;
  media_desc_t *sdp_media;

  /*
   * This will open the rtsp session
   */
  err = psptr->create_streaming(name, errmsg);
  if (err != 0) {
    return (1);
  }
  sdp_media = psptr->get_sdp_info()->media;
  while (sdp_media != NULL) {
    CPlayerMedia *mptr = new CPlayerMedia;
    err = mptr->create_streaming(psptr, sdp_media, errmsg);
    if (err != 0) {
      return (1);
    }
    sdp_media = sdp_media->next;
  }
  return (0);
}

/*
 * parse_name_for_session - look at the name, determine what routine to 
 * call to set up the session.  This should be redone with plugins at
 * some point.
 */
int parse_name_for_session (CPlayerSession *psptr,
			    const char *name,
			    const char **errmsg)
{
  int err;
  if (strncmp(name, "rtsp://", strlen("rtsp://")) == 0) {    
    err = create_media_for_streaming(psptr, name, errmsg);
    if (err != 0) {
      return (1);
    }
    return (0);
  }
  
  struct stat statbuf;
  if (stat(name, &statbuf) != 0) {
    *errmsg = "File not found";
    return (1);
  }
  if (!S_ISREG(statbuf.st_mode)) {
    *errmsg = "Not a file";
    return (1);
  }

  if (strstr(name, ".aac") != NULL) {
    err = create_media_for_aac_file(psptr, name, errmsg);
    if (err != 0) {
      return (1);
    }
  } else if ((strstr(name, ".mp4v") != NULL) ||
	     (strstr(name, ".cmp") != NULL)) {
    err = create_media_for_mpeg4_file(psptr, name, errmsg);
    if (err != 0) {
      return(1);
    }
  } else if ((strstr(name, ".mov") != NULL) ||
	     (strstr(name, ".mp4") != NULL)) {
    err = create_media_for_qtime_file(psptr, name, errmsg);
    if (err != 0) {
      return (1);
    }
  } else {
    *errmsg = "Illegal or unknown file type";
    player_error_message("Illegal or unknown file type - %s", name);
    return (1);
  }
  return (0);
}



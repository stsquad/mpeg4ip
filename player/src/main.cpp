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
 * This is a command line based player for testing the library
 */
#include "systems.h"
#include <rtsp/rtsp_client.h>
#include "player_session.h"
#include "player_util.h"
#include "our_msg_queue.h"
#include "ip_port.h"
#include "media_utils.h"

int main (int argc, char **argv)
{
  CPlayerSession *psptr;
  char *name;
  CMsgQueue master_queue;
  SDL_sem *master_sem;
  int loopcount = 0;
  int max_loop = 1;

  rtsp_set_loglevel(LOG_DEBUG);
  argv++;
  argc--;
  if (argc && strcmp(*argv, "-l") == 0) {
    argv++;
    argc--;
    max_loop = atoi(*argv);
    argc--;
    argv++;
  }
  master_sem = SDL_CreateSemaphore(0);
  if (*argv == NULL) {
    //name = "rtsp://171.71.233.210/bond_new.mov";
    //name = "rtsp://171.71.233.210/batman_a.mov";
    name = "/home/wmay/content/bond.mov";
  } else {
    name = *argv;
  }
  psptr = new CPlayerSession(&master_queue, master_sem,
			     // this should probably be name...
			     "Cisco Open Source MPEG4 Player");
  if (psptr == NULL) {
    return (-1);
  }
  
  const char *errmsg;
  int ret = parse_name_for_session(psptr, name, &errmsg);
  if (ret < 0) {
	player_debug_message(errmsg);
    delete psptr;
    return (1);
  }

  if (ret > 0) {
	  player_debug_message(errmsg);
  }

  psptr->set_up_sync_thread();
  psptr->set_screen_location(100, 100);
  psptr->set_screen_size(2);
  while (loopcount < max_loop) {
    loopcount++;
	player_debug_message("Starting");
    if (psptr->play_all_media(TRUE) != 0) {
      delete psptr;
      return (1);
    }

#ifdef _WINDOWS
	psptr->sync_thread();
#else
    int keep_going = 0;
    int paused = 0;
#ifdef DO_PAUSE
    int did_pause = 0;
#endif
    do {
      CMsg *msg;
      SDL_SemWaitTimeout(master_sem, 10000);
      while ((msg = master_queue.get_message()) != NULL) {
	switch (msg->get_value()) {
	case MSG_SESSION_FINISHED:
	  if (paused == 0)
	    keep_going = 1;
	  break;
	case MSG_RECEIVED_QUIT:
	  keep_going = 1;
	  break;
	}
	delete msg;
      }
#ifdef DO_PAUSE
      if (keep_going == 0) {
	if (did_pause == 0) {
	  if (paused == 0 ) {
	    player_debug_message("Pausing");
	    err = psptr->pause_all_media();
	    if (err < 0) {
	      delete psptr;
	      return (1);
	    }
	    paused = 1;
	  } else {
	    double play_time = psptr->get_playing_time();
#if 1
	    if (play_time > 2.0) play_time -= 2.0;
#else
	    play_time += 5.2;
#endif
	    player_debug_message("Playing at %g", play_time);
	    
	    err = psptr->play_all_media(FALSE, play_time);
	    if (err < 0) {
	      delete psptr;
	      return (1);
	    }
	    paused = 0;
	    did_pause = 1;
	  }
	}
      }
#endif
    } while (keep_going == 0);
#if 1
    if (loopcount != max_loop) {
      psptr->pause_all_media();
    }
#endif
#endif
  }
  delete psptr;
  SDL_DestroySemaphore(master_sem);

  // remove invalid global ports
  CIpPort *first;
  while (global_invalid_ports != NULL) {
    first = global_invalid_ports;
    global_invalid_ports = first->get_next();
    player_debug_message("Freeing invalid port %u", first->get_port_num());
    delete first;
  }
     
  return(0); 
}  
  

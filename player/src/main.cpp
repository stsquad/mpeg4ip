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
#include "mpeg4ip.h"
#include "codec_plugin_private.h"
#include <rtsp/rtsp_client.h>
#include "player_session.h"
#include "player_media.h"
#include "player_util.h"
#include "our_msg_queue.h"
#include "ip_port.h"
#include "media_utils.h"
#include "playlist.h"
#include "our_config_file.h"
#include <rtp/debug.h>
#include <libhttp/http.h>
#include "video.h"
#include "video_sdl.h"

static int session_paused;
static int screen_size = 2;
static int fullscreen = 0;

static int set_aspect_ratio(int newaspect, CPlayerSession *psptr)
{
  if (psptr != NULL) {
    switch (newaspect) {
      case 1 : 
	psptr->set_screen_size(screen_size,fullscreen,4,3);
        break;
      case 2 : 
	psptr->set_screen_size(screen_size,fullscreen,16,9);
        break;
      case 3 : 
	psptr->set_screen_size(screen_size,fullscreen,185,100);
        break;
      case 4 : 
	psptr->set_screen_size(screen_size,fullscreen,235,100);
        break;
      default: 
	psptr->set_screen_size(screen_size,fullscreen,0,0);
        newaspect = 0;
        break;
    }
    config.set_config_value(CONFIG_ASPECT_RATIO, newaspect);
  } else player_error_message("Can't set aspect ratio yet");
  return(newaspect);
}

int process_sdl_key_events (CPlayerSession *psptr,
		 				   sdl_event_msg_t *msg)
{
  int volume;
  uint64_t play_time;
  //printf("key %d mod %x\n", msg->sym, msg->mod);
  switch (msg->sym) {
  case SDLK_c:
    if ((msg->mod & KMOD_CTRL) != 0) {
      return 0;
    }
    break;
  case SDLK_x:
    if ((msg->mod & KMOD_CTRL) != 0) {
      return -1;
    }
  case SDLK_UP:
    volume = psptr->get_audio_volume();
    volume += 10;
    if (volume > 100) volume = 100;
    psptr->set_audio_volume(volume);
    config.set_config_value(CONFIG_VOLUME, volume);
    break;
  case SDLK_DOWN:
    volume = psptr->get_audio_volume();
    volume -= 10;
    if (volume < 0) volume = 0;
    psptr->set_audio_volume(volume);
    config.set_config_value(CONFIG_VOLUME, volume);
    break;
  case SDLK_SPACE:
    if (session_paused == 0) {
      psptr->pause_all_media();
      session_paused = 1;
    } else {
      if (psptr->play_all_media(FALSE) < 0) return -1;
      session_paused = 0;
    }
    break;
  case SDLK_END:
    // They want the end - just close, or go on to the next playlist.
    return 0;
  case SDLK_HOME:
    psptr->pause_all_media();
    if (psptr->play_all_media(TRUE, 0.0) < 0) return -1;
    break;
  case SDLK_RIGHT:
    if (psptr->session_is_seekable()) {
      play_time = psptr->get_playing_time();
      double ptime, maxtime;
      play_time += TO_U64(10000);
      ptime = UINT64_TO_DOUBLE(play_time);
      ptime /= 1000.0;
      maxtime = psptr->get_max_time();
      if (ptime < maxtime) {
	psptr->pause_all_media();
	if (psptr->play_all_media(FALSE, ptime) < 0) return -1;
      }
    }
    break;
  case SDLK_LEFT:
    if (psptr->session_is_seekable()) {
      play_time = psptr->get_playing_time();
      double ptime;
      if (play_time >= TO_U64(10000)) {
	play_time -= TO_U64(10000);
	ptime = UINT64_TO_DOUBLE(play_time);
	ptime /= 1000.0;
	psptr->pause_all_media();
	if (psptr->play_all_media(FALSE, ptime) < 0) return -1;
      }
    }
    break;
  case SDLK_PAGEUP:
    if (screen_size < 4 && fullscreen == 0) {
      screen_size *= 2;
      psptr->set_screen_size(screen_size);
    }
    break;
  case SDLK_PAGEDOWN:
    if (screen_size > 1 && fullscreen == 0) {
      screen_size /= 2;
      psptr->set_screen_size(screen_size);
    }
    break;
  case SDLK_RETURN:
    if ((msg->mod & (KMOD_ALT | KMOD_META)) != 0) {
		fullscreen = 1;
	}
	break;
  case SDLK_ESCAPE:
	  fullscreen = 0;
	  break;

  case SDLK_0:
  case SDLK_1:
  case SDLK_2:
  case SDLK_3:
  case SDLK_4:
    if ((msg->mod & KMOD_CTRL) != 0) {
      int newaspect = msg->sym - SDLK_0;
      config.set_config_value(CONFIG_ASPECT_RATIO, newaspect);
      switch (newaspect) {
      case 1 : 
	psptr->set_screen_size(screen_size,fullscreen,4,3);
	break;
      case 2 : 
	psptr->set_screen_size(screen_size,fullscreen,16,9);
	break;
      case 3 : 
	psptr->set_screen_size(screen_size,fullscreen,185,100);
	break;
      case 4 : 
	psptr->set_screen_size(screen_size,fullscreen,235,100);
	break;
      default: 
	psptr->set_screen_size(screen_size,fullscreen,0,0);
	newaspect = 0;
	break;
      }
    }
    break;
  default:
    break;
  }
  return 1;

}

/*
 * Start_session will return the video persistence handle, if grab is 1.
 * set persist to the value, when you want to re-use.  Remember to delete
 */
static void *start_session (const char *name, int max_loop, int grab = 0, 
			    void *persist = NULL)
{
  char buffer[80];
  int loopcount = 0;
  CPlayerSession *psptr;

  CMsgQueue master_queue;
  SDL_sem *master_sem;

  master_sem = SDL_CreateSemaphore(0);
  snprintf(buffer, sizeof(buffer), "%s %s - %s", MPEG4IP_PACKAGE, MPEG4IP_VERSION, name);
  psptr = new CPlayerSession(&master_queue, master_sem,
			     buffer, 
			     persist);
  if (psptr == NULL) {
    return (NULL);
  }
  
  char errmsg[512];
  errmsg[0] = '\0';
  int ret = parse_name_for_session(psptr, name, errmsg, sizeof(errmsg), NULL);
  if (ret < 0) {
    player_debug_message("%s %s", errmsg, name);
    delete psptr;
    return (NULL);
  }

  if (ret > 0) {
    player_debug_message("%s", errmsg);
  }

  psptr->set_up_sync_thread();
  psptr->set_screen_location(100, 100);

  fullscreen = config.get_config_value(CONFIG_FULL_SCREEN);
  set_aspect_ratio(config.get_config_value(CONFIG_ASPECT_RATIO),psptr);
  psptr->set_audio_volume(config.get_config_value(CONFIG_VOLUME));
  while (loopcount < max_loop) {
    loopcount++;
    if (psptr->play_all_media(TRUE) != 0) {
      delete psptr;
      return (NULL);
    }
    session_paused = 0;
    int keep_going = 0;
#ifdef NEED_SDL_VIDEO_IN_MAIN_THREAD
    int state = 0;
#endif
    do {
      CMsg *msg;
#ifdef NEED_SDL_VIDEO_IN_MAIN_THREAD
      state = psptr->sync_thread(state);
#else
      SDL_SemWaitTimeout(master_sem, 10000);
#endif
      while ((msg = master_queue.get_message()) != NULL) {
	switch (msg->get_value()) {
	case MSG_SESSION_FINISHED:
	  keep_going = 1;
	  break;
	case MSG_RECEIVED_QUIT:
	  keep_going = 1;
	  break;
	case MSG_SDL_KEY_EVENT:
	  sdl_event_msg_t *smsg;
	  uint32_t len;
	  int ret;
	  smsg = (sdl_event_msg_t *)msg->get_message(len);
	  ret = process_sdl_key_events(psptr, smsg);
	  if (ret < 0) {
	    loopcount = max_loop;
	    keep_going = 1;
	  } else if (ret == 0) {
	    keep_going = 1;
	  }
	  break;
	}
	delete msg;
      }
    } while (keep_going == 0);
    if (loopcount != max_loop) {
      psptr->pause_all_media();
    }
  }
  if (grab == 1) {
    // grab before we delete
    persist = psptr->grab_video_persistence();
  }
    
  delete psptr;
  SDL_DestroySemaphore(master_sem);
  return (persist);
}

int main (int argc, char **argv)
{

  int max_loop = 1;
  char *name;
  char buffer[FILENAME_MAX];
  char *home = getenv("HOME");
  if (home == NULL) {
#ifdef _WIN32
	strcpy(buffer, "gmp4player_rc");
#else
    strcpy(buffer, ".gmp4player_rc");
#endif
  } else {
    strcpy(buffer, home);
    strcat(buffer, "/.gmp4player_rc");
  }
  
  initialize_plugins();
  config.SetDefaultFileName(buffer);
  config.InitializeIndexes();
  config.ReadDefaultFile();
  rtsp_set_error_func(library_message);
  rtsp_set_loglevel(config.get_config_value(CONFIG_RTSP_DEBUG));
  rtp_set_error_msg_func(library_message);
  rtp_set_loglevel(config.get_config_value(CONFIG_RTP_DEBUG));
  sdp_set_error_func(library_message);
  sdp_set_loglevel(config.get_config_value(CONFIG_SDP_DEBUG));
  http_set_error_func(library_message);
  http_set_loglevel(config.get_config_value(CONFIG_HTTP_DEBUG));

  argv++;
  argc--;
  if (argc && strcmp(*argv, "-l") == 0) {
    argv++;
    argc--;
    max_loop = atoi(*argv);
    argc--;
    argv++;
  }
  if (*argv == NULL) {
    player_error_message("usage - mp4player [-l] <content>");
    exit(-1);
  } else {
    name = *argv;
  }

  const char *suffix = strrchr(name, '.');

    void *persist = NULL;
#ifdef _darwin
 CSDLVideo *sdl_video = new CSDLVideo();
  sdl_video->set_image_size(720, 480, 1.0);
  sdl_video->set_screen_size(0, 2);
  persist = sdl_video;
#endif
   if ((suffix != NULL) && 
	  ((strcasecmp(suffix, ".mp4plist") == 0) ||
	   (strcasecmp(suffix, ".mxu") == 0) ||
       (strcasecmp(suffix, ".gmp4_playlist") == 0))) {
    const char *errmsg = NULL;
    CPlaylist *list = new CPlaylist(name, &errmsg);
    if (errmsg != NULL) {
      player_error_message("%s", errmsg);
      return (-1);
    }
    for (int loopcount = 0; loopcount < max_loop; loopcount++) {
      const char *start = list->get_first();
      do {
	if (start != NULL) {
	  if (persist == NULL) {
	    persist = start_session(start, 1, 1);
	  } else {
	    persist = start_session(start, 1, 0, persist);
	  }
	}
	start = list->get_next();
      } while (start != NULL);
    }
  } else {
    start_session(name, max_loop, 0, persist);
  }
  // remove invalid global ports
  if (persist != NULL) {
    DestroyVideoPersistence(persist);
  }
  close_plugins();

  return(0); 
}  
  

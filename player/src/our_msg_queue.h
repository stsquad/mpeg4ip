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
 * msg_queue.h - provides a basic, SDL based message passing routine
 */
#ifndef __OUR_MSG_QUEUE_H__
#define __OUR_MSG_QUEUE_H__ 1

#include "msg_queue.h"

#define MSG_SESSION_FINISHED 1
#define MSG_PAUSE_SESSION 2
#define MSG_START_SESSION 3
#define MSG_STOP_THREAD 4
#define MSG_START_DECODING 5
#define MSG_SYNC_RESIZE_SCREEN 6
#define MSG_RECEIVED_QUIT 7
#define MSG_SDL_KEY_EVENT 8
#define MSG_SESSION_ERROR 9
#define MSG_SESSION_WARNING 10
#define MSG_SESSION_STARTED 11
#define MSG_AUDIO_CONFIGURED 12
#define MSG_SDL_MOUSE_EVENT 13
#define MSG_OUR_LAST_MESSAGE 14

typedef struct sdl_event_msg_t {
  SDLKey sym;
  SDLMod mod;
} sdl_event_msg_t;

typedef struct sdl_mouse_msg_t {
  uint8_t button;
  uint16_t x, y;
} sdl_mouse_msg_t;
#endif

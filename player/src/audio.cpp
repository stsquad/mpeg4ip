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
 * audio.cpp provides an interface (CAudioSync) between the codec and
 * the SDL audio APIs.
 */
#include <stdlib.h>
#include <string.h>
#include "player_session.h"
#include "audio.h"
#include "player_util.h"
//#define DEBUG_SYNC 1
//#define DEBUG_AUDIO_FILL 1
//#define DEBUG_DELAY 1
#ifdef _WIN32
DEFINE_MESSAGE_MACRO(audio_message, "audiosync")
#else
#define audio_message(loglevel, fmt...) message(loglevel, "audiosync", fmt)
#endif


void CAudioSync::set_eof(void) 
{ 
  audio_message(LOG_DEBUG, "Setting audio EOF");
  m_eof = 1;
}

void CAudioSync::clear_eof (void)
{
  m_eof = 0;
}

// Sync task api - initialize the sucker.
// May need to check out non-standard frequencies, see about conversion.
// returns 0 for not yet, 1 for initialized, -1 for error
int CAudioSync::initialize_audio (int have_video) 
{
  return (0);
}

// This is used by the sync thread to determine if a valid amount of
// buffers are ready, and what time they start.  It is used to determine
// when we should start.
int CAudioSync::is_audio_ready (uint64_t &disptime)
{
  return (0);
}

// Used by the sync thread to see if resync is needed.
// 0 - in sync.  > 0 - sync time we need. -1 - need to do sync 
uint64_t CAudioSync::check_audio_sync (uint64_t current_time, int &have_eof)
{
  return (0);
}

void CAudioSync::play_audio (void)
{
}

// Called from the sync thread when we want to stop.  Pause the audio,
// and indicate that we're not to fill any more buffers - this should let
// the decode thread get back to receive the pause message.  Only called
// when pausing - could cause m_dont_fill race conditions if called on play
void CAudioSync::flush_sync_buffers (void)
{
}

// this is called from the decode thread.  It gets called on entry into pause,
// and entry into play.  This way, m_dont_fill race conditions are resolved.
void CAudioSync::flush_decode_buffers (void)
{
}

void CAudioSync::set_volume (int volume)
{
}

/* end audio.cpp */


#ifndef _player_session_wrap_h
#define _player_session_wrap_h

#include <SDL.h>

/*
 * C interface for player_session wrapped around the C++ interface
 * so that we can call it from Objective-C since we don't have
 * Objective-C++.
 */

#ifdef __cplusplus
extern "C" {
#endif

/* Message queue */

typedef struct _CMQ CMQ;

CMQ*	new_CMQ(void);
void	CMQ_destroy(CMQ*);

int		CMQ_send_message(CMQ*, size_t msgval, unsigned char *msg,
		   size_t msg_len, SDL_sem *sem);
void player_initialize_plugins(void);

/* Player session */
#ifndef __PLAYER_SESSION_H__ /* HACK */
typedef enum {
  SESSION_PAUSED,
  SESSION_BUFFERING,
  SESSION_PLAYING,
  SESSION_DONE
} session_state_t;
#endif

typedef struct _CPS CPS;

CPS* 	new_CPS(CMQ* master_queue, SDL_sem* master_sem, const char* name);
	
void 	CPS_destroy(CPS* s);

int 	CPS_play_all_media(CPS* s, int start_from_begin, double start_time);

int 	CPS_pause_all_media(CPS* s);

void 	CPS_set_up_sync_thread(CPS* s);

double 	CPS_get_playing_time(CPS* s);

double 	CPS_get_max_time(CPS* s);

void 	CPS_set_audio_volume(CPS* s, int volume);

void 	CPS_set_screen_location(CPS* s, int x, int y);

void 	CPS_set_screen_size(CPS* s, int scaletimes2, int fullscreen);

int 	CPS_session_is_seekable(CPS* s);

session_state_t
        CPS_get_session_state(CPS* s);
		

/* Media utilities */

int 	parse_name_for_c_session(CPS* s, const char* name, char* errmsg, unsigned int len);

#ifdef __cplusplus
};
#endif

#endif

#include <rtp_bytestream.h>
#include "player_session.h"
#include "media_utils.h"
#include "our_msg_queue.h"
#include "player_session_wrap.h"

struct _CMQ {
	CMsgQueue* p;
};

extern "C" CMQ*	new_CMQ(void)
{
	CMQ* s = new CMQ;
	s->p = new CMsgQueue;
	return s;
}

void	CMQ_destroy(CMQ* s)
{
	delete s->p;
	delete s;
}

int		CMQ_send_message(CMQ* s, size_t msgval, unsigned char *msg,
		   size_t msg_len, SDL_sem *sem)
{
	return s->p->send_message(msgval, msg, msg_len, sem);
}


struct _CPS {
	CPlayerSession* p;
};

CPS* 	new_CPS(CMQ* master_queue, SDL_sem* master_sem, const char* name)
{
	CPS* s = new CPS;
	s->p = new CPlayerSession(master_queue->p, master_sem, name);
	return s;
}
	
extern "C" void 	CPS_destroy(CPS* s)
{
	delete s->p;
	delete s;
}

int 	CPS_play_all_media(CPS* s, int start_from_begin, double start_time)
{
	return s->p->play_all_media(start_from_begin, start_time);
}

int 	CPS_pause_all_media(CPS* s)
{
	return s->p->pause_all_media();
}

void 	CPS_set_up_sync_thread(CPS* s)
{
	s->p->set_up_sync_thread();
}

double 	CPS_get_playing_time(CPS* s)
{
  double pt = (double)s->p->get_playing_time();
  return pt / 1000.0;
}

double 	CPS_get_max_time(CPS* s)
{
	return s->p->get_max_time();
}

void 	CPS_set_audio_volume(CPS* s, int volume)
{
	s->p->set_audio_volume(volume);
}

void 	CPS_set_screen_location(CPS* s, int x, int y)
{
	s->p->set_screen_location(x, y);
}

void 	CPS_set_screen_size(CPS* s, int scaletimes2, int fullscreen)
{
	s->p->set_screen_size(scaletimes2, fullscreen);
}

int 	CPS_session_is_seekable(CPS* s)
{
	return s->p->session_is_seekable();
}

session_state_t
		CPS_get_session_state(CPS* s)
{
	return s->p->get_session_state();
}

int 	parse_name_for_c_session(CPS* s, const char* name, char* errmsg, unsigned int len)
{
	return parse_name_for_session(s->p, name, errmsg, len, NULL);
}

extern "C" void initialize_plugins(void);
void player_initialize_plugins(void)
{
  initialize_plugins();
}


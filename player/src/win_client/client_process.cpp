#include "client_process.h"
#include <stdio.h>
#include <SDL_thread.h>
#include "player_session.h"
#include "player_util.h"
#include "wmp4client.h"

static int c_msg_send (void *ud)
{
	CClientProcess *cp = (CClientProcess *)ud;

	cp->msg_send_thread();
	return 0;
}

CClientProcess::CClientProcess()
{
	m_map_file = NULL;
	m_map_file_read = NULL;
	m_from_gui_event = NULL;
	m_from_gui_resp_event = NULL;
	m_to_gui_event = NULL;
	m_to_gui_resp_event = NULL;
	m_thread_created = FALSE;
	m_msg_queue_sem = NULL;
	m_send_thread = NULL;
	m_stop_send_thread = 0;
	m_debugging = 0;
}

#define CLOSE_AND_CLEAR(a) if ((a) != NULL) { CloseHandle(a); (a) = NULL; }

CClientProcess::~CClientProcess(void)
{
	if (m_send_thread != NULL) {
		m_stop_send_thread = 1;
		SDL_SemPost(m_msg_queue_sem);
		SDL_WaitThread(m_send_thread, NULL);
		m_send_thread = NULL;
	}
	SDL_DestroySemaphore(m_msg_queue_sem);
	if (m_map_file_read != NULL) {
		UnmapViewOfFile(m_map_file_read);
		m_map_file_read = NULL;
	}
	CLOSE_AND_CLEAR(m_map_file);
	CLOSE_AND_CLEAR(m_from_gui_event);
	CLOSE_AND_CLEAR(m_from_gui_resp_event);
	CLOSE_AND_CLEAR(m_to_gui_event);
	CLOSE_AND_CLEAR(m_to_gui_resp_event);
}
int CClientProcess::enable_communication(int debugging)
{
    m_debugging = debugging;
	if (m_debugging) return 0;
	m_map_file = OpenFileMapping(FILE_MAP_ALL_ACCESS, 
		FALSE,                    
		"MP4ThreadToEventFileMap");
	
	if (m_map_file == NULL) 
	{ 
		printf("Could not open file-mapping object.\n"); 
		return -1;
	} 
 
    m_map_file_read = MapViewOfFile(m_map_file,
									FILE_MAP_ALL_ACCESS, 
									0,
									0,
									0);
	if (m_map_file_read == NULL) 
	{ 
		printf("Could not map view of file.\n");
		return -1;
			
	}	 
 
	m_from_gui_event = OpenEvent(EVENT_ALL_ACCESS, FALSE, "MP4GuiToClient");
	if (m_from_gui_event == NULL) {
		printf("Can't get event\n");
		return -1;
	}

	m_from_gui_resp_event = OpenEvent(EVENT_ALL_ACCESS, FALSE, "MP4GuiToClientResp");
	if (m_from_gui_resp_event == NULL) {
		printf("Can't get event (from resp)\n");
		return -1;
	}

	m_to_gui_event = OpenEvent(EVENT_ALL_ACCESS, FALSE, "MP4ClientToGui");
	if (m_to_gui_event == NULL) {
		printf("Can't get event (to gui)\n");
		return -1;
	}

	m_to_gui_resp_event = OpenEvent(EVENT_ALL_ACCESS, FALSE, "MP4ClientToGuiResp");
	if (m_to_gui_resp_event == NULL) {
		printf("Can't get event (to gui resp)\n");
		return -1;
	}

	m_msg_queue_sem = SDL_CreateSemaphore(0);
	m_send_thread = SDL_CreateThread(c_msg_send, this);

	return 0;
}

void CClientProcess::initial_response (int code, 
									   CPlayerSession *psptr,
									   const char *errmsg)
{
	char temp;
	int len;
	if (m_debugging) return;
	if (errmsg == NULL) {
		temp = '\0';
		errmsg = &temp;
	}
    msg_mbox_t *msg_mbox = (msg_mbox_t *)m_map_file_read;
	msg_mbox->from_client_response = code;
	len = strlen(errmsg);

	msg_initial_resp_t *msg_resp;
	msg_resp = (msg_initial_resp_t *)msg_mbox->to_client_mbox;
	if (psptr == NULL) {
		memset(msg_resp, 0, sizeof(msg_initial_resp_t));
	} else {
		msg_resp->has_audio = psptr->session_has_audio();
		msg_resp->session_is_seekable = psptr->session_is_seekable();
		msg_resp->max_time = psptr->get_max_time();
	}
	char *next = msg_mbox->to_client_mbox + sizeof(msg_initial_resp_t);
	memcpy(next,
			errmsg,
			MIN(len, (MBOX_SIZE - sizeof(msg_initial_resp_t))));
	SetEvent(m_from_gui_resp_event);
}

int CClientProcess::receive_messages (CPlayerSession *psptr)
{
	int retval;
	int our_ret = 0;

	if (m_debugging) return our_ret;
	retval = WaitForSingleObject(m_from_gui_event, 0);
	if (retval == WAIT_OBJECT_0) {
		msg_mbox_t *msg_mbox = (msg_mbox_t *)m_map_file_read;
		switch (msg_mbox->to_client_command) {
		case CLIENT_MSG_TERMINATE:
			our_ret = 1;
			msg_mbox->from_client_response = 0;
			break;
		case CLIENT_MSG_SET_SCREEN_SIZE: 
			{
			int size = *(int *)msg_mbox->to_client_mbox;
			if (size == 0) size = 1;
			else size *= 2;
			psptr->set_screen_size(size);
			break;
			}
		case CLIENT_READ_CONFIG_FILE:
			set_configs();
			break;
		case CLIENT_MSG_PLAY: 
			{
			client_msg_play_t *cmp = 
				(client_msg_play_t *)msg_mbox->to_client_mbox;

			player_debug_message("play command received %d %g", 
				cmp->start_from_begin, cmp->start_time);
			psptr->play_all_media(cmp->start_from_begin, 
								  cmp->start_time);
			}
			break;
		case CLIENT_MSG_PAUSE:
			psptr->pause_all_media();
			player_debug_message("Pause message received");
			break;
		case CLIENT_MSG_GET_CURRENT_TIME:
			msg_mbox->from_client_response = 
				psptr->get_playing_time();
			break;
		}
		msg_mbox->to_client_command = BLANK_MSG;
		SetEvent(m_from_gui_resp_event);
	}
	return (our_ret);
}

int CClientProcess::send_message (uint32_t msg, uint32_t param)
{
	return m_msg_queue.send_message(msg, param, m_msg_queue_sem);
}

int CClientProcess::send_message (uint32_t message,
								   void *msg_body,
								   int msg_len)
{
	return m_msg_queue.send_message(message, 
									(unsigned char *)msg_body, 
									msg_len, 
									m_msg_queue_sem);
}

int CClientProcess::send_string (uint32_t message,
								 const char *string)
{
	char value = '\0';

	if (string == NULL) { 
		string = &value;
	}
	return (m_msg_queue.send_message(message, 
									 (unsigned char *)string,
									 strlen(string) + 1,
									 m_msg_queue_sem));
}

void CClientProcess::msg_send_thread (void)
{
	msg_mbox_t *pmbox = (msg_mbox_t *)m_map_file_read;
	CMsg *msg;
	while (1) {
		SDL_SemWait(m_msg_queue_sem);
		if (m_stop_send_thread != 0) return;
		while ((msg = m_msg_queue.get_message()) != NULL) {
			if (m_debugging == 0) {
			pmbox->to_gui_command = msg->get_value();
			if (msg->has_param()) {
				uint32_t param = msg->get_param();
				memcpy(pmbox->to_gui_mbox,
					   &param,
					   sizeof(param));
			} else {
				uint32_t msg_len;
				const void *foo;
				foo = msg->get_message(msg_len);
				player_debug_message("message %x size %d", msg->get_value(), msg_len);
				if (msg_len > 0) {
					memcpy(pmbox->to_gui_mbox,
						   foo,
						   MIN(msg_len, MBOX_SIZE));
				}
			}
			pmbox->from_gui_response = MBOX_RESP_EMPTY;
			ResetEvent(m_to_gui_resp_event);
			SetEvent(m_to_gui_event);
			
			// Now wait for response.
			WaitForSingleObject(m_to_gui_resp_event, 2 * 1000);
			if (pmbox->from_gui_response != MBOX_RESP_EMPTY) {
				
			}
			}
			delete msg;
		}
	}
}


#ifndef __MSG_MBOX_H__
#define __MSG_MBOX_H__ 1
#ifndef MIN
#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#endif

#define MBOX_SIZE 2048
#define MBOX_RESP_EMPTY 0xfeedbeef
typedef struct msg_mbox_t {
	unsigned __int32 to_client_command;
	unsigned __int64 from_client_response;
	unsigned __int32 to_gui_command;
	unsigned __int64 from_gui_response;
	char to_client_mbox[MBOX_SIZE];
	char to_gui_mbox[MBOX_SIZE];
} msg_mbox_t;

typedef struct msg_initial_resp_t {
	int has_audio;
	int session_is_seekable;
	double max_time;
	// Then the error message
} msg_initial_resp_t;

#define BLANK_MSG 0xffffffff
// To client messages
#define CLIENT_MSG_TERMINATE 0
#define CLIENT_READ_CONFIG_FILE 1 // no parameter
#define CLIENT_MSG_SET_SCREEN_SIZE 2 // int parameter
#define CLIENT_MSG_PLAY 3    // no parameter
typedef struct client_msg_play_t {
	int start_from_begin;
	double start_time;
} client_msg_play_t;

#define CLIENT_MSG_PAUSE 4   // no parameter
#define CLIENT_MSG_GET_CURRENT_TIME 5

// To gui (from client) messages

#define GUI_MSG_SESSION_FINISHED 0x80
#define GUI_MSG_RECEIVED_QUIT    0x81
#define GUI_MSG_SDL_KEY          0x82
#define GUI_MSG_SESSION_ERROR        0x83
#define GUI_MSG_SESSION_WARNING      0x84

#endif
#include "mpeg4ip.h"
 #include "msg_mbox.h"
#include <msg_queue/msg_queue.h>
#include <SDL.h>
#include <SDL_thread.h>

class CPlayerSession;

class CClientProcess {
public:
	CClientProcess(void);
	~CClientProcess(void);
	int enable_communication(int debugging = 0);
	void initial_response(int code, CPlayerSession *psptr, const char *errmsg);
	int receive_messages(CPlayerSession *psptr);
	int send_message(uint32_t to_command,
					 uint32_t param);

	int send_message(uint32_t to_command, 
					 void *msg_body = NULL, 
					 int len = 0);
	int send_string(uint32_t to_command, 
				    const char *string);
	void msg_send_thread(void);
private:
	LPVOID m_map_file_read;
	HANDLE m_from_gui_event;
	HANDLE m_from_gui_resp_event;
	HANDLE m_to_gui_event;
	HANDLE m_to_gui_resp_event;
	HANDLE m_map_file;
	BOOL m_thread_created;
	SECURITY_ATTRIBUTES m_saAttr;
	PROCESS_INFORMATION m_piProcInfo; 
	STARTUPINFO m_siStartInfo; 
	CMsgQueue m_msg_queue;
	SDL_sem *m_msg_queue_sem;
	SDL_Thread *m_send_thread;
	volatile int m_stop_send_thread;
	int m_debugging;
};
#ifndef __MP4PROCESS_H__
#define __MP4PROCESS_H__ 1

#include "msg_mbox.h"


class CMP4Process {
public:
	CMP4Process(void)  { init(); };
	CMP4Process(CString &name);
	~CMP4Process(void);
	BOOL is_thread_created(void) { return m_thread_created;};
	int get_initial_response(msg_initial_resp_t *msg, CString &errmsg);
	BOOL start_process(CString &name);
	bool send_message(unsigned __int32 message,
					  unsigned __int64 *retval,
					  const char *msg_body = NULL,
					  int msg_len = 0);
	bool kill_process(void);
	void receive_thread(void);

private:
	LPVOID m_map_file_write;
	HANDLE m_to_client_event;
	HANDLE m_to_client_resp_event;
	HANDLE m_from_client_event;
	HANDLE m_from_client_resp_event;
	HANDLE m_map_file;
	HANDLE m_receive_thread;
	DWORD m_receive_thread_id;
	BOOL m_thread_created;
	volatile int m_stop_receive_thread;
	SECURITY_ATTRIBUTES m_saAttr;
	PROCESS_INFORMATION m_piProcInfo; 
	STARTUPINFO m_siStartInfo; 
	void clean_up_process(void);
	void init(void);
	int m_terminating;
};

#endif
#include "msg_queue/msg_queue.h"

#define to_thread_event handles[0]
#define socket_event handles[1]

struct rtsp_thread_info_ {
	HANDLE handles[2];
	HANDLE from_thread_resp_event;
	int from_thread_resp;
	SECURITY_ATTRIBUTES m_saAttr;
	CMsgQueue *m_msg_queue;
	CMsg *msg;
};

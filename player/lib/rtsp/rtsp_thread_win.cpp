#include "rtsp_private.h"
#include "rtsp_thread_win.h"

/*
 * rtsp_thread_ipc_respond
 * respond with the message given
 */
int rtsp_thread_ipc_respond (rtsp_client_t *info, int msg)
{
	info->m_thread_info->from_thread_resp = msg;
	if (SetEvent(info->m_thread_info->from_thread_resp_event)) {
		return 0;
	}
	return -1;
}


int rtsp_thread_ipc_receive (rtsp_client_t *info, char *msg, size_t msg_len)
{
	const void *ptr;
	uint32_t len;
	if (info->m_thread_info->msg == NULL) 
		return -1;

	ptr = info->m_thread_info->msg->get_message(len);

	if (len != msg_len) return -1;

	memcpy(msg, ptr, msg_len);
	return msg_len;
}

void rtsp_thread_init_thread_info (rtsp_client_t *info)
{
	
  //COMM_SOCKET_THREAD = info->m_thread_info->comm_socket[0];

}

int rtsp_thread_wait_for_event (rtsp_client_t *info)
{
  rtsp_thread_info_t *tinfo = info->m_thread_info;
  DWORD ret;

  ret = 
	  WaitForMultipleObjects(info->server_socket == -1 ? 1 : 2,
							 tinfo->handles,
							 FALSE,
							 info->recv_timeout);

  //rtsp_debug(LOG_DEBUG, "wait for event return %ld", ret);

  if (ret == WAIT_TIMEOUT) {
	  //rtsp_debug(LOG_DEBUG, "timeout");
	  return 0;
  }
  return 1;
}

int rtsp_thread_has_control_message (rtsp_client_t *info)
{
  rtsp_thread_info_t *tinfo = info->m_thread_info;
  DWORD ret;

  ret = WaitForSingleObject(tinfo->to_thread_event, 0);
  if (ret == WAIT_TIMEOUT) {
	  return FALSE;
  }
  ResetEvent(tinfo->to_thread_event);
  tinfo->msg = tinfo->m_msg_queue->get_message();
  if (tinfo->msg == NULL) {
	  rtsp_debug(LOG_ERR, "has control message - event, but no msg");
	  return FALSE;
  }
  return TRUE;
}

int rtsp_thread_has_receive_data (rtsp_client_t *info)
{
  rtsp_thread_info_t *tinfo = info->m_thread_info;
#if 0
  DWORD ret;

  if (tinfo->socket_event != NULL)
	  return FALSE;

  ret = WaitForSingleObject(tinfo->socket_event, 0);
  if (ret == WAIT_TIMEOUT) {
	  return FALSE;
  }
  rtsp_debug(LOG_DEBUG, "has receive data");
#endif
  ResetEvent(tinfo->socket_event);
  return TRUE;
}

int rtsp_thread_get_control_message (rtsp_client_t *info,
				     rtsp_msg_type_t *msg)
{
	if (info->m_thread_info->msg == NULL) return -1;

	*msg = info->m_thread_info->msg->get_value();
	return sizeof(rtsp_msg_type_t);
}

void rtsp_close_thread (rtsp_client_t *rptr)
{
    uint32_t msg = RTSP_MSG_QUIT;
    rtsp_thread_info_t *info;
    
    rtsp_thread_ipc_send(rptr, (unsigned char *)&msg, sizeof(msg));
    SDL_WaitThread(rptr->thread, NULL);
    info = rptr->m_thread_info;

	CloseHandle(info->handles[0]);
	CloseHandle(info->from_thread_resp_event);
    
    free(info);
    rptr->m_thread_info = NULL;
    rptr->thread = NULL;
}

/*
 * rtsp_create_thread - create the thread we need, along with the
 * communications socket.
 */
int rtsp_create_thread (rtsp_client_t *cptr)
{
	rtsp_thread_info_t *info;
	info = cptr->m_thread_info =
		(rtsp_thread_info_t *)malloc(sizeof(rtsp_thread_info_t));
	
	if (info == NULL) return -1;
	info->m_msg_queue = new CMsgQueue();

	info->m_saAttr.nLength = sizeof(info->m_saAttr);
    info->m_saAttr.bInheritHandle = TRUE;
    info->m_saAttr.lpSecurityDescriptor = NULL;

	info->from_thread_resp_event = 
		CreateEvent(NULL, TRUE, FALSE, NULL);
	info->to_thread_event = 
		CreateEvent(NULL, TRUE, FALSE, NULL);
	info->socket_event = NULL;

	cptr->thread = SDL_CreateThread(rtsp_thread, cptr);
	if (cptr->thread == NULL) {
		rtsp_debug(LOG_CRIT, "Couldn't create comm thread");
		return -1;
	}
	
  return 0;
}

/*
 * rtsp_thread_ipc_send - send message to rtsp thread
 */
int rtsp_thread_ipc_send (rtsp_client_t *info,
			  unsigned char *msg,
			  int len)
{
	rtsp_msg_type_t this_msg;
	rtsp_thread_info_t *tinfo = info->m_thread_info;

	this_msg = *(rtsp_msg_type_t *)msg;
	msg += sizeof(rtsp_msg_type_t);
	len -= sizeof(rtsp_msg_type_t);
	if (len == 0) msg = NULL;

	tinfo->m_msg_queue->send_message(this_msg, msg, len);
	return SetEvent(tinfo->to_thread_event);
}

/*
 * rtsp_thread_ipc_send_wait
 * send a message, and wait for response
 * returns number of bytes we've received.
 */
int rtsp_thread_ipc_send_wait (rtsp_client_t *info,
			       unsigned char *msg,
			       int msg_len,
			       int *return_msg)
{
	DWORD ret;
	rtsp_debug(LOG_DEBUG, "Send-wait msg to thread %u - len %d",
		*(rtsp_msg_type_t *)msg, msg_len);
	SDL_LockMutex(info->msg_mutex);
	ResetEvent(info->m_thread_info->from_thread_resp_event);
	
	if (!rtsp_thread_ipc_send(info, msg, msg_len)) {
		SDL_UnlockMutex(info->msg_mutex);
		return -1;
	}
	
	
	ret = WaitForSingleObject(info->m_thread_info->from_thread_resp_event, 
		30 * 1000);
	if (ret == WAIT_TIMEOUT) {
		SDL_UnlockMutex(info->msg_mutex);
		rtsp_debug(LOG_ERR, "Timedout from ipc_send_wait");
		return -1;
	}
	rtsp_debug(LOG_DEBUG, "Response received - %d", 
		info->m_thread_info->from_thread_resp);
	*return_msg = info->m_thread_info->from_thread_resp;
	return (sizeof(int));
}

void rtsp_thread_close (rtsp_client_t *info)
{
  rtsp_close_socket(info);
  if (info->m_thread_info->socket_event != NULL) {
	  CloseHandle(info->m_thread_info->socket_event);
	  info->m_thread_info->socket_event = NULL;
  }
}

void rtsp_thread_set_nonblocking (rtsp_client_t *info)
{
    rtsp_thread_info_t *tinfo = info->m_thread_info;
	
	tinfo->socket_event = CreateEvent(NULL,
		TRUE, 
		FALSE,
		NULL);
	if (WSAEventSelect(info->server_socket,
		tinfo->socket_event,
		FD_READ) != 0) {
		rtsp_debug(LOG_CRIT, "Couldn't associate event with socket - err %d", 
			WSAGetLastError());
	}
}

#include "rtsp_private.h"
#include "rtsp_thread_nx.h"

#define COMM_SOCKET_THREAD info->m_thread_info->comm_socket_write_to
#define COMM_SOCKET_CALLER info->m_thread_info->comm_socket[1]

/*
 * rtsp_thread_ipc_respond
 * respond with the message given
 */
int rtsp_thread_ipc_respond (rtsp_client_t *info, int msg)
{
  size_t ret;
  rtsp_debug(LOG_DEBUG, "Sending resp to thread %d", msg);
  ret = send(COMM_SOCKET_THREAD, (char *)&msg, sizeof(int), 0);
  return ret == sizeof(int);
}

int rtsp_thread_ipc_receive (rtsp_client_t *info, char *msg, size_t msg_len)
{
  int ret;
  
  ret = recv(COMM_SOCKET_THREAD, msg, msg_len, 0);
  return (ret);
}

void rtsp_thread_init_thread_info (rtsp_client_t *info)
{
#ifndef HAVE_SOCKETPAIR
  struct sockaddr_un our_name, his_name;
  int len;
  int ret;
  COMM_SOCKET_THREAD = socket(AF_UNIX, SOCK_STREAM, 0);

  memset(&our_name, 0, sizeof(our_name));
  our_name.sun_family = AF_UNIX;
  strcpy(our_name.sun_path, info->m_thread_info->socket_name);
  ret = bind(COMM_SOCKET_THREAD, (struct sockaddr *)&our_name, sizeof(our_name));
  listen(COMM_SOCKET_THREAD, 1);
  len = sizeof(his_name);
  COMM_SOCKET_THREAD = accept(COMM_SOCKET_THREAD,
				      (struct sockaddr *)&his_name,
				      &len);
#else
  COMM_SOCKET_THREAD = info->m_thread_info->comm_socket[0];
#endif
  rtsp_debug(LOG_DEBUG, "rtsp_thread running");

}
int rtsp_thread_wait_for_event (rtsp_client_t *info)
{
  rtsp_thread_info_t *tinfo = info->m_thread_info;
  int ret;

  #ifdef HAVE_POLL
#define SERVER_SOCKET_HAS_DATA ((tinfo->pollit[1].revents & (POLLIN | POLLPRI)) != 0)
#define COMM_SOCKET_HAS_DATA   ((tinfo->pollit[0].revents & (POLLIN | POLLPRI)) != 0)
    tinfo->pollit[0].fd = COMM_SOCKET_THREAD;
    tinfo->pollit[0].events = POLLIN | POLLPRI;
    tinfo->pollit[0].revents = 0;
    tinfo->pollit[1].fd = info->server_socket;
    tinfo->pollit[1].events = POLLIN | POLLPRI;
    tinfo->pollit[1].revents = 0;

    ret = poll(tinfo->pollit,
	       info->server_socket == -1 ? 1 : 2,
	       info->recv_timeout);
    //rtsp_debug(LOG_DEBUG, "poll ret %d", ret);
#else
#define SERVER_SOCKET_HAS_DATA (FD_ISSET(info->server_socket, &tinfo->read_set))
#define COMM_SOCKET_HAS_DATA   (FD_ISSET(COMM_SOCKET_THREAD, &tinfo->read_set))
    int max_fd;
    struct timeval timeout;

    FD_ZERO(&tinfo->read_set);
    max_fd = COMM_SOCKET_THREAD;
    if (info->server_socket != -1) {
      FD_SET(info->server_socket, &tinfo->read_set);
      max_fd = MAX(info->server_socket, max_fd);
    }
    FD_SET(COMM_SOCKET_THREAD, &tinfo->read_set);
    timeout.tv_sec = info->recv_timeout / 1000;
    timeout.tv_usec = (info->recv_timeout % 1000) * 1000;
    ret = select(max_fd + 1, &tinfo->read_set, NULL, NULL, &timeout);
#endif
    return ret;
}

int rtsp_thread_has_control_message (rtsp_client_t *info)
{
  rtsp_thread_info_t *tinfo = info->m_thread_info;
  return COMM_SOCKET_HAS_DATA;
}

int rtsp_thread_has_receive_data (rtsp_client_t *info)
{
  rtsp_thread_info_t *tinfo = info->m_thread_info;
  return SERVER_SOCKET_HAS_DATA;
}

int rtsp_thread_get_control_message (rtsp_client_t *info,
				     rtsp_msg_type_t *msg)
{
  return recv(COMM_SOCKET_THREAD, (char *)msg, sizeof(rtsp_msg_type_t), 0);
}

void rtsp_close_thread (rtsp_client_t *rptr)
{
    uint32_t msg = RTSP_MSG_QUIT;
    rtsp_thread_info_t *info;
    
    rtsp_thread_ipc_send(rptr, (unsigned char *)&msg, sizeof(msg));
    SDL_WaitThread(rptr->thread, NULL);
    info = rptr->m_thread_info;
    
    closesocket(info->comm_socket[0]);
    closesocket(info->comm_socket[1]);
    free(info);
    rptr->m_thread_info = NULL;
    rptr->thread = NULL;
}


/*
 * rtsp_create_thread - create the thread we need, along with the
 * communications socket.
 */
int rtsp_create_thread (rtsp_client_t *info)
{
  rtsp_thread_info_t *tinfo;
#ifndef HAVE_SOCKETPAIR
  int ret;
  struct sockaddr_un addr;
#endif
  tinfo = info->m_thread_info =
    (rtsp_thread_info_t *)malloc(sizeof(rtsp_thread_info_t));

  if (tinfo == NULL) return -1;
  
  tinfo->comm_socket[0] = -1;
  tinfo->comm_socket[1] = -1;
  tinfo->comm_socket_write_to = -1;
#ifdef HAVE_SOCKETPAIR
  if (socketpair(PF_UNIX, SOCK_STREAM, 0, tinfo->comm_socket) < 0) {
    rtsp_debug(LOG_CRIT, "Couldn't create comm sockets - errno %d", errno);
    return -1;
  }
#else
  COMM_SOCKET_THREAD = -1;
  COMM_SOCKET_CALLER = socket(AF_UNIX, SOCK_STREAM, 0);
  snprintf(tinfo->socket_name, sizeof(tinfo->socket_name) - 1, "RTSPCLIENT%p", tinfo);
  unlink(tinfo->socket_name);
#endif
  info->thread = SDL_CreateThread(rtsp_thread, info);
  if (info->thread == NULL) {
    rtsp_debug(LOG_CRIT, "Couldn't create comm thread");
    return -1;
  }
#ifndef HAVE_SOCKETPAIR
  addr.sun_family = AF_UNIX;
  strcpy(addr.sun_path, tinfo->socket_name);
  ret = -1;
  do {
    ret = connect(COMM_SOCKET_CALLER, (struct sockaddr *)&addr, sizeof(addr));
    if (ret == -1)
      SDL_Delay(10);
  } while (ret < 0);
#endif
  return 0;
}

/*
 * rtsp_thread_ipc_send - send message to rtsp thread
 */
int rtsp_thread_ipc_send (rtsp_client_t *info,
			  unsigned char *msg,
			  int len)
{
  int ret;
  rtsp_debug(LOG_DEBUG, "Sending msg to thread %d - len %d", msg[0], len);
  ret = send(COMM_SOCKET_CALLER, msg, len, 0);
  return ret == len;
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
  int read, ret;
#ifdef HAVE_POLL
  struct pollfd pollit;
#else
  fd_set read_set;
  struct timeval timeout;
#endif
  rtsp_debug(LOG_DEBUG, "Send-wait msg to thread %d - len %d",
	     *(rtsp_msg_type_t *)msg, msg_len);
  SDL_LockMutex(info->msg_mutex);
  ret = send(COMM_SOCKET_CALLER, msg, msg_len, 0);
  if (ret != msg_len) {
    SDL_UnlockMutex(info->msg_mutex);
    return -1;
  }
#ifdef HAVE_POLL
  pollit.fd = COMM_SOCKET_CALLER;
  pollit.events = POLLIN | POLLPRI;
  pollit.revents = 0;

  ret = poll(&pollit, 1, 30 * 1000);
  rtsp_debug(LOG_DEBUG, "return comm socket value %x", pollit.revents);
#else
  FD_ZERO(&read_set);
  FD_SET(COMM_SOCKET_CALLER, &read_set);
  timeout.tv_sec = 30;
  timeout.tv_usec = 0;
  ret = select(COMM_SOCKET_CALLER + 1, &read_set, NULL, NULL, &timeout);
#endif

  if (ret <= 0) {
    if (ret < 0) {
      //rtsp_debug(LOG_ERR, "RTSP loop error %d errno %d", ret, errno);
    }
    SDL_UnlockMutex(info->msg_mutex);
    return -1;
  }

  read = recv(COMM_SOCKET_CALLER, return_msg, sizeof(int), 0);
  SDL_UnlockMutex(info->msg_mutex);
  rtsp_debug(LOG_DEBUG, "comm socket got return value of %d", read);
  return (read);
}

void rtsp_thread_close (rtsp_client_t *info)
{
  rtsp_close_socket(info);
#ifndef HAVE_SOCKETPAIR
  closesocket(info->m_thread_info->comm_socket_write_to);
  info->m_thread_info->comm_socket_write_to = -1;
  unlink(info->m_thread_info->socket_name);
#endif
}

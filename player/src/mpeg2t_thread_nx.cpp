#include "mpeg2t_private.h"
#include "mpeg2t_thread_nx.h"
#include "player_util.h"

#define COMM_SOCKET_THREAD info->m_thread_info->comm_socket_write_to
#define COMM_SOCKET_CALLER info->m_thread_info->comm_socket[1]

#ifdef _WIN32
DEFINE_MESSAGE_MACRO(mpeg2t_message, "mpeg2t")
#else
#define mpeg2t_message(loglevel, fmt...) message(loglevel, "mpeg2t", fmt)
#endif
/*
 * mpeg2t_thread_ipc_respond
 * respond with the message given
 */
int mpeg2t_thread_ipc_respond (mpeg2t_client_t *info, int msg)
{
  size_t ret;
  mpeg2t_message(LOG_DEBUG, "Sending resp to thread %d", msg);
  ret = send(COMM_SOCKET_THREAD, (char *)&msg, sizeof(int), 0);
  return ret == sizeof(int);
}

int mpeg2t_thread_ipc_receive (mpeg2t_client_t *info, char *msg, size_t msg_len)
{
  int ret;
  
  ret = recv(COMM_SOCKET_THREAD, msg, msg_len, 0);
  return (ret);
}

void mpeg2t_thread_init_thread_info (mpeg2t_client_t *info)
{
#ifndef HAVE_SOCKETPAIR
  struct sockaddr_un our_name, his_name;
  socklen_t len;
  int ret;
  info->m_thread_info->comm_socket[0] = socket(AF_UNIX, SOCK_STREAM, 0);

  memset(&our_name, 0, sizeof(our_name));
  our_name.sun_family = AF_UNIX;
  strcpy(our_name.sun_path, info->m_thread_info->socket_name);
  ret = bind(info->m_thread_info->comm_socket[0], 
	     (struct sockaddr *)&our_name, sizeof(our_name));
  if (listen(info->m_thread_info->comm_socket[0], 1) < 0) {
    mpeg2t_message(LOG_ERR, "Listen failure %d", errno);
  }

  len = sizeof(his_name);
  COMM_SOCKET_THREAD = accept(info->m_thread_info->comm_socket[0],
			      (struct sockaddr *)&his_name,
			      &len);
  mpeg2t_message(LOG_DEBUG, "return from accept %d", COMM_SOCKET_THREAD);
#else
  COMM_SOCKET_THREAD = info->m_thread_info->comm_socket[0];
#endif

}
int mpeg2t_thread_wait_for_event (mpeg2t_client_t *info)
{
  mpeg2t_thread_info_t *tinfo = info->m_thread_info;
  int ret;

  #ifdef HAVE_POLL
  int count;
#define DATA_SOCKET_HAS_DATA ((tinfo->pollit[1].revents & (POLLIN | POLLPRI)) != 0)
#define RTCP_SOCKET_HAS_DATA ((tinfo->pollit[2].revents & (POLLIN | POLLPRI)) != 0)
#define COMM_SOCKET_HAS_DATA   ((tinfo->pollit[0].revents & (POLLIN | POLLPRI)) != 0)
    tinfo->pollit[0].fd = COMM_SOCKET_THREAD;
    tinfo->pollit[0].events = POLLIN | POLLPRI;
    tinfo->pollit[0].revents = 0;
    tinfo->pollit[1].fd = info->data_socket;
    tinfo->pollit[1].events = POLLIN | POLLPRI;
    tinfo->pollit[1].revents = 0;
    tinfo->pollit[2].fd = info->rtcp_socket;
    tinfo->pollit[2].events = POLLIN | POLLPRI;
    tinfo->pollit[2].revents = 0;

    count = info->data_socket == 0 ? 1 : info->useRTP ? 3 : 2;

    //mpeg2t_message(LOG_DEBUG, "start poll");
    ret = poll(tinfo->pollit,count, info->recv_timeout);
    //mpeg2t_message(LOG_DEBUG, "poll ret %d - count %d", ret, count);
#else
#define DATA_SOCKET_HAS_DATA (FD_ISSET(info->data_socket, &tinfo->read_set))
#define RTCP_SOCKET_HAS_DATA (FD_ISSET(info->rtcp_socket, &tinfo->read_set))
#define COMM_SOCKET_HAS_DATA   (FD_ISSET(COMM_SOCKET_THREAD, &tinfo->read_set))
    int max_fd;
    struct timeval timeout;

    FD_ZERO(&tinfo->read_set);
    max_fd = COMM_SOCKET_THREAD;
    if (info->data_socket > 0) {
      FD_SET(info->data_socket, &tinfo->read_set);
      max_fd = MAX(info->data_socket, max_fd);
      if (info->useRTP && info->rtcp_socket > 0) {
	FD_SET(info->rtcp_socket, &tinfo->read_set);
	max_fd = MAX(info->rtcp_socket, max_fd);
      }
    }
    FD_SET(COMM_SOCKET_THREAD, &tinfo->read_set);
    timeout.tv_sec = info->recv_timeout / 1000;
    timeout.tv_usec = (info->recv_timeout % 1000) * 1000;
    ret = select(max_fd + 1, &tinfo->read_set, NULL, NULL, &timeout);
#endif
    return ret;
}

int mpeg2t_thread_has_control_message (mpeg2t_client_t *info)
{
  mpeg2t_thread_info_t *tinfo = info->m_thread_info;
  return COMM_SOCKET_HAS_DATA;
}

int mpeg2t_thread_has_receive_data (mpeg2t_client_t *info)
{
  mpeg2t_thread_info_t *tinfo = info->m_thread_info;
  return DATA_SOCKET_HAS_DATA;
}

int mpeg2t_thread_has_rtcp_data (mpeg2t_client_t *info)
{
  mpeg2t_thread_info_t *tinfo = info->m_thread_info;
  return RTCP_SOCKET_HAS_DATA;
}

int mpeg2t_thread_get_control_message (mpeg2t_client_t *info,
				     mpeg2t_msg_type_t *msg)
{
  return recv(COMM_SOCKET_THREAD, (char *)msg, sizeof(mpeg2t_msg_type_t), 0);
}

void mpeg2t_thread_close (mpeg2t_client_t *rptr)
{
    uint32_t msg = MPEG2T_MSG_QUIT;
    mpeg2t_thread_info_t *info;
    
    mpeg2t_thread_ipc_send(rptr, (unsigned char *)&msg, sizeof(msg));
    SDL_WaitThread(rptr->thread, NULL);
    info = rptr->m_thread_info;
    
    closesocket(info->comm_socket[0]);
    closesocket(info->comm_socket[1]);
    free(info);
    rptr->m_thread_info = NULL;
    rptr->thread = NULL;
}


/*
 * mpeg2t_create_thread - create the thread we need, along with the
 * communications socket.
 */
int mpeg2t_create_thread (mpeg2t_client_t *info)
{
  mpeg2t_thread_info_t *tinfo;
#ifndef HAVE_SOCKETPAIR
  int ret;
  struct sockaddr_un addr;
#endif
  tinfo = info->m_thread_info =
    (mpeg2t_thread_info_t *)malloc(sizeof(mpeg2t_thread_info_t));

  if (tinfo == NULL) return -1;
  
  tinfo->comm_socket[0] = -1;
  tinfo->comm_socket[1] = -1;
  tinfo->comm_socket_write_to = -1;
#ifdef HAVE_SOCKETPAIR
  if (socketpair(PF_UNIX, SOCK_STREAM, 0, tinfo->comm_socket) < 0) {
    mpeg2t_message(LOG_CRIT, "Couldn't create comm sockets - errno %d", errno);
    return -1;
  }
#else
  COMM_SOCKET_THREAD = -1;
  COMM_SOCKET_CALLER = socket(AF_UNIX, SOCK_STREAM, 0);
  snprintf(tinfo->socket_name, sizeof(tinfo->socket_name) - 1, "MPEG2TCLIENT%p", tinfo);
  unlink(tinfo->socket_name);
#endif


  info->thread = SDL_CreateThread(mpeg2t_thread, info);
  if (info->thread == NULL) {
    mpeg2t_message(LOG_CRIT, "Couldn't create comm thread");
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
 * mpeg2t_thread_ipc_send - send message to mpeg2t thread
 */
int mpeg2t_thread_ipc_send (mpeg2t_client_t *info,
			  unsigned char *msg,
			  int len)
{
  int ret;
  mpeg2t_message(LOG_DEBUG, "Sending msg to thread %d - len %d", msg[0], len);
  ret = send(COMM_SOCKET_CALLER, msg, len, 0);
  return ret == len;
}

/*
 * mpeg2t_thread_ipc_send_wait
 * send a message, and wait for response
 * returns number of bytes we've received.
 */
int mpeg2t_thread_ipc_send_wait (mpeg2t_client_t *info,
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
  mpeg2t_message(LOG_DEBUG, "Send-wait msg to thread %d - len %d",
	     *(mpeg2t_msg_type_t *)msg, msg_len);
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
  mpeg2t_message(LOG_DEBUG, "return comm socket value %x", pollit.revents);
#else
  FD_ZERO(&read_set);
  FD_SET(COMM_SOCKET_CALLER, &read_set);
  timeout.tv_sec = 30;
  timeout.tv_usec = 0;
  ret = select(COMM_SOCKET_CALLER + 1, &read_set, NULL, NULL, &timeout);
#endif

  if (ret <= 0) {
    if (ret < 0) {
      //mpeg2t_message(LOG_ERR, "MPEG2T loop error %d errno %d", ret, errno);
    }
    SDL_UnlockMutex(info->msg_mutex);
    return -1;
  }

  read = recv(COMM_SOCKET_CALLER, return_msg, sizeof(int), 0);
  SDL_UnlockMutex(info->msg_mutex);
  mpeg2t_message(LOG_DEBUG, "comm socket got return value of %d", read);
  return (read);
}

void mpeg2t_close_thread (mpeg2t_client_t *info)
{
#ifndef HAVE_SOCKETPAIR
  closesocket(info->m_thread_info->comm_socket_write_to);
  info->m_thread_info->comm_socket_write_to = -1;
  unlink(info->m_thread_info->socket_name);
#endif
}

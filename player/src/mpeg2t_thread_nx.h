#include <sys/un.h>
#ifdef HAVE_POLL
#include <sys/poll.h>
#endif


struct mpeg2t_thread_info_ {
  int comm_socket[2];
  int comm_socket_write_to;
  char socket_name[108];
#ifdef HAVE_POLL
  struct pollfd pollit[3];
#else
  fd_set read_set;
#endif
};

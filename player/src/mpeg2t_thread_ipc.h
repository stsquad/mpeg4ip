
#ifndef __MPEG2T_THREAD_IPC_H__
#define __MPEG2T_THREAD_IPC_H__ 1


#define MPEG2T_MSG_QUIT 1
#define MPEG2T_MSG_START 2
#define MPEG2T_MSG_SEND_AND_GET 3
#define MPEG2T_MSG_PERFORM_CALLBACK 4
#define MPEG2T_MSG_SET_RTP_CALLBACK 5
#define MPEG2T_MSG_SEND_RTCP 6

typedef uint32_t mpeg2t_msg_type_t;
typedef int mpeg2t_msg_resp_t;

#if 0
typedef struct mpeg2t_msg_send_and_get_t {
  char *buffer;
  uint32_t buflen;
} mpeg2t_msg_send_and_get_t;

typedef struct mpeg2t_wrap_send_and_get_t {
  mpeg2t_msg_type_t msg;
  mpeg2t_msg_send_and_get_t body;
} mpeg2t_wrap_send_and_get_t;

typedef struct mpeg2t_msg_callback_t {
  mpeg2t_thread_callback_f func;
  void *ud;
} mpeg2t_msg_callback_t;

typedef struct mpeg2t_wrap_msg_callback_t {
  mpeg2t_msg_type_t msg;
  mpeg2t_msg_callback_t body;
} mpeg2t_wrap_msg_callback_t;

typedef struct mpeg2t_msg_rtp_callback_t {
  rtp_callback_f callback_func;
  mpeg2t_thread_callback_f periodic_func;
  void *ud;
  int interleave;
} mpeg2t_msg_rtp_callback_t;

typedef struct mpeg2t_wrap_msg_rtp_callback_t {
  mpeg2t_msg_type_t msg;
  mpeg2t_msg_rtp_callback_t body;
} mpeg2t_wrap_msg_rtp_callback_t;
#endif
#endif




#ifndef __RTSP_THREAD_IPC_H__
#define __RTSP_THREAD_IPC_H__ 1


#define RTSP_MSG_QUIT 1
#define RTSP_MSG_START 2
#define RTSP_MSG_SEND_AND_GET 3
#define RTSP_MSG_PERFORM_CALLBACK 4
#define RTSP_MSG_SET_RTP_CALLBACK 5
#define RTSP_MSG_SEND_RTCP 6

typedef uint32_t rtsp_msg_type_t;
typedef int rtsp_msg_resp_t;

typedef struct rtsp_msg_send_and_get_t {
  char *buffer;
  uint32_t buflen;
} rtsp_msg_send_and_get_t;

typedef struct rtsp_wrap_send_and_get_t {
  rtsp_msg_type_t msg;
  rtsp_msg_send_and_get_t body;
} rtsp_wrap_send_and_get_t;

typedef struct rtsp_msg_callback_t {
  rtsp_thread_callback_f func;
  void *ud;
} rtsp_msg_callback_t;

typedef struct rtsp_wrap_msg_callback_t {
  rtsp_msg_type_t msg;
  rtsp_msg_callback_t body;
} rtsp_wrap_msg_callback_t;

typedef struct rtsp_msg_rtp_callback_t {
  process_rtp_callback_f callback_func;
  rtsp_thread_callback_f periodic_func;
  void *ud;
  int interleave;
} rtsp_msg_rtp_callback_t;

typedef struct rtsp_wrap_msg_rtp_callback_t {
  rtsp_msg_type_t msg;
  rtsp_msg_rtp_callback_t body;
} rtsp_wrap_msg_rtp_callback_t;

#endif



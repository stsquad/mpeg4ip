
#ifndef __HAVE_PLAYER_RTSP__
#define __HAVE_PLAYER_RTSP__ 1

#ifdef __cplusplus
extern "C" {
#endif

typedef struct rtsp_transport_parse_t {
  int have_unicast;
  int have_multicast;
  in_port_t client_port;
  in_port_t server_port;
  char *source;
  int have_ssrc;
  uint32_t ssrc;
  unsigned int interleave_port;
  int use_interleaved;
} rtsp_transport_parse_t;

int process_rtsp_transport(rtsp_transport_parse_t *parse,
			   char *transport,
			   const char *proto);

#ifdef __cplusplus
}
#endif

#endif

#ifndef __MPEG2T_THREAD_H__
#define __MPEG2T_THREAD_H__ 1
#include "systems.h"

typedef struct mpeg2t_client_ mpeg2t_client_t;
#ifdef __cplusplus
extern "C" {
#endif
mpeg2t_client_t *mpeg2t_create_client(const char *address,
				       in_port_t rx_port,
				       in_port_t tx_port,
				       int use_rtp,
				       double rtcp_bw, 
				       int ttl,
				       char *errmsg,
				      uint32_t errmsg_len);

void mpeg2t_delete_client(mpeg2t_client_t *info);

void mpeg2t_set_loglevel(int loglevel);
void mpeg2t_set_error_func (error_msg_func_t func);

#ifdef __cplusplus
}
#endif
#endif

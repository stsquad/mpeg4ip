#ifndef __MPEG2T_THREAD_H__
#define __MPEG2T_THREAD_H__ 1
#include "mpeg4ip.h"

typedef struct mpeg2t_client_ mpeg2t_client_t;
#ifdef __cplusplus
extern "C" {
#endif
void mpeg2t_delete_client(mpeg2t_client_t *info);
void mpeg2t_set_loglevel(int loglevel);
void mpeg2t_set_error_func (error_msg_func_t func);

#ifdef __cplusplus
}
#endif
#endif

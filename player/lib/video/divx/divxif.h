#ifndef __DIVX_IF_H__
#define __DIVX_IF_H__ 1
#ifdef __cplusplus
extern "C" {
#endif
  
#include "global.h"

  void newdec_init(get_more_t get, void *userdata);

int newdec_read_volvop (unsigned char *buffer, unsigned int buflen);

void post_volprocessing(void);

int newdec_frame(unsigned char *y,
		 unsigned char *u,
		 unsigned char *v,
		 int wait_for_i,
		 unsigned char *buffer,
		 unsigned int buflen);

  int dec_init(const char *infilename, int hor_size, int ver_size);

int dec_release(void);
  int newget_bytes_used(void);
#ifdef __cplusplus
}
#endif
#endif

#ifndef __DIVX_IF_H__
#define __DIVX_IF_H__ 1
#ifdef __cplusplus
extern "C" {
#endif
  
#include "global.h"

void newdec_init(get_t get, bookmark_t book, void *userdata);

int newdec_read_volvop (void);

void post_volprocessing(void);

int newdec_frame(unsigned char *y,
		 unsigned char *u,
		 unsigned char *v,
		 int wait_for_i);

int dec_init(const char *infilename, int hor_size, int ver_size,
	     get_t get, bookmark_t book, void *ud);

int dec_release(void);
#ifdef __cplusplus
}
#endif
#endif

#ifndef _IMAGE_IN_H
#define _IMAGE_IN_H

#include "../portab.h"


typedef void (color_inFunc)(uint8_t *y_out, uint8_t *u_out, uint8_t *v_out,
							uint8_t *src, int width, int height, int stride);

typedef color_inFunc *color_inFuncPtr;	

extern color_inFuncPtr rgb555_to_yuv;
extern color_inFuncPtr rgb565_to_yuv;
extern color_inFuncPtr rgb24_to_yuv;
extern color_inFuncPtr rgb32_to_yuv;
extern color_inFuncPtr yuv_to_yuv;
extern color_inFuncPtr yuyv_to_yuv;
extern color_inFuncPtr uyvy_to_yuv;

/* plain c */
color_inFunc rgb555_to_yuv_c;
color_inFunc rgb565_to_yuv_c;
color_inFunc rgb24_to_yuv_c;
color_inFunc rgb32_to_yuv_c;
color_inFunc yuv_to_yuv_c;
color_inFunc yuyv_to_yuv_c;
color_inFunc uyvy_to_yuv_c;

#ifdef MPEG4IP
void yuv_to_yuv_clip_c(
	uint8_t *y_out, uint8_t *u_out, uint8_t *v_out,
	uint8_t *src, int width, int height, int raw_height, int stride);
#endif

/* mmx */
color_inFunc rgb24_to_yuv_mmx;
color_inFunc rgb32_to_yuv_mmx;
color_inFunc yuv_to_yuv_mmx;
color_inFunc yuyv_to_yuv_mmx;
color_inFunc uyvy_to_yuv_mmx;

/* xmm */
color_inFunc yuv_to_yuv_xmm;

#endif /* _IMAGE_IN_H_ */

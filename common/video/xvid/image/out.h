#ifndef _IMAGE_OUT_H_
#define _IMAGE_OUT_H_


#include "../portab.h"


typedef void (color_outFunc)(uint8_t *dst, int dst_stride,
							 uint8_t *y_src, uint8_t *v_src,
							 uint8_t * u_src, int y_stride,
							 int uv_stride, int width, int height);

typedef color_outFunc* color_outFuncPtr;	

extern color_outFuncPtr yuv_to_rgb24;
extern color_outFuncPtr yuv_to_rgb32;
extern color_outFuncPtr out_yuv;
extern color_outFuncPtr yuv_to_yuyv;
extern color_outFuncPtr yuv_to_uyvy;

/* plain c */
void init_yuv_to_rgb(void);

color_outFunc yuv_to_rgb24_c;
color_outFunc yuv_to_rgb32_c;
color_outFunc out_yuv_c;
color_outFunc yuv_to_yuyv_c;
color_outFunc yuv_to_uyvy_c;

/* mmx */
color_outFunc yuv_to_rgb24_mmx;
color_outFunc yuv_to_rgb32_mmx;
color_outFunc yuv_to_yuyv_mmx;
color_outFunc yuv_to_uyvy_mmx;

#endif /* _IMAGE_OUT_H_ */

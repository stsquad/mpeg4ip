#ifndef _COLORSPACE_H
#define _COLORSPACE_H

#include "../portab.h"


/* initialize tables */

void colorspace_init(void);


/* input color conversion functions (encoder) */

typedef void (color_inputFunc)(uint8_t *y_out, uint8_t *u_out, uint8_t *v_out,
							uint8_t *src, int width, int height, int stride);

typedef color_inputFunc *color_inputFuncPtr;	

extern color_inputFuncPtr rgb555_to_yv12;
extern color_inputFuncPtr rgb565_to_yv12;
extern color_inputFuncPtr rgb24_to_yv12;
extern color_inputFuncPtr rgb32_to_yv12;
extern color_inputFuncPtr yuv_to_yv12;
extern color_inputFuncPtr yuyv_to_yv12;
extern color_inputFuncPtr uyvy_to_yv12;

/* plain c */
color_inputFunc rgb555_to_yv12_c;
color_inputFunc rgb565_to_yv12_c;
color_inputFunc rgb24_to_yv12_c;
color_inputFunc rgb32_to_yv12_c;
color_inputFunc yuv_to_yv12_c;
color_inputFunc yuyv_to_yv12_c;
color_inputFunc uyvy_to_yv12_c;

/* mmx */
color_inputFunc rgb24_to_yv12_mmx;
color_inputFunc rgb32_to_yv12_mmx;
color_inputFunc yuv_to_yv12_mmx;
color_inputFunc yuyv_to_yv12_mmx;
color_inputFunc uyvy_to_yv12_mmx;

/* xmm */
color_inputFunc yuv_to_yv12_xmm;


/* output color conversion functions (decoder) */
 
typedef void (color_outputFunc)(uint8_t *dst, int dst_stride,
							 uint8_t *y_src, uint8_t *v_src,
							 uint8_t * u_src, int y_stride,
							 int uv_stride, int width, int height);

typedef color_outputFunc* color_outputFuncPtr;	

extern color_outputFuncPtr yv12_to_rgb555;
extern color_outputFuncPtr yv12_to_rgb565;
extern color_outputFuncPtr yv12_to_rgb24;
extern color_outputFuncPtr yv12_to_rgb32;
extern color_outputFuncPtr yv12_to_yuv;
extern color_outputFuncPtr yv12_to_yuyv;
extern color_outputFuncPtr yv12_to_uyvy;

/* plain c */
void init_yuv_to_rgb(void);

color_outputFunc yv12_to_rgb555_c;
color_outputFunc yv12_to_rgb565_c;
color_outputFunc yv12_to_rgb24_c;
color_outputFunc yv12_to_rgb32_c;
color_outputFunc yv12_to_yuv_c;
color_outputFunc yv12_to_yuyv_c;
color_outputFunc yv12_to_uyvy_c;

/* mmx */
color_outputFunc yv12_to_rgb24_mmx;
color_outputFunc yv12_to_rgb32_mmx;
color_outputFunc yv12_to_yuyv_mmx;
color_outputFunc yv12_to_uyvy_mmx;

#endif /* _COLORSPACE_H_ */

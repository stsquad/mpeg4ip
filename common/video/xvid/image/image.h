#ifndef _IMAGE_H_
#define _IMAGE_H_

#include "../portab.h"
#include "halfpel.h"
#include "in.h"
#include "out.h"

#define EDGE_SIZE  32


typedef struct
{
	uint8_t * y;
	uint8_t * u;
	uint8_t * v;
} IMAGE;

void init_image(uint32_t cpu_flags);

uint32_t image_create(IMAGE * image, uint32_t edged_width, uint32_t edged_height);
void image_destroy(IMAGE * image, uint32_t edged_width, uint32_t edged_height);

void image_swap(IMAGE * image1, IMAGE * image2);
void image_copy(IMAGE *image1, IMAGE * image2, uint32_t edged_width, uint32_t height);
void image_setedges(IMAGE * image, uint32_t edged_width, uint32_t edged_height, uint32_t width, uint32_t height);
void image_interpolate(const IMAGE * refn, 
					   IMAGE * refh, IMAGE * refv,	IMAGE * refhv, 
					   uint32_t edged_width, uint32_t edged_height, uint32_t rounding);

int image_input(IMAGE * image, uint32_t width, int height, uint32_t edged_width,
			int8_t * src, int csp);

int image_output(IMAGE * image, uint32_t width, int height, uint32_t edged_width,
			int8_t * dst, uint32_t dst_stride, int csp);

#endif /* _IMAGE_H_ */

#ifndef DSPUTIL_H
#define DSPUTIL_H

#include "common.h"
#include <stdint.h>

typedef short DCTELEM;

/* depends on JPEG librarie */
void jpeg_fdct_ifast (DCTELEM *data);

void j_rev_dct (DCTELEM *data);

#define MAX_NEG_CROP 384

/* temporary */
extern UINT32 squareTbl[512];
extern void (*add_pixels_tab[4])(DCTELEM *block, const UINT8 *pixels, int line_size);
extern void (*sub_pixels_tab[4])(DCTELEM *block, const UINT8 *pixels, int line_size);

void dsputil_init(void);
void get_pixels(DCTELEM *block, const UINT8 *pixels, int line_size);
void put_pixels(const DCTELEM *block, UINT8 *pixels, int line_size);

#define add_pixels_2(block, pixels, line_size, dxy) \
   add_pixels_tab[dxy](block, pixels, line_size)

#define sub_pixels_2(block, pixels, line_size, dxy) \
   sub_pixels_tab[dxy](block, pixels, line_size)

#endif

/*
 * colorspace conversion functions
 *    -- yuv to rgb colorspace conversions
 *
 * (c) 2001 Gerd Knorr <kraxel@bytesex.org>
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/time.h>
#ifdef HAVE_ENDIAN_H
# include <endian.h>
#endif
#include "grab-ng.h"

/* ------------------------------------------------------------------- */

#define CLIP         320

#if 0
# define RED_NULL    137
# define BLUE_NULL   156
# define LUN_MUL     360
# define RED_MUL     512
# define BLUE_MUL    512
#else
# define RED_NULL    128
# define BLUE_NULL   128
# define LUN_MUL     256
# define RED_MUL     512
# define BLUE_MUL    512
#endif

#define GREEN1_MUL  (-RED_MUL/2)
#define GREEN2_MUL  (-BLUE_MUL/6)
#define RED_ADD     (-RED_NULL  * RED_MUL)
#define BLUE_ADD    (-BLUE_NULL * BLUE_MUL)
#define GREEN1_ADD  (-RED_ADD/2)
#define GREEN2_ADD  (-BLUE_ADD/6)

/* lookup tables */
static unsigned int  ng_yuv_gray[256];
static unsigned int  ng_yuv_red[256];
static unsigned int  ng_yuv_blue[256];
static unsigned int  ng_yuv_g1[256];
static unsigned int  ng_yuv_g2[256];
static unsigned int  ng_clip[256 + 2 * CLIP];

#define GRAY(val)		ng_yuv_gray[val]
#define RED(gray,red)		ng_clip[ CLIP + gray + ng_yuv_red[red] ]
#define GREEN(gray,red,blue)	ng_clip[ CLIP + gray + ng_yuv_g1[red] +	\
						       ng_yuv_g2[blue] ]
#define BLUE(gray,blue)		ng_clip[ CLIP + gray + ng_yuv_blue[blue] ]

/* ------------------------------------------------------------------- */
/* packed pixel yuv to RGB                                             */

static void
yuv422_to_rgb24(unsigned char *dest, unsigned char *s, int p)
{
    unsigned char *d = dest;
    int gray;
    
    while (p) {
	gray = GRAY(s[0]);
	d[0] = RED(gray,s[3]);
	d[1] = GREEN(gray,s[3],s[1]);
	d[2] = BLUE(gray,s[1]);
	gray = GRAY(s[2]);
	d[3] = RED(gray,s[3]);
	d[4] = GREEN(gray,s[3],s[1]);
	d[5] = BLUE(gray,s[1]);
	d += 6;
	s += 4;
	p -= 2;
    }
}

void
ng_yuv422_to_lut2(unsigned char *dest, unsigned char *s, int p)
{
    unsigned short *d = (unsigned short*)dest;
    int gray;
    
    while (p) {
	gray   = GRAY(s[0]);
	*(d++) =
	    ng_lut_red[RED(gray,s[3])] |
	    ng_lut_green[GREEN(gray,s[3],s[1])] |
	    ng_lut_blue[BLUE(gray,s[1])];
	gray   = GRAY(s[2]);
	*(d++) =
	    ng_lut_red[RED(gray,s[3])] |
	    ng_lut_green[GREEN(gray,s[3],s[1])] |
	    ng_lut_blue[BLUE(gray,s[1])];
	s += 4;
	p -= 2;
    }
}

void
ng_yuv422_to_lut4(unsigned char *dest, unsigned char *s, int p)
{
    unsigned int *d = (unsigned int*)dest;
    int gray;
    
    while (p) {
	gray   = GRAY(s[0]);
	*(d++) =
	    ng_lut_red[RED(gray,s[3])] |
	    ng_lut_green[GREEN(gray,s[3],s[1])] |
	    ng_lut_blue[BLUE(gray,s[1])];
	gray   = GRAY(s[2]);
	*(d++) =
	    ng_lut_red[RED(gray,s[3])] |
	    ng_lut_green[GREEN(gray,s[3],s[1])] |
	    ng_lut_blue[BLUE(gray,s[1])];
	s += 4;
	p -= 2;
    }
}

/* ------------------------------------------------------------------- */
/* planar yuv to RGB                                                   */

void
yuv422p_to_rgb24(void *h, struct ng_video_buf *out, struct ng_video_buf *in)
{
    unsigned char *y,*u,*v;
    unsigned char *dp,*d;
    int i,j,gray;

    dp = out->data;
    y  = in->data;
    u  = y + in->fmt.width * in->fmt.height;
    v  = u + in->fmt.width * in->fmt.height / 2;

    for (i = 0; i < in->fmt.height; i++) {
	d = dp;
	for (j = 0; j < in->fmt.width; j+= 2) {
	    gray   = GRAY(*y);
	    *(d++) = RED(gray,*v);
	    *(d++) = GREEN(gray,*v,*u);
	    *(d++) = BLUE(gray,*u);
	    y++;
	    gray   = GRAY(*y);
	    *(d++) = RED(gray,*v);
	    *(d++) = GREEN(gray,*v,*u);
	    *(d++) = BLUE(gray,*u);
	    y++; u++; v++;
	}
	dp += out->fmt.bytesperline;
    }
}

void
ng_yuv422p_to_lut2(void *h, struct ng_video_buf *out, struct ng_video_buf *in)
{
    unsigned char *y,*u,*v;
    unsigned char *dp;
    unsigned short *d;
    int i,j,gray;

    dp = out->data;
    y  = in->data;
    u  = y + in->fmt.width * in->fmt.height;
    v  = u + in->fmt.width * in->fmt.height / 2;

    for (i = 0; i < in->fmt.height; i++) {
	d = (unsigned short*) dp;
	for (j = 0; j < in->fmt.width; j+= 2) {
	    gray   = GRAY(*y);
	    *(d++) =
		ng_lut_red[RED(gray,*v)] |
		ng_lut_green[GREEN(gray,*v,*u)] |
		ng_lut_blue[BLUE(gray,*u)];
	    y++;
	    gray   = GRAY(*y);
	    *(d++) =
		ng_lut_red[RED(gray,*v)] |
		ng_lut_green[GREEN(gray,*v,*u)] |
		ng_lut_blue[BLUE(gray,*u)];
	    y++; u++; v++;
	}
	dp += out->fmt.bytesperline;
    }
}

void
ng_yuv422p_to_lut4(void *h, struct ng_video_buf *out, struct ng_video_buf *in)
{
    unsigned char *y,*u,*v;
    unsigned char *dp;
    unsigned int  *d;
    int i,j,gray;

    dp = out->data;
    y  = in->data;
    u  = y + in->fmt.width * in->fmt.height;
    v  = u + in->fmt.width * in->fmt.height / 2;

    for (i = 0; i < in->fmt.height; i++) {
	d = (unsigned int*) dp;
	for (j = 0; j < in->fmt.width; j+= 2) {
	    gray   = GRAY(*y);
	    *(d++) =
		ng_lut_red[RED(gray,*v)] |
		ng_lut_green[GREEN(gray,*v,*u)] |
		ng_lut_blue[BLUE(gray,*u)];
	    y++;
	    gray   = GRAY(*y);
	    *(d++) =
		ng_lut_red[RED(gray,*v)] |
		ng_lut_green[GREEN(gray,*v,*u)] |
		ng_lut_blue[BLUE(gray,*u)];
	    y++; u++; v++;
	}
	dp += out->fmt.bytesperline;
    }
}

/* ------------------------------------------------------------------- */

static struct ng_video_conv conv_list[] = {
    {
	NG_GENERIC_PACKED,
	fmtid_in:	VIDEO_YUV422,
	fmtid_out:	VIDEO_RGB24,
	priv:		yuv422_to_rgb24,
    },{
	init:           ng_conv_nop_init,
	fini:           ng_conv_nop_fini,
	frame:          yuv422p_to_rgb24,
	fmtid_in:	VIDEO_YUV422P,
	fmtid_out:	VIDEO_RGB24,
    }
};
static const int nconv = sizeof(conv_list)/sizeof(struct ng_video_conv);

/* ------------------------------------------------------------------- */

void ng_color_yuv2rgb_init(void)
{
    int i;
    
    /* init Lookup tables */
    for (i = 0; i < 256; i++) {
        ng_yuv_gray[i] = i * LUN_MUL >> 8;
        ng_yuv_red[i]  = (RED_ADD    + i * RED_MUL)    >> 8;
        ng_yuv_blue[i] = (BLUE_ADD   + i * BLUE_MUL)   >> 8;
        ng_yuv_g1[i]   = (GREEN1_ADD + i * GREEN1_MUL) >> 8;
        ng_yuv_g2[i]   = (GREEN2_ADD + i * GREEN2_MUL) >> 8;
    }
    for (i = 0; i < CLIP; i++)
        ng_clip[i] = 0;
    for (; i < CLIP + 256; i++)
        ng_clip[i] = i - CLIP;
    for (; i < 2 * CLIP + 256; i++)
        ng_clip[i] = 255;

    /* register stuff */
    ng_conv_register(conv_list,nconv);
}

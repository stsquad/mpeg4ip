/*
 * colorspace conversion functions
 *    -- packed pixel formats (rgb/gray right now)
 *
 * (c) 1998-2001 Gerd Knorr <kraxel@bytesex.org>
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
/* RGB conversions                                                     */

static void
redblue_swap(unsigned char *dest, unsigned char *src, int p)
{
    register unsigned char *s = src;
    register unsigned char *d = dest;

    while (p--) {
	*(d++) = s[2];
	*(d++) = s[1];
	*(d++) = s[0];
	s += 3;
    }
}

static void
bgr24_to_bgr32(unsigned char *dest, unsigned char *src, int p)
{
    register unsigned char *s = src;
    register unsigned char *d = dest;

    while (p--) {
        *(d++) = *(s++);
        *(d++) = *(s++);
        *(d++) = *(s++);
	*(d++) = 0;
    }
}

static void
bgr24_to_rgb32(unsigned char *dest, unsigned char *src, int p)
{
    register unsigned char *s = src;
    register unsigned char *d = dest;

    while (p--) {
	*(d++) = 0;
        *(d++) = s[2];
        *(d++) = s[1];
        *(d++) = s[0];
	s +=3;
    }
}

static void
rgb32_to_rgb24(unsigned char *dest, unsigned char *src, int p)
{
    register unsigned char *s = src;
    register unsigned char *d = dest;

    while (p--) {
	s++;
	*(d++) = *(s++);
	*(d++) = *(s++);
	*(d++) = *(s++);
    }
}

static void
rgb32_to_bgr24(unsigned char *dest, unsigned char *src, int p)
{
    register unsigned char *s = src;
    register unsigned char *d = dest;

    while (p--) {
	s++;
	d[2] = *(s++);
	d[1] = *(s++);
	d[0] = *(s++);
	d += 3;
    }
}

/* 15+16 bpp LE <=> BE */
static void
byteswap_short(unsigned char *dest, unsigned char *src, int p)
{
    register unsigned char *s = src;
    register unsigned char *d = dest;

    while (--p) {
	*(d++) = s[1];
	*(d++) = s[0];
	s += 2;
    }
}

/* ------------------------------------------------------------------- */
/* color => grayscale                                                  */

static void
rgb15_native_gray(unsigned char *dest, unsigned char *s, int p)
{
    int              r,g,b;
    unsigned short  *src = (unsigned short*)s;

    while (p--) {
	r = (src[0] & 0x7c00) >> 10;
	g = (src[0] & 0x03e0) >>  5;
	b =  src[1] & 0x001f;

	*(dest++) = ((3*r + 6*g + b)/10) << 3;
	src++;
    }
}

#if BYTE_ORDER == LITTLE_ENDIAN
static void
rgb15_be_gray(unsigned char *dest, unsigned char *src, int p)
{
    register unsigned char *d = dest;

    while (p--) {
	unsigned char r = (src[0] & 0x7c) >> 2;
	unsigned char g = (src[0] & 0x03) << 3 | (src[1] & 0xe0) >> 5;
	unsigned char b = src[1] & 0x1f;

	*(d++) = ((3*r + 6*g + b)/10) << 3;
	src += 2;
    }
}
#endif

#if BYTE_ORDER == BIG_ENDIAN
static void
rgb15_le_gray(unsigned char *dest, unsigned char *src, int p)
{
    register unsigned char *d = dest;

    while (p--) {
	unsigned char r = (src[1] & 0x7c) >> 2;
	unsigned char g = (src[1] & 0x03) << 3 | (src[0] & 0xe0) >> 5;
	unsigned char b = src[0] & 0x1f;

	*(dest++) = ((3*r + 6*g + b)/10) << 3;
	src += 2;
    }
}
#endif

/* ------------------------------------------------------------------- */

static struct ng_video_conv conv_list[] = {
    {
	/* ----------------------------------- write GRAY -- */ 
#if BYTE_ORDER == BIG_ENDIAN
	NG_GENERIC_PACKED,
	fmtid_in:	VIDEO_RGB15_BE,
	fmtid_out:	VIDEO_GRAY,
	priv:		rgb15_native_gray,
    }, {
	NG_GENERIC_PACKED,
	fmtid_in:	VIDEO_RGB15_LE,
	fmtid_out:	VIDEO_GRAY,
	priv:		rgb15_le_gray,
    }, {
#else
	NG_GENERIC_PACKED,
	fmtid_in:	VIDEO_RGB15_BE,
	fmtid_out:	VIDEO_GRAY,
	priv:		rgb15_be_gray,
    }, {
	NG_GENERIC_PACKED,
	fmtid_in:	VIDEO_RGB15_LE,
	fmtid_out:	VIDEO_GRAY,
	priv:		rgb15_native_gray,
    }, {
#endif
	/* ----------------------------------- write RGB15 -- */ 
	NG_GENERIC_PACKED,
	fmtid_in:	VIDEO_RGB15_LE,
	fmtid_out:	VIDEO_RGB15_BE,
	priv:		byteswap_short,
    }, {
	NG_GENERIC_PACKED,
	fmtid_in:	VIDEO_RGB15_BE,
	fmtid_out:	VIDEO_RGB15_LE,
	priv:		byteswap_short,
    }, {
	/* ----------------------------------- write RGB16 -- */
	NG_GENERIC_PACKED,
	fmtid_in:	VIDEO_RGB16_LE,
	fmtid_out:	VIDEO_RGB16_BE,
	priv:		byteswap_short,
    }, {
	NG_GENERIC_PACKED,
	fmtid_in:	VIDEO_RGB16_BE,
	fmtid_out:	VIDEO_RGB16_LE,
	priv:		byteswap_short,
    }, {
	/* ----------------------------------- write RGB24 -- */
	NG_GENERIC_PACKED,
	fmtid_in:	VIDEO_BGR24,
	fmtid_out:	VIDEO_RGB24,
	priv:		redblue_swap,
    }, {
	NG_GENERIC_PACKED,
	fmtid_in:	VIDEO_RGB24,
	fmtid_out:	VIDEO_BGR24,
	priv:		redblue_swap,
    }, {
	NG_GENERIC_PACKED,
	fmtid_in:	VIDEO_RGB32,
	fmtid_out:	VIDEO_RGB24,
	priv:		rgb32_to_rgb24,
    }, {
	NG_GENERIC_PACKED,
	fmtid_in:	VIDEO_RGB32,
	fmtid_out:	VIDEO_BGR24,
	priv:		rgb32_to_bgr24,
    }, {
	/* ----------------------------------- write RGB32 -- */
	NG_GENERIC_PACKED,
	fmtid_in:	VIDEO_BGR24,
	fmtid_out:	VIDEO_BGR32,
	priv:		bgr24_to_bgr32,
    }, {
	NG_GENERIC_PACKED,
	fmtid_in:	VIDEO_BGR24,
	fmtid_out:	VIDEO_RGB32,
	priv:		bgr24_to_rgb32,
    }
};
static const int nconv = sizeof(conv_list)/sizeof(struct ng_video_conv);

/* ------------------------------------------------------------------- */

void
ng_color_packed_init(void)
{
    ng_conv_register(conv_list,nconv);
}

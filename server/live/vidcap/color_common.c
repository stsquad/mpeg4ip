
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

void*
ng_packed_init(struct ng_video_fmt *out, void *priv)
{
    return priv;
}

void
ng_packed_frame(void *handle, struct ng_video_buf *out,
		struct ng_video_buf *in)
{
    int (*func)(unsigned char *dest, unsigned char *src, int p) = handle;
    unsigned char *sp,*dp;
    int i,sw,dw;

    dw  = (out->fmt.width * ng_vfmt_to_depth[out->fmt.fmtid]) >> 3;
    sw  = (in->fmt.width  * ng_vfmt_to_depth[in->fmt.fmtid])  >> 3;
    if (in->fmt.bytesperline == sw && out->fmt.bytesperline == dw) {
	/* can convert in one go */
	func(out->data, in->data, in->fmt.width * in->fmt.height);
    } else {
	/* convert line by line */
	dp = out->data;
	sp = in->data;
	for (i = 0; i < in->fmt.height; i++) {
	    func(dp,sp,in->fmt.width);
	    dp += out->fmt.bytesperline;
	    sp += in->fmt.bytesperline;
	}
    }
}

/* ------------------------------------------------------------------- */

void*
ng_conv_nop_init(struct ng_video_fmt *out, void *priv)
{
    /* nothing */
    return NULL;
}

void
ng_conv_nop_fini(void *handle)
{
    /* nothing */
}

/*
 * interface to the bsd bktr driver
 *
 *   (c) 2000,01 Gerd Knorr <kraxel@bytesex.org>
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/mman.h>
#ifdef HAVE_ENDIAN_H
# include <endian.h>
#endif
#include <pthread.h>

#include "grab-ng.h"

#if !defined(__OpenBSD__) && !defined(__FreeBSD__)
struct ng_driver bsd_driver;
#else /* BSD */

#include <machine/ioctl_bt848.h>
#include <machine/ioctl_meteor.h>

/* ---------------------------------------------------------------------- */
/* global variables                                                       */

struct bsd_handle {
    int fd;
    int tfd;

    /* formats */
    int                     pf_count;
    struct meteor_pixfmt    pf[64];
    int                     xawtv2pf[VIDEO_FMT_COUNT];
    unsigned char           *map;

    /* attributes */
    int muted;

    /* overlay */
    struct meteor_video     fb,pos;
    struct meteor_geomet    ovgeo;
    struct meteor_pixfmt    *ovfmt;
    struct bktr_clip        clip[BT848_MAX_CLIP_NODE];
    int                     ov_enabled,ov_on;

    /* capture */
    int                     fps;
    long long               start;
    struct ng_video_fmt     fmt;
    struct meteor_video     nofb;
    struct meteor_geomet    capgeo;
    struct meteor_pixfmt    *capfmt;
    struct bktr_clip        noclip[BT848_MAX_CLIP_NODE];
};

/* ---------------------------------------------------------------------- */
/* prototypes                                                             */

/* open/close */
static void*   bsd_open(char *device);
static int     bsd_close(void *handle);

/* attributes */
static int     bsd_flags(void *handle);
static struct ng_attribute* bsd_attrs(void *handle);
static int     bsd_read_attr(void *handle, struct ng_attribute*);
static void    bsd_write_attr(void *handle, struct ng_attribute*, int val);

static int   bsd_setupfb(void *handle, struct ng_video_fmt *fmt, void *base);
static int   bsd_overlay(void *handle, struct ng_video_fmt *fmt, int x, int y,
			 struct OVERLAY_CLIP *oc, int count, int aspect);

/* capture */
static void catchsignal(int signal);
static void siginit(void);
static int bsd_setformat(void *handle, struct ng_video_fmt *fmt);
static int bsd_startvideo(void *handle, int fps, int buffers);
static void bsd_stopvideo(void *handle);
static struct ng_video_buf* bsd_nextframe(void *handle);
static struct ng_video_buf* bsd_getimage(void *handle);

/* tuner */
static unsigned long bsd_getfreq(void *handle);
static void bsd_setfreq(void *handle, unsigned long freq);
static int bsd_tuned(void *handle);

const struct ng_driver bsd_driver = {
    name:          "bktr",
    open:          bsd_open,
    close:         bsd_close,

    capabilities:  bsd_flags,
    list_attrs:    bsd_attrs,
    read_attr:     bsd_read_attr,
    write_attr:    bsd_write_attr,

    setupfb:       bsd_setupfb,
    overlay:       bsd_overlay,

    setformat:     bsd_setformat,
    startvideo:    bsd_startvideo,
    stopvideo:     bsd_stopvideo,
    nextframe:     bsd_nextframe,
    getimage:      bsd_getimage,
    
    getfreq:       bsd_getfreq,
    setfreq:       bsd_setfreq,
    is_tuned:      bsd_tuned,
};

/* ---------------------------------------------------------------------- */

static struct STRTAB inputs[] = {
    {  0, "Television"   },
    {  1, "Composite1"   },
    {  2, "S-Video"      },
    {  3, "CSVIDEO"      },
    { -1, NULL }
};
static int inputs_map[] = {
    METEOR_INPUT_DEV1,
    METEOR_INPUT_DEV0,
    METEOR_INPUT_DEV_SVIDEO,
    METEOR_INPUT_DEV2,
};

static struct STRTAB norms[] = {
    {  0, "NTSC"      },
    {  1, "NTSC-JP"   },
    {  2, "PAL"       },
    {  3, "PAL-M"     },
    {  4, "PAL-N"     },
    {  5, "SECAM"     },
    {  6, "RSVD"      },
    { -1, NULL }
};
static int norms_map[] = {
    BT848_IFORM_F_NTSCM,
    BT848_IFORM_F_NTSCJ,
    BT848_IFORM_F_PALBDGHI,
    BT848_IFORM_F_PALM,
    BT848_IFORM_F_PALN,
    BT848_IFORM_F_SECAM,
    BT848_IFORM_F_RSVD,
};

static struct ng_attribute bsd_attr[] = {
    {
	id:       ATTR_ID_NORM,
	name:     "norm",
	type:     ATTR_TYPE_CHOICE,
	choices:  norms,
    },{
	id:       ATTR_ID_INPUT,
	name:     "input",
	type:     ATTR_TYPE_CHOICE,
	choices:  inputs,
    },{
	id:       ATTR_ID_MUTE,
	name:     "mute",
	type:     ATTR_TYPE_BOOL,
    },{
	id:       ATTR_ID_HUE,
	name:     "hue",
	type:     ATTR_TYPE_INTEGER,
    },{
	id:       ATTR_ID_BRIGHT,
	name:     "bright",
	type:     ATTR_TYPE_INTEGER,
    },{
	id:       ATTR_ID_CONTRAST,
	name:     "contrast",
	type:     ATTR_TYPE_INTEGER,
    },{
	/* end of list */
    }
};

static int single     = METEOR_CAP_SINGLE;
static int start      = METEOR_CAP_CONTINOUS;
static int stop       = METEOR_CAP_STOP_CONT;
static int signal_on  = SIGUSR1;
static int signal_off = METEOR_SIG_MODE_MASK;

/* ---------------------------------------------------------------------- */

#define PREFIX "ioctl: "

static int
xioctl(int fd, int cmd, void *arg)
{
    int rc;

    rc = ioctl(fd,cmd,arg);
    if (0 == rc && ng_debug < 2)
	return 0;
    switch (cmd) {
    case METEORSVIDEO:
    {
	struct meteor_video *a = arg;

	fprintf(stderr,PREFIX "METEORSVIDEO(addr=0x%08lx,width=%ld,bank=%ld,ram=%ld)",
		a->addr,a->width,a->banksize,a->ramsize);
	break;
    }
    case METEORSETGEO:
    {
	struct meteor_geomet *a = arg;

	fprintf(stderr,PREFIX "METEORSETGEO(%dx%d,frames=%d,oformat=%ld)",
		a->columns,a->rows,a->frames,a->oformat);
	break;
    }
    case METEORSACTPIXFMT:
    {
	struct meteor_pixfmt *a = arg;

	fprintf(stderr,PREFIX "METEORSACTPIXFMT(%d,type=%d,bpp=%d,"
		"masks=0x%lx/0x%lx/0x%lx,sb=%d,ss=%d)",
		a->index,a->type,a->Bpp,a->masks[0],a->masks[1],a->masks[2],
		a->swap_bytes,a->swap_shorts);
	break;
    }
    case METEORCAPTUR:
    {
        int *a = arg;

	fprintf(stderr,PREFIX "METEORCAPTUR(%d)",*a);
	break;
    }
    case METEORSSIGNAL:
    {
        int *a = arg;

	fprintf(stderr,PREFIX "METEORSSIGNAL(0x%x)",*a);
	break;
    }
    case BT848SCLIP:
    {
	fprintf(stderr,PREFIX "BT848SCLIP");
	break;
    }
    default:
	fprintf(stderr,PREFIX "UNKNOWN(cmd=0x%x)",cmd);
	break;
    }
    fprintf(stderr,": %s\n",(rc == 0) ? "ok" : strerror(errno));
    return rc;
}

/* ---------------------------------------------------------------------- */

void
bsd_print_format(struct meteor_pixfmt *pf, int format)
{
    switch (pf->type) {
    case METEOR_PIXTYPE_RGB:
	fprintf(stderr,
		"bktr: pf: rgb bpp=%d mask=%ld,%ld,%ld sbytes=%d sshorts=%d",
		pf->Bpp,pf->masks[0],pf->masks[1],pf->masks[2],
		pf->swap_bytes,pf->swap_shorts);
	break;
    case METEOR_PIXTYPE_YUV:
	fprintf(stderr,"bktr: pf: yuv h422 v111 (planar)");
	break;
    case METEOR_PIXTYPE_YUV_PACKED:
	fprintf(stderr,"bktr: pf: yuyv h422 v111 (packed)");
	break;
    case METEOR_PIXTYPE_YUV_12:
	fprintf(stderr,"bktr: pf: yuv h422 v422 (planar)");
	break;
    default:
	fprintf(stderr,"bktr: pf: unknown");
    }
    fprintf(stderr," (fmt=%d)\n",format);
}

/* ---------------------------------------------------------------------- */

static void*
bsd_open(char *filename)
{
    struct bsd_handle *h;
    int format;

    h = malloc(sizeof(*h));
    if (NULL == h)
	return NULL;
    memset(h,0,sizeof(*h));
    
    if (-1 == (h->fd = open(filename,O_RDONLY))) {
	fprintf(stderr,"open %s: %s\n", filename,strerror(errno));
	goto err;
    }

    /* video formats */
    for (format = 0; format < VIDEO_FMT_COUNT; format++)
	h->xawtv2pf[format] = -1;

    for (h->pf_count = 0; h->pf_count < 64; h->pf_count++) {
	h->pf[h->pf_count].index = h->pf_count;
	if (-1 == ioctl(h->fd, METEORGSUPPIXFMT,h->pf+h->pf_count)) {
	    perror("ioctl METEORGSUPPIXFMT");
	    if (0 == h->pf_count)
		goto err;
	    break;
	}
	format = -1;
	switch (h->pf[h->pf_count].type) {
	case METEOR_PIXTYPE_RGB:
	    switch(h->pf[h->pf_count].masks[0]) {
	    case 31744: /* 15 bpp */
	        format = h->pf[h->pf_count].swap_bytes
		    ? VIDEO_RGB15_LE : VIDEO_RGB15_BE;
		break;
	    case 63488: /* 16 bpp */
	        format = h->pf[h->pf_count].swap_bytes
		    ? VIDEO_RGB16_LE : VIDEO_RGB16_BE;
		break;
	    case 16711680: /* 24/32 bpp */
		if (h->pf[h->pf_count].Bpp == 3 &&
		    h->pf[h->pf_count].swap_bytes == 1) {
		    format = VIDEO_BGR24;
		} else if (h->pf[h->pf_count].Bpp == 4 &&
			   h->pf[h->pf_count].swap_bytes == 1 &&
			   h->pf[h->pf_count].swap_shorts == 1) {
		    format = VIDEO_BGR32;
		} else if (h->pf[h->pf_count].Bpp == 4 &&
			   h->pf[h->pf_count].swap_bytes == 0 &&
			   h->pf[h->pf_count].swap_shorts == 0) {
		    format = VIDEO_RGB32;
		}
	    }
	    break;
	case METEOR_PIXTYPE_YUV:
	    format = VIDEO_YUV422P;
	    break;
	case METEOR_PIXTYPE_YUV_PACKED:
	    format = VIDEO_YUV422;
	    break;
	case METEOR_PIXTYPE_YUV_12:
	    /* nothing */
	    break;
	}
	if (-1 != format)
	    h->xawtv2pf[format] = h->pf_count;

	bsd_print_format(h->pf+h->pf_count,format);
    }

    h->map = mmap(0,768*576*4, PROT_READ, MAP_SHARED, h->fd, 0);
    if ((unsigned char*)-1 == h->map) {
	perror("bktr: mmap");
	h->map = NULL;
    }

    if (-1 == (h->tfd = open("/dev/tuner0",O_RDONLY))) {
	fprintf(stderr,"open %s: %s\n", "/dev/tuner0",strerror(errno));
    }
    siginit();

    return h;

 err:
    if (-1 != h->fd)
	close(h->fd);
    if (-1 != h->tfd)
	close(h->tfd);
    if (h)
	free(h);
    return NULL;
}

static int
bsd_close(void *handle)
{
    struct bsd_handle *h = handle;

    if (ng_debug)
	fprintf(stderr, "bktr: close\n");

    close(h->fd);
    if (-1 != h->tfd)
	close(h->tfd);
    if (NULL != h->map)
	munmap(h->map,768*576*4);
    free(h);
    return 0;
}

static int bsd_flags(void *handle)
{
    int ret = 0;

    ret |= CAN_OVERLAY;
    ret |= CAN_CAPTURE;
    ret |= CAN_TUNE;
    return ret;
}

static struct ng_attribute* bsd_attrs(void *handle)
{
    return bsd_attr;
}

/* ---------------------------------------------------------------------- */

static int
bsd_get_range(int id, int *min, int *max, int *get, int *set)
{
    switch (id) {
    case ATTR_ID_HUE:
	*min = BT848_HUEREGMIN;
	*max = BT848_HUEREGMAX;
	*get = BT848_GHUE;
	*set = BT848_SHUE;
	break;
    case ATTR_ID_BRIGHT:
	*min = BT848_BRIGHTREGMIN;
	*max = BT848_BRIGHTREGMAX;
	*get = BT848_GBRIG;
	*set = BT848_SBRIG;
	break;
    case ATTR_ID_CONTRAST:
	*min = BT848_CONTRASTREGMIN;
	*max = BT848_CONTRASTREGMAX;
	*get = BT848_GCONT;
	*set = BT848_SCONT;
	break;
    default:
	return -1;
    }
    return 0;
}

static int bsd_read_attr(void *handle, struct ng_attribute *attr)
{
    struct bsd_handle *h = handle;
    int arg, min, max, get, set, i;
    int value = -1;

    switch (attr->id) {
    case ATTR_ID_NORM:
	if (-1 != xioctl(h->fd,BT848GFMT,&arg))
	    for (i = 0; i < sizeof(norms_map)/sizeof(int); i++)
		if (arg == norms_map[i])
		    value = i;
	break;
    case ATTR_ID_INPUT:
	if (-1 != xioctl(h->fd,METEORGINPUT,&arg))
	    for (i = 0; i < sizeof(inputs_map)/sizeof(int); i++)
		if (arg == inputs_map[i])
		    value = i;
	break;
    case ATTR_ID_MUTE:
	if (-1 != xioctl(h->tfd, BT848_GAUDIO, &arg))
	    value = (arg == AUDIO_MUTE) ? 1 : 0;
	break;
    case ATTR_ID_HUE:
    case ATTR_ID_BRIGHT:
    case ATTR_ID_CONTRAST:
	bsd_get_range(attr->id,&min,&max,&get,&set);
	if (-1 != xioctl(h->tfd,get,&arg)) {
	    value = (arg + min) * 65536 / (max - min);
	    if (value < 0)      value = 0;
	    if (value > 65535)  value = 65535;
	}
	break;
    default:
	break;
    }
    return value;
}

static void bsd_write_attr(void *handle, struct ng_attribute *attr, int value)
{
    struct bsd_handle *h = handle;
    int arg, min, max, get, set;

    switch (attr->id) {
    case ATTR_ID_NORM:
	xioctl(h->fd,BT848SFMT,&norms_map[value]);
	break;
    case ATTR_ID_INPUT:
	xioctl(h->fd,METEORSINPUT,&inputs_map[value]);
	break;
    case ATTR_ID_MUTE:
	h->muted = value;
	arg = h->muted ? AUDIO_MUTE : AUDIO_UNMUTE;
	xioctl(h->tfd, BT848_SAUDIO, &arg);
	break;
    case ATTR_ID_HUE:
    case ATTR_ID_BRIGHT:
    case ATTR_ID_CONTRAST:
	bsd_get_range(attr->id,&min,&max,&get,&set);
	arg = value * (max - min) / 65536 + min;
	if (arg < min)  value = min;
	if (arg > max)  value = max;
	xioctl(h->tfd,set,&arg);
	break;
    default:
	break;
    }
}

static unsigned long bsd_getfreq(void *handle)
{
    struct bsd_handle *h = handle;
    unsigned long freq = 0;

    if (-1 == ioctl(h->tfd, TVTUNER_GETFREQ, &freq))
	perror("ioctl TVTUNER_GETFREQ");
    if (ng_debug)
	fprintf(stderr,"bktr: get freq: %.3f\n",(float)freq/16);
    return freq;
}

static void bsd_setfreq(void *handle, unsigned long freq)
{
    struct bsd_handle *h = handle;

    if (ng_debug)
	fprintf(stderr,"bktr: set freq: %.3f\n",(float)freq/16);
    if (-1 == ioctl(h->tfd, TVTUNER_SETFREQ, &freq))
	perror("ioctl TVTUNER_SETFREQ");
}

static int bsd_tuned(void *handle)
{
    return 0;
}

/* ---------------------------------------------------------------------- */
/* overlay                                                                */

static void
set_overlay(struct bsd_handle *h, int state)
{
    if (h->ov_on == state)
	return;
    h->ov_on = state;
    
    if (state) {
	/* enable */
	xioctl(h->fd, METEORSVIDEO, &h->pos);
	xioctl(h->fd, METEORSETGEO, &h->ovgeo);
	xioctl(h->fd, METEORSACTPIXFMT, h->ovfmt);
	xioctl(h->fd, BT848SCLIP, &h->clip);
	xioctl(h->fd, METEORCAPTUR, &start);
    } else {
	/* disable */
	xioctl(h->fd, METEORCAPTUR, &stop);
    }
}

static int bsd_setupfb(void *handle, struct ng_video_fmt *fmt, void *base)
{
    struct bsd_handle *h = handle;

    h->fb.addr     = (long)base;
    h->fb.width    = fmt->bytesperline;
    h->fb.banksize = fmt->bytesperline * fmt->height;
    h->fb.ramsize  = fmt->bytesperline * fmt->height / 1024;
    return 0;
}

static int bsd_overlay(void *handle, struct ng_video_fmt *fmt, int x, int y,
		       struct OVERLAY_CLIP *oc, int count, int aspect)
{
    struct bsd_handle *h = handle;
    int i,xadjust=0,yadjust=0,win_width,win_height,win_x,win_y;

    h->ov_enabled = 0;
    set_overlay(h,h->ov_enabled);
    if (NULL == fmt)
	return 0;

    if (-1 == h->xawtv2pf[fmt->fmtid])
	return -1;

    /* fixups - fixme: no fixed max size */
    win_x      = x;
    win_y      = y;
    win_width  = fmt->width;
    win_height = fmt->height;
    if (win_width > 768) {
	win_width = 768;
	win_x += (fmt->width - win_width)/2;
    }
    if (win_height > 576) {
	win_height = 576;
	win_y +=  (fmt->height - win_height)/2;
    }
    if (aspect)
	ng_ratio_fixup(&win_width,&win_height,&win_x,&win_y);
    xadjust = win_x - x;
    yadjust = win_y - y;

    /* fill data */
    h->pos           = h->fb;
    h->pos.addr     += win_y*h->pos.width;
    h->pos.addr     += win_x*ng_vfmt_to_depth[fmt->fmtid]>>3;
    h->ovgeo.rows    = win_height;
    h->ovgeo.columns = win_width;
    h->ovgeo.frames  = 1;
    h->ovgeo.oformat = 0x10000;

    if (ng_debug)
	fprintf(stderr,"bktr: overlay win=%dx%d+%d+%d, %d clips\n",
		win_width,win_height,win_x,win_y,count);

    /* clipping */
    memset(h->clip,0,sizeof(h->clip));
    for (i = 0; i < count; i++) {
#if 0
	/* This way it *should* work IMHO ... */
	h->clip[i].x_min      = oc[i].x1 - xadjust;
	h->clip[i].x_max      = oc[i].x2 - xadjust;
	h->clip[i].y_min      = oc[i].y1 - yadjust;
	h->clip[i].y_max      = oc[i].y2 - yadjust;
#else
	/* This way it does work.  Sort of ... */
	h->clip[i].x_min      = (oc[i].y1 - yadjust) >> 1;
	h->clip[i].x_max      = (oc[i].y2 - yadjust) >> 1;
	h->clip[i].y_min      = oc[i].x1 - xadjust;
	h->clip[i].y_max      = oc[i].x2 - xadjust;
#endif
	if (ng_debug)
	    fprintf(stderr,"bktr:   clip x=%d-%d y=%d-%d\n",
		    h->clip[i].x_min,h->clip[i].x_max,
		    h->clip[i].y_min,h->clip[i].y_max);
    }
    h->ovfmt = h->pf+h->xawtv2pf[fmt->fmtid];

    h->ov_enabled = 1;
    set_overlay(h,h->ov_enabled);
    return 0;
}

/* ---------------------------------------------------------------------- */
/* capture                                                                */

static void
catchsignal(int signal)
{
    if (signal == SIGUSR1  &&  ng_debug > 1)
	fprintf(stderr,"bktr: sigusr1\n");
    if (signal == SIGALRM)
	fprintf(stderr,"bktr: sigalrm\n");
}

static void
siginit(void)
{
    struct sigaction act,old;

    memset(&act,0,sizeof(act));
    sigemptyset(&act.sa_mask);
    act.sa_handler  = catchsignal;
    sigaction(SIGUSR1,&act,&old);
    sigaction(SIGALRM,&act,&old);
}

static int bsd_setformat(void *handle, struct ng_video_fmt *fmt)
{
    struct bsd_handle *h = handle;

    if (-1 == h->xawtv2pf[fmt->fmtid])
	return -1;

    if (fmt->width > 768)
	fmt->width = 768;
    if (fmt->height > 576)
	fmt->height = 576;
    fmt->bytesperline = fmt->width * ng_vfmt_to_depth[fmt->fmtid] / 8;

    h->capfmt = h->pf+h->xawtv2pf[fmt->fmtid];
    h->capgeo.rows    = fmt->height;
    h->capgeo.columns = fmt->width;
    h->capgeo.frames  = 1;
    h->capgeo.oformat = 0 /* FIXME */;
    if (fmt->height <= 320)
	h->capgeo.oformat |= METEOR_GEO_ODD_ONLY;
    h->fmt = *fmt;
    return 0;
}

static void
set_capture(struct bsd_handle *h, int state)
{
    if (state) {
	/* enable */
	xioctl(h->fd, METEORSVIDEO, &h->nofb);
	xioctl(h->fd, METEORSETGEO, &h->capgeo);
	xioctl(h->fd, METEORSACTPIXFMT, h->capfmt);
	xioctl(h->fd, BT848SCLIP, &h->noclip);
    } else {
	/* disable */
	xioctl(h->fd, METEORCAPTUR, &stop);
    }
}

static int bsd_startvideo(void *handle, int fps, int buffers)
{
    struct bsd_handle *h = handle;

    set_overlay(h,0);
    h->fps = fps;
    h->start = ng_get_timestamp();
    set_capture(h,1);
    xioctl(h->fd, METEORSSIGNAL, &signal_on);
    xioctl(h->fd, METEORCAPTUR, &start);
    return 0;
}

static void bsd_stopvideo(void *handle)
{
    struct bsd_handle *h = handle;

    h->fps = 0;
    set_capture(h,0);
    xioctl(h->fd, METEORCAPTUR, &stop);
    xioctl(h->fd, METEORSSIGNAL, &signal_off);
    set_overlay(h,h->ov_enabled);
}

static struct ng_video_buf* bsd_nextframe(void *handle)
{
    struct bsd_handle *h = handle;
    struct ng_video_buf *buf;
    int size;
    sigset_t sa_mask;

    size = h->fmt.bytesperline * h->fmt.height;
    buf = ng_malloc_video_buf(&h->fmt,size);

    alarm(1);
    sigfillset(&sa_mask);
    sigdelset(&sa_mask,SIGUSR1);
    sigdelset(&sa_mask,SIGALRM);
    sigsuspend(&sa_mask);
    alarm(0);

    memcpy(buf->data,h->map,size);
    buf->info.ts = ng_get_timestamp() - h->start;
    return buf;
}

static struct ng_video_buf* bsd_getimage(void *handle)
{
    struct bsd_handle *h = handle;
    struct ng_video_buf *buf;
    int size;

    set_overlay(h,0);
    set_capture(h,1);

    size = h->fmt.bytesperline * h->fmt.height;
    buf = ng_malloc_video_buf(&h->fmt,size);
    xioctl(h->fd, METEORCAPTUR, &single);
    memcpy(buf->data,h->map,size);

    set_capture(h,0);
    set_overlay(h,h->ov_enabled);

    return buf;
}

#endif /* BSD */

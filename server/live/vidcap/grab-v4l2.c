/*
 * interface to the v4l2 driver
 *
 *   (c) 1998-2001 Gerd Knorr <kraxel@bytesex.org>
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

#ifndef __linux__
struct ng_driver v4l2_driver;
#else /* __linux__ */

#include <asm/types.h>		/* XXX glibc */
#include "videodev2.h"

/* ---------------------------------------------------------------------- */

/* open+close */
static void*   v4l2_open(char *device);
static int     v4l2_close(void *handle);

/* attributes */
static int     v4l2_flags(void *handle);
static struct ng_attribute* v4l2_attrs(void *handle);
static int     v4l2_read_attr(void *handle, struct ng_attribute*);
static void    v4l2_write_attr(void *handle, struct ng_attribute*, int val);

/* overlay */
static int   v4l2_setupfb(void *handle, struct ng_video_fmt *fmt, void *base);
static int   v4l2_overlay(void *handle, struct ng_video_fmt *fmt, int x, int y,
			  struct OVERLAY_CLIP *oc, int count, int aspect);

/* capture video */
static int v4l2_setformat(void *handle, struct ng_video_fmt *fmt);
static int v4l2_startvideo(void *handle, int fps, int buffers);
static void v4l2_stopvideo(void *handle);
static struct ng_video_buf* v4l2_nextframe(void *handle);
static struct ng_video_buf* v4l2_getimage(void *handle);

/* tuner */
static unsigned long v4l2_getfreq(void *handle);
static void v4l2_setfreq(void *handle, unsigned long freq);
static int v4l2_tuned(void *handle);

/* ---------------------------------------------------------------------- */

#define WANTED_BUFFERS 16

#define MAX_INPUT   16
#define MAX_NORM    16
#define MAX_FORMAT  32
#define MAX_CTRL    32

struct v4l2_handle {
    int                         fd;

    /* device descriptions */
    int                         ninputs,nstds,nfmts;
    struct v4l2_capability	cap;
    struct v4l2_streamparm	streamparm;
    struct v4l2_input		inp[MAX_INPUT];
    struct v4l2_enumstd		std[MAX_NORM];
    struct v4l2_fmtdesc		fmt[MAX_FORMAT];
    struct v4l2_queryctrl	ctl[MAX_CTRL*2];

    /* attributes */
    int                         nattr;
    struct ng_attribute         *attr;

    /* capture */
    int                            fps,first;
    long long                      start;
    struct v4l2_format             fmt_v4l2;
    struct ng_video_fmt            fmt_me;
    struct v4l2_requestbuffers     reqbufs;
    struct v4l2_buffer             buf_v4l2[WANTED_BUFFERS];
    struct ng_video_buf            buf_me[WANTED_BUFFERS];
    int                            queue,waiton;

    /* overlay */
    struct v4l2_framebuffer        ov_fb;
    struct v4l2_window             ov_win;
    struct v4l2_clip               ov_clips[256];
    int                            ov_error;
    int                            ov_enabled;
    int                            ov_on;
};

/* ---------------------------------------------------------------------- */

const struct ng_driver v4l2_driver = {
    name:          "v4l2",
    open:          v4l2_open,
    close:         v4l2_close,

    capabilities:  v4l2_flags,
    list_attrs:    v4l2_attrs,
    read_attr:     v4l2_read_attr,
    write_attr:    v4l2_write_attr,

    setupfb:       v4l2_setupfb,
    overlay:       v4l2_overlay,

    setformat:     v4l2_setformat,
    startvideo:    v4l2_startvideo,
    stopvideo:     v4l2_stopvideo,
    nextframe:     v4l2_nextframe,
    getimage:      v4l2_getimage,
    
    getfreq:       v4l2_getfreq,
    setfreq:       v4l2_setfreq,
    is_tuned:      v4l2_tuned,
};

static __u32 xawtv_pixelformat[VIDEO_FMT_COUNT] = {
    0,                    /* unused   */
    0,                    /* RGB8     */
    V4L2_PIX_FMT_GREY,    /* GRAY8    */
    V4L2_PIX_FMT_RGB555,  /* RGB15_LE */
    V4L2_PIX_FMT_RGB565,  /* RGB16_LE */
    0,                    /* RGB15_BE */
    0,                    /* RGB16_BE */
    V4L2_PIX_FMT_BGR24,   /* BGR24    */
    V4L2_PIX_FMT_BGR32,   /* BGR32    */
    V4L2_PIX_FMT_RGB24,   /* RGB24    */
    0,                    /* RGB32    */
    0,                    /* LUT 2    */
    0,                    /* LUT 4    */
    V4L2_PIX_FMT_YUYV,    /* YUV422   */
    V4L2_PIX_FMT_YVU422P, /* YUV422P  */
    V4L2_PIX_FMT_YUV420,  /* YUV420P  */
};

/* ---------------------------------------------------------------------- */
/* debug output                                                           */

#define PREFIX "ioctl: "

static const char *io_names[] = {
    "QUERYCAP", "1", "ENUM_PIXFMT", "ENUM_FBUFFMT", "G_FMT", "S_FMT",
    "G_COMP", "S_COMP", "REQBUFS", "QUERYBUF", "G_FBUF", "S_FBUF",
    "G_WIN", "S_WIN", "PREVIEW", "QBUF", "16", "DQBUF", "STREAMON",
    "STREAMOFF", "G_PERF", "G_PARM", "S_PARM", "G_STD", "S_STD",
    "ENUMSTD", "ENUMINPUT", "G_CTRL", "S_CTRL", "G_TUNER", "S_TUNER",
    "G_FREQ", "S_FREQ", "G_AUDIO", "S_AUDIO", "35", "QUERYCTRL",
    "QUERYMENU", "G_INPUT", "S_INPUT", "ENUMCVT", "41", "42", "43",
    "44", "45",  "G_OUTPUT", "S_OUTPUT", "ENUMOUTPUT", "G_AUDOUT",
    "S_AUDOUT", "ENUMFX", "G_EFFECT", "S_EFFECT", "G_MODULATOR",
    "S_MODULATOR"
};
static const int io_count = (sizeof(io_names)/sizeof(char*));
#define IONAME(cmd)	((cmd & 0xff) < io_count ? \
			io_names[cmd & 0xff] : "UNKNOWN")

static int
xioctl(int fd, int cmd, void *arg, int mayfail)
{
    int rc;

    rc = ioctl(fd,cmd,arg);
    if (0 == rc && ng_debug < 2)
	return rc;
    if (mayfail && errno == mayfail && ng_debug < 2)
	return rc;
    switch (cmd) {
    case VIDIOC_QUERYCAP:
    {
	struct v4l2_capability *a = arg;
	fprintf(stderr,PREFIX "VIDIOC_QUERYCAP(%s,type=0x%x,in=%d,out=%d,"
		"audio=%d,size=%dx%d-%dx%d,fps=%d,flags=0x%x)",
		a->name,a->type,a->inputs,a->outputs,a->audios,
		a->minwidth,a->minheight,a->maxwidth,a->maxheight,
		a->maxframerate,a->flags);
	break;
    }
    
    case VIDIOC_G_FMT:
    case VIDIOC_S_FMT:
    {
	struct v4l2_format *a = arg;

	fprintf(stderr,PREFIX "VIDIOC_%s(type=%d,",IONAME(cmd),a->type);
	switch (a->type) {
	case V4L2_BUF_TYPE_CAPTURE:
	    fprintf(stderr,
		    "%dx%d,depth=%d,%c%c%c%c,flags=0x%x,bpl=%d,size=%d)",
		    a->fmt.pix.width,a->fmt.pix.height,a->fmt.pix.depth,
		    a->fmt.pix.pixelformat & 0xff,
		    (a->fmt.pix.pixelformat >>  8) & 0xff,
		    (a->fmt.pix.pixelformat >> 16) & 0xff,
		    (a->fmt.pix.pixelformat >> 24) & 0xff,
		    a->fmt.pix.depth,a->fmt.pix.bytesperline,
		    a->fmt.pix.sizeimage);
	    break;
	default:
	    fprintf(stderr,"???)");
	    break;
	}
	break;
    }
    case VIDIOC_REQBUFS:
    {
	struct v4l2_requestbuffers *a = arg;
	
	fprintf(stderr,PREFIX "VIDIOC_REQBUFS(count=%d,type=%d)",
		a->count,a->type);
	break;
    }
    case VIDIOC_QBUF:
    case VIDIOC_DQBUF:
    {
	struct v4l2_buffer *a = arg;
	
	fprintf(stderr,PREFIX "VIDIOC_%s(%d,type=%d,off=%d,len=%d,used=%d,"
		"flags=0x%x,ts=%Ld,seq=%d)",
		IONAME(cmd),a->index,a->type,a->offset,a->length,
		a->bytesused,a->flags,a->timestamp,a->sequence);
	break;
    }
    
    case VIDIOC_G_WIN:
    case VIDIOC_S_WIN:
    {
	struct v4l2_window *a = arg;
	
	fprintf(stderr,PREFIX "VIDIOC_%s(%dx%d+%d+%d,key=0x%x,clips=%d)",
		IONAME(cmd), a->width, a->height, a->x, a->y,
		a->chromakey,a->clipcount);
	break;
    }
    case VIDIOC_PREVIEW:
    {
	int *a = arg;

	fprintf(stderr,PREFIX "VIDIOC_PREVIEW(%s)",*a ? "on" : "off");
	break;
    }
    
    case VIDIOC_QUERYCTRL:
    {
	struct v4l2_queryctrl *a = arg;
	
	fprintf(stderr,PREFIX "VIDIOC_QUERYCTRL(id=%d,%s,%d-%d/%d,def=%d,"
		"type=%d,flags=0x%x)",
		a->id,a->name,a->minimum,a->maximum,a->step,
		a->default_value,a->type,a->flags);
	break;
    }
    case VIDIOC_QUERYMENU:
    {
	struct v4l2_querymenu *a = arg;

	fprintf(stderr,PREFIX "VIDIOC_QUERYMENU(id=%d,index=%d,%s)",
		a->id,a->index,a->name);
	break;
    }
    case VIDIOC_G_CTRL:
    case VIDIOC_S_CTRL:
    {
	struct v4l2_control *a = arg;
	
	fprintf(stderr,PREFIX "VIDIOC_%s(id=%d,value=%d)",
		IONAME(cmd),a->id,a->value);
	break;
    }
    
    default:
	fprintf(stderr,PREFIX "VIDIOC_%s(cmd=0x%x)",IONAME(cmd),cmd);
	break;
    }
    fprintf(stderr,": %s\n",(rc == 0) ? "ok" : strerror(errno));
    return rc;
}

static void
print_bits(char *title, char **names, int count, int value)
{
    int i;
    
    fprintf(stderr,"%s: ",title);
    for (i = 0; i < count; i++) {
	if (value & (1 << i))
	    fprintf(stderr,"%s ",names[i]);
    }
    fprintf(stderr,"\n");
}    

static void
print_device_capabilities(struct v4l2_handle *h)
{
    static char *cap_type[] = {
	"capture",
	"codec",
	"output",
	"fx",
	"vbi",
	"vtr",
	"vtx",
	"radio",
    };
    static char *cap_flags[] = {
	"read",
	"write",
	"streaming",
	"preview",
	"select",
	"tuner",
	"monochrome",
	"teletext"
    };
    static char *ctl_type[] = {
	"integer",
	"boolean",
	"menu"
    };
    static char *cap_parm[] = {
	"highquality",
	"vflip",
	"hflip"
    };

    int i;

    fprintf(stderr,"\n*** v4l2: video device capabilities ***\n");

    /* capabilities */
    fprintf(stderr,"type: %s\n",
	    h->cap.type < sizeof(cap_type)/sizeof(char*) ?
	    cap_type[h->cap.type] : "unknown");
    print_bits("flags",cap_flags,sizeof(cap_flags)/sizeof(char*),h->cap.flags);
    fprintf(stderr,"\n");
    fprintf(stderr,"inputs: %d\naudios: %d\n",h->cap.inputs,h->cap.audios);
    fprintf(stderr,"size: %dx%d => %dx%d\n",
	    h->cap.minwidth,h->cap.minheight,h->cap.maxwidth,h->cap.maxheight);
    fprintf(stderr,"fps: %d max\n",h->cap.maxframerate);

    /* inputs */
    fprintf(stderr,"video inputs:\n");
    for (i = 0; i < h->ninputs; i++) {
	printf("  %d: \"%s\", tuner: %s, audio: %s\n", i, h->inp[i].name,
	       (h->inp[i].type       == V4L2_INPUT_TYPE_TUNER) ? "yes" : "no",
	       (h->inp[i].capability &  V4L2_INPUT_CAP_AUDIO)  ? "yes" : "no");
    }

    /* video standards */
    fprintf(stderr,"video standards:\n");
    for (i = 0; i < h->nstds; i++) {
	printf("  %d: \"%s\"\n", i, h->std[i].std.name);
    }

    /* capture formats */
    fprintf(stderr,"capture formats:\n");
    for (i = 0; i < h->nfmts; i++) {
	printf("  %d: %c%c%c%c, depth=%d,%s \"%s\"\n", i,
	       h->fmt[i].pixelformat & 0xff,
	       (h->fmt[i].pixelformat >>  8) & 0xff,
	       (h->fmt[i].pixelformat >> 16) & 0xff,
	       (h->fmt[i].pixelformat >> 24) & 0xff,
	       h->fmt[i].depth,
	       (h->fmt[i].flags & V4L2_FMT_FLAG_COMPRESSED) ? " compressed" : "",
	       h->fmt[i].description);
    }

    /* capture parameters */
    fprintf(stderr,"capture parameters:\n");
    print_bits("  cap",cap_parm,sizeof(cap_parm)/sizeof(char*),
	       h->streamparm.parm.capture.capability);
    print_bits("  cur",cap_parm,sizeof(cap_parm)/sizeof(char*),
	       h->streamparm.parm.capture.capturemode);
    fprintf(stderr,"  timeperframe=%ld\n",
	    h->streamparm.parm.capture.timeperframe);

    /* controls */
    printf("supported controls:\n");
    for (i = 0; i < MAX_CTRL*2; i++) {
	if (h->ctl[i].id == -1)
	    continue;
	fprintf(stderr,"  %2d: \"%s\", [%d .. %d], step=%d, def=%d, type=%s\n",
		i, h->ctl[i].name,
		h->ctl[i].minimum,h->ctl[i].maximum,
		h->ctl[i].step,h->ctl[i].default_value,
		ctl_type[h->ctl[i].type]);
    }
    fprintf(stderr,"\n");
}

static void
print_bufinfo(struct v4l2_buffer *buf)
{
    static char *type[] = {
	"",
	"capture",
	"codec in",
	"codec out",
	"effects in1",
	"effects in2",
	"effects out",
	"video out"
    };

    fprintf(stderr,"v4l2: buf %d: %s 0x%x+%d, used %d\n",
		   buf->index,
	    	   buf->type < sizeof(type)/sizeof(char*) ?
			type[buf->type] : "unknown",
		   buf->offset,buf->length,buf->bytesused);
}

static void
print_fbinfo(struct v4l2_framebuffer *fb)
{
    static char *fb_cap[] = {
	"extern",
	"chromakey",
	"clipping",
	"scale-up",
	"scale-down"
    };
    static char *fb_flags[] = {
	"primary",
	"overlay",
	"chromakey"
    };

    /* capabilities */
    fprintf(stderr,"v4l2: framebuffer info\n");
    print_bits("  cap",fb_cap,sizeof(fb_cap)/sizeof(char*),fb->capability);
    print_bits("  flags",fb_cap,sizeof(fb_flags)/sizeof(char*),fb->flags);
    fprintf(stderr,"  base: %p %p %p\n",fb->base[0],fb->base[1],fb->base[2]);
    fprintf(stderr,"  format: %dx%d, %c%c%c%c, %d byte\n",
	    fb->fmt.width, fb->fmt.height,
	    fb->fmt.pixelformat & 0xff,
	    (fb->fmt.pixelformat >>  8) & 0xff,
	    (fb->fmt.pixelformat >> 16) & 0xff,
	    (fb->fmt.pixelformat >> 24) & 0xff,
	    fb->fmt.sizeimage);
}

/* ---------------------------------------------------------------------- */
/* helpers                                                                */

static void
get_device_capabilities(struct v4l2_handle *h)
{
    int i;
    
    for (h->ninputs = 0; h->ninputs < h->cap.inputs; h->ninputs++) {
	h->inp[h->ninputs].index = h->ninputs;
	if (-1 == xioctl(h->fd, VIDIOC_ENUMINPUT, &h->inp[h->ninputs], 0))
	    break;
    }
    for (h->nstds = 0; h->nstds < MAX_NORM; h->nstds++) {
	h->std[h->nstds].index = h->nstds;
	if (-1 == xioctl(h->fd, VIDIOC_ENUMSTD, &h->std[h->nstds], EINVAL))
	    break;
    }
    for (h->nfmts = 0; h->nfmts < MAX_FORMAT; h->nfmts++) {
	h->fmt[h->nfmts].index = h->nfmts;
	if (-1 == xioctl(h->fd, VIDIOC_ENUM_PIXFMT, &h->fmt[h->nfmts], EINVAL))
	    break;
    }

    h->streamparm.type = V4L2_BUF_TYPE_CAPTURE;
    ioctl(h->fd,VIDIOC_G_PARM,&h->streamparm);

    /* controls */
    for (i = 0; i < MAX_CTRL; i++) {
	h->ctl[i].id = V4L2_CID_BASE+i;
	if (-1 == xioctl(h->fd, VIDIOC_QUERYCTRL, &h->ctl[i], EINVAL) ||
	    (h->ctl[i].flags & V4L2_CTRL_FLAG_DISABLED))
	    h->ctl[i].id = -1;
    }
    for (i = 0; i < MAX_CTRL; i++) {
	h->ctl[i+MAX_CTRL].id = V4L2_CID_PRIVATE_BASE+i;
	if (-1 == xioctl(h->fd, VIDIOC_QUERYCTRL, &h->ctl[i+MAX_CTRL], EINVAL) ||
	    (h->ctl[i+MAX_CTRL].flags & V4L2_CTRL_FLAG_DISABLED))
	    h->ctl[i+MAX_CTRL].id = -1;
    }
}

static struct STRTAB *
build_norms(struct v4l2_handle *h)
{
    struct STRTAB *norms;
    int i;

    norms = malloc(sizeof(struct STRTAB) * (h->nstds+1));
    for (i = 0; i < h->nstds; i++) {
	norms[i].nr  = i;
	norms[i].str = h->std[i].std.name;
    }
    norms[i].nr  = -1;
    norms[i].str = NULL;
    return norms;
}

static struct STRTAB *
build_inputs(struct v4l2_handle *h)
{
    struct STRTAB *inputs;
    int i;

    inputs = malloc(sizeof(struct STRTAB) * (h->ninputs+1));
    for (i = 0; i < h->ninputs; i++) {
	inputs[i].nr  = i;
	inputs[i].str = h->inp[i].name;
    }
    inputs[i].nr  = -1;
    inputs[i].str = NULL;
    return inputs;
}

/* ---------------------------------------------------------------------- */

static struct V4L2_ATTR {
    int id;
    int v4l2;
} v4l2_attr[] = {
    { ATTR_ID_VOLUME,   V4L2_CID_AUDIO_VOLUME },
    { ATTR_ID_MUTE,     V4L2_CID_AUDIO_MUTE   },
    { ATTR_ID_COLOR,    V4L2_CID_SATURATION   },
    { ATTR_ID_BRIGHT,   V4L2_CID_BRIGHTNESS   },
    { ATTR_ID_HUE,      V4L2_CID_HUE          },
    { ATTR_ID_CONTRAST, V4L2_CID_CONTRAST     },
};
#define NUM_ATTR (sizeof(v4l2_attr)/sizeof(struct V4L2_ATTR))

static int
v4l2_to_me(const struct v4l2_queryctrl *ctl, int value)
{
    int me = 65536;
    int v4l2;
    
    v4l2 = ctl->maximum - ctl->minimum;
    if (v4l2 > 16384) {
	v4l2 >>= 2;
	me   >>= 2;
    }
    value = (value + ctl->minimum) * me / v4l2;
    if (value < 0)      value = 0;
    if (value > 65535)  value = 65535;
    return value;
}

static int
me_to_v4l2(const struct v4l2_queryctrl *ctl, int value)
{
    int me = 65536;
    int v4l2;
    
    v4l2 = ctl->maximum - ctl->minimum;
    if (v4l2 > 16384) {
	v4l2 >>= 2;
	me   >>= 2;
    }
    value = value * v4l2 / me + ctl->minimum;
    if (value < ctl->minimum)  value = ctl->minimum;
    if (value > ctl->maximum)  value = ctl->maximum;
    return value;
}

static struct STRTAB*
v4l2_menu(int fd, const struct v4l2_queryctrl *ctl)
{
    struct STRTAB *menu;
    struct v4l2_querymenu item;
    int i;

    menu = malloc(sizeof(struct STRTAB) * (ctl->maximum-ctl->minimum+2));
    for (i = ctl->minimum; i <= ctl->maximum; i++) {
	item.id = ctl->id;
	item.index = i;
	if (-1 == xioctl(fd, VIDIOC_QUERYMENU, &item, 0)) {
	    free(menu);
	    return NULL;
	}
	menu[i-ctl->minimum].nr  = i;
	menu[i-ctl->minimum].str = strdup(item.name);
    }
    menu[i-ctl->minimum].nr  = -1;
    menu[i-ctl->minimum].str = NULL;
    return menu;
}

static void
v4l2_add_attr(struct v4l2_handle *h, struct v4l2_queryctrl *ctl,
	      int id, struct STRTAB *choices)
{
    static int private_ids = ATTR_ID_COUNT;
    int i;
    
    h->attr = realloc(h->attr,(h->nattr+2) * sizeof(struct ng_attribute));
    memset(h->attr+h->nattr,0,sizeof(struct ng_attribute)*2);
    if (ctl) {
	for (i = 0; i < NUM_ATTR; i++)
	    if (v4l2_attr[i].v4l2 == ctl->id)
		break;
	if (i != NUM_ATTR) {
	    h->attr[h->nattr].id  = v4l2_attr[i].id;
	} else {
	    h->attr[h->nattr].id  = private_ids++;
	}
	h->attr[h->nattr].name    = ctl->name;
	h->attr[h->nattr].priv    = ctl;
	h->attr[h->nattr].defval  = ctl->default_value;
	switch (ctl->type) {
	case V4L2_CTRL_TYPE_INTEGER:
	    h->attr[h->nattr].type    = ATTR_TYPE_INTEGER;
	    h->attr[h->nattr].defval  = v4l2_to_me(ctl,ctl->default_value);
	    break;
	case V4L2_CTRL_TYPE_BOOLEAN:
	    h->attr[h->nattr].type    = ATTR_TYPE_BOOL;
	    break;
	case V4L2_CTRL_TYPE_MENU:
	    h->attr[h->nattr].type    = ATTR_TYPE_CHOICE;
	    h->attr[h->nattr].choices = v4l2_menu(h->fd, ctl);
	    break;
	default:
	    return;
	}
    } else {
	/* for norms + inputs */
	h->attr[h->nattr].id      = id;
	h->attr[h->nattr].defval  = 0;
	h->attr[h->nattr].type    = ATTR_TYPE_CHOICE;
	h->attr[h->nattr].choices = choices;
    }
    if (h->attr[h->nattr].id < ATTR_ID_COUNT)
	h->attr[h->nattr].name = ng_attr_to_desc[h->attr[h->nattr].id];
    h->nattr++;
}

static int v4l2_read_attr(void *handle, struct ng_attribute *attr)
{
    struct v4l2_handle *h = handle;
    const struct v4l2_queryctrl *ctl = attr->priv;
    struct v4l2_control c;
    int value = 0;

    if (NULL != ctl) {
	c.id = ctl->id;
	xioctl(h->fd,VIDIOC_G_CTRL,&c,0);
	if (ctl->type == V4L2_CTRL_TYPE_INTEGER) {
	    value = v4l2_to_me(ctl,c.value);
	} else {
	    value = c.value;
	}
	
    } else if (attr->id == ATTR_ID_NORM) {
	value = -1; /* FIXME */
	
    } else if (attr->id == ATTR_ID_INPUT) {
	xioctl(h->fd,VIDIOC_G_INPUT,&value,0);

    }
    return value;
}

static void v4l2_write_attr(void *handle, struct ng_attribute *attr, int value)
{
    struct v4l2_handle *h   = handle;
    const struct v4l2_queryctrl *ctl = attr->priv;
    struct v4l2_control c;

    if (NULL != ctl) {
	c.id = ctl->id;
	if (ctl->type == V4L2_CTRL_TYPE_INTEGER) {
	    c.value = me_to_v4l2(ctl,value);
	} else {
	    c.value = value;
	}
	xioctl(h->fd,VIDIOC_S_CTRL,&c,0);
	
    } else if (attr->id == ATTR_ID_NORM) {
	xioctl(h->fd,VIDIOC_S_STD,&h->std[value].std,0);
	
    } else if (attr->id == ATTR_ID_INPUT) {
	xioctl(h->fd,VIDIOC_S_INPUT,&value,0);
	
    }
}

/* ---------------------------------------------------------------------- */

static void*
v4l2_open(char *device)
{
    struct v4l2_handle *h;
    int i;

    h = malloc(sizeof(*h));
    if (NULL == h)
	return NULL;
    memset(h,0,sizeof(*h));
    
    if (-1 == (h->fd = open(device,O_RDWR))) {
	fprintf(stderr,"open %s: %s\n",device,strerror(errno));
	goto err;
    }

    if (-1 == ioctl(h->fd,VIDIOC_QUERYCAP,&h->cap))
	goto err;
    if (ng_debug)
	fprintf(stderr, "v4l2: open\n");
    fcntl(h->fd,F_SETFD,FD_CLOEXEC);
    fprintf(stderr,"v4l2: device is %s\n",h->cap.name);

    get_device_capabilities(h);
    if (ng_debug)
	print_device_capabilities(h);

    /* attributes */
    v4l2_add_attr(h, NULL, ATTR_ID_NORM,  build_norms(h));
    v4l2_add_attr(h, NULL, ATTR_ID_INPUT, build_inputs(h));
    for (i = 0; i < MAX_CTRL*2; i++) {
	if (h->ctl[i].id == -1)
	    continue;
	v4l2_add_attr(h, &h->ctl[i], 0, NULL);
    }

    /* capture buffers */
    for (i = 0; i < WANTED_BUFFERS; i++) {
    	ng_init_video_buf(h->buf_me+i);
	h->buf_me[i].release = ng_wakeup_video_buf;
    }

    return h;

 err:
    if (h->fd != -1)
	close(h->fd);
    if (h)
	free(h);
    return NULL;
}

static int
v4l2_close(void *handle)
{
    struct v4l2_handle *h = handle;

    if (ng_debug)
	fprintf(stderr, "v4l2: close\n");

    close(h->fd);
    free(h);
    return 0;
}

static int v4l2_flags(void *handle)
{
    struct v4l2_handle *h = handle;
    int ret = 0;

    if (h->cap.flags & V4L2_FLAG_PREVIEW && !h->ov_error)
	ret |= CAN_OVERLAY;
    if ((h->cap.flags & V4L2_FLAG_STREAMING) ||
	(h->cap.flags & V4L2_FLAG_READ))
	ret |= CAN_CAPTURE;
    if (h->cap.flags & V4L2_FLAG_TUNER)
	ret |= CAN_TUNE;
    return ret;
}

static struct ng_attribute* v4l2_attrs(void *handle)
{
    struct v4l2_handle *h = handle;
    return h->attr;
}

/* ---------------------------------------------------------------------- */

static unsigned long
v4l2_getfreq(void *handle)
{
    struct v4l2_handle *h = handle;
    unsigned long freq;

    xioctl(h->fd, VIDIOC_G_FREQ, &freq, 0);
    return freq;
}

static void
v4l2_setfreq(void *handle, unsigned long freq)
{
    struct v4l2_handle *h = handle;

    if (ng_debug)
	fprintf(stderr,"v4l2: freq: %.3f\n",(float)freq/16);
    xioctl(h->fd, VIDIOC_S_FREQ, &freq, 0);
}

static int
v4l2_tuned(void *handle)
{
    struct v4l2_handle *h = handle;
    struct v4l2_tuner tuner;

    usleep(10000);
    if (-1 == xioctl(h->fd,VIDIOC_G_TUNER,&tuner,0))
	return 0;
    return tuner.signal ? 1 : 0;
}

/* ---------------------------------------------------------------------- */
/* overlay                                                                */

static int
v4l2_setupfb(void *handle, struct ng_video_fmt *fmt, void *base)
{
    struct v4l2_handle *h = handle;

    if (-1 == xioctl(h->fd, VIDIOC_G_FBUF, &h->ov_fb, 0))
	return -1;
    
    if (1 /* ng_debug */)
	print_fbinfo(&h->ov_fb);

    /* double-check settings */
    if (NULL != base && h->ov_fb.base[0] != base) {
	fprintf(stderr,"v4l2: WARNING: framebuffer base address mismatch\n");
	fprintf(stderr,"v4l2: %p %p\n",base,h->ov_fb.base);
	h->ov_error = 1;
	return -1;
    }
    if (h->ov_fb.fmt.width  != fmt->width ||
	h->ov_fb.fmt.height != fmt->height) {
	fprintf(stderr,"v4l2: WARNING: framebuffer size mismatch\n");
	h->ov_error = 1;
	return -1;
    }
    if ((h->ov_fb.fmt.flags & V4L2_FMT_FLAG_BYTESPERLINE) &&
	fmt->bytesperline >  0 &&
	fmt->bytesperline != h->ov_fb.fmt.bytesperline) {
	fprintf(stderr,"v4l2: WARNING: framebuffer bpl mismatch\n");
	h->ov_error = 1;
	return -1;
    }
    if (h->ov_fb.fmt.pixelformat != xawtv_pixelformat[fmt->fmtid]) {
	fprintf(stderr,"v4l2: WARNING: framebuffer format mismatch\n");
	h->ov_error = 1;
	return -1;
    }
    return 0;
}

static int
v4l2_overlay(void *handle, struct ng_video_fmt *fmt, int x, int y,
	     struct OVERLAY_CLIP *oc, int count, int aspect)
{
    struct v4l2_handle *h = handle;
    int i,xadjust=0,yadjust=0;

    if (h->ov_error)
	return -1;
    
    if (NULL == fmt) {
	if (ng_debug)
	    fprintf(stderr,"v4l2: overlay off\n");
	h->ov_enabled = 0;
	h->ov_on = 0;
	xioctl(h->fd, VIDIOC_PREVIEW, &h->ov_on, 0);
	return 0;
    }

    if (ng_debug)
	fprintf(stderr,"v4l2: overlay win=%dx%d+%d+%d, %d clips\n",
		fmt->width,fmt->height,x,y,count);
    h->ov_win.x          = x;
    h->ov_win.y          = y;
    h->ov_win.width      = fmt->width;
    h->ov_win.height     = fmt->height;

    /* check against max. size */
    ioctl(h->fd,VIDIOC_QUERYCAP,&h->cap);
    if (h->ov_win.width > h->cap.maxwidth) {
	h->ov_win.width = h->cap.maxwidth;
	h->ov_win.x += (fmt->width - h->ov_win.width)/2;
    }
    if (h->ov_win.height > h->cap.maxheight) {
	h->ov_win.height = h->cap.maxheight;
	h->ov_win.y +=  (fmt->height - h->ov_win.height)/2;
    }
    if (aspect)
	ng_ratio_fixup(&h->ov_win.width,&h->ov_win.height,
		       &h->ov_win.x,&h->ov_win.y);

    /* fixups */
    xadjust = h->ov_win.x - x;
    yadjust = h->ov_win.y - y;

    if (h->ov_fb.capability & V4L2_FBUF_CAP_CLIPPING) {
	h->ov_win.clips      = h->ov_clips;
	h->ov_win.clipcount  = count;
	
	for (i = 0; i < count; i++) {
	    h->ov_clips[i].next   = (i+1 == count) ? NULL : &h->ov_clips[i+1];
	    h->ov_clips[i].x      = oc[i].x1 - xadjust;
	    h->ov_clips[i].y      = oc[i].y1 - yadjust;
	    h->ov_clips[i].width  = oc[i].x2-oc[i].x1;
	    h->ov_clips[i].height = oc[i].y2-oc[i].y1;
	    if (ng_debug)
		fprintf(stderr,"v4l2: clip=%dx%d+%d+%d\n",
			h->ov_clips[i].width,h->ov_clips[i].height,
			h->ov_clips[i].x,h->ov_clips[i].y);
	}
    }
#if 0
    if (h->ov_fb.flags & V4L2_FBUF_FLAG_CHROMAKEY) {
	h->ov_win.chromakey  = 0;    /* FIXME */
    }
#endif
    xioctl(h->fd, VIDIOC_S_WIN, &h->ov_win, 0);

    h->ov_enabled = 1;
    h->ov_on = 1;
    xioctl(h->fd, VIDIOC_PREVIEW, &h->ov_on, 0);

    return 0;
}

/* ---------------------------------------------------------------------- */
/* capture helpers                                                        */

static int
v4l2_queue_buffer(struct v4l2_handle *h)
{
    int frame = h->queue % h->reqbufs.count;
    int rc;

    if (0 != h->buf_me[frame].refcount) {
	if (0 != h->queue - h->waiton)
	    return -1;
	fprintf(stderr,"v4l2: waiting for a free buffer\n");
	ng_waiton_video_buf(h->buf_me+frame);
    }

    rc = xioctl(h->fd,VIDIOC_QBUF,&h->buf_v4l2[frame], 0);
    if (0 == rc)
	h->queue++;
    return rc;
}

static void
v4l2_queue_all(struct v4l2_handle *h)
{
    for (;;) {
	if (h->queue - h->waiton >= h->reqbufs.count)
	    return;
	if (0 != v4l2_queue_buffer(h))
	    return;
    }
}

static int
v4l2_waiton(struct v4l2_handle *h)
{
    struct v4l2_buffer buf;
    struct timeval tv;
    fd_set rdset;
    
    /* wait for the next frame */
 again:
    tv.tv_sec  = 1;
    tv.tv_usec = 0;
    FD_ZERO(&rdset);
    FD_SET(h->fd, &rdset);
    switch (select(h->fd + 1, &rdset, NULL, NULL, &tv)) {
    case -1:
	if (EINTR == errno)
	    goto again;
	perror("v4l2: select");
	return -1;
    case  0:
	fprintf(stderr,"v4l2: oops: select timeout\n");
	return -1;
    }

    /* get it */
    memset(&buf,0,sizeof(buf));
    buf.type = V4L2_BUF_TYPE_CAPTURE;
    if (-1 == xioctl(h->fd,VIDIOC_DQBUF,&buf, 0))
	return -1;
    h->waiton++;
    h->buf_v4l2[buf.index] = buf;
    return buf.index;
}

static int
v4l2_start_streaming(struct v4l2_handle *h, int buffers)
{
    int disable_overlay = 0;
    int i;
    
    /* setup buffers */
    h->reqbufs.count = buffers;
    h->reqbufs.type  = V4L2_BUF_TYPE_CAPTURE;
    if (-1 == xioctl(h->fd, VIDIOC_REQBUFS, &h->reqbufs, 0))
	return -1;
    for (i = 0; i < h->reqbufs.count; i++) {
	h->buf_v4l2[i].index = i;
	h->buf_v4l2[i].type  = V4L2_BUF_TYPE_CAPTURE;
	if (-1 == ioctl(h->fd, VIDIOC_QUERYBUF, &h->buf_v4l2[i]))
	    return -1;
	h->buf_me[i].fmt  = h->fmt_me;
	h->buf_me[i].size = h->buf_v4l2[i].length;
	h->buf_me[i].data = mmap(NULL, h->buf_me[i].size,
				 PROT_READ | PROT_WRITE, MAP_SHARED,
				 h->fd, h->buf_v4l2[i].offset);
	if ((void*)-1 == h->buf_me[i].data) {
	    perror("mmap");
	    return -1;
	}
	if (ng_debug)
	    print_bufinfo(&h->buf_v4l2[i]);
    }

    /* queue up all buffers */
    v4l2_queue_all(h);

 try_again:
    /* turn off preview (if needed) */
    if (disable_overlay) {
	h->ov_on = 0;
	xioctl(h->fd, VIDIOC_PREVIEW, &h->ov_on, 0);
	if (ng_debug)
	    fprintf(stderr,"v4l2: overlay off (start_streaming)\n");
    }

    /* start capture */
    if (-1 == xioctl(h->fd,VIDIOC_STREAMON,&h->fmt_v4l2.type,
		     h->ov_on ? EBUSY : 0)) {
	if (h->ov_on && errno == EBUSY) {
	    disable_overlay = 1;
	    goto try_again;
	}
	return -1;
    }
    return 0;
}

static void
v4l2_stop_streaming(struct v4l2_handle *h)
{
    int i;
    
    /* stop capture */
    if (-1 == ioctl(h->fd,VIDIOC_STREAMOFF,&h->fmt_v4l2.type))
	perror("ioctl VIDIOC_STREAMOFF");
    
    /* free buffers */
    for (i = 0; i < h->reqbufs.count; i++) {
	if (0 != h->buf_me[i].refcount)
	    ng_waiton_video_buf(&h->buf_me[i]);
	if (-1 == munmap(h->buf_me[i].data,h->buf_me[i].size))
	    perror("munmap");
    }
    h->queue = 0;
    h->waiton = 0;

    /* turn on preview (if needed) */
    if (h->ov_on != h->ov_enabled) {
	h->ov_on = h->ov_enabled;
	xioctl(h->fd, VIDIOC_PREVIEW, &h->ov_on, 0);
	if (ng_debug)
	    fprintf(stderr,"v4l2: overlay on (stop_streaming)\n");
    }
}

/* ---------------------------------------------------------------------- */
/* capture interface                                                      */

/* set capture parameters */
static int
v4l2_setformat(void *handle, struct ng_video_fmt *fmt)
{
    struct v4l2_handle *h = handle;
    
    h->fmt_v4l2.type = V4L2_BUF_TYPE_CAPTURE;
    h->fmt_v4l2.fmt.pix.pixelformat  = xawtv_pixelformat[fmt->fmtid];
    h->fmt_v4l2.fmt.pix.flags        = V4L2_FMT_FLAG_INTERLACED;
    h->fmt_v4l2.fmt.pix.depth        = ng_vfmt_to_depth[fmt->fmtid];
    h->fmt_v4l2.fmt.pix.width        = fmt->width;
    h->fmt_v4l2.fmt.pix.height       = fmt->height;
    h->fmt_v4l2.fmt.pix.bytesperline = fmt->bytesperline;

    if (-1 == xioctl(h->fd, VIDIOC_S_FMT, &h->fmt_v4l2, EINVAL))
	return -1;
    if (h->fmt_v4l2.fmt.pix.pixelformat != xawtv_pixelformat[fmt->fmtid])
	return -1;
    fmt->width        = h->fmt_v4l2.fmt.pix.width;
    fmt->height       = h->fmt_v4l2.fmt.pix.height;
    fmt->bytesperline = h->fmt_v4l2.fmt.pix.bytesperline;
    if (0 == fmt->bytesperline)
	fmt->bytesperline = fmt->width * ng_vfmt_to_depth[fmt->fmtid] / 8;
    h->fmt_me = *fmt;
    if (ng_debug)
	fprintf(stderr,"v4l2: new capture params (%dx%d, %c%c%c%c, %d byte)\n",
		fmt->width,fmt->height,
		h->fmt_v4l2.fmt.pix.pixelformat & 0xff,
		(h->fmt_v4l2.fmt.pix.pixelformat >>  8) & 0xff,
		(h->fmt_v4l2.fmt.pix.pixelformat >> 16) & 0xff,
		(h->fmt_v4l2.fmt.pix.pixelformat >> 24) & 0xff,
		h->fmt_v4l2.fmt.pix.sizeimage);
    return 0;
}

/* start/stop video */
static int
v4l2_startvideo(void *handle, int fps, int buffers)
{
    struct v4l2_handle *h = handle;

    if (0 != h->fps)
	fprintf(stderr,"v4l2_startvideo: oops: fps!=0\n");
    h->fps = fps;
    h->first = 1;
    h->start = 0;

    if (h->cap.flags & V4L2_FLAG_STREAMING)
	v4l2_start_streaming(h,buffers);
    return 0;
}

static void
v4l2_stopvideo(void *handle)
{
    struct v4l2_handle *h = handle;

    if (0 == h->fps)
	fprintf(stderr,"v4l2_stopvideo: oops: fps==0\n");
    h->fps = 0;

    if (h->cap.flags & V4L2_FLAG_STREAMING)
	v4l2_stop_streaming(h);
}

/* read images */
static struct ng_video_buf*
v4l2_nextframe(void *handle)
{
    struct v4l2_handle *h = handle;
    struct ng_video_buf *buf = NULL;
    int rc,size,frame = 0;

    if (h->cap.flags & V4L2_FLAG_STREAMING) {
	v4l2_queue_all(h);
	frame = v4l2_waiton(h);
	if (-1 == frame)
	    return NULL;
	h->buf_me[frame].refcount++;
	buf = &h->buf_me[frame];
	memset(&buf->info,0,sizeof(buf->info));
	buf->info.ts = h->buf_v4l2[frame].timestamp;
    } else {
	size = h->fmt_me.bytesperline * h->fmt_me.height;
	buf = ng_malloc_video_buf(&h->fmt_me,size);
	rc = read(h->fd,buf->data,size);
	if (rc != size) {
	    if (-1 == rc) {
		perror("v4l2: read");
	    } else {
		fprintf(stderr, "v4l2: read: rc=%d/size=%d\n",rc,size);
	    }
	    ng_release_video_buf(buf);
	    return NULL;
	}
	memset(&buf->info,0,sizeof(buf->info));
	buf->info.ts = ng_get_timestamp();
    }

    if (h->first) {
	h->first = 0;
	h->start = buf->info.ts;
	if (ng_debug)
	    fprintf(stderr,"v4l2: start ts=%lld\n",h->start);
    }
    buf->info.ts -= h->start;
    return buf;
}

static struct ng_video_buf*
v4l2_getimage(void *handle)
{
    struct v4l2_handle *h = handle;
    struct ng_video_buf *buf; 
    int size,frame,rc;

    size = h->fmt_me.bytesperline * h->fmt_me.height;
    buf = ng_malloc_video_buf(&h->fmt_me,size);
    if (h->cap.flags & V4L2_FLAG_READ) {
	rc = read(h->fd,buf->data,size);
	if (rc != size) {
	    if (-1 == rc) {
		perror("v4l2: read");
	    } else {
		fprintf(stderr, "v4l2: read: rc=%d/size=%d\n",rc,size);
	    }
	    ng_release_video_buf(buf);
	    return NULL;
	}
    } else {
	v4l2_start_streaming(h,1);
	frame = v4l2_waiton(h);
	if (-1 == frame) {
	    v4l2_stop_streaming(h);
	    return NULL;
	}
	memcpy(buf->data,h->buf_me[0].data,size);
	v4l2_stop_streaming(h);
    }
    return buf;
}

#endif /* __linux__ */

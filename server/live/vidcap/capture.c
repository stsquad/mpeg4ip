#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include <pthread.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#include "grab-ng.h"
#include "capture.h"

static const struct ng_driver     *drv;
static void                       *h_drv;
static struct ng_attribute        *a_drv;
static int                         f_drv;

const struct ng_driver* ng_open(char* v4l_device)
{
	drv = ng_grabber_open(v4l_device, NULL, 0, &h_drv);
	if (drv == NULL) {
		return NULL;
	}
	f_drv = drv->capabilities(h_drv);
	a_drv = drv->list_attrs(h_drv);
	if (!(f_drv & CAN_CAPTURE)) {
		/* TBD leaking here */
		return NULL;
	}
	return drv;
}

/*-------------------------------------------------------------------------*/
/* data fifos (audio/video)                                                */

void
fifo_init(struct FIFO *fifo, char *name, int slots)
{
    pthread_mutex_init(&fifo->lock, NULL);
    pthread_cond_init(&fifo->hasdata, NULL);
    fifo->name  = name;
    fifo->slots = slots;
    fifo->read  = 0;
    fifo->write = 0;
    fifo->eof   = 0;
}

int
fifo_put(struct FIFO *fifo, void *data)
{
    pthread_mutex_lock(&fifo->lock);
    if (NULL == data) {
	fifo->eof = 1;
	pthread_cond_signal(&fifo->hasdata);
	pthread_mutex_unlock(&fifo->lock);
	return 0;
    }
    if ((fifo->write + 1) % fifo->slots == fifo->read) {
	pthread_mutex_unlock(&fifo->lock);
	fprintf(stderr,"fifo %s is full\n",fifo->name);
	return -1;
    }
    if (ng_debug > 1)
	fprintf(stderr,"put %s %d=%p [pid=%d]\n",
		fifo->name,fifo->write,data,getpid());
    fifo->data[fifo->write] = data;
    fifo->write++;
    if (fifo->write >= fifo->slots)
	fifo->write = 0;
    pthread_cond_signal(&fifo->hasdata);
    pthread_mutex_unlock(&fifo->lock);
    return 0;
}

void*
fifo_get(struct FIFO *fifo)
{
    void *data;

    pthread_mutex_lock(&fifo->lock);
    while (fifo->write == fifo->read && 0 == fifo->eof) {
	pthread_cond_wait(&fifo->hasdata, &fifo->lock);
    }
    if (fifo->write == fifo->read) {
	pthread_cond_signal(&fifo->hasdata);
	pthread_mutex_unlock(&fifo->lock);
	return NULL;
    }
    if (ng_debug > 1)
	fprintf(stderr,"get %s %d=%p [pid=%d]\n",
		fifo->name,fifo->read,fifo->data[fifo->read],getpid());
    data = fifo->data[fifo->read];
    fifo->read++;
    if (fifo->read >= fifo->slots)
	fifo->read = 0;
    pthread_mutex_unlock(&fifo->lock);
    return data;
}

static void*
flushit(void *arg)
{
    int old;

    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE,&old);
    pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS,&old);
    for (;;) {
	sleep(1);
	sync();
    }
    return NULL;
}

/*-------------------------------------------------------------------------*/
/* color space conversion / compression functions + thread                 */

struct ng_convert_handle {
    /* converter data / state */
    struct ng_video_fmt     ifmt;
    struct ng_video_fmt     ofmt;
    int                     isize;
    int                     osize;
    struct ng_video_conv    *conv;
    void                    *chandle;

    /* thread data */
    struct FIFO             *in;
    struct FIFO             *out;
};

struct ng_convert_handle*
ng_convert_alloc(struct ng_video_conv *conv,
		 struct ng_video_fmt *i,
		 struct ng_video_fmt *o)
{
    struct ng_convert_handle *h;
    
    h = malloc(sizeof(*h));
    if (NULL == h)
	return 0;
    memset(h,0,sizeof(*h));

    /* fixup output image size to match incoming */
    o->width  = i->width;
    o->height = i->height;
    if (0 == o->bytesperline)
	o->bytesperline = o->width * ng_vfmt_to_depth[o->fmtid] / 8;

    h->ifmt = *i;
    h->ofmt = *o;
    if (conv)
	h->conv = conv;
    return h;
}

void
ng_convert_init(struct ng_convert_handle *h)
{
    if (0 == h->ifmt.bytesperline)
	h->ifmt.bytesperline = h->ifmt.width *
	    ng_vfmt_to_depth[h->ifmt.fmtid] / 8;
    if (0 == h->ofmt.bytesperline)
	h->ofmt.bytesperline = h->ofmt.width *
	    ng_vfmt_to_depth[h->ofmt.fmtid] / 8;

    h->isize = h->ifmt.height * h->ifmt.bytesperline;
    if (0 == h->isize)
	h->isize = h->ifmt.width * h->ifmt.height * 3;
    h->osize = h->ofmt.height * h->ofmt.bytesperline;
    if (0 == h->osize)
	h->osize = h->ofmt.width * h->ofmt.height * 3;

    if (h->conv)
	h->chandle = h->conv->init(&h->ofmt,h->conv->priv);

    if (ng_debug) {
	fprintf(stderr,"convert-in : %dx%d %s (size=%d)\n",
		h->ifmt.width, h->ifmt.height,
		ng_vfmt_to_desc[h->ifmt.fmtid], h->isize);
	fprintf(stderr,"convert-out: %dx%d %s (size=%d)\n",
		h->ofmt.width, h->ofmt.height,
		ng_vfmt_to_desc[h->ofmt.fmtid], h->osize);
    }
}

static void
ng_convert_copyframe(struct ng_video_buf *dest,
		     struct ng_video_buf *src)
{
    int i,sw,dw;
    unsigned char *sp,*dp;

    dw = dest->fmt.width * ng_vfmt_to_depth[dest->fmt.fmtid] / 8;
    sw = src->fmt.width * ng_vfmt_to_depth[src->fmt.fmtid] / 8;
    if (src->fmt.bytesperline == sw && dest->fmt.bytesperline == dw) {
	/* can copy in one go */
	memcpy(dest->data, src->data,
	       src->fmt.bytesperline * src->fmt.height);
    } else {
	/* copy line by line */
	dp = dest->data;
	sp = src->data;
	for (i = 0; i < src->fmt.height; i++) {
	    memcpy(dp,sp,dw);
	    dp += dest->fmt.bytesperline;
	    sp += src->fmt.bytesperline;
	}
    }
}

struct ng_video_buf*
ng_convert_frame(struct ng_convert_handle *h,
		 struct ng_video_buf *dest,
		 struct ng_video_buf *buf)
{
    if (NULL == buf)
	return NULL;

    if (NULL == dest && NULL != h->conv)
        dest = ng_malloc_video_buf(&h->ofmt,h->osize);

    if (NULL != dest) {
	dest->fmt  = h->ofmt;
	dest->size = h->osize;
	if (NULL != h->conv) {
	    h->conv->frame(h->chandle,dest,buf);
	} else {
	    ng_convert_copyframe(dest,buf);
	}
	dest->info = buf->info;
	ng_release_video_buf(buf);
	buf = dest;
    }

    return buf;
}

void
ng_convert_fini(struct ng_convert_handle *h)
{
    if (h->conv)
	h->conv->fini(h->chandle);
    free(h);
}

struct ng_video_buf*
ng_convert_single(struct ng_convert_handle *h, struct ng_video_buf *in)
{
    struct ng_video_buf *out;

    ng_convert_init(h);
    out = ng_convert_frame(h,NULL,in);
    ng_convert_fini(h);
    return out;
}

void*
ng_convert_thread(void *arg)
{
    struct ng_convert_handle *h = arg;
    struct ng_video_buf *in, *out;
    
    if (ng_debug)
	fprintf(stderr,"convert_thread start [pid=%d]\n",getpid());
    ng_convert_init(h);
    for (;;) {
	in  = fifo_get(h->in);
	if (NULL == in)
	    break;
	out = ng_convert_frame(h,NULL,in);
	fifo_put(h->out,out);
    }
    fifo_put(h->out,NULL);
    ng_convert_fini(h);
    if (ng_debug)
	fprintf(stderr,"convert_thread done\n");
    return NULL;
}

/*-------------------------------------------------------------------------*/
/* parameter negotiation -- look what the driver can do and what           */
/* convert functions are available                                         */

int
ng_grabber_setformat(struct ng_video_fmt *fmt, int fix_ratio)
{
    struct ng_video_fmt gfmt;
    int rc;
    
    /* no capture support */
    if (!(f_drv & CAN_CAPTURE))
	return -1;

    /* try setting the format */
    gfmt = *fmt;
    rc = drv->setformat(h_drv,&gfmt);
    if (ng_debug)
	fprintf(stderr,"setformat: %s (%dx%d): %s\n",
		ng_vfmt_to_desc[gfmt.fmtid],
		gfmt.width,gfmt.height,
		(0 == rc) ? "ok" : "failed");
    if (0 != rc)
	return -1;

    if (fix_ratio) {
	/* fixup aspect ratio if needed */
	ng_ratio_fixup(&gfmt.width, &gfmt.height, NULL, NULL);
	gfmt.bytesperline = 0;
	if (0 != drv->setformat(h_drv,&gfmt)) {
	    fprintf(stderr,"Oops: ratio size renegotiation failed\n");
	    exit(1);
	}
    }

    /* return the real format the grabber uses now */
    *fmt = gfmt;
    return 0;
}

struct ng_video_conv*
ng_grabber_findconv(struct ng_video_fmt *fmt,
		    int fix_ratio)
{
    struct ng_video_fmt  gfmt;
    struct ng_video_conv *conv;
    int i;
    
    /* check all available conversion functions */
    for (i = 0;;) {
	conv = ng_conv_find(fmt->fmtid, &i);
	if (NULL == conv)
	    break;
	gfmt = *fmt;
	gfmt.fmtid = conv->fmtid_in;
	if (0 == ng_grabber_setformat(&gfmt,fix_ratio))
	    goto found;
    }
    fprintf(stderr,"no way to get: %dx%d %s\n",
	    fmt->width,fmt->height,ng_vfmt_to_desc[fmt->fmtid]);
    return NULL;

 found:
    *fmt = gfmt;
    return conv;
}

struct ng_video_buf*
ng_grabber_grab_image(int single)
{
    return single ? drv->getimage(h_drv) : drv->nextframe(h_drv);
}

struct ng_video_buf*
ng_grabber_get_image(struct ng_video_fmt *fmt)
{
    struct ng_video_fmt gfmt;
    struct ng_video_conv *conv;
    struct ng_convert_handle *ch;
    struct ng_video_buf *buf;
    
    if (0 == ng_grabber_setformat(fmt,1))
	return ng_grabber_grab_image(1);
    gfmt = *fmt;
    if (NULL == (conv = ng_grabber_findconv(&gfmt,1)))
	return NULL;
    ch = ng_convert_alloc(conv,&gfmt,fmt);
    if (NULL == (buf = ng_grabber_grab_image(1)))
	return NULL;
    buf = ng_convert_single(ch,buf);
    return buf;
}

/*-------------------------------------------------------------------------*/

struct movie_handle {
    /* general */
    pthread_mutex_t           lock;
    const struct ng_writer    *writer;
    void                      *handle;
    pthread_t                 tflush;
    long long                 start;
    long long                 rts;
    long long                 stopby;
    int                       slots;

    /* video */
    struct ng_video_fmt       vfmt;
    int                       fps;
    int                       frames;
    int                       seq;
    struct FIFO               vfifo;
    pthread_t                 tvideo;
    long long                 vts;

    /* video converter thread */
    struct ng_convert_handle  *hconv;
    struct FIFO               cfifo;
    pthread_t                 tconv;

    /* audio */
    void                      *sndhandle;
    struct ng_audio_fmt       afmt;
    unsigned long             bytes_per_sec;
    unsigned long             bytes;
    struct FIFO               afifo;
    pthread_t                 taudio;
    pthread_t                 raudio;
    long long                 ats;
};

#ifdef NOTDEF
static void*
writer_audio_thread(void *arg)
{
    struct movie_handle *h = arg;
    struct ng_audio_buf *buf;

    if (ng_debug)
	fprintf(stderr,"writer_audio_thread start [pid=%d]\n",getpid());
    for (;;) {
	buf = fifo_get(&h->afifo);
	if (NULL == buf)
	    break;
	pthread_mutex_lock(&h->lock);
	h->writer->wr_audio(h->handle,buf);
	pthread_mutex_unlock(&h->lock);
	free(buf);
    }
    if (ng_debug)
	fprintf(stderr,"writer_audio_thread done\n");
    return NULL;
}
#endif

static void *
writer_video_thread(void *arg)
{
    struct movie_handle *h = arg;
    struct ng_video_buf *buf;

    if (ng_debug)
	fprintf(stderr,"writer_video_thread start [pid=%d]\n",getpid());
    for (;;) {
        buf = fifo_get(&h->vfifo);
	if (NULL == buf)
	    break;
	pthread_mutex_lock(&h->lock);
	h->writer->wr_video(h->handle,buf);
#if 0
	fprintf(stderr,"ts=%lld seq=%d twice=%d\n",
		buf->info.ts,buf->info.seq,buf->info.twice);
#endif
	if (buf->info.twice)
	    h->writer->wr_video(h->handle,buf);
	pthread_mutex_unlock(&h->lock);
	ng_release_video_buf(buf);
    }
    if (ng_debug)
	fprintf(stderr,"writer_video_thread done\n");
    return NULL;
}

#ifdef NOTDEF
static void*
record_audio_thread(void *arg)
{
    struct movie_handle *h = arg;
    struct ng_audio_buf *buf;

    if (ng_debug)
	fprintf(stderr,"record_audio_thread start [pid=%d]\n",getpid());
    for (;;) {
	buf = oss_read(h->sndhandle,h->stopby);
	if (NULL == buf)
	    break;
	if (0 == buf->size)
	    continue;
	h->ats = buf->ts;
	fifo_put(&h->afifo,buf);
    }
    fifo_put(&h->afifo,NULL);
    if (ng_debug)
	fprintf(stderr,"record_audio_thread done\n");
    return NULL;
}
#endif

struct movie_handle*
movie_writer_init(char *moviename, char *audioname,
		  const struct ng_writer *writer, 
		  struct ng_video_fmt *video,const void *priv_video,int fps,
		  struct ng_audio_fmt *audio,const void *priv_audio,char *dsp,
		  int slots)
{
    struct movie_handle *h;
    struct ng_video_conv *conv;
    void *dummy;

    if (ng_debug)
	fprintf(stderr,"movie_init_writer start\n");
    h = malloc(sizeof(*h));
    if (NULL == h)
	return NULL;
    memset(h,0,sizeof(*h));
    pthread_mutex_init(&h->lock, NULL);
    h->writer = writer;
    h->slots = slots;

#ifdef NOTDEF
    /* audio */
    if (audio->fmtid != AUDIO_NONE) {
	h->sndhandle = oss_open(dsp,audio);
	if (NULL == h->sndhandle) {
	    free(h);
	    return NULL;
	}
	fifo_init(&h->afifo,"audio",slots);
	pthread_create(&h->taudio,NULL,writer_audio_thread,h);
	h->bytes_per_sec = ng_afmt_to_bits[audio->fmtid] *
	    ng_afmt_to_channels[audio->fmtid] * audio->rate / 8;
	h->afmt = *audio;
    }
#endif

    /* video */
    if (video->fmtid != VIDEO_NONE) {
#if 0
	if (-1 == ng_grabber_setparams(video,0)) {
	    if (h->afmt.fmtid != AUDIO_NONE)
		oss_close(h->sndhandle);
	    free(h);
	    return NULL;
	}
	fifo_init(&h->vfifo,"video",slots);
	pthread_create(&h->tvideo,NULL,writer_video_thread,h);
#else
	if (0 == ng_grabber_setformat(video,1)) {
	    /* native format works -- no conversion needed */
	    fifo_init(&h->vfifo,"video",slots);
	    pthread_create(&h->tvideo,NULL,writer_video_thread,h);
	} else {
	    /* have to convert video frames */
	    struct ng_video_fmt gfmt = *video;
	    if (NULL == (conv = ng_grabber_findconv(&gfmt,1)) ||
		NULL == (h->hconv = ng_convert_alloc(conv,&gfmt,video))) {
#ifdef NOTDEF
		if (h->afmt.fmtid != AUDIO_NONE)
		    oss_close(h->sndhandle);
#endif
		free(h);
		return NULL;
	    }
	    fifo_init(&h->vfifo,"video",slots);
	    fifo_init(&h->cfifo,"conv",slots);
	    h->hconv->in  = &h->cfifo;
	    h->hconv->out = &h->vfifo;
	    pthread_create(&h->tvideo,NULL,writer_video_thread,h);
	    pthread_create(&h->tconv,NULL,ng_convert_thread,h->hconv);
	}
#endif
	h->vfmt = *video;
	h->fps  = fps;
    }	
    
    /* open file */
    h->handle = writer->wr_open(moviename,audioname,
				video,priv_video,fps,
				audio,priv_audio);
    if (ng_debug)
	fprintf(stderr,"movie_init_writer end (h=%p)\n",h->handle);
    if (NULL != h->handle)
	return h;

    /* Oops -- wr_open() didn't work.  cleanup.  */
#ifdef NOTDEF
    if (h->afmt.fmtid != AUDIO_NONE) {
	pthread_cancel(h->taudio);
	pthread_join(h->taudio,&dummy);
	oss_close(h->sndhandle);
    }
#endif
    if (h->vfmt.fmtid != VIDEO_NONE) {
	pthread_cancel(h->tvideo);
	pthread_join(h->tvideo,&dummy);
    }
    if (h->tconv) {
	pthread_cancel(h->tconv);
	pthread_join(h->tconv,&dummy);
    }
    free(h);
    return NULL;
}

int
movie_writer_start(struct movie_handle *h)
{
    if (ng_debug)
	fprintf(stderr,"movie_writer_start\n");
    h->start = ng_get_timestamp();
#ifdef NOTDEF
    if (h->afmt.fmtid != AUDIO_NONE)
	oss_startrec(h->sndhandle);
#endif
    if (h->vfmt.fmtid != VIDEO_NONE)
	drv->startvideo(h_drv,h->fps,h->slots);
#ifdef NOTDEF
    if (h->afmt.fmtid != AUDIO_NONE)
	pthread_create(&h->raudio,NULL,record_audio_thread,h);
#endif
    pthread_create(&h->tflush,NULL,flushit,NULL);
    return 0;
}

int
movie_writer_stop(struct movie_handle *h)
{
    long long stopby;
    int frames;
    void *dummy;

    if (ng_debug)
	fprintf(stderr,"movie_writer_stop\n");

    if (h->vfmt.fmtid != VIDEO_NONE && h->afmt.fmtid != AUDIO_NONE) {
	for (frames = 0; frames < 16; frames++) {
	    stopby = (long long)(h->frames + frames) * 1000000000000 / h->fps;
	    if (stopby > h->ats)
		break;
	}
	frames++;
	h->stopby = (long long)(h->frames + frames) * 1000000000000 / h->fps;
	while (frames) {
	    movie_grab_put_video(h,NULL);
	    frames--;
	}
    } else if (h->afmt.fmtid != AUDIO_NONE) {
	h->stopby = h->ats;
    }

    /* send EOF */
    if (h->tconv)
	fifo_put(&h->cfifo,NULL);
    else
	fifo_put(&h->vfifo,NULL);

    /* join threads */
#ifdef NOTDEF
    if (h->afmt.fmtid != AUDIO_NONE) {
	pthread_join(h->raudio,&dummy);
	pthread_join(h->taudio,&dummy);
    }
#endif
    if (h->vfmt.fmtid != VIDEO_NONE)
	pthread_join(h->tvideo,&dummy);
    if (h->tconv)
	pthread_join(h->tconv,&dummy);
    pthread_cancel(h->tflush);
    pthread_join(h->tflush,&dummy);

    /* close file */
    h->writer->wr_close(h->handle);
#ifdef NOTDEF
    if (h->afmt.fmtid != AUDIO_NONE)
	oss_close(h->sndhandle);
#endif
    if (h->vfmt.fmtid != VIDEO_NONE)
	drv->stopvideo(h_drv);
    free(h);
    return 0;
}

/*-------------------------------------------------------------------------*/

static void
movie_print_timestamps(struct movie_handle *h)
{
#ifdef NOTDEF
    char line[128];

    if (NULL == rec_status)
	return;
    h->rts = ng_get_timestamp() - h->start;
    sprintf(line,"real: %d.%03ds   audio: %d.%03ds   video: %d.%03ds",
	    (int)((h->rts / 1000000000)),
	    (int)((h->rts % 1000000000) / 1000000),
	    (int)((h->ats / 1000000000)),
	    (int)((h->ats % 1000000000) / 1000000),
	    (int)((h->vts / 1000000000)),
	    (int)((h->vts % 1000000000) / 1000000));
    rec_status(line);
#endif
}

int
movie_grab_put_video(struct movie_handle *h, struct ng_video_buf **ret)
{
    struct ng_video_buf *buf;
    int expected;

    if (ng_debug > 1)
	fprintf(stderr,"grab_put_video\n");

    /* fetch next frame */
    buf = ng_grabber_grab_image(0);
    if (NULL == buf)
	return -1;

    /* rate control */
    expected = buf->info.ts * h->fps / 1000000000000;
    if (expected < h->frames) {
	if (ng_debug > 1)
	    fprintf(stderr,"rate: ignoring frame\n");
	ng_release_video_buf(buf);
	return 0;
    }
    if (expected > h->frames) {
	fprintf(stderr,"rate: queueing frame twice (%d)\n",
		expected-h->frames);
	buf->info.twice++;
	h->frames++;
    }
    h->frames++;
    h->vts = buf->info.ts;
    buf->info.seq = h->seq++;

    /* return a pointer to the frame if requested */
    if (NULL != ret) {
	buf->refcount++;
	*ret = buf;
    }
    
    /* put into fifo */
    if (h->hconv) {
	fifo_put(&h->cfifo,buf);
    } else {
	fifo_put(&h->vfifo,buf);
    }

    /* feedback */
    movie_print_timestamps(h);
    return h->frames;
}

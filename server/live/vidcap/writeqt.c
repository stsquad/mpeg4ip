#ifdef HAVE_LIBQUICKTIME

#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <quicktime/quicktime.h>

#include "grab-ng.h"

/* ----------------------------------------------------------------------- */

struct qt_video_priv {
    const char codec[4];
    const int libencode;
};

struct qt_audio_priv {
    const char codec[4];
    const int libencode;
};

struct qt_handle {
    /* libquicktime handle */
    quicktime_t *fh;

    /* format */
    struct ng_video_fmt video;
    struct ng_audio_fmt audio;

    /* misc */
    int lib_video;
    int lib_audio;
    int audio_sample;
    unsigned char **rows;
};

/* ----------------------------------------------------------------------- */

void*
qt_open(char *filename, char *dummy,
	struct ng_video_fmt *video, const void *priv_video, int fps,
	struct ng_audio_fmt *audio, const void *priv_audio)
{
    const struct qt_video_priv *pvideo = priv_video;
    const struct qt_audio_priv *paudio = priv_audio;
    struct qt_handle *h;

    if (NULL == (h = malloc(sizeof(*h))))
	return NULL;

    memset(h,0,sizeof(*h));
    h->video      = *video;
    h->audio      = *audio;
    if (h->video.fmtid != VIDEO_NONE)
	h->lib_video  = pvideo->libencode;
    if (h->audio.fmtid != AUDIO_NONE)
	h->lib_audio  = paudio->libencode;

    if (NULL == (h->fh = quicktime_open(filename,0,1))) {
	fprintf(stderr,"quicktime_open failed (%s)\n",filename);
	goto fail;
    }
    if (h->lib_video)
	if (NULL == (h->rows = malloc(h->video.height * sizeof(char*))))
	    goto fail;

    if (h->audio.fmtid != AUDIO_NONE) {
	quicktime_set_audio(h->fh,
			    ng_afmt_to_channels[h->audio.fmtid],
			    h->audio.rate,
			    ng_afmt_to_bits[h->audio.fmtid],
			    (char*)paudio->codec);
	h->audio_sample = ng_afmt_to_channels[h->audio.fmtid] *
	    ng_afmt_to_bits[h->audio.fmtid] / 8;
	if (h->lib_audio && !quicktime_supported_audio(h->fh, 0)) {
	    fprintf(stderr,"libquicktime: audio codec not supported\n");
	    goto fail;
	}
    }
    if (h->video.fmtid != VIDEO_NONE) {
	quicktime_set_video(h->fh,1,h->video.width,h->video.height,fps/1000,
			    (char*)pvideo->codec);
	if (h->lib_video && !quicktime_supported_video(h->fh, 0)) {
	    fprintf(stderr,"libquicktime: video codec not supported\n");
	    goto fail;
	}
    }
    quicktime_set_info(h->fh, "Dumme Bemerkungen gibt's hier umsonst.");
    return h;

 fail:
    if (h->rows)
	free(h->rows);
    free(h);
    return NULL;
}

int
qt_video(void *handle, struct ng_video_buf *buf)
{
    struct qt_handle *h = handle;
    int rc;

    if (h->lib_video) {
	int row,len;
	char *line;

	/* QuickTime library expects an array of pointers to image rows (RGB) */
	len = h->video.width * 3;
	for (row = 0, line = buf->data; row < h->video.height; row++, line += len)
	    h->rows[row] = line;
	rc = quicktime_encode_video(h->fh, h->rows, 0);
    } else {
	rc = quicktime_write_frame(h->fh, buf->data, buf->size, 0);
    }
    return rc;
}

int
qt_audio(void *handle, struct ng_audio_buf *buf)
{
    struct qt_handle *h = handle;
    if (h->lib_audio) {
	/* not used yet */
	return 0;
    } else {
	return quicktime_write_audio(h->fh, buf->data,
				     buf->size / h->audio_sample, 0);
    }
}

int
qt_close(void *handle)
{
    struct qt_handle *h = handle;

    quicktime_close(h->fh);
    if (h->rows)
	free(h->rows);
    return 0;
}

/* ----------------------------------------------------------------------- */

static const struct qt_video_priv qt_raw = {
    codec:     QUICKTIME_RAW,
    libencode: 0,
};
static const struct qt_video_priv qt_yuv2 = {
    codec:     QUICKTIME_YUV2,
    libencode: 0,
};
static const struct qt_video_priv qt_jpeg = {
    codec:     QUICKTIME_JPEG,
    libencode: 0,
};
static const struct qt_video_priv qt_mjpa = {
    codec:     QUICKTIME_MJPA,
    libencode: 1,
};
static const struct qt_video_priv qt_png = {
    codec:     QUICKTIME_PNG,
    libencode: 1,
};
static const struct ng_format_list qt_vformats[] = {
    {
	name:  "raw",
	ext:   "mov",
	fmtid: VIDEO_RGB24,
	priv:  &qt_raw,
#if 0 /* FIXME: looks funny, byteswapped? */
    },{
	name:  "yuv2",
	ext:   "mov",
	fmtid: VIDEO_YUV422,
	priv:  &qt_yuv2,
#endif
    },{
	name:  "jpeg",
	ext:   "mov",
	fmtid: VIDEO_JPEG,
	priv:  &qt_jpeg,
    },{
	name:  "mjpa",
	desc:  "Motion JPEG-A",
	ext:   "mov",
	fmtid: VIDEO_RGB24,
	priv:  &qt_mjpa,
    },{
	name:  "png",
	desc:  "PNG images",
	ext:   "mov",
	fmtid: VIDEO_RGB24,
	priv:  &qt_png,
    },{
	/* EOF */
    }
};

static const struct qt_audio_priv qt_mono8 = {
    codec:     QUICKTIME_RAW,
    libencode: 0,
};
static const struct qt_audio_priv qt_mono16 = {
    codec:	QUICKTIME_TWOS,
    libencode:	0,
};
static const struct qt_audio_priv qt_stereo = {
    codec:	QUICKTIME_TWOS,
    libencode:	0,
};
static const struct ng_format_list qt_aformats[] = {
    {
	name:  "mono8",
	ext:   "mov",
	fmtid: AUDIO_U8_MONO,
	priv:  &qt_mono8,
    },{
        name:  "mono16",
	ext:   "mov",
	fmtid: AUDIO_S16_BE_MONO,
	priv:  &qt_mono16,
    },{
        name:  "stereo",
	ext:   "mov",
	fmtid: AUDIO_S16_BE_STEREO,
	priv:  &qt_stereo,
    },{
	/* EOF */
    }
};

const struct ng_writer qt_writer = {
    name:      "qt",
    desc:      "Apple QuickTime format",
    combined:  1,
    video:     qt_vformats,
    audio:     qt_aformats,
    wr_open:   qt_open,
    wr_video:  qt_video,
    wr_audio:  qt_audio,
    wr_close:  qt_close,
};

#endif /* HAVE_LIBQUICKTIME */

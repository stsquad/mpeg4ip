
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/param.h>
#include <sys/uio.h>
#include <pthread.h>
#ifdef HAVE_ENDIAN_H
# include <endian.h>
#endif
#include "byteswap.h"

#include "grab-ng.h"

#if BYTE_ORDER == BIG_ENDIAN
# define AVI_SWAP2(a) SWAP2((a))
# define AVI_SWAP4(a) SWAP4((a))
#else
# define AVI_SWAP2(a) (a)
# define AVI_SWAP4(a) (a)
#endif

/*
 * M$ vidcap avi video+audio layout
 *
 * riff avi
 *   list hdrl       header
 *     avih          avi header
 *     list strl     video stream header
 *       strh         
 *       strf        
 *     list strl     audio stream header
 *       strh        
 *       strf        
 *     istf          ??? software
 *     idit          ??? timestamp
 *   yunk            ??? 4k page pad
 *   list movi       data
 *     00db          video data
 *     yunk          ??? 4k page pad
 *     [ ... ]
 *     01wb          audio data
 *     [ ... ]
 *   idx1            video frame index
 *
 */

/* ----------------------------------------------------------------------- */

#define TRAP(txt) fprintf(stderr,"%s:%d:%s\n",__FILE__,__LINE__,txt);exit(1);

#define WAVE_FORMAT_PCM                 (0x0001)
#define WAVE_FORMAT_ALAW                (0x0006)
#define WAVE_FORMAT_MULAW               (0x0007)

#define AVIF_HASINDEX                   0x10

typedef unsigned int   uint32;
typedef unsigned short uint16;

struct RIFF_avih {
    uint32 us_frame;          /* microsec per frame */
    uint32 bps;               /* byte/s overall */
    uint32 unknown1;          /* pad_gran (???) */
    uint32 flags;
    uint32 frames;            /* # of frames (all) */
    uint32 init_frames;       /* initial frames (???) */
    uint32 streams;
    uint32 bufsize;           /* suggested buffer size */
    uint32 width;
    uint32 height;
    uint32 scale;
    uint32 rate;
    uint32 start;
    uint32 length;
};

struct RIFF_strh {
    char   type[4];           /* stream type */
    char   handler[4];
    uint32 flags;
    uint32 priority;
    uint32 init_frames;       /* initial frames (???) */
    uint32 scale;
    uint32 rate;
    uint32 start;
    uint32 length;
    uint32 bufsize;           /* suggested buffer size */
    uint32 quality;
    uint32 samplesize;
    /* XXX 16 bytes ? */
};

struct RIFF_strf_vids {       /* == BitMapInfoHeader */
    uint32 size;
    uint32 width;
    uint32 height;
    uint16 planes;
    uint16 bit_cnt;
    char   compression[4];
    uint32 image_size;
    uint32 xpels_meter;
    uint32 ypels_meter;
    uint32 num_colors;        /* used colors */
    uint32 imp_colors;        /* important colors */
    /* may be more for some codecs */
};

struct RIFF_strf_auds {       /* == WaveHeader (?) */
    uint16 format;
    uint16 channels;
    uint32 rate;
    uint32 av_bps;
    uint16 blockalign;
    uint16 size;
};

#define size_strl_vids (sizeof(struct RIFF_strh) + \
			sizeof(struct RIFF_strf_vids) + \
			4*5)
#define size_strl_auds (sizeof(struct RIFF_strh) + \
			sizeof(struct RIFF_strf_auds) + \
			4*5)

static const struct AVI_HDR {
    char                     riff_id[4];
    uint32                   riff_size;
    char                     riff_type[4];

    char                       hdrl_list_id[4];
    uint32                     hdrl_size;
    char                       hdrl_type[4];

    char                         avih_id[4];
    uint32                       avih_size;
    struct RIFF_avih             avih;
} avi_hdr = {
    {'R','I','F','F'}, 0,                             {'A','V','I',' '},
    {'L','I','S','T'}, 0,                             {'h','d','r','l'},
    {'a','v','i','h'}, AVI_SWAP4(sizeof(struct RIFF_avih)),      {}
};

static const struct AVIX_HDR {
    char                     riff_id[4];
    uint32                   riff_size;
    char                     riff_type[4];

    char                       data_list_id[4];
    uint32                     data_size;
    char                       data_type[4];
} avix_hdr = {
    {'R','I','F','F'}, 0,                             {'A','V','I','X'},
    {'L','I','S','T'}, 0,                             {'m','o','v','i'},
};

static const struct AVI_HDR_VIDEO {
    char                         strl_list_id[4];
    uint32                       strl_size;
    char                         strl_type[4];

    char                           strh_id[4];
    uint32                         strh_size;
    struct RIFF_strh               strh;

    char                           strf_id[4];
    uint32                         strf_size;
    struct RIFF_strf_vids          strf;
} avi_hdr_video = {
    {'L','I','S','T'}, AVI_SWAP4(size_strl_vids),                {'s','t','r','l'},
    {'s','t','r','h'}, AVI_SWAP4(sizeof(struct RIFF_strh)),      {{'v','i','d','s'}},
    {'s','t','r','f'}, AVI_SWAP4(sizeof(struct RIFF_strf_vids)), {}
};

static const struct AVI_HDR_AUDIO {
    char                         strl_list_id[4];
    uint32                       strl_size;
    char                         strl_type[4];

    char                           strh_id[4];
    uint32                         strh_size;
    struct RIFF_strh               strh;

    char                           strf_id[4];
    uint32                         strf_size;
    struct RIFF_strf_auds          strf;
} avi_hdr_audio = {
    {'L','I','S','T'}, AVI_SWAP4(size_strl_auds),                {'s','t','r','l'},
    {'s','t','r','h'}, AVI_SWAP4(sizeof(struct RIFF_strh)),      {{'a','u','d','s'}},
    {'s','t','r','f'}, AVI_SWAP4(sizeof(struct RIFF_strf_auds)), {}
};

static const struct AVI_HDR_ODML {
    char                         strl_list_id[4];
    uint32                       strl_size;
    char                         strl_type[4];

    char                           strh_id[4];
    uint32                         strh_size;
    uint32                         total_frames;
} avi_hdr_odml = {
    {'L','I','S','T'}, AVI_SWAP4(sizeof(uint32) + 4*3),  {'o','d','m','l'},
    {'d','m','l','h'}, AVI_SWAP4(sizeof(uint32)),
};

static const struct AVI_DATA {
    char                       data_list_id[4];
    uint32                     data_size;
    char                       data_type[4];

    /* audio+video data follows */
    
} avi_data = {
    {'L','I','S','T'}, 0,                   {'m','o','v','i'},
};

struct CHUNK_HDR {
    char                       id[4];
    uint32                     size;
};

static const struct CHUNK_HDR frame_hdr = {
    {'0','0','d','b'}, 0
};
static const struct CHUNK_HDR sound_hdr = {
    {'0','1','w','b'}, 0
};
static const struct CHUNK_HDR idx_hdr = {
    {'i','d','x','1'}, 0
};

struct IDX_RECORD {
    char                      id[4];
    uint32                    flags;
    uint32                    offset;
    uint32                    size;
};

/* ----------------------------------------------------------------------- */

struct avi_video_priv {
    const char handler[4];
    const char compress[4];
    const int  bytesperpixel;
};

struct avi_handle {
    /* file name+handle */
    char   file[MAXPATHLEN];
    int    fd;
    struct iovec *vec;

    /* format */
    struct ng_video_fmt video;
    struct ng_audio_fmt audio;

    /* headers */
    struct AVI_HDR        avi_hdr;
    struct AVIX_HDR       avix_hdr;
    struct AVI_HDR_ODML   avi_hdr_odml;
    struct AVI_HDR_AUDIO  avi_hdr_audio;
    struct AVI_HDR_VIDEO  avi_hdr_video;
    struct AVI_DATA       avi_data;
    struct CHUNK_HDR      frame_hdr;
    struct CHUNK_HDR      sound_hdr;
    struct CHUNK_HDR      idx_hdr;

    /* statistics -- first chunk */
    int    frames;
    off_t  hdr_size;
    off_t  audio_size;
    off_t  data_size;

    /* statistics -- current chunk */
    int    bigfile;
    int    framesx;
    off_t  avix_start;
    off_t  datax_size;
    
    /* statistics -- total */
    int    frames_total;

    /* frame index */
    struct IDX_RECORD *idx_array;
    int    idx_index, idx_count;
    off_t  idx_offset;
    off_t  idx_size;
};

/* ----------------------------------------------------------------------- */
/* idx1 frame index                                                        */

static void
avi_addindex(struct avi_handle *h, char *fourcc,int flags,int chunksize)
{
    if (h->idx_index == h->idx_count) {
	h->idx_count += 256;
	h->idx_array = realloc(h->idx_array,h->idx_count*sizeof(struct IDX_RECORD));
    }
    memcpy(h->idx_array[h->idx_index].id,fourcc,4);
    h->idx_array[h->idx_index].flags=AVI_SWAP4(flags);
    h->idx_array[h->idx_index].offset=AVI_SWAP4(h->idx_offset-h->hdr_size-8);
    h->idx_array[h->idx_index].size=AVI_SWAP4(chunksize);
    h->idx_index++;
    h->idx_offset += chunksize + sizeof(struct CHUNK_HDR);
}

static void
avi_writeindex(struct avi_handle *h)
{
    /* write frame index */
    h->idx_hdr.size = AVI_SWAP4(h->idx_index * sizeof(struct IDX_RECORD));
    write(h->fd,&h->idx_hdr,sizeof(struct CHUNK_HDR));
    write(h->fd,h->idx_array,h->idx_index*sizeof(struct IDX_RECORD));
    h->idx_size += h->idx_index * sizeof(struct IDX_RECORD)
	+ sizeof(struct CHUNK_HDR);

    /* update header */
    h->avi_hdr.avih.flags       |= AVI_SWAP4(AVIF_HASINDEX);
}   

static void
avi_bigfile(struct avi_handle *h, int last)
{
    off_t avix_end;
    
    if (h->bigfile) {
	/* finish this chunk */
	avix_end = lseek(h->fd,0,SEEK_CUR);
	lseek(h->fd,h->avix_start,SEEK_SET);
	h->avix_hdr.riff_size = h->datax_size + 4*4;
	h->avix_hdr.data_size = h->datax_size + 4;
	write(h->fd,&h->avix_hdr,sizeof(struct AVIX_HDR));
	lseek(h->fd,avix_end,SEEK_SET);
	h->avix_start = avix_end;
    } else {
	h->avix_start = lseek(h->fd,0,SEEK_CUR);
    }
    if (last)
	return;
    h->bigfile++;
    h->framesx = 0;
    h->datax_size = 0;
    write(h->fd,&h->avix_hdr,sizeof(struct AVIX_HDR));
    if (ng_debug)
	fprintf(stderr,"avi bigfile #%d\n",h->bigfile);
}

/* ----------------------------------------------------------------------- */

static void*
avi_open(char *filename, char *dummy,
	 struct ng_video_fmt *video, const void *priv_video, int fps,
	 struct ng_audio_fmt *audio, const void *priv_audio)
{
    const struct avi_video_priv  *pvideo = priv_video;
    struct avi_handle      *h;
    int i,frame_bytes,depth,streams,rate,us_frame;

    if (NULL == (h = malloc(sizeof(*h))))
	return NULL;

    /* init */
    memset(h,0,sizeof(*h));
    h->video         = *video;
    h->audio         = *audio;
    h->avi_hdr       = avi_hdr;
    h->avix_hdr      = avix_hdr;
    h->avi_hdr_odml  = avi_hdr_odml;
    h->avi_hdr_video = avi_hdr_video;
    h->avi_hdr_audio = avi_hdr_audio;
    h->avi_data      = avi_data;
    h->frame_hdr     = frame_hdr;
    h->sound_hdr     = sound_hdr;
    h->idx_hdr       = idx_hdr;
    h->vec           = malloc(sizeof(struct iovec) * video->height);

    strcpy(h->file,filename);
    if (-1 == (h->fd = open(h->file,O_CREAT | O_RDWR | O_TRUNC, 0666))) {
	fprintf(stderr,"open %s: %s\n",h->file,strerror(errno));
	free(h);
	return NULL;
    }

    /* general */
    streams = 0;
    rate = 0;
    if (h->video.fmtid != VIDEO_NONE) {
	streams++;
	rate += pvideo->bytesperpixel * fps / 1000;
	h->avi_hdr.avih.width       = AVI_SWAP4(h->video.width);
	h->avi_hdr.avih.height      = AVI_SWAP4(h->video.height);
    }
    if (h->audio.fmtid != AUDIO_NONE) {
	streams++;
	rate += ng_afmt_to_channels[h->audio.fmtid] *
	    ng_afmt_to_bits[h->audio.fmtid] *
	    h->audio.rate / 8;
    }
    us_frame = (long long)1000000000/fps;
    h->avi_hdr.avih.us_frame    = AVI_SWAP4(us_frame);
    h->avi_hdr.avih.bps         = AVI_SWAP4(rate);
    h->avi_hdr.avih.streams     = AVI_SWAP4(streams);
    h->hdr_size += write(h->fd,&h->avi_hdr,sizeof(struct AVI_HDR));

    /* video */
    if (h->video.fmtid != VIDEO_NONE) {
	for (i = 0; i < 4; i++) {
	    h->avi_hdr_video.strh.handler[i]     = pvideo->handler[i];
	    h->avi_hdr_video.strf.compression[i] = pvideo->compress[i];
	}
	frame_bytes = pvideo->bytesperpixel * h->video.width * h->video.height;
	depth = ng_vfmt_to_depth[h->video.fmtid];
	h->frame_hdr.size                = AVI_SWAP4(frame_bytes);
	h->avi_hdr_video.strh.scale      = AVI_SWAP4(us_frame);
	h->avi_hdr_video.strh.rate       = AVI_SWAP4(1000000);
	h->avi_hdr_video.strf.size       = AVI_SWAP4(sizeof(avi_hdr_video.strf));
	h->avi_hdr_video.strf.width      = AVI_SWAP4(h->video.width);
	h->avi_hdr_video.strf.height     = AVI_SWAP4(h->video.height);
	h->avi_hdr_video.strf.planes     = AVI_SWAP2(1);
	h->avi_hdr_video.strf.bit_cnt    = AVI_SWAP2(depth ? depth : 24);
	h->avi_hdr_video.strf.image_size = AVI_SWAP4(frame_bytes);
	h->hdr_size += write(h->fd,&h->avi_hdr_video,sizeof(struct AVI_HDR_VIDEO));
    }

    /* audio */
    if (h->audio.fmtid != AUDIO_NONE) {
	h->avi_hdr_audio.strh.scale      =
	    AVI_SWAP4(ng_afmt_to_channels[h->audio.fmtid] *
		      ng_afmt_to_bits[h->audio.fmtid] / 8);
	h->avi_hdr_audio.strh.rate       =
	    AVI_SWAP4(ng_afmt_to_channels[h->audio.fmtid] *
		      ng_afmt_to_bits[h->audio.fmtid] *
		      h->audio.rate / 8);
	h->avi_hdr_audio.strh.samplesize =
	    AVI_SWAP4(ng_afmt_to_channels[h->audio.fmtid] *
		      ng_afmt_to_bits[h->audio.fmtid] / 8);
	h->avi_hdr_audio.strf.format     =
	    AVI_SWAP2(WAVE_FORMAT_PCM);
	h->avi_hdr_audio.strf.channels   =
	    AVI_SWAP2(ng_afmt_to_channels[h->audio.fmtid]);
	h->avi_hdr_audio.strf.rate       =
	    AVI_SWAP4(h->audio.rate);
	h->avi_hdr_audio.strf.av_bps     = 
	    AVI_SWAP4(ng_afmt_to_channels[h->audio.fmtid] *
		      ng_afmt_to_bits[h->audio.fmtid] *
		      h->audio.rate / 8);
	h->avi_hdr_audio.strf.blockalign =
	    AVI_SWAP2(ng_afmt_to_channels[h->audio.fmtid] *
		      ng_afmt_to_bits[h->audio.fmtid] / 8);
	h->avi_hdr_audio.strf.size       =
	    AVI_SWAP2(ng_afmt_to_bits[h->audio.fmtid]);
	h->hdr_size += write(h->fd,&h->avi_hdr_audio,
			     sizeof(struct AVI_HDR_AUDIO));
    }
    if (h->video.fmtid != VIDEO_NONE)
	h->hdr_size += write(h->fd,&h->avi_hdr_odml,sizeof(struct AVI_HDR_ODML));

    /* data */
    if (-1 == write(h->fd,&h->avi_data,sizeof(struct AVI_DATA))) {
	fprintf(stderr,"write %s: %s\n",h->file,strerror(errno));
	free(h);
	return NULL;
    }
    h->data_size  = 4; /* list type */

    h->idx_index  = 0;
    h->idx_offset = h->hdr_size + sizeof(struct AVI_DATA);

    return h;
}

static int
avi_video(void *handle, struct ng_video_buf *buf)
{
    struct avi_handle *h = handle;
    struct iovec *line;
    int y,bpl,size=0;

    size = (buf->size + 3) & ~3;
    h->frame_hdr.size = AVI_SWAP4(size);
    if (-1 == write(h->fd,&h->frame_hdr,sizeof(struct CHUNK_HDR))) {
	fprintf(stderr,"write %s: %s\n",h->file,strerror(errno));
	return -1;
    }
    switch (h->video.fmtid) {
    case VIDEO_RGB15_LE:
    case VIDEO_BGR24:
	bpl = h->video.width * ng_vfmt_to_depth[h->video.fmtid] / 8;
	for (line = h->vec, y = h->video.height-1;
	     y >= 0; line++, y--) {
	    line->iov_base = ((unsigned char*)buf->data) + y * bpl;
	    line->iov_len  = bpl;
	}
	if (-1 == writev(h->fd,h->vec,h->video.height)) {
	    fprintf(stderr,"writev %s: %s\n",h->file,strerror(errno));
	    return -1;
	}
	break;
    case VIDEO_MJPEG:
    case VIDEO_JPEG:
	if (-1 == write(h->fd,buf->data,size)) {
	    fprintf(stderr,"write %s: %s\n",h->file,strerror(errno));
	    return -1;
	}
	break;
    }
    h->frames_total += 1;
    if (!h->bigfile) {
	avi_addindex(h,h->frame_hdr.id,0x12,size);
	h->data_size  += size + sizeof(struct CHUNK_HDR);
	h->frames     += 1;
    } else {
	h->datax_size += size + sizeof(struct CHUNK_HDR);
	h->framesx    += 1;
    }
    if ((h->bigfile ? h->datax_size : h->data_size) > 1024*1024*2000)
	avi_bigfile(h,0);
    return 0;
}

static int
avi_audio(void *handle, struct ng_audio_buf *buf)
{
    struct avi_handle *h = handle;

    h->sound_hdr.size = AVI_SWAP4(buf->size);
    if (-1 == write(h->fd,&h->sound_hdr,sizeof(struct CHUNK_HDR))) {
	fprintf(stderr,"write %s: %s\n",h->file,strerror(errno));
	return -1;
    }
    if (-1 == write(h->fd,buf->data,buf->size)) {
	fprintf(stderr,"write %s: %s\n",h->file,strerror(errno));
	return -1;
    }

    if (!h->bigfile) {
	avi_addindex(h,h->sound_hdr.id,0x0,buf->size);
	h->data_size  += buf->size + sizeof(struct CHUNK_HDR);
	h->audio_size += buf->size;
    } else {
	h->datax_size += buf->size + sizeof(struct CHUNK_HDR);
    }
    return 0;
}

static int
avi_close(void *handle)
{
    struct avi_handle *h = handle;

    /* write frame index */
    if (h->video.fmtid != VIDEO_NONE) {
	if (!h->bigfile) {
	    avi_writeindex(h);
	} else {
	    avi_bigfile(h,1);
	    h->idx_size = 0;
	}
    }
    
    /* fill in some statistic values ... */
    h->avi_hdr.riff_size         = AVI_SWAP4(h->hdr_size+h->data_size+h->idx_size);
    h->avi_hdr.hdrl_size         = AVI_SWAP4(h->hdr_size - 4*5);
    h->avi_hdr.avih.frames       = AVI_SWAP4(h->frames);
    if (h->video.fmtid != VIDEO_NONE)
	h->avi_hdr_video.strh.length = AVI_SWAP4(h->frames);
    if (h->audio.fmtid != AUDIO_NONE)
	h->avi_hdr_audio.strh.length =
	    AVI_SWAP4(h->audio_size/h->avi_hdr_audio.strh.scale);
    h->avi_data.data_size        = AVI_SWAP4(h->data_size);
    
    /* ... and write header again */
    lseek(h->fd,0,SEEK_SET);
    write(h->fd,&h->avi_hdr,sizeof(struct AVI_HDR));
    if (h->video.fmtid != VIDEO_NONE)
	write(h->fd,&h->avi_hdr_video,sizeof(struct AVI_HDR_VIDEO));
    if (h->audio.fmtid != AUDIO_NONE)
	write(h->fd,&h->avi_hdr_audio,sizeof(struct AVI_HDR_AUDIO));
    if (h->video.fmtid != VIDEO_NONE) {
	h->avi_hdr_odml.total_frames = h->frames_total;
	write(h->fd,&h->avi_hdr_odml,sizeof(struct AVI_HDR_ODML));
    }
    write(h->fd,&h->avi_data,sizeof(struct AVI_DATA));

    close(h->fd);
    free(h->vec);
    free(h);
    return 0;
}

/* ----------------------------------------------------------------------- */
/* data structures describing our capabilities                             */

static const struct avi_video_priv avi_rgb15 = {
    bytesperpixel:  2,
};
static const struct avi_video_priv avi_rgb24 = {
    bytesperpixel:  3,
};
#ifdef NOTDEF
static const struct avi_video_priv avi_mjpeg = {
    handler:        {'M','J','P','G'},
    compress:       {'M','J','P','G'},
    bytesperpixel:  3,
};
#endif
static const struct ng_format_list avi_vformats[] = {
    {
	name:  "rgb15",
	ext:   "avi",
	fmtid: VIDEO_RGB15_LE,
	priv:  &avi_rgb15,
    },{
	name:  "rgb24",
	ext:   "avi",
	fmtid: VIDEO_BGR24,
	priv:  &avi_rgb24,
    },{
#ifdef NOTDEF
	name:  "mjpeg",
	ext:   "avi",
	fmtid: VIDEO_MJPEG,
	priv:  &avi_mjpeg,
    },{
	name:  "jpeg",
	ext:   "avi",
	fmtid: VIDEO_JPEG,
	priv:  &avi_mjpeg,
    },{
#endif
	/* EOF */
    }
};

static const struct ng_format_list avi_aformats[] = {
    {
	name:  "mono8",
	ext:   "avi",
	fmtid: AUDIO_U8_MONO,
    },{
	name:  "mono16",
	ext:   "avi",
	fmtid: AUDIO_S16_LE_MONO,
    },{
	name:  "stereo",
	ext:   "avi",
	fmtid: AUDIO_S16_LE_STEREO,
    },{
	/* EOF */
    }
};

const struct ng_writer avi_writer = {
    name:      "avi",
    desc:      "Microsoft AVI (RIFF) format",
    combined:  1,
    video:     avi_vformats,
    audio:     avi_aformats,
    wr_open:   avi_open,
    wr_video:  avi_video,
    wr_audio:  avi_audio,
    wr_close:  avi_close,
};

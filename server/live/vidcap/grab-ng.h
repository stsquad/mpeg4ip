/*
 * next generation[tm] xawtv capture interfaces
 *
 * (c) 2001 Gerd Knorr <kraxel@bytesex.org>
 *
 */

extern int  ng_debug;
extern int  ng_chromakey;
extern int  ng_mjpeg_quality;
extern char ng_v4l_conf[256];

/* --------------------------------------------------------------------- */
/* defines                                                               */

#define VIDEO_NONE           0
#define VIDEO_RGB08          1  /* bt848 dithered */
#define VIDEO_GRAY           2
#define VIDEO_RGB15_LE       3  /* 15 bpp little endian */
#define VIDEO_RGB16_LE       4  /* 16 bpp little endian */
#define VIDEO_RGB15_BE       5  /* 15 bpp big endian */
#define VIDEO_RGB16_BE       6  /* 16 bpp big endian */
#define VIDEO_BGR24          7  /* bgrbgrbgrbgr (LE) */
#define VIDEO_BGR32          8  /* bgr-bgr-bgr- (LE) */
#define VIDEO_RGB24          9  /* rgbrgbrgbrgb (BE) */
#define VIDEO_RGB32         10  /* -rgb-rgb-rgb (BE) */
#define VIDEO_LUT2          11  /* lookup-table 2 byte depth */
#define VIDEO_LUT4          12  /* lookup-table 4 byte depth */
#define VIDEO_YUV422	    13  /* YUV 4:2:2 */
#define VIDEO_YUV422P       14  /* YUV 4:2:2 (planar) */
#define VIDEO_YUV420P	    15  /* YUV 4:2:0 (planar) */
#define VIDEO_MJPEG	    16  /* MJPEG (AVI) */
#define VIDEO_JPEG	    17  /* JPEG (JFIF) */
#define VIDEO_FMT_COUNT	    18

#define AUDIO_NONE           0
#define AUDIO_U8_MONO        1
#define AUDIO_U8_STEREO      2
#define AUDIO_S16_LE_MONO    3
#define AUDIO_S16_LE_STEREO  4
#define AUDIO_S16_BE_MONO    5
#define AUDIO_S16_BE_STEREO  6
#define AUDIO_FMT_COUNT      7
#if BYTE_ORDER == BIG_ENDIAN
# define AUDIO_S16_NATIVE_MONO   AUDIO_S16_BE_MONO
# define AUDIO_S16_NATIVE_STEREO AUDIO_S16_BE_STEREO
#endif
#if BYTE_ORDER == LITTLE_ENDIAN
# define AUDIO_S16_NATIVE_MONO   AUDIO_S16_LE_MONO
# define AUDIO_S16_NATIVE_STEREO AUDIO_S16_LE_STEREO
#endif

#define ATTR_TYPE_INTEGER    1   /*  range 0 - 65535  */
#define ATTR_TYPE_CHOICE     2   /*  multiple choice  */
#define ATTR_TYPE_BOOL       3   /*  yes/no           */

#define ATTR_ID_NORM         1
#define ATTR_ID_INPUT        2
#define ATTR_ID_VOLUME       3
#define ATTR_ID_MUTE         4
#define ATTR_ID_AUDIO_MODE   5
#define ATTR_ID_COLOR        6
#define ATTR_ID_BRIGHT       7
#define ATTR_ID_HUE          8
#define ATTR_ID_CONTRAST     9
#define ATTR_ID_COUNT       10

#define CAN_OVERLAY          1
#define CAN_CAPTURE          2
#define CAN_TUNE             4
#define NEEDS_CHROMAKEY      8

/* --------------------------------------------------------------------- */

extern const unsigned int   ng_vfmt_to_depth[];
extern const char*          ng_vfmt_to_desc[];

extern const unsigned int   ng_afmt_to_channels[];
extern const unsigned int   ng_afmt_to_bits[];
extern const char*          ng_afmt_to_desc[];

extern const char*          ng_attr_to_desc[];

/* --------------------------------------------------------------------- */

struct STRTAB {
    long nr;
    const char *str;
};

struct OVERLAY_CLIP {
    int x1,x2,y1,y2;
};

/* --------------------------------------------------------------------- */
/* video data structures                                                 */

struct ng_video_fmt {
    int   fmtid;         /* VIDEO_* */
    int   width;
    int   height;
    int   bytesperline;  /* zero for compressed formats */
};

struct ng_video_buf {
    struct ng_video_fmt  fmt;
    int                  size;
    char                 *data;

    /* meta info for frame */
    struct {
	long long        ts;      /* time stamp */
	int              seq;
	int              twice;
    } info;

    /*
     * the lock is for the reference counter.
     * if the reference counter goes down to zero release()
     * should be called.  priv is for the owner of the
     * buffer (can be used by the release callback)
     */
    pthread_mutex_t      lock;
    pthread_cond_t       cond;
    int                  refcount;
    void                 (*release)(struct ng_video_buf *buf);
    void                 *priv;
};

void ng_init_video_buf(struct ng_video_buf *buf);
void ng_release_video_buf(struct ng_video_buf *buf);
struct ng_video_buf* ng_malloc_video_buf(struct ng_video_fmt *fmt,
					 int size);
void ng_wakeup_video_buf(struct ng_video_buf *buf);
void ng_waiton_video_buf(struct ng_video_buf *buf);


/* --------------------------------------------------------------------- */
/* audio data structures                                                 */

struct ng_audio_fmt {
    int   fmtid;         /* AUDIO_* */
    int   rate;
};

struct ng_audio_buf {
    struct ng_audio_fmt  fmt;
    int                  size;
    char                 *data;

    long long            ts;
};


/* --------------------------------------------------------------------- */
/* someone who receives video and/or audio data (writeavi, ...)          */

struct ng_format_list {
    const char  *name;
    const char  *desc;  /* if standard fmtid description doesn't work
			   because it's converted somehow */
    const char  *ext;
    const int   fmtid;
    const void  *priv;
};

struct ng_writer {
    const char *name;
    const char *desc;
    const struct ng_format_list *video;
    const struct ng_format_list *audio;
    const int combined; /* both audio + video in one file */

    void* (*wr_open)(char *moviename, char *audioname,
		     struct ng_video_fmt *video, const void *priv_video, int fps,
		     struct ng_audio_fmt *audio, const void *priv_audio);
    int (*wr_video)(void *handle, struct ng_video_buf *buf);
    int (*wr_audio)(void *handle, struct ng_audio_buf *buf);
    int (*wr_close)(void *handle);
};


/* --------------------------------------------------------------------- */
/* attributes                                                            */

struct ng_attribute {
    int                  id;
    const char           *name;
    int                  type;
    int                  defval;
    struct STRTAB        *choices;
    const void           *priv;
};

struct ng_attribute* ng_attr_byid(struct ng_attribute *attrs, int id);
struct ng_attribute* ng_attr_byname(struct ng_attribute *attrs, char *name);
const char* ng_attr_getstr(struct ng_attribute *attr, int value);
int ng_attr_getint(struct ng_attribute *attr, char *value);
void ng_attr_listchoices(struct ng_attribute *attr);

/* --------------------------------------------------------------------- */

void ng_ratio_configure(int x, int y);
void ng_ratio_fixup(int *width, int *height, int *xoff, int *yoff);

/* --------------------------------------------------------------------- */
/* capture/overlay interface driver                                      */

struct ng_driver {
    const char *name;

    /* open/close */
    void*  (*open)(char *device);
    int    (*close)(void *handle);

    /* attributes */
    int   (*capabilities)(void *handle);
    struct ng_attribute* (*list_attrs)(void *handle);
    int   (*read_attr)(void *handle, struct ng_attribute*);
    void  (*write_attr)(void *handle, struct ng_attribute*, int val);

    /* overlay */
    int   (*setupfb)(void *handle, struct ng_video_fmt *fmt, void *base);
    int   (*overlay)(void *handle, struct ng_video_fmt *fmt, int x, int y,
		     struct OVERLAY_CLIP *oc, int count, int aspect);
    
    /* capture */
    int   (*setformat)(void *handle, struct ng_video_fmt *fmt);
    int   (*startvideo)(void *handle, int fps, int buffers);
    void  (*stopvideo)(void *handle);
    struct ng_video_buf* (*nextframe)(void *handle); /* video frame */
    struct ng_video_buf* (*getimage)(void *handle);  /* single image */


    /* tuner */
    unsigned long (*getfreq)(void *handle);
    void  (*setfreq)(void *handle, unsigned long freq);
    int   (*is_tuned)(void *handle);
};


/* --------------------------------------------------------------------- */
/* maybe add filters for on-the-fly image processing later               */

struct ng_video_conv {
    int                   fmtid_in;
    int                   fmtid_out;
    void*                 (*init)(struct ng_video_fmt *out,
				  void *priv);
    void                  (*frame)(void *handle,
				   struct ng_video_buf *out,
				   struct ng_video_buf *in);
    void                  (*fini)(void *handle);
    void                  *priv;
};


/* --------------------------------------------------------------------- */

extern const struct ng_driver *ng_drivers[];
extern const struct ng_writer *ng_writers[];

void ng_conv_register(struct ng_video_conv *list, int count);
struct ng_video_conv* ng_conv_find(int out, int *i);

const struct ng_driver*
ng_grabber_open(char *device, struct ng_video_fmt *screen,
		void *base, void **handle);
long long ng_get_timestamp(void);

/* --------------------------------------------------------------------- */

void ng_init(void);

/* --------------------------------------------------------------------- */
/* internal stuff starts here                                            */

/* init functions */
void ng_color_packed_init(void);
void ng_color_yuv2rgb_init(void);

/* for yuv2rgb using lookup tables (color_lut.c, color_yuv2rgb.c) */
unsigned long   ng_lut_red[256];
unsigned long   ng_lut_green[256];
unsigned long   ng_lut_blue[256];

/* color_common.c stuff */
void* ng_packed_init(struct ng_video_fmt *out, void *priv);
void  ng_packed_frame(void *handle, struct ng_video_buf *out,
		      struct ng_video_buf *in);
void* ng_conv_nop_init(struct ng_video_fmt *out, void *priv);
void  ng_conv_nop_fini(void *handle);

#define NG_GENERIC_PACKED			\
	init:           ng_packed_init,		\
	frame:          ng_packed_frame,       	\
	fini:           ng_conv_nop_fini

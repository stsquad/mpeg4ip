#ifndef __AVCODEC_H__
#define __AVCODEC_H__

#include "common.h"

enum CodecID {
    CODEC_ID_NONE, 
    CODEC_ID_MPEG1VIDEO,
    CODEC_ID_H263,
    CODEC_ID_RV10,
    CODEC_ID_MP2,
    CODEC_ID_AC3,
    CODEC_ID_MJPEG,
    CODEC_ID_DIVX,
};

enum CodecType {
    CODEC_TYPE_VIDEO,
    CODEC_TYPE_AUDIO,
};

#define CODEC_FLAG_HQ 0x0001 /* high quality (non real time) encoding */
    
typedef struct AVEncodeContext {
    int bit_rate;
    int rate; /* frames per sec or samples per sec */
    int flags;

    /* video only */
    int width, height;
    int gop_size; /* 0 = intra only */
    
    /* audio only */
    int channels;

	int want_key_frame;	/* mechanism for app to ask for a key frame */

    /* the following data should not be initialized */
    int frame_size; /* in samples, initialized when calling 'init' */
    int frame_number; /* audio or video frame number */
    int key_frame;    /* true if the previous compressed frame was 
                         a key frame (intra, or seekable) */
    int quality;      /* quality of the previous encoded frame 
                         (between 1 (good) and 31 (bad)) */
    struct AVEncoder *codec;
    void *priv_data;
} AVEncodeContext;

typedef struct AVEncoder {
    char *name;
    int type;
    int id;
    int priv_data_size;
    int (*init)(AVEncodeContext *);
    int (*encode)(AVEncodeContext *, UINT8 *buf, int buf_size, void *data);
    int (*close)(AVEncodeContext *);
    struct AVEncoder *next;
} AVEncoder;

extern AVEncoder ac3_encoder;
extern AVEncoder mp2_encoder;
extern AVEncoder mpeg1video_encoder;
extern AVEncoder h263_encoder;
extern AVEncoder rv10_encoder;
extern AVEncoder mjpeg_encoder;
extern AVEncoder divx_encoder;

/* resample.c */

typedef struct {
    /* fractional resampling */
    UINT32 incr; /* fractional increment */
    UINT32 frac;
    int last_sample;
    /* integer down sample */
    int iratio;  /* integer divison ratio */
    int icount, isum;
    int inv;
} ReSampleChannelContext;

typedef struct {
    ReSampleChannelContext channel_ctx[2];
    float ratio;
    /* channel convert */
    int input_channels, output_channels;
} ReSampleContext;

int audio_resample_init(ReSampleContext *s, 
                        int output_channels, int input_channels, 
                        int output_rate, int input_rate);
int audio_resample(ReSampleContext *s, short *output, short *input, int nb_samples);

#endif /* __AVCODEC_H__ */

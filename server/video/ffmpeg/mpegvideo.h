/* mpegencode.c */

/* Start codes. */
#define SEQ_END_CODE		0x000001b7
#define SEQ_START_CODE		0x000001b3
#define GOP_START_CODE		0x000001b8
#define PICTURE_START_CODE	0x00000100
#define SLICE_MIN_START_CODE	0x00000101
#define SLICE_MAX_START_CODE	0x000001af
#define EXT_START_CODE		0x000001b5
#define USER_START_CODE		0x000001b2

/* Macros for picture code type. */
#define I_TYPE 1
#define P_TYPE 2
#define B_TYPE 3

enum OutputFormat {
    FMT_MPEG1,
    FMT_H263,
    FMT_MJPEG,
};

#define MPEG_BUF_SIZE (16 * 1024)

typedef struct MpegEncContext {
    /* the following parameters must be initialized before encoding */
    int width, height; /* picture size. must be a multiple of 16 */
    int gop_size;
    int frame_rate; /* number of frames per second */
    int intra_only; /* if true, only intra pictures are generated */
    int bit_rate;        /* wanted bit rate */
    enum OutputFormat out_format; /* output format */
    int h263_rv10; /* use RV10 variation for H263 */
    int h263_pred; /* use DIVX (aka mpeg4) ac/dc predictions */

    /* the following fields are managed internally by the encoder */

    /* bit output */
    PutBitContext pb;

    /* sequence parameters */
    int picture_number;
    int fake_picture_number; /* picture number at the bitstream frame rate */
    int gop_picture_number; /* index of the first picture of a GOP */
	int num_time_increment_bits;	/* (int)log2(frame_rate) + 1 */
    int mb_width, mb_height;
    UINT8 *new_picture[3];     /* picture to be compressed */
    UINT8 *last_picture[3];    /* previous picture */
    UINT8 *current_picture[3]; /* buffer to store the decompressed current picture */
    int last_dc[3]; /* last DC values for MPEG1 */
    INT16 *dc_val[3]; /* used for mpeg4 DC prediction */
    int y_dc_scale, c_dc_scale;

    int qscale;
    int pict_type;
    int frame_rate_index;
    /* motion compensation */
    int f_code; /* resolution */
    int last_motion_x, last_motion_y;
    INT16 (*motion_val)[2]; /* used for MV prediction */
    int full_search;

    /* macroblock layer */
    int mb_x, mb_y;
    int mb_incr;
    int mb_intra;
    /* matrix transmitted in the bitstream */
    UINT8 init_intra_matrix[64];
    /* precomputed matrix (combine qscale and DCT renorm) */
    int intra_matrix[64];
    int non_intra_matrix[64];
    int block_last_index[6];  /* last non zero coefficient in block */

    void *opaque; /* private data for the user */

    /* bit rate control */
    int I_frame_bits;    /* wanted number of bits per I frame */
    int P_frame_bits;    /* same for P frame */
    long long wanted_bits;
    long long total_bits;
    struct MJpegContext *mjpeg_ctx;
} MpegEncContext;

extern const UINT8 zigzag_direct[64];

/* motion_est.c */

int estimate_motion(MpegEncContext *s, 
                    int mb_x, int mb_y,
                    int *mx_ptr, int *my_ptr);

/* h263enc.c */

void h263_encode_mb(MpegEncContext *s, 
                    DCTELEM block[6][64],
                    int motion_x, int motion_y);
void h263_picture_header(MpegEncContext *s, int picture_number);
void rv10_encode_picture_header(MpegEncContext *s, int picture_number);
void h263_dc_scale(MpegEncContext *s);
void mpeg4_encode_picture_header(MpegEncContext *s, int picture_number);

/* mjpegenc.c */

int mjpeg_init(MpegEncContext *s);
void mjpeg_close(MpegEncContext *s);
void mjpeg_encode_mb(MpegEncContext *s, 
                     DCTELEM block[6][64]);
void mjpeg_picture_header(MpegEncContext *s);
void mjpeg_picture_trailer(MpegEncContext *s);

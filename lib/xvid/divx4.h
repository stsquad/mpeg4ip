#ifndef _DIVX4_H_
#define _DIVX4_H_

#include "mpeg4ip.h"

#ifdef __cplusplus
extern "C" {
#endif 


/*********************************************************************************
 * Decoder part                                                                  *
 *********************************************************************************/

/* decore commands */
#define DEC_OPT_MEMORY_REQS	0
#define DEC_OPT_INIT		1
#define DEC_OPT_RELEASE		2
#define DEC_OPT_SETPP		3 
#define DEC_OPT_SETOUT		4 
#define DEC_OPT_FRAME		5
#define DEC_OPT_FRAME_311	6
#define DEC_OPT_SETPP2		7
#define DEC_OPT_VERSION		8

/* return values */
#define DEC_OK			0
#define DEC_MEMORY		1
#define DEC_BAD_FORMAT		2
#define DEC_EXIT		3

/* yuv colour formats */
#define DEC_YUY2		1
#define DEC_YUV2 		DEC_YUY2
#define DEC_UYVY		2
#define DEC_420			3
#define DEC_YV12		13

/* rgb colour formats */
#define DEC_RGB32		4 
#define DEC_RGB24		5 
#define DEC_RGB555		6 
#define DEC_RGB565		7	

#define DEC_RGB32_INV		8
#define DEC_RGB24_INV		9
#define DEC_RGB555_INV 		10
#define DEC_RGB565_INV 		11

/* return pointers to initial buffers
   equivalent to XVID_CSP_USER */  
#define DEC_USER		12

/* output structure for DEC_USER */
typedef struct
{
	void *y;
	void *u;
	void *v;
	int stride_y;
	int stride_uv;
} DEC_PICTURE;

typedef struct
{
	unsigned long mp4_edged_ref_buffers_size;
	unsigned long mp4_edged_for_buffers_size;
	unsigned long mp4_edged_back_buffers_size;
	unsigned long mp4_display_buffers_size;
	unsigned long mp4_state_size;
	unsigned long mp4_tables_size;
	unsigned long mp4_stream_size;
	unsigned long mp4_reference_size;
} DEC_MEM_REQS;

typedef struct 
{
	void * mp4_edged_ref_buffers;  
	void * mp4_edged_for_buffers; 
	void * mp4_edged_back_buffers;
	void * mp4_display_buffers;
	void * mp4_state;
	void * mp4_tables;
	void * mp4_stream;
	void * mp4_reference;
} DEC_BUFFERS;

typedef struct 
{
	int x_dim; /* frame width */
	int y_dim; /* frame height */
	int output_format;	
	int time_incr;
	DEC_BUFFERS buffers;
} DEC_PARAM;

typedef struct
{
	void *bmp; /* pointer to output buffer */
	void *bitstream; /* input bit stream */
	long length; /* length of bitstream */
	int render_flag;
	unsigned int stride;
} DEC_FRAME;

typedef struct
{
	int intra;
	int *quant_store;
	int quant_stride;
} DEC_FRAME_INFO;

/* configure postprocessing */
typedef struct
{
	int postproc_level; /* ranging from 0 to 100 */ 
} DEC_SET;

int decore( unsigned long handle, unsigned long dec_opt, void* param1, void* param2);



/*********************************************************************************
 * Encoder part                                                                  *
 *********************************************************************************/

/**
    Structure passed as an argument when creating encoder.
    You have to initialize at least x_dim and y_dim ( valid range:
	0<x_dim<=1920, 0<y_dim<=1280, both dimensions should be even ).
    You can set all other values to 0, in which case they'll be initialized
    to default values, or specify them directly.
    On success 'handle' member will contain non-zero handle to initialized 
    encoder.
**/
typedef struct _ENC_PARAM_
{
	int x_dim;		// the x dimension of the frames to be encoded
	int y_dim;		// the y dimension of the frames to be encoded
	float framerate;	// the frame rate of the sequence to be encoded, in frames/second
	int bitrate;		// the bitrate of the target encoded stream, in bits/second
	int rc_period;		// the intended rate control averaging period
	int rc_reaction_period;	// the reaction period for rate control
	int rc_reaction_ratio;	// the ratio for down/up rate control
	int max_quantizer;	// the upper limit of the quantizer
	int min_quantizer;	// the lower limit of the quantizer
	int max_key_interval;	// the maximum interval between key frames
	int use_bidirect;	// use bidirectional coding
	int deinterlace;	// fast deinterlace
	int quality;		// the quality of compression ( 1 - fastest, 5 - best )
	int obmc;			// flag to enable overlapped block motion compensation mode
	void *handle;		// will be filled by encore
}
ENC_PARAM;


// encore2


/**
    Structure passed as a first argument when encoding a frame.
    Both pointers should be non-NULL. You are responsible for allocation
    of bitstream buffer, its size should be large enough to hold a frame.
    Checks for buffer overflow are too expensive and it will be almost
    impossible to recover from such overflow. Thus, no checks for buffer
    overflow will be done.
    Theoretical upper limit of frame size is around 6 bytes/pixel
    or 2.5 Mb for 720x576 frame.
    On success 'length' will contain number of bytes written into the stream.
**/
typedef struct _ENC_FRAME_
{
    void *image;	// the image frame to be encoded
    void *bitstream;	// the buffer for encoded bitstream
    int length;		// the length of the encoded bitstream
    int colorspace;	// the format of image frame
    int quant;		// quantizer for this frame; only used in VBR modes
    int intra;		// force this frame to be intra/inter; only used in VBR 2-pass
    void *mvs;		// optional pointer to array of motion vectors
#ifdef MPEG4IP
  int general;
#endif
}
ENC_FRAME;

#ifdef MPEG4IP
#define DEC_SHORT_HEADERS 1
#endif
/**
    Structure passed as a second optional argument when encoding a frame.
    On successful return its members are filled with parameters of encoded
    stream.
**/
    typedef struct _ENC_RESULT_
    {
	int is_key_frame;	// the current frame is encoded as a key frame
	int quantizer;		// the quantizer used for this frame
	int texture_bits;	// amount of bits spent on coding DCT coeffs
	int motion_bits;	// amount of bits spend on coding motion
	int total_bits;		// sum of two previous fields
    }
    ENC_RESULT;

// the prototype of the encore() - main encode engine entrance
int encore(void *handle,	// handle               - the handle of the calling entity, must be unique
	       int enc_opt,	// enc_opt              - the option for encoding, see below
	       void *param1,	// param1               - the parameter 1 (its actually meaning depends on enc_opt
	       void *param2);	// param2               - the parameter 2 (its actually meaning depends on enc_opt

// encore options (the enc_opt parameter of encore())
#define ENC_OPT_INIT    0	// initialize the encoder, return a handle
#define ENC_OPT_RELEASE 1	// release all the resource associated with the handle
#define ENC_OPT_ENCODE  2	// encode a single frame
#define ENC_OPT_ENCODE_VBR 3	// encode a single frame, not using internal rate control algorithm
#define ENC_OPT_VERSION	4

#define ENCORE_VERSION		20010807
	
     

// return code of encore()
#define ENC_FAIL		-1
#define ENC_OK			0
#define	ENC_MEMORY		1
#define ENC_BAD_FORMAT		2

/** Common 24-bit RGB, order of components b-g-r **/
#define ENC_CSP_RGB24 	0

/** Planar YUV, U & V subsampled by 2 in both directions, 
    average 12 bit per pixel; order of components y-v-u **/
#define ENC_CSP_YV12	1

/** Packed YUV, U and V subsampled by 2 horizontally,
    average 16 bit per pixel; order of components y-u-y-v **/
#define ENC_CSP_YUY2	2

/** Same as above, but order of components is u-y-v-y **/
#define ENC_CSP_UYVY	3

/** Same as ENC_CSP_YV12, but chroma components are swapped ( order y-u-v ) **/
#define ENC_CSP_I420	4

/** Same as above **/
#define ENC_CSP_IYUV	ENC_CSP_I420

#ifdef __cplusplus
}
#endif 

#endif // _DIVX4_H_


// This is the header file describing 
// the entrance function of the encoder core
// or the encore ...

#ifdef __cplusplus
extern "C" {
#endif 

typedef struct _ENC_PARAM_ {
	int x_dim;		// the x dimension of the frames to be encoded
	int y_dim;		// the y dimension of the frames to be encoded
	float framerate;// the frame rate of the sequence to be encoded
	long bitrate;	// the bitrate of the target encoded stream
	long rc_period; // the intended rate control averaging period
	long rc_reaction_period; // the reation period for rate control
	long rc_reaction_ratio; // the ratio for down/up rate control
	long max_key_interval; // the maximum interval between key frames
	int max_quantizer; // the upper limit of the quantizer
	int min_quantizer; // the lower limit of the quantizer
	int search_range; // the forward search range for motion estimation
#ifdef MPEG4IP
	int enable_8x8_mv; // control generation of motion vectors
	int raw_y_dim;		// the y dimension of the raw frames to be encoded
	// when set to > y_dim causes raw frames to be vertically cropped
#endif
} ENC_PARAM;

typedef struct _ENC_FRAME_ {
	void *image;	// the image frame to be encoded
	void *bitstream;// the buffer for encoded bitstream
	long length;	// the length of the encoded bitstream
} ENC_FRAME;

typedef struct _ENC_RESULT_ {
	int isKeyFrame; // the current frame is encoded as a key frame
#ifdef MPEG4IP
	// reconstructed image planes
	// pointers only guaranteed to be valid until next call of encore
	void *reconstruct_y;
	void *reconstruct_u;
	void *reconstruct_v;
#endif
} ENC_RESULT;

// the prototype of the encore() - main encode engine entrance
int encore(
			unsigned long handle,	// handle		- the handle of the calling entity, must be unique
			unsigned long enc_opt,	// enc_opt		- the option for encoding, see below
			void *param1,			// param1		- the parameter 1 (its actually meaning depends on enc_opt
			void *param2);			// param2		- the parameter 2 (its actually meaning depends on enc_opt

// encore options (the enc_opt parameter of encore())
#define ENC_OPT_WRITE	1024	// write the reconstruct image to files (for debuging)
#define ENC_OPT_INIT	32768	// initialize the encoder for an handle
#define ENC_OPT_RELEASE 65536	// release all the resource associated with the handle

// return code of encore()
#define ENC_OK			0
#define	ENC_MEMORY		1
#define ENC_BAD_FORMAT	2

#ifdef __cplusplus
}
#endif 






#ifndef _XVID_H_
#define _XVID_H_

#ifdef __cplusplus
extern "C" {
#endif 

// ==========================================
//	global
// ==========================================

// API Version: 1.0
#define API_VERSION ((1 << 16) | (0))

// cpu features
#define XVID_CPU_MMX		0x00000001
#define XVID_CPU_MMXEXT		0x00000002
#define XVID_CPU_SSE		0x00000004
#define XVID_CPU_SSE2		0x00000008
#define XVID_CPU_3DNOW		0x00000010
#define XVID_CPU_3DNOWEXT	0x00000020
#define XVID_CPU_TSC		0x00000040
#define XVID_CPU_FORCE		0x80000000

// colorspaces
#define XVID_CSP_RGB24 	0
#define XVID_CSP_YV12	1
#define XVID_CSP_YUY2	2
#define XVID_CSP_UYVY	3
#define XVID_CSP_I420	4
#define XVID_CSP_RGB555	10
#define XVID_CSP_RGB565	11
#define XVID_CSP_USER	12
#define XVID_CSP_YVYU	1002
#define XVID_CSP_RGB32 	1000
#define XVID_CSP_NULL 	9999

#define XVID_CSP_VFLIP	0x80000000	// flip mask

// error
#define XVID_ERR_FAIL		-1
#define XVID_ERR_OK			0
#define	XVID_ERR_MEMORY		1
#define XVID_ERR_FORMAT		2

typedef struct 
{
	int cpu_flags;
	int api_version;
	int core_build;
} XVID_INIT_PARAM;

int xvid_init(void *handle, int opt,
			  void *param1, void *param2);


// ==========================================
//	decoder
// ==========================================

typedef struct 
{
	int width;
	int height;
	void *handle;
} XVID_DEC_PARAM;


typedef struct
{
	void * bitstream;
	int length;

	void * image;
	int stride;
	int colorspace;
} XVID_DEC_FRAME;


// decoder options
#define XVID_DEC_DECODE		0
#define XVID_DEC_CREATE		1
#define XVID_DEC_DESTROY	2

int xvid_decore(void * handle,
		int opt,
		void * param1,
		void * param2);


// ==========================================
//	encoder
// ==========================================


typedef struct
{
	int width, height;
#ifdef MPEG4IP
	int raw_height;		// height of raw image, >= height
#endif
	int fincr, fbase;	// frame increment, fbase. each frame = "fincr/fbase" seconds
	int bitrate;		// the bitrate of the target encoded stream, in bits/second
	int rc_period;		// the intended rate control averaging period
	int rc_reaction_period;	// the reaction period for rate control
	int rc_reaction_ratio;	// the ratio for down/up rate control
	int max_quantizer;	// the upper limit of the quantizer
	int min_quantizer;	// the lower limit of the quantizer
	int max_key_interval;	// the maximum interval between key frames
	int motion_search;		// the quality of compression ( 1 - fastest, 5 - best )
	int lum_masking;	// lum masking on/off
	int quant_type;		// 0=h.263, 1=mpeg4

	void * handle;		// [out] encoder instance handle
						
} XVID_ENC_PARAM;


typedef struct
{
    void * bitstream;		// [in] bitstream ptr
	int length;				// [out] bitstream length (bytes)

	void * image;			// [in] image ptr
    int colorspace;			// [in] source colorspace

    int quant;				// [in] frame quantizer (vbr)
    int intra;				// [in]	force intra frame (vbr only)
							// [out] intra state
} XVID_ENC_FRAME;


typedef struct
{
	int quant;					// [out] frame quantizer
	int hlength;				// [out] header length (bytes)
	int kblks, mblks, ublks;	// [out]

#ifdef MPEG4IP
	// [out] reconstructed image
	void* image_y;
	void* image_u;
	void* image_v;
	int stride_y;
	int stride_uv;
#endif
	
} XVID_ENC_STATS;


#define XVID_ENC_ENCODE		0
#define XVID_ENC_CREATE		1
#define XVID_ENC_DESTROY	2

int xvid_encore(void * handle,
		int opt,
		void * param1,
		void * param2);


#ifdef __cplusplus
}
#endif 

#endif /* _XVID_H_ */

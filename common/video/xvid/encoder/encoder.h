/**************************************************************************
 *
 *  Modifications:
 *
 *  22.08.2001 added support for EXT_MODE encoding mode
 *             support for EXTENDED API
 *  22.08.2001 fixed bug in iDQtab
 *
 *  Michael Militzer <isibaar@videocoding.de>
 *
 **************************************************************************/

#ifndef _ENCODER_H_
#define _ENCODER_H_


#include "../xvid.h"

#include "../portab.h"
#include "../common/common.h"
#include "../image/image.h"


typedef uint32_t bool;


typedef enum
{
    I_VOP = 0,
    P_VOP = 1
}
VOP_TYPE;


typedef struct
{
	uint32_t buf;
	uint32_t pos;
	uint32_t * start;
	uint32_t * tail;
}
Bitstream;


/***********************************

       Encoding Parameters

************************************/ 

typedef struct
{
    uint32_t width;
    uint32_t height;

#ifdef MPEG4IP
	uint32_t raw_height;
#endif

	uint32_t edged_width;
	uint32_t edged_height;
	uint32_t mb_width;
	uint32_t mb_height;

    VOP_TYPE coding_type;

    /* rounding type; alternate 0-1 after each interframe */

    uint32_t rounding_type;

	/* 1 <= fixed_code <= 4
	   automatically adjusted using motion vector statistics inside
	 */

    uint32_t fixed_code;
    uint32_t quality;
    uint32_t quant;
	uint32_t quant_type;

} MBParam;



/***********************************

Rate Control Parameters

************************************/ 


typedef struct
{
    double quant;
    uint32_t rc_period;
    double target_rate;
    double average_rate;
    double reaction_rate;
    double average_delta;
    double reaction_delta;
    double reaction_ratio;
    uint32_t max_key_interval;
    uint8_t max_quant;
    uint8_t min_quant;
}
RateCtlParam;


typedef struct
{
    int iTextBits;
    float fMvPrevSigma;
    int iMvSum;
    int iMvCount;
	int kblks;
	int mblks;
	int ublks;
}
Statistics;



typedef struct
{
    MBParam mbParam;
    RateCtlParam rateCtlParam;

    int iFrameNum;
    int iMaxKeyInterval;
	int lum_masking;
	int bitrate;

	// images

    IMAGE sCurrent;
    IMAGE sReference;
    IMAGE vInterH;
    IMAGE vInterV;
    IMAGE vInterHV;

	// macroblock

	MACROBLOCK * pMBs;

#ifdef MPEG4IP
	int time_incr_bits;
#endif

    Statistics sStat;
}
Encoder;


// indicates no quantizer changes in INTRA_Q/INTER_Q modes
#define NO_CHANGE 64

void init_encoder(uint32_t cpu_flags);

int encoder_create(XVID_ENC_PARAM * pParam);
int encoder_destroy(Encoder * pEnc);
int encoder_encode(Encoder * pEnc, XVID_ENC_FRAME * pFrame, XVID_ENC_STATS * pResult);
		

#endif /* _ENCODER_H_ */

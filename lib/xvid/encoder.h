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


#include "xvid.h"

#include "portab.h"
#include "global.h"
#include "image/image.h"


#define H263_QUANT	0
#define MPEG4_QUANT	1


typedef uint32_t bool;


typedef enum
{
    I_VOP = 0,
    P_VOP = 1
}
VOP_TYPE;

/***********************************

       Encoding Parameters

************************************/ 

typedef struct
{
    uint32_t width;
    uint32_t height;

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
    uint32_t quant;
	uint32_t quant_type;
	uint32_t motion_flags;	
	uint32_t global_flags;

#ifdef MPEG4IP
	uint32_t raw_height;
	uint16_t fincr;
	uint16_t fbase;
	uint8_t  time_inc_bits;
#endif

	HINTINFO * hint;
} MBParam;

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

    int iFrameNum;
    int iMaxKeyInterval;
	int lum_masking;
	int bitrate;

	// images

    IMAGE sCurrent;
    IMAGE sReference;
#ifdef _DEBUG
	IMAGE sOriginal;
#endif
    IMAGE vInterH;
    IMAGE vInterV;
	IMAGE vInterVf;
    IMAGE vInterHV;
	IMAGE vInterHVf;

	// macroblock

	MACROBLOCK * pMBs;

    Statistics sStat;
}
Encoder;


// indicates no quantizer changes in INTRA_Q/INTER_Q modes
#define NO_CHANGE 64

void init_encoder(uint32_t cpu_flags);

int encoder_create(XVID_ENC_PARAM * pParam);
int encoder_destroy(Encoder * pEnc);
int encoder_encode(Encoder * pEnc, XVID_ENC_FRAME * pFrame, XVID_ENC_STATS * pResult);
		
static __inline uint8_t get_fcode(uint16_t sr)
{
    if (sr <= 16)
		return 1;

    else if (sr <= 32) 
		return 2;

    else if (sr <= 64)
		return 3;

    else if (sr <= 128)
		return 4;

    else if (sr <= 256)
		return 5;

    else if (sr <= 512)
		return 6;

    else if (sr <= 1024)
		return 7;

    else
		return 0;
}

#endif /* _ENCODER_H_ */

#ifndef _DECODER_H_
#define _DECODER_H_

#include "../xvid.h"

#include "../portab.h"
#include "../common/common.h"
#include "../image/image.h"


typedef struct
{
	// bitstream

	uint32_t shape;
	uint32_t time_inc_bits;
	uint32_t quant_bits;
	uint32_t quant_type;
	uint32_t quarterpel;

	// image

	uint32_t width;
	uint32_t height;
	uint32_t edged_width;
	uint32_t edged_height;
	
	IMAGE cur;
	IMAGE refn;
	IMAGE refh;
	IMAGE refv;
	IMAGE refhv;

	// macroblock

	uint32_t mb_width;
	uint32_t mb_height;
	MACROBLOCK * mbs;

	
} DECODER;

void init_decoder(uint32_t cpu_flags);

int decoder_create(XVID_DEC_PARAM * param);
int decoder_destroy(DECODER * dec);
int decoder_decode(DECODER * dec, XVID_DEC_FRAME * frame);


#endif /* _DECODER_H_ */

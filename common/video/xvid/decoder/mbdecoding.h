#ifndef _DECODER_MBDECODING_H_
#define _DECODER_MBDECODING_H_

#include "../portab.h"
#include "bitreader.h"
#include "decoder.h"

int get_mcbpc_intra(BITREADER * bs);
int get_mcbpc_inter(BITREADER * bs);
int get_cbpy(BITREADER * bs, int intra);
int get_mv(BITREADER * bs, int fcode);

int get_dc_dif(BITREADER * bs, uint32_t dc_size);
int get_dc_size_lum(BITREADER * bs);
int get_dc_size_chrom(BITREADER * bs);

int get_intra_coeff(BITREADER * bs, int *run, int *last);
int get_inter_coeff(BITREADER * bs, int *run, int *last);

void get_intra_block(BITREADER * bs, int16_t * block, int direction);
void get_inter_block(BITREADER * bs, int16_t * block);


#endif /* _DECODER_MBDECODING_H_ */

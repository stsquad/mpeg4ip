#ifndef _MB_CODING_H_
#define _MB_CODING_H_

#include "../portab.h"
#include "../global.h"
#include "bitstream.h"

void init_vlc_tables(void);

int get_mcbpc_intra(Bitstream * bs);
int get_mcbpc_inter(Bitstream * bs);
int get_cbpy(Bitstream * bs, int intra);
int get_mv(Bitstream * bs, int fcode);

int get_dc_dif(Bitstream * bs, uint32_t dc_size);
int get_dc_size_lum(Bitstream * bs);
int get_dc_size_chrom(Bitstream * bs);

int get_coeff(Bitstream * bs, int *run, int *last, int intra, int short_video_header);

void get_intra_block(Bitstream * bs, int16_t * block, int direction, int coeff);
void get_inter_block(Bitstream * bs, int16_t * block);

#endif /* _MB_CODING_H_ */

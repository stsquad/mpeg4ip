
#ifndef USE_MMX
void fdct_enc(short *block);
void init_fdct_enc(void);
void idct_enc(short *block);
void init_idct_enc(void);
#else
void fdct_mmx(short *blk);
void idct_mmx(short *blk);
#endif


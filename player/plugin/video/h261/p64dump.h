
class P64Dumper: public FullP64Decoder {
    public:
	P64Dumper();
	P64Dumper(int q);
	int decode(const u_char* bp, int cc, int sbit, int ebit,
		   int mba, int gob, int mq, int mvdh, int mvdv);
   protected:
	void err(const char* msg ...) const;
	void dump_bits(char c);
#ifdef INT_64
	int parse_block(short* blk, INT_64* mask);
#else
	int parse_block(short* blk, u_int* mask);
#endif
	void decode_block(u_int tc, u_int x, u_int y, u_int stride,
			  u_char* front, u_char* back, int sf, int n);
	int parse_picture_hdr();
	int parse_sc();
	int parse_gob_hdr(int);
	int parse_mb_hdr(u_int& cbp);

	int decode_gob(u_int gob);
	int decode_mb();

	u_int dbb_;		/* 32-bit bit buffer */
	int dnbb_;		/* number bits in bit buffer */
	const u_short* dbs_;	/* input bit stream (less bits in bb_) */
	int dump_quantized_;	/* dump quantized coef. values if = 1 */
};

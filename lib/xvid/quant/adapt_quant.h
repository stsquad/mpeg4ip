#ifndef _ADAPT_QUANT_H_
#define _ADAPT_QUANT_H_

int adaptive_quantization(unsigned char* buf, int stride, int* intquant, 
        int framequant, int min_quant, int max_quant,
		int mb_width, int mb_height);  // no qstride because normalization

#endif /* _ADAPT_QUANT_H_ */
